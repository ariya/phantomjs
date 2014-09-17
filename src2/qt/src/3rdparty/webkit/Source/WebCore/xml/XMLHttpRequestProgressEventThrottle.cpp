/*
 * Copyright (C) 2010 Julien Chaffraix <jchaffraix@webkit.org>
 * All right reserved.
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
    , m_suspended(false)
{
    ASSERT(target);
}

XMLHttpRequestProgressEventThrottle::~XMLHttpRequestProgressEventThrottle()
{
}

void XMLHttpRequestProgressEventThrottle::dispatchProgressEvent(bool lengthComputable, unsigned long long loaded, unsigned long long total)
{
    ASSERT(!suspended());
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

void XMLHttpRequestProgressEventThrottle::dispatchEvent(PassRefPtr<Event> event, ProgressEventAction progressEventAction)
{
    ASSERT(!suspended());
    // We should not have any pending events from a previous resume.
    ASSERT(!m_pausedEvent);

    if (progressEventAction == FlushProgressEvent)
        flushProgressEvent();

    m_target->dispatchEvent(event);
}

void XMLHttpRequestProgressEventThrottle::flushProgressEvent()
{
    if (!hasEventToDispatch())
        return;

    PassRefPtr<Event> event = XMLHttpRequestProgressEvent::create(eventNames().progressEvent, m_lengthComputable, m_loaded, m_total);
    m_loaded = 0;
    m_total = 0;

    // We stop the timer as this is called when no more events are supposed to occur.
    stop();

    m_target->dispatchEvent(event);
}

void XMLHttpRequestProgressEventThrottle::dispatchPausedEvent()
{
    ASSERT(!suspended());
    if (!m_pausedEvent)
        return;

    m_target->dispatchEvent(m_pausedEvent);
    m_pausedEvent = 0;
}

void XMLHttpRequestProgressEventThrottle::fired()
{
    ASSERT(isActive());
    ASSERT(!suspended());
    ASSERT(!m_pausedEvent);
    if (!hasEventToDispatch()) {
        // No progress event was queued since the previous dispatch, we can safely stop the timer.
        stop();
        return;
    }

    m_target->dispatchEvent(XMLHttpRequestProgressEvent::create(eventNames().progressEvent, m_lengthComputable, m_loaded, m_total));
    m_total = 0;
    m_loaded = 0;
}

bool XMLHttpRequestProgressEventThrottle::hasEventToDispatch() const
{
    return (m_total || m_loaded) && isActive();
}

void XMLHttpRequestProgressEventThrottle::suspend()
{
    ASSERT(!m_pausedEvent);

    m_suspended = true;
    // If we have a progress event waiting to be dispatched,
    // just queue it.
    if (hasEventToDispatch()) {
        m_pausedEvent = XMLHttpRequestProgressEvent::create(eventNames().progressEvent, m_lengthComputable, m_loaded, m_total);
        m_total = 0;
        m_loaded = 0;
    }
    stop();
}

void XMLHttpRequestProgressEventThrottle::resume()
{
    ASSERT(!m_loaded);
    ASSERT(!m_total);

    m_suspended = false;
    dispatchPausedEvent();
}

} // namespace WebCore
