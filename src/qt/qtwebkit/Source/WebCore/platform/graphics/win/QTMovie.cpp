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

#include "QTMovie.h"

#include "QTMovieTask.h"
#include "QTMovieWinTimer.h"
#include <FixMath.h>
#include <GXMath.h>
#include <Movies.h>
#include <QTML.h>
#include <QuickTimeComponents.h>
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#include <wtf/Assertions.h>
#include <wtf/MathExtras.h>
#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>

using namespace std;

static const long minimumQuickTimeVersion = 0x07300000; // 7.3

static const long closedCaptionTrackType = 'clcp';
static const long subTitleTrackType = 'sbtl';
static const long mpeg4ObjectDescriptionTrackType = 'odsm';
static const long mpeg4SceneDescriptionTrackType = 'sdsm';
static const long closedCaptionDisplayPropertyID = 'disp';

// Resizing GWorlds is slow, give them a minimum size so size of small 
// videos can be animated smoothly
static const int cGWorldMinWidth = 640;
static const int cGWorldMinHeight = 360;

static const float cNonContinuousTimeChange = 0.2f;

union UppParam {
    long longValue;
    void* ptr;
};

static CFMutableArrayRef gSupportedTypes = 0;
static SInt32 quickTimeVersion = 0;

class QTMoviePrivate : public QTMovieTaskClient {
    WTF_MAKE_NONCOPYABLE(QTMoviePrivate);
public:
    QTMoviePrivate();
    ~QTMoviePrivate();
    void task();
    void startTask();
    void endTask();

    void createMovieController();
    void cacheMovieScale();

    QTMovie* m_movieWin;
    Movie m_movie;
    MovieController m_movieController;
    bool m_tasking;
    bool m_disabled;
    Vector<QTMovieClient*> m_clients;
    long m_loadState;
    bool m_ended;
    bool m_seeking;
    float m_lastMediaTime;
    double m_lastLoadStateCheckTime;
    int m_width;
    int m_height;
    bool m_visible;
    long m_loadError;
    float m_widthScaleFactor;
    float m_heightScaleFactor;
    CFURLRef m_currentURL;
    float m_timeToRestore;
    float m_rateToRestore;
    bool m_privateBrowsing;
#if !ASSERT_DISABLED
    bool m_scaleCached;
#endif
};

QTMoviePrivate::QTMoviePrivate()
    : m_movieWin(0)
    , m_movie(0)
    , m_movieController(0)
    , m_tasking(false)
    , m_loadState(0)
    , m_ended(false)
    , m_seeking(false)
    , m_lastMediaTime(0)
    , m_lastLoadStateCheckTime(0)
    , m_width(0)
    , m_height(0)
    , m_visible(false)
    , m_loadError(0)
    , m_widthScaleFactor(1)
    , m_heightScaleFactor(1)
    , m_currentURL(0)
    , m_timeToRestore(-1.0f)
    , m_rateToRestore(-1.0f)
    , m_disabled(false)
    , m_privateBrowsing(false)
#if !ASSERT_DISABLED
    , m_scaleCached(false)
#endif
{
}

QTMoviePrivate::~QTMoviePrivate()
{
    endTask();
    if (m_movieController)
        DisposeMovieController(m_movieController);
    if (m_movie)
        DisposeMovie(m_movie);
    if (m_currentURL)
        CFRelease(m_currentURL);
}

void QTMoviePrivate::startTask() 
{
    if (!m_tasking) {
        QTMovieTask::sharedTask()->addTaskClient(this);
        m_tasking = true;
    }
    QTMovieTask::sharedTask()->updateTaskTimer();
}

void QTMoviePrivate::endTask() 
{
    if (m_tasking) {
        QTMovieTask::sharedTask()->removeTaskClient(this);
        m_tasking = false;
    }
    QTMovieTask::sharedTask()->updateTaskTimer();
}

void QTMoviePrivate::task() 
{
    ASSERT(m_tasking);

    if (!m_loadError) {
        if (m_movieController)
            MCIdle(m_movieController);
        else
            MoviesTask(m_movie, 0);
    }

    // GetMovieLoadState documentation says that you should not call it more often than every quarter of a second.
    if (systemTime() >= m_lastLoadStateCheckTime + 0.25 || m_loadError) { 
        // If load fails QT's load state is QTMovieLoadStateComplete.
        // This is different from QTKit API and seems strange.
        long loadState = m_loadError ? QTMovieLoadStateError : GetMovieLoadState(m_movie);
        if (loadState != m_loadState) {
            // we only need to erase the movie gworld when the load state changes to loaded while it
            //  is visible as the gworld is destroyed/created when visibility changes
            bool shouldRestorePlaybackState = false;
            bool movieNewlyPlayable = loadState >= QTMovieLoadStateLoaded && m_loadState < QTMovieLoadStateLoaded;
            m_loadState = loadState;
            if (movieNewlyPlayable) {
                cacheMovieScale();
                shouldRestorePlaybackState = true;
            }

            if (!m_movieController && m_loadState >= QTMovieLoadStateLoaded)
                createMovieController();

            for (size_t i = 0; i < m_clients.size(); ++i)
                m_clients[i]->movieLoadStateChanged(m_movieWin);
            
            if (shouldRestorePlaybackState && m_timeToRestore != -1.0f) {
                m_movieWin->setCurrentTime(m_timeToRestore);
                m_timeToRestore = -1.0f;
                m_movieWin->setRate(m_rateToRestore);
                m_rateToRestore = -1.0f;
            }

            if (m_disabled) {
                endTask();
                return;
            }
        }
        m_lastLoadStateCheckTime = systemTime();
    }

    bool ended = !!IsMovieDone(m_movie);
    if (ended != m_ended) {
        m_ended = ended;
        if (ended) {
            for (size_t i = 0; i < m_clients.size(); ++i)
               m_clients[i]->movieEnded(m_movieWin);
        }
    }

    float time = m_movieWin->currentTime();
    if (time < m_lastMediaTime || time >= m_lastMediaTime + cNonContinuousTimeChange || m_seeking) {
        m_seeking = false;
        for (size_t i = 0; i < m_clients.size(); ++i)
            m_clients[i]->movieTimeChanged(m_movieWin);
    }
    m_lastMediaTime = time;

    if (m_loadError)
        endTask();
    else
        QTMovieTask::sharedTask()->updateTaskTimer();
}

void QTMoviePrivate::createMovieController()
{
    Rect bounds;
    long flags;

    if (!m_movie)
        return;

    if (m_movieController)
        DisposeMovieController(m_movieController);

    GetMovieBox(m_movie, &bounds);
    flags = mcTopLeftMovie | mcNotVisible;
    m_movieController = NewMovieController(m_movie, &bounds, flags);
    if (!m_movieController)
        return;

    // Disable automatic looping.
    MCDoAction(m_movieController, mcActionSetLooping, 0);
}

void QTMoviePrivate::cacheMovieScale()
{
    Rect naturalRect;
    Rect initialRect;

    GetMovieNaturalBoundsRect(m_movie, &naturalRect);
    GetMovieBox(m_movie, &initialRect);

    float naturalWidth = naturalRect.right - naturalRect.left;
    float naturalHeight = naturalRect.bottom - naturalRect.top;

    if (naturalWidth)
        m_widthScaleFactor = (initialRect.right - initialRect.left) / naturalWidth;
    if (naturalHeight)
        m_heightScaleFactor = (initialRect.bottom - initialRect.top) / naturalHeight;
#if !ASSERT_DISABLED
    m_scaleCached = true;
#endif
}

QTMovie::QTMovie(QTMovieClient* client)
    : m_private(new QTMoviePrivate())
{
    m_private->m_movieWin = this;
    if (client)
        m_private->m_clients.append(client);
    initializeQuickTime();
}

QTMovie::~QTMovie()
{
    delete m_private;
}

void QTMovie::disableComponent(uint32_t cd[5])
{
    ComponentDescription nullDesc = {'null', 'base', kAppleManufacturer, 0, 0};
    Component nullComp = FindNextComponent(0, &nullDesc);
    Component disabledComp = 0;

    while (disabledComp = FindNextComponent(disabledComp, (ComponentDescription*)&cd[0]))
        CaptureComponent(disabledComp, nullComp);
}

void QTMovie::addClient(QTMovieClient* client)
{
    if (client)
        m_private->m_clients.append(client);
}

void QTMovie::removeClient(QTMovieClient* client)
{
    size_t indexOfClient = m_private->m_clients.find(client);
    if (indexOfClient != notFound)
        m_private->m_clients.remove(indexOfClient);
}

void QTMovie::play()
{
    m_private->m_timeToRestore = -1.0f;

    if (m_private->m_movieController)
        MCDoAction(m_private->m_movieController, mcActionPrerollAndPlay, (void *)GetMoviePreferredRate(m_private->m_movie));
    else
        StartMovie(m_private->m_movie);
    m_private->startTask();
}

void QTMovie::pause()
{
    m_private->m_timeToRestore = -1.0f;

    if (m_private->m_movieController)
        MCDoAction(m_private->m_movieController, mcActionPlay, 0);
    else
        StopMovie(m_private->m_movie);
    QTMovieTask::sharedTask()->updateTaskTimer();
}

float QTMovie::rate() const
{
    if (!m_private->m_movie)
        return 0;
    return FixedToFloat(GetMovieRate(m_private->m_movie));
}

void QTMovie::setRate(float rate)
{
    if (!m_private->m_movie)
        return;    
    m_private->m_timeToRestore = -1.0f;
    
    if (m_private->m_movieController)
        MCDoAction(m_private->m_movieController, mcActionPrerollAndPlay, (void *)FloatToFixed(rate));
    else
        SetMovieRate(m_private->m_movie, FloatToFixed(rate));
    QTMovieTask::sharedTask()->updateTaskTimer();
}

float QTMovie::duration() const
{
    if (!m_private->m_movie)
        return 0;
    TimeValue val = GetMovieDuration(m_private->m_movie);
    TimeScale scale = GetMovieTimeScale(m_private->m_movie);
    return static_cast<float>(val) / scale;
}

float QTMovie::currentTime() const
{
    if (!m_private->m_movie)
        return 0;
    TimeValue val = GetMovieTime(m_private->m_movie, 0);
    TimeScale scale = GetMovieTimeScale(m_private->m_movie);
    return static_cast<float>(val) / scale;
}

void QTMovie::setCurrentTime(float time) const
{
    if (!m_private->m_movie)
        return;

    m_private->m_timeToRestore = -1.0f;
    
    m_private->m_seeking = true;
    TimeScale scale = GetMovieTimeScale(m_private->m_movie);
    if (m_private->m_movieController) {
        QTRestartAtTimeRecord restart = { lroundf(time * scale) , 0 };
        MCDoAction(m_private->m_movieController, mcActionRestartAtTime, (void *)&restart);
    } else
        SetMovieTimeValue(m_private->m_movie, TimeValue(lroundf(time * scale)));
    QTMovieTask::sharedTask()->updateTaskTimer();
}

void QTMovie::setVolume(float volume)
{
    if (!m_private->m_movie)
        return;
    SetMovieVolume(m_private->m_movie, static_cast<short>(volume * 256));
}

void QTMovie::setPreservesPitch(bool preservesPitch)
{
    if (!m_private->m_movie || !m_private->m_currentURL)
        return;

    OSErr error;
    bool prop = false;

    error = QTGetMovieProperty(m_private->m_movie, kQTPropertyClass_Audio, kQTAudioPropertyID_RateChangesPreservePitch,
                               sizeof(kQTAudioPropertyID_RateChangesPreservePitch), static_cast<QTPropertyValuePtr>(&prop), 0);

    if (error || prop == preservesPitch)
        return;

    m_private->m_timeToRestore = currentTime();
    m_private->m_rateToRestore = rate();
    load(m_private->m_currentURL, preservesPitch);
}

unsigned QTMovie::dataSize() const
{
    if (!m_private->m_movie)
        return 0;
    return GetMovieDataSize(m_private->m_movie, 0, GetMovieDuration(m_private->m_movie));
}

float QTMovie::maxTimeLoaded() const
{
    if (!m_private->m_movie)
        return 0;
    TimeValue val;
    GetMaxLoadedTimeInMovie(m_private->m_movie, &val);
    TimeScale scale = GetMovieTimeScale(m_private->m_movie);
    return static_cast<float>(val) / scale;
}

long QTMovie::loadState() const
{
    return m_private->m_loadState;
}

void QTMovie::getNaturalSize(int& width, int& height)
{
    Rect rect = { 0, };

    if (m_private->m_movie)
        GetMovieNaturalBoundsRect(m_private->m_movie, &rect);
    width = (rect.right - rect.left) * m_private->m_widthScaleFactor;
    height = (rect.bottom - rect.top) * m_private->m_heightScaleFactor;
}

void QTMovie::loadPath(const UChar* url, int len, bool preservesPitch)
{
    CFStringRef urlStringRef = CFStringCreateWithCharacters(kCFAllocatorDefault, reinterpret_cast<const UniChar*>(url), len);
    CFURLRef cfURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, urlStringRef, kCFURLWindowsPathStyle, false);

    load(cfURL, preservesPitch);

    if (cfURL)
        CFRelease(cfURL);
    if (urlStringRef)
        CFRelease(urlStringRef);
}

void QTMovie::load(const UChar* url, int len, bool preservesPitch)
{
    CFStringRef urlStringRef = CFStringCreateWithCharacters(kCFAllocatorDefault, reinterpret_cast<const UniChar*>(url), len);
    CFURLRef cfURL = CFURLCreateWithString(kCFAllocatorDefault, urlStringRef, 0);

    load(cfURL, preservesPitch);

    if (cfURL)
        CFRelease(cfURL);
    if (urlStringRef)
        CFRelease(urlStringRef);
}

void QTMovie::load(CFURLRef url, bool preservesPitch)
{
    if (!url)
        return;

    if (m_private->m_movie) {
        m_private->endTask();
        if (m_private->m_movieController)
            DisposeMovieController(m_private->m_movieController);
        m_private->m_movieController = 0;
        DisposeMovie(m_private->m_movie);
        m_private->m_movie = 0;
        m_private->m_loadState = 0;
    }  

    // Define a property array for NewMovieFromProperties.
    QTNewMoviePropertyElement movieProps[9]; 
    ItemCount moviePropCount = 0; 

    bool boolTrue = true;
    
    // Disable streaming support for now. 
    CFStringRef scheme = CFURLCopyScheme(url);
    bool isRTSP = CFStringHasPrefix(scheme, CFSTR("rtsp:"));
    CFRelease(scheme);

    if (isRTSP) {
        m_private->m_loadError = noMovieFound;
        goto end;
    }

    if (m_private->m_currentURL) {
        if (m_private->m_currentURL != url) {
            CFRelease(m_private->m_currentURL);
            m_private->m_currentURL = url;
            CFRetain(url);
        }
    } else {
        m_private->m_currentURL = url;
        CFRetain(url);
    }

    // Add the movie data location to the property array 
    movieProps[moviePropCount].propClass = kQTPropertyClass_DataLocation; 
    movieProps[moviePropCount].propID = kQTDataLocationPropertyID_CFURL; 
    movieProps[moviePropCount].propValueSize = sizeof(m_private->m_currentURL); 
    movieProps[moviePropCount].propValueAddress = &(m_private->m_currentURL); 
    movieProps[moviePropCount].propStatus = 0; 
    moviePropCount++; 

    movieProps[moviePropCount].propClass = kQTPropertyClass_MovieInstantiation; 
    movieProps[moviePropCount].propID = kQTMovieInstantiationPropertyID_DontAskUnresolvedDataRefs; 
    movieProps[moviePropCount].propValueSize = sizeof(boolTrue); 
    movieProps[moviePropCount].propValueAddress = &boolTrue; 
    movieProps[moviePropCount].propStatus = 0; 
    moviePropCount++; 

    movieProps[moviePropCount].propClass = kQTPropertyClass_MovieInstantiation; 
    movieProps[moviePropCount].propID = kQTMovieInstantiationPropertyID_AsyncOK; 
    movieProps[moviePropCount].propValueSize = sizeof(boolTrue); 
    movieProps[moviePropCount].propValueAddress = &boolTrue; 
    movieProps[moviePropCount].propStatus = 0; 
    moviePropCount++; 

    movieProps[moviePropCount].propClass = kQTPropertyClass_NewMovieProperty; 
    movieProps[moviePropCount].propID = kQTNewMoviePropertyID_Active; 
    movieProps[moviePropCount].propValueSize = sizeof(boolTrue); 
    movieProps[moviePropCount].propValueAddress = &boolTrue; 
    movieProps[moviePropCount].propStatus = 0; 
    moviePropCount++; 

    movieProps[moviePropCount].propClass = kQTPropertyClass_NewMovieProperty; 
    movieProps[moviePropCount].propID = kQTNewMoviePropertyID_DontInteractWithUser; 
    movieProps[moviePropCount].propValueSize = sizeof(boolTrue); 
    movieProps[moviePropCount].propValueAddress = &boolTrue; 
    movieProps[moviePropCount].propStatus = 0; 
    moviePropCount++; 

    movieProps[moviePropCount].propClass = kQTPropertyClass_MovieInstantiation;
    movieProps[moviePropCount].propID = '!url';
    movieProps[moviePropCount].propValueSize = sizeof(boolTrue); 
    movieProps[moviePropCount].propValueAddress = &boolTrue; 
    movieProps[moviePropCount].propStatus = 0; 
    moviePropCount++; 

    movieProps[moviePropCount].propClass = kQTPropertyClass_MovieInstantiation; 
    movieProps[moviePropCount].propID = 'site';
    movieProps[moviePropCount].propValueSize = sizeof(boolTrue); 
    movieProps[moviePropCount].propValueAddress = &boolTrue; 
    movieProps[moviePropCount].propStatus = 0; 
    moviePropCount++;

    movieProps[moviePropCount].propClass = kQTPropertyClass_Audio; 
    movieProps[moviePropCount].propID = kQTAudioPropertyID_RateChangesPreservePitch;
    movieProps[moviePropCount].propValueSize = sizeof(preservesPitch); 
    movieProps[moviePropCount].propValueAddress = &preservesPitch; 
    movieProps[moviePropCount].propStatus = 0; 
    moviePropCount++; 

    bool allowCaching = !m_private->m_privateBrowsing;
    movieProps[moviePropCount].propClass = kQTPropertyClass_MovieInstantiation; 
    movieProps[moviePropCount].propID = 'pers';
    movieProps[moviePropCount].propValueSize = sizeof(allowCaching); 
    movieProps[moviePropCount].propValueAddress = &allowCaching; 
    movieProps[moviePropCount].propStatus = 0; 
    moviePropCount++;

    ASSERT(moviePropCount <= WTF_ARRAY_LENGTH(movieProps));
    m_private->m_loadError = NewMovieFromProperties(moviePropCount, movieProps, 0, 0, &m_private->m_movie);

end:
    m_private->startTask();
    // get the load fail callback quickly 
    if (m_private->m_loadError)
        QTMovieTask::sharedTask()->updateTaskTimer(0);
    else {
        OSType mode = kQTApertureMode_CleanAperture;

        // Set the aperture mode property on a movie to signal that we want aspect ratio
        // and clean aperture dimensions. Don't worry about errors, we can't do anything if
        // the installed version of QT doesn't support it and it isn't serious enough to 
        // warrant failing.
        QTSetMovieProperty(m_private->m_movie, kQTPropertyClass_Visual, kQTVisualPropertyID_ApertureMode, sizeof(mode), &mode);
    }
}

void QTMovie::disableUnsupportedTracks(unsigned& enabledTrackCount, unsigned& totalTrackCount)
{
    if (!m_private->m_movie) {
        totalTrackCount = 0;
        enabledTrackCount = 0;
        return;
    }

    static HashSet<OSType>* allowedTrackTypes = 0;
    if (!allowedTrackTypes) {
        allowedTrackTypes = new HashSet<OSType>;
        allowedTrackTypes->add(VideoMediaType);
        allowedTrackTypes->add(SoundMediaType);
        allowedTrackTypes->add(TextMediaType);
        allowedTrackTypes->add(BaseMediaType);
        allowedTrackTypes->add(closedCaptionTrackType);
        allowedTrackTypes->add(subTitleTrackType);
        allowedTrackTypes->add(mpeg4ObjectDescriptionTrackType);
        allowedTrackTypes->add(mpeg4SceneDescriptionTrackType);
        allowedTrackTypes->add(TimeCodeMediaType);
        allowedTrackTypes->add(TimeCode64MediaType);
    }

    long trackCount = GetMovieTrackCount(m_private->m_movie);
    enabledTrackCount = trackCount;
    totalTrackCount = trackCount;

    // Track indexes are 1-based. yuck. These things must descend from old-
    // school mac resources or something.
    for (long trackIndex = 1; trackIndex <= trackCount; trackIndex++) {
        // Grab the track at the current index. If there isn't one there, then
        // we can move onto the next one.
        Track currentTrack = GetMovieIndTrack(m_private->m_movie, trackIndex);
        if (!currentTrack)
            continue;
        
        // Check to see if the track is disabled already, we should move along.
        // We don't need to re-disable it.
        if (!GetTrackEnabled(currentTrack))
            continue;

        // Grab the track's media. We're going to check to see if we need to
        // disable the tracks. They could be unsupported.
        Media trackMedia = GetTrackMedia(currentTrack);
        if (!trackMedia)
            continue;
        
        // Grab the media type for this track. Make sure that we don't
        // get an error in doing so. If we do, then something really funky is
        // wrong.
        OSType mediaType;
        GetMediaHandlerDescription(trackMedia, &mediaType, nil, nil);
        OSErr mediaErr = GetMoviesError();    
        if (mediaErr != noErr)
            continue;
        
        if (!allowedTrackTypes->contains(mediaType)) {

            // Different mpeg variants import as different track types so check for the "mpeg 
            // characteristic" instead of hard coding the (current) list of mpeg media types.
            if (GetMovieIndTrackType(m_private->m_movie, 1, 'mpeg', movieTrackCharacteristic | movieTrackEnabledOnly))
                continue;

            SetTrackEnabled(currentTrack, false);
            --enabledTrackCount;
        }
        
        // Grab the track reference count for chapters. This will tell us if it
        // has chapter tracks in it. If there aren't any references, then we
        // can move on the next track.
        long referenceCount = GetTrackReferenceCount(currentTrack, kTrackReferenceChapterList);
        if (referenceCount <= 0)
            continue;
        
        long referenceIndex = 0;        
        while (1) {
            // If we get nothing here, we've overstepped our bounds and can stop
            // looking. Chapter indices here are 1-based as well - hence, the
            // pre-increment.
            referenceIndex++;
            Track chapterTrack = GetTrackReference(currentTrack, kTrackReferenceChapterList, referenceIndex);
            if (!chapterTrack)
                break;
            
            // Try to grab the media for the track.
            Media chapterMedia = GetTrackMedia(chapterTrack);
            if (!chapterMedia)
                continue;
        
            // Grab the media type for this track. Make sure that we don't
            // get an error in doing so. If we do, then something really
            // funky is wrong.
            OSType mediaType;
            GetMediaHandlerDescription(chapterMedia, &mediaType, nil, nil);
            OSErr mediaErr = GetMoviesError();
            if (mediaErr != noErr)
                continue;
            
            // Check to see if the track is a video track. We don't care about
            // other non-video tracks.
            if (mediaType != VideoMediaType)
                continue;
            
            // Check to see if the track is already disabled. If it is, we
            // should move along.
            if (!GetTrackEnabled(chapterTrack))
                continue;
            
            // Disabled the evil, evil track.
            SetTrackEnabled(chapterTrack, false);
            --enabledTrackCount;
        }
    }
}

bool QTMovie::isDisabled() const
{
    return m_private->m_disabled;
}

void QTMovie::setDisabled(bool b)
{
    m_private->m_disabled = b;
}


bool QTMovie::hasVideo() const
{
    if (!m_private->m_movie)
        return false;
    return (GetMovieIndTrackType(m_private->m_movie, 1, VisualMediaCharacteristic, movieTrackCharacteristic | movieTrackEnabledOnly));
}

bool QTMovie::hasAudio() const
{
    if (!m_private->m_movie)
        return false;
    return (GetMovieIndTrackType(m_private->m_movie, 1, AudioMediaCharacteristic, movieTrackCharacteristic | movieTrackEnabledOnly));
}

QTTrackArray QTMovie::videoTracks() const
{
    QTTrackArray tracks;
    long trackIndex = 1;

    while (Track theTrack = GetMovieIndTrackType(m_private->m_movie, trackIndex++, VisualMediaCharacteristic, movieTrackCharacteristic | movieTrackEnabledOnly))
        tracks.append(QTTrack::create(theTrack));

    return tracks;
}

bool QTMovie::hasClosedCaptions() const 
{
    if (!m_private->m_movie)
        return false;
    return GetMovieIndTrackType(m_private->m_movie, 1, closedCaptionTrackType, movieTrackMediaType);
}

void QTMovie::setClosedCaptionsVisible(bool visible)
{
    if (!m_private->m_movie)
        return;

    Track ccTrack = GetMovieIndTrackType(m_private->m_movie, 1, closedCaptionTrackType, movieTrackMediaType);
    if (!ccTrack)
        return;

    Boolean doDisplay = visible;
    QTSetTrackProperty(ccTrack, closedCaptionTrackType, closedCaptionDisplayPropertyID, sizeof(doDisplay), &doDisplay);
}

long QTMovie::timeScale() const 
{
    if (!m_private->m_movie)
        return 0;

    return GetMovieTimeScale(m_private->m_movie);
}

static void getMIMETypeCallBack(const char* type);

static void initializeSupportedTypes() 
{
    if (gSupportedTypes)
        return;

    gSupportedTypes = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
    if (quickTimeVersion < minimumQuickTimeVersion) {
        LOG_ERROR("QuickTime version %x detected, at least %x required. Returning empty list of supported media MIME types.", quickTimeVersion, minimumQuickTimeVersion);
        return;
    }

    // QuickTime doesn't have an importer for video/quicktime. Add it manually.
    CFArrayAppendValue(gSupportedTypes, CFSTR("video/quicktime"));
    
    wkGetQuickTimeMIMETypeList(getMIMETypeCallBack);
}

static void getMIMETypeCallBack(const char* type)
{
    ASSERT(type);
    CFStringRef cfType = CFStringCreateWithCString(kCFAllocatorDefault, type, kCFStringEncodingMacRoman);
    if (!cfType)
        return;

    // Filter out all non-audio or -video MIME Types, and only add each type once:
    if (CFStringHasPrefix(cfType, CFSTR("audio/")) || CFStringHasPrefix(cfType, CFSTR("video/"))) {
        CFRange range = CFRangeMake(0, CFArrayGetCount(gSupportedTypes));
        if (!CFArrayContainsValue(gSupportedTypes, range, cfType))
            CFArrayAppendValue(gSupportedTypes, cfType);
    }

    CFRelease(cfType);
}

unsigned QTMovie::countSupportedTypes()
{
    initializeSupportedTypes();
    return static_cast<unsigned>(CFArrayGetCount(gSupportedTypes));
}

void QTMovie::getSupportedType(unsigned index, const UChar*& str, unsigned& len)
{
    initializeSupportedTypes();
    ASSERT(index < CFArrayGetCount(gSupportedTypes));

    // Allocate sufficient buffer to hold any MIME type
    static UniChar* staticBuffer = 0;
    if (!staticBuffer)
        staticBuffer = new UniChar[32];

    CFStringRef cfstr = (CFStringRef)CFArrayGetValueAtIndex(gSupportedTypes, index);
    len = CFStringGetLength(cfstr);
    CFRange range = { 0, len };
    CFStringGetCharacters(cfstr, range, staticBuffer);
    str = reinterpret_cast<const UChar*>(staticBuffer);
    
}

CGAffineTransform QTMovie::getTransform() const
{
    ASSERT(m_private->m_movie);
    MatrixRecord m = {0};
    GetMovieMatrix(m_private->m_movie, &m);

    ASSERT(!m.matrix[0][2]);
    ASSERT(!m.matrix[1][2]);
    CGAffineTransform transform = CGAffineTransformMake(
        Fix2X(m.matrix[0][0]),
        Fix2X(m.matrix[0][1]),
        Fix2X(m.matrix[1][0]),
        Fix2X(m.matrix[1][1]),
        Fix2X(m.matrix[2][0]),
        Fix2X(m.matrix[2][1]));
    return transform;
}

void QTMovie::setTransform(CGAffineTransform t)
{
    ASSERT(m_private->m_movie);
    MatrixRecord m = {{
        {X2Fix(t.a), X2Fix(t.b), 0},
        {X2Fix(t.c), X2Fix(t.d), 0},
        {X2Fix(t.tx), X2Fix(t.ty), fract1},
    }};

    SetMovieMatrix(m_private->m_movie, &m);
    m_private->cacheMovieScale();
}

void QTMovie::resetTransform()
{
    ASSERT(m_private->m_movie);
    SetMovieMatrix(m_private->m_movie, 0);
    m_private->cacheMovieScale();
}

void QTMovie::setPrivateBrowsingMode(bool privateBrowsing)
{
    m_private->m_privateBrowsing = privateBrowsing;
    if (m_private->m_movie) {
        bool allowCaching = !m_private->m_privateBrowsing;
        QTSetMovieProperty(m_private->m_movie, 'cach', 'pers', sizeof(allowCaching), &allowCaching);
    }
}

bool QTMovie::initializeQuickTime() 
{
    static bool initialized = false;
    static bool initializationSucceeded = false;
    if (!initialized) {
        initialized = true;
        // Initialize and check QuickTime version
        OSErr result = InitializeQTML(kInitializeQTMLEnableDoubleBufferedSurface);
        if (result == noErr)
            result = Gestalt(gestaltQuickTime, &quickTimeVersion);
        if (result != noErr) {
            LOG_ERROR("No QuickTime available. Disabling <video> and <audio> support.");
            return false;
        }
        if (quickTimeVersion < minimumQuickTimeVersion) {
            LOG_ERROR("QuickTime version %x detected, at least %x required. Disabling <video> and <audio> support.", quickTimeVersion, minimumQuickTimeVersion);
            return false;
        }
        EnterMovies();
        initializationSucceeded = true;
    }
    return initializationSucceeded;
}

Movie QTMovie::getMovieHandle() const 
{
    return m_private->m_movie;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        return TRUE;
    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        return FALSE;
    }
    ASSERT_NOT_REACHED();
    return FALSE;
}
