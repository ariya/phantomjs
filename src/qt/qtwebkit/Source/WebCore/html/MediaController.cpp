/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "MediaController.h"

#include "Clock.h"
#include "ExceptionCode.h"
#include "HTMLMediaElement.h"
#include "TimeRanges.h"
#include <wtf/CurrentTime.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/AtomicString.h>

using namespace WebCore;
using namespace std;

PassRefPtr<MediaController> MediaController::create(ScriptExecutionContext* context)
{
    return adoptRef(new MediaController(context));
}

MediaController::MediaController(ScriptExecutionContext* context)
    : m_paused(false)
    , m_defaultPlaybackRate(1)
    , m_volume(1)
    , m_position(MediaPlayer::invalidTime())
    , m_muted(false)
    , m_readyState(HAVE_NOTHING)
    , m_playbackState(WAITING)
    , m_asyncEventTimer(this, &MediaController::asyncEventTimerFired)
    , m_clearPositionTimer(this, &MediaController::clearPositionTimerFired)
    , m_closedCaptionsVisible(false)
    , m_clock(Clock::create())
    , m_scriptExecutionContext(context)
    , m_timeupdateTimer(this, &MediaController::timeupdateTimerFired)
    , m_previousTimeupdateTime(0)
{
}

MediaController::~MediaController()
{
}

void MediaController::addMediaElement(HTMLMediaElement* element)
{
    ASSERT(element);
    ASSERT(!m_mediaElements.contains(element));

    m_mediaElements.append(element);
    bringElementUpToSpeed(element);
}

void MediaController::removeMediaElement(HTMLMediaElement* element)
{
    ASSERT(element);
    ASSERT(m_mediaElements.contains(element));
    m_mediaElements.remove(m_mediaElements.find(element));
}

bool MediaController::containsMediaElement(HTMLMediaElement* element) const
{
    return m_mediaElements.contains(element);
}

PassRefPtr<TimeRanges> MediaController::buffered() const
{
    if (m_mediaElements.isEmpty())
        return TimeRanges::create();

    // The buffered attribute must return a new static normalized TimeRanges object that represents 
    // the intersection of the ranges of the media resources of the slaved media elements that the 
    // user agent has buffered, at the time the attribute is evaluated.
    RefPtr<TimeRanges> bufferedRanges = m_mediaElements.first()->buffered();
    for (size_t index = 1; index < m_mediaElements.size(); ++index)
        bufferedRanges->intersectWith(m_mediaElements[index]->buffered().get());
    return bufferedRanges;
}

PassRefPtr<TimeRanges> MediaController::seekable() const
{
    if (m_mediaElements.isEmpty())
        return TimeRanges::create();

    // The seekable attribute must return a new static normalized TimeRanges object that represents
    // the intersection of the ranges of the media resources of the slaved media elements that the
    // user agent is able to seek to, at the time the attribute is evaluated.
    RefPtr<TimeRanges> seekableRanges = m_mediaElements.first()->seekable();
    for (size_t index = 1; index < m_mediaElements.size(); ++index)
        seekableRanges->intersectWith(m_mediaElements[index]->seekable().get());
    return seekableRanges;
}

PassRefPtr<TimeRanges> MediaController::played()
{
    if (m_mediaElements.isEmpty())
        return TimeRanges::create();

    // The played attribute must return a new static normalized TimeRanges object that represents 
    // the union of the ranges of the media resources of the slaved media elements that the 
    // user agent has so far rendered, at the time the attribute is evaluated.
    RefPtr<TimeRanges> playedRanges = m_mediaElements.first()->played();
    for (size_t index = 1; index < m_mediaElements.size(); ++index)
        playedRanges->unionWith(m_mediaElements[index]->played().get());
    return playedRanges;
}

double MediaController::duration() const
{
    // FIXME: Investigate caching the maximum duration and only updating the cached value
    // when the slaved media elements' durations change.
    double maxDuration = 0;
    for (size_t index = 0; index < m_mediaElements.size(); ++index) {
        double duration = m_mediaElements[index]->duration();
        if (std::isnan(duration))
            continue;
        maxDuration = max(maxDuration, duration);
    }
    return maxDuration;
}

double MediaController::currentTime() const
{
    if (m_mediaElements.isEmpty())
        return 0;

    if (m_position == MediaPlayer::invalidTime()) {
        // Some clocks may return times outside the range of [0..duration].
        m_position = max(0.0, min(duration(), m_clock->currentTime()));
        m_clearPositionTimer.startOneShot(0);
    }

    return m_position;
}

void MediaController::setCurrentTime(double time, ExceptionCode& code)
{
    // When the user agent is to seek the media controller to a particular new playback position, 
    // it must follow these steps:
    // If the new playback position is less than zero, then set it to zero.
    time = max(0.0, time);
    
    // If the new playback position is greater than the media controller duration, then set it 
    // to the media controller duration.
    time = min(time, duration());
    
    // Set the media controller position to the new playback position.
    m_clock->setCurrentTime(time);
    
    // Seek each slaved media element to the new playback position relative to the media element timeline.
    for (size_t index = 0; index < m_mediaElements.size(); ++index)
        m_mediaElements[index]->seek(time, code);

    scheduleTimeupdateEvent();
}

void MediaController::unpause()
{
    // When the unpause() method is invoked, if the MediaController is a paused media controller,
    if (!m_paused)
        return;
    // the user agent must change the MediaController into a playing media controller,
    m_paused = false;
    // queue a task to fire a simple event named play at the MediaController,
    scheduleEvent(eventNames().playEvent);
    // and then report the controller state of the MediaController.
    reportControllerState();
}

void MediaController::play()
{
    // When the play() method is invoked, the user agent must invoke the play method of each
    // slaved media element in turn,
    for (size_t index = 0; index < m_mediaElements.size(); ++index)
        m_mediaElements[index]->play();

    // and then invoke the unpause method of the MediaController.
    unpause();
}

void MediaController::pause()
{
    // When the pause() method is invoked, if the MediaController is a playing media controller,
    if (m_paused)
        return;

    // then the user agent must change the MediaController into a paused media controller,
    m_paused = true;
    // queue a task to fire a simple event named pause at the MediaController,
    scheduleEvent(eventNames().pauseEvent);
    // and then report the controller state of the MediaController.
    reportControllerState();
}

void MediaController::setDefaultPlaybackRate(double rate)
{
    if (m_defaultPlaybackRate == rate)
        return;

    // The defaultPlaybackRate attribute, on setting, must set the MediaController's media controller
    // default playback rate to the new value,
    m_defaultPlaybackRate = rate;

    // then queue a task to fire a simple event named ratechange at the MediaController.
    scheduleEvent(eventNames().ratechangeEvent);
}

double MediaController::playbackRate() const
{
    return m_clock->playRate();
}

void MediaController::setPlaybackRate(double rate)
{
    if (m_clock->playRate() == rate)
        return;

    // The playbackRate attribute, on setting, must set the MediaController's media controller 
    // playback rate to the new value,
    m_clock->setPlayRate(rate);

    for (size_t index = 0; index < m_mediaElements.size(); ++index)
        m_mediaElements[index]->updatePlaybackRate();

    // then queue a task to fire a simple event named ratechange at the MediaController.
    scheduleEvent(eventNames().ratechangeEvent);
}

void MediaController::setVolume(double level, ExceptionCode& code)
{
    if (m_volume == level)
        return;

    // If the new value is outside the range 0.0 to 1.0 inclusive, then, on setting, an 
    // IndexSizeError exception must be raised instead.
    if (level < 0 || level > 1) {
        code = INDEX_SIZE_ERR;
        return;
    }
        
    // The volume attribute, on setting, if the new value is in the range 0.0 to 1.0 inclusive,
    // must set the MediaController's media controller volume multiplier to the new value
    m_volume = level;

    // and queue a task to fire a simple event named volumechange at the MediaController.
    scheduleEvent(eventNames().volumechangeEvent);

    for (size_t index = 0; index < m_mediaElements.size(); ++index)
        m_mediaElements[index]->updateVolume();
}

void MediaController::setMuted(bool flag)
{
    if (m_muted == flag)
        return;

    // The muted attribute, on setting, must set the MediaController's media controller mute override
    // to the new value
    m_muted = flag;

    // and queue a task to fire a simple event named volumechange at the MediaController.
    scheduleEvent(eventNames().volumechangeEvent);

    for (size_t index = 0; index < m_mediaElements.size(); ++index)
        m_mediaElements[index]->updateVolume();
}

static const AtomicString& playbackStateWaiting()
{
    DEFINE_STATIC_LOCAL(AtomicString, waiting, ("waiting", AtomicString::ConstructFromLiteral));
    return waiting;
}

static const AtomicString& playbackStatePlaying()
{
    DEFINE_STATIC_LOCAL(AtomicString, playing, ("playing", AtomicString::ConstructFromLiteral));
    return playing;
}

static const AtomicString& playbackStateEnded()
{
    DEFINE_STATIC_LOCAL(AtomicString, ended, ("ended", AtomicString::ConstructFromLiteral));
    return ended;
}

const AtomicString& MediaController::playbackState() const
{
    switch (m_playbackState) {
    case WAITING:
        return playbackStateWaiting();
    case PLAYING:
        return playbackStatePlaying();
    case ENDED:
        return playbackStateEnded();
    default:
        ASSERT_NOT_REACHED();
        return nullAtom;
    }
}

void MediaController::reportControllerState()
{
    updateReadyState();
    updatePlaybackState();
}

static AtomicString eventNameForReadyState(MediaControllerInterface::ReadyState state)
{
    switch (state) {
    case MediaControllerInterface::HAVE_NOTHING:
        return eventNames().emptiedEvent;
    case MediaControllerInterface::HAVE_METADATA:
        return eventNames().loadedmetadataEvent;
    case MediaControllerInterface::HAVE_CURRENT_DATA:
        return eventNames().loadeddataEvent;
    case MediaControllerInterface::HAVE_FUTURE_DATA:
        return eventNames().canplayEvent;
    case MediaControllerInterface::HAVE_ENOUGH_DATA:
        return eventNames().canplaythroughEvent;
    default:
        ASSERT_NOT_REACHED();
        return nullAtom;
    }
}

void MediaController::updateReadyState()
{
    ReadyState oldReadyState = m_readyState;
    ReadyState newReadyState;
    
    if (m_mediaElements.isEmpty()) {
        // If the MediaController has no slaved media elements, let new readiness state be 0.
        newReadyState = HAVE_NOTHING;
    } else {
        // Otherwise, let it have the lowest value of the readyState IDL attributes of all of its
        // slaved media elements.
        newReadyState = m_mediaElements.first()->readyState();
        for (size_t index = 1; index < m_mediaElements.size(); ++index)
            newReadyState = min(newReadyState, m_mediaElements[index]->readyState());
    }

    if (newReadyState == oldReadyState) 
        return;

    // If the MediaController's most recently reported readiness state is greater than new readiness 
    // state then queue a task to fire a simple event at the MediaController object, whose name is the
    // event name corresponding to the value of new readiness state given in the table below. [omitted]
    if (oldReadyState > newReadyState) {
        scheduleEvent(eventNameForReadyState(newReadyState));
        return;
    }

    // If the MediaController's most recently reported readiness state is less than the new readiness
    // state, then run these substeps:
    // 1. Let next state be the MediaController's most recently reported readiness state.
    ReadyState nextState = oldReadyState;
    do {
        // 2. Loop: Increment next state by one.
        nextState = static_cast<ReadyState>(nextState + 1);
        // 3. Queue a task to fire a simple event at the MediaController object, whose name is the
        // event name corresponding to the value of next state given in the table below. [omitted]
        scheduleEvent(eventNameForReadyState(nextState));        
        // If next state is less than new readiness state, then return to the step labeled loop
    } while (nextState < newReadyState);

    // Let the MediaController's most recently reported readiness state be new readiness state.
    m_readyState = newReadyState;
}

void MediaController::updatePlaybackState()
{
    PlaybackState oldPlaybackState = m_playbackState;
    PlaybackState newPlaybackState;

    // Initialize new playback state by setting it to the state given for the first matching 
    // condition from the following list:
    if (m_mediaElements.isEmpty()) {
        // If the MediaController has no slaved media elements
        // Let new playback state be waiting.
        newPlaybackState = WAITING;
    } else if (hasEnded()) {
        // If all of the MediaController's slaved media elements have ended playback and the media
        // controller playback rate is positive or zero
        // Let new playback state be ended.
        newPlaybackState = ENDED;
    } else if (isBlocked()) {
        // If the MediaController is a blocked media controller
        // Let new playback state be waiting.
        newPlaybackState = WAITING;
    } else {
        // Otherwise
        // Let new playback state be playing.
        newPlaybackState = PLAYING;
    }

    // If the MediaController's most recently reported playback state is not equal to new playback state
    if (newPlaybackState == oldPlaybackState)
        return;

    // and the new playback state is ended,
    if (newPlaybackState == ENDED) {
        // then queue a task that, if the MediaController object is a playing media controller, and 
        // all of the MediaController's slaved media elements have still ended playback, and the 
        // media controller playback rate is still positive or zero, 
        if (!m_paused && hasEnded()) {
            // changes the MediaController object to a paused media controller
            m_paused = true;

            // and then fires a simple event named pause at the MediaController object.
            scheduleEvent(eventNames().pauseEvent);
        }
    }

    // If the MediaController's most recently reported playback state is not equal to new playback state
    // then queue a task to fire a simple event at the MediaController object, whose name is playing 
    // if new playback state is playing, ended if new playback state is ended, and waiting otherwise.
    AtomicString eventName;
    switch (newPlaybackState) {
    case WAITING:
        eventName = eventNames().waitingEvent;
        m_clock->stop();
        m_timeupdateTimer.stop();
        break;
    case ENDED:
        eventName = eventNames().endedEvent;
        m_clock->stop();
        m_timeupdateTimer.stop();
        break;
    case PLAYING:
        eventName = eventNames().playingEvent;
        m_clock->start();
        startTimeupdateTimer();
        break;
    default:
        ASSERT_NOT_REACHED();
    }
    scheduleEvent(eventName);

    // Let the MediaController's most recently reported playback state be new playback state.
    m_playbackState = newPlaybackState;

    updateMediaElements();
}

void MediaController::updateMediaElements()
{
    for (size_t index = 0; index < m_mediaElements.size(); ++index)
        m_mediaElements[index]->updatePlayState();
}

void MediaController::bringElementUpToSpeed(HTMLMediaElement* element)
{
    ASSERT(element);
    ASSERT(m_mediaElements.contains(element));

    // When the user agent is to bring a media element up to speed with its new media controller,
    // it must seek that media element to the MediaController's media controller position relative
    // to the media element's timeline.
    element->seek(currentTime(), IGNORE_EXCEPTION);
}

bool MediaController::isBlocked() const
{
    // A MediaController is a blocked media controller if the MediaController is a paused media 
    // controller,
    if (m_paused)
        return true;
    
    if (m_mediaElements.isEmpty())
        return false;
    
    bool allPaused = true;
    for (size_t index = 0; index < m_mediaElements.size(); ++index) {
        HTMLMediaElement* element = m_mediaElements[index];
        //  or if any of its slaved media elements are blocked media elements,
        if (element->isBlocked())
            return true;
        
        // or if any of its slaved media elements whose autoplaying flag is true still have their 
        // paused attribute set to true,
        if (element->isAutoplaying() && element->paused())
            return true;
        
        if (!element->paused())
            allPaused = false;
    }
    
    // or if all of its slaved media elements have their paused attribute set to true.
    return allPaused;
}

bool MediaController::hasEnded() const
{
    // If the ... media controller playback rate is positive or zero
    if (m_clock->playRate() < 0)
        return false;

    // [and] all of the MediaController's slaved media elements have ended playback ... let new
    // playback state be ended.
    if (m_mediaElements.isEmpty())
        return false;
    
    bool allHaveEnded = true;
    for (size_t index = 0; index < m_mediaElements.size(); ++index) {
        if (!m_mediaElements[index]->ended())
            allHaveEnded = false;
    }
    return allHaveEnded;
}

void MediaController::scheduleEvent(const AtomicString& eventName)
{
    m_pendingEvents.append(Event::create(eventName, false, true));
    if (!m_asyncEventTimer.isActive())
        m_asyncEventTimer.startOneShot(0);
}

void MediaController::asyncEventTimerFired(Timer<MediaController>*)
{
    Vector<RefPtr<Event> > pendingEvents;

    m_pendingEvents.swap(pendingEvents);
    size_t count = pendingEvents.size();
    for (size_t index = 0; index < count; ++index)
        dispatchEvent(pendingEvents[index].release(), IGNORE_EXCEPTION);
}

void MediaController::clearPositionTimerFired(Timer<MediaController>*)
{
    m_position = MediaPlayer::invalidTime();
}

bool MediaController::hasAudio() const
{
    for (size_t index = 0; index < m_mediaElements.size(); ++index) {
        if (m_mediaElements[index]->hasAudio())
            return true;
    }
    return false;
}

bool MediaController::hasVideo() const
{
    for (size_t index = 0; index < m_mediaElements.size(); ++index) {
        if (m_mediaElements[index]->hasVideo())
            return true;
    }
    return false;
}

bool MediaController::hasClosedCaptions() const
{
    for (size_t index = 0; index < m_mediaElements.size(); ++index) {
        if (m_mediaElements[index]->hasClosedCaptions())
            return true;
    }
    return false;
}

void MediaController::setClosedCaptionsVisible(bool visible)
{
    m_closedCaptionsVisible = visible;
    for (size_t index = 0; index < m_mediaElements.size(); ++index)
        m_mediaElements[index]->setClosedCaptionsVisible(visible);
}

bool MediaController::supportsScanning() const
{
    for (size_t index = 0; index < m_mediaElements.size(); ++index) {
        if (!m_mediaElements[index]->supportsScanning())
            return false;
    }
    return true;
}

void MediaController::beginScrubbing()
{
    for (size_t index = 0; index < m_mediaElements.size(); ++index)
        m_mediaElements[index]->beginScrubbing();
    if (m_playbackState == PLAYING)
        m_clock->stop();
}

void MediaController::endScrubbing()
{
    for (size_t index = 0; index < m_mediaElements.size(); ++index)
        m_mediaElements[index]->endScrubbing();
    if (m_playbackState == PLAYING)
        m_clock->start();
}

bool MediaController::canPlay() const
{
    if (m_paused)
        return true;

    for (size_t index = 0; index < m_mediaElements.size(); ++index) {
        if (!m_mediaElements[index]->canPlay())
            return false;
    }
    return true;
}

bool MediaController::isLiveStream() const
{
    for (size_t index = 0; index < m_mediaElements.size(); ++index) {
        if (!m_mediaElements[index]->isLiveStream())
            return false;
    }
    return true;
}

bool MediaController::hasCurrentSrc() const
{
    for (size_t index = 0; index < m_mediaElements.size(); ++index) {
        if (!m_mediaElements[index]->hasCurrentSrc())
            return false;
    }
    return true;
}

void MediaController::returnToRealtime()
{
    for (size_t index = 0; index < m_mediaElements.size(); ++index)
        m_mediaElements[index]->returnToRealtime();
}

const AtomicString& MediaController::interfaceName() const
{
    return eventNames().interfaceForMediaController;
}

// The spec says to fire periodic timeupdate events (those sent while playing) every
// "15 to 250ms", we choose the slowest frequency
static const double maxTimeupdateEventFrequency = 0.25;

void MediaController::startTimeupdateTimer()
{
    if (m_timeupdateTimer.isActive())
        return;

    m_timeupdateTimer.startRepeating(maxTimeupdateEventFrequency);
}

void MediaController::timeupdateTimerFired(Timer<MediaController>*)
{
    scheduleTimeupdateEvent();
}

void MediaController::scheduleTimeupdateEvent()
{
    double now = WTF::currentTime();
    double timedelta = now - m_previousTimeupdateTime;

    if (timedelta < maxTimeupdateEventFrequency)
        return;

    scheduleEvent(eventNames().timeupdateEvent);
    m_previousTimeupdateTime = now;
}

#endif
