/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef JSSegmentedVariableObject_h
#define JSSegmentedVariableObject_h

#include "JSObject.h"
#include "JSSymbolTableObject.h"
#include "Register.h"
#include "SymbolTable.h"
#include <wtf/OwnArrayPtr.h>
#include <wtf/SegmentedVector.h>

namespace JSC {

class LLIntOffsetsExtractor;
class Register;

// This is a mostly drop-in replacement for JSVariableObject, except that it preserves
// the invariant that after a variable is created, its address in memory will not change
// so long as the JSSegmentedVariableObject is alive. This allows optimizations based
// on getting the address of the variable and remembering it. As well, unlike a
// JSVariableObject, this will manage the memory for the registers itself and neither
// requires nor allows for the subclasses to manage that memory. Finally,
// JSSegmentedVariableObject has its own GC tracing functionality, since it knows the
// exact dimensions of the variables array at all times.

class JSSegmentedVariableObject : public JSSymbolTableObject {
    friend class JIT;
    friend class LLIntOffsetsExtractor;

public:
    typedef JSSymbolTableObject Base;

    WriteBarrier<Unknown>& registerAt(int index) { return m_registers[index]; }
    
    // This is a slow method call, which searches the register bank to find the index
    // given a pointer. It will CRASH() if it does not find the register. Only use this
    // in debug code (like bytecode dumping).
    JS_EXPORT_PRIVATE int findRegisterIndex(void*);
    
    WriteBarrier<Unknown>* assertRegisterIsInThisObject(WriteBarrier<Unknown>* registerPointer)
    {
#if !ASSERT_DISABLED
        findRegisterIndex(registerPointer);
#endif
        return registerPointer;
    }
    
    // Adds numberOfRegistersToAdd registers, initializes them to Undefined, and returns
    // the index of the first one added.
    JS_EXPORT_PRIVATE int addRegisters(int numberOfRegistersToAdd);
    
    JS_EXPORT_PRIVATE static void visitChildren(JSCell*, SlotVisitor&);

protected:
    static const unsigned StructureFlags = OverridesVisitChildren | JSSymbolTableObject::StructureFlags;

    JSSegmentedVariableObject(VM& vm, Structure* structure, JSScope* scope)
        : JSSymbolTableObject(vm, structure, scope)
    {
    }

    void finishCreation(VM& vm)
    {
        Base::finishCreation(vm);
    }

    SegmentedVector<WriteBarrier<Unknown>, 16> m_registers;
};

} // namespace JSC

#endif // JSSegmentedVariableObject_h

