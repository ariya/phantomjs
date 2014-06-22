/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef DFGGPRInfo_h
#define DFGGPRInfo_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "DFGRegisterBank.h"
#include "MacroAssembler.h"

namespace JSC { namespace DFG {

typedef MacroAssembler::RegisterID GPRReg;
#define InvalidGPRReg ((GPRReg)-1)

#if USE(JSVALUE64)
class JSValueRegs {
public:
    JSValueRegs()
        : m_gpr(InvalidGPRReg)
    {
    }
    
    explicit JSValueRegs(GPRReg gpr)
        : m_gpr(gpr)
    {
    }
    
    bool operator!() const { return m_gpr == InvalidGPRReg; }
    
    GPRReg gpr() const { return m_gpr; }
    
private:
    GPRReg m_gpr;
};

class JSValueSource {
public:
    JSValueSource()
        : m_offset(notAddress())
        , m_base(InvalidGPRReg)
    {
    }
    
    JSValueSource(JSValueRegs regs)
        : m_offset(notAddress())
        , m_base(regs.gpr())
    {
    }
    
    explicit JSValueSource(GPRReg gpr)
        : m_offset(notAddress())
        , m_base(gpr)
    {
    }
    
    JSValueSource(MacroAssembler::Address address)
        : m_offset(address.offset)
        , m_base(address.base)
    {
        ASSERT(m_offset != notAddress());
        ASSERT(m_base != InvalidGPRReg);
    }
    
    static JSValueSource unboxedCell(GPRReg payloadGPR)
    {
        return JSValueSource(payloadGPR);
    }
    
    bool operator!() const { return m_base == InvalidGPRReg; }
    
    bool isAddress() const { return m_offset != notAddress(); }
    
    int32_t offset() const
    {
        ASSERT(isAddress());
        return m_offset;
    }
    
    GPRReg base() const
    {
        ASSERT(isAddress());
        return m_base;
    }
    
    GPRReg gpr() const
    {
        ASSERT(!isAddress());
        return m_base;
    }
    
    MacroAssembler::Address asAddress() const { return MacroAssembler::Address(base(), offset()); }
    
private:
    static inline int32_t notAddress() { return 0x80000000; }     
          
    int32_t m_offset;
    GPRReg m_base;
};
#endif

#if USE(JSVALUE32_64)
class JSValueRegs {
public:
    JSValueRegs()
        : m_tagGPR(static_cast<int8_t>(InvalidGPRReg))
        , m_payloadGPR(static_cast<int8_t>(InvalidGPRReg))
    {
    }
    
    JSValueRegs(GPRReg tagGPR, GPRReg payloadGPR)
        : m_tagGPR(tagGPR)
        , m_payloadGPR(payloadGPR)
    {
        ASSERT((static_cast<GPRReg>(m_tagGPR) == InvalidGPRReg) == (static_cast<GPRReg>(payloadGPR) == InvalidGPRReg));
    }
    
    bool operator!() const { return static_cast<GPRReg>(m_tagGPR) == InvalidGPRReg; }
    
    GPRReg tagGPR() const { return static_cast<GPRReg>(m_tagGPR); }
    GPRReg payloadGPR() const { return static_cast<GPRReg>(m_payloadGPR); }

private:
    int8_t m_tagGPR;
    int8_t m_payloadGPR;
};

class JSValueSource {
public:
    JSValueSource()
        : m_offset(notAddress())
        , m_baseOrTag(static_cast<int8_t>(InvalidGPRReg))
        , m_payload(static_cast<int8_t>(InvalidGPRReg))
        , m_tagType(0)
    {
    }
    
    JSValueSource(JSValueRegs regs)
        : m_offset(notAddress())
        , m_baseOrTag(regs.tagGPR())
        , m_payload(regs.payloadGPR())
        , m_tagType(0)
    {
    }
    
    JSValueSource(GPRReg tagGPR, GPRReg payloadGPR)
        : m_offset(notAddress())
        , m_baseOrTag(static_cast<int8_t>(tagGPR))
        , m_payload(static_cast<int8_t>(payloadGPR))
        , m_tagType(0)
    {
    }
    
    JSValueSource(MacroAssembler::Address address)
        : m_offset(address.offset)
        , m_baseOrTag(static_cast<int8_t>(address.base))
        , m_payload(static_cast<int8_t>(InvalidGPRReg))
        , m_tagType(0)
    {
        ASSERT(m_offset != notAddress());
        ASSERT(static_cast<GPRReg>(m_baseOrTag) != InvalidGPRReg);
    }
    
    static JSValueSource unboxedCell(GPRReg payloadGPR)
    {
        JSValueSource result;
        result.m_offset = notAddress();
        result.m_baseOrTag = static_cast<int8_t>(InvalidGPRReg);
        result.m_payload = static_cast<int8_t>(payloadGPR);
        result.m_tagType = static_cast<int8_t>(JSValue::CellTag);
        return result;
    }
    
    bool operator!() const { return static_cast<GPRReg>(m_baseOrTag) == InvalidGPRReg && static_cast<GPRReg>(m_payload) == InvalidGPRReg; }
    
    bool isAddress() const
    {
        ASSERT(!!*this);
        return m_offset != notAddress();
    }
    
    int32_t offset() const
    {
        ASSERT(isAddress());
        return m_offset;
    }
    
    GPRReg base() const
    {
        ASSERT(isAddress());
        return static_cast<GPRReg>(m_baseOrTag);
    }
    
    GPRReg tagGPR() const
    {
        ASSERT(!isAddress() && static_cast<GPRReg>(m_baseOrTag) != InvalidGPRReg);
        return static_cast<GPRReg>(m_baseOrTag);
    }
    
    GPRReg payloadGPR() const
    {
        ASSERT(!isAddress());
        return static_cast<GPRReg>(m_payload);
    }
    
    bool hasKnownTag() const
    {
        ASSERT(!!*this);
        ASSERT(!isAddress());
        return static_cast<GPRReg>(m_baseOrTag) == InvalidGPRReg;
    }
    
    uint32_t tag() const
    {
        return static_cast<int32_t>(m_tagType);
    }
    
    MacroAssembler::Address asAddress(unsigned additionalOffset = 0) const { return MacroAssembler::Address(base(), offset() + additionalOffset); }
    
private:
    static inline int32_t notAddress() { return 0x80000000; }     
          
    int32_t m_offset;
    int8_t m_baseOrTag;
    int8_t m_payload; 
    int8_t m_tagType; // Contains the low bits of the tag.
};
#endif

#if CPU(X86)
#define NUMBER_OF_ARGUMENT_REGISTERS 0

class GPRInfo {
public:
    typedef GPRReg RegisterType;
    static const unsigned numberOfRegisters = 5;

    // Temporary registers.
    static const GPRReg regT0 = X86Registers::eax;
    static const GPRReg regT1 = X86Registers::edx;
    static const GPRReg regT2 = X86Registers::ecx;
    static const GPRReg regT3 = X86Registers::ebx;
    static const GPRReg regT4 = X86Registers::esi;
    // These registers match the baseline JIT.
    static const GPRReg cachedResultRegister = regT0;
    static const GPRReg cachedResultRegister2 = regT1;
    static const GPRReg callFrameRegister = X86Registers::edi;
    // These constants provide the names for the general purpose argument & return value registers.
    static const GPRReg argumentGPR0 = X86Registers::ecx; // regT2
    static const GPRReg argumentGPR1 = X86Registers::edx; // regT1
    static const GPRReg nonArgGPR0 = X86Registers::eax; // regT0
    static const GPRReg nonArgGPR1 = X86Registers::ebx; // regT3
    static const GPRReg nonArgGPR2 = X86Registers::esi; // regT4
    static const GPRReg returnValueGPR = X86Registers::eax; // regT0
    static const GPRReg returnValueGPR2 = X86Registers::edx; // regT1
    static const GPRReg nonPreservedNonReturnGPR = X86Registers::ecx;

    static GPRReg toRegister(unsigned index)
    {
        ASSERT(index < numberOfRegisters);
        static const GPRReg registerForIndex[numberOfRegisters] = { regT0, regT1, regT2, regT3, regT4 };
        return registerForIndex[index];
    }

    static unsigned toIndex(GPRReg reg)
    {
        ASSERT(reg != InvalidGPRReg);
        ASSERT(static_cast<int>(reg) < 8);
        static const unsigned indexForRegister[8] = { 0, 2, 1, 3, InvalidIndex, InvalidIndex, 4, InvalidIndex };
        unsigned result = indexForRegister[reg];
        ASSERT(result != InvalidIndex);
        return result;
    }

    static const char* debugName(GPRReg reg)
    {
        ASSERT(reg != InvalidGPRReg);
        ASSERT(static_cast<int>(reg) < 8);
        static const char* nameForRegister[8] = {
            "eax", "ecx", "edx", "ebx",
            "esp", "ebp", "esi", "edi",
        };
        return nameForRegister[reg];
    }
private:

    static const unsigned InvalidIndex = 0xffffffff;
};

#endif

#if CPU(X86_64)
#define NUMBER_OF_ARGUMENT_REGISTERS 6

class GPRInfo {
public:
    typedef GPRReg RegisterType;
    static const unsigned numberOfRegisters = 9;

    // These registers match the baseline JIT.
    static const GPRReg cachedResultRegister = X86Registers::eax;
    static const GPRReg callFrameRegister = X86Registers::r13;
    static const GPRReg tagTypeNumberRegister = X86Registers::r14;
    static const GPRReg tagMaskRegister = X86Registers::r15;
    // Temporary registers.
    static const GPRReg regT0 = X86Registers::eax;
    static const GPRReg regT1 = X86Registers::edx;
    static const GPRReg regT2 = X86Registers::ecx;
    static const GPRReg regT3 = X86Registers::ebx;
    static const GPRReg regT4 = X86Registers::edi;
    static const GPRReg regT5 = X86Registers::esi;
    static const GPRReg regT6 = X86Registers::r8;
    static const GPRReg regT7 = X86Registers::r9;
    static const GPRReg regT8 = X86Registers::r10;
    // These constants provide the names for the general purpose argument & return value registers.
    static const GPRReg argumentGPR0 = X86Registers::edi; // regT4
    static const GPRReg argumentGPR1 = X86Registers::esi; // regT5
    static const GPRReg argumentGPR2 = X86Registers::edx; // regT1
    static const GPRReg argumentGPR3 = X86Registers::ecx; // regT2
    static const GPRReg argumentGPR4 = X86Registers::r8;  // regT6
    static const GPRReg argumentGPR5 = X86Registers::r9;  // regT7
    static const GPRReg nonArgGPR0 = X86Registers::eax; // regT0
    static const GPRReg nonArgGPR1 = X86Registers::ebx; // regT3
    static const GPRReg nonArgGPR2 = X86Registers::r10; // regT8
    static const GPRReg returnValueGPR = X86Registers::eax; // regT0
    static const GPRReg returnValueGPR2 = X86Registers::edx; // regT1
    static const GPRReg nonPreservedNonReturnGPR = X86Registers::esi;

    static GPRReg toRegister(unsigned index)
    {
        ASSERT(index < numberOfRegisters);
        static const GPRReg registerForIndex[numberOfRegisters] = { regT0, regT1, regT2, regT3, regT4, regT5, regT6, regT7, regT8 };
        return registerForIndex[index];
    }

    static unsigned toIndex(GPRReg reg)
    {
        ASSERT(reg != InvalidGPRReg);
        ASSERT(static_cast<int>(reg) < 16);
        static const unsigned indexForRegister[16] = { 0, 2, 1, 3, InvalidIndex, InvalidIndex, 5, 4, 6, 7, 8, InvalidIndex, InvalidIndex, InvalidIndex, InvalidIndex, InvalidIndex };
        unsigned result = indexForRegister[reg];
        ASSERT(result != InvalidIndex);
        return result;
    }

    static const char* debugName(GPRReg reg)
    {
        ASSERT(reg != InvalidGPRReg);
        ASSERT(static_cast<int>(reg) < 16);
        static const char* nameForRegister[16] = {
            "rax", "rcx", "rdx", "rbx",
            "rsp", "rbp", "rsi", "rdi",
            "r8", "r9", "r10", "r11",
            "r12", "r13", "r14", "r15"
        };
        return nameForRegister[reg];
    }
private:

    static const unsigned InvalidIndex = 0xffffffff;
};

#endif

#if CPU(ARM)
#define NUMBER_OF_ARGUMENT_REGISTERS 4

class GPRInfo {
public:
    typedef GPRReg RegisterType;
    static const unsigned numberOfRegisters = 8;

    // Temporary registers.
    static const GPRReg regT0 = ARMRegisters::r0;
    static const GPRReg regT1 = ARMRegisters::r1;
    static const GPRReg regT2 = ARMRegisters::r2;
    static const GPRReg regT3 = ARMRegisters::r4;
    static const GPRReg regT4 = ARMRegisters::r8;
    static const GPRReg regT5 = ARMRegisters::r9;
    static const GPRReg regT6 = ARMRegisters::r10;
    static const GPRReg regT7 = ARMRegisters::r11;
    // These registers match the baseline JIT.
    static const GPRReg cachedResultRegister = regT0;
    static const GPRReg cachedResultRegister2 = regT1;
    static const GPRReg callFrameRegister = ARMRegisters::r5;
    // These constants provide the names for the general purpose argument & return value registers.
    static const GPRReg argumentGPR0 = ARMRegisters::r0; // regT0
    static const GPRReg argumentGPR1 = ARMRegisters::r1; // regT1
    static const GPRReg argumentGPR2 = ARMRegisters::r2; // regT2
    // FIXME: r3 is currently used be the MacroAssembler as a temporary - it seems
    // This could threoretically be a problem if this is used in code generation
    // between the arguments being set up, and the call being made. That said,
    // any change introducing a problem here is likely to be immediately apparent!
    static const GPRReg argumentGPR3 = ARMRegisters::r3; // FIXME!
    static const GPRReg nonArgGPR0 = ARMRegisters::r4; // regT3
    static const GPRReg nonArgGPR1 = ARMRegisters::r8; // regT4
    static const GPRReg nonArgGPR2 = ARMRegisters::r9; // regT5
    static const GPRReg returnValueGPR = ARMRegisters::r0; // regT0
    static const GPRReg returnValueGPR2 = ARMRegisters::r1; // regT1
    static const GPRReg nonPreservedNonReturnGPR = ARMRegisters::r2;

    static GPRReg toRegister(unsigned index)
    {
        ASSERT(index < numberOfRegisters);
        static const GPRReg registerForIndex[numberOfRegisters] = { regT0, regT1, regT2, regT3, regT4, regT5, regT6, regT7 };
        return registerForIndex[index];
    }

    static unsigned toIndex(GPRReg reg)
    {
        ASSERT(static_cast<unsigned>(reg) != InvalidGPRReg);
        ASSERT(static_cast<unsigned>(reg) < 16);
        static const unsigned indexForRegister[16] = { 0, 1, 2, InvalidIndex, 3, InvalidIndex, InvalidIndex, InvalidIndex, 4, 5, 6, 7, InvalidIndex, InvalidIndex, InvalidIndex, InvalidIndex };
        unsigned result = indexForRegister[reg];
        ASSERT(result != InvalidIndex);
        return result;
    }

    static const char* debugName(GPRReg reg)
    {
        ASSERT(static_cast<unsigned>(reg) != InvalidGPRReg);
        ASSERT(static_cast<unsigned>(reg) < 16);
        static const char* nameForRegister[16] = {
            "r0", "r1", "r2", "r3",
            "r4", "r5", "r6", "r7",
            "r8", "r9", "r10", "r11",
            "r12", "r13", "r14", "r15"
        };
        return nameForRegister[reg];
    }
private:

    static const unsigned InvalidIndex = 0xffffffff;
};

#endif

#if CPU(MIPS)
#define NUMBER_OF_ARGUMENT_REGISTERS 4

class GPRInfo {
public:
    typedef GPRReg RegisterType;
    static const unsigned numberOfRegisters = 6;

    // Temporary registers.
    static const GPRReg regT0 = MIPSRegisters::v0;
    static const GPRReg regT1 = MIPSRegisters::v1;
    static const GPRReg regT2 = MIPSRegisters::t4;
    static const GPRReg regT3 = MIPSRegisters::t5;
    static const GPRReg regT4 = MIPSRegisters::t6;
    static const GPRReg regT5 = MIPSRegisters::t7;
    // These registers match the baseline JIT.
    static const GPRReg cachedResultRegister = regT0;
    static const GPRReg cachedResultRegister2 = regT1;
    static const GPRReg callFrameRegister = MIPSRegisters::s0;
    // These constants provide the names for the general purpose argument & return value registers.
    static const GPRReg argumentGPR0 = MIPSRegisters::a0;
    static const GPRReg argumentGPR1 = MIPSRegisters::a1;
    static const GPRReg argumentGPR2 = MIPSRegisters::a2;
    static const GPRReg argumentGPR3 = MIPSRegisters::a3;
    static const GPRReg nonArgGPR0 = regT2;
    static const GPRReg nonArgGPR1 = regT3;
    static const GPRReg nonArgGPR2 = regT4;
    static const GPRReg returnValueGPR = regT0;
    static const GPRReg returnValueGPR2 = regT1;
    static const GPRReg nonPreservedNonReturnGPR = regT5;

    static GPRReg toRegister(unsigned index)
    {
        ASSERT(index < numberOfRegisters);
        static const GPRReg registerForIndex[numberOfRegisters] = { regT0, regT1, regT2, regT3, regT4, regT5 };
        return registerForIndex[index];
    }

    static unsigned toIndex(GPRReg reg)
    {
        ASSERT(reg != InvalidGPRReg);
        ASSERT(reg < 16);
        static const unsigned indexForRegister[16] = { InvalidIndex, InvalidIndex, 0, 1, InvalidIndex, InvalidIndex, InvalidIndex, InvalidIndex, InvalidIndex, InvalidIndex, InvalidIndex, InvalidIndex, 2, 3, 4, 5 };
        unsigned result = indexForRegister[reg];
        ASSERT(result != InvalidIndex);
        return result;
    }

    static const char* debugName(GPRReg reg)
    {
        ASSERT(reg != InvalidGPRReg);
        ASSERT(reg < 16);
        static const char* nameForRegister[16] = {
            "zero", "at", "v0", "v1",
            "a0", "a1", "a2", "a3",
            "t0", "t1", "t2", "t3",
            "t4", "t5", "t6", "t7"
        };
        return nameForRegister[reg];
    }
private:

    static const unsigned InvalidIndex = 0xffffffff;
};

#endif

#if CPU(SH4)
#define NUMBER_OF_ARGUMENT_REGISTERS 4

class GPRInfo {
public:
    typedef GPRReg RegisterType;
    static const unsigned numberOfRegisters = 10;

    // Temporary registers.
    static const GPRReg regT0 = SH4Registers::r0;
    static const GPRReg regT1 = SH4Registers::r1;
    static const GPRReg regT2 = SH4Registers::r2;
    static const GPRReg regT3 = SH4Registers::r10;
    static const GPRReg regT4 = SH4Registers::r4;
    static const GPRReg regT5 = SH4Registers::r5;
    static const GPRReg regT6 = SH4Registers::r6;
    static const GPRReg regT7 = SH4Registers::r7;
    static const GPRReg regT8 = SH4Registers::r8;
    static const GPRReg regT9 = SH4Registers::r9;
    // These registers match the baseline JIT.
    static const GPRReg cachedResultRegister = regT0;
    static const GPRReg cachedResultRegister2 = regT1;
    static const GPRReg callFrameRegister = SH4Registers::fp;
    // These constants provide the names for the general purpose argument & return value registers.
    static const GPRReg argumentGPR0 = regT4;
    static const GPRReg argumentGPR1 = regT5;
    static const GPRReg argumentGPR2 = regT6;
    static const GPRReg argumentGPR3 = regT7;
    static const GPRReg nonArgGPR0 = regT3;
    static const GPRReg nonArgGPR1 = regT8;
    static const GPRReg nonArgGPR2 = regT9;
    static const GPRReg returnValueGPR = regT0;
    static const GPRReg returnValueGPR2 = regT1;
    static const GPRReg nonPreservedNonReturnGPR = regT2;

    static GPRReg toRegister(unsigned index)
    {
        ASSERT(index < numberOfRegisters);
        static const GPRReg registerForIndex[numberOfRegisters] = { regT0, regT1, regT2, regT3, regT4, regT5, regT6, regT7, regT8, regT9 };
        return registerForIndex[index];
    }

    static unsigned toIndex(GPRReg reg)
    {
        ASSERT(reg != InvalidGPRReg);
        ASSERT(reg < 14);
        static const unsigned indexForRegister[14] = { 0, 1, 2, InvalidIndex, 4, 5, 6, 7, 8, 9, 3, InvalidIndex, InvalidIndex, InvalidIndex };
        unsigned result = indexForRegister[reg];
        ASSERT(result != InvalidIndex);
        return result;
    }

    static const char* debugName(GPRReg reg)
    {
        ASSERT(reg != InvalidGPRReg);
        ASSERT(reg < 16);
        static const char* nameForRegister[16] = {
            "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
            "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
        };
        return nameForRegister[reg];
    }

private:
    static const unsigned InvalidIndex = 0xffffffff;
};

#endif

typedef RegisterBank<GPRInfo>::iterator gpr_iterator;

} } // namespace JSC::DFG

#endif
#endif
