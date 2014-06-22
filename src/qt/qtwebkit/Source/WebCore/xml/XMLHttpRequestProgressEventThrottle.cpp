/*
 * Copyright (C) 2010 Julien Chaffraix <jchaffraix@webkit.org>  All right reserved.
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "XMLHttpRequestProgressEventThrottle.h"

#include "EventTarget.h"
#include "XMLHttpRequestProgressEvent.h"

namespace WebCore {

const double XMLHttpRequestProgressEventThrottle::minimumProgressEventDispatchingIntervalInSeconds = .05; // 50 ms per specification.

XMLHttpRequestProgressEventThrottle::XMLHttpRequestProgressEventThrottle(EventTarget* target)
    : m_target(target)
    , m_loaded(0)
    , m_total(0)
    , m_deferEvents(false)
    , m_dispatchDeferredEventsTimer(this, &XMLHttpRequestProgressEventThrottle::dispatchDeferredEvents)
{
    ASSERT(target);
}

XMLHttpRequestProgressEventThrottle::~XMLHttpRequestProgressEventThrottle()
{
}

void XMLHttpRequestProgressEventThrottle::dispatchProgressEvent(bool lengthComputable, unsigned long long loaded, unsigned long long total)
{
    if (m_deferEvents) {
        // Only store the latest progress event while suspended.
        m_deferredProgressEvent = XMLHttpRequestProgressEvent::create(eventNames().progressEvent, lengthComputable, loaded, total);
        return;
    }

    if (!isActive()) {
        // The timer is not active so the least frequent event for now is every byte.
        // Just go ahead and dispatch the event.

        // We should not have any pending loaded & total information from a previous run.
        ASSERT(!m_loaded);
        ASSERT(!m_total);

        dispatchEvent(XMLHttpRequestProgressEvent::create(eventNames().progressEvent, lengthComputable, loaded, total));
        startRepeating(minimumProgressEventDispatchingIntervalInSeconds);
        return;
    }

    // The timer is already active so minimumProgressEventDispatchingIntervalInSeconds is the least frequent event.
    m_lengthComputable = lengthComputable;
    m_loaded = loaded;
    m_total = total;
}

void XMLHttpRequestProgressEventThrottle::dispatchReadyStateChangeEvent(PassRefPtr<Event> event, ProgressEventAction progressEventAction)
{
    if (progressEventAction == FlushProgressEvent)
        flushProgressEvent();

    dispatchEvent(event);
}

void XMLHttpRequestProgressEventThrottle::dispatchEvent(PassRefPtr<Event> event)
{
    ASSERT(event);
    if (m_deferEvents) {
        if (m_deferredEvents.size() > 1 && event->type() == eventNames().readystatechangeEvent && event->type() == m_deferredEvents.last()->type()) {
            // Readystatechange events are state-less so avoid repeating two identical events in a row on resume.
            return;
        }
        m_deferredEvents.append(event);
    } else
        m_target->dispatchEvent(event);
}

void XMLHttpRequestProgressEventThrottle::dispatchEventAndLoadEnd(PassRefPtr<Event> event)
{
    ASSERT(event->type() == eventNames().loadEvent || event->type() == eventNames().abortEvent || event->type() == eventNames().errorEvent || event->type() == eventNames().timeoutEvent);

    dispatchEvent(event);
    dispatchEvent(XMLHttpRequestProgressEvent::create(eventNames().loadendEvent));
}

void XMLHttpRequestProgressEventThrottle::flushProgressEvent()
{
    if (m_deferEvents && m_deferredProgressEvent) {
        // Move the progress event to the queue, to get it in the right order on resume.
        m_deferredEvents.append(m_deferredProgressEvent);
        m_deferredProgressEvent = 0;
        return;
    }

    if (!hasEventToDispatch())
        return;

    PassRefPtr<Event> event = XMLHttpRequestProgressEvent::create(eventNames().progressEvent, m_lengthComputable, m_loaded, m_total);
    m_loaded = 0;
    m_total = 0;

    // We stop the timer as this is called when no more events are supposed to occur.
    stop();

    dispatchEvent(event);
}

void XMLHttpRequestProgressEventThrottle::dispatchDeferredEvents(Timer<XMLHttpRequestProgressEventThrottle>* timer)
{
    ASSERT_UNUSED(timer, timer == &m_dispatchDeferredEventsTimer);
    ASSERT(m_deferEvents);
    m_deferEvents = false;

    // Take over the deferred events before dispatching them which can potentially add more.
    Vector<RefPtr<Event> > deferredEvents;
    m_deferredEvents.swap(deferredEvents);

    RefPtr<Event> deferredProgressEvent = m_deferredProgressEvent;
    m_deferredProgressEvent = 0;

    Vector<RefPtr<Event> >::const_iterator it = deferredEvents.begin();
    const Vector<RefPtr<Event> >::const_iterator end = deferredEvents.end();
    for (; it != end; ++it)
        dispatchEvent(*it);

    // The progress event will be in the m_deferredEvents vector if the load was finished while suspended.
    // If not, just send the most up-to-date progress on resume.
    if (deferredProgressEvent)
        dispatchEvent(deferredProgressEvent);
}

void XMLHttpRequestProgressEventThrottle::fired()
{
    ASSERT(isActive());
    if (!hasEventToDispatch()) {
        // No progress event was queued since the previous dispatch, we can safely stop the timer.
        stop();
        return;
    }

    dispatchEvent(XMLHttpRequestProgressEvent::create(eventNames().progressEvent, m_lengthComputable, m_loaded, m_total));
    m_total = 0;
    m_loaded = 0;
}

bool XMLHttpRequestProgressEventThrottle::hasEventToDispatch() const
{
    return (m_total || m_loaded) && isActive();
}

void XMLHttpRequestProgressEventThrottle::suspend()
{
    // If re-suspended before deferred events have been dispatched, just stop the dispatch
    // and continue the last suspend.
    if (m_dispatchDeferredEventsTimer.isActive()) {
        ASSERT(m_deferEvents);
        m_dispatchDeferredEventsTimer.stop();
        return;
    }
    ASSERT(!m_deferredProgressEvent);
    ASSERT(m_deferredEvents.isEmpty());
    ASSERT(!m_deferEvents);

    m_deferEvents = true;
    // If we have a progress event waiting to be dispatched,
    // just defer it.
    if (hasEventToDispatch()) {
        m_deferredProgressEvent = XMLHttpRequestProgressEvent::create(eventNames().progressEvent, m_lengthComputable, m_loaded, m_total);
        m_total = 0;
        m_loaded = 0;
    }
    stop();
}

void XMLHttpRequestProgressEventThrottle::resume()
{
    ASSERT(!m_loaded);
    ASSERT(!m_total);

    if (m_deferredEvents.isEmpty() && !m_deferredProgressEvent) {
        m_deferEvents = false;
        return;
    }

    // Do not dispatch events inline here, since ScriptExecutionContext is iterating over
    // the list of active DOM objects to resume them, and any activated JS event-handler
    // could insert new active DOM objects to the list.
    // m_deferEvents is kept true until all deferred events have been dispatched.
    m_dispatchDeferredEventsTimer.startOneShot(0);
}

} // namespace WebCore
