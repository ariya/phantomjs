/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "DFGDCEPhase.h"

#if ENABLE(DFG_JIT)

#include "DFGBasicBlockInlines.h"
#include "DFGGraph.h"
#include "DFGInsertionSet.h"
#include "DFGPhase.h"
#include "Operations.h"

namespace JSC { namespace DFG {

class DCEPhase : public Phase {
public:
    DCEPhase(Graph& graph)
        : Phase(graph, "dead code elimination")
    {
    }
    
    bool run()
    {
        // First reset the counts to 0 for all nodes.
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
            BasicBlock* block = m_graph.m_blocks[blockIndex].get();
            if (!block)
                continue;
            for (unsigned indexInBlock = block->size(); indexInBlock--;)
                block->at(indexInBlock)->setRefCount(0);
            for (unsigned phiIndex = block->phis.size(); phiIndex--;)
                block->phis[phiIndex]->setRefCount(0);
        }
    
        // Now find the roots:
        // - Nodes that are must-generate.
        // - Nodes that are reachable from type checks.
        // Set their ref counts to 1 and put them on the worklist.
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
            BasicBlock* block = m_graph.m_blocks[blockIndex].get();
            if (!block)
                continue;
            for (unsigned indexInBlock = block->size(); indexInBlock--;) {
                Node* node = block->at(indexInBlock);
                DFG_NODE_DO_TO_CHILDREN(m_graph, node, findTypeCheckRoot);
                if (!(node->flags() & NodeMustGenerate))
                    continue;
                if (!node->postfixRef())
                    m_worklist.append(node);
            }
        }
        
        while (!m_worklist.isEmpty()) {
            Node* node = m_worklist.last();
            m_worklist.removeLast();
            ASSERT(node->shouldGenerate()); // It should not be on the worklist unless it's ref'ed.
            DFG_NODE_DO_TO_CHILDREN(m_graph, node, countEdge);
        }
        
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
            BasicBlock* block = m_graph.m_blocks[blockIndex].get();
            if (!block)
                continue;

            InsertionSet insertionSet(m_graph);

            for (unsigned indexInBlock = block->size(); indexInBlock--;) {
                Node* node = block->at(indexInBlock);
                if (node->shouldGenerate())
                    continue;
                
                switch (node->op()) {
                case SetLocal: {
                    if (node->child1().isProved() || node->child1().useKind() == UntypedUse) {
                        // Consider the possibility that UInt32ToNumber is dead but its
                        // child isn't; if so then we should MovHint the child.
                        if (!node->child1()->shouldGenerate()
                            && node->child1()->op() == UInt32ToNumber)
                            node->child1() = node->child1()->child1();

                        if (!node->child1()->shouldGenerate()) {
                            node->setOpAndDefaultFlags(ZombieHint);
                            node->child1() = Edge();
                            break;
                        }
                        node->setOpAndDefaultFlags(MovHint);
                        break;
                    }
                    node->setOpAndDefaultFlags(MovHintAndCheck);
                    node->setRefCount(1);
                    break;
                }
                    
                case GetLocal:
                case SetArgument: {
                    // Leave them as not shouldGenerate.
                    break;
                }

                default: {
                    if (node->flags() & NodeHasVarArgs) {
                        for (unsigned childIdx = node->firstChild(); childIdx < node->firstChild() + node->numChildren(); childIdx++) {
                            Edge edge = m_graph.m_varArgChildren[childIdx];

                            if (!edge || edge.isProved() || edge.useKind() == UntypedUse)
                                continue;

                            insertionSet.insertNode(indexInBlock, SpecNone, Phantom, node->codeOrigin, edge);
                        }

                        node->convertToPhantomUnchecked();
                        node->children.reset();
                        node->setRefCount(1);
                        break;
                    }

                    node->convertToPhantom();
                    eliminateIrrelevantPhantomChildren(node);
                    node->setRefCount(1);
                    break;
                } }
            }

            insertionSet.execute(block);
        }
        
        m_graph.m_refCountState = ExactRefCount;
        
        return true;
    }

private:
    void findTypeCheckRoot(Node*, Edge edge)
    {
        // We may have an "unproved" untyped use for code that is unreachable. The CFA
        // will just not have gotten around to it.
        if (edge.isProved() || edge.useKind() == UntypedUse)
            return;
        if (!edge->postfixRef())
            m_worklist.append(edge.node());
    }
    
    void countEdge(Node*, Edge edge)
    {
        // Don't count edges that are already counted for their type checks.
        if (!(edge.isProved() || edge.useKind() == UntypedUse))
            return;
        
        if (edge->postfixRef())
            return;
        m_worklist.append(edge.node());
    }
    
    void eliminateIrrelevantPhantomChildren(Node* node)
    {
        for (unsigned i = 0; i < AdjacencyList::Size; ++i) {
            Edge edge = node->children.child(i);
            if (!edge)
                continue;
            if (edge.isProved() || edge.useKind() == UntypedUse)
                node->children.removeEdge(i--);
        }
    }
    
    Vector<Node*, 128> m_worklist;
};

bool performDCE(Graph& graph)
{
    SamplingRegion samplingRegion("DFG DCE Phase");
    return runPhase<DCEPhase>(graph);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

