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
#include "IncrementalSweeper.h"

#include "APIShims.h"
#include "Heap.h"
#include "JSObject.h"
#include "JSString.h"
#include "MarkedBlock.h"

#include <wtf/HashSet.h>
#include <wtf/WTFThreadData.h>

namespace JSC {

#if USE(CF) || PLATFORM(BLACKBERRY) || PLATFORM(QT)

static const double sweepTimeSlice = .01; // seconds
static const double sweepTimeTotal = .10;
static const double sweepTimeMultiplier = 1.0 / sweepTimeTotal;

#if USE(CF)
    
IncrementalSweeper::IncrementalSweeper(Heap* heap, CFRunLoopRef runLoop)
    : HeapTimer(heap->vm(), runLoop)
    , m_currentBlockToSweepIndex(0)
    , m_blocksToSweep(heap->m_blockSnapshot)
{
}

PassOwnPtr<IncrementalSweeper> IncrementalSweeper::create(Heap* heap)
{
    return adoptPtr(new IncrementalSweeper(heap, CFRunLoopGetCurrent()));
}

void IncrementalSweeper::scheduleTimer()
{
    CFRunLoopTimerSetNextFireDate(m_timer.get(), CFAbsoluteTimeGetCurrent() + (sweepTimeSlice * sweepTimeMultiplier));
}

void IncrementalSweeper::cancelTimer()
{
    CFRunLoopTimerSetNextFireDate(m_timer.get(), CFAbsoluteTimeGetCurrent() + s_decade);
}

#elif PLATFORM(BLACKBERRY) || PLATFORM(QT)
   
IncrementalSweeper::IncrementalSweeper(Heap* heap)
    : HeapTimer(heap->vm())
    , m_currentBlockToSweepIndex(0)
    , m_blocksToSweep(heap->m_blockSnapshot)
{
}

PassOwnPtr<IncrementalSweeper> IncrementalSweeper::create(Heap* heap)
{
    return adoptPtr(new IncrementalSweeper(heap));
}

void IncrementalSweeper::scheduleTimer()
{
#if PLATFORM(QT)
    m_timer.start(sweepTimeSlice * sweepTimeMultiplier * 1000, this);
#else
    m_timer.start(sweepTimeSlice * sweepTimeMultiplier);
#endif
}

void IncrementalSweeper::cancelTimer()
{
    m_timer.stop();
}

#endif

void IncrementalSweeper::doWork()
{
    doSweep(WTF::monotonicallyIncreasingTime());
}

void IncrementalSweeper::doSweep(double sweepBeginTime)
{
    while (m_currentBlockToSweepIndex < m_blocksToSweep.size()) {
        sweepNextBlock();

        double elapsedTime = WTF::monotonicallyIncreasingTime() - sweepBeginTime;
        if (elapsedTime < sweepTimeSlice)
            continue;

        scheduleTimer();
        return;
    }

    m_blocksToSweep.clear();
    cancelTimer();
}

void IncrementalSweeper::sweepNextBlock()
{
    while (m_currentBlockToSweepIndex < m_blocksToSweep.size()) {
        MarkedBlock* block = m_blocksToSweep[m_currentBlockToSweepIndex++];

        if (!block->needsSweeping())
            continue;

        block->sweep();
        m_vm->heap.objectSpace().freeOrShrinkBlock(block);
        return;
    }
}

void IncrementalSweeper::startSweeping(Vector<MarkedBlock*>& blockSnapshot)
{
    m_blocksToSweep = blockSnapshot;
    m_currentBlockToSweepIndex = 0;
    scheduleTimer();
}

void IncrementalSweeper::willFinishSweeping()
{
    m_currentBlockToSweepIndex = 0;
    m_blocksToSweep.clear();
    if (m_vm)
        cancelTimer();
}

#else

IncrementalSweeper::IncrementalSweeper(VM* vm)
    : HeapTimer(vm)
{
}

void IncrementalSweeper::doWork()
{
}

PassOwnPtr<IncrementalSweeper> IncrementalSweeper::create(Heap* heap)
{
    return adoptPtr(new IncrementalSweeper(heap->vm()));
}

void IncrementalSweeper::startSweeping(Vector<MarkedBlock*>&)
{
}

void IncrementalSweeper::willFinishSweeping()
{
}

void IncrementalSweeper::sweepNextBlock()
{
}

#endif

} // namespace JSC
