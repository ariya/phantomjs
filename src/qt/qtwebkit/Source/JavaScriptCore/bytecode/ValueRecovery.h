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

#ifndef ValueRecovery_h
#define ValueRecovery_h

#include "DataFormat.h"
#include "JSCJSValue.h"
#include "MacroAssembler.h"
#include "VirtualRegister.h"
#include <stdio.h>
#include <wtf/Platform.h>

namespace JSC {

// Describes how to recover a given bytecode virtual register at a given
// code point.
enum ValueRecoveryTechnique {
    // It's already in the stack at the right location.
    AlreadyInJSStack,
    // It's already in the stack but unboxed.
    AlreadyInJSStackAsUnboxedInt32,
    AlreadyInJSStackAsUnboxedCell,
    AlreadyInJSStackAsUnboxedBoolean,
    AlreadyInJSStackAsUnboxedDouble,
    // It's in a register.
    InGPR,
    UnboxedInt32InGPR,
    UnboxedBooleanInGPR,
#if USE(JSVALUE32_64)
    InPair,
#endif
    InFPR,
    UInt32InGPR,
    // It's in the stack, but at a different location.
    DisplacedInJSStack,
    // It's in the stack, at a different location, and it's unboxed.
    Int32DisplacedInJSStack,
    DoubleDisplacedInJSStack,
    CellDisplacedInJSStack,
    BooleanDisplacedInJSStack,
    // It's an Arguments object.
    ArgumentsThatWereNotCreated,
    // It's a constant.
    Constant,
    // Don't know how to recover it.
    DontKnow
};

class ValueRecovery {
public:
    ValueRecovery()
        : m_technique(DontKnow)
    {
    }
    
    bool isSet() const { return m_technique != DontKnow; }
    bool operator!() const { return !isSet(); }
    
    static ValueRecovery alreadyInJSStack()
    {
        ValueRecovery result;
        result.m_technique = AlreadyInJSStack;
        return result;
    }
    
    static ValueRecovery alreadyInJSStackAsUnboxedInt32()
    {
        ValueRecovery result;
        result.m_technique = AlreadyInJSStackAsUnboxedInt32;
        return result;
    }
    
    static ValueRecovery alreadyInJSStackAsUnboxedCell()
    {
        ValueRecovery result;
        result.m_technique = AlreadyInJSStackAsUnboxedCell;
        return result;
    }
    
    static ValueRecovery alreadyInJSStackAsUnboxedBoolean()
    {
        ValueRecovery result;
        result.m_technique = AlreadyInJSStackAsUnboxedBoolean;
        return result;
    }
    
    static ValueRecovery alreadyInJSStackAsUnboxedDouble()
    {
        ValueRecovery result;
        result.m_technique = AlreadyInJSStackAsUnboxedDouble;
        return result;
    }
    
    static ValueRecovery inGPR(MacroAssembler::RegisterID gpr, DataFormat dataFormat)
    {
        ASSERT(dataFormat != DataFormatNone);
#if USE(JSVALUE32_64)
        ASSERT(dataFormat == DataFormatInteger || dataFormat == DataFormatCell || dataFormat == DataFormatBoolean);
#endif
        ValueRecovery result;
        if (dataFormat == DataFormatInteger)
            result.m_technique = UnboxedInt32InGPR;
        else if (dataFormat == DataFormatBoolean)
            result.m_technique = UnboxedBooleanInGPR;
        else
            result.m_technique = InGPR;
        result.m_source.gpr = gpr;
        return result;
    }
    
    static ValueRecovery uint32InGPR(MacroAssembler::RegisterID gpr)
    {
        ValueRecovery result;
        result.m_technique = UInt32InGPR;
        result.m_source.gpr = gpr;
        return result;
    }
    
#if USE(JSVALUE32_64)
    static ValueRecovery inPair(MacroAssembler::RegisterID tagGPR, MacroAssembler::RegisterID payloadGPR)
    {
        ValueRecovery result;
        result.m_technique = InPair;
        result.m_source.pair.tagGPR = tagGPR;
        result.m_source.pair.payloadGPR = payloadGPR;
        return result;
    }
#endif

    static ValueRecovery inFPR(MacroAssembler::FPRegisterID fpr)
    {
        ValueRecovery result;
        result.m_technique = InFPR;
        result.m_source.fpr = fpr;
        return result;
    }
    
    static ValueRecovery displacedInJSStack(VirtualRegister virtualReg, DataFormat dataFormat)
    {
        ValueRecovery result;
        switch (dataFormat) {
        case DataFormatInteger:
            result.m_technique = Int32DisplacedInJSStack;
            break;
            
        case DataFormatDouble:
            result.m_technique = DoubleDisplacedInJSStack;
            break;

        case DataFormatCell:
            result.m_technique = CellDisplacedInJSStack;
            break;
            
        case DataFormatBoolean:
            result.m_technique = BooleanDisplacedInJSStack;
            break;
            
        default:
            ASSERT(dataFormat != DataFormatNone && dataFormat != DataFormatStorage);
            result.m_technique = DisplacedInJSStack;
            break;
        }
        result.m_source.virtualReg = virtualReg;
        return result;
    }
    
    static ValueRecovery constant(JSValue value)
    {
        ValueRecovery result;
        result.m_technique = Constant;
        result.m_source.constant = JSValue::encode(value);
        return result;
    }
    
    static ValueRecovery argumentsThatWereNotCreated()
    {
        ValueRecovery result;
        result.m_technique = ArgumentsThatWereNotCreated;
        return result;
    }
    
    ValueRecoveryTechnique technique() const { return m_technique; }
    
    bool isConstant() const { return m_technique == Constant; }
    
    bool isInRegisters() const
    {
        switch (m_technique) {
        case InGPR:
        case UnboxedInt32InGPR:
        case UnboxedBooleanInGPR:
#if USE(JSVALUE32_64)
        case InPair:
#endif
        case InFPR:
            return true;
        default:
            return false;
        }
    }
    
    bool isAlreadyInJSStack() const
    {
        switch (technique()) {
        case AlreadyInJSStack:
        case AlreadyInJSStackAsUnboxedInt32:
        case AlreadyInJSStackAsUnboxedCell:
        case AlreadyInJSStackAsUnboxedBoolean:
        case AlreadyInJSStackAsUnboxedDouble:
            return true;
        default:
            return false;
        }
    }
    
    MacroAssembler::RegisterID gpr() const
    {
        ASSERT(m_technique == InGPR || m_technique == UnboxedInt32InGPR || m_technique == UnboxedBooleanInGPR || m_technique == UInt32InGPR);
        return m_source.gpr;
    }
    
#if USE(JSVALUE32_64)
    MacroAssembler::RegisterID tagGPR() const
    {
        ASSERT(m_technique == InPair);
        return m_source.pair.tagGPR;
    }
    
    MacroAssembler::RegisterID payloadGPR() const
    {
        ASSERT(m_technique == InPair);
        return m_source.pair.payloadGPR;
    }
#endif
    
    MacroAssembler::FPRegisterID fpr() const
    {
        ASSERT(m_technique == InFPR);
        return m_source.fpr;
    }
    
    VirtualRegister virtualRegister() const
    {
        ASSERT(m_technique == DisplacedInJSStack || m_technique == Int32DisplacedInJSStack || m_technique == DoubleDisplacedInJSStack || m_technique == CellDisplacedInJSStack || m_technique == BooleanDisplacedInJSStack);
        return m_source.virtualReg;
    }
    
    JSValue constant() const
    {
        ASSERT(m_technique == Constant);
        return JSValue::decode(m_source.constant);
    }
    
    void dump(PrintStream& out) const
    {
        switch (technique()) {
        case AlreadyInJSStack:
            out.printf("-");
            break;
        case AlreadyInJSStackAsUnboxedInt32:
            out.printf("(int32)");
            break;
        case AlreadyInJSStackAsUnboxedCell:
            out.printf("(cell)");
            break;
        case AlreadyInJSStackAsUnboxedBoolean:
            out.printf("(bool)");
            break;
        case AlreadyInJSStackAsUnboxedDouble:
            out.printf("(double)");
            break;
        case InGPR:
            out.printf("%%r%d", gpr());
            break;
        case UnboxedInt32InGPR:
            out.printf("int32(%%r%d)", gpr());
            break;
        case UnboxedBooleanInGPR:
            out.printf("bool(%%r%d)", gpr());
            break;
        case UInt32InGPR:
            out.printf("uint32(%%r%d)", gpr());
            break;
        case InFPR:
            out.printf("%%fr%d", fpr());
            break;
#if USE(JSVALUE32_64)
        case InPair:
            out.printf("pair(%%r%d, %%r%d)", tagGPR(), payloadGPR());
            break;
#endif
        case DisplacedInJSStack:
            out.printf("*%d", virtualRegister());
            break;
        case Int32DisplacedInJSStack:
            out.printf("*int32(%d)", virtualRegister());
            break;
        case DoubleDisplacedInJSStack:
            out.printf("*double(%d)", virtualRegister());
            break;
        case CellDisplacedInJSStack:
            out.printf("*cell(%d)", virtualRegister());
            break;
        case BooleanDisplacedInJSStack:
            out.printf("*bool(%d)", virtualRegister());
            break;
        case ArgumentsThatWereNotCreated:
            out.printf("arguments");
            break;
        case Constant:
            out.print("[", constant(), "]");
            break;
        case DontKnow:
            out.printf("!");
            break;
        default:
            out.printf("?%d", technique());
            break;
        }
    }
    
private:
    ValueRecoveryTechnique m_technique;
    union {
        MacroAssembler::RegisterID gpr;
        MacroAssembler::FPRegisterID fpr;
#if USE(JSVALUE32_64)
        struct {
            MacroAssembler::RegisterID tagGPR;
            MacroAssembler::RegisterID payloadGPR;
        } pair;
#endif
        VirtualRegister virtualReg;
        EncodedJSValue constant;
    } m_source;
};

} // namespace JSC

#endif // ValueRecovery_h
