/*
 * Copyright (C) 2008 Apple Inc.
 * Copyright (C) 2009, 2010 University of Szeged
 * All rights reserved.
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

#ifndef MacroAssemblerARM_h
#define MacroAssemblerARM_h

#if ENABLE(ASSEMBLER) && CPU(ARM_TRADITIONAL)

#include "ARMAssembler.h"
#include "AbstractMacroAssembler.h"

namespace JSC {

class MacroAssemblerARM : public AbstractMacroAssembler<ARMAssembler> {
    static const int DoubleConditionMask = 0x0f;
    static const int DoubleConditionBitSpecial = 0x10;
    COMPILE_ASSERT(!(DoubleConditionBitSpecial & DoubleConditionMask), DoubleConditionBitSpecial_should_not_interfere_with_ARMAssembler_Condition_codes);
public:
    typedef ARMRegisters::FPRegisterID FPRegisterID;

    enum RelationalCondition {
        Equal = ARMAssembler::EQ,
        NotEqual = ARMAssembler::NE,
        Above = ARMAssembler::HI,
        AboveOrEqual = ARMAssembler::CS,
        Below = ARMAssembler::CC,
        BelowOrEqual = ARMAssembler::LS,
        GreaterThan = ARMAssembler::GT,
        GreaterThanOrEqual = ARMAssembler::GE,
        LessThan = ARMAssembler::LT,
        LessThanOrEqual = ARMAssembler::LE
    };

    enum ResultCondition {
        Overflow = ARMAssembler::VS,
        Signed = ARMAssembler::MI,
        PositiveOrZero = ARMAssembler::PL,
        Zero = ARMAssembler::EQ,
        NonZero = ARMAssembler::NE
    };

    enum DoubleCondition {
        // These conditions will only evaluate to true if the comparison is ordered - i.e. neither operand is NaN.
        DoubleEqual = ARMAssembler::EQ,
        DoubleNotEqual = ARMAssembler::NE | DoubleConditionBitSpecial,
        DoubleGreaterThan = ARMAssembler::GT,
        DoubleGreaterThanOrEqual = ARMAssembler::GE,
        DoubleLessThan = ARMAssembler::CC,
        DoubleLessThanOrEqual = ARMAssembler::LS,
        // If either operand is NaN, these conditions always evaluate to true.
        DoubleEqualOrUnordered = ARMAssembler::EQ | DoubleConditionBitSpecial,
        DoubleNotEqualOrUnordered = ARMAssembler::NE,
        DoubleGreaterThanOrUnordered = ARMAssembler::HI,
        DoubleGreaterThanOrEqualOrUnordered = ARMAssembler::CS,
        DoubleLessThanOrUnordered = ARMAssembler::LT,
        DoubleLessThanOrEqualOrUnordered = ARMAssembler::LE,
    };

    static const RegisterID stackPointerRegister = ARMRegisters::sp;
    static const RegisterID linkRegister = ARMRegisters::lr;

    static const Scale ScalePtr = TimesFour;

    void add32(RegisterID src, RegisterID dest)
    {
        m_assembler.adds(dest, dest, src);
    }

    void add32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.adds(dest, op1, op2);
    }

    void add32(TrustedImm32 imm, Address address)
    {
        load32(address, ARMRegisters::S1);
        add32(imm, ARMRegisters::S1);
        store32(ARMRegisters::S1, address);
    }

    void add32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.adds(dest, dest, m_assembler.getImm(imm.m_value, ARMRegisters::S0));
    }

    void add32(AbsoluteAddress src, RegisterID dest)
    {
        move(TrustedImmPtr(src.m_ptr), ARMRegisters::S1);
        m_assembler.dtrUp(ARMAssembler::LoadUint32, ARMRegisters::S1, ARMRegisters::S1, 0);
        add32(ARMRegisters::S1, dest);
    }

    void add32(Address src, RegisterID dest)
    {
        load32(src, ARMRegisters::S1);
        add32(ARMRegisters::S1, dest);
    }

    void add32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.adds(dest, src, m_assembler.getImm(imm.m_value, ARMRegisters::S0));
    }

    void and32(RegisterID src, RegisterID dest)
    {
        m_assembler.bitAnds(dest, dest, src);
    }

    void and32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.bitAnds(dest, op1, op2);
    }

    void and32(TrustedImm32 imm, RegisterID dest)
    {
        ARMWord w = m_assembler.getImm(imm.m_value, ARMRegisters::S0, true);
        if (w & ARMAssembler::Op2InvertedImmediate)
            m_assembler.bics(dest, dest, w & ~ARMAssembler::Op2InvertedImmediate);
        else
            m_assembler.bitAnds(dest, dest, w);
    }

    void and32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        ARMWord w = m_assembler.getImm(imm.m_value, ARMRegisters::S0, true);
        if (w & ARMAssembler::Op2InvertedImmediate)
            m_assembler.bics(dest, src, w & ~ARMAssembler::Op2InvertedImmediate);
        else
            m_assembler.bitAnds(dest, src, w);
    }

    void and32(Address src, RegisterID dest)
    {
        load32(src, ARMRegisters::S1);
        and32(ARMRegisters::S1, dest);
    }

    void lshift32(RegisterID shiftAmount, RegisterID dest)
    {
        lshift32(dest, shiftAmount, dest);
    }

    void lshift32(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        ARMWord w = ARMAssembler::getOp2Byte(0x1f);
        m_assembler.bitAnd(ARMRegisters::S0, shiftAmount, w);

        m_assembler.movs(dest, m_assembler.lslRegister(src, ARMRegisters::S0));
    }

    void lshift32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.movs(dest, m_assembler.lsl(dest, imm.m_value & 0x1f));
    }

    void lshift32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.movs(dest, m_assembler.lsl(src, imm.m_value & 0x1f));
    }

    void mul32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        if (op2 == dest) {
            if (op1 == dest) {
                move(op2, ARMRegisters::S0);
                op2 = ARMRegisters::S0;
            } else {
                // Swap the operands.
                RegisterID tmp = op1;
                op1 = op2;
                op2 = tmp;
            }
        }
        m_assembler.muls(dest, op1, op2);
    }

    void mul32(RegisterID src, RegisterID dest)
    {
        mul32(src, dest, dest);
    }

    void mul32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        move(imm, ARMRegisters::S0);
        m_assembler.muls(dest, src, ARMRegisters::S0);
    }

    void neg32(RegisterID srcDest)
    {
        m_assembler.rsbs(srcDest, srcDest, ARMAssembler::getOp2Byte(0));
    }

    void or32(RegisterID src, RegisterID dest)
    {
        m_assembler.orrs(dest, dest, src);
    }

    void or32(RegisterID src, AbsoluteAddress dest)
    {
        move(TrustedImmPtr(dest.m_ptr), ARMRegisters::S0);
        load32(Address(ARMRegisters::S0), ARMRegisters::S1);
        or32(src, ARMRegisters::S1);
        store32(ARMRegisters::S1, ARMRegisters::S0);
    }

    void or32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.orrs(dest, dest, m_assembler.getImm(imm.m_value, ARMRegisters::S0));
    }

    void or32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        m_assembler.orrs(dest, src, m_assembler.getImm(imm.m_value, ARMRegisters::S0));
    }

    void or32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.orrs(dest, op1, op2);
    }

    void rshift32(RegisterID shiftAmount, RegisterID dest)
    {
        rshift32(dest, shiftAmount, dest);
    }

    void rshift32(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        ARMWord w = ARMAssembler::getOp2Byte(0x1f);
        m_assembler.bitAnd(ARMRegisters::S0, shiftAmount, w);

        m_assembler.movs(dest, m_assembler.asrRegister(src, ARMRegisters::S0));
    }

    void rshift32(TrustedImm32 imm, RegisterID dest)
    {
        rshift32(dest, imm, dest);
    }

    void rshift32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.movs(dest, m_assembler.asr(src, imm.m_value & 0x1f));
    }

    void urshift32(RegisterID shiftAmount, RegisterID dest)
    {
        urshift32(dest, shiftAmount, dest);
    }

    void urshift32(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        ARMWord w = ARMAssembler::getOp2Byte(0x1f);
        m_assembler.bitAnd(ARMRegisters::S0, shiftAmount, w);

        m_assembler.movs(dest, m_assembler.lsrRegister(src, ARMRegisters::S0));
    }

    void urshift32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.movs(dest, m_assembler.lsr(dest, imm.m_value & 0x1f));
    }
    
    void urshift32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.movs(dest, m_assembler.lsr(src, imm.m_value & 0x1f));
    }

    void sub32(RegisterID src, RegisterID dest)
    {
        m_assembler.subs(dest, dest, src);
    }

    void sub32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.subs(dest, dest, m_assembler.getImm(imm.m_value, ARMRegisters::S0));
    }

    void sub32(TrustedImm32 imm, Address address)
    {
        load32(address, ARMRegisters::S1);
        sub32(imm, ARMRegisters::S1);
        store32(ARMRegisters::S1, address);
    }

    void sub32(Address src, RegisterID dest)
    {
        load32(src, ARMRegisters::S1);
        sub32(ARMRegisters::S1, dest);
    }

    void sub32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.subs(dest, src, m_assembler.getImm(imm.m_value, ARMRegisters::S0));
    }

    void xor32(RegisterID src, RegisterID dest)
    {
        m_assembler.eors(dest, dest, src);
    }

    void xor32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.eors(dest, op1, op2);
    }

    void xor32(TrustedImm32 imm, RegisterID dest)
    {
        if (imm.m_value == -1)
            m_assembler.mvns(dest, dest);
        else
            m_assembler.eors(dest, dest, m_assembler.getImm(imm.m_value, ARMRegisters::S0));
    }

    void xor32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        if (imm.m_value == -1)
            m_assembler.mvns(dest, src);
        else    
            m_assembler.eors(dest, src, m_assembler.getImm(imm.m_value, ARMRegisters::S0));
    }

    void countLeadingZeros32(RegisterID src, RegisterID dest)
    {
#if WTF_ARM_ARCH_AT_LEAST(5)
        m_assembler.clz(dest, src);
#else
        UNUSED_PARAM(src);
        UNUSED_PARAM(dest);
        RELEASE_ASSERT_NOT_REACHED();
#endif
    }

    void load8(ImplicitAddress address, RegisterID dest)
    {
        m_assembler.dataTransfer32(ARMAssembler::LoadUint8, dest, address.base, address.offset);
    }

    void load8(BaseIndex address, RegisterID dest)
    {
        m_assembler.baseIndexTransfer32(ARMAssembler::LoadUint8, dest, address.base, address.index, static_cast<int>(address.scale), address.offset);
    }

    void load8Signed(BaseIndex address, RegisterID dest)
    {
        m_assembler.baseIndexTransfer16(ARMAssembler::LoadInt8, dest, address.base, address.index, static_cast<int>(address.scale), address.offset);
    }

    void load16(ImplicitAddress address, RegisterID dest)
    {
        m_assembler.dataTransfer16(ARMAssembler::LoadUint16, dest, address.base, address.offset);
    }

    void load16(BaseIndex address, RegisterID dest)
    {
        m_assembler.baseIndexTransfer16(ARMAssembler::LoadUint16, dest, address.base, address.index, static_cast<int>(address.scale), address.offset);
    }

    void load16Signed(BaseIndex address, RegisterID dest)
    {
        m_assembler.baseIndexTransfer16(ARMAssembler::LoadInt16, dest, address.base, address.index, static_cast<int>(address.scale), address.offset);
    }

    void load32(ImplicitAddress address, RegisterID dest)
    {
        m_assembler.dataTransfer32(ARMAssembler::LoadUint32, dest, address.base, address.offset);
    }

    void load32(BaseIndex address, RegisterID dest)
    {
        m_assembler.baseIndexTransfer32(ARMAssembler::LoadUint32, dest, address.base, address.index, static_cast<int>(address.scale), address.offset);
    }

#if CPU(ARMV5_OR_LOWER)
    void load32WithUnalignedHalfWords(BaseIndex address, RegisterID dest);
#else
    void load32WithUnalignedHalfWords(BaseIndex address, RegisterID dest)
    {
        load32(address, dest);
    }
#endif

    void load16Unaligned(BaseIndex address, RegisterID dest)
    {
        load16(address, dest);
    }

    ConvertibleLoadLabel convertibleLoadPtr(Address address, RegisterID dest)
    {
        ConvertibleLoadLabel result(this);
        ASSERT(address.offset >= 0 && address.offset <= 255);
        m_assembler.dtrUp(ARMAssembler::LoadUint32, dest, address.base, address.offset);
        return result;
    }

    DataLabel32 load32WithAddressOffsetPatch(Address address, RegisterID dest)
    {
        DataLabel32 dataLabel(this);
        m_assembler.ldrUniqueImmediate(ARMRegisters::S0, 0);
        m_assembler.dtrUpRegister(ARMAssembler::LoadUint32, dest, address.base, ARMRegisters::S0);
        return dataLabel;
    }

    static bool isCompactPtrAlignedAddressOffset(ptrdiff_t value)
    {
        return value >= -4095 && value <= 4095;
    }

    DataLabelCompact load32WithCompactAddressOffsetPatch(Address address, RegisterID dest)
    {
        DataLabelCompact dataLabel(this);
        ASSERT(isCompactPtrAlignedAddressOffset(address.offset));
        if (address.offset >= 0)
            m_assembler.dtrUp(ARMAssembler::LoadUint32, dest, address.base, address.offset);
        else
            m_assembler.dtrDown(ARMAssembler::LoadUint32, dest, address.base, address.offset);
        return dataLabel;
    }

    DataLabel32 store32WithAddressOffsetPatch(RegisterID src, Address address)
    {
        DataLabel32 dataLabel(this);
        m_assembler.ldrUniqueImmediate(ARMRegisters::S0, 0);
        m_assembler.dtrUpRegister(ARMAssembler::StoreUint32, src, address.base, ARMRegisters::S0);
        return dataLabel;
    }

    void store8(RegisterID src, BaseIndex address)
    {
        m_assembler.baseIndexTransfer32(ARMAssembler::StoreUint8, src, address.base, address.index, static_cast<int>(address.scale), address.offset);
    }

    void store8(TrustedImm32 imm, const void* address)
    {
        move(TrustedImm32(reinterpret_cast<ARMWord>(address)), ARMRegisters::S0);
        move(imm, ARMRegisters::S1);
        m_assembler.dtrUp(ARMAssembler::StoreUint8, ARMRegisters::S1, ARMRegisters::S0, 0);
    }

    void store16(RegisterID src, BaseIndex address)
    {
        m_assembler.baseIndexTransfer16(ARMAssembler::StoreUint16, src, address.base, address.index, static_cast<int>(address.scale), address.offset);
    }

    void store32(RegisterID src, ImplicitAddress address)
    {
        m_assembler.dataTransfer32(ARMAssembler::StoreUint32, src, address.base, address.offset);
    }

    void store32(RegisterID src, BaseIndex address)
    {
        m_assembler.baseIndexTransfer32(ARMAssembler::StoreUint32, src, address.base, address.index, static_cast<int>(address.scale), address.offset);
    }

    void store32(TrustedImm32 imm, ImplicitAddress address)
    {
        move(imm, ARMRegisters::S1);
        store32(ARMRegisters::S1, address);
    }

    void store32(TrustedImm32 imm, BaseIndex address)
    {
        move(imm, ARMRegisters::S1);
        m_assembler.baseIndexTransfer32(ARMAssembler::StoreUint32, ARMRegisters::S1, address.base, address.index, static_cast<int>(address.scale), address.offset);
    }

    void store32(RegisterID src, const void* address)
    {
        m_assembler.ldrUniqueImmediate(ARMRegisters::S0, reinterpret_cast<ARMWord>(address));
        m_assembler.dtrUp(ARMAssembler::StoreUint32, src, ARMRegisters::S0, 0);
    }

    void store32(TrustedImm32 imm, const void* address)
    {
        m_assembler.ldrUniqueImmediate(ARMRegisters::S0, reinterpret_cast<ARMWord>(address));
        m_assembler.moveImm(imm.m_value, ARMRegisters::S1);
        m_assembler.dtrUp(ARMAssembler::StoreUint32, ARMRegisters::S1, ARMRegisters::S0, 0);
    }

    void pop(RegisterID dest)
    {
        m_assembler.pop(dest);
    }

    void push(RegisterID src)
    {
        m_assembler.push(src);
    }

    void push(Address address)
    {
        load32(address, ARMRegisters::S1);
        push(ARMRegisters::S1);
    }

    void push(TrustedImm32 imm)
    {
        move(imm, ARMRegisters::S0);
        push(ARMRegisters::S0);
    }

    void move(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.moveImm(imm.m_value, dest);
    }

    void move(RegisterID src, RegisterID dest)
    {
        if (src != dest)
            m_assembler.mov(dest, src);
    }

    void move(TrustedImmPtr imm, RegisterID dest)
    {
        move(TrustedImm32(imm), dest);
    }

    void swap(RegisterID reg1, RegisterID reg2)
    {
        xor32(reg1, reg2);
        xor32(reg2, reg1);
        xor32(reg1, reg2);
    }

    void signExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        if (src != dest)
            move(src, dest);
    }

    void zeroExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        if (src != dest)
            move(src, dest);
    }

    Jump branch8(RelationalCondition cond, Address left, TrustedImm32 right)
    {
        load8(left, ARMRegisters::S1);
        return branch32(cond, ARMRegisters::S1, right);
    }

    Jump branch8(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        ASSERT(!(right.m_value & 0xFFFFFF00));
        load8(left, ARMRegisters::S1);
        return branch32(cond, ARMRegisters::S1, right);
    }

    Jump branch32(RelationalCondition cond, RegisterID left, RegisterID right, int useConstantPool = 0)
    {
        m_assembler.cmp(left, right);
        return Jump(m_assembler.jmp(ARMCondition(cond), useConstantPool));
    }

    Jump branch32(RelationalCondition cond, RegisterID left, TrustedImm32 right, int useConstantPool = 0)
    {
        internalCompare32(left, right);
        return Jump(m_assembler.jmp(ARMCondition(cond), useConstantPool));
    }

    Jump branch32(RelationalCondition cond, RegisterID left, Address right)
    {
        load32(right, ARMRegisters::S1);
        return branch32(cond, left, ARMRegisters::S1);
    }

    Jump branch32(RelationalCondition cond, Address left, RegisterID right)
    {
        load32(left, ARMRegisters::S1);
        return branch32(cond, ARMRegisters::S1, right);
    }

    Jump branch32(RelationalCondition cond, Address left, TrustedImm32 right)
    {
        load32(left, ARMRegisters::S1);
        return branch32(cond, ARMRegisters::S1, right);
    }

    Jump branch32(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        load32(left, ARMRegisters::S1);
        return branch32(cond, ARMRegisters::S1, right);
    }

    Jump branch32WithUnalignedHalfWords(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        load32WithUnalignedHalfWords(left, ARMRegisters::S1);
        return branch32(cond, ARMRegisters::S1, right);
    }

    Jump branchTest8(ResultCondition cond, Address address, TrustedImm32 mask = TrustedImm32(-1))
    {
        load8(address, ARMRegisters::S1);
        return branchTest32(cond, ARMRegisters::S1, mask);
    }

    Jump branchTest8(ResultCondition cond, AbsoluteAddress address, TrustedImm32 mask = TrustedImm32(-1))
    {
        move(TrustedImmPtr(address.m_ptr), ARMRegisters::S1);
        load8(Address(ARMRegisters::S1), ARMRegisters::S1);
        return branchTest32(cond, ARMRegisters::S1, mask);
    }

    Jump branchTest32(ResultCondition cond, RegisterID reg, RegisterID mask)
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        m_assembler.tst(reg, mask);
        return Jump(m_assembler.jmp(ARMCondition(cond)));
    }

    Jump branchTest32(ResultCondition cond, RegisterID reg, TrustedImm32 mask = TrustedImm32(-1))
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        ARMWord w = m_assembler.getImm(mask.m_value, ARMRegisters::S0, true);
        if (w & ARMAssembler::Op2InvertedImmediate)
            m_assembler.bics(ARMRegisters::S0, reg, w & ~ARMAssembler::Op2InvertedImmediate);
        else
            m_assembler.tst(reg, w);
        return Jump(m_assembler.jmp(ARMCondition(cond)));
    }

    Jump branchTest32(ResultCondition cond, Address address, TrustedImm32 mask = TrustedImm32(-1))
    {
        load32(address, ARMRegisters::S1);
        return branchTest32(cond, ARMRegisters::S1, mask);
    }

    Jump branchTest32(ResultCondition cond, BaseIndex address, TrustedImm32 mask = TrustedImm32(-1))
    {
        load32(address, ARMRegisters::S1);
        return branchTest32(cond, ARMRegisters::S1, mask);
    }

    Jump jump()
    {
        return Jump(m_assembler.jmp());
    }

    void jump(RegisterID target)
    {
        m_assembler.bx(target);
    }

    void jump(Address address)
    {
        load32(address, ARMRegisters::pc);
    }

    void jump(AbsoluteAddress address)
    {
        move(TrustedImmPtr(address.m_ptr), ARMRegisters::S0);
        load32(Address(ARMRegisters::S0, 0), ARMRegisters::pc);
    }

    void moveDoubleToInts(FPRegisterID src, RegisterID dest1, RegisterID dest2)
    {
        m_assembler.vmov(dest1, dest2, src);
    }

    void moveIntsToDouble(RegisterID src1, RegisterID src2, FPRegisterID dest, FPRegisterID)
    {
        m_assembler.vmov(dest, src1, src2);
    }

    Jump branchAdd32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero)
            || (cond == NonZero) || (cond == PositiveOrZero));
        add32(src, dest);
        return Jump(m_assembler.jmp(ARMCondition(cond)));
    }

    Jump branchAdd32(ResultCondition cond, RegisterID op1, RegisterID op2, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero)
            || (cond == NonZero) || (cond == PositiveOrZero));
        add32(op1, op2, dest);
        return Jump(m_assembler.jmp(ARMCondition(cond)));
    }

    Jump branchAdd32(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero)
            || (cond == NonZero) || (cond == PositiveOrZero));
        add32(imm, dest);
        return Jump(m_assembler.jmp(ARMCondition(cond)));
    }

    Jump branchAdd32(ResultCondition cond, RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero)
            || (cond == NonZero) || (cond == PositiveOrZero));
        add32(src, imm, dest);
        return Jump(m_assembler.jmp(ARMCondition(cond)));
    }

    Jump branchAdd32(ResultCondition cond, TrustedImm32 imm, AbsoluteAddress dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero)
            || (cond == NonZero) || (cond == PositiveOrZero));
        add32(imm, dest);
        return Jump(m_assembler.jmp(ARMCondition(cond)));
    }

    void mull32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        if (op2 == dest) {
            if (op1 == dest) {
                move(op2, ARMRegisters::S0);
                op2 = ARMRegisters::S0;
            } else {
                // Swap the operands.
                RegisterID tmp = op1;
                op1 = op2;
                op2 = tmp;
            }
        }
        m_assembler.mull(ARMRegisters::S1, dest, op1, op2);
        m_assembler.cmp(ARMRegisters::S1, m_assembler.asr(dest, 31));
    }

    Jump branchMul32(ResultCondition cond, RegisterID src1, RegisterID src2, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        if (cond == Overflow) {
            mull32(src1, src2, dest);
            cond = NonZero;
        }
        else
            mul32(src1, src2, dest);
        return Jump(m_assembler.jmp(ARMCondition(cond)));
    }

    Jump branchMul32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        return branchMul32(cond, src, dest, dest);
    }

    Jump branchMul32(ResultCondition cond, TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        if (cond == Overflow) {
            move(imm, ARMRegisters::S0);
            mull32(ARMRegisters::S0, src, dest);
            cond = NonZero;
        }
        else
            mul32(imm, src, dest);
        return Jump(m_assembler.jmp(ARMCondition(cond)));
    }

    Jump branchSub32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        sub32(src, dest);
        return Jump(m_assembler.jmp(ARMCondition(cond)));
    }

    Jump branchSub32(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        sub32(imm, dest);
        return Jump(m_assembler.jmp(ARMCondition(cond)));
    }

    Jump branchSub32(ResultCondition cond, RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        sub32(src, imm, dest);
        return Jump(m_assembler.jmp(ARMCondition(cond)));
    }

    Jump branchSub32(ResultCondition cond, RegisterID op1, RegisterID op2, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        m_assembler.subs(dest, op1, op2);
        return Jump(m_assembler.jmp(ARMCondition(cond)));
    }

    Jump branchNeg32(ResultCondition cond, RegisterID srcDest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        neg32(srcDest);
        return Jump(m_assembler.jmp(ARMCondition(cond)));
    }

    Jump branchOr32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Signed) || (cond == Zero) || (cond == NonZero));
        or32(src, dest);
        return Jump(m_assembler.jmp(ARMCondition(cond)));
    }

    PatchableJump patchableBranch32(RelationalCondition cond, RegisterID reg, TrustedImm32 imm)
    {
        internalCompare32(reg, imm);
        Jump jump(m_assembler.loadBranchTarget(ARMRegisters::S1, ARMCondition(cond), true));
        m_assembler.bx(ARMRegisters::S1, ARMCondition(cond));
        return PatchableJump(jump);
    }

    void breakpoint()
    {
        m_assembler.bkpt(0);
    }

    Call nearCall()
    {
        m_assembler.loadBranchTarget(ARMRegisters::S1, ARMAssembler::AL, true);
        return Call(m_assembler.blx(ARMRegisters::S1), Call::LinkableNear);
    }

    Call call(RegisterID target)
    {
        return Call(m_assembler.blx(target), Call::None);
    }

    void call(Address address)
    {
        call32(address.base, address.offset);
    }

    void ret()
    {
        m_assembler.bx(linkRegister);
    }

    void compare32(RelationalCondition cond, RegisterID left, RegisterID right, RegisterID dest)
    {
        m_assembler.cmp(left, right);
        m_assembler.mov(dest, ARMAssembler::getOp2Byte(0));
        m_assembler.mov(dest, ARMAssembler::getOp2Byte(1), ARMCondition(cond));
    }

    void compare32(RelationalCondition cond, RegisterID left, TrustedImm32 right, RegisterID dest)
    {
        m_assembler.cmp(left, m_assembler.getImm(right.m_value, ARMRegisters::S0));
        m_assembler.mov(dest, ARMAssembler::getOp2Byte(0));
        m_assembler.mov(dest, ARMAssembler::getOp2Byte(1), ARMCondition(cond));
    }

    void compare8(RelationalCondition cond, Address left, TrustedImm32 right, RegisterID dest)
    {
        load8(left, ARMRegisters::S1);
        compare32(cond, ARMRegisters::S1, right, dest);
    }

    void test32(ResultCondition cond, RegisterID reg, TrustedImm32 mask, RegisterID dest)
    {
        if (mask.m_value == -1)
            m_assembler.cmp(0, reg);
        else
            m_assembler.tst(reg, m_assembler.getImm(mask.m_value, ARMRegisters::S0));
        m_assembler.mov(dest, ARMAssembler::getOp2Byte(0));
        m_assembler.mov(dest, ARMAssembler::getOp2Byte(1), ARMCondition(cond));
    }

    void test32(ResultCondition cond, Address address, TrustedImm32 mask, RegisterID dest)
    {
        load32(address, ARMRegisters::S1);
        test32(cond, ARMRegisters::S1, mask, dest);
    }

    void test8(ResultCondition cond, Address address, TrustedImm32 mask, RegisterID dest)
    {
        load8(address, ARMRegisters::S1);
        test32(cond, ARMRegisters::S1, mask, dest);
    }

    void add32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        m_assembler.add(dest, src, m_assembler.getImm(imm.m_value, ARMRegisters::S0));
    }

    void add32(TrustedImm32 imm, AbsoluteAddress address)
    {
        load32(address.m_ptr, ARMRegisters::S1);
        add32(imm, ARMRegisters::S1);
        store32(ARMRegisters::S1, address.m_ptr);
    }

    void add64(TrustedImm32 imm, AbsoluteAddress address)
    {
        ARMWord tmp;

        move(TrustedImmPtr(address.m_ptr), ARMRegisters::S1);
        m_assembler.dtrUp(ARMAssembler::LoadUint32, ARMRegisters::S0, ARMRegisters::S1, 0);

        if ((tmp = ARMAssembler::getOp2(imm.m_value)) != ARMAssembler::InvalidImmediate)
            m_assembler.adds(ARMRegisters::S0, ARMRegisters::S0, tmp);
        else if ((tmp = ARMAssembler::getOp2(-imm.m_value)) != ARMAssembler::InvalidImmediate)
            m_assembler.subs(ARMRegisters::S0, ARMRegisters::S0, tmp);
        else {
            m_assembler.adds(ARMRegisters::S0, ARMRegisters::S0, m_assembler.getImm(imm.m_value, ARMRegisters::S1));
            move(TrustedImmPtr(address.m_ptr), ARMRegisters::S1);
        }
        m_assembler.dtrUp(ARMAssembler::StoreUint32, ARMRegisters::S0, ARMRegisters::S1, 0);

        m_assembler.dtrUp(ARMAssembler::LoadUint32, ARMRegisters::S0, ARMRegisters::S1, sizeof(ARMWord));
        if (imm.m_value >= 0)
            m_assembler.adc(ARMRegisters::S0, ARMRegisters::S0, ARMAssembler::getOp2Byte(0));
        else
            m_assembler.sbc(ARMRegisters::S0, ARMRegisters::S0, ARMAssembler::getOp2Byte(0));
        m_assembler.dtrUp(ARMAssembler::StoreUint32, ARMRegisters::S0, ARMRegisters::S1, sizeof(ARMWord));
    }

    void sub32(TrustedImm32 imm, AbsoluteAddress address)
    {
        load32(address.m_ptr, ARMRegisters::S1);
        sub32(imm, ARMRegisters::S1);
        store32(ARMRegisters::S1, address.m_ptr);
    }

    void load32(const void* address, RegisterID dest)
    {
        m_assembler.ldrUniqueImmediate(ARMRegisters::S0, reinterpret_cast<ARMWord>(address));
        m_assembler.dtrUp(ARMAssembler::LoadUint32, dest, ARMRegisters::S0, 0);
    }

    Jump branch32(RelationalCondition cond, AbsoluteAddress left, RegisterID right)
    {
        load32(left.m_ptr, ARMRegisters::S1);
        return branch32(cond, ARMRegisters::S1, right);
    }

    Jump branch32(RelationalCondition cond, AbsoluteAddress left, TrustedImm32 right)
    {
        load32(left.m_ptr, ARMRegisters::S1);
        return branch32(cond, ARMRegisters::S1, right);
    }

    void relativeTableJump(RegisterID index, int scale)
    {
        ASSERT(scale >= 0 && scale <= 31);
        m_assembler.add(ARMRegisters::pc, ARMRegisters::pc, m_assembler.lsl(index, scale));

        // NOP the default prefetching
        m_assembler.mov(ARMRegisters::r0, ARMRegisters::r0);
    }

    Call call()
    {
        ensureSpace(2 * sizeof(ARMWord), sizeof(ARMWord));
        m_assembler.loadBranchTarget(ARMRegisters::S1, ARMAssembler::AL, true);
        return Call(m_assembler.blx(ARMRegisters::S1), Call::Linkable);
    }

    Call tailRecursiveCall()
    {
        return Call::fromTailJump(jump());
    }

    Call makeTailRecursiveCall(Jump oldJump)
    {
        return Call::fromTailJump(oldJump);
    }

    DataLabelPtr moveWithPatch(TrustedImmPtr initialValue, RegisterID dest)
    {
        DataLabelPtr dataLabel(this);
        m_assembler.ldrUniqueImmediate(dest, reinterpret_cast<ARMWord>(initialValue.m_value));
        return dataLabel;
    }

    Jump branchPtrWithPatch(RelationalCondition cond, RegisterID left, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(0))
    {
        ensureSpace(3 * sizeof(ARMWord), 2 * sizeof(ARMWord));
        dataLabel = moveWithPatch(initialRightValue, ARMRegisters::S1);
        Jump jump = branch32(cond, left, ARMRegisters::S1, true);
        return jump;
    }

    Jump branchPtrWithPatch(RelationalCondition cond, Address left, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(0))
    {
        load32(left, ARMRegisters::S1);
        ensureSpace(3 * sizeof(ARMWord), 2 * sizeof(ARMWord));
        dataLabel = moveWithPatch(initialRightValue, ARMRegisters::S0);
        Jump jump = branch32(cond, ARMRegisters::S0, ARMRegisters::S1, true);
        return jump;
    }

    DataLabelPtr storePtrWithPatch(TrustedImmPtr initialValue, ImplicitAddress address)
    {
        DataLabelPtr dataLabel = moveWithPatch(initialValue, ARMRegisters::S1);
        store32(ARMRegisters::S1, address);
        return dataLabel;
    }

    DataLabelPtr storePtrWithPatch(ImplicitAddress address)
    {
        return storePtrWithPatch(TrustedImmPtr(0), address);
    }

    // Floating point operators
    static bool supportsFloatingPoint()
    {
        return s_isVFPPresent;
    }

    static bool supportsFloatingPointTruncate()
    {
        return false;
    }

    static bool supportsFloatingPointSqrt()
    {
        return s_isVFPPresent;
    }
    static bool supportsFloatingPointAbs() { return false; }

    void loadFloat(BaseIndex address, FPRegisterID dest)
    {
        m_assembler.baseIndexTransferFloat(ARMAssembler::LoadFloat, dest, address.base, address.index, static_cast<int>(address.scale), address.offset);
    }

    void loadDouble(ImplicitAddress address, FPRegisterID dest)
    {
        m_assembler.dataTransferFloat(ARMAssembler::LoadDouble, dest, address.base, address.offset);
    }

    void loadDouble(BaseIndex address, FPRegisterID dest)
    {
        m_assembler.baseIndexTransferFloat(ARMAssembler::LoadDouble, dest, address.base, address.index, static_cast<int>(address.scale), address.offset);
    }

    void loadDouble(const void* address, FPRegisterID dest)
    {
        move(TrustedImm32(reinterpret_cast<ARMWord>(address)), ARMRegisters::S0);
        m_assembler.doubleDtrUp(ARMAssembler::LoadDouble, dest, ARMRegisters::S0, 0);
    }

    void storeFloat(FPRegisterID src, BaseIndex address)
    {
        m_assembler.baseIndexTransferFloat(ARMAssembler::StoreFloat, src, address.base, address.index, static_cast<int>(address.scale), address.offset);
    }

    void storeDouble(FPRegisterID src, ImplicitAddress address)
    {
        m_assembler.dataTransferFloat(ARMAssembler::StoreDouble, src, address.base, address.offset);
    }

    void storeDouble(FPRegisterID src, BaseIndex address)
    {
        m_assembler.baseIndexTransferFloat(ARMAssembler::StoreDouble, src, address.base, address.index, static_cast<int>(address.scale), address.offset);
    }

    void storeDouble(FPRegisterID src, const void* address)
    {
        move(TrustedImm32(reinterpret_cast<ARMWord>(address)), ARMRegisters::S0);
        m_assembler.dataTransferFloat(ARMAssembler::StoreDouble, src, ARMRegisters::S0, 0);
    }

    void moveDouble(FPRegisterID src, FPRegisterID dest)
    {
        if (src != dest)
            m_assembler.vmov_f64(dest, src);
    }

    void addDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vadd_f64(dest, dest, src);
    }

    void addDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.vadd_f64(dest, op1, op2);
    }

    void addDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, ARMRegisters::SD0);
        addDouble(ARMRegisters::SD0, dest);
    }

    void addDouble(AbsoluteAddress address, FPRegisterID dest)
    {
        loadDouble(address.m_ptr, ARMRegisters::SD0);
        addDouble(ARMRegisters::SD0, dest);
    }

    void divDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vdiv_f64(dest, dest, src);
    }

    void divDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.vdiv_f64(dest, op1, op2);
    }

    void divDouble(Address src, FPRegisterID dest)
    {
        RELEASE_ASSERT_NOT_REACHED(); // Untested
        loadDouble(src, ARMRegisters::SD0);
        divDouble(ARMRegisters::SD0, dest);
    }

    void subDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vsub_f64(dest, dest, src);
    }

    void subDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.vsub_f64(dest, op1, op2);
    }

    void subDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, ARMRegisters::SD0);
        subDouble(ARMRegisters::SD0, dest);
    }

    void mulDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vmul_f64(dest, dest, src);
    }

    void mulDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, ARMRegisters::SD0);
        mulDouble(ARMRegisters::SD0, dest);
    }

    void mulDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.vmul_f64(dest, op1, op2);
    }

    void sqrtDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vsqrt_f64(dest, src);
    }
    
    void absDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vabs_f64(dest, src);
    }

    void negateDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vneg_f64(dest, src);
    }

    void convertInt32ToDouble(RegisterID src, FPRegisterID dest)
    {
        m_assembler.vmov_vfp32(dest << 1, src);
        m_assembler.vcvt_f64_s32(dest, dest << 1);
    }

    void convertInt32ToDouble(Address src, FPRegisterID dest)
    {
        load32(src, ARMRegisters::S1);
        convertInt32ToDouble(ARMRegisters::S1, dest);
    }

    void convertInt32ToDouble(AbsoluteAddress src, FPRegisterID dest)
    {
        move(TrustedImmPtr(src.m_ptr), ARMRegisters::S1);
        load32(Address(ARMRegisters::S1), ARMRegisters::S1);
        convertInt32ToDouble(ARMRegisters::S1, dest);
    }

    void convertFloatToDouble(FPRegisterID src, FPRegisterID dst)
    {
        m_assembler.vcvt_f64_f32(dst, src);
    }

    void convertDoubleToFloat(FPRegisterID src, FPRegisterID dst)
    {
        m_assembler.vcvt_f32_f64(dst, src);
    }

    Jump branchDouble(DoubleCondition cond, FPRegisterID left, FPRegisterID right)
    {
        m_assembler.vcmp_f64(left, right);
        m_assembler.vmrs_apsr();
        if (cond & DoubleConditionBitSpecial)
            m_assembler.cmp(ARMRegisters::S0, ARMRegisters::S0, ARMAssembler::VS);
        return Jump(m_assembler.jmp(static_cast<ARMAssembler::Condition>(cond & ~DoubleConditionMask)));
    }

    // Truncates 'src' to an integer, and places the resulting 'dest'.
    // If the result is not representable as a 32 bit value, branch.
    // May also branch for some values that are representable in 32 bits
    // (specifically, in this case, INT_MIN).
    enum BranchTruncateType { BranchIfTruncateFailed, BranchIfTruncateSuccessful };
    Jump branchTruncateDoubleToInt32(FPRegisterID src, RegisterID dest, BranchTruncateType branchType = BranchIfTruncateFailed)
    {
        truncateDoubleToInt32(src, dest);

        m_assembler.add(ARMRegisters::S0, dest, ARMAssembler::getOp2Byte(1));
        m_assembler.bic(ARMRegisters::S0, ARMRegisters::S0, ARMAssembler::getOp2Byte(1));

        ARMWord w = ARMAssembler::getOp2(0x80000000);
        ASSERT(w != ARMAssembler::InvalidImmediate);
        m_assembler.cmp(ARMRegisters::S0, w);
        return Jump(m_assembler.jmp(branchType == BranchIfTruncateFailed ? ARMAssembler::EQ : ARMAssembler::NE));
    }

    Jump branchTruncateDoubleToUint32(FPRegisterID src, RegisterID dest, BranchTruncateType branchType = BranchIfTruncateFailed)
    {
        truncateDoubleToUint32(src, dest);

        m_assembler.add(ARMRegisters::S0, dest, ARMAssembler::getOp2Byte(1));
        m_assembler.bic(ARMRegisters::S0, ARMRegisters::S0, ARMAssembler::getOp2Byte(1));

        m_assembler.cmp(ARMRegisters::S0, ARMAssembler::getOp2Byte(0));
        return Jump(m_assembler.jmp(branchType == BranchIfTruncateFailed ? ARMAssembler::EQ : ARMAssembler::NE));
    }

    // Result is undefined if the value is outside of the integer range.
    void truncateDoubleToInt32(FPRegisterID src, RegisterID dest)
    {
        m_assembler.vcvt_s32_f64(ARMRegisters::SD0 << 1, src);
        m_assembler.vmov_arm32(dest, ARMRegisters::SD0 << 1);
    }

    void truncateDoubleToUint32(FPRegisterID src, RegisterID dest)
    {
        m_assembler.vcvt_u32_f64(ARMRegisters::SD0 << 1, src);
        m_assembler.vmov_arm32(dest, ARMRegisters::SD0 << 1);
    }

    // Convert 'src' to an integer, and places the resulting 'dest'.
    // If the result is not representable as a 32 bit value, branch.
    // May also branch for some values that are representable in 32 bits
    // (specifically, in this case, 0).
    void branchConvertDoubleToInt32(FPRegisterID src, RegisterID dest, JumpList& failureCases, FPRegisterID, bool negZeroCheck = true)
    {
        m_assembler.vcvt_s32_f64(ARMRegisters::SD0 << 1, src);
        m_assembler.vmov_arm32(dest, ARMRegisters::SD0 << 1);

        // Convert the integer result back to float & compare to the original value - if not equal or unordered (NaN) then jump.
        m_assembler.vcvt_f64_s32(ARMRegisters::SD0, ARMRegisters::SD0 << 1);
        failureCases.append(branchDouble(DoubleNotEqualOrUnordered, src, ARMRegisters::SD0));

        // If the result is zero, it might have been -0.0, and 0.0 equals to -0.0
        if (negZeroCheck)
            failureCases.append(branchTest32(Zero, dest));
    }

    Jump branchDoubleNonZero(FPRegisterID reg, FPRegisterID scratch)
    {
        m_assembler.mov(ARMRegisters::S0, ARMAssembler::getOp2Byte(0));
        convertInt32ToDouble(ARMRegisters::S0, scratch);
        return branchDouble(DoubleNotEqual, reg, scratch);
    }

    Jump branchDoubleZeroOrNaN(FPRegisterID reg, FPRegisterID scratch)
    {
        m_assembler.mov(ARMRegisters::S0, ARMAssembler::getOp2Byte(0));
        convertInt32ToDouble(ARMRegisters::S0, scratch);
        return branchDouble(DoubleEqualOrUnordered, reg, scratch);
    }

    // Invert a relational condition, e.g. == becomes !=, < becomes >=, etc.
    static RelationalCondition invert(RelationalCondition cond)
    {
        ASSERT((static_cast<uint32_t>(cond & 0x0fffffff)) == 0 && static_cast<uint32_t>(cond) < static_cast<uint32_t>(ARMAssembler::AL));
        return static_cast<RelationalCondition>(cond ^ 0x10000000);
    }

    void nop()
    {
        m_assembler.nop();
    }

    static FunctionPtr readCallTarget(CodeLocationCall call)
    {
        return FunctionPtr(reinterpret_cast<void(*)()>(ARMAssembler::readCallTarget(call.dataLocation())));
    }

    static void replaceWithJump(CodeLocationLabel instructionStart, CodeLocationLabel destination)
    {
        ARMAssembler::replaceWithJump(instructionStart.dataLocation(), destination.dataLocation());
    }
    
    static ptrdiff_t maxJumpReplacementSize()
    {
        ARMAssembler::maxJumpReplacementSize();
        return 0;
    }

    static bool canJumpReplacePatchableBranchPtrWithPatch() { return false; }

    static CodeLocationLabel startOfPatchableBranchPtrWithPatchOnAddress(CodeLocationDataLabelPtr)
    {
        UNREACHABLE_FOR_PLATFORM();
        return CodeLocationLabel();
    }

    static CodeLocationLabel startOfBranchPtrWithPatchOnRegister(CodeLocationDataLabelPtr label)
    {
        return label.labelAtOffset(0);
    }

    static void revertJumpReplacementToBranchPtrWithPatch(CodeLocationLabel instructionStart, RegisterID reg, void* initialValue)
    {
        ARMAssembler::revertBranchPtrWithPatch(instructionStart.dataLocation(), reg, reinterpret_cast<uintptr_t>(initialValue) & 0xffff);
    }

    static void revertJumpReplacementToPatchableBranchPtrWithPatch(CodeLocationLabel, Address, void*)
    {
        UNREACHABLE_FOR_PLATFORM();
    }

protected:
    ARMAssembler::Condition ARMCondition(RelationalCondition cond)
    {
        return static_cast<ARMAssembler::Condition>(cond);
    }

    ARMAssembler::Condition ARMCondition(ResultCondition cond)
    {
        return static_cast<ARMAssembler::Condition>(cond);
    }

    void ensureSpace(int insnSpace, int constSpace)
    {
        m_assembler.ensureSpace(insnSpace, constSpace);
    }

    int sizeOfConstantPool()
    {
        return m_assembler.sizeOfConstantPool();
    }

    void call32(RegisterID base, int32_t offset)
    {
        load32(Address(base, offset), ARMRegisters::S1);
        m_assembler.blx(ARMRegisters::S1);
    }

private:
    friend class LinkBuffer;
    friend class RepatchBuffer;

    void internalCompare32(RegisterID left, TrustedImm32 right)
    {
        ARMWord tmp = (static_cast<unsigned>(right.m_value) == 0x80000000) ? ARMAssembler::InvalidImmediate : m_assembler.getOp2(-right.m_value);
        if (tmp != ARMAssembler::InvalidImmediate)
            m_assembler.cmn(left, tmp);
        else
            m_assembler.cmp(left, m_assembler.getImm(right.m_value, ARMRegisters::S0));
    }

    static void linkCall(void* code, Call call, FunctionPtr function)
    {
        ARMAssembler::linkCall(code, call.m_label, function.value());
    }

    static void repatchCall(CodeLocationCall call, CodeLocationLabel destination)
    {
        ARMAssembler::relinkCall(call.dataLocation(), destination.executableAddress());
    }

    static void repatchCall(CodeLocationCall call, FunctionPtr destination)
    {
        ARMAssembler::relinkCall(call.dataLocation(), destination.executableAddress());
    }

    static const bool s_isVFPPresent;
};

}

#endif // ENABLE(ASSEMBLER) && CPU(ARM_TRADITIONAL)

#endif // MacroAssemblerARM_h
