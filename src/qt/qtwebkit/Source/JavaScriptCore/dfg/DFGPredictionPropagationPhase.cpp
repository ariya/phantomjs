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
#include "DFGPredictionPropagationPhase.h"

#if ENABLE(DFG_JIT)

#include "DFGGraph.h"
#include "DFGPhase.h"
#include "Operations.h"

namespace JSC { namespace DFG {

SpeculatedType resultOfToPrimitive(SpeculatedType type)
{
    if (type & SpecObject) {
        // Objects get turned into strings. So if the input has hints of objectness,
        // the output will have hinsts of stringiness.
        return mergeSpeculations(type & ~SpecObject, SpecString);
    }
    
    return type;
}

class PredictionPropagationPhase : public Phase {
public:
    PredictionPropagationPhase(Graph& graph)
        : Phase(graph, "prediction propagation")
    {
    }
    
    bool run()
    {
        ASSERT(m_graph.m_form == ThreadedCPS);
        ASSERT(m_graph.m_unificationState == GloballyUnified);
        
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        m_count = 0;
#endif
        // 1) propagate predictions

        do {
            m_changed = false;
            
            // Forward propagation is near-optimal for both topologically-sorted and
            // DFS-sorted code.
            propagateForward();
            if (!m_changed)
                break;
            
            // Backward propagation reduces the likelihood that pathological code will
            // cause slowness. Loops (especially nested ones) resemble backward flow.
            // This pass captures two cases: (1) it detects if the forward fixpoint
            // found a sound solution and (2) short-circuits backward flow.
            m_changed = false;
            propagateBackward();
        } while (m_changed);
        
        // 2) repropagate predictions while doing double voting.

        do {
            m_changed = false;
            doRoundOfDoubleVoting();
            if (!m_changed)
                break;
            m_changed = false;
            propagateForward();
        } while (m_changed);
        
        return true;
    }
    
private:
    bool setPrediction(SpeculatedType prediction)
    {
        ASSERT(m_currentNode->hasResult());
        
        // setPrediction() is used when we know that there is no way that we can change
        // our minds about what the prediction is going to be. There is no semantic
        // difference between setPrediction() and mergeSpeculation() other than the
        // increased checking to validate this property.
        ASSERT(m_currentNode->prediction() == SpecNone || m_currentNode->prediction() == prediction);
        
        return m_currentNode->predict(prediction);
    }
    
    bool mergePrediction(SpeculatedType prediction)
    {
        ASSERT(m_currentNode->hasResult());
        
        return m_currentNode->predict(prediction);
    }
    
    SpeculatedType speculatedDoubleTypeForPrediction(SpeculatedType value)
    {
        if (!isNumberSpeculation(value))
            return SpecDouble;
        if (value & SpecDoubleNaN)
            return SpecDouble;
        return SpecDoubleReal;
    }

    SpeculatedType speculatedDoubleTypeForPredictions(SpeculatedType left, SpeculatedType right)
    {
        return speculatedDoubleTypeForPrediction(mergeSpeculations(left, right));
    }

    void propagate(Node* node)
    {
        NodeType op = node->op();

#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLog("   ", Graph::opName(op), " ", m_currentNode, ": ", NodeFlagsDump(node->flags()), " ");
#endif
        
        bool changed = false;
        
        switch (op) {
        case JSConstant:
        case WeakJSConstant: {
            changed |= setPrediction(speculationFromValue(m_graph.valueOfJSConstant(node)));
            break;
        }
            
        case GetLocal: {
            VariableAccessData* variableAccessData = node->variableAccessData();
            SpeculatedType prediction = variableAccessData->prediction();
            if (prediction)
                changed |= mergePrediction(prediction);
            break;
        }
            
        case SetLocal: {
            VariableAccessData* variableAccessData = node->variableAccessData();
            changed |= variableAccessData->predict(node->child1()->prediction());
            break;
        }
            
        case BitAnd:
        case BitOr:
        case BitXor:
        case BitRShift:
        case BitLShift:
        case BitURShift:
        case ArithIMul: {
            changed |= setPrediction(SpecInt32);
            break;
        }
            
        case ValueToInt32: {
            changed |= setPrediction(SpecInt32);
            break;
        }
            
        case ArrayPop:
        case ArrayPush:
        case RegExpExec:
        case RegExpTest:
        case GetById:
        case GetByIdFlush:
        case GetMyArgumentByValSafe:
        case GetByOffset:
        case Call:
        case Construct:
        case GetGlobalVar:
        case GetScopedVar:
        case Resolve:
        case ResolveBase:
        case ResolveBaseStrictPut:
        case ResolveGlobal: {
            changed |= setPrediction(node->getHeapPrediction());
            break;
        }

        case StringCharCodeAt: {
            changed |= setPrediction(SpecInt32);
            break;
        }

        case UInt32ToNumber: {
            if (nodeCanSpeculateInteger(node->arithNodeFlags()))
                changed |= mergePrediction(SpecInt32);
            else
                changed |= mergePrediction(SpecNumber);
            break;
        }

        case ValueAdd: {
            SpeculatedType left = node->child1()->prediction();
            SpeculatedType right = node->child2()->prediction();
            
            if (left && right) {
                if (isNumberSpeculationExpectingDefined(left) && isNumberSpeculationExpectingDefined(right)) {
                    if (m_graph.addSpeculationMode(node) != DontSpeculateInteger)
                        changed |= mergePrediction(SpecInt32);
                    else
                        changed |= mergePrediction(speculatedDoubleTypeForPredictions(left, right));
                } else if (!(left & SpecNumber) || !(right & SpecNumber)) {
                    // left or right is definitely something other than a number.
                    changed |= mergePrediction(SpecString);
                } else
                    changed |= mergePrediction(SpecString | SpecInt32 | SpecDouble);
            }
            break;
        }
            
        case ArithAdd: {
            SpeculatedType left = node->child1()->prediction();
            SpeculatedType right = node->child2()->prediction();
            
            if (left && right) {
                if (m_graph.addSpeculationMode(node) != DontSpeculateInteger)
                    changed |= mergePrediction(SpecInt32);
                else
                    changed |= mergePrediction(speculatedDoubleTypeForPredictions(left, right));
            }
            break;
        }
            
        case ArithSub: {
            SpeculatedType left = node->child1()->prediction();
            SpeculatedType right = node->child2()->prediction();
            
            if (left && right) {
                if (m_graph.addSpeculationMode(node) != DontSpeculateInteger)
                    changed |= mergePrediction(SpecInt32);
                else
                    changed |= mergePrediction(speculatedDoubleTypeForPredictions(left, right));
            }
            break;
        }
            
        case ArithNegate:
            if (node->child1()->prediction()) {
                if (m_graph.negateShouldSpeculateInteger(node))
                    changed |= mergePrediction(SpecInt32);
                else
                    changed |= mergePrediction(speculatedDoubleTypeForPrediction(node->child1()->prediction()));
            }
            break;
            
        case ArithMin:
        case ArithMax: {
            SpeculatedType left = node->child1()->prediction();
            SpeculatedType right = node->child2()->prediction();
            
            if (left && right) {
                if (Node::shouldSpeculateIntegerForArithmetic(node->child1().node(), node->child2().node())
                    && nodeCanSpeculateInteger(node->arithNodeFlags()))
                    changed |= mergePrediction(SpecInt32);
                else
                    changed |= mergePrediction(speculatedDoubleTypeForPredictions(left, right));
            }
            break;
        }

        case ArithMul: {
            SpeculatedType left = node->child1()->prediction();
            SpeculatedType right = node->child2()->prediction();
            
            if (left && right) {
                if (m_graph.mulShouldSpeculateInteger(node))
                    changed |= mergePrediction(SpecInt32);
                else
                    changed |= mergePrediction(speculatedDoubleTypeForPredictions(left, right));
            }
            break;
        }
            
        case ArithDiv: {
            SpeculatedType left = node->child1()->prediction();
            SpeculatedType right = node->child2()->prediction();
            
            if (left && right) {
                if (Node::shouldSpeculateIntegerForArithmetic(node->child1().node(), node->child2().node())
                    && nodeCanSpeculateInteger(node->arithNodeFlags()))
                    changed |= mergePrediction(SpecInt32);
                else
                    changed |= mergePrediction(SpecDouble);
            }
            break;
        }
            
        case ArithMod: {
            SpeculatedType left = node->child1()->prediction();
            SpeculatedType right = node->child2()->prediction();
            
            if (left && right) {
                if (Node::shouldSpeculateIntegerForArithmetic(node->child1().node(), node->child2().node())
                    && nodeCanSpeculateInteger(node->arithNodeFlags()))
                    changed |= mergePrediction(SpecInt32);
                else
                    changed |= mergePrediction(SpecDouble);
            }
            break;
        }
            
        case ArithSqrt: {
            changed |= setPrediction(SpecDouble);
            break;
        }
            
        case ArithAbs: {
            SpeculatedType child = node->child1()->prediction();
            if (isInt32SpeculationForArithmetic(child)
                && nodeCanSpeculateInteger(node->arithNodeFlags()))
                changed |= mergePrediction(SpecInt32);
            else
                changed |= mergePrediction(speculatedDoubleTypeForPrediction(child));
            break;
        }
            
        case LogicalNot:
        case CompareLess:
        case CompareLessEq:
        case CompareGreater:
        case CompareGreaterEq:
        case CompareEq:
        case CompareEqConstant:
        case CompareStrictEq:
        case CompareStrictEqConstant:
        case InstanceOf:
        case IsUndefined:
        case IsBoolean:
        case IsNumber:
        case IsString:
        case IsObject:
        case IsFunction: {
            changed |= setPrediction(SpecBoolean);
            break;
        }

        case TypeOf: {
            changed |= setPrediction(SpecString);
            break;
        }

        case GetByVal: {
            if (node->child1()->shouldSpeculateFloat32Array()
                || node->child1()->shouldSpeculateFloat64Array())
                changed |= mergePrediction(SpecDouble);
            else
                changed |= mergePrediction(node->getHeapPrediction());
            break;
        }
            
        case GetMyArgumentsLengthSafe: {
            changed |= setPrediction(SpecInt32);
            break;
        }

        case GetScopeRegisters:            
        case GetButterfly: 
        case GetIndexedPropertyStorage:
        case AllocatePropertyStorage:
        case ReallocatePropertyStorage: {
            changed |= setPrediction(SpecOther);
            break;
        }

        case ConvertThis: {
            SpeculatedType prediction = node->child1()->prediction();
            if (prediction) {
                if (prediction & ~SpecObject) {
                    prediction &= SpecObject;
                    prediction = mergeSpeculations(prediction, SpecObjectOther);
                }
                changed |= mergePrediction(prediction);
            }
            break;
        }
            
        case GetMyScope:
        case SkipTopScope:
        case SkipScope: {
            changed |= setPrediction(SpecCellOther);
            break;
        }
            
        case GetCallee: {
            changed |= setPrediction(SpecFunction);
            break;
        }
            
        case CreateThis:
        case NewObject: {
            changed |= setPrediction(SpecFinalObject);
            break;
        }
            
        case NewArray:
        case NewArrayWithSize:
        case NewArrayBuffer: {
            changed |= setPrediction(SpecArray);
            break;
        }
            
        case NewRegexp:
        case CreateActivation: {
            changed |= setPrediction(SpecObjectOther);
            break;
        }
        
        case StringFromCharCode: {
            changed |= setPrediction(SpecString);
            changed |= node->child1()->mergeFlags(NodeUsedAsNumber | NodeUsedAsInt);            
            break;
        }
        case StringCharAt:
        case ToString:
        case MakeRope: {
            changed |= setPrediction(SpecString);
            break;
        }
            
        case ToPrimitive: {
            SpeculatedType child = node->child1()->prediction();
            if (child)
                changed |= mergePrediction(resultOfToPrimitive(child));
            break;
        }
            
        case NewStringObject: {
            changed |= setPrediction(SpecStringObject);
            break;
        }
            
        case CreateArguments: {
            changed |= setPrediction(SpecArguments);
            break;
        }
            
        case NewFunction: {
            SpeculatedType child = node->child1()->prediction();
            if (child & SpecEmpty)
                changed |= mergePrediction((child & ~SpecEmpty) | SpecFunction);
            else
                changed |= mergePrediction(child);
            break;
        }
            
        case NewFunctionNoCheck:
        case NewFunctionExpression: {
            changed |= setPrediction(SpecFunction);
            break;
        }
            
        case PutByValAlias:
        case GetArrayLength:
        case Int32ToDouble:
        case ForwardInt32ToDouble:
        case DoubleAsInt32:
        case GetLocalUnlinked:
        case GetMyArgumentsLength:
        case GetMyArgumentByVal:
        case PhantomPutStructure:
        case PhantomArguments:
        case CheckArray:
        case Arrayify:
        case ArrayifyToStructure:
        case MovHint:
        case MovHintAndCheck:
        case ZombieHint: {
            // This node should never be visible at this stage of compilation. It is
            // inserted by fixup(), which follows this phase.
            CRASH();
            break;
        }
        
        case Phi:
            // Phis should not be visible here since we're iterating the all-but-Phi's
            // part of basic blocks.
            CRASH();
            break;

        case GetScope:
            changed |= setPrediction(SpecCellOther);
            break;

        case Identity:
            changed |= mergePrediction(node->child1()->prediction());
            break;

#ifndef NDEBUG
        // These get ignored because they don't return anything.
        case PutByVal:
        case PutScopedVar:
        case Return:
        case Throw:
        case PutById:
        case PutByIdDirect:
        case PutByOffset:
        case SetCallee:
        case SetMyScope:
        case DFG::Jump:
        case Branch:
        case Breakpoint:
        case CheckHasInstance:
        case ThrowReferenceError:
        case ForceOSRExit:
        case SetArgument:
        case CheckStructure:
        case CheckExecutable:
        case ForwardCheckStructure:
        case StructureTransitionWatchpoint:
        case ForwardStructureTransitionWatchpoint:
        case CheckFunction:
        case PutStructure:
        case TearOffActivation:
        case TearOffArguments:
        case CheckArgumentsNotCreated:
        case GlobalVarWatchpoint:
        case GarbageValue:
        case AllocationProfileWatchpoint:
        case Phantom:
        case PutGlobalVar:
        case PutGlobalVarCheck:
        case CheckWatchdogTimer:
            break;
            
        // These gets ignored because it doesn't do anything.
        case InlineStart:
        case Nop:
        case CountExecution:
        case PhantomLocal:
        case Flush:
            break;
            
        case LastNodeType:
            CRASH();
            break;
#else
        default:
            break;
#endif
        }

#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLog(SpeculationDump(node->prediction()), "\n");
#endif
        
        m_changed |= changed;
    }
        
    void propagateForward()
    {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLogF("Propagating predictions forward [%u]\n", ++m_count);
#endif
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
            BasicBlock* block = m_graph.m_blocks[blockIndex].get();
            if (!block)
                continue;
            ASSERT(block->isReachable);
            for (unsigned i = 0; i < block->size(); ++i) {
                m_currentNode = block->at(i);
                propagate(m_currentNode);
            }
        }
    }
    
    void propagateBackward()
    {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLogF("Propagating predictions backward [%u]\n", ++m_count);
#endif
        for (BlockIndex blockIndex = m_graph.m_blocks.size(); blockIndex--;) {
            BasicBlock* block = m_graph.m_blocks[blockIndex].get();
            if (!block)
                continue;
            ASSERT(block->isReachable);
            for (unsigned i = block->size(); i--;) {
                m_currentNode = block->at(i);
                propagate(m_currentNode);
            }
        }
    }
    
    void doDoubleVoting(Node* node)
    {
        switch (node->op()) {
        case ValueAdd:
        case ArithAdd:
        case ArithSub: {
            SpeculatedType left = node->child1()->prediction();
            SpeculatedType right = node->child2()->prediction();
                
            DoubleBallot ballot;
                
            if (isNumberSpeculationExpectingDefined(left) && isNumberSpeculationExpectingDefined(right)
                && !m_graph.addShouldSpeculateInteger(node))
                ballot = VoteDouble;
            else
                ballot = VoteValue;
                
            m_graph.voteNode(node->child1(), ballot);
            m_graph.voteNode(node->child2(), ballot);
            break;
        }
                
        case ArithMul: {
            SpeculatedType left = node->child1()->prediction();
            SpeculatedType right = node->child2()->prediction();
                
            DoubleBallot ballot;
                
            if (isNumberSpeculation(left) && isNumberSpeculation(right)
                && !m_graph.mulShouldSpeculateInteger(node))
                ballot = VoteDouble;
            else
                ballot = VoteValue;
                
            m_graph.voteNode(node->child1(), ballot);
            m_graph.voteNode(node->child2(), ballot);
            break;
        }

        case ArithMin:
        case ArithMax:
        case ArithMod:
        case ArithDiv: {
            SpeculatedType left = node->child1()->prediction();
            SpeculatedType right = node->child2()->prediction();
                
            DoubleBallot ballot;
                
            if (isNumberSpeculation(left) && isNumberSpeculation(right)
                && !(Node::shouldSpeculateIntegerForArithmetic(node->child1().node(), node->child2().node()) && node->canSpeculateInteger()))
                ballot = VoteDouble;
            else
                ballot = VoteValue;
                
            m_graph.voteNode(node->child1(), ballot);
            m_graph.voteNode(node->child2(), ballot);
            break;
        }
                
        case ArithAbs:
            DoubleBallot ballot;
            if (!(node->child1()->shouldSpeculateIntegerForArithmetic() && node->canSpeculateInteger()))
                ballot = VoteDouble;
            else
                ballot = VoteValue;
                
            m_graph.voteNode(node->child1(), ballot);
            break;
                
        case ArithSqrt:
            m_graph.voteNode(node->child1(), VoteDouble);
            break;
                
        case SetLocal: {
            SpeculatedType prediction = node->child1()->prediction();
            if (isDoubleSpeculation(prediction))
                node->variableAccessData()->vote(VoteDouble);
            else if (!isNumberSpeculation(prediction) || isInt32Speculation(prediction))
                node->variableAccessData()->vote(VoteValue);
            break;
        }
                
        case PutByVal:
        case PutByValAlias: {
            Edge child1 = m_graph.varArgChild(node, 0);
            Edge child2 = m_graph.varArgChild(node, 1);
            Edge child3 = m_graph.varArgChild(node, 2);
            m_graph.voteNode(child1, VoteValue);
            m_graph.voteNode(child2, VoteValue);
            switch (node->arrayMode().type()) {
            case Array::Double:
                m_graph.voteNode(child3, VoteDouble);
                break;
            default:
                m_graph.voteNode(child3, VoteValue);
                break;
            }
            break;
        }
            
        default:
            m_graph.voteChildren(node, VoteValue);
            break;
        }
    }
    
    void doRoundOfDoubleVoting()
    {
#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
        dataLogF("Voting on double uses of locals [%u]\n", m_count);
#endif
        for (unsigned i = 0; i < m_graph.m_variableAccessData.size(); ++i)
            m_graph.m_variableAccessData[i].find()->clearVotes();
        for (BlockIndex blockIndex = 0; blockIndex < m_graph.m_blocks.size(); ++blockIndex) {
            BasicBlock* block = m_graph.m_blocks[blockIndex].get();
            if (!block)
                continue;
            ASSERT(block->isReachable);
            for (unsigned i = 0; i < block->size(); ++i) {
                m_currentNode = block->at(i);
                doDoubleVoting(m_currentNode);
            }
        }
        for (unsigned i = 0; i < m_graph.m_variableAccessData.size(); ++i) {
            VariableAccessData* variableAccessData = &m_graph.m_variableAccessData[i];
            if (!variableAccessData->isRoot())
                continue;
            m_changed |= variableAccessData->tallyVotesForShouldUseDoubleFormat();
        }
        for (unsigned i = 0; i < m_graph.m_argumentPositions.size(); ++i)
            m_changed |= m_graph.m_argumentPositions[i].mergeArgumentPredictionAwareness();
        for (unsigned i = 0; i < m_graph.m_variableAccessData.size(); ++i) {
            VariableAccessData* variableAccessData = &m_graph.m_variableAccessData[i];
            if (!variableAccessData->isRoot())
                continue;
            m_changed |= variableAccessData->makePredictionForDoubleFormat();
        }
    }
    
    Node* m_currentNode;
    bool m_changed;

#if DFG_ENABLE(DEBUG_PROPAGATION_VERBOSE)
    unsigned m_count;
#endif
};
    
bool performPredictionPropagation(Graph& graph)
{
    SamplingRegion samplingRegion("DFG Prediction Propagation Phase");
    return runPhase<PredictionPropagationPhase>(graph);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

