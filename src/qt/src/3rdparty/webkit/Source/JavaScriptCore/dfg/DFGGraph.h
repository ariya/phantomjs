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

#ifndef DFGGraph_h
#define DFGGraph_h

#if ENABLE(DFG_JIT)

#include <RegisterFile.h>
#include <dfg/DFGNode.h>
#include <wtf/Vector.h>
#include <wtf/StdLibExtras.h>

namespace JSC {

class CodeBlock;

namespace DFG {

// helper function to distinguish vars & temporaries from arguments.
inline bool operandIsArgument(int operand) { return operand < 0; }

typedef uint8_t PredictedType;
static const PredictedType PredictNone  = 0;
static const PredictedType PredictCell  = 0x01;
static const PredictedType PredictArray = 0x03;
static const PredictedType PredictInt32 = 0x04;

struct PredictionSlot {
public:
    PredictionSlot()
        : m_value(PredictNone)
    {
    }
    PredictedType m_value;
};

typedef uint32_t BlockIndex;

// For every local variable we track any existing get or set of the value.
// We track the get so that these may be shared, and we track the set to
// retrieve the current value, and to reference the final definition.
struct VariableRecord {
    VariableRecord()
        : value(NoNode)
    {
    }

    NodeIndex value;
};

typedef Vector <BlockIndex, 2> PredecessorList;

struct BasicBlock {
    BasicBlock(unsigned bytecodeBegin, NodeIndex begin, unsigned numArguments, unsigned numLocals)
        : bytecodeBegin(bytecodeBegin)
        , begin(begin)
        , end(NoNode)
        , m_arguments(numArguments)
        , m_locals(numLocals)
    {
    }

    static inline BlockIndex getBytecodeBegin(OwnPtr<BasicBlock>* block)
    {
        return (*block)->bytecodeBegin;
    }

    unsigned bytecodeBegin;
    NodeIndex begin;
    NodeIndex end;

    PredecessorList m_predecessors;
    Vector <VariableRecord, 8> m_arguments;
    Vector <VariableRecord, 16> m_locals;
};

// 
// === Graph ===
//
// The dataflow graph is an ordered vector of nodes.
// The order may be significant for nodes with side-effects (property accesses, value conversions).
// Nodes that are 'dead' remain in the vector with refCount 0.
class Graph : public Vector<Node, 64> {
public:
    Graph(unsigned numArguments, unsigned numVariables)
        : m_argumentPredictions(numArguments)
        , m_variablePredictions(numVariables)
    {
    }

    // Mark a node as being referenced.
    void ref(NodeIndex nodeIndex)
    {
        Node& node = at(nodeIndex);
        // If the value (before incrementing) was at refCount zero then we need to ref its children.
        if (node.ref())
            refChildren(nodeIndex);
    }

#ifndef NDEBUG
    // CodeBlock is optional, but may allow additional information to be dumped (e.g. Identifier names).
    void dump(CodeBlock* = 0);
    void dump(NodeIndex, CodeBlock* = 0);
#endif

    BlockIndex blockIndexForBytecodeOffset(unsigned bytecodeBegin)
    {
        OwnPtr<BasicBlock>* begin = m_blocks.begin();
        OwnPtr<BasicBlock>* block = binarySearch<OwnPtr<BasicBlock>, unsigned, BasicBlock::getBytecodeBegin>(begin, m_blocks.size(), bytecodeBegin);
        ASSERT(block >= m_blocks.begin() && block < m_blocks.end());
        return static_cast<BlockIndex>(block - begin);
    }

    BasicBlock& blockForBytecodeOffset(unsigned bytecodeBegin)
    {
        return *m_blocks[blockIndexForBytecodeOffset(bytecodeBegin)];
    }

    void predict(int operand, PredictedType prediction)
    {
        if (operandIsArgument(operand)) {
            unsigned argument = operand + m_argumentPredictions.size() + RegisterFile::CallFrameHeaderSize;
            m_argumentPredictions[argument].m_value |= prediction;
        } else if ((unsigned)operand < m_variablePredictions.size())
            m_variablePredictions[operand].m_value |= prediction;
            
    }

    PredictedType getPrediction(int operand)
    {
        if (operandIsArgument(operand)) {
            unsigned argument = operand + m_argumentPredictions.size() + RegisterFile::CallFrameHeaderSize;
            return m_argumentPredictions[argument].m_value;
        }
        if ((unsigned)operand < m_variablePredictions.size())
            return m_variablePredictions[operand].m_value;
        return PredictNone;
    }

    Vector< OwnPtr<BasicBlock> , 8> m_blocks;
private:

    // When a node's refCount goes from 0 to 1, it must (logically) recursively ref all of its children, and vice versa.
    void refChildren(NodeIndex);

    Vector<PredictionSlot, 16> m_argumentPredictions;
    Vector<PredictionSlot, 16> m_variablePredictions;
};

} } // namespace JSC::DFG

#endif
#endif
