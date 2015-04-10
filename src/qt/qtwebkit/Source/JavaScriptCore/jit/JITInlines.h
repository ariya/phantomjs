/*
 * Copyright (C) 2008, 2012 Apple Inc. All rights reserved.
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

#ifndef JITInlines_h
#define JITInlines_h


#if ENABLE(JIT)

namespace JSC {

ALWAYS_INLINE bool JIT::isOperandConstantImmediateDouble(unsigned src)
{
    return m_codeBlock->isConstantRegisterIndex(src) && getConstantOperand(src).isDouble();
}

ALWAYS_INLINE JSValue JIT::getConstantOperand(unsigned src)
{
    ASSERT(m_codeBlock->isConstantRegisterIndex(src));
    return m_codeBlock->getConstant(src);
}

ALWAYS_INLINE void JIT::emitPutIntToCallFrameHeader(RegisterID from, JSStack::CallFrameHeaderEntry entry)
{
#if USE(JSVALUE32_64)
    store32(TrustedImm32(Int32Tag), intTagFor(entry, callFrameRegister));
    store32(from, intPayloadFor(entry, callFrameRegister));
#else
    store64(from, addressFor(entry, callFrameRegister));
#endif
}

ALWAYS_INLINE void JIT::emitGetFromCallFrameHeaderPtr(JSStack::CallFrameHeaderEntry entry, RegisterID to, RegisterID from)
{
    loadPtr(Address(from, entry * sizeof(Register)), to);
#if USE(JSVALUE64)
    killLastResultRegister();
#endif
}

ALWAYS_INLINE void JIT::emitGetFromCallFrameHeader32(JSStack::CallFrameHeaderEntry entry, RegisterID to, RegisterID from)
{
    load32(Address(from, entry * sizeof(Register)), to);
#if USE(JSVALUE64)
    killLastResultRegister();
#endif
}

#if USE(JSVALUE64)
ALWAYS_INLINE void JIT::emitGetFromCallFrameHeader64(JSStack::CallFrameHeaderEntry entry, RegisterID to, RegisterID from)
{
    load64(Address(from, entry * sizeof(Register)), to);
    killLastResultRegister();
}
#endif

ALWAYS_INLINE void JIT::emitLoadCharacterString(RegisterID src, RegisterID dst, JumpList& failures)
{
    failures.append(branchPtr(NotEqual, Address(src, JSCell::structureOffset()), TrustedImmPtr(m_vm->stringStructure.get())));
    failures.append(branch32(NotEqual, MacroAssembler::Address(src, ThunkHelpers::jsStringLengthOffset()), TrustedImm32(1)));
    loadPtr(MacroAssembler::Address(src, ThunkHelpers::jsStringValueOffset()), dst);
    failures.append(branchTest32(Zero, dst));
    loadPtr(MacroAssembler::Address(dst, StringImpl::flagsOffset()), regT1);
    loadPtr(MacroAssembler::Address(dst, StringImpl::dataOffset()), dst);

    JumpList is16Bit;
    JumpList cont8Bit;
    is16Bit.append(branchTest32(Zero, regT1, TrustedImm32(StringImpl::flagIs8Bit())));
    load8(MacroAssembler::Address(dst, 0), dst);
    cont8Bit.append(jump());
    is16Bit.link(this);
    load16(MacroAssembler::Address(dst, 0), dst);
    cont8Bit.link(this);
}

ALWAYS_INLINE JIT::Call JIT::emitNakedCall(CodePtr function)
{
    ASSERT(m_bytecodeOffset != (unsigned)-1); // This method should only be called during hot/cold path generation, so that m_bytecodeOffset is set.

    Call nakedCall = nearCall();
    m_calls.append(CallRecord(nakedCall, m_bytecodeOffset, function.executableAddress()));
    return nakedCall;
}

ALWAYS_INLINE bool JIT::atJumpTarget()
{
    while (m_jumpTargetsPosition < m_codeBlock->numberOfJumpTargets() && m_codeBlock->jumpTarget(m_jumpTargetsPosition) <= m_bytecodeOffset) {
        if (m_codeBlock->jumpTarget(m_jumpTargetsPosition) == m_bytecodeOffset)
            return true;
        ++m_jumpTargetsPosition;
    }
    return false;
}

#if defined(ASSEMBLER_HAS_CONSTANT_POOL) && ASSEMBLER_HAS_CONSTANT_POOL

ALWAYS_INLINE void JIT::beginUninterruptedSequence(int insnSpace, int constSpace)
{
#if CPU(ARM_TRADITIONAL)
#ifndef NDEBUG
    // Ensure the label after the sequence can also fit
    insnSpace += sizeof(ARMWord);
    constSpace += sizeof(uint64_t);
#endif

    ensureSpace(insnSpace, constSpace);

#elif CPU(SH4)
#ifndef NDEBUG
    insnSpace += sizeof(SH4Word);
    constSpace += sizeof(uint64_t);
#endif

    m_assembler.ensureSpace(insnSpace + m_assembler.maxInstructionSize + 2, constSpace + 8);
#endif

#ifndef NDEBUG
    m_uninterruptedInstructionSequenceBegin = label();
    m_uninterruptedConstantSequenceBegin = sizeOfConstantPool();
#endif
}

ALWAYS_INLINE void JIT::endUninterruptedSequence(int insnSpace, int constSpace, int dst)
{
#ifndef NDEBUG
    /* There are several cases when the uninterrupted sequence is larger than
     * maximum required offset for pathing the same sequence. Eg.: if in a
     * uninterrupted sequence the last macroassembler's instruction is a stub
     * call, it emits store instruction(s) which should not be included in the
     * calculation of length of uninterrupted sequence. So, the insnSpace and
     * constSpace should be upper limit instead of hard limit.
     */

#if CPU(SH4)
    if ((dst > 15) || (dst < -16)) {
        insnSpace += 8;
        constSpace += 2;
    }

    if (((dst >= -16) && (dst < 0)) || ((dst > 7) && (dst <= 15)))
        insnSpace += 8;
#else
    UNUSED_PARAM(dst);
#endif

    ASSERT(differenceBetween(m_uninterruptedInstructionSequenceBegin, label()) <= insnSpace);
    ASSERT(sizeOfConstantPool() - m_uninterruptedConstantSequenceBegin <= constSpace);
#else
    UNUSED_PARAM(insnSpace);
    UNUSED_PARAM(constSpace);
    UNUSED_PARAM(dst);
#endif
}

#endif // ASSEMBLER_HAS_CONSTANT_POOL

ALWAYS_INLINE void JIT::updateTopCallFrame()
{
    ASSERT(static_cast<int>(m_bytecodeOffset) >= 0);
#if USE(JSVALUE32_64)
    storePtr(TrustedImmPtr(m_codeBlock->instructions().begin() + m_bytecodeOffset + 1), intTagFor(JSStack::ArgumentCount));
#else
    store32(TrustedImm32(m_bytecodeOffset + 1), intTagFor(JSStack::ArgumentCount));
#endif
    storePtr(callFrameRegister, &m_vm->topCallFrame);
}

ALWAYS_INLINE void JIT::restoreArgumentReferenceForTrampoline()
{
#if CPU(X86)
    // Within a trampoline the return address will be on the stack at this point.
    addPtr(TrustedImm32(sizeof(void*)), stackPointerRegister, firstArgumentRegister);
#elif CPU(ARM)
    move(stackPointerRegister, firstArgumentRegister);
#elif CPU(SH4)
    move(stackPointerRegister, firstArgumentRegister);
#endif
    // In the trampoline on x86-64, the first argument register is not overwritten.
}

ALWAYS_INLINE JIT::Jump JIT::checkStructure(RegisterID reg, Structure* structure)
{
    return branchPtr(NotEqual, Address(reg, JSCell::structureOffset()), TrustedImmPtr(structure));
}

ALWAYS_INLINE void JIT::linkSlowCaseIfNotJSCell(Vector<SlowCaseEntry>::iterator& iter, int vReg)
{
    if (!m_codeBlock->isKnownNotImmediate(vReg))
        linkSlowCase(iter);
}

ALWAYS_INLINE void JIT::addSlowCase(Jump jump)
{
    ASSERT(m_bytecodeOffset != (unsigned)-1); // This method should only be called during hot/cold path generation, so that m_bytecodeOffset is set.

    m_slowCases.append(SlowCaseEntry(jump, m_bytecodeOffset));
}

ALWAYS_INLINE void JIT::addSlowCase(JumpList jumpList)
{
    ASSERT(m_bytecodeOffset != (unsigned)-1); // This method should only be called during hot/cold path generation, so that m_bytecodeOffset is set.

    const JumpList::JumpVector& jumpVector = jumpList.jumps();
    size_t size = jumpVector.size();
    for (size_t i = 0; i < size; ++i)
        m_slowCases.append(SlowCaseEntry(jumpVector[i], m_bytecodeOffset));
}

ALWAYS_INLINE void JIT::addSlowCase()
{
    ASSERT(m_bytecodeOffset != (unsigned)-1); // This method should only be called during hot/cold path generation, so that m_bytecodeOffset is set.
    
    Jump emptyJump; // Doing it this way to make Windows happy.
    m_slowCases.append(SlowCaseEntry(emptyJump, m_bytecodeOffset));
}

ALWAYS_INLINE void JIT::addJump(Jump jump, int relativeOffset)
{
    ASSERT(m_bytecodeOffset != (unsigned)-1); // This method should only be called during hot/cold path generation, so that m_bytecodeOffset is set.

    m_jmpTable.append(JumpTable(jump, m_bytecodeOffset + relativeOffset));
}

ALWAYS_INLINE void JIT::emitJumpSlowToHot(Jump jump, int relativeOffset)
{
    ASSERT(m_bytecodeOffset != (unsigned)-1); // This method should only be called during hot/cold path generation, so that m_bytecodeOffset is set.

    jump.linkTo(m_labels[m_bytecodeOffset + relativeOffset], this);
}

ALWAYS_INLINE JIT::Jump JIT::emitJumpIfNotObject(RegisterID structureReg)
{
    return branch8(Below, Address(structureReg, Structure::typeInfoTypeOffset()), TrustedImm32(ObjectType));
}

#if ENABLE(SAMPLING_FLAGS)
ALWAYS_INLINE void JIT::setSamplingFlag(int32_t flag)
{
    ASSERT(flag >= 1);
    ASSERT(flag <= 32);
    or32(TrustedImm32(1u << (flag - 1)), AbsoluteAddress(SamplingFlags::addressOfFlags()));
}

ALWAYS_INLINE void JIT::clearSamplingFlag(int32_t flag)
{
    ASSERT(flag >= 1);
    ASSERT(flag <= 32);
    and32(TrustedImm32(~(1u << (flag - 1))), AbsoluteAddress(SamplingFlags::addressOfFlags()));
}
#endif

#if ENABLE(SAMPLING_COUNTERS)
ALWAYS_INLINE void JIT::emitCount(AbstractSamplingCounter& counter, int32_t count)
{
    add64(TrustedImm32(count), AbsoluteAddress(counter.addressOfCounter()));
}
#endif

#if ENABLE(OPCODE_SAMPLING)
#if CPU(X86_64)
ALWAYS_INLINE void JIT::sampleInstruction(Instruction* instruction, bool inHostFunction)
{
    move(TrustedImmPtr(m_interpreter->sampler()->sampleSlot()), X86Registers::ecx);
    storePtr(TrustedImmPtr(m_interpreter->sampler()->encodeSample(instruction, inHostFunction)), X86Registers::ecx);
}
#else
ALWAYS_INLINE void JIT::sampleInstruction(Instruction* instruction, bool inHostFunction)
{
    storePtr(TrustedImmPtr(m_interpreter->sampler()->encodeSample(instruction, inHostFunction)), m_interpreter->sampler()->sampleSlot());
}
#endif
#endif

#if ENABLE(CODEBLOCK_SAMPLING)
#if CPU(X86_64)
ALWAYS_INLINE void JIT::sampleCodeBlock(CodeBlock* codeBlock)
{
    move(TrustedImmPtr(m_interpreter->sampler()->codeBlockSlot()), X86Registers::ecx);
    storePtr(TrustedImmPtr(codeBlock), X86Registers::ecx);
}
#else
ALWAYS_INLINE void JIT::sampleCodeBlock(CodeBlock* codeBlock)
{
    storePtr(TrustedImmPtr(codeBlock), m_interpreter->sampler()->codeBlockSlot());
}
#endif
#endif

ALWAYS_INLINE bool JIT::isOperandConstantImmediateChar(unsigned src)
{
    return m_codeBlock->isConstantRegisterIndex(src) && getConstantOperand(src).isString() && asString(getConstantOperand(src).asCell())->length() == 1;
}

template<typename StructureType>
inline void JIT::emitAllocateJSObject(RegisterID allocator, StructureType structure, RegisterID result, RegisterID scratch)
{
    loadPtr(Address(allocator, MarkedAllocator::offsetOfFreeListHead()), result);
    addSlowCase(branchTestPtr(Zero, result));

    // remove the object from the free list
    loadPtr(Address(result), scratch);
    storePtr(scratch, Address(allocator, MarkedAllocator::offsetOfFreeListHead()));

    // initialize the object's structure
    storePtr(structure, Address(result, JSCell::structureOffset()));

    // initialize the object's property storage pointer
    storePtr(TrustedImmPtr(0), Address(result, JSObject::butterflyOffset()));
}

#if ENABLE(VALUE_PROFILER)
inline void JIT::emitValueProfilingSite(ValueProfile* valueProfile)
{
    ASSERT(shouldEmitProfiling());
    ASSERT(valueProfile);

    const RegisterID value = regT0;
#if USE(JSVALUE32_64)
    const RegisterID valueTag = regT1;
#endif
    const RegisterID scratch = regT3;
    
    if (ValueProfile::numberOfBuckets == 1) {
        // We're in a simple configuration: only one bucket, so we can just do a direct
        // store.
#if USE(JSVALUE64)
        store64(value, valueProfile->m_buckets);
#else
        EncodedValueDescriptor* descriptor = bitwise_cast<EncodedValueDescriptor*>(valueProfile->m_buckets);
        store32(value, &descriptor->asBits.payload);
        store32(valueTag, &descriptor->asBits.tag);
#endif
        return;
    }
    
    if (m_randomGenerator.getUint32() & 1)
        add32(TrustedImm32(1), bucketCounterRegister);
    else
        add32(TrustedImm32(3), bucketCounterRegister);
    and32(TrustedImm32(ValueProfile::bucketIndexMask), bucketCounterRegister);
    move(TrustedImmPtr(valueProfile->m_buckets), scratch);
#if USE(JSVALUE64)
    store64(value, BaseIndex(scratch, bucketCounterRegister, TimesEight));
#elif USE(JSVALUE32_64)
    store32(value, BaseIndex(scratch, bucketCounterRegister, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
    store32(valueTag, BaseIndex(scratch, bucketCounterRegister, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
#endif
}

inline void JIT::emitValueProfilingSite(unsigned bytecodeOffset)
{
    if (!shouldEmitProfiling())
        return;
    emitValueProfilingSite(m_codeBlock->valueProfileForBytecodeOffset(bytecodeOffset));
}

inline void JIT::emitValueProfilingSite()
{
    emitValueProfilingSite(m_bytecodeOffset);
}
#endif // ENABLE(VALUE_PROFILER)

inline void JIT::emitArrayProfilingSite(RegisterID structureAndIndexingType, RegisterID scratch, ArrayProfile* arrayProfile)
{
    UNUSED_PARAM(scratch); // We had found this scratch register useful here before, so I will keep it for now.
    
    RegisterID structure = structureAndIndexingType;
    RegisterID indexingType = structureAndIndexingType;
    
    if (shouldEmitProfiling())
        storePtr(structure, arrayProfile->addressOfLastSeenStructure());

    load8(Address(structure, Structure::indexingTypeOffset()), indexingType);
}

inline void JIT::emitArrayProfilingSiteForBytecodeIndex(RegisterID structureAndIndexingType, RegisterID scratch, unsigned bytecodeIndex)
{
#if ENABLE(VALUE_PROFILER)
    emitArrayProfilingSite(structureAndIndexingType, scratch, m_codeBlock->getOrAddArrayProfile(bytecodeIndex));
#else
    UNUSED_PARAM(bytecodeIndex);
    emitArrayProfilingSite(structureAndIndexingType, scratch, 0);
#endif
}

inline void JIT::emitArrayProfileStoreToHoleSpecialCase(ArrayProfile* arrayProfile)
{
#if ENABLE(VALUE_PROFILER)    
    store8(TrustedImm32(1), arrayProfile->addressOfMayStoreToHole());
#else
    UNUSED_PARAM(arrayProfile);
#endif
}

inline void JIT::emitArrayProfileOutOfBoundsSpecialCase(ArrayProfile* arrayProfile)
{
#if ENABLE(VALUE_PROFILER)    
    store8(TrustedImm32(1), arrayProfile->addressOfOutOfBounds());
#else
    UNUSED_PARAM(arrayProfile);
#endif
}

static inline bool arrayProfileSaw(ArrayModes arrayModes, IndexingType capability)
{
#if ENABLE(VALUE_PROFILER)
    return arrayModesInclude(arrayModes, capability);
#else
    UNUSED_PARAM(arrayModes);
    UNUSED_PARAM(capability);
    return false;
#endif
}

inline JITArrayMode JIT::chooseArrayMode(ArrayProfile* profile)
{
#if ENABLE(VALUE_PROFILER)        
    profile->computeUpdatedPrediction(m_codeBlock);
    ArrayModes arrayModes = profile->observedArrayModes();
    if (arrayProfileSaw(arrayModes, DoubleShape))
        return JITDouble;
    if (arrayProfileSaw(arrayModes, Int32Shape))
        return JITInt32;
    if (arrayProfileSaw(arrayModes, ArrayStorageShape))
        return JITArrayStorage;
    return JITContiguous;
#else
    UNUSED_PARAM(profile);
    return JITContiguous;
#endif
}

#if USE(JSVALUE32_64)

inline void JIT::emitLoadTag(int index, RegisterID tag)
{
    RegisterID mappedTag;
    if (getMappedTag(index, mappedTag)) {
        move(mappedTag, tag);
        unmap(tag);
        return;
    }

    if (m_codeBlock->isConstantRegisterIndex(index)) {
        move(Imm32(getConstantOperand(index).tag()), tag);
        unmap(tag);
        return;
    }

    load32(tagFor(index), tag);
    unmap(tag);
}

inline void JIT::emitLoadPayload(int index, RegisterID payload)
{
    RegisterID mappedPayload;
    if (getMappedPayload(index, mappedPayload)) {
        move(mappedPayload, payload);
        unmap(payload);
        return;
    }

    if (m_codeBlock->isConstantRegisterIndex(index)) {
        move(Imm32(getConstantOperand(index).payload()), payload);
        unmap(payload);
        return;
    }

    load32(payloadFor(index), payload);
    unmap(payload);
}

inline void JIT::emitLoad(const JSValue& v, RegisterID tag, RegisterID payload)
{
    move(Imm32(v.payload()), payload);
    move(Imm32(v.tag()), tag);
}

inline void JIT::emitLoad(int index, RegisterID tag, RegisterID payload, RegisterID base)
{
    RELEASE_ASSERT(tag != payload);

    if (base == callFrameRegister) {
        RELEASE_ASSERT(payload != base);
        emitLoadPayload(index, payload);
        emitLoadTag(index, tag);
        return;
    }

    if (payload == base) { // avoid stomping base
        load32(tagFor(index, base), tag);
        load32(payloadFor(index, base), payload);
        return;
    }

    load32(payloadFor(index, base), payload);
    load32(tagFor(index, base), tag);
}

inline void JIT::emitLoad2(int index1, RegisterID tag1, RegisterID payload1, int index2, RegisterID tag2, RegisterID payload2)
{
    if (isMapped(index1)) {
        emitLoad(index1, tag1, payload1);
        emitLoad(index2, tag2, payload2);
        return;
    }
    emitLoad(index2, tag2, payload2);
    emitLoad(index1, tag1, payload1);
}

inline void JIT::emitLoadDouble(int index, FPRegisterID value)
{
    if (m_codeBlock->isConstantRegisterIndex(index)) {
        WriteBarrier<Unknown>& inConstantPool = m_codeBlock->constantRegister(index);
        loadDouble(&inConstantPool, value);
    } else
        loadDouble(addressFor(index), value);
}

inline void JIT::emitLoadInt32ToDouble(int index, FPRegisterID value)
{
    if (m_codeBlock->isConstantRegisterIndex(index)) {
        WriteBarrier<Unknown>& inConstantPool = m_codeBlock->constantRegister(index);
        char* bytePointer = reinterpret_cast<char*>(&inConstantPool);
        convertInt32ToDouble(AbsoluteAddress(bytePointer + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), value);
    } else
        convertInt32ToDouble(payloadFor(index), value);
}

inline void JIT::emitStore(int index, RegisterID tag, RegisterID payload, RegisterID base)
{
    store32(payload, payloadFor(index, base));
    store32(tag, tagFor(index, base));
}

inline void JIT::emitStoreInt32(int index, RegisterID payload, bool indexIsInt32)
{
    store32(payload, payloadFor(index, callFrameRegister));
    if (!indexIsInt32)
        store32(TrustedImm32(JSValue::Int32Tag), tagFor(index, callFrameRegister));
}

inline void JIT::emitStoreAndMapInt32(int index, RegisterID tag, RegisterID payload, bool indexIsInt32, size_t opcodeLength)
{
    emitStoreInt32(index, payload, indexIsInt32);
    map(m_bytecodeOffset + opcodeLength, index, tag, payload);
}

inline void JIT::emitStoreInt32(int index, TrustedImm32 payload, bool indexIsInt32)
{
    store32(payload, payloadFor(index, callFrameRegister));
    if (!indexIsInt32)
        store32(TrustedImm32(JSValue::Int32Tag), tagFor(index, callFrameRegister));
}

inline void JIT::emitStoreCell(int index, RegisterID payload, bool indexIsCell)
{
    store32(payload, payloadFor(index, callFrameRegister));
    if (!indexIsCell)
        store32(TrustedImm32(JSValue::CellTag), tagFor(index, callFrameRegister));
}

inline void JIT::emitStoreBool(int index, RegisterID payload, bool indexIsBool)
{
    store32(payload, payloadFor(index, callFrameRegister));
    if (!indexIsBool)
        store32(TrustedImm32(JSValue::BooleanTag), tagFor(index, callFrameRegister));
}

inline void JIT::emitStoreDouble(int index, FPRegisterID value)
{
    storeDouble(value, addressFor(index));
}

inline void JIT::emitStore(int index, const JSValue constant, RegisterID base)
{
    store32(Imm32(constant.payload()), payloadFor(index, base));
    store32(Imm32(constant.tag()), tagFor(index, base));
}

ALWAYS_INLINE void JIT::emitInitRegister(unsigned dst)
{
    emitStore(dst, jsUndefined());
}

inline bool JIT::isLabeled(unsigned bytecodeOffset)
{
    for (size_t numberOfJumpTargets = m_codeBlock->numberOfJumpTargets(); m_jumpTargetIndex != numberOfJumpTargets; ++m_jumpTargetIndex) {
        unsigned jumpTarget = m_codeBlock->jumpTarget(m_jumpTargetIndex);
        if (jumpTarget == bytecodeOffset)
            return true;
        if (jumpTarget > bytecodeOffset)
            return false;
    }
    return false;
}

inline void JIT::map(unsigned bytecodeOffset, int virtualRegisterIndex, RegisterID tag, RegisterID payload)
{
    if (isLabeled(bytecodeOffset))
        return;

    m_mappedBytecodeOffset = bytecodeOffset;
    m_mappedVirtualRegisterIndex = virtualRegisterIndex;
    m_mappedTag = tag;
    m_mappedPayload = payload;
    
    ASSERT(!canBeOptimizedOrInlined() || m_mappedPayload == regT0);
    ASSERT(!canBeOptimizedOrInlined() || m_mappedTag == regT1);
}

inline void JIT::unmap(RegisterID registerID)
{
    if (m_mappedTag == registerID)
        m_mappedTag = (RegisterID)-1;
    else if (m_mappedPayload == registerID)
        m_mappedPayload = (RegisterID)-1;
}

inline void JIT::unmap()
{
    m_mappedBytecodeOffset = (unsigned)-1;
    m_mappedVirtualRegisterIndex = JSStack::ReturnPC;
    m_mappedTag = (RegisterID)-1;
    m_mappedPayload = (RegisterID)-1;
}

inline bool JIT::isMapped(int virtualRegisterIndex)
{
    if (m_mappedBytecodeOffset != m_bytecodeOffset)
        return false;
    if (m_mappedVirtualRegisterIndex != virtualRegisterIndex)
        return false;
    return true;
}

inline bool JIT::getMappedPayload(int virtualRegisterIndex, RegisterID& payload)
{
    if (m_mappedBytecodeOffset != m_bytecodeOffset)
        return false;
    if (m_mappedVirtualRegisterIndex != virtualRegisterIndex)
        return false;
    if (m_mappedPayload == (RegisterID)-1)
        return false;
    payload = m_mappedPayload;
    return true;
}

inline bool JIT::getMappedTag(int virtualRegisterIndex, RegisterID& tag)
{
    if (m_mappedBytecodeOffset != m_bytecodeOffset)
        return false;
    if (m_mappedVirtualRegisterIndex != virtualRegisterIndex)
        return false;
    if (m_mappedTag == (RegisterID)-1)
        return false;
    tag = m_mappedTag;
    return true;
}

inline void JIT::emitJumpSlowCaseIfNotJSCell(int virtualRegisterIndex)
{
    if (!m_codeBlock->isKnownNotImmediate(virtualRegisterIndex)) {
        if (m_codeBlock->isConstantRegisterIndex(virtualRegisterIndex))
            addSlowCase(jump());
        else
            addSlowCase(emitJumpIfNotJSCell(virtualRegisterIndex));
    }
}

inline void JIT::emitJumpSlowCaseIfNotJSCell(int virtualRegisterIndex, RegisterID tag)
{
    if (!m_codeBlock->isKnownNotImmediate(virtualRegisterIndex)) {
        if (m_codeBlock->isConstantRegisterIndex(virtualRegisterIndex))
            addSlowCase(jump());
        else
            addSlowCase(branch32(NotEqual, tag, TrustedImm32(JSValue::CellTag)));
    }
}

ALWAYS_INLINE bool JIT::isOperandConstantImmediateInt(unsigned src)
{
    return m_codeBlock->isConstantRegisterIndex(src) && getConstantOperand(src).isInt32();
}

ALWAYS_INLINE bool JIT::getOperandConstantImmediateInt(unsigned op1, unsigned op2, unsigned& op, int32_t& constant)
{
    if (isOperandConstantImmediateInt(op1)) {
        constant = getConstantOperand(op1).asInt32();
        op = op2;
        return true;
    }

    if (isOperandConstantImmediateInt(op2)) {
        constant = getConstantOperand(op2).asInt32();
        op = op1;
        return true;
    }
    
    return false;
}

#else // USE(JSVALUE32_64)

/* Deprecated: Please use JITStubCall instead. */

ALWAYS_INLINE void JIT::emitGetJITStubArg(unsigned argumentNumber, RegisterID dst)
{
    unsigned argumentStackOffset = (argumentNumber * (sizeof(JSValue) / sizeof(void*))) + JITSTACKFRAME_ARGS_INDEX;
    peek64(dst, argumentStackOffset);
}

ALWAYS_INLINE void JIT::killLastResultRegister()
{
    m_lastResultBytecodeRegister = std::numeric_limits<int>::max();
}

// get arg puts an arg from the SF register array into a h/w register
ALWAYS_INLINE void JIT::emitGetVirtualRegister(int src, RegisterID dst)
{
    ASSERT(m_bytecodeOffset != (unsigned)-1); // This method should only be called during hot/cold path generation, so that m_bytecodeOffset is set.

    // TODO: we want to reuse values that are already in registers if we can - add a register allocator!
    if (m_codeBlock->isConstantRegisterIndex(src)) {
        JSValue value = m_codeBlock->getConstant(src);
        if (!value.isNumber())
            move(TrustedImm64(JSValue::encode(value)), dst);
        else
            move(Imm64(JSValue::encode(value)), dst);
        killLastResultRegister();
        return;
    }

    if (src == m_lastResultBytecodeRegister && m_codeBlock->isTemporaryRegisterIndex(src) && !atJumpTarget()) {
        // The argument we want is already stored in eax
        if (dst != cachedResultRegister)
            move(cachedResultRegister, dst);
        killLastResultRegister();
        return;
    }

    load64(Address(callFrameRegister, src * sizeof(Register)), dst);
    killLastResultRegister();
}

ALWAYS_INLINE void JIT::emitGetVirtualRegisters(int src1, RegisterID dst1, int src2, RegisterID dst2)
{
    if (src2 == m_lastResultBytecodeRegister) {
        emitGetVirtualRegister(src2, dst2);
        emitGetVirtualRegister(src1, dst1);
    } else {
        emitGetVirtualRegister(src1, dst1);
        emitGetVirtualRegister(src2, dst2);
    }
}

ALWAYS_INLINE int32_t JIT::getConstantOperandImmediateInt(unsigned src)
{
    return getConstantOperand(src).asInt32();
}

ALWAYS_INLINE bool JIT::isOperandConstantImmediateInt(unsigned src)
{
    return m_codeBlock->isConstantRegisterIndex(src) && getConstantOperand(src).isInt32();
}

ALWAYS_INLINE void JIT::emitPutVirtualRegister(unsigned dst, RegisterID from)
{
    store64(from, Address(callFrameRegister, dst * sizeof(Register)));
    m_lastResultBytecodeRegister = (from == cachedResultRegister) ? static_cast<int>(dst) : std::numeric_limits<int>::max();
}

ALWAYS_INLINE void JIT::emitInitRegister(unsigned dst)
{
    store64(TrustedImm64(JSValue::encode(jsUndefined())), Address(callFrameRegister, dst * sizeof(Register)));
}

ALWAYS_INLINE JIT::Jump JIT::emitJumpIfJSCell(RegisterID reg)
{
    return branchTest64(Zero, reg, tagMaskRegister);
}

ALWAYS_INLINE JIT::Jump JIT::emitJumpIfBothJSCells(RegisterID reg1, RegisterID reg2, RegisterID scratch)
{
    move(reg1, scratch);
    or64(reg2, scratch);
    return emitJumpIfJSCell(scratch);
}

ALWAYS_INLINE void JIT::emitJumpSlowCaseIfJSCell(RegisterID reg)
{
    addSlowCase(emitJumpIfJSCell(reg));
}

ALWAYS_INLINE void JIT::emitJumpSlowCaseIfNotJSCell(RegisterID reg)
{
    addSlowCase(emitJumpIfNotJSCell(reg));
}

ALWAYS_INLINE void JIT::emitJumpSlowCaseIfNotJSCell(RegisterID reg, int vReg)
{
    if (!m_codeBlock->isKnownNotImmediate(vReg))
        emitJumpSlowCaseIfNotJSCell(reg);
}

inline void JIT::emitLoadDouble(int index, FPRegisterID value)
{
    if (m_codeBlock->isConstantRegisterIndex(index)) {
        WriteBarrier<Unknown>& inConstantPool = m_codeBlock->constantRegister(index);
        loadDouble(&inConstantPool, value);
    } else
        loadDouble(addressFor(index), value);
}

inline void JIT::emitLoadInt32ToDouble(int index, FPRegisterID value)
{
    if (m_codeBlock->isConstantRegisterIndex(index)) {
        ASSERT(isOperandConstantImmediateInt(index));
        convertInt32ToDouble(Imm32(getConstantOperand(index).asInt32()), value);
    } else
        convertInt32ToDouble(addressFor(index), value);
}

ALWAYS_INLINE JIT::Jump JIT::emitJumpIfImmediateInteger(RegisterID reg)
{
    return branch64(AboveOrEqual, reg, tagTypeNumberRegister);
}

ALWAYS_INLINE JIT::Jump JIT::emitJumpIfNotImmediateInteger(RegisterID reg)
{
    return branch64(Below, reg, tagTypeNumberRegister);
}

ALWAYS_INLINE JIT::Jump JIT::emitJumpIfNotImmediateIntegers(RegisterID reg1, RegisterID reg2, RegisterID scratch)
{
    move(reg1, scratch);
    and64(reg2, scratch);
    return emitJumpIfNotImmediateInteger(scratch);
}

ALWAYS_INLINE void JIT::emitJumpSlowCaseIfNotImmediateInteger(RegisterID reg)
{
    addSlowCase(emitJumpIfNotImmediateInteger(reg));
}

ALWAYS_INLINE void JIT::emitJumpSlowCaseIfNotImmediateIntegers(RegisterID reg1, RegisterID reg2, RegisterID scratch)
{
    addSlowCase(emitJumpIfNotImmediateIntegers(reg1, reg2, scratch));
}

ALWAYS_INLINE void JIT::emitJumpSlowCaseIfNotImmediateNumber(RegisterID reg)
{
    addSlowCase(emitJumpIfNotImmediateNumber(reg));
}

ALWAYS_INLINE void JIT::emitFastArithReTagImmediate(RegisterID src, RegisterID dest)
{
    emitFastArithIntToImmNoCheck(src, dest);
}

ALWAYS_INLINE void JIT::emitTagAsBoolImmediate(RegisterID reg)
{
    or32(TrustedImm32(static_cast<int32_t>(ValueFalse)), reg);
}

#endif // USE(JSVALUE32_64)

} // namespace JSC

#endif // ENABLE(JIT)

#endif // JITInlines_h

