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
#include "GCAwareJITStubRoutine.h"
#include "GetterSetter.h"
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
#if USE(JSVALUE64)

JIT::CodeRef JIT::stringGetByValStubGenerator(VM* vm)
{
    JSInterfaceJIT jit;
    JumpList failures;
    failures.append(jit.branchPtr(NotEqual, Address(regT0, JSCell::structureOffset()), TrustedImmPtr(vm->stringStructure.get())));

    // Load string length to regT2, and start the process of loading the data pointer into regT0
    jit.load32(Address(regT0, ThunkHelpers::jsStringLengthOffset()), regT2);
    jit.loadPtr(Address(regT0, ThunkHelpers::jsStringValueOffset()), regT0);
    failures.append(jit.branchTest32(Zero, regT0));

    // Do an unsigned compare to simultaneously filter negative indices as well as indices that are too large
    failures.append(jit.branch32(AboveOrEqual, regT1, regT2));
    
    // Load the character
    JumpList is16Bit;
    JumpList cont8Bit;
    // Load the string flags
    jit.loadPtr(Address(regT0, StringImpl::flagsOffset()), regT2);
    jit.loadPtr(Address(regT0, StringImpl::dataOffset()), regT0);
    is16Bit.append(jit.branchTest32(Zero, regT2, TrustedImm32(StringImpl::flagIs8Bit())));
    jit.load8(BaseIndex(regT0, regT1, TimesOne, 0), regT0);
    cont8Bit.append(jit.jump());
    is16Bit.link(&jit);
    jit.load16(BaseIndex(regT0, regT1, TimesTwo, 0), regT0);
    cont8Bit.link(&jit);

    failures.append(jit.branch32(AboveOrEqual, regT0, TrustedImm32(0x100)));
    jit.move(TrustedImmPtr(vm->smallStrings.singleCharacterStrings()), regT1);
    jit.loadPtr(BaseIndex(regT1, regT0, ScalePtr, 0), regT0);
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
    loadPtr(Address(regT0, JSCell::structureOffset()), regT2);
    emitArrayProfilingSite(regT2, regT3, profile);
    and32(TrustedImm32(IndexingShapeMask), regT2);

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
        break;
    }
    
    addSlowCase(badType);
    addSlowCase(slowCases);
    
    Label done = label();
    
#if !ASSERT_DISABLED
    Jump resultOK = branchTest64(NonZero, regT0);
    breakpoint();
    resultOK.link(this);
#endif

    emitValueProfilingSite();
    emitPutVirtualRegister(dst);
    
    m_byValCompilationInfo.append(ByValCompilationInfo(m_bytecodeOffset, badType, mode, done));
}

JIT::JumpList JIT::emitDoubleGetByVal(Instruction*, PatchableJump& badType)
{
    JumpList slowCases;
    
    badType = patchableBranch32(NotEqual, regT2, TrustedImm32(DoubleShape));
    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT2);
    slowCases.append(branch32(AboveOrEqual, regT1, Address(regT2, Butterfly::offsetOfPublicLength())));
    loadDouble(BaseIndex(regT2, regT1, TimesEight), fpRegT0);
    slowCases.append(branchDouble(DoubleNotEqualOrUnordered, fpRegT0, fpRegT0));
    moveDoubleTo64(fpRegT0, regT0);
    sub64(tagTypeNumberRegister, regT0);
    
    return slowCases;
}

JIT::JumpList JIT::emitContiguousGetByVal(Instruction*, PatchableJump& badType, IndexingType expectedShape)
{
    JumpList slowCases;
    
    badType = patchableBranch32(NotEqual, regT2, TrustedImm32(expectedShape));
    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT2);
    slowCases.append(branch32(AboveOrEqual, regT1, Address(regT2, Butterfly::offsetOfPublicLength())));
    load64(BaseIndex(regT2, regT1, TimesEight), regT0);
    slowCases.append(branchTest64(Zero, regT0));
    
    return slowCases;
}

JIT::JumpList JIT::emitArrayStorageGetByVal(Instruction*, PatchableJump& badType)
{
    JumpList slowCases;

    add32(TrustedImm32(-ArrayStorageShape), regT2, regT3);
    badType = patchableBranch32(Above, regT3, TrustedImm32(SlowPutArrayStorageShape - ArrayStorageShape));

    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT2);
    slowCases.append(branch32(AboveOrEqual, regT1, Address(regT2, ArrayStorage::vectorLengthOffset())));

    load64(BaseIndex(regT2, regT1, TimesEight, ArrayStorage::vectorOffset()), regT0);
    slowCases.append(branchTest64(Zero, regT0));
    
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
    emitNakedCall(CodeLocationLabel(m_vm->getCTIStub(stringGetByValStubGenerator).code()));
    Jump failed = branchTest64(Zero, regT0);
    emitPutVirtualRegister(dst, regT0);
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
    stubCall.addArgument(base, regT2);
    stubCall.addArgument(property, regT2);
    Call call = stubCall.call(dst);

    m_byValCompilationInfo[m_byValInstructionIndex].slowPathTarget = slowPath;
    m_byValCompilationInfo[m_byValInstructionIndex].returnAddress = call;
    m_byValInstructionIndex++;

    emitValueProfilingSite();
}

void JIT::compileGetDirectOffset(RegisterID base, RegisterID result, RegisterID offset, RegisterID scratch, FinalObjectMode finalObjectMode)
{
    ASSERT(sizeof(JSValue) == 8);
    
    if (finalObjectMode == MayBeFinal) {
        Jump isInline = branch32(LessThan, offset, TrustedImm32(firstOutOfLineOffset));
        loadPtr(Address(base, JSObject::butterflyOffset()), scratch);
        neg32(offset);
        Jump done = jump();
        isInline.link(this);
        addPtr(TrustedImm32(JSObject::offsetOfInlineStorage() - (firstOutOfLineOffset - 2) * sizeof(EncodedJSValue)), base, scratch);
        done.link(this);
    } else {
#if !ASSERT_DISABLED
        Jump isOutOfLine = branch32(GreaterThanOrEqual, offset, TrustedImm32(firstOutOfLineOffset));
        breakpoint();
        isOutOfLine.link(this);
#endif
        loadPtr(Address(base, JSObject::butterflyOffset()), scratch);
        neg32(offset);
    }
    signExtend32ToPtr(offset, offset);
    load64(BaseIndex(scratch, offset, TimesEight, (firstOutOfLineOffset - 2) * sizeof(EncodedJSValue)), result);
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
    addSlowCase(branch64(NotEqual, regT0, addressFor(expected)));
    emitGetVirtualRegisters(base, regT0, iter, regT1);
    emitJumpSlowCaseIfNotJSCell(regT0, base);

    // Test base's structure
    loadPtr(Address(regT0, JSCell::structureOffset()), regT2);
    addSlowCase(branchPtr(NotEqual, regT2, Address(regT1, OBJECT_OFFSETOF(JSPropertyNameIterator, m_cachedStructure))));
    load32(addressFor(i), regT3);
    sub32(TrustedImm32(1), regT3);
    addSlowCase(branch32(AboveOrEqual, regT3, Address(regT1, OBJECT_OFFSETOF(JSPropertyNameIterator, m_numCacheableSlots))));
    Jump inlineProperty = branch32(Below, regT3, Address(regT1, OBJECT_OFFSETOF(JSPropertyNameIterator, m_cachedStructureInlineCapacity)));
    add32(TrustedImm32(firstOutOfLineOffset), regT3);
    sub32(Address(regT1, OBJECT_OFFSETOF(JSPropertyNameIterator, m_cachedStructureInlineCapacity)), regT3);
    inlineProperty.link(this);
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

    JITStubCall stubCall(this, cti_op_get_by_val_generic);
    stubCall.addArgument(base, regT2);
    stubCall.addArgument(property, regT2);
    stubCall.call(dst);
}

void JIT::emit_op_put_by_val(Instruction* currentInstruction)
{
    unsigned base = currentInstruction[1].u.operand;
    unsigned property = currentInstruction[2].u.operand;
    ArrayProfile* profile = currentInstruction[4].u.arrayProfile;

    emitGetVirtualRegisters(base, regT0, property, regT1);
    emitJumpSlowCaseIfNotImmediateInteger(regT1);
    // See comment in op_get_by_val.
    zeroExtend32ToPtr(regT1, regT1);
    emitJumpSlowCaseIfNotJSCell(regT0, base);
    loadPtr(Address(regT0, JSCell::structureOffset()), regT2);
    emitArrayProfilingSite(regT2, regT3, profile);
    and32(TrustedImm32(IndexingShapeMask), regT2);
    
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

    emitWriteBarrier(regT0, regT3, regT1, regT3, ShouldFilterImmediates, WriteBarrierForPropertyAccess);
}

JIT::JumpList JIT::emitGenericContiguousPutByVal(Instruction* currentInstruction, PatchableJump& badType, IndexingType indexingShape)
{
    unsigned value = currentInstruction[3].u.operand;
    ArrayProfile* profile = currentInstruction[4].u.arrayProfile;
    
    JumpList slowCases;

    badType = patchableBranch32(NotEqual, regT2, TrustedImm32(indexingShape));
    
    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT2);
    Jump outOfBounds = branch32(AboveOrEqual, regT1, Address(regT2, Butterfly::offsetOfPublicLength()));

    Label storeResult = label();
    emitGetVirtualRegister(value, regT3);
    switch (indexingShape) {
    case Int32Shape:
        slowCases.append(emitJumpIfNotImmediateInteger(regT3));
        store64(regT3, BaseIndex(regT2, regT1, TimesEight));
        break;
    case DoubleShape: {
        Jump notInt = emitJumpIfNotImmediateInteger(regT3);
        convertInt32ToDouble(regT3, fpRegT0);
        Jump ready = jump();
        notInt.link(this);
        add64(tagTypeNumberRegister, regT3);
        move64ToDouble(regT3, fpRegT0);
        slowCases.append(branchDouble(DoubleNotEqualOrUnordered, fpRegT0, fpRegT0));
        ready.link(this);
        storeDouble(fpRegT0, BaseIndex(regT2, regT1, TimesEight));
        break;
    }
    case ContiguousShape:
        store64(regT3, BaseIndex(regT2, regT1, TimesEight));
        break;
    default:
        CRASH();
        break;
    }
    
    Jump done = jump();
    outOfBounds.link(this);
    
    slowCases.append(branch32(AboveOrEqual, regT1, Address(regT2, Butterfly::offsetOfVectorLength())));
    
    emitArrayProfileStoreToHoleSpecialCase(profile);
    
    add32(TrustedImm32(1), regT1, regT3);
    store32(regT3, Address(regT2, Butterfly::offsetOfPublicLength()));
    jump().linkTo(storeResult, this);
    
    done.link(this);
    
    return slowCases;
}

JIT::JumpList JIT::emitArrayStoragePutByVal(Instruction* currentInstruction, PatchableJump& badType)
{
    unsigned value = currentInstruction[3].u.operand;
    ArrayProfile* profile = currentInstruction[4].u.arrayProfile;
    
    JumpList slowCases;
    
    badType = patchableBranch32(NotEqual, regT2, TrustedImm32(ArrayStorageShape));
    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT2);
    slowCases.append(branch32(AboveOrEqual, regT1, Address(regT2, ArrayStorage::vectorLengthOffset())));

    Jump empty = branchTest64(Zero, BaseIndex(regT2, regT1, TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])));

    Label storeResult(this);
    emitGetVirtualRegister(value, regT3);
    store64(regT3, BaseIndex(regT2, regT1, TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])));
    Jump end = jump();
    
    empty.link(this);
    emitArrayProfileStoreToHoleSpecialCase(profile);
    add32(TrustedImm32(1), Address(regT2, ArrayStorage::numValuesInVectorOffset()));
    branch32(Below, regT1, Address(regT2, ArrayStorage::lengthOffset())).linkTo(storeResult, this);

    add32(TrustedImm32(1), regT1);
    store32(regT1, Address(regT2, ArrayStorage::lengthOffset()));
    sub32(TrustedImm32(1), regT1);
    jump().linkTo(storeResult, this);

    end.link(this);
    
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
    stubPutByValCall.addArgument(regT0);
    stubPutByValCall.addArgument(property, regT2);
    stubPutByValCall.addArgument(value, regT2);
    Call call = stubPutByValCall.call();

    m_byValCompilationInfo[m_byValInstructionIndex].slowPathTarget = slowPath;
    m_byValCompilationInfo[m_byValInstructionIndex].returnAddress = call;
    m_byValInstructionIndex++;
}

void JIT::emit_op_put_by_index(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_put_by_index);
    stubCall.addArgument(currentInstruction[1].u.operand, regT2);
    stubCall.addArgument(TrustedImm32(currentInstruction[2].u.operand));
    stubCall.addArgument(currentInstruction[3].u.operand, regT2);
    stubCall.call();
}

void JIT::emit_op_put_getter_setter(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_put_getter_setter);
    stubCall.addArgument(currentInstruction[1].u.operand, regT2);
    stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(currentInstruction[2].u.operand)));
    stubCall.addArgument(currentInstruction[3].u.operand, regT2);
    stubCall.addArgument(currentInstruction[4].u.operand, regT2);
    stubCall.call();
}

void JIT::emit_op_del_by_id(Instruction* currentInstruction)
{
    JITStubCall stubCall(this, cti_op_del_by_id);
    stubCall.addArgument(currentInstruction[2].u.operand, regT2);
    stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(currentInstruction[3].u.operand)));
    stubCall.call(currentInstruction[1].u.operand);
}

void JIT::emit_op_get_by_id(Instruction* currentInstruction)
{
    unsigned resultVReg = currentInstruction[1].u.operand;
    unsigned baseVReg = currentInstruction[2].u.operand;
    Identifier* ident = &(m_codeBlock->identifier(currentInstruction[3].u.operand));

    emitGetVirtualRegister(baseVReg, regT0);
    compileGetByIdHotPath(baseVReg, ident);
    emitValueProfilingSite();
    emitPutVirtualRegister(resultVReg);
}

void JIT::compileGetByIdHotPath(int baseVReg, Identifier* ident)
{
    // As for put_by_id, get_by_id requires the offset of the Structure and the offset of the access to be patched.
    // Additionally, for get_by_id we need patch the offset of the branch to the slow case (we patch this to jump
    // to array-length / prototype access tranpolines, and finally we also the the property-map access offset as a label
    // to jump back to if one of these trampolies finds a match.

    emitJumpSlowCaseIfNotJSCell(regT0, baseVReg);
    
    if (*ident == m_vm->propertyNames->length && shouldEmitProfiling()) {
        loadPtr(Address(regT0, JSCell::structureOffset()), regT1);
        emitArrayProfilingSiteForBytecodeIndex(regT1, regT2, m_bytecodeOffset);
    }

    BEGIN_UNINTERRUPTED_SEQUENCE(sequenceGetByIdHotPath);

    Label hotPathBegin(this);

    DataLabelPtr structureToCompare;
    PatchableJump structureCheck = patchableBranchPtrWithPatch(NotEqual, Address(regT0, JSCell::structureOffset()), structureToCompare, TrustedImmPtr(reinterpret_cast<void*>(patchGetByIdDefaultStructure)));
    addSlowCase(structureCheck);

    ConvertibleLoadLabel propertyStorageLoad = convertibleLoadPtr(Address(regT0, JSObject::butterflyOffset()), regT0);
    DataLabelCompact displacementLabel = load64WithCompactAddressOffsetPatch(Address(regT0, patchGetByIdDefaultOffset), regT0);

    Label putResult(this);

    END_UNINTERRUPTED_SEQUENCE(sequenceGetByIdHotPath);

    m_propertyAccessCompilationInfo.append(PropertyStubCompilationInfo(PropertyStubGetById, m_bytecodeOffset, hotPathBegin, structureToCompare, structureCheck, propertyStorageLoad, displacementLabel, putResult));
}

void JIT::emitSlow_op_get_by_id(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned resultVReg = currentInstruction[1].u.operand;
    unsigned baseVReg = currentInstruction[2].u.operand;
    Identifier* ident = &(m_codeBlock->identifier(currentInstruction[3].u.operand));

    compileGetByIdSlowCase(resultVReg, baseVReg, ident, iter);
    emitValueProfilingSite();
}

void JIT::compileGetByIdSlowCase(int resultVReg, int baseVReg, Identifier* ident, Vector<SlowCaseEntry>::iterator& iter)
{
    // As for the hot path of get_by_id, above, we ensure that we can use an architecture specific offset
    // so that we only need track one pointer into the slow case code - we track a pointer to the location
    // of the call (which we can use to look up the patch information), but should a array-length or
    // prototype access trampoline fail we want to bail out back to here.  To do so we can subtract back
    // the distance from the call to the head of the slow case.

    linkSlowCaseIfNotJSCell(iter, baseVReg);
    linkSlowCase(iter);

    BEGIN_UNINTERRUPTED_SEQUENCE(sequenceGetByIdSlowCase);

    Label coldPathBegin(this);
    JITStubCall stubCall(this, cti_op_get_by_id);
    stubCall.addArgument(regT0);
    stubCall.addArgument(TrustedImmPtr(ident));
    Call call = stubCall.call(resultVReg);

    END_UNINTERRUPTED_SEQUENCE(sequenceGetByIdSlowCase);

    // Track the location of the call; this will be used to recover patch information.
    m_propertyAccessCompilationInfo[m_propertyAccessInstructionIndex++].slowCaseInfo(PropertyStubGetById, coldPathBegin, call);
}

void JIT::emit_op_put_by_id(Instruction* currentInstruction)
{
    unsigned baseVReg = currentInstruction[1].u.operand;
    unsigned valueVReg = currentInstruction[3].u.operand;

    // In order to be able to patch both the Structure, and the object offset, we store one pointer,
    // to just after the arguments have been loaded into registers 'hotPathBegin', and we generate code
    // such that the Structure & offset are always at the same distance from this.

    emitGetVirtualRegisters(baseVReg, regT0, valueVReg, regT1);

    // Jump to a slow case if either the base object is an immediate, or if the Structure does not match.
    emitJumpSlowCaseIfNotJSCell(regT0, baseVReg);

    BEGIN_UNINTERRUPTED_SEQUENCE(sequencePutById);

    Label hotPathBegin(this);

    // It is important that the following instruction plants a 32bit immediate, in order that it can be patched over.
    DataLabelPtr structureToCompare;
    addSlowCase(branchPtrWithPatch(NotEqual, Address(regT0, JSCell::structureOffset()), structureToCompare, TrustedImmPtr(reinterpret_cast<void*>(patchGetByIdDefaultStructure))));

    ConvertibleLoadLabel propertyStorageLoad = convertibleLoadPtr(Address(regT0, JSObject::butterflyOffset()), regT2);
    DataLabel32 displacementLabel = store64WithAddressOffsetPatch(regT1, Address(regT2, patchPutByIdDefaultOffset));

    END_UNINTERRUPTED_SEQUENCE(sequencePutById);

    emitWriteBarrier(regT0, regT1, regT2, regT3, ShouldFilterImmediates, WriteBarrierForPropertyAccess);

    m_propertyAccessCompilationInfo.append(PropertyStubCompilationInfo(PropertyStubPutById, m_bytecodeOffset, hotPathBegin, structureToCompare, propertyStorageLoad, displacementLabel));
}

void JIT::emitSlow_op_put_by_id(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned baseVReg = currentInstruction[1].u.operand;
    Identifier* ident = &(m_codeBlock->identifier(currentInstruction[2].u.operand));
    unsigned direct = currentInstruction[8].u.operand;

    linkSlowCaseIfNotJSCell(iter, baseVReg);
    linkSlowCase(iter);

    JITStubCall stubCall(this, direct ? cti_op_put_by_id_direct : cti_op_put_by_id);
    stubCall.addArgument(regT0);
    stubCall.addArgument(TrustedImmPtr(ident));
    stubCall.addArgument(regT1);
    move(regT0, nonArgGPR1);
    Call call = stubCall.call();

    // Track the location of the call; this will be used to recover patch information.
    m_propertyAccessCompilationInfo[m_propertyAccessInstructionIndex++].slowCaseInfo(PropertyStubPutById, call);
}

// Compile a store into an object's property storage.  May overwrite the
// value in objectReg.
void JIT::compilePutDirectOffset(RegisterID base, RegisterID value, PropertyOffset cachedOffset)
{
    if (isInlineOffset(cachedOffset)) {
        store64(value, Address(base, JSObject::offsetOfInlineStorage() + sizeof(JSValue) * offsetInInlineStorage(cachedOffset)));
        return;
    }
    
    loadPtr(Address(base, JSObject::butterflyOffset()), base);
    store64(value, Address(base, sizeof(JSValue) * offsetInButterfly(cachedOffset)));
}

// Compile a load from an object's property storage.  May overwrite base.
void JIT::compileGetDirectOffset(RegisterID base, RegisterID result, PropertyOffset cachedOffset)
{
    if (isInlineOffset(cachedOffset)) {
        load64(Address(base, JSObject::offsetOfInlineStorage() + sizeof(JSValue) * offsetInInlineStorage(cachedOffset)), result);
        return;
    }
    
    loadPtr(Address(base, JSObject::butterflyOffset()), result);
    load64(Address(result, sizeof(JSValue) * offsetInButterfly(cachedOffset)), result);
}

void JIT::compileGetDirectOffset(JSObject* base, RegisterID result, PropertyOffset cachedOffset)
{
    if (isInlineOffset(cachedOffset)) {
        load64(base->locationForOffset(cachedOffset), result);
        return;
    }
    
    loadPtr(base->butterflyAddress(), result);
    load64(Address(result, offsetInButterfly(cachedOffset) * sizeof(WriteBarrier<Unknown>)), result);
}

void JIT::privateCompilePutByIdTransition(StructureStubInfo* stubInfo, Structure* oldStructure, Structure* newStructure, PropertyOffset cachedOffset, StructureChain* chain, ReturnAddressPtr returnAddress, bool direct)
{
    move(nonArgGPR1, regT0);

    JumpList failureCases;
    // Check eax is an object of the right Structure.
    failureCases.append(emitJumpIfNotJSCell(regT0));
    failureCases.append(branchPtr(NotEqual, Address(regT0, JSCell::structureOffset()), TrustedImmPtr(oldStructure)));
    
    testPrototype(oldStructure->storedPrototype(), failureCases, stubInfo);
    
    ASSERT(oldStructure->storedPrototype().isNull() || oldStructure->storedPrototype().asCell()->structure() == chain->head()->get());

    // ecx = baseObject->m_structure
    if (!direct) {
        for (WriteBarrier<Structure>* it = chain->head(); *it; ++it) {
            ASSERT((*it)->storedPrototype().isNull() || (*it)->storedPrototype().asCell()->structure() == it[1].get());
            testPrototype((*it)->storedPrototype(), failureCases, stubInfo);
        }
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
    
    // emit a call only if storage realloc is needed
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
        emitGetJITStubArg(2, regT1);

        restoreReturnAddressBeforeReturn(regT3);
    }

    // Planting the new structure triggers the write barrier so we need
    // an unconditional barrier here.
    emitWriteBarrier(regT0, regT1, regT2, regT3, UnconditionalWriteBarrier, WriteBarrierForPropertyAccess);

    ASSERT(newStructure->classInfo() == oldStructure->classInfo());
    storePtr(TrustedImmPtr(newStructure), Address(regT0, JSCell::structureOffset()));
    compilePutDirectOffset(regT0, regT1, cachedOffset);

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
            ("Baseline put_by_id transition for %s, return point %p",
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
    // Should probably go to cti_op_get_by_id_fail, but that doesn't do anything interesting right now.
    repatchBuffer.relinkCallerToFunction(returnAddress, FunctionPtr(cti_op_get_by_id_self_fail));

    // Patch the offset into the propoerty map to load from, then patch the Structure to look for.
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelPtrAtOffset(stubInfo->patch.baseline.u.get.structureToCompare), structure);
    repatchBuffer.setLoadInstructionIsActive(stubInfo->hotPathBegin.convertibleLoadAtOffset(stubInfo->patch.baseline.u.get.propertyStorageLoad), isOutOfLineOffset(cachedOffset));
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelCompactAtOffset(stubInfo->patch.baseline.u.get.displacementLabel), offsetRelativeToPatchedStorage(cachedOffset));
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
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabel32AtOffset(stubInfo->patch.baseline.u.put.displacementLabel), offsetRelativeToPatchedStorage(cachedOffset));
}

void JIT::privateCompilePatchGetArrayLength(ReturnAddressPtr returnAddress)
{
    StructureStubInfo* stubInfo = &m_codeBlock->getStubInfo(returnAddress);

    // Check eax is an array
    loadPtr(Address(regT0, JSCell::structureOffset()), regT2);
    Jump failureCases1 = branchTest32(Zero, regT2, TrustedImm32(IsArray));
    Jump failureCases2 = branchTest32(Zero, regT2, TrustedImm32(IndexingShapeMask));

    // Checks out okay! - get the length from the storage
    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT3);
    load32(Address(regT3, ArrayStorage::lengthOffset()), regT2);
    Jump failureCases3 = branch32(LessThan, regT2, TrustedImm32(0));

    emitFastArithIntToImmNoCheck(regT2, regT0);
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
        ("Basline JIT get_by_id array length stub for %s, return point %p",
            toCString(*m_codeBlock).data(),
            stubInfo->hotPathBegin.labelAtOffset(
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
    // The prototype object definitely exists (if this stub exists the CodeBlock is referencing a Structure that is
    // referencing the prototype object - let's speculatively load it's table nice and early!)
    JSObject* protoObject = asObject(structure->prototypeForLookup(callFrame));

    // Check eax is an object of the right Structure.
    Jump failureCases1 = checkStructure(regT0, structure);

    // Check the prototype object's Structure had not changed.
    Jump failureCases2 = addStructureTransitionCheck(protoObject, prototypeStructure, stubInfo, regT3);

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
            ("Baseline JIT get_by_id proto stub for %s, return point %p",
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
    Jump failureCase = checkStructure(regT0, structure);
    bool needsStubLink = false;
    bool isDirect = false;
    if (slot.cachedPropertyType() == PropertySlot::Getter) {
        needsStubLink = true;
        compileGetDirectOffset(regT0, regT1, cachedOffset);
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
        compileGetDirectOffset(regT0, regT0, cachedOffset);
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

    RefPtr<JITStubRoutine> stubCode = createJITStubRoutine(
        FINALIZE_CODE(
            patchBuffer,
            ("Baseline JIT get_by_id list stub for %s, return point %p",
                toCString(*m_codeBlock).data(), stubInfo->hotPathBegin.labelAtOffset(
                    stubInfo->patch.baseline.u.get.putResult).executableAddress())),
        *m_vm,
        m_codeBlock->ownerExecutable(),
        needsStubLink);

    polymorphicStructures->list[currentIndex].set(*m_vm, m_codeBlock->ownerExecutable(), stubCode, structure, isDirect);

    // Finally patch the jump to slow case back in the hot path to jump here instead.
    CodeLocationJump jumpLocation = stubInfo->hotPathBegin.jumpAtOffset(stubInfo->patch.baseline.u.get.structureCheck);
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(jumpLocation, CodeLocationLabel(stubCode->code().code()));
}

void JIT::privateCompileGetByIdProtoList(StructureStubInfo* stubInfo, PolymorphicAccessStructureList* prototypeStructures, int currentIndex, Structure* structure, Structure* prototypeStructure, const Identifier& ident, const PropertySlot& slot, PropertyOffset cachedOffset, CallFrame* callFrame)
{
    // The prototype object definitely exists (if this stub exists the CodeBlock is referencing a Structure that is
    // referencing the prototype object - let's speculatively load it's table nice and early!)
    JSObject* protoObject = asObject(structure->prototypeForLookup(callFrame));

    // Check eax is an object of the right Structure.
    Jump failureCases1 = checkStructure(regT0, structure);

    // Check the prototype object's Structure had not changed.
    Jump failureCases2 = addStructureTransitionCheck(protoObject, prototypeStructure, stubInfo, regT3);

    // Checks out okay!
    bool needsStubLink = false;
    bool isDirect = false;
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
    } else {
        isDirect = true;
        compileGetDirectOffset(protoObject, regT0, cachedOffset);
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

    RefPtr<JITStubRoutine> stubCode = createJITStubRoutine(
        FINALIZE_CODE(
            patchBuffer,
            ("Baseline JIT get_by_id proto list stub for %s, return point %p",
                toCString(*m_codeBlock).data(), stubInfo->hotPathBegin.labelAtOffset(
                    stubInfo->patch.baseline.u.get.putResult).executableAddress())),
        *m_vm,
        m_codeBlock->ownerExecutable(),
        needsStubLink);
    prototypeStructures->list[currentIndex].set(*m_vm, m_codeBlock->ownerExecutable(), stubCode, structure, prototypeStructure, isDirect);

    // Finally patch the jump to slow case back in the hot path to jump here instead.
    CodeLocationJump jumpLocation = stubInfo->hotPathBegin.jumpAtOffset(stubInfo->patch.baseline.u.get.structureCheck);
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(jumpLocation, CodeLocationLabel(stubCode->code().code()));
}

void JIT::privateCompileGetByIdChainList(StructureStubInfo* stubInfo, PolymorphicAccessStructureList* prototypeStructures, int currentIndex, Structure* structure, StructureChain* chain, size_t count, const Identifier& ident, const PropertySlot& slot, PropertyOffset cachedOffset, CallFrame* callFrame)
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
        testPrototype(protoObject, bucketsOfFail, stubInfo);
    }
    ASSERT(protoObject);
    
    bool needsStubLink = false;
    bool isDirect = false;
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
    } else {
        isDirect = true;
        compileGetDirectOffset(protoObject, regT0, cachedOffset);
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
            ("Baseline JIT get_by_id chain list stub for %s, return point %p",
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
            ("Baseline JIT get_by_id chain stub for %s, return point %p",
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

void JIT::emit_op_get_scoped_var(Instruction* currentInstruction)
{
    int skip = currentInstruction[3].u.operand;

    emitGetFromCallFrameHeaderPtr(JSStack::ScopeChain, regT0);
    bool checkTopLevel = m_codeBlock->codeType() == FunctionCode && m_codeBlock->needsFullScopeChain();
    ASSERT(skip || !checkTopLevel);
    if (checkTopLevel && skip--) {
        Jump activationNotCreated;
        if (checkTopLevel)
            activationNotCreated = branchTestPtr(Zero, addressFor(m_codeBlock->activationRegister()));
        loadPtr(Address(regT0, JSScope::offsetOfNext()), regT0);
        activationNotCreated.link(this);
    }
    while (skip--)
        loadPtr(Address(regT0, JSScope::offsetOfNext()), regT0);

    loadPtr(Address(regT0, JSVariableObject::offsetOfRegisters()), regT0);
    loadPtr(Address(regT0, currentInstruction[2].u.operand * sizeof(Register)), regT0);
    emitValueProfilingSite();
    emitPutVirtualRegister(currentInstruction[1].u.operand);
}

void JIT::emit_op_put_scoped_var(Instruction* currentInstruction)
{
    int skip = currentInstruction[2].u.operand;

    emitGetVirtualRegister(currentInstruction[3].u.operand, regT0);

    emitGetFromCallFrameHeaderPtr(JSStack::ScopeChain, regT1);
    bool checkTopLevel = m_codeBlock->codeType() == FunctionCode && m_codeBlock->needsFullScopeChain();
    ASSERT(skip || !checkTopLevel);
    if (checkTopLevel && skip--) {
        Jump activationNotCreated;
        if (checkTopLevel)
            activationNotCreated = branchTestPtr(Zero, addressFor(m_codeBlock->activationRegister()));
        loadPtr(Address(regT1, JSScope::offsetOfNext()), regT1);
        activationNotCreated.link(this);
    }
    while (skip--)
        loadPtr(Address(regT1, JSScope::offsetOfNext()), regT1);

    emitWriteBarrier(regT1, regT0, regT2, regT3, ShouldFilterImmediates, WriteBarrierForVariableAccess);

    loadPtr(Address(regT1, JSVariableObject::offsetOfRegisters()), regT1);
    storePtr(regT0, Address(regT1, currentInstruction[1].u.operand * sizeof(Register)));
}

void JIT::emit_op_init_global_const(Instruction* currentInstruction)
{
    JSGlobalObject* globalObject = m_codeBlock->globalObject();

    emitGetVirtualRegister(currentInstruction[2].u.operand, regT0);

    store64(regT0, currentInstruction[1].u.registerPointer);
    if (Heap::isWriteBarrierEnabled())
        emitWriteBarrier(globalObject, regT0, regT2, ShouldFilterImmediates, WriteBarrierForVariableAccess);
}

void JIT::emit_op_init_global_const_check(Instruction* currentInstruction)
{
    emitGetVirtualRegister(currentInstruction[2].u.operand, regT0);

    addSlowCase(branchTest8(NonZero, AbsoluteAddress(currentInstruction[3].u.predicatePointer)));

    JSGlobalObject* globalObject = m_codeBlock->globalObject();

    store64(regT0, currentInstruction[1].u.registerPointer);
    if (Heap::isWriteBarrierEnabled())
        emitWriteBarrier(globalObject, regT0, regT2, ShouldFilterImmediates, WriteBarrierForVariableAccess);
}

void JIT::emitSlow_op_init_global_const_check(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);

    JITStubCall stubCall(this, cti_op_init_global_const_check);
    stubCall.addArgument(regT0);
    stubCall.addArgument(TrustedImm32(currentInstruction[4].u.operand));
    stubCall.call();
}

void JIT::resetPatchGetById(RepatchBuffer& repatchBuffer, StructureStubInfo* stubInfo)
{
    repatchBuffer.relink(stubInfo->callReturnLocation, cti_op_get_by_id);
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelPtrAtOffset(stubInfo->patch.baseline.u.get.structureToCompare), reinterpret_cast<void*>(unusedPointer));
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelCompactAtOffset(stubInfo->patch.baseline.u.get.displacementLabel), 0);
    repatchBuffer.relink(stubInfo->hotPathBegin.jumpAtOffset(stubInfo->patch.baseline.u.get.structureCheck), stubInfo->callReturnLocation.labelAtOffset(-stubInfo->patch.baseline.u.get.coldPathBegin));
}

void JIT::resetPatchPutById(RepatchBuffer& repatchBuffer, StructureStubInfo* stubInfo)
{
    if (isDirectPutById(stubInfo))
        repatchBuffer.relink(stubInfo->callReturnLocation, cti_op_put_by_id_direct);
    else
        repatchBuffer.relink(stubInfo->callReturnLocation, cti_op_put_by_id);
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelPtrAtOffset(stubInfo->patch.baseline.u.put.structureToCompare), reinterpret_cast<void*>(unusedPointer));
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabel32AtOffset(stubInfo->patch.baseline.u.put.displacementLabel), 0);
}

#endif // USE(JSVALUE64)

void JIT::emitWriteBarrier(RegisterID owner, RegisterID value, RegisterID scratch, RegisterID scratch2, WriteBarrierMode mode, WriteBarrierUseKind useKind)
{
    UNUSED_PARAM(owner);
    UNUSED_PARAM(scratch);
    UNUSED_PARAM(scratch2);
    UNUSED_PARAM(useKind);
    UNUSED_PARAM(value);
    UNUSED_PARAM(mode);
    ASSERT(owner != scratch);
    ASSERT(owner != scratch2);
    
#if ENABLE(WRITE_BARRIER_PROFILING)
    emitCount(WriteBarrierCounters::jitCounterFor(useKind));
#endif
}

void JIT::emitWriteBarrier(JSCell* owner, RegisterID value, RegisterID scratch, WriteBarrierMode mode, WriteBarrierUseKind useKind)
{
    UNUSED_PARAM(owner);
    UNUSED_PARAM(scratch);
    UNUSED_PARAM(useKind);
    UNUSED_PARAM(value);
    UNUSED_PARAM(mode);
    
#if ENABLE(WRITE_BARRIER_PROFILING)
    emitCount(WriteBarrierCounters::jitCounterFor(useKind));
#endif
}

JIT::Jump JIT::addStructureTransitionCheck(JSCell* object, Structure* structure, StructureStubInfo* stubInfo, RegisterID scratch)
{
    if (object->structure() == structure && structure->transitionWatchpointSetIsStillValid()) {
        structure->addTransitionWatchpoint(stubInfo->addWatchpoint(m_codeBlock));
#if !ASSERT_DISABLED
        move(TrustedImmPtr(object), scratch);
        Jump ok = branchPtr(Equal, Address(scratch, JSCell::structureOffset()), TrustedImmPtr(structure));
        breakpoint();
        ok.link(this);
#endif
        Jump result; // Returning an unset jump this way because otherwise VC++ would complain.
        return result;
    }
    
    move(TrustedImmPtr(object), scratch);
    return branchPtr(NotEqual, Address(scratch, JSCell::structureOffset()), TrustedImmPtr(structure));
}

void JIT::addStructureTransitionCheck(JSCell* object, Structure* structure, StructureStubInfo* stubInfo, JumpList& failureCases, RegisterID scratch)
{
    Jump failureCase = addStructureTransitionCheck(object, structure, stubInfo, scratch);
    if (!failureCase.isSet())
        return;
    
    failureCases.append(failureCase);
}

void JIT::testPrototype(JSValue prototype, JumpList& failureCases, StructureStubInfo* stubInfo)
{
    if (prototype.isNull())
        return;

    ASSERT(prototype.isCell());
    addStructureTransitionCheck(prototype.asCell(), prototype.asCell()->structure(), stubInfo, failureCases, regT3);
}

bool JIT::isDirectPutById(StructureStubInfo* stubInfo)
{
    switch (stubInfo->accessType) {
    case access_put_by_id_transition_normal:
        return false;
    case access_put_by_id_transition_direct:
        return true;
    case access_put_by_id_replace:
    case access_put_by_id_generic: {
        void* oldCall = MacroAssembler::readCallTarget(stubInfo->callReturnLocation).executableAddress();
        if (oldCall == bitwise_cast<void*>(cti_op_put_by_id_direct)
            || oldCall == bitwise_cast<void*>(cti_op_put_by_id_direct_generic)
            || oldCall == bitwise_cast<void*>(cti_op_put_by_id_direct_fail))
            return true;
        ASSERT(oldCall == bitwise_cast<void*>(cti_op_put_by_id)
               || oldCall == bitwise_cast<void*>(cti_op_put_by_id_generic)
               || oldCall == bitwise_cast<void*>(cti_op_put_by_id_fail));
        return false;
    }
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return false;
    }
}

void JIT::privateCompileGetByVal(ByValInfo* byValInfo, ReturnAddressPtr returnAddress, JITArrayMode arrayMode)
{
    Instruction* currentInstruction = m_codeBlock->instructions().begin() + byValInfo->bytecodeIndex;
    
    PatchableJump badType;
    JumpList slowCases;
    
    switch (arrayMode) {
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
    case JITInt8Array:
        slowCases = emitIntTypedArrayGetByVal(currentInstruction, badType, m_vm->int8ArrayDescriptor(), 1, SignedTypedArray);
        break;
    case JITInt16Array:
        slowCases = emitIntTypedArrayGetByVal(currentInstruction, badType, m_vm->int16ArrayDescriptor(), 2, SignedTypedArray);
        break;
    case JITInt32Array:
        slowCases = emitIntTypedArrayGetByVal(currentInstruction, badType, m_vm->int32ArrayDescriptor(), 4, SignedTypedArray);
        break;
    case JITUint8Array:
        slowCases = emitIntTypedArrayGetByVal(currentInstruction, badType, m_vm->uint8ArrayDescriptor(), 1, UnsignedTypedArray);
        break;
    case JITUint8ClampedArray:
        slowCases = emitIntTypedArrayGetByVal(currentInstruction, badType, m_vm->uint8ClampedArrayDescriptor(), 1, UnsignedTypedArray);
        break;
    case JITUint16Array:
        slowCases = emitIntTypedArrayGetByVal(currentInstruction, badType, m_vm->uint16ArrayDescriptor(), 2, UnsignedTypedArray);
        break;
    case JITUint32Array:
        slowCases = emitIntTypedArrayGetByVal(currentInstruction, badType, m_vm->uint32ArrayDescriptor(), 4, UnsignedTypedArray);
        break;
    case JITFloat32Array:
        slowCases = emitFloatTypedArrayGetByVal(currentInstruction, badType, m_vm->float32ArrayDescriptor(), 4);
        break;
    case JITFloat64Array:
        slowCases = emitFloatTypedArrayGetByVal(currentInstruction, badType, m_vm->float64ArrayDescriptor(), 8);
        break;
    default:
        CRASH();
    }
    
    Jump done = jump();

    LinkBuffer patchBuffer(*m_vm, this, m_codeBlock);
    
    patchBuffer.link(badType, CodeLocationLabel(MacroAssemblerCodePtr::createFromExecutableAddress(returnAddress.value())).labelAtOffset(byValInfo->returnAddressToSlowPath));
    patchBuffer.link(slowCases, CodeLocationLabel(MacroAssemblerCodePtr::createFromExecutableAddress(returnAddress.value())).labelAtOffset(byValInfo->returnAddressToSlowPath));
    
    patchBuffer.link(done, byValInfo->badTypeJump.labelAtOffset(byValInfo->badTypeJumpToDone));
    
    byValInfo->stubRoutine = FINALIZE_CODE_FOR_STUB(
        patchBuffer,
        ("Baseline get_by_val stub for %s, return point %p", toCString(*m_codeBlock).data(), returnAddress.value()));
    
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(byValInfo->badTypeJump, CodeLocationLabel(byValInfo->stubRoutine->code().code()));
    repatchBuffer.relinkCallerToFunction(returnAddress, FunctionPtr(cti_op_get_by_val_generic));
}

void JIT::privateCompilePutByVal(ByValInfo* byValInfo, ReturnAddressPtr returnAddress, JITArrayMode arrayMode)
{
    Instruction* currentInstruction = m_codeBlock->instructions().begin() + byValInfo->bytecodeIndex;
    
    PatchableJump badType;
    JumpList slowCases;
    
    switch (arrayMode) {
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
    case JITInt8Array:
        slowCases = emitIntTypedArrayPutByVal(currentInstruction, badType, m_vm->int8ArrayDescriptor(), 1, SignedTypedArray, TruncateRounding);
        break;
    case JITInt16Array:
        slowCases = emitIntTypedArrayPutByVal(currentInstruction, badType, m_vm->int16ArrayDescriptor(), 2, SignedTypedArray, TruncateRounding);
        break;
    case JITInt32Array:
        slowCases = emitIntTypedArrayPutByVal(currentInstruction, badType, m_vm->int32ArrayDescriptor(), 4, SignedTypedArray, TruncateRounding);
        break;
    case JITUint8Array:
        slowCases = emitIntTypedArrayPutByVal(currentInstruction, badType, m_vm->uint8ArrayDescriptor(), 1, UnsignedTypedArray, TruncateRounding);
        break;
    case JITUint8ClampedArray:
        slowCases = emitIntTypedArrayPutByVal(currentInstruction, badType, m_vm->uint8ClampedArrayDescriptor(), 1, UnsignedTypedArray, ClampRounding);
        break;
    case JITUint16Array:
        slowCases = emitIntTypedArrayPutByVal(currentInstruction, badType, m_vm->uint16ArrayDescriptor(), 2, UnsignedTypedArray, TruncateRounding);
        break;
    case JITUint32Array:
        slowCases = emitIntTypedArrayPutByVal(currentInstruction, badType, m_vm->uint32ArrayDescriptor(), 4, UnsignedTypedArray, TruncateRounding);
        break;
    case JITFloat32Array:
        slowCases = emitFloatTypedArrayPutByVal(currentInstruction, badType, m_vm->float32ArrayDescriptor(), 4);
        break;
    case JITFloat64Array:
        slowCases = emitFloatTypedArrayPutByVal(currentInstruction, badType, m_vm->float64ArrayDescriptor(), 8);
        break;
    default:
        CRASH();
        break;
    }
    
    Jump done = jump();

    LinkBuffer patchBuffer(*m_vm, this, m_codeBlock);
    
    patchBuffer.link(badType, CodeLocationLabel(MacroAssemblerCodePtr::createFromExecutableAddress(returnAddress.value())).labelAtOffset(byValInfo->returnAddressToSlowPath));
    patchBuffer.link(slowCases, CodeLocationLabel(MacroAssemblerCodePtr::createFromExecutableAddress(returnAddress.value())).labelAtOffset(byValInfo->returnAddressToSlowPath));
    
    patchBuffer.link(done, byValInfo->badTypeJump.labelAtOffset(byValInfo->badTypeJumpToDone));
    
    byValInfo->stubRoutine = FINALIZE_CODE_FOR_STUB(
        patchBuffer,
        ("Baseline put_by_val stub for %s, return point %p", toCString(*m_codeBlock).data(), returnAddress.value()));
    
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(byValInfo->badTypeJump, CodeLocationLabel(byValInfo->stubRoutine->code().code()));
    repatchBuffer.relinkCallerToFunction(returnAddress, FunctionPtr(cti_op_put_by_val_generic));
}

JIT::JumpList JIT::emitIntTypedArrayGetByVal(Instruction*, PatchableJump& badType, const TypedArrayDescriptor& descriptor, size_t elementSize, TypedArraySignedness signedness)
{
    // The best way to test the array type is to use the classInfo. We need to do so without
    // clobbering the register that holds the indexing type, base, and property.

#if USE(JSVALUE64)
    RegisterID base = regT0;
    RegisterID property = regT1;
    RegisterID resultPayload = regT0;
    RegisterID scratch = regT3;
#else
    RegisterID base = regT0;
    RegisterID property = regT2;
    RegisterID resultPayload = regT0;
    RegisterID resultTag = regT1;
    RegisterID scratch = regT3;
#endif
    
    JumpList slowCases;
    
    loadPtr(Address(base, JSCell::structureOffset()), scratch);
    badType = patchableBranchPtr(NotEqual, Address(scratch, Structure::classInfoOffset()), TrustedImmPtr(descriptor.m_classInfo));
    slowCases.append(branch32(AboveOrEqual, property, Address(base, descriptor.m_lengthOffset)));
    loadPtr(Address(base, descriptor.m_storageOffset), base);
    
    switch (elementSize) {
    case 1:
        if (signedness == SignedTypedArray)
            load8Signed(BaseIndex(base, property, TimesOne), resultPayload);
        else
            load8(BaseIndex(base, property, TimesOne), resultPayload);
        break;
    case 2:
        if (signedness == SignedTypedArray)
            load16Signed(BaseIndex(base, property, TimesTwo), resultPayload);
        else
            load16(BaseIndex(base, property, TimesTwo), resultPayload);
        break;
    case 4:
        load32(BaseIndex(base, property, TimesFour), resultPayload);
        break;
    default:
        CRASH();
    }
    
    Jump done;
    if (elementSize == 4 && signedness == UnsignedTypedArray) {
        Jump canBeInt = branch32(GreaterThanOrEqual, resultPayload, TrustedImm32(0));
        
        convertInt32ToDouble(resultPayload, fpRegT0);
        addDouble(AbsoluteAddress(&twoToThe32), fpRegT0);
#if USE(JSVALUE64)
        moveDoubleTo64(fpRegT0, resultPayload);
        sub64(tagTypeNumberRegister, resultPayload);
#else
        moveDoubleToInts(fpRegT0, resultPayload, resultTag);
#endif
        
        done = jump();
        canBeInt.link(this);
    }

#if USE(JSVALUE64)
    or64(tagTypeNumberRegister, resultPayload);
#else
    move(TrustedImm32(JSValue::Int32Tag), resultTag);
#endif
    if (done.isSet())
        done.link(this);
    return slowCases;
}

JIT::JumpList JIT::emitFloatTypedArrayGetByVal(Instruction*, PatchableJump& badType, const TypedArrayDescriptor& descriptor, size_t elementSize)
{
#if USE(JSVALUE64)
    RegisterID base = regT0;
    RegisterID property = regT1;
    RegisterID resultPayload = regT0;
    RegisterID scratch = regT3;
#else
    RegisterID base = regT0;
    RegisterID property = regT2;
    RegisterID resultPayload = regT0;
    RegisterID resultTag = regT1;
    RegisterID scratch = regT3;
#endif
    
    JumpList slowCases;
    
    loadPtr(Address(base, JSCell::structureOffset()), scratch);
    badType = patchableBranchPtr(NotEqual, Address(scratch, Structure::classInfoOffset()), TrustedImmPtr(descriptor.m_classInfo));
    slowCases.append(branch32(AboveOrEqual, property, Address(base, descriptor.m_lengthOffset)));
    loadPtr(Address(base, descriptor.m_storageOffset), base);
    
    switch (elementSize) {
    case 4:
        loadFloat(BaseIndex(base, property, TimesFour), fpRegT0);
        convertFloatToDouble(fpRegT0, fpRegT0);
        break;
    case 8: {
        loadDouble(BaseIndex(base, property, TimesEight), fpRegT0);
        break;
    }
    default:
        CRASH();
    }
    
    Jump notNaN = branchDouble(DoubleEqual, fpRegT0, fpRegT0);
    static const double NaN = QNaN;
    loadDouble(&NaN, fpRegT0);
    notNaN.link(this);
    
#if USE(JSVALUE64)
    moveDoubleTo64(fpRegT0, resultPayload);
    sub64(tagTypeNumberRegister, resultPayload);
#else
    moveDoubleToInts(fpRegT0, resultPayload, resultTag);
#endif
    return slowCases;    
}

JIT::JumpList JIT::emitIntTypedArrayPutByVal(Instruction* currentInstruction, PatchableJump& badType, const TypedArrayDescriptor& descriptor, size_t elementSize, TypedArraySignedness signedness, TypedArrayRounding rounding)
{
    unsigned value = currentInstruction[3].u.operand;

#if USE(JSVALUE64)
    RegisterID base = regT0;
    RegisterID property = regT1;
    RegisterID earlyScratch = regT3;
    RegisterID lateScratch = regT2;
#else
    RegisterID base = regT0;
    RegisterID property = regT2;
    RegisterID earlyScratch = regT3;
    RegisterID lateScratch = regT1;
#endif
    
    JumpList slowCases;
    
    loadPtr(Address(base, JSCell::structureOffset()), earlyScratch);
    badType = patchableBranchPtr(NotEqual, Address(earlyScratch, Structure::classInfoOffset()), TrustedImmPtr(descriptor.m_classInfo));
    slowCases.append(branch32(AboveOrEqual, property, Address(base, descriptor.m_lengthOffset)));
    
#if USE(JSVALUE64)
    emitGetVirtualRegister(value, earlyScratch);
    slowCases.append(emitJumpIfNotImmediateInteger(earlyScratch));
#else
    emitLoad(value, lateScratch, earlyScratch);
    slowCases.append(branch32(NotEqual, lateScratch, TrustedImm32(JSValue::Int32Tag)));
#endif
    
    // We would be loading this into base as in get_by_val, except that the slow
    // path expects the base to be unclobbered.
    loadPtr(Address(base, descriptor.m_storageOffset), lateScratch);
    
    if (rounding == ClampRounding) {
        ASSERT(elementSize == 1);
        ASSERT_UNUSED(signedness, signedness = UnsignedTypedArray);
        Jump inBounds = branch32(BelowOrEqual, earlyScratch, TrustedImm32(0xff));
        Jump tooBig = branch32(GreaterThan, earlyScratch, TrustedImm32(0xff));
        xor32(earlyScratch, earlyScratch);
        Jump clamped = jump();
        tooBig.link(this);
        move(TrustedImm32(0xff), earlyScratch);
        clamped.link(this);
        inBounds.link(this);
    }
    
    switch (elementSize) {
    case 1:
        store8(earlyScratch, BaseIndex(lateScratch, property, TimesOne));
        break;
    case 2:
        store16(earlyScratch, BaseIndex(lateScratch, property, TimesTwo));
        break;
    case 4:
        store32(earlyScratch, BaseIndex(lateScratch, property, TimesFour));
        break;
    default:
        CRASH();
    }
    
    return slowCases;
}

JIT::JumpList JIT::emitFloatTypedArrayPutByVal(Instruction* currentInstruction, PatchableJump& badType, const TypedArrayDescriptor& descriptor, size_t elementSize)
{
    unsigned value = currentInstruction[3].u.operand;

#if USE(JSVALUE64)
    RegisterID base = regT0;
    RegisterID property = regT1;
    RegisterID earlyScratch = regT3;
    RegisterID lateScratch = regT2;
#else
    RegisterID base = regT0;
    RegisterID property = regT2;
    RegisterID earlyScratch = regT3;
    RegisterID lateScratch = regT1;
#endif
    
    JumpList slowCases;
    
    loadPtr(Address(base, JSCell::structureOffset()), earlyScratch);
    badType = patchableBranchPtr(NotEqual, Address(earlyScratch, Structure::classInfoOffset()), TrustedImmPtr(descriptor.m_classInfo));
    slowCases.append(branch32(AboveOrEqual, property, Address(base, descriptor.m_lengthOffset)));
    
#if USE(JSVALUE64)
    emitGetVirtualRegister(value, earlyScratch);
    Jump doubleCase = emitJumpIfNotImmediateInteger(earlyScratch);
    convertInt32ToDouble(earlyScratch, fpRegT0);
    Jump ready = jump();
    doubleCase.link(this);
    slowCases.append(emitJumpIfNotImmediateNumber(earlyScratch));
    add64(tagTypeNumberRegister, earlyScratch);
    move64ToDouble(earlyScratch, fpRegT0);
    ready.link(this);
#else
    emitLoad(value, lateScratch, earlyScratch);
    Jump doubleCase = branch32(NotEqual, lateScratch, TrustedImm32(JSValue::Int32Tag));
    convertInt32ToDouble(earlyScratch, fpRegT0);
    Jump ready = jump();
    doubleCase.link(this);
    slowCases.append(branch32(Above, lateScratch, TrustedImm32(JSValue::LowestTag)));
    moveIntsToDouble(earlyScratch, lateScratch, fpRegT0, fpRegT1);
    ready.link(this);
#endif
    
    // We would be loading this into base as in get_by_val, except that the slow
    // path expects the base to be unclobbered.
    loadPtr(Address(base, descriptor.m_storageOffset), lateScratch);
    
    switch (elementSize) {
    case 4:
        convertDoubleToFloat(fpRegT0, fpRegT0);
        storeFloat(fpRegT0, BaseIndex(lateScratch, property, TimesFour));
        break;
    case 8:
        storeDouble(fpRegT0, BaseIndex(lateScratch, property, TimesEight));
        break;
    default:
        CRASH();
    }
    
    return slowCases;
}

} // namespace JSC

#endif // ENABLE(JIT)
