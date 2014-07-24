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

#ifndef GCThreadSharedData_h
#define GCThreadSharedData_h

#include "ListableHandler.h"
#include "MarkStack.h"
#include "MarkedBlock.h"
#include "UnconditionalFinalizer.h"
#include "WeakReferenceHarvester.h"
#include <wtf/HashSet.h>
#include <wtf/TCSpinLock.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>

namespace JSC {

class GCThread;
class VM;
class CopiedSpace;
class CopyVisitor;

enum GCPhase {
    NoPhase,
    Mark,
    Copy,
    Exit
};

class GCThreadSharedData {
public:
    GCThreadSharedData(VM*);
    ~GCThreadSharedData();
    
    void reset();

    void didStartMarking();
    void didFinishMarking();
    void didStartCopying();
    void didFinishCopying();

#if ENABLE(PARALLEL_GC)
    void resetChildren();
    size_t childVisitCount();
    size_t childDupStrings();
#endif
    
private:
    friend class GCThread;
    friend class SlotVisitor;
    friend class CopyVisitor;

    void getNextBlocksToCopy(size_t&, size_t&);
    void startNextPhase(GCPhase);
    void endCurrentPhase();

    VM* m_vm;
    CopiedSpace* m_copiedSpace;
    
    bool m_shouldHashCons;

    Vector<GCThread*> m_gcThreads;

    Mutex m_markingLock;
    ThreadCondition m_markingCondition;
    MarkStackArray m_sharedMarkStack;
    unsigned m_numberOfActiveParallelMarkers;
    bool m_parallelMarkersShouldExit;

    Mutex m_opaqueRootsLock;
    HashSet<void*> m_opaqueRoots;

    SpinLock m_copyLock;
    Vector<CopiedBlock*> m_blocksToCopy;
    size_t m_copyIndex;
    static const size_t s_blockFragmentLength = 32;

    Mutex m_phaseLock;
    ThreadCondition m_phaseCondition;
    ThreadCondition m_activityCondition;
    unsigned m_numberOfActiveGCThreads;
    bool m_gcThreadsShouldWait;
    GCPhase m_currentPhase;

    ListableHandler<WeakReferenceHarvester>::List m_weakReferenceHarvesters;
    ListableHandler<UnconditionalFinalizer>::List m_unconditionalFinalizers;
};

inline void GCThreadSharedData::getNextBlocksToCopy(size_t& start, size_t& end)
{
    SpinLockHolder locker(&m_copyLock);
    start = m_copyIndex;
    end = std::min(m_blocksToCopy.size(), m_copyIndex + s_blockFragmentLength);
    m_copyIndex = end;
}

} // namespace JSC

#endif
