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

#include "config.h"
#include "DFGDominators.h"

#if ENABLE(DFG_JIT)

#include "DFGGraph.h"

namespace JSC { namespace DFG {

Dominators::Dominators()
    : m_valid(false)
{
}

Dominators::~Dominators()
{
}

void Dominators::compute(Graph& graph)
{
    // This implements a naive dominator solver.
    
    ASSERT(graph.m_blocks[0]->m_predecessors.isEmpty());
    
    unsigned numBlocks = graph.m_blocks.size();
    
    if (numBlocks > m_results.size()) {
        m_results.grow(numBlocks);
        for (unsigned i = numBlocks; i--;)
            m_results[i].resize(numBlocks);
        m_scratch.resize(numBlocks);
    }
    
    m_results[0].clearAll();
    m_results[0].set(0);
    
    m_scratch.clearAll();
    for (unsigned i = numBlocks; i--;) {
        if (!graph.m_blocks[i])
            continue;
        m_scratch.set(i);
    }
    
    for (unsigned i = numBlocks; i-- > 1;) {
        if (!graph.m_blocks[i] || graph.m_blocks[i]->m_predecessors.isEmpty())
            m_results[i].clearAll();
        else
            m_results[i].set(m_scratch);
    }
    
    bool changed;
    do {
        changed = false;
        for (unsigned i = 1; i < numBlocks; ++i)
            changed |= iterateForBlock(graph, i);
        if (!changed)
            break;
        
        changed = false;
        for (unsigned i = numBlocks; i-- > 1;)
            changed |= iterateForBlock(graph, i);
    } while (changed);
    
    m_valid = true;
}

bool Dominators::iterateForBlock(Graph& graph, BlockIndex i)
{
    BasicBlock* block = graph.m_blocks[i].get();
    if (!block)
        return false;
    if (block->m_predecessors.isEmpty())
        return false;
    m_scratch.set(m_results[block->m_predecessors[0]]);
    for (unsigned j = block->m_predecessors.size(); j-- > 1;)
        m_scratch.filter(m_results[block->m_predecessors[j]]);
    m_scratch.set(i);
    return m_results[i].setAndCheck(m_scratch);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

