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

#ifndef DFGVariableEvent_h
#define DFGVariableEvent_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "DFGCommon.h"
#include "DFGMinifiedID.h"
#include "DataFormat.h"
#include "MacroAssembler.h"
#include <stdio.h>

namespace JSC { namespace DFG {

enum VariableEventKind {
    // Marks the beginning of a checkpoint. If you interpret the variable
    // events starting at a Reset point then you'll get everything you need.
    Reset,
    
    // Node births. Points in the code where a node becomes relevant for OSR.
    // It may be the point where it is actually born (i.e. assigned) or it may
    // be a later point, if it's only later in the sequence of instructions
    // that we start to care about this node.
    BirthToFill,
    BirthToSpill,
    
    // Events related to how a node is represented.
    Fill,
    Spill,
    
    // Death of a node - after this we no longer care about this node.
    Death,
    
    // A MovHintEvent means that a node is being associated with a bytecode operand,
    // but that it has not been stored into that operand.
    MovHintEvent,
    
    // A SetLocalEvent means that a node's value has actually been stored into the
    // bytecode operand that it's associated with.
    SetLocalEvent,
    
    // Used to indicate an uninitialized VariableEvent. Don't use for other
    // purposes.
    InvalidEventKind
};

union VariableRepresentation {
    MacroAssembler::RegisterID gpr;
    MacroAssembler::FPRegisterID fpr;
#if USE(JSVALUE32_64)
    struct {
        MacroAssembler::RegisterID tagGPR;
        MacroAssembler::RegisterID payloadGPR;
    } pair;
#endif
    int32_t virtualReg;
};

class VariableEvent {
public:
    VariableEvent()
        : m_kind(InvalidEventKind)
    {
    }
    
    static VariableEvent reset()
    {
        VariableEvent event;
        event.m_kind = Reset;
        return event;
    }
    
    static VariableEvent fillGPR(VariableEventKind kind, MinifiedID id, MacroAssembler::RegisterID gpr, DataFormat dataFormat)
    {
        ASSERT(kind == BirthToFill || kind == Fill);
        ASSERT(dataFormat != DataFormatDouble);
#if USE(JSVALUE32_64)
        ASSERT(!(dataFormat & DataFormatJS));
#endif
        VariableEvent event;
        event.m_id = id;
        event.u.gpr = gpr;
        event.m_kind = kind;
        event.m_dataFormat = dataFormat;
        return event;
    }
    
#if USE(JSVALUE32_64)
    static VariableEvent fillPair(VariableEventKind kind, MinifiedID id, MacroAssembler::RegisterID tagGPR, MacroAssembler::RegisterID payloadGPR)
    {
        ASSERT(kind == BirthToFill || kind == Fill);
        VariableEvent event;
        event.m_id = id;
        event.u.pair.tagGPR = tagGPR;
        event.u.pair.payloadGPR = payloadGPR;
        event.m_kind = kind;
        event.m_dataFormat = DataFormatJS;
        return event;
    }
#endif // USE(JSVALUE32_64)
    
    static VariableEvent fillFPR(VariableEventKind kind, MinifiedID id, MacroAssembler::FPRegisterID fpr)
    {
        ASSERT(kind == BirthToFill || kind == Fill);
        VariableEvent event;
        event.m_id = id;
        event.u.fpr = fpr;
        event.m_kind = kind;
        event.m_dataFormat = DataFormatDouble;
        return event;
    }
    
    static VariableEvent spill(VariableEventKind kind, MinifiedID id, VirtualRegister virtualRegister, DataFormat format)
    {
        ASSERT(kind == BirthToSpill || kind == Spill);
        VariableEvent event;
        event.m_id = id;
        event.u.virtualReg = virtualRegister;
        event.m_kind = kind;
        event.m_dataFormat = format;
        return event;
    }
    
    static VariableEvent death(MinifiedID id)
    {
        VariableEvent event;
        event.m_id = id;
        event.m_kind = Death;
        return event;
    }
    
    static VariableEvent setLocal(int operand, DataFormat format)
    {
        VariableEvent event;
        event.u.virtualReg = operand;
        event.m_kind = SetLocalEvent;
        event.m_dataFormat = format;
        return event;
    }
    
    static VariableEvent movHint(MinifiedID id, int operand)
    {
        VariableEvent event;
        event.m_id = id;
        event.u.virtualReg = operand;
        event.m_kind = MovHintEvent;
        return event;
    }
    
    VariableEventKind kind() const
    {
        return static_cast<VariableEventKind>(m_kind);
    }
    
    MinifiedID id() const
    {
        ASSERT(m_kind == BirthToFill || m_kind == Fill
               || m_kind == BirthToSpill || m_kind == Spill
               || m_kind == Death || m_kind == MovHintEvent);
        return m_id;
    }
    
    DataFormat dataFormat() const
    {
        ASSERT(m_kind == BirthToFill || m_kind == Fill
               || m_kind == BirthToSpill || m_kind == Spill
               || m_kind == SetLocalEvent);
        return static_cast<DataFormat>(m_dataFormat);
    }
    
    MacroAssembler::RegisterID gpr() const
    {
        ASSERT(m_kind == BirthToFill || m_kind == Fill);
        ASSERT(m_dataFormat);
        ASSERT(m_dataFormat != DataFormatDouble);
#if USE(JSVALUE32_64)
        ASSERT(!(m_dataFormat & DataFormatJS));
#endif
        return u.gpr;
    }
    
#if USE(JSVALUE32_64)
    MacroAssembler::RegisterID tagGPR() const
    {
        ASSERT(m_kind == BirthToFill || m_kind == Fill);
        ASSERT(m_dataFormat & DataFormatJS);
        return u.pair.tagGPR;
    }
    MacroAssembler::RegisterID payloadGPR() const
    {
        ASSERT(m_kind == BirthToFill || m_kind == Fill);
        ASSERT(m_dataFormat & DataFormatJS);
        return u.pair.payloadGPR;
    }
#endif // USE(JSVALUE32_64)
    
    MacroAssembler::FPRegisterID fpr() const
    {
        ASSERT(m_kind == BirthToFill || m_kind == Fill);
        ASSERT(m_dataFormat == DataFormatDouble);
        return u.fpr;
    }
    
    VirtualRegister virtualRegister() const
    {
        ASSERT(m_kind == BirthToSpill || m_kind == Spill);
        return static_cast<VirtualRegister>(u.virtualReg);
    }
    
    int operand() const
    {
        ASSERT(m_kind == SetLocalEvent || m_kind == MovHintEvent);
        return u.virtualReg;
    }
    
    const VariableRepresentation& variableRepresentation() const { return u; }
    
    void dump(PrintStream&) const;
    
private:
    void dumpFillInfo(const char* name, PrintStream&) const;
    void dumpSpillInfo(const char* name, PrintStream&) const;
    
    MinifiedID m_id;
    
    // For BirthToFill, Fill:
    //   - The GPR or FPR, or a GPR pair.
    // For BirthToSpill, Spill:
    //   - The virtual register.
    // For MovHintEvent, SetLocalEvent:
    //   - The bytecode operand.
    // For Death:
    //   - Unused.
    VariableRepresentation u;
    
    int8_t m_kind;
    int8_t m_dataFormat;
};

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGVariableEvent_h

