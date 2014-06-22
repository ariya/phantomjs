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
#include "JITStubRoutineSet.h"

#if ENABLE(JIT)

#include "GCAwareJITStubRoutine.h"

#include "SlotVisitor.h"

namespace JSC {

JITStubRoutineSet::JITStubRoutineSet() { }
JITStubRoutineSet::~JITStubRoutineSet()
{
    for (size_t i = m_listOfRoutines.size(); i--;) {
        GCAwareJITStubRoutine* routine = m_listOfRoutines[i];
        
        routine->m_mayBeExecuting = false;
        
        if (!routine->m_isJettisoned) {
            // Inform the deref() routine that it should delete this guy as soon
            // as the ref count reaches zero.
            routine->m_isJettisoned = true;
            continue;
        }
        
        routine->deleteFromGC();
    }
}

void JITStubRoutineSet::add(GCAwareJITStubRoutine* routine)
{
    ASSERT(!routine->m_isJettisoned);
    
    m_listOfRoutines.append(routine);
    
    uintptr_t start = routine->startAddress();
    uintptr_t end = routine->endAddress();
    uintptr_t step = JITStubRoutine::addressStep();
    for (uintptr_t iter = start; iter < end; iter += step) {
        ASSERT(m_addressToRoutineMap.find(iter) == m_addressToRoutineMap.end());
        m_addressToRoutineMap.add(iter, routine);
    }
}

void JITStubRoutineSet::clearMarks()
{
    for (size_t i = m_listOfRoutines.size(); i--;)
        m_listOfRoutines[i]->m_mayBeExecuting = false;
}

void JITStubRoutineSet::markSlow(uintptr_t address)
{
    HashMap<uintptr_t, GCAwareJITStubRoutine*>::iterator iter =
        m_addressToRoutineMap.find(address & ~(JITStubRoutine::addressStep() - 1));
    
    if (iter == m_addressToRoutineMap.end())
        return;
    
    iter->value->m_mayBeExecuting = true;
}

void JITStubRoutineSet::deleteUnmarkedJettisonedStubRoutines()
{
    for (size_t i = 0; i < m_listOfRoutines.size(); i++) {
        GCAwareJITStubRoutine* routine = m_listOfRoutines[i];
        if (!routine->m_isJettisoned || routine->m_mayBeExecuting)
            continue;
        
        uintptr_t start = routine->startAddress();
        uintptr_t end = routine->endAddress();
        uintptr_t step = JITStubRoutine::addressStep();
        for (uintptr_t iter = start; iter < end; iter += step) {
            ASSERT(m_addressToRoutineMap.find(iter) != m_addressToRoutineMap.end());
            ASSERT(m_addressToRoutineMap.find(iter)->value == routine);
            m_addressToRoutineMap.remove(iter);
        }
        
        routine->deleteFromGC();
        
        m_listOfRoutines[i] = m_listOfRoutines.last();
        m_listOfRoutines.removeLast();
        i--;
    }
}

void JITStubRoutineSet::traceMarkedStubRoutines(SlotVisitor& visitor)
{
    for (size_t i = m_listOfRoutines.size(); i--;) {
        GCAwareJITStubRoutine* routine = m_listOfRoutines[i];
        if (!routine->m_mayBeExecuting)
            continue;
        
        routine->markRequiredObjects(visitor);
    }
}

} // namespace JSC

#endif // ENABLE(JIT)

