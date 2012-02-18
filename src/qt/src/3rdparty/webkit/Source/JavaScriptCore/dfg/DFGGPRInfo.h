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

#if ENABLE(DFG_JIT)

#include <assembler/MacroAssembler.h>
#include <dfg/DFGRegisterBank.h>

namespace JSC { namespace DFG {

typedef MacroAssembler::RegisterID GPRReg;
#define InvalidGPRReg ((GPRReg)-1)

class GPRInfo {
public:
    typedef GPRReg RegisterType;
    static const unsigned numberOfRegisters = 9;

    // These registers match the old JIT.
    static const GPRReg timeoutCheckRegister = X86Registers::r12;
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
    static const GPRReg returnValueGPR = X86Registers::eax; // regT0
    static const GPRReg returnValueGPR2 = X86Registers::edx; // regT1

    static GPRReg toRegister(unsigned index)
    {
        ASSERT(index < numberOfRegisters);
        static const GPRReg registerForIndex[numberOfRegisters] = { regT0, regT1, regT2, regT3, regT4, regT5, regT6, regT7, regT8 };
        return registerForIndex[index];
    }

    static unsigned toIndex(GPRReg reg)
    {
        ASSERT(reg != InvalidGPRReg);
        ASSERT(reg < 16);
        static const unsigned indexForRegister[16] = { 0, 2, 1, 3, InvalidIndex, InvalidIndex, 5, 4, 6, 7, 8, InvalidIndex, InvalidIndex, InvalidIndex, InvalidIndex, InvalidIndex };
        unsigned result = indexForRegister[reg];
        ASSERT(result != InvalidIndex);
        return result;
    }

#ifndef NDEBUG
    static const char* debugName(GPRReg reg)
    {
        ASSERT(reg != InvalidGPRReg);
        ASSERT(reg < 16);
        static const char* nameForRegister[16] = {
            "rax", "rcx", "rdx", "rbx",
            "rsp", "rbp", "rsi", "rdi",
            "r8", "r9", "r10", "r11",
            "r12", "r13", "r14", "r15"
        };
        return nameForRegister[reg];
    }
#endif
private:

    static const unsigned InvalidIndex = 0xffffffff;
};

typedef RegisterBank<GPRInfo>::iterator gpr_iterator;

} } // namespace JSC::DFG

#endif
#endif
