/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef MediaPlayerPrivateQt_h
#define MediaPlayerPrivateQt_h

#include "MediaPlayerPrivate.h"

#include <QMediaPlayer>
#include <QObject>

QT_BEGIN_NAMESPACE
class QMediaPlayerControl;
class QGraphicsVideoItem;
class QGraphicsScene;
QT_END_NAMESPACE

namespace WebCore {

class TextureMapperMediaLayer;

class MediaPlayerPrivateQt : public QObject, public MediaPlayerPrivateInterface {

    Q_OBJECT

public:
    static PassOwnPtr<MediaPlayerPrivateInterface> create(MediaPlayer*);
    ~MediaPlayerPrivateQt();

    static void registerMediaEngine(MediaEngineRegistrar);
    static void getSupportedTypes(HashSet<String>&);
    static MediaPlayer::SupportsType supportsType(const String&, const String&);
    static bool isAvailable() { return true; }

    bool hasVideo() const;
    bool hasAudio() const;

    void load(const String &url);
    void commitLoad(const String& url);
    void resumeLoad();
    void cancelLoad();

    void play();
    void pause();
    void prepareToPlay();

    bool paused() const;
    bool seeking() const;

    float duration() const;
    float currentTime() const;
    void seek(float);

    void setRate(float);
    void setVolume(float);

    bool supportsMuting() const;
    void setMuted(bool);

    void setPreload(MediaPlayer::Preload);

    MediaPlayer::NetworkState networkState() const;
    MediaPlayer::ReadyState readyState() const;

    PassRefPtr<TimeRanges> buffered() const;
    float maxTimeSeekable() const;
    unsigned bytesLoaded() const;
    unsigned totalBytes() const;

    void setVisible(bool);

    IntSize naturalSize() const;
    void setSize(const IntSize&);

    void paint(GraphicsContext*, const IntRect&);
    // reimplemented for canvas drawImage(HTMLVideoElement)
    void paintCurrentFrameInContext(GraphicsContext*, const IntRect&);

    bool supportsFullscreen() const { return true; }

#if USE(ACCELERATED_COMPOSITING)
#if USE(TEXTURE_MAPPER)
    // whether accelerated rendering is supported by the media engine for the current media.
    virtual bool supportsAcceleratedRendering() const { return true; }
    // called when the rendering system flips the into or out of accelerated rendering mode.
    virtual void acceleratedRenderingStateChanged();
    // returns an object that can be directly composited via GraphicsLayerQt (essentially a QGraphicsItem*)
    virtual PlatformLayer* platformLayer() const;
#else
    virtual bool supportsAcceleratedRendering() const { return false; }
    virtual void acceleratedRenderingStateChanged() { }
    virtual PlatformLayer* platformLayer() const { return 0; }
#endif
#endif

    virtual PlatformMedia platformMedia() const;

    QMediaPlayer* mediaPlayer() const { return m_mediaPlayer; }
    void removeVideoItem();
    void restoreVideoItem();

private slots:
    void mediaStatusChanged(QMediaPlayer::MediaStatus);
    void handleError(QMediaPlayer::Error);
    void stateChanged(QMediaPlayer::State);
    void nativeSizeChanged(const QSizeF&);
    void positionChanged(qint64);
    void durationChanged(qint64);
    void bufferStatusChanged(int);
    void volumeChanged(int);
    void mutedChanged(bool);
    void repaint();

private:
    void updateStates();

private:
    MediaPlayerPrivateQt(MediaPlayer*);

    MediaPlayer* m_webCorePlayer;
    QMediaPlayer* m_mediaPlayer;
    QMediaPlayerControl* m_mediaPlayerControl;
    QGraphicsVideoItem* m_videoItem;
    QGraphicsScene* m_videoScene;
#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)
    OwnPtr<TextureMapperMediaLayer> m_platformLayer;
#endif

    mutable MediaPlayer::NetworkState m_networkState;
    mutable MediaPlayer::ReadyState m_readyState;

    IntSize m_currentSize;
    IntSize m_naturalSize;
    IntSize m_oldNaturalSize;
    bool m_isVisible;
    bool m_isSeeking;
    bool m_composited;
    MediaPlayer::Preload m_preload;
    bool m_delayingLoad;
    String m_mediaUrl;
    bool m_suppressNextPlaybackChanged;

};
}

#endif // MediaPlayerPrivateQt_h
