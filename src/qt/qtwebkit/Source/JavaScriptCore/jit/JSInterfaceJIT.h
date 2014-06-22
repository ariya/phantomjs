/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JSInterfaceJIT_h
#define JSInterfaceJIT_h

#include "BytecodeConventions.h"
#include "JITCode.h"
#include "JITStubs.h"
#include "JSCJSValue.h"
#include "JSStack.h"
#include "JSString.h"
#include "MacroAssembler.h"
#include <wtf/Vector.h>

#if ENABLE(JIT)

namespace JSC {
    class JSInterfaceJIT : public MacroAssembler {
    public:
        // NOTES:
        //
        // regT0 has two special meanings.  The return value from a stub
        // call will always be in regT0, and by default (unless
        // a register is specified) emitPutVirtualRegister() will store
        // the value from regT0.
        //
        // regT3 is required to be callee-preserved.
        //
        // tempRegister2 is has no such dependencies.  It is important that
        // on x86/x86-64 it is ecx for performance reasons, since the
        // MacroAssembler will need to plant register swaps if it is not -
        // however the code will still function correctly.
#if CPU(X86_64)
        static const RegisterID returnValueRegister = X86Registers::eax;
        static const RegisterID cachedResultRegister = X86Registers::eax;
#if !OS(WINDOWS)
        static const RegisterID firstArgumentRegister = X86Registers::edi;
#else
        static const RegisterID firstArgumentRegister = X86Registers::ecx;
#endif

#if ENABLE(VALUE_PROFILER)
        static const RegisterID bucketCounterRegister = X86Registers::r10;
#endif

        static const RegisterID callFrameRegister = X86Registers::r13;
        static const RegisterID tagTypeNumberRegister = X86Registers::r14;
        static const RegisterID tagMaskRegister = X86Registers::r15;

        static const RegisterID regT0 = X86Registers::eax;
        static const RegisterID regT1 = X86Registers::edx;
        static const RegisterID regT2 = X86Registers::ecx;
        static const RegisterID regT3 = X86Registers::ebx;

        static const FPRegisterID fpRegT0 = X86Registers::xmm0;
        static const FPRegisterID fpRegT1 = X86Registers::xmm1;
        static const FPRegisterID fpRegT2 = X86Registers::xmm2;
        static const FPRegisterID fpRegT3 = X86Registers::xmm3;

        static const RegisterID nonArgGPR1 = X86Registers::eax; // regT0
#elif CPU(X86)
        static const RegisterID returnValueRegister = X86Registers::eax;
        static const RegisterID cachedResultRegister = X86Registers::eax;
        // On x86 we always use fastcall conventions = but on
        // OS X if might make more sense to just use regparm.
        static const RegisterID firstArgumentRegister = X86Registers::ecx;
        
        static const RegisterID bucketCounterRegister = X86Registers::esi;
        static const RegisterID callFrameRegister = X86Registers::edi;
        
        static const RegisterID regT0 = X86Registers::eax;
        static const RegisterID regT1 = X86Registers::edx;
        static const RegisterID regT2 = X86Registers::ecx;
        static const RegisterID regT3 = X86Registers::ebx;
        
        static const FPRegisterID fpRegT0 = X86Registers::xmm0;
        static const FPRegisterID fpRegT1 = X86Registers::xmm1;
        static const FPRegisterID fpRegT2 = X86Registers::xmm2;
        static const FPRegisterID fpRegT3 = X86Registers::xmm3;
#elif CPU(ARM)
        static const RegisterID returnValueRegister = ARMRegisters::r0;
        static const RegisterID cachedResultRegister = ARMRegisters::r0;
        static const RegisterID firstArgumentRegister = ARMRegisters::r0;

#if ENABLE(VALUE_PROFILER)
        static const RegisterID bucketCounterRegister = ARMRegisters::r7;
#endif

        static const RegisterID regT0 = ARMRegisters::r0;
        static const RegisterID regT1 = ARMRegisters::r1;
        static const RegisterID regT2 = ARMRegisters::r2;
        static const RegisterID regT3 = ARMRegisters::r4;

        // Update ctiTrampoline in JITStubs.cpp if these values are changed!
        static const RegisterID callFrameRegister = ARMRegisters::r5;

        static const FPRegisterID fpRegT0 = ARMRegisters::d0;
        static const FPRegisterID fpRegT1 = ARMRegisters::d1;
        static const FPRegisterID fpRegT2 = ARMRegisters::d2;
        static const FPRegisterID fpRegT3 = ARMRegisters::d3;
#elif CPU(MIPS)
        static const RegisterID returnValueRegister = MIPSRegisters::v0;
        static const RegisterID cachedResultRegister = MIPSRegisters::v0;
        static const RegisterID firstArgumentRegister = MIPSRegisters::a0;
        
#if ENABLE(VALUE_PROFILER)
        static const RegisterID bucketCounterRegister = MIPSRegisters::s3;
#endif

        // regT0 must be v0 for returning a 32-bit value.
        static const RegisterID regT0 = MIPSRegisters::v0;
        
        // regT1 must be v1 for returning a pair of 32-bit value.
        static const RegisterID regT1 = MIPSRegisters::v1;
        
        static const RegisterID regT2 = MIPSRegisters::t4;
        
        // regT3 must be saved in the callee, so use an S register.
        static const RegisterID regT3 = MIPSRegisters::s2;
        
        static const RegisterID callFrameRegister = MIPSRegisters::s0;
        
        static const FPRegisterID fpRegT0 = MIPSRegisters::f4;
        static const FPRegisterID fpRegT1 = MIPSRegisters::f6;
        static const FPRegisterID fpRegT2 = MIPSRegisters::f8;
        static const FPRegisterID fpRegT3 = MIPSRegisters::f10;
#elif CPU(SH4)
        static const RegisterID callFrameRegister = SH4Registers::fp;

#if ENABLE(VALUE_PROFILER)
        static const RegisterID bucketCounterRegister = SH4Registers::r8;
#endif

        static const RegisterID regT0 = SH4Registers::r0;
        static const RegisterID regT1 = SH4Registers::r1;
        static const RegisterID regT2 = SH4Registers::r2;
        static const RegisterID regT3 = SH4Registers::r10;
        static const RegisterID regT4 = SH4Registers::r4;
        static const RegisterID regT5 = SH4Registers::r5;
        static const RegisterID regT6 = SH4Registers::r6;
        static const RegisterID regT7 = SH4Registers::r7;
        static const RegisterID firstArgumentRegister = regT4;

        static const RegisterID returnValueRegister = SH4Registers::r0;
        static const RegisterID cachedResultRegister = SH4Registers::r0;

        static const FPRegisterID fpRegT0 = SH4Registers::dr0;
        static const FPRegisterID fpRegT1 = SH4Registers::dr2;
        static const FPRegisterID fpRegT2 = SH4Registers::dr4;
        static const FPRegisterID fpRegT3 = SH4Registers::dr6;
        static const FPRegisterID fpRegT4 = SH4Registers::dr8;
        static const FPRegisterID fpRegT5 = SH4Registers::dr10;
#else
#error "JIT not supported on this platform."
#endif

#if USE(JSVALUE32_64)
        // Can't just propogate JSValue::Int32Tag as visual studio doesn't like it
        static const unsigned Int32Tag = 0xffffffff;
        COMPILE_ASSERT(Int32Tag == JSValue::Int32Tag, Int32Tag_out_of_sync);
#else
        static const unsigned Int32Tag = TagTypeNumber >> 32;
#endif
        inline Jump emitLoadJSCell(unsigned virtualRegisterIndex, RegisterID payload);
        inline Jump emitLoadInt32(unsigned virtualRegisterIndex, RegisterID dst);
        inline Jump emitLoadDouble(unsigned virtualRegisterIndex, FPRegisterID dst, RegisterID scratch);

#if USE(JSVALUE32_64)
        inline Jump emitJumpIfNotJSCell(unsigned virtualRegisterIndex);
        inline Address tagFor(int index, RegisterID base = callFrameRegister);
#endif

#if USE(JSVALUE64)
        Jump emitJumpIfNotJSCell(RegisterID);
        Jump emitJumpIfImmediateNumber(RegisterID reg);
        Jump emitJumpIfNotImmediateNumber(RegisterID reg);
        void emitFastArithImmToInt(RegisterID reg);
        void emitFastArithIntToImmNoCheck(RegisterID src, RegisterID dest);
#endif

        Jump emitJumpIfNotType(RegisterID baseReg, RegisterID scratchReg, JSType);

        void emitGetFromCallFrameHeaderPtr(JSStack::CallFrameHeaderEntry, RegisterID to, RegisterID from = callFrameRegister);
        void emitPutToCallFrameHeader(RegisterID from, JSStack::CallFrameHeaderEntry);
        void emitPutImmediateToCallFrameHeader(void* value, JSStack::CallFrameHeaderEntry);
        void emitPutCellToCallFrameHeader(RegisterID from, JSStack::CallFrameHeaderEntry);

        void preserveReturnAddressAfterCall(RegisterID);
        void restoreReturnAddressBeforeReturn(RegisterID);
        void restoreReturnAddressBeforeReturn(Address);
        void restoreArgumentReference();

        inline Address payloadFor(int index, RegisterID base = callFrameRegister);
        inline Address intPayloadFor(int index, RegisterID base = callFrameRegister);
        inline Address intTagFor(int index, RegisterID base = callFrameRegister);
        inline Address addressFor(int index, RegisterID base = callFrameRegister);
    };

    struct ThunkHelpers {
        static unsigned jsStringLengthOffset() { return OBJECT_OFFSETOF(JSString, m_length); }
        static unsigned jsStringValueOffset() { return OBJECT_OFFSETOF(JSString, m_value); }
    };

#if USE(JSVALUE32_64)
    inline JSInterfaceJIT::Jump JSInterfaceJIT::emitLoadJSCell(unsigned virtualRegisterIndex, RegisterID payload)
    {
        loadPtr(payloadFor(virtualRegisterIndex), payload);
        return emitJumpIfNotJSCell(virtualRegisterIndex);
    }

    inline JSInterfaceJIT::Jump JSInterfaceJIT::emitJumpIfNotJSCell(unsigned virtualRegisterIndex)
    {
        ASSERT(static_cast<int>(virtualRegisterIndex) < FirstConstantRegisterIndex);
        return branch32(NotEqual, tagFor(virtualRegisterIndex), TrustedImm32(JSValue::CellTag));
    }
    
    inline JSInterfaceJIT::Jump JSInterfaceJIT::emitLoadInt32(unsigned virtualRegisterIndex, RegisterID dst)
    {
        ASSERT(static_cast<int>(virtualRegisterIndex) < FirstConstantRegisterIndex);
        loadPtr(payloadFor(virtualRegisterIndex), dst);
        return branch32(NotEqual, tagFor(static_cast<int>(virtualRegisterIndex)), TrustedImm32(JSValue::Int32Tag));
    }
    
    inline JSInterfaceJIT::Address JSInterfaceJIT::tagFor(int virtualRegisterIndex, RegisterID base)
    {
        ASSERT(virtualRegisterIndex < FirstConstantRegisterIndex);
        return Address(base, (static_cast<unsigned>(virtualRegisterIndex) * sizeof(Register)) + OBJECT_OFFSETOF(JSValue, u.asBits.tag));
    }
    
    inline JSInterfaceJIT::Address JSInterfaceJIT::payloadFor(int virtualRegisterIndex, RegisterID base)
    {
        ASSERT(virtualRegisterIndex < FirstConstantRegisterIndex);
        return Address(base, (static_cast<unsigned>(virtualRegisterIndex) * sizeof(Register)) + OBJECT_OFFSETOF(JSValue, u.asBits.payload));
    }

    inline JSInterfaceJIT::Address JSInterfaceJIT::intPayloadFor(int virtualRegisterIndex, RegisterID base)
    {
        return payloadFor(virtualRegisterIndex, base);
    }

    inline JSInterfaceJIT::Address JSInterfaceJIT::intTagFor(int virtualRegisterIndex, RegisterID base)
    {
        return tagFor(virtualRegisterIndex, base);
    }

    inline JSInterfaceJIT::Jump JSInterfaceJIT::emitLoadDouble(unsigned virtualRegisterIndex, FPRegisterID dst, RegisterID scratch)
    {
        ASSERT(static_cast<int>(virtualRegisterIndex) < FirstConstantRegisterIndex);
        loadPtr(tagFor(virtualRegisterIndex), scratch);
        Jump isDouble = branch32(Below, scratch, TrustedImm32(JSValue::LowestTag));
        Jump notInt = branch32(NotEqual, scratch, TrustedImm32(JSValue::Int32Tag));
        loadPtr(payloadFor(virtualRegisterIndex), scratch);
        convertInt32ToDouble(scratch, dst);
        Jump done = jump();
        isDouble.link(this);
        loadDouble(addressFor(virtualRegisterIndex), dst);
        done.link(this);
        return notInt;
    }

#endif

#if USE(JSVALUE64)
    ALWAYS_INLINE JSInterfaceJIT::Jump JSInterfaceJIT::emitJumpIfNotJSCell(RegisterID reg)
    {
        return branchTest64(NonZero, reg, tagMaskRegister);
    }

    ALWAYS_INLINE JSInterfaceJIT::Jump JSInterfaceJIT::emitJumpIfImmediateNumber(RegisterID reg)
    {
        return branchTest64(NonZero, reg, tagTypeNumberRegister);
    }
    ALWAYS_INLINE JSInterfaceJIT::Jump JSInterfaceJIT::emitJumpIfNotImmediateNumber(RegisterID reg)
    {
        return branchTest64(Zero, reg, tagTypeNumberRegister);
    }
    inline JSInterfaceJIT::Jump JSInterfaceJIT::emitLoadJSCell(unsigned virtualRegisterIndex, RegisterID dst)
    {
        load64(addressFor(virtualRegisterIndex), dst);
        return branchTest64(NonZero, dst, tagMaskRegister);
    }
    
    inline JSInterfaceJIT::Jump JSInterfaceJIT::emitLoadInt32(unsigned virtualRegisterIndex, RegisterID dst)
    {
        load64(addressFor(virtualRegisterIndex), dst);
        Jump result = branch64(Below, dst, tagTypeNumberRegister);
        zeroExtend32ToPtr(dst, dst);
        return result;
    }

    inline JSInterfaceJIT::Jump JSInterfaceJIT::emitLoadDouble(unsigned virtualRegisterIndex, FPRegisterID dst, RegisterID scratch)
    {
        load64(addressFor(virtualRegisterIndex), scratch);
        Jump notNumber = emitJumpIfNotImmediateNumber(scratch);
        Jump notInt = branch64(Below, scratch, tagTypeNumberRegister);
        convertInt32ToDouble(scratch, dst);
        Jump done = jump();
        notInt.link(this);
        add64(tagTypeNumberRegister, scratch);
        move64ToDouble(scratch, dst);
        done.link(this);
        return notNumber;
    }

    ALWAYS_INLINE void JSInterfaceJIT::emitFastArithImmToInt(RegisterID)
    {
    }
    
    // operand is int32_t, must have been zero-extended if register is 64-bit.
    ALWAYS_INLINE void JSInterfaceJIT::emitFastArithIntToImmNoCheck(RegisterID src, RegisterID dest)
    {
        if (src != dest)
            move(src, dest);
        or64(tagTypeNumberRegister, dest);
    }
#endif

#if USE(JSVALUE64)
    inline JSInterfaceJIT::Address JSInterfaceJIT::payloadFor(int virtualRegisterIndex, RegisterID base)
    {
        ASSERT(virtualRegisterIndex < FirstConstantRegisterIndex);
        return addressFor(virtualRegisterIndex, base);
    }

    inline JSInterfaceJIT::Address JSInterfaceJIT::intPayloadFor(int virtualRegisterIndex, RegisterID base)
    {
        ASSERT(virtualRegisterIndex < FirstConstantRegisterIndex);
        return Address(base, (static_cast<unsigned>(virtualRegisterIndex) * sizeof(Register)) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
    }
    inline JSInterfaceJIT::Address JSInterfaceJIT::intTagFor(int virtualRegisterIndex, RegisterID base)
    {
        ASSERT(virtualRegisterIndex < FirstConstantRegisterIndex);
        return Address(base, (static_cast<unsigned>(virtualRegisterIndex) * sizeof(Register)) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
    }
#endif

    ALWAYS_INLINE JSInterfaceJIT::Jump JSInterfaceJIT::emitJumpIfNotType(RegisterID baseReg, RegisterID scratchReg, JSType type)
    {
        loadPtr(Address(baseReg, JSCell::structureOffset()), scratchReg);
        return branch8(NotEqual, Address(scratchReg, Structure::typeInfoTypeOffset()), TrustedImm32(type));
    }

    ALWAYS_INLINE void JSInterfaceJIT::emitGetFromCallFrameHeaderPtr(JSStack::CallFrameHeaderEntry entry, RegisterID to, RegisterID from)
    {
        loadPtr(Address(from, entry * sizeof(Register)), to);
    }

    ALWAYS_INLINE void JSInterfaceJIT::emitPutToCallFrameHeader(RegisterID from, JSStack::CallFrameHeaderEntry entry)
    {
#if USE(JSVALUE32_64)
        storePtr(from, payloadFor(entry, callFrameRegister));
#else
        store64(from, addressFor(entry, callFrameRegister));
#endif
    }

    ALWAYS_INLINE void JSInterfaceJIT::emitPutImmediateToCallFrameHeader(void* value, JSStack::CallFrameHeaderEntry entry)
    {
        storePtr(TrustedImmPtr(value), Address(callFrameRegister, entry * sizeof(Register)));
    }

    ALWAYS_INLINE void JSInterfaceJIT::emitPutCellToCallFrameHeader(RegisterID from, JSStack::CallFrameHeaderEntry entry)
    {
#if USE(JSVALUE32_64)
        store32(TrustedImm32(JSValue::CellTag), tagFor(entry, callFrameRegister));
        store32(from, payloadFor(entry, callFrameRegister));
#else
        store64(from, addressFor(entry, callFrameRegister));
#endif
    }

    inline JSInterfaceJIT::Address JSInterfaceJIT::addressFor(int virtualRegisterIndex, RegisterID base)
    {
        ASSERT(virtualRegisterIndex < FirstConstantRegisterIndex);
        return Address(base, (static_cast<unsigned>(virtualRegisterIndex) * sizeof(Register)));
    }

#if CPU(ARM)

    ALWAYS_INLINE void JSInterfaceJIT::preserveReturnAddressAfterCall(RegisterID reg)
    {
        move(linkRegister, reg);
    }
    
    ALWAYS_INLINE void JSInterfaceJIT::restoreReturnAddressBeforeReturn(RegisterID reg)
    {
        move(reg, linkRegister);
    }
    
    ALWAYS_INLINE void JSInterfaceJIT::restoreReturnAddressBeforeReturn(Address address)
    {
        loadPtr(address, linkRegister);
    }
#elif CPU(SH4)

    ALWAYS_INLINE void JSInterfaceJIT::preserveReturnAddressAfterCall(RegisterID reg)
    {
        m_assembler.stspr(reg);
    }
    
    ALWAYS_INLINE void JSInterfaceJIT::restoreReturnAddressBeforeReturn(RegisterID reg)
    {
        m_assembler.ldspr(reg);
    }
    
    ALWAYS_INLINE void JSInterfaceJIT::restoreReturnAddressBeforeReturn(Address address)
    {
        loadPtrLinkReg(address);
    }
    
#elif CPU(MIPS)

    ALWAYS_INLINE void JSInterfaceJIT::preserveReturnAddressAfterCall(RegisterID reg)
    {
        move(returnAddressRegister, reg);
    }
    
    ALWAYS_INLINE void JSInterfaceJIT::restoreReturnAddressBeforeReturn(RegisterID reg)
    {
        move(reg, returnAddressRegister);
    }
    
    ALWAYS_INLINE void JSInterfaceJIT::restoreReturnAddressBeforeReturn(Address address)
    {
        loadPtr(address, returnAddressRegister);
    }
    
#else // CPU(X86) || CPU(X86_64)

    ALWAYS_INLINE void JSInterfaceJIT::preserveReturnAddressAfterCall(RegisterID reg)
    {
        pop(reg);
    }
    
    ALWAYS_INLINE void JSInterfaceJIT::restoreReturnAddressBeforeReturn(RegisterID reg)
    {
        push(reg);
    }
    
    ALWAYS_INLINE void JSInterfaceJIT::restoreReturnAddressBeforeReturn(Address address)
    {
        push(address);
    }
    
#endif

    ALWAYS_INLINE void JSInterfaceJIT::restoreArgumentReference()
    {
        move(stackPointerRegister, firstArgumentRegister);
        poke(callFrameRegister, OBJECT_OFFSETOF(struct JITStackFrame, callFrame) / sizeof(void*));
    }

} // namespace JSC

#endif // ENABLE(JIT)

#endif // JSInterfaceJIT_h
