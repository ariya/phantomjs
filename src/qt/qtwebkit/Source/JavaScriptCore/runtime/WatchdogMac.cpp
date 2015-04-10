/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "Watchdog.h"

namespace JSC {

void Watchdog::initTimer()
{
    m_queue = 0;
    m_timer = 0;
}

void Watchdog::destroyTimer()
{
    ASSERT(!m_timer);
    if (m_queue)
        dispatch_release(m_queue);
}

void Watchdog::startTimer(double limit)
{
    ASSERT(!m_timer);
    if (!m_queue)
        m_queue = dispatch_queue_create("jsc.watchdog.queue", 0);
    m_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, m_queue);

    dispatch_source_set_timer(m_timer,
        dispatch_time(DISPATCH_TIME_NOW, limit * NSEC_PER_SEC),
        DISPATCH_TIME_FOREVER, 0);

    dispatch_source_set_event_handler(m_timer, ^{
        m_timerDidFire = true;
    });

    dispatch_resume(m_timer);
}

void Watchdog::stopTimer()
{
    ASSERT(m_queue);
    dispatch_sync(m_queue, ^{
        dispatch_source_cancel(m_timer);
    });
    dispatch_release(m_timer);
    m_timer = 0;
}

} // namespace JSC
