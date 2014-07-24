/*
 * Copyright (C) 2007, 2009 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Collabora Ltd. All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2009, 2010 Igalia S.L
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * aint with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef MediaPlayerPrivateGStreamer_h
#define MediaPlayerPrivateGStreamer_h
#if ENABLE(VIDEO) && USE(GSTREAMER)

#include "GRefPtrGStreamer.h"
#include "MediaPlayerPrivateGStreamerBase.h"
#include "Timer.h"

#include <glib.h>
#include <gst/gst.h>
#include <gst/pbutils/install-plugins.h>
#include <wtf/Forward.h>

typedef struct _GstBuffer GstBuffer;
typedef struct _GstMessage GstMessage;
typedef struct _GstElement GstElement;

namespace WebCore {

class MediaPlayerPrivateGStreamer : public MediaPlayerPrivateGStreamerBase {
public:
    ~MediaPlayerPrivateGStreamer();
    static void registerMediaEngine(MediaEngineRegistrar);
    gboolean handleMessage(GstMessage*);
    void handlePluginInstallerResult(GstInstallPluginsReturn);

    bool hasVideo() const { return m_hasVideo; }
    bool hasAudio() const { return m_hasAudio; }

    void load(const String &url);
#if ENABLE(MEDIA_SOURCE)
    void load(const String& url, PassRefPtr<MediaSource>);
#endif
    void commitLoad();
    void cancelLoad();

    void prepareToPlay();
    void play();
    void pause();

    bool paused() const;
    bool seeking() const;

    float duration() const;
    float currentTime() const;
    void seek(float);

    void setRate(float);
    void setPreservesPitch(bool);

    void setPreload(MediaPlayer::Preload);
    void fillTimerFired(Timer<MediaPlayerPrivateGStreamer>*);

    PassRefPtr<TimeRanges> buffered() const;
    float maxTimeSeekable() const;
    bool didLoadingProgress() const;
    unsigned totalBytes() const;
    float maxTimeLoaded() const;

    void loadStateChanged();
    void timeChanged();
    void didEnd();
    void durationChanged();
    void loadingFailed(MediaPlayer::NetworkState);

    void videoChanged();
    void audioChanged();
    void notifyPlayerOfVideo();
    void notifyPlayerOfAudio();

    void sourceChanged();
    GstElement* audioSink() const;

    void setAudioStreamProperties(GObject*);

    void simulateAudioInterruption();

private:
    MediaPlayerPrivateGStreamer(MediaPlayer*);

    static PassOwnPtr<MediaPlayerPrivateInterface> create(MediaPlayer*);

    static void getSupportedTypes(HashSet<String>&);
    static MediaPlayer::SupportsType supportsType(const String& type, const String& codecs, const KURL&);

    static bool isAvailable();

    void updateAudioSink();
    void createAudioSink();

    float playbackPosition() const;

    void cacheDuration();
    void updateStates();
    void asyncStateChangeDone();

    void createGSTPlayBin();
    bool changePipelineState(GstState);

    bool loadNextLocation();
    void mediaLocationChanged(GstMessage*);

    void setDownloadBuffering();
    void processBufferingStats(GstMessage*);

    virtual String engineDescription() const { return "GStreamer"; }
    virtual bool isLiveStream() const { return m_isStreaming; }

private:
    GRefPtr<GstElement> m_playBin;
    GRefPtr<GstElement> m_source;
    float m_seekTime;
    bool m_changingRate;
    float m_endTime;
    bool m_isEndReached;
    mutable bool m_isStreaming;
    GstStructure* m_mediaLocations;
    int m_mediaLocationCurrentIndex;
    bool m_resetPipeline;
    bool m_paused;
    bool m_seeking;
    bool m_seekIsPending;
    float m_timeOfOverlappingSeek;
    bool m_canFallBackToLastFinishedSeekPositon;
    bool m_buffering;
    float m_playbackRate;
    bool m_errorOccured;
    mutable gfloat m_mediaDuration;
    bool m_downloadFinished;
    Timer<MediaPlayerPrivateGStreamer> m_fillTimer;
    float m_maxTimeLoaded;
    int m_bufferingPercentage;
    MediaPlayer::Preload m_preload;
    bool m_delayingLoad;
    bool m_mediaDurationKnown;
    mutable float m_maxTimeLoadedAtLastDidLoadingProgress;
    bool m_volumeAndMuteInitialized;
    bool m_hasVideo;
    bool m_hasAudio;
    guint m_audioTimerHandler;
    guint m_videoTimerHandler;
    GRefPtr<GstElement> m_webkitAudioSink;
    mutable long m_totalBytes;
    KURL m_url;
    bool m_preservesPitch;
    GstState m_requestedState;
    GRefPtr<GstElement> m_autoAudioSink;
    bool m_missingPlugins;
};
}

#endif // USE(GSTREAMER)
#endif
