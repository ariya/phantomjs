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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "GCThread.h"

#include "CopyVisitor.h"
#include "CopyVisitorInlines.h"
#include "GCThreadSharedData.h"
#include "SlotVisitor.h"
#include <wtf/MainThread.h>
#include <wtf/PassOwnPtr.h>

namespace JSC {

GCThread::GCThread(GCThreadSharedData& shared, SlotVisitor* slotVisitor, CopyVisitor* copyVisitor)
    : m_threadID(0)
    , m_shared(shared)
    , m_slotVisitor(WTF::adoptPtr(slotVisitor))
    , m_copyVisitor(WTF::adoptPtr(copyVisitor))
{
}

ThreadIdentifier GCThread::threadID()
{
    ASSERT(m_threadID);
    return m_threadID;
}

void GCThread::initializeThreadID(ThreadIdentifier threadID)
{
    ASSERT(!m_threadID);
    m_threadID = threadID;
}

SlotVisitor* GCThread::slotVisitor()
{
    ASSERT(m_slotVisitor);
    return m_slotVisitor.get();
}

CopyVisitor* GCThread::copyVisitor()
{
    ASSERT(m_copyVisitor);
    return m_copyVisitor.get();
}

GCPhase GCThread::waitForNextPhase()
{
    MutexLocker locker(m_shared.m_phaseLock);
    while (m_shared.m_gcThreadsShouldWait)
        m_shared.m_phaseCondition.wait(m_shared.m_phaseLock);

    m_shared.m_numberOfActiveGCThreads--;
    if (!m_shared.m_numberOfActiveGCThreads)
        m_shared.m_activityCondition.signal();

    while (m_shared.m_currentPhase == NoPhase)
        m_shared.m_phaseCondition.wait(m_shared.m_phaseLock);
    m_shared.m_numberOfActiveGCThreads++;
    return m_shared.m_currentPhase;
}

void GCThread::gcThreadMain()
{
    GCPhase currentPhase;
#if ENABLE(PARALLEL_GC)
    WTF::registerGCThread();
#endif
    // Wait for the main thread to finish creating and initializing us. The main thread grabs this lock before 
    // creating this thread. We aren't guaranteed to have a valid threadID until the main thread releases this lock.
    {
        MutexLocker locker(m_shared.m_phaseLock);
    }
    {
        ParallelModeEnabler enabler(*m_slotVisitor);
        while ((currentPhase = waitForNextPhase()) != Exit) {
            // Note: Each phase is responsible for its own termination conditions. The comments below describe 
            // how each phase reaches termination.
            switch (currentPhase) {
            case Mark:
                m_slotVisitor->drainFromShared(SlotVisitor::SlaveDrain);
                // GCThreads only return from drainFromShared() if the main thread sets the m_parallelMarkersShouldExit 
                // flag in the GCThreadSharedData. The only way the main thread sets that flag is if it realizes 
                // that all of the various subphases in Heap::markRoots() have been fully finished and there is 
                // no more marking work to do and all of the GCThreads are idle, meaning no more work can be generated.
                break;
            case Copy:
                // We don't have to call startCopying() because it's called for us on the main thread to avoid a 
                // race condition.
                m_copyVisitor->copyFromShared();
                // We know we're done copying when we return from copyFromShared() because we would 
                // only do so if there were no more chunks of copying work left to do. When there is no 
                // more copying work to do, the main thread will wait in CopiedSpace::doneCopying() until 
                // all of the blocks that the GCThreads borrowed have been returned. doneCopying() 
                // returns our borrowed CopiedBlock, allowing the copying phase to finish.
                m_copyVisitor->doneCopying();
                break;
            case NoPhase:
                RELEASE_ASSERT_NOT_REACHED();
                break;
            case Exit:
                RELEASE_ASSERT_NOT_REACHED();
                break;
            }
        }
    }
}

void GCThread::gcThreadStartFunc(void* data)
{
    GCThread* thread = static_cast<GCThread*>(data);
    thread->gcThreadMain();
}

} // namespace JSC
