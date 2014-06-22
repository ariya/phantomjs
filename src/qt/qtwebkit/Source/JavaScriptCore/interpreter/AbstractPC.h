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

#ifndef AbstractPC_h
#define AbstractPC_h

#include "MacroAssemblerCodeRef.h"
#include <wtf/Platform.h>

namespace JSC {

class VM;
class ExecState;
struct Instruction;

class AbstractPC {
public:
    AbstractPC()
        : m_pointer(0)
        , m_mode(None)
    {
    }
    
    AbstractPC(VM&, ExecState*);
    
#if ENABLE(JIT)
    AbstractPC(ReturnAddressPtr ptr)
        : m_pointer(ptr.value())
        , m_mode(JIT)
    {
    }
    
    bool hasJITReturnAddress() const { return m_mode == JIT; }
    ReturnAddressPtr jitReturnAddress() const
    {
        ASSERT(hasJITReturnAddress());
        return ReturnAddressPtr(m_pointer);
    }
#endif

    bool isSet() const { return m_mode != None; }
    bool operator!() const { return !isSet(); }

private:
    void* m_pointer;
    
    enum Mode { None, JIT, Interpreter };
    Mode m_mode;
};

} // namespace JSC

#endif // AbstractPC_h

