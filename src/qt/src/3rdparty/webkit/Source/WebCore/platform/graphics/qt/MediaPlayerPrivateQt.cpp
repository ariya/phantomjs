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

#include "FrameView.h"
#include "GraphicsContext.h"
#include "HTMLMediaElement.h"
#include "HTMLVideoElement.h"
#include "NetworkingContext.h"
#include "NotImplemented.h"
#include "RenderVideo.h"
#include "TimeRanges.h"
#include "Widget.h"
#include "qwebframe.h"
#include "qwebpage.h"

#include <QGraphicsScene>
#include <QGraphicsVideoItem>
#include <QMediaPlayerControl>
#include <QMediaService>
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QNetworkRequest>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <QStyleOptionGraphicsItem>
#include <QTime>
#include <QTimer>
#include <QUrl>
#include <limits>
#include <wtf/HashSet.h>
#include <wtf/text/CString.h>

#if USE(ACCELERATED_COMPOSITING)
#include "texmap/TextureMapperPlatformLayer.h"
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

MediaPlayer::SupportsType MediaPlayerPrivateQt::supportsType(const String& mime, const String& codec)
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

    if (QMediaPlayer::hasSupport(mime, codecListTrimmed) >= QtMultimediaKit::ProbablySupported)
        return MediaPlayer::IsSupported;

    return MediaPlayer::MayBeSupported;
}

MediaPlayerPrivateQt::MediaPlayerPrivateQt(MediaPlayer* player)
    : m_webCorePlayer(player)
    , m_mediaPlayer(new QMediaPlayer)
    , m_mediaPlayerControl(0)
    , m_videoItem(new QGraphicsVideoItem)
    , m_videoScene(new QGraphicsScene)
    , m_networkState(MediaPlayer::Empty)
    , m_readyState(MediaPlayer::HaveNothing)
    , m_currentSize(0, 0)
    , m_naturalSize(RenderVideo::defaultSize())
    , m_isVisible(false)
    , m_isSeeking(false)
    , m_composited(false)
    , m_preload(MediaPlayer::Auto)
    , m_suppressNextPlaybackChanged(false)
{
    m_mediaPlayer->setVideoOutput(m_videoItem);
    m_videoScene->addItem(m_videoItem);

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
    connect(m_videoScene, SIGNAL(changed(QList<QRectF>)),
            this, SLOT(repaint()));
    connect(m_videoItem, SIGNAL(nativeSizeChanged(QSizeF)),
           this, SLOT(nativeSizeChanged(QSizeF)));

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
    delete m_videoScene;
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

    // Grab the client media element
    HTMLMediaElement* element = static_cast<HTMLMediaElement*>(m_webCorePlayer->mediaPlayerClient());

    // Construct the media content with a network request if the resource is http[s]
    if (scheme == QString::fromLatin1("http") || scheme == QString::fromLatin1("https")) {
        QNetworkRequest request = QNetworkRequest(rUrl);

        // Grab the current document
        Document* document = element->document();
        if (!document)
            document = element->ownerDocument();

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
    m_mediaPlayer->setMuted(element->muted());
    m_mediaPlayer->setVolume(static_cast<int>(element->volume() * 100.0));

    // Don't send PlaybackChanged notification for pre-roll.
    m_suppressNextPlaybackChanged = true;

    // Setting a media source will start loading the media, but we need
    // to pre-roll as well to get video size-hints and buffer-status
    if (element->paused())
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

unsigned MediaPlayerPrivateQt::bytesLoaded() const
{
    QLatin1String bytesLoadedKey("bytes-loaded");
    if (m_mediaPlayer->availableExtendedMetaData().contains(bytesLoadedKey))
        return m_mediaPlayer->extendedMetaData(bytesLoadedKey).toInt();

    return m_mediaPlayer->bufferStatus();
}

unsigned MediaPlayerPrivateQt::totalBytes() const
{
    if (m_mediaPlayer->availableMetaData().contains(QtMultimediaKit::Size))
        return m_mediaPlayer->metaData(QtMultimediaKit::Size).toInt();

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

void MediaPlayerPrivateQt::setVisible(bool visible)
{
    m_isVisible = visible;
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

void MediaPlayerPrivateQt::nativeSizeChanged(const QSizeF& size)
{
    LOG(Media, "MediaPlayerPrivateQt::naturalSizeChanged(%dx%d)",
            size.toSize().width(), size.toSize().height());

    if (!size.isValid())
        return;

    m_naturalSize = size.toSize();
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
    m_videoItem->setSize(QSizeF(QSize(size)));
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
    m_oldNaturalSize = m_naturalSize;
    m_mediaPlayer->setVideoOutput(static_cast<QGraphicsVideoItem*>(0));
    m_videoScene->removeItem(m_videoItem);
}

void MediaPlayerPrivateQt::restoreVideoItem()
{
    m_mediaPlayer->setVideoOutput(m_videoItem);
    m_videoScene->addItem(m_videoItem);
    // FIXME: a qtmobility bug, need to reset the size when restore the videoitem, otherwise the size is 0
    // http://bugreports.qt.nokia.com/browse/QTMOBILITY-971
    nativeSizeChanged(QSize(m_oldNaturalSize));
}

void MediaPlayerPrivateQt::paint(GraphicsContext* context, const IntRect& rect)
{
#if USE(ACCELERATED_COMPOSITING)
    if (m_composited)
        return;
#endif
    if (context->paintingDisabled())
        return;

    if (!m_isVisible)
        return;

    QPainter* painter = context->platformContext();
    m_videoScene->render(painter, QRectF(QRect(rect)), m_videoItem->sceneBoundingRect());
}

void MediaPlayerPrivateQt::paintCurrentFrameInContext(GraphicsContext* context, const IntRect& rect)
{
    if (context->paintingDisabled())
        return;

    if (!m_isVisible)
        return;

    // Grab the painter and widget
    QPainter* painter = context->platformContext();

    // Render the video, using the item as it might not be in the scene
    m_videoItem->paint(painter, 0, 0);
}

void MediaPlayerPrivateQt::repaint()
{
    m_webCorePlayer->repaint();
}

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)

class TextureMapperVideoLayerQt : public virtual TextureMapperMediaLayer {
public:
    TextureMapperVideoLayerQt(QGraphicsVideoItem* videoItem)
        : m_videoItem(videoItem)
    {
    }

    virtual void setPlatformLayerClient(TextureMapperLayerClient* client)
    {
        m_client = client;
    }

    virtual void paint(GraphicsContext* context)
    {
        if (!m_videoItem)
            return;

        QStyleOptionGraphicsItem opt;
        opt.exposedRect = m_videoItem.data()->sceneBoundingRect();
        opt.rect = opt.exposedRect.toRect();
        m_videoItem.data()->paint(context->platformContext(), &opt);
    }

    virtual IntSize size() const
    {
        return m_videoItem ? IntSize(m_videoItem.data()->size().width(), m_videoItem.data()->size().height()) : IntSize();
    }

    QWeakPointer<QGraphicsVideoItem> m_videoItem;
    TextureMapperLayerClient* m_client;
};


void MediaPlayerPrivateQt::acceleratedRenderingStateChanged()
{
    MediaPlayerClient* client = m_webCorePlayer->mediaPlayerClient();
    bool composited = client->mediaPlayerRenderingCanBeAccelerated(m_webCorePlayer);
    if (composited == m_composited)
        return;

    m_composited = composited;
    if (composited)
        m_platformLayer = new TextureMapperVideoLayerQt(m_videoItem);
}

PlatformLayer* MediaPlayerPrivateQt::platformLayer() const
{
    return m_composited ? m_platformLayer.get() : 0;
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
