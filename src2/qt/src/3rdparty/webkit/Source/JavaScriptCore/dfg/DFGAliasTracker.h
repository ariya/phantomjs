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

#ifndef DFGAliasTracker_h
#define DFGAliasTracker_h

#if ENABLE(DFG_JIT)

#include <dfg/DFGGraph.h>
#include <wtf/Vector.h>

namespace JSC { namespace DFG {

// === AliasTracker ===
//
// This class id used to detect aliasing property accesses, which we may
// be able to speculatively optimize (for example removing redundant loads
// where we know a getter will not be called, or optimizing puts to arrays
// where we know the value being written to in within length and is not a
// hole value). In time, this should be more than a 1-deep buffer!
class AliasTracker {
public:
    AliasTracker(Graph& graph)
        : m_graph(graph)
        , m_candidateAliasGetByVal(NoNode)
    {
    }

    NodeIndex lookupGetByVal(NodeIndex base, NodeIndex property)
    {
        // Try to detect situations where a GetByVal follows another GetByVal to the same
        // property; in these cases, we may be able to omit the subsequent get on the
        // speculative path, where we know conditions hold to make this safe (for example,
        // on the speculative path we will not have allowed getter access).
        if (m_candidateAliasGetByVal != NoNode) {
            Node& possibleAlias = m_graph[m_candidateAliasGetByVal];
            ASSERT(possibleAlias.op == GetByVal);
            // This check ensures the accesses alias, provided that the subscript is an
            // integer index (this is good enough; the speculative path will only generate
            // optimized accesses to handle integer subscripts).
            if (possibleAlias.child1 == base && equalIgnoringLaterNumericConversion(possibleAlias.child2, property))
                return m_candidateAliasGetByVal;
        }
        return NoNode;
    }

    void recordGetByVal(NodeIndex getByVal)
    {
        m_candidateAliasGetByVal = getByVal;
    }

    void recordPutByVal(NodeIndex putByVal)
    {
        ASSERT_UNUSED(putByVal, m_graph[putByVal].op == PutByVal || m_graph[putByVal].op == PutByValAlias);
        m_candidateAliasGetByVal = NoNode;
    }

    void recordGetById(NodeIndex getById)
    {
        ASSERT_UNUSED(getById, m_graph[getById].op == GetById);
        m_candidateAliasGetByVal = NoNode;
    }

    void recordPutById(NodeIndex putById)
    {
        ASSERT_UNUSED(putById, m_graph[putById].op == PutById);
        m_candidateAliasGetByVal = NoNode;
    }

    void recordPutByIdDirect(NodeIndex putByVal)
    {
        ASSERT_UNUSED(putByVal, m_graph[putByVal].op == PutByIdDirect);
        m_candidateAliasGetByVal = NoNode;
    }

private:
    // This method returns true for arguments:
    //   - (X, X)
    //   - (X, ValueToNumber(X))
    //   - (X, ValueToInt32(X))
    //   - (X, NumberToInt32(X))
    bool equalIgnoringLaterNumericConversion(NodeIndex op1, NodeIndex op2)
    {
        if (op1 == op2)
            return true;
        Node& node2 = m_graph[op2];
        return (node2.op == ValueToNumber || node2.op == ValueToInt32 || node2.op == NumberToInt32) && op1 == node2.child1;
    }

    // The graph, to look up potentially aliasing nodes.
    Graph& m_graph;
    // Currently a 1-deep buffer!
    NodeIndex m_candidateAliasGetByVal;
};

} } // namespace JSC::DFG

#endif
#endif
