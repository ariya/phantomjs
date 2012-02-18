/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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

#include "CodeBlock.h"
#include "GetterSetter.h"
#include "JITInlineMethods.h"
#include "JITStubCall.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "JSPropertyNameIterator.h"
#include "Interpreter.h"
#include "LinkBuffer.h"
#include "RepatchBuffer.h"
#include "ResultType.h"
#include "SamplingTool.h"

#ifndef NDEBUG
#include <stdio.h>
#endif

using namespace std;

namespace JSC {
#if USE(JSVALUE64)

JIT::CodePtr JIT::stringGetByValStubGenerator(JSGlobalData* globalData, ExecutablePool* pool)
{
    JSInterfaceJIT jit;
    JumpList failures;
    failures.append(jit.branchPtr(NotEqual, Address(regT0), TrustedImmPtr(globalData->jsStringVPtr)));
    failures.append(jit.branchTest32(NonZero, Address(regT0, OBJECT_OFFSETOF(JSString, m_fiberCount))));

    // Load string length to regT1, and start the process of loading the data pointer into regT0
    jit.load32(Address(regT0, ThunkHelpers::jsStringLengthOffset()), regT2);
    jit.loadPtr(Address(regT0, ThunkHelpers::jsStringValueOffset()), regT0);
    jit.loadPtr(Address(regT0, ThunkHelpers::stringImplDataOffset()), regT0);
    
    // Do an unsigned compare to simultaneously filter negative indices as well as indices that are too large
    failures.append(jit.branch32(AboveOrEqual, regT1, regT2));
    
    // Load the character
    jit.load16(BaseIndex(regT0, regT1, TimesTwo, 0), regT0);
    
    failures.append(jit.branch32(AboveOrEqual, regT0, TrustedImm32(0x100)));
    jit.move(TrustedImmPtr(globalData->smallStrings.singleCharacterStrings()), regT1);
    jit.loadPtr(BaseIndex(regT1, regT0, ScalePtr, 0), regT0);
    jit.ret();
    
    failures.link(&jit);
    jit.move(TrustedImm32(0), regT0);
    jit.ret();
    
    LinkBuffer patchBuffer(&jit, pool);
    return patchBuffer.finalizeCode().m_code;
}

void JIT::emit_op_get_by_val(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned base = currentInstruction[2].u.operand;
    unsigned property = currentInstruction[3].u.operand;

    emitGetVirtualRegisters(base, regT0, property, regT1);
    emitJumpSlowCaseIfNotImmediateInteger(regT1);

    // This is technically incorrect - we're zero-extending an int32.  On the hot path this doesn't matter.
    // We check the value as if it was a uint32 against the m_vectorLength - which will always fail if
    // number was signed since m_vectorLength is always less than intmax (since the total allocation
    // size is always less than 4Gb).  As such zero extending wil have been correct (and extending the value
    // to 64-bits is necessary since it's used in the address calculation.  We zero extend rather than sign
    // extending since it makes it easier to re-tag the value in the slow case.
    zeroExtend32ToPtr(regT1, regT1);

    emitJumpSlowCaseIfNotJSCell(regT0, base);
    addSlowCase(branchPtr(NotEqual, Address(regT0), TrustedImmPtr(m_globalData->jsArrayVPtr)));

    loadPtr(Address(regT0, JSArray::storageOffset()), regT2);
    addSlowCase(branch32(AboveOrEqual, regT1, Address(regT0, JSArray::vectorLengthOffset())));

    loadPtr(BaseIndex(regT2, regT1, ScalePtr, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])), regT0);
    addSlowCase(branchTestPtr(Zero, regT0));

    emitPutVirtualRegister(dst);
}

void JIT::emitSlow_op_get_by_val(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned base = currentInstruction[2].u.operand;
    unsigned property = currentInstruction[3].u.operand;
    
    linkSlowCase(iter); // property int32 check
    linkSlowCaseIfNotJSCell(iter, base); // base cell check
    Jump nonCell = jump();
    linkSlowCase(iter); // base array check
    Jump notString = branchPtr(NotEqual, Address(regT0), TrustedImmPtr(m_globalData->jsStringVPtr));
    emitNakedCall(m_globalData->getCTIStub(stringGetByValStubGenerator));
    Jump failed = branchTestPtr(Zero, regT0);
    emitPutVirtualRegister(dst, regT0);
    emitJumpSlowToHot(jump(), OPCODE_LENGTH(op_get_by_val));
    failed.link(this);
    notString.link(this);
    nonCell.link(this);
    
    linkSlowCase(iter); // vector length check
    linkSlowCase(iter); // empty value
    
    JITStubCall stubCall(this, cti_op_get_by_val);
    stubCall.addArgument(base, regT2);
    stubCall.addArgument(property, regT2);
    stubCall.call(dst);
}

void JIT::compileGetDirectOffset(RegisterID base, RegisterID result, RegisterID offset, RegisterID scratch)
{
    loadPtr(Address(base, OBJECT_OFFSETOF(JSObject, m_propertyStorage)), scratch);
    loadPtr(BaseIndex(scratch, offset, ScalePtr, 0), result);
}

void JIT::emit_op_get_by_pname(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned base = currentInstruction[2].u.operand;
    unsigned property = currentInstruction[3].u.operand;
    unsigned expected = currentInstruction[4].u.operand;
    unsigned iter = currentInstruction[5].u.operand;
    unsigned i = currentInstruction[6].u.operand;

    emitGetVirtualRegister(property, regT0);
    addSlowCase(branchPtr(NotEqual, regT0, addressFor(expected)));
    emitGetVirtualRegisters(base, regT0, iter, regT1);
    emitJumpSlowCaseIfNotJSCell(regT0, base);

    // Test base's structure
    loadPtr(Address(regT0, JSCell::structureOffset()), regT2);
    addSlowCase(branchPtr(NotEqual, regT2, Address(regT1, OBJECT_OFFSETOF(JSPropertyNameIterator, m_cachedStructure))));
    load32(addressFor(i), regT3);
    sub32(TrustedImm32(1), regT3);
    addSlowCase(branch32(AboveOrEqual, regT3, Address(regT1, OBJECT_OFFSETOF(JSPropertyNameIterator, m_numCacheableSlots))));
    compileGetDirectOffset(regT0, regT0, regT3, regT1);

    emitPutVirtualRegister(dst, regT0);
}

void JIT::emitSlow_op_get_by_pname(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned base = currentInstruction[2].u.operand;
    unsigned property = currentInstruction[3].u.operand;

    linkSlowCase(iter);
    linkSlowCaseIfNotJSCell(iter, base);
    linkSlowCase(iter);
    linkSlowCase(iter);

    JITStubCall stubCall(this, cti_op_get_by_val);
    stubCall.addArgument(base, regT2);
    stubCall.addArgument(property, regT2);
    stubCall.call(dst);
}

void JIT::emit_op_put_by_val(Instruction* currentInstruction)
{
    unsigned base = currentInstruction[1].u.operand;
    unsigned property = currentInstruction[2].u.operand;
    unsigned value = currentInstruction[3].u.operand;

    emitGetVirtualRegisters(base, regT0, property, regT1);
    emitJumpSlowCaseIfNotImmediateInteger(regT1);
    // See comment in op_get_by_val.
    zeroExtend32ToPtr(regT1, regT1);
    emitJumpSlowCaseIfNotJSCell(regT0, base);
    addSlowCase(branchPtr(NotEqual, Address(regT0), TrustedImmPtr(m_globalData->jsArrayVPtr)));
    addSlowCase(branch32(AboveOrEqual, regT1, Address(regT0, JSArray::vectorLengthOffset())));

    loadPtr(Address(regT0, JSArray::storageOffset()), regT2);
    Jump empty = branchTestPtr(Zero, BaseIndex(regT2, regT1, ScalePtr, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])));

    Label storeResult(this);
    emitGetVirtualRegister(value, regT0);
    storePtr(regT0, BaseIndex(regT2, regT1, ScalePtr, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])));
    Jump end = jump();
    
    empty.link(this);
    add32(TrustedImm32(1), Address(regT2, OBJECT_OFFSETOF(ArrayStorage, m_numValuesInVector)));
    branch32(Below, regT1, Address(regT2, OBJECT_OFFSETOF(ArrayStorage, m_length))).linkTo(storeResult, this);

    move(regT1, regT0);
    add32(TrustedImm32(1), regT0);
    store32(regT0, Address(regT2, OBJECT_OFFSETOF(ArrayStorage, m_length)));
    jump().linkTo(storeResult, this);

    end.link(this);
}

void JIT::emit_op_put_by_index(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_put_by_index);
    stubCall.addArgument(currentInstruction[1].u.operand, regT2);
    stubCall.addArgument(TrustedImm32(currentInstruction[2].u.operand));
    stubCall.addArgument(currentInstruction[3].u.operand, regT2);
    stubCall.call();
}

void JIT::emit_op_put_getter(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_put_getter);
    stubCall.addArgument(currentInstruction[1].u.operand, regT2);
    stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(currentInstruction[2].u.operand)));
    stubCall.addArgument(currentInstruction[3].u.operand, regT2);
    stubCall.call();
}

void JIT::emit_op_put_setter(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_put_setter);
    stubCall.addArgument(currentInstruction[1].u.operand, regT2);
    stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(currentInstruction[2].u.operand)));
    stubCall.addArgument(currentInstruction[3].u.operand, regT2);
    stubCall.call();
}

void JIT::emit_op_del_by_id(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_del_by_id);
    stubCall.addArgument(currentInstruction[2].u.operand, regT2);
    stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(currentInstruction[3].u.operand)));
    stubCall.call(currentInstruction[1].u.operand);
}


#if !ENABLE(JIT_OPTIMIZE_PROPERTY_ACCESS)

/* ------------------------------ BEGIN: !ENABLE(JIT_OPTIMIZE_PROPERTY_ACCESS) ------------------------------ */

// Treat these as nops - the call will be handed as a regular get_by_id/op_call pair.
void JIT::emit_op_method_check(Instruction*) {}
void JIT::emitSlow_op_method_check(Instruction*, Vector<SlowCaseEntry>::iterator&) { ASSERT_NOT_REACHED(); }
#if ENABLE(JIT_OPTIMIZE_METHOD_CALLS)
#error "JIT_OPTIMIZE_METHOD_CALLS requires JIT_OPTIMIZE_PROPERTY_ACCESS"
#endif

void JIT::emit_op_get_by_id(Instruction* currentInstruction)
{
    unsigned resultVReg = currentInstruction[1].u.operand;
    unsigned baseVReg = currentInstruction[2].u.operand;
    Identifier* ident = &(m_codeBlock->identifier(currentInstruction[3].u.operand));

    emitGetVirtualRegister(baseVReg, regT0);
    JITStubCall stubCall(this, cti_op_get_by_id_generic);
    stubCall.addArgument(regT0);
    stubCall.addArgument(TrustedImmPtr(ident));
    stubCall.call(resultVReg);

    m_propertyAccessInstructionIndex++;
}

void JIT::emitSlow_op_get_by_id(Instruction*, Vector<SlowCaseEntry>::iterator&)
{
    ASSERT_NOT_REACHED();
}

void JIT::emit_op_put_by_id(Instruction* currentInstruction)
{
    unsigned baseVReg = currentInstruction[1].u.operand;
    Identifier* ident = &(m_codeBlock->identifier(currentInstruction[2].u.operand));
    unsigned valueVReg = currentInstruction[3].u.operand;
    unsigned direct = currentInstruction[8].u.operand;

    emitGetVirtualRegisters(baseVReg, regT0, valueVReg, regT1);

    JITStubCall stubCall(this, direct ? cti_op_put_by_id_direct_generic, cti_op_put_by_id_generic);
    stubCall.addArgument(regT0);
    stubCall.addArgument(TrustedImmPtr(ident));
    stubCall.addArgument(regT1);
    stubCall.call();

    m_propertyAccessInstructionIndex++;
}

void JIT::emitSlow_op_put_by_id(Instruction*, Vector<SlowCaseEntry>::iterator&)
{
    ASSERT_NOT_REACHED();
}

#else // !ENABLE(JIT_OPTIMIZE_PROPERTY_ACCESS)

/* ------------------------------ BEGIN: ENABLE(JIT_OPTIMIZE_PROPERTY_ACCESS) ------------------------------ */

#if ENABLE(JIT_OPTIMIZE_METHOD_CALLS)

void JIT::emit_op_method_check(Instruction* currentInstruction)
{
    // Assert that the following instruction is a get_by_id.
    ASSERT(m_interpreter->getOpcodeID((currentInstruction + OPCODE_LENGTH(op_method_check))->u.opcode) == op_get_by_id);

    currentInstruction += OPCODE_LENGTH(op_method_check);
    unsigned resultVReg = currentInstruction[1].u.operand;
    unsigned baseVReg = currentInstruction[2].u.operand;
    Identifier* ident = &(m_codeBlock->identifier(currentInstruction[3].u.operand));

    emitGetVirtualRegister(baseVReg, regT0);

    // Do the method check - check the object & its prototype's structure inline (this is the common case).
    m_methodCallCompilationInfo.append(MethodCallCompilationInfo(m_propertyAccessInstructionIndex));
    MethodCallCompilationInfo& info = m_methodCallCompilationInfo.last();

    Jump notCell = emitJumpIfNotJSCell(regT0);

    BEGIN_UNINTERRUPTED_SEQUENCE(sequenceMethodCheck);

    Jump structureCheck = branchPtrWithPatch(NotEqual, Address(regT0, JSCell::structureOffset()), info.structureToCompare, TrustedImmPtr(reinterpret_cast<void*>(patchGetByIdDefaultStructure)));
    DataLabelPtr protoStructureToCompare, protoObj = moveWithPatch(TrustedImmPtr(0), regT1);
    Jump protoStructureCheck = branchPtrWithPatch(NotEqual, Address(regT1, JSCell::structureOffset()), protoStructureToCompare, TrustedImmPtr(reinterpret_cast<void*>(patchGetByIdDefaultStructure)));

    // This will be relinked to load the function without doing a load.
    DataLabelPtr putFunction = moveWithPatch(TrustedImmPtr(0), regT0);

    END_UNINTERRUPTED_SEQUENCE(sequenceMethodCheck);

    Jump match = jump();

    ASSERT_JIT_OFFSET_UNUSED(protoObj, differenceBetween(info.structureToCompare, protoObj), patchOffsetMethodCheckProtoObj);
    ASSERT_JIT_OFFSET(differenceBetween(info.structureToCompare, protoStructureToCompare), patchOffsetMethodCheckProtoStruct);
    ASSERT_JIT_OFFSET_UNUSED(putFunction, differenceBetween(info.structureToCompare, putFunction), patchOffsetMethodCheckPutFunction);

    // Link the failure cases here.
    notCell.link(this);
    structureCheck.link(this);
    protoStructureCheck.link(this);

    // Do a regular(ish) get_by_id (the slow case will be link to
    // cti_op_get_by_id_method_check instead of cti_op_get_by_id.
    compileGetByIdHotPath(resultVReg, baseVReg, ident, m_propertyAccessInstructionIndex++);

    match.link(this);
    emitPutVirtualRegister(resultVReg);

    // We've already generated the following get_by_id, so make sure it's skipped over.
    m_bytecodeOffset += OPCODE_LENGTH(op_get_by_id);
}

void JIT::emitSlow_op_method_check(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    currentInstruction += OPCODE_LENGTH(op_method_check);
    unsigned resultVReg = currentInstruction[1].u.operand;
    unsigned baseVReg = currentInstruction[2].u.operand;
    Identifier* ident = &(m_codeBlock->identifier(currentInstruction[3].u.operand));

    compileGetByIdSlowCase(resultVReg, baseVReg, ident, iter, true);

    // We've already generated the following get_by_id, so make sure it's skipped over.
    m_bytecodeOffset += OPCODE_LENGTH(op_get_by_id);
}

#else //!ENABLE(JIT_OPTIMIZE_METHOD_CALLS)

// Treat these as nops - the call will be handed as a regular get_by_id/op_call pair.
void JIT::emit_op_method_check(Instruction*) {}
void JIT::emitSlow_op_method_check(Instruction*, Vector<SlowCaseEntry>::iterator&) { ASSERT_NOT_REACHED(); }

#endif

void JIT::emit_op_get_by_id(Instruction* currentInstruction)
{
    unsigned resultVReg = currentInstruction[1].u.operand;
    unsigned baseVReg = currentInstruction[2].u.operand;
    Identifier* ident = &(m_codeBlock->identifier(currentInstruction[3].u.operand));

    emitGetVirtualRegister(baseVReg, regT0);
    compileGetByIdHotPath(resultVReg, baseVReg, ident, m_propertyAccessInstructionIndex++);
    emitPutVirtualRegister(resultVReg);
}

void JIT::compileGetByIdHotPath(int, int baseVReg, Identifier*, unsigned propertyAccessInstructionIndex)
{
    // As for put_by_id, get_by_id requires the offset of the Structure and the offset of the access to be patched.
    // Additionally, for get_by_id we need patch the offset of the branch to the slow case (we patch this to jump
    // to array-length / prototype access tranpolines, and finally we also the the property-map access offset as a label
    // to jump back to if one of these trampolies finds a match.

    emitJumpSlowCaseIfNotJSCell(regT0, baseVReg);

    BEGIN_UNINTERRUPTED_SEQUENCE(sequenceGetByIdHotPath);

    Label hotPathBegin(this);
    m_propertyAccessCompilationInfo[propertyAccessInstructionIndex].hotPathBegin = hotPathBegin;

    DataLabelPtr structureToCompare;
    Jump structureCheck = branchPtrWithPatch(NotEqual, Address(regT0, JSCell::structureOffset()), structureToCompare, TrustedImmPtr(reinterpret_cast<void*>(patchGetByIdDefaultStructure)));
    addSlowCase(structureCheck);
    ASSERT_JIT_OFFSET(differenceBetween(hotPathBegin, structureToCompare), patchOffsetGetByIdStructure);
    ASSERT_JIT_OFFSET(differenceBetween(hotPathBegin, structureCheck), patchOffsetGetByIdBranchToSlowCase)

    loadPtr(Address(regT0, OBJECT_OFFSETOF(JSObject, m_propertyStorage)), regT0);
    DataLabel32 displacementLabel = loadPtrWithAddressOffsetPatch(Address(regT0, patchGetByIdDefaultOffset), regT0);
    ASSERT_JIT_OFFSET_UNUSED(displacementLabel, differenceBetween(hotPathBegin, displacementLabel), patchOffsetGetByIdPropertyMapOffset);

    Label putResult(this);

    END_UNINTERRUPTED_SEQUENCE(sequenceGetByIdHotPath);

    ASSERT_JIT_OFFSET(differenceBetween(hotPathBegin, putResult), patchOffsetGetByIdPutResult);
}

void JIT::emitSlow_op_get_by_id(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned resultVReg = currentInstruction[1].u.operand;
    unsigned baseVReg = currentInstruction[2].u.operand;
    Identifier* ident = &(m_codeBlock->identifier(currentInstruction[3].u.operand));

    compileGetByIdSlowCase(resultVReg, baseVReg, ident, iter, false);
}

void JIT::compileGetByIdSlowCase(int resultVReg, int baseVReg, Identifier* ident, Vector<SlowCaseEntry>::iterator& iter, bool isMethodCheck)
{
    // As for the hot path of get_by_id, above, we ensure that we can use an architecture specific offset
    // so that we only need track one pointer into the slow case code - we track a pointer to the location
    // of the call (which we can use to look up the patch information), but should a array-length or
    // prototype access trampoline fail we want to bail out back to here.  To do so we can subtract back
    // the distance from the call to the head of the slow case.

    linkSlowCaseIfNotJSCell(iter, baseVReg);
    linkSlowCase(iter);

    BEGIN_UNINTERRUPTED_SEQUENCE(sequenceGetByIdSlowCase);

#ifndef NDEBUG
    Label coldPathBegin(this);
#endif
    JITStubCall stubCall(this, isMethodCheck ? cti_op_get_by_id_method_check : cti_op_get_by_id);
    stubCall.addArgument(regT0);
    stubCall.addArgument(TrustedImmPtr(ident));
    Call call = stubCall.call(resultVReg);

    END_UNINTERRUPTED_SEQUENCE(sequenceGetByIdSlowCase);

    ASSERT_JIT_OFFSET(differenceBetween(coldPathBegin, call), patchOffsetGetByIdSlowCaseCall);

    // Track the location of the call; this will be used to recover patch information.
    m_propertyAccessCompilationInfo[m_propertyAccessInstructionIndex].callReturnLocation = call;
    m_propertyAccessInstructionIndex++;
}

void JIT::emit_op_put_by_id(Instruction* currentInstruction)
{
    unsigned baseVReg = currentInstruction[1].u.operand;
    unsigned valueVReg = currentInstruction[3].u.operand;

    unsigned propertyAccessInstructionIndex = m_propertyAccessInstructionIndex++;

    // In order to be able to patch both the Structure, and the object offset, we store one pointer,
    // to just after the arguments have been loaded into registers 'hotPathBegin', and we generate code
    // such that the Structure & offset are always at the same distance from this.

    emitGetVirtualRegisters(baseVReg, regT0, valueVReg, regT1);

    // Jump to a slow case if either the base object is an immediate, or if the Structure does not match.
    emitJumpSlowCaseIfNotJSCell(regT0, baseVReg);

    BEGIN_UNINTERRUPTED_SEQUENCE(sequencePutById);

    Label hotPathBegin(this);
    m_propertyAccessCompilationInfo[propertyAccessInstructionIndex].hotPathBegin = hotPathBegin;

    // It is important that the following instruction plants a 32bit immediate, in order that it can be patched over.
    DataLabelPtr structureToCompare;
    addSlowCase(branchPtrWithPatch(NotEqual, Address(regT0, JSCell::structureOffset()), structureToCompare, TrustedImmPtr(reinterpret_cast<void*>(patchGetByIdDefaultStructure))));
    ASSERT_JIT_OFFSET(differenceBetween(hotPathBegin, structureToCompare), patchOffsetPutByIdStructure);

    loadPtr(Address(regT0, OBJECT_OFFSETOF(JSObject, m_propertyStorage)), regT0);
    DataLabel32 displacementLabel = storePtrWithAddressOffsetPatch(regT1, Address(regT0, patchGetByIdDefaultOffset));

    END_UNINTERRUPTED_SEQUENCE(sequencePutById);

    ASSERT_JIT_OFFSET_UNUSED(displacementLabel, differenceBetween(hotPathBegin, displacementLabel), patchOffsetPutByIdPropertyMapOffset);
}

void JIT::emitSlow_op_put_by_id(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned baseVReg = currentInstruction[1].u.operand;
    Identifier* ident = &(m_codeBlock->identifier(currentInstruction[2].u.operand));
    unsigned direct = currentInstruction[8].u.operand;

    unsigned propertyAccessInstructionIndex = m_propertyAccessInstructionIndex++;

    linkSlowCaseIfNotJSCell(iter, baseVReg);
    linkSlowCase(iter);

    JITStubCall stubCall(this, direct ? cti_op_put_by_id_direct : cti_op_put_by_id);
    stubCall.addArgument(regT0);
    stubCall.addArgument(TrustedImmPtr(ident));
    stubCall.addArgument(regT1);
    Call call = stubCall.call();

    // Track the location of the call; this will be used to recover patch information.
    m_propertyAccessCompilationInfo[propertyAccessInstructionIndex].callReturnLocation = call;
}

// Compile a store into an object's property storage.  May overwrite the
// value in objectReg.
void JIT::compilePutDirectOffset(RegisterID base, RegisterID value, Structure* structure, size_t cachedOffset)
{
    int offset = cachedOffset * sizeof(JSValue);
    if (structure->isUsingInlineStorage())
        offset += JSObject::offsetOfInlineStorage();
    else
        loadPtr(Address(base, OBJECT_OFFSETOF(JSObject, m_propertyStorage)), base);
    storePtr(value, Address(base, offset));
}

// Compile a load from an object's property storage.  May overwrite base.
void JIT::compileGetDirectOffset(RegisterID base, RegisterID result, Structure* structure, size_t cachedOffset)
{
    int offset = cachedOffset * sizeof(JSValue);
    if (structure->isUsingInlineStorage()) {
        offset += JSObject::offsetOfInlineStorage();
        loadPtr(Address(base, offset), result);
    } else {
        loadPtr(Address(base, OBJECT_OFFSETOF(JSObject, m_propertyStorage)), result);
        loadPtr(Address(result, offset), result);
    }
}

void JIT::compileGetDirectOffset(JSObject* base, RegisterID result, size_t cachedOffset)
{
    loadPtr(static_cast<void*>(&base->m_propertyStorage[cachedOffset]), result);
}

void JIT::privateCompilePutByIdTransition(StructureStubInfo* stubInfo, Structure* oldStructure, Structure* newStructure, size_t cachedOffset, StructureChain* chain, ReturnAddressPtr returnAddress, bool direct)
{
    JumpList failureCases;
    // Check eax is an object of the right Structure.
    failureCases.append(emitJumpIfNotJSCell(regT0));
    failureCases.append(branchPtr(NotEqual, Address(regT0, JSCell::structureOffset()), TrustedImmPtr(oldStructure)));
    testPrototype(oldStructure->storedPrototype(), failureCases);

    // ecx = baseObject->m_structure
    if (!direct) {
        for (WriteBarrier<Structure>* it = chain->head(); *it; ++it)
            testPrototype((*it)->storedPrototype(), failureCases);
    }

    Call callTarget;

    // emit a call only if storage realloc is needed
    bool willNeedStorageRealloc = oldStructure->propertyStorageCapacity() != newStructure->propertyStorageCapacity();
    if (willNeedStorageRealloc) {
        // This trampoline was called to like a JIT stub; before we can can call again we need to
        // remove the return address from the stack, to prevent the stack from becoming misaligned.
        preserveReturnAddressAfterCall(regT3);
 
        JITStubCall stubCall(this, cti_op_put_by_id_transition_realloc);
        stubCall.skipArgument(); // base
        stubCall.skipArgument(); // ident
        stubCall.skipArgument(); // value
        stubCall.addArgument(TrustedImm32(oldStructure->propertyStorageCapacity()));
        stubCall.addArgument(TrustedImm32(newStructure->propertyStorageCapacity()));
        stubCall.call(regT0);
        emitGetJITStubArg(2, regT1);

        restoreReturnAddressBeforeReturn(regT3);
    }

    storePtrWithWriteBarrier(TrustedImmPtr(newStructure), regT0, Address(regT0, JSCell::structureOffset()));

    // write the value
    compilePutDirectOffset(regT0, regT1, newStructure, cachedOffset);

    ret();
    
    ASSERT(!failureCases.empty());
    failureCases.link(this);
    restoreArgumentReferenceForTrampoline();
    Call failureCall = tailRecursiveCall();

    LinkBuffer patchBuffer(this, m_codeBlock->executablePool());

    patchBuffer.link(failureCall, FunctionPtr(direct ? cti_op_put_by_id_direct_fail : cti_op_put_by_id_fail));

    if (willNeedStorageRealloc) {
        ASSERT(m_calls.size() == 1);
        patchBuffer.link(m_calls[0].from, FunctionPtr(cti_op_put_by_id_transition_realloc));
    }
    
    CodeLocationLabel entryLabel = patchBuffer.finalizeCodeAddendum();
    stubInfo->stubRoutine = entryLabel;
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relinkCallerToTrampoline(returnAddress, entryLabel);
}

void JIT::patchGetByIdSelf(CodeBlock* codeBlock, StructureStubInfo* stubInfo, Structure* structure, size_t cachedOffset, ReturnAddressPtr returnAddress)
{
    RepatchBuffer repatchBuffer(codeBlock);

    // We don't want to patch more than once - in future go to cti_op_get_by_id_generic.
    // Should probably go to cti_op_get_by_id_fail, but that doesn't do anything interesting right now.
    repatchBuffer.relinkCallerToFunction(returnAddress, FunctionPtr(cti_op_get_by_id_self_fail));

    int offset = sizeof(JSValue) * cachedOffset;

    // Patch the offset into the propoerty map to load from, then patch the Structure to look for.
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelPtrAtOffset(patchOffsetGetByIdStructure), structure);
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabel32AtOffset(patchOffsetGetByIdPropertyMapOffset), offset);
}

void JIT::patchMethodCallProto(JSGlobalData& globalData, CodeBlock* codeBlock, MethodCallLinkInfo& methodCallLinkInfo, JSFunction* callee, Structure* structure, JSObject* proto, ReturnAddressPtr returnAddress)
{
    RepatchBuffer repatchBuffer(codeBlock);

    ASSERT(!methodCallLinkInfo.cachedStructure);
    methodCallLinkInfo.cachedStructure.set(globalData, codeBlock->ownerExecutable(), structure);

    Structure* prototypeStructure = proto->structure();
    methodCallLinkInfo.cachedPrototypeStructure.set(globalData, codeBlock->ownerExecutable(), prototypeStructure);

    repatchBuffer.repatch(methodCallLinkInfo.structureLabel, structure);
    repatchBuffer.repatch(methodCallLinkInfo.structureLabel.dataLabelPtrAtOffset(patchOffsetMethodCheckProtoObj), proto);
    repatchBuffer.repatch(methodCallLinkInfo.structureLabel.dataLabelPtrAtOffset(patchOffsetMethodCheckProtoStruct), prototypeStructure);
    repatchBuffer.repatch(methodCallLinkInfo.structureLabel.dataLabelPtrAtOffset(patchOffsetMethodCheckPutFunction), callee);

    repatchBuffer.relinkCallerToFunction(returnAddress, FunctionPtr(cti_op_get_by_id));
}

void JIT::patchPutByIdReplace(CodeBlock* codeBlock, StructureStubInfo* stubInfo, Structure* structure, size_t cachedOffset, ReturnAddressPtr returnAddress, bool direct)
{
    RepatchBuffer repatchBuffer(codeBlock);

    // We don't want to patch more than once - in future go to cti_op_put_by_id_generic.
    // Should probably go to cti_op_put_by_id_fail, but that doesn't do anything interesting right now.
    repatchBuffer.relinkCallerToFunction(returnAddress, FunctionPtr(direct ? cti_op_put_by_id_direct_generic : cti_op_put_by_id_generic));

    int offset = sizeof(JSValue) * cachedOffset;

    // Patch the offset into the propoerty map to load from, then patch the Structure to look for.
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelPtrAtOffset(patchOffsetPutByIdStructure), structure);
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabel32AtOffset(patchOffsetPutByIdPropertyMapOffset), offset);
}

void JIT::privateCompilePatchGetArrayLength(ReturnAddressPtr returnAddress)
{
    StructureStubInfo* stubInfo = &m_codeBlock->getStubInfo(returnAddress);

    // Check eax is an array
    Jump failureCases1 = branchPtr(NotEqual, Address(regT0), TrustedImmPtr(m_globalData->jsArrayVPtr));

    // Checks out okay! - get the length from the storage
    loadPtr(Address(regT0, JSArray::storageOffset()), regT3);
    load32(Address(regT3, OBJECT_OFFSETOF(ArrayStorage, m_length)), regT2);
    Jump failureCases2 = branch32(LessThan, regT2, TrustedImm32(0));

    emitFastArithIntToImmNoCheck(regT2, regT0);
    Jump success = jump();

    LinkBuffer patchBuffer(this, m_codeBlock->executablePool());

    // Use the patch information to link the failure cases back to the original slow case routine.
    CodeLocationLabel slowCaseBegin = stubInfo->callReturnLocation.labelAtOffset(-patchOffsetGetByIdSlowCaseCall);
    patchBuffer.link(failureCases1, slowCaseBegin);
    patchBuffer.link(failureCases2, slowCaseBegin);

    // On success return back to the hot patch code, at a point it will perform the store to dest for us.
    patchBuffer.link(success, stubInfo->hotPathBegin.labelAtOffset(patchOffsetGetByIdPutResult));

    // Track the stub we have created so that it will be deleted later.
    CodeLocationLabel entryLabel = patchBuffer.finalizeCodeAddendum();
    stubInfo->stubRoutine = entryLabel;

    // Finally patch the jump to slow case back in the hot path to jump here instead.
    CodeLocationJump jumpLocation = stubInfo->hotPathBegin.jumpAtOffset(patchOffsetGetByIdBranchToSlowCase);
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(jumpLocation, entryLabel);

    // We don't want to patch more than once - in future go to cti_op_put_by_id_generic.
    repatchBuffer.relinkCallerToFunction(returnAddress, FunctionPtr(cti_op_get_by_id_array_fail));
}

void JIT::privateCompileGetByIdProto(StructureStubInfo* stubInfo, Structure* structure, Structure* prototypeStructure, const Identifier& ident, const PropertySlot& slot, size_t cachedOffset, ReturnAddressPtr returnAddress, CallFrame* callFrame)
{
    // The prototype object definitely exists (if this stub exists the CodeBlock is referencing a Structure that is
    // referencing the prototype object - let's speculatively load it's table nice and early!)
    JSObject* protoObject = asObject(structure->prototypeForLookup(callFrame));

    // Check eax is an object of the right Structure.
    Jump failureCases1 = checkStructure(regT0, structure);

    // Check the prototype object's Structure had not changed.
    move(TrustedImmPtr(protoObject), regT3);
    Jump failureCases2 = branchPtr(NotEqual, Address(regT3, JSCell::structureOffset()), TrustedImmPtr(prototypeStructure));

    bool needsStubLink = false;
    
    // Checks out okay!
    if (slot.cachedPropertyType() == PropertySlot::Getter) {
        needsStubLink = true;
        compileGetDirectOffset(protoObject, regT1, cachedOffset);
        JITStubCall stubCall(this, cti_op_get_by_id_getter_stub);
        stubCall.addArgument(regT1);
        stubCall.addArgument(regT0);
        stubCall.addArgument(TrustedImmPtr(stubInfo->callReturnLocation.executableAddress()));
        stubCall.call();
    } else if (slot.cachedPropertyType() == PropertySlot::Custom) {
        needsStubLink = true;
        JITStubCall stubCall(this, cti_op_get_by_id_custom_stub);
        stubCall.addArgument(TrustedImmPtr(protoObject));
        stubCall.addArgument(TrustedImmPtr(FunctionPtr(slot.customGetter()).executableAddress()));
        stubCall.addArgument(TrustedImmPtr(const_cast<Identifier*>(&ident)));
        stubCall.addArgument(TrustedImmPtr(stubInfo->callReturnLocation.executableAddress()));
        stubCall.call();
    } else
        compileGetDirectOffset(protoObject, regT0, cachedOffset);
    Jump success = jump();
    LinkBuffer patchBuffer(this, m_codeBlock->executablePool());

    // Use the patch information to link the failure cases back to the original slow case routine.
    CodeLocationLabel slowCaseBegin = stubInfo->callReturnLocation.labelAtOffset(-patchOffsetGetByIdSlowCaseCall);
    patchBuffer.link(failureCases1, slowCaseBegin);
    patchBuffer.link(failureCases2, slowCaseBegin);

    // On success return back to the hot patch code, at a point it will perform the store to dest for us.
    patchBuffer.link(success, stubInfo->hotPathBegin.labelAtOffset(patchOffsetGetByIdPutResult));

    if (needsStubLink) {
        for (Vector<CallRecord>::iterator iter = m_calls.begin(); iter != m_calls.end(); ++iter) {
            if (iter->to)
                patchBuffer.link(iter->from, FunctionPtr(iter->to));
        }
    }
    // Track the stub we have created so that it will be deleted later.
    CodeLocationLabel entryLabel = patchBuffer.finalizeCodeAddendum();
    stubInfo->stubRoutine = entryLabel;

    // Finally patch the jump to slow case back in the hot path to jump here instead.
    CodeLocationJump jumpLocation = stubInfo->hotPathBegin.jumpAtOffset(patchOffsetGetByIdBranchToSlowCase);
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(jumpLocation, entryLabel);

    // We don't want to patch more than once - in future go to cti_op_put_by_id_generic.
    repatchBuffer.relinkCallerToFunction(returnAddress, FunctionPtr(cti_op_get_by_id_proto_list));
}

void JIT::privateCompileGetByIdSelfList(StructureStubInfo* stubInfo, PolymorphicAccessStructureList* polymorphicStructures, int currentIndex, Structure* structure, const Identifier& ident, const PropertySlot& slot, size_t cachedOffset)
{
    Jump failureCase = checkStructure(regT0, structure);
    bool needsStubLink = false;
    if (slot.cachedPropertyType() == PropertySlot::Getter) {
        needsStubLink = true;
        compileGetDirectOffset(regT0, regT1, structure, cachedOffset);
        JITStubCall stubCall(this, cti_op_get_by_id_getter_stub);
        stubCall.addArgument(regT1);
        stubCall.addArgument(regT0);
        stubCall.addArgument(TrustedImmPtr(stubInfo->callReturnLocation.executableAddress()));
        stubCall.call();
    } else if (slot.cachedPropertyType() == PropertySlot::Custom) {
        needsStubLink = true;
        JITStubCall stubCall(this, cti_op_get_by_id_custom_stub);
        stubCall.addArgument(regT0);
        stubCall.addArgument(TrustedImmPtr(FunctionPtr(slot.customGetter()).executableAddress()));
        stubCall.addArgument(TrustedImmPtr(const_cast<Identifier*>(&ident)));
        stubCall.addArgument(TrustedImmPtr(stubInfo->callReturnLocation.executableAddress()));
        stubCall.call();
    } else
        compileGetDirectOffset(regT0, regT0, structure, cachedOffset);
    Jump success = jump();

    LinkBuffer patchBuffer(this, m_codeBlock->executablePool());

    if (needsStubLink) {
        for (Vector<CallRecord>::iterator iter = m_calls.begin(); iter != m_calls.end(); ++iter) {
            if (iter->to)
                patchBuffer.link(iter->from, FunctionPtr(iter->to));
        }
    }

    // Use the patch information to link the failure cases back to the original slow case routine.
    CodeLocationLabel lastProtoBegin = polymorphicStructures->list[currentIndex - 1].stubRoutine;
    if (!lastProtoBegin)
        lastProtoBegin = stubInfo->callReturnLocation.labelAtOffset(-patchOffsetGetByIdSlowCaseCall);

    patchBuffer.link(failureCase, lastProtoBegin);

    // On success return back to the hot patch code, at a point it will perform the store to dest for us.
    patchBuffer.link(success, stubInfo->hotPathBegin.labelAtOffset(patchOffsetGetByIdPutResult));

    CodeLocationLabel entryLabel = patchBuffer.finalizeCodeAddendum();

    polymorphicStructures->list[currentIndex].set(*m_globalData, m_codeBlock->ownerExecutable(), entryLabel, structure);

    // Finally patch the jump to slow case back in the hot path to jump here instead.
    CodeLocationJump jumpLocation = stubInfo->hotPathBegin.jumpAtOffset(patchOffsetGetByIdBranchToSlowCase);
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(jumpLocation, entryLabel);
}

void JIT::privateCompileGetByIdProtoList(StructureStubInfo* stubInfo, PolymorphicAccessStructureList* prototypeStructures, int currentIndex, Structure* structure, Structure* prototypeStructure, const Identifier& ident, const PropertySlot& slot, size_t cachedOffset, CallFrame* callFrame)
{
    // The prototype object definitely exists (if this stub exists the CodeBlock is referencing a Structure that is
    // referencing the prototype object - let's speculatively load it's table nice and early!)
    JSObject* protoObject = asObject(structure->prototypeForLookup(callFrame));

    // Check eax is an object of the right Structure.
    Jump failureCases1 = checkStructure(regT0, structure);

    // Check the prototype object's Structure had not changed.
    move(TrustedImmPtr(protoObject), regT3);
    Jump failureCases2 = branchPtr(NotEqual, Address(regT3, JSCell::structureOffset()), TrustedImmPtr(prototypeStructure));

    // Checks out okay!
    bool needsStubLink = false;
    if (slot.cachedPropertyType() == PropertySlot::Getter) {
        needsStubLink = true;
        compileGetDirectOffset(protoObject, regT1, cachedOffset);
        JITStubCall stubCall(this, cti_op_get_by_id_getter_stub);
        stubCall.addArgument(regT1);
        stubCall.addArgument(regT0);
        stubCall.addArgument(TrustedImmPtr(stubInfo->callReturnLocation.executableAddress()));
        stubCall.call();
    } else if (slot.cachedPropertyType() == PropertySlot::Custom) {
        needsStubLink = true;
        JITStubCall stubCall(this, cti_op_get_by_id_custom_stub);
        stubCall.addArgument(TrustedImmPtr(protoObject));
        stubCall.addArgument(TrustedImmPtr(FunctionPtr(slot.customGetter()).executableAddress()));
        stubCall.addArgument(TrustedImmPtr(const_cast<Identifier*>(&ident)));
        stubCall.addArgument(TrustedImmPtr(stubInfo->callReturnLocation.executableAddress()));
        stubCall.call();
    } else
        compileGetDirectOffset(protoObject, regT0, cachedOffset);

    Jump success = jump();

    LinkBuffer patchBuffer(this, m_codeBlock->executablePool());

    if (needsStubLink) {
        for (Vector<CallRecord>::iterator iter = m_calls.begin(); iter != m_calls.end(); ++iter) {
            if (iter->to)
                patchBuffer.link(iter->from, FunctionPtr(iter->to));
        }
    }

    // Use the patch information to link the failure cases back to the original slow case routine.
    CodeLocationLabel lastProtoBegin = prototypeStructures->list[currentIndex - 1].stubRoutine;
    patchBuffer.link(failureCases1, lastProtoBegin);
    patchBuffer.link(failureCases2, lastProtoBegin);

    // On success return back to the hot patch code, at a point it will perform the store to dest for us.
    patchBuffer.link(success, stubInfo->hotPathBegin.labelAtOffset(patchOffsetGetByIdPutResult));

    CodeLocationLabel entryLabel = patchBuffer.finalizeCodeAddendum();
    prototypeStructures->list[currentIndex].set(*m_globalData, m_codeBlock->ownerExecutable(), entryLabel, structure, prototypeStructure);

    // Finally patch the jump to slow case back in the hot path to jump here instead.
    CodeLocationJump jumpLocation = stubInfo->hotPathBegin.jumpAtOffset(patchOffsetGetByIdBranchToSlowCase);
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(jumpLocation, entryLabel);
}

void JIT::privateCompileGetByIdChainList(StructureStubInfo* stubInfo, PolymorphicAccessStructureList* prototypeStructures, int currentIndex, Structure* structure, StructureChain* chain, size_t count, const Identifier& ident, const PropertySlot& slot, size_t cachedOffset, CallFrame* callFrame)
{
    ASSERT(count);
    JumpList bucketsOfFail;

    // Check eax is an object of the right Structure.
    Jump baseObjectCheck = checkStructure(regT0, structure);
    bucketsOfFail.append(baseObjectCheck);

    Structure* currStructure = structure;
    WriteBarrier<Structure>* it = chain->head();
    JSObject* protoObject = 0;
    for (unsigned i = 0; i < count; ++i, ++it) {
        protoObject = asObject(currStructure->prototypeForLookup(callFrame));
        currStructure = it->get();
        testPrototype(protoObject, bucketsOfFail);
    }
    ASSERT(protoObject);
    
    bool needsStubLink = false;
    if (slot.cachedPropertyType() == PropertySlot::Getter) {
        needsStubLink = true;
        compileGetDirectOffset(protoObject, regT1, cachedOffset);
        JITStubCall stubCall(this, cti_op_get_by_id_getter_stub);
        stubCall.addArgument(regT1);
        stubCall.addArgument(regT0);
        stubCall.addArgument(TrustedImmPtr(stubInfo->callReturnLocation.executableAddress()));
        stubCall.call();
    } else if (slot.cachedPropertyType() == PropertySlot::Custom) {
        needsStubLink = true;
        JITStubCall stubCall(this, cti_op_get_by_id_custom_stub);
        stubCall.addArgument(TrustedImmPtr(protoObject));
        stubCall.addArgument(TrustedImmPtr(FunctionPtr(slot.customGetter()).executableAddress()));
        stubCall.addArgument(TrustedImmPtr(const_cast<Identifier*>(&ident)));
        stubCall.addArgument(TrustedImmPtr(stubInfo->callReturnLocation.executableAddress()));
        stubCall.call();
    } else
        compileGetDirectOffset(protoObject, regT0, cachedOffset);
    Jump success = jump();

    LinkBuffer patchBuffer(this, m_codeBlock->executablePool());
    
    if (needsStubLink) {
        for (Vector<CallRecord>::iterator iter = m_calls.begin(); iter != m_calls.end(); ++iter) {
            if (iter->to)
                patchBuffer.link(iter->from, FunctionPtr(iter->to));
        }
    }

    // Use the patch information to link the failure cases back to the original slow case routine.
    CodeLocationLabel lastProtoBegin = prototypeStructures->list[currentIndex - 1].stubRoutine;

    patchBuffer.link(bucketsOfFail, lastProtoBegin);

    // On success return back to the hot patch code, at a point it will perform the store to dest for us.
    patchBuffer.link(success, stubInfo->hotPathBegin.labelAtOffset(patchOffsetGetByIdPutResult));

    CodeLocationLabel entryLabel = patchBuffer.finalizeCodeAddendum();

    // Track the stub we have created so that it will be deleted later.
    prototypeStructures->list[currentIndex].set(callFrame->globalData(), m_codeBlock->ownerExecutable(), entryLabel, structure, chain);

    // Finally patch the jump to slow case back in the hot path to jump here instead.
    CodeLocationJump jumpLocation = stubInfo->hotPathBegin.jumpAtOffset(patchOffsetGetByIdBranchToSlowCase);
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(jumpLocation, entryLabel);
}

void JIT::privateCompileGetByIdChain(StructureStubInfo* stubInfo, Structure* structure, StructureChain* chain, size_t count, const Identifier& ident, const PropertySlot& slot, size_t cachedOffset, ReturnAddressPtr returnAddress, CallFrame* callFrame)
{
    ASSERT(count);

    JumpList bucketsOfFail;

    // Check eax is an object of the right Structure.
    bucketsOfFail.append(checkStructure(regT0, structure));

    Structure* currStructure = structure;
    WriteBarrier<Structure>* it = chain->head();
    JSObject* protoObject = 0;
    for (unsigned i = 0; i < count; ++i, ++it) {
        protoObject = asObject(currStructure->prototypeForLookup(callFrame));
        currStructure = it->get();
        testPrototype(protoObject, bucketsOfFail);
    }
    ASSERT(protoObject);

    bool needsStubLink = false;
    if (slot.cachedPropertyType() == PropertySlot::Getter) {
        needsStubLink = true;
        compileGetDirectOffset(protoObject, regT1, cachedOffset);
        JITStubCall stubCall(this, cti_op_get_by_id_getter_stub);
        stubCall.addArgument(regT1);
        stubCall.addArgument(regT0);
        stubCall.addArgument(TrustedImmPtr(stubInfo->callReturnLocation.executableAddress()));
        stubCall.call();
    } else if (slot.cachedPropertyType() == PropertySlot::Custom) {
        needsStubLink = true;
        JITStubCall stubCall(this, cti_op_get_by_id_custom_stub);
        stubCall.addArgument(TrustedImmPtr(protoObject));
        stubCall.addArgument(TrustedImmPtr(FunctionPtr(slot.customGetter()).executableAddress()));
        stubCall.addArgument(TrustedImmPtr(const_cast<Identifier*>(&ident)));
        stubCall.addArgument(TrustedImmPtr(stubInfo->callReturnLocation.executableAddress()));
        stubCall.call();
    } else
        compileGetDirectOffset(protoObject, regT0, cachedOffset);
    Jump success = jump();

    LinkBuffer patchBuffer(this, m_codeBlock->executablePool());

    if (needsStubLink) {
        for (Vector<CallRecord>::iterator iter = m_calls.begin(); iter != m_calls.end(); ++iter) {
            if (iter->to)
                patchBuffer.link(iter->from, FunctionPtr(iter->to));
        }
    }

    // Use the patch information to link the failure cases back to the original slow case routine.
    patchBuffer.link(bucketsOfFail, stubInfo->callReturnLocation.labelAtOffset(-patchOffsetGetByIdSlowCaseCall));

    // On success return back to the hot patch code, at a point it will perform the store to dest for us.
    patchBuffer.link(success, stubInfo->hotPathBegin.labelAtOffset(patchOffsetGetByIdPutResult));

    // Track the stub we have created so that it will be deleted later.
    CodeLocationLabel entryLabel = patchBuffer.finalizeCodeAddendum();
    stubInfo->stubRoutine = entryLabel;

    // Finally patch the jump to slow case back in the hot path to jump here instead.
    CodeLocationJump jumpLocation = stubInfo->hotPathBegin.jumpAtOffset(patchOffsetGetByIdBranchToSlowCase);
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(jumpLocation, entryLabel);

    // We don't want to patch more than once - in future go to cti_op_put_by_id_generic.
    repatchBuffer.relinkCallerToFunction(returnAddress, FunctionPtr(cti_op_get_by_id_proto_list));
}

/* ------------------------------ END: !ENABLE / ENABLE(JIT_OPTIMIZE_PROPERTY_ACCESS) ------------------------------ */

#endif // !ENABLE(JIT_OPTIMIZE_PROPERTY_ACCESS)

#endif // USE(JSVALUE64)

void JIT::testPrototype(JSValue prototype, JumpList& failureCases)
{
    if (prototype.isNull())
        return;

    ASSERT(prototype.isCell());
    move(TrustedImmPtr(prototype.asCell()), regT3);
    failureCases.append(branchPtr(NotEqual, Address(regT3, JSCell::structureOffset()), TrustedImmPtr(prototype.asCell()->structure())));
}

} // namespace JSC

#endif // ENABLE(JIT)
