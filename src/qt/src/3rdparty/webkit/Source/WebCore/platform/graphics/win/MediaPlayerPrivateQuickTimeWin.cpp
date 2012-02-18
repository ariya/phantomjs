/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple, Inc.  All rights reserved.
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
#include "MediaPlayerPrivateQuickTimeWin.h"

#include "Cookie.h"
#include "CookieJar.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "KURL.h"
#include "MediaPlayerPrivateTaskTimer.h"
#include "QTMovieTask.h"
#include "ScrollView.h"
#include "SoftLinking.h"
#include "TimeRanges.h"
#include "Timer.h"
#include <CoreGraphics/CGColorSpace.h>
#include <CoreGraphics/CGContext.h>
#include <CoreGraphics/CGImage.h>
#include <Wininet.h>
#include <wtf/CurrentTime.h>
#include <wtf/HashSet.h>
#include <wtf/MathExtras.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringHash.h>

#if USE(ACCELERATED_COMPOSITING)
#include "GraphicsLayerCACF.h"
#include "PlatformCALayer.h"
#endif

#if DRAW_FRAME_RATE
#include "Document.h"
#include "Font.h"
#include "RenderObject.h"
#include "RenderStyle.h"
#include "Windows.h"
#endif

using namespace std;

namespace WebCore {

SOFT_LINK_LIBRARY(Wininet)
SOFT_LINK(Wininet, InternetSetCookieExW, DWORD, WINAPI, (LPCWSTR lpszUrl, LPCWSTR lpszCookieName, LPCWSTR lpszCookieData, DWORD dwFlags, DWORD_PTR dwReserved), (lpszUrl, lpszCookieName, lpszCookieData, dwFlags, dwReserved))

PassOwnPtr<MediaPlayerPrivateInterface> MediaPlayerPrivate::create(MediaPlayer* player)
{ 
    return adoptPtr(new MediaPlayerPrivate(player));
}

void MediaPlayerPrivate::registerMediaEngine(MediaEngineRegistrar registrar)
{
    if (isAvailable())
        registrar(create, getSupportedTypes, supportsType, 0, 0, 0);
}

MediaPlayerPrivate::MediaPlayerPrivate(MediaPlayer* player)
    : m_player(player)
    , m_seekTo(-1)
    , m_seekTimer(this, &MediaPlayerPrivate::seekTimerFired)
    , m_networkState(MediaPlayer::Empty)
    , m_readyState(MediaPlayer::HaveNothing)
    , m_enabledTrackCount(0)
    , m_totalTrackCount(0)
    , m_hasUnsupportedTracks(false)
    , m_startedPlaying(false)
    , m_isStreaming(false)
    , m_visible(false)
    , m_newFrameAvailable(false)
#if DRAW_FRAME_RATE
    , m_frameCountWhilePlaying(0)
    , m_timeStartedPlaying(0)
    , m_timeStoppedPlaying(0)
#endif
{
}

MediaPlayerPrivate::~MediaPlayerPrivate()
{
    tearDownVideoRendering();
    m_qtGWorld->setMovie(0);
}

bool MediaPlayerPrivate::supportsFullscreen() const
{
    return true;
}

PlatformMedia MediaPlayerPrivate::platformMedia() const
{
    PlatformMedia p;
    p.type = PlatformMedia::QTMovieGWorldType;
    p.media.qtMovieGWorld = m_qtGWorld.get();
    return p;
}

#if USE(ACCELERATED_COMPOSITING)
PlatformLayer* MediaPlayerPrivate::platformLayer() const
{
    return m_qtVideoLayer ? m_qtVideoLayer->platformLayer() : 0;
}
#endif

String MediaPlayerPrivate::rfc2616DateStringFromTime(CFAbsoluteTime time)
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


void MediaPlayerPrivate::setUpCookiesForQuickTime(const String& url)
{
    // WebCore loaded the page with the movie URL with CFNetwork but QuickTime will 
    // use WinINet to download the movie, so we need to copy any cookies needed to
    // download the movie into WinInet before asking QuickTime to open it.
    Frame* frame = m_player->frameView() ? m_player->frameView()->frame() : 0;
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

void MediaPlayerPrivate::load(const String& url)
{
    if (!QTMovie::initializeQuickTime()) {
        // FIXME: is this the right error to return?
        m_networkState = MediaPlayer::DecodeError; 
        m_player->networkStateChanged();
        return;
    }

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

    m_qtMovie = adoptRef(new QTMovie(this));
    m_qtMovie->load(url.characters(), url.length(), m_player->preservesPitch());
    m_qtMovie->setVolume(m_player->volume());

    m_qtGWorld = adoptRef(new QTMovieGWorld(this));
    m_qtGWorld->setMovie(m_qtMovie.get());
    m_qtGWorld->setVisible(m_player->visible());
}

void MediaPlayerPrivate::play()
{
    if (!m_qtMovie)
        return;
    m_startedPlaying = true;
#if DRAW_FRAME_RATE
    m_frameCountWhilePlaying = 0;
#endif

    m_qtMovie->play();
}

void MediaPlayerPrivate::pause()
{
    if (!m_qtMovie)
        return;
    m_startedPlaying = false;
#if DRAW_FRAME_RATE
    m_timeStoppedPlaying = WTF::currentTime();
#endif
    m_qtMovie->pause();
}

float MediaPlayerPrivate::duration() const
{
    if (!m_qtMovie)
        return 0;
    return m_qtMovie->duration();
}

float MediaPlayerPrivate::currentTime() const
{
    if (!m_qtMovie)
        return 0;
    return m_qtMovie->currentTime();
}

void MediaPlayerPrivate::seek(float time)
{
    cancelSeek();
    
    if (!m_qtMovie)
        return;
    
    if (time > duration())
        time = duration();
    
    m_seekTo = time;
    if (maxTimeLoaded() >= m_seekTo)
        doSeek();
    else 
        m_seekTimer.start(0, 0.5f);
}
    
void MediaPlayerPrivate::doSeek() 
{
    float oldRate = m_qtMovie->rate();
    if (oldRate)
        m_qtMovie->setRate(0);
    m_qtMovie->setCurrentTime(m_seekTo);
    float timeAfterSeek = currentTime();
    // restore playback only if not at end, othewise QTMovie will loop
    if (oldRate && timeAfterSeek < duration())
        m_qtMovie->setRate(oldRate);
    cancelSeek();
}

void MediaPlayerPrivate::cancelSeek()
{
    m_seekTo = -1;
    m_seekTimer.stop();
}

void MediaPlayerPrivate::seekTimerFired(Timer<MediaPlayerPrivate>*)
{        
    if (!m_qtMovie || !seeking() || currentTime() == m_seekTo) {
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

bool MediaPlayerPrivate::paused() const
{
    if (!m_qtMovie)
        return true;
    return (!m_qtMovie->rate());
}

bool MediaPlayerPrivate::seeking() const
{
    if (!m_qtMovie)
        return false;
    return m_seekTo >= 0;
}

IntSize MediaPlayerPrivate::naturalSize() const
{
    if (!m_qtMovie)
        return IntSize();
    int width;
    int height;
    m_qtMovie->getNaturalSize(width, height);
    return IntSize(width, height);
}

bool MediaPlayerPrivate::hasVideo() const
{
    if (!m_qtMovie)
        return false;
    return m_qtMovie->hasVideo();
}

bool MediaPlayerPrivate::hasAudio() const
{
    if (!m_qtMovie)
        return false;
    return m_qtMovie->hasAudio();
}

void MediaPlayerPrivate::setVolume(float volume)
{
    if (!m_qtMovie)
        return;
    m_qtMovie->setVolume(volume);
}

void MediaPlayerPrivate::setRate(float rate)
{
    if (!m_qtMovie)
        return;
    m_qtMovie->setRate(rate);
}

void MediaPlayerPrivate::setPreservesPitch(bool preservesPitch)
{
    if (!m_qtMovie)
        return;
    m_qtMovie->setPreservesPitch(preservesPitch);
}

bool MediaPlayerPrivate::hasClosedCaptions() const
{
    if (!m_qtMovie)
        return false;
    return m_qtMovie->hasClosedCaptions();
}

void MediaPlayerPrivate::setClosedCaptionsVisible(bool visible)
{
    if (!m_qtMovie)
        return;
    m_qtMovie->setClosedCaptionsVisible(visible);
}

PassRefPtr<TimeRanges> MediaPlayerPrivate::buffered() const
{
    RefPtr<TimeRanges> timeRanges = TimeRanges::create();
    float loaded = maxTimeLoaded();
    // rtsp streams are not buffered
    if (!m_isStreaming && loaded > 0)
        timeRanges->add(0, loaded);
    return timeRanges.release();
}

float MediaPlayerPrivate::maxTimeSeekable() const
{
    // infinite duration means live stream
    return !isfinite(duration()) ? 0 : maxTimeLoaded();
}

float MediaPlayerPrivate::maxTimeLoaded() const
{
    if (!m_qtMovie)
        return 0;
    return m_qtMovie->maxTimeLoaded(); 
}

unsigned MediaPlayerPrivate::bytesLoaded() const
{
    if (!m_qtMovie)
        return 0;
    float dur = duration();
    float maxTime = maxTimeLoaded();
    if (!dur)
        return 0;
    return totalBytes() * maxTime / dur;
}

unsigned MediaPlayerPrivate::totalBytes() const
{
    if (!m_qtMovie)
        return 0;
    return m_qtMovie->dataSize();
}

void MediaPlayerPrivate::cancelLoad()
{
    if (m_networkState < MediaPlayer::Loading || m_networkState == MediaPlayer::Loaded)
        return;
    
    tearDownVideoRendering();

    // Cancel the load by destroying the movie.
    m_qtMovie.clear();

    updateStates();
}

void MediaPlayerPrivate::updateStates()
{
    MediaPlayer::NetworkState oldNetworkState = m_networkState;
    MediaPlayer::ReadyState oldReadyState = m_readyState;
  
    long loadState = m_qtMovie ? m_qtMovie->loadState() : QTMovieLoadStateError;

    if (loadState >= QTMovieLoadStateLoaded && m_readyState < MediaPlayer::HaveMetadata) {
        m_qtMovie->disableUnsupportedTracks(m_enabledTrackCount, m_totalTrackCount);
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

bool MediaPlayerPrivate::isReadyForRendering() const
{
    return m_readyState >= MediaPlayer::HaveMetadata && m_player->visible();
}

void MediaPlayerPrivate::sawUnsupportedTracks()
{
    m_qtMovie->setDisabled(true);
    m_hasUnsupportedTracks = true;
    m_player->mediaPlayerClient()->mediaPlayerSawUnsupportedTracks(m_player);
}

void MediaPlayerPrivate::didEnd()
{
    if (m_hasUnsupportedTracks)
        return;

    m_startedPlaying = false;
#if DRAW_FRAME_RATE
    m_timeStoppedPlaying = WTF::currentTime();
#endif
    updateStates();
    m_player->timeChanged();
}

void MediaPlayerPrivate::setSize(const IntSize& size) 
{ 
    if (m_hasUnsupportedTracks || !m_qtMovie || m_size == size)
        return;
    m_size = size;
    m_qtGWorld->setSize(size.width(), size.height());
}

void MediaPlayerPrivate::setVisible(bool visible)
{
    if (m_hasUnsupportedTracks || !m_qtMovie || m_visible == visible)
        return;

    m_qtGWorld->setVisible(visible);
    m_visible = visible;
    if (m_visible) {
        if (isReadyForRendering())
            setUpVideoRendering();
    } else
        tearDownVideoRendering();
}

void MediaPlayerPrivate::paint(GraphicsContext* p, const IntRect& r)
{
#if USE(ACCELERATED_COMPOSITING)
    if (m_qtVideoLayer)
        return;
#endif
    if (p->paintingDisabled() || !m_qtMovie || m_hasUnsupportedTracks)
        return;

    bool usingTempBitmap = false;
    OwnPtr<GraphicsContext::WindowsBitmap> bitmap;
    // FIXME: use LocalWindowsContext.
    HDC hdc = p->getWindowsContext(r);
    if (!hdc) {
        // The graphics context doesn't have an associated HDC so create a temporary
        // bitmap where QTMovieGWorld can draw the frame and we can copy it.
        usingTempBitmap = true;
        bitmap = p->createWindowsBitmap(r.size());
        hdc = bitmap->hdc();

        // FIXME: is this necessary??
        XFORM xform;
        xform.eM11 = 1.0f;
        xform.eM12 = 0.0f;
        xform.eM21 = 0.0f;
        xform.eM22 = 1.0f;
        xform.eDx = -r.x();
        xform.eDy = -r.y();
        SetWorldTransform(hdc, &xform);
    }

    m_qtGWorld->paint(hdc, r.x(), r.y());
    if (usingTempBitmap)
        p->drawWindowsBitmap(bitmap.get(), r.location());
    else
        p->releaseWindowsContext(hdc, r);

    paintCompleted(*p, r);
}

void MediaPlayerPrivate::paintCompleted(GraphicsContext& context, const IntRect& rect)
{
    m_newFrameAvailable = false;

#if DRAW_FRAME_RATE
    if (m_frameCountWhilePlaying > 10) {
        double interval =  m_startedPlaying ? WTF::currentTime() - m_timeStartedPlaying : m_timeStoppedPlaying - m_timeStartedPlaying;
        double frameRate = (m_frameCountWhilePlaying - 1) / interval;
        CGContextRef cgContext = context.platformContext();
        CGRect drawRect = rect;

        char text[8];
        _snprintf(text, sizeof(text), "%1.2f", frameRate);

        static const int fontSize = 25;
        static const int fontCharWidth = 12;
        static const int boxHeight = 25;
        static const int boxBorderWidth = 4;
        drawRect.size.width = boxBorderWidth * 2 + fontCharWidth * strlen(text);
        drawRect.size.height = boxHeight;

        CGContextSaveGState(cgContext);
#if USE(ACCELERATED_COMPOSITING)
        if (m_qtVideoLayer)
            CGContextScaleCTM(cgContext, 1, -1);
        CGContextTranslateCTM(cgContext, rect.width() - drawRect.size.width, m_qtVideoLayer ? -rect.height() : 0);
#else
        CGContextTranslateCTM(cgContext, rect.width() - drawRect.size.width, 0);
#endif
        static const CGFloat backgroundColor[4] = { 0.98, 0.98, 0.82, 0.8 };
        CGContextSetFillColor(cgContext, backgroundColor);
        CGContextFillRect(cgContext, drawRect);

        static const CGFloat textColor[4] = { 0, 0, 0, 1 };
        CGContextSetFillColor(cgContext, textColor);
        CGContextSetTextMatrix(cgContext, CGAffineTransformMakeScale(1, -1));
        CGContextSelectFont(cgContext, "Helvetica", fontSize, kCGEncodingMacRoman);

        CGContextShowTextAtPoint(cgContext, drawRect.origin.x + boxBorderWidth, drawRect.origin.y + boxHeight - boxBorderWidth, text, strlen(text));
        
        CGContextRestoreGState(cgContext);        
    }
#endif
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

void MediaPlayerPrivate::getSupportedTypes(HashSet<String>& types)
{
    types = mimeTypeCache();
} 

bool MediaPlayerPrivate::isAvailable()
{
    return QTMovie::initializeQuickTime();
}

MediaPlayer::SupportsType MediaPlayerPrivate::supportsType(const String& type, const String& codecs)
{
    // only return "IsSupported" if there is no codecs parameter for now as there is no way to ask QT if it supports an
    //  extended MIME type
    return mimeTypeCache().contains(type) ? (codecs.isEmpty() ? MediaPlayer::MayBeSupported : MediaPlayer::IsSupported) : MediaPlayer::IsNotSupported;
}

void MediaPlayerPrivate::movieEnded(QTMovie* movie)
{
    if (m_hasUnsupportedTracks)
        return;

    ASSERT(m_qtMovie.get() == movie);
    didEnd();
}

void MediaPlayerPrivate::movieLoadStateChanged(QTMovie* movie)
{
    if (m_hasUnsupportedTracks)
        return;

    ASSERT(m_qtMovie.get() == movie);
    updateStates();
}

void MediaPlayerPrivate::movieTimeChanged(QTMovie* movie)
{
    if (m_hasUnsupportedTracks)
        return;

    ASSERT(m_qtMovie.get() == movie);
    updateStates();
    m_player->timeChanged();
}

void MediaPlayerPrivate::movieNewImageAvailable(QTMovieGWorld* movie)
{
    if (m_hasUnsupportedTracks)
        return;

    ASSERT(m_qtGWorld.get() == movie);
#if DRAW_FRAME_RATE
    if (m_startedPlaying) {
        m_frameCountWhilePlaying++;
        // To eliminate preroll costs from our calculation, our frame rate calculation excludes
        // the first frame drawn after playback starts.
        if (m_frameCountWhilePlaying == 1)
            m_timeStartedPlaying = WTF::currentTime();
    }
#endif

    m_newFrameAvailable = true;

#if USE(ACCELERATED_COMPOSITING)
    if (m_qtVideoLayer)
        m_qtVideoLayer->setNeedsDisplay();
    else
#endif
        m_player->repaint();
}

bool MediaPlayerPrivate::hasSingleSecurityOrigin() const
{
    // We tell quicktime to disallow resources that come from different origins
    // so we all media is single origin.
    return true;
}

MediaPlayerPrivate::MediaRenderingMode MediaPlayerPrivate::currentRenderingMode() const
{
    if (!m_qtMovie)
        return MediaRenderingNone;

#if USE(ACCELERATED_COMPOSITING)
    if (m_qtVideoLayer)
        return MediaRenderingMovieLayer;
#endif

    return MediaRenderingSoftwareRenderer;
}

MediaPlayerPrivate::MediaRenderingMode MediaPlayerPrivate::preferredRenderingMode() const
{
    if (!m_player->frameView() || !m_qtMovie)
        return MediaRenderingNone;

#if USE(ACCELERATED_COMPOSITING)
    if (supportsAcceleratedRendering() && m_player->mediaPlayerClient()->mediaPlayerRenderingCanBeAccelerated(m_player))
        return MediaRenderingMovieLayer;
#endif

    return MediaRenderingSoftwareRenderer;
}

void MediaPlayerPrivate::setUpVideoRendering()
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
}

void MediaPlayerPrivate::tearDownVideoRendering()
{
#if USE(ACCELERATED_COMPOSITING)
    if (m_qtVideoLayer)
        destroyLayerForMovie();
#endif
}

bool MediaPlayerPrivate::hasSetUpVideoRendering() const
{
#if USE(ACCELERATED_COMPOSITING)
    return m_qtVideoLayer || currentRenderingMode() != MediaRenderingMovieLayer;
#else
    return true;
#endif
}

#if USE(ACCELERATED_COMPOSITING)

// Up-call from compositing layer drawing callback.
void MediaPlayerPrivate::paintContents(const GraphicsLayer*, GraphicsContext& context, GraphicsLayerPaintingPhase, const IntRect&)
{
     if (m_hasUnsupportedTracks)
         return;

    ASSERT(supportsAcceleratedRendering());

    // No reason to replace the current layer image unless we have something new to show.
    if (!m_newFrameAvailable)
        return;

    static CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    void* buffer;
    unsigned bitsPerPixel;
    unsigned rowBytes;
    unsigned width;
    unsigned height;

    m_qtGWorld->getCurrentFrameInfo(buffer, bitsPerPixel, rowBytes, width, height);
    if (!buffer)
        return;

    RetainPtr<CFDataRef> data(AdoptCF, CFDataCreateWithBytesNoCopy(0, static_cast<UInt8*>(buffer), rowBytes * height, kCFAllocatorNull));
    RetainPtr<CGDataProviderRef> provider(AdoptCF, CGDataProviderCreateWithCFData(data.get()));
    RetainPtr<CGImageRef> frameImage(AdoptCF, CGImageCreate(width, height, 8, bitsPerPixel, rowBytes, colorSpace, 
        kCGBitmapByteOrder32Little | kCGImageAlphaFirst, provider.get(), 0, false, kCGRenderingIntentDefault));
    if (!frameImage)
        return;

    IntRect rect(0, 0, m_size.width(), m_size.height());
    CGContextDrawImage(context.platformContext(), rect, frameImage.get()); 
    paintCompleted(context, rect);
}
#endif

void MediaPlayerPrivate::createLayerForMovie()
{
#if USE(ACCELERATED_COMPOSITING)
    ASSERT(supportsAcceleratedRendering());

    if (!m_qtMovie || m_qtVideoLayer)
        return;

    // Create a GraphicsLayer that won't be inserted directly into the render tree, but will used 
    // as a wrapper for a PlatformCALayer which gets inserted as the content layer of the video 
    // renderer's GraphicsLayer.
    m_qtVideoLayer = adoptPtr(new GraphicsLayerCACF(this));
    if (!m_qtVideoLayer)
        return;

    // Mark the layer as drawing itself, anchored in the top left, and bottom-up.
    m_qtVideoLayer->setDrawsContent(true);
    m_qtVideoLayer->setAnchorPoint(FloatPoint3D());
    m_qtVideoLayer->setContentsOrientation(GraphicsLayer::CompositingCoordinatesBottomUp);
#ifndef NDEBUG
    m_qtVideoLayer->setName("Video layer");
#endif
    // The layer will get hooked up via RenderLayerBacking::updateGraphicsLayerConfiguration().
#endif
}

void MediaPlayerPrivate::destroyLayerForMovie()
{
#if USE(ACCELERATED_COMPOSITING)
    if (!m_qtVideoLayer)
        return;
    m_qtVideoLayer = nullptr;
#endif
}

#if USE(ACCELERATED_COMPOSITING)
bool MediaPlayerPrivate::supportsAcceleratedRendering() const
{
    return isReadyForRendering();
}

void MediaPlayerPrivate::acceleratedRenderingStateChanged()
{
    // Set up or change the rendering path if necessary.
    setUpVideoRendering();
}

#endif


}

#endif
