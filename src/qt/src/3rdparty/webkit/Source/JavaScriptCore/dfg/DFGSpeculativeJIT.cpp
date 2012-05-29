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
#include "DFGSpeculativeJIT.h"

#if ENABLE(DFG_JIT)

namespace JSC { namespace DFG {

template<bool strict>
GPRReg SpeculativeJIT::fillSpeculateIntInternal(NodeIndex nodeIndex, DataFormat& returnFormat)
{
    Node& node = m_jit.graph()[nodeIndex];
    VirtualRegister virtualRegister = node.virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    switch (info.registerFormat()) {
    case DataFormatNone: {
        GPRReg gpr = allocate();

        if (node.isConstant()) {
            m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
            if (isInt32Constant(nodeIndex)) {
                m_jit.move(MacroAssembler::Imm32(valueOfInt32Constant(nodeIndex)), gpr);
                info.fillInteger(gpr);
                returnFormat = DataFormatInteger;
                return gpr;
            }
            m_jit.move(constantAsJSValueAsImmPtr(nodeIndex), gpr);
        } else {
            DataFormat spillFormat = info.spillFormat();
            ASSERT(spillFormat & DataFormatJS);

            m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);

            if (spillFormat == DataFormatJSInteger) {
                // If we know this was spilled as an integer we can fill without checking.
                if (strict) {
                    m_jit.load32(JITCompiler::addressFor(virtualRegister), gpr);
                    info.fillInteger(gpr);
                    returnFormat = DataFormatInteger;
                    return gpr;
                }
                m_jit.loadPtr(JITCompiler::addressFor(virtualRegister), gpr);
                info.fillJSValue(gpr, DataFormatJSInteger);
                returnFormat = DataFormatJSInteger;
                return gpr;
            }
            m_jit.loadPtr(JITCompiler::addressFor(virtualRegister), gpr);
        }

        // Fill as JSValue, and fall through.
        info.fillJSValue(gpr, DataFormatJSInteger);
        m_gprs.unlock(gpr);
    }

    case DataFormatJS: {
        // Check the value is an integer.
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        speculationCheck(m_jit.branchPtr(MacroAssembler::Below, gpr, GPRInfo::tagTypeNumberRegister));
        info.fillJSValue(gpr, DataFormatJSInteger);
        // If !strict we're done, return.
        if (!strict) {
            returnFormat = DataFormatJSInteger;
            return gpr;
        }
        // else fall through & handle as DataFormatJSInteger.
        m_gprs.unlock(gpr);
    }

    case DataFormatJSInteger: {
        // In a strict fill we need to strip off the value tag.
        if (strict) {
            GPRReg gpr = info.gpr();
            GPRReg result;
            // If the register has already been locked we need to take a copy.
            // If not, we'll zero extend in place, so mark on the info that this is now type DataFormatInteger, not DataFormatJSInteger.
            if (m_gprs.isLocked(gpr))
                result = allocate();
            else {
                m_gprs.lock(gpr);
                info.fillInteger(gpr);
                result = gpr;
            }
            m_jit.zeroExtend32ToPtr(gpr, result);
            returnFormat = DataFormatInteger;
            return result;
        }

        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        returnFormat = DataFormatJSInteger;
        return gpr;
    }

    case DataFormatInteger: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        returnFormat = DataFormatInteger;
        return gpr;
    }

    case DataFormatDouble:
    case DataFormatCell:
    case DataFormatJSDouble:
    case DataFormatJSCell: {
        terminateSpeculativeExecution();
        returnFormat = DataFormatInteger;
        return allocate();
    }
    }

    ASSERT_NOT_REACHED();
    return InvalidGPRReg;
}

SpeculationCheck::SpeculationCheck(MacroAssembler::Jump check, SpeculativeJIT* jit, unsigned recoveryIndex)
    : m_check(check)
    , m_nodeIndex(jit->m_compileIndex)
    , m_recoveryIndex(recoveryIndex)
{
    for (gpr_iterator iter = jit->m_gprs.begin(); iter != jit->m_gprs.end(); ++iter) {
        if (iter.name() != InvalidVirtualRegister) {
            GenerationInfo& info =  jit->m_generationInfo[iter.name()];
            m_gprInfo[iter.index()].nodeIndex = info.nodeIndex();
            m_gprInfo[iter.index()].format = info.registerFormat();
        } else
            m_gprInfo[iter.index()].nodeIndex = NoNode;
    }
    for (fpr_iterator iter = jit->m_fprs.begin(); iter != jit->m_fprs.end(); ++iter) {
        if (iter.name() != InvalidVirtualRegister) {
            GenerationInfo& info =  jit->m_generationInfo[iter.name()];
            ASSERT(info.registerFormat() == DataFormatDouble);
            m_fprInfo[iter.index()] = info.nodeIndex();
        } else
            m_fprInfo[iter.index()] = NoNode;
    }
}

GPRReg SpeculativeJIT::fillSpeculateInt(NodeIndex nodeIndex, DataFormat& returnFormat)
{
    return fillSpeculateIntInternal<false>(nodeIndex, returnFormat);
}

GPRReg SpeculativeJIT::fillSpeculateIntStrict(NodeIndex nodeIndex)
{
    DataFormat mustBeDataFormatInteger;
    GPRReg result = fillSpeculateIntInternal<true>(nodeIndex, mustBeDataFormatInteger);
    ASSERT(mustBeDataFormatInteger == DataFormatInteger);
    return result;
}

GPRReg SpeculativeJIT::fillSpeculateCell(NodeIndex nodeIndex)
{
    Node& node = m_jit.graph()[nodeIndex];
    VirtualRegister virtualRegister = node.virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    switch (info.registerFormat()) {
    case DataFormatNone: {
        GPRReg gpr = allocate();

        if (node.isConstant()) {
            m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
            JSValue jsValue = constantAsJSValue(nodeIndex);
            if (jsValue.isCell()) {
                m_jit.move(MacroAssembler::TrustedImmPtr(jsValue.asCell()), gpr);
                info.fillJSValue(gpr, DataFormatJSCell);
                return gpr;
            }
            terminateSpeculativeExecution();
            return gpr;
        }
        ASSERT(info.spillFormat() & DataFormatJS);
        m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
        m_jit.loadPtr(JITCompiler::addressFor(virtualRegister), gpr);

        if (info.spillFormat() != DataFormatJSCell)
            speculationCheck(m_jit.branchTestPtr(MacroAssembler::NonZero, gpr, GPRInfo::tagMaskRegister));
        info.fillJSValue(gpr, DataFormatJSCell);
        return gpr;
    }

    case DataFormatCell:
    case DataFormatJSCell: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        return gpr;
    }

    case DataFormatJS: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        speculationCheck(m_jit.branchTestPtr(MacroAssembler::NonZero, gpr, GPRInfo::tagMaskRegister));
        info.fillJSValue(gpr, DataFormatJSCell);
        return gpr;
    }

    case DataFormatJSInteger:
    case DataFormatInteger:
    case DataFormatJSDouble:
    case DataFormatDouble: {
        terminateSpeculativeExecution();
        return allocate();
    }
    }

    ASSERT_NOT_REACHED();
    return InvalidGPRReg;
}

void SpeculativeJIT::compilePeepHoleBranch(Node& node, JITCompiler::RelationalCondition condition)
{
    Node& branchNode = m_jit.graph()[m_compileIndex + 1];
    BlockIndex taken = m_jit.graph().blockIndexForBytecodeOffset(branchNode.takenBytecodeOffset());
    BlockIndex notTaken = m_jit.graph().blockIndexForBytecodeOffset(branchNode.notTakenBytecodeOffset());

    // The branch instruction will branch to the taken block.
    // If taken is next, switch taken with notTaken & invert the branch condition so we can fall through.
    if (taken == (m_block + 1)) {
        condition = JITCompiler::invert(condition);
        BlockIndex tmp = taken;
        taken = notTaken;
        notTaken = tmp;
    }

    int32_t imm;
    if (isJSConstantWithInt32Value(node.child1, imm)) {
        SpeculateIntegerOperand op2(this, node.child2);
        addBranch(m_jit.branch32(condition, JITCompiler::Imm32(imm), op2.gpr()), taken);
    } else if (isJSConstantWithInt32Value(node.child2, imm)) {
        SpeculateIntegerOperand op1(this, node.child1);
        addBranch(m_jit.branch32(condition, op1.gpr(), JITCompiler::Imm32(imm)), taken);
    } else {
        SpeculateIntegerOperand op1(this, node.child1);
        SpeculateIntegerOperand op2(this, node.child2);
        addBranch(m_jit.branch32(condition, op1.gpr(), op2.gpr()), taken);
    }

    // Check for fall through, otherwise we need to jump.
    if (notTaken != (m_block + 1))
        addBranch(m_jit.jump(), notTaken);
}

void SpeculativeJIT::compile(Node& node)
{
    NodeType op = node.op;

    switch (op) {
    case Int32Constant:
    case DoubleConstant:
    case JSConstant:
        initConstantInfo(m_compileIndex);
        break;

    case GetLocal: {
        GPRTemporary result(this);
        PredictedType prediction = m_jit.graph().getPrediction(node.local());
        if (prediction == PredictInt32) {
            m_jit.load32(JITCompiler::payloadFor(node.local()), result.gpr());

            // Like integerResult, but don't useChildren - our children are phi nodes,
            // and don't represent values within this dataflow with virtual registers.
            VirtualRegister virtualRegister = node.virtualRegister();
            m_gprs.retain(result.gpr(), virtualRegister, SpillOrderInteger);
            m_generationInfo[virtualRegister].initInteger(m_compileIndex, node.refCount(), result.gpr());
        } else {
            m_jit.loadPtr(JITCompiler::addressFor(node.local()), result.gpr());

            // Like jsValueResult, but don't useChildren - our children are phi nodes,
            // and don't represent values within this dataflow with virtual registers.
            VirtualRegister virtualRegister = node.virtualRegister();
            m_gprs.retain(result.gpr(), virtualRegister, SpillOrderJS);
            m_generationInfo[virtualRegister].initJSValue(m_compileIndex, node.refCount(), result.gpr(), (prediction == PredictArray) ? DataFormatJSCell : DataFormatJS);
        }
        break;
    }

    case SetLocal: {
        switch (m_jit.graph().getPrediction(node.local())) {
        case PredictInt32: {
            SpeculateIntegerOperand value(this, node.child1);
            m_jit.store32(value.gpr(), JITCompiler::payloadFor(node.local()));
            noResult(m_compileIndex);
            break;
        }
        case PredictArray: {
            SpeculateCellOperand cell(this, node.child1);
            m_jit.storePtr(cell.gpr(), JITCompiler::addressFor(node.local()));
            noResult(m_compileIndex);
            break;
        }

        default: {
            JSValueOperand value(this, node.child1);
            m_jit.storePtr(value.gpr(), JITCompiler::addressFor(node.local()));
            noResult(m_compileIndex);
            break;
        }
        }
        break;
    }

    case BitAnd:
    case BitOr:
    case BitXor:
        if (isInt32Constant(node.child1)) {
            SpeculateIntegerOperand op2(this, node.child2);
            GPRTemporary result(this, op2);

            bitOp(op, valueOfInt32Constant(node.child1), op2.gpr(), result.gpr());

            integerResult(result.gpr(), m_compileIndex);
        } else if (isInt32Constant(node.child2)) {
            SpeculateIntegerOperand op1(this, node.child1);
            GPRTemporary result(this, op1);

            bitOp(op, valueOfInt32Constant(node.child2), op1.gpr(), result.gpr());

            integerResult(result.gpr(), m_compileIndex);
        } else {
            SpeculateIntegerOperand op1(this, node.child1);
            SpeculateIntegerOperand op2(this, node.child2);
            GPRTemporary result(this, op1, op2);

            GPRReg reg1 = op1.gpr();
            GPRReg reg2 = op2.gpr();
            bitOp(op, reg1, reg2, result.gpr());

            integerResult(result.gpr(), m_compileIndex);
        }
        break;

    case BitRShift:
    case BitLShift:
    case BitURShift:
        if (isInt32Constant(node.child2)) {
            SpeculateIntegerOperand op1(this, node.child1);
            GPRTemporary result(this, op1);

            shiftOp(op, op1.gpr(), valueOfInt32Constant(node.child2) & 0x1f, result.gpr());

            integerResult(result.gpr(), m_compileIndex);
        } else {
            // Do not allow shift amount to be used as the result, MacroAssembler does not permit this.
            SpeculateIntegerOperand op1(this, node.child1);
            SpeculateIntegerOperand op2(this, node.child2);
            GPRTemporary result(this, op1);

            GPRReg reg1 = op1.gpr();
            GPRReg reg2 = op2.gpr();
            shiftOp(op, reg1, reg2, result.gpr());

            integerResult(result.gpr(), m_compileIndex);
        }
        break;

    case UInt32ToNumber: {
        IntegerOperand op1(this, node.child1);
        GPRTemporary result(this, op1);

        // Test the operand is positive.
        speculationCheck(m_jit.branch32(MacroAssembler::LessThan, op1.gpr(), TrustedImm32(0)));

        m_jit.move(op1.gpr(), result.gpr());
        integerResult(result.gpr(), m_compileIndex, op1.format());
        break;
    }

    case NumberToInt32: {
        SpeculateIntegerOperand op1(this, node.child1);
        GPRTemporary result(this, op1);
        m_jit.move(op1.gpr(), result.gpr());
        integerResult(result.gpr(), m_compileIndex, op1.format());
        break;
    }

    case Int32ToNumber: {
        SpeculateIntegerOperand op1(this, node.child1);
        GPRTemporary result(this, op1);
        m_jit.move(op1.gpr(), result.gpr());
        integerResult(result.gpr(), m_compileIndex, op1.format());
        break;
    }

    case ValueToInt32: {
        SpeculateIntegerOperand op1(this, node.child1);
        GPRTemporary result(this, op1);
        m_jit.move(op1.gpr(), result.gpr());
        integerResult(result.gpr(), m_compileIndex, op1.format());
        break;
    }

    case ValueToNumber: {
        SpeculateIntegerOperand op1(this, node.child1);
        GPRTemporary result(this, op1);
        m_jit.move(op1.gpr(), result.gpr());
        integerResult(result.gpr(), m_compileIndex, op1.format());
        break;
    }

    case ValueAdd:
    case ArithAdd: {
        int32_t imm1;
        if (isDoubleConstantWithInt32Value(node.child1, imm1)) {
            SpeculateIntegerOperand op2(this, node.child2);
            GPRTemporary result(this);

            speculationCheck(m_jit.branchAdd32(MacroAssembler::Overflow, op2.gpr(), Imm32(imm1), result.gpr()));

            integerResult(result.gpr(), m_compileIndex);
            break;
        }
            
        int32_t imm2;
        if (isDoubleConstantWithInt32Value(node.child2, imm2)) {
            SpeculateIntegerOperand op1(this, node.child1);
            GPRTemporary result(this);

            speculationCheck(m_jit.branchAdd32(MacroAssembler::Overflow, op1.gpr(), Imm32(imm2), result.gpr()));

            integerResult(result.gpr(), m_compileIndex);
            break;
        }
            
        SpeculateIntegerOperand op1(this, node.child1);
        SpeculateIntegerOperand op2(this, node.child2);
        GPRTemporary result(this, op1, op2);

        GPRReg gpr1 = op1.gpr();
        GPRReg gpr2 = op2.gpr();
        GPRReg gprResult = result.gpr();
        MacroAssembler::Jump check = m_jit.branchAdd32(MacroAssembler::Overflow, gpr1, gpr2, gprResult);

        if (gpr1 == gprResult)
            speculationCheck(check, SpeculationRecovery(SpeculativeAdd, gprResult, gpr2));
        else if (gpr2 == gprResult)
            speculationCheck(check, SpeculationRecovery(SpeculativeAdd, gprResult, gpr1));
        else
            speculationCheck(check);

        integerResult(gprResult, m_compileIndex);
        break;
    }

    case ArithSub: {
        int32_t imm2;
        if (isDoubleConstantWithInt32Value(node.child2, imm2)) {
            SpeculateIntegerOperand op1(this, node.child1);
            GPRTemporary result(this);

            speculationCheck(m_jit.branchSub32(MacroAssembler::Overflow, op1.gpr(), Imm32(imm2), result.gpr()));

            integerResult(result.gpr(), m_compileIndex);
            break;
        }
            
        SpeculateIntegerOperand op1(this, node.child1);
        SpeculateIntegerOperand op2(this, node.child2);
        GPRTemporary result(this);

        speculationCheck(m_jit.branchSub32(MacroAssembler::Overflow, op1.gpr(), op2.gpr(), result.gpr()));

        integerResult(result.gpr(), m_compileIndex);
        break;
    }

    case ArithMul: {
        SpeculateIntegerOperand op1(this, node.child1);
        SpeculateIntegerOperand op2(this, node.child2);
        GPRTemporary result(this);

        GPRReg reg1 = op1.gpr();
        GPRReg reg2 = op2.gpr();
        speculationCheck(m_jit.branchMul32(MacroAssembler::Overflow, reg1, reg2, result.gpr()));

        MacroAssembler::Jump resultNonZero = m_jit.branchTest32(MacroAssembler::NonZero, result.gpr());
        speculationCheck(m_jit.branch32(MacroAssembler::LessThan, reg1, TrustedImm32(0)));
        speculationCheck(m_jit.branch32(MacroAssembler::LessThan, reg2, TrustedImm32(0)));
        resultNonZero.link(&m_jit);

        integerResult(result.gpr(), m_compileIndex);
        break;
    }

    case ArithDiv: {
        SpeculateIntegerOperand op1(this, node.child1);
        SpeculateIntegerOperand op2(this, node.child2);
        GPRTemporary result(this, op1, op2);

        op1.gpr();
        op2.gpr();
        terminateSpeculativeExecution();

        integerResult(result.gpr(), m_compileIndex);
        break;
    }

    case ArithMod: {
        SpeculateIntegerOperand op1(this, node.child1);
        SpeculateIntegerOperand op2(this, node.child2);
        GPRTemporary result(this, op1, op2);

        op1.gpr();
        op2.gpr();
        terminateSpeculativeExecution();

        integerResult(result.gpr(), m_compileIndex);
        break;
    }

    case LogicalNot: {
        JSValueOperand value(this, node.child1);
        GPRTemporary result(this); // FIXME: We could reuse, but on speculation fail would need recovery to restore tag (akin to add).

        m_jit.move(value.gpr(), result.gpr());
        m_jit.xorPtr(TrustedImm32(static_cast<int32_t>(ValueFalse)), result.gpr());
        speculationCheck(m_jit.branchTestPtr(JITCompiler::NonZero, result.gpr(), TrustedImm32(static_cast<int32_t>(~1))));
        m_jit.xorPtr(TrustedImm32(static_cast<int32_t>(ValueTrue)), result.gpr());

        // If we add a DataFormatBool, we should use it here.
        jsValueResult(result.gpr(), m_compileIndex);
        break;
    }

    case CompareLess: {
        // Fused compare & branch.
        if (detectPeepHoleBranch()) {
            // detectPeepHoleBranch currently only permits the branch to be the very next node,
            // so can be no intervening nodes to also reference the compare. 
            ASSERT(node.adjustedRefCount() == 1);

            compilePeepHoleBranch(node, JITCompiler::LessThan);

            use(node.child1);
            use(node.child2);
            ++m_compileIndex;
            return;
        }

        // Normal case, not fused to branch.
        SpeculateIntegerOperand op1(this, node.child1);
        SpeculateIntegerOperand op2(this, node.child2);
        GPRTemporary result(this, op1, op2);

        m_jit.compare32(JITCompiler::LessThan, op1.gpr(), op2.gpr(), result.gpr());

        // If we add a DataFormatBool, we should use it here.
        m_jit.or32(TrustedImm32(ValueFalse), result.gpr());
        jsValueResult(result.gpr(), m_compileIndex);
        break;
    }

    case CompareLessEq: {
        // Fused compare & branch.
        if (detectPeepHoleBranch()) {
            // detectPeepHoleBranch currently only permits the branch to be the very next node,
            // so can be no intervening nodes to also reference the compare. 
            ASSERT(node.adjustedRefCount() == 1);

            compilePeepHoleBranch(node, JITCompiler::LessThanOrEqual);

            use(node.child1);
            use(node.child2);
            ++m_compileIndex;
            return;
        }

        // Normal case, not fused to branch.
        SpeculateIntegerOperand op1(this, node.child1);
        SpeculateIntegerOperand op2(this, node.child2);
        GPRTemporary result(this, op1, op2);

        m_jit.compare32(JITCompiler::LessThanOrEqual, op1.gpr(), op2.gpr(), result.gpr());

        // If we add a DataFormatBool, we should use it here.
        m_jit.or32(TrustedImm32(ValueFalse), result.gpr());
        jsValueResult(result.gpr(), m_compileIndex);
        break;
    }

    case CompareEq: {
        SpeculateIntegerOperand op1(this, node.child1);
        SpeculateIntegerOperand op2(this, node.child2);
        GPRTemporary result(this, op1, op2);

        m_jit.compare32(JITCompiler::Equal, op1.gpr(), op2.gpr(), result.gpr());

        // If we add a DataFormatBool, we should use it here.
        m_jit.or32(TrustedImm32(ValueFalse), result.gpr());
        jsValueResult(result.gpr(), m_compileIndex);
        break;
    }

    case CompareStrictEq: {
        SpeculateIntegerOperand op1(this, node.child1);
        SpeculateIntegerOperand op2(this, node.child2);
        GPRTemporary result(this, op1, op2);

        m_jit.compare32(JITCompiler::Equal, op1.gpr(), op2.gpr(), result.gpr());

        // If we add a DataFormatBool, we should use it here.
        m_jit.or32(TrustedImm32(ValueFalse), result.gpr());
        jsValueResult(result.gpr(), m_compileIndex);
        break;
    }

    case GetByVal: {
        NodeIndex alias = node.child3;
        if (alias != NoNode) {
            // FIXME: result should be able to reuse child1, child2. Should have an 'UnusedOperand' type.
            JSValueOperand aliasedValue(this, node.child3);
            GPRTemporary result(this, aliasedValue);
            m_jit.move(aliasedValue.gpr(), result.gpr());
            jsValueResult(result.gpr(), m_compileIndex);
            break;
        }

        SpeculateCellOperand base(this, node.child1);
        SpeculateStrictInt32Operand property(this, node.child2);
        GPRTemporary storage(this);

        GPRReg baseReg = base.gpr();
        GPRReg propertyReg = property.gpr();
        GPRReg storageReg = storage.gpr();

        // Get the array storage. We haven't yet checked this is a JSArray, so this is only safe if
        // an access with offset JSArray::storageOffset() is valid for all JSCells!
        m_jit.loadPtr(MacroAssembler::Address(baseReg, JSArray::storageOffset()), storageReg);

        // Check that base is an array, and that property is contained within m_vector (< m_vectorLength).
        // If we have predicted the base to be type array, we can skip the check.
        Node& baseNode = m_jit.graph()[node.child1];
        if (baseNode.op != GetLocal || m_jit.graph().getPrediction(baseNode.local()) != PredictArray)
            speculationCheck(m_jit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(baseReg), MacroAssembler::TrustedImmPtr(m_jit.globalData()->jsArrayVPtr)));
        speculationCheck(m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(baseReg, JSArray::vectorLengthOffset())));

        // FIXME: In cases where there are subsequent by_val accesses to the same base it might help to cache
        // the storage pointer - especially if there happens to be another register free right now. If we do so,
        // then we'll need to allocate a new temporary for result.
        GPRTemporary& result = storage;
        m_jit.loadPtr(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::ScalePtr, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])), result.gpr());
        speculationCheck(m_jit.branchTestPtr(MacroAssembler::Zero, result.gpr()));

        jsValueResult(result.gpr(), m_compileIndex);
        break;
    }

    case PutByVal: {
        SpeculateCellOperand base(this, node.child1);
        SpeculateStrictInt32Operand property(this, node.child2);
        JSValueOperand value(this, node.child3);
        GPRTemporary storage(this);

        // Map base, property & value into registers, allocate a register for storage.
        GPRReg baseReg = base.gpr();
        GPRReg propertyReg = property.gpr();
        GPRReg valueReg = value.gpr();
        GPRReg storageReg = storage.gpr();

        // Check that base is an array, and that property is contained within m_vector (< m_vectorLength).
        // If we have predicted the base to be type array, we can skip the check.
        Node& baseNode = m_jit.graph()[node.child1];
        if (baseNode.op != GetLocal || m_jit.graph().getPrediction(baseNode.local()) != PredictArray)
            speculationCheck(m_jit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(baseReg), MacroAssembler::TrustedImmPtr(m_jit.globalData()->jsArrayVPtr)));
        speculationCheck(m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(baseReg, JSArray::vectorLengthOffset())));

        // Get the array storage.
        m_jit.loadPtr(MacroAssembler::Address(baseReg, JSArray::storageOffset()), storageReg);

        // Check if we're writing to a hole; if so increment m_numValuesInVector.
        MacroAssembler::Jump notHoleValue = m_jit.branchTestPtr(MacroAssembler::NonZero, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::ScalePtr, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])));
        m_jit.add32(TrustedImm32(1), MacroAssembler::Address(storageReg, OBJECT_OFFSETOF(ArrayStorage, m_numValuesInVector)));

        // If we're writing to a hole we might be growing the array; 
        MacroAssembler::Jump lengthDoesNotNeedUpdate = m_jit.branch32(MacroAssembler::Below, propertyReg, MacroAssembler::Address(storageReg, OBJECT_OFFSETOF(ArrayStorage, m_length)));
        m_jit.add32(TrustedImm32(1), propertyReg);
        m_jit.store32(propertyReg, MacroAssembler::Address(storageReg, OBJECT_OFFSETOF(ArrayStorage, m_length)));
        m_jit.sub32(TrustedImm32(1), propertyReg);

        lengthDoesNotNeedUpdate.link(&m_jit);
        notHoleValue.link(&m_jit);

        // Store the value to the array.
        m_jit.storePtr(valueReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::ScalePtr, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])));

        noResult(m_compileIndex);
        break;
    }

    case PutByValAlias: {
        SpeculateCellOperand base(this, node.child1);
        SpeculateStrictInt32Operand property(this, node.child2);
        JSValueOperand value(this, node.child3);
        GPRTemporary storage(this, base); // storage may overwrite base.

        // Get the array storage.
        GPRReg storageReg = storage.gpr();
        m_jit.loadPtr(MacroAssembler::Address(base.gpr(), JSArray::storageOffset()), storageReg);

        // Map property & value into registers.
        GPRReg propertyReg = property.gpr();
        GPRReg valueReg = value.gpr();

        // Store the value to the array.
        m_jit.storePtr(valueReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::ScalePtr, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])));

        noResult(m_compileIndex);
        break;
    }

    case DFG::Jump: {
        BlockIndex taken = m_jit.graph().blockIndexForBytecodeOffset(node.takenBytecodeOffset());
        if (taken != (m_block + 1))
            addBranch(m_jit.jump(), taken);
        noResult(m_compileIndex);
        break;
    }

    case Branch: {
        JSValueOperand value(this, node.child1);
        GPRReg valueReg = value.gpr();

        BlockIndex taken = m_jit.graph().blockIndexForBytecodeOffset(node.takenBytecodeOffset());
        BlockIndex notTaken = m_jit.graph().blockIndexForBytecodeOffset(node.notTakenBytecodeOffset());

        // Integers
        addBranch(m_jit.branchPtr(MacroAssembler::Equal, valueReg, MacroAssembler::ImmPtr(JSValue::encode(jsNumber(0)))), notTaken);
        MacroAssembler::Jump isNonZeroInteger = m_jit.branchPtr(MacroAssembler::AboveOrEqual, valueReg, GPRInfo::tagTypeNumberRegister);

        // Booleans
        addBranch(m_jit.branchPtr(MacroAssembler::Equal, valueReg, MacroAssembler::ImmPtr(JSValue::encode(jsBoolean(false)))), notTaken);
        speculationCheck(m_jit.branchPtr(MacroAssembler::NotEqual, valueReg, MacroAssembler::ImmPtr(JSValue::encode(jsBoolean(true)))));

        if (taken == (m_block + 1))
            isNonZeroInteger.link(&m_jit);
        else {
            addBranch(isNonZeroInteger, taken);
            addBranch(m_jit.jump(), taken);
        }

        noResult(m_compileIndex);
        break;
    }

    case Return: {
        ASSERT(GPRInfo::callFrameRegister != GPRInfo::regT1);
        ASSERT(GPRInfo::regT1 != GPRInfo::returnValueGPR);
        ASSERT(GPRInfo::returnValueGPR != GPRInfo::callFrameRegister);

#if DFG_SUCCESS_STATS
        static SamplingCounter counter("SpeculativeJIT");
        m_jit.emitCount(counter);
#endif

        // Return the result in returnValueGPR.
        JSValueOperand op1(this, node.child1);
        m_jit.move(op1.gpr(), GPRInfo::returnValueGPR);

        // Grab the return address.
        m_jit.emitGetFromCallFrameHeaderPtr(RegisterFile::ReturnPC, GPRInfo::regT1);
        // Restore our caller's "r".
        m_jit.emitGetFromCallFrameHeaderPtr(RegisterFile::CallerFrame, GPRInfo::callFrameRegister);
        // Return.
        m_jit.restoreReturnAddressBeforeReturn(GPRInfo::regT1);
        m_jit.ret();
        
        noResult(m_compileIndex);
        break;
    }

    case ConvertThis: {
        SpeculateCellOperand thisValue(this, node.child1);
        GPRTemporary temp(this);

        m_jit.loadPtr(JITCompiler::Address(thisValue.gpr(), JSCell::structureOffset()), temp.gpr());
        speculationCheck(m_jit.branchTest8(JITCompiler::NonZero, JITCompiler::Address(temp.gpr(), Structure::typeInfoFlagsOffset()), JITCompiler::TrustedImm32(NeedsThisConversion)));

        cellResult(thisValue.gpr(), m_compileIndex);
        break;
    }

    case GetById: {
        JSValueOperand base(this, node.child1);
        GPRReg baseGPR = base.gpr();
        flushRegisters();

        GPRResult result(this);
        callOperation(operationGetById, result.gpr(), baseGPR, identifier(node.identifierNumber()));
        jsValueResult(result.gpr(), m_compileIndex);
        break;
    }

    case PutById: {
        JSValueOperand base(this, node.child1);
        JSValueOperand value(this, node.child2);
        GPRReg valueGPR = value.gpr();
        GPRReg baseGPR = base.gpr();
        flushRegisters();

        callOperation(m_jit.codeBlock()->isStrictMode() ? operationPutByIdStrict : operationPutByIdNonStrict, valueGPR, baseGPR, identifier(node.identifierNumber()));
        noResult(m_compileIndex);
        break;
    }

    case PutByIdDirect: {
        JSValueOperand base(this, node.child1);
        JSValueOperand value(this, node.child2);
        GPRReg valueGPR = value.gpr();
        GPRReg baseGPR = base.gpr();
        flushRegisters();

        callOperation(m_jit.codeBlock()->isStrictMode() ? operationPutByIdDirectStrict : operationPutByIdDirectNonStrict, valueGPR, baseGPR, identifier(node.identifierNumber()));
        noResult(m_compileIndex);
        break;
    }

    case GetGlobalVar: {
        GPRTemporary result(this);

        JSVariableObject* globalObject = m_jit.codeBlock()->globalObject();
        m_jit.loadPtr(globalObject->addressOfRegisters(), result.gpr());
        m_jit.loadPtr(JITCompiler::addressForGlobalVar(result.gpr(), node.varNumber()), result.gpr());

        jsValueResult(result.gpr(), m_compileIndex);
        break;
    }

    case PutGlobalVar: {
        JSValueOperand value(this, node.child1);
        GPRTemporary temp(this);

        JSVariableObject* globalObject = m_jit.codeBlock()->globalObject();
        m_jit.loadPtr(globalObject->addressOfRegisters(), temp.gpr());
        m_jit.storePtr(value.gpr(), JITCompiler::addressForGlobalVar(temp.gpr(), node.varNumber()));

        noResult(m_compileIndex);
        break;
    }

    case Phi:
        ASSERT_NOT_REACHED();
    }

    if (node.hasResult() && node.mustGenerate())
        use(m_compileIndex);
}

void SpeculativeJIT::compile(BasicBlock& block)
{
    ASSERT(m_compileIndex == block.begin);
    m_blockHeads[m_block] = m_jit.label();
#if DFG_JIT_BREAK_ON_EVERY_BLOCK
    m_jit.breakpoint();
#endif

    for (; m_compileIndex < block.end; ++m_compileIndex) {
        Node& node = m_jit.graph()[m_compileIndex];
        if (!node.shouldGenerate())
            continue;

#if DFG_DEBUG_VERBOSE
        fprintf(stderr, "SpeculativeJIT generating Node @%d at JIT offset 0x%x\n", (int)m_compileIndex, m_jit.debugOffset());
#endif
#if DFG_JIT_BREAK_ON_EVERY_NODE
        m_jit.breakpoint();
#endif
        checkConsistency();
        compile(node);
        if (!m_compileOkay)
            return;
        checkConsistency();
    }
}

// If we are making type predictions about our arguments then
// we need to check that they are correct on function entry.
void SpeculativeJIT::checkArgumentTypes()
{
    ASSERT(!m_compileIndex);
    for (int i = 0; i < m_jit.codeBlock()->m_numParameters; ++i) {
        VirtualRegister virtualRegister = (VirtualRegister)(m_jit.codeBlock()->thisRegister() + i);
        switch (m_jit.graph().getPrediction(virtualRegister)) {
        case PredictInt32:
            speculationCheck(m_jit.branchPtr(MacroAssembler::Below, JITCompiler::addressFor(virtualRegister), GPRInfo::tagTypeNumberRegister));
            break;

        case PredictArray: {
            GPRTemporary temp(this);
            m_jit.loadPtr(JITCompiler::addressFor(virtualRegister), temp.gpr());
            speculationCheck(m_jit.branchTestPtr(MacroAssembler::NonZero, temp.gpr(), GPRInfo::tagMaskRegister));
            speculationCheck(m_jit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(temp.gpr()), MacroAssembler::TrustedImmPtr(m_jit.globalData()->jsArrayVPtr)));
            break;
        }

        default:
            break;
        }
    }
}

// For any vars that we will be treating as numeric, write 0 to
// the var on entry. Throughout the block we will only read/write
// to the payload, by writing the tag now we prevent the GC from
// misinterpreting values as pointers.
void SpeculativeJIT::initializeVariableTypes()
{
    ASSERT(!m_compileIndex);
    for (int var = 0; var < m_jit.codeBlock()->m_numVars; ++var) {
        if (m_jit.graph().getPrediction(var) == PredictInt32)
            m_jit.storePtr(GPRInfo::tagTypeNumberRegister, JITCompiler::addressFor((VirtualRegister)var));
    }
}

bool SpeculativeJIT::compile()
{
    checkArgumentTypes();
    initializeVariableTypes();

    ASSERT(!m_compileIndex);
    for (m_block = 0; m_block < m_jit.graph().m_blocks.size(); ++m_block) {
        compile(*m_jit.graph().m_blocks[m_block]);
        if (!m_compileOkay)
            return false;
    }
    linkBranches();
    return true;
}

} } // namespace JSC::DFG

#endif
