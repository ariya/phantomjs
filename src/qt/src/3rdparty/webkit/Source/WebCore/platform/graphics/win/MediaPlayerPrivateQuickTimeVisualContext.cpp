/*
 * Copyright (C) 2007, 2008, 2009, 2010, 2011 Apple, Inc.  All rights reserved.
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
#include "MediaPlayerPrivateQuickTimeVisualContext.h"

#include "ApplicationCacheHost.h"
#include "ApplicationCacheResource.h"
#include "Cookie.h"
#include "CookieJar.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "KURL.h"
#include "MediaPlayerPrivateTaskTimer.h"
#include "QTCFDictionary.h"
#include "QTDecompressionSession.h"
#include "QTMovie.h"
#include "QTMovieTask.h"
#include "QTMovieVisualContext.h"
#include "ScrollView.h"
#include "Settings.h"
#include "SoftLinking.h"
#include "TimeRanges.h"
#include "Timer.h"
#include <AssertMacros.h>
#include <CoreGraphics/CGAffineTransform.h>
#include <CoreGraphics/CGContext.h>
#include <QuartzCore/CATransform3D.h>
#include <Wininet.h>
#include <wtf/CurrentTime.h>
#include <wtf/HashSet.h>
#include <wtf/MainThread.h>
#include <wtf/MathExtras.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringHash.h>

#if USE(ACCELERATED_COMPOSITING)
#include "PlatformCALayer.h"
#include "WKCAImageQueue.h"
#endif

using namespace std;

namespace WebCore {

static CGImageRef CreateCGImageFromPixelBuffer(QTPixelBuffer buffer);
static bool requiredDllsAvailable();

SOFT_LINK_LIBRARY(Wininet)
SOFT_LINK(Wininet, InternetSetCookieExW, DWORD, WINAPI, (LPCWSTR lpszUrl, LPCWSTR lpszCookieName, LPCWSTR lpszCookieData, DWORD dwFlags, DWORD_PTR dwReserved), (lpszUrl, lpszCookieName, lpszCookieData, dwFlags, dwReserved))

// Interface declaration for MediaPlayerPrivateQuickTimeVisualContext's QTMovieClient aggregate
class MediaPlayerPrivateQuickTimeVisualContext::MovieClient : public QTMovieClient {
public:
    MovieClient(MediaPlayerPrivateQuickTimeVisualContext* parent) : m_parent(parent) {}
    virtual ~MovieClient() { m_parent = 0; }
    virtual void movieEnded(QTMovie*);
    virtual void movieLoadStateChanged(QTMovie*);
    virtual void movieTimeChanged(QTMovie*);
private:
    MediaPlayerPrivateQuickTimeVisualContext* m_parent;
};

#if USE(ACCELERATED_COMPOSITING)
class MediaPlayerPrivateQuickTimeVisualContext::LayerClient : public PlatformCALayerClient {
public:
    LayerClient(MediaPlayerPrivateQuickTimeVisualContext* parent) : m_parent(parent) {}
    virtual ~LayerClient() { m_parent = 0; }

private:
    virtual void platformCALayerLayoutSublayersOfLayer(PlatformCALayer*);
    virtual bool platformCALayerRespondsToLayoutChanges() const { return true; }

    virtual void platformCALayerAnimationStarted(CFTimeInterval beginTime) { }
    virtual GraphicsLayer::CompositingCoordinatesOrientation platformCALayerContentsOrientation() const { return GraphicsLayer::CompositingCoordinatesBottomUp; }
    virtual void platformCALayerPaintContents(GraphicsContext&, const IntRect& inClip) { }
    virtual bool platformCALayerShowDebugBorders() const { return false; }
    virtual bool platformCALayerShowRepaintCounter() const { return false; }
    virtual int platformCALayerIncrementRepaintCount() { return 0; }

    virtual bool platformCALayerContentsOpaque() const { return false; }
    virtual bool platformCALayerDrawsContent() const { return false; }
    virtual void platformCALayerLayerDidDisplay(PlatformLayer*) { }

    MediaPlayerPrivateQuickTimeVisualContext* m_parent;
};

void MediaPlayerPrivateQuickTimeVisualContext::LayerClient::platformCALayerLayoutSublayersOfLayer(PlatformCALayer* layer)
{
    ASSERT(m_parent);
    ASSERT(m_parent->m_transformLayer == layer);

    FloatSize parentSize = layer->bounds().size();
    FloatSize naturalSize = m_parent->naturalSize();

    // Calculate the ratio of these two sizes and use that ratio to scale the qtVideoLayer:
    FloatSize ratio(parentSize.width() / naturalSize.width(), parentSize.height() / naturalSize.height());

    int videoWidth = 0;
    int videoHeight = 0;
    m_parent->m_movie->getNaturalSize(videoWidth, videoHeight);
    FloatRect videoBounds(0, 0, videoWidth * ratio.width(), videoHeight * ratio.height());
    FloatPoint3D videoAnchor = m_parent->m_qtVideoLayer->anchorPoint();

    // Calculate the new position based on the parent's size:
    FloatPoint position(parentSize.width() * 0.5 - videoBounds.width() * (0.5 - videoAnchor.x()),
        parentSize.height() * 0.5 - videoBounds.height() * (0.5 - videoAnchor.y())); 

    m_parent->m_qtVideoLayer->setBounds(videoBounds);
    m_parent->m_qtVideoLayer->setPosition(position);
}
#endif

class MediaPlayerPrivateQuickTimeVisualContext::VisualContextClient : public QTMovieVisualContextClient {
public:
    VisualContextClient(MediaPlayerPrivateQuickTimeVisualContext* parent) : m_parent(parent) {}
    virtual ~VisualContextClient() { m_parent = 0; }
    void imageAvailableForTime(const QTCVTimeStamp*);
    static void retrieveCurrentImageProc(void*);
private:
    MediaPlayerPrivateQuickTimeVisualContext* m_parent;
};

PassOwnPtr<MediaPlayerPrivateInterface> MediaPlayerPrivateQuickTimeVisualContext::create(MediaPlayer* player)
{ 
    return adoptPtr(new MediaPlayerPrivateQuickTimeVisualContext(player));
}

void MediaPlayerPrivateQuickTimeVisualContext::registerMediaEngine(MediaEngineRegistrar registrar)
{
    if (isAvailable())
        registrar(create, getSupportedTypes, supportsType, 0, 0, 0);
}

MediaPlayerPrivateQuickTimeVisualContext::MediaPlayerPrivateQuickTimeVisualContext(MediaPlayer* player)
    : m_player(player)
    , m_seekTo(-1)
    , m_seekTimer(this, &MediaPlayerPrivateQuickTimeVisualContext::seekTimerFired)
    , m_visualContextTimer(this, &MediaPlayerPrivateQuickTimeVisualContext::visualContextTimerFired)
    , m_networkState(MediaPlayer::Empty)
    , m_readyState(MediaPlayer::HaveNothing)
    , m_enabledTrackCount(0)
    , m_totalTrackCount(0)
    , m_hasUnsupportedTracks(false)
    , m_startedPlaying(false)
    , m_isStreaming(false)
    , m_visible(false)
    , m_newFrameAvailable(false)
    , m_movieClient(adoptPtr(new MediaPlayerPrivateQuickTimeVisualContext::MovieClient(this)))
#if USE(ACCELERATED_COMPOSITING)
    , m_layerClient(adoptPtr(new MediaPlayerPrivateQuickTimeVisualContext::LayerClient(this)))
    , m_movieTransform(CGAffineTransformIdentity)
#endif
    , m_visualContextClient(adoptPtr(new MediaPlayerPrivateQuickTimeVisualContext::VisualContextClient(this)))
    , m_delayingLoad(false)
    , m_privateBrowsing(false)
    , m_preload(MediaPlayer::Auto)
{
}

MediaPlayerPrivateQuickTimeVisualContext::~MediaPlayerPrivateQuickTimeVisualContext()
{
    tearDownVideoRendering();
    cancelCallOnMainThread(&VisualContextClient::retrieveCurrentImageProc, this);
}

bool MediaPlayerPrivateQuickTimeVisualContext::supportsFullscreen() const
{
#if USE(ACCELERATED_COMPOSITING)
    Document* document = m_player->mediaPlayerClient()->mediaPlayerOwningDocument(); 
    if (document && document->settings())
        return document->settings()->acceleratedCompositingEnabled();
#endif
    return false;
}

PlatformMedia MediaPlayerPrivateQuickTimeVisualContext::platformMedia() const
{
    PlatformMedia p;
    p.type = PlatformMedia::QTMovieVisualContextType;
    p.media.qtMovieVisualContext = m_visualContext.get();
    return p;
}
#if USE(ACCELERATED_COMPOSITING)

PlatformLayer* MediaPlayerPrivateQuickTimeVisualContext::platformLayer() const
{
    return m_transformLayer ? m_transformLayer->platformLayer() : 0;
}
#endif

String MediaPlayerPrivateQuickTimeVisualContext::rfc2616DateStringFromTime(CFAbsoluteTime time)
{
    static const char* const dayStrings[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
    static const char* const monthStrings[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    static const CFStringRef dateFormatString = CFSTR("%s, %02d %s %04d %02d:%02d:%02d GMT");
    static CFTimeZoneRef gmtTimeZone;
    if (!gmtTimeZone)
        gmtTimeZone = CFTimeZoneCopyDefault();

    CFGregorianDate dateValue = CFAbsoluteTimeGetGregorianDate(time, gmtTimeZone); 
    if (!CFGregorianDateIsValid(dateValue, kCFGregorianAllUnits))
        return String();

    time = CFGregorianDateGetAbsoluteTime(dateValue, gmtTimeZone);
    SInt32 day = CFAbsoluteTimeGetDayOfWeek(time, 0);

    RetainPtr<CFStringRef> dateCFString(AdoptCF, CFStringCreateWithFormat(0, 0, dateFormatString, dayStrings[day - 1], dateValue.day, 
        monthStrings[dateValue.month - 1], dateValue.year, dateValue.hour, dateValue.minute, (int)dateValue.second));
    return dateCFString.get();
}

static void addCookieParam(StringBuilder& cookieBuilder, const String& name, const String& value)
{
    if (name.isEmpty())
        return;

    // If this isn't the first parameter added, terminate the previous one.
    if (cookieBuilder.length())
        cookieBuilder.append("; ");

    // Add parameter name, and value if there is one.
    cookieBuilder.append(name);
    if (!value.isEmpty()) {
        cookieBuilder.append('=');
        cookieBuilder.append(value);
    }
}

void MediaPlayerPrivateQuickTimeVisualContext::setUpCookiesForQuickTime(const String& url)
{
    // WebCore loaded the page with the movie URL with CFNetwork but QuickTime will 
    // use WinINet to download the movie, so we need to copy any cookies needed to
    // download the movie into WinInet before asking QuickTime to open it.
    Document* document = m_player->mediaPlayerClient()->mediaPlayerOwningDocument();
    Frame* frame = document ? document->frame() : 0;
    if (!frame || !frame->page() || !frame->page()->cookieEnabled())
        return;

    KURL movieURL = KURL(KURL(), url);
    Vector<Cookie> documentCookies;
    if (!getRawCookies(frame->document(), movieURL, documentCookies))
        return;

    for (size_t ndx = 0; ndx < documentCookies.size(); ndx++) {
        const Cookie& cookie = documentCookies[ndx];

        if (cookie.name.isEmpty())
            continue;

        // Build up the cookie string with as much information as we can get so WinINet
        // knows what to do with it.
        StringBuilder cookieBuilder;
        addCookieParam(cookieBuilder, cookie.name, cookie.value);
        addCookieParam(cookieBuilder, "path", cookie.path);
        if (cookie.expires) 
            addCookieParam(cookieBuilder, "expires", rfc2616DateStringFromTime(cookie.expires));
        if (cookie.httpOnly) 
            addCookieParam(cookieBuilder, "httpOnly", String());
        cookieBuilder.append(';');

        String cookieURL;
        if (!cookie.domain.isEmpty()) {
            StringBuilder urlBuilder;

            urlBuilder.append(movieURL.protocol());
            urlBuilder.append("://");
            if (cookie.domain[0] == '.')
                urlBuilder.append(cookie.domain.substring(1));
            else
                urlBuilder.append(cookie.domain);
            if (cookie.path.length() > 1)
                urlBuilder.append(cookie.path);

            cookieURL = urlBuilder.toString();
        } else
            cookieURL = movieURL;

        InternetSetCookieExW(cookieURL.charactersWithNullTermination(), 0, cookieBuilder.toString().charactersWithNullTermination(), 0, 0);
    }
}

static void disableComponentsOnce()
{
    static bool sComponentsDisabled = false;
    if (sComponentsDisabled)
        return;
    sComponentsDisabled = true;

    uint32_t componentsToDisable[][5] = {
        {'eat ', 'TEXT', 'text', 0, 0},
        {'eat ', 'TXT ', 'text', 0, 0},    
        {'eat ', 'utxt', 'text', 0, 0},  
        {'eat ', 'TEXT', 'tx3g', 0, 0},  
    };

    for (size_t i = 0; i < WTF_ARRAY_LENGTH(componentsToDisable); ++i) 
        QTMovie::disableComponent(componentsToDisable[i]);
}

void MediaPlayerPrivateQuickTimeVisualContext::resumeLoad()
{
    m_delayingLoad = false;

    if (!m_movieURL.isEmpty())
        loadInternal(m_movieURL);
}

void MediaPlayerPrivateQuickTimeVisualContext::load(const String& url)
{
    m_movieURL = url;

    if (m_preload == MediaPlayer::None) {
        m_delayingLoad = true;
        return;
    }

    loadInternal(url);
}

void MediaPlayerPrivateQuickTimeVisualContext::loadInternal(const String& url)
{
    if (!QTMovie::initializeQuickTime()) {
        // FIXME: is this the right error to return?
        m_networkState = MediaPlayer::DecodeError; 
        m_player->networkStateChanged();
        return;
    }

    disableComponentsOnce();

    // Initialize the task timer.
    MediaPlayerPrivateTaskTimer::initialize();

    if (m_networkState != MediaPlayer::Loading) {
        m_networkState = MediaPlayer::Loading;
        m_player->networkStateChanged();
    }
    if (m_readyState != MediaPlayer::HaveNothing) {
        m_readyState = MediaPlayer::HaveNothing;
        m_player->readyStateChanged();
    }
    cancelSeek();

    setUpCookiesForQuickTime(url);

    m_movie = adoptRef(new QTMovie(m_movieClient.get()));

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    Frame* frame = m_player->frameView() ? m_player->frameView()->frame() : 0;
    ApplicationCacheHost* cacheHost = frame ? frame->loader()->documentLoader()->applicationCacheHost() : 0;
    ApplicationCacheResource* resource = 0;
    if (cacheHost && cacheHost->shouldLoadResourceFromApplicationCache(ResourceRequest(url), resource) && resource && !resource->path().isEmpty())
        m_movie->load(resource->path().characters(), resource->path().length(), m_player->preservesPitch());
    else
#endif
        m_movie->load(url.characters(), url.length(), m_player->preservesPitch());
    m_movie->setVolume(m_player->volume());
}

void MediaPlayerPrivateQuickTimeVisualContext::prepareToPlay()
{
    if (!m_movie || m_delayingLoad)
        resumeLoad();
}

void MediaPlayerPrivateQuickTimeVisualContext::play()
{
    if (!m_movie)
        return;
    m_startedPlaying = true;

    m_movie->play();
    m_visualContextTimer.startRepeating(1.0 / 30);
}

void MediaPlayerPrivateQuickTimeVisualContext::pause()
{
    if (!m_movie)
        return;
    m_startedPlaying = false;

    m_movie->pause();
    m_visualContextTimer.stop();
}

float MediaPlayerPrivateQuickTimeVisualContext::duration() const
{
    if (!m_movie)
        return 0;
    return m_movie->duration();
}

float MediaPlayerPrivateQuickTimeVisualContext::currentTime() const
{
    if (!m_movie)
        return 0;
    return m_movie->currentTime();
}

void MediaPlayerPrivateQuickTimeVisualContext::seek(float time)
{
    cancelSeek();
    
    if (!m_movie)
        return;
    
    if (time > duration())
        time = duration();
    
    m_seekTo = time;
    if (maxTimeLoaded() >= m_seekTo)
        doSeek();
    else 
        m_seekTimer.start(0, 0.5f);
}
    
void MediaPlayerPrivateQuickTimeVisualContext::doSeek() 
{
    float oldRate = m_movie->rate();
    if (oldRate)
        m_movie->setRate(0);
    m_movie->setCurrentTime(m_seekTo);
    float timeAfterSeek = currentTime();
    // restore playback only if not at end, othewise QTMovie will loop
    if (oldRate && timeAfterSeek < duration())
        m_movie->setRate(oldRate);
    cancelSeek();
}

void MediaPlayerPrivateQuickTimeVisualContext::cancelSeek()
{
    m_seekTo = -1;
    m_seekTimer.stop();
}

void MediaPlayerPrivateQuickTimeVisualContext::seekTimerFired(Timer<MediaPlayerPrivateQuickTimeVisualContext>*)
{        
    if (!m_movie || !seeking() || currentTime() == m_seekTo) {
        cancelSeek();
        updateStates();
        m_player->timeChanged(); 
        return;
    } 
    
    if (maxTimeLoaded() >= m_seekTo)
        doSeek();
    else {
        MediaPlayer::NetworkState state = networkState();
        if (state == MediaPlayer::Empty || state == MediaPlayer::Loaded) {
            cancelSeek();
            updateStates();
            m_player->timeChanged();
        }
    }
}

bool MediaPlayerPrivateQuickTimeVisualContext::paused() const
{
    if (!m_movie)
        return true;
    return (!m_movie->rate());
}

bool MediaPlayerPrivateQuickTimeVisualContext::seeking() const
{
    if (!m_movie)
        return false;
    return m_seekTo >= 0;
}

IntSize MediaPlayerPrivateQuickTimeVisualContext::naturalSize() const
{
    if (!m_movie)
        return IntSize();
    int width;
    int height;
    m_movie->getNaturalSize(width, height);
#if USE(ACCELERATED_COMPOSITING)
    CGSize originalSize = {width, height};
    CGSize transformedSize = CGSizeApplyAffineTransform(originalSize, m_movieTransform);
    return IntSize(abs(transformedSize.width), abs(transformedSize.height));
#else
    return IntSize(width, height);
#endif
}

bool MediaPlayerPrivateQuickTimeVisualContext::hasVideo() const
{
    if (!m_movie)
        return false;
    return m_movie->hasVideo();
}

bool MediaPlayerPrivateQuickTimeVisualContext::hasAudio() const
{
    if (!m_movie)
        return false;
    return m_movie->hasAudio();
}

void MediaPlayerPrivateQuickTimeVisualContext::setVolume(float volume)
{
    if (!m_movie)
        return;
    m_movie->setVolume(volume);
}

void MediaPlayerPrivateQuickTimeVisualContext::setRate(float rate)
{
    if (!m_movie)
        return;

    // Do not call setRate(...) unless we have started playing; otherwise
    // QuickTime's VisualContext can get wedged waiting for a rate change
    // call which will never come.
    if (m_startedPlaying)
        m_movie->setRate(rate);
}

void MediaPlayerPrivateQuickTimeVisualContext::setPreservesPitch(bool preservesPitch)
{
    if (!m_movie)
        return;
    m_movie->setPreservesPitch(preservesPitch);
}

bool MediaPlayerPrivateQuickTimeVisualContext::hasClosedCaptions() const
{
    if (!m_movie)
        return false;
    return m_movie->hasClosedCaptions();
}

void MediaPlayerPrivateQuickTimeVisualContext::setClosedCaptionsVisible(bool visible)
{
    if (!m_movie)
        return;
    m_movie->setClosedCaptionsVisible(visible);
}

PassRefPtr<TimeRanges> MediaPlayerPrivateQuickTimeVisualContext::buffered() const
{
    RefPtr<TimeRanges> timeRanges = TimeRanges::create();
    float loaded = maxTimeLoaded();
    // rtsp streams are not buffered
    if (!m_isStreaming && loaded > 0)
        timeRanges->add(0, loaded);
    return timeRanges.release();
}

float MediaPlayerPrivateQuickTimeVisualContext::maxTimeSeekable() const
{
    // infinite duration means live stream
    return !isfinite(duration()) ? 0 : maxTimeLoaded();
}

float MediaPlayerPrivateQuickTimeVisualContext::maxTimeLoaded() const
{
    if (!m_movie)
        return 0;
    return m_movie->maxTimeLoaded(); 
}

unsigned MediaPlayerPrivateQuickTimeVisualContext::bytesLoaded() const
{
    if (!m_movie)
        return 0;
    float dur = duration();
    float maxTime = maxTimeLoaded();
    if (!dur)
        return 0;
    return totalBytes() * maxTime / dur;
}

unsigned MediaPlayerPrivateQuickTimeVisualContext::totalBytes() const
{
    if (!m_movie)
        return 0;
    return m_movie->dataSize();
}

void MediaPlayerPrivateQuickTimeVisualContext::cancelLoad()
{
    if (m_networkState < MediaPlayer::Loading || m_networkState == MediaPlayer::Loaded)
        return;
    
    tearDownVideoRendering();

    // Cancel the load by destroying the movie.
    m_movie.clear();

    updateStates();
}

void MediaPlayerPrivateQuickTimeVisualContext::updateStates()
{
    MediaPlayer::NetworkState oldNetworkState = m_networkState;
    MediaPlayer::ReadyState oldReadyState = m_readyState;
  
    long loadState = m_movie ? m_movie->loadState() : QTMovieLoadStateError;

    if (loadState >= QTMovieLoadStateLoaded && m_readyState < MediaPlayer::HaveMetadata) {
        m_movie->disableUnsupportedTracks(m_enabledTrackCount, m_totalTrackCount);
        if (m_player->inMediaDocument()) {
            if (!m_enabledTrackCount || m_enabledTrackCount != m_totalTrackCount) {
                // This is a type of media that we do not handle directly with a <video> 
                // element, eg. QuickTime VR, a movie with a sprite track, etc. Tell the 
                // MediaPlayerClient that we won't support it.
                sawUnsupportedTracks();
                return;
            }
        } else if (!m_enabledTrackCount)
            loadState = QTMovieLoadStateError;
    }

    // "Loaded" is reserved for fully buffered movies, never the case when streaming
    if (loadState >= QTMovieLoadStateComplete && !m_isStreaming) {
        m_networkState = MediaPlayer::Loaded;
        m_readyState = MediaPlayer::HaveEnoughData;
    } else if (loadState >= QTMovieLoadStatePlaythroughOK) {
        m_readyState = MediaPlayer::HaveEnoughData;
    } else if (loadState >= QTMovieLoadStatePlayable) {
        // FIXME: This might not work correctly in streaming case, <rdar://problem/5693967>
        m_readyState = currentTime() < maxTimeLoaded() ? MediaPlayer::HaveFutureData : MediaPlayer::HaveCurrentData;
    } else if (loadState >= QTMovieLoadStateLoaded) {
        m_readyState = MediaPlayer::HaveMetadata;
    } else if (loadState > QTMovieLoadStateError) {
        m_networkState = MediaPlayer::Loading;
        m_readyState = MediaPlayer::HaveNothing;        
    } else {
        if (m_player->inMediaDocument()) {
            // Something went wrong in the loading of media within a standalone file. 
            // This can occur with chained ref movies that eventually resolve to a
            // file we don't support.
            sawUnsupportedTracks();
            return;
        }

        float loaded = maxTimeLoaded();
        if (!loaded)
            m_readyState = MediaPlayer::HaveNothing;

        if (!m_enabledTrackCount)
            m_networkState = MediaPlayer::FormatError;
        else {
            // FIXME: We should differentiate between load/network errors and decode errors <rdar://problem/5605692>
            if (loaded > 0)
                m_networkState = MediaPlayer::DecodeError;
            else
                m_readyState = MediaPlayer::HaveNothing;
        }
    }

    if (isReadyForRendering() && !hasSetUpVideoRendering())
        setUpVideoRendering();

    if (seeking())
        m_readyState = MediaPlayer::HaveNothing;
    
    if (m_networkState != oldNetworkState)
        m_player->networkStateChanged();
    if (m_readyState != oldReadyState)
        m_player->readyStateChanged();
}

bool MediaPlayerPrivateQuickTimeVisualContext::isReadyForRendering() const
{
    return m_readyState >= MediaPlayer::HaveMetadata && m_player->visible();
}

void MediaPlayerPrivateQuickTimeVisualContext::sawUnsupportedTracks()
{
    m_movie->setDisabled(true);
    m_hasUnsupportedTracks = true;
    m_player->mediaPlayerClient()->mediaPlayerSawUnsupportedTracks(m_player);
}

void MediaPlayerPrivateQuickTimeVisualContext::didEnd()
{
    if (m_hasUnsupportedTracks)
        return;

    m_startedPlaying = false;

    updateStates();
    m_player->timeChanged();
}

void MediaPlayerPrivateQuickTimeVisualContext::setSize(const IntSize& size) 
{ 
    if (m_hasUnsupportedTracks || !m_movie || m_size == size)
        return;
    m_size = size;
}

void MediaPlayerPrivateQuickTimeVisualContext::setVisible(bool visible)
{
    if (m_hasUnsupportedTracks || !m_movie || m_visible == visible)
        return;

    m_visible = visible;
    if (m_visible) {
        if (isReadyForRendering())
            setUpVideoRendering();
    } else
        tearDownVideoRendering();
}

void MediaPlayerPrivateQuickTimeVisualContext::paint(GraphicsContext* p, const IntRect& r)
{
    MediaRenderingMode currentMode = currentRenderingMode();
 
    if (currentMode == MediaRenderingNone)
        return;

    if (currentMode == MediaRenderingSoftwareRenderer && !m_visualContext)
        return;

    QTPixelBuffer buffer = m_visualContext->imageForTime(0);
    if (buffer.pixelBufferRef()) {
#if USE(ACCELERATED_COMPOSITING)
        if (m_qtVideoLayer) {
            // We are probably being asked to render the video into a canvas, but 
            // there's a good chance the QTPixelBuffer is not ARGB and thus can't be
            // drawn using CG.  If so, fire up an ICMDecompressionSession and convert 
            // the current frame into something which can be rendered by CG.
            if (!buffer.pixelFormatIs32ARGB() && !buffer.pixelFormatIs32BGRA()) {
                // The decompression session will only decompress a specific pixelFormat 
                // at a specific width and height; if these differ, the session must be
                // recreated with the new parameters.
                if (!m_decompressionSession || !m_decompressionSession->canDecompress(buffer))
                    m_decompressionSession = QTDecompressionSession::create(buffer.pixelFormatType(), buffer.width(), buffer.height());
                buffer = m_decompressionSession->decompress(buffer);
            }
        }
#endif
        CGImageRef image = CreateCGImageFromPixelBuffer(buffer);
        
        CGContextRef context = p->platformContext();
        CGContextSaveGState(context);
        CGContextTranslateCTM(context, r.x(), r.y());
        CGContextTranslateCTM(context, 0, r.height());
        CGContextScaleCTM(context, 1, -1);
        CGContextDrawImage(context, CGRectMake(0, 0, r.width(), r.height()), image);
        CGContextRestoreGState(context);

        CGImageRelease(image);
    }
    paintCompleted(*p, r);
}

void MediaPlayerPrivateQuickTimeVisualContext::paintCompleted(GraphicsContext& context, const IntRect& rect)
{
    m_newFrameAvailable = false;
}

void MediaPlayerPrivateQuickTimeVisualContext::VisualContextClient::retrieveCurrentImageProc(void* refcon)
{
    static_cast<MediaPlayerPrivateQuickTimeVisualContext*>(refcon)->retrieveCurrentImage();
}

void MediaPlayerPrivateQuickTimeVisualContext::VisualContextClient::imageAvailableForTime(const QTCVTimeStamp* timeStamp)
{
    // This call may come in on another thread, so marshall to the main thread first:
    callOnMainThread(&retrieveCurrentImageProc, m_parent);

    // callOnMainThread must be paired with cancelCallOnMainThread in the destructor,
    // in case this object is deleted before the main thread request is handled.
}

void MediaPlayerPrivateQuickTimeVisualContext::visualContextTimerFired(Timer<MediaPlayerPrivateQuickTimeVisualContext>*)
{
    if (m_visualContext && m_visualContext->isImageAvailableForTime(0))
        retrieveCurrentImage();
}

static CFDictionaryRef QTCFDictionaryCreateWithDataCallback(CFAllocatorRef allocator, const UInt8* bytes, CFIndex length)
{
    RetainPtr<CFDataRef> data(AdoptCF, CFDataCreateWithBytesNoCopy(allocator, bytes, length, kCFAllocatorNull));
    if (!data)
        return 0;

    return reinterpret_cast<CFDictionaryRef>(CFPropertyListCreateFromXMLData(allocator, data.get(), kCFPropertyListImmutable, 0));
}

static CGImageRef CreateCGImageFromPixelBuffer(QTPixelBuffer buffer)
{
#if USE(ACCELERATED_COMPOSITING)
    CGDataProviderRef provider = 0;
    CGColorSpaceRef colorSpace = 0;
    CGImageRef image = 0;

    size_t bitsPerComponent = 0;
    size_t bitsPerPixel = 0;
    CGImageAlphaInfo alphaInfo = kCGImageAlphaNone;
        
    if (buffer.pixelFormatIs32BGRA()) {
        bitsPerComponent = 8;
        bitsPerPixel = 32;
        alphaInfo = (CGImageAlphaInfo)(kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little);
    } else if (buffer.pixelFormatIs32ARGB()) {
        bitsPerComponent = 8;
        bitsPerPixel = 32;
        alphaInfo = (CGImageAlphaInfo)(kCGImageAlphaNoneSkipLast | kCGBitmapByteOrder32Big);
    } else {
        // All other pixel formats are currently unsupported:
        ASSERT_NOT_REACHED();
    }

    CGDataProviderDirectAccessCallbacks callbacks = {
        &QTPixelBuffer::dataProviderGetBytePointerCallback,
        &QTPixelBuffer::dataProviderReleaseBytePointerCallback,
        &QTPixelBuffer::dataProviderGetBytesAtPositionCallback,
        &QTPixelBuffer::dataProviderReleaseInfoCallback,
    };
    
    // Colorspace should be device, so that Quartz does not have to do an extra render.
    colorSpace = CGColorSpaceCreateDeviceRGB();
    require(colorSpace, Bail);
            
    provider = CGDataProviderCreateDirectAccess(buffer.pixelBufferRef(), buffer.dataSize(), &callbacks);
    require(provider, Bail);

    // CGDataProvider does not retain the buffer, but it will release it later, so do an extra retain here:
    QTPixelBuffer::retainCallback(buffer.pixelBufferRef());
        
    image = CGImageCreate(buffer.width(), buffer.height(), bitsPerComponent, bitsPerPixel, buffer.bytesPerRow(), colorSpace, alphaInfo, provider, 0, false, kCGRenderingIntentDefault);
 
Bail:
    // Once the image is created we can release our reference to the provider and the colorspace, they are retained by the image
    if (provider)
        CGDataProviderRelease(provider);
    if (colorSpace)
        CGColorSpaceRelease(colorSpace);
 
    return image;
#else
    return 0;
#endif
}


void MediaPlayerPrivateQuickTimeVisualContext::retrieveCurrentImage()
{
    if (!m_visualContext)
        return;

#if USE(ACCELERATED_COMPOSITING)
    if (m_qtVideoLayer) {

        QTPixelBuffer buffer = m_visualContext->imageForTime(0);
        if (!buffer.pixelBufferRef())
            return;

        PlatformCALayer* layer = m_qtVideoLayer.get();

        if (!buffer.lockBaseAddress()) {
            if (requiredDllsAvailable()) {
                if (!m_imageQueue) {
                    m_imageQueue = adoptPtr(new WKCAImageQueue(buffer.width(), buffer.height(), 30));
                    m_imageQueue->setFlags(WKCAImageQueue::Fill, WKCAImageQueue::Fill);
                    layer->setContents(m_imageQueue->get());
                }

                // Debug QuickTime links against a non-Debug version of CoreFoundation, so the
                // CFDictionary attached to the CVPixelBuffer cannot be directly passed on into the
                // CAImageQueue without being converted to a non-Debug CFDictionary.  Additionally,
                // old versions of QuickTime used a non-AAS CoreFoundation, so the types are not 
                // interchangable even in the release case.
                RetainPtr<CFDictionaryRef> attachments(AdoptCF, QTCFDictionaryCreateCopyWithDataCallback(kCFAllocatorDefault, buffer.attachments(), &QTCFDictionaryCreateWithDataCallback));
                CFTimeInterval imageTime = QTMovieVisualContext::currentHostTime();

                m_imageQueue->collect();

                uint64_t imageId = m_imageQueue->registerPixelBuffer(buffer.baseAddress(), buffer.dataSize(), buffer.bytesPerRow(), buffer.width(), buffer.height(), buffer.pixelFormatType(), attachments.get(), 0);

                if (m_imageQueue->insertImage(imageTime, WKCAImageQueue::Buffer, imageId, WKCAImageQueue::Opaque | WKCAImageQueue::Flush, &QTPixelBuffer::imageQueueReleaseCallback, buffer.pixelBufferRef())) {
                    // Retain the buffer one extra time so it doesn't dissappear before CAImageQueue decides to release it:
                    QTPixelBuffer::retainCallback(buffer.pixelBufferRef());
                }

            } else {
                CGImageRef image = CreateCGImageFromPixelBuffer(buffer);
                layer->setContents(image);
                CGImageRelease(image);
            }

            buffer.unlockBaseAddress();
            layer->setNeedsCommit();
        }
    } else
#endif
        m_player->repaint();

    m_visualContext->task();
}

static HashSet<String> mimeTypeCache()
{
    DEFINE_STATIC_LOCAL(HashSet<String>, typeCache, ());
    static bool typeListInitialized = false;

    if (!typeListInitialized) {
        unsigned count = QTMovie::countSupportedTypes();
        for (unsigned n = 0; n < count; n++) {
            const UChar* character;
            unsigned len;
            QTMovie::getSupportedType(n, character, len);
            if (len)
                typeCache.add(String(character, len));
        }

        typeListInitialized = true;
    }
    
    return typeCache;
}

static CFStringRef createVersionStringFromModuleName(LPCWSTR moduleName)
{
    HMODULE module = GetModuleHandleW(moduleName);
    if (!module) 
        return 0;

    wchar_t filePath[MAX_PATH] = {0};
    if (!GetModuleFileNameW(module, filePath, MAX_PATH)) 
        return 0;

    DWORD versionInfoSize = GetFileVersionInfoSizeW(filePath, 0);
    if (!versionInfoSize)
        return 0;

    CFStringRef versionString = 0;
    void* versionInfo = calloc(versionInfoSize, sizeof(char));
    if (GetFileVersionInfo(filePath, 0, versionInfoSize, versionInfo)) {
        VS_FIXEDFILEINFO* fileInfo = 0;
        UINT fileInfoLength = 0;

        if (VerQueryValueW(versionInfo, L"\\", reinterpret_cast<LPVOID*>(&fileInfo), &fileInfoLength)) {
            versionString = CFStringCreateWithFormat(kCFAllocatorDefault, 0, CFSTR("%d.%d.%d.%d"), 
                HIWORD(fileInfo->dwFileVersionMS), LOWORD(fileInfo->dwFileVersionMS), 
                HIWORD(fileInfo->dwFileVersionLS), LOWORD(fileInfo->dwFileVersionLS));
        }
    }
    free(versionInfo);

    return versionString;
}

static bool requiredDllsAvailable() 
{
    static bool s_prerequisitesChecked = false;
    static bool s_prerequisitesSatisfied;
    static const CFStringRef kMinQuartzCoreVersion = CFSTR("1.0.42.0");
    static const CFStringRef kMinCoreVideoVersion = CFSTR("1.0.1.0");

    if (s_prerequisitesChecked)
        return s_prerequisitesSatisfied;
    s_prerequisitesChecked = true;
    s_prerequisitesSatisfied = false;

    CFStringRef quartzCoreString = createVersionStringFromModuleName(L"QuartzCore");
    if (!quartzCoreString)
        quartzCoreString = createVersionStringFromModuleName(L"QuartzCore_debug");

    CFStringRef coreVideoString = createVersionStringFromModuleName(L"CoreVideo");
    if (!coreVideoString)
        coreVideoString = createVersionStringFromModuleName(L"CoreVideo_debug");

    s_prerequisitesSatisfied = (quartzCoreString && coreVideoString
        && CFStringCompare(quartzCoreString, kMinQuartzCoreVersion, kCFCompareNumerically) != kCFCompareLessThan 
        && CFStringCompare(coreVideoString, kMinCoreVideoVersion, kCFCompareNumerically) != kCFCompareLessThan);

    if (quartzCoreString)
        CFRelease(quartzCoreString);
    if (coreVideoString)
        CFRelease(coreVideoString);

    return s_prerequisitesSatisfied;
}

void MediaPlayerPrivateQuickTimeVisualContext::getSupportedTypes(HashSet<String>& types)
{
    types = mimeTypeCache();
} 

bool MediaPlayerPrivateQuickTimeVisualContext::isAvailable()
{
    return QTMovie::initializeQuickTime();
}

MediaPlayer::SupportsType MediaPlayerPrivateQuickTimeVisualContext::supportsType(const String& type, const String& codecs)
{
    // only return "IsSupported" if there is no codecs parameter for now as there is no way to ask QT if it supports an
    //  extended MIME type
    return mimeTypeCache().contains(type) ? (codecs.isEmpty() ? MediaPlayer::MayBeSupported : MediaPlayer::IsSupported) : MediaPlayer::IsNotSupported;
}

void MediaPlayerPrivateQuickTimeVisualContext::MovieClient::movieEnded(QTMovie* movie)
{
    if (m_parent->m_hasUnsupportedTracks)
        return;

    m_parent->m_visualContextTimer.stop();

    ASSERT(m_parent->m_movie.get() == movie);
    m_parent->didEnd();
}

void MediaPlayerPrivateQuickTimeVisualContext::MovieClient::movieLoadStateChanged(QTMovie* movie)
{
    if (m_parent->m_hasUnsupportedTracks)
        return;

    ASSERT(m_parent->m_movie.get() == movie);
    m_parent->updateStates();
}

void MediaPlayerPrivateQuickTimeVisualContext::MovieClient::movieTimeChanged(QTMovie* movie)
{
    if (m_parent->m_hasUnsupportedTracks)
        return;

    ASSERT(m_parent->m_movie.get() == movie);
    m_parent->updateStates();
    m_parent->m_player->timeChanged();
}

bool MediaPlayerPrivateQuickTimeVisualContext::hasSingleSecurityOrigin() const
{
    // We tell quicktime to disallow resources that come from different origins
    // so we all media is single origin.
    return true;
}

void MediaPlayerPrivateQuickTimeVisualContext::setPreload(MediaPlayer::Preload preload)
{
    m_preload = preload;
    if (m_delayingLoad && m_preload != MediaPlayer::None)
        resumeLoad();
}

float MediaPlayerPrivateQuickTimeVisualContext::mediaTimeForTimeValue(float timeValue) const
{
    long timeScale;
    if (m_readyState < MediaPlayer::HaveMetadata || !(timeScale = m_movie->timeScale()))
        return timeValue;

    long mediaTimeValue = lroundf(timeValue * timeScale);
    return static_cast<float>(mediaTimeValue) / timeScale;
}

MediaPlayerPrivateQuickTimeVisualContext::MediaRenderingMode MediaPlayerPrivateQuickTimeVisualContext::currentRenderingMode() const
{
    if (!m_movie)
        return MediaRenderingNone;

#if USE(ACCELERATED_COMPOSITING)
    if (m_qtVideoLayer)
        return MediaRenderingMovieLayer;
#endif

    return m_visualContext ? MediaRenderingSoftwareRenderer : MediaRenderingNone;
}

MediaPlayerPrivateQuickTimeVisualContext::MediaRenderingMode MediaPlayerPrivateQuickTimeVisualContext::preferredRenderingMode() const
{
    if (!m_player->frameView() || !m_movie)
        return MediaRenderingNone;

#if USE(ACCELERATED_COMPOSITING)
    if (supportsAcceleratedRendering() && m_player->mediaPlayerClient()->mediaPlayerRenderingCanBeAccelerated(m_player))
        return MediaRenderingMovieLayer;
#endif

    return MediaRenderingSoftwareRenderer;
}

void MediaPlayerPrivateQuickTimeVisualContext::setUpVideoRendering()
{
    MediaRenderingMode currentMode = currentRenderingMode();
    MediaRenderingMode preferredMode = preferredRenderingMode();

#if !USE(ACCELERATED_COMPOSITING)
    ASSERT(preferredMode != MediaRenderingMovieLayer);
#endif

    if (currentMode == preferredMode && currentMode != MediaRenderingNone)
        return;

    if (currentMode != MediaRenderingNone)  
        tearDownVideoRendering();

    if (preferredMode == MediaRenderingMovieLayer)
        createLayerForMovie();

#if USE(ACCELERATED_COMPOSITING)
    if (currentMode == MediaRenderingMovieLayer || preferredMode == MediaRenderingMovieLayer)
        m_player->mediaPlayerClient()->mediaPlayerRenderingModeChanged(m_player);
#endif

    QTPixelBuffer::Type contextType = requiredDllsAvailable() && preferredMode == MediaRenderingMovieLayer ? QTPixelBuffer::ConfigureForCAImageQueue : QTPixelBuffer::ConfigureForCGImage;
    m_visualContext = QTMovieVisualContext::create(m_visualContextClient.get(), contextType);
    m_visualContext->setMovie(m_movie.get());
}

void MediaPlayerPrivateQuickTimeVisualContext::tearDownVideoRendering()
{
#if USE(ACCELERATED_COMPOSITING)
    if (m_qtVideoLayer)
        destroyLayerForMovie();
#endif

    m_visualContext = 0;
}

bool MediaPlayerPrivateQuickTimeVisualContext::hasSetUpVideoRendering() const
{
#if USE(ACCELERATED_COMPOSITING)
    return m_qtVideoLayer || (currentRenderingMode() != MediaRenderingMovieLayer && m_visualContext);
#else
    return true;
#endif
}

void MediaPlayerPrivateQuickTimeVisualContext::retrieveAndResetMovieTransform()
{
#if USE(ACCELERATED_COMPOSITING)
    // First things first, reset the total movie transform so that
    // we can bail out early:
    m_movieTransform = CGAffineTransformIdentity;

    if (!m_movie || !m_movie->hasVideo())
        return;

    // This trick will only work on movies with a single video track,
    // so bail out early if the video contains more than one (or zero)
    // video tracks.
    QTTrackArray videoTracks = m_movie->videoTracks();
    if (videoTracks.size() != 1)
        return;

    QTTrack* track = videoTracks[0].get();
    ASSERT(track);

    CGAffineTransform movieTransform = m_movie->getTransform();
    if (!CGAffineTransformEqualToTransform(movieTransform, CGAffineTransformIdentity))
        m_movie->resetTransform();

    CGAffineTransform trackTransform = track->getTransform();
    if (!CGAffineTransformEqualToTransform(trackTransform, CGAffineTransformIdentity))
        track->resetTransform();

    // Multiply the two transforms together, taking care to 
    // do so in the correct order, track * movie = final:
    m_movieTransform = CGAffineTransformConcat(trackTransform, movieTransform);
#endif
}

void MediaPlayerPrivateQuickTimeVisualContext::createLayerForMovie()
{
#if USE(ACCELERATED_COMPOSITING)
    ASSERT(supportsAcceleratedRendering());

    if (!m_movie || m_qtVideoLayer)
        return;

    // Create a PlatformCALayer which will transform the contents of the video layer
    // which is in m_qtVideoLayer.
    m_transformLayer = PlatformCALayer::create(PlatformCALayer::LayerTypeLayer, m_layerClient.get());
    if (!m_transformLayer)
        return;

    // Mark the layer as anchored in the top left.
    m_transformLayer->setAnchorPoint(FloatPoint3D());

    m_qtVideoLayer = PlatformCALayer::create(PlatformCALayer::LayerTypeLayer, 0);
    if (!m_qtVideoLayer)
        return;

    if (CGAffineTransformEqualToTransform(m_movieTransform, CGAffineTransformIdentity))
        retrieveAndResetMovieTransform();
    CGAffineTransform t = m_movieTransform;

    // Remove the translation portion of the transform, since we will always rotate about
    // the layer's center point.  In our limited use-case (a single video track), this is
    // safe:
    t.tx = t.ty = 0;
    m_qtVideoLayer->setTransform(CATransform3DMakeAffineTransform(t));

#ifndef NDEBUG
    m_qtVideoLayer->setName("Video layer");
#endif
    m_transformLayer->appendSublayer(m_qtVideoLayer.get());
    m_transformLayer->setNeedsLayout();
    // The layer will get hooked up via RenderLayerBacking::updateGraphicsLayerConfiguration().
#endif

    // Fill the newly created layer with image data, so we're not looking at 
    // an empty layer until the next time a new image is available, which could
    // be a long time if we're paused.
    if (m_visualContext)
        retrieveCurrentImage();
}

void MediaPlayerPrivateQuickTimeVisualContext::destroyLayerForMovie()
{
#if USE(ACCELERATED_COMPOSITING)
    if (m_qtVideoLayer) {
        m_qtVideoLayer->removeFromSuperlayer();
        m_qtVideoLayer = 0;
    }

    if (m_transformLayer)
        m_transformLayer = 0;

    if (m_imageQueue)
        m_imageQueue = nullptr;
#endif
}

#if USE(ACCELERATED_COMPOSITING)
bool MediaPlayerPrivateQuickTimeVisualContext::supportsAcceleratedRendering() const
{
    return isReadyForRendering();
}

void MediaPlayerPrivateQuickTimeVisualContext::acceleratedRenderingStateChanged()
{
    // Set up or change the rendering path if necessary.
    setUpVideoRendering();
}

void MediaPlayerPrivateQuickTimeVisualContext::setPrivateBrowsingMode(bool privateBrowsing)
{
    m_privateBrowsing = privateBrowsing;
    if (m_movie)
        m_movie->setPrivateBrowsingMode(m_privateBrowsing);
}
    
#endif


}

#endif
