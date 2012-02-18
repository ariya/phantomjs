/*
 * Copyright (C) 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
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

#import "config.h"

#if ENABLE(VIDEO)

#import "MediaPlayerPrivateQTKit.h"

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
#include "ApplicationCacheHost.h"
#include "ApplicationCacheResource.h"
#include "DocumentLoader.h"
#endif


#import "BlockExceptions.h"
#import "DocumentLoader.h"
#import "FrameView.h"
#import "HostWindow.h"
#import "GraphicsContext.h"
#import "KURL.h"
#import "MIMETypeRegistry.h"
#import "SecurityOrigin.h"
#import "SoftLinking.h"
#import "TimeRanges.h"
#import "WebCoreSystemInterface.h"
#import <QTKit/QTKit.h>
#import <objc/objc-runtime.h>
#import <wtf/UnusedParam.h>

#if USE(ACCELERATED_COMPOSITING)
#include "GraphicsLayer.h"
#endif

#if DRAW_FRAME_RATE
#import "Font.h"
#import "Frame.h"
#import "Document.h"
#import "RenderObject.h"
#import "RenderStyle.h"
#endif

SOFT_LINK_FRAMEWORK(QTKit)

SOFT_LINK(QTKit, QTMakeTime, QTTime, (long long timeValue, long timeScale), (timeValue, timeScale))

SOFT_LINK_CLASS(QTKit, QTMovie)
SOFT_LINK_CLASS(QTKit, QTMovieView)
SOFT_LINK_CLASS(QTKit, QTMovieLayer)

SOFT_LINK_POINTER(QTKit, QTTrackMediaTypeAttribute, NSString *)
SOFT_LINK_POINTER(QTKit, QTMediaTypeAttribute, NSString *)
SOFT_LINK_POINTER(QTKit, QTMediaTypeBase, NSString *)
SOFT_LINK_POINTER(QTKit, QTMediaTypeMPEG, NSString *)
SOFT_LINK_POINTER(QTKit, QTMediaTypeSound, NSString *)
SOFT_LINK_POINTER(QTKit, QTMediaTypeText, NSString *)
SOFT_LINK_POINTER(QTKit, QTMediaTypeVideo, NSString *)
SOFT_LINK_POINTER(QTKit, QTMovieAskUnresolvedDataRefsAttribute, NSString *)
SOFT_LINK_POINTER(QTKit, QTMovieLoopsAttribute, NSString *)
SOFT_LINK_POINTER(QTKit, QTMovieDataAttribute, NSString *)
SOFT_LINK_POINTER(QTKit, QTMovieDataSizeAttribute, NSString *)
SOFT_LINK_POINTER(QTKit, QTMovieDidEndNotification, NSString *)
SOFT_LINK_POINTER(QTKit, QTMovieHasVideoAttribute, NSString *)
SOFT_LINK_POINTER(QTKit, QTMovieHasAudioAttribute, NSString *)
SOFT_LINK_POINTER(QTKit, QTMovieIsActiveAttribute, NSString *)
SOFT_LINK_POINTER(QTKit, QTMovieLoadStateAttribute, NSString *)
SOFT_LINK_POINTER(QTKit, QTMovieLoadStateDidChangeNotification, NSString *)
SOFT_LINK_POINTER(QTKit, QTMovieNaturalSizeAttribute, NSString *)
SOFT_LINK_POINTER(QTKit, QTMovieCurrentSizeAttribute, NSString *)
SOFT_LINK_POINTER(QTKit, QTMoviePreventExternalURLLinksAttribute, NSString *)
SOFT_LINK_POINTER(QTKit, QTMovieRateChangesPreservePitchAttribute, NSString *)
SOFT_LINK_POINTER(QTKit, QTMovieRateDidChangeNotification, NSString *)
SOFT_LINK_POINTER(QTKit, QTMovieSizeDidChangeNotification, NSString *)
SOFT_LINK_POINTER(QTKit, QTMovieTimeDidChangeNotification, NSString *)
SOFT_LINK_POINTER(QTKit, QTMovieTimeScaleAttribute, NSString *)
SOFT_LINK_POINTER(QTKit, QTMovieURLAttribute, NSString *)
SOFT_LINK_POINTER(QTKit, QTMovieVolumeDidChangeNotification, NSString *)
SOFT_LINK_POINTER(QTKit, QTSecurityPolicyNoCrossSiteAttribute, NSString *)
SOFT_LINK_POINTER(QTKit, QTSecurityPolicyNoLocalToRemoteSiteAttribute, NSString *)
SOFT_LINK_POINTER(QTKit, QTSecurityPolicyNoRemoteToLocalSiteAttribute, NSString *)
SOFT_LINK_POINTER(QTKit, QTVideoRendererWebKitOnlyNewImageAvailableNotification, NSString *)
SOFT_LINK_POINTER(QTKit, QTMovieApertureModeClean, NSString *)
SOFT_LINK_POINTER(QTKit, QTMovieApertureModeAttribute, NSString *)

#define QTMovie getQTMovieClass()
#define QTMovieView getQTMovieViewClass()
#define QTMovieLayer getQTMovieLayerClass()

#define QTTrackMediaTypeAttribute getQTTrackMediaTypeAttribute()
#define QTMediaTypeAttribute getQTMediaTypeAttribute()
#define QTMediaTypeBase getQTMediaTypeBase()
#define QTMediaTypeMPEG getQTMediaTypeMPEG()
#define QTMediaTypeSound getQTMediaTypeSound()
#define QTMediaTypeText getQTMediaTypeText()
#define QTMediaTypeVideo getQTMediaTypeVideo()
#define QTMovieAskUnresolvedDataRefsAttribute getQTMovieAskUnresolvedDataRefsAttribute()
#define QTMovieLoopsAttribute getQTMovieLoopsAttribute()
#define QTMovieDataAttribute getQTMovieDataAttribute()
#define QTMovieDataSizeAttribute getQTMovieDataSizeAttribute()
#define QTMovieDidEndNotification getQTMovieDidEndNotification()
#define QTMovieHasVideoAttribute getQTMovieHasVideoAttribute()
#define QTMovieHasAudioAttribute getQTMovieHasAudioAttribute()
#define QTMovieIsActiveAttribute getQTMovieIsActiveAttribute()
#define QTMovieLoadStateAttribute getQTMovieLoadStateAttribute()
#define QTMovieLoadStateDidChangeNotification getQTMovieLoadStateDidChangeNotification()
#define QTMovieNaturalSizeAttribute getQTMovieNaturalSizeAttribute()
#define QTMovieCurrentSizeAttribute getQTMovieCurrentSizeAttribute()
#define QTMoviePreventExternalURLLinksAttribute getQTMoviePreventExternalURLLinksAttribute()
#define QTMovieRateChangesPreservePitchAttribute getQTMovieRateChangesPreservePitchAttribute()
#define QTMovieRateDidChangeNotification getQTMovieRateDidChangeNotification()
#define QTMovieSizeDidChangeNotification getQTMovieSizeDidChangeNotification()
#define QTMovieTimeDidChangeNotification getQTMovieTimeDidChangeNotification()
#define QTMovieTimeScaleAttribute getQTMovieTimeScaleAttribute()
#define QTMovieURLAttribute getQTMovieURLAttribute()
#define QTMovieVolumeDidChangeNotification getQTMovieVolumeDidChangeNotification()
#define QTSecurityPolicyNoCrossSiteAttribute getQTSecurityPolicyNoCrossSiteAttribute()
#define QTSecurityPolicyNoLocalToRemoteSiteAttribute getQTSecurityPolicyNoLocalToRemoteSiteAttribute()
#define QTSecurityPolicyNoRemoteToLocalSiteAttribute getQTSecurityPolicyNoRemoteToLocalSiteAttribute()
#define QTVideoRendererWebKitOnlyNewImageAvailableNotification getQTVideoRendererWebKitOnlyNewImageAvailableNotification()
#define QTMovieApertureModeClean getQTMovieApertureModeClean()
#define QTMovieApertureModeAttribute getQTMovieApertureModeAttribute()

// Older versions of the QTKit header don't have these constants.
#if !defined QTKIT_VERSION_MAX_ALLOWED || QTKIT_VERSION_MAX_ALLOWED <= QTKIT_VERSION_7_0
enum {
    QTMovieLoadStateError = -1L,
    QTMovieLoadStateLoaded  = 2000L,
    QTMovieLoadStatePlayable = 10000L,
    QTMovieLoadStatePlaythroughOK = 20000L,
    QTMovieLoadStateComplete = 100000L
};
#endif

@interface FakeQTMovieView : NSObject
- (WebCoreMovieObserver *)delegate;
@end

using namespace WebCore;
using namespace std;

@interface WebCoreMovieObserver : NSObject
{
    MediaPlayerPrivateQTKit* m_callback;
    NSView* m_view;
    BOOL m_delayCallbacks;
}
-(id)initWithCallback:(MediaPlayerPrivateQTKit*)callback;
-(void)disconnect;
-(void)setView:(NSView*)view;
-(void)repaint;
-(void)setDelayCallbacks:(BOOL)shouldDelay;
-(void)loadStateChanged:(NSNotification *)notification;
-(void)rateChanged:(NSNotification *)notification;
-(void)sizeChanged:(NSNotification *)notification;
-(void)timeChanged:(NSNotification *)notification;
-(void)didEnd:(NSNotification *)notification;
-(void)layerHostChanged:(NSNotification *)notification;
@end

@protocol WebKitVideoRenderingDetails
-(void)setMovie:(id)movie;
-(void)drawInRect:(NSRect)rect;
@end

namespace WebCore {

PassOwnPtr<MediaPlayerPrivateInterface> MediaPlayerPrivateQTKit::create(MediaPlayer* player)
{ 
    return adoptPtr(new MediaPlayerPrivateQTKit(player));
}

void MediaPlayerPrivateQTKit::registerMediaEngine(MediaEngineRegistrar registrar)
{
    if (isAvailable())
        registrar(create, getSupportedTypes, supportsType, getSitesInMediaCache, clearMediaCache, clearMediaCacheForSite);
}

MediaPlayerPrivateQTKit::MediaPlayerPrivateQTKit(MediaPlayer* player)
    : m_player(player)
    , m_objcObserver(AdoptNS, [[WebCoreMovieObserver alloc] initWithCallback:this])
    , m_seekTo(-1)
    , m_seekTimer(this, &MediaPlayerPrivateQTKit::seekTimerFired)
    , m_networkState(MediaPlayer::Empty)
    , m_readyState(MediaPlayer::HaveNothing)
    , m_rect()
    , m_scaleFactor(1, 1)
    , m_enabledTrackCount(0)
    , m_totalTrackCount(0)
    , m_reportedDuration(-1)
    , m_cachedDuration(-1)
    , m_timeToRestore(-1)
    , m_preload(MediaPlayer::Auto)
    , m_startedPlaying(false)
    , m_isStreaming(false)
    , m_visible(false)
    , m_hasUnsupportedTracks(false)
    , m_videoFrameHasDrawn(false)
    , m_isAllowedToRender(false)
    , m_privateBrowsing(false)
#if DRAW_FRAME_RATE
    , m_frameCountWhilePlaying(0)
    , m_timeStartedPlaying(0)
    , m_timeStoppedPlaying(0)
#endif
{
}

MediaPlayerPrivateQTKit::~MediaPlayerPrivateQTKit()
{
    tearDownVideoRendering();

    [[NSNotificationCenter defaultCenter] removeObserver:m_objcObserver.get()];
    [m_objcObserver.get() disconnect];
}

NSMutableDictionary *MediaPlayerPrivateQTKit::commonMovieAttributes() 
{
    NSMutableDictionary *movieAttributes = [NSMutableDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithBool:m_player->preservesPitch()], QTMovieRateChangesPreservePitchAttribute,
            [NSNumber numberWithBool:YES], QTMoviePreventExternalURLLinksAttribute,
            [NSNumber numberWithBool:NO], QTSecurityPolicyNoCrossSiteAttribute,
            [NSNumber numberWithBool:YES], QTSecurityPolicyNoRemoteToLocalSiteAttribute,
            [NSNumber numberWithBool:YES], QTSecurityPolicyNoLocalToRemoteSiteAttribute,
            [NSNumber numberWithBool:NO], QTMovieAskUnresolvedDataRefsAttribute,
            [NSNumber numberWithBool:NO], QTMovieLoopsAttribute,
            [NSNumber numberWithBool:!m_privateBrowsing], @"QTMovieAllowPersistentCacheAttribute",
            QTMovieApertureModeClean, QTMovieApertureModeAttribute,
            nil];

    if (m_preload < MediaPlayer::Auto)
        [movieAttributes setValue:[NSNumber numberWithBool:YES] forKey:@"QTMovieLimitReadAheadAttribute"];

    return movieAttributes;
}

void MediaPlayerPrivateQTKit::createQTMovie(const String& url)
{
    NSURL *cocoaURL = KURL(ParsedURLString, url);
    NSMutableDictionary *movieAttributes = commonMovieAttributes();    
    [movieAttributes setValue:cocoaURL forKey:QTMovieURLAttribute];

#if !defined(BUILDING_ON_LEOPARD)
    CFDictionaryRef proxySettings = CFNetworkCopySystemProxySettings();
    CFArrayRef proxiesForURL = CFNetworkCopyProxiesForURL((CFURLRef)cocoaURL, proxySettings);
    BOOL willUseProxy = YES;
    
    if (!proxiesForURL || !CFArrayGetCount(proxiesForURL))
        willUseProxy = NO;
    
    if (CFArrayGetCount(proxiesForURL) == 1) {
        CFDictionaryRef proxy = (CFDictionaryRef)CFArrayGetValueAtIndex(proxiesForURL, 0);
        ASSERT(CFGetTypeID(proxy) == CFDictionaryGetTypeID());
        
        CFStringRef proxyType = (CFStringRef)CFDictionaryGetValue(proxy, kCFProxyTypeKey);
        ASSERT(CFGetTypeID(proxyType) == CFStringGetTypeID());
        
        if (CFStringCompare(proxyType, kCFProxyTypeNone, 0) == kCFCompareEqualTo)
            willUseProxy = NO;
    }

    if (!willUseProxy) {
        // Only pass the QTMovieOpenForPlaybackAttribute flag if there are no proxy servers, due
        // to rdar://problem/7531776.
        [movieAttributes setObject:[NSNumber numberWithBool:YES] forKey:@"QTMovieOpenForPlaybackAttribute"];
    }
    
    if (proxiesForURL)
        CFRelease(proxiesForURL);
    if (proxySettings)
        CFRelease(proxySettings);
#endif
    
    createQTMovie(cocoaURL, movieAttributes);
}

void MediaPlayerPrivateQTKit::createQTMovie(ApplicationCacheResource* resource)
{
#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    ASSERT(resource);

    NSMutableDictionary *movieAttributes = commonMovieAttributes();    
    [movieAttributes setObject:[NSNumber numberWithBool:YES] forKey:@"QTMovieOpenForPlaybackAttribute"];
    
    // ApplicationCacheResources can supply either a data pointer, or a path to a locally cached 
    // flat file.  We would prefer the path over the data, but QTKit can handle either:
    String localPath = resource->path();
    NSURL* cocoaURL = !localPath.isEmpty() ? [NSURL fileURLWithPath:localPath isDirectory:NO] : nil;
    if (cocoaURL)
        [movieAttributes setValue:cocoaURL forKey:QTMovieURLAttribute];
    else {
        NSData* movieData = resource->data()->createNSData();
        [movieAttributes setValue:movieData forKey:QTMovieDataAttribute];
        [movieData release];
    }
    
    createQTMovie(cocoaURL, movieAttributes);
    
#else
    ASSERT_NOT_REACHED();
#endif
}

static void disableComponentsOnce()
{
    static bool sComponentsDisabled = false;
    if (sComponentsDisabled)
        return;
    sComponentsDisabled = true;

    // eat/PDF and grip/PDF components must be disabled twice since they are registered twice
    // with different flags.  However, there is currently a bug in 64-bit QTKit (<rdar://problem/8378237>)
    // which causes subsequent disable component requests of exactly the same type to be ignored if
    // QTKitServer has not yet started.  As a result, we must pass in exactly the flags we want to
    // disable per component.  As a failsafe, if in the future these flags change, we will disable the
    // PDF components for a third time with a wildcard flags field:
    uint32_t componentsToDisable[11][5] = {
        {'eat ', 'TEXT', 'text', 0, 0},
        {'eat ', 'TXT ', 'text', 0, 0},    
        {'eat ', 'utxt', 'text', 0, 0},  
        {'eat ', 'TEXT', 'tx3g', 0, 0},  
        {'eat ', 'PDF ', 'vide', 0x44802, 0},
        {'eat ', 'PDF ', 'vide', 0x45802, 0},
        {'eat ', 'PDF ', 'vide', 0, 0},  
        {'grip', 'PDF ', 'appl', 0x844a00, 0},
        {'grip', 'PDF ', 'appl', 0x845a00, 0},
        {'grip', 'PDF ', 'appl', 0, 0},  
        {'imdc', 'pdf ', 'appl', 0, 0},  
    };

    for (size_t i = 0; i < WTF_ARRAY_LENGTH(componentsToDisable); ++i) 
        wkQTMovieDisableComponent(componentsToDisable[i]);
}

void MediaPlayerPrivateQTKit::createQTMovie(NSURL *url, NSDictionary *movieAttributes)
{
    disableComponentsOnce();

    [[NSNotificationCenter defaultCenter] removeObserver:m_objcObserver.get()];
    
    bool recreating = false;
    if (m_qtMovie) {
        recreating = true;
        destroyQTVideoRenderer();
        m_qtMovie = 0;
    }
    
    // Disable rtsp streams for now, <rdar://problem/5693967>
    if (protocolIs([url scheme], "rtsp"))
        return;
    
    NSError *error = nil;
    m_qtMovie.adoptNS([[QTMovie alloc] initWithAttributes:movieAttributes error:&error]);
    
    if (!m_qtMovie)
        return;
    
    [m_qtMovie.get() setVolume:m_player->volume()];

    if (recreating && hasVideo())
        createQTVideoRenderer(QTVideoRendererModeListensForNewImages);
    
    [[NSNotificationCenter defaultCenter] addObserver:m_objcObserver.get()
                                             selector:@selector(loadStateChanged:) 
                                                 name:QTMovieLoadStateDidChangeNotification 
                                               object:m_qtMovie.get()];

    // In updateState(), we track when maxTimeLoaded() == duration().
    // In newer version of QuickTime, a notification is emitted when maxTimeLoaded changes.
    // In older version of QuickTime, QTMovieLoadStateDidChangeNotification be fired.
    if (NSString *maxTimeLoadedChangeNotification = wkQTMovieMaxTimeLoadedChangeNotification()) {
        [[NSNotificationCenter defaultCenter] addObserver:m_objcObserver.get()
                                                 selector:@selector(loadStateChanged:) 
                                                     name:maxTimeLoadedChangeNotification
                                                   object:m_qtMovie.get()];        
    }

    [[NSNotificationCenter defaultCenter] addObserver:m_objcObserver.get()
                                             selector:@selector(rateChanged:) 
                                                 name:QTMovieRateDidChangeNotification 
                                               object:m_qtMovie.get()];
    [[NSNotificationCenter defaultCenter] addObserver:m_objcObserver.get()
                                             selector:@selector(sizeChanged:) 
                                                 name:QTMovieSizeDidChangeNotification 
                                               object:m_qtMovie.get()];
    [[NSNotificationCenter defaultCenter] addObserver:m_objcObserver.get()
                                             selector:@selector(timeChanged:) 
                                                 name:QTMovieTimeDidChangeNotification 
                                               object:m_qtMovie.get()];
    [[NSNotificationCenter defaultCenter] addObserver:m_objcObserver.get()
                                             selector:@selector(didEnd:) 
                                                 name:QTMovieDidEndNotification 
                                               object:m_qtMovie.get()];
#if defined(BUILDING_ON_SNOW_LEOPARD)
    [[NSNotificationCenter defaultCenter] addObserver:m_objcObserver.get()
                                             selector:@selector(layerHostChanged:)
                                                 name:@"WebKitLayerHostChanged"
                                               object:nil];
#endif
}

static void mainThreadSetNeedsDisplay(id self, SEL)
{
    id view = [self superview];
    ASSERT(!view || [view isKindOfClass:[QTMovieView class]]);
    if (!view || ![view isKindOfClass:[QTMovieView class]])
        return;

    FakeQTMovieView *movieView = static_cast<FakeQTMovieView *>(view);
    WebCoreMovieObserver* delegate = [movieView delegate];
    ASSERT(!delegate || [delegate isKindOfClass:[WebCoreMovieObserver class]]);
    if (!delegate || ![delegate isKindOfClass:[WebCoreMovieObserver class]])
        return;

    [delegate repaint];
}

static Class QTVideoRendererClass()
{
     static Class QTVideoRendererWebKitOnlyClass = NSClassFromString(@"QTVideoRendererWebKitOnly");
     return QTVideoRendererWebKitOnlyClass;
}

void MediaPlayerPrivateQTKit::createQTMovieView()
{
    detachQTMovieView();

    static bool addedCustomMethods = false;
    if (!m_player->inMediaDocument() && !addedCustomMethods) {
        Class QTMovieContentViewClass = NSClassFromString(@"QTMovieContentView");
        ASSERT(QTMovieContentViewClass);

        Method mainThreadSetNeedsDisplayMethod = class_getInstanceMethod(QTMovieContentViewClass, @selector(_mainThreadSetNeedsDisplay));
        ASSERT(mainThreadSetNeedsDisplayMethod);

        method_setImplementation(mainThreadSetNeedsDisplayMethod, reinterpret_cast<IMP>(mainThreadSetNeedsDisplay));
        addedCustomMethods = true;
    }

    // delay callbacks as we *will* get notifications during setup
    [m_objcObserver.get() setDelayCallbacks:YES];

    m_qtMovieView.adoptNS([[QTMovieView alloc] init]);
    setSize(m_player->size());
    NSView* parentView = 0;
#if PLATFORM(MAC)
    parentView = m_player->frameView()->documentView();
    [parentView addSubview:m_qtMovieView.get()];
#endif
    [m_qtMovieView.get() setDelegate:m_objcObserver.get()];
    [m_objcObserver.get() setView:m_qtMovieView.get()];
    [m_qtMovieView.get() setMovie:m_qtMovie.get()];
    [m_qtMovieView.get() setControllerVisible:NO];
    [m_qtMovieView.get() setPreservesAspectRatio:NO];
    // the area not covered by video should be transparent
    [m_qtMovieView.get() setFillColor:[NSColor clearColor]];

    // If we're in a media document, allow QTMovieView to render in its default mode;
    // otherwise tell it to draw synchronously.
    // Note that we expect mainThreadSetNeedsDisplay to be invoked only when synchronous drawing is requested.
    if (!m_player->inMediaDocument())
        wkQTMovieViewSetDrawSynchronously(m_qtMovieView.get(), YES);

    [m_objcObserver.get() setDelayCallbacks:NO];
}

void MediaPlayerPrivateQTKit::detachQTMovieView()
{
    if (m_qtMovieView) {
        [m_objcObserver.get() setView:nil];
        [m_qtMovieView.get() setDelegate:nil];
        [m_qtMovieView.get() removeFromSuperview];
        m_qtMovieView = nil;
    }
}

void MediaPlayerPrivateQTKit::createQTVideoRenderer(QTVideoRendererMode rendererMode)
{
    destroyQTVideoRenderer();

    m_qtVideoRenderer.adoptNS([[QTVideoRendererClass() alloc] init]);
    if (!m_qtVideoRenderer)
        return;
    
    // associate our movie with our instance of QTVideoRendererWebKitOnly
    [(id<WebKitVideoRenderingDetails>)m_qtVideoRenderer.get() setMovie:m_qtMovie.get()];

    if (rendererMode == QTVideoRendererModeListensForNewImages) {
        // listen to QTVideoRendererWebKitOnly's QTVideoRendererWebKitOnlyNewImageDidBecomeAvailableNotification
        [[NSNotificationCenter defaultCenter] addObserver:m_objcObserver.get()
                                                 selector:@selector(newImageAvailable:)
                                                     name:QTVideoRendererWebKitOnlyNewImageAvailableNotification
                                                   object:m_qtVideoRenderer.get()];
    }
}

void MediaPlayerPrivateQTKit::destroyQTVideoRenderer()
{
    if (!m_qtVideoRenderer)
        return;

    // stop observing the renderer's notifications before we toss it
    [[NSNotificationCenter defaultCenter] removeObserver:m_objcObserver.get()
                                                    name:QTVideoRendererWebKitOnlyNewImageAvailableNotification
                                                  object:m_qtVideoRenderer.get()];

    // disassociate our movie from our instance of QTVideoRendererWebKitOnly
    [(id<WebKitVideoRenderingDetails>)m_qtVideoRenderer.get() setMovie:nil];    

    m_qtVideoRenderer = nil;
}

void MediaPlayerPrivateQTKit::createQTMovieLayer()
{
#if USE(ACCELERATED_COMPOSITING) && !(PLATFORM(QT) && USE(QTKIT))
    if (!m_qtMovie)
        return;

    ASSERT(supportsAcceleratedRendering());
    
    if (!m_qtVideoLayer) {
        m_qtVideoLayer.adoptNS([[QTMovieLayer alloc] init]);
        if (!m_qtVideoLayer)
            return;

        [m_qtVideoLayer.get() setMovie:m_qtMovie.get()];
#ifndef NDEBUG
        [(CALayer *)m_qtVideoLayer.get() setName:@"Video layer"];
#endif
        // The layer will get hooked up via RenderLayerBacking::updateGraphicsLayerConfiguration().
    }
#endif
}

void MediaPlayerPrivateQTKit::destroyQTMovieLayer()
{
#if USE(ACCELERATED_COMPOSITING)
    if (!m_qtVideoLayer)
        return;

    // disassociate our movie from our instance of QTMovieLayer
    [m_qtVideoLayer.get() setMovie:nil];    
    m_qtVideoLayer = nil;
#endif
}

MediaPlayerPrivateQTKit::MediaRenderingMode MediaPlayerPrivateQTKit::currentRenderingMode() const
{
    if (m_qtMovieView)
        return MediaRenderingMovieView;
    
    if (m_qtVideoLayer)
        return MediaRenderingMovieLayer;

    if (m_qtVideoRenderer)
        return MediaRenderingSoftwareRenderer;
    
    return MediaRenderingNone;
}

MediaPlayerPrivateQTKit::MediaRenderingMode MediaPlayerPrivateQTKit::preferredRenderingMode() const
{
    if (!m_player->frameView() || !m_qtMovie)
        return MediaRenderingNone;

#if USE(ACCELERATED_COMPOSITING) && !(PLATFORM(QT) && USE(QTKIT))
    if (supportsAcceleratedRendering() && m_player->mediaPlayerClient()->mediaPlayerRenderingCanBeAccelerated(m_player))
        return MediaRenderingMovieLayer;
#endif

    if (!QTVideoRendererClass())
        return MediaRenderingMovieView;
    
    return MediaRenderingSoftwareRenderer;
}

void MediaPlayerPrivateQTKit::setUpVideoRendering()
{
    if (!isReadyForVideoSetup())
        return;

    MediaRenderingMode currentMode = currentRenderingMode();
    MediaRenderingMode preferredMode = preferredRenderingMode();
    if (currentMode == preferredMode && currentMode != MediaRenderingNone)
        return;

    if (currentMode != MediaRenderingNone)  
        tearDownVideoRendering();

    switch (preferredMode) {
    case MediaRenderingMovieView:
        createQTMovieView();
        break;
    case MediaRenderingNone:
    case MediaRenderingSoftwareRenderer:
        createQTVideoRenderer(QTVideoRendererModeListensForNewImages);
        break;
    case MediaRenderingMovieLayer:
        createQTMovieLayer();
        break;
    }

    // If using a movie layer, inform the client so the compositing tree is updated.
    if (currentMode == MediaRenderingMovieLayer || preferredMode == MediaRenderingMovieLayer)
        m_player->mediaPlayerClient()->mediaPlayerRenderingModeChanged(m_player);
}

void MediaPlayerPrivateQTKit::tearDownVideoRendering()
{
    if (m_qtMovieView)
        detachQTMovieView();
    if (m_qtVideoRenderer)
        destroyQTVideoRenderer();
    if (m_qtVideoLayer)
        destroyQTMovieLayer();
}

bool MediaPlayerPrivateQTKit::hasSetUpVideoRendering() const
{
    return m_qtMovieView
        || m_qtVideoLayer
        || m_qtVideoRenderer;
}

QTTime MediaPlayerPrivateQTKit::createQTTime(float time) const
{
    if (!metaDataAvailable())
        return QTMakeTime(0, 600);
    long timeScale = [[m_qtMovie.get() attributeForKey:QTMovieTimeScaleAttribute] longValue];
    return QTMakeTime(lroundf(time * timeScale), timeScale);
}

void MediaPlayerPrivateQTKit::resumeLoad()
{
    if (!m_movieURL.isNull())
        loadInternal(m_movieURL);
}

void MediaPlayerPrivateQTKit::load(const String& url)
{
    m_movieURL = url;

    // If the element is not supposed to load any data return immediately.
    if (m_preload == MediaPlayer::None)
        return;

    loadInternal(url);
}

void MediaPlayerPrivateQTKit::loadInternal(const String& url)
{
    if (m_networkState != MediaPlayer::Loading) {
        m_networkState = MediaPlayer::Loading;
        m_player->networkStateChanged();
    }
    if (m_readyState != MediaPlayer::HaveNothing) {
        m_readyState = MediaPlayer::HaveNothing;
        m_player->readyStateChanged();
    }
    cancelSeek();
    m_videoFrameHasDrawn = false;
    
    [m_objcObserver.get() setDelayCallbacks:YES];

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    Frame* frame = m_player->frameView() ? m_player->frameView()->frame() : NULL;
    ApplicationCacheHost* cacheHost = frame ? frame->loader()->documentLoader()->applicationCacheHost() : NULL;
    ApplicationCacheResource* resource = NULL;
    if (cacheHost && cacheHost->shouldLoadResourceFromApplicationCache(ResourceRequest(url), resource) && resource)
        createQTMovie(resource);
    else
#endif    
    createQTMovie(url);

    [m_objcObserver.get() loadStateChanged:nil];
    [m_objcObserver.get() setDelayCallbacks:NO];
}

void MediaPlayerPrivateQTKit::prepareToPlay()
{
    setPreload(MediaPlayer::Auto);
}

PlatformMedia MediaPlayerPrivateQTKit::platformMedia() const
{
    PlatformMedia pm;
    pm.type = PlatformMedia::QTMovieType;
    pm.media.qtMovie = m_qtMovie.get();
    return pm;
}

#if USE(ACCELERATED_COMPOSITING) && !(PLATFORM(QT) && USE(QTKIT))
PlatformLayer* MediaPlayerPrivateQTKit::platformLayer() const
{
    return m_qtVideoLayer.get();
}
#endif

void MediaPlayerPrivateQTKit::play()
{
    if (!metaDataAvailable())
        return;
    m_startedPlaying = true;
#if DRAW_FRAME_RATE
    m_frameCountWhilePlaying = 0;
#endif
    [m_objcObserver.get() setDelayCallbacks:YES];
    [m_qtMovie.get() setRate:m_player->rate()];
    [m_objcObserver.get() setDelayCallbacks:NO];
}

void MediaPlayerPrivateQTKit::pause()
{
    if (!metaDataAvailable())
        return;
    m_startedPlaying = false;
#if DRAW_FRAME_RATE
    m_timeStoppedPlaying = [NSDate timeIntervalSinceReferenceDate];
#endif
    [m_objcObserver.get() setDelayCallbacks:YES];
    [m_qtMovie.get() stop];
    [m_objcObserver.get() setDelayCallbacks:NO];
}

float MediaPlayerPrivateQTKit::duration() const
{
    if (!metaDataAvailable())
        return 0;

    if (m_cachedDuration != -1.0f)
        return m_cachedDuration;

    QTTime time = [m_qtMovie.get() duration];
    if (time.flags == kQTTimeIsIndefinite)
        return numeric_limits<float>::infinity();
    return static_cast<float>(time.timeValue) / time.timeScale;
}

float MediaPlayerPrivateQTKit::currentTime() const
{
    if (!metaDataAvailable())
        return 0;
    QTTime time = [m_qtMovie.get() currentTime];
    return static_cast<float>(time.timeValue) / time.timeScale;
}

void MediaPlayerPrivateQTKit::seek(float time)
{
    // Nothing to do if we are already in the middle of a seek to the same time.
    if (time == m_seekTo)
        return;

    cancelSeek();
    
    if (!metaDataAvailable())
        return;
    
    if (time > duration())
        time = duration();

    m_seekTo = time;
    if (maxTimeSeekable() >= m_seekTo)
        doSeek();
    else 
        m_seekTimer.start(0, 0.5f);
}

void MediaPlayerPrivateQTKit::doSeek() 
{
    QTTime qttime = createQTTime(m_seekTo);
    // setCurrentTime generates several event callbacks, update afterwards
    [m_objcObserver.get() setDelayCallbacks:YES];
    float oldRate = [m_qtMovie.get() rate];

    if (oldRate)
        [m_qtMovie.get() setRate:0];
    [m_qtMovie.get() setCurrentTime:qttime];

    // restore playback only if not at end, otherwise QTMovie will loop
    float timeAfterSeek = currentTime();
    if (oldRate && timeAfterSeek < duration())
        [m_qtMovie.get() setRate:oldRate];

    cancelSeek();
    [m_objcObserver.get() setDelayCallbacks:NO];
}

void MediaPlayerPrivateQTKit::cancelSeek()
{
    m_seekTo = -1;
    m_seekTimer.stop();
}

void MediaPlayerPrivateQTKit::seekTimerFired(Timer<MediaPlayerPrivateQTKit>*)
{        
    if (!metaDataAvailable()|| !seeking() || currentTime() == m_seekTo) {
        cancelSeek();
        updateStates();
        m_player->timeChanged(); 
        return;
    } 

    if (maxTimeSeekable() >= m_seekTo)
        doSeek();
    else {
        MediaPlayer::NetworkState state = networkState();
        if (state == MediaPlayer::Empty || state == MediaPlayer::Loaded) {
            cancelSeek();
            updateStates();
            m_player->timeChanged();
        }
    }
}

bool MediaPlayerPrivateQTKit::paused() const
{
    if (!metaDataAvailable())
        return true;
    return [m_qtMovie.get() rate] == 0;
}

bool MediaPlayerPrivateQTKit::seeking() const
{
    if (!metaDataAvailable())
        return false;
    return m_seekTo >= 0;
}

IntSize MediaPlayerPrivateQTKit::naturalSize() const
{
    if (!metaDataAvailable())
        return IntSize();

    // In spite of the name of this method, return QTMovieNaturalSizeAttribute transformed by the 
    // initial movie scale because the spec says intrinsic size is:
    //
    //    ... the dimensions of the resource in CSS pixels after taking into account the resource's 
    //    dimensions, aspect ratio, clean aperture, resolution, and so forth, as defined for the 
    //    format used by the resource
    
    FloatSize naturalSize([[m_qtMovie.get() attributeForKey:QTMovieNaturalSizeAttribute] sizeValue]);
    if (naturalSize.isEmpty() && m_isStreaming) {
        // HTTP Live Streams will occasionally return {0,0} natural sizes while scrubbing.
        // Work around this problem (<rdar://problem/9078563>) by returning the last valid 
        // cached natural size:
        naturalSize = m_cachedNaturalSize;
    } else {
        // Unfortunately, due to another QTKit bug (<rdar://problem/9082071>) we won't get a sizeChanged
        // event when this happens, so we must cache the last valid naturalSize here:
        m_cachedNaturalSize = naturalSize;
    }
        
    return IntSize(naturalSize.width() * m_scaleFactor.width(), naturalSize.height() * m_scaleFactor.height());
}

bool MediaPlayerPrivateQTKit::hasVideo() const
{
    if (!metaDataAvailable())
        return false;
    return [[m_qtMovie.get() attributeForKey:QTMovieHasVideoAttribute] boolValue];
}

bool MediaPlayerPrivateQTKit::hasAudio() const
{
    if (!m_qtMovie)
        return false;
    return [[m_qtMovie.get() attributeForKey:QTMovieHasAudioAttribute] boolValue];
}

bool MediaPlayerPrivateQTKit::supportsFullscreen() const
{
#ifndef BUILDING_ON_LEOPARD
    return true;
#else
    // See <rdar://problem/7389945>
    return false;
#endif
}

void MediaPlayerPrivateQTKit::setVolume(float volume)
{
    if (m_qtMovie)
        [m_qtMovie.get() setVolume:volume];  
}

bool MediaPlayerPrivateQTKit::hasClosedCaptions() const
{
    if (!metaDataAvailable())
        return false;
    return wkQTMovieHasClosedCaptions(m_qtMovie.get());  
}

void MediaPlayerPrivateQTKit::setClosedCaptionsVisible(bool closedCaptionsVisible)
{
    if (metaDataAvailable()) {
        wkQTMovieSetShowClosedCaptions(m_qtMovie.get(), closedCaptionsVisible);

#if USE(ACCELERATED_COMPOSITING) && !defined(BUILDING_ON_LEOPARD)
    if (closedCaptionsVisible && m_qtVideoLayer) {
        // Captions will be rendered upside down unless we flag the movie as flipped (again). See <rdar://7408440>.
        [m_qtVideoLayer.get() setGeometryFlipped:YES];
    }
#endif
    }
}

void MediaPlayerPrivateQTKit::setRate(float rate)
{
    if (m_qtMovie)
        [m_qtMovie.get() setRate:rate];
}

void MediaPlayerPrivateQTKit::setPreservesPitch(bool preservesPitch)
{
    if (!m_qtMovie)
        return;

    // QTMovieRateChangesPreservePitchAttribute cannot be changed dynamically after QTMovie creation.
    // If the passed in value is different than what already exists, we need to recreate the QTMovie for it to take effect.
    if ([[m_qtMovie.get() attributeForKey:QTMovieRateChangesPreservePitchAttribute] boolValue] == preservesPitch)
        return;

    RetainPtr<NSDictionary> movieAttributes(AdoptNS, [[m_qtMovie.get() movieAttributes] mutableCopy]);
    ASSERT(movieAttributes);
    [movieAttributes.get() setValue:[NSNumber numberWithBool:preservesPitch] forKey:QTMovieRateChangesPreservePitchAttribute];
    m_timeToRestore = currentTime();

    createQTMovie([movieAttributes.get() valueForKey:QTMovieURLAttribute], movieAttributes.get());
}

PassRefPtr<TimeRanges> MediaPlayerPrivateQTKit::buffered() const
{
    RefPtr<TimeRanges> timeRanges = TimeRanges::create();
    float loaded = maxTimeLoaded();
    if (loaded > 0)
        timeRanges->add(0, loaded);
    return timeRanges.release();
}

float MediaPlayerPrivateQTKit::maxTimeSeekable() const
{
    if (!metaDataAvailable())
        return 0;

    // infinite duration means live stream
    if (isinf(duration()))
        return 0;

    return wkQTMovieMaxTimeSeekable(m_qtMovie.get());
}

float MediaPlayerPrivateQTKit::maxTimeLoaded() const
{
    if (!metaDataAvailable())
        return 0;
    return wkQTMovieMaxTimeLoaded(m_qtMovie.get()); 
}

unsigned MediaPlayerPrivateQTKit::bytesLoaded() const
{
    float dur = duration();
    if (!dur)
        return 0;
    return totalBytes() * maxTimeLoaded() / dur;
}

unsigned MediaPlayerPrivateQTKit::totalBytes() const
{
    if (!metaDataAvailable())
        return 0;
    return [[m_qtMovie.get() attributeForKey:QTMovieDataSizeAttribute] intValue];
}

void MediaPlayerPrivateQTKit::cancelLoad()
{
    // FIXME: Is there a better way to check for this?
    if (m_networkState < MediaPlayer::Loading || m_networkState == MediaPlayer::Loaded)
        return;
    
    tearDownVideoRendering();
    m_qtMovie = nil;
    
    updateStates();
}

void MediaPlayerPrivateQTKit::cacheMovieScale()
{
    NSSize initialSize = NSZeroSize;
    NSSize naturalSize = [[m_qtMovie.get() attributeForKey:QTMovieNaturalSizeAttribute] sizeValue];

#ifndef BUILDING_ON_LEOPARD
    // QTMovieCurrentSizeAttribute is not allowed with instances of QTMovie that have been 
    // opened with QTMovieOpenForPlaybackAttribute, so ask for the display transform attribute instead.
    NSAffineTransform *displayTransform = [m_qtMovie.get() attributeForKey:@"QTMoviePreferredTransformAttribute"];
    if (displayTransform)
        initialSize = [displayTransform transformSize:naturalSize];
    else {
        initialSize.width = naturalSize.width;
        initialSize.height = naturalSize.height;
    }
#else
    initialSize = [[m_qtMovie.get() attributeForKey:QTMovieCurrentSizeAttribute] sizeValue];
#endif

    if (naturalSize.width)
        m_scaleFactor.setWidth(initialSize.width / naturalSize.width);
    if (naturalSize.height)
        m_scaleFactor.setHeight(initialSize.height / naturalSize.height);
}

bool MediaPlayerPrivateQTKit::isReadyForVideoSetup() const
{
    return m_readyState >= MediaPlayer::HaveMetadata && m_player->visible();
}

void MediaPlayerPrivateQTKit::prepareForRendering()
{
    if (m_isAllowedToRender)
        return;
    m_isAllowedToRender = true;

    if (!hasSetUpVideoRendering())
        setUpVideoRendering();

    // If using a movie layer, inform the client so the compositing tree is updated. This is crucial if the movie
    // has a poster, as it will most likely not have a layer and we will now be rendering frames to the movie layer.
    if (currentRenderingMode() == MediaRenderingMovieLayer || preferredRenderingMode() == MediaRenderingMovieLayer)
        m_player->mediaPlayerClient()->mediaPlayerRenderingModeChanged(m_player);
}

void MediaPlayerPrivateQTKit::updateStates()
{
    MediaPlayer::NetworkState oldNetworkState = m_networkState;
    MediaPlayer::ReadyState oldReadyState = m_readyState;
    
    long loadState = m_qtMovie ? [[m_qtMovie.get() attributeForKey:QTMovieLoadStateAttribute] longValue] : static_cast<long>(QTMovieLoadStateError);

    if (loadState >= QTMovieLoadStateLoaded && m_readyState < MediaPlayer::HaveMetadata) {
        disableUnsupportedTracks();
        if (m_player->inMediaDocument()) {
            if (!m_enabledTrackCount || m_hasUnsupportedTracks) {
                // This has a type of media that we do not handle directly with a <video> 
                // element, eg. a rtsp track or QuickTime VR. Tell the MediaPlayerClient
                // that we noticed.
                sawUnsupportedTracks();
                return;
            }
        } else if (!m_enabledTrackCount)
            loadState = QTMovieLoadStateError;

        if (loadState != QTMovieLoadStateError) {
            wkQTMovieSelectPreferredAlternates(m_qtMovie.get());
            cacheMovieScale();
            MediaPlayer::MovieLoadType movieType = movieLoadType();
            m_isStreaming = movieType == MediaPlayer::StoredStream || movieType == MediaPlayer::LiveStream;
        }
    }
    
    // If this movie is reloading and we mean to restore the current time/rate, this might be the right time to do it.
    if (loadState >= QTMovieLoadStateLoaded && oldNetworkState < MediaPlayer::Loaded && m_timeToRestore != -1.0f) {
        QTTime qttime = createQTTime(m_timeToRestore);
        m_timeToRestore = -1.0f;
            
        // Disable event callbacks from setCurrentTime for restoring time in a recreated video
        [m_objcObserver.get() setDelayCallbacks:YES];
        [m_qtMovie.get() setCurrentTime:qttime];
        [m_qtMovie.get() setRate:m_player->rate()];
        [m_objcObserver.get() setDelayCallbacks:NO];
    }

    BOOL completelyLoaded = !m_isStreaming && (loadState >= QTMovieLoadStateComplete);

    // Note: QT indicates that we are fully loaded with QTMovieLoadStateComplete.
    // However newer versions of QT do not, so we check maxTimeLoaded against duration.
    if (!completelyLoaded && !m_isStreaming && metaDataAvailable())
        completelyLoaded = maxTimeLoaded() == duration();

    if (completelyLoaded) {
        // "Loaded" is reserved for fully buffered movies, never the case when streaming
        m_networkState = MediaPlayer::Loaded;
        m_readyState = MediaPlayer::HaveEnoughData;
    } else if (loadState >= QTMovieLoadStatePlaythroughOK) {
        m_readyState = MediaPlayer::HaveEnoughData;
        m_networkState = MediaPlayer::Loading;
    } else if (loadState >= QTMovieLoadStatePlayable) {
        // FIXME: This might not work correctly in streaming case, <rdar://problem/5693967>
        m_readyState = currentTime() < maxTimeLoaded() ? MediaPlayer::HaveFutureData : MediaPlayer::HaveCurrentData;
        m_networkState = MediaPlayer::Loading;
    } else if (loadState >= QTMovieLoadStateLoaded) {
        m_readyState = MediaPlayer::HaveMetadata;
        m_networkState = MediaPlayer::Loading;
    } else if (loadState > QTMovieLoadStateError) {
        m_readyState = MediaPlayer::HaveNothing;
        m_networkState = MediaPlayer::Loading;
    } else {
        // Loading or decoding failed.

        if (m_player->inMediaDocument()) {
            // Something went wrong in the loading of media within a standalone file. 
            // This can occur with chained refmovies pointing to streamed media.
            sawUnsupportedTracks();
            return;
        }

        float loaded = maxTimeLoaded();
        if (!loaded)
            m_readyState = MediaPlayer::HaveNothing;

        if (!m_enabledTrackCount)
            m_networkState = MediaPlayer::FormatError;
        else {
            // FIXME: We should differentiate between load/network errors and decode errors <rdar://problem/5605692>
            if (loaded > 0)
                m_networkState = MediaPlayer::DecodeError;
            else
                m_readyState = MediaPlayer::HaveNothing;
        }
    }

    if (isReadyForVideoSetup() && !hasSetUpVideoRendering())
        setUpVideoRendering();

    if (seeking())
        m_readyState = m_readyState >= MediaPlayer::HaveMetadata ? MediaPlayer::HaveMetadata : MediaPlayer::HaveNothing;

    // Streaming movies don't use the network when paused.
    if (m_isStreaming && m_readyState >= MediaPlayer::HaveMetadata && m_networkState >= MediaPlayer::Loading && [m_qtMovie.get() rate] == 0)
        m_networkState = MediaPlayer::Idle;

    if (m_networkState != oldNetworkState)
        m_player->networkStateChanged();

    if (m_readyState != oldReadyState)
        m_player->readyStateChanged();

    if (loadState >= QTMovieLoadStateLoaded) {
        float dur = duration();
        if (dur != m_reportedDuration) {
            if (m_reportedDuration != -1.0f)
                m_player->durationChanged();
            m_reportedDuration = dur;
        }
    }
}

void MediaPlayerPrivateQTKit::loadStateChanged()
{
    if (!m_hasUnsupportedTracks)
        updateStates();
}

void MediaPlayerPrivateQTKit::rateChanged()
{
    if (m_hasUnsupportedTracks)
        return;

    updateStates();
    m_player->rateChanged();
}

void MediaPlayerPrivateQTKit::sizeChanged()
{
    if (!m_hasUnsupportedTracks)
        m_player->sizeChanged();
}

void MediaPlayerPrivateQTKit::timeChanged()
{
    if (m_hasUnsupportedTracks)
        return;

    // It may not be possible to seek to a specific time in a streamed movie. When seeking in a 
    // stream QuickTime sets the movie time to closest time possible and posts a timechanged 
    // notification. Update m_seekTo so we can detect when the seek completes.
    if (m_seekTo != -1)
        m_seekTo = currentTime();

    m_timeToRestore = -1.0f;
    updateStates();
    m_player->timeChanged();
}

void MediaPlayerPrivateQTKit::didEnd()
{
    if (m_hasUnsupportedTracks)
        return;

    m_startedPlaying = false;
#if DRAW_FRAME_RATE
    m_timeStoppedPlaying = [NSDate timeIntervalSinceReferenceDate];
#endif

    // Hang onto the current time and use it as duration from now on since QuickTime is telling us we
    // are at the end. Do this because QuickTime sometimes reports one time for duration and stops
    // playback at another time, which causes problems in HTMLMediaElement. QTKit's 'ended' event 
    // fires when playing in reverse so don't update duration when at time zero!
    float now = currentTime();
    if (now > 0)
        m_cachedDuration = now;

    updateStates();
    m_player->timeChanged();
}

#if USE(ACCELERATED_COMPOSITING) && !(PLATFORM(QT) && USE(QTKIT))
#if defined(BUILDING_ON_SNOW_LEOPARD)
static bool layerIsDescendentOf(PlatformLayer* child, PlatformLayer* descendent)
{
    if (!child || !descendent)
        return false;

    do {
        if (child == descendent)
            return true;
    } while((child = [child superlayer]));

    return false;
}
#endif

void MediaPlayerPrivateQTKit::layerHostChanged(PlatformLayer* rootLayer)
{
#if defined(BUILDING_ON_SNOW_LEOPARD)
    if (!rootLayer)
        return;

    if (layerIsDescendentOf(m_qtVideoLayer.get(), rootLayer)) {
        // We own a child layer of a layer which has switched contexts.  
        // Tear down our layer, and set m_visible to false, so that the 
        // next time setVisible(true) is called, the layer will be re-
        // created in the correct context.
        tearDownVideoRendering();
        m_visible = false;
    }
#else
    UNUSED_PARAM(rootLayer);
#endif
}
#endif

void MediaPlayerPrivateQTKit::setSize(const IntSize&) 
{ 
    // Don't resize the view now because [view setFrame] also resizes the movie itself, and because
    // the renderer calls this function immediately when we report a size change (QTMovieSizeDidChangeNotification)
    // we can get into a feedback loop observing the size change and resetting the size, and this can cause
    // QuickTime to miss resetting a movie's size when the media size changes (as happens with an rtsp movie
    // once the rtsp server sends the track sizes). Instead we remember the size passed to paint() and resize
    // the view when it changes.
    // <rdar://problem/6336092> REGRESSION: rtsp movie does not resize correctly
}

void MediaPlayerPrivateQTKit::setVisible(bool b)
{
    if (m_visible != b) {
        m_visible = b;
        if (b)
            setUpVideoRendering();
        else
            tearDownVideoRendering();
    }
}

bool MediaPlayerPrivateQTKit::hasAvailableVideoFrame() const
{
    // When using a QTMovieLayer return true as soon as the movie reaches QTMovieLoadStatePlayable 
    // because although we don't *know* when the first frame has decoded, by the time we get and 
    // process the notification a frame should have propagated the VisualContext and been set on
    // the layer.
    if (currentRenderingMode() == MediaRenderingMovieLayer)
        return m_readyState >= MediaPlayer::HaveCurrentData;

    // When using the software renderer QuickTime signals that a frame is available so we might as well
    // wait until we know that a frame has been drawn.
    return m_videoFrameHasDrawn;
}

void MediaPlayerPrivateQTKit::repaint()
{
    if (m_hasUnsupportedTracks)
        return;

#if DRAW_FRAME_RATE
    if (m_startedPlaying) {
        m_frameCountWhilePlaying++;
        // to eliminate preroll costs from our calculation,
        // our frame rate calculation excludes the first frame drawn after playback starts
        if (1==m_frameCountWhilePlaying)
            m_timeStartedPlaying = [NSDate timeIntervalSinceReferenceDate];
    }
#endif
    m_videoFrameHasDrawn = true;
    m_player->repaint();
}

void MediaPlayerPrivateQTKit::paintCurrentFrameInContext(GraphicsContext* context, const IntRect& r)
{
    id qtVideoRenderer = m_qtVideoRenderer.get();
    if (!qtVideoRenderer && currentRenderingMode() == MediaRenderingMovieLayer) {
        // We're being told to render into a context, but we already have the
        // MovieLayer going. This probably means we've been called from <canvas>.
        // Set up a QTVideoRenderer to use, but one that doesn't register for
        // update callbacks. That way, it won't bother us asking to repaint.
        createQTVideoRenderer(QTVideoRendererModeDefault);
        qtVideoRenderer = m_qtVideoRenderer.get();
    }
    paint(context, r);
}

void MediaPlayerPrivateQTKit::paint(GraphicsContext* context, const IntRect& r)
{
    if (context->paintingDisabled() || m_hasUnsupportedTracks)
        return;
    NSView *view = m_qtMovieView.get();
    id qtVideoRenderer = m_qtVideoRenderer.get();
    if (!view && !qtVideoRenderer)
        return;

    [m_objcObserver.get() setDelayCallbacks:YES];
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    NSGraphicsContext* newContext;
    FloatSize scaleFactor(1.0f, -1.0f);
    IntRect paintRect(IntPoint(0, 0), IntSize(r.width(), r.height()));

#if PLATFORM(QT) && USE(QTKIT)
    // In Qt, GraphicsContext is a QPainter so every transformations applied on it won't matter because here
    // the video is rendered by QuickTime not by Qt.
    CGContextRef cgContext = static_cast<CGContextRef>([[NSGraphicsContext currentContext] graphicsPort]);
    CGContextSaveGState(cgContext);
    CGContextSetInterpolationQuality(cgContext, kCGInterpolationLow);
    CGContextTranslateCTM(cgContext, r.x(), r.y() + r.height());
    CGContextScaleCTM(cgContext, scaleFactor.width(), scaleFactor.height());

    newContext = [NSGraphicsContext currentContext];
#else
    GraphicsContextStateSaver stateSaver(*context);
    context->translate(r.x(), r.y() + r.height());
    context->scale(scaleFactor);
    context->setImageInterpolationQuality(InterpolationLow);

    newContext = [NSGraphicsContext graphicsContextWithGraphicsPort:context->platformContext() flipped:NO];
#endif
    // draw the current video frame
    if (qtVideoRenderer) {
        [NSGraphicsContext saveGraphicsState];
        [NSGraphicsContext setCurrentContext:newContext];
        [(id<WebKitVideoRenderingDetails>)qtVideoRenderer drawInRect:paintRect];
        [NSGraphicsContext restoreGraphicsState];
    } else {
        if (m_rect != r) {
             m_rect = r;
            if (m_player->inMediaDocument()) {
                // the QTMovieView needs to be placed in the proper location for document mode
                [view setFrame:m_rect];
            }
            else {
                // We don't really need the QTMovieView in any specific location so let's just get it out of the way
                // where it won't intercept events or try to bring up the context menu.
                IntRect farAwayButCorrectSize(m_rect);
                farAwayButCorrectSize.move(-1000000, -1000000);
                [view setFrame:farAwayButCorrectSize];
            }
        }

        if (m_player->inMediaDocument()) {
            // If we're using a QTMovieView in a media document, the view may get layer-backed. AppKit won't update
            // the layer hosting correctly if we call displayRectIgnoringOpacity:inContext:, so use displayRectIgnoringOpacity:
            // in this case. See <rdar://problem/6702882>.
            [view displayRectIgnoringOpacity:paintRect];
        } else
            [view displayRectIgnoringOpacity:paintRect inContext:newContext];
    }

#if DRAW_FRAME_RATE
    // Draw the frame rate only after having played more than 10 frames.
    if (m_frameCountWhilePlaying > 10) {
        Frame* frame = m_player->frameView() ? m_player->frameView()->frame() : NULL;
        Document* document = frame ? frame->document() : NULL;
        RenderObject* renderer = document ? document->renderer() : NULL;
        RenderStyle* styleToUse = renderer ? renderer->style() : NULL;
        if (styleToUse) {
            double frameRate = (m_frameCountWhilePlaying - 1) / ( m_startedPlaying ? ([NSDate timeIntervalSinceReferenceDate] - m_timeStartedPlaying) :
                (m_timeStoppedPlaying - m_timeStartedPlaying) );
            String text = String::format("%1.2f", frameRate);
            TextRun textRun(text.characters(), text.length());
            const Color color(255, 0, 0);
            context->scale(FloatSize(1.0f, -1.0f));    
            context->setStrokeColor(color, styleToUse->colorSpace());
            context->setStrokeStyle(SolidStroke);
            context->setStrokeThickness(1.0f);
            context->setFillColor(color, styleToUse->colorSpace());
            context->drawText(styleToUse->font(), textRun, IntPoint(2, -3));
        }
    }
#endif
#if PLATFORM(QT) && USE(QTKIT)
    CGContextRestoreGState(cgContext);
#endif
    END_BLOCK_OBJC_EXCEPTIONS;
    [m_objcObserver.get() setDelayCallbacks:NO];
}

static void addFileTypesToCache(NSArray * fileTypes, HashSet<String> &cache)
{
    int count = [fileTypes count];
    for (int n = 0; n < count; n++) {
        CFStringRef ext = reinterpret_cast<CFStringRef>([fileTypes objectAtIndex:n]);
        RetainPtr<CFStringRef> uti(AdoptCF, UTTypeCreatePreferredIdentifierForTag(kUTTagClassFilenameExtension, ext, NULL));
        if (!uti)
            continue;
        RetainPtr<CFStringRef> mime(AdoptCF, UTTypeCopyPreferredTagWithClass(uti.get(), kUTTagClassMIMEType));
        if (mime)
            cache.add(mime.get());

        // -movieFileTypes: returns both file extensions and OSTypes. The later are surrounded by single
        // quotes, eg. 'MooV', so don't bother looking at those.
        if (CFStringGetCharacterAtIndex(ext, 0) != '\'') {
            // UTI is missing many media related MIME types supported by QTKit (see rdar://6434168), and not all
            // web servers use the MIME type UTI returns for an extension (see rdar://7875393), so even if UTI 
            // has a type for this extension add any types in hard coded table in the MIME type regsitry.
            Vector<String> typesForExtension = MIMETypeRegistry::getMediaMIMETypesForExtension(ext);
            unsigned count = typesForExtension.size();
            for (unsigned ndx = 0; ndx < count; ++ndx) {
                if (!cache.contains(typesForExtension[ndx]))
                    cache.add(typesForExtension[ndx]);
            }
        }
    }    
}

static HashSet<String> mimeCommonTypesCache()
{
    DEFINE_STATIC_LOCAL(HashSet<String>, cache, ());
    static bool typeListInitialized = false;

    if (!typeListInitialized) {
        typeListInitialized = true;
        NSArray* fileTypes = [QTMovie movieFileTypes:QTIncludeCommonTypes];
        addFileTypesToCache(fileTypes, cache);
    }
    
    return cache;
} 

static HashSet<String> mimeModernTypesCache()
{
    DEFINE_STATIC_LOCAL(HashSet<String>, cache, ());
    static bool typeListInitialized = false;
    
    if (!typeListInitialized) {
        typeListInitialized = true;
        NSArray* fileTypes = [QTMovie movieFileTypes:(QTMovieFileTypeOptions)wkQTIncludeOnlyModernMediaFileTypes()];
        addFileTypesToCache(fileTypes, cache);
    }
    
    return cache;
} 

void MediaPlayerPrivateQTKit::getSupportedTypes(HashSet<String>& supportedTypes)
{
    supportedTypes = mimeModernTypesCache();
    
    // Note: this method starts QTKitServer if it isn't already running when in 64-bit because it has to return the list 
    // of every MIME type supported by QTKit.
    HashSet<String> commonTypes = mimeCommonTypesCache();
    HashSet<String>::const_iterator it = commonTypes.begin();
    HashSet<String>::const_iterator end = commonTypes.end();
    for (; it != end; ++it)
        supportedTypes.add(*it);
} 

MediaPlayer::SupportsType MediaPlayerPrivateQTKit::supportsType(const String& type, const String& codecs)
{
    // Only return "IsSupported" if there is no codecs parameter for now as there is no way to ask QT if it supports an
    // extended MIME type yet.

    // We check the "modern" type cache first, as it doesn't require QTKitServer to start.
    if (mimeModernTypesCache().contains(type) || mimeCommonTypesCache().contains(type))
        return codecs.isEmpty() ? MediaPlayer::MayBeSupported : MediaPlayer::IsSupported;

    return MediaPlayer::IsNotSupported;
}

bool MediaPlayerPrivateQTKit::isAvailable()
{
    // On 10.5 and higher, QuickTime will always be new enough for <video> and <audio> support, so we just check that the framework can be loaded.
    return QTKitLibrary();
}

void MediaPlayerPrivateQTKit::getSitesInMediaCache(Vector<String>& sites) 
{
    NSArray *mediaSites = wkQTGetSitesInMediaDownloadCache();
    for (NSString *site in mediaSites)
        sites.append(site);
}

void MediaPlayerPrivateQTKit::clearMediaCache()
{
    wkQTClearMediaDownloadCache();
}

void MediaPlayerPrivateQTKit::clearMediaCacheForSite(const String& site)
{
    wkQTClearMediaDownloadCacheForSite(site);
}

void MediaPlayerPrivateQTKit::disableUnsupportedTracks()
{
    if (!m_qtMovie) {
        m_enabledTrackCount = 0;
        m_totalTrackCount = 0;
        return;
    }
    
    static HashSet<String>* allowedTrackTypes = 0;
    if (!allowedTrackTypes) {
        allowedTrackTypes = new HashSet<String>;
        allowedTrackTypes->add(QTMediaTypeVideo);
        allowedTrackTypes->add(QTMediaTypeSound);
        allowedTrackTypes->add(QTMediaTypeText);
        allowedTrackTypes->add(QTMediaTypeBase);
        allowedTrackTypes->add(QTMediaTypeMPEG);
        allowedTrackTypes->add("clcp"); // Closed caption
        allowedTrackTypes->add("sbtl"); // Subtitle
        allowedTrackTypes->add("odsm"); // MPEG-4 object descriptor stream
        allowedTrackTypes->add("sdsm"); // MPEG-4 scene description stream
        allowedTrackTypes->add("tmcd"); // timecode
        allowedTrackTypes->add("tc64"); // timcode-64
        allowedTrackTypes->add("tmet"); // timed metadata
    }
    
    NSArray *tracks = [m_qtMovie.get() tracks];
    
    m_totalTrackCount = [tracks count];
    m_enabledTrackCount = m_totalTrackCount;
    for (unsigned trackIndex = 0; trackIndex < m_totalTrackCount; trackIndex++) {
        // Grab the track at the current index. If there isn't one there, then
        // we can move onto the next one.
        QTTrack *track = [tracks objectAtIndex:trackIndex];
        if (!track)
            continue;
        
        // Check to see if the track is disabled already, we should move along.
        // We don't need to re-disable it.
        if (![track isEnabled]) {
            --m_enabledTrackCount;
            continue;
        }
        
        // Get the track's media type.
        NSString *mediaType = [track attributeForKey:QTTrackMediaTypeAttribute];
        if (!mediaType)
            continue;

        // Test whether the media type is in our white list.
        if (!allowedTrackTypes->contains(mediaType)) {
            // If this track type is not allowed, then we need to disable it.
            [track setEnabled:NO];
            --m_enabledTrackCount;
            m_hasUnsupportedTracks = true;
        }

        // Disable chapter tracks. These are most likely to lead to trouble, as
        // they will be composited under the video tracks, forcing QT to do extra
        // work.
        QTTrack *chapterTrack = [track performSelector:@selector(chapterlist)];
        if (!chapterTrack)
            continue;
        
        // Try to grab the media for the track.
        QTMedia *chapterMedia = [chapterTrack media];
        if (!chapterMedia)
            continue;
        
        // Grab the media type for this track.
        id chapterMediaType = [chapterMedia attributeForKey:QTMediaTypeAttribute];
        if (!chapterMediaType)
            continue;
        
        // Check to see if the track is a video track. We don't care about
        // other non-video tracks.
        if (![chapterMediaType isEqual:QTMediaTypeVideo])
            continue;
        
        // Check to see if the track is already disabled. If it is, we
        // should move along.
        if (![chapterTrack isEnabled])
            continue;
        
        // Disable the evil, evil track.
        [chapterTrack setEnabled:NO];
        --m_enabledTrackCount;
        m_hasUnsupportedTracks = true;
    }
}

void MediaPlayerPrivateQTKit::sawUnsupportedTracks()
{
    m_hasUnsupportedTracks = true;
    m_player->mediaPlayerClient()->mediaPlayerSawUnsupportedTracks(m_player);
}

#if USE(ACCELERATED_COMPOSITING) && !(PLATFORM(QT) && USE(QTKIT))
bool MediaPlayerPrivateQTKit::supportsAcceleratedRendering() const
{
    return isReadyForVideoSetup() && getQTMovieLayerClass() != Nil;
}

void MediaPlayerPrivateQTKit::acceleratedRenderingStateChanged()
{
    // Set up or change the rendering path if necessary.
    setUpVideoRendering();
}
#endif

bool MediaPlayerPrivateQTKit::hasSingleSecurityOrigin() const
{
    if (!m_qtMovie)
        return false;

    RefPtr<SecurityOrigin> resolvedOrigin = SecurityOrigin::create(KURL(wkQTMovieResolvedURL(m_qtMovie.get())));
    RefPtr<SecurityOrigin> requestedOrigin = SecurityOrigin::createFromString(m_movieURL);
    return resolvedOrigin->isSameSchemeHostPort(requestedOrigin.get());
}

MediaPlayer::MovieLoadType MediaPlayerPrivateQTKit::movieLoadType() const
{
    if (!m_qtMovie)
        return MediaPlayer::Unknown;

    MediaPlayer::MovieLoadType movieType = (MediaPlayer::MovieLoadType)wkQTMovieGetType(m_qtMovie.get());

    // Can't include WebKitSystemInterface from WebCore so we can't get the enum returned
    // by wkQTMovieGetType, but at least verify that the value is in the valid range.
    ASSERT(movieType >= MediaPlayer::Unknown && movieType <= MediaPlayer::LiveStream);

    return movieType;
}

void MediaPlayerPrivateQTKit::setPreload(MediaPlayer::Preload preload)
{
    m_preload = preload;
    if (m_preload == MediaPlayer::None)
        return;

    if (!m_qtMovie)
        resumeLoad();
    else if (m_preload == MediaPlayer::Auto)
        [m_qtMovie.get() setAttribute:[NSNumber numberWithBool:NO] forKey:@"QTMovieLimitReadAheadAttribute"];
}

float MediaPlayerPrivateQTKit::mediaTimeForTimeValue(float timeValue) const
{
    if (!metaDataAvailable())
        return timeValue;

    QTTime qttime = createQTTime(timeValue);
    return static_cast<float>(qttime.timeValue) / qttime.timeScale;
}

void MediaPlayerPrivateQTKit::setPrivateBrowsingMode(bool privateBrowsing)
{
    m_privateBrowsing = privateBrowsing;
    if (!m_qtMovie)
        return;
    [m_qtMovie.get() setAttribute:[NSNumber numberWithBool:!privateBrowsing] forKey:@"QTMovieAllowPersistentCacheAttribute"];
}

} // namespace WebCore

@implementation WebCoreMovieObserver

- (id)initWithCallback:(MediaPlayerPrivateQTKit*)callback
{
    m_callback = callback;
    return [super init];
}

- (void)disconnect
{
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
    m_callback = 0;
}

-(NSMenu*)menuForEventDelegate:(NSEvent*)theEvent
{
    // Get the contextual menu from the QTMovieView's superview, the frame view
    return [[m_view superview] menuForEvent:theEvent];
}

-(void)setView:(NSView*)view
{
    m_view = view;
}

-(void)repaint
{
    if (m_delayCallbacks)
        [self performSelector:_cmd withObject:nil afterDelay:0.];
    else if (m_callback)
        m_callback->repaint();
}

- (void)loadStateChanged:(NSNotification *)unusedNotification
{
    UNUSED_PARAM(unusedNotification);
    if (m_delayCallbacks)
        [self performSelector:_cmd withObject:nil afterDelay:0];
    else
        m_callback->loadStateChanged();
}

- (void)rateChanged:(NSNotification *)unusedNotification
{
    UNUSED_PARAM(unusedNotification);
    if (m_delayCallbacks)
        [self performSelector:_cmd withObject:nil afterDelay:0];
    else
        m_callback->rateChanged();
}

- (void)sizeChanged:(NSNotification *)unusedNotification
{
    UNUSED_PARAM(unusedNotification);
    if (m_delayCallbacks)
        [self performSelector:_cmd withObject:nil afterDelay:0];
    else
        m_callback->sizeChanged();
}

- (void)timeChanged:(NSNotification *)unusedNotification
{
    UNUSED_PARAM(unusedNotification);
    if (m_delayCallbacks)
        [self performSelector:_cmd withObject:nil afterDelay:0];
    else
        m_callback->timeChanged();
}

- (void)didEnd:(NSNotification *)unusedNotification
{
    UNUSED_PARAM(unusedNotification);
    if (m_delayCallbacks)
        [self performSelector:_cmd withObject:nil afterDelay:0];
    else
        m_callback->didEnd();
}

- (void)newImageAvailable:(NSNotification *)unusedNotification
{
    UNUSED_PARAM(unusedNotification);
    [self repaint];
}

- (void)layerHostChanged:(NSNotification *)notification
{
#if USE(ACCELERATED_COMPOSITING) && !(PLATFORM(QT) && USE(QTKIT))
    CALayer* rootLayer = static_cast<CALayer*>([notification object]);
    m_callback->layerHostChanged(rootLayer);
#else
    UNUSED_PARAM(notification);
#endif
}

- (void)setDelayCallbacks:(BOOL)shouldDelay
{
    m_delayCallbacks = shouldDelay;
}

@end

#endif
