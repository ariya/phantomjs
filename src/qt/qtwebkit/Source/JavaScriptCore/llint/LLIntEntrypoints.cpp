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
#include "LLIntEntrypoints.h"

#if ENABLE(LLINT)

#include "JITCode.h"
#include "VM.h"
#include "JSObject.h"
#include "LLIntThunks.h"
#include "LowLevelInterpreter.h"


namespace JSC { namespace LLInt {

void getFunctionEntrypoint(VM& vm, CodeSpecializationKind kind, JITCode& jitCode, MacroAssemblerCodePtr& arityCheck)
{
    if (!vm.canUseJIT()) {
        if (kind == CodeForCall) {
            jitCode = JITCode(MacroAssemblerCodeRef::createLLIntCodeRef(llint_function_for_call_prologue), JITCode::InterpreterThunk);
            arityCheck = MacroAssemblerCodePtr::createLLIntCodePtr(llint_function_for_call_arity_check);
            return;
        }

        ASSERT(kind == CodeForConstruct);
        jitCode = JITCode(MacroAssemblerCodeRef::createLLIntCodeRef(llint_function_for_construct_prologue), JITCode::InterpreterThunk);
        arityCheck = MacroAssemblerCodePtr::createLLIntCodePtr(llint_function_for_construct_arity_check);
        return;
    }
    
#if ENABLE(JIT)
    if (kind == CodeForCall) {
        jitCode = JITCode(vm.getCTIStub(functionForCallEntryThunkGenerator), JITCode::InterpreterThunk);
        arityCheck = vm.getCTIStub(functionForCallArityCheckThunkGenerator).code();
        return;
    }

    ASSERT(kind == CodeForConstruct);
    jitCode = JITCode(vm.getCTIStub(functionForConstructEntryThunkGenerator), JITCode::InterpreterThunk);
    arityCheck = vm.getCTIStub(functionForConstructArityCheckThunkGenerator).code();
#endif // ENABLE(JIT)
}

void getEvalEntrypoint(VM& vm, JITCode& jitCode)
{
    if (!vm.canUseJIT()) {
        jitCode = JITCode(MacroAssemblerCodeRef::createLLIntCodeRef(llint_eval_prologue), JITCode::InterpreterThunk);
        return;
    }
#if ENABLE(JIT)    
    jitCode = JITCode(vm.getCTIStub(evalEntryThunkGenerator), JITCode::InterpreterThunk);
#endif
}

void getProgramEntrypoint(VM& vm, JITCode& jitCode)
{
    if (!vm.canUseJIT()) {
        jitCode = JITCode(MacroAssemblerCodeRef::createLLIntCodeRef(llint_program_prologue), JITCode::InterpreterThunk);
        return;
    }
#if ENABLE(JIT)
    jitCode = JITCode(vm.getCTIStub(programEntryThunkGenerator), JITCode::InterpreterThunk);
#endif
}

} } // namespace JSC::LLInt

#endif // ENABLE(LLINT)
