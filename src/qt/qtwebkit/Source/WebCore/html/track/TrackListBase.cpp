/*
 * Copyright (C) 2011, 2012 Apple Inc.  All rights reserved.
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

#if ENABLE(VIDEO_TRACK)

#include "TrackListBase.h"

#include "EventNames.h"
#include "HTMLMediaElement.h"
#include "ScriptExecutionContext.h"
#include "TrackBase.h"
#include "TrackEvent.h"

using namespace WebCore;

TrackListBase::TrackListBase(HTMLMediaElement* element, ScriptExecutionContext* context)
    : m_context(context)
    , m_element(element)
    , m_pendingEventTimer(this, &TrackListBase::asyncEventTimerFired)
    , m_dispatchingEvents(0)
{
    ASSERT(context->isDocument());
}

TrackListBase::~TrackListBase()
{
}

Element* TrackListBase::element() const
{
    return m_element;
}

unsigned TrackListBase::length() const
{
    return m_inbandTracks.size();
}

void TrackListBase::remove(TrackBase* track)
{
    size_t index = m_inbandTracks.find(track);
    ASSERT(index != notFound);

    ASSERT(track->mediaElement() == m_element);
    track->setMediaElement(0);

    RefPtr<TrackBase> trackRef = m_inbandTracks[index];

    m_inbandTracks.remove(index);

    scheduleRemoveTrackEvent(trackRef.release());
}

bool TrackListBase::contains(TrackBase* track) const
{
    return m_inbandTracks.find(track) != notFound;
}

void TrackListBase::scheduleAddTrackEvent(PassRefPtr<TrackBase> track)
{
    // 4.8.10.5 Loading the media resource
    // ...
    // Fire a trusted event with the name addtrack, that does not bubble and is
    // not cancelable, and that uses the TrackEvent interface, with the track
    // attribute initialized to the new AudioTrack object, at this
    // AudioTrackList object.
    // ...
    // Fire a trusted event with the name addtrack, that does not bubble and is
    // not cancelable, and that uses the TrackEvent interface, with the track
    // attribute initialized to the new VideoTrack object, at this
    // VideoTrackList object.

    // 4.8.10.12.3 Sourcing out-of-band text tracks
    // 4.8.10.12.4 Text track API
    // ... then queue a task to fire an event with the name addtrack, that does not
    // bubble and is not cancelable, and that uses the TrackEvent interface, with
    // the track attribute initialized to the text track's TextTrack object, at
    // the media element's textTracks attribute's TextTrackList object. 

    RefPtr<TrackBase> trackRef = track;
    TrackEventInit initializer;
    initializer.track = trackRef;
    initializer.bubbles = false;
    initializer.cancelable = false;

    m_pendingEvents.append(TrackEvent::create(eventNames().addtrackEvent, initializer));
    if (!m_pendingEventTimer.isActive())
        m_pendingEventTimer.startOneShot(0);
}

void TrackListBase::scheduleRemoveTrackEvent(PassRefPtr<TrackBase> track)
{
    // 4.8.10.6 Offsets into the media resource
    // If at any time the user agent learns that an audio or video track has
    // ended and all media data relating to that track corresponds to parts of
    // the media timeline that are before the earliest possible position, the
    // user agent may queue a task to remove the track from the audioTracks
    // attribute's AudioTrackList object or the videoTracks attribute's
    // VideoTrackList object as appropriate and then fire a trusted event
    // with the name removetrack, that does not bubble and is not cancelable,
    // and that uses the TrackEvent interface, with the track attribute
    // initialized to the AudioTrack or VideoTrack object representing the
    // track, at the media element's aforementioned AudioTrackList or
    // VideoTrackList object.

    // 4.8.10.12.3 Sourcing out-of-band text tracks
    // When a track element's parent element changes and the old parent was a
    // media element, then the user agent must remove the track element's
    // corresponding text track from the media element's list of text tracks,
    // and then queue a task to fire a trusted event with the name removetrack,
    // that does not bubble and is not cancelable, and that uses the TrackEvent
    // interface, with the track attribute initialized to the text track's
    // TextTrack object, at the media element's textTracks attribute's
    // TextTrackList object.

    RefPtr<TrackBase> trackRef = track;
    TrackEventInit initializer;
    initializer.track = trackRef;
    initializer.bubbles = false;
    initializer.cancelable = false;

    m_pendingEvents.append(TrackEvent::create(eventNames().removetrackEvent, initializer));
    if (!m_pendingEventTimer.isActive())
        m_pendingEventTimer.startOneShot(0);
}

void TrackListBase::scheduleChangeEvent()
{
    // 4.8.10.6 Offsets into the media resource
    // Whenever an audio track in an AudioTrackList is enabled or disabled, the
    // user agent must queue a task to fire a simple event named change at the
    // AudioTrackList object.
    // ...
    // Whenever a track in a VideoTrackList that was previously not selected is
    // selected, the user agent must queue a task to fire a simple event named
    // change at the VideoTrackList object.

    EventInit initializer;
    initializer.bubbles = false;
    initializer.cancelable = false;

    m_pendingEvents.append(Event::create(eventNames().changeEvent, initializer));
    if (!m_pendingEventTimer.isActive())
        m_pendingEventTimer.startOneShot(0);
}

void TrackListBase::asyncEventTimerFired(Timer<TrackListBase>*)
{
    Vector<RefPtr<Event> > pendingEvents;

    ++m_dispatchingEvents;
    m_pendingEvents.swap(pendingEvents);
    size_t count = pendingEvents.size();
    for (size_t index = 0; index < count; ++index)
        dispatchEvent(pendingEvents[index].release(), IGNORE_EXCEPTION);
    --m_dispatchingEvents;
}

#endif
