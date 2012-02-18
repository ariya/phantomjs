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

#ifndef DFGGenerationInfo_h
#define DFGGenerationInfo_h

#if ENABLE(DFG_JIT)

#include <dfg/DFGJITCompiler.h>

namespace JSC { namespace DFG {

// === DataFormat ===
//
// This enum tracks the current representation in which a value is being held.
// Values may be unboxed primitives (int32, double, or cell), or boxed as a JSValue.
// For boxed values, we may know the type of boxing that has taken place.
// (May also need bool, array, object, string types!)
enum DataFormat {
    DataFormatNone = 0,
    DataFormatInteger = 1,
    DataFormatDouble = 2,
    DataFormatCell = 3,
    DataFormatJS = 8,
    DataFormatJSInteger = DataFormatJS | DataFormatInteger,
    DataFormatJSDouble = DataFormatJS | DataFormatDouble,
    DataFormatJSCell = DataFormatJS | DataFormatCell,
};

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
        : m_nodeIndex(NoNode)
        , m_useCount(0)
        , m_registerFormat(DataFormatNone)
        , m_spillFormat(DataFormatNone)
        , m_canFill(false)
    {
    }

    void initConstant(NodeIndex nodeIndex, uint32_t useCount)
    {
        m_nodeIndex = nodeIndex;
        m_useCount = useCount;
        m_registerFormat = DataFormatNone;
        m_spillFormat = DataFormatNone;
        m_canFill = true;
    }
    void initInteger(NodeIndex nodeIndex, uint32_t useCount, GPRReg gpr)
    {
        m_nodeIndex = nodeIndex;
        m_useCount = useCount;
        m_registerFormat = DataFormatInteger;
        m_spillFormat = DataFormatNone;
        m_canFill = false;
        u.gpr = gpr;
    }
    void initJSValue(NodeIndex nodeIndex, uint32_t useCount, GPRReg gpr, DataFormat format = DataFormatJS)
    {
        ASSERT(format & DataFormatJS);

        m_nodeIndex = nodeIndex;
        m_useCount = useCount;
        m_registerFormat = format;
        m_spillFormat = DataFormatNone;
        m_canFill = false;
        u.gpr = gpr;
    }
    void initCell(NodeIndex nodeIndex, uint32_t useCount, GPRReg gpr)
    {
        m_nodeIndex = nodeIndex;
        m_useCount = useCount;
        m_registerFormat = DataFormatCell;
        m_spillFormat = DataFormatNone;
        m_canFill = false;
        u.gpr = gpr;
    }
    void initDouble(NodeIndex nodeIndex, uint32_t useCount, FPRReg fpr)
    {
        m_nodeIndex = nodeIndex;
        m_useCount = useCount;
        m_registerFormat = DataFormatDouble;
        m_spillFormat = DataFormatNone;
        m_canFill = false;
        u.fpr = fpr;
    }

    // Get the index of the node that produced this value.
    NodeIndex nodeIndex() { return m_nodeIndex; }

    // Mark the value as having been used (decrement the useCount).
    // Returns true if this was the last use of the value, and any
    // associated machine registers may be freed.
    bool use()
    {
        return !--m_useCount;
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
    // Get the format of the value as it is spilled in the RegisterFile (or 'none').
    DataFormat spillFormat() { return m_spillFormat; }

    // Get the machine resister currently holding the value.
    GPRReg gpr() { ASSERT(m_registerFormat && m_registerFormat != DataFormatDouble); return u.gpr; }
    FPRReg fpr() { ASSERT(m_registerFormat == DataFormatDouble); return u.fpr; }

    // Check whether a value needs spilling in order to free up any associated machine registers.
    bool needsSpill()
    {
        // This should only be called on values that are currently in a register.
        ASSERT(m_registerFormat != DataFormatNone);
        // Constants do not need spilling, nor do values that have already been
        // spilled to the RegisterFile.
        return !m_canFill;
    }

    // Called when a VirtualRegister is being spilled to the RegisterFile for the first time.
    void spill(DataFormat spillFormat)
    {
        // We shouldn't be spill values that don't need spilling.
        ASSERT(!m_canFill);
        ASSERT(m_spillFormat == DataFormatNone);
        // We should only be spilling values that are currently in machine registers.
        ASSERT(m_registerFormat != DataFormatNone);
        // We only spill values that have been boxed as a JSValue; otherwise the GC
        // would need a way to distinguish cell pointers from numeric primitives.
        ASSERT(spillFormat & DataFormatJS);

        m_registerFormat = DataFormatNone;
        m_spillFormat = spillFormat;
        m_canFill = true;
    }

    // Called on values that don't need spilling (constants and values that have
    // already been spilled), to mark them as no longer being in machine registers.
    void setSpilled()
    {
        // Should only be called on values that don't need spilling, and are currently in registers.
        ASSERT(m_canFill && m_registerFormat != DataFormatNone);
        m_registerFormat = DataFormatNone;
    }

    // Record that this value is filled into machine registers,
    // tracking which registers, and what format the value has.
    void fillJSValue(GPRReg gpr, DataFormat format = DataFormatJS)
    {
        ASSERT(format & DataFormatJS);
        m_registerFormat = format;
        u.gpr = gpr;
    }
    void fillInteger(GPRReg gpr)
    {
        m_registerFormat = DataFormatInteger;
        u.gpr = gpr;
    }
    void fillDouble(FPRReg fpr)
    {
        m_registerFormat = DataFormatDouble;
        u.fpr = fpr;
    }

#ifndef NDEBUG
    bool alive()
    {
        return m_useCount;
    }
#endif

private:
    // The index of the node whose result is stored in this virtual register.
    // FIXME: Can we remove this? - this is currently only used when collecting
    // snapshots of the RegisterBank for SpeculationCheck/EntryLocation. Could
    // investigate storing NodeIndex as the name in RegsiterBank, instead of
    // VirtualRegister.
    NodeIndex m_nodeIndex;
    uint32_t m_useCount;
    DataFormat m_registerFormat;
    DataFormat m_spillFormat;
    bool m_canFill;
    union {
        GPRReg gpr;
        FPRReg fpr;
    } u;
};

} } // namespace JSC::DFG

#endif
#endif
