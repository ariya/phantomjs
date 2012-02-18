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
        m_assembler.adds_r(dest, dest, src);
    }

    void add32(TrustedImm32 imm, Address address)
    {
        load32(address, ARMRegisters::S1);
        add32(imm, ARMRegisters::S1);
        store32(ARMRegisters::S1, address);
    }

    void add32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.adds_r(dest, dest, m_assembler.getImm(imm.m_value, ARMRegisters::S0));
    }

    void add32(Address src, RegisterID dest)
    {
        load32(src, ARMRegisters::S1);
        add32(ARMRegisters::S1, dest);
    }

    void and32(RegisterID src, RegisterID dest)
    {
        m_assembler.ands_r(dest, dest, src);
    }

    void and32(TrustedImm32 imm, RegisterID dest)
    {
        ARMWord w = m_assembler.getImm(imm.m_value, ARMRegisters::S0, true);
        if (w & ARMAssembler::OP2_INV_IMM)
            m_assembler.bics_r(dest, dest, w & ~ARMAssembler::OP2_INV_IMM);
        else
            m_assembler.ands_r(dest, dest, w);
    }

    void lshift32(RegisterID shift_amount, RegisterID dest)
    {
        ARMWord w = ARMAssembler::getOp2(0x1f);
        ASSERT(w != ARMAssembler::INVALID_IMM);
        m_assembler.and_r(ARMRegisters::S0, shift_amount, w);

        m_assembler.movs_r(dest, m_assembler.lsl_r(dest, ARMRegisters::S0));
    }

    void lshift32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.movs_r(dest, m_assembler.lsl(dest, imm.m_value & 0x1f));
    }

    void mul32(RegisterID src, RegisterID dest)
    {
        if (src == dest) {
            move(src, ARMRegisters::S0);
            src = ARMRegisters::S0;
        }
        m_assembler.muls_r(dest, dest, src);
    }

    void mul32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        move(imm, ARMRegisters::S0);
        m_assembler.muls_r(dest, src, ARMRegisters::S0);
    }

    void neg32(RegisterID srcDest)
    {
        m_assembler.rsbs_r(srcDest, srcDest, ARMAssembler::getOp2(0));
    }

    void not32(RegisterID dest)
    {
        m_assembler.mvns_r(dest, dest);
    }

    void or32(RegisterID src, RegisterID dest)
    {
        m_assembler.orrs_r(dest, dest, src);
    }

    void or32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.orrs_r(dest, dest, m_assembler.getImm(imm.m_value, ARMRegisters::S0));
    }

    void rshift32(RegisterID shift_amount, RegisterID dest)
    {
        ARMWord w = ARMAssembler::getOp2(0x1f);
        ASSERT(w != ARMAssembler::INVALID_IMM);
        m_assembler.and_r(ARMRegisters::S0, shift_amount, w);

        m_assembler.movs_r(dest, m_assembler.asr_r(dest, ARMRegisters::S0));
    }

    void rshift32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.movs_r(dest, m_assembler.asr(dest, imm.m_value & 0x1f));
    }
    
    void urshift32(RegisterID shift_amount, RegisterID dest)
    {
        ARMWord w = ARMAssembler::getOp2(0x1f);
        ASSERT(w != ARMAssembler::INVALID_IMM);
        m_assembler.and_r(ARMRegisters::S0, shift_amount, w);
        
        m_assembler.movs_r(dest, m_assembler.lsr_r(dest, ARMRegisters::S0));
    }
    
    void urshift32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.movs_r(dest, m_assembler.lsr(dest, imm.m_value & 0x1f));
    }

    void sub32(RegisterID src, RegisterID dest)
    {
        m_assembler.subs_r(dest, dest, src);
    }

    void sub32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.subs_r(dest, dest, m_assembler.getImm(imm.m_value, ARMRegisters::S0));
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

    void xor32(RegisterID src, RegisterID dest)
    {
        m_assembler.eors_r(dest, dest, src);
    }

    void xor32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.eors_r(dest, dest, m_assembler.getImm(imm.m_value, ARMRegisters::S0));
    }

    void countLeadingZeros32(RegisterID src, RegisterID dest)
    {
#if WTF_ARM_ARCH_AT_LEAST(5)
        m_assembler.clz_r(dest, src);
#else
        UNUSED_PARAM(src);
        UNUSED_PARAM(dest);
        ASSERT_NOT_REACHED();
#endif
    }

    void load8(ImplicitAddress address, RegisterID dest)
    {
        m_assembler.dataTransfer32(true, dest, address.base, address.offset, true);
    }

    void load32(ImplicitAddress address, RegisterID dest)
    {
        m_assembler.dataTransfer32(true, dest, address.base, address.offset);
    }

    void load32(BaseIndex address, RegisterID dest)
    {
        m_assembler.baseIndexTransfer32(true, dest, address.base, address.index, static_cast<int>(address.scale), address.offset);
    }

#if CPU(ARMV5_OR_LOWER)
    void load32WithUnalignedHalfWords(BaseIndex address, RegisterID dest);
#else
    void load32WithUnalignedHalfWords(BaseIndex address, RegisterID dest)
    {
        load32(address, dest);
    }
#endif

    DataLabel32 load32WithAddressOffsetPatch(Address address, RegisterID dest)
    {
        DataLabel32 dataLabel(this);
        m_assembler.ldr_un_imm(ARMRegisters::S0, 0);
        m_assembler.dtr_ur(true, dest, address.base, ARMRegisters::S0);
        return dataLabel;
    }

    void load16(BaseIndex address, RegisterID dest)
    {
        m_assembler.add_r(ARMRegisters::S1, address.base, m_assembler.lsl(address.index, address.scale));
        load16(Address(ARMRegisters::S1, address.offset), dest);
    }
    
    void load16(ImplicitAddress address, RegisterID dest)
    {
        if (address.offset >= 0)
            m_assembler.ldrh_u(dest, address.base, m_assembler.getOffsetForHalfwordDataTransfer(address.offset, ARMRegisters::S0));
        else
            m_assembler.ldrh_d(dest, address.base, m_assembler.getOffsetForHalfwordDataTransfer(-address.offset, ARMRegisters::S0));
    }

    DataLabel32 store32WithAddressOffsetPatch(RegisterID src, Address address)
    {
        DataLabel32 dataLabel(this);
        m_assembler.ldr_un_imm(ARMRegisters::S0, 0);
        m_assembler.dtr_ur(false, src, address.base, ARMRegisters::S0);
        return dataLabel;
    }

    void store32(RegisterID src, ImplicitAddress address)
    {
        m_assembler.dataTransfer32(false, src, address.base, address.offset);
    }

    void store32(RegisterID src, BaseIndex address)
    {
        m_assembler.baseIndexTransfer32(false, src, address.base, address.index, static_cast<int>(address.scale), address.offset);
    }

    void store32(TrustedImm32 imm, ImplicitAddress address)
    {
        if (imm.m_isPointer)
            m_assembler.ldr_un_imm(ARMRegisters::S1, imm.m_value);
        else
            move(imm, ARMRegisters::S1);
        store32(ARMRegisters::S1, address);
    }

    void store32(RegisterID src, void* address)
    {
        m_assembler.ldr_un_imm(ARMRegisters::S0, reinterpret_cast<ARMWord>(address));
        m_assembler.dtr_u(false, src, ARMRegisters::S0, 0);
    }

    void store32(TrustedImm32 imm, void* address)
    {
        m_assembler.ldr_un_imm(ARMRegisters::S0, reinterpret_cast<ARMWord>(address));
        if (imm.m_isPointer)
            m_assembler.ldr_un_imm(ARMRegisters::S1, imm.m_value);
        else
            m_assembler.moveImm(imm.m_value, ARMRegisters::S1);
        m_assembler.dtr_u(false, ARMRegisters::S1, ARMRegisters::S0, 0);
    }

    void pop(RegisterID dest)
    {
        m_assembler.pop_r(dest);
    }

    void push(RegisterID src)
    {
        m_assembler.push_r(src);
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
        if (imm.m_isPointer)
            m_assembler.ldr_un_imm(dest, imm.m_value);
        else
            m_assembler.moveImm(imm.m_value, dest);
    }

    void move(RegisterID src, RegisterID dest)
    {
        m_assembler.mov_r(dest, src);
    }

    void move(TrustedImmPtr imm, RegisterID dest)
    {
        move(TrustedImm32(imm), dest);
    }

    void swap(RegisterID reg1, RegisterID reg2)
    {
        m_assembler.mov_r(ARMRegisters::S0, reg1);
        m_assembler.mov_r(reg1, reg2);
        m_assembler.mov_r(reg2, ARMRegisters::S0);
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

    Jump branch32(RelationalCondition cond, RegisterID left, RegisterID right, int useConstantPool = 0)
    {
        m_assembler.cmp_r(left, right);
        return Jump(m_assembler.jmp(ARMCondition(cond), useConstantPool));
    }

    Jump branch32(RelationalCondition cond, RegisterID left, TrustedImm32 right, int useConstantPool = 0)
    {
        if (right.m_isPointer) {
            m_assembler.ldr_un_imm(ARMRegisters::S0, right.m_value);
            m_assembler.cmp_r(left, ARMRegisters::S0);
        } else {
            ARMWord tmp = m_assembler.getOp2(-right.m_value);
            if (tmp != ARMAssembler::INVALID_IMM)
                m_assembler.cmn_r(left, tmp);
            else
                m_assembler.cmp_r(left, m_assembler.getImm(right.m_value, ARMRegisters::S0));
        }
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

    Jump branch16(RelationalCondition cond, BaseIndex left, RegisterID right)
    {
        UNUSED_PARAM(cond);
        UNUSED_PARAM(left);
        UNUSED_PARAM(right);
        ASSERT_NOT_REACHED();
        return jump();
    }

    Jump branch16(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        load16(left, ARMRegisters::S0);
        move(right, ARMRegisters::S1);
        m_assembler.cmp_r(ARMRegisters::S0, ARMRegisters::S1);
        return m_assembler.jmp(ARMCondition(cond));
    }

    Jump branchTest8(ResultCondition cond, Address address, TrustedImm32 mask = TrustedImm32(-1))
    {
        load8(address, ARMRegisters::S1);
        return branchTest32(cond, ARMRegisters::S1, mask);
    }

    Jump branchTest32(ResultCondition cond, RegisterID reg, RegisterID mask)
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        m_assembler.tst_r(reg, mask);
        return Jump(m_assembler.jmp(ARMCondition(cond)));
    }

    Jump branchTest32(ResultCondition cond, RegisterID reg, TrustedImm32 mask = TrustedImm32(-1))
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        ARMWord w = m_assembler.getImm(mask.m_value, ARMRegisters::S0, true);
        if (w & ARMAssembler::OP2_INV_IMM)
            m_assembler.bics_r(ARMRegisters::S0, reg, w & ~ARMAssembler::OP2_INV_IMM);
        else
            m_assembler.tst_r(reg, w);
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

    Jump branchAdd32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        add32(src, dest);
        return Jump(m_assembler.jmp(ARMCondition(cond)));
    }

    Jump branchAdd32(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        add32(imm, dest);
        return Jump(m_assembler.jmp(ARMCondition(cond)));
    }

    void mull32(RegisterID src1, RegisterID src2, RegisterID dest)
    {
        if (src1 == dest) {
            move(src1, ARMRegisters::S0);
            src1 = ARMRegisters::S0;
        }
        m_assembler.mull_r(ARMRegisters::S1, dest, src2, src1);
        m_assembler.cmp_r(ARMRegisters::S1, m_assembler.asr(dest, 31));
    }

    Jump branchMul32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        if (cond == Overflow) {
            mull32(src, dest, dest);
            cond = NonZero;
        }
        else
            mul32(src, dest);
        return Jump(m_assembler.jmp(ARMCondition(cond)));
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

    void breakpoint()
    {
        m_assembler.bkpt(0);
    }

    Call nearCall()
    {
#if WTF_ARM_ARCH_AT_LEAST(5)
        ensureSpace(2 * sizeof(ARMWord), sizeof(ARMWord));
        m_assembler.loadBranchTarget(ARMRegisters::S1, ARMAssembler::AL, true);
        return Call(m_assembler.blx(ARMRegisters::S1), Call::LinkableNear);
#else
        prepareCall();
        return Call(m_assembler.jmp(ARMAssembler::AL, true), Call::LinkableNear);
#endif
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
        m_assembler.cmp_r(left, right);
        m_assembler.mov_r(dest, ARMAssembler::getOp2(0));
        m_assembler.mov_r(dest, ARMAssembler::getOp2(1), ARMCondition(cond));
    }

    void compare32(RelationalCondition cond, RegisterID left, TrustedImm32 right, RegisterID dest)
    {
        m_assembler.cmp_r(left, m_assembler.getImm(right.m_value, ARMRegisters::S0));
        m_assembler.mov_r(dest, ARMAssembler::getOp2(0));
        m_assembler.mov_r(dest, ARMAssembler::getOp2(1), ARMCondition(cond));
    }

    void test32(ResultCondition cond, RegisterID reg, TrustedImm32 mask, RegisterID dest)
    {
        if (mask.m_value == -1)
            m_assembler.cmp_r(0, reg);
        else
            m_assembler.tst_r(reg, m_assembler.getImm(mask.m_value, ARMRegisters::S0));
        m_assembler.mov_r(dest, ARMAssembler::getOp2(0));
        m_assembler.mov_r(dest, ARMAssembler::getOp2(1), ARMCondition(cond));
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
        m_assembler.add_r(dest, src, m_assembler.getImm(imm.m_value, ARMRegisters::S0));
    }

    void add32(TrustedImm32 imm, AbsoluteAddress address)
    {
        m_assembler.ldr_un_imm(ARMRegisters::S1, reinterpret_cast<ARMWord>(address.m_ptr));
        m_assembler.dtr_u(true, ARMRegisters::S1, ARMRegisters::S1, 0);
        add32(imm, ARMRegisters::S1);
        m_assembler.ldr_un_imm(ARMRegisters::S0, reinterpret_cast<ARMWord>(address.m_ptr));
        m_assembler.dtr_u(false, ARMRegisters::S1, ARMRegisters::S0, 0);
    }

    void sub32(TrustedImm32 imm, AbsoluteAddress address)
    {
        m_assembler.ldr_un_imm(ARMRegisters::S1, reinterpret_cast<ARMWord>(address.m_ptr));
        m_assembler.dtr_u(true, ARMRegisters::S1, ARMRegisters::S1, 0);
        sub32(imm, ARMRegisters::S1);
        m_assembler.ldr_un_imm(ARMRegisters::S0, reinterpret_cast<ARMWord>(address.m_ptr));
        m_assembler.dtr_u(false, ARMRegisters::S1, ARMRegisters::S0, 0);
    }

    void load32(const void* address, RegisterID dest)
    {
        m_assembler.ldr_un_imm(ARMRegisters::S0, reinterpret_cast<ARMWord>(address));
        m_assembler.dtr_u(true, dest, ARMRegisters::S0, 0);
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
        m_assembler.add_r(ARMRegisters::pc, ARMRegisters::pc, m_assembler.lsl(index, scale));

        // NOP the default prefetching
        m_assembler.mov_r(ARMRegisters::r0, ARMRegisters::r0);
    }

    Call call()
    {
#if WTF_ARM_ARCH_AT_LEAST(5)
        ensureSpace(2 * sizeof(ARMWord), sizeof(ARMWord));
        m_assembler.loadBranchTarget(ARMRegisters::S1, ARMAssembler::AL, true);
        return Call(m_assembler.blx(ARMRegisters::S1), Call::Linkable);
#else
        prepareCall();
        return Call(m_assembler.jmp(ARMAssembler::AL, true), Call::Linkable);
#endif
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
        m_assembler.ldr_un_imm(dest, reinterpret_cast<ARMWord>(initialValue.m_value));
        return dataLabel;
    }

    Jump branchPtrWithPatch(RelationalCondition cond, RegisterID left, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(0))
    {
        dataLabel = moveWithPatch(initialRightValue, ARMRegisters::S1);
        Jump jump = branch32(cond, left, ARMRegisters::S1, true);
        return jump;
    }

    Jump branchPtrWithPatch(RelationalCondition cond, Address left, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(0))
    {
        load32(left, ARMRegisters::S1);
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
    bool supportsFloatingPoint() const
    {
        return s_isVFPPresent;
    }

    bool supportsFloatingPointTruncate() const
    {
        return s_isVFPPresent;
    }

    bool supportsFloatingPointSqrt() const
    {
        return s_isVFPPresent;
    }

    void loadDouble(ImplicitAddress address, FPRegisterID dest)
    {
        m_assembler.doubleTransfer(true, dest, address.base, address.offset);
    }

    void loadDouble(const void* address, FPRegisterID dest)
    {
        m_assembler.ldr_un_imm(ARMRegisters::S0, (ARMWord)address);
        m_assembler.fdtr_u(true, dest, ARMRegisters::S0, 0);
    }

    void storeDouble(FPRegisterID src, ImplicitAddress address)
    {
        m_assembler.doubleTransfer(false, src, address.base, address.offset);
    }

    void addDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vadd_f64_r(dest, dest, src);
    }

    void addDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, ARMRegisters::SD0);
        addDouble(ARMRegisters::SD0, dest);
    }

    void divDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vdiv_f64_r(dest, dest, src);
    }

    void divDouble(Address src, FPRegisterID dest)
    {
        ASSERT_NOT_REACHED(); // Untested
        loadDouble(src, ARMRegisters::SD0);
        divDouble(ARMRegisters::SD0, dest);
    }

    void subDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vsub_f64_r(dest, dest, src);
    }

    void subDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, ARMRegisters::SD0);
        subDouble(ARMRegisters::SD0, dest);
    }

    void mulDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vmul_f64_r(dest, dest, src);
    }

    void mulDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, ARMRegisters::SD0);
        mulDouble(ARMRegisters::SD0, dest);
    }

    void sqrtDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vsqrt_f64_r(dest, src);
    }

    void convertInt32ToDouble(RegisterID src, FPRegisterID dest)
    {
        m_assembler.vmov_vfp_r(dest << 1, src);
        m_assembler.vcvt_f64_s32_r(dest, dest << 1);
    }

    void convertInt32ToDouble(Address src, FPRegisterID dest)
    {
        ASSERT_NOT_REACHED(); // Untested
        // flds does not worth the effort here
        load32(src, ARMRegisters::S1);
        convertInt32ToDouble(ARMRegisters::S1, dest);
    }

    void convertInt32ToDouble(AbsoluteAddress src, FPRegisterID dest)
    {
        ASSERT_NOT_REACHED(); // Untested
        // flds does not worth the effort here
        m_assembler.ldr_un_imm(ARMRegisters::S1, (ARMWord)src.m_ptr);
        m_assembler.dtr_u(true, ARMRegisters::S1, ARMRegisters::S1, 0);
        convertInt32ToDouble(ARMRegisters::S1, dest);
    }

    Jump branchDouble(DoubleCondition cond, FPRegisterID left, FPRegisterID right)
    {
        m_assembler.vcmp_f64_r(left, right);
        m_assembler.vmrs_apsr();
        if (cond & DoubleConditionBitSpecial)
            m_assembler.cmp_r(ARMRegisters::S0, ARMRegisters::S0, ARMAssembler::VS);
        return Jump(m_assembler.jmp(static_cast<ARMAssembler::Condition>(cond & ~DoubleConditionMask)));
    }

    // Truncates 'src' to an integer, and places the resulting 'dest'.
    // If the result is not representable as a 32 bit value, branch.
    // May also branch for some values that are representable in 32 bits
    // (specifically, in this case, INT_MIN and INT_MAX).
    Jump branchTruncateDoubleToInt32(FPRegisterID src, RegisterID dest)
    {
        m_assembler.vcvtr_s32_f64_r(ARMRegisters::SD0 << 1, src);
        // If VCVTR.S32.F64 can't fit the result into a 32-bit
        // integer, it saturates at INT_MAX or INT_MIN. Testing this is
        // probably quicker than testing FPSCR for exception.
        m_assembler.vmov_arm_r(dest, ARMRegisters::SD0 << 1);
        m_assembler.sub_r(ARMRegisters::S0, dest, ARMAssembler::getOp2(0x80000000));
        m_assembler.cmn_r(ARMRegisters::S0, ARMAssembler::getOp2(1), ARMCondition(NotEqual));
        return Jump(m_assembler.jmp(ARMCondition(Equal)));
    }

    // Convert 'src' to an integer, and places the resulting 'dest'.
    // If the result is not representable as a 32 bit value, branch.
    // May also branch for some values that are representable in 32 bits
    // (specifically, in this case, 0).
    void branchConvertDoubleToInt32(FPRegisterID src, RegisterID dest, JumpList& failureCases, FPRegisterID fpTemp)
    {
        m_assembler.vcvt_s32_f64_r(ARMRegisters::SD0 << 1, src);
        m_assembler.vmov_arm_r(dest, ARMRegisters::SD0 << 1);

        // Convert the integer result back to float & compare to the original value - if not equal or unordered (NaN) then jump.
        m_assembler.vcvt_f64_s32_r(ARMRegisters::SD0, ARMRegisters::SD0 << 1);
        failureCases.append(branchDouble(DoubleNotEqualOrUnordered, src, ARMRegisters::SD0));

        // If the result is zero, it might have been -0.0, and 0.0 equals to -0.0
        failureCases.append(branchTest32(Zero, dest));
    }

    Jump branchDoubleNonZero(FPRegisterID reg, FPRegisterID scratch)
    {
        m_assembler.mov_r(ARMRegisters::S0, ARMAssembler::getOp2(0));
        convertInt32ToDouble(ARMRegisters::S0, scratch);
        return branchDouble(DoubleNotEqual, reg, scratch);
    }

    Jump branchDoubleZeroOrNaN(FPRegisterID reg, FPRegisterID scratch)
    {
        m_assembler.mov_r(ARMRegisters::S0, ARMAssembler::getOp2(0));
        convertInt32ToDouble(ARMRegisters::S0, scratch);
        return branchDouble(DoubleEqualOrUnordered, reg, scratch);
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

    void prepareCall()
    {
#if WTF_ARM_ARCH_VERSION < 5
        ensureSpace(2 * sizeof(ARMWord), sizeof(ARMWord));

        m_assembler.mov_r(linkRegister, ARMRegisters::pc);
#endif
    }

    void call32(RegisterID base, int32_t offset)
    {
#if WTF_ARM_ARCH_AT_LEAST(5)
        int targetReg = ARMRegisters::S1;
#else
        int targetReg = ARMRegisters::pc;
#endif
        int tmpReg = ARMRegisters::S1;

        if (base == ARMRegisters::sp)
            offset += 4;

        if (offset >= 0) {
            if (offset <= 0xfff) {
                prepareCall();
                m_assembler.dtr_u(true, targetReg, base, offset);
            } else if (offset <= 0xfffff) {
                m_assembler.add_r(tmpReg, base, ARMAssembler::OP2_IMM | (offset >> 12) | (10 << 8));
                prepareCall();
                m_assembler.dtr_u(true, targetReg, tmpReg, offset & 0xfff);
            } else {
                m_assembler.moveImm(offset, tmpReg);
                prepareCall();
                m_assembler.dtr_ur(true, targetReg, base, tmpReg);
            }
        } else  {
            offset = -offset;
            if (offset <= 0xfff) {
                prepareCall();
                m_assembler.dtr_d(true, targetReg, base, offset);
            } else if (offset <= 0xfffff) {
                m_assembler.sub_r(tmpReg, base, ARMAssembler::OP2_IMM | (offset >> 12) | (10 << 8));
                prepareCall();
                m_assembler.dtr_d(true, targetReg, tmpReg, offset & 0xfff);
            } else {
                m_assembler.moveImm(offset, tmpReg);
                prepareCall();
                m_assembler.dtr_dr(true, targetReg, base, tmpReg);
            }
        }
#if WTF_ARM_ARCH_AT_LEAST(5)
        m_assembler.blx(targetReg);
#endif
    }

private:
    friend class LinkBuffer;
    friend class RepatchBuffer;

    static void linkCall(void* code, Call call, FunctionPtr function)
    {
        ARMAssembler::linkCall(code, call.m_jmp, function.value());
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
