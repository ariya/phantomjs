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
#include "DFGByteCodeParser.h"

#if ENABLE(DFG_JIT)

#include "DFGAliasTracker.h"
#include "DFGScoreBoard.h"
#include "CodeBlock.h"

namespace JSC { namespace DFG {

#if ENABLE(DFG_JIT_RESTRICTIONS)
// FIXME: Temporarily disable arithmetic, until we fix associated performance regressions.
#define ARITHMETIC_OP() m_parseFailed = true
#else
#define ARITHMETIC_OP() ((void)0)
#endif

// === ByteCodeParser ===
//
// This class is used to compile the dataflow graph from a CodeBlock.
class ByteCodeParser {
public:
    ByteCodeParser(JSGlobalData* globalData, CodeBlock* codeBlock, Graph& graph)
        : m_globalData(globalData)
        , m_codeBlock(codeBlock)
        , m_graph(graph)
        , m_currentIndex(0)
        , m_parseFailed(false)
        , m_constantUndefined(UINT_MAX)
        , m_constantNull(UINT_MAX)
        , m_constant1(UINT_MAX)
        , m_constants(codeBlock->numberOfConstantRegisters())
        , m_numArguments(codeBlock->m_numParameters)
        , m_numLocals(codeBlock->m_numCalleeRegisters)
        , m_preservedVars(codeBlock->m_numVars)
    {
    }

    // Parse a full CodeBlock of bytecode.
    bool parse();

private:
    // Parse a single basic block of bytecode instructions.
    bool parseBlock(unsigned limit);
    // Setup predecessor links in the graph's BasicBlocks.
    void setupPredecessors();
    // Link GetLocal & SetLocal nodes, to ensure live values are generated.
    enum PhiStackType {
        LocalPhiStack,
        ArgumentPhiStack
    };
    template<PhiStackType stackType>
    void processPhiStack();
    // Add spill locations to nodes.
    void allocateVirtualRegisters();

    // Get/Set the operands/result of a bytecode instruction.
    NodeIndex get(int operand)
    {
        // Is this a constant?
        if (operand >= FirstConstantRegisterIndex) {
            unsigned constant = operand - FirstConstantRegisterIndex;
            ASSERT(constant < m_constants.size());
            return getJSConstant(constant);
        }

        // Is this an argument?
        if (operandIsArgument(operand))
            return getArgument(operand);

        // Must be a local.
        return getLocal((unsigned)operand);
    }
    void set(int operand, NodeIndex value, PredictedType prediction = PredictNone)
    {
        m_graph.predict(operand, prediction);

        // Is this an argument?
        if (operandIsArgument(operand)) {
            setArgument(operand, value);
            return;
        }

        // Must be a local.
        setLocal((unsigned)operand, value);
    }

    // Used in implementing get/set, above, where the operand is a local variable.
    NodeIndex getLocal(unsigned operand)
    {
        NodeIndex nodeIndex = m_currentBlock->m_locals[operand].value;

        if (nodeIndex != NoNode) {
            Node& node = m_graph[nodeIndex];
            if (node.op == GetLocal)
                return nodeIndex;
            ASSERT(node.op == SetLocal);
            return node.child1;
        }

        // Check for reads of temporaries from prior blocks,
        // expand m_preservedVars to cover these.
        m_preservedVars = std::max(m_preservedVars, operand + 1);

        NodeIndex phi = addToGraph(Phi);
        m_localPhiStack.append(PhiStackEntry(m_currentBlock, phi, operand));
        nodeIndex = addToGraph(GetLocal, OpInfo(operand), phi);
        m_currentBlock->m_locals[operand].value = nodeIndex;
        return nodeIndex;
    }
    void setLocal(unsigned operand, NodeIndex value)
    {
        m_currentBlock->m_locals[operand].value = addToGraph(SetLocal, OpInfo(operand), value);
    }

    // Used in implementing get/set, above, where the operand is an argument.
    NodeIndex getArgument(unsigned operand)
    {
        unsigned argument = operand + m_codeBlock->m_numParameters + RegisterFile::CallFrameHeaderSize;
        ASSERT(argument < m_numArguments);

        NodeIndex nodeIndex = m_currentBlock->m_arguments[argument].value;

        if (nodeIndex != NoNode) {
            Node& node = m_graph[nodeIndex];
            if (node.op == GetLocal)
                return nodeIndex;
            ASSERT(node.op == SetLocal);
            return node.child1;
        }

        NodeIndex phi = addToGraph(Phi);
        m_argumentPhiStack.append(PhiStackEntry(m_currentBlock, phi, argument));
        nodeIndex = addToGraph(GetLocal, OpInfo(operand), phi);
        m_currentBlock->m_arguments[argument].value = nodeIndex;
        return nodeIndex;
    }
    void setArgument(int operand, NodeIndex value)
    {
        unsigned argument = operand + m_codeBlock->m_numParameters + RegisterFile::CallFrameHeaderSize;
        ASSERT(argument < m_numArguments);

        m_currentBlock->m_arguments[argument].value = addToGraph(SetLocal, OpInfo(operand), value);
    }

    // Get an operand, and perform a ToInt32/ToNumber conversion on it.
    NodeIndex getToInt32(int operand)
    {
        // Avoid wastefully adding a JSConstant node to the graph, only to
        // replace it with a Int32Constant (which is what would happen if
        // we called 'toInt32(get(operand))' in this case).
        if (operand >= FirstConstantRegisterIndex) {
            JSValue v = m_codeBlock->getConstant(operand);
            if (v.isInt32())
                return getInt32Constant(v.asInt32(), operand - FirstConstantRegisterIndex);
        }
        return toInt32(get(operand));
    }
    NodeIndex getToNumber(int operand)
    {
        // Avoid wastefully adding a JSConstant node to the graph, only to
        // replace it with a DoubleConstant (which is what would happen if
        // we called 'toNumber(get(operand))' in this case).
        if (operand >= FirstConstantRegisterIndex) {
            JSValue v = m_codeBlock->getConstant(operand);
            if (v.isNumber())
                return getDoubleConstant(v.uncheckedGetNumber(), operand - FirstConstantRegisterIndex);
        }
        return toNumber(get(operand));
    }

    // Perform an ES5 ToInt32 operation - returns a node of type NodeResultInt32.
    NodeIndex toInt32(NodeIndex index)
    {
        Node& node = m_graph[index];

        if (node.hasInt32Result())
            return index;

        if (node.hasDoubleResult()) {
            if (node.op == DoubleConstant)
                return getInt32Constant(JSC::toInt32(valueOfDoubleConstant(index)), node.constantNumber());
            // 'NumberToInt32(Int32ToNumber(X))' == X, and 'NumberToInt32(UInt32ToNumber(X)) == X'
            if (node.op == Int32ToNumber || node.op == UInt32ToNumber)
                return node.child1;

            // We unique NumberToInt32 nodes in a map to prevent duplicate conversions.
            pair<UnaryOpMap::iterator, bool> result = m_numberToInt32Nodes.add(index, NoNode);
            // Either we added a new value, or the existing value in the map is non-zero.
            ASSERT(result.second == (result.first->second == NoNode));
            if (result.second)
                result.first->second = addToGraph(NumberToInt32, index);
            return result.first->second;
        }

        // Check for numeric constants boxed as JSValues.
        if (node.op == JSConstant) {
            JSValue v = valueOfJSConstant(index);
            if (v.isInt32())
                return getInt32Constant(v.asInt32(), node.constantNumber());
            if (v.isNumber())
                return getInt32Constant(JSC::toInt32(v.uncheckedGetNumber()), node.constantNumber());
        }

        return addToGraph(ValueToInt32, index);
    }

    // Perform an ES5 ToNumber operation - returns a node of type NodeResultDouble.
    NodeIndex toNumber(NodeIndex index)
    {
        Node& node = m_graph[index];

        if (node.hasDoubleResult())
            return index;

        if (node.hasInt32Result()) {
            if (node.op == Int32Constant)
                return getDoubleConstant(valueOfInt32Constant(index), node.constantNumber());

            // We unique Int32ToNumber nodes in a map to prevent duplicate conversions.
            pair<UnaryOpMap::iterator, bool> result = m_int32ToNumberNodes.add(index, NoNode);
            // Either we added a new value, or the existing value in the map is non-zero.
            ASSERT(result.second == (result.first->second == NoNode));
            if (result.second)
                result.first->second = addToGraph(Int32ToNumber, index);
            return result.first->second;
        }

        if (node.op == JSConstant) {
            JSValue v = valueOfJSConstant(index);
            if (v.isNumber())
                return getDoubleConstant(v.uncheckedGetNumber(), node.constantNumber());
        }

        return addToGraph(ValueToNumber, index);
    }


    // Used in implementing get, above, where the operand is a constant.
    NodeIndex getInt32Constant(int32_t value, unsigned constant)
    {
        NodeIndex index = m_constants[constant].asInt32;
        if (index != NoNode)
            return index;
        NodeIndex resultIndex = addToGraph(Int32Constant, OpInfo(constant));
        m_graph[resultIndex].setInt32Constant(value);
        m_constants[constant].asInt32 = resultIndex;
        return resultIndex;
    }
    NodeIndex getDoubleConstant(double value, unsigned constant)
    {
        NodeIndex index = m_constants[constant].asNumeric;
        if (index != NoNode)
            return index;
        NodeIndex resultIndex = addToGraph(DoubleConstant, OpInfo(constant));
        m_graph[resultIndex].setDoubleConstant(value);
        m_constants[constant].asNumeric = resultIndex;
        return resultIndex;
    }
    NodeIndex getJSConstant(unsigned constant)
    {
        NodeIndex index = m_constants[constant].asJSValue;
        if (index != NoNode)
            return index;

        NodeIndex resultIndex = addToGraph(JSConstant, OpInfo(constant));
        m_constants[constant].asJSValue = resultIndex;
        return resultIndex;
    }

    // Helper functions to get/set the this value.
    NodeIndex getThis()
    {
        return getArgument(m_codeBlock->thisRegister());
    }
    void setThis(NodeIndex value)
    {
        setArgument(m_codeBlock->thisRegister(), value);
    }

    // Convenience methods for checking nodes for constants.
    bool isInt32Constant(NodeIndex index)
    {
        return m_graph[index].op == Int32Constant;
    }
    bool isDoubleConstant(NodeIndex index)
    {
        return m_graph[index].op == DoubleConstant;
    }
    bool isJSConstant(NodeIndex index)
    {
        return m_graph[index].op == JSConstant;
    }

    // Convenience methods for getting constant values.
    int32_t valueOfInt32Constant(NodeIndex index)
    {
        ASSERT(isInt32Constant(index));
        return m_graph[index].int32Constant();
    }
    double valueOfDoubleConstant(NodeIndex index)
    {
        ASSERT(isDoubleConstant(index));
        return m_graph[index].numericConstant();
    }
    JSValue valueOfJSConstant(NodeIndex index)
    {
        ASSERT(isJSConstant(index));
        return m_codeBlock->getConstant(FirstConstantRegisterIndex + m_graph[index].constantNumber());
    }

    // This method returns a JSConstant with the value 'undefined'.
    NodeIndex constantUndefined()
    {
        // Has m_constantUndefined been set up yet?
        if (m_constantUndefined == UINT_MAX) {
            // Search the constant pool for undefined, if we find it, we can just reuse this!
            unsigned numberOfConstants = m_codeBlock->numberOfConstantRegisters();
            for (m_constantUndefined = 0; m_constantUndefined < numberOfConstants; ++m_constantUndefined) {
                JSValue testMe = m_codeBlock->getConstant(FirstConstantRegisterIndex + m_constantUndefined);
                if (testMe.isUndefined())
                    return getJSConstant(m_constantUndefined);
            }

            // Add undefined to the CodeBlock's constants, and add a corresponding slot in m_constants.
            ASSERT(m_constants.size() == numberOfConstants);
            m_codeBlock->addConstant(jsUndefined());
            m_constants.append(ConstantRecord());
            ASSERT(m_constants.size() == m_codeBlock->numberOfConstantRegisters());
        }

        // m_constantUndefined must refer to an entry in the CodeBlock's constant pool that has the value 'undefined'.
        ASSERT(m_codeBlock->getConstant(FirstConstantRegisterIndex + m_constantUndefined).isUndefined());
        return getJSConstant(m_constantUndefined);
    }

    // This method returns a JSConstant with the value 'null'.
    NodeIndex constantNull()
    {
        // Has m_constantNull been set up yet?
        if (m_constantNull == UINT_MAX) {
            // Search the constant pool for null, if we find it, we can just reuse this!
            unsigned numberOfConstants = m_codeBlock->numberOfConstantRegisters();
            for (m_constantNull = 0; m_constantNull < numberOfConstants; ++m_constantNull) {
                JSValue testMe = m_codeBlock->getConstant(FirstConstantRegisterIndex + m_constantNull);
                if (testMe.isNull())
                    return getJSConstant(m_constantNull);
            }

            // Add null to the CodeBlock's constants, and add a corresponding slot in m_constants.
            ASSERT(m_constants.size() == numberOfConstants);
            m_codeBlock->addConstant(jsNull());
            m_constants.append(ConstantRecord());
            ASSERT(m_constants.size() == m_codeBlock->numberOfConstantRegisters());
        }

        // m_constantNull must refer to an entry in the CodeBlock's constant pool that has the value 'null'.
        ASSERT(m_codeBlock->getConstant(FirstConstantRegisterIndex + m_constantNull).isNull());
        return getJSConstant(m_constantNull);
    }

    // This method returns a DoubleConstant with the value 1.
    NodeIndex one()
    {
        // Has m_constant1 been set up yet?
        if (m_constant1 == UINT_MAX) {
            // Search the constant pool for the value 1, if we find it, we can just reuse this!
            unsigned numberOfConstants = m_codeBlock->numberOfConstantRegisters();
            for (m_constant1 = 0; m_constant1 < numberOfConstants; ++m_constant1) {
                JSValue testMe = m_codeBlock->getConstant(FirstConstantRegisterIndex + m_constant1);
                if (testMe.isInt32() && testMe.asInt32() == 1)
                    return getDoubleConstant(1, m_constant1);
            }

            // Add the value 1 to the CodeBlock's constants, and add a corresponding slot in m_constants.
            ASSERT(m_constants.size() == numberOfConstants);
            m_codeBlock->addConstant(jsNumber(1));
            m_constants.append(ConstantRecord());
            ASSERT(m_constants.size() == m_codeBlock->numberOfConstantRegisters());
        }

        // m_constant1 must refer to an entry in the CodeBlock's constant pool that has the integer value 1.
        ASSERT(m_codeBlock->getConstant(FirstConstantRegisterIndex + m_constant1).isInt32());
        ASSERT(m_codeBlock->getConstant(FirstConstantRegisterIndex + m_constant1).asInt32() == 1);
        return getDoubleConstant(1, m_constant1);
    }


    // These methods create a node and add it to the graph. If nodes of this type are
    // 'mustGenerate' then the node  will implicitly be ref'ed to ensure generation.
    NodeIndex addToGraph(NodeType op, NodeIndex child1 = NoNode, NodeIndex child2 = NoNode, NodeIndex child3 = NoNode)
    {
        NodeIndex resultIndex = (NodeIndex)m_graph.size();
        m_graph.append(Node(op, m_currentIndex, child1, child2, child3));

        if (op & NodeMustGenerate)
            m_graph.ref(resultIndex);
        return resultIndex;
    }
    NodeIndex addToGraph(NodeType op, OpInfo info, NodeIndex child1 = NoNode, NodeIndex child2 = NoNode, NodeIndex child3 = NoNode)
    {
        NodeIndex resultIndex = (NodeIndex)m_graph.size();
        m_graph.append(Node(op, m_currentIndex, info, child1, child2, child3));

        if (op & NodeMustGenerate)
            m_graph.ref(resultIndex);
        return resultIndex;
    }
    NodeIndex addToGraph(NodeType op, OpInfo info1, OpInfo info2, NodeIndex child1 = NoNode, NodeIndex child2 = NoNode, NodeIndex child3 = NoNode)
    {
        NodeIndex resultIndex = (NodeIndex)m_graph.size();
        m_graph.append(Node(op, m_currentIndex, info1, info2, child1, child2, child3));

        if (op & NodeMustGenerate)
            m_graph.ref(resultIndex);
        return resultIndex;
    }

    void predictArray(NodeIndex nodeIndex)
    {
        Node* nodePtr = &m_graph[nodeIndex];

        if (nodePtr->op == GetLocal)
            m_graph.predict(nodePtr->local(), PredictArray);
    }

    void predictInt32(NodeIndex nodeIndex)
    {
        Node* nodePtr = &m_graph[nodeIndex];

        if (nodePtr->op == ValueToNumber)
            nodePtr = &m_graph[nodePtr->child1];

        if (nodePtr->op == ValueToInt32)
            nodePtr = &m_graph[nodePtr->child1];

        if (nodePtr->op == NumberToInt32)
            nodePtr = &m_graph[nodePtr->child1];

        if (nodePtr->op == GetLocal)
            m_graph.predict(nodePtr->local(), PredictInt32);
    }

    JSGlobalData* m_globalData;
    CodeBlock* m_codeBlock;
    Graph& m_graph;

    // The current block being generated.
    BasicBlock* m_currentBlock;
    // The bytecode index of the current instruction being generated.
    unsigned m_currentIndex;

    // Record failures due to unimplemented functionality or regressions.
    bool m_parseFailed;

    // We use these values during code generation, and to avoid the need for
    // special handling we make sure they are available as constants in the
    // CodeBlock's constant pool. These variables are initialized to
    // UINT_MAX, and lazily updated to hold an index into the CodeBlock's
    // constant pool, as necessary.
    unsigned m_constantUndefined;
    unsigned m_constantNull;
    unsigned m_constant1;

    // A constant in the constant pool may be represented by more than one
    // node in the graph, depending on the context in which it is being used.
    struct ConstantRecord {
        ConstantRecord()
            : asInt32(NoNode)
            , asNumeric(NoNode)
            , asJSValue(NoNode)
        {
        }

        NodeIndex asInt32;
        NodeIndex asNumeric;
        NodeIndex asJSValue;
    };

    // Track the index of the node whose result is the current value for every
    // register value in the bytecode - argument, local, and temporary.
    Vector<ConstantRecord, 16> m_constants;

    // The number of arguments passed to the function.
    unsigned m_numArguments;
    // The number of locals (vars + temporaries) used in the function.
    unsigned m_numLocals;
    // The number of registers we need to preserve across BasicBlock boundaries;
    // typically equal to the number vars, but we expand this to cover all
    // temporaries that persist across blocks (dues to ?:, &&, ||, etc).
    unsigned m_preservedVars;

    struct PhiStackEntry {
        PhiStackEntry(BasicBlock* block, NodeIndex phi, unsigned varNo)
            : m_block(block)
            , m_phi(phi)
            , m_varNo(varNo)
        {
        }

        BasicBlock* m_block;
        NodeIndex m_phi;
        unsigned m_varNo;
    };
    Vector<PhiStackEntry, 16> m_argumentPhiStack;
    Vector<PhiStackEntry, 16> m_localPhiStack;

    // These maps are used to unique ToNumber and ToInt32 operations.
    typedef HashMap<NodeIndex, NodeIndex> UnaryOpMap;
    UnaryOpMap m_int32ToNumberNodes;
    UnaryOpMap m_numberToInt32Nodes;
};

#define NEXT_OPCODE(name) \
    m_currentIndex += OPCODE_LENGTH(name); \
    continue

#define LAST_OPCODE(name) \
    m_currentIndex += OPCODE_LENGTH(name); \
    return !m_parseFailed

bool ByteCodeParser::parseBlock(unsigned limit)
{
    // No need to reset state initially, since it has been set by the constructor.
    if (m_currentIndex) {
        for (unsigned i = 0; i < m_constants.size(); ++i)
            m_constants[i] = ConstantRecord();
    }

    AliasTracker aliases(m_graph);

    Interpreter* interpreter = m_globalData->interpreter;
    Instruction* instructionsBegin = m_codeBlock->instructions().begin();
    while (true) {
        // Don't extend over jump destinations.
        if (m_currentIndex == limit) {
            addToGraph(Jump, OpInfo(m_currentIndex));
            return !m_parseFailed;
        }

        // Switch on the current bytecode opcode.
        Instruction* currentInstruction = instructionsBegin + m_currentIndex;
        switch (interpreter->getOpcodeID(currentInstruction->u.opcode)) {

        // === Function entry opcodes ===

        case op_enter:
            // Initialize all locals to undefined.
            for (int i = 0; i < m_codeBlock->m_numVars; ++i)
                set(i, constantUndefined());
            NEXT_OPCODE(op_enter);

        case op_convert_this: {
            NodeIndex op1 = getThis();
            setThis(addToGraph(ConvertThis, op1));
            NEXT_OPCODE(op_convert_this);
        }

        // === Bitwise operations ===

        case op_bitand: {
            NodeIndex op1 = getToInt32(currentInstruction[2].u.operand);
            NodeIndex op2 = getToInt32(currentInstruction[3].u.operand);
            predictInt32(op1);
            predictInt32(op2);
            set(currentInstruction[1].u.operand, addToGraph(BitAnd, op1, op2), PredictInt32);
            NEXT_OPCODE(op_bitand);
        }

        case op_bitor: {
            NodeIndex op1 = getToInt32(currentInstruction[2].u.operand);
            NodeIndex op2 = getToInt32(currentInstruction[3].u.operand);
            predictInt32(op1);
            predictInt32(op2);
            set(currentInstruction[1].u.operand, addToGraph(BitOr, op1, op2), PredictInt32);
            NEXT_OPCODE(op_bitor);
        }

        case op_bitxor: {
            NodeIndex op1 = getToInt32(currentInstruction[2].u.operand);
            NodeIndex op2 = getToInt32(currentInstruction[3].u.operand);
            predictInt32(op1);
            predictInt32(op2);
            set(currentInstruction[1].u.operand, addToGraph(BitXor, op1, op2), PredictInt32);
            NEXT_OPCODE(op_bitxor);
        }

        case op_rshift: {
            NodeIndex op1 = getToInt32(currentInstruction[2].u.operand);
            NodeIndex op2 = getToInt32(currentInstruction[3].u.operand);
            predictInt32(op1);
            predictInt32(op2);
            NodeIndex result;
            // Optimize out shifts by zero.
            if (isInt32Constant(op2) && !(valueOfInt32Constant(op2) & 0x1f))
                result = op1;
            else
                result = addToGraph(BitRShift, op1, op2);
            set(currentInstruction[1].u.operand, result, PredictInt32);
            NEXT_OPCODE(op_rshift);
        }

        case op_lshift: {
            NodeIndex op1 = getToInt32(currentInstruction[2].u.operand);
            NodeIndex op2 = getToInt32(currentInstruction[3].u.operand);
            predictInt32(op1);
            predictInt32(op2);
            NodeIndex result;
            // Optimize out shifts by zero.
            if (isInt32Constant(op2) && !(valueOfInt32Constant(op2) & 0x1f))
                result = op1;
            else
                result = addToGraph(BitLShift, op1, op2);
            set(currentInstruction[1].u.operand, result, PredictInt32);
            NEXT_OPCODE(op_lshift);
        }

        case op_urshift: {
            NodeIndex op1 = getToInt32(currentInstruction[2].u.operand);
            NodeIndex op2 = getToInt32(currentInstruction[3].u.operand);
            predictInt32(op1);
            predictInt32(op2);
            NodeIndex result;
            // The result of a zero-extending right shift is treated as an unsigned value.
            // This means that if the top bit is set, the result is not in the int32 range,
            // and as such must be stored as a double. If the shift amount is a constant,
            // we may be able to optimize.
            if (isInt32Constant(op2)) {
                // If we know we are shifting by a non-zero amount, then since the operation
                // zero fills we know the top bit of the result must be zero, and as such the
                // result must be within the int32 range. Conversely, if this is a shift by
                // zero, then the result may be changed by the conversion to unsigned, but it
                // is not necessary to perform the shift!
                if (valueOfInt32Constant(op2) & 0x1f)
                    result = addToGraph(BitURShift, op1, op2);
                else
                    result = addToGraph(UInt32ToNumber, op1);
            }  else {
                // Cannot optimize at this stage; shift & potentially rebox as a double.
                result = addToGraph(BitURShift, op1, op2);
                result = addToGraph(UInt32ToNumber, result);
            }
            set(currentInstruction[1].u.operand, result, PredictInt32);
            NEXT_OPCODE(op_urshift);
        }

        // === Increment/Decrement opcodes ===

        case op_pre_inc: {
            unsigned srcDst = currentInstruction[1].u.operand;
            NodeIndex op = getToNumber(srcDst);
            predictInt32(op);
            set(srcDst, addToGraph(ArithAdd, op, one()));
            NEXT_OPCODE(op_pre_inc);
        }

        case op_post_inc: {
            unsigned result = currentInstruction[1].u.operand;
            unsigned srcDst = currentInstruction[2].u.operand;
            NodeIndex op = getToNumber(srcDst);
            predictInt32(op);
            set(result, op);
            set(srcDst, addToGraph(ArithAdd, op, one()));
            NEXT_OPCODE(op_post_inc);
        }

        case op_pre_dec: {
            unsigned srcDst = currentInstruction[1].u.operand;
            NodeIndex op = getToNumber(srcDst);
            predictInt32(op);
            set(srcDst, addToGraph(ArithSub, op, one()));
            NEXT_OPCODE(op_pre_dec);
        }

        case op_post_dec: {
            unsigned result = currentInstruction[1].u.operand;
            unsigned srcDst = currentInstruction[2].u.operand;
            NodeIndex op = getToNumber(srcDst);
            predictInt32(op);
            set(result, op);
            set(srcDst, addToGraph(ArithSub, op, one()));
            NEXT_OPCODE(op_post_dec);
        }

        // === Arithmetic operations ===

        case op_add: {
            ARITHMETIC_OP();
            NodeIndex op1 = get(currentInstruction[2].u.operand);
            NodeIndex op2 = get(currentInstruction[3].u.operand);
            // If both operands can statically be determined to the numbers, then this is an arithmetic add.
            // Otherwise, we must assume this may be performing a concatenation to a string.
            if (m_graph[op1].hasNumericResult() && m_graph[op2].hasNumericResult())
                set(currentInstruction[1].u.operand, addToGraph(ArithAdd, toNumber(op1), toNumber(op2)));
            else
                set(currentInstruction[1].u.operand, addToGraph(ValueAdd, op1, op2));
            NEXT_OPCODE(op_add);
        }

        case op_sub: {
            ARITHMETIC_OP();
            NodeIndex op1 = getToNumber(currentInstruction[2].u.operand);
            NodeIndex op2 = getToNumber(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(ArithSub, op1, op2));
            NEXT_OPCODE(op_sub);
        }

        case op_mul: {
            ARITHMETIC_OP();
            NodeIndex op1 = getToNumber(currentInstruction[2].u.operand);
            NodeIndex op2 = getToNumber(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(ArithMul, op1, op2));
            NEXT_OPCODE(op_mul);
        }

        case op_mod: {
            ARITHMETIC_OP();
            NodeIndex op1 = getToNumber(currentInstruction[2].u.operand);
            NodeIndex op2 = getToNumber(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(ArithMod, op1, op2));
            NEXT_OPCODE(op_mod);
        }

        case op_div: {
            ARITHMETIC_OP();
            NodeIndex op1 = getToNumber(currentInstruction[2].u.operand);
            NodeIndex op2 = getToNumber(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(ArithDiv, op1, op2));
            NEXT_OPCODE(op_div);
        }

        // === Misc operations ===

        case op_mov: {
            NodeIndex op = get(currentInstruction[2].u.operand);
            set(currentInstruction[1].u.operand, op);
            NEXT_OPCODE(op_mov);
        }

        case op_not: {
            ARITHMETIC_OP();
            NodeIndex value = get(currentInstruction[2].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(LogicalNot, value));
            NEXT_OPCODE(op_not);
        }

        case op_less: {
            ARITHMETIC_OP();
            NodeIndex op1 = get(currentInstruction[2].u.operand);
            NodeIndex op2 = get(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(CompareLess, op1, op2));
            NEXT_OPCODE(op_less);
        }

        case op_lesseq: {
            ARITHMETIC_OP();
            NodeIndex op1 = get(currentInstruction[2].u.operand);
            NodeIndex op2 = get(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(CompareLessEq, op1, op2));
            NEXT_OPCODE(op_lesseq);
        }

        case op_eq: {
            ARITHMETIC_OP();
            NodeIndex op1 = get(currentInstruction[2].u.operand);
            NodeIndex op2 = get(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(CompareEq, op1, op2));
            NEXT_OPCODE(op_eq);
        }

        case op_eq_null: {
            ARITHMETIC_OP();
            NodeIndex value = get(currentInstruction[2].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(CompareEq, value, constantNull()));
            NEXT_OPCODE(op_eq_null);
        }

        case op_stricteq: {
            ARITHMETIC_OP();
            NodeIndex op1 = get(currentInstruction[2].u.operand);
            NodeIndex op2 = get(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(CompareStrictEq, op1, op2));
            NEXT_OPCODE(op_stricteq);
        }

        case op_neq: {
            ARITHMETIC_OP();
            NodeIndex op1 = get(currentInstruction[2].u.operand);
            NodeIndex op2 = get(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(LogicalNot, addToGraph(CompareEq, op1, op2)));
            NEXT_OPCODE(op_neq);
        }

        case op_neq_null: {
            ARITHMETIC_OP();
            NodeIndex value = get(currentInstruction[2].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(LogicalNot, addToGraph(CompareEq, value, constantNull())));
            NEXT_OPCODE(op_neq_null);
        }

        case op_nstricteq: {
            ARITHMETIC_OP();
            NodeIndex op1 = get(currentInstruction[2].u.operand);
            NodeIndex op2 = get(currentInstruction[3].u.operand);
            set(currentInstruction[1].u.operand, addToGraph(LogicalNot, addToGraph(CompareStrictEq, op1, op2)));
            NEXT_OPCODE(op_nstricteq);
        }

        // === Property access operations ===

        case op_get_by_val: {
            NodeIndex base = get(currentInstruction[2].u.operand);
            NodeIndex property = get(currentInstruction[3].u.operand);
            predictArray(base);
            predictInt32(property);

            NodeIndex getByVal = addToGraph(GetByVal, base, property, aliases.lookupGetByVal(base, property));
            set(currentInstruction[1].u.operand, getByVal);
            aliases.recordGetByVal(getByVal);

            NEXT_OPCODE(op_get_by_val);
        }

        case op_put_by_val: {
            NodeIndex base = get(currentInstruction[1].u.operand);
            NodeIndex property = get(currentInstruction[2].u.operand);
            NodeIndex value = get(currentInstruction[3].u.operand);
            predictArray(base);
            predictInt32(property);

            NodeIndex aliasedGet = aliases.lookupGetByVal(base, property);
            NodeIndex putByVal = addToGraph(aliasedGet != NoNode ? PutByValAlias : PutByVal, base, property, value);
            aliases.recordPutByVal(putByVal);

            NEXT_OPCODE(op_put_by_val);
        }

        case op_get_by_id: {
            NodeIndex base = get(currentInstruction[2].u.operand);
            unsigned identifier = currentInstruction[3].u.operand;

            NodeIndex getById = addToGraph(GetById, OpInfo(identifier), base);
            set(currentInstruction[1].u.operand, getById);
            aliases.recordGetById(getById);

            NEXT_OPCODE(op_get_by_id);
        }

        case op_put_by_id: {
            NodeIndex value = get(currentInstruction[3].u.operand);
            NodeIndex base = get(currentInstruction[1].u.operand);
            unsigned identifier = currentInstruction[2].u.operand;
            bool direct = currentInstruction[8].u.operand;

            if (direct) {
                NodeIndex putByIdDirect = addToGraph(PutByIdDirect, OpInfo(identifier), base, value);
                aliases.recordPutByIdDirect(putByIdDirect);
            } else {
                NodeIndex putById = addToGraph(PutById, OpInfo(identifier), base, value);
                aliases.recordPutById(putById);
            }

            NEXT_OPCODE(op_put_by_id);
        }

        case op_get_global_var: {
            NodeIndex getGlobalVar = addToGraph(GetGlobalVar, OpInfo(currentInstruction[2].u.operand));
            set(currentInstruction[1].u.operand, getGlobalVar);
            NEXT_OPCODE(op_get_global_var);
        }

        case op_put_global_var: {
            NodeIndex value = get(currentInstruction[2].u.operand);
            addToGraph(PutGlobalVar, OpInfo(currentInstruction[1].u.operand), value);
            NEXT_OPCODE(op_put_global_var);
        }

        // === Block terminators. ===

        case op_jmp: {
            unsigned relativeOffset = currentInstruction[1].u.operand;
            addToGraph(Jump, OpInfo(m_currentIndex + relativeOffset));
            LAST_OPCODE(op_jmp);
        }

        case op_loop: {
            unsigned relativeOffset = currentInstruction[1].u.operand;
            addToGraph(Jump, OpInfo(m_currentIndex + relativeOffset));
            LAST_OPCODE(op_loop);
        }

        case op_jtrue: {
            unsigned relativeOffset = currentInstruction[2].u.operand;
            NodeIndex condition = get(currentInstruction[1].u.operand);
            addToGraph(Branch, OpInfo(m_currentIndex + relativeOffset), OpInfo(m_currentIndex + OPCODE_LENGTH(op_jtrue)), condition);
            LAST_OPCODE(op_jtrue);
        }

        case op_jfalse: {
            unsigned relativeOffset = currentInstruction[2].u.operand;
            NodeIndex condition = get(currentInstruction[1].u.operand);
            addToGraph(Branch, OpInfo(m_currentIndex + OPCODE_LENGTH(op_jfalse)), OpInfo(m_currentIndex + relativeOffset), condition);
            LAST_OPCODE(op_jfalse);
        }

        case op_loop_if_true: {
            unsigned relativeOffset = currentInstruction[2].u.operand;
            NodeIndex condition = get(currentInstruction[1].u.operand);
            addToGraph(Branch, OpInfo(m_currentIndex + relativeOffset), OpInfo(m_currentIndex + OPCODE_LENGTH(op_loop_if_true)), condition);
            LAST_OPCODE(op_loop_if_true);
        }

        case op_loop_if_false: {
            unsigned relativeOffset = currentInstruction[2].u.operand;
            NodeIndex condition = get(currentInstruction[1].u.operand);
            addToGraph(Branch, OpInfo(m_currentIndex + OPCODE_LENGTH(op_loop_if_false)), OpInfo(m_currentIndex + relativeOffset), condition);
            LAST_OPCODE(op_loop_if_false);
        }

        case op_jeq_null: {
            unsigned relativeOffset = currentInstruction[2].u.operand;
            NodeIndex value = get(currentInstruction[1].u.operand);
            NodeIndex condition = addToGraph(CompareEq, value, constantNull());
            addToGraph(Branch, OpInfo(m_currentIndex + relativeOffset), OpInfo(m_currentIndex + OPCODE_LENGTH(op_jeq_null)), condition);
            LAST_OPCODE(op_jeq_null);
        }

        case op_jneq_null: {
            unsigned relativeOffset = currentInstruction[2].u.operand;
            NodeIndex value = get(currentInstruction[1].u.operand);
            NodeIndex condition = addToGraph(CompareEq, value, constantNull());
            addToGraph(Branch, OpInfo(m_currentIndex + OPCODE_LENGTH(op_jneq_null)), OpInfo(m_currentIndex + relativeOffset), condition);
            LAST_OPCODE(op_jneq_null);
        }

        case op_jnless: {
            unsigned relativeOffset = currentInstruction[3].u.operand;
            NodeIndex op1 = get(currentInstruction[1].u.operand);
            NodeIndex op2 = get(currentInstruction[2].u.operand);
            NodeIndex condition = addToGraph(CompareLess, op1, op2);
            addToGraph(Branch, OpInfo(m_currentIndex + OPCODE_LENGTH(op_jnless)), OpInfo(m_currentIndex + relativeOffset), condition);
            LAST_OPCODE(op_jnless);
        }

        case op_jnlesseq: {
            unsigned relativeOffset = currentInstruction[3].u.operand;
            NodeIndex op1 = get(currentInstruction[1].u.operand);
            NodeIndex op2 = get(currentInstruction[2].u.operand);
            NodeIndex condition = addToGraph(CompareLessEq, op1, op2);
            addToGraph(Branch, OpInfo(m_currentIndex + OPCODE_LENGTH(op_jnlesseq)), OpInfo(m_currentIndex + relativeOffset), condition);
            LAST_OPCODE(op_jnlesseq);
        }

        case op_jless: {
            unsigned relativeOffset = currentInstruction[3].u.operand;
            NodeIndex op1 = get(currentInstruction[1].u.operand);
            NodeIndex op2 = get(currentInstruction[2].u.operand);
            NodeIndex condition = addToGraph(CompareLess, op1, op2);
            addToGraph(Branch, OpInfo(m_currentIndex + relativeOffset), OpInfo(m_currentIndex + OPCODE_LENGTH(op_jless)), condition);
            LAST_OPCODE(op_jless);
        }

        case op_jlesseq: {
            unsigned relativeOffset = currentInstruction[3].u.operand;
            NodeIndex op1 = get(currentInstruction[1].u.operand);
            NodeIndex op2 = get(currentInstruction[2].u.operand);
            NodeIndex condition = addToGraph(CompareLessEq, op1, op2);
            addToGraph(Branch, OpInfo(m_currentIndex + relativeOffset), OpInfo(m_currentIndex + OPCODE_LENGTH(op_jlesseq)), condition);
            LAST_OPCODE(op_jlesseq);
        }

        case op_loop_if_less: {
            unsigned relativeOffset = currentInstruction[3].u.operand;
            NodeIndex op1 = get(currentInstruction[1].u.operand);
            NodeIndex op2 = get(currentInstruction[2].u.operand);
            NodeIndex condition = addToGraph(CompareLess, op1, op2);
            addToGraph(Branch, OpInfo(m_currentIndex + relativeOffset), OpInfo(m_currentIndex + OPCODE_LENGTH(op_loop_if_less)), condition);
            LAST_OPCODE(op_loop_if_less);
        }

        case op_loop_if_lesseq: {
            unsigned relativeOffset = currentInstruction[3].u.operand;
            NodeIndex op1 = get(currentInstruction[1].u.operand);
            NodeIndex op2 = get(currentInstruction[2].u.operand);
            NodeIndex condition = addToGraph(CompareLessEq, op1, op2);
            addToGraph(Branch, OpInfo(m_currentIndex + relativeOffset), OpInfo(m_currentIndex + OPCODE_LENGTH(op_loop_if_lesseq)), condition);
            LAST_OPCODE(op_loop_if_lesseq);
        }

        case op_ret: {
            addToGraph(Return, get(currentInstruction[1].u.operand));
            LAST_OPCODE(op_ret);
        }

        default:
            // Parse failed!
            return false;
        }
    }
}

template<ByteCodeParser::PhiStackType stackType>
void ByteCodeParser::processPhiStack()
{
    Vector<PhiStackEntry, 16>& phiStack = (stackType == ArgumentPhiStack) ? m_argumentPhiStack : m_localPhiStack;

    while (!phiStack.isEmpty()) {
        PhiStackEntry entry = phiStack.last();
        phiStack.removeLast();
        
        Node& phiNode = m_graph[entry.m_phi];
        PredecessorList& predecessors = entry.m_block->m_predecessors;
        unsigned varNo = entry.m_varNo;

        for (size_t i = 0; i < predecessors.size(); ++i) {
            BasicBlock* predecessorBlock = m_graph.m_blocks[predecessors[i]].get();

            VariableRecord& var = (stackType == ArgumentPhiStack) ? predecessorBlock->m_arguments[varNo] : predecessorBlock->m_locals[varNo];

            NodeIndex valueInPredecessor = var.value;
            if (valueInPredecessor == NoNode) {
                valueInPredecessor = addToGraph(Phi);
                var.value = valueInPredecessor;
                phiStack.append(PhiStackEntry(predecessorBlock, valueInPredecessor, varNo));
            } else if (m_graph[valueInPredecessor].op == GetLocal)
                valueInPredecessor = m_graph[valueInPredecessor].child1;
            ASSERT(m_graph[valueInPredecessor].op == SetLocal || m_graph[valueInPredecessor].op == Phi);

            if (phiNode.refCount())
                m_graph.ref(valueInPredecessor);

            if (phiNode.child1 == NoNode) {
                phiNode.child1 = valueInPredecessor;
                continue;
            }
            if (phiNode.child2 == NoNode) {
                phiNode.child2 = valueInPredecessor;
                continue;
            }
            if (phiNode.child3 == NoNode) {
                phiNode.child3 = valueInPredecessor;
                continue;
            }

            NodeIndex newPhi = addToGraph(Phi);
            Node& newPhiNode = m_graph[newPhi];
            if (phiNode.refCount())
                m_graph.ref(newPhi);

            newPhiNode.child1 = phiNode.child1;
            newPhiNode.child2 = phiNode.child2;
            newPhiNode.child3 = phiNode.child3;

            phiNode.child1 = newPhi;
            phiNode.child1 = valueInPredecessor;
            phiNode.child3 = NoNode;
        }
    }
}

void ByteCodeParser::setupPredecessors()
{
    for (BlockIndex index = 0; index < m_graph.m_blocks.size(); ++index) {
        BasicBlock* block = m_graph.m_blocks[index].get();
        ASSERT(block->end != NoNode);
        Node& node = m_graph[block->end - 1];
        ASSERT(node.isTerminal());

        if (node.isJump())
            m_graph.blockForBytecodeOffset(node.takenBytecodeOffset()).m_predecessors.append(index);
        else if (node.isBranch()) {
            m_graph.blockForBytecodeOffset(node.takenBytecodeOffset()).m_predecessors.append(index);
            m_graph.blockForBytecodeOffset(node.notTakenBytecodeOffset()).m_predecessors.append(index);
        }
    }
}

void ByteCodeParser::allocateVirtualRegisters()
{
    ScoreBoard scoreBoard(m_graph, m_preservedVars);
    unsigned sizeExcludingPhiNodes = m_graph.m_blocks.last()->end;
    for (size_t i = 0; i < sizeExcludingPhiNodes; ++i) {
        Node& node = m_graph[i];
        if (!node.shouldGenerate())
            continue;

        // GetLocal nodes are effectively phi nodes in the graph, referencing
        // results from prior blocks.
        if (node.op != GetLocal) {
            // First, call use on all of the current node's children, then
            // allocate a VirtualRegister for this node. We do so in this
            // order so that if a child is on its last use, and a
            // VirtualRegister is freed, then it may be reused for node.
            scoreBoard.use(node.child1);
            scoreBoard.use(node.child2);
            scoreBoard.use(node.child3);
        }

        if (!node.hasResult())
            continue;

        node.setVirtualRegister(scoreBoard.allocate());
        // 'mustGenerate' nodes have their useCount artificially elevated,
        // call use now to account for this.
        if (node.mustGenerate())
            scoreBoard.use(i);
    }

    // 'm_numCalleeRegisters' is the number of locals and temporaries allocated
    // for the function (and checked for on entry). Since we perform a new and
    // different allocation of temporaries, more registers may now be required.
    unsigned calleeRegisters = scoreBoard.allocatedCount() + m_preservedVars;
    if ((unsigned)m_codeBlock->m_numCalleeRegisters < calleeRegisters)
        m_codeBlock->m_numCalleeRegisters = calleeRegisters;
}

bool ByteCodeParser::parse()
{
    // Set during construction.
    ASSERT(!m_currentIndex);

    for (unsigned jumpTargetIndex = 0; jumpTargetIndex <= m_codeBlock->numberOfJumpTargets(); ++jumpTargetIndex) {
        // The maximum bytecode offset to go into the current basicblock is either the next jump target, or the end of the instructions.
        unsigned limit = jumpTargetIndex < m_codeBlock->numberOfJumpTargets() ? m_codeBlock->jumpTarget(jumpTargetIndex) : m_codeBlock->instructions().size();
        ASSERT(m_currentIndex < limit);

        // Loop until we reach the current limit (i.e. next jump target).
        do {
            OwnPtr<BasicBlock> block = adoptPtr(new BasicBlock(m_currentIndex, m_graph.size(), m_numArguments, m_numLocals));
            m_currentBlock = block.get();
            m_graph.m_blocks.append(block.release());

            if (!parseBlock(limit))
                return false;
            // We should not have gone beyond the limit.
            ASSERT(m_currentIndex <= limit);

            m_currentBlock->end = m_graph.size();
        } while (m_currentIndex < limit);
    }

    // Should have reached the end of the instructions.
    ASSERT(m_currentIndex == m_codeBlock->instructions().size());

    setupPredecessors();
    processPhiStack<LocalPhiStack>();
    processPhiStack<ArgumentPhiStack>();

    allocateVirtualRegisters();

#if DFG_DEBUG_VERBOSE
    m_graph.dump(m_codeBlock);
#endif

    return true;
}

bool parse(Graph& graph, JSGlobalData* globalData, CodeBlock* codeBlock)
{
#if DFG_DEBUG_LOCAL_DISBALE
    UNUSED_PARAM(graph);
    UNUSED_PARAM(globalData);
    UNUSED_PARAM(codeBlock);
    return false;
#else
    return ByteCodeParser(globalData, codeBlock, graph).parse();
#endif
}

} } // namespace JSC::DFG

#endif
