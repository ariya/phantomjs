/*
 * Copyright (C) 2011, 2012, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Intel Corporation. All rights reserved.
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

#include "ArrayPrototype.h"
#include "DFGCallArrayAllocatorSlowPathGenerator.h"
#include "DFGSlowPathGenerator.h"
#include "JSActivation.h"
#include "ObjectPrototype.h"
#include "Operations.h"

namespace JSC { namespace DFG {

#if USE(JSVALUE32_64)

GPRReg SpeculativeJIT::fillInteger(Edge edge, DataFormat& returnFormat)
{
    ASSERT(!needsTypeCheck(edge, SpecInt32));
    
    VirtualRegister virtualRegister = edge->virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    if (info.registerFormat() == DataFormatNone) {
        GPRReg gpr = allocate();

        if (edge->hasConstant()) {
            m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
            if (isInt32Constant(edge.node()))
                m_jit.move(MacroAssembler::Imm32(valueOfInt32Constant(edge.node())), gpr);
            else if (isNumberConstant(edge.node()))
                RELEASE_ASSERT_NOT_REACHED();
            else {
                ASSERT(isJSConstant(edge.node()));
                JSValue jsValue = valueOfJSConstant(edge.node());
                m_jit.move(MacroAssembler::Imm32(jsValue.payload()), gpr);
            }
        } else {
            ASSERT(info.spillFormat() == DataFormatJS || info.spillFormat() == DataFormatJSInteger || info.spillFormat() == DataFormatInteger);
            m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
            m_jit.load32(JITCompiler::payloadFor(virtualRegister), gpr);
        }

        info.fillInteger(*m_stream, gpr);
        returnFormat = DataFormatInteger;
        return gpr;
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
        GPRReg tagGPR = info.tagGPR();
        GPRReg payloadGPR = info.payloadGPR();
        m_gprs.lock(tagGPR);
        m_jit.jitAssertIsJSInt32(tagGPR);
        m_gprs.unlock(tagGPR);
        m_gprs.lock(payloadGPR);
        m_gprs.release(tagGPR);
        m_gprs.release(payloadGPR);
        m_gprs.retain(payloadGPR, virtualRegister, SpillOrderInteger);
        info.fillInteger(*m_stream, payloadGPR);
        returnFormat = DataFormatInteger;
        return payloadGPR;
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

bool SpeculativeJIT::fillJSValue(Edge edge, GPRReg& tagGPR, GPRReg& payloadGPR, FPRReg& fpr)
{
    // FIXME: For double we could fill with a FPR.
    UNUSED_PARAM(fpr);

    VirtualRegister virtualRegister = edge->virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    switch (info.registerFormat()) {
    case DataFormatNone: {

        if (edge->hasConstant()) {
            tagGPR = allocate();
            payloadGPR = allocate();
            m_jit.move(Imm32(valueOfJSConstant(edge.node()).tag()), tagGPR);
            m_jit.move(Imm32(valueOfJSConstant(edge.node()).payload()), payloadGPR);
            m_gprs.retain(tagGPR, virtualRegister, SpillOrderConstant);
            m_gprs.retain(payloadGPR, virtualRegister, SpillOrderConstant);
            info.fillJSValue(*m_stream, tagGPR, payloadGPR, isInt32Constant(edge.node()) ? DataFormatJSInteger : DataFormatJS);
        } else {
            DataFormat spillFormat = info.spillFormat();
            ASSERT(spillFormat != DataFormatNone && spillFormat != DataFormatStorage);
            tagGPR = allocate();
            payloadGPR = allocate();
            switch (spillFormat) {
            case DataFormatInteger:
                m_jit.move(TrustedImm32(JSValue::Int32Tag), tagGPR);
                spillFormat = DataFormatJSInteger; // This will be used as the new register format.
                break;
            case DataFormatCell:
                m_jit.move(TrustedImm32(JSValue::CellTag), tagGPR);
                spillFormat = DataFormatJSCell; // This will be used as the new register format.
                break;
            case DataFormatBoolean:
                m_jit.move(TrustedImm32(JSValue::BooleanTag), tagGPR);
                spillFormat = DataFormatJSBoolean; // This will be used as the new register format.
                break;
            default:
                m_jit.load32(JITCompiler::tagFor(virtualRegister), tagGPR);
                break;
            }
            m_jit.load32(JITCompiler::payloadFor(virtualRegister), payloadGPR);
            m_gprs.retain(tagGPR, virtualRegister, SpillOrderSpilled);
            m_gprs.retain(payloadGPR, virtualRegister, SpillOrderSpilled);
            info.fillJSValue(*m_stream, tagGPR, payloadGPR, spillFormat == DataFormatJSDouble ? DataFormatJS : spillFormat);
        }

        return true;
    }

    case DataFormatInteger:
    case DataFormatCell:
    case DataFormatBoolean: {
        GPRReg gpr = info.gpr();
        // If the register has already been locked we need to take a copy.
        if (m_gprs.isLocked(gpr)) {
            payloadGPR = allocate();
            m_jit.move(gpr, payloadGPR);
        } else {
            payloadGPR = gpr;
            m_gprs.lock(gpr);
        }
        tagGPR = allocate();
        uint32_t tag = JSValue::EmptyValueTag;
        DataFormat fillFormat = DataFormatJS;
        switch (info.registerFormat()) {
        case DataFormatInteger:
            tag = JSValue::Int32Tag;
            fillFormat = DataFormatJSInteger;
            break;
        case DataFormatCell:
            tag = JSValue::CellTag;
            fillFormat = DataFormatJSCell;
            break;
        case DataFormatBoolean:
            tag = JSValue::BooleanTag;
            fillFormat = DataFormatJSBoolean;
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        m_jit.move(TrustedImm32(tag), tagGPR);
        m_gprs.release(gpr);
        m_gprs.retain(tagGPR, virtualRegister, SpillOrderJS);
        m_gprs.retain(payloadGPR, virtualRegister, SpillOrderJS);
        info.fillJSValue(*m_stream, tagGPR, payloadGPR, fillFormat);
        return true;
    }

    case DataFormatJSDouble:
    case DataFormatDouble: {
        FPRReg oldFPR = info.fpr();
        m_fprs.lock(oldFPR);
        tagGPR = allocate();
        payloadGPR = allocate();
        boxDouble(oldFPR, tagGPR, payloadGPR);
        m_fprs.unlock(oldFPR);
        m_fprs.release(oldFPR);
        m_gprs.retain(tagGPR, virtualRegister, SpillOrderJS);
        m_gprs.retain(payloadGPR, virtualRegister, SpillOrderJS);
        info.fillJSValue(*m_stream, tagGPR, payloadGPR, DataFormatJS);
        return true;
    }

    case DataFormatJS:
    case DataFormatJSInteger:
    case DataFormatJSCell:
    case DataFormatJSBoolean: {
        tagGPR = info.tagGPR();
        payloadGPR = info.payloadGPR();
        m_gprs.lock(tagGPR);
        m_gprs.lock(payloadGPR);
        return true;
    }
        
    case DataFormatStorage:
        // this type currently never occurs
        RELEASE_ASSERT_NOT_REACHED();

    default:
        RELEASE_ASSERT_NOT_REACHED();
        return true;
    }
}

void SpeculativeJIT::nonSpeculativeUInt32ToNumber(Node* node)
{
    IntegerOperand op1(this, node->child1());
    FPRTemporary boxer(this);
    GPRTemporary resultTag(this, op1);
    GPRTemporary resultPayload(this);
        
    JITCompiler::Jump positive = m_jit.branch32(MacroAssembler::GreaterThanOrEqual, op1.gpr(), TrustedImm32(0));
        
    m_jit.convertInt32ToDouble(op1.gpr(), boxer.fpr());
    m_jit.move(JITCompiler::TrustedImmPtr(&AssemblyHelpers::twoToThe32), resultPayload.gpr()); // reuse resultPayload register here.
    m_jit.addDouble(JITCompiler::Address(resultPayload.gpr(), 0), boxer.fpr());
        
    boxDouble(boxer.fpr(), resultTag.gpr(), resultPayload.gpr());
        
    JITCompiler::Jump done = m_jit.jump();
        
    positive.link(&m_jit);
        
    m_jit.move(TrustedImm32(JSValue::Int32Tag), resultTag.gpr());
    m_jit.move(op1.gpr(), resultPayload.gpr());
        
    done.link(&m_jit);

    jsValueResult(resultTag.gpr(), resultPayload.gpr(), node);
}

void SpeculativeJIT::cachedGetById(CodeOrigin codeOrigin, GPRReg baseTagGPROrNone, GPRReg basePayloadGPR, GPRReg resultTagGPR, GPRReg resultPayloadGPR, unsigned identifierNumber, JITCompiler::Jump slowPathTarget, SpillRegistersMode spillMode)
{
    JITCompiler::DataLabelPtr structureToCompare;
    JITCompiler::PatchableJump structureCheck = m_jit.patchableBranchPtrWithPatch(JITCompiler::NotEqual, JITCompiler::Address(basePayloadGPR, JSCell::structureOffset()), structureToCompare, JITCompiler::TrustedImmPtr(reinterpret_cast<void*>(unusedPointer)));
    
    JITCompiler::ConvertibleLoadLabel propertyStorageLoad = m_jit.convertibleLoadPtr(JITCompiler::Address(basePayloadGPR, JSObject::butterflyOffset()), resultPayloadGPR);
    JITCompiler::DataLabelCompact tagLoadWithPatch = m_jit.load32WithCompactAddressOffsetPatch(JITCompiler::Address(resultPayloadGPR, OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)), resultTagGPR);
    JITCompiler::DataLabelCompact payloadLoadWithPatch = m_jit.load32WithCompactAddressOffsetPatch(JITCompiler::Address(resultPayloadGPR, OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)), resultPayloadGPR);
    
    JITCompiler::Label doneLabel = m_jit.label();

    OwnPtr<SlowPathGenerator> slowPath;
    if (baseTagGPROrNone == InvalidGPRReg) {
        if (!slowPathTarget.isSet()) {
            slowPath = slowPathCall(
                structureCheck.m_jump, this, operationGetByIdOptimize,
                JSValueRegs(resultTagGPR, resultPayloadGPR),
                static_cast<int32_t>(JSValue::CellTag), basePayloadGPR,
                identifier(identifierNumber));
        } else {
            JITCompiler::JumpList slowCases;
            slowCases.append(structureCheck.m_jump);
            slowCases.append(slowPathTarget);
            slowPath = slowPathCall(
                slowCases, this, operationGetByIdOptimize,
                JSValueRegs(resultTagGPR, resultPayloadGPR),
                static_cast<int32_t>(JSValue::CellTag), basePayloadGPR,
                identifier(identifierNumber));
        }
    } else {
        if (!slowPathTarget.isSet()) {
            slowPath = slowPathCall(
                structureCheck.m_jump, this, operationGetByIdOptimize,
                JSValueRegs(resultTagGPR, resultPayloadGPR), baseTagGPROrNone, basePayloadGPR,
                identifier(identifierNumber));
        } else {
            JITCompiler::JumpList slowCases;
            slowCases.append(structureCheck.m_jump);
            slowCases.append(slowPathTarget);
            slowPath = slowPathCall(
                slowCases, this, operationGetByIdOptimize,
                JSValueRegs(resultTagGPR, resultPayloadGPR), baseTagGPROrNone, basePayloadGPR,
                identifier(identifierNumber));
        }
    }
    m_jit.addPropertyAccess(
        PropertyAccessRecord(
            codeOrigin, structureToCompare, structureCheck, propertyStorageLoad,
            tagLoadWithPatch, payloadLoadWithPatch, slowPath.get(), doneLabel,
            safeCast<int8_t>(basePayloadGPR), safeCast<int8_t>(resultTagGPR),
            safeCast<int8_t>(resultPayloadGPR), usedRegisters(),
            spillMode == NeedToSpill ? PropertyAccessRecord::RegistersInUse : PropertyAccessRecord::RegistersFlushed));
    addSlowPathGenerator(slowPath.release());
}

void SpeculativeJIT::cachedPutById(CodeOrigin codeOrigin, GPRReg basePayloadGPR, GPRReg valueTagGPR, GPRReg valuePayloadGPR, Edge valueUse, GPRReg scratchGPR, unsigned identifierNumber, PutKind putKind, JITCompiler::Jump slowPathTarget)
{
    JITCompiler::DataLabelPtr structureToCompare;
    JITCompiler::PatchableJump structureCheck = m_jit.patchableBranchPtrWithPatch(JITCompiler::NotEqual, JITCompiler::Address(basePayloadGPR, JSCell::structureOffset()), structureToCompare, JITCompiler::TrustedImmPtr(reinterpret_cast<void*>(unusedPointer)));

    writeBarrier(basePayloadGPR, valueTagGPR, valueUse, WriteBarrierForPropertyAccess, scratchGPR);

    JITCompiler::ConvertibleLoadLabel propertyStorageLoad = m_jit.convertibleLoadPtr(JITCompiler::Address(basePayloadGPR, JSObject::butterflyOffset()), scratchGPR);
    JITCompiler::DataLabel32 tagStoreWithPatch = m_jit.store32WithAddressOffsetPatch(valueTagGPR, JITCompiler::Address(scratchGPR, OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
    JITCompiler::DataLabel32 payloadStoreWithPatch = m_jit.store32WithAddressOffsetPatch(valuePayloadGPR, JITCompiler::Address(scratchGPR, OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));

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
            structureCheck.m_jump, this, optimizedCall, NoResult, valueTagGPR, valuePayloadGPR,
            basePayloadGPR, identifier(identifierNumber));
    } else {
        JITCompiler::JumpList slowCases;
        slowCases.append(structureCheck.m_jump);
        slowCases.append(slowPathTarget);
        slowPath = slowPathCall(
            slowCases, this, optimizedCall, NoResult, valueTagGPR, valuePayloadGPR,
            basePayloadGPR, identifier(identifierNumber));
    }
    RegisterSet currentlyUsedRegisters = usedRegisters();
    currentlyUsedRegisters.clear(scratchGPR);
    ASSERT(currentlyUsedRegisters.get(basePayloadGPR));
    ASSERT(currentlyUsedRegisters.get(valueTagGPR));
    ASSERT(currentlyUsedRegisters.get(valuePayloadGPR));
    m_jit.addPropertyAccess(
        PropertyAccessRecord(
            codeOrigin, structureToCompare, structureCheck, propertyStorageLoad,
            JITCompiler::DataLabelCompact(tagStoreWithPatch.label()),
            JITCompiler::DataLabelCompact(payloadStoreWithPatch.label()),
            slowPath.get(), doneLabel, safeCast<int8_t>(basePayloadGPR),
            safeCast<int8_t>(valueTagGPR), safeCast<int8_t>(valuePayloadGPR),
            usedRegisters()));
    addSlowPathGenerator(slowPath.release());
}

void SpeculativeJIT::nonSpeculativeNonPeepholeCompareNull(Edge operand, bool invert)
{
    JSValueOperand arg(this, operand);
    GPRReg argTagGPR = arg.tagGPR();
    GPRReg argPayloadGPR = arg.payloadGPR();

    GPRTemporary resultPayload(this, arg, false);
    GPRReg resultPayloadGPR = resultPayload.gpr();

    JITCompiler::Jump notCell;
    JITCompiler::Jump notMasqueradesAsUndefined;   
    if (m_jit.graph().globalObjectFor(m_currentNode->codeOrigin)->masqueradesAsUndefinedWatchpoint()->isStillValid()) {
        if (!isKnownCell(operand.node()))
            notCell = m_jit.branch32(MacroAssembler::NotEqual, argTagGPR, TrustedImm32(JSValue::CellTag));

        m_jit.graph().globalObjectFor(m_currentNode->codeOrigin)->masqueradesAsUndefinedWatchpoint()->add(speculationWatchpoint());
        m_jit.move(invert ? TrustedImm32(1) : TrustedImm32(0), resultPayloadGPR);
        notMasqueradesAsUndefined = m_jit.jump();
    } else {
        GPRTemporary localGlobalObject(this);
        GPRTemporary remoteGlobalObject(this);

        if (!isKnownCell(operand.node()))
            notCell = m_jit.branch32(MacroAssembler::NotEqual, argTagGPR, TrustedImm32(JSValue::CellTag));

        m_jit.loadPtr(JITCompiler::Address(argPayloadGPR, JSCell::structureOffset()), resultPayloadGPR);
        JITCompiler::Jump isMasqueradesAsUndefined = m_jit.branchTest8(JITCompiler::NonZero, JITCompiler::Address(resultPayloadGPR, Structure::typeInfoFlagsOffset()), JITCompiler::TrustedImm32(MasqueradesAsUndefined));
        
        m_jit.move(invert ? TrustedImm32(1) : TrustedImm32(0), resultPayloadGPR);
        notMasqueradesAsUndefined = m_jit.jump();

        isMasqueradesAsUndefined.link(&m_jit);
        GPRReg localGlobalObjectGPR = localGlobalObject.gpr();
        GPRReg remoteGlobalObjectGPR = remoteGlobalObject.gpr();
        m_jit.move(JITCompiler::TrustedImmPtr(m_jit.graph().globalObjectFor(m_currentNode->codeOrigin)), localGlobalObjectGPR);
        m_jit.loadPtr(JITCompiler::Address(resultPayloadGPR, Structure::globalObjectOffset()), remoteGlobalObjectGPR);
        m_jit.compare32(invert ? JITCompiler::NotEqual : JITCompiler::Equal, localGlobalObjectGPR, remoteGlobalObjectGPR, resultPayloadGPR);
    }
 
    if (!isKnownCell(operand.node())) {
        JITCompiler::Jump done = m_jit.jump();
        
        notCell.link(&m_jit);
        // null or undefined?
        COMPILE_ASSERT((JSValue::UndefinedTag | 1) == JSValue::NullTag, UndefinedTag_OR_1_EQUALS_NullTag);
        m_jit.move(argTagGPR, resultPayloadGPR);
        m_jit.or32(TrustedImm32(1), resultPayloadGPR);
        m_jit.compare32(invert ? JITCompiler::NotEqual : JITCompiler::Equal, resultPayloadGPR, TrustedImm32(JSValue::NullTag), resultPayloadGPR);

        done.link(&m_jit);
    }
    
    notMasqueradesAsUndefined.link(&m_jit);
 
    booleanResult(resultPayloadGPR, m_currentNode);
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
    GPRReg argTagGPR = arg.tagGPR();
    GPRReg argPayloadGPR = arg.payloadGPR();
    
    GPRTemporary result(this, arg);
    GPRReg resultGPR = result.gpr();

    JITCompiler::Jump notCell;

    if (m_jit.graph().globalObjectFor(m_currentNode->codeOrigin)->masqueradesAsUndefinedWatchpoint()->isStillValid()) {
        if (!isKnownCell(operand.node()))
            notCell = m_jit.branch32(MacroAssembler::NotEqual, argTagGPR, TrustedImm32(JSValue::CellTag));

        m_jit.graph().globalObjectFor(m_currentNode->codeOrigin)->masqueradesAsUndefinedWatchpoint()->add(speculationWatchpoint());
        jump(invert ? taken : notTaken, ForceJump);
    } else {
        GPRTemporary localGlobalObject(this);
        GPRTemporary remoteGlobalObject(this);

        if (!isKnownCell(operand.node()))
            notCell = m_jit.branch32(MacroAssembler::NotEqual, argTagGPR, TrustedImm32(JSValue::CellTag));

        m_jit.loadPtr(JITCompiler::Address(argPayloadGPR, JSCell::structureOffset()), resultGPR);
        branchTest8(JITCompiler::Zero, JITCompiler::Address(resultGPR, Structure::typeInfoFlagsOffset()), JITCompiler::TrustedImm32(MasqueradesAsUndefined), invert ? taken : notTaken);
   
        GPRReg localGlobalObjectGPR = localGlobalObject.gpr();
        GPRReg remoteGlobalObjectGPR = remoteGlobalObject.gpr();
        m_jit.move(TrustedImmPtr(m_jit.graph().globalObjectFor(m_currentNode->codeOrigin)), localGlobalObjectGPR);
        m_jit.loadPtr(JITCompiler::Address(resultGPR, Structure::globalObjectOffset()), remoteGlobalObjectGPR);
        branchPtr(JITCompiler::Equal, localGlobalObjectGPR, remoteGlobalObjectGPR, invert ? notTaken : taken);
    }
 
    if (!isKnownCell(operand.node())) {
        jump(notTaken, ForceJump);
        
        notCell.link(&m_jit);
        // null or undefined?
        COMPILE_ASSERT((JSValue::UndefinedTag | 1) == JSValue::NullTag, UndefinedTag_OR_1_EQUALS_NullTag);
        m_jit.move(argTagGPR, resultGPR);
        m_jit.or32(TrustedImm32(1), resultGPR);
        branch32(invert ? JITCompiler::NotEqual : JITCompiler::Equal, resultGPR, JITCompiler::TrustedImm32(JSValue::NullTag), taken);
    }
    
    jump(notTaken);
}

bool SpeculativeJIT::nonSpeculativeCompareNull(Node* node, Edge operand, bool invert)
{
    unsigned branchIndexInBlock = detectPeepHoleBranch();
    if (branchIndexInBlock != UINT_MAX) {
        Node* branchNode = m_jit.graph().m_blocks[m_block]->at(branchIndexInBlock);

        ASSERT(node->adjustedRefCount() == 1);
        
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
    GPRReg arg1TagGPR = arg1.tagGPR();
    GPRReg arg1PayloadGPR = arg1.payloadGPR();
    GPRReg arg2TagGPR = arg2.tagGPR();
    GPRReg arg2PayloadGPR = arg2.payloadGPR();
    
    JITCompiler::JumpList slowPath;
    
    if (isKnownNotInteger(node->child1().node()) || isKnownNotInteger(node->child2().node())) {
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();

        arg1.use();
        arg2.use();

        flushRegisters();
        callOperation(helperFunction, resultGPR, arg1TagGPR, arg1PayloadGPR, arg2TagGPR, arg2PayloadGPR);

        branchTest32(callResultCondition, resultGPR, taken);
    } else {
        GPRTemporary result(this);
        GPRReg resultGPR = result.gpr();
    
        arg1.use();
        arg2.use();

        if (!isKnownInteger(node->child1().node()))
            slowPath.append(m_jit.branch32(MacroAssembler::NotEqual, arg1TagGPR, JITCompiler::TrustedImm32(JSValue::Int32Tag)));
        if (!isKnownInteger(node->child2().node()))
            slowPath.append(m_jit.branch32(MacroAssembler::NotEqual, arg2TagGPR, JITCompiler::TrustedImm32(JSValue::Int32Tag)));
    
        branch32(cond, arg1PayloadGPR, arg2PayloadGPR, taken);
    
        if (!isKnownInteger(node->child1().node()) || !isKnownInteger(node->child2().node())) {
            jump(notTaken, ForceJump);
    
            slowPath.link(&m_jit);
    
            silentSpillAllRegisters(resultGPR);
            callOperation(helperFunction, resultGPR, arg1TagGPR, arg1PayloadGPR, arg2TagGPR, arg2PayloadGPR);
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
        S_DFGOperation_EJJ function, GPRReg result, GPRReg arg1Tag, GPRReg arg1Payload,
        GPRReg arg2Tag, GPRReg arg2Payload)
        : CallSlowPathGenerator<JumpType, S_DFGOperation_EJJ, GPRReg>(
            from, jit, function, NeedToSpill, result)
        , m_arg1Tag(arg1Tag)
        , m_arg1Payload(arg1Payload)
        , m_arg2Tag(arg2Tag)
        , m_arg2Payload(arg2Payload)
    {
    }
    
protected:
    virtual void generateInternal(SpeculativeJIT* jit)
    {
        this->setUp(jit);
        this->recordCall(
            jit->callOperation(
                this->m_function, this->m_result, m_arg1Tag, m_arg1Payload, m_arg2Tag,
                m_arg2Payload));
        jit->m_jit.and32(JITCompiler::TrustedImm32(1), this->m_result);
        this->tearDown(jit);
    }
   
private:
    GPRReg m_arg1Tag;
    GPRReg m_arg1Payload;
    GPRReg m_arg2Tag;
    GPRReg m_arg2Payload;
};

void SpeculativeJIT::nonSpeculativeNonPeepholeCompare(Node* node, MacroAssembler::RelationalCondition cond, S_DFGOperation_EJJ helperFunction)
{
    JSValueOperand arg1(this, node->child1());
    JSValueOperand arg2(this, node->child2());
    GPRReg arg1TagGPR = arg1.tagGPR();
    GPRReg arg1PayloadGPR = arg1.payloadGPR();
    GPRReg arg2TagGPR = arg2.tagGPR();
    GPRReg arg2PayloadGPR = arg2.payloadGPR();
    
    JITCompiler::JumpList slowPath;
    
    if (isKnownNotInteger(node->child1().node()) || isKnownNotInteger(node->child2().node())) {
        GPRResult result(this);
        GPRReg resultPayloadGPR = result.gpr();
    
        arg1.use();
        arg2.use();

        flushRegisters();
        callOperation(helperFunction, resultPayloadGPR, arg1TagGPR, arg1PayloadGPR, arg2TagGPR, arg2PayloadGPR);
        
        booleanResult(resultPayloadGPR, node, UseChildrenCalledExplicitly);
    } else {
        GPRTemporary resultPayload(this, arg1, false);
        GPRReg resultPayloadGPR = resultPayload.gpr();

        arg1.use();
        arg2.use();
    
        if (!isKnownInteger(node->child1().node()))
            slowPath.append(m_jit.branch32(MacroAssembler::NotEqual, arg1TagGPR, JITCompiler::TrustedImm32(JSValue::Int32Tag)));
        if (!isKnownInteger(node->child2().node()))
            slowPath.append(m_jit.branch32(MacroAssembler::NotEqual, arg2TagGPR, JITCompiler::TrustedImm32(JSValue::Int32Tag)));

        m_jit.compare32(cond, arg1PayloadGPR, arg2PayloadGPR, resultPayloadGPR);
    
        if (!isKnownInteger(node->child1().node()) || !isKnownInteger(node->child2().node())) {
            addSlowPathGenerator(adoptPtr(
                new CompareAndBoxBooleanSlowPathGenerator<JITCompiler::JumpList>(
                    slowPath, this, helperFunction, resultPayloadGPR, arg1TagGPR,
                    arg1PayloadGPR, arg2TagGPR, arg2PayloadGPR)));
        }
        
        booleanResult(resultPayloadGPR, node, UseChildrenCalledExplicitly);
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
    GPRReg arg1TagGPR = arg1.tagGPR();
    GPRReg arg1PayloadGPR = arg1.payloadGPR();
    GPRReg arg2TagGPR = arg2.tagGPR();
    GPRReg arg2PayloadGPR = arg2.payloadGPR();
    
    GPRTemporary resultPayload(this, arg1, false);
    GPRReg resultPayloadGPR = resultPayload.gpr();
    
    arg1.use();
    arg2.use();
    
    if (isKnownCell(node->child1().node()) && isKnownCell(node->child2().node())) {
        // see if we get lucky: if the arguments are cells and they reference the same
        // cell, then they must be strictly equal.
        branchPtr(JITCompiler::Equal, arg1PayloadGPR, arg2PayloadGPR, invert ? notTaken : taken);
        
        silentSpillAllRegisters(resultPayloadGPR);
        callOperation(operationCompareStrictEqCell, resultPayloadGPR, arg1TagGPR, arg1PayloadGPR, arg2TagGPR, arg2PayloadGPR);
        silentFillAllRegisters(resultPayloadGPR);
        
        branchTest32(invert ? JITCompiler::Zero : JITCompiler::NonZero, resultPayloadGPR, taken);
    } else {
        // FIXME: Add fast paths for twoCells, number etc.

        silentSpillAllRegisters(resultPayloadGPR);
        callOperation(operationCompareStrictEq, resultPayloadGPR, arg1TagGPR, arg1PayloadGPR, arg2TagGPR, arg2PayloadGPR);
        silentFillAllRegisters(resultPayloadGPR);
        
        branchTest32(invert ? JITCompiler::Zero : JITCompiler::NonZero, resultPayloadGPR, taken);
    }
    
    jump(notTaken);
}

void SpeculativeJIT::nonSpeculativeNonPeepholeStrictEq(Node* node, bool invert)
{
    JSValueOperand arg1(this, node->child1());
    JSValueOperand arg2(this, node->child2());
    GPRReg arg1TagGPR = arg1.tagGPR();
    GPRReg arg1PayloadGPR = arg1.payloadGPR();
    GPRReg arg2TagGPR = arg2.tagGPR();
    GPRReg arg2PayloadGPR = arg2.payloadGPR();
    
    GPRTemporary resultPayload(this, arg1, false);
    GPRReg resultPayloadGPR = resultPayload.gpr();
    
    arg1.use();
    arg2.use();
    
    if (isKnownCell(node->child1().node()) && isKnownCell(node->child2().node())) {
        // see if we get lucky: if the arguments are cells and they reference the same
        // cell, then they must be strictly equal.
        // FIXME: this should flush registers instead of silent spill/fill.
        JITCompiler::Jump notEqualCase = m_jit.branchPtr(JITCompiler::NotEqual, arg1PayloadGPR, arg2PayloadGPR);
        
        m_jit.move(JITCompiler::TrustedImm32(!invert), resultPayloadGPR);
        JITCompiler::Jump done = m_jit.jump();

        notEqualCase.link(&m_jit);
        
        silentSpillAllRegisters(resultPayloadGPR);
        callOperation(operationCompareStrictEqCell, resultPayloadGPR, arg1TagGPR, arg1PayloadGPR, arg2TagGPR, arg2PayloadGPR);
        silentFillAllRegisters(resultPayloadGPR);
        
        m_jit.andPtr(JITCompiler::TrustedImm32(1), resultPayloadGPR);
        
        done.link(&m_jit);
    } else {
        // FIXME: Add fast paths.

        silentSpillAllRegisters(resultPayloadGPR);
        callOperation(operationCompareStrictEq, resultPayloadGPR, arg1TagGPR, arg1PayloadGPR, arg2TagGPR, arg2PayloadGPR);
        silentFillAllRegisters(resultPayloadGPR);
        
        m_jit.andPtr(JITCompiler::TrustedImm32(1), resultPayloadGPR);
    }

    booleanResult(resultPayloadGPR, node, UseChildrenCalledExplicitly);
}

void SpeculativeJIT::emitCall(Node* node)
{
    if (node->op() != Call)
        ASSERT(node->op() == Construct);

    // For constructors, the this argument is not passed but we have to make space
    // for it.
    int dummyThisArgument = node->op() == Call ? 0 : 1;

    CallLinkInfo::CallType callType = node->op() == Call ? CallLinkInfo::Call : CallLinkInfo::Construct;

    Edge calleeEdge = m_jit.graph().m_varArgChildren[node->firstChild()];
    JSValueOperand callee(this, calleeEdge);
    GPRReg calleeTagGPR = callee.tagGPR();
    GPRReg calleePayloadGPR = callee.payloadGPR();
    use(calleeEdge);

    // The call instruction's first child is either the function (normal call) or the
    // receiver (method call). subsequent children are the arguments.
    int numPassedArgs = node->numChildren() - 1;

    m_jit.store32(MacroAssembler::TrustedImm32(numPassedArgs + dummyThisArgument), callFramePayloadSlot(JSStack::ArgumentCount));
    m_jit.storePtr(GPRInfo::callFrameRegister, callFramePayloadSlot(JSStack::CallerFrame));
    m_jit.store32(calleePayloadGPR, callFramePayloadSlot(JSStack::Callee));
    m_jit.store32(calleeTagGPR, callFrameTagSlot(JSStack::Callee));

    for (int i = 0; i < numPassedArgs; i++) {
        Edge argEdge = m_jit.graph().m_varArgChildren[node->firstChild() + 1 + i];
        JSValueOperand arg(this, argEdge);
        GPRReg argTagGPR = arg.tagGPR();
        GPRReg argPayloadGPR = arg.payloadGPR();
        use(argEdge);

        m_jit.store32(argTagGPR, argumentTagSlot(i + dummyThisArgument));
        m_jit.store32(argPayloadGPR, argumentPayloadSlot(i + dummyThisArgument));
    }

    flushRegisters();

    GPRResult resultPayload(this);
    GPRResult2 resultTag(this);
    GPRReg resultPayloadGPR = resultPayload.gpr();
    GPRReg resultTagGPR = resultTag.gpr();

    JITCompiler::DataLabelPtr targetToCheck;
    JITCompiler::JumpList slowPath;

    CallBeginToken token;
    m_jit.beginCall(node->codeOrigin, token);
    
    m_jit.addPtr(TrustedImm32(m_jit.codeBlock()->m_numCalleeRegisters * sizeof(Register)), GPRInfo::callFrameRegister);
    
    slowPath.append(m_jit.branch32(MacroAssembler::NotEqual, calleeTagGPR, TrustedImm32(JSValue::CellTag)));
    slowPath.append(m_jit.branchPtrWithPatch(MacroAssembler::NotEqual, calleePayloadGPR, targetToCheck));
    m_jit.loadPtr(MacroAssembler::Address(calleePayloadGPR, OBJECT_OFFSETOF(JSFunction, m_scope)), resultPayloadGPR);
    m_jit.storePtr(resultPayloadGPR, MacroAssembler::Address(GPRInfo::callFrameRegister, static_cast<ptrdiff_t>(sizeof(Register)) * JSStack::ScopeChain + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
    m_jit.store32(MacroAssembler::TrustedImm32(JSValue::CellTag), MacroAssembler::Address(GPRInfo::callFrameRegister, static_cast<ptrdiff_t>(sizeof(Register)) * JSStack::ScopeChain + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));

    CodeOrigin codeOrigin = node->codeOrigin;
    JITCompiler::Call fastCall = m_jit.nearCall();
    m_jit.notifyCall(fastCall, codeOrigin, token);

    JITCompiler::Jump done = m_jit.jump();

    slowPath.link(&m_jit);

    if (calleeTagGPR == GPRInfo::nonArgGPR0) {
        if (calleePayloadGPR == GPRInfo::nonArgGPR1)
            m_jit.swap(GPRInfo::nonArgGPR1, GPRInfo::nonArgGPR0);
        else {
            m_jit.move(calleeTagGPR, GPRInfo::nonArgGPR1);
            m_jit.move(calleePayloadGPR, GPRInfo::nonArgGPR0);
        }
    } else {
        m_jit.move(calleePayloadGPR, GPRInfo::nonArgGPR0);
        m_jit.move(calleeTagGPR, GPRInfo::nonArgGPR1);
    }
    m_jit.prepareForExceptionCheck();
    JITCompiler::Call slowCall = m_jit.nearCall();
    m_jit.notifyCall(slowCall, codeOrigin, token);

    done.link(&m_jit);

    m_jit.setupResults(resultPayloadGPR, resultTagGPR);

    jsValueResult(resultTagGPR, resultPayloadGPR, node, DataFormatJS, UseChildrenCalledExplicitly);

    m_jit.addJSCall(fastCall, slowCall, targetToCheck, callType, calleePayloadGPR, node->codeOrigin);
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
        
        if (edge->hasConstant()) {
            ASSERT(isInt32Constant(edge.node()));
            GPRReg gpr = allocate();
            m_jit.move(MacroAssembler::Imm32(valueOfInt32Constant(edge.node())), gpr);
            m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
            info.fillInteger(*m_stream, gpr);
            returnFormat = DataFormatInteger;
            return gpr;
        }

        DataFormat spillFormat = info.spillFormat();
        ASSERT_UNUSED(spillFormat, (spillFormat & DataFormatJS) || spillFormat == DataFormatInteger);

        // If we know this was spilled as an integer we can fill without checking.
        if (type & ~SpecInt32)
            speculationCheck(BadType, JSValueSource(JITCompiler::addressFor(virtualRegister)), edge, m_jit.branch32(MacroAssembler::NotEqual, JITCompiler::tagFor(virtualRegister), TrustedImm32(JSValue::Int32Tag)));

        GPRReg gpr = allocate();
        m_jit.load32(JITCompiler::payloadFor(virtualRegister), gpr);
        m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
        info.fillInteger(*m_stream, gpr);
        returnFormat = DataFormatInteger;
        return gpr;
    }

    case DataFormatJSInteger:
    case DataFormatJS: {
        // Check the value is an integer.
        GPRReg tagGPR = info.tagGPR();
        GPRReg payloadGPR = info.payloadGPR();
        m_gprs.lock(tagGPR);
        m_gprs.lock(payloadGPR);
        if (type & ~SpecInt32)
            speculationCheck(BadType, JSValueRegs(tagGPR, payloadGPR), edge, m_jit.branch32(MacroAssembler::NotEqual, tagGPR, TrustedImm32(JSValue::Int32Tag)));
        m_gprs.unlock(tagGPR);
        m_gprs.release(tagGPR);
        m_gprs.release(payloadGPR);
        m_gprs.retain(payloadGPR, virtualRegister, SpillOrderInteger);
        info.fillInteger(*m_stream, payloadGPR);
        // If !strict we're done, return.
        returnFormat = DataFormatInteger;
        return payloadGPR;
    }

    case DataFormatInteger: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        returnFormat = DataFormatInteger;
        return gpr;
    }

    case DataFormatDouble:
    case DataFormatCell:
    case DataFormatBoolean:
    case DataFormatJSDouble:
    case DataFormatJSCell:
    case DataFormatJSBoolean:
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
        returnFormat = DataFormatInteger;
        return allocate();

    case DataFormatStorage:
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
    ASSERT(mustBeDataFormatInteger == DataFormatInteger);
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
            if (isInt32Constant(edge.node())) {
                GPRReg gpr = allocate();
                m_jit.move(MacroAssembler::Imm32(valueOfInt32Constant(edge.node())), gpr);
                m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
                info.fillInteger(*m_stream, gpr);
                unlock(gpr);
            } else if (isNumberConstant(edge.node())) {
                FPRReg fpr = fprAllocate();
                m_jit.loadDouble(addressOfDoubleConstant(edge.node()), fpr);
                m_fprs.retain(fpr, virtualRegister, SpillOrderConstant);
                info.fillDouble(*m_stream, fpr);
                return fpr;
            } else {
                terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
                return fprAllocate();
            }
        } else {
            DataFormat spillFormat = info.spillFormat();
            ASSERT((spillFormat & DataFormatJS) || spillFormat == DataFormatInteger);
            if (spillFormat == DataFormatJSDouble || spillFormat == DataFormatDouble) {
                FPRReg fpr = fprAllocate();
                m_jit.loadDouble(JITCompiler::addressFor(virtualRegister), fpr);
                m_fprs.retain(fpr, virtualRegister, SpillOrderSpilled);
                info.fillDouble(*m_stream, fpr);
                return fpr;
            }

            FPRReg fpr = fprAllocate();
            JITCompiler::Jump hasUnboxedDouble;

            if (spillFormat != DataFormatJSInteger && spillFormat != DataFormatInteger) {
                JITCompiler::Jump isInteger = m_jit.branch32(MacroAssembler::Equal, JITCompiler::tagFor(virtualRegister), TrustedImm32(JSValue::Int32Tag));
                if (type & ~SpecNumber)
                    speculationCheck(BadType, JSValueSource(JITCompiler::addressFor(virtualRegister)), edge, m_jit.branch32(MacroAssembler::AboveOrEqual, JITCompiler::tagFor(virtualRegister), TrustedImm32(JSValue::LowestTag)));
                m_jit.loadDouble(JITCompiler::addressFor(virtualRegister), fpr);
                hasUnboxedDouble = m_jit.jump();

                isInteger.link(&m_jit);
            }

            m_jit.convertInt32ToDouble(JITCompiler::payloadFor(virtualRegister), fpr);

            if (hasUnboxedDouble.isSet())
                hasUnboxedDouble.link(&m_jit);

            m_fprs.retain(fpr, virtualRegister, SpillOrderSpilled);
            info.fillDouble(*m_stream, fpr);
            info.killSpilled();
            return fpr;
        }
    }

    switch (info.registerFormat()) {
    case DataFormatJS:
    case DataFormatJSInteger: {
        GPRReg tagGPR = info.tagGPR();
        GPRReg payloadGPR = info.payloadGPR();
        FPRReg fpr = fprAllocate();

        m_gprs.lock(tagGPR);
        m_gprs.lock(payloadGPR);

        JITCompiler::Jump hasUnboxedDouble;

        if (info.registerFormat() != DataFormatJSInteger) {
            FPRTemporary scratch(this);
            JITCompiler::Jump isInteger = m_jit.branch32(MacroAssembler::Equal, tagGPR, TrustedImm32(JSValue::Int32Tag));
            if (type & ~SpecNumber)
                speculationCheck(BadType, JSValueRegs(tagGPR, payloadGPR), edge, m_jit.branch32(MacroAssembler::AboveOrEqual, tagGPR, TrustedImm32(JSValue::LowestTag)));
            unboxDouble(tagGPR, payloadGPR, fpr, scratch.fpr());
            hasUnboxedDouble = m_jit.jump();
            isInteger.link(&m_jit);
        }

        m_jit.convertInt32ToDouble(payloadGPR, fpr);

        if (hasUnboxedDouble.isSet())
            hasUnboxedDouble.link(&m_jit);

        m_gprs.release(tagGPR);
        m_gprs.release(payloadGPR);
        m_gprs.unlock(tagGPR);
        m_gprs.unlock(payloadGPR);
        m_fprs.retain(fpr, virtualRegister, SpillOrderDouble);
        info.fillDouble(*m_stream, fpr);
        info.killSpilled();
        return fpr;
    }

    case DataFormatInteger: {
        FPRReg fpr = fprAllocate();
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        m_jit.convertInt32ToDouble(gpr, fpr);
        m_gprs.unlock(gpr);
        return fpr;
    }

    case DataFormatJSDouble:
    case DataFormatDouble: {
        FPRReg fpr = info.fpr();
        m_fprs.lock(fpr);
        return fpr;
    }

    case DataFormatNone:
    case DataFormatStorage:
        RELEASE_ASSERT_NOT_REACHED();

    case DataFormatCell:
    case DataFormatJSCell:
    case DataFormatBoolean:
    case DataFormatJSBoolean:
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
        return fprAllocate();

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

        if (edge->hasConstant()) {
            JSValue jsValue = valueOfJSConstant(edge.node());
            GPRReg gpr = allocate();
            if (jsValue.isCell()) {
                m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
                m_jit.move(MacroAssembler::TrustedImmPtr(jsValue.asCell()), gpr);
                info.fillCell(*m_stream, gpr);
                return gpr;
            }
            terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
            return gpr;
        }

        ASSERT((info.spillFormat() & DataFormatJS) || info.spillFormat() == DataFormatCell);
        if (type & ~SpecCell)
            speculationCheck(BadType, JSValueSource(JITCompiler::addressFor(virtualRegister)), edge, m_jit.branch32(MacroAssembler::NotEqual, JITCompiler::tagFor(virtualRegister), TrustedImm32(JSValue::CellTag)));
        GPRReg gpr = allocate();
        m_jit.load32(JITCompiler::payloadFor(virtualRegister), gpr);
        m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
        info.fillCell(*m_stream, gpr);
        return gpr;
    }

    case DataFormatCell: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        return gpr;
    }

    case DataFormatJSCell:
    case DataFormatJS: {
        GPRReg tagGPR = info.tagGPR();
        GPRReg payloadGPR = info.payloadGPR();
        m_gprs.lock(tagGPR);
        m_gprs.lock(payloadGPR);
        if (type & ~SpecCell)
            speculationCheck(BadType, JSValueRegs(tagGPR, payloadGPR), edge, m_jit.branch32(MacroAssembler::NotEqual, tagGPR, TrustedImm32(JSValue::CellTag)));
        m_gprs.unlock(tagGPR);
        m_gprs.release(tagGPR);
        m_gprs.release(payloadGPR);
        m_gprs.retain(payloadGPR, virtualRegister, SpillOrderCell);
        info.fillCell(*m_stream, payloadGPR);
        return payloadGPR;
    }

    case DataFormatJSInteger:
    case DataFormatInteger:
    case DataFormatJSDouble:
    case DataFormatDouble:
    case DataFormatJSBoolean:
    case DataFormatBoolean:
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
        return allocate();

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
    dataLogF("SpecBool@%d   ", edge.node()->index());
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
        
        if (edge->hasConstant()) {
            JSValue jsValue = valueOfJSConstant(edge.node());
            GPRReg gpr = allocate();
            if (jsValue.isBoolean()) {
                m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
                m_jit.move(MacroAssembler::TrustedImm32(jsValue.asBoolean()), gpr);
                info.fillBoolean(*m_stream, gpr);
                return gpr;
            }
            terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
            return gpr;
        }

        ASSERT((info.spillFormat() & DataFormatJS) || info.spillFormat() == DataFormatBoolean);

        if (type & ~SpecBoolean)
            speculationCheck(BadType, JSValueSource(JITCompiler::addressFor(virtualRegister)), edge, m_jit.branch32(MacroAssembler::NotEqual, JITCompiler::tagFor(virtualRegister), TrustedImm32(JSValue::BooleanTag)));

        GPRReg gpr = allocate();
        m_jit.load32(JITCompiler::payloadFor(virtualRegister), gpr);
        m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
        info.fillBoolean(*m_stream, gpr);
        return gpr;
    }

    case DataFormatBoolean: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        return gpr;
    }

    case DataFormatJSBoolean:
    case DataFormatJS: {
        GPRReg tagGPR = info.tagGPR();
        GPRReg payloadGPR = info.payloadGPR();
        m_gprs.lock(tagGPR);
        m_gprs.lock(payloadGPR);
        if (type & ~SpecBoolean)
            speculationCheck(BadType, JSValueRegs(tagGPR, payloadGPR), edge, m_jit.branch32(MacroAssembler::NotEqual, tagGPR, TrustedImm32(JSValue::BooleanTag)));

        m_gprs.unlock(tagGPR);
        m_gprs.release(tagGPR);
        m_gprs.release(payloadGPR);
        m_gprs.retain(payloadGPR, virtualRegister, SpillOrderBoolean);
        info.fillBoolean(*m_stream, payloadGPR);
        return payloadGPR;
    }

    case DataFormatJSInteger:
    case DataFormatInteger:
    case DataFormatJSDouble:
    case DataFormatDouble:
    case DataFormatJSCell:
    case DataFormatCell:
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
        return allocate();

    case DataFormatStorage:
        RELEASE_ASSERT_NOT_REACHED();

    default:
        RELEASE_ASSERT_NOT_REACHED();
        return InvalidGPRReg;
    }
}

JITCompiler::Jump SpeculativeJIT::convertToDouble(JSValueOperand& op, FPRReg result)
{
    FPRTemporary scratch(this);

    GPRReg opPayloadGPR = op.payloadGPR();
    GPRReg opTagGPR = op.tagGPR();
    FPRReg scratchFPR = scratch.fpr();

    JITCompiler::Jump isInteger = m_jit.branch32(MacroAssembler::Equal, opTagGPR, TrustedImm32(JSValue::Int32Tag));
    JITCompiler::Jump notNumber = m_jit.branch32(MacroAssembler::AboveOrEqual, opPayloadGPR, TrustedImm32(JSValue::LowestTag));

    unboxDouble(opTagGPR, opPayloadGPR, result, scratchFPR);
    JITCompiler::Jump done = m_jit.jump();

    isInteger.link(&m_jit);
    m_jit.convertInt32ToDouble(opPayloadGPR, result);

    done.link(&m_jit);

    return notNumber;
}

void SpeculativeJIT::compileObjectEquality(Node* node)
{
    SpeculateCellOperand op1(this, node->child1());
    SpeculateCellOperand op2(this, node->child2());
    GPRReg op1GPR = op1.gpr();
    GPRReg op2GPR = op2.gpr();
    
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
    
    GPRTemporary resultPayload(this, op2);
    GPRReg resultPayloadGPR = resultPayload.gpr();
    
    MacroAssembler::Jump falseCase = m_jit.branchPtr(MacroAssembler::NotEqual, op1GPR, op2GPR);
    m_jit.move(TrustedImm32(1), resultPayloadGPR);
    MacroAssembler::Jump done = m_jit.jump();
    falseCase.link(&m_jit);
    m_jit.move(TrustedImm32(0), resultPayloadGPR);
    done.link(&m_jit);

    booleanResult(resultPayloadGPR, node);
}

void SpeculativeJIT::compileObjectToObjectOrOtherEquality(Edge leftChild, Edge rightChild)
{
    SpeculateCellOperand op1(this, leftChild);
    JSValueOperand op2(this, rightChild, ManualOperandSpeculation);
    GPRTemporary result(this);
    
    GPRReg op1GPR = op1.gpr();
    GPRReg op2TagGPR = op2.tagGPR();
    GPRReg op2PayloadGPR = op2.payloadGPR();
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
        m_jit.branch32(MacroAssembler::NotEqual, op2TagGPR, TrustedImm32(JSValue::CellTag));
    
    // We know that within this branch, rightChild must be a cell.
    if (masqueradesAsUndefinedWatchpointValid) {
        m_jit.graph().globalObjectFor(m_currentNode->codeOrigin)->masqueradesAsUndefinedWatchpoint()->add(speculationWatchpoint());
        DFG_TYPE_CHECK(
            JSValueRegs(op2TagGPR, op2PayloadGPR), rightChild, (~SpecCell) | SpecObject,
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op2PayloadGPR, JSCell::structureOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
    } else {
        m_jit.loadPtr(MacroAssembler::Address(op2PayloadGPR, JSCell::structureOffset()), structureGPR);
        DFG_TYPE_CHECK(
            JSValueRegs(op2TagGPR, op2PayloadGPR), rightChild, (~SpecCell) | SpecObject,
            m_jit.branchPtr(
                MacroAssembler::Equal,
                structureGPR,
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
        speculationCheck(BadType, JSValueRegs(op2TagGPR, op2PayloadGPR), rightChild, 
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(structureGPR, Structure::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));
    }
    
    // At this point we know that we can perform a straight-forward equality comparison on pointer
    // values because both left and right are pointers to objects that have no special equality
    // protocols.
    MacroAssembler::Jump falseCase = m_jit.branchPtr(MacroAssembler::NotEqual, op1GPR, op2PayloadGPR);
    MacroAssembler::Jump trueCase = m_jit.jump();
    
    rightNotCell.link(&m_jit);
    
    // We know that within this branch, rightChild must not be a cell. Check if that is enough to
    // prove that it is either null or undefined.
    if (needsTypeCheck(rightChild, SpecCell | SpecOther)) {
        m_jit.move(op2TagGPR, resultGPR);
        m_jit.or32(TrustedImm32(1), resultGPR);
        
        typeCheck(
            JSValueRegs(op2TagGPR, op2PayloadGPR), rightChild, SpecCell | SpecOther,
            m_jit.branch32(
                MacroAssembler::NotEqual, resultGPR,
                MacroAssembler::TrustedImm32(JSValue::NullTag)));
    }
    
    falseCase.link(&m_jit);
    m_jit.move(TrustedImm32(0), resultGPR);
    MacroAssembler::Jump done = m_jit.jump();
    trueCase.link(&m_jit);
    m_jit.move(TrustedImm32(1), resultGPR);
    done.link(&m_jit);
    
    booleanResult(resultGPR, m_currentNode);
}

void SpeculativeJIT::compilePeepHoleObjectToObjectOrOtherEquality(Edge leftChild, Edge rightChild, Node* branchNode)
{
    BlockIndex taken = branchNode->takenBlockIndex();
    BlockIndex notTaken = branchNode->notTakenBlockIndex();
    
    SpeculateCellOperand op1(this, leftChild);
    JSValueOperand op2(this, rightChild, ManualOperandSpeculation);
    GPRTemporary result(this);
    
    GPRReg op1GPR = op1.gpr();
    GPRReg op2TagGPR = op2.tagGPR();
    GPRReg op2PayloadGPR = op2.payloadGPR();
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
        m_jit.branch32(MacroAssembler::NotEqual, op2TagGPR, TrustedImm32(JSValue::CellTag));
    
    // We know that within this branch, rightChild must be a cell.
    if (masqueradesAsUndefinedWatchpointValid) {
        m_jit.graph().globalObjectFor(m_currentNode->codeOrigin)->masqueradesAsUndefinedWatchpoint()->add(speculationWatchpoint());
        DFG_TYPE_CHECK(
            JSValueRegs(op2TagGPR, op2PayloadGPR), rightChild, (~SpecCell) | SpecObject,
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op2PayloadGPR, JSCell::structureOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
    } else {
        m_jit.loadPtr(MacroAssembler::Address(op2PayloadGPR, JSCell::structureOffset()), structureGPR);
        DFG_TYPE_CHECK(
            JSValueRegs(op2TagGPR, op2PayloadGPR), rightChild, (~SpecCell) | SpecObject,
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                structureGPR, 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
        speculationCheck(BadType, JSValueRegs(op2TagGPR, op2PayloadGPR), rightChild,
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(structureGPR, Structure::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));
    }
    
    // At this point we know that we can perform a straight-forward equality comparison on pointer
    // values because both left and right are pointers to objects that have no special equality
    // protocols.
    branch32(MacroAssembler::Equal, op1GPR, op2PayloadGPR, taken);
    
    // We know that within this branch, rightChild must not be a cell. Check if that is enough to
    // prove that it is either null or undefined.
    if (!needsTypeCheck(rightChild, SpecCell | SpecOther))
        rightNotCell.link(&m_jit);
    else {
        jump(notTaken, ForceJump);
        
        rightNotCell.link(&m_jit);
        m_jit.move(op2TagGPR, resultGPR);
        m_jit.or32(TrustedImm32(1), resultGPR);
        
        typeCheck(
            JSValueRegs(op2TagGPR, op2PayloadGPR), rightChild, SpecCell | SpecOther,
            m_jit.branch32(
                MacroAssembler::NotEqual, resultGPR,
                MacroAssembler::TrustedImm32(JSValue::NullTag)));
    }
    
    jump(notTaken);
}

void SpeculativeJIT::compileIntegerCompare(Node* node, MacroAssembler::RelationalCondition condition)
{
    SpeculateIntegerOperand op1(this, node->child1());
    SpeculateIntegerOperand op2(this, node->child2());
    GPRTemporary resultPayload(this);
    
    m_jit.compare32(condition, op1.gpr(), op2.gpr(), resultPayload.gpr());
    
    // If we add a DataFormatBool, we should use it here.
    booleanResult(resultPayload.gpr(), node);
}

void SpeculativeJIT::compileDoubleCompare(Node* node, MacroAssembler::DoubleCondition condition)
{
    SpeculateDoubleOperand op1(this, node->child1());
    SpeculateDoubleOperand op2(this, node->child2());
    GPRTemporary resultPayload(this);
    
    m_jit.move(TrustedImm32(1), resultPayload.gpr());
    MacroAssembler::Jump trueCase = m_jit.branchDouble(condition, op1.fpr(), op2.fpr());
    m_jit.move(TrustedImm32(0), resultPayload.gpr());
    trueCase.link(&m_jit);
    
    booleanResult(resultPayload.gpr(), node);
}

void SpeculativeJIT::compileValueAdd(Node* node)
{
    JSValueOperand op1(this, node->child1());
    JSValueOperand op2(this, node->child2());

    GPRReg op1TagGPR = op1.tagGPR();
    GPRReg op1PayloadGPR = op1.payloadGPR();
    GPRReg op2TagGPR = op2.tagGPR();
    GPRReg op2PayloadGPR = op2.payloadGPR();

    flushRegisters();
    
    GPRResult2 resultTag(this);
    GPRResult resultPayload(this);
    if (isKnownNotNumber(node->child1().node()) || isKnownNotNumber(node->child2().node()))
        callOperation(operationValueAddNotNumber, resultTag.gpr(), resultPayload.gpr(), op1TagGPR, op1PayloadGPR, op2TagGPR, op2PayloadGPR);
    else
        callOperation(operationValueAdd, resultTag.gpr(), resultPayload.gpr(), op1TagGPR, op1PayloadGPR, op2TagGPR, op2PayloadGPR);
    
    jsValueResult(resultTag.gpr(), resultPayload.gpr(), node);
}

void SpeculativeJIT::compileObjectOrOtherLogicalNot(Edge nodeUse)
{
    JSValueOperand value(this, nodeUse, ManualOperandSpeculation);
    GPRTemporary resultPayload(this);
    GPRReg valueTagGPR = value.tagGPR();
    GPRReg valuePayloadGPR = value.payloadGPR();
    GPRReg resultPayloadGPR = resultPayload.gpr();
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

    MacroAssembler::Jump notCell = m_jit.branch32(MacroAssembler::NotEqual, valueTagGPR, TrustedImm32(JSValue::CellTag));
    if (masqueradesAsUndefinedWatchpointValid) {
        m_jit.graph().globalObjectFor(m_currentNode->codeOrigin)->masqueradesAsUndefinedWatchpoint()->add(speculationWatchpoint());

        DFG_TYPE_CHECK(
            JSValueRegs(valueTagGPR, valuePayloadGPR), nodeUse, (~SpecCell) | SpecObject,
            m_jit.branchPtr(
                MacroAssembler::Equal,
                MacroAssembler::Address(valuePayloadGPR, JSCell::structureOffset()),
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
    } else {
        m_jit.loadPtr(MacroAssembler::Address(valuePayloadGPR, JSCell::structureOffset()), structureGPR);

        DFG_TYPE_CHECK(
            JSValueRegs(valueTagGPR, valuePayloadGPR), nodeUse, (~SpecCell) | SpecObject,
            m_jit.branchPtr(
                MacroAssembler::Equal,
                structureGPR,
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));

        MacroAssembler::Jump isNotMasqueradesAsUndefined = 
            m_jit.branchTest8(
                MacroAssembler::Zero, 
                MacroAssembler::Address(structureGPR, Structure::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined));

        speculationCheck(BadType, JSValueRegs(valueTagGPR, valuePayloadGPR), nodeUse, 
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(structureGPR, Structure::globalObjectOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.graph().globalObjectFor(m_currentNode->codeOrigin))));

        isNotMasqueradesAsUndefined.link(&m_jit);
    }
    m_jit.move(TrustedImm32(0), resultPayloadGPR);
    MacroAssembler::Jump done = m_jit.jump();
    
    notCell.link(&m_jit);
 
    COMPILE_ASSERT((JSValue::UndefinedTag | 1) == JSValue::NullTag, UndefinedTag_OR_1_EQUALS_NullTag);
    if (needsTypeCheck(nodeUse, SpecCell | SpecOther)) {
        m_jit.move(valueTagGPR, resultPayloadGPR);
        m_jit.or32(TrustedImm32(1), resultPayloadGPR);
        typeCheck(
            JSValueRegs(valueTagGPR, valuePayloadGPR), nodeUse, SpecCell | SpecOther,
            m_jit.branch32(
                MacroAssembler::NotEqual, 
                resultPayloadGPR, 
                TrustedImm32(JSValue::NullTag)));
    }
    m_jit.move(TrustedImm32(1), resultPayloadGPR);
    
    done.link(&m_jit);
    
    booleanResult(resultPayloadGPR, m_currentNode);
}

void SpeculativeJIT::compileLogicalNot(Node* node)
{
    switch (node->child1().useKind()) {
    case BooleanUse: {
        SpeculateBooleanOperand value(this, node->child1());
        GPRTemporary result(this, value);
        m_jit.xor32(TrustedImm32(1), value.gpr(), result.gpr());
        booleanResult(result.gpr(), node);
        return;
    }
        
    case ObjectOrOtherUse: {
        compileObjectOrOtherLogicalNot(node->child1());
        return;
    }
        
    case Int32Use: {
        SpeculateIntegerOperand value(this, node->child1());
        GPRTemporary resultPayload(this, value);
        m_jit.compare32(MacroAssembler::Equal, value.gpr(), MacroAssembler::TrustedImm32(0), resultPayload.gpr());
        booleanResult(resultPayload.gpr(), node);
        return;
    }
        
    case NumberUse: {
        SpeculateDoubleOperand value(this, node->child1());
        FPRTemporary scratch(this);
        GPRTemporary resultPayload(this);
        m_jit.move(TrustedImm32(0), resultPayload.gpr());
        MacroAssembler::Jump nonZero = m_jit.branchDoubleNonZero(value.fpr(), scratch.fpr());
        m_jit.move(TrustedImm32(1), resultPayload.gpr());
        nonZero.link(&m_jit);
        booleanResult(resultPayload.gpr(), node);
        return;
    }

    case UntypedUse: {
        JSValueOperand arg1(this, node->child1());
        GPRTemporary resultPayload(this, arg1, false);
        GPRReg arg1TagGPR = arg1.tagGPR();
        GPRReg arg1PayloadGPR = arg1.payloadGPR();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        
        arg1.use();

        JITCompiler::Jump slowCase = m_jit.branch32(JITCompiler::NotEqual, arg1TagGPR, TrustedImm32(JSValue::BooleanTag));
    
        m_jit.move(arg1PayloadGPR, resultPayloadGPR);

        addSlowPathGenerator(
            slowPathCall(
                slowCase, this, dfgConvertJSValueToBoolean, resultPayloadGPR, arg1TagGPR,
                arg1PayloadGPR));
    
        m_jit.xor32(TrustedImm32(1), resultPayloadGPR);
        booleanResult(resultPayloadGPR, node, UseChildrenCalledExplicitly);
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
    GPRReg valueTagGPR = value.tagGPR();
    GPRReg valuePayloadGPR = value.payloadGPR();
    GPRReg scratchGPR = scratch.gpr();
    
    MacroAssembler::Jump notCell = m_jit.branch32(MacroAssembler::NotEqual, valueTagGPR, TrustedImm32(JSValue::CellTag));
    if (m_jit.graph().globalObjectFor(m_currentNode->codeOrigin)->masqueradesAsUndefinedWatchpoint()->isStillValid()) {
        m_jit.graph().globalObjectFor(m_currentNode->codeOrigin)->masqueradesAsUndefinedWatchpoint()->add(speculationWatchpoint());

        DFG_TYPE_CHECK(
            JSValueRegs(valueTagGPR, valuePayloadGPR), nodeUse, (~SpecCell) | SpecObject,
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(valuePayloadGPR, JSCell::structureOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
    } else {
        m_jit.loadPtr(MacroAssembler::Address(valuePayloadGPR, JSCell::structureOffset()), scratchGPR);

        DFG_TYPE_CHECK(
            JSValueRegs(valueTagGPR, valuePayloadGPR), nodeUse, (~SpecCell) | SpecObject,
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                scratchGPR,
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));

        JITCompiler::Jump isNotMasqueradesAsUndefined = m_jit.branchTest8(JITCompiler::Zero, MacroAssembler::Address(scratchGPR, Structure::typeInfoFlagsOffset()), TrustedImm32(MasqueradesAsUndefined));

        speculationCheck(BadType, JSValueRegs(valueTagGPR, valuePayloadGPR), nodeUse,
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(scratchGPR, Structure::globalObjectOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.graph().globalObjectFor(m_currentNode->codeOrigin))));

        isNotMasqueradesAsUndefined.link(&m_jit);
    }
    jump(taken, ForceJump);
    
    notCell.link(&m_jit);
    
    COMPILE_ASSERT((JSValue::UndefinedTag | 1) == JSValue::NullTag, UndefinedTag_OR_1_EQUALS_NullTag);
    if (needsTypeCheck(nodeUse, SpecCell | SpecOther)) {
        m_jit.move(valueTagGPR, scratchGPR);
        m_jit.or32(TrustedImm32(1), scratchGPR);
        typeCheck(
            JSValueRegs(valueTagGPR, valuePayloadGPR), nodeUse, SpecCell | SpecOther,
            m_jit.branch32(MacroAssembler::NotEqual, scratchGPR, TrustedImm32(JSValue::NullTag)));
    }

    jump(notTaken);
    
    noResult(m_currentNode);
}

void SpeculativeJIT::emitBranch(Node* node)
{
    BlockIndex taken = node->takenBlockIndex();
    BlockIndex notTaken = node->notTakenBlockIndex();

    switch (node->child1().useKind()) {
    case BooleanUse: {
        SpeculateBooleanOperand value(this, node->child1());
        MacroAssembler::ResultCondition condition = MacroAssembler::NonZero;

        if (taken == nextBlock()) {
            condition = MacroAssembler::Zero;
            BlockIndex tmp = taken;
            taken = notTaken;
            notTaken = tmp;
        }

        branchTest32(condition, value.gpr(), TrustedImm32(1), taken);
        jump(notTaken);

        noResult(node);
        return;
    }
    
    case ObjectOrOtherUse: {
        emitObjectOrOtherBranch(node->child1(), taken, notTaken);
        return;
    }
    
    case NumberUse:
    case Int32Use: {
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
    
    case UntypedUse: {
        JSValueOperand value(this, node->child1());
        value.fill();
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();

        GPRTemporary result(this);
        GPRReg resultGPR = result.gpr();
    
        use(node->child1());
    
        JITCompiler::Jump fastPath = m_jit.branch32(JITCompiler::Equal, valueTagGPR, JITCompiler::TrustedImm32(JSValue::Int32Tag));
        JITCompiler::Jump slowPath = m_jit.branch32(JITCompiler::NotEqual, valueTagGPR, JITCompiler::TrustedImm32(JSValue::BooleanTag));

        fastPath.link(&m_jit);
        branchTest32(JITCompiler::Zero, valuePayloadGPR, notTaken);
        jump(taken, ForceJump);

        slowPath.link(&m_jit);
        silentSpillAllRegisters(resultGPR);
        callOperation(dfgConvertJSValueToBoolean, resultGPR, valueTagGPR, valuePayloadGPR);
        silentFillAllRegisters(resultGPR);
    
        branchTest32(JITCompiler::NonZero, resultGPR, taken);
        jump(notTaken);
    
        noResult(node, UseChildrenCalledExplicitly);
        return;
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
}

template<typename BaseOperandType, typename PropertyOperandType, typename ValueOperandType, typename TagType>
void SpeculativeJIT::compileContiguousPutByVal(Node* node, BaseOperandType& base, PropertyOperandType& property, ValueOperandType& value, GPRReg valuePayloadReg, TagType valueTag)
{
    Edge child4 = m_jit.graph().varArgChild(node, 3);

    ArrayMode arrayMode = node->arrayMode();
    
    GPRReg baseReg = base.gpr();
    GPRReg propertyReg = property.gpr();
    
    StorageOperand storage(this, child4);
    GPRReg storageReg = storage.gpr();

    if (node->op() == PutByValAlias) {
        // Store the value to the array.
        GPRReg propertyReg = property.gpr();
        m_jit.store32(valueTag, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
        m_jit.store32(valuePayloadReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
        
        noResult(node);
        return;
    }
    
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
        
        m_jit.add32(TrustedImm32(1), propertyReg);
        m_jit.store32(propertyReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength()));
        m_jit.sub32(TrustedImm32(1), propertyReg);
        
        inBounds.link(&m_jit);
    }
    
    m_jit.store32(valueTag, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
    m_jit.store32(valuePayloadReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
    
    base.use();
    property.use();
    value.use();
    storage.use();
    
    if (arrayMode.isOutOfBounds()) {
        addSlowPathGenerator(
            slowPathCall(
                slowCase, this,
                m_jit.codeBlock()->isStrictMode() ? operationPutByValBeyondArrayBoundsStrict : operationPutByValBeyondArrayBoundsNonStrict,
                NoResult, baseReg, propertyReg, valueTag, valuePayloadReg));
    }

    noResult(node, UseChildrenCalledExplicitly);    
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
        
        if (isCellSpeculation(value.m_type)) {
            GPRTemporary result(this);
            m_jit.load32(JITCompiler::payloadFor(node->local()), result.gpr());
            
            // Like cellResult, but don't useChildren - our children are phi nodes,
            // and don't represent values within this dataflow with virtual registers.
            VirtualRegister virtualRegister = node->virtualRegister();
            m_gprs.retain(result.gpr(), virtualRegister, SpillOrderCell);
            m_generationInfo[virtualRegister].initCell(node, node->refCount(), result.gpr());
            break;
        }
        
        if (isBooleanSpeculation(value.m_type)) {
            GPRTemporary result(this);
            m_jit.load32(JITCompiler::payloadFor(node->local()), result.gpr());
            
            // Like booleanResult, but don't useChildren - our children are phi nodes,
            // and don't represent values within this dataflow with virtual registers.
            VirtualRegister virtualRegister = node->virtualRegister();
            m_gprs.retain(result.gpr(), virtualRegister, SpillOrderBoolean);
            m_generationInfo[virtualRegister].initBoolean(node, node->refCount(), result.gpr());
            break;
        }

        GPRTemporary result(this);
        GPRTemporary tag(this);
        m_jit.load32(JITCompiler::payloadFor(node->local()), result.gpr());
        m_jit.load32(JITCompiler::tagFor(node->local()), tag.gpr());

        // Like jsValueResult, but don't useChildren - our children are phi nodes,
        // and don't represent values within this dataflow with virtual registers.
        VirtualRegister virtualRegister = node->virtualRegister();
        m_gprs.retain(result.gpr(), virtualRegister, SpillOrderJS);
        m_gprs.retain(tag.gpr(), virtualRegister, SpillOrderJS);

        m_generationInfo[virtualRegister].initJSValue(node, node->refCount(), tag.gpr(), result.gpr(), DataFormatJS);
        break;
    }
        
    case GetLocalUnlinked: {
        GPRTemporary payload(this);
        GPRTemporary tag(this);
        m_jit.load32(JITCompiler::payloadFor(node->unlinkedLocal()), payload.gpr());
        m_jit.load32(JITCompiler::tagFor(node->unlinkedLocal()), tag.gpr());
        jsValueResult(tag.gpr(), payload.gpr(), node);
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
            if (m_generationInfo[node->child1()->virtualRegister()].registerFormat() == DataFormatDouble) {
                SpeculateDoubleOperand value(this, node->child1(), ManualOperandSpeculation);
                m_jit.storeDouble(value.fpr(), JITCompiler::addressFor(node->local()));
                noResult(node);
                recordSetLocal(node->local(), ValueSource(DoubleInJSStack));
                break;
            }
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
                m_jit.storePtr(cellGPR, JITCompiler::payloadFor(node->local()));
                noResult(node);
                recordSetLocal(node->local(), ValueSource(CellInJSStack));
                break;
            }
            if (isBooleanSpeculation(predictedType)) {
                SpeculateBooleanOperand value(this, node->child1());
                m_jit.store32(value.gpr(), JITCompiler::payloadFor(node->local()));
                noResult(node);
                recordSetLocal(node->local(), ValueSource(BooleanInJSStack));
                break;
            }
        }
        JSValueOperand value(this, node->child1());
        m_jit.store32(value.payloadGPR(), JITCompiler::payloadFor(node->local()));
        m_jit.store32(value.tagGPR(), JITCompiler::tagFor(node->local()));
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
#if CPU(X86)
            compileIntegerArithDivForX86(node);
#elif CPU(APPLE_ARMV7S)
            compileIntegerArithDivForARMv7s(node);
#else // CPU type without integer divide
            RELEASE_ASSERT_NOT_REACHED(); // should have been coverted into a double divide.
#endif
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
            GPRTemporary result(this, op1);
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

            GPRReg op1GPR = op1.gpr();
            GPRReg op2GPR = op2.gpr();
            GPRReg resultGPR = result.gpr();

            MacroAssembler::Jump op1Less = m_jit.branch32(op == ArithMin ? MacroAssembler::LessThan : MacroAssembler::GreaterThan, op1GPR, op2GPR);
            m_jit.move(op2GPR, resultGPR);
            if (op1GPR != resultGPR) {
                MacroAssembler::Jump done = m_jit.jump();
                op1Less.link(&m_jit);
                m_jit.move(op1GPR, resultGPR);
                done.link(&m_jit);
            } else
                op1Less.link(&m_jit);
            
            integerResult(resultGPR, node);
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
            SpeculateCellOperand base(this, node->child1()); // Save a register, speculate cell. We'll probably be right.
            JSValueOperand property(this, node->child2());
            GPRReg baseGPR = base.gpr();
            GPRReg propertyTagGPR = property.tagGPR();
            GPRReg propertyPayloadGPR = property.payloadGPR();
            
            flushRegisters();
            GPRResult2 resultTag(this);
            GPRResult resultPayload(this);
            callOperation(operationGetByValCell, resultTag.gpr(), resultPayload.gpr(), baseGPR, propertyTagGPR, propertyPayloadGPR);
            
            jsValueResult(resultTag.gpr(), resultPayload.gpr(), node);
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
            
                GPRTemporary resultPayload(this);
                if (node->arrayMode().type() == Array::Int32) {
                    speculationCheck(
                        OutOfBounds, JSValueRegs(), 0,
                        m_jit.branch32(
                            MacroAssembler::Equal,
                            MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)),
                            TrustedImm32(JSValue::EmptyValueTag)));
                    m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)), resultPayload.gpr());
                    integerResult(resultPayload.gpr(), node);
                    break;
                }
                
                GPRTemporary resultTag(this);
                m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)), resultTag.gpr());
                speculationCheck(LoadFromHole, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::Equal, resultTag.gpr(), TrustedImm32(JSValue::EmptyValueTag)));
                m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)), resultPayload.gpr());
                jsValueResult(resultTag.gpr(), resultPayload.gpr(), node);
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
            
            GPRTemporary resultTag(this);
            GPRTemporary resultPayload(this);
            GPRReg resultTagReg = resultTag.gpr();
            GPRReg resultPayloadReg = resultPayload.gpr();
            
            MacroAssembler::JumpList slowCases;

            slowCases.append(m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength())));
            
            m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)), resultTagReg);
            m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)), resultPayloadReg);
            slowCases.append(m_jit.branch32(MacroAssembler::Equal, resultTagReg, TrustedImm32(JSValue::EmptyValueTag)));
            
            addSlowPathGenerator(
                slowPathCall(
                    slowCases, this, operationGetByValArrayInt,
                    JSValueRegs(resultTagReg, resultPayloadReg), baseReg, propertyReg));
            
            jsValueResult(resultTagReg, resultPayloadReg, node);
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
            
            GPRTemporary resultTag(this);
            GPRTemporary resultPayload(this);
            FPRTemporary temp(this);
            GPRReg resultTagReg = resultTag.gpr();
            GPRReg resultPayloadReg = resultPayload.gpr();
            FPRReg tempReg = temp.fpr();
            
            MacroAssembler::JumpList slowCases;
            
            slowCases.append(m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength())));
            
            m_jit.loadDouble(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight), tempReg);
            slowCases.append(m_jit.branchDouble(MacroAssembler::DoubleNotEqualOrUnordered, tempReg, tempReg));
            boxDouble(tempReg, resultTagReg, resultPayloadReg);

            addSlowPathGenerator(
                slowPathCall(
                    slowCases, this, operationGetByValArrayInt,
                    JSValueRegs(resultTagReg, resultPayloadReg), baseReg, propertyReg));
            
            jsValueResult(resultTagReg, resultPayloadReg, node);
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

                GPRTemporary resultTag(this);
                GPRTemporary resultPayload(this);

                m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), resultTag.gpr());
                speculationCheck(LoadFromHole, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::Equal, resultTag.gpr(), TrustedImm32(JSValue::EmptyValueTag)));
                m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), resultPayload.gpr());
            
                jsValueResult(resultTag.gpr(), resultPayload.gpr(), node);
                break;
            }

            SpeculateCellOperand base(this, node->child1());
            SpeculateStrictInt32Operand property(this, node->child2());
            StorageOperand storage(this, node->child3());
            GPRReg propertyReg = property.gpr();
            GPRReg storageReg = storage.gpr();
            GPRReg baseReg = base.gpr();

            if (!m_compileOkay)
                return;

            GPRTemporary resultTag(this);
            GPRTemporary resultPayload(this);
            GPRReg resultTagReg = resultTag.gpr();
            GPRReg resultPayloadReg = resultPayload.gpr();

            JITCompiler::Jump outOfBounds = m_jit.branch32(
                MacroAssembler::AboveOrEqual, propertyReg,
                MacroAssembler::Address(storageReg, ArrayStorage::vectorLengthOffset()));

            m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), resultTagReg);
            JITCompiler::Jump hole = m_jit.branch32(
                MacroAssembler::Equal, resultTag.gpr(), TrustedImm32(JSValue::EmptyValueTag));
            m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), resultPayloadReg);
            
            JITCompiler::JumpList slowCases;
            slowCases.append(outOfBounds);
            slowCases.append(hole);
            addSlowPathGenerator(
                slowPathCall(
                    slowCases, this, operationGetByValArrayInt,
                    JSValueRegs(resultTagReg, resultPayloadReg),
                    baseReg, propertyReg));

            jsValueResult(resultTagReg, resultPayloadReg, node);
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
            ASSERT(node->op() == PutByVal);
            
            SpeculateCellOperand base(this, child1); // Save a register, speculate cell. We'll probably be right.
            JSValueOperand property(this, child2);
            JSValueOperand value(this, child3);
            GPRReg baseGPR = base.gpr();
            GPRReg propertyTagGPR = property.tagGPR();
            GPRReg propertyPayloadGPR = property.payloadGPR();
            GPRReg valueTagGPR = value.tagGPR();
            GPRReg valuePayloadGPR = value.payloadGPR();
            
            flushRegisters();
            callOperation(m_jit.codeBlock()->isStrictMode() ? operationPutByValCellStrict : operationPutByValCellNonStrict, baseGPR, propertyTagGPR, propertyPayloadGPR, valueTagGPR, valuePayloadGPR);
            
            noResult(node);
            alreadyHandled = true;
            break;
        }
        default:
            break;
        }
        
        if (alreadyHandled)
            break;
        
        SpeculateCellOperand base(this, child1);
        SpeculateStrictInt32Operand property(this, child2);
        
        GPRReg baseReg = base.gpr();
        GPRReg propertyReg = property.gpr();

        switch (arrayMode.type()) {
        case Array::Int32: {
            SpeculateIntegerOperand value(this, child3);

            GPRReg valuePayloadReg = value.gpr();
        
            if (!m_compileOkay)
                return;
            
            compileContiguousPutByVal(node, base, property, value, valuePayloadReg, TrustedImm32(JSValue::Int32Tag));
            break;
        }
        case Array::Contiguous: {
            JSValueOperand value(this, child3);

            GPRReg valueTagReg = value.tagGPR();
            GPRReg valuePayloadReg = value.payloadGPR();
        
            if (!m_compileOkay)
                return;
        
            if (Heap::isWriteBarrierEnabled()) {
                GPRTemporary scratch(this);
                writeBarrier(baseReg, valueTagReg, child3, WriteBarrierForPropertyAccess, scratch.gpr());
            }
            
            compileContiguousPutByVal(node, base, property, value, valuePayloadReg, valueTagReg);
            break;
        }
        case Array::Double: {
            compileDoublePutByVal(node, base, property);
            break;
        }
        case Array::ArrayStorage:
        case Array::SlowPutArrayStorage: {
            JSValueOperand value(this, child3);

            GPRReg valueTagReg = value.tagGPR();
            GPRReg valuePayloadReg = value.payloadGPR();
            
            if (!m_compileOkay)
                return;
            
            {
                GPRTemporary scratch(this);
                GPRReg scratchReg = scratch.gpr();
                writeBarrier(baseReg, valueTagReg, child3, WriteBarrierForPropertyAccess, scratchReg);
            }
            
            StorageOperand storage(this, child4);
            GPRReg storageReg = storage.gpr();

            if (node->op() == PutByValAlias) {
                // Store the value to the array.
                GPRReg propertyReg = property.gpr();
                m_jit.store32(value.tagGPR(), MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
                m_jit.store32(value.payloadGPR(), MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
                
                noResult(node);
                break;
            }

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
                    m_jit.branch32(MacroAssembler::Equal, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), TrustedImm32(JSValue::EmptyValueTag)));
            } else {
                MacroAssembler::Jump notHoleValue = m_jit.branch32(MacroAssembler::NotEqual, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), TrustedImm32(JSValue::EmptyValueTag));
                if (arrayMode.isSlowPut()) {
                    // This is sort of strange. If we wanted to optimize this code path, we would invert
                    // the above branch. But it's simply not worth it since this only happens if we're
                    // already having a bad time.
                    slowCases.append(m_jit.jump());
                } else {
                    m_jit.add32(TrustedImm32(1), MacroAssembler::Address(storageReg, ArrayStorage::numValuesInVectorOffset()));
                
                    // If we're writing to a hole we might be growing the array; 
                    MacroAssembler::Jump lengthDoesNotNeedUpdate = m_jit.branch32(MacroAssembler::Below, propertyReg, MacroAssembler::Address(storageReg, ArrayStorage::lengthOffset()));
                    m_jit.add32(TrustedImm32(1), propertyReg);
                    m_jit.store32(propertyReg, MacroAssembler::Address(storageReg, ArrayStorage::lengthOffset()));
                    m_jit.sub32(TrustedImm32(1), propertyReg);
                
                    lengthDoesNotNeedUpdate.link(&m_jit);
                }
                notHoleValue.link(&m_jit);
            }
    
            // Store the value to the array.
            m_jit.store32(valueTagReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
            m_jit.store32(valuePayloadReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)));

            base.use();
            property.use();
            value.use();
            storage.use();
            
            if (!slowCases.empty()) {
                addSlowPathGenerator(
                    slowPathCall(
                        slowCases, this,
                        m_jit.codeBlock()->isStrictMode() ? operationPutByValBeyondArrayBoundsStrict : operationPutByValBeyondArrayBoundsNonStrict,
                        NoResult, baseReg, propertyReg, valueTagReg, valuePayloadReg));
            }

            noResult(node, UseChildrenCalledExplicitly);
            break;
        }
            
        case Array::Arguments:
            // FIXME: we could at some point make this work. Right now we're assuming that the register
            // pressure would be too great.
            RELEASE_ASSERT_NOT_REACHED();
            break;
            
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
            booleanResult(result.gpr(), node);
            break;
        }

        SpeculateCellOperand base(this, node->child1());
        SpeculateCellOperand argument(this, node->child2());
        GPRReg baseGPR = base.gpr();
        GPRReg argumentGPR = argument.gpr();
        
        flushRegisters();
        GPRResult2 resultTag(this);
        GPRResult resultPayload(this);
        callOperation(operationRegExpExec, resultTag.gpr(), resultPayload.gpr(), baseGPR, argumentGPR);
        
        jsValueResult(resultTag.gpr(), resultPayload.gpr(), node);
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
        booleanResult(result.gpr(), node);
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
        case Array::Int32: {
            SpeculateIntegerOperand value(this, node->child2());
            GPRReg valuePayloadGPR = value.gpr();
            
            m_jit.load32(MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()), storageLengthGPR);
            MacroAssembler::Jump slowPath = m_jit.branch32(MacroAssembler::AboveOrEqual, storageLengthGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfVectorLength()));
            m_jit.store32(TrustedImm32(JSValue::Int32Tag), MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
            m_jit.store32(valuePayloadGPR, MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
            m_jit.add32(TrustedImm32(1), storageLengthGPR);
            m_jit.store32(storageLengthGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()));
            m_jit.move(TrustedImm32(JSValue::Int32Tag), storageGPR);
            
            addSlowPathGenerator(
                slowPathCall(
                    slowPath, this, operationArrayPush,
                    JSValueRegs(storageGPR, storageLengthGPR),
                    TrustedImm32(JSValue::Int32Tag), valuePayloadGPR, baseGPR));
        
            jsValueResult(storageGPR, storageLengthGPR, node);
            break;
        }
            
        case Array::Contiguous: {
            JSValueOperand value(this, node->child2());
            GPRReg valueTagGPR = value.tagGPR();
            GPRReg valuePayloadGPR = value.payloadGPR();

            if (Heap::isWriteBarrierEnabled()) {
                GPRTemporary scratch(this);
                writeBarrier(baseGPR, valueTagGPR, node->child2(), WriteBarrierForPropertyAccess, scratch.gpr(), storageLengthGPR);
            }

            m_jit.load32(MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()), storageLengthGPR);
            MacroAssembler::Jump slowPath = m_jit.branch32(MacroAssembler::AboveOrEqual, storageLengthGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfVectorLength()));
            m_jit.store32(valueTagGPR, MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
            m_jit.store32(valuePayloadGPR, MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
            m_jit.add32(TrustedImm32(1), storageLengthGPR);
            m_jit.store32(storageLengthGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()));
            m_jit.move(TrustedImm32(JSValue::Int32Tag), storageGPR);
            
            addSlowPathGenerator(
                slowPathCall(
                    slowPath, this, operationArrayPush,
                    JSValueRegs(storageGPR, storageLengthGPR),
                    valueTagGPR, valuePayloadGPR, baseGPR));
        
            jsValueResult(storageGPR, storageLengthGPR, node);
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
            m_jit.move(TrustedImm32(JSValue::Int32Tag), storageGPR);
            
            addSlowPathGenerator(
                slowPathCall(
                    slowPath, this, operationArrayPushDouble,
                    JSValueRegs(storageGPR, storageLengthGPR),
                    valueFPR, baseGPR));
        
            jsValueResult(storageGPR, storageLengthGPR, node);
            break;
        }
            
        case Array::ArrayStorage: {
            JSValueOperand value(this, node->child2());
            GPRReg valueTagGPR = value.tagGPR();
            GPRReg valuePayloadGPR = value.payloadGPR();

            if (Heap::isWriteBarrierEnabled()) {
                GPRTemporary scratch(this);
                writeBarrier(baseGPR, valueTagGPR, node->child2(), WriteBarrierForPropertyAccess, scratch.gpr(), storageLengthGPR);
            }

            m_jit.load32(MacroAssembler::Address(storageGPR, ArrayStorage::lengthOffset()), storageLengthGPR);
        
            // Refuse to handle bizarre lengths.
            speculationCheck(Uncountable, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::Above, storageLengthGPR, TrustedImm32(0x7ffffffe)));
        
            MacroAssembler::Jump slowPath = m_jit.branch32(MacroAssembler::AboveOrEqual, storageLengthGPR, MacroAssembler::Address(storageGPR, ArrayStorage::vectorLengthOffset()));
        
            m_jit.store32(valueTagGPR, MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
            m_jit.store32(valuePayloadGPR, MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
        
            m_jit.add32(TrustedImm32(1), storageLengthGPR);
            m_jit.store32(storageLengthGPR, MacroAssembler::Address(storageGPR, ArrayStorage::lengthOffset()));
            m_jit.add32(TrustedImm32(1), MacroAssembler::Address(storageGPR, OBJECT_OFFSETOF(ArrayStorage, m_numValuesInVector)));
            m_jit.move(TrustedImm32(JSValue::Int32Tag), storageGPR);
        
            addSlowPathGenerator(slowPathCall(slowPath, this, operationArrayPush, JSValueRegs(storageGPR, storageLengthGPR), valueTagGPR, valuePayloadGPR, baseGPR));
        
            jsValueResult(storageGPR, storageLengthGPR, node);
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
        GPRTemporary valueTag(this);
        GPRTemporary valuePayload(this);
        
        GPRReg baseGPR = base.gpr();
        GPRReg valueTagGPR = valueTag.gpr();
        GPRReg valuePayloadGPR = valuePayload.gpr();
        GPRReg storageGPR = storage.gpr();
        
        switch (node->arrayMode().type()) {
        case Array::Int32:
        case Array::Contiguous: {
            m_jit.load32(
                MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()), valuePayloadGPR);
            MacroAssembler::Jump undefinedCase =
                m_jit.branchTest32(MacroAssembler::Zero, valuePayloadGPR);
            m_jit.sub32(TrustedImm32(1), valuePayloadGPR);
            m_jit.store32(
                valuePayloadGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()));
            m_jit.load32(
                MacroAssembler::BaseIndex(storageGPR, valuePayloadGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)),
                valueTagGPR);
            MacroAssembler::Jump slowCase = m_jit.branch32(MacroAssembler::Equal, valueTagGPR, TrustedImm32(JSValue::EmptyValueTag));
            m_jit.store32(
                MacroAssembler::TrustedImm32(JSValue::EmptyValueTag),
                MacroAssembler::BaseIndex(storageGPR, valuePayloadGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
            m_jit.load32(
                MacroAssembler::BaseIndex(storageGPR, valuePayloadGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)),
                valuePayloadGPR);

            addSlowPathGenerator(
                slowPathMove(
                    undefinedCase, this,
                    MacroAssembler::TrustedImm32(jsUndefined().tag()), valueTagGPR,
                    MacroAssembler::TrustedImm32(jsUndefined().payload()), valuePayloadGPR));
            addSlowPathGenerator(
                slowPathCall(
                    slowCase, this, operationArrayPopAndRecoverLength,
                    JSValueRegs(valueTagGPR, valuePayloadGPR), baseGPR));
            
            jsValueResult(valueTagGPR, valuePayloadGPR, node);
            break;
        }
            
        case Array::Double: {
            FPRTemporary temp(this);
            FPRReg tempFPR = temp.fpr();
            
            m_jit.load32(
                MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()), valuePayloadGPR);
            MacroAssembler::Jump undefinedCase =
                m_jit.branchTest32(MacroAssembler::Zero, valuePayloadGPR);
            m_jit.sub32(TrustedImm32(1), valuePayloadGPR);
            m_jit.store32(
                valuePayloadGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()));
            m_jit.loadDouble(
                MacroAssembler::BaseIndex(storageGPR, valuePayloadGPR, MacroAssembler::TimesEight),
                tempFPR);
            MacroAssembler::Jump slowCase = m_jit.branchDouble(MacroAssembler::DoubleNotEqualOrUnordered, tempFPR, tempFPR);
            JSValue nan = JSValue(JSValue::EncodeAsDouble, QNaN);
            m_jit.store32(
                MacroAssembler::TrustedImm32(nan.u.asBits.tag),
                MacroAssembler::BaseIndex(storageGPR, valuePayloadGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
            m_jit.store32(
                MacroAssembler::TrustedImm32(nan.u.asBits.payload),
                MacroAssembler::BaseIndex(storageGPR, valuePayloadGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
            boxDouble(tempFPR, valueTagGPR, valuePayloadGPR);

            addSlowPathGenerator(
                slowPathMove(
                    undefinedCase, this,
                    MacroAssembler::TrustedImm32(jsUndefined().tag()), valueTagGPR,
                    MacroAssembler::TrustedImm32(jsUndefined().payload()), valuePayloadGPR));
            addSlowPathGenerator(
                slowPathCall(
                    slowCase, this, operationArrayPopAndRecoverLength,
                    JSValueRegs(valueTagGPR, valuePayloadGPR), baseGPR));
            
            jsValueResult(valueTagGPR, valuePayloadGPR, node);
            break;
        }

        case Array::ArrayStorage: {
            GPRTemporary storageLength(this);
            GPRReg storageLengthGPR = storageLength.gpr();

            m_jit.load32(MacroAssembler::Address(storageGPR, ArrayStorage::lengthOffset()), storageLengthGPR);
        
            JITCompiler::JumpList setUndefinedCases;
            setUndefinedCases.append(m_jit.branchTest32(MacroAssembler::Zero, storageLengthGPR));
        
            m_jit.sub32(TrustedImm32(1), storageLengthGPR);
        
            MacroAssembler::Jump slowCase = m_jit.branch32(MacroAssembler::AboveOrEqual, storageLengthGPR, MacroAssembler::Address(storageGPR, ArrayStorage::vectorLengthOffset()));
        
            m_jit.load32(MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), valueTagGPR);
            m_jit.load32(MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), valuePayloadGPR);
        
            m_jit.store32(storageLengthGPR, MacroAssembler::Address(storageGPR, ArrayStorage::lengthOffset()));

            setUndefinedCases.append(m_jit.branch32(MacroAssembler::Equal, TrustedImm32(JSValue::EmptyValueTag), valueTagGPR));
        
            m_jit.store32(TrustedImm32(JSValue::EmptyValueTag), MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)));

            m_jit.sub32(TrustedImm32(1), MacroAssembler::Address(storageGPR, OBJECT_OFFSETOF(ArrayStorage, m_numValuesInVector)));
        
            addSlowPathGenerator(
                slowPathMove(
                    setUndefinedCases, this,
                    MacroAssembler::TrustedImm32(jsUndefined().tag()), valueTagGPR,
                    MacroAssembler::TrustedImm32(jsUndefined().payload()), valuePayloadGPR));
        
            addSlowPathGenerator(
                slowPathCall(
                    slowCase, this, operationArrayPop,
                    JSValueRegs(valueTagGPR, valuePayloadGPR), baseGPR));

            jsValueResult(valueTagGPR, valuePayloadGPR, node);
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
        ASSERT(GPRInfo::callFrameRegister != GPRInfo::regT2);
        ASSERT(GPRInfo::regT1 != GPRInfo::returnValueGPR);
        ASSERT(GPRInfo::returnValueGPR != GPRInfo::callFrameRegister);

#if DFG_ENABLE(SUCCESS_STATS)
        static SamplingCounter counter("SpeculativeJIT");
        m_jit.emitCount(counter);
#endif

        // Return the result in returnValueGPR.
        JSValueOperand op1(this, node->child1());
        op1.fill();
        if (op1.isDouble())
            boxDouble(op1.fpr(), GPRInfo::returnValueGPR2, GPRInfo::returnValueGPR);
        else {
            if (op1.payloadGPR() == GPRInfo::returnValueGPR2 && op1.tagGPR() == GPRInfo::returnValueGPR)
                m_jit.swap(GPRInfo::returnValueGPR, GPRInfo::returnValueGPR2);
            else if (op1.payloadGPR() == GPRInfo::returnValueGPR2) {
                m_jit.move(op1.payloadGPR(), GPRInfo::returnValueGPR);
                m_jit.move(op1.tagGPR(), GPRInfo::returnValueGPR2);
            } else {
                m_jit.move(op1.tagGPR(), GPRInfo::returnValueGPR2);
                m_jit.move(op1.payloadGPR(), GPRInfo::returnValueGPR);
            }
        }

        // Grab the return address.
        m_jit.emitGetFromCallFrameHeaderPtr(JSStack::ReturnPC, GPRInfo::regT2);
        // Restore our caller's "r".
        m_jit.emitGetFromCallFrameHeaderPtr(JSStack::CallerFrame, GPRInfo::callFrameRegister);
        // Return.
        m_jit.restoreReturnAddressBeforeReturn(GPRInfo::regT2);
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
        GPRTemporary resultTag(this, op1);
        GPRTemporary resultPayload(this, op1, false);
        
        GPRReg op1TagGPR = op1.tagGPR();
        GPRReg op1PayloadGPR = op1.payloadGPR();
        GPRReg resultTagGPR = resultTag.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        
        op1.use();
        
        if (!(m_state.forNode(node->child1()).m_type & ~(SpecNumber | SpecBoolean))) {
            m_jit.move(op1TagGPR, resultTagGPR);
            m_jit.move(op1PayloadGPR, resultPayloadGPR);
        } else {
            MacroAssembler::Jump alreadyPrimitive = m_jit.branch32(MacroAssembler::NotEqual, op1TagGPR, TrustedImm32(JSValue::CellTag));
            MacroAssembler::Jump notPrimitive = m_jit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(op1PayloadGPR, JSCell::structureOffset()), MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get()));
            
            alreadyPrimitive.link(&m_jit);
            m_jit.move(op1TagGPR, resultTagGPR);
            m_jit.move(op1PayloadGPR, resultPayloadGPR);
            
            addSlowPathGenerator(
                slowPathCall(
                    notPrimitive, this, operationToPrimitive,
                    JSValueRegs(resultTagGPR, resultPayloadGPR), op1TagGPR, op1PayloadGPR));
        }
        
        jsValueResult(resultTagGPR, resultPayloadGPR, node, UseChildrenCalledExplicitly);
        break;
    }
        
    case ToString: {
        if (node->child1().useKind() == UntypedUse) {
            JSValueOperand op1(this, node->child1());
            GPRReg op1PayloadGPR = op1.payloadGPR();
            GPRReg op1TagGPR = op1.tagGPR();
            
            GPRResult result(this);
            GPRReg resultGPR = result.gpr();
            
            flushRegisters();
            
            JITCompiler::Jump done;
            if (node->child1()->prediction() & SpecString) {
                JITCompiler::Jump slowPath1 = m_jit.branch32(
                    JITCompiler::NotEqual, op1TagGPR, TrustedImm32(JSValue::CellTag));
                JITCompiler::Jump slowPath2 = m_jit.branchPtr(
                    JITCompiler::NotEqual,
                    JITCompiler::Address(op1PayloadGPR, JSCell::structureOffset()),
                    TrustedImmPtr(m_jit.vm()->stringStructure.get()));
                m_jit.move(op1PayloadGPR, resultGPR);
                done = m_jit.jump();
                slowPath1.link(&m_jit);
                slowPath2.link(&m_jit);
            }
            callOperation(operationToString, resultGPR, op1TagGPR, op1PayloadGPR);
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
            ASSERT(structure->indexingType() == node->indexingType());
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
                        m_jit.branchDouble(MacroAssembler::DoubleNotEqualOrUnordered, opFPR, opFPR));
        
                    m_jit.storeDouble(opFPR, MacroAssembler::Address(storageGPR, sizeof(double) * operandIdx));
                    break;
                }
                case ALL_INT32_INDEXING_TYPES: {
                    SpeculateIntegerOperand operand(this, use);
                    m_jit.store32(TrustedImm32(JSValue::Int32Tag), MacroAssembler::Address(storageGPR, sizeof(JSValue) * operandIdx + OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
                    m_jit.store32(operand.gpr(), MacroAssembler::Address(storageGPR, sizeof(JSValue) * operandIdx + OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
                    break;
                }
                case ALL_CONTIGUOUS_INDEXING_TYPES: {
                    JSValueOperand operand(this, m_jit.graph().m_varArgChildren[node->firstChild() + operandIdx]);
                    GPRReg opTagGPR = operand.tagGPR();
                    GPRReg opPayloadGPR = operand.payloadGPR();
                    m_jit.store32(opTagGPR, MacroAssembler::Address(storageGPR, sizeof(JSValue) * operandIdx + OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
                    m_jit.store32(opPayloadGPR, MacroAssembler::Address(storageGPR, sizeof(JSValue) * operandIdx + OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
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
            callOperation(
                operationNewEmptyArray, result.gpr(), globalObject->arrayStructureForIndexingTypeDuringAllocation(node->indexingType()));
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
                FPRReg opFPR = operand.fpr();
                DFG_TYPE_CHECK(
                    JSValueRegs(), use, SpecRealNumber,
                    m_jit.branchDouble(MacroAssembler::DoubleNotEqualOrUnordered, opFPR, opFPR));
                
                m_jit.storeDouble(opFPR, reinterpret_cast<char*>(buffer + operandIdx));
                break;
            }
            case ALL_INT32_INDEXING_TYPES: {
                SpeculateIntegerOperand operand(this, use);
                GPRReg opGPR = operand.gpr();
                m_jit.store32(TrustedImm32(JSValue::Int32Tag), reinterpret_cast<char*>(buffer + operandIdx) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
                m_jit.store32(opGPR, reinterpret_cast<char*>(buffer + operandIdx) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
                break;
            }
            case ALL_CONTIGUOUS_INDEXING_TYPES:
            case ALL_ARRAY_STORAGE_INDEXING_TYPES: {
                JSValueOperand operand(this, m_jit.graph().m_varArgChildren[node->firstChild() + operandIdx]);
                GPRReg opTagGPR = operand.tagGPR();
                GPRReg opPayloadGPR = operand.payloadGPR();
                
                m_jit.store32(opTagGPR, reinterpret_cast<char*>(buffer + operandIdx) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
                m_jit.store32(opPayloadGPR, reinterpret_cast<char*>(buffer + operandIdx) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
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
            emitAllocateJSObject<JSArray>(resultGPR, TrustedImmPtr(structure), storageGPR, scratchGPR, scratch2GPR, slowCases);
            
            m_jit.store32(sizeGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()));
            m_jit.store32(sizeGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfVectorLength()));
            
            if (hasDouble(node->indexingType())) {
                JSValue nan = JSValue(JSValue::EncodeAsDouble, QNaN);
                
                m_jit.move(sizeGPR, scratchGPR);
                MacroAssembler::Jump done = m_jit.branchTest32(MacroAssembler::Zero, scratchGPR);
                MacroAssembler::Label loop = m_jit.label();
                m_jit.sub32(TrustedImm32(1), scratchGPR);
                m_jit.store32(TrustedImm32(nan.u.asBits.tag), MacroAssembler::BaseIndex(storageGPR, scratchGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
                m_jit.store32(TrustedImm32(nan.u.asBits.payload), MacroAssembler::BaseIndex(storageGPR, scratchGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
                m_jit.branchTest32(MacroAssembler::NonZero, scratchGPR).linkTo(loop, &m_jit);
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
        callOperation(
            operationNewArrayWithSize, resultGPR, structureGPR, sizeGPR);
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
            
            if (node->indexingType() == ArrayWithDouble) {
                JSValue* data = m_jit.codeBlock()->constantBuffer(node->startConstant());
                for (unsigned index = 0; index < node->numConstants(); ++index) {
                    union {
                        int32_t halves[2];
                        double value;
                    } u;
                    u.value = data[index].asNumber();
                    m_jit.store32(Imm32(u.halves[0]), MacroAssembler::Address(storageGPR, sizeof(double) * index));
                    m_jit.store32(Imm32(u.halves[1]), MacroAssembler::Address(storageGPR, sizeof(double) * index + sizeof(int32_t)));
                }
            } else {
                int32_t* data = bitwise_cast<int32_t*>(m_jit.codeBlock()->constantBuffer(node->startConstant()));
                for (unsigned index = 0; index < node->numConstants() * 2; ++index) {
                    m_jit.store32(
                        Imm32(data[index]), MacroAssembler::Address(storageGPR, sizeof(int32_t) * index));
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
        GPRResult resultPayload(this);
        GPRResult2 resultTag(this);
        
        callOperation(operationNewRegexp, resultTag.gpr(), resultPayload.gpr(), m_jit.codeBlock()->regexp(node->regexpIndex()));
        
        // FIXME: make the callOperation above explicitly return a cell result, or jitAssert the tag is a cell tag.
        cellResult(resultPayload.gpr(), node);
        break;
    }
        
    case ConvertThis: {
        ASSERT(node->child1().useKind() == UntypedUse);

        JSValueOperand thisValue(this, node->child1());
        GPRReg thisValueTagGPR = thisValue.tagGPR();
        GPRReg thisValuePayloadGPR = thisValue.payloadGPR();
        
        flushRegisters();
        
        GPRResult2 resultTag(this);
        GPRResult resultPayload(this);
        callOperation(operationConvertThis, resultTag.gpr(), resultPayload.gpr(), thisValueTagGPR, thisValuePayloadGPR);
        
        cellResult(resultPayload.gpr(), node);
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
        m_jit.loadPtr(JITCompiler::payloadFor(static_cast<VirtualRegister>(node->codeOrigin.stackOffset() + static_cast<int>(JSStack::Callee))), result.gpr());
        cellResult(result.gpr(), node);
        break;
    }
        
    case SetCallee: {
        SpeculateCellOperand callee(this, node->child1());
        m_jit.storePtr(callee.gpr(), JITCompiler::payloadFor(static_cast<VirtualRegister>(node->codeOrigin.stackOffset() + static_cast<int>(JSStack::Callee))));
        m_jit.store32(MacroAssembler::TrustedImm32(JSValue::CellTag), JITCompiler::tagFor(static_cast<VirtualRegister>(node->codeOrigin.stackOffset() + static_cast<int>(JSStack::Callee))));
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

        m_jit.loadPtr(JITCompiler::payloadFor(static_cast<VirtualRegister>(node->codeOrigin.stackOffset() + static_cast<int>(JSStack::ScopeChain))), resultGPR);
        cellResult(resultGPR, node);
        break;
    }
        
    case SetMyScope: {
        SpeculateCellOperand callee(this, node->child1());
        m_jit.storePtr(callee.gpr(), JITCompiler::payloadFor(static_cast<VirtualRegister>(node->codeOrigin.stackOffset() + static_cast<int>(JSStack::ScopeChain))));
        noResult(node);
        break;
    }
        
    case SkipTopScope: {
        SpeculateCellOperand scope(this, node->child1());
        GPRTemporary result(this, scope);
        GPRReg resultGPR = result.gpr();
        m_jit.move(scope.gpr(), resultGPR);
        JITCompiler::Jump activationNotCreated =
            m_jit.branchTestPtr(
                JITCompiler::Zero,
                JITCompiler::payloadFor(
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
        GPRTemporary resultTag(this);
        GPRTemporary resultPayload(this);
        GPRReg registersGPR = registers.gpr();
        GPRReg resultTagGPR = resultTag.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        m_jit.load32(JITCompiler::Address(registersGPR, node->varNumber() * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)), resultTagGPR);
        m_jit.load32(JITCompiler::Address(registersGPR, node->varNumber() * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)), resultPayloadGPR);
        jsValueResult(resultTagGPR, resultPayloadGPR, node);
        break;
    }
    case PutScopedVar: {
        SpeculateCellOperand scope(this, node->child1());
        StorageOperand registers(this, node->child2());
        JSValueOperand value(this, node->child3());
        GPRTemporary scratchRegister(this);
        GPRReg scopeGPR = scope.gpr();
        GPRReg registersGPR = registers.gpr();
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
        GPRReg scratchGPR = scratchRegister.gpr();

        m_jit.store32(valueTagGPR, JITCompiler::Address(registersGPR, node->varNumber() * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
        m_jit.store32(valuePayloadGPR, JITCompiler::Address(registersGPR, node->varNumber() * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
        writeBarrier(scopeGPR, valueTagGPR, node->child2(), WriteBarrierForVariableAccess, scratchGPR);
        noResult(node);
        break;
    }
        
    case GetById: {
        if (!node->prediction()) {
            terminateSpeculativeExecution(InadequateCoverage, JSValueRegs(), 0);
            break;
        }
        
        if (isCellSpeculation(node->child1()->prediction())) {
            SpeculateCellOperand base(this, node->child1());
            GPRTemporary resultTag(this, base);
            GPRTemporary resultPayload(this);
            
            GPRReg baseGPR = base.gpr();
            GPRReg resultTagGPR = resultTag.gpr();
            GPRReg resultPayloadGPR = resultPayload.gpr();

            base.use();
            
            cachedGetById(node->codeOrigin, InvalidGPRReg, baseGPR, resultTagGPR, resultPayloadGPR, node->identifierNumber());
            
            jsValueResult(resultTagGPR, resultPayloadGPR, node, UseChildrenCalledExplicitly);
            break;
        }
        
        JSValueOperand base(this, node->child1());
        GPRTemporary resultTag(this, base);
        GPRTemporary resultPayload(this);
        
        GPRReg baseTagGPR = base.tagGPR();
        GPRReg basePayloadGPR = base.payloadGPR();
        GPRReg resultTagGPR = resultTag.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        
        base.use();
        
        JITCompiler::Jump notCell = m_jit.branch32(JITCompiler::NotEqual, baseTagGPR, TrustedImm32(JSValue::CellTag));
        
        cachedGetById(node->codeOrigin, baseTagGPR, basePayloadGPR, resultTagGPR, resultPayloadGPR, node->identifierNumber(), notCell);
        
        jsValueResult(resultTagGPR, resultPayloadGPR, node, UseChildrenCalledExplicitly);
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

            GPRResult resultTag(this);
            GPRResult2 resultPayload(this);
            GPRReg resultTagGPR = resultTag.gpr();
            GPRReg resultPayloadGPR = resultPayload.gpr();

            base.use();
            
            flushRegisters();
            
            cachedGetById(node->codeOrigin, InvalidGPRReg, baseGPR, resultTagGPR, resultPayloadGPR, node->identifierNumber(), JITCompiler::Jump(), DontSpill);
            
            jsValueResult(resultTagGPR, resultPayloadGPR, node, UseChildrenCalledExplicitly);
            break;
        }
        
        case UntypedUse: {
            JSValueOperand base(this, node->child1());
            GPRReg baseTagGPR = base.tagGPR();
            GPRReg basePayloadGPR = base.payloadGPR();

            GPRResult resultTag(this);
            GPRResult2 resultPayload(this);
            GPRReg resultTagGPR = resultTag.gpr();
            GPRReg resultPayloadGPR = resultPayload.gpr();

            base.use();
        
            flushRegisters();
        
            JITCompiler::Jump notCell = m_jit.branch32(JITCompiler::NotEqual, baseTagGPR, TrustedImm32(JSValue::CellTag));
        
            cachedGetById(node->codeOrigin, baseTagGPR, basePayloadGPR, resultTagGPR, resultPayloadGPR, node->identifierNumber(), notCell, DontSpill);
        
            jsValueResult(resultTagGPR, resultPayloadGPR, node, UseChildrenCalledExplicitly);
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
        
        if (node->structureSet().size() == 1) {
            speculationCheck(
                BadCache, JSValueSource::unboxedCell(base.gpr()), 0,
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
                BadCache, JSValueSource::unboxedCell(base.gpr()), 0,
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
        GPRTemporary resultTag(this, storage);
        GPRTemporary resultPayload(this);
        
        GPRReg storageGPR = storage.gpr();
        GPRReg resultTagGPR = resultTag.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        
        StorageAccessData& storageAccessData = m_jit.graph().m_storageAccessData[node->storageAccessDataIndex()];
        
        m_jit.load32(JITCompiler::Address(storageGPR, storageAccessData.offset * sizeof(EncodedJSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)), resultPayloadGPR);
        m_jit.load32(JITCompiler::Address(storageGPR, storageAccessData.offset * sizeof(EncodedJSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)), resultTagGPR);
        
        jsValueResult(resultTagGPR, resultPayloadGPR, node);
        break;
    }
        
    case PutByOffset: {
#if ENABLE(WRITE_BARRIER_PROFILING)
        SpeculateCellOperand base(this, node->child2());
#endif
        StorageOperand storage(this, node->child1());
        JSValueOperand value(this, node->child3());

        GPRReg storageGPR = storage.gpr();
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
        
#if ENABLE(WRITE_BARRIER_PROFILING)
        writeBarrier(base.gpr(), valueTagGPR, node->child3(), WriteBarrierForPropertyAccess);
#endif

        StorageAccessData& storageAccessData = m_jit.graph().m_storageAccessData[node->storageAccessDataIndex()];
        
        m_jit.storePtr(valueTagGPR, JITCompiler::Address(storageGPR, storageAccessData.offset * sizeof(EncodedJSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
        m_jit.storePtr(valuePayloadGPR, JITCompiler::Address(storageGPR, storageAccessData.offset * sizeof(EncodedJSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
        
        noResult(node);
        break;
    }
        
    case PutById: {
        SpeculateCellOperand base(this, node->child1());
        JSValueOperand value(this, node->child2());
        GPRTemporary scratch(this);
        
        GPRReg baseGPR = base.gpr();
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
        GPRReg scratchGPR = scratch.gpr();
        
        base.use();
        value.use();

        cachedPutById(node->codeOrigin, baseGPR, valueTagGPR, valuePayloadGPR, node->child2(), scratchGPR, node->identifierNumber(), NotDirect);
        
        noResult(node, UseChildrenCalledExplicitly);
        break;
    }

    case PutByIdDirect: {
        SpeculateCellOperand base(this, node->child1());
        JSValueOperand value(this, node->child2());
        GPRTemporary scratch(this);
        
        GPRReg baseGPR = base.gpr();
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
        GPRReg scratchGPR = scratch.gpr();
        
        base.use();
        value.use();

        cachedPutById(node->codeOrigin, baseGPR, valueTagGPR, valuePayloadGPR, node->child2(), scratchGPR, node->identifierNumber(), Direct);

        noResult(node, UseChildrenCalledExplicitly);
        break;
    }

    case GetGlobalVar: {
        GPRTemporary resultPayload(this);
        GPRTemporary resultTag(this);

        m_jit.move(TrustedImmPtr(node->registerPointer()), resultPayload.gpr());
        m_jit.load32(JITCompiler::Address(resultPayload.gpr(), OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)), resultTag.gpr());
        m_jit.load32(JITCompiler::Address(resultPayload.gpr(), OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)), resultPayload.gpr());

        jsValueResult(resultTag.gpr(), resultPayload.gpr(), node);
        break;
    }

    case PutGlobalVar: {
        JSValueOperand value(this, node->child1());
        if (Heap::isWriteBarrierEnabled()) {
            GPRTemporary scratch(this);
            GPRReg scratchReg = scratch.gpr();
            
            writeBarrier(m_jit.globalObjectFor(node->codeOrigin), value.tagGPR(), node->child1(), WriteBarrierForVariableAccess, scratchReg);
        }

        // FIXME: if we happen to have a spare register - and _ONLY_ if we happen to have
        // a spare register - a good optimization would be to put the register pointer into
        // a register and then do a zero offset store followed by a four-offset store (or
        // vice-versa depending on endianness).
        m_jit.store32(value.tagGPR(), node->registerPointer()->tagPointer());
        m_jit.store32(value.payloadGPR(), node->registerPointer()->payloadPointer());

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
            
            writeBarrier(m_jit.globalObjectFor(node->codeOrigin), value.tagGPR(), node->child1(), WriteBarrierForVariableAccess, scratchReg);
        }

        // FIXME: if we happen to have a spare register - and _ONLY_ if we happen to have
        // a spare register - a good optimization would be to put the register pointer into
        // a register and then do a zero offset store followed by a four-offset store (or
        // vice-versa depending on endianness).
        m_jit.store32(value.tagGPR(), node->registerPointer()->tagPointer());
        m_jit.store32(value.payloadGPR(), node->registerPointer()->payloadPointer());

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
        m_jit.load32(node->registerPointer()->tagPointer(), scratchGPR);
        JITCompiler::Jump notOK = m_jit.branch32(
            JITCompiler::NotEqual, scratchGPR,
            TrustedImm32(node->registerPointer()->get().tag()));
        m_jit.load32(node->registerPointer()->payloadPointer(), scratchGPR);
        JITCompiler::Jump ok = m_jit.branch32(
            JITCompiler::Equal, scratchGPR,
            TrustedImm32(node->registerPointer()->get().payload()));
        notOK.link(&m_jit);
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

        JITCompiler::Jump isCell = m_jit.branch32(JITCompiler::Equal, value.tagGPR(), JITCompiler::TrustedImm32(JSValue::CellTag));
        
        m_jit.compare32(JITCompiler::Equal, value.tagGPR(), TrustedImm32(JSValue::UndefinedTag), result.gpr());
        JITCompiler::Jump done = m_jit.jump();
        
        isCell.link(&m_jit);
        JITCompiler::Jump notMasqueradesAsUndefined;
        if (m_jit.graph().globalObjectFor(node->codeOrigin)->masqueradesAsUndefinedWatchpoint()->isStillValid()) {
            m_jit.graph().globalObjectFor(node->codeOrigin)->masqueradesAsUndefinedWatchpoint()->add(speculationWatchpoint());
            m_jit.move(TrustedImm32(0), result.gpr());
            notMasqueradesAsUndefined = m_jit.jump();
        } else {
            m_jit.loadPtr(JITCompiler::Address(value.payloadGPR(), JSCell::structureOffset()), result.gpr());
            JITCompiler::Jump isMasqueradesAsUndefined = m_jit.branchTest8(JITCompiler::NonZero, JITCompiler::Address(result.gpr(), Structure::typeInfoFlagsOffset()), TrustedImm32(MasqueradesAsUndefined));
            m_jit.move(TrustedImm32(0), result.gpr());
            notMasqueradesAsUndefined = m_jit.jump();
            
            isMasqueradesAsUndefined.link(&m_jit);
            GPRReg localGlobalObjectGPR = localGlobalObject.gpr();
            GPRReg remoteGlobalObjectGPR = remoteGlobalObject.gpr();
            m_jit.move(TrustedImmPtr(m_jit.globalObjectFor(node->codeOrigin)), localGlobalObjectGPR);
            m_jit.loadPtr(JITCompiler::Address(result.gpr(), Structure::globalObjectOffset()), remoteGlobalObjectGPR); 
            m_jit.compare32(JITCompiler::Equal, localGlobalObjectGPR, remoteGlobalObjectGPR, result.gpr());
        }

        notMasqueradesAsUndefined.link(&m_jit);
        done.link(&m_jit);
        booleanResult(result.gpr(), node);
        break;
    }

    case IsBoolean: {
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this, value);
        
        m_jit.compare32(JITCompiler::Equal, value.tagGPR(), JITCompiler::TrustedImm32(JSValue::BooleanTag), result.gpr());
        booleanResult(result.gpr(), node);
        break;
    }

    case IsNumber: {
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this, value);
        
        m_jit.add32(TrustedImm32(1), value.tagGPR(), result.gpr());
        m_jit.compare32(JITCompiler::Below, result.gpr(), JITCompiler::TrustedImm32(JSValue::LowestTag + 1), result.gpr());
        booleanResult(result.gpr(), node);
        break;
    }

    case IsString: {
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this, value);
        
        JITCompiler::Jump isNotCell = m_jit.branch32(JITCompiler::NotEqual, value.tagGPR(), JITCompiler::TrustedImm32(JSValue::CellTag));
        
        m_jit.loadPtr(JITCompiler::Address(value.payloadGPR(), JSCell::structureOffset()), result.gpr());
        m_jit.compare8(JITCompiler::Equal, JITCompiler::Address(result.gpr(), Structure::typeInfoTypeOffset()), TrustedImm32(StringType), result.gpr());
        JITCompiler::Jump done = m_jit.jump();
        
        isNotCell.link(&m_jit);
        m_jit.move(TrustedImm32(0), result.gpr());
        
        done.link(&m_jit);
        booleanResult(result.gpr(), node);
        break;
    }

    case IsObject: {
        JSValueOperand value(this, node->child1());
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();
        flushRegisters();
        callOperation(operationIsObject, resultGPR, valueTagGPR, valuePayloadGPR);
        booleanResult(result.gpr(), node);
        break;
    }

    case IsFunction: {
        JSValueOperand value(this, node->child1());
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();
        flushRegisters();
        callOperation(operationIsFunction, resultGPR, valueTagGPR, valuePayloadGPR);
        booleanResult(result.gpr(), node);
        break;
    }
    case TypeOf: {
        JSValueOperand value(this, node->child1(), ManualOperandSpeculation);
        GPRReg tagGPR = value.tagGPR();
        GPRReg payloadGPR = value.payloadGPR();
        GPRTemporary temp(this);
        GPRReg tempGPR = temp.gpr();
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();
        JITCompiler::JumpList doneJumps;

        flushRegisters();

        ASSERT(node->child1().useKind() == UntypedUse || node->child1().useKind() == CellUse || node->child1().useKind() == StringUse);

        JITCompiler::Jump isNotCell = m_jit.branch32(JITCompiler::NotEqual, tagGPR, JITCompiler::TrustedImm32(JSValue::CellTag));
        if (node->child1().useKind() != UntypedUse)
            DFG_TYPE_CHECK(JSValueRegs(tagGPR, payloadGPR), node->child1(), SpecCell, isNotCell);

        if (!node->child1()->shouldSpeculateObject() || node->child1().useKind() == StringUse) {
            m_jit.loadPtr(JITCompiler::Address(payloadGPR, JSCell::structureOffset()), tempGPR);
            JITCompiler::Jump notString = m_jit.branch8(JITCompiler::NotEqual, JITCompiler::Address(tempGPR, Structure::typeInfoTypeOffset()), TrustedImm32(StringType));
            if (node->child1().useKind() == StringUse)
                DFG_TYPE_CHECK(JSValueRegs(tagGPR, payloadGPR), node->child1(), SpecString, notString);
            m_jit.move(TrustedImmPtr(m_jit.vm()->smallStrings.stringString()), resultGPR);
            doneJumps.append(m_jit.jump());
            if (node->child1().useKind() != StringUse) {
                notString.link(&m_jit);
                callOperation(operationTypeOf, resultGPR, payloadGPR);
                doneJumps.append(m_jit.jump());
            }
        } else {
            callOperation(operationTypeOf, resultGPR, payloadGPR);
            doneJumps.append(m_jit.jump());
        }

        if (node->child1().useKind() == UntypedUse) {
            isNotCell.link(&m_jit);

            m_jit.add32(TrustedImm32(1), tagGPR, tempGPR);
            JITCompiler::Jump notNumber = m_jit.branch32(JITCompiler::AboveOrEqual, tempGPR, JITCompiler::TrustedImm32(JSValue::LowestTag + 1));
            m_jit.move(TrustedImmPtr(m_jit.vm()->smallStrings.numberString()), resultGPR);
            doneJumps.append(m_jit.jump());
            notNumber.link(&m_jit);

            JITCompiler::Jump notUndefined = m_jit.branch32(JITCompiler::NotEqual, tagGPR, TrustedImm32(JSValue::UndefinedTag));
            m_jit.move(TrustedImmPtr(m_jit.vm()->smallStrings.undefinedString()), resultGPR);
            doneJumps.append(m_jit.jump());
            notUndefined.link(&m_jit);

            JITCompiler::Jump notNull = m_jit.branch32(JITCompiler::NotEqual, tagGPR, TrustedImm32(JSValue::NullTag));
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

    case Phi:
    case Flush:
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
        GPRResult resultPayload(this);
        GPRResult2 resultTag(this);
        ResolveOperationData& data = m_jit.graph().m_resolveOperationsData[node->resolveOperationsDataIndex()];
        callOperation(operationResolve, resultTag.gpr(), resultPayload.gpr(), identifier(data.identifierNumber), data.resolveOperations);
        jsValueResult(resultTag.gpr(), resultPayload.gpr(), node);
        break;
    }

    case ResolveBase: {
        flushRegisters();
        GPRResult resultPayload(this);
        GPRResult2 resultTag(this);
        ResolveOperationData& data = m_jit.graph().m_resolveOperationsData[node->resolveOperationsDataIndex()];
        callOperation(operationResolveBase, resultTag.gpr(), resultPayload.gpr(), identifier(data.identifierNumber), data.resolveOperations, data.putToBaseOperation);
        jsValueResult(resultTag.gpr(), resultPayload.gpr(), node);
        break;
    }

    case ResolveBaseStrictPut: {
        flushRegisters();
        GPRResult resultPayload(this);
        GPRResult2 resultTag(this);
        ResolveOperationData& data = m_jit.graph().m_resolveOperationsData[node->resolveOperationsDataIndex()];
        callOperation(operationResolveBaseStrictPut, resultTag.gpr(), resultPayload.gpr(), identifier(data.identifierNumber), data.resolveOperations, data.putToBaseOperation);
        jsValueResult(resultTag.gpr(), resultPayload.gpr(), node);
        break;
    }

    case ResolveGlobal: {
        GPRTemporary globalObject(this);
        GPRTemporary resolveInfo(this);
        GPRTemporary resultTag(this);
        GPRTemporary resultPayload(this);

        GPRReg globalObjectGPR = globalObject.gpr();
        GPRReg resolveInfoGPR = resolveInfo.gpr();
        GPRReg resultTagGPR = resultTag.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();

        ResolveGlobalData& data = m_jit.graph().m_resolveGlobalData[node->resolveGlobalDataIndex()];
        ResolveOperation* resolveOperationAddress = &(data.resolveOperations->data()[data.resolvePropertyIndex]);

        // Check Structure of global object
        m_jit.move(JITCompiler::TrustedImmPtr(m_jit.globalObjectFor(node->codeOrigin)), globalObjectGPR);
        m_jit.move(JITCompiler::TrustedImmPtr(resolveOperationAddress), resolveInfoGPR);
        m_jit.loadPtr(JITCompiler::Address(resolveInfoGPR, OBJECT_OFFSETOF(ResolveOperation, m_structure)), resultPayloadGPR);

        JITCompiler::Jump structuresNotMatch = m_jit.branchPtr(JITCompiler::NotEqual, resultPayloadGPR, JITCompiler::Address(globalObjectGPR, JSCell::structureOffset()));

        // Fast case
        m_jit.loadPtr(JITCompiler::Address(globalObjectGPR, JSObject::butterflyOffset()), resultPayloadGPR);
        m_jit.load32(JITCompiler::Address(resolveInfoGPR, OBJECT_OFFSETOF(ResolveOperation, m_offset)), resolveInfoGPR);
#if DFG_ENABLE(JIT_ASSERT)
        JITCompiler::Jump isOutOfLine = m_jit.branch32(JITCompiler::GreaterThanOrEqual, resolveInfoGPR, TrustedImm32(firstOutOfLineOffset));
        m_jit.breakpoint();
        isOutOfLine.link(&m_jit);
#endif
        m_jit.neg32(resolveInfoGPR);
        m_jit.signExtend32ToPtr(resolveInfoGPR, resolveInfoGPR);
        m_jit.load32(JITCompiler::BaseIndex(resultPayloadGPR, resolveInfoGPR, JITCompiler::TimesEight, OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag) + (firstOutOfLineOffset - 2) * static_cast<ptrdiff_t>(sizeof(JSValue))), resultTagGPR);
        m_jit.load32(JITCompiler::BaseIndex(resultPayloadGPR, resolveInfoGPR, JITCompiler::TimesEight, OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload) + (firstOutOfLineOffset - 2) * static_cast<ptrdiff_t>(sizeof(JSValue))), resultPayloadGPR);

        addSlowPathGenerator(
            slowPathCall(
                structuresNotMatch, this, operationResolveGlobal,
                JSValueRegs(resultTagGPR, resultPayloadGPR), resolveInfoGPR, globalObjectGPR,
                &m_jit.codeBlock()->identifier(data.identifierNumber)));

        jsValueResult(resultTagGPR, resultPayloadGPR, node);
        break;
    }

    case CreateActivation: {
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this, value, false);
        
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
        GPRReg resultGPR = result.gpr();
        
        m_jit.move(valuePayloadGPR, resultGPR);
        
        JITCompiler::Jump notCreated = m_jit.branch32(JITCompiler::Equal, valueTagGPR, TrustedImm32(JSValue::EmptyValueTag));
        
        addSlowPathGenerator(
            slowPathCall(notCreated, this, operationCreateActivation, resultGPR));
        
        cellResult(resultGPR, node);
        break;
    }
        
    case CreateArguments: {
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this, value, false);
        
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
        GPRReg resultGPR = result.gpr();
        
        m_jit.move(valuePayloadGPR, resultGPR);
        
        JITCompiler::Jump notCreated = m_jit.branch32(JITCompiler::Equal, valueTagGPR, TrustedImm32(JSValue::EmptyValueTag));
        
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
        JSValueOperand activationValue(this, node->child1());
        GPRTemporary scratch(this);
        
        GPRReg activationValueTagGPR = activationValue.tagGPR();
        GPRReg activationValuePayloadGPR = activationValue.payloadGPR();
        GPRReg scratchGPR = scratch.gpr();

        JITCompiler::Jump notCreated = m_jit.branch32(JITCompiler::Equal, activationValueTagGPR, TrustedImm32(JSValue::EmptyValueTag));

        SharedSymbolTable* symbolTable = m_jit.symbolTableFor(node->codeOrigin);
        int registersOffset = JSActivation::registersOffset(symbolTable);

        int captureEnd = symbolTable->captureEnd();
        for (int i = symbolTable->captureStart(); i < captureEnd; ++i) {
            m_jit.loadPtr(
                JITCompiler::Address(
                    GPRInfo::callFrameRegister, i * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)),
                scratchGPR);
            m_jit.storePtr(
                scratchGPR, JITCompiler::Address(
                    activationValuePayloadGPR, registersOffset + i * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
            m_jit.loadPtr(
                JITCompiler::Address(
                    GPRInfo::callFrameRegister, i * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)),
                scratchGPR);
            m_jit.storePtr(
                scratchGPR, JITCompiler::Address(
                    activationValuePayloadGPR, registersOffset + i * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
        }
        m_jit.addPtr(TrustedImm32(registersOffset), activationValuePayloadGPR, scratchGPR);
        m_jit.storePtr(scratchGPR, JITCompiler::Address(activationValuePayloadGPR, JSActivation::offsetOfRegisters()));
        
        notCreated.link(&m_jit);
        noResult(node);
        break;
    }
        
    case TearOffArguments: {
        JSValueOperand unmodifiedArgumentsValue(this, node->child1());
        JSValueOperand activationValue(this, node->child2());
        GPRReg unmodifiedArgumentsValuePayloadGPR = unmodifiedArgumentsValue.payloadGPR();
        GPRReg activationValuePayloadGPR = activationValue.payloadGPR();
        
        JITCompiler::Jump created = m_jit.branchTest32(
            JITCompiler::NonZero, unmodifiedArgumentsValuePayloadGPR);
        
        if (node->codeOrigin.inlineCallFrame) {
            addSlowPathGenerator(
                slowPathCall(
                    created, this, operationTearOffInlinedArguments, NoResult,
                    unmodifiedArgumentsValuePayloadGPR, activationValuePayloadGPR, node->codeOrigin.inlineCallFrame));
        } else {
            addSlowPathGenerator(
                slowPathCall(
                    created, this, operationTearOffArguments, NoResult,
                    unmodifiedArgumentsValuePayloadGPR, activationValuePayloadGPR));
        }
        
        noResult(node);
        break;
    }
        
    case CheckArgumentsNotCreated: {
        ASSERT(!isEmptySpeculation(
            m_state.variables().operand(
                m_jit.graph().argumentsRegisterFor(node->codeOrigin)).m_type));
        speculationCheck(
            Uncountable, JSValueRegs(), 0,
            m_jit.branch32(
                JITCompiler::NotEqual,
                JITCompiler::tagFor(m_jit.argumentsRegisterFor(node->codeOrigin)),
                TrustedImm32(JSValue::EmptyValueTag)));
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
                m_jit.branch32(
                    JITCompiler::NotEqual,
                    JITCompiler::tagFor(m_jit.argumentsRegisterFor(node->codeOrigin)),
                    TrustedImm32(JSValue::EmptyValueTag)));
        }
        
        ASSERT(!node->codeOrigin.inlineCallFrame);
        m_jit.load32(JITCompiler::payloadFor(JSStack::ArgumentCount), resultGPR);
        m_jit.sub32(TrustedImm32(1), resultGPR);
        integerResult(resultGPR, node);
        break;
    }
        
    case GetMyArgumentsLengthSafe: {
        GPRTemporary resultPayload(this);
        GPRTemporary resultTag(this);
        GPRReg resultPayloadGPR = resultPayload.gpr();
        GPRReg resultTagGPR = resultTag.gpr();
        
        JITCompiler::Jump created = m_jit.branch32(
            JITCompiler::NotEqual,
            JITCompiler::tagFor(m_jit.argumentsRegisterFor(node->codeOrigin)),
            TrustedImm32(JSValue::EmptyValueTag));
        
        if (node->codeOrigin.inlineCallFrame) {
            m_jit.move(
                Imm32(node->codeOrigin.inlineCallFrame->arguments.size() - 1),
                resultPayloadGPR);
        } else {
            m_jit.load32(JITCompiler::payloadFor(JSStack::ArgumentCount), resultPayloadGPR);
            m_jit.sub32(TrustedImm32(1), resultPayloadGPR);
        }
        m_jit.move(TrustedImm32(JSValue::Int32Tag), resultTagGPR);
        
        // FIXME: the slow path generator should perform a forward speculation that the
        // result is an integer. For now we postpone the speculation by having this return
        // a JSValue.
        
        addSlowPathGenerator(
            slowPathCall(
                created, this, operationGetArgumentsLength,
                JSValueRegs(resultTagGPR, resultPayloadGPR),
                m_jit.argumentsRegisterFor(node->codeOrigin)));
        
        jsValueResult(resultTagGPR, resultPayloadGPR, node);
        break;
    }
        
    case GetMyArgumentByVal: {
        SpeculateStrictInt32Operand index(this, node->child1());
        GPRTemporary resultPayload(this);
        GPRTemporary resultTag(this);
        GPRReg indexGPR = index.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        GPRReg resultTagGPR = resultTag.gpr();
        
        if (!isEmptySpeculation(
                m_state.variables().operand(
                    m_jit.graph().argumentsRegisterFor(node->codeOrigin)).m_type)) {
            speculationCheck(
                ArgumentsEscaped, JSValueRegs(), 0,
                m_jit.branch32(
                    JITCompiler::NotEqual,
                    JITCompiler::tagFor(m_jit.argumentsRegisterFor(node->codeOrigin)),
                    TrustedImm32(JSValue::EmptyValueTag)));
        }
            
        m_jit.add32(TrustedImm32(1), indexGPR, resultPayloadGPR);
            
        if (node->codeOrigin.inlineCallFrame) {
            speculationCheck(
                Uncountable, JSValueRegs(), 0,
                m_jit.branch32(
                    JITCompiler::AboveOrEqual,
                    resultPayloadGPR,
                    Imm32(node->codeOrigin.inlineCallFrame->arguments.size())));
        } else {
            speculationCheck(
                Uncountable, JSValueRegs(), 0,
                m_jit.branch32(
                    JITCompiler::AboveOrEqual,
                    resultPayloadGPR,
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
            m_jit.move(ImmPtr(slowArguments), resultPayloadGPR);
            m_jit.load32(
                JITCompiler::BaseIndex(
                    resultPayloadGPR, indexGPR, JITCompiler::TimesEight, 
                    OBJECT_OFFSETOF(SlowArgument, index)), 
                resultPayloadGPR);

            m_jit.load32(
                JITCompiler::BaseIndex(
                    GPRInfo::callFrameRegister, resultPayloadGPR, JITCompiler::TimesEight,
                    m_jit.offsetOfLocals(node->codeOrigin) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)),
                resultTagGPR);
            m_jit.load32(
                JITCompiler::BaseIndex(
                    GPRInfo::callFrameRegister, resultPayloadGPR, JITCompiler::TimesEight,
                    m_jit.offsetOfLocals(node->codeOrigin) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)),
                resultPayloadGPR);
            slowArgument.append(m_jit.jump());
        }
        slowArgumentOutOfBounds.link(&m_jit);

        m_jit.neg32(resultPayloadGPR);
        
        m_jit.load32(
            JITCompiler::BaseIndex(
                GPRInfo::callFrameRegister, resultPayloadGPR, JITCompiler::TimesEight,
                m_jit.offsetOfArgumentsIncludingThis(node->codeOrigin) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)),
            resultTagGPR);
        m_jit.load32(
            JITCompiler::BaseIndex(
                GPRInfo::callFrameRegister, resultPayloadGPR, JITCompiler::TimesEight,
                m_jit.offsetOfArgumentsIncludingThis(node->codeOrigin) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)),
            resultPayloadGPR);
            
        slowArgument.link(&m_jit);
        jsValueResult(resultTagGPR, resultPayloadGPR, node);
        break;
    }
    case GetMyArgumentByValSafe: {
        SpeculateStrictInt32Operand index(this, node->child1());
        GPRTemporary resultPayload(this);
        GPRTemporary resultTag(this);
        GPRReg indexGPR = index.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        GPRReg resultTagGPR = resultTag.gpr();
        
        JITCompiler::JumpList slowPath;
        slowPath.append(
            m_jit.branch32(
                JITCompiler::NotEqual,
                JITCompiler::tagFor(m_jit.argumentsRegisterFor(node->codeOrigin)),
                TrustedImm32(JSValue::EmptyValueTag)));
        
        m_jit.add32(TrustedImm32(1), indexGPR, resultPayloadGPR);
        if (node->codeOrigin.inlineCallFrame) {
            slowPath.append(
                m_jit.branch32(
                    JITCompiler::AboveOrEqual,
                    resultPayloadGPR,
                    Imm32(node->codeOrigin.inlineCallFrame->arguments.size())));
        } else {
            slowPath.append(
                m_jit.branch32(
                    JITCompiler::AboveOrEqual,
                    resultPayloadGPR,
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
            m_jit.move(ImmPtr(slowArguments), resultPayloadGPR);
            m_jit.load32(
                JITCompiler::BaseIndex(
                    resultPayloadGPR, indexGPR, JITCompiler::TimesEight, 
                    OBJECT_OFFSETOF(SlowArgument, index)), 
                resultPayloadGPR);
            m_jit.load32(
                JITCompiler::BaseIndex(
                    GPRInfo::callFrameRegister, resultPayloadGPR, JITCompiler::TimesEight,
                    m_jit.offsetOfLocals(node->codeOrigin) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)),
                resultTagGPR);
            m_jit.load32(
                JITCompiler::BaseIndex(
                    GPRInfo::callFrameRegister, resultPayloadGPR, JITCompiler::TimesEight,
                    m_jit.offsetOfLocals(node->codeOrigin) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)),
                resultPayloadGPR);
            slowArgument.append(m_jit.jump());
        }
        slowArgumentOutOfBounds.link(&m_jit);

        m_jit.neg32(resultPayloadGPR);
        
        m_jit.load32(
            JITCompiler::BaseIndex(
                GPRInfo::callFrameRegister, resultPayloadGPR, JITCompiler::TimesEight,
                m_jit.offsetOfArgumentsIncludingThis(node->codeOrigin) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)),
            resultTagGPR);
        m_jit.load32(
            JITCompiler::BaseIndex(
                GPRInfo::callFrameRegister, resultPayloadGPR, JITCompiler::TimesEight,
                m_jit.offsetOfArgumentsIncludingThis(node->codeOrigin) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)),
            resultPayloadGPR);
        
        if (node->codeOrigin.inlineCallFrame) {
            addSlowPathGenerator(
                slowPathCall(
                    slowPath, this, operationGetInlinedArgumentByVal,
                    JSValueRegs(resultTagGPR, resultPayloadGPR),
                    m_jit.argumentsRegisterFor(node->codeOrigin),
                    node->codeOrigin.inlineCallFrame, indexGPR));
        } else {
            addSlowPathGenerator(
                slowPathCall(
                    slowPath, this, operationGetArgumentByVal,
                    JSValueRegs(resultTagGPR, resultPayloadGPR),
                    m_jit.argumentsRegisterFor(node->codeOrigin), indexGPR));
        }
        
        slowArgument.link(&m_jit);
        jsValueResult(resultTagGPR, resultPayloadGPR, node);
        break;
    }
        
    case NewFunctionNoCheck:
        compileNewFunctionNoCheck(node);
        break;
        
    case NewFunction: {
        JSValueOperand value(this, node->child1());
        GPRTemporary resultTag(this, value);
        GPRTemporary resultPayload(this, value, false);
        
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
        GPRReg resultTagGPR = resultTag.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        
        m_jit.move(valuePayloadGPR, resultPayloadGPR);
        m_jit.move(valueTagGPR, resultTagGPR);
        
        JITCompiler::Jump notCreated = m_jit.branch32(JITCompiler::Equal, valueTagGPR, TrustedImm32(JSValue::EmptyValueTag));
        
        addSlowPathGenerator(
            slowPathCall(
                notCreated, this, operationNewFunction, JSValueRegs(resultTagGPR, resultPayloadGPR),
                m_jit.codeBlock()->functionDecl(node->functionDeclIndex())));
        
        jsValueResult(resultTagGPR, resultPayloadGPR, node);
        break;
    }
        
    case NewFunctionExpression:
        compileNewFunctionExpression(node);
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

    case CountExecution:
        m_jit.add64(TrustedImm32(1), MacroAssembler::AbsoluteAddress(node->executionCounter()->address()));
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
