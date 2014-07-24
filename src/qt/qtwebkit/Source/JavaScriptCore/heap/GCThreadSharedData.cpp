/*
 * Copyright (C) 2009, 2011 Apple Inc. All rights reserved.
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
#include "GCThreadSharedData.h"

#include "CopyVisitor.h"
#include "CopyVisitorInlines.h"
#include "GCThread.h"
#include "VM.h"
#include "MarkStack.h"
#include "SlotVisitor.h"
#include "SlotVisitorInlines.h"

namespace JSC {

#if ENABLE(PARALLEL_GC)
void GCThreadSharedData::resetChildren()
{
    for (size_t i = 0; i < m_gcThreads.size(); ++i)
        m_gcThreads[i]->slotVisitor()->reset();
}

size_t GCThreadSharedData::childVisitCount()
{       
    unsigned long result = 0;
    for (unsigned i = 0; i < m_gcThreads.size(); ++i)
        result += m_gcThreads[i]->slotVisitor()->visitCount();
    return result;
}
#endif

GCThreadSharedData::GCThreadSharedData(VM* vm)
    : m_vm(vm)
    , m_copiedSpace(&vm->heap.m_storageSpace)
    , m_shouldHashCons(false)
    , m_sharedMarkStack(vm->heap.blockAllocator())
    , m_numberOfActiveParallelMarkers(0)
    , m_parallelMarkersShouldExit(false)
    , m_copyIndex(0)
    , m_numberOfActiveGCThreads(0)
    , m_gcThreadsShouldWait(false)
    , m_currentPhase(NoPhase)
{
    m_copyLock.Init();
#if ENABLE(PARALLEL_GC)
    // Grab the lock so the new GC threads can be properly initialized before they start running.
    MutexLocker locker(m_phaseLock);
    for (unsigned i = 1; i < Options::numberOfGCMarkers(); ++i) {
        m_numberOfActiveGCThreads++;
        SlotVisitor* slotVisitor = new SlotVisitor(*this);
        CopyVisitor* copyVisitor = new CopyVisitor(*this);
        GCThread* newThread = new GCThread(*this, slotVisitor, copyVisitor);
        ThreadIdentifier threadID = createThread(GCThread::gcThreadStartFunc, newThread, "JavaScriptCore::Marking");
        newThread->initializeThreadID(threadID);
        m_gcThreads.append(newThread);
    }

    // Wait for all the GCThreads to get to the right place.
    while (m_numberOfActiveGCThreads)
        m_activityCondition.wait(m_phaseLock);
#endif
}

GCThreadSharedData::~GCThreadSharedData()
{
#if ENABLE(PARALLEL_GC)    
    // Destroy our marking threads.
    {
        MutexLocker markingLocker(m_markingLock);
        MutexLocker phaseLocker(m_phaseLock);
        ASSERT(m_currentPhase == NoPhase);
        m_parallelMarkersShouldExit = true;
        m_gcThreadsShouldWait = false;
        m_currentPhase = Exit;
        m_phaseCondition.broadcast();
    }
    for (unsigned i = 0; i < m_gcThreads.size(); ++i) {
        waitForThreadCompletion(m_gcThreads[i]->threadID());
        delete m_gcThreads[i];
    }
#endif
}
    
void GCThreadSharedData::reset()
{
    ASSERT(m_sharedMarkStack.isEmpty());
    
#if ENABLE(PARALLEL_GC)
    m_opaqueRoots.clear();
#else
    ASSERT(m_opaqueRoots.isEmpty());
#endif
    m_weakReferenceHarvesters.removeAll();

    if (m_shouldHashCons) {
        m_vm->resetNewStringsSinceLastHashCons();
        m_shouldHashCons = false;
    }
}

void GCThreadSharedData::startNextPhase(GCPhase phase)
{
    MutexLocker phaseLocker(m_phaseLock);
    ASSERT(!m_gcThreadsShouldWait);
    ASSERT(m_currentPhase == NoPhase);
    m_gcThreadsShouldWait = true;
    m_currentPhase = phase;
    m_phaseCondition.broadcast();
}

void GCThreadSharedData::endCurrentPhase()
{
    ASSERT(m_gcThreadsShouldWait);
    MutexLocker locker(m_phaseLock);
    m_currentPhase = NoPhase;
    m_gcThreadsShouldWait = false;
    m_phaseCondition.broadcast();
    while (m_numberOfActiveGCThreads)
        m_activityCondition.wait(m_phaseLock);
}

void GCThreadSharedData::didStartMarking()
{
    MutexLocker markingLocker(m_markingLock);
    m_parallelMarkersShouldExit = false;
    startNextPhase(Mark);
}

void GCThreadSharedData::didFinishMarking()
{
    {
        MutexLocker markingLocker(m_markingLock);
        m_parallelMarkersShouldExit = true;
        m_markingCondition.broadcast();
    }

    ASSERT(m_currentPhase == Mark);
    endCurrentPhase();
}

void GCThreadSharedData::didStartCopying()
{
    {
        SpinLockHolder locker(&m_copyLock);
        WTF::copyToVector(m_copiedSpace->m_blockSet, m_blocksToCopy);
        m_copyIndex = 0;
    }

    // We do this here so that we avoid a race condition where the main thread can 
    // blow through all of the copying work before the GCThreads fully wake up. 
    // The GCThreads then request a block from the CopiedSpace when the copying phase 
    // has completed, which isn't allowed.
    for (size_t i = 0; i < m_gcThreads.size(); i++)
        m_gcThreads[i]->copyVisitor()->startCopying();

    startNextPhase(Copy);
}

void GCThreadSharedData::didFinishCopying()
{
    ASSERT(m_currentPhase == Copy);
    endCurrentPhase();
}

} // namespace JSC
