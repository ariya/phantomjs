/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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
#include "DFGValidate.h"

#if ENABLE(DFG_JIT)

#include "CodeBlockWithJITType.h"
#include <wtf/Assertions.h>
#include <wtf/BitVector.h>

namespace JSC { namespace DFG {

class Validate {
public:
    Validate(Graph& graph, GraphDumpMode graphDumpMode)
        : m_graph(graph)
        , m_graphDumpMode(graphDumpMode)
    {
    }
    
    #define VALIDATE(context, assertion) do { \
        if (!(assertion)) { \
            dataLogF("\n\n\nAt "); \
            reportValidationContext context; \
            dataLogF(": validation %s (%s:%d) failed.\n", #assertion, __FILE__, __LINE__); \
            dumpGraphIfAppropriate(); \
            WTFReportAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #assertion); \
            CRASH(); \
        } \
    } while (0)
    
    #define V_EQUAL(context, left, right) do { \
        if (left != right) { \
            dataLogF("\n\n\nAt "); \
            reportValidationContext context; \
            dataLogF(": validation (%s = ", #left); \
            dataLog(left); \
            dataLogF(") == (%s = ", #right); \
            dataLog(right); \
            dataLogF(") (%s:%d) failed.\n", __FILE__, __LINE__); \
            dumpGraphIfAppropriate(); \
            WTFReportAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #left " == " #right); \
            CRASH(); \
        } \
    } while (0)

    #define notSet (static_cast<size_t>(-1))

    void validate()
    {
        // NB. This code is not written for performance, since it is not intended to run
        // in release builds.
        
        // Validate that all local variables at the head of the root block are dead.
        BasicBlock* root = m_graph.m_blocks[0].get();
        for (unsigned i = 0; i < root->variablesAtHead.numberOfLocals(); ++i)
            V_EQUAL((static_cast<VirtualRegister>(i), 0), static_cast<Node*>(0), root->variablesAtHead.local(i));
        
        // Validate ref counts and uses.
        HashMap<Node*, unsigned> myRefCounts;
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
            BasicBlock* block = m_graph.m_blocks[blockIndex].get();
            if (!block || !block->isReachable)
                continue;
            for (size_t i = 0; i < block->numNodes(); ++i)
                myRefCounts.add(block->node(i), 0);
        }
        HashSet<Node*> acceptableNodes;
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
            BasicBlock* block = m_graph.m_blocks[blockIndex].get();
            if (!block || !block->isReachable)
                continue;
            for (size_t i = 0; i < block->numNodes(); ++i) {
                Node* node = block->node(i);
                acceptableNodes.add(node);
                if (!node->shouldGenerate())
                    continue;
                for (unsigned j = 0; j < m_graph.numChildren(node); ++j) {
                    // Phi children in LoadStore form are invalid.
                    if (m_graph.m_form == LoadStore && block->isPhiIndex(i))
                        continue;
                    
                    Edge edge = m_graph.child(node, j);
                    if (!edge)
                        continue;
                    
                    myRefCounts.find(edge.node())->value++;
                    
                    // Unless I'm a Flush, Phantom, GetLocal, or Phi, my children should hasResult().
                    switch (node->op()) {
                    case Flush:
                    case GetLocal:
                        VALIDATE((node, edge), edge->hasVariableAccessData());
                        VALIDATE((node, edge), edge->variableAccessData() == node->variableAccessData());
                        break;
                    case PhantomLocal:
                        VALIDATE((node, edge), edge->hasVariableAccessData());
                        VALIDATE((node, edge), edge->variableAccessData() == node->variableAccessData());
                        VALIDATE((node, edge), edge->op() != SetLocal);
                        break;
                    case Phi:
                        VALIDATE((node, edge), edge->hasVariableAccessData());
                        if (m_graph.m_unificationState == LocallyUnified)
                            break;
                        VALIDATE((node, edge), edge->variableAccessData() == node->variableAccessData());
                        break;
                    case Phantom:
                        switch (m_graph.m_form) {
                        case LoadStore:
                            if (j) {
                                VALIDATE((node, edge), edge->hasResult());
                                break;
                            }
                            switch (edge->op()) {
                            case Phi:
                            case SetArgument:
                            case SetLocal:
                                break;
                            default:
                                VALIDATE((node, edge), edge->hasResult());
                                break;
                            }
                            break;
                        case ThreadedCPS:
                            VALIDATE((node, edge), edge->hasResult());
                            break;
                        }
                        break;
                    default:
                        VALIDATE((node, edge), edge->hasResult());
                        break;
                    }
                }
            }
        }
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
            BasicBlock* block = m_graph.m_blocks[blockIndex].get();
            if (!block || !block->isReachable)
                continue;
            
            HashSet<Node*> phisInThisBlock;
            HashSet<Node*> nodesInThisBlock;
            
            for (size_t i = 0; i < block->numNodes(); ++i) {
                Node* node = block->node(i);
                nodesInThisBlock.add(node);
                if (block->isPhiIndex(i))
                    phisInThisBlock.add(node);
                if (m_graph.m_refCountState == ExactRefCount)
                    V_EQUAL((node), myRefCounts.get(node), node->adjustedRefCount());
                else
                    V_EQUAL((node), node->refCount(), 1);
                for (unsigned j = 0; j < m_graph.numChildren(node); ++j) {
                    Edge edge = m_graph.child(node, j);
                    if (!edge)
                        continue;
                    VALIDATE((node, edge), acceptableNodes.contains(edge.node()));
                }
            }
            
            for (size_t i = 0; i < block->phis.size(); ++i) {
                Node* node = block->phis[i];
                ASSERT(phisInThisBlock.contains(node));
                VALIDATE((node), node->op() == Phi);
                VirtualRegister local = node->local();
                for (unsigned j = 0; j < m_graph.numChildren(node); ++j) {
                    // Phi children in LoadStore form are invalid.
                    if (m_graph.m_form == LoadStore && block->isPhiIndex(i))
                        continue;
                    
                    Edge edge = m_graph.child(node, j);
                    if (!edge)
                        continue;
                    
                    VALIDATE(
                        (node, edge),
                        edge->op() == SetLocal
                        || edge->op() == SetArgument
                        || edge->op() == Flush
                        || edge->op() == Phi
                        || edge->op() == ZombieHint
                        || edge->op() == MovHint
                        || edge->op() == MovHintAndCheck);
                    
                    if (phisInThisBlock.contains(edge.node()))
                        continue;
                    
                    if (nodesInThisBlock.contains(edge.node())) {
                        VALIDATE(
                            (node, edge),
                            edge->op() == SetLocal
                            || edge->op() == ZombieHint
                            || edge->op() == MovHint
                            || edge->op() == MovHintAndCheck
                            || edge->op() == SetArgument
                            || edge->op() == Flush);
                        
                        continue;
                    }
                    
                    // There must exist a predecessor block that has this node index in
                    // its tail variables.
                    bool found = false;
                    for (unsigned k = 0; k < block->m_predecessors.size(); ++k) {
                        BasicBlock* prevBlock = m_graph.m_blocks[block->m_predecessors[k]].get();
                        VALIDATE((Block, block->m_predecessors[k]), prevBlock);
                        VALIDATE((Block, block->m_predecessors[k]), prevBlock->isReachable);
                        Node* prevNode = prevBlock->variablesAtTail.operand(local);
                        // If we have a Phi that is not referring to *this* block then all predecessors
                        // must have that local available.
                        VALIDATE((local, blockIndex, Block, block->m_predecessors[k]), prevNode);
                        switch (prevNode->op()) {
                        case GetLocal:
                        case Flush:
                        case PhantomLocal:
                            prevNode = prevNode->child1().node();
                            break;
                        default:
                            break;
                        }
                        if (node->shouldGenerate()) {
                            VALIDATE((local, block->m_predecessors[k], prevNode),
                                     prevNode->shouldGenerate());
                        }
                        VALIDATE(
                            (local, block->m_predecessors[k], prevNode),
                            prevNode->op() == SetLocal
                            || prevNode->op() == MovHint
                            || prevNode->op() == MovHintAndCheck
                            || prevNode->op() == ZombieHint
                            || prevNode->op() == SetArgument
                            || prevNode->op() == Phi);
                        if (prevNode == edge.node()) {
                            found = true;
                            break;
                        }
                        // At this point it cannot refer into this block.
                        VALIDATE((local, block->m_predecessors[k], prevNode), !prevBlock->isInBlock(edge.node()));
                    }
                    
                    VALIDATE((node, edge), found);
                }
            }
            
            Operands<size_t> getLocalPositions(
                block->variablesAtHead.numberOfArguments(),
                block->variablesAtHead.numberOfLocals());
            Operands<size_t> setLocalPositions(
                block->variablesAtHead.numberOfArguments(),
                block->variablesAtHead.numberOfLocals());
            
            for (size_t i = 0; i < block->variablesAtHead.numberOfArguments(); ++i) {
                VALIDATE((static_cast<VirtualRegister>(argumentToOperand(i)), blockIndex), !block->variablesAtHead.argument(i) || block->variablesAtHead.argument(i)->hasVariableAccessData());
                if (m_graph.m_form == ThreadedCPS)
                    VALIDATE((static_cast<VirtualRegister>(argumentToOperand(i)), blockIndex), !block->variablesAtTail.argument(i) || block->variablesAtTail.argument(i)->hasVariableAccessData());
                
                getLocalPositions.argument(i) = notSet;
                setLocalPositions.argument(i) = notSet;
            }
            for (size_t i = 0; i < block->variablesAtHead.numberOfLocals(); ++i) {
                VALIDATE((static_cast<VirtualRegister>(i), blockIndex), !block->variablesAtHead.local(i) || block->variablesAtHead.local(i)->hasVariableAccessData());
                if (m_graph.m_form == ThreadedCPS)
                    VALIDATE((static_cast<VirtualRegister>(i), blockIndex), !block->variablesAtTail.local(i) || block->variablesAtTail.local(i)->hasVariableAccessData());

                getLocalPositions.local(i) = notSet;
                setLocalPositions.local(i) = notSet;
            }
            
            for (size_t i = 0; i < block->size(); ++i) {
                Node* node = block->at(i);
                ASSERT(nodesInThisBlock.contains(node));
                VALIDATE((node), node->op() != Phi);
                for (unsigned j = 0; j < m_graph.numChildren(node); ++j) {
                    Edge edge = m_graph.child(node, j);
                    if (!edge)
                        continue;
                    VALIDATE((node, edge), nodesInThisBlock.contains(edge.node()));
                    switch (node->op()) {
                    case PhantomLocal:
                    case GetLocal:
                    case Flush:
                        break;
                    case Phantom:
                        if (m_graph.m_form == LoadStore && !j)
                            break;
                    default:
                        VALIDATE((node, edge), !phisInThisBlock.contains(edge.node()));
                        break;
                    }
                }
                
                if (!node->shouldGenerate())
                    continue;
                switch (node->op()) {
                case GetLocal:
                    if (node->variableAccessData()->isCaptured())
                        break;
                    // Ignore GetLocal's that we know to be dead, but that the graph
                    // doesn't yet know to be dead.
                    if (!myRefCounts.get(node))
                        break;
                    if (m_graph.m_form == ThreadedCPS)
                        VALIDATE((node, blockIndex), getLocalPositions.operand(node->local()) == notSet);
                    getLocalPositions.operand(node->local()) = i;
                    break;
                case SetLocal:
                    if (node->variableAccessData()->isCaptured())
                        break;
                    // Only record the first SetLocal. There may be multiple SetLocals
                    // because of flushing.
                    if (setLocalPositions.operand(node->local()) != notSet)
                        break;
                    setLocalPositions.operand(node->local()) = i;
                    break;
                default:
                    break;
                }
            }
            
            if (m_graph.m_form == LoadStore)
                continue;
            
            for (size_t i = 0; i < block->variablesAtHead.numberOfArguments(); ++i) {
                checkOperand(
                    blockIndex, getLocalPositions, setLocalPositions, argumentToOperand(i));
            }
            for (size_t i = 0; i < block->variablesAtHead.numberOfLocals(); ++i) {
                checkOperand(
                    blockIndex, getLocalPositions, setLocalPositions, i);
            }
        }
    }
    
private:
    Graph& m_graph;
    GraphDumpMode m_graphDumpMode;
    
    void checkOperand(
        BlockIndex blockIndex, Operands<size_t>& getLocalPositions,
        Operands<size_t>& setLocalPositions, int operand)
    {
        if (getLocalPositions.operand(operand) == notSet)
            return;
        if (setLocalPositions.operand(operand) == notSet)
            return;
        
        BasicBlock* block = m_graph.m_blocks[blockIndex].get();
        
        VALIDATE(
            (block->at(getLocalPositions.operand(operand)),
             block->at(setLocalPositions.operand(operand)),
             blockIndex),
            getLocalPositions.operand(operand) < setLocalPositions.operand(operand));
    }
    
    void reportValidationContext(Node* node)
    {
        dataLogF("@%u", node->index());
    }
    
    enum BlockTag { Block };
    void reportValidationContext(BlockTag, BlockIndex blockIndex)
    {
        dataLogF("Block #%u", blockIndex);
    }
    
    void reportValidationContext(Node* node, Edge edge)
    {
        dataLog(node, " -> ", edge);
    }
    
    void reportValidationContext(VirtualRegister local, BlockIndex blockIndex)
    {
        dataLogF("r%d in Block #%u", local, blockIndex);
    }
    
    void reportValidationContext(
        VirtualRegister local, BlockIndex sourceBlockIndex, BlockTag, BlockIndex destinationBlockIndex)
    {
        dataLogF("r%d in Block #%u -> #%u", local, sourceBlockIndex, destinationBlockIndex);
    }
    
    void reportValidationContext(
        VirtualRegister local, BlockIndex sourceBlockIndex, Node* prevNode)
    {
        dataLogF("@%u for r%d in Block #%u", prevNode->index(), local, sourceBlockIndex);
    }
    
    void reportValidationContext(
        Node* node, BlockIndex blockIndex)
    {
        dataLogF("@%u in Block #%u", node->index(), blockIndex);
    }
    
    void reportValidationContext(
        Node* node, Node* node2, BlockIndex blockIndex)
    {
        dataLogF("@%u and @%u in Block #%u", node->index(), node2->index(), blockIndex);
    }
    
    void reportValidationContext(
        Node* node, BlockIndex blockIndex, Node* expectedNode, Edge incomingEdge)
    {
        dataLog(node, " in Block #", blockIndex, ", searching for ", expectedNode, " from ", incomingEdge);
    }
    
    void dumpGraphIfAppropriate()
    {
        if (m_graphDumpMode == DontDumpGraph)
            return;
        dataLog("At time of failure:\n");
        m_graph.dump();
    }
};

void validate(Graph& graph, GraphDumpMode graphDumpMode)
{
    Validate validationObject(graph, graphDumpMode);
    validationObject.validate();
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

