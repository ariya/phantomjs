/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "DFGTypeCheckHoistingPhase.h"

#if ENABLE(DFG_JIT)

#include "DFGBasicBlock.h"
#include "DFGGraph.h"
#include "DFGInsertionSet.h"
#include "DFGPhase.h"
#include "DFGVariableAccessDataDump.h"
#include "Operations.h"
#include <wtf/HashMap.h>

namespace JSC { namespace DFG {

enum CheckBallot { VoteOther, VoteStructureCheck };

class TypeCheckHoistingPhase : public Phase {
public:
    TypeCheckHoistingPhase(Graph& graph)
        : Phase(graph, "structure check hoisting")
    {
    }
    
    bool run()
    {
        ASSERT(m_graph.m_form == ThreadedCPS);
        
        for (unsigned i = m_graph.m_variableAccessData.size(); i--;) {
            VariableAccessData* variable = &m_graph.m_variableAccessData[i];
            if (!variable->isRoot())
                continue;
            variable->clearVotes();
        }
        
        // Identify the set of variables that are always subject to the same structure
        // checks. For now, only consider monomorphic structure checks (one structure).
        
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
            BasicBlock* block = m_graph.m_blocks[blockIndex].get();
            if (!block)
                continue;
            for (unsigned indexInBlock = 0; indexInBlock < block->size(); ++indexInBlock) {
                Node* node = block->at(indexInBlock);
                switch (node->op()) {
                case CheckStructure:
                case StructureTransitionWatchpoint: {
                    Node* child = node->child1().node();
                    if (child->op() != GetLocal)
                        break;
                    VariableAccessData* variable = child->variableAccessData();
                    variable->vote(VoteStructureCheck);
                    if (!shouldConsiderForHoisting(variable))
                        break;
                    noticeStructureCheck(variable, node->structureSet());
                    break;
                }
                    
                case ForwardCheckStructure:
                case ForwardStructureTransitionWatchpoint:
                    // We currently rely on the fact that we're the only ones who would
                    // insert this node.
                    RELEASE_ASSERT_NOT_REACHED();
                    break;
                    
                case GetByOffset:
                case PutByOffset:
                case PutStructure:
                case AllocatePropertyStorage:
                case ReallocatePropertyStorage:
                case GetButterfly:
                case GetByVal:
                case PutByVal:
                case PutByValAlias:
                case GetArrayLength:
                case CheckArray:
                case GetIndexedPropertyStorage:
                case Phantom:
                    // Don't count these uses.
                    break;
                    
                case ArrayifyToStructure:
                case Arrayify:
                    if (node->arrayMode().conversion() == Array::RageConvert) {
                        // Rage conversion changes structures. We should avoid tying to do
                        // any kind of hoisting when rage conversion is in play.
                        Node* child = node->child1().node();
                        if (child->op() != GetLocal)
                            break;
                        VariableAccessData* variable = child->variableAccessData();
                        variable->vote(VoteOther);
                        if (!shouldConsiderForHoisting(variable))
                            break;
                        noticeStructureCheck(variable, 0);
                    }
                    break;
                    
                case SetLocal: {
                    // Find all uses of the source of the SetLocal. If any of them are a
                    // kind of CheckStructure, then we should notice them to ensure that
                    // we're not hoisting a check that would contravene checks that are
                    // already being performed.
                    VariableAccessData* variable = node->variableAccessData();
                    if (!shouldConsiderForHoisting(variable))
                        break;
                    Node* source = node->child1().node();
                    for (unsigned subIndexInBlock = 0; subIndexInBlock < block->size(); ++subIndexInBlock) {
                        Node* subNode = block->at(subIndexInBlock);
                        switch (subNode->op()) {
                        case CheckStructure: {
                            if (subNode->child1() != source)
                                break;
                            
                            noticeStructureCheck(variable, subNode->structureSet());
                            break;
                        }
                        case StructureTransitionWatchpoint: {
                            if (subNode->child1() != source)
                                break;
                            
                            noticeStructureCheck(variable, subNode->structure());
                            break;
                        }
                        default:
                            break;
                        }
                    }
                    
                    m_graph.voteChildren(node, VoteOther);
                    break;
                }
                case GarbageValue:
                    break;
                    
                default:
                    m_graph.voteChildren(node, VoteOther);
                    break;
                }
            }
        }
        
        // Disable structure hoisting on variables that appear to mostly be used in
        // contexts where it doesn't make sense.
        
        for (unsigned i = m_graph.m_variableAccessData.size(); i--;) {
            VariableAccessData* variable = &m_graph.m_variableAccessData[i];
            if (!variable->isRoot())
                continue;
            if (variable->voteRatio() >= Options::structureCheckVoteRatioForHoisting())
                continue;
            HashMap<VariableAccessData*, CheckData>::iterator iter = m_map.find(variable);
            if (iter == m_map.end())
                continue;
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
            dataLog(
                "Zeroing the structure to hoist for ", VariableAccessDataDump(m_graph, variable),
                " because the ratio is ", variable->voteRatio(), ".\n");
#endif
            iter->value.m_structure = 0;
        }
        
        // Disable structure check hoisting for variables that cross the OSR entry that
        // we're currently taking, and where the value currently does not have the
        // structure we want.
        
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
            BasicBlock* block = m_graph.m_blocks[blockIndex].get();
            if (!block)
                continue;
            ASSERT(block->isReachable);
            if (!block->isOSRTarget)
                continue;
            if (block->bytecodeBegin != m_graph.m_osrEntryBytecodeIndex)
                continue;
            for (size_t i = 0; i < m_graph.m_mustHandleValues.size(); ++i) {
                int operand = m_graph.m_mustHandleValues.operandForIndex(i);
                Node* node = block->variablesAtHead.operand(operand);
                if (!node)
                    continue;
                VariableAccessData* variable = node->variableAccessData();
                HashMap<VariableAccessData*, CheckData>::iterator iter = m_map.find(variable);
                if (iter == m_map.end())
                    continue;
                if (!iter->value.m_structure)
                    continue;
                JSValue value = m_graph.m_mustHandleValues[i];
                if (!value || !value.isCell()) {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
                    dataLog(
                        "Zeroing the structure to hoist for ", VariableAccessDataDump(m_graph, variable),
                        " because the OSR entry value is not a cell: ", value, ".\n");
#endif
                    iter->value.m_structure = 0;
                    continue;
                }
                if (value.asCell()->structure() != iter->value.m_structure) {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
                    dataLog(
                        "Zeroing the structure to hoist for ", VariableAccessDataDump(m_graph, variable),
                        " because the OSR entry value has structure ",
                        RawPointer(value.asCell()->structure()), " and we wanted ",
                        RawPointer(iter->value.m_structure), ".\n");
#endif
                    iter->value.m_structure = 0;
                    continue;
                }
            }
        }

        bool changed = false;

#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        for (HashMap<VariableAccessData*, CheckData>::iterator it = m_map.begin();
            it != m_map.end(); ++it) {
            if (!it->value.m_structure) {
                dataLog(
                    "Not hoisting checks for ", VariableAccessDataDump(m_graph, it->key),
                    " because of heuristics.\n");
                continue;
            }
            dataLog("Hoisting checks for ", VariableAccessDataDump(m_graph, it->key), "\n");
        }
#endif // DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        
        // Place CheckStructure's at SetLocal sites.
        
        InsertionSet insertionSet(m_graph);
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
            BasicBlock* block = m_graph.m_blocks[blockIndex].get();
            if (!block)
                continue;
            for (unsigned indexInBlock = 0; indexInBlock < block->size(); ++indexInBlock) {
                Node* node = block->at(indexInBlock);
                // Be careful not to use 'node' after appending to the graph. In those switch
                // cases where we need to append, we first carefully extract everything we need
                // from the node, before doing any appending.
                switch (node->op()) {
                case SetArgument: {
                    ASSERT(!blockIndex);
                    // Insert a GetLocal and a CheckStructure immediately following this
                    // SetArgument, if the variable was a candidate for structure hoisting.
                    // If the basic block previously only had the SetArgument as its
                    // variable-at-tail, then replace it with this GetLocal.
                    VariableAccessData* variable = node->variableAccessData();
                    HashMap<VariableAccessData*, CheckData>::iterator iter = m_map.find(variable);
                    if (iter == m_map.end())
                        break;
                    if (!iter->value.m_structure)
                        break;
                    
                    CodeOrigin codeOrigin = node->codeOrigin;
                    
                    Node* getLocal = insertionSet.insertNode(
                        indexInBlock + 1, variable->prediction(), GetLocal, codeOrigin,
                        OpInfo(variable), Edge(node));
                    insertionSet.insertNode(
                        indexInBlock + 1, SpecNone, CheckStructure, codeOrigin,
                        OpInfo(m_graph.addStructureSet(iter->value.m_structure)),
                        Edge(getLocal, CellUse));

                    if (block->variablesAtTail.operand(variable->local()) == node)
                        block->variablesAtTail.operand(variable->local()) = getLocal;
                    
                    m_graph.substituteGetLocal(*block, indexInBlock, variable, getLocal);
                    
                    changed = true;
                    break;
                }
                    
                case SetLocal: {
                    VariableAccessData* variable = node->variableAccessData();
                    HashMap<VariableAccessData*, CheckData>::iterator iter = m_map.find(variable);
                    if (iter == m_map.end())
                        break;
                    if (!iter->value.m_structure)
                        break;

                    // First insert a dead SetLocal to tell OSR that the child's value should
                    // be dropped into this bytecode variable if the CheckStructure decides
                    // to exit.
                    
                    CodeOrigin codeOrigin = node->codeOrigin;
                    Edge child1 = node->child1();
                    
                    insertionSet.insertNode(
                        indexInBlock, SpecNone, SetLocal, codeOrigin, OpInfo(variable), child1);

                    // Use a ForwardCheckStructure to indicate that we should exit to the
                    // next bytecode instruction rather than reexecuting the current one.
                    insertionSet.insertNode(
                        indexInBlock, SpecNone, ForwardCheckStructure, codeOrigin,
                        OpInfo(m_graph.addStructureSet(iter->value.m_structure)),
                        Edge(child1.node(), CellUse));
                    changed = true;
                    break;
                }
                    
                default:
                    break;
                }
            }
            insertionSet.execute(block);
        }
        
        return changed;
    }

private:
    bool shouldConsiderForHoisting(VariableAccessData* variable)
    {
        if (!variable->shouldUnboxIfPossible())
            return false;
        if (variable->structureCheckHoistingFailed())
            return false;
        if (!isCellSpeculation(variable->prediction()))
            return false;
        return true;
    }
    
    void noticeStructureCheck(VariableAccessData* variable, Structure* structure)
    {
        HashMap<VariableAccessData*, CheckData>::AddResult result =
            m_map.add(variable, CheckData(structure));
        if (result.isNewEntry)
            return;
        if (result.iterator->value.m_structure == structure)
            return;
        result.iterator->value.m_structure = 0;
    }
    
    void noticeStructureCheck(VariableAccessData* variable, const StructureSet& set)
    {
        if (set.size() != 1) {
            noticeStructureCheck(variable, 0);
            return;
        }
        noticeStructureCheck(variable, set.singletonStructure());
    }
    
    struct CheckData {
        Structure* m_structure;
        
        CheckData()
            : m_structure(0)
        {
        }
        
        CheckData(Structure* structure)
            : m_structure(structure)
        {
        }
    };
    
    HashMap<VariableAccessData*, CheckData> m_map;
};

bool performTypeCheckHoisting(Graph& graph)
{
    SamplingRegion samplingRegion("DFG Type Check Hoisting Phase");
    return runPhase<TypeCheckHoistingPhase>(graph);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)


