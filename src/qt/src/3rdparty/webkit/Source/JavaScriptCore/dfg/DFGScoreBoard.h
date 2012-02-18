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

#ifndef DFGScoreBoard_h
#define DFGScoreBoard_h

#if ENABLE(DFG_JIT)

#include <dfg/DFGGraph.h>
#include <wtf/Vector.h>

namespace JSC { namespace DFG {

// === ScoreBoard ===
//
// This class is used in performing a virtual register allocation over the graph.
// VirtualRegisters are allocated to nodes, with a used count for each virtual
// register tracking the lifespan of the value; after the final use of a node
// the VirtualRegister associated is freed such that it can be reused for
// another node.
class ScoreBoard {
public:
    ScoreBoard(Graph& graph, uint32_t firstTemporary)
        : m_graph(graph)
        , m_firstTemporary(firstTemporary)
    {
    }

#if DFG_CONSISTENCY_CHECK
    ~ScoreBoard()
    {
        // Every VirtualRegister that was allocated should now be free.
        ASSERT(m_used.size() == m_free.size());
        // Every entry in the used list should be available in the free list.
        for (size_t i = 0; i < m_used.size(); ++i)
            ASSERT(m_free.contains(i));
        // For every entry in the used list the use count of the virtual register should be zero.
        for (size_t i = 0; i < m_free.size(); ++i)
            ASSERT(!m_used[i]);
    }
#endif

    VirtualRegister allocate()
    {
        // Do we have any VirtualRegsiters in the free list, that were used by
        // prior nodes, but are now available?
        if (!m_free.isEmpty()) {
            uint32_t index = m_free.last();
            m_free.removeLast();
            // Use count must have hit zero for it to have been added to the free list!
            ASSERT(!m_used[index]);
            return (VirtualRegister)(m_firstTemporary + index);
        }

        // Allocate a new VirtualRegister, and add a corresponding entry to m_used.
        size_t next = allocatedCount();
        m_used.append(0);
        return (VirtualRegister)(m_firstTemporary + next);
    }

    // Increment the usecount for the VirtualRegsiter associated with 'child',
    // if it reaches the node's refcount, free the VirtualRegsiter.
    void use(NodeIndex child)
    {
        if (child == NoNode)
            return;

        // Find the virtual register number for this child, increment its use count.
        Node& node = m_graph[child];
        uint32_t index = node.virtualRegister() - m_firstTemporary;
        if (node.refCount() == ++m_used[index]) {
            // If the use count in the scoreboard reaches the use count for the node,
            // then this was its last use; the virtual register is now free.
            // Clear the use count & add to the free list.
            m_used[index] = 0;
            m_free.append(index);
        }
    }

    unsigned allocatedCount()
    {
        // m_used contains an entry for every allocated VirtualRegister.
        return m_used.size();
    }

#ifndef NDEBUG
    void dump()
    {
        printf("    USED: [ ");
        for (unsigned i = 0; i < m_used.size(); ++i) {
            if (!m_free.contains(i))
                printf("%d:%d ", m_firstTemporary + i, m_used[i]);
        }
        printf("]\n");

        printf("    FREE: [ ");
        for (unsigned i = 0; i < m_used.size(); ++i) {
            if (m_free.contains(i)) {
                ASSERT(!m_used[i]);
                printf("%d ", m_firstTemporary + i);
            }
        }
        printf("]\n");
    }

#endif

private:
    // The graph, so we can get refCounts for nodes, to determine when values are dead.
    Graph& m_graph;
    // The first VirtualRegsiter to be used as a temporary.
    uint32_t m_firstTemporary;
    
    // For every virtual register that has been allocated (either currently alive, or in
    // the free list), we keep a count of the number of remaining uses until it is dead
    // (0, in the case of entries in the free list). Since there is an entry for every
    // allocated VirtualRegister, the length of this array conveniently provides the
    // next available VirtualRegister number.
    Vector<uint32_t, 64> m_used;
    // A free list of VirtualRegsiters no longer alive.
    Vector<uint32_t, 64> m_free;
};

} } // namespace JSC::DFG

#endif
#endif
