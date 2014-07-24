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
#include "DFGCFGSimplificationPhase.h"

#if ENABLE(DFG_JIT)

#include "DFGAbstractState.h"
#include "DFGBasicBlockInlines.h"
#include "DFGGraph.h"
#include "DFGInsertionSet.h"
#include "DFGPhase.h"
#include "DFGValidate.h"
#include "Operations.h"

namespace JSC { namespace DFG {

class CFGSimplificationPhase : public Phase {
public:
    CFGSimplificationPhase(Graph& graph)
        : Phase(graph, "CFG simplification")
    {
    }
    
    bool run()
    {
        const bool extremeLogging = false;

        bool outerChanged = false;
        bool innerChanged;
        
        do {
            innerChanged = false;
            for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
                BasicBlock* block = m_graph.m_blocks[blockIndex].get();
                if (!block)
                    continue;
                ASSERT(block->isReachable);
            
                switch (block->last()->op()) {
                case Jump: {
                    // Successor with one predecessor -> merge.
                    if (m_graph.m_blocks[m_graph.successor(block, 0)]->m_predecessors.size() == 1) {
                        ASSERT(m_graph.m_blocks[m_graph.successor(block, 0)]->m_predecessors[0]
                               == blockIndex);
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
                        dataLogF("CFGSimplify: Jump merge on Block #%u to Block #%u.\n",
                                blockIndex, m_graph.successor(block, 0));
#endif
                        if (extremeLogging)
                            m_graph.dump();
                        m_graph.dethread();
                        mergeBlocks(blockIndex, m_graph.successor(block, 0), NoBlock);
                        innerChanged = outerChanged = true;
                        break;
                    } else {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
                        dataLogF("Not jump merging on Block #%u to Block #%u because predecessors = ",
                                blockIndex, m_graph.successor(block, 0));
                        for (unsigned i = 0; i < m_graph.m_blocks[m_graph.successor(block, 0)]->m_predecessors.size(); ++i) {
                            if (i)
                                dataLogF(", ");
                            dataLogF("#%u", m_graph.m_blocks[m_graph.successor(block, 0)]->m_predecessors[i]);
                        }
                        dataLogF(".\n");
#endif
                    }
                
                    // FIXME: Block only has a jump -> remove. This is tricky though because of
                    // liveness. What we really want is to slam in a phantom at the end of the
                    // block, after the terminal. But we can't right now. :-(
                    // Idea: what if I slam the ghosties into my successor? Nope, that's
                    // suboptimal, because if my successor has multiple predecessors then we'll
                    // be keeping alive things on other predecessor edges unnecessarily.
                    // What we really need is the notion of end-of-block ghosties!
                    break;
                }
                
                case Branch: {
                    // Branch on constant -> jettison the not-taken block and merge.
                    if (isKnownDirection(block->cfaBranchDirection)) {
                        bool condition = branchCondition(block->cfaBranchDirection);
                        BasicBlock* targetBlock = m_graph.m_blocks[
                            m_graph.successorForCondition(block, condition)].get();
                        if (targetBlock->m_predecessors.size() == 1) {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
                            dataLogF("CFGSimplify: Known condition (%s) branch merge on Block #%u to Block #%u, jettisoning Block #%u.\n",
                                    condition ? "true" : "false",
                                    blockIndex, m_graph.successorForCondition(block, condition),
                                    m_graph.successorForCondition(block, !condition));
#endif
                            if (extremeLogging)
                                m_graph.dump();
                            m_graph.dethread();
                            mergeBlocks(
                                blockIndex,
                                m_graph.successorForCondition(block, condition),
                                m_graph.successorForCondition(block, !condition));
                        } else {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
                            dataLogF("CFGSimplify: Known condition (%s) branch->jump conversion on Block #%u to Block #%u, jettisoning Block #%u.\n",
                                    condition ? "true" : "false",
                                    blockIndex, m_graph.successorForCondition(block, condition),
                                    m_graph.successorForCondition(block, !condition));
#endif
                            if (extremeLogging)
                                m_graph.dump();
                            m_graph.dethread();
                            BlockIndex takenBlockIndex = m_graph.successorForCondition(block, condition);
                            BlockIndex notTakenBlockIndex = m_graph.successorForCondition(block, !condition);
                        
                            ASSERT(block->last()->isTerminal());
                            CodeOrigin boundaryCodeOrigin = block->last()->codeOrigin;
                            block->last()->convertToPhantom();
                            ASSERT(block->last()->refCount() == 1);
                        
                            jettisonBlock(blockIndex, notTakenBlockIndex, boundaryCodeOrigin);
                        
                            block->appendNode(
                                m_graph, SpecNone, Jump, boundaryCodeOrigin,
                                OpInfo(takenBlockIndex));
                        }
                        innerChanged = outerChanged = true;
                        break;
                    }
                    
                    if (m_graph.successor(block, 0) == m_graph.successor(block, 1)) {
                        BlockIndex targetBlockIndex = m_graph.successor(block, 0);
                        BasicBlock* targetBlock = m_graph.m_blocks[targetBlockIndex].get();
                        ASSERT(targetBlock);
                        ASSERT(targetBlock->isReachable);
                        if (targetBlock->m_predecessors.size() == 1) {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
                            dataLogF("CFGSimplify: Branch to same successor merge on Block #%u to Block #%u.\n",
                                    blockIndex, targetBlockIndex);
#endif
                            m_graph.dethread();
                            mergeBlocks(blockIndex, targetBlockIndex, NoBlock);
                        } else {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
                            dataLogF("CFGSimplify: Branch->jump conversion to same successor on Block #%u to Block #%u.\n",
                                    blockIndex, targetBlockIndex);
#endif
                            Node* branch = block->last();
                            ASSERT(branch->isTerminal());
                            ASSERT(branch->op() == Branch);
                            branch->convertToPhantom();
                            ASSERT(branch->refCount() == 1);
                            
                            block->appendNode(
                                m_graph, SpecNone, Jump, branch->codeOrigin,
                                OpInfo(targetBlockIndex));
                        }
                        innerChanged = outerChanged = true;
                        break;
                    }
                    
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
                    dataLogF("Not branch simplifying on Block #%u because the successors differ and the condition is not known.\n",
                            blockIndex);
#endif
                
                    // Branch to same destination -> jump.
                    // FIXME: this will currently not be hit because of the lack of jump-only
                    // block simplification.
                    
                    break;
                }
                
                default:
                    break;
                }
            }
            
            if (innerChanged) {
                // Here's the reason for this pass:
                // Blocks: A, B, C, D, E, F
                // A -> B, C
                // B -> F
                // C -> D, E
                // D -> F
                // E -> F
                //
                // Assume that A's branch is determined to go to B. Then the rest of this phase
                // is smart enough to simplify down to:
                // A -> B
                // B -> F
                // C -> D, E
                // D -> F
                // E -> F
                //
                // We will also merge A and B. But then we don't have any other mechanism to
                // remove D, E as predecessors for F. Worse, the rest of this phase does not
                // know how to fix the Phi functions of F to ensure that they no longer refer
                // to variables in D, E. In general, we need a way to handle Phi simplification
                // upon:
                // 1) Removal of a predecessor due to branch simplification. The branch
                //    simplifier already does that.
                // 2) Invalidation of a predecessor because said predecessor was rendered
                //    unreachable. We do this here.
                //
                // This implies that when a block is unreachable, we must inspect its
                // successors' Phi functions to remove any references from them into the
                // removed block.
                
                m_graph.resetReachability();

                for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
                    BasicBlock* block = m_graph.m_blocks[blockIndex].get();
                    if (!block)
                        continue;
                    if (block->isReachable)
                        continue;
                    
                    killUnreachable(blockIndex);
                }
            }
            
            if (Options::validateGraphAtEachPhase())
                validate(m_graph);
        } while (innerChanged);
        
        return outerChanged;
    }

private:
    void killUnreachable(BlockIndex blockIndex)
    {
        BasicBlock* block = m_graph.m_blocks[blockIndex].get();
        
        ASSERT(block);
        ASSERT(!block->isReachable);
        
        for (unsigned phiIndex = block->phis.size(); phiIndex--;)
            m_graph.m_allocator.free(block->phis[phiIndex]);
        for (unsigned nodeIndex = block->size(); nodeIndex--;)
            m_graph.m_allocator.free(block->at(nodeIndex));
        
        m_graph.m_blocks[blockIndex].clear();
    }
    
    void keepOperandAlive(BasicBlock* block, BasicBlock* jettisonedBlock, CodeOrigin codeOrigin, int operand)
    {
        Node* livenessNode = jettisonedBlock->variablesAtHead.operand(operand);
        if (!livenessNode)
            return;
        if (livenessNode->variableAccessData()->isCaptured())
            return;
        block->appendNode(
            m_graph, SpecNone, PhantomLocal, codeOrigin, 
            OpInfo(livenessNode->variableAccessData()));
    }
    
    void jettisonBlock(BlockIndex blockIndex, BlockIndex jettisonedBlockIndex, CodeOrigin boundaryCodeOrigin)
    {
        BasicBlock* block = m_graph.m_blocks[blockIndex].get();
        BasicBlock* jettisonedBlock = m_graph.m_blocks[jettisonedBlockIndex].get();
        
        for (size_t i = 0; i < jettisonedBlock->variablesAtHead.numberOfArguments(); ++i)
            keepOperandAlive(block, jettisonedBlock, boundaryCodeOrigin, argumentToOperand(i));
        for (size_t i = 0; i < jettisonedBlock->variablesAtHead.numberOfLocals(); ++i)
            keepOperandAlive(block, jettisonedBlock, boundaryCodeOrigin, i);
        
        fixJettisonedPredecessors(blockIndex, jettisonedBlockIndex);
    }
    
    void fixJettisonedPredecessors(BlockIndex blockIndex, BlockIndex jettisonedBlockIndex)
    {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLogF("Fixing predecessors and phis due to jettison of Block #%u from Block #%u.\n",
                jettisonedBlockIndex, blockIndex);
#endif
        BasicBlock* jettisonedBlock = m_graph.m_blocks[jettisonedBlockIndex].get();
        for (unsigned i = 0; i < jettisonedBlock->m_predecessors.size(); ++i) {
            if (jettisonedBlock->m_predecessors[i] != blockIndex)
                continue;
            jettisonedBlock->m_predecessors[i] = jettisonedBlock->m_predecessors.last();
            jettisonedBlock->m_predecessors.removeLast();
            break;
        }
    }
    
    void mergeBlocks(
        BlockIndex firstBlockIndex, BlockIndex secondBlockIndex, BlockIndex jettisonedBlockIndex)
    {
        // This will add all of the nodes in secondBlock to firstBlock, but in so doing
        // it will also ensure that any GetLocals from the second block that refer to
        // SetLocals in the first block are relinked. If jettisonedBlock is not NoBlock,
        // then Phantoms are inserted for anything that the jettisonedBlock would have
        // kept alive.
        
        BasicBlock* firstBlock = m_graph.m_blocks[firstBlockIndex].get();
        BasicBlock* secondBlock = m_graph.m_blocks[secondBlockIndex].get();
        
        // Remove the terminal of firstBlock since we don't need it anymore. Well, we don't
        // really remove it; we actually turn it into a Phantom.
        ASSERT(firstBlock->last()->isTerminal());
        CodeOrigin boundaryCodeOrigin = firstBlock->last()->codeOrigin;
        firstBlock->last()->convertToPhantom();
        ASSERT(firstBlock->last()->refCount() == 1);
        
        if (jettisonedBlockIndex != NoBlock) {
            BasicBlock* jettisonedBlock = m_graph.m_blocks[jettisonedBlockIndex].get();
            
            // Time to insert ghosties for things that need to be kept alive in case we OSR
            // exit prior to hitting the firstBlock's terminal, and end up going down a
            // different path than secondBlock.
            
            for (size_t i = 0; i < jettisonedBlock->variablesAtHead.numberOfArguments(); ++i)
                keepOperandAlive(firstBlock, jettisonedBlock, boundaryCodeOrigin, argumentToOperand(i));
            for (size_t i = 0; i < jettisonedBlock->variablesAtHead.numberOfLocals(); ++i)
                keepOperandAlive(firstBlock, jettisonedBlock, boundaryCodeOrigin, i);
        }
        
        for (size_t i = 0; i < secondBlock->phis.size(); ++i)
            firstBlock->phis.append(secondBlock->phis[i]);

        for (size_t i = 0; i < secondBlock->size(); ++i)
            firstBlock->append(secondBlock->at(i));
        
        ASSERT(firstBlock->last()->isTerminal());
        
        // Fix the predecessors of my new successors. This is tricky, since we are going to reset
        // all predecessors anyway due to reachability analysis. But we need to fix the
        // predecessors eagerly to ensure that we know what they are in case the next block we
        // consider in this phase wishes to query the predecessors of one of the blocks we
        // affected.
        for (unsigned i = m_graph.numSuccessors(firstBlock); i--;) {
            BasicBlock* successor = m_graph.m_blocks[m_graph.successor(firstBlock, i)].get();
            for (unsigned j = 0; j < successor->m_predecessors.size(); ++j) {
                if (successor->m_predecessors[j] == secondBlockIndex)
                    successor->m_predecessors[j] = firstBlockIndex;
            }
        }
        
        // Fix the predecessors of my former successors. Again, we'd rather not do this, but it's
        // an unfortunate necessity. See above comment.
        if (jettisonedBlockIndex != NoBlock)
            fixJettisonedPredecessors(firstBlockIndex, jettisonedBlockIndex);
        
        firstBlock->valuesAtTail = secondBlock->valuesAtTail;
        firstBlock->cfaBranchDirection = secondBlock->cfaBranchDirection;
        
        m_graph.m_blocks[secondBlockIndex].clear();
    }
};

bool performCFGSimplification(Graph& graph)
{
    SamplingRegion samplingRegion("DFG CFG Simplification Phase");
    return runPhase<CFGSimplificationPhase>(graph);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)


