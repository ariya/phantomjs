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

#ifndef MediaPlayerPrivateQTKit_h
#define MediaPlayerPrivateQTKit_h

#if ENABLE(VIDEO)

#include "MediaPlayerPrivate.h"
#include "Timer.h"
#include "FloatSize.h"
#include <wtf/RetainPtr.h>

#ifdef __OBJC__
#import <QTKit/QTTime.h>
#else
class QTTime;
#endif

OBJC_CLASS NSDictionary;
OBJC_CLASS NSMutableDictionary;
OBJC_CLASS QTMovie;
OBJC_CLASS QTMovieView;
OBJC_CLASS QTMovieLayer;
OBJC_CLASS QTVideoRendererWebKitOnly;
OBJC_CLASS WebCoreMovieObserver;

#ifndef DRAW_FRAME_RATE
#define DRAW_FRAME_RATE 0
#endif

namespace WebCore {
    
class MediaPlayerPrivateQTKit : public MediaPlayerPrivateInterface {
public:
    ~MediaPlayerPrivateQTKit();
    static void registerMediaEngine(MediaEngineRegistrar);

    void repaint();
    void loadStateChanged();
    void loadedRangesChanged();
    void rateChanged();
    void sizeChanged();
    void timeChanged();
    void didEnd();
#if USE(ACCELERATED_COMPOSITING)
    void layerHostChanged(PlatformLayer* rootLayer);
#endif

private:
    MediaPlayerPrivateQTKit(MediaPlayer*);

    // engine support
    static PassOwnPtr<MediaPlayerPrivateInterface> create(MediaPlayer*);
    static void getSupportedTypes(HashSet<String>& types);
    static MediaPlayer::SupportsType supportsType(const String& type, const String& codecs, const KURL&);
#if ENABLE(ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA_V2)
    static MediaPlayer::SupportsType extendedSupportsType(const String& type, const String& codecs, const String& keySystem, const KURL&);
#endif

    static void getSitesInMediaCache(Vector<String>&);
    static void clearMediaCache();
    static void clearMediaCacheForSite(const String&);
    static bool isAvailable();

    PlatformMedia platformMedia() const;
#if USE(ACCELERATED_COMPOSITING)
    PlatformLayer* platformLayer() const;
#endif

    IntSize naturalSize() const;
    bool hasVideo() const;
    bool hasAudio() const;
    bool supportsFullscreen() const;
    virtual bool supportsScanning() const { return true; }
    
    void load(const String& url);
    void cancelLoad();
    void loadInternal(const String& url);
    void resumeLoad();
    
    void play();
    void pause();    
    void prepareToPlay();
    
    bool paused() const;
    bool seeking() const;
    
    float duration() const;
    float currentTime() const;
    void seek(float time);
    
    void setRate(float);
    void setVolume(float);
    void setPreservesPitch(bool);

    bool hasClosedCaptions() const;
    void setClosedCaptionsVisible(bool);

    void setPreload(MediaPlayer::Preload);

    MediaPlayer::NetworkState networkState() const { return m_networkState; }
    MediaPlayer::ReadyState readyState() const { return m_readyState; }
    
    PassRefPtr<TimeRanges> buffered() const;
    float maxTimeSeekable() const;
    bool didLoadingProgress() const;
    unsigned totalBytes() const;
    
    void setVisible(bool);
    void setSize(const IntSize&);
    
    virtual bool hasAvailableVideoFrame() const;

    void paint(GraphicsContext*, const IntRect&);
    void paintCurrentFrameInContext(GraphicsContext*, const IntRect&);
    virtual void prepareForRendering();


#if USE(ACCELERATED_COMPOSITING)
    bool supportsAcceleratedRendering() const;
    void acceleratedRenderingStateChanged();
#endif

    bool hasSingleSecurityOrigin() const;
    MediaPlayer::MovieLoadType movieLoadType() const;

    void createQTMovie(const String& url);
    void createQTMovie(NSURL *, NSDictionary *movieAttributes);

    enum MediaRenderingMode { MediaRenderingNone, MediaRenderingMovieView, MediaRenderingSoftwareRenderer, MediaRenderingMovieLayer };
    MediaRenderingMode currentRenderingMode() const;
    MediaRenderingMode preferredRenderingMode() const;
    
    void setUpVideoRendering();
    void tearDownVideoRendering();
    bool hasSetUpVideoRendering() const;
    
    void createQTMovieView();
    void detachQTMovieView();
    
    enum QTVideoRendererMode { QTVideoRendererModeDefault, QTVideoRendererModeListensForNewImages };
    void createQTVideoRenderer(QTVideoRendererMode rendererMode);
    void destroyQTVideoRenderer();
    
    void createQTMovieLayer();
    void destroyQTMovieLayer();

    QTTime createQTTime(float time) const;
    
    void updateStates();
    void doSeek();
    void cancelSeek();
    void seekTimerFired(Timer<MediaPlayerPrivateQTKit>*);
    float maxTimeLoaded() const;
    void disableUnsupportedTracks();
    
    void sawUnsupportedTracks();
    void cacheMovieScale();
    bool metaDataAvailable() const { return m_qtMovie && m_readyState >= MediaPlayer::HaveMetadata; }

    bool isReadyForVideoSetup() const;
    
    virtual float mediaTimeForTimeValue(float) const;

    virtual double maximumDurationToCacheMediaTime() const { return 5; }

    virtual void setPrivateBrowsingMode(bool);
    
    NSMutableDictionary* commonMovieAttributes();

    virtual String engineDescription() const { return "QTKit"; }

    MediaPlayer* m_player;
    RetainPtr<QTMovie> m_qtMovie;
    RetainPtr<QTMovieView> m_qtMovieView;
    RetainPtr<QTVideoRendererWebKitOnly> m_qtVideoRenderer;
    RetainPtr<WebCoreMovieObserver> m_objcObserver;
    String m_movieURL;
    float m_seekTo;
    Timer<MediaPlayerPrivateQTKit> m_seekTimer;
    MediaPlayer::NetworkState m_networkState;
    MediaPlayer::ReadyState m_readyState;
    IntRect m_rect;
    FloatSize m_scaleFactor;
    unsigned m_enabledTrackCount;
    unsigned m_totalTrackCount;
    float m_reportedDuration;
    float m_cachedDuration;
    float m_timeToRestore;
    RetainPtr<QTMovieLayer> m_qtVideoLayer;
    MediaPlayer::Preload m_preload;
    bool m_startedPlaying;
    bool m_isStreaming;
    bool m_visible;
    bool m_hasUnsupportedTracks;
    bool m_videoFrameHasDrawn;
    bool m_isAllowedToRender;
    bool m_privateBrowsing;
    mutable float m_maxTimeLoadedAtLastDidLoadingProgress;
#if DRAW_FRAME_RATE
    int  m_frameCountWhilePlaying;
    double m_timeStartedPlaying;
    double m_timeStoppedPlaying;
#endif
    mutable FloatSize m_cachedNaturalSize;
};

}

#endif
#endif
