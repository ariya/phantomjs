/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2011 Ericsson AB. All rights reserved.
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
#include "MediaStreamTrack.h"

#if ENABLE(MEDIA_STREAM)

#include "Event.h"
#include "MediaStreamCenter.h"
#include "MediaStreamComponent.h"

namespace WebCore {

PassRefPtr<MediaStreamTrack> MediaStreamTrack::create(ScriptExecutionContext* context, MediaStreamComponent* component)
{
    RefPtr<MediaStreamTrack> track = adoptRef(new MediaStreamTrack(context, component));
    track->suspendIfNeeded();
    return track.release();
}

MediaStreamTrack::MediaStreamTrack(ScriptExecutionContext* context, MediaStreamComponent* component)
    : ActiveDOMObject(context)
    , m_stopped(false)
    , m_component(component)
{
    m_component->source()->addObserver(this);
}

MediaStreamTrack::~MediaStreamTrack()
{
    m_component->source()->removeObserver(this);
}

String MediaStreamTrack::kind() const
{
    DEFINE_STATIC_LOCAL(String, audioKind, (ASCIILiteral("audio")));
    DEFINE_STATIC_LOCAL(String, videoKind, (ASCIILiteral("video")));

    switch (m_component->source()->type()) {
    case MediaStreamSource::TypeAudio:
        return audioKind;
    case MediaStreamSource::TypeVideo:
        return videoKind;
    }

    ASSERT_NOT_REACHED();
    return audioKind;
}

String MediaStreamTrack::id() const
{
    return m_component->id();
}

String MediaStreamTrack::label() const
{
    return m_component->source()->name();
}

bool MediaStreamTrack::enabled() const
{
    return m_component->enabled();
}

void MediaStreamTrack::setEnabled(bool enabled)
{
    if (m_stopped || enabled == m_component->enabled())
        return;

    m_component->setEnabled(enabled);

    if (m_component->stream()->ended())
        return;

    MediaStreamCenter::instance().didSetMediaStreamTrackEnabled(m_component->stream(), m_component.get());
}

String MediaStreamTrack::readyState() const
{
    if (m_stopped)
        return ASCIILiteral("ended");

    switch (m_component->source()->readyState()) {
    case MediaStreamSource::ReadyStateLive:
        return ASCIILiteral("live");
    case MediaStreamSource::ReadyStateMuted:
        return ASCIILiteral("muted");
    case MediaStreamSource::ReadyStateEnded:
        return ASCIILiteral("ended");
    }

    ASSERT_NOT_REACHED();
    return String();
}

bool MediaStreamTrack::ended() const
{
    return m_stopped || (m_component->source()->readyState() == MediaStreamSource::ReadyStateEnded);
}

void MediaStreamTrack::sourceChangedState()
{
    if (m_stopped)
        return;

    switch (m_component->source()->readyState()) {
    case MediaStreamSource::ReadyStateLive:
        dispatchEvent(Event::create(eventNames().unmuteEvent, false, false));
        break;
    case MediaStreamSource::ReadyStateMuted:
        dispatchEvent(Event::create(eventNames().muteEvent, false, false));
        break;
    case MediaStreamSource::ReadyStateEnded:
        dispatchEvent(Event::create(eventNames().endedEvent, false, false));
        break;
    }
}

MediaStreamComponent* MediaStreamTrack::component()
{
    return m_component.get();
}

void MediaStreamTrack::stop()
{
    m_stopped = true;
}

const AtomicString& MediaStreamTrack::interfaceName() const
{
    return eventNames().interfaceForMediaStreamTrack;
}

ScriptExecutionContext* MediaStreamTrack::scriptExecutionContext() const
{
    return ActiveDOMObject::scriptExecutionContext();
}

EventTargetData* MediaStreamTrack::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* MediaStreamTrack::ensureEventTargetData()
{
    return &m_eventTargetData;
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
