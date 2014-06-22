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

#ifndef MacroAssemblerX86Common_h
#define MacroAssemblerX86Common_h

#if ENABLE(ASSEMBLER)

#include "X86Assembler.h"
#include "AbstractMacroAssembler.h"

namespace JSC {

class MacroAssemblerX86Common : public AbstractMacroAssembler<X86Assembler> {
protected:
#if CPU(X86_64)
    static const X86Registers::RegisterID scratchRegister = X86Registers::r11;
#endif

    static const int DoubleConditionBitInvert = 0x10;
    static const int DoubleConditionBitSpecial = 0x20;
    static const int DoubleConditionBits = DoubleConditionBitInvert | DoubleConditionBitSpecial;

public:
    typedef X86Assembler::FPRegisterID FPRegisterID;
    typedef X86Assembler::XMMRegisterID XMMRegisterID;
    
    static bool isCompactPtrAlignedAddressOffset(ptrdiff_t value)
    {
        return value >= -128 && value <= 127;
    }

    enum RelationalCondition {
        Equal = X86Assembler::ConditionE,
        NotEqual = X86Assembler::ConditionNE,
        Above = X86Assembler::ConditionA,
        AboveOrEqual = X86Assembler::ConditionAE,
        Below = X86Assembler::ConditionB,
        BelowOrEqual = X86Assembler::ConditionBE,
        GreaterThan = X86Assembler::ConditionG,
        GreaterThanOrEqual = X86Assembler::ConditionGE,
        LessThan = X86Assembler::ConditionL,
        LessThanOrEqual = X86Assembler::ConditionLE
    };

    enum ResultCondition {
        Overflow = X86Assembler::ConditionO,
        Signed = X86Assembler::ConditionS,
        PositiveOrZero = X86Assembler::ConditionNS,
        Zero = X86Assembler::ConditionE,
        NonZero = X86Assembler::ConditionNE
    };

    enum DoubleCondition {
        // These conditions will only evaluate to true if the comparison is ordered - i.e. neither operand is NaN.
        DoubleEqual = X86Assembler::ConditionE | DoubleConditionBitSpecial,
        DoubleNotEqual = X86Assembler::ConditionNE,
        DoubleGreaterThan = X86Assembler::ConditionA,
        DoubleGreaterThanOrEqual = X86Assembler::ConditionAE,
        DoubleLessThan = X86Assembler::ConditionA | DoubleConditionBitInvert,
        DoubleLessThanOrEqual = X86Assembler::ConditionAE | DoubleConditionBitInvert,
        // If either operand is NaN, these conditions always evaluate to true.
        DoubleEqualOrUnordered = X86Assembler::ConditionE,
        DoubleNotEqualOrUnordered = X86Assembler::ConditionNE | DoubleConditionBitSpecial,
        DoubleGreaterThanOrUnordered = X86Assembler::ConditionB | DoubleConditionBitInvert,
        DoubleGreaterThanOrEqualOrUnordered = X86Assembler::ConditionBE | DoubleConditionBitInvert,
        DoubleLessThanOrUnordered = X86Assembler::ConditionB,
        DoubleLessThanOrEqualOrUnordered = X86Assembler::ConditionBE,
    };
    COMPILE_ASSERT(
        !((X86Assembler::ConditionE | X86Assembler::ConditionNE | X86Assembler::ConditionA | X86Assembler::ConditionAE | X86Assembler::ConditionB | X86Assembler::ConditionBE) & DoubleConditionBits),
        DoubleConditionBits_should_not_interfere_with_X86Assembler_Condition_codes);

    static const RegisterID stackPointerRegister = X86Registers::esp;

#if ENABLE(JIT_CONSTANT_BLINDING)
    static bool shouldBlindForSpecificArch(uint32_t value) { return value >= 0x00ffffff; }
#if CPU(X86_64)
    static bool shouldBlindForSpecificArch(uint64_t value) { return value >= 0x00ffffff; }
#if OS(DARWIN) // On 64-bit systems other than DARWIN uint64_t and uintptr_t are the same type so overload is prohibited.
    static bool shouldBlindForSpecificArch(uintptr_t value) { return value >= 0x00ffffff; }
#endif
#endif
#endif

    // Integer arithmetic operations:
    //
    // Operations are typically two operand - operation(source, srcDst)
    // For many operations the source may be an TrustedImm32, the srcDst operand
    // may often be a memory location (explictly described using an Address
    // object).

    void add32(RegisterID src, RegisterID dest)
    {
        m_assembler.addl_rr(src, dest);
    }

    void add32(TrustedImm32 imm, Address address)
    {
        m_assembler.addl_im(imm.m_value, address.offset, address.base);
    }

    void add32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.addl_ir(imm.m_value, dest);
    }
    
    void add32(Address src, RegisterID dest)
    {
        m_assembler.addl_mr(src.offset, src.base, dest);
    }

    void add32(RegisterID src, Address dest)
    {
        m_assembler.addl_rm(src, dest.offset, dest.base);
    }

    void add32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        m_assembler.leal_mr(imm.m_value, src, dest);
    }
    
    void and32(RegisterID src, RegisterID dest)
    {
        m_assembler.andl_rr(src, dest);
    }

    void and32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.andl_ir(imm.m_value, dest);
    }

    void and32(RegisterID src, Address dest)
    {
        m_assembler.andl_rm(src, dest.offset, dest.base);
    }

    void and32(Address src, RegisterID dest)
    {
        m_assembler.andl_mr(src.offset, src.base, dest);
    }

    void and32(TrustedImm32 imm, Address address)
    {
        m_assembler.andl_im(imm.m_value, address.offset, address.base);
    }

    void and32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        if (op1 == op2)
            zeroExtend32ToPtr(op1, dest);
        else if (op1 == dest)
            and32(op2, dest);
        else {
            move(op2, dest);
            and32(op1, dest);
        }
    }

    void and32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        move(src, dest);
        and32(imm, dest);
    }

    void lshift32(RegisterID shift_amount, RegisterID dest)
    {
        ASSERT(shift_amount != dest);

        if (shift_amount == X86Registers::ecx)
            m_assembler.shll_CLr(dest);
        else {
            // On x86 we can only shift by ecx; if asked to shift by another register we'll
            // need rejig the shift amount into ecx first, and restore the registers afterwards.
            // If we dest is ecx, then shift the swapped register!
            swap(shift_amount, X86Registers::ecx);
            m_assembler.shll_CLr(dest == X86Registers::ecx ? shift_amount : dest);
            swap(shift_amount, X86Registers::ecx);
        }
    }

    void lshift32(RegisterID src, RegisterID shift_amount, RegisterID dest)
    {
        ASSERT(shift_amount != dest);

        if (src != dest)
            move(src, dest);
        lshift32(shift_amount, dest);
    }

    void lshift32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.shll_i8r(imm.m_value, dest);
    }
    
    void lshift32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        if (src != dest)
            move(src, dest);
        lshift32(imm, dest);
    }
    
    void mul32(RegisterID src, RegisterID dest)
    {
        m_assembler.imull_rr(src, dest);
    }

    void mul32(Address src, RegisterID dest)
    {
        m_assembler.imull_mr(src.offset, src.base, dest);
    }
    
    void mul32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        m_assembler.imull_i32r(src, imm.m_value, dest);
    }

    void neg32(RegisterID srcDest)
    {
        m_assembler.negl_r(srcDest);
    }

    void neg32(Address srcDest)
    {
        m_assembler.negl_m(srcDest.offset, srcDest.base);
    }

    void or32(RegisterID src, RegisterID dest)
    {
        m_assembler.orl_rr(src, dest);
    }

    void or32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.orl_ir(imm.m_value, dest);
    }

    void or32(RegisterID src, Address dest)
    {
        m_assembler.orl_rm(src, dest.offset, dest.base);
    }

    void or32(Address src, RegisterID dest)
    {
        m_assembler.orl_mr(src.offset, src.base, dest);
    }

    void or32(TrustedImm32 imm, Address address)
    {
        m_assembler.orl_im(imm.m_value, address.offset, address.base);
    }

    void or32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        if (op1 == op2)
            zeroExtend32ToPtr(op1, dest);
        else if (op1 == dest)
            or32(op2, dest);
        else {
            move(op2, dest);
            or32(op1, dest);
        }
    }

    void or32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        move(src, dest);
        or32(imm, dest);
    }

    void rshift32(RegisterID shift_amount, RegisterID dest)
    {
        ASSERT(shift_amount != dest);

        if (shift_amount == X86Registers::ecx)
            m_assembler.sarl_CLr(dest);
        else {
            // On x86 we can only shift by ecx; if asked to shift by another register we'll
            // need rejig the shift amount into ecx first, and restore the registers afterwards.
            // If we dest is ecx, then shift the swapped register!
            swap(shift_amount, X86Registers::ecx);
            m_assembler.sarl_CLr(dest == X86Registers::ecx ? shift_amount : dest);
            swap(shift_amount, X86Registers::ecx);
        }
    }

    void rshift32(RegisterID src, RegisterID shift_amount, RegisterID dest)
    {
        ASSERT(shift_amount != dest);

        if (src != dest)
            move(src, dest);
        rshift32(shift_amount, dest);
    }

    void rshift32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.sarl_i8r(imm.m_value, dest);
    }
    
    void rshift32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        if (src != dest)
            move(src, dest);
        rshift32(imm, dest);
    }
    
    void urshift32(RegisterID shift_amount, RegisterID dest)
    {
        ASSERT(shift_amount != dest);

        if (shift_amount == X86Registers::ecx)
            m_assembler.shrl_CLr(dest);
        else {
            // On x86 we can only shift by ecx; if asked to shift by another register we'll
            // need rejig the shift amount into ecx first, and restore the registers afterwards.
            // If we dest is ecx, then shift the swapped register!
            swap(shift_amount, X86Registers::ecx);
            m_assembler.shrl_CLr(dest == X86Registers::ecx ? shift_amount : dest);
            swap(shift_amount, X86Registers::ecx);
        }
    }

    void urshift32(RegisterID src, RegisterID shift_amount, RegisterID dest)
    {
        ASSERT(shift_amount != dest);

        if (src != dest)
            move(src, dest);
        urshift32(shift_amount, dest);
    }

    void urshift32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.shrl_i8r(imm.m_value, dest);
    }
    
    void urshift32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        if (src != dest)
            move(src, dest);
        urshift32(imm, dest);
    }
    
    void sub32(RegisterID src, RegisterID dest)
    {
        m_assembler.subl_rr(src, dest);
    }
    
    void sub32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.subl_ir(imm.m_value, dest);
    }
    
    void sub32(TrustedImm32 imm, Address address)
    {
        m_assembler.subl_im(imm.m_value, address.offset, address.base);
    }

    void sub32(Address src, RegisterID dest)
    {
        m_assembler.subl_mr(src.offset, src.base, dest);
    }

    void sub32(RegisterID src, Address dest)
    {
        m_assembler.subl_rm(src, dest.offset, dest.base);
    }

    void xor32(RegisterID src, RegisterID dest)
    {
        m_assembler.xorl_rr(src, dest);
    }

    void xor32(TrustedImm32 imm, Address dest)
    {
        if (imm.m_value == -1)
            m_assembler.notl_m(dest.offset, dest.base);
        else
            m_assembler.xorl_im(imm.m_value, dest.offset, dest.base);
    }

    void xor32(TrustedImm32 imm, RegisterID dest)
    {
        if (imm.m_value == -1)
        m_assembler.notl_r(dest);
        else
        m_assembler.xorl_ir(imm.m_value, dest);
    }

    void xor32(RegisterID src, Address dest)
    {
        m_assembler.xorl_rm(src, dest.offset, dest.base);
    }

    void xor32(Address src, RegisterID dest)
    {
        m_assembler.xorl_mr(src.offset, src.base, dest);
    }
    
    void xor32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        if (op1 == op2)
            move(TrustedImm32(0), dest);
        else if (op1 == dest)
            xor32(op2, dest);
        else {
            move(op2, dest);
            xor32(op1, dest);
        }
    }

    void xor32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        move(src, dest);
        xor32(imm, dest);
    }

    void sqrtDouble(FPRegisterID src, FPRegisterID dst)
    {
        m_assembler.sqrtsd_rr(src, dst);
    }

    void absDouble(FPRegisterID src, FPRegisterID dst)
    {
        ASSERT(src != dst);
        static const double negativeZeroConstant = -0.0;
        loadDouble(&negativeZeroConstant, dst);
        m_assembler.andnpd_rr(src, dst);
    }

    void negateDouble(FPRegisterID src, FPRegisterID dst)
    {
        ASSERT(src != dst);
        static const double negativeZeroConstant = -0.0;
        loadDouble(&negativeZeroConstant, dst);
        m_assembler.xorpd_rr(src, dst);
    }


    // Memory access operations:
    //
    // Loads are of the form load(address, destination) and stores of the form
    // store(source, address).  The source for a store may be an TrustedImm32.  Address
    // operand objects to loads and store will be implicitly constructed if a
    // register is passed.

    void load32(ImplicitAddress address, RegisterID dest)
    {
        m_assembler.movl_mr(address.offset, address.base, dest);
    }

    void load32(BaseIndex address, RegisterID dest)
    {
        m_assembler.movl_mr(address.offset, address.base, address.index, address.scale, dest);
    }

    void load32WithUnalignedHalfWords(BaseIndex address, RegisterID dest)
    {
        load32(address, dest);
    }

    void load16Unaligned(BaseIndex address, RegisterID dest)
    {
        load16(address, dest);
    }

    DataLabel32 load32WithAddressOffsetPatch(Address address, RegisterID dest)
    {
        padBeforePatch();
        m_assembler.movl_mr_disp32(address.offset, address.base, dest);
        return DataLabel32(this);
    }
    
    DataLabelCompact load32WithCompactAddressOffsetPatch(Address address, RegisterID dest)
    {
        padBeforePatch();
        m_assembler.movl_mr_disp8(address.offset, address.base, dest);
        return DataLabelCompact(this);
    }
    
    static void repatchCompact(CodeLocationDataLabelCompact dataLabelCompact, int32_t value)
    {
        ASSERT(isCompactPtrAlignedAddressOffset(value));
        AssemblerType_T::repatchCompact(dataLabelCompact.dataLocation(), value);
    }
    
    DataLabelCompact loadCompactWithAddressOffsetPatch(Address address, RegisterID dest)
    {
        padBeforePatch();
        m_assembler.movl_mr_disp8(address.offset, address.base, dest);
        return DataLabelCompact(this);
    }

    void load8(BaseIndex address, RegisterID dest)
    {
        m_assembler.movzbl_mr(address.offset, address.base, address.index, address.scale, dest);
    }

    void load8(ImplicitAddress address, RegisterID dest)
    {
        m_assembler.movzbl_mr(address.offset, address.base, dest);
    }
    
    void load8Signed(BaseIndex address, RegisterID dest)
    {
        m_assembler.movsbl_mr(address.offset, address.base, address.index, address.scale, dest);
    }

    void load8Signed(ImplicitAddress address, RegisterID dest)
    {
        m_assembler.movsbl_mr(address.offset, address.base, dest);
    }
    
    void load16(BaseIndex address, RegisterID dest)
    {
        m_assembler.movzwl_mr(address.offset, address.base, address.index, address.scale, dest);
    }
    
    void load16(Address address, RegisterID dest)
    {
        m_assembler.movzwl_mr(address.offset, address.base, dest);
    }

    void load16Signed(BaseIndex address, RegisterID dest)
    {
        m_assembler.movswl_mr(address.offset, address.base, address.index, address.scale, dest);
    }
    
    void load16Signed(Address address, RegisterID dest)
    {
        m_assembler.movswl_mr(address.offset, address.base, dest);
    }

    DataLabel32 store32WithAddressOffsetPatch(RegisterID src, Address address)
    {
        padBeforePatch();
        m_assembler.movl_rm_disp32(src, address.offset, address.base);
        return DataLabel32(this);
    }

    void store32(RegisterID src, ImplicitAddress address)
    {
        m_assembler.movl_rm(src, address.offset, address.base);
    }

    void store32(RegisterID src, BaseIndex address)
    {
        m_assembler.movl_rm(src, address.offset, address.base, address.index, address.scale);
    }

    void store32(TrustedImm32 imm, ImplicitAddress address)
    {
        m_assembler.movl_i32m(imm.m_value, address.offset, address.base);
    }
    
    void store32(TrustedImm32 imm, BaseIndex address)
    {
        m_assembler.movl_i32m(imm.m_value, address.offset, address.base, address.index, address.scale);
    }

    void store8(TrustedImm32 imm, Address address)
    {
        ASSERT(-128 <= imm.m_value && imm.m_value < 128);
        m_assembler.movb_i8m(imm.m_value, address.offset, address.base);
    }

    void store8(TrustedImm32 imm, BaseIndex address)
    {
        ASSERT(-128 <= imm.m_value && imm.m_value < 128);
        m_assembler.movb_i8m(imm.m_value, address.offset, address.base, address.index, address.scale);
    }
    
    void store8(RegisterID src, BaseIndex address)
    {
#if CPU(X86)
        // On 32-bit x86 we can only store from the first 4 registers;
        // esp..edi are mapped to the 'h' registers!
        if (src >= 4) {
            // Pick a temporary register.
            RegisterID temp;
            if (address.base != X86Registers::eax && address.index != X86Registers::eax)
                temp = X86Registers::eax;
            else if (address.base != X86Registers::ebx && address.index != X86Registers::ebx)
                temp = X86Registers::ebx;
            else {
                ASSERT(address.base != X86Registers::ecx && address.index != X86Registers::ecx);
                temp = X86Registers::ecx;
            }

            // Swap to the temporary register to perform the store.
            swap(src, temp);
            m_assembler.movb_rm(temp, address.offset, address.base, address.index, address.scale);
            swap(src, temp);
            return;
        }
#endif
        m_assembler.movb_rm(src, address.offset, address.base, address.index, address.scale);
    }

    void store16(RegisterID src, BaseIndex address)
    {
#if CPU(X86)
        // On 32-bit x86 we can only store from the first 4 registers;
        // esp..edi are mapped to the 'h' registers!
        if (src >= 4) {
            // Pick a temporary register.
            RegisterID temp;
            if (address.base != X86Registers::eax && address.index != X86Registers::eax)
                temp = X86Registers::eax;
            else if (address.base != X86Registers::ebx && address.index != X86Registers::ebx)
                temp = X86Registers::ebx;
            else {
                ASSERT(address.base != X86Registers::ecx && address.index != X86Registers::ecx);
                temp = X86Registers::ecx;
            }
            
            // Swap to the temporary register to perform the store.
            swap(src, temp);
            m_assembler.movw_rm(temp, address.offset, address.base, address.index, address.scale);
            swap(src, temp);
            return;
        }
#endif
        m_assembler.movw_rm(src, address.offset, address.base, address.index, address.scale);
    }


    // Floating-point operation:
    //
    // Presently only supports SSE, not x87 floating point.

    void moveDouble(FPRegisterID src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        if (src != dest)
            m_assembler.movsd_rr(src, dest);
    }

    void loadDouble(const void* address, FPRegisterID dest)
    {
#if CPU(X86)
        ASSERT(isSSE2Present());
        m_assembler.movsd_mr(address, dest);
#else
        move(TrustedImmPtr(address), scratchRegister);
        loadDouble(scratchRegister, dest);
#endif
    }

    void loadDouble(ImplicitAddress address, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.movsd_mr(address.offset, address.base, dest);
    }
    
    void loadDouble(BaseIndex address, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.movsd_mr(address.offset, address.base, address.index, address.scale, dest);
    }
    void loadFloat(BaseIndex address, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.movss_mr(address.offset, address.base, address.index, address.scale, dest);
    }

    void storeDouble(FPRegisterID src, ImplicitAddress address)
    {
        ASSERT(isSSE2Present());
        m_assembler.movsd_rm(src, address.offset, address.base);
    }
    
    void storeDouble(FPRegisterID src, BaseIndex address)
    {
        ASSERT(isSSE2Present());
        m_assembler.movsd_rm(src, address.offset, address.base, address.index, address.scale);
    }
    
    void storeFloat(FPRegisterID src, BaseIndex address)
    {
        ASSERT(isSSE2Present());
        m_assembler.movss_rm(src, address.offset, address.base, address.index, address.scale);
    }
    
    void convertDoubleToFloat(FPRegisterID src, FPRegisterID dst)
    {
        ASSERT(isSSE2Present());
        m_assembler.cvtsd2ss_rr(src, dst);
    }

    void convertFloatToDouble(FPRegisterID src, FPRegisterID dst)
    {
        ASSERT(isSSE2Present());
        m_assembler.cvtss2sd_rr(src, dst);
    }

    void addDouble(FPRegisterID src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.addsd_rr(src, dest);
    }

    void addDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        if (op1 == dest)
            addDouble(op2, dest);
        else {
            moveDouble(op2, dest);
            addDouble(op1, dest);
        }
    }

    void addDouble(Address src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.addsd_mr(src.offset, src.base, dest);
    }

    void divDouble(FPRegisterID src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.divsd_rr(src, dest);
    }

    void divDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        // B := A / B is invalid.
        ASSERT(op1 == dest || op2 != dest);

        moveDouble(op1, dest);
        divDouble(op2, dest);
    }

    void divDouble(Address src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.divsd_mr(src.offset, src.base, dest);
    }

    void subDouble(FPRegisterID src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.subsd_rr(src, dest);
    }

    void subDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        // B := A - B is invalid.
        ASSERT(op1 == dest || op2 != dest);

        moveDouble(op1, dest);
        subDouble(op2, dest);
    }

    void subDouble(Address src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.subsd_mr(src.offset, src.base, dest);
    }

    void mulDouble(FPRegisterID src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.mulsd_rr(src, dest);
    }

    void mulDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        if (op1 == dest)
            mulDouble(op2, dest);
        else {
            moveDouble(op2, dest);
            mulDouble(op1, dest);
        }
    }

    void mulDouble(Address src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.mulsd_mr(src.offset, src.base, dest);
    }

    void convertInt32ToDouble(RegisterID src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.cvtsi2sd_rr(src, dest);
    }

    void convertInt32ToDouble(Address src, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.cvtsi2sd_mr(src.offset, src.base, dest);
    }

    Jump branchDouble(DoubleCondition cond, FPRegisterID left, FPRegisterID right)
    {
        ASSERT(isSSE2Present());

        if (cond & DoubleConditionBitInvert)
            m_assembler.ucomisd_rr(left, right);
        else
            m_assembler.ucomisd_rr(right, left);

        if (cond == DoubleEqual) {
            if (left == right)
                return Jump(m_assembler.jnp());
            Jump isUnordered(m_assembler.jp());
            Jump result = Jump(m_assembler.je());
            isUnordered.link(this);
            return result;
        } else if (cond == DoubleNotEqualOrUnordered) {
            if (left == right)
                return Jump(m_assembler.jp());
            Jump isUnordered(m_assembler.jp());
            Jump isEqual(m_assembler.je());
            isUnordered.link(this);
            Jump result = jump();
            isEqual.link(this);
            return result;
        }

        ASSERT(!(cond & DoubleConditionBitSpecial));
        return Jump(m_assembler.jCC(static_cast<X86Assembler::Condition>(cond & ~DoubleConditionBits)));
    }

    // Truncates 'src' to an integer, and places the resulting 'dest'.
    // If the result is not representable as a 32 bit value, branch.
    // May also branch for some values that are representable in 32 bits
    // (specifically, in this case, INT_MIN).
    enum BranchTruncateType { BranchIfTruncateFailed, BranchIfTruncateSuccessful };
    Jump branchTruncateDoubleToInt32(FPRegisterID src, RegisterID dest, BranchTruncateType branchType = BranchIfTruncateFailed)
    {
        ASSERT(isSSE2Present());
        m_assembler.cvttsd2si_rr(src, dest);
        return branch32(branchType ? NotEqual : Equal, dest, TrustedImm32(0x80000000));
    }

    Jump branchTruncateDoubleToUint32(FPRegisterID src, RegisterID dest, BranchTruncateType branchType = BranchIfTruncateFailed)
    {
        ASSERT(isSSE2Present());
        m_assembler.cvttsd2si_rr(src, dest);
        return branch32(branchType ? GreaterThanOrEqual : LessThan, dest, TrustedImm32(0));
    }

    void truncateDoubleToInt32(FPRegisterID src, RegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.cvttsd2si_rr(src, dest);
    }
    
#if CPU(X86_64)
    void truncateDoubleToUint32(FPRegisterID src, RegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.cvttsd2siq_rr(src, dest);
    }
#endif
    
    // Convert 'src' to an integer, and places the resulting 'dest'.
    // If the result is not representable as a 32 bit value, branch.
    // May also branch for some values that are representable in 32 bits
    // (specifically, in this case, 0).
    void branchConvertDoubleToInt32(FPRegisterID src, RegisterID dest, JumpList& failureCases, FPRegisterID fpTemp, bool negZeroCheck = true)
    {
        ASSERT(isSSE2Present());
        m_assembler.cvttsd2si_rr(src, dest);

        // If the result is zero, it might have been -0.0, and the double comparison won't catch this!
        if (negZeroCheck)
            failureCases.append(branchTest32(Zero, dest));

        // Convert the integer result back to float & compare to the original value - if not equal or unordered (NaN) then jump.
        convertInt32ToDouble(dest, fpTemp);
        m_assembler.ucomisd_rr(fpTemp, src);
        failureCases.append(m_assembler.jp());
        failureCases.append(m_assembler.jne());
    }

    Jump branchDoubleNonZero(FPRegisterID reg, FPRegisterID scratch)
    {
        ASSERT(isSSE2Present());
        m_assembler.xorpd_rr(scratch, scratch);
        return branchDouble(DoubleNotEqual, reg, scratch);
    }

    Jump branchDoubleZeroOrNaN(FPRegisterID reg, FPRegisterID scratch)
    {
        ASSERT(isSSE2Present());
        m_assembler.xorpd_rr(scratch, scratch);
        return branchDouble(DoubleEqualOrUnordered, reg, scratch);
    }

    void lshiftPacked(TrustedImm32 imm, XMMRegisterID reg)
    {
        ASSERT(isSSE2Present());
        m_assembler.psllq_i8r(imm.m_value, reg);
    }

    void rshiftPacked(TrustedImm32 imm, XMMRegisterID reg)
    {
        ASSERT(isSSE2Present());
        m_assembler.psrlq_i8r(imm.m_value, reg);
    }

    void orPacked(XMMRegisterID src, XMMRegisterID dst)
    {
        ASSERT(isSSE2Present());
        m_assembler.por_rr(src, dst);
    }

    void moveInt32ToPacked(RegisterID src, XMMRegisterID dst)
    {
        ASSERT(isSSE2Present());
        m_assembler.movd_rr(src, dst);
    }

    void movePackedToInt32(XMMRegisterID src, RegisterID dst)
    {
        ASSERT(isSSE2Present());
        m_assembler.movd_rr(src, dst);
    }

    // Stack manipulation operations:
    //
    // The ABI is assumed to provide a stack abstraction to memory,
    // containing machine word sized units of data.  Push and pop
    // operations add and remove a single register sized unit of data
    // to or from the stack.  Peek and poke operations read or write
    // values on the stack, without moving the current stack position.
    
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
        m_assembler.push_m(address.offset, address.base);
    }

    void push(TrustedImm32 imm)
    {
        m_assembler.push_i32(imm.m_value);
    }


    // Register move operations:
    //
    // Move values in registers.

    void move(TrustedImm32 imm, RegisterID dest)
    {
        // Note: on 64-bit the TrustedImm32 value is zero extended into the register, it
        // may be useful to have a separate version that sign extends the value?
        if (!imm.m_value)
            m_assembler.xorl_rr(dest, dest);
        else
            m_assembler.movl_i32r(imm.m_value, dest);
    }

#if CPU(X86_64)
    void move(RegisterID src, RegisterID dest)
    {
        // Note: on 64-bit this is is a full register move; perhaps it would be
        // useful to have separate move32 & movePtr, with move32 zero extending?
        if (src != dest)
            m_assembler.movq_rr(src, dest);
    }

    void move(TrustedImmPtr imm, RegisterID dest)
    {
        m_assembler.movq_i64r(imm.asIntptr(), dest);
    }

    void move(TrustedImm64 imm, RegisterID dest)
    {
        m_assembler.movq_i64r(imm.m_value, dest);
    }

    void swap(RegisterID reg1, RegisterID reg2)
    {
        if (reg1 != reg2)
            m_assembler.xchgq_rr(reg1, reg2);
    }

    void signExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        m_assembler.movsxd_rr(src, dest);
    }

    void zeroExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        m_assembler.movl_rr(src, dest);
    }
#else
    void move(RegisterID src, RegisterID dest)
    {
        if (src != dest)
            m_assembler.movl_rr(src, dest);
    }

    void move(TrustedImmPtr imm, RegisterID dest)
    {
        m_assembler.movl_i32r(imm.asIntptr(), dest);
    }

    void swap(RegisterID reg1, RegisterID reg2)
    {
        if (reg1 != reg2)
            m_assembler.xchgl_rr(reg1, reg2);
    }

    void signExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        move(src, dest);
    }

    void zeroExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        move(src, dest);
    }
#endif


    // Forwards / external control flow operations:
    //
    // This set of jump and conditional branch operations return a Jump
    // object which may linked at a later point, allow forwards jump,
    // or jumps that will require external linkage (after the code has been
    // relocated).
    //
    // For branches, signed <, >, <= and >= are denoted as l, g, le, and ge
    // respecitvely, for unsigned comparisons the names b, a, be, and ae are
    // used (representing the names 'below' and 'above').
    //
    // Operands to the comparision are provided in the expected order, e.g.
    // jle32(reg1, TrustedImm32(5)) will branch if the value held in reg1, when
    // treated as a signed 32bit value, is less than or equal to 5.
    //
    // jz and jnz test whether the first operand is equal to zero, and take
    // an optional second operand of a mask under which to perform the test.

public:
    Jump branch8(RelationalCondition cond, Address left, TrustedImm32 right)
    {
        m_assembler.cmpb_im(right.m_value, left.offset, left.base);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branch32(RelationalCondition cond, RegisterID left, RegisterID right)
    {
        m_assembler.cmpl_rr(right, left);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branch32(RelationalCondition cond, RegisterID left, TrustedImm32 right)
    {
        if (((cond == Equal) || (cond == NotEqual)) && !right.m_value)
            m_assembler.testl_rr(left, left);
        else
            m_assembler.cmpl_ir(right.m_value, left);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }
    
    Jump branch32(RelationalCondition cond, RegisterID left, Address right)
    {
        m_assembler.cmpl_mr(right.offset, right.base, left);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }
    
    Jump branch32(RelationalCondition cond, Address left, RegisterID right)
    {
        m_assembler.cmpl_rm(right, left.offset, left.base);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branch32(RelationalCondition cond, Address left, TrustedImm32 right)
    {
        m_assembler.cmpl_im(right.m_value, left.offset, left.base);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branch32(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        m_assembler.cmpl_im(right.m_value, left.offset, left.base, left.index, left.scale);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branch32WithUnalignedHalfWords(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        return branch32(cond, left, right);
    }

    Jump branchTest32(ResultCondition cond, RegisterID reg, RegisterID mask)
    {
        m_assembler.testl_rr(reg, mask);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchTest32(ResultCondition cond, RegisterID reg, TrustedImm32 mask = TrustedImm32(-1))
    {
        // if we are only interested in the low seven bits, this can be tested with a testb
        if (mask.m_value == -1)
            m_assembler.testl_rr(reg, reg);
        else
            m_assembler.testl_i32r(mask.m_value, reg);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchTest32(ResultCondition cond, Address address, TrustedImm32 mask = TrustedImm32(-1))
    {
        if (mask.m_value == -1)
            m_assembler.cmpl_im(0, address.offset, address.base);
        else
            m_assembler.testl_i32m(mask.m_value, address.offset, address.base);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchTest32(ResultCondition cond, BaseIndex address, TrustedImm32 mask = TrustedImm32(-1))
    {
        if (mask.m_value == -1)
            m_assembler.cmpl_im(0, address.offset, address.base, address.index, address.scale);
        else
            m_assembler.testl_i32m(mask.m_value, address.offset, address.base, address.index, address.scale);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }
    
    Jump branchTest8(ResultCondition cond, Address address, TrustedImm32 mask = TrustedImm32(-1))
    {
        // Byte in TrustedImm32 is not well defined, so be a little permisive here, but don't accept nonsense values.
        ASSERT(mask.m_value >= -128 && mask.m_value <= 255);
        if (mask.m_value == -1)
            m_assembler.cmpb_im(0, address.offset, address.base);
        else
            m_assembler.testb_im(mask.m_value, address.offset, address.base);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }
    
    Jump branchTest8(ResultCondition cond, BaseIndex address, TrustedImm32 mask = TrustedImm32(-1))
    {
        // Byte in TrustedImm32 is not well defined, so be a little permisive here, but don't accept nonsense values.
        ASSERT(mask.m_value >= -128 && mask.m_value <= 255);
        if (mask.m_value == -1)
            m_assembler.cmpb_im(0, address.offset, address.base, address.index, address.scale);
        else
            m_assembler.testb_im(mask.m_value, address.offset, address.base, address.index, address.scale);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branch8(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        ASSERT(!(right.m_value & 0xFFFFFF00));

        m_assembler.cmpb_im(right.m_value, left.offset, left.base, left.index, left.scale);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump jump()
    {
        return Jump(m_assembler.jmp());
    }

    void jump(RegisterID target)
    {
        m_assembler.jmp_r(target);
    }

    // Address is a memory location containing the address to jump to
    void jump(Address address)
    {
        m_assembler.jmp_m(address.offset, address.base);
    }


    // Arithmetic control flow operations:
    //
    // This set of conditional branch operations branch based
    // on the result of an arithmetic operation.  The operation
    // is performed as normal, storing the result.
    //
    // * jz operations branch if the result is zero.
    // * jo operations branch if the (signed) arithmetic
    //   operation caused an overflow to occur.
    
    Jump branchAdd32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        add32(src, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchAdd32(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        add32(imm, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }
    
    Jump branchAdd32(ResultCondition cond, TrustedImm32 src, Address dest)
    {
        add32(src, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchAdd32(ResultCondition cond, RegisterID src, Address dest)
    {
        add32(src, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchAdd32(ResultCondition cond, Address src, RegisterID dest)
    {
        add32(src, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchAdd32(ResultCondition cond, RegisterID src1, RegisterID src2, RegisterID dest)
    {
        if (src1 == dest)
            return branchAdd32(cond, src2, dest);
        move(src2, dest);
        return branchAdd32(cond, src1, dest);
    }

    Jump branchAdd32(ResultCondition cond, RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        move(src, dest);
        return branchAdd32(cond, imm, dest);
    }

    Jump branchMul32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        mul32(src, dest);
        if (cond != Overflow)
            m_assembler.testl_rr(dest, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchMul32(ResultCondition cond, Address src, RegisterID dest)
    {
        mul32(src, dest);
        if (cond != Overflow)
            m_assembler.testl_rr(dest, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }
    
    Jump branchMul32(ResultCondition cond, TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        mul32(imm, src, dest);
        if (cond != Overflow)
            m_assembler.testl_rr(dest, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }
    
    Jump branchMul32(ResultCondition cond, RegisterID src1, RegisterID src2, RegisterID dest)
    {
        if (src1 == dest)
            return branchMul32(cond, src2, dest);
        move(src2, dest);
        return branchMul32(cond, src1, dest);
    }

    Jump branchSub32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        sub32(src, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }
    
    Jump branchSub32(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        sub32(imm, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchSub32(ResultCondition cond, TrustedImm32 imm, Address dest)
    {
        sub32(imm, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchSub32(ResultCondition cond, RegisterID src, Address dest)
    {
        sub32(src, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchSub32(ResultCondition cond, Address src, RegisterID dest)
    {
        sub32(src, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchSub32(ResultCondition cond, RegisterID src1, RegisterID src2, RegisterID dest)
    {
        // B := A - B is invalid.
        ASSERT(src1 == dest || src2 != dest);

        move(src1, dest);
        return branchSub32(cond, src2, dest);
    }

    Jump branchSub32(ResultCondition cond, RegisterID src1, TrustedImm32 src2, RegisterID dest)
    {
        move(src1, dest);
        return branchSub32(cond, src2, dest);
    }

    Jump branchNeg32(ResultCondition cond, RegisterID srcDest)
    {
        neg32(srcDest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchOr32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        or32(src, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }


    // Miscellaneous operations:

    void breakpoint()
    {
        m_assembler.int3();
    }

    Call nearCall()
    {
        return Call(m_assembler.call(), Call::LinkableNear);
    }

    Call call(RegisterID target)
    {
        return Call(m_assembler.call(target), Call::None);
    }

    void call(Address address)
    {
        m_assembler.call_m(address.offset, address.base);
    }

    void ret()
    {
        m_assembler.ret();
    }

    void compare8(RelationalCondition cond, Address left, TrustedImm32 right, RegisterID dest)
    {
        m_assembler.cmpb_im(right.m_value, left.offset, left.base);
        set32(x86Condition(cond), dest);
    }
    
    void compare32(RelationalCondition cond, RegisterID left, RegisterID right, RegisterID dest)
    {
        m_assembler.cmpl_rr(right, left);
        set32(x86Condition(cond), dest);
    }

    void compare32(RelationalCondition cond, RegisterID left, TrustedImm32 right, RegisterID dest)
    {
        if (((cond == Equal) || (cond == NotEqual)) && !right.m_value)
            m_assembler.testl_rr(left, left);
        else
            m_assembler.cmpl_ir(right.m_value, left);
        set32(x86Condition(cond), dest);
    }

    // FIXME:
    // The mask should be optional... perhaps the argument order should be
    // dest-src, operations always have a dest? ... possibly not true, considering
    // asm ops like test, or pseudo ops like pop().

    void test8(ResultCondition cond, Address address, TrustedImm32 mask, RegisterID dest)
    {
        if (mask.m_value == -1)
            m_assembler.cmpb_im(0, address.offset, address.base);
        else
            m_assembler.testb_im(mask.m_value, address.offset, address.base);
        set32(x86Condition(cond), dest);
    }

    void test32(ResultCondition cond, Address address, TrustedImm32 mask, RegisterID dest)
    {
        if (mask.m_value == -1)
            m_assembler.cmpl_im(0, address.offset, address.base);
        else
            m_assembler.testl_i32m(mask.m_value, address.offset, address.base);
        set32(x86Condition(cond), dest);
    }

    // Invert a relational condition, e.g. == becomes !=, < becomes >=, etc.
    static RelationalCondition invert(RelationalCondition cond)
    {
        return static_cast<RelationalCondition>(cond ^ 1);
    }

    void nop()
    {
        m_assembler.nop();
    }

    static void replaceWithJump(CodeLocationLabel instructionStart, CodeLocationLabel destination)
    {
        X86Assembler::replaceWithJump(instructionStart.executableAddress(), destination.executableAddress());
    }
    
    static ptrdiff_t maxJumpReplacementSize()
    {
        return X86Assembler::maxJumpReplacementSize();
    }

protected:
    X86Assembler::Condition x86Condition(RelationalCondition cond)
    {
        return static_cast<X86Assembler::Condition>(cond);
    }

    X86Assembler::Condition x86Condition(ResultCondition cond)
    {
        return static_cast<X86Assembler::Condition>(cond);
    }

    void set32(X86Assembler::Condition cond, RegisterID dest)
    {
#if CPU(X86)
        // On 32-bit x86 we can only set the first 4 registers;
        // esp..edi are mapped to the 'h' registers!
        if (dest >= 4) {
            m_assembler.xchgl_rr(dest, X86Registers::eax);
            m_assembler.setCC_r(cond, X86Registers::eax);
            m_assembler.movzbl_rr(X86Registers::eax, X86Registers::eax);
            m_assembler.xchgl_rr(dest, X86Registers::eax);
            return;
        }
#endif
        m_assembler.setCC_r(cond, dest);
        m_assembler.movzbl_rr(dest, dest);
    }

private:
    // Only MacroAssemblerX86 should be using the following method; SSE2 is always available on
    // x86_64, and clients & subclasses of MacroAssembler should be using 'supportsFloatingPoint()'.
    friend class MacroAssemblerX86;

#if CPU(X86)
#if OS(MAC_OS_X)

    // All X86 Macs are guaranteed to support at least SSE2,
    static bool isSSE2Present()
    {
        return true;
    }

#else // OS(MAC_OS_X)

    enum SSE2CheckState {
        NotCheckedSSE2,
        HasSSE2,
        NoSSE2
    };

    static bool isSSE2Present()
    {
        if (s_sse2CheckState == NotCheckedSSE2) {
            // Default the flags value to zero; if the compiler is
            // not MSVC or GCC we will read this as SSE2 not present.
            int flags = 0;
#if COMPILER(MSVC)
            _asm {
                mov eax, 1 // cpuid function 1 gives us the standard feature set
                cpuid;
                mov flags, edx;
            }
#elif COMPILER(GCC)
            asm (
                 "movl $0x1, %%eax;"
                 "pushl %%ebx;"
                 "cpuid;"
                 "popl %%ebx;"
                 "movl %%edx, %0;"
                 : "=g" (flags)
                 :
                 : "%eax", "%ecx", "%edx"
                 );
#endif
            static const int SSE2FeatureBit = 1 << 26;
            s_sse2CheckState = (flags & SSE2FeatureBit) ? HasSSE2 : NoSSE2;
        }
        // Only check once.
        ASSERT(s_sse2CheckState != NotCheckedSSE2);

        return s_sse2CheckState == HasSSE2;
    }
    
    static SSE2CheckState s_sse2CheckState;

#endif // OS(MAC_OS_X)
#elif !defined(NDEBUG) // CPU(X86)

    // On x86-64 we should never be checking for SSE2 in a non-debug build,
    // but non debug add this method to keep the asserts above happy.
    static bool isSSE2Present()
    {
        return true;
    }

#endif
};

} // namespace JSC

#endif // ENABLE(ASSEMBLER)

#endif // MacroAssemblerX86Common_h
