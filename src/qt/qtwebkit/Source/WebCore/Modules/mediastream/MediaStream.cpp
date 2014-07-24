/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2011, 2012 Ericsson AB. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MediaStream.h"

#if ENABLE(MEDIA_STREAM)

#include "Event.h"
#include "ExceptionCode.h"
#include "MediaStreamCenter.h"
#include "MediaStreamSource.h"
#include "MediaStreamTrackEvent.h"
#include "UUID.h"

namespace WebCore {

static bool containsSource(MediaStreamSourceVector& sourceVector, MediaStreamSource* source)
{
    for (size_t i = 0; i < sourceVector.size(); ++i) {
        if (source->id() == sourceVector[i]->id())
            return true;
    }
    return false;
}

static void processTrack(MediaStreamTrack* track, MediaStreamSourceVector& sourceVector)
{
    if (track->ended())
        return;

    MediaStreamSource* source = track->component()->source();
    if (!containsSource(sourceVector, source))
        sourceVector.append(source);
}

static PassRefPtr<MediaStream> createFromSourceVectors(ScriptExecutionContext* context, const MediaStreamSourceVector& audioSources, const MediaStreamSourceVector& videoSources)
{
    RefPtr<MediaStreamDescriptor> descriptor = MediaStreamDescriptor::create(createCanonicalUUIDString(), audioSources, videoSources);
    MediaStreamCenter::instance().didCreateMediaStream(descriptor.get());

    return MediaStream::create(context, descriptor.release());
}

PassRefPtr<MediaStream> MediaStream::create(ScriptExecutionContext* context)
{
    MediaStreamSourceVector audioSources;
    MediaStreamSourceVector videoSources;

    return createFromSourceVectors(context, audioSources, videoSources);
}

PassRefPtr<MediaStream> MediaStream::create(ScriptExecutionContext* context, PassRefPtr<MediaStream> stream)
{
    ASSERT(stream);

    MediaStreamSourceVector audioSources;
    MediaStreamSourceVector videoSources;

    for (size_t i = 0; i < stream->m_audioTracks.size(); ++i)
        processTrack(stream->m_audioTracks[i].get(), audioSources);

    for (size_t i = 0; i < stream->m_videoTracks.size(); ++i)
        processTrack(stream->m_videoTracks[i].get(), videoSources);

    return createFromSourceVectors(context, audioSources, videoSources);
}

PassRefPtr<MediaStream> MediaStream::create(ScriptExecutionContext* context, const MediaStreamTrackVector& tracks)
{
    MediaStreamSourceVector audioSources;
    MediaStreamSourceVector videoSources;

    for (size_t i = 0; i < tracks.size(); ++i)
        processTrack(tracks[i].get(), tracks[i]->kind() == "audio" ? audioSources : videoSources);

    return createFromSourceVectors(context, audioSources, videoSources);
}

PassRefPtr<MediaStream> MediaStream::create(ScriptExecutionContext* context, PassRefPtr<MediaStreamDescriptor> streamDescriptor)
{
    return adoptRef(new MediaStream(context, streamDescriptor));
}

MediaStream::MediaStream(ScriptExecutionContext* context, PassRefPtr<MediaStreamDescriptor> streamDescriptor)
    : ContextDestructionObserver(context)
    , m_stopped(false)
    , m_descriptor(streamDescriptor)
    , m_scheduledEventTimer(this, &MediaStream::scheduledEventTimerFired)
{
    m_descriptor->setClient(this);

    size_t numberOfAudioTracks = m_descriptor->numberOfAudioComponents();
    m_audioTracks.reserveCapacity(numberOfAudioTracks);
    for (size_t i = 0; i < numberOfAudioTracks; i++)
        m_audioTracks.append(MediaStreamTrack::create(context, m_descriptor->audioComponent(i)));

    size_t numberOfVideoTracks = m_descriptor->numberOfVideoComponents();
    m_videoTracks.reserveCapacity(numberOfVideoTracks);
    for (size_t i = 0; i < numberOfVideoTracks; i++)
        m_videoTracks.append(MediaStreamTrack::create(context, m_descriptor->videoComponent(i)));
}

MediaStream::~MediaStream()
{
    m_descriptor->setClient(0);
}

bool MediaStream::ended() const
{
    return m_stopped || m_descriptor->ended();
}

void MediaStream::addTrack(PassRefPtr<MediaStreamTrack> prpTrack, ExceptionCode& ec)
{
    if (ended()) {
        ec = INVALID_STATE_ERR;
        return;
    }

    if (!prpTrack) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    RefPtr<MediaStreamTrack> track = prpTrack;

    if (getTrackById(track->id()))
        return;

    RefPtr<MediaStreamComponent> component = MediaStreamComponent::create(m_descriptor.get(), track->component()->source());
    RefPtr<MediaStreamTrack> newTrack = MediaStreamTrack::create(scriptExecutionContext(), component.get());

    switch (component->source()->type()) {
    case MediaStreamSource::TypeAudio:
        m_descriptor->addAudioComponent(component.release());
        m_audioTracks.append(newTrack);
        break;
    case MediaStreamSource::TypeVideo:
        m_descriptor->addVideoComponent(component.release());
        m_videoTracks.append(newTrack);
        break;
    }

    MediaStreamCenter::instance().didAddMediaStreamTrack(m_descriptor.get(), newTrack->component());
}

void MediaStream::removeTrack(PassRefPtr<MediaStreamTrack> prpTrack , ExceptionCode& ec)
{
    if (ended()) {
        ec = INVALID_STATE_ERR;
        return;
    }

    if (!prpTrack) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    RefPtr<MediaStreamTrack> track = prpTrack;

    switch (track->component()->source()->type()) {
    case MediaStreamSource::TypeAudio: {
        size_t pos = m_audioTracks.find(track);
        if (pos != notFound) {
            m_audioTracks.remove(pos);
            m_descriptor->removeAudioComponent(track->component());
        }
        break;
    }
    case MediaStreamSource::TypeVideo: {
        size_t pos = m_videoTracks.find(track);
        if (pos != notFound) {
            m_videoTracks.remove(pos);
            m_descriptor->removeVideoComponent(track->component());
        }
        break;
    }
    }

    if (!m_audioTracks.size() && !m_videoTracks.size())
        m_descriptor->setEnded();

    MediaStreamCenter::instance().didRemoveMediaStreamTrack(m_descriptor.get(), track->component());
}

MediaStreamTrack* MediaStream::getTrackById(String id)
{
    for (MediaStreamTrackVector::iterator iter = m_audioTracks.begin(); iter != m_audioTracks.end(); ++iter) {
        if ((*iter)->id() == id)
            return (*iter).get();
    }

    for (MediaStreamTrackVector::iterator iter = m_videoTracks.begin(); iter != m_videoTracks.end(); ++iter) {
        if ((*iter)->id() == id)
            return (*iter).get();
    }

    return 0;
}

void MediaStream::streamEnded()
{
    if (ended())
        return;

    m_descriptor->setEnded();
    scheduleDispatchEvent(Event::create(eventNames().endedEvent, false, false));
}

void MediaStream::contextDestroyed()
{
    ContextDestructionObserver::contextDestroyed();
    m_stopped = true;
}

const AtomicString& MediaStream::interfaceName() const
{
    return eventNames().interfaceForMediaStream;
}

ScriptExecutionContext* MediaStream::scriptExecutionContext() const
{
    return ContextDestructionObserver::scriptExecutionContext();
}

EventTargetData* MediaStream::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* MediaStream::ensureEventTargetData()
{
    return &m_eventTargetData;
}

void MediaStream::addRemoteTrack(MediaStreamComponent* component)
{
    ASSERT(component && !component->stream());
    if (ended())
        return;

    component->setStream(descriptor());

    RefPtr<MediaStreamTrack> track = MediaStreamTrack::create(scriptExecutionContext(), component);
    switch (component->source()->type()) {
    case MediaStreamSource::TypeAudio:
        m_audioTracks.append(track);
        break;
    case MediaStreamSource::TypeVideo:
        m_videoTracks.append(track);
        break;
    }

    scheduleDispatchEvent(MediaStreamTrackEvent::create(eventNames().addtrackEvent, false, false, track));
}

void MediaStream::removeRemoteTrack(MediaStreamComponent* component)
{
    if (ended())
        return;

    MediaStreamTrackVector* tracks = 0;
    switch (component->source()->type()) {
    case MediaStreamSource::TypeAudio:
        tracks = &m_audioTracks;
        break;
    case MediaStreamSource::TypeVideo:
        tracks = &m_videoTracks;
        break;
    }

    size_t index = notFound;
    for (size_t i = 0; i < tracks->size(); ++i) {
        if ((*tracks)[i]->component() == component) {
            index = i;
            break;
        }
    }
    if (index == notFound)
        return;

    RefPtr<MediaStreamTrack> track = (*tracks)[index];
    tracks->remove(index);
    scheduleDispatchEvent(MediaStreamTrackEvent::create(eventNames().removetrackEvent, false, false, track));
}

void MediaStream::scheduleDispatchEvent(PassRefPtr<Event> event)
{
    m_scheduledEvents.append(event);

    if (!m_scheduledEventTimer.isActive())
        m_scheduledEventTimer.startOneShot(0);
}

void MediaStream::scheduledEventTimerFired(Timer<MediaStream>*)
{
    if (m_stopped)
        return;

    Vector<RefPtr<Event> > events;
    events.swap(m_scheduledEvents);

    Vector<RefPtr<Event> >::iterator it = events.begin();
    for (; it != events.end(); ++it)
        dispatchEvent((*it).release());

    events.clear();
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
