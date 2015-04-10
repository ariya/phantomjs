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

#ifndef DFGGraph_h
#define DFGGraph_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "CodeBlock.h"
#include "DFGArgumentPosition.h"
#include "DFGAssemblyHelpers.h"
#include "DFGBasicBlock.h"
#include "DFGDominators.h"
#include "DFGLongLivedState.h"
#include "DFGNode.h"
#include "DFGNodeAllocator.h"
#include "DFGVariadicFunction.h"
#include "JSStack.h"
#include "MethodOfGettingAValueProfile.h"
#include <wtf/BitVector.h>
#include <wtf/HashMap.h>
#include <wtf/Vector.h>
#include <wtf/StdLibExtras.h>

namespace JSC {

class CodeBlock;
class ExecState;

namespace DFG {

struct StorageAccessData {
    size_t offset;
    unsigned identifierNumber;
};

struct ResolveGlobalData {
    unsigned identifierNumber;
    ResolveOperations* resolveOperations;
    PutToBaseOperation* putToBaseOperation;
    unsigned resolvePropertyIndex;
};

struct ResolveOperationData {
    unsigned identifierNumber;
    ResolveOperations* resolveOperations;
    PutToBaseOperation* putToBaseOperation;
};

struct PutToBaseOperationData {
    PutToBaseOperation* putToBaseOperation;
};

enum AddSpeculationMode {
    DontSpeculateInteger,
    SpeculateIntegerAndTruncateConstants,
    SpeculateInteger
};


//
// === Graph ===
//
// The order may be significant for nodes with side-effects (property accesses, value conversions).
// Nodes that are 'dead' remain in the vector with refCount 0.
class Graph {
public:
    Graph(VM&, CodeBlock*, unsigned osrEntryBytecodeIndex, const Operands<JSValue>& mustHandleValues);
    ~Graph();
    
    void changeChild(Edge& edge, Node* newNode)
    {
        edge.setNode(newNode);
    }
    
    void changeEdge(Edge& edge, Edge newEdge)
    {
        edge = newEdge;
    }
    
    void compareAndSwap(Edge& edge, Node* oldNode, Node* newNode)
    {
        if (edge.node() != oldNode)
            return;
        changeChild(edge, newNode);
    }
    
    void compareAndSwap(Edge& edge, Edge oldEdge, Edge newEdge)
    {
        if (edge != oldEdge)
            return;
        changeEdge(edge, newEdge);
    }
    
    void clearAndDerefChild(Node* node, unsigned index)
    {
        if (!node->children.child(index))
            return;
        node->children.setChild(index, Edge());
    }
    void clearAndDerefChild1(Node* node) { clearAndDerefChild(node, 0); }
    void clearAndDerefChild2(Node* node) { clearAndDerefChild(node, 1); }
    void clearAndDerefChild3(Node* node) { clearAndDerefChild(node, 2); }
    
    void performSubstitution(Node* node)
    {
        if (node->flags() & NodeHasVarArgs) {
            for (unsigned childIdx = node->firstChild(); childIdx < node->firstChild() + node->numChildren(); childIdx++)
                performSubstitutionForEdge(m_varArgChildren[childIdx]);
        } else {
            performSubstitutionForEdge(node->child1());
            performSubstitutionForEdge(node->child2());
            performSubstitutionForEdge(node->child3());
        }
    }
    
    void performSubstitutionForEdge(Edge& child)
    {
        // Check if this operand is actually unused.
        if (!child)
            return;
        
        // Check if there is any replacement.
        Node* replacement = child->replacement;
        if (!replacement)
            return;
        
        child.setNode(replacement);
        
        // There is definitely a replacement. Assert that the replacement does not
        // have a replacement.
        ASSERT(!child->replacement);
    }
    
#define DFG_DEFINE_ADD_NODE(templatePre, templatePost, typeParams, valueParamsComma, valueParams, valueArgs) \
    templatePre typeParams templatePost Node* addNode(SpeculatedType type valueParamsComma valueParams) \
    { \
        Node* node = new (m_allocator) Node(valueArgs); \
        node->predict(type); \
        return node; \
    }
    DFG_VARIADIC_TEMPLATE_FUNCTION(DFG_DEFINE_ADD_NODE)
#undef DFG_DEFINE_ADD_NODE

    void dethread();
    
    void convertToConstant(Node* node, unsigned constantNumber)
    {
        if (node->op() == GetLocal)
            dethread();
        else
            ASSERT(!node->hasVariableAccessData());
        node->convertToConstant(constantNumber);
    }
    
    void convertToConstant(Node* node, JSValue value)
    {
        convertToConstant(node, m_codeBlock->addOrFindConstant(value));
    }

    // CodeBlock is optional, but may allow additional information to be dumped (e.g. Identifier names).
    void dump(PrintStream& = WTF::dataFile());
    enum PhiNodeDumpMode { DumpLivePhisOnly, DumpAllPhis };
    void dumpBlockHeader(PrintStream&, const char* prefix, BlockIndex, PhiNodeDumpMode);
    void dump(PrintStream&, Edge);
    void dump(PrintStream&, const char* prefix, Node*);
    static int amountOfNodeWhiteSpace(Node*);
    static void printNodeWhiteSpace(PrintStream&, Node*);

    // Dump the code origin of the given node as a diff from the code origin of the
    // preceding node. Returns true if anything was printed.
    bool dumpCodeOrigin(PrintStream&, const char* prefix, Node* previousNode, Node* currentNode);

    BlockIndex blockIndexForBytecodeOffset(Vector<BlockIndex>& blocks, unsigned bytecodeBegin);

    SpeculatedType getJSConstantSpeculation(Node* node)
    {
        return speculationFromValue(node->valueOfJSConstant(m_codeBlock));
    }
    
    AddSpeculationMode addSpeculationMode(Node* add, bool leftShouldSpeculateInteger, bool rightShouldSpeculateInteger)
    {
        ASSERT(add->op() == ValueAdd || add->op() == ArithAdd || add->op() == ArithSub);
        
        Node* left = add->child1().node();
        Node* right = add->child2().node();
        
        if (left->hasConstant())
            return addImmediateShouldSpeculateInteger(add, rightShouldSpeculateInteger, left);
        if (right->hasConstant())
            return addImmediateShouldSpeculateInteger(add, leftShouldSpeculateInteger, right);
        
        return (leftShouldSpeculateInteger && rightShouldSpeculateInteger && add->canSpeculateInteger()) ? SpeculateInteger : DontSpeculateInteger;
    }
    
    AddSpeculationMode valueAddSpeculationMode(Node* add)
    {
        return addSpeculationMode(add, add->child1()->shouldSpeculateIntegerExpectingDefined(), add->child2()->shouldSpeculateIntegerExpectingDefined());
    }
    
    AddSpeculationMode arithAddSpeculationMode(Node* add)
    {
        return addSpeculationMode(add, add->child1()->shouldSpeculateIntegerForArithmetic(), add->child2()->shouldSpeculateIntegerForArithmetic());
    }
    
    AddSpeculationMode addSpeculationMode(Node* add)
    {
        if (add->op() == ValueAdd)
            return valueAddSpeculationMode(add);
        
        return arithAddSpeculationMode(add);
    }
    
    bool addShouldSpeculateInteger(Node* add)
    {
        return addSpeculationMode(add) != DontSpeculateInteger;
    }
    
    bool mulShouldSpeculateInteger(Node* mul)
    {
        ASSERT(mul->op() == ArithMul);
        
        Node* left = mul->child1().node();
        Node* right = mul->child2().node();
        
        return Node::shouldSpeculateIntegerForArithmetic(left, right) && mul->canSpeculateInteger();
    }
    
    bool negateShouldSpeculateInteger(Node* negate)
    {
        ASSERT(negate->op() == ArithNegate);
        return negate->child1()->shouldSpeculateIntegerForArithmetic() && negate->canSpeculateInteger();
    }
    
    // Helper methods to check nodes for constants.
    bool isConstant(Node* node)
    {
        return node->hasConstant();
    }
    bool isJSConstant(Node* node)
    {
        return node->hasConstant();
    }
    bool isInt32Constant(Node* node)
    {
        return node->isInt32Constant(m_codeBlock);
    }
    bool isDoubleConstant(Node* node)
    {
        return node->isDoubleConstant(m_codeBlock);
    }
    bool isNumberConstant(Node* node)
    {
        return node->isNumberConstant(m_codeBlock);
    }
    bool isBooleanConstant(Node* node)
    {
        return node->isBooleanConstant(m_codeBlock);
    }
    bool isCellConstant(Node* node)
    {
        if (!isJSConstant(node))
            return false;
        JSValue value = valueOfJSConstant(node);
        return value.isCell() && !!value;
    }
    bool isFunctionConstant(Node* node)
    {
        if (!isJSConstant(node))
            return false;
        if (!getJSFunction(valueOfJSConstant(node)))
            return false;
        return true;
    }
    bool isInternalFunctionConstant(Node* node)
    {
        if (!isJSConstant(node))
            return false;
        JSValue value = valueOfJSConstant(node);
        if (!value.isCell() || !value)
            return false;
        JSCell* cell = value.asCell();
        if (!cell->inherits(&InternalFunction::s_info))
            return false;
        return true;
    }
    // Helper methods get constant values from nodes.
    JSValue valueOfJSConstant(Node* node)
    {
        return node->valueOfJSConstant(m_codeBlock);
    }
    int32_t valueOfInt32Constant(Node* node)
    {
        return valueOfJSConstant(node).asInt32();
    }
    double valueOfNumberConstant(Node* node)
    {
        return valueOfJSConstant(node).asNumber();
    }
    bool valueOfBooleanConstant(Node* node)
    {
        return valueOfJSConstant(node).asBoolean();
    }
    JSFunction* valueOfFunctionConstant(Node* node)
    {
        JSCell* function = getJSFunction(valueOfJSConstant(node));
        ASSERT(function);
        return jsCast<JSFunction*>(function);
    }

    static const char *opName(NodeType);
    
    StructureSet* addStructureSet(const StructureSet& structureSet)
    {
        ASSERT(structureSet.size());
        m_structureSet.append(structureSet);
        return &m_structureSet.last();
    }
    
    StructureTransitionData* addStructureTransitionData(const StructureTransitionData& structureTransitionData)
    {
        m_structureTransitionData.append(structureTransitionData);
        return &m_structureTransitionData.last();
    }
    
    JSGlobalObject* globalObjectFor(CodeOrigin codeOrigin)
    {
        return m_codeBlock->globalObjectFor(codeOrigin);
    }
    
    JSObject* globalThisObjectFor(CodeOrigin codeOrigin)
    {
        JSGlobalObject* object = globalObjectFor(codeOrigin);
        return object->methodTable()->toThisObject(object, 0);
    }
    
    ExecutableBase* executableFor(InlineCallFrame* inlineCallFrame)
    {
        if (!inlineCallFrame)
            return m_codeBlock->ownerExecutable();
        
        return inlineCallFrame->executable.get();
    }
    
    ExecutableBase* executableFor(const CodeOrigin& codeOrigin)
    {
        return executableFor(codeOrigin.inlineCallFrame);
    }
    
    CodeBlock* baselineCodeBlockFor(const CodeOrigin& codeOrigin)
    {
        return baselineCodeBlockForOriginAndBaselineCodeBlock(codeOrigin, m_profiledBlock);
    }
    
    bool hasGlobalExitSite(const CodeOrigin& codeOrigin, ExitKind exitKind)
    {
        return baselineCodeBlockFor(codeOrigin)->hasExitSite(FrequentExitSite(exitKind));
    }
    
    bool hasExitSite(const CodeOrigin& codeOrigin, ExitKind exitKind)
    {
        return baselineCodeBlockFor(codeOrigin)->hasExitSite(FrequentExitSite(codeOrigin.bytecodeIndex, exitKind));
    }
    
    int argumentsRegisterFor(const CodeOrigin& codeOrigin)
    {
        if (!codeOrigin.inlineCallFrame)
            return m_codeBlock->argumentsRegister();
        
        return baselineCodeBlockForInlineCallFrame(
            codeOrigin.inlineCallFrame)->argumentsRegister() +
            codeOrigin.inlineCallFrame->stackOffset;
    }
    
    int uncheckedArgumentsRegisterFor(const CodeOrigin& codeOrigin)
    {
        if (!codeOrigin.inlineCallFrame)
            return m_codeBlock->uncheckedArgumentsRegister();
        
        CodeBlock* codeBlock = baselineCodeBlockForInlineCallFrame(
            codeOrigin.inlineCallFrame);
        if (!codeBlock->usesArguments())
            return InvalidVirtualRegister;
        
        return codeBlock->argumentsRegister() +
            codeOrigin.inlineCallFrame->stackOffset;
    }
    
    int uncheckedActivationRegisterFor(const CodeOrigin&)
    {
        // This will ignore CodeOrigin because we don't inline code that uses activations.
        // Hence for inlined call frames it will return the outermost code block's
        // activation register. This method is only used to compare the result to a local
        // to see if we're mucking with the activation register. Hence if we return the
        // "wrong" activation register for the frame then it will compare false, which is
        // what we wanted.
        return m_codeBlock->uncheckedActivationRegister();
    }
    
    ValueProfile* valueProfileFor(Node* node)
    {
        if (!node)
            return 0;
        
        CodeBlock* profiledBlock = baselineCodeBlockFor(node->codeOrigin);
        
        if (node->hasLocal()) {
            if (!operandIsArgument(node->local()))
                return 0;
            int argument = operandToArgument(node->local());
            if (node->variableAccessData() != m_arguments[argument]->variableAccessData())
                return 0;
            return profiledBlock->valueProfileForArgument(argument);
        }
        
        if (node->hasHeapPrediction())
            return profiledBlock->valueProfileForBytecodeOffset(node->codeOrigin.bytecodeIndexForValueProfile());
        
        return 0;
    }
    
    MethodOfGettingAValueProfile methodOfGettingAValueProfileFor(Node* node)
    {
        if (!node)
            return MethodOfGettingAValueProfile();
        
        CodeBlock* profiledBlock = baselineCodeBlockFor(node->codeOrigin);
        
        if (node->op() == GetLocal) {
            return MethodOfGettingAValueProfile::fromLazyOperand(
                profiledBlock,
                LazyOperandValueProfileKey(
                    node->codeOrigin.bytecodeIndex, node->local()));
        }
        
        return MethodOfGettingAValueProfile(valueProfileFor(node));
    }
    
    bool needsActivation() const
    {
        return m_codeBlock->needsFullScopeChain() && m_codeBlock->codeType() != GlobalCode;
    }
    
    bool usesArguments() const
    {
        return m_codeBlock->usesArguments();
    }
    
    unsigned numSuccessors(BasicBlock* block)
    {
        return block->last()->numSuccessors();
    }
    BlockIndex successor(BasicBlock* block, unsigned index)
    {
        return block->last()->successor(index);
    }
    BlockIndex successorForCondition(BasicBlock* block, bool condition)
    {
        return block->last()->successorForCondition(condition);
    }
    
    bool isPredictedNumerical(Node* node)
    {
        return isNumerical(node->child1().useKind()) && isNumerical(node->child2().useKind());
    }
    
    // Note that a 'true' return does not actually mean that the ByVal access clobbers nothing.
    // It really means that it will not clobber the entire world. It's still up to you to
    // carefully consider things like:
    // - PutByVal definitely changes the array it stores to, and may even change its length.
    // - PutByOffset definitely changes the object it stores to.
    // - and so on.
    bool byValIsPure(Node* node)
    {
        switch (node->arrayMode().type()) {
        case Array::Generic:
            return false;
        case Array::Int32:
        case Array::Double:
        case Array::Contiguous:
        case Array::ArrayStorage:
            return !node->arrayMode().isOutOfBounds();
        case Array::SlowPutArrayStorage:
            return !node->arrayMode().mayStoreToHole();
        case Array::String:
            return node->op() == GetByVal;
#if USE(JSVALUE32_64)
        case Array::Arguments:
            if (node->op() == GetByVal)
                return true;
            return false;
#endif // USE(JSVALUE32_64)
        default:
            return true;
        }
    }
    
    bool clobbersWorld(Node* node)
    {
        if (node->flags() & NodeClobbersWorld)
            return true;
        if (!(node->flags() & NodeMightClobber))
            return false;
        switch (node->op()) {
        case ValueAdd:
        case CompareLess:
        case CompareLessEq:
        case CompareGreater:
        case CompareGreaterEq:
        case CompareEq:
            return !isPredictedNumerical(node);
        case GetByVal:
        case PutByVal:
        case PutByValAlias:
            return !byValIsPure(node);
        case ToString:
            switch (node->child1().useKind()) {
            case StringObjectUse:
            case StringOrStringObjectUse:
                return false;
            case CellUse:
            case UntypedUse:
                return true;
            default:
                RELEASE_ASSERT_NOT_REACHED();
                return true;
            }
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return true; // If by some oddity we hit this case in release build it's safer to have CSE assume the worst.
        }
    }
    
    void determineReachability();
    void resetReachability();
    
    void resetExitStates();
    
    unsigned varArgNumChildren(Node* node)
    {
        ASSERT(node->flags() & NodeHasVarArgs);
        return node->numChildren();
    }
    
    unsigned numChildren(Node* node)
    {
        if (node->flags() & NodeHasVarArgs)
            return varArgNumChildren(node);
        return AdjacencyList::Size;
    }
    
    Edge& varArgChild(Node* node, unsigned index)
    {
        ASSERT(node->flags() & NodeHasVarArgs);
        return m_varArgChildren[node->firstChild() + index];
    }
    
    Edge& child(Node* node, unsigned index)
    {
        if (node->flags() & NodeHasVarArgs)
            return varArgChild(node, index);
        return node->children.child(index);
    }
    
    void voteNode(Node* node, unsigned ballot)
    {
        switch (node->op()) {
        case ValueToInt32:
        case UInt32ToNumber:
            node = node->child1().node();
            break;
        default:
            break;
        }
        
        if (node->op() == GetLocal)
            node->variableAccessData()->vote(ballot);
    }
    
    void voteNode(Edge edge, unsigned ballot)
    {
        voteNode(edge.node(), ballot);
    }
    
    void voteChildren(Node* node, unsigned ballot)
    {
        if (node->flags() & NodeHasVarArgs) {
            for (unsigned childIdx = node->firstChild();
                childIdx < node->firstChild() + node->numChildren();
                childIdx++) {
                if (!!m_varArgChildren[childIdx])
                    voteNode(m_varArgChildren[childIdx], ballot);
            }
            return;
        }
        
        if (!node->child1())
            return;
        voteNode(node->child1(), ballot);
        if (!node->child2())
            return;
        voteNode(node->child2(), ballot);
        if (!node->child3())
            return;
        voteNode(node->child3(), ballot);
    }
    
    template<typename T> // T = Node* or Edge
    void substitute(BasicBlock& block, unsigned startIndexInBlock, T oldThing, T newThing)
    {
        for (unsigned indexInBlock = startIndexInBlock; indexInBlock < block.size(); ++indexInBlock) {
            Node* node = block[indexInBlock];
            if (node->flags() & NodeHasVarArgs) {
                for (unsigned childIdx = node->firstChild(); childIdx < node->firstChild() + node->numChildren(); ++childIdx) {
                    if (!!m_varArgChildren[childIdx])
                        compareAndSwap(m_varArgChildren[childIdx], oldThing, newThing);
                }
                continue;
            }
            if (!node->child1())
                continue;
            compareAndSwap(node->children.child1(), oldThing, newThing);
            if (!node->child2())
                continue;
            compareAndSwap(node->children.child2(), oldThing, newThing);
            if (!node->child3())
                continue;
            compareAndSwap(node->children.child3(), oldThing, newThing);
        }
    }
    
    // Use this if you introduce a new GetLocal and you know that you introduced it *before*
    // any GetLocals in the basic block.
    // FIXME: it may be appropriate, in the future, to generalize this to handle GetLocals
    // introduced anywhere in the basic block.
    void substituteGetLocal(BasicBlock& block, unsigned startIndexInBlock, VariableAccessData* variableAccessData, Node* newGetLocal)
    {
        if (variableAccessData->isCaptured()) {
            // Let CSE worry about this one.
            return;
        }
        for (unsigned indexInBlock = startIndexInBlock; indexInBlock < block.size(); ++indexInBlock) {
            Node* node = block[indexInBlock];
            bool shouldContinue = true;
            switch (node->op()) {
            case SetLocal: {
                if (node->local() == variableAccessData->local())
                    shouldContinue = false;
                break;
            }
                
            case GetLocal: {
                if (node->variableAccessData() != variableAccessData)
                    continue;
                substitute(block, indexInBlock, node, newGetLocal);
                Node* oldTailNode = block.variablesAtTail.operand(variableAccessData->local());
                if (oldTailNode == node)
                    block.variablesAtTail.operand(variableAccessData->local()) = newGetLocal;
                shouldContinue = false;
                break;
            }
                
            default:
                break;
            }
            if (!shouldContinue)
                break;
        }
    }
    
    VM& m_vm;
    CodeBlock* m_codeBlock;
    RefPtr<Profiler::Compilation> m_compilation;
    CodeBlock* m_profiledBlock;
    
    NodeAllocator& m_allocator;

    Vector< OwnPtr<BasicBlock> , 8> m_blocks;
    Vector<Edge, 16> m_varArgChildren;
    Vector<StorageAccessData> m_storageAccessData;
    Vector<ResolveGlobalData> m_resolveGlobalData;
    Vector<ResolveOperationData> m_resolveOperationsData;
    Vector<PutToBaseOperationData> m_putToBaseOperationData;
    Vector<Node*, 8> m_arguments;
    SegmentedVector<VariableAccessData, 16> m_variableAccessData;
    SegmentedVector<ArgumentPosition, 8> m_argumentPositions;
    SegmentedVector<StructureSet, 16> m_structureSet;
    SegmentedVector<StructureTransitionData, 8> m_structureTransitionData;
    SegmentedVector<NewArrayBufferData, 4> m_newArrayBufferData;
    bool m_hasArguments;
    HashSet<ExecutableBase*> m_executablesWhoseArgumentsEscaped;
    BitVector m_preservedVars;
    Dominators m_dominators;
    unsigned m_localVars;
    unsigned m_parameterSlots;
    unsigned m_osrEntryBytecodeIndex;
    Operands<JSValue> m_mustHandleValues;
    
    OptimizationFixpointState m_fixpointState;
    GraphForm m_form;
    UnificationState m_unificationState;
    RefCountState m_refCountState;
private:
    
    void handleSuccessor(Vector<BlockIndex, 16>& worklist, BlockIndex blockIndex, BlockIndex successorIndex);
    
    AddSpeculationMode addImmediateShouldSpeculateInteger(Node* add, bool variableShouldSpeculateInteger, Node* immediate)
    {
        ASSERT(immediate->hasConstant());
        
        JSValue immediateValue = immediate->valueOfJSConstant(m_codeBlock);
        if (!immediateValue.isNumber())
            return DontSpeculateInteger;
        
        if (!variableShouldSpeculateInteger)
            return DontSpeculateInteger;
        
        if (immediateValue.isInt32())
            return add->canSpeculateInteger() ? SpeculateInteger : DontSpeculateInteger;
        
        double doubleImmediate = immediateValue.asDouble();
        const double twoToThe48 = 281474976710656.0;
        if (doubleImmediate < -twoToThe48 || doubleImmediate > twoToThe48)
            return DontSpeculateInteger;
        
        return nodeCanTruncateInteger(add->arithNodeFlags()) ? SpeculateIntegerAndTruncateConstants : DontSpeculateInteger;
    }
    
    bool mulImmediateShouldSpeculateInteger(Node* mul, Node* variable, Node* immediate)
    {
        ASSERT(immediate->hasConstant());
        
        JSValue immediateValue = immediate->valueOfJSConstant(m_codeBlock);
        if (!immediateValue.isInt32())
            return false;
        
        if (!variable->shouldSpeculateIntegerForArithmetic())
            return false;
        
        int32_t intImmediate = immediateValue.asInt32();
        // Doubles have a 53 bit mantissa so we expect a multiplication of 2^31 (the highest
        // magnitude possible int32 value) and any value less than 2^22 to not result in any
        // rounding in a double multiplication - hence it will be equivalent to an integer
        // multiplication, if we are doing int32 truncation afterwards (which is what
        // canSpeculateInteger() implies).
        const int32_t twoToThe22 = 1 << 22;
        if (intImmediate <= -twoToThe22 || intImmediate >= twoToThe22)
            return mul->canSpeculateInteger() && !nodeMayOverflow(mul->arithNodeFlags());

        return mul->canSpeculateInteger();
    }
};

class GetBytecodeBeginForBlock {
public:
    GetBytecodeBeginForBlock(Graph& graph)
        : m_graph(graph)
    {
    }
    
    unsigned operator()(BlockIndex* blockIndex) const
    {
        return m_graph.m_blocks[*blockIndex]->bytecodeBegin;
    }

private:
    Graph& m_graph;
};

inline BlockIndex Graph::blockIndexForBytecodeOffset(Vector<BlockIndex>& linkingTargets, unsigned bytecodeBegin)
{
    return *binarySearch<BlockIndex, unsigned>(linkingTargets, linkingTargets.size(), bytecodeBegin, GetBytecodeBeginForBlock(*this));
}

#define DFG_NODE_DO_TO_CHILDREN(graph, node, thingToDo) do {            \
        Node* _node = (node);                                           \
        if (_node->flags() & NodeHasVarArgs) {                          \
            for (unsigned _childIdx = _node->firstChild();              \
                _childIdx < _node->firstChild() + _node->numChildren(); \
                _childIdx++) {                                          \
                if (!!(graph).m_varArgChildren[_childIdx])              \
                    thingToDo(_node, (graph).m_varArgChildren[_childIdx]); \
            }                                                           \
        } else {                                                        \
            if (!_node->child1()) {                                     \
                ASSERT(                                                 \
                    !_node->child2()                                    \
                    && !_node->child3());                               \
                break;                                                  \
            }                                                           \
            thingToDo(_node, _node->child1());                          \
                                                                        \
            if (!_node->child2()) {                                     \
                ASSERT(!_node->child3());                               \
                break;                                                  \
            }                                                           \
            thingToDo(_node, _node->child2());                          \
                                                                        \
            if (!_node->child3())                                       \
                break;                                                  \
            thingToDo(_node, _node->child3());                          \
        }                                                               \
    } while (false)

} } // namespace JSC::DFG

#endif
#endif
