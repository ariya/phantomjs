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

#ifndef JITInlineMethods_h
#define JITInlineMethods_h


#if ENABLE(JIT)

namespace JSC {

/* Deprecated: Please use JITStubCall instead. */

ALWAYS_INLINE void JIT::emitGetJITStubArg(unsigned argumentNumber, RegisterID dst)
{
    unsigned argumentStackOffset = (argumentNumber * (sizeof(JSValue) / sizeof(void*))) + JITSTACKFRAME_ARGS_INDEX;
    peek(dst, argumentStackOffset);
}

ALWAYS_INLINE bool JIT::isOperandConstantImmediateDouble(unsigned src)
{
    return m_codeBlock->isConstantRegisterIndex(src) && getConstantOperand(src).isDouble();
}

ALWAYS_INLINE JSValue JIT::getConstantOperand(unsigned src)
{
    ASSERT(m_codeBlock->isConstantRegisterIndex(src));
    return m_codeBlock->getConstant(src);
}

ALWAYS_INLINE void JIT::emitPutToCallFrameHeader(RegisterID from, RegisterFile::CallFrameHeaderEntry entry)
{
    storePtr(from, payloadFor(entry, callFrameRegister));
}

ALWAYS_INLINE void JIT::emitPutCellToCallFrameHeader(RegisterID from, RegisterFile::CallFrameHeaderEntry entry)
{
#if USE(JSVALUE32_64)
    store32(TrustedImm32(JSValue::CellTag), tagFor(entry, callFrameRegister));
#endif
    storePtr(from, payloadFor(entry, callFrameRegister));
}

ALWAYS_INLINE void JIT::emitPutIntToCallFrameHeader(RegisterID from, RegisterFile::CallFrameHeaderEntry entry)
{
    store32(TrustedImm32(Int32Tag), intTagFor(entry, callFrameRegister));
    store32(from, intPayloadFor(entry, callFrameRegister));
}

ALWAYS_INLINE void JIT::emitPutImmediateToCallFrameHeader(void* value, RegisterFile::CallFrameHeaderEntry entry)
{
    storePtr(TrustedImmPtr(value), Address(callFrameRegister, entry * sizeof(Register)));
}

ALWAYS_INLINE void JIT::emitGetFromCallFrameHeaderPtr(RegisterFile::CallFrameHeaderEntry entry, RegisterID to, RegisterID from)
{
    loadPtr(Address(from, entry * sizeof(Register)), to);
#if USE(JSVALUE64)
    killLastResultRegister();
#endif
}

ALWAYS_INLINE void JIT::emitLoadCharacterString(RegisterID src, RegisterID dst, JumpList& failures)
{
    failures.append(branchPtr(NotEqual, Address(src), TrustedImmPtr(m_globalData->jsStringVPtr)));
    failures.append(branchTest32(NonZero, Address(src, OBJECT_OFFSETOF(JSString, m_fiberCount))));
    failures.append(branch32(NotEqual, MacroAssembler::Address(src, ThunkHelpers::jsStringLengthOffset()), TrustedImm32(1)));
    loadPtr(MacroAssembler::Address(src, ThunkHelpers::jsStringValueOffset()), dst);
    loadPtr(MacroAssembler::Address(dst, ThunkHelpers::stringImplDataOffset()), dst);
    load16(MacroAssembler::Address(dst, 0), dst);
}

ALWAYS_INLINE void JIT::emitGetFromCallFrameHeader32(RegisterFile::CallFrameHeaderEntry entry, RegisterID to, RegisterID from)
{
    load32(Address(from, entry * sizeof(Register)), to);
#if USE(JSVALUE64)
    killLastResultRegister();
#endif
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
    JSInterfaceJIT::beginUninterruptedSequence();
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

#if defined(ASSEMBLER_HAS_CONSTANT_POOL) && ASSEMBLER_HAS_CONSTANT_POOL
#ifndef NDEBUG
    m_uninterruptedInstructionSequenceBegin = label();
    m_uninterruptedConstantSequenceBegin = sizeOfConstantPool();
#endif
#endif
}

ALWAYS_INLINE void JIT::endUninterruptedSequence(int insnSpace, int constSpace, int dst)
{
    UNUSED_PARAM(dst);
#if defined(ASSEMBLER_HAS_CONSTANT_POOL) && ASSEMBLER_HAS_CONSTANT_POOL
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
#endif
    ASSERT(differenceBetween(m_uninterruptedInstructionSequenceBegin, label()) <= insnSpace);
    ASSERT(sizeOfConstantPool() - m_uninterruptedConstantSequenceBegin <= constSpace);
#endif
    JSInterfaceJIT::endUninterruptedSequence();
}

#endif

#if CPU(ARM)

ALWAYS_INLINE void JIT::preserveReturnAddressAfterCall(RegisterID reg)
{
    move(linkRegister, reg);
}

ALWAYS_INLINE void JIT::restoreReturnAddressBeforeReturn(RegisterID reg)
{
    move(reg, linkRegister);
}

ALWAYS_INLINE void JIT::restoreReturnAddressBeforeReturn(Address address)
{
    loadPtr(address, linkRegister);
}
#elif CPU(SH4)

ALWAYS_INLINE void JIT::preserveReturnAddressAfterCall(RegisterID reg)
{
    m_assembler.stspr(reg);
}

ALWAYS_INLINE void JIT::restoreReturnAddressBeforeReturn(RegisterID reg)
{
    m_assembler.ldspr(reg);
}

ALWAYS_INLINE void JIT::restoreReturnAddressBeforeReturn(Address address)
{
    loadPtrLinkReg(address);
}

#elif CPU(MIPS)

ALWAYS_INLINE void JIT::preserveReturnAddressAfterCall(RegisterID reg)
{
    move(returnAddressRegister, reg);
}

ALWAYS_INLINE void JIT::restoreReturnAddressBeforeReturn(RegisterID reg)
{
    move(reg, returnAddressRegister);
}

ALWAYS_INLINE void JIT::restoreReturnAddressBeforeReturn(Address address)
{
    loadPtr(address, returnAddressRegister);
}

#else // CPU(X86) || CPU(X86_64)

ALWAYS_INLINE void JIT::preserveReturnAddressAfterCall(RegisterID reg)
{
    pop(reg);
}

ALWAYS_INLINE void JIT::restoreReturnAddressBeforeReturn(RegisterID reg)
{
    push(reg);
}

ALWAYS_INLINE void JIT::restoreReturnAddressBeforeReturn(Address address)
{
    push(address);
}

#endif

ALWAYS_INLINE void JIT::restoreArgumentReference()
{
    move(stackPointerRegister, firstArgumentRegister);
    poke(callFrameRegister, OBJECT_OFFSETOF(struct JITStackFrame, callFrame) / sizeof(void*));
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
ALWAYS_INLINE void JIT::emitCount(AbstractSamplingCounter& counter, uint32_t count)
{
#if CPU(X86_64) // Or any other 64-bit plattform.
    addPtr(TrustedImm32(count), AbsoluteAddress(counter.addressOfCounter()));
#elif CPU(X86) // Or any other little-endian 32-bit plattform.
    intptr_t hiWord = reinterpret_cast<intptr_t>(counter.addressOfCounter()) + sizeof(int32_t);
    add32(TrustedImm32(count), AbsoluteAddress(counter.addressOfCounter()));
    addWithCarry32(TrustedImm32(0), AbsoluteAddress(reinterpret_cast<void*>(hiWord)));
#else
#error "SAMPLING_FLAGS not implemented on this platform."
#endif
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

#if USE(JSVALUE32_64)

inline void JIT::emitLoadTag(unsigned index, RegisterID tag)
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

inline void JIT::emitLoadPayload(unsigned index, RegisterID payload)
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

inline void JIT::emitLoad(unsigned index, RegisterID tag, RegisterID payload, RegisterID base)
{
    ASSERT(tag != payload);

    if (base == callFrameRegister) {
        ASSERT(payload != base);
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

inline void JIT::emitLoad2(unsigned index1, RegisterID tag1, RegisterID payload1, unsigned index2, RegisterID tag2, RegisterID payload2)
{
    if (isMapped(index1)) {
        emitLoad(index1, tag1, payload1);
        emitLoad(index2, tag2, payload2);
        return;
    }
    emitLoad(index2, tag2, payload2);
    emitLoad(index1, tag1, payload1);
}

inline void JIT::emitLoadDouble(unsigned index, FPRegisterID value)
{
    if (m_codeBlock->isConstantRegisterIndex(index)) {
        WriteBarrier<Unknown>& inConstantPool = m_codeBlock->constantRegister(index);
        loadDouble(&inConstantPool, value);
    } else
        loadDouble(addressFor(index), value);
}

inline void JIT::emitLoadInt32ToDouble(unsigned index, FPRegisterID value)
{
    if (m_codeBlock->isConstantRegisterIndex(index)) {
        WriteBarrier<Unknown>& inConstantPool = m_codeBlock->constantRegister(index);
        char* bytePointer = reinterpret_cast<char*>(&inConstantPool);
        convertInt32ToDouble(AbsoluteAddress(bytePointer + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), value);
    } else
        convertInt32ToDouble(payloadFor(index), value);
}

inline void JIT::emitStore(unsigned index, RegisterID tag, RegisterID payload, RegisterID base)
{
    store32(payload, payloadFor(index, base));
    store32(tag, tagFor(index, base));
}

inline void JIT::emitStoreInt32(unsigned index, RegisterID payload, bool indexIsInt32)
{
    store32(payload, payloadFor(index, callFrameRegister));
    if (!indexIsInt32)
        store32(TrustedImm32(JSValue::Int32Tag), tagFor(index, callFrameRegister));
}

inline void JIT::emitStoreInt32(unsigned index, TrustedImm32 payload, bool indexIsInt32)
{
    store32(payload, payloadFor(index, callFrameRegister));
    if (!indexIsInt32)
        store32(TrustedImm32(JSValue::Int32Tag), tagFor(index, callFrameRegister));
}

inline void JIT::emitStoreCell(unsigned index, RegisterID payload, bool indexIsCell)
{
    store32(payload, payloadFor(index, callFrameRegister));
    if (!indexIsCell)
        store32(TrustedImm32(JSValue::CellTag), tagFor(index, callFrameRegister));
}

inline void JIT::emitStoreBool(unsigned index, RegisterID payload, bool indexIsBool)
{
    store32(payload, payloadFor(index, callFrameRegister));
    if (!indexIsBool)
        store32(TrustedImm32(JSValue::BooleanTag), tagFor(index, callFrameRegister));
}

inline void JIT::emitStoreDouble(unsigned index, FPRegisterID value)
{
    storeDouble(value, addressFor(index));
}

inline void JIT::emitStore(unsigned index, const JSValue constant, RegisterID base)
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

inline void JIT::map(unsigned bytecodeOffset, unsigned virtualRegisterIndex, RegisterID tag, RegisterID payload)
{
    if (isLabeled(bytecodeOffset))
        return;

    m_mappedBytecodeOffset = bytecodeOffset;
    m_mappedVirtualRegisterIndex = virtualRegisterIndex;
    m_mappedTag = tag;
    m_mappedPayload = payload;
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
    m_mappedVirtualRegisterIndex = (unsigned)-1;
    m_mappedTag = (RegisterID)-1;
    m_mappedPayload = (RegisterID)-1;
}

inline bool JIT::isMapped(unsigned virtualRegisterIndex)
{
    if (m_mappedBytecodeOffset != m_bytecodeOffset)
        return false;
    if (m_mappedVirtualRegisterIndex != virtualRegisterIndex)
        return false;
    return true;
}

inline bool JIT::getMappedPayload(unsigned virtualRegisterIndex, RegisterID& payload)
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

inline bool JIT::getMappedTag(unsigned virtualRegisterIndex, RegisterID& tag)
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

inline void JIT::emitJumpSlowCaseIfNotJSCell(unsigned virtualRegisterIndex)
{
    if (!m_codeBlock->isKnownNotImmediate(virtualRegisterIndex)) {
        if (m_codeBlock->isConstantRegisterIndex(virtualRegisterIndex))
            addSlowCase(jump());
        else
            addSlowCase(emitJumpIfNotJSCell(virtualRegisterIndex));
    }
}

inline void JIT::emitJumpSlowCaseIfNotJSCell(unsigned virtualRegisterIndex, RegisterID tag)
{
    if (!m_codeBlock->isKnownNotImmediate(virtualRegisterIndex)) {
        if (m_codeBlock->isConstantRegisterIndex(virtualRegisterIndex))
            addSlowCase(jump());
        else
            addSlowCase(branch32(NotEqual, tag, TrustedImm32(JSValue::CellTag)));
    }
}

inline void JIT::linkSlowCaseIfNotJSCell(Vector<SlowCaseEntry>::iterator& iter, unsigned virtualRegisterIndex)
{
    if (!m_codeBlock->isKnownNotImmediate(virtualRegisterIndex))
        linkSlowCase(iter);
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
        move(ImmPtr(JSValue::encode(value)), dst);
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

    loadPtr(Address(callFrameRegister, src * sizeof(Register)), dst);
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
    storePtr(from, Address(callFrameRegister, dst * sizeof(Register)));
    m_lastResultBytecodeRegister = (from == cachedResultRegister) ? static_cast<int>(dst) : std::numeric_limits<int>::max();
}

ALWAYS_INLINE void JIT::emitInitRegister(unsigned dst)
{
    storePtr(TrustedImmPtr(JSValue::encode(jsUndefined())), Address(callFrameRegister, dst * sizeof(Register)));
}

ALWAYS_INLINE JIT::Jump JIT::emitJumpIfJSCell(RegisterID reg)
{
#if USE(JSVALUE64)
    return branchTestPtr(Zero, reg, tagMaskRegister);
#else
    return branchTest32(Zero, reg, TrustedImm32(TagMask));
#endif
}

ALWAYS_INLINE JIT::Jump JIT::emitJumpIfBothJSCells(RegisterID reg1, RegisterID reg2, RegisterID scratch)
{
    move(reg1, scratch);
    orPtr(reg2, scratch);
    return emitJumpIfJSCell(scratch);
}

ALWAYS_INLINE void JIT::emitJumpSlowCaseIfJSCell(RegisterID reg)
{
    addSlowCase(emitJumpIfJSCell(reg));
}

ALWAYS_INLINE JIT::Jump JIT::emitJumpIfNotJSCell(RegisterID reg)
{
#if USE(JSVALUE64)
    return branchTestPtr(NonZero, reg, tagMaskRegister);
#else
    return branchTest32(NonZero, reg, TrustedImm32(TagMask));
#endif
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

#if USE(JSVALUE64)

inline void JIT::emitLoadDouble(unsigned index, FPRegisterID value)
{
    if (m_codeBlock->isConstantRegisterIndex(index)) {
        WriteBarrier<Unknown>& inConstantPool = m_codeBlock->constantRegister(index);
        loadDouble(&inConstantPool, value);
    } else
        loadDouble(addressFor(index), value);
}

inline void JIT::emitLoadInt32ToDouble(unsigned index, FPRegisterID value)
{
    if (m_codeBlock->isConstantRegisterIndex(index)) {
        ASSERT(isOperandConstantImmediateInt(index));
        convertInt32ToDouble(Imm32(getConstantOperand(index).asInt32()), value);
    } else
        convertInt32ToDouble(addressFor(index), value);
}
#endif

ALWAYS_INLINE JIT::Jump JIT::emitJumpIfImmediateInteger(RegisterID reg)
{
#if USE(JSVALUE64)
    return branchPtr(AboveOrEqual, reg, tagTypeNumberRegister);
#else
    return branchTest32(NonZero, reg, TrustedImm32(TagTypeNumber));
#endif
}

ALWAYS_INLINE JIT::Jump JIT::emitJumpIfNotImmediateInteger(RegisterID reg)
{
#if USE(JSVALUE64)
    return branchPtr(Below, reg, tagTypeNumberRegister);
#else
    return branchTest32(Zero, reg, TrustedImm32(TagTypeNumber));
#endif
}

ALWAYS_INLINE JIT::Jump JIT::emitJumpIfNotImmediateIntegers(RegisterID reg1, RegisterID reg2, RegisterID scratch)
{
    move(reg1, scratch);
    andPtr(reg2, scratch);
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

#if USE(JSVALUE32_64)
ALWAYS_INLINE void JIT::emitFastArithDeTagImmediate(RegisterID reg)
{
    subPtr(TrustedImm32(TagTypeNumber), reg);
}

ALWAYS_INLINE JIT::Jump JIT::emitFastArithDeTagImmediateJumpIfZero(RegisterID reg)
{
    return branchSubPtr(Zero, TrustedImm32(TagTypeNumber), reg);
}
#endif

ALWAYS_INLINE void JIT::emitFastArithReTagImmediate(RegisterID src, RegisterID dest)
{
#if USE(JSVALUE64)
    emitFastArithIntToImmNoCheck(src, dest);
#else
    if (src != dest)
        move(src, dest);
    addPtr(TrustedImm32(TagTypeNumber), dest);
#endif
}

// operand is int32_t, must have been zero-extended if register is 64-bit.
ALWAYS_INLINE void JIT::emitFastArithIntToImmNoCheck(RegisterID src, RegisterID dest)
{
#if USE(JSVALUE64)
    if (src != dest)
        move(src, dest);
    orPtr(tagTypeNumberRegister, dest);
#else
    signExtend32ToPtr(src, dest);
    addPtr(dest, dest);
    emitFastArithReTagImmediate(dest, dest);
#endif
}

ALWAYS_INLINE void JIT::emitTagAsBoolImmediate(RegisterID reg)
{
    or32(TrustedImm32(static_cast<int32_t>(ValueFalse)), reg);
}

#endif // USE(JSVALUE32_64)

} // namespace JSC

#endif // ENABLE(JIT)

#endif
