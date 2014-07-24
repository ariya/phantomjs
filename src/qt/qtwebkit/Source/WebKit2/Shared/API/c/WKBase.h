/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
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

#ifndef WKBase_h
#define WKBase_h

#include <stdint.h>

#if defined(BUILDING_GTK__)
#include <WebKit2/WKBaseGtk.h>
#endif

#if defined(WTF_USE_SOUP)
#include <WebKit2/WKBaseSoup.h>
#endif

#if defined(BUILDING_EFL__)
#include <WebKit2/WKBaseEfl.h>
#endif

#if defined(__APPLE__) && !defined(BUILDING_QT__)
#include <WebKit2/WKBaseMac.h>
#endif

/* WebKit2 shared types */

typedef uint32_t WKTypeID;
typedef const void* WKTypeRef;

typedef const struct OpaqueWKArray* WKArrayRef;
typedef struct OpaqueWKArray* WKMutableArrayRef;

typedef const struct OpaqueWKDictionary* WKDictionaryRef;
typedef struct OpaqueWKDictionary* WKMutableDictionaryRef;

typedef const struct OpaqueWKBoolean* WKBooleanRef;
typedef const struct OpaqueWKCertificateInfo* WKCertificateInfoRef;
typedef const struct OpaqueWKConnection* WKConnectionRef;
typedef const struct OpaqueWKContextMenuItem* WKContextMenuItemRef;
typedef const struct OpaqueWKData* WKDataRef;
typedef const struct OpaqueWKDouble* WKDoubleRef;
typedef const struct OpaqueWKError* WKErrorRef;
typedef const struct OpaqueWKGraphicsContext* WKGraphicsContextRef;
typedef const struct OpaqueWKImage* WKImageRef;
typedef const struct OpaqueWKPointRef* WKPointRef;
typedef const struct OpaqueWKRectRef* WKRectRef;
typedef const struct OpaqueWKRenderLayer* WKRenderLayerRef;
typedef const struct OpaqueWKRenderObject* WKRenderObjectRef;
typedef const struct OpaqueWKSecurityOrigin* WKSecurityOriginRef;
typedef const struct OpaqueWKSerializedScriptValue* WKSerializedScriptValueRef;
typedef const struct OpaqueWKSizeRef* WKSizeRef;
typedef const struct OpaqueWKString* WKStringRef;
typedef const struct OpaqueWKUInt64* WKUInt64Ref;
typedef const struct OpaqueWKURL* WKURLRef;
typedef const struct OpaqueWKURLRequest* WKURLRequestRef;
typedef const struct OpaqueWKURLResponse* WKURLResponseRef;
typedef const struct OpaqueWKUserContentURLPattern* WKUserContentURLPatternRef;
typedef const struct OpaqueWKWebArchive* WKWebArchiveRef;
typedef const struct OpaqueWKWebArchiveResource* WKWebArchiveResourceRef;

enum WKUserContentInjectedFrames {
    kWKInjectInAllFrames,
    kWKInjectInTopFrameOnly
};
typedef enum WKUserContentInjectedFrames WKUserContentInjectedFrames;

enum WKUserScriptInjectionTime {
    kWKInjectAtDocumentStart,
    kWKInjectAtDocumentEnd
};
typedef enum WKUserScriptInjectionTime WKUserScriptInjectionTime;

/* WebKit2 main API types */

typedef const struct OpaqueWKApplicationCacheManager* WKApplicationCacheManagerRef;
typedef const struct OpaqueWKAuthenticationChallenge* WKAuthenticationChallengeRef;
typedef const struct OpaqueWKAuthenticationDecisionListener* WKAuthenticationDecisionListenerRef;
typedef const struct OpaqueWKBackForwardList* WKBackForwardListRef;
typedef const struct OpaqueWKBackForwardListItem* WKBackForwardListItemRef;
typedef const struct OpaqueWKBatteryManager* WKBatteryManagerRef;
typedef const struct OpaqueWKBatteryStatus* WKBatteryStatusRef;
typedef const struct OpaqueWKResourceCacheManager* WKResourceCacheManagerRef;
typedef const struct OpaqueWKColorPickerResultListener* WKColorPickerResultListenerRef;
typedef const struct OpaqueWKContext* WKContextRef;
typedef const struct OpaqueWKCookieManager* WKCookieManagerRef;
typedef const struct OpaqueWKCredential* WKCredentialRef;
typedef const struct OpaqueWKDatabaseManager* WKDatabaseManagerRef;
typedef const struct OpaqueWKDownload* WKDownloadRef;
typedef const struct OpaqueWKFormSubmissionListener* WKFormSubmissionListenerRef;
typedef const struct OpaqueWKFrame* WKFrameRef;
typedef const struct OpaqueWKFramePolicyListener* WKFramePolicyListenerRef;
typedef const struct OpaqueWKGeolocationManager* WKGeolocationManagerRef;
typedef const struct OpaqueWKGeolocationPermissionRequest* WKGeolocationPermissionRequestRef;
typedef const struct OpaqueWKGeolocationPosition* WKGeolocationPositionRef;
typedef const struct OpaqueWKGrammarDetail* WKGrammarDetailRef;
typedef const struct OpaqueWKHitTestResult* WKHitTestResultRef;
typedef const struct OpaqueWKIconDatabase* WKIconDatabaseRef;
typedef const struct OpaqueWKInspector* WKInspectorRef;
typedef const struct OpaqueWKKeyValueStorageManager* WKKeyValueStorageManagerRef;
typedef const struct OpaqueWKMediaCacheManager* WKMediaCacheManagerRef;
typedef const struct OpaqueWKNavigationData* WKNavigationDataRef;
typedef const struct OpaqueWKNetworkInfoManager* WKNetworkInfoManagerRef;
typedef const struct OpaqueWKNetworkInfo* WKNetworkInfoRef;
typedef const struct OpaqueWKNotification* WKNotificationRef;
typedef const struct OpaqueWKNotificationManager* WKNotificationManagerRef;
typedef const struct OpaqueWKNotificationPermissionRequest* WKNotificationPermissionRequestRef;
typedef const struct OpaqueWKNotificationProvider* WKNotificationProviderRef;
typedef const struct OpaqueWKOpenPanelParameters* WKOpenPanelParametersRef;
typedef const struct OpaqueWKOpenPanelResultListener* WKOpenPanelResultListenerRef;
typedef const struct OpaqueWKPage* WKPageRef;
typedef const struct OpaqueWKPageGroup* WKPageGroupRef;
typedef const struct OpaqueWKPluginSiteDataManager* WKPluginSiteDataManagerRef;
typedef const struct OpaqueWKPreferences* WKPreferencesRef;
typedef const struct OpaqueWKProtectionSpace* WKProtectionSpaceRef;
typedef const struct OpaqueWKTextChecker* WKTextCheckerRef;
typedef const struct OpaqueWKVibration* WKVibrationRef;
typedef const struct OpaqueWKViewportAttributes* WKViewportAttributesRef;

/* WebKit2 Bundle types */

typedef const struct OpaqueWKBundle* WKBundleRef;
typedef const struct OpaqueWKBundleBackForwardList* WKBundleBackForwardListRef;
typedef const struct OpaqueWKBundleBackForwardListItem* WKBundleBackForwardListItemRef;
typedef const struct OpaqueWKBundleDOMCSSStyleDeclaration* WKBundleCSSStyleDeclarationRef;
typedef const struct OpaqueWKBundleDOMWindowExtension* WKBundleDOMWindowExtensionRef;
typedef const struct OpaqueWKBundleFrame* WKBundleFrameRef;
typedef const struct OpaqueWKBundleHitTestResult* WKBundleHitTestResultRef;
typedef const struct OpaqueWKBundleInspector* WKBundleInspectorRef;
typedef const struct OpaqueWKBundleNavigationAction* WKBundleNavigationActionRef;
typedef const struct OpaqueWKBundleNodeHandle* WKBundleNodeHandleRef;
typedef const struct OpaqueWKBundlePage* WKBundlePageRef;
typedef const struct OpaqueWKBundlePageBanner* WKBundlePageBannerRef;
typedef const struct OpaqueWKBundlePageGroup* WKBundlePageGroupRef;
typedef const struct OpaqueWKBundlePageOverlay* WKBundlePageOverlayRef;
typedef const struct OpaqueWKBundleRangeHandle* WKBundleRangeHandleRef;
typedef const struct OpaqueWKBundleScriptWorld* WKBundleScriptWorldRef;

#undef WK_EXPORT
#if defined(WK_NO_EXPORT)
#define WK_EXPORT
#elif defined(__GNUC__) && !defined(__CC_ARM) && !defined(__ARMCC__)
#define WK_EXPORT __attribute__((visibility("default")))
#elif defined(WIN32) || defined(_WIN32) || defined(_WIN32_WCE) || defined(__CC_ARM) || defined(__ARMCC__)
#if BUILDING_WEBKIT
#define WK_EXPORT __declspec(dllexport)
#else
#define WK_EXPORT __declspec(dllimport)
#endif
#else /* !defined(WK_NO_EXPORT) */
#define WK_EXPORT
#endif /* defined(WK_NO_EXPORT) */

#if !defined(WK_INLINE)
#if defined(__cplusplus)
#define WK_INLINE static inline
#elif defined(__GNUC__)
#define WK_INLINE static __inline__
#elif defined(__WIN32__)
#define WK_INLINE static __inline
#else
#define WK_INLINE static    
#endif
#endif /* !defined(WK_INLINE) */

#endif /* WKBase_h */
