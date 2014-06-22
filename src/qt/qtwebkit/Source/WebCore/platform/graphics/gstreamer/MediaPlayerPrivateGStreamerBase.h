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

#ifndef MediaPlayerPrivateGStreamerBase_h
#define MediaPlayerPrivateGStreamerBase_h
#if ENABLE(VIDEO) && USE(GSTREAMER)

#include "GRefPtrGStreamer.h"
#include "MediaPlayerPrivate.h"

#include <glib.h>

#include <wtf/Forward.h>

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER_GL) && !USE(COORDINATED_GRAPHICS)
#include "TextureMapperPlatformLayer.h"
#endif

typedef struct _GstBuffer GstBuffer;
typedef struct _GstElement GstElement;
typedef struct _GstMessage GstMessage;
typedef struct _GstStreamVolume GstStreamVolume;
typedef struct _WebKitVideoSink WebKitVideoSink;

namespace WebCore {

class FullscreenVideoControllerGStreamer;
class GraphicsContext;
class IntSize;
class IntRect;
class GStreamerGWorld;

class MediaPlayerPrivateGStreamerBase : public MediaPlayerPrivateInterface
#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER_GL) && !USE(COORDINATED_GRAPHICS)
    , public TextureMapperPlatformLayer
#endif
{

public:
    ~MediaPlayerPrivateGStreamerBase();

    IntSize naturalSize() const;

    void setVolume(float);
    float volume() const;
    void volumeChanged();
    void notifyPlayerOfVolumeChange();

    bool supportsMuting() const { return true; }
    void setMuted(bool);
    bool muted() const;
    void muteChanged();
    void notifyPlayerOfMute();

    MediaPlayer::NetworkState networkState() const;
    MediaPlayer::ReadyState readyState() const;

    void setVisible(bool) { }
    void setSize(const IntSize&);
    void sizeChanged();

    void triggerRepaint(GstBuffer*);
    void paint(GraphicsContext*, const IntRect&);

    virtual bool hasSingleSecurityOrigin() const { return true; }
    virtual float maxTimeLoaded() const { return 0.0; }

#if USE(NATIVE_FULLSCREEN_VIDEO)
    void enterFullscreen();
    void exitFullscreen();
    bool canEnterFullscreen() const { return true; }
#endif

    bool supportsFullscreen() const;
    PlatformMedia platformMedia() const;

    MediaPlayer::MovieLoadType movieLoadType() const;
    virtual bool isLiveStream() const = 0;

    MediaPlayer* mediaPlayer() const { return m_player; }

    unsigned decodedFrameCount() const;
    unsigned droppedFrameCount() const;
    unsigned audioDecodedByteCount() const;
    unsigned videoDecodedByteCount() const;

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER_GL) && !USE(COORDINATED_GRAPHICS)
    virtual PlatformLayer* platformLayer() const { return const_cast<MediaPlayerPrivateGStreamerBase*>(this); }
    virtual bool supportsAcceleratedRendering() const { return true; }
    virtual void paintToTextureMapper(TextureMapper*, const FloatRect&, const TransformationMatrix&, float);
#endif

protected:
    MediaPlayerPrivateGStreamerBase(MediaPlayer*);
    GstElement* createVideoSink(GstElement* pipeline);
    void setStreamVolumeElement(GstStreamVolume*);
    virtual GstElement* audioSink() const { return 0; }
    GRefPtr<GstCaps> currentVideoSinkCaps() const;

    MediaPlayer* m_player;
    GRefPtr<GstStreamVolume> m_volumeElement;
    GRefPtr<GstElement> m_webkitVideoSink;
    GRefPtr<GstElement> m_videoSinkBin;
    GstElement* m_fpsSink;
    MediaPlayer::ReadyState m_readyState;
    MediaPlayer::NetworkState m_networkState;
    IntSize m_size;
    GMutex* m_bufferMutex;
    GstBuffer* m_buffer;
#if USE(NATIVE_FULLSCREEN_VIDEO)
    RefPtr<GStreamerGWorld> m_gstGWorld;
    OwnPtr<FullscreenVideoControllerGStreamer> m_fullscreenVideoController;
#endif
    unsigned long m_volumeTimerHandler;
    unsigned long m_muteTimerHandler;
    unsigned long m_repaintHandler;
    unsigned long m_volumeSignalHandler;
    unsigned long m_muteSignalHandler;
    mutable IntSize m_videoSize;
#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER_GL) && !USE(COORDINATED_GRAPHICS)
    PassRefPtr<BitmapTexture> updateTexture(TextureMapper*);
#endif
};
}

#endif // USE(GSTREAMER)
#endif
