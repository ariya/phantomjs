/*
 * Copyright (C) 2011, 2013 Apple Inc. All rights reserved.
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

#ifndef DFGGenerationInfo_h
#define DFGGenerationInfo_h

#if ENABLE(DFG_JIT)

#include "DFGJITCompiler.h"
#include "DFGMinifiedID.h"
#include "DFGVariableEvent.h"
#include "DFGVariableEventStream.h"
#include "DataFormat.h"

namespace JSC { namespace DFG {

// === GenerationInfo ===
//
// This class is used to track the current status of a live values during code generation.
// Can provide information as to whether a value is in machine registers, and if so which,
// whether a value has been spilled to the RegsiterFile, and if so may be able to provide
// details of the format in memory (all values are spilled in a boxed form, but we may be
// able to track the type of box), and tracks how many outstanding uses of a value remain,
// so that we know when the value is dead and the machine registers associated with it
// may be released.
class GenerationInfo {
public:
    GenerationInfo()
        : m_node(0)
        , m_useCount(0)
        , m_registerFormat(DataFormatNone)
        , m_spillFormat(DataFormatNone)
        , m_canFill(false)
        , m_bornForOSR(false)
        , m_isConstant(false)
    {
    }

    void initConstant(Node* node, uint32_t useCount)
    {
        m_node = node;
        m_useCount = useCount;
        m_registerFormat = DataFormatNone;
        m_spillFormat = DataFormatNone;
        m_canFill = true;
        m_bornForOSR = false;
        m_isConstant = true;
        ASSERT(m_useCount);
    }
    void initInteger(Node* node, uint32_t useCount, GPRReg gpr)
    {
        m_node = node;
        m_useCount = useCount;
        m_registerFormat = DataFormatInteger;
        m_spillFormat = DataFormatNone;
        m_canFill = false;
        u.gpr = gpr;
        m_bornForOSR = false;
        m_isConstant = false;
        ASSERT(m_useCount);
    }
#if USE(JSVALUE64)
    void initJSValue(Node* node, uint32_t useCount, GPRReg gpr, DataFormat format = DataFormatJS)
    {
        ASSERT(format & DataFormatJS);

        m_node = node;
        m_useCount = useCount;
        m_registerFormat = format;
        m_spillFormat = DataFormatNone;
        m_canFill = false;
        u.gpr = gpr;
        m_bornForOSR = false;
        m_isConstant = false;
        ASSERT(m_useCount);
    }
#elif USE(JSVALUE32_64)
    void initJSValue(Node* node, uint32_t useCount, GPRReg tagGPR, GPRReg payloadGPR, DataFormat format = DataFormatJS)
    {
        ASSERT(format & DataFormatJS);

        m_node = node;
        m_useCount = useCount;
        m_registerFormat = format;
        m_spillFormat = DataFormatNone;
        m_canFill = false;
        u.v.tagGPR = tagGPR;
        u.v.payloadGPR = payloadGPR;
        m_bornForOSR = false;
        m_isConstant = false;
        ASSERT(m_useCount);
    }
#endif
    void initCell(Node* node, uint32_t useCount, GPRReg gpr)
    {
        m_node = node;
        m_useCount = useCount;
        m_registerFormat = DataFormatCell;
        m_spillFormat = DataFormatNone;
        m_canFill = false;
        u.gpr = gpr;
        m_bornForOSR = false;
        m_isConstant = false;
        ASSERT(m_useCount);
    }
    void initBoolean(Node* node, uint32_t useCount, GPRReg gpr)
    {
        m_node = node;
        m_useCount = useCount;
        m_registerFormat = DataFormatBoolean;
        m_spillFormat = DataFormatNone;
        m_canFill = false;
        u.gpr = gpr;
        m_bornForOSR = false;
        m_isConstant = false;
        ASSERT(m_useCount);
    }
    void initDouble(Node* node, uint32_t useCount, FPRReg fpr)
    {
        ASSERT(fpr != InvalidFPRReg);
        m_node = node;
        m_useCount = useCount;
        m_registerFormat = DataFormatDouble;
        m_spillFormat = DataFormatNone;
        m_canFill = false;
        u.fpr = fpr;
        m_bornForOSR = false;
        m_isConstant = false;
        ASSERT(m_useCount);
    }
    void initStorage(Node* node, uint32_t useCount, GPRReg gpr)
    {
        m_node = node;
        m_useCount = useCount;
        m_registerFormat = DataFormatStorage;
        m_spillFormat = DataFormatNone;
        m_canFill = false;
        u.gpr = gpr;
        m_bornForOSR = false;
        m_isConstant = false;
        ASSERT(m_useCount);
    }

    // Get the node that produced this value.
    Node* node() { return m_node; }
    
    void noticeOSRBirth(VariableEventStream& stream, Node* node, VirtualRegister virtualRegister)
    {
        if (m_isConstant)
            return;
        if (m_node != node)
            return;
        if (!alive())
            return;
        if (m_bornForOSR)
            return;
        
        m_bornForOSR = true;
        
        if (m_registerFormat != DataFormatNone)
            appendFill(BirthToFill, stream);
        else if (m_spillFormat != DataFormatNone)
            appendSpill(BirthToSpill, stream, virtualRegister);
    }

    // Mark the value as having been used (decrement the useCount).
    // Returns true if this was the last use of the value, and any
    // associated machine registers may be freed.
    bool use(VariableEventStream& stream)
    {
        ASSERT(m_useCount);
        bool result = !--m_useCount;
        
        if (result && m_bornForOSR) {
            ASSERT(m_node);
            stream.appendAndLog(VariableEvent::death(MinifiedID(m_node)));
        }
        
        return result;
    }

    // Used to check the operands of operations to see if they are on
    // their last use; in some cases it may be safe to reuse the same
    // machine register for the result of the operation.
    bool canReuse()
    {
        ASSERT(m_useCount);
        return m_useCount == 1;
    }

    // Get the format of the value in machine registers (or 'none').
    DataFormat registerFormat() { return m_registerFormat; }
    // Get the format of the value as it is spilled in the JSStack (or 'none').
    DataFormat spillFormat() { return m_spillFormat; }
    
    bool isJSFormat(DataFormat expectedFormat)
    {
        return JSC::isJSFormat(registerFormat(), expectedFormat) || JSC::isJSFormat(spillFormat(), expectedFormat);
    }
    
    bool isJSInteger()
    {
        return isJSFormat(DataFormatJSInteger);
    }
    
    bool isJSDouble()
    {
        return isJSFormat(DataFormatJSDouble);
    }
    
    bool isJSCell()
    {
        return isJSFormat(DataFormatJSCell);
    }
    
    bool isJSBoolean()
    {
        return isJSFormat(DataFormatJSBoolean);
    }
    
    bool isUnknownJS()
    {
        return spillFormat() == DataFormatNone
            ? registerFormat() == DataFormatJS || registerFormat() == DataFormatNone
            : spillFormat() == DataFormatJS;
    }

    // Get the machine resister currently holding the value.
#if USE(JSVALUE64)
    GPRReg gpr() { ASSERT(m_registerFormat && m_registerFormat != DataFormatDouble); return u.gpr; }
    FPRReg fpr() { ASSERT(m_registerFormat == DataFormatDouble); return u.fpr; }
    JSValueRegs jsValueRegs() { ASSERT(m_registerFormat & DataFormatJS); return JSValueRegs(u.gpr); }
#elif USE(JSVALUE32_64)
    GPRReg gpr() { ASSERT(!(m_registerFormat & DataFormatJS) && m_registerFormat != DataFormatDouble); return u.gpr; }
    GPRReg tagGPR() { ASSERT(m_registerFormat & DataFormatJS); return u.v.tagGPR; }
    GPRReg payloadGPR() { ASSERT(m_registerFormat & DataFormatJS); return u.v.payloadGPR; }
    FPRReg fpr() { ASSERT(m_registerFormat == DataFormatDouble || m_registerFormat == DataFormatJSDouble); return u.fpr; }
    JSValueRegs jsValueRegs() { ASSERT(m_registerFormat & DataFormatJS); return JSValueRegs(u.v.tagGPR, u.v.payloadGPR); }
#endif

    // Check whether a value needs spilling in order to free up any associated machine registers.
    bool needsSpill()
    {
        // This should only be called on values that are currently in a register.
        ASSERT(m_registerFormat != DataFormatNone);
        // Constants do not need spilling, nor do values that have already been
        // spilled to the JSStack.
        return !m_canFill;
    }

    // Called when a VirtualRegister is being spilled to the JSStack for the first time.
    void spill(VariableEventStream& stream, VirtualRegister virtualRegister, DataFormat spillFormat)
    {
        // We shouldn't be spill values that don't need spilling.
        ASSERT(!m_canFill);
        ASSERT(m_spillFormat == DataFormatNone);
        // We should only be spilling values that are currently in machine registers.
        ASSERT(m_registerFormat != DataFormatNone);

        m_registerFormat = DataFormatNone;
        m_spillFormat = spillFormat;
        m_canFill = true;
        
        if (m_bornForOSR)
            appendSpill(Spill, stream, virtualRegister);
    }

    // Called on values that don't need spilling (constants and values that have
    // already been spilled), to mark them as no longer being in machine registers.
    void setSpilled(VariableEventStream& stream, VirtualRegister virtualRegister)
    {
        // Should only be called on values that don't need spilling, and are currently in registers.
        ASSERT(m_canFill && m_registerFormat != DataFormatNone);
        m_registerFormat = DataFormatNone;
        
        if (m_bornForOSR)
            appendSpill(Spill, stream, virtualRegister);
    }
    
    void killSpilled()
    {
        m_spillFormat = DataFormatNone;
        m_canFill = false;
    }

    // Record that this value is filled into machine registers,
    // tracking which registers, and what format the value has.
#if USE(JSVALUE64)
    void fillJSValue(VariableEventStream& stream, GPRReg gpr, DataFormat format = DataFormatJS)
    {
        ASSERT(format & DataFormatJS);
        m_registerFormat = format;
        u.gpr = gpr;
        
        if (m_bornForOSR)
            appendFill(Fill, stream);
    }
#elif USE(JSVALUE32_64)
    void fillJSValue(VariableEventStream& stream, GPRReg tagGPR, GPRReg payloadGPR, DataFormat format = DataFormatJS)
    {
        ASSERT(format & DataFormatJS);
        m_registerFormat = format;
        u.v.tagGPR = tagGPR; // FIXME: for JSValues with known type (boolean, integer, cell etc.) no tagGPR is needed?
        u.v.payloadGPR = payloadGPR;
        
        if (m_bornForOSR)
            appendFill(Fill, stream);
    }
    void fillCell(VariableEventStream& stream, GPRReg gpr)
    {
        m_registerFormat = DataFormatCell;
        u.gpr = gpr;
        
        if (m_bornForOSR)
            appendFill(Fill, stream);
    }
#endif
    void fillInteger(VariableEventStream& stream, GPRReg gpr)
    {
        m_registerFormat = DataFormatInteger;
        u.gpr = gpr;
        
        if (m_bornForOSR)
            appendFill(Fill, stream);
    }
    void fillBoolean(VariableEventStream& stream, GPRReg gpr)
    {
        m_registerFormat = DataFormatBoolean;
        u.gpr = gpr;
        
        if (m_bornForOSR)
            appendFill(Fill, stream);
    }
    void fillDouble(VariableEventStream& stream, FPRReg fpr)
    {
        ASSERT(fpr != InvalidFPRReg);
        m_registerFormat = DataFormatDouble;
        u.fpr = fpr;
        
        if (m_bornForOSR)
            appendFill(Fill, stream);
    }
    void fillStorage(VariableEventStream& stream, GPRReg gpr)
    {
        m_registerFormat = DataFormatStorage;
        u.gpr = gpr;
        
        if (m_bornForOSR)
            appendFill(Fill, stream);
    }

    bool alive()
    {
        return m_useCount;
    }

private:
    void appendFill(VariableEventKind kind, VariableEventStream& stream)
    {
        ASSERT(m_bornForOSR);
        
        if (m_registerFormat == DataFormatDouble) {
            stream.appendAndLog(VariableEvent::fillFPR(kind, MinifiedID(m_node), u.fpr));
            return;
        }
#if USE(JSVALUE32_64)
        if (m_registerFormat & DataFormatJS) {
            stream.appendAndLog(VariableEvent::fillPair(kind, MinifiedID(m_node), u.v.tagGPR, u.v.payloadGPR));
            return;
        }
#endif
        stream.appendAndLog(VariableEvent::fillGPR(kind, MinifiedID(m_node), u.gpr, m_registerFormat));
    }
    
    void appendSpill(VariableEventKind kind, VariableEventStream& stream, VirtualRegister virtualRegister)
    {
        stream.appendAndLog(VariableEvent::spill(kind, MinifiedID(m_node), virtualRegister, m_spillFormat));
    }
    
    // The node whose result is stored in this virtual register.
    Node* m_node;
    uint32_t m_useCount;
    DataFormat m_registerFormat;
    DataFormat m_spillFormat;
    bool m_canFill;
    bool m_bornForOSR;
    bool m_isConstant;
    union {
        GPRReg gpr;
        FPRReg fpr;
#if USE(JSVALUE32_64)
        struct {
            GPRReg tagGPR;
            GPRReg payloadGPR;
        } v;
#endif
    } u;
};

} } // namespace JSC::DFG

#endif
#endif
