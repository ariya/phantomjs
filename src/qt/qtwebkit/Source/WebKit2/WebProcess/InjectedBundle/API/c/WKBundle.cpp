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
#include "WKBundle.h"

#include "ImmutableArray.h"
#include "InjectedBundle.h"
#include "WKAPICast.h"
#include "WKBundleAPICast.h"
#include "WKBundlePrivate.h"
#include "WebData.h"

using namespace WebKit;

WKTypeID WKBundleGetTypeID()
{
    return toAPI(InjectedBundle::APIType);
}

void WKBundleSetClient(WKBundleRef bundleRef, WKBundleClient * wkClient)
{
    toImpl(bundleRef)->initializeClient(wkClient);
}

void WKBundlePostMessage(WKBundleRef bundleRef, WKStringRef messageNameRef, WKTypeRef messageBodyRef)
{
    toImpl(bundleRef)->postMessage(toWTFString(messageNameRef), toImpl(messageBodyRef));
}

void WKBundlePostSynchronousMessage(WKBundleRef bundleRef, WKStringRef messageNameRef, WKTypeRef messageBodyRef, WKTypeRef* returnDataRef)
{
    RefPtr<APIObject> returnData;
    toImpl(bundleRef)->postSynchronousMessage(toWTFString(messageNameRef), toImpl(messageBodyRef), returnData);
    if (returnDataRef)
        *returnDataRef = toAPI(returnData.release().leakRef());
}

WKConnectionRef WKBundleGetApplicationConnection(WKBundleRef bundleRef)
{
    return toAPI(toImpl(bundleRef)->webConnectionToUIProcess());
}

void WKBundleSetShouldTrackVisitedLinks(WKBundleRef bundleRef, bool shouldTrackVisitedLinks)
{
    toImpl(bundleRef)->setShouldTrackVisitedLinks(shouldTrackVisitedLinks);
}

void WKBundleRemoveAllVisitedLinks(WKBundleRef bundleRef)
{
    toImpl(bundleRef)->removeAllVisitedLinks();
}

void WKBundleActivateMacFontAscentHack(WKBundleRef bundleRef)
{
    toImpl(bundleRef)->activateMacFontAscentHack();
}

void WKBundleSetCacheModel(WKBundleRef bundleRef, uint32_t cacheModel)
{
    toImpl(bundleRef)->setCacheModel(cacheModel);
}

void WKBundleGarbageCollectJavaScriptObjects(WKBundleRef bundleRef)
{
    toImpl(bundleRef)->garbageCollectJavaScriptObjects();
}

void WKBundleGarbageCollectJavaScriptObjectsOnAlternateThreadForDebugging(WKBundleRef bundleRef, bool waitUntilDone)
{
    toImpl(bundleRef)->garbageCollectJavaScriptObjectsOnAlternateThreadForDebugging(waitUntilDone);
}

size_t WKBundleGetJavaScriptObjectsCount(WKBundleRef bundleRef)
{
    return toImpl(bundleRef)->javaScriptObjectsCount();
}

void WKBundleSetAlwaysAcceptCookies(WKBundleRef bundleRef, bool accept)
{
    toImpl(bundleRef)->setAlwaysAcceptCookies(accept);
}

void WKBundleAddUserScript(WKBundleRef bundleRef, WKBundlePageGroupRef pageGroupRef, WKBundleScriptWorldRef scriptWorldRef, WKStringRef sourceRef, WKURLRef urlRef, WKArrayRef whitelistRef, WKArrayRef blacklistRef, WKUserScriptInjectionTime injectionTimeRef, WKUserContentInjectedFrames injectedFramesRef)
{
    toImpl(bundleRef)->addUserScript(toImpl(pageGroupRef), toImpl(scriptWorldRef), toWTFString(sourceRef), toWTFString(urlRef), toImpl(whitelistRef), toImpl(blacklistRef), toUserScriptInjectionTime(injectionTimeRef), toUserContentInjectedFrames(injectedFramesRef));
}

void WKBundleAddUserStyleSheet(WKBundleRef bundleRef, WKBundlePageGroupRef pageGroupRef, WKBundleScriptWorldRef scriptWorldRef, WKStringRef sourceRef, WKURLRef urlRef, WKArrayRef whitelistRef, WKArrayRef blacklistRef, WKUserContentInjectedFrames injectedFramesRef)
{
    toImpl(bundleRef)->addUserStyleSheet(toImpl(pageGroupRef), toImpl(scriptWorldRef), toWTFString(sourceRef), toWTFString(urlRef), toImpl(whitelistRef), toImpl(blacklistRef), toUserContentInjectedFrames(injectedFramesRef));
}

void WKBundleRemoveUserScript(WKBundleRef bundleRef, WKBundlePageGroupRef pageGroupRef, WKBundleScriptWorldRef scriptWorldRef, WKURLRef urlRef)
{
    toImpl(bundleRef)->removeUserScript(toImpl(pageGroupRef), toImpl(scriptWorldRef), toWTFString(urlRef));
}

void WKBundleRemoveUserStyleSheet(WKBundleRef bundleRef, WKBundlePageGroupRef pageGroupRef, WKBundleScriptWorldRef scriptWorldRef, WKURLRef urlRef)
{
    toImpl(bundleRef)->removeUserStyleSheet(toImpl(pageGroupRef), toImpl(scriptWorldRef), toWTFString(urlRef));
}

void WKBundleRemoveUserScripts(WKBundleRef bundleRef, WKBundlePageGroupRef pageGroupRef, WKBundleScriptWorldRef scriptWorldRef)
{
    toImpl(bundleRef)->removeUserScripts(toImpl(pageGroupRef), toImpl(scriptWorldRef));
}

void WKBundleRemoveUserStyleSheets(WKBundleRef bundleRef, WKBundlePageGroupRef pageGroupRef, WKBundleScriptWorldRef scriptWorldRef)
{
    toImpl(bundleRef)->removeUserStyleSheets(toImpl(pageGroupRef), toImpl(scriptWorldRef));
}

void WKBundleRemoveAllUserContent(WKBundleRef bundleRef, WKBundlePageGroupRef pageGroupRef)
{
    toImpl(bundleRef)->removeAllUserContent(toImpl(pageGroupRef));
}

void WKBundleOverrideBoolPreferenceForTestRunner(WKBundleRef bundleRef, WKBundlePageGroupRef pageGroupRef, WKStringRef preference, bool enabled)
{
    toImpl(bundleRef)->overrideBoolPreferenceForTestRunner(toImpl(pageGroupRef), toWTFString(preference), enabled);
}

void WKBundleSetAllowUniversalAccessFromFileURLs(WKBundleRef bundleRef, WKBundlePageGroupRef pageGroupRef, bool enabled)
{
    toImpl(bundleRef)->setAllowUniversalAccessFromFileURLs(toImpl(pageGroupRef), enabled);
}

void WKBundleSetAllowFileAccessFromFileURLs(WKBundleRef bundleRef, WKBundlePageGroupRef pageGroupRef, bool enabled)
{
    toImpl(bundleRef)->setAllowFileAccessFromFileURLs(toImpl(pageGroupRef), enabled);
}

void WKBundleSetMinimumLogicalFontSize(WKBundleRef bundleRef, WKBundlePageGroupRef pageGroupRef, int size)
{
    toImpl(bundleRef)->setMinimumLogicalFontSize(toImpl(pageGroupRef), size);
}

void WKBundleSetFrameFlatteningEnabled(WKBundleRef bundleRef, WKBundlePageGroupRef pageGroupRef, bool enabled)
{
    toImpl(bundleRef)->setFrameFlatteningEnabled(toImpl(pageGroupRef), enabled);
}

void WKBundleSetPluginsEnabled(WKBundleRef bundleRef, WKBundlePageGroupRef pageGroupRef, bool enabled)
{
    toImpl(bundleRef)->setPluginsEnabled(toImpl(pageGroupRef), enabled);
}

void WKBundleSetJavaScriptCanAccessClipboard(WKBundleRef bundleRef, WKBundlePageGroupRef pageGroupRef, bool enabled)
{
    toImpl(bundleRef)->setJavaScriptCanAccessClipboard(toImpl(pageGroupRef), enabled);
}

void WKBundleSetPrivateBrowsingEnabled(WKBundleRef bundleRef, WKBundlePageGroupRef pageGroupRef, bool enabled)
{
    toImpl(bundleRef)->setPrivateBrowsingEnabled(toImpl(pageGroupRef), enabled);
}

void WKBundleSetPopupBlockingEnabled(WKBundleRef bundleRef, WKBundlePageGroupRef pageGroupRef, bool enabled)
{
    toImpl(bundleRef)->setPopupBlockingEnabled(toImpl(pageGroupRef), enabled);
}

void WKBundleSwitchNetworkLoaderToNewTestingSession(WKBundleRef bundleRef)
{
    toImpl(bundleRef)->switchNetworkLoaderToNewTestingSession();
}

void WKBundleSetAuthorAndUserStylesEnabled(WKBundleRef bundleRef, WKBundlePageGroupRef pageGroupRef, bool enabled)
{
    toImpl(bundleRef)->setAuthorAndUserStylesEnabled(toImpl(pageGroupRef), enabled);
}

void WKBundleSetSpatialNavigationEnabled(WKBundleRef bundleRef, WKBundlePageGroupRef pageGroupRef, bool enabled)
{
    toImpl(bundleRef)->setSpatialNavigationEnabled(toImpl(pageGroupRef), enabled);
}

void WKBundleAddOriginAccessWhitelistEntry(WKBundleRef bundleRef, WKStringRef sourceOrigin, WKStringRef destinationProtocol, WKStringRef destinationHost, bool allowDestinationSubdomains)
{
    toImpl(bundleRef)->addOriginAccessWhitelistEntry(toWTFString(sourceOrigin), toWTFString(destinationProtocol), toWTFString(destinationHost), allowDestinationSubdomains);
}

void WKBundleRemoveOriginAccessWhitelistEntry(WKBundleRef bundleRef, WKStringRef sourceOrigin, WKStringRef destinationProtocol, WKStringRef destinationHost, bool allowDestinationSubdomains)
{
    toImpl(bundleRef)->removeOriginAccessWhitelistEntry(toWTFString(sourceOrigin), toWTFString(destinationProtocol), toWTFString(destinationHost), allowDestinationSubdomains);
}

void WKBundleResetOriginAccessWhitelists(WKBundleRef bundleRef)
{
    toImpl(bundleRef)->resetOriginAccessWhitelists();
}

void WKBundleSetAsynchronousSpellCheckingEnabled(WKBundleRef bundleRef, WKBundlePageGroupRef pageGroupRef, bool enabled)
{
    toImpl(bundleRef)->setAsynchronousSpellCheckingEnabled(toImpl(pageGroupRef), enabled);
}

void WKBundleReportException(JSContextRef context, JSValueRef exception)
{
    InjectedBundle::reportException(context, exception);
}

void WKBundleClearAllDatabases(WKBundleRef bundleRef)
{
    toImpl(bundleRef)->clearAllDatabases();
}

void WKBundleSetDatabaseQuota(WKBundleRef bundleRef, uint64_t quota)
{
    toImpl(bundleRef)->setDatabaseQuota(quota);
}

void WKBundleClearApplicationCache(WKBundleRef bundleRef)
{
    toImpl(bundleRef)->clearApplicationCache();
}

void WKBundleClearApplicationCacheForOrigin(WKBundleRef bundleRef, WKStringRef origin)
{
    toImpl(bundleRef)->clearApplicationCacheForOrigin(toWTFString(origin));
}

void WKBundleSetAppCacheMaximumSize(WKBundleRef bundleRef, uint64_t size)
{
    toImpl(bundleRef)->setAppCacheMaximumSize(size);
}

uint64_t WKBundleGetAppCacheUsageForOrigin(WKBundleRef bundleRef, WKStringRef origin)
{
    return toImpl(bundleRef)->appCacheUsageForOrigin(toWTFString(origin));
}

void WKBundleSetApplicationCacheOriginQuota(WKBundleRef bundleRef, WKStringRef origin, uint64_t bytes)
{
    toImpl(bundleRef)->setApplicationCacheOriginQuota(toWTFString(origin), bytes);
}

void WKBundleResetApplicationCacheOriginQuota(WKBundleRef bundleRef, WKStringRef origin)
{
    toImpl(bundleRef)->resetApplicationCacheOriginQuota(toWTFString(origin));
}

WKArrayRef WKBundleCopyOriginsWithApplicationCache(WKBundleRef bundleRef)
{
    RefPtr<ImmutableArray> origins = toImpl(bundleRef)->originsWithApplicationCache();
    return toAPI(origins.release().leakRef());
}

WKDataRef WKBundleCreateWKDataFromUInt8Array(WKBundleRef bundle, JSContextRef context, JSValueRef data)
{
    RefPtr<WebData> webData = toImpl(bundle)->createWebDataFromUint8Array(context, data);
    return toAPI(webData.release().leakRef());
}

int WKBundleNumberOfPages(WKBundleRef bundleRef, WKBundleFrameRef frameRef, double pageWidthInPixels, double pageHeightInPixels)
{
    return toImpl(bundleRef)->numberOfPages(toImpl(frameRef), pageWidthInPixels, pageHeightInPixels);
}

int WKBundlePageNumberForElementById(WKBundleRef bundleRef, WKBundleFrameRef frameRef, WKStringRef idRef, double pageWidthInPixels, double pageHeightInPixels)
{
    return toImpl(bundleRef)->pageNumberForElementById(toImpl(frameRef), toWTFString(idRef), pageWidthInPixels, pageHeightInPixels);
}

WKStringRef WKBundlePageSizeAndMarginsInPixels(WKBundleRef bundleRef, WKBundleFrameRef frameRef, int pageIndex, int width, int height, int marginTop, int marginRight, int marginBottom, int marginLeft)
{
    return toCopiedAPI(toImpl(bundleRef)->pageSizeAndMarginsInPixels(toImpl(frameRef), pageIndex, width, height, marginTop, marginRight, marginBottom, marginLeft));
}

bool WKBundleIsPageBoxVisible(WKBundleRef bundleRef, WKBundleFrameRef frameRef, int pageIndex)
{
    return toImpl(bundleRef)->isPageBoxVisible(toImpl(frameRef), pageIndex);
}

bool WKBundleIsProcessingUserGesture(WKBundleRef)
{
    return InjectedBundle::isProcessingUserGesture();
}

void WKBundleSetUserStyleSheetLocation(WKBundleRef bundleRef, WKBundlePageGroupRef pageGroupRef, WKStringRef location)
{
    toImpl(bundleRef)->setUserStyleSheetLocation(toImpl(pageGroupRef), toWTFString(location));
}

void WKBundleSetWebNotificationPermission(WKBundleRef bundleRef, WKBundlePageRef pageRef, WKStringRef originStringRef, bool allowed)
{
    toImpl(bundleRef)->setWebNotificationPermission(toImpl(pageRef), toWTFString(originStringRef), allowed);
}

void WKBundleRemoveAllWebNotificationPermissions(WKBundleRef bundleRef, WKBundlePageRef pageRef)
{
    toImpl(bundleRef)->removeAllWebNotificationPermissions(toImpl(pageRef));
}

uint64_t WKBundleGetWebNotificationID(WKBundleRef bundleRef, JSContextRef context, JSValueRef notification)
{
    return toImpl(bundleRef)->webNotificationID(context, notification);
}

void WKBundleSetTabKeyCyclesThroughElements(WKBundleRef bundleRef, WKBundlePageRef pageRef, bool enabled)
{
    toImpl(bundleRef)->setTabKeyCyclesThroughElements(toImpl(pageRef), enabled);
}

void WKBundleSetSerialLoadingEnabled(WKBundleRef bundleRef, bool enabled)
{
    toImpl(bundleRef)->setSerialLoadingEnabled(enabled);
}

void WKBundleSetShadowDOMEnabled(WKBundleRef bundleRef, bool enabled)
{
    toImpl(bundleRef)->setShadowDOMEnabled(enabled);
}

void WKBundleSetSeamlessIFramesEnabled(WKBundleRef bundleRef, bool enabled)
{
    toImpl(bundleRef)->setSeamlessIFramesEnabled(enabled);
}

void WKBundleDispatchPendingLoadRequests(WKBundleRef bundleRef)
{
    toImpl(bundleRef)->dispatchPendingLoadRequests();
}
