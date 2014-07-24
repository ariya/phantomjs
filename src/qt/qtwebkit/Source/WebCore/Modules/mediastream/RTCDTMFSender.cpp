/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(MEDIA_STREAM)

#include "RTCDTMFSender.h"

#include "ExceptionCode.h"
#include "MediaStreamTrack.h"
#include "RTCDTMFSenderHandler.h"
#include "RTCDTMFToneChangeEvent.h"
#include "RTCPeerConnectionHandler.h"
#include "ScriptExecutionContext.h"

namespace WebCore {

static const long minToneDurationMs = 70;
static const long defaultToneDurationMs = 100;
static const long maxToneDurationMs = 6000;
static const long minInterToneGapMs = 50;
static const long defaultInterToneGapMs = 50;

PassRefPtr<RTCDTMFSender> RTCDTMFSender::create(ScriptExecutionContext* context, RTCPeerConnectionHandler* peerConnectionHandler, PassRefPtr<MediaStreamTrack> prpTrack, ExceptionCode& ec)
{
    RefPtr<MediaStreamTrack> track = prpTrack;
    OwnPtr<RTCDTMFSenderHandler> handler = peerConnectionHandler->createDTMFSender(track->component());
    if (!handler) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    RefPtr<RTCDTMFSender> dtmfSender = adoptRef(new RTCDTMFSender(context, track, handler.release()));
    dtmfSender->suspendIfNeeded();
    return dtmfSender.release();
}

RTCDTMFSender::RTCDTMFSender(ScriptExecutionContext* context, PassRefPtr<MediaStreamTrack> track, PassOwnPtr<RTCDTMFSenderHandler> handler)
    : ActiveDOMObject(context)
    , m_track(track)
    , m_duration(defaultToneDurationMs)
    , m_interToneGap(defaultInterToneGapMs)
    , m_handler(handler)
    , m_stopped(false)
    , m_scheduledEventTimer(this, &RTCDTMFSender::scheduledEventTimerFired)
{
    m_handler->setClient(this);
}

RTCDTMFSender::~RTCDTMFSender()
{
}

bool RTCDTMFSender::canInsertDTMF() const
{
    return m_handler->canInsertDTMF();
}

MediaStreamTrack* RTCDTMFSender::track() const
{
    return m_track.get();
}

String RTCDTMFSender::toneBuffer() const
{
    return m_handler->currentToneBuffer();
}

void RTCDTMFSender::insertDTMF(const String& tones, ExceptionCode& ec)
{
    insertDTMF(tones, defaultToneDurationMs, defaultInterToneGapMs, ec);
}

void RTCDTMFSender::insertDTMF(const String& tones, long duration, ExceptionCode& ec)
{
    insertDTMF(tones, duration, defaultInterToneGapMs, ec);
}

void RTCDTMFSender::insertDTMF(const String& tones, long duration, long interToneGap, ExceptionCode& ec)
{
    if (!canInsertDTMF()) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }

    if (duration > maxToneDurationMs || duration < minToneDurationMs) {
        ec = SYNTAX_ERR;
        return;
    }

    if (interToneGap < minInterToneGapMs) {
        ec = SYNTAX_ERR;
        return;
    }

    m_duration = duration;
    m_interToneGap = interToneGap;

    if (!m_handler->insertDTMF(tones, m_duration, m_interToneGap))
        ec = SYNTAX_ERR;
}

void RTCDTMFSender::didPlayTone(const String& tone)
{
    scheduleDispatchEvent(RTCDTMFToneChangeEvent::create(tone));
}

const AtomicString& RTCDTMFSender::interfaceName() const
{
    return eventNames().interfaceForRTCDTMFSender;
}

ScriptExecutionContext* RTCDTMFSender::scriptExecutionContext() const
{
    return ActiveDOMObject::scriptExecutionContext();
}

void RTCDTMFSender::stop()
{
    m_stopped = true;
    m_handler->setClient(0);
}

EventTargetData* RTCDTMFSender::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* RTCDTMFSender::ensureEventTargetData()
{
    return &m_eventTargetData;
}

void RTCDTMFSender::scheduleDispatchEvent(PassRefPtr<Event> event)
{
    m_scheduledEvents.append(event);

    if (!m_scheduledEventTimer.isActive())
        m_scheduledEventTimer.startOneShot(0);
}

void RTCDTMFSender::scheduledEventTimerFired(Timer<RTCDTMFSender>*)
{
    if (m_stopped)
        return;

    Vector<RefPtr<Event> > events;
    events.swap(m_scheduledEvents);

    Vector<RefPtr<Event> >::iterator it = events.begin();
    for (; it != events.end(); ++it)
        dispatchEvent((*it).release());
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
