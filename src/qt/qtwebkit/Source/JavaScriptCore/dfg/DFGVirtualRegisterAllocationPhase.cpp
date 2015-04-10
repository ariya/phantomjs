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
#include "DFGVirtualRegisterAllocationPhase.h"

#if ENABLE(DFG_JIT)

#include "DFGGraph.h"
#include "DFGScoreBoard.h"
#include "JSCellInlines.h"

namespace JSC { namespace DFG {

class VirtualRegisterAllocationPhase : public Phase {
public:
    VirtualRegisterAllocationPhase(Graph& graph)
        : Phase(graph, "virtual register allocation")
    {
    }
    
    bool run()
    {
#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLogF("Preserved vars: ");
        m_graph.m_preservedVars.dump(WTF::dataFile());
        dataLogF("\n");
#endif
        ScoreBoard scoreBoard(m_graph.m_preservedVars);
        scoreBoard.assertClear();
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        bool needsNewLine = false;
#endif
        for (size_t blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
            BasicBlock* block = m_graph.m_blocks[blockIndex].get();
            if (!block)
                continue;
            if (!block->isReachable)
                continue;
            for (size_t indexInBlock = 0; indexInBlock < block->size(); ++indexInBlock) {
                Node* node = block->at(indexInBlock);
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
                if (needsNewLine)
                    dataLogF("\n");
                dataLogF("   @%u:", node->index());
                needsNewLine = true;
#endif
        
                if (!node->shouldGenerate())
                    continue;
                
                switch (node->op()) {
                case Phi:
                case Flush:
                case PhantomLocal:
                    continue;
                case GetLocal:
                    ASSERT(!node->child1()->hasResult());
                    break;
                default:
                    break;
                }
                
                // First, call use on all of the current node's children, then
                // allocate a VirtualRegister for this node. We do so in this
                // order so that if a child is on its last use, and a
                // VirtualRegister is freed, then it may be reused for node.
                if (node->flags() & NodeHasVarArgs) {
                    for (unsigned childIdx = node->firstChild(); childIdx < node->firstChild() + node->numChildren(); childIdx++)
                        scoreBoard.useIfHasResult(m_graph.m_varArgChildren[childIdx]);
                } else {
                    scoreBoard.useIfHasResult(node->child1());
                    scoreBoard.useIfHasResult(node->child2());
                    scoreBoard.useIfHasResult(node->child3());
                }

                if (!node->hasResult())
                    continue;

                VirtualRegister virtualRegister = scoreBoard.allocate();
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
                dataLogF(
                    " Assigning virtual register %u to node %u.",
                    virtualRegister, node->index());
#endif
                node->setVirtualRegister(virtualRegister);
                // 'mustGenerate' nodes have their useCount artificially elevated,
                // call use now to account for this.
                if (node->mustGenerate())
                    scoreBoard.use(node);
            }
            scoreBoard.assertClear();
        }
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        if (needsNewLine)
            dataLogF("\n");
#endif

        // 'm_numCalleeRegisters' is the number of locals and temporaries allocated
        // for the function (and checked for on entry). Since we perform a new and
        // different allocation of temporaries, more registers may now be required.
        unsigned calleeRegisters = scoreBoard.highWatermark() + m_graph.m_parameterSlots;
        size_t inlineCallFrameCount = codeBlock()->inlineCallFrames().size();
        for (size_t i = 0; i < inlineCallFrameCount; i++) {
            InlineCallFrame& inlineCallFrame = codeBlock()->inlineCallFrames()[i];
            CodeBlock* codeBlock = baselineCodeBlockForInlineCallFrame(&inlineCallFrame);
            unsigned requiredCalleeRegisters = inlineCallFrame.stackOffset + codeBlock->m_numCalleeRegisters;
            if (requiredCalleeRegisters > calleeRegisters)
                calleeRegisters = requiredCalleeRegisters;
        }
        if ((unsigned)codeBlock()->m_numCalleeRegisters < calleeRegisters)
            codeBlock()->m_numCalleeRegisters = calleeRegisters;
#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLogF("Num callee registers: %u\n", calleeRegisters);
#endif
        
        return true;
    }
};

bool performVirtualRegisterAllocation(Graph& graph)
{
    SamplingRegion samplingRegion("DFG Virtual Register Allocation Phase");
    return runPhase<VirtualRegisterAllocationPhase>(graph);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)
