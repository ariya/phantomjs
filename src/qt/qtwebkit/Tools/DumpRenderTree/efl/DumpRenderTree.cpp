/*
 * Copyright (C) 2011 ProFUSION Embedded Systems
 * Copyright (C) 2011 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DumpRenderTree.h"

#include "DumpHistoryItem.h"
#include "DumpRenderTreeChrome.h"
#include "DumpRenderTreeView.h"
#include "EventSender.h"
#include "FontManagement.h"
#include "NotImplemented.h"
#include "PixelDumpSupport.h"
#include "TestRunner.h"
#include "WebCoreSupport/DumpRenderTreeSupportEfl.h"
#include "WebCoreTestSupport.h"
#include "WorkQueue.h"
#include "ewk_private.h"
#include <EWebKit.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_File.h>
#include <Edje.h>
#include <Evas.h>
#include <fontconfig/fontconfig.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <wtf/Assertions.h>
#include <wtf/OwnPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

OwnPtr<DumpRenderTreeChrome> browser;
Evas_Object* topLoadingFrame = 0;
bool waitForPolicy = false;
bool policyDelegateEnabled = false;
bool policyDelegatePermissive = false;
Ecore_Timer* waitToDumpWatchdog = 0;
extern Ewk_History_Item* prevTestBFItem;

// From the top-level DumpRenderTree.h
RefPtr<TestRunner> gTestRunner;
volatile bool done = false;

static bool dumpPixelsForCurrentTest;
static int dumpPixelsForAllTests = false;
static int dumpTree = true;
static int printSeparators = true;
static int useTimeoutWatchdog = true;

static String dumpFramesAsText(Evas_Object* frame)
{
    if (!frame)
        return String();

    String result;
    const char* frameContents = ewk_frame_plain_text_get(frame);

    if (!frameContents)
        return String();

    if (browser->mainFrame() != frame) {
        result.append("\n--------\nFrame: '");
        result.append(String::fromUTF8(ewk_frame_name_get(frame)));
        result.append("'\n--------\n");
    }

    result.append(String::fromUTF8(frameContents));
    result.append("\n");
    eina_stringshare_del(frameContents);

    if (gTestRunner->dumpChildFramesAsText()) {
        Eina_List* children = DumpRenderTreeSupportEfl::frameChildren(frame);
        void* iterator;

        EINA_LIST_FREE(children, iterator) {
            Evas_Object* currentFrame = static_cast<Evas_Object*>(iterator);
            String tempText(dumpFramesAsText(currentFrame));

            if (tempText.isEmpty())
                continue;

            result.append(tempText);
        }
    }

    return result;
}

static void dumpFrameScrollPosition(Evas_Object* frame)
{
    int x, y;
    ewk_frame_scroll_pos_get(frame, &x, &y);
    if (abs(x) > 0 || abs(y) > 0) {
        StringBuilder result;

        Evas_Object* parent = evas_object_smart_parent_get(frame);

        // smart parent of main frame is view object.
        if (parent != browser->mainView()) {
            result.append("frame '");
            result.append(ewk_frame_name_get(frame));
            result.append("' ");
        }

        result.append("scrolled to ");
        result.append(WTF::String::number(x));
        result.append(",");
        result.append(WTF::String::number(y));
        result.append("\n");

        printf("%s", result.toString().utf8().data());
    }

    if (gTestRunner->dumpChildFrameScrollPositions()) {
        Eina_List* children = DumpRenderTreeSupportEfl::frameChildren(frame);
        void* iterator;

        EINA_LIST_FREE(children, iterator) {
            Evas_Object* currentFrame = static_cast<Evas_Object*>(iterator);
            dumpFrameScrollPosition(currentFrame);
        }
    }
}

static bool shouldLogFrameLoadDelegates(const String& pathOrURL)
{
    return pathOrURL.contains("loading/");
}

static bool shouldDumpAsText(const String& pathOrURL)
{
    return pathOrURL.contains("dumpAsText/");
}

static bool shouldOpenWebInspector(const String& pathOrURL)
{
    return pathOrURL.contains("inspector/");
}

static void sendPixelResultsEOF()
{
    puts("#EOF");
    fflush(stdout);
    fflush(stderr);
}

bool shouldSetWaitToDumpWatchdog()
{
    return !waitToDumpWatchdog && useTimeoutWatchdog;
}

static void invalidateAnyPreviousWaitToDumpWatchdog()
{
    if (waitToDumpWatchdog) {
        ecore_timer_del(waitToDumpWatchdog);
        waitToDumpWatchdog = 0;
    }
    waitForPolicy = false;
}

static void onEcoreEvasResize(Ecore_Evas* ecoreEvas)
{
    int width, height;

    ecore_evas_geometry_get(ecoreEvas, 0, 0, &width, &height);
    evas_object_move(browser->mainView(), 0, 0);
    evas_object_resize(browser->mainView(), width, height);
}

static void onCloseWindow(Ecore_Evas*)
{
    notImplemented();
}

static Eina_Bool useLongRunningServerMode(int argc, char** argv)
{
    return (argc == optind + 1 && !strcmp(argv[optind], "-"));
}

static bool parseCommandLineOptions(int argc, char** argv)
{
    static const option options[] = {
        {"notree", no_argument, &dumpTree, false},
        {"pixel-tests", no_argument, &dumpPixelsForAllTests, true},
        {"tree", no_argument, &dumpTree, true},
        {"no-timeout", no_argument, &useTimeoutWatchdog, false},
        {0, 0, 0, 0}
    };

    int option;
    while ((option = getopt_long(argc, (char* const*)argv, "", options, 0)) != -1) {
        switch (option) {
        case '?':
        case ':':
            return false;
        }
    }

    return true;
}

static inline bool isGlobalHistoryTest(const String& cTestPathOrURL)
{
    return cTestPathOrURL.contains("/globalhistory/");
}

static void createTestRunner(const String& testURL, const String& expectedPixelHash)
{
    gTestRunner =
        TestRunner::create(std::string(testURL.utf8().data()),
                                     std::string(expectedPixelHash.utf8().data()));

    topLoadingFrame = 0;
    done = false;

    gTestRunner->setIconDatabaseEnabled(false);

    if (shouldLogFrameLoadDelegates(testURL))
        gTestRunner->setDumpFrameLoadCallbacks(true);

    gTestRunner->setDeveloperExtrasEnabled(true);
    if (shouldOpenWebInspector(testURL))
        gTestRunner->showWebInspector();

    gTestRunner->setDumpHistoryDelegateCallbacks(isGlobalHistoryTest(testURL));

    if (shouldDumpAsText(testURL)) {
        gTestRunner->setDumpAsText(true);
        gTestRunner->setGeneratePixelResults(false);
    }
}

static String getFinalTestURL(const String& testURL)
{
    if (!testURL.startsWith("http://") && !testURL.startsWith("https://")) {
        char* cFilePath = ecore_file_realpath(testURL.utf8().data());
        const String filePath = String::fromUTF8(cFilePath);
        free(cFilePath);

        if (ecore_file_exists(filePath.utf8().data()))
            return String("file://") + filePath;
    }

    return testURL;
}

static void runTest(const char* inputLine)
{
    TestCommand command = parseInputLine(inputLine);
    const String testPathOrURL(command.pathOrURL.c_str());
    ASSERT(!testPathOrURL.isEmpty());
    dumpPixelsForCurrentTest = command.shouldDumpPixels || dumpPixelsForAllTests;
    const String expectedPixelHash(command.expectedPixelHash.c_str());

    // Convert the path into a full file URL if it does not look
    // like an HTTP/S URL (doesn't start with http:// or https://).
    const String testURL = getFinalTestURL(testPathOrURL);

    browser->resetDefaultsToConsistentValues();
    createTestRunner(testURL, expectedPixelHash);

    WorkQueue::shared()->clear();
    WorkQueue::shared()->setFrozen(false);

    const bool isSVGW3CTest = testURL.contains("svg/W3C-SVG-1.1");
    const int width = isSVGW3CTest ? TestRunner::w3cSVGViewWidth : TestRunner::viewWidth;
    const int height = isSVGW3CTest ? TestRunner::w3cSVGViewHeight : TestRunner::viewHeight;
    evas_object_resize(browser->mainView(), width, height);

    if (prevTestBFItem)
        ewk_history_item_free(prevTestBFItem);
    const Ewk_History* history = ewk_view_history_get(browser->mainView());
    prevTestBFItem = ewk_history_history_item_current_get(history);

    evas_object_focus_set(browser->mainView(), EINA_TRUE);
    ewk_view_uri_set(browser->mainView(), testURL.utf8().data());

    ecore_main_loop_begin();

    gTestRunner->closeWebInspector();
    gTestRunner->setDeveloperExtrasEnabled(false);

    browser->clearExtraViews();

    // FIXME: Move to DRTChrome::resetDefaultsToConsistentValues() after bug 85209 lands.
    WebCoreTestSupport::resetInternalsObject(DumpRenderTreeSupportEfl::globalContextRefForFrame(browser->mainFrame()));

    ewk_view_uri_set(browser->mainView(), "about:blank");

    gTestRunner.clear();
    sendPixelResultsEOF();
}

static void runTestingServerLoop()
{
    char filename[PATH_MAX];

    while (fgets(filename, sizeof(filename), stdin)) {
        char* newLine = strrchr(filename, '\n');
        if (newLine)
            *newLine = '\0';

        if (filename[0] != '\0')
            runTest(filename);
    }
}

static void adjustOutputTypeByMimeType(const Evas_Object* frame)
{
    const String responseMimeType(DumpRenderTreeSupportEfl::responseMimeType(frame));
    if (responseMimeType == "text/plain") {
        gTestRunner->setDumpAsText(true);
        gTestRunner->setGeneratePixelResults(false);
    }
}

static void dumpFrameContentsAsText(Evas_Object* frame)
{
    String result;
    if (gTestRunner->dumpAsText())
        result = dumpFramesAsText(frame);
    else
        result = DumpRenderTreeSupportEfl::renderTreeDump(frame);

    printf("%s", result.utf8().data());
}

static bool shouldDumpFrameScrollPosition()
{
    return !gTestRunner->dumpAsText() && !gTestRunner->dumpDOMAsWebArchive() && !gTestRunner->dumpSourceAsWebArchive();
}

static bool shouldDumpPixelsAndCompareWithExpected()
{
    return dumpPixelsForCurrentTest && gTestRunner->generatePixelResults() && !gTestRunner->dumpDOMAsWebArchive() && !gTestRunner->dumpSourceAsWebArchive();
}

static bool shouldDumpBackForwardList()
{
    return gTestRunner->dumpBackForwardList();
}

static bool initEfl()
{
    if (!ecore_evas_init())
        return false;
    if (!ecore_file_init()) {
        ecore_evas_shutdown();
        return false;
    }
    if (!edje_init()) {
        ecore_file_shutdown();
        ecore_evas_shutdown();
        return false;
    }
    if (!ewk_init()) {
        edje_shutdown();
        ecore_file_shutdown();
        ecore_evas_shutdown();
        return false;
    }

    return true;
}

static void shutdownEfl()
{
    ewk_shutdown();
    edje_shutdown();
    ecore_file_shutdown();
    ecore_evas_shutdown();
}

void displayWebView()
{
    DumpRenderTreeSupportEfl::forceLayout(browser->mainFrame());
    DumpRenderTreeSupportEfl::setTracksRepaints(browser->mainFrame(), true);
    DumpRenderTreeSupportEfl::resetTrackedRepaints(browser->mainFrame());
}

void dump()
{
    Evas_Object* frame = browser->mainFrame();

    invalidateAnyPreviousWaitToDumpWatchdog();

    if (dumpTree) {
        adjustOutputTypeByMimeType(frame);
        dumpFrameContentsAsText(frame);

        if (shouldDumpFrameScrollPosition())
            dumpFrameScrollPosition(frame);

        if (shouldDumpBackForwardList())
            dumpBackForwardListForWebViews();

        if (printSeparators) {
            puts("#EOF");
            fputs("#EOF\n", stderr);
            fflush(stdout);
            fflush(stderr);
        }
    }

    if (shouldDumpPixelsAndCompareWithExpected())
        dumpWebViewAsPixelsAndCompareWithExpected(gTestRunner->expectedPixelHash());

    done = true;
    ecore_main_loop_quit();
}

static Ecore_Evas* initEcoreEvas()
{
    Ecore_Evas* ecoreEvas = 0;
#if defined(WTF_USE_ACCELERATED_COMPOSITING) && defined(HAVE_ECORE_X)
    ecoreEvas = ecore_evas_new("opengl_x11", 0, 0, 800, 600, 0);
    if (!ecoreEvas)
#endif
    ecoreEvas = ecore_evas_new(0, 0, 0, 800, 600, 0);
    if (!ecoreEvas) {
        shutdownEfl();
        exit(EXIT_FAILURE);
    }

    ecore_evas_title_set(ecoreEvas, "EFL DumpRenderTree");
    ecore_evas_callback_resize_set(ecoreEvas, onEcoreEvasResize);
    ecore_evas_callback_delete_request_set(ecoreEvas, onCloseWindow);
    ecore_evas_show(ecoreEvas);

    return ecoreEvas;
}

int main(int argc, char** argv)
{
    if (!parseCommandLineOptions(argc, argv))
        return EXIT_FAILURE;

    if (!initEfl())
        return EXIT_FAILURE;

    WTFInstallReportBacktraceOnCrashHook();

    OwnPtr<Ecore_Evas> ecoreEvas = adoptPtr(initEcoreEvas());
    browser = DumpRenderTreeChrome::create(ecore_evas_get(ecoreEvas.get()));
    addFontsToEnvironment();

    if (useLongRunningServerMode(argc, argv)) {
        printSeparators = true;
        runTestingServerLoop();
    } else {
        printSeparators = (optind < argc - 1 || (dumpPixelsForCurrentTest && dumpTree));
        for (int i = optind; i != argc; ++i)
            runTest(argv[i]);
    }

    ecoreEvas.clear();

    shutdownEfl();
    return EXIT_SUCCESS;
}
