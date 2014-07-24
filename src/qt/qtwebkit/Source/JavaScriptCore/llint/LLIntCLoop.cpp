/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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
#include "LLIntCLoop.h"

#include "Instruction.h"

namespace JSC {

namespace LLInt {

#if ENABLE(LLINT_C_LOOP)

void CLoop::initialize()
{
    execute(0, llint_unused, true);
}

void* CLoop::catchRoutineFor(Instruction* catchPCForInterpreter)
{
    return reinterpret_cast<Instruction*>(catchPCForInterpreter->u.opcode);
}

MacroAssemblerCodePtr CLoop::hostCodeEntryFor(CodeSpecializationKind kind)
{
    MacroAssemblerCodePtr codePtr;
    codePtr = (kind == CodeForCall) ?
        MacroAssemblerCodePtr::createLLIntCodePtr(llint_native_call_trampoline) :
        MacroAssemblerCodePtr::createLLIntCodePtr(llint_native_construct_trampoline);
    return codePtr;
}

MacroAssemblerCodePtr CLoop::jsCodeEntryWithArityCheckFor(CodeSpecializationKind kind)
{
    MacroAssemblerCodePtr codePtr;
    codePtr = (kind == CodeForCall) ?
        MacroAssemblerCodePtr::createLLIntCodePtr(llint_function_for_call_arity_check) :
        MacroAssemblerCodePtr::createLLIntCodePtr(llint_function_for_construct_arity_check);
    return codePtr;
}

MacroAssemblerCodePtr CLoop::jsCodeEntryFor(CodeSpecializationKind kind)
{
    MacroAssemblerCodePtr codePtr;
    codePtr = (kind == CodeForCall) ?
        MacroAssemblerCodePtr::createLLIntCodePtr(llint_function_for_call_prologue) :
        MacroAssemblerCodePtr::createLLIntCodePtr(llint_function_for_construct_prologue);
    return codePtr;
}

#endif // ENABLE(LLINT_C_LOOP)

} } // namespace JSC::LLInt
