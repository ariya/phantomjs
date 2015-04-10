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

#include "config.h"
#include "InjectedBundle.h"

#include "ActivateFonts.h"
#include "InjectedBundlePage.h"
#include "StringFunctions.h"
#include <WebKit2/WKBundle.h>
#include <WebKit2/WKBundlePage.h>
#include <WebKit2/WKBundlePagePrivate.h>
#include <WebKit2/WKBundlePrivate.h>
#include <WebKit2/WKRetainPtr.h>
#include <WebKit2/WebKit2_C.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/Vector.h>

namespace WTR {

InjectedBundle& InjectedBundle::shared()
{
    static InjectedBundle& shared = *new InjectedBundle;
    return shared;
}

InjectedBundle::InjectedBundle()
    : m_bundle(0)
    , m_topLoadingFrame(0)
    , m_state(Idle)
    , m_dumpPixels(false)
    , m_useWaitToDumpWatchdogTimer(true)
    , m_useWorkQueue(false)
    , m_timeout(0)
{
}

void InjectedBundle::didCreatePage(WKBundleRef bundle, WKBundlePageRef page, const void* clientInfo)
{
    static_cast<InjectedBundle*>(const_cast<void*>(clientInfo))->didCreatePage(page);
}

void InjectedBundle::willDestroyPage(WKBundleRef bundle, WKBundlePageRef page, const void* clientInfo)
{
    static_cast<InjectedBundle*>(const_cast<void*>(clientInfo))->willDestroyPage(page);
}

void InjectedBundle::didInitializePageGroup(WKBundleRef bundle, WKBundlePageGroupRef pageGroup, const void* clientInfo)
{
    static_cast<InjectedBundle*>(const_cast<void*>(clientInfo))->didInitializePageGroup(pageGroup);
}

void InjectedBundle::didReceiveMessage(WKBundleRef bundle, WKStringRef messageName, WKTypeRef messageBody, const void* clientInfo)
{
    static_cast<InjectedBundle*>(const_cast<void*>(clientInfo))->didReceiveMessage(messageName, messageBody);
}

void InjectedBundle::didReceiveMessageToPage(WKBundleRef bundle, WKBundlePageRef page, WKStringRef messageName, WKTypeRef messageBody, const void* clientInfo)
{
    static_cast<InjectedBundle*>(const_cast<void*>(clientInfo))->didReceiveMessageToPage(page, messageName, messageBody);
}

void InjectedBundle::initialize(WKBundleRef bundle, WKTypeRef initializationUserData)
{
    m_bundle = bundle;

    WKBundleClient client = {
        kWKBundleClientCurrentVersion,
        this,
        didCreatePage,
        willDestroyPage,
        didInitializePageGroup,
        didReceiveMessage,
        didReceiveMessageToPage
    };
    WKBundleSetClient(m_bundle, &client);

    platformInitialize(initializationUserData);

    activateFonts();
    WKBundleActivateMacFontAscentHack(m_bundle);

    // FIXME: We'd like to start with a clean state for every test, but this function can't be used more than once yet.
    WKBundleSwitchNetworkLoaderToNewTestingSession(m_bundle);
}

void InjectedBundle::didCreatePage(WKBundlePageRef page)
{
    m_pages.append(adoptPtr(new InjectedBundlePage(page)));
}

void InjectedBundle::willDestroyPage(WKBundlePageRef page)
{
    size_t size = m_pages.size();
    for (size_t i = 0; i < size; ++i) {
        if (m_pages[i]->page() == page) {
            m_pages.remove(i);
            break;
        }
    }
}

void InjectedBundle::didInitializePageGroup(WKBundlePageGroupRef pageGroup)
{
    m_pageGroup = pageGroup;
}

InjectedBundlePage* InjectedBundle::page() const
{
    // It might be better to have the UI process send over a reference to the main
    // page instead of just assuming it's the first one.
    return m_pages[0].get();
}

void InjectedBundle::resetLocalSettings()
{
    setlocale(LC_ALL, "");
}

void InjectedBundle::didReceiveMessage(WKStringRef messageName, WKTypeRef messageBody)
{
    if (WKStringIsEqualToUTF8CString(messageName, "BeginTest")) {
        ASSERT(messageBody);
        ASSERT(WKGetTypeID(messageBody) == WKDictionaryGetTypeID());
        WKDictionaryRef messageBodyDictionary = static_cast<WKDictionaryRef>(messageBody);

        WKRetainPtr<WKStringRef> dumpPixelsKey(AdoptWK, WKStringCreateWithUTF8CString("DumpPixels"));
        m_dumpPixels = WKBooleanGetValue(static_cast<WKBooleanRef>(WKDictionaryGetItemForKey(messageBodyDictionary, dumpPixelsKey.get())));

        WKRetainPtr<WKStringRef> useWaitToDumpWatchdogTimerKey(AdoptWK, WKStringCreateWithUTF8CString("UseWaitToDumpWatchdogTimer"));
        m_useWaitToDumpWatchdogTimer = WKBooleanGetValue(static_cast<WKBooleanRef>(WKDictionaryGetItemForKey(messageBodyDictionary, useWaitToDumpWatchdogTimerKey.get())));

        WKRetainPtr<WKStringRef> timeoutKey(AdoptWK, WKStringCreateWithUTF8CString("Timeout"));
        m_timeout = (int)WKUInt64GetValue(static_cast<WKUInt64Ref>(WKDictionaryGetItemForKey(messageBodyDictionary, timeoutKey.get())));

        WKRetainPtr<WKStringRef> ackMessageName(AdoptWK, WKStringCreateWithUTF8CString("Ack"));
        WKRetainPtr<WKStringRef> ackMessageBody(AdoptWK, WKStringCreateWithUTF8CString("BeginTest"));
        WKBundlePostMessage(m_bundle, ackMessageName.get(), ackMessageBody.get());

        beginTesting(messageBodyDictionary);
        return;
    } else if (WKStringIsEqualToUTF8CString(messageName, "Reset")) {
        ASSERT(messageBody);
        ASSERT(WKGetTypeID(messageBody) == WKDictionaryGetTypeID());
        WKDictionaryRef messageBodyDictionary = static_cast<WKDictionaryRef>(messageBody);

        WKRetainPtr<WKStringRef> shouldGCKey(AdoptWK, WKStringCreateWithUTF8CString("ShouldGC"));
        bool shouldGC = WKBooleanGetValue(static_cast<WKBooleanRef>(WKDictionaryGetItemForKey(messageBodyDictionary, shouldGCKey.get())));

        if (shouldGC)
            WKBundleGarbageCollectJavaScriptObjects(m_bundle);

        m_state = Idle;
        m_dumpPixels = false;

        resetLocalSettings();
        m_testRunner->removeAllWebNotificationPermissions();

        page()->resetAfterTest();

        return;
    }
    if (WKStringIsEqualToUTF8CString(messageName, "CallAddChromeInputFieldCallback")) {
        m_testRunner->callAddChromeInputFieldCallback();
        return;
    }
    if (WKStringIsEqualToUTF8CString(messageName, "CallRemoveChromeInputFieldCallback")) {
        m_testRunner->callRemoveChromeInputFieldCallback();
        return;
    }
    if (WKStringIsEqualToUTF8CString(messageName, "CallFocusWebViewCallback")) {
        m_testRunner->callFocusWebViewCallback();
        return;
    }
    if (WKStringIsEqualToUTF8CString(messageName, "CallSetBackingScaleFactorCallback")) {
        m_testRunner->callSetBackingScaleFactorCallback();
        return;
    }   
    if (WKStringIsEqualToUTF8CString(messageName, "WorkQueueProcessedCallback")) {
        if (!topLoadingFrame() && !m_testRunner->waitToDump())
            page()->dump();
        return;
    }

    WKRetainPtr<WKStringRef> errorMessageName(AdoptWK, WKStringCreateWithUTF8CString("Error"));
    WKRetainPtr<WKStringRef> errorMessageBody(AdoptWK, WKStringCreateWithUTF8CString("Unknown"));
    WKBundlePostMessage(m_bundle, errorMessageName.get(), errorMessageBody.get());
}

void InjectedBundle::didReceiveMessageToPage(WKBundlePageRef page, WKStringRef messageName, WKTypeRef messageBody)
{
    WKRetainPtr<WKStringRef> errorMessageName(AdoptWK, WKStringCreateWithUTF8CString("Error"));
    WKRetainPtr<WKStringRef> errorMessageBody(AdoptWK, WKStringCreateWithUTF8CString("Unknown"));
    WKBundlePostMessage(m_bundle, errorMessageName.get(), errorMessageBody.get());
}

bool InjectedBundle::booleanForKey(WKDictionaryRef dictionary, const char* key)
{
    WKRetainPtr<WKStringRef> wkKey(AdoptWK, WKStringCreateWithUTF8CString(key));
    WKTypeRef value = WKDictionaryGetItemForKey(dictionary, wkKey.get());
    if (WKGetTypeID(value) != WKBooleanGetTypeID()) {
        outputText(makeString("Boolean value for key", key, " not found in dictionary\n"));
        return false;
    }
    return WKBooleanGetValue(static_cast<WKBooleanRef>(value));
}

void InjectedBundle::beginTesting(WKDictionaryRef settings)
{
    m_state = Testing;

    m_pixelResult.clear();
    m_repaintRects.clear();

    m_testRunner = TestRunner::create();
    m_gcController = GCController::create();
    m_eventSendingController = EventSendingController::create();
    m_textInputController = TextInputController::create();
    m_accessibilityController = AccessibilityController::create();

    WKBundleSetShouldTrackVisitedLinks(m_bundle, false);
    WKBundleRemoveAllVisitedLinks(m_bundle);
    WKBundleSetAllowUniversalAccessFromFileURLs(m_bundle, m_pageGroup, true);
    WKBundleSetJavaScriptCanAccessClipboard(m_bundle, m_pageGroup, true);
    WKBundleSetPrivateBrowsingEnabled(m_bundle, m_pageGroup, false);
    WKBundleSetAuthorAndUserStylesEnabled(m_bundle, m_pageGroup, true);
    WKBundleSetFrameFlatteningEnabled(m_bundle, m_pageGroup, false);
    WKBundleSetMinimumLogicalFontSize(m_bundle, m_pageGroup, 9);
    WKBundleSetSpatialNavigationEnabled(m_bundle, m_pageGroup, false);
    WKBundleSetAllowFileAccessFromFileURLs(m_bundle, m_pageGroup, true);
    WKBundleSetPluginsEnabled(m_bundle, m_pageGroup, true);
    WKBundleSetPopupBlockingEnabled(m_bundle, m_pageGroup, false);
    WKBundleSetAlwaysAcceptCookies(m_bundle, false);
    WKBundleSetSerialLoadingEnabled(m_bundle, false);
    WKBundleSetShadowDOMEnabled(m_bundle, true);
    WKBundleSetSeamlessIFramesEnabled(m_bundle, true);
    WKBundleSetCacheModel(m_bundle, 1 /*CacheModelDocumentBrowser*/);

    WKBundleRemoveAllUserContent(m_bundle, m_pageGroup);

    m_testRunner->setShouldDumpFrameLoadCallbacks(booleanForKey(settings, "DumpFrameLoadDelegates"));
    m_testRunner->setUserStyleSheetEnabled(false);
    m_testRunner->setXSSAuditorEnabled(false);
    m_testRunner->setCloseRemainingWindowsWhenComplete(false);
    m_testRunner->setAcceptsEditing(true);
    m_testRunner->setTabKeyCyclesThroughElements(true);

    m_testRunner->setCustomTimeout(m_timeout);

    page()->prepare();

    WKBundleClearAllDatabases(m_bundle);
    WKBundleClearApplicationCache(m_bundle);
    WKBundleResetOriginAccessWhitelists(m_bundle);

    // [WK2] REGRESSION(r128623): It made layout tests extremely slow
    // https://bugs.webkit.org/show_bug.cgi?id=96862
    // WKBundleSetDatabaseQuota(m_bundle, 5 * 1024 * 1024);
}

void InjectedBundle::done()
{
    m_state = Stopping;

    m_useWorkQueue = false;

    page()->stopLoading();
    setTopLoadingFrame(0);

    m_accessibilityController->resetToConsistentState();

    WKRetainPtr<WKStringRef> doneMessageName(AdoptWK, WKStringCreateWithUTF8CString("Done"));
    WKRetainPtr<WKMutableDictionaryRef> doneMessageBody(AdoptWK, WKMutableDictionaryCreate());

    WKRetainPtr<WKStringRef> pixelResultKey = adoptWK(WKStringCreateWithUTF8CString("PixelResult"));
    WKDictionaryAddItem(doneMessageBody.get(), pixelResultKey.get(), m_pixelResult.get());

    WKRetainPtr<WKStringRef> repaintRectsKey = adoptWK(WKStringCreateWithUTF8CString("RepaintRects"));
    WKDictionaryAddItem(doneMessageBody.get(), repaintRectsKey.get(), m_repaintRects.get());

    WKRetainPtr<WKStringRef> audioResultKey = adoptWK(WKStringCreateWithUTF8CString("AudioResult"));
    WKDictionaryAddItem(doneMessageBody.get(), audioResultKey.get(), m_audioResult.get());

    WKBundlePostMessage(m_bundle, doneMessageName.get(), doneMessageBody.get());

    closeOtherPages();

    m_state = Idle;
}

void InjectedBundle::closeOtherPages()
{
    Vector<WKBundlePageRef> pagesToClose;
    size_t size = m_pages.size();
    for (size_t i = 1; i < size; ++i)
        pagesToClose.append(m_pages[i]->page());
    size = pagesToClose.size();
    for (size_t i = 0; i < size; ++i)
        WKBundlePageClose(pagesToClose[i]);
}

void InjectedBundle::dumpBackForwardListsForAllPages(StringBuilder& stringBuilder)
{
    size_t size = m_pages.size();
    for (size_t i = 0; i < size; ++i)
        m_pages[i]->dumpBackForwardList(stringBuilder);
}

void InjectedBundle::outputText(const String& output)
{
    if (m_state != Testing)
        return;
    if (output.isEmpty())
        return;
    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("TextOutput"));
    WKRetainPtr<WKStringRef> messageBody(AdoptWK, WKStringCreateWithUTF8CString(output.utf8().data()));
    WKBundlePostMessage(m_bundle, messageName.get(), messageBody.get());
}

void InjectedBundle::postNewBeforeUnloadReturnValue(bool value)
{
    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("BeforeUnloadReturnValue"));
    WKRetainPtr<WKBooleanRef> messageBody(AdoptWK, WKBooleanCreate(value));
    WKBundlePostMessage(m_bundle, messageName.get(), messageBody.get());
}

void InjectedBundle::postAddChromeInputField()
{
    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("AddChromeInputField"));
    WKBundlePostMessage(m_bundle, messageName.get(), 0);
}

void InjectedBundle::postRemoveChromeInputField()
{
    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("RemoveChromeInputField"));
    WKBundlePostMessage(m_bundle, messageName.get(), 0);
}

void InjectedBundle::postFocusWebView()
{
    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("FocusWebView"));
    WKBundlePostMessage(m_bundle, messageName.get(), 0);
}

void InjectedBundle::postSetBackingScaleFactor(double backingScaleFactor)
{
    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("SetBackingScaleFactor"));
    WKRetainPtr<WKDoubleRef> messageBody(AdoptWK, WKDoubleCreate(backingScaleFactor));
    WKBundlePostMessage(m_bundle, messageName.get(), messageBody.get());
}

void InjectedBundle::postSetWindowIsKey(bool isKey)
{
    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("SetWindowIsKey"));
    WKRetainPtr<WKBooleanRef> messageBody(AdoptWK, WKBooleanCreate(isKey));
    WKBundlePostSynchronousMessage(m_bundle, messageName.get(), messageBody.get(), 0);
}

void InjectedBundle::postSimulateWebNotificationClick(uint64_t notificationID)
{
    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("SimulateWebNotificationClick"));
    WKRetainPtr<WKUInt64Ref> messageBody(AdoptWK, WKUInt64Create(notificationID));
    WKBundlePostMessage(m_bundle, messageName.get(), messageBody.get());
}

void InjectedBundle::setGeolocationPermission(bool enabled)
{
    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("SetGeolocationPermission"));
    WKRetainPtr<WKBooleanRef> messageBody(AdoptWK, WKBooleanCreate(enabled));
    WKBundlePostMessage(m_bundle, messageName.get(), messageBody.get());
}

void InjectedBundle::setMockGeolocationPosition(double latitude, double longitude, double accuracy, bool providesAltitude, double altitude, bool providesAltitudeAccuracy, double altitudeAccuracy, bool providesHeading, double heading, bool providesSpeed, double speed)
{
    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("SetMockGeolocationPosition"));

    WKRetainPtr<WKMutableDictionaryRef> messageBody(AdoptWK, WKMutableDictionaryCreate());

    WKRetainPtr<WKStringRef> latitudeKeyWK(AdoptWK, WKStringCreateWithUTF8CString("latitude"));
    WKRetainPtr<WKDoubleRef> latitudeWK(AdoptWK, WKDoubleCreate(latitude));
    WKDictionaryAddItem(messageBody.get(), latitudeKeyWK.get(), latitudeWK.get());

    WKRetainPtr<WKStringRef> longitudeKeyWK(AdoptWK, WKStringCreateWithUTF8CString("longitude"));
    WKRetainPtr<WKDoubleRef> longitudeWK(AdoptWK, WKDoubleCreate(longitude));
    WKDictionaryAddItem(messageBody.get(), longitudeKeyWK.get(), longitudeWK.get());

    WKRetainPtr<WKStringRef> accuracyKeyWK(AdoptWK, WKStringCreateWithUTF8CString("accuracy"));
    WKRetainPtr<WKDoubleRef> accuracyWK(AdoptWK, WKDoubleCreate(accuracy));
    WKDictionaryAddItem(messageBody.get(), accuracyKeyWK.get(), accuracyWK.get());

    WKRetainPtr<WKStringRef> providesAltitudeKeyWK(AdoptWK, WKStringCreateWithUTF8CString("providesAltitude"));
    WKRetainPtr<WKBooleanRef> providesAltitudeWK(AdoptWK, WKBooleanCreate(providesAltitude));
    WKDictionaryAddItem(messageBody.get(), providesAltitudeKeyWK.get(), providesAltitudeWK.get());

    WKRetainPtr<WKStringRef> altitudeKeyWK(AdoptWK, WKStringCreateWithUTF8CString("altitude"));
    WKRetainPtr<WKDoubleRef> altitudeWK(AdoptWK, WKDoubleCreate(altitude));
    WKDictionaryAddItem(messageBody.get(), altitudeKeyWK.get(), altitudeWK.get());

    WKRetainPtr<WKStringRef> providesAltitudeAccuracyKeyWK(AdoptWK, WKStringCreateWithUTF8CString("providesAltitudeAccuracy"));
    WKRetainPtr<WKBooleanRef> providesAltitudeAccuracyWK(AdoptWK, WKBooleanCreate(providesAltitudeAccuracy));
    WKDictionaryAddItem(messageBody.get(), providesAltitudeAccuracyKeyWK.get(), providesAltitudeAccuracyWK.get());

    WKRetainPtr<WKStringRef> altitudeAccuracyKeyWK(AdoptWK, WKStringCreateWithUTF8CString("altitudeAccuracy"));
    WKRetainPtr<WKDoubleRef> altitudeAccuracyWK(AdoptWK, WKDoubleCreate(altitudeAccuracy));
    WKDictionaryAddItem(messageBody.get(), altitudeAccuracyKeyWK.get(), altitudeAccuracyWK.get());

    WKRetainPtr<WKStringRef> providesHeadingKeyWK(AdoptWK, WKStringCreateWithUTF8CString("providesHeading"));
    WKRetainPtr<WKBooleanRef> providesHeadingWK(AdoptWK, WKBooleanCreate(providesHeading));
    WKDictionaryAddItem(messageBody.get(), providesHeadingKeyWK.get(), providesHeadingWK.get());

    WKRetainPtr<WKStringRef> headingKeyWK(AdoptWK, WKStringCreateWithUTF8CString("heading"));
    WKRetainPtr<WKDoubleRef> headingWK(AdoptWK, WKDoubleCreate(heading));
    WKDictionaryAddItem(messageBody.get(), headingKeyWK.get(), headingWK.get());

    WKRetainPtr<WKStringRef> providesSpeedKeyWK(AdoptWK, WKStringCreateWithUTF8CString("providesSpeed"));
    WKRetainPtr<WKBooleanRef> providesSpeedWK(AdoptWK, WKBooleanCreate(providesSpeed));
    WKDictionaryAddItem(messageBody.get(), providesSpeedKeyWK.get(), providesSpeedWK.get());

    WKRetainPtr<WKStringRef> speedKeyWK(AdoptWK, WKStringCreateWithUTF8CString("speed"));
    WKRetainPtr<WKDoubleRef> speedWK(AdoptWK, WKDoubleCreate(speed));
    WKDictionaryAddItem(messageBody.get(), speedKeyWK.get(), speedWK.get());

    WKBundlePostMessage(m_bundle, messageName.get(), messageBody.get());
}

void InjectedBundle::setMockGeolocationPositionUnavailableError(WKStringRef errorMessage)
{
    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("SetMockGeolocationPositionUnavailableError"));
    WKBundlePostMessage(m_bundle, messageName.get(), errorMessage);
}

void InjectedBundle::setCustomPolicyDelegate(bool enabled, bool permissive)
{
    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("SetCustomPolicyDelegate"));

    WKRetainPtr<WKMutableDictionaryRef> messageBody(AdoptWK, WKMutableDictionaryCreate());

    WKRetainPtr<WKStringRef> enabledKeyWK(AdoptWK, WKStringCreateWithUTF8CString("enabled"));
    WKRetainPtr<WKBooleanRef> enabledWK(AdoptWK, WKBooleanCreate(enabled));
    WKDictionaryAddItem(messageBody.get(), enabledKeyWK.get(), enabledWK.get());

    WKRetainPtr<WKStringRef> permissiveKeyWK(AdoptWK, WKStringCreateWithUTF8CString("permissive"));
    WKRetainPtr<WKBooleanRef> permissiveWK(AdoptWK, WKBooleanCreate(permissive));
    WKDictionaryAddItem(messageBody.get(), permissiveKeyWK.get(), permissiveWK.get());

    WKBundlePostMessage(m_bundle, messageName.get(), messageBody.get());
}

void InjectedBundle::setVisibilityState(WKPageVisibilityState visibilityState, bool isInitialState)
{
    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("SetVisibilityState"));
    WKRetainPtr<WKMutableDictionaryRef> messageBody(AdoptWK, WKMutableDictionaryCreate());

    WKRetainPtr<WKStringRef> visibilityStateKeyWK(AdoptWK, WKStringCreateWithUTF8CString("visibilityState"));
    WKRetainPtr<WKUInt64Ref> visibilityStateWK(AdoptWK, WKUInt64Create(visibilityState));
    WKDictionaryAddItem(messageBody.get(), visibilityStateKeyWK.get(), visibilityStateWK.get());

    WKRetainPtr<WKStringRef> isInitialKeyWK(AdoptWK, WKStringCreateWithUTF8CString("isInitialState"));
    WKRetainPtr<WKBooleanRef> isInitialWK(AdoptWK, WKBooleanCreate(isInitialState));
    WKDictionaryAddItem(messageBody.get(), isInitialKeyWK.get(), isInitialWK.get());

    WKBundlePostMessage(m_bundle, messageName.get(), messageBody.get());
}

bool InjectedBundle::shouldProcessWorkQueue() const
{
    if (!m_useWorkQueue)
        return false;

    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("IsWorkQueueEmpty"));
    WKTypeRef resultToPass = 0;
    WKBundlePostSynchronousMessage(m_bundle, messageName.get(), 0, &resultToPass);
    WKRetainPtr<WKBooleanRef> isEmpty(AdoptWK, static_cast<WKBooleanRef>(resultToPass));

    return !WKBooleanGetValue(isEmpty.get());
}

void InjectedBundle::processWorkQueue()
{
    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("ProcessWorkQueue"));
    WKBundlePostMessage(m_bundle, messageName.get(), 0);
}

void InjectedBundle::queueBackNavigation(unsigned howFarBackward)
{
    m_useWorkQueue = true;

    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("QueueBackNavigation"));
    WKRetainPtr<WKUInt64Ref> messageBody(AdoptWK, WKUInt64Create(howFarBackward));
    WKBundlePostMessage(m_bundle, messageName.get(), messageBody.get());
}

void InjectedBundle::queueForwardNavigation(unsigned howFarForward)
{
    m_useWorkQueue = true;

    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("QueueForwardNavigation"));
    WKRetainPtr<WKUInt64Ref> messageBody(AdoptWK, WKUInt64Create(howFarForward));
    WKBundlePostMessage(m_bundle, messageName.get(), messageBody.get());
}

void InjectedBundle::queueLoad(WKStringRef url, WKStringRef target)
{
    m_useWorkQueue = true;

    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("QueueLoad"));

    WKRetainPtr<WKMutableDictionaryRef> loadData(AdoptWK, WKMutableDictionaryCreate());

    WKRetainPtr<WKStringRef> urlKey(AdoptWK, WKStringCreateWithUTF8CString("url"));
    WKDictionaryAddItem(loadData.get(), urlKey.get(), url);

    WKRetainPtr<WKStringRef> targetKey(AdoptWK, WKStringCreateWithUTF8CString("target"));
    WKDictionaryAddItem(loadData.get(), targetKey.get(), target);

    WKBundlePostMessage(m_bundle, messageName.get(), loadData.get());
}

void InjectedBundle::queueLoadHTMLString(WKStringRef content, WKStringRef baseURL, WKStringRef unreachableURL)
{
    m_useWorkQueue = true;

    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("QueueLoadHTMLString"));

    WKRetainPtr<WKMutableDictionaryRef> loadData(AdoptWK, WKMutableDictionaryCreate());

    WKRetainPtr<WKStringRef> contentKey(AdoptWK, WKStringCreateWithUTF8CString("content"));
    WKDictionaryAddItem(loadData.get(), contentKey.get(), content);

    if (baseURL) {
        WKRetainPtr<WKStringRef> baseURLKey(AdoptWK, WKStringCreateWithUTF8CString("baseURL"));
        WKDictionaryAddItem(loadData.get(), baseURLKey.get(), baseURL);
    }

    if (unreachableURL) {
        WKRetainPtr<WKStringRef> unreachableURLKey(AdoptWK, WKStringCreateWithUTF8CString("unreachableURL"));
        WKDictionaryAddItem(loadData.get(), unreachableURLKey.get(), unreachableURL);
    }

    WKBundlePostMessage(m_bundle, messageName.get(), loadData.get());
}

void InjectedBundle::queueReload()
{
    m_useWorkQueue = true;

    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("QueueReload"));
    WKBundlePostMessage(m_bundle, messageName.get(), 0);
}

void InjectedBundle::queueLoadingScript(WKStringRef script)
{
    m_useWorkQueue = true;

    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("QueueLoadingScript"));
    WKBundlePostMessage(m_bundle, messageName.get(), script);
}

void InjectedBundle::queueNonLoadingScript(WKStringRef script)
{
    m_useWorkQueue = true;

    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString("QueueNonLoadingScript"));
    WKBundlePostMessage(m_bundle, messageName.get(), script);
}

} // namespace WTR
