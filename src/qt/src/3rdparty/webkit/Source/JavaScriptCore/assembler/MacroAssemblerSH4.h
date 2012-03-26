/*
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
    static const FPRegisterID fscratch = SH4Registers::fr10;
    static const RegisterID stackPointerRegister = SH4Registers::sp;
    static const RegisterID linkRegister = SH4Registers::pr;
    static const RegisterID scratchReg3 = SH4Registers::r13;

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

    // Integer arithmetic operations

    void add32(RegisterID src, RegisterID dest)
    {
        m_assembler.addlRegReg(src, dest);
    }

    void add32(TrustedImm32 imm, RegisterID dest)
    {
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
        if (src != dest)
            m_assembler.movlRegReg(src, dest);
        add32(imm, dest);
    }

    void add32(TrustedImm32 imm, Address address)
    {
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

    void and32(RegisterID src, RegisterID dest)
    {
        m_assembler.andlRegReg(src, dest);
    }

    void and32(TrustedImm32 imm, RegisterID dest)
    {
        if ((imm.m_value <= 255) && (imm.m_value >= 0) && (dest == SH4Registers::r0)) {
            m_assembler.andlImm8r(imm.m_value, dest);
            return;
        }

        RegisterID scr = claimScratch();
        m_assembler.loadConstant((imm.m_value), scr);
        m_assembler.andlRegReg(scr, dest);
        releaseScratch(scr);
    }

    void lshift32(RegisterID shiftamount, RegisterID dest)
    {
        m_assembler.shllRegReg(dest, shiftamount);
    }

    void rshift32(int imm, RegisterID dest)
    {
        RegisterID scr = claimScratch();
        m_assembler.loadConstant(-imm, scr);
        m_assembler.shaRegReg(dest, scr);
        releaseScratch(scr);
    }

    void lshift32(TrustedImm32 imm, RegisterID dest)
    {
        if ((imm.m_value == 1) || (imm.m_value == 2) || (imm.m_value == 8) || (imm.m_value == 16)) {
            m_assembler.shllImm8r(imm.m_value, dest);
            return;
        }

        RegisterID scr = claimScratch();
        m_assembler.loadConstant(imm.m_value, scr);
        m_assembler.shllRegReg(dest, scr);
        releaseScratch(scr);
    }

    void mul32(RegisterID src, RegisterID dest)
    {
        m_assembler.imullRegReg(src, dest);
        m_assembler.stsmacl(dest);
    }

    void mul32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        RegisterID scr = claimScratch();
        move(imm, scr);
        if  (src != dest)
            move(src, dest);
        mul32(scr,  dest);
        releaseScratch(scr);
    }

    void not32(RegisterID src, RegisterID dest)
    {
        m_assembler.notlReg(src, dest);
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

    void rshift32(RegisterID shiftamount, RegisterID dest)
    {
        compare32(32, shiftamount, Equal);
        m_assembler.ensureSpace(m_assembler.maxInstructionSize + 4);
        m_assembler.branch(BT_OPCODE, 1);
        m_assembler.neg(shiftamount, shiftamount);
        m_assembler.shaRegReg(dest, shiftamount);
    }

    void rshift32(TrustedImm32 imm, RegisterID dest)
    {
        if (imm.m_value & 0x1f)
            rshift32(imm.m_value & 0x1f, dest);
    }

    void sub32(RegisterID src, RegisterID dest)
    {
        m_assembler.sublRegReg(src, dest);
    }

    void sub32(TrustedImm32 imm, AbsoluteAddress address, RegisterID scratchReg)
    {
        RegisterID result = claimScratch();

        m_assembler.loadConstant(reinterpret_cast<uint32_t>(address.m_ptr), scratchReg);
        m_assembler.movlMemReg(scratchReg, result);

        if (m_assembler.isImmediate(-imm.m_value))
            m_assembler.addlImm8r(-imm.m_value, result);
        else {
            m_assembler.loadConstant(imm.m_value, scratchReg3);
            m_assembler.sublRegReg(scratchReg3, result);
        }

        store32(result, scratchReg);
        releaseScratch(result);
    }

    void sub32(TrustedImm32 imm, AbsoluteAddress address)
    {
        RegisterID result = claimScratch();
        RegisterID scratchReg = claimScratch();

        m_assembler.loadConstant(reinterpret_cast<uint32_t>(address.m_ptr), scratchReg);
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

    void add32(TrustedImm32 imm, AbsoluteAddress address, RegisterID scratchReg)
    {
        RegisterID result = claimScratch();

        m_assembler.loadConstant(reinterpret_cast<uint32_t>(address.m_ptr), scratchReg);
        m_assembler.movlMemReg(scratchReg, result);

        if (m_assembler.isImmediate(imm.m_value))
            m_assembler.addlImm8r(imm.m_value, result);
        else {
            m_assembler.loadConstant(imm.m_value, scratchReg3);
            m_assembler.addlRegReg(scratchReg3, result);
        }

        store32(result, scratchReg);
        releaseScratch(result);
    }

    void add32(TrustedImm32 imm, AbsoluteAddress address)
    {
        RegisterID result = claimScratch();
        RegisterID scratchReg = claimScratch();

        m_assembler.loadConstant(reinterpret_cast<uint32_t>(address.m_ptr), scratchReg);
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

    void sub32(TrustedImm32 imm, RegisterID dest)
    {
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

    void xor32(TrustedImm32 imm, RegisterID srcDest)
    {
        if ((srcDest != SH4Registers::r0) || (imm.m_value > 255) || (imm.m_value < 0)) {
            RegisterID scr = claimScratch();
            m_assembler.loadConstant((imm.m_value), scr);
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
        RegisterID scr1 = claimScratch();

        if ((offset < 0) || (offset >= 64)) {
            m_assembler.loadConstant(offset, scr);
            m_assembler.addlRegReg(base, scr);
            m_assembler.movlMemReg(scr, scr);
        } else if (offset)
            m_assembler.movlMemReg(offset >> 2, base, scr);
        else
            m_assembler.movlMemReg(base, scr);
        if (m_assembler.isImmediate(imm))
            m_assembler.movImm8(imm, scr1);
        else
            m_assembler.loadConstant(imm, scr1);

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
        if (!offset) {
            RegisterID scr = claimScratch();
            RegisterID scr1 = claimScratch();
            m_assembler.movlMemReg(base, scr);
            m_assembler.loadConstant(imm, scr1);
            m_assembler.cmplRegReg(scr1, scr, SH4Condition(cond));
            releaseScratch(scr1);
            releaseScratch(scr);
            return;
        }

        if ((offset < 0) || (offset >= 64)) {
            RegisterID scr = claimScratch();
            RegisterID scr1 = claimScratch();
            m_assembler.loadConstant(offset, scr);
            m_assembler.addlRegReg(base, scr);
            m_assembler.movlMemReg(scr, scr);
            m_assembler.loadConstant(imm, scr1);
            m_assembler.cmplRegReg(scr1, scr, SH4Condition(cond));
            releaseScratch(scr1);
            releaseScratch(scr);
            return;
        }

        RegisterID scr = claimScratch();
        RegisterID scr1 = claimScratch();
        m_assembler.movlMemReg(offset >> 2, base, scr);
        m_assembler.loadConstant(imm, scr1);
        m_assembler.cmplRegReg(scr1, scr, SH4Condition(cond));
        releaseScratch(scr1);
        releaseScratch(scr);
    }

    // Memory access operation

    void load32(ImplicitAddress address, RegisterID dest)
    {
        load32(address.base, address.offset, dest);
    }

    void load8(ImplicitAddress address, RegisterID dest)
    {
        load8(address.base, address.offset, dest);
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

    void load32(void* address, RegisterID dest)
    {
        m_assembler.loadConstant(reinterpret_cast<uint32_t>(address), dest);
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

        if ((dest == SH4Registers::r0) && (dest != base)) {
            m_assembler.loadConstant((offset), dest);
            m_assembler.movlR0mr(base, dest);
            return;
        }

        RegisterID scr;
        if (dest == base)
            scr = claimScratch();
        else
            scr = dest;
        m_assembler.loadConstant((offset), scr);
        m_assembler.addlRegReg(base, scr);
        m_assembler.movlMemReg(scr, dest);

        if (dest == base)
            releaseScratch(scr);
    }

    void load8(RegisterID base, int offset, RegisterID dest)
    {
        if (!offset) {
            m_assembler.movbMemReg(base, dest);
            return;
        }

        if ((offset > 0) && (offset < 64) && (dest == SH4Registers::r0)) {
            m_assembler.movbMemReg(offset, base, dest);
            return;
        }

        if (base != dest) {
            m_assembler.loadConstant((offset), dest);
            m_assembler.addlRegReg(base, dest);
            m_assembler.movbMemReg(dest, dest);
            return;
        }

        RegisterID scr = claimScratch();
        m_assembler.loadConstant((offset), scr);
        m_assembler.addlRegReg(base, scr);
        m_assembler.movbMemReg(scr, dest);
        releaseScratch(scr);
    }

    void load32(RegisterID r0, RegisterID src, RegisterID dst)
    {
        ASSERT(r0 == SH4Registers::r0);
        m_assembler.movlR0mr(src, dst);
    }

    void load32(RegisterID src, RegisterID dst)
    {
        m_assembler.movlMemReg(src, dst);
    }

    void load16(ImplicitAddress address, RegisterID dest)
    {
        if (!address.offset) {
            m_assembler.movwMemReg(address.base, dest);
            return;
        }

        if ((address.offset > 0) && (address.offset < 64) && (dest == SH4Registers::r0)) {
            m_assembler.movwMemReg(address.offset, address.base, dest);
            return;
        }

        if (address.base != dest) {
            m_assembler.loadConstant((address.offset), dest);
            m_assembler.addlRegReg(address.base, dest);
            m_assembler.movwMemReg(dest, dest);
            return;
        }

        RegisterID scr = claimScratch();
        m_assembler.loadConstant((address.offset), scr);
        m_assembler.addlRegReg(address.base, scr);
        m_assembler.movwMemReg(scr, dest);
        releaseScratch(scr);
    }

    void load16(RegisterID src, RegisterID dest)
    {
        m_assembler.movwMemReg(src, dest);
    }

    void load16(RegisterID r0, RegisterID src, RegisterID dest)
    {
        ASSERT(r0 == SH4Registers::r0);
        m_assembler.movwR0mr(src, dest);
    }

    void load16(BaseIndex address, RegisterID dest)
    {
        RegisterID scr = claimScratch();

        move(address.index, scr);
        lshift32(TrustedImm32(address.scale), scr);

        if (address.offset)
            add32(TrustedImm32(address.offset), scr);
        if (scr == SH4Registers::r0)
            m_assembler.movwR0mr(address.base, scr);
        else {
            add32(address.base, scr);
            load16(scr, scr);
        }

        extuw(scr, dest);
        releaseScratch(scr);
    }

    void store32(RegisterID src, ImplicitAddress address)
    {
        RegisterID scr = claimScratch();
        store32(src, address.offset, address.base, scr);
        releaseScratch(scr);
    }

    void store32(RegisterID src, int offset, RegisterID base, RegisterID scr)
    {
        if (!offset) {
            m_assembler.movlRegMem(src, base);
            return;
        }

        if ((offset >=0) && (offset < 64)) {
            m_assembler.movlRegMem(src, offset >> 2, base);
            return;
        }

        m_assembler.loadConstant((offset), scr);
        if (scr == SH4Registers::r0) {
            m_assembler.movlRegMemr0(src, base);
            return;
        }

        m_assembler.addlRegReg(base, scr);
        m_assembler.movlRegMem(src, scr);
    }

    void store32(RegisterID src, RegisterID offset, RegisterID base)
    {
        ASSERT(offset == SH4Registers::r0);
        m_assembler.movlRegMemr0(src, base);
    }

    void store32(RegisterID src, RegisterID dst)
    {
        m_assembler.movlRegMem(src, dst);
    }

    void store32(TrustedImm32 imm, ImplicitAddress address)
    {
        RegisterID scr = claimScratch();
        RegisterID scr1 = claimScratch();
        m_assembler.loadConstant((imm.m_value), scr);
        store32(scr, address.offset, address.base, scr1);
        releaseScratch(scr);
        releaseScratch(scr1);
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
        m_assembler.loadConstant((imm.m_value), scr);
        m_assembler.loadConstant(reinterpret_cast<uint32_t>(address), scr1);
        m_assembler.movlMemReg(scr, scr1);
        releaseScratch(scr);
        releaseScratch(scr1);
    }

    void store32(RegisterID src, void* address)
    {
        RegisterID scr = claimScratch();
        m_assembler.loadConstant(reinterpret_cast<uint32_t>(address), scr);
        m_assembler.movlMemReg(src, scr);
        releaseScratch(scr);
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

     // Floating-point operations

    bool supportsFloatingPoint() const { return true; }
    bool supportsFloatingPointTruncate() const { return true; }
    bool supportsFloatingPointSqrt() const { return true; }

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
        m_assembler.loadConstant(reinterpret_cast<uint32_t>(address), scr);
        m_assembler.fmovsReadrminc(scr, (FPRegisterID)(dest + 1));
        m_assembler.fmovsReadrm(scr, dest);
        releaseScratch(scr);
    }

    void storeDouble(FPRegisterID src, ImplicitAddress address)
    {
        RegisterID scr = claimScratch();
        m_assembler.loadConstant(address.offset, scr);
        m_assembler.addlRegReg(address.base, scr);
        m_assembler.fmovsWriterm((FPRegisterID)(src + 1), scr);
        m_assembler.addlImm8r(4, scr);
        m_assembler.fmovsWriterm(src, scr);
        releaseScratch(scr);
    }

    void addDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.daddRegReg(src, dest);
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

    void subDouble(Address address, FPRegisterID dest)
    {
        loadDouble(address, fscratch);
        subDouble(fscratch, dest);
    }

    void mulDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.dmulRegReg(src, dest);
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

    void convertInt32ToDouble(RegisterID src, FPRegisterID dest)
    {
        m_assembler.ldsrmfpul(src);
        m_assembler.floatfpulDreg(dest);
    }

    void convertInt32ToDouble(AbsoluteAddress src, FPRegisterID dest)
    {
        RegisterID scr = claimScratch();
        m_assembler.loadConstant(reinterpret_cast<uint32_t>(src.m_ptr), scr);
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

        move(address.index, scr);
        lshift32(TrustedImm32(address.scale), scr);
        add32(address.base, scr);

        if (address.offset)
            add32(TrustedImm32(address.offset), scr);

        RegisterID scr1 = claimScratch();
        load16(scr, scr1);
        add32(TrustedImm32(2), scr);
        load16(scr, dest);
        move(TrustedImm32(16), scr);
        m_assembler.shllRegReg(dest, scr);
        or32(scr1, dest);

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
            RegisterID scr = claimScratch();
            m_assembler.loadConstant(0x7fbfffff, scratchReg3);
            m_assembler.dcnvds(right);
            m_assembler.stsfpulReg(scr);
            m_assembler.cmplRegReg(scratchReg3, scr, SH4Condition(Equal));
            m_assembler.ensureSpace(m_assembler.maxInstructionSize + 22, sizeof(uint32_t));
            m_assembler.branch(BT_OPCODE, 8);
            m_assembler.dcnvds(left);
            m_assembler.stsfpulReg(scr);
            m_assembler.cmplRegReg(scratchReg3, scr, SH4Condition(Equal));
            m_assembler.branch(BT_OPCODE, 4);
            m_assembler.dcmppeq(right, left);
            releaseScratch(scr);
            return branchFalse();
        }

        if (cond == DoubleGreaterThan) {
            m_assembler.dcmppgt(right, left);
            return branchTrue();
        }

        if (cond == DoubleGreaterThanOrEqual) {
            m_assembler.dcmppgt(left, right);
            return branchFalse();
        }

        if (cond == DoubleLessThan) {
            m_assembler.dcmppgt(left, right);
            return branchTrue();
        }

        if (cond == DoubleLessThanOrEqual) {
            m_assembler.dcmppgt(right, left);
            return branchFalse();
        }

        if (cond == DoubleEqualOrUnordered) {
            RegisterID scr = claimScratch();
            m_assembler.loadConstant(0x7fbfffff, scratchReg3);
            m_assembler.dcnvds(right);
            m_assembler.stsfpulReg(scr);
            m_assembler.cmplRegReg(scratchReg3, scr, SH4Condition(Equal));
            m_assembler.ensureSpace(m_assembler.maxInstructionSize + 22, sizeof(uint32_t));
            m_assembler.branch(BT_OPCODE, 5);
            m_assembler.dcnvds(left);
            m_assembler.stsfpulReg(scr);
            m_assembler.cmplRegReg(scratchReg3, scr, SH4Condition(Equal));
            m_assembler.branch(BT_OPCODE, 1);
            m_assembler.dcmppeq(left, right);
            releaseScratch(scr);
            return branchTrue();
        }

        if (cond == DoubleGreaterThanOrUnordered) {
            RegisterID scr = claimScratch();
            m_assembler.loadConstant(0x7fbfffff, scratchReg3);
            m_assembler.dcnvds(right);
            m_assembler.stsfpulReg(scr);
            m_assembler.cmplRegReg(scratchReg3, scr, SH4Condition(Equal));
            m_assembler.ensureSpace(m_assembler.maxInstructionSize + 22, sizeof(uint32_t));
            m_assembler.branch(BT_OPCODE, 5);
            m_assembler.dcnvds(left);
            m_assembler.stsfpulReg(scr);
            m_assembler.cmplRegReg(scratchReg3, scr, SH4Condition(Equal));
            m_assembler.branch(BT_OPCODE, 1);
            m_assembler.dcmppgt(right, left);
            releaseScratch(scr);
            return branchTrue();
        }

        if (cond == DoubleGreaterThanOrEqualOrUnordered) {
            RegisterID scr = claimScratch();
            m_assembler.loadConstant(0x7fbfffff, scratchReg3);
            m_assembler.dcnvds(right);
            m_assembler.stsfpulReg(scr);
            m_assembler.cmplRegReg(scratchReg3, scr, SH4Condition(Equal));
            m_assembler.ensureSpace(m_assembler.maxInstructionSize + 22, sizeof(uint32_t));
            m_assembler.branch(BT_OPCODE, 5);
            m_assembler.dcnvds(left);
            m_assembler.stsfpulReg(scr);
            m_assembler.cmplRegReg(scratchReg3, scr, SH4Condition(Equal));
            m_assembler.branch(BT_OPCODE, 1);
            m_assembler.dcmppgt(left, right);
            releaseScratch(scr);
            return branchFalse();
        }

        if (cond == DoubleLessThanOrUnordered) {
            RegisterID scr = claimScratch();
            m_assembler.loadConstant(0x7fbfffff, scratchReg3);
            m_assembler.dcnvds(right);
            m_assembler.stsfpulReg(scr);
            m_assembler.cmplRegReg(scratchReg3, scr, SH4Condition(Equal));
            m_assembler.ensureSpace(m_assembler.maxInstructionSize + 22, sizeof(uint32_t));
            m_assembler.branch(BT_OPCODE, 5);
            m_assembler.dcnvds(left);
            m_assembler.stsfpulReg(scr);
            m_assembler.cmplRegReg(scratchReg3, scr, SH4Condition(Equal));
            m_assembler.branch(BT_OPCODE, 1);
            m_assembler.dcmppgt(left, right);
            releaseScratch(scr);
            return branchTrue();
        }

        if (cond == DoubleLessThanOrEqualOrUnordered) {
            RegisterID scr = claimScratch();
            m_assembler.loadConstant(0x7fbfffff, scratchReg3);
            m_assembler.dcnvds(right);
            m_assembler.stsfpulReg(scr);
            m_assembler.cmplRegReg(scratchReg3, scr, SH4Condition(Equal));
            m_assembler.ensureSpace(m_assembler.maxInstructionSize + 22, sizeof(uint32_t));
            m_assembler.branch(BT_OPCODE, 5);
            m_assembler.dcnvds(left);
            m_assembler.stsfpulReg(scr);
            m_assembler.cmplRegReg(scratchReg3, scr, SH4Condition(Equal));
            m_assembler.branch(BT_OPCODE, 1);
            m_assembler.dcmppgt(right, left);
            releaseScratch(scr);
            return branchFalse();
        }

        ASSERT(cond == DoubleNotEqualOrUnordered);
        RegisterID scr = claimScratch();
        m_assembler.loadConstant(0x7fbfffff, scratchReg3);
        m_assembler.dcnvds(right);
        m_assembler.stsfpulReg(scr);
        m_assembler.cmplRegReg(scratchReg3, scr, SH4Condition(Equal));
        m_assembler.ensureSpace(m_assembler.maxInstructionSize + 22, sizeof(uint32_t));
        m_assembler.branch(BT_OPCODE, 5);
        m_assembler.dcnvds(left);
        m_assembler.stsfpulReg(scr);
        m_assembler.cmplRegReg(scratchReg3, scr, SH4Condition(Equal));
        m_assembler.branch(BT_OPCODE, 1);
        m_assembler.dcmppeq(right, left);
        releaseScratch(scr);
        return branchFalse();
    }

    Jump branchTrue()
    {
        m_assembler.ensureSpace(m_assembler.maxInstructionSize + 6, sizeof(uint32_t));
        Jump m_jump = Jump(m_assembler.je());
        m_assembler.loadConstantUnReusable(0x0, scratchReg3);
        m_assembler.nop();
        m_assembler.nop();
        return m_jump;
    }

    Jump branchFalse()
    {
        m_assembler.ensureSpace(m_assembler.maxInstructionSize + 6, sizeof(uint32_t));
        Jump m_jump = Jump(m_assembler.jne());
        m_assembler.loadConstantUnReusable(0x0, scratchReg3);
        m_assembler.nop();
        m_assembler.nop();
        return m_jump;
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
        if (dest != src)
            m_assembler.dmovRegReg(src, dest);
        m_assembler.dsqrt(dest);
    }

    Jump branchTest8(ResultCondition cond, Address address, TrustedImm32 mask = TrustedImm32(-1))
    {
        RegisterID addressTempRegister = claimScratch();
        load8(address, addressTempRegister);
        Jump jmp = branchTest32(cond, addressTempRegister, mask);
        releaseScratch(addressTempRegister);
        return jmp;
    }

    void signExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        if (src != dest)
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

    Jump branchTruncateDoubleToInt32(FPRegisterID src, RegisterID dest)
    {
        m_assembler.ftrcdrmfpul(src);
        m_assembler.stsfpulReg(dest);
        m_assembler.loadConstant(0x7fffffff, scratchReg3);
        m_assembler.cmplRegReg(dest, scratchReg3, SH4Condition(Equal));
        m_assembler.ensureSpace(m_assembler.maxInstructionSize + 14, sizeof(uint32_t));
        m_assembler.branch(BT_OPCODE, 2);
        m_assembler.addlImm8r(1, scratchReg3);
        m_assembler.cmplRegReg(dest, scratchReg3, SH4Condition(Equal));
        return branchTrue();
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

    void push(Address address)
    {
        if (!address.offset) {
            push(address.base);
            return;
        }

        if ((address.offset < 0) || (address.offset >= 64)) {
            RegisterID scr = claimScratch();
            m_assembler.loadConstant(address.offset, scr);
            m_assembler.addlRegReg(address.base, scr);
            m_assembler.movlMemReg(scr, SH4Registers::sp);
            m_assembler.addlImm8r(-4, SH4Registers::sp);
            releaseScratch(scr);
            return;
        }

        m_assembler.movlMemReg(address.offset >> 2, address.base, SH4Registers::sp);
        m_assembler.addlImm8r(-4, SH4Registers::sp);
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
        DataLabelPtr dataLabel(this);
        m_assembler.loadConstantUnReusable(reinterpret_cast<uint32_t>(initialValue.m_value), dest, true);
        return dataLabel;
    }

    void move(RegisterID src, RegisterID dest)
    {
        m_assembler.movlRegReg(src, dest);
    }

    void move(TrustedImmPtr imm, RegisterID dest)
    {
        m_assembler.loadConstant(imm.asIntptr(), dest);
    }

    void extuw(RegisterID src, RegisterID dst)
    {
        m_assembler.extuw(src, dst);
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

        move(TrustedImm32(reinterpret_cast<uint32_t>(left.m_ptr)), scr);
        m_assembler.cmplRegReg(right, scr, SH4Condition(cond));
        releaseScratch(scr);

        if (cond == NotEqual)
            return branchFalse();
        return branchTrue();
    }

    Jump branch32(RelationalCondition cond, AbsoluteAddress left, TrustedImm32 right)
    {
        RegisterID addressTempRegister = claimScratch();

        m_assembler.loadConstant(reinterpret_cast<uint32_t>(left.m_ptr), addressTempRegister);
        m_assembler.movlMemReg(addressTempRegister, addressTempRegister);
        compare32(right.m_value, addressTempRegister, cond);
        releaseScratch(addressTempRegister);

        if (cond == NotEqual)
            return branchFalse();
        return branchTrue();
    }

    Jump branch16(RelationalCondition cond,  BaseIndex left, RegisterID right)
    {
        RegisterID scr = claimScratch();

        move(left.index, scr);
        lshift32(TrustedImm32(left.scale), scr);

        if (left.offset)
            add32(TrustedImm32(left.offset), scr);
        add32(left.base, scr);
        load16(scr, scr);
        extuw(scr, scr);
        releaseScratch(scr);

        return branch32(cond, scr, right);
    }

    Jump branch16(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        RegisterID scr = claimScratch();

        move(left.index, scr);
        lshift32(TrustedImm32(left.scale), scr);

        if (left.offset)
            add32(TrustedImm32(left.offset), scr);
        add32(left.base, scr);
        load16(scr, scr);
        extuw(scr, scr);
        RegisterID scr1 = claimScratch();
        m_assembler.loadConstant(right.m_value, scr1);
        releaseScratch(scr);
        releaseScratch(scr1);

        return branch32(cond, scr, scr1);
    }

    Jump branchTest32(ResultCondition cond, RegisterID reg, RegisterID mask)
    {
        ASSERT((cond == Zero) || (cond == NonZero));

        m_assembler.testlRegReg(reg, mask);

        if (cond == NotEqual)
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

        if (cond == NotEqual)
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

        if (cond == NotEqual)
            return branchFalse();
        return branchTrue();
    }

    Jump branchTest32(ResultCondition cond, BaseIndex address, TrustedImm32 mask = TrustedImm32(-1))
    {
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

        if (cond == NotEqual)
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

        if ((address.offset < 0) || (address.offset >= 64)) {
            m_assembler.loadConstant(address.offset, scr);
            m_assembler.addlRegReg(address.base, scr);
            m_assembler.movlMemReg(scr, scr);
        } else if (address.offset)
            m_assembler.movlMemReg(address.offset >> 2, address.base, scr);
        else
            m_assembler.movlMemReg(address.base, scr);
        m_assembler.jmpReg(scr);

        releaseScratch(scr);
    }

    // Arithmetic control flow operations

    Jump branchAdd32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));

        if (cond == Overflow) {
            m_assembler.addvlRegReg(src, dest);
            return branchTrue();
        }

        if (cond == Signed) {
            m_assembler.addlRegReg(src, dest);
            // Check if dest is negative
            m_assembler.cmppz(dest);
            return branchFalse();
        }

        m_assembler.addlRegReg(src, dest);
        compare32(0, dest, Equal);

        if (cond == NotEqual)
            return branchFalse();
        return branchTrue();
    }

    Jump branchAdd32(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));

        move(imm, scratchReg3);
        return branchAdd32(cond, scratchReg3, dest);
    }

    Jump branchMul32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));

        if (cond == Overflow) {
            RegisterID scr1 = claimScratch();
            RegisterID scr = claimScratch();
            m_assembler.dmullRegReg(src, dest);
            m_assembler.stsmacl(dest);
            m_assembler.movImm8(-31, scr);
            m_assembler.movlRegReg(dest, scr1);
            m_assembler.shaRegReg(scr1, scr);
            m_assembler.stsmach(scr);
            m_assembler.cmplRegReg(scr, scr1, SH4Condition(Equal));
            releaseScratch(scr1);
            releaseScratch(scr);
            return branchFalse();
        }

        m_assembler.imullRegReg(src, dest);
        m_assembler.stsmacl(dest);
        if (cond == Signed) {
            // Check if dest is negative
            m_assembler.cmppz(dest);
            return branchFalse();
        }

        compare32(0, dest, static_cast<RelationalCondition>(cond));

        if (cond == NotEqual)
            return branchFalse();
        return branchTrue();
    }

    Jump branchMul32(ResultCondition cond, TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));

        move(imm, scratchReg3);
        if (src != dest)
            move(src, dest);

        return branchMul32(cond, scratchReg3, dest);
    }

    Jump branchSub32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));

        if (cond == Overflow) {
            m_assembler.subvlRegReg(src, dest);
            return branchTrue();
        }

        if (cond == Signed) {
            // Check if dest is negative
            m_assembler.sublRegReg(src, dest);
            compare32(0, dest, LessThan);
            return branchTrue();
        }

        sub32(src, dest);
        compare32(0, dest, static_cast<RelationalCondition>(cond));

        if (cond == NotEqual)
            return branchFalse();
        return branchTrue();
    }

    Jump branchSub32(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));

        move(imm, scratchReg3);
        return branchSub32(cond, scratchReg3, dest);
    }

    Jump branchOr32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Signed) || (cond == Zero) || (cond == NonZero));

        if (cond == Signed) {
            or32(src, dest);
            compare32(0, dest, static_cast<RelationalCondition>(LessThan));
            return branchTrue();
        }

        or32(src, dest);
        compare32(0, dest, static_cast<RelationalCondition>(cond));

        if (cond == NotEqual)
            return branchFalse();
        return branchTrue();
    }

    void branchConvertDoubleToInt32(FPRegisterID src, RegisterID dest, JumpList& failureCases, FPRegisterID fpTemp)
    {
        m_assembler.ftrcdrmfpul(src);
        m_assembler.stsfpulReg(dest);
        convertInt32ToDouble(dest, fscratch);
        failureCases.append(branchDouble(DoubleNotEqualOrUnordered, fscratch, src));

        if (dest == SH4Registers::r0)
            m_assembler.cmpEqImmR0(0, dest);
        else {
            m_assembler.movImm8(0, scratchReg3);
            m_assembler.cmplRegReg(scratchReg3, dest, SH4Condition(Equal));
        }
        failureCases.append(branchTrue());
    }

    void neg32(RegisterID dst)
    {
        m_assembler.neg(dst, dst);
    }

    void not32(RegisterID dst)
    {
        m_assembler.notlReg(dst, dst);
    }

    void urshift32(RegisterID shiftamount, RegisterID dest)
    {
        compare32(32, shiftamount, Equal);
        m_assembler.ensureSpace(m_assembler.maxInstructionSize + 4);
        m_assembler.branch(BT_OPCODE, 1);
        m_assembler.neg(shiftamount, shiftamount);
        m_assembler.shllRegReg(dest, shiftamount);
    }

    void urshift32(TrustedImm32 imm, RegisterID dest)
    {
        RegisterID scr = claimScratch();
        m_assembler.loadConstant(-(imm.m_value), scr);
        m_assembler.shaRegReg(dest, scr);
        releaseScratch(scr);
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
        m_assembler.bkpt();
        m_assembler.nop();
    }

    Jump branchPtrWithPatch(RelationalCondition cond, RegisterID left, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(0))
    {
        RegisterID dataTempRegister = claimScratch();

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

    static void linkCall(void*, Call, FunctionPtr);
    static void repatchCall(CodeLocationCall, CodeLocationLabel);
    static void repatchCall(CodeLocationCall, FunctionPtr);
};

} // namespace JSC

#endif // ENABLE(ASSEMBLER)

#endif // MacroAssemblerSH4_h
