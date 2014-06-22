/*
 * Copyright (C) 2010, 2011, 2012 Apple Inc. All rights reserved.
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

#ifndef InjectedBundle_h
#define InjectedBundle_h

#include "AccessibilityController.h"
#include "EventSendingController.h"
#include "GCController.h"
#include "TestRunner.h"
#include "TextInputController.h"
#include <WebKit2/WKBase.h>
#include <WebKit2/WKRetainPtr.h>
#include <sstream>
#include <wtf/Forward.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WTR {

class InjectedBundlePage;

class InjectedBundle {
public:
    static InjectedBundle& shared();

    // Initialize the InjectedBundle.
    void initialize(WKBundleRef, WKTypeRef initializationUserData);

    WKBundleRef bundle() const { return m_bundle; }
    WKBundlePageGroupRef pageGroup() const { return m_pageGroup; }

    TestRunner* testRunner() { return m_testRunner.get(); }
    GCController* gcController() { return m_gcController.get(); }
    EventSendingController* eventSendingController() { return m_eventSendingController.get(); }
    TextInputController* textInputController() { return m_textInputController.get(); }
    AccessibilityController* accessibilityController() { return m_accessibilityController.get(); }

    InjectedBundlePage* page() const;
    size_t pageCount() const { return m_pages.size(); }
    void closeOtherPages();

    void dumpBackForwardListsForAllPages(StringBuilder&);

    void done();
    void setAudioResult(WKDataRef audioData) { m_audioResult = audioData; }
    void setPixelResult(WKImageRef image) { m_pixelResult = image; }
    void setRepaintRects(WKArrayRef rects) { m_repaintRects = rects; }

    bool isTestRunning() { return m_state == Testing; }

    WKBundleFrameRef topLoadingFrame() { return m_topLoadingFrame; }
    void setTopLoadingFrame(WKBundleFrameRef frame) { m_topLoadingFrame = frame; }

    bool shouldDumpPixels() const { return m_dumpPixels; }
    bool useWaitToDumpWatchdogTimer() const { return m_useWaitToDumpWatchdogTimer; }
    
    void outputText(const String&);
    void postNewBeforeUnloadReturnValue(bool);
    void postAddChromeInputField();
    void postRemoveChromeInputField();
    void postFocusWebView();
    void postSetBackingScaleFactor(double);
    void postSetWindowIsKey(bool);
    void postSimulateWebNotificationClick(uint64_t notificationID);

    // Geolocation.
    void setGeolocationPermission(bool);
    void setMockGeolocationPosition(double latitude, double longitude, double accuracy, bool providesAltitude, double altitude, bool providesAltitudeAccuracy, double altitudeAccuracy, bool providesHeading, double heading, bool providesSpeed, double speed);
    void setMockGeolocationPositionUnavailableError(WKStringRef errorMessage);

    // Policy delegate.
    void setCustomPolicyDelegate(bool enabled, bool permissive);

    // Page Visibility.
    void setVisibilityState(WKPageVisibilityState, bool isInitialState);

    // Work queue.
    bool shouldProcessWorkQueue() const;
    void processWorkQueue();
    void queueBackNavigation(unsigned howFarBackward);
    void queueForwardNavigation(unsigned howFarForward);
    void queueLoad(WKStringRef url, WKStringRef target);
    void queueLoadHTMLString(WKStringRef content, WKStringRef baseURL = 0, WKStringRef unreachableURL = 0);
    void queueReload();
    void queueLoadingScript(WKStringRef script);
    void queueNonLoadingScript(WKStringRef script);

private:
    InjectedBundle();
    ~InjectedBundle();

    static void didCreatePage(WKBundleRef, WKBundlePageRef, const void* clientInfo);
    static void willDestroyPage(WKBundleRef, WKBundlePageRef, const void* clientInfo);
    static void didInitializePageGroup(WKBundleRef, WKBundlePageGroupRef, const void* clientInfo);
    static void didReceiveMessage(WKBundleRef, WKStringRef messageName, WKTypeRef messageBody, const void* clientInfo);
    static void didReceiveMessageToPage(WKBundleRef, WKBundlePageRef, WKStringRef messageName, WKTypeRef messageBody, const void* clientInfo);

    void didCreatePage(WKBundlePageRef);
    void willDestroyPage(WKBundlePageRef);
    void didInitializePageGroup(WKBundlePageGroupRef);
    void didReceiveMessage(WKStringRef messageName, WKTypeRef messageBody);
    void didReceiveMessageToPage(WKBundlePageRef, WKStringRef messageName, WKTypeRef messageBody);

    void platformInitialize(WKTypeRef initializationUserData);
    void resetLocalSettings();

    void beginTesting(WKDictionaryRef initialSettings);

    bool booleanForKey(WKDictionaryRef, const char* key);

    WKBundleRef m_bundle;
    WKBundlePageGroupRef m_pageGroup;
    Vector<OwnPtr<InjectedBundlePage> > m_pages;

    RefPtr<AccessibilityController> m_accessibilityController;
    RefPtr<TestRunner> m_testRunner;
    RefPtr<GCController> m_gcController;
    RefPtr<EventSendingController> m_eventSendingController;
    RefPtr<TextInputController> m_textInputController;

    WKBundleFrameRef m_topLoadingFrame;

    enum State {
        Idle,
        Testing,
        Stopping
    };
    State m_state;

    bool m_dumpPixels;
    bool m_useWaitToDumpWatchdogTimer;
    bool m_useWorkQueue;
    int m_timeout;

    WKRetainPtr<WKDataRef> m_audioResult;
    WKRetainPtr<WKImageRef> m_pixelResult;
    WKRetainPtr<WKArrayRef> m_repaintRects;
};

} // namespace WTR

#endif // InjectedBundle_h
