/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
#include "Timer.h"

#include "SharedTimer.h"
#include "ThreadGlobalData.h"
#include "ThreadTimers.h"
#include <limits.h>
#include <limits>
#include <math.h>
#include <wtf/CurrentTime.h>
#include <wtf/HashSet.h>
#include <wtf/Vector.h>

using namespace std;

namespace WebCore {

// Timers are stored in a heap data structure, used to implement a priority queue.
// This allows us to efficiently determine which timer needs to fire the soonest.
// Then we set a single shared system timer to fire at that time.
//
// When a timer's "next fire time" changes, we need to move it around in the priority queue.

// Simple accessors to thread-specific data.
static Vector<TimerBase*>& timerHeap()
{
    return threadGlobalData().threadTimers().timerHeap();
}

// Class to represent elements in the heap when calling the standard library heap algorithms.
// Maintains the m_heapIndex value in the timers themselves, which allows us to do efficient
// modification of the heap.
class TimerHeapElement {
public:
    explicit TimerHeapElement(int i)
        : m_index(i)
        , m_timer(timerHeap()[m_index])
    { 
        checkConsistency(); 
    }

    TimerHeapElement(const TimerHeapElement&);
    TimerHeapElement& operator=(const TimerHeapElement&);

    TimerBase* timer() const { return m_timer; }

    void checkConsistency() const
    {
        ASSERT(m_index >= 0);
        ASSERT(m_index < static_cast<int>(timerHeap().size()));
    }

private:
    TimerHeapElement();

    int m_index;
    TimerBase* m_timer;
};

inline TimerHeapElement::TimerHeapElement(const TimerHeapElement& o)
    : m_index(-1), m_timer(o.timer())
{
}

inline TimerHeapElement& TimerHeapElement::operator=(const TimerHeapElement& o)
{
    TimerBase* t = o.timer();
    m_timer = t;
    if (m_index != -1) {
        checkConsistency();
        timerHeap()[m_index] = t;
        t->m_heapIndex = m_index;
    }
    return *this;
}

inline bool operator<(const TimerHeapElement& a, const TimerHeapElement& b)
{
    // The comparisons below are "backwards" because the heap puts the largest 
    // element first and we want the lowest time to be the first one in the heap.
    double aFireTime = a.timer()->m_nextFireTime;
    double bFireTime = b.timer()->m_nextFireTime;
    if (bFireTime != aFireTime)
        return bFireTime < aFireTime;
    
    // We need to look at the difference of the insertion orders instead of comparing the two 
    // outright in case of overflow. 
    unsigned difference = a.timer()->m_heapInsertionOrder - b.timer()->m_heapInsertionOrder;
    return difference < UINT_MAX / 2;
}

// ----------------

// Class to represent iterators in the heap when calling the standard library heap algorithms.
// Returns TimerHeapElement for elements in the heap rather than the TimerBase pointers themselves.
class TimerHeapIterator : public iterator<random_access_iterator_tag, TimerHeapElement, int> {
public:
    TimerHeapIterator() : m_index(-1) { }
    TimerHeapIterator(int i) : m_index(i) { checkConsistency(); }

    TimerHeapIterator& operator++() { checkConsistency(); ++m_index; checkConsistency(); return *this; }
    TimerHeapIterator operator++(int) { checkConsistency(); checkConsistency(1); return m_index++; }

    TimerHeapIterator& operator--() { checkConsistency(); --m_index; checkConsistency(); return *this; }
    TimerHeapIterator operator--(int) { checkConsistency(); checkConsistency(-1); return m_index--; }

    TimerHeapIterator& operator+=(int i) { checkConsistency(); m_index += i; checkConsistency(); return *this; }
    TimerHeapIterator& operator-=(int i) { checkConsistency(); m_index -= i; checkConsistency(); return *this; }

    TimerHeapElement operator*() const { return TimerHeapElement(m_index); }
    TimerHeapElement operator[](int i) const { return TimerHeapElement(m_index + i); }

    int index() const { return m_index; }

    void checkConsistency(int offset = 0) const
    {
        ASSERT_UNUSED(offset, m_index + offset >= 0);
        ASSERT_UNUSED(offset, m_index + offset <= static_cast<int>(timerHeap().size()));
    }

private:
    int m_index;
};

inline bool operator==(TimerHeapIterator a, TimerHeapIterator b) { return a.index() == b.index(); }
inline bool operator!=(TimerHeapIterator a, TimerHeapIterator b) { return a.index() != b.index(); }
inline bool operator<(TimerHeapIterator a, TimerHeapIterator b) { return a.index() < b.index(); }

inline TimerHeapIterator operator+(TimerHeapIterator a, int b) { return a.index() + b; }
inline TimerHeapIterator operator+(int a, TimerHeapIterator b) { return a + b.index(); }

inline TimerHeapIterator operator-(TimerHeapIterator a, int b) { return a.index() - b; }
inline int operator-(TimerHeapIterator a, TimerHeapIterator b) { return a.index() - b.index(); }

// ----------------

TimerBase::TimerBase()
    : m_nextFireTime(0)
    , m_repeatInterval(0)
    , m_heapIndex(-1)
#ifndef NDEBUG
    , m_thread(currentThread())
#endif
{
}

TimerBase::~TimerBase()
{
    stop();
    ASSERT(!inHeap());
}

void TimerBase::start(double nextFireInterval, double repeatInterval)
{
    ASSERT(m_thread == currentThread());

    m_repeatInterval = repeatInterval;
    setNextFireTime(currentTime() + nextFireInterval);
}

void TimerBase::stop()
{
    ASSERT(m_thread == currentThread());

    m_repeatInterval = 0;
    setNextFireTime(0);

    ASSERT(m_nextFireTime == 0);
    ASSERT(m_repeatInterval == 0);
    ASSERT(!inHeap());
}

double TimerBase::nextFireInterval() const
{
    ASSERT(isActive());
    double current = currentTime();
    if (m_nextFireTime < current)
        return 0;
    return m_nextFireTime - current;
}

inline void TimerBase::checkHeapIndex() const
{
    ASSERT(!timerHeap().isEmpty());
    ASSERT(m_heapIndex >= 0);
    ASSERT(m_heapIndex < static_cast<int>(timerHeap().size()));
    ASSERT(timerHeap()[m_heapIndex] == this);
}

inline void TimerBase::checkConsistency() const
{
    // Timers should be in the heap if and only if they have a non-zero next fire time.
    ASSERT(inHeap() == (m_nextFireTime != 0));
    if (inHeap())
        checkHeapIndex();
}

void TimerBase::heapDecreaseKey()
{
    ASSERT(m_nextFireTime != 0);
    checkHeapIndex();
    push_heap(TimerHeapIterator(0), TimerHeapIterator(m_heapIndex + 1));
    checkHeapIndex();
}

inline void TimerBase::heapDelete()
{
    ASSERT(m_nextFireTime == 0);
    heapPop();
    timerHeap().removeLast();
    m_heapIndex = -1;
}

void TimerBase::heapDeleteMin()
{
    ASSERT(m_nextFireTime == 0);
    heapPopMin();
    timerHeap().removeLast();
    m_heapIndex = -1;
}

inline void TimerBase::heapIncreaseKey()
{
    ASSERT(m_nextFireTime != 0);
    heapPop();
    heapDecreaseKey();
}

inline void TimerBase::heapInsert()
{
    ASSERT(!inHeap());
    timerHeap().append(this);
    m_heapIndex = timerHeap().size() - 1;
    heapDecreaseKey();
}

inline void TimerBase::heapPop()
{
    // Temporarily force this timer to have the minimum key so we can pop it.
    double fireTime = m_nextFireTime;
    m_nextFireTime = -numeric_limits<double>::infinity();
    heapDecreaseKey();
    heapPopMin();
    m_nextFireTime = fireTime;
}

void TimerBase::heapPopMin()
{
    ASSERT(this == timerHeap().first());
    checkHeapIndex();
    pop_heap(TimerHeapIterator(0), TimerHeapIterator(timerHeap().size()));
    checkHeapIndex();
    ASSERT(this == timerHeap().last());
}

void TimerBase::setNextFireTime(double newTime)
{
    ASSERT(m_thread == currentThread());

    // Keep heap valid while changing the next-fire time.
    double oldTime = m_nextFireTime;
    if (oldTime != newTime) {
        m_nextFireTime = newTime;
        static unsigned currentHeapInsertionOrder;
        m_heapInsertionOrder = currentHeapInsertionOrder++;

        bool wasFirstTimerInHeap = m_heapIndex == 0;

        if (oldTime == 0)
            heapInsert();
        else if (newTime == 0)
            heapDelete();
        else if (newTime < oldTime)
            heapDecreaseKey();
        else
            heapIncreaseKey();

        bool isFirstTimerInHeap = m_heapIndex == 0;

        if (wasFirstTimerInHeap || isFirstTimerInHeap)
            threadGlobalData().threadTimers().updateSharedTimer();
    }

    checkConsistency();
}

void TimerBase::fireTimersInNestedEventLoop()
{
    // Redirect to ThreadTimers.
    threadGlobalData().threadTimers().fireTimersInNestedEventLoop();
}

} // namespace WebCore

