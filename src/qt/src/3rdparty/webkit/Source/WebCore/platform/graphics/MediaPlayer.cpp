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

#include "config.h"

#if ENABLE(VIDEO)
#include "MediaPlayer.h"

#include "ContentType.h"
#include "Document.h"
#include "Frame.h"
#include "FrameView.h"
#include "IntRect.h"
#include "MIMETypeRegistry.h"
#include "MediaPlayerPrivate.h"
#include "Settings.h"
#include "TimeRanges.h"

#if PLATFORM(QT)
#include <QtGlobal>
#endif

#if USE(GSTREAMER)
#include "MediaPlayerPrivateGStreamer.h"
#endif

#if PLATFORM(MAC) || (PLATFORM(QT) && USE(QTKIT))
#include "MediaPlayerPrivateQTKit.h"
#if USE(AVFOUNDATION)
#include "MediaPlayerPrivateAVFoundationObjC.h"
#endif
#define PlatformMediaEngineClassName MediaPlayerPrivateQTKit
#elif OS(WINCE) && !PLATFORM(QT)
#include "MediaPlayerPrivateWinCE.h"
#define PlatformMediaEngineClassName MediaPlayerPrivate
#elif PLATFORM(WIN)
#include "MediaPlayerPrivateQuickTimeVisualContext.h"
#define PlatformMediaEngineClassName MediaPlayerPrivateQuickTimeVisualContext
#elif PLATFORM(QT)
#if USE(QT_MULTIMEDIA) && !USE(GSTREAMER)
#include "MediaPlayerPrivateQt.h"
#define PlatformMediaEngineClassName MediaPlayerPrivateQt
#endif
#elif PLATFORM(CHROMIUM)
#include "MediaPlayerPrivateChromium.h"
#define PlatformMediaEngineClassName MediaPlayerPrivate
#endif

namespace WebCore {

const PlatformMedia NoPlatformMedia = { PlatformMedia::None, {0} };

// a null player to make MediaPlayer logic simpler

class NullMediaPlayerPrivate : public MediaPlayerPrivateInterface {
public:
    NullMediaPlayerPrivate(MediaPlayer*) { }

    virtual void load(const String&) { }
    virtual void cancelLoad() { }

    virtual void prepareToPlay() { }
    virtual void play() { }
    virtual void pause() { }    

    virtual PlatformMedia platformMedia() const { return NoPlatformMedia; }
#if USE(ACCELERATED_COMPOSITING)
    virtual PlatformLayer* platformLayer() const { return 0; }
#endif

    virtual IntSize naturalSize() const { return IntSize(0, 0); }

    virtual bool hasVideo() const { return false; }
    virtual bool hasAudio() const { return false; }

    virtual void setVisible(bool) { }

    virtual float duration() const { return 0; }

    virtual float currentTime() const { return 0; }
    virtual void seek(float) { }
    virtual bool seeking() const { return false; }

    virtual void setRate(float) { }
    virtual void setPreservesPitch(bool) { }
    virtual bool paused() const { return false; }

    virtual void setVolume(float) { }

    virtual bool supportsMuting() const { return false; }
    virtual void setMuted(bool) { }

    virtual bool hasClosedCaptions() const { return false; }
    virtual void setClosedCaptionsVisible(bool) { };

    virtual MediaPlayer::NetworkState networkState() const { return MediaPlayer::Empty; }
    virtual MediaPlayer::ReadyState readyState() const { return MediaPlayer::HaveNothing; }

    virtual float maxTimeSeekable() const { return 0; }
    virtual PassRefPtr<TimeRanges> buffered() const { return TimeRanges::create(); }

    virtual unsigned totalBytes() const { return 0; }
    virtual unsigned bytesLoaded() const { return 0; }

    virtual void setSize(const IntSize&) { }

    virtual void paint(GraphicsContext*, const IntRect&) { }

    virtual bool canLoadPoster() const { return false; }
    virtual void setPoster(const String&) { }

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    virtual void deliverNotification(MediaPlayerProxyNotificationType) { }
    virtual void setMediaPlayerProxy(WebMediaPlayerProxy*) { }
    virtual void setControls(bool) { }
#endif

    virtual bool hasSingleSecurityOrigin() const { return true; }
};

static PassOwnPtr<MediaPlayerPrivateInterface> createNullMediaPlayer(MediaPlayer* player) 
{ 
    return adoptPtr(new NullMediaPlayerPrivate(player)); 
}


// engine support

struct MediaPlayerFactory {
    WTF_MAKE_NONCOPYABLE(MediaPlayerFactory); WTF_MAKE_FAST_ALLOCATED;
public:
    MediaPlayerFactory(CreateMediaEnginePlayer constructor, MediaEngineSupportedTypes getSupportedTypes, MediaEngineSupportsType supportsTypeAndCodecs,
        MediaEngineGetSitesInMediaCache getSitesInMediaCache, MediaEngineClearMediaCache clearMediaCache, MediaEngineClearMediaCacheForSite clearMediaCacheForSite) 
        : constructor(constructor)
        , getSupportedTypes(getSupportedTypes)
        , supportsTypeAndCodecs(supportsTypeAndCodecs)
        , getSitesInMediaCache(getSitesInMediaCache)
        , clearMediaCache(clearMediaCache)
        , clearMediaCacheForSite(clearMediaCacheForSite)

    { 
    }

    CreateMediaEnginePlayer constructor;
    MediaEngineSupportedTypes getSupportedTypes;
    MediaEngineSupportsType supportsTypeAndCodecs;
    MediaEngineGetSitesInMediaCache getSitesInMediaCache;
    MediaEngineClearMediaCache clearMediaCache;
    MediaEngineClearMediaCacheForSite clearMediaCacheForSite;
};

static void addMediaEngine(CreateMediaEnginePlayer, MediaEngineSupportedTypes, MediaEngineSupportsType, MediaEngineGetSitesInMediaCache, MediaEngineClearMediaCache, MediaEngineClearMediaCacheForSite);
static MediaPlayerFactory* bestMediaEngineForTypeAndCodecs(const String& type, const String& codecs, MediaPlayerFactory* current = 0);
static MediaPlayerFactory* nextMediaEngine(MediaPlayerFactory* current);

static Vector<MediaPlayerFactory*>& installedMediaEngines() 
{
    DEFINE_STATIC_LOCAL(Vector<MediaPlayerFactory*>, installedEngines, ());
    static bool enginesQueried = false;

    if (!enginesQueried) {
        enginesQueried = true;

#if USE(GSTREAMER)
        MediaPlayerPrivateGStreamer::registerMediaEngine(addMediaEngine);
#endif

#if USE(AVFOUNDATION) && PLATFORM(MAC)
        if (Settings::isAVFoundationEnabled())
            MediaPlayerPrivateAVFoundationObjC::registerMediaEngine(addMediaEngine);
#endif

#if !PLATFORM(GTK) && !PLATFORM(EFL) && !(PLATFORM(QT) && USE(GSTREAMER))
        PlatformMediaEngineClassName::registerMediaEngine(addMediaEngine);
#endif
    }

    return installedEngines;
}

static void addMediaEngine(CreateMediaEnginePlayer constructor, MediaEngineSupportedTypes getSupportedTypes, MediaEngineSupportsType supportsType,
    MediaEngineGetSitesInMediaCache getSitesInMediaCache, MediaEngineClearMediaCache clearMediaCache, MediaEngineClearMediaCacheForSite clearMediaCacheForSite)
{
    ASSERT(constructor);
    ASSERT(getSupportedTypes);
    ASSERT(supportsType);

    installedMediaEngines().append(new MediaPlayerFactory(constructor, getSupportedTypes, supportsType, getSitesInMediaCache, clearMediaCache, clearMediaCacheForSite));
}

static const AtomicString& applicationOctetStream()
{
    DEFINE_STATIC_LOCAL(const AtomicString, applicationOctetStream, ("application/octet-stream"));
    return applicationOctetStream;
}

static const AtomicString& textPlain()
{
    DEFINE_STATIC_LOCAL(const AtomicString, textPlain, ("text/plain"));
    return textPlain;
}

static const AtomicString& codecs()
{
    DEFINE_STATIC_LOCAL(const AtomicString, codecs, ("codecs"));
    return codecs;
}

static MediaPlayerFactory* bestMediaEngineForTypeAndCodecs(const String& type, const String& codecs, MediaPlayerFactory* current)
{
    if (type.isEmpty())
        return 0;

    Vector<MediaPlayerFactory*>& engines = installedMediaEngines();
    if (engines.isEmpty())
        return 0;

    // 4.8.10.3 MIME types - In the absence of a specification to the contrary, the MIME type "application/octet-stream" 
    // when used with parameters, e.g. "application/octet-stream;codecs=theora", is a type that the user agent knows 
    // it cannot render.
    if (type == applicationOctetStream()) {
        if (!codecs.isEmpty())
            return 0;
    }

    MediaPlayerFactory* engine = 0;
    MediaPlayer::SupportsType supported = MediaPlayer::IsNotSupported;
    unsigned count = engines.size();
    for (unsigned ndx = 0; ndx < count; ndx++) {
        if (current) {
            if (current == engines[ndx])
                current = 0;
            continue;
        }
        MediaPlayer::SupportsType engineSupport = engines[ndx]->supportsTypeAndCodecs(type, codecs);
        if (engineSupport > supported) {
            supported = engineSupport;
            engine = engines[ndx];
        }
    }

    return engine;
}

static MediaPlayerFactory* nextMediaEngine(MediaPlayerFactory* current)
{
    Vector<MediaPlayerFactory*>& engines = installedMediaEngines();
    if (engines.isEmpty())
        return 0;

    if (!current) 
        return engines.first();

    size_t currentIndex = engines.find(current);
    if (currentIndex == WTF::notFound || currentIndex + 1 >= engines.size()) 
        return 0;

    return engines[currentIndex + 1];
}

// media player

MediaPlayer::MediaPlayer(MediaPlayerClient* client)
    : m_mediaPlayerClient(client)
    , m_reloadTimer(this, &MediaPlayer::reloadTimerFired)
    , m_private(createNullMediaPlayer(this))
    , m_currentMediaEngine(0)
    , m_frameView(0)
    , m_preload(Auto)
    , m_visible(false)
    , m_rate(1.0f)
    , m_volume(1.0f)
    , m_muted(false)
    , m_preservesPitch(true)
    , m_privateBrowsing(false)
    , m_shouldPrepareToRender(false)
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    , m_playerProxy(0)
#endif
{
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    Vector<MediaPlayerFactory*>& engines = installedMediaEngines();
    if (!engines.isEmpty()) {
        m_currentMediaEngine = engines[0];
        m_private = engines[0]->constructor(this);
        if (m_mediaPlayerClient)
            m_mediaPlayerClient->mediaPlayerEngineUpdated(this);
    }
#endif
}

MediaPlayer::~MediaPlayer()
{
    m_mediaPlayerClient = 0;
}

void MediaPlayer::load(const String& url, const ContentType& contentType)
{
    String type = contentType.type().lower();
    String typeCodecs = contentType.parameter(codecs());

    // If the MIME type is missing or is not meaningful, try to figure it out from the URL.
    if (type.isEmpty() || type == applicationOctetStream() || type == textPlain()) {
        if (protocolIs(url, "data"))
            type = mimeTypeFromDataURL(url);
        else {
            size_t pos = url.reverseFind('.');
            if (pos != notFound) {
                String extension = url.substring(pos + 1);
                String mediaType = MIMETypeRegistry::getMediaMIMETypeForExtension(extension);
                if (!mediaType.isEmpty())
                    type = mediaType;
            }
        }
    }

    m_url = url;
    m_contentMIMEType = type;
    m_contentTypeCodecs = typeCodecs;
    loadWithNextMediaEngine(0);
}

void MediaPlayer::loadWithNextMediaEngine(MediaPlayerFactory* current)
{
    MediaPlayerFactory* engine;

    // If no MIME type is specified, just use the next engine.
    if (m_contentMIMEType.isEmpty())
        engine = nextMediaEngine(current);
    else
        engine = bestMediaEngineForTypeAndCodecs(m_contentMIMEType, m_contentTypeCodecs, current);

    // Don't delete and recreate the player unless it comes from a different engine.
    if (!engine) {
        m_currentMediaEngine = engine;
        m_private = nullptr;
    } else if (m_currentMediaEngine != engine) {
        m_currentMediaEngine = engine;
        m_private = engine->constructor(this);
        if (m_mediaPlayerClient)
            m_mediaPlayerClient->mediaPlayerEngineUpdated(this);
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
        m_private->setMediaPlayerProxy(m_playerProxy);
#endif
        m_private->setPrivateBrowsingMode(m_privateBrowsing);
        m_private->setPreload(m_preload);
        m_private->setPreservesPitch(preservesPitch());
        if (m_shouldPrepareToRender)
            m_private->prepareForRendering();
    }

    if (m_private)
        m_private->load(m_url);
    else {
        m_private = createNullMediaPlayer(this);
        if (m_mediaPlayerClient)
            m_mediaPlayerClient->mediaPlayerEngineUpdated(this);
    }
}    

bool MediaPlayer::hasAvailableVideoFrame() const
{
    return m_private->hasAvailableVideoFrame();
}

void MediaPlayer::prepareForRendering()
{
    m_shouldPrepareToRender = true;
    m_private->prepareForRendering();
}

bool MediaPlayer::canLoadPoster() const
{
    return m_private->canLoadPoster();
}

void MediaPlayer::setPoster(const String& url)
{
    m_private->setPoster(url);
}    

void MediaPlayer::cancelLoad()
{
    m_private->cancelLoad();
}    

void MediaPlayer::prepareToPlay()
{
    m_private->prepareToPlay();
}

void MediaPlayer::play()
{
    m_private->play();
}

void MediaPlayer::pause()
{
    m_private->pause();
}

float MediaPlayer::duration() const
{
    return m_private->duration();
}

float MediaPlayer::startTime() const
{
    return m_private->startTime();
}

float MediaPlayer::currentTime() const
{
    return m_private->currentTime();
}

void MediaPlayer::seek(float time)
{
    m_private->seek(time);
}

bool MediaPlayer::paused() const
{
    return m_private->paused();
}

bool MediaPlayer::seeking() const
{
    return m_private->seeking();
}

bool MediaPlayer::supportsFullscreen() const
{
    return m_private->supportsFullscreen();
}

bool MediaPlayer::supportsSave() const
{
    return m_private->supportsSave();
}

IntSize MediaPlayer::naturalSize()
{
    return m_private->naturalSize();
}

bool MediaPlayer::hasVideo() const
{
    return m_private->hasVideo();
}

bool MediaPlayer::hasAudio() const
{
    return m_private->hasAudio();
}

bool MediaPlayer::inMediaDocument()
{
    Frame* frame = m_frameView ? m_frameView->frame() : 0;
    Document* document = frame ? frame->document() : 0;

    return document && document->isMediaDocument();
}

PlatformMedia MediaPlayer::platformMedia() const
{
    return m_private->platformMedia();
}

#if USE(ACCELERATED_COMPOSITING)
PlatformLayer* MediaPlayer::platformLayer() const
{
    return m_private->platformLayer();
}
#endif

MediaPlayer::NetworkState MediaPlayer::networkState()
{
    return m_private->networkState();
}

MediaPlayer::ReadyState MediaPlayer::readyState()
{
    return m_private->readyState();
}

float MediaPlayer::volume() const
{
    return m_volume;
}

void MediaPlayer::setVolume(float volume)
{
    m_volume = volume;

    if (m_private->supportsMuting() || !m_muted)
        m_private->setVolume(volume);
}

bool MediaPlayer::muted() const
{
    return m_muted;
}

void MediaPlayer::setMuted(bool muted)
{
    m_muted = muted;

    if (m_private->supportsMuting())
        m_private->setMuted(muted);
    else
        m_private->setVolume(muted ? 0 : m_volume);
}

bool MediaPlayer::hasClosedCaptions() const
{
    return m_private->hasClosedCaptions();
}

void MediaPlayer::setClosedCaptionsVisible(bool closedCaptionsVisible)
{
    m_private->setClosedCaptionsVisible(closedCaptionsVisible);
}

float MediaPlayer::rate() const
{
    return m_rate;
}

void MediaPlayer::setRate(float rate)
{
    m_rate = rate;
    m_private->setRate(rate);   
}

bool MediaPlayer::preservesPitch() const
{
    return m_preservesPitch;
}

void MediaPlayer::setPreservesPitch(bool preservesPitch)
{
    m_preservesPitch = preservesPitch;
    m_private->setPreservesPitch(preservesPitch);
}

PassRefPtr<TimeRanges> MediaPlayer::buffered()
{
    return m_private->buffered();
}

float MediaPlayer::maxTimeSeekable()
{
    return m_private->maxTimeSeekable();
}

unsigned MediaPlayer::bytesLoaded()
{
    return m_private->bytesLoaded();
}

void MediaPlayer::setSize(const IntSize& size)
{ 
    m_size = size;
    m_private->setSize(size);
}

bool MediaPlayer::visible() const
{
    return m_visible;
}

void MediaPlayer::setVisible(bool b)
{
    m_visible = b;
    m_private->setVisible(b);
}

MediaPlayer::Preload MediaPlayer::preload() const
{
    return m_preload;
}

void MediaPlayer::setPreload(MediaPlayer::Preload preload)
{
    m_preload = preload;
    m_private->setPreload(preload);
}

void MediaPlayer::paint(GraphicsContext* p, const IntRect& r)
{
    m_private->paint(p, r);
}

void MediaPlayer::paintCurrentFrameInContext(GraphicsContext* p, const IntRect& r)
{
    m_private->paintCurrentFrameInContext(p, r);
}

MediaPlayer::SupportsType MediaPlayer::supportsType(const ContentType& contentType)
{
    String type = contentType.type().lower();
    String typeCodecs = contentType.parameter(codecs());

    // 4.8.10.3 MIME types - The canPlayType(type) method must return the empty string if type is a type that the 
    // user agent knows it cannot render or is the type "application/octet-stream"
    if (type == applicationOctetStream())
        return IsNotSupported;

    MediaPlayerFactory* engine = bestMediaEngineForTypeAndCodecs(type, typeCodecs);
    if (!engine)
        return IsNotSupported;

    return engine->supportsTypeAndCodecs(type, typeCodecs);
}

void MediaPlayer::getSupportedTypes(HashSet<String>& types)
{
    Vector<MediaPlayerFactory*>& engines = installedMediaEngines();
    if (engines.isEmpty())
        return;

    unsigned count = engines.size();
    for (unsigned ndx = 0; ndx < count; ndx++)
        engines[ndx]->getSupportedTypes(types);
} 

bool MediaPlayer::isAvailable()
{
    return !installedMediaEngines().isEmpty();
} 

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
void MediaPlayer::deliverNotification(MediaPlayerProxyNotificationType notification)
{
    m_private->deliverNotification(notification);
}

void MediaPlayer::setMediaPlayerProxy(WebMediaPlayerProxy* proxy)
{
    m_playerProxy = proxy;
    m_private->setMediaPlayerProxy(proxy);
}

void MediaPlayer::setControls(bool controls)
{
    m_private->setControls(controls);
}    

void MediaPlayer::enterFullscreen()
{
    m_private->enterFullscreen();
}    

void MediaPlayer::exitFullscreen()
{
    m_private->exitFullscreen();
}    
#endif

#if USE(ACCELERATED_COMPOSITING)
void MediaPlayer::acceleratedRenderingStateChanged()
{
    m_private->acceleratedRenderingStateChanged();
}

bool MediaPlayer::supportsAcceleratedRendering() const
{
    return m_private->supportsAcceleratedRendering();
}
#endif // USE(ACCELERATED_COMPOSITING)

bool MediaPlayer::hasSingleSecurityOrigin() const
{
    return m_private->hasSingleSecurityOrigin();
}

MediaPlayer::MovieLoadType MediaPlayer::movieLoadType() const
{
    return m_private->movieLoadType();
}

float MediaPlayer::mediaTimeForTimeValue(float timeValue) const
{
    return m_private->mediaTimeForTimeValue(timeValue);
}

double MediaPlayer::maximumDurationToCacheMediaTime() const
{
    return m_private->maximumDurationToCacheMediaTime();
}

unsigned MediaPlayer::decodedFrameCount() const
{
    return m_private->decodedFrameCount();
}

unsigned MediaPlayer::droppedFrameCount() const
{
    return m_private->droppedFrameCount();
}

unsigned MediaPlayer::audioDecodedByteCount() const
{
    return m_private->audioDecodedByteCount();
}

unsigned MediaPlayer::videoDecodedByteCount() const
{
    return m_private->videoDecodedByteCount();
}

void MediaPlayer::reloadTimerFired(Timer<MediaPlayer>*)
{
    m_private->cancelLoad();
    loadWithNextMediaEngine(m_currentMediaEngine);
}

void MediaPlayer::getSitesInMediaCache(Vector<String>& sites)
{
    Vector<MediaPlayerFactory*>& engines = installedMediaEngines();
    unsigned size = engines.size();
    for (unsigned i = 0; i < size; i++) {
        if (!engines[i]->getSitesInMediaCache)
            continue;
        Vector<String> engineSites;
        engines[i]->getSitesInMediaCache(engineSites);
        sites.append(engineSites);
    }
}

void MediaPlayer::clearMediaCache()
{
    Vector<MediaPlayerFactory*>& engines = installedMediaEngines();
    unsigned size = engines.size();
    for (unsigned i = 0; i < size; i++) {
        if (engines[i]->clearMediaCache)
            engines[i]->clearMediaCache();
    }
}

void MediaPlayer::clearMediaCacheForSite(const String& site)
{
    Vector<MediaPlayerFactory*>& engines = installedMediaEngines();
    unsigned size = engines.size();
    for (unsigned i = 0; i < size; i++) {
        if (engines[i]->clearMediaCacheForSite)
            engines[i]->clearMediaCacheForSite(site);
    }
}

void MediaPlayer::setPrivateBrowsingMode(bool privateBrowsingMode)
{
    m_privateBrowsing = privateBrowsingMode;
    m_private->setPrivateBrowsingMode(m_privateBrowsing);
}

// Client callbacks.
void MediaPlayer::networkStateChanged()
{
    // If more than one media engine is installed and this one failed before finding metadata,
    // let the next engine try.
    if (m_private->networkState() >= FormatError
        && m_private->readyState() < HaveMetadata
        && installedMediaEngines().size() > 1) {
        if ( m_contentMIMEType.isEmpty() || bestMediaEngineForTypeAndCodecs(m_contentMIMEType, m_contentTypeCodecs, m_currentMediaEngine)) {
            m_reloadTimer.startOneShot(0);
            return;
        }
    }
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerNetworkStateChanged(this);
}

void MediaPlayer::readyStateChanged()
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerReadyStateChanged(this);
}

void MediaPlayer::volumeChanged(float newVolume)
{
    m_volume = newVolume;
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerVolumeChanged(this);
}

void MediaPlayer::muteChanged(bool newMuted)
{
    m_muted = newMuted;
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerMuteChanged(this);
}

void MediaPlayer::timeChanged()
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerTimeChanged(this);
}

void MediaPlayer::sizeChanged()
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerSizeChanged(this);
}

void MediaPlayer::repaint()
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerRepaint(this);
}

void MediaPlayer::durationChanged()
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerDurationChanged(this);
}

void MediaPlayer::rateChanged()
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerRateChanged(this);
}

void MediaPlayer::playbackStateChanged()
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerPlaybackStateChanged(this);
}

void MediaPlayer::firstVideoFrameAvailable()
{
    if (m_mediaPlayerClient)
        m_mediaPlayerClient->mediaPlayerFirstVideoFrameAvailable(this);
}

}

#endif
