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

#include "DFGGraph.h"
#include <wtf/BitVector.h>
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
    ScoreBoard(const BitVector& usedVars)
        : m_highWatermark(0)
    {
        m_used.fill(0, usedVars.size());
        m_free.reserveCapacity(usedVars.size());
        for (size_t i = usedVars.size(); i-- > 0;) {
            if (usedVars.get(i)) {
                m_used[i] = max(); // This is mostly for debugging and sanity.
                m_highWatermark = std::max(m_highWatermark, static_cast<unsigned>(i) + 1);
            } else
                m_free.append(i);
        }
    }

    ~ScoreBoard()
    {
        assertClear();
    }
    
    void assertClear()
    {
#if !ASSERT_DISABLED
        // For every entry in the used list the use count of the virtual register should be zero, or max, due to it being a preserved local.
        for (size_t i = 0; i < m_used.size(); ++i)
            ASSERT(!m_used[i] || m_used[i] == max());
        // For every entry in the free list, the use count should be zero.
        for (size_t i = 0; i < m_free.size(); ++i)
            ASSERT(!m_used[m_free[i]]);
        // There must not be duplicates in the free list.
        for (size_t i = 0; i < m_free.size(); ++i) {
            for (size_t j = i + 1; j < m_free.size(); ++j)
                ASSERT(m_free[i] != m_free[j]);
        }
#endif
    }

    VirtualRegister allocate()
    {
        // Do we have any VirtualRegsiters in the free list, that were used by
        // prior nodes, but are now available?
        if (!m_free.isEmpty()) {
            uint32_t index = m_free.last();
            m_free.removeLast();
            // Use count must have hit zero for it to have been added to the free list!
            ASSERT(!m_used[index]);
            m_highWatermark = std::max(m_highWatermark, static_cast<unsigned>(index) + 1);
            return (VirtualRegister)index;
        }

        // Allocate a new VirtualRegister, and add a corresponding entry to m_used.
        size_t next = m_used.size();
        m_used.append(0);
        m_highWatermark = std::max(m_highWatermark, static_cast<unsigned>(next) + 1);
        return (VirtualRegister)next;
    }

    // Increment the usecount for the VirtualRegsiter associated with 'child',
    // if it reaches the node's refcount, free the VirtualRegsiter.
    void use(Node* child)
    {
        if (!child)
            return;

        // Find the virtual register number for this child, increment its use count.
        uint32_t index = child->virtualRegister();
        ASSERT(m_used[index] != max());
        if (child->refCount() == ++m_used[index]) {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
            dataLogF(" Freeing virtual register %u.", index);
#endif
            // If the use count in the scoreboard reaches the use count for the node,
            // then this was its last use; the virtual register is now free.
            // Clear the use count & add to the free list.
            m_used[index] = 0;
            m_free.append(index);
        } else {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
            dataLogF(" Virtual register %u is at %u/%u uses.", index, m_used[index], child->refCount());
#endif
        }
    }
    void use(Edge child)
    {
        use(child.node());
    }
    
    void useIfHasResult(Edge child)
    {
        if (!child)
            return;
        if (!child->hasResult())
            return;
        use(child);
    }

    unsigned highWatermark()
    {
        return m_highWatermark;
    }
    
#ifndef NDEBUG
    void dump()
    {
        dataLogF("    USED: [ ");
        for (unsigned i = 0; i < m_used.size(); ++i) {
            if (!m_free.contains(i)) {
                dataLogF("%d:", i);
                if (m_used[i] == max())
                    dataLogF("local ");
                else
                    dataLogF("%d ", m_used[i]);
            }
        }
        dataLogF("]\n");

        dataLogF("    FREE: [ ");
        for (unsigned i = 0; i < m_used.size(); ++i) {
            if (m_free.contains(i) && m_used[i] != max()) {
                ASSERT(!m_used[i]);
                dataLogF("%d ", i);
            }
        }
        dataLogF("]\n");
    }

#endif

private:
    static uint32_t max() { return std::numeric_limits<uint32_t>::max(); }
    
    // The size of the span of virtual registers that this code block will use.
    unsigned m_highWatermark;
    
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
