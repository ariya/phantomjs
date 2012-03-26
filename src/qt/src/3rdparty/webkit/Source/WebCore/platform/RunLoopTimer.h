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

#ifndef RunLoopTimer_h
#define RunLoopTimer_h

#include "SchedulePair.h"
#include <wtf/RetainPtr.h>

namespace WebCore {

// Time intervals are all in seconds.

class RunLoopTimerBase {
    WTF_MAKE_NONCOPYABLE(RunLoopTimerBase);
public:
    RunLoopTimerBase() { }
    virtual ~RunLoopTimerBase();

    void schedule(const SchedulePair*);
    void schedule(const SchedulePairHashSet&);

    void start(double nextFireInterval, double repeatInterval);

    void startRepeating(double repeatInterval) { start(repeatInterval, repeatInterval); }
    void startOneShot(double interval) { start(interval, 0); }

    void stop();
    bool isActive() const;

    virtual void fired() = 0;

private:
#if USE(CF)
    RetainPtr<CFRunLoopTimerRef> m_timer;
#endif
};

template <typename TimerFiredClass> class RunLoopTimer : public RunLoopTimerBase {
public:
    typedef void (TimerFiredClass::*TimerFiredFunction)(RunLoopTimer*);

    RunLoopTimer(TimerFiredClass* o, TimerFiredFunction f)
        : m_object(o), m_function(f) { }

    virtual void fired() { (m_object->*m_function)(this); }

private:
    TimerFiredClass* m_object;
    TimerFiredFunction m_function;
};

} // namespace WebCore

#endif
