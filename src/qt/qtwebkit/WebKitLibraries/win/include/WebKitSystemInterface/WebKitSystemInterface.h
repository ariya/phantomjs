/*
 * Copyright (C) 2007, 2008, 2009 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef WebKitSystemInterface_h
#define WebKitSystemInterface_h

struct CGAffineTransform;
struct CGPoint;
struct CGRect;
struct CGSize;
struct IDirect3DDevice9;
struct WKCACFContext;
struct WKCACFUpdateRectEnumerator;

typedef struct _CACFLayer* CACFLayerRef;
typedef const struct __CFArray* CFArrayRef;
typedef const struct __CFData* CFDataRef;
typedef const struct __CFString* CFStringRef;
typedef double CFTimeInterval;
typedef struct CGColor* CGColorRef;
typedef struct CGContext* CGContextRef;
typedef unsigned short CGFontIndex;
typedef struct CGFont* CGFontRef;
typedef CGFontIndex CGGlyph;
typedef struct CGImage* CGImageRef;
typedef struct CGPattern* CGPatternRef;
typedef wchar_t UChar;
typedef struct _CFURLResponse* CFURLResponseRef;
typedef struct OpaqueCFHTTPCookieStorage*  CFHTTPCookieStorageRef;
typedef struct __CFDictionary* CFMutableDictionaryRef;
typedef struct _CFURLRequest* CFMutableURLRequestRef;
typedef const struct _CFURLRequest* CFURLRequestRef;
typedef struct __CFHTTPMessage* CFHTTPMessageRef;
typedef const struct __CFNumber* CFNumberRef;
typedef struct __CFReadStream* CFReadStreamRef;
typedef const struct __CFURL* CFURLRef;
typedef struct _CFURLProtectionSpace* CFURLProtectionSpaceRef;
typedef struct tagLOGFONTW LOGFONTW;
typedef LOGFONTW LOGFONT;
typedef struct _CACFLayer *CACFLayerRef;
typedef struct __CVBuffer *CVBufferRef;
typedef CVBufferRef CVImageBufferRef;
typedef CVImageBufferRef CVPixelBufferRef;
typedef struct _CAImageQueue *CAImageQueueRef;
typedef unsigned long CFTypeID;
typedef struct _CFURLCredential* WKCFURLCredentialRef;
typedef const struct __CFURLStorageSession* CFURLStorageSessionRef;
typedef const struct _CFURLCache* CFURLCacheRef;

void wkSetFontSmoothingLevel(int type);
int wkGetFontSmoothingLevel();
void wkSetFontSmoothingContrast(CGFloat);
CGFloat wkGetFontSmoothingContrast();
void wkSystemFontSmoothingChanged();
uint32_t wkSetFontSmoothingStyle(CGContextRef cg, bool fontAllowsSmoothing);
void wkRestoreFontSmoothingStyle(CGContextRef cg, uint32_t oldStyle);
void wkSetCGContextFontRenderingStyle(CGContextRef, bool isSystemFont, bool isPrinterFont, bool usePlatformNativeGlyphs);
void wkGetGlyphAdvances(CGFontRef, const CGAffineTransform&, bool isSystemFont, bool isPrinterFont, CGGlyph, CGSize& advance);
void wkGetGlyphs(CGFontRef, const UChar[], CGGlyph[], size_t count);
void wkSetUpFontCache(size_t s);

void wkSetBaseCTM(CGContextRef, CGAffineTransform);
void wkSetPatternPhaseInUserSpace(CGContextRef, CGPoint phasePoint);
CGAffineTransform wkGetUserToBaseCTM(CGContextRef);

void wkDrawFocusRing(CGContextRef, CGColorRef, float radius);

CFDictionaryRef wkGetSSLCertificateInfo(CFURLResponseRef);
CFDataRef wkGetSSLPeerCertificateData(CFDictionaryRef);
void* wkGetSSLPeerCertificateDataBytePtr(CFDictionaryRef);
void wkSetSSLPeerCertificateData(CFMutableDictionaryRef, CFDataRef);
void* wkGetSSLCertificateChainContext(CFDictionaryRef);
CFHTTPCookieStorageRef wkGetDefaultHTTPCookieStorage();
CFHTTPCookieStorageRef wkCreateInMemoryHTTPCookieStorage();
void wkSetCFURLRequestShouldContentSniff(CFMutableURLRequestRef, bool);
CFStringRef wkCopyFoundationCacheDirectory(CFURLStorageSessionRef);
void wkSetClientCertificateInSSLProperties(CFMutableDictionaryRef, CFDataRef);

CFArrayRef wkCFURLRequestCopyHTTPRequestBodyParts(CFURLRequestRef);
void wkCFURLRequestSetHTTPRequestBodyParts(CFMutableURLRequestRef, CFArrayRef bodyParts);

CFURLStorageSessionRef wkCreatePrivateStorageSession(CFStringRef identifier, CFURLStorageSessionRef defaultStorageSession);
void wkSetRequestStorageSession(CFURLStorageSessionRef, CFMutableURLRequestRef);
CFURLCacheRef wkCopyURLCache(CFURLStorageSessionRef);
CFHTTPCookieStorageRef wkCopyHTTPCookieStorage(CFURLStorageSessionRef);
CFDataRef wkCopySerializedDefaultStorageSession();
CFURLStorageSessionRef wkDeserializeStorageSession(CFDataRef);

CFArrayRef wkCFURLCacheCopyAllHostNamesInPersistentStore();
void wkCFURLCacheDeleteHostNamesInPersistentStore(CFArrayRef hostNames);

unsigned wkInitializeMaximumHTTPConnectionCountPerHost(unsigned preferredConnectionCount);
int wkGetHTTPPipeliningPriority(CFURLRequestRef);
void wkSetHTTPPipeliningMaximumPriority(int maximumPriority);
void wkSetHTTPPipeliningPriority(CFURLRequestRef, int priority);

void wkSetCONNECTProxyForStream(CFReadStreamRef, CFStringRef proxyHost, CFNumberRef proxyPort);
void wkSetCONNECTProxyAuthorizationForStream(CFReadStreamRef, CFStringRef proxyAuthorizationString);
CFHTTPMessageRef wkCopyCONNECTProxyResponse(CFReadStreamRef, CFURLRef responseURL, CFStringRef proxyHost, CFNumberRef proxyPort);

WKCFURLCredentialRef wkCopyCredentialFromCFPersistentStorage(CFURLProtectionSpaceRef protectionSpace);

CFStringRef wkCFNetworkErrorGetLocalizedDescription(CFIndex errorCode);


enum wkCAImageQueueFlags {
    kWKCAImageQueueAsync = 1U << 0,
    kWKCAImageQueueFill = 1U << 1,
    kWKCAImageQueueProtected = 1U << 2,
    kWKCAImageQueueUseCleanAperture = 1U << 3,
    kWKCAImageQueueUseAspectRatio = 1U << 4,
    kWKCAImageQueueLowQualityColor = 1U << 5,
};

enum wkWKCAImageQueueImageType {
    kWKCAImageQueueNil = 1,
    kWKCAImageQueueSurface,
    kWKCAImageQueueBuffer,
    kWKCAImageQueueIOSurface,
};

enum wkWKCAImageQueueImageFlags {
    kWKCAImageQueueOpaque = 1U << 0,
    kWKCAImageQueueFlush = 1U << 1,
    kWKCAImageQueueWillFlush = 1U << 2,
    kWKCAImageQueueFlipped = 1U << 3,
    kWKCAImageQueueWaitGPU = 1U << 4,
};

typedef void (*wkCAImageQueueReleaseCallback)(unsigned int type, uint64_t id, void *info);
CAImageQueueRef wkCAImageQueueCreate(uint32_t width, uint32_t height, uint32_t capacity);
void wkCAImageQueueInvalidate(CAImageQueueRef iq);
size_t wkCAImageQueueCollect(CAImageQueueRef iq);
bool wkCAImageQueueInsertImage(CAImageQueueRef iq, CFTimeInterval t, unsigned int type, uint64_t id, uint32_t flags, wkCAImageQueueReleaseCallback release, void *info);
uint64_t wkCAImageQueueRegisterPixelBuffer(CAImageQueueRef iq, void *data, size_t data_size, size_t rowbytes, size_t width, size_t height, OSType pixel_format, CFDictionaryRef attachments, uint32_t flags);
void wkCAImageQueueSetFlags(CAImageQueueRef iq, uint32_t mask, uint32_t flags);
uint32_t wkCAImageQueueGetFlags(CAImageQueueRef iq);
CFTypeID wkCAImageQueueGetTypeID(void);

WKCACFContext* wkCACFContextCreate();
void wkCACFContextDestroy(WKCACFContext*);

void wkCACFContextSetLayer(WKCACFContext*, CACFLayerRef);
void wkCACFContextFlush(WKCACFContext*);

CFTimeInterval wkCACFContextGetLastCommitTime(WKCACFContext*);
CFTimeInterval wkCACFContextGetNextUpdateTime(WKCACFContext*);

void* wkCACFContextGetUserData(WKCACFContext*);
void wkCACFContextSetUserData(WKCACFContext*, void*);

void* wkCACFLayerGetContextUserData(CACFLayerRef);

void wkCACFContextSetD3DDevice(WKCACFContext*, IDirect3DDevice9*);
void wkCACFContextReleaseD3DResources(WKCACFContext*);

bool wkCACFContextBeginUpdate(WKCACFContext*, void* buffer, size_t bufferSize, CFTimeInterval time, const CGRect& bounds, const CGRect dirtyRects[], size_t dirtyRectCount);
void wkCACFContextRenderUpdate(WKCACFContext*);
void wkCACFContextFinishUpdate(WKCACFContext*);
void wkCACFContextAddUpdateRect(WKCACFContext*, const CGRect&);

WKCACFUpdateRectEnumerator* wkCACFContextCopyUpdateRectEnumerator(WKCACFContext*);
const CGRect* wkCACFUpdateRectEnumeratorNextRect(WKCACFUpdateRectEnumerator*);
void wkCACFUpdateRectEnumeratorRelease(WKCACFUpdateRectEnumerator*);

typedef enum {
    wkPatternTilingNoDistortion,
    wkPatternTilingConstantSpacingMinimalDistortion,
    wkPatternTilingConstantSpacing
} wkPatternTiling;

CGPatternRef wkCGPatternCreateWithImageAndTransform(CGImageRef image, CGAffineTransform transform, int tiling);

CFDictionaryRef wkCFURLRequestCreateSerializableRepresentation(CFURLRequestRef cfRequest, CFTypeRef tokenNull);
CFURLRequestRef wkCFURLRequestCreateFromSerializableRepresentation(CFDictionaryRef representation, CFTypeRef tokenNull);
CFDictionaryRef wkCFURLResponseCreateSerializableRepresentation(CFURLResponseRef cfResponse, CFTypeRef tokenNull);
CFURLResponseRef wkCFURLResponseCreateFromSerializableRepresentation(CFDictionaryRef representation, CFTypeRef tokenNull);

typedef void (*wkQuickTimeMIMETypeCallBack)(const char* mimeType);
void wkGetQuickTimeMIMETypeList(wkQuickTimeMIMETypeCallBack);

typedef enum {
    WKMediaUIPartFullscreenButton   = 0,
    WKMediaUIPartMuteButton,
    WKMediaUIPartPlayButton,
    WKMediaUIPartSeekBackButton,
    WKMediaUIPartSeekForwardButton,
    WKMediaUIPartTimelineSlider,
    WKMediaUIPartTimelineSliderThumb,
    WKMediaUIPartRewindButton,
    WKMediaUIPartSeekToRealtimeButton,
    WKMediaUIPartShowClosedCaptionsButton,
    WKMediaUIPartHideClosedCaptionsButton,
    WKMediaUIPartUnMuteButton,
    WKMediaUIPartPauseButton,
    WKMediaUIPartBackground,
    WKMediaUIPartCurrentTimeDisplay,
    WKMediaUIPartTimeRemainingDisplay,
    WKMediaUIPartStatusDisplay,
    WKMediaUIPartControlsPanel,
    WKMediaUIPartVolumeSliderContainer,
    WKMediaUIPartVolumeSlider,
    WKMediaUIPartVolumeSliderThumb,
    WKMediaUIPartFullScreenVolumeSlider,
    WKMediaUIPartFullScreenVolumeSliderThumb,
    WKMediaUIPartVolumeSliderMuteButton,
    WKMediaUIPartTextTrackDisplayContainer,
    WKMediaUIPartTextTrackDisplay,
    WKMediaUIPartExitFullscreenButton,
} WKMediaUIPart;

typedef enum {
    WKMediaControllerThemeClassic   = 1,
    WKMediaControllerThemeQuickTime = 2
} WKMediaControllerThemeStyle;

typedef enum {
    WKMediaControllerFlagDisabled = 1 << 0,
    WKMediaControllerFlagPressed = 1 << 1,
    WKMediaControllerFlagDrawEndCaps = 1 << 3,
    WKMediaControllerFlagFocused = 1 << 4
} WKMediaControllerThemeState;

#ifdef __cplusplus
extern "C" {
#endif

bool WKMediaControllerThemeAvailable(int themeStyle);
bool WKHitTestMediaUIPart(int part, int themeStyle, CGRect bounds, CGPoint point);
void WKMeasureMediaUIPart(int part, int themeStyle, CGRect *bounds, CGSize *naturalSize);
void WKDrawMediaUIPart(int part, int themeStyle, CGContextRef context, CGRect rect, unsigned state);
void WKDrawMediaSliderTrack(int themeStyle, CGContextRef context, CGRect rect, float timeLoaded, float currentTime, float duration, unsigned state);

#ifdef __cplusplus
}
#endif

#endif // WebKitSystemInterface_h
