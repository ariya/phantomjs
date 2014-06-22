/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TestController.h"

#include "PlatformWebView.h"
#include "StringFunctions.h"
#include "TestInvocation.h"
#include <WebKit2/WKAuthenticationChallenge.h>
#include <WebKit2/WKAuthenticationDecisionListener.h>
#include <WebKit2/WKContextPrivate.h>
#include <WebKit2/WKCredential.h>
#include <WebKit2/WKIconDatabase.h>
#include <WebKit2/WKNotification.h>
#include <WebKit2/WKNotificationManager.h>
#include <WebKit2/WKNotificationPermissionRequest.h>
#include <WebKit2/WKNumber.h>
#include <WebKit2/WKPageGroup.h>
#include <WebKit2/WKPagePrivate.h>
#include <WebKit2/WKPreferencesPrivate.h>
#include <WebKit2/WKRetainPtr.h>
#include <algorithm>
#include <cstdio>
#include <ctype.h>
#include <stdlib.h>
#include <string>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/CString.h>

#if PLATFORM(MAC)
#include <WebKit2/WKPagePrivateMac.h>
#endif

#if PLATFORM(MAC) || PLATFORM(QT) || PLATFORM(GTK) || PLATFORM(EFL)
#include "EventSenderProxy.h"
#endif

#if !PLATFORM(MAC)
#include <WebKit2/WKTextChecker.h>
#endif

namespace WTR {

const unsigned TestController::viewWidth = 800;
const unsigned TestController::viewHeight = 600;

const unsigned TestController::w3cSVGViewWidth = 480;
const unsigned TestController::w3cSVGViewHeight = 360;

// defaultLongTimeout + defaultShortTimeout should be less than 80,
// the default timeout value of the test harness so we can detect an
// unresponsive web process.
static const double defaultLongTimeout = 60;
static const double defaultShortTimeout = 15;
static const double defaultNoTimeout = -1;

static WKURLRef blankURL()
{
    static WKURLRef staticBlankURL = WKURLCreateWithUTF8CString("about:blank");
    return staticBlankURL;
}

static TestController* controller;

TestController& TestController::shared()
{
    ASSERT(controller);
    return *controller;
}

TestController::TestController(int argc, const char* argv[])
    : m_verbose(false)
    , m_printSeparators(false)
    , m_usingServerMode(false)
    , m_gcBetweenTests(false)
    , m_shouldDumpPixelsForAllTests(false)
    , m_state(Initial)
    , m_doneResetting(false)
    , m_longTimeout(defaultLongTimeout)
    , m_shortTimeout(defaultShortTimeout)
    , m_noTimeout(defaultNoTimeout)
    , m_useWaitToDumpWatchdogTimer(true)
    , m_forceNoTimeout(false)
    , m_timeout(0)
    , m_didPrintWebProcessCrashedMessage(false)
    , m_shouldExitWhenWebProcessCrashes(true)
    , m_beforeUnloadReturnValue(true)
    , m_isGeolocationPermissionSet(false)
    , m_isGeolocationPermissionAllowed(false)
    , m_policyDelegateEnabled(false)
    , m_policyDelegatePermissive(false)
    , m_handlesAuthenticationChallenges(false)
    , m_shouldBlockAllPlugins(false)
{
    initialize(argc, argv);
    controller = this;
    run();
    controller = 0;
}

TestController::~TestController()
{
    WKIconDatabaseClose(WKContextGetIconDatabase(m_context.get()));

    platformDestroy();
}

static WKRect getWindowFrame(WKPageRef page, const void* clientInfo)
{
    PlatformWebView* view = static_cast<PlatformWebView*>(const_cast<void*>(clientInfo));
    return view->windowFrame();
}

static void setWindowFrame(WKPageRef page, WKRect frame, const void* clientInfo)
{
    PlatformWebView* view = static_cast<PlatformWebView*>(const_cast<void*>(clientInfo));
    view->setWindowFrame(frame);
}

static bool runBeforeUnloadConfirmPanel(WKPageRef page, WKStringRef message, WKFrameRef frame, const void*)
{
    printf("CONFIRM NAVIGATION: %s\n", toSTD(message).c_str());
    return TestController::shared().beforeUnloadReturnValue();
}

static unsigned long long exceededDatabaseQuota(WKPageRef, WKFrameRef, WKSecurityOriginRef, WKStringRef, WKStringRef, unsigned long long, unsigned long long, unsigned long long, unsigned long long, const void*)
{
    static const unsigned long long defaultQuota = 5 * 1024 * 1024;
    return defaultQuota;
}


void TestController::runModal(WKPageRef page, const void* clientInfo)
{
    PlatformWebView* view = static_cast<PlatformWebView*>(const_cast<void*>(clientInfo));
    view->setWindowIsKey(false);
    runModal(view);
    view->setWindowIsKey(true);
}

static void closeOtherPage(WKPageRef page, const void* clientInfo)
{
    WKPageClose(page);
    PlatformWebView* view = static_cast<PlatformWebView*>(const_cast<void*>(clientInfo));
    delete view;
}

static void focus(WKPageRef page, const void* clientInfo)
{
    PlatformWebView* view = static_cast<PlatformWebView*>(const_cast<void*>(clientInfo));
    view->focus();
    view->setWindowIsKey(true);
}

static void unfocus(WKPageRef page, const void* clientInfo)
{
    PlatformWebView* view = static_cast<PlatformWebView*>(const_cast<void*>(clientInfo));
    view->setWindowIsKey(false);
}

static void decidePolicyForGeolocationPermissionRequest(WKPageRef, WKFrameRef, WKSecurityOriginRef, WKGeolocationPermissionRequestRef permissionRequest, const void* clientInfo)
{
    TestController::shared().handleGeolocationPermissionRequest(permissionRequest);
}

int TestController::getCustomTimeout()
{
    return m_timeout;
}

WKPageRef TestController::createOtherPage(WKPageRef oldPage, WKURLRequestRef, WKDictionaryRef, WKEventModifiers, WKEventMouseButton, const void* clientInfo)
{
    PlatformWebView* parentView = static_cast<PlatformWebView*>(const_cast<void*>(clientInfo));

    PlatformWebView* view = new PlatformWebView(WKPageGetContext(oldPage), WKPageGetPageGroup(oldPage), oldPage, parentView->options());
    WKPageRef newPage = view->page();

    view->resizeTo(800, 600);

    WKPageUIClient otherPageUIClient = {
        kWKPageUIClientCurrentVersion,
        view,
        0, // createNewPage_deprecatedForUseWithV0
        0, // showPage
        closeOtherPage,
        0, // takeFocus
        focus,
        unfocus,
        0, // runJavaScriptAlert
        0, // runJavaScriptConfirm
        0, // runJavaScriptPrompt
        0, // setStatusText
        0, // mouseDidMoveOverElement_deprecatedForUseWithV0
        0, // missingPluginButtonClicked
        0, // didNotHandleKeyEvent
        0, // didNotHandleWheelEvent
        0, // toolbarsAreVisible
        0, // setToolbarsAreVisible
        0, // menuBarIsVisible
        0, // setMenuBarIsVisible
        0, // statusBarIsVisible
        0, // setStatusBarIsVisible
        0, // isResizable
        0, // setIsResizable
        getWindowFrame,
        setWindowFrame,
        runBeforeUnloadConfirmPanel,
        0, // didDraw
        0, // pageDidScroll
        exceededDatabaseQuota,
        0, // runOpenPanel
        decidePolicyForGeolocationPermissionRequest,
        0, // headerHeight
        0, // footerHeight
        0, // drawHeader
        0, // drawFooter
        0, // printFrame
        runModal,
        0, // didCompleteRubberBandForMainFrame
        0, // saveDataToFileInDownloadsFolder
        0, // shouldInterruptJavaScript
        createOtherPage,
        0, // mouseDidMoveOverElement
        0, // decidePolicyForNotificationPermissionRequest
        0, // unavailablePluginButtonClicked_deprecatedForUseWithV1
        0, // showColorPicker
        0, // hideColorPicker
        0, // unavailablePluginButtonClicked
    };
    WKPageSetPageUIClient(newPage, &otherPageUIClient);

    view->didInitializeClients();

    WKRetain(newPage);
    return newPage;
}

const char* TestController::libraryPathForTesting()
{
    // FIXME: This may not be sufficient to prevent interactions/crashes
    // when running more than one copy of DumpRenderTree.
    // See https://bugs.webkit.org/show_bug.cgi?id=10906
    char* dumpRenderTreeTemp = getenv("DUMPRENDERTREE_TEMP");
    if (dumpRenderTreeTemp)
        return dumpRenderTreeTemp;
    return platformLibraryPathForTesting();
}


void TestController::initialize(int argc, const char* argv[])
{
    platformInitialize();

    if (argc < 2) {
        fputs("Usage: WebKitTestRunner [options] filename [filename2..n]\n", stderr);
        // FIXME: Refactor option parsing to allow us to print
        // an auto-generated list of options.
        exit(1);
    }

    bool printSupportedFeatures = false;

    for (int i = 1; i < argc; ++i) {
        std::string argument(argv[i]);

        if (argument == "--timeout" && i + 1 < argc) {
            m_longTimeout = atoi(argv[++i]);
            // Scale up the short timeout to match.
            m_shortTimeout = defaultShortTimeout * m_longTimeout / defaultLongTimeout;
            continue;
        }

        if (argument == "--no-timeout") {
            m_useWaitToDumpWatchdogTimer = false;
            continue;
        }

        if (argument == "--no-timeout-at-all") {
            m_useWaitToDumpWatchdogTimer = false;
            m_forceNoTimeout = true;
            continue;
        }

        if (argument == "--verbose") {
            m_verbose = true;
            continue;
        }
        if (argument == "--gc-between-tests") {
            m_gcBetweenTests = true;
            continue;
        }
        if (argument == "--pixel-tests" || argument == "-p") {
            m_shouldDumpPixelsForAllTests = true;
            continue;
        }
        if (argument == "--print-supported-features") {
            printSupportedFeatures = true;
            break;
        }


        // Skip any other arguments that begin with '--'.
        if (argument.length() >= 2 && argument[0] == '-' && argument[1] == '-')
            continue;

        m_paths.push_back(argument);
    }

    if (printSupportedFeatures) {
        // FIXME: On Windows, DumpRenderTree uses this to expose whether it supports 3d
        // transforms and accelerated compositing. When we support those features, we
        // should match DRT's behavior.
        exit(0);
    }

    m_usingServerMode = (m_paths.size() == 1 && m_paths[0] == "-");
    if (m_usingServerMode)
        m_printSeparators = true;
    else
        m_printSeparators = m_paths.size() > 1;

    initializeInjectedBundlePath();
    initializeTestPluginDirectory();

    WKRetainPtr<WKStringRef> pageGroupIdentifier(AdoptWK, WKStringCreateWithUTF8CString("WebKitTestRunnerPageGroup"));
    m_pageGroup.adopt(WKPageGroupCreateWithIdentifier(pageGroupIdentifier.get()));

    m_context.adopt(WKContextCreateWithInjectedBundlePath(injectedBundlePath()));
    m_geolocationProvider = adoptPtr(new GeolocationProviderMock(m_context.get()));

#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED > 1080
    WKContextSetUsesNetworkProcess(m_context.get(), true);
    WKContextSetProcessModel(m_context.get(), kWKProcessModelMultipleSecondaryProcesses);
#endif

    if (const char* dumpRenderTreeTemp = libraryPathForTesting()) {
        String temporaryFolder = String::fromUTF8(dumpRenderTreeTemp);

        // WebCore::pathByAppendingComponent is not used here because of the namespace,
        // which leads us to this ugly #ifdef and file path concatenation.
#if OS(WINDOWS)
        const char separator = '\\';
#else
        const char separator = '/';
#endif

        WKContextSetApplicationCacheDirectory(m_context.get(), toWK(temporaryFolder + separator + "ApplicationCache").get());
        WKContextSetDatabaseDirectory(m_context.get(), toWK(temporaryFolder + separator + "Databases").get());
        WKContextSetLocalStorageDirectory(m_context.get(), toWK(temporaryFolder + separator + "LocalStorage").get());
        WKContextSetDiskCacheDirectory(m_context.get(), toWK(temporaryFolder + separator + "Cache").get());
        WKContextSetCookieStorageDirectory(m_context.get(), toWK(temporaryFolder + separator + "Cookies").get());
        WKContextSetIconDatabasePath(m_context.get(), toWK(temporaryFolder + separator + "IconDatabase" + separator + "WebpageIcons.db").get());
    }

    platformInitializeContext();

    WKContextInjectedBundleClient injectedBundleClient = {
        kWKContextInjectedBundleClientCurrentVersion,
        this,
        didReceiveMessageFromInjectedBundle,
        didReceiveSynchronousMessageFromInjectedBundle,
        0 // getInjectedBundleInitializationUserData
    };
    WKContextSetInjectedBundleClient(m_context.get(), &injectedBundleClient);

    WKNotificationManagerRef notificationManager = WKContextGetNotificationManager(m_context.get());
    WKNotificationProvider notificationKit = m_webNotificationProvider.provider();
    WKNotificationManagerSetProvider(notificationManager, &notificationKit);

    if (testPluginDirectory())
        WKContextSetAdditionalPluginsDirectory(m_context.get(), testPluginDirectory());

    createWebViewWithOptions(0);
}

void TestController::createWebViewWithOptions(WKDictionaryRef options)
{
    m_mainWebView = adoptPtr(new PlatformWebView(m_context.get(), m_pageGroup.get(), 0, options));
    WKPageUIClient pageUIClient = {
        kWKPageUIClientCurrentVersion,
        m_mainWebView.get(),
        0, // createNewPage_deprecatedForUseWithV0
        0, // showPage
        0, // close
        0, // takeFocus
        focus,
        unfocus,
        0, // runJavaScriptAlert
        0, // runJavaScriptConfirm
        0, // runJavaScriptPrompt
        0, // setStatusText
        0, // mouseDidMoveOverElement_deprecatedForUseWithV0
        0, // missingPluginButtonClicked
        0, // didNotHandleKeyEvent
        0, // didNotHandleWheelEvent
        0, // toolbarsAreVisible
        0, // setToolbarsAreVisible
        0, // menuBarIsVisible
        0, // setMenuBarIsVisible
        0, // statusBarIsVisible
        0, // setStatusBarIsVisible
        0, // isResizable
        0, // setIsResizable
        getWindowFrame,
        setWindowFrame,
        runBeforeUnloadConfirmPanel,
        0, // didDraw
        0, // pageDidScroll
        exceededDatabaseQuota,
        0, // runOpenPanel
        decidePolicyForGeolocationPermissionRequest,
        0, // headerHeight
        0, // footerHeight
        0, // drawHeader
        0, // drawFooter
        0, // printFrame
        runModal,
        0, // didCompleteRubberBandForMainFrame
        0, // saveDataToFileInDownloadsFolder
        0, // shouldInterruptJavaScript
        createOtherPage,
        0, // mouseDidMoveOverElement
        decidePolicyForNotificationPermissionRequest, // decidePolicyForNotificationPermissionRequest
        0, // unavailablePluginButtonClicked_deprecatedForUseWithV1
        0, // showColorPicker
        0, // hideColorPicker
        unavailablePluginButtonClicked,
    };
    WKPageSetPageUIClient(m_mainWebView->page(), &pageUIClient);

    WKPageLoaderClient pageLoaderClient = {
        kWKPageLoaderClientCurrentVersion,
        this,
        0, // didStartProvisionalLoadForFrame
        0, // didReceiveServerRedirectForProvisionalLoadForFrame
        0, // didFailProvisionalLoadWithErrorForFrame
        didCommitLoadForFrame,
        0, // didFinishDocumentLoadForFrame
        didFinishLoadForFrame,
        0, // didFailLoadWithErrorForFrame
        0, // didSameDocumentNavigationForFrame
        0, // didReceiveTitleForFrame
        0, // didFirstLayoutForFrame
        0, // didFirstVisuallyNonEmptyLayoutForFrame
        0, // didRemoveFrameFromHierarchy
        0, // didFailToInitializePlugin
        0, // didDisplayInsecureContentForFrame
        0, // canAuthenticateAgainstProtectionSpaceInFrame
        didReceiveAuthenticationChallengeInFrame, // didReceiveAuthenticationChallengeInFrame
        0, // didStartProgress
        0, // didChangeProgress
        0, // didFinishProgress
        0, // didBecomeUnresponsive
        0, // didBecomeResponsive
        processDidCrash,
        0, // didChangeBackForwardList
        0, // shouldGoToBackForwardListItem
        0, // didRunInsecureContentForFrame
        0, // didDetectXSSForFrame
        0, // didNewFirstVisuallyNonEmptyLayout
        0, // willGoToBackForwardListItem
        0, // interactionOccurredWhileProcessUnresponsive
        0, // pluginDidFail_deprecatedForUseWithV1
        0, // didReceiveIntentForFrame
        0, // registerIntentServiceForFrame
        0, // didLayout
        0, // pluginLoadPolicy_deprecatedForUseWithV2
        0, // pluginDidFail
        pluginLoadPolicy, // pluginLoadPolicy
    };
    WKPageSetPageLoaderClient(m_mainWebView->page(), &pageLoaderClient);

    WKPagePolicyClient pagePolicyClient = {
        kWKPagePolicyClientCurrentVersion,
        this,
        decidePolicyForNavigationAction,
        0, // decidePolicyForNewWindowAction
        decidePolicyForResponse,
        0, // unableToImplementPolicy
    };
    WKPageSetPagePolicyClient(m_mainWebView->page(), &pagePolicyClient);

    m_mainWebView->didInitializeClients();
}

void TestController::ensureViewSupportsOptions(WKDictionaryRef options)
{
    if (m_mainWebView && !m_mainWebView->viewSupportsOptions(options)) {
        WKPageSetPageUIClient(m_mainWebView->page(), 0);
        WKPageSetPageLoaderClient(m_mainWebView->page(), 0);
        WKPageSetPagePolicyClient(m_mainWebView->page(), 0);
        WKPageClose(m_mainWebView->page());
        
        m_mainWebView = nullptr;

        createWebViewWithOptions(options);
        resetStateToConsistentValues();
    }
}

bool TestController::resetStateToConsistentValues()
{
    m_state = Resetting;

    m_beforeUnloadReturnValue = true;

    WKRetainPtr<WKStringRef> messageName = adoptWK(WKStringCreateWithUTF8CString("Reset"));
    WKRetainPtr<WKMutableDictionaryRef> resetMessageBody = adoptWK(WKMutableDictionaryCreate());

    WKRetainPtr<WKStringRef> shouldGCKey = adoptWK(WKStringCreateWithUTF8CString("ShouldGC"));
    WKRetainPtr<WKBooleanRef> shouldGCValue = adoptWK(WKBooleanCreate(m_gcBetweenTests));
    WKDictionaryAddItem(resetMessageBody.get(), shouldGCKey.get(), shouldGCValue.get());

    WKContextPostMessageToInjectedBundle(TestController::shared().context(), messageName.get(), resetMessageBody.get());

    WKContextSetShouldUseFontSmoothing(TestController::shared().context(), false);

    WKContextSetCacheModel(TestController::shared().context(), kWKCacheModelDocumentBrowser);

    // FIXME: This function should also ensure that there is only one page open.

    // Reset the EventSender for each test.
#if PLATFORM(MAC) || PLATFORM(QT) || PLATFORM(GTK) || PLATFORM(EFL)
    m_eventSenderProxy = adoptPtr(new EventSenderProxy(this));
#endif

    // Reset preferences
    WKPreferencesRef preferences = WKPageGroupGetPreferences(m_pageGroup.get());
    WKPreferencesResetTestRunnerOverrides(preferences);
    WKPreferencesSetOfflineWebApplicationCacheEnabled(preferences, true);
    WKPreferencesSetFontSmoothingLevel(preferences, kWKFontSmoothingLevelNoSubpixelAntiAliasing);
    WKPreferencesSetXSSAuditorEnabled(preferences, false);
    WKPreferencesSetWebAudioEnabled(preferences, true);
    WKPreferencesSetDeveloperExtrasEnabled(preferences, true);
    WKPreferencesSetJavaScriptExperimentsEnabled(preferences, true);
    WKPreferencesSetJavaScriptCanOpenWindowsAutomatically(preferences, true);
    WKPreferencesSetJavaScriptCanAccessClipboard(preferences, true);
    WKPreferencesSetDOMPasteAllowed(preferences, true);
    WKPreferencesSetUniversalAccessFromFileURLsAllowed(preferences, true);
    WKPreferencesSetFileAccessFromFileURLsAllowed(preferences, true);
#if ENABLE(FULLSCREEN_API)
    WKPreferencesSetFullScreenEnabled(preferences, true);
#endif
    WKPreferencesSetPageCacheEnabled(preferences, false);
    WKPreferencesSetAsynchronousPluginInitializationEnabled(preferences, false);
    WKPreferencesSetAsynchronousPluginInitializationEnabledForAllPlugins(preferences, false);
    WKPreferencesSetArtificialPluginInitializationDelayEnabled(preferences, false);
    WKPreferencesSetTabToLinksEnabled(preferences, false);
    WKPreferencesSetInteractiveFormValidationEnabled(preferences, true);
    WKPreferencesSetMockScrollbarsEnabled(preferences, true);

#if !PLATFORM(QT)
    static WKStringRef standardFontFamily = WKStringCreateWithUTF8CString("Times");
    static WKStringRef cursiveFontFamily = WKStringCreateWithUTF8CString("Apple Chancery");
    static WKStringRef fantasyFontFamily = WKStringCreateWithUTF8CString("Papyrus");
    static WKStringRef fixedFontFamily = WKStringCreateWithUTF8CString("Courier");
    static WKStringRef pictographFontFamily = WKStringCreateWithUTF8CString("Apple Color Emoji");
    static WKStringRef sansSerifFontFamily = WKStringCreateWithUTF8CString("Helvetica");
    static WKStringRef serifFontFamily = WKStringCreateWithUTF8CString("Times");

    WKPreferencesSetStandardFontFamily(preferences, standardFontFamily);
    WKPreferencesSetCursiveFontFamily(preferences, cursiveFontFamily);
    WKPreferencesSetFantasyFontFamily(preferences, fantasyFontFamily);
    WKPreferencesSetFixedFontFamily(preferences, fixedFontFamily);
    WKPreferencesSetPictographFontFamily(preferences, pictographFontFamily);
    WKPreferencesSetSansSerifFontFamily(preferences, sansSerifFontFamily);
    WKPreferencesSetSerifFontFamily(preferences, serifFontFamily);
#endif
    WKPreferencesSetScreenFontSubstitutionEnabled(preferences, true);
    WKPreferencesSetInspectorUsesWebKitUserInterface(preferences, true);
    WKPreferencesSetAsynchronousSpellCheckingEnabled(preferences, false);
#if !PLATFORM(MAC)
    WKTextCheckerContinuousSpellCheckingEnabledStateChanged(true);
#endif

    // in the case that a test using the chrome input field failed, be sure to clean up for the next test
    m_mainWebView->removeChromeInputField();
    m_mainWebView->focus();

    // Re-set to the default backing scale factor by setting the custom scale factor to 0.
    WKPageSetCustomBackingScaleFactor(m_mainWebView->page(), 0);

#if PLATFORM(EFL)
    // EFL use a real window while other ports such as Qt don't.
    // In EFL, we need to resize the window to the original size after calls to window.resizeTo.
    WKRect rect = m_mainWebView->windowFrame();
    m_mainWebView->setWindowFrame(WKRectMake(rect.origin.x, rect.origin.y, TestController::viewWidth, TestController::viewHeight));
#endif

    // Reset notification permissions
    m_webNotificationProvider.reset();

    // Reset Geolocation permissions.
    m_geolocationPermissionRequests.clear();
    m_isGeolocationPermissionSet = false;
    m_isGeolocationPermissionAllowed = false;

    // Reset Custom Policy Delegate.
    setCustomPolicyDelegate(false, false);

    m_workQueueManager.clearWorkQueue();

    m_handlesAuthenticationChallenges = false;
    m_authenticationUsername = String();
    m_authenticationPassword = String();

    m_shouldBlockAllPlugins = false;

    // Reset main page back to about:blank
    m_doneResetting = false;

    WKPageLoadURL(m_mainWebView->page(), blankURL());
    runUntil(m_doneResetting, ShortTimeout);
    return m_doneResetting;
}

struct TestCommand {
    TestCommand() : shouldDumpPixels(false), timeout(0) { }

    std::string pathOrURL;
    bool shouldDumpPixels;
    std::string expectedPixelHash;
    int timeout;
};

class CommandTokenizer {
public:
    explicit CommandTokenizer(const std::string& input)
        : m_input(input)
        , m_posNextSeparator(0)
    {
        pump();
    }

    bool hasNext() const;
    std::string next();

private:
    void pump();
    static const char kSeparator = '\'';
    const std::string& m_input;
    std::string m_next;
    size_t m_posNextSeparator;
};

void CommandTokenizer::pump()
{
    if (m_posNextSeparator == std::string::npos || m_posNextSeparator == m_input.size()) {
        m_next = std::string();
        return;
    }
    size_t start = m_posNextSeparator ? m_posNextSeparator + 1 : 0;
    m_posNextSeparator = m_input.find(kSeparator, start);
    size_t size = m_posNextSeparator == std::string::npos ? std::string::npos : m_posNextSeparator - start;
    m_next = std::string(m_input, start, size);
}

std::string CommandTokenizer::next()
{
    ASSERT(hasNext());

    std::string oldNext = m_next;
    pump();
    return oldNext;
}

bool CommandTokenizer::hasNext() const
{
    return !m_next.empty();
}

NO_RETURN static void die(const std::string& inputLine)
{
    fprintf(stderr, "Unexpected input line: %s\n", inputLine.c_str());
    exit(1);
}

TestCommand parseInputLine(const std::string& inputLine)
{
    TestCommand result;
    CommandTokenizer tokenizer(inputLine);
    if (!tokenizer.hasNext())
        die(inputLine);

    std::string arg = tokenizer.next();
    result.pathOrURL = arg;
    while (tokenizer.hasNext()) {
        arg = tokenizer.next();
        if (arg == std::string("--timeout")) {
            std::string timeoutToken = tokenizer.next();
            result.timeout = atoi(timeoutToken.c_str());
        } else if (arg == std::string("-p") || arg == std::string("--pixel-test")) {
            result.shouldDumpPixels = true;
            if (tokenizer.hasNext())
                result.expectedPixelHash = tokenizer.next();
        } else
            die(inputLine);
    }
    return result;
}

bool TestController::runTest(const char* inputLine)
{
    TestCommand command = parseInputLine(std::string(inputLine));

    m_state = RunningTest;

    m_currentInvocation = adoptPtr(new TestInvocation(command.pathOrURL));
    if (command.shouldDumpPixels || m_shouldDumpPixelsForAllTests)
        m_currentInvocation->setIsPixelTest(command.expectedPixelHash);
    if (command.timeout > 0)
        m_currentInvocation->setCustomTimeout(command.timeout);

    m_currentInvocation->invoke();
    m_currentInvocation.clear();

    return true;
}

void TestController::runTestingServerLoop()
{
    char filenameBuffer[2048];
    while (fgets(filenameBuffer, sizeof(filenameBuffer), stdin)) {
        char* newLineCharacter = strchr(filenameBuffer, '\n');
        if (newLineCharacter)
            *newLineCharacter = '\0';

        if (strlen(filenameBuffer) == 0)
            continue;

        if (!runTest(filenameBuffer))
            break;
    }
}

void TestController::run()
{
    if (!resetStateToConsistentValues()) {
        TestInvocation::dumpWebProcessUnresponsiveness("<unknown> - TestController::run - Failed to reset state to consistent values\n");
        return;
    }

    if (m_usingServerMode)
        runTestingServerLoop();
    else {
        for (size_t i = 0; i < m_paths.size(); ++i) {
            if (!runTest(m_paths[i].c_str()))
                break;
        }
    }
}

void TestController::runUntil(bool& done, TimeoutDuration timeoutDuration)
{
    double timeout = m_noTimeout;
    if (!m_forceNoTimeout) {
        switch (timeoutDuration) {
        case ShortTimeout:
            timeout = m_shortTimeout;
            break;
        case LongTimeout:
            timeout = m_longTimeout;
            break;
        case CustomTimeout:
            timeout = m_timeout;
            break;
        case NoTimeout:
        default:
            timeout = m_noTimeout;
            break;
        }
    }

    platformRunUntil(done, timeout);
}

// WKContextInjectedBundleClient

void TestController::didReceiveMessageFromInjectedBundle(WKContextRef context, WKStringRef messageName, WKTypeRef messageBody, const void* clientInfo)
{
    static_cast<TestController*>(const_cast<void*>(clientInfo))->didReceiveMessageFromInjectedBundle(messageName, messageBody);
}

void TestController::didReceiveSynchronousMessageFromInjectedBundle(WKContextRef context, WKStringRef messageName, WKTypeRef messageBody, WKTypeRef* returnData, const void* clientInfo)
{
    *returnData = static_cast<TestController*>(const_cast<void*>(clientInfo))->didReceiveSynchronousMessageFromInjectedBundle(messageName, messageBody).leakRef();
}

void TestController::didReceiveKeyDownMessageFromInjectedBundle(WKDictionaryRef messageBodyDictionary, bool synchronous)
{
    WKRetainPtr<WKStringRef> keyKey = adoptWK(WKStringCreateWithUTF8CString("Key"));
    WKStringRef key = static_cast<WKStringRef>(WKDictionaryGetItemForKey(messageBodyDictionary, keyKey.get()));

    WKRetainPtr<WKStringRef> modifiersKey = adoptWK(WKStringCreateWithUTF8CString("Modifiers"));
    WKEventModifiers modifiers = static_cast<WKEventModifiers>(WKUInt64GetValue(static_cast<WKUInt64Ref>(WKDictionaryGetItemForKey(messageBodyDictionary, modifiersKey.get()))));

    WKRetainPtr<WKStringRef> locationKey = adoptWK(WKStringCreateWithUTF8CString("Location"));
    unsigned location = static_cast<unsigned>(WKUInt64GetValue(static_cast<WKUInt64Ref>(WKDictionaryGetItemForKey(messageBodyDictionary, locationKey.get()))));

    if (synchronous)
        WKPageSetShouldSendEventsSynchronously(mainWebView()->page(), true);

    m_eventSenderProxy->keyDown(key, modifiers, location);

    if (synchronous)
        WKPageSetShouldSendEventsSynchronously(mainWebView()->page(), false);
}

void TestController::didReceiveMessageFromInjectedBundle(WKStringRef messageName, WKTypeRef messageBody)
{
#if PLATFORM(MAC) || PLATFORM(QT) || PLATFORM(GTK) || PLATFORM(EFL)
    if (WKStringIsEqualToUTF8CString(messageName, "EventSender")) {
        ASSERT(WKGetTypeID(messageBody) == WKDictionaryGetTypeID());
        WKDictionaryRef messageBodyDictionary = static_cast<WKDictionaryRef>(messageBody);

        WKRetainPtr<WKStringRef> subMessageKey(AdoptWK, WKStringCreateWithUTF8CString("SubMessage"));
        WKStringRef subMessageName = static_cast<WKStringRef>(WKDictionaryGetItemForKey(messageBodyDictionary, subMessageKey.get()));

        if (WKStringIsEqualToUTF8CString(subMessageName, "MouseDown") || WKStringIsEqualToUTF8CString(subMessageName, "MouseUp")) {
            WKRetainPtr<WKStringRef> buttonKey = adoptWK(WKStringCreateWithUTF8CString("Button"));
            unsigned button = static_cast<unsigned>(WKUInt64GetValue(static_cast<WKUInt64Ref>(WKDictionaryGetItemForKey(messageBodyDictionary, buttonKey.get()))));

            WKRetainPtr<WKStringRef> modifiersKey = adoptWK(WKStringCreateWithUTF8CString("Modifiers"));
            WKEventModifiers modifiers = static_cast<WKEventModifiers>(WKUInt64GetValue(static_cast<WKUInt64Ref>(WKDictionaryGetItemForKey(messageBodyDictionary, modifiersKey.get()))));

            // Forward to WebProcess
            WKPageSetShouldSendEventsSynchronously(mainWebView()->page(), false);
            if (WKStringIsEqualToUTF8CString(subMessageName, "MouseDown"))
                m_eventSenderProxy->mouseDown(button, modifiers);
            else
                m_eventSenderProxy->mouseUp(button, modifiers);

            return;
        }

        if (WKStringIsEqualToUTF8CString(subMessageName, "KeyDown")) {
            didReceiveKeyDownMessageFromInjectedBundle(messageBodyDictionary, false);

            return;
        }

        ASSERT_NOT_REACHED();
    }
#endif

    if (!m_currentInvocation)
        return;

    m_currentInvocation->didReceiveMessageFromInjectedBundle(messageName, messageBody);
}

WKRetainPtr<WKTypeRef> TestController::didReceiveSynchronousMessageFromInjectedBundle(WKStringRef messageName, WKTypeRef messageBody)
{
#if PLATFORM(MAC) || PLATFORM(QT) || PLATFORM(GTK) || PLATFORM(EFL)
    if (WKStringIsEqualToUTF8CString(messageName, "EventSender")) {
        ASSERT(WKGetTypeID(messageBody) == WKDictionaryGetTypeID());
        WKDictionaryRef messageBodyDictionary = static_cast<WKDictionaryRef>(messageBody);

        WKRetainPtr<WKStringRef> subMessageKey(AdoptWK, WKStringCreateWithUTF8CString("SubMessage"));
        WKStringRef subMessageName = static_cast<WKStringRef>(WKDictionaryGetItemForKey(messageBodyDictionary, subMessageKey.get()));

        if (WKStringIsEqualToUTF8CString(subMessageName, "KeyDown")) {
            didReceiveKeyDownMessageFromInjectedBundle(messageBodyDictionary, true);

            return 0;
        }

        if (WKStringIsEqualToUTF8CString(subMessageName, "MouseDown") || WKStringIsEqualToUTF8CString(subMessageName, "MouseUp")) {
            WKRetainPtr<WKStringRef> buttonKey = adoptWK(WKStringCreateWithUTF8CString("Button"));
            unsigned button = static_cast<unsigned>(WKUInt64GetValue(static_cast<WKUInt64Ref>(WKDictionaryGetItemForKey(messageBodyDictionary, buttonKey.get()))));

            WKRetainPtr<WKStringRef> modifiersKey = adoptWK(WKStringCreateWithUTF8CString("Modifiers"));
            WKEventModifiers modifiers = static_cast<WKEventModifiers>(WKUInt64GetValue(static_cast<WKUInt64Ref>(WKDictionaryGetItemForKey(messageBodyDictionary, modifiersKey.get()))));

            // Forward to WebProcess
            WKPageSetShouldSendEventsSynchronously(mainWebView()->page(), true);
            if (WKStringIsEqualToUTF8CString(subMessageName, "MouseDown"))
                m_eventSenderProxy->mouseDown(button, modifiers);
            else
                m_eventSenderProxy->mouseUp(button, modifiers);
            WKPageSetShouldSendEventsSynchronously(mainWebView()->page(), false);
            return 0;
        }

        if (WKStringIsEqualToUTF8CString(subMessageName, "MouseMoveTo")) {
            WKRetainPtr<WKStringRef> xKey = adoptWK(WKStringCreateWithUTF8CString("X"));
            double x = WKDoubleGetValue(static_cast<WKDoubleRef>(WKDictionaryGetItemForKey(messageBodyDictionary, xKey.get())));

            WKRetainPtr<WKStringRef> yKey = adoptWK(WKStringCreateWithUTF8CString("Y"));
            double y = WKDoubleGetValue(static_cast<WKDoubleRef>(WKDictionaryGetItemForKey(messageBodyDictionary, yKey.get())));

            // Forward to WebProcess
            WKPageSetShouldSendEventsSynchronously(mainWebView()->page(), true);
            m_eventSenderProxy->mouseMoveTo(x, y);
            WKPageSetShouldSendEventsSynchronously(mainWebView()->page(), false);
            return 0;
        }

        if (WKStringIsEqualToUTF8CString(subMessageName, "MouseScrollBy")) {
            WKRetainPtr<WKStringRef> xKey = adoptWK(WKStringCreateWithUTF8CString("X"));
            double x = WKDoubleGetValue(static_cast<WKDoubleRef>(WKDictionaryGetItemForKey(messageBodyDictionary, xKey.get())));

            WKRetainPtr<WKStringRef> yKey = adoptWK(WKStringCreateWithUTF8CString("Y"));
            double y = WKDoubleGetValue(static_cast<WKDoubleRef>(WKDictionaryGetItemForKey(messageBodyDictionary, yKey.get())));

            // Forward to WebProcess
            WKPageSetShouldSendEventsSynchronously(mainWebView()->page(), true);
            m_eventSenderProxy->mouseScrollBy(x, y);
            WKPageSetShouldSendEventsSynchronously(mainWebView()->page(), false);
            return 0;
        }

        if (WKStringIsEqualToUTF8CString(subMessageName, "ContinuousMouseScrollBy")) {
            WKRetainPtr<WKStringRef> xKey = adoptWK(WKStringCreateWithUTF8CString("X"));
            double x = WKDoubleGetValue(static_cast<WKDoubleRef>(WKDictionaryGetItemForKey(messageBodyDictionary, xKey.get())));

            WKRetainPtr<WKStringRef> yKey = adoptWK(WKStringCreateWithUTF8CString("Y"));
            double y = WKDoubleGetValue(static_cast<WKDoubleRef>(WKDictionaryGetItemForKey(messageBodyDictionary, yKey.get())));

            WKRetainPtr<WKStringRef> pagedKey = adoptWK(WKStringCreateWithUTF8CString("Paged"));
            bool paged = static_cast<bool>(WKUInt64GetValue(static_cast<WKUInt64Ref>(WKDictionaryGetItemForKey(messageBodyDictionary, pagedKey.get()))));

            // Forward to WebProcess
            WKPageSetShouldSendEventsSynchronously(mainWebView()->page(), true);
            m_eventSenderProxy->continuousMouseScrollBy(x, y, paged);
            WKPageSetShouldSendEventsSynchronously(mainWebView()->page(), false);
            return 0;
        }

        if (WKStringIsEqualToUTF8CString(subMessageName, "LeapForward")) {
            WKRetainPtr<WKStringRef> timeKey = adoptWK(WKStringCreateWithUTF8CString("TimeInMilliseconds"));
            unsigned time = static_cast<unsigned>(WKUInt64GetValue(static_cast<WKUInt64Ref>(WKDictionaryGetItemForKey(messageBodyDictionary, timeKey.get()))));

            m_eventSenderProxy->leapForward(time);
            return 0;
        }

#if ENABLE(TOUCH_EVENTS)
        if (WKStringIsEqualToUTF8CString(subMessageName, "AddTouchPoint")) {
            WKRetainPtr<WKStringRef> xKey = adoptWK(WKStringCreateWithUTF8CString("X"));
            int x = static_cast<int>(WKUInt64GetValue(static_cast<WKUInt64Ref>(WKDictionaryGetItemForKey(messageBodyDictionary, xKey.get()))));

            WKRetainPtr<WKStringRef> yKey = adoptWK(WKStringCreateWithUTF8CString("Y"));
            int y = static_cast<int>(WKUInt64GetValue(static_cast<WKUInt64Ref>(WKDictionaryGetItemForKey(messageBodyDictionary, yKey.get()))));

            m_eventSenderProxy->addTouchPoint(x, y);
            return 0;
        }

        if (WKStringIsEqualToUTF8CString(subMessageName, "UpdateTouchPoint")) {
            WKRetainPtr<WKStringRef> indexKey = adoptWK(WKStringCreateWithUTF8CString("Index"));
            int index = static_cast<int>(WKUInt64GetValue(static_cast<WKUInt64Ref>(WKDictionaryGetItemForKey(messageBodyDictionary, indexKey.get()))));

            WKRetainPtr<WKStringRef> xKey = adoptWK(WKStringCreateWithUTF8CString("X"));
            int x = static_cast<int>(WKUInt64GetValue(static_cast<WKUInt64Ref>(WKDictionaryGetItemForKey(messageBodyDictionary, xKey.get()))));

            WKRetainPtr<WKStringRef> yKey = adoptWK(WKStringCreateWithUTF8CString("Y"));
            int y = static_cast<int>(WKUInt64GetValue(static_cast<WKUInt64Ref>(WKDictionaryGetItemForKey(messageBodyDictionary, yKey.get()))));

            m_eventSenderProxy->updateTouchPoint(index, x, y);
            return 0;
        }

        if (WKStringIsEqualToUTF8CString(subMessageName, "SetTouchModifier")) {
            WKRetainPtr<WKStringRef> modifierKey = adoptWK(WKStringCreateWithUTF8CString("Modifier"));
            WKEventModifiers modifier = static_cast<WKEventModifiers>(WKUInt64GetValue(static_cast<WKUInt64Ref>(WKDictionaryGetItemForKey(messageBodyDictionary, modifierKey.get()))));

            WKRetainPtr<WKStringRef> enableKey = adoptWK(WKStringCreateWithUTF8CString("Enable"));
            bool enable = static_cast<bool>(WKUInt64GetValue(static_cast<WKUInt64Ref>(WKDictionaryGetItemForKey(messageBodyDictionary, enableKey.get()))));

            m_eventSenderProxy->setTouchModifier(modifier, enable);
            return 0;
        }

        if (WKStringIsEqualToUTF8CString(subMessageName, "SetTouchPointRadius")) {
            WKRetainPtr<WKStringRef> xKey = adoptWK(WKStringCreateWithUTF8CString("RadiusX"));
            int x = static_cast<int>(WKUInt64GetValue(static_cast<WKUInt64Ref>(WKDictionaryGetItemForKey(messageBodyDictionary, xKey.get()))));

            WKRetainPtr<WKStringRef> yKey = adoptWK(WKStringCreateWithUTF8CString("RadiusY"));
            int y = static_cast<int>(WKUInt64GetValue(static_cast<WKUInt64Ref>(WKDictionaryGetItemForKey(messageBodyDictionary, yKey.get()))));

            m_eventSenderProxy->setTouchPointRadius(x, y);
            return 0;
        }

        if (WKStringIsEqualToUTF8CString(subMessageName, "TouchStart")) {
            WKPageSetShouldSendEventsSynchronously(mainWebView()->page(), true);
            m_eventSenderProxy->touchStart();
            WKPageSetShouldSendEventsSynchronously(mainWebView()->page(), false);
            return 0;
        }

        if (WKStringIsEqualToUTF8CString(subMessageName, "TouchMove")) {
            WKPageSetShouldSendEventsSynchronously(mainWebView()->page(), true);
            m_eventSenderProxy->touchMove();
            WKPageSetShouldSendEventsSynchronously(mainWebView()->page(), false);
            return 0;
        }

        if (WKStringIsEqualToUTF8CString(subMessageName, "TouchEnd")) {
            WKPageSetShouldSendEventsSynchronously(mainWebView()->page(), true);
            m_eventSenderProxy->touchEnd();
            WKPageSetShouldSendEventsSynchronously(mainWebView()->page(), false);
            return 0;
        }

        if (WKStringIsEqualToUTF8CString(subMessageName, "TouchCancel")) {
            WKPageSetShouldSendEventsSynchronously(mainWebView()->page(), true);
            m_eventSenderProxy->touchCancel();
            WKPageSetShouldSendEventsSynchronously(mainWebView()->page(), false);
            return 0;
        }

        if (WKStringIsEqualToUTF8CString(subMessageName, "ClearTouchPoints")) {
            m_eventSenderProxy->clearTouchPoints();
            return 0;
        }

        if (WKStringIsEqualToUTF8CString(subMessageName, "ReleaseTouchPoint")) {
            WKRetainPtr<WKStringRef> indexKey = adoptWK(WKStringCreateWithUTF8CString("Index"));
            int index = static_cast<int>(WKUInt64GetValue(static_cast<WKUInt64Ref>(WKDictionaryGetItemForKey(messageBodyDictionary, indexKey.get()))));
            m_eventSenderProxy->releaseTouchPoint(index);
            return 0;
        }

        if (WKStringIsEqualToUTF8CString(subMessageName, "CancelTouchPoint")) {
            WKRetainPtr<WKStringRef> indexKey = adoptWK(WKStringCreateWithUTF8CString("Index"));
            int index = static_cast<int>(WKUInt64GetValue(static_cast<WKUInt64Ref>(WKDictionaryGetItemForKey(messageBodyDictionary, indexKey.get()))));
            m_eventSenderProxy->cancelTouchPoint(index);
            return 0;
        }
#endif
        ASSERT_NOT_REACHED();
    }
#endif
    return m_currentInvocation->didReceiveSynchronousMessageFromInjectedBundle(messageName, messageBody);
}

// WKPageLoaderClient

void TestController::didCommitLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef, const void* clientInfo)
{
    static_cast<TestController*>(const_cast<void*>(clientInfo))->didCommitLoadForFrame(page, frame);
}

void TestController::didFinishLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef, const void* clientInfo)
{
    static_cast<TestController*>(const_cast<void*>(clientInfo))->didFinishLoadForFrame(page, frame);
}

void TestController::didReceiveAuthenticationChallengeInFrame(WKPageRef page, WKFrameRef frame, WKAuthenticationChallengeRef authenticationChallenge, const void *clientInfo)
{
    static_cast<TestController*>(const_cast<void*>(clientInfo))->didReceiveAuthenticationChallengeInFrame(page, frame, authenticationChallenge);
}

void TestController::processDidCrash(WKPageRef page, const void* clientInfo)
{
    static_cast<TestController*>(const_cast<void*>(clientInfo))->processDidCrash();
}

WKPluginLoadPolicy TestController::pluginLoadPolicy(WKPageRef page, WKPluginLoadPolicy currentPluginLoadPolicy, WKDictionaryRef pluginInformation, WKStringRef* unavailabilityDescription, const void* clientInfo)
{
    return static_cast<TestController*>(const_cast<void*>(clientInfo))->pluginLoadPolicy(page, currentPluginLoadPolicy, pluginInformation, unavailabilityDescription);
}

WKPluginLoadPolicy TestController::pluginLoadPolicy(WKPageRef, WKPluginLoadPolicy currentPluginLoadPolicy, WKDictionaryRef pluginInformation, WKStringRef* unavailabilityDescription)
{
    if (m_shouldBlockAllPlugins)
        return kWKPluginLoadPolicyBlocked;
    return currentPluginLoadPolicy;
}

void TestController::didCommitLoadForFrame(WKPageRef page, WKFrameRef frame)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    mainWebView()->focus();
}

void TestController::didFinishLoadForFrame(WKPageRef page, WKFrameRef frame)
{
    if (m_state != Resetting)
        return;

    if (!WKFrameIsMainFrame(frame))
        return;

    WKRetainPtr<WKURLRef> wkURL(AdoptWK, WKFrameCopyURL(frame));
    if (!WKURLIsEqual(wkURL.get(), blankURL()))
        return;

    m_doneResetting = true;
    shared().notifyDone();
}

void TestController::didReceiveAuthenticationChallengeInFrame(WKPageRef page, WKFrameRef frame, WKAuthenticationChallengeRef authenticationChallenge)
{
    String message;
    if (!m_handlesAuthenticationChallenges)
        message = "<unknown> - didReceiveAuthenticationChallenge - Simulating cancelled authentication sheet\n";
    else
        message = String::format("<unknown> - didReceiveAuthenticationChallenge - Responding with %s:%s\n", m_authenticationUsername.utf8().data(), m_authenticationPassword.utf8().data());
    m_currentInvocation->outputText(message);

    WKAuthenticationDecisionListenerRef decisionListener = WKAuthenticationChallengeGetDecisionListener(authenticationChallenge);
    if (!m_handlesAuthenticationChallenges) {
        WKAuthenticationDecisionListenerUseCredential(decisionListener, 0);
        return;
    }
    WKRetainPtr<WKStringRef> username(AdoptWK, WKStringCreateWithUTF8CString(m_authenticationUsername.utf8().data()));
    WKRetainPtr<WKStringRef> password(AdoptWK, WKStringCreateWithUTF8CString(m_authenticationPassword.utf8().data()));
    WKRetainPtr<WKCredentialRef> credential(AdoptWK, WKCredentialCreate(username.get(), password.get(), kWKCredentialPersistenceForSession));
    WKAuthenticationDecisionListenerUseCredential(decisionListener, credential.get());
}

void TestController::processDidCrash()
{
    // This function can be called multiple times when crash logs are being saved on Windows, so
    // ensure we only print the crashed message once.
    if (!m_didPrintWebProcessCrashedMessage) {
#if PLATFORM(MAC)
        pid_t pid = WKPageGetProcessIdentifier(m_mainWebView->page());
        fprintf(stderr, "#CRASHED - WebProcess (pid %ld)\n", static_cast<long>(pid));
#else
        fputs("#CRASHED - WebProcess\n", stderr);
#endif
        fflush(stderr);
        m_didPrintWebProcessCrashedMessage = true;
    }

    if (m_shouldExitWhenWebProcessCrashes)
        exit(1);
}

void TestController::simulateWebNotificationClick(uint64_t notificationID)
{
    m_webNotificationProvider.simulateWebNotificationClick(notificationID);
}

void TestController::setGeolocationPermission(bool enabled)
{
    m_isGeolocationPermissionSet = true;
    m_isGeolocationPermissionAllowed = enabled;
    decidePolicyForGeolocationPermissionRequestIfPossible();
}

void TestController::setMockGeolocationPosition(double latitude, double longitude, double accuracy, bool providesAltitude, double altitude, bool providesAltitudeAccuracy, double altitudeAccuracy, bool providesHeading, double heading, bool providesSpeed, double speed)
{
    m_geolocationProvider->setPosition(latitude, longitude, accuracy, providesAltitude, altitude, providesAltitudeAccuracy, altitudeAccuracy, providesHeading, heading, providesSpeed, speed);
}

void TestController::setMockGeolocationPositionUnavailableError(WKStringRef errorMessage)
{
    m_geolocationProvider->setPositionUnavailableError(errorMessage);
}

void TestController::handleGeolocationPermissionRequest(WKGeolocationPermissionRequestRef geolocationPermissionRequest)
{
    m_geolocationPermissionRequests.append(geolocationPermissionRequest);
    decidePolicyForGeolocationPermissionRequestIfPossible();
}

void TestController::setCustomPolicyDelegate(bool enabled, bool permissive)
{
    m_policyDelegateEnabled = enabled;
    m_policyDelegatePermissive = permissive;
}

void TestController::setVisibilityState(WKPageVisibilityState visibilityState, bool isInitialState)
{
    WKPageSetVisibilityState(m_mainWebView->page(), visibilityState, isInitialState);
}

void TestController::decidePolicyForGeolocationPermissionRequestIfPossible()
{
    if (!m_isGeolocationPermissionSet)
        return;

    for (size_t i = 0; i < m_geolocationPermissionRequests.size(); ++i) {
        WKGeolocationPermissionRequestRef permissionRequest = m_geolocationPermissionRequests[i].get();
        if (m_isGeolocationPermissionAllowed)
            WKGeolocationPermissionRequestAllow(permissionRequest);
        else
            WKGeolocationPermissionRequestDeny(permissionRequest);
    }
    m_geolocationPermissionRequests.clear();
}

void TestController::decidePolicyForNotificationPermissionRequest(WKPageRef page, WKSecurityOriginRef origin, WKNotificationPermissionRequestRef request, const void*)
{
    TestController::shared().decidePolicyForNotificationPermissionRequest(page, origin, request);
}

void TestController::decidePolicyForNotificationPermissionRequest(WKPageRef, WKSecurityOriginRef, WKNotificationPermissionRequestRef request)
{
    WKNotificationPermissionRequestAllow(request);
}

void TestController::unavailablePluginButtonClicked(WKPageRef, WKPluginUnavailabilityReason, WKDictionaryRef, const void*)
{
    printf("MISSING PLUGIN BUTTON PRESSED\n");
}

void TestController::decidePolicyForNavigationAction(WKPageRef, WKFrameRef, WKFrameNavigationType, WKEventModifiers, WKEventMouseButton, WKURLRequestRef, WKFramePolicyListenerRef listener, WKTypeRef, const void* clientInfo)
{
    static_cast<TestController*>(const_cast<void*>(clientInfo))->decidePolicyForNavigationAction(listener);
}

void TestController::decidePolicyForNavigationAction(WKFramePolicyListenerRef listener)
{
    if (m_policyDelegateEnabled && !m_policyDelegatePermissive) {
        WKFramePolicyListenerIgnore(listener);
        return;
    }

    WKFramePolicyListenerUse(listener);
}

void TestController::decidePolicyForResponse(WKPageRef, WKFrameRef frame, WKURLResponseRef response, WKURLRequestRef, WKFramePolicyListenerRef listener, WKTypeRef, const void* clientInfo)
{
    static_cast<TestController*>(const_cast<void*>(clientInfo))->decidePolicyForResponse(frame, response, listener);
}

void TestController::decidePolicyForResponse(WKFrameRef frame, WKURLResponseRef response, WKFramePolicyListenerRef listener)
{
    // Even though Response was already checked by WKBundlePagePolicyClient, the check did not include plugins
    // so we have to re-check again.
    WKRetainPtr<WKStringRef> wkMIMEType(AdoptWK, WKURLResponseCopyMIMEType(response));
    if (WKFrameCanShowMIMEType(frame, wkMIMEType.get())) {
        WKFramePolicyListenerUse(listener);
        return;
    }

    WKFramePolicyListenerIgnore(listener);
}

} // namespace WTR
