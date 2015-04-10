/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef DFGRegisterSet_h
#define DFGRegisterSet_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "DFGFPRInfo.h"
#include "DFGGPRInfo.h"
#include <wtf/Bitmap.h>

namespace JSC { namespace DFG {

static const unsigned totalNumberOfRegisters =
    GPRInfo::numberOfRegisters + FPRInfo::numberOfRegisters;

static const unsigned numberOfBytesInRegisterSet =
    (totalNumberOfRegisters + 7) >> 3;

typedef uint8_t RegisterSetPOD[numberOfBytesInRegisterSet];

class RegisterSet {
public:
    RegisterSet()
    {
        for (unsigned i = numberOfBytesInRegisterSet; i--;)
            m_set[i] = 0;
    }
    
    RegisterSet(const RegisterSetPOD& other)
    {
        for (unsigned i = numberOfBytesInRegisterSet; i--;)
            m_set[i] = other[i];
    }
    
    const RegisterSetPOD& asPOD() const { return m_set; }
    
    void copyInfo(RegisterSetPOD& other) const
    {
        for (unsigned i = numberOfBytesInRegisterSet; i--;)
            other[i] = m_set[i];
    }
    
    void set(GPRReg reg)
    {
        setBit(GPRInfo::toIndex(reg));
    }
    
    void setGPRByIndex(unsigned index)
    {
        ASSERT(index < GPRInfo::numberOfRegisters);
        setBit(index);
    }
    
    void clear(GPRReg reg)
    {
        clearBit(GPRInfo::toIndex(reg));
    }
    
    bool get(GPRReg reg) const
    {
        return getBit(GPRInfo::toIndex(reg));
    }
    
    bool getGPRByIndex(unsigned index) const
    {
        ASSERT(index < GPRInfo::numberOfRegisters);
        return getBit(index);
    }
    
    // Return the index'th free GPR.
    GPRReg getFreeGPR(unsigned index = 0) const
    {
        for (unsigned i = GPRInfo::numberOfRegisters; i--;) {
            if (!getGPRByIndex(i) && !index--)
                return GPRInfo::toRegister(i);
        }
        return InvalidGPRReg;
    }
    
    void set(FPRReg reg)
    {
        setBit(GPRInfo::numberOfRegisters + FPRInfo::toIndex(reg));
    }
    
    void setFPRByIndex(unsigned index)
    {
        ASSERT(index < FPRInfo::numberOfRegisters);
        setBit(GPRInfo::numberOfRegisters + index);
    }
    
    void clear(FPRReg reg)
    {
        clearBit(GPRInfo::numberOfRegisters + FPRInfo::toIndex(reg));
    }
    
    bool get(FPRReg reg) const
    {
        return getBit(GPRInfo::numberOfRegisters + FPRInfo::toIndex(reg));
    }
    
    bool getFPRByIndex(unsigned index) const
    {
        ASSERT(index < FPRInfo::numberOfRegisters);
        return getBit(GPRInfo::numberOfRegisters + index);
    }
    
    template<typename BankInfo>
    void setByIndex(unsigned index)
    {
        set(BankInfo::toRegister(index));
    }
    
    template<typename BankInfo>
    bool getByIndex(unsigned index)
    {
        return get(BankInfo::toRegister(index));
    }
    
    unsigned numberOfSetGPRs() const
    {
        unsigned result = 0;
        for (unsigned i = GPRInfo::numberOfRegisters; i--;) {
            if (!getBit(i))
                continue;
            result++;
        }
        return result;
    }
    
    unsigned numberOfSetFPRs() const
    {
        unsigned result = 0;
        for (unsigned i = FPRInfo::numberOfRegisters; i--;) {
            if (!getBit(GPRInfo::numberOfRegisters + i))
                continue;
            result++;
        }
        return result;
    }
    
    unsigned numberOfSetRegisters() const
    {
        unsigned result = 0;
        for (unsigned i = totalNumberOfRegisters; i--;) {
            if (!getBit(i))
                continue;
            result++;
        }
        return result;
    }
    
private:
    void setBit(unsigned i)
    {
        ASSERT(i < totalNumberOfRegisters);
        m_set[i >> 3] |= (1 << (i & 7));
    }
    
    void clearBit(unsigned i)
    {
        ASSERT(i < totalNumberOfRegisters);
        m_set[i >> 3] &= ~(1 << (i & 7));
    }
    
    bool getBit(unsigned i) const
    {
        ASSERT(i < totalNumberOfRegisters);
        return !!(m_set[i >> 3] & (1 << (i & 7)));
    }
    
    RegisterSetPOD m_set;
};

} } // namespace JSC::DFG

#else // ENABLE(DFG_JIT) -> so if DFG is disabled

namespace JSC { namespace DFG {

// Define RegisterSetPOD to something that is a POD, but is otherwise useless,
// to make it easier to refer to this type in code that may be compiled when
// the DFG is disabled.

struct RegisterSetPOD { };

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGRegisterSet_h

