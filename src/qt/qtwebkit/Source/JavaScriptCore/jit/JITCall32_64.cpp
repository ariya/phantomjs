/*
 * Copyright (C) 2008, 2013 Apple Inc. All rights reserved.
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
#if USE(JSVALUE32_64)
#include "JIT.h"

#include "Arguments.h"
#include "CodeBlock.h"
#include "Interpreter.h"
#include "JITInlines.h"
#include "JITStubCall.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "Operations.h"
#include "RepatchBuffer.h"
#include "ResultType.h"
#include "SamplingTool.h"
#include <wtf/StringPrintStream.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

using namespace std;

namespace JSC {

void JIT::emit_op_call_put_result(Instruction* instruction)
{
    int dst = instruction[1].u.operand;
    emitValueProfilingSite();
    emitStore(dst, regT1, regT0);
}

void JIT::emit_op_ret(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;

    emitLoad(dst, regT1, regT0);
    emitGetFromCallFrameHeaderPtr(JSStack::ReturnPC, regT2);
    emitGetFromCallFrameHeaderPtr(JSStack::CallerFrame, callFrameRegister);

    restoreReturnAddressBeforeReturn(regT2);
    ret();
}

void JIT::emit_op_ret_object_or_this(Instruction* currentInstruction)
{
    unsigned result = currentInstruction[1].u.operand;
    unsigned thisReg = currentInstruction[2].u.operand;

    emitLoad(result, regT1, regT0);
    Jump notJSCell = branch32(NotEqual, regT1, TrustedImm32(JSValue::CellTag));
    loadPtr(Address(regT0, JSCell::structureOffset()), regT2);
    Jump notObject = emitJumpIfNotObject(regT2);

    emitGetFromCallFrameHeaderPtr(JSStack::ReturnPC, regT2);
    emitGetFromCallFrameHeaderPtr(JSStack::CallerFrame, callFrameRegister);

    restoreReturnAddressBeforeReturn(regT2);
    ret();

    notJSCell.link(this);
    notObject.link(this);
    emitLoad(thisReg, regT1, regT0);

    emitGetFromCallFrameHeaderPtr(JSStack::ReturnPC, regT2);
    emitGetFromCallFrameHeaderPtr(JSStack::CallerFrame, callFrameRegister);

    restoreReturnAddressBeforeReturn(regT2);
    ret();
}

void JIT::emitSlow_op_call(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    compileOpCallSlowCase(op_call, currentInstruction, iter, m_callLinkInfoIndex++);
}

void JIT::emitSlow_op_call_eval(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    compileOpCallSlowCase(op_call_eval, currentInstruction, iter, m_callLinkInfoIndex);
}
 
void JIT::emitSlow_op_call_varargs(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    compileOpCallSlowCase(op_call_varargs, currentInstruction, iter, m_callLinkInfoIndex++);
}

void JIT::emitSlow_op_construct(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    compileOpCallSlowCase(op_construct, currentInstruction, iter, m_callLinkInfoIndex++);
}

void JIT::emit_op_call(Instruction* currentInstruction)
{
    compileOpCall(op_call, currentInstruction, m_callLinkInfoIndex++);
}

void JIT::emit_op_call_eval(Instruction* currentInstruction)
{
    compileOpCall(op_call_eval, currentInstruction, m_callLinkInfoIndex);
}

void JIT::emit_op_call_varargs(Instruction* currentInstruction)
{
    compileOpCall(op_call_varargs, currentInstruction, m_callLinkInfoIndex++);
}

void JIT::emit_op_construct(Instruction* currentInstruction)
{
    compileOpCall(op_construct, currentInstruction, m_callLinkInfoIndex++);
}

void JIT::compileLoadVarargs(Instruction* instruction)
{
    int thisValue = instruction[2].u.operand;
    int arguments = instruction[3].u.operand;
    int firstFreeRegister = instruction[4].u.operand;

    JumpList slowCase;
    JumpList end;
    bool canOptimize = m_codeBlock->usesArguments()
        && arguments == m_codeBlock->argumentsRegister()
        && !m_codeBlock->symbolTable()->slowArguments();

    if (canOptimize) {
        emitLoadTag(arguments, regT1);
        slowCase.append(branch32(NotEqual, regT1, TrustedImm32(JSValue::EmptyValueTag)));

        load32(payloadFor(JSStack::ArgumentCount), regT2);
        slowCase.append(branch32(Above, regT2, TrustedImm32(Arguments::MaxArguments + 1)));
        // regT2: argumentCountIncludingThis

        move(regT2, regT3);
        add32(TrustedImm32(firstFreeRegister + JSStack::CallFrameHeaderSize), regT3);
        lshift32(TrustedImm32(3), regT3);
        addPtr(callFrameRegister, regT3);
        // regT3: newCallFrame

        slowCase.append(branchPtr(Below, AbsoluteAddress(m_vm->interpreter->stack().addressOfEnd()), regT3));

        // Initialize ArgumentCount.
        store32(regT2, payloadFor(JSStack::ArgumentCount, regT3));

        // Initialize 'this'.
        emitLoad(thisValue, regT1, regT0);
        store32(regT0, Address(regT3, OBJECT_OFFSETOF(JSValue, u.asBits.payload) + (CallFrame::thisArgumentOffset() * static_cast<int>(sizeof(Register)))));
        store32(regT1, Address(regT3, OBJECT_OFFSETOF(JSValue, u.asBits.tag) + (CallFrame::thisArgumentOffset() * static_cast<int>(sizeof(Register)))));

        // Copy arguments.
        neg32(regT2);
        end.append(branchAdd32(Zero, TrustedImm32(1), regT2));
        // regT2: -argumentCount;

        Label copyLoop = label();
        load32(BaseIndex(callFrameRegister, regT2, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload) +(CallFrame::thisArgumentOffset() * static_cast<int>(sizeof(Register)))), regT0);
        load32(BaseIndex(callFrameRegister, regT2, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag) +(CallFrame::thisArgumentOffset() * static_cast<int>(sizeof(Register)))), regT1);
        store32(regT0, BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload) +(CallFrame::thisArgumentOffset() * static_cast<int>(sizeof(Register)))));
        store32(regT1, BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag) +(CallFrame::thisArgumentOffset() * static_cast<int>(sizeof(Register)))));
        branchAdd32(NonZero, TrustedImm32(1), regT2).linkTo(copyLoop, this);

        end.append(jump());
    }

    if (canOptimize)
        slowCase.link(this);

    JITStubCall stubCall(this, cti_op_load_varargs);
    stubCall.addArgument(thisValue);
    stubCall.addArgument(arguments);
    stubCall.addArgument(Imm32(firstFreeRegister));
    stubCall.call(regT3);

    if (canOptimize)
        end.link(this);
}

void JIT::compileCallEval()
{
    JITStubCall stubCall(this, cti_op_call_eval); // Initializes ScopeChain; ReturnPC; CodeBlock.
    stubCall.call();
    addSlowCase(branch32(Equal, regT1, TrustedImm32(JSValue::EmptyValueTag)));
    emitGetFromCallFrameHeaderPtr(JSStack::CallerFrame, callFrameRegister);

    sampleCodeBlock(m_codeBlock);
}

void JIT::compileCallEvalSlowCase(Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);

    emitLoad(JSStack::Callee, regT1, regT0);
    emitNakedCall(m_vm->getCTIStub(virtualCallGenerator).code());

    sampleCodeBlock(m_codeBlock);
}

void JIT::compileOpCall(OpcodeID opcodeID, Instruction* instruction, unsigned callLinkInfoIndex)
{
    int callee = instruction[1].u.operand;

    /* Caller always:
        - Updates callFrameRegister to callee callFrame.
        - Initializes ArgumentCount; CallerFrame; Callee.

       For a JS call:
        - Caller initializes ScopeChain.
        - Callee initializes ReturnPC; CodeBlock.
        - Callee restores callFrameRegister before return.

       For a non-JS call:
        - Caller initializes ScopeChain; ReturnPC; CodeBlock.
        - Caller restores callFrameRegister after return.
    */
    
    if (opcodeID == op_call_varargs)
        compileLoadVarargs(instruction);
    else {
        int argCount = instruction[2].u.operand;
        int registerOffset = instruction[3].u.operand;
        
        if (opcodeID == op_call && shouldEmitProfiling()) {
            emitLoad(registerOffset + CallFrame::argumentOffsetIncludingThis(0), regT0, regT1);
            Jump done = branch32(NotEqual, regT0, TrustedImm32(JSValue::CellTag));
            loadPtr(Address(regT1, JSCell::structureOffset()), regT1);
            storePtr(regT1, instruction[5].u.arrayProfile->addressOfLastSeenStructure());
            done.link(this);
        }
    
        addPtr(TrustedImm32(registerOffset * sizeof(Register)), callFrameRegister, regT3);

        store32(TrustedImm32(argCount), payloadFor(JSStack::ArgumentCount, regT3));
    } // regT3 holds newCallFrame with ArgumentCount initialized.
    
    storePtr(TrustedImmPtr(instruction), tagFor(JSStack::ArgumentCount, callFrameRegister));
    emitLoad(callee, regT1, regT0); // regT1, regT0 holds callee.

    storePtr(callFrameRegister, Address(regT3, JSStack::CallerFrame * static_cast<int>(sizeof(Register))));
    emitStore(JSStack::Callee, regT1, regT0, regT3);
    move(regT3, callFrameRegister);

    if (opcodeID == op_call_eval) {
        compileCallEval();
        return;
    }

    DataLabelPtr addressOfLinkedFunctionCheck;
    BEGIN_UNINTERRUPTED_SEQUENCE(sequenceOpCall);
    Jump slowCase = branchPtrWithPatch(NotEqual, regT0, addressOfLinkedFunctionCheck, TrustedImmPtr(0));
    END_UNINTERRUPTED_SEQUENCE(sequenceOpCall);

    addSlowCase(slowCase);
    addSlowCase(branch32(NotEqual, regT1, TrustedImm32(JSValue::CellTag)));

    ASSERT(m_callStructureStubCompilationInfo.size() == callLinkInfoIndex);
    m_callStructureStubCompilationInfo.append(StructureStubCompilationInfo());
    m_callStructureStubCompilationInfo[callLinkInfoIndex].hotPathBegin = addressOfLinkedFunctionCheck;
    m_callStructureStubCompilationInfo[callLinkInfoIndex].callType = CallLinkInfo::callTypeFor(opcodeID);
    m_callStructureStubCompilationInfo[callLinkInfoIndex].bytecodeIndex = m_bytecodeOffset;

    loadPtr(Address(regT0, OBJECT_OFFSETOF(JSFunction, m_scope)), regT1);
    emitPutCellToCallFrameHeader(regT1, JSStack::ScopeChain);
    m_callStructureStubCompilationInfo[callLinkInfoIndex].hotPathOther = emitNakedCall();

    sampleCodeBlock(m_codeBlock);
}

void JIT::compileOpCallSlowCase(OpcodeID opcodeID, Instruction*, Vector<SlowCaseEntry>::iterator& iter, unsigned callLinkInfoIndex)
{
    if (opcodeID == op_call_eval) {
        compileCallEvalSlowCase(iter);
        return;
    }

    linkSlowCase(iter);
    linkSlowCase(iter);
    
    m_callStructureStubCompilationInfo[callLinkInfoIndex].callReturnLocation = emitNakedCall(opcodeID == op_construct ? m_vm->getCTIStub(linkConstructGenerator).code() : m_vm->getCTIStub(linkCallGenerator).code());

    sampleCodeBlock(m_codeBlock);
}

void JIT::privateCompileClosureCall(CallLinkInfo* callLinkInfo, CodeBlock* calleeCodeBlock, Structure* expectedStructure, ExecutableBase* expectedExecutable, MacroAssemblerCodePtr codePtr)
{
    JumpList slowCases;

    slowCases.append(branch32(NotEqual, regT1, TrustedImm32(JSValue::CellTag)));
    slowCases.append(branchPtr(NotEqual, Address(regT0, JSCell::structureOffset()), TrustedImmPtr(expectedStructure)));
    slowCases.append(branchPtr(NotEqual, Address(regT0, JSFunction::offsetOfExecutable()), TrustedImmPtr(expectedExecutable)));
    
    loadPtr(Address(regT0, JSFunction::offsetOfScopeChain()), regT1);
    emitPutCellToCallFrameHeader(regT1, JSStack::ScopeChain);
    
    Call call = nearCall();
    Jump done = jump();
    
    slowCases.link(this);
    move(TrustedImmPtr(callLinkInfo->callReturnLocation.executableAddress()), regT2);
    restoreReturnAddressBeforeReturn(regT2);
    Jump slow = jump();
    
    LinkBuffer patchBuffer(*m_vm, this, m_codeBlock);
    
    patchBuffer.link(call, FunctionPtr(codePtr.executableAddress()));
    patchBuffer.link(done, callLinkInfo->hotPathOther.labelAtOffset(0));
    patchBuffer.link(slow, CodeLocationLabel(m_vm->getCTIStub(virtualCallGenerator).code()));
    
    RefPtr<ClosureCallStubRoutine> stubRoutine = adoptRef(new ClosureCallStubRoutine(
        FINALIZE_CODE(
            patchBuffer,
            ("Baseline closure call stub for %s, return point %p, target %p (%s)",
                toCString(*m_codeBlock).data(),
                callLinkInfo->hotPathOther.labelAtOffset(0).executableAddress(),
                codePtr.executableAddress(),
                toCString(pointerDump(calleeCodeBlock)).data())),
        *m_vm, m_codeBlock->ownerExecutable(), expectedStructure, expectedExecutable,
        callLinkInfo->codeOrigin));
    
    RepatchBuffer repatchBuffer(m_codeBlock);
    
    repatchBuffer.replaceWithJump(
        RepatchBuffer::startOfBranchPtrWithPatchOnRegister(callLinkInfo->hotPathBegin),
        CodeLocationLabel(stubRoutine->code().code()));
    repatchBuffer.relink(callLinkInfo->callReturnLocation, m_vm->getCTIStub(virtualCallGenerator).code());
    
    callLinkInfo->stub = stubRoutine.release();
}

} // namespace JSC

#endif // USE(JSVALUE32_64)
#endif // ENABLE(JIT)
