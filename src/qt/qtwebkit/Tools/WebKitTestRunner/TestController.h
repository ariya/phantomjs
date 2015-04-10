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

#ifndef TestController_h
#define TestController_h

#include "WebNotificationProvider.h"
#include "WorkQueueManager.h"
#include <GeolocationProviderMock.h>
#include <WebKit2/WKRetainPtr.h>
#include <string>
#include <vector>
#include <wtf/OwnPtr.h>
#include <wtf/Vector.h>

namespace WTR {

class TestInvocation;
class PlatformWebView;
class EventSenderProxy;

// FIXME: Rename this TestRunner?
class TestController {
public:
    static TestController& shared();

    static const unsigned viewWidth;
    static const unsigned viewHeight;

    static const unsigned w3cSVGViewWidth;
    static const unsigned w3cSVGViewHeight;

    TestController(int argc, const char* argv[]);
    ~TestController();

    bool verbose() const { return m_verbose; }

    WKStringRef injectedBundlePath() { return m_injectedBundlePath.get(); }
    WKStringRef testPluginDirectory() { return m_testPluginDirectory.get(); }

    PlatformWebView* mainWebView() { return m_mainWebView.get(); }
    WKContextRef context() { return m_context.get(); }

    void ensureViewSupportsOptions(WKDictionaryRef options);
    
    // Runs the run loop until `done` is true or the timeout elapses.
    enum TimeoutDuration { ShortTimeout, LongTimeout, NoTimeout, CustomTimeout };
    bool useWaitToDumpWatchdogTimer() { return m_useWaitToDumpWatchdogTimer; }
    void runUntil(bool& done, TimeoutDuration);
    void notifyDone();

    int getCustomTimeout();
    
    bool beforeUnloadReturnValue() const { return m_beforeUnloadReturnValue; }
    void setBeforeUnloadReturnValue(bool value) { m_beforeUnloadReturnValue = value; }

    void simulateWebNotificationClick(uint64_t notificationID);

    // Geolocation.
    void setGeolocationPermission(bool);
    void setMockGeolocationPosition(double latitude, double longitude, double accuracy, bool providesAltitude, double altitude, bool providesAltitudeAccuracy, double altitudeAccuracy, bool providesHeading, double heading, bool providesSpeed, double speed);
    void setMockGeolocationPositionUnavailableError(WKStringRef errorMessage);
    void handleGeolocationPermissionRequest(WKGeolocationPermissionRequestRef);

    // Policy delegate.
    void setCustomPolicyDelegate(bool enabled, bool permissive);

    // Page Visibility.
    void setVisibilityState(WKPageVisibilityState, bool isInitialState);

    bool resetStateToConsistentValues();

    WorkQueueManager& workQueueManager() { return m_workQueueManager; }

    void setHandlesAuthenticationChallenges(bool value) { m_handlesAuthenticationChallenges = value; }
    void setAuthenticationUsername(String username) { m_authenticationUsername = username; }
    void setAuthenticationPassword(String password) { m_authenticationPassword = password; }

    void setBlockAllPlugins(bool shouldBlock) { m_shouldBlockAllPlugins = shouldBlock; }

private:
    void initialize(int argc, const char* argv[]);
    void createWebViewWithOptions(WKDictionaryRef);
    void run();

    void runTestingServerLoop();
    bool runTest(const char* pathOrURL);

    void platformInitialize();
    void platformDestroy();
    void platformInitializeContext();
    void platformRunUntil(bool& done, double timeout);
    void platformDidCommitLoadForFrame(WKPageRef, WKFrameRef);
    void initializeInjectedBundlePath();
    void initializeTestPluginDirectory();

    void decidePolicyForGeolocationPermissionRequestIfPossible();

    // WKContextInjectedBundleClient
    static void didReceiveMessageFromInjectedBundle(WKContextRef, WKStringRef messageName, WKTypeRef messageBody, const void*);
    static void didReceiveSynchronousMessageFromInjectedBundle(WKContextRef, WKStringRef messageName, WKTypeRef messageBody, WKTypeRef* returnData, const void*);
    void didReceiveMessageFromInjectedBundle(WKStringRef messageName, WKTypeRef messageBody);
    WKRetainPtr<WKTypeRef> didReceiveSynchronousMessageFromInjectedBundle(WKStringRef messageName, WKTypeRef messageBody);

    void didReceiveKeyDownMessageFromInjectedBundle(WKDictionaryRef messageBodyDictionary, bool synchronous);

    // WKPageLoaderClient
    static void didCommitLoadForFrame(WKPageRef, WKFrameRef, WKTypeRef userData, const void*);
    void didCommitLoadForFrame(WKPageRef, WKFrameRef);

    static void didFinishLoadForFrame(WKPageRef, WKFrameRef, WKTypeRef userData, const void*);
    void didFinishLoadForFrame(WKPageRef, WKFrameRef);

    static void processDidCrash(WKPageRef, const void* clientInfo);
    void processDidCrash();

    static WKPluginLoadPolicy pluginLoadPolicy(WKPageRef, WKPluginLoadPolicy currentPluginLoadPolicy, WKDictionaryRef pluginInformation, WKStringRef* unavailabilityDescription, const void* clientInfo);
    WKPluginLoadPolicy pluginLoadPolicy(WKPageRef, WKPluginLoadPolicy currentPluginLoadPolicy, WKDictionaryRef pluginInformation, WKStringRef* unavailabilityDescription);
    

    static void decidePolicyForNotificationPermissionRequest(WKPageRef, WKSecurityOriginRef, WKNotificationPermissionRequestRef, const void*);
    void decidePolicyForNotificationPermissionRequest(WKPageRef, WKSecurityOriginRef, WKNotificationPermissionRequestRef);

    static void unavailablePluginButtonClicked(WKPageRef, WKPluginUnavailabilityReason, WKDictionaryRef, const void*);

    static void didReceiveAuthenticationChallengeInFrame(WKPageRef, WKFrameRef, WKAuthenticationChallengeRef, const void *clientInfo);
    void didReceiveAuthenticationChallengeInFrame(WKPageRef, WKFrameRef, WKAuthenticationChallengeRef);

    // WKPagePolicyClient
    static void decidePolicyForNavigationAction(WKPageRef, WKFrameRef, WKFrameNavigationType, WKEventModifiers, WKEventMouseButton, WKURLRequestRef, WKFramePolicyListenerRef, WKTypeRef, const void*);
    void decidePolicyForNavigationAction(WKFramePolicyListenerRef);

    static void decidePolicyForResponse(WKPageRef, WKFrameRef, WKURLResponseRef, WKURLRequestRef, WKFramePolicyListenerRef, WKTypeRef, const void*);
    void decidePolicyForResponse(WKFrameRef, WKURLResponseRef, WKFramePolicyListenerRef);

    static WKPageRef createOtherPage(WKPageRef oldPage, WKURLRequestRef, WKDictionaryRef, WKEventModifiers, WKEventMouseButton, const void*);

    static void runModal(WKPageRef, const void* clientInfo);
    static void runModal(PlatformWebView*);

    static const char* libraryPathForTesting();
    static const char* platformLibraryPathForTesting();

    OwnPtr<TestInvocation> m_currentInvocation;

    bool m_verbose;
    bool m_printSeparators;
    bool m_usingServerMode;
    bool m_gcBetweenTests;
    bool m_shouldDumpPixelsForAllTests;
    std::vector<std::string> m_paths;
    WKRetainPtr<WKStringRef> m_injectedBundlePath;
    WKRetainPtr<WKStringRef> m_testPluginDirectory;

    WebNotificationProvider m_webNotificationProvider;

    OwnPtr<PlatformWebView> m_mainWebView;
    WKRetainPtr<WKContextRef> m_context;
    WKRetainPtr<WKPageGroupRef> m_pageGroup;

    enum State {
        Initial,
        Resetting,
        RunningTest
    };
    State m_state;
    bool m_doneResetting;

    double m_longTimeout;
    double m_shortTimeout;
    double m_noTimeout;
    bool m_useWaitToDumpWatchdogTimer;
    bool m_forceNoTimeout;

    int m_timeout;

    bool m_didPrintWebProcessCrashedMessage;
    bool m_shouldExitWhenWebProcessCrashes;
    
    bool m_beforeUnloadReturnValue;

    OwnPtr<GeolocationProviderMock> m_geolocationProvider;
    Vector<WKRetainPtr<WKGeolocationPermissionRequestRef> > m_geolocationPermissionRequests;
    bool m_isGeolocationPermissionSet;
    bool m_isGeolocationPermissionAllowed;

    bool m_policyDelegateEnabled;
    bool m_policyDelegatePermissive;

    bool m_handlesAuthenticationChallenges;
    String m_authenticationUsername;
    String m_authenticationPassword;

    bool m_shouldBlockAllPlugins;

#if PLATFORM(MAC) || PLATFORM(QT) || PLATFORM(GTK) || PLATFORM(EFL)
    OwnPtr<EventSenderProxy> m_eventSenderProxy;
#endif

#if PLATFORM(QT)
    class RunLoop;
    RunLoop* m_runLoop;
#endif

    WorkQueueManager m_workQueueManager;
};

} // namespace WTR

#endif // TestController_h
