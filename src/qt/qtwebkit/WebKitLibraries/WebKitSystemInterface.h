/*      
    WebKitSystemInterface.h
    Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Apple Inc. All rights reserved.

    Public header file.
*/

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <dispatch/dispatch.h>

@class AVAsset;
@class QTMovie;
@class QTMovieView;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _CFURLResponse* CFURLResponseRef;
typedef const struct _CFURLRequest* CFURLRequestRef;
typedef struct _CFURLRequest* CFMutableURLRequestRef;

typedef struct _CFURLCredential* WKCFURLCredentialRef;
typedef struct _CFURLProtectionSpace* CFURLProtectionSpaceRef;

typedef enum {
    WKCertificateParseResultSucceeded  = 0,
    WKCertificateParseResultFailed     = 1,
    WKCertificateParseResultPKCS7      = 2,
} WKCertificateParseResult;

CFStringRef WKCopyCFLocalizationPreferredName(CFStringRef localization);
void WKSetDefaultLocalization(CFStringRef localization);

CFStringRef WKSignedPublicKeyAndChallengeString(unsigned keySize, CFStringRef challenge, CFStringRef keyDescription);
WKCertificateParseResult WKAddCertificatesToKeychainFromData(const void *bytes, unsigned length);

NSString *WKGetPreferredExtensionForMIMEType(NSString *type);
NSArray *WKGetExtensionsForMIMEType(NSString *type);
NSString *WKGetMIMETypeForExtension(NSString *extension);

NSDate *WKGetNSURLResponseLastModifiedDate(NSURLResponse *response);
NSTimeInterval WKGetNSURLResponseFreshnessLifetime(NSURLResponse *response);
NSString *WKCopyNSURLResponseStatusLine(NSURLResponse *response);

CFArrayRef WKCopyNSURLResponseCertificateChain(NSURLResponse *response);

CFStringEncoding WKGetWebDefaultCFStringEncoding(void);

void WKSetMetadataURL(NSString *URLString, NSString *referrer, NSString *path);
void WKSetNSURLConnectionDefersCallbacks(NSURLConnection *connection, BOOL defers);

void WKShowKeyAndMain(void);
#ifndef __LP64__
OSStatus WKSyncWindowWithCGAfterMove(WindowRef);
unsigned WKCarbonWindowMask(void);
void *WKGetNativeWindowFromWindowRef(WindowRef);
OSType WKCarbonWindowPropertyCreator(void);
OSType WKCarbonWindowPropertyTag(void);
#endif

void WKDisableCGDeferredUpdates(void);

Class WKNSURLProtocolClassForRequest(NSURLRequest *request);
void WKSetNSURLRequestShouldContentSniff(NSMutableURLRequest *request, BOOL shouldContentSniff);

#ifndef __LP64__
unsigned WKGetNSAutoreleasePoolCount(void);
#endif

void WKAdvanceDefaultButtonPulseAnimation(NSButtonCell *button);

NSString *WKMouseMovedNotification(void);
NSString *WKWindowWillOrderOnScreenNotification(void);
NSString *WKWindowWillOrderOffScreenNotification(void);
void WKSetNSWindowShouldPostEventNotifications(NSWindow *window, BOOL post);

CFTypeID WKGetAXTextMarkerTypeID(void);
CFTypeID WKGetAXTextMarkerRangeTypeID(void);
CFTypeRef WKCreateAXTextMarker(const void *bytes, size_t len);
BOOL WKGetBytesFromAXTextMarker(CFTypeRef textMarker, void *bytes, size_t length);
CFTypeRef WKCreateAXTextMarkerRange(CFTypeRef start, CFTypeRef end);
CFTypeRef WKCopyAXTextMarkerRangeStart(CFTypeRef range);
CFTypeRef WKCopyAXTextMarkerRangeEnd(CFTypeRef range);
void WKAccessibilityHandleFocusChanged(void);
AXUIElementRef WKCreateAXUIElementRef(id element);
void WKUnregisterUniqueIdForElement(id element);
    
NSArray *WKSpeechSynthesisGetVoiceIdentifiers(void);
NSString *WKSpeechSynthesisGetDefaultVoiceIdentifierForLocale(NSLocale*);
    
BOOL WKShouldBlockPlugin(NSString *bundleIdentifier, NSString *bundleVersionString);
BOOL WKIsPluginUpdateAvailable(NSString *bundleIdentifier);

// Remote Accessibility API.
void WKAXRegisterRemoteApp(void);
void WKAXInitializeElementWithPresenterPid(id, pid_t);
NSData *WKAXRemoteTokenForElement(id);
id WKAXRemoteElementForToken(NSData *);
void WKAXSetWindowForRemoteElement(id remoteWindow, id remoteElement);
void WKAXRegisterRemoteProcess(bool registerProcess, pid_t);
pid_t WKAXRemoteProcessIdentifier(id remoteElement);

void WKSetUpFontCache(void);

void WKSignalCFReadStreamEnd(CFReadStreamRef stream);
void WKSignalCFReadStreamHasBytes(CFReadStreamRef stream);
void WKSignalCFReadStreamError(CFReadStreamRef stream, CFStreamError *error);

CFReadStreamRef WKCreateCustomCFReadStream(void *(*formCreate)(CFReadStreamRef, void *), 
    void (*formFinalize)(CFReadStreamRef, void *), 
    Boolean (*formOpen)(CFReadStreamRef, CFStreamError *, Boolean *, void *), 
    CFIndex (*formRead)(CFReadStreamRef, UInt8 *, CFIndex, CFStreamError *, Boolean *, void *), 
    Boolean (*formCanRead)(CFReadStreamRef, void *), 
    void (*formClose)(CFReadStreamRef, void *), 
    void (*formSchedule)(CFReadStreamRef, CFRunLoopRef, CFStringRef, void *), 
    void (*formUnschedule)(CFReadStreamRef, CFRunLoopRef, CFStringRef, void *),
    void *context);

void WKDrawCapsLockIndicator(CGContextRef, CGRect);

void WKDrawFocusRing(CGContextRef context, CGColorRef color, int radius);
    // The CG context's current path is the focus ring's path.
    // A color of 0 means "use system focus ring color".
    // A radius of 0 means "use default focus ring radius".

void WKSetDragImage(NSImage *image, NSPoint offset);

void WKDrawBezeledTextFieldCell(NSRect, BOOL enabled);
void WKDrawTextFieldCellFocusRing(NSTextFieldCell*, NSRect);
void WKDrawBezeledTextArea(NSRect, BOOL enabled);
void WKPopupMenu(NSMenu*, NSPoint location, float width, NSView*, int selectedItem, NSFont*);
void WKPopupContextMenu(NSMenu *menu, NSPoint screenLocation);
void WKSendUserChangeNotifications(void);
#ifndef __LP64__
BOOL WKConvertNSEventToCarbonEvent(EventRecord *carbonEvent, NSEvent *cocoaEvent);
void WKSendKeyEventToTSM(NSEvent *theEvent);
void WKCallDrawingNotification(CGrafPtr port, Rect *bounds);
#endif

BOOL WKGetGlyphTransformedAdvances(CGFontRef, NSFont*, CGAffineTransform *m, ATSGlyphRef *glyph, CGSize *advance);
NSFont *WKGetFontInLanguageForRange(NSFont *font, NSString *string, NSRange range);
NSFont *WKGetFontInLanguageForCharacter(NSFont *font, UniChar ch);
void WKSetCGFontRenderingMode(CGContextRef cgContext, NSFont *font, BOOL shouldSubpixelQuantize);
BOOL WKCGContextGetShouldSmoothFonts(CGContextRef cgContext);


void WKSetBaseCTM(CGContextRef, CGAffineTransform);
void WKSetPatternPhaseInUserSpace(CGContextRef, CGPoint);
CGAffineTransform WKGetUserToBaseCTM(CGContextRef);

void WKGetGlyphsForCharacters(CGFontRef, const UniChar[], CGGlyph[], size_t);
bool WKGetVerticalGlyphsForCharacters(CTFontRef, const UniChar[], CGGlyph[], size_t);

CTLineRef WKCreateCTLineWithUniCharProvider(const UniChar* (*provide)(CFIndex stringIndex, CFIndex* charCount, CFDictionaryRef* attributes, void*), void (*dispose)(const UniChar* chars, void*), void*);

#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
enum {
    WKCTFontTransformApplyShaping = (1 << 0),
    WKCTFontTransformApplyPositioning = (1 << 1)
};

typedef int WKCTFontTransformOptions;

bool WKCTFontTransformGlyphs(CTFontRef font, CGGlyph glyphs[], CGSize advances[], CFIndex count, WKCTFontTransformOptions options);
#endif

CTTypesetterRef WKCreateCTTypesetterWithUniCharProviderAndOptions(const UniChar* (*provide)(CFIndex stringIndex, CFIndex* charCount, CFDictionaryRef* attributes, void*), void (*dispose)(const UniChar* chars, void*), void*, CFDictionaryRef options);

CGSize WKCTRunGetInitialAdvance(CTRunRef);

CGContextRef WKIOSurfaceContextCreate(IOSurfaceRef, unsigned width, unsigned height, CGColorSpaceRef);
CGImageRef WKIOSurfaceContextCreateImage(CGContextRef context);

typedef enum {
    WKPatternTilingNoDistortion,
    WKPatternTilingConstantSpacingMinimalDistortion,
    WKPatternTilingConstantSpacing
} WKPatternTiling;

CGPatternRef WKCGPatternCreateWithImageAndTransform(CGImageRef image, CGAffineTransform transform, int tiling);
void WKCGContextResetClip(CGContextRef);

#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
bool WKCGContextDrawsWithCorrectShadowOffsets(CGContextRef);
#endif

#ifndef __LP64__
NSEvent *WKCreateNSEventWithCarbonEvent(EventRef eventRef);
NSEvent *WKCreateNSEventWithCarbonMouseMoveEvent(EventRef inEvent, NSWindow *window);
NSEvent *WKCreateNSEventWithCarbonClickEvent(EventRef inEvent, WindowRef windowRef);
#endif

CGContextRef WKNSWindowOverrideCGContext(NSWindow *, CGContextRef);
void WKNSWindowRestoreCGContext(NSWindow *, CGContextRef);

void WKNSWindowMakeBottomCornersSquare(NSWindow *);

// These constants match the ones used by ThemeScrollbarArrowStyle (some of the values are private, so we can't just
// use that enum directly).
typedef enum {
    WKThemeScrollBarArrowsSingle     = 0,
    WKThemeScrollBarArrowsLowerRight = 1,
    WKThemeScrollBarArrowsDouble     = 2,
    WKThemeScrollBarArrowsUpperLeft  = 3,
} WKThemeScrollBarArrowStyle;

OSStatus WKThemeDrawTrack(const HIThemeTrackDrawInfo* inDrawInfo, CGContextRef inContext, int inArrowStyle);


BOOL WKCGContextIsBitmapContext(CGContextRef context);

void WKGetWheelEventDeltas(NSEvent *, float *deltaX, float *deltaY, BOOL *continuous);

BOOL WKAppVersionCheckLessThan(NSString *, int, double);

typedef enum {
    WKMovieTypeUnknown,
    WKMovieTypeDownload,
    WKMovieTypeStoredStream,
    WKMovieTypeLiveStream
} WKMovieType;

int WKQTMovieGetType(QTMovie* movie);

BOOL WKQTMovieHasClosedCaptions(QTMovie* movie);
void WKQTMovieSetShowClosedCaptions(QTMovie* movie, BOOL showClosedCaptions);
void WKQTMovieSelectPreferredAlternates(QTMovie* movie);
void WKQTMovieSelectPreferredAlternateTrackForMediaType(QTMovie* movie, NSString* mediaType);

unsigned WKQTIncludeOnlyModernMediaFileTypes(void);
int WKQTMovieDataRate(QTMovie* movie);
float WKQTMovieMaxTimeLoaded(QTMovie* movie);
float WKQTMovieMaxTimeSeekable(QTMovie* movie);
NSString *WKQTMovieMaxTimeLoadedChangeNotification(void);
void WKQTMovieViewSetDrawSynchronously(QTMovieView* view, BOOL sync);
void WKQTMovieDisableComponent(uint32_t[5]);
NSURL *WKQTMovieResolvedURL(QTMovie* movie);

CFStringRef WKCopyFoundationCacheDirectory(void);

typedef const struct __CFURLStorageSession* CFURLStorageSessionRef;
CFURLStorageSessionRef WKCreatePrivateStorageSession(CFStringRef);
NSURLRequest *WKCopyRequestWithStorageSession(CFURLStorageSessionRef, NSURLRequest *);
NSCachedURLResponse *WKCachedResponseForRequest(CFURLStorageSessionRef, NSURLRequest *);
void WKSetRequestStorageSession(CFURLStorageSessionRef, CFMutableURLRequestRef);

typedef struct OpaqueCFHTTPCookieStorage* CFHTTPCookieStorageRef;
CFHTTPCookieStorageRef WKCopyHTTPCookieStorage(CFURLStorageSessionRef);
unsigned WKGetHTTPCookieAcceptPolicy(CFHTTPCookieStorageRef);
void WKSetHTTPCookieAcceptPolicy(CFHTTPCookieStorageRef, unsigned policy);
NSArray *WKHTTPCookies(CFHTTPCookieStorageRef);
NSArray *WKHTTPCookiesForURL(CFHTTPCookieStorageRef, NSURL *);
void WKSetHTTPCookiesForURL(CFHTTPCookieStorageRef, NSArray *, NSURL *, NSURL *);
void WKDeleteAllHTTPCookies(CFHTTPCookieStorageRef);
void WKDeleteHTTPCookie(CFHTTPCookieStorageRef, NSHTTPCookie *);

CFHTTPCookieStorageRef WKGetDefaultHTTPCookieStorage(void);
WKCFURLCredentialRef WKCopyCredentialFromCFPersistentStorage(CFURLProtectionSpaceRef);
void WKSetCFURLRequestShouldContentSniff(CFMutableURLRequestRef, bool flag);
CFArrayRef WKCFURLRequestCopyHTTPRequestBodyParts(CFURLRequestRef);
void WKCFURLRequestSetHTTPRequestBodyParts(CFMutableURLRequestRef, CFArrayRef bodyParts);

void WKSetVisibleApplicationName(CFStringRef);
void WKSetApplicationInformationItem(CFStringRef key, CFTypeRef value);

CFURLRef WKCopyBundleURLForExecutableURL(CFURLRef);

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

BOOL WKMediaControllerThemeAvailable(int themeStyle);
BOOL WKHitTestMediaUIPart(int part, int themeStyle, CGRect bounds, CGPoint point);
void WKMeasureMediaUIPart(int part, int themeStyle, CGRect *bounds, CGSize *naturalSize);
void WKDrawMediaUIPart(int part, int themeStyle, CGContextRef context, CGRect rect, unsigned state);
void WKDrawMediaSliderTrack(int themeStyle, CGContextRef context, CGRect rect, float timeLoaded, float currentTime, float duration, unsigned state);
NSView *WKCreateMediaUIBackgroundView(void);

typedef enum {
    WKMediaUIControlTimeline,
    WKMediaUIControlSlider,
    WKMediaUIControlPlayPauseButton,
    WKMediaUIControlExitFullscreenButton,
    WKMediaUIControlRewindButton,
    WKMediaUIControlFastForwardButton,
    WKMediaUIControlVolumeUpButton,
    WKMediaUIControlVolumeDownButton
} WKMediaUIControlType;
    
NSControl *WKCreateMediaUIControl(int controlType);

NSArray *WKQTGetSitesInMediaDownloadCache();
void WKQTClearMediaDownloadCacheForSite(NSString *site);
void WKQTClearMediaDownloadCache();
    
mach_port_t WKInitializeRenderServer(void);
    
@class CALayer;

CALayer *WKMakeRenderLayer(uint32_t contextID);
    
typedef struct __WKSoftwareCARendererRef *WKSoftwareCARendererRef;

WKSoftwareCARendererRef WKSoftwareCARendererCreate(uint32_t contextID);
void WKSoftwareCARendererDestroy(WKSoftwareCARendererRef);
void WKSoftwareCARendererRender(WKSoftwareCARendererRef, CGContextRef, CGRect);

typedef struct __WKCAContextRef *WKCAContextRef;

WKCAContextRef WKCAContextMakeRemoteWithServerPort(mach_port_t port);
WKCAContextRef WKCAContextMakeRemoteForWindowServer(void);
void WKCAContextInvalidate(WKCAContextRef);
uint32_t WKCAContextGetContextId(WKCAContextRef);
void WKCAContextSetLayer(WKCAContextRef, CALayer *);
CALayer *WKCAContextGetLayer(WKCAContextRef);
void WKCAContextSetColorSpace(WKCAContextRef, CGColorSpaceRef);
CGColorSpaceRef WKCAContextGetColorSpace(WKCAContextRef);
void WKDestroyRenderingResources(void);

void WKCALayerEnumerateRectsBeingDrawnWithBlock(CALayer *layer, CGContextRef context, void (^block)(CGRect rect));

@class CARenderer;

void WKCARendererAddChangeNotificationObserver(CARenderer *, void (*callback)(void*), void* context);
void WKCARendererRemoveChangeNotificationObserver(CARenderer *, void (*callback)(void*), void* context);

typedef struct __WKWindowBounceAnimationContext *WKWindowBounceAnimationContextRef;

WKWindowBounceAnimationContextRef WKWindowBounceAnimationContextCreate(NSWindow *window);
void WKWindowBounceAnimationContextDestroy(WKWindowBounceAnimationContextRef context);
void WKWindowBounceAnimationSetAnimationProgress(WKWindowBounceAnimationContextRef context, double animationProgress);

void WKWindowSetClipRect(NSWindow*, NSRect);

#if defined(__x86_64__)
#import <mach/mig.h>
CFRunLoopSourceRef WKCreateMIGServerSource(mig_subsystem_t subsystem, mach_port_t serverPort);
#endif // defined(__x86_64__)

NSUInteger WKGetInputPanelWindowStyle(void);
UInt8 WKGetNSEventKeyChar(NSEvent *);

@class CAPropertyAnimation;
void WKSetCAAnimationValueFunction(CAPropertyAnimation*, NSString* function);

unsigned WKInitializeMaximumHTTPConnectionCountPerHost(unsigned preferredConnectionCount);
int WKGetHTTPPipeliningPriority(CFURLRequestRef);
void WKSetHTTPPipeliningMaximumPriority(int maximumPriority);
void WKSetHTTPPipeliningPriority(CFURLRequestRef, int priority);
void WKSetHTTPPipeliningMinimumFastLanePriority(int priority);

void WKSetCONNECTProxyForStream(CFReadStreamRef, CFStringRef proxyHost, CFNumberRef proxyPort);
void WKSetCONNECTProxyAuthorizationForStream(CFReadStreamRef, CFStringRef proxyAuthorizationString);
CFHTTPMessageRef WKCopyCONNECTProxyResponse(CFReadStreamRef, CFURLRef responseURL, CFStringRef proxyHost, CFNumberRef proxyPort);

void WKWindowSetAlpha(NSWindow *window, float alphaValue);
void WKWindowSetScaledFrame(NSWindow *window, NSRect scaleFrame, NSRect nonScaledFrame);

void WKSyncSurfaceToView(NSView *view);

void WKEnableSettingCursorWhenInBackground(void);

CFDictionaryRef WKNSURLRequestCreateSerializableRepresentation(NSURLRequest *request, CFTypeRef tokenNull);
NSURLRequest *WKNSURLRequestFromSerializableRepresentation(CFDictionaryRef representation, CFTypeRef tokenNull);

CFDictionaryRef WKNSURLResponseCreateSerializableRepresentation(NSURLResponse *response, CFTypeRef tokenNull);
NSURLResponse *WKNSURLResponseFromSerializableRepresentation(CFDictionaryRef representation, CFTypeRef tokenNull);

#ifndef __LP64__
ScriptCode WKGetScriptCodeFromCurrentKeyboardInputSource(void);
#endif

CFArrayRef WKCFURLCacheCopyAllHostNamesInPersistentStore(void);
void WKCFURLCacheDeleteHostNamesInPersistentStore(CFArrayRef hostArray);    

CFStringRef WKGetCFURLResponseMIMEType(CFURLResponseRef);
CFURLRef WKGetCFURLResponseURL(CFURLResponseRef);
CFHTTPMessageRef WKGetCFURLResponseHTTPResponse(CFURLResponseRef);
CFStringRef WKCopyCFURLResponseSuggestedFilename(CFURLResponseRef);
void WKSetCFURLResponseMIMEType(CFURLResponseRef, CFStringRef mimeType);

CIFormat WKCIGetRGBA8Format(void);

typedef enum {
    WKSandboxExtensionTypeReadOnly,
    WKSandboxExtensionTypeReadWrite,
} WKSandboxExtensionType;
typedef struct __WKSandboxExtension *WKSandboxExtensionRef;

WKSandboxExtensionRef WKSandboxExtensionCreate(const char* path, WKSandboxExtensionType type);
void WKSandboxExtensionDestroy(WKSandboxExtensionRef sandboxExtension);

bool WKSandboxExtensionConsume(WKSandboxExtensionRef sandboxExtension);
bool WKSandboxExtensionInvalidate(WKSandboxExtensionRef sandboxExtension);

const char* WKSandboxExtensionGetSerializedFormat(WKSandboxExtensionRef sandboxExtension, size_t* length);
WKSandboxExtensionRef WKSandboxExtensionCreateFromSerializedFormat(const char* serializationFormat, size_t length);

OSStatus WKEnableSandboxStyleFileQuarantine(void);

int WKRecommendedScrollerStyle(void);

bool WKExecutableWasLinkedOnOrBeforeSnowLeopard(void);

NSRange WKExtractWordDefinitionTokenRangeFromContextualString(NSString *contextString, NSRange range, NSDictionary **options);
void WKShowWordDefinitionWindow(NSAttributedString *term, NSPoint screenPoint, NSDictionary *options);
void WKHideWordDefinitionWindow(void);

CFStringRef WKCopyDefaultSearchProviderDisplayName(void);

void WKSetCrashReportApplicationSpecificInformation(CFStringRef);

NSURL* WKAVAssetResolvedURL(AVAsset*);

NSCursor *WKCursor(const char *name);

dispatch_source_t WKCreateVMPressureDispatchOnMainQueue(void);

#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
bool WKExecutableWasLinkedOnOrBeforeLion(void);
#endif

#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
bool WKExecutableWasLinkedOnOrBeforeMountainLion(void);
#endif

void WKCGPathAddRoundedRect(CGMutablePathRef path, const CGAffineTransform* matrix, CGRect rect, CGFloat cornerWidth, CGFloat cornerHeight);

void WKCFURLRequestAllowAllPostCaching(CFURLRequestRef);

#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
@class WebFilterEvaluator;

BOOL WKFilterIsManagedSession(void);
WebFilterEvaluator *WKFilterCreateInstance(NSURLResponse *);
BOOL WKFilterIsBuffering(WebFilterEvaluator *);
BOOL WKFilterWasBlocked(WebFilterEvaluator *);
NSData *WKFilterAddData(WebFilterEvaluator *, NSData *);
NSData *WKFilterDataComplete(WebFilterEvaluator *);

CGFloat WKNSElasticDeltaForTimeDelta(CGFloat initialPosition, CGFloat initialVelocity, CGFloat elapsedTime);
CGFloat WKNSElasticDeltaForReboundDelta(CGFloat delta);
CGFloat WKNSReboundDeltaForElasticDelta(CGFloat delta);
#endif

#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
typedef enum {
    WKOcclusionNotificationTypeApplicationBecameVisible,
    WKOcclusionNotificationTypeApplicationBecameOccluded,
    WKOcclusionNotificationTypeApplicationWindowModificationsStarted,
    WKOcclusionNotificationTypeApplicationWindowModificationsStopped,
    WKOcclusionNotificationTypeWindowBecameVisible,
    WKOcclusionNotificationTypeWindowBecameOccluded,
} WKOcclusionNotificationType;

typedef uint32_t WKWindowID;

typedef void (*WKOcclusionNotificationHandler)(uint32_t, void* data, uint32_t dataLength, void*, uint32_t);

bool WKRegisterOcclusionNotificationHandler(WKOcclusionNotificationType, WKOcclusionNotificationHandler);
bool WKUnregisterOcclusionNotificationHandler(WKOcclusionNotificationType, WKOcclusionNotificationHandler);
bool WKEnableWindowOcclusionNotifications(NSInteger windowID, bool *outCurrentOcclusionState);
#endif

bool WKIsJavaPlugInActive(void);
void WKActivateJavaPlugIn(void);

void WKCFNetworkSetOverrideSystemProxySettings(CFDictionaryRef);

#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
bool WKIsPublicSuffix(NSString *domain);

CFArrayRef WKCFURLCacheCopyAllHostNamesInPersistentStoreForPartition(CFStringRef partition);
typedef void (^CFURLCacheCopyAllPartitionNamesResultsNotification)(CFArrayRef partitionNames);

void WKCFURLCacheDeleteHostNamesInPersistentStoreForPartition(CFArrayRef hostArray, CFStringRef partition);
CFStringRef WKCachePartitionKey(void);
void WKCFURLCacheCopyAllPartitionNames(CFURLCacheCopyAllPartitionNamesResultsNotification resultsBlock);
#endif

#ifdef __cplusplus
}
#endif
