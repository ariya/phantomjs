/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "StackStats.h"

#if ENABLE(STACK_STATS) 

#include "Assertions.h"
#include "DataLog.h"
#include "WTFThreadData.h"

// Define the following flag if you want to collect stats on every single
// checkpoint. By default, we only log checkpoints that establish new
// max values.

// #define ENABLE_VERBOSE_STACK_STATS 1


namespace WTF {

// CheckPoint management:
Mutex* StackStats::s_sharedLock = 0;
StackStats::CheckPoint* StackStats::s_topCheckPoint = 0;
StackStats::LayoutCheckPoint* StackStats::s_firstLayoutCheckPoint = 0;
StackStats::LayoutCheckPoint* StackStats::s_topLayoutCheckPoint = 0;

// High watermark stats:
int StackStats::s_maxCheckPointDiff = 0;
int StackStats::s_maxStackHeight = 0;
int StackStats::s_maxReentryDepth = 0;

int StackStats::s_maxLayoutCheckPointDiff = 0;
int StackStats::s_maxTotalLayoutCheckPointDiff = 0;
int StackStats::s_maxLayoutReentryDepth = 0;


// Initializes locks and the log. Should only be called once.
void StackStats::initialize()
{
    s_sharedLock = new Mutex();
    dataLogF(" === LOG new stack stats ========\n");
}

StackStats::PerThreadStats::PerThreadStats()
{
    const StackBounds& stack = wtfThreadData().stack();
    m_reentryDepth = 0;
    m_stackStart = (char*)stack.origin();
    m_currentCheckPoint = 0;

    dataLogF(" === THREAD new stackStart %p ========\n", m_stackStart);
}

StackStats::CheckPoint::CheckPoint()
{
    MutexLocker locker(*StackStats::s_sharedLock);
    WTFThreadData* threadData = const_cast<WTFThreadData*>(&wtfThreadData());
    StackStats::PerThreadStats& t = threadData->stackStats();
    const StackBounds& stack = threadData->stack();

    bool isGrowingDownward = stack.isGrowingDownward();
    bool needToLog = false;
    char* current = reinterpret_cast<char*>(this);
    char* last = reinterpret_cast<char*>(t.m_currentCheckPoint);

    // If there was no previous checkpoint, measure from the start of the stack:
    if (!last)
        last = t.m_stackStart;

    // Update the reentry depth stats:
    t.m_reentryDepth++;
    if (t.m_reentryDepth > StackStats::s_maxReentryDepth) {
        StackStats::s_maxReentryDepth = t.m_reentryDepth;
        needToLog = true;
    }

    // Update the stack height stats:
    int height = t.m_stackStart - current;
    if (!isGrowingDownward)
        height = -height;
    if (height > StackStats::s_maxStackHeight) {
        StackStats::s_maxStackHeight = height;
        needToLog = true;
    }

    // Update the checkpoint diff stats:
    int diff = last - current;
    if (!isGrowingDownward)
        diff = -diff;
    if (diff > StackStats::s_maxCheckPointDiff) {
        StackStats::s_maxCheckPointDiff = diff;
        needToLog = true;
    }

    // Push this checkpoint:
    m_prev = t.m_currentCheckPoint;
    t.m_currentCheckPoint = this;

#if ENABLE(VERBOSE_STACK_STATS)
    needToLog = true; // always log.
#endif

    // Log this checkpoint if needed:
    if (needToLog)
        dataLogF(" CHECKPOINT %p diff %d/%.1fk/max %.1fk | reentry %d/max %d | height %.1fk/max %.1fk | stack %p size %.1fk\n",
            this, diff, diff / 1024.0, StackStats::s_maxCheckPointDiff / 1024.0,
            t.m_reentryDepth, StackStats::s_maxReentryDepth,
            height / 1024.0, StackStats::s_maxStackHeight / 1024.0,
            stack.origin(), stack.size() / 1024.0);
}

StackStats::CheckPoint::~CheckPoint()
{
    MutexLocker locker(*StackStats::s_sharedLock);
    WTFThreadData* threadData = const_cast<WTFThreadData*>(&wtfThreadData());
    StackStats::PerThreadStats& t = threadData->stackStats();

    // Pop to previous checkpoint:
    t.m_currentCheckPoint = m_prev;
    --t.m_reentryDepth;

    // Log this checkpoint if needed:
#if ENABLE(VERBOSE_STACK_STATS)
    if (!m_prev) {
        const StackBounds& stack = threadData->stack();
        bool isGrowingDownward = stack.isGrowingDownward();

        char* current = reinterpret_cast<char*>(this);
        int height = t.m_stackStart - current;

        if (!isGrowingDownward)
            height = -height;

        dataLogF(" POP to %p diff max %.1fk | reentry %d/%d max | height %.1fk/max %.1fk | stack %p size %.1fk)\n",
            this, StackStats::s_maxCheckPointDiff / 1024.0,
            t.m_reentryDepth, StackStats::s_maxReentryDepth,
            height / 1024.0, StackStats::s_maxStackHeight / 1024.0,
            stack.origin(), stack.size() / 1024.0);
    }
#endif
}

void StackStats::probe()
{
    MutexLocker locker(*StackStats::s_sharedLock);
    WTFThreadData* threadData = const_cast<WTFThreadData*>(&wtfThreadData());
    StackStats::PerThreadStats& t = threadData->stackStats();
    const StackBounds& stack = threadData->stack();

    bool isGrowingDownward = stack.isGrowingDownward();

    bool needToLog = false;

    int dummy;
    char* current = reinterpret_cast<char*>(&dummy);
    char* last = reinterpret_cast<char*>(t.m_currentCheckPoint);

    // If there was no previous checkpoint, measure from the start of the stack:
    if (!last)
        last = t.m_stackStart;

    // We did not reach another checkpoint yet. Hence, we do not touch the
    // reentry stats.

    // Update the stack height stats:
    int height = t.m_stackStart - current;
    if (!isGrowingDownward)
        height = -height;
    if (height > StackStats::s_maxStackHeight) {
        StackStats::s_maxStackHeight = height;
        needToLog = true;
    }

    // Update the checkpoint diff stats:
    int diff = last - current;
    if (!isGrowingDownward)
        diff = -diff;
    if (diff > StackStats::s_maxCheckPointDiff) {
        StackStats::s_maxCheckPointDiff = diff;
        needToLog = true;
    }

#if ENABLE(VERBOSE_STACK_STATS)
    needToLog = true; // always log.
#endif

    if (needToLog)
        dataLogF(" PROBE %p diff %d/%.1fk/max %.1fk | reentry %d/max %d | height %.1fk/max %.1fk | stack %p size %.1fk\n",
            current, diff, diff / 1024.0, StackStats::s_maxCheckPointDiff / 1024.0,
            t.m_reentryDepth, StackStats::s_maxReentryDepth,
            height / 1024.0, StackStats::s_maxStackHeight / 1024.0,
            stack.origin(), stack.size() / 1024.0);
}

StackStats::LayoutCheckPoint::LayoutCheckPoint()
{
    // While a layout checkpoint is not necessarily a checkpoint where we
    // we will do a recursion check, it is a convenient spot for doing a
    // probe to measure the height of stack usage.
    //
    // We'll do this probe before we commence with the layout checkpoint.
    // This is because the probe also locks the sharedLock. By calling the
    // probe first, we can avoid re-entering the lock.
    StackStats::probe();

    MutexLocker locker(*StackStats::s_sharedLock);
    WTFThreadData* threadData = const_cast<WTFThreadData*>(&wtfThreadData());
    StackStats::PerThreadStats& t = threadData->stackStats();
    const StackBounds& stack = threadData->stack();

    bool isGrowingDownward = stack.isGrowingDownward();

    // Push this checkpoint:
    m_prev = StackStats::s_topLayoutCheckPoint;
    if (m_prev)
        m_depth = m_prev->m_depth + 1;
    else {
        StackStats::s_firstLayoutCheckPoint = this;
        m_depth = 0;
    }
    StackStats::s_topLayoutCheckPoint = this;

    // 
    char* current = reinterpret_cast<char*>(this);
    char* last = reinterpret_cast<char*>(m_prev);
    char* root = reinterpret_cast<char*>(StackStats::s_firstLayoutCheckPoint);
    bool needToLog = false;

    int diff = last - current;
    if (!last)
        diff = 0;
    int totalDiff = root - current;
    if (!root)
        totalDiff = 0;

    // Update the stack height stats:
    int height = t.m_stackStart - current;
    if (!isGrowingDownward)
        height = -height;
    if (height > StackStats::s_maxStackHeight) {
        StackStats::s_maxStackHeight = height;
        needToLog = true;
    }

    // Update the layout checkpoint diff stats:
    if (!isGrowingDownward)
        diff = -diff;
    if (diff > StackStats::s_maxLayoutCheckPointDiff) {
        StackStats::s_maxLayoutCheckPointDiff = diff;
        needToLog = true;
    }

    // Update the total layout checkpoint diff stats:
    if (!isGrowingDownward)
        totalDiff = -totalDiff;
    if (totalDiff > StackStats::s_maxTotalLayoutCheckPointDiff) {
        StackStats::s_maxTotalLayoutCheckPointDiff = totalDiff;
        needToLog = true;
    }

#if ENABLE(VERBOSE_STACK_STATS)
    needToLog = true; // always log.
#endif

    if (needToLog)
        dataLogF(" LAYOUT %p diff %d/%.1fk/max %.1fk | reentry %d/max %d | height %.1fk/max %.1fk | stack %p size %.1fk\n",
            current, diff, diff / 1024.0, StackStats::s_maxLayoutCheckPointDiff / 1024.0,
            m_depth, StackStats::s_maxLayoutReentryDepth,
            totalDiff / 1024.0, StackStats::s_maxTotalLayoutCheckPointDiff / 1024.0,
            stack.origin(), stack.size() / 1024.0);
}

StackStats::LayoutCheckPoint::~LayoutCheckPoint()
{
    MutexLocker locker(*StackStats::s_sharedLock);

    // Pop to the previous layout checkpoint:
    StackStats::s_topLayoutCheckPoint = m_prev;
    if (!m_depth)
        StackStats::s_firstLayoutCheckPoint = 0;
}

} // namespace WTF

#endif // ENABLE(STACK_STATS)

