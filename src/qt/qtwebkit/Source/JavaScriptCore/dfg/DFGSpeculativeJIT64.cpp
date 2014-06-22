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
#include "DFGSpeculativeJIT.h"

#if ENABLE(DFG_JIT)

#include "Arguments.h"
#include "ArrayPrototype.h"
#include "DFGCallArrayAllocatorSlowPathGenerator.h"
#include "DFGSlowPathGenerator.h"
#include "JSCJSValueInlines.h"
#include "ObjectPrototype.h"

namespace JSC { namespace DFG {

#if USE(JSVALUE64)

GPRReg SpeculativeJIT::fillInteger(Edge edge, DataFormat& returnFormat)
{
    ASSERT(!needsTypeCheck(edge, SpecInt32));
    
    VirtualRegister virtualRegister = edge->virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    if (info.registerFormat() == DataFormatNone) {
        GPRReg gpr = allocate();

        if (edge->hasConstant()) {
            m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
            if (isInt32Constant(edge.node())) {
                m_jit.move(MacroAssembler::Imm32(valueOfInt32Constant(edge.node())), gpr);
                info.fillInteger(*m_stream, gpr);
                returnFormat = DataFormatInteger;
                return gpr;
            }
            if (isNumberConstant(edge.node())) {
                JSValue jsValue = jsNumber(valueOfNumberConstant(edge.node()));
                m_jit.move(MacroAssembler::Imm64(JSValue::encode(jsValue)), gpr);
            } else {
                ASSERT(isJSConstant(edge.node()));
                JSValue jsValue = valueOfJSConstant(edge.node());
                m_jit.move(MacroAssembler::TrustedImm64(JSValue::encode(jsValue)), gpr);
            }
        } else if (info.spillFormat() == DataFormatInteger) {
            m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
            m_jit.load32(JITCompiler::payloadFor(virtualRegister), gpr);
            // Tag it, since fillInteger() is used when we want a boxed integer.
            m_jit.or64(GPRInfo::tagTypeNumberRegister, gpr);
        } else {
            RELEASE_ASSERT(info.spillFormat() == DataFormatJS || info.spillFormat() == DataFormatJSInteger);
            m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
            m_jit.load64(JITCompiler::addressFor(virtualRegister), gpr);
        }

        // Since we statically know that we're filling an integer, and values
        // in the JSStack are boxed, this must be DataFormatJSInteger.
        // We will check this with a jitAssert below.
        info.fillJSValue(*m_stream, gpr, DataFormatJSInteger);
        unlock(gpr);
    }
    
    switch (info.registerFormat()) {
    case DataFormatNone:
        // Should have filled, above.
    case DataFormatJSDouble:
    case DataFormatDouble:
    case DataFormatJS:
    case DataFormatCell:
    case DataFormatJSCell:
    case DataFormatBoolean:
    case DataFormatJSBoolean:
    case DataFormatStorage:
        // Should only be calling this function if we know this operand to be integer.
        RELEASE_ASSERT_NOT_REACHED();

    case DataFormatJSInteger: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        m_jit.jitAssertIsJSInt32(gpr);
        returnFormat = DataFormatJSInteger;
        return gpr;
    }

    case DataFormatInteger: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        m_jit.jitAssertIsInt32(gpr);
        returnFormat = DataFormatInteger;
        return gpr;
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return InvalidGPRReg;
    }
}

GPRReg SpeculativeJIT::fillJSValue(Edge edge)
{
    VirtualRegister virtualRegister = edge->virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];
    
    switch (info.registerFormat()) {
    case DataFormatNone: {
        GPRReg gpr = allocate();

        if (edge->hasConstant()) {
            if (isInt32Constant(edge.node())) {
                info.fillJSValue(*m_stream, gpr, DataFormatJSInteger);
                JSValue jsValue = jsNumber(valueOfInt32Constant(edge.node()));
                m_jit.move(MacroAssembler::Imm64(JSValue::encode(jsValue)), gpr);
            } else if (isNumberConstant(edge.node())) {
                info.fillJSValue(*m_stream, gpr, DataFormatJSDouble);
                JSValue jsValue(JSValue::EncodeAsDouble, valueOfNumberConstant(edge.node()));
                m_jit.move(MacroAssembler::Imm64(JSValue::encode(jsValue)), gpr);
            } else {
                ASSERT(isJSConstant(edge.node()));
                JSValue jsValue = valueOfJSConstant(edge.node());
                m_jit.move(MacroAssembler::TrustedImm64(JSValue::encode(jsValue)), gpr);
                info.fillJSValue(*m_stream, gpr, DataFormatJS);
            }

            m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
        } else {
            DataFormat spillFormat = info.spillFormat();
            m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
            if (spillFormat == DataFormatInteger) {
                m_jit.load32(JITCompiler::addressFor(virtualRegister), gpr);
                m_jit.or64(GPRInfo::tagTypeNumberRegister, gpr);
                spillFormat = DataFormatJSInteger;
            } else {
                m_jit.load64(JITCompiler::addressFor(virtualRegister), gpr);
                if (spillFormat == DataFormatDouble) {
                    // Need to box the double, since we want a JSValue.
                    m_jit.sub64(GPRInfo::tagTypeNumberRegister, gpr);
                    spillFormat = DataFormatJSDouble;
                } else
                    RELEASE_ASSERT(spillFormat & DataFormatJS);
            }
            info.fillJSValue(*m_stream, gpr, spillFormat);
        }
        return gpr;
    }

    case DataFormatInteger: {
        GPRReg gpr = info.gpr();
        // If the register has already been locked we need to take a copy.
        // If not, we'll zero extend in place, so mark on the info that this is now type DataFormatInteger, not DataFormatJSInteger.
        if (m_gprs.isLocked(gpr)) {
            GPRReg result = allocate();
            m_jit.or64(GPRInfo::tagTypeNumberRegister, gpr, result);
            return result;
        }
        m_gprs.lock(gpr);
        m_jit.or64(GPRInfo::tagTypeNumberRegister, gpr);
        info.fillJSValue(*m_stream, gpr, DataFormatJSInteger);
        return gpr;
    }

    case DataFormatDouble: {
        FPRReg fpr = info.fpr();
        GPRReg gpr = boxDouble(fpr);

        // Update all info
        info.fillJSValue(*m_stream, gpr, DataFormatJSDouble);
        m_fprs.release(fpr);
        m_gprs.retain(gpr, virtualRegister, SpillOrderJS);

        return gpr;
    }

    case DataFormatCell:
        // No retag required on JSVALUE64!
    case DataFormatJS:
    case DataFormatJSInteger:
    case DataFormatJSDouble:
    case DataFormatJSCell:
    case DataFormatJSBoolean: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        return gpr;
    }
        
    case DataFormatBoolean:
    case DataFormatStorage:
        // this type currently never occurs
        RELEASE_ASSERT_NOT_REACHED();
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return InvalidGPRReg;
    }
}

void SpeculativeJIT::nonSpeculativeUInt32ToNumber(Node* node)
{
    IntegerOperand op1(this, node->child1());
    FPRTemporary boxer(this);
    GPRTemporary result(this, op1);
    
    JITCompiler::Jump positive = m_jit.branch32(MacroAssembler::GreaterThanOrEqual, op1.gpr(), TrustedImm32(0));
    
    m_jit.convertInt32ToDouble(op1.gpr(), boxer.fpr());
    m_jit.addDouble(JITCompiler::AbsoluteAddress(&AssemblyHelpers::twoToThe32), boxer.fpr());
    
    boxDouble(boxer.fpr(), result.gpr());
    
    JITCompiler::Jump done = m_jit.jump();
    
    positive.link(&m_jit);
    
    m_jit.or64(GPRInfo::tagTypeNumberRegister, op1.gpr(), result.gpr());
    
    done.link(&m_jit);
    
    jsValueResult(result.gpr(), m_currentNode);
}

void SpeculativeJIT::cachedGetById(CodeOrigin codeOrigin, GPRReg baseGPR, GPRReg resultGPR, unsigned identifierNumber, JITCompiler::Jump slowPathTarget, SpillRegistersMode spillMode)
{
    JITCompiler::DataLabelPtr structureToCompare;
    JITCompiler::PatchableJump structureCheck = m_jit.patchableBranchPtrWithPatch(JITCompiler::NotEqual, JITCompiler::Address(baseGPR, JSCell::structureOffset()), structureToCompare, JITCompiler::TrustedImmPtr(reinterpret_cast<void*>(unusedPointer)));
    
    JITCompiler::ConvertibleLoadLabel propertyStorageLoad =
        m_jit.convertibleLoadPtr(JITCompiler::Address(baseGPR, JSObject::butterflyOffset()), resultGPR);
    JITCompiler::DataLabelCompact loadWithPatch = m_jit.load64WithCompactAddressOffsetPatch(JITCompiler::Address(resultGPR, 0), resultGPR);
    
    JITCompiler::Label doneLabel = m_jit.label();

    OwnPtr<SlowPathGenerator> slowPath;
    if (!slowPathTarget.isSet()) {
        slowPath = slowPathCall(
            structureCheck.m_jump, this, operationGetByIdOptimize, resultGPR, baseGPR,
            identifier(identifierNumber), spillMode);
    } else {
        JITCompiler::JumpList slowCases;
        slowCases.append(structureCheck.m_jump);
        slowCases.append(slowPathTarget);
        slowPath = slowPathCall(
            slowCases, this, operationGetByIdOptimize, resultGPR, baseGPR,
            identifier(identifierNumber), spillMode);
    }
    m_jit.addPropertyAccess(
        PropertyAccessRecord(
            codeOrigin, structureToCompare, structureCheck, propertyStorageLoad, loadWithPatch,
            slowPath.get(), doneLabel, safeCast<int8_t>(baseGPR), safeCast<int8_t>(resultGPR),
            usedRegisters(),
            spillMode == NeedToSpill ? PropertyAccessRecord::RegistersInUse : PropertyAccessRecord::RegistersFlushed));
    addSlowPathGenerator(slowPath.release());
}

void SpeculativeJIT::cachedPutById(CodeOrigin codeOrigin, GPRReg baseGPR, GPRReg valueGPR, Edge valueUse, GPRReg scratchGPR, unsigned identifierNumber, PutKind putKind, JITCompiler::Jump slowPathTarget)
{
    
    JITCompiler::DataLabelPtr structureToCompare;
    JITCompiler::PatchableJump structureCheck = m_jit.patchableBranchPtrWithPatch(JITCompiler::NotEqual, JITCompiler::Address(baseGPR, JSCell::structureOffset()), structureToCompare, JITCompiler::TrustedImmPtr(reinterpret_cast<void*>(unusedPointer)));

    writeBarrier(baseGPR, valueGPR, valueUse, WriteBarrierForPropertyAccess, scratchGPR);

    JITCompiler::ConvertibleLoadLabel propertyStorageLoad =
        m_jit.convertibleLoadPtr(JITCompiler::Address(baseGPR, JSObject::butterflyOffset()), scratchGPR);
    JITCompiler::DataLabel32 storeWithPatch = m_jit.store64WithAddressOffsetPatch(valueGPR, JITCompiler::Address(scratchGPR, 0));

    JITCompiler::Label doneLabel = m_jit.label();
    
    V_DFGOperation_EJCI optimizedCall;
    if (m_jit.strictModeFor(m_currentNode->codeOrigin)) {
        if (putKind == Direct)
            optimizedCall = operationPutByIdDirectStrictOptimize;
        else
            optimizedCall = operationPutByIdStrictOptimize;
    } else {
        if (putKind == Direct)
            optimizedCall = operationPutByIdDirectNonStrictOptimize;
        else
            optimizedCall = operationPutByIdNonStrictOptimize;
    }
    OwnPtr<SlowPathGenerator> slowPath;
    if (!slowPathTarget.isSet()) {
        slowPath = slowPathCall(
            structureCheck.m_jump, this, optimizedCall, NoResult, valueGPR, baseGPR,
            identifier(identifierNumber));
    } else {
        JITCompiler::JumpList slowCases;
        slowCases.append(structureCheck.m_jump);
        slowCases.append(slowPathTarget);
        slowPath = slowPathCall(
            slowCases, this, optimizedCall, NoResult, valueGPR, baseGPR,
            identifier(identifierNumber));
    }
    RegisterSet currentlyUsedRegisters = usedRegisters();
    currentlyUsedRegisters.clear(scratchGPR);
    ASSERT(currentlyUsedRegisters.get(baseGPR));
    ASSERT(currentlyUsedRegisters.get(valueGPR));
    m_jit.addPropertyAccess(
        PropertyAccessRecord(
            codeOrigin, structureToCompare, structureCheck, propertyStorageLoad,
            JITCompiler::DataLabelCompact(storeWithPatch.label()), slowPath.get(), doneLabel,
            safeCast<int8_t>(baseGPR), safeCast<int8_t>(valueGPR), currentlyUsedRegisters));
    addSlowPathGenerator(slowPath.release());
}

void SpeculativeJIT::nonSpeculativeNonPeepholeCompareNull(Edge operand, bool invert)
{
    JSValueOperand arg(this, operand);
    GPRReg argGPR = arg.gpr();
    
    GPRTemporary result(this, arg);
    GPRReg resultGPR = result.gpr();
    
    JITCompiler::Jump notCell;
    
    JITCompiler::Jump notMasqueradesAsUndefined;
    if (m_jit.graph().globalObjectFor(operand->codeOrigin)->masqueradesAsUndefinedWatchpoint()->isStillValid()) {
        if (!isKnownCell(operand.node()))
            notCell = m_jit.branchTest64(MacroAssembler::NonZero, argGPR, GPRInfo::tagMaskRegister);

        m_jit.graph().globalObjectFor(operand->codeOrigin)->masqueradesAsUndefinedWatchpoint()->add(speculationWatchpoint());
        m_jit.move(invert ? TrustedImm32(1) : TrustedImm32(0), resultGPR);
        notMasqueradesAsUndefined = m_jit.jump();
    } else {
        GPRTemporary localGlobalObject(this);
        GPRTemporary remoteGlobalObject(this);

        if (!isKnownCell(operand.node()))
            notCell = m_jit.branchTest64(MacroAssembler::NonZero, argGPR, GPRInfo::tagMaskRegister);

        m_jit.loadPtr(JITCompiler::Address(argGPR, JSCell::structureOffset()), resultGPR);
        JITCompiler::Jump isMasqueradesAsUndefined = m_jit.branchTest8(JITCompiler::NonZero, JITCompiler::Address(resultGPR, Structure::typeInfoFlagsOffset()), JITCompiler::TrustedImm32(MasqueradesAsUndefined));

        m_jit.move(invert ? TrustedImm32(1) : TrustedImm32(0), resultGPR);
        notMasqueradesAsUndefined = m_jit.jump();

        isMasqueradesAsUndefined.link(&m_jit);
        GPRReg localGlobalObjectGPR = localGlobalObject.gpr();
        GPRReg remoteGlobalObjectGPR = remoteGlobalObject.gpr();
        m_jit.move(JITCompiler::TrustedImmPtr(m_jit.graph().globalObjectFor(operand->codeOrigin)), localGlobalObjectGPR);
        m_jit.loadPtr(JITCompiler::Address(resultGPR, Structure::globalObjectOffset()), remoteGlobalObjectGPR);
        m_jit.comparePtr(invert ? JITCompiler::NotEqual : JITCompiler::Equal, localGlobalObjectGPR, remoteGlobalObjectGPR, resultGPR);
    }
 
    if (!isKnownCell(operand.node())) {
        JITCompiler::Jump done = m_jit.jump();
        
        notCell.link(&m_jit);
        
        m_jit.move(argGPR, resultGPR);
        m_jit.and64(JITCompiler::TrustedImm32(~TagBitUndefined), resultGPR);
        m_jit.compare64(invert ? JITCompiler::NotEqual : JITCompiler::Equal, resultGPR, JITCompiler::TrustedImm32(ValueNull), resultGPR);
        
        done.link(&m_jit);
    }
   
    notMasqueradesAsUndefined.link(&m_jit);
 
    m_jit.or32(TrustedImm32(ValueFalse), resultGPR);
    jsValueResult(resultGPR, m_currentNode, DataFormatJSBoolean);
}

void SpeculativeJIT::nonSpeculativePeepholeBranchNull(Edge operand, Node* branchNode, bool invert)
{
    BlockIndex taken = branchNode->takenBlockIndex();
    BlockIndex notTaken = branchNode->notTakenBlockIndex();
    
    if (taken == nextBlock()) {
        invert = !invert;
        BlockIndex tmp = taken;
        taken = notTaken;
        notTaken = tmp;
    }

    JSValueOperand arg(this, operand);
    GPRReg argGPR = arg.gpr();
    
    GPRTemporary result(this, arg);
    GPRReg resultGPR = result.gpr();
    
    JITCompiler::Jump notCell;
    
    if (m_jit.graph().globalObjectFor(operand->codeOrigin)->masqueradesAsUndefinedWatchpoint()->isStillValid()) {
        if (!isKnownCell(operand.node()))
            notCell = m_jit.branchTest64(MacroAssembler::NonZero, argGPR, GPRInfo::tagMaskRegister);

        m_jit.graph().globalObjectFor(operand->codeOrigin)->masqueradesAsUndefinedWatchpoint()->add(speculationWatchpoint());
        jump(invert ? taken : notTaken, ForceJump);
    } else {
        GPRTemporary localGlobalObject(this);
        GPRTemporary remoteGlobalObject(this);

        if (!isKnownCell(operand.node()))
            notCell = m_jit.branchTest64(MacroAssembler::NonZero, argGPR, GPRInfo::tagMaskRegister);

        m_jit.loadPtr(JITCompiler::Address(argGPR, JSCell::structureOffset()), resultGPR);
        branchTest8(JITCompiler::Zero, JITCompiler::Address(resultGPR, Structure::typeInfoFlagsOffset()), JITCompiler::TrustedImm32(MasqueradesAsUndefined), invert ? taken : notTaken);

        GPRReg localGlobalObjectGPR = localGlobalObject.gpr();
        GPRReg remoteGlobalObjectGPR = remoteGlobalObject.gpr();
        m_jit.move(TrustedImmPtr(m_jit.graph().globalObjectFor(operand->codeOrigin)), localGlobalObjectGPR);
        m_jit.loadPtr(JITCompiler::Address(resultGPR, Structure::globalObjectOffset()), remoteGlobalObjectGPR);
        branchPtr(JITCompiler::Equal, localGlobalObjectGPR, remoteGlobalObjectGPR, invert ? notTaken : taken);
    }
 
    if (!isKnownCell(operand.node())) {
        jump(notTaken, ForceJump);
        
        notCell.link(&m_jit);
        
        m_jit.move(argGPR, resultGPR);
        m_jit.and64(JITCompiler::TrustedImm32(~TagBitUndefined), resultGPR);
        branch64(invert ? JITCompiler::NotEqual : JITCompiler::Equal, resultGPR, JITCompiler::TrustedImm64(ValueNull), taken);
    }
    
    jump(notTaken);
}

bool SpeculativeJIT::nonSpeculativeCompareNull(Node* node, Edge operand, bool invert)
{
    unsigned branchIndexInBlock = detectPeepHoleBranch();
    if (branchIndexInBlock != UINT_MAX) {
        Node* branchNode = m_jit.graph().m_blocks[m_block]->at(branchIndexInBlock);

        RELEASE_ASSERT(node->adjustedRefCount() == 1);
        
        nonSpeculativePeepholeBranchNull(operand, branchNode, invert);
    
        use(node->child1());
        use(node->child2());
        m_indexInBlock = branchIndexInBlock;
        m_currentNode = branchNode;
        
        return true;
    }
    
    nonSpeculativeNonPeepholeCompareNull(operand, invert);
    
    return false;
}

void SpeculativeJIT::nonSpeculativePeepholeBranch(Node* node, Node* branchNode, MacroAssembler::RelationalCondition cond, S_DFGOperation_EJJ helperFunction)
{
    BlockIndex taken = branchNode->takenBlockIndex();
    BlockIndex notTaken = branchNode->notTakenBlockIndex();

    JITCompiler::ResultCondition callResultCondition = JITCompiler::NonZero;

    // The branch instruction will branch to the taken block.
    // If taken is next, switch taken with notTaken & invert the branch condition so we can fall through.
    if (taken == nextBlock()) {
        cond = JITCompiler::invert(cond);
        callResultCondition = JITCompiler::Zero;
        BlockIndex tmp = taken;
        taken = notTaken;
        notTaken = tmp;
    }

    JSValueOperand arg1(this, node->child1());
    JSValueOperand arg2(this, node->child2());
    GPRReg arg1GPR = arg1.gpr();
    GPRReg arg2GPR = arg2.gpr();
    
    JITCompiler::JumpList slowPath;
    
    if (isKnownNotInteger(node->child1().node()) || isKnownNotInteger(node->child2().node())) {
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();
    
        arg1.use();
        arg2.use();
    
        flushRegisters();
        callOperation(helperFunction, resultGPR, arg1GPR, arg2GPR);

        branchTest32(callResultCondition, resultGPR, taken);
    } else {
        GPRTemporary result(this, arg2);
        GPRReg resultGPR = result.gpr();
    
        arg1.use();
        arg2.use();
    
        if (!isKnownInteger(node->child1().node()))
            slowPath.append(m_jit.branch64(MacroAssembler::Below, arg1GPR, GPRInfo::tagTypeNumberRegister));
        if (!isKnownInteger(node->child2().node()))
            slowPath.append(m_jit.branch64(MacroAssembler::Below, arg2GPR, GPRInfo::tagTypeNumberRegister));
    
        branch32(cond, arg1GPR, arg2GPR, taken);
    
        if (!isKnownInteger(node->child1().node()) || !isKnownInteger(node->child2().node())) {
            jump(notTaken, ForceJump);
    
            slowPath.link(&m_jit);
    
            silentSpillAllRegisters(resultGPR);
            callOperation(helperFunction, resultGPR, arg1GPR, arg2GPR);
            silentFillAllRegisters(resultGPR);
        
            branchTest32(callResultCondition, resultGPR, taken);
        }
    }

    jump(notTaken);

    m_indexInBlock = m_jit.graph().m_blocks[m_block]->size() - 1;
    m_currentNode = branchNode;
}

template<typename JumpType>
class CompareAndBoxBooleanSlowPathGenerator
    : public CallSlowPathGenerator<JumpType, S_DFGOperation_EJJ, GPRReg> {
public:
    CompareAndBoxBooleanSlowPathGenerator(
        JumpType from, SpeculativeJIT* jit,
        S_DFGOperation_EJJ function, GPRReg result, GPRReg arg1, GPRReg arg2)
        : CallSlowPathGenerator<JumpType, S_DFGOperation_EJJ, GPRReg>(
            from, jit, function, NeedToSpill, result)
        , m_arg1(arg1)
        , m_arg2(arg2)
    {
    }
    
protected:
    virtual void generateInternal(SpeculativeJIT* jit)
    {
        this->setUp(jit);
        this->recordCall(jit->callOperation(this->m_function, this->m_result, m_arg1, m_arg2));
        jit->m_jit.and32(JITCompiler::TrustedImm32(1), this->m_result);
        jit->m_jit.or32(JITCompiler::TrustedImm32(ValueFalse), this->m_result);
        this->tearDown(jit);
    }
   
private:
    GPRReg m_arg1;
    GPRReg m_arg2;
};

void SpeculativeJIT::nonSpeculativeNonPeepholeCompare(Node* node, MacroAssembler::RelationalCondition cond, S_DFGOperation_EJJ helperFunction)
{
    JSValueOperand arg1(this, node->child1());
    JSValueOperand arg2(this, node->child2());
    GPRReg arg1GPR = arg1.gpr();
    GPRReg arg2GPR = arg2.gpr();
    
    JITCompiler::JumpList slowPath;
    
    if (isKnownNotInteger(node->child1().node()) || isKnownNotInteger(node->child2().node())) {
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();
    
        arg1.use();
        arg2.use();
    
        flushRegisters();
        callOperation(helperFunction, resultGPR, arg1GPR, arg2GPR);
        
        m_jit.or32(TrustedImm32(ValueFalse), resultGPR);
        jsValueResult(resultGPR, m_currentNode, DataFormatJSBoolean, UseChildrenCalledExplicitly);
    } else {
        GPRTemporary result(this, arg2);
        GPRReg resultGPR = result.gpr();

        arg1.use();
        arg2.use();
    
        if (!isKnownInteger(node->child1().node()))
            slowPath.append(m_jit.branch64(MacroAssembler::Below, arg1GPR, GPRInfo::tagTypeNumberRegister));
        if (!isKnownInteger(node->child2().node()))
            slowPath.append(m_jit.branch64(MacroAssembler::Below, arg2GPR, GPRInfo::tagTypeNumberRegister));
    
        m_jit.compare32(cond, arg1GPR, arg2GPR, resultGPR);
        m_jit.or32(TrustedImm32(ValueFalse), resultGPR);
        
        if (!isKnownInteger(node->child1().node()) || !isKnownInteger(node->child2().node())) {
            addSlowPathGenerator(adoptPtr(
                new CompareAndBoxBooleanSlowPathGenerator<JITCompiler::JumpList>(
                    slowPath, this, helperFunction, resultGPR, arg1GPR, arg2GPR)));
        }

        jsValueResult(resultGPR, m_currentNode, DataFormatJSBoolean, UseChildrenCalledExplicitly);
    }
}

void SpeculativeJIT::nonSpeculativePeepholeStrictEq(Node* node, Node* branchNode, bool invert)
{
    BlockIndex taken = branchNode->takenBlockIndex();
    BlockIndex notTaken = branchNode->notTakenBlockIndex();

    // The branch instruction will branch to the taken block.
    // If taken is next, switch taken with notTaken & invert the branch condition so we can fall through.
    if (taken == nextBlock()) {
        invert = !invert;
        BlockIndex tmp = taken;
        taken = notTaken;
        notTaken = tmp;
    }
    
    JSValueOperand arg1(this, node->child1());
    JSValueOperand arg2(this, node->child2());
    GPRReg arg1GPR = arg1.gpr();
    GPRReg arg2GPR = arg2.gpr();
    
    GPRTemporary result(this);
    GPRReg resultGPR = result.gpr();
    
    arg1.use();
    arg2.use();
    
    if (isKnownCell(node->child1().node()) && isKnownCell(node->child2().node())) {
        // see if we get lucky: if the arguments are cells and they reference the same
        // cell, then they must be strictly equal.
        branch64(JITCompiler::Equal, arg1GPR, arg2GPR, invert ? notTaken : taken);
        
        silentSpillAllRegisters(resultGPR);
        callOperation(operationCompareStrictEqCell, resultGPR, arg1GPR, arg2GPR);
        silentFillAllRegisters(resultGPR);
        
        branchTest32(invert ? JITCompiler::Zero : JITCompiler::NonZero, resultGPR, taken);
    } else {
        m_jit.or64(arg1GPR, arg2GPR, resultGPR);
        
        JITCompiler::Jump twoCellsCase = m_jit.branchTest64(JITCompiler::Zero, resultGPR, GPRInfo::tagMaskRegister);
        
        JITCompiler::Jump leftOK = m_jit.branch64(JITCompiler::AboveOrEqual, arg1GPR, GPRInfo::tagTypeNumberRegister);
        JITCompiler::Jump leftDouble = m_jit.branchTest64(JITCompiler::NonZero, arg1GPR, GPRInfo::tagTypeNumberRegister);
        leftOK.link(&m_jit);
        JITCompiler::Jump rightOK = m_jit.branch64(JITCompiler::AboveOrEqual, arg2GPR, GPRInfo::tagTypeNumberRegister);
        JITCompiler::Jump rightDouble = m_jit.branchTest64(JITCompiler::NonZero, arg2GPR, GPRInfo::tagTypeNumberRegister);
        rightOK.link(&m_jit);
        
        branch64(invert ? JITCompiler::NotEqual : JITCompiler::Equal, arg1GPR, arg2GPR, taken);
        jump(notTaken, ForceJump);
        
        twoCellsCase.link(&m_jit);
        branch64(JITCompiler::Equal, arg1GPR, arg2GPR, invert ? notTaken : taken);
        
        leftDouble.link(&m_jit);
        rightDouble.link(&m_jit);
        
        silentSpillAllRegisters(resultGPR);
        callOperation(operationCompareStrictEq, resultGPR, arg1GPR, arg2GPR);
        silentFillAllRegisters(resultGPR);
        
        branchTest32(invert ? JITCompiler::Zero : JITCompiler::NonZero, resultGPR, taken);
    }
    
    jump(notTaken);
}

void SpeculativeJIT::nonSpeculativeNonPeepholeStrictEq(Node* node, bool invert)
{
    JSValueOperand arg1(this, node->child1());
    JSValueOperand arg2(this, node->child2());
    GPRReg arg1GPR = arg1.gpr();
    GPRReg arg2GPR = arg2.gpr();
    
    GPRTemporary result(this);
    GPRReg resultGPR = result.gpr();
    
    arg1.use();
    arg2.use();
    
    if (isKnownCell(node->child1().node()) && isKnownCell(node->child2().node())) {
        // see if we get lucky: if the arguments are cells and they reference the same
        // cell, then they must be strictly equal.
        // FIXME: this should flush registers instead of silent spill/fill.
        JITCompiler::Jump notEqualCase = m_jit.branch64(JITCompiler::NotEqual, arg1GPR, arg2GPR);
        
        m_jit.move(JITCompiler::TrustedImm64(JSValue::encode(jsBoolean(!invert))), resultGPR);
        
        JITCompiler::Jump done = m_jit.jump();

        notEqualCase.link(&m_jit);
        
        silentSpillAllRegisters(resultGPR);
        callOperation(operationCompareStrictEqCell, resultGPR, arg1GPR, arg2GPR);
        silentFillAllRegisters(resultGPR);
        
        m_jit.and64(JITCompiler::TrustedImm32(1), resultGPR);
        m_jit.or32(JITCompiler::TrustedImm32(ValueFalse), resultGPR);
        
        done.link(&m_jit);
    } else {
        m_jit.or64(arg1GPR, arg2GPR, resultGPR);
        
        JITCompiler::JumpList slowPathCases;
        
        JITCompiler::Jump twoCellsCase = m_jit.branchTest64(JITCompiler::Zero, resultGPR, GPRInfo::tagMaskRegister);
        
        JITCompiler::Jump leftOK = m_jit.branch64(JITCompiler::AboveOrEqual, arg1GPR, GPRInfo::tagTypeNumberRegister);
        slowPathCases.append(m_jit.branchTest64(JITCompiler::NonZero, arg1GPR, GPRInfo::tagTypeNumberRegister));
        leftOK.link(&m_jit);
        JITCompiler::Jump rightOK = m_jit.branch64(JITCompiler::AboveOrEqual, arg2GPR, GPRInfo::tagTypeNumberRegister);
        slowPathCases.append(m_jit.branchTest64(JITCompiler::NonZero, arg2GPR, GPRInfo::tagTypeNumberRegister));
        rightOK.link(&m_jit);
        
        m_jit.compare64(invert ? JITCompiler::NotEqual : JITCompiler::Equal, arg1GPR, arg2GPR, resultGPR);
        m_jit.or32(JITCompiler::TrustedImm32(ValueFalse), resultGPR);
        
        JITCompiler::Jump done = m_jit.jump();
        
        twoCellsCase.link(&m_jit);
        slowPathCases.append(m_jit.branch64(JITCompiler::NotEqual, arg1GPR, arg2GPR));
        
        m_jit.move(JITCompiler::TrustedImm64(JSValue::encode(jsBoolean(!invert))), resultGPR);
        
        addSlowPathGenerator(
            adoptPtr(
                new CompareAndBoxBooleanSlowPathGenerator<MacroAssembler::JumpList>(
                    slowPathCases, this, operationCompareStrictEq, resultGPR, arg1GPR,
                    arg2GPR)));
        
        done.link(&m_jit);
    }
    
    jsValueResult(resultGPR, m_currentNode, DataFormatJSBoolean, UseChildrenCalledExplicitly);
}

void SpeculativeJIT::emitCall(Node* node)
{
    if (node->op() != Call)
        RELEASE_ASSERT(node->op() == Construct);

    // For constructors, the this argument is not passed but we have to make space
    // for it.
    int dummyThisArgument = node->op() == Call ? 0 : 1;
    
    CallLinkInfo::CallType callType = node->op() == Call ? CallLinkInfo::Call : CallLinkInfo::Construct;
    
    Edge calleeEdge = m_jit.graph().m_varArgChildren[node->firstChild()];
    JSValueOperand callee(this, calleeEdge);
    GPRReg calleeGPR = callee.gpr();
    use(calleeEdge);
    
    // The call instruction's first child is the function; the subsequent children are the
    // arguments.
    int numPassedArgs = node->numChildren() - 1;
    
    m_jit.store32(MacroAssembler::TrustedImm32(numPassedArgs + dummyThisArgument), callFramePayloadSlot(JSStack::ArgumentCount));
    m_jit.store64(GPRInfo::callFrameRegister, callFrameSlot(JSStack::CallerFrame));
    m_jit.store64(calleeGPR, callFrameSlot(JSStack::Callee));
    
    for (int i = 0; i < numPassedArgs; i++) {
        Edge argEdge = m_jit.graph().m_varArgChildren[node->firstChild() + 1 + i];
        JSValueOperand arg(this, argEdge);
        GPRReg argGPR = arg.gpr();
        use(argEdge);
        
        m_jit.store64(argGPR, argumentSlot(i + dummyThisArgument));
    }

    flushRegisters();

    GPRResult result(this);
    GPRReg resultGPR = result.gpr();

    JITCompiler::DataLabelPtr targetToCheck;
    JITCompiler::JumpList slowPath;

    CallBeginToken token;
    m_jit.beginCall(node->codeOrigin, token);
    
    m_jit.addPtr(TrustedImm32(m_jit.codeBlock()->m_numCalleeRegisters * sizeof(Register)), GPRInfo::callFrameRegister);
    
    slowPath.append(m_jit.branchPtrWithPatch(MacroAssembler::NotEqual, calleeGPR, targetToCheck, MacroAssembler::TrustedImmPtr(0)));

    m_jit.loadPtr(MacroAssembler::Address(calleeGPR, OBJECT_OFFSETOF(JSFunction, m_scope)), resultGPR);
    m_jit.store64(resultGPR, MacroAssembler::Address(GPRInfo::callFrameRegister, static_cast<ptrdiff_t>(sizeof(Register)) * JSStack::ScopeChain));

    CodeOrigin codeOrigin = m_currentNode->codeOrigin;
    JITCompiler::Call fastCall = m_jit.nearCall();
    m_jit.notifyCall(fastCall, codeOrigin, token);
    
    JITCompiler::Jump done = m_jit.jump();
    
    slowPath.link(&m_jit);
    
    m_jit.move(calleeGPR, GPRInfo::nonArgGPR0);
    m_jit.prepareForExceptionCheck();
    JITCompiler::Call slowCall = m_jit.nearCall();
    m_jit.notifyCall(slowCall, codeOrigin, token);
    
    done.link(&m_jit);
    
    m_jit.move(GPRInfo::returnValueGPR, resultGPR);
    
    jsValueResult(resultGPR, m_currentNode, DataFormatJS, UseChildrenCalledExplicitly);
    
    m_jit.addJSCall(fastCall, slowCall, targetToCheck, callType, calleeGPR, m_currentNode->codeOrigin);
}

template<bool strict>
GPRReg SpeculativeJIT::fillSpeculateIntInternal(Edge edge, DataFormat& returnFormat)
{
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLogF("SpecInt@%d   ", edge->index());
#endif
    AbstractValue& value = m_state.forNode(edge);
    SpeculatedType type = value.m_type;
    ASSERT(edge.useKind() != KnownInt32Use || !(value.m_type & ~SpecInt32));
    value.filter(SpecInt32);
    VirtualRegister virtualRegister = edge->virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    switch (info.registerFormat()) {
    case DataFormatNone: {
        if ((edge->hasConstant() && !isInt32Constant(edge.node())) || info.spillFormat() == DataFormatDouble) {
            terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
            returnFormat = DataFormatInteger;
            return allocate();
        }
        
        GPRReg gpr = allocate();

        if (edge->hasConstant()) {
            m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
            ASSERT(isInt32Constant(edge.node()));
            m_jit.move(MacroAssembler::Imm32(valueOfInt32Constant(edge.node())), gpr);
            info.fillInteger(*m_stream, gpr);
            returnFormat = DataFormatInteger;
            return gpr;
        }
        
        DataFormat spillFormat = info.spillFormat();
        
        RELEASE_ASSERT((spillFormat & DataFormatJS) || spillFormat == DataFormatInteger);
        
        m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
        
        if (spillFormat == DataFormatJSInteger || spillFormat == DataFormatInteger) {
            // If we know this was spilled as an integer we can fill without checking.
            if (strict) {
                m_jit.load32(JITCompiler::addressFor(virtualRegister), gpr);
                info.fillInteger(*m_stream, gpr);
                returnFormat = DataFormatInteger;
                return gpr;
            }
            if (spillFormat == DataFormatInteger) {
                m_jit.load32(JITCompiler::addressFor(virtualRegister), gpr);
                m_jit.or64(GPRInfo::tagTypeNumberRegister, gpr);
            } else
                m_jit.load64(JITCompiler::addressFor(virtualRegister), gpr);
            info.fillJSValue(*m_stream, gpr, DataFormatJSInteger);
            returnFormat = DataFormatJSInteger;
            return gpr;
        }
        m_jit.load64(JITCompiler::addressFor(virtualRegister), gpr);

        // Fill as JSValue, and fall through.
        info.fillJSValue(*m_stream, gpr, DataFormatJSInteger);
        m_gprs.unlock(gpr);
    }

    case DataFormatJS: {
        // Check the value is an integer.
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        if (type & ~SpecInt32)
            speculationCheck(BadType, JSValueRegs(gpr), edge, m_jit.branch64(MacroAssembler::Below, gpr, GPRInfo::tagTypeNumberRegister));
        info.fillJSValue(*m_stream, gpr, DataFormatJSInteger);
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
                info.fillInteger(*m_stream, gpr);
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
    case DataFormatJSDouble: {
        if (edge->hasConstant() && isInt32Constant(edge.node())) {
            GPRReg gpr = allocate();
            ASSERT(isInt32Constant(edge.node()));
            m_jit.move(MacroAssembler::Imm32(valueOfInt32Constant(edge.node())), gpr);
            returnFormat = DataFormatInteger;
            return gpr;
        }
    }
    case DataFormatCell:
    case DataFormatBoolean:
    case DataFormatJSCell:
    case DataFormatJSBoolean: {
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
        returnFormat = DataFormatInteger;
        return allocate();
    }

    case DataFormatStorage:
        RELEASE_ASSERT_NOT_REACHED();
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return InvalidGPRReg;
    }
}

GPRReg SpeculativeJIT::fillSpeculateInt(Edge edge, DataFormat& returnFormat)
{
    return fillSpeculateIntInternal<false>(edge, returnFormat);
}

GPRReg SpeculativeJIT::fillSpeculateIntStrict(Edge edge)
{
    DataFormat mustBeDataFormatInteger;
    GPRReg result = fillSpeculateIntInternal<true>(edge, mustBeDataFormatInteger);
    RELEASE_ASSERT(mustBeDataFormatInteger == DataFormatInteger);
    return result;
}

FPRReg SpeculativeJIT::fillSpeculateDouble(Edge edge)
{
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLogF("SpecDouble@%d   ", edge->index());
#endif
    AbstractValue& value = m_state.forNode(edge);
    SpeculatedType type = value.m_type;
    ASSERT(edge.useKind() != KnownNumberUse || !(value.m_type & ~SpecNumber));
    value.filter(SpecNumber);
    VirtualRegister virtualRegister = edge->virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    if (info.registerFormat() == DataFormatNone) {
        if (edge->hasConstant()) {
            GPRReg gpr = allocate();

            if (isInt32Constant(edge.node())) {
                FPRReg fpr = fprAllocate();
                m_jit.move(MacroAssembler::Imm64(reinterpretDoubleToInt64(static_cast<double>(valueOfInt32Constant(edge.node())))), gpr);
                m_jit.move64ToDouble(gpr, fpr);
                unlock(gpr);

                m_fprs.retain(fpr, virtualRegister, SpillOrderDouble);
                info.fillDouble(*m_stream, fpr);
                return fpr;
            }
            if (isNumberConstant(edge.node())) {
                FPRReg fpr = fprAllocate();
                m_jit.move(MacroAssembler::Imm64(reinterpretDoubleToInt64(valueOfNumberConstant(edge.node()))), gpr);
                m_jit.move64ToDouble(gpr, fpr);
                unlock(gpr);

                m_fprs.retain(fpr, virtualRegister, SpillOrderDouble);
                info.fillDouble(*m_stream, fpr);
                return fpr;
            }
            terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
            return fprAllocate();
        }
        
        DataFormat spillFormat = info.spillFormat();
        switch (spillFormat) {
        case DataFormatDouble: {
            FPRReg fpr = fprAllocate();
            m_jit.loadDouble(JITCompiler::addressFor(virtualRegister), fpr);
            m_fprs.retain(fpr, virtualRegister, SpillOrderDouble);
            info.fillDouble(*m_stream, fpr);
            return fpr;
        }
            
        case DataFormatInteger: {
            GPRReg gpr = allocate();
            
            m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
            m_jit.load32(JITCompiler::addressFor(virtualRegister), gpr);
            info.fillInteger(*m_stream, gpr);
            unlock(gpr);
            break;
        }

        default:
            GPRReg gpr = allocate();

            RELEASE_ASSERT(spillFormat & DataFormatJS);
            m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
            m_jit.load64(JITCompiler::addressFor(virtualRegister), gpr);
            info.fillJSValue(*m_stream, gpr, spillFormat);
            unlock(gpr);
            break;
        }
    }

    switch (info.registerFormat()) {
    case DataFormatNone: // Should have filled, above.
    case DataFormatBoolean: // This type never occurs.
    case DataFormatStorage:
        RELEASE_ASSERT_NOT_REACHED();

    case DataFormatCell:
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
        return fprAllocate();

    case DataFormatJSCell:
    case DataFormatJS:
    case DataFormatJSBoolean: {
        GPRReg jsValueGpr = info.gpr();
        m_gprs.lock(jsValueGpr);
        FPRReg fpr = fprAllocate();
        GPRReg tempGpr = allocate();

        JITCompiler::Jump isInteger = m_jit.branch64(MacroAssembler::AboveOrEqual, jsValueGpr, GPRInfo::tagTypeNumberRegister);

        if (type & ~SpecNumber)
            speculationCheck(BadType, JSValueRegs(jsValueGpr), edge, m_jit.branchTest64(MacroAssembler::Zero, jsValueGpr, GPRInfo::tagTypeNumberRegister));

        // First, if we get here we have a double encoded as a JSValue
        m_jit.move(jsValueGpr, tempGpr);
        unboxDouble(tempGpr, fpr);
        JITCompiler::Jump hasUnboxedDouble = m_jit.jump();

        // Finally, handle integers.
        isInteger.link(&m_jit);
        m_jit.convertInt32ToDouble(jsValueGpr, fpr);
        hasUnboxedDouble.link(&m_jit);

        m_gprs.release(jsValueGpr);
        m_gprs.unlock(jsValueGpr);
        m_gprs.unlock(tempGpr);
        m_fprs.retain(fpr, virtualRegister, SpillOrderDouble);
        info.fillDouble(*m_stream, fpr);
        info.killSpilled();
        return fpr;
    }

    case DataFormatJSInteger:
    case DataFormatInteger: {
        FPRReg fpr = fprAllocate();
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        m_jit.convertInt32ToDouble(gpr, fpr);
        m_gprs.unlock(gpr);
        return fpr;
    }

    // Unbox the double
    case DataFormatJSDouble: {
        GPRReg gpr = info.gpr();
        FPRReg fpr = fprAllocate();
        if (m_gprs.isLocked(gpr)) {
            // Make sure we don't trample gpr if it is in use.
            GPRReg temp = allocate();
            m_jit.move(gpr, temp);
            unboxDouble(temp, fpr);
            unlock(temp);
        } else
            unboxDouble(gpr, fpr);

        m_gprs.release(gpr);
        m_fprs.retain(fpr, virtualRegister, SpillOrderDouble);

        info.fillDouble(*m_stream, fpr);
        return fpr;
    }

    case DataFormatDouble: {
        FPRReg fpr = info.fpr();
        m_fprs.lock(fpr);
        return fpr;
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return InvalidFPRReg;
    }
}

GPRReg SpeculativeJIT::fillSpeculateCell(Edge edge)
{
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLogF("SpecCell@%d   ", edge->index());
#endif
    AbstractValue& value = m_state.forNode(edge);
    SpeculatedType type = value.m_type;
    ASSERT((edge.useKind() != KnownCellUse && edge.useKind() != KnownStringUse) || !(value.m_type & ~SpecCell));
    value.filter(SpecCell);
    VirtualRegister virtualRegister = edge->virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    switch (info.registerFormat()) {
    case DataFormatNone: {
        if (info.spillFormat() == DataFormatInteger || info.spillFormat() == DataFormatDouble) {
            terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
            return allocate();
        }
        
        GPRReg gpr = allocate();

        if (edge->hasConstant()) {
            JSValue jsValue = valueOfJSConstant(edge.node());
            if (jsValue.isCell()) {
                m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
                m_jit.move(MacroAssembler::TrustedImm64(JSValue::encode(jsValue)), gpr);
                info.fillJSValue(*m_stream, gpr, DataFormatJSCell);
                return gpr;
            }
            terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
            return gpr;
        }
        RELEASE_ASSERT(info.spillFormat() & DataFormatJS);
        m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
        m_jit.load64(JITCompiler::addressFor(virtualRegister), gpr);

        info.fillJSValue(*m_stream, gpr, DataFormatJS);
        if (type & ~SpecCell)
            speculationCheck(BadType, JSValueRegs(gpr), edge, m_jit.branchTest64(MacroAssembler::NonZero, gpr, GPRInfo::tagMaskRegister));
        info.fillJSValue(*m_stream, gpr, DataFormatJSCell);
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
        if (type & ~SpecCell)
            speculationCheck(BadType, JSValueRegs(gpr), edge, m_jit.branchTest64(MacroAssembler::NonZero, gpr, GPRInfo::tagMaskRegister));
        info.fillJSValue(*m_stream, gpr, DataFormatJSCell);
        return gpr;
    }

    case DataFormatJSInteger:
    case DataFormatInteger:
    case DataFormatJSDouble:
    case DataFormatDouble:
    case DataFormatJSBoolean:
    case DataFormatBoolean: {
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
        return allocate();
    }

    case DataFormatStorage:
        RELEASE_ASSERT_NOT_REACHED();
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return InvalidGPRReg;
    }
}

GPRReg SpeculativeJIT::fillSpeculateBoolean(Edge edge)
{
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLogF("SpecBool@%d   ", edge->index());
#endif
    AbstractValue& value = m_state.forNode(edge);
    SpeculatedType type = value.m_type;
    value.filter(SpecBoolean);
    VirtualRegister virtualRegister = edge->virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    switch (info.registerFormat()) {
    case DataFormatNone: {
        if (info.spillFormat() == DataFormatInteger || info.spillFormat() == DataFormatDouble) {
            terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
            return allocate();
        }
        
        GPRReg gpr = allocate();

        if (edge->hasConstant()) {
            JSValue jsValue = valueOfJSConstant(edge.node());
            if (jsValue.isBoolean()) {
                m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
                m_jit.move(MacroAssembler::TrustedImm64(JSValue::encode(jsValue)), gpr);
                info.fillJSValue(*m_stream, gpr, DataFormatJSBoolean);
                return gpr;
            }
            terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
            return gpr;
        }
        RELEASE_ASSERT(info.spillFormat() & DataFormatJS);
        m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
        m_jit.load64(JITCompiler::addressFor(virtualRegister), gpr);

        info.fillJSValue(*m_stream, gpr, DataFormatJS);
        if (type & ~SpecBoolean) {
            m_jit.xor64(TrustedImm32(static_cast<int32_t>(ValueFalse)), gpr);
            speculationCheck(BadType, JSValueRegs(gpr), edge, m_jit.branchTest64(MacroAssembler::NonZero, gpr, TrustedImm32(static_cast<int32_t>(~1))), SpeculationRecovery(BooleanSpeculationCheck, gpr, InvalidGPRReg));
            m_jit.xor64(TrustedImm32(static_cast<int32_t>(ValueFalse)), gpr);
        }
        info.fillJSValue(*m_stream, gpr, DataFormatJSBoolean);
        return gpr;
    }

    case DataFormatBoolean:
    case DataFormatJSBoolean: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        return gpr;
    }

    case DataFormatJS: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        if (type & ~SpecBoolean) {
            m_jit.xor64(TrustedImm32(static_cast<int32_t>(ValueFalse)), gpr);
            speculationCheck(BadType, JSValueRegs(gpr), edge, m_jit.branchTest64(MacroAssembler::NonZero, gpr, TrustedImm32(static_cast<int32_t>(~1))), SpeculationRecovery(BooleanSpeculationCheck, gpr, InvalidGPRReg));
            m_jit.xor64(TrustedImm32(static_cast<int32_t>(ValueFalse)), gpr);
        }
        info.fillJSValue(*m_stream, gpr, DataFormatJSBoolean);
        return gpr;
    }

    case DataFormatJSInteger:
    case DataFormatInteger:
    case DataFormatJSDouble:
    case DataFormatDouble:
    case DataFormatJSCell:
    case DataFormatCell: {
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
        return allocate();
    }
        
    case DataFormatStorage:
        RELEASE_ASSERT_NOT_REACHED();
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return InvalidGPRReg;
    }
}

JITCompiler::Jump SpeculativeJIT::convertToDouble(GPRReg value, FPRReg result, GPRReg tmp)
{
    JITCompiler::Jump isInteger = m_jit.branch64(MacroAssembler::AboveOrEqual, value, GPRInfo::tagTypeNumberRegister);
    
    JITCompiler::Jump notNumber = m_jit.branchTest64(MacroAssembler::Zero, value, GPRInfo::tagTypeNumberRegister);
    
    m_jit.move(value, tmp);
    unboxDouble(tmp, result);
    
    JITCompiler::Jump done = m_jit.jump();
    
    isInteger.link(&m_jit);
    
    m_jit.convertInt32ToDouble(value, result);
    
    done.link(&m_jit);

    return notNumber;
}

void SpeculativeJIT::compileObjectEquality(Node* node)
{
    SpeculateCellOperand op1(this, node->child1());
    SpeculateCellOperand op2(this, node->child2());
    GPRTemporary result(this, op1);
    
    GPRReg op1GPR = op1.gpr();
    GPRReg op2GPR = op2.gpr();
    GPRReg resultGPR = result.gpr();
   
    if (m_jit.graph().globalObjectFor(node->codeOrigin)->masqueradesAsUndefinedWatchpoint()->isStillValid()) {
        m_jit.graph().globalObjectFor(node->codeOrigin)->masqueradesAsUndefinedWatchpoint()->add(speculationWatchpoint());
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op1GPR), node->child1(), SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op1GPR, JSCell::structureOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op2GPR), node->child2(), SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op2GPR, JSCell::structureOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
    } else {
        GPRTemporary structure(this);
        GPRReg structureGPR = structure.gpr();

        m_jit.loadPtr(MacroAssembler::Address(op1GPR, JSCell::structureOffset()), structureGPR);
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op1GPR), node->child1(), SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal, 
                structureGPR, 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
        speculationCheck(BadType, JSValueSource::unboxedCell(op1GPR), node->child1(),
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(structureGPR, Structure::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));

        m_jit.loadPtr(MacroAssembler::Address(op2GPR, JSCell::structureOffset()), structureGPR);
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op2GPR), node->child2(), SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal, 
                structureGPR, 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
        speculationCheck(BadType, JSValueSource::unboxedCell(op2GPR), node->child2(),
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(structureGPR, Structure::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));
    }
    
    MacroAssembler::Jump falseCase = m_jit.branch64(MacroAssembler::NotEqual, op1GPR, op2GPR);
    m_jit.move(TrustedImm32(ValueTrue), resultGPR);
    MacroAssembler::Jump done = m_jit.jump();
    falseCase.link(&m_jit);
    m_jit.move(TrustedImm32(ValueFalse), resultGPR);
    done.link(&m_jit);

    jsValueResult(resultGPR, m_currentNode, DataFormatJSBoolean);
}

void SpeculativeJIT::compileObjectToObjectOrOtherEquality(Edge leftChild, Edge rightChild)
{
    SpeculateCellOperand op1(this, leftChild);
    JSValueOperand op2(this, rightChild, ManualOperandSpeculation);
    GPRTemporary result(this);
    
    GPRReg op1GPR = op1.gpr();
    GPRReg op2GPR = op2.gpr();
    GPRReg resultGPR = result.gpr();
    GPRTemporary structure;
    GPRReg structureGPR = InvalidGPRReg;

    bool masqueradesAsUndefinedWatchpointValid = m_jit.graph().globalObjectFor(m_currentNode->codeOrigin)->masqueradesAsUndefinedWatchpoint()->isStillValid();

    if (!masqueradesAsUndefinedWatchpointValid) {
        // The masquerades as undefined case will use the structure register, so allocate it here.
        // Do this at the top of the function to avoid branching around a register allocation.
        GPRTemporary realStructure(this);
        structure.adopt(realStructure);
        structureGPR = structure.gpr();
    }

    if (masqueradesAsUndefinedWatchpointValid) {
        m_jit.graph().globalObjectFor(m_currentNode->codeOrigin)->masqueradesAsUndefinedWatchpoint()->add(speculationWatchpoint());
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op1GPR), leftChild, SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op1GPR, JSCell::structureOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
    } else {
        m_jit.loadPtr(MacroAssembler::Address(op1GPR, JSCell::structureOffset()), structureGPR);
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op1GPR), leftChild, SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal,
                structureGPR,
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
        speculationCheck(BadType, JSValueSource::unboxedCell(op1GPR), leftChild,
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(structureGPR, Structure::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));
    }
    
    // It seems that most of the time when programs do a == b where b may be either null/undefined
    // or an object, b is usually an object. Balance the branches to make that case fast.
    MacroAssembler::Jump rightNotCell =
        m_jit.branchTest64(MacroAssembler::NonZero, op2GPR, GPRInfo::tagMaskRegister);
    
    // We know that within this branch, rightChild must be a cell. 
    if (masqueradesAsUndefinedWatchpointValid) { 
        m_jit.graph().globalObjectFor(m_currentNode->codeOrigin)->masqueradesAsUndefinedWatchpoint()->add(speculationWatchpoint());
        DFG_TYPE_CHECK(
            JSValueRegs(op2GPR), rightChild, (~SpecCell) | SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op2GPR, JSCell::structureOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
    } else {
        m_jit.loadPtr(MacroAssembler::Address(op2GPR, JSCell::structureOffset()), structureGPR);
        DFG_TYPE_CHECK(
            JSValueRegs(op2GPR), rightChild, (~SpecCell) | SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal,
                structureGPR,
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
        speculationCheck(BadType, JSValueRegs(op2GPR), rightChild,
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(structureGPR, Structure::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));
    }
    
    // At this point we know that we can perform a straight-forward equality comparison on pointer
    // values because both left and right are pointers to objects that have no special equality
    // protocols.
    MacroAssembler::Jump falseCase = m_jit.branch64(MacroAssembler::NotEqual, op1GPR, op2GPR);
    MacroAssembler::Jump trueCase = m_jit.jump();
    
    rightNotCell.link(&m_jit);
    
    // We know that within this branch, rightChild must not be a cell. Check if that is enough to
    // prove that it is either null or undefined.
    if (needsTypeCheck(rightChild, SpecCell | SpecOther)) {
        m_jit.move(op2GPR, resultGPR);
        m_jit.and64(MacroAssembler::TrustedImm32(~TagBitUndefined), resultGPR);
        
        typeCheck(
            JSValueRegs(op2GPR), rightChild, SpecCell | SpecOther,
            m_jit.branch64(
                MacroAssembler::NotEqual, resultGPR,
                MacroAssembler::TrustedImm64(ValueNull)));
    }
    
    falseCase.link(&m_jit);
    m_jit.move(TrustedImm32(ValueFalse), resultGPR);
    MacroAssembler::Jump done = m_jit.jump();
    trueCase.link(&m_jit);
    m_jit.move(TrustedImm32(ValueTrue), resultGPR);
    done.link(&m_jit);
    
    jsValueResult(resultGPR, m_currentNode, DataFormatJSBoolean);
}

void SpeculativeJIT::compilePeepHoleObjectToObjectOrOtherEquality(Edge leftChild, Edge rightChild, Node* branchNode)
{
    BlockIndex taken = branchNode->takenBlockIndex();
    BlockIndex notTaken = branchNode->notTakenBlockIndex();
    
    SpeculateCellOperand op1(this, leftChild);
    JSValueOperand op2(this, rightChild, ManualOperandSpeculation);
    GPRTemporary result(this);
    
    GPRReg op1GPR = op1.gpr();
    GPRReg op2GPR = op2.gpr();
    GPRReg resultGPR = result.gpr();
    GPRTemporary structure;
    GPRReg structureGPR = InvalidGPRReg;
    
    bool masqueradesAsUndefinedWatchpointValid = m_jit.graph().globalObjectFor(m_currentNode->codeOrigin)->masqueradesAsUndefinedWatchpoint()->isStillValid();

    if (!masqueradesAsUndefinedWatchpointValid) {
        // The masquerades as undefined case will use the structure register, so allocate it here.
        // Do this at the top of the function to avoid branching around a register allocation.
        GPRTemporary realStructure(this);
        structure.adopt(realStructure);
        structureGPR = structure.gpr();
    }

    if (masqueradesAsUndefinedWatchpointValid) {
        m_jit.graph().globalObjectFor(m_currentNode->codeOrigin)->masqueradesAsUndefinedWatchpoint()->add(speculationWatchpoint());
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op1GPR), leftChild, SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op1GPR, JSCell::structureOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
    } else {
        m_jit.loadPtr(MacroAssembler::Address(op1GPR, JSCell::structureOffset()), structureGPR);
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op1GPR), leftChild, SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal, 
                structureGPR, 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
        speculationCheck(BadType, JSValueSource::unboxedCell(op1GPR), leftChild, 
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(structureGPR, Structure::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));
    }

    // It seems that most of the time when programs do a == b where b may be either null/undefined
    // or an object, b is usually an object. Balance the branches to make that case fast.
    MacroAssembler::Jump rightNotCell =
        m_jit.branchTest64(MacroAssembler::NonZero, op2GPR, GPRInfo::tagMaskRegister);
    
    // We know that within this branch, rightChild must be a cell. 
    if (masqueradesAsUndefinedWatchpointValid) {
        m_jit.graph().globalObjectFor(m_currentNode->codeOrigin)->masqueradesAsUndefinedWatchpoint()->add(speculationWatchpoint());
        DFG_TYPE_CHECK(
            JSValueRegs(op2GPR), rightChild, (~SpecCell) | SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op2GPR, JSCell::structureOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
    } else {
        m_jit.loadPtr(MacroAssembler::Address(op2GPR, JSCell::structureOffset()), structureGPR);
        DFG_TYPE_CHECK(
            JSValueRegs(op2GPR), rightChild, (~SpecCell) | SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal, 
                structureGPR, 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
        speculationCheck(BadType, JSValueRegs(op2GPR), rightChild,
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(structureGPR, Structure::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));
    }
    
    // At this point we know that we can perform a straight-forward equality comparison on pointer
    // values because both left and right are pointers to objects that have no special equality
    // protocols.
    branch64(MacroAssembler::Equal, op1GPR, op2GPR, taken);
    
    // We know that within this branch, rightChild must not be a cell. Check if that is enough to
    // prove that it is either null or undefined.
    if (!needsTypeCheck(rightChild, SpecCell | SpecOther))
        rightNotCell.link(&m_jit);
    else {
        jump(notTaken, ForceJump);
        
        rightNotCell.link(&m_jit);
        m_jit.move(op2GPR, resultGPR);
        m_jit.and64(MacroAssembler::TrustedImm32(~TagBitUndefined), resultGPR);
        
        typeCheck(
            JSValueRegs(op2GPR), rightChild, SpecCell | SpecOther, m_jit.branch64(
                MacroAssembler::NotEqual, resultGPR,
                MacroAssembler::TrustedImm64(ValueNull)));
    }
    
    jump(notTaken);
}

void SpeculativeJIT::compileIntegerCompare(Node* node, MacroAssembler::RelationalCondition condition)
{
    SpeculateIntegerOperand op1(this, node->child1());
    SpeculateIntegerOperand op2(this, node->child2());
    GPRTemporary result(this, op1, op2);
    
    m_jit.compare32(condition, op1.gpr(), op2.gpr(), result.gpr());
    
    // If we add a DataFormatBool, we should use it here.
    m_jit.or32(TrustedImm32(ValueFalse), result.gpr());
    jsValueResult(result.gpr(), m_currentNode, DataFormatJSBoolean);
}

void SpeculativeJIT::compileDoubleCompare(Node* node, MacroAssembler::DoubleCondition condition)
{
    SpeculateDoubleOperand op1(this, node->child1());
    SpeculateDoubleOperand op2(this, node->child2());
    GPRTemporary result(this);
    
    m_jit.move(TrustedImm32(ValueTrue), result.gpr());
    MacroAssembler::Jump trueCase = m_jit.branchDouble(condition, op1.fpr(), op2.fpr());
    m_jit.xor64(TrustedImm32(true), result.gpr());
    trueCase.link(&m_jit);
    
    jsValueResult(result.gpr(), node, DataFormatJSBoolean);
}

void SpeculativeJIT::compileValueAdd(Node* node)
{
    JSValueOperand op1(this, node->child1());
    JSValueOperand op2(this, node->child2());
    
    GPRReg op1GPR = op1.gpr();
    GPRReg op2GPR = op2.gpr();
    
    flushRegisters();
    
    GPRResult result(this);
    if (isKnownNotNumber(node->child1().node()) || isKnownNotNumber(node->child2().node()))
        callOperation(operationValueAddNotNumber, result.gpr(), op1GPR, op2GPR);
    else
        callOperation(operationValueAdd, result.gpr(), op1GPR, op2GPR);
    
    jsValueResult(result.gpr(), node);
}

void SpeculativeJIT::compileObjectOrOtherLogicalNot(Edge nodeUse)
{
    JSValueOperand value(this, nodeUse, ManualOperandSpeculation);
    GPRTemporary result(this);
    GPRReg valueGPR = value.gpr();
    GPRReg resultGPR = result.gpr();
    GPRTemporary structure;
    GPRReg structureGPR = InvalidGPRReg;

    bool masqueradesAsUndefinedWatchpointValid = m_jit.graph().globalObjectFor(m_currentNode->codeOrigin)->masqueradesAsUndefinedWatchpoint()->isStillValid();

    if (!masqueradesAsUndefinedWatchpointValid) {
        // The masquerades as undefined case will use the structure register, so allocate it here.
        // Do this at the top of the function to avoid branching around a register allocation.
        GPRTemporary realStructure(this);
        structure.adopt(realStructure);
        structureGPR = structure.gpr();
    }

    MacroAssembler::Jump notCell = m_jit.branchTest64(MacroAssembler::NonZero, valueGPR, GPRInfo::tagMaskRegister);
    if (masqueradesAsUndefinedWatchpointValid) {
        m_jit.graph().globalObjectFor(m_currentNode->codeOrigin)->masqueradesAsUndefinedWatchpoint()->add(speculationWatchpoint());
        DFG_TYPE_CHECK(
            JSValueRegs(valueGPR), nodeUse, (~SpecCell) | SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal,
                MacroAssembler::Address(valueGPR, JSCell::structureOffset()),
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
    } else {
        m_jit.loadPtr(MacroAssembler::Address(valueGPR, JSCell::structureOffset()), structureGPR);

        DFG_TYPE_CHECK(
            JSValueRegs(valueGPR), nodeUse, (~SpecCell) | SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal,
                structureGPR,
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));

        MacroAssembler::Jump isNotMasqueradesAsUndefined = 
            m_jit.branchTest8(
                MacroAssembler::Zero, 
                MacroAssembler::Address(structureGPR, Structure::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined));

        speculationCheck(BadType, JSValueRegs(valueGPR), nodeUse, 
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(structureGPR, Structure::globalObjectOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.graph().globalObjectFor(m_currentNode->codeOrigin))));

        isNotMasqueradesAsUndefined.link(&m_jit);
    }
    m_jit.move(TrustedImm32(ValueFalse), resultGPR);
    MacroAssembler::Jump done = m_jit.jump();
    
    notCell.link(&m_jit);

    if (needsTypeCheck(nodeUse, SpecCell | SpecOther)) {
        m_jit.move(valueGPR, resultGPR);
        m_jit.and64(MacroAssembler::TrustedImm32(~TagBitUndefined), resultGPR);
        typeCheck(
            JSValueRegs(valueGPR), nodeUse, SpecCell | SpecOther, m_jit.branch64(
                MacroAssembler::NotEqual, 
                resultGPR, 
                MacroAssembler::TrustedImm64(ValueNull)));
    }
    m_jit.move(TrustedImm32(ValueTrue), resultGPR);
    
    done.link(&m_jit);
    
    jsValueResult(resultGPR, m_currentNode, DataFormatJSBoolean);
}

void SpeculativeJIT::compileLogicalNot(Node* node)
{
    switch (node->child1().useKind()) {
    case ObjectOrOtherUse: {
        compileObjectOrOtherLogicalNot(node->child1());
        return;
    }
        
    case Int32Use: {
        SpeculateIntegerOperand value(this, node->child1());
        GPRTemporary result(this, value);
        m_jit.compare32(MacroAssembler::Equal, value.gpr(), MacroAssembler::TrustedImm32(0), result.gpr());
        m_jit.or32(TrustedImm32(ValueFalse), result.gpr());
        jsValueResult(result.gpr(), node, DataFormatJSBoolean);
        return;
    }
        
    case NumberUse: {
        SpeculateDoubleOperand value(this, node->child1());
        FPRTemporary scratch(this);
        GPRTemporary result(this);
        m_jit.move(TrustedImm32(ValueFalse), result.gpr());
        MacroAssembler::Jump nonZero = m_jit.branchDoubleNonZero(value.fpr(), scratch.fpr());
        m_jit.xor32(TrustedImm32(true), result.gpr());
        nonZero.link(&m_jit);
        jsValueResult(result.gpr(), node, DataFormatJSBoolean);
        return;
    }
    
    case BooleanUse: {
        if (!needsTypeCheck(node->child1(), SpecBoolean)) {
            SpeculateBooleanOperand value(this, node->child1());
            GPRTemporary result(this, value);
            
            m_jit.move(value.gpr(), result.gpr());
            m_jit.xor64(TrustedImm32(true), result.gpr());
            
            jsValueResult(result.gpr(), node, DataFormatJSBoolean);
            return;
        }
        
        JSValueOperand value(this, node->child1(), ManualOperandSpeculation);
        GPRTemporary result(this); // FIXME: We could reuse, but on speculation fail would need recovery to restore tag (akin to add).
        
        m_jit.move(value.gpr(), result.gpr());
        m_jit.xor64(TrustedImm32(static_cast<int32_t>(ValueFalse)), result.gpr());
        typeCheck(
            JSValueRegs(value.gpr()), node->child1(), SpecBoolean, m_jit.branchTest64(
                JITCompiler::NonZero, result.gpr(), TrustedImm32(static_cast<int32_t>(~1))));
        m_jit.xor64(TrustedImm32(static_cast<int32_t>(ValueTrue)), result.gpr());
        
        // If we add a DataFormatBool, we should use it here.
        jsValueResult(result.gpr(), node, DataFormatJSBoolean);
        return;
    }
        
    case UntypedUse: {
        JSValueOperand arg1(this, node->child1());
        GPRTemporary result(this);
    
        GPRReg arg1GPR = arg1.gpr();
        GPRReg resultGPR = result.gpr();
    
        arg1.use();
    
        m_jit.move(arg1GPR, resultGPR);
        m_jit.xor64(TrustedImm32(static_cast<int32_t>(ValueFalse)), resultGPR);
        JITCompiler::Jump slowCase = m_jit.branchTest64(JITCompiler::NonZero, resultGPR, TrustedImm32(static_cast<int32_t>(~1)));
    
        addSlowPathGenerator(
            slowPathCall(slowCase, this, dfgConvertJSValueToBoolean, resultGPR, arg1GPR));
    
        m_jit.xor64(TrustedImm32(static_cast<int32_t>(ValueTrue)), resultGPR);
        jsValueResult(resultGPR, node, DataFormatJSBoolean, UseChildrenCalledExplicitly);
        return;
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
}

void SpeculativeJIT::emitObjectOrOtherBranch(Edge nodeUse, BlockIndex taken, BlockIndex notTaken)
{
    JSValueOperand value(this, nodeUse, ManualOperandSpeculation);
    GPRTemporary scratch(this);
    GPRReg valueGPR = value.gpr();
    GPRReg scratchGPR = scratch.gpr();
    
    MacroAssembler::Jump notCell = m_jit.branchTest64(MacroAssembler::NonZero, valueGPR, GPRInfo::tagMaskRegister);
    if (m_jit.graph().globalObjectFor(m_currentNode->codeOrigin)->masqueradesAsUndefinedWatchpoint()->isStillValid()) {
        m_jit.graph().globalObjectFor(m_currentNode->codeOrigin)->masqueradesAsUndefinedWatchpoint()->add(speculationWatchpoint());

        DFG_TYPE_CHECK(
            JSValueRegs(valueGPR), nodeUse, (~SpecCell) | SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(valueGPR, JSCell::structureOffset()),
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
    } else {
        m_jit.loadPtr(MacroAssembler::Address(valueGPR, JSCell::structureOffset()), scratchGPR);

        DFG_TYPE_CHECK(
            JSValueRegs(valueGPR), nodeUse, (~SpecCell) | SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal, 
                scratchGPR,
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));

        JITCompiler::Jump isNotMasqueradesAsUndefined = m_jit.branchTest8(JITCompiler::Zero, MacroAssembler::Address(scratchGPR, Structure::typeInfoFlagsOffset()), TrustedImm32(MasqueradesAsUndefined));

        speculationCheck(BadType, JSValueRegs(valueGPR), nodeUse,
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(scratchGPR, Structure::globalObjectOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.graph().globalObjectFor(m_currentNode->codeOrigin))));

        isNotMasqueradesAsUndefined.link(&m_jit);
    }
    jump(taken, ForceJump);
    
    notCell.link(&m_jit);
    
    if (needsTypeCheck(nodeUse, SpecCell | SpecOther)) {
        m_jit.move(valueGPR, scratchGPR);
        m_jit.and64(MacroAssembler::TrustedImm32(~TagBitUndefined), scratchGPR);
        typeCheck(
            JSValueRegs(valueGPR), nodeUse, SpecCell | SpecOther, m_jit.branch64(
                MacroAssembler::NotEqual, scratchGPR, MacroAssembler::TrustedImm64(ValueNull)));
    }
    jump(notTaken);
    
    noResult(m_currentNode);
}

void SpeculativeJIT::emitBranch(Node* node)
{
    BlockIndex taken = node->takenBlockIndex();
    BlockIndex notTaken = node->notTakenBlockIndex();
    
    switch (node->child1().useKind()) {
    case ObjectOrOtherUse: {
        emitObjectOrOtherBranch(node->child1(), taken, notTaken);
        return;
    }
        
    case Int32Use:
    case NumberUse: {
        if (node->child1().useKind() == Int32Use) {
            bool invert = false;
            
            if (taken == nextBlock()) {
                invert = true;
                BlockIndex tmp = taken;
                taken = notTaken;
                notTaken = tmp;
            }

            SpeculateIntegerOperand value(this, node->child1());
            branchTest32(invert ? MacroAssembler::Zero : MacroAssembler::NonZero, value.gpr(), taken);
        } else {
            SpeculateDoubleOperand value(this, node->child1());
            FPRTemporary scratch(this);
            branchDoubleNonZero(value.fpr(), scratch.fpr(), taken);
        }
        
        jump(notTaken);
        
        noResult(node);
        return;
    }

    case UntypedUse:
    case BooleanUse: {
        JSValueOperand value(this, node->child1(), ManualOperandSpeculation);
        GPRReg valueGPR = value.gpr();
        
        if (node->child1().useKind() == BooleanUse) {
            if (!needsTypeCheck(node->child1(), SpecBoolean)) {
                MacroAssembler::ResultCondition condition = MacroAssembler::NonZero;
                
                if (taken == nextBlock()) {
                    condition = MacroAssembler::Zero;
                    BlockIndex tmp = taken;
                    taken = notTaken;
                    notTaken = tmp;
                }
                
                branchTest32(condition, valueGPR, TrustedImm32(true), taken);
                jump(notTaken);
            } else {
                branch64(MacroAssembler::Equal, valueGPR, MacroAssembler::TrustedImm64(JSValue::encode(jsBoolean(false))), notTaken);
                branch64(MacroAssembler::Equal, valueGPR, MacroAssembler::TrustedImm64(JSValue::encode(jsBoolean(true))), taken);
                
                typeCheck(JSValueRegs(valueGPR), node->child1(), SpecBoolean, m_jit.jump());
            }
            value.use();
        } else {
            GPRTemporary result(this);
            GPRReg resultGPR = result.gpr();
            
            if (node->child1()->prediction() & SpecInt32) {
                branch64(MacroAssembler::Equal, valueGPR, MacroAssembler::TrustedImm64(JSValue::encode(jsNumber(0))), notTaken);
                branch64(MacroAssembler::AboveOrEqual, valueGPR, GPRInfo::tagTypeNumberRegister, taken);
            }
    
            if (node->child1()->prediction() & SpecBoolean) {
                branch64(MacroAssembler::Equal, valueGPR, MacroAssembler::TrustedImm64(JSValue::encode(jsBoolean(false))), notTaken);
                branch64(MacroAssembler::Equal, valueGPR, MacroAssembler::TrustedImm64(JSValue::encode(jsBoolean(true))), taken);
            }
    
            value.use();
    
            silentSpillAllRegisters(resultGPR);
            callOperation(dfgConvertJSValueToBoolean, resultGPR, valueGPR);
            silentFillAllRegisters(resultGPR);
    
            branchTest32(MacroAssembler::NonZero, resultGPR, taken);
            jump(notTaken);
        }
        
        noResult(node, UseChildrenCalledExplicitly);
        return;
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}

void SpeculativeJIT::compile(Node* node)
{
    NodeType op = node->op();
    
#if ENABLE(DFG_REGISTER_ALLOCATION_VALIDATION)
    m_jit.clearRegisterAllocationOffsets();
#endif

    switch (op) {
    case JSConstant:
        initConstantInfo(node);
        break;

    case PhantomArguments:
        initConstantInfo(node);
        break;

    case WeakJSConstant:
        m_jit.addWeakReference(node->weakConstant());
        initConstantInfo(node);
        break;
        
    case Identity: {
        // CSE should always eliminate this.
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }

    case GetLocal: {
        SpeculatedType prediction = node->variableAccessData()->prediction();
        AbstractValue& value = m_state.variables().operand(node->local());

        // If we have no prediction for this local, then don't attempt to compile.
        if (prediction == SpecNone) {
            terminateSpeculativeExecution(InadequateCoverage, JSValueRegs(), 0);
            break;
        }
        
        // If the CFA is tracking this variable and it found that the variable
        // cannot have been assigned, then don't attempt to proceed.
        if (value.isClear()) {
            // FIXME: We should trap instead.
            // https://bugs.webkit.org/show_bug.cgi?id=110383
            terminateSpeculativeExecution(InadequateCoverage, JSValueRegs(), 0);
            break;
        }
        
        if (node->variableAccessData()->shouldUseDoubleFormat()) {
            FPRTemporary result(this);
            m_jit.loadDouble(JITCompiler::addressFor(node->local()), result.fpr());
            VirtualRegister virtualRegister = node->virtualRegister();
            m_fprs.retain(result.fpr(), virtualRegister, SpillOrderDouble);
            m_generationInfo[virtualRegister].initDouble(node, node->refCount(), result.fpr());
            break;
        }
        
        if (isInt32Speculation(value.m_type)) {
            GPRTemporary result(this);
            m_jit.load32(JITCompiler::payloadFor(node->local()), result.gpr());
            
            // Like integerResult, but don't useChildren - our children are phi nodes,
            // and don't represent values within this dataflow with virtual registers.
            VirtualRegister virtualRegister = node->virtualRegister();
            m_gprs.retain(result.gpr(), virtualRegister, SpillOrderInteger);
            m_generationInfo[virtualRegister].initInteger(node, node->refCount(), result.gpr());
            break;
        }

        GPRTemporary result(this);
        m_jit.load64(JITCompiler::addressFor(node->local()), result.gpr());

        // Like jsValueResult, but don't useChildren - our children are phi nodes,
        // and don't represent values within this dataflow with virtual registers.
        VirtualRegister virtualRegister = node->virtualRegister();
        m_gprs.retain(result.gpr(), virtualRegister, SpillOrderJS);

        DataFormat format;
        if (isCellSpeculation(value.m_type))
            format = DataFormatJSCell;
        else if (isBooleanSpeculation(value.m_type))
            format = DataFormatJSBoolean;
        else
            format = DataFormatJS;

        m_generationInfo[virtualRegister].initJSValue(node, node->refCount(), result.gpr(), format);
        break;
    }

    case GetLocalUnlinked: {
        GPRTemporary result(this);
        
        m_jit.load64(JITCompiler::addressFor(node->unlinkedLocal()), result.gpr());
        
        jsValueResult(result.gpr(), node);
        break;
    }
        
    case MovHintAndCheck: {
        compileMovHintAndCheck(node);
        break;
    }
        
    case InlineStart: {
        compileInlineStart(node);
        break;
    }

    case MovHint:
    case ZombieHint: {
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }

    case SetLocal: {
        // SetLocal doubles as a hint as to where a node will be stored and
        // as a speculation point. So before we speculate make sure that we
        // know where the child of this node needs to go in the virtual
        // stack.
        compileMovHint(node);
        
        if (node->variableAccessData()->shouldUnboxIfPossible()) {
            if (node->variableAccessData()->shouldUseDoubleFormat()) {
                SpeculateDoubleOperand value(this, node->child1());
                m_jit.storeDouble(value.fpr(), JITCompiler::addressFor(node->local()));
                noResult(node);
                // Indicate that it's no longer necessary to retrieve the value of
                // this bytecode variable from registers or other locations in the stack,
                // but that it is stored as a double.
                recordSetLocal(node->local(), ValueSource(DoubleInJSStack));
                break;
            }
        
            SpeculatedType predictedType = node->variableAccessData()->argumentAwarePrediction();
            if (isInt32Speculation(predictedType)) {
                SpeculateIntegerOperand value(this, node->child1());
                m_jit.store32(value.gpr(), JITCompiler::payloadFor(node->local()));
                noResult(node);
                recordSetLocal(node->local(), ValueSource(Int32InJSStack));
                break;
            }
            if (isCellSpeculation(predictedType)) {
                SpeculateCellOperand cell(this, node->child1());
                GPRReg cellGPR = cell.gpr();
                m_jit.store64(cellGPR, JITCompiler::addressFor(node->local()));
                noResult(node);
                recordSetLocal(node->local(), ValueSource(CellInJSStack));
                break;
            }
            if (isBooleanSpeculation(predictedType)) {
                SpeculateBooleanOperand boolean(this, node->child1());
                m_jit.store64(boolean.gpr(), JITCompiler::addressFor(node->local()));
                noResult(node);
                recordSetLocal(node->local(), ValueSource(BooleanInJSStack));
                break;
            }
        }
        
        JSValueOperand value(this, node->child1());
        m_jit.store64(value.gpr(), JITCompiler::addressFor(node->local()));
        noResult(node);

        recordSetLocal(node->local(), ValueSource(ValueInJSStack));

        // If we're storing an arguments object that has been optimized away,
        // our variable event stream for OSR exit now reflects the optimized
        // value (JSValue()). On the slow path, we want an arguments object
        // instead. We add an additional move hint to show OSR exit that it
        // needs to reconstruct the arguments object.
        if (node->child1()->op() == PhantomArguments)
            compileMovHint(node);

        break;
    }

    case SetArgument:
        // This is a no-op; it just marks the fact that the argument is being used.
        // But it may be profitable to use this as a hook to run speculation checks
        // on arguments, thereby allowing us to trivially eliminate such checks if
        // the argument is not used.
        break;

    case BitAnd:
    case BitOr:
    case BitXor:
        if (isInt32Constant(node->child1().node())) {
            SpeculateIntegerOperand op2(this, node->child2());
            GPRTemporary result(this, op2);

            bitOp(op, valueOfInt32Constant(node->child1().node()), op2.gpr(), result.gpr());

            integerResult(result.gpr(), node);
        } else if (isInt32Constant(node->child2().node())) {
            SpeculateIntegerOperand op1(this, node->child1());
            GPRTemporary result(this, op1);

            bitOp(op, valueOfInt32Constant(node->child2().node()), op1.gpr(), result.gpr());

            integerResult(result.gpr(), node);
        } else {
            SpeculateIntegerOperand op1(this, node->child1());
            SpeculateIntegerOperand op2(this, node->child2());
            GPRTemporary result(this, op1, op2);

            GPRReg reg1 = op1.gpr();
            GPRReg reg2 = op2.gpr();
            bitOp(op, reg1, reg2, result.gpr());

            integerResult(result.gpr(), node);
        }
        break;

    case BitRShift:
    case BitLShift:
    case BitURShift:
        if (isInt32Constant(node->child2().node())) {
            SpeculateIntegerOperand op1(this, node->child1());
            GPRTemporary result(this, op1);

            shiftOp(op, op1.gpr(), valueOfInt32Constant(node->child2().node()) & 0x1f, result.gpr());

            integerResult(result.gpr(), node);
        } else {
            // Do not allow shift amount to be used as the result, MacroAssembler does not permit this.
            SpeculateIntegerOperand op1(this, node->child1());
            SpeculateIntegerOperand op2(this, node->child2());
            GPRTemporary result(this, op1);

            GPRReg reg1 = op1.gpr();
            GPRReg reg2 = op2.gpr();
            shiftOp(op, reg1, reg2, result.gpr());

            integerResult(result.gpr(), node);
        }
        break;

    case UInt32ToNumber: {
        compileUInt32ToNumber(node);
        break;
    }

    case DoubleAsInt32: {
        compileDoubleAsInt32(node);
        break;
    }

    case ValueToInt32: {
        compileValueToInt32(node);
        break;
    }
        
    case Int32ToDouble:
    case ForwardInt32ToDouble: {
        compileInt32ToDouble(node);
        break;
    }
        
    case ValueAdd:
    case ArithAdd:
        compileAdd(node);
        break;
        
    case MakeRope:
        compileMakeRope(node);
        break;

    case ArithSub:
        compileArithSub(node);
        break;

    case ArithNegate:
        compileArithNegate(node);
        break;

    case ArithMul:
        compileArithMul(node);
        break;

    case ArithIMul:
        compileArithIMul(node);
        break;

    case ArithDiv: {
        switch (node->binaryUseKind()) {
        case Int32Use: {
            compileIntegerArithDivForX86(node);
            break;
        }
            
        case NumberUse: {
            SpeculateDoubleOperand op1(this, node->child1());
            SpeculateDoubleOperand op2(this, node->child2());
            FPRTemporary result(this, op1);
            
            FPRReg reg1 = op1.fpr();
            FPRReg reg2 = op2.fpr();
            m_jit.divDouble(reg1, reg2, result.fpr());
            
            doubleResult(result.fpr(), node);
            break;
        }
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        break;
    }

    case ArithMod: {
        compileArithMod(node);
        break;
    }

    case ArithAbs: {
        switch (node->child1().useKind()) {
        case Int32Use: {
            SpeculateIntegerOperand op1(this, node->child1());
            GPRTemporary result(this);
            GPRTemporary scratch(this);
            
            m_jit.zeroExtend32ToPtr(op1.gpr(), result.gpr());
            m_jit.rshift32(result.gpr(), MacroAssembler::TrustedImm32(31), scratch.gpr());
            m_jit.add32(scratch.gpr(), result.gpr());
            m_jit.xor32(scratch.gpr(), result.gpr());
            speculationCheck(Overflow, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::Equal, result.gpr(), MacroAssembler::TrustedImm32(1 << 31)));
            integerResult(result.gpr(), node);
            break;
        }
        
        case NumberUse: {
            SpeculateDoubleOperand op1(this, node->child1());
            FPRTemporary result(this);
            
            m_jit.absDouble(op1.fpr(), result.fpr());
            doubleResult(result.fpr(), node);
            break;
        }
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        break;
    }
        
    case ArithMin:
    case ArithMax: {
        switch (node->binaryUseKind()) {
        case Int32Use: {
            SpeculateStrictInt32Operand op1(this, node->child1());
            SpeculateStrictInt32Operand op2(this, node->child2());
            GPRTemporary result(this, op1);
            
            MacroAssembler::Jump op1Less = m_jit.branch32(op == ArithMin ? MacroAssembler::LessThan : MacroAssembler::GreaterThan, op1.gpr(), op2.gpr());
            m_jit.move(op2.gpr(), result.gpr());
            if (op1.gpr() != result.gpr()) {
                MacroAssembler::Jump done = m_jit.jump();
                op1Less.link(&m_jit);
                m_jit.move(op1.gpr(), result.gpr());
                done.link(&m_jit);
            } else
                op1Less.link(&m_jit);
            
            integerResult(result.gpr(), node);
            break;
        }
        
        case NumberUse: {
            SpeculateDoubleOperand op1(this, node->child1());
            SpeculateDoubleOperand op2(this, node->child2());
            FPRTemporary result(this, op1);
            
            FPRReg op1FPR = op1.fpr();
            FPRReg op2FPR = op2.fpr();
            FPRReg resultFPR = result.fpr();
        
            MacroAssembler::JumpList done;
        
            MacroAssembler::Jump op1Less = m_jit.branchDouble(op == ArithMin ? MacroAssembler::DoubleLessThan : MacroAssembler::DoubleGreaterThan, op1FPR, op2FPR);
        
            // op2 is eather the lesser one or one of then is NaN
            MacroAssembler::Jump op2Less = m_jit.branchDouble(op == ArithMin ? MacroAssembler::DoubleGreaterThanOrEqual : MacroAssembler::DoubleLessThanOrEqual, op1FPR, op2FPR);
        
            // Unordered case. We don't know which of op1, op2 is NaN. Manufacture NaN by adding 
            // op1 + op2 and putting it into result.
            m_jit.addDouble(op1FPR, op2FPR, resultFPR);
            done.append(m_jit.jump());
        
            op2Less.link(&m_jit);
            m_jit.moveDouble(op2FPR, resultFPR);
        
            if (op1FPR != resultFPR) {
                done.append(m_jit.jump());
            
                op1Less.link(&m_jit);
                m_jit.moveDouble(op1FPR, resultFPR);
            } else
                op1Less.link(&m_jit);
        
            done.link(&m_jit);
        
            doubleResult(resultFPR, node);
            break;
        }
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        break;
    }
        
    case ArithSqrt: {
        SpeculateDoubleOperand op1(this, node->child1());
        FPRTemporary result(this, op1);
        
        m_jit.sqrtDouble(op1.fpr(), result.fpr());
        
        doubleResult(result.fpr(), node);
        break;
    }

    case LogicalNot:
        compileLogicalNot(node);
        break;

    case CompareLess:
        if (compare(node, JITCompiler::LessThan, JITCompiler::DoubleLessThan, operationCompareLess))
            return;
        break;

    case CompareLessEq:
        if (compare(node, JITCompiler::LessThanOrEqual, JITCompiler::DoubleLessThanOrEqual, operationCompareLessEq))
            return;
        break;

    case CompareGreater:
        if (compare(node, JITCompiler::GreaterThan, JITCompiler::DoubleGreaterThan, operationCompareGreater))
            return;
        break;

    case CompareGreaterEq:
        if (compare(node, JITCompiler::GreaterThanOrEqual, JITCompiler::DoubleGreaterThanOrEqual, operationCompareGreaterEq))
            return;
        break;
        
    case CompareEqConstant:
        ASSERT(isNullConstant(node->child2().node()));
        if (nonSpeculativeCompareNull(node, node->child1()))
            return;
        break;

    case CompareEq:
        if (compare(node, JITCompiler::Equal, JITCompiler::DoubleEqual, operationCompareEq))
            return;
        break;

    case CompareStrictEqConstant:
        if (compileStrictEqForConstant(node, node->child1(), valueOfJSConstant(node->child2().node())))
            return;
        break;

    case CompareStrictEq:
        if (compileStrictEq(node))
            return;
        break;

    case StringCharCodeAt: {
        compileGetCharCodeAt(node);
        break;
    }

    case StringCharAt: {
        // Relies on StringCharAt node having same basic layout as GetByVal
        compileGetByValOnString(node);
        break;
    }

    case StringFromCharCode: {
        compileFromCharCode(node);
        break;
    }
        
    case CheckArray: {
        checkArray(node);
        break;
    }
        
    case Arrayify:
    case ArrayifyToStructure: {
        arrayify(node);
        break;
    }

    case GetByVal: {
        switch (node->arrayMode().type()) {
        case Array::SelectUsingPredictions:
        case Array::ForceExit:
            RELEASE_ASSERT_NOT_REACHED();
            terminateSpeculativeExecution(InadequateCoverage, JSValueRegs(), 0);
            break;
        case Array::Generic: {
            JSValueOperand base(this, node->child1());
            JSValueOperand property(this, node->child2());
            GPRReg baseGPR = base.gpr();
            GPRReg propertyGPR = property.gpr();
            
            flushRegisters();
            GPRResult result(this);
            callOperation(operationGetByVal, result.gpr(), baseGPR, propertyGPR);
            
            jsValueResult(result.gpr(), node);
            break;
        }
        case Array::Int32:
        case Array::Contiguous: {
            if (node->arrayMode().isInBounds()) {
                SpeculateStrictInt32Operand property(this, node->child2());
                StorageOperand storage(this, node->child3());
                
                GPRReg propertyReg = property.gpr();
                GPRReg storageReg = storage.gpr();
                
                if (!m_compileOkay)
                    return;
                
                speculationCheck(OutOfBounds, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength())));
                
                GPRTemporary result(this);
                m_jit.load64(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight), result.gpr());
                speculationCheck(LoadFromHole, JSValueRegs(), 0, m_jit.branchTest64(MacroAssembler::Zero, result.gpr()));
                jsValueResult(result.gpr(), node, node->arrayMode().type() == Array::Int32 ? DataFormatJSInteger : DataFormatJS);
                break;
            }
            
            SpeculateCellOperand base(this, node->child1());
            SpeculateStrictInt32Operand property(this, node->child2());
            StorageOperand storage(this, node->child3());
            
            GPRReg baseReg = base.gpr();
            GPRReg propertyReg = property.gpr();
            GPRReg storageReg = storage.gpr();
            
            if (!m_compileOkay)
                return;
            
            GPRTemporary result(this);
            GPRReg resultReg = result.gpr();
            
            MacroAssembler::JumpList slowCases;
            
            slowCases.append(m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength())));
            
            m_jit.load64(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight), resultReg);
            slowCases.append(m_jit.branchTest64(MacroAssembler::Zero, resultReg));
            
            addSlowPathGenerator(
                slowPathCall(
                    slowCases, this, operationGetByValArrayInt,
                    result.gpr(), baseReg, propertyReg));
            
            jsValueResult(resultReg, node);
            break;
        }

        case Array::Double: {
            if (node->arrayMode().isInBounds()) {
                if (node->arrayMode().isSaneChain()) {
                    JSGlobalObject* globalObject = m_jit.globalObjectFor(node->codeOrigin);
                    ASSERT(globalObject->arrayPrototypeChainIsSane());
                    globalObject->arrayPrototype()->structure()->addTransitionWatchpoint(speculationWatchpoint());
                    globalObject->objectPrototype()->structure()->addTransitionWatchpoint(speculationWatchpoint());
                }
                
                SpeculateStrictInt32Operand property(this, node->child2());
                StorageOperand storage(this, node->child3());
            
                GPRReg propertyReg = property.gpr();
                GPRReg storageReg = storage.gpr();
            
                if (!m_compileOkay)
                    return;
            
                speculationCheck(OutOfBounds, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength())));
            
                FPRTemporary result(this);
                m_jit.loadDouble(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight), result.fpr());
                if (!node->arrayMode().isSaneChain())
                    speculationCheck(LoadFromHole, JSValueRegs(), 0, m_jit.branchDouble(MacroAssembler::DoubleNotEqualOrUnordered, result.fpr(), result.fpr()));
                doubleResult(result.fpr(), node);
                break;
            }

            SpeculateCellOperand base(this, node->child1());
            SpeculateStrictInt32Operand property(this, node->child2());
            StorageOperand storage(this, node->child3());
            
            GPRReg baseReg = base.gpr();
            GPRReg propertyReg = property.gpr();
            GPRReg storageReg = storage.gpr();
            
            if (!m_compileOkay)
                return;
            
            GPRTemporary result(this);
            FPRTemporary temp(this);
            GPRReg resultReg = result.gpr();
            FPRReg tempReg = temp.fpr();
            
            MacroAssembler::JumpList slowCases;
            
            slowCases.append(m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength())));
            
            m_jit.loadDouble(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight), tempReg);
            slowCases.append(m_jit.branchDouble(MacroAssembler::DoubleNotEqualOrUnordered, tempReg, tempReg));
            boxDouble(tempReg, resultReg);
            
            addSlowPathGenerator(
                slowPathCall(
                    slowCases, this, operationGetByValArrayInt,
                    result.gpr(), baseReg, propertyReg));
            
            jsValueResult(resultReg, node);
            break;
        }

        case Array::ArrayStorage:
        case Array::SlowPutArrayStorage: {
            if (node->arrayMode().isInBounds()) {
                SpeculateStrictInt32Operand property(this, node->child2());
                StorageOperand storage(this, node->child3());
            
                GPRReg propertyReg = property.gpr();
                GPRReg storageReg = storage.gpr();
            
                if (!m_compileOkay)
                    return;
            
                speculationCheck(OutOfBounds, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, ArrayStorage::vectorLengthOffset())));
            
                GPRTemporary result(this);
                m_jit.load64(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])), result.gpr());
                speculationCheck(LoadFromHole, JSValueRegs(), 0, m_jit.branchTest64(MacroAssembler::Zero, result.gpr()));
            
                jsValueResult(result.gpr(), node);
                break;
            }

            SpeculateCellOperand base(this, node->child1());
            SpeculateStrictInt32Operand property(this, node->child2());
            StorageOperand storage(this, node->child3());
            
            GPRReg baseReg = base.gpr();
            GPRReg propertyReg = property.gpr();
            GPRReg storageReg = storage.gpr();
            
            if (!m_compileOkay)
                return;
            
            GPRTemporary result(this);
            GPRReg resultReg = result.gpr();
            
            MacroAssembler::JumpList slowCases;
            
            slowCases.append(m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, ArrayStorage::vectorLengthOffset())));
    
            m_jit.load64(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])), resultReg);
            slowCases.append(m_jit.branchTest64(MacroAssembler::Zero, resultReg));
    
            addSlowPathGenerator(
                slowPathCall(
                    slowCases, this, operationGetByValArrayInt,
                    result.gpr(), baseReg, propertyReg));
            
            jsValueResult(resultReg, node);
            break;
        }
        case Array::String:
            compileGetByValOnString(node);
            break;
        case Array::Arguments:
            compileGetByValOnArguments(node);
            break;
        case Array::Int8Array:
            compileGetByValOnIntTypedArray(m_jit.vm()->int8ArrayDescriptor(), node, sizeof(int8_t), SignedTypedArray);
            break;
        case Array::Int16Array:
            compileGetByValOnIntTypedArray(m_jit.vm()->int16ArrayDescriptor(), node, sizeof(int16_t), SignedTypedArray);
            break;
        case Array::Int32Array:
            compileGetByValOnIntTypedArray(m_jit.vm()->int32ArrayDescriptor(), node, sizeof(int32_t), SignedTypedArray);
            break;
        case Array::Uint8Array:
            compileGetByValOnIntTypedArray(m_jit.vm()->uint8ArrayDescriptor(), node, sizeof(uint8_t), UnsignedTypedArray);
            break;
        case Array::Uint8ClampedArray:
            compileGetByValOnIntTypedArray(m_jit.vm()->uint8ClampedArrayDescriptor(), node, sizeof(uint8_t), UnsignedTypedArray);
            break;
        case Array::Uint16Array:
            compileGetByValOnIntTypedArray(m_jit.vm()->uint16ArrayDescriptor(), node, sizeof(uint16_t), UnsignedTypedArray);
            break;
        case Array::Uint32Array:
            compileGetByValOnIntTypedArray(m_jit.vm()->uint32ArrayDescriptor(), node, sizeof(uint32_t), UnsignedTypedArray);
            break;
        case Array::Float32Array:
            compileGetByValOnFloatTypedArray(m_jit.vm()->float32ArrayDescriptor(), node, sizeof(float));
            break;
        case Array::Float64Array:
            compileGetByValOnFloatTypedArray(m_jit.vm()->float64ArrayDescriptor(), node, sizeof(double));
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        break;
    }

    case PutByVal:
    case PutByValAlias: {
        Edge child1 = m_jit.graph().varArgChild(node, 0);
        Edge child2 = m_jit.graph().varArgChild(node, 1);
        Edge child3 = m_jit.graph().varArgChild(node, 2);
        Edge child4 = m_jit.graph().varArgChild(node, 3);
        
        ArrayMode arrayMode = node->arrayMode().modeForPut();
        bool alreadyHandled = false;
        
        switch (arrayMode.type()) {
        case Array::SelectUsingPredictions:
        case Array::ForceExit:
            RELEASE_ASSERT_NOT_REACHED();
            terminateSpeculativeExecution(InadequateCoverage, JSValueRegs(), 0);
            alreadyHandled = true;
            break;
        case Array::Generic: {
            RELEASE_ASSERT(node->op() == PutByVal);
            
            JSValueOperand arg1(this, child1);
            JSValueOperand arg2(this, child2);
            JSValueOperand arg3(this, child3);
            GPRReg arg1GPR = arg1.gpr();
            GPRReg arg2GPR = arg2.gpr();
            GPRReg arg3GPR = arg3.gpr();
            flushRegisters();
            
            callOperation(m_jit.strictModeFor(node->codeOrigin) ? operationPutByValStrict : operationPutByValNonStrict, arg1GPR, arg2GPR, arg3GPR);
            
            noResult(node);
            alreadyHandled = true;
            break;
        }
        default:
            break;
        }
        
        if (alreadyHandled)
            break;
        
        // FIXME: the base may not be necessary for some array access modes. But we have to
        // keep it alive to this point, so it's likely to be in a register anyway. Likely
        // no harm in locking it here.
        SpeculateCellOperand base(this, child1);
        SpeculateStrictInt32Operand property(this, child2);
        
        GPRReg baseReg = base.gpr();
        GPRReg propertyReg = property.gpr();

        switch (arrayMode.type()) {
        case Array::Int32:
        case Array::Contiguous: {
            JSValueOperand value(this, child3, ManualOperandSpeculation);

            GPRReg valueReg = value.gpr();
        
            if (!m_compileOkay)
                return;
            
            if (arrayMode.type() == Array::Int32) {
                DFG_TYPE_CHECK(
                    JSValueRegs(valueReg), child3, SpecInt32,
                    m_jit.branch64(
                        MacroAssembler::Below, valueReg, GPRInfo::tagTypeNumberRegister));
            }
        
            if (arrayMode.type() == Array::Contiguous && Heap::isWriteBarrierEnabled()) {
                GPRTemporary scratch(this);
                writeBarrier(baseReg, value.gpr(), child3, WriteBarrierForPropertyAccess, scratch.gpr());
            }

            StorageOperand storage(this, child4);
            GPRReg storageReg = storage.gpr();

            if (node->op() == PutByValAlias) {
                // Store the value to the array.
                GPRReg propertyReg = property.gpr();
                GPRReg valueReg = value.gpr();
                m_jit.store64(valueReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight));
                
                noResult(node);
                break;
            }
            
            GPRTemporary temporary;
            GPRReg temporaryReg = temporaryRegisterForPutByVal(temporary, node);

            MacroAssembler::Jump slowCase;
            
            if (arrayMode.isInBounds()) {
                speculationCheck(
                    StoreToHoleOrOutOfBounds, JSValueRegs(), 0,
                    m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength())));
            } else {
                MacroAssembler::Jump inBounds = m_jit.branch32(MacroAssembler::Below, propertyReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength()));
                
                slowCase = m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfVectorLength()));
                
                if (!arrayMode.isOutOfBounds())
                    speculationCheck(OutOfBounds, JSValueRegs(), 0, slowCase);
                
                m_jit.add32(TrustedImm32(1), propertyReg, temporaryReg);
                m_jit.store32(temporaryReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength()));
                
                inBounds.link(&m_jit);
            }
            
            m_jit.store64(valueReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight));

            base.use();
            property.use();
            value.use();
            storage.use();
            
            if (arrayMode.isOutOfBounds()) {
                addSlowPathGenerator(
                    slowPathCall(
                        slowCase, this,
                        m_jit.codeBlock()->isStrictMode() ? operationPutByValBeyondArrayBoundsStrict : operationPutByValBeyondArrayBoundsNonStrict,
                        NoResult, baseReg, propertyReg, valueReg));
            }

            noResult(node, UseChildrenCalledExplicitly);
            break;
        }
            
        case Array::Double: {
            compileDoublePutByVal(node, base, property);
            break;
        }
            
        case Array::ArrayStorage:
        case Array::SlowPutArrayStorage: {
            JSValueOperand value(this, child3);

            GPRReg valueReg = value.gpr();
        
            if (!m_compileOkay)
                return;
        
            if (Heap::isWriteBarrierEnabled()) {
                GPRTemporary scratch(this);
                writeBarrier(baseReg, value.gpr(), child3, WriteBarrierForPropertyAccess, scratch.gpr());
            }

            StorageOperand storage(this, child4);
            GPRReg storageReg = storage.gpr();

            if (node->op() == PutByValAlias) {
                // Store the value to the array.
                GPRReg propertyReg = property.gpr();
                GPRReg valueReg = value.gpr();
                m_jit.store64(valueReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])));
                
                noResult(node);
                break;
            }
            
            GPRTemporary temporary;
            GPRReg temporaryReg = temporaryRegisterForPutByVal(temporary, node);

            MacroAssembler::JumpList slowCases;

            MacroAssembler::Jump beyondArrayBounds = m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, ArrayStorage::vectorLengthOffset()));
            if (!arrayMode.isOutOfBounds())
                speculationCheck(OutOfBounds, JSValueRegs(), 0, beyondArrayBounds);
            else
                slowCases.append(beyondArrayBounds);

            // Check if we're writing to a hole; if so increment m_numValuesInVector.
            if (arrayMode.isInBounds()) {
                speculationCheck(
                    StoreToHole, JSValueRegs(), 0,
                    m_jit.branchTest64(MacroAssembler::Zero, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]))));
            } else {
                MacroAssembler::Jump notHoleValue = m_jit.branchTest64(MacroAssembler::NonZero, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])));
                if (arrayMode.isSlowPut()) {
                    // This is sort of strange. If we wanted to optimize this code path, we would invert
                    // the above branch. But it's simply not worth it since this only happens if we're
                    // already having a bad time.
                    slowCases.append(m_jit.jump());
                } else {
                    m_jit.add32(TrustedImm32(1), MacroAssembler::Address(storageReg, ArrayStorage::numValuesInVectorOffset()));
                
                    // If we're writing to a hole we might be growing the array; 
                    MacroAssembler::Jump lengthDoesNotNeedUpdate = m_jit.branch32(MacroAssembler::Below, propertyReg, MacroAssembler::Address(storageReg, ArrayStorage::lengthOffset()));
                    m_jit.add32(TrustedImm32(1), propertyReg, temporaryReg);
                    m_jit.store32(temporaryReg, MacroAssembler::Address(storageReg, ArrayStorage::lengthOffset()));
                
                    lengthDoesNotNeedUpdate.link(&m_jit);
                }
                notHoleValue.link(&m_jit);
            }
    
            // Store the value to the array.
            m_jit.store64(valueReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])));

            base.use();
            property.use();
            value.use();
            storage.use();
            
            if (!slowCases.empty()) {
                addSlowPathGenerator(
                    slowPathCall(
                        slowCases, this,
                        m_jit.codeBlock()->isStrictMode() ? operationPutByValBeyondArrayBoundsStrict : operationPutByValBeyondArrayBoundsNonStrict,
                        NoResult, baseReg, propertyReg, valueReg));
            }

            noResult(node, UseChildrenCalledExplicitly);
            break;
        }
            
        case Array::Arguments: {
            JSValueOperand value(this, child3);
            GPRTemporary scratch(this);
            GPRTemporary scratch2(this);
            
            GPRReg valueReg = value.gpr();
            GPRReg scratchReg = scratch.gpr();
            GPRReg scratch2Reg = scratch2.gpr();
            
            if (!m_compileOkay)
                return;

            // Two really lame checks.
            speculationCheck(
                Uncountable, JSValueSource(), 0,
                m_jit.branch32(
                    MacroAssembler::AboveOrEqual, propertyReg,
                    MacroAssembler::Address(baseReg, OBJECT_OFFSETOF(Arguments, m_numArguments))));
            speculationCheck(
                Uncountable, JSValueSource(), 0,
                m_jit.branchTestPtr(
                    MacroAssembler::NonZero,
                    MacroAssembler::Address(
                        baseReg, OBJECT_OFFSETOF(Arguments, m_slowArguments))));
    
            m_jit.move(propertyReg, scratch2Reg);
            m_jit.neg32(scratch2Reg);
            m_jit.signExtend32ToPtr(scratch2Reg, scratch2Reg);
            m_jit.loadPtr(
                MacroAssembler::Address(baseReg, OBJECT_OFFSETOF(Arguments, m_registers)),
                scratchReg);
            
            m_jit.store64(
                valueReg,
                MacroAssembler::BaseIndex(
                    scratchReg, scratch2Reg, MacroAssembler::TimesEight,
                    CallFrame::thisArgumentOffset() * sizeof(Register) - sizeof(Register)));
            
            noResult(node);
            break;
        }
            
        case Array::Int8Array:
            compilePutByValForIntTypedArray(m_jit.vm()->int8ArrayDescriptor(), base.gpr(), property.gpr(), node, sizeof(int8_t), SignedTypedArray);
            break;
            
        case Array::Int16Array:
            compilePutByValForIntTypedArray(m_jit.vm()->int16ArrayDescriptor(), base.gpr(), property.gpr(), node, sizeof(int16_t), SignedTypedArray);
            break;
            
        case Array::Int32Array:
            compilePutByValForIntTypedArray(m_jit.vm()->int32ArrayDescriptor(), base.gpr(), property.gpr(), node, sizeof(int32_t), SignedTypedArray);
            break;
            
        case Array::Uint8Array:
            compilePutByValForIntTypedArray(m_jit.vm()->uint8ArrayDescriptor(), base.gpr(), property.gpr(), node, sizeof(uint8_t), UnsignedTypedArray);
            break;
            
        case Array::Uint8ClampedArray:
            compilePutByValForIntTypedArray(m_jit.vm()->uint8ClampedArrayDescriptor(), base.gpr(), property.gpr(), node, sizeof(uint8_t), UnsignedTypedArray, ClampRounding);
            break;
            
        case Array::Uint16Array:
            compilePutByValForIntTypedArray(m_jit.vm()->uint16ArrayDescriptor(), base.gpr(), property.gpr(), node, sizeof(uint16_t), UnsignedTypedArray);
            break;
            
        case Array::Uint32Array:
            compilePutByValForIntTypedArray(m_jit.vm()->uint32ArrayDescriptor(), base.gpr(), property.gpr(), node, sizeof(uint32_t), UnsignedTypedArray);
            break;
            
        case Array::Float32Array:
            compilePutByValForFloatTypedArray(m_jit.vm()->float32ArrayDescriptor(), base.gpr(), property.gpr(), node, sizeof(float));
            break;
            
        case Array::Float64Array:
            compilePutByValForFloatTypedArray(m_jit.vm()->float64ArrayDescriptor(), base.gpr(), property.gpr(), node, sizeof(double));
            break;
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }

        break;
    }

    case RegExpExec: {
        if (compileRegExpExec(node))
            return;
        if (!node->adjustedRefCount()) {
            SpeculateCellOperand base(this, node->child1());
            SpeculateCellOperand argument(this, node->child2());
            GPRReg baseGPR = base.gpr();
            GPRReg argumentGPR = argument.gpr();
            
            flushRegisters();
            GPRResult result(this);
            callOperation(operationRegExpTest, result.gpr(), baseGPR, argumentGPR);
            
            // Must use jsValueResult because otherwise we screw up register
            // allocation, which thinks that this node has a result.
            jsValueResult(result.gpr(), node);
            break;
        }

        SpeculateCellOperand base(this, node->child1());
        SpeculateCellOperand argument(this, node->child2());
        GPRReg baseGPR = base.gpr();
        GPRReg argumentGPR = argument.gpr();
        
        flushRegisters();
        GPRResult result(this);
        callOperation(operationRegExpExec, result.gpr(), baseGPR, argumentGPR);
        
        jsValueResult(result.gpr(), node);
        break;
    }

    case RegExpTest: {
        SpeculateCellOperand base(this, node->child1());
        SpeculateCellOperand argument(this, node->child2());
        GPRReg baseGPR = base.gpr();
        GPRReg argumentGPR = argument.gpr();
        
        flushRegisters();
        GPRResult result(this);
        callOperation(operationRegExpTest, result.gpr(), baseGPR, argumentGPR);
        
        // If we add a DataFormatBool, we should use it here.
        m_jit.or32(TrustedImm32(ValueFalse), result.gpr());
        jsValueResult(result.gpr(), node, DataFormatJSBoolean);
        break;
    }
        
    case ArrayPush: {
        ASSERT(node->arrayMode().isJSArray());
        
        SpeculateCellOperand base(this, node->child1());
        GPRTemporary storageLength(this);
        
        GPRReg baseGPR = base.gpr();
        GPRReg storageLengthGPR = storageLength.gpr();
        
        StorageOperand storage(this, node->child3());
        GPRReg storageGPR = storage.gpr();

        switch (node->arrayMode().type()) {
        case Array::Int32:
        case Array::Contiguous: {
            JSValueOperand value(this, node->child2(), ManualOperandSpeculation);
            GPRReg valueGPR = value.gpr();

            if (node->arrayMode().type() == Array::Int32) {
                DFG_TYPE_CHECK(
                    JSValueRegs(valueGPR), node->child2(), SpecInt32,
                    m_jit.branch64(
                        MacroAssembler::Below, valueGPR, GPRInfo::tagTypeNumberRegister));
            }

            if (node->arrayMode().type() != Array::Int32 && Heap::isWriteBarrierEnabled()) {
                GPRTemporary scratch(this);
                writeBarrier(baseGPR, valueGPR, node->child2(), WriteBarrierForPropertyAccess, scratch.gpr(), storageLengthGPR);
            }
            
            m_jit.load32(MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()), storageLengthGPR);
            MacroAssembler::Jump slowPath = m_jit.branch32(MacroAssembler::AboveOrEqual, storageLengthGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfVectorLength()));
            m_jit.store64(valueGPR, MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight));
            m_jit.add32(TrustedImm32(1), storageLengthGPR);
            m_jit.store32(storageLengthGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()));
            m_jit.or64(GPRInfo::tagTypeNumberRegister, storageLengthGPR);
            
            addSlowPathGenerator(
                slowPathCall(
                    slowPath, this, operationArrayPush, NoResult, storageLengthGPR,
                    valueGPR, baseGPR));
        
            jsValueResult(storageLengthGPR, node);
            break;
        }
            
        case Array::Double: {
            SpeculateDoubleOperand value(this, node->child2());
            FPRReg valueFPR = value.fpr();

            DFG_TYPE_CHECK(
                JSValueRegs(), node->child2(), SpecRealNumber,
                m_jit.branchDouble(MacroAssembler::DoubleNotEqualOrUnordered, valueFPR, valueFPR));
            
            m_jit.load32(MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()), storageLengthGPR);
            MacroAssembler::Jump slowPath = m_jit.branch32(MacroAssembler::AboveOrEqual, storageLengthGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfVectorLength()));
            m_jit.storeDouble(valueFPR, MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight));
            m_jit.add32(TrustedImm32(1), storageLengthGPR);
            m_jit.store32(storageLengthGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()));
            m_jit.or64(GPRInfo::tagTypeNumberRegister, storageLengthGPR);
            
            addSlowPathGenerator(
                slowPathCall(
                    slowPath, this, operationArrayPushDouble, NoResult, storageLengthGPR,
                    valueFPR, baseGPR));
        
            jsValueResult(storageLengthGPR, node);
            break;
        }
            
        case Array::ArrayStorage: {
            JSValueOperand value(this, node->child2());
            GPRReg valueGPR = value.gpr();

            if (Heap::isWriteBarrierEnabled()) {
                GPRTemporary scratch(this);
                writeBarrier(baseGPR, valueGPR, node->child2(), WriteBarrierForPropertyAccess, scratch.gpr(), storageLengthGPR);
            }

            m_jit.load32(MacroAssembler::Address(storageGPR, ArrayStorage::lengthOffset()), storageLengthGPR);
        
            // Refuse to handle bizarre lengths.
            speculationCheck(Uncountable, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::Above, storageLengthGPR, TrustedImm32(0x7ffffffe)));
        
            MacroAssembler::Jump slowPath = m_jit.branch32(MacroAssembler::AboveOrEqual, storageLengthGPR, MacroAssembler::Address(storageGPR, ArrayStorage::vectorLengthOffset()));
        
            m_jit.store64(valueGPR, MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])));
        
            m_jit.add32(TrustedImm32(1), storageLengthGPR);
            m_jit.store32(storageLengthGPR, MacroAssembler::Address(storageGPR, ArrayStorage::lengthOffset()));
            m_jit.add32(TrustedImm32(1), MacroAssembler::Address(storageGPR, OBJECT_OFFSETOF(ArrayStorage, m_numValuesInVector)));
            m_jit.or64(GPRInfo::tagTypeNumberRegister, storageLengthGPR);
        
            addSlowPathGenerator(
                slowPathCall(
                    slowPath, this, operationArrayPush, NoResult, storageLengthGPR,
                    valueGPR, baseGPR));
        
            jsValueResult(storageLengthGPR, node);
            break;
        }
            
        default:
            CRASH();
            break;
        }
        break;
    }
        
    case ArrayPop: {
        ASSERT(node->arrayMode().isJSArray());

        SpeculateCellOperand base(this, node->child1());
        StorageOperand storage(this, node->child2());
        GPRTemporary value(this);
        GPRTemporary storageLength(this);
        FPRTemporary temp(this); // This is kind of lame, since we don't always need it. I'm relying on the fact that we don't have FPR pressure, especially in code that uses pop().
        
        GPRReg baseGPR = base.gpr();
        GPRReg storageGPR = storage.gpr();
        GPRReg valueGPR = value.gpr();
        GPRReg storageLengthGPR = storageLength.gpr();
        FPRReg tempFPR = temp.fpr();
        
        switch (node->arrayMode().type()) {
        case Array::Int32:
        case Array::Double:
        case Array::Contiguous: {
            m_jit.load32(
                MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()), storageLengthGPR);
            MacroAssembler::Jump undefinedCase =
                m_jit.branchTest32(MacroAssembler::Zero, storageLengthGPR);
            m_jit.sub32(TrustedImm32(1), storageLengthGPR);
            m_jit.store32(
                storageLengthGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()));
            MacroAssembler::Jump slowCase;
            if (node->arrayMode().type() == Array::Double) {
                m_jit.loadDouble(
                    MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight),
                    tempFPR);
                // FIXME: This would not have to be here if changing the publicLength also zeroed the values between the old
                // length and the new length.
                m_jit.store64(
                MacroAssembler::TrustedImm64((int64_t)0), MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight));
                slowCase = m_jit.branchDouble(MacroAssembler::DoubleNotEqualOrUnordered, tempFPR, tempFPR);
                boxDouble(tempFPR, valueGPR);
            } else {
                m_jit.load64(
                    MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight),
                    valueGPR);
                // FIXME: This would not have to be here if changing the publicLength also zeroed the values between the old
                // length and the new length.
                m_jit.store64(
                MacroAssembler::TrustedImm64((int64_t)0), MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight));
                slowCase = m_jit.branchTest64(MacroAssembler::Zero, valueGPR);
            }

            addSlowPathGenerator(
                slowPathMove(
                    undefinedCase, this,
                    MacroAssembler::TrustedImm64(JSValue::encode(jsUndefined())), valueGPR));
            addSlowPathGenerator(
                slowPathCall(
                    slowCase, this, operationArrayPopAndRecoverLength, valueGPR, baseGPR));
            
            // We can't know for sure that the result is an int because of the slow paths. :-/
            jsValueResult(valueGPR, node);
            break;
        }
            
        case Array::ArrayStorage: {
            m_jit.load32(MacroAssembler::Address(storageGPR, ArrayStorage::lengthOffset()), storageLengthGPR);
        
            JITCompiler::Jump undefinedCase =
                m_jit.branchTest32(MacroAssembler::Zero, storageLengthGPR);
        
            m_jit.sub32(TrustedImm32(1), storageLengthGPR);
        
            JITCompiler::JumpList slowCases;
            slowCases.append(m_jit.branch32(MacroAssembler::AboveOrEqual, storageLengthGPR, MacroAssembler::Address(storageGPR, ArrayStorage::vectorLengthOffset())));
        
            m_jit.load64(MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])), valueGPR);
            slowCases.append(m_jit.branchTest64(MacroAssembler::Zero, valueGPR));
        
            m_jit.store32(storageLengthGPR, MacroAssembler::Address(storageGPR, ArrayStorage::lengthOffset()));
        
            m_jit.store64(MacroAssembler::TrustedImm64((int64_t)0), MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])));
            m_jit.sub32(MacroAssembler::TrustedImm32(1), MacroAssembler::Address(storageGPR, OBJECT_OFFSETOF(ArrayStorage, m_numValuesInVector)));
        
            addSlowPathGenerator(
                slowPathMove(
                    undefinedCase, this,
                    MacroAssembler::TrustedImm64(JSValue::encode(jsUndefined())), valueGPR));
        
            addSlowPathGenerator(
                slowPathCall(
                    slowCases, this, operationArrayPop, valueGPR, baseGPR));

            jsValueResult(valueGPR, node);
            break;
        }
            
        default:
            CRASH();
            break;
        }
        break;
    }

    case DFG::Jump: {
        BlockIndex taken = node->takenBlockIndex();
        jump(taken);
        noResult(node);
        break;
    }

    case Branch:
        emitBranch(node);
        break;

    case Return: {
        ASSERT(GPRInfo::callFrameRegister != GPRInfo::regT1);
        ASSERT(GPRInfo::regT1 != GPRInfo::returnValueGPR);
        ASSERT(GPRInfo::returnValueGPR != GPRInfo::callFrameRegister);

#if DFG_ENABLE(SUCCESS_STATS)
        static SamplingCounter counter("SpeculativeJIT");
        m_jit.emitCount(counter);
#endif

        // Return the result in returnValueGPR.
        JSValueOperand op1(this, node->child1());
        m_jit.move(op1.gpr(), GPRInfo::returnValueGPR);

        // Grab the return address.
        m_jit.emitGetFromCallFrameHeaderPtr(JSStack::ReturnPC, GPRInfo::regT1);
        // Restore our caller's "r".
        m_jit.emitGetFromCallFrameHeaderPtr(JSStack::CallerFrame, GPRInfo::callFrameRegister);
        // Return.
        m_jit.restoreReturnAddressBeforeReturn(GPRInfo::regT1);
        m_jit.ret();
        
        noResult(node);
        break;
    }
        
    case Throw:
    case ThrowReferenceError: {
        // We expect that throw statements are rare and are intended to exit the code block
        // anyway, so we just OSR back to the old JIT for now.
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
        break;
    }
        
    case ToPrimitive: {
        RELEASE_ASSERT(node->child1().useKind() == UntypedUse);
        JSValueOperand op1(this, node->child1());
        GPRTemporary result(this, op1);
        
        GPRReg op1GPR = op1.gpr();
        GPRReg resultGPR = result.gpr();
        
        op1.use();
        
        if (!(m_state.forNode(node->child1()).m_type & ~(SpecNumber | SpecBoolean)))
            m_jit.move(op1GPR, resultGPR);
        else {
            MacroAssembler::Jump alreadyPrimitive = m_jit.branchTest64(MacroAssembler::NonZero, op1GPR, GPRInfo::tagMaskRegister);
            MacroAssembler::Jump notPrimitive = m_jit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(op1GPR, JSCell::structureOffset()), MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get()));
            
            alreadyPrimitive.link(&m_jit);
            m_jit.move(op1GPR, resultGPR);
            
            addSlowPathGenerator(
                slowPathCall(notPrimitive, this, operationToPrimitive, resultGPR, op1GPR));
        }
        
        jsValueResult(resultGPR, node, UseChildrenCalledExplicitly);
        break;
    }
        
    case ToString: {
        if (node->child1().useKind() == UntypedUse) {
            JSValueOperand op1(this, node->child1());
            GPRReg op1GPR = op1.gpr();
            
            GPRResult result(this);
            GPRReg resultGPR = result.gpr();
            
            flushRegisters();
            
            JITCompiler::Jump done;
            if (node->child1()->prediction() & SpecString) {
                JITCompiler::Jump slowPath1 = m_jit.branchTest64(
                    JITCompiler::NonZero, op1GPR, GPRInfo::tagMaskRegister);
                JITCompiler::Jump slowPath2 = m_jit.branchPtr(
                    JITCompiler::NotEqual,
                    JITCompiler::Address(op1GPR, JSCell::structureOffset()),
                    TrustedImmPtr(m_jit.vm()->stringStructure.get()));
                m_jit.move(op1GPR, resultGPR);
                done = m_jit.jump();
                slowPath1.link(&m_jit);
                slowPath2.link(&m_jit);
            }
            callOperation(operationToString, resultGPR, op1GPR);
            if (done.isSet())
                done.link(&m_jit);
            cellResult(resultGPR, node);
            break;
        }
        
        compileToStringOnCell(node);
        break;
    }
        
    case NewStringObject: {
        compileNewStringObject(node);
        break;
    }
        
    case NewArray: {
        JSGlobalObject* globalObject = m_jit.graph().globalObjectFor(node->codeOrigin);
        if (!globalObject->isHavingABadTime() && !hasArrayStorage(node->indexingType())) {
            globalObject->havingABadTimeWatchpoint()->add(speculationWatchpoint());
            
            Structure* structure = globalObject->arrayStructureForIndexingTypeDuringAllocation(node->indexingType());
            RELEASE_ASSERT(structure->indexingType() == node->indexingType());
            ASSERT(
                hasUndecided(structure->indexingType())
                || hasInt32(structure->indexingType())
                || hasDouble(structure->indexingType())
                || hasContiguous(structure->indexingType()));
            
            unsigned numElements = node->numChildren();
            
            GPRTemporary result(this);
            GPRTemporary storage(this);
            
            GPRReg resultGPR = result.gpr();
            GPRReg storageGPR = storage.gpr();

            emitAllocateJSArray(resultGPR, structure, storageGPR, numElements);
            
            // At this point, one way or another, resultGPR and storageGPR have pointers to
            // the JSArray and the Butterfly, respectively.
            
            ASSERT(!hasUndecided(structure->indexingType()) || !node->numChildren());
            
            for (unsigned operandIdx = 0; operandIdx < node->numChildren(); ++operandIdx) {
                Edge use = m_jit.graph().m_varArgChildren[node->firstChild() + operandIdx];
                switch (node->indexingType()) {
                case ALL_BLANK_INDEXING_TYPES:
                case ALL_UNDECIDED_INDEXING_TYPES:
                    CRASH();
                    break;
                case ALL_DOUBLE_INDEXING_TYPES: {
                    SpeculateDoubleOperand operand(this, use);
                    FPRReg opFPR = operand.fpr();
                    DFG_TYPE_CHECK(
                        JSValueRegs(), use, SpecRealNumber,
                        m_jit.branchDouble(
                            MacroAssembler::DoubleNotEqualOrUnordered, opFPR, opFPR));
                    m_jit.storeDouble(opFPR, MacroAssembler::Address(storageGPR, sizeof(double) * operandIdx));
                    break;
                }
                case ALL_INT32_INDEXING_TYPES:
                case ALL_CONTIGUOUS_INDEXING_TYPES: {
                    JSValueOperand operand(this, use, ManualOperandSpeculation);
                    GPRReg opGPR = operand.gpr();
                    if (hasInt32(node->indexingType())) {
                        DFG_TYPE_CHECK(
                            JSValueRegs(opGPR), use, SpecInt32,
                            m_jit.branch64(
                                MacroAssembler::Below, opGPR, GPRInfo::tagTypeNumberRegister));
                    }
                    m_jit.store64(opGPR, MacroAssembler::Address(storageGPR, sizeof(JSValue) * operandIdx));
                    break;
                }
                default:
                    CRASH();
                    break;
                }
            }
            
            // Yuck, we should *really* have a way of also returning the storageGPR. But
            // that's the least of what's wrong with this code. We really shouldn't be
            // allocating the array after having computed - and probably spilled to the
            // stack - all of the things that will go into the array. The solution to that
            // bigger problem will also likely fix the redundancy in reloading the storage
            // pointer that we currently have.
            
            cellResult(resultGPR, node);
            break;
        }
        
        if (!node->numChildren()) {
            flushRegisters();
            GPRResult result(this);
            callOperation(operationNewEmptyArray, result.gpr(), globalObject->arrayStructureForIndexingTypeDuringAllocation(node->indexingType()));
            cellResult(result.gpr(), node);
            break;
        }
        
        size_t scratchSize = sizeof(EncodedJSValue) * node->numChildren();
        ScratchBuffer* scratchBuffer = m_jit.vm()->scratchBufferForSize(scratchSize);
        EncodedJSValue* buffer = scratchBuffer ? static_cast<EncodedJSValue*>(scratchBuffer->dataBuffer()) : 0;
        
        for (unsigned operandIdx = 0; operandIdx < node->numChildren(); ++operandIdx) {
            // Need to perform the speculations that this node promises to perform. If we're
            // emitting code here and the indexing type is not array storage then there is
            // probably something hilarious going on and we're already failing at all the
            // things, but at least we're going to be sound.
            Edge use = m_jit.graph().m_varArgChildren[node->firstChild() + operandIdx];
            switch (node->indexingType()) {
            case ALL_BLANK_INDEXING_TYPES:
            case ALL_UNDECIDED_INDEXING_TYPES:
                CRASH();
                break;
            case ALL_DOUBLE_INDEXING_TYPES: {
                SpeculateDoubleOperand operand(this, use);
                GPRTemporary scratch(this);
                FPRReg opFPR = operand.fpr();
                GPRReg scratchGPR = scratch.gpr();
                DFG_TYPE_CHECK(
                    JSValueRegs(), use, SpecRealNumber,
                    m_jit.branchDouble(
                        MacroAssembler::DoubleNotEqualOrUnordered, opFPR, opFPR));
                m_jit.boxDouble(opFPR, scratchGPR);
                m_jit.store64(scratchGPR, buffer + operandIdx);
                break;
            }
            case ALL_INT32_INDEXING_TYPES: {
                JSValueOperand operand(this, use, ManualOperandSpeculation);
                GPRReg opGPR = operand.gpr();
                if (hasInt32(node->indexingType())) {
                    DFG_TYPE_CHECK(
                        JSValueRegs(opGPR), use, SpecInt32,
                        m_jit.branch64(
                            MacroAssembler::Below, opGPR, GPRInfo::tagTypeNumberRegister));
                }
                m_jit.store64(opGPR, buffer + operandIdx);
                break;
            }
            case ALL_CONTIGUOUS_INDEXING_TYPES:
            case ALL_ARRAY_STORAGE_INDEXING_TYPES: {
                JSValueOperand operand(this, use);
                GPRReg opGPR = operand.gpr();
                m_jit.store64(opGPR, buffer + operandIdx);
                operand.use();
                break;
            }
            default:
                CRASH();
                break;
            }
        }
        
        switch (node->indexingType()) {
        case ALL_DOUBLE_INDEXING_TYPES:
        case ALL_INT32_INDEXING_TYPES:
            useChildren(node);
            break;
        default:
            break;
        }
        
        flushRegisters();

        if (scratchSize) {
            GPRTemporary scratch(this);

            // Tell GC mark phase how much of the scratch buffer is active during call.
            m_jit.move(TrustedImmPtr(scratchBuffer->activeLengthPtr()), scratch.gpr());
            m_jit.storePtr(TrustedImmPtr(scratchSize), scratch.gpr());
        }

        GPRResult result(this);
        
        callOperation(
            operationNewArray, result.gpr(), globalObject->arrayStructureForIndexingTypeDuringAllocation(node->indexingType()),
            static_cast<void*>(buffer), node->numChildren());

        if (scratchSize) {
            GPRTemporary scratch(this);

            m_jit.move(TrustedImmPtr(scratchBuffer->activeLengthPtr()), scratch.gpr());
            m_jit.storePtr(TrustedImmPtr(0), scratch.gpr());
        }

        cellResult(result.gpr(), node, UseChildrenCalledExplicitly);
        break;
    }
        
    case NewArrayWithSize: {
        JSGlobalObject* globalObject = m_jit.graph().globalObjectFor(node->codeOrigin);
        if (!globalObject->isHavingABadTime() && !hasArrayStorage(node->indexingType())) {
            globalObject->havingABadTimeWatchpoint()->add(speculationWatchpoint());
            
            SpeculateStrictInt32Operand size(this, node->child1());
            GPRTemporary result(this);
            GPRTemporary storage(this);
            GPRTemporary scratch(this);
            GPRTemporary scratch2(this);
            
            GPRReg sizeGPR = size.gpr();
            GPRReg resultGPR = result.gpr();
            GPRReg storageGPR = storage.gpr();
            GPRReg scratchGPR = scratch.gpr();
            GPRReg scratch2GPR = scratch2.gpr();
            
            MacroAssembler::JumpList slowCases;
            slowCases.append(m_jit.branch32(MacroAssembler::AboveOrEqual, sizeGPR, TrustedImm32(MIN_SPARSE_ARRAY_INDEX)));
            
            ASSERT((1 << 3) == sizeof(JSValue));
            m_jit.move(sizeGPR, scratchGPR);
            m_jit.lshift32(TrustedImm32(3), scratchGPR);
            m_jit.add32(TrustedImm32(sizeof(IndexingHeader)), scratchGPR, resultGPR);
            slowCases.append(
                emitAllocateBasicStorage(resultGPR, storageGPR));
            m_jit.subPtr(scratchGPR, storageGPR);
            Structure* structure = globalObject->arrayStructureForIndexingTypeDuringAllocation(node->indexingType());
            emitAllocateJSObject<JSArray>(resultGPR, ImmPtr(structure), storageGPR, scratchGPR, scratch2GPR, slowCases);
            
            m_jit.store32(sizeGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()));
            m_jit.store32(sizeGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfVectorLength()));
            
            if (hasDouble(node->indexingType())) {
                m_jit.move(TrustedImm64(bitwise_cast<int64_t>(QNaN)), scratchGPR);
                m_jit.move(sizeGPR, scratch2GPR);
                MacroAssembler::Jump done = m_jit.branchTest32(MacroAssembler::Zero, scratch2GPR);
                MacroAssembler::Label loop = m_jit.label();
                m_jit.sub32(TrustedImm32(1), scratch2GPR);
                m_jit.store64(scratchGPR, MacroAssembler::BaseIndex(storageGPR, scratch2GPR, MacroAssembler::TimesEight));
                m_jit.branchTest32(MacroAssembler::NonZero, scratch2GPR).linkTo(loop, &m_jit);
                done.link(&m_jit);
            }
            
            addSlowPathGenerator(adoptPtr(
                new CallArrayAllocatorWithVariableSizeSlowPathGenerator(
                    slowCases, this, operationNewArrayWithSize, resultGPR,
                    globalObject->arrayStructureForIndexingTypeDuringAllocation(node->indexingType()),
                    globalObject->arrayStructureForIndexingTypeDuringAllocation(ArrayWithArrayStorage),
                    sizeGPR)));
            
            cellResult(resultGPR, node);
            break;
        }
        
        SpeculateStrictInt32Operand size(this, node->child1());
        GPRReg sizeGPR = size.gpr();
        flushRegisters();
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();
        GPRReg structureGPR = selectScratchGPR(sizeGPR);
        MacroAssembler::Jump bigLength = m_jit.branch32(MacroAssembler::AboveOrEqual, sizeGPR, TrustedImm32(MIN_SPARSE_ARRAY_INDEX));
        m_jit.move(TrustedImmPtr(globalObject->arrayStructureForIndexingTypeDuringAllocation(node->indexingType())), structureGPR);
        MacroAssembler::Jump done = m_jit.jump();
        bigLength.link(&m_jit);
        m_jit.move(TrustedImmPtr(globalObject->arrayStructureForIndexingTypeDuringAllocation(ArrayWithArrayStorage)), structureGPR);
        done.link(&m_jit);
        callOperation(operationNewArrayWithSize, resultGPR, structureGPR, sizeGPR);
        cellResult(resultGPR, node);
        break;
    }
        
    case NewArrayBuffer: {
        JSGlobalObject* globalObject = m_jit.graph().globalObjectFor(node->codeOrigin);
        IndexingType indexingType = node->indexingType();
        if (!globalObject->isHavingABadTime() && !hasArrayStorage(indexingType)) {
            globalObject->havingABadTimeWatchpoint()->add(speculationWatchpoint());
            
            unsigned numElements = node->numConstants();
            
            GPRTemporary result(this);
            GPRTemporary storage(this);
            
            GPRReg resultGPR = result.gpr();
            GPRReg storageGPR = storage.gpr();

            emitAllocateJSArray(resultGPR, globalObject->arrayStructureForIndexingTypeDuringAllocation(indexingType), storageGPR, numElements);
            
            RELEASE_ASSERT(indexingType & IsArray);
            JSValue* data = m_jit.codeBlock()->constantBuffer(node->startConstant());
            if (indexingType == ArrayWithDouble) {
                for (unsigned index = 0; index < node->numConstants(); ++index) {
                    double value = data[index].asNumber();
                    m_jit.store64(
                        Imm64(bitwise_cast<int64_t>(value)),
                        MacroAssembler::Address(storageGPR, sizeof(double) * index));
                }
            } else {
                for (unsigned index = 0; index < node->numConstants(); ++index) {
                    m_jit.store64(
                        Imm64(JSValue::encode(data[index])),
                        MacroAssembler::Address(storageGPR, sizeof(JSValue) * index));
                }
            }
            
            cellResult(resultGPR, node);
            break;
        }
        
        flushRegisters();
        GPRResult result(this);
        
        callOperation(operationNewArrayBuffer, result.gpr(), globalObject->arrayStructureForIndexingTypeDuringAllocation(node->indexingType()), node->startConstant(), node->numConstants());
        
        cellResult(result.gpr(), node);
        break;
    }
        
    case NewRegexp: {
        flushRegisters();
        GPRResult result(this);
        
        callOperation(operationNewRegexp, result.gpr(), m_jit.codeBlock()->regexp(node->regexpIndex()));
        
        cellResult(result.gpr(), node);
        break;
    }
        
    case ConvertThis: {
        ASSERT(node->child1().useKind() == UntypedUse);
        JSValueOperand thisValue(this, node->child1());
        GPRReg thisValueGPR = thisValue.gpr();
        
        flushRegisters();
        
        GPRResult result(this);
        callOperation(operationConvertThis, result.gpr(), thisValueGPR);
        
        cellResult(result.gpr(), node);
        break;
    }

    case CreateThis: {
        // Note that there is not so much profit to speculate here. The only things we
        // speculate on are (1) that it's a cell, since that eliminates cell checks
        // later if the proto is reused, and (2) if we have a FinalObject prediction
        // then we speculate because we want to get recompiled if it isn't (since
        // otherwise we'd start taking slow path a lot).
        
        SpeculateCellOperand callee(this, node->child1());
        GPRTemporary result(this);
        GPRTemporary allocator(this);
        GPRTemporary structure(this);
        GPRTemporary scratch(this);
        
        GPRReg calleeGPR = callee.gpr();
        GPRReg resultGPR = result.gpr();
        GPRReg allocatorGPR = allocator.gpr();
        GPRReg structureGPR = structure.gpr();
        GPRReg scratchGPR = scratch.gpr();

        MacroAssembler::JumpList slowPath;

        m_jit.loadPtr(JITCompiler::Address(calleeGPR, JSFunction::offsetOfAllocationProfile() + ObjectAllocationProfile::offsetOfAllocator()), allocatorGPR);
        m_jit.loadPtr(JITCompiler::Address(calleeGPR, JSFunction::offsetOfAllocationProfile() + ObjectAllocationProfile::offsetOfStructure()), structureGPR);
        slowPath.append(m_jit.branchTestPtr(MacroAssembler::Zero, allocatorGPR));
        emitAllocateJSObject(resultGPR, allocatorGPR, structureGPR, TrustedImmPtr(0), scratchGPR, slowPath);

        addSlowPathGenerator(slowPathCall(slowPath, this, operationCreateThis, resultGPR, calleeGPR, node->inlineCapacity()));
        
        cellResult(resultGPR, node);
        break;
    }
        
    case AllocationProfileWatchpoint: {
        jsCast<JSFunction*>(node->function())->addAllocationProfileWatchpoint(speculationWatchpoint());
        noResult(node);
        break;
    }

    case NewObject: {
        GPRTemporary result(this);
        GPRTemporary allocator(this);
        GPRTemporary scratch(this);
        
        GPRReg resultGPR = result.gpr();
        GPRReg allocatorGPR = allocator.gpr();
        GPRReg scratchGPR = scratch.gpr();
        
        MacroAssembler::JumpList slowPath;

        Structure* structure = node->structure();
        size_t allocationSize = JSObject::allocationSize(structure->inlineCapacity());
        MarkedAllocator* allocatorPtr = &m_jit.vm()->heap.allocatorForObjectWithoutDestructor(allocationSize);

        m_jit.move(TrustedImmPtr(allocatorPtr), allocatorGPR);
        emitAllocateJSObject(resultGPR, allocatorGPR, TrustedImmPtr(structure), TrustedImmPtr(0), scratchGPR, slowPath);

        addSlowPathGenerator(slowPathCall(slowPath, this, operationNewObject, resultGPR, structure));
        
        cellResult(resultGPR, node);
        break;
    }

    case GetCallee: {
        GPRTemporary result(this);
        m_jit.loadPtr(JITCompiler::addressFor(static_cast<VirtualRegister>(node->codeOrigin.stackOffset() + static_cast<int>(JSStack::Callee))), result.gpr());
        cellResult(result.gpr(), node);
        break;
    }
        
    case SetCallee: {
        SpeculateCellOperand callee(this, node->child1());
        m_jit.storePtr(callee.gpr(), JITCompiler::addressFor(static_cast<VirtualRegister>(node->codeOrigin.stackOffset() + static_cast<int>(JSStack::Callee))));
        noResult(node);
        break;
    }
        
    case GetScope: {
        SpeculateCellOperand function(this, node->child1());
        GPRTemporary result(this, function);
        m_jit.loadPtr(JITCompiler::Address(function.gpr(), JSFunction::offsetOfScopeChain()), result.gpr());
        cellResult(result.gpr(), node);
        break;
    }
        
    case GetMyScope: {
        GPRTemporary result(this);
        GPRReg resultGPR = result.gpr();

        m_jit.loadPtr(JITCompiler::addressFor(static_cast<VirtualRegister>(node->codeOrigin.stackOffset() + static_cast<int>(JSStack::ScopeChain))), resultGPR);
        cellResult(resultGPR, node);
        break;
    }
        
    case SetMyScope: {
        SpeculateCellOperand callee(this, node->child1());
        m_jit.storePtr(callee.gpr(), JITCompiler::addressFor(static_cast<VirtualRegister>(node->codeOrigin.stackOffset() + static_cast<int>(JSStack::ScopeChain))));
        noResult(node);
        break;
    }
        
    case SkipTopScope: {
        SpeculateCellOperand scope(this, node->child1());
        GPRTemporary result(this, scope);
        GPRReg resultGPR = result.gpr();
        m_jit.move(scope.gpr(), resultGPR);
        JITCompiler::Jump activationNotCreated =
            m_jit.branchTest64(
                JITCompiler::Zero,
                JITCompiler::addressFor(
                    static_cast<VirtualRegister>(m_jit.codeBlock()->activationRegister())));
        m_jit.loadPtr(JITCompiler::Address(resultGPR, JSScope::offsetOfNext()), resultGPR);
        activationNotCreated.link(&m_jit);
        cellResult(resultGPR, node);
        break;
    }
        
    case SkipScope: {
        SpeculateCellOperand scope(this, node->child1());
        GPRTemporary result(this, scope);
        m_jit.loadPtr(JITCompiler::Address(scope.gpr(), JSScope::offsetOfNext()), result.gpr());
        cellResult(result.gpr(), node);
        break;
    }
        
    case GetScopeRegisters: {
        SpeculateCellOperand scope(this, node->child1());
        GPRTemporary result(this);
        GPRReg scopeGPR = scope.gpr();
        GPRReg resultGPR = result.gpr();

        m_jit.loadPtr(JITCompiler::Address(scopeGPR, JSVariableObject::offsetOfRegisters()), resultGPR);
        storageResult(resultGPR, node);
        break;
    }
    case GetScopedVar: {
        StorageOperand registers(this, node->child1());
        GPRTemporary result(this);
        GPRReg registersGPR = registers.gpr();
        GPRReg resultGPR = result.gpr();

        m_jit.load64(JITCompiler::Address(registersGPR, node->varNumber() * sizeof(Register)), resultGPR);
        jsValueResult(resultGPR, node);
        break;
    }
    case PutScopedVar: {
        SpeculateCellOperand scope(this, node->child1());
        StorageOperand registers(this, node->child2());
        JSValueOperand value(this, node->child3());
        GPRTemporary scratchRegister(this);

        GPRReg scopeGPR = scope.gpr();
        GPRReg registersGPR = registers.gpr();
        GPRReg valueGPR = value.gpr();
        GPRReg scratchGPR = scratchRegister.gpr();

        m_jit.store64(valueGPR, JITCompiler::Address(registersGPR, node->varNumber() * sizeof(Register)));
        writeBarrier(scopeGPR, valueGPR, node->child3(), WriteBarrierForVariableAccess, scratchGPR);
        noResult(node);
        break;
    }
    case GetById: {
        if (!node->prediction()) {
            terminateSpeculativeExecution(InadequateCoverage, JSValueRegs(), 0);
            break;
        }

        switch (node->child1().useKind()) {
        case CellUse: {
            SpeculateCellOperand base(this, node->child1());
            GPRTemporary result(this, base);
            
            GPRReg baseGPR = base.gpr();
            GPRReg resultGPR = result.gpr();
            
            base.use();
            
            cachedGetById(node->codeOrigin, baseGPR, resultGPR, node->identifierNumber());
            
            jsValueResult(resultGPR, node, UseChildrenCalledExplicitly);
            break;
        }
        
        case UntypedUse: {
            JSValueOperand base(this, node->child1());
            GPRTemporary result(this, base);
        
            GPRReg baseGPR = base.gpr();
            GPRReg resultGPR = result.gpr();
        
            base.use();
        
            JITCompiler::Jump notCell = m_jit.branchTest64(JITCompiler::NonZero, baseGPR, GPRInfo::tagMaskRegister);
        
            cachedGetById(node->codeOrigin, baseGPR, resultGPR, node->identifierNumber(), notCell);
        
            jsValueResult(resultGPR, node, UseChildrenCalledExplicitly);
            break;
        }
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        break;
    }

    case GetByIdFlush: {
        if (!node->prediction()) {
            terminateSpeculativeExecution(InadequateCoverage, JSValueRegs(), 0);
            break;
        }

        switch (node->child1().useKind()) {
        case CellUse: {
            SpeculateCellOperand base(this, node->child1());
            GPRReg baseGPR = base.gpr();

            GPRResult result(this);
            
            GPRReg resultGPR = result.gpr();
            
            base.use();
            
            flushRegisters();
            
            cachedGetById(node->codeOrigin, baseGPR, resultGPR, node->identifierNumber(), JITCompiler::Jump(), DontSpill);
            
            jsValueResult(resultGPR, node, UseChildrenCalledExplicitly);
            break;
        }
        
        case UntypedUse: {
            JSValueOperand base(this, node->child1());
            GPRReg baseGPR = base.gpr();

            GPRResult result(this);
            GPRReg resultGPR = result.gpr();
        
            base.use();
            flushRegisters();
        
            JITCompiler::Jump notCell = m_jit.branchTest64(JITCompiler::NonZero, baseGPR, GPRInfo::tagMaskRegister);
        
            cachedGetById(node->codeOrigin, baseGPR, resultGPR, node->identifierNumber(), notCell, DontSpill);
        
            jsValueResult(resultGPR, node, UseChildrenCalledExplicitly);
            break;
        }
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        break;
    }

    case GetArrayLength:
        compileGetArrayLength(node);
        break;
        
    case CheckFunction: {
        SpeculateCellOperand function(this, node->child1());
        speculationCheck(BadFunction, JSValueSource::unboxedCell(function.gpr()), node->child1(), m_jit.branchWeakPtr(JITCompiler::NotEqual, function.gpr(), node->function()));
        noResult(node);
        break;
    }
        
    case CheckExecutable: {
        SpeculateCellOperand function(this, node->child1());
        speculationCheck(BadExecutable, JSValueSource::unboxedCell(function.gpr()), node->child1(), m_jit.branchWeakPtr(JITCompiler::NotEqual, JITCompiler::Address(function.gpr(), JSFunction::offsetOfExecutable()), node->executable()));
        noResult(node);
        break;
    }
        
    case CheckStructure:
    case ForwardCheckStructure: {
        SpeculateCellOperand base(this, node->child1());
        
        ASSERT(node->structureSet().size());
        
        ExitKind exitKind;
        if (node->child1()->op() == WeakJSConstant)
            exitKind = BadWeakConstantCache;
        else
            exitKind = BadCache;
        
        if (node->structureSet().size() == 1) {
            speculationCheck(
                exitKind, JSValueSource::unboxedCell(base.gpr()), 0,
                m_jit.branchWeakPtr(
                    JITCompiler::NotEqual,
                    JITCompiler::Address(base.gpr(), JSCell::structureOffset()),
                    node->structureSet()[0]));
        } else {
            GPRTemporary structure(this);
            
            m_jit.loadPtr(JITCompiler::Address(base.gpr(), JSCell::structureOffset()), structure.gpr());
            
            JITCompiler::JumpList done;
            
            for (size_t i = 0; i < node->structureSet().size() - 1; ++i)
                done.append(m_jit.branchWeakPtr(JITCompiler::Equal, structure.gpr(), node->structureSet()[i]));
            
            speculationCheck(
                exitKind, JSValueSource::unboxedCell(base.gpr()), 0,
                m_jit.branchWeakPtr(
                    JITCompiler::NotEqual, structure.gpr(), node->structureSet().last()));
            
            done.link(&m_jit);
        }
        
        noResult(node);
        break;
    }
        
    case StructureTransitionWatchpoint:
    case ForwardStructureTransitionWatchpoint: {
        // There is a fascinating question here of what to do about array profiling.
        // We *could* try to tell the OSR exit about where the base of the access is.
        // The DFG will have kept it alive, though it may not be in a register, and
        // we shouldn't really load it since that could be a waste. For now though,
        // we'll just rely on the fact that when a watchpoint fires then that's
        // quite a hint already.
        
        m_jit.addWeakReference(node->structure());
        node->structure()->addTransitionWatchpoint(
            speculationWatchpoint(
                node->child1()->op() == WeakJSConstant ? BadWeakConstantCache : BadCache));

#if !ASSERT_DISABLED
        SpeculateCellOperand op1(this, node->child1());
        JITCompiler::Jump isOK = m_jit.branchPtr(JITCompiler::Equal, JITCompiler::Address(op1.gpr(), JSCell::structureOffset()), TrustedImmPtr(node->structure()));
        m_jit.breakpoint();
        isOK.link(&m_jit);
#else
        speculateCell(node->child1());
#endif
        
        noResult(node);
        break;
    }
        
    case PhantomPutStructure: {
        ASSERT(isKnownCell(node->child1().node()));
        
        ASSERT(node->structureTransitionData().previousStructure->transitionWatchpointSetHasBeenInvalidated());
        m_jit.addWeakReferenceTransition(
            node->codeOrigin.codeOriginOwner(),
            node->structureTransitionData().previousStructure,
            node->structureTransitionData().newStructure);
        noResult(node);
        break;
    }
        
    case PutStructure: {
        ASSERT(node->structureTransitionData().previousStructure->transitionWatchpointSetHasBeenInvalidated());

        SpeculateCellOperand base(this, node->child1());
        GPRReg baseGPR = base.gpr();
        
        m_jit.addWeakReferenceTransition(
            node->codeOrigin.codeOriginOwner(),
            node->structureTransitionData().previousStructure,
            node->structureTransitionData().newStructure);
        
#if ENABLE(WRITE_BARRIER_PROFILING)
        // Must always emit this write barrier as the structure transition itself requires it
        writeBarrier(baseGPR, node->structureTransitionData().newStructure, WriteBarrierForGenericAccess);
#endif
        
        m_jit.storePtr(MacroAssembler::TrustedImmPtr(node->structureTransitionData().newStructure), MacroAssembler::Address(baseGPR, JSCell::structureOffset()));
        
        noResult(node);
        break;
    }
        
    case AllocatePropertyStorage:
        compileAllocatePropertyStorage(node);
        break;
        
    case ReallocatePropertyStorage:
        compileReallocatePropertyStorage(node);
        break;
        
    case GetButterfly: {
        SpeculateCellOperand base(this, node->child1());
        GPRTemporary result(this, base);
        
        GPRReg baseGPR = base.gpr();
        GPRReg resultGPR = result.gpr();
        
        m_jit.loadPtr(JITCompiler::Address(baseGPR, JSObject::butterflyOffset()), resultGPR);
        
        storageResult(resultGPR, node);
        break;
    }

    case GetIndexedPropertyStorage: {
        compileGetIndexedPropertyStorage(node);
        break;
    }
        
    case GetByOffset: {
        StorageOperand storage(this, node->child1());
        GPRTemporary result(this, storage);
        
        GPRReg storageGPR = storage.gpr();
        GPRReg resultGPR = result.gpr();
        
        StorageAccessData& storageAccessData = m_jit.graph().m_storageAccessData[node->storageAccessDataIndex()];
        
        m_jit.load64(JITCompiler::Address(storageGPR, storageAccessData.offset * sizeof(EncodedJSValue)), resultGPR);
        
        jsValueResult(resultGPR, node);
        break;
    }
        
    case PutByOffset: {
#if ENABLE(WRITE_BARRIER_PROFILING)
        SpeculateCellOperand base(this, node->child2());
#endif
        StorageOperand storage(this, node->child1());
        JSValueOperand value(this, node->child3());

        GPRReg storageGPR = storage.gpr();
        GPRReg valueGPR = value.gpr();
        
#if ENABLE(WRITE_BARRIER_PROFILING)
        writeBarrier(base.gpr(), value.gpr(), node->child3(), WriteBarrierForPropertyAccess);
#endif

        StorageAccessData& storageAccessData = m_jit.graph().m_storageAccessData[node->storageAccessDataIndex()];
        
        m_jit.store64(valueGPR, JITCompiler::Address(storageGPR, storageAccessData.offset * sizeof(EncodedJSValue)));
        
        noResult(node);
        break;
    }
        
    case PutById: {
        SpeculateCellOperand base(this, node->child1());
        JSValueOperand value(this, node->child2());
        GPRTemporary scratch(this);
        
        GPRReg baseGPR = base.gpr();
        GPRReg valueGPR = value.gpr();
        GPRReg scratchGPR = scratch.gpr();
        
        base.use();
        value.use();

        cachedPutById(node->codeOrigin, baseGPR, valueGPR, node->child2(), scratchGPR, node->identifierNumber(), NotDirect);
        
        noResult(node, UseChildrenCalledExplicitly);
        break;
    }

    case PutByIdDirect: {
        SpeculateCellOperand base(this, node->child1());
        JSValueOperand value(this, node->child2());
        GPRTemporary scratch(this);
        
        GPRReg baseGPR = base.gpr();
        GPRReg valueGPR = value.gpr();
        GPRReg scratchGPR = scratch.gpr();
        
        base.use();
        value.use();

        cachedPutById(node->codeOrigin, baseGPR, valueGPR, node->child2(), scratchGPR, node->identifierNumber(), Direct);

        noResult(node, UseChildrenCalledExplicitly);
        break;
    }

    case GetGlobalVar: {
        GPRTemporary result(this);

        m_jit.load64(node->registerPointer(), result.gpr());

        jsValueResult(result.gpr(), node);
        break;
    }

    case PutGlobalVar: {
        JSValueOperand value(this, node->child1());
        
        if (Heap::isWriteBarrierEnabled()) {
            GPRTemporary scratch(this);
            GPRReg scratchReg = scratch.gpr();
            
            writeBarrier(m_jit.globalObjectFor(node->codeOrigin), value.gpr(), node->child1(), WriteBarrierForVariableAccess, scratchReg);
        }
        
        m_jit.store64(value.gpr(), node->registerPointer());

        noResult(node);
        break;
    }

    case PutGlobalVarCheck: {
        JSValueOperand value(this, node->child1());
        
        WatchpointSet* watchpointSet =
            m_jit.globalObjectFor(node->codeOrigin)->symbolTable()->get(
                identifier(node->identifierNumberForCheck())->impl()).watchpointSet();
        addSlowPathGenerator(
            slowPathCall(
                m_jit.branchTest8(
                    JITCompiler::NonZero,
                    JITCompiler::AbsoluteAddress(watchpointSet->addressOfIsWatched())),
                this, operationNotifyGlobalVarWrite, NoResult, watchpointSet));
        
        if (Heap::isWriteBarrierEnabled()) {
            GPRTemporary scratch(this);
            GPRReg scratchReg = scratch.gpr();
            
            writeBarrier(m_jit.globalObjectFor(node->codeOrigin), value.gpr(), node->child1(), WriteBarrierForVariableAccess, scratchReg);
        }
        
        m_jit.store64(value.gpr(), node->registerPointer());

        noResult(node);
        break;
    }
        
    case GlobalVarWatchpoint: {
        m_jit.globalObjectFor(node->codeOrigin)->symbolTable()->get(
            identifier(node->identifierNumberForCheck())->impl()).addWatchpoint(
                speculationWatchpoint());
        
#if DFG_ENABLE(JIT_ASSERT)
        GPRTemporary scratch(this);
        GPRReg scratchGPR = scratch.gpr();
        m_jit.load64(node->registerPointer(), scratchGPR);
        JITCompiler::Jump ok = m_jit.branch64(
            JITCompiler::Equal, scratchGPR,
            TrustedImm64(JSValue::encode(node->registerPointer()->get())));
        m_jit.breakpoint();
        ok.link(&m_jit);
#endif
        
        noResult(node);
        break;
    }

    case CheckHasInstance: {
        SpeculateCellOperand base(this, node->child1());
        GPRTemporary structure(this);

        // Speculate that base 'ImplementsDefaultHasInstance'.
        m_jit.loadPtr(MacroAssembler::Address(base.gpr(), JSCell::structureOffset()), structure.gpr());
        speculationCheck(Uncountable, JSValueRegs(), 0, m_jit.branchTest8(MacroAssembler::Zero, MacroAssembler::Address(structure.gpr(), Structure::typeInfoFlagsOffset()), MacroAssembler::TrustedImm32(ImplementsDefaultHasInstance)));

        noResult(node);
        break;
    }

    case InstanceOf: {
        compileInstanceOf(node);
        break;
    }
        
    case IsUndefined: {
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this);
        GPRTemporary localGlobalObject(this);
        GPRTemporary remoteGlobalObject(this);

        JITCompiler::Jump isCell = m_jit.branchTest64(JITCompiler::Zero, value.gpr(), GPRInfo::tagMaskRegister);

        m_jit.compare64(JITCompiler::Equal, value.gpr(), TrustedImm32(ValueUndefined), result.gpr());
        JITCompiler::Jump done = m_jit.jump();
        
        isCell.link(&m_jit);
        JITCompiler::Jump notMasqueradesAsUndefined;
        if (m_jit.graph().globalObjectFor(node->codeOrigin)->masqueradesAsUndefinedWatchpoint()->isStillValid()) {
            m_jit.graph().globalObjectFor(node->codeOrigin)->masqueradesAsUndefinedWatchpoint()->add(speculationWatchpoint());
            m_jit.move(TrustedImm32(0), result.gpr());
            notMasqueradesAsUndefined = m_jit.jump();
        } else {
            m_jit.loadPtr(JITCompiler::Address(value.gpr(), JSCell::structureOffset()), result.gpr());
            JITCompiler::Jump isMasqueradesAsUndefined = m_jit.branchTest8(JITCompiler::NonZero, JITCompiler::Address(result.gpr(), Structure::typeInfoFlagsOffset()), TrustedImm32(MasqueradesAsUndefined));
            m_jit.move(TrustedImm32(0), result.gpr());
            notMasqueradesAsUndefined = m_jit.jump();

            isMasqueradesAsUndefined.link(&m_jit);
            GPRReg localGlobalObjectGPR = localGlobalObject.gpr();
            GPRReg remoteGlobalObjectGPR = remoteGlobalObject.gpr();
            m_jit.move(TrustedImmPtr(m_jit.globalObjectFor(node->codeOrigin)), localGlobalObjectGPR);
            m_jit.loadPtr(JITCompiler::Address(result.gpr(), Structure::globalObjectOffset()), remoteGlobalObjectGPR); 
            m_jit.comparePtr(JITCompiler::Equal, localGlobalObjectGPR, remoteGlobalObjectGPR, result.gpr());
        }

        notMasqueradesAsUndefined.link(&m_jit);
        done.link(&m_jit);
        m_jit.or32(TrustedImm32(ValueFalse), result.gpr());
        jsValueResult(result.gpr(), node, DataFormatJSBoolean);
        break;
    }
        
    case IsBoolean: {
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this, value);
        
        m_jit.move(value.gpr(), result.gpr());
        m_jit.xor64(JITCompiler::TrustedImm32(ValueFalse), result.gpr());
        m_jit.test64(JITCompiler::Zero, result.gpr(), JITCompiler::TrustedImm32(static_cast<int32_t>(~1)), result.gpr());
        m_jit.or32(TrustedImm32(ValueFalse), result.gpr());
        jsValueResult(result.gpr(), node, DataFormatJSBoolean);
        break;
    }
        
    case IsNumber: {
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this, value);
        
        m_jit.test64(JITCompiler::NonZero, value.gpr(), GPRInfo::tagTypeNumberRegister, result.gpr());
        m_jit.or32(TrustedImm32(ValueFalse), result.gpr());
        jsValueResult(result.gpr(), node, DataFormatJSBoolean);
        break;
    }
        
    case IsString: {
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this, value);
        
        JITCompiler::Jump isNotCell = m_jit.branchTest64(JITCompiler::NonZero, value.gpr(), GPRInfo::tagMaskRegister);
        
        m_jit.loadPtr(JITCompiler::Address(value.gpr(), JSCell::structureOffset()), result.gpr());
        m_jit.compare8(JITCompiler::Equal, JITCompiler::Address(result.gpr(), Structure::typeInfoTypeOffset()), TrustedImm32(StringType), result.gpr());
        m_jit.or32(TrustedImm32(ValueFalse), result.gpr());
        JITCompiler::Jump done = m_jit.jump();
        
        isNotCell.link(&m_jit);
        m_jit.move(TrustedImm32(ValueFalse), result.gpr());
        
        done.link(&m_jit);
        jsValueResult(result.gpr(), node, DataFormatJSBoolean);
        break;
    }
        
    case IsObject: {
        JSValueOperand value(this, node->child1());
        GPRReg valueGPR = value.gpr();
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();
        flushRegisters();
        callOperation(operationIsObject, resultGPR, valueGPR);
        m_jit.or32(TrustedImm32(ValueFalse), resultGPR);
        jsValueResult(result.gpr(), node, DataFormatJSBoolean);
        break;
    }

    case IsFunction: {
        JSValueOperand value(this, node->child1());
        GPRReg valueGPR = value.gpr();
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();
        flushRegisters();
        callOperation(operationIsFunction, resultGPR, valueGPR);
        m_jit.or32(TrustedImm32(ValueFalse), resultGPR);
        jsValueResult(result.gpr(), node, DataFormatJSBoolean);
        break;
    }

    case TypeOf: {
        JSValueOperand value(this, node->child1(), ManualOperandSpeculation);
        GPRReg valueGPR = value.gpr();
        GPRTemporary temp(this);
        GPRReg tempGPR = temp.gpr();
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();
        JITCompiler::JumpList doneJumps;

        flushRegisters();
        
        ASSERT(node->child1().useKind() == UntypedUse || node->child1().useKind() == CellUse || node->child1().useKind() == StringUse);

        JITCompiler::Jump isNotCell = m_jit.branchTest64(JITCompiler::NonZero, valueGPR, GPRInfo::tagMaskRegister);
        if (node->child1().useKind() != UntypedUse)
            DFG_TYPE_CHECK(JSValueSource(valueGPR), node->child1(), SpecCell, isNotCell);

        if (!node->child1()->shouldSpeculateObject() || node->child1().useKind() == StringUse) {
            m_jit.loadPtr(JITCompiler::Address(valueGPR, JSCell::structureOffset()), tempGPR);
            JITCompiler::Jump notString = m_jit.branch8(JITCompiler::NotEqual, JITCompiler::Address(tempGPR, Structure::typeInfoTypeOffset()), TrustedImm32(StringType));
            if (node->child1().useKind() == StringUse)
                DFG_TYPE_CHECK(JSValueSource(valueGPR), node->child1(), SpecString, notString);
            m_jit.move(TrustedImmPtr(m_jit.vm()->smallStrings.stringString()), resultGPR);
            doneJumps.append(m_jit.jump());
            if (node->child1().useKind() != StringUse) {
                notString.link(&m_jit);
                callOperation(operationTypeOf, resultGPR, valueGPR);
                doneJumps.append(m_jit.jump());
            }
        } else {
            callOperation(operationTypeOf, resultGPR, valueGPR);
            doneJumps.append(m_jit.jump());
        }

        if (node->child1().useKind() == UntypedUse) {
            isNotCell.link(&m_jit);
            JITCompiler::Jump notNumber = m_jit.branchTest64(JITCompiler::Zero, valueGPR, GPRInfo::tagTypeNumberRegister);
            m_jit.move(TrustedImmPtr(m_jit.vm()->smallStrings.numberString()), resultGPR);
            doneJumps.append(m_jit.jump());
            notNumber.link(&m_jit);

            JITCompiler::Jump notUndefined = m_jit.branch64(JITCompiler::NotEqual, valueGPR, JITCompiler::TrustedImm64(ValueUndefined));
            m_jit.move(TrustedImmPtr(m_jit.vm()->smallStrings.undefinedString()), resultGPR);
            doneJumps.append(m_jit.jump());
            notUndefined.link(&m_jit);

            JITCompiler::Jump notNull = m_jit.branch64(JITCompiler::NotEqual, valueGPR, JITCompiler::TrustedImm64(ValueNull));
            m_jit.move(TrustedImmPtr(m_jit.vm()->smallStrings.objectString()), resultGPR);
            doneJumps.append(m_jit.jump());
            notNull.link(&m_jit);

            // Only boolean left
            m_jit.move(TrustedImmPtr(m_jit.vm()->smallStrings.booleanString()), resultGPR);
        }
        doneJumps.link(&m_jit);
        cellResult(resultGPR, node);
        break;
    }

    case Flush:
    case Phi:
        break;

    case Breakpoint:
#if ENABLE(DEBUG_WITH_BREAKPOINT)
        m_jit.breakpoint();
#else
        RELEASE_ASSERT_NOT_REACHED();
#endif
        break;
        
    case Call:
    case Construct:
        emitCall(node);
        break;

    case Resolve: {
        flushRegisters();
        GPRResult result(this);
        ResolveOperationData& data = m_jit.graph().m_resolveOperationsData[node->resolveOperationsDataIndex()];
        callOperation(operationResolve, result.gpr(), identifier(data.identifierNumber), data.resolveOperations);
        jsValueResult(result.gpr(), node);
        break;
    }

    case ResolveBase: {
        flushRegisters();
        GPRResult result(this);
        ResolveOperationData& data = m_jit.graph().m_resolveOperationsData[node->resolveOperationsDataIndex()];
        callOperation(operationResolveBase, result.gpr(), identifier(data.identifierNumber), data.resolveOperations, data.putToBaseOperation);
        jsValueResult(result.gpr(), node);
        break;
    }

    case ResolveBaseStrictPut: {
        flushRegisters();
        GPRResult result(this);
        ResolveOperationData& data = m_jit.graph().m_resolveOperationsData[node->resolveOperationsDataIndex()];
        callOperation(operationResolveBaseStrictPut, result.gpr(), identifier(data.identifierNumber), data.resolveOperations, data.putToBaseOperation);
        jsValueResult(result.gpr(), node);
        break;
    }

    case ResolveGlobal: {
        GPRTemporary globalObject(this);
        GPRTemporary resolveInfo(this);
        GPRTemporary result(this);

        GPRReg globalObjectGPR = globalObject.gpr();
        GPRReg resolveInfoGPR = resolveInfo.gpr();
        GPRReg resultGPR = result.gpr();

        ResolveGlobalData& data = m_jit.graph().m_resolveGlobalData[node->resolveGlobalDataIndex()];
        ResolveOperation* resolveOperationAddress = &(data.resolveOperations->data()[data.resolvePropertyIndex]);

        // Check Structure of global object
        m_jit.move(JITCompiler::TrustedImmPtr(m_jit.globalObjectFor(node->codeOrigin)), globalObjectGPR);
        m_jit.move(JITCompiler::TrustedImmPtr(resolveOperationAddress), resolveInfoGPR);
        m_jit.loadPtr(JITCompiler::Address(resolveInfoGPR, OBJECT_OFFSETOF(ResolveOperation, m_structure)), resultGPR);
        JITCompiler::Jump structuresDontMatch = m_jit.branchPtr(JITCompiler::NotEqual, resultGPR, JITCompiler::Address(globalObjectGPR, JSCell::structureOffset()));

        // Fast case
        m_jit.load32(JITCompiler::Address(resolveInfoGPR, OBJECT_OFFSETOF(ResolveOperation, m_offset)), resolveInfoGPR);
#if DFG_ENABLE(JIT_ASSERT)
        JITCompiler::Jump isOutOfLine = m_jit.branch32(JITCompiler::GreaterThanOrEqual, resolveInfoGPR, TrustedImm32(firstOutOfLineOffset));
        m_jit.breakpoint();
        isOutOfLine.link(&m_jit);
#endif
        m_jit.neg32(resolveInfoGPR);
        m_jit.signExtend32ToPtr(resolveInfoGPR, resolveInfoGPR);
        m_jit.loadPtr(JITCompiler::Address(globalObjectGPR, JSObject::butterflyOffset()), resultGPR);
        m_jit.load64(JITCompiler::BaseIndex(resultGPR, resolveInfoGPR, JITCompiler::TimesEight, (firstOutOfLineOffset - 2) * static_cast<ptrdiff_t>(sizeof(JSValue))), resultGPR);
        
        addSlowPathGenerator(
            slowPathCall(
                structuresDontMatch, this, operationResolveGlobal,
                resultGPR, resolveInfoGPR, globalObjectGPR,
                &m_jit.codeBlock()->identifier(data.identifierNumber)));

        jsValueResult(resultGPR, node);
        break;
    }
        
    case CreateActivation: {
        RELEASE_ASSERT(!node->codeOrigin.inlineCallFrame);
        
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this, value);
        
        GPRReg valueGPR = value.gpr();
        GPRReg resultGPR = result.gpr();
        
        m_jit.move(valueGPR, resultGPR);
        
        JITCompiler::Jump notCreated = m_jit.branchTest64(JITCompiler::Zero, resultGPR);
        
        addSlowPathGenerator(
            slowPathCall(notCreated, this, operationCreateActivation, resultGPR));
        
        cellResult(resultGPR, node);
        break;
    }
        
    case CreateArguments: {
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this, value);
        
        GPRReg valueGPR = value.gpr();
        GPRReg resultGPR = result.gpr();
        
        m_jit.move(valueGPR, resultGPR);
        
        JITCompiler::Jump notCreated = m_jit.branchTest64(JITCompiler::Zero, resultGPR);
        
        if (node->codeOrigin.inlineCallFrame) {
            addSlowPathGenerator(
                slowPathCall(
                    notCreated, this, operationCreateInlinedArguments, resultGPR,
                    node->codeOrigin.inlineCallFrame));
        } else {
            addSlowPathGenerator(
                slowPathCall(notCreated, this, operationCreateArguments, resultGPR));
        }
        
        cellResult(resultGPR, node);
        break;
    }

    case TearOffActivation: {
        RELEASE_ASSERT(!node->codeOrigin.inlineCallFrame);

        JSValueOperand activationValue(this, node->child1());
        GPRTemporary scratch(this);
        GPRReg activationValueGPR = activationValue.gpr();
        GPRReg scratchGPR = scratch.gpr();

        JITCompiler::Jump notCreated = m_jit.branchTest64(JITCompiler::Zero, activationValueGPR);

        SharedSymbolTable* symbolTable = m_jit.symbolTableFor(node->codeOrigin);
        int registersOffset = JSActivation::registersOffset(symbolTable);

        int captureEnd = symbolTable->captureEnd();
        for (int i = symbolTable->captureStart(); i < captureEnd; ++i) {
            m_jit.load64(
                JITCompiler::Address(
                    GPRInfo::callFrameRegister, i * sizeof(Register)), scratchGPR);
            m_jit.store64(
                scratchGPR, JITCompiler::Address(
                    activationValueGPR, registersOffset + i * sizeof(Register)));
        }
        m_jit.addPtr(TrustedImm32(registersOffset), activationValueGPR, scratchGPR);
        m_jit.storePtr(scratchGPR, JITCompiler::Address(activationValueGPR, JSActivation::offsetOfRegisters()));

        notCreated.link(&m_jit);
        noResult(node);
        break;
    }

    case TearOffArguments: {
        JSValueOperand unmodifiedArgumentsValue(this, node->child1());
        JSValueOperand activationValue(this, node->child2());
        GPRReg unmodifiedArgumentsValueGPR = unmodifiedArgumentsValue.gpr();
        GPRReg activationValueGPR = activationValue.gpr();

        JITCompiler::Jump created = m_jit.branchTest64(JITCompiler::NonZero, unmodifiedArgumentsValueGPR);

        if (node->codeOrigin.inlineCallFrame) {
            addSlowPathGenerator(
                slowPathCall(
                    created, this, operationTearOffInlinedArguments, NoResult,
                    unmodifiedArgumentsValueGPR, activationValueGPR, node->codeOrigin.inlineCallFrame));
        } else {
            addSlowPathGenerator(
                slowPathCall(
                    created, this, operationTearOffArguments, NoResult, unmodifiedArgumentsValueGPR, activationValueGPR));
        }
        
        noResult(node);
        break;
    }
        
    case GetMyArgumentsLength: {
        GPRTemporary result(this);
        GPRReg resultGPR = result.gpr();
        
        if (!isEmptySpeculation(
                m_state.variables().operand(
                    m_jit.graph().argumentsRegisterFor(node->codeOrigin)).m_type)) {
            speculationCheck(
                ArgumentsEscaped, JSValueRegs(), 0,
                m_jit.branchTest64(
                    JITCompiler::NonZero,
                    JITCompiler::addressFor(
                        m_jit.argumentsRegisterFor(node->codeOrigin))));
        }
        
        RELEASE_ASSERT(!node->codeOrigin.inlineCallFrame);
        m_jit.load32(JITCompiler::payloadFor(JSStack::ArgumentCount), resultGPR);
        m_jit.sub32(TrustedImm32(1), resultGPR);
        integerResult(resultGPR, node);
        break;
    }
        
    case GetMyArgumentsLengthSafe: {
        GPRTemporary result(this);
        GPRReg resultGPR = result.gpr();
        
        JITCompiler::Jump created = m_jit.branchTest64(
            JITCompiler::NonZero,
            JITCompiler::addressFor(
                m_jit.argumentsRegisterFor(node->codeOrigin)));
        
        if (node->codeOrigin.inlineCallFrame) {
            m_jit.move(
                Imm64(JSValue::encode(jsNumber(node->codeOrigin.inlineCallFrame->arguments.size() - 1))),
                resultGPR);
        } else {
            m_jit.load32(JITCompiler::payloadFor(JSStack::ArgumentCount), resultGPR);
            m_jit.sub32(TrustedImm32(1), resultGPR);
            m_jit.or64(GPRInfo::tagTypeNumberRegister, resultGPR);
        }
        
        // FIXME: the slow path generator should perform a forward speculation that the
        // result is an integer. For now we postpone the speculation by having this return
        // a JSValue.
        
        addSlowPathGenerator(
            slowPathCall(
                created, this, operationGetArgumentsLength, resultGPR,
                m_jit.argumentsRegisterFor(node->codeOrigin)));
        
        jsValueResult(resultGPR, node);
        break;
    }
        
    case GetMyArgumentByVal: {
        SpeculateStrictInt32Operand index(this, node->child1());
        GPRTemporary result(this);
        GPRReg indexGPR = index.gpr();
        GPRReg resultGPR = result.gpr();

        if (!isEmptySpeculation(
                m_state.variables().operand(
                    m_jit.graph().argumentsRegisterFor(node->codeOrigin)).m_type)) {
            speculationCheck(
                ArgumentsEscaped, JSValueRegs(), 0,
                m_jit.branchTest64(
                    JITCompiler::NonZero,
                    JITCompiler::addressFor(
                        m_jit.argumentsRegisterFor(node->codeOrigin))));
        }

        m_jit.add32(TrustedImm32(1), indexGPR, resultGPR);
        if (node->codeOrigin.inlineCallFrame) {
            speculationCheck(
                Uncountable, JSValueRegs(), 0,
                m_jit.branch32(
                    JITCompiler::AboveOrEqual,
                    resultGPR,
                    Imm32(node->codeOrigin.inlineCallFrame->arguments.size())));
        } else {
            speculationCheck(
                Uncountable, JSValueRegs(), 0,
                m_jit.branch32(
                    JITCompiler::AboveOrEqual,
                    resultGPR,
                    JITCompiler::payloadFor(JSStack::ArgumentCount)));
        }

        JITCompiler::JumpList slowArgument;
        JITCompiler::JumpList slowArgumentOutOfBounds;
        if (const SlowArgument* slowArguments = m_jit.symbolTableFor(node->codeOrigin)->slowArguments()) {
            slowArgumentOutOfBounds.append(
                m_jit.branch32(
                    JITCompiler::AboveOrEqual, indexGPR,
                    Imm32(m_jit.symbolTableFor(node->codeOrigin)->parameterCount())));

            COMPILE_ASSERT(sizeof(SlowArgument) == 8, SlowArgument_size_is_eight_bytes);
            m_jit.move(ImmPtr(slowArguments), resultGPR);
            m_jit.load32(
                JITCompiler::BaseIndex(
                    resultGPR, indexGPR, JITCompiler::TimesEight, 
                    OBJECT_OFFSETOF(SlowArgument, index)), 
                resultGPR);
            m_jit.signExtend32ToPtr(resultGPR, resultGPR);
            m_jit.load64(
                JITCompiler::BaseIndex(
                    GPRInfo::callFrameRegister, resultGPR, JITCompiler::TimesEight, m_jit.offsetOfLocals(node->codeOrigin)),
                resultGPR);
            slowArgument.append(m_jit.jump());
        }
        slowArgumentOutOfBounds.link(&m_jit);

        m_jit.neg32(resultGPR);
        m_jit.signExtend32ToPtr(resultGPR, resultGPR);
            
        m_jit.load64(
            JITCompiler::BaseIndex(
                GPRInfo::callFrameRegister, resultGPR, JITCompiler::TimesEight, m_jit.offsetOfArgumentsIncludingThis(node->codeOrigin)),
            resultGPR);

        slowArgument.link(&m_jit);
        jsValueResult(resultGPR, node);
        break;
    }
        
    case GetMyArgumentByValSafe: {
        SpeculateStrictInt32Operand index(this, node->child1());
        GPRTemporary result(this);
        GPRReg indexGPR = index.gpr();
        GPRReg resultGPR = result.gpr();
        
        JITCompiler::JumpList slowPath;
        slowPath.append(
            m_jit.branchTest64(
                JITCompiler::NonZero,
                JITCompiler::addressFor(
                    m_jit.argumentsRegisterFor(node->codeOrigin))));
        
        m_jit.add32(TrustedImm32(1), indexGPR, resultGPR);
        if (node->codeOrigin.inlineCallFrame) {
            slowPath.append(
                m_jit.branch32(
                    JITCompiler::AboveOrEqual,
                    resultGPR,
                    Imm32(node->codeOrigin.inlineCallFrame->arguments.size())));
        } else {
            slowPath.append(
                m_jit.branch32(
                    JITCompiler::AboveOrEqual,
                    resultGPR,
                    JITCompiler::payloadFor(JSStack::ArgumentCount)));
        }
        
        JITCompiler::JumpList slowArgument;
        JITCompiler::JumpList slowArgumentOutOfBounds;
        if (const SlowArgument* slowArguments = m_jit.symbolTableFor(node->codeOrigin)->slowArguments()) {
            slowArgumentOutOfBounds.append(
                m_jit.branch32(
                    JITCompiler::AboveOrEqual, indexGPR,
                    Imm32(m_jit.symbolTableFor(node->codeOrigin)->parameterCount())));

            COMPILE_ASSERT(sizeof(SlowArgument) == 8, SlowArgument_size_is_eight_bytes);
            m_jit.move(ImmPtr(slowArguments), resultGPR);
            m_jit.load32(
                JITCompiler::BaseIndex(
                    resultGPR, indexGPR, JITCompiler::TimesEight, 
                    OBJECT_OFFSETOF(SlowArgument, index)), 
                resultGPR);
            m_jit.signExtend32ToPtr(resultGPR, resultGPR);
            m_jit.load64(
                JITCompiler::BaseIndex(
                    GPRInfo::callFrameRegister, resultGPR, JITCompiler::TimesEight, m_jit.offsetOfLocals(node->codeOrigin)),
                resultGPR);
            slowArgument.append(m_jit.jump());
        }
        slowArgumentOutOfBounds.link(&m_jit);

        m_jit.neg32(resultGPR);
        m_jit.signExtend32ToPtr(resultGPR, resultGPR);
        
        m_jit.load64(
            JITCompiler::BaseIndex(
                GPRInfo::callFrameRegister, resultGPR, JITCompiler::TimesEight, m_jit.offsetOfArgumentsIncludingThis(node->codeOrigin)),
            resultGPR);
        
        if (node->codeOrigin.inlineCallFrame) {
            addSlowPathGenerator(
                slowPathCall(
                    slowPath, this, operationGetInlinedArgumentByVal, resultGPR, 
                    m_jit.argumentsRegisterFor(node->codeOrigin),
                    node->codeOrigin.inlineCallFrame,
                    indexGPR));
        } else {
            addSlowPathGenerator(
                slowPathCall(
                    slowPath, this, operationGetArgumentByVal, resultGPR, 
                    m_jit.argumentsRegisterFor(node->codeOrigin),
                    indexGPR));
        }
        
        slowArgument.link(&m_jit);
        jsValueResult(resultGPR, node);
        break;
    }
        
    case CheckArgumentsNotCreated: {
        ASSERT(!isEmptySpeculation(
            m_state.variables().operand(
                m_jit.graph().argumentsRegisterFor(node->codeOrigin)).m_type));
        speculationCheck(
            ArgumentsEscaped, JSValueRegs(), 0,
            m_jit.branchTest64(
                JITCompiler::NonZero,
                JITCompiler::addressFor(
                    m_jit.argumentsRegisterFor(node->codeOrigin))));
        noResult(node);
        break;
    }
        
    case NewFunctionNoCheck:
        compileNewFunctionNoCheck(node);
        break;
        
    case NewFunction: {
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this, value);
        
        GPRReg valueGPR = value.gpr();
        GPRReg resultGPR = result.gpr();
        
        m_jit.move(valueGPR, resultGPR);
        
        JITCompiler::Jump notCreated = m_jit.branchTest64(JITCompiler::Zero, resultGPR);
        
        addSlowPathGenerator(
            slowPathCall(
                notCreated, this, operationNewFunction,
                resultGPR, m_jit.codeBlock()->functionDecl(node->functionDeclIndex())));
        
        jsValueResult(resultGPR, node);
        break;
    }
        
    case NewFunctionExpression:
        compileNewFunctionExpression(node);
        break;
        
    case CountExecution:
        m_jit.add64(TrustedImm32(1), MacroAssembler::AbsoluteAddress(node->executionCounter()->address()));
        break;

    case GarbageValue:
        // We should never get to the point of code emission for a GarbageValue
        CRASH();
        break;

    case ForceOSRExit: {
        terminateSpeculativeExecution(InadequateCoverage, JSValueRegs(), 0);
        break;
    }

    case CheckWatchdogTimer:
        speculationCheck(
            WatchdogTimerFired, JSValueRegs(), 0,
            m_jit.branchTest8(
                JITCompiler::NonZero,
                JITCompiler::AbsoluteAddress(m_jit.vm()->watchdog.timerDidFireAddress())));
        break;

    case Phantom:
        DFG_NODE_DO_TO_CHILDREN(m_jit.graph(), node, speculate);
        noResult(node);
        break;
        
    case PhantomLocal:
        // This is a no-op.
        noResult(node);
        break;
        
    case Nop:
        RELEASE_ASSERT_NOT_REACHED();
        break;
        
    case LastNodeType:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }

#if ENABLE(DFG_REGISTER_ALLOCATION_VALIDATION)
    m_jit.clearRegisterAllocationOffsets();
#endif

    if (!m_compileOkay)
        return;
    
    if (node->hasResult() && node->mustGenerate())
        use(node);
}

#endif

} } // namespace JSC::DFG

#endif
