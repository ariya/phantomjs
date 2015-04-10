/*
 * Copyright (C) 2007, 2008, 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JSVariableObject_h
#define JSVariableObject_h

#include "JSObject.h"
#include "JSSymbolTableObject.h"
#include "Register.h"
#include "SymbolTable.h"
#include <wtf/OwnArrayPtr.h>

namespace JSC {

    class LLIntOffsetsExtractor;
    class Register;

    class JSVariableObject : public JSSymbolTableObject {
        friend class JIT;
        friend class LLIntOffsetsExtractor;

    public:
        typedef JSSymbolTableObject Base;

        WriteBarrierBase<Unknown>& registerAt(int index) const { return m_registers[index]; }

        WriteBarrierBase<Unknown>* const * addressOfRegisters() const { return &m_registers; }
        static size_t offsetOfRegisters() { return OBJECT_OFFSETOF(JSVariableObject, m_registers); }

        static const ClassInfo s_info;

    protected:
        static const unsigned StructureFlags = Base::StructureFlags;

        JSVariableObject(
            VM& vm,
            Structure* structure,
            Register* registers,
            JSScope* scope,
            SharedSymbolTable* symbolTable = 0
        )
            : Base(vm, structure, scope, symbolTable)
            , m_registers(reinterpret_cast<WriteBarrierBase<Unknown>*>(registers))
        {
        }

        WriteBarrierBase<Unknown>* m_registers; // "r" in the stack.
    };

} // namespace JSC

#endif // JSVariableObject_h
