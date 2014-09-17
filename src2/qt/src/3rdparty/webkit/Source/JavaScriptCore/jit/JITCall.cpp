/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#if ENABLE(JIT)
#if USE(JSVALUE64)
#include "JIT.h"

#include "CodeBlock.h"
#include "JITInlineMethods.h"
#include "JITStubCall.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "Interpreter.h"
#include "ResultType.h"
#include "SamplingTool.h"

#ifndef NDEBUG
#include <stdio.h>
#endif

using namespace std;

namespace JSC {

void JIT::compileOpCallInitializeCallFrame()
{
    // regT0 holds callee, regT1 holds argCount
    loadPtr(Address(regT0, OBJECT_OFFSETOF(JSFunction, m_scopeChain)), regT3); // scopeChain
    emitPutIntToCallFrameHeader(regT1, RegisterFile::ArgumentCount);
    emitPutCellToCallFrameHeader(regT0, RegisterFile::Callee);
    emitPutCellToCallFrameHeader(regT3, RegisterFile::ScopeChain);
}

void JIT::emit_op_call_put_result(Instruction* instruction)
{
    int dst = instruction[1].u.operand;
    emitPutVirtualRegister(dst);
}

void JIT::compileOpCallVarargs(Instruction* instruction)
{
    int callee = instruction[1].u.operand;
    int argCountRegister = instruction[2].u.operand;
    int registerOffset = instruction[3].u.operand;

    emitGetVirtualRegister(argCountRegister, regT1);
    emitFastArithImmToInt(regT1);
    emitGetVirtualRegister(callee, regT0);
    addPtr(Imm32(registerOffset), regT1, regT2);

    // Check for JSFunctions.
    emitJumpSlowCaseIfNotJSCell(regT0);
    addSlowCase(branchPtr(NotEqual, Address(regT0), TrustedImmPtr(m_globalData->jsFunctionVPtr)));

    // Speculatively roll the callframe, assuming argCount will match the arity.
    mul32(TrustedImm32(sizeof(Register)), regT2, regT2);
    intptr_t offset = (intptr_t)sizeof(Register) * (intptr_t)RegisterFile::CallerFrame;
    addPtr(Imm32((int32_t)offset), regT2, regT3);
    addPtr(callFrameRegister, regT3);
    storePtr(callFrameRegister, regT3);
    addPtr(regT2, callFrameRegister);
    emitNakedCall(m_globalData->jitStubs->ctiVirtualCall());

    sampleCodeBlock(m_codeBlock);
}

void JIT::compileOpCallVarargsSlowCase(Instruction*, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);
    linkSlowCase(iter);

    JITStubCall stubCall(this, cti_op_call_NotJSFunction);
    stubCall.addArgument(regT0);
    stubCall.addArgument(regT2);
    stubCall.addArgument(regT1);
    stubCall.call();
    
    sampleCodeBlock(m_codeBlock);
}
    
#if !ENABLE(JIT_OPTIMIZE_CALL)

/* ------------------------------ BEGIN: !ENABLE(JIT_OPTIMIZE_CALL) ------------------------------ */

void JIT::compileOpCall(OpcodeID opcodeID, Instruction* instruction, unsigned)
{
    int callee = instruction[1].u.operand;
    int argCount = instruction[2].u.operand;
    int registerOffset = instruction[3].u.operand;

    // Handle eval
    Jump wasEval;
    if (opcodeID == op_call_eval) {
        JITStubCall stubCall(this, cti_op_call_eval);
        stubCall.addArgument(callee, regT0);
        stubCall.addArgument(JIT::Imm32(registerOffset));
        stubCall.addArgument(JIT::Imm32(argCount));
        stubCall.call();
        wasEval = branchPtr(NotEqual, regT0, TrustedImmPtr(JSValue::encode(JSValue())));
    }

    emitGetVirtualRegister(callee, regT0);

    // Check for JSFunctions.
    emitJumpSlowCaseIfNotJSCell(regT0);
    addSlowCase(branchPtr(NotEqual, Address(regT0), TrustedImmPtr(m_globalData->jsFunctionVPtr)));

    // Speculatively roll the callframe, assuming argCount will match the arity.
    storePtr(callFrameRegister, Address(callFrameRegister, (RegisterFile::CallerFrame + registerOffset) * static_cast<int>(sizeof(Register))));
    addPtr(Imm32(registerOffset * static_cast<int>(sizeof(Register))), callFrameRegister);
    move(Imm32(argCount), regT1);

    emitNakedCall(opcodeID == op_construct ? m_globalData->jitStubs->ctiVirtualConstruct() : m_globalData->jitStubs->ctiVirtualCall());

    if (opcodeID == op_call_eval)
        wasEval.link(this);

    sampleCodeBlock(m_codeBlock);
}

void JIT::compileOpCallSlowCase(Instruction* instruction, Vector<SlowCaseEntry>::iterator& iter, unsigned, OpcodeID opcodeID)
{
    int argCount = instruction[2].u.operand;
    int registerOffset = instruction[3].u.operand;

    linkSlowCase(iter);
    linkSlowCase(iter);

    JITStubCall stubCall(this, opcodeID == op_construct ? cti_op_construct_NotJSConstruct : cti_op_call_NotJSFunction);
    stubCall.addArgument(regT0);
    stubCall.addArgument(JIT::Imm32(registerOffset));
    stubCall.addArgument(JIT::Imm32(argCount));
    stubCall.call();

    sampleCodeBlock(m_codeBlock);
}

#else // !ENABLE(JIT_OPTIMIZE_CALL)

/* ------------------------------ BEGIN: ENABLE(JIT_OPTIMIZE_CALL) ------------------------------ */

void JIT::compileOpCall(OpcodeID opcodeID, Instruction* instruction, unsigned callLinkInfoIndex)
{
    int callee = instruction[1].u.operand;
    int argCount = instruction[2].u.operand;
    int registerOffset = instruction[3].u.operand;

    // Handle eval
    Jump wasEval;
    if (opcodeID == op_call_eval) {
        JITStubCall stubCall(this, cti_op_call_eval);
        stubCall.addArgument(callee, regT0);
        stubCall.addArgument(JIT::Imm32(registerOffset));
        stubCall.addArgument(JIT::Imm32(argCount));
        stubCall.call();
        wasEval = branchPtr(NotEqual, regT0, TrustedImmPtr(JSValue::encode(JSValue())));
    }

    // This plants a check for a cached JSFunction value, so we can plant a fast link to the callee.
    // This deliberately leaves the callee in ecx, used when setting up the stack frame below
    emitGetVirtualRegister(callee, regT0);
    DataLabelPtr addressOfLinkedFunctionCheck;

    BEGIN_UNINTERRUPTED_SEQUENCE(sequenceOpCall);

    Jump jumpToSlow = branchPtrWithPatch(NotEqual, regT0, addressOfLinkedFunctionCheck, TrustedImmPtr(JSValue::encode(JSValue())));

    END_UNINTERRUPTED_SEQUENCE(sequenceOpCall);

    addSlowCase(jumpToSlow);
    ASSERT_JIT_OFFSET(differenceBetween(addressOfLinkedFunctionCheck, jumpToSlow), patchOffsetOpCallCompareToJump);
    m_callStructureStubCompilationInfo[callLinkInfoIndex].hotPathBegin = addressOfLinkedFunctionCheck;

    // The following is the fast case, only used whan a callee can be linked.

    // Fast version of stack frame initialization, directly relative to edi.
    // Note that this omits to set up RegisterFile::CodeBlock, which is set in the callee

    loadPtr(Address(regT0, OBJECT_OFFSETOF(JSFunction, m_scopeChain)), regT1); // newScopeChain
    
    store32(TrustedImm32(Int32Tag), intTagFor(registerOffset + RegisterFile::ArgumentCount));
    store32(Imm32(argCount), intPayloadFor(registerOffset + RegisterFile::ArgumentCount));
    storePtr(callFrameRegister, Address(callFrameRegister, (registerOffset + RegisterFile::CallerFrame) * static_cast<int>(sizeof(Register))));
    storePtr(regT0, Address(callFrameRegister, (registerOffset + RegisterFile::Callee) * static_cast<int>(sizeof(Register))));
    storePtr(regT1, Address(callFrameRegister, (registerOffset + RegisterFile::ScopeChain) * static_cast<int>(sizeof(Register))));
    addPtr(Imm32(registerOffset * sizeof(Register)), callFrameRegister);

    // Call to the callee
    m_callStructureStubCompilationInfo[callLinkInfoIndex].hotPathOther = emitNakedCall();
    
    if (opcodeID == op_call_eval)
        wasEval.link(this);

    sampleCodeBlock(m_codeBlock);
}

void JIT::compileOpCallSlowCase(Instruction* instruction, Vector<SlowCaseEntry>::iterator& iter, unsigned callLinkInfoIndex, OpcodeID opcodeID)
{
    int argCount = instruction[2].u.operand;
    int registerOffset = instruction[3].u.operand;

    linkSlowCase(iter);

    // Fast check for JS function.
    Jump callLinkFailNotObject = emitJumpIfNotJSCell(regT0);
    Jump callLinkFailNotJSFunction = branchPtr(NotEqual, Address(regT0), TrustedImmPtr(m_globalData->jsFunctionVPtr));

    // Speculatively roll the callframe, assuming argCount will match the arity.
    storePtr(callFrameRegister, Address(callFrameRegister, (RegisterFile::CallerFrame + registerOffset) * static_cast<int>(sizeof(Register))));
    addPtr(Imm32(registerOffset * static_cast<int>(sizeof(Register))), callFrameRegister);
    move(Imm32(argCount), regT1);

    m_callStructureStubCompilationInfo[callLinkInfoIndex].callReturnLocation = emitNakedCall(opcodeID == op_construct ? m_globalData->jitStubs->ctiVirtualConstructLink() : m_globalData->jitStubs->ctiVirtualCallLink());

    // Done! - return back to the hot path.
    ASSERT(OPCODE_LENGTH(op_call) == OPCODE_LENGTH(op_call_eval));
    ASSERT(OPCODE_LENGTH(op_call) == OPCODE_LENGTH(op_construct));
    emitJumpSlowToHot(jump(), OPCODE_LENGTH(op_call));

    // This handles host functions
    callLinkFailNotObject.link(this);
    callLinkFailNotJSFunction.link(this);

    JITStubCall stubCall(this, opcodeID == op_construct ? cti_op_construct_NotJSConstruct : cti_op_call_NotJSFunction);
    stubCall.addArgument(regT0);
    stubCall.addArgument(JIT::Imm32(registerOffset));
    stubCall.addArgument(JIT::Imm32(argCount));
    stubCall.call();

    sampleCodeBlock(m_codeBlock);
}

/* ------------------------------ END: !ENABLE / ENABLE(JIT_OPTIMIZE_CALL) ------------------------------ */

#endif // !ENABLE(JIT_OPTIMIZE_CALL)

} // namespace JSC

#endif // USE(JSVALUE64)
#endif // ENABLE(JIT)
