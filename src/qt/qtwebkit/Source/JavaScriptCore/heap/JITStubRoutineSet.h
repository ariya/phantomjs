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

#ifndef JITStubRoutineSet_h
#define JITStubRoutineSet_h

#include <wtf/Platform.h>

#include "JITStubRoutine.h"
#include <wtf/FastAllocBase.h>
#include <wtf/HashMap.h>
#include <wtf/Vector.h>

namespace JSC {

class GCAwareJITStubRoutine;
class SlotVisitor;

#if ENABLE(JIT)

class JITStubRoutineSet {
    WTF_MAKE_NONCOPYABLE(JITStubRoutineSet);
    WTF_MAKE_FAST_ALLOCATED;
    
public:
    JITStubRoutineSet();
    ~JITStubRoutineSet();
    
    void add(GCAwareJITStubRoutine*);

    void clearMarks();
    
    void mark(void* candidateAddress)
    {
        uintptr_t address = reinterpret_cast<uintptr_t>(candidateAddress);
        if (!JITStubRoutine::passesFilter(address))
            return;
        
        markSlow(address);
    }
    
    void deleteUnmarkedJettisonedStubRoutines();
    
    void traceMarkedStubRoutines(SlotVisitor&);
    
    unsigned size() const { return m_listOfRoutines.size(); }
    GCAwareJITStubRoutine* at(unsigned i) const { return m_listOfRoutines[i]; }
    
private:
    void markSlow(uintptr_t address);
    
    HashMap<uintptr_t, GCAwareJITStubRoutine*> m_addressToRoutineMap;
    Vector<GCAwareJITStubRoutine*> m_listOfRoutines;
};

#else // !ENABLE(JIT)

class JITStubRoutineSet {
    WTF_MAKE_NONCOPYABLE(JITStubRoutineSet);
    WTF_MAKE_FAST_ALLOCATED;
    
public:
    JITStubRoutineSet() { }
    ~JITStubRoutineSet() { }

    void add(GCAwareJITStubRoutine*) { }
    void clearMarks() { }
    void mark(void*) { }
    void deleteUnmarkedJettisonedStubRoutines() { }
    void traceMarkedStubRoutines(SlotVisitor&) { }
};

#endif // !ENABLE(JIT)

} // namespace JSC

#endif // JITStubRoutineSet_h

