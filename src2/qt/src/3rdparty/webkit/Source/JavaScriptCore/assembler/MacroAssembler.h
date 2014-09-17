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

#ifndef MacroAssembler_h
#define MacroAssembler_h

#if ENABLE(ASSEMBLER)

#if CPU(ARM_THUMB2)
#include "MacroAssemblerARMv7.h"
namespace JSC { typedef MacroAssemblerARMv7 MacroAssemblerBase; };

#elif CPU(ARM_TRADITIONAL)
#include "MacroAssemblerARM.h"
namespace JSC { typedef MacroAssemblerARM MacroAssemblerBase; };

#elif CPU(MIPS)
#include "MacroAssemblerMIPS.h"
namespace JSC {
typedef MacroAssemblerMIPS MacroAssemblerBase;
};

#elif CPU(X86)
#include "MacroAssemblerX86.h"
namespace JSC { typedef MacroAssemblerX86 MacroAssemblerBase; };

#elif CPU(X86_64)
#include "MacroAssemblerX86_64.h"
namespace JSC { typedef MacroAssemblerX86_64 MacroAssemblerBase; };

#elif CPU(SH4)
#include "MacroAssemblerSH4.h"
namespace JSC {
typedef MacroAssemblerSH4 MacroAssemblerBase;
};

#else
#error "The MacroAssembler is not supported on this platform."
#endif


namespace JSC {

class MacroAssembler : public MacroAssemblerBase {
public:

    using MacroAssemblerBase::pop;
    using MacroAssemblerBase::jump;
    using MacroAssemblerBase::branch32;
    using MacroAssemblerBase::branch16;
#if CPU(X86_64)
    using MacroAssemblerBase::branchPtr;
    using MacroAssemblerBase::branchTestPtr;
#endif


    // Platform agnostic onvenience functions,
    // described in terms of other macro assembly methods.
    void pop()
    {
        addPtr(TrustedImm32(sizeof(void*)), stackPointerRegister);
    }
    
    void peek(RegisterID dest, int index = 0)
    {
        loadPtr(Address(stackPointerRegister, (index * sizeof(void*))), dest);
    }

    void poke(RegisterID src, int index = 0)
    {
        storePtr(src, Address(stackPointerRegister, (index * sizeof(void*))));
    }

    void poke(TrustedImm32 value, int index = 0)
    {
        store32(value, Address(stackPointerRegister, (index * sizeof(void*))));
    }

    void poke(TrustedImmPtr imm, int index = 0)
    {
        storePtr(imm, Address(stackPointerRegister, (index * sizeof(void*))));
    }


    // Backwards banches, these are currently all implemented using existing forwards branch mechanisms.
    void branchPtr(RelationalCondition cond, RegisterID op1, TrustedImmPtr imm, Label target)
    {
        branchPtr(cond, op1, imm).linkTo(target, this);
    }

    void branch32(RelationalCondition cond, RegisterID op1, RegisterID op2, Label target)
    {
        branch32(cond, op1, op2).linkTo(target, this);
    }

    void branch32(RelationalCondition cond, RegisterID op1, TrustedImm32 imm, Label target)
    {
        branch32(cond, op1, imm).linkTo(target, this);
    }

    void branch32(RelationalCondition cond, RegisterID left, Address right, Label target)
    {
        branch32(cond, left, right).linkTo(target, this);
    }

    void branch16(RelationalCondition cond, BaseIndex left, RegisterID right, Label target)
    {
        branch16(cond, left, right).linkTo(target, this);
    }
    
    void branchTestPtr(ResultCondition cond, RegisterID reg, Label target)
    {
        branchTestPtr(cond, reg).linkTo(target, this);
    }

    void jump(Label target)
    {
        jump().linkTo(target, this);
    }


    // Ptr methods
    // On 32-bit platforms (i.e. x86), these methods directly map onto their 32-bit equivalents.
    // FIXME: should this use a test for 32-bitness instead of this specific exception?
#if !CPU(X86_64)
    void addPtr(RegisterID src, RegisterID dest)
    {
        add32(src, dest);
    }

    void addPtr(TrustedImm32 imm, RegisterID srcDest)
    {
        add32(imm, srcDest);
    }

    void addPtr(TrustedImmPtr imm, RegisterID dest)
    {
        add32(TrustedImm32(imm), dest);
    }

    void addPtr(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        add32(imm, src, dest);
    }

    void andPtr(RegisterID src, RegisterID dest)
    {
        and32(src, dest);
    }

    void andPtr(TrustedImm32 imm, RegisterID srcDest)
    {
        and32(imm, srcDest);
    }

    void orPtr(RegisterID src, RegisterID dest)
    {
        or32(src, dest);
    }

    void orPtr(TrustedImmPtr imm, RegisterID dest)
    {
        or32(TrustedImm32(imm), dest);
    }

    void orPtr(TrustedImm32 imm, RegisterID dest)
    {
        or32(imm, dest);
    }

    void subPtr(RegisterID src, RegisterID dest)
    {
        sub32(src, dest);
    }
    
    void subPtr(TrustedImm32 imm, RegisterID dest)
    {
        sub32(imm, dest);
    }
    
    void subPtr(TrustedImmPtr imm, RegisterID dest)
    {
        sub32(TrustedImm32(imm), dest);
    }

    void xorPtr(RegisterID src, RegisterID dest)
    {
        xor32(src, dest);
    }

    void xorPtr(TrustedImm32 imm, RegisterID srcDest)
    {
        xor32(imm, srcDest);
    }


    void loadPtr(ImplicitAddress address, RegisterID dest)
    {
        load32(address, dest);
    }

    void loadPtr(BaseIndex address, RegisterID dest)
    {
        load32(address, dest);
    }

    void loadPtr(void* address, RegisterID dest)
    {
        load32(address, dest);
    }

    DataLabel32 loadPtrWithAddressOffsetPatch(Address address, RegisterID dest)
    {
        return load32WithAddressOffsetPatch(address, dest);
    }

    void comparePtr(RelationalCondition cond, RegisterID left, TrustedImm32 right, RegisterID dest)
    {
        compare32(cond, left, right, dest);
    }

    void storePtr(RegisterID src, ImplicitAddress address)
    {
        store32(src, address);
    }

    void storePtr(RegisterID src, BaseIndex address)
    {
        store32(src, address);
    }

    void storePtr(RegisterID src, void* address)
    {
        store32(src, address);
    }

    void storePtr(TrustedImmPtr imm, ImplicitAddress address)
    {
        store32(TrustedImm32(imm), address);
    }

    void storePtr(TrustedImmPtr imm, void* address)
    {
        store32(TrustedImm32(imm), address);
    }

    DataLabel32 storePtrWithAddressOffsetPatch(RegisterID src, Address address)
    {
        return store32WithAddressOffsetPatch(src, address);
    }


    Jump branchPtr(RelationalCondition cond, RegisterID left, RegisterID right)
    {
        return branch32(cond, left, right);
    }

    Jump branchPtr(RelationalCondition cond, RegisterID left, TrustedImmPtr right)
    {
        return branch32(cond, left, TrustedImm32(right));
    }

    Jump branchPtr(RelationalCondition cond, RegisterID left, Address right)
    {
        return branch32(cond, left, right);
    }

    Jump branchPtr(RelationalCondition cond, Address left, RegisterID right)
    {
        return branch32(cond, left, right);
    }

    Jump branchPtr(RelationalCondition cond, AbsoluteAddress left, RegisterID right)
    {
        return branch32(cond, left, right);
    }

    Jump branchPtr(RelationalCondition cond, Address left, TrustedImmPtr right)
    {
        return branch32(cond, left, TrustedImm32(right));
    }

    Jump branchPtr(RelationalCondition cond, AbsoluteAddress left, TrustedImmPtr right)
    {
        return branch32(cond, left, TrustedImm32(right));
    }

    Jump branchTestPtr(ResultCondition cond, RegisterID reg, RegisterID mask)
    {
        return branchTest32(cond, reg, mask);
    }

    Jump branchTestPtr(ResultCondition cond, RegisterID reg, TrustedImm32 mask = TrustedImm32(-1))
    {
        return branchTest32(cond, reg, mask);
    }

    Jump branchTestPtr(ResultCondition cond, Address address, TrustedImm32 mask = TrustedImm32(-1))
    {
        return branchTest32(cond, address, mask);
    }

    Jump branchTestPtr(ResultCondition cond, BaseIndex address, TrustedImm32 mask = TrustedImm32(-1))
    {
        return branchTest32(cond, address, mask);
    }


    Jump branchAddPtr(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        return branchAdd32(cond, src, dest);
    }

    Jump branchSubPtr(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        return branchSub32(cond, imm, dest);
    }
    using MacroAssemblerBase::branchTest8;
    Jump branchTest8(ResultCondition cond, ExtendedAddress address, TrustedImm32 mask = TrustedImm32(-1))
    {
        return MacroAssemblerBase::branchTest8(cond, Address(address.base, address.offset), mask);
    }
#endif

};

} // namespace JSC

#endif // ENABLE(ASSEMBLER)

#endif // MacroAssembler_h
