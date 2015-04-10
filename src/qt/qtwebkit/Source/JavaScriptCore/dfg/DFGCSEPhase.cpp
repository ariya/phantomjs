/*
 * Copyright (C) 2011, 2012, 2013 Apple Inc. All rights reserved.
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
#include "DFGCSEPhase.h"

#if ENABLE(DFG_JIT)

#include "DFGGraph.h"
#include "DFGPhase.h"
#include "JSCellInlines.h"
#include <wtf/FastBitVector.h>

namespace JSC { namespace DFG {

enum CSEMode { NormalCSE, StoreElimination };

template<CSEMode cseMode>
class CSEPhase : public Phase {
public:
    CSEPhase(Graph& graph)
        : Phase(graph, cseMode == NormalCSE ? "common subexpression elimination" : "store elimination")
    {
    }
    
    bool run()
    {
        ASSERT((cseMode == NormalCSE) == (m_graph.m_fixpointState == FixpointNotConverged));
        ASSERT(m_graph.m_fixpointState != BeforeFixpoint);
        
        m_changed = false;
        
        for (unsigned block = 0; block < m_graph.m_blocks.size(); ++block)
            performBlockCSE(m_graph.m_blocks[block].get());
        
        return m_changed;
    }
    
private:
    
    Node* canonicalize(Node* node)
    {
        if (!node)
            return 0;
        
        if (node->op() == ValueToInt32)
            node = node->child1().node();
        
        return node;
    }
    Node* canonicalize(Edge edge)
    {
        return canonicalize(edge.node());
    }
    
    unsigned endIndexForPureCSE()
    {
        unsigned result = m_lastSeen[m_currentNode->op()];
        if (result == UINT_MAX)
            result = 0;
        else
            result++;
        ASSERT(result <= m_indexInBlock);
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLogF("  limit %u: ", result);
#endif
        return result;
    }

    Node* pureCSE(Node* node)
    {
        Node* child1 = canonicalize(node->child1());
        Node* child2 = canonicalize(node->child2());
        Node* child3 = canonicalize(node->child3());
        
        for (unsigned i = endIndexForPureCSE(); i--;) {
            Node* otherNode = m_currentBlock->at(i);
            if (otherNode == child1 || otherNode == child2 || otherNode == child3)
                break;

            if (node->op() != otherNode->op())
                continue;
            
            if (node->arithNodeFlags() != otherNode->arithNodeFlags())
                continue;
            
            Node* otherChild = canonicalize(otherNode->child1());
            if (!otherChild)
                return otherNode;
            if (otherChild != child1)
                continue;
            
            otherChild = canonicalize(otherNode->child2());
            if (!otherChild)
                return otherNode;
            if (otherChild != child2)
                continue;
            
            otherChild = canonicalize(otherNode->child3());
            if (!otherChild)
                return otherNode;
            if (otherChild != child3)
                continue;
            
            return otherNode;
        }
        return 0;
    }
    
    Node* int32ToDoubleCSE(Node* node)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* otherNode = m_currentBlock->at(i);
            if (otherNode == node->child1())
                return 0;
            switch (otherNode->op()) {
            case Int32ToDouble:
            case ForwardInt32ToDouble:
                if (otherNode->child1() == node->child1())
                    return otherNode;
                break;
            default:
                break;
            }
        }
        return 0;
    }
    
    Node* constantCSE(Node* node)
    {
        for (unsigned i = endIndexForPureCSE(); i--;) {
            Node* otherNode = m_currentBlock->at(i);
            if (otherNode->op() != JSConstant)
                continue;
            
            if (otherNode->constantNumber() != node->constantNumber())
                continue;
            
            return otherNode;
        }
        return 0;
    }
    
    Node* weakConstantCSE(Node* node)
    {
        for (unsigned i = endIndexForPureCSE(); i--;) {
            Node* otherNode = m_currentBlock->at(i);
            if (otherNode->op() != WeakJSConstant)
                continue;
            
            if (otherNode->weakConstant() != node->weakConstant())
                continue;
            
            return otherNode;
        }
        return 0;
    }
    
    Node* getCalleeLoadElimination(InlineCallFrame* inlineCallFrame)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node->codeOrigin.inlineCallFrame != inlineCallFrame)
                continue;
            switch (node->op()) {
            case GetCallee:
                return node;
            case SetCallee:
                return node->child1().node();
            default:
                break;
            }
        }
        return 0;
    }
    
    Node* getArrayLengthElimination(Node* array)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            switch (node->op()) {
            case GetArrayLength:
                if (node->child1() == array)
                    return node;
                break;
                
            case PutByVal:
                if (!m_graph.byValIsPure(node))
                    return 0;
                if (node->arrayMode().mayStoreToHole())
                    return 0;
                break;
                
            default:
                if (m_graph.clobbersWorld(node))
                    return 0;
            }
        }
        return 0;
    }
    
    Node* globalVarLoadElimination(WriteBarrier<Unknown>* registerPointer)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            switch (node->op()) {
            case GetGlobalVar:
                if (node->registerPointer() == registerPointer)
                    return node;
                break;
            case PutGlobalVar:
                if (node->registerPointer() == registerPointer)
                    return node->child1().node();
                break;
            default:
                break;
            }
            if (m_graph.clobbersWorld(node))
                break;
        }
        return 0;
    }
    
    Node* scopedVarLoadElimination(Node* registers, unsigned varNumber)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            switch (node->op()) {
            case GetScopedVar: {
                if (node->child1() == registers && node->varNumber() == varNumber)
                    return node;
                break;
            } 
            case PutScopedVar: {
                if (node->child2() == registers && node->varNumber() == varNumber)
                    return node->child3().node();
                break;
            }
            case SetLocal: {
                VariableAccessData* variableAccessData = node->variableAccessData();
                if (variableAccessData->isCaptured()
                    && variableAccessData->local() == static_cast<VirtualRegister>(varNumber))
                    return 0;
                break;
            }
            default:
                break;
            }
            if (m_graph.clobbersWorld(node))
                break;
        }
        return 0;
    }
    
    bool globalVarWatchpointElimination(WriteBarrier<Unknown>* registerPointer)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            switch (node->op()) {
            case GlobalVarWatchpoint:
                if (node->registerPointer() == registerPointer)
                    return true;
                break;
            case PutGlobalVar:
                if (node->registerPointer() == registerPointer)
                    return false;
                break;
            default:
                break;
            }
            if (m_graph.clobbersWorld(node))
                break;
        }
        return false;
    }
    
    Node* globalVarStoreElimination(WriteBarrier<Unknown>* registerPointer)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            switch (node->op()) {
            case PutGlobalVar:
            case PutGlobalVarCheck:
                if (node->registerPointer() == registerPointer)
                    return node;
                break;
                
            case GetGlobalVar:
                if (node->registerPointer() == registerPointer)
                    return 0;
                break;
                
            default:
                break;
            }
            if (m_graph.clobbersWorld(node) || node->canExit())
                return 0;
        }
        return 0;
    }
    
    Node* scopedVarStoreElimination(Node* scope, Node* registers, unsigned varNumber)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            switch (node->op()) {
            case PutScopedVar: {
                if (node->child1() == scope && node->child2() == registers && node->varNumber() == varNumber)
                    return node;
                break;
            }
                
            case GetScopedVar: {
                // Let's be conservative.
                if (node->varNumber() == varNumber)
                    return 0;
                break;
            }
                
            case GetLocal: {
                VariableAccessData* variableAccessData = node->variableAccessData();
                if (variableAccessData->isCaptured()
                    && variableAccessData->local() == static_cast<VirtualRegister>(varNumber))
                    return 0;
                break;
            }

            default:
                break;
            }
            if (m_graph.clobbersWorld(node) || node->canExit())
                return 0;
        }
        return 0;
    }
    
    Node* getByValLoadElimination(Node* child1, Node* child2)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1 || node == canonicalize(child2)) 
                break;

            switch (node->op()) {
            case GetByVal:
                if (!m_graph.byValIsPure(node))
                    return 0;
                if (node->child1() == child1 && canonicalize(node->child2()) == canonicalize(child2))
                    return node;
                break;
            case PutByVal:
            case PutByValAlias: {
                if (!m_graph.byValIsPure(node))
                    return 0;
                if (m_graph.varArgChild(node, 0) == child1 && canonicalize(m_graph.varArgChild(node, 1)) == canonicalize(child2))
                    return m_graph.varArgChild(node, 2).node();
                // We must assume that the PutByVal will clobber the location we're getting from.
                // FIXME: We can do better; if we know that the PutByVal is accessing an array of a
                // different type than the GetByVal, then we know that they won't clobber each other.
                // ... except of course for typed arrays, where all typed arrays clobber all other
                // typed arrays!  An Int32Array can alias a Float64Array for example, and so on.
                return 0;
            }
            case PutStructure:
            case PutByOffset:
                // GetByVal currently always speculates that it's accessing an
                // array with an integer index, which means that it's impossible
                // for a structure change or a put to property storage to affect
                // the GetByVal.
                break;
            default:
                if (m_graph.clobbersWorld(node))
                    return 0;
                break;
            }
        }
        return 0;
    }

    bool checkFunctionElimination(JSCell* function, Node* child1)
    {
        for (unsigned i = endIndexForPureCSE(); i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1) 
                break;

            if (node->op() == CheckFunction && node->child1() == child1 && node->function() == function)
                return true;
        }
        return false;
    }
    
    bool checkExecutableElimination(ExecutableBase* executable, Node* child1)
    {
        for (unsigned i = endIndexForPureCSE(); i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1)
                break;

            if (node->op() == CheckExecutable && node->child1() == child1 && node->executable() == executable)
                return true;
        }
        return false;
    }

    bool checkStructureElimination(const StructureSet& structureSet, Node* child1)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1) 
                break;

            switch (node->op()) {
            case CheckStructure:
            case ForwardCheckStructure:
                if (node->child1() == child1
                    && structureSet.isSupersetOf(node->structureSet()))
                    return true;
                break;
                
            case StructureTransitionWatchpoint:
            case ForwardStructureTransitionWatchpoint:
                if (node->child1() == child1
                    && structureSet.contains(node->structure()))
                    return true;
                break;
                
            case PutStructure:
                if (node->child1() == child1
                    && structureSet.contains(node->structureTransitionData().newStructure))
                    return true;
                if (structureSet.contains(node->structureTransitionData().previousStructure))
                    return false;
                break;
                
            case PutByOffset:
                // Setting a property cannot change the structure.
                break;
                
            case PutByVal:
            case PutByValAlias:
                if (m_graph.byValIsPure(node)) {
                    // If PutByVal speculates that it's accessing an array with an
                    // integer index, then it's impossible for it to cause a structure
                    // change.
                    break;
                }
                return false;
                
            case Arrayify:
            case ArrayifyToStructure:
                // We could check if the arrayification could affect our structures.
                // But that seems like it would take Effort.
                return false;
                
            default:
                if (m_graph.clobbersWorld(node))
                    return false;
                break;
            }
        }
        return false;
    }
    
    bool structureTransitionWatchpointElimination(Structure* structure, Node* child1)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1) 
                break;

            switch (node->op()) {
            case CheckStructure:
            case ForwardCheckStructure:
                if (node->child1() == child1
                    && node->structureSet().containsOnly(structure))
                    return true;
                break;
                
            case PutStructure:
                ASSERT(node->structureTransitionData().previousStructure != structure);
                break;
                
            case PutByOffset:
                // Setting a property cannot change the structure.
                break;
                
            case PutByVal:
            case PutByValAlias:
                if (m_graph.byValIsPure(node)) {
                    // If PutByVal speculates that it's accessing an array with an
                    // integer index, then it's impossible for it to cause a structure
                    // change.
                    break;
                }
                return false;
                
            case StructureTransitionWatchpoint:
            case ForwardStructureTransitionWatchpoint:
                if (node->structure() == structure && node->child1() == child1)
                    return true;
                break;
                
            case Arrayify:
            case ArrayifyToStructure:
                // We could check if the arrayification could affect our structures.
                // But that seems like it would take Effort.
                return false;
                
            default:
                if (m_graph.clobbersWorld(node))
                    return false;
                break;
            }
        }
        return false;
    }
    
    Node* putStructureStoreElimination(Node* child1)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1)
                break;
            switch (node->op()) {
            case CheckStructure:
            case ForwardCheckStructure:
                return 0;
                
            case PhantomPutStructure:
                if (node->child1() == child1) // No need to retrace our steps.
                    return 0;
                break;
                
            case PutStructure:
                if (node->child1() == child1)
                    return node;
                break;
                
            // PutStructure needs to execute if we GC. Hence this needs to
            // be careful with respect to nodes that GC.
            case CreateArguments:
            case TearOffArguments:
            case NewFunctionNoCheck:
            case NewFunction:
            case NewFunctionExpression:
            case CreateActivation:
            case TearOffActivation:
            case ToPrimitive:
            case NewRegexp:
            case NewArrayBuffer:
            case NewArray:
            case NewObject:
            case CreateThis:
            case AllocatePropertyStorage:
            case ReallocatePropertyStorage:
            case TypeOf:
            case ToString:
            case NewStringObject:
            case MakeRope:
                return 0;
                
            case GetIndexedPropertyStorage:
                if (node->arrayMode().getIndexedPropertyStorageMayTriggerGC())
                    return 0;
                break;
                
            default:
                break;
            }
            if (m_graph.clobbersWorld(node) || node->canExit())
                return 0;
        }
        return 0;
    }
    
    Node* getByOffsetLoadElimination(unsigned identifierNumber, Node* child1)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1)
                break;

            switch (node->op()) {
            case GetByOffset:
                if (node->child1() == child1
                    && m_graph.m_storageAccessData[node->storageAccessDataIndex()].identifierNumber == identifierNumber)
                    return node;
                break;
                
            case PutByOffset:
                if (m_graph.m_storageAccessData[node->storageAccessDataIndex()].identifierNumber == identifierNumber) {
                    if (node->child1() == child1) // Must be same property storage.
                        return node->child3().node();
                    return 0;
                }
                break;
                
            case PutStructure:
                // Changing the structure cannot change the outcome of a property get.
                break;
                
            case PutByVal:
            case PutByValAlias:
                if (m_graph.byValIsPure(node)) {
                    // If PutByVal speculates that it's accessing an array with an
                    // integer index, then it's impossible for it to cause a structure
                    // change.
                    break;
                }
                return 0;
                
            default:
                if (m_graph.clobbersWorld(node))
                    return 0;
                break;
            }
        }
        return 0;
    }
    
    Node* putByOffsetStoreElimination(unsigned identifierNumber, Node* child1)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1)
                break;

            switch (node->op()) {
            case GetByOffset:
                if (m_graph.m_storageAccessData[node->storageAccessDataIndex()].identifierNumber == identifierNumber)
                    return 0;
                break;
                
            case PutByOffset:
                if (m_graph.m_storageAccessData[node->storageAccessDataIndex()].identifierNumber == identifierNumber) {
                    if (node->child1() == child1) // Must be same property storage.
                        return node;
                    return 0;
                }
                break;
                
            case PutByVal:
            case PutByValAlias:
            case GetByVal:
                if (m_graph.byValIsPure(node)) {
                    // If PutByVal speculates that it's accessing an array with an
                    // integer index, then it's impossible for it to cause a structure
                    // change.
                    break;
                }
                return 0;
                
            default:
                if (m_graph.clobbersWorld(node))
                    return 0;
                break;
            }
            if (node->canExit())
                return 0;
        }
        return 0;
    }
    
    Node* getPropertyStorageLoadElimination(Node* child1)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1) 
                break;

            switch (node->op()) {
            case GetButterfly:
                if (node->child1() == child1)
                    return node;
                break;

            case AllocatePropertyStorage:
            case ReallocatePropertyStorage:
                // If we can cheaply prove this is a change to our object's storage, we
                // can optimize and use its result.
                if (node->child1() == child1)
                    return node;
                // Otherwise, we currently can't prove that this doesn't change our object's
                // storage, so we conservatively assume that it may change the storage
                // pointer of any object, including ours.
                return 0;
                
            case PutByOffset:
            case PutStructure:
                // Changing the structure or putting to the storage cannot
                // change the property storage pointer.
                break;
                
            case PutByVal:
            case PutByValAlias:
                if (m_graph.byValIsPure(node)) {
                    // If PutByVal speculates that it's accessing an array with an
                    // integer index, then it's impossible for it to cause a structure
                    // change.
                    break;
                }
                return 0;
                
            case Arrayify:
            case ArrayifyToStructure:
                // We could check if the arrayification could affect our butterfly.
                // But that seems like it would take Effort.
                return 0;
                
            default:
                if (m_graph.clobbersWorld(node))
                    return 0;
                break;
            }
        }
        return 0;
    }
    
    bool checkArrayElimination(Node* child1, ArrayMode arrayMode)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1) 
                break;

            switch (node->op()) {
            case PutByOffset:
            case PutStructure:
                // Changing the structure or putting to the storage cannot
                // change the property storage pointer.
                break;
                
            case CheckArray:
                if (node->child1() == child1 && node->arrayMode() == arrayMode)
                    return true;
                break;
                
            case Arrayify:
            case ArrayifyToStructure:
                // We could check if the arrayification could affect our array.
                // But that seems like it would take Effort.
                return false;
                
            default:
                if (m_graph.clobbersWorld(node))
                    return false;
                break;
            }
        }
        return false;
    }

    Node* getIndexedPropertyStorageLoadElimination(Node* child1, ArrayMode arrayMode)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node == child1) 
                break;

            switch (node->op()) {
            case GetIndexedPropertyStorage: {
                if (node->child1() == child1 && node->arrayMode() == arrayMode)
                    return node;
                break;
            }

            case PutByOffset:
            case PutStructure:
                // Changing the structure or putting to the storage cannot
                // change the property storage pointer.
                break;
                
            default:
                if (m_graph.clobbersWorld(node))
                    return 0;
                break;
            }
        }
        return 0;
    }
    
    Node* getMyScopeLoadElimination(InlineCallFrame* inlineCallFrame)
    {
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            if (node->codeOrigin.inlineCallFrame != inlineCallFrame)
                continue;
            switch (node->op()) {
            case CreateActivation:
                // This may cause us to return a different scope.
                return 0;
            case GetMyScope:
                return node;
            case SetMyScope:
                return node->child1().node();
            default:
                break;
            }
        }
        return 0;
    }
    
    Node* getLocalLoadElimination(VirtualRegister local, Node*& relevantLocalOp, bool careAboutClobbering)
    {
        relevantLocalOp = 0;
        
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            switch (node->op()) {
            case GetLocal:
                if (node->local() == local) {
                    relevantLocalOp = node;
                    return node;
                }
                break;
                
            case GetLocalUnlinked:
                if (node->unlinkedLocal() == local) {
                    relevantLocalOp = node;
                    return node;
                }
                break;
                
            case SetLocal:
                if (node->local() == local) {
                    relevantLocalOp = node;
                    return node->child1().node();
                }
                break;
                
            case PutScopedVar:
                if (static_cast<VirtualRegister>(node->varNumber()) == local)
                    return 0;
                break;
                
            default:
                if (careAboutClobbering && m_graph.clobbersWorld(node))
                    return 0;
                break;
            }
        }
        return 0;
    }
    
    struct SetLocalStoreEliminationResult {
        SetLocalStoreEliminationResult()
            : mayBeAccessed(false)
            , mayExit(false)
            , mayClobberWorld(false)
        {
        }
        
        bool mayBeAccessed;
        bool mayExit;
        bool mayClobberWorld;
    };
    SetLocalStoreEliminationResult setLocalStoreElimination(
        VirtualRegister local, Node* expectedNode)
    {
        SetLocalStoreEliminationResult result;
        for (unsigned i = m_indexInBlock; i--;) {
            Node* node = m_currentBlock->at(i);
            switch (node->op()) {
            case GetLocal:
            case Flush:
                if (node->local() == local)
                    result.mayBeAccessed = true;
                break;
                
            case GetLocalUnlinked:
                if (node->unlinkedLocal() == local)
                    result.mayBeAccessed = true;
                break;
                
            case SetLocal: {
                if (node->local() != local)
                    break;
                if (node != expectedNode)
                    result.mayBeAccessed = true;
                return result;
            }
                
            case GetScopedVar:
                if (static_cast<VirtualRegister>(node->varNumber()) == local)
                    result.mayBeAccessed = true;
                break;
                
            case GetMyScope:
            case SkipTopScope:
                if (m_graph.uncheckedActivationRegisterFor(node->codeOrigin) == local)
                    result.mayBeAccessed = true;
                break;
                
            case CheckArgumentsNotCreated:
            case GetMyArgumentsLength:
            case GetMyArgumentsLengthSafe:
                if (m_graph.uncheckedArgumentsRegisterFor(node->codeOrigin) == local)
                    result.mayBeAccessed = true;
                break;
                
            case GetMyArgumentByVal:
            case GetMyArgumentByValSafe:
                result.mayBeAccessed = true;
                break;
                
            case GetByVal:
                // If this is accessing arguments then it's potentially accessing locals.
                if (node->arrayMode().type() == Array::Arguments)
                    result.mayBeAccessed = true;
                break;
                
            case CreateArguments:
            case TearOffActivation:
            case TearOffArguments:
                // If an activation is being torn off then it means that captured variables
                // are live. We could be clever here and check if the local qualifies as an
                // argument register. But that seems like it would buy us very little since
                // any kind of tear offs are rare to begin with.
                result.mayBeAccessed = true;
                break;
                
            default:
                break;
            }
            result.mayExit |= node->canExit();
            result.mayClobberWorld |= m_graph.clobbersWorld(node);
        }
        RELEASE_ASSERT_NOT_REACHED();
        // Be safe in release mode.
        result.mayBeAccessed = true;
        return result;
    }
    
    void eliminateIrrelevantPhantomChildren(Node* node)
    {
        ASSERT(node->op() == Phantom);
        for (unsigned i = 0; i < AdjacencyList::Size; ++i) {
            Edge edge = node->children.child(i);
            if (!edge)
                continue;
            if (edge.useKind() != UntypedUse)
                continue; // Keep the type check.
            if (edge->flags() & NodeRelevantToOSR)
                continue;
            
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
            dataLog("   Eliminating edge @", m_currentNode->index(), " -> @", edge->index());
#endif
            node->children.removeEdge(i--);
            m_changed = true;
        }
    }
    
    bool setReplacement(Node* replacement)
    {
        if (!replacement)
            return false;
        
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLogF("   Replacing @%u -> @%u", m_currentNode->index(), replacement->index());
#endif
        
        m_currentNode->convertToPhantom();
        eliminateIrrelevantPhantomChildren(m_currentNode);
        
        // At this point we will eliminate all references to this node.
        m_currentNode->replacement = replacement;
        
        m_changed = true;
        
        return true;
    }
    
    void eliminate()
    {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLogF("   Eliminating @%u", m_currentNode->index());
#endif
        
        ASSERT(m_currentNode->mustGenerate());
        m_currentNode->convertToPhantom();
        eliminateIrrelevantPhantomChildren(m_currentNode);
        
        m_changed = true;
    }
    
    void eliminate(Node* node, NodeType phantomType = Phantom)
    {
        if (!node)
            return;
        ASSERT(node->mustGenerate());
        node->setOpAndDefaultNonExitFlags(phantomType);
        if (phantomType == Phantom)
            eliminateIrrelevantPhantomChildren(node);
        
        m_changed = true;
    }
    
    void performNodeCSE(Node* node)
    {
        if (cseMode == NormalCSE)
            m_graph.performSubstitution(node);
        
        if (node->op() == SetLocal)
            node->child1()->mergeFlags(NodeRelevantToOSR);
        
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLogF("   %s @%u: ", Graph::opName(node->op()), node->index());
#endif
        
        // NOTE: there are some nodes that we deliberately don't CSE even though we
        // probably could, like MakeRope and ToPrimitive. That's because there is no
        // evidence that doing CSE on these nodes would result in a performance
        // progression. Hence considering these nodes in CSE would just mean that this
        // code does more work with no win. Of course, we may want to reconsider this,
        // since MakeRope is trivially CSE-able. It's not trivially doable for
        // ToPrimitive, but we could change that with some speculations if we really
        // needed to.
        
        switch (node->op()) {
        
        case Identity:
            if (cseMode == StoreElimination)
                break;
            setReplacement(node->child1().node());
            break;
            
        // Handle the pure nodes. These nodes never have any side-effects.
        case BitAnd:
        case BitOr:
        case BitXor:
        case BitRShift:
        case BitLShift:
        case BitURShift:
        case ArithAdd:
        case ArithSub:
        case ArithNegate:
        case ArithMul:
        case ArithIMul:
        case ArithMod:
        case ArithDiv:
        case ArithAbs:
        case ArithMin:
        case ArithMax:
        case ArithSqrt:
        case StringCharAt:
        case StringCharCodeAt:
        case IsUndefined:
        case IsBoolean:
        case IsNumber:
        case IsString:
        case IsObject:
        case IsFunction:
        case DoubleAsInt32:
        case LogicalNot:
        case SkipTopScope:
        case SkipScope:
        case GetScopeRegisters:
        case GetScope:
        case TypeOf:
        case CompareEqConstant:
        case ValueToInt32:
            if (cseMode == StoreElimination)
                break;
            setReplacement(pureCSE(node));
            break;
            
        case Int32ToDouble:
        case ForwardInt32ToDouble:
            if (cseMode == StoreElimination)
                break;
            setReplacement(int32ToDoubleCSE(node));
            break;
            
        case GetCallee:
            if (cseMode == StoreElimination)
                break;
            setReplacement(getCalleeLoadElimination(node->codeOrigin.inlineCallFrame));
            break;

        case GetLocal: {
            if (cseMode == StoreElimination)
                break;
            VariableAccessData* variableAccessData = node->variableAccessData();
            if (!variableAccessData->isCaptured())
                break;
            Node* relevantLocalOp;
            Node* possibleReplacement = getLocalLoadElimination(variableAccessData->local(), relevantLocalOp, variableAccessData->isCaptured());
            if (!relevantLocalOp)
                break;
            if (relevantLocalOp->op() != GetLocalUnlinked
                && relevantLocalOp->variableAccessData() != variableAccessData)
                break;
            Node* phi = node->child1().node();
            if (!setReplacement(possibleReplacement))
                break;
            
            m_graph.dethread();
            
            // If we replace a GetLocal with a GetLocalUnlinked, then turn the GetLocalUnlinked
            // into a GetLocal.
            if (relevantLocalOp->op() == GetLocalUnlinked)
                relevantLocalOp->convertToGetLocal(variableAccessData, phi);

            m_changed = true;
            break;
        }
            
        case GetLocalUnlinked: {
            if (cseMode == StoreElimination)
                break;
            Node* relevantLocalOpIgnored;
            setReplacement(getLocalLoadElimination(node->unlinkedLocal(), relevantLocalOpIgnored, true));
            break;
        }
            
        case Flush: {
            VariableAccessData* variableAccessData = node->variableAccessData();
            VirtualRegister local = variableAccessData->local();
            Node* replacement = node->child1().node();
            if (replacement->op() != SetLocal)
                break;
            ASSERT(replacement->variableAccessData() == variableAccessData);
            // FIXME: We should be able to remove SetLocals that can exit; we just need
            // to replace them with appropriate type checks.
            if (cseMode == NormalCSE) {
                // Need to be conservative at this time; if the SetLocal has any chance of performing
                // any speculations then we cannot do anything.
                if (variableAccessData->isCaptured()) {
                    // Captured SetLocals never speculate and hence never exit.
                } else {
                    if (variableAccessData->shouldUseDoubleFormat())
                        break;
                    SpeculatedType prediction = variableAccessData->argumentAwarePrediction();
                    if (isInt32Speculation(prediction))
                        break;
                    if (isArraySpeculation(prediction))
                        break;
                    if (isBooleanSpeculation(prediction))
                        break;
                }
            } else {
                if (replacement->canExit())
                    break;
            }
            SetLocalStoreEliminationResult result =
                setLocalStoreElimination(local, replacement);
            if (result.mayBeAccessed || result.mayClobberWorld)
                break;
            ASSERT(replacement->op() == SetLocal);
            // FIXME: Investigate using mayExit as a further optimization.
            node->convertToPhantom();
            Node* dataNode = replacement->child1().node();
            ASSERT(dataNode->hasResult());
            m_graph.clearAndDerefChild1(node);
            node->children.child1() = Edge(dataNode);
            m_graph.dethread();
            m_changed = true;
            break;
        }
            
        case JSConstant:
            if (cseMode == StoreElimination)
                break;
            // This is strange, but necessary. Some phases will convert nodes to constants,
            // which may result in duplicated constants. We use CSE to clean this up.
            setReplacement(constantCSE(node));
            break;
            
        case WeakJSConstant:
            if (cseMode == StoreElimination)
                break;
            // FIXME: have CSE for weak constants against strong constants and vice-versa.
            setReplacement(weakConstantCSE(node));
            break;
            
        case GetArrayLength:
            if (cseMode == StoreElimination)
                break;
            setReplacement(getArrayLengthElimination(node->child1().node()));
            break;

        case GetMyScope:
            if (cseMode == StoreElimination)
                break;
            setReplacement(getMyScopeLoadElimination(node->codeOrigin.inlineCallFrame));
            break;
            
        // Handle nodes that are conditionally pure: these are pure, and can
        // be CSE'd, so long as the prediction is the one we want.
        case ValueAdd:
        case CompareLess:
        case CompareLessEq:
        case CompareGreater:
        case CompareGreaterEq:
        case CompareEq: {
            if (cseMode == StoreElimination)
                break;
            if (m_graph.isPredictedNumerical(node)) {
                Node* replacement = pureCSE(node);
                if (replacement && m_graph.isPredictedNumerical(replacement))
                    setReplacement(replacement);
            }
            break;
        }
            
        // Finally handle heap accesses. These are not quite pure, but we can still
        // optimize them provided that some subtle conditions are met.
        case GetGlobalVar:
            if (cseMode == StoreElimination)
                break;
            setReplacement(globalVarLoadElimination(node->registerPointer()));
            break;

        case GetScopedVar: {
            if (cseMode == StoreElimination)
                break;
            setReplacement(scopedVarLoadElimination(node->child1().node(), node->varNumber()));
            break;
        }

        case GlobalVarWatchpoint:
            if (cseMode == StoreElimination)
                break;
            if (globalVarWatchpointElimination(node->registerPointer()))
                eliminate();
            break;
            
        case PutGlobalVar:
        case PutGlobalVarCheck:
            if (cseMode == NormalCSE)
                break;
            eliminate(globalVarStoreElimination(node->registerPointer()));
            break;
            
        case PutScopedVar: {
            if (cseMode == NormalCSE)
                break;
            eliminate(scopedVarStoreElimination(node->child1().node(), node->child2().node(), node->varNumber()));
            break;
        }

        case GetByVal:
            if (cseMode == StoreElimination)
                break;
            if (m_graph.byValIsPure(node))
                setReplacement(getByValLoadElimination(node->child1().node(), node->child2().node()));
            break;
            
        case PutByVal: {
            if (cseMode == StoreElimination)
                break;
            Edge child1 = m_graph.varArgChild(node, 0);
            Edge child2 = m_graph.varArgChild(node, 1);
            if (node->arrayMode().canCSEStorage()) {
                Node* replacement = getByValLoadElimination(child1.node(), child2.node());
                if (!replacement)
                    break;
                node->setOp(PutByValAlias);
            }
            break;
        }
            
        case CheckStructure:
        case ForwardCheckStructure:
            if (cseMode == StoreElimination)
                break;
            if (checkStructureElimination(node->structureSet(), node->child1().node()))
                eliminate();
            break;
            
        case StructureTransitionWatchpoint:
        case ForwardStructureTransitionWatchpoint:
            if (cseMode == StoreElimination)
                break;
            if (structureTransitionWatchpointElimination(node->structure(), node->child1().node()))
                eliminate();
            break;
            
        case PutStructure:
            if (cseMode == NormalCSE)
                break;
            eliminate(putStructureStoreElimination(node->child1().node()), PhantomPutStructure);
            break;

        case CheckFunction:
            if (cseMode == StoreElimination)
                break;
            if (checkFunctionElimination(node->function(), node->child1().node()))
                eliminate();
            break;
                
        case CheckExecutable:
            if (cseMode == StoreElimination)
                break;
            if (checkExecutableElimination(node->executable(), node->child1().node()))
                eliminate();
            break;
                
        case CheckArray:
            if (cseMode == StoreElimination)
                break;
            if (checkArrayElimination(node->child1().node(), node->arrayMode()))
                eliminate();
            break;
            
        case GetIndexedPropertyStorage: {
            if (cseMode == StoreElimination)
                break;
            setReplacement(getIndexedPropertyStorageLoadElimination(node->child1().node(), node->arrayMode()));
            break;
        }

        case GetButterfly:
            if (cseMode == StoreElimination)
                break;
            setReplacement(getPropertyStorageLoadElimination(node->child1().node()));
            break;

        case GetByOffset:
            if (cseMode == StoreElimination)
                break;
            setReplacement(getByOffsetLoadElimination(m_graph.m_storageAccessData[node->storageAccessDataIndex()].identifierNumber, node->child1().node()));
            break;
            
        case PutByOffset:
            if (cseMode == NormalCSE)
                break;
            eliminate(putByOffsetStoreElimination(m_graph.m_storageAccessData[node->storageAccessDataIndex()].identifierNumber, node->child1().node()));
            break;
            
        case Phantom:
            // FIXME: we ought to remove Phantom's that have no children.
            
            eliminateIrrelevantPhantomChildren(node);
            break;
            
        default:
            // do nothing.
            break;
        }
        
        m_lastSeen[node->op()] = m_indexInBlock;
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLogF("\n");
#endif
    }
    
    void performBlockCSE(BasicBlock* block)
    {
        if (!block)
            return;
        if (!block->isReachable)
            return;
        
        m_currentBlock = block;
        for (unsigned i = 0; i < LastNodeType; ++i)
            m_lastSeen[i] = UINT_MAX;
        
        // All Phis need to already be marked as relevant to OSR, and have their
        // replacements cleared, so we don't get confused while doing substitutions on
        // GetLocal's.
        for (unsigned i = 0; i < block->phis.size(); ++i) {
            ASSERT(block->phis[i]->flags() & NodeRelevantToOSR);
            block->phis[i]->replacement = 0;
        }
        
        // Make all of my SetLocal and GetLocal nodes relevant to OSR, and do some other
        // necessary bookkeeping.
        for (unsigned i = 0; i < block->size(); ++i) {
            Node* node = block->at(i);
            
            node->replacement = 0;
            
            switch (node->op()) {
            case SetLocal:
            case GetLocal: // FIXME: The GetLocal case is only necessary until we do https://bugs.webkit.org/show_bug.cgi?id=106707.
                node->mergeFlags(NodeRelevantToOSR);
                break;
            default:
                node->clearFlags(NodeRelevantToOSR);
                break;
            }
        }

        for (m_indexInBlock = 0; m_indexInBlock < block->size(); ++m_indexInBlock) {
            m_currentNode = block->at(m_indexInBlock);
            performNodeCSE(m_currentNode);
        }
        
        if (!ASSERT_DISABLED && cseMode == StoreElimination) {
            // Nobody should have replacements set.
            for (unsigned i = 0; i < block->size(); ++i)
                ASSERT(!block->at(i)->replacement);
        }
    }
    
    BasicBlock* m_currentBlock;
    Node* m_currentNode;
    unsigned m_indexInBlock;
    FixedArray<unsigned, LastNodeType> m_lastSeen;
    bool m_changed; // Only tracks changes that have a substantive effect on other optimizations.
};

bool performCSE(Graph& graph)
{
    SamplingRegion samplingRegion("DFG CSE Phase");
    return runPhase<CSEPhase<NormalCSE> >(graph);
}

bool performStoreElimination(Graph& graph)
{
    SamplingRegion samplingRegion("DFG Store Elimination Phase");
    return runPhase<CSEPhase<StoreElimination> >(graph);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)


