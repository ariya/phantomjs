/*
 * Copyright (C) 2007, 2008, 2009, 2012 Apple Inc. All rights reserved.
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
 
#ifndef TestRunner_h
#define TestRunner_h

#include <JavaScriptCore/JSObjectRef.h>
#include <JavaScriptCore/JSRetainPtr.h>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

class TestRunner : public RefCounted<TestRunner> {
public:
    static PassRefPtr<TestRunner> create(const std::string& testPathOrURL, const std::string& expectedPixelHash);

    static const unsigned viewWidth;
    static const unsigned viewHeight;

    static const unsigned w3cSVGViewWidth;
    static const unsigned w3cSVGViewHeight;

    ~TestRunner();

    void makeWindowObject(JSContextRef, JSObjectRef windowObject, JSValueRef* exception);

    void addDisallowedURL(JSStringRef url);
    void addURLToRedirect(std::string origin, std::string destination);
    const std::string& redirectionDestinationForURL(std::string);
    void clearAllApplicationCaches();
    void clearAllDatabases();
    void clearApplicationCacheForOrigin(JSStringRef name);
    void clearBackForwardList();
    void clearPersistentUserStyleSheet();
    bool callShouldCloseOnWebView();
    JSStringRef copyDecodedHostName(JSStringRef name);
    JSStringRef copyEncodedHostName(JSStringRef name);
    void dispatchPendingLoadRequests();
    void display();
    void displayInvalidatedRegion();
    void execCommand(JSStringRef name, JSStringRef value);
    bool findString(JSContextRef, JSStringRef, JSObjectRef optionsArray);
    void goBack();
    JSValueRef originsWithApplicationCache(JSContextRef);
    long long applicationCacheDiskUsageForOrigin(JSStringRef name);
    bool isCommandEnabled(JSStringRef name);
    void keepWebHistory();
    void notifyDone();
    int numberOfPendingGeolocationPermissionRequests();
    void overridePreference(JSStringRef key, JSStringRef value);
    JSStringRef pathToLocalResource(JSContextRef, JSStringRef url);
    void queueBackNavigation(int howFarBackward);
    void queueForwardNavigation(int howFarForward);
    void queueLoad(JSStringRef url, JSStringRef target);
    void queueLoadHTMLString(JSStringRef content, JSStringRef baseURL);
    void queueLoadAlternateHTMLString(JSStringRef content, JSStringRef baseURL, JSStringRef unreachableURL);
    void queueLoadingScript(JSStringRef script);
    void queueNonLoadingScript(JSStringRef script);
    void queueReload();
    void removeAllVisitedLinks();
    void setAcceptsEditing(bool);
    void setAllowUniversalAccessFromFileURLs(bool);
    void setAllowFileAccessFromFileURLs(bool);
    void setAppCacheMaximumSize(unsigned long long quota);
    void setApplicationCacheOriginQuota(unsigned long long);
    void setAuthorAndUserStylesEnabled(bool);
    void setCacheModel(int);
    void setCustomPolicyDelegate(bool setDelegate, bool permissive);
    void setDatabaseQuota(unsigned long long quota);
    void setDomainRelaxationForbiddenForURLScheme(bool forbidden, JSStringRef scheme);
    void setDefersLoading(bool);
    void setIconDatabaseEnabled(bool);
    void setJavaScriptCanAccessClipboard(bool flag);
    void setAutomaticLinkDetectionEnabled(bool flag);
    void setMainFrameIsFirstResponder(bool flag);
    void setMockDeviceOrientation(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma);
    void setMockGeolocationPosition(double latitude, double longitude, double accuracy, bool providesAltitude, double altitude, bool providesAltitudeAccuracy, double altitudeAccuracy, bool providesHeading, double heading, bool providesSpeed, double speed);
    void setMockGeolocationPositionUnavailableError(JSStringRef message);
    void addMockSpeechInputResult(JSStringRef result, double confidence, JSStringRef language);
    void setMockSpeechInputDumpRect(bool flag);
    void setPersistentUserStyleSheetLocation(JSStringRef path);
    void setPluginsEnabled(bool);
    void setPopupBlockingEnabled(bool);
    void setPrivateBrowsingEnabled(bool);
    void setTabKeyCyclesThroughElements(bool);
    void setUseDashboardCompatibilityMode(bool flag);
    void setUserStyleSheetEnabled(bool flag);
    void setUserStyleSheetLocation(JSStringRef path);
    void setValueForUser(JSContextRef, JSValueRef nodeObject, JSStringRef value);
    void setViewModeMediaFeature(JSStringRef);
    void setXSSAuditorEnabled(bool flag);
    void setSpatialNavigationEnabled(bool);
    void setScrollbarPolicy(JSStringRef orientation, JSStringRef policy);
    void startSpeechInput(JSContextRef inputElement);
    void setPageVisibility(const char*);
    void resetPageVisibility();

    void waitForPolicyDelegate();
    size_t webHistoryItemCount();
    int windowCount();
    
#if PLATFORM(MAC) || PLATFORM(GTK) || PLATFORM(WIN) || PLATFORM(EFL)
    JSRetainPtr<JSStringRef> platformName() const;
#endif

    // Legacy here refers to the old TestRunner API for handling web notifications, not the legacy web notification API.
    void ignoreLegacyWebNotificationPermissionRequests();
    // Legacy here refers to the old TestRunner API for handling web notifications, not the legacy web notification API.
    void simulateLegacyWebNotificationClick(JSStringRef title);
    void grantWebNotificationPermission(JSStringRef origin);
    void denyWebNotificationPermission(JSStringRef origin);
    void removeAllWebNotificationPermissions();
    void simulateWebNotificationClick(JSValueRef notification);

    bool dumpAsAudio() const { return m_dumpAsAudio; }
    void setDumpAsAudio(bool dumpAsAudio) { m_dumpAsAudio = dumpAsAudio; }
    
    bool dumpAsPDF() const { return m_dumpAsPDF; }
    void setDumpAsPDF(bool dumpAsPDF) { m_dumpAsPDF = dumpAsPDF; }

    bool dumpAsText() const { return m_dumpAsText; }
    void setDumpAsText(bool dumpAsText) { m_dumpAsText = dumpAsText; }

    bool generatePixelResults() const { return m_generatePixelResults; }
    void setGeneratePixelResults(bool generatePixelResults) { m_generatePixelResults = generatePixelResults; }

    bool disallowIncreaseForApplicationCacheQuota() const { return m_disallowIncreaseForApplicationCacheQuota; }
    void setDisallowIncreaseForApplicationCacheQuota(bool disallowIncrease) { m_disallowIncreaseForApplicationCacheQuota = disallowIncrease; }

    bool dumpApplicationCacheDelegateCallbacks() const { return m_dumpApplicationCacheDelegateCallbacks; }
    void setDumpApplicationCacheDelegateCallbacks(bool dumpCallbacks) { m_dumpApplicationCacheDelegateCallbacks = dumpCallbacks; }

    bool dumpBackForwardList() const { return m_dumpBackForwardList; }
    void setDumpBackForwardList(bool dumpBackForwardList) { m_dumpBackForwardList = dumpBackForwardList; }

    bool dumpChildFrameScrollPositions() const { return m_dumpChildFrameScrollPositions; }
    void setDumpChildFrameScrollPositions(bool dumpChildFrameScrollPositions) { m_dumpChildFrameScrollPositions = dumpChildFrameScrollPositions; }

    bool dumpChildFramesAsText() const { return m_dumpChildFramesAsText; }
    void setDumpChildFramesAsText(bool dumpChildFramesAsText) { m_dumpChildFramesAsText = dumpChildFramesAsText; }

    bool dumpDatabaseCallbacks() const { return m_dumpDatabaseCallbacks; }
    void setDumpDatabaseCallbacks(bool dumpDatabaseCallbacks) { m_dumpDatabaseCallbacks = dumpDatabaseCallbacks; }

    bool dumpDOMAsWebArchive() const { return m_dumpDOMAsWebArchive; }
    void setDumpDOMAsWebArchive(bool dumpDOMAsWebArchive) { m_dumpDOMAsWebArchive = dumpDOMAsWebArchive; }

    bool dumpEditingCallbacks() const { return m_dumpEditingCallbacks; }
    void setDumpEditingCallbacks(bool dumpEditingCallbacks) { m_dumpEditingCallbacks = dumpEditingCallbacks; }

    bool dumpFrameLoadCallbacks() const { return m_dumpFrameLoadCallbacks; }
    void setDumpFrameLoadCallbacks(bool dumpFrameLoadCallbacks) { m_dumpFrameLoadCallbacks = dumpFrameLoadCallbacks; }

    bool dumpProgressFinishedCallback() const { return m_dumpProgressFinishedCallback; }
    void setDumpProgressFinishedCallback(bool dumpProgressFinishedCallback) { m_dumpProgressFinishedCallback = dumpProgressFinishedCallback; }
    
    bool dumpUserGestureInFrameLoadCallbacks() const { return m_dumpUserGestureInFrameLoadCallbacks; }
    void setDumpUserGestureInFrameLoadCallbacks(bool dumpUserGestureInFrameLoadCallbacks) { m_dumpUserGestureInFrameLoadCallbacks = dumpUserGestureInFrameLoadCallbacks; }    

    bool dumpHistoryDelegateCallbacks() const { return m_dumpHistoryDelegateCallbacks; }
    void setDumpHistoryDelegateCallbacks(bool dumpHistoryDelegateCallbacks) { m_dumpHistoryDelegateCallbacks = dumpHistoryDelegateCallbacks; }
    
    bool dumpResourceLoadCallbacks() const { return m_dumpResourceLoadCallbacks; }
    void setDumpResourceLoadCallbacks(bool dumpResourceLoadCallbacks) { m_dumpResourceLoadCallbacks = dumpResourceLoadCallbacks; }
    
    bool dumpResourceResponseMIMETypes() const { return m_dumpResourceResponseMIMETypes; }
    void setDumpResourceResponseMIMETypes(bool dumpResourceResponseMIMETypes) { m_dumpResourceResponseMIMETypes = dumpResourceResponseMIMETypes; }

    bool dumpSelectionRect() const { return m_dumpSelectionRect; }
    void setDumpSelectionRect(bool dumpSelectionRect) { m_dumpSelectionRect = dumpSelectionRect; }

    bool dumpSourceAsWebArchive() const { return m_dumpSourceAsWebArchive; }
    void setDumpSourceAsWebArchive(bool dumpSourceAsWebArchive) { m_dumpSourceAsWebArchive = dumpSourceAsWebArchive; }

    bool dumpStatusCallbacks() const { return m_dumpStatusCallbacks; }
    void setDumpStatusCallbacks(bool dumpStatusCallbacks) { m_dumpStatusCallbacks = dumpStatusCallbacks; }

    bool dumpTitleChanges() const { return m_dumpTitleChanges; }
    void setDumpTitleChanges(bool dumpTitleChanges) { m_dumpTitleChanges = dumpTitleChanges; }

    bool dumpIconChanges() const { return m_dumpIconChanges; }
    void setDumpIconChanges(bool dumpIconChanges) { m_dumpIconChanges = dumpIconChanges; }

    bool dumpVisitedLinksCallback() const { return m_dumpVisitedLinksCallback; }
    void setDumpVisitedLinksCallback(bool dumpVisitedLinksCallback) { m_dumpVisitedLinksCallback = dumpVisitedLinksCallback; }
    
    bool dumpWillCacheResponse() const { return m_dumpWillCacheResponse; }
    void setDumpWillCacheResponse(bool dumpWillCacheResponse) { m_dumpWillCacheResponse = dumpWillCacheResponse; }
    
    bool callCloseOnWebViews() const { return m_callCloseOnWebViews; }
    void setCallCloseOnWebViews(bool callCloseOnWebViews) { m_callCloseOnWebViews = callCloseOnWebViews; }

    bool canOpenWindows() const { return m_canOpenWindows; }
    void setCanOpenWindows(bool canOpenWindows) { m_canOpenWindows = canOpenWindows; }

    bool closeRemainingWindowsWhenComplete() const { return m_closeRemainingWindowsWhenComplete; }
    void setCloseRemainingWindowsWhenComplete(bool closeRemainingWindowsWhenComplete) { m_closeRemainingWindowsWhenComplete = closeRemainingWindowsWhenComplete; }
    
    bool newWindowsCopyBackForwardList() const { return m_newWindowsCopyBackForwardList; }
    void setNewWindowsCopyBackForwardList(bool newWindowsCopyBackForwardList) { m_newWindowsCopyBackForwardList = newWindowsCopyBackForwardList; }
    
    bool stopProvisionalFrameLoads() const { return m_stopProvisionalFrameLoads; }
    void setStopProvisionalFrameLoads(bool stopProvisionalFrameLoads) { m_stopProvisionalFrameLoads = stopProvisionalFrameLoads; }

    bool testOnscreen() const { return m_testOnscreen; }
    void setTestOnscreen(bool testOnscreen) { m_testOnscreen = testOnscreen; }

    bool testRepaint() const { return m_testRepaint; }
    void setTestRepaint(bool testRepaint) { m_testRepaint = testRepaint; }

    bool testRepaintSweepHorizontally() const { return m_testRepaintSweepHorizontally; }
    void setTestRepaintSweepHorizontally(bool testRepaintSweepHorizontally) { m_testRepaintSweepHorizontally = testRepaintSweepHorizontally; }

    bool waitToDump() const { return m_waitToDump; }
    void setWaitToDump(bool);
    void waitToDumpWatchdogTimerFired();

    const std::set<std::string>& willSendRequestClearHeaders() const { return m_willSendRequestClearHeaders; }
    void setWillSendRequestClearHeader(std::string header) { m_willSendRequestClearHeaders.insert(header); }

    bool willSendRequestReturnsNull() const { return m_willSendRequestReturnsNull; }
    void setWillSendRequestReturnsNull(bool returnsNull) { m_willSendRequestReturnsNull = returnsNull; }

    bool willSendRequestReturnsNullOnRedirect() const { return m_willSendRequestReturnsNullOnRedirect; }
    void setWillSendRequestReturnsNullOnRedirect(bool returnsNull) { m_willSendRequestReturnsNullOnRedirect = returnsNull; }

    bool windowIsKey() const { return m_windowIsKey; }
    void setWindowIsKey(bool);

    bool alwaysAcceptCookies() const { return m_alwaysAcceptCookies; }
    void setAlwaysAcceptCookies(bool);
    
    bool handlesAuthenticationChallenges() const { return m_handlesAuthenticationChallenges; }
    void setHandlesAuthenticationChallenges(bool handlesAuthenticationChallenges) { m_handlesAuthenticationChallenges = handlesAuthenticationChallenges; }
    
    bool isPrinting() const { return m_isPrinting; }
    void setIsPrinting(bool isPrinting) { m_isPrinting = isPrinting; }

    const std::string& authenticationUsername() const { return m_authenticationUsername; }
    void setAuthenticationUsername(std::string username) { m_authenticationUsername = username; }
    
    const std::string& authenticationPassword() const { return m_authenticationPassword; }
    void setAuthenticationPassword(std::string password) { m_authenticationPassword = password; }

    bool globalFlag() const { return m_globalFlag; }
    void setGlobalFlag(bool globalFlag) { m_globalFlag = globalFlag; }
    
    bool deferMainResourceDataLoad() const { return m_deferMainResourceDataLoad; }
    void setDeferMainResourceDataLoad(bool flag) { m_deferMainResourceDataLoad = flag; }

    bool useDeferredFrameLoading() const { return m_useDeferredFrameLoading; }
    void setUseDeferredFrameLoading(bool flag) { m_useDeferredFrameLoading = flag; }

    const std::string& testPathOrURL() const { return m_testPathOrURL; }
    const std::string& expectedPixelHash() const { return m_expectedPixelHash; }

    const std::string& encodedAudioData() const { return m_encodedAudioData; }
    void setEncodedAudioData(const std::string& encodedAudioData) { m_encodedAudioData = encodedAudioData; }

    void addOriginAccessWhitelistEntry(JSStringRef sourceOrigin, JSStringRef destinationProtocol, JSStringRef destinationHost, bool allowDestinationSubdomains);
    void removeOriginAccessWhitelistEntry(JSStringRef sourceOrigin, JSStringRef destinationProtocol, JSStringRef destinationHost, bool allowDestinationSubdomains);

    void addUserScript(JSStringRef source, bool runAtStart, bool allFrames);
    void addUserStyleSheet(JSStringRef source, bool allFrames);

    void setGeolocationPermission(bool allow);
    bool isGeolocationPermissionSet() const { return m_isGeolocationPermissionSet; }
    bool geolocationPermission() const { return m_geolocationPermission; }

    void setDeveloperExtrasEnabled(bool);
    void showWebInspector();
    void closeWebInspector();
    void evaluateInWebInspector(long callId, JSStringRef script);
    void evaluateScriptInIsolatedWorld(unsigned worldID, JSObjectRef globalObject, JSStringRef script);
    void evaluateScriptInIsolatedWorldAndReturnValue(unsigned worldID, JSObjectRef globalObject, JSStringRef script);

    bool shouldStayOnPageAfterHandlingBeforeUnload() const { return m_shouldStayOnPageAfterHandlingBeforeUnload; }
    void setShouldStayOnPageAfterHandlingBeforeUnload(bool shouldStayOnPageAfterHandlingBeforeUnload) { m_shouldStayOnPageAfterHandlingBeforeUnload = shouldStayOnPageAfterHandlingBeforeUnload; }

    void addChromeInputField();
    void removeChromeInputField();
    void focusWebView();

    void setBackingScaleFactor(double);

    void setPOSIXLocale(JSStringRef);

    void setWebViewEditable(bool);

    void abortModal();

    static void setSerializeHTTPLoads(bool);

    // The following API test functions should probably be moved to platform-specific 
    // unit tests outside of DRT once they exist.
    void apiTestNewWindowDataLoadBaseURL(JSStringRef utf8Data, JSStringRef baseURL);
    void apiTestGoToCurrentBackForwardItem();

    // Simulate a request an embedding application could make, populating per-session credential storage.
    void authenticateSession(JSStringRef url, JSStringRef username, JSStringRef password);

    JSValueRef originsWithLocalStorage(JSContextRef);
    void deleteAllLocalStorage();
    void deleteLocalStorageForOrigin(JSStringRef originIdentifier);
    long long localStorageDiskUsageForOrigin(JSStringRef originIdentifier);
    void observeStorageTrackerNotifications(unsigned number);
    void syncLocalStorage();

    void setShouldPaintBrokenImage(bool);
    bool shouldPaintBrokenImage() const { return m_shouldPaintBrokenImage; }

    void setTextDirection(JSStringRef);
    const std::string& titleTextDirection() const { return m_titleTextDirection; }
    void setTitleTextDirection(const std::string& direction) { m_titleTextDirection = direction; }

    // Custom full screen behavior.
    void setHasCustomFullScreenBehavior(bool value) { m_customFullScreenBehavior = value; }
    bool hasCustomFullScreenBehavior() const { return m_customFullScreenBehavior; }

    void setStorageDatabaseIdleInterval(double);
    void closeIdleLocalStorageDatabases();

    bool hasPendingWebNotificationClick() const { return m_hasPendingWebNotificationClick; }

private:
    TestRunner(const std::string& testPathOrURL, const std::string& expectedPixelHash);

    void setGeolocationPermissionCommon(bool allow);

    bool m_disallowIncreaseForApplicationCacheQuota;
    bool m_dumpApplicationCacheDelegateCallbacks;
    bool m_dumpAsAudio;
    bool m_dumpAsPDF;
    bool m_dumpAsText;
    bool m_dumpBackForwardList;
    bool m_dumpChildFrameScrollPositions;
    bool m_dumpChildFramesAsText;
    bool m_dumpDOMAsWebArchive;
    bool m_dumpDatabaseCallbacks;
    bool m_dumpEditingCallbacks;
    bool m_dumpFrameLoadCallbacks;
    bool m_dumpProgressFinishedCallback;
    bool m_dumpUserGestureInFrameLoadCallbacks;
    bool m_dumpHistoryDelegateCallbacks;
    bool m_dumpResourceLoadCallbacks;
    bool m_dumpResourceResponseMIMETypes;
    bool m_dumpSelectionRect;
    bool m_dumpSourceAsWebArchive;
    bool m_dumpStatusCallbacks;
    bool m_dumpTitleChanges;
    bool m_dumpIconChanges;
    bool m_dumpVisitedLinksCallback;
    bool m_dumpWillCacheResponse;
    bool m_generatePixelResults;
    bool m_callCloseOnWebViews;
    bool m_canOpenWindows;
    bool m_closeRemainingWindowsWhenComplete;
    bool m_newWindowsCopyBackForwardList;
    bool m_stopProvisionalFrameLoads;
    bool m_testOnscreen;
    bool m_testRepaint;
    bool m_testRepaintSweepHorizontally;
    bool m_waitToDump; // True if waitUntilDone() has been called, but notifyDone() has not yet been called.
    bool m_willSendRequestReturnsNull;
    bool m_willSendRequestReturnsNullOnRedirect;
    bool m_windowIsKey;
    bool m_alwaysAcceptCookies;
    bool m_globalFlag;
    bool m_isGeolocationPermissionSet;
    bool m_geolocationPermission;
    bool m_handlesAuthenticationChallenges;
    bool m_isPrinting;
    bool m_deferMainResourceDataLoad;
    bool m_useDeferredFrameLoading;
    bool m_shouldPaintBrokenImage;
    bool m_shouldStayOnPageAfterHandlingBeforeUnload;
    // FIXME 81697: This variable most likely will be removed once we have migrated the tests from fast/notifications to http/tests/notifications.
    bool m_areLegacyWebNotificationPermissionRequestsIgnored;
    bool m_customFullScreenBehavior;
    bool m_hasPendingWebNotificationClick;

    std::string m_authenticationUsername;
    std::string m_authenticationPassword; 
    std::string m_testPathOrURL;
    std::string m_expectedPixelHash; // empty string if no hash
    std::string m_titleTextDirection;

    std::set<std::string> m_willSendRequestClearHeaders;
    
    // base64 encoded WAV audio data is stored here.
    std::string m_encodedAudioData;

    std::map<std::string, std::string> m_URLsToRedirect;
    
    static JSClassRef getJSClass();
    static JSStaticValue* staticValues();
    static JSStaticFunction* staticFunctions();
};

#endif // TestRunner_h
