/*
 * Copyright (C) 2009, 2012 Apple Inc. All rights reserved.
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
#include "CopiedSpaceInlines.h"
#include "Heap.h"
#include "JITInlines.h"
#include "JITStubCall.h"
#include "JSArray.h"
#include "JSCell.h"
#include "JSFunction.h"
#include "JSPropertyNameIterator.h"
#include "LinkBuffer.h"

namespace JSC {

#if USE(JSVALUE64)

JIT::CodeRef JIT::privateCompileCTINativeCall(VM* vm, NativeFunction)
{
    return vm->getCTIStub(nativeCallGenerator);
}

void JIT::emit_op_mov(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    int src = currentInstruction[2].u.operand;

    if (canBeOptimizedOrInlined()) {
        // Use simpler approach, since the DFG thinks that the last result register
        // is always set to the destination on every operation.
        emitGetVirtualRegister(src, regT0);
        emitPutVirtualRegister(dst);
    } else {
        if (m_codeBlock->isConstantRegisterIndex(src)) {
            if (!getConstantOperand(src).isNumber())
                store64(TrustedImm64(JSValue::encode(getConstantOperand(src))), Address(callFrameRegister, dst * sizeof(Register)));
            else
                store64(Imm64(JSValue::encode(getConstantOperand(src))), Address(callFrameRegister, dst * sizeof(Register)));
            if (dst == m_lastResultBytecodeRegister)
                killLastResultRegister();
        } else if ((src == m_lastResultBytecodeRegister) || (dst == m_lastResultBytecodeRegister)) {
            // If either the src or dst is the cached register go though
            // get/put registers to make sure we track this correctly.
            emitGetVirtualRegister(src, regT0);
            emitPutVirtualRegister(dst);
        } else {
            // Perform the copy via regT1; do not disturb any mapping in regT0.
            load64(Address(callFrameRegister, src * sizeof(Register)), regT1);
            store64(regT1, Address(callFrameRegister, dst * sizeof(Register)));
        }
    }
}

void JIT::emit_op_end(Instruction* currentInstruction)
{
    RELEASE_ASSERT(returnValueRegister != callFrameRegister);
    emitGetVirtualRegister(currentInstruction[1].u.operand, returnValueRegister);
    restoreReturnAddressBeforeReturn(Address(callFrameRegister, JSStack::ReturnPC * static_cast<int>(sizeof(Register))));
    ret();
}

void JIT::emit_op_jmp(Instruction* currentInstruction)
{
    unsigned target = currentInstruction[1].u.operand;
    addJump(jump(), target);
}

void JIT::emit_op_new_object(Instruction* currentInstruction)
{
    Structure* structure = currentInstruction[3].u.objectAllocationProfile->structure();
    size_t allocationSize = JSObject::allocationSize(structure->inlineCapacity());
    MarkedAllocator* allocator = &m_vm->heap.allocatorForObjectWithoutDestructor(allocationSize);

    RegisterID resultReg = regT0;
    RegisterID allocatorReg = regT1;
    RegisterID scratchReg = regT2;

    move(TrustedImmPtr(allocator), allocatorReg);
    emitAllocateJSObject(allocatorReg, TrustedImmPtr(structure), resultReg, scratchReg);
    emitPutVirtualRegister(currentInstruction[1].u.operand);
}

void JIT::emitSlow_op_new_object(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_new_object);
    stubCall.addArgument(TrustedImmPtr(currentInstruction[3].u.objectAllocationProfile->structure()));
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emit_op_check_has_instance(Instruction* currentInstruction)
{
    unsigned baseVal = currentInstruction[3].u.operand;

    emitGetVirtualRegister(baseVal, regT0);

    // Check that baseVal is a cell.
    emitJumpSlowCaseIfNotJSCell(regT0, baseVal);

    // Check that baseVal 'ImplementsHasInstance'.
    loadPtr(Address(regT0, JSCell::structureOffset()), regT0);
    addSlowCase(branchTest8(Zero, Address(regT0, Structure::typeInfoFlagsOffset()), TrustedImm32(ImplementsDefaultHasInstance)));
}

void JIT::emit_op_instanceof(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned value = currentInstruction[2].u.operand;
    unsigned proto = currentInstruction[3].u.operand;

    // Load the operands (baseVal, proto, and value respectively) into registers.
    // We use regT0 for baseVal since we will be done with this first, and we can then use it for the result.
    emitGetVirtualRegister(value, regT2);
    emitGetVirtualRegister(proto, regT1);

    // Check that proto are cells.  baseVal must be a cell - this is checked by op_check_has_instance.
    emitJumpSlowCaseIfNotJSCell(regT2, value);
    emitJumpSlowCaseIfNotJSCell(regT1, proto);

    // Check that prototype is an object
    loadPtr(Address(regT1, JSCell::structureOffset()), regT3);
    addSlowCase(emitJumpIfNotObject(regT3));
    
    // Optimistically load the result true, and start looping.
    // Initially, regT1 still contains proto and regT2 still contains value.
    // As we loop regT2 will be updated with its prototype, recursively walking the prototype chain.
    move(TrustedImm64(JSValue::encode(jsBoolean(true))), regT0);
    Label loop(this);

    // Load the prototype of the object in regT2.  If this is equal to regT1 - WIN!
    // Otherwise, check if we've hit null - if we have then drop out of the loop, if not go again.
    loadPtr(Address(regT2, JSCell::structureOffset()), regT2);
    load64(Address(regT2, Structure::prototypeOffset()), regT2);
    Jump isInstance = branchPtr(Equal, regT2, regT1);
    emitJumpIfJSCell(regT2).linkTo(loop, this);

    // We get here either by dropping out of the loop, or if value was not an Object.  Result is false.
    move(TrustedImm64(JSValue::encode(jsBoolean(false))), regT0);

    // isInstance jumps right down to here, to skip setting the result to false (it has already set true).
    isInstance.link(this);
    emitPutVirtualRegister(dst);
}

void JIT::emit_op_is_undefined(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned value = currentInstruction[2].u.operand;
    
    emitGetVirtualRegister(value, regT0);
    Jump isCell = emitJumpIfJSCell(regT0);

    compare64(Equal, regT0, TrustedImm32(ValueUndefined), regT0);
    Jump done = jump();
    
    isCell.link(this);
    loadPtr(Address(regT0, JSCell::structureOffset()), regT1);
    Jump isMasqueradesAsUndefined = branchTest8(NonZero, Address(regT1, Structure::typeInfoFlagsOffset()), TrustedImm32(MasqueradesAsUndefined));
    move(TrustedImm32(0), regT0);
    Jump notMasqueradesAsUndefined = jump();

    isMasqueradesAsUndefined.link(this);
    move(TrustedImmPtr(m_codeBlock->globalObject()), regT0);
    loadPtr(Address(regT1, Structure::globalObjectOffset()), regT1);
    comparePtr(Equal, regT0, regT1, regT0);

    notMasqueradesAsUndefined.link(this);
    done.link(this);
    emitTagAsBoolImmediate(regT0);
    emitPutVirtualRegister(dst);
}

void JIT::emit_op_is_boolean(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned value = currentInstruction[2].u.operand;
    
    emitGetVirtualRegister(value, regT0);
    xor64(TrustedImm32(static_cast<int32_t>(ValueFalse)), regT0);
    test64(Zero, regT0, TrustedImm32(static_cast<int32_t>(~1)), regT0);
    emitTagAsBoolImmediate(regT0);
    emitPutVirtualRegister(dst);
}

void JIT::emit_op_is_number(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned value = currentInstruction[2].u.operand;
    
    emitGetVirtualRegister(value, regT0);
    test64(NonZero, regT0, tagTypeNumberRegister, regT0);
    emitTagAsBoolImmediate(regT0);
    emitPutVirtualRegister(dst);
}

void JIT::emit_op_is_string(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned value = currentInstruction[2].u.operand;
    
    emitGetVirtualRegister(value, regT0);
    Jump isNotCell = emitJumpIfNotJSCell(regT0);
    
    loadPtr(Address(regT0, JSCell::structureOffset()), regT1);
    compare8(Equal, Address(regT1, Structure::typeInfoTypeOffset()), TrustedImm32(StringType), regT0);
    emitTagAsBoolImmediate(regT0);
    Jump done = jump();
    
    isNotCell.link(this);
    move(TrustedImm32(ValueFalse), regT0);
    
    done.link(this);
    emitPutVirtualRegister(dst);
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

void JIT::emit_op_tear_off_activation(Instruction* currentInstruction)
{
    int activation = currentInstruction[1].u.operand;
    Jump activationNotCreated = branchTest64(Zero, addressFor(activation));
    JITStubCall stubCall(this, cti_op_tear_off_activation);
    stubCall.addArgument(activation, regT2);
    stubCall.call();
    activationNotCreated.link(this);
}

void JIT::emit_op_tear_off_arguments(Instruction* currentInstruction)
{
    int arguments = currentInstruction[1].u.operand;
    int activation = currentInstruction[2].u.operand;

    Jump argsNotCreated = branchTest64(Zero, Address(callFrameRegister, sizeof(Register) * (unmodifiedArgumentsRegister(arguments))));
    JITStubCall stubCall(this, cti_op_tear_off_arguments);
    stubCall.addArgument(unmodifiedArgumentsRegister(arguments), regT2);
    stubCall.addArgument(activation, regT2);
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
    emitGetFromCallFrameHeaderPtr(JSStack::ReturnPC, regT1);

    // Restore our caller's "r".
    emitGetFromCallFrameHeaderPtr(JSStack::CallerFrame, callFrameRegister);

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
    Jump notObject = emitJumpIfNotObject(regT2);

    // Grab the return address.
    emitGetFromCallFrameHeaderPtr(JSStack::ReturnPC, regT1);

    // Restore our caller's "r".
    emitGetFromCallFrameHeaderPtr(JSStack::CallerFrame, callFrameRegister);

    // Return.
    restoreReturnAddressBeforeReturn(regT1);
    ret();

    // Return 'this' in %eax.
    notJSCell.link(this);
    notObject.link(this);
    emitGetVirtualRegister(currentInstruction[2].u.operand, returnValueRegister);

    // Grab the return address.
    emitGetFromCallFrameHeaderPtr(JSStack::ReturnPC, regT1);

    // Restore our caller's "r".
    emitGetFromCallFrameHeaderPtr(JSStack::CallerFrame, callFrameRegister);

    // Return.
    restoreReturnAddressBeforeReturn(regT1);
    ret();
}

void JIT::emit_op_to_primitive(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    int src = currentInstruction[2].u.operand;

    emitGetVirtualRegister(src, regT0);
    
    Jump isImm = emitJumpIfNotJSCell(regT0);
    addSlowCase(branchPtr(NotEqual, Address(regT0, JSCell::structureOffset()), TrustedImmPtr(m_vm->stringStructure.get())));
    isImm.link(this);

    if (dst != src)
        emitPutVirtualRegister(dst);

}

void JIT::emit_op_strcat(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_strcat);
    stubCall.addArgument(TrustedImm32(currentInstruction[2].u.operand));
    stubCall.addArgument(TrustedImm32(currentInstruction[3].u.operand));
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emit_op_not(Instruction* currentInstruction)
{
    emitGetVirtualRegister(currentInstruction[2].u.operand, regT0);

    // Invert against JSValue(false); if the value was tagged as a boolean, then all bits will be
    // clear other than the low bit (which will be 0 or 1 for false or true inputs respectively).
    // Then invert against JSValue(true), which will add the tag back in, and flip the low bit.
    xor64(TrustedImm32(static_cast<int32_t>(ValueFalse)), regT0);
    addSlowCase(branchTestPtr(NonZero, regT0, TrustedImm32(static_cast<int32_t>(~1))));
    xor64(TrustedImm32(static_cast<int32_t>(ValueTrue)), regT0);

    emitPutVirtualRegister(currentInstruction[1].u.operand);
}

void JIT::emit_op_jfalse(Instruction* currentInstruction)
{
    unsigned target = currentInstruction[2].u.operand;
    emitGetVirtualRegister(currentInstruction[1].u.operand, regT0);

    addJump(branch64(Equal, regT0, TrustedImm64(JSValue::encode(jsNumber(0)))), target);
    Jump isNonZero = emitJumpIfImmediateInteger(regT0);

    addJump(branch64(Equal, regT0, TrustedImm64(JSValue::encode(jsBoolean(false)))), target);
    addSlowCase(branch64(NotEqual, regT0, TrustedImm64(JSValue::encode(jsBoolean(true)))));

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
    Jump isNotMasqueradesAsUndefined = branchTest8(Zero, Address(regT2, Structure::typeInfoFlagsOffset()), TrustedImm32(MasqueradesAsUndefined));
    move(TrustedImmPtr(m_codeBlock->globalObject()), regT0);
    addJump(branchPtr(Equal, Address(regT2, Structure::globalObjectOffset()), regT0), target);
    Jump masqueradesGlobalObjectIsForeign = jump();

    // Now handle the immediate cases - undefined & null
    isImmediate.link(this);
    and64(TrustedImm32(~TagBitUndefined), regT0);
    addJump(branch64(Equal, regT0, TrustedImm64(JSValue::encode(jsNull()))), target);            

    isNotMasqueradesAsUndefined.link(this);
    masqueradesGlobalObjectIsForeign.link(this);
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
    move(TrustedImmPtr(m_codeBlock->globalObject()), regT0);
    addJump(branchPtr(NotEqual, Address(regT2, Structure::globalObjectOffset()), regT0), target);
    Jump wasNotImmediate = jump();

    // Now handle the immediate cases - undefined & null
    isImmediate.link(this);
    and64(TrustedImm32(~TagBitUndefined), regT0);
    addJump(branch64(NotEqual, regT0, TrustedImm64(JSValue::encode(jsNull()))), target);            

    wasNotImmediate.link(this);
}

void JIT::emit_op_jneq_ptr(Instruction* currentInstruction)
{
    unsigned src = currentInstruction[1].u.operand;
    Special::Pointer ptr = currentInstruction[2].u.specialPointer;
    unsigned target = currentInstruction[3].u.operand;
    
    emitGetVirtualRegister(src, regT0);
    addJump(branchPtr(NotEqual, regT0, TrustedImmPtr(actualPointerFor(m_codeBlock, ptr))), target);
}

void JIT::emit_op_eq(Instruction* currentInstruction)
{
    emitGetVirtualRegisters(currentInstruction[2].u.operand, regT0, currentInstruction[3].u.operand, regT1);
    emitJumpSlowCaseIfNotImmediateIntegers(regT0, regT1, regT2);
    compare32(Equal, regT1, regT0, regT0);
    emitTagAsBoolImmediate(regT0);
    emitPutVirtualRegister(currentInstruction[1].u.operand);
}

void JIT::emit_op_jtrue(Instruction* currentInstruction)
{
    unsigned target = currentInstruction[2].u.operand;
    emitGetVirtualRegister(currentInstruction[1].u.operand, regT0);

    Jump isZero = branch64(Equal, regT0, TrustedImm64(JSValue::encode(jsNumber(0))));
    addJump(emitJumpIfImmediateInteger(regT0), target);

    addJump(branch64(Equal, regT0, TrustedImm64(JSValue::encode(jsBoolean(true)))), target);
    addSlowCase(branch64(NotEqual, regT0, TrustedImm64(JSValue::encode(jsBoolean(false)))));

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
    xor64(regT1, regT0);
    emitFastArithReTagImmediate(regT0, regT0);
    emitPutVirtualRegister(currentInstruction[1].u.operand);
}

void JIT::emit_op_bitor(Instruction* currentInstruction)
{
    emitGetVirtualRegisters(currentInstruction[2].u.operand, regT0, currentInstruction[3].u.operand, regT1);
    emitJumpSlowCaseIfNotImmediateIntegers(regT0, regT1, regT2);
    or64(regT1, regT0);
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
        isNotObject.append(emitJumpIfNotObject(regT2));
    }

    // We could inline the case where you have a valid cache, but
    // this call doesn't seem to be hot.
    Label isObject(this);
    JITStubCall getPnamesStubCall(this, cti_op_get_pnames);
    getPnamesStubCall.addArgument(regT0);
    getPnamesStubCall.call(dst);
    load32(Address(regT0, OBJECT_OFFSETOF(JSPropertyNameIterator, m_jsStringsSize)), regT3);
    store64(tagTypeNumberRegister, addressFor(i));
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

    load64(BaseIndex(regT2, regT0, TimesEight), regT2);

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
    load64(Address(regT2, Structure::prototypeOffset()), regT2);
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

void JIT::emit_op_push_with_scope(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_push_with_scope);
    stubCall.addArgument(currentInstruction[1].u.operand, regT2);
    stubCall.call();
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
    
    // Jump slow if both are cells (to cover strings).
    move(regT0, regT2);
    or64(regT1, regT2);
    addSlowCase(emitJumpIfJSCell(regT2));
    
    // Jump slow if either is a double. First test if it's an integer, which is fine, and then test
    // if it's a double.
    Jump leftOK = emitJumpIfImmediateInteger(regT0);
    addSlowCase(emitJumpIfImmediateNumber(regT0));
    leftOK.link(this);
    Jump rightOK = emitJumpIfImmediateInteger(regT1);
    addSlowCase(emitJumpIfImmediateNumber(regT1));
    rightOK.link(this);

    if (type == OpStrictEq)
        compare64(Equal, regT1, regT0, regT0);
    else
        compare64(NotEqual, regT1, regT0, regT0);
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

void JIT::emit_op_to_number(Instruction* currentInstruction)
{
    int srcVReg = currentInstruction[2].u.operand;
    emitGetVirtualRegister(srcVReg, regT0);
    
    addSlowCase(emitJumpIfNotImmediateNumber(regT0));

    emitPutVirtualRegister(currentInstruction[1].u.operand);
}

void JIT::emit_op_push_name_scope(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_push_name_scope);
    stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(currentInstruction[1].u.operand)));
    stubCall.addArgument(currentInstruction[2].u.operand, regT2);
    stubCall.addArgument(TrustedImm32(currentInstruction[3].u.operand));
    stubCall.call();
}

void JIT::emit_op_catch(Instruction* currentInstruction)
{
    killLastResultRegister(); // FIXME: Implicitly treat op_catch as a labeled statement, and remove this line of code.
    move(regT0, callFrameRegister);
    peek(regT3, OBJECT_OFFSETOF(struct JITStackFrame, vm) / sizeof(void*));
    load64(Address(regT3, OBJECT_OFFSETOF(VM, exception)), regT0);
    store64(TrustedImm64(JSValue::encode(JSValue())), Address(regT3, OBJECT_OFFSETOF(VM, exception)));
    emitPutVirtualRegister(currentInstruction[1].u.operand);
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
    stubCall.addArgument(TrustedImm32(tableIndex));
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
    stubCall.addArgument(TrustedImm32(tableIndex));
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
    stubCall.addArgument(TrustedImm32(tableIndex));
    stubCall.call();
    jump(regT0);
}

void JIT::emit_op_throw_static_error(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_throw_static_error);
    if (!m_codeBlock->getConstant(currentInstruction[1].u.operand).isNumber())
        stubCall.addArgument(TrustedImm64(JSValue::encode(m_codeBlock->getConstant(currentInstruction[1].u.operand))));
    else
        stubCall.addArgument(Imm64(JSValue::encode(m_codeBlock->getConstant(currentInstruction[1].u.operand))));
    stubCall.addArgument(TrustedImm32(currentInstruction[2].u.operand));
    stubCall.call();
}

void JIT::emit_op_debug(Instruction* currentInstruction)
{
#if ENABLE(DEBUG_WITH_BREAKPOINT)
    UNUSED_PARAM(currentInstruction);
    breakpoint();
#else
    JITStubCall stubCall(this, cti_op_debug);
    stubCall.addArgument(TrustedImm32(currentInstruction[1].u.operand));
    stubCall.addArgument(TrustedImm32(currentInstruction[2].u.operand));
    stubCall.addArgument(TrustedImm32(currentInstruction[3].u.operand));
    stubCall.addArgument(TrustedImm32(currentInstruction[4].u.operand));
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
    Jump isMasqueradesAsUndefined = branchTest8(NonZero, Address(regT2, Structure::typeInfoFlagsOffset()), TrustedImm32(MasqueradesAsUndefined));
    move(TrustedImm32(0), regT0);
    Jump wasNotMasqueradesAsUndefined = jump();

    isMasqueradesAsUndefined.link(this);
    move(TrustedImmPtr(m_codeBlock->globalObject()), regT0);
    loadPtr(Address(regT2, Structure::globalObjectOffset()), regT2);
    comparePtr(Equal, regT0, regT2, regT0);
    Jump wasNotImmediate = jump();

    isImmediate.link(this);

    and64(TrustedImm32(~TagBitUndefined), regT0);
    compare64(Equal, regT0, TrustedImm32(ValueNull), regT0);

    wasNotImmediate.link(this);
    wasNotMasqueradesAsUndefined.link(this);

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
    Jump isMasqueradesAsUndefined = branchTest8(NonZero, Address(regT2, Structure::typeInfoFlagsOffset()), TrustedImm32(MasqueradesAsUndefined));
    move(TrustedImm32(1), regT0);
    Jump wasNotMasqueradesAsUndefined = jump();

    isMasqueradesAsUndefined.link(this);
    move(TrustedImmPtr(m_codeBlock->globalObject()), regT0);
    loadPtr(Address(regT2, Structure::globalObjectOffset()), regT2);
    comparePtr(NotEqual, regT0, regT2, regT0);
    Jump wasNotImmediate = jump();

    isImmediate.link(this);

    and64(TrustedImm32(~TagBitUndefined), regT0);
    compare64(NotEqual, regT0, TrustedImm32(ValueNull), regT0);

    wasNotImmediate.link(this);
    wasNotMasqueradesAsUndefined.link(this);

    emitTagAsBoolImmediate(regT0);
    emitPutVirtualRegister(dst);
}

void JIT::emit_op_enter(Instruction*)
{
    emitEnterOptimizationCheck();
    
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
    
    Jump activationCreated = branchTest64(NonZero, Address(callFrameRegister, sizeof(Register) * dst));
    JITStubCall(this, cti_op_push_activation).call(currentInstruction[1].u.operand);
    emitPutVirtualRegister(dst);
    activationCreated.link(this);
}

void JIT::emit_op_create_arguments(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;

    Jump argsCreated = branchTest64(NonZero, Address(callFrameRegister, sizeof(Register) * dst));
    JITStubCall(this, cti_op_create_arguments).call();
    emitPutVirtualRegister(dst);
    emitPutVirtualRegister(unmodifiedArgumentsRegister(dst));
    argsCreated.link(this);
}

void JIT::emit_op_init_lazy_reg(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;

    store64(TrustedImm64((int64_t)0), Address(callFrameRegister, sizeof(Register) * dst));
}

void JIT::emit_op_convert_this(Instruction* currentInstruction)
{
    emitGetVirtualRegister(currentInstruction[1].u.operand, regT1);

    emitJumpSlowCaseIfNotJSCell(regT1);
    if (shouldEmitProfiling()) {
        loadPtr(Address(regT1, JSCell::structureOffset()), regT0);
        emitValueProfilingSite();
    }
    addSlowCase(branchPtr(Equal, Address(regT1, JSCell::structureOffset()), TrustedImmPtr(m_vm->stringStructure.get())));
}

void JIT::emit_op_get_callee(Instruction* currentInstruction)
{
    unsigned result = currentInstruction[1].u.operand;
    emitGetFromCallFrameHeaderPtr(JSStack::Callee, regT0);
    emitValueProfilingSite();
    emitPutVirtualRegister(result);
}

void JIT::emit_op_create_this(Instruction* currentInstruction)
{
    int callee = currentInstruction[2].u.operand;
    RegisterID calleeReg = regT0;
    RegisterID resultReg = regT0;
    RegisterID allocatorReg = regT1;
    RegisterID structureReg = regT2;
    RegisterID scratchReg = regT3;

    emitGetVirtualRegister(callee, calleeReg);
    loadPtr(Address(calleeReg, JSFunction::offsetOfAllocationProfile() + ObjectAllocationProfile::offsetOfAllocator()), allocatorReg);
    loadPtr(Address(calleeReg, JSFunction::offsetOfAllocationProfile() + ObjectAllocationProfile::offsetOfStructure()), structureReg);
    addSlowCase(branchTestPtr(Zero, allocatorReg));

    emitAllocateJSObject(allocatorReg, structureReg, resultReg, scratchReg);
    emitPutVirtualRegister(currentInstruction[1].u.operand);
}

void JIT::emitSlow_op_create_this(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter); // doesn't have an allocation profile
    linkSlowCase(iter); // allocation failed

    JITStubCall stubCall(this, cti_op_create_this);
    stubCall.addArgument(TrustedImm32(currentInstruction[3].u.operand));
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emit_op_profile_will_call(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_profile_will_call);
    stubCall.addArgument(currentInstruction[1].u.operand, regT1);
    stubCall.call();
}

void JIT::emit_op_profile_did_call(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_profile_did_call);
    stubCall.addArgument(currentInstruction[1].u.operand, regT1);
    stubCall.call();
}


// Slow cases

void JIT::emitSlow_op_convert_this(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    void* globalThis = m_codeBlock->globalObject()->globalThis();

    linkSlowCase(iter);
    if (shouldEmitProfiling())
        move(TrustedImm64((JSValue::encode(jsUndefined()))), regT0);
    Jump isNotUndefined = branch64(NotEqual, regT1, TrustedImm64(JSValue::encode(jsUndefined())));
    emitValueProfilingSite();
    move(TrustedImm64(JSValue::encode(JSValue(static_cast<JSCell*>(globalThis)))), regT0);
    emitPutVirtualRegister(currentInstruction[1].u.operand, regT0);
    emitJumpSlowToHot(jump(), OPCODE_LENGTH(op_convert_this));

    linkSlowCase(iter);
    if (shouldEmitProfiling())
        move(TrustedImm64(JSValue::encode(m_vm->stringStructure.get())), regT0);
    isNotUndefined.link(this);
    emitValueProfilingSite();
    JITStubCall stubCall(this, cti_op_convert_this);
    stubCall.addArgument(regT1);
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emitSlow_op_to_primitive(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);

    JITStubCall stubCall(this, cti_op_to_primitive);
    stubCall.addArgument(regT0);
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emitSlow_op_not(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);
    xor64(TrustedImm32(static_cast<int32_t>(ValueFalse)), regT0);
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
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_nstricteq);
    stubCall.addArgument(regT0);
    stubCall.addArgument(regT1);
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emitSlow_op_check_has_instance(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned value = currentInstruction[2].u.operand;
    unsigned baseVal = currentInstruction[3].u.operand;

    linkSlowCaseIfNotJSCell(iter, baseVal);
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_check_has_instance);
    stubCall.addArgument(value, regT2);
    stubCall.addArgument(baseVal, regT2);
    stubCall.call(dst);

    emitJumpSlowToHot(jump(), currentInstruction[4].u.operand);
}

void JIT::emitSlow_op_instanceof(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned value = currentInstruction[2].u.operand;
    unsigned proto = currentInstruction[3].u.operand;

    linkSlowCaseIfNotJSCell(iter, value);
    linkSlowCaseIfNotJSCell(iter, proto);
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_instanceof);
    stubCall.addArgument(value, regT2);
    stubCall.addArgument(proto, regT2);
    stubCall.call(dst);
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

void JIT::emitSlow_op_to_number(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);

    JITStubCall stubCall(this, cti_op_to_number);
    stubCall.addArgument(regT0);
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emit_op_get_arguments_length(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    int argumentsRegister = currentInstruction[2].u.operand;
    addSlowCase(branchTest64(NonZero, addressFor(argumentsRegister)));
    emitGetFromCallFrameHeader32(JSStack::ArgumentCount, regT0);
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
    addSlowCase(branchTest64(NonZero, addressFor(argumentsRegister)));
    emitGetVirtualRegister(property, regT1);
    addSlowCase(emitJumpIfNotImmediateInteger(regT1));
    add32(TrustedImm32(1), regT1);
    // regT1 now contains the integer index of the argument we want, including this
    emitGetFromCallFrameHeader32(JSStack::ArgumentCount, regT2);
    addSlowCase(branch32(AboveOrEqual, regT1, regT2));

    neg32(regT1);
    signExtend32ToPtr(regT1, regT1);
    load64(BaseIndex(callFrameRegister, regT1, TimesEight, CallFrame::thisArgumentOffset() * static_cast<int>(sizeof(Register))), regT0);
    emitValueProfilingSite();
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
    JITStubCall(this, cti_op_create_arguments).call();
    emitPutVirtualRegister(arguments);
    emitPutVirtualRegister(unmodifiedArgumentsRegister(arguments));
    
    skipArgumentsCreation.link(this);
    JITStubCall stubCall(this, cti_op_get_by_val_generic);
    stubCall.addArgument(arguments, regT2);
    stubCall.addArgument(property, regT2);
    stubCall.callWithValueProfiling(dst);
}

void JIT::emit_op_put_to_base(Instruction* currentInstruction)
{
    int base = currentInstruction[1].u.operand;
    int id = currentInstruction[2].u.operand;
    int value = currentInstruction[3].u.operand;

    PutToBaseOperation* operation = currentInstruction[4].u.putToBaseOperation;
    switch (operation->m_kind) {
    case PutToBaseOperation::GlobalVariablePutChecked:
        addSlowCase(branchTest8(NonZero, AbsoluteAddress(operation->m_predicatePointer)));
    case PutToBaseOperation::GlobalVariablePut: {
        JSGlobalObject* globalObject = m_codeBlock->globalObject();
        if (operation->m_isDynamic) {
            emitGetVirtualRegister(base, regT0);
            addSlowCase(branchPtr(NotEqual, regT0, TrustedImmPtr(globalObject)));
        }
        emitGetVirtualRegister(value, regT0);
        store64(regT0, operation->m_registerAddress);
        if (Heap::isWriteBarrierEnabled())
            emitWriteBarrier(globalObject, regT0, regT2, ShouldFilterImmediates, WriteBarrierForVariableAccess);
        return;
    }
    case PutToBaseOperation::VariablePut: {
        emitGetVirtualRegisters(base, regT0, value, regT1);
        loadPtr(Address(regT0, JSVariableObject::offsetOfRegisters()), regT2);
        store64(regT1, Address(regT2, operation->m_offset * sizeof(Register)));
        if (Heap::isWriteBarrierEnabled())
            emitWriteBarrier(regT0, regT1, regT2, regT3, ShouldFilterImmediates, WriteBarrierForVariableAccess);
        return;
    }

    case PutToBaseOperation::GlobalPropertyPut: {
        emitGetVirtualRegisters(base, regT0, value, regT1);
        loadPtr(&operation->m_structure, regT2);
        addSlowCase(branchPtr(NotEqual, Address(regT0, JSCell::structureOffset()), regT2));
        ASSERT(!operation->m_structure || !operation->m_structure->inlineCapacity());
        loadPtr(Address(regT0, JSObject::butterflyOffset()), regT2);
        load32(&operation->m_offsetInButterfly, regT3);
        signExtend32ToPtr(regT3, regT3);
        store64(regT1, BaseIndex(regT2, regT3, TimesEight));
        if (Heap::isWriteBarrierEnabled())
            emitWriteBarrier(regT0, regT1, regT2, regT3, ShouldFilterImmediates, WriteBarrierForVariableAccess);
        return;
    }

    case PutToBaseOperation::Uninitialised:
    case PutToBaseOperation::Readonly:
    case PutToBaseOperation::Generic:
        JITStubCall stubCall(this, cti_op_put_to_base);

        stubCall.addArgument(TrustedImm32(base));
        stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(id)));
        stubCall.addArgument(TrustedImm32(value));
        stubCall.addArgument(TrustedImmPtr(operation));
        stubCall.call();
        return;
    }
}

#endif // USE(JSVALUE64)

void JIT::emit_op_loop_hint(Instruction*)
{
    // Emit the JIT optimization check: 
    if (canBeOptimized())
        addSlowCase(branchAdd32(PositiveOrZero, TrustedImm32(Options::executionCounterIncrementForLoop()),
            AbsoluteAddress(m_codeBlock->addressOfJITExecuteCounter())));

    // Emit the watchdog timer check:
    if (m_vm->watchdog.isEnabled())
        addSlowCase(branchTest8(NonZero, AbsoluteAddress(m_vm->watchdog.timerDidFireAddress())));
}

void JIT::emitSlow_op_loop_hint(Instruction*, Vector<SlowCaseEntry>::iterator& iter)
{
#if ENABLE(DFG_JIT)
    // Emit the slow path for the JIT optimization check:
    if (canBeOptimized()) {
        linkSlowCase(iter);

        JITStubCall stubCall(this, cti_optimize);
        stubCall.addArgument(TrustedImm32(m_bytecodeOffset));
        stubCall.call();

        emitJumpSlowToHot(jump(), OPCODE_LENGTH(op_loop_hint));
    }
#endif

    // Emit the slow path of the watchdog timer check:
    if (m_vm->watchdog.isEnabled()) {
        linkSlowCase(iter);

        JITStubCall stubCall(this, cti_handle_watchdog_timer);
        stubCall.call();

        emitJumpSlowToHot(jump(), OPCODE_LENGTH(op_loop_hint));
    }

}

void JIT::emit_resolve_operations(ResolveOperations* resolveOperations, const int* baseVR, const int* valueVR)
{

#if USE(JSVALUE32_64)
    unmap();
#else
    killLastResultRegister();
#endif

    if (resolveOperations->isEmpty()) {
        addSlowCase(jump());
        return;
    }

    const RegisterID value = regT0;
#if USE(JSVALUE32_64)
    const RegisterID valueTag = regT1;
#endif
    const RegisterID scope = regT2;
    const RegisterID scratch = regT3;

    JSGlobalObject* globalObject = m_codeBlock->globalObject();
    ResolveOperation* pc = resolveOperations->data();
    emitGetFromCallFrameHeaderPtr(JSStack::ScopeChain, scope);
    bool setBase = false;
    bool resolvingBase = true;
    while (resolvingBase) {
        switch (pc->m_operation) {
        case ResolveOperation::ReturnGlobalObjectAsBase:
            move(TrustedImmPtr(globalObject), value);
#if USE(JSVALUE32_64)
            move(TrustedImm32(JSValue::CellTag), valueTag);
#endif
            emitValueProfilingSite();
            emitStoreCell(*baseVR, value);
            return;
        case ResolveOperation::SetBaseToGlobal:
            RELEASE_ASSERT(baseVR);
            setBase = true;
            move(TrustedImmPtr(globalObject), scratch);
            emitStoreCell(*baseVR, scratch);
            resolvingBase = false;
            ++pc;
            break;
        case ResolveOperation::SetBaseToUndefined: {
            RELEASE_ASSERT(baseVR);
            setBase = true;
#if USE(JSVALUE64)
            move(TrustedImm64(JSValue::encode(jsUndefined())), scratch);
            emitPutVirtualRegister(*baseVR, scratch);
#else
            emitStore(*baseVR, jsUndefined());
#endif
            resolvingBase = false;
            ++pc;
            break;
        }
        case ResolveOperation::SetBaseToScope:
            RELEASE_ASSERT(baseVR);
            setBase = true;
            emitStoreCell(*baseVR, scope);
            resolvingBase = false;
            ++pc;
            break;
        case ResolveOperation::ReturnScopeAsBase:
            emitStoreCell(*baseVR, scope);
            RELEASE_ASSERT(value == regT0);
            move(scope, value);
#if USE(JSVALUE32_64)
            move(TrustedImm32(JSValue::CellTag), valueTag);
#endif
            emitValueProfilingSite();
            return;
        case ResolveOperation::SkipTopScopeNode: {
#if USE(JSVALUE32_64)
            Jump activationNotCreated = branch32(Equal, tagFor(m_codeBlock->activationRegister()), TrustedImm32(JSValue::EmptyValueTag));
#else
            Jump activationNotCreated = branchTest64(Zero, addressFor(m_codeBlock->activationRegister()));
#endif
            loadPtr(Address(scope, JSScope::offsetOfNext()), scope);
            activationNotCreated.link(this);
            ++pc;
            break;
        }
        case ResolveOperation::CheckForDynamicEntriesBeforeGlobalScope: {
            move(scope, regT3);
            loadPtr(Address(regT3, JSScope::offsetOfNext()), regT1);
            Jump atTopOfScope = branchTestPtr(Zero, regT1);
            Label loopStart = label();
            loadPtr(Address(regT3, JSCell::structureOffset()), regT2);
            Jump isActivation = branchPtr(Equal, regT2, TrustedImmPtr(globalObject->activationStructure()));
            addSlowCase(branchPtr(NotEqual, regT2, TrustedImmPtr(globalObject->nameScopeStructure())));
            isActivation.link(this);
            move(regT1, regT3);
            loadPtr(Address(regT3, JSScope::offsetOfNext()), regT1);
            branchTestPtr(NonZero, regT1, loopStart);
            atTopOfScope.link(this);
            ++pc;
            break;
        }
        case ResolveOperation::SkipScopes: {
            for (int i = 0; i < pc->m_scopesToSkip; i++)
                loadPtr(Address(scope, JSScope::offsetOfNext()), scope);
            ++pc;
            break;
        }
        case ResolveOperation::Fail:
            addSlowCase(jump());
            return;
        default:
            resolvingBase = false;
        }
    }
    if (baseVR && !setBase)
        emitStoreCell(*baseVR, scope);

    RELEASE_ASSERT(valueVR);
    ResolveOperation* resolveValueOperation = pc;
    switch (resolveValueOperation->m_operation) {
    case ResolveOperation::GetAndReturnGlobalProperty: {
        // Verify structure.
        move(TrustedImmPtr(globalObject), regT2);
        move(TrustedImmPtr(resolveValueOperation), regT3);
        loadPtr(Address(regT3, OBJECT_OFFSETOF(ResolveOperation, m_structure)), regT1);
        addSlowCase(branchPtr(NotEqual, regT1, Address(regT2, JSCell::structureOffset())));

        // Load property.
        load32(Address(regT3, OBJECT_OFFSETOF(ResolveOperation, m_offset)), regT3);

        // regT2: GlobalObject
        // regT3: offset
#if USE(JSVALUE32_64)
        compileGetDirectOffset(regT2, valueTag, value, regT3, KnownNotFinal);
#else
        compileGetDirectOffset(regT2, value, regT3, regT1, KnownNotFinal);
#endif
        break;
    }
    case ResolveOperation::GetAndReturnGlobalVarWatchable:
    case ResolveOperation::GetAndReturnGlobalVar: {
#if USE(JSVALUE32_64)
        load32(reinterpret_cast<char*>(pc->m_registerAddress) + OBJECT_OFFSETOF(JSValue, u.asBits.tag), valueTag);
        load32(reinterpret_cast<char*>(pc->m_registerAddress) + OBJECT_OFFSETOF(JSValue, u.asBits.payload), value);
#else
        load64(reinterpret_cast<char*>(pc->m_registerAddress), value);
#endif
        break;
    }
    case ResolveOperation::GetAndReturnScopedVar: {
        loadPtr(Address(scope, JSVariableObject::offsetOfRegisters()), scope);
#if USE(JSVALUE32_64)
        load32(Address(scope, pc->m_offset * sizeof(Register) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), valueTag);
        load32(Address(scope, pc->m_offset * sizeof(Register) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), value);
#else
        load64(Address(scope, pc->m_offset * sizeof(Register)), value);
#endif
        break;
    }
    default:
        CRASH();
        return;
    }

#if USE(JSVALUE32_64)
    emitStore(*valueVR, valueTag, value);
#else
    emitPutVirtualRegister(*valueVR, value);
#endif
    emitValueProfilingSite();
}

void JIT::emitSlow_link_resolve_operations(ResolveOperations* resolveOperations, Vector<SlowCaseEntry>::iterator& iter)
{
    if (resolveOperations->isEmpty()) {
        linkSlowCase(iter);
        return;
    }

    ResolveOperation* pc = resolveOperations->data();
    bool resolvingBase = true;
    while (resolvingBase) {
        switch (pc->m_operation) {
        case ResolveOperation::ReturnGlobalObjectAsBase:
            return;
        case ResolveOperation::SetBaseToGlobal:
            resolvingBase = false;
            ++pc;
            break;
        case ResolveOperation::SetBaseToUndefined: {
            resolvingBase = false;
            ++pc;
            break;
        }
        case ResolveOperation::SetBaseToScope:
            resolvingBase = false;
            ++pc;
            break;
        case ResolveOperation::ReturnScopeAsBase:
            return;
        case ResolveOperation::SkipTopScopeNode: {
            ++pc;
            break;
        }
        case ResolveOperation::SkipScopes:
            ++pc;
            break;
        case ResolveOperation::Fail:
            linkSlowCase(iter);
            return;
        case ResolveOperation::CheckForDynamicEntriesBeforeGlobalScope: {
            linkSlowCase(iter);
            ++pc;
            break;
        }
        default:
            resolvingBase = false;
        }
    }
    ResolveOperation* resolveValueOperation = pc;
    switch (resolveValueOperation->m_operation) {
    case ResolveOperation::GetAndReturnGlobalProperty: {
        linkSlowCase(iter);
        break;
    }
    case ResolveOperation::GetAndReturnGlobalVarWatchable:
    case ResolveOperation::GetAndReturnGlobalVar:
        break;
    case ResolveOperation::GetAndReturnScopedVar:
        break;
    default:
        CRASH();
        return;
    }
}

void JIT::emit_op_resolve(Instruction* currentInstruction)
{
    ResolveOperations* operations = currentInstruction[3].u.resolveOperations;
    int dst = currentInstruction[1].u.operand;
    emit_resolve_operations(operations, 0, &dst);
}

void JIT::emitSlow_op_resolve(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    ResolveOperations* operations = currentInstruction[3].u.resolveOperations;
    emitSlow_link_resolve_operations(operations, iter);
    JITStubCall stubCall(this, cti_op_resolve);
    stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(currentInstruction[2].u.operand)));
    stubCall.addArgument(TrustedImmPtr(currentInstruction[3].u.resolveOperations));
    stubCall.callWithValueProfiling(currentInstruction[1].u.operand);
}

void JIT::emit_op_resolve_base(Instruction* currentInstruction)
{
    ResolveOperations* operations = currentInstruction[4].u.resolveOperations;
    int dst = currentInstruction[1].u.operand;
    emit_resolve_operations(operations, &dst, 0);
}

void JIT::emitSlow_op_resolve_base(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    ResolveOperations* operations = currentInstruction[4].u.resolveOperations;
    emitSlow_link_resolve_operations(operations, iter);
    JITStubCall stubCall(this, currentInstruction[3].u.operand ? cti_op_resolve_base_strict_put : cti_op_resolve_base);
    stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(currentInstruction[2].u.operand)));
    stubCall.addArgument(TrustedImmPtr(currentInstruction[4].u.resolveOperations));
    stubCall.addArgument(TrustedImmPtr(currentInstruction[5].u.putToBaseOperation));
    stubCall.callWithValueProfiling(currentInstruction[1].u.operand);
}

void JIT::emit_op_resolve_with_base(Instruction* currentInstruction)
{
    ResolveOperations* operations = currentInstruction[4].u.resolveOperations;
    int base = currentInstruction[1].u.operand;
    int value = currentInstruction[2].u.operand;
    emit_resolve_operations(operations, &base, &value);
}

void JIT::emitSlow_op_resolve_with_base(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    ResolveOperations* operations = currentInstruction[4].u.resolveOperations;
    emitSlow_link_resolve_operations(operations, iter);
    JITStubCall stubCall(this, cti_op_resolve_with_base);
    stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(currentInstruction[3].u.operand)));
    stubCall.addArgument(TrustedImm32(currentInstruction[1].u.operand));
    stubCall.addArgument(TrustedImmPtr(currentInstruction[4].u.resolveOperations));
    stubCall.addArgument(TrustedImmPtr(currentInstruction[5].u.putToBaseOperation));
    stubCall.callWithValueProfiling(currentInstruction[2].u.operand);
}

void JIT::emit_op_resolve_with_this(Instruction* currentInstruction)
{
    ResolveOperations* operations = currentInstruction[4].u.resolveOperations;
    int base = currentInstruction[1].u.operand;
    int value = currentInstruction[2].u.operand;
    emit_resolve_operations(operations, &base, &value);
}

void JIT::emitSlow_op_resolve_with_this(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    ResolveOperations* operations = currentInstruction[4].u.resolveOperations;
    emitSlow_link_resolve_operations(operations, iter);
    JITStubCall stubCall(this, cti_op_resolve_with_this);
    stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(currentInstruction[3].u.operand)));
    stubCall.addArgument(TrustedImm32(currentInstruction[1].u.operand));
    stubCall.addArgument(TrustedImmPtr(currentInstruction[4].u.resolveOperations));
    stubCall.callWithValueProfiling(currentInstruction[2].u.operand);
}

void JIT::emitSlow_op_put_to_base(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    int base = currentInstruction[1].u.operand;
    int id = currentInstruction[2].u.operand;
    int value = currentInstruction[3].u.operand;

    PutToBaseOperation* putToBaseOperation = currentInstruction[4].u.putToBaseOperation;
    switch (putToBaseOperation->m_kind) {
    case PutToBaseOperation::VariablePut:
        return;

    case PutToBaseOperation::GlobalVariablePutChecked:
        linkSlowCase(iter);
    case PutToBaseOperation::GlobalVariablePut:
        if (!putToBaseOperation->m_isDynamic)
            return;
        linkSlowCase(iter);
        break;

    case PutToBaseOperation::Uninitialised:
    case PutToBaseOperation::Readonly:
    case PutToBaseOperation::Generic:
        return;

    case PutToBaseOperation::GlobalPropertyPut:
        linkSlowCase(iter);
        break;

    }

    JITStubCall stubCall(this, cti_op_put_to_base);

    stubCall.addArgument(TrustedImm32(base));
    stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(id)));
    stubCall.addArgument(TrustedImm32(value));
    stubCall.addArgument(TrustedImmPtr(putToBaseOperation));
    stubCall.call();
}

void JIT::emit_op_new_regexp(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_new_regexp);
    stubCall.addArgument(TrustedImmPtr(m_codeBlock->regexp(currentInstruction[2].u.operand)));
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emit_op_new_func(Instruction* currentInstruction)
{
    Jump lazyJump;
    int dst = currentInstruction[1].u.operand;
    if (currentInstruction[3].u.operand) {
#if USE(JSVALUE32_64)
        lazyJump = branch32(NotEqual, tagFor(dst), TrustedImm32(JSValue::EmptyValueTag));
#else
        lazyJump = branchTest64(NonZero, addressFor(dst));
#endif
    }

    JITStubCall stubCall(this, cti_op_new_func);
    stubCall.addArgument(TrustedImmPtr(m_codeBlock->functionDecl(currentInstruction[2].u.operand)));
    stubCall.call(dst);

    if (currentInstruction[3].u.operand) {
#if USE(JSVALUE32_64)        
        unmap();
#else
        killLastResultRegister();
#endif
        lazyJump.link(this);
    }
}

void JIT::emit_op_new_func_exp(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_new_func_exp);
    stubCall.addArgument(TrustedImmPtr(m_codeBlock->functionExpr(currentInstruction[2].u.operand)));
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emit_op_new_array(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_new_array);
    stubCall.addArgument(TrustedImm32(currentInstruction[2].u.operand));
    stubCall.addArgument(TrustedImm32(currentInstruction[3].u.operand));
    stubCall.addArgument(TrustedImmPtr(currentInstruction[4].u.arrayAllocationProfile));
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emit_op_new_array_with_size(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_new_array_with_size);
#if USE(JSVALUE64)
    stubCall.addArgument(currentInstruction[2].u.operand, regT2);
#else
    stubCall.addArgument(currentInstruction[2].u.operand);
#endif
    stubCall.addArgument(TrustedImmPtr(currentInstruction[3].u.arrayAllocationProfile));
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emit_op_new_array_buffer(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_new_array_buffer);
    stubCall.addArgument(TrustedImm32(currentInstruction[2].u.operand));
    stubCall.addArgument(TrustedImm32(currentInstruction[3].u.operand));
    stubCall.addArgument(TrustedImmPtr(currentInstruction[4].u.arrayAllocationProfile));
    stubCall.call(currentInstruction[1].u.operand);
}

} // namespace JSC

#endif // ENABLE(JIT)
