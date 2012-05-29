/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
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
#include "JIT.h"

#include "Arguments.h"
#include "JITInlineMethods.h"
#include "JITStubCall.h"
#include "JSArray.h"
#include "JSCell.h"
#include "JSFunction.h"
#include "JSPropertyNameIterator.h"
#include "LinkBuffer.h"

namespace JSC {

#if USE(JSVALUE64)

void JIT::privateCompileCTIMachineTrampolines(RefPtr<ExecutablePool>* executablePool, JSGlobalData* globalData, TrampolineStructure *trampolines)
{
#if ENABLE(JIT_OPTIMIZE_PROPERTY_ACCESS)
    // (2) The second function provides fast property access for string length
    Label stringLengthBegin = align();

    // Check eax is a string
    Jump string_failureCases1 = emitJumpIfNotJSCell(regT0);
    Jump string_failureCases2 = branchPtr(NotEqual, Address(regT0), TrustedImmPtr(m_globalData->jsStringVPtr));

    // Checks out okay! - get the length from the Ustring.
    load32(Address(regT0, OBJECT_OFFSETOF(JSString, m_length)), regT0);

    Jump string_failureCases3 = branch32(LessThan, regT0, TrustedImm32(0));

    // regT0 contains a 64 bit value (is positive, is zero extended) so we don't need sign extend here.
    emitFastArithIntToImmNoCheck(regT0, regT0);
    
    ret();
#endif

    // (3) Trampolines for the slow cases of op_call / op_call_eval / op_construct.
    COMPILE_ASSERT(sizeof(CodeType) == 4, CodeTypeEnumMustBe32Bit);

    // VirtualCallLink Trampoline
    // regT0 holds callee, regT1 holds argCount.  regT2 will hold the FunctionExecutable.
    JumpList callLinkFailures;
    Label virtualCallLinkBegin = align();
    compileOpCallInitializeCallFrame();
    preserveReturnAddressAfterCall(regT3);
    emitPutToCallFrameHeader(regT3, RegisterFile::ReturnPC);
    restoreArgumentReference();
    Call callLazyLinkCall = call();
    callLinkFailures.append(branchTestPtr(Zero, regT0));
    restoreReturnAddressBeforeReturn(regT3);
    emitGetFromCallFrameHeader32(RegisterFile::ArgumentCount, regT1);
    jump(regT0);

    // VirtualConstructLink Trampoline
    // regT0 holds callee, regT1 holds argCount.  regT2 will hold the FunctionExecutable.
    Label virtualConstructLinkBegin = align();
    compileOpCallInitializeCallFrame();
    preserveReturnAddressAfterCall(regT3);
    emitPutToCallFrameHeader(regT3, RegisterFile::ReturnPC);
    restoreArgumentReference();
    Call callLazyLinkConstruct = call();
    callLinkFailures.append(branchTestPtr(Zero, regT0));
    restoreReturnAddressBeforeReturn(regT3);
    emitGetFromCallFrameHeader32(RegisterFile::ArgumentCount, regT1);
    jump(regT0);

    // VirtualCall Trampoline
    // regT0 holds callee, regT1 holds argCount.  regT2 will hold the FunctionExecutable.
    Label virtualCallBegin = align();
    compileOpCallInitializeCallFrame();

    loadPtr(Address(regT0, OBJECT_OFFSETOF(JSFunction, m_executable)), regT2);

    Jump hasCodeBlock3 = branch32(GreaterThanOrEqual, Address(regT2, OBJECT_OFFSETOF(FunctionExecutable, m_numParametersForCall)), TrustedImm32(0));
    preserveReturnAddressAfterCall(regT3);
    restoreArgumentReference();
    Call callCompileCall = call();
    callLinkFailures.append(branchTestPtr(Zero, regT0));
    emitGetFromCallFrameHeader32(RegisterFile::ArgumentCount, regT1);
    restoreReturnAddressBeforeReturn(regT3);
    loadPtr(Address(regT0, OBJECT_OFFSETOF(JSFunction, m_executable)), regT2);
    hasCodeBlock3.link(this);

    loadPtr(Address(regT2, OBJECT_OFFSETOF(FunctionExecutable, m_jitCodeForCallWithArityCheck)), regT0);
    jump(regT0);

    // VirtualConstruct Trampoline
    // regT0 holds callee, regT1 holds argCount.  regT2 will hold the FunctionExecutable.
    Label virtualConstructBegin = align();
    compileOpCallInitializeCallFrame();

    loadPtr(Address(regT0, OBJECT_OFFSETOF(JSFunction, m_executable)), regT2);

    Jump hasCodeBlock4 = branch32(GreaterThanOrEqual, Address(regT2, OBJECT_OFFSETOF(FunctionExecutable, m_numParametersForConstruct)), TrustedImm32(0));
    preserveReturnAddressAfterCall(regT3);
    restoreArgumentReference();
    Call callCompileConstruct = call();
    callLinkFailures.append(branchTestPtr(Zero, regT0));
    emitGetFromCallFrameHeader32(RegisterFile::ArgumentCount, regT1);
    restoreReturnAddressBeforeReturn(regT3);
    loadPtr(Address(regT0, OBJECT_OFFSETOF(JSFunction, m_executable)), regT2);
    hasCodeBlock4.link(this);

    loadPtr(Address(regT2, OBJECT_OFFSETOF(FunctionExecutable, m_jitCodeForConstructWithArityCheck)), regT0);
    jump(regT0);
    
    // If the parser fails we want to be able to be able to keep going,
    // So we handle this as a parse failure.
    callLinkFailures.link(this);
    emitGetFromCallFrameHeaderPtr(RegisterFile::ReturnPC, regT1);
    emitGetFromCallFrameHeaderPtr(RegisterFile::CallerFrame, callFrameRegister);
    restoreReturnAddressBeforeReturn(regT1);
    move(TrustedImmPtr(&globalData->exceptionLocation), regT2);
    storePtr(regT1, regT2);
    poke(callFrameRegister, 1 + OBJECT_OFFSETOF(struct JITStackFrame, callFrame) / sizeof(void*));
    poke(TrustedImmPtr(FunctionPtr(ctiVMThrowTrampoline).value()));
    ret();

    // NativeCall Trampoline
    Label nativeCallThunk = privateCompileCTINativeCall(globalData);    
    Label nativeConstructThunk = privateCompileCTINativeCall(globalData, true);    

#if ENABLE(JIT_OPTIMIZE_PROPERTY_ACCESS)
    Call string_failureCases1Call = makeTailRecursiveCall(string_failureCases1);
    Call string_failureCases2Call = makeTailRecursiveCall(string_failureCases2);
    Call string_failureCases3Call = makeTailRecursiveCall(string_failureCases3);
#endif

    // All trampolines constructed! copy the code, link up calls, and set the pointers on the Machine object.
    LinkBuffer patchBuffer(this, m_globalData->executableAllocator);

#if ENABLE(JIT_OPTIMIZE_PROPERTY_ACCESS)
    patchBuffer.link(string_failureCases1Call, FunctionPtr(cti_op_get_by_id_string_fail));
    patchBuffer.link(string_failureCases2Call, FunctionPtr(cti_op_get_by_id_string_fail));
    patchBuffer.link(string_failureCases3Call, FunctionPtr(cti_op_get_by_id_string_fail));
#endif
#if ENABLE(JIT_OPTIMIZE_CALL)
    patchBuffer.link(callLazyLinkCall, FunctionPtr(cti_vm_lazyLinkCall));
    patchBuffer.link(callLazyLinkConstruct, FunctionPtr(cti_vm_lazyLinkConstruct));
#endif
    patchBuffer.link(callCompileCall, FunctionPtr(cti_op_call_jitCompile));
    patchBuffer.link(callCompileConstruct, FunctionPtr(cti_op_construct_jitCompile));

    CodeRef finalCode = patchBuffer.finalizeCode();
    *executablePool = finalCode.m_executablePool;

    trampolines->ctiVirtualCallLink = patchBuffer.trampolineAt(virtualCallLinkBegin);
    trampolines->ctiVirtualConstructLink = patchBuffer.trampolineAt(virtualConstructLinkBegin);
    trampolines->ctiVirtualCall = patchBuffer.trampolineAt(virtualCallBegin);
    trampolines->ctiVirtualConstruct = patchBuffer.trampolineAt(virtualConstructBegin);
    trampolines->ctiNativeCall = patchBuffer.trampolineAt(nativeCallThunk);
    trampolines->ctiNativeConstruct = patchBuffer.trampolineAt(nativeConstructThunk);
#if ENABLE(JIT_OPTIMIZE_PROPERTY_ACCESS)
    trampolines->ctiStringLengthTrampoline = patchBuffer.trampolineAt(stringLengthBegin);
#endif
}

JIT::Label JIT::privateCompileCTINativeCall(JSGlobalData* globalData, bool isConstruct)
{
    int executableOffsetToFunction = isConstruct ? OBJECT_OFFSETOF(NativeExecutable, m_constructor) : OBJECT_OFFSETOF(NativeExecutable, m_function);

    Label nativeCallThunk = align();
    
    emitPutImmediateToCallFrameHeader(0, RegisterFile::CodeBlock);

#if CPU(X86_64)
    // Load caller frame's scope chain into this callframe so that whatever we call can
    // get to its global data.
    emitGetFromCallFrameHeaderPtr(RegisterFile::CallerFrame, regT0);
    emitGetFromCallFrameHeaderPtr(RegisterFile::ScopeChain, regT1, regT0);
    emitPutCellToCallFrameHeader(regT1, RegisterFile::ScopeChain);

    peek(regT1);
    emitPutToCallFrameHeader(regT1, RegisterFile::ReturnPC);

    // Calling convention:      f(edi, esi, edx, ecx, ...);
    // Host function signature: f(ExecState*);
    move(callFrameRegister, X86Registers::edi);

    subPtr(TrustedImm32(16 - sizeof(void*)), stackPointerRegister); // Align stack after call.

    emitGetFromCallFrameHeaderPtr(RegisterFile::Callee, X86Registers::esi);
    loadPtr(Address(X86Registers::esi, OBJECT_OFFSETOF(JSFunction, m_executable)), X86Registers::r9);
    move(regT0, callFrameRegister); // Eagerly restore caller frame register to avoid loading from stack.
    call(Address(X86Registers::r9, executableOffsetToFunction));

    addPtr(TrustedImm32(16 - sizeof(void*)), stackPointerRegister);

#elif CPU(ARM)
    // Load caller frame's scope chain into this callframe so that whatever we call can
    // get to its global data.
    emitGetFromCallFrameHeaderPtr(RegisterFile::CallerFrame, regT2);
    emitGetFromCallFrameHeaderPtr(RegisterFile::ScopeChain, regT1, regT2);
    emitPutCellToCallFrameHeader(regT1, RegisterFile::ScopeChain);

    preserveReturnAddressAfterCall(regT3); // Callee preserved
    emitPutToCallFrameHeader(regT3, RegisterFile::ReturnPC);

    // Calling convention:      f(r0 == regT0, r1 == regT1, ...);
    // Host function signature: f(ExecState*);
    move(callFrameRegister, ARMRegisters::r0);

    emitGetFromCallFrameHeaderPtr(RegisterFile::Callee, ARMRegisters::r1);
    move(regT2, callFrameRegister); // Eagerly restore caller frame register to avoid loading from stack.
    loadPtr(Address(ARMRegisters::r1, OBJECT_OFFSETOF(JSFunction, m_executable)), regT2);
    call(Address(regT2, executableOffsetToFunction));

    restoreReturnAddressBeforeReturn(regT3);

#elif CPU(MIPS)
    // Load caller frame's scope chain into this callframe so that whatever we call can
    // get to its global data.
    emitGetFromCallFrameHeaderPtr(RegisterFile::CallerFrame, regT0);
    emitGetFromCallFrameHeaderPtr(RegisterFile::ScopeChain, regT1, regT0);
    emitPutCellToCallFrameHeader(regT1, RegisterFile::ScopeChain);

    preserveReturnAddressAfterCall(regT3); // Callee preserved
    emitPutToCallFrameHeader(regT3, RegisterFile::ReturnPC);

    // Calling convention:      f(a0, a1, a2, a3);
    // Host function signature: f(ExecState*);

    // Allocate stack space for 16 bytes (8-byte aligned)
    // 16 bytes (unused) for 4 arguments
    subPtr(TrustedImm32(16), stackPointerRegister);

    // Setup arg0
    move(callFrameRegister, MIPSRegisters::a0);

    // Call
    emitGetFromCallFrameHeaderPtr(RegisterFile::Callee, MIPSRegisters::a2);
    loadPtr(Address(MIPSRegisters::a2, OBJECT_OFFSETOF(JSFunction, m_executable)), regT2);
    move(regT0, callFrameRegister); // Eagerly restore caller frame register to avoid loading from stack.
    call(Address(regT2, executableOffsetToFunction));

    // Restore stack space
    addPtr(TrustedImm32(16), stackPointerRegister);

    restoreReturnAddressBeforeReturn(regT3);

#elif ENABLE(JIT_OPTIMIZE_NATIVE_CALL)
#error "JIT_OPTIMIZE_NATIVE_CALL not yet supported on this platform."
#else
    UNUSED_PARAM(executableOffsetToFunction);
    breakpoint();
#endif

    // Check for an exception
    loadPtr(&(globalData->exception), regT2);
    Jump exceptionHandler = branchTestPtr(NonZero, regT2);

    // Return.
    ret();

    // Handle an exception
    exceptionHandler.link(this);

    // Grab the return address.
    preserveReturnAddressAfterCall(regT1);

    move(TrustedImmPtr(&globalData->exceptionLocation), regT2);
    storePtr(regT1, regT2);
    poke(callFrameRegister, OBJECT_OFFSETOF(struct JITStackFrame, callFrame) / sizeof(void*));

    // Set the return address.
    move(TrustedImmPtr(FunctionPtr(ctiVMThrowTrampoline).value()), regT1);
    restoreReturnAddressBeforeReturn(regT1);

    ret();

    return nativeCallThunk;
}

JIT::CodePtr JIT::privateCompileCTINativeCall(PassRefPtr<ExecutablePool>, JSGlobalData* globalData, NativeFunction)
{
    return globalData->jitStubs->ctiNativeCall();
}

void JIT::emit_op_mov(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    int src = currentInstruction[2].u.operand;

    if (m_codeBlock->isConstantRegisterIndex(src)) {
        storePtr(ImmPtr(JSValue::encode(getConstantOperand(src))), Address(callFrameRegister, dst * sizeof(Register)));
        if (dst == m_lastResultBytecodeRegister)
            killLastResultRegister();
    } else if ((src == m_lastResultBytecodeRegister) || (dst == m_lastResultBytecodeRegister)) {
        // If either the src or dst is the cached register go though
        // get/put registers to make sure we track this correctly.
        emitGetVirtualRegister(src, regT0);
        emitPutVirtualRegister(dst);
    } else {
        // Perform the copy via regT1; do not disturb any mapping in regT0.
        loadPtr(Address(callFrameRegister, src * sizeof(Register)), regT1);
        storePtr(regT1, Address(callFrameRegister, dst * sizeof(Register)));
    }
}

void JIT::emit_op_end(Instruction* currentInstruction)
{
    ASSERT(returnValueRegister != callFrameRegister);
    emitGetVirtualRegister(currentInstruction[1].u.operand, returnValueRegister);
    restoreReturnAddressBeforeReturn(Address(callFrameRegister, RegisterFile::ReturnPC * static_cast<int>(sizeof(Register))));
    ret();
}

void JIT::emit_op_jmp(Instruction* currentInstruction)
{
    unsigned target = currentInstruction[1].u.operand;
    addJump(jump(), target);
}

void JIT::emit_op_loop_if_lesseq(Instruction* currentInstruction)
{
    emitTimeoutCheck();

    unsigned op1 = currentInstruction[1].u.operand;
    unsigned op2 = currentInstruction[2].u.operand;
    unsigned target = currentInstruction[3].u.operand;
    if (isOperandConstantImmediateInt(op2)) {
        emitGetVirtualRegister(op1, regT0);
        emitJumpSlowCaseIfNotImmediateInteger(regT0);
        int32_t op2imm = getConstantOperandImmediateInt(op2);
        addJump(branch32(LessThanOrEqual, regT0, Imm32(op2imm)), target);
    } else {
        emitGetVirtualRegisters(op1, regT0, op2, regT1);
        emitJumpSlowCaseIfNotImmediateInteger(regT0);
        emitJumpSlowCaseIfNotImmediateInteger(regT1);
        addJump(branch32(LessThanOrEqual, regT0, regT1), target);
    }
}

void JIT::emit_op_new_object(Instruction* currentInstruction)
{
    JITStubCall(this, cti_op_new_object).call(currentInstruction[1].u.operand);
}

void JIT::emit_op_check_has_instance(Instruction* currentInstruction)
{
    unsigned baseVal = currentInstruction[1].u.operand;

    emitGetVirtualRegister(baseVal, regT0);

    // Check that baseVal is a cell.
    emitJumpSlowCaseIfNotJSCell(regT0, baseVal);

    // Check that baseVal 'ImplementsHasInstance'.
    loadPtr(Address(regT0, JSCell::structureOffset()), regT0);
    addSlowCase(branchTest8(Zero, Address(regT0, Structure::typeInfoFlagsOffset()), TrustedImm32(ImplementsHasInstance)));
}

void JIT::emit_op_instanceof(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned value = currentInstruction[2].u.operand;
    unsigned baseVal = currentInstruction[3].u.operand;
    unsigned proto = currentInstruction[4].u.operand;

    // Load the operands (baseVal, proto, and value respectively) into registers.
    // We use regT0 for baseVal since we will be done with this first, and we can then use it for the result.
    emitGetVirtualRegister(value, regT2);
    emitGetVirtualRegister(baseVal, regT0);
    emitGetVirtualRegister(proto, regT1);

    // Check that proto are cells.  baseVal must be a cell - this is checked by op_check_has_instance.
    emitJumpSlowCaseIfNotJSCell(regT2, value);
    emitJumpSlowCaseIfNotJSCell(regT1, proto);

    // Check that prototype is an object
    loadPtr(Address(regT1, JSCell::structureOffset()), regT3);
    addSlowCase(branch8(NotEqual, Address(regT3, Structure::typeInfoTypeOffset()), TrustedImm32(ObjectType)));
    
    // Fixme: this check is only needed because the JSC API allows HasInstance to be overridden; we should deprecate this.
    // Check that baseVal 'ImplementsDefaultHasInstance'.
    loadPtr(Address(regT0, JSCell::structureOffset()), regT0);
    addSlowCase(branchTest8(Zero, Address(regT0, Structure::typeInfoFlagsOffset()), TrustedImm32(ImplementsDefaultHasInstance)));

    // Optimistically load the result true, and start looping.
    // Initially, regT1 still contains proto and regT2 still contains value.
    // As we loop regT2 will be updated with its prototype, recursively walking the prototype chain.
    move(TrustedImmPtr(JSValue::encode(jsBoolean(true))), regT0);
    Label loop(this);

    // Load the prototype of the object in regT2.  If this is equal to regT1 - WIN!
    // Otherwise, check if we've hit null - if we have then drop out of the loop, if not go again.
    loadPtr(Address(regT2, JSCell::structureOffset()), regT2);
    loadPtr(Address(regT2, Structure::prototypeOffset()), regT2);
    Jump isInstance = branchPtr(Equal, regT2, regT1);
    emitJumpIfJSCell(regT2).linkTo(loop, this);

    // We get here either by dropping out of the loop, or if value was not an Object.  Result is false.
    move(TrustedImmPtr(JSValue::encode(jsBoolean(false))), regT0);

    // isInstance jumps right down to here, to skip setting the result to false (it has already set true).
    isInstance.link(this);
    emitPutVirtualRegister(dst);
}

void JIT::emit_op_call(Instruction* currentInstruction)
{
    compileOpCall(op_call, currentInstruction, m_callLinkInfoIndex++);
}

void JIT::emit_op_call_eval(Instruction* currentInstruction)
{
    compileOpCall(op_call_eval, currentInstruction, m_callLinkInfoIndex++);
}

void JIT::emit_op_call_varargs(Instruction* currentInstruction)
{
    compileOpCallVarargs(currentInstruction);
}

void JIT::emit_op_construct(Instruction* currentInstruction)
{
    compileOpCall(op_construct, currentInstruction, m_callLinkInfoIndex++);
}

void JIT::emit_op_get_global_var(Instruction* currentInstruction)
{
    JSVariableObject* globalObject = m_codeBlock->globalObject();
    loadPtr(&globalObject->m_registers, regT0);
    loadPtr(Address(regT0, currentInstruction[2].u.operand * sizeof(Register)), regT0);
    emitPutVirtualRegister(currentInstruction[1].u.operand);
}

void JIT::emit_op_put_global_var(Instruction* currentInstruction)
{
    emitGetVirtualRegister(currentInstruction[2].u.operand, regT1);
    JSVariableObject* globalObject = m_codeBlock->globalObject();
    loadPtr(&globalObject->m_registers, regT0);
    storePtr(regT1, Address(regT0, currentInstruction[1].u.operand * sizeof(Register)));
}

void JIT::emit_op_get_scoped_var(Instruction* currentInstruction)
{
    int skip = currentInstruction[3].u.operand;

    emitGetFromCallFrameHeaderPtr(RegisterFile::ScopeChain, regT0);
    bool checkTopLevel = m_codeBlock->codeType() == FunctionCode && m_codeBlock->needsFullScopeChain();
    ASSERT(skip || !checkTopLevel);
    if (checkTopLevel && skip--) {
        Jump activationNotCreated;
        if (checkTopLevel)
            activationNotCreated = branchTestPtr(Zero, addressFor(m_codeBlock->activationRegister()));
        loadPtr(Address(regT0, OBJECT_OFFSETOF(ScopeChainNode, next)), regT0);
        activationNotCreated.link(this);
    }
    while (skip--)
        loadPtr(Address(regT0, OBJECT_OFFSETOF(ScopeChainNode, next)), regT0);

    loadPtr(Address(regT0, OBJECT_OFFSETOF(ScopeChainNode, object)), regT0);
    loadPtr(Address(regT0, OBJECT_OFFSETOF(JSVariableObject, m_registers)), regT0);
    loadPtr(Address(regT0, currentInstruction[2].u.operand * sizeof(Register)), regT0);
    emitPutVirtualRegister(currentInstruction[1].u.operand);
}

void JIT::emit_op_put_scoped_var(Instruction* currentInstruction)
{
    int skip = currentInstruction[2].u.operand;

    emitGetFromCallFrameHeaderPtr(RegisterFile::ScopeChain, regT1);
    emitGetVirtualRegister(currentInstruction[3].u.operand, regT0);
    bool checkTopLevel = m_codeBlock->codeType() == FunctionCode && m_codeBlock->needsFullScopeChain();
    ASSERT(skip || !checkTopLevel);
    if (checkTopLevel && skip--) {
        Jump activationNotCreated;
        if (checkTopLevel)
            activationNotCreated = branchTestPtr(Zero, addressFor(m_codeBlock->activationRegister()));
        loadPtr(Address(regT1, OBJECT_OFFSETOF(ScopeChainNode, next)), regT1);
        activationNotCreated.link(this);
    }
    while (skip--)
        loadPtr(Address(regT1, OBJECT_OFFSETOF(ScopeChainNode, next)), regT1);

    loadPtr(Address(regT1, OBJECT_OFFSETOF(ScopeChainNode, object)), regT1);
    loadPtr(Address(regT1, OBJECT_OFFSETOF(JSVariableObject, m_registers)), regT1);
    storePtr(regT0, Address(regT1, currentInstruction[1].u.operand * sizeof(Register)));
}

void JIT::emit_op_tear_off_activation(Instruction* currentInstruction)
{
    unsigned activation = currentInstruction[1].u.operand;
    unsigned arguments = currentInstruction[2].u.operand;
    Jump activationCreated = branchTestPtr(NonZero, addressFor(activation));
    Jump argumentsNotCreated = branchTestPtr(Zero, addressFor(arguments));
    activationCreated.link(this);
    JITStubCall stubCall(this, cti_op_tear_off_activation);
    stubCall.addArgument(activation, regT2);
    stubCall.addArgument(unmodifiedArgumentsRegister(arguments), regT2);
    stubCall.call();
    argumentsNotCreated.link(this);
}

void JIT::emit_op_tear_off_arguments(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;

    Jump argsNotCreated = branchTestPtr(Zero, Address(callFrameRegister, sizeof(Register) * (unmodifiedArgumentsRegister(dst))));
    JITStubCall stubCall(this, cti_op_tear_off_arguments);
    stubCall.addArgument(unmodifiedArgumentsRegister(dst), regT2);
    stubCall.call();
    argsNotCreated.link(this);
}

void JIT::emit_op_ret(Instruction* currentInstruction)
{
    ASSERT(callFrameRegister != regT1);
    ASSERT(regT1 != returnValueRegister);
    ASSERT(returnValueRegister != callFrameRegister);

    // Return the result in %eax.
    emitGetVirtualRegister(currentInstruction[1].u.operand, returnValueRegister);

    // Grab the return address.
    emitGetFromCallFrameHeaderPtr(RegisterFile::ReturnPC, regT1);

    // Restore our caller's "r".
    emitGetFromCallFrameHeaderPtr(RegisterFile::CallerFrame, callFrameRegister);

    // Return.
    restoreReturnAddressBeforeReturn(regT1);
    ret();
}

void JIT::emit_op_ret_object_or_this(Instruction* currentInstruction)
{
    ASSERT(callFrameRegister != regT1);
    ASSERT(regT1 != returnValueRegister);
    ASSERT(returnValueRegister != callFrameRegister);

    // Return the result in %eax.
    emitGetVirtualRegister(currentInstruction[1].u.operand, returnValueRegister);
    Jump notJSCell = emitJumpIfNotJSCell(returnValueRegister);
    loadPtr(Address(returnValueRegister, JSCell::structureOffset()), regT2);
    Jump notObject = branch8(NotEqual, Address(regT2, Structure::typeInfoTypeOffset()), TrustedImm32(ObjectType));

    // Grab the return address.
    emitGetFromCallFrameHeaderPtr(RegisterFile::ReturnPC, regT1);

    // Restore our caller's "r".
    emitGetFromCallFrameHeaderPtr(RegisterFile::CallerFrame, callFrameRegister);

    // Return.
    restoreReturnAddressBeforeReturn(regT1);
    ret();

    // Return 'this' in %eax.
    notJSCell.link(this);
    notObject.link(this);
    emitGetVirtualRegister(currentInstruction[2].u.operand, returnValueRegister);

    // Grab the return address.
    emitGetFromCallFrameHeaderPtr(RegisterFile::ReturnPC, regT1);

    // Restore our caller's "r".
    emitGetFromCallFrameHeaderPtr(RegisterFile::CallerFrame, callFrameRegister);

    // Return.
    restoreReturnAddressBeforeReturn(regT1);
    ret();
}

void JIT::emit_op_new_array(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_new_array);
    stubCall.addArgument(Imm32(currentInstruction[2].u.operand));
    stubCall.addArgument(Imm32(currentInstruction[3].u.operand));
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emit_op_resolve(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_resolve);
    stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(currentInstruction[2].u.operand)));
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emit_op_to_primitive(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    int src = currentInstruction[2].u.operand;

    emitGetVirtualRegister(src, regT0);
    
    Jump isImm = emitJumpIfNotJSCell(regT0);
    addSlowCase(branchPtr(NotEqual, Address(regT0), TrustedImmPtr(m_globalData->jsStringVPtr)));
    isImm.link(this);

    if (dst != src)
        emitPutVirtualRegister(dst);

}

void JIT::emit_op_strcat(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_strcat);
    stubCall.addArgument(Imm32(currentInstruction[2].u.operand));
    stubCall.addArgument(Imm32(currentInstruction[3].u.operand));
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emit_op_resolve_base(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, currentInstruction[3].u.operand ? cti_op_resolve_base_strict_put : cti_op_resolve_base);
    stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(currentInstruction[2].u.operand)));
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emit_op_ensure_property_exists(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_ensure_property_exists);
    stubCall.addArgument(Imm32(currentInstruction[1].u.operand));
    stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(currentInstruction[2].u.operand)));
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emit_op_resolve_skip(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_resolve_skip);
    stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(currentInstruction[2].u.operand)));
    stubCall.addArgument(Imm32(currentInstruction[3].u.operand));
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emit_op_resolve_global(Instruction* currentInstruction, bool)
{
    // Fast case
    void* globalObject = m_codeBlock->globalObject();
    unsigned currentIndex = m_globalResolveInfoIndex++;
    void* structureAddress = &(m_codeBlock->globalResolveInfo(currentIndex).structure);
    void* offsetAddr = &(m_codeBlock->globalResolveInfo(currentIndex).offset);

    // Check Structure of global object
    move(TrustedImmPtr(globalObject), regT0);
    loadPtr(structureAddress, regT1);
    addSlowCase(branchPtr(NotEqual, regT1, Address(regT0, JSCell::structureOffset()))); // Structures don't match

    // Load cached property
    // Assume that the global object always uses external storage.
    loadPtr(Address(regT0, OBJECT_OFFSETOF(JSGlobalObject, m_propertyStorage)), regT0);
    load32(offsetAddr, regT1);
    loadPtr(BaseIndex(regT0, regT1, ScalePtr), regT0);
    emitPutVirtualRegister(currentInstruction[1].u.operand);
}

void JIT::emitSlow_op_resolve_global(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;
    Identifier* ident = &m_codeBlock->identifier(currentInstruction[2].u.operand);
    
    unsigned currentIndex = m_globalResolveInfoIndex++;
    
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_resolve_global);
    stubCall.addArgument(TrustedImmPtr(ident));
    stubCall.addArgument(Imm32(currentIndex));
    stubCall.addArgument(regT0);
    stubCall.call(dst);
}

void JIT::emit_op_not(Instruction* currentInstruction)
{
    emitGetVirtualRegister(currentInstruction[2].u.operand, regT0);

    // Invert against JSValue(false); if the value was tagged as a boolean, then all bits will be
    // clear other than the low bit (which will be 0 or 1 for false or true inputs respectively).
    // Then invert against JSValue(true), which will add the tag back in, and flip the low bit.
    xorPtr(TrustedImm32(static_cast<int32_t>(ValueFalse)), regT0);
    addSlowCase(branchTestPtr(NonZero, regT0, TrustedImm32(static_cast<int32_t>(~1))));
    xorPtr(TrustedImm32(static_cast<int32_t>(ValueTrue)), regT0);

    emitPutVirtualRegister(currentInstruction[1].u.operand);
}

void JIT::emit_op_jfalse(Instruction* currentInstruction)
{
    unsigned target = currentInstruction[2].u.operand;
    emitGetVirtualRegister(currentInstruction[1].u.operand, regT0);

    addJump(branchPtr(Equal, regT0, TrustedImmPtr(JSValue::encode(jsNumber(0)))), target);
    Jump isNonZero = emitJumpIfImmediateInteger(regT0);

    addJump(branchPtr(Equal, regT0, TrustedImmPtr(JSValue::encode(jsBoolean(false)))), target);
    addSlowCase(branchPtr(NotEqual, regT0, TrustedImmPtr(JSValue::encode(jsBoolean(true)))));

    isNonZero.link(this);
}

void JIT::emit_op_jeq_null(Instruction* currentInstruction)
{
    unsigned src = currentInstruction[1].u.operand;
    unsigned target = currentInstruction[2].u.operand;

    emitGetVirtualRegister(src, regT0);
    Jump isImmediate = emitJumpIfNotJSCell(regT0);

    // First, handle JSCell cases - check MasqueradesAsUndefined bit on the structure.
    loadPtr(Address(regT0, JSCell::structureOffset()), regT2);
    addJump(branchTest8(NonZero, Address(regT2, Structure::typeInfoFlagsOffset()), TrustedImm32(MasqueradesAsUndefined)), target);
    Jump wasNotImmediate = jump();

    // Now handle the immediate cases - undefined & null
    isImmediate.link(this);
    andPtr(TrustedImm32(~TagBitUndefined), regT0);
    addJump(branchPtr(Equal, regT0, TrustedImmPtr(JSValue::encode(jsNull()))), target);            

    wasNotImmediate.link(this);
};
void JIT::emit_op_jneq_null(Instruction* currentInstruction)
{
    unsigned src = currentInstruction[1].u.operand;
    unsigned target = currentInstruction[2].u.operand;

    emitGetVirtualRegister(src, regT0);
    Jump isImmediate = emitJumpIfNotJSCell(regT0);

    // First, handle JSCell cases - check MasqueradesAsUndefined bit on the structure.
    loadPtr(Address(regT0, JSCell::structureOffset()), regT2);
    addJump(branchTest8(Zero, Address(regT2, Structure::typeInfoFlagsOffset()), TrustedImm32(MasqueradesAsUndefined)), target);
    Jump wasNotImmediate = jump();

    // Now handle the immediate cases - undefined & null
    isImmediate.link(this);
    andPtr(TrustedImm32(~TagBitUndefined), regT0);
    addJump(branchPtr(NotEqual, regT0, TrustedImmPtr(JSValue::encode(jsNull()))), target);            

    wasNotImmediate.link(this);
}

void JIT::emit_op_jneq_ptr(Instruction* currentInstruction)
{
    unsigned src = currentInstruction[1].u.operand;
    JSCell* ptr = currentInstruction[2].u.jsCell.get();
    unsigned target = currentInstruction[3].u.operand;
    
    emitGetVirtualRegister(src, regT0);
    addJump(branchPtr(NotEqual, regT0, TrustedImmPtr(JSValue::encode(JSValue(ptr)))), target);            
}

void JIT::emit_op_jsr(Instruction* currentInstruction)
{
    int retAddrDst = currentInstruction[1].u.operand;
    int target = currentInstruction[2].u.operand;
    DataLabelPtr storeLocation = storePtrWithPatch(TrustedImmPtr(0), Address(callFrameRegister, sizeof(Register) * retAddrDst));
    addJump(jump(), target);
    m_jsrSites.append(JSRInfo(storeLocation, label()));
    killLastResultRegister();
}

void JIT::emit_op_sret(Instruction* currentInstruction)
{
    jump(Address(callFrameRegister, sizeof(Register) * currentInstruction[1].u.operand));
    killLastResultRegister();
}

void JIT::emit_op_eq(Instruction* currentInstruction)
{
    emitGetVirtualRegisters(currentInstruction[2].u.operand, regT0, currentInstruction[3].u.operand, regT1);
    emitJumpSlowCaseIfNotImmediateIntegers(regT0, regT1, regT2);
    compare32(Equal, regT1, regT0, regT0);
    emitTagAsBoolImmediate(regT0);
    emitPutVirtualRegister(currentInstruction[1].u.operand);
}

void JIT::emit_op_bitnot(Instruction* currentInstruction)
{
    emitGetVirtualRegister(currentInstruction[2].u.operand, regT0);
    emitJumpSlowCaseIfNotImmediateInteger(regT0);
    not32(regT0);
    emitFastArithIntToImmNoCheck(regT0, regT0);
    emitPutVirtualRegister(currentInstruction[1].u.operand);
}

void JIT::emit_op_resolve_with_base(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_resolve_with_base);
    stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(currentInstruction[3].u.operand)));
    stubCall.addArgument(Imm32(currentInstruction[1].u.operand));
    stubCall.call(currentInstruction[2].u.operand);
}

void JIT::emit_op_new_func_exp(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_new_func_exp);
    stubCall.addArgument(TrustedImmPtr(m_codeBlock->functionExpr(currentInstruction[2].u.operand)));
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emit_op_jtrue(Instruction* currentInstruction)
{
    unsigned target = currentInstruction[2].u.operand;
    emitGetVirtualRegister(currentInstruction[1].u.operand, regT0);

    Jump isZero = branchPtr(Equal, regT0, TrustedImmPtr(JSValue::encode(jsNumber(0))));
    addJump(emitJumpIfImmediateInteger(regT0), target);

    addJump(branchPtr(Equal, regT0, TrustedImmPtr(JSValue::encode(jsBoolean(true)))), target);
    addSlowCase(branchPtr(NotEqual, regT0, TrustedImmPtr(JSValue::encode(jsBoolean(false)))));

    isZero.link(this);
}

void JIT::emit_op_neq(Instruction* currentInstruction)
{
    emitGetVirtualRegisters(currentInstruction[2].u.operand, regT0, currentInstruction[3].u.operand, regT1);
    emitJumpSlowCaseIfNotImmediateIntegers(regT0, regT1, regT2);
    compare32(NotEqual, regT1, regT0, regT0);
    emitTagAsBoolImmediate(regT0);

    emitPutVirtualRegister(currentInstruction[1].u.operand);

}

void JIT::emit_op_bitxor(Instruction* currentInstruction)
{
    emitGetVirtualRegisters(currentInstruction[2].u.operand, regT0, currentInstruction[3].u.operand, regT1);
    emitJumpSlowCaseIfNotImmediateIntegers(regT0, regT1, regT2);
    xorPtr(regT1, regT0);
    emitFastArithReTagImmediate(regT0, regT0);
    emitPutVirtualRegister(currentInstruction[1].u.operand);
}

void JIT::emit_op_bitor(Instruction* currentInstruction)
{
    emitGetVirtualRegisters(currentInstruction[2].u.operand, regT0, currentInstruction[3].u.operand, regT1);
    emitJumpSlowCaseIfNotImmediateIntegers(regT0, regT1, regT2);
    orPtr(regT1, regT0);
    emitPutVirtualRegister(currentInstruction[1].u.operand);
}

void JIT::emit_op_throw(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_throw);
    stubCall.addArgument(currentInstruction[1].u.operand, regT2);
    stubCall.call();
    ASSERT(regT0 == returnValueRegister);
#ifndef NDEBUG
    // cti_op_throw always changes it's return address,
    // this point in the code should never be reached.
    breakpoint();
#endif
}

void JIT::emit_op_get_pnames(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    int base = currentInstruction[2].u.operand;
    int i = currentInstruction[3].u.operand;
    int size = currentInstruction[4].u.operand;
    int breakTarget = currentInstruction[5].u.operand;

    JumpList isNotObject;

    emitGetVirtualRegister(base, regT0);
    if (!m_codeBlock->isKnownNotImmediate(base))
        isNotObject.append(emitJumpIfNotJSCell(regT0));
    if (base != m_codeBlock->thisRegister() || m_codeBlock->isStrictMode()) {
        loadPtr(Address(regT0, JSCell::structureOffset()), regT2);
        isNotObject.append(branch8(NotEqual, Address(regT2, Structure::typeInfoTypeOffset()), TrustedImm32(ObjectType)));
    }

    // We could inline the case where you have a valid cache, but
    // this call doesn't seem to be hot.
    Label isObject(this);
    JITStubCall getPnamesStubCall(this, cti_op_get_pnames);
    getPnamesStubCall.addArgument(regT0);
    getPnamesStubCall.call(dst);
    load32(Address(regT0, OBJECT_OFFSETOF(JSPropertyNameIterator, m_jsStringsSize)), regT3);
    storePtr(tagTypeNumberRegister, payloadFor(i));
    store32(TrustedImm32(Int32Tag), intTagFor(size));
    store32(regT3, intPayloadFor(size));
    Jump end = jump();

    isNotObject.link(this);
    move(regT0, regT1);
    and32(TrustedImm32(~TagBitUndefined), regT1);
    addJump(branch32(Equal, regT1, TrustedImm32(ValueNull)), breakTarget);

    JITStubCall toObjectStubCall(this, cti_to_object);
    toObjectStubCall.addArgument(regT0);
    toObjectStubCall.call(base);
    jump().linkTo(isObject, this);
    
    end.link(this);
}

void JIT::emit_op_next_pname(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    int base = currentInstruction[2].u.operand;
    int i = currentInstruction[3].u.operand;
    int size = currentInstruction[4].u.operand;
    int it = currentInstruction[5].u.operand;
    int target = currentInstruction[6].u.operand;
    
    JumpList callHasProperty;

    Label begin(this);
    load32(intPayloadFor(i), regT0);
    Jump end = branch32(Equal, regT0, intPayloadFor(size));

    // Grab key @ i
    loadPtr(addressFor(it), regT1);
    loadPtr(Address(regT1, OBJECT_OFFSETOF(JSPropertyNameIterator, m_jsStrings)), regT2);

    loadPtr(BaseIndex(regT2, regT0, TimesEight), regT2);

    emitPutVirtualRegister(dst, regT2);

    // Increment i
    add32(TrustedImm32(1), regT0);
    store32(regT0, intPayloadFor(i));

    // Verify that i is valid:
    emitGetVirtualRegister(base, regT0);

    // Test base's structure
    loadPtr(Address(regT0, JSCell::structureOffset()), regT2);
    callHasProperty.append(branchPtr(NotEqual, regT2, Address(Address(regT1, OBJECT_OFFSETOF(JSPropertyNameIterator, m_cachedStructure)))));

    // Test base's prototype chain
    loadPtr(Address(Address(regT1, OBJECT_OFFSETOF(JSPropertyNameIterator, m_cachedPrototypeChain))), regT3);
    loadPtr(Address(regT3, OBJECT_OFFSETOF(StructureChain, m_vector)), regT3);
    addJump(branchTestPtr(Zero, Address(regT3)), target);

    Label checkPrototype(this);
    loadPtr(Address(regT2, Structure::prototypeOffset()), regT2);
    callHasProperty.append(emitJumpIfNotJSCell(regT2));
    loadPtr(Address(regT2, JSCell::structureOffset()), regT2);
    callHasProperty.append(branchPtr(NotEqual, regT2, Address(regT3)));
    addPtr(TrustedImm32(sizeof(Structure*)), regT3);
    branchTestPtr(NonZero, Address(regT3)).linkTo(checkPrototype, this);

    // Continue loop.
    addJump(jump(), target);

    // Slow case: Ask the object if i is valid.
    callHasProperty.link(this);
    emitGetVirtualRegister(dst, regT1);
    JITStubCall stubCall(this, cti_has_property);
    stubCall.addArgument(regT0);
    stubCall.addArgument(regT1);
    stubCall.call();

    // Test for valid key.
    addJump(branchTest32(NonZero, regT0), target);
    jump().linkTo(begin, this);

    // End of loop.
    end.link(this);
}

void JIT::emit_op_push_scope(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_push_scope);
    stubCall.addArgument(currentInstruction[1].u.operand, regT2);
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emit_op_pop_scope(Instruction*)
{
    JITStubCall(this, cti_op_pop_scope).call();
}

void JIT::compileOpStrictEq(Instruction* currentInstruction, CompileOpStrictEqType type)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned src1 = currentInstruction[2].u.operand;
    unsigned src2 = currentInstruction[3].u.operand;

    emitGetVirtualRegisters(src1, regT0, src2, regT1);

    // Jump to a slow case if either operand is a number, or if both are JSCell*s.
    move(regT0, regT2);
    orPtr(regT1, regT2);
    addSlowCase(emitJumpIfJSCell(regT2));
    addSlowCase(emitJumpIfImmediateNumber(regT2));

    if (type == OpStrictEq)
        compare32(Equal, regT1, regT0, regT0);
    else
        compare32(NotEqual, regT1, regT0, regT0);
    emitTagAsBoolImmediate(regT0);

    emitPutVirtualRegister(dst);
}

void JIT::emit_op_stricteq(Instruction* currentInstruction)
{
    compileOpStrictEq(currentInstruction, OpStrictEq);
}

void JIT::emit_op_nstricteq(Instruction* currentInstruction)
{
    compileOpStrictEq(currentInstruction, OpNStrictEq);
}

void JIT::emit_op_to_jsnumber(Instruction* currentInstruction)
{
    int srcVReg = currentInstruction[2].u.operand;
    emitGetVirtualRegister(srcVReg, regT0);
    
    Jump wasImmediate = emitJumpIfImmediateInteger(regT0);

    emitJumpSlowCaseIfNotJSCell(regT0, srcVReg);
    loadPtr(Address(regT0, JSCell::structureOffset()), regT2);
    addSlowCase(branch8(NotEqual, Address(regT2, Structure::typeInfoTypeOffset()), TrustedImm32(NumberType)));
    
    wasImmediate.link(this);

    emitPutVirtualRegister(currentInstruction[1].u.operand);
}

void JIT::emit_op_push_new_scope(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_push_new_scope);
    stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(currentInstruction[2].u.operand)));
    stubCall.addArgument(currentInstruction[3].u.operand, regT2);
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emit_op_catch(Instruction* currentInstruction)
{
    killLastResultRegister(); // FIXME: Implicitly treat op_catch as a labeled statement, and remove this line of code.
    move(regT0, callFrameRegister);
    peek(regT3, OBJECT_OFFSETOF(struct JITStackFrame, globalData) / sizeof(void*));
    loadPtr(Address(regT3, OBJECT_OFFSETOF(JSGlobalData, exception)), regT0);
    storePtr(TrustedImmPtr(JSValue::encode(JSValue())), Address(regT3, OBJECT_OFFSETOF(JSGlobalData, exception)));
    emitPutVirtualRegister(currentInstruction[1].u.operand);
}

void JIT::emit_op_jmp_scopes(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_jmp_scopes);
    stubCall.addArgument(Imm32(currentInstruction[1].u.operand));
    stubCall.call();
    addJump(jump(), currentInstruction[2].u.operand);
}

void JIT::emit_op_switch_imm(Instruction* currentInstruction)
{
    unsigned tableIndex = currentInstruction[1].u.operand;
    unsigned defaultOffset = currentInstruction[2].u.operand;
    unsigned scrutinee = currentInstruction[3].u.operand;

    // create jump table for switch destinations, track this switch statement.
    SimpleJumpTable* jumpTable = &m_codeBlock->immediateSwitchJumpTable(tableIndex);
    m_switches.append(SwitchRecord(jumpTable, m_bytecodeOffset, defaultOffset, SwitchRecord::Immediate));
    jumpTable->ctiOffsets.grow(jumpTable->branchOffsets.size());

    JITStubCall stubCall(this, cti_op_switch_imm);
    stubCall.addArgument(scrutinee, regT2);
    stubCall.addArgument(Imm32(tableIndex));
    stubCall.call();
    jump(regT0);
}

void JIT::emit_op_switch_char(Instruction* currentInstruction)
{
    unsigned tableIndex = currentInstruction[1].u.operand;
    unsigned defaultOffset = currentInstruction[2].u.operand;
    unsigned scrutinee = currentInstruction[3].u.operand;

    // create jump table for switch destinations, track this switch statement.
    SimpleJumpTable* jumpTable = &m_codeBlock->characterSwitchJumpTable(tableIndex);
    m_switches.append(SwitchRecord(jumpTable, m_bytecodeOffset, defaultOffset, SwitchRecord::Character));
    jumpTable->ctiOffsets.grow(jumpTable->branchOffsets.size());

    JITStubCall stubCall(this, cti_op_switch_char);
    stubCall.addArgument(scrutinee, regT2);
    stubCall.addArgument(Imm32(tableIndex));
    stubCall.call();
    jump(regT0);
}

void JIT::emit_op_switch_string(Instruction* currentInstruction)
{
    unsigned tableIndex = currentInstruction[1].u.operand;
    unsigned defaultOffset = currentInstruction[2].u.operand;
    unsigned scrutinee = currentInstruction[3].u.operand;

    // create jump table for switch destinations, track this switch statement.
    StringJumpTable* jumpTable = &m_codeBlock->stringSwitchJumpTable(tableIndex);
    m_switches.append(SwitchRecord(jumpTable, m_bytecodeOffset, defaultOffset));

    JITStubCall stubCall(this, cti_op_switch_string);
    stubCall.addArgument(scrutinee, regT2);
    stubCall.addArgument(Imm32(tableIndex));
    stubCall.call();
    jump(regT0);
}

void JIT::emit_op_throw_reference_error(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_throw_reference_error);
    stubCall.addArgument(ImmPtr(JSValue::encode(m_codeBlock->getConstant(currentInstruction[1].u.operand))));
    stubCall.call();
}

void JIT::emit_op_debug(Instruction* currentInstruction)
{
#if ENABLE(DEBUG_WITH_BREAKPOINT)
    UNUSED_PARAM(currentInstruction);
    breakpoint();
#else
    JITStubCall stubCall(this, cti_op_debug);
    stubCall.addArgument(Imm32(currentInstruction[1].u.operand));
    stubCall.addArgument(Imm32(currentInstruction[2].u.operand));
    stubCall.addArgument(Imm32(currentInstruction[3].u.operand));
    stubCall.call();
#endif
}

void JIT::emit_op_eq_null(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned src1 = currentInstruction[2].u.operand;

    emitGetVirtualRegister(src1, regT0);
    Jump isImmediate = emitJumpIfNotJSCell(regT0);

    loadPtr(Address(regT0, JSCell::structureOffset()), regT2);
    test8(NonZero, Address(regT2, Structure::typeInfoFlagsOffset()), TrustedImm32(MasqueradesAsUndefined), regT0);

    Jump wasNotImmediate = jump();

    isImmediate.link(this);

    andPtr(TrustedImm32(~TagBitUndefined), regT0);
    comparePtr(Equal, regT0, TrustedImm32(ValueNull), regT0);

    wasNotImmediate.link(this);

    emitTagAsBoolImmediate(regT0);
    emitPutVirtualRegister(dst);

}

void JIT::emit_op_neq_null(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned src1 = currentInstruction[2].u.operand;

    emitGetVirtualRegister(src1, regT0);
    Jump isImmediate = emitJumpIfNotJSCell(regT0);

    loadPtr(Address(regT0, JSCell::structureOffset()), regT2);
    test8(Zero, Address(regT2, Structure::typeInfoFlagsOffset()), TrustedImm32(MasqueradesAsUndefined), regT0);

    Jump wasNotImmediate = jump();

    isImmediate.link(this);

    andPtr(TrustedImm32(~TagBitUndefined), regT0);
    comparePtr(NotEqual, regT0, TrustedImm32(ValueNull), regT0);

    wasNotImmediate.link(this);

    emitTagAsBoolImmediate(regT0);
    emitPutVirtualRegister(dst);
}

void JIT::emit_op_enter(Instruction*)
{
    // Even though CTI doesn't use them, we initialize our constant
    // registers to zap stale pointers, to avoid unnecessarily prolonging
    // object lifetime and increasing GC pressure.
    size_t count = m_codeBlock->m_numVars;
    for (size_t j = 0; j < count; ++j)
        emitInitRegister(j);

}

void JIT::emit_op_create_activation(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    
    Jump activationCreated = branchTestPtr(NonZero, Address(callFrameRegister, sizeof(Register) * dst));
    JITStubCall(this, cti_op_push_activation).call(currentInstruction[1].u.operand);
    emitPutVirtualRegister(dst);
    activationCreated.link(this);
}

void JIT::emit_op_create_arguments(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;

    Jump argsCreated = branchTestPtr(NonZero, Address(callFrameRegister, sizeof(Register) * dst));
    if (m_codeBlock->m_numParameters == 1)
        JITStubCall(this, cti_op_create_arguments_no_params).call();
    else
        JITStubCall(this, cti_op_create_arguments).call();
    emitPutVirtualRegister(dst);
    emitPutVirtualRegister(unmodifiedArgumentsRegister(dst));
    argsCreated.link(this);
}

void JIT::emit_op_init_lazy_reg(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;

    storePtr(TrustedImmPtr(0), Address(callFrameRegister, sizeof(Register) * dst));
}

void JIT::emit_op_convert_this(Instruction* currentInstruction)
{
    emitGetVirtualRegister(currentInstruction[1].u.operand, regT0);

    emitJumpSlowCaseIfNotJSCell(regT0);
    loadPtr(Address(regT0, JSCell::structureOffset()), regT1);
    addSlowCase(branchTest8(NonZero, Address(regT1, Structure::typeInfoFlagsOffset()), TrustedImm32(NeedsThisConversion)));
}

void JIT::emit_op_convert_this_strict(Instruction* currentInstruction)
{
    emitGetVirtualRegister(currentInstruction[1].u.operand, regT0);
    Jump notNull = branchTestPtr(NonZero, regT0);
    move(TrustedImmPtr(JSValue::encode(jsNull())), regT0);
    emitPutVirtualRegister(currentInstruction[1].u.operand, regT0);
    Jump setThis = jump();
    notNull.link(this);
    Jump isImmediate = emitJumpIfNotJSCell(regT0);
    loadPtr(Address(regT0, JSCell::structureOffset()), regT1);
    Jump notAnObject = branch8(NotEqual, Address(regT1, Structure::typeInfoTypeOffset()), TrustedImm32(ObjectType));
    addSlowCase(branchTest8(NonZero, Address(regT1, Structure::typeInfoFlagsOffset()), TrustedImm32(NeedsThisConversion)));
    isImmediate.link(this);
    notAnObject.link(this);
    setThis.link(this);
}

void JIT::emit_op_get_callee(Instruction* currentInstruction)
{
    unsigned result = currentInstruction[1].u.operand;
    emitGetFromCallFrameHeaderPtr(RegisterFile::Callee, regT0);
    emitPutVirtualRegister(result);
}

void JIT::emit_op_create_this(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_create_this);
    stubCall.addArgument(currentInstruction[2].u.operand, regT1);
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emit_op_profile_will_call(Instruction* currentInstruction)
{
    peek(regT1, OBJECT_OFFSETOF(JITStackFrame, enabledProfilerReference) / sizeof(void*));
    Jump noProfiler = branchTestPtr(Zero, Address(regT1));

    JITStubCall stubCall(this, cti_op_profile_will_call);
    stubCall.addArgument(currentInstruction[1].u.operand, regT1);
    stubCall.call();
    noProfiler.link(this);

}

void JIT::emit_op_profile_did_call(Instruction* currentInstruction)
{
    peek(regT1, OBJECT_OFFSETOF(JITStackFrame, enabledProfilerReference) / sizeof(void*));
    Jump noProfiler = branchTestPtr(Zero, Address(regT1));

    JITStubCall stubCall(this, cti_op_profile_did_call);
    stubCall.addArgument(currentInstruction[1].u.operand, regT1);
    stubCall.call();
    noProfiler.link(this);
}


// Slow cases

void JIT::emitSlow_op_convert_this(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_convert_this);
    stubCall.addArgument(regT0);
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emitSlow_op_convert_this_strict(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_convert_this_strict);
    stubCall.addArgument(regT0);
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emitSlow_op_to_primitive(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);

    JITStubCall stubCall(this, cti_op_to_primitive);
    stubCall.addArgument(regT0);
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emitSlow_op_loop_if_lesseq(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned op2 = currentInstruction[2].u.operand;
    unsigned target = currentInstruction[3].u.operand;
    if (isOperandConstantImmediateInt(op2)) {
        linkSlowCase(iter);
        JITStubCall stubCall(this, cti_op_loop_if_lesseq);
        stubCall.addArgument(regT0);
        stubCall.addArgument(currentInstruction[2].u.operand, regT2);
        stubCall.call();
        emitJumpSlowToHot(branchTest32(NonZero, regT0), target);
    } else {
        linkSlowCase(iter);
        linkSlowCase(iter);
        JITStubCall stubCall(this, cti_op_loop_if_lesseq);
        stubCall.addArgument(regT0);
        stubCall.addArgument(regT1);
        stubCall.call();
        emitJumpSlowToHot(branchTest32(NonZero, regT0), target);
    }
}

void JIT::emitSlow_op_put_by_val(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned base = currentInstruction[1].u.operand;
    unsigned property = currentInstruction[2].u.operand;
    unsigned value = currentInstruction[3].u.operand;

    linkSlowCase(iter); // property int32 check
    linkSlowCaseIfNotJSCell(iter, base); // base cell check
    linkSlowCase(iter); // base not array check
    linkSlowCase(iter); // in vector check

    JITStubCall stubPutByValCall(this, cti_op_put_by_val);
    stubPutByValCall.addArgument(regT0);
    stubPutByValCall.addArgument(property, regT2);
    stubPutByValCall.addArgument(value, regT2);
    stubPutByValCall.call();
}

void JIT::emitSlow_op_not(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);
    xorPtr(TrustedImm32(static_cast<int32_t>(ValueFalse)), regT0);
    JITStubCall stubCall(this, cti_op_not);
    stubCall.addArgument(regT0);
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emitSlow_op_jfalse(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_jtrue);
    stubCall.addArgument(regT0);
    stubCall.call();
    emitJumpSlowToHot(branchTest32(Zero, regT0), currentInstruction[2].u.operand); // inverted!
}

void JIT::emitSlow_op_bitnot(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_bitnot);
    stubCall.addArgument(regT0);
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emitSlow_op_jtrue(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_jtrue);
    stubCall.addArgument(regT0);
    stubCall.call();
    emitJumpSlowToHot(branchTest32(NonZero, regT0), currentInstruction[2].u.operand);
}

void JIT::emitSlow_op_bitxor(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_bitxor);
    stubCall.addArgument(regT0);
    stubCall.addArgument(regT1);
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emitSlow_op_bitor(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_bitor);
    stubCall.addArgument(regT0);
    stubCall.addArgument(regT1);
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emitSlow_op_eq(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_eq);
    stubCall.addArgument(regT0);
    stubCall.addArgument(regT1);
    stubCall.call();
    emitTagAsBoolImmediate(regT0);
    emitPutVirtualRegister(currentInstruction[1].u.operand);
}

void JIT::emitSlow_op_neq(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_eq);
    stubCall.addArgument(regT0);
    stubCall.addArgument(regT1);
    stubCall.call();
    xor32(TrustedImm32(0x1), regT0);
    emitTagAsBoolImmediate(regT0);
    emitPutVirtualRegister(currentInstruction[1].u.operand);
}

void JIT::emitSlow_op_stricteq(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_stricteq);
    stubCall.addArgument(regT0);
    stubCall.addArgument(regT1);
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emitSlow_op_nstricteq(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_nstricteq);
    stubCall.addArgument(regT0);
    stubCall.addArgument(regT1);
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emitSlow_op_check_has_instance(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned baseVal = currentInstruction[1].u.operand;

    linkSlowCaseIfNotJSCell(iter, baseVal);
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_check_has_instance);
    stubCall.addArgument(baseVal, regT2);
    stubCall.call();
}

void JIT::emitSlow_op_instanceof(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned value = currentInstruction[2].u.operand;
    unsigned baseVal = currentInstruction[3].u.operand;
    unsigned proto = currentInstruction[4].u.operand;

    linkSlowCaseIfNotJSCell(iter, value);
    linkSlowCaseIfNotJSCell(iter, proto);
    linkSlowCase(iter);
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_instanceof);
    stubCall.addArgument(value, regT2);
    stubCall.addArgument(baseVal, regT2);
    stubCall.addArgument(proto, regT2);
    stubCall.call(dst);
}

void JIT::emitSlow_op_call(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    compileOpCallSlowCase(currentInstruction, iter, m_callLinkInfoIndex++, op_call);
}

void JIT::emitSlow_op_call_eval(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    compileOpCallSlowCase(currentInstruction, iter, m_callLinkInfoIndex++, op_call_eval);
}

void JIT::emitSlow_op_call_varargs(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    compileOpCallVarargsSlowCase(currentInstruction, iter);
}

void JIT::emitSlow_op_construct(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    compileOpCallSlowCase(currentInstruction, iter, m_callLinkInfoIndex++, op_construct);
}

void JIT::emitSlow_op_to_jsnumber(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCaseIfNotJSCell(iter, currentInstruction[2].u.operand);
    linkSlowCase(iter);

    JITStubCall stubCall(this, cti_op_to_jsnumber);
    stubCall.addArgument(regT0);
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emit_op_get_arguments_length(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    int argumentsRegister = currentInstruction[2].u.operand;
    addSlowCase(branchTestPtr(NonZero, addressFor(argumentsRegister)));
    emitGetFromCallFrameHeader32(RegisterFile::ArgumentCount, regT0);
    sub32(TrustedImm32(1), regT0);
    emitFastArithReTagImmediate(regT0, regT0);
    emitPutVirtualRegister(dst, regT0);
}

void JIT::emitSlow_op_get_arguments_length(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);
    unsigned dst = currentInstruction[1].u.operand;
    unsigned base = currentInstruction[2].u.operand;
    Identifier* ident = &(m_codeBlock->identifier(currentInstruction[3].u.operand));
    
    emitGetVirtualRegister(base, regT0);
    JITStubCall stubCall(this, cti_op_get_by_id_generic);
    stubCall.addArgument(regT0);
    stubCall.addArgument(TrustedImmPtr(ident));
    stubCall.call(dst);
}

void JIT::emit_op_get_argument_by_val(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    int argumentsRegister = currentInstruction[2].u.operand;
    int property = currentInstruction[3].u.operand;
    addSlowCase(branchTestPtr(NonZero, addressFor(argumentsRegister)));
    emitGetVirtualRegister(property, regT1);
    addSlowCase(emitJumpIfNotImmediateInteger(regT1));
    add32(TrustedImm32(1), regT1);
    // regT1 now contains the integer index of the argument we want, including this
    emitGetFromCallFrameHeader32(RegisterFile::ArgumentCount, regT2);
    addSlowCase(branch32(AboveOrEqual, regT1, regT2));
    
    Jump skipOutofLineParams;
    int numArgs = m_codeBlock->m_numParameters;
    if (numArgs) {
        Jump notInInPlaceArgs = branch32(AboveOrEqual, regT1, Imm32(numArgs));
        addPtr(Imm32(static_cast<unsigned>(-(RegisterFile::CallFrameHeaderSize + numArgs) * sizeof(Register))), callFrameRegister, regT0);
        loadPtr(BaseIndex(regT0, regT1, TimesEight, 0), regT0);
        skipOutofLineParams = jump();
        notInInPlaceArgs.link(this);
    }
    
    addPtr(Imm32(static_cast<unsigned>(-(RegisterFile::CallFrameHeaderSize + numArgs) * sizeof(Register))), callFrameRegister, regT0);
    mul32(TrustedImm32(sizeof(Register)), regT2, regT2);
    subPtr(regT2, regT0);
    loadPtr(BaseIndex(regT0, regT1, TimesEight, 0), regT0);
    if (numArgs)
        skipOutofLineParams.link(this);
    emitPutVirtualRegister(dst, regT0);
}

void JIT::emitSlow_op_get_argument_by_val(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned arguments = currentInstruction[2].u.operand;
    unsigned property = currentInstruction[3].u.operand;
    
    linkSlowCase(iter);
    Jump skipArgumentsCreation = jump();
    
    linkSlowCase(iter);
    linkSlowCase(iter);
    if (m_codeBlock->m_numParameters == 1)
        JITStubCall(this, cti_op_create_arguments_no_params).call();
    else
        JITStubCall(this, cti_op_create_arguments).call();
    emitPutVirtualRegister(arguments);
    emitPutVirtualRegister(unmodifiedArgumentsRegister(arguments));
    
    skipArgumentsCreation.link(this);
    JITStubCall stubCall(this, cti_op_get_by_val);
    stubCall.addArgument(arguments, regT2);
    stubCall.addArgument(property, regT2);
    stubCall.call(dst);
}

#endif // USE(JSVALUE64)

void JIT::emit_op_resolve_global_dynamic(Instruction* currentInstruction)
{
    int skip = currentInstruction[5].u.operand;
    
    emitGetFromCallFrameHeaderPtr(RegisterFile::ScopeChain, regT0);
    
    bool checkTopLevel = m_codeBlock->codeType() == FunctionCode && m_codeBlock->needsFullScopeChain();
    ASSERT(skip || !checkTopLevel);
    if (checkTopLevel && skip--) {
        Jump activationNotCreated;
        if (checkTopLevel)
            activationNotCreated = branchTestPtr(Zero, addressFor(m_codeBlock->activationRegister()));
        loadPtr(Address(regT0, OBJECT_OFFSETOF(ScopeChainNode, object)), regT1);
        addSlowCase(checkStructure(regT1, m_globalData->activationStructure.get()));
        loadPtr(Address(regT0, OBJECT_OFFSETOF(ScopeChainNode, next)), regT0);
        activationNotCreated.link(this);
    }
    while (skip--) {
        loadPtr(Address(regT0, OBJECT_OFFSETOF(ScopeChainNode, object)), regT1);
        addSlowCase(checkStructure(regT1, m_globalData->activationStructure.get()));
        loadPtr(Address(regT0, OBJECT_OFFSETOF(ScopeChainNode, next)), regT0);
    }
    emit_op_resolve_global(currentInstruction, true);
}

void JIT::emitSlow_op_resolve_global_dynamic(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;
    Identifier* ident = &m_codeBlock->identifier(currentInstruction[2].u.operand);
    int skip = currentInstruction[5].u.operand;
    while (skip--)
        linkSlowCase(iter);
    JITStubCall resolveStubCall(this, cti_op_resolve);
    resolveStubCall.addArgument(TrustedImmPtr(ident));
    resolveStubCall.call(dst);
    emitJumpSlowToHot(jump(), OPCODE_LENGTH(op_resolve_global_dynamic));
    
    unsigned currentIndex = m_globalResolveInfoIndex++;
    
    linkSlowCase(iter); // We managed to skip all the nodes in the scope chain, but the cache missed.
    JITStubCall stubCall(this, cti_op_resolve_global);
    stubCall.addArgument(TrustedImmPtr(ident));
    stubCall.addArgument(Imm32(currentIndex));
    stubCall.addArgument(regT0);
    stubCall.call(dst);
}

void JIT::emit_op_new_regexp(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_new_regexp);
    stubCall.addArgument(TrustedImmPtr(m_codeBlock->regexp(currentInstruction[2].u.operand)));
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emit_op_load_varargs(Instruction* currentInstruction)
{
    int argCountDst = currentInstruction[1].u.operand;
    int argsOffset = currentInstruction[2].u.operand;
    int registerOffset = currentInstruction[3].u.operand;
    ASSERT(argsOffset <= registerOffset);
    
    int expectedParams = m_codeBlock->m_numParameters - 1;
    // Don't do inline copying if we aren't guaranteed to have a single stream
    // of arguments
    if (expectedParams) {
        JITStubCall stubCall(this, cti_op_load_varargs);
        stubCall.addArgument(Imm32(argsOffset));
        stubCall.call();
        // Stores a naked int32 in the register file.
        store32(returnValueRegister, Address(callFrameRegister, argCountDst * sizeof(Register)));
        return;
    }

#if USE(JSVALUE32_64)
    addSlowCase(branch32(NotEqual, tagFor(argsOffset), TrustedImm32(JSValue::EmptyValueTag)));
#else
    addSlowCase(branchTestPtr(NonZero, addressFor(argsOffset)));
#endif
    // Load arg count into regT0
    emitGetFromCallFrameHeader32(RegisterFile::ArgumentCount, regT0);
    store32(TrustedImm32(Int32Tag), intTagFor(argCountDst));
    store32(regT0, intPayloadFor(argCountDst));
    Jump endBranch = branch32(Equal, regT0, TrustedImm32(1));

    mul32(TrustedImm32(sizeof(Register)), regT0, regT3);
    addPtr(TrustedImm32(static_cast<unsigned>(sizeof(Register) - RegisterFile::CallFrameHeaderSize * sizeof(Register))), callFrameRegister, regT1);
    subPtr(regT3, regT1); // regT1 is now the start of the out of line arguments
    addPtr(Imm32(argsOffset * sizeof(Register)), callFrameRegister, regT2); // regT2 is the target buffer
    
    // Bounds check the registerfile
    addPtr(regT2, regT3);
    addPtr(Imm32((registerOffset - argsOffset) * sizeof(Register)), regT3);
    addSlowCase(branchPtr(Below, AbsoluteAddress(m_globalData->interpreter->registerFile().addressOfEnd()), regT3));

    sub32(TrustedImm32(1), regT0);
    Label loopStart = label();
    loadPtr(BaseIndex(regT1, regT0, TimesEight, static_cast<unsigned>(0 - 2 * sizeof(Register))), regT3);
    storePtr(regT3, BaseIndex(regT2, regT0, TimesEight, static_cast<unsigned>(0 - sizeof(Register))));
#if USE(JSVALUE32_64)
    loadPtr(BaseIndex(regT1, regT0, TimesEight, static_cast<unsigned>(sizeof(void*) - 2 * sizeof(Register))), regT3);
    storePtr(regT3, BaseIndex(regT2, regT0, TimesEight, static_cast<unsigned>(sizeof(void*) - sizeof(Register))));
#endif
    branchSubPtr(NonZero, TrustedImm32(1), regT0).linkTo(loopStart, this);
    endBranch.link(this);
}

void JIT::emitSlow_op_load_varargs(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    int argCountDst = currentInstruction[1].u.operand;
    int argsOffset = currentInstruction[2].u.operand;
    int expectedParams = m_codeBlock->m_numParameters - 1;
    if (expectedParams)
        return;
    
    linkSlowCase(iter);
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_load_varargs);
    stubCall.addArgument(Imm32(argsOffset));
    stubCall.call();
    
    store32(TrustedImm32(Int32Tag), intTagFor(argCountDst));
    store32(returnValueRegister, intPayloadFor(argCountDst));
}

void JIT::emit_op_new_func(Instruction* currentInstruction)
{
    Jump lazyJump;
    int dst = currentInstruction[1].u.operand;
    if (currentInstruction[3].u.operand) {
#if USE(JSVALUE32_64)
        lazyJump = branch32(NotEqual, tagFor(dst), TrustedImm32(JSValue::EmptyValueTag));
#else
        lazyJump = branchTestPtr(NonZero, addressFor(dst));
#endif
    }
    JITStubCall stubCall(this, cti_op_new_func);
    stubCall.addArgument(TrustedImmPtr(m_codeBlock->functionDecl(currentInstruction[2].u.operand)));
    stubCall.call(currentInstruction[1].u.operand);
    if (currentInstruction[3].u.operand)
        lazyJump.link(this);
}

} // namespace JSC

#endif // ENABLE(JIT)
