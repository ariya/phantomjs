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

#include "config.h"
#include "MediaPlayerPrivateQt.h"

#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "GraphicsLayer.h"
#include "HTMLMediaElement.h"
#include "HTMLVideoElement.h"
#include "Logging.h"
#include "NetworkingContext.h"
#include "NotImplemented.h"
#include "RenderVideo.h"
#include "TimeRanges.h"
#include "Widget.h"

#include <QMediaPlayerControl>
#include <QMediaService>
#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkRequest>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <QTime>
#include <QTimer>
#include <QUrl>
#include <limits>
#include <qmediametadata.h>
#include <qmultimedia.h>
#include <wtf/HashSet.h>
#include <wtf/text/CString.h>

#if USE(ACCELERATED_COMPOSITING)
#include "texmap/TextureMapper.h"
#endif

using namespace WTF;

namespace WebCore {

PassOwnPtr<MediaPlayerPrivateInterface> MediaPlayerPrivateQt::create(MediaPlayer* player)
{
    return adoptPtr(new MediaPlayerPrivateQt(player));
}

void MediaPlayerPrivateQt::registerMediaEngine(MediaEngineRegistrar registrar)
{
    registrar(create, getSupportedTypes, supportsType, 0, 0, 0);
}

void MediaPlayerPrivateQt::getSupportedTypes(HashSet<String> &supported)
{
    QStringList types = QMediaPlayer::supportedMimeTypes();

    for (int i = 0; i < types.size(); i++) {
        QString mime = types.at(i);
        if (mime.startsWith(QString::fromLatin1("audio/")) || mime.startsWith(QString::fromLatin1("video/")))
            supported.add(mime);
    }
}

MediaPlayer::SupportsType MediaPlayerPrivateQt::supportsType(const String& mime, const String& codec, const KURL&)
{
    if (!mime.startsWith("audio/") && !mime.startsWith("video/"))
        return MediaPlayer::IsNotSupported;

    // Parse and trim codecs.
    QString codecStr = codec;
    QStringList codecList = codecStr.split(QLatin1Char(','), QString::SkipEmptyParts);
    QStringList codecListTrimmed;
    foreach (const QString& codecStrNotTrimmed, codecList) {
        QString codecStrTrimmed = codecStrNotTrimmed.trimmed();
        if (!codecStrTrimmed.isEmpty())
            codecListTrimmed.append(codecStrTrimmed);
    }

    if (QMediaPlayer::hasSupport(mime, codecListTrimmed) >= QMultimedia::ProbablySupported)
        return MediaPlayer::IsSupported;

    return MediaPlayer::MayBeSupported;
}

MediaPlayerPrivateQt::MediaPlayerPrivateQt(MediaPlayer* player)
    : m_webCorePlayer(player)
    , m_mediaPlayer(new QMediaPlayer)
    , m_mediaPlayerControl(0)
    , m_networkState(MediaPlayer::Empty)
    , m_readyState(MediaPlayer::HaveNothing)
    , m_currentSize(0, 0)
    , m_naturalSize(RenderVideo::defaultSize())
    , m_isSeeking(false)
    , m_composited(false)
    , m_preload(MediaPlayer::Auto)
    , m_bytesLoadedAtLastDidLoadingProgress(0)
    , m_suppressNextPlaybackChanged(false)
{
    m_mediaPlayer->setVideoOutput(this);

    // Signal Handlers
    connect(m_mediaPlayer, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
            this, SLOT(mediaStatusChanged(QMediaPlayer::MediaStatus)));
    connect(m_mediaPlayer, SIGNAL(stateChanged(QMediaPlayer::State)),
            this, SLOT(stateChanged(QMediaPlayer::State)));
    connect(m_mediaPlayer, SIGNAL(error(QMediaPlayer::Error)),
            this, SLOT(handleError(QMediaPlayer::Error)));
    connect(m_mediaPlayer, SIGNAL(bufferStatusChanged(int)),
            this, SLOT(bufferStatusChanged(int)));
    connect(m_mediaPlayer, SIGNAL(durationChanged(qint64)),
            this, SLOT(durationChanged(qint64)));
    connect(m_mediaPlayer, SIGNAL(positionChanged(qint64)),
            this, SLOT(positionChanged(qint64)));
    connect(m_mediaPlayer, SIGNAL(volumeChanged(int)),
            this, SLOT(volumeChanged(int)));
    connect(m_mediaPlayer, SIGNAL(mutedChanged(bool)),
            this, SLOT(mutedChanged(bool)));
    connect(this, SIGNAL(surfaceFormatChanged(const QVideoSurfaceFormat&)),
            this, SLOT(surfaceFormatChanged(const QVideoSurfaceFormat&)));

    // Grab the player control
    if (QMediaService* service = m_mediaPlayer->service()) {
        m_mediaPlayerControl = qobject_cast<QMediaPlayerControl *>(
                service->requestControl(QMediaPlayerControl_iid));
    }
}

MediaPlayerPrivateQt::~MediaPlayerPrivateQt()
{
    m_mediaPlayer->disconnect(this);
    m_mediaPlayer->stop();
    m_mediaPlayer->setMedia(QMediaContent());

    delete m_mediaPlayer;
}

bool MediaPlayerPrivateQt::hasVideo() const
{
    return m_mediaPlayer->isVideoAvailable();
}

bool MediaPlayerPrivateQt::hasAudio() const
{
    return true;
}

void MediaPlayerPrivateQt::load(const String& url)
{
    m_mediaUrl = url;

    // QtMultimedia does not have an API to throttle loading
    // so we handle this ourselves by delaying the load
    if (m_preload == MediaPlayer::None) {
        m_delayingLoad = true;
        return;
    }

    commitLoad(url);
}

void MediaPlayerPrivateQt::commitLoad(const String& url)
{
    // We are now loading
    if (m_networkState != MediaPlayer::Loading) {
        m_networkState = MediaPlayer::Loading;
        m_webCorePlayer->networkStateChanged();
    }

    // And we don't have any data yet
    if (m_readyState != MediaPlayer::HaveNothing) {
        m_readyState = MediaPlayer::HaveNothing;
        m_webCorePlayer->readyStateChanged();
    }

    KURL kUrl(ParsedURLString, url);
    const QUrl rUrl = kUrl;
    const QString scheme = rUrl.scheme().toLower();

    // Construct the media content with a network request if the resource is http[s]
    if (scheme == QString::fromLatin1("http") || scheme == QString::fromLatin1("https")) {
        QNetworkRequest request = QNetworkRequest(rUrl);

        // Grab the current document
        Document* document = m_webCorePlayer->mediaPlayerClient()->mediaPlayerOwningDocument();

        // Grab the frame and network manager
        Frame* frame = document ? document->frame() : 0;
        FrameLoader* frameLoader = frame ? frame->loader() : 0;
        QNetworkAccessManager* manager = frameLoader ? frameLoader->networkingContext()->networkAccessManager() : 0;

        if (manager) {
            // Set the cookies
            QNetworkCookieJar* jar = manager->cookieJar();
            QList<QNetworkCookie> cookies = jar->cookiesForUrl(rUrl);

            // Don't set the header if there are no cookies.
            // This prevents a warning from being emitted.
            if (!cookies.isEmpty())
                request.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(cookies));

            // Set the refferer, but not when requesting insecure content from a secure page
            QUrl documentUrl = QUrl(QString(document->documentURI()));
            if (documentUrl.scheme().toLower() == QString::fromLatin1("http") || scheme == QString::fromLatin1("https"))
                request.setRawHeader("Referer", documentUrl.toEncoded());

            // Set the user agent
            request.setRawHeader("User-Agent", frameLoader->userAgent(rUrl).utf8().data());
        }

        m_mediaPlayer->setMedia(QMediaContent(request));
    } else {
        // Otherwise, just use the URL
        m_mediaPlayer->setMedia(QMediaContent(rUrl));
    }

    // Set the current volume and mute status
    // We get these from the element, rather than the player, in case we have
    // transitioned from a media engine which doesn't support muting, to a media
    // engine which does.
    m_mediaPlayer->setMuted(m_webCorePlayer->muted());
    m_mediaPlayer->setVolume(static_cast<int>(m_webCorePlayer->volume() * 100.0));

    // Don't send PlaybackChanged notification for pre-roll.
    m_suppressNextPlaybackChanged = true;

    // Setting a media source will start loading the media, but we need
    // to pre-roll as well to get video size-hints and buffer-status
    if (m_webCorePlayer->paused())
        m_mediaPlayer->pause();
    else
        m_mediaPlayer->play();
}

void MediaPlayerPrivateQt::resumeLoad()
{
    m_delayingLoad = false;

    if (!m_mediaUrl.isNull())
        commitLoad(m_mediaUrl);
}

void MediaPlayerPrivateQt::cancelLoad()
{
    m_mediaPlayer->setMedia(QMediaContent());
    updateStates();
}

void MediaPlayerPrivateQt::prepareToPlay()
{
    if (m_mediaPlayer->media().isNull() || m_delayingLoad)
        resumeLoad();
}

void MediaPlayerPrivateQt::play()
{
    if (m_mediaPlayer->state() != QMediaPlayer::PlayingState)
        m_mediaPlayer->play();
}

void MediaPlayerPrivateQt::pause()
{
    if (m_mediaPlayer->state() == QMediaPlayer::PlayingState)
        m_mediaPlayer->pause();
}

bool MediaPlayerPrivateQt::paused() const
{
    return (m_mediaPlayer->state() != QMediaPlayer::PlayingState);
}

void MediaPlayerPrivateQt::seek(float position)
{
    if (!m_mediaPlayer->isSeekable())
        return;

    if (m_mediaPlayerControl && !m_mediaPlayerControl->availablePlaybackRanges().contains(position * 1000))
        return;

    m_isSeeking = true;
    m_mediaPlayer->setPosition(static_cast<qint64>(position * 1000));
}

bool MediaPlayerPrivateQt::seeking() const
{
    return m_isSeeking;
}

float MediaPlayerPrivateQt::duration() const
{
    if (m_readyState < MediaPlayer::HaveMetadata)
        return 0.0f;

    float duration = m_mediaPlayer->duration() / 1000.0f;

    // We are streaming
    if (duration <= 0.0f)
        duration = std::numeric_limits<float>::infinity();

    return duration;
}

float MediaPlayerPrivateQt::currentTime() const
{
    return m_mediaPlayer->position() / 1000.0f;
}

PassRefPtr<TimeRanges> MediaPlayerPrivateQt::buffered() const
{
    RefPtr<TimeRanges> buffered = TimeRanges::create();

    if (!m_mediaPlayerControl)
        return buffered;

    QMediaTimeRange playbackRanges = m_mediaPlayerControl->availablePlaybackRanges();

    foreach (const QMediaTimeInterval interval, playbackRanges.intervals()) {
        float rangeMin = static_cast<float>(interval.start()) / 1000.0f;
        float rangeMax = static_cast<float>(interval.end()) / 1000.0f;
        buffered->add(rangeMin, rangeMax);
    }

    return buffered.release();
}

float MediaPlayerPrivateQt::maxTimeSeekable() const
{
    if (!m_mediaPlayerControl)
        return 0;

    return static_cast<float>(m_mediaPlayerControl->availablePlaybackRanges().latestTime()) / 1000.0f;
}

bool MediaPlayerPrivateQt::didLoadingProgress() const
{
    unsigned bytesLoaded = 0;
    QLatin1String bytesLoadedKey("bytes-loaded");
    if (m_mediaPlayer->availableMetaData().contains(bytesLoadedKey))
        bytesLoaded = m_mediaPlayer->metaData(bytesLoadedKey).toInt();
    else
        bytesLoaded = m_mediaPlayer->bufferStatus();
    bool didLoadingProgress = bytesLoaded != m_bytesLoadedAtLastDidLoadingProgress;
    m_bytesLoadedAtLastDidLoadingProgress = bytesLoaded;
    return didLoadingProgress;
}

unsigned MediaPlayerPrivateQt::totalBytes() const
{
    if (m_mediaPlayer->availableMetaData().contains(QMediaMetaData::Size))
        return m_mediaPlayer->metaData(QMediaMetaData::Size).toInt();

    return 100;
}

void MediaPlayerPrivateQt::setPreload(MediaPlayer::Preload preload)
{
    m_preload = preload;
    if (m_delayingLoad && m_preload != MediaPlayer::None)
        resumeLoad();
}

void MediaPlayerPrivateQt::setRate(float rate)
{
    m_mediaPlayer->setPlaybackRate(rate);
}

void MediaPlayerPrivateQt::setVolume(float volume)
{
    m_mediaPlayer->setVolume(static_cast<int>(volume * 100.0));
}

bool MediaPlayerPrivateQt::supportsMuting() const
{
    return true;
}

void MediaPlayerPrivateQt::setMuted(bool muted)
{
    m_mediaPlayer->setMuted(muted);
}

MediaPlayer::NetworkState MediaPlayerPrivateQt::networkState() const
{
    return m_networkState;
}

MediaPlayer::ReadyState MediaPlayerPrivateQt::readyState() const
{
    return m_readyState;
}

void MediaPlayerPrivateQt::setVisible(bool)
{
}

void MediaPlayerPrivateQt::mediaStatusChanged(QMediaPlayer::MediaStatus)
{
    updateStates();
}

void MediaPlayerPrivateQt::handleError(QMediaPlayer::Error)
{
    updateStates();
}

void MediaPlayerPrivateQt::stateChanged(QMediaPlayer::State)
{
    if (!m_suppressNextPlaybackChanged)
        m_webCorePlayer->playbackStateChanged();
    else
        m_suppressNextPlaybackChanged = false;
}

void MediaPlayerPrivateQt::surfaceFormatChanged(const QVideoSurfaceFormat& format)
{
    QSize size = format.sizeHint();
    LOG(Media, "MediaPlayerPrivateQt::naturalSizeChanged(%dx%d)",
            size.width(), size.height());

    if (!size.isValid())
        return;

    IntSize webCoreSize = size;
    if (webCoreSize == m_naturalSize)
        return;

    m_naturalSize = webCoreSize;
    m_webCorePlayer->sizeChanged();
}

void MediaPlayerPrivateQt::positionChanged(qint64)
{
    // Only propagate this event if we are seeking
    if (m_isSeeking) {
        m_isSeeking = false;
        m_webCorePlayer->timeChanged();
    }
}

void MediaPlayerPrivateQt::bufferStatusChanged(int)
{
    notImplemented();
}

void MediaPlayerPrivateQt::durationChanged(qint64)
{
    m_webCorePlayer->durationChanged();
}

void MediaPlayerPrivateQt::volumeChanged(int volume)
{
    m_webCorePlayer->volumeChanged(static_cast<float>(volume) / 100.0);
}

void MediaPlayerPrivateQt::mutedChanged(bool muted)
{
    m_webCorePlayer->muteChanged(muted);
}

void MediaPlayerPrivateQt::updateStates()
{
    // Store the old states so that we can detect a change and raise change events
    MediaPlayer::NetworkState oldNetworkState = m_networkState;
    MediaPlayer::ReadyState oldReadyState = m_readyState;

    QMediaPlayer::MediaStatus currentStatus = m_mediaPlayer->mediaStatus();
    QMediaPlayer::Error currentError = m_mediaPlayer->error();

    if (currentError != QMediaPlayer::NoError) {
        m_readyState = MediaPlayer::HaveNothing;
        if (currentError == QMediaPlayer::FormatError || currentError == QMediaPlayer::ResourceError)
            m_networkState = MediaPlayer::FormatError;
        else
            m_networkState = MediaPlayer::NetworkError;
    } else if (currentStatus == QMediaPlayer::UnknownMediaStatus
               || currentStatus == QMediaPlayer::NoMedia) {
        m_networkState = MediaPlayer::Idle;
        m_readyState = MediaPlayer::HaveNothing;
    } else if (currentStatus == QMediaPlayer::LoadingMedia) {
        m_networkState = MediaPlayer::Loading;
        m_readyState = MediaPlayer::HaveNothing;
    } else if (currentStatus == QMediaPlayer::LoadedMedia) {
        m_networkState = MediaPlayer::Loading;
        m_readyState = MediaPlayer::HaveMetadata;
    } else if (currentStatus == QMediaPlayer::BufferingMedia) {
        m_networkState = MediaPlayer::Loading;
        m_readyState = MediaPlayer::HaveFutureData;
    } else if (currentStatus == QMediaPlayer::StalledMedia) {
        m_networkState = MediaPlayer::Loading;
        m_readyState = MediaPlayer::HaveCurrentData;
    } else if (currentStatus == QMediaPlayer::BufferedMedia
               || currentStatus == QMediaPlayer::EndOfMedia) {
        m_networkState = MediaPlayer::Loaded;
        m_readyState = MediaPlayer::HaveEnoughData;
    } else if (currentStatus == QMediaPlayer::InvalidMedia) {
        m_networkState = MediaPlayer::FormatError;
        m_readyState = MediaPlayer::HaveNothing;
    }

    // Log the state changes and raise the state change events
    // NB: The readyStateChanged event must come before the networkStateChanged event.
    // Breaking this invariant will cause the resource selection algorithm for multiple
    // sources to fail.
    if (m_readyState != oldReadyState)
        m_webCorePlayer->readyStateChanged();

    if (m_networkState != oldNetworkState)
        m_webCorePlayer->networkStateChanged();
}

void MediaPlayerPrivateQt::setSize(const IntSize& size)
{
    LOG(Media, "MediaPlayerPrivateQt::setSize(%dx%d)",
            size.width(), size.height());

    if (size == m_currentSize)
        return;

    m_currentSize = size;
}

IntSize MediaPlayerPrivateQt::naturalSize() const
{
    if (!hasVideo() ||  m_readyState < MediaPlayer::HaveMetadata) {
        LOG(Media, "MediaPlayerPrivateQt::naturalSize() -> 0x0 (!hasVideo || !haveMetaData)");
        return IntSize();
    }

    LOG(Media, "MediaPlayerPrivateQt::naturalSize() -> %dx%d (m_naturalSize)",
            m_naturalSize.width(), m_naturalSize.height());

    return m_naturalSize;
}

void MediaPlayerPrivateQt::removeVideoItem()
{
    m_mediaPlayer->setVideoOutput(static_cast<QAbstractVideoSurface*>(0));
}

void MediaPlayerPrivateQt::restoreVideoItem()
{
    m_mediaPlayer->setVideoOutput(this);
}

// Begin QAbstractVideoSurface implementation.

bool MediaPlayerPrivateQt::start(const QVideoSurfaceFormat& format)
{
    m_currentVideoFrame = QVideoFrame();
    m_frameFormat = format;

    // If the pixel format is not supported by QImage, then we return false here and the QtMultimedia back-end
    // will re-negotiate and call us again with a better format.
    if (QVideoFrame::imageFormatFromPixelFormat(m_frameFormat.pixelFormat()) == QImage::Format_Invalid)
        return false;

    return QAbstractVideoSurface::start(format);
}

QList<QVideoFrame::PixelFormat> MediaPlayerPrivateQt::supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const
{
    QList<QVideoFrame::PixelFormat> formats;
    switch (handleType) {
    case QAbstractVideoBuffer::QPixmapHandle:
    case QAbstractVideoBuffer::NoHandle:
        formats << QVideoFrame::Format_RGB32 << QVideoFrame::Format_ARGB32 << QVideoFrame::Format_RGB565;
        break;
    default: break;
    }
    return formats;
}

bool MediaPlayerPrivateQt::present(const QVideoFrame& frame)
{
    m_currentVideoFrame = frame;
    m_webCorePlayer->repaint();
    return true;
}

// End QAbstractVideoSurface implementation.

void MediaPlayerPrivateQt::paint(GraphicsContext* context, const IntRect& rect)
{
#if USE(ACCELERATED_COMPOSITING)
    if (m_composited)
        return;
#endif
    paintCurrentFrameInContext(context, rect);
}

void MediaPlayerPrivateQt::paintCurrentFrameInContext(GraphicsContext* context, const IntRect& rect)
{
    if (context->paintingDisabled())
        return;

    if (!m_currentVideoFrame.isValid())
        return;

    QPainter* painter = context->platformContext();

    if (m_currentVideoFrame.handleType() == QAbstractVideoBuffer::QPixmapHandle) {
        painter->drawPixmap(rect, m_currentVideoFrame.handle().value<QPixmap>());
    } else if (m_currentVideoFrame.map(QAbstractVideoBuffer::ReadOnly)) {
        QImage image(m_currentVideoFrame.bits(),
                     m_frameFormat.frameSize().width(),
                     m_frameFormat.frameSize().height(),
                     m_currentVideoFrame.bytesPerLine(),
                     QVideoFrame::imageFormatFromPixelFormat(m_frameFormat.pixelFormat()));
        const QRect target = rect;

        if (m_frameFormat.scanLineDirection() == QVideoSurfaceFormat::BottomToTop) {
            const QTransform oldTransform = painter->transform();
            painter->scale(1, -1);
            painter->translate(0, -target.bottom());
            painter->drawImage(QRect(target.x(), 0, target.width(), target.height()), image);
            painter->setTransform(oldTransform);
        } else {
            painter->drawImage(target, image);
        }

        m_currentVideoFrame.unmap();
    }
}

#if USE(ACCELERATED_COMPOSITING)
void MediaPlayerPrivateQt::paintToTextureMapper(TextureMapper* textureMapper, const FloatRect& targetRect, const TransformationMatrix& matrix, float opacity)
{
}
#endif

PlatformMedia MediaPlayerPrivateQt::platformMedia() const
{
    PlatformMedia pm;
    pm.type = PlatformMedia::QtMediaPlayerType;
    pm.media.qtMediaPlayer = const_cast<MediaPlayerPrivateQt*>(this);
    return pm;
}

} // namespace WebCore

#include "moc_MediaPlayerPrivateQt.cpp"
