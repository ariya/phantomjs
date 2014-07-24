/*
 * Copyright (C) 2007, 2008, 2009, 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Joone Hur <joone@kldp.org>
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
#include "TestRunner.h"

#include "WorkQueue.h"
#include "WorkQueueItem.h"
#include <JavaScriptCore/JSContextRef.h>
#include <JavaScriptCore/JSCTestRunnerUtils.h>
#include <JavaScriptCore/JSObjectRef.h>
#include <JavaScriptCore/JSRetainPtr.h>
#include <cstring>
#include <locale.h>
#include <stdio.h>
#include <wtf/Assertions.h>
#include <wtf/CurrentTime.h>
#include <wtf/MathExtras.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/RefPtr.h>

#if PLATFORM(MAC)
#include <Carbon/Carbon.h>
#endif

const unsigned TestRunner::viewWidth = 800;
const unsigned TestRunner::viewHeight = 600;

const unsigned TestRunner::w3cSVGViewWidth = 480;
const unsigned TestRunner::w3cSVGViewHeight = 360;

TestRunner::TestRunner(const std::string& testPathOrURL, const std::string& expectedPixelHash)
    : m_disallowIncreaseForApplicationCacheQuota(false)
    , m_dumpApplicationCacheDelegateCallbacks(false)
    , m_dumpAsAudio(false)
    , m_dumpAsPDF(false)
    , m_dumpAsText(false)
    , m_dumpBackForwardList(false)
    , m_dumpChildFrameScrollPositions(false)
    , m_dumpChildFramesAsText(false)
    , m_dumpDOMAsWebArchive(false)
    , m_dumpDatabaseCallbacks(false)
    , m_dumpEditingCallbacks(false)
    , m_dumpFrameLoadCallbacks(false)
    , m_dumpProgressFinishedCallback(false)
    , m_dumpUserGestureInFrameLoadCallbacks(false)
    , m_dumpHistoryDelegateCallbacks(false)
    , m_dumpResourceLoadCallbacks(false)
    , m_dumpResourceResponseMIMETypes(false)
    , m_dumpSelectionRect(false)
    , m_dumpSourceAsWebArchive(false)
    , m_dumpStatusCallbacks(false)
    , m_dumpTitleChanges(false)
    , m_dumpIconChanges(false)
    , m_dumpVisitedLinksCallback(false)
    , m_dumpWillCacheResponse(false)
    , m_generatePixelResults(true)
    , m_callCloseOnWebViews(true)
    , m_canOpenWindows(false)
    , m_closeRemainingWindowsWhenComplete(true)
    , m_newWindowsCopyBackForwardList(false)
    , m_stopProvisionalFrameLoads(false)
    , m_testOnscreen(false)
    , m_testRepaint(false)
    , m_testRepaintSweepHorizontally(false)
    , m_waitToDump(false)
    , m_willSendRequestReturnsNull(false)
    , m_willSendRequestReturnsNullOnRedirect(false)
    , m_windowIsKey(true)
    , m_alwaysAcceptCookies(false)
    , m_globalFlag(false)
    , m_isGeolocationPermissionSet(false)
    , m_geolocationPermission(false)
    , m_handlesAuthenticationChallenges(false)
    , m_isPrinting(false)
    , m_deferMainResourceDataLoad(true)
    , m_useDeferredFrameLoading(false)
    , m_shouldPaintBrokenImage(true)
    , m_shouldStayOnPageAfterHandlingBeforeUnload(false)
    , m_areLegacyWebNotificationPermissionRequestsIgnored(false)
    , m_customFullScreenBehavior(false) 
    , m_hasPendingWebNotificationClick(false)
    , m_testPathOrURL(testPathOrURL)
    , m_expectedPixelHash(expectedPixelHash)
    , m_titleTextDirection("ltr")
{
}

PassRefPtr<TestRunner> TestRunner::create(const std::string& testPathOrURL, const std::string& expectedPixelHash)
{
    return adoptRef(new TestRunner(testPathOrURL, expectedPixelHash));
}

// Static Functions

static JSValueRef disallowIncreaseForApplicationCacheQuotaCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDisallowIncreaseForApplicationCacheQuota(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef dumpApplicationCacheDelegateCallbacksCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDumpApplicationCacheDelegateCallbacks(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef dumpAsPDFCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDumpAsPDF(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef dumpAsTextCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDumpAsText(true);

    // Optional paramater, describing whether it's allowed to dump pixel results in dumpAsText mode.
    controller->setGeneratePixelResults(argumentCount > 0 ? JSValueToBoolean(context, arguments[0]) : false);

    return JSValueMakeUndefined(context);
}

static JSValueRef dumpBackForwardListCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDumpBackForwardList(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef dumpChildFramesAsTextCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDumpChildFramesAsText(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef dumpChildFrameScrollPositionsCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDumpChildFrameScrollPositions(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef dumpDatabaseCallbacksCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDumpDatabaseCallbacks(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef dumpDOMAsWebArchiveCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDumpDOMAsWebArchive(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef dumpEditingCallbacksCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDumpEditingCallbacks(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef dumpFrameLoadCallbacksCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDumpFrameLoadCallbacks(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef dumpProgressFinishedCallbackCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDumpProgressFinishedCallback(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef dumpUserGestureInFrameLoadCallbacksCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDumpUserGestureInFrameLoadCallbacks(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef dumpResourceLoadCallbacksCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDumpResourceLoadCallbacks(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef dumpResourceResponseMIMETypesCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDumpResourceResponseMIMETypes(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef dumpSelectionRectCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDumpSelectionRect(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef dumpSourceAsWebArchiveCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDumpSourceAsWebArchive(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef dumpStatusCallbacksCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDumpStatusCallbacks(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef dumpTitleChangesCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDumpTitleChanges(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef dumpIconChangesCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDumpIconChanges(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef dumpWillCacheResponseCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDumpWillCacheResponse(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef pathToLocalResourceCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    JSRetainPtr<JSStringRef> localPath(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    JSRetainPtr<JSStringRef> convertedPath(Adopt, controller->pathToLocalResource(context, localPath.get()));
    if (!convertedPath)
        return JSValueMakeUndefined(context);

    return JSValueMakeString(context, convertedPath.get());
}

static JSValueRef removeAllVisitedLinksCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDumpVisitedLinksCallback(true);
    controller->removeAllVisitedLinks();
    return JSValueMakeUndefined(context);
}

static JSValueRef repaintSweepHorizontallyCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setTestRepaintSweepHorizontally(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef setCallCloseOnWebViewsCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setCallCloseOnWebViews(JSValueToBoolean(context, arguments[0]));
    return JSValueMakeUndefined(context);
}

static JSValueRef setCanOpenWindowsCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setCanOpenWindows(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef setCloseRemainingWindowsWhenCompleteCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setCloseRemainingWindowsWhenComplete(JSValueToBoolean(context, arguments[0]));
    return JSValueMakeUndefined(context);
}

static JSValueRef setEncodedAudioDataCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> encodedAudioData(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);
    
    size_t maxLength = JSStringGetMaximumUTF8CStringSize(encodedAudioData.get());
    OwnArrayPtr<char> encodedAudioDataBuffer = adoptArrayPtr(new char[maxLength + 1]);
    JSStringGetUTF8CString(encodedAudioData.get(), encodedAudioDataBuffer.get(), maxLength + 1);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setEncodedAudioData(encodedAudioDataBuffer.get());
    controller->setDumpAsAudio(true);
    
    return JSValueMakeUndefined(context);
}

static JSValueRef testOnscreenCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setTestOnscreen(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef testRepaintCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setTestRepaint(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef addDisallowedURLCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> url(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->addDisallowedURL(url.get());

    return JSValueMakeUndefined(context);
}

static JSValueRef addURLToRedirectCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> origin(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    JSRetainPtr<JSStringRef> destination(Adopt, JSValueToStringCopy(context, arguments[1], exception));
    ASSERT(!*exception);

    size_t maxLength = JSStringGetMaximumUTF8CStringSize(origin.get());
    OwnArrayPtr<char> originBuffer = adoptArrayPtr(new char[maxLength + 1]);
    JSStringGetUTF8CString(origin.get(), originBuffer.get(), maxLength + 1);

    maxLength = JSStringGetMaximumUTF8CStringSize(destination.get());
    OwnArrayPtr<char> destinationBuffer = adoptArrayPtr(new char[maxLength + 1]);
    JSStringGetUTF8CString(destination.get(), destinationBuffer.get(), maxLength + 1);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->addURLToRedirect(originBuffer.get(), destinationBuffer.get());

    return JSValueMakeUndefined(context);
}

static JSValueRef callShouldCloseOnWebViewCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));

    return JSValueMakeBoolean(context, controller->callShouldCloseOnWebView());
}

static JSValueRef clearAllApplicationCachesCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->clearAllApplicationCaches();

    return JSValueMakeUndefined(context);
}

static JSValueRef clearApplicationCacheForOriginCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> originURL(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->clearApplicationCacheForOrigin(originURL.get());
    
    return JSValueMakeUndefined(context);
}

static JSValueRef applicationCacheDiskUsageForOriginCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> originURL(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);
    
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));

    return JSValueMakeNumber(context, controller->applicationCacheDiskUsageForOrigin(originURL.get()));
}

static JSValueRef originsWithApplicationCacheCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    return controller->originsWithApplicationCache(context);
}

static JSValueRef clearAllDatabasesCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->clearAllDatabases();

    return JSValueMakeUndefined(context);
}

static JSValueRef syncLocalStorageCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));

    controller->syncLocalStorage();

    return JSValueMakeUndefined(context);
}

static JSValueRef observeStorageTrackerNotificationsCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));

    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    unsigned numNotifications = JSValueToNumber(context, arguments[0], exception);

    ASSERT(!*exception);

    controller->observeStorageTrackerNotifications(numNotifications);

    return JSValueMakeUndefined(context);
}

static JSValueRef deleteAllLocalStorageCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->deleteAllLocalStorage();

    return JSValueMakeUndefined(context);
}

static JSValueRef deleteLocalStorageForOriginCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));

    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> url(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    controller->deleteLocalStorageForOrigin(url.get());

    return JSValueMakeUndefined(context);
}

static JSValueRef localStorageDiskUsageForOriginCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));

    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> originURL(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    return JSValueMakeNumber(context, controller->localStorageDiskUsageForOrigin(originURL.get()));
}

static JSValueRef originsWithLocalStorageCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    return controller->originsWithLocalStorage(context);
}

static JSValueRef clearBackForwardListCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->clearBackForwardList();

    return JSValueMakeUndefined(context);
}

static JSValueRef clearPersistentUserStyleSheetCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->clearPersistentUserStyleSheet();

    return JSValueMakeUndefined(context);
}

static JSValueRef decodeHostNameCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> name(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    JSRetainPtr<JSStringRef> decodedHostName(Adopt, controller->copyDecodedHostName(name.get()));
    return JSValueMakeString(context, decodedHostName.get());
}

static JSValueRef dispatchPendingLoadRequestsCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation, needs windows implementation
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->dispatchPendingLoadRequests();
    
    return JSValueMakeUndefined(context);
}

static JSValueRef displayCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->display();

    return JSValueMakeUndefined(context);
}

static JSValueRef displayInvalidatedRegionCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    // TestRunner::display() only renders the invalidated region so
    // we can just use that.
    controller->display();

    return JSValueMakeUndefined(context);
}

static JSValueRef encodeHostNameCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> name(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    JSRetainPtr<JSStringRef> encodedHostName(Adopt, controller->copyEncodedHostName(name.get()));
    return JSValueMakeString(context, encodedHostName.get());
}

static JSValueRef execCommandCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has Mac & Windows implementations.
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> name(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    // Ignoring the second parameter (userInterface), as this command emulates a manual action.

    JSRetainPtr<JSStringRef> value;
    if (argumentCount >= 3) {
        value.adopt(JSValueToStringCopy(context, arguments[2], exception));
        ASSERT(!*exception);
    } else
        value.adopt(JSStringCreateWithUTF8CString(""));


    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->execCommand(name.get(), value.get());

    return JSValueMakeUndefined(context);
}

static JSValueRef findStringCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has Mac implementation.
    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> target(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    JSObjectRef options = JSValueToObject(context, arguments[1], exception);
    ASSERT(!*exception);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    return JSValueMakeBoolean(context, controller->findString(context, target.get(), options));
}

static JSValueRef goBackCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->goBack();
    
    return JSValueMakeUndefined(context);
}

static JSValueRef isCommandEnabledCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has Mac implementation.

    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> name(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));

    return JSValueMakeBoolean(context, controller->isCommandEnabled(name.get()));
}

static JSValueRef overridePreferenceCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> key(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);
    JSRetainPtr<JSStringRef> value(Adopt, JSValueToStringCopy(context, arguments[1], exception));
    ASSERT(!*exception);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->overridePreference(key.get(), value.get());

    return JSValueMakeUndefined(context);
}

static JSValueRef keepWebHistoryCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->keepWebHistory();

    return JSValueMakeUndefined(context);
}

static JSValueRef notifyDoneCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    // May be able to be made platform independant by using shared WorkQueue
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->notifyDone();
    return JSValueMakeUndefined(context);
}

static JSValueRef numberOfPendingGeolocationPermissionRequestsCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    return JSValueMakeNumber(context, controller->numberOfPendingGeolocationPermissionRequests());
}

static JSValueRef queueBackNavigationCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    // May be able to be made platform independant by using shared WorkQueue
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    double howFarBackDouble = JSValueToNumber(context, arguments[0], exception);
    ASSERT(!*exception);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->queueBackNavigation(static_cast<int>(howFarBackDouble));

    return JSValueMakeUndefined(context);
}

static JSValueRef queueForwardNavigationCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    // May be able to be made platform independant by using shared WorkQueue
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    double howFarForwardDouble = JSValueToNumber(context, arguments[0], exception);
    ASSERT(!*exception);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->queueForwardNavigation(static_cast<int>(howFarForwardDouble));

    return JSValueMakeUndefined(context);
}

static JSValueRef queueLoadCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    // May be able to be made platform independant by using shared WorkQueue
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> url(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    JSRetainPtr<JSStringRef> target;
    if (argumentCount >= 2) {
        target.adopt(JSValueToStringCopy(context, arguments[1], exception));
        ASSERT(!*exception);
    } else
        target.adopt(JSStringCreateWithUTF8CString(""));

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->queueLoad(url.get(), target.get());

    return JSValueMakeUndefined(context);
}

static JSValueRef queueLoadHTMLStringCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has Mac & Windows implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> content(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    JSRetainPtr<JSStringRef> baseURL;
    if (argumentCount >= 2) {
        baseURL.adopt(JSValueToStringCopy(context, arguments[1], exception));
        ASSERT(!*exception);
    } else
        baseURL.adopt(JSStringCreateWithUTF8CString(""));

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));

    if (argumentCount >= 3) {
        JSRetainPtr<JSStringRef> unreachableURL;
        unreachableURL.adopt(JSValueToStringCopy(context, arguments[2], exception));
        ASSERT(!*exception);
        controller->queueLoadAlternateHTMLString(content.get(), baseURL.get(), unreachableURL.get());
        return JSValueMakeUndefined(context);
    }

    controller->queueLoadHTMLString(content.get(), baseURL.get());
    return JSValueMakeUndefined(context);
}

static JSValueRef queueReloadCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    // May be able to be made platform independant by using shared WorkQueue

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->queueReload();

    return JSValueMakeUndefined(context);
}

static JSValueRef queueLoadingScriptCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    // May be able to be made platform independant by using shared WorkQueue
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> script(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->queueLoadingScript(script.get());

    return JSValueMakeUndefined(context);
}

static JSValueRef queueNonLoadingScriptCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    // May be able to be made platform independant by using shared WorkQueue
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> script(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->queueNonLoadingScript(script.get());

    return JSValueMakeUndefined(context);
}

static JSValueRef setAcceptsEditingCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setAcceptsEditing(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setAlwaysAcceptCookiesCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setAlwaysAcceptCookies(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setAppCacheMaximumSizeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));

    double size = JSValueToNumber(context, arguments[0], NULL);
    if (!std::isnan(size))
        controller->setAppCacheMaximumSize(static_cast<unsigned long long>(size));
        
    return JSValueMakeUndefined(context);
}

static JSValueRef setApplicationCacheOriginQuotaCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));

    double size = JSValueToNumber(context, arguments[0], NULL);
    if (!std::isnan(size))
        controller->setApplicationCacheOriginQuota(static_cast<unsigned long long>(size));

    return JSValueMakeUndefined(context);
}

static JSValueRef setAuthenticationPasswordCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> password(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    size_t maxLength = JSStringGetMaximumUTF8CStringSize(password.get());
    char* passwordBuffer = new char[maxLength + 1];
    JSStringGetUTF8CString(password.get(), passwordBuffer, maxLength + 1);
    
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setAuthenticationPassword(passwordBuffer);
    delete[] passwordBuffer;

    return JSValueMakeUndefined(context);
}

static JSValueRef setAuthenticationUsernameCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> username(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    size_t maxLength = JSStringGetMaximumUTF8CStringSize(username.get());
    char* usernameBuffer = new char[maxLength + 1];
    JSStringGetUTF8CString(username.get(), usernameBuffer, maxLength + 1);
    
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setAuthenticationUsername(usernameBuffer);
    delete[] usernameBuffer;

    return JSValueMakeUndefined(context);
}

static JSValueRef setAuthorAndUserStylesEnabledCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setAuthorAndUserStylesEnabled(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setCacheModelCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has Mac implementation.
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    int cacheModel = JSValueToNumber(context, arguments[0], exception);
    ASSERT(!*exception);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setCacheModel(cacheModel);

    return JSValueMakeUndefined(context);
}

static JSValueRef setCustomPolicyDelegateCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);
    
    bool permissive = false;
    if (argumentCount >= 2)
        permissive = JSValueToBoolean(context, arguments[1]);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setCustomPolicyDelegate(JSValueToBoolean(context, arguments[0]), permissive);

    return JSValueMakeUndefined(context);
}

static JSValueRef setDatabaseQuotaCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));

    double quota = JSValueToNumber(context, arguments[0], NULL);
    if (!std::isnan(quota))
        controller->setDatabaseQuota(static_cast<unsigned long long>(quota));
        
    return JSValueMakeUndefined(context);
}

static JSValueRef setDeferMainResourceDataLoadCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has Mac and Windows implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDeferMainResourceDataLoad(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setDefersLoadingCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setDefersLoading(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setUseDeferredFrameLoadingCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setUseDeferredFrameLoading(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setDomainRelaxationForbiddenForURLSchemeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has Mac and Windows implementation
    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));

    bool forbidden = JSValueToBoolean(context, arguments[0]);
    JSRetainPtr<JSStringRef> scheme(Adopt, JSValueToStringCopy(context, arguments[1], 0));
    controller->setDomainRelaxationForbiddenForURLScheme(forbidden, scheme.get());

    return JSValueMakeUndefined(context);
}

static JSValueRef setMockDeviceOrientationCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 6)
        return JSValueMakeUndefined(context);

    bool canProvideAlpha = JSValueToBoolean(context, arguments[0]);
    double alpha = JSValueToNumber(context, arguments[1], exception);
    ASSERT(!*exception);
    bool canProvideBeta = JSValueToBoolean(context, arguments[2]);
    double beta = JSValueToNumber(context, arguments[3], exception);
    ASSERT(!*exception);
    bool canProvideGamma = JSValueToBoolean(context, arguments[4]);
    double gamma = JSValueToNumber(context, arguments[5], exception);
    ASSERT(!*exception);

    TestRunner* controller = reinterpret_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setMockDeviceOrientation(canProvideAlpha, alpha, canProvideBeta, beta, canProvideGamma, gamma);

    return JSValueMakeUndefined(context);
}

static JSValueRef setMockGeolocationPositionCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 3)
        return JSValueMakeUndefined(context);

    double latitude = JSValueToNumber(context, arguments[0], 0);
    double longitude = JSValueToNumber(context, arguments[1], 0);
    double accuracy = JSValueToNumber(context, arguments[2], 0);

    bool canProvideAltitude = false;
    double altitude = 0.;
    if (argumentCount > 3 && !JSValueIsUndefined(context, arguments[3])) {
        canProvideAltitude = true;
        altitude = JSValueToNumber(context, arguments[3], 0);
    }

    bool canProvideAltitudeAccuracy = false;
    double altitudeAccuracy = 0.;
    if (argumentCount > 4 && !JSValueIsUndefined(context, arguments[4])) {
        canProvideAltitudeAccuracy = true;
        altitudeAccuracy = JSValueToNumber(context, arguments[4], 0);
    }

    bool canProvideHeading = false;
    double heading = 0.;
    if (argumentCount > 5 && !JSValueIsUndefined(context, arguments[5])) {
        canProvideHeading = true;
        heading = JSValueToNumber(context, arguments[5], 0);
    }

    bool canProvideSpeed = false;
    double speed = 0.;
    if (argumentCount > 6 && !JSValueIsUndefined(context, arguments[6])) {
        canProvideSpeed = true;
        speed = JSValueToNumber(context, arguments[6], 0);
    }

    TestRunner* controller = reinterpret_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setMockGeolocationPosition(latitude, longitude, accuracy, canProvideAltitude, altitude, canProvideAltitudeAccuracy, altitudeAccuracy, canProvideHeading, heading, canProvideSpeed, speed);

    return JSValueMakeUndefined(context);
}

static JSValueRef setMockGeolocationPositionUnavailableErrorCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount != 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> message(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    TestRunner* controller = reinterpret_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setMockGeolocationPositionUnavailableError(message.get());

    return JSValueMakeUndefined(context);
}

static JSValueRef addMockSpeechInputResultCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 3)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> result(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    double confidence = JSValueToNumber(context, arguments[1], exception);

    JSRetainPtr<JSStringRef> language(Adopt, JSValueToStringCopy(context, arguments[2], exception));
    ASSERT(!*exception);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->addMockSpeechInputResult(result.get(), confidence, language.get());

    return JSValueMakeUndefined(context);
}

static JSValueRef setMockSpeechInputDumpRectCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    bool dumpRect = JSValueToBoolean(context, arguments[0]);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setMockSpeechInputDumpRect(dumpRect);

    return JSValueMakeUndefined(context);
}

static JSValueRef setNewWindowsCopyBackForwardListCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);
    
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setNewWindowsCopyBackForwardList(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setGeolocationPermissionCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setGeolocationPermission(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setHandlesAuthenticationChallengesCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setHandlesAuthenticationChallenges(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setPOSIXLocaleCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    JSRetainPtr<JSStringRef> locale(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);
    controller->setPOSIXLocale(locale.get());

    return JSValueMakeUndefined(context);
}

static JSValueRef setIconDatabaseEnabledCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setIconDatabaseEnabled(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setMainFrameIsFirstResponderCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setMainFrameIsFirstResponder(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setPersistentUserStyleSheetLocationCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> path(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setPersistentUserStyleSheetLocation(path.get());

    return JSValueMakeUndefined(context);
}

static JSValueRef setPrivateBrowsingEnabledCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setPrivateBrowsingEnabled(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setJavaScriptCanAccessClipboardCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setJavaScriptCanAccessClipboard(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setXSSAuditorEnabledCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setXSSAuditorEnabled(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setSpatialNavigationEnabledCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation.
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setSpatialNavigationEnabled(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setPrintingCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setIsPrinting(true);
    return JSValueMakeUndefined(context);
}


static JSValueRef setAllowUniversalAccessFromFileURLsCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setAllowUniversalAccessFromFileURLs(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setAllowFileAccessFromFileURLsCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setAllowFileAccessFromFileURLs(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setTabKeyCyclesThroughElementsCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setTabKeyCyclesThroughElements(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setUseDashboardCompatibilityModeCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setUseDashboardCompatibilityMode(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setUserStyleSheetEnabledCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setUserStyleSheetEnabled(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setUserStyleSheetLocationCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> path(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setUserStyleSheetLocation(path.get());

    return JSValueMakeUndefined(context);
}

static JSValueRef setValueForUserCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation
    if (argumentCount != 2)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> value(Adopt, JSValueToStringCopy(context, arguments[1], exception));
    ASSERT(!*exception);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setValueForUser(context, arguments[0], value.get());

    return JSValueMakeUndefined(context);
}

static JSValueRef setViewModeMediaFeatureCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> mode(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setViewModeMediaFeature(mode.get());

    return JSValueMakeUndefined(context);
}

static JSValueRef setWillSendRequestClearHeaderCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> header(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    size_t maxLength = JSStringGetMaximumUTF8CStringSize(header.get());
    OwnArrayPtr<char> headerBuffer = adoptArrayPtr(new char[maxLength + 1]);
    JSStringGetUTF8CString(header.get(), headerBuffer.get(), maxLength + 1);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setWillSendRequestClearHeader(headerBuffer.get());

    return JSValueMakeUndefined(context);
}

static JSValueRef setWillSendRequestReturnsNullCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has cross-platform implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setWillSendRequestReturnsNull(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setWillSendRequestReturnsNullOnRedirectCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has cross-platform implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setWillSendRequestReturnsNullOnRedirect(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setWindowIsKeyCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setWindowIsKey(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef waitUntilDoneCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setWaitToDump(true);

    return JSValueMakeUndefined(context);
}

static JSValueRef windowCountCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac implementation
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    int windows = controller->windowCount();
    return JSValueMakeNumber(context, windows);
}

static JSValueRef setPopupBlockingEnabledCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setPopupBlockingEnabled(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef setPluginsEnabledCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);
    
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setPluginsEnabled(JSValueToBoolean(context, arguments[0]));
    
    return JSValueMakeUndefined(context);
}    

static JSValueRef setPageVisibilityCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has mac & windows implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> visibility(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    size_t maxLength = JSStringGetMaximumUTF8CStringSize(visibility.get());
    char* visibilityBuffer = new char[maxLength + 1];
    JSStringGetUTF8CString(visibility.get(), visibilityBuffer, maxLength + 1);
    
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setPageVisibility(visibilityBuffer);
    delete[] visibilityBuffer;
    
    return JSValueMakeUndefined(context);
}    

static JSValueRef resetPageVisibilityCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->resetPageVisibility();
    return JSValueMakeUndefined(context);
}    

static JSValueRef setAutomaticLinkDetectionEnabledCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setAutomaticLinkDetectionEnabled(JSValueToBoolean(context, arguments[0]));
    return JSValueMakeUndefined(context);
}

static JSValueRef setStopProvisionalFrameLoadsCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setStopProvisionalFrameLoads(true);
    return JSValueMakeUndefined(context);
}

static JSValueRef showWebInspectorCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->showWebInspector();
    return JSValueMakeUndefined(context);
}

static JSValueRef closeWebInspectorCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->closeWebInspector();
    return JSValueMakeUndefined(context);
}

static JSValueRef evaluateInWebInspectorCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    double callId = JSValueToNumber(context, arguments[0], exception);
    ASSERT(!*exception);
    JSRetainPtr<JSStringRef> script(Adopt, JSValueToStringCopy(context, arguments[1], exception));
    ASSERT(!*exception);

    controller->evaluateInWebInspector(static_cast<long>(callId), script.get());
    return JSValueMakeUndefined(context);
}

static JSValueRef evaluateScriptInIsolatedWorldCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    double worldID = JSValueToNumber(context, arguments[0], exception);
    ASSERT(!*exception);
    JSRetainPtr<JSStringRef> script(Adopt, JSValueToStringCopy(context, arguments[1], exception));
    ASSERT(!*exception);

    controller->evaluateScriptInIsolatedWorld(static_cast<unsigned>(worldID), JSContextGetGlobalObject(context), script.get());
    return JSValueMakeUndefined(context);
}

static JSValueRef evaluateScriptInIsolatedWorldAndReturnValueCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    double worldID = JSValueToNumber(context, arguments[0], exception);
    ASSERT(!*exception);
    JSRetainPtr<JSStringRef> script(Adopt, JSValueToStringCopy(context, arguments[1], exception));
    ASSERT(!*exception);

    controller->evaluateScriptInIsolatedWorldAndReturnValue(static_cast<unsigned>(worldID), JSContextGetGlobalObject(context), script.get());
    return JSValueMakeUndefined(context);
}

static JSValueRef waitForPolicyDelegateCallback(JSContextRef context, JSObjectRef, JSObjectRef thisObject, size_t, const JSValueRef[], JSValueRef*)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->waitForPolicyDelegate();
    return JSValueMakeUndefined(context);
}

static JSValueRef addOriginAccessWhitelistEntryCallback(JSContextRef context, JSObjectRef, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount != 4)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> sourceOrigin(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);
    JSRetainPtr<JSStringRef> destinationProtocol(Adopt, JSValueToStringCopy(context, arguments[1], exception));
    ASSERT(!*exception);
    JSRetainPtr<JSStringRef> destinationHost(Adopt, JSValueToStringCopy(context, arguments[2], exception));
    ASSERT(!*exception);
    bool allowDestinationSubdomains = JSValueToBoolean(context, arguments[3]);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->addOriginAccessWhitelistEntry(sourceOrigin.get(), destinationProtocol.get(), destinationHost.get(), allowDestinationSubdomains);
    return JSValueMakeUndefined(context);
}

static JSValueRef removeOriginAccessWhitelistEntryCallback(JSContextRef context, JSObjectRef, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount != 4)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> sourceOrigin(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);
    JSRetainPtr<JSStringRef> destinationProtocol(Adopt, JSValueToStringCopy(context, arguments[1], exception));
    ASSERT(!*exception);
    JSRetainPtr<JSStringRef> destinationHost(Adopt, JSValueToStringCopy(context, arguments[2], exception));
    ASSERT(!*exception);
    bool allowDestinationSubdomains = JSValueToBoolean(context, arguments[3]);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->removeOriginAccessWhitelistEntry(sourceOrigin.get(), destinationProtocol.get(), destinationHost.get(), allowDestinationSubdomains);
    return JSValueMakeUndefined(context);
}

static JSValueRef setScrollbarPolicyCallback(JSContextRef context, JSObjectRef, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount != 2)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> orientation(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);
    JSRetainPtr<JSStringRef> policy(Adopt, JSValueToStringCopy(context, arguments[1], exception));
    ASSERT(!*exception);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setScrollbarPolicy(orientation.get(), policy.get());
    return JSValueMakeUndefined(context);
}

static JSValueRef addUserScriptCallback(JSContextRef context, JSObjectRef, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount != 3)
        return JSValueMakeUndefined(context);
    
    JSRetainPtr<JSStringRef> source(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);
    bool runAtStart = JSValueToBoolean(context, arguments[1]);
    bool allFrames = JSValueToBoolean(context, arguments[2]);
    
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->addUserScript(source.get(), runAtStart, allFrames);
    return JSValueMakeUndefined(context);
}
 
static JSValueRef addUserStyleSheetCallback(JSContextRef context, JSObjectRef, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount != 2)
        return JSValueMakeUndefined(context);
    
    JSRetainPtr<JSStringRef> source(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);
    bool allFrames = JSValueToBoolean(context, arguments[1]);
   
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->addUserStyleSheet(source.get(), allFrames);
    return JSValueMakeUndefined(context);
}

static JSValueRef setShouldPaintBrokenImageCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has Mac implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setShouldPaintBrokenImage(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef apiTestNewWindowDataLoadBaseURLCallback(JSContextRef context, JSObjectRef, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount != 2)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> utf8Data(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);

    JSRetainPtr<JSStringRef> baseURL(Adopt, JSValueToStringCopy(context, arguments[1], exception));
    ASSERT(!*exception);
        
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->apiTestNewWindowDataLoadBaseURL(utf8Data.get(), baseURL.get());
    return JSValueMakeUndefined(context);
}

static JSValueRef apiTestGoToCurrentBackForwardItemCallback(JSContextRef context, JSObjectRef, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->apiTestGoToCurrentBackForwardItem();
    return JSValueMakeUndefined(context);
}

static JSValueRef setWebViewEditableCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has Mac implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setWebViewEditable(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef abortModalCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->abortModal();
    return JSValueMakeUndefined(context);
}

static JSValueRef authenticateSessionCallback(JSContextRef context, JSObjectRef, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // authenticateSession(url, username, password)
    if (argumentCount != 3)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> url(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);
    JSRetainPtr<JSStringRef> username(Adopt, JSValueToStringCopy(context, arguments[1], exception));
    ASSERT(!*exception);
    JSRetainPtr<JSStringRef> password(Adopt, JSValueToStringCopy(context, arguments[2], exception));
    ASSERT(!*exception);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->authenticateSession(url.get(), username.get(), password.get());
    return JSValueMakeUndefined(context);
}

static JSValueRef setSerializeHTTPLoadsCallback(JSContextRef context, JSObjectRef, JSObjectRef, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    bool serialize = true;
    if (argumentCount == 1)
        serialize = JSValueToBoolean(context, arguments[0]);

    TestRunner::setSerializeHTTPLoads(serialize);
    return JSValueMakeUndefined(context);
}

static JSValueRef setShouldStayOnPageAfterHandlingBeforeUnloadCallback(JSContextRef context, JSObjectRef, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    
    if (argumentCount == 1)
        controller->setShouldStayOnPageAfterHandlingBeforeUnload(JSValueToBoolean(context, arguments[0]));

    return JSValueMakeUndefined(context);
}

static JSValueRef addChromeInputFieldCallback(JSContextRef context, JSObjectRef, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->addChromeInputField();
    // the first argument is a callback that is called once the input field has been added
    if (argumentCount == 1)
        JSObjectCallAsFunction(context, JSValueToObject(context, arguments[0], 0), thisObject, 0, 0, 0);
    return JSValueMakeUndefined(context);
}

static JSValueRef removeChromeInputFieldCallback(JSContextRef context, JSObjectRef, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->removeChromeInputField();
    // the first argument is a callback that is called once the input field has been added
    if (argumentCount == 1)
        JSObjectCallAsFunction(context, JSValueToObject(context, arguments[0], 0), thisObject, 0, 0, 0);
    return JSValueMakeUndefined(context);
}

static JSValueRef focusWebViewCallback(JSContextRef context, JSObjectRef, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->focusWebView();
    // the first argument is a callback that is called once the input field has been added
    if (argumentCount == 1)
        JSObjectCallAsFunction(context, JSValueToObject(context, arguments[0], 0), thisObject, 0, 0, 0);
    return JSValueMakeUndefined(context);
}

static JSValueRef setBackingScaleFactorCallback(JSContextRef context, JSObjectRef, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount != 2)
        return JSValueMakeUndefined(context);

    double backingScaleFactor = JSValueToNumber(context, arguments[0], exception);
    ASSERT(!*exception);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setBackingScaleFactor(backingScaleFactor);

    // The second argument is a callback that is called once the backing scale factor has been set.
    JSObjectCallAsFunction(context, JSValueToObject(context, arguments[1], 0), thisObject, 0, 0, 0);
    return JSValueMakeUndefined(context);
}

static JSValueRef preciseTimeCallback(JSContextRef context, JSObjectRef, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    return JSValueMakeNumber(context, WTF::currentTime());
}

// Static Values

static JSValueRef getGlobalFlagCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    return JSValueMakeBoolean(context, controller->globalFlag());
}

static JSValueRef getWebHistoryItemCountCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    return JSValueMakeNumber(context, controller->webHistoryItemCount());
}

#if PLATFORM(MAC) || PLATFORM(GTK) || PLATFORM(WIN) || PLATFORM(EFL)
static JSValueRef getPlatformNameCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    JSRetainPtr<JSStringRef> platformName(controller->platformName());
    if (!platformName.get())
        return JSValueMakeUndefined(context);
    return JSValueMakeString(context, platformName.get());
}
#endif

static JSValueRef getSecureEventInputIsEnabledCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
#if PLATFORM(MAC)
    return JSValueMakeBoolean(context, IsSecureEventInputEnabled());
#else
    return JSValueMakeBoolean(context, false);
#endif
}

static JSValueRef getTitleTextDirectionCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    JSRetainPtr<JSStringRef> titleDirection(Adopt, JSStringCreateWithUTF8CString(controller->titleTextDirection().c_str()));
    return JSValueMakeString(context, titleDirection.get());
}

static bool setGlobalFlagCallback(JSContextRef context, JSObjectRef thisObject, JSStringRef propertyName, JSValueRef value, JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setGlobalFlag(JSValueToBoolean(context, value));
    return true;
}

static JSValueRef ignoreLegacyWebNotificationPermissionRequestsCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->ignoreLegacyWebNotificationPermissionRequests();
    return JSValueMakeUndefined(context);
}

static JSValueRef simulateLegacyWebNotificationClickCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject)); 
    JSRetainPtr<JSStringRef> title(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    controller->simulateLegacyWebNotificationClick(title.get());
    return JSValueMakeUndefined(context);
}

static JSValueRef setTextDirectionCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount == 1) {
        JSRetainPtr<JSStringRef> direction(Adopt, JSValueToStringCopy(context, arguments[0], exception));
        TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
        controller->setTextDirection(direction.get());
    }

    return JSValueMakeUndefined(context);

}

static JSValueRef setHasCustomFullScreenBehaviorCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount == 1) {
        bool hasCustomBehavior = JSValueToBoolean(context, arguments[0]);
        TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
        controller->setHasCustomFullScreenBehavior(hasCustomBehavior);
    }

    return JSValueMakeUndefined(context);
}

static JSValueRef setStorageDatabaseIdleIntervalCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount != 1)
        return JSValueMakeUndefined(context);

    double interval = JSValueToNumber(context, arguments[0], exception);
    ASSERT(!*exception);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->setStorageDatabaseIdleInterval(interval);

    return JSValueMakeUndefined(context);
}

static JSValueRef closeIdleLocalStorageDatabasesCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));
    controller->closeIdleLocalStorageDatabases();

    return JSValueMakeUndefined(context);
}

static JSValueRef grantWebNotificationPermissionCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));

    JSRetainPtr<JSStringRef> origin(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);
    controller->grantWebNotificationPermission(origin.get());

    return JSValueMakeUndefined(context);
}


static JSValueRef denyWebNotificationPermissionCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));

    JSRetainPtr<JSStringRef> origin(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    ASSERT(!*exception);
    controller->denyWebNotificationPermission(origin.get());

    return JSValueMakeUndefined(context);
}


static JSValueRef removeAllWebNotificationPermissionsCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));

    controller->removeAllWebNotificationPermissions();

    return JSValueMakeUndefined(context);
}


static JSValueRef simulateWebNotificationClickCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Has Windows implementation
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(thisObject));

    controller->simulateWebNotificationClick(arguments[0]);

    return JSValueMakeUndefined(context);
}

static JSValueRef numberOfDFGCompiles(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);
    
    return JSC::numberOfDFGCompiles(context, arguments[0]);
}

static JSValueRef neverInlineFunction(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);
    
    return JSC::setNeverInline(context, arguments[0]);
}

static void testRunnerObjectFinalize(JSObjectRef object)
{
    TestRunner* controller = static_cast<TestRunner*>(JSObjectGetPrivate(object));
    controller->deref();
}

// Object Creation

void TestRunner::makeWindowObject(JSContextRef context, JSObjectRef windowObject, JSValueRef* exception)
{
    JSRetainPtr<JSStringRef> testRunnerStr(Adopt, JSStringCreateWithUTF8CString("testRunner"));
    ref();

    JSClassRef classRef = getJSClass();
    JSValueRef layoutTestContollerObject = JSObjectMake(context, classRef, this);
    JSClassRelease(classRef);

    JSObjectSetProperty(context, windowObject, testRunnerStr.get(), layoutTestContollerObject, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete, exception);
}

JSClassRef TestRunner::getJSClass()
{
    static JSStaticValue* staticValues = TestRunner::staticValues();
    static JSStaticFunction* staticFunctions = TestRunner::staticFunctions();
    static JSClassDefinition classDefinition = {
        0, kJSClassAttributeNone, "TestRunner", 0, staticValues, staticFunctions,
        0, testRunnerObjectFinalize, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    return JSClassCreate(&classDefinition);
}

JSStaticValue* TestRunner::staticValues()
{
    static JSStaticValue staticValues[] = {
        { "globalFlag", getGlobalFlagCallback, setGlobalFlagCallback, kJSPropertyAttributeNone },
        { "webHistoryItemCount", getWebHistoryItemCountCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
#if PLATFORM(MAC) || PLATFORM(GTK) || PLATFORM(WIN) || PLATFORM(EFL)
        { "platformName", getPlatformNameCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
#endif
        { "secureEventInputIsEnabled", getSecureEventInputIsEnabledCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "titleTextDirection", getTitleTextDirectionCallback, 0, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { 0, 0, 0, 0 }
    };
    return staticValues;
}

JSStaticFunction* TestRunner::staticFunctions()
{
    static JSStaticFunction staticFunctions[] = {
        { "abortModal", abortModalCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "addDisallowedURL", addDisallowedURLCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "addURLToRedirect", addURLToRedirectCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "addUserScript", addUserScriptCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "addUserStyleSheet", addUserStyleSheetCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "apiTestNewWindowDataLoadBaseURL", apiTestNewWindowDataLoadBaseURLCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "apiTestGoToCurrentBackForwardItem", apiTestGoToCurrentBackForwardItemCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "applicationCacheDiskUsageForOrigin", applicationCacheDiskUsageForOriginCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "callShouldCloseOnWebView", callShouldCloseOnWebViewCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "clearAllApplicationCaches", clearAllApplicationCachesCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "clearAllDatabases", clearAllDatabasesCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "clearApplicationCacheForOrigin", clearApplicationCacheForOriginCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "clearBackForwardList", clearBackForwardListCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "clearPersistentUserStyleSheet", clearPersistentUserStyleSheetCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "closeWebInspector", closeWebInspectorCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "decodeHostName", decodeHostNameCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "disallowIncreaseForApplicationCacheQuota", disallowIncreaseForApplicationCacheQuotaCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "dispatchPendingLoadRequests", dispatchPendingLoadRequestsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "display", displayCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "displayInvalidatedRegion", displayInvalidatedRegionCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "dumpApplicationCacheDelegateCallbacks", dumpApplicationCacheDelegateCallbacksCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "dumpAsText", dumpAsTextCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "dumpBackForwardList", dumpBackForwardListCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "dumpChildFrameScrollPositions", dumpChildFrameScrollPositionsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "dumpChildFramesAsText", dumpChildFramesAsTextCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "dumpDOMAsWebArchive", dumpDOMAsWebArchiveCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "dumpDatabaseCallbacks", dumpDatabaseCallbacksCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "dumpEditingCallbacks", dumpEditingCallbacksCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "dumpFrameLoadCallbacks", dumpFrameLoadCallbacksCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "dumpProgressFinishedCallback", dumpProgressFinishedCallbackCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "dumpUserGestureInFrameLoadCallbacks", dumpUserGestureInFrameLoadCallbacksCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },        
        { "dumpResourceLoadCallbacks", dumpResourceLoadCallbacksCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "dumpResourceResponseMIMETypes", dumpResourceResponseMIMETypesCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "dumpSelectionRect", dumpSelectionRectCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "dumpSourceAsWebArchive", dumpSourceAsWebArchiveCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "dumpStatusCallbacks", dumpStatusCallbacksCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "dumpTitleChanges", dumpTitleChangesCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "dumpIconChanges", dumpIconChangesCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "dumpWillCacheResponse", dumpWillCacheResponseCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "encodeHostName", encodeHostNameCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "evaluateInWebInspector", evaluateInWebInspectorCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "evaluateScriptInIsolatedWorldAndReturnValue", evaluateScriptInIsolatedWorldAndReturnValueCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "evaluateScriptInIsolatedWorld", evaluateScriptInIsolatedWorldCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "execCommand", execCommandCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "findString", findStringCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "originsWithApplicationCache", originsWithApplicationCacheCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "goBack", goBackCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete }, 
        { "ignoreLegacyWebNotificationPermissionRequests", ignoreLegacyWebNotificationPermissionRequestsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "isCommandEnabled", isCommandEnabledCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "keepWebHistory", keepWebHistoryCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "numberOfPendingGeolocationPermissionRequests", numberOfPendingGeolocationPermissionRequestsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "notifyDone", notifyDoneCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "overridePreference", overridePreferenceCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "pathToLocalResource", pathToLocalResourceCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "printToPDF", dumpAsPDFCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "queueBackNavigation", queueBackNavigationCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "queueForwardNavigation", queueForwardNavigationCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "queueLoad", queueLoadCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "queueLoadHTMLString", queueLoadHTMLStringCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "queueLoadingScript", queueLoadingScriptCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "queueNonLoadingScript", queueNonLoadingScriptCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "queueReload", queueReloadCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "removeAllVisitedLinks", removeAllVisitedLinksCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "removeOriginAccessWhitelistEntry", removeOriginAccessWhitelistEntryCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "repaintSweepHorizontally", repaintSweepHorizontallyCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "resetPageVisibility", resetPageVisibilityCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setAcceptsEditing", setAcceptsEditingCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setAllowUniversalAccessFromFileURLs", setAllowUniversalAccessFromFileURLsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setAllowFileAccessFromFileURLs", setAllowFileAccessFromFileURLsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setAlwaysAcceptCookies", setAlwaysAcceptCookiesCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setAppCacheMaximumSize", setAppCacheMaximumSizeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setApplicationCacheOriginQuota", setApplicationCacheOriginQuotaCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setEncodedAudioData", setEncodedAudioDataCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setAuthenticationPassword", setAuthenticationPasswordCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setAuthenticationUsername", setAuthenticationUsernameCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setAuthorAndUserStylesEnabled", setAuthorAndUserStylesEnabledCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setCacheModel", setCacheModelCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setCallCloseOnWebViews", setCallCloseOnWebViewsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setCanOpenWindows", setCanOpenWindowsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setCloseRemainingWindowsWhenComplete", setCloseRemainingWindowsWhenCompleteCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setCustomPolicyDelegate", setCustomPolicyDelegateCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setDatabaseQuota", setDatabaseQuotaCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete }, 
        { "setDeferMainResourceDataLoad", setDeferMainResourceDataLoadCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setDefersLoading", setDefersLoadingCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setUseDeferredFrameLoading", setUseDeferredFrameLoadingCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setDomainRelaxationForbiddenForURLScheme", setDomainRelaxationForbiddenForURLSchemeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setGeolocationPermission", setGeolocationPermissionCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setHandlesAuthenticationChallenges", setHandlesAuthenticationChallengesCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setIconDatabaseEnabled", setIconDatabaseEnabledCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setAutomaticLinkDetectionEnabled", setAutomaticLinkDetectionEnabledCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setMainFrameIsFirstResponder", setMainFrameIsFirstResponderCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setMockDeviceOrientation", setMockDeviceOrientationCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setMockGeolocationPositionUnavailableError", setMockGeolocationPositionUnavailableErrorCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setMockGeolocationPosition", setMockGeolocationPositionCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "addMockSpeechInputResult", addMockSpeechInputResultCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setMockSpeechInputDumpRect", setMockSpeechInputDumpRectCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setNewWindowsCopyBackForwardList", setNewWindowsCopyBackForwardListCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setPageVisibility", setPageVisibilityCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setPOSIXLocale", setPOSIXLocaleCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setPersistentUserStyleSheetLocation", setPersistentUserStyleSheetLocationCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setPopupBlockingEnabled", setPopupBlockingEnabledCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setPluginsEnabled", setPluginsEnabledCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setPrinting", setPrintingCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setPrivateBrowsingEnabled", setPrivateBrowsingEnabledCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setSerializeHTTPLoads", setSerializeHTTPLoadsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setSpatialNavigationEnabled", setSpatialNavigationEnabledCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setStopProvisionalFrameLoads", setStopProvisionalFrameLoadsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setTabKeyCyclesThroughElements", setTabKeyCyclesThroughElementsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setUseDashboardCompatibilityMode", setUseDashboardCompatibilityModeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setUserStyleSheetEnabled", setUserStyleSheetEnabledCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setUserStyleSheetLocation", setUserStyleSheetLocationCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setValueForUser", setValueForUserCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setViewModeMediaFeature", setViewModeMediaFeatureCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setWebViewEditable", setWebViewEditableCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setWillSendRequestClearHeader", setWillSendRequestClearHeaderCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setWillSendRequestReturnsNull", setWillSendRequestReturnsNullCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setWillSendRequestReturnsNullOnRedirect", setWillSendRequestReturnsNullOnRedirectCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setWindowIsKey", setWindowIsKeyCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setJavaScriptCanAccessClipboard", setJavaScriptCanAccessClipboardCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setXSSAuditorEnabled", setXSSAuditorEnabledCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "showWebInspector", showWebInspectorCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "simulateLegacyWebNotificationClick", simulateLegacyWebNotificationClickCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "testOnscreen", testOnscreenCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "testRepaint", testRepaintCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "waitForPolicyDelegate", waitForPolicyDelegateCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "waitUntilDone", waitUntilDoneCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "windowCount", windowCountCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "addOriginAccessWhitelistEntry", addOriginAccessWhitelistEntryCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setScrollbarPolicy", setScrollbarPolicyCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "authenticateSession", authenticateSessionCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "deleteAllLocalStorage", deleteAllLocalStorageCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "syncLocalStorage", syncLocalStorageCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },                
        { "observeStorageTrackerNotifications", observeStorageTrackerNotificationsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },        
        { "deleteLocalStorageForOrigin", deleteLocalStorageForOriginCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "localStorageDiskUsageForOrigin", localStorageDiskUsageForOriginCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "originsWithLocalStorage", originsWithLocalStorageCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setShouldPaintBrokenImage", setShouldPaintBrokenImageCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setTextDirection", setTextDirectionCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setShouldStayOnPageAfterHandlingBeforeUnload", setShouldStayOnPageAfterHandlingBeforeUnloadCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "addChromeInputField", addChromeInputFieldCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "removeChromeInputField", removeChromeInputFieldCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "focusWebView", focusWebViewCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setBackingScaleFactor", setBackingScaleFactorCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "preciseTime", preciseTimeCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setHasCustomFullScreenBehavior", setHasCustomFullScreenBehaviorCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "setStorageDatabaseIdleInterval", setStorageDatabaseIdleIntervalCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "closeIdleLocalStorageDatabases", closeIdleLocalStorageDatabasesCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "grantWebNotificationPermission", grantWebNotificationPermissionCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "denyWebNotificationPermission", denyWebNotificationPermissionCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "removeAllWebNotificationPermissions", removeAllWebNotificationPermissionsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "simulateWebNotificationClick", simulateWebNotificationClickCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "numberOfDFGCompiles", numberOfDFGCompiles, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { "neverInlineFunction", neverInlineFunction, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
        { 0, 0, 0 }
    };

    return staticFunctions;
}

void TestRunner::queueLoadHTMLString(JSStringRef content, JSStringRef baseURL)
{
    WorkQueue::shared()->queue(new LoadHTMLStringItem(content, baseURL));
}

void TestRunner::queueLoadAlternateHTMLString(JSStringRef content, JSStringRef baseURL, JSStringRef unreachableURL)
{
    WorkQueue::shared()->queue(new LoadHTMLStringItem(content, baseURL, unreachableURL));
}

void TestRunner::queueBackNavigation(int howFarBack)
{
    WorkQueue::shared()->queue(new BackItem(howFarBack));
}

void TestRunner::queueForwardNavigation(int howFarForward)
{
    WorkQueue::shared()->queue(new ForwardItem(howFarForward));
}

void TestRunner::queueLoadingScript(JSStringRef script)
{
    WorkQueue::shared()->queue(new LoadingScriptItem(script));
}

void TestRunner::queueNonLoadingScript(JSStringRef script)
{
    WorkQueue::shared()->queue(new NonLoadingScriptItem(script));
}

void TestRunner::queueReload()
{
    WorkQueue::shared()->queue(new ReloadItem);
}

void TestRunner::ignoreLegacyWebNotificationPermissionRequests()
{
    m_areLegacyWebNotificationPermissionRequestsIgnored = false;
}

void TestRunner::waitToDumpWatchdogTimerFired()
{
    const char* message = "FAIL: Timed out waiting for notifyDone to be called\n";
    fprintf(stderr, "%s", message);
    fprintf(stdout, "%s", message);
    notifyDone();
}

void TestRunner::setGeolocationPermissionCommon(bool allow)
{
    m_isGeolocationPermissionSet = true;
    m_geolocationPermission = allow;
}

void TestRunner::setPOSIXLocale(JSStringRef locale)
{
    char localeBuf[32];
    JSStringGetUTF8CString(locale, localeBuf, sizeof(localeBuf));
    setlocale(LC_ALL, localeBuf);
}

void TestRunner::addURLToRedirect(std::string origin, std::string destination)
{
    m_URLsToRedirect[origin] = destination;
}

const std::string& TestRunner::redirectionDestinationForURL(std::string origin)
{
    return m_URLsToRedirect[origin];
}

void TestRunner::setShouldPaintBrokenImage(bool shouldPaintBrokenImage)
{
    m_shouldPaintBrokenImage = shouldPaintBrokenImage;
}
