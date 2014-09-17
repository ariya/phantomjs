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

#ifndef MediaPlayerPrivateQuickTimeVisualContext_h
#define MediaPlayerPrivateQuickTimeVisualContext_h

#if ENABLE(VIDEO)

#include "MediaPlayerPrivate.h"
#include "Timer.h"
#include <CoreGraphics/CGAffineTransform.h>
#include <wtf/Forward.h>
#include <wtf/OwnPtr.h>
#include <wtf/RetainPtr.h>

#ifndef DRAW_FRAME_RATE
#define DRAW_FRAME_RATE 0
#endif

typedef struct CGImage *CGImageRef;
class QTMovie;
class QTMovieVisualContext;
class QTDecompressionSession;

namespace WebCore {

class GraphicsContext;
class IntSize;
class IntRect;

#if USE(ACCELERATED_COMPOSITING)
class PlatformCALayer;
class WKCAImageQueue;
#endif

class MediaPlayerPrivateQuickTimeVisualContext : public MediaPlayerPrivateInterface {
public:
    static void registerMediaEngine(MediaEngineRegistrar);

    ~MediaPlayerPrivateQuickTimeVisualContext();

private:
    MediaPlayerPrivateQuickTimeVisualContext(MediaPlayer*);

    virtual bool supportsFullscreen() const;
    virtual PlatformMedia platformMedia() const;
#if USE(ACCELERATED_COMPOSITING)
    virtual PlatformLayer* platformLayer() const;
#endif

    IntSize naturalSize() const;
    bool hasVideo() const;
    bool hasAudio() const;

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
    
    MediaPlayer::NetworkState networkState() const { return m_networkState; }
    MediaPlayer::ReadyState readyState() const { return m_readyState; }
    
    PassRefPtr<TimeRanges> buffered() const;
    float maxTimeSeekable() const;
    unsigned bytesLoaded() const;
    unsigned totalBytes() const;
    
    void setVisible(bool);
    void setSize(const IntSize&);
    
    void loadStateChanged();
    void didEnd();
    
    void paint(GraphicsContext*, const IntRect&);
    void paintCompleted(GraphicsContext&, const IntRect&);

    bool hasSingleSecurityOrigin() const;

    bool hasClosedCaptions() const;
    void setClosedCaptionsVisible(bool);

    void setPreload(MediaPlayer::Preload);

    void updateStates();
    void doSeek();
    void cancelSeek();
    void seekTimerFired(Timer<MediaPlayerPrivateQuickTimeVisualContext>*);
    float maxTimeLoaded() const;
    void sawUnsupportedTracks();

    // engine support
    static PassOwnPtr<MediaPlayerPrivateInterface> create(MediaPlayer*);
    static void getSupportedTypes(HashSet<String>& types);
    static MediaPlayer::SupportsType supportsType(const String& type, const String& codecs);
    static bool isAvailable();

#if USE(ACCELERATED_COMPOSITING)
    virtual bool supportsAcceleratedRendering() const;
    virtual void acceleratedRenderingStateChanged();
#endif

    enum MediaRenderingMode { MediaRenderingNone, MediaRenderingSoftwareRenderer, MediaRenderingMovieLayer };
    MediaRenderingMode currentRenderingMode() const;
    MediaRenderingMode preferredRenderingMode() const;
    bool isReadyForRendering() const;

    void setUpVideoRendering();
    void tearDownVideoRendering();
    bool hasSetUpVideoRendering() const;

    void createLayerForMovie();
    void destroyLayerForMovie();

    void setUpCookiesForQuickTime(const String& url);
    String rfc2616DateStringFromTime(CFAbsoluteTime);

    void visualContextTimerFired(Timer<MediaPlayerPrivateQuickTimeVisualContext>*);
    void retrieveCurrentImage();

    virtual void setPrivateBrowsingMode(bool);

    class MovieClient;
    friend class MovieClient;
    OwnPtr<MovieClient> m_movieClient;

#if USE(ACCELERATED_COMPOSITING)
    class LayerClient;
    friend class LayerClient;
    OwnPtr<LayerClient> m_layerClient;
#endif

    class VisualContextClient;
    friend class VisualContextClient;
    OwnPtr<VisualContextClient> m_visualContextClient;

    void retrieveAndResetMovieTransform();

    virtual float mediaTimeForTimeValue(float) const;

    MediaPlayer* m_player;
    RefPtr<QTMovie> m_movie;
#if USE(ACCELERATED_COMPOSITING)
    RefPtr<PlatformCALayer> m_qtVideoLayer;
    RefPtr<PlatformCALayer> m_transformLayer;
    OwnPtr<WKCAImageQueue> m_imageQueue;
    OwnPtr<QTDecompressionSession> m_decompressionSession;
    CGAffineTransform m_movieTransform; 
#endif
    RefPtr<QTMovieVisualContext> m_visualContext;
    float m_seekTo;
    Timer<MediaPlayerPrivateQuickTimeVisualContext> m_seekTimer;
    Timer<MediaPlayerPrivateQuickTimeVisualContext> m_visualContextTimer;
    IntSize m_size;
    MediaPlayer::NetworkState m_networkState;
    MediaPlayer::ReadyState m_readyState;
    unsigned m_enabledTrackCount;
    unsigned m_totalTrackCount;
    bool m_hasUnsupportedTracks;
    bool m_startedPlaying;
    bool m_isStreaming;
    bool m_visible;
    bool m_newFrameAvailable;
    bool m_delayingLoad;
    String m_movieURL;
    bool m_privateBrowsing;
    MediaPlayer::Preload m_preload;
#if DRAW_FRAME_RATE
    double m_frameCountWhilePlaying;
    double m_timeStartedPlaying;
    double m_timeStoppedPlaying;
#endif

};

}

#endif
#endif
