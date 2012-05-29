/*      
    WebKitSystemInterface.h
    Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.

    Public header file.
*/

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>

@class QTMovie;
@class QTMovieView;
@class AVAsset;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _CFURLResponse* CFURLResponseRef;

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

#ifndef BUILDING_ON_LEOPARD
CFArrayRef WKCopyNSURLResponseCertificateChain(NSURLResponse *response);
#endif

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

typedef id WKNSURLConnectionDelegateProxyPtr;

WKNSURLConnectionDelegateProxyPtr WKCreateNSURLConnectionDelegateProxy(void);

void WKDisableCGDeferredUpdates(void);

Class WKNSURLProtocolClassForRequest(NSURLRequest *request);
void WKSetNSURLRequestShouldContentSniff(NSMutableURLRequest *request, BOOL shouldContentSniff);

void WKSetCookieStoragePrivateBrowsingEnabled(BOOL enabled);

unsigned WKGetNSAutoreleasePoolCount(void);

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


#if !defined(BUILDING_ON_LEOPARD)
// Remote Accessibility API.
void WKAXRegisterRemoteApp(void);
void WKAXInitializeElementWithPresenterPid(id, pid_t);
NSData *WKAXRemoteTokenForElement(id);
id WKAXRemoteElementForToken(NSData *);
void WKAXSetWindowForRemoteElement(id remoteWindow, id remoteElement);
void WKAXRegisterRemoteProcess(bool registerProcess, pid_t);
pid_t WKAXRemoteProcessIdentifier(id remoteElement);
#endif

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
void WKSetCGFontRenderingMode(CGContextRef cgContext, NSFont *font);
BOOL WKCGContextGetShouldSmoothFonts(CGContextRef cgContext);


void WKSetPatternBaseCTM(CGContextRef, CGAffineTransform);
void WKSetPatternPhaseInUserSpace(CGContextRef, CGPoint);
CGAffineTransform WKGetUserToBaseCTM(CGContextRef);

void WKGetGlyphsForCharacters(CGFontRef, const UniChar[], CGGlyph[], size_t);

CTLineRef WKCreateCTLineWithUniCharProvider(const UniChar* (*provide)(CFIndex stringIndex, CFIndex* charCount, CFDictionaryRef* attributes, void*), void (*dispose)(const UniChar* chars, void*), void*);
#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
CTTypesetterRef WKCreateCTTypesetterWithUniCharProviderAndOptions(const UniChar* (*provide)(CFIndex stringIndex, CFIndex* charCount, CFDictionaryRef* attributes, void*), void (*dispose)(const UniChar* chars, void*), void*, CFDictionaryRef options);

CGContextRef WKIOSurfaceContextCreate(IOSurfaceRef, unsigned width, unsigned height, CGColorSpaceRef);
CGImageRef WKIOSurfaceContextCreateImage(CGContextRef context);
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
NSURLRequest *WKCopyRequestWithStorageSession(CFURLStorageSessionRef, NSURLRequest*);
NSCachedURLResponse *WKCachedResponseForRequest(CFURLStorageSessionRef, NSURLRequest*);

typedef struct OpaqueCFHTTPCookieStorage* CFHTTPCookieStorageRef;
CFHTTPCookieStorageRef WKCopyHTTPCookieStorage(CFURLStorageSessionRef);
unsigned WKGetHTTPCookieAcceptPolicy(CFHTTPCookieStorageRef);
NSArray *WKHTTPCookiesForURL(CFHTTPCookieStorageRef, NSURL *);
void WKSetHTTPCookiesForURL(CFHTTPCookieStorageRef, NSArray *, NSURL *, NSURL *);
void WKDeleteHTTPCookie(CFHTTPCookieStorageRef, NSHTTPCookie *);

void WKSetVisibleApplicationName(CFStringRef);

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
    WKMediaUIPartVolumeSliderThumb
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
    
#ifndef BUILDING_ON_LEOPARD
mach_port_t WKInitializeRenderServer(void);
    
@class CALayer;

CALayer *WKMakeRenderLayer(uint32_t contextID);
    
typedef struct __WKSoftwareCARendererRef *WKSoftwareCARendererRef;

WKSoftwareCARendererRef WKSoftwareCARendererCreate(uint32_t contextID);
void WKSoftwareCARendererDestroy(WKSoftwareCARendererRef);
void WKSoftwareCARendererRender(WKSoftwareCARendererRef, CGContextRef, CGRect);

typedef struct __WKCARemoteLayerClientRef *WKCARemoteLayerClientRef;

WKCARemoteLayerClientRef WKCARemoteLayerClientMakeWithServerPort(mach_port_t port);
void WKCARemoteLayerClientInvalidate(WKCARemoteLayerClientRef);
uint32_t WKCARemoteLayerClientGetClientId(WKCARemoteLayerClientRef);
void WKCARemoteLayerClientSetLayer(WKCARemoteLayerClientRef, CALayer *);
CALayer *WKCARemoteLayerClientGetLayer(WKCARemoteLayerClientRef);

@class CARenderer;

void WKCARendererAddChangeNotificationObserver(CARenderer *, void (*callback)(void*), void* context);
void WKCARendererRemoveChangeNotificationObserver(CARenderer *, void (*callback)(void*), void* context);

typedef struct __WKWindowBounceAnimationContext *WKWindowBounceAnimationContextRef;

WKWindowBounceAnimationContextRef WKWindowBounceAnimationContextCreate(NSWindow *window);
void WKWindowBounceAnimationContextDestroy(WKWindowBounceAnimationContextRef context);
void WKWindowBounceAnimationSetAnimationProgress(WKWindowBounceAnimationContextRef context, double animationProgress);

#if defined(__x86_64__)
#import <mach/mig.h>
CFRunLoopSourceRef WKCreateMIGServerSource(mig_subsystem_t subsystem, mach_port_t serverPort);
#endif // defined(__x86_64__)

NSUInteger WKGetInputPanelWindowStyle(void);
UInt8 WKGetNSEventKeyChar(NSEvent *);
#endif // !defined(BUILDING_ON_LEOPARD)

@class CAPropertyAnimation;
void WKSetCAAnimationValueFunction(CAPropertyAnimation*, NSString* function);

unsigned WKInitializeMaximumHTTPConnectionCountPerHost(unsigned preferredConnectionCount);
int WKGetHTTPPipeliningPriority(NSURLRequest *);
void WKSetHTTPPipeliningMaximumPriority(int maximumPriority);
void WKSetHTTPPipeliningPriority(NSMutableURLRequest *, int priority);
void WKSetHTTPPipeliningMinimumFastLanePriority(int priority);

void WKSetCONNECTProxyForStream(CFReadStreamRef, CFStringRef proxyHost, CFNumberRef proxyPort);
void WKSetCONNECTProxyAuthorizationForStream(CFReadStreamRef, CFStringRef proxyAuthorizationString);
CFHTTPMessageRef WKCopyCONNECTProxyResponse(CFReadStreamRef, CFURLRef responseURL);

#if defined(BUILDING_ON_LEOPARD) || defined(BUILDING_ON_SNOW_LEOPARD)
typedef enum {
    WKEventPhaseNone = 0,
    WKEventPhaseBegan = 1,
    WKEventPhaseChanged = 2,
    WKEventPhaseEnded = 3,
} WKEventPhase;

int WKGetNSEventMomentumPhase(NSEvent *);
#endif

void WKWindowSetAlpha(NSWindow *window, float alphaValue);
void WKWindowSetScaledFrame(NSWindow *window, NSRect scaleFrame, NSRect nonScaledFrame);

#ifndef BUILDING_ON_LEOPARD
void WKSyncSurfaceToView(NSView *view);

void WKEnableSettingCursorWhenInBackground(void);

CFDictionaryRef WKNSURLRequestCreateSerializableRepresentation(NSURLRequest *request, CFTypeRef tokenNull);
NSURLRequest *WKNSURLRequestFromSerializableRepresentation(CFDictionaryRef representation, CFTypeRef tokenNull);

CFDictionaryRef WKNSURLResponseCreateSerializableRepresentation(NSURLResponse *response, CFTypeRef tokenNull);
NSURLResponse *WKNSURLResponseFromSerializableRepresentation(CFDictionaryRef representation, CFTypeRef tokenNull);

#ifndef __LP64__
ScriptCode WKGetScriptCodeFromCurrentKeyboardInputSource(void);
#endif

#endif

#if defined(BUILDING_ON_LEOPARD) || defined(BUILDING_ON_SNOW_LEOPARD)
CFIndex WKGetHyphenationLocationBeforeIndex(CFStringRef string, CFIndex index);
#endif

CFArrayRef WKCFURLCacheCopyAllHostNamesInPersistentStore(void);
void WKCFURLCacheDeleteHostNamesInPersistentStore(CFArrayRef hostArray);    

CFStringRef WKGetCFURLResponseMIMEType(CFURLResponseRef);
CFURLRef WKGetCFURLResponseURL(CFURLResponseRef);
CFHTTPMessageRef WKGetCFURLResponseHTTPResponse(CFURLResponseRef);
CFStringRef WKCopyCFURLResponseSuggestedFilename(CFURLResponseRef);
void WKSetCFURLResponseMIMEType(CFURLResponseRef, CFStringRef mimeType);

#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
typedef enum {
    WKSandboxExtensionTypeReadOnly,
    WKSandboxExtensionTypeWriteOnly,    
    WKSandboxExtensionTypeReadWrite,
} WKSandboxExtensionType;
typedef struct __WKSandboxExtension *WKSandboxExtensionRef;

WKSandboxExtensionRef WKSandboxExtensionCreate(const char* path, WKSandboxExtensionType type);
void WKSandboxExtensionDestroy(WKSandboxExtensionRef sandboxExtension);

bool WKSandboxExtensionConsume(WKSandboxExtensionRef sandboxExtension);
bool WKSandboxExtensionInvalidate(WKSandboxExtensionRef sandboxExtension);

const char* WKSandboxExtensionGetSerializedFormat(WKSandboxExtensionRef sandboxExtension, size_t* length);
WKSandboxExtensionRef WKSandboxExtensionCreateFromSerializedFormat(const char* serializationFormat, size_t length);

typedef struct __WKScrollbarPainter *WKScrollbarPainterRef;
typedef struct __WKScrollbarPainterController *WKScrollbarPainterControllerRef;

WKScrollbarPainterRef WKMakeScrollbarPainter(int controlSize, bool isHorizontal);
WKScrollbarPainterRef WKMakeScrollbarReplacementPainter(WKScrollbarPainterRef oldPainter, int newStyle, int controlSize, bool isHorizontal);
void WKScrollbarPainterSetDelegate(WKScrollbarPainterRef, id scrollbarPainterDelegate);
void WKScrollbarPainterPaint(WKScrollbarPainterRef, bool enabled, double value, CGFloat proportion, CGRect frameRect);
void WKScrollbarPainterForceFlashScrollers(WKScrollbarPainterControllerRef);
int WKScrollbarThickness(int controlSize);
int WKScrollbarMinimumThumbLength(WKScrollbarPainterRef);
int WKScrollbarMinimumTotalLengthNeededForThumb(WKScrollbarPainterRef);
CGFloat WKScrollbarPainterKnobAlpha(WKScrollbarPainterRef);
void WKSetScrollbarPainterKnobAlpha(WKScrollbarPainterRef, CGFloat);
CGFloat WKScrollbarPainterTrackAlpha(WKScrollbarPainterRef);
void WKSetScrollbarPainterTrackAlpha(WKScrollbarPainterRef, CGFloat);
bool WKScrollbarPainterIsHorizontal(WKScrollbarPainterRef);
CGRect WKScrollbarPainterKnobRect(WKScrollbarPainterRef);
void WKScrollbarPainterSetOverlayState(WKScrollbarPainterRef, int overlayScrollerState);

// The wk* to WK* renaming does not apply to enums. The way to
// circumvent this is to define the enum anonymously twice using
// the two prefixes. (See WebCoreSystemInterface.h)
enum {
    WKScrollerKnobStyleDefault = 0,
    WKScrollerKnobStyleDark = 1,
    WKScrollerKnobStyleLight = 2
};
typedef uint32 WKScrollerKnobStyle;
void WKSetScrollbarPainterKnobStyle(WKScrollbarPainterRef, WKScrollerKnobStyle);

WKScrollbarPainterControllerRef WKMakeScrollbarPainterController(id painterControllerDelegate);
void WKSetPainterForPainterController(WKScrollbarPainterControllerRef, WKScrollbarPainterRef, bool isHorizontal);
WKScrollbarPainterRef WKVerticalScrollbarPainterForController(WKScrollbarPainterControllerRef);
WKScrollbarPainterRef WKHorizontalScrollbarPainterForController(WKScrollbarPainterControllerRef);
void WKSetScrollbarPainterControllerStyle(WKScrollbarPainterControllerRef, int newStyle);
void WKContentAreaScrolled(WKScrollbarPainterControllerRef);
void WKContentAreaWillPaint(WKScrollbarPainterControllerRef);
void WKMouseEnteredContentArea(WKScrollbarPainterControllerRef);
void WKMouseExitedContentArea(WKScrollbarPainterControllerRef);
void WKMouseMovedInContentArea(WKScrollbarPainterControllerRef);
void WKWillStartLiveResize(WKScrollbarPainterControllerRef);
void WKContentAreaResized(WKScrollbarPainterControllerRef);
void WKWillEndLiveResize(WKScrollbarPainterControllerRef);
void WKContentAreaDidShow(WKScrollbarPainterControllerRef);
void WKContentAreaDidHide(WKScrollbarPainterControllerRef);
void WKDidBeginScrollGesture(WKScrollbarPainterControllerRef);
void WKDidEndScrollGesture(WKScrollbarPainterControllerRef);

bool WKScrollbarPainterUsesOverlayScrollers(void);

NSRange WKExtractWordDefinitionTokenRangeFromContextualString(NSString *contextString, NSRange range, NSDictionary **options);
void WKShowWordDefinitionWindow(NSAttributedString *term, NSPoint screenPoint, NSDictionary *options);
void WKHideWordDefinitionWindow(void);

NSURL* WKAVAssetResolvedURL(AVAsset*);
#endif

#ifdef __cplusplus
}
#endif
