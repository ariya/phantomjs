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

#ifndef ClosureCallStubRoutine_h
#define ClosureCallStubRoutine_h

#include <wtf/Platform.h>

#if ENABLE(JIT)

#include "CodeOrigin.h"
#include "GCAwareJITStubRoutine.h"

namespace JSC {

class ClosureCallStubRoutine : public GCAwareJITStubRoutine {
public:
    ClosureCallStubRoutine(
        const MacroAssemblerCodeRef&, VM&, const JSCell* owner,
        Structure*, ExecutableBase*, const CodeOrigin&);
    
    virtual ~ClosureCallStubRoutine();
    
    Structure* structure() const { return m_structure.get(); }
    ExecutableBase* executable() const { return m_executable.get(); }
    const CodeOrigin& codeOrigin() const { return m_codeOrigin; }

protected:
    virtual void markRequiredObjectsInternal(SlotVisitor&);

private:
    WriteBarrier<Structure> m_structure;
    WriteBarrier<ExecutableBase> m_executable;
    // This allows us to figure out who a call is linked to by searching through
    // stub routines.
    CodeOrigin m_codeOrigin;
};

} // namespace JSC

#endif // ENABLE(JIT)

#endif // ClosureCallStubRoutine_h

