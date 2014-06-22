/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef WKContext_h
#define WKContext_h

#include <WebKit2/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    kWKCacheModelDocumentViewer = 0,
    kWKCacheModelDocumentBrowser = 1,
    kWKCacheModelPrimaryWebBrowser = 2
};
typedef uint32_t WKCacheModel;

// Context Client
typedef void (*WKContextPlugInAutoStartOriginHashesChangedCallback)(WKContextRef context, const void *clientInfo);
typedef void (*WKContextNetworkProcessDidCrashCallback)(WKContextRef context, const void *clientInfo);
typedef void (*WKContextPlugInInformationBecameAvailableCallback)(WKContextRef context, WKArrayRef plugIn, const void *clientInfo);

struct WKContextClient {
    int                                                                 version;
    const void *                                                        clientInfo;

    // Version 0.
    WKContextPlugInAutoStartOriginHashesChangedCallback                 plugInAutoStartOriginHashesChanged;
    WKContextNetworkProcessDidCrashCallback                             networkProcessDidCrash;
    WKContextPlugInInformationBecameAvailableCallback                   plugInInformationBecameAvailable;
};
typedef struct WKContextClient WKContextClient;

enum { kWKContextClientCurrentVersion = 0 };

// Injected Bundle Client
typedef void (*WKContextDidReceiveMessageFromInjectedBundleCallback)(WKContextRef page, WKStringRef messageName, WKTypeRef messageBody, const void *clientInfo);
typedef void (*WKContextDidReceiveSynchronousMessageFromInjectedBundleCallback)(WKContextRef page, WKStringRef messageName, WKTypeRef messageBody, WKTypeRef* returnData, const void *clientInfo);
typedef WKTypeRef (*WKContextGetInjectedBundleInitializationUserDataCallback)(WKContextRef context, const void *clientInfo);

struct WKContextInjectedBundleClient {
    int                                                                 version;
    const void *                                                        clientInfo;

    // Version 0.
    WKContextDidReceiveMessageFromInjectedBundleCallback                didReceiveMessageFromInjectedBundle;
    WKContextDidReceiveSynchronousMessageFromInjectedBundleCallback     didReceiveSynchronousMessageFromInjectedBundle;

    // Version 1.
    WKContextGetInjectedBundleInitializationUserDataCallback            getInjectedBundleInitializationUserData;
};
typedef struct WKContextInjectedBundleClient WKContextInjectedBundleClient;

enum { kWKContextInjectedBundleClientCurrentVersion = 1 };

// History Client
typedef void (*WKContextDidNavigateWithNavigationDataCallback)(WKContextRef context, WKPageRef page, WKNavigationDataRef navigationData, WKFrameRef frame, const void *clientInfo);
typedef void (*WKContextDidPerformClientRedirectCallback)(WKContextRef context, WKPageRef page, WKURLRef sourceURL, WKURLRef destinationURL, WKFrameRef frame, const void *clientInfo);
typedef void (*WKContextDidPerformServerRedirectCallback)(WKContextRef context, WKPageRef page, WKURLRef sourceURL, WKURLRef destinationURL, WKFrameRef frame, const void *clientInfo);
typedef void (*WKContextDidUpdateHistoryTitleCallback)(WKContextRef context, WKPageRef page, WKStringRef title, WKURLRef URL, WKFrameRef frame, const void *clientInfo);
typedef void (*WKContextPopulateVisitedLinksCallback)(WKContextRef context, const void *clientInfo);

struct WKContextHistoryClient {
    int                                                                 version;
    const void *                                                        clientInfo;
    WKContextDidNavigateWithNavigationDataCallback                      didNavigateWithNavigationData;
    WKContextDidPerformClientRedirectCallback                           didPerformClientRedirect;
    WKContextDidPerformServerRedirectCallback                           didPerformServerRedirect;
    WKContextDidUpdateHistoryTitleCallback                              didUpdateHistoryTitle;
    WKContextPopulateVisitedLinksCallback                               populateVisitedLinks;
};
typedef struct WKContextHistoryClient WKContextHistoryClient;

enum { kWKContextHistoryClientCurrentVersion = 0 };

// Download Client
typedef void (*WKContextDownloadDidStartCallback)(WKContextRef context, WKDownloadRef download, const void *clientInfo);
typedef void (*WKContextDownloadDidReceiveAuthenticationChallengeCallback)(WKContextRef context, WKDownloadRef download, WKAuthenticationChallengeRef authenticationChallenge, const void *clientInfo);
typedef void (*WKContextDownloadDidReceiveResponseCallback)(WKContextRef context, WKDownloadRef download, WKURLResponseRef response, const void *clientInfo);
typedef void (*WKContextDownloadDidReceiveDataCallback)(WKContextRef context, WKDownloadRef download, uint64_t length, const void *clientInfo);
typedef bool (*WKContextDownloadShouldDecodeSourceDataOfMIMETypeCallback)(WKContextRef context, WKDownloadRef download, WKStringRef mimeType, const void *clientInfo);
typedef WKStringRef (*WKContextDownloadDecideDestinationWithSuggestedFilenameCallback)(WKContextRef context, WKDownloadRef download, WKStringRef filename, bool* allowOverwrite, const void *clientInfo);
typedef void (*WKContextDownloadDidCreateDestinationCallback)(WKContextRef context, WKDownloadRef download, WKStringRef path, const void *clientInfo);
typedef void (*WKContextDownloadDidFinishCallback)(WKContextRef context, WKDownloadRef download, const void *clientInfo);
typedef void (*WKContextDownloadDidFailCallback)(WKContextRef context, WKDownloadRef download, WKErrorRef error, const void *clientInfo);
typedef void (*WKContextDownloadDidCancel)(WKContextRef context, WKDownloadRef download, const void *clientInfo);
typedef void (*WKContextDownloadProcessDidCrashCallback)(WKContextRef context, WKDownloadRef download, const void *clientInfo);

struct WKContextDownloadClient {
    int                                                                 version;
    const void *                                                        clientInfo;
    WKContextDownloadDidStartCallback                                   didStart;
    WKContextDownloadDidReceiveAuthenticationChallengeCallback          didReceiveAuthenticationChallenge;
    WKContextDownloadDidReceiveResponseCallback                         didReceiveResponse;
    WKContextDownloadDidReceiveDataCallback                             didReceiveData;
    WKContextDownloadShouldDecodeSourceDataOfMIMETypeCallback           shouldDecodeSourceDataOfMIMEType;
    WKContextDownloadDecideDestinationWithSuggestedFilenameCallback     decideDestinationWithSuggestedFilename;
    WKContextDownloadDidCreateDestinationCallback                       didCreateDestination;
    WKContextDownloadDidFinishCallback                                  didFinish;
    WKContextDownloadDidFailCallback                                    didFail;
    WKContextDownloadDidCancel                                          didCancel;
    WKContextDownloadProcessDidCrashCallback                            processDidCrash;
};
typedef struct WKContextDownloadClient WKContextDownloadClient;

enum { kWKContextDownloadClientCurrentVersion = 0 };

// Connection Client
typedef void (*WKContextDidCreateConnection)(WKContextRef context, WKConnectionRef connection, const void* clientInfo);

struct WKContextConnectionClient {
    int                                                                 version;
    const void *                                                        clientInfo;
    WKContextDidCreateConnection                                        didCreateConnection;
};
typedef struct WKContextConnectionClient WKContextConnectionClient;

enum { kWKContextConnectionClientCurrentVersion = 0 };

enum {
    kWKProcessModelSharedSecondaryProcess = 0,
    kWKProcessModelMultipleSecondaryProcesses = 1
};
typedef uint32_t WKProcessModel;

enum {
    kWKStatisticsOptionsWebContent = 1 << 0,
    kWKStatisticsOptionsNetworking = 1 << 1
};
typedef uint32_t WKStatisticsOptions;

WK_EXPORT WKTypeID WKContextGetTypeID();

WK_EXPORT WKContextRef WKContextCreate();
WK_EXPORT WKContextRef WKContextCreateWithInjectedBundlePath(WKStringRef path);

WK_EXPORT void WKContextSetClient(WKContextRef context, const WKContextClient* client);
WK_EXPORT void WKContextSetInjectedBundleClient(WKContextRef context, const WKContextInjectedBundleClient* client);
WK_EXPORT void WKContextSetHistoryClient(WKContextRef context, const WKContextHistoryClient* client);
WK_EXPORT void WKContextSetDownloadClient(WKContextRef context, const WKContextDownloadClient* client);
WK_EXPORT void WKContextSetConnectionClient(WKContextRef context, const WKContextConnectionClient* client);

WK_EXPORT WKDownloadRef WKContextDownloadURLRequest(WKContextRef context, const WKURLRequestRef request);

WK_EXPORT void WKContextSetInitializationUserDataForInjectedBundle(WKContextRef context, WKTypeRef userData);
WK_EXPORT void WKContextPostMessageToInjectedBundle(WKContextRef context, WKStringRef messageName, WKTypeRef messageBody);

WK_EXPORT void WKContextAddVisitedLink(WKContextRef context, WKStringRef visitedURL);

WK_EXPORT void WKContextSetCacheModel(WKContextRef context, WKCacheModel cacheModel);
WK_EXPORT WKCacheModel WKContextGetCacheModel(WKContextRef context);

WK_EXPORT void WKContextSetProcessModel(WKContextRef context, WKProcessModel processModel);
WK_EXPORT WKProcessModel WKContextGetProcessModel(WKContextRef context);

WK_EXPORT void WKContextSetMaximumNumberOfProcesses(WKContextRef context, unsigned numberOfProcesses);
WK_EXPORT unsigned WKContextGetMaximumNumberOfProcesses(WKContextRef context);

WK_EXPORT void WKContextStartMemorySampler(WKContextRef context, WKDoubleRef interval);
WK_EXPORT void WKContextStopMemorySampler(WKContextRef context);

WK_EXPORT WKApplicationCacheManagerRef WKContextGetApplicationCacheManager(WKContextRef context);
WK_EXPORT WKBatteryManagerRef WKContextGetBatteryManager(WKContextRef context);
WK_EXPORT WKCookieManagerRef WKContextGetCookieManager(WKContextRef context);
WK_EXPORT WKDatabaseManagerRef WKContextGetDatabaseManager(WKContextRef context);
WK_EXPORT WKGeolocationManagerRef WKContextGetGeolocationManager(WKContextRef context);
WK_EXPORT WKIconDatabaseRef WKContextGetIconDatabase(WKContextRef context);
WK_EXPORT WKKeyValueStorageManagerRef WKContextGetKeyValueStorageManager(WKContextRef context);
WK_EXPORT WKMediaCacheManagerRef WKContextGetMediaCacheManager(WKContextRef context);
WK_EXPORT WKNetworkInfoManagerRef WKContextGetNetworkInfoManager(WKContextRef context);
WK_EXPORT WKNotificationManagerRef WKContextGetNotificationManager(WKContextRef context);
WK_EXPORT WKPluginSiteDataManagerRef WKContextGetPluginSiteDataManager(WKContextRef context);
WK_EXPORT WKResourceCacheManagerRef WKContextGetResourceCacheManager(WKContextRef context);
    
typedef void (*WKContextGetStatisticsFunction)(WKDictionaryRef statistics, WKErrorRef error, void* functionContext);
WK_EXPORT void WKContextGetStatistics(WKContextRef context, void* functionContext, WKContextGetStatisticsFunction function);
WK_EXPORT void WKContextGetStatisticsWithOptions(WKContextRef context, WKStatisticsOptions statisticsMask, void* functionContext, WKContextGetStatisticsFunction function);

WK_EXPORT void WKContextGarbageCollectJavaScriptObjects(WKContextRef context);
WK_EXPORT void WKContextSetJavaScriptGarbageCollectorTimerEnabled(WKContextRef context, bool enable);

WK_EXPORT WKDictionaryRef WKContextCopyPlugInAutoStartOriginHashes(WKContextRef context);
WK_EXPORT void WKContextSetPlugInAutoStartOriginHashes(WKContextRef context, WKDictionaryRef dictionary);
WK_EXPORT void WKContextSetPlugInAutoStartOrigins(WKContextRef contextRef, WKArrayRef arrayRef);

#ifdef __cplusplus
}
#endif

#endif /* WKContext_h */
