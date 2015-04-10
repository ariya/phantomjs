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
#include "WKContext.h"
#include "WKContextPrivate.h"

#include "WKAPICast.h"
#include "WebContext.h"
#include "WebURLRequest.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

// Supplements
#include "WebApplicationCacheManagerProxy.h"
#include "WebCookieManagerProxy.h"
#include "WebGeolocationManagerProxy.h"
#include "WebKeyValueStorageManager.h"
#include "WebMediaCacheManagerProxy.h"
#include "WebNotificationManagerProxy.h"
#include "WebResourceCacheManagerProxy.h"
#if ENABLE(SQL_DATABASE)
#include "WebDatabaseManagerProxy.h"
#endif
#if ENABLE(BATTERY_STATUS)
#include "WebBatteryManagerProxy.h"
#endif
#if ENABLE(NETWORK_INFO)
#include "WebNetworkInfoManagerProxy.h"
#endif

using namespace WebKit;

extern "C" {
// For binary compatibility with Safari 5.1. Should be removed eventually.
WK_EXPORT void _WKContextSetAdditionalPluginsDirectory(WKContextRef context, WKStringRef pluginsDirectory);
WK_EXPORT void _WKContextRegisterURLSchemeAsEmptyDocument(WKContextRef context, WKStringRef urlScheme);
WK_EXPORT void _WKContextSetAlwaysUsesComplexTextCodePath(WKContextRef context, bool alwaysUseComplexTextCodePath);
WK_EXPORT void _WKContextSetHTTPPipeliningEnabled(WKContextRef context, bool enabled);
}

WKTypeID WKContextGetTypeID()
{
    return toAPI(WebContext::APIType);
}

WKContextRef WKContextCreate()
{
    RefPtr<WebContext> context = WebContext::create(String());
    return toAPI(context.release().leakRef());
}

WKContextRef WKContextCreateWithInjectedBundlePath(WKStringRef pathRef)
{
    RefPtr<WebContext> context = WebContext::create(toImpl(pathRef)->string());
    return toAPI(context.release().leakRef());
}

void WKContextSetClient(WKContextRef contextRef, const WKContextClient* wkClient)
{
    toImpl(contextRef)->initializeClient(wkClient);
}

void WKContextSetInjectedBundleClient(WKContextRef contextRef, const WKContextInjectedBundleClient* wkClient)
{
    toImpl(contextRef)->initializeInjectedBundleClient(wkClient);
}

void WKContextSetHistoryClient(WKContextRef contextRef, const WKContextHistoryClient* wkClient)
{
    toImpl(contextRef)->initializeHistoryClient(wkClient);
}

void WKContextSetDownloadClient(WKContextRef contextRef, const WKContextDownloadClient* wkClient)
{
    toImpl(contextRef)->initializeDownloadClient(wkClient);
}

void WKContextSetConnectionClient(WKContextRef contextRef, const WKContextConnectionClient* wkClient)
{
    toImpl(contextRef)->initializeConnectionClient(wkClient);
}

WKDownloadRef WKContextDownloadURLRequest(WKContextRef contextRef, const WKURLRequestRef requestRef)
{
    return toAPI(toImpl(contextRef)->download(0, toImpl(requestRef)->resourceRequest()));
}

void WKContextSetInitializationUserDataForInjectedBundle(WKContextRef contextRef,  WKTypeRef userDataRef)
{
    toImpl(contextRef)->setInjectedBundleInitializationUserData(toImpl(userDataRef));
}

void WKContextPostMessageToInjectedBundle(WKContextRef contextRef, WKStringRef messageNameRef, WKTypeRef messageBodyRef)
{
    toImpl(contextRef)->postMessageToInjectedBundle(toImpl(messageNameRef)->string(), toImpl(messageBodyRef));
}

void WKContextGetGlobalStatistics(WKContextStatistics* statistics)
{
    const WebContext::Statistics& webContextStatistics = WebContext::statistics();

    statistics->wkViewCount = webContextStatistics.wkViewCount;
    statistics->wkPageCount = webContextStatistics.wkPageCount;
    statistics->wkFrameCount = webContextStatistics.wkFrameCount;
}

void WKContextAddVisitedLink(WKContextRef contextRef, WKStringRef visitedURL)
{
    toImpl(contextRef)->addVisitedLink(toImpl(visitedURL)->string());
}

void WKContextSetCacheModel(WKContextRef contextRef, WKCacheModel cacheModel)
{
    toImpl(contextRef)->setCacheModel(toCacheModel(cacheModel));
}

WKCacheModel WKContextGetCacheModel(WKContextRef contextRef)
{
    return toAPI(toImpl(contextRef)->cacheModel());
}

void WKContextSetProcessModel(WKContextRef contextRef, WKProcessModel processModel)
{
    toImpl(contextRef)->setProcessModel(toProcessModel(processModel));
}

WKProcessModel WKContextGetProcessModel(WKContextRef contextRef)
{
    return toAPI(toImpl(contextRef)->processModel());
}

void WKContextSetMaximumNumberOfProcesses(WKContextRef contextRef, unsigned numberOfProcesses)
{
    toImpl(contextRef)->setMaximumNumberOfProcesses(numberOfProcesses);
}

unsigned WKContextGetMaximumNumberOfProcesses(WKContextRef contextRef)
{
    return toImpl(contextRef)->maximumNumberOfProcesses();
}

void WKContextSetAlwaysUsesComplexTextCodePath(WKContextRef contextRef, bool alwaysUseComplexTextCodePath)
{
    toImpl(contextRef)->setAlwaysUsesComplexTextCodePath(alwaysUseComplexTextCodePath);
}

void WKContextSetShouldUseFontSmoothing(WKContextRef contextRef, bool useFontSmoothing)
{
    toImpl(contextRef)->setShouldUseFontSmoothing(useFontSmoothing);
}

void WKContextSetAdditionalPluginsDirectory(WKContextRef contextRef, WKStringRef pluginsDirectory)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    toImpl(contextRef)->setAdditionalPluginsDirectory(toImpl(pluginsDirectory)->string());
#else
    UNUSED_PARAM(contextRef);
    UNUSED_PARAM(pluginsDirectory);
#endif
}

void WKContextRegisterURLSchemeAsEmptyDocument(WKContextRef contextRef, WKStringRef urlScheme)
{
    toImpl(contextRef)->registerURLSchemeAsEmptyDocument(toImpl(urlScheme)->string());
}

void WKContextRegisterURLSchemeAsSecure(WKContextRef contextRef, WKStringRef urlScheme)
{
    toImpl(contextRef)->registerURLSchemeAsSecure(toImpl(urlScheme)->string());
}

void WKContextSetDomainRelaxationForbiddenForURLScheme(WKContextRef contextRef, WKStringRef urlScheme)
{
    toImpl(contextRef)->setDomainRelaxationForbiddenForURLScheme(toImpl(urlScheme)->string());
}

WKCookieManagerRef WKContextGetCookieManager(WKContextRef contextRef)
{
    return toAPI(toImpl(contextRef)->supplement<WebCookieManagerProxy>());
}

WKApplicationCacheManagerRef WKContextGetApplicationCacheManager(WKContextRef contextRef)
{
    return toAPI(toImpl(contextRef)->supplement<WebApplicationCacheManagerProxy>());
}

WKBatteryManagerRef WKContextGetBatteryManager(WKContextRef contextRef)
{
#if ENABLE(BATTERY_STATUS)
    return toAPI(toImpl(contextRef)->supplement<WebBatteryManagerProxy>());
#else
    return 0;
#endif
}

WKDatabaseManagerRef WKContextGetDatabaseManager(WKContextRef contextRef)
{
#if ENABLE(SQL_DATABASE)
    return toAPI(toImpl(contextRef)->supplement<WebDatabaseManagerProxy>());
#else
    return 0;
#endif
}

WKGeolocationManagerRef WKContextGetGeolocationManager(WKContextRef contextRef)
{
    return toAPI(toImpl(contextRef)->supplement<WebGeolocationManagerProxy>());
}

WKNetworkInfoManagerRef WKContextGetNetworkInfoManager(WKContextRef contextRef)
{
#if ENABLE(NETWORK_INFO)
    return toAPI(toImpl(contextRef)->supplement<WebNetworkInfoManagerProxy>());
#else
    return 0;
#endif
}

WKIconDatabaseRef WKContextGetIconDatabase(WKContextRef contextRef)
{
    return toAPI(toImpl(contextRef)->iconDatabase());
}

WKKeyValueStorageManagerRef WKContextGetKeyValueStorageManager(WKContextRef contextRef)
{
    return toAPI(toImpl(contextRef)->supplement<WebKeyValueStorageManager>());
}

WKMediaCacheManagerRef WKContextGetMediaCacheManager(WKContextRef contextRef)
{
    return toAPI(toImpl(contextRef)->supplement<WebMediaCacheManagerProxy>());
}

WKNotificationManagerRef WKContextGetNotificationManager(WKContextRef contextRef)
{
    return toAPI(toImpl(contextRef)->supplement<WebNotificationManagerProxy>());
}

WKPluginSiteDataManagerRef WKContextGetPluginSiteDataManager(WKContextRef contextRef)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    return toAPI(toImpl(contextRef)->pluginSiteDataManager());
#else
    return 0;
#endif
}

WKResourceCacheManagerRef WKContextGetResourceCacheManager(WKContextRef contextRef)
{
    return toAPI(toImpl(contextRef)->supplement<WebResourceCacheManagerProxy>());
}

void WKContextStartMemorySampler(WKContextRef contextRef, WKDoubleRef interval)
{
    toImpl(contextRef)->startMemorySampler(toImpl(interval)->value());
}

void WKContextStopMemorySampler(WKContextRef contextRef)
{
    toImpl(contextRef)->stopMemorySampler();
}

void WKContextSetIconDatabasePath(WKContextRef contextRef, WKStringRef iconDatabasePath)
{
    toImpl(contextRef)->setIconDatabasePath(toImpl(iconDatabasePath)->string());
}

void WKContextAllowSpecificHTTPSCertificateForHost(WKContextRef contextRef, WKCertificateInfoRef certificateRef, WKStringRef hostRef)
{
    toImpl(contextRef)->allowSpecificHTTPSCertificateForHost(toImpl(certificateRef), toImpl(hostRef)->string());
}

WK_EXPORT void WKContextSetApplicationCacheDirectory(WKContextRef contextRef, WKStringRef applicationCacheDirectory)
{
    toImpl(contextRef)->setApplicationCacheDirectory(toImpl(applicationCacheDirectory)->string());
}

void WKContextSetDatabaseDirectory(WKContextRef contextRef, WKStringRef databaseDirectory)
{
    toImpl(contextRef)->setDatabaseDirectory(toImpl(databaseDirectory)->string());
}

void WKContextSetLocalStorageDirectory(WKContextRef contextRef, WKStringRef localStorageDirectory)
{
    toImpl(contextRef)->setLocalStorageDirectory(toImpl(localStorageDirectory)->string());
}

WK_EXPORT void WKContextSetDiskCacheDirectory(WKContextRef contextRef, WKStringRef diskCacheDirectory)
{
    toImpl(contextRef)->setDiskCacheDirectory(toImpl(diskCacheDirectory)->string());
}

WK_EXPORT void WKContextSetCookieStorageDirectory(WKContextRef contextRef, WKStringRef cookieStorageDirectory)
{
    toImpl(contextRef)->setCookieStorageDirectory(toImpl(cookieStorageDirectory)->string());
}

void WKContextDisableProcessTermination(WKContextRef contextRef)
{
    toImpl(contextRef)->disableProcessTermination();
}

void WKContextEnableProcessTermination(WKContextRef contextRef)
{
    toImpl(contextRef)->enableProcessTermination();
}

void WKContextSetHTTPPipeliningEnabled(WKContextRef contextRef, bool enabled)
{
    toImpl(contextRef)->setHTTPPipeliningEnabled(enabled);
}

void WKContextWarmInitialProcess(WKContextRef contextRef)
{
    toImpl(contextRef)->warmInitialProcess();
}

void WKContextGetStatistics(WKContextRef contextRef, void* context, WKContextGetStatisticsFunction callback)
{
    toImpl(contextRef)->getStatistics(0xFFFFFFFF, DictionaryCallback::create(context, callback));
}

void WKContextGetStatisticsWithOptions(WKContextRef contextRef, WKStatisticsOptions optionsMask, void* context, WKContextGetStatisticsFunction callback)
{
    toImpl(contextRef)->getStatistics(optionsMask, DictionaryCallback::create(context, callback));
}

void WKContextGarbageCollectJavaScriptObjects(WKContextRef contextRef)
{
    toImpl(contextRef)->garbageCollectJavaScriptObjects();
}

void WKContextSetJavaScriptGarbageCollectorTimerEnabled(WKContextRef contextRef, bool enable)
{
    toImpl(contextRef)->setJavaScriptGarbageCollectorTimerEnabled(enable);
}

void WKContextSetUsesNetworkProcess(WKContextRef contextRef, bool usesNetworkProcess)
{
    toImpl(contextRef)->setUsesNetworkProcess(usesNetworkProcess);
}

WKDictionaryRef WKContextCopyPlugInAutoStartOriginHashes(WKContextRef contextRef)
{
    return toAPI(toImpl(contextRef)->plugInAutoStartOriginHashes().leakRef());
}

void WKContextSetPlugInAutoStartOriginHashes(WKContextRef contextRef, WKDictionaryRef dictionaryRef)
{
    if (!dictionaryRef)
        return;
    toImpl(contextRef)->setPlugInAutoStartOriginHashes(*toImpl(dictionaryRef));
}

void WKContextSetPlugInAutoStartOrigins(WKContextRef contextRef, WKArrayRef arrayRef)
{
    if (!arrayRef)
        return;
    toImpl(contextRef)->setPlugInAutoStartOrigins(*toImpl(arrayRef));
}

void WKContextSetInvalidMessageFunction(WKContextInvalidMessageFunction invalidMessageFunction)
{
    WebContext::setInvalidMessageCallback(invalidMessageFunction);
}

// Deprecated functions.
void _WKContextSetAdditionalPluginsDirectory(WKContextRef context, WKStringRef pluginsDirectory)
{
    WKContextSetAdditionalPluginsDirectory(context, pluginsDirectory);
}

void _WKContextRegisterURLSchemeAsEmptyDocument(WKContextRef context, WKStringRef urlScheme)
{
    WKContextRegisterURLSchemeAsEmptyDocument(context, urlScheme);
}

void _WKContextSetAlwaysUsesComplexTextCodePath(WKContextRef context, bool alwaysUseComplexTextCodePath)
{
    WKContextSetAlwaysUsesComplexTextCodePath(context, alwaysUseComplexTextCodePath);
}

void _WKContextSetHTTPPipeliningEnabled(WKContextRef context, bool enabled)
{
    WKContextSetHTTPPipeliningEnabled(context, enabled);
}
