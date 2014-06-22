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
#if USE(JSVALUE32_64)
#include "JIT.h"

#include "CodeBlock.h"
#include "GCAwareJITStubRoutine.h"
#include "Interpreter.h"
#include "JITInlines.h"
#include "JITStubCall.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "JSPropertyNameIterator.h"
#include "JSVariableObject.h"
#include "LinkBuffer.h"
#include "RepatchBuffer.h"
#include "ResultType.h"
#include "SamplingTool.h"
#include <wtf/StringPrintStream.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

using namespace std;

namespace JSC {
    
void JIT::emit_op_put_by_index(Instruction* currentInstruction)
{
    unsigned base = currentInstruction[1].u.operand;
    unsigned property = currentInstruction[2].u.operand;
    unsigned value = currentInstruction[3].u.operand;
    
    JITStubCall stubCall(this, cti_op_put_by_index);
    stubCall.addArgument(base);
    stubCall.addArgument(TrustedImm32(property));
    stubCall.addArgument(value);
    stubCall.call();
}

void JIT::emit_op_put_getter_setter(Instruction* currentInstruction)
{
    unsigned base = currentInstruction[1].u.operand;
    unsigned property = currentInstruction[2].u.operand;
    unsigned getter = currentInstruction[3].u.operand;
    unsigned setter = currentInstruction[4].u.operand;
    
    JITStubCall stubCall(this, cti_op_put_getter_setter);
    stubCall.addArgument(base);
    stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(property)));
    stubCall.addArgument(getter);
    stubCall.addArgument(setter);
    stubCall.call();
}

void JIT::emit_op_del_by_id(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned base = currentInstruction[2].u.operand;
    unsigned property = currentInstruction[3].u.operand;
    
    JITStubCall stubCall(this, cti_op_del_by_id);
    stubCall.addArgument(base);
    stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(property)));
    stubCall.call(dst);
}

JIT::CodeRef JIT::stringGetByValStubGenerator(VM* vm)
{
    JSInterfaceJIT jit;
    JumpList failures;
    failures.append(jit.branchPtr(NotEqual, Address(regT0, JSCell::structureOffset()), TrustedImmPtr(vm->stringStructure.get())));
    
    // Load string length to regT1, and start the process of loading the data pointer into regT0
    jit.load32(Address(regT0, ThunkHelpers::jsStringLengthOffset()), regT1);
    jit.loadPtr(Address(regT0, ThunkHelpers::jsStringValueOffset()), regT0);
    failures.append(jit.branchTest32(Zero, regT0));
    
    // Do an unsigned compare to simultaneously filter negative indices as well as indices that are too large
    failures.append(jit.branch32(AboveOrEqual, regT2, regT1));
    
    // Load the character
    JumpList is16Bit;
    JumpList cont8Bit;
    // Load the string flags
    jit.loadPtr(Address(regT0, StringImpl::flagsOffset()), regT1);
    jit.loadPtr(Address(regT0, StringImpl::dataOffset()), regT0);
    is16Bit.append(jit.branchTest32(Zero, regT1, TrustedImm32(StringImpl::flagIs8Bit())));
    jit.load8(BaseIndex(regT0, regT2, TimesOne, 0), regT0);
    cont8Bit.append(jit.jump());
    is16Bit.link(&jit);
    jit.load16(BaseIndex(regT0, regT2, TimesTwo, 0), regT0);

    cont8Bit.link(&jit);
    
    failures.append(jit.branch32(AboveOrEqual, regT0, TrustedImm32(0x100)));
    jit.move(TrustedImmPtr(vm->smallStrings.singleCharacterStrings()), regT1);
    jit.loadPtr(BaseIndex(regT1, regT0, ScalePtr, 0), regT0);
    jit.move(TrustedImm32(JSValue::CellTag), regT1); // We null check regT0 on return so this is safe
    jit.ret();

    failures.link(&jit);
    jit.move(TrustedImm32(0), regT0);
    jit.ret();
    
    LinkBuffer patchBuffer(*vm, &jit, GLOBAL_THUNK_ID);
    return FINALIZE_CODE(patchBuffer, ("String get_by_val stub"));
}

void JIT::emit_op_get_by_val(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned base = currentInstruction[2].u.operand;
    unsigned property = currentInstruction[3].u.operand;
    ArrayProfile* profile = currentInstruction[4].u.arrayProfile;
    
    emitLoad2(base, regT1, regT0, property, regT3, regT2);
    
    addSlowCase(branch32(NotEqual, regT3, TrustedImm32(JSValue::Int32Tag)));
    emitJumpSlowCaseIfNotJSCell(base, regT1);
    loadPtr(Address(regT0, JSCell::structureOffset()), regT1);
    emitArrayProfilingSite(regT1, regT3, profile);
    and32(TrustedImm32(IndexingShapeMask), regT1);

    PatchableJump badType;
    JumpList slowCases;
    
    JITArrayMode mode = chooseArrayMode(profile);
    switch (mode) {
    case JITInt32:
        slowCases = emitInt32GetByVal(currentInstruction, badType);
        break;
    case JITDouble:
        slowCases = emitDoubleGetByVal(currentInstruction, badType);
        break;
    case JITContiguous:
        slowCases = emitContiguousGetByVal(currentInstruction, badType);
        break;
    case JITArrayStorage:
        slowCases = emitArrayStorageGetByVal(currentInstruction, badType);
        break;
    default:
        CRASH();
    }
    
    addSlowCase(badType);
    addSlowCase(slowCases);
    
    Label done = label();

#if !ASSERT_DISABLED
    Jump resultOK = branch32(NotEqual, regT1, TrustedImm32(JSValue::EmptyValueTag));
    breakpoint();
    resultOK.link(this);
#endif

    emitValueProfilingSite();
    emitStore(dst, regT1, regT0);
    map(m_bytecodeOffset + OPCODE_LENGTH(op_get_by_val), dst, regT1, regT0);
    
    m_byValCompilationInfo.append(ByValCompilationInfo(m_bytecodeOffset, badType, mode, done));
}

JIT::JumpList JIT::emitContiguousGetByVal(Instruction*, PatchableJump& badType, IndexingType expectedShape)
{
    JumpList slowCases;
    
    badType = patchableBranch32(NotEqual, regT1, TrustedImm32(expectedShape));
    
    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT3);
    slowCases.append(branch32(AboveOrEqual, regT2, Address(regT3, Butterfly::offsetOfPublicLength())));
    
    load32(BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)), regT1); // tag
    load32(BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)), regT0); // payload
    slowCases.append(branch32(Equal, regT1, TrustedImm32(JSValue::EmptyValueTag)));
    
    return slowCases;
}

JIT::JumpList JIT::emitDoubleGetByVal(Instruction*, PatchableJump& badType)
{
    JumpList slowCases;
    
    badType = patchableBranch32(NotEqual, regT1, TrustedImm32(DoubleShape));
    
    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT3);
    slowCases.append(branch32(AboveOrEqual, regT2, Address(regT3, Butterfly::offsetOfPublicLength())));
    
    loadDouble(BaseIndex(regT3, regT2, TimesEight), fpRegT0);
    slowCases.append(branchDouble(DoubleNotEqualOrUnordered, fpRegT0, fpRegT0));
    moveDoubleToInts(fpRegT0, regT0, regT1);
    
    return slowCases;
}

JIT::JumpList JIT::emitArrayStorageGetByVal(Instruction*, PatchableJump& badType)
{
    JumpList slowCases;
    
    add32(TrustedImm32(-ArrayStorageShape), regT1, regT3);
    badType = patchableBranch32(Above, regT3, TrustedImm32(SlowPutArrayStorageShape - ArrayStorageShape));
    
    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT3);
    slowCases.append(branch32(AboveOrEqual, regT2, Address(regT3, ArrayStorage::vectorLengthOffset())));
    
    load32(BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), regT1); // tag
    load32(BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), regT0); // payload
    slowCases.append(branch32(Equal, regT1, TrustedImm32(JSValue::EmptyValueTag)));
    
    return slowCases;
}
    
void JIT::emitSlow_op_get_by_val(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned base = currentInstruction[2].u.operand;
    unsigned property = currentInstruction[3].u.operand;
    ArrayProfile* profile = currentInstruction[4].u.arrayProfile;
    
    linkSlowCase(iter); // property int32 check
    linkSlowCaseIfNotJSCell(iter, base); // base cell check

    Jump nonCell = jump();
    linkSlowCase(iter); // base array check
    Jump notString = branchPtr(NotEqual, Address(regT0, JSCell::structureOffset()), TrustedImmPtr(m_vm->stringStructure.get()));
    emitNakedCall(m_vm->getCTIStub(stringGetByValStubGenerator).code());
    Jump failed = branchTestPtr(Zero, regT0);
    emitStore(dst, regT1, regT0);
    emitJumpSlowToHot(jump(), OPCODE_LENGTH(op_get_by_val));
    failed.link(this);
    notString.link(this);
    nonCell.link(this);
    
    Jump skipProfiling = jump();

    linkSlowCase(iter); // vector length check
    linkSlowCase(iter); // empty value
    
    emitArrayProfileOutOfBoundsSpecialCase(profile);
    
    skipProfiling.link(this);
    
    Label slowPath = label();
    
    JITStubCall stubCall(this, cti_op_get_by_val);
    stubCall.addArgument(base);
    stubCall.addArgument(property);
    Call call = stubCall.call(dst);

    m_byValCompilationInfo[m_byValInstructionIndex].slowPathTarget = slowPath;
    m_byValCompilationInfo[m_byValInstructionIndex].returnAddress = call;
    m_byValInstructionIndex++;

    emitValueProfilingSite();
}

void JIT::emit_op_put_by_val(Instruction* currentInstruction)
{
    unsigned base = currentInstruction[1].u.operand;
    unsigned property = currentInstruction[2].u.operand;
    ArrayProfile* profile = currentInstruction[4].u.arrayProfile;
    
    emitLoad2(base, regT1, regT0, property, regT3, regT2);
    
    addSlowCase(branch32(NotEqual, regT3, TrustedImm32(JSValue::Int32Tag)));
    emitJumpSlowCaseIfNotJSCell(base, regT1);
    loadPtr(Address(regT0, JSCell::structureOffset()), regT1);
    emitArrayProfilingSite(regT1, regT3, profile);
    and32(TrustedImm32(IndexingShapeMask), regT1);
    
    PatchableJump badType;
    JumpList slowCases;
    
    JITArrayMode mode = chooseArrayMode(profile);
    switch (mode) {
    case JITInt32:
        slowCases = emitInt32PutByVal(currentInstruction, badType);
        break;
    case JITDouble:
        slowCases = emitDoublePutByVal(currentInstruction, badType);
        break;
    case JITContiguous:
        slowCases = emitContiguousPutByVal(currentInstruction, badType);
        break;
    case JITArrayStorage:
        slowCases = emitArrayStoragePutByVal(currentInstruction, badType);
        break;
    default:
        CRASH();
        break;
    }
    
    addSlowCase(badType);
    addSlowCase(slowCases);
    
    Label done = label();
    
    m_byValCompilationInfo.append(ByValCompilationInfo(m_bytecodeOffset, badType, mode, done));
}

JIT::JumpList JIT::emitGenericContiguousPutByVal(Instruction* currentInstruction, PatchableJump& badType, IndexingType indexingShape)
{
    unsigned value = currentInstruction[3].u.operand;
    ArrayProfile* profile = currentInstruction[4].u.arrayProfile;
    
    JumpList slowCases;
    
    badType = patchableBranch32(NotEqual, regT1, TrustedImm32(ContiguousShape));
    
    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT3);
    Jump outOfBounds = branch32(AboveOrEqual, regT2, Address(regT3, Butterfly::offsetOfPublicLength()));
    
    Label storeResult = label();
    emitLoad(value, regT1, regT0);
    switch (indexingShape) {
    case Int32Shape:
        slowCases.append(branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag)));
        // Fall through.
    case ContiguousShape:
        store32(regT0, BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
        store32(regT1, BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
        break;
    case DoubleShape: {
        Jump notInt = branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag));
        convertInt32ToDouble(regT0, fpRegT0);
        Jump ready = jump();
        notInt.link(this);
        moveIntsToDouble(regT0, regT1, fpRegT0, fpRegT1);
        slowCases.append(branchDouble(DoubleNotEqualOrUnordered, fpRegT0, fpRegT0));
        ready.link(this);
        storeDouble(fpRegT0, BaseIndex(regT3, regT2, TimesEight));
        break;
    }
    default:
        CRASH();
        break;
    }
        
    Jump done = jump();
    
    outOfBounds.link(this);
    slowCases.append(branch32(AboveOrEqual, regT2, Address(regT3, Butterfly::offsetOfVectorLength())));
    
    emitArrayProfileStoreToHoleSpecialCase(profile);
    
    add32(TrustedImm32(1), regT2, regT1);
    store32(regT1, Address(regT3, Butterfly::offsetOfPublicLength()));
    jump().linkTo(storeResult, this);
    
    done.link(this);
    
    emitWriteBarrier(regT0, regT1, regT1, regT3, UnconditionalWriteBarrier, WriteBarrierForPropertyAccess);
    
    return slowCases;
}

JIT::JumpList JIT::emitArrayStoragePutByVal(Instruction* currentInstruction, PatchableJump& badType)
{
    unsigned value = currentInstruction[3].u.operand;
    ArrayProfile* profile = currentInstruction[4].u.arrayProfile;
    
    JumpList slowCases;
    
    badType = patchableBranch32(NotEqual, regT1, TrustedImm32(ArrayStorageShape));
    
    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT3);
    slowCases.append(branch32(AboveOrEqual, regT2, Address(regT3, ArrayStorage::vectorLengthOffset())));

    Jump empty = branch32(Equal, BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), TrustedImm32(JSValue::EmptyValueTag));
    
    Label storeResult(this);
    emitLoad(value, regT1, regT0);
    store32(regT0, BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload))); // payload
    store32(regT1, BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag))); // tag
    Jump end = jump();
    
    empty.link(this);
    emitArrayProfileStoreToHoleSpecialCase(profile);
    add32(TrustedImm32(1), Address(regT3, OBJECT_OFFSETOF(ArrayStorage, m_numValuesInVector)));
    branch32(Below, regT2, Address(regT3, ArrayStorage::lengthOffset())).linkTo(storeResult, this);
    
    add32(TrustedImm32(1), regT2, regT0);
    store32(regT0, Address(regT3, ArrayStorage::lengthOffset()));
    jump().linkTo(storeResult, this);
    
    end.link(this);
    
    emitWriteBarrier(regT0, regT1, regT1, regT3, UnconditionalWriteBarrier, WriteBarrierForPropertyAccess);
    
    return slowCases;
}

void JIT::emitSlow_op_put_by_val(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned base = currentInstruction[1].u.operand;
    unsigned property = currentInstruction[2].u.operand;
    unsigned value = currentInstruction[3].u.operand;
    ArrayProfile* profile = currentInstruction[4].u.arrayProfile;
    
    linkSlowCase(iter); // property int32 check
    linkSlowCaseIfNotJSCell(iter, base); // base cell check
    linkSlowCase(iter); // base not array check
    
    JITArrayMode mode = chooseArrayMode(profile);
    switch (mode) {
    case JITInt32:
    case JITDouble:
        linkSlowCase(iter); // value type check
        break;
    default:
        break;
    }
    
    Jump skipProfiling = jump();
    linkSlowCase(iter); // out of bounds
    emitArrayProfileOutOfBoundsSpecialCase(profile);
    skipProfiling.link(this);

    Label slowPath = label();
    
    JITStubCall stubPutByValCall(this, cti_op_put_by_val);
    stubPutByValCall.addArgument(base);
    stubPutByValCall.addArgument(property);
    stubPutByValCall.addArgument(value);
    Call call = stubPutByValCall.call();
    
    m_byValCompilationInfo[m_byValInstructionIndex].slowPathTarget = slowPath;
    m_byValCompilationInfo[m_byValInstructionIndex].returnAddress = call;
    m_byValInstructionIndex++;
}

void JIT::emit_op_get_by_id(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    int base = currentInstruction[2].u.operand;
    Identifier* ident = &(m_codeBlock->identifier(currentInstruction[3].u.operand));
    
    emitLoad(base, regT1, regT0);
    emitJumpSlowCaseIfNotJSCell(base, regT1);
    compileGetByIdHotPath(ident);
    emitValueProfilingSite();
    emitStore(dst, regT1, regT0);
    map(m_bytecodeOffset + OPCODE_LENGTH(op_get_by_id), dst, regT1, regT0);
}

void JIT::compileGetByIdHotPath(Identifier* ident)
{
    // As for put_by_id, get_by_id requires the offset of the Structure and the offset of the access to be patched.
    // Additionally, for get_by_id we need patch the offset of the branch to the slow case (we patch this to jump
    // to array-length / prototype access tranpolines, and finally we also the the property-map access offset as a label
    // to jump back to if one of these trampolies finds a match.
    
    if (*ident == m_vm->propertyNames->length && shouldEmitProfiling()) {
        loadPtr(Address(regT0, JSCell::structureOffset()), regT2);
        emitArrayProfilingSiteForBytecodeIndex(regT2, regT3, m_bytecodeOffset);
    }

    BEGIN_UNINTERRUPTED_SEQUENCE(sequenceGetByIdHotPath);
    
    Label hotPathBegin(this);
    
    DataLabelPtr structureToCompare;
    PatchableJump structureCheck = patchableBranchPtrWithPatch(NotEqual, Address(regT0, JSCell::structureOffset()), structureToCompare, TrustedImmPtr(reinterpret_cast<void*>(patchGetByIdDefaultStructure)));
    addSlowCase(structureCheck);
    
    ConvertibleLoadLabel propertyStorageLoad = convertibleLoadPtr(Address(regT0, JSObject::butterflyOffset()), regT2);
    DataLabelCompact displacementLabel1 = loadPtrWithCompactAddressOffsetPatch(Address(regT2, patchGetByIdDefaultOffset), regT0); // payload
    DataLabelCompact displacementLabel2 = loadPtrWithCompactAddressOffsetPatch(Address(regT2, patchGetByIdDefaultOffset), regT1); // tag
    
    Label putResult(this);
    
    END_UNINTERRUPTED_SEQUENCE(sequenceGetByIdHotPath);

    m_propertyAccessCompilationInfo.append(PropertyStubCompilationInfo(PropertyStubGetById, m_bytecodeOffset, hotPathBegin, structureToCompare, structureCheck, propertyStorageLoad, displacementLabel1, displacementLabel2, putResult));
}

void JIT::emitSlow_op_get_by_id(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    int dst = currentInstruction[1].u.operand;
    int base = currentInstruction[2].u.operand;
    int ident = currentInstruction[3].u.operand;
    
    compileGetByIdSlowCase(dst, base, &(m_codeBlock->identifier(ident)), iter);
    emitValueProfilingSite();
}

void JIT::compileGetByIdSlowCase(int dst, int base, Identifier* ident, Vector<SlowCaseEntry>::iterator& iter)
{
    // As for the hot path of get_by_id, above, we ensure that we can use an architecture specific offset
    // so that we only need track one pointer into the slow case code - we track a pointer to the location
    // of the call (which we can use to look up the patch information), but should a array-length or
    // prototype access trampoline fail we want to bail out back to here.  To do so we can subtract back
    // the distance from the call to the head of the slow case.
    linkSlowCaseIfNotJSCell(iter, base);
    linkSlowCase(iter);
    
    BEGIN_UNINTERRUPTED_SEQUENCE(sequenceGetByIdSlowCase);
    
    Label coldPathBegin(this);
    JITStubCall stubCall(this, cti_op_get_by_id);
    stubCall.addArgument(regT1, regT0);
    stubCall.addArgument(TrustedImmPtr(ident));
    Call call = stubCall.call(dst);
    
    END_UNINTERRUPTED_SEQUENCE_FOR_PUT(sequenceGetByIdSlowCase, dst);
    
    // Track the location of the call; this will be used to recover patch information.
    m_propertyAccessCompilationInfo[m_propertyAccessInstructionIndex++].slowCaseInfo(PropertyStubGetById, coldPathBegin, call);
}

void JIT::emit_op_put_by_id(Instruction* currentInstruction)
{
    // In order to be able to patch both the Structure, and the object offset, we store one pointer,
    // to just after the arguments have been loaded into registers 'hotPathBegin', and we generate code
    // such that the Structure & offset are always at the same distance from this.
    
    int base = currentInstruction[1].u.operand;
    int value = currentInstruction[3].u.operand;
    
    emitLoad2(base, regT1, regT0, value, regT3, regT2);
    
    emitJumpSlowCaseIfNotJSCell(base, regT1);
    
    BEGIN_UNINTERRUPTED_SEQUENCE(sequencePutById);
    
    Label hotPathBegin(this);
    
    // It is important that the following instruction plants a 32bit immediate, in order that it can be patched over.
    DataLabelPtr structureToCompare;
    addSlowCase(branchPtrWithPatch(NotEqual, Address(regT0, JSCell::structureOffset()), structureToCompare, TrustedImmPtr(reinterpret_cast<void*>(patchGetByIdDefaultStructure))));
    
    ConvertibleLoadLabel propertyStorageLoad = convertibleLoadPtr(Address(regT0, JSObject::butterflyOffset()), regT1);
    DataLabel32 displacementLabel1 = storePtrWithAddressOffsetPatch(regT2, Address(regT1, patchPutByIdDefaultOffset)); // payload
    DataLabel32 displacementLabel2 = storePtrWithAddressOffsetPatch(regT3, Address(regT1, patchPutByIdDefaultOffset)); // tag
    
    END_UNINTERRUPTED_SEQUENCE(sequencePutById);

    emitWriteBarrier(regT0, regT2, regT1, regT2, ShouldFilterImmediates, WriteBarrierForPropertyAccess);

    m_propertyAccessCompilationInfo.append(PropertyStubCompilationInfo(PropertyStubPutById, m_bytecodeOffset, hotPathBegin, structureToCompare, propertyStorageLoad, displacementLabel1, displacementLabel2));
}

void JIT::emitSlow_op_put_by_id(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    int base = currentInstruction[1].u.operand;
    int ident = currentInstruction[2].u.operand;
    int direct = currentInstruction[8].u.operand;

    linkSlowCaseIfNotJSCell(iter, base);
    linkSlowCase(iter);
    
    JITStubCall stubCall(this, direct ? cti_op_put_by_id_direct : cti_op_put_by_id);
    stubCall.addArgument(base);
    stubCall.addArgument(TrustedImmPtr(&(m_codeBlock->identifier(ident))));
    stubCall.addArgument(regT3, regT2); 
    Call call = stubCall.call();
    
    // Track the location of the call; this will be used to recover patch information.
    m_propertyAccessCompilationInfo[m_propertyAccessInstructionIndex++].slowCaseInfo(PropertyStubPutById, call);
}

// Compile a store into an object's property storage.  May overwrite base.
void JIT::compilePutDirectOffset(RegisterID base, RegisterID valueTag, RegisterID valuePayload, PropertyOffset cachedOffset)
{
    if (isOutOfLineOffset(cachedOffset))
        loadPtr(Address(base, JSObject::butterflyOffset()), base);
    emitStore(indexRelativeToBase(cachedOffset), valueTag, valuePayload, base);
}

// Compile a load from an object's property storage.  May overwrite base.
void JIT::compileGetDirectOffset(RegisterID base, RegisterID resultTag, RegisterID resultPayload, PropertyOffset cachedOffset)
{
    if (isInlineOffset(cachedOffset)) {
        emitLoad(indexRelativeToBase(cachedOffset), resultTag, resultPayload, base);
        return;
    }
    
    RegisterID temp = resultPayload;
    loadPtr(Address(base, JSObject::butterflyOffset()), temp);
    emitLoad(indexRelativeToBase(cachedOffset), resultTag, resultPayload, temp);
}

void JIT::compileGetDirectOffset(JSObject* base, RegisterID resultTag, RegisterID resultPayload, PropertyOffset cachedOffset)
{
    if (isInlineOffset(cachedOffset)) {
        move(TrustedImmPtr(base->locationForOffset(cachedOffset)), resultTag);
        load32(Address(resultTag, OBJECT_OFFSETOF(JSValue, u.asBits.payload)), resultPayload);
        load32(Address(resultTag, OBJECT_OFFSETOF(JSValue, u.asBits.tag)), resultTag);
        return;
    }
    
    loadPtr(base->butterflyAddress(), resultTag);
    load32(Address(resultTag, offsetInButterfly(cachedOffset) * sizeof(WriteBarrier<Unknown>) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), resultPayload);
    load32(Address(resultTag, offsetInButterfly(cachedOffset) * sizeof(WriteBarrier<Unknown>) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), resultTag);
}

void JIT::privateCompilePutByIdTransition(StructureStubInfo* stubInfo, Structure* oldStructure, Structure* newStructure, PropertyOffset cachedOffset, StructureChain* chain, ReturnAddressPtr returnAddress, bool direct)
{
    // The code below assumes that regT0 contains the basePayload and regT1 contains the baseTag. Restore them from the stack.
#if CPU(MIPS) || CPU(SH4) || CPU(ARM)
    // For MIPS, we don't add sizeof(void*) to the stack offset.
    load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), regT0);
    // For MIPS, we don't add sizeof(void*) to the stack offset.
    load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), regT1);
#else
    load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[0]) + sizeof(void*) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), regT0);
    load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[0]) + sizeof(void*) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), regT1);
#endif

    JumpList failureCases;
    failureCases.append(branch32(NotEqual, regT1, TrustedImm32(JSValue::CellTag)));
    failureCases.append(branchPtr(NotEqual, Address(regT0, JSCell::structureOffset()), TrustedImmPtr(oldStructure)));
    testPrototype(oldStructure->storedPrototype(), failureCases, stubInfo);
    
    if (!direct) {
        // Verify that nothing in the prototype chain has a setter for this property. 
        for (WriteBarrier<Structure>* it = chain->head(); *it; ++it)
            testPrototype((*it)->storedPrototype(), failureCases, stubInfo);
    }

    // If we succeed in all of our checks, and the code was optimizable, then make sure we
    // decrement the rare case counter.
#if ENABLE(VALUE_PROFILER)
    if (m_codeBlock->canCompileWithDFG() >= DFG::MayInline) {
        sub32(
            TrustedImm32(1),
            AbsoluteAddress(&m_codeBlock->rareCaseProfileForBytecodeOffset(stubInfo->bytecodeIndex)->m_counter));
    }
#endif
    
    // Reallocate property storage if needed.
    Call callTarget;
    bool willNeedStorageRealloc = oldStructure->outOfLineCapacity() != newStructure->outOfLineCapacity();
    if (willNeedStorageRealloc) {
        // This trampoline was called to like a JIT stub; before we can can call again we need to
        // remove the return address from the stack, to prevent the stack from becoming misaligned.
        preserveReturnAddressAfterCall(regT3);
        
        JITStubCall stubCall(this, cti_op_put_by_id_transition_realloc);
        stubCall.skipArgument(); // base
        stubCall.skipArgument(); // ident
        stubCall.skipArgument(); // value
        stubCall.addArgument(TrustedImm32(oldStructure->outOfLineCapacity()));
        stubCall.addArgument(TrustedImmPtr(newStructure));
        stubCall.call(regT0);

        restoreReturnAddressBeforeReturn(regT3);

#if CPU(MIPS) || CPU(SH4) || CPU(ARM)
        // For MIPS, we don't add sizeof(void*) to the stack offset.
        load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), regT0);
        // For MIPS, we don't add sizeof(void*) to the stack offset.
        load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), regT1);
#else
        load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[0]) + sizeof(void*) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), regT0);
        load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[0]) + sizeof(void*) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), regT1);
#endif
    }

    emitWriteBarrier(regT0, regT1, regT1, regT3, UnconditionalWriteBarrier, WriteBarrierForPropertyAccess);

    storePtr(TrustedImmPtr(newStructure), Address(regT0, JSCell::structureOffset()));
#if CPU(MIPS) || CPU(SH4) || CPU(ARM)
    // For MIPS, we don't add sizeof(void*) to the stack offset.
    load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[2]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), regT3);
    load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[2]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), regT2);
#else
    load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[2]) + sizeof(void*) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), regT3);
    load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[2]) + sizeof(void*) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), regT2);
#endif
    compilePutDirectOffset(regT0, regT2, regT3, cachedOffset);
    
    ret();
    
    ASSERT(!failureCases.empty());
    failureCases.link(this);
    restoreArgumentReferenceForTrampoline();
    Call failureCall = tailRecursiveCall();
    
    LinkBuffer patchBuffer(*m_vm, this, m_codeBlock);
    
    patchBuffer.link(failureCall, FunctionPtr(direct ? cti_op_put_by_id_direct_fail : cti_op_put_by_id_fail));
    
    if (willNeedStorageRealloc) {
        ASSERT(m_calls.size() == 1);
        patchBuffer.link(m_calls[0].from, FunctionPtr(cti_op_put_by_id_transition_realloc));
    }
    
    stubInfo->stubRoutine = createJITStubRoutine(
        FINALIZE_CODE(
            patchBuffer,
            ("Baseline put_by_id transition stub for %s, return point %p",
                toCString(*m_codeBlock).data(), returnAddress.value())),
        *m_vm,
        m_codeBlock->ownerExecutable(),
        willNeedStorageRealloc,
        newStructure);
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relinkCallerToTrampoline(returnAddress, CodeLocationLabel(stubInfo->stubRoutine->code().code()));
}

void JIT::patchGetByIdSelf(CodeBlock* codeBlock, StructureStubInfo* stubInfo, Structure* structure, PropertyOffset cachedOffset, ReturnAddressPtr returnAddress)
{
    RepatchBuffer repatchBuffer(codeBlock);
    
    // We don't want to patch more than once - in future go to cti_op_get_by_id_generic.
    // Should probably go to JITStubs::cti_op_get_by_id_fail, but that doesn't do anything interesting right now.
    repatchBuffer.relinkCallerToFunction(returnAddress, FunctionPtr(cti_op_get_by_id_self_fail));
    
    // Patch the offset into the propoerty map to load from, then patch the Structure to look for.
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelPtrAtOffset(stubInfo->patch.baseline.u.get.structureToCompare), structure);
    repatchBuffer.setLoadInstructionIsActive(stubInfo->hotPathBegin.convertibleLoadAtOffset(stubInfo->patch.baseline.u.get.propertyStorageLoad), isOutOfLineOffset(cachedOffset));
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelCompactAtOffset(stubInfo->patch.baseline.u.get.displacementLabel1), offsetRelativeToPatchedStorage(cachedOffset) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)); // payload
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelCompactAtOffset(stubInfo->patch.baseline.u.get.displacementLabel2), offsetRelativeToPatchedStorage(cachedOffset) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)); // tag
}

void JIT::patchPutByIdReplace(CodeBlock* codeBlock, StructureStubInfo* stubInfo, Structure* structure, PropertyOffset cachedOffset, ReturnAddressPtr returnAddress, bool direct)
{
    RepatchBuffer repatchBuffer(codeBlock);
    
    // We don't want to patch more than once - in future go to cti_op_put_by_id_generic.
    // Should probably go to cti_op_put_by_id_fail, but that doesn't do anything interesting right now.
    repatchBuffer.relinkCallerToFunction(returnAddress, FunctionPtr(direct ? cti_op_put_by_id_direct_generic : cti_op_put_by_id_generic));
    
    // Patch the offset into the propoerty map to load from, then patch the Structure to look for.
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelPtrAtOffset(stubInfo->patch.baseline.u.put.structureToCompare), structure);
    repatchBuffer.setLoadInstructionIsActive(stubInfo->hotPathBegin.convertibleLoadAtOffset(stubInfo->patch.baseline.u.put.propertyStorageLoad), isOutOfLineOffset(cachedOffset));
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabel32AtOffset(stubInfo->patch.baseline.u.put.displacementLabel1), offsetRelativeToPatchedStorage(cachedOffset) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)); // payload
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabel32AtOffset(stubInfo->patch.baseline.u.put.displacementLabel2), offsetRelativeToPatchedStorage(cachedOffset) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)); // tag
}

void JIT::privateCompilePatchGetArrayLength(ReturnAddressPtr returnAddress)
{
    StructureStubInfo* stubInfo = &m_codeBlock->getStubInfo(returnAddress);
    
    // regT0 holds a JSCell*
    
    // Check for array
    loadPtr(Address(regT0, JSCell::structureOffset()), regT2);
    Jump failureCases1 = branchTest32(Zero, regT2, TrustedImm32(IsArray));
    Jump failureCases2 = branchTest32(Zero, regT2, TrustedImm32(IndexingShapeMask));
    
    // Checks out okay! - get the length from the storage
    loadPtr(Address(regT0, JSArray::butterflyOffset()), regT2);
    load32(Address(regT2, ArrayStorage::lengthOffset()), regT2);
    
    Jump failureCases3 = branch32(Above, regT2, TrustedImm32(INT_MAX));
    move(regT2, regT0);
    move(TrustedImm32(JSValue::Int32Tag), regT1);
    Jump success = jump();
    
    LinkBuffer patchBuffer(*m_vm, this, m_codeBlock);
    
    // Use the patch information to link the failure cases back to the original slow case routine.
    CodeLocationLabel slowCaseBegin = stubInfo->callReturnLocation.labelAtOffset(-stubInfo->patch.baseline.u.get.coldPathBegin);
    patchBuffer.link(failureCases1, slowCaseBegin);
    patchBuffer.link(failureCases2, slowCaseBegin);
    patchBuffer.link(failureCases3, slowCaseBegin);
    
    // On success return back to the hot patch code, at a point it will perform the store to dest for us.
    patchBuffer.link(success, stubInfo->hotPathBegin.labelAtOffset(stubInfo->patch.baseline.u.get.putResult));
    
    // Track the stub we have created so that it will be deleted later.
    stubInfo->stubRoutine = FINALIZE_CODE_FOR_STUB(
        patchBuffer,
        ("Baseline get_by_id array length stub for %s, return point %p",
            toCString(*m_codeBlock).data(), stubInfo->hotPathBegin.labelAtOffset(
                stubInfo->patch.baseline.u.get.putResult).executableAddress()));
    
    // Finally patch the jump to slow case back in the hot path to jump here instead.
    CodeLocationJump jumpLocation = stubInfo->hotPathBegin.jumpAtOffset(stubInfo->patch.baseline.u.get.structureCheck);
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(jumpLocation, CodeLocationLabel(stubInfo->stubRoutine->code().code()));
    
    // We don't want to patch more than once - in future go to cti_op_put_by_id_generic.
    repatchBuffer.relinkCallerToFunction(returnAddress, FunctionPtr(cti_op_get_by_id_array_fail));
}

void JIT::privateCompileGetByIdProto(StructureStubInfo* stubInfo, Structure* structure, Structure* prototypeStructure, const Identifier& ident, const PropertySlot& slot, PropertyOffset cachedOffset, ReturnAddressPtr returnAddress, CallFrame* callFrame)
{
    // regT0 holds a JSCell*
    
    // The prototype object definitely exists (if this stub exists the CodeBlock is referencing a Structure that is
    // referencing the prototype object - let's speculatively load it's table nice and early!)
    JSObject* protoObject = asObject(structure->prototypeForLookup(callFrame));
    
    Jump failureCases1 = checkStructure(regT0, structure);
    
    // Check the prototype object's Structure had not changed.
    Jump failureCases2 = addStructureTransitionCheck(protoObject, prototypeStructure, stubInfo, regT3);

    bool needsStubLink = false;
    // Checks out okay!
    if (slot.cachedPropertyType() == PropertySlot::Getter) {
        needsStubLink = true;
        compileGetDirectOffset(protoObject, regT2, regT1, cachedOffset);
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
        compileGetDirectOffset(protoObject, regT1, regT0, cachedOffset);
    
    Jump success = jump();
    
    LinkBuffer patchBuffer(*m_vm, this, m_codeBlock);
    
    // Use the patch information to link the failure cases back to the original slow case routine.
    CodeLocationLabel slowCaseBegin = stubInfo->callReturnLocation.labelAtOffset(-stubInfo->patch.baseline.u.get.coldPathBegin);
    patchBuffer.link(failureCases1, slowCaseBegin);
    if (failureCases2.isSet())
        patchBuffer.link(failureCases2, slowCaseBegin);
    
    // On success return back to the hot patch code, at a point it will perform the store to dest for us.
    patchBuffer.link(success, stubInfo->hotPathBegin.labelAtOffset(stubInfo->patch.baseline.u.get.putResult));

    if (needsStubLink) {
        for (Vector<CallRecord>::iterator iter = m_calls.begin(); iter != m_calls.end(); ++iter) {
            if (iter->to)
                patchBuffer.link(iter->from, FunctionPtr(iter->to));
        }
    }

    // Track the stub we have created so that it will be deleted later.
    stubInfo->stubRoutine = createJITStubRoutine(
        FINALIZE_CODE(
            patchBuffer,
            ("Baseline get_by_id proto stub for %s, return point %p",
                toCString(*m_codeBlock).data(), stubInfo->hotPathBegin.labelAtOffset(
                    stubInfo->patch.baseline.u.get.putResult).executableAddress())),
        *m_vm,
        m_codeBlock->ownerExecutable(),
        needsStubLink);
    
    // Finally patch the jump to slow case back in the hot path to jump here instead.
    CodeLocationJump jumpLocation = stubInfo->hotPathBegin.jumpAtOffset(stubInfo->patch.baseline.u.get.structureCheck);
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(jumpLocation, CodeLocationLabel(stubInfo->stubRoutine->code().code()));
    
    // We don't want to patch more than once - in future go to cti_op_put_by_id_generic.
    repatchBuffer.relinkCallerToFunction(returnAddress, FunctionPtr(cti_op_get_by_id_proto_list));
}


void JIT::privateCompileGetByIdSelfList(StructureStubInfo* stubInfo, PolymorphicAccessStructureList* polymorphicStructures, int currentIndex, Structure* structure, const Identifier& ident, const PropertySlot& slot, PropertyOffset cachedOffset)
{
    // regT0 holds a JSCell*
    Jump failureCase = checkStructure(regT0, structure);
    bool needsStubLink = false;
    bool isDirect = false;
    if (slot.cachedPropertyType() == PropertySlot::Getter) {
        needsStubLink = true;
        compileGetDirectOffset(regT0, regT2, regT1, cachedOffset);
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
    } else {
        isDirect = true;
        compileGetDirectOffset(regT0, regT1, regT0, cachedOffset);
    }

    Jump success = jump();
    
    LinkBuffer patchBuffer(*m_vm, this, m_codeBlock);
    if (needsStubLink) {
        for (Vector<CallRecord>::iterator iter = m_calls.begin(); iter != m_calls.end(); ++iter) {
            if (iter->to)
                patchBuffer.link(iter->from, FunctionPtr(iter->to));
        }
    }    
    // Use the patch information to link the failure cases back to the original slow case routine.
    CodeLocationLabel lastProtoBegin = CodeLocationLabel(JITStubRoutine::asCodePtr(polymorphicStructures->list[currentIndex - 1].stubRoutine));
    if (!lastProtoBegin)
        lastProtoBegin = stubInfo->callReturnLocation.labelAtOffset(-stubInfo->patch.baseline.u.get.coldPathBegin);
    
    patchBuffer.link(failureCase, lastProtoBegin);
    
    // On success return back to the hot patch code, at a point it will perform the store to dest for us.
    patchBuffer.link(success, stubInfo->hotPathBegin.labelAtOffset(stubInfo->patch.baseline.u.get.putResult));

    RefPtr<JITStubRoutine> stubRoutine = createJITStubRoutine(
        FINALIZE_CODE(
            patchBuffer,
            ("Baseline get_by_id self list stub for %s, return point %p",
                toCString(*m_codeBlock).data(), stubInfo->hotPathBegin.labelAtOffset(
                    stubInfo->patch.baseline.u.get.putResult).executableAddress())),
        *m_vm,
        m_codeBlock->ownerExecutable(),
        needsStubLink);

    polymorphicStructures->list[currentIndex].set(*m_vm, m_codeBlock->ownerExecutable(), stubRoutine, structure, isDirect);
    
    // Finally patch the jump to slow case back in the hot path to jump here instead.
    CodeLocationJump jumpLocation = stubInfo->hotPathBegin.jumpAtOffset(stubInfo->patch.baseline.u.get.structureCheck);
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(jumpLocation, CodeLocationLabel(stubRoutine->code().code()));
}

void JIT::privateCompileGetByIdProtoList(StructureStubInfo* stubInfo, PolymorphicAccessStructureList* prototypeStructures, int currentIndex, Structure* structure, Structure* prototypeStructure, const Identifier& ident, const PropertySlot& slot, PropertyOffset cachedOffset, CallFrame* callFrame)
{
    // regT0 holds a JSCell*
    
    // The prototype object definitely exists (if this stub exists the CodeBlock is referencing a Structure that is
    // referencing the prototype object - let's speculatively load it's table nice and early!)
    JSObject* protoObject = asObject(structure->prototypeForLookup(callFrame));
    
    // Check eax is an object of the right Structure.
    Jump failureCases1 = checkStructure(regT0, structure);
    
    // Check the prototype object's Structure had not changed.
    Jump failureCases2 = addStructureTransitionCheck(protoObject, prototypeStructure, stubInfo, regT3);
    
    bool needsStubLink = false;
    bool isDirect = false;
    if (slot.cachedPropertyType() == PropertySlot::Getter) {
        needsStubLink = true;
        compileGetDirectOffset(protoObject, regT2, regT1, cachedOffset);
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
    } else {
        isDirect = true;
        compileGetDirectOffset(protoObject, regT1, regT0, cachedOffset);
    }
    
    Jump success = jump();
    
    LinkBuffer patchBuffer(*m_vm, this, m_codeBlock);
    if (needsStubLink) {
        for (Vector<CallRecord>::iterator iter = m_calls.begin(); iter != m_calls.end(); ++iter) {
            if (iter->to)
                patchBuffer.link(iter->from, FunctionPtr(iter->to));
        }
    }
    // Use the patch information to link the failure cases back to the original slow case routine.
    CodeLocationLabel lastProtoBegin = CodeLocationLabel(JITStubRoutine::asCodePtr(prototypeStructures->list[currentIndex - 1].stubRoutine));
    patchBuffer.link(failureCases1, lastProtoBegin);
    if (failureCases2.isSet())
        patchBuffer.link(failureCases2, lastProtoBegin);
    
    // On success return back to the hot patch code, at a point it will perform the store to dest for us.
    patchBuffer.link(success, stubInfo->hotPathBegin.labelAtOffset(stubInfo->patch.baseline.u.get.putResult));
    
    RefPtr<JITStubRoutine> stubRoutine = createJITStubRoutine(
        FINALIZE_CODE(
            patchBuffer,
            ("Baseline get_by_id proto list stub for %s, return point %p",
                toCString(*m_codeBlock).data(), stubInfo->hotPathBegin.labelAtOffset(
                    stubInfo->patch.baseline.u.get.putResult).executableAddress())),
        *m_vm,
        m_codeBlock->ownerExecutable(),
        needsStubLink);

    prototypeStructures->list[currentIndex].set(callFrame->vm(), m_codeBlock->ownerExecutable(), stubRoutine, structure, prototypeStructure, isDirect);
    
    // Finally patch the jump to slow case back in the hot path to jump here instead.
    CodeLocationJump jumpLocation = stubInfo->hotPathBegin.jumpAtOffset(stubInfo->patch.baseline.u.get.structureCheck);
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(jumpLocation, CodeLocationLabel(stubRoutine->code().code()));
}

void JIT::privateCompileGetByIdChainList(StructureStubInfo* stubInfo, PolymorphicAccessStructureList* prototypeStructures, int currentIndex, Structure* structure, StructureChain* chain, size_t count, const Identifier& ident, const PropertySlot& slot, PropertyOffset cachedOffset, CallFrame* callFrame)
{
    // regT0 holds a JSCell*
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
        testPrototype(protoObject, bucketsOfFail, stubInfo);
    }
    ASSERT(protoObject);
    
    bool needsStubLink = false;
    bool isDirect = false;
    if (slot.cachedPropertyType() == PropertySlot::Getter) {
        needsStubLink = true;
        compileGetDirectOffset(protoObject, regT2, regT1, cachedOffset);
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
    } else {
        isDirect = true;
        compileGetDirectOffset(protoObject, regT1, regT0, cachedOffset);
    }

    Jump success = jump();
    
    LinkBuffer patchBuffer(*m_vm, this, m_codeBlock);
    if (needsStubLink) {
        for (Vector<CallRecord>::iterator iter = m_calls.begin(); iter != m_calls.end(); ++iter) {
            if (iter->to)
                patchBuffer.link(iter->from, FunctionPtr(iter->to));
        }
    }
    // Use the patch information to link the failure cases back to the original slow case routine.
    CodeLocationLabel lastProtoBegin = CodeLocationLabel(JITStubRoutine::asCodePtr(prototypeStructures->list[currentIndex - 1].stubRoutine));
    
    patchBuffer.link(bucketsOfFail, lastProtoBegin);
    
    // On success return back to the hot patch code, at a point it will perform the store to dest for us.
    patchBuffer.link(success, stubInfo->hotPathBegin.labelAtOffset(stubInfo->patch.baseline.u.get.putResult));
    
    RefPtr<JITStubRoutine> stubRoutine = createJITStubRoutine(
        FINALIZE_CODE(
            patchBuffer,
            ("Baseline get_by_id chain list stub for %s, return point %p",
                toCString(*m_codeBlock).data(), stubInfo->hotPathBegin.labelAtOffset(
                    stubInfo->patch.baseline.u.get.putResult).executableAddress())),
        *m_vm,
        m_codeBlock->ownerExecutable(),
        needsStubLink);
    
    // Track the stub we have created so that it will be deleted later.
    prototypeStructures->list[currentIndex].set(callFrame->vm(), m_codeBlock->ownerExecutable(), stubRoutine, structure, chain, isDirect);
    
    // Finally patch the jump to slow case back in the hot path to jump here instead.
    CodeLocationJump jumpLocation = stubInfo->hotPathBegin.jumpAtOffset(stubInfo->patch.baseline.u.get.structureCheck);
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(jumpLocation, CodeLocationLabel(stubRoutine->code().code()));
}

void JIT::privateCompileGetByIdChain(StructureStubInfo* stubInfo, Structure* structure, StructureChain* chain, size_t count, const Identifier& ident, const PropertySlot& slot, PropertyOffset cachedOffset, ReturnAddressPtr returnAddress, CallFrame* callFrame)
{
    // regT0 holds a JSCell*
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
        testPrototype(protoObject, bucketsOfFail, stubInfo);
    }
    ASSERT(protoObject);
    
    bool needsStubLink = false;
    if (slot.cachedPropertyType() == PropertySlot::Getter) {
        needsStubLink = true;
        compileGetDirectOffset(protoObject, regT2, regT1, cachedOffset);
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
        compileGetDirectOffset(protoObject, regT1, regT0, cachedOffset);
    Jump success = jump();
    
    LinkBuffer patchBuffer(*m_vm, this, m_codeBlock);
    if (needsStubLink) {
        for (Vector<CallRecord>::iterator iter = m_calls.begin(); iter != m_calls.end(); ++iter) {
            if (iter->to)
                patchBuffer.link(iter->from, FunctionPtr(iter->to));
        }
    }
    // Use the patch information to link the failure cases back to the original slow case routine.
    patchBuffer.link(bucketsOfFail, stubInfo->callReturnLocation.labelAtOffset(-stubInfo->patch.baseline.u.get.coldPathBegin));
    
    // On success return back to the hot patch code, at a point it will perform the store to dest for us.
    patchBuffer.link(success, stubInfo->hotPathBegin.labelAtOffset(stubInfo->patch.baseline.u.get.putResult));
    
    // Track the stub we have created so that it will be deleted later.
    RefPtr<JITStubRoutine> stubRoutine = createJITStubRoutine(
        FINALIZE_CODE(
            patchBuffer,
            ("Baseline get_by_id chain stub for %s, return point %p",
                toCString(*m_codeBlock).data(), stubInfo->hotPathBegin.labelAtOffset(
                    stubInfo->patch.baseline.u.get.putResult).executableAddress())),
        *m_vm,
        m_codeBlock->ownerExecutable(),
        needsStubLink);
    stubInfo->stubRoutine = stubRoutine;
    
    // Finally patch the jump to slow case back in the hot path to jump here instead.
    CodeLocationJump jumpLocation = stubInfo->hotPathBegin.jumpAtOffset(stubInfo->patch.baseline.u.get.structureCheck);
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(jumpLocation, CodeLocationLabel(stubRoutine->code().code()));
    
    // We don't want to patch more than once - in future go to cti_op_put_by_id_generic.
    repatchBuffer.relinkCallerToFunction(returnAddress, FunctionPtr(cti_op_get_by_id_proto_list));
}

void JIT::compileGetDirectOffset(RegisterID base, RegisterID resultTag, RegisterID resultPayload, RegisterID offset, FinalObjectMode finalObjectMode)
{
    ASSERT(sizeof(JSValue) == 8);
    
    if (finalObjectMode == MayBeFinal) {
        Jump isInline = branch32(LessThan, offset, TrustedImm32(firstOutOfLineOffset));
        loadPtr(Address(base, JSObject::butterflyOffset()), base);
        neg32(offset);
        Jump done = jump();
        isInline.link(this);
        addPtr(TrustedImmPtr(JSObject::offsetOfInlineStorage() - (firstOutOfLineOffset - 2) * sizeof(EncodedJSValue)), base);
        done.link(this);
    } else {
#if !ASSERT_DISABLED
        Jump isOutOfLine = branch32(GreaterThanOrEqual, offset, TrustedImm32(firstOutOfLineOffset));
        breakpoint();
        isOutOfLine.link(this);
#endif
        loadPtr(Address(base, JSObject::butterflyOffset()), base);
        neg32(offset);
    }
    load32(BaseIndex(base, offset, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload) + (firstOutOfLineOffset - 2) * sizeof(EncodedJSValue)), resultPayload);
    load32(BaseIndex(base, offset, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag) + (firstOutOfLineOffset - 2) * sizeof(EncodedJSValue)), resultTag);
}

void JIT::emit_op_get_by_pname(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned base = currentInstruction[2].u.operand;
    unsigned property = currentInstruction[3].u.operand;
    unsigned expected = currentInstruction[4].u.operand;
    unsigned iter = currentInstruction[5].u.operand;
    unsigned i = currentInstruction[6].u.operand;
    
    emitLoad2(property, regT1, regT0, base, regT3, regT2);
    emitJumpSlowCaseIfNotJSCell(property, regT1);
    addSlowCase(branchPtr(NotEqual, regT0, payloadFor(expected)));
    // Property registers are now available as the property is known
    emitJumpSlowCaseIfNotJSCell(base, regT3);
    emitLoadPayload(iter, regT1);
    
    // Test base's structure
    loadPtr(Address(regT2, JSCell::structureOffset()), regT0);
    addSlowCase(branchPtr(NotEqual, regT0, Address(regT1, OBJECT_OFFSETOF(JSPropertyNameIterator, m_cachedStructure))));
    load32(addressFor(i), regT3);
    sub32(TrustedImm32(1), regT3);
    addSlowCase(branch32(AboveOrEqual, regT3, Address(regT1, OBJECT_OFFSETOF(JSPropertyNameIterator, m_numCacheableSlots))));
    Jump inlineProperty = branch32(Below, regT3, Address(regT1, OBJECT_OFFSETOF(JSPropertyNameIterator, m_cachedStructureInlineCapacity)));
    add32(TrustedImm32(firstOutOfLineOffset), regT3);
    sub32(Address(regT1, OBJECT_OFFSETOF(JSPropertyNameIterator, m_cachedStructureInlineCapacity)), regT3);
    inlineProperty.link(this);
    compileGetDirectOffset(regT2, regT1, regT0, regT3);    
    
    emitStore(dst, regT1, regT0);
    map(m_bytecodeOffset + OPCODE_LENGTH(op_get_by_pname), dst, regT1, regT0);
}

void JIT::emitSlow_op_get_by_pname(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned base = currentInstruction[2].u.operand;
    unsigned property = currentInstruction[3].u.operand;
    
    linkSlowCaseIfNotJSCell(iter, property);
    linkSlowCase(iter);
    linkSlowCaseIfNotJSCell(iter, base);
    linkSlowCase(iter);
    linkSlowCase(iter);
    
    JITStubCall stubCall(this, cti_op_get_by_val_generic);
    stubCall.addArgument(base);
    stubCall.addArgument(property);
    stubCall.call(dst);
}

void JIT::emit_op_get_scoped_var(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    int index = currentInstruction[2].u.operand;
    int skip = currentInstruction[3].u.operand;

    emitGetFromCallFrameHeaderPtr(JSStack::ScopeChain, regT2);
    bool checkTopLevel = m_codeBlock->codeType() == FunctionCode && m_codeBlock->needsFullScopeChain();
    ASSERT(skip || !checkTopLevel);
    if (checkTopLevel && skip--) {
        Jump activationNotCreated;
        if (checkTopLevel)
            activationNotCreated = branch32(Equal, tagFor(m_codeBlock->activationRegister()), TrustedImm32(JSValue::EmptyValueTag));
        loadPtr(Address(regT2, JSScope::offsetOfNext()), regT2);
        activationNotCreated.link(this);
    }
    while (skip--)
        loadPtr(Address(regT2, JSScope::offsetOfNext()), regT2);

    loadPtr(Address(regT2, JSVariableObject::offsetOfRegisters()), regT2);

    emitLoad(index, regT1, regT0, regT2);
    emitValueProfilingSite();
    emitStore(dst, regT1, regT0);
    map(m_bytecodeOffset + OPCODE_LENGTH(op_get_scoped_var), dst, regT1, regT0);
}

void JIT::emit_op_put_scoped_var(Instruction* currentInstruction)
{
    int index = currentInstruction[1].u.operand;
    int skip = currentInstruction[2].u.operand;
    int value = currentInstruction[3].u.operand;

    emitLoad(value, regT1, regT0);

    emitGetFromCallFrameHeaderPtr(JSStack::ScopeChain, regT2);
    bool checkTopLevel = m_codeBlock->codeType() == FunctionCode && m_codeBlock->needsFullScopeChain();
    ASSERT(skip || !checkTopLevel);
    if (checkTopLevel && skip--) {
        Jump activationNotCreated;
        if (checkTopLevel)
            activationNotCreated = branch32(Equal, tagFor(m_codeBlock->activationRegister()), TrustedImm32(JSValue::EmptyValueTag));
        loadPtr(Address(regT2, JSScope::offsetOfNext()), regT2);
        activationNotCreated.link(this);
    }
    while (skip--)
        loadPtr(Address(regT2, JSScope::offsetOfNext()), regT2);

    loadPtr(Address(regT2, JSVariableObject::offsetOfRegisters()), regT3);
    emitStore(index, regT1, regT0, regT3);
    emitWriteBarrier(regT2, regT1, regT0, regT1, ShouldFilterImmediates, WriteBarrierForVariableAccess);
}

void JIT::emit_op_init_global_const(Instruction* currentInstruction)
{
    WriteBarrier<Unknown>* registerPointer = currentInstruction[1].u.registerPointer;
    int value = currentInstruction[2].u.operand;

    JSGlobalObject* globalObject = m_codeBlock->globalObject();

    emitLoad(value, regT1, regT0);
    
    if (Heap::isWriteBarrierEnabled()) {
        move(TrustedImmPtr(globalObject), regT2);
        
        emitWriteBarrier(globalObject, regT1, regT3, ShouldFilterImmediates, WriteBarrierForVariableAccess);
    }

    store32(regT1, registerPointer->tagPointer());
    store32(regT0, registerPointer->payloadPointer());
    map(m_bytecodeOffset + OPCODE_LENGTH(op_init_global_const), value, regT1, regT0);
}

void JIT::emit_op_init_global_const_check(Instruction* currentInstruction)
{
    WriteBarrier<Unknown>* registerPointer = currentInstruction[1].u.registerPointer;
    int value = currentInstruction[2].u.operand;
    
    JSGlobalObject* globalObject = m_codeBlock->globalObject();
    
    emitLoad(value, regT1, regT0);
    
    addSlowCase(branchTest8(NonZero, AbsoluteAddress(currentInstruction[3].u.predicatePointer)));
    
    if (Heap::isWriteBarrierEnabled()) {
        move(TrustedImmPtr(globalObject), regT2);
        emitWriteBarrier(globalObject, regT1, regT3, ShouldFilterImmediates, WriteBarrierForVariableAccess);
    }
    
    store32(regT1, registerPointer->tagPointer());
    store32(regT0, registerPointer->payloadPointer());
    unmap();
}

void JIT::emitSlow_op_init_global_const_check(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);
    
    JITStubCall stubCall(this, cti_op_init_global_const_check);
    stubCall.addArgument(regT1, regT0);
    stubCall.addArgument(TrustedImm32(currentInstruction[4].u.operand));
    stubCall.call();
}

void JIT::resetPatchGetById(RepatchBuffer& repatchBuffer, StructureStubInfo* stubInfo)
{
    repatchBuffer.relink(stubInfo->callReturnLocation, cti_op_get_by_id);
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelPtrAtOffset(stubInfo->patch.baseline.u.get.structureToCompare), reinterpret_cast<void*>(unusedPointer));
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelCompactAtOffset(stubInfo->patch.baseline.u.get.displacementLabel1), 0);
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelCompactAtOffset(stubInfo->patch.baseline.u.get.displacementLabel2), 0);
    repatchBuffer.relink(stubInfo->hotPathBegin.jumpAtOffset(stubInfo->patch.baseline.u.get.structureCheck), stubInfo->callReturnLocation.labelAtOffset(-stubInfo->patch.baseline.u.get.coldPathBegin));
}

void JIT::resetPatchPutById(RepatchBuffer& repatchBuffer, StructureStubInfo* stubInfo)
{
    if (isDirectPutById(stubInfo))
        repatchBuffer.relink(stubInfo->callReturnLocation, cti_op_put_by_id_direct);
    else
        repatchBuffer.relink(stubInfo->callReturnLocation, cti_op_put_by_id);
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelPtrAtOffset(stubInfo->patch.baseline.u.put.structureToCompare), reinterpret_cast<void*>(unusedPointer));
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabel32AtOffset(stubInfo->patch.baseline.u.put.displacementLabel1), 0);
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabel32AtOffset(stubInfo->patch.baseline.u.put.displacementLabel2), 0);
}

} // namespace JSC

#endif // USE(JSVALUE32_64)
#endif // ENABLE(JIT)
