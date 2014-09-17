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
#include "DFGNonSpeculativeJIT.h"

#include "DFGSpeculativeJIT.h"

#if ENABLE(DFG_JIT)

namespace JSC { namespace DFG {

const double twoToThe32 = (double)0x100000000ull;

EntryLocation::EntryLocation(MacroAssembler::Label entry, NonSpeculativeJIT* jit)
    : m_entry(entry)
    , m_nodeIndex(jit->m_compileIndex)
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

void NonSpeculativeJIT::valueToNumber(JSValueOperand& operand, FPRReg fpr)
{
    GPRReg jsValueGpr = operand.gpr();
    GPRReg tempGpr = allocate(); // FIXME: can we skip this allocation on the last use of the virtual register?

    JITCompiler::Jump isInteger = m_jit.branchPtr(MacroAssembler::AboveOrEqual, jsValueGpr, GPRInfo::tagTypeNumberRegister);
    JITCompiler::Jump nonNumeric = m_jit.branchTestPtr(MacroAssembler::Zero, jsValueGpr, GPRInfo::tagTypeNumberRegister);

    // First, if we get here we have a double encoded as a JSValue
    m_jit.move(jsValueGpr, tempGpr);
    m_jit.addPtr(GPRInfo::tagTypeNumberRegister, tempGpr);
    m_jit.movePtrToDouble(tempGpr, fpr);
    JITCompiler::Jump hasUnboxedDouble = m_jit.jump();

    // Next handle cells (& other JS immediates)
    nonNumeric.link(&m_jit);
    silentSpillAllRegisters(fpr, jsValueGpr);
    m_jit.move(jsValueGpr, GPRInfo::argumentGPR1);
    m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
    appendCallWithExceptionCheck(dfgConvertJSValueToNumber);
    m_jit.moveDouble(FPRInfo::returnValueFPR, fpr);
    silentFillAllRegisters(fpr);
    JITCompiler::Jump hasCalledToNumber = m_jit.jump();
    
    // Finally, handle integers.
    isInteger.link(&m_jit);
    m_jit.convertInt32ToDouble(jsValueGpr, fpr);
    hasUnboxedDouble.link(&m_jit);
    hasCalledToNumber.link(&m_jit);

    m_gprs.unlock(tempGpr);
}

void NonSpeculativeJIT::valueToInt32(JSValueOperand& operand, GPRReg result)
{
    GPRReg jsValueGpr = operand.gpr();

    JITCompiler::Jump isInteger = m_jit.branchPtr(MacroAssembler::AboveOrEqual, jsValueGpr, GPRInfo::tagTypeNumberRegister);

    // First handle non-integers
    silentSpillAllRegisters(result, jsValueGpr);
    m_jit.move(jsValueGpr, GPRInfo::argumentGPR1);
    m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
    appendCallWithExceptionCheck(dfgConvertJSValueToInt32);
    m_jit.zeroExtend32ToPtr(GPRInfo::returnValueGPR, result);
    silentFillAllRegisters(result);
    JITCompiler::Jump hasCalledToInt32 = m_jit.jump();
    
    // Then handle integers.
    isInteger.link(&m_jit);
    m_jit.zeroExtend32ToPtr(jsValueGpr, result);
    hasCalledToInt32.link(&m_jit);
}

void NonSpeculativeJIT::numberToInt32(FPRReg fpr, GPRReg gpr)
{
    JITCompiler::Jump truncatedToInteger = m_jit.branchTruncateDoubleToInt32(fpr, gpr, JITCompiler::BranchIfTruncateSuccessful);

    silentSpillAllRegisters(gpr);

    m_jit.moveDouble(fpr, FPRInfo::argumentFPR0);
    appendCallWithExceptionCheck(toInt32);
    m_jit.zeroExtend32ToPtr(GPRInfo::returnValueGPR, gpr);

    silentFillAllRegisters(gpr);

    truncatedToInteger.link(&m_jit);
}

bool NonSpeculativeJIT::isKnownInteger(NodeIndex nodeIndex)
{
    GenerationInfo& info = m_generationInfo[m_jit.graph()[nodeIndex].virtualRegister()];

    DataFormat registerFormat = info.registerFormat();
    if (registerFormat != DataFormatNone)
        return (registerFormat | DataFormatJS) == DataFormatJSInteger;

    DataFormat spillFormat = info.spillFormat();
    if (spillFormat != DataFormatNone)
        return (spillFormat | DataFormatJS) == DataFormatJSInteger;

    ASSERT(isConstant(nodeIndex));
    return isInt32Constant(nodeIndex);
}

bool NonSpeculativeJIT::isKnownNumeric(NodeIndex nodeIndex)
{
    GenerationInfo& info = m_generationInfo[m_jit.graph()[nodeIndex].virtualRegister()];

    DataFormat registerFormat = info.registerFormat();
    if (registerFormat != DataFormatNone)
        return (registerFormat | DataFormatJS) == DataFormatJSInteger
            || (registerFormat | DataFormatJS) == DataFormatJSDouble;

    DataFormat spillFormat = info.spillFormat();
    if (spillFormat != DataFormatNone)
        return (spillFormat | DataFormatJS) == DataFormatJSInteger
            || (spillFormat | DataFormatJS) == DataFormatJSDouble;

    ASSERT(isConstant(nodeIndex));
    return isInt32Constant(nodeIndex) || isDoubleConstant(nodeIndex);
}

void NonSpeculativeJIT::compile(SpeculationCheckIndexIterator& checkIterator, Node& node)
{
    // Check for speculation checks from the corresponding instruction in the
    // speculative path. Do not check for NodeIndex 0, since this is checked
    // in the outermost compile layer, at the head of the non-speculative path
    // (for index 0 we may need to check regardless of whether or not the node
    // will be generated, since argument type speculation checks will appear
    // as speculation checks at this index).
    if (m_compileIndex && checkIterator.hasCheckAtIndex(m_compileIndex))
        trackEntry(m_jit.label());

    NodeType op = node.op;

    switch (op) {
    case ConvertThis: {
        JSValueOperand thisValue(this, node.child1);
        GPRReg thisGPR = thisValue.gpr();
        flushRegisters();

        GPRResult result(this);
        callOperation(operationConvertThis, result.gpr(), thisGPR);
        cellResult(result.gpr(), m_compileIndex);
        break;
    }

    case Int32Constant:
    case DoubleConstant:
    case JSConstant:
        initConstantInfo(m_compileIndex);
        break;

    case GetLocal: {
        GPRTemporary result(this);
        m_jit.loadPtr(JITCompiler::addressFor(node.local()), result.gpr());

        // Like jsValueResult, but don't useChildren - our children are phi nodes,
        // and don't represent values within this dataflow with virtual registers.
        VirtualRegister virtualRegister = node.virtualRegister();
        m_gprs.retain(result.gpr(), virtualRegister, SpillOrderJS);
        m_generationInfo[virtualRegister].initJSValue(m_compileIndex, node.refCount(), result.gpr(), DataFormatJS);
        break;
    }

    case SetLocal: {
        JSValueOperand value(this, node.child1);
        m_jit.storePtr(value.gpr(), JITCompiler::addressFor(node.local()));
        noResult(m_compileIndex);
        break;
    }

    case BitAnd:
    case BitOr:
    case BitXor:
        if (isInt32Constant(node.child1)) {
            IntegerOperand op2(this, node.child2);
            GPRTemporary result(this, op2);

            bitOp(op, valueOfInt32Constant(node.child1), op2.gpr(), result.gpr());

            integerResult(result.gpr(), m_compileIndex);
        } else if (isInt32Constant(node.child2)) {
            IntegerOperand op1(this, node.child1);
            GPRTemporary result(this, op1);

            bitOp(op, valueOfInt32Constant(node.child2), op1.gpr(), result.gpr());

            integerResult(result.gpr(), m_compileIndex);
        } else {
            IntegerOperand op1(this, node.child1);
            IntegerOperand op2(this, node.child2);
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
            IntegerOperand op1(this, node.child1);
            GPRTemporary result(this, op1);

            int shiftAmount = valueOfInt32Constant(node.child2) & 0x1f;
            // Shifts by zero should have been optimized out of the graph!
            ASSERT(shiftAmount);
            shiftOp(op, op1.gpr(), shiftAmount, result.gpr());

            integerResult(result.gpr(), m_compileIndex);
        } else {
            // Do not allow shift amount to be used as the result, MacroAssembler does not permit this.
            IntegerOperand op1(this, node.child1);
            IntegerOperand op2(this, node.child2);
            GPRTemporary result(this, op1);

            GPRReg reg1 = op1.gpr();
            GPRReg reg2 = op2.gpr();
            shiftOp(op, reg1, reg2, result.gpr());

            integerResult(result.gpr(), m_compileIndex);
        }
        break;

    case UInt32ToNumber: {
        IntegerOperand op1(this, node.child1);
        FPRTemporary result(this);
        m_jit.convertInt32ToDouble(op1.gpr(), result.fpr());

        MacroAssembler::Jump positive = m_jit.branch32(MacroAssembler::GreaterThanOrEqual, op1.gpr(), TrustedImm32(0));
        m_jit.addDouble(JITCompiler::AbsoluteAddress(&twoToThe32), result.fpr());
        positive.link(&m_jit);

        doubleResult(result.fpr(), m_compileIndex);
        break;
    }

    case Int32ToNumber: {
        IntegerOperand op1(this, node.child1);
        FPRTemporary result(this);
        m_jit.convertInt32ToDouble(op1.gpr(), result.fpr());
        doubleResult(result.fpr(), m_compileIndex);
        break;
    }

    case NumberToInt32:
    case ValueToInt32: {
        ASSERT(!isInt32Constant(node.child1));

        if (isKnownInteger(node.child1)) {
            IntegerOperand op1(this, node.child1);
            GPRTemporary result(this, op1);
            m_jit.move(op1.gpr(), result.gpr());
            integerResult(result.gpr(), m_compileIndex);
            break;
        }

        if (isKnownNumeric(node.child1)) {
            DoubleOperand op1(this, node.child1);
            GPRTemporary result(this);
            numberToInt32(op1.fpr(), result.gpr());
            integerResult(result.gpr(), m_compileIndex);
            break;
        }

        // We should have handled this via isKnownInteger, or isKnownNumeric!
        ASSERT(op != NumberToInt32);

        JSValueOperand op1(this, node.child1);
        GPRTemporary result(this, op1);
        valueToInt32(op1, result.gpr());
        integerResult(result.gpr(), m_compileIndex);
        break;
    }

    case ValueToNumber: {
        ASSERT(!isInt32Constant(node.child1));
        ASSERT(!isDoubleConstant(node.child1));

        if (isKnownInteger(node.child1)) {
            IntegerOperand op1(this, node.child1);
            FPRTemporary result(this);
            m_jit.convertInt32ToDouble(op1.gpr(), result.fpr());
            doubleResult(result.fpr(), m_compileIndex);
            break;
        }

        if (isKnownNumeric(node.child1)) {
            DoubleOperand op1(this, node.child1);
            FPRTemporary result(this, op1);
            m_jit.moveDouble(op1.fpr(), result.fpr());
            doubleResult(result.fpr(), m_compileIndex);
            break;
        }

        JSValueOperand op1(this, node.child1);
        FPRTemporary result(this);
        valueToNumber(op1, result.fpr());
        doubleResult(result.fpr(), m_compileIndex);
        break;
    }

    case ValueAdd: {
        JSValueOperand arg1(this, node.child1);
        JSValueOperand arg2(this, node.child2);
        GPRReg arg1GPR = arg1.gpr();
        GPRReg arg2GPR = arg2.gpr();
        flushRegisters();

        GPRResult result(this);
        callOperation(operationValueAdd, result.gpr(), arg1GPR, arg2GPR);

        jsValueResult(result.gpr(), m_compileIndex);
        break;
    }
        
    case ArithAdd: {
        DoubleOperand op1(this, node.child1);
        DoubleOperand op2(this, node.child2);
        FPRTemporary result(this, op1, op2);

        FPRReg reg1 = op1.fpr();
        FPRReg reg2 = op2.fpr();
        m_jit.addDouble(reg1, reg2, result.fpr());

        doubleResult(result.fpr(), m_compileIndex);
        break;
    }

    case ArithSub: {
        DoubleOperand op1(this, node.child1);
        DoubleOperand op2(this, node.child2);
        FPRTemporary result(this, op1);

        FPRReg reg1 = op1.fpr();
        FPRReg reg2 = op2.fpr();
        m_jit.subDouble(reg1, reg2, result.fpr());

        doubleResult(result.fpr(), m_compileIndex);
        break;
    }

    case ArithMul: {
        DoubleOperand op1(this, node.child1);
        DoubleOperand op2(this, node.child2);
        FPRTemporary result(this, op1, op2);

        FPRReg reg1 = op1.fpr();
        FPRReg reg2 = op2.fpr();
        m_jit.mulDouble(reg1, reg2, result.fpr());

        doubleResult(result.fpr(), m_compileIndex);
        break;
    }

    case ArithDiv: {
        DoubleOperand op1(this, node.child1);
        DoubleOperand op2(this, node.child2);
        FPRTemporary result(this, op1);

        FPRReg reg1 = op1.fpr();
        FPRReg reg2 = op2.fpr();
        m_jit.divDouble(reg1, reg2, result.fpr());

        doubleResult(result.fpr(), m_compileIndex);
        break;
    }

    case ArithMod: {
        DoubleOperand arg1(this, node.child1);
        DoubleOperand arg2(this, node.child2);
        FPRReg arg1FPR = arg1.fpr();
        FPRReg arg2FPR = arg2.fpr();
        flushRegisters();

        FPRResult result(this);
        callOperation(fmod, result.fpr(), arg1FPR, arg2FPR);

        doubleResult(result.fpr(), m_compileIndex);
        break;
    }

    case LogicalNot: {
        JSValueOperand arg1(this, node.child1);
        GPRReg arg1GPR = arg1.gpr();
        flushRegisters();

        GPRResult result(this);
        callOperation(dfgConvertJSValueToBoolean, result.gpr(), arg1GPR);

        // If we add a DataFormatBool, we should use it here.
        m_jit.xor32(TrustedImm32(ValueTrue), result.gpr());
        jsValueResult(result.gpr(), m_compileIndex);
        break;
    }

    case CompareLess: {
        JSValueOperand arg1(this, node.child1);
        JSValueOperand arg2(this, node.child2);
        GPRReg arg1GPR = arg1.gpr();
        GPRReg arg2GPR = arg2.gpr();
        flushRegisters();

        GPRResult result(this);
        callOperation(operationCompareLess, result.gpr(), arg1GPR, arg2GPR);
        m_jit.or32(TrustedImm32(ValueFalse), result.gpr());

        jsValueResult(result.gpr(), m_compileIndex);
        break;
    }

    case CompareLessEq: {
        JSValueOperand arg1(this, node.child1);
        JSValueOperand arg2(this, node.child2);
        GPRReg arg1GPR = arg1.gpr();
        GPRReg arg2GPR = arg2.gpr();
        flushRegisters();

        GPRResult result(this);
        callOperation(operationCompareLessEq, result.gpr(), arg1GPR, arg2GPR);
        m_jit.or32(TrustedImm32(ValueFalse), result.gpr());

        jsValueResult(result.gpr(), m_compileIndex);
        break;
    }

    case CompareEq: {
        JSValueOperand arg1(this, node.child1);
        JSValueOperand arg2(this, node.child2);
        GPRReg arg1GPR = arg1.gpr();
        GPRReg arg2GPR = arg2.gpr();
        flushRegisters();

        GPRResult result(this);
        callOperation(operationCompareEq, result.gpr(), arg1GPR, arg2GPR);
        m_jit.or32(TrustedImm32(ValueFalse), result.gpr());

        jsValueResult(result.gpr(), m_compileIndex);
        break;
    }

    case CompareStrictEq: {
        JSValueOperand arg1(this, node.child1);
        JSValueOperand arg2(this, node.child2);
        GPRReg arg1GPR = arg1.gpr();
        GPRReg arg2GPR = arg2.gpr();
        flushRegisters();

        GPRResult result(this);
        callOperation(operationCompareStrictEq, result.gpr(), arg1GPR, arg2GPR);
        m_jit.or32(TrustedImm32(ValueFalse), result.gpr());

        jsValueResult(result.gpr(), m_compileIndex);
        break;
    }

    case GetByVal: {
        JSValueOperand arg1(this, node.child1);
        JSValueOperand arg2(this, node.child2);
        GPRReg arg1GPR = arg1.gpr();
        GPRReg arg2GPR = arg2.gpr();
        flushRegisters();

        GPRResult result(this);
        callOperation(operationGetByVal, result.gpr(), arg1GPR, arg2GPR);

        jsValueResult(result.gpr(), m_compileIndex);
        break;
    }

    case PutByVal:
    case PutByValAlias: {
        JSValueOperand arg1(this, node.child1);
        JSValueOperand arg2(this, node.child2);
        JSValueOperand arg3(this, node.child3);
        GPRReg arg1GPR = arg1.gpr();
        GPRReg arg2GPR = arg2.gpr();
        GPRReg arg3GPR = arg3.gpr();
        flushRegisters();

        GPRResult result(this);
        callOperation(m_jit.codeBlock()->isStrictMode() ? operationPutByValStrict : operationPutByValNonStrict, arg1GPR, arg2GPR, arg3GPR);

        noResult(m_compileIndex);
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

    case DFG::Jump: {
        BlockIndex taken = m_jit.graph().blockIndexForBytecodeOffset(node.takenBytecodeOffset());
        if (taken != (m_block + 1))
            addBranch(m_jit.jump(), taken);
        noResult(m_compileIndex);
        break;
    }

    case Branch: {
        JSValueOperand value(this, node.child1);
        GPRReg valueGPR = value.gpr();
        flushRegisters();

        GPRResult result(this);
        callOperation(dfgConvertJSValueToBoolean, result.gpr(), valueGPR);

        BlockIndex taken = m_jit.graph().blockIndexForBytecodeOffset(node.takenBytecodeOffset());
        BlockIndex notTaken = m_jit.graph().blockIndexForBytecodeOffset(node.notTakenBytecodeOffset());

        addBranch(m_jit.branchTest8(MacroAssembler::NonZero, result.gpr()), taken);
        if (notTaken != (m_block + 1))
            addBranch(m_jit.jump(), notTaken);

        noResult(m_compileIndex);
        break;
    }

    case Return: {
        ASSERT(GPRInfo::callFrameRegister != GPRInfo::regT1);
        ASSERT(GPRInfo::regT1 != GPRInfo::returnValueGPR);
        ASSERT(GPRInfo::returnValueGPR != GPRInfo::callFrameRegister);

#if DFG_SUCCESS_STATS
        static SamplingCounter counter("NonSpeculativeJIT");
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

    case Phi:
        ASSERT_NOT_REACHED();
    }

    if (node.hasResult() && node.mustGenerate())
        use(m_compileIndex);
}

void NonSpeculativeJIT::compile(SpeculationCheckIndexIterator& checkIterator, BasicBlock& block)
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
        fprintf(stderr, "NonSpeculativeJIT generating Node @%d at code offset 0x%x\n", (int)m_compileIndex, m_jit.debugOffset());
#endif
#if DFG_JIT_BREAK_ON_EVERY_NODE
        m_jit.breakpoint();
#endif

        checkConsistency();
        compile(checkIterator, node);
        checkConsistency();
    }
}

void NonSpeculativeJIT::compile(SpeculationCheckIndexIterator& checkIterator)
{
    // Check for speculation checks added at function entry (checking argument types).
    if (checkIterator.hasCheckAtIndex(m_compileIndex))
        trackEntry(m_jit.label());

    ASSERT(!m_compileIndex);
    for (m_block = 0; m_block < m_jit.graph().m_blocks.size(); ++m_block)
        compile(checkIterator, *m_jit.graph().m_blocks[m_block]);
    linkBranches();
}

} } // namespace JSC::DFG

#endif
