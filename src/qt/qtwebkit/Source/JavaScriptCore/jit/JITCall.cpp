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
#if USE(JSVALUE64)
#include "JIT.h"

#include "Arguments.h"
#include "CodeBlock.h"
#include "JITInlines.h"
#include "JITStubCall.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "Interpreter.h"
#include "Operations.h"
#include "RepatchBuffer.h"
#include "ResultType.h"
#include "SamplingTool.h"
#include "ThunkGenerators.h"
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
    emitPutVirtualRegister(dst);
    if (canBeOptimizedOrInlined())
        killLastResultRegister(); // Make lastResultRegister tracking simpler in the DFG.
}

void JIT::compileLoadVarargs(Instruction* instruction)
{
    int thisValue = instruction[2].u.operand;
    int arguments = instruction[3].u.operand;
    int firstFreeRegister = instruction[4].u.operand;

    killLastResultRegister();

    JumpList slowCase;
    JumpList end;
    bool canOptimize = m_codeBlock->usesArguments()
        && arguments == m_codeBlock->argumentsRegister()
        && !m_codeBlock->symbolTable()->slowArguments();

    if (canOptimize) {
        emitGetVirtualRegister(arguments, regT0);
        slowCase.append(branch64(NotEqual, regT0, TrustedImm64(JSValue::encode(JSValue()))));

        emitGetFromCallFrameHeader32(JSStack::ArgumentCount, regT0);
        slowCase.append(branch32(Above, regT0, TrustedImm32(Arguments::MaxArguments + 1)));
        // regT0: argumentCountIncludingThis

        move(regT0, regT1);
        add32(TrustedImm32(firstFreeRegister + JSStack::CallFrameHeaderSize), regT1);
        lshift32(TrustedImm32(3), regT1);
        addPtr(callFrameRegister, regT1);
        // regT1: newCallFrame

        slowCase.append(branchPtr(Below, AbsoluteAddress(m_vm->interpreter->stack().addressOfEnd()), regT1));

        // Initialize ArgumentCount.
        store32(regT0, Address(regT1, JSStack::ArgumentCount * static_cast<int>(sizeof(Register)) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));

        // Initialize 'this'.
        emitGetVirtualRegister(thisValue, regT2);
        store64(regT2, Address(regT1, CallFrame::thisArgumentOffset() * static_cast<int>(sizeof(Register))));

        // Copy arguments.
        neg32(regT0);
        signExtend32ToPtr(regT0, regT0);
        end.append(branchAdd64(Zero, TrustedImm32(1), regT0));
        // regT0: -argumentCount

        Label copyLoop = label();
        load64(BaseIndex(callFrameRegister, regT0, TimesEight, CallFrame::thisArgumentOffset() * static_cast<int>(sizeof(Register))), regT2);
        store64(regT2, BaseIndex(regT1, regT0, TimesEight, CallFrame::thisArgumentOffset() * static_cast<int>(sizeof(Register))));
        branchAdd64(NonZero, TrustedImm32(1), regT0).linkTo(copyLoop, this);

        end.append(jump());
    }

    if (canOptimize)
        slowCase.link(this);

    JITStubCall stubCall(this, cti_op_load_varargs);
    stubCall.addArgument(thisValue, regT0);
    stubCall.addArgument(arguments, regT0);
    stubCall.addArgument(Imm32(firstFreeRegister));
    stubCall.call(regT1);

    if (canOptimize)
        end.link(this);
}

void JIT::compileCallEval()
{
    JITStubCall stubCall(this, cti_op_call_eval); // Initializes ScopeChain; ReturnPC; CodeBlock.
    stubCall.call();
    addSlowCase(branch64(Equal, regT0, TrustedImm64(JSValue::encode(JSValue()))));
    emitGetFromCallFrameHeaderPtr(JSStack::CallerFrame, callFrameRegister);

    sampleCodeBlock(m_codeBlock);
}

void JIT::compileCallEvalSlowCase(Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);

    emitGetFromCallFrameHeader64(JSStack::Callee, regT0);
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
            emitGetVirtualRegister(registerOffset + CallFrame::argumentOffsetIncludingThis(0), regT0);
            Jump done = emitJumpIfNotJSCell(regT0);
            loadPtr(Address(regT0, JSCell::structureOffset()), regT0);
            storePtr(regT0, instruction[5].u.arrayProfile->addressOfLastSeenStructure());
            done.link(this);
        }
    
        addPtr(TrustedImm32(registerOffset * sizeof(Register)), callFrameRegister, regT1);
        store32(TrustedImm32(argCount), Address(regT1, JSStack::ArgumentCount * static_cast<int>(sizeof(Register)) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
    } // regT1 holds newCallFrame with ArgumentCount initialized.
    
    store32(TrustedImm32(instruction - m_codeBlock->instructions().begin()), Address(callFrameRegister, JSStack::ArgumentCount * static_cast<int>(sizeof(Register)) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
    emitGetVirtualRegister(callee, regT0); // regT0 holds callee.

    store64(callFrameRegister, Address(regT1, JSStack::CallerFrame * static_cast<int>(sizeof(Register))));
    store64(regT0, Address(regT1, JSStack::Callee * static_cast<int>(sizeof(Register))));
    move(regT1, callFrameRegister);

    if (opcodeID == op_call_eval) {
        compileCallEval();
        return;
    }

    DataLabelPtr addressOfLinkedFunctionCheck;
    BEGIN_UNINTERRUPTED_SEQUENCE(sequenceOpCall);
    Jump slowCase = branchPtrWithPatch(NotEqual, regT0, addressOfLinkedFunctionCheck, TrustedImmPtr(0));
    END_UNINTERRUPTED_SEQUENCE(sequenceOpCall);
    addSlowCase(slowCase);

    ASSERT(m_callStructureStubCompilationInfo.size() == callLinkInfoIndex);
    m_callStructureStubCompilationInfo.append(StructureStubCompilationInfo());
    m_callStructureStubCompilationInfo[callLinkInfoIndex].hotPathBegin = addressOfLinkedFunctionCheck;
    m_callStructureStubCompilationInfo[callLinkInfoIndex].callType = CallLinkInfo::callTypeFor(opcodeID);
    m_callStructureStubCompilationInfo[callLinkInfoIndex].bytecodeIndex = m_bytecodeOffset;

    loadPtr(Address(regT0, OBJECT_OFFSETOF(JSFunction, m_scope)), regT1);
    emitPutToCallFrameHeader(regT1, JSStack::ScopeChain);
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
    
    m_callStructureStubCompilationInfo[callLinkInfoIndex].callReturnLocation = emitNakedCall(opcodeID == op_construct ? m_vm->getCTIStub(linkConstructGenerator).code() : m_vm->getCTIStub(linkCallGenerator).code());

    sampleCodeBlock(m_codeBlock);
}

void JIT::privateCompileClosureCall(CallLinkInfo* callLinkInfo, CodeBlock* calleeCodeBlock, Structure* expectedStructure, ExecutableBase* expectedExecutable, MacroAssemblerCodePtr codePtr)
{
    JumpList slowCases;
    
    slowCases.append(branchTestPtr(NonZero, regT0, tagMaskRegister));
    slowCases.append(branchPtr(NotEqual, Address(regT0, JSCell::structureOffset()), TrustedImmPtr(expectedStructure)));
    slowCases.append(branchPtr(NotEqual, Address(regT0, JSFunction::offsetOfExecutable()), TrustedImmPtr(expectedExecutable)));
    
    loadPtr(Address(regT0, JSFunction::offsetOfScopeChain()), regT1);
    emitPutToCallFrameHeader(regT1, JSStack::ScopeChain);
    
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

#endif // USE(JSVALUE64)
#endif // ENABLE(JIT)
