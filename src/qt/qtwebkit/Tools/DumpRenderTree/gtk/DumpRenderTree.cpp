/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Alp Toker <alp@nuanti.com>
 * Copyright (C) 2009 Jan Alonzo <jmalonzo@gmail.com>
 * Copyright (C) 2010, 2011 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DumpRenderTree.h"

#include "AccessibilityController.h"
#include "EditingCallbacks.h"
#include "EventSender.h"
#include "GCController.h"
#include "PixelDumpSupport.h"
#include "SelfScrollingWebKitWebView.h"
#include "TestRunner.h"
#include "TextInputController.h"
#include "WebCoreSupport/DumpRenderTreeSupportGtk.h"
#include "WebCoreTestSupport.h"
#include "WorkQueue.h"
#include "WorkQueueItem.h"
#include <JavaScriptCore/JavaScript.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <gtk/gtk.h>
#include <locale.h>
#include <webkit/webkit.h>
#include <wtf/Assertions.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/gobject/GlibUtilities.h>

#if PLATFORM(X11)
#include <fontconfig/fontconfig.h>
#endif


using namespace std;

extern "C" {
// This API is not yet public.
extern gchar* webkit_web_history_item_get_target(WebKitWebHistoryItem*);
extern gboolean webkit_web_history_item_is_target_item(WebKitWebHistoryItem*);
extern GList* webkit_web_history_item_get_children(WebKitWebHistoryItem*);
extern void webkit_web_settings_add_extra_plugin_directory(WebKitWebView* view, const gchar* directory);
extern gchar* webkit_web_frame_get_response_mime_type(WebKitWebFrame* frame);
}

volatile bool done;
static bool printSeparators;
static int dumpPixelsForAllTests = false;
static bool dumpPixelsForCurrentTest;
static int dumpTree = 1;
static int useTimeoutWatchdog = 1;

AccessibilityController* axController = 0;
RefPtr<TestRunner> gTestRunner;
static GCController* gcController = 0;
static WebKitWebView* webView;
static GtkWidget* window;
static GtkWidget* container;
static GtkWidget* webInspectorWindow;
WebKitWebFrame* mainFrame = 0;
WebKitWebFrame* topLoadingFrame = 0;
guint waitToDumpWatchdog = 0;
bool waitForPolicy = false;

// This is a list of opened webviews
GSList* webViewList = 0;

// current b/f item at the end of the previous test
static WebKitWebHistoryItem* prevTestBFItem = NULL;

const unsigned historyItemIndent = 8;

static void runTest(const string& inputLine);

static void didRunInsecureContent(WebKitWebFrame*, WebKitSecurityOrigin*, const char* url);

static bool shouldLogFrameLoadDelegates(const string& pathOrURL)
{
    return pathOrURL.find("loading/") != string::npos;
}

static bool shouldOpenWebInspector(const string& pathOrURL)
{
    return pathOrURL.find("inspector/") != string::npos;
}

static bool shouldDumpAsText(const string& pathOrURL)
{
    return pathOrURL.find("dumpAsText/") != string::npos;
}

static bool shouldEnableDeveloperExtras(const string& pathOrURL)
{
    return true;
}

void dumpFrameScrollPosition(WebKitWebFrame* frame)
{
    WebKitDOMDocument* document = webkit_web_frame_get_dom_document(frame);
    if (!document)
        return;

    WebKitDOMDOMWindow* domWindow = webkit_dom_document_get_default_view(document);
    if (!domWindow)
        return;

    glong x = webkit_dom_dom_window_get_page_x_offset(domWindow);
    glong y = webkit_dom_dom_window_get_page_y_offset(domWindow);

    if (abs(x) > 0 || abs(y) > 0) {
        if (webkit_web_frame_get_parent(frame))
            printf("frame '%s' ", webkit_web_frame_get_name(frame));
        printf("scrolled to %ld,%ld\n", x, y);
    }

    if (gTestRunner->dumpChildFrameScrollPositions()) {
        GSList* children = DumpRenderTreeSupportGtk::getFrameChildren(frame);
        for (GSList* child = children; child; child = g_slist_next(child))
            dumpFrameScrollPosition(static_cast<WebKitWebFrame*>(child->data));
        g_slist_free(children);
    }
}

void displayWebView()
{
    DumpRenderTreeSupportGtk::forceWebViewPaint(webView);
    DumpRenderTreeSupportGtk::setTracksRepaints(mainFrame, true);
    DumpRenderTreeSupportGtk::resetTrackedRepaints(mainFrame);
}

static void appendString(gchar*& target, const gchar* string)
{
    gchar* oldString = target;
    target = g_strconcat(target, string, NULL);
    g_free(oldString);
}

static void initializeGtkFontSettings(const char* testURL)
{
    GtkSettings* settings = gtk_settings_get_default();
    if (!settings)
        return;
    g_object_set(settings,
                 "gtk-xft-dpi", 98304, // This is 96 * 1024 or 96 DPI according to the GTK+ docs.
                 "gtk-xft-antialias", 1,
                 "gtk-xft-hinting", 0,
                 "gtk-font-name", "Liberation Sans 12",
                 "gtk-icon-theme-name", "gnome",
                 NULL);
    gdk_screen_set_resolution(gdk_screen_get_default(), 96.0);

    // One test needs subpixel anti-aliasing turned on, but generally we
    // want all text in other tests to use to grayscale anti-aliasing.
    if (testURL && strstr(testURL, "xsettings_antialias_settings.html"))
        g_object_set(settings, "gtk-xft-rgba", "rgb", NULL);
    else
        g_object_set(settings, "gtk-xft-rgba", "none", NULL);
}

CString getTopLevelPath()
{
    if (!g_getenv("WEBKIT_TOP_LEVEL"))
        g_setenv("WEBKIT_TOP_LEVEL", TOP_LEVEL_DIR, FALSE);

    return TOP_LEVEL_DIR;
}

CString getOutputDir()
{
    const char* webkitOutputDir = g_getenv("WEBKIT_OUTPUTDIR");
    if (webkitOutputDir)
        return webkitOutputDir;

    CString topLevelPath = getTopLevelPath();
    GOwnPtr<char> outputDir(g_build_filename(topLevelPath.data(), "WebKitBuild", NULL));
    return outputDir.get();
}

static CString getFontsPath()
{
    CString webkitOutputDir = getOutputDir();
    GOwnPtr<char> fontsPath(g_build_filename(webkitOutputDir.data(), "Dependencies", "Root", "webkitgtk-test-fonts", NULL));
    if (g_file_test(fontsPath.get(), static_cast<GFileTest>(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)))
        return fontsPath.get();

    // Try alternative fonts path.
    fontsPath.set(g_build_filename(webkitOutputDir.data(), "webkitgtk-test-fonts", NULL));
    if (g_file_test(fontsPath.get(), static_cast<GFileTest>(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)))
        return fontsPath.get();

    return CString();
}

static void initializeFonts(const char* testURL = 0)
{
#if PLATFORM(X11)
    initializeGtkFontSettings(testURL);

    FcInit();

    // If a test resulted a font being added or removed via the @font-face rule, then
    // we want to reset the FontConfig configuration to prevent it from affecting other tests.
    static int numFonts = 0;
    FcFontSet* appFontSet = FcConfigGetFonts(0, FcSetApplication);
    if (appFontSet && numFonts && appFontSet->nfont == numFonts)
        return;

    // Load our configuration file, which sets up proper aliases for family
    // names like sans, serif and monospace.
    FcConfig* config = FcConfigCreate();
    GOwnPtr<gchar> fontConfigFilename(g_build_filename(FONTS_CONF_DIR, "fonts.conf", NULL));
    if (!FcConfigParseAndLoad(config, reinterpret_cast<FcChar8*>(fontConfigFilename.get()), true))
        g_error("Couldn't load font configuration file from: %s", fontConfigFilename.get());

    CString fontsPath = getFontsPath();
    if (fontsPath.isNull())
        g_error("Could not locate test fonts at %s. Is WEBKIT_TOP_LEVEL set?", fontsPath.data());

    GOwnPtr<GDir> fontsDirectory(g_dir_open(fontsPath.data(), 0, 0));
    while (const char* directoryEntry = g_dir_read_name(fontsDirectory.get())) {
        if (!g_str_has_suffix(directoryEntry, ".ttf") && !g_str_has_suffix(directoryEntry, ".otf"))
            continue;
        GOwnPtr<gchar> fontPath(g_build_filename(fontsPath.data(), directoryEntry, NULL));
        if (!FcConfigAppFontAddFile(config, reinterpret_cast<const FcChar8*>(fontPath.get())))
            g_error("Could not load font at %s!", fontPath.get());

    }

    // Ahem is used by many layout tests.
    GOwnPtr<gchar> ahemFontFilename(g_build_filename(FONTS_CONF_DIR, "AHEM____.TTF", NULL));
    if (!FcConfigAppFontAddFile(config, reinterpret_cast<FcChar8*>(ahemFontFilename.get())))
        g_error("Could not load font at %s!", ahemFontFilename.get()); 

    for (int i = 1; i <= 9; i++) {
        GOwnPtr<gchar> fontFilename(g_strdup_printf("WebKitWeightWatcher%i00.ttf", i));
        GOwnPtr<gchar> fontPath(g_build_filename(FONTS_CONF_DIR, "..", "..", "fonts", fontFilename.get(), NULL));
        if (!FcConfigAppFontAddFile(config, reinterpret_cast<FcChar8*>(fontPath.get())))
            g_error("Could not load font at %s!", fontPath.get()); 
    }

    // A font with no valid Fontconfig encoding to test https://bugs.webkit.org/show_bug.cgi?id=47452
    GOwnPtr<gchar> fontWithNoValidEncodingFilename(g_build_filename(FONTS_CONF_DIR, "FontWithNoValidEncoding.fon", NULL));
    if (!FcConfigAppFontAddFile(config, reinterpret_cast<FcChar8*>(fontWithNoValidEncodingFilename.get())))
        g_error("Could not load font at %s!", fontWithNoValidEncodingFilename.get()); 

    if (!FcConfigSetCurrent(config))
        g_error("Could not set the current font configuration!");

    numFonts = FcConfigGetFonts(config, FcSetApplication)->nfont;
#endif
}

static gchar* dumpFramesAsText(WebKitWebFrame* frame)
{
    gchar* result = 0;

    // Add header for all but the main frame.
    bool isMainFrame = (webkit_web_view_get_main_frame(webView) == frame);

    CString innerText = DumpRenderTreeSupportGtk::getInnerText(frame);
    if (isMainFrame)
        result = g_strdup_printf("%s\n", innerText.data());
    else {
        const gchar* frameName = webkit_web_frame_get_name(frame);
        result = g_strdup_printf("\n--------\nFrame: '%s'\n--------\n%s\n", frameName, innerText.data());
    }

    if (gTestRunner->dumpChildFramesAsText()) {
        GSList* children = DumpRenderTreeSupportGtk::getFrameChildren(frame);
        for (GSList* child = children; child; child = g_slist_next(child)) {
            GOwnPtr<gchar> childData(dumpFramesAsText(static_cast<WebKitWebFrame*>(child->data)));
            appendString(result, childData.get());
        }
        g_slist_free(children);
    }

    return result;
}

static gint compareHistoryItems(gpointer* item1, gpointer* item2)
{
    GOwnPtr<gchar> firstItemTarget(webkit_web_history_item_get_target(WEBKIT_WEB_HISTORY_ITEM(item1)));
    GOwnPtr<gchar> secondItemTarget(webkit_web_history_item_get_target(WEBKIT_WEB_HISTORY_ITEM(item2)));
    return g_ascii_strcasecmp(firstItemTarget.get(), secondItemTarget.get());
}

static void dumpHistoryItem(WebKitWebHistoryItem* item, int indent, bool current)
{
    ASSERT(item != NULL);
    int start = 0;
    g_object_ref(item);
    if (current) {
        printf("curr->");
        start = 6;
    }
    for (int i = start; i < indent; i++)
        putchar(' ');

    // normalize file URLs.
    const gchar* uri = webkit_web_history_item_get_uri(item);
    gchar* uriScheme = g_uri_parse_scheme(uri);
    if (g_strcmp0(uriScheme, "file") == 0) {
        gchar* pos = g_strstr_len(uri, -1, "/LayoutTests/");
        if (!pos) {
            g_free(uriScheme);
            return;
        }

        GString* result = g_string_sized_new(strlen(uri));
        result = g_string_append(result, "(file test):");
        result = g_string_append(result, pos + strlen("/LayoutTests/"));
        printf("%s", result->str);
        g_string_free(result, TRUE);
    } else
        printf("%s", uri);

    g_free(uriScheme);

    GOwnPtr<gchar> target(webkit_web_history_item_get_target(item));
    if (target.get() && strlen(target.get()) > 0)
        printf(" (in frame \"%s\")", target.get());
    if (webkit_web_history_item_is_target_item(item))
        printf("  **nav target**");
    putchar('\n');

    if (GList* kids = webkit_web_history_item_get_children(item)) {
        // must sort to eliminate arbitrary result ordering which defeats reproducible testing
        for (GList* kid = g_list_sort(kids, (GCompareFunc) compareHistoryItems); kid; kid = g_list_next(kid)) {
            WebKitWebHistoryItem* item = WEBKIT_WEB_HISTORY_ITEM(kid->data);
            dumpHistoryItem(item, indent + 4, FALSE);
            g_object_unref(item);
        }
        g_list_free(kids);
    }
    g_object_unref(item);
}

static void dumpBackForwardListForWebView(WebKitWebView* view)
{
    printf("\n============== Back Forward List ==============\n");
    WebKitWebBackForwardList* bfList = webkit_web_view_get_back_forward_list(view);

    // Print out all items in the list after prevTestBFItem, which was from the previous test
    // Gather items from the end of the list, the print them out from oldest to newest
    GList* itemsToPrint = NULL;
    gint forwardListCount = webkit_web_back_forward_list_get_forward_length(bfList);
    for (int i = forwardListCount; i > 0; i--) {
        WebKitWebHistoryItem* item = webkit_web_back_forward_list_get_nth_item(bfList, i);
        // something is wrong if the item from the last test is in the forward part of the b/f list
        ASSERT(item != prevTestBFItem);
        g_object_ref(item);
        itemsToPrint = g_list_prepend(itemsToPrint, item);
    }

    WebKitWebHistoryItem* currentItem = webkit_web_back_forward_list_get_current_item(bfList);
    g_object_ref(currentItem);
    itemsToPrint = g_list_prepend(itemsToPrint, currentItem);

    gint backListCount = webkit_web_back_forward_list_get_back_length(bfList);
    for (int i = -1; i >= -(backListCount); i--) {
        WebKitWebHistoryItem* item = webkit_web_back_forward_list_get_nth_item(bfList, i);
        if (item == prevTestBFItem)
            break;
        g_object_ref(item);
        itemsToPrint = g_list_prepend(itemsToPrint, item);
    }

    for (GList* itemToPrint = itemsToPrint; itemToPrint; itemToPrint = g_list_next(itemToPrint)) {
        WebKitWebHistoryItem* item = WEBKIT_WEB_HISTORY_ITEM(itemToPrint->data);
        dumpHistoryItem(item, historyItemIndent, item == currentItem);
        g_object_unref(item);
    }

    g_list_free(itemsToPrint);
    printf("===============================================\n");
}

static void dumpBackForwardListForAllWebViews()
{
    // Dump the back forward list of the main WebView first
    dumpBackForwardListForWebView(webView);

    // The view list is prepended. Reverse the list so we get the order right.
    for (GSList* currentView = g_slist_reverse(webViewList); currentView; currentView = g_slist_next(currentView))
        dumpBackForwardListForWebView(WEBKIT_WEB_VIEW(currentView->data));
}

void setWaitToDumpWatchdog(guint timer)
{
    waitToDumpWatchdog = timer;
}

bool shouldSetWaitToDumpWatchdog()
{
    return !waitToDumpWatchdog && useTimeoutWatchdog;
}

static void invalidateAnyPreviousWaitToDumpWatchdog()
{
    if (waitToDumpWatchdog) {
        g_source_remove(waitToDumpWatchdog);
        waitToDumpWatchdog = 0;
    }

    waitForPolicy = false;
}

static void resetDefaultsToConsistentValues()
{
    WebKitWebSettings* settings = webkit_web_view_get_settings(webView);
    GOwnPtr<gchar> localStoragePath(g_build_filename(g_get_user_data_dir(), "DumpRenderTreeGtk", "databases", NULL));
    g_object_set(G_OBJECT(settings),
        "enable-accelerated-compositing", FALSE,
        "enable-private-browsing", FALSE,
        "enable-developer-extras", FALSE,
        "enable-spell-checking", TRUE,
        "enable-html5-database", TRUE,
        "enable-html5-local-storage", TRUE,
        "html5-local-storage-database-path", localStoragePath.get(),
        "enable-xss-auditor", FALSE,
        "enable-spatial-navigation", FALSE,
        "javascript-can-access-clipboard", TRUE,
        "javascript-can-open-windows-automatically", TRUE,
        "enable-offline-web-application-cache", TRUE,
        "enable-universal-access-from-file-uris", TRUE,
        "enable-file-access-from-file-uris", TRUE,
        "enable-scripts", TRUE,
        "enable-dom-paste", TRUE,
        "default-font-family", "Times",
        "monospace-font-family", "Courier",
        "serif-font-family", "Times",
        "sans-serif-font-family", "Helvetica",
        "cursive-font-family", "cursive",
        "fantasy-font-family", "fantasy",
        "default-font-size", 12,
        "default-monospace-font-size", 10,
        "minimum-font-size", 0,
        "enable-caret-browsing", FALSE,
        "enable-page-cache", FALSE,
        "auto-resize-window", TRUE,
        "auto-load-images", TRUE,
        "enable-java-applet", FALSE,
        "enable-plugins", TRUE,
        "enable-hyperlink-auditing", FALSE,
        "editing-behavior", WEBKIT_EDITING_BEHAVIOR_UNIX,
        "enable-fullscreen", TRUE,
        NULL);
    webkit_web_view_set_settings(webView, settings);
    webkit_set_cache_model(WEBKIT_CACHE_MODEL_DOCUMENT_BROWSER);

    DumpRenderTreeSupportGtk::clearMainFrameName(mainFrame);
    DumpRenderTreeSupportGtk::scalePageBy(webView, 1, 0, 0);

    WebKitWebInspector* inspector = webkit_web_view_get_inspector(webView);
    g_object_set(G_OBJECT(inspector), "javascript-profiling-enabled", FALSE, NULL);

    webkit_web_view_set_zoom_level(webView, 1.0);

    DumpRenderTreeSupportGtk::resetOriginAccessWhiteLists();

    WebKitWebBackForwardList* list = webkit_web_view_get_back_forward_list(webView);
    webkit_web_back_forward_list_clear(list);

    SoupSession* session = webkit_get_default_session();
    SoupCookieJar* jar = reinterpret_cast<SoupCookieJar*>(soup_session_get_feature(session, SOUP_TYPE_COOKIE_JAR));

    // We only create the jar when the soup backend needs to do
    // HTTP. Should we initialize it earlier, perhaps?
    if (jar)
        g_object_set(G_OBJECT(jar), SOUP_COOKIE_JAR_ACCEPT_POLICY, SOUP_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY, NULL);

    setlocale(LC_ALL, "");

    DumpRenderTreeSupportGtk::setLinksIncludedInFocusChain(true);
    webkit_icon_database_set_path(webkit_get_icon_database(), 0);
    DumpRenderTreeSupportGtk::setDefersLoading(webView, false);
    DumpRenderTreeSupportGtk::setSerializeHTTPLoads(false);

    if (axController)
        axController->resetToConsistentState();

    DumpRenderTreeSupportGtk::clearOpener(mainFrame);
    DumpRenderTreeSupportGtk::setTracksRepaints(mainFrame, false);

    DumpRenderTreeSupportGtk::resetGeolocationClientMock(webView);

    DumpRenderTreeSupportGtk::setCSSGridLayoutEnabled(webView, false);
    DumpRenderTreeSupportGtk::setCSSRegionsEnabled(webView, true);
    DumpRenderTreeSupportGtk::setCSSCustomFilterEnabled(webView, false);
    DumpRenderTreeSupportGtk::setExperimentalContentSecurityPolicyFeaturesEnabled(true);
    DumpRenderTreeSupportGtk::setSeamlessIFramesEnabled(true);
    DumpRenderTreeSupportGtk::setShadowDOMEnabled(true);
    DumpRenderTreeSupportGtk::setStyleScopedEnabled(true);

    if (gTestRunner) {
        gTestRunner->setAuthenticationPassword("");
        gTestRunner->setAuthenticationUsername("");
        gTestRunner->setHandlesAuthenticationChallenges(false);
    }

    gtk_widget_set_direction(GTK_WIDGET(webView), GTK_TEXT_DIR_NONE);
}

static bool useLongRunningServerMode(int argc, char *argv[])
{
    // This assumes you've already called getopt_long
    return (argc == optind+1 && !strcmp(argv[optind], "-"));
}

static void runTestingServerLoop()
{
    // When DumpRenderTree runs in server mode, we just wait around for file names
    // to be passed to us and read each in turn, passing the results back to the client
    char filenameBuffer[2048];
    while (fgets(filenameBuffer, sizeof(filenameBuffer), stdin)) {
        char* newLineCharacter = strchr(filenameBuffer, '\n');
        if (newLineCharacter)
            *newLineCharacter = '\0';

        if (!strlen(filenameBuffer))
            continue;

        runTest(filenameBuffer);
    }
}

static void initializeGlobalsFromCommandLineOptions(int argc, char *argv[])
{
    struct option options[] = {
        {"notree", no_argument, &dumpTree, false},
        {"pixel-tests", no_argument, &dumpPixelsForAllTests, true},
        {"tree", no_argument, &dumpTree, true},
        {"no-timeout", no_argument, &useTimeoutWatchdog, false},
        {NULL, 0, NULL, 0}
    };
    
    int option;
    while ((option = getopt_long(argc, (char * const *)argv, "", options, NULL)) != -1) {
        switch (option) {
        case '?': // unknown or ambiguous option
        case ':': // missing argument
            exit(1);
            break;
        }
    }
}


void dump()
{
    invalidateAnyPreviousWaitToDumpWatchdog();

    // Grab widget focus before dumping the contents of a widget, in
    // case it was lost in the course of the test.
    gtk_widget_grab_focus(GTK_WIDGET(webView));

    if (dumpTree) {
        char* result = 0;
        gchar* responseMimeType = webkit_web_frame_get_response_mime_type(mainFrame);

        if (g_str_equal(responseMimeType, "text/plain")) {
            gTestRunner->setDumpAsText(true);
            gTestRunner->setGeneratePixelResults(false);
        }
        g_free(responseMimeType);

        if (gTestRunner->dumpAsText())
            result = dumpFramesAsText(mainFrame);
        else {
            // Widget resizing is done asynchronously in GTK+. We pump the main
            // loop here, to flush any pending resize requests. This prevents
            // timing issues which affect the size of elements in the output.
            // We only enable this workaround for tests that print the render tree
            // because this seems to break some dumpAsText tests: see bug 39988
            // After fixing that test, we should apply this approach to all dumps.
            while (gtk_events_pending())
                gtk_main_iteration();

            result = g_strdup(DumpRenderTreeSupportGtk::dumpRenderTree(mainFrame).data());
        }

        if (!result) {
            const char* errorMessage;
            if (gTestRunner->dumpAsText())
                errorMessage = "[documentElement innerText]";
            else if (gTestRunner->dumpDOMAsWebArchive())
                errorMessage = "[[mainFrame DOMDocument] webArchive]";
            else if (gTestRunner->dumpSourceAsWebArchive())
                errorMessage = "[[mainFrame dataSource] webArchive]";
            else
                errorMessage = "[mainFrame renderTreeAsExternalRepresentation]";
            printf("ERROR: nil result from %s", errorMessage);
        } else {
            printf("%s", result);
            g_free(result);
            if (!gTestRunner->dumpAsText() && !gTestRunner->dumpDOMAsWebArchive() && !gTestRunner->dumpSourceAsWebArchive())
                dumpFrameScrollPosition(mainFrame);

            if (gTestRunner->dumpBackForwardList())
                dumpBackForwardListForAllWebViews();
        }

        if (printSeparators) {
            puts("#EOF"); // terminate the content block
            fputs("#EOF\n", stderr);
            fflush(stdout);
            fflush(stderr);
        }
    }

    if (dumpPixelsForCurrentTest
     && gTestRunner->generatePixelResults()
     && !gTestRunner->dumpDOMAsWebArchive()
     && !gTestRunner->dumpSourceAsWebArchive()) {
        DumpRenderTreeSupportGtk::forceWebViewPaint(webView);
        dumpWebViewAsPixelsAndCompareWithExpected(gTestRunner->expectedPixelHash());
    }

    // FIXME: call displayWebView here when we support --paint

    done = true;
    gtk_main_quit();
}

static CString temporaryDatabaseDirectory()
{
    const char* directoryFromEnvironment = g_getenv("DUMPRENDERTREE_TEMP");
    if (directoryFromEnvironment)
        return directoryFromEnvironment;
    GOwnPtr<char> fallback(g_build_filename(g_get_user_data_dir(), "gtkwebkitdrt", "databases", NULL));
    return fallback.get();
}

static void setDefaultsToConsistentStateValuesForTesting()
{
    resetDefaultsToConsistentValues();

#if PLATFORM(X11)
    webkit_web_settings_add_extra_plugin_directory(webView, TEST_PLUGIN_DIR);
#endif

    webkit_set_web_database_directory_path(temporaryDatabaseDirectory().data());

#if defined(GTK_API_VERSION_2)
    gtk_rc_parse_string("style \"nix_scrollbar_spacing\"                    "
                        "{                                                  "
                        "    GtkScrolledWindow::scrollbar-spacing = 0       "
                        "}                                                  "
                        "class \"GtkWidget\" style \"nix_scrollbar_spacing\"");

#else
    GtkCssProvider* cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider,
                                    "@binding-set NoKeyboardNavigation {        "
                                    "   unbind \"<shift>F10\";                  "
                                    "}                                          "
                                    " * {                                       "
                                    "   -GtkScrolledWindow-scrollbar-spacing: 0;"
                                    "   gtk-key-bindings: NoKeyboardNavigation; "
                                    "}                                          ",
                                    -1, 0);
    gtk_style_context_add_provider_for_screen(gdk_display_get_default_screen(gdk_display_get_default()),
                                              GTK_STYLE_PROVIDER(cssProvider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(cssProvider);
#endif
}

static void sendPixelResultsEOF()
{
    puts("#EOF");

    fflush(stdout);
    fflush(stderr);
}

static void runTest(const string& inputLine)
{
    ASSERT(!inputLine.empty());

    TestCommand command = parseInputLine(inputLine);
    string& testURL = command.pathOrURL;
    dumpPixelsForCurrentTest = command.shouldDumpPixels || dumpPixelsForAllTests;

    // Convert the path into a full file URL if it does not look
    // like an HTTP/S URL (doesn't start with http:// or https://).
    if (testURL.find("http://") && testURL.find("https://")) {
        GFile* testFile = g_file_new_for_path(testURL.c_str());
        gchar* testURLCString = g_file_get_uri(testFile);
        testURL = testURLCString;
        g_free(testURLCString);
        g_object_unref(testFile);
    }

    resetDefaultsToConsistentValues();

    gTestRunner = TestRunner::create(testURL, command.expectedPixelHash);
    topLoadingFrame = 0;
    done = false;

    gTestRunner->setIconDatabaseEnabled(false);

    if (shouldLogFrameLoadDelegates(testURL))
        gTestRunner->setDumpFrameLoadCallbacks(true);

    if (shouldEnableDeveloperExtras(testURL)) {
        gTestRunner->setDeveloperExtrasEnabled(true);
        if (shouldOpenWebInspector(testURL))
            gTestRunner->showWebInspector();
        if (shouldDumpAsText(testURL)) {
            gTestRunner->setDumpAsText(true);
            gTestRunner->setGeneratePixelResults(false);
        }
    }

    WorkQueue::shared()->clear();
    WorkQueue::shared()->setFrozen(false);

    bool isSVGW3CTest = (testURL.find("svg/W3C-SVG-1.1") != string::npos);
    GtkAllocation size;
    size.x = size.y = 0;
    size.width = isSVGW3CTest ? TestRunner::w3cSVGViewWidth : TestRunner::viewWidth;
    size.height = isSVGW3CTest ? TestRunner::w3cSVGViewHeight : TestRunner::viewHeight;
    gtk_window_resize(GTK_WINDOW(window), size.width, size.height);
    gtk_widget_size_allocate(container, &size);

    if (prevTestBFItem)
        g_object_unref(prevTestBFItem);
    WebKitWebBackForwardList* bfList = webkit_web_view_get_back_forward_list(webView);
    prevTestBFItem = webkit_web_back_forward_list_get_current_item(bfList);
    if (prevTestBFItem)
        g_object_ref(prevTestBFItem);

    initializeFonts(testURL.c_str());

    // Focus the web view before loading the test to avoid focusing problems
    gtk_widget_grab_focus(GTK_WIDGET(webView));
    webkit_web_view_open(webView, testURL.c_str());

    gtk_main();

    // If developer extras enabled Web Inspector may have been open by the test.
    if (shouldEnableDeveloperExtras(testURL)) {
        gTestRunner->closeWebInspector();
        gTestRunner->setDeveloperExtrasEnabled(false);
    }

    // Also check if we still have opened webViews and free them.
    if (gTestRunner->closeRemainingWindowsWhenComplete() || webViewList) {
        while (webViewList) {
            g_object_unref(WEBKIT_WEB_VIEW(webViewList->data));
            webViewList = g_slist_next(webViewList);
        }
        g_slist_free(webViewList);
        webViewList = 0;
    }

    WebCoreTestSupport::resetInternalsObject(webkit_web_frame_get_global_context(mainFrame));
    DumpRenderTreeSupportGtk::clearMemoryCache();
    DumpRenderTreeSupportGtk::clearApplicationCache();

    // A blank load seems to be necessary to reset state after certain tests.
    webkit_web_view_open(webView, "about:blank");

    gTestRunner.clear();

    // terminate the (possibly empty) pixels block after all the state reset
    sendPixelResultsEOF();
}

void webViewLoadStarted(WebKitWebView* view, WebKitWebFrame* frame, void*)
{
    // Make sure we only set this once per test.  If it gets cleared, and then set again, we might
    // end up doing two dumps for one test.
    if (!topLoadingFrame && !done)
        topLoadingFrame = frame;
}

static gboolean processWork(void* data)
{
    // if we finish all the commands, we're ready to dump state
    if (WorkQueue::shared()->processWork() && !gTestRunner->waitToDump())
        dump();

    return FALSE;
}

static char* getFrameNameSuitableForTestResult(WebKitWebView* view, WebKitWebFrame* frame)
{
    char* frameName = g_strdup(webkit_web_frame_get_name(frame));

    if (frame == webkit_web_view_get_main_frame(view)) {
        // This is a bit strange. Shouldn't web_frame_get_name return NULL?
        if (frameName && (frameName[0] != '\0')) {
            char* tmp = g_strdup_printf("main frame \"%s\"", frameName);
            g_free(frameName);
            frameName = tmp;
        } else {
            g_free(frameName);
            frameName = g_strdup("main frame");
        }
    } else if (!frameName || (frameName[0] == '\0')) {
        g_free(frameName);
        frameName = g_strdup("frame (anonymous)");
    } else {
        char* tmp = g_strdup_printf("frame \"%s\"", frameName);
        g_free(frameName);
        frameName = tmp;
    }

    return frameName;
}

static void webViewLoadFinished(WebKitWebView* view, WebKitWebFrame* frame, void*)
{
    // The deprecated "load-finished" signal is triggered by postProgressFinishedNotification(),
    // so we can use it here in the DRT to provide the correct dump.
    if (frame != topLoadingFrame)
        return;
    if (gTestRunner->dumpProgressFinishedCallback())
        printf("postProgressFinishedNotification\n");
}

static gboolean webViewLoadError(WebKitWebView*, WebKitWebFrame*, gchar*, gpointer, gpointer)
{
    return TRUE; // Return true here to disable the default error page.
}

static void webViewDocumentLoadFinished(WebKitWebView* view, WebKitWebFrame* frame, void*)
{
    if (!done && gTestRunner->dumpFrameLoadCallbacks()) {
        char* frameName = getFrameNameSuitableForTestResult(view, frame);
        printf("%s - didFinishDocumentLoadForFrame\n", frameName);
        g_free(frameName);
    } else if (!done) {
        guint pendingFrameUnloadEvents = DumpRenderTreeSupportGtk::getPendingUnloadEventCount(frame);
        if (pendingFrameUnloadEvents) {
            char* frameName = getFrameNameSuitableForTestResult(view, frame);
            printf("%s - has %u onunload handler(s)\n", frameName, pendingFrameUnloadEvents);
            g_free(frameName);
        }
    }
}

static void webViewOnloadEvent(WebKitWebView* view, WebKitWebFrame* frame, void*)
{
    if (!done && gTestRunner->dumpFrameLoadCallbacks()) {
        char* frameName = getFrameNameSuitableForTestResult(view, frame);
        printf("%s - didHandleOnloadEventsForFrame\n", frameName);
        g_free(frameName);
    }
}

static void addControllerToWindow(JSContextRef context, JSObjectRef windowObject, const char* controllerName, JSValueRef controller)
{
    JSStringRef controllerNameStr = JSStringCreateWithUTF8CString(controllerName);
    JSObjectSetProperty(context, windowObject, controllerNameStr, controller, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete, 0);
    JSStringRelease(controllerNameStr); 
}

static void webViewWindowObjectCleared(WebKitWebView* view, WebKitWebFrame* frame, JSGlobalContextRef context, JSObjectRef windowObject, gpointer data)
{
    JSValueRef exception = 0;
    ASSERT(gTestRunner);

    gTestRunner->makeWindowObject(context, windowObject, &exception);
    ASSERT(!exception);

    gcController->makeWindowObject(context, windowObject, &exception);
    ASSERT(!exception);

    axController->makeWindowObject(context, windowObject, &exception);
    ASSERT(!exception);

    addControllerToWindow(context, windowObject, "eventSender", makeEventSender(context, !webkit_web_frame_get_parent(frame)));
    addControllerToWindow(context, windowObject, "textInputController", makeTextInputController(context));
    WebCoreTestSupport::injectInternalsObject(context);
}

static gboolean webViewConsoleMessage(WebKitWebView* view, const gchar* message, unsigned int line, const gchar* sourceId, gpointer data)
{
    gchar* testMessage = 0;
    const gchar* uriScheme;

    // Tests expect only the filename part of local URIs
    uriScheme = g_strstr_len(message, -1, "file://");
    if (uriScheme) {
        GString* tempString = g_string_sized_new(strlen(message));
        gchar* filename = g_strrstr(uriScheme, G_DIR_SEPARATOR_S);

        if (filename) {
            // If the path is a lone slash, keep it to avoid empty output.
            if (strlen(filename) > 1)
                filename += strlen(G_DIR_SEPARATOR_S);
            tempString = g_string_append_len(tempString, message, (uriScheme - message));
            tempString = g_string_append_len(tempString, filename, strlen(filename));
            testMessage = g_string_free(tempString, FALSE);
        }
    }

    fprintf(stdout, "CONSOLE MESSAGE: ");
    if (line)
        fprintf(stdout, "line %d: ", line);
    fprintf(stdout, "%s\n", testMessage ? testMessage : message);
    g_free(testMessage);

    return TRUE;
}


static gboolean webViewScriptAlert(WebKitWebView* view, WebKitWebFrame* frame, const gchar* message, gpointer data)
{
    fprintf(stdout, "ALERT: %s\n", message);
    fflush(stdout);
    return TRUE;
}

static gboolean webViewScriptPrompt(WebKitWebView* webView, WebKitWebFrame* frame, const gchar* message, const gchar* defaultValue, gchar** value, gpointer data)
{
    fprintf(stdout, "PROMPT: %s, default text: %s\n", message, defaultValue);
    *value = g_strdup(defaultValue);
    return TRUE;
}

static gboolean webViewScriptConfirm(WebKitWebView* view, WebKitWebFrame* frame, const gchar* message, gboolean* didConfirm, gpointer data)
{
    fprintf(stdout, "CONFIRM: %s\n", message);
    *didConfirm = TRUE;
    return TRUE;
}

static void webViewTitleChanged(WebKitWebView* view, WebKitWebFrame* frame, const gchar* title, gpointer data)
{
    if (gTestRunner->dumpFrameLoadCallbacks() && !done) {
        GOwnPtr<char> frameName(getFrameNameSuitableForTestResult(view, frame));
        printf("%s - didReceiveTitle: %s\n", frameName.get(), title ? title : "");
    }

    if (gTestRunner->dumpTitleChanges() && !done)
        printf("TITLE CHANGED: '%s'\n", title ? title : "");
}

static bool webViewNavigationPolicyDecisionRequested(WebKitWebView* view, WebKitWebFrame* frame,
                                                     WebKitNetworkRequest* request,
                                                     WebKitWebNavigationAction* navAction,
                                                     WebKitWebPolicyDecision* policyDecision)
{
    // Use the default handler if we're not waiting for policy,
    // i.e., TestRunner::waitForPolicyDelegate
    if (!waitForPolicy)
        return FALSE;

    gchar* typeDescription;
    WebKitWebNavigationReason reason;
    g_object_get(G_OBJECT(navAction), "reason", &reason, NULL);

    switch(reason) {
        case WEBKIT_WEB_NAVIGATION_REASON_LINK_CLICKED:
            typeDescription = g_strdup("link clicked");
            break;
        case WEBKIT_WEB_NAVIGATION_REASON_FORM_SUBMITTED:
            typeDescription = g_strdup("form submitted");
            break;
        case WEBKIT_WEB_NAVIGATION_REASON_BACK_FORWARD:
            typeDescription = g_strdup("back/forward");
            break;
        case WEBKIT_WEB_NAVIGATION_REASON_RELOAD:
            typeDescription = g_strdup("reload");
            break;
        case WEBKIT_WEB_NAVIGATION_REASON_FORM_RESUBMITTED:
            typeDescription = g_strdup("form resubmitted");
            break;
        case WEBKIT_WEB_NAVIGATION_REASON_OTHER:
            typeDescription = g_strdup("other");
            break;
        default:
            typeDescription = g_strdup("illegal value");
    }

    printf("Policy delegate: attempt to load %s with navigation type '%s'\n", webkit_network_request_get_uri(request), typeDescription);
    g_free(typeDescription);

    webkit_web_policy_decision_ignore(policyDecision);
    gTestRunner->notifyDone();

    return TRUE;
}

static void webViewStatusBarTextChanged(WebKitWebView* view, const gchar* message, gpointer data)
{
    // Are we doing anything wrong? One test that does not call
    // dumpStatusCallbacks gets true here
    if (gTestRunner->dumpStatusCallbacks())
        printf("UI DELEGATE STATUS CALLBACK: setStatusText:%s\n", message);
}

static gboolean webViewClose(WebKitWebView* view)
{
    ASSERT(view);

    webViewList = g_slist_remove(webViewList, view);
    g_object_unref(view);

    return TRUE;
}

static void databaseQuotaExceeded(WebKitWebView* view, WebKitWebFrame* frame, WebKitWebDatabase *database)
{
    ASSERT(view);
    ASSERT(frame);
    ASSERT(database);

    WebKitSecurityOrigin* origin = webkit_web_database_get_security_origin(database);
    if (gTestRunner->dumpDatabaseCallbacks()) {
        printf("UI DELEGATE DATABASE CALLBACK: exceededDatabaseQuotaForSecurityOrigin:{%s, %s, %i} database:%s\n",
            webkit_security_origin_get_protocol(origin),
            webkit_security_origin_get_host(origin),
            webkit_security_origin_get_port(origin),
            webkit_web_database_get_name(database));
    }
    webkit_security_origin_set_web_database_quota(origin, 5 * 1024 * 1024);
}

static bool
geolocationPolicyDecisionRequested(WebKitWebView*, WebKitWebFrame*, WebKitGeolocationPolicyDecision* decision)
{
    if (!gTestRunner->isGeolocationPermissionSet())
        return FALSE;
    if (gTestRunner->geolocationPermission())
        webkit_geolocation_policy_allow(decision);
    else
        webkit_geolocation_policy_deny(decision);

    return TRUE;
}


static WebKitWebView* webViewCreate(WebKitWebView*, WebKitWebFrame*);

static gboolean webInspectorShowWindow(WebKitWebInspector*, gpointer data)
{
    gtk_window_set_default_size(GTK_WINDOW(webInspectorWindow), TestRunner::viewWidth, TestRunner::viewHeight);
    gtk_widget_show_all(webInspectorWindow);
    return TRUE;
}

static gboolean webInspectorCloseWindow(WebKitWebInspector*, gpointer data)
{
    gtk_widget_destroy(webInspectorWindow);
    webInspectorWindow = 0;
    return TRUE;
}

static WebKitWebView* webInspectorInspectWebView(WebKitWebInspector*, gpointer data)
{
    webInspectorWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    GtkWidget* webView = self_scrolling_webkit_web_view_new();
    gtk_container_add(GTK_CONTAINER(webInspectorWindow),
                      webView);

    return WEBKIT_WEB_VIEW(webView);
}

static void topLoadingFrameLoadFinished()
{
    topLoadingFrame = 0;
    WorkQueue::shared()->setFrozen(true); // first complete load freezes the queue for the rest of this test
    if (gTestRunner->waitToDump())
        return;

    if (WorkQueue::shared()->count())
        g_timeout_add(0, processWork, 0);
    else
        dump();
}

static void webFrameLoadStatusNotified(WebKitWebFrame* frame, gpointer user_data)
{
    WebKitLoadStatus loadStatus = webkit_web_frame_get_load_status(frame);

    if (gTestRunner->dumpFrameLoadCallbacks()) {
        GOwnPtr<char> frameName(getFrameNameSuitableForTestResult(webkit_web_frame_get_web_view(frame), frame));

        switch (loadStatus) {
        case WEBKIT_LOAD_PROVISIONAL:
            if (!done)
                printf("%s - didStartProvisionalLoadForFrame\n", frameName.get());
            break;
        case WEBKIT_LOAD_COMMITTED:
            if (!done)
                printf("%s - didCommitLoadForFrame\n", frameName.get());
            break;
        case WEBKIT_LOAD_FINISHED:
            if (!done)
                printf("%s - didFinishLoadForFrame\n", frameName.get());
            break;
        default:
            break;
        }
    }

    if ((loadStatus == WEBKIT_LOAD_FINISHED || loadStatus == WEBKIT_LOAD_FAILED)
        && frame == topLoadingFrame)
        topLoadingFrameLoadFinished();
}

static void frameCreatedCallback(WebKitWebView* webView, WebKitWebFrame* webFrame, gpointer user_data)
{
    g_signal_connect(webFrame, "notify::load-status", G_CALLBACK(webFrameLoadStatusNotified), NULL);
    g_signal_connect(webFrame, "insecure-content-run", G_CALLBACK(didRunInsecureContent), NULL);
}


static CString pathFromSoupURI(SoupURI* uri)
{
    if (!uri)
        return CString();

    if (g_str_equal(uri->scheme, "http") || g_str_equal(uri->scheme, "ftp")) {
        GOwnPtr<char> uriString(soup_uri_to_string(uri, FALSE));
        return CString(uriString.get());
    }

    GOwnPtr<gchar> parentPath(g_path_get_dirname(uri->path));
    GOwnPtr<gchar> pathDirname(g_path_get_basename(parentPath.get()));
    GOwnPtr<gchar> pathBasename(g_path_get_basename(uri->path));
    GOwnPtr<gchar> urlPath(g_strdup_printf("%s/%s", pathDirname.get(), pathBasename.get()));
    return CString(urlPath.get());
}

static CString convertSoupMessageToURLPath(SoupMessage* soupMessage)
{
    if (!soupMessage)
        return CString();
    if (SoupURI* requestURI = soup_message_get_uri(soupMessage))
        return pathFromSoupURI(requestURI);
    return CString();
}

static CString convertNetworkRequestToURLPath(WebKitNetworkRequest* request)
{
    return convertSoupMessageToURLPath(webkit_network_request_get_message(request));
}

static CString convertWebResourceToURLPath(WebKitWebResource* webResource)
{
    SoupURI* uri = soup_uri_new(webkit_web_resource_get_uri(webResource));
    CString urlPath(pathFromSoupURI(uri));
    soup_uri_free(uri);
    return urlPath;
}

static CString urlSuitableForTestResult(const char* uriString)
{
    if (!g_str_has_prefix(uriString, "file://"))
        return CString(uriString);

    GOwnPtr<gchar> basename(g_path_get_basename(uriString));
    return CString(basename.get());
}

static CString descriptionSuitableForTestResult(SoupURI* uri)
{
    if (!uri)
        return CString("");

    GOwnPtr<char> uriString(soup_uri_to_string(uri, false));
    return urlSuitableForTestResult(uriString.get());
}

static CString descriptionSuitableForTestResult(WebKitWebView* webView, WebKitWebFrame* webFrame, WebKitWebResource* webResource)
{
    SoupURI* uri = soup_uri_new(webkit_web_resource_get_uri(webResource));
    CString description;
    WebKitWebDataSource* dataSource = webkit_web_frame_get_data_source(webFrame);

    if (webResource == webkit_web_data_source_get_main_resource(dataSource)
        && (!webkit_web_view_get_progress(webView) || g_str_equal(uri->scheme, "file")))
        description = CString("<unknown>");
    else
        description = convertWebResourceToURLPath(webResource);

    if (uri)
        soup_uri_free(uri);

    return description;
}

static CString descriptionSuitableForTestResult(GError* error, WebKitWebResource* webResource)
{
    const gchar* errorDomain = g_quark_to_string(error->domain);
    CString resourceURIString(urlSuitableForTestResult(webkit_web_resource_get_uri(webResource)));

    if (g_str_equal(errorDomain, "webkit-network-error-quark") || g_str_equal(errorDomain, "soup_http_error_quark"))
        errorDomain = "NSURLErrorDomain";

    if (g_str_equal(errorDomain, "WebKitPolicyError"))
        errorDomain = "WebKitErrorDomain";

    // TODO: the other ports get the failingURL from the ResourceError
    GOwnPtr<char> errorString(g_strdup_printf("<NSError domain %s, code %d, failing URL \"%s\">",
                                              errorDomain, error->code, resourceURIString.data()));
    return CString(errorString.get());
}

static CString descriptionSuitableForTestResult(WebKitNetworkRequest* request)
{
    SoupMessage* soupMessage = webkit_network_request_get_message(request);

    if (!soupMessage)
        return CString("");

    SoupURI* requestURI = soup_message_get_uri(soupMessage);
    SoupURI* mainDocumentURI = soup_message_get_first_party(soupMessage);
    CString requestURIString(descriptionSuitableForTestResult(requestURI));
    CString mainDocumentURIString(descriptionSuitableForTestResult(mainDocumentURI));
    CString path(convertNetworkRequestToURLPath(request));
    GOwnPtr<char> description(g_strdup_printf("<NSURLRequest URL %s, main document URL %s, http method %s>",
        path.data(), mainDocumentURIString.data(), soupMessage->method));
    return CString(description.get());
}

static CString descriptionSuitableForTestResult(WebKitNetworkResponse* response)
{
    if (!response)
        return CString("(null)");

    int statusCode = 0;
    CString responseURIString(urlSuitableForTestResult(webkit_network_response_get_uri(response)));
    SoupMessage* soupMessage = webkit_network_response_get_message(response);
    CString path;

    if (soupMessage) {
        statusCode = soupMessage->status_code;
        path = convertSoupMessageToURLPath(soupMessage);
    } else
        path = CString("");

    GOwnPtr<char> description(g_strdup_printf("<NSURLResponse %s, http status code %d>", path.data(), statusCode));
    return CString(description.get());
}

static void willSendRequestCallback(WebKitWebView* webView, WebKitWebFrame* webFrame, WebKitWebResource* resource, WebKitNetworkRequest* request, WebKitNetworkResponse* response)
{


    if (!done && gTestRunner->willSendRequestReturnsNull()) {
        // As requested by the TestRunner, don't perform the request.
        webkit_network_request_set_uri(request, "about:blank");
        return;
    }

    if (!done && gTestRunner->dumpResourceLoadCallbacks())
        printf("%s - willSendRequest %s redirectResponse %s\n",
               convertNetworkRequestToURLPath(request).data(),
               descriptionSuitableForTestResult(request).data(),
               descriptionSuitableForTestResult(response).data());

    SoupMessage* soupMessage = webkit_network_request_get_message(request);
    SoupURI* uri = soup_uri_new(webkit_network_request_get_uri(request));

    if (SOUP_URI_IS_VALID(uri)) {
        GOwnPtr<char> uriString(soup_uri_to_string(uri, FALSE));

        if (SOUP_URI_VALID_FOR_HTTP(uri) && g_strcmp0(uri->host, "127.0.0.1")
            && g_strcmp0(uri->host, "255.255.255.255")
            && g_ascii_strncasecmp(uri->host, "localhost", 9)) {
            printf("Blocked access to external URL %s\n", uriString.get());
            // Cancel load of blocked resource to avoid potential
            // network-related timeouts in tests.
            webkit_network_request_set_uri(request, "about:blank");
            soup_uri_free(uri);
            return;
        }

        const string& destination = gTestRunner->redirectionDestinationForURL(uriString.get());
        if (!destination.empty())
            webkit_network_request_set_uri(request, destination.c_str());
    }

    if (uri)
        soup_uri_free(uri);

    if (soupMessage) {
        const set<string>& clearHeaders = gTestRunner->willSendRequestClearHeaders();
        for (set<string>::const_iterator header = clearHeaders.begin(); header != clearHeaders.end(); ++header)
            soup_message_headers_remove(soupMessage->request_headers, header->c_str());
    }
}


static void didReceiveResponse(WebKitWebView* webView, WebKitWebFrame*, WebKitWebResource* webResource, WebKitNetworkResponse* response)
{
    if (!done && gTestRunner->dumpResourceLoadCallbacks()) {
        CString responseDescription(descriptionSuitableForTestResult(response));
        CString path(convertWebResourceToURLPath(webResource));
        printf("%s - didReceiveResponse %s\n", path.data(), responseDescription.data());
    }

    // TODO: add "has MIME type" whenever dumpResourceResponseMIMETypes() is supported.
    // See https://bugs.webkit.org/show_bug.cgi?id=58222.
}

static void didFinishLoading(WebKitWebView* webView, WebKitWebFrame* webFrame, WebKitWebResource* webResource)
{
    if (!done && gTestRunner->dumpResourceLoadCallbacks())
        printf("%s - didFinishLoading\n", descriptionSuitableForTestResult(webView, webFrame, webResource).data());
}

static void didFailLoadingWithError(WebKitWebView* webView, WebKitWebFrame* webFrame, WebKitWebResource* webResource, GError* webError)
{
    if (!done && gTestRunner->dumpResourceLoadCallbacks()) {
        CString webErrorString(descriptionSuitableForTestResult(webError, webResource));
        printf("%s - didFailLoadingWithError: %s\n", descriptionSuitableForTestResult(webView, webFrame, webResource).data(),
               webErrorString.data());
    }
}

static void didRunInsecureContent(WebKitWebFrame*, WebKitSecurityOrigin*, const char* url)
{
    if (!done && gTestRunner->dumpFrameLoadCallbacks())
        printf("didRunInsecureContent\n");
}

static gboolean webViewRunFileChooser(WebKitWebView*, WebKitFileChooserRequest*)
{
    // We return TRUE to not propagate the event further so the
    // default file chooser dialog is not shown.
    return TRUE;
}

static void frameLoadEventCallback(WebKitWebFrame* frame, DumpRenderTreeSupportGtk::FrameLoadEvent event, const char* url)
{
    if (done || !gTestRunner->dumpFrameLoadCallbacks())
        return;

    GOwnPtr<char> frameName(getFrameNameSuitableForTestResult(webkit_web_frame_get_web_view(frame), frame));
    switch (event) {
    case DumpRenderTreeSupportGtk::WillPerformClientRedirectToURL:
        ASSERT(url);
        printf("%s - willPerformClientRedirectToURL: %s \n", frameName.get(), url);
        break;
    case DumpRenderTreeSupportGtk::DidCancelClientRedirect:
        printf("%s - didCancelClientRedirectForFrame\n", frameName.get());
        break;
    case DumpRenderTreeSupportGtk::DidReceiveServerRedirectForProvisionalLoad:
        printf("%s - didReceiveServerRedirectForProvisionalLoadForFrame\n", frameName.get());
        break;
    case DumpRenderTreeSupportGtk::DidDisplayInsecureContent:
        printf ("didDisplayInsecureContent\n");
        break;
    case DumpRenderTreeSupportGtk::DidDetectXSS:
        printf ("didDetectXSS\n");
        break;
    default:
        ASSERT_NOT_REACHED();
    }
}

static bool authenticationCallback(CString& username, CString& password)
{
    if (!gTestRunner->handlesAuthenticationChallenges()) {
        printf("<unknown> - didReceiveAuthenticationChallenge - Simulating cancelled authentication sheet\n");
        return false;
    }

    username = gTestRunner->authenticationUsername().c_str();
    password = gTestRunner->authenticationPassword().c_str();
    printf("<unknown> - didReceiveAuthenticationChallenge - Responding with %s:%s\n", username.data(), password.data());
    return true;
}

static WebKitWebView* createWebView()
{
    // It is important to declare DRT is running early so when creating
    // web view mock clients are used instead of proper ones.
    DumpRenderTreeSupportGtk::setDumpRenderTreeModeEnabled(true);

    DumpRenderTreeSupportGtk::setFrameLoadEventCallback(frameLoadEventCallback);
    DumpRenderTreeSupportGtk::setAuthenticationCallback(authenticationCallback);

    WebKitWebView* view = WEBKIT_WEB_VIEW(self_scrolling_webkit_web_view_new());

    g_object_connect(G_OBJECT(view),
                     "signal::load-started", webViewLoadStarted, 0,
                     "signal::load-finished", webViewLoadFinished, 0,
                     "signal::load-error", webViewLoadError, 0,
                     "signal::window-object-cleared", webViewWindowObjectCleared, 0,
                     "signal::console-message", webViewConsoleMessage, 0,
                     "signal::script-alert", webViewScriptAlert, 0,
                     "signal::script-prompt", webViewScriptPrompt, 0,
                     "signal::script-confirm", webViewScriptConfirm, 0,
                     "signal::title-changed", webViewTitleChanged, 0,
                     "signal::navigation-policy-decision-requested", webViewNavigationPolicyDecisionRequested, 0,
                     "signal::status-bar-text-changed", webViewStatusBarTextChanged, 0,
                     "signal::create-web-view", webViewCreate, 0,
                     "signal::close-web-view", webViewClose, 0,
                     "signal::database-quota-exceeded", databaseQuotaExceeded, 0,
                     "signal::document-load-finished", webViewDocumentLoadFinished, 0,
                     "signal::geolocation-policy-decision-requested", geolocationPolicyDecisionRequested, 0,
                     "signal::onload-event", webViewOnloadEvent, 0,
                     "signal::drag-begin", dragBeginCallback, 0,
                     "signal::drag-end", dragEndCallback, 0,
                     "signal::drag-failed", dragFailedCallback, 0,
                     "signal::frame-created", frameCreatedCallback, 0,
                     "signal::resource-request-starting", willSendRequestCallback, 0,
                     "signal::resource-response-received", didReceiveResponse, 0,
                     "signal::resource-load-finished", didFinishLoading, 0,
                     "signal::resource-load-failed", didFailLoadingWithError, 0,
                     "signal::run-file-chooser", webViewRunFileChooser, 0,
                     NULL);
    connectEditingCallbacks(view);

    WebKitWebInspector* inspector = webkit_web_view_get_inspector(view);
    g_object_connect(G_OBJECT(inspector),
                     "signal::inspect-web-view", webInspectorInspectWebView, 0,
                     "signal::show-window", webInspectorShowWindow, 0,
                     "signal::close-window", webInspectorCloseWindow, 0,
                     NULL);

    if (webView) {
        WebKitWebSettings* settings = webkit_web_view_get_settings(webView);
        webkit_web_view_set_settings(view, settings);
    }

    // frame-created is not issued for main frame. That's why we must do this here
    WebKitWebFrame* frame = webkit_web_view_get_main_frame(view);
    g_signal_connect(frame, "notify::load-status", G_CALLBACK(webFrameLoadStatusNotified), NULL);
    g_signal_connect(frame, "insecure-content-run", G_CALLBACK(didRunInsecureContent), NULL);

    return view;
}

static WebKitWebView* webViewCreate(WebKitWebView* view, WebKitWebFrame* frame)
{
    if (!gTestRunner->canOpenWindows())
        return 0;

    // Make sure that waitUntilDone has been called.
    ASSERT(gTestRunner->waitToDump());

    WebKitWebView* newWebView = createWebView();
    g_object_ref_sink(G_OBJECT(newWebView));
    webViewList = g_slist_prepend(webViewList, newWebView);
    return newWebView;
}

static void logHandler(const gchar* domain, GLogLevelFlags level, const gchar* message, gpointer data)
{
    if (level < G_LOG_LEVEL_DEBUG)
        fprintf(stderr, "%s\n", message);
}

int main(int argc, char* argv[])
{
    gtk_init(&argc, &argv);

    // Some plugins might try to use the GLib logger for printing debug
    // messages. This will cause tests to fail because of unexpected output.
    // We squelch all debug messages sent to the logger.
    g_log_set_default_handler(logHandler, 0);

    initializeGlobalsFromCommandLineOptions(argc, argv);
    initializeFonts();

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
#ifdef GTK_API_VERSION_2
    container = gtk_hbox_new(TRUE, 0);
#else
    container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(container), TRUE);
#endif
    gtk_container_add(GTK_CONTAINER(window), container);
    gtk_widget_show_all(window);

    webView = createWebView();
    gtk_box_pack_start(GTK_BOX(container), GTK_WIDGET(webView), TRUE, TRUE, 0);
    gtk_widget_realize(GTK_WIDGET(webView));
    gtk_widget_show_all(container);
    mainFrame = webkit_web_view_get_main_frame(webView);

    setDefaultsToConsistentStateValuesForTesting();

    gcController = new GCController();
    axController = new AccessibilityController();

    if (useLongRunningServerMode(argc, argv)) {
        printSeparators = true;
        runTestingServerLoop();
    } else {
        printSeparators = (optind < argc-1 || (dumpPixelsForCurrentTest && dumpTree));
        for (int i = optind; i != argc; ++i)
            runTest(argv[i]);
    }

    delete gcController;
    gcController = 0;

    delete axController;
    axController = 0;

    gtk_widget_destroy(window);

    return 0;
}
