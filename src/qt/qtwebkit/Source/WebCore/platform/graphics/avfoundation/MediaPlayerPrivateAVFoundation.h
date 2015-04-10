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

#ifndef MediaPlayerPrivateAVFoundation_h
#define MediaPlayerPrivateAVFoundation_h

#if ENABLE(VIDEO) && USE(AVFOUNDATION)

#include "FloatSize.h"
#include "InbandTextTrackPrivateAVF.h"
#include "MediaPlayerPrivate.h"
#include "Timer.h"
#include <wtf/RetainPtr.h>

namespace WebCore {

class InbandTextTrackPrivateAVF;
class GenericCueData;

class MediaPlayerPrivateAVFoundation : public MediaPlayerPrivateInterface, public AVFInbandTrackParent
{
public:

    virtual void repaint();
    virtual void metadataLoaded();
    virtual void playabilityKnown();
    virtual void rateChanged();
    virtual void loadedTimeRangesChanged();
    virtual void seekableTimeRangesChanged();
    virtual void timeChanged(double);
    virtual void seekCompleted(bool);
    virtual void didEnd();
    virtual void contentsNeedsDisplay() { }
    virtual void configureInbandTracks();
    virtual void setCurrentTrack(InbandTextTrackPrivateAVF*) { }
    virtual InbandTextTrackPrivateAVF* currentTrack() const = 0;

    class Notification {
    public:
#define FOR_EACH_MEDIAPLAYERPRIVATEAVFOUNDATION_NOTIFICATION_TYPE(macro) \
    macro(None) \
    macro(ItemDidPlayToEndTime) \
    macro(ItemTracksChanged) \
    macro(ItemStatusChanged) \
    macro(ItemSeekableTimeRangesChanged) \
    macro(ItemLoadedTimeRangesChanged) \
    macro(ItemPresentationSizeChanged) \
    macro(ItemIsPlaybackLikelyToKeepUpChanged) \
    macro(ItemIsPlaybackBufferEmptyChanged) \
    macro(ItemIsPlaybackBufferFullChanged) \
    macro(AssetMetadataLoaded) \
    macro(AssetPlayabilityKnown) \
    macro(PlayerRateChanged) \
    macro(PlayerTimeChanged) \
    macro(SeekCompleted) \
    macro(DurationChanged) \
    macro(ContentsNeedsDisplay) \
    macro(InbandTracksNeedConfiguration) \

        enum Type {
#define DEFINE_TYPE_ENUM(type) type,
            FOR_EACH_MEDIAPLAYERPRIVATEAVFOUNDATION_NOTIFICATION_TYPE(DEFINE_TYPE_ENUM)
#undef DEFINE_TYPE_ENUM
        };
        
        Notification()
            : m_type(None)
            , m_time(0)
            , m_finished(false)
        {
        }

        Notification(Type type, double time)
            : m_type(type)
            , m_time(time)
            , m_finished(false)
        {
        }
        
        Notification(Type type, bool finished)
            : m_type(type)
            , m_time(0)
            , m_finished(finished)
        {
        }
        
        Type type() { return m_type; }
        bool isValid() { return m_type != None; }
        double time() { return m_time; }
        bool finished() { return m_finished; }
        
    private:
        Type m_type;
        double m_time;
        bool m_finished;
    };

    void scheduleMainThreadNotification(Notification);
    void scheduleMainThreadNotification(Notification::Type, double time = 0);
    void scheduleMainThreadNotification(Notification::Type, bool completed);
    void dispatchNotification();
    void clearMainThreadPendingFlag();

protected:
    MediaPlayerPrivateAVFoundation(MediaPlayer*);
    virtual ~MediaPlayerPrivateAVFoundation();

    // MediaPlayerPrivatePrivateInterface overrides.
    virtual void load(const String& url);
    virtual void cancelLoad() = 0;

    virtual void prepareToPlay();
    virtual PlatformMedia platformMedia() const = 0;

    virtual void play();
    virtual void pause();

    virtual IntSize naturalSize() const;
    virtual bool hasVideo() const { return m_cachedHasVideo; }
    virtual bool hasAudio() const { return m_cachedHasAudio; }
    virtual void setVisible(bool);
    virtual float duration() const;
    virtual float currentTime() const = 0;
    virtual void seek(float);
    virtual bool seeking() const;
    virtual void setRate(float);
    virtual bool paused() const;
    virtual void setVolume(float) = 0;
    virtual bool hasClosedCaptions() const { return m_cachedHasCaptions; }
    virtual void setClosedCaptionsVisible(bool) = 0;
    virtual MediaPlayer::NetworkState networkState() const { return m_networkState; }
    virtual MediaPlayer::ReadyState readyState() const { return m_readyState; }
    virtual double maxTimeSeekableDouble() const;
    virtual double minTimeSeekable() const;
    virtual PassRefPtr<TimeRanges> buffered() const;
    virtual bool didLoadingProgress() const;
    virtual void setSize(const IntSize&);
    virtual void paint(GraphicsContext*, const IntRect&) = 0;
    virtual void paintCurrentFrameInContext(GraphicsContext*, const IntRect&) = 0;
    virtual void setPreload(MediaPlayer::Preload);
#if USE(ACCELERATED_COMPOSITING)
    virtual PlatformLayer* platformLayer() const { return 0; }
    virtual bool supportsAcceleratedRendering() const = 0;
    virtual void acceleratedRenderingStateChanged();
#endif
    virtual MediaPlayer::MovieLoadType movieLoadType() const;
    virtual void prepareForRendering();
    virtual float mediaTimeForTimeValue(float) const = 0;

    virtual bool supportsFullscreen() const;
    virtual bool supportsScanning() const { return true; }

    // Required interfaces for concrete derived classes.
    virtual void createAVAssetForURL(const String&) = 0;
    virtual void createAVPlayer() = 0;
    virtual void createAVPlayerItem() = 0;

    enum ItemStatus {
        MediaPlayerAVPlayerItemStatusDoesNotExist,
        MediaPlayerAVPlayerItemStatusUnknown,
        MediaPlayerAVPlayerItemStatusFailed,
        MediaPlayerAVPlayerItemStatusReadyToPlay,
        MediaPlayerAVPlayerItemStatusPlaybackBufferEmpty,
        MediaPlayerAVPlayerItemStatusPlaybackBufferFull,
        MediaPlayerAVPlayerItemStatusPlaybackLikelyToKeepUp,
    };
    virtual ItemStatus playerItemStatus() const = 0;

    enum AssetStatus {
        MediaPlayerAVAssetStatusDoesNotExist,
        MediaPlayerAVAssetStatusUnknown,
        MediaPlayerAVAssetStatusLoading,
        MediaPlayerAVAssetStatusFailed,
        MediaPlayerAVAssetStatusCancelled,
        MediaPlayerAVAssetStatusLoaded,
        MediaPlayerAVAssetStatusPlayable,
    };
    virtual AssetStatus assetStatus() const = 0;

    virtual void platformSetVisible(bool) = 0;
    virtual void platformPlay() = 0;
    virtual void platformPause() = 0;
    virtual void checkPlayability() = 0;
    virtual void updateRate() = 0;
    virtual float rate() const = 0;
    virtual void seekToTime(double time) = 0;
    virtual unsigned totalBytes() const = 0;
    virtual PassRefPtr<TimeRanges> platformBufferedTimeRanges() const = 0;
    virtual double platformMaxTimeSeekable() const = 0;
    virtual double platformMinTimeSeekable() const = 0;
    virtual float platformMaxTimeLoaded() const = 0;
    virtual float platformDuration() const = 0;

    virtual void beginLoadingMetadata() = 0;
    virtual void tracksChanged() = 0;
    virtual void sizeChanged() = 0;

    virtual void createContextVideoRenderer() = 0;
    virtual void destroyContextVideoRenderer() = 0;

    virtual void createVideoLayer() = 0;
    virtual void destroyVideoLayer() = 0;

    virtual bool hasAvailableVideoFrame() const = 0;

    virtual bool hasContextRenderer() const = 0;
    virtual bool hasLayerRenderer() const = 0;

protected:
    void updateStates();

    void setHasVideo(bool);
    void setHasAudio(bool);
    void setHasClosedCaptions(bool);
    void setDelayCallbacks(bool) const;
    void setIgnoreLoadStateChanges(bool delay) { m_ignoreLoadStateChanges = delay; }
    void setNaturalSize(IntSize);
    bool isLiveStream() const { return std::isinf(duration()); }

    enum MediaRenderingMode { MediaRenderingNone, MediaRenderingToContext, MediaRenderingToLayer };
    MediaRenderingMode currentRenderingMode() const;
    MediaRenderingMode preferredRenderingMode() const;

    bool metaDataAvailable() const { return m_readyState >= MediaPlayer::HaveMetadata; }
    float requestedRate() const { return m_requestedRate; }
    float maxTimeLoaded() const;
    bool isReadyForVideoSetup() const;
    virtual void setUpVideoRendering();
    virtual void tearDownVideoRendering();
    bool hasSetUpVideoRendering() const;

    static void mainThreadCallback(void*);
    
    void invalidateCachedDuration();

    const String& assetURL() const { return m_assetURL; }

    MediaPlayer* player() { return m_player; }

    virtual String engineDescription() const { return "AVFoundation"; }

    virtual size_t extraMemoryCost() const OVERRIDE;

    virtual void trackModeChanged() OVERRIDE;
    void processNewAndRemovedTextTracks(const Vector<RefPtr<InbandTextTrackPrivateAVF> >&);
    void clearTextTracks();
    Vector<RefPtr<InbandTextTrackPrivateAVF> > m_textTracks;
    
private:
    MediaPlayer* m_player;

    Vector<Notification> m_queuedNotifications;
    mutable Mutex m_queueMutex;

    mutable RefPtr<TimeRanges> m_cachedLoadedTimeRanges;

    MediaPlayer::NetworkState m_networkState;
    MediaPlayer::ReadyState m_readyState;

    String m_assetURL;
    MediaPlayer::Preload m_preload;

    IntSize m_cachedNaturalSize;
    mutable float m_cachedMaxTimeLoaded;
    mutable double m_cachedMaxTimeSeekable;
    mutable double m_cachedMinTimeSeekable;
    mutable float m_cachedDuration;
    float m_reportedDuration;
    mutable float m_maxTimeLoadedAtLastDidLoadingProgress;
    double m_seekTo;
    float m_requestedRate;
    mutable int m_delayCallbacks;
    bool m_mainThreadCallPending;
    bool m_assetIsPlayable;
    bool m_visible;
    bool m_loadingMetadata;
    bool m_isAllowedToRender;
    bool m_cachedHasAudio;
    bool m_cachedHasVideo;
    bool m_cachedHasCaptions;
    bool m_ignoreLoadStateChanges;
    bool m_haveReportedFirstVideoFrame;
    bool m_playWhenFramesAvailable;
    bool m_inbandTrackConfigurationPending;
    size_t m_seekCount;
};

} // namespace WebCore

#endif // ENABLE(VIDEO) && USE(AVFOUNDATION)

#endif // MediaPlayerPrivateAVFoundation_h
