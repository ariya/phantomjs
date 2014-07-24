/*
 * Copyright (C) 2013 Cisco Systems, Inc. All rights reserved.
 * Copyright (C) 2009-2011 STMicroelectronics. All rights reserved.
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

#ifndef MacroAssemblerSH4_h
#define MacroAssemblerSH4_h

#if ENABLE(ASSEMBLER) && CPU(SH4)

#include "SH4Assembler.h"
#include "AbstractMacroAssembler.h"
#include <wtf/Assertions.h>

namespace JSC {

class MacroAssemblerSH4 : public AbstractMacroAssembler<SH4Assembler> {
public:
    typedef SH4Assembler::FPRegisterID FPRegisterID;

    static const Scale ScalePtr = TimesFour;
    static const FPRegisterID fscratch = SH4Registers::dr10;
    static const RegisterID stackPointerRegister = SH4Registers::sp;
    static const RegisterID linkRegister = SH4Registers::pr;
    static const RegisterID scratchReg3 = SH4Registers::r13;

    static const int MaximumCompactPtrAlignedAddressOffset = 60;

    static bool isCompactPtrAlignedAddressOffset(ptrdiff_t value)
    {
        return (value >= 0) && (value <= MaximumCompactPtrAlignedAddressOffset) && (!(value & 3));
    }

    enum RelationalCondition {
        Equal = SH4Assembler::EQ,
        NotEqual = SH4Assembler::NE,
        Above = SH4Assembler::HI,
        AboveOrEqual = SH4Assembler::HS,
        Below = SH4Assembler::LI,
        BelowOrEqual = SH4Assembler::LS,
        GreaterThan = SH4Assembler::GT,
        GreaterThanOrEqual = SH4Assembler::GE,
        LessThan = SH4Assembler::LT,
        LessThanOrEqual = SH4Assembler::LE
    };

    enum ResultCondition {
        Overflow = SH4Assembler::OF,
        Signed = SH4Assembler::SI,
        PositiveOrZero = SH4Assembler::NS,
        Zero = SH4Assembler::EQ,
        NonZero = SH4Assembler::NE
    };

    enum DoubleCondition {
        // These conditions will only evaluate to true if the comparison is ordered - i.e. neither operand is NaN.
        DoubleEqual = SH4Assembler::EQ,
        DoubleNotEqual = SH4Assembler::NE,
        DoubleGreaterThan = SH4Assembler::GT,
        DoubleGreaterThanOrEqual = SH4Assembler::GE,
        DoubleLessThan = SH4Assembler::LT,
        DoubleLessThanOrEqual = SH4Assembler::LE,
        // If either operand is NaN, these conditions always evaluate to true.
        DoubleEqualOrUnordered = SH4Assembler::EQU,
        DoubleNotEqualOrUnordered = SH4Assembler::NEU,
        DoubleGreaterThanOrUnordered = SH4Assembler::GTU,
        DoubleGreaterThanOrEqualOrUnordered = SH4Assembler::GEU,
        DoubleLessThanOrUnordered = SH4Assembler::LTU,
        DoubleLessThanOrEqualOrUnordered = SH4Assembler::LEU,
    };

    RegisterID claimScratch()
    {
        return m_assembler.claimScratch();
    }

    void releaseScratch(RegisterID reg)
    {
        m_assembler.releaseScratch(reg);
    }

    static RelationalCondition invert(RelationalCondition cond)
    {
        switch (cond) {
        case Equal:
            return NotEqual;
        case NotEqual:
            return Equal;
        case Above:
            return BelowOrEqual;
        case AboveOrEqual:
            return Below;
        case Below:
            return AboveOrEqual;
        case BelowOrEqual:
            return Above;
        case GreaterThan:
            return LessThanOrEqual;
        case GreaterThanOrEqual:
            return LessThan;
        case LessThan:
            return GreaterThanOrEqual;
        case LessThanOrEqual:
            return GreaterThan;
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    // Integer arithmetic operations

    void add32(RegisterID src, RegisterID dest)
    {
        m_assembler.addlRegReg(src, dest);
    }

    void add32(RegisterID src1, RegisterID src2, RegisterID dest)
    {
        if (src1 == dest)
            add32(src2, dest);
        else {
            move(src2, dest);
            add32(src1, dest);
        }
    }

    void add32(TrustedImm32 imm, RegisterID dest)
    {
        if (!imm.m_value)
            return;

        if (m_assembler.isImmediate(imm.m_value)) {
            m_assembler.addlImm8r(imm.m_value, dest);
            return;
        }

        RegisterID scr = claimScratch();
        m_assembler.loadConstant(imm.m_value, scr);
        m_assembler.addlRegReg(scr, dest);
        releaseScratch(scr);
    }

    void add32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        move(src, dest);
        add32(imm, dest);
    }

    void add32(TrustedImm32 imm, Address address)
    {
        if (!imm.m_value)
            return;

        RegisterID scr = claimScratch();
        load32(address, scr);
        add32(imm, scr);
        store32(scr, address);
        releaseScratch(scr);
    }

    void add32(Address src, RegisterID dest)
    {
        RegisterID scr = claimScratch();
        load32(src, scr);
        m_assembler.addlRegReg(scr, dest);
        releaseScratch(scr);
    }

    void add32(AbsoluteAddress src, RegisterID dest)
    {
        RegisterID scr = claimScratch();
        load32(src.m_ptr, scr);
        m_assembler.addlRegReg(scr, dest);
        releaseScratch(scr);
    }

    void and32(RegisterID src, RegisterID dest)
    {
        m_assembler.andlRegReg(src, dest);
    }

    void and32(RegisterID src1, RegisterID src2, RegisterID dest)
    {
        if (src1 == dest)
            and32(src2, dest);
        else {
            move(src2, dest);
            and32(src1, dest);
        }
    }

    void and32(Address src, RegisterID dest)
    {
        RegisterID scr = claimScratch();
        load32(src, scr);
        and32(scr, dest);
        releaseScratch(scr);
    }

    void and32(TrustedImm32 imm, RegisterID dest)
    {
        if (!imm.m_value) {
            m_assembler.movImm8(0, dest);
            return;
        }

        if ((imm.m_value <= 255) && (imm.m_value >= 0) && (dest == SH4Registers::r0)) {
            m_assembler.andlImm8r(imm.m_value, dest);
            return;
        }

        RegisterID scr = claimScratch();
        m_assembler.loadConstant(imm.m_value, scr);
        m_assembler.andlRegReg(scr, dest);
        releaseScratch(scr);
    }

    void and32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        if (src != dest) {
            move(imm, dest);
            and32(src, dest);
            return;
        }

        and32(imm, dest);
    }

    void lshift32(RegisterID shiftamount, RegisterID dest)
    {
        RegisterID shiftTmp = claimScratch();
        m_assembler.loadConstant(0x1f, shiftTmp);
        m_assembler.andlRegReg(shiftamount, shiftTmp);
        m_assembler.shldRegReg(dest, shiftTmp);
        releaseScratch(shiftTmp);
    }

    void lshift32(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        move(src, dest);
        lshift32(shiftAmount, dest);
    }

    void lshift32(TrustedImm32 imm, RegisterID dest)
    {
        int immMasked = imm.m_value & 0x1f;
        if (!immMasked)
            return;

        if ((immMasked == 1) || (immMasked == 2) || (immMasked == 8) || (immMasked == 16)) {
            m_assembler.shllImm8r(immMasked, dest);
            return;
        }

        RegisterID shiftTmp = claimScratch();
        m_assembler.loadConstant(immMasked, shiftTmp);
        m_assembler.shldRegReg(dest, shiftTmp);
        releaseScratch(shiftTmp);
    }

    void lshift32(RegisterID src, TrustedImm32 shiftamount, RegisterID dest)
    {
        move(src, dest);
        lshift32(shiftamount, dest);
    }

    void mul32(RegisterID src, RegisterID dest)
    {
        mul32(src, dest, dest);    
    }

    void mul32(RegisterID src1, RegisterID src2, RegisterID dest)
    {
        m_assembler.imullRegReg(src1, src2);
        m_assembler.stsmacl(dest);
    }

    void mul32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        if (src == dest) {
            RegisterID immval = claimScratch();
            move(imm, immval);
            mul32(immval, dest);
            releaseScratch(immval);
        } else {
            move(imm, dest);
            mul32(src, dest);
        }
    }

    void or32(RegisterID src, RegisterID dest)
    {
        m_assembler.orlRegReg(src, dest);
    }

    void or32(TrustedImm32 imm, RegisterID dest)
    {
        if ((imm.m_value <= 255) && (imm.m_value >= 0) && (dest == SH4Registers::r0)) {
            m_assembler.orlImm8r(imm.m_value, dest);
            return;
        }

        RegisterID scr = claimScratch();
        m_assembler.loadConstant(imm.m_value, scr);
        m_assembler.orlRegReg(scr, dest);
        releaseScratch(scr);
    }

    void or32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        if (op1 == op2)
            move(op1, dest);
        else if (op1 == dest)
            or32(op2, dest);
        else {
            move(op2, dest);
            or32(op1, dest);
        }
    }

    void or32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        if (src != dest) {
            move(imm, dest);
            or32(src, dest);
            return;
        }

        or32(imm, dest);
    }

    void or32(RegisterID src, AbsoluteAddress address)
    {
        RegisterID destptr = claimScratch();
        move(TrustedImmPtr(address.m_ptr), destptr);
        RegisterID destval = claimScratch();
        m_assembler.movlMemReg(destptr, destval);
        m_assembler.orlRegReg(src, destval);
        m_assembler.movlRegMem(destval, destptr);
        releaseScratch(destval);
        releaseScratch(destptr);
    }

    void xor32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        if (src != dest) {
            move(imm, dest);
            xor32(src, dest);
            return;
        }

        xor32(imm, dest);
    }

    void rshift32(RegisterID shiftamount, RegisterID dest)
    {
        RegisterID shiftTmp = claimScratch();
        m_assembler.loadConstant(0x1f, shiftTmp);
        m_assembler.andlRegReg(shiftamount, shiftTmp);
        m_assembler.neg(shiftTmp, shiftTmp);
        m_assembler.shadRegReg(dest, shiftTmp);
        releaseScratch(shiftTmp);
    }

    void rshift32(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        move(src, dest);
        rshift32(shiftAmount, dest);
    }

    void rshift32(TrustedImm32 imm, RegisterID dest)
    {
        int immMasked = imm.m_value & 0x1f;
        if (!immMasked)
            return;

        if (immMasked == 1) {
            m_assembler.sharImm8r(immMasked, dest);
            return;
        }

        RegisterID shiftTmp = claimScratch();
        m_assembler.loadConstant(-immMasked, shiftTmp);
        m_assembler.shadRegReg(dest, shiftTmp);
        releaseScratch(shiftTmp);
    }

    void rshift32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        move(src, dest);
        rshift32(imm, dest);
    }

    void sub32(RegisterID src, RegisterID dest)
    {
        m_assembler.sublRegReg(src, dest);
    }

    void sub32(TrustedImm32 imm, AbsoluteAddress address)
    {
        if (!imm.m_value)
            return;

        RegisterID result = claimScratch();
        RegisterID scratchReg = claimScratch();

        move(TrustedImmPtr(address.m_ptr), scratchReg);
        m_assembler.movlMemReg(scratchReg, result);

        if (m_assembler.isImmediate(-imm.m_value))
            m_assembler.addlImm8r(-imm.m_value, result);
        else {
            m_assembler.loadConstant(imm.m_value, scratchReg3);
            m_assembler.sublRegReg(scratchReg3, result);
        }

        store32(result, scratchReg);
        releaseScratch(result);
        releaseScratch(scratchReg);
    }

    void sub32(TrustedImm32 imm, Address address)
    {
        add32(TrustedImm32(-imm.m_value), address);
    }

    void add32(TrustedImm32 imm, AbsoluteAddress address)
    {
        if (!imm.m_value)
            return;

        RegisterID result = claimScratch();
        RegisterID scratchReg = claimScratch();

        move(TrustedImmPtr(address.m_ptr), scratchReg);
        m_assembler.movlMemReg(scratchReg, result);

        if (m_assembler.isImmediate(imm.m_value))
            m_assembler.addlImm8r(imm.m_value, result);
        else {
            m_assembler.loadConstant(imm.m_value, scratchReg3);
            m_assembler.addlRegReg(scratchReg3, result);
        }

        store32(result, scratchReg);
        releaseScratch(result);
        releaseScratch(scratchReg);
    }

    void add64(TrustedImm32 imm, AbsoluteAddress address)
    {
        RegisterID scr1 = claimScratch();
        RegisterID scr2 = claimScratch();

        // Add 32-bit LSB first.
        move(TrustedImmPtr(address.m_ptr), scratchReg3);
        m_assembler.movlMemReg(scratchReg3, scr1); // scr1 = 32-bit LSB of int64 @ address
        m_assembler.loadConstant(imm.m_value, scr2);
        m_assembler.clrt();
        m_assembler.addclRegReg(scr1, scr2);
        m_assembler.movlRegMem(scr2, scratchReg3); // Update address with 32-bit LSB result.

        // Then add 32-bit MSB.
        m_assembler.addlImm8r(4, scratchReg3);
        m_assembler.movlMemReg(scratchReg3, scr1); // scr1 = 32-bit MSB of int64 @ address
        m_assembler.movt(scr2);
        if (imm.m_value < 0)
            m_assembler.addlImm8r(-1, scr2); // Sign extend imm value if needed.
        m_assembler.addvlRegReg(scr2, scr1);
        m_assembler.movlRegMem(scr1, scratchReg3); // Update (address + 4) with 32-bit MSB result.

        releaseScratch(scr2);
        releaseScratch(scr1);
    }

    void sub32(TrustedImm32 imm, RegisterID dest)
    {
        if (!imm.m_value)
            return;

        if (m_assembler.isImmediate(-imm.m_value)) {
            m_assembler.addlImm8r(-imm.m_value, dest);
            return;
        }

        RegisterID scr = claimScratch();
        m_assembler.loadConstant(imm.m_value, scr);
        m_assembler.sublRegReg(scr, dest);
        releaseScratch(scr);
    }

    void sub32(Address src, RegisterID dest)
    {
        RegisterID scr = claimScratch();
        load32(src, scr);
        m_assembler.sublRegReg(scr, dest);
        releaseScratch(scr);
    }

    void xor32(RegisterID src, RegisterID dest)
    {
        m_assembler.xorlRegReg(src, dest);
    }

    void xor32(RegisterID src1, RegisterID src2, RegisterID dest)
    {
        if (src1 == dest)
            xor32(src2, dest);
        else {
            move(src2, dest);
            xor32(src1, dest);
        }
    }

    void xor32(TrustedImm32 imm, RegisterID srcDest)
    {
        if (imm.m_value == -1) {
            m_assembler.notlReg(srcDest, srcDest);
            return;
        }

        if ((srcDest != SH4Registers::r0) || (imm.m_value > 255) || (imm.m_value < 0)) {
            RegisterID scr = claimScratch();
            m_assembler.loadConstant(imm.m_value, scr);
            m_assembler.xorlRegReg(scr, srcDest);
            releaseScratch(scr);
            return;
        }

        m_assembler.xorlImm8r(imm.m_value, srcDest);
    }

    void compare32(int imm, RegisterID dst, RelationalCondition cond)
    {
        if (((cond == Equal) || (cond == NotEqual)) && (dst == SH4Registers::r0) && m_assembler.isImmediate(imm)) {
            m_assembler.cmpEqImmR0(imm, dst);
            return;
        }

        if (((cond == Equal) || (cond == NotEqual)) && !imm) {
            m_assembler.testlRegReg(dst, dst);
            return;
        }

        RegisterID scr = claimScratch();
        m_assembler.loadConstant(imm, scr);
        m_assembler.cmplRegReg(scr, dst, SH4Condition(cond));
        releaseScratch(scr);
    }

    void compare32(int offset, RegisterID base, RegisterID left, RelationalCondition cond)
    {
        RegisterID scr = claimScratch();
        if (!offset) {
            m_assembler.movlMemReg(base, scr);
            m_assembler.cmplRegReg(scr, left, SH4Condition(cond));
            releaseScratch(scr);
            return;
        }

        if ((offset < 0) || (offset >= 64)) {
            m_assembler.loadConstant(offset, scr);
            m_assembler.addlRegReg(base, scr);
            m_assembler.movlMemReg(scr, scr);
            m_assembler.cmplRegReg(scr, left, SH4Condition(cond));
            releaseScratch(scr);
            return;
        }

        m_assembler.movlMemReg(offset >> 2, base, scr);
        m_assembler.cmplRegReg(scr, left, SH4Condition(cond));
        releaseScratch(scr);
    }

    void testImm(int imm, int offset, RegisterID base)
    {
        RegisterID scr = claimScratch();
        load32(base, offset, scr);

        RegisterID scr1 = claimScratch();
        move(TrustedImm32(imm), scr1);

        m_assembler.testlRegReg(scr, scr1);
        releaseScratch(scr);
        releaseScratch(scr1);
    }

    void testlImm(int imm, RegisterID dst)
    {
        if ((dst == SH4Registers::r0) && (imm <= 255) && (imm >= 0)) {
            m_assembler.testlImm8r(imm, dst);
            return;
        }

        RegisterID scr = claimScratch();
        m_assembler.loadConstant(imm, scr);
        m_assembler.testlRegReg(scr, dst);
        releaseScratch(scr);
    }

    void compare32(RegisterID right, int offset, RegisterID base, RelationalCondition cond)
    {
        if (!offset) {
            RegisterID scr = claimScratch();
            m_assembler.movlMemReg(base, scr);
            m_assembler.cmplRegReg(right, scr, SH4Condition(cond));
            releaseScratch(scr);
            return;
        }

        if ((offset < 0) || (offset >= 64)) {
            RegisterID scr = claimScratch();
            m_assembler.loadConstant(offset, scr);
            m_assembler.addlRegReg(base, scr);
            m_assembler.movlMemReg(scr, scr);
            m_assembler.cmplRegReg(right, scr, SH4Condition(cond));
            releaseScratch(scr);
            return;
        }

        RegisterID scr = claimScratch();
        m_assembler.movlMemReg(offset >> 2, base, scr);
        m_assembler.cmplRegReg(right, scr, SH4Condition(cond));
        releaseScratch(scr);
    }

    void compare32(int imm, int offset, RegisterID base, RelationalCondition cond)
    {
        RegisterID scr = claimScratch();
        load32(base, offset, scr);

        RegisterID scr1 = claimScratch();
        move(TrustedImm32(imm), scr1);

        m_assembler.cmplRegReg(scr1, scr, SH4Condition(cond));

        releaseScratch(scr1);
        releaseScratch(scr);
    }

    // Memory access operation

    ALWAYS_INLINE void loadEffectiveAddress(BaseIndex address, RegisterID dest, int extraoffset = 0)
    {
        if (dest == address.base) {
            RegisterID scaledIndex = claimScratch();
            move(address.index, scaledIndex);
            lshift32(TrustedImm32(address.scale), scaledIndex);
            add32(scaledIndex, dest);
            releaseScratch(scaledIndex);
        } else {
            move(address.index, dest);
            lshift32(TrustedImm32(address.scale), dest);
            add32(address.base, dest);
        }

        add32(TrustedImm32(address.offset + extraoffset), dest);
    }

    void load32(ImplicitAddress address, RegisterID dest)
    {
        load32(address.base, address.offset, dest);
    }

    void load8(ImplicitAddress address, RegisterID dest)
    {
        load8(address.base, address.offset, dest);
    }

    void load8(BaseIndex address, RegisterID dest)
    {
        RegisterID scr = claimScratch();
        move(address.index, scr);
        lshift32(TrustedImm32(address.scale), scr);
        add32(address.base, scr);
        load8(scr, address.offset, dest);
        releaseScratch(scr);
    }

    void load8PostInc(RegisterID base, RegisterID dest)
    {
        m_assembler.movbMemRegIn(base, dest);
        m_assembler.extub(dest, dest);
    }

    void load8Signed(BaseIndex address, RegisterID dest)
    {
        RegisterID scr = claimScratch();
        move(address.index, scr);
        lshift32(TrustedImm32(address.scale), scr);
        add32(address.base, scr);
        load8Signed(scr, address.offset, dest);
        releaseScratch(scr);
    }

    void load32(BaseIndex address, RegisterID dest)
    {
        RegisterID scr = claimScratch();
        move(address.index, scr);
        lshift32(TrustedImm32(address.scale), scr);
        add32(address.base, scr);
        load32(scr, address.offset, dest);
        releaseScratch(scr);
    }

    void load32(const void* address, RegisterID dest)
    {
        move(TrustedImmPtr(address), dest);
        m_assembler.movlMemReg(dest, dest);
    }

    void load32(RegisterID base, int offset, RegisterID dest)
    {
        if (!offset) {
            m_assembler.movlMemReg(base, dest);
            return;
        }

        if ((offset >= 0) && (offset < 64)) {
            m_assembler.movlMemReg(offset >> 2, base, dest);
            return;
        }

        RegisterID scr = (dest == base) ? claimScratch() : dest;

        m_assembler.loadConstant(offset, scr);
        if (base == SH4Registers::r0)
            m_assembler.movlR0mr(scr, dest);
        else {
            m_assembler.addlRegReg(base, scr);
            m_assembler.movlMemReg(scr, dest);
        }

        if (dest == base)
            releaseScratch(scr);
    }

    void load8Signed(RegisterID base, int offset, RegisterID dest)
    {
        if (!offset) {
            m_assembler.movbMemReg(base, dest);
            return;
        }

        if ((offset > 0) && (offset <= 15) && (dest == SH4Registers::r0)) {
            m_assembler.movbMemReg(offset, base, dest);
            return;
        }

        RegisterID scr = (dest == base) ? claimScratch() : dest;

        m_assembler.loadConstant(offset, scr);
        if (base == SH4Registers::r0)
            m_assembler.movbR0mr(scr, dest);
        else {
            m_assembler.addlRegReg(base, scr);
            m_assembler.movbMemReg(scr, dest);
        }

        if (dest == base)
            releaseScratch(scr);
    }

    void load8(RegisterID base, int offset, RegisterID dest)
    {
        load8Signed(base, offset, dest);
        m_assembler.extub(dest, dest);
    }

    void load32(RegisterID src, RegisterID dst)
    {
        m_assembler.movlMemReg(src, dst);
    }

    void load16(ImplicitAddress address, RegisterID dest)
    {
        if (!address.offset) {
            m_assembler.movwMemReg(address.base, dest);
            m_assembler.extuw(dest, dest);
            return;
        }

        if ((address.offset > 0) && (address.offset <= 30) && (dest == SH4Registers::r0)) {
            m_assembler.movwMemReg(address.offset >> 1, address.base, dest);
            m_assembler.extuw(dest, dest);
            return;
        }

        RegisterID scr = (dest == address.base) ? claimScratch() : dest;

        m_assembler.loadConstant(address.offset, scr);
        if (address.base == SH4Registers::r0)
            m_assembler.movwR0mr(scr, dest);
        else {
            m_assembler.addlRegReg(address.base, scr);
            m_assembler.movwMemReg(scr, dest);
        }
        m_assembler.extuw(dest, dest);

        if (dest == address.base)
            releaseScratch(scr);
    }

    void load16Unaligned(BaseIndex address, RegisterID dest)
    {
        RegisterID scr = claimScratch();

        loadEffectiveAddress(address, scr);

        RegisterID scr1 = claimScratch();
        load8PostInc(scr, scr1);
        load8(scr, dest);
        m_assembler.shllImm8r(8, dest);
        or32(scr1, dest);

        releaseScratch(scr);
        releaseScratch(scr1);
    }

    void load16(RegisterID src, RegisterID dest)
    {
        m_assembler.movwMemReg(src, dest);
        m_assembler.extuw(dest, dest);
    }

    void load16Signed(RegisterID src, RegisterID dest)
    {
        m_assembler.movwMemReg(src, dest);
    }

    void load16(BaseIndex address, RegisterID dest)
    {
        load16Signed(address, dest);
        m_assembler.extuw(dest, dest);
    }

    void load16PostInc(RegisterID base, RegisterID dest)
    {
        m_assembler.movwMemRegIn(base, dest);
        m_assembler.extuw(dest, dest);
    }

    void load16Signed(BaseIndex address, RegisterID dest)
    {
        RegisterID scr = claimScratch();

        move(address.index, scr);
        lshift32(TrustedImm32(address.scale), scr);
        add32(TrustedImm32(address.offset), scr);

        if (address.base == SH4Registers::r0)
            m_assembler.movwR0mr(scr, dest);
        else {
            add32(address.base, scr);
            load16Signed(scr, dest);
        }

        releaseScratch(scr);
    }

    void store8(RegisterID src, BaseIndex address)
    {
        RegisterID scr = claimScratch();

        move(address.index, scr);
        lshift32(TrustedImm32(address.scale), scr);
        add32(TrustedImm32(address.offset), scr);

        if (address.base == SH4Registers::r0)
            m_assembler.movbRegMemr0(src, scr);
        else {
            add32(address.base, scr);
            m_assembler.movbRegMem(src, scr);
        }

        releaseScratch(scr);
    }

    void store8(TrustedImm32 imm, void* address)
    {
        RegisterID srcval = claimScratch();
        RegisterID dstptr = claimScratch();
        move(imm, srcval);
        m_assembler.loadConstant(reinterpret_cast<uint32_t>(address), dstptr);
        m_assembler.movbRegMem(srcval, dstptr);
        releaseScratch(dstptr);
        releaseScratch(srcval);
    }

    void store16(RegisterID src, BaseIndex address)
    {
        RegisterID scr = claimScratch();

        move(address.index, scr);
        lshift32(TrustedImm32(address.scale), scr);
        add32(TrustedImm32(address.offset), scr);

        if (address.base == SH4Registers::r0)
            m_assembler.movwRegMemr0(src, scr);
        else {
            add32(address.base, scr);
            m_assembler.movwRegMem(src, scr);
        }

        releaseScratch(scr);
    }

    void store32(RegisterID src, ImplicitAddress address)
    {
        if (!address.offset) {
            m_assembler.movlRegMem(src, address.base);
            return;
        }

        if ((address.offset >= 0) && (address.offset < 64)) {
            m_assembler.movlRegMem(src, address.offset >> 2, address.base);
            return;
        }

        RegisterID scr = claimScratch();
        m_assembler.loadConstant(address.offset, scr);
        if (address.base == SH4Registers::r0)
            m_assembler.movlRegMemr0(src, scr);
        else {
            m_assembler.addlRegReg(address.base, scr);
            m_assembler.movlRegMem(src, scr);
        }
        releaseScratch(scr);
    }

    void store32(RegisterID src, RegisterID dst)
    {
        m_assembler.movlRegMem(src, dst);
    }

    void store32(TrustedImm32 imm, ImplicitAddress address)
    {
        RegisterID scr = claimScratch();
        m_assembler.loadConstant(imm.m_value, scr);
        store32(scr, address);
        releaseScratch(scr);
    }

    void store32(RegisterID src, BaseIndex address)
    {
        RegisterID scr = claimScratch();

        move(address.index, scr);
        lshift32(TrustedImm32(address.scale), scr);
        add32(address.base, scr);
        store32(src, Address(scr, address.offset));

        releaseScratch(scr);
    }

    void store32(TrustedImm32 imm, void* address)
    {
        RegisterID scr = claimScratch();
        RegisterID scr1 = claimScratch();
        m_assembler.loadConstant(imm.m_value, scr);
        move(TrustedImmPtr(address), scr1);
        m_assembler.movlRegMem(scr, scr1);
        releaseScratch(scr);
        releaseScratch(scr1);
    }

    void store32(RegisterID src, void* address)
    {
        RegisterID scr = claimScratch();
        move(TrustedImmPtr(address), scr);
        m_assembler.movlRegMem(src, scr);
        releaseScratch(scr);
    }

    void store32(TrustedImm32 imm, BaseIndex address)
    {
        RegisterID destptr = claimScratch();

        loadEffectiveAddress(address, destptr);

        RegisterID srcval = claimScratch();
        move(imm, srcval);
        m_assembler.movlRegMem(srcval, destptr);
        releaseScratch(srcval);
        releaseScratch(destptr);
    }

    DataLabel32 load32WithAddressOffsetPatch(Address address, RegisterID dest)
    {
        RegisterID scr = claimScratch();
        DataLabel32 label(this);
        m_assembler.loadConstantUnReusable(address.offset, scr);
        m_assembler.addlRegReg(address.base, scr);
        m_assembler.movlMemReg(scr, dest);
        releaseScratch(scr);
        return label;
    }
    
    DataLabel32 store32WithAddressOffsetPatch(RegisterID src, Address address)
    {
        RegisterID scr = claimScratch();
        DataLabel32 label(this);
        m_assembler.loadConstantUnReusable(address.offset, scr);
        m_assembler.addlRegReg(address.base, scr);
        m_assembler.movlRegMem(src, scr);
        releaseScratch(scr);
        return label;
    }

    DataLabelCompact load32WithCompactAddressOffsetPatch(Address address, RegisterID dest)
    {
        DataLabelCompact dataLabel(this);
        ASSERT(isCompactPtrAlignedAddressOffset(address.offset));
        m_assembler.movlMemRegCompact(address.offset >> 2, address.base, dest);
        return dataLabel;
    }

    ConvertibleLoadLabel convertibleLoadPtr(Address address, RegisterID dest)
    {
        ConvertibleLoadLabel result(this);

        RegisterID scr = claimScratch();
        m_assembler.movImm8(address.offset, scr);
        m_assembler.addlRegReg(address.base, scr);
        m_assembler.movlMemReg(scr, dest);
        releaseScratch(scr);

        return result;
    }

    // Floating-point operations

    static bool supportsFloatingPoint() { return true; }
    static bool supportsFloatingPointTruncate() { return true; }
    static bool supportsFloatingPointSqrt() { return true; }
    static bool supportsFloatingPointAbs() { return true; }

    void moveDoubleToInts(FPRegisterID src, RegisterID dest1, RegisterID dest2)
    {
        m_assembler.fldsfpul((FPRegisterID)(src + 1));
        m_assembler.stsfpulReg(dest1);
        m_assembler.fldsfpul(src);
        m_assembler.stsfpulReg(dest2);
    }

    void moveIntsToDouble(RegisterID src1, RegisterID src2, FPRegisterID dest, FPRegisterID)
    {
        m_assembler.ldsrmfpul(src1);
        m_assembler.fstsfpul((FPRegisterID)(dest + 1));
        m_assembler.ldsrmfpul(src2);
        m_assembler.fstsfpul(dest);
    }

    void moveDouble(FPRegisterID src, FPRegisterID dest)
    {
        if (src != dest) {
            m_assembler.fmovsRegReg((FPRegisterID)(src + 1), (FPRegisterID)(dest + 1));
            m_assembler.fmovsRegReg(src, dest);
        }
    }

    void swapDouble(FPRegisterID fr1, FPRegisterID fr2)
    {
        if (fr1 != fr2) {
            m_assembler.fldsfpul((FPRegisterID)(fr1 + 1));
            m_assembler.fmovsRegReg((FPRegisterID)(fr2 + 1), (FPRegisterID)(fr1 + 1));
            m_assembler.fstsfpul((FPRegisterID)(fr2 + 1));
            m_assembler.fldsfpul(fr1);
            m_assembler.fmovsRegReg(fr2, fr1);
            m_assembler.fstsfpul(fr2);
        }
    }

    void loadFloat(BaseIndex address, FPRegisterID dest)
    {
        RegisterID scr = claimScratch();

        loadEffectiveAddress(address, scr);

        m_assembler.fmovsReadrm(scr, dest);
        releaseScratch(scr);
    }

    void loadDouble(BaseIndex address, FPRegisterID dest)
    {
        RegisterID scr = claimScratch();

        loadEffectiveAddress(address, scr);

        m_assembler.fmovsReadrminc(scr, (FPRegisterID)(dest + 1));
        m_assembler.fmovsReadrm(scr, dest);
        releaseScratch(scr);
    }

    void loadDouble(ImplicitAddress address, FPRegisterID dest)
    {
        RegisterID scr = claimScratch();

        m_assembler.loadConstant(address.offset, scr);
        if (address.base == SH4Registers::r0) {
            m_assembler.fmovsReadr0r(scr, (FPRegisterID)(dest + 1));
            m_assembler.addlImm8r(4, scr);
            m_assembler.fmovsReadr0r(scr, dest);
            releaseScratch(scr);
            return;
        }

        m_assembler.addlRegReg(address.base, scr);
        m_assembler.fmovsReadrminc(scr, (FPRegisterID)(dest + 1));
        m_assembler.fmovsReadrm(scr, dest);
        releaseScratch(scr);
    }

    void loadDouble(const void* address, FPRegisterID dest)
    {
        RegisterID scr = claimScratch();
        move(TrustedImmPtr(address), scr);
        m_assembler.fmovsReadrminc(scr, (FPRegisterID)(dest + 1));
        m_assembler.fmovsReadrm(scr, dest);
        releaseScratch(scr);
    }

    void storeFloat(FPRegisterID src, BaseIndex address)
    {
        RegisterID scr = claimScratch();
        loadEffectiveAddress(address, scr);
        m_assembler.fmovsWriterm(src, scr);
        releaseScratch(scr);
    }

    void storeDouble(FPRegisterID src, ImplicitAddress address)
    {
        RegisterID scr = claimScratch();
        m_assembler.loadConstant(address.offset + 8, scr);
        m_assembler.addlRegReg(address.base, scr);
        m_assembler.fmovsWriterndec(src, scr);
        m_assembler.fmovsWriterndec((FPRegisterID)(src + 1), scr);
        releaseScratch(scr);
    }

    void storeDouble(FPRegisterID src, BaseIndex address)
    {
        RegisterID scr = claimScratch();

        loadEffectiveAddress(address, scr, 8);

        m_assembler.fmovsWriterndec(src, scr);
        m_assembler.fmovsWriterndec((FPRegisterID)(src + 1), scr);

        releaseScratch(scr);
    }

    void addDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        if (op1 == dest)
            addDouble(op2, dest);
        else {
            moveDouble(op2, dest);
            addDouble(op1, dest);
        }
    }

    void storeDouble(FPRegisterID src, const void* address)
    {
        RegisterID scr = claimScratch();
        m_assembler.loadConstant(reinterpret_cast<uint32_t>(const_cast<void*>(address)) + 8, scr);
        m_assembler.fmovsWriterndec(src, scr);
        m_assembler.fmovsWriterndec((FPRegisterID)(src + 1), scr);
        releaseScratch(scr);
    }

    void addDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.daddRegReg(src, dest);
    }

    void addDouble(AbsoluteAddress address, FPRegisterID dest)
    {
        loadDouble(address.m_ptr, fscratch);
        addDouble(fscratch, dest);
    }

    void addDouble(Address address, FPRegisterID dest)
    {
        loadDouble(address, fscratch);
        addDouble(fscratch, dest);
    }

    void subDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.dsubRegReg(src, dest);
    }

    void subDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        if (op2 == dest) {
            moveDouble(op1, fscratch);
            subDouble(op2, fscratch);
            moveDouble(fscratch, dest);
        } else {
            moveDouble(op1, dest);
            subDouble(op2, dest);
        }
    }

    void subDouble(Address address, FPRegisterID dest)
    {
        loadDouble(address, fscratch);
        subDouble(fscratch, dest);
    }

    void mulDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.dmulRegReg(src, dest);
    }

    void mulDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        if (op1 == dest)
            mulDouble(op2, dest);
        else {
            moveDouble(op2, dest);
            mulDouble(op1, dest);
        }
    }

    void mulDouble(Address address, FPRegisterID dest)
    {
        loadDouble(address, fscratch);
        mulDouble(fscratch, dest);
    }

    void divDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.ddivRegReg(src, dest);
    }

    void divDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        if (op2 == dest) {
            moveDouble(op1, fscratch);
            divDouble(op2, fscratch);
            moveDouble(fscratch, dest);
        } else {
            moveDouble(op1, dest);
            divDouble(op2, dest);
        }
    }

    void negateDouble(FPRegisterID src, FPRegisterID dest)
    {
        moveDouble(src, dest);
        m_assembler.dneg(dest);
    }

    void convertFloatToDouble(FPRegisterID src, FPRegisterID dst)
    {
        m_assembler.fldsfpul(src);
        m_assembler.dcnvsd(dst);
    }

    void convertDoubleToFloat(FPRegisterID src, FPRegisterID dst)
    {
        m_assembler.dcnvds(src);
        m_assembler.fstsfpul(dst);
    }

    void convertInt32ToDouble(RegisterID src, FPRegisterID dest)
    {
        m_assembler.ldsrmfpul(src);
        m_assembler.floatfpulDreg(dest);
    }

    void convertInt32ToDouble(AbsoluteAddress src, FPRegisterID dest)
    {
        RegisterID scr = claimScratch();
        load32(src.m_ptr, scr);
        convertInt32ToDouble(scr, dest);
        releaseScratch(scr);
    }

    void convertInt32ToDouble(Address src, FPRegisterID dest)
    {
        RegisterID scr = claimScratch();
        load32(src, scr);
        convertInt32ToDouble(scr, dest);
        releaseScratch(scr);
    }

    void load32WithUnalignedHalfWords(BaseIndex address, RegisterID dest)
    {
        RegisterID scr = claimScratch();
        Jump m_jump;
        JumpList end;

        loadEffectiveAddress(address, scr);

        RegisterID scr1 = claimScratch();
        if (dest != SH4Registers::r0)
            move(SH4Registers::r0, scr1);

        m_assembler.ensureSpace(m_assembler.maxInstructionSize + 58, sizeof(uint32_t));
        move(scr, SH4Registers::r0);
        m_assembler.testlImm8r(0x3, SH4Registers::r0);
        m_jump = Jump(m_assembler.jne(), SH4Assembler::JumpNear);

        if (dest != SH4Registers::r0)
            move(scr1, SH4Registers::r0);

        load32(scr, dest);
        end.append(Jump(m_assembler.bra(), SH4Assembler::JumpNear));
        m_assembler.nop();
        m_jump.link(this);
        m_assembler.testlImm8r(0x1, SH4Registers::r0);

        if (dest != SH4Registers::r0)
            move(scr1, SH4Registers::r0);

        m_jump = Jump(m_assembler.jne(), SH4Assembler::JumpNear);
        load16PostInc(scr, scr1);
        load16(scr, dest);
        m_assembler.shllImm8r(16, dest);
        or32(scr1, dest);
        end.append(Jump(m_assembler.bra(), SH4Assembler::JumpNear));
        m_assembler.nop();
        m_jump.link(this);
        load8PostInc(scr, scr1);
        load16PostInc(scr, dest);
        m_assembler.shllImm8r(8, dest);
        or32(dest, scr1);
        load8(scr, dest);
        m_assembler.shllImm8r(8, dest);
        m_assembler.shllImm8r(16, dest);
        or32(scr1, dest);
        end.link(this);

        releaseScratch(scr);
        releaseScratch(scr1);
    }

    Jump branch32WithUnalignedHalfWords(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        RegisterID scr = scratchReg3;
        load32WithUnalignedHalfWords(left, scr);
        if (((cond == Equal) || (cond == NotEqual)) && !right.m_value)
            m_assembler.testlRegReg(scr, scr);
        else
            compare32(right.m_value, scr, cond);

        if (cond == NotEqual)
            return branchFalse();
        return branchTrue();
    }

    Jump branchDoubleNonZero(FPRegisterID reg, FPRegisterID scratch)
    {
        m_assembler.movImm8(0, scratchReg3);
        convertInt32ToDouble(scratchReg3, scratch);
        return branchDouble(DoubleNotEqual, reg, scratch);
    }

    Jump branchDoubleZeroOrNaN(FPRegisterID reg, FPRegisterID scratch)
    {
        m_assembler.movImm8(0, scratchReg3);
        convertInt32ToDouble(scratchReg3, scratch);
        return branchDouble(DoubleEqualOrUnordered, reg, scratch);
    }

    Jump branchDouble(DoubleCondition cond, FPRegisterID left, FPRegisterID right)
    {
        if (cond == DoubleEqual) {
            m_assembler.dcmppeq(right, left);
            return branchTrue();
        }

        if (cond == DoubleNotEqual) {
            JumpList end;
            m_assembler.dcmppeq(left, left);
            m_assembler.ensureSpace(m_assembler.maxInstructionSize + 22, sizeof(uint32_t));
            end.append(Jump(m_assembler.jne(), SH4Assembler::JumpNear));
            m_assembler.dcmppeq(right, right);
            end.append(Jump(m_assembler.jne(), SH4Assembler::JumpNear));
            m_assembler.dcmppeq(right, left);
            Jump m_jump = branchFalse();
            end.link(this);
            return m_jump;
        }

        if (cond == DoubleGreaterThan) {
            m_assembler.dcmppgt(right, left);
            return branchTrue();
        }

        if (cond == DoubleGreaterThanOrEqual) {
            JumpList end;
            m_assembler.dcmppeq(left, left);
            m_assembler.ensureSpace(m_assembler.maxInstructionSize + 22, sizeof(uint32_t));
            end.append(Jump(m_assembler.jne(), SH4Assembler::JumpNear));
            m_assembler.dcmppeq(right, right);
            end.append(Jump(m_assembler.jne(), SH4Assembler::JumpNear));
            m_assembler.dcmppgt(left, right);
            Jump m_jump = branchFalse();
            end.link(this);
            return m_jump;
        }

        if (cond == DoubleLessThan) {
            m_assembler.dcmppgt(left, right);
            return branchTrue();
        }

        if (cond == DoubleLessThanOrEqual) {
            JumpList end;
            m_assembler.dcmppeq(left, left);
            m_assembler.ensureSpace(m_assembler.maxInstructionSize + 22, sizeof(uint32_t));
            end.append(Jump(m_assembler.jne(), SH4Assembler::JumpNear));
            m_assembler.dcmppeq(right, right);
            end.append(Jump(m_assembler.jne(), SH4Assembler::JumpNear));
            m_assembler.dcmppgt(right, left);
            Jump m_jump = branchFalse();
            end.link(this);
            return m_jump;
        }

        if (cond == DoubleEqualOrUnordered) {
            JumpList takeBranch;
            m_assembler.dcmppeq(left, left);
            m_assembler.ensureSpace(m_assembler.maxInstructionSize + 22, sizeof(uint32_t));
            takeBranch.append(Jump(m_assembler.jne(), SH4Assembler::JumpNear));
            m_assembler.dcmppeq(right, right);
            takeBranch.append(Jump(m_assembler.jne(), SH4Assembler::JumpNear));
            m_assembler.dcmppeq(left, right);
            m_assembler.branch(BF_OPCODE, 2);
            takeBranch.link(this);
            return Jump(m_assembler.extraInstrForBranch(scratchReg3));
        }

        if (cond == DoubleGreaterThanOrUnordered) {
            JumpList takeBranch;
            m_assembler.dcmppeq(left, left);
            m_assembler.ensureSpace(m_assembler.maxInstructionSize + 22, sizeof(uint32_t));
            takeBranch.append(Jump(m_assembler.jne(), SH4Assembler::JumpNear));
            m_assembler.dcmppeq(right, right);
            takeBranch.append(Jump(m_assembler.jne(), SH4Assembler::JumpNear));
            m_assembler.dcmppgt(right, left);
            m_assembler.branch(BF_OPCODE, 2);
            takeBranch.link(this);
            return Jump(m_assembler.extraInstrForBranch(scratchReg3));
        }

        if (cond == DoubleGreaterThanOrEqualOrUnordered) {
            m_assembler.dcmppgt(left, right);
            return branchFalse();
        }

        if (cond == DoubleLessThanOrUnordered) {
            JumpList takeBranch;
            m_assembler.dcmppeq(left, left);
            m_assembler.ensureSpace(m_assembler.maxInstructionSize + 22, sizeof(uint32_t));
            takeBranch.append(Jump(m_assembler.jne(), SH4Assembler::JumpNear));
            m_assembler.dcmppeq(right, right);
            takeBranch.append(Jump(m_assembler.jne(), SH4Assembler::JumpNear));
            m_assembler.dcmppgt(left, right);
            m_assembler.branch(BF_OPCODE, 2);
            takeBranch.link(this);
            return Jump(m_assembler.extraInstrForBranch(scratchReg3));
        }

        if (cond == DoubleLessThanOrEqualOrUnordered) {
            m_assembler.dcmppgt(right, left);
            return branchFalse();
        }

        ASSERT(cond == DoubleNotEqualOrUnordered);
        m_assembler.dcmppeq(right, left);
        return branchFalse();
    }

    Jump branchTrue()
    {
        m_assembler.ensureSpace(m_assembler.maxInstructionSize + 6, sizeof(uint32_t));
        m_assembler.branch(BF_OPCODE, 2);
        return Jump(m_assembler.extraInstrForBranch(scratchReg3));
    }

    Jump branchFalse()
    {
        m_assembler.ensureSpace(m_assembler.maxInstructionSize + 6, sizeof(uint32_t));
        m_assembler.branch(BT_OPCODE, 2);
        return Jump(m_assembler.extraInstrForBranch(scratchReg3));
    }

    Jump branch32(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        RegisterID scr = claimScratch();
        move(left.index, scr);
        lshift32(TrustedImm32(left.scale), scr);
        add32(left.base, scr);
        load32(scr, left.offset, scr);
        compare32(right.m_value, scr, cond);
        releaseScratch(scr);

        if (cond == NotEqual)
            return branchFalse();
        return branchTrue();
    }

    void sqrtDouble(FPRegisterID src, FPRegisterID dest)
    {
        moveDouble(src, dest);
        m_assembler.dsqrt(dest);
    }
    
    void absDouble(FPRegisterID src, FPRegisterID dest)
    {
        moveDouble(src, dest);
        m_assembler.dabs(dest);
    }

    Jump branchTest8(ResultCondition cond, Address address, TrustedImm32 mask = TrustedImm32(-1))
    {
        RegisterID addressTempRegister = claimScratch();
        load8(address, addressTempRegister);
        Jump jmp = branchTest32(cond, addressTempRegister, mask);
        releaseScratch(addressTempRegister);
        return jmp;
    }

    Jump branchTest8(ResultCondition cond, AbsoluteAddress address, TrustedImm32 mask = TrustedImm32(-1))
    {
        RegisterID addressTempRegister = claimScratch();
        move(TrustedImmPtr(address.m_ptr), addressTempRegister);
        load8(Address(addressTempRegister), addressTempRegister);
        Jump jmp = branchTest32(cond, addressTempRegister, mask);
        releaseScratch(addressTempRegister);
        return jmp;
    }

    void signExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        move(src, dest);
    }

    void zeroExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        move(src, dest);
    }

    Jump branch8(RelationalCondition cond, Address left, TrustedImm32 right)
    {
        RegisterID addressTempRegister = claimScratch();
        load8(left, addressTempRegister);
        Jump jmp = branch32(cond, addressTempRegister, right);
        releaseScratch(addressTempRegister);
        return jmp;
    }

    void compare8(RelationalCondition cond, Address left, TrustedImm32 right, RegisterID dest)
    {
        RegisterID addressTempRegister = claimScratch();
        load8(left, addressTempRegister);
        compare32(cond, addressTempRegister, right, dest);
        releaseScratch(addressTempRegister);
    }

    enum BranchTruncateType { BranchIfTruncateFailed, BranchIfTruncateSuccessful };
    Jump branchTruncateDoubleToInt32(FPRegisterID src, RegisterID dest, BranchTruncateType branchType = BranchIfTruncateFailed)
    {
        Jump result;
        truncateDoubleToInt32(src, dest);
        RegisterID intscr = claimScratch();
        m_assembler.loadConstant(0x7fffffff, intscr);
        m_assembler.cmplRegReg(dest, intscr, SH4Condition(Equal));
        m_assembler.ensureSpace(m_assembler.maxInstructionSize + 12, sizeof(uint32_t));
        if (branchType == BranchIfTruncateFailed) {
            m_assembler.branch(BT_OPCODE, 2);
            m_assembler.addlImm8r(1, intscr);
            m_assembler.cmplRegReg(dest, intscr, SH4Condition(Equal));
            result = branchTrue();
        } else {
            Jump out = Jump(m_assembler.je(), SH4Assembler::JumpNear);
            m_assembler.addlImm8r(1, intscr);
            m_assembler.cmplRegReg(dest, intscr, SH4Condition(Equal));
            result = branchFalse();
            out.link(this);
        }
        releaseScratch(intscr);
        return result;
    }

    Jump branchTruncateDoubleToUint32(FPRegisterID src, RegisterID dest, BranchTruncateType branchType = BranchIfTruncateFailed)
    {
        Jump result;
        RegisterID intscr = claimScratch();
        m_assembler.loadConstant(0x80000000, intscr);
        convertInt32ToDouble(intscr, fscratch);
        addDouble(src, fscratch);
        truncateDoubleToInt32(fscratch, dest);
        m_assembler.cmplRegReg(dest, intscr, SH4Condition(Equal));
        m_assembler.ensureSpace(m_assembler.maxInstructionSize + 16, sizeof(uint32_t));
        if (branchType == BranchIfTruncateFailed) {
            m_assembler.branch(BT_OPCODE, 4);
            m_assembler.addlImm8r(-1, intscr);
            m_assembler.cmplRegReg(dest, intscr, SH4Condition(Equal));
            m_assembler.addlImm8r(1, intscr);
            m_assembler.sublRegReg(intscr, dest);
            result = branchTrue();
        } else {
            Jump out = Jump(m_assembler.je(), SH4Assembler::JumpNear);
            m_assembler.addlImm8r(-1, intscr);
            m_assembler.cmplRegReg(dest, intscr, SH4Condition(Equal));
            m_assembler.addlImm8r(1, intscr);
            m_assembler.sublRegReg(intscr, dest);
            result = branchFalse();
            out.link(this);
        }
        releaseScratch(intscr);
        return result;
    }

    void truncateDoubleToInt32(FPRegisterID src, RegisterID dest)
    {
        m_assembler.ftrcdrmfpul(src);
        m_assembler.stsfpulReg(dest);
    }

    void truncateDoubleToUint32(FPRegisterID src, RegisterID dest)
    {
        RegisterID intscr = claimScratch();
        m_assembler.loadConstant(0x80000000, intscr);
        convertInt32ToDouble(intscr, fscratch);
        addDouble(src, fscratch);
        m_assembler.ftrcdrmfpul(fscratch);
        m_assembler.stsfpulReg(dest);
        m_assembler.sublRegReg(intscr, dest);
        releaseScratch(intscr);
    }

    // Stack manipulation operations

    void pop(RegisterID dest)
    {
        m_assembler.popReg(dest);
    }

    void push(RegisterID src)
    {
        m_assembler.pushReg(src);
    }

    void push(TrustedImm32 imm)
    {
        RegisterID scr = claimScratch();
        m_assembler.loadConstant(imm.m_value, scr);
        push(scr);
        releaseScratch(scr);
    }

    // Register move operations

    void move(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.loadConstant(imm.m_value, dest);
    }

    DataLabelPtr moveWithPatch(TrustedImmPtr initialValue, RegisterID dest)
    {
        m_assembler.ensureSpace(m_assembler.maxInstructionSize, sizeof(uint32_t));
        DataLabelPtr dataLabel(this);
        m_assembler.loadConstantUnReusable(reinterpret_cast<uint32_t>(initialValue.m_value), dest);
        return dataLabel;
    }

    void move(RegisterID src, RegisterID dest)
    {
        if (src != dest)
            m_assembler.movlRegReg(src, dest);
    }

    void move(TrustedImmPtr imm, RegisterID dest)
    {
        m_assembler.loadConstant(imm.asIntptr(), dest);
    }

    void swap(RegisterID reg1, RegisterID reg2)
    {
        if (reg1 != reg2) {
            xor32(reg1, reg2);
            xor32(reg2, reg1);
            xor32(reg1, reg2);
        }
    }

    void compare32(RelationalCondition cond, RegisterID left, RegisterID right, RegisterID dest)
    {
        m_assembler.cmplRegReg(right, left, SH4Condition(cond));
        if (cond != NotEqual) {
            m_assembler.movt(dest);
            return;
        }

        m_assembler.ensureSpace(m_assembler.maxInstructionSize + 4);
        m_assembler.movImm8(0, dest);
        m_assembler.branch(BT_OPCODE, 0);
        m_assembler.movImm8(1, dest);
    }

    void compare32(RelationalCondition cond, RegisterID left, TrustedImm32 right, RegisterID dest)
    {
        if (left != dest) {
            move(right, dest);
            compare32(cond, left, dest, dest);
            return;
        }

        RegisterID scr = claimScratch();
        move(right, scr);
        compare32(cond, left, scr, dest);
        releaseScratch(scr);
    }

    void test8(ResultCondition cond, Address address, TrustedImm32 mask, RegisterID dest)
    {
        ASSERT((cond == Zero) || (cond == NonZero));

        load8(address, dest);
        if (mask.m_value == -1)
            compare32(0, dest, static_cast<RelationalCondition>(cond));
        else
            testlImm(mask.m_value, dest);
        if (cond != NonZero) {
            m_assembler.movt(dest);
            return;
        }

        m_assembler.ensureSpace(m_assembler.maxInstructionSize + 4);
        m_assembler.movImm8(0, dest);
        m_assembler.branch(BT_OPCODE, 0);
        m_assembler.movImm8(1, dest);
    }

    void loadPtrLinkReg(ImplicitAddress address)
    {
        RegisterID scr = claimScratch();
        load32(address, scr);
        m_assembler.ldspr(scr);
        releaseScratch(scr);
    }

    Jump branch32(RelationalCondition cond, RegisterID left, RegisterID right)
    {
        m_assembler.cmplRegReg(right, left, SH4Condition(cond));
        /* BT label => BF off
           nop         LDR reg
           nop         braf @reg
           nop         nop
         */
        if (cond == NotEqual)
            return branchFalse();
        return branchTrue();
    }

    Jump branch32(RelationalCondition cond, RegisterID left, TrustedImm32 right)
    {
        if (((cond == Equal) || (cond == NotEqual)) && !right.m_value)
            m_assembler.testlRegReg(left, left);
        else
            compare32(right.m_value, left, cond);

        if (cond == NotEqual)
            return branchFalse();
        return branchTrue();
    }

    Jump branch32(RelationalCondition cond, RegisterID left, Address right)
    {
        compare32(right.offset, right.base, left, cond);
        if (cond == NotEqual)
            return branchFalse();
        return branchTrue();
    }

    Jump branch32(RelationalCondition cond, Address left, RegisterID right)
    {
        compare32(right, left.offset, left.base, cond);
        if (cond == NotEqual)
            return branchFalse();
        return branchTrue();
    }

    Jump branch32(RelationalCondition cond, Address left, TrustedImm32 right)
    {
        compare32(right.m_value, left.offset, left.base, cond);
        if (cond == NotEqual)
            return branchFalse();
        return branchTrue();
    }

    Jump branch32(RelationalCondition cond, AbsoluteAddress left, RegisterID right)
    {
        RegisterID scr = claimScratch();

        load32(left.m_ptr, scr);
        m_assembler.cmplRegReg(right, scr, SH4Condition(cond));
        releaseScratch(scr);

        if (cond == NotEqual)
            return branchFalse();
        return branchTrue();
    }

    Jump branch32(RelationalCondition cond, AbsoluteAddress left, TrustedImm32 right)
    {
        RegisterID addressTempRegister = claimScratch();

        move(TrustedImmPtr(left.m_ptr), addressTempRegister);
        m_assembler.movlMemReg(addressTempRegister, addressTempRegister);
        compare32(right.m_value, addressTempRegister, cond);
        releaseScratch(addressTempRegister);

        if (cond == NotEqual)
            return branchFalse();
        return branchTrue();
    }

    Jump branch8(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        ASSERT(!(right.m_value & 0xFFFFFF00));
        RegisterID lefttmp = claimScratch();

        loadEffectiveAddress(left, lefttmp);

        load8(lefttmp, lefttmp);
        RegisterID righttmp = claimScratch();
        m_assembler.loadConstant(right.m_value, righttmp);

        Jump result = branch32(cond, lefttmp, righttmp);
        releaseScratch(lefttmp);
        releaseScratch(righttmp);
        return result;
    }

    Jump branchTest32(ResultCondition cond, RegisterID reg, RegisterID mask)
    {
        ASSERT((cond == Zero) || (cond == NonZero));

        m_assembler.testlRegReg(reg, mask);

        if (cond == NonZero) // NotEqual
            return branchFalse();
        return branchTrue();
    }

    Jump branchTest32(ResultCondition cond, RegisterID reg, TrustedImm32 mask = TrustedImm32(-1))
    {
        ASSERT((cond == Zero) || (cond == NonZero));

        if (mask.m_value == -1)
            m_assembler.testlRegReg(reg, reg);
        else
            testlImm(mask.m_value, reg);

        if (cond == NonZero) // NotEqual
            return branchFalse();
        return branchTrue();
    }

    Jump branchTest32(ResultCondition cond, Address address, TrustedImm32 mask = TrustedImm32(-1))
    {
        ASSERT((cond == Zero) || (cond == NonZero));

        if (mask.m_value == -1)
            compare32(0, address.offset, address.base, static_cast<RelationalCondition>(cond));
        else
            testImm(mask.m_value, address.offset, address.base);

        if (cond == NonZero) // NotEqual
            return branchFalse();
        return branchTrue();
    }

    Jump branchTest32(ResultCondition cond, BaseIndex address, TrustedImm32 mask = TrustedImm32(-1))
    {
        ASSERT((cond == Zero) || (cond == NonZero));

        RegisterID scr = claimScratch();

        move(address.index, scr);
        lshift32(TrustedImm32(address.scale), scr);
        add32(address.base, scr);
        load32(scr, address.offset, scr);

        if (mask.m_value == -1)
            m_assembler.testlRegReg(scr, scr);
        else
            testlImm(mask.m_value, scr);

        releaseScratch(scr);

        if (cond == NonZero) // NotEqual
            return branchFalse();
        return branchTrue();
    }

    Jump jump()
    {
        return Jump(m_assembler.jmp());
    }

    void jump(RegisterID target)
    {
        m_assembler.jmpReg(target);
    }

    void jump(Address address)
    {
        RegisterID scr = claimScratch();
        load32(address, scr);
        m_assembler.jmpReg(scr);
        releaseScratch(scr);
    }

    void jump(AbsoluteAddress address)
    {
        RegisterID scr = claimScratch();

        move(TrustedImmPtr(address.m_ptr), scr);
        m_assembler.movlMemReg(scr, scr);
        m_assembler.jmpReg(scr);
        releaseScratch(scr);
    }

    // Arithmetic control flow operations

    Jump branchNeg32(ResultCondition cond, RegisterID srcDest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));

        if (cond == Overflow)
            return branchMul32(cond, TrustedImm32(-1), srcDest, srcDest);

        neg32(srcDest);

        if (cond == Signed) {
            m_assembler.cmppz(srcDest);
            return branchFalse();
        }

        compare32(0, srcDest, Equal);
        return (cond == NonZero) ? branchFalse() : branchTrue();
    }

    Jump branchAdd32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == PositiveOrZero) || (cond == Zero) || (cond == NonZero));

        if (cond == Overflow) {
            m_assembler.addvlRegReg(src, dest);
            return branchTrue();
        }

        m_assembler.addlRegReg(src, dest);

        if ((cond == Signed) || (cond == PositiveOrZero)) {
            m_assembler.cmppz(dest);
            return (cond == Signed) ? branchFalse() : branchTrue();
        }

        compare32(0, dest, Equal);
        return (cond == NonZero) ? branchFalse() : branchTrue();
    }

    Jump branchAdd32(ResultCondition cond, RegisterID src1, RegisterID src2, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == PositiveOrZero) || (cond == Zero) || (cond == NonZero));

        if (cond == Overflow) {
            if (src1 == dest)
                m_assembler.addvlRegReg(src2, dest);
            else {
                move(src2, dest);
                m_assembler.addvlRegReg(src1, dest);
            }
            return branchTrue();
        }

        add32(src1, src2, dest);

        if ((cond == Signed) || (cond == PositiveOrZero)) {
            m_assembler.cmppz(dest);
            return (cond == Signed) ? branchFalse() : branchTrue();
        }

        compare32(0, dest, Equal);
        return (cond == NonZero) ? branchFalse() : branchTrue();
    }

    Jump branchAdd32(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == PositiveOrZero) || (cond == Zero) || (cond == NonZero));

        RegisterID immval = claimScratch();
        move(imm, immval);
        Jump result = branchAdd32(cond, immval, dest);
        releaseScratch(immval);
        return result;
    }

    Jump branchAdd32(ResultCondition cond, RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == PositiveOrZero) || (cond == Zero) || (cond == NonZero));

        move(src, dest);

        if (cond == Overflow) {
            move(imm, scratchReg3);
            m_assembler.addvlRegReg(scratchReg3, dest);
            return branchTrue();
        }

        add32(imm, dest);

        if ((cond == Signed) || (cond == PositiveOrZero)) {
            m_assembler.cmppz(dest);
            return (cond == Signed) ? branchFalse() : branchTrue();
        }

        compare32(0, dest, Equal);
        return (cond == NonZero) ? branchFalse() : branchTrue();
    }

    Jump branchAdd32(ResultCondition cond, TrustedImm32 imm, AbsoluteAddress dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == PositiveOrZero) || (cond == Zero) || (cond == NonZero));
        bool result;

        move(imm, scratchReg3);
        RegisterID destptr = claimScratch();
        RegisterID destval = claimScratch();
        move(TrustedImmPtr(dest.m_ptr), destptr);
        m_assembler.movlMemReg(destptr, destval);
        if (cond == Overflow) {
            m_assembler.addvlRegReg(scratchReg3, destval);
            result = true;
        } else {
            m_assembler.addlRegReg(scratchReg3, destval);
            if ((cond == Signed) || (cond == PositiveOrZero)) {
                m_assembler.cmppz(destval);
                result = (cond == PositiveOrZero);
            } else {
                m_assembler.testlRegReg(destval, destval);
                result = (cond != NonZero);
            }
        }
        m_assembler.movlRegMem(destval, destptr);
        releaseScratch(destval);
        releaseScratch(destptr);
        return result ? branchTrue() : branchFalse();
    }

    Jump branchMul32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));

        if (cond == Overflow) {
            RegisterID scrsign = claimScratch();
            RegisterID msbres = claimScratch();
            m_assembler.dmulslRegReg(src, dest);
            m_assembler.stsmacl(dest);
            m_assembler.cmppz(dest);
            m_assembler.movt(scrsign);
            m_assembler.addlImm8r(-1, scrsign);
            m_assembler.stsmach(msbres);
            m_assembler.cmplRegReg(msbres, scrsign, SH4Condition(Equal));
            releaseScratch(msbres);
            releaseScratch(scrsign);
            return branchFalse();
        }

        mul32(src, dest);

        if (cond == Signed) {
            m_assembler.cmppz(dest);
            return branchFalse();
        }

        compare32(0, dest, static_cast<RelationalCondition>(cond));
        return (cond == NonZero) ? branchFalse() : branchTrue();
    }

    Jump branchMul32(ResultCondition cond, RegisterID src1, RegisterID src2, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));

        if (cond == Overflow) {
            RegisterID scrsign = claimScratch();
            RegisterID msbres = claimScratch();
            m_assembler.dmulslRegReg(src1, src2);
            m_assembler.stsmacl(dest);
            m_assembler.cmppz(dest);
            m_assembler.movt(scrsign);
            m_assembler.addlImm8r(-1, scrsign);
            m_assembler.stsmach(msbres);
            m_assembler.cmplRegReg(msbres, scrsign, SH4Condition(Equal));
            releaseScratch(msbres);
            releaseScratch(scrsign);
            return branchFalse();
        }

        mul32(src1, src2, dest);

        if (cond == Signed) {
            m_assembler.cmppz(dest);
            return branchFalse();
        }

        compare32(0, dest, Equal);
        return (cond == NonZero) ? branchFalse() : branchTrue();
    }

    Jump branchMul32(ResultCondition cond, TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));

        if (src == dest) {
            move(imm, scratchReg3);
            return branchMul32(cond, scratchReg3, dest);
        }

        move(imm, dest);
        return branchMul32(cond, src, dest);
    }

    Jump branchSub32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));

        if (cond == Overflow) {
            m_assembler.subvlRegReg(src, dest);
            return branchTrue();
        }

        sub32(src, dest);

        if (cond == Signed) {
            m_assembler.cmppz(dest);
            return branchFalse();
        }

        compare32(0, dest, static_cast<RelationalCondition>(cond));
        return (cond == NonZero) ? branchFalse() : branchTrue();
    }

    Jump branchSub32(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));

        RegisterID immval = claimScratch();
        move(imm, immval);
        Jump result = branchSub32(cond, immval, dest);
        releaseScratch(immval);
        return result;
    }

    Jump branchSub32(ResultCondition cond, RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));

        move(src, dest);
        return branchSub32(cond, imm, dest);
    }

    Jump branchSub32(ResultCondition cond, RegisterID src1, RegisterID src2, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));

        if (src2 != dest) {
            move(src1, dest);
            return branchSub32(cond, src2, dest);
        }

        if (cond == Overflow) {
            RegisterID tmpval = claimScratch();
            move(src1, tmpval);
            m_assembler.subvlRegReg(src2, tmpval);
            move(tmpval, dest);
            releaseScratch(tmpval);
            return branchTrue();
        }

        RegisterID tmpval = claimScratch();
        move(src1, tmpval);
        sub32(src2, tmpval);
        move(tmpval, dest);
        releaseScratch(tmpval);

        if (cond == Signed) {
            m_assembler.cmppz(dest);
            return branchFalse();
        }

        compare32(0, dest, static_cast<RelationalCondition>(cond));
        return (cond == NonZero) ? branchFalse() : branchTrue();
    }

    Jump branchOr32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Signed) || (cond == Zero) || (cond == NonZero));

        or32(src, dest);

        if (cond == Signed) {
            m_assembler.cmppz(dest);
            return branchFalse();
        }

        compare32(0, dest, static_cast<RelationalCondition>(cond));
        return (cond == NonZero) ? branchFalse() : branchTrue();
    }

    void branchConvertDoubleToInt32(FPRegisterID src, RegisterID dest, JumpList& failureCases, FPRegisterID, bool negZeroCheck = true)
    {
        truncateDoubleToInt32(src, dest);
        convertInt32ToDouble(dest, fscratch);
        failureCases.append(branchDouble(DoubleNotEqualOrUnordered, fscratch, src));

        if (negZeroCheck)
            failureCases.append(branch32(Equal, dest, TrustedImm32(0)));
    }

    void neg32(RegisterID dst)
    {
        m_assembler.neg(dst, dst);
    }

    void urshift32(RegisterID shiftamount, RegisterID dest)
    {
        RegisterID shiftTmp = claimScratch();
        m_assembler.loadConstant(0x1f, shiftTmp);
        m_assembler.andlRegReg(shiftamount, shiftTmp);
        m_assembler.neg(shiftTmp, shiftTmp);
        m_assembler.shldRegReg(dest, shiftTmp);
        releaseScratch(shiftTmp);
    }

    void urshift32(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        move(src, dest);
        urshift32(shiftAmount, dest);
    }

    void urshift32(TrustedImm32 imm, RegisterID dest)
    {
        int immMasked = imm.m_value & 0x1f;
        if (!immMasked)
            return;

        if ((immMasked == 1) || (immMasked == 2) || (immMasked == 8) || (immMasked == 16)) {
            m_assembler.shlrImm8r(immMasked, dest);
            return;
        }

        RegisterID shiftTmp = claimScratch();
        m_assembler.loadConstant(-immMasked, shiftTmp);
        m_assembler.shldRegReg(dest, shiftTmp);
        releaseScratch(shiftTmp);
    }

    void urshift32(RegisterID src, TrustedImm32 shiftamount, RegisterID dest)
    {
        move(src, dest);
        urshift32(shiftamount, dest);
    }

    Call call()
    {
        return Call(m_assembler.call(), Call::Linkable);
    }

    Call nearCall()
    {
        return Call(m_assembler.call(), Call::LinkableNear);
    }

    Call call(RegisterID target)
    {
        return Call(m_assembler.call(target), Call::None);
    }

    void call(Address address, RegisterID target)
    {
        load32(address.base, address.offset, target);
        m_assembler.ensureSpace(m_assembler.maxInstructionSize + 2);
        m_assembler.branch(JSR_OPCODE, target);
        m_assembler.nop();
    }

    void breakpoint()
    {
        m_assembler.ensureSpace(m_assembler.maxInstructionSize + 2);
        m_assembler.bkpt();
        m_assembler.nop();
    }

    Jump branchPtrWithPatch(RelationalCondition cond, RegisterID left, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(0))
    {
        RegisterID dataTempRegister = claimScratch();

        m_assembler.ensureSpace(m_assembler.maxInstructionSize + 10, 2 * sizeof(uint32_t));
        dataLabel = moveWithPatch(initialRightValue, dataTempRegister);
        m_assembler.cmplRegReg(dataTempRegister, left, SH4Condition(cond));
        releaseScratch(dataTempRegister);

        if (cond == NotEqual)
            return branchFalse();
        return branchTrue();
    }

    Jump branchPtrWithPatch(RelationalCondition cond, Address left, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(0))
    {
        RegisterID scr = claimScratch();

        m_assembler.loadConstant(left.offset, scr);
        m_assembler.addlRegReg(left.base, scr);
        m_assembler.movlMemReg(scr, scr);
        RegisterID scr1 = claimScratch();
        m_assembler.ensureSpace(m_assembler.maxInstructionSize + 10, 2 * sizeof(uint32_t));
        dataLabel = moveWithPatch(initialRightValue, scr1);
        m_assembler.cmplRegReg(scr1, scr, SH4Condition(cond));
        releaseScratch(scr);
        releaseScratch(scr1);

        if (cond == NotEqual)
            return branchFalse();
        return branchTrue();
    }

    void ret()
    {
        m_assembler.ret();
        m_assembler.nop();
    }

    DataLabelPtr storePtrWithPatch(TrustedImmPtr initialValue, ImplicitAddress address)
    {
        RegisterID scr = claimScratch();
        DataLabelPtr label = moveWithPatch(initialValue, scr);
        store32(scr, address);
        releaseScratch(scr);
        return label;
    }

    DataLabelPtr storePtrWithPatch(ImplicitAddress address) { return storePtrWithPatch(TrustedImmPtr(0), address); }

    int sizeOfConstantPool()
    {
        return m_assembler.sizeOfConstantPool();
    }

    Call tailRecursiveCall()
    {
        RegisterID scr = claimScratch();

        m_assembler.loadConstantUnReusable(0x0, scr, true);
        Jump m_jump = Jump(m_assembler.jmp(scr));
        releaseScratch(scr);

        return Call::fromTailJump(m_jump);
    }

    Call makeTailRecursiveCall(Jump oldJump)
    {
        oldJump.link(this);
        return tailRecursiveCall();
    }

    void nop()
    {
        m_assembler.nop();
    }

    static FunctionPtr readCallTarget(CodeLocationCall call)
    {
        return FunctionPtr(reinterpret_cast<void(*)()>(SH4Assembler::readCallTarget(call.dataLocation())));
    }

    static void replaceWithJump(CodeLocationLabel instructionStart, CodeLocationLabel destination)
    {
        SH4Assembler::replaceWithJump(instructionStart.dataLocation(), destination.dataLocation());
    }
    
    static ptrdiff_t maxJumpReplacementSize()
    {
        return SH4Assembler::maxJumpReplacementSize();
    }

    static bool canJumpReplacePatchableBranchPtrWithPatch() { return false; }

    static CodeLocationLabel startOfBranchPtrWithPatchOnRegister(CodeLocationDataLabelPtr label)
    {
        return label.labelAtOffset(0);
    }

    static void revertJumpReplacementToBranchPtrWithPatch(CodeLocationLabel instructionStart, RegisterID rd, void* initialValue)
    {
        SH4Assembler::revertJumpReplacementToBranchPtrWithPatch(instructionStart.dataLocation(), rd, reinterpret_cast<int>(initialValue));
    }

    static CodeLocationLabel startOfPatchableBranchPtrWithPatchOnAddress(CodeLocationDataLabelPtr)
    {
        UNREACHABLE_FOR_PLATFORM();
        return CodeLocationLabel();
    }

    static void revertJumpReplacementToPatchableBranchPtrWithPatch(CodeLocationLabel instructionStart, Address, void* initialValue)
    {
        UNREACHABLE_FOR_PLATFORM();
    }

protected:
    SH4Assembler::Condition SH4Condition(RelationalCondition cond)
    {
        return static_cast<SH4Assembler::Condition>(cond);
    }

    SH4Assembler::Condition SH4Condition(ResultCondition cond)
    {
        return static_cast<SH4Assembler::Condition>(cond);
    }
private:
    friend class LinkBuffer;
    friend class RepatchBuffer;

    static void linkCall(void* code, Call call, FunctionPtr function)
    {
        SH4Assembler::linkCall(code, call.m_label, function.value());
    }

    static void repatchCall(CodeLocationCall call, CodeLocationLabel destination)
    {
        SH4Assembler::relinkCall(call.dataLocation(), destination.executableAddress());
    }

    static void repatchCall(CodeLocationCall call, FunctionPtr destination)
    {
        SH4Assembler::relinkCall(call.dataLocation(), destination.executableAddress());
    }
};

} // namespace JSC

#endif // ENABLE(ASSEMBLER)

#endif // MacroAssemblerSH4_h
