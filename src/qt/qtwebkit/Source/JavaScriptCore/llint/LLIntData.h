/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef LLIntData_h
#define LLIntData_h

#include "JSCJSValue.h"
#include "Opcode.h"
#include <wtf/Platform.h>

namespace JSC {

class VM;
struct Instruction;

#if ENABLE(LLINT_C_LOOP)
typedef OpcodeID LLIntCode;
#else
typedef void (*LLIntCode)();
#endif

namespace LLInt {

#if ENABLE(LLINT)

class Data {
public:
    static void performAssertions(VM&);

private:
    static Instruction* s_exceptionInstructions;
    static Opcode* s_opcodeMap;

    friend void initialize();

    friend Instruction* exceptionInstructions();
    friend Opcode* opcodeMap();
    friend Opcode getOpcode(OpcodeID);
    friend void* getCodePtr(OpcodeID);
};

void initialize();

inline Instruction* exceptionInstructions()
{
    return Data::s_exceptionInstructions;
}
    
inline Opcode* opcodeMap()
{
    return Data::s_opcodeMap;
}

inline Opcode getOpcode(OpcodeID id)
{
#if ENABLE(COMPUTED_GOTO_OPCODES)
    return Data::s_opcodeMap[id];
#else
    return static_cast<Opcode>(id);
#endif
}

ALWAYS_INLINE void* getCodePtr(OpcodeID id)
{
    return reinterpret_cast<void*>(getOpcode(id));
}

#else // !ENABLE(LLINT)

#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#endif

class Data {
public:
    static void performAssertions(VM&) { }
};

#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif

#endif // !ENABLE(LLINT)

ALWAYS_INLINE void* getOpcode(void llintOpcode())
{
    return bitwise_cast<void*>(llintOpcode);
}

ALWAYS_INLINE void* getCodePtr(void glueHelper())
{
    return bitwise_cast<void*>(glueHelper);
}

ALWAYS_INLINE void* getCodePtr(JSC::EncodedJSValue glueHelper())
{
    return bitwise_cast<void*>(glueHelper);
}


} } // namespace JSC::LLInt

#endif // LLIntData_h

