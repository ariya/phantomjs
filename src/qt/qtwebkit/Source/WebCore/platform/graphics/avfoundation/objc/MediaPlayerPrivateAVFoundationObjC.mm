/*
 * Copyright (C) 2011, 2012, 2013 Apple Inc. All rights reserved.
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

#if ENABLE(VIDEO) && USE(AVFOUNDATION)

#import "MediaPlayerPrivateAVFoundationObjC.h"

#import "BlockExceptions.h"
#import "DataView.h"
#import "ExceptionCodePlaceholder.h"
#import "FloatConversion.h"
#import "FloatConversion.h"
#import "FrameView.h"
#import "GraphicsContext.h"
#import "InbandTextTrackPrivateAVFObjC.h"
#import "InbandTextTrackPrivateLegacyAVFObjC.h"
#import "KURL.h"
#import "Logging.h"
#import "SecurityOrigin.h"
#import "SoftLinking.h"
#import "TimeRanges.h"
#import "UUID.h"
#import "WebCoreAVFResourceLoader.h"
#import "WebCoreSystemInterface.h"
#import <objc/runtime.h>
#import <wtf/CurrentTime.h>
#import <wtf/Uint16Array.h>
#import <wtf/Uint32Array.h>
#import <wtf/Uint8Array.h>
#import <wtf/text/CString.h>

#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
#import <CoreVideo/CoreVideo.h>
#import <VideoToolbox/VideoToolbox.h>
#endif

SOFT_LINK_FRAMEWORK_OPTIONAL(AVFoundation)
SOFT_LINK_FRAMEWORK_OPTIONAL(CoreMedia)

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
SOFT_LINK_FRAMEWORK_OPTIONAL(CoreVideo)
SOFT_LINK_FRAMEWORK_OPTIONAL(VideoToolbox)
#endif

SOFT_LINK(CoreMedia, CMTimeCompare, int32_t, (CMTime time1, CMTime time2), (time1, time2))
SOFT_LINK(CoreMedia, CMTimeMakeWithSeconds, CMTime, (Float64 seconds, int32_t preferredTimeScale), (seconds, preferredTimeScale))
SOFT_LINK(CoreMedia, CMTimeGetSeconds, Float64, (CMTime time), (time))
SOFT_LINK(CoreMedia, CMTimeRangeGetEnd, CMTime, (CMTimeRange range), (range))

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
SOFT_LINK(CoreVideo, CVPixelBufferGetWidth, size_t, (CVPixelBufferRef pixelBuffer), (pixelBuffer))
SOFT_LINK(CoreVideo, CVPixelBufferGetHeight, size_t, (CVPixelBufferRef pixelBuffer), (pixelBuffer))
SOFT_LINK(VideoToolbox, VTPixelTransferSessionCreate, OSStatus, (CFAllocatorRef allocator, VTPixelTransferSessionRef *pixelTransferSessionOut), (allocator, pixelTransferSessionOut))
SOFT_LINK(VideoToolbox, VTPixelTransferSessionTransferImage, OSStatus, (VTPixelTransferSessionRef session, CVPixelBufferRef sourceBuffer, CVPixelBufferRef destinationBuffer), (session, sourceBuffer, destinationBuffer))
#endif

SOFT_LINK_CLASS(AVFoundation, AVPlayer)
SOFT_LINK_CLASS(AVFoundation, AVPlayerItem)
SOFT_LINK_CLASS(AVFoundation, AVPlayerItemVideoOutput)
SOFT_LINK_CLASS(AVFoundation, AVPlayerLayer)
SOFT_LINK_CLASS(AVFoundation, AVURLAsset)
SOFT_LINK_CLASS(AVFoundation, AVAssetImageGenerator)

SOFT_LINK_POINTER(AVFoundation, AVMediaCharacteristicVisual, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaCharacteristicAudible, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaTypeClosedCaption, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaTypeVideo, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaTypeAudio, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVPlayerItemDidPlayToEndTimeNotification, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVAssetImageGeneratorApertureModeCleanAperture, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVURLAssetReferenceRestrictionsKey, NSString *)

SOFT_LINK_CONSTANT(CoreMedia, kCMTimeZero, CMTime)

#define AVPlayer getAVPlayerClass()
#define AVPlayerItem getAVPlayerItemClass()
#define AVPlayerItemVideoOutput getAVPlayerItemVideoOutputClass()
#define AVPlayerLayer getAVPlayerLayerClass()
#define AVURLAsset getAVURLAssetClass()
#define AVAssetImageGenerator getAVAssetImageGeneratorClass()

#define AVMediaCharacteristicVisual getAVMediaCharacteristicVisual()
#define AVMediaCharacteristicAudible getAVMediaCharacteristicAudible()
#define AVMediaTypeClosedCaption getAVMediaTypeClosedCaption()
#define AVMediaTypeVideo getAVMediaTypeVideo()
#define AVMediaTypeAudio getAVMediaTypeAudio()
#define AVPlayerItemDidPlayToEndTimeNotification getAVPlayerItemDidPlayToEndTimeNotification()
#define AVAssetImageGeneratorApertureModeCleanAperture getAVAssetImageGeneratorApertureModeCleanAperture()
#define AVURLAssetReferenceRestrictionsKey getAVURLAssetReferenceRestrictionsKey()

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
typedef AVMediaSelectionGroup AVMediaSelectionGroupType;
typedef AVMediaSelectionOption AVMediaSelectionOptionType;

SOFT_LINK_CLASS(AVFoundation, AVPlayerItemLegibleOutput)
SOFT_LINK_CLASS(AVFoundation, AVMediaSelectionGroup)
SOFT_LINK_CLASS(AVFoundation, AVMediaSelectionOption)

SOFT_LINK_POINTER(AVFoundation, AVMediaCharacteristicLegible, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaTypeSubtitle, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaCharacteristicContainsOnlyForcedSubtitles, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVPlayerItemLegibleOutputTextStylingResolutionSourceAndRulesOnly, NSString *)

#define AVPlayerItemLegibleOutput getAVPlayerItemLegibleOutputClass()
#define AVMediaSelectionGroup getAVMediaSelectionGroupClass()
#define AVMediaSelectionOption getAVMediaSelectionOptionClass()
#define AVMediaCharacteristicLegible getAVMediaCharacteristicLegible()
#define AVMediaTypeSubtitle getAVMediaTypeSubtitle()
#define AVMediaCharacteristicContainsOnlyForcedSubtitles getAVMediaCharacteristicContainsOnlyForcedSubtitles()
#define AVPlayerItemLegibleOutputTextStylingResolutionSourceAndRulesOnly getAVPlayerItemLegibleOutputTextStylingResolutionSourceAndRulesOnly()
#endif

#define kCMTimeZero getkCMTimeZero()

using namespace WebCore;
using namespace std;

enum MediaPlayerAVFoundationObservationContext {
    MediaPlayerAVFoundationObservationContextPlayerItem,
    MediaPlayerAVFoundationObservationContextPlayer
};

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP) && HAVE(AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT)
@interface WebCoreAVFMovieObserver : NSObject <AVPlayerItemLegibleOutputPushDelegate>
#else
@interface WebCoreAVFMovieObserver : NSObject
#endif
{
    MediaPlayerPrivateAVFoundationObjC* m_callback;
    int m_delayCallbacks;
}
-(id)initWithCallback:(MediaPlayerPrivateAVFoundationObjC*)callback;
-(void)disconnect;
-(void)playableKnown;
-(void)metadataLoaded;
-(void)seekCompleted:(BOOL)finished;
-(void)didEnd:(NSNotification *)notification;
-(void)observeValueForKeyPath:keyPath ofObject:(id)object change:(NSDictionary *)change context:(MediaPlayerAVFoundationObservationContext)context;
#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
- (void)legibleOutput:(id)output didOutputAttributedStrings:(NSArray *)strings nativeSampleBuffers:(NSArray *)nativeSamples forItemTime:(CMTime)itemTime;
- (void)outputSequenceWasFlushed:(id)output;
#endif
@end

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
@interface WebCoreAVFLoaderDelegate : NSObject<AVAssetResourceLoaderDelegate> {
    MediaPlayerPrivateAVFoundationObjC* m_callback;
}
- (id)initWithCallback:(MediaPlayerPrivateAVFoundationObjC*)callback;
- (BOOL)resourceLoader:(AVAssetResourceLoader *)resourceLoader shouldWaitForLoadingOfRequestedResource:(AVAssetResourceLoadingRequest *)loadingRequest;
- (void)setCallback:(MediaPlayerPrivateAVFoundationObjC*)callback;
@end
#endif

namespace WebCore {

static NSArray *assetMetadataKeyNames();
static NSArray *itemKVOProperties();

#if !LOG_DISABLED
static const char *boolString(bool val)
{
    return val ? "true" : "false";
}
#endif

#if ENABLE(ENCRYPTED_MEDIA_V2)
typedef HashMap<MediaPlayer*, MediaPlayerPrivateAVFoundationObjC*> PlayerToPrivateMapType;
static PlayerToPrivateMapType& playerToPrivateMap()
{
    DEFINE_STATIC_LOCAL(PlayerToPrivateMapType, map, ());
    return map;
};
#endif

#if ENABLE(ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA_V2)
static dispatch_queue_t globalLoaderDelegateQueue()
{
    static dispatch_queue_t globalQueue;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        globalQueue = dispatch_queue_create("WebCoreAVFLoaderDelegate queue", DISPATCH_QUEUE_SERIAL);
    });
    return globalQueue;
}
#endif

PassOwnPtr<MediaPlayerPrivateInterface> MediaPlayerPrivateAVFoundationObjC::create(MediaPlayer* player)
{ 
    return adoptPtr(new MediaPlayerPrivateAVFoundationObjC(player));
}

void MediaPlayerPrivateAVFoundationObjC::registerMediaEngine(MediaEngineRegistrar registrar)
{
    if (isAvailable())
#if ENABLE(ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA_V2)
        registrar(create, getSupportedTypes, extendedSupportsType, 0, 0, 0);
#else
        registrar(create, getSupportedTypes, supportsType, 0, 0, 0);
#endif
}

MediaPlayerPrivateAVFoundationObjC::MediaPlayerPrivateAVFoundationObjC(MediaPlayer* player)
    : MediaPlayerPrivateAVFoundation(player)
    , m_objcObserver(adoptNS([[WebCoreAVFMovieObserver alloc] initWithCallback:this]))
    , m_videoFrameHasDrawn(false)
    , m_haveCheckedPlayability(false)
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    , m_loaderDelegate(adoptNS([[WebCoreAVFLoaderDelegate alloc] initWithCallback:this]))
#endif
    , m_currentTrack(0)
{
#if ENABLE(ENCRYPTED_MEDIA_V2)
    playerToPrivateMap().set(player, this);
#endif
}

MediaPlayerPrivateAVFoundationObjC::~MediaPlayerPrivateAVFoundationObjC()
{
#if ENABLE(ENCRYPTED_MEDIA_V2)
    playerToPrivateMap().remove(player());
#endif
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    [m_loaderDelegate.get() setCallback:0];
    [[m_avAsset.get() resourceLoader] setDelegate:nil queue:0];
#endif
    cancelLoad();
}

void MediaPlayerPrivateAVFoundationObjC::cancelLoad()
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::cancelLoad(%p)", this);
    tearDownVideoRendering();

    [[NSNotificationCenter defaultCenter] removeObserver:m_objcObserver.get()];
    [m_objcObserver.get() disconnect];

    // Tell our observer to do nothing when our cancellation of pending loading calls its completion handler.
    setIgnoreLoadStateChanges(true);
    if (m_avAsset) {
        [m_avAsset.get() cancelLoading];
        m_avAsset = nil;
    }

    clearTextTracks();

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP) && HAVE(AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT)
    if (m_legibleOutput) {
        if (m_avPlayerItem)
            [m_avPlayerItem.get() removeOutput:m_legibleOutput.get()];
        m_legibleOutput = nil;
    }
#endif

    if (m_avPlayerItem) {
        for (NSString *keyName in itemKVOProperties())
            [m_avPlayerItem.get() removeObserver:m_objcObserver.get() forKeyPath:keyName];
        
        m_avPlayerItem = nil;
    }
    if (m_avPlayer) {
        if (m_timeObserver)
            [m_avPlayer.get() removeTimeObserver:m_timeObserver.get()];
        m_timeObserver = nil;
        [m_avPlayer.get() removeObserver:m_objcObserver.get() forKeyPath:@"rate"];
        m_avPlayer = nil;
    }
    setIgnoreLoadStateChanges(false);
}

bool MediaPlayerPrivateAVFoundationObjC::hasLayerRenderer() const
{
    return m_videoLayer;
}

bool MediaPlayerPrivateAVFoundationObjC::hasContextRenderer() const
{
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
    return m_videoOutput;
#else
    return m_imageGenerator;
#endif
}

void MediaPlayerPrivateAVFoundationObjC::createContextVideoRenderer()
{
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
    createVideoOutput();
#else
    createImageGenerator();
#endif
}

#if __MAC_OS_X_VERSION_MIN_REQUIRED < 1080
void MediaPlayerPrivateAVFoundationObjC::createImageGenerator()
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createImageGenerator(%p)", this);

    if (!m_avAsset || m_imageGenerator)
        return;

    m_imageGenerator = [AVAssetImageGenerator assetImageGeneratorWithAsset:m_avAsset.get()];

    [m_imageGenerator.get() setApertureMode:AVAssetImageGeneratorApertureModeCleanAperture];
    [m_imageGenerator.get() setAppliesPreferredTrackTransform:YES];
    [m_imageGenerator.get() setRequestedTimeToleranceBefore:kCMTimeZero];
    [m_imageGenerator.get() setRequestedTimeToleranceAfter:kCMTimeZero];

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createImageGenerator(%p) - returning %p", this, m_imageGenerator.get());
}
#endif

void MediaPlayerPrivateAVFoundationObjC::destroyContextVideoRenderer()
{
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
    destroyVideoOutput();
#else
    destroyImageGenerator();
#endif
}

#if __MAC_OS_X_VERSION_MIN_REQUIRED < 1080
void MediaPlayerPrivateAVFoundationObjC::destroyImageGenerator()
{
    if (!m_imageGenerator)
        return;

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::destroyImageGenerator(%p) - destroying  %p", this, m_imageGenerator.get());

    m_imageGenerator = 0;
}
#endif

void MediaPlayerPrivateAVFoundationObjC::createVideoLayer()
{
    if (!m_avPlayer)
        return;

    if (!m_videoLayer) {
        m_videoLayer = adoptNS([[AVPlayerLayer alloc] init]);
        [m_videoLayer.get() setPlayer:m_avPlayer.get()];
        [m_videoLayer.get() setBackgroundColor:CGColorGetConstantColor(kCGColorBlack)];
#ifndef NDEBUG
        [m_videoLayer.get() setName:@"Video layer"];
#endif
        LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createVideoLayer(%p) - returning %p", this, m_videoLayer.get());
    }
}

void MediaPlayerPrivateAVFoundationObjC::destroyVideoLayer()
{
    if (!m_videoLayer)
        return;

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::destroyVideoLayer(%p) - destroying", this, m_videoLayer.get());

    [m_videoLayer.get() setPlayer:nil];

    m_videoLayer = 0;
}

bool MediaPlayerPrivateAVFoundationObjC::hasAvailableVideoFrame() const
{
    return (m_videoFrameHasDrawn || (m_videoLayer && [m_videoLayer.get() isReadyForDisplay]));
}

void MediaPlayerPrivateAVFoundationObjC::createAVAssetForURL(const String& url)
{
    if (m_avAsset)
        return;

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createAVAssetForURL(%p)", this);

    setDelayCallbacks(true);

    RetainPtr<NSMutableDictionary> options = adoptNS([[NSMutableDictionary alloc] init]);    

    [options.get() setObject:[NSNumber numberWithInt:AVAssetReferenceRestrictionForbidRemoteReferenceToLocal | AVAssetReferenceRestrictionForbidLocalReferenceToRemote] forKey:AVURLAssetReferenceRestrictionsKey];

#if PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
    RetainPtr<NSMutableDictionary> headerFields = adoptNS([[NSMutableDictionary alloc] init]);

    String referrer = player()->referrer();
    if (!referrer.isEmpty())
        [headerFields.get() setObject:referrer forKey:@"Referer"];

    String userAgent = player()->userAgent();
    if (!userAgent.isEmpty())
        [headerFields.get() setObject:userAgent forKey:@"User-Agent"];

    if ([headerFields.get() count])
        [options.get() setObject:headerFields.get() forKey:@"AVURLAssetHTTPHeaderFieldsKey"];
#endif

    NSURL *cocoaURL = KURL(ParsedURLString, url);
    m_avAsset = adoptNS([[AVURLAsset alloc] initWithURL:cocoaURL options:options.get()]);

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    [[m_avAsset.get() resourceLoader] setDelegate:m_loaderDelegate.get() queue:globalLoaderDelegateQueue()];
#endif

    m_haveCheckedPlayability = false;

    setDelayCallbacks(false);
}

void MediaPlayerPrivateAVFoundationObjC::createAVPlayer()
{
    if (m_avPlayer)
        return;

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createAVPlayer(%p)", this);

    setDelayCallbacks(true);

    m_avPlayer = adoptNS([[AVPlayer alloc] init]);
    [m_avPlayer.get() addObserver:m_objcObserver.get() forKeyPath:@"rate" options:nil context:(void *)MediaPlayerAVFoundationObservationContextPlayer];

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP) && HAVE(AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT)
    [m_avPlayer.get() setAppliesMediaSelectionCriteriaAutomatically:YES];
#endif

    if (m_avPlayerItem)
        [m_avPlayer.get() replaceCurrentItemWithPlayerItem:m_avPlayerItem.get()];

    setDelayCallbacks(false);
}

void MediaPlayerPrivateAVFoundationObjC::createAVPlayerItem()
{
    if (m_avPlayerItem)
        return;

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createAVPlayerItem(%p)", this);

    setDelayCallbacks(true);

    // Create the player item so we can load media data. 
    m_avPlayerItem = adoptNS([[AVPlayerItem alloc] initWithAsset:m_avAsset.get()]);

    [[NSNotificationCenter defaultCenter] addObserver:m_objcObserver.get() selector:@selector(didEnd:) name:AVPlayerItemDidPlayToEndTimeNotification object:m_avPlayerItem.get()];

    for (NSString *keyName in itemKVOProperties())
        [m_avPlayerItem.get() addObserver:m_objcObserver.get() forKeyPath:keyName options:nil context:(void *)MediaPlayerAVFoundationObservationContextPlayerItem];

    if (m_avPlayer)
        [m_avPlayer.get() replaceCurrentItemWithPlayerItem:m_avPlayerItem.get()];

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP) && HAVE(AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT)
    const NSTimeInterval legibleOutputAdvanceInterval = 2;

    m_legibleOutput = adoptNS([[AVPlayerItemLegibleOutput alloc] initWithMediaSubtypesForNativeRepresentation:[NSArray array]]);
    [m_legibleOutput.get() setSuppressesPlayerRendering:YES];

    [m_legibleOutput.get() setDelegate:m_objcObserver.get() queue:dispatch_get_main_queue()];
    [m_legibleOutput.get() setAdvanceIntervalForDelegateInvocation:legibleOutputAdvanceInterval];
    [m_legibleOutput.get() setTextStylingResolution:AVPlayerItemLegibleOutputTextStylingResolutionSourceAndRulesOnly];
    [m_avPlayerItem.get() addOutput:m_legibleOutput.get()];
#endif

    setDelayCallbacks(false);
}

void MediaPlayerPrivateAVFoundationObjC::checkPlayability()
{
    if (m_haveCheckedPlayability)
        return;
    m_haveCheckedPlayability = true;

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::checkPlayability(%p)", this);

    [m_avAsset.get() loadValuesAsynchronouslyForKeys:[NSArray arrayWithObject:@"playable"] completionHandler:^{
        [m_objcObserver.get() playableKnown];
    }];
}

void MediaPlayerPrivateAVFoundationObjC::beginLoadingMetadata()
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::beginLoadingMetadata(%p) - requesting metadata loading", this);
    [m_avAsset.get() loadValuesAsynchronouslyForKeys:[assetMetadataKeyNames() retain] completionHandler:^{
        [m_objcObserver.get() metadataLoaded];
    }];
}

MediaPlayerPrivateAVFoundation::ItemStatus MediaPlayerPrivateAVFoundationObjC::playerItemStatus() const
{
    if (!m_avPlayerItem)
        return MediaPlayerPrivateAVFoundation::MediaPlayerAVPlayerItemStatusDoesNotExist;

    AVPlayerItemStatus status = [m_avPlayerItem.get() status];
    if (status == AVPlayerItemStatusUnknown)
        return MediaPlayerPrivateAVFoundation::MediaPlayerAVPlayerItemStatusUnknown;
    if (status == AVPlayerItemStatusFailed)
        return MediaPlayerPrivateAVFoundation::MediaPlayerAVPlayerItemStatusFailed;
    if ([m_avPlayerItem.get() isPlaybackLikelyToKeepUp])
        return MediaPlayerPrivateAVFoundation::MediaPlayerAVPlayerItemStatusPlaybackLikelyToKeepUp;
    if ([m_avPlayerItem.get() isPlaybackBufferFull])
        return MediaPlayerPrivateAVFoundation::MediaPlayerAVPlayerItemStatusPlaybackBufferFull;
    if ([m_avPlayerItem.get() isPlaybackBufferEmpty])
        return MediaPlayerPrivateAVFoundation::MediaPlayerAVPlayerItemStatusPlaybackBufferEmpty;

    return MediaPlayerPrivateAVFoundation::MediaPlayerAVPlayerItemStatusReadyToPlay;
}

PlatformMedia MediaPlayerPrivateAVFoundationObjC::platformMedia() const
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::platformMedia(%p)", this);
    PlatformMedia pm;
    pm.type = PlatformMedia::AVFoundationMediaPlayerType;
    pm.media.avfMediaPlayer = m_avPlayer.get();
    return pm;
}

PlatformLayer* MediaPlayerPrivateAVFoundationObjC::platformLayer() const
{
    return m_videoLayer.get();
}

void MediaPlayerPrivateAVFoundationObjC::platformSetVisible(bool isVisible)
{
    [CATransaction begin];
    [CATransaction setDisableActions:YES];    
    if (m_videoLayer)
        [m_videoLayer.get() setHidden:!isVisible];
    [CATransaction commit];
}
    
void MediaPlayerPrivateAVFoundationObjC::platformPlay()
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::platformPlay(%p)", this);
    if (!metaDataAvailable())
        return;

    setDelayCallbacks(true);
    [m_avPlayer.get() setRate:requestedRate()];
    setDelayCallbacks(false);
}

void MediaPlayerPrivateAVFoundationObjC::platformPause()
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::platformPause(%p)", this);
    if (!metaDataAvailable())
        return;

    setDelayCallbacks(true);
    [m_avPlayer.get() setRate:nil];
    setDelayCallbacks(false);
}

float MediaPlayerPrivateAVFoundationObjC::platformDuration() const
{
    // Do not ask the asset for duration before it has been loaded or it will fetch the
    // answer synchronously.
    if (!m_avAsset || assetStatus() < MediaPlayerAVAssetStatusLoaded)
         return MediaPlayer::invalidTime();
    
    CMTime cmDuration;
    
    // Check the AVItem if we have one and it has loaded duration, some assets never report duration.
    if (m_avPlayerItem && playerItemStatus() >= MediaPlayerAVPlayerItemStatusReadyToPlay)
        cmDuration = [m_avPlayerItem.get() duration];
    else
        cmDuration= [m_avAsset.get() duration];

    if (CMTIME_IS_NUMERIC(cmDuration))
        return narrowPrecisionToFloat(CMTimeGetSeconds(cmDuration));

    if (CMTIME_IS_INDEFINITE(cmDuration)) {
        return numeric_limits<float>::infinity();
    }

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::platformDuration(%p) - invalid duration, returning %.0f", this, MediaPlayer::invalidTime());
    return MediaPlayer::invalidTime();
}

float MediaPlayerPrivateAVFoundationObjC::currentTime() const
{
    if (!metaDataAvailable() || !m_avPlayerItem)
        return 0;

    CMTime itemTime = [m_avPlayerItem.get() currentTime];
    if (CMTIME_IS_NUMERIC(itemTime)) {
        return max(narrowPrecisionToFloat(CMTimeGetSeconds(itemTime)), 0.0f);
    }

    return 0;
}

void MediaPlayerPrivateAVFoundationObjC::seekToTime(double time)
{
    // setCurrentTime generates several event callbacks, update afterwards.
    setDelayCallbacks(true);

    WebCoreAVFMovieObserver *observer = m_objcObserver.get();
    [m_avPlayerItem.get() seekToTime:CMTimeMakeWithSeconds(time, 600) toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero completionHandler:^(BOOL finished) {
        [observer seekCompleted:finished];
    }];

    setDelayCallbacks(false);
}

void MediaPlayerPrivateAVFoundationObjC::setVolume(float volume)
{
    if (!metaDataAvailable())
        return;

    [m_avPlayer.get() setVolume:volume];
}

void MediaPlayerPrivateAVFoundationObjC::setClosedCaptionsVisible(bool closedCaptionsVisible)
{
    UNUSED_PARAM(closedCaptionsVisible);

    if (!metaDataAvailable())
        return;

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::setClosedCaptionsVisible(%p) - set to %s", this, boolString(closedCaptionsVisible));
}

void MediaPlayerPrivateAVFoundationObjC::updateRate()
{
    setDelayCallbacks(true);
    [m_avPlayer.get() setRate:requestedRate()];
    setDelayCallbacks(false);
}

float MediaPlayerPrivateAVFoundationObjC::rate() const
{
    if (!metaDataAvailable())
        return 0;

    return [m_avPlayer.get() rate];
}

PassRefPtr<TimeRanges> MediaPlayerPrivateAVFoundationObjC::platformBufferedTimeRanges() const
{
    RefPtr<TimeRanges> timeRanges = TimeRanges::create();

    if (!m_avPlayerItem)
        return timeRanges.release();

    NSArray *loadedRanges = [m_avPlayerItem.get() loadedTimeRanges];
    for (NSValue *thisRangeValue in loadedRanges) {
        CMTimeRange timeRange = [thisRangeValue CMTimeRangeValue];
        if (CMTIMERANGE_IS_VALID(timeRange) && !CMTIMERANGE_IS_EMPTY(timeRange)) {
            float rangeStart = narrowPrecisionToFloat(CMTimeGetSeconds(timeRange.start));
            float rangeEnd = narrowPrecisionToFloat(CMTimeGetSeconds(CMTimeRangeGetEnd(timeRange)));
            timeRanges->add(rangeStart, rangeEnd);
        }
    }
    return timeRanges.release();
}

double MediaPlayerPrivateAVFoundationObjC::platformMinTimeSeekable() const
{
    NSArray *seekableRanges = [m_avPlayerItem.get() seekableTimeRanges];
    if (!seekableRanges || ![seekableRanges count])
        return 0;

    double minTimeSeekable = std::numeric_limits<double>::infinity();
    bool hasValidRange = false;
    for (NSValue *thisRangeValue in seekableRanges) {
        CMTimeRange timeRange = [thisRangeValue CMTimeRangeValue];
        if (!CMTIMERANGE_IS_VALID(timeRange) || CMTIMERANGE_IS_EMPTY(timeRange))
            continue;

        hasValidRange = true;
        double startOfRange = CMTimeGetSeconds(timeRange.start);
        if (minTimeSeekable > startOfRange)
            minTimeSeekable = startOfRange;
    }
    return hasValidRange ? minTimeSeekable : 0;
}

double MediaPlayerPrivateAVFoundationObjC::platformMaxTimeSeekable() const
{
    NSArray *seekableRanges = [m_avPlayerItem.get() seekableTimeRanges];
    if (!seekableRanges)
        return 0;

    double maxTimeSeekable = 0;
    for (NSValue *thisRangeValue in seekableRanges) {
        CMTimeRange timeRange = [thisRangeValue CMTimeRangeValue];
        if (!CMTIMERANGE_IS_VALID(timeRange) || CMTIMERANGE_IS_EMPTY(timeRange))
            continue;
        
        double endOfRange = CMTimeGetSeconds(CMTimeRangeGetEnd(timeRange));
        if (maxTimeSeekable < endOfRange)
            maxTimeSeekable = endOfRange;
    }
    return maxTimeSeekable;
}

float MediaPlayerPrivateAVFoundationObjC::platformMaxTimeLoaded() const
{
    NSArray *loadedRanges = [m_avPlayerItem.get() loadedTimeRanges];
    if (!loadedRanges)
        return 0;

    float maxTimeLoaded = 0;
    for (NSValue *thisRangeValue in loadedRanges) {
        CMTimeRange timeRange = [thisRangeValue CMTimeRangeValue];
        if (!CMTIMERANGE_IS_VALID(timeRange) || CMTIMERANGE_IS_EMPTY(timeRange))
            continue;
        
        float endOfRange = narrowPrecisionToFloat(CMTimeGetSeconds(CMTimeRangeGetEnd(timeRange)));
        if (maxTimeLoaded < endOfRange)
            maxTimeLoaded = endOfRange;
    }

    return maxTimeLoaded;   
}

unsigned MediaPlayerPrivateAVFoundationObjC::totalBytes() const
{
    if (!metaDataAvailable())
        return 0;

    long long totalMediaSize = 0;
    NSArray *tracks = [m_avAsset.get() tracks];
    for (AVAssetTrack *thisTrack in tracks)
        totalMediaSize += [thisTrack totalSampleDataLength];

    return static_cast<unsigned>(totalMediaSize);
}

void MediaPlayerPrivateAVFoundationObjC::setAsset(id asset)
{
    m_avAsset = asset;
}

MediaPlayerPrivateAVFoundation::AssetStatus MediaPlayerPrivateAVFoundationObjC::assetStatus() const
{
    if (!m_avAsset)
        return MediaPlayerAVAssetStatusDoesNotExist;

    for (NSString *keyName in assetMetadataKeyNames()) {
        AVKeyValueStatus keyStatus = [m_avAsset.get() statusOfValueForKey:keyName error:nil];

        if (keyStatus < AVKeyValueStatusLoaded)
            return MediaPlayerAVAssetStatusLoading;// At least one key is not loaded yet.
        
        if (keyStatus == AVKeyValueStatusFailed)
            return MediaPlayerAVAssetStatusFailed; // At least one key could not be loaded.

        if (keyStatus == AVKeyValueStatusCancelled)
            return MediaPlayerAVAssetStatusCancelled; // Loading of at least one key was cancelled.
    }

    if ([[m_avAsset.get() valueForKey:@"playable"] boolValue])
        return MediaPlayerAVAssetStatusPlayable;

    return MediaPlayerAVAssetStatusLoaded;
}

void MediaPlayerPrivateAVFoundationObjC::paintCurrentFrameInContext(GraphicsContext* context, const IntRect& rect)
{
    if (!metaDataAvailable() || context->paintingDisabled())
        return;

    paint(context, rect);
}

void MediaPlayerPrivateAVFoundationObjC::paint(GraphicsContext* context, const IntRect& rect)
{
    if (!metaDataAvailable() || context->paintingDisabled())
        return;

    setDelayCallbacks(true);
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
    paintWithVideoOutput(context, rect);
#else
    paintWithImageGenerator(context, rect);
#endif

    END_BLOCK_OBJC_EXCEPTIONS;
    setDelayCallbacks(false);

    m_videoFrameHasDrawn = true;
}

#if __MAC_OS_X_VERSION_MIN_REQUIRED < 1080
void MediaPlayerPrivateAVFoundationObjC::paintWithImageGenerator(GraphicsContext* context, const IntRect& rect)
{
    RetainPtr<CGImageRef> image = createImageForTimeInRect(currentTime(), rect);
    if (image) {
        GraphicsContextStateSaver stateSaver(*context);
        context->translate(rect.x(), rect.y() + rect.height());
        context->scale(FloatSize(1.0f, -1.0f));
        context->setImageInterpolationQuality(InterpolationLow);
        IntRect paintRect(IntPoint(0, 0), IntSize(rect.width(), rect.height()));
        CGContextDrawImage(context->platformContext(), CGRectMake(0, 0, paintRect.width(), paintRect.height()), image.get());
        image = 0;
    }
}
#endif

static HashSet<String> mimeTypeCache()
{
    DEFINE_STATIC_LOCAL(HashSet<String>, cache, ());
    static bool typeListInitialized = false;

    if (typeListInitialized)
        return cache;
    typeListInitialized = true;

    NSArray *types = [AVURLAsset audiovisualMIMETypes];
    for (NSString *mimeType in types)
        cache.add(mimeType);

    return cache;
} 

#if __MAC_OS_X_VERSION_MIN_REQUIRED < 1080
RetainPtr<CGImageRef> MediaPlayerPrivateAVFoundationObjC::createImageForTimeInRect(float time, const IntRect& rect)
{
    if (!m_imageGenerator)
        createImageGenerator();
    ASSERT(m_imageGenerator);

#if !LOG_DISABLED
    double start = WTF::currentTime();
#endif

    [m_imageGenerator.get() setMaximumSize:CGSize(rect.size())];
    RetainPtr<CGImageRef> image = adoptCF([m_imageGenerator.get() copyCGImageAtTime:CMTimeMakeWithSeconds(time, 600) actualTime:nil error:nil]);

#if !LOG_DISABLED
    double duration = WTF::currentTime() - start;
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createImageForTimeInRect(%p) - creating image took %.4f", this, narrowPrecisionToFloat(duration));
#endif

    return image;
}
#endif

void MediaPlayerPrivateAVFoundationObjC::getSupportedTypes(HashSet<String>& supportedTypes)
{
    supportedTypes = mimeTypeCache();
} 

MediaPlayer::SupportsType MediaPlayerPrivateAVFoundationObjC::supportsType(const String& type, const String& codecs, const KURL&)
{
    if (!mimeTypeCache().contains(type))
        return MediaPlayer::IsNotSupported;

    // The spec says:
    // "Implementors are encouraged to return "maybe" unless the type can be confidently established as being supported or not."
    if (codecs.isEmpty())
        return MediaPlayer::MayBeSupported;

    NSString *typeString = [NSString stringWithFormat:@"%@; codecs=\"%@\"", (NSString *)type, (NSString *)codecs];
    return [AVURLAsset isPlayableExtendedMIMEType:typeString] ? MediaPlayer::IsSupported : MediaPlayer::MayBeSupported;;
}

#if ENABLE(ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA_V2)
static bool keySystemIsSupported(const String& keySystem)
{
    if (equalIgnoringCase(keySystem, "com.apple.lskd") || equalIgnoringCase(keySystem, "com.apple.lskd.1_0"))
        return true;

    return false;
}

MediaPlayer::SupportsType MediaPlayerPrivateAVFoundationObjC::extendedSupportsType(const String& type, const String& codecs, const String& keySystem, const KURL& url)
{
    // From: <http://dvcs.w3.org/hg/html-media/raw-file/eme-v0.1b/encrypted-media/encrypted-media.html#dom-canplaytype>
    // In addition to the steps in the current specification, this method must run the following steps:

    // 1. Check whether the Key System is supported with the specified container and codec type(s) by following the steps for the first matching condition from the following list:
    //    If keySystem is null, continue to the next step.
    if (keySystem.isNull() || keySystem.isEmpty())
        return supportsType(type, codecs, url);

    // If keySystem contains an unrecognized or unsupported Key System, return the empty string
    if (!keySystemIsSupported(keySystem))
        return MediaPlayer::IsNotSupported;

    // If the Key System specified by keySystem does not support decrypting the container and/or codec specified in the rest of the type string.
    // (AVFoundation does not provide an API which would allow us to determine this, so this is a no-op)

    // 2. Return "maybe" or "probably" as appropriate per the existing specification of canPlayType().
    return supportsType(type, codecs, url);
}

bool MediaPlayerPrivateAVFoundationObjC::shouldWaitForLoadingOfResource(AVAssetResourceLoadingRequest* avRequest)
{
    String scheme = [[[avRequest request] URL] scheme];
    String keyURI = [[[avRequest request] URL] absoluteString];

#if ENABLE(ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA_V2)
    if (scheme == "skd") {
        // Create an initData with the following layout:
        // [4 bytes: keyURI size], [keyURI size bytes: keyURI]
        unsigned keyURISize = keyURI.length() * sizeof(UChar);
        RefPtr<ArrayBuffer> initDataBuffer = ArrayBuffer::create(4 + keyURISize, 1);
        RefPtr<DataView> initDataView = DataView::create(initDataBuffer, 0, initDataBuffer->byteLength());
        initDataView->setUint32(0, keyURISize, true, IGNORE_EXCEPTION);

        RefPtr<Uint16Array> keyURIArray = Uint16Array::create(initDataBuffer, 4, keyURI.length());
        keyURIArray->setRange(keyURI.characters(), keyURI.length() / sizeof(unsigned char), 0);

#if ENABLE(ENCRYPTED_MEDIA)
        if (!player()->keyNeeded("com.apple.lskd", emptyString(), static_cast<const unsigned char*>(initDataBuffer->data()), initDataBuffer->byteLength()))
#elif ENABLE(ENCRYPTED_MEDIA_V2)
        RefPtr<Uint8Array> initData = Uint8Array::create(initDataBuffer, 0, initDataBuffer->byteLength());
        if (!player()->keyNeeded(initData.get()))
#endif
            return false;

        m_keyURIToRequestMap.set(keyURI, avRequest);
        return true;
    }
#endif

    m_resourceLoader = WebCoreAVFResourceLoader::create(this, avRequest);
    m_resourceLoader->startLoading();
    return true;
}

void MediaPlayerPrivateAVFoundationObjC::didCancelLoadingRequest(AVAssetResourceLoadingRequest* avRequest)
{
    String scheme = [[[avRequest request] URL] scheme];

    if (m_resourceLoader)
        m_resourceLoader->stopLoading();
}
#endif

bool MediaPlayerPrivateAVFoundationObjC::isAvailable()
{
    return AVFoundationLibrary() && CoreMediaLibrary();
}

float MediaPlayerPrivateAVFoundationObjC::mediaTimeForTimeValue(float timeValue) const
{
    if (!metaDataAvailable())
        return timeValue;

    // FIXME - impossible to implement until rdar://8721510 is fixed.
    return timeValue;
}

void MediaPlayerPrivateAVFoundationObjC::tracksChanged()
{
    String primaryAudioTrackLanguage = m_languageOfPrimaryAudioTrack;
    m_languageOfPrimaryAudioTrack = String();

    if (!m_avAsset)
        return;

    bool haveCCTrack = false;
    bool hasCaptions = false;

    // This is called whenever the tracks collection changes so cache hasVideo and hasAudio since we are
    // asked about those fairly fequently.
    if (!m_avPlayerItem) {
        // We don't have a player item yet, so check with the asset because some assets support inspection
        // prior to becoming ready to play.
        setHasVideo([[m_avAsset.get() tracksWithMediaCharacteristic:AVMediaCharacteristicVisual] count]);
        setHasAudio([[m_avAsset.get() tracksWithMediaCharacteristic:AVMediaCharacteristicAudible] count]);
#if !HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
        hasCaptions = [[m_avAsset.get() tracksWithMediaType:AVMediaTypeClosedCaption] count];
#endif
    } else {
        bool hasVideo = false;
        bool hasAudio = false;
        NSArray *tracks = [m_avPlayerItem.get() tracks];
        for (AVPlayerItemTrack *track in tracks) {
            if ([track isEnabled]) {
                AVAssetTrack *assetTrack = [track assetTrack];
                if ([[assetTrack mediaType] isEqualToString:AVMediaTypeVideo])
                    hasVideo = true;
                else if ([[assetTrack mediaType] isEqualToString:AVMediaTypeAudio])
                    hasAudio = true;
                else if ([[assetTrack mediaType] isEqualToString:AVMediaTypeClosedCaption]) {
#if !HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
                    hasCaptions = true;
#endif
                    haveCCTrack = true;
                }
            }
        }
        setHasVideo(hasVideo);
        setHasAudio(hasAudio);
    }

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
    if (AVMediaSelectionGroupType *legibleGroup = safeMediaSelectionGroupForLegibleMedia()) {
        hasCaptions = [[AVMediaSelectionGroup playableMediaSelectionOptionsFromArray:[legibleGroup options]] count];
        if (hasCaptions)
            processMediaSelectionOptions();
    }
#endif

#if !HAVE(AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT) && HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
    if (!hasCaptions && haveCCTrack)
        processLegacyClosedCaptionsTracks();
#elif !HAVE(AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT)
    if (haveCCTrack)
        processLegacyClosedCaptionsTracks();
#endif

    setHasClosedCaptions(hasCaptions);

    LOG(Media, "WebCoreAVFMovieObserver:tracksChanged(%p) - hasVideo = %s, hasAudio = %s, hasCaptions = %s",
        this, boolString(hasVideo()), boolString(hasAudio()), boolString(hasClosedCaptions()));

    sizeChanged();

    if (!primaryAudioTrackLanguage.isNull() && primaryAudioTrackLanguage != languageOfPrimaryAudioTrack())
        player()->characteristicChanged();
}

void MediaPlayerPrivateAVFoundationObjC::sizeChanged()
{
    if (!m_avAsset)
        return;

    NSArray *tracks = [m_avAsset.get() tracks];

    // Some assets don't report track properties until they are completely ready to play, but we
    // want to report a size as early as possible so use presentationSize when an asset has no tracks.
    if (m_avPlayerItem && ![tracks count]) {
        setNaturalSize(IntSize([m_avPlayerItem.get() presentationSize]));
        return;
    }

    // AVAsset's 'naturalSize' property only considers the movie's first video track, so we need to compute
    // the union of all visual track rects.
    CGRect trackUnionRect = CGRectZero;
    for (AVAssetTrack *track in tracks) {
        CGSize trackSize = [track naturalSize];
        CGRect trackRect = CGRectMake(0, 0, trackSize.width, trackSize.height);
        trackUnionRect = CGRectUnion(trackUnionRect, CGRectApplyAffineTransform(trackRect, [track preferredTransform]));
    }

    // The movie is always displayed at 0,0 so move the track rect to the origin before using width and height.
    trackUnionRect = CGRectOffset(trackUnionRect, trackUnionRect.origin.x, trackUnionRect.origin.y);
    
    // Also look at the asset's preferred transform so we account for a movie matrix.
    CGSize naturalSize = CGSizeApplyAffineTransform(trackUnionRect.size, [m_avAsset.get() preferredTransform]);

    // Cache the natural size (setNaturalSize will notify the player if it has changed).
    setNaturalSize(IntSize(naturalSize));
}

bool MediaPlayerPrivateAVFoundationObjC::hasSingleSecurityOrigin() const 
{
    if (!m_avAsset)
        return false;
    
    RefPtr<SecurityOrigin> resolvedOrigin = SecurityOrigin::create(KURL(wkAVAssetResolvedURL(m_avAsset.get())));
    RefPtr<SecurityOrigin> requestedOrigin = SecurityOrigin::createFromString(assetURL());
    return resolvedOrigin->isSameSchemeHostPort(requestedOrigin.get());
}

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
void MediaPlayerPrivateAVFoundationObjC::createVideoOutput()
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createVideoOutput(%p)", this);

    if (!m_avPlayerItem || m_videoOutput)
        return;

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    NSDictionary* attributes = @{ (NSString*)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_422YpCbCr8) };
#else
    NSDictionary* attributes = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithUnsignedInt:k32BGRAPixelFormat], kCVPixelBufferPixelFormatTypeKey,
                                nil];
#endif
    m_videoOutput = adoptNS([[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes:attributes]);
    ASSERT(m_videoOutput);

    [m_avPlayerItem.get() addOutput:m_videoOutput.get()];

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createVideoOutput(%p) - returning %p", this, m_videoOutput.get());
}

void MediaPlayerPrivateAVFoundationObjC::destroyVideoOutput()
{
    if (!m_videoOutput)
        return;

    if (m_avPlayerItem)
        [m_avPlayerItem.get() removeOutput:m_videoOutput.get()];
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::destroyVideoOutput(%p) - destroying  %p", this, m_videoOutput.get());

    m_videoOutput = 0;
}

RetainPtr<CVPixelBufferRef> MediaPlayerPrivateAVFoundationObjC::createPixelBuffer()
{
    if (!m_videoOutput)
        createVideoOutput();
    ASSERT(m_videoOutput);

#if !LOG_DISABLED
    double start = WTF::currentTime();
#endif

    CMTime currentTime = [m_avPlayerItem.get() currentTime];

    if (![m_videoOutput.get() hasNewPixelBufferForItemTime:currentTime])
        return 0;

    RetainPtr<CVPixelBufferRef> buffer = adoptCF([m_videoOutput.get() copyPixelBufferForItemTime:currentTime itemTimeForDisplay:nil]);
    if (!buffer)
        return 0;

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    // Create a VTPixelTransferSession, if necessary, as we cannot guarantee timely delivery of ARGB pixels.
    if (!m_pixelTransferSession) {
        VTPixelTransferSessionRef session = 0;
        VTPixelTransferSessionCreate(kCFAllocatorDefault, &session);
        m_pixelTransferSession = adoptCF(session);
    }

    CVPixelBufferRef outputBuffer;
    CVPixelBufferCreate(kCFAllocatorDefault, CVPixelBufferGetWidth(buffer.get()), CVPixelBufferGetHeight(buffer.get()), k32BGRAPixelFormat, 0, &outputBuffer);
    VTPixelTransferSessionTransferImage(m_pixelTransferSession.get(), buffer.get(), outputBuffer);
    buffer = adoptCF(outputBuffer);
#endif

#if !LOG_DISABLED
    double duration = WTF::currentTime() - start;
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createPixelBuffer() - creating buffer took %.4f", this, narrowPrecisionToFloat(duration));
#endif

    return buffer;
}

void MediaPlayerPrivateAVFoundationObjC::paintWithVideoOutput(GraphicsContext* context, const IntRect& rect)
{
    RetainPtr<CVPixelBufferRef> pixelBuffer = createPixelBuffer();

    // Calls to copyPixelBufferForItemTime:itemTimeForDisplay: may return nil if the pixel buffer
    // for the requested time has already been retrieved. In this case, the last valid image (if any)
    // should be displayed.
    if (pixelBuffer)
        m_lastImage = pixelBuffer;

    if (m_lastImage) {
        GraphicsContextStateSaver stateSaver(*context);
        context->translate(rect.x(), rect.y() + rect.height());
        context->scale(FloatSize(1.0f, -1.0f));
        RetainPtr<CIImage> image = adoptNS([[CIImage alloc] initWithCVImageBuffer:m_lastImage.get()]);

        // ciContext does not use a RetainPtr for results of contextWithCGContext:, as the returned value
        // is autoreleased, and there is no non-autoreleased version of that function.
        CIContext* ciContext = [CIContext contextWithCGContext:context->platformContext() options:nil];
        CGRect outputRect = { CGPointZero, rect.size() };
        CGRect imageRect = CGRectMake(0, 0, CVPixelBufferGetWidth(m_lastImage.get()), CVPixelBufferGetHeight(m_lastImage.get()));
        [ciContext drawImage:image.get() inRect:outputRect fromRect:imageRect];
    }
}
#endif

#if ENABLE(ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA_V2)
bool MediaPlayerPrivateAVFoundationObjC::extractKeyURIKeyIDAndCertificateFromInitData(Uint8Array* initData, String& keyURI, String& keyID, RefPtr<Uint8Array>& certificate)
{
    // initData should have the following layout:
    // [4 bytes: keyURI length][N bytes: keyURI][4 bytes: contentID length], [N bytes: contentID], [4 bytes: certificate length][N bytes: certificate]
    if (initData->byteLength() < 4)
        return false;

    RefPtr<ArrayBuffer> initDataBuffer = initData->buffer();

    // Use a DataView to read uint32 values from the buffer, as Uint32Array requires the reads be aligned on 4-byte boundaries. 
    RefPtr<DataView> initDataView = DataView::create(initDataBuffer, 0, initDataBuffer->byteLength());
    uint32_t offset = 0;
    ExceptionCode ec = 0;

    uint32_t keyURILength = initDataView->getUint32(offset, true, ec);
    offset += 4;
    if (ec || offset + keyURILength > initData->length())
        return false;

    RefPtr<Uint16Array> keyURIArray = Uint16Array::create(initDataBuffer, offset, keyURILength);
    if (!keyURIArray)
        return false;

    keyURI = String(keyURIArray->data(), keyURILength / sizeof(unsigned short));
    offset += keyURILength;

    uint32_t keyIDLength = initDataView->getUint32(offset, true, ec);
    offset += 4;
    if (ec || offset + keyIDLength > initData->length())
        return false;

    RefPtr<Uint16Array> keyIDArray = Uint16Array::create(initDataBuffer, offset, keyIDLength);
    if (!keyIDArray)
        return false;

    keyID = String(keyIDArray->data(), keyIDLength / sizeof(unsigned short));
    offset += keyIDLength;

    uint32_t certificateLength = initDataView->getUint32(offset, true, ec);
    offset += 4;
    if (ec || offset + certificateLength > initData->length())
        return false;

    certificate = Uint8Array::create(initDataBuffer, offset, certificateLength);
    if (!certificate)
        return false;

    return true;
}
#endif

#if ENABLE(ENCRYPTED_MEDIA)
MediaPlayer::MediaKeyException MediaPlayerPrivateAVFoundationObjC::generateKeyRequest(const String& keySystem, const unsigned char* initDataPtr, unsigned initDataLength)
{
    if (!keySystemIsSupported(keySystem))
        return MediaPlayer::KeySystemNotSupported;

    RefPtr<Uint8Array> initData = Uint8Array::create(initDataPtr, initDataLength);
    String keyURI;
    String keyID;
    RefPtr<Uint8Array> certificate;
    if (!extractKeyURIKeyIDAndCertificateFromInitData(initData.get(), keyURI, keyID, certificate))
        return MediaPlayer::InvalidPlayerState;

    if (!m_keyURIToRequestMap.contains(keyURI))
        return MediaPlayer::InvalidPlayerState;

    String sessionID = createCanonicalUUIDString();

    RetainPtr<AVAssetResourceLoadingRequest> avRequest = m_keyURIToRequestMap.get(keyURI);

    RetainPtr<NSData> certificateData = adoptNS([[NSData alloc] initWithBytes:certificate->baseAddress() length:certificate->byteLength()]);
    NSString* assetStr = keyID;
    RetainPtr<NSData> assetID = [NSData dataWithBytes: [assetStr cStringUsingEncoding:NSUTF8StringEncoding] length:[assetStr lengthOfBytesUsingEncoding:NSUTF8StringEncoding]];
    NSError* error = 0;
    RetainPtr<NSData> keyRequest = [avRequest.get() streamingContentKeyRequestDataForApp:certificateData.get() contentIdentifier:assetID.get() options:nil error:&error];

    if (!keyRequest) {
        NSError* underlyingError = [[error userInfo] objectForKey:NSUnderlyingErrorKey];
        player()->keyError(keySystem, sessionID, MediaPlayerClient::DomainError, [underlyingError code]);
        return MediaPlayer::NoError;
    }

    RefPtr<ArrayBuffer> keyRequestBuffer = ArrayBuffer::create([keyRequest.get() bytes], [keyRequest.get() length]);
    RefPtr<Uint8Array> keyRequestArray = Uint8Array::create(keyRequestBuffer, 0, keyRequestBuffer->byteLength());
    player()->keyMessage(keySystem, sessionID, keyRequestArray->data(), keyRequestArray->byteLength(), KURL());

    // Move ownership of the AVAssetResourceLoadingRequestfrom the keyIDToRequestMap to the sessionIDToRequestMap:
    m_sessionIDToRequestMap.set(sessionID, avRequest);
    m_keyURIToRequestMap.remove(keyURI);

    return MediaPlayer::NoError;
}

MediaPlayer::MediaKeyException MediaPlayerPrivateAVFoundationObjC::addKey(const String& keySystem, const unsigned char* keyPtr, unsigned keyLength, const unsigned char* initDataPtr, unsigned initDataLength, const String& sessionID)
{
    if (!keySystemIsSupported(keySystem))
        return MediaPlayer::KeySystemNotSupported;

    if (!m_sessionIDToRequestMap.contains(sessionID))
        return MediaPlayer::InvalidPlayerState;

    RetainPtr<AVAssetResourceLoadingRequest> avRequest = m_sessionIDToRequestMap.get(sessionID);
    RetainPtr<NSData> keyData = adoptNS([[NSData alloc] initWithBytes:keyPtr length:keyLength]);
    [[avRequest.get() dataRequest] respondWithData:keyData.get()];
    [avRequest.get() finishLoading];
    m_sessionIDToRequestMap.remove(sessionID);

    player()->keyAdded(keySystem, sessionID);

    UNUSED_PARAM(initDataPtr);
    UNUSED_PARAM(initDataLength);
    return MediaPlayer::NoError;
}

MediaPlayer::MediaKeyException MediaPlayerPrivateAVFoundationObjC::cancelKeyRequest(const String& keySystem, const String& sessionID)
{
    if (!keySystemIsSupported(keySystem))
        return MediaPlayer::KeySystemNotSupported;

    if (!m_sessionIDToRequestMap.contains(sessionID))
        return MediaPlayer::InvalidPlayerState;

    m_sessionIDToRequestMap.remove(sessionID);
    return MediaPlayer::NoError;
}
#endif

#if ENABLE(ENCRYPTED_MEDIA_V2)
RetainPtr<AVAssetResourceLoadingRequest> MediaPlayerPrivateAVFoundationObjC::takeRequestForPlayerAndKeyURI(MediaPlayer* player, const String& keyURI)
{
    MediaPlayerPrivateAVFoundationObjC* _this = playerToPrivateMap().get(player);
    if (!_this)
        return nullptr;

    return _this->m_keyURIToRequestMap.take(keyURI);

}
#endif

#if !HAVE(AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT)
void MediaPlayerPrivateAVFoundationObjC::processLegacyClosedCaptionsTracks()
{
#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
    [m_avPlayerItem.get() selectMediaOption:nil inMediaSelectionGroup:safeMediaSelectionGroupForLegibleMedia()];
#endif

    Vector<RefPtr<InbandTextTrackPrivateAVF> > removedTextTracks = m_textTracks;
    NSArray *tracks = [m_avPlayerItem.get() tracks];
    for (AVPlayerItemTrack *playerItemTrack in tracks) {

        AVAssetTrack *assetTrack = [playerItemTrack assetTrack];
        if (![[assetTrack mediaType] isEqualToString:AVMediaTypeClosedCaption])
            continue;

        bool newCCTrack = true;
        for (unsigned i = removedTextTracks.size(); i > 0; --i) {
            if (!removedTextTracks[i - 1]->isLegacyClosedCaptionsTrack())
                continue;

            RefPtr<InbandTextTrackPrivateLegacyAVFObjC> track = static_cast<InbandTextTrackPrivateLegacyAVFObjC*>(m_textTracks[i - 1].get());
            if (track->avPlayerItemTrack() == playerItemTrack) {
                removedTextTracks.remove(i - 1);
                newCCTrack = false;
                break;
            }
        }

        if (!newCCTrack)
            continue;
        
        m_textTracks.append(InbandTextTrackPrivateLegacyAVFObjC::create(this, playerItemTrack));
    }

    processNewAndRemovedTextTracks(removedTextTracks);
}
#endif

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
AVMediaSelectionGroupType* MediaPlayerPrivateAVFoundationObjC::safeMediaSelectionGroupForLegibleMedia()
{
    if (!m_avAsset)
        return nil;
    
    if ([m_avAsset.get() statusOfValueForKey:@"availableMediaCharacteristicsWithMediaSelectionOptions" error:NULL] != AVKeyValueStatusLoaded)
        return nil;
    
    return [m_avAsset.get() mediaSelectionGroupForMediaCharacteristic:AVMediaCharacteristicLegible];
}

void MediaPlayerPrivateAVFoundationObjC::processMediaSelectionOptions()
{
    AVMediaSelectionGroupType *legibleGroup = safeMediaSelectionGroupForLegibleMedia();
    if (!legibleGroup) {
        LOG(Media, "MediaPlayerPrivateAVFoundationObjC::processMediaSelectionOptions(%p) - nil mediaSelectionGroup", this);
        return;
    }

    // We enabled automatic media selection because we want alternate audio tracks to be enabled/disabled automatically,
    // but set the selected legible track to nil so text tracks will not be automatically configured.
    if (!m_textTracks.size())
        [m_avPlayerItem.get() selectMediaOption:nil inMediaSelectionGroup:safeMediaSelectionGroupForLegibleMedia()];

    Vector<RefPtr<InbandTextTrackPrivateAVF> > removedTextTracks = m_textTracks;
    NSArray *legibleOptions = [AVMediaSelectionGroup playableMediaSelectionOptionsFromArray:[legibleGroup options]];
    for (AVMediaSelectionOptionType *option in legibleOptions) {
        bool newTrack = true;
        for (unsigned i = removedTextTracks.size(); i > 0; --i) {
             if (removedTextTracks[i - 1]->isLegacyClosedCaptionsTrack())
                 continue;

            RefPtr<InbandTextTrackPrivateAVFObjC> track = static_cast<InbandTextTrackPrivateAVFObjC*>(removedTextTracks[i - 1].get());
            if ([track->mediaSelectionOption() isEqual:option]) {
                removedTextTracks.remove(i - 1);
                newTrack = false;
                break;
            }
        }
        if (!newTrack)
            continue;

        m_textTracks.append(InbandTextTrackPrivateAVFObjC::create(this, option));
    }

    processNewAndRemovedTextTracks(removedTextTracks);
}

void MediaPlayerPrivateAVFoundationObjC::processCue(NSArray *attributedStrings, double time)
{
    if (!m_currentTrack)
        return;

    m_currentTrack->processCue(reinterpret_cast<CFArrayRef>(attributedStrings), time);
}

void MediaPlayerPrivateAVFoundationObjC::flushCues()
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::flushCues(%p)", this);

    if (!m_currentTrack)
        return;
    
    m_currentTrack->resetCueValues();
}
#endif // HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)

void MediaPlayerPrivateAVFoundationObjC::setCurrentTrack(InbandTextTrackPrivateAVF *track)
{
    if (m_currentTrack == track)
        return;

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::setCurrentTrack(%p) - selecting track %p, language = %s", this, track, track ? track->language().string().utf8().data() : "");
        
    m_currentTrack = track;

    if (track) {
        if (track->isLegacyClosedCaptionsTrack())
            [m_avPlayer.get() setClosedCaptionDisplayEnabled:YES];
#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
        else
            [m_avPlayerItem.get() selectMediaOption:static_cast<InbandTextTrackPrivateAVFObjC*>(track)->mediaSelectionOption() inMediaSelectionGroup:safeMediaSelectionGroupForLegibleMedia()];
#endif
    } else {
#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
        [m_avPlayerItem.get() selectMediaOption:0 inMediaSelectionGroup:safeMediaSelectionGroupForLegibleMedia()];
#endif
        [m_avPlayer.get() setClosedCaptionDisplayEnabled:NO];
    }

}

String MediaPlayerPrivateAVFoundationObjC::languageOfPrimaryAudioTrack() const
{
    if (!m_languageOfPrimaryAudioTrack.isNull())
        return m_languageOfPrimaryAudioTrack;

    if (!m_avPlayerItem.get())
        return emptyString();

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
    // If AVFoundation has an audible group, return the language of the currently selected audible option.
    AVMediaSelectionGroupType *audibleGroup = [m_avAsset.get() mediaSelectionGroupForMediaCharacteristic:AVMediaCharacteristicAudible];
    AVMediaSelectionOptionType *currentlySelectedAudibleOption = [m_avPlayerItem.get() selectedMediaOptionInMediaSelectionGroup:audibleGroup];
    if (currentlySelectedAudibleOption) {
        m_languageOfPrimaryAudioTrack = [[currentlySelectedAudibleOption locale] localeIdentifier];
        LOG(Media, "MediaPlayerPrivateAVFoundationObjC::languageOfPrimaryAudioTrack(%p) - returning language of selected audible option: %s", this, m_languageOfPrimaryAudioTrack.utf8().data());

        return m_languageOfPrimaryAudioTrack;
    }
#endif // HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)

    // AVFoundation synthesizes an audible group when there is only one ungrouped audio track if there is also a legible group (one or
    // more in-band text tracks). It doesn't know about out-of-band tracks, so if there is a single audio track return its language.
    NSArray *tracks = [m_avAsset.get() tracksWithMediaType:AVMediaTypeAudio];
    if (!tracks || [tracks count] != 1) {
        m_languageOfPrimaryAudioTrack = emptyString();
        LOG(Media, "MediaPlayerPrivateAVFoundationObjC::languageOfPrimaryAudioTrack(%p) - %i audio tracks, returning emptyString()", this, (tracks ? [tracks count] : 0));
        return m_languageOfPrimaryAudioTrack;
    }

    AVAssetTrack *track = [tracks objectAtIndex:0];
    NSString *language = [track extendedLanguageTag];

    // Some legacy tracks have "und" as a language, treat that the same as no language at all.
    if (language && ![language isEqualToString:@"und"]) {
        m_languageOfPrimaryAudioTrack = language;
        LOG(Media, "MediaPlayerPrivateAVFoundationObjC::languageOfPrimaryAudioTrack(%p) - returning language of single audio track: %s", this, m_languageOfPrimaryAudioTrack.utf8().data());
        return m_languageOfPrimaryAudioTrack;
    }

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::languageOfPrimaryAudioTrack(%p) - single audio track has no language, returning emptyString()", this);
    m_languageOfPrimaryAudioTrack = emptyString();
    return m_languageOfPrimaryAudioTrack;
}

NSArray* assetMetadataKeyNames()
{
    static NSArray* keys;
    if (!keys) {
        keys = [[NSArray alloc] initWithObjects:@"duration",
                    @"naturalSize",
                    @"preferredTransform",
                    @"preferredVolume",
                    @"preferredRate",
                    @"playable",
                    @"tracks",
                    @"availableMediaCharacteristicsWithMediaSelectionOptions",
                   nil];
    }
    return keys;
}

NSArray* itemKVOProperties()
{
    static NSArray* keys;
    if (!keys) {
        keys = [[NSArray alloc] initWithObjects:@"presentationSize",
                @"status",
                @"asset",
                @"tracks",
                @"seekableTimeRanges",
                @"loadedTimeRanges",
                @"playbackLikelyToKeepUp",
                @"playbackBufferFull",
                @"playbackBufferEmpty",
                @"duration",
                @"hasEnabledAudio",
                nil];
    }
    return keys;
}

} // namespace WebCore

@implementation WebCoreAVFMovieObserver

- (id)initWithCallback:(MediaPlayerPrivateAVFoundationObjC*)callback
{
    m_callback = callback;
    return [super init];
}

- (void)disconnect
{
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
    m_callback = 0;
}

- (void)metadataLoaded
{
    if (!m_callback)
        return;

    m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::AssetMetadataLoaded);
}

- (void)playableKnown
{
    if (!m_callback)
        return;

    m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::AssetPlayabilityKnown);
}

- (void)seekCompleted:(BOOL)finished
{
    if (!m_callback)
        return;
    
    m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::SeekCompleted, static_cast<bool>(finished));
}

- (void)didEnd:(NSNotification *)unusedNotification
{
    UNUSED_PARAM(unusedNotification);
    if (!m_callback)
        return;
    m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::ItemDidPlayToEndTime);
}

- (void)observeValueForKeyPath:keyPath ofObject:(id)object change:(NSDictionary *)change context:(MediaPlayerAVFoundationObservationContext)context
{
    UNUSED_PARAM(change);

    LOG(Media, "WebCoreAVFMovieObserver:observeValueForKeyPath(%p) - keyPath = %s", self, [keyPath UTF8String]);

    if (!m_callback)
        return;

    if (context == MediaPlayerAVFoundationObservationContextPlayerItem) {
        // A value changed for an AVPlayerItem
        if ([keyPath isEqualToString:@"status"])
            m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::ItemStatusChanged);
        else if ([keyPath isEqualToString:@"playbackLikelyToKeepUp"])
            m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::ItemIsPlaybackLikelyToKeepUpChanged);
        else if ([keyPath isEqualToString:@"playbackBufferEmpty"])
            m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::ItemIsPlaybackBufferEmptyChanged);
        else if ([keyPath isEqualToString:@"playbackBufferFull"])
            m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::ItemIsPlaybackBufferFullChanged);
        else if ([keyPath isEqualToString:@"asset"])
            m_callback->setAsset([object asset]);
        else if ([keyPath isEqualToString:@"loadedTimeRanges"])
            m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::ItemLoadedTimeRangesChanged);
        else if ([keyPath isEqualToString:@"seekableTimeRanges"])
            m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::ItemSeekableTimeRangesChanged);
        else if ([keyPath isEqualToString:@"tracks"])
            m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::ItemTracksChanged);
        else if ([keyPath isEqualToString:@"hasEnabledAudio"])
            m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::ItemTracksChanged);
        else if ([keyPath isEqualToString:@"presentationSize"])
            m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::ItemPresentationSizeChanged);
        else if ([keyPath isEqualToString:@"duration"])
            m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::DurationChanged);

        return;
    }

    if (context == MediaPlayerAVFoundationObservationContextPlayer) {
        // A value changed for an AVPlayer.
        if ([keyPath isEqualToString:@"rate"])
            m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::PlayerRateChanged);
    }
}

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
- (void)legibleOutput:(id)output didOutputAttributedStrings:(NSArray *)strings nativeSampleBuffers:(NSArray *)nativeSamples forItemTime:(CMTime)itemTime
{
    UNUSED_PARAM(output);
    UNUSED_PARAM(nativeSamples);

    if (!m_callback)
        return;

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!m_callback)
            return;
        m_callback->processCue(strings, CMTimeGetSeconds(itemTime));
    });
}

- (void)outputSequenceWasFlushed:(id)output
{
    UNUSED_PARAM(output);

    if (!m_callback)
        return;
    
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!m_callback)
            return;
        m_callback->flushCues();
    });
}
#endif

@end

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
@implementation WebCoreAVFLoaderDelegate

- (id)initWithCallback:(MediaPlayerPrivateAVFoundationObjC*)callback
{
    m_callback = callback;
    return [super init];
}

- (BOOL)resourceLoader:(AVAssetResourceLoader *)resourceLoader shouldWaitForLoadingOfRequestedResource:(AVAssetResourceLoadingRequest *)loadingRequest
{
    UNUSED_PARAM(resourceLoader);
    if (!m_callback)
        return NO;

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!m_callback) {
            [loadingRequest finishLoadingWithError:nil];
            return;
        }

        if (!m_callback->shouldWaitForLoadingOfResource(loadingRequest))
            [loadingRequest finishLoadingWithError:nil];
    });

    return YES;
}

- (void)resourceLoader:(AVAssetResourceLoader *)resourceLoader didCancelLoadingRequest:(AVAssetResourceLoadingRequest *)loadingRequest
{
    UNUSED_PARAM(resourceLoader);
    if (!m_callback)
        return;

    dispatch_async(dispatch_get_main_queue(), ^{
        if (m_callback)
            m_callback->didCancelLoadingRequest(loadingRequest);
    });
}

- (void)setCallback:(MediaPlayerPrivateAVFoundationObjC*)callback
{
    m_callback = callback;
}
@end
#endif

#endif
