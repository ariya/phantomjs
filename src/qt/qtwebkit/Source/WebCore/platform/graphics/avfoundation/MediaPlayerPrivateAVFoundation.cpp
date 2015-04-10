/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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

#include "config.h"

#if ENABLE(VIDEO) && USE(AVFOUNDATION)

#include "MediaPlayerPrivateAVFoundation.h"

#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "InbandTextTrackPrivateAVF.h"
#include "InbandTextTrackPrivateClient.h"
#include "KURL.h"
#include "Logging.h"
#include "PlatformLayer.h"
#include "SoftLinking.h"
#include "TimeRanges.h"
#include <CoreMedia/CoreMedia.h>
#include <wtf/MainThread.h>

using namespace std;

namespace WebCore {

MediaPlayerPrivateAVFoundation::MediaPlayerPrivateAVFoundation(MediaPlayer* player)
    : m_player(player)
    , m_queuedNotifications()
    , m_queueMutex()
    , m_networkState(MediaPlayer::Empty)
    , m_readyState(MediaPlayer::HaveNothing)
    , m_preload(MediaPlayer::Auto)
    , m_cachedMaxTimeLoaded(0)
    , m_cachedMaxTimeSeekable(0)
    , m_cachedMinTimeSeekable(0)
    , m_cachedDuration(MediaPlayer::invalidTime())
    , m_reportedDuration(MediaPlayer::invalidTime())
    , m_maxTimeLoadedAtLastDidLoadingProgress(MediaPlayer::invalidTime())
    , m_seekTo(MediaPlayer::invalidTime())
    , m_requestedRate(1)
    , m_delayCallbacks(0)
    , m_mainThreadCallPending(false)
    , m_assetIsPlayable(false)
    , m_visible(false)
    , m_loadingMetadata(false)
    , m_isAllowedToRender(false)
    , m_cachedHasAudio(false)
    , m_cachedHasVideo(false)
    , m_cachedHasCaptions(false)
    , m_ignoreLoadStateChanges(false)
    , m_haveReportedFirstVideoFrame(false)
    , m_playWhenFramesAvailable(false)
    , m_inbandTrackConfigurationPending(false)
    , m_seekCount(0)
{
    LOG(Media, "MediaPlayerPrivateAVFoundation::MediaPlayerPrivateAVFoundation(%p)", this);
}

MediaPlayerPrivateAVFoundation::~MediaPlayerPrivateAVFoundation()
{
    LOG(Media, "MediaPlayerPrivateAVFoundation::~MediaPlayerPrivateAVFoundation(%p)", this);
    setIgnoreLoadStateChanges(true);
    cancelCallOnMainThread(mainThreadCallback, this);
}

MediaPlayerPrivateAVFoundation::MediaRenderingMode MediaPlayerPrivateAVFoundation::currentRenderingMode() const
{
#if USE(ACCELERATED_COMPOSITING)
    if (platformLayer())
        return MediaRenderingToLayer;
#endif

    if (hasContextRenderer())
        return MediaRenderingToContext;

    return MediaRenderingNone;
}

MediaPlayerPrivateAVFoundation::MediaRenderingMode MediaPlayerPrivateAVFoundation::preferredRenderingMode() const
{
    if (!m_player->visible() || !m_player->frameView() || assetStatus() == MediaPlayerAVAssetStatusUnknown)
        return MediaRenderingNone;

#if USE(ACCELERATED_COMPOSITING)
    if (supportsAcceleratedRendering() && m_player->mediaPlayerClient()->mediaPlayerRenderingCanBeAccelerated(m_player))
        return MediaRenderingToLayer;
#endif

    return MediaRenderingToContext;
}

void MediaPlayerPrivateAVFoundation::setUpVideoRendering()
{
    if (!isReadyForVideoSetup())
        return;

    MediaRenderingMode currentMode = currentRenderingMode();
    MediaRenderingMode preferredMode = preferredRenderingMode();

    if (preferredMode == MediaRenderingNone)
        preferredMode = MediaRenderingToContext;

    if (currentMode == preferredMode && currentMode != MediaRenderingNone)
        return;

    LOG(Media, "MediaPlayerPrivateAVFoundation::setUpVideoRendering(%p) - current mode = %d, preferred mode = %d", 
        this, static_cast<int>(currentMode), static_cast<int>(preferredMode));

    if (currentMode != MediaRenderingNone)  
        tearDownVideoRendering();

    switch (preferredMode) {
    case MediaRenderingNone:
    case MediaRenderingToContext:
        createContextVideoRenderer();
        break;
        
#if USE(ACCELERATED_COMPOSITING)
    case MediaRenderingToLayer:
        createVideoLayer();
        break;
#endif
    }

#if USE(ACCELERATED_COMPOSITING)
    // If using a movie layer, inform the client so the compositing tree is updated.
    if (currentMode == MediaRenderingToLayer || preferredMode == MediaRenderingToLayer) {
        LOG(Media, "MediaPlayerPrivateAVFoundation::setUpVideoRendering(%p) - calling mediaPlayerRenderingModeChanged()", this);
        m_player->mediaPlayerClient()->mediaPlayerRenderingModeChanged(m_player);
    }
#endif
}

void MediaPlayerPrivateAVFoundation::tearDownVideoRendering()
{
    LOG(Media, "MediaPlayerPrivateAVFoundation::tearDownVideoRendering(%p)", this);

    destroyContextVideoRenderer();

#if USE(ACCELERATED_COMPOSITING)
    if (platformLayer())
        destroyVideoLayer();
#endif
}

bool MediaPlayerPrivateAVFoundation::hasSetUpVideoRendering() const
{
    return hasLayerRenderer() || hasContextRenderer();
}

void MediaPlayerPrivateAVFoundation::load(const String& url)
{
    LOG(Media, "MediaPlayerPrivateAVFoundation::load(%p)", this);

    if (m_networkState != MediaPlayer::Loading) {
        m_networkState = MediaPlayer::Loading;
        m_player->networkStateChanged();
    }
    if (m_readyState != MediaPlayer::HaveNothing) {
        m_readyState = MediaPlayer::HaveNothing;
        m_player->readyStateChanged();
    }

    m_assetURL = url;

    // Don't do any more work if the url is empty.
    if (!url.length())
        return;

    setPreload(m_preload);
}

void MediaPlayerPrivateAVFoundation::playabilityKnown()
{
    LOG(Media, "MediaPlayerPrivateAVFoundation::playabilityKnown(%p)", this);

    if (m_assetIsPlayable)
        return;

    // Nothing more to do if we already have all of the item's metadata.
    if (assetStatus() > MediaPlayerAVAssetStatusLoading) {
        LOG(Media, "MediaPlayerPrivateAVFoundation::playabilityKnown(%p) - all metadata loaded", this);
        return;
    }

    // At this point we are supposed to load metadata. It is OK to ask the asset to load the same 
    // information multiple times, because if it has already been loaded the completion handler 
    // will just be called synchronously.
    m_loadingMetadata = true;
    beginLoadingMetadata();
}

void MediaPlayerPrivateAVFoundation::prepareToPlay()
{
    LOG(Media, "MediaPlayerPrivateAVFoundation::prepareToPlay(%p)", this);

    setPreload(MediaPlayer::Auto);
}

void MediaPlayerPrivateAVFoundation::play()
{
    LOG(Media, "MediaPlayerPrivateAVFoundation::play(%p)", this);

    // If the file has video, don't request playback until the first frame of video is ready to display
    // or the audio may start playing before we can render video.
    if (!m_cachedHasVideo || hasAvailableVideoFrame())
        platformPlay();
    else
        m_playWhenFramesAvailable = true;
}

void MediaPlayerPrivateAVFoundation::pause()
{
    LOG(Media, "MediaPlayerPrivateAVFoundation::pause(%p)", this);
    m_playWhenFramesAvailable = false;
    platformPause();
}

float MediaPlayerPrivateAVFoundation::duration() const
{
    if (m_cachedDuration != MediaPlayer::invalidTime())
        return m_cachedDuration;

    float duration = platformDuration();
    if (!duration || duration == MediaPlayer::invalidTime())
        return 0;

    m_cachedDuration = duration;
    LOG(Media, "MediaPlayerPrivateAVFoundation::duration(%p) - caching %f", this, m_cachedDuration);
    return m_cachedDuration;
}

void MediaPlayerPrivateAVFoundation::seek(float time)
{
    if (!metaDataAvailable())
        return;

    if (time > duration())
        time = duration();

    if (currentTime() == time)
        return;

    if (currentTrack())
        currentTrack()->beginSeeking();
    
    LOG(Media, "MediaPlayerPrivateAVFoundation::seek(%p) - seeking to %f", this, time);
    m_seekTo = time;

    ++m_seekCount;
    seekToTime(time);
}

void MediaPlayerPrivateAVFoundation::setRate(float rate)
{
    LOG(Media, "MediaPlayerPrivateAVFoundation::setRate(%p) - seting to %f", this, rate);
    m_requestedRate = rate;

    updateRate();
}

bool MediaPlayerPrivateAVFoundation::paused() const
{
    if (!metaDataAvailable())
        return true;

    return rate() == 0;
}

bool MediaPlayerPrivateAVFoundation::seeking() const
{
    if (!metaDataAvailable())
        return false;

    return m_seekTo != MediaPlayer::invalidTime();
}

IntSize MediaPlayerPrivateAVFoundation::naturalSize() const
{
    if (!metaDataAvailable())
        return IntSize();

    // In spite of the name of this method, return the natural size transformed by the 
    // initial movie scale because the spec says intrinsic size is:
    //
    //    ... the dimensions of the resource in CSS pixels after taking into account the resource's 
    //    dimensions, aspect ratio, clean aperture, resolution, and so forth, as defined for the 
    //    format used by the resource

    return m_cachedNaturalSize;
}

void MediaPlayerPrivateAVFoundation::setNaturalSize(IntSize size)
{
    LOG(Media, "MediaPlayerPrivateAVFoundation:setNaturalSize(%p) - size = %d x %d", this, size.width(), size.height());

    IntSize oldSize = m_cachedNaturalSize;
    m_cachedNaturalSize = size;
    if (oldSize != m_cachedNaturalSize)
        m_player->sizeChanged();
}

void MediaPlayerPrivateAVFoundation::setHasVideo(bool b)
{
    if (m_cachedHasVideo != b) {
        m_cachedHasVideo = b;
        m_player->characteristicChanged();
    }
}

void MediaPlayerPrivateAVFoundation::setHasAudio(bool b)
{
    if (m_cachedHasAudio != b) {
        m_cachedHasAudio = b;
        m_player->characteristicChanged();
    }
}

void MediaPlayerPrivateAVFoundation::setHasClosedCaptions(bool b)
{
    if (m_cachedHasCaptions != b) {
        m_cachedHasCaptions = b;
        m_player->characteristicChanged();
    }
}

PassRefPtr<TimeRanges> MediaPlayerPrivateAVFoundation::buffered() const
{
    if (!m_cachedLoadedTimeRanges)
        m_cachedLoadedTimeRanges = platformBufferedTimeRanges();

    return m_cachedLoadedTimeRanges->copy();
}

double MediaPlayerPrivateAVFoundation::maxTimeSeekableDouble() const
{
    if (!metaDataAvailable())
        return 0;

    if (!m_cachedMaxTimeSeekable)
        m_cachedMaxTimeSeekable = platformMaxTimeSeekable();

    LOG(Media, "MediaPlayerPrivateAVFoundation::maxTimeSeekable(%p) - returning %f", this, m_cachedMaxTimeSeekable);
    return m_cachedMaxTimeSeekable;   
}

double MediaPlayerPrivateAVFoundation::minTimeSeekable() const
{
    if (!metaDataAvailable())
        return 0;

    if (!m_cachedMinTimeSeekable)
        m_cachedMinTimeSeekable = platformMinTimeSeekable();

    LOG(Media, "MediaPlayerPrivateAVFoundation::minTimeSeekable(%p) - returning %f", this, m_cachedMinTimeSeekable);
    return m_cachedMinTimeSeekable;
}

float MediaPlayerPrivateAVFoundation::maxTimeLoaded() const
{
    if (!metaDataAvailable())
        return 0;

    if (!m_cachedMaxTimeLoaded)
        m_cachedMaxTimeLoaded = platformMaxTimeLoaded();

    return m_cachedMaxTimeLoaded;   
}

bool MediaPlayerPrivateAVFoundation::didLoadingProgress() const
{
    if (!duration() || !totalBytes())
        return false;
    float currentMaxTimeLoaded = maxTimeLoaded();
    bool didLoadingProgress = currentMaxTimeLoaded != m_maxTimeLoadedAtLastDidLoadingProgress;
    m_maxTimeLoadedAtLastDidLoadingProgress = currentMaxTimeLoaded;

    return didLoadingProgress;
}

bool MediaPlayerPrivateAVFoundation::isReadyForVideoSetup() const
{
    // AVFoundation will not return true for firstVideoFrameAvailable until
    // an AVPlayerLayer has been added to the AVPlayerItem, so allow video setup
    // here if a video track to trigger allocation of a AVPlayerLayer.
    return (m_isAllowedToRender || m_cachedHasVideo) && m_readyState >= MediaPlayer::HaveMetadata && m_player->visible();
}

void MediaPlayerPrivateAVFoundation::prepareForRendering()
{
    if (m_isAllowedToRender)
        return;
    m_isAllowedToRender = true;

    setUpVideoRendering();

    if (currentRenderingMode() == MediaRenderingToLayer || preferredRenderingMode() == MediaRenderingToLayer)
        m_player->mediaPlayerClient()->mediaPlayerRenderingModeChanged(m_player);
}

bool MediaPlayerPrivateAVFoundation::supportsFullscreen() const
{
#if ENABLE(FULLSCREEN_API)
    return true;
#else
    // FIXME: WebVideoFullscreenController assumes a QTKit/QuickTime media engine
    return false;
#endif
}

void MediaPlayerPrivateAVFoundation::updateStates()
{
    if (m_ignoreLoadStateChanges)
        return;

    MediaPlayer::NetworkState oldNetworkState = m_networkState;
    MediaPlayer::ReadyState oldReadyState = m_readyState;

    if (m_loadingMetadata)
        m_networkState = MediaPlayer::Loading;
    else {
        // -loadValuesAsynchronouslyForKeys:completionHandler: has invoked its handler; test status of keys and determine state.
        AssetStatus assetStatus = this->assetStatus();
        ItemStatus itemStatus = playerItemStatus();
        
        m_assetIsPlayable = (assetStatus == MediaPlayerAVAssetStatusPlayable);
        if (m_readyState < MediaPlayer::HaveMetadata && assetStatus > MediaPlayerAVAssetStatusLoading) {
            if (m_assetIsPlayable) {
                if (assetStatus >= MediaPlayerAVAssetStatusLoaded)
                    m_readyState = MediaPlayer::HaveMetadata;
                if (itemStatus <= MediaPlayerAVPlayerItemStatusUnknown) {
                    if (assetStatus == MediaPlayerAVAssetStatusFailed || m_preload > MediaPlayer::MetaData || isLiveStream()) {
                        // The asset is playable but doesn't support inspection prior to playback (eg. streaming files),
                        // or we are supposed to prepare for playback immediately, so create the player item now.
                        m_networkState = MediaPlayer::Loading;
                        prepareToPlay();
                    } else
                        m_networkState = MediaPlayer::Idle;
                }
            } else {
                // FIX ME: fetch the error associated with the @"playable" key to distinguish between format 
                // and network errors.
                m_networkState = MediaPlayer::FormatError;
            }
        }
        
        if (assetStatus >= MediaPlayerAVAssetStatusLoaded && itemStatus > MediaPlayerAVPlayerItemStatusUnknown) {
            switch (itemStatus) {
            case MediaPlayerAVPlayerItemStatusDoesNotExist:
            case MediaPlayerAVPlayerItemStatusUnknown:
            case MediaPlayerAVPlayerItemStatusFailed:
                break;

            case MediaPlayerAVPlayerItemStatusPlaybackLikelyToKeepUp:
            case MediaPlayerAVPlayerItemStatusPlaybackBufferFull:
                // If the status becomes PlaybackBufferFull, loading stops and the status will not
                // progress to LikelyToKeepUp. Set the readyState to  HAVE_ENOUGH_DATA, on the
                // presumption that if the playback buffer is full, playback will probably not stall.
                m_readyState = MediaPlayer::HaveEnoughData;
                break;

            case MediaPlayerAVPlayerItemStatusReadyToPlay:
                // If the readyState is already HaveEnoughData, don't go lower because of this state change.
                if (m_readyState == MediaPlayer::HaveEnoughData)
                    break;

            case MediaPlayerAVPlayerItemStatusPlaybackBufferEmpty:
                if (maxTimeLoaded() > currentTime())
                    m_readyState = MediaPlayer::HaveFutureData;
                else
                    m_readyState = MediaPlayer::HaveCurrentData;
                break;
            }

            if (itemStatus == MediaPlayerAVPlayerItemStatusPlaybackBufferFull)
                m_networkState = MediaPlayer::Idle;
            else if (itemStatus == MediaPlayerAVPlayerItemStatusFailed)
                m_networkState = MediaPlayer::DecodeError;
            else if (itemStatus != MediaPlayerAVPlayerItemStatusPlaybackBufferFull && itemStatus >= MediaPlayerAVPlayerItemStatusReadyToPlay)
                m_networkState = (maxTimeLoaded() == duration()) ? MediaPlayer::Loaded : MediaPlayer::Loading;
        }
    }

    if (isReadyForVideoSetup() && currentRenderingMode() != preferredRenderingMode())
        setUpVideoRendering();

    if (!m_haveReportedFirstVideoFrame && m_cachedHasVideo && hasAvailableVideoFrame()) {
        if (m_readyState < MediaPlayer::HaveCurrentData)
            m_readyState = MediaPlayer::HaveCurrentData;
        m_haveReportedFirstVideoFrame = true;
        m_player->firstVideoFrameAvailable();
    }

    if (m_networkState != oldNetworkState)
        m_player->networkStateChanged();

    if (m_readyState != oldReadyState)
        m_player->readyStateChanged();

    if (m_playWhenFramesAvailable && hasAvailableVideoFrame()) {
        m_playWhenFramesAvailable = false;
        platformPlay();
    }

#if !LOG_DISABLED
    if (m_networkState != oldNetworkState || oldReadyState != m_readyState) {
        LOG(Media, "MediaPlayerPrivateAVFoundation::updateStates(%p) - entered with networkState = %i, readyState = %i,  exiting with networkState = %i, readyState = %i",
            this, static_cast<int>(oldNetworkState), static_cast<int>(oldReadyState), static_cast<int>(m_networkState), static_cast<int>(m_readyState));
    }
#endif
}

void MediaPlayerPrivateAVFoundation::setSize(const IntSize&) 
{ 
}

void MediaPlayerPrivateAVFoundation::setVisible(bool visible)
{
    if (m_visible == visible)
        return;

    m_visible = visible;
    if (visible)
        setUpVideoRendering();
    
    platformSetVisible(visible);
}

void MediaPlayerPrivateAVFoundation::acceleratedRenderingStateChanged()
{
    // Set up or change the rendering path if necessary.
    setUpVideoRendering();
}

void MediaPlayerPrivateAVFoundation::metadataLoaded()
{
    m_loadingMetadata = false;
    tracksChanged();
}

void MediaPlayerPrivateAVFoundation::rateChanged()
{
    m_player->rateChanged();
}

void MediaPlayerPrivateAVFoundation::loadedTimeRangesChanged()
{
    m_cachedLoadedTimeRanges = 0;
    m_cachedMaxTimeLoaded = 0;
    invalidateCachedDuration();
}

void MediaPlayerPrivateAVFoundation::seekableTimeRangesChanged()
{
    m_cachedMaxTimeSeekable = 0;
    m_cachedMinTimeSeekable = 0;
}

void MediaPlayerPrivateAVFoundation::timeChanged(double time)
{
    LOG(Media, "MediaPlayerPrivateAVFoundation::timeChanged(%p) - time = %f", this, time);
    UNUSED_PARAM(time);
}

void MediaPlayerPrivateAVFoundation::seekCompleted(bool finished)
{
    LOG(Media, "MediaPlayerPrivateAVFoundation::seekCompleted(%p) - finished = %d", this, finished);
    UNUSED_PARAM(finished);

    ASSERT(m_seekCount);
    if (--m_seekCount)
        return;

    if (currentTrack())
        currentTrack()->endSeeking();

    m_seekTo = MediaPlayer::invalidTime();
    updateStates();
    m_player->timeChanged();
}

void MediaPlayerPrivateAVFoundation::didEnd()
{
    // Hang onto the current time and use it as duration from now on since we are definitely at
    // the end of the movie. Do this because the initial duration is sometimes an estimate.
    float now = currentTime();
    if (now > 0)
        m_cachedDuration = now;

    updateStates();
    m_player->timeChanged();
}

void MediaPlayerPrivateAVFoundation::invalidateCachedDuration()
{
    LOG(Media, "MediaPlayerPrivateAVFoundation::invalidateCachedDuration(%p)", this);
    
    m_cachedDuration = MediaPlayer::invalidTime();

    // For some media files, reported duration is estimated and updated as media is loaded
    // so report duration changed when the estimate is upated.
    float duration = this->duration();
    if (duration != m_reportedDuration) {
        if (m_reportedDuration != MediaPlayer::invalidTime())
            m_player->durationChanged();
        m_reportedDuration = duration;
    }
    
}

void MediaPlayerPrivateAVFoundation::repaint()
{
    m_player->repaint();
}

MediaPlayer::MovieLoadType MediaPlayerPrivateAVFoundation::movieLoadType() const
{
    if (!metaDataAvailable() || assetStatus() == MediaPlayerAVAssetStatusUnknown)
        return MediaPlayer::Unknown;

    if (isLiveStream())
        return MediaPlayer::LiveStream;

    return MediaPlayer::Download;
}

void MediaPlayerPrivateAVFoundation::setPreload(MediaPlayer::Preload preload)
{
    m_preload = preload;
    if (!m_assetURL.length())
        return;

    setDelayCallbacks(true);

    if (m_preload >= MediaPlayer::MetaData && assetStatus() == MediaPlayerAVAssetStatusDoesNotExist) {
        createAVAssetForURL(m_assetURL);
        checkPlayability();
    }

    // Don't force creation of the player and player item unless we already know that the asset is playable. If we aren't
    // there yet, or if we already know it is not playable, creating them now won't help.
    if (m_preload == MediaPlayer::Auto && m_assetIsPlayable) {
        createAVPlayerItem();
        createAVPlayer();
    }

    setDelayCallbacks(false);
}

void MediaPlayerPrivateAVFoundation::setDelayCallbacks(bool delay) const
{
    MutexLocker lock(m_queueMutex);
    if (delay)
        ++m_delayCallbacks;
    else {
        ASSERT(m_delayCallbacks);
        --m_delayCallbacks;
    }
}

void MediaPlayerPrivateAVFoundation::mainThreadCallback(void* context)
{
    LOG(Media, "MediaPlayerPrivateAVFoundation::mainThreadCallback(%p)", context);
    MediaPlayerPrivateAVFoundation* player = static_cast<MediaPlayerPrivateAVFoundation*>(context);
    player->clearMainThreadPendingFlag();
    player->dispatchNotification();
}

void MediaPlayerPrivateAVFoundation::clearMainThreadPendingFlag()
{
    MutexLocker lock(m_queueMutex);
    m_mainThreadCallPending = false;
}

void MediaPlayerPrivateAVFoundation::scheduleMainThreadNotification(Notification::Type type, double time)
{
    scheduleMainThreadNotification(Notification(type, time));
}

void MediaPlayerPrivateAVFoundation::scheduleMainThreadNotification(Notification::Type type, bool finished)
{
    scheduleMainThreadNotification(Notification(type, finished));
}

#if !LOG_DISABLED
static const char* notificationName(MediaPlayerPrivateAVFoundation::Notification& notification)
{
#define DEFINE_TYPE_STRING_CASE(type) case MediaPlayerPrivateAVFoundation::Notification::type: return #type;
    switch (notification.type()) {
        FOR_EACH_MEDIAPLAYERPRIVATEAVFOUNDATION_NOTIFICATION_TYPE(DEFINE_TYPE_STRING_CASE)
        default: return "";
    }
#undef DEFINE_TYPE_STRING_CASE
}
#endif // !LOG_DISABLED
    

void MediaPlayerPrivateAVFoundation::scheduleMainThreadNotification(Notification notification)
{
    LOG(Media, "MediaPlayerPrivateAVFoundation::scheduleMainThreadNotification(%p) - notification %s", this, notificationName(notification));
    m_queueMutex.lock();

    // It is important to always process the properties in the order that we are notified, 
    // so always go through the queue because notifications happen on different threads.
    m_queuedNotifications.append(notification);

    bool delayDispatch = m_delayCallbacks || !isMainThread();
    if (delayDispatch && !m_mainThreadCallPending) {
        m_mainThreadCallPending = true;
        callOnMainThread(mainThreadCallback, this);
    }

    m_queueMutex.unlock();

    if (delayDispatch) {
        LOG(Media, "MediaPlayerPrivateAVFoundation::scheduleMainThreadNotification(%p) - early return", this);
        return;
    }

    dispatchNotification();
}

void MediaPlayerPrivateAVFoundation::dispatchNotification()
{
    ASSERT(isMainThread());

    Notification notification = Notification();
    {
        MutexLocker lock(m_queueMutex);
        
        if (m_queuedNotifications.isEmpty())
            return;
        
        if (!m_delayCallbacks) {
            // Only dispatch one notification callback per invocation because they can cause recursion.
            notification = m_queuedNotifications.first();
            m_queuedNotifications.remove(0);
        }
        
        if (!m_queuedNotifications.isEmpty() && !m_mainThreadCallPending)
            callOnMainThread(mainThreadCallback, this);
        
        if (!notification.isValid())
            return;
    }

    LOG(Media, "MediaPlayerPrivateAVFoundation::dispatchNotification(%p) - dispatching %s", this, notificationName(notification));

    switch (notification.type()) {
    case Notification::ItemDidPlayToEndTime:
        didEnd();
        break;
    case Notification::ItemTracksChanged:
        tracksChanged();
        updateStates();
        break;
    case Notification::ItemStatusChanged:
        updateStates();
        break;
    case Notification::ItemSeekableTimeRangesChanged:
        seekableTimeRangesChanged();
        updateStates();
        break;
    case Notification::ItemLoadedTimeRangesChanged:
        loadedTimeRangesChanged();
        updateStates();
        break;
    case Notification::ItemPresentationSizeChanged:
        sizeChanged();
        updateStates();
        break;
    case Notification::ItemIsPlaybackLikelyToKeepUpChanged:
        updateStates();
        break;
    case Notification::ItemIsPlaybackBufferEmptyChanged:
        updateStates();
        break;
    case Notification::ItemIsPlaybackBufferFullChanged:
        updateStates();
        break;
    case Notification::PlayerRateChanged:
        updateStates();
        rateChanged();
        break;
    case Notification::PlayerTimeChanged:
        timeChanged(notification.time());
        break;
    case Notification::SeekCompleted:
        seekCompleted(notification.finished());
        break;
    case Notification::AssetMetadataLoaded:
        metadataLoaded();
        updateStates();
        break;
    case Notification::AssetPlayabilityKnown:
        updateStates();
        playabilityKnown();
        break;
    case Notification::DurationChanged:
        invalidateCachedDuration();
        break;
    case Notification::ContentsNeedsDisplay:
        contentsNeedsDisplay();
        break;
    case Notification::InbandTracksNeedConfiguration:
        m_inbandTrackConfigurationPending = false;
        configureInbandTracks();
        break;

    case Notification::None:
        ASSERT_NOT_REACHED();
        break;
    }
}

void MediaPlayerPrivateAVFoundation::configureInbandTracks()
{
    RefPtr<InbandTextTrackPrivateAVF> trackToEnable;

    // AVFoundation can only emit cues for one track at a time, so enable the first track that is showing, or the first that
    // is hidden if none are showing. Otherwise disable all tracks.
    for (unsigned i = 0; i < m_textTracks.size(); ++i) {
        RefPtr<InbandTextTrackPrivateAVF> track = m_textTracks[i];
        if (track->mode() == InbandTextTrackPrivate::Showing) {
            trackToEnable = track;
            break;
        }
        if (track->mode() == InbandTextTrackPrivate::Hidden)
            trackToEnable = track;
    }

    setCurrentTrack(trackToEnable.get());
}

void MediaPlayerPrivateAVFoundation::trackModeChanged()
{
    if (m_inbandTrackConfigurationPending)
        return;
    m_inbandTrackConfigurationPending = true;
    scheduleMainThreadNotification(Notification::InbandTracksNeedConfiguration);
}

size_t MediaPlayerPrivateAVFoundation::extraMemoryCost() const
{
    double duration = this->duration();
    if (!duration)
        return 0;

    return totalBytes() * buffered()->totalDuration() / duration;
}

void MediaPlayerPrivateAVFoundation::clearTextTracks()
{
    for (unsigned i = 0; i < m_textTracks.size(); ++i) {
        RefPtr<InbandTextTrackPrivateAVF> track = m_textTracks[i];
        player()->removeTextTrack(track);
        track->disconnect();
    }
    m_textTracks.clear();
}

void MediaPlayerPrivateAVFoundation::processNewAndRemovedTextTracks(const Vector<RefPtr<InbandTextTrackPrivateAVF> >& removedTextTracks)
{
    if (removedTextTracks.size()) {
        for (unsigned i = 0; i < m_textTracks.size(); ++i) {
            if (!removedTextTracks.contains(m_textTracks[i]))
                continue;
            
            player()->removeTextTrack(removedTextTracks[i].get());
            m_textTracks.remove(i);
        }
    }
    
    for (unsigned i = 0; i < m_textTracks.size(); ++i) {
        RefPtr<InbandTextTrackPrivateAVF> track = m_textTracks[i];
        
        track->setTextTrackIndex(i);
        if (track->hasBeenReported())
            continue;
        
        track->setHasBeenReported(true);
        player()->addTextTrack(track.get());
    }
    LOG(Media, "MediaPlayerPrivateAVFoundation::processNewAndRemovedTextTracks(%p) - found %lu text tracks", this, m_textTracks.size());
}

} // namespace WebCore

#endif
