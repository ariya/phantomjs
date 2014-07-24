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
#include "DFGCPSRethreadingPhase.h"

#if ENABLE(DFG_JIT)

#include "DFGBasicBlockInlines.h"
#include "DFGGraph.h"
#include "DFGPhase.h"
#include "Operations.h"

namespace JSC { namespace DFG {

class CPSRethreadingPhase : public Phase {
public:
    CPSRethreadingPhase(Graph& graph)
        : Phase(graph, "CPS rethreading")
    {
    }
    
    bool run()
    {
        if (m_graph.m_form == ThreadedCPS)
            return false;
        
        clearIsLoadedFrom();
        freeUnnecessaryNodes();
        canonicalizeLocalsInBlocks();
        propagatePhis<LocalOperand>();
        propagatePhis<ArgumentOperand>();
        
        m_graph.m_form = ThreadedCPS;
        return true;
    }

private:
    
    void clearIsLoadedFrom()
    {
        for (unsigned i = 0; i < m_graph.m_variableAccessData.size(); ++i)
            m_graph.m_variableAccessData[i].setIsLoadedFrom(false);
    }
    
    void freeUnnecessaryNodes()
    {
        SamplingRegion samplingRegion("DFG CPS Rethreading: freeUnnecessaryNodes");
        
        for (BlockIndex blockIndex = m_graph.m_blocks.size(); blockIndex--;) {
            BasicBlock* block = m_graph.m_blocks[blockIndex].get();
            if (!block)
                continue;
            ASSERT(block->isReachable);
            
            unsigned fromIndex = 0;
            unsigned toIndex = 0;
            while (fromIndex < block->size()) {
                Node* node = block->at(fromIndex++);
                switch (node->op()) {
                case GetLocal:
                case Flush:
                case PhantomLocal:
                    node->children.setChild1(Edge());
                    break;
                case Phantom:
                    if (!node->child1())
                        continue;
                    switch (node->child1()->op()) {
                    case Phi:
                    case SetArgument:
                    case SetLocal:
                        node->convertToPhantomLocal();
                        break;
                    default:
                        ASSERT(node->child1()->hasResult());
                        break;
                    }
                    break;
                case Nop:
                    continue;
                default:
                    break;
                }
                node->replacement = 0; // Reset the replacement since the next phase will use it.
                block->at(toIndex++) = node;
            }
            block->resize(toIndex);
            
            for (unsigned phiIndex = block->phis.size(); phiIndex--;)
                m_graph.m_allocator.free(block->phis[phiIndex]);
            block->phis.resize(0);
        }
    }
    
    template<OperandKind operandKind>
    void clearVariablesAtHeadAndTail()
    {
        ASSERT(
            m_block->variablesAtHead.sizeFor<operandKind>()
            == m_block->variablesAtTail.sizeFor<operandKind>());
        
        for (unsigned i = m_block->variablesAtHead.sizeFor<operandKind>(); i--;) {
            m_block->variablesAtHead.atFor<operandKind>(i) = 0;
            m_block->variablesAtTail.atFor<operandKind>(i) = 0;
        }
    }
    
    ALWAYS_INLINE Node* addPhiSilently(BasicBlock* block, const CodeOrigin& codeOrigin, VariableAccessData* variable)
    {
        Node* result = m_graph.addNode(SpecNone, Phi, codeOrigin, OpInfo(variable));
        block->phis.append(result);
        return result;
    }
    
    template<OperandKind operandKind>
    ALWAYS_INLINE Node* addPhi(BasicBlock* block, const CodeOrigin& codeOrigin, VariableAccessData* variable, size_t index)
    {
        Node* result = addPhiSilently(block, codeOrigin, variable);
        phiStackFor<operandKind>().append(PhiStackEntry(block, index, result));
        return result;
    }
    
    template<OperandKind operandKind>
    ALWAYS_INLINE Node* addPhi(const CodeOrigin& codeOrigin, VariableAccessData* variable, size_t index)
    {
        return addPhi<operandKind>(m_block, codeOrigin, variable, index);
    }
    
    template<OperandKind operandKind>
    void canonicalizeGetLocalFor(Node* node, VariableAccessData* variable, size_t idx)
    {
        ASSERT(!node->child1());
        
        if (Node* otherNode = m_block->variablesAtTail.atFor<operandKind>(idx)) {
            ASSERT(otherNode->variableAccessData() == variable);
            
            switch (otherNode->op()) {
            case Flush:
            case PhantomLocal:
                otherNode = otherNode->child1().node();
                if (otherNode->op() == Phi) {
                    // We need to have a GetLocal, so this might as well be the one.
                    node->children.setChild1(Edge(otherNode));
                    m_block->variablesAtTail.atFor<operandKind>(idx) = node;
                    return;
                }
                ASSERT(otherNode->op() == SetLocal || otherNode->op() == SetArgument);
                break;
            default:
                break;
            }
            
            ASSERT(otherNode->op() == SetLocal || otherNode->op() == SetArgument || otherNode->op() == GetLocal);
            ASSERT(otherNode->variableAccessData() == variable);
            
            if (otherNode->op() == SetArgument) {
                variable->setIsLoadedFrom(true);
                node->children.setChild1(Edge(otherNode));
                m_block->variablesAtTail.atFor<operandKind>(idx) = node;
                return;
            }
            
            if (variable->isCaptured()) {
                variable->setIsLoadedFrom(true);
                if (otherNode->op() == GetLocal)
                    otherNode = otherNode->child1().node();
                else
                    ASSERT(otherNode->op() == SetLocal || otherNode->op() == SetArgument);
                
                ASSERT(otherNode->op() == Phi || otherNode->op() == SetLocal || otherNode->op() == SetArgument);
                
                // Keep this GetLocal but link it to the prior ones.
                node->children.setChild1(Edge(otherNode));
                m_block->variablesAtTail.atFor<operandKind>(idx) = node;
                return;
            }
            
            if (otherNode->op() == GetLocal) {
                // Replace all references to this GetLocal with otherNode.
                node->replacement = otherNode;
                return;
            }
            
            ASSERT(otherNode->op() == SetLocal);
            node->replacement = otherNode->child1().node();
            return;
        }
        
        variable->setIsLoadedFrom(true);
        Node* phi = addPhi<operandKind>(node->codeOrigin, variable, idx);
        node->children.setChild1(Edge(phi));
        m_block->variablesAtHead.atFor<operandKind>(idx) = phi;
        m_block->variablesAtTail.atFor<operandKind>(idx) = node;
    }
    
    void canonicalizeGetLocal(Node* node)
    {
        VariableAccessData* variable = node->variableAccessData();
        if (operandIsArgument(variable->local()))
            canonicalizeGetLocalFor<ArgumentOperand>(node, variable, operandToArgument(variable->local()));
        else
            canonicalizeGetLocalFor<LocalOperand>(node, variable, variable->local());
    }
    
    void canonicalizeSetLocal(Node* node)
    {
        m_block->variablesAtTail.setOperand(node->local(), node);
    }
    
    template<NodeType nodeType, OperandKind operandKind>
    void canonicalizeFlushOrPhantomLocalFor(Node* node, VariableAccessData* variable, size_t idx)
    {
        ASSERT(!node->child1());
        
        if (Node* otherNode = m_block->variablesAtTail.atFor<operandKind>(idx)) {
            ASSERT(otherNode->variableAccessData() == variable);
            
            switch (otherNode->op()) {
            case Flush:
            case PhantomLocal:
            case GetLocal:
                otherNode = otherNode->child1().node();
                break;
            default:
                break;
            }
            
            ASSERT(otherNode->op() == Phi || otherNode->op() == SetLocal || otherNode->op() == SetArgument);
            
            if (nodeType == PhantomLocal && otherNode->op() == SetLocal) {
                // PhantomLocal(SetLocal) doesn't make sense. PhantomLocal means: at this
                // point I know I would have been interested in the value of this variable
                // for the purpose of OSR. PhantomLocal(SetLocal) means: at this point I
                // know that I would have read the value written by that SetLocal. This is
                // redundant and inefficient, since really it just means that we want to
                // be keeping the operand to the SetLocal alive. The SetLocal may die, and
                // we'll be fine because OSR tracks dead SetLocals.
                
                // So we turn this into a Phantom on the child of the SetLocal.
                
                node->convertToPhantom();
                node->children.setChild1(otherNode->child1());
                return;
            }
            
            variable->setIsLoadedFrom(true);
            // There is nothing wrong with having redundant Flush's. It just needs to
            // be linked appropriately. Note that if there had already been a previous
            // use at tail then we don't override it. It's fine for variablesAtTail to
            // omit Flushes and PhantomLocals. On the other hand, having it refer to a
            // Flush or a PhantomLocal if just before it the last use was a GetLocal would
            // seriously confuse the CFA.
            node->children.setChild1(Edge(otherNode));
            return;
        }
        
        variable->setIsLoadedFrom(true);
        node->children.setChild1(Edge(addPhi<operandKind>(node->codeOrigin, variable, idx)));
        m_block->variablesAtHead.atFor<operandKind>(idx) = node;
        m_block->variablesAtTail.atFor<operandKind>(idx) = node;
    }

    template<NodeType nodeType>
    void canonicalizeFlushOrPhantomLocal(Node* node)
    {
        VariableAccessData* variable = node->variableAccessData();
        if (operandIsArgument(variable->local()))
            canonicalizeFlushOrPhantomLocalFor<nodeType, ArgumentOperand>(node, variable, operandToArgument(variable->local()));
        else
            canonicalizeFlushOrPhantomLocalFor<nodeType, LocalOperand>(node, variable, variable->local());
    }
    
    void canonicalizeSetArgument(Node* node)
    {
        int local = node->local();
        ASSERT(operandIsArgument(local));
        int argument = operandToArgument(local);
        m_block->variablesAtHead.setArgumentFirstTime(argument, node);
        m_block->variablesAtTail.setArgumentFirstTime(argument, node);
    }
    
    void canonicalizeLocalsInBlock()
    {
        if (!m_block)
            return;
        ASSERT(m_block->isReachable);
        
        clearVariablesAtHeadAndTail<ArgumentOperand>();
        clearVariablesAtHeadAndTail<LocalOperand>();
        
        // Assumes that all phi references have been removed. Assumes that things that
        // should be live have a non-zero ref count, but doesn't assume that the ref
        // counts are correct beyond that (more formally !!logicalRefCount == !!actualRefCount
        // but not logicalRefCount == actualRefCount). Assumes that it can break ref
        // counts.
        
        for (unsigned nodeIndex = 0; nodeIndex < m_block->size(); ++nodeIndex) {
            Node* node = m_block->at(nodeIndex);
            
            m_graph.performSubstitution(node);
            
            // The rules for threaded CPS form:
            // 
            // Head variable: describes what is live at the head of the basic block.
            // Head variable links may refer to Flush, PhantomLocal, Phi, or SetArgument.
            // SetArgument may only appear in the root block.
            //
            // Tail variable: the last thing that happened to the variable in the block.
            // It may be a Flush, PhantomLocal, GetLocal, SetLocal, or SetArgument.
            // SetArgument may only appear in the root block. Note that if there ever
            // was a GetLocal to the variable, and it was followed by PhantomLocals and
            // Flushes but not SetLocals, then the tail variable will be the GetLocal.
            // This reflects the fact that you only care that the tail variable is a
            // Flush or PhantomLocal if nothing else interesting happened.
            //
            // Child of GetLocal: the operation that the GetLocal keeps alive. For
            // uncaptured locals, it may be a Phi from the current block. For arguments,
            // it may be a SetArgument. For captured locals and arguments it may also be
            // a SetLocal.
            //
            // Child of SetLocal: must be a value producing node.
            //
            // Child of Flush: it may be a Phi from the current block or a SetLocal. For
            // arguments it may also be a SetArgument.
            //
            // Child of PhantomLocal: it may be a Phi from the current block. For
            // arguments it may also be a SetArgument.
            //
            // Children of Phi: other Phis in the same basic block, or any of the
            // following from predecessor blocks: SetLocal, Phi, or SetArgument. These
            // are computed by looking at the tail variables of the predecessor  blocks
            // and either using it directly (if it's a SetLocal, Phi, or SetArgument) or
            // loading that nodes child (if it's a GetLocal, PhanomLocal, or Flush - all
            // of these will have children that are SetLocal, Phi, or SetArgument).
            
            switch (node->op()) {
            case GetLocal:
                canonicalizeGetLocal(node);
                break;
                
            case SetLocal:
                canonicalizeSetLocal(node);
                break;
                
            case Flush:
                canonicalizeFlushOrPhantomLocal<Flush>(node);
                break;
                
            case PhantomLocal:
                canonicalizeFlushOrPhantomLocal<PhantomLocal>(node);
                break;
                
            case SetArgument:
                canonicalizeSetArgument(node);
                break;
                
            default:
                break;
            }
        }
    }
    
    void canonicalizeLocalsInBlocks()
    {
        SamplingRegion samplingRegion("DFG CPS Rethreading: canonicalizeLocalsInBlocks");
        
        for (m_blockIndex = m_graph.m_blocks.size(); m_blockIndex--;) {
            m_block = m_graph.m_blocks[m_blockIndex].get();
            canonicalizeLocalsInBlock();
        }
    }
    
    template<OperandKind operandKind>
    void propagatePhis()
    {
        Vector<PhiStackEntry, 128>& phiStack = operandKind == ArgumentOperand ? m_argumentPhiStack : m_localPhiStack;
        
        SamplingRegion samplingRegion("DFG CPS Rethreading: propagatePhis");
        
        // Ensure that attempts to use this fail instantly.
        m_block = 0;
        m_blockIndex = NoBlock;
        
        while (!phiStack.isEmpty()) {
            PhiStackEntry entry = phiStack.last();
            phiStack.removeLast();
            
            BasicBlock* block = entry.m_block;
            PredecessorList& predecessors = block->m_predecessors;
            Node* currentPhi = entry.m_phi;
            VariableAccessData* variable = currentPhi->variableAccessData();
            size_t index = entry.m_index;
            
            for (size_t i = predecessors.size(); i--;) {
                BasicBlock* predecessorBlock = m_graph.m_blocks[predecessors[i]].get();
                
                Node* variableInPrevious = predecessorBlock->variablesAtTail.atFor<operandKind>(index);
                if (!variableInPrevious) {
                    variableInPrevious = addPhi<operandKind>(predecessorBlock, currentPhi->codeOrigin, variable, index);
                    predecessorBlock->variablesAtTail.atFor<operandKind>(index) = variableInPrevious;
                    predecessorBlock->variablesAtHead.atFor<operandKind>(index) = variableInPrevious;
                } else {
                    switch (variableInPrevious->op()) {
                    case GetLocal:
                    case PhantomLocal:
                    case Flush:
                        ASSERT(variableInPrevious->variableAccessData() == variableInPrevious->child1()->variableAccessData());
                        variableInPrevious = variableInPrevious->child1().node();
                        break;
                    default:
                        break;
                    }
                }
                
                ASSERT(
                    variableInPrevious->op() == SetLocal
                    || variableInPrevious->op() == Phi
                    || variableInPrevious->op() == SetArgument);
          
                if (!currentPhi->child1()) {
                    currentPhi->children.setChild1(Edge(variableInPrevious));
                    continue;
                }
                if (!currentPhi->child2()) {
                    currentPhi->children.setChild2(Edge(variableInPrevious));
                    continue;
                }
                if (!currentPhi->child3()) {
                    currentPhi->children.setChild3(Edge(variableInPrevious));
                    continue;
                }
                
                Node* newPhi = addPhiSilently(block, currentPhi->codeOrigin, variable);
                newPhi->children = currentPhi->children;
                currentPhi->children.initialize(newPhi, variableInPrevious, 0);
            }
        }
    }
    
    struct PhiStackEntry {
        PhiStackEntry(BasicBlock* block, size_t index, Node* phi)
            : m_block(block)
            , m_index(index)
            , m_phi(phi)
        {
        }
        
        BasicBlock* m_block;
        size_t m_index;
        Node* m_phi;
    };
    
    template<OperandKind operandKind>
    Vector<PhiStackEntry, 128>& phiStackFor()
    {
        if (operandKind == ArgumentOperand)
            return m_argumentPhiStack;
        return m_localPhiStack;
    }
    
    BlockIndex m_blockIndex;
    BasicBlock* m_block;
    Vector<PhiStackEntry, 128> m_argumentPhiStack;
    Vector<PhiStackEntry, 128> m_localPhiStack;
};

bool performCPSRethreading(Graph& graph)
{
    SamplingRegion samplingRegion("DFG CPS Rethreading Phase");
    return runPhase<CPSRethreadingPhase>(graph);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

