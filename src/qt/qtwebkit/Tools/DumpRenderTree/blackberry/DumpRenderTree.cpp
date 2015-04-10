/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "DumpRenderTree.h"

#include "APICast.h"
#include "AccessibilityController.h"
#include "BackForwardController.h"
#include "BackForwardListBlackBerry.h"
#include "Credential.h"
#include "DatabaseTracker.h"
#include "DocumentLoader.h"
#include "DumpRenderTree/GCController.h"
#include "DumpRenderTreeSupport.h"
#include "EditingBehaviorTypes.h"
#include "EditorClientBlackBerry.h"
#include "EditorInsertAction.h"
#include "Element.h"
#include "EventSender.h"
#include "Frame.h"
#include "FrameLoaderTypes.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "HTTPParsers.h"
#include "HistoryItem.h"
#include "HitTestResult.h"
#include "IntSize.h"
#include "JSDOMBinding.h"
#include "MouseEvent.h"
#include "Node.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PageGroup.h"
#include "PixelDumpSupport.h"
#include "PixelDumpSupportBlackBerry.h"
#include "Range.h"
#include "RenderTreeAsText.h"
#include "ScriptController.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include "TestRunner.h"
#include "TextAffinity.h"
#include "Timer.h"
#include "WebCoreTestSupport.h"
#include "WebPage.h"
#include "WebPageClient.h"
#include "WorkQueue.h"
#include "WorkQueueItem.h"
#include <BlackBerryPlatformLayoutTest.h>
#include <BlackBerryPlatformPrimitives.h>
#include <WebSettings.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wtf/NonCopyingSort.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/Vector.h>

#define SDCARD_PATH "/developer"

volatile bool testDone;

RefPtr<TestRunner> gTestRunner;

WebCore::Frame* mainFrame = 0;
WebCore::Frame* topLoadingFrame = 0;
bool waitForPolicy = false;
bool runFromCommandLine = false;

// FIXME: Assuming LayoutTests has been copied to /developer/LayoutTests/
static const char* const kSDCLayoutTestsURI = "file:///developer/LayoutTests/";
static const char* httpTestSyntax = "http/tests/";
static const char* localTestSyntax = "local/";
static const char* httpPrefixURL = "http://127.0.0.1:8000/";

using namespace std;

static String drtAffinityDescription(WebCore::EAffinity affinity)
{
    if (affinity == WebCore::UPSTREAM)
        return String("NSSelectionAffinityUpstream");
    if (affinity == WebCore::DOWNSTREAM)
        return String("NSSelectionAffinityDownstream");
    return "";
}

static String drtDumpPath(WebCore::Node* node)
{
    WebCore::Node* parent = node->parentNode();
    String str = String::format("%s", node->nodeName().utf8().data());
    if (parent) {
        str.append(" > ");
        str.append(drtDumpPath(parent));
    }
    return str;
}

static String drtRangeDescription(WebCore::Range* range)
{
    if (!range)
        return "(null)";
    return String::format("range from %d of %s to %d of %s", range->startOffset(), drtDumpPath(range->startContainer()).utf8().data(), range->endOffset(), drtDumpPath(range->endContainer()).utf8().data());
}

static String drtFrameDescription(WebCore::Frame* frame)
{
    String name = frame->tree()->uniqueName().string();
    if (frame == mainFrame) {
        if (!name.isNull() && name.length())
            return String::format("main frame \"%s\"", name.utf8().data());
        return "main frame";
    }
    if (!name.isNull())
        return String::format("frame \"%s\"", name.utf8().data());
    return "frame (anonymous)";
}

static WTF::String drtCredentialDescription(WebCore::Credential&)
{
    // TODO: Implementation needed.
    return "<unknown>";
}

static bool shouldLogFrameLoadDelegates(const String& url)
{
    return url.contains("loading/");
}

static bool shouldDumpAsText(const String& url)
{
    return url.contains("dumpAsText/");
}

namespace BlackBerry {
namespace WebKit {

DumpRenderTree* DumpRenderTree::s_currentInstance = 0;

static void createFile(const String& fileName)
{
    FILE* fd = fopen(fileName.utf8().data(), "wb");
    if (fd)
        fclose(fd);
}

static bool isFullUrl(const String& url)
{
    static Vector<String> *prefixes = 0;
    if (!prefixes)  {
        prefixes = new Vector<String>();
        prefixes->append("http://");
        prefixes->append("file://");
    }
    for (unsigned i = 0; i < prefixes->size(); ++i) {
        if (url.startsWith(prefixes->at(i), false))
            return true;
    }
    return false;
}

DumpRenderTree::DumpRenderTree(BlackBerry::WebKit::WebPage* page)
    : m_gcController(0)
    , m_accessibilityController(0)
    , m_page(page)
    , m_enablePixelTests(getenv("pixelTests"))
    , m_waitToDumpWatchdogTimer(this, &DumpRenderTree::waitToDumpWatchdogTimerFired)
    , m_workTimer(this, &DumpRenderTree::processWork)
    , m_acceptsEditing(true)
    , m_policyDelegateEnabled(false)
    , m_policyDelegateIsPermissive(false)
{
    const char* workerNumber = getenv("WORKER_NUMBER") ? getenv("WORKER_NUMBER") : "0";
    String sdcardPath = SDCARD_PATH;
    m_resultsDir = sdcardPath + "/results/";
    m_doneFile = sdcardPath + "/done" + workerNumber;
    m_currentTestFile = sdcardPath + "/current" + workerNumber + ".drt";
    m_page->resetVirtualViewportOnCommitted(false);
    m_page->setVirtualViewportSize(Platform::IntSize(800, 600));
    s_currentInstance = this;
}

DumpRenderTree::~DumpRenderTree()
{
    delete m_gcController;
    delete m_accessibilityController;
}

void DumpRenderTree::runTest(const String& url, const String& imageHash)
{
    mainFrame->loader()->stopForUserCancel();
    resetToConsistentStateBeforeTesting(url, imageHash);
    if (shouldLogFrameLoadDelegates(url))
        gTestRunner->setDumpFrameLoadCallbacks(true);
    if (!runFromCommandLine) {
        createFile(m_resultsDir + *m_currentTest + ".dump.crash");

        String stdoutFile = m_resultsDir + *m_currentTest + ".dump";
        String stderrFile = m_resultsDir + *m_currentTest + ".stderr";

        // FIXME: we should preserve the original stdout and stderr here but aren't doing
        // that yet due to issues with dup, etc.
        freopen(stdoutFile.utf8().data(), "wb", stdout);
        freopen(stderrFile.utf8().data(), "wb", stderr);
    }
    FILE* current = fopen(m_currentTestFile.utf8().data(), "w");
    if (current) {
        fwrite(m_currentTest->utf8().data(), 1, m_currentTest->utf8().length(), current);
        fclose(current);
    }
    BlackBerry::Platform::NetworkRequest request;
    STATIC_LOCAL_STRING(s_get, "GET");
    request.setRequestUrl(url, s_get);
    m_page->load(request);
}

void DumpRenderTree::doneDrt()
{
    fclose(stdout);
    fclose(stderr);
    unlink(getPPSPath().c_str());

    // Notify the external world that we're done.
    createFile(m_doneFile);
    (m_page->client())->notifyRunLayoutTestsFinished();
}

void DumpRenderTree::runCurrentTest()
{
    String imageHash = "";
    int posSplitter = m_currentTest->find('?');
    if (posSplitter > 1 && (unsigned)posSplitter < m_currentTest->length() - 1) {
        imageHash = m_currentTest->substring(posSplitter + 1);
        m_currentTest->truncate(posSplitter);
    }
    if (isHTTPTest(m_currentTest->utf8().data())) {
        m_currentHttpTest = m_currentTest->utf8().data();
        m_currentHttpTest.remove(0, strlen(httpTestSyntax));
        runTest(httpPrefixURL + m_currentHttpTest, imageHash);
    } else if (isFullUrl(*m_currentTest))
        runTest(*m_currentTest, imageHash);
    else
        runTest(kSDCLayoutTestsURI + *m_currentTest, imageHash);
}

void DumpRenderTree::runRemainingTests()
{
    if (runFromCommandLine) {
        doneDrt();
        return;
    }

    // FIXME: fflush should not be necessary but is temporarily required due to a bug in stdio output.
    fflush(stdout);
    fflush(stderr);

    if (m_currentTest >= m_tests.end() - 1) {
        m_tests.clear();
        if (m_bufferedTests.size() > 0) {
            m_tests.append(m_bufferedTests);
            m_bufferedTests.clear();
            m_currentTest = m_tests.begin();
            runCurrentTest();
        }
        return;
    }

    m_currentTest++;
    runCurrentTest();
}

void DumpRenderTree::resetToConsistentStateBeforeTesting(const String& url, const String& imageHash)
{
    gTestRunner = TestRunner::create(url.utf8().data(), imageHash.utf8().data());

    if (shouldDumpAsText(url)) {
        gTestRunner->setDumpAsText(true);
        gTestRunner->setGeneratePixelResults(false);
    }
    gTestRunner->setIconDatabaseEnabled(false);

    DumpRenderTreeSupport::resetGeolocationMock(m_page);

    topLoadingFrame = 0;
    m_loadFinished = false;
    m_policyDelegateEnabled = false;
    m_policyDelegateIsPermissive = false;
    waitForPolicy = false;
    testDone = false;
    WorkQueue::shared()->clear();
    WorkQueue::shared()->setFrozen(false);

    WebSettings* settings = m_page->settings();
    // Apply new settings to current page, see more in the destructor of WebSettingsTransaction.
    WebSettingsTransaction webSettingTransaction(settings);

    settings->setTextReflowMode(WebSettings::TextReflowDisabled);
    settings->setJavaScriptEnabled(true);
    settings->setLoadsImagesAutomatically(true);
    settings->setJavaScriptOpenWindowsAutomatically(true);
    settings->setZoomToFitOnLoad(false);
    settings->setDefaultFontSize(16);
    settings->setDefaultFixedFontSize(13);
    settings->setMinimumFontSize(1);
    STATIC_LOCAL_STRING(s_arial, "Arial");
    STATIC_LOCAL_STRING(s_courier, "Courier New");
    STATIC_LOCAL_STRING(s_times, "Times");
    settings->setSerifFontFamily(s_times);
    settings->setFixedFontFamily(s_courier);
    settings->setSansSerifFontFamily(s_arial);
    settings->setStandardFontFamily(s_times);
    settings->setXSSAuditorEnabled(false);
    settings->setMaximumPagesInCache(0);
    settings->setPluginsEnabled(true);

    BlackBerry::WebKit::DumpRenderTree::currentInstance()->page()->clearBackForwardList(false);

    setAcceptsEditing(true);
    DumpRenderTreeSupport::setLinksIncludedInFocusChain(true);
#if ENABLE(STYLE_SCOPED)
    DumpRenderTreeSupport::setStyleScopedEnabled(true);
#endif

    m_page->setVirtualViewportSize(Platform::IntSize(800, 600));
    m_page->resetVirtualViewportOnCommitted(false);
    m_page->setUserScalable(true);
    m_page->setJavaScriptCanAccessClipboard(true);

    if (WebCore::Page* page = DumpRenderTreeSupport::corePage(m_page)) {
        page->setTabKeyCyclesThroughElements(true);

        // FIXME: Remove this once BlackBerry uses resetInternalsObject: https://bugs.webkit.org/show_bug.cgi?id=86899.
        page->settings()->setEditingBehaviorType(WebCore::EditingUnixBehavior);

        page->settings()->setDOMPasteAllowed(true);
        page->settings()->setValidationMessageTimerMagnification(-1);
        page->settings()->setInteractiveFormValidationEnabled(true);
        page->settings()->setAllowFileAccessFromFileURLs(true);
        page->settings()->setAllowUniversalAccessFromFileURLs(true);
        page->settings()->setAuthorAndUserStylesEnabled(true);
        page->settings()->setUsePreHTML5ParserQuirks(false);
        // FIXME: Other ports also clear history/backForwardList allong with visited links.
        page->group().removeVisitedLinks();
        if ((mainFrame = page->mainFrame())) {
            mainFrame->tree()->clearName();
            mainFrame->loader()->setOpener(0);
            // [WebKit bug #86899] Reset JS state settings.
            JSGlobalContextRef jsContext = toGlobalRef(mainFrame->script()->globalObject(WebCore::mainThreadNormalWorld())->globalExec());
            WebCoreTestSupport::resetInternalsObject(jsContext);
        }
    }

    // For now we manually garbage collect between each test to make sure the device won't run out of memory due to lazy collection.
    DumpRenderTreeSupport::garbageCollectorCollect();
}

void DumpRenderTree::runTests()
{
    m_gcController = new GCController();
    m_accessibilityController = new AccessibilityController();
    if (!ensurePPS()) {
        fprintf(stderr, "Failed to open PPS file '%s', error=%d\n", getPPSPath().c_str(), errno);
        (m_page->client())->notifyRunLayoutTestsFinished();
        return;
    }

    mainFrame = DumpRenderTreeSupport::corePage(m_page)->mainFrame();

    if (const char* testFile = getenv("drtTestFile")) {
        runFromCommandLine = true;
        addTest(testFile);
    } else {
        // Get Test file name from PPS: /pps/services/drt/input
        // Example: test_file::fast/js/arguments.html
        waitForTest();
    }
}

void DumpRenderTree::addTest(const char* testFile)
{
    String test(testFile);
    if (test == "#DONE")
        doneDrt();
    else if (!test.isEmpty()) {
        if (m_tests.isEmpty()) {
            // No test is being run, initialize iterator and start test
            m_tests.append(test);
            m_currentTest = m_tests.begin();
            runCurrentTest();
        } else
            m_bufferedTests.append(test);
    }
}

String DumpRenderTree::dumpFramesAsText(WebCore::Frame* frame)
{
    String s;
    WebCore::Element* documentElement = frame->document()->documentElement();
    if (!documentElement)
        return s.utf8().data();

    if (frame->tree()->parent())
        s = String::format("\n--------\nFrame: '%s'\n--------\n", frame->tree()->uniqueName().string().utf8().data());

    s = s + documentElement->innerText() + "\n";

    if (gTestRunner->dumpChildFramesAsText()) {
        WebCore::FrameTree* tree = frame->tree();
        for (WebCore::Frame* child = tree->firstChild(); child; child = child->tree()->nextSibling())
            s = s + dumpFramesAsText(child);
    }
    return s;
}

static void dumpToFile(const String& data)
{
    fwrite(data.utf8().data(), 1, data.utf8().length(), stdout);
}

bool DumpRenderTree::isHTTPTest(const String& test)
{
    if (test.length() < strlen(httpTestSyntax))
        return false;
    String testLower = test.lower();
    int lenHttpTestSyntax = strlen(httpTestSyntax);
    return testLower.substring(0, lenHttpTestSyntax) == httpTestSyntax
        && testLower.substring(lenHttpTestSyntax, strlen(localTestSyntax)) != localTestSyntax;
}

void DumpRenderTree::invalidateAnyPreviousWaitToDumpWatchdog()
{
    m_waitToDumpWatchdogTimer.stop();
    waitForPolicy = false;
}

String DumpRenderTree::renderTreeDump() const
{
    if (mainFrame) {
        if (mainFrame->view() && mainFrame->view()->layoutPending())
            mainFrame->view()->layout();

        return externalRepresentation(mainFrame);
    }
    return "";
}

static bool historyItemCompare(const RefPtr<WebCore::HistoryItem>& a, const RefPtr<WebCore::HistoryItem>& b)
{
    return codePointCompare(a->urlString(), b->urlString()) < 0;
}

static String dumpHistoryItem(PassRefPtr<WebCore::HistoryItem> item, int indent, bool current)
{
    String result;

    int start = 0;
    if (current) {
        result = result + "curr->";
        start = 6;
    }
    for (int i = start; i < indent; i++)
        result = result + ' ';

    String url = item->urlString();
    if (url.contains("file://")) {
        static String layoutTestsString("/LayoutTests/");
        static String fileTestString("(file test):");

        String res = url.substring(url.find(layoutTestsString) + layoutTestsString.length());
        if (res.isEmpty())
            return result;

        result = result + fileTestString;
        result = result + res;
    } else
        result = result + url;

    String target = item->target();
    if (!target.isEmpty())
        result = result + " (in frame \"" + target + "\")";

    if (item->isTargetItem())
        result = result + "  **nav target**";
    result = result + '\n';

    WebCore::HistoryItemVector children = item->children();
    // Must sort to eliminate arbitrary result ordering which defeats reproducible testing.
    nonCopyingSort(children.begin(), children.end(), historyItemCompare);
    unsigned resultSize = children.size();
    for (unsigned i = 0; i < resultSize; ++i)
        result = result + dumpHistoryItem(children[i], indent + 4, false);

    return result;
}

static String dumpBackForwardListForWebView()
{
    String result = "\n============== Back Forward List ==============\n";
    // FORMAT:
    // "        (file test):fast/loader/resources/click-fragment-link.html  **nav target**"
    // "curr->  (file test):fast/loader/resources/click-fragment-link.html#testfragment  **nav target**"
    WebCore::BackForwardListBlackBerry* bfList = static_cast<WebCore::BackForwardListBlackBerry*>(mainFrame->page()->backForward()->client());

    int maxItems = bfList->capacity();
    WebCore::HistoryItemVector entries;
    bfList->backListWithLimit(maxItems, entries);
    unsigned resultSize = entries.size();
    for (unsigned i = 0; i < resultSize; ++i)
        result = result + dumpHistoryItem(entries[i], 8, false);

    result = result + dumpHistoryItem(bfList->currentItem(), 8, true);

    bfList->forwardListWithLimit(maxItems, entries);
    resultSize = entries.size();
    for (unsigned i = 0; i < resultSize; ++i)
        result = result + dumpHistoryItem(entries[i], 8, false);

    result = result +  "===============================================\n";

    return result;
}

void DumpRenderTree::dump()
{
    if (testDone)
        return;

    invalidateAnyPreviousWaitToDumpWatchdog();

    String dumpFile = m_resultsDir + *m_currentTest + ".dump";

    String resultMimeType = "text/plain";
    String responseMimeType = mainFrame->loader()->documentLoader()->responseMIMEType();

    bool dumpAsText = gTestRunner->dumpAsText() || responseMimeType == "text/plain";
    String data = dumpAsText ? dumpFramesAsText(mainFrame) : renderTreeDump();

    if (gTestRunner->dumpBackForwardList())
        data = data + dumpBackForwardListForWebView();

    String result = "Content-Type: " + resultMimeType + "\n" + data;

    dumpToFile(result);

    if (!runFromCommandLine) {
        // There are two scenarios for dumping pixels:
        // 1. When the test case explicitly asks for it by calling dumpAsText(true) with that extra true passed as a parameter value, from JavaScript
        bool explicitPixelResults = gTestRunner->dumpAsText() && gTestRunner->generatePixelResults();
        // 2. When the test case implicitly allows it by not calling dumpAsText() at all (with no parameters).
        bool implicitPixelResults = !gTestRunner->dumpAsText();

        // But only if m_enablePixelTests is set, to say that the user wants to run pixel tests at all.
        bool generatePixelResults = m_enablePixelTests && (explicitPixelResults || implicitPixelResults);
        if (generatePixelResults) {
            // signal end of text block
            fputs("#EOF\n", stdout);
            dumpWebViewAsPixelsAndCompareWithExpected(gTestRunner->expectedPixelHash());
        }

        String crashFile = dumpFile + ".crash";
        unlink(crashFile.utf8().data());

        String doneFile =  m_resultsDir + *m_currentTest + ".done";
        createFile(doneFile);
    }
    testDone = true;
    runRemainingTests();
}

void DumpRenderTree::setWaitToDumpWatchdog(double interval)
{
    invalidateAnyPreviousWaitToDumpWatchdog();
    m_waitToDumpWatchdogTimer.startOneShot(interval);
}

void DumpRenderTree::waitToDumpWatchdogTimerFired(WebCore::Timer<DumpRenderTree>*)
{
    gTestRunner->waitToDumpWatchdogTimerFired();
}

void DumpRenderTree::processWork(WebCore::Timer<DumpRenderTree>*)
{
    if (topLoadingFrame)
        return;

    if (WorkQueue::shared()->processWork() && !gTestRunner->waitToDump())
        dump();
}

void DumpRenderTree::locationChangeForFrame(WebCore::Frame* frame)
{
    if (frame != topLoadingFrame)
        return;

    topLoadingFrame = 0;
    WorkQueue::shared()->setFrozen(true); // first complete load freezes the queue
    if (gTestRunner->waitToDump())
        return;

    if (WorkQueue::shared()->count())
        m_workTimer.startOneShot(0);
    else
        dump();
}

// FrameLoadClient delegates.
bool DumpRenderTree::willSendRequestForFrame(WebCore::Frame* frame, WebCore::ResourceRequest& request, const WebCore::ResourceResponse& redirectResponse)
{
    if (!testDone && (gTestRunner->willSendRequestReturnsNull() || (gTestRunner->willSendRequestReturnsNullOnRedirect() && !redirectResponse.isNull()))) {
        request = WebCore::ResourceRequest();
        return false;
    }

    return true;
}

void DumpRenderTree::didStartProvisionalLoadForFrame(WebCore::Frame* frame)
{
    if (!testDone && gTestRunner->dumpFrameLoadCallbacks())
        printf("%s - didStartProvisionalLoadForFrame\n", drtFrameDescription(frame).utf8().data());

    if (!testDone && gTestRunner->dumpUserGestureInFrameLoadCallbacks())
        printf("Frame with user gesture \"%s\" - in didStartProvisionalLoadForFrame\n", WebCore::ScriptController::processingUserGesture() ? "true" : "false");

    if (!topLoadingFrame && !testDone)
        topLoadingFrame = frame;

    if (!testDone && gTestRunner->stopProvisionalFrameLoads()) {
        printf("%s - stopping load in didStartProvisionalLoadForFrame callback\n", drtFrameDescription(frame).utf8().data());
        frame->loader()->stopForUserCancel();
    }
}

void DumpRenderTree::didCommitLoadForFrame(WebCore::Frame* frame)
{
    if (!testDone && gTestRunner->dumpFrameLoadCallbacks())
        printf("%s - didCommitLoadForFrame\n", drtFrameDescription(frame).utf8().data());

    gTestRunner->setWindowIsKey(true);
}

void DumpRenderTree::didFailProvisionalLoadForFrame(WebCore::Frame* frame)
{
    if (!testDone && gTestRunner->dumpFrameLoadCallbacks())
        printf("%s - didFailProvisionalLoadWithError\n", drtFrameDescription(frame).utf8().data());

    locationChangeForFrame(frame);
}

void DumpRenderTree::didFailLoadForFrame(WebCore::Frame* frame)
{
    if (!testDone && gTestRunner->dumpFrameLoadCallbacks())
        printf("%s - didFailLoadWithError\n", drtFrameDescription(frame).utf8().data());

    locationChangeForFrame(frame);
}

void DumpRenderTree::didFinishLoadForFrame(WebCore::Frame* frame)
{
    if (!testDone && gTestRunner->dumpFrameLoadCallbacks())
        printf("%s - didFinishLoadForFrame\n", drtFrameDescription(frame).utf8().data());

    if (frame == topLoadingFrame) {
        m_loadFinished = true;
        locationChangeForFrame(frame);
    }
}

void DumpRenderTree::didFinishDocumentLoadForFrame(WebCore::Frame* frame)
{
    if (!testDone) {
        if (gTestRunner->dumpFrameLoadCallbacks())
            printf("%s - didFinishDocumentLoadForFrame\n", drtFrameDescription(frame).utf8().data());
        else {
            unsigned pendingFrameUnloadEvents = frame->document()->domWindow()->pendingUnloadEventListeners();
            if (pendingFrameUnloadEvents)
                printf("%s - has %u onunload handler(s)\n", drtFrameDescription(frame).utf8().data(), pendingFrameUnloadEvents);
        }
    }
}

void DumpRenderTree::didClearWindowObjectInWorld(WebCore::DOMWrapperWorld*, JSGlobalContextRef context, JSObjectRef windowObject)
{
    JSValueRef exception = 0;

    gTestRunner->makeWindowObject(context, windowObject, &exception);
    ASSERT(!exception);

    m_gcController->makeWindowObject(context, windowObject, &exception);
    ASSERT(!exception);

    m_accessibilityController->makeWindowObject(context, windowObject, &exception);
    ASSERT(!exception);

    JSStringRef eventSenderStr = JSStringCreateWithUTF8CString("eventSender");
    JSValueRef eventSender = makeEventSender(context);
    JSObjectSetProperty(context, windowObject, eventSenderStr, eventSender, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete, 0);
    JSStringRelease(eventSenderStr);
    WebCoreTestSupport::injectInternalsObject(context);
}

void DumpRenderTree::didReceiveTitleForFrame(const String& title, WebCore::Frame* frame)
{
    if (!testDone && gTestRunner->dumpFrameLoadCallbacks())
        printf("%s - didReceiveTitle: %s\n", drtFrameDescription(frame).utf8().data(), title.utf8().data());

    if (gTestRunner->dumpTitleChanges())
        printf("TITLE CHANGED: %s\n", title.utf8().data());
}

// ChromeClient delegates.
void DumpRenderTree::addMessageToConsole(const String& message, unsigned lineNumber, const String&)
{
    printf("CONSOLE MESSAGE: ");
    if (lineNumber)
        printf("line %d: ", lineNumber);
    String newMessage = message;
    int pos = message.find("file://");
    if (pos >= 0) {
        newMessage = message.substring(0, pos);
        String remaining = message.substring(pos);
        String fileName;
        int indexFile = remaining.reverseFind('/') + 1;
        if (indexFile > 0 && unsigned(indexFile) < remaining.length())
            fileName = remaining.substring(indexFile);
        else
            fileName = "file:";
        newMessage.append(fileName);
    }
    printf("%s\n", newMessage.utf8().data());
}

void DumpRenderTree::runJavaScriptAlert(const String& message)
{
    if (!testDone)
        printf("ALERT: %s\n", message.utf8().data());
}

bool DumpRenderTree::runJavaScriptConfirm(const String& message)
{
    if (!testDone)
        printf("CONFIRM: %s\n", message.utf8().data());
    return true;
}

String DumpRenderTree::runJavaScriptPrompt(const String& message, const String& defaultValue)
{
    if (!testDone)
        printf("PROMPT: %s, default text: %s\n", message.utf8().data(), defaultValue.utf8().data());
    return defaultValue;
}

bool DumpRenderTree::runBeforeUnloadConfirmPanel(const String& message)
{
    if (!testDone)
        printf("CONFIRM NAVIGATION: %s\n", message.utf8().data());
    return true;
}

void DumpRenderTree::setStatusText(const String& status)
{
    if (gTestRunner->dumpStatusCallbacks())
        printf("UI DELEGATE STATUS CALLBACK: setStatusText:%s\n", status.utf8().data());
}

void DumpRenderTree::exceededDatabaseQuota(WebCore::SecurityOrigin* origin, const String& name)
{
    if (!testDone && gTestRunner->dumpDatabaseCallbacks())
        printf("UI DELEGATE DATABASE CALLBACK: exceededDatabaseQuotaForSecurityOrigin:{%s, %s, %i} database:%s\n", origin->protocol().utf8().data(), origin->host().utf8().data(), origin->port(), name.utf8().data());

    WebCore::DatabaseTracker::tracker().setQuota(mainFrame->document()->securityOrigin(), 5 * 1024 * 1024);
}

bool DumpRenderTree::allowsOpeningWindow()
{
    return gTestRunner->canOpenWindows();
}

void DumpRenderTree::windowCreated(BlackBerry::WebKit::WebPage* page)
{
    page->settings()->setJavaScriptOpenWindowsAutomatically(true);
}

// EditorClient delegates.
void DumpRenderTree::didBeginEditing()
{
    if (!testDone && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: webViewDidBeginEditing:%s\n", "WebViewDidBeginEditingNotification");
}

void DumpRenderTree::didEndEditing()
{
    if (!testDone && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: webViewDidEndEditing:%s\n", "WebViewDidEndEditingNotification");
}

void DumpRenderTree::didChange()
{
    if (!testDone && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: webViewDidChange:%s\n", "WebViewDidChangeNotification");
}

void DumpRenderTree::didChangeSelection()
{
    if (!testDone && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: webViewDidChangeSelection:%s\n", "WebViewDidChangeSelectionNotification");
}

bool DumpRenderTree::shouldBeginEditingInDOMRange(WebCore::Range* range)
{
    if (!testDone && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: shouldBeginEditingInDOMRange:%s\n", drtRangeDescription(range).utf8().data());
    return m_acceptsEditing;
}

bool DumpRenderTree::shouldEndEditingInDOMRange(WebCore::Range* range)
{
    if (!testDone && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: shouldEndEditingInDOMRange:%s\n", drtRangeDescription(range).utf8().data());
    return m_acceptsEditing;
}

bool DumpRenderTree::shouldDeleteDOMRange(WebCore::Range* range)
{
    if (!testDone && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: shouldDeleteDOMRange:%s\n", drtRangeDescription(range).utf8().data());
    return m_acceptsEditing;
}

bool DumpRenderTree::shouldChangeSelectedDOMRangeToDOMRangeAffinityStillSelecting(WebCore::Range* fromRange, WebCore::Range* toRange, int affinity, bool stillSelecting)
{
    if (!testDone && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: shouldChangeSelectedDOMRange:%s toDOMRange:%s affinity:%s stillSelecting:%s\n", drtRangeDescription(fromRange).utf8().data(), drtRangeDescription(toRange).utf8().data(), drtAffinityDescription(static_cast<WebCore::EAffinity>(affinity)).utf8().data(), stillSelecting ? "TRUE" : "FALSE");
    return m_acceptsEditing;
}

static const char* insertActionString(WebCore::EditorInsertAction action)
{
    switch (action) {
    case WebCore::EditorInsertActionTyped:
        return "WebViewInsertActionTyped";
    case WebCore::EditorInsertActionPasted:
        return "WebViewInsertActionPasted";
    case WebCore::EditorInsertActionDropped:
        return "WebViewInsertActionDropped";
    }
    ASSERT_NOT_REACHED();
    return "WebViewInsertActionTyped";
}

bool DumpRenderTree::shouldInsertNode(WebCore::Node* node, WebCore::Range* range, int action)
{
    if (!testDone && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: shouldInsertNode:%s replacingDOMRange:%s givenAction:%s\n", drtDumpPath(node).utf8().data(), drtRangeDescription(range).utf8().data(), insertActionString((WebCore::EditorInsertAction)action));
    return m_acceptsEditing;
}

bool DumpRenderTree::shouldInsertText(const String& text, WebCore::Range* range, int action)
{
    if (!testDone && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: shouldInsertText:%s replacingDOMRange:%s givenAction:%s\n", text.utf8().data(), drtRangeDescription(range).utf8().data(), insertActionString((WebCore::EditorInsertAction)action));
    return m_acceptsEditing;
}

void DumpRenderTree::didDecidePolicyForNavigationAction(const WebCore::NavigationAction& action, const WebCore::ResourceRequest& request, WebCore::Frame* frame)
{
    if (testDone || !m_policyDelegateEnabled)
        return;

    const char* typeDescription;
    switch (action.type()) {
    case WebCore::NavigationTypeLinkClicked:
        typeDescription = "link clicked";
        break;
    case WebCore::NavigationTypeFormSubmitted:
        typeDescription = "form submitted";
        break;
    case WebCore::NavigationTypeBackForward:
        typeDescription = "back/forward";
        break;
    case WebCore::NavigationTypeReload:
        typeDescription = "reload";
        break;
    case WebCore::NavigationTypeFormResubmitted:
        typeDescription = "form resubmitted";
        break;
    case WebCore::NavigationTypeOther:
        typeDescription = "other";
        break;
    default:
        typeDescription = "illegal value";
    }

    bool shouldWaitForResponse = !request.url().string().startsWith("mailto:");
    printf("Policy delegate: attempt to load %s with navigation type '%s'", request.url().string().utf8().data(), typeDescription);
    // Originating part, borrowed from Chromium.
    RefPtr<WebCore::Node> node;
    for (const WebCore::Event* event = action.event(); event; event = event->underlyingEvent()) {
        if (event->isMouseEvent()) {
            const WebCore::MouseEvent* mouseEvent = static_cast<const WebCore::MouseEvent*>(event);
            node = frame->eventHandler()->hitTestResultAtPoint(mouseEvent->absoluteLocation(), false).innerNonSharedNode();
            break;
        }
    }
    if (node.get())
        printf(" originating from %s\n", drtDumpPath(node.get()).utf8().data());
    else
        printf("\n");

    if (waitForPolicy && !shouldWaitForResponse)
        gTestRunner->notifyDone();
}

void DumpRenderTree::didDecidePolicyForResponse(const WebCore::ResourceResponse& response)
{
    if (!testDone && m_policyDelegateEnabled) {
        if (WebCore::contentDispositionType(response.httpHeaderField("Content-Disposition")) == WebCore::ContentDispositionAttachment)
            printf("Policy delegate: resource is an attachment, suggested file name '%s'\n", response.suggestedFilename().utf8().data());
        if (waitForPolicy)
            gTestRunner->notifyDone();
    }
}

void DumpRenderTree::didDispatchWillPerformClientRedirect()
{
    if (!testDone && gTestRunner->dumpUserGestureInFrameLoadCallbacks())
        printf("Frame with user gesture \"%s\" - in willPerformClientRedirect\n", WebCore::ScriptController::processingUserGesture() ? "true" : "false");
}

void DumpRenderTree::didHandleOnloadEventsForFrame(WebCore::Frame* frame)
{
    if (!testDone && gTestRunner->dumpFrameLoadCallbacks())
        printf("%s - didHandleOnloadEventsForFrame\n", drtFrameDescription(frame).utf8().data());
}

void DumpRenderTree::didReceiveResponseForFrame(WebCore::Frame*, const WebCore::ResourceResponse& response)
{
    if (!testDone && gTestRunner->dumpResourceResponseMIMETypes())
        printf("%s has MIME type %s\n", response.url().lastPathComponent().utf8().data(), response.mimeType().utf8().data());
}

bool DumpRenderTree::didReceiveAuthenticationChallenge(WebCore::Credential& credential)
{
    if (!gTestRunner->handlesAuthenticationChallenges()) {
        credential = WebCore::Credential();
        printf("%s - didReceiveAuthenticationChallenge - Simulating cancelled authentication\n", drtCredentialDescription(credential).utf8().data());
        return false;
    }
    const char* user = gTestRunner->authenticationUsername().c_str();
    const char* password = gTestRunner->authenticationPassword().c_str();
    credential = WebCore::Credential(user, password, WebCore::CredentialPersistenceForSession);
    printf("%s - didReceiveAuthenticationChallenge - Responding with %s:%s\n", drtCredentialDescription(credential).utf8().data(), user, password);
    return true;
}

void DumpRenderTree::setCustomPolicyDelegate(bool setDelegate, bool permissive)
{
    m_policyDelegateEnabled = setDelegate;
    m_policyDelegateIsPermissive = permissive;
}
}
}

// Static dump() function required by cross-platform DRT code.
void dump()
{
    BlackBerry::WebKit::DumpRenderTree* dumper = BlackBerry::WebKit::DumpRenderTree::currentInstance();
    if (!dumper)
        return;

    dumper->dump();
}
