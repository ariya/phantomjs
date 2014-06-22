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

#ifndef DFGNode_h
#define DFGNode_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "CodeBlock.h"
#include "CodeOrigin.h"
#include "DFGAbstractValue.h"
#include "DFGAdjacencyList.h"
#include "DFGArrayMode.h"
#include "DFGCommon.h"
#include "DFGNodeFlags.h"
#include "DFGNodeType.h"
#include "DFGVariableAccessData.h"
#include "JSCJSValueInlines.h"
#include "JSCJSValue.h"
#include "Operands.h"
#include "SpeculatedType.h"
#include "StructureSet.h"
#include "ValueProfile.h"

namespace JSC { namespace DFG {

struct StructureTransitionData {
    Structure* previousStructure;
    Structure* newStructure;
    
    StructureTransitionData() { }
    
    StructureTransitionData(Structure* previousStructure, Structure* newStructure)
        : previousStructure(previousStructure)
        , newStructure(newStructure)
    {
    }
};

struct NewArrayBufferData {
    unsigned startConstant;
    unsigned numConstants;
    IndexingType indexingType;
};

// This type used in passing an immediate argument to Node constructor;
// distinguishes an immediate value (typically an index into a CodeBlock data structure - 
// a constant index, argument, or identifier) from a Node*.
struct OpInfo {
    explicit OpInfo(int32_t value) : m_value(static_cast<uintptr_t>(value)) { }
    explicit OpInfo(uint32_t value) : m_value(static_cast<uintptr_t>(value)) { }
#if OS(DARWIN) || USE(JSVALUE64)
    explicit OpInfo(size_t value) : m_value(static_cast<uintptr_t>(value)) { }
#endif
    explicit OpInfo(void* value) : m_value(reinterpret_cast<uintptr_t>(value)) { }
    uintptr_t m_value;
};

// === Node ===
//
// Node represents a single operation in the data flow graph.
struct Node {
    enum VarArgTag { VarArg };
    
    Node() { }
    
    Node(NodeType op, CodeOrigin codeOrigin, const AdjacencyList& children)
        : codeOrigin(codeOrigin)
        , children(children)
        , m_virtualRegister(InvalidVirtualRegister)
        , m_refCount(1)
        , m_prediction(SpecNone)
    {
        setOpAndDefaultFlags(op);
    }
    
    // Construct a node with up to 3 children, no immediate value.
    Node(NodeType op, CodeOrigin codeOrigin, Edge child1 = Edge(), Edge child2 = Edge(), Edge child3 = Edge())
        : codeOrigin(codeOrigin)
        , children(AdjacencyList::Fixed, child1, child2, child3)
        , m_virtualRegister(InvalidVirtualRegister)
        , m_refCount(1)
        , m_prediction(SpecNone)
    {
        setOpAndDefaultFlags(op);
        ASSERT(!(m_flags & NodeHasVarArgs));
    }

    // Construct a node with up to 3 children and an immediate value.
    Node(NodeType op, CodeOrigin codeOrigin, OpInfo imm, Edge child1 = Edge(), Edge child2 = Edge(), Edge child3 = Edge())
        : codeOrigin(codeOrigin)
        , children(AdjacencyList::Fixed, child1, child2, child3)
        , m_virtualRegister(InvalidVirtualRegister)
        , m_refCount(1)
        , m_opInfo(imm.m_value)
        , m_prediction(SpecNone)
    {
        setOpAndDefaultFlags(op);
        ASSERT(!(m_flags & NodeHasVarArgs));
    }

    // Construct a node with up to 3 children and two immediate values.
    Node(NodeType op, CodeOrigin codeOrigin, OpInfo imm1, OpInfo imm2, Edge child1 = Edge(), Edge child2 = Edge(), Edge child3 = Edge())
        : codeOrigin(codeOrigin)
        , children(AdjacencyList::Fixed, child1, child2, child3)
        , m_virtualRegister(InvalidVirtualRegister)
        , m_refCount(1)
        , m_opInfo(imm1.m_value)
        , m_opInfo2(safeCast<unsigned>(imm2.m_value))
        , m_prediction(SpecNone)
    {
        setOpAndDefaultFlags(op);
        ASSERT(!(m_flags & NodeHasVarArgs));
    }
    
    // Construct a node with a variable number of children and two immediate values.
    Node(VarArgTag, NodeType op, CodeOrigin codeOrigin, OpInfo imm1, OpInfo imm2, unsigned firstChild, unsigned numChildren)
        : codeOrigin(codeOrigin)
        , children(AdjacencyList::Variable, firstChild, numChildren)
        , m_virtualRegister(InvalidVirtualRegister)
        , m_refCount(1)
        , m_opInfo(imm1.m_value)
        , m_opInfo2(safeCast<unsigned>(imm2.m_value))
        , m_prediction(SpecNone)
    {
        setOpAndDefaultFlags(op);
        ASSERT(m_flags & NodeHasVarArgs);
    }
    
    NodeType op() const { return static_cast<NodeType>(m_op); }
    NodeFlags flags() const { return m_flags; }
    
    // This is not a fast method.
    unsigned index() const;
    
    void setOp(NodeType op)
    {
        m_op = op;
    }
    
    void setFlags(NodeFlags flags)
    {
        m_flags = flags;
    }
    
    bool mergeFlags(NodeFlags flags)
    {
        ASSERT(!(flags & NodeDoesNotExit));
        NodeFlags newFlags = m_flags | flags;
        if (newFlags == m_flags)
            return false;
        m_flags = newFlags;
        return true;
    }
    
    bool filterFlags(NodeFlags flags)
    {
        ASSERT(flags & NodeDoesNotExit);
        NodeFlags newFlags = m_flags & flags;
        if (newFlags == m_flags)
            return false;
        m_flags = newFlags;
        return true;
    }
    
    bool clearFlags(NodeFlags flags)
    {
        return filterFlags(~flags);
    }
    
    void setOpAndDefaultFlags(NodeType op)
    {
        m_op = op;
        m_flags = defaultFlags(op);
    }

    void setOpAndDefaultNonExitFlags(NodeType op)
    {
        ASSERT(!(m_flags & NodeHasVarArgs));
        setOpAndDefaultNonExitFlagsUnchecked(op);
    }

    void setOpAndDefaultNonExitFlagsUnchecked(NodeType op)
    {
        m_op = op;
        m_flags = (defaultFlags(op) & ~NodeExitsForward) | (m_flags & NodeExitsForward);
    }

    void convertToPhantom()
    {
        setOpAndDefaultNonExitFlags(Phantom);
    }

    void convertToPhantomUnchecked()
    {
        setOpAndDefaultNonExitFlagsUnchecked(Phantom);
    }

    void convertToIdentity()
    {
        RELEASE_ASSERT(child1());
        RELEASE_ASSERT(!child2());
        setOpAndDefaultNonExitFlags(Identity);
    }

    bool mustGenerate()
    {
        return m_flags & NodeMustGenerate;
    }
    
    void setCanExit(bool exits)
    {
        if (exits)
            m_flags &= ~NodeDoesNotExit;
        else
            m_flags |= NodeDoesNotExit;
    }
    
    bool canExit()
    {
        return !(m_flags & NodeDoesNotExit);
    }
    
    bool isConstant()
    {
        return op() == JSConstant;
    }
    
    bool isWeakConstant()
    {
        return op() == WeakJSConstant;
    }
    
    bool isStronglyProvedConstantIn(InlineCallFrame* inlineCallFrame)
    {
        return isConstant() && codeOrigin.inlineCallFrame == inlineCallFrame;
    }
    
    bool isStronglyProvedConstantIn(const CodeOrigin& codeOrigin)
    {
        return isStronglyProvedConstantIn(codeOrigin.inlineCallFrame);
    }
    
    bool isPhantomArguments()
    {
        return op() == PhantomArguments;
    }
    
    bool hasConstant()
    {
        switch (op()) {
        case JSConstant:
        case WeakJSConstant:
        case PhantomArguments:
            return true;
        default:
            return false;
        }
    }

    unsigned constantNumber()
    {
        ASSERT(isConstant());
        return m_opInfo;
    }
    
    void convertToConstant(unsigned constantNumber)
    {
        m_op = JSConstant;
        m_flags &= ~(NodeMustGenerate | NodeMightClobber | NodeClobbersWorld);
        m_opInfo = constantNumber;
        children.reset();
    }
    
    void convertToWeakConstant(JSCell* cell)
    {
        m_op = WeakJSConstant;
        m_flags &= ~(NodeMustGenerate | NodeMightClobber | NodeClobbersWorld);
        m_opInfo = bitwise_cast<uintptr_t>(cell);
        children.reset();
    }
    
    void convertToGetLocalUnlinked(VirtualRegister local)
    {
        m_op = GetLocalUnlinked;
        m_flags &= ~(NodeMustGenerate | NodeMightClobber | NodeClobbersWorld);
        m_opInfo = local;
        children.reset();
    }
    
    void convertToStructureTransitionWatchpoint(Structure* structure)
    {
        ASSERT(m_op == CheckStructure || m_op == ForwardCheckStructure || m_op == ArrayifyToStructure);
        m_opInfo = bitwise_cast<uintptr_t>(structure);
        if (m_op == CheckStructure || m_op == ArrayifyToStructure)
            m_op = StructureTransitionWatchpoint;
        else
            m_op = ForwardStructureTransitionWatchpoint;
    }
    
    void convertToStructureTransitionWatchpoint()
    {
        convertToStructureTransitionWatchpoint(structureSet().singletonStructure());
    }
    
    void convertToGetByOffset(unsigned storageAccessDataIndex, Edge storage)
    {
        ASSERT(m_op == GetById || m_op == GetByIdFlush);
        m_opInfo = storageAccessDataIndex;
        children.setChild1(storage);
        m_op = GetByOffset;
        m_flags &= ~NodeClobbersWorld;
    }
    
    void convertToPutByOffset(unsigned storageAccessDataIndex, Edge storage)
    {
        ASSERT(m_op == PutById || m_op == PutByIdDirect);
        m_opInfo = storageAccessDataIndex;
        children.setChild3(children.child2());
        children.setChild2(children.child1());
        children.setChild1(storage);
        m_op = PutByOffset;
        m_flags &= ~NodeClobbersWorld;
    }
    
    void convertToPhantomLocal()
    {
        ASSERT(m_op == Phantom && (child1()->op() == Phi || child1()->op() == SetLocal || child1()->op() == SetArgument));
        m_op = PhantomLocal;
        m_opInfo = child1()->m_opInfo; // Copy the variableAccessData.
        children.setChild1(Edge());
    }
    
    void convertToGetLocal(VariableAccessData* variable, Node* phi)
    {
        ASSERT(m_op == GetLocalUnlinked);
        m_op = GetLocal;
        m_opInfo = bitwise_cast<uintptr_t>(variable);
        children.setChild1(Edge(phi));
    }
    
    void convertToToString()
    {
        ASSERT(m_op == ToPrimitive);
        m_op = ToString;
    }
    
    JSCell* weakConstant()
    {
        ASSERT(op() == WeakJSConstant);
        return bitwise_cast<JSCell*>(m_opInfo);
    }
    
    JSValue valueOfJSConstant(CodeBlock* codeBlock)
    {
        switch (op()) {
        case WeakJSConstant:
            return JSValue(weakConstant());
        case JSConstant:
            return codeBlock->constantRegister(FirstConstantRegisterIndex + constantNumber()).get();
        case PhantomArguments:
            return JSValue();
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return JSValue(); // Have to return something in release mode.
        }
    }

    bool isInt32Constant(CodeBlock* codeBlock)
    {
        return isConstant() && valueOfJSConstant(codeBlock).isInt32();
    }
    
    bool isDoubleConstant(CodeBlock* codeBlock)
    {
        bool result = isConstant() && valueOfJSConstant(codeBlock).isDouble();
        if (result)
            ASSERT(!isInt32Constant(codeBlock));
        return result;
    }
    
    bool isNumberConstant(CodeBlock* codeBlock)
    {
        bool result = isConstant() && valueOfJSConstant(codeBlock).isNumber();
        ASSERT(result == (isInt32Constant(codeBlock) || isDoubleConstant(codeBlock)));
        return result;
    }
    
    bool isBooleanConstant(CodeBlock* codeBlock)
    {
        return isConstant() && valueOfJSConstant(codeBlock).isBoolean();
    }
    
    bool containsMovHint()
    {
        switch (op()) {
        case SetLocal:
        case MovHint:
        case MovHintAndCheck:
        case ZombieHint:
            return true;
        default:
            return false;
        }
    }
    
    bool hasVariableAccessData()
    {
        switch (op()) {
        case GetLocal:
        case SetLocal:
        case MovHint:
        case MovHintAndCheck:
        case ZombieHint:
        case Phi:
        case SetArgument:
        case Flush:
        case PhantomLocal:
            return true;
        default:
            return false;
        }
    }
    
    bool hasLocal()
    {
        return hasVariableAccessData();
    }
    
    VariableAccessData* variableAccessData()
    {
        ASSERT(hasVariableAccessData());
        return reinterpret_cast<VariableAccessData*>(m_opInfo)->find();
    }
    
    VirtualRegister local()
    {
        return variableAccessData()->local();
    }
    
    VirtualRegister unlinkedLocal()
    {
        ASSERT(op() == GetLocalUnlinked);
        return static_cast<VirtualRegister>(m_opInfo);
    }
    
    bool hasIdentifier()
    {
        switch (op()) {
        case GetById:
        case GetByIdFlush:
        case PutById:
        case PutByIdDirect:
            return true;
        default:
            return false;
        }
    }

    unsigned identifierNumber()
    {
        ASSERT(hasIdentifier());
        return m_opInfo;
    }
    
    unsigned resolveGlobalDataIndex()
    {
        ASSERT(op() == ResolveGlobal);
        return m_opInfo;
    }

    unsigned resolveOperationsDataIndex()
    {
        ASSERT(op() == Resolve || op() == ResolveBase || op() == ResolveBaseStrictPut);
        return m_opInfo;
    }

    bool hasArithNodeFlags()
    {
        switch (op()) {
        case UInt32ToNumber:
        case ArithAdd:
        case ArithSub:
        case ArithNegate:
        case ArithMul:
        case ArithAbs:
        case ArithMin:
        case ArithMax:
        case ArithMod:
        case ArithDiv:
        case ValueAdd:
            return true;
        default:
            return false;
        }
    }
    
    // This corrects the arithmetic node flags, so that irrelevant bits are
    // ignored. In particular, anything other than ArithMul does not need
    // to know if it can speculate on negative zero.
    NodeFlags arithNodeFlags()
    {
        NodeFlags result = m_flags & NodeArithFlagsMask;
        if (op() == ArithMul || op() == ArithDiv || op() == ArithMod || op() == ArithNegate || op() == DoubleAsInt32)
            return result;
        return result & ~NodeNeedsNegZero;
    }
    
    bool hasConstantBuffer()
    {
        return op() == NewArrayBuffer;
    }
    
    NewArrayBufferData* newArrayBufferData()
    {
        ASSERT(hasConstantBuffer());
        return reinterpret_cast<NewArrayBufferData*>(m_opInfo);
    }
    
    unsigned startConstant()
    {
        return newArrayBufferData()->startConstant;
    }
    
    unsigned numConstants()
    {
        return newArrayBufferData()->numConstants;
    }
    
    bool hasIndexingType()
    {
        switch (op()) {
        case NewArray:
        case NewArrayWithSize:
        case NewArrayBuffer:
            return true;
        default:
            return false;
        }
    }
    
    IndexingType indexingType()
    {
        ASSERT(hasIndexingType());
        if (op() == NewArrayBuffer)
            return newArrayBufferData()->indexingType;
        return m_opInfo;
    }
    
    bool hasInlineCapacity()
    {
        return op() == CreateThis;
    }

    unsigned inlineCapacity()
    {
        ASSERT(hasInlineCapacity());
        return m_opInfo;
    }

    void setIndexingType(IndexingType indexingType)
    {
        ASSERT(hasIndexingType());
        m_opInfo = indexingType;
    }
    
    bool hasRegexpIndex()
    {
        return op() == NewRegexp;
    }
    
    unsigned regexpIndex()
    {
        ASSERT(hasRegexpIndex());
        return m_opInfo;
    }
    
    bool hasVarNumber()
    {
        return op() == GetScopedVar || op() == PutScopedVar;
    }

    unsigned varNumber()
    {
        ASSERT(hasVarNumber());
        return m_opInfo;
    }
    
    bool hasIdentifierNumberForCheck()
    {
        return op() == GlobalVarWatchpoint || op() == PutGlobalVarCheck;
    }
    
    unsigned identifierNumberForCheck()
    {
        ASSERT(hasIdentifierNumberForCheck());
        return m_opInfo2;
    }
    
    bool hasRegisterPointer()
    {
        return op() == GetGlobalVar || op() == PutGlobalVar || op() == GlobalVarWatchpoint || op() == PutGlobalVarCheck;
    }
    
    WriteBarrier<Unknown>* registerPointer()
    {
        return bitwise_cast<WriteBarrier<Unknown>*>(m_opInfo);
    }

    bool hasResult()
    {
        return m_flags & NodeResultMask;
    }

    bool hasInt32Result()
    {
        return (m_flags & NodeResultMask) == NodeResultInt32;
    }
    
    bool hasNumberResult()
    {
        return (m_flags & NodeResultMask) == NodeResultNumber;
    }
    
    bool hasJSResult()
    {
        return (m_flags & NodeResultMask) == NodeResultJS;
    }
    
    bool hasBooleanResult()
    {
        return (m_flags & NodeResultMask) == NodeResultBoolean;
    }

    bool hasStorageResult()
    {
        return (m_flags & NodeResultMask) == NodeResultStorage;
    }

    bool isJump()
    {
        return op() == Jump;
    }

    bool isBranch()
    {
        return op() == Branch;
    }

    bool isTerminal()
    {
        switch (op()) {
        case Jump:
        case Branch:
        case Return:
        case Throw:
        case ThrowReferenceError:
            return true;
        default:
            return false;
        }
    }

    unsigned takenBytecodeOffsetDuringParsing()
    {
        ASSERT(isBranch() || isJump());
        return m_opInfo;
    }

    unsigned notTakenBytecodeOffsetDuringParsing()
    {
        ASSERT(isBranch());
        return m_opInfo2;
    }
    
    void setTakenBlockIndex(BlockIndex blockIndex)
    {
        ASSERT(isBranch() || isJump());
        m_opInfo = blockIndex;
    }
    
    void setNotTakenBlockIndex(BlockIndex blockIndex)
    {
        ASSERT(isBranch());
        m_opInfo2 = blockIndex;
    }
    
    BlockIndex takenBlockIndex()
    {
        ASSERT(isBranch() || isJump());
        return m_opInfo;
    }
    
    BlockIndex notTakenBlockIndex()
    {
        ASSERT(isBranch());
        return m_opInfo2;
    }
    
    unsigned numSuccessors()
    {
        switch (op()) {
        case Jump:
            return 1;
        case Branch:
            return 2;
        default:
            return 0;
        }
    }
    
    BlockIndex successor(unsigned index)
    {
        switch (index) {
        case 0:
            return takenBlockIndex();
        case 1:
            return notTakenBlockIndex();
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return NoBlock;
        }
    }
    
    BlockIndex successorForCondition(bool condition)
    {
        ASSERT(isBranch());
        return condition ? takenBlockIndex() : notTakenBlockIndex();
    }
    
    bool hasHeapPrediction()
    {
        switch (op()) {
        case GetById:
        case GetByIdFlush:
        case GetByVal:
        case GetMyArgumentByVal:
        case GetMyArgumentByValSafe:
        case Call:
        case Construct:
        case GetByOffset:
        case GetScopedVar:
        case Resolve:
        case ResolveBase:
        case ResolveBaseStrictPut:
        case ResolveGlobal:
        case ArrayPop:
        case ArrayPush:
        case RegExpExec:
        case RegExpTest:
        case GetGlobalVar:
            return true;
        default:
            return false;
        }
    }
    
    SpeculatedType getHeapPrediction()
    {
        ASSERT(hasHeapPrediction());
        return static_cast<SpeculatedType>(m_opInfo2);
    }
    
    bool predictHeap(SpeculatedType prediction)
    {
        ASSERT(hasHeapPrediction());
        
        return mergeSpeculation(m_opInfo2, prediction);
    }
    
    bool hasFunction()
    {
        switch (op()) {
        case CheckFunction:
        case AllocationProfileWatchpoint:
            return true;
        default:
            return false;
        }
    }

    JSCell* function()
    {
        ASSERT(hasFunction());
        JSCell* result = reinterpret_cast<JSFunction*>(m_opInfo);
        ASSERT(JSValue(result).isFunction());
        return result;
    }
    
    bool hasExecutable()
    {
        return op() == CheckExecutable;
    }
    
    ExecutableBase* executable()
    {
        return jsCast<ExecutableBase*>(reinterpret_cast<JSCell*>(m_opInfo));
    }

    bool hasStructureTransitionData()
    {
        switch (op()) {
        case PutStructure:
        case PhantomPutStructure:
        case AllocatePropertyStorage:
        case ReallocatePropertyStorage:
            return true;
        default:
            return false;
        }
    }
    
    StructureTransitionData& structureTransitionData()
    {
        ASSERT(hasStructureTransitionData());
        return *reinterpret_cast<StructureTransitionData*>(m_opInfo);
    }
    
    bool hasStructureSet()
    {
        switch (op()) {
        case CheckStructure:
        case ForwardCheckStructure:
            return true;
        default:
            return false;
        }
    }
    
    StructureSet& structureSet()
    {
        ASSERT(hasStructureSet());
        return *reinterpret_cast<StructureSet*>(m_opInfo);
    }
    
    bool hasStructure()
    {
        switch (op()) {
        case StructureTransitionWatchpoint:
        case ForwardStructureTransitionWatchpoint:
        case ArrayifyToStructure:
        case NewObject:
        case NewStringObject:
            return true;
        default:
            return false;
        }
    }
    
    Structure* structure()
    {
        ASSERT(hasStructure());
        return reinterpret_cast<Structure*>(m_opInfo);
    }
    
    bool hasStorageAccessData()
    {
        return op() == GetByOffset || op() == PutByOffset;
    }
    
    unsigned storageAccessDataIndex()
    {
        ASSERT(hasStorageAccessData());
        return m_opInfo;
    }
    
    bool hasFunctionDeclIndex()
    {
        return op() == NewFunction
            || op() == NewFunctionNoCheck;
    }
    
    unsigned functionDeclIndex()
    {
        ASSERT(hasFunctionDeclIndex());
        return m_opInfo;
    }
    
    bool hasFunctionExprIndex()
    {
        return op() == NewFunctionExpression;
    }
    
    unsigned functionExprIndex()
    {
        ASSERT(hasFunctionExprIndex());
        return m_opInfo;
    }
    
    bool hasArrayMode()
    {
        switch (op()) {
        case GetIndexedPropertyStorage:
        case GetArrayLength:
        case PutByVal:
        case PutByValAlias:
        case GetByVal:
        case StringCharAt:
        case StringCharCodeAt:
        case CheckArray:
        case Arrayify:
        case ArrayifyToStructure:
        case ArrayPush:
        case ArrayPop:
            return true;
        default:
            return false;
        }
    }
    
    ArrayMode arrayMode()
    {
        ASSERT(hasArrayMode());
        if (op() == ArrayifyToStructure)
            return ArrayMode::fromWord(m_opInfo2);
        return ArrayMode::fromWord(m_opInfo);
    }
    
    bool setArrayMode(ArrayMode arrayMode)
    {
        ASSERT(hasArrayMode());
        if (this->arrayMode() == arrayMode)
            return false;
        m_opInfo = arrayMode.asWord();
        return true;
    }
    
    bool hasVirtualRegister()
    {
        return m_virtualRegister != InvalidVirtualRegister;
    }
    
    VirtualRegister virtualRegister()
    {
        ASSERT(hasResult());
        ASSERT(m_virtualRegister != InvalidVirtualRegister);
        return m_virtualRegister;
    }
    
    void setVirtualRegister(VirtualRegister virtualRegister)
    {
        ASSERT(hasResult());
        ASSERT(m_virtualRegister == InvalidVirtualRegister);
        m_virtualRegister = virtualRegister;
    }
    
    bool hasArgumentPositionStart()
    {
        return op() == InlineStart;
    }
    
    unsigned argumentPositionStart()
    {
        ASSERT(hasArgumentPositionStart());
        return m_opInfo;
    }
    
    bool hasExecutionCounter()
    {
        return op() == CountExecution;
    }
    
    Profiler::ExecutionCounter* executionCounter()
    {
        return bitwise_cast<Profiler::ExecutionCounter*>(m_opInfo);
    }

    bool shouldGenerate()
    {
        return m_refCount;
    }
    
    bool willHaveCodeGenOrOSR()
    {
        switch (op()) {
        case SetLocal:
        case MovHint:
        case ZombieHint:
        case MovHintAndCheck:
        case Int32ToDouble:
        case ForwardInt32ToDouble:
        case ValueToInt32:
        case UInt32ToNumber:
        case DoubleAsInt32:
        case PhantomArguments:
            return true;
        case Nop:
            return false;
        case Phantom:
            return child1().useKindUnchecked() != UntypedUse || child2().useKindUnchecked() != UntypedUse || child3().useKindUnchecked() != UntypedUse;
        default:
            return shouldGenerate();
        }
    }

    unsigned refCount()
    {
        return m_refCount;
    }

    unsigned postfixRef()
    {
        return m_refCount++;
    }

    unsigned adjustedRefCount()
    {
        return mustGenerate() ? m_refCount - 1 : m_refCount;
    }
    
    void setRefCount(unsigned refCount)
    {
        m_refCount = refCount;
    }
    
    Edge& child1()
    {
        ASSERT(!(m_flags & NodeHasVarArgs));
        return children.child1();
    }
    
    // This is useful if you want to do a fast check on the first child
    // before also doing a check on the opcode. Use this with care and
    // avoid it if possible.
    Edge child1Unchecked()
    {
        return children.child1Unchecked();
    }

    Edge& child2()
    {
        ASSERT(!(m_flags & NodeHasVarArgs));
        return children.child2();
    }

    Edge& child3()
    {
        ASSERT(!(m_flags & NodeHasVarArgs));
        return children.child3();
    }
    
    unsigned firstChild()
    {
        ASSERT(m_flags & NodeHasVarArgs);
        return children.firstChild();
    }
    
    unsigned numChildren()
    {
        ASSERT(m_flags & NodeHasVarArgs);
        return children.numChildren();
    }
    
    UseKind binaryUseKind()
    {
        ASSERT(child1().useKind() == child2().useKind());
        return child1().useKind();
    }
    
    bool isBinaryUseKind(UseKind useKind)
    {
        return child1().useKind() == useKind && child2().useKind() == useKind;
    }
    
    SpeculatedType prediction()
    {
        return m_prediction;
    }
    
    bool predict(SpeculatedType prediction)
    {
        return mergeSpeculation(m_prediction, prediction);
    }
    
    bool shouldSpeculateInteger()
    {
        return isInt32Speculation(prediction());
    }
    
    bool shouldSpeculateIntegerForArithmetic()
    {
        return isInt32SpeculationForArithmetic(prediction());
    }
    
    bool shouldSpeculateIntegerExpectingDefined()
    {
        return isInt32SpeculationExpectingDefined(prediction());
    }
    
    bool shouldSpeculateDouble()
    {
        return isDoubleSpeculation(prediction());
    }
    
    bool shouldSpeculateDoubleForArithmetic()
    {
        return isDoubleSpeculationForArithmetic(prediction());
    }
    
    bool shouldSpeculateNumber()
    {
        return isNumberSpeculation(prediction());
    }
    
    bool shouldSpeculateNumberExpectingDefined()
    {
        return isNumberSpeculationExpectingDefined(prediction());
    }
    
    bool shouldSpeculateBoolean()
    {
        return isBooleanSpeculation(prediction());
    }
   
    bool shouldSpeculateString()
    {
        return isStringSpeculation(prediction());
    }
 
    bool shouldSpeculateStringObject()
    {
        return isStringObjectSpeculation(prediction());
    }
    
    bool shouldSpeculateStringOrStringObject()
    {
        return isStringOrStringObjectSpeculation(prediction());
    }
    
    bool shouldSpeculateFinalObject()
    {
        return isFinalObjectSpeculation(prediction());
    }
    
    bool shouldSpeculateFinalObjectOrOther()
    {
        return isFinalObjectOrOtherSpeculation(prediction());
    }
    
    bool shouldSpeculateArray()
    {
        return isArraySpeculation(prediction());
    }
    
    bool shouldSpeculateArguments()
    {
        return isArgumentsSpeculation(prediction());
    }
    
    bool shouldSpeculateInt8Array()
    {
        return isInt8ArraySpeculation(prediction());
    }
    
    bool shouldSpeculateInt16Array()
    {
        return isInt16ArraySpeculation(prediction());
    }
    
    bool shouldSpeculateInt32Array()
    {
        return isInt32ArraySpeculation(prediction());
    }
    
    bool shouldSpeculateUint8Array()
    {
        return isUint8ArraySpeculation(prediction());
    }

    bool shouldSpeculateUint8ClampedArray()
    {
        return isUint8ClampedArraySpeculation(prediction());
    }
    
    bool shouldSpeculateUint16Array()
    {
        return isUint16ArraySpeculation(prediction());
    }
    
    bool shouldSpeculateUint32Array()
    {
        return isUint32ArraySpeculation(prediction());
    }
    
    bool shouldSpeculateFloat32Array()
    {
        return isFloat32ArraySpeculation(prediction());
    }
    
    bool shouldSpeculateFloat64Array()
    {
        return isFloat64ArraySpeculation(prediction());
    }
    
    bool shouldSpeculateArrayOrOther()
    {
        return isArrayOrOtherSpeculation(prediction());
    }
    
    bool shouldSpeculateObject()
    {
        return isObjectSpeculation(prediction());
    }
    
    bool shouldSpeculateObjectOrOther()
    {
        return isObjectOrOtherSpeculation(prediction());
    }

    bool shouldSpeculateCell()
    {
        return isCellSpeculation(prediction());
    }
    
    static bool shouldSpeculateBoolean(Node* op1, Node* op2)
    {
        return op1->shouldSpeculateBoolean() && op2->shouldSpeculateBoolean();
    }
    
    static bool shouldSpeculateInteger(Node* op1, Node* op2)
    {
        return op1->shouldSpeculateInteger() && op2->shouldSpeculateInteger();
    }
    
    static bool shouldSpeculateIntegerForArithmetic(Node* op1, Node* op2)
    {
        return op1->shouldSpeculateIntegerForArithmetic() && op2->shouldSpeculateIntegerForArithmetic();
    }
    
    static bool shouldSpeculateIntegerExpectingDefined(Node* op1, Node* op2)
    {
        return op1->shouldSpeculateIntegerExpectingDefined() && op2->shouldSpeculateIntegerExpectingDefined();
    }
    
    static bool shouldSpeculateDoubleForArithmetic(Node* op1, Node* op2)
    {
        return op1->shouldSpeculateDoubleForArithmetic() && op2->shouldSpeculateDoubleForArithmetic();
    }
    
    static bool shouldSpeculateNumber(Node* op1, Node* op2)
    {
        return op1->shouldSpeculateNumber() && op2->shouldSpeculateNumber();
    }
    
    static bool shouldSpeculateNumberExpectingDefined(Node* op1, Node* op2)
    {
        return op1->shouldSpeculateNumberExpectingDefined() && op2->shouldSpeculateNumberExpectingDefined();
    }
    
    static bool shouldSpeculateFinalObject(Node* op1, Node* op2)
    {
        return op1->shouldSpeculateFinalObject() && op2->shouldSpeculateFinalObject();
    }

    static bool shouldSpeculateArray(Node* op1, Node* op2)
    {
        return op1->shouldSpeculateArray() && op2->shouldSpeculateArray();
    }
    
    bool canSpeculateInteger()
    {
        return nodeCanSpeculateInteger(arithNodeFlags());
    }
    
    void dumpChildren(PrintStream& out)
    {
        if (!child1())
            return;
        out.printf("@%u", child1()->index());
        if (!child2())
            return;
        out.printf(", @%u", child2()->index());
        if (!child3())
            return;
        out.printf(", @%u", child3()->index());
    }
    
    // NB. This class must have a trivial destructor.
    
    // Used to look up exception handling information (currently implemented as a bytecode index).
    CodeOrigin codeOrigin;
    // References to up to 3 children, or links to a variable length set of children.
    AdjacencyList children;

private:
    unsigned m_op : 10; // real type is NodeType
    unsigned m_flags : 22;
    // The virtual register number (spill location) associated with this .
    VirtualRegister m_virtualRegister;
    // The number of uses of the result of this operation (+1 for 'must generate' nodes, which have side-effects).
    unsigned m_refCount;
    // Immediate values, accesses type-checked via accessors above. The first one is
    // big enough to store a pointer.
    uintptr_t m_opInfo;
    unsigned m_opInfo2;
    // The prediction ascribed to this node after propagation.
    SpeculatedType m_prediction;

public:
    // Fields used by various analyses.
    AbstractValue value;
    Node* replacement;
};

} } // namespace JSC::DFG

namespace WTF {

void printInternal(PrintStream&, JSC::DFG::Node*);

} // namespace WTF

#endif
#endif
