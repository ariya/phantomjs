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
#include "DFGConstantFoldingPhase.h"

#if ENABLE(DFG_JIT)

#include "DFGAbstractState.h"
#include "DFGBasicBlock.h"
#include "DFGGraph.h"
#include "DFGInsertionSet.h"
#include "DFGPhase.h"
#include "GetByIdStatus.h"
#include "Operations.h"
#include "PutByIdStatus.h"

namespace JSC { namespace DFG {

class ConstantFoldingPhase : public Phase {
public:
    ConstantFoldingPhase(Graph& graph)
        : Phase(graph, "constant folding")
        , m_state(graph)
        , m_insertionSet(graph)
    {
    }
    
    bool run()
    {
        bool changed = false;
        
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
            BasicBlock* block = m_graph.m_blocks[blockIndex].get();
            if (!block)
                continue;
            if (!block->cfaDidFinish)
                changed |= paintUnreachableCode(blockIndex);
            if (block->cfaFoundConstants)
                changed |= foldConstants(blockIndex);
        }
        
        return changed;
    }

private:
    bool foldConstants(BlockIndex blockIndex)
    {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLogF("Constant folding considering Block #%u.\n", blockIndex);
#endif
        BasicBlock* block = m_graph.m_blocks[blockIndex].get();
        bool changed = false;
        m_state.beginBasicBlock(block);
        for (unsigned indexInBlock = 0; indexInBlock < block->size(); ++indexInBlock) {
            if (!m_state.isValid())
                break;
            
            Node* node = block->at(indexInBlock);

            bool eliminated = false;
                    
            switch (node->op()) {
            case CheckArgumentsNotCreated: {
                if (!isEmptySpeculation(
                        m_state.variables().operand(
                            m_graph.argumentsRegisterFor(node->codeOrigin)).m_type))
                    break;
                node->convertToPhantom();
                eliminated = true;
                break;
            }
                    
            case CheckStructure:
            case ForwardCheckStructure:
            case ArrayifyToStructure: {
                AbstractValue& value = m_state.forNode(node->child1());
                StructureSet set;
                if (node->op() == ArrayifyToStructure)
                    set = node->structure();
                else
                    set = node->structureSet();
                if (value.m_currentKnownStructure.isSubsetOf(set)) {
                    m_state.execute(indexInBlock); // Catch the fact that we may filter on cell.
                    node->convertToPhantom();
                    eliminated = true;
                    break;
                }
                StructureAbstractValue& structureValue = value.m_futurePossibleStructure;
                if (structureValue.isSubsetOf(set)
                    && structureValue.hasSingleton()) {
                    Structure* structure = structureValue.singleton();
                    m_state.execute(indexInBlock); // Catch the fact that we may filter on cell.
                    node->convertToStructureTransitionWatchpoint(structure);
                    eliminated = true;
                    break;
                }
                break;
            }
                
            case CheckArray:
            case Arrayify: {
                if (!node->arrayMode().alreadyChecked(m_graph, node, m_state.forNode(node->child1())))
                    break;
                node->convertToPhantom();
                eliminated = true;
                break;
            }
                
            case CheckFunction: {
                if (m_state.forNode(node->child1()).value() != node->function())
                    break;
                node->convertToPhantom();
                eliminated = true;
                break;
            }
                
            case GetById:
            case GetByIdFlush: {
                CodeOrigin codeOrigin = node->codeOrigin;
                Edge childEdge = node->child1();
                Node* child = childEdge.node();
                unsigned identifierNumber = node->identifierNumber();
                
                if (childEdge.useKind() != CellUse)
                    break;
                
                Structure* structure = m_state.forNode(child).bestProvenStructure();
                if (!structure)
                    break;
                
                bool needsWatchpoint = !m_state.forNode(child).m_currentKnownStructure.hasSingleton();
                bool needsCellCheck = m_state.forNode(child).m_type & ~SpecCell;
                
                GetByIdStatus status = GetByIdStatus::computeFor(
                    vm(), structure, codeBlock()->identifier(identifierNumber));
                
                if (!status.isSimple()) {
                    // FIXME: We could handle prototype cases.
                    // https://bugs.webkit.org/show_bug.cgi?id=110386
                    break;
                }
                
                ASSERT(status.structureSet().size() == 1);
                ASSERT(status.chain().isEmpty());
                ASSERT(status.structureSet().singletonStructure() == structure);
                
                // Now before we do anything else, push the CFA forward over the GetById
                // and make sure we signal to the loop that it should continue and not
                // do any eliminations.
                m_state.execute(indexInBlock);
                eliminated = true;
                
                if (needsWatchpoint) {
                    ASSERT(m_state.forNode(child).m_futurePossibleStructure.isSubsetOf(StructureSet(structure)));
                    m_insertionSet.insertNode(
                        indexInBlock, SpecNone, StructureTransitionWatchpoint, codeOrigin,
                        OpInfo(structure), childEdge);
                } else if (needsCellCheck) {
                    m_insertionSet.insertNode(
                        indexInBlock, SpecNone, Phantom, codeOrigin, childEdge);
                }
                
                childEdge.setUseKind(KnownCellUse);
                
                Edge propertyStorage;
                
                if (isInlineOffset(status.offset()))
                    propertyStorage = childEdge;
                else {
                    propertyStorage = Edge(m_insertionSet.insertNode(
                        indexInBlock, SpecNone, GetButterfly, codeOrigin, childEdge));
                }
                
                node->convertToGetByOffset(m_graph.m_storageAccessData.size(), propertyStorage);
                
                StorageAccessData storageAccessData;
                storageAccessData.offset = indexRelativeToBase(status.offset());
                storageAccessData.identifierNumber = identifierNumber;
                m_graph.m_storageAccessData.append(storageAccessData);
                break;
            }
                
            case PutById:
            case PutByIdDirect: {
                CodeOrigin codeOrigin = node->codeOrigin;
                Edge childEdge = node->child1();
                Node* child = childEdge.node();
                unsigned identifierNumber = node->identifierNumber();
                
                ASSERT(childEdge.useKind() == CellUse);
                
                Structure* structure = m_state.forNode(child).bestProvenStructure();
                if (!structure)
                    break;
                
                bool needsWatchpoint = !m_state.forNode(child).m_currentKnownStructure.hasSingleton();
                bool needsCellCheck = m_state.forNode(child).m_type & ~SpecCell;
                
                PutByIdStatus status = PutByIdStatus::computeFor(
                    vm(),
                    m_graph.globalObjectFor(codeOrigin),
                    structure,
                    codeBlock()->identifier(identifierNumber),
                    node->op() == PutByIdDirect);
                
                if (!status.isSimpleReplace() && !status.isSimpleTransition())
                    break;
                
                ASSERT(status.oldStructure() == structure);
                
                // Now before we do anything else, push the CFA forward over the PutById
                // and make sure we signal to the loop that it should continue and not
                // do any eliminations.
                m_state.execute(indexInBlock);
                eliminated = true;
                
                if (needsWatchpoint) {
                    ASSERT(m_state.forNode(child).m_futurePossibleStructure.isSubsetOf(StructureSet(structure)));
                    m_insertionSet.insertNode(
                        indexInBlock, SpecNone, StructureTransitionWatchpoint, codeOrigin,
                        OpInfo(structure), childEdge);
                } else if (needsCellCheck) {
                    m_insertionSet.insertNode(
                        indexInBlock, SpecNone, Phantom, codeOrigin, childEdge);
                }
                
                childEdge.setUseKind(KnownCellUse);
                
                StructureTransitionData* transitionData = 0;
                if (status.isSimpleTransition()) {
                    transitionData = m_graph.addStructureTransitionData(
                        StructureTransitionData(structure, status.newStructure()));
                    
                    if (node->op() == PutById) {
                        if (!structure->storedPrototype().isNull()) {
                            addStructureTransitionCheck(
                                codeOrigin, indexInBlock,
                                structure->storedPrototype().asCell());
                        }
                        
                        for (WriteBarrier<Structure>* it = status.structureChain()->head(); *it; ++it) {
                            JSValue prototype = (*it)->storedPrototype();
                            if (prototype.isNull())
                                continue;
                            ASSERT(prototype.isCell());
                            addStructureTransitionCheck(
                                codeOrigin, indexInBlock, prototype.asCell());
                        }
                    }
                }
                
                Edge propertyStorage;
                
                if (isInlineOffset(status.offset()))
                    propertyStorage = childEdge;
                else if (status.isSimpleReplace() || structure->outOfLineCapacity() == status.newStructure()->outOfLineCapacity()) {
                    propertyStorage = Edge(m_insertionSet.insertNode(
                        indexInBlock, SpecNone, GetButterfly, codeOrigin, childEdge));
                } else if (!structure->outOfLineCapacity()) {
                    ASSERT(status.newStructure()->outOfLineCapacity());
                    ASSERT(!isInlineOffset(status.offset()));
                    propertyStorage = Edge(m_insertionSet.insertNode(
                        indexInBlock, SpecNone, AllocatePropertyStorage,
                        codeOrigin, OpInfo(transitionData), childEdge));
                } else {
                    ASSERT(structure->outOfLineCapacity());
                    ASSERT(status.newStructure()->outOfLineCapacity() > structure->outOfLineCapacity());
                    ASSERT(!isInlineOffset(status.offset()));
                    
                    propertyStorage = Edge(m_insertionSet.insertNode(
                        indexInBlock, SpecNone, ReallocatePropertyStorage, codeOrigin,
                        OpInfo(transitionData), childEdge,
                        Edge(m_insertionSet.insertNode(
                            indexInBlock, SpecNone, GetButterfly, codeOrigin, childEdge))));
                }
                
                if (status.isSimpleTransition()) {
                    m_insertionSet.insertNode(
                        indexInBlock, SpecNone, PutStructure, codeOrigin, 
                        OpInfo(transitionData), childEdge);
                }
                
                node->convertToPutByOffset(m_graph.m_storageAccessData.size(), propertyStorage);
                
                StorageAccessData storageAccessData;
                storageAccessData.offset = indexRelativeToBase(status.offset());
                storageAccessData.identifierNumber = identifierNumber;
                m_graph.m_storageAccessData.append(storageAccessData);
                break;
            }
                
            default:
                break;
            }
                
            if (eliminated) {
                changed = true;
                continue;
            }
                
            m_state.execute(indexInBlock);
            if (!node->shouldGenerate() || m_state.didClobber() || node->hasConstant())
                continue;
            JSValue value = m_state.forNode(node).value();
            if (!value)
                continue;
                
            CodeOrigin codeOrigin = node->codeOrigin;
            AdjacencyList children = node->children;
            
            if (node->op() == GetLocal) {
                // GetLocals without a Phi child are guaranteed dead. We don't have to
                // do anything about them.
                if (!node->child1())
                    continue;
                
                if (m_graph.m_form != LoadStore) {
                    VariableAccessData* variable = node->variableAccessData();
                    Node* phi = node->child1().node();
                    if (phi->op() == Phi
                        && block->variablesAtHead.operand(variable->local()) == phi
                        && block->variablesAtTail.operand(variable->local()) == node) {
                        
                        // Keep the graph threaded for easy cases. This is improves compile
                        // times. It would be correct to just dethread here.
                        
                        m_graph.convertToConstant(node, value);
                        Node* phantom = m_insertionSet.insertNode(
                            indexInBlock, SpecNone, PhantomLocal,  codeOrigin,
                            OpInfo(variable), Edge(phi));
                        block->variablesAtHead.operand(variable->local()) = phantom;
                        block->variablesAtTail.operand(variable->local()) = phantom;
                        
                        changed = true;
                        
                        continue;
                    }
                    
                    m_graph.dethread();
                }
            } else
                ASSERT(!node->hasVariableAccessData());
            
            m_graph.convertToConstant(node, value);
            m_insertionSet.insertNode(
                indexInBlock, SpecNone, Phantom, codeOrigin, children);
            
            changed = true;
        }
        m_state.reset();
        m_insertionSet.execute(block);
        
        return changed;
    }
    
#if !ASSERT_DISABLED
    bool isCapturedAtOrAfter(BasicBlock* block, unsigned indexInBlock, int operand)
    {
        for (; indexInBlock < block->size(); ++indexInBlock) {
            Node* node = block->at(indexInBlock);
            if (!node->hasLocal())
                continue;
            if (node->local() != operand)
                continue;
            if (node->variableAccessData()->isCaptured())
                return true;
        }
        return false;
    }
#endif // !ASSERT_DISABLED
    
    void addStructureTransitionCheck(CodeOrigin codeOrigin, unsigned indexInBlock, JSCell* cell)
    {
        Node* weakConstant = m_insertionSet.insertNode(
            indexInBlock, speculationFromValue(cell), WeakJSConstant, codeOrigin, OpInfo(cell));
        
        if (cell->structure()->transitionWatchpointSetIsStillValid()) {
            m_insertionSet.insertNode(
                indexInBlock, SpecNone, StructureTransitionWatchpoint, codeOrigin,
                OpInfo(cell->structure()), Edge(weakConstant, CellUse));
            return;
        }

        m_insertionSet.insertNode(
            indexInBlock, SpecNone, CheckStructure, codeOrigin,
            OpInfo(m_graph.addStructureSet(cell->structure())), Edge(weakConstant, CellUse));
    }
    
    // This is necessary because the CFA may reach conclusions about constants based on its
    // assumption that certain code must exit, but then those constants may lead future
    // reexecutions of the CFA to believe that the same code will now no longer exit. Thus
    // to ensure soundness, we must paint unreachable code as such, by inserting an
    // unconditional ForceOSRExit wherever we find that a node would have always exited.
    // This will only happen in cases where we are making static speculations, or we're
    // making totally wrong speculations due to imprecision on the prediction propagator.
    bool paintUnreachableCode(BlockIndex blockIndex)
    {
        bool changed = false;
        
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLogF("Painting unreachable code in Block #%u.\n", blockIndex);
#endif
        BasicBlock* block = m_graph.m_blocks[blockIndex].get();
        m_state.beginBasicBlock(block);
        
        for (unsigned indexInBlock = 0; indexInBlock < block->size(); ++indexInBlock) {
            m_state.execute(indexInBlock);
            if (m_state.isValid())
                continue;
            
            Node* node = block->at(indexInBlock);
            switch (node->op()) {
            case Return:
            case Throw:
            case ThrowReferenceError:
            case ForceOSRExit:
                // Do nothing. These nodes will already do the right thing.
                break;
                
            default:
                m_insertionSet.insertNode(
                    indexInBlock, SpecNone, ForceOSRExit, node->codeOrigin);
                changed = true;
                break;
            }
            break;
        }
        m_state.reset();
        m_insertionSet.execute(block);
        
        return changed;
    }

    AbstractState m_state;
    InsertionSet m_insertionSet;
};

bool performConstantFolding(Graph& graph)
{
    SamplingRegion samplingRegion("DFG Constant Folding Phase");
    return runPhase<ConstantFoldingPhase>(graph);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)


