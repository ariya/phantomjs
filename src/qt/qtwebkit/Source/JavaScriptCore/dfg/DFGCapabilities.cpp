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

#include "config.h"
#include "DFGCapabilities.h"

#include "CodeBlock.h"
#include "DFGCommon.h"
#include "Interpreter.h"

namespace JSC { namespace DFG {

#if ENABLE(DFG_JIT)
bool mightCompileEval(CodeBlock* codeBlock)
{
    return codeBlock->instructionCount() <= Options::maximumOptimizationCandidateInstructionCount();
}
bool mightCompileProgram(CodeBlock* codeBlock)
{
    return codeBlock->instructionCount() <= Options::maximumOptimizationCandidateInstructionCount();
}
bool mightCompileFunctionForCall(CodeBlock* codeBlock)
{
    return codeBlock->instructionCount() <= Options::maximumOptimizationCandidateInstructionCount();
}
bool mightCompileFunctionForConstruct(CodeBlock* codeBlock)
{
    return codeBlock->instructionCount() <= Options::maximumOptimizationCandidateInstructionCount();
}

bool mightInlineFunctionForCall(CodeBlock* codeBlock)
{
    return codeBlock->instructionCount() <= Options::maximumFunctionForCallInlineCandidateInstructionCount()
        && !codeBlock->ownerExecutable()->needsActivation()
        && codeBlock->ownerExecutable()->isInliningCandidate();
}
bool mightInlineFunctionForClosureCall(CodeBlock* codeBlock)
{
    return codeBlock->instructionCount() <= Options::maximumFunctionForClosureCallInlineCandidateInstructionCount()
        && !codeBlock->ownerExecutable()->needsActivation()
        && codeBlock->ownerExecutable()->isInliningCandidate();
}
bool mightInlineFunctionForConstruct(CodeBlock* codeBlock)
{
    return codeBlock->instructionCount() <= Options::maximumFunctionForConstructInlineCandidateInstructionCount()
        && !codeBlock->ownerExecutable()->needsActivation()
        && codeBlock->ownerExecutable()->isInliningCandidate();
}

static inline void debugFail(CodeBlock* codeBlock, OpcodeID opcodeID, bool result)
{
    ASSERT_UNUSED(result, !result);
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLogF("Cannot handle code block %p because of opcode %s.\n", codeBlock, opcodeNames[opcodeID]);
#else
    UNUSED_PARAM(codeBlock);
    UNUSED_PARAM(opcodeID);
    UNUSED_PARAM(result);
#endif
}

static inline void debugFail(CodeBlock* codeBlock, OpcodeID opcodeID, CapabilityLevel result)
{
    ASSERT(result != CanCompile);
#if DFG_ENABLE(DEBUG_VERBOSE)
    if (result == CannotCompile)
        dataLogF("Cannot handle code block %p because of opcode %s.\n", codeBlock, opcodeNames[opcodeID]);
    else {
        dataLogF("Cannot compile code block %p because of opcode %s, but inlining might be possible.\n", codeBlock, opcodeNames[opcodeID]);
    }
#else
    UNUSED_PARAM(codeBlock);
    UNUSED_PARAM(opcodeID);
    UNUSED_PARAM(result);
#endif
}

template<typename ReturnType, ReturnType (*canHandleOpcode)(OpcodeID, CodeBlock*, Instruction*)>
ReturnType canHandleOpcodes(CodeBlock* codeBlock, ReturnType initialValue)
{
    Interpreter* interpreter = codeBlock->vm()->interpreter;
    Instruction* instructionsBegin = codeBlock->instructions().begin();
    unsigned instructionCount = codeBlock->instructions().size();
    ReturnType result = initialValue;
    
    for (unsigned bytecodeOffset = 0; bytecodeOffset < instructionCount; ) {
        switch (interpreter->getOpcodeID(instructionsBegin[bytecodeOffset].u.opcode)) {
#define DEFINE_OP(opcode, length) \
        case opcode: { \
            ReturnType current = canHandleOpcode( \
                opcode, codeBlock, instructionsBegin + bytecodeOffset); \
            if (current < result) { \
                result = current; \
                debugFail(codeBlock, opcode, current); \
            } \
            bytecodeOffset += length; \
            break; \
        }
            FOR_EACH_OPCODE_ID(DEFINE_OP)
#undef DEFINE_OP
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
    }
    
    return result;
}

CapabilityLevel canCompileOpcodes(CodeBlock* codeBlock)
{
    if (!MacroAssembler::supportsFloatingPoint())
        return CannotCompile;
    return canHandleOpcodes<CapabilityLevel, canCompileOpcode>(codeBlock, CanCompile);
}

bool canInlineOpcodes(CodeBlock* codeBlock)
{
    return canHandleOpcodes<bool, canInlineOpcode>(codeBlock, true);
}

#endif

} } // namespace JSC::DFG

