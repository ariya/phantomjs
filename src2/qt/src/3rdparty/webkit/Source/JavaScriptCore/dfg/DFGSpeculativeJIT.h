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

#ifndef DFGSpeculativeJIT_h
#define DFGSpeculativeJIT_h

#if ENABLE(DFG_JIT)

#include <dfg/DFGJITCodeGenerator.h>

namespace JSC { namespace DFG {

class SpeculativeJIT;

// This enum describes the types of additional recovery that
// may need be performed should a speculation check fail.
enum SpeculationRecoveryType {
    SpeculativeAdd
};

// === SpeculationRecovery ===
//
// This class provides additional information that may be associated with a
// speculation check - for example 
class SpeculationRecovery {
public:
    SpeculationRecovery(SpeculationRecoveryType type, GPRReg dest, GPRReg src)
        : m_type(type)
        , m_dest(dest)
        , m_src(src)
    {
    }

    SpeculationRecoveryType type() { return m_type; }
    GPRReg dest() { return m_dest; }
    GPRReg src() { return m_src; }

private:
    // Indicates the type of additional recovery to be performed.
    SpeculationRecoveryType m_type;
    // different recovery types may required different additional information here.
    GPRReg m_dest;
    GPRReg m_src;
};

// === SpeculationCheck ===
//
// This structure records a bail-out from the speculative path,
// which will need to be linked in to the non-speculative one.
struct SpeculationCheck {
    SpeculationCheck(MacroAssembler::Jump, SpeculativeJIT*, unsigned recoveryIndex = 0);

    // The location of the jump out from the speculative path, 
    // and the node we were generating code for.
    MacroAssembler::Jump m_check;
    NodeIndex m_nodeIndex;
    // Used to record any additional recovery to be performed; this
    // value is an index into the SpeculativeJIT's m_speculationRecoveryList
    // array, offset by 1. (m_recoveryIndex == 0) means no recovery.
    unsigned m_recoveryIndex;

    struct RegisterInfo {
        NodeIndex nodeIndex;
        DataFormat format;
    };
    RegisterInfo m_gprInfo[GPRInfo::numberOfRegisters];
    NodeIndex m_fprInfo[FPRInfo::numberOfRegisters];
};
typedef SegmentedVector<SpeculationCheck, 16> SpeculationCheckVector;


// === SpeculativeJIT ===
//
// The SpeculativeJIT is used to generate a fast, but potentially
// incomplete code path for the dataflow. When code generating
// we may make assumptions about operand types, dynamically check,
// and bail-out to an alternate code path if these checks fail.
// Importantly, the speculative code path cannot be reentered once
// a speculative check has failed. This allows the SpeculativeJIT
// to propagate type information (including information that has
// only speculatively been asserted) through the dataflow.
class SpeculativeJIT : public JITCodeGenerator {
    friend struct SpeculationCheck;
public:
    SpeculativeJIT(JITCompiler& jit)
        : JITCodeGenerator(jit, true)
        , m_compileOkay(true)
    {
    }

    bool compile();

    // Retrieve the list of bail-outs from the speculative path,
    // and additional recovery information.
    SpeculationCheckVector& speculationChecks()
    {
        return m_speculationChecks;
    }
    SpeculationRecovery* speculationRecovery(size_t index)
    {
        // SpeculationCheck::m_recoveryIndex is offset by 1,
        // 0 means no recovery.
        return index ? &m_speculationRecoveryList[index - 1] : 0;
    }

    // Called by the speculative operand types, below, to fill operand to
    // machine registers, implicitly generating speculation checks as needed.
    GPRReg fillSpeculateInt(NodeIndex, DataFormat& returnFormat);
    GPRReg fillSpeculateIntStrict(NodeIndex);
    GPRReg fillSpeculateCell(NodeIndex);

private:
    void compile(Node&);
    void compile(BasicBlock&);

    void checkArgumentTypes();
    void initializeVariableTypes();

    bool isDoubleConstantWithInt32Value(NodeIndex nodeIndex, int32_t& out)
    {
        if (!m_jit.isDoubleConstant(nodeIndex))
            return false;
        double value = m_jit.valueOfDoubleConstant(nodeIndex);

        int32_t asInt32 = static_cast<int32_t>(value);
        if (value != asInt32)
            return false;
        if (!asInt32 && signbit(value))
            return false;

        out = asInt32;
        return true;
    }

    bool isJSConstantWithInt32Value(NodeIndex nodeIndex, int32_t& out)
    {
        if (!m_jit.isJSConstant(nodeIndex))
            return false;
        JSValue value = m_jit.valueOfJSConstant(nodeIndex);

        if (!value.isInt32())
            return false;
        
        out = value.asInt32();
        return true;
    }

    bool detectPeepHoleBranch()
    {
        // Check if the block contains precisely one more node.
        if (m_compileIndex + 2 != m_jit.graph().m_blocks[m_block]->end)
            return false;

        // Check if the lastNode is a branch on this node.
        Node& lastNode = m_jit.graph()[m_compileIndex + 1];
        return lastNode.op == Branch && lastNode.child1 == m_compileIndex;
    }

    void compilePeepHoleBranch(Node&, JITCompiler::RelationalCondition);

    // Add a speculation check without additional recovery.
    void speculationCheck(MacroAssembler::Jump jumpToFail)
    {
        m_speculationChecks.append(SpeculationCheck(jumpToFail, this));
    }
    // Add a speculation check with additional recovery.
    void speculationCheck(MacroAssembler::Jump jumpToFail, const SpeculationRecovery& recovery)
    {
        m_speculationRecoveryList.append(recovery);
        m_speculationChecks.append(SpeculationCheck(jumpToFail, this, m_speculationRecoveryList.size()));
    }

    // Called when we statically determine that a speculation will fail.
    void terminateSpeculativeExecution()
    {
        // FIXME: in cases where we can statically determine we're going to bail out from the speculative
        // JIT we should probably rewind code generation and only produce the non-speculative path.
        m_compileOkay = false;
        speculationCheck(m_jit.jump());
    }

    template<bool strict>
    GPRReg fillSpeculateIntInternal(NodeIndex, DataFormat& returnFormat);

    // It is possible, during speculative generation, to reach a situation in which we
    // can statically determine a speculation will fail (for example, when two nodes
    // will make conflicting speculations about the same operand). In such cases this
    // flag is cleared, indicating no further code generation should take place.
    bool m_compileOkay;
    // This vector tracks bail-outs from the speculative path to the non-speculative one.
    SpeculationCheckVector m_speculationChecks;
    // Some bail-outs need to record additional information recording specific recovery
    // to be performed (for example, on detected overflow from an add, we may need to
    // reverse the addition if an operand is being overwritten).
    Vector<SpeculationRecovery, 16> m_speculationRecoveryList;
};


// === Speculative Operand types ===
//
// SpeculateIntegerOperand, SpeculateStrictInt32Operand and SpeculateCellOperand.
//
// These are used to lock the operands to a node into machine registers within the
// SpeculativeJIT. The classes operate like those provided by the JITCodeGenerator,
// however these will perform a speculative check for a more restrictive type than
// we can statically determine the operand to have. If the operand does not have
// the requested type, a bail-out to the non-speculative path will be taken.

class SpeculateIntegerOperand {
public:
    explicit SpeculateIntegerOperand(SpeculativeJIT* jit, NodeIndex index)
        : m_jit(jit)
        , m_index(index)
        , m_gprOrInvalid(InvalidGPRReg)
#ifndef NDEBUG
        , m_format(DataFormatNone)
#endif
    {
        ASSERT(m_jit);
        if (jit->isFilled(index))
            gpr();
    }

    ~SpeculateIntegerOperand()
    {
        ASSERT(m_gprOrInvalid != InvalidGPRReg);
        m_jit->unlock(m_gprOrInvalid);
    }

    NodeIndex index() const
    {
        return m_index;
    }

    DataFormat format()
    {
        gpr(); // m_format is set when m_gpr is locked.
        ASSERT(m_format == DataFormatInteger || m_format == DataFormatJSInteger);
        return m_format;
    }

    GPRReg gpr()
    {
        if (m_gprOrInvalid == InvalidGPRReg)
            m_gprOrInvalid = m_jit->fillSpeculateInt(index(), m_format);
        return m_gprOrInvalid;
    }

private:
    SpeculativeJIT* m_jit;
    NodeIndex m_index;
    GPRReg m_gprOrInvalid;
    DataFormat m_format;
};

class SpeculateStrictInt32Operand {
public:
    explicit SpeculateStrictInt32Operand(SpeculativeJIT* jit, NodeIndex index)
        : m_jit(jit)
        , m_index(index)
        , m_gprOrInvalid(InvalidGPRReg)
    {
        ASSERT(m_jit);
        if (jit->isFilled(index))
            gpr();
    }

    ~SpeculateStrictInt32Operand()
    {
        ASSERT(m_gprOrInvalid != InvalidGPRReg);
        m_jit->unlock(m_gprOrInvalid);
    }

    NodeIndex index() const
    {
        return m_index;
    }

    GPRReg gpr()
    {
        if (m_gprOrInvalid == InvalidGPRReg)
            m_gprOrInvalid = m_jit->fillSpeculateIntStrict(index());
        return m_gprOrInvalid;
    }

private:
    SpeculativeJIT* m_jit;
    NodeIndex m_index;
    GPRReg m_gprOrInvalid;
};

class SpeculateCellOperand {
public:
    explicit SpeculateCellOperand(SpeculativeJIT* jit, NodeIndex index)
        : m_jit(jit)
        , m_index(index)
        , m_gprOrInvalid(InvalidGPRReg)
    {
        ASSERT(m_jit);
        if (jit->isFilled(index))
            gpr();
    }

    ~SpeculateCellOperand()
    {
        ASSERT(m_gprOrInvalid != InvalidGPRReg);
        m_jit->unlock(m_gprOrInvalid);
    }

    NodeIndex index() const
    {
        return m_index;
    }

    GPRReg gpr()
    {
        if (m_gprOrInvalid == InvalidGPRReg)
            m_gprOrInvalid = m_jit->fillSpeculateCell(index());
        return m_gprOrInvalid;
    }

private:
    SpeculativeJIT* m_jit;
    NodeIndex m_index;
    GPRReg m_gprOrInvalid;
};


// === SpeculationCheckIndexIterator ===
//
// This class is used by the non-speculative JIT to check which
// nodes require entry points from the speculative path.
class SpeculationCheckIndexIterator {
public:
    SpeculationCheckIndexIterator(SpeculationCheckVector& speculationChecks)
        : m_speculationChecks(speculationChecks)
        , m_iter(m_speculationChecks.begin())
        , m_end(m_speculationChecks.end())
    {
    }

    bool hasCheckAtIndex(NodeIndex nodeIndex)
    {
        while (m_iter != m_end) {
            NodeIndex current = m_iter->m_nodeIndex;
            if (current >= nodeIndex)
                return current == nodeIndex;
            ++m_iter;
        }
        return false;
    }

private:
    SpeculationCheckVector& m_speculationChecks;
    SpeculationCheckVector::Iterator m_iter;
    SpeculationCheckVector::Iterator m_end;
};


} } // namespace JSC::DFG

#endif
#endif

