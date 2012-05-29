/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if PLATFORM(MAC) && HAVE(RUNLOOP_TIMER)

#include "RunLoopTimer.h"

namespace WebCore {

RunLoopTimerBase::~RunLoopTimerBase()
{
    stop();
}

static void timerFired(CFRunLoopTimerRef, void* context)
{
    RunLoopTimerBase* timer = static_cast<RunLoopTimerBase*>(context);
    timer->fired();
}

void RunLoopTimerBase::start(double nextFireInterval, double repeatInterval)
{
    if (m_timer)
        CFRunLoopTimerInvalidate(m_timer.get());
    CFRunLoopTimerContext context = { 0, this, 0, 0, 0 };
    m_timer.adoptCF(CFRunLoopTimerCreate(0, CFAbsoluteTimeGetCurrent() + nextFireInterval, repeatInterval, 0, 0, timerFired, &context));
}

void RunLoopTimerBase::schedule(const SchedulePair* schedulePair)
{
    ASSERT_ARG(schedulePair, schedulePair);
    ASSERT_WITH_MESSAGE(m_timer, "Timer must have one of the start functions called before calling schedule().");
    CFRunLoopAddTimer(schedulePair->runLoop(), m_timer.get(), schedulePair->mode());
}

void RunLoopTimerBase::schedule(const SchedulePairHashSet& schedulePairs)
{
    SchedulePairHashSet::const_iterator end = schedulePairs.end();
    for (SchedulePairHashSet::const_iterator it = schedulePairs.begin(); it != end; ++it)
        schedule((*it).get());
}

void RunLoopTimerBase::stop()
{
    if (!m_timer)
        return;
    CFRunLoopTimerInvalidate(m_timer.get());
    m_timer = 0;
}

bool RunLoopTimerBase::isActive() const
{
    return m_timer && CFRunLoopTimerIsValid(m_timer.get());
}

} // namespace WebCore

#endif // PLATFORM(MAC) && HAVE(RUNLOOP_TIMER)
