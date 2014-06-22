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

#ifndef JITDriver_h
#define JITDriver_h

#include <wtf/Platform.h>

#if ENABLE(JIT)

#include "BytecodeGenerator.h"
#include "DFGDriver.h"
#include "JIT.h"
#include "LLIntEntrypoints.h"

namespace JSC {

template<typename CodeBlockType>
inline bool jitCompileIfAppropriate(ExecState* exec, OwnPtr<CodeBlockType>& codeBlock, JITCode& jitCode, JITCode::JITType jitType, unsigned bytecodeIndex, JITCompilationEffort effort)
{
    VM& vm = exec->vm();
    
    if (jitType == codeBlock->getJITType())
        return true;
    
    if (!vm.canUseJIT())
        return true;
    
    codeBlock->unlinkIncomingCalls();
    
    JITCode oldJITCode = jitCode;
    
    bool dfgCompiled = false;
    if (jitType == JITCode::DFGJIT)
        dfgCompiled = DFG::tryCompile(exec, codeBlock.get(), jitCode, bytecodeIndex);
    if (dfgCompiled) {
        if (codeBlock->alternative())
            codeBlock->alternative()->unlinkIncomingCalls();
    } else {
        if (codeBlock->alternative()) {
            codeBlock = static_pointer_cast<CodeBlockType>(codeBlock->releaseAlternative());
            jitCode = oldJITCode;
            return false;
        }
        jitCode = JIT::compile(&vm, codeBlock.get(), effort);
        if (!jitCode) {
            jitCode = oldJITCode;
            return false;
        }
    }
    codeBlock->setJITCode(jitCode, MacroAssemblerCodePtr());
    
    return true;
}

inline bool jitCompileFunctionIfAppropriate(ExecState* exec, OwnPtr<FunctionCodeBlock>& codeBlock, JITCode& jitCode, MacroAssemblerCodePtr& jitCodeWithArityCheck, JITCode::JITType jitType, unsigned bytecodeIndex, JITCompilationEffort effort)
{
    VM& vm = exec->vm();
    
    if (jitType == codeBlock->getJITType())
        return true;
    
    if (!vm.canUseJIT())
        return true;
    
    codeBlock->unlinkIncomingCalls();
    
    JITCode oldJITCode = jitCode;
    MacroAssemblerCodePtr oldJITCodeWithArityCheck = jitCodeWithArityCheck;
    
    bool dfgCompiled = false;
    if (jitType == JITCode::DFGJIT)
        dfgCompiled = DFG::tryCompileFunction(exec, codeBlock.get(), jitCode, jitCodeWithArityCheck, bytecodeIndex);
    if (dfgCompiled) {
        if (codeBlock->alternative())
            codeBlock->alternative()->unlinkIncomingCalls();
    } else {
        if (codeBlock->alternative()) {
            codeBlock = static_pointer_cast<FunctionCodeBlock>(codeBlock->releaseAlternative());
            jitCode = oldJITCode;
            jitCodeWithArityCheck = oldJITCodeWithArityCheck;
            return false;
        }
        jitCode = JIT::compile(&vm, codeBlock.get(), effort, &jitCodeWithArityCheck);
        if (!jitCode) {
            jitCode = oldJITCode;
            jitCodeWithArityCheck = oldJITCodeWithArityCheck;
            return false;
        }
    }
    codeBlock->setJITCode(jitCode, jitCodeWithArityCheck);
    
    return true;
}

} // namespace JSC

#endif // ENABLE(JIT)

#endif // JITDriver_h

