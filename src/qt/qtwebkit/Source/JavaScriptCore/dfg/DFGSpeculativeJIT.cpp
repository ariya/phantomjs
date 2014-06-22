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
#include "DFGArrayifySlowPathGenerator.h"
#include "DFGCallArrayAllocatorSlowPathGenerator.h"
#include "DFGSlowPathGenerator.h"
#include "JSCJSValueInlines.h"
#include "LinkBuffer.h"

namespace JSC { namespace DFG {

SpeculativeJIT::SpeculativeJIT(JITCompiler& jit)
    : m_compileOkay(true)
    , m_jit(jit)
    , m_currentNode(0)
    , m_indexInBlock(0)
    , m_generationInfo(m_jit.codeBlock()->m_numCalleeRegisters)
    , m_blockHeads(jit.graph().m_blocks.size())
    , m_arguments(jit.codeBlock()->numParameters())
    , m_variables(jit.graph().m_localVars)
    , m_lastSetOperand(std::numeric_limits<int>::max())
    , m_state(m_jit.graph())
    , m_stream(&jit.codeBlock()->variableEventStream())
    , m_minifiedGraph(&jit.codeBlock()->minifiedDFG())
    , m_isCheckingArgumentTypes(false)
{
}

SpeculativeJIT::~SpeculativeJIT()
{
}

void SpeculativeJIT::emitAllocateJSArray(GPRReg resultGPR, Structure* structure, GPRReg storageGPR, unsigned numElements)
{
    ASSERT(hasUndecided(structure->indexingType()) || hasInt32(structure->indexingType()) || hasDouble(structure->indexingType()) || hasContiguous(structure->indexingType()));
    
    GPRTemporary scratch(this);
    GPRTemporary scratch2(this);
    GPRReg scratchGPR = scratch.gpr();
    GPRReg scratch2GPR = scratch2.gpr();
    
    unsigned vectorLength = std::max(BASE_VECTOR_LEN, numElements);
    
    JITCompiler::JumpList slowCases;
    
    slowCases.append(
        emitAllocateBasicStorage(TrustedImm32(vectorLength * sizeof(JSValue) + sizeof(IndexingHeader)), storageGPR));
    m_jit.subPtr(TrustedImm32(vectorLength * sizeof(JSValue)), storageGPR);
    emitAllocateJSObject<JSArray>(resultGPR, TrustedImmPtr(structure), storageGPR, scratchGPR, scratch2GPR, slowCases);
    
    m_jit.store32(TrustedImm32(numElements), MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()));
    m_jit.store32(TrustedImm32(vectorLength), MacroAssembler::Address(storageGPR, Butterfly::offsetOfVectorLength()));
    
    if (hasDouble(structure->indexingType()) && numElements < vectorLength) {
#if USE(JSVALUE64)
        m_jit.move(TrustedImm64(bitwise_cast<int64_t>(QNaN)), scratchGPR);
        for (unsigned i = numElements; i < vectorLength; ++i)
            m_jit.store64(scratchGPR, MacroAssembler::Address(storageGPR, sizeof(double) * i));
#else
        EncodedValueDescriptor value;
        value.asInt64 = JSValue::encode(JSValue(JSValue::EncodeAsDouble, QNaN));
        for (unsigned i = numElements; i < vectorLength; ++i) {
            m_jit.store32(TrustedImm32(value.asBits.tag), MacroAssembler::Address(storageGPR, sizeof(double) * i + OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
            m_jit.store32(TrustedImm32(value.asBits.payload), MacroAssembler::Address(storageGPR, sizeof(double) * i + OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
        }
#endif
    }
    
    // I want a slow path that also loads out the storage pointer, and that's
    // what this custom CallArrayAllocatorSlowPathGenerator gives me. It's a lot
    // of work for a very small piece of functionality. :-/
    addSlowPathGenerator(adoptPtr(
        new CallArrayAllocatorSlowPathGenerator(
            slowCases, this, operationNewArrayWithSize, resultGPR, storageGPR,
            structure, numElements)));
}

void SpeculativeJIT::backwardSpeculationCheck(ExitKind kind, JSValueSource jsValueSource, Node* node, MacroAssembler::Jump jumpToFail)
{
    if (!m_compileOkay)
        return;
    ASSERT(m_isCheckingArgumentTypes || m_canExit);
    m_jit.appendExitInfo(jumpToFail);
    m_jit.codeBlock()->appendOSRExit(OSRExit(kind, jsValueSource, m_jit.graph().methodOfGettingAValueProfileFor(node), this, m_stream->size()));
}

void SpeculativeJIT::backwardSpeculationCheck(ExitKind kind, JSValueSource jsValueSource, Node* node, const MacroAssembler::JumpList& jumpsToFail)
{
    if (!m_compileOkay)
        return;
    ASSERT(m_isCheckingArgumentTypes || m_canExit);
    m_jit.appendExitInfo(jumpsToFail);
    m_jit.codeBlock()->appendOSRExit(OSRExit(kind, jsValueSource, m_jit.graph().methodOfGettingAValueProfileFor(node), this, m_stream->size()));
}

void SpeculativeJIT::speculationCheck(ExitKind kind, JSValueSource jsValueSource, Node* node, MacroAssembler::Jump jumpToFail)
{
    if (!m_compileOkay)
        return;
    backwardSpeculationCheck(kind, jsValueSource, node, jumpToFail);
    if (m_speculationDirection == ForwardSpeculation)
        convertLastOSRExitToForward();
}

void SpeculativeJIT::speculationCheck(ExitKind kind, JSValueSource jsValueSource, Edge nodeUse, MacroAssembler::Jump jumpToFail)
{
    ASSERT(m_isCheckingArgumentTypes || m_canExit);
    speculationCheck(kind, jsValueSource, nodeUse.node(), jumpToFail);
}

OSRExitJumpPlaceholder SpeculativeJIT::backwardSpeculationCheck(ExitKind kind, JSValueSource jsValueSource, Node* node)
{
    if (!m_compileOkay)
        return OSRExitJumpPlaceholder();
    ASSERT(m_isCheckingArgumentTypes || m_canExit);
    unsigned index = m_jit.codeBlock()->numberOfOSRExits();
    m_jit.appendExitInfo();
    m_jit.codeBlock()->appendOSRExit(OSRExit(kind, jsValueSource, m_jit.graph().methodOfGettingAValueProfileFor(node), this, m_stream->size()));
    return OSRExitJumpPlaceholder(index);
}

OSRExitJumpPlaceholder SpeculativeJIT::backwardSpeculationCheck(ExitKind kind, JSValueSource jsValueSource, Edge nodeUse)
{
    ASSERT(m_isCheckingArgumentTypes || m_canExit);
    return backwardSpeculationCheck(kind, jsValueSource, nodeUse.node());
}

void SpeculativeJIT::speculationCheck(ExitKind kind, JSValueSource jsValueSource, Node* node, const MacroAssembler::JumpList& jumpsToFail)
{
    if (!m_compileOkay)
        return;
    backwardSpeculationCheck(kind, jsValueSource, node, jumpsToFail);
    if (m_speculationDirection == ForwardSpeculation)
        convertLastOSRExitToForward();
}

void SpeculativeJIT::speculationCheck(ExitKind kind, JSValueSource jsValueSource, Edge nodeUse, const MacroAssembler::JumpList& jumpsToFail)
{
    ASSERT(m_isCheckingArgumentTypes || m_canExit);
    speculationCheck(kind, jsValueSource, nodeUse.node(), jumpsToFail);
}

void SpeculativeJIT::backwardSpeculationCheck(ExitKind kind, JSValueSource jsValueSource, Node* node, MacroAssembler::Jump jumpToFail, const SpeculationRecovery& recovery)
{
    if (!m_compileOkay)
        return;
    ASSERT(m_isCheckingArgumentTypes || m_canExit);
    m_jit.codeBlock()->appendSpeculationRecovery(recovery);
    m_jit.appendExitInfo(jumpToFail);
    m_jit.codeBlock()->appendOSRExit(OSRExit(kind, jsValueSource, m_jit.graph().methodOfGettingAValueProfileFor(node), this, m_stream->size(), m_jit.codeBlock()->numberOfSpeculationRecoveries()));
}

void SpeculativeJIT::backwardSpeculationCheck(ExitKind kind, JSValueSource jsValueSource, Edge nodeUse, MacroAssembler::Jump jumpToFail, const SpeculationRecovery& recovery)
{
    ASSERT(m_isCheckingArgumentTypes || m_canExit);
    backwardSpeculationCheck(kind, jsValueSource, nodeUse.node(), jumpToFail, recovery);
}

void SpeculativeJIT::speculationCheck(ExitKind kind, JSValueSource jsValueSource, Node* node, MacroAssembler::Jump jumpToFail, const SpeculationRecovery& recovery)
{
    if (!m_compileOkay)
        return;
    backwardSpeculationCheck(kind, jsValueSource, node, jumpToFail, recovery);
    if (m_speculationDirection == ForwardSpeculation)
        convertLastOSRExitToForward();
}

void SpeculativeJIT::speculationCheck(ExitKind kind, JSValueSource jsValueSource, Edge edge, MacroAssembler::Jump jumpToFail, const SpeculationRecovery& recovery)
{
    speculationCheck(kind, jsValueSource, edge.node(), jumpToFail, recovery);
}

JumpReplacementWatchpoint* SpeculativeJIT::speculationWatchpoint(ExitKind kind, JSValueSource jsValueSource, Node* node)
{
    if (!m_compileOkay)
        return 0;
    ASSERT(m_isCheckingArgumentTypes || m_canExit);
    m_jit.appendExitInfo(JITCompiler::JumpList());
    OSRExit& exit = m_jit.codeBlock()->osrExit(
        m_jit.codeBlock()->appendOSRExit(OSRExit(
            kind, jsValueSource,
            m_jit.graph().methodOfGettingAValueProfileFor(node),
            this, m_stream->size())));
    exit.m_watchpointIndex = m_jit.codeBlock()->appendWatchpoint(
        JumpReplacementWatchpoint(m_jit.watchpointLabel()));
    if (m_speculationDirection == ForwardSpeculation)
        convertLastOSRExitToForward();
    return &m_jit.codeBlock()->watchpoint(exit.m_watchpointIndex);
}

JumpReplacementWatchpoint* SpeculativeJIT::speculationWatchpoint(ExitKind kind)
{
    return speculationWatchpoint(kind, JSValueSource(), 0);
}

void SpeculativeJIT::convertLastOSRExitToForward(const ValueRecovery& valueRecovery)
{
    if (!valueRecovery) {
        // Check that either the current node is a SetLocal, or the preceding node was a
        // SetLocal with the same code origin.
        if (!m_currentNode->containsMovHint()) {
            Node* setLocal = m_jit.graph().m_blocks[m_block]->at(m_indexInBlock - 1);
            ASSERT_UNUSED(setLocal, setLocal->containsMovHint());
            ASSERT_UNUSED(setLocal, setLocal->codeOrigin == m_currentNode->codeOrigin);
        }
        
        // Find the next node.
        unsigned indexInBlock = m_indexInBlock + 1;
        Node* node = 0;
        for (;;) {
            if (indexInBlock == m_jit.graph().m_blocks[m_block]->size()) {
                // This is an inline return. Give up and do a backwards speculation. This is safe
                // because an inline return has its own bytecode index and it's always safe to
                // reexecute that bytecode.
                ASSERT(node->op() == Jump);
                return;
            }
            node = m_jit.graph().m_blocks[m_block]->at(indexInBlock);
            if (node->codeOrigin != m_currentNode->codeOrigin)
                break;
            indexInBlock++;
        }
        
        ASSERT(node->codeOrigin != m_currentNode->codeOrigin);
        OSRExit& exit = m_jit.codeBlock()->lastOSRExit();
        exit.m_codeOrigin = node->codeOrigin;
        return;
    }
    
    unsigned setLocalIndexInBlock = m_indexInBlock + 1;
    
    Node* setLocal = m_jit.graph().m_blocks[m_block]->at(setLocalIndexInBlock);
    bool hadInt32ToDouble = false;
    
    if (setLocal->op() == ForwardInt32ToDouble) {
        setLocal = m_jit.graph().m_blocks[m_block]->at(++setLocalIndexInBlock);
        hadInt32ToDouble = true;
    }
    if (setLocal->op() == Flush || setLocal->op() == Phantom)
        setLocal = m_jit.graph().m_blocks[m_block]->at(++setLocalIndexInBlock);
        
    if (hadInt32ToDouble)
        ASSERT(setLocal->child1()->child1() == m_currentNode);
    else
        ASSERT(setLocal->child1() == m_currentNode);
    ASSERT(setLocal->containsMovHint());
    ASSERT(setLocal->codeOrigin == m_currentNode->codeOrigin);

    Node* nextNode = m_jit.graph().m_blocks[m_block]->at(setLocalIndexInBlock + 1);
    if (nextNode->op() == Jump && nextNode->codeOrigin == m_currentNode->codeOrigin) {
        // We're at an inlined return. Use a backward speculation instead.
        return;
    }
    ASSERT(nextNode->codeOrigin != m_currentNode->codeOrigin);
        
    OSRExit& exit = m_jit.codeBlock()->lastOSRExit();
    exit.m_codeOrigin = nextNode->codeOrigin;
        
    exit.m_lastSetOperand = setLocal->local();
    exit.m_valueRecoveryOverride = adoptRef(
        new ValueRecoveryOverride(setLocal->local(), valueRecovery));
}

void SpeculativeJIT::forwardSpeculationCheck(ExitKind kind, JSValueSource jsValueSource, Node* node, MacroAssembler::Jump jumpToFail, const ValueRecovery& valueRecovery)
{
    ASSERT(m_isCheckingArgumentTypes || m_canExit);
    backwardSpeculationCheck(kind, jsValueSource, node, jumpToFail);
    convertLastOSRExitToForward(valueRecovery);
}

void SpeculativeJIT::forwardSpeculationCheck(ExitKind kind, JSValueSource jsValueSource, Node* node, const MacroAssembler::JumpList& jumpsToFail, const ValueRecovery& valueRecovery)
{
    ASSERT(m_isCheckingArgumentTypes || m_canExit);
    backwardSpeculationCheck(kind, jsValueSource, node, jumpsToFail);
    convertLastOSRExitToForward(valueRecovery);
}

void SpeculativeJIT::terminateSpeculativeExecution(ExitKind kind, JSValueRegs jsValueRegs, Node* node)
{
    ASSERT(m_isCheckingArgumentTypes || m_canExit);
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLogF("SpeculativeJIT was terminated.\n");
#endif
    if (!m_compileOkay)
        return;
    speculationCheck(kind, jsValueRegs, node, m_jit.jump());
    m_compileOkay = false;
}

void SpeculativeJIT::terminateSpeculativeExecution(ExitKind kind, JSValueRegs jsValueRegs, Edge nodeUse)
{
    ASSERT(m_isCheckingArgumentTypes || m_canExit);
    terminateSpeculativeExecution(kind, jsValueRegs, nodeUse.node());
}

void SpeculativeJIT::backwardTypeCheck(JSValueSource source, Edge edge, SpeculatedType typesPassedThrough, MacroAssembler::Jump jumpToFail)
{
    ASSERT(needsTypeCheck(edge, typesPassedThrough));
    m_state.forNode(edge).filter(typesPassedThrough);
    backwardSpeculationCheck(BadType, source, edge.node(), jumpToFail);
}

void SpeculativeJIT::typeCheck(JSValueSource source, Edge edge, SpeculatedType typesPassedThrough, MacroAssembler::Jump jumpToFail)
{
    backwardTypeCheck(source, edge, typesPassedThrough, jumpToFail);
    if (m_speculationDirection == ForwardSpeculation)
        convertLastOSRExitToForward();
}

void SpeculativeJIT::forwardTypeCheck(JSValueSource source, Edge edge, SpeculatedType typesPassedThrough, MacroAssembler::Jump jumpToFail, const ValueRecovery& valueRecovery)
{
    backwardTypeCheck(source, edge, typesPassedThrough, jumpToFail);
    convertLastOSRExitToForward(valueRecovery);
}

void SpeculativeJIT::addSlowPathGenerator(PassOwnPtr<SlowPathGenerator> slowPathGenerator)
{
    m_slowPathGenerators.append(slowPathGenerator);
}

void SpeculativeJIT::runSlowPathGenerators()
{
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLogF("Running %lu slow path generators.\n", m_slowPathGenerators.size());
#endif
    for (unsigned i = 0; i < m_slowPathGenerators.size(); ++i)
        m_slowPathGenerators[i]->generate(this);
}

// On Windows we need to wrap fmod; on other platforms we can call it directly.
// On ARMv7 we assert that all function pointers have to low bit set (point to thumb code).
#if CALLING_CONVENTION_IS_STDCALL || CPU(ARM_THUMB2)
static double DFG_OPERATION fmodAsDFGOperation(double x, double y)
{
    return fmod(x, y);
}
#else
#define fmodAsDFGOperation fmod
#endif

void SpeculativeJIT::clearGenerationInfo()
{
    for (unsigned i = 0; i < m_generationInfo.size(); ++i)
        m_generationInfo[i] = GenerationInfo();
    m_gprs = RegisterBank<GPRInfo>();
    m_fprs = RegisterBank<FPRInfo>();
}

SilentRegisterSavePlan SpeculativeJIT::silentSavePlanForGPR(VirtualRegister spillMe, GPRReg source)
{
    GenerationInfo& info = m_generationInfo[spillMe];
    Node* node = info.node();
    DataFormat registerFormat = info.registerFormat();
    ASSERT(registerFormat != DataFormatNone);
    ASSERT(registerFormat != DataFormatDouble);
        
    SilentSpillAction spillAction;
    SilentFillAction fillAction;
        
    if (!info.needsSpill())
        spillAction = DoNothingForSpill;
    else {
#if USE(JSVALUE64)
        ASSERT(info.gpr() == source);
        if (registerFormat == DataFormatInteger)
            spillAction = Store32Payload;
        else if (registerFormat == DataFormatCell || registerFormat == DataFormatStorage)
            spillAction = StorePtr;
        else {
            ASSERT(registerFormat & DataFormatJS);
            spillAction = Store64;
        }
#elif USE(JSVALUE32_64)
        if (registerFormat & DataFormatJS) {
            ASSERT(info.tagGPR() == source || info.payloadGPR() == source);
            spillAction = source == info.tagGPR() ? Store32Tag : Store32Payload;
        } else {
            ASSERT(info.gpr() == source);
            spillAction = Store32Payload;
        }
#endif
    }
        
    if (registerFormat == DataFormatInteger) {
        ASSERT(info.gpr() == source);
        ASSERT(isJSInteger(info.registerFormat()));
        if (node->hasConstant()) {
            ASSERT(isInt32Constant(node));
            fillAction = SetInt32Constant;
        } else
            fillAction = Load32Payload;
    } else if (registerFormat == DataFormatBoolean) {
#if USE(JSVALUE64)
        RELEASE_ASSERT_NOT_REACHED();
        fillAction = DoNothingForFill;
#elif USE(JSVALUE32_64)
        ASSERT(info.gpr() == source);
        if (node->hasConstant()) {
            ASSERT(isBooleanConstant(node));
            fillAction = SetBooleanConstant;
        } else
            fillAction = Load32Payload;
#endif
    } else if (registerFormat == DataFormatCell) {
        ASSERT(info.gpr() == source);
        if (node->hasConstant()) {
            JSValue value = valueOfJSConstant(node);
            ASSERT_UNUSED(value, value.isCell());
            fillAction = SetCellConstant;
        } else {
#if USE(JSVALUE64)
            fillAction = LoadPtr;
#else
            fillAction = Load32Payload;
#endif
        }
    } else if (registerFormat == DataFormatStorage) {
        ASSERT(info.gpr() == source);
        fillAction = LoadPtr;
    } else {
        ASSERT(registerFormat & DataFormatJS);
#if USE(JSVALUE64)
        ASSERT(info.gpr() == source);
        if (node->hasConstant()) {
            if (valueOfJSConstant(node).isCell())
                fillAction = SetTrustedJSConstant;
            else
                fillAction = SetJSConstant;
        } else if (info.spillFormat() == DataFormatInteger) {
            ASSERT(registerFormat == DataFormatJSInteger);
            fillAction = Load32PayloadBoxInt;
        } else if (info.spillFormat() == DataFormatDouble) {
            ASSERT(registerFormat == DataFormatJSDouble);
            fillAction = LoadDoubleBoxDouble;
        } else
            fillAction = Load64;
#else
        ASSERT(info.tagGPR() == source || info.payloadGPR() == source);
        if (node->hasConstant())
            fillAction = info.tagGPR() == source ? SetJSConstantTag : SetJSConstantPayload;
        else if (info.payloadGPR() == source)
            fillAction = Load32Payload;
        else { // Fill the Tag
            switch (info.spillFormat()) {
            case DataFormatInteger:
                ASSERT(registerFormat == DataFormatJSInteger);
                fillAction = SetInt32Tag;
                break;
            case DataFormatCell:
                ASSERT(registerFormat == DataFormatJSCell);
                fillAction = SetCellTag;
                break;
            case DataFormatBoolean:
                ASSERT(registerFormat == DataFormatJSBoolean);
                fillAction = SetBooleanTag;
                break;
            default:
                fillAction = Load32Tag;
                break;
            }
        }
#endif
    }
        
    return SilentRegisterSavePlan(spillAction, fillAction, node, source);
}
    
SilentRegisterSavePlan SpeculativeJIT::silentSavePlanForFPR(VirtualRegister spillMe, FPRReg source)
{
    GenerationInfo& info = m_generationInfo[spillMe];
    Node* node = info.node();
    ASSERT(info.registerFormat() == DataFormatDouble);

    SilentSpillAction spillAction;
    SilentFillAction fillAction;
        
    if (!info.needsSpill())
        spillAction = DoNothingForSpill;
    else {
        ASSERT(!node->hasConstant());
        ASSERT(info.spillFormat() == DataFormatNone);
        ASSERT(info.fpr() == source);
        spillAction = StoreDouble;
    }
        
#if USE(JSVALUE64)
    if (node->hasConstant()) {
        ASSERT(isNumberConstant(node));
        fillAction = SetDoubleConstant;
    } else if (info.spillFormat() != DataFormatNone && info.spillFormat() != DataFormatDouble) {
        // it was already spilled previously and not as a double, which means we need unboxing.
        ASSERT(info.spillFormat() & DataFormatJS);
        fillAction = LoadJSUnboxDouble;
    } else
        fillAction = LoadDouble;
#elif USE(JSVALUE32_64)
    ASSERT(info.registerFormat() == DataFormatDouble || info.registerFormat() == DataFormatJSDouble);
    if (node->hasConstant()) {
        ASSERT(isNumberConstant(node));
        fillAction = SetDoubleConstant;
    } else
        fillAction = LoadDouble;
#endif

    return SilentRegisterSavePlan(spillAction, fillAction, node, source);
}
    
void SpeculativeJIT::silentSpill(const SilentRegisterSavePlan& plan)
{
    switch (plan.spillAction()) {
    case DoNothingForSpill:
        break;
    case Store32Tag:
        m_jit.store32(plan.gpr(), JITCompiler::tagFor(plan.node()->virtualRegister()));
        break;
    case Store32Payload:
        m_jit.store32(plan.gpr(), JITCompiler::payloadFor(plan.node()->virtualRegister()));
        break;
    case StorePtr:
        m_jit.storePtr(plan.gpr(), JITCompiler::addressFor(plan.node()->virtualRegister()));
        break;
#if USE(JSVALUE64)
    case Store64:
        m_jit.store64(plan.gpr(), JITCompiler::addressFor(plan.node()->virtualRegister()));
        break;
#endif
    case StoreDouble:
        m_jit.storeDouble(plan.fpr(), JITCompiler::addressFor(plan.node()->virtualRegister()));
        break;
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}
    
void SpeculativeJIT::silentFill(const SilentRegisterSavePlan& plan, GPRReg canTrample)
{
#if USE(JSVALUE32_64)
    UNUSED_PARAM(canTrample);
#endif
    switch (plan.fillAction()) {
    case DoNothingForFill:
        break;
    case SetInt32Constant:
        m_jit.move(Imm32(valueOfInt32Constant(plan.node())), plan.gpr());
        break;
    case SetBooleanConstant:
        m_jit.move(TrustedImm32(valueOfBooleanConstant(plan.node())), plan.gpr());
        break;
    case SetCellConstant:
        m_jit.move(TrustedImmPtr(valueOfJSConstant(plan.node()).asCell()), plan.gpr());
        break;
#if USE(JSVALUE64)
    case SetTrustedJSConstant:
        m_jit.move(valueOfJSConstantAsImm64(plan.node()).asTrustedImm64(), plan.gpr());
        break;
    case SetJSConstant:
        m_jit.move(valueOfJSConstantAsImm64(plan.node()), plan.gpr());
        break;
    case SetDoubleConstant:
        m_jit.move(Imm64(reinterpretDoubleToInt64(valueOfNumberConstant(plan.node()))), canTrample);
        m_jit.move64ToDouble(canTrample, plan.fpr());
        break;
    case Load32PayloadBoxInt:
        m_jit.load32(JITCompiler::payloadFor(plan.node()->virtualRegister()), plan.gpr());
        m_jit.or64(GPRInfo::tagTypeNumberRegister, plan.gpr());
        break;
    case LoadDoubleBoxDouble:
        m_jit.load64(JITCompiler::addressFor(plan.node()->virtualRegister()), plan.gpr());
        m_jit.sub64(GPRInfo::tagTypeNumberRegister, plan.gpr());
        break;
    case LoadJSUnboxDouble:
        m_jit.load64(JITCompiler::addressFor(plan.node()->virtualRegister()), canTrample);
        unboxDouble(canTrample, plan.fpr());
        break;
#else
    case SetJSConstantTag:
        m_jit.move(Imm32(valueOfJSConstant(plan.node()).tag()), plan.gpr());
        break;
    case SetJSConstantPayload:
        m_jit.move(Imm32(valueOfJSConstant(plan.node()).payload()), plan.gpr());
        break;
    case SetInt32Tag:
        m_jit.move(TrustedImm32(JSValue::Int32Tag), plan.gpr());
        break;
    case SetCellTag:
        m_jit.move(TrustedImm32(JSValue::CellTag), plan.gpr());
        break;
    case SetBooleanTag:
        m_jit.move(TrustedImm32(JSValue::BooleanTag), plan.gpr());
        break;
    case SetDoubleConstant:
        m_jit.loadDouble(addressOfDoubleConstant(plan.node()), plan.fpr());
        break;
#endif
    case Load32Tag:
        m_jit.load32(JITCompiler::tagFor(plan.node()->virtualRegister()), plan.gpr());
        break;
    case Load32Payload:
        m_jit.load32(JITCompiler::payloadFor(plan.node()->virtualRegister()), plan.gpr());
        break;
    case LoadPtr:
        m_jit.loadPtr(JITCompiler::addressFor(plan.node()->virtualRegister()), plan.gpr());
        break;
#if USE(JSVALUE64)
    case Load64:
        m_jit.load64(JITCompiler::addressFor(plan.node()->virtualRegister()), plan.gpr());
        break;
#endif
    case LoadDouble:
        m_jit.loadDouble(JITCompiler::addressFor(plan.node()->virtualRegister()), plan.fpr());
        break;
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}
    
const TypedArrayDescriptor* SpeculativeJIT::typedArrayDescriptor(ArrayMode arrayMode)
{
    switch (arrayMode.type()) {
    case Array::Int8Array:
        return &m_jit.vm()->int8ArrayDescriptor();
    case Array::Int16Array:
        return &m_jit.vm()->int16ArrayDescriptor();
    case Array::Int32Array:
        return &m_jit.vm()->int32ArrayDescriptor();
    case Array::Uint8Array:
        return &m_jit.vm()->uint8ArrayDescriptor();
    case Array::Uint8ClampedArray:
        return &m_jit.vm()->uint8ClampedArrayDescriptor();
    case Array::Uint16Array:
        return &m_jit.vm()->uint16ArrayDescriptor();
    case Array::Uint32Array:
        return &m_jit.vm()->uint32ArrayDescriptor();
    case Array::Float32Array:
        return &m_jit.vm()->float32ArrayDescriptor();
    case Array::Float64Array:
        return &m_jit.vm()->float64ArrayDescriptor();
    default:
        return 0;
    }
}

JITCompiler::Jump SpeculativeJIT::jumpSlowForUnwantedArrayMode(GPRReg tempGPR, ArrayMode arrayMode, IndexingType shape)
{
    switch (arrayMode.arrayClass()) {
    case Array::OriginalArray: {
        CRASH();
        JITCompiler::Jump result; // I already know that VC++ takes unkindly to the expression "return Jump()", so I'm doing it this way in anticipation of someone eventually using VC++ to compile the DFG.
        return result;
    }
        
    case Array::Array:
        m_jit.and32(TrustedImm32(IsArray | IndexingShapeMask), tempGPR);
        return m_jit.branch32(
            MacroAssembler::NotEqual, tempGPR, TrustedImm32(IsArray | shape));
        
    default:
        m_jit.and32(TrustedImm32(IndexingShapeMask), tempGPR);
        return m_jit.branch32(MacroAssembler::NotEqual, tempGPR, TrustedImm32(shape));
    }
}

JITCompiler::JumpList SpeculativeJIT::jumpSlowForUnwantedArrayMode(GPRReg tempGPR, ArrayMode arrayMode)
{
    JITCompiler::JumpList result;
    
    switch (arrayMode.type()) {
    case Array::Int32:
        return jumpSlowForUnwantedArrayMode(tempGPR, arrayMode, Int32Shape);

    case Array::Double:
        return jumpSlowForUnwantedArrayMode(tempGPR, arrayMode, DoubleShape);

    case Array::Contiguous:
        return jumpSlowForUnwantedArrayMode(tempGPR, arrayMode, ContiguousShape);

    case Array::ArrayStorage:
    case Array::SlowPutArrayStorage: {
        ASSERT(!arrayMode.isJSArrayWithOriginalStructure());
        
        if (arrayMode.isJSArray()) {
            if (arrayMode.isSlowPut()) {
                result.append(
                    m_jit.branchTest32(
                        MacroAssembler::Zero, tempGPR, MacroAssembler::TrustedImm32(IsArray)));
                m_jit.and32(TrustedImm32(IndexingShapeMask), tempGPR);
                m_jit.sub32(TrustedImm32(ArrayStorageShape), tempGPR);
                result.append(
                    m_jit.branch32(
                        MacroAssembler::Above, tempGPR,
                        TrustedImm32(SlowPutArrayStorageShape - ArrayStorageShape)));
                break;
            }
            m_jit.and32(TrustedImm32(IsArray | IndexingShapeMask), tempGPR);
            result.append(
                m_jit.branch32(MacroAssembler::NotEqual, tempGPR, TrustedImm32(IsArray | ArrayStorageShape)));
            break;
        }
        m_jit.and32(TrustedImm32(IndexingShapeMask), tempGPR);
        if (arrayMode.isSlowPut()) {
            m_jit.sub32(TrustedImm32(ArrayStorageShape), tempGPR);
            result.append(
                m_jit.branch32(
                    MacroAssembler::Above, tempGPR,
                    TrustedImm32(SlowPutArrayStorageShape - ArrayStorageShape)));
            break;
        }
        result.append(
            m_jit.branch32(MacroAssembler::NotEqual, tempGPR, TrustedImm32(ArrayStorageShape)));
        break;
    }
    default:
        CRASH();
        break;
    }
    
    return result;
}

void SpeculativeJIT::checkArray(Node* node)
{
    ASSERT(node->arrayMode().isSpecific());
    ASSERT(!node->arrayMode().doesConversion());
    
    SpeculateCellOperand base(this, node->child1());
    GPRReg baseReg = base.gpr();
    
    const TypedArrayDescriptor* result = typedArrayDescriptor(node->arrayMode());
    
    if (node->arrayMode().alreadyChecked(m_jit.graph(), node, m_state.forNode(node->child1()))) {
        noResult(m_currentNode);
        return;
    }
    
    const ClassInfo* expectedClassInfo = 0;
    
    switch (node->arrayMode().type()) {
    case Array::String:
        expectedClassInfo = &JSString::s_info;
        break;
    case Array::Int32:
    case Array::Double:
    case Array::Contiguous:
    case Array::ArrayStorage:
    case Array::SlowPutArrayStorage: {
        GPRTemporary temp(this);
        GPRReg tempGPR = temp.gpr();
        m_jit.loadPtr(
            MacroAssembler::Address(baseReg, JSCell::structureOffset()), tempGPR);
        m_jit.load8(MacroAssembler::Address(tempGPR, Structure::indexingTypeOffset()), tempGPR);
        speculationCheck(
            BadIndexingType, JSValueSource::unboxedCell(baseReg), 0,
            jumpSlowForUnwantedArrayMode(tempGPR, node->arrayMode()));
        
        noResult(m_currentNode);
        return;
    }
    case Array::Arguments:
        expectedClassInfo = &Arguments::s_info;
        break;
    case Array::Int8Array:
    case Array::Int16Array:
    case Array::Int32Array:
    case Array::Uint8Array:
    case Array::Uint8ClampedArray:
    case Array::Uint16Array:
    case Array::Uint32Array:
    case Array::Float32Array:
    case Array::Float64Array:
        expectedClassInfo = result->m_classInfo;
        break;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
    
    GPRTemporary temp(this);
    m_jit.loadPtr(
        MacroAssembler::Address(baseReg, JSCell::structureOffset()), temp.gpr());
    speculationCheck(
        Uncountable, JSValueRegs(), 0,
        m_jit.branchPtr(
            MacroAssembler::NotEqual,
            MacroAssembler::Address(temp.gpr(), Structure::classInfoOffset()),
            MacroAssembler::TrustedImmPtr(expectedClassInfo)));
    
    noResult(m_currentNode);
}

void SpeculativeJIT::arrayify(Node* node, GPRReg baseReg, GPRReg propertyReg)
{
    ASSERT(node->arrayMode().doesConversion());
    
    GPRTemporary temp(this);
    GPRTemporary structure;
    GPRReg tempGPR = temp.gpr();
    GPRReg structureGPR = InvalidGPRReg;
    
    if (node->op() != ArrayifyToStructure) {
        GPRTemporary realStructure(this);
        structure.adopt(realStructure);
        structureGPR = structure.gpr();
    }
        
    // We can skip all that comes next if we already have array storage.
    MacroAssembler::JumpList slowPath;
    
    if (node->op() == ArrayifyToStructure) {
        slowPath.append(m_jit.branchWeakPtr(
            JITCompiler::NotEqual,
            JITCompiler::Address(baseReg, JSCell::structureOffset()),
            node->structure()));
    } else {
        m_jit.loadPtr(
            MacroAssembler::Address(baseReg, JSCell::structureOffset()), structureGPR);
        
        m_jit.load8(
            MacroAssembler::Address(structureGPR, Structure::indexingTypeOffset()), tempGPR);
        
        slowPath.append(jumpSlowForUnwantedArrayMode(tempGPR, node->arrayMode()));
    }
    
    addSlowPathGenerator(adoptPtr(new ArrayifySlowPathGenerator(
        slowPath, this, node, baseReg, propertyReg, tempGPR, structureGPR)));
    
    noResult(m_currentNode);
}

void SpeculativeJIT::arrayify(Node* node)
{
    ASSERT(node->arrayMode().isSpecific());
    
    SpeculateCellOperand base(this, node->child1());
    
    if (!node->child2()) {
        arrayify(node, base.gpr(), InvalidGPRReg);
        return;
    }
    
    SpeculateIntegerOperand property(this, node->child2());
    
    arrayify(node, base.gpr(), property.gpr());
}

GPRReg SpeculativeJIT::fillStorage(Edge edge)
{
    VirtualRegister virtualRegister = edge->virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];
    
    switch (info.registerFormat()) {
    case DataFormatNone: {
        if (info.spillFormat() == DataFormatStorage) {
            GPRReg gpr = allocate();
            m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
            m_jit.loadPtr(JITCompiler::addressFor(virtualRegister), gpr);
            info.fillStorage(*m_stream, gpr);
            return gpr;
        }
        
        // Must be a cell; fill it as a cell and then return the pointer.
        return fillSpeculateCell(edge);
    }
        
    case DataFormatStorage: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        return gpr;
    }
        
    default:
        return fillSpeculateCell(edge);
    }
}

void SpeculativeJIT::useChildren(Node* node)
{
    if (node->flags() & NodeHasVarArgs) {
        for (unsigned childIdx = node->firstChild(); childIdx < node->firstChild() + node->numChildren(); childIdx++) {
            if (!!m_jit.graph().m_varArgChildren[childIdx])
                use(m_jit.graph().m_varArgChildren[childIdx]);
        }
    } else {
        Edge child1 = node->child1();
        if (!child1) {
            ASSERT(!node->child2() && !node->child3());
            return;
        }
        use(child1);
        
        Edge child2 = node->child2();
        if (!child2) {
            ASSERT(!node->child3());
            return;
        }
        use(child2);
        
        Edge child3 = node->child3();
        if (!child3)
            return;
        use(child3);
    }
}

void SpeculativeJIT::writeBarrier(MacroAssembler& jit, GPRReg owner, GPRReg scratch1, GPRReg scratch2, WriteBarrierUseKind useKind)
{
    UNUSED_PARAM(jit);
    UNUSED_PARAM(owner);
    UNUSED_PARAM(scratch1);
    UNUSED_PARAM(scratch2);
    UNUSED_PARAM(useKind);
    ASSERT(owner != scratch1);
    ASSERT(owner != scratch2);
    ASSERT(scratch1 != scratch2);

#if ENABLE(WRITE_BARRIER_PROFILING)
    JITCompiler::emitCount(jit, WriteBarrierCounters::jitCounterFor(useKind));
#endif
}

void SpeculativeJIT::writeBarrier(GPRReg ownerGPR, GPRReg valueGPR, Edge valueUse, WriteBarrierUseKind useKind, GPRReg scratch1, GPRReg scratch2)
{
    UNUSED_PARAM(ownerGPR);
    UNUSED_PARAM(valueGPR);
    UNUSED_PARAM(scratch1);
    UNUSED_PARAM(scratch2);
    UNUSED_PARAM(useKind);

    if (isKnownNotCell(valueUse.node()))
        return;

#if ENABLE(WRITE_BARRIER_PROFILING)
    JITCompiler::emitCount(m_jit, WriteBarrierCounters::jitCounterFor(useKind));
#endif
}

void SpeculativeJIT::writeBarrier(GPRReg ownerGPR, JSCell* value, WriteBarrierUseKind useKind, GPRReg scratch1, GPRReg scratch2)
{
    UNUSED_PARAM(ownerGPR);
    UNUSED_PARAM(value);
    UNUSED_PARAM(scratch1);
    UNUSED_PARAM(scratch2);
    UNUSED_PARAM(useKind);
    
    if (Heap::isMarked(value))
        return;

#if ENABLE(WRITE_BARRIER_PROFILING)
    JITCompiler::emitCount(m_jit, WriteBarrierCounters::jitCounterFor(useKind));
#endif
}

void SpeculativeJIT::writeBarrier(JSCell* owner, GPRReg valueGPR, Edge valueUse, WriteBarrierUseKind useKind, GPRReg scratch)
{
    UNUSED_PARAM(owner);
    UNUSED_PARAM(valueGPR);
    UNUSED_PARAM(scratch);
    UNUSED_PARAM(useKind);

    if (isKnownNotCell(valueUse.node()))
        return;

#if ENABLE(WRITE_BARRIER_PROFILING)
    JITCompiler::emitCount(m_jit, WriteBarrierCounters::jitCounterFor(useKind));
#endif
}

bool SpeculativeJIT::nonSpeculativeCompare(Node* node, MacroAssembler::RelationalCondition cond, S_DFGOperation_EJJ helperFunction)
{
    unsigned branchIndexInBlock = detectPeepHoleBranch();
    if (branchIndexInBlock != UINT_MAX) {
        Node* branchNode = m_jit.graph().m_blocks[m_block]->at(branchIndexInBlock);

        ASSERT(node->adjustedRefCount() == 1);
        
        nonSpeculativePeepholeBranch(node, branchNode, cond, helperFunction);
    
        m_indexInBlock = branchIndexInBlock;
        m_currentNode = branchNode;
        
        return true;
    }
    
    nonSpeculativeNonPeepholeCompare(node, cond, helperFunction);
    
    return false;
}

bool SpeculativeJIT::nonSpeculativeStrictEq(Node* node, bool invert)
{
    unsigned branchIndexInBlock = detectPeepHoleBranch();
    if (branchIndexInBlock != UINT_MAX) {
        Node* branchNode = m_jit.graph().m_blocks[m_block]->at(branchIndexInBlock);

        ASSERT(node->adjustedRefCount() == 1);
        
        nonSpeculativePeepholeStrictEq(node, branchNode, invert);
    
        m_indexInBlock = branchIndexInBlock;
        m_currentNode = branchNode;
        
        return true;
    }
    
    nonSpeculativeNonPeepholeStrictEq(node, invert);
    
    return false;
}

#ifndef NDEBUG
static const char* dataFormatString(DataFormat format)
{
    // These values correspond to the DataFormat enum.
    const char* strings[] = {
        "[  ]",
        "[ i]",
        "[ d]",
        "[ c]",
        "Err!",
        "Err!",
        "Err!",
        "Err!",
        "[J ]",
        "[Ji]",
        "[Jd]",
        "[Jc]",
        "Err!",
        "Err!",
        "Err!",
        "Err!",
    };
    return strings[format];
}

void SpeculativeJIT::dump(const char* label)
{
    if (label)
        dataLogF("<%s>\n", label);

    dataLogF("  gprs:\n");
    m_gprs.dump();
    dataLogF("  fprs:\n");
    m_fprs.dump();
    dataLogF("  VirtualRegisters:\n");
    for (unsigned i = 0; i < m_generationInfo.size(); ++i) {
        GenerationInfo& info = m_generationInfo[i];
        if (info.alive())
            dataLogF("    % 3d:%s%s", i, dataFormatString(info.registerFormat()), dataFormatString(info.spillFormat()));
        else
            dataLogF("    % 3d:[__][__]", i);
        if (info.registerFormat() == DataFormatDouble)
            dataLogF(":fpr%d\n", info.fpr());
        else if (info.registerFormat() != DataFormatNone
#if USE(JSVALUE32_64)
            && !(info.registerFormat() & DataFormatJS)
#endif
            ) {
            ASSERT(info.gpr() != InvalidGPRReg);
            dataLogF(":%s\n", GPRInfo::debugName(info.gpr()));
        } else
            dataLogF("\n");
    }
    if (label)
        dataLogF("</%s>\n", label);
}
#endif


#if DFG_ENABLE(CONSISTENCY_CHECK)
void SpeculativeJIT::checkConsistency()
{
    bool failed = false;

    for (gpr_iterator iter = m_gprs.begin(); iter != m_gprs.end(); ++iter) {
        if (iter.isLocked()) {
            dataLogF("DFG_CONSISTENCY_CHECK failed: gpr %s is locked.\n", iter.debugName());
            failed = true;
        }
    }
    for (fpr_iterator iter = m_fprs.begin(); iter != m_fprs.end(); ++iter) {
        if (iter.isLocked()) {
            dataLogF("DFG_CONSISTENCY_CHECK failed: fpr %s is locked.\n", iter.debugName());
            failed = true;
        }
    }

    for (unsigned i = 0; i < m_generationInfo.size(); ++i) {
        VirtualRegister virtualRegister = (VirtualRegister)i;
        GenerationInfo& info = m_generationInfo[virtualRegister];
        if (!info.alive())
            continue;
        switch (info.registerFormat()) {
        case DataFormatNone:
            break;
        case DataFormatJS:
        case DataFormatJSInteger:
        case DataFormatJSDouble:
        case DataFormatJSCell:
        case DataFormatJSBoolean:
#if USE(JSVALUE32_64)
            break;
#endif
        case DataFormatInteger:
        case DataFormatCell:
        case DataFormatBoolean:
        case DataFormatStorage: {
            GPRReg gpr = info.gpr();
            ASSERT(gpr != InvalidGPRReg);
            if (m_gprs.name(gpr) != virtualRegister) {
                dataLogF("DFG_CONSISTENCY_CHECK failed: name mismatch for virtual register %d (gpr %s).\n", virtualRegister, GPRInfo::debugName(gpr));
                failed = true;
            }
            break;
        }
        case DataFormatDouble: {
            FPRReg fpr = info.fpr();
            ASSERT(fpr != InvalidFPRReg);
            if (m_fprs.name(fpr) != virtualRegister) {
                dataLogF("DFG_CONSISTENCY_CHECK failed: name mismatch for virtual register %d (fpr %s).\n", virtualRegister, FPRInfo::debugName(fpr));
                failed = true;
            }
            break;
        }
        case DataFormatOSRMarker:
        case DataFormatDead:
        case DataFormatArguments:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
    }

    for (gpr_iterator iter = m_gprs.begin(); iter != m_gprs.end(); ++iter) {
        VirtualRegister virtualRegister = iter.name();
        if (virtualRegister == InvalidVirtualRegister)
            continue;

        GenerationInfo& info = m_generationInfo[virtualRegister];
#if USE(JSVALUE64)
        if (iter.regID() != info.gpr()) {
            dataLogF("DFG_CONSISTENCY_CHECK failed: name mismatch for gpr %s (virtual register %d).\n", iter.debugName(), virtualRegister);
            failed = true;
        }
#else
        if (!(info.registerFormat() & DataFormatJS)) {
            if (iter.regID() != info.gpr()) {
                dataLogF("DFG_CONSISTENCY_CHECK failed: name mismatch for gpr %s (virtual register %d).\n", iter.debugName(), virtualRegister);
                failed = true;
            }
        } else {
            if (iter.regID() != info.tagGPR() && iter.regID() != info.payloadGPR()) {
                dataLogF("DFG_CONSISTENCY_CHECK failed: name mismatch for gpr %s (virtual register %d).\n", iter.debugName(), virtualRegister);
                failed = true;
            }
        }
#endif
    }

    for (fpr_iterator iter = m_fprs.begin(); iter != m_fprs.end(); ++iter) {
        VirtualRegister virtualRegister = iter.name();
        if (virtualRegister == InvalidVirtualRegister)
            continue;

        GenerationInfo& info = m_generationInfo[virtualRegister];
        if (iter.regID() != info.fpr()) {
            dataLogF("DFG_CONSISTENCY_CHECK failed: name mismatch for fpr %s (virtual register %d).\n", iter.debugName(), virtualRegister);
            failed = true;
        }
    }

    if (failed) {
        dump();
        CRASH();
    }
}
#endif

GPRTemporary::GPRTemporary()
    : m_jit(0)
    , m_gpr(InvalidGPRReg)
{
}

GPRTemporary::GPRTemporary(SpeculativeJIT* jit)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    m_gpr = m_jit->allocate();
}

GPRTemporary::GPRTemporary(SpeculativeJIT* jit, GPRReg specific)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    m_gpr = m_jit->allocate(specific);
}

GPRTemporary::GPRTemporary(SpeculativeJIT* jit, SpeculateIntegerOperand& op1)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.node()))
        m_gpr = m_jit->reuse(op1.gpr());
    else
        m_gpr = m_jit->allocate();
}

GPRTemporary::GPRTemporary(SpeculativeJIT* jit, SpeculateIntegerOperand& op1, SpeculateIntegerOperand& op2)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.node()))
        m_gpr = m_jit->reuse(op1.gpr());
    else if (m_jit->canReuse(op2.node()))
        m_gpr = m_jit->reuse(op2.gpr());
    else
        m_gpr = m_jit->allocate();
}

GPRTemporary::GPRTemporary(SpeculativeJIT* jit, SpeculateStrictInt32Operand& op1)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.node()))
        m_gpr = m_jit->reuse(op1.gpr());
    else
        m_gpr = m_jit->allocate();
}

GPRTemporary::GPRTemporary(SpeculativeJIT* jit, IntegerOperand& op1)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.node()))
        m_gpr = m_jit->reuse(op1.gpr());
    else
        m_gpr = m_jit->allocate();
}

GPRTemporary::GPRTemporary(SpeculativeJIT* jit, IntegerOperand& op1, IntegerOperand& op2)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.node()))
        m_gpr = m_jit->reuse(op1.gpr());
    else if (m_jit->canReuse(op2.node()))
        m_gpr = m_jit->reuse(op2.gpr());
    else
        m_gpr = m_jit->allocate();
}

GPRTemporary::GPRTemporary(SpeculativeJIT* jit, SpeculateCellOperand& op1)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.node()))
        m_gpr = m_jit->reuse(op1.gpr());
    else
        m_gpr = m_jit->allocate();
}

GPRTemporary::GPRTemporary(SpeculativeJIT* jit, SpeculateBooleanOperand& op1)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.node()))
        m_gpr = m_jit->reuse(op1.gpr());
    else
        m_gpr = m_jit->allocate();
}

#if USE(JSVALUE64)
GPRTemporary::GPRTemporary(SpeculativeJIT* jit, JSValueOperand& op1)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.node()))
        m_gpr = m_jit->reuse(op1.gpr());
    else
        m_gpr = m_jit->allocate();
}
#else
GPRTemporary::GPRTemporary(SpeculativeJIT* jit, JSValueOperand& op1, bool tag)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (!op1.isDouble() && m_jit->canReuse(op1.node()))
        m_gpr = m_jit->reuse(tag ? op1.tagGPR() : op1.payloadGPR());
    else
        m_gpr = m_jit->allocate();
}
#endif

GPRTemporary::GPRTemporary(SpeculativeJIT* jit, StorageOperand& op1)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.node()))
        m_gpr = m_jit->reuse(op1.gpr());
    else
        m_gpr = m_jit->allocate();
}

void GPRTemporary::adopt(GPRTemporary& other)
{
    ASSERT(!m_jit);
    ASSERT(m_gpr == InvalidGPRReg);
    ASSERT(other.m_jit);
    ASSERT(other.m_gpr != InvalidGPRReg);
    m_jit = other.m_jit;
    m_gpr = other.m_gpr;
    other.m_jit = 0;
    other.m_gpr = InvalidGPRReg;
}

FPRTemporary::FPRTemporary(SpeculativeJIT* jit)
    : m_jit(jit)
    , m_fpr(InvalidFPRReg)
{
    m_fpr = m_jit->fprAllocate();
}

FPRTemporary::FPRTemporary(SpeculativeJIT* jit, SpeculateDoubleOperand& op1)
    : m_jit(jit)
    , m_fpr(InvalidFPRReg)
{
    if (m_jit->canReuse(op1.node()))
        m_fpr = m_jit->reuse(op1.fpr());
    else
        m_fpr = m_jit->fprAllocate();
}

FPRTemporary::FPRTemporary(SpeculativeJIT* jit, SpeculateDoubleOperand& op1, SpeculateDoubleOperand& op2)
    : m_jit(jit)
    , m_fpr(InvalidFPRReg)
{
    if (m_jit->canReuse(op1.node()))
        m_fpr = m_jit->reuse(op1.fpr());
    else if (m_jit->canReuse(op2.node()))
        m_fpr = m_jit->reuse(op2.fpr());
    else
        m_fpr = m_jit->fprAllocate();
}

#if USE(JSVALUE32_64)
FPRTemporary::FPRTemporary(SpeculativeJIT* jit, JSValueOperand& op1)
    : m_jit(jit)
    , m_fpr(InvalidFPRReg)
{
    if (op1.isDouble() && m_jit->canReuse(op1.node()))
        m_fpr = m_jit->reuse(op1.fpr());
    else
        m_fpr = m_jit->fprAllocate();
}
#endif

void SpeculativeJIT::compilePeepHoleDoubleBranch(Node* node, Node* branchNode, JITCompiler::DoubleCondition condition)
{
    BlockIndex taken = branchNode->takenBlockIndex();
    BlockIndex notTaken = branchNode->notTakenBlockIndex();
    
    SpeculateDoubleOperand op1(this, node->child1());
    SpeculateDoubleOperand op2(this, node->child2());
    
    branchDouble(condition, op1.fpr(), op2.fpr(), taken);
    jump(notTaken);
}

void SpeculativeJIT::compilePeepHoleObjectEquality(Node* node, Node* branchNode)
{
    BlockIndex taken = branchNode->takenBlockIndex();
    BlockIndex notTaken = branchNode->notTakenBlockIndex();

    MacroAssembler::RelationalCondition condition = MacroAssembler::Equal;
    
    if (taken == nextBlock()) {
        condition = MacroAssembler::NotEqual;
        BlockIndex tmp = taken;
        taken = notTaken;
        notTaken = tmp;
    }

    SpeculateCellOperand op1(this, node->child1());
    SpeculateCellOperand op2(this, node->child2());
    
    GPRReg op1GPR = op1.gpr();
    GPRReg op2GPR = op2.gpr();
    
    if (m_jit.graph().globalObjectFor(node->codeOrigin)->masqueradesAsUndefinedWatchpoint()->isStillValid()) {
        m_jit.graph().globalObjectFor(node->codeOrigin)->masqueradesAsUndefinedWatchpoint()->add(speculationWatchpoint());
        if (m_state.forNode(node->child1()).m_type & ~SpecObject) {
            speculationCheck(
                BadType, JSValueSource::unboxedCell(op1GPR), node->child1(), 
                m_jit.branchPtr(
                    MacroAssembler::Equal, 
                    MacroAssembler::Address(op1GPR, JSCell::structureOffset()), 
                    MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
        }
        if (m_state.forNode(node->child2()).m_type & ~SpecObject) {
            speculationCheck(
                BadType, JSValueSource::unboxedCell(op2GPR), node->child2(),
                m_jit.branchPtr(
                    MacroAssembler::Equal, 
                    MacroAssembler::Address(op2GPR, JSCell::structureOffset()), 
                    MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
        }
    } else {
        GPRTemporary structure(this);
        GPRReg structureGPR = structure.gpr();

        m_jit.loadPtr(MacroAssembler::Address(op1GPR, JSCell::structureOffset()), structureGPR);
        if (m_state.forNode(node->child1()).m_type & ~SpecObject) {
            speculationCheck(
                BadType, JSValueSource::unboxedCell(op1GPR), node->child1(),
                m_jit.branchPtr(
                    MacroAssembler::Equal, 
                    structureGPR, 
                    MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
        }
        speculationCheck(BadType, JSValueSource::unboxedCell(op1GPR), node->child1(),
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(structureGPR, Structure::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));

        m_jit.loadPtr(MacroAssembler::Address(op2GPR, JSCell::structureOffset()), structureGPR);
        if (m_state.forNode(node->child2()).m_type & ~SpecObject) {
            speculationCheck(
                BadType, JSValueSource::unboxedCell(op2GPR), node->child2(),
                m_jit.branchPtr(
                    MacroAssembler::Equal, 
                    structureGPR, 
                    MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
        }
        speculationCheck(BadType, JSValueSource::unboxedCell(op2GPR), node->child2(),
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(structureGPR, Structure::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));
    }

    branchPtr(condition, op1GPR, op2GPR, taken);
    jump(notTaken);
}

void SpeculativeJIT::compilePeepHoleBooleanBranch(Node* node, Node* branchNode, JITCompiler::RelationalCondition condition)
{
    BlockIndex taken = branchNode->takenBlockIndex();
    BlockIndex notTaken = branchNode->notTakenBlockIndex();

    // The branch instruction will branch to the taken block.
    // If taken is next, switch taken with notTaken & invert the branch condition so we can fall through.
    if (taken == nextBlock()) {
        condition = JITCompiler::invert(condition);
        BlockIndex tmp = taken;
        taken = notTaken;
        notTaken = tmp;
    }

    if (isBooleanConstant(node->child1().node())) {
        bool imm = valueOfBooleanConstant(node->child1().node());
        SpeculateBooleanOperand op2(this, node->child2());
        branch32(condition, JITCompiler::Imm32(static_cast<int32_t>(JSValue::encode(jsBoolean(imm)))), op2.gpr(), taken);
    } else if (isBooleanConstant(node->child2().node())) {
        SpeculateBooleanOperand op1(this, node->child1());
        bool imm = valueOfBooleanConstant(node->child2().node());
        branch32(condition, op1.gpr(), JITCompiler::Imm32(static_cast<int32_t>(JSValue::encode(jsBoolean(imm)))), taken);
    } else {
        SpeculateBooleanOperand op1(this, node->child1());
        SpeculateBooleanOperand op2(this, node->child2());
        branch32(condition, op1.gpr(), op2.gpr(), taken);
    }

    jump(notTaken);
}

void SpeculativeJIT::compilePeepHoleIntegerBranch(Node* node, Node* branchNode, JITCompiler::RelationalCondition condition)
{
    BlockIndex taken = branchNode->takenBlockIndex();
    BlockIndex notTaken = branchNode->notTakenBlockIndex();

    // The branch instruction will branch to the taken block.
    // If taken is next, switch taken with notTaken & invert the branch condition so we can fall through.
    if (taken == nextBlock()) {
        condition = JITCompiler::invert(condition);
        BlockIndex tmp = taken;
        taken = notTaken;
        notTaken = tmp;
    }

    if (isInt32Constant(node->child1().node())) {
        int32_t imm = valueOfInt32Constant(node->child1().node());
        SpeculateIntegerOperand op2(this, node->child2());
        branch32(condition, JITCompiler::Imm32(imm), op2.gpr(), taken);
    } else if (isInt32Constant(node->child2().node())) {
        SpeculateIntegerOperand op1(this, node->child1());
        int32_t imm = valueOfInt32Constant(node->child2().node());
        branch32(condition, op1.gpr(), JITCompiler::Imm32(imm), taken);
    } else {
        SpeculateIntegerOperand op1(this, node->child1());
        SpeculateIntegerOperand op2(this, node->child2());
        branch32(condition, op1.gpr(), op2.gpr(), taken);
    }

    jump(notTaken);
}

// Returns true if the compare is fused with a subsequent branch.
bool SpeculativeJIT::compilePeepHoleBranch(Node* node, MacroAssembler::RelationalCondition condition, MacroAssembler::DoubleCondition doubleCondition, S_DFGOperation_EJJ operation)
{
    // Fused compare & branch.
    unsigned branchIndexInBlock = detectPeepHoleBranch();
    if (branchIndexInBlock != UINT_MAX) {
        Node* branchNode = m_jit.graph().m_blocks[m_block]->at(branchIndexInBlock);

        // detectPeepHoleBranch currently only permits the branch to be the very next node,
        // so can be no intervening nodes to also reference the compare. 
        ASSERT(node->adjustedRefCount() == 1);

        if (node->isBinaryUseKind(Int32Use))
            compilePeepHoleIntegerBranch(node, branchNode, condition);
        else if (node->isBinaryUseKind(NumberUse))
            compilePeepHoleDoubleBranch(node, branchNode, doubleCondition);
        else if (node->op() == CompareEq) {
            if (node->isBinaryUseKind(StringUse)) {
                // Use non-peephole comparison, for now.
                return false;
            }
            if (node->isBinaryUseKind(BooleanUse))
                compilePeepHoleBooleanBranch(node, branchNode, condition);
            else if (node->isBinaryUseKind(ObjectUse))
                compilePeepHoleObjectEquality(node, branchNode);
            else if (node->child1().useKind() == ObjectUse && node->child2().useKind() == ObjectOrOtherUse)
                compilePeepHoleObjectToObjectOrOtherEquality(node->child1(), node->child2(), branchNode);
            else if (node->child1().useKind() == ObjectOrOtherUse && node->child2().useKind() == ObjectUse)
                compilePeepHoleObjectToObjectOrOtherEquality(node->child2(), node->child1(), branchNode);
            else {
                nonSpeculativePeepholeBranch(node, branchNode, condition, operation);
                return true;
            }
        } else {
            nonSpeculativePeepholeBranch(node, branchNode, condition, operation);
            return true;
        }

        use(node->child1());
        use(node->child2());
        m_indexInBlock = branchIndexInBlock;
        m_currentNode = branchNode;
        return true;
    }
    return false;
}

void SpeculativeJIT::noticeOSRBirth(Node* node)
{
    if (!node->hasVirtualRegister())
        return;
    
    VirtualRegister virtualRegister = node->virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];
    
    info.noticeOSRBirth(*m_stream, node, virtualRegister);
}

void SpeculativeJIT::compileMovHint(Node* node)
{
    ASSERT(node->containsMovHint() && node->op() != ZombieHint);
    
    m_lastSetOperand = node->local();

    Node* child = node->child1().node();
    noticeOSRBirth(child);
    
    if (child->op() == UInt32ToNumber)
        noticeOSRBirth(child->child1().node());
    
    m_stream->appendAndLog(VariableEvent::movHint(MinifiedID(child), node->local()));
}

void SpeculativeJIT::compileMovHintAndCheck(Node* node)
{
    compileMovHint(node);
    speculate(node, node->child1());
    noResult(node);
}

void SpeculativeJIT::compileInlineStart(Node* node)
{
    InlineCallFrame* inlineCallFrame = node->codeOrigin.inlineCallFrame;
    int argumentCountIncludingThis = inlineCallFrame->arguments.size();
    unsigned argumentPositionStart = node->argumentPositionStart();
    CodeBlock* codeBlock = baselineCodeBlockForInlineCallFrame(inlineCallFrame);
    for (int i = 0; i < argumentCountIncludingThis; ++i) {
        ValueRecovery recovery;
        if (codeBlock->isCaptured(argumentToOperand(i)))
            recovery = ValueRecovery::alreadyInJSStack();
        else {
            ArgumentPosition& argumentPosition =
                m_jit.graph().m_argumentPositions[argumentPositionStart + i];
            ValueSource valueSource;
            if (!argumentPosition.shouldUnboxIfPossible())
                valueSource = ValueSource(ValueInJSStack);
            else if (argumentPosition.shouldUseDoubleFormat())
                valueSource = ValueSource(DoubleInJSStack);
            else if (isInt32Speculation(argumentPosition.prediction()))
                valueSource = ValueSource(Int32InJSStack);
            else if (isCellSpeculation(argumentPosition.prediction()))
                valueSource = ValueSource(CellInJSStack);
            else if (isBooleanSpeculation(argumentPosition.prediction()))
                valueSource = ValueSource(BooleanInJSStack);
            else
                valueSource = ValueSource(ValueInJSStack);
            recovery = computeValueRecoveryFor(valueSource);
        }
        // The recovery should refer either to something that has already been
        // stored into the stack at the right place, or to a constant,
        // since the Arguments code isn't smart enough to handle anything else.
        // The exception is the this argument, which we don't really need to be
        // able to recover.
#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLogF("\nRecovery for argument %d: ", i);
        recovery.dump(WTF::dataFile());
#endif
        inlineCallFrame->arguments[i] = recovery;
    }
}

void SpeculativeJIT::compile(BasicBlock& block)
{
    ASSERT(m_compileOkay);
    
    if (!block.isReachable)
        return;
    
    if (!block.cfaHasVisited) {
        // Don't generate code for basic blocks that are unreachable according to CFA.
        // But to be sure that nobody has generated a jump to this block, drop in a
        // breakpoint here.
#if !ASSERT_DISABLED
        m_jit.breakpoint();
#endif
        return;
    }

    m_blockHeads[m_block] = m_jit.label();
#if DFG_ENABLE(JIT_BREAK_ON_EVERY_BLOCK)
    m_jit.breakpoint();
#endif
    
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLogF("Setting up state for block #%u: ", m_block);
#endif
    
    m_stream->appendAndLog(VariableEvent::reset());
    
    m_jit.jitAssertHasValidCallFrame();

    ASSERT(m_arguments.size() == block.variablesAtHead.numberOfArguments());
    for (size_t i = 0; i < m_arguments.size(); ++i) {
        ValueSource valueSource = ValueSource(ValueInJSStack);
        m_arguments[i] = valueSource;
        m_stream->appendAndLog(VariableEvent::setLocal(argumentToOperand(i), valueSource.dataFormat()));
    }
    
    m_state.reset();
    m_state.beginBasicBlock(&block);
    
    ASSERT(m_variables.size() == block.variablesAtHead.numberOfLocals());
    for (size_t i = 0; i < m_variables.size(); ++i) {
        Node* node = block.variablesAtHead.local(i);
        ValueSource valueSource;
        if (!node)
            valueSource = ValueSource(SourceIsDead);
        else if (node->variableAccessData()->isArgumentsAlias())
            valueSource = ValueSource(ArgumentsSource);
        else if (!node->refCount())
            valueSource = ValueSource(SourceIsDead);
        else if (!node->variableAccessData()->shouldUnboxIfPossible())
            valueSource = ValueSource(ValueInJSStack);
        else if (node->variableAccessData()->shouldUseDoubleFormat())
            valueSource = ValueSource(DoubleInJSStack);
        else
            valueSource = ValueSource::forSpeculation(node->variableAccessData()->argumentAwarePrediction());
        m_variables[i] = valueSource;
        // FIXME: Don't emit SetLocal(Dead). https://bugs.webkit.org/show_bug.cgi?id=108019
        m_stream->appendAndLog(VariableEvent::setLocal(i, valueSource.dataFormat()));
    }
    
    m_lastSetOperand = std::numeric_limits<int>::max();
    m_codeOriginForOSR = CodeOrigin();
    
    if (DFG_ENABLE_EDGE_CODE_VERIFICATION) {
        JITCompiler::Jump verificationSucceeded =
            m_jit.branch32(JITCompiler::Equal, GPRInfo::regT0, TrustedImm32(m_block));
        m_jit.breakpoint();
        verificationSucceeded.link(&m_jit);
    }

#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLogF("\n");
#endif

    for (m_indexInBlock = 0; m_indexInBlock < block.size(); ++m_indexInBlock) {
        m_currentNode = block[m_indexInBlock];
#if !ASSERT_DISABLED
        m_canExit = m_currentNode->canExit();
#endif
        bool shouldExecuteEffects = m_state.startExecuting(m_currentNode);
        m_jit.setForNode(m_currentNode);
        m_codeOriginForOSR = m_currentNode->codeOrigin;
        if (!m_currentNode->shouldGenerate()) {
#if DFG_ENABLE(DEBUG_VERBOSE)
            dataLogF("SpeculativeJIT skipping Node @%d (bc#%u) at JIT offset 0x%x     ", m_currentNode->index(), m_currentNode->codeOrigin.bytecodeIndex, m_jit.debugOffset());
#endif
            switch (m_currentNode->op()) {
            case JSConstant:
                m_minifiedGraph->append(MinifiedNode::fromNode(m_currentNode));
                break;
                
            case WeakJSConstant:
                m_jit.addWeakReference(m_currentNode->weakConstant());
                m_minifiedGraph->append(MinifiedNode::fromNode(m_currentNode));
                break;
                
            case SetLocal:
                RELEASE_ASSERT_NOT_REACHED();
                break;
                
            case MovHint:
                compileMovHint(m_currentNode);
                break;
                
            case ZombieHint: {
                m_lastSetOperand = m_currentNode->local();
                m_stream->appendAndLog(VariableEvent::setLocal(m_currentNode->local(), DataFormatDead));
                break;
            }

            default:
                if (belongsInMinifiedGraph(m_currentNode->op()))
                    m_minifiedGraph->append(MinifiedNode::fromNode(m_currentNode));
                break;
            }
        } else {
            
            if (verboseCompilationEnabled()) {
                dataLogF(
                    "SpeculativeJIT generating Node @%d (bc#%u) at JIT offset 0x%x",
                    (int)m_currentNode->index(),
                    m_currentNode->codeOrigin.bytecodeIndex, m_jit.debugOffset());
#if DFG_ENABLE(DEBUG_VERBOSE)
                dataLog("   ");
#else
                dataLog("\n");
#endif
            }
#if DFG_ENABLE(JIT_BREAK_ON_EVERY_NODE)
            m_jit.breakpoint();
#endif
#if DFG_ENABLE(XOR_DEBUG_AID)
            m_jit.xorPtr(JITCompiler::TrustedImm32(m_currentNode->index()), GPRInfo::regT0);
            m_jit.xorPtr(JITCompiler::TrustedImm32(m_currentNode->index()), GPRInfo::regT0);
#endif
            checkConsistency();
            
            m_speculationDirection = (m_currentNode->flags() & NodeExitsForward) ? ForwardSpeculation : BackwardSpeculation;
            
            compile(m_currentNode);
            if (!m_compileOkay) {
                m_compileOkay = true;
                clearGenerationInfo();
                return;
            }
            
            if (belongsInMinifiedGraph(m_currentNode->op())) {
                m_minifiedGraph->append(MinifiedNode::fromNode(m_currentNode));
                noticeOSRBirth(m_currentNode);
            }
            
#if DFG_ENABLE(DEBUG_VERBOSE)
            if (m_currentNode->hasResult()) {
                GenerationInfo& info = m_generationInfo[m_currentNode->virtualRegister()];
                dataLogF("-> %s, vr#%d", dataFormatToString(info.registerFormat()), (int)m_currentNode->virtualRegister());
                if (info.registerFormat() != DataFormatNone) {
                    if (info.registerFormat() == DataFormatDouble)
                        dataLogF(", %s", FPRInfo::debugName(info.fpr()));
#if USE(JSVALUE32_64)
                    else if (info.registerFormat() & DataFormatJS)
                        dataLogF(", %s %s", GPRInfo::debugName(info.tagGPR()), GPRInfo::debugName(info.payloadGPR()));
#endif
                    else
                        dataLogF(", %s", GPRInfo::debugName(info.gpr()));
                }
                dataLogF("    ");
            } else
                dataLogF("    ");
#endif
        }
        
#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLogF("\n");
#endif
        
        // Make sure that the abstract state is rematerialized for the next node.
        if (shouldExecuteEffects)
            m_state.executeEffects(m_indexInBlock);
        
        if (m_currentNode->shouldGenerate())
            checkConsistency();
    }
    
    // Perform the most basic verification that children have been used correctly.
#if !ASSERT_DISABLED
    for (unsigned index = 0; index < m_generationInfo.size(); ++index) {
        GenerationInfo& info = m_generationInfo[index];
        ASSERT(!info.alive());
    }
#endif
}

// If we are making type predictions about our arguments then
// we need to check that they are correct on function entry.
void SpeculativeJIT::checkArgumentTypes()
{
    ASSERT(!m_currentNode);
    m_isCheckingArgumentTypes = true;
    m_speculationDirection = BackwardSpeculation;
    m_codeOriginForOSR = CodeOrigin(0);

    for (size_t i = 0; i < m_arguments.size(); ++i)
        m_arguments[i] = ValueSource(ValueInJSStack);
    for (size_t i = 0; i < m_variables.size(); ++i)
        m_variables[i] = ValueSource(ValueInJSStack);
    
    for (int i = 0; i < m_jit.codeBlock()->numParameters(); ++i) {
        Node* node = m_jit.graph().m_arguments[i];
        ASSERT(node->op() == SetArgument);
        if (!node->shouldGenerate()) {
            // The argument is dead. We don't do any checks for such arguments.
            continue;
        }
        
        VariableAccessData* variableAccessData = node->variableAccessData();
        if (!variableAccessData->isProfitableToUnbox())
            continue;
        
        VirtualRegister virtualRegister = variableAccessData->local();
        SpeculatedType predictedType = variableAccessData->prediction();

        JSValueSource valueSource = JSValueSource(JITCompiler::addressFor(virtualRegister));
        
#if USE(JSVALUE64)
        if (isInt32Speculation(predictedType))
            speculationCheck(BadType, valueSource, node, m_jit.branch64(MacroAssembler::Below, JITCompiler::addressFor(virtualRegister), GPRInfo::tagTypeNumberRegister));
        else if (isBooleanSpeculation(predictedType)) {
            GPRTemporary temp(this);
            m_jit.load64(JITCompiler::addressFor(virtualRegister), temp.gpr());
            m_jit.xor64(TrustedImm32(static_cast<int32_t>(ValueFalse)), temp.gpr());
            speculationCheck(BadType, valueSource, node, m_jit.branchTest64(MacroAssembler::NonZero, temp.gpr(), TrustedImm32(static_cast<int32_t>(~1))));
        } else if (isCellSpeculation(predictedType))
            speculationCheck(BadType, valueSource, node, m_jit.branchTest64(MacroAssembler::NonZero, JITCompiler::addressFor(virtualRegister), GPRInfo::tagMaskRegister));
#else
        if (isInt32Speculation(predictedType))
            speculationCheck(BadType, valueSource, node, m_jit.branch32(MacroAssembler::NotEqual, JITCompiler::tagFor(virtualRegister), TrustedImm32(JSValue::Int32Tag)));
        else if (isBooleanSpeculation(predictedType))
            speculationCheck(BadType, valueSource, node, m_jit.branch32(MacroAssembler::NotEqual, JITCompiler::tagFor(virtualRegister), TrustedImm32(JSValue::BooleanTag)));
        else if (isCellSpeculation(predictedType))
            speculationCheck(BadType, valueSource, node, m_jit.branch32(MacroAssembler::NotEqual, JITCompiler::tagFor(virtualRegister), TrustedImm32(JSValue::CellTag)));
#endif
    }
    m_isCheckingArgumentTypes = false;
}

bool SpeculativeJIT::compile()
{
    checkArgumentTypes();

    if (DFG_ENABLE_EDGE_CODE_VERIFICATION)
        m_jit.move(TrustedImm32(0), GPRInfo::regT0);

    ASSERT(!m_currentNode);
    for (m_block = 0; m_block < m_jit.graph().m_blocks.size(); ++m_block) {
        m_jit.setForBlock(m_block);
        BasicBlock* block = m_jit.graph().m_blocks[m_block].get();
        if (block)
            compile(*block);
    }
    linkBranches();
    return true;
}

void SpeculativeJIT::createOSREntries()
{
    for (BlockIndex blockIndex = 0; blockIndex < m_jit.graph().m_blocks.size(); ++blockIndex) {
        BasicBlock* block = m_jit.graph().m_blocks[blockIndex].get();
        if (!block)
            continue;
        if (!block->isOSRTarget)
            continue;

        // Currently we only need to create OSR entry trampolines when using edge code
        // verification. But in the future, we'll need this for other things as well (like
        // when we have global reg alloc).
        // If we don't need OSR entry trampolin
        if (!DFG_ENABLE_EDGE_CODE_VERIFICATION) {
            m_osrEntryHeads.append(m_blockHeads[blockIndex]);
            continue;
        }
        
        m_osrEntryHeads.append(m_jit.label());
        m_jit.move(TrustedImm32(blockIndex), GPRInfo::regT0);
        m_jit.jump().linkTo(m_blockHeads[blockIndex], &m_jit);
    }
}

void SpeculativeJIT::linkOSREntries(LinkBuffer& linkBuffer)
{
    unsigned osrEntryIndex = 0;
    for (BlockIndex blockIndex = 0; blockIndex < m_jit.graph().m_blocks.size(); ++blockIndex) {
        BasicBlock* block = m_jit.graph().m_blocks[blockIndex].get();
        if (!block)
            continue;
        if (!block->isOSRTarget)
            continue;
        m_jit.noticeOSREntry(*block, m_osrEntryHeads[osrEntryIndex++], linkBuffer);
    }
    ASSERT(osrEntryIndex == m_osrEntryHeads.size());
}

ValueRecovery SpeculativeJIT::computeValueRecoveryFor(const ValueSource& valueSource)
{
    if (valueSource.isInJSStack())
        return valueSource.valueRecovery();
        
    ASSERT(valueSource.kind() == HaveNode);
    Node* node = valueSource.id().node(m_jit.graph());
    if (isConstant(node))
        return ValueRecovery::constant(valueOfJSConstant(node));
    
    return ValueRecovery();
}

void SpeculativeJIT::compileDoublePutByVal(Node* node, SpeculateCellOperand& base, SpeculateStrictInt32Operand& property)
{
    Edge child3 = m_jit.graph().varArgChild(node, 2);
    Edge child4 = m_jit.graph().varArgChild(node, 3);

    ArrayMode arrayMode = node->arrayMode();
    
    GPRReg baseReg = base.gpr();
    GPRReg propertyReg = property.gpr();
    
    SpeculateDoubleOperand value(this, child3);

    FPRReg valueReg = value.fpr();
    
    DFG_TYPE_CHECK(
        JSValueRegs(), child3, SpecRealNumber,
        m_jit.branchDouble(
            MacroAssembler::DoubleNotEqualOrUnordered, valueReg, valueReg));
    
    if (!m_compileOkay)
        return;
    
    StorageOperand storage(this, child4);
    GPRReg storageReg = storage.gpr();

    if (node->op() == PutByValAlias) {
        // Store the value to the array.
        GPRReg propertyReg = property.gpr();
        FPRReg valueReg = value.fpr();
        m_jit.storeDouble(valueReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight));
        
        noResult(m_currentNode);
        return;
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
    
    m_jit.storeDouble(valueReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight));

    base.use();
    property.use();
    value.use();
    storage.use();
    
    if (arrayMode.isOutOfBounds()) {
        addSlowPathGenerator(
            slowPathCall(
                slowCase, this,
                m_jit.codeBlock()->isStrictMode() ? operationPutDoubleByValBeyondArrayBoundsStrict : operationPutDoubleByValBeyondArrayBoundsNonStrict,
                NoResult, baseReg, propertyReg, valueReg));
    }

    noResult(m_currentNode, UseChildrenCalledExplicitly);
}

void SpeculativeJIT::compileGetCharCodeAt(Node* node)
{
    SpeculateCellOperand string(this, node->child1());
    SpeculateStrictInt32Operand index(this, node->child2());
    StorageOperand storage(this, node->child3());

    GPRReg stringReg = string.gpr();
    GPRReg indexReg = index.gpr();
    GPRReg storageReg = storage.gpr();
    
    ASSERT(speculationChecked(m_state.forNode(node->child1()).m_type, SpecString));

    // unsigned comparison so we can filter out negative indices and indices that are too large
    speculationCheck(Uncountable, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::AboveOrEqual, indexReg, MacroAssembler::Address(stringReg, JSString::offsetOfLength())));

    GPRTemporary scratch(this);
    GPRReg scratchReg = scratch.gpr();

    m_jit.loadPtr(MacroAssembler::Address(stringReg, JSString::offsetOfValue()), scratchReg);

    // Load the character into scratchReg
    JITCompiler::Jump is16Bit = m_jit.branchTest32(MacroAssembler::Zero, MacroAssembler::Address(scratchReg, StringImpl::flagsOffset()), TrustedImm32(StringImpl::flagIs8Bit()));

    m_jit.load8(MacroAssembler::BaseIndex(storageReg, indexReg, MacroAssembler::TimesOne, 0), scratchReg);
    JITCompiler::Jump cont8Bit = m_jit.jump();

    is16Bit.link(&m_jit);

    m_jit.load16(MacroAssembler::BaseIndex(storageReg, indexReg, MacroAssembler::TimesTwo, 0), scratchReg);

    cont8Bit.link(&m_jit);

    integerResult(scratchReg, m_currentNode);
}

void SpeculativeJIT::compileGetByValOnString(Node* node)
{
    SpeculateCellOperand base(this, node->child1());
    SpeculateStrictInt32Operand property(this, node->child2());
    StorageOperand storage(this, node->child3());
    GPRReg baseReg = base.gpr();
    GPRReg propertyReg = property.gpr();
    GPRReg storageReg = storage.gpr();

    ASSERT(ArrayMode(Array::String).alreadyChecked(m_jit.graph(), node, m_state.forNode(node->child1())));

    // unsigned comparison so we can filter out negative indices and indices that are too large
    speculationCheck(Uncountable, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(baseReg, JSString::offsetOfLength())));

    GPRTemporary scratch(this);
    GPRReg scratchReg = scratch.gpr();

    m_jit.loadPtr(MacroAssembler::Address(baseReg, JSString::offsetOfValue()), scratchReg);

    // Load the character into scratchReg
    JITCompiler::Jump is16Bit = m_jit.branchTest32(MacroAssembler::Zero, MacroAssembler::Address(scratchReg, StringImpl::flagsOffset()), TrustedImm32(StringImpl::flagIs8Bit()));

    m_jit.load8(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesOne, 0), scratchReg);
    JITCompiler::Jump cont8Bit = m_jit.jump();

    is16Bit.link(&m_jit);

    m_jit.load16(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesTwo, 0), scratchReg);

    // We only support ascii characters
    speculationCheck(Uncountable, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::AboveOrEqual, scratchReg, TrustedImm32(0x100)));

    // 8 bit string values don't need the isASCII check.
    cont8Bit.link(&m_jit);

    GPRTemporary smallStrings(this);
    GPRReg smallStringsReg = smallStrings.gpr();
    m_jit.move(MacroAssembler::TrustedImmPtr(m_jit.vm()->smallStrings.singleCharacterStrings()), smallStringsReg);
    m_jit.loadPtr(MacroAssembler::BaseIndex(smallStringsReg, scratchReg, MacroAssembler::ScalePtr, 0), scratchReg);
    speculationCheck(Uncountable, JSValueRegs(), 0, m_jit.branchTest32(MacroAssembler::Zero, scratchReg));
    cellResult(scratchReg, m_currentNode);
}

void SpeculativeJIT::compileFromCharCode(Node* node)
{
    SpeculateStrictInt32Operand property(this, node->child1());
    GPRReg propertyReg = property.gpr();
    GPRTemporary smallStrings(this);
    GPRTemporary scratch(this);
    GPRReg scratchReg = scratch.gpr();
    GPRReg smallStringsReg = smallStrings.gpr();

    JITCompiler::JumpList slowCases;
    slowCases.append(m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, TrustedImm32(0xff)));
    m_jit.move(MacroAssembler::TrustedImmPtr(m_jit.vm()->smallStrings.singleCharacterStrings()), smallStringsReg);
    m_jit.loadPtr(MacroAssembler::BaseIndex(smallStringsReg, propertyReg, MacroAssembler::ScalePtr, 0), scratchReg);

    slowCases.append(m_jit.branchTest32(MacroAssembler::Zero, scratchReg));
    addSlowPathGenerator(slowPathCall(slowCases, this, operationStringFromCharCode, scratchReg, propertyReg));
    cellResult(scratchReg, m_currentNode);
}

GeneratedOperandType SpeculativeJIT::checkGeneratedTypeForToInt32(Node* node)
{
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLogF("checkGeneratedTypeForToInt32@%d   ", node->index());
#endif
    VirtualRegister virtualRegister = node->virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    switch (info.registerFormat()) {
    case DataFormatStorage:
        RELEASE_ASSERT_NOT_REACHED();

    case DataFormatBoolean:
    case DataFormatCell:
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
        return GeneratedOperandTypeUnknown;

    case DataFormatNone:
    case DataFormatJSCell:
    case DataFormatJS:
    case DataFormatJSBoolean:
        return GeneratedOperandJSValue;

    case DataFormatJSInteger:
    case DataFormatInteger:
        return GeneratedOperandInteger;

    case DataFormatJSDouble:
    case DataFormatDouble:
        return GeneratedOperandDouble;
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return GeneratedOperandTypeUnknown;
    }
}

void SpeculativeJIT::compileValueToInt32(Node* node)
{
    switch (node->child1().useKind()) {
    case Int32Use: {
        SpeculateIntegerOperand op1(this, node->child1());
        GPRTemporary result(this, op1);
        m_jit.move(op1.gpr(), result.gpr());
        integerResult(result.gpr(), node, op1.format());
        return;
    }
    
    case NumberUse:
    case NotCellUse: {
        switch (checkGeneratedTypeForToInt32(node->child1().node())) {
        case GeneratedOperandInteger: {
            SpeculateIntegerOperand op1(this, node->child1(), ManualOperandSpeculation);
            GPRTemporary result(this, op1);
            m_jit.move(op1.gpr(), result.gpr());
            integerResult(result.gpr(), node, op1.format());
            return;
        }
        case GeneratedOperandDouble: {
            GPRTemporary result(this);
            SpeculateDoubleOperand op1(this, node->child1(), ManualOperandSpeculation);
            FPRReg fpr = op1.fpr();
            GPRReg gpr = result.gpr();
            JITCompiler::Jump notTruncatedToInteger = m_jit.branchTruncateDoubleToInt32(fpr, gpr, JITCompiler::BranchIfTruncateFailed);
            
            addSlowPathGenerator(slowPathCall(notTruncatedToInteger, this, toInt32, gpr, fpr));

            integerResult(gpr, node);
            return;
        }
        case GeneratedOperandJSValue: {
            GPRTemporary result(this);
#if USE(JSVALUE64)
            JSValueOperand op1(this, node->child1(), ManualOperandSpeculation);

            GPRReg gpr = op1.gpr();
            GPRReg resultGpr = result.gpr();
            FPRTemporary tempFpr(this);
            FPRReg fpr = tempFpr.fpr();

            JITCompiler::Jump isInteger = m_jit.branch64(MacroAssembler::AboveOrEqual, gpr, GPRInfo::tagTypeNumberRegister);
            JITCompiler::JumpList converted;

            if (node->child1().useKind() == NumberUse) {
                DFG_TYPE_CHECK(
                    JSValueRegs(gpr), node->child1(), SpecNumber,
                    m_jit.branchTest64(
                        MacroAssembler::Zero, gpr, GPRInfo::tagTypeNumberRegister));
            } else {
                JITCompiler::Jump isNumber = m_jit.branchTest64(MacroAssembler::NonZero, gpr, GPRInfo::tagTypeNumberRegister);
                
                DFG_TYPE_CHECK(
                    JSValueRegs(gpr), node->child1(), ~SpecCell,
                    m_jit.branchTest64(
                        JITCompiler::Zero, gpr, GPRInfo::tagMaskRegister));
                
                // It's not a cell: so true turns into 1 and all else turns into 0.
                m_jit.compare64(JITCompiler::Equal, gpr, TrustedImm32(ValueTrue), resultGpr);
                converted.append(m_jit.jump());
                
                isNumber.link(&m_jit);
            }

            // First, if we get here we have a double encoded as a JSValue
            m_jit.move(gpr, resultGpr);
            unboxDouble(resultGpr, fpr);

            silentSpillAllRegisters(resultGpr);
            callOperation(toInt32, resultGpr, fpr);
            silentFillAllRegisters(resultGpr);

            converted.append(m_jit.jump());

            isInteger.link(&m_jit);
            m_jit.zeroExtend32ToPtr(gpr, resultGpr);

            converted.link(&m_jit);
#else
            Node* childNode = node->child1().node();
            VirtualRegister virtualRegister = childNode->virtualRegister();
            GenerationInfo& info = m_generationInfo[virtualRegister];

            JSValueOperand op1(this, node->child1(), ManualOperandSpeculation);

            GPRReg payloadGPR = op1.payloadGPR();
            GPRReg resultGpr = result.gpr();
        
            JITCompiler::JumpList converted;

            if (info.registerFormat() == DataFormatJSInteger)
                m_jit.move(payloadGPR, resultGpr);
            else {
                GPRReg tagGPR = op1.tagGPR();
                FPRTemporary tempFpr(this);
                FPRReg fpr = tempFpr.fpr();
                FPRTemporary scratch(this);

                JITCompiler::Jump isInteger = m_jit.branch32(MacroAssembler::Equal, tagGPR, TrustedImm32(JSValue::Int32Tag));

                if (node->child1().useKind() == NumberUse) {
                    DFG_TYPE_CHECK(
                        JSValueRegs(tagGPR, payloadGPR), node->child1(), SpecNumber,
                        m_jit.branch32(
                            MacroAssembler::AboveOrEqual, tagGPR,
                            TrustedImm32(JSValue::LowestTag)));
                } else {
                    JITCompiler::Jump isNumber = m_jit.branch32(MacroAssembler::Below, tagGPR, TrustedImm32(JSValue::LowestTag));
                    
                    DFG_TYPE_CHECK(
                        JSValueRegs(tagGPR, payloadGPR), node->child1(), ~SpecCell,
                        m_jit.branch32(
                            JITCompiler::Equal, tagGPR, TrustedImm32(JSValue::CellTag)));
                    
                    // It's not a cell: so true turns into 1 and all else turns into 0.
                    JITCompiler::Jump isBoolean = m_jit.branch32(JITCompiler::Equal, tagGPR, TrustedImm32(JSValue::BooleanTag));
                    m_jit.move(TrustedImm32(0), resultGpr);
                    converted.append(m_jit.jump());
                    
                    isBoolean.link(&m_jit);
                    m_jit.move(payloadGPR, resultGpr);
                    converted.append(m_jit.jump());
                    
                    isNumber.link(&m_jit);
                }

                unboxDouble(tagGPR, payloadGPR, fpr, scratch.fpr());

                silentSpillAllRegisters(resultGpr);
                callOperation(toInt32, resultGpr, fpr);
                silentFillAllRegisters(resultGpr);

                converted.append(m_jit.jump());

                isInteger.link(&m_jit);
                m_jit.move(payloadGPR, resultGpr);

                converted.link(&m_jit);
            }
#endif
            integerResult(resultGpr, node);
            return;
        }
        case GeneratedOperandTypeUnknown:
            RELEASE_ASSERT(!m_compileOkay);
            return;
        }
        RELEASE_ASSERT_NOT_REACHED();
        return;
    }
    
    case BooleanUse: {
        SpeculateBooleanOperand op1(this, node->child1());
        GPRTemporary result(this, op1);
        
        m_jit.move(op1.gpr(), result.gpr());
        m_jit.and32(JITCompiler::TrustedImm32(1), result.gpr());
        
        integerResult(result.gpr(), node);
        return;
    }

    default:
        ASSERT(!m_compileOkay);
        return;
    }
}

void SpeculativeJIT::compileUInt32ToNumber(Node* node)
{
    if (!nodeCanSpeculateInteger(node->arithNodeFlags())) {
        // We know that this sometimes produces doubles. So produce a double every
        // time. This at least allows subsequent code to not have weird conditionals.
            
        IntegerOperand op1(this, node->child1());
        FPRTemporary result(this);
            
        GPRReg inputGPR = op1.gpr();
        FPRReg outputFPR = result.fpr();
            
        m_jit.convertInt32ToDouble(inputGPR, outputFPR);
            
        JITCompiler::Jump positive = m_jit.branch32(MacroAssembler::GreaterThanOrEqual, inputGPR, TrustedImm32(0));
        m_jit.addDouble(JITCompiler::AbsoluteAddress(&AssemblyHelpers::twoToThe32), outputFPR);
        positive.link(&m_jit);
            
        doubleResult(outputFPR, node);
        return;
    }

    IntegerOperand op1(this, node->child1());
    GPRTemporary result(this); // For the benefit of OSR exit, force these to be in different registers. In reality the OSR exit compiler could find cases where you have uint32(%r1) followed by int32(%r1) and then use different registers, but that seems like too much effort.

    m_jit.move(op1.gpr(), result.gpr());

    // Test the operand is positive. This is a very special speculation check - we actually
    // use roll-forward speculation here, where if this fails, we jump to the baseline
    // instruction that follows us, rather than the one we're executing right now. We have
    // to do this because by this point, the original values necessary to compile whatever
    // operation the UInt32ToNumber originated from might be dead.
    forwardSpeculationCheck(Overflow, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::LessThan, result.gpr(), TrustedImm32(0)), ValueRecovery::uint32InGPR(result.gpr()));

    integerResult(result.gpr(), node, op1.format());
}

void SpeculativeJIT::compileDoubleAsInt32(Node* node)
{
    SpeculateDoubleOperand op1(this, node->child1());
    FPRTemporary scratch(this);
    GPRTemporary result(this);
    
    FPRReg valueFPR = op1.fpr();
    FPRReg scratchFPR = scratch.fpr();
    GPRReg resultGPR = result.gpr();

    JITCompiler::JumpList failureCases;
    bool negZeroCheck = !nodeCanIgnoreNegativeZero(node->arithNodeFlags());
    m_jit.branchConvertDoubleToInt32(valueFPR, resultGPR, failureCases, scratchFPR, negZeroCheck);
    forwardSpeculationCheck(Overflow, JSValueRegs(), 0, failureCases, ValueRecovery::inFPR(valueFPR));

    integerResult(resultGPR, node);
}

void SpeculativeJIT::compileInt32ToDouble(Node* node)
{
    ASSERT(!isInt32Constant(node->child1().node())); // This should have been constant folded.
    
    if (isInt32Speculation(m_state.forNode(node->child1()).m_type)) {
        SpeculateIntegerOperand op1(this, node->child1(), ManualOperandSpeculation);
        FPRTemporary result(this);
        m_jit.convertInt32ToDouble(op1.gpr(), result.fpr());
        doubleResult(result.fpr(), node);
        return;
    }
    
    JSValueOperand op1(this, node->child1(), ManualOperandSpeculation);
    FPRTemporary result(this);
    
#if USE(JSVALUE64)
    GPRTemporary temp(this);

    GPRReg op1GPR = op1.gpr();
    GPRReg tempGPR = temp.gpr();
    FPRReg resultFPR = result.fpr();
    
    JITCompiler::Jump isInteger = m_jit.branch64(
        MacroAssembler::AboveOrEqual, op1GPR, GPRInfo::tagTypeNumberRegister);
    
    if (needsTypeCheck(node->child1(), SpecNumber)) {
        if (node->op() == ForwardInt32ToDouble) {
            forwardTypeCheck(
                JSValueRegs(op1GPR), node->child1(), SpecNumber,
                m_jit.branchTest64(MacroAssembler::Zero, op1GPR, GPRInfo::tagTypeNumberRegister),
                ValueRecovery::inGPR(op1GPR, DataFormatJS));
        } else {
            backwardTypeCheck(
                JSValueRegs(op1GPR), node->child1(), SpecNumber,
                m_jit.branchTest64(MacroAssembler::Zero, op1GPR, GPRInfo::tagTypeNumberRegister));
        }
    }
    
    m_jit.move(op1GPR, tempGPR);
    unboxDouble(tempGPR, resultFPR);
    JITCompiler::Jump done = m_jit.jump();
    
    isInteger.link(&m_jit);
    m_jit.convertInt32ToDouble(op1GPR, resultFPR);
    done.link(&m_jit);
#else
    FPRTemporary temp(this);
    
    GPRReg op1TagGPR = op1.tagGPR();
    GPRReg op1PayloadGPR = op1.payloadGPR();
    FPRReg tempFPR = temp.fpr();
    FPRReg resultFPR = result.fpr();
    
    JITCompiler::Jump isInteger = m_jit.branch32(
        MacroAssembler::Equal, op1TagGPR, TrustedImm32(JSValue::Int32Tag));
    
    if (needsTypeCheck(node->child1(), SpecNumber)) {
        if (node->op() == ForwardInt32ToDouble) {
            forwardTypeCheck(
                JSValueRegs(op1TagGPR, op1PayloadGPR), node->child1(), SpecNumber,
                m_jit.branch32(MacroAssembler::AboveOrEqual, op1TagGPR, TrustedImm32(JSValue::LowestTag)),
                ValueRecovery::inPair(op1TagGPR, op1PayloadGPR));
        } else {
            backwardTypeCheck(
                JSValueRegs(op1TagGPR, op1PayloadGPR), node->child1(), SpecNumber,
                m_jit.branch32(MacroAssembler::AboveOrEqual, op1TagGPR, TrustedImm32(JSValue::LowestTag)));
        }
    }
    
    unboxDouble(op1TagGPR, op1PayloadGPR, resultFPR, tempFPR);
    JITCompiler::Jump done = m_jit.jump();
    
    isInteger.link(&m_jit);
    m_jit.convertInt32ToDouble(op1PayloadGPR, resultFPR);
    done.link(&m_jit);
#endif
    
    doubleResult(resultFPR, node);
}

static double clampDoubleToByte(double d)
{
    d += 0.5;
    if (!(d > 0))
        d = 0;
    else if (d > 255)
        d = 255;
    return d;
}

static void compileClampIntegerToByte(JITCompiler& jit, GPRReg result)
{
    MacroAssembler::Jump inBounds = jit.branch32(MacroAssembler::BelowOrEqual, result, JITCompiler::TrustedImm32(0xff));
    MacroAssembler::Jump tooBig = jit.branch32(MacroAssembler::GreaterThan, result, JITCompiler::TrustedImm32(0xff));
    jit.xorPtr(result, result);
    MacroAssembler::Jump clamped = jit.jump();
    tooBig.link(&jit);
    jit.move(JITCompiler::TrustedImm32(255), result);
    clamped.link(&jit);
    inBounds.link(&jit);
}

static void compileClampDoubleToByte(JITCompiler& jit, GPRReg result, FPRReg source, FPRReg scratch)
{
    // Unordered compare so we pick up NaN
    static const double zero = 0;
    static const double byteMax = 255;
    static const double half = 0.5;
    jit.loadDouble(&zero, scratch);
    MacroAssembler::Jump tooSmall = jit.branchDouble(MacroAssembler::DoubleLessThanOrEqualOrUnordered, source, scratch);
    jit.loadDouble(&byteMax, scratch);
    MacroAssembler::Jump tooBig = jit.branchDouble(MacroAssembler::DoubleGreaterThan, source, scratch);
    
    jit.loadDouble(&half, scratch);
    // FIXME: This should probably just use a floating point round!
    // https://bugs.webkit.org/show_bug.cgi?id=72054
    jit.addDouble(source, scratch);
    jit.truncateDoubleToInt32(scratch, result);   
    MacroAssembler::Jump truncatedInt = jit.jump();
    
    tooSmall.link(&jit);
    jit.xorPtr(result, result);
    MacroAssembler::Jump zeroed = jit.jump();
    
    tooBig.link(&jit);
    jit.move(JITCompiler::TrustedImm32(255), result);
    
    truncatedInt.link(&jit);
    zeroed.link(&jit);

}

void SpeculativeJIT::compileGetByValOnIntTypedArray(const TypedArrayDescriptor& descriptor, Node* node, size_t elementSize, TypedArraySignedness signedness)
{
    SpeculateCellOperand base(this, node->child1());
    SpeculateStrictInt32Operand property(this, node->child2());
    StorageOperand storage(this, node->child3());

    GPRReg baseReg = base.gpr();
    GPRReg propertyReg = property.gpr();
    GPRReg storageReg = storage.gpr();

    GPRTemporary result(this);
    GPRReg resultReg = result.gpr();

    ASSERT(node->arrayMode().alreadyChecked(m_jit.graph(), node, m_state.forNode(node->child1())));

    speculationCheck(
        Uncountable, JSValueRegs(), 0,
        m_jit.branch32(
            MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(baseReg, descriptor.m_lengthOffset)));
    switch (elementSize) {
    case 1:
        if (signedness == SignedTypedArray)
            m_jit.load8Signed(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesOne), resultReg);
        else
            m_jit.load8(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesOne), resultReg);
        break;
    case 2:
        if (signedness == SignedTypedArray)
            m_jit.load16Signed(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesTwo), resultReg);
        else
            m_jit.load16(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesTwo), resultReg);
        break;
    case 4:
        m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesFour), resultReg);
        break;
    default:
        CRASH();
    }
    if (elementSize < 4 || signedness == SignedTypedArray) {
        integerResult(resultReg, node);
        return;
    }
    
    ASSERT(elementSize == 4 && signedness == UnsignedTypedArray);
    if (node->shouldSpeculateInteger()) {
        forwardSpeculationCheck(Overflow, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::LessThan, resultReg, TrustedImm32(0)), ValueRecovery::uint32InGPR(resultReg));
        integerResult(resultReg, node);
        return;
    }
    
    FPRTemporary fresult(this);
    m_jit.convertInt32ToDouble(resultReg, fresult.fpr());
    JITCompiler::Jump positive = m_jit.branch32(MacroAssembler::GreaterThanOrEqual, resultReg, TrustedImm32(0));
    m_jit.addDouble(JITCompiler::AbsoluteAddress(&AssemblyHelpers::twoToThe32), fresult.fpr());
    positive.link(&m_jit);
    doubleResult(fresult.fpr(), node);
}

void SpeculativeJIT::compilePutByValForIntTypedArray(const TypedArrayDescriptor& descriptor, GPRReg base, GPRReg property, Node* node, size_t elementSize, TypedArraySignedness signedness, TypedArrayRounding rounding)
{
    StorageOperand storage(this, m_jit.graph().varArgChild(node, 3));
    GPRReg storageReg = storage.gpr();
    
    Edge valueUse = m_jit.graph().varArgChild(node, 2);
    
    GPRTemporary value;
    GPRReg valueGPR = InvalidGPRReg;
    
    if (valueUse->isConstant()) {
        JSValue jsValue = valueOfJSConstant(valueUse.node());
        if (!jsValue.isNumber()) {
            terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
            noResult(node);
            return;
        }
        double d = jsValue.asNumber();
        if (rounding == ClampRounding) {
            ASSERT(elementSize == 1);
            d = clampDoubleToByte(d);
        }
        GPRTemporary scratch(this);
        GPRReg scratchReg = scratch.gpr();
        m_jit.move(Imm32(toInt32(d)), scratchReg);
        value.adopt(scratch);
        valueGPR = scratchReg;
    } else {
        switch (valueUse.useKind()) {
        case Int32Use: {
            SpeculateIntegerOperand valueOp(this, valueUse);
            GPRTemporary scratch(this);
            GPRReg scratchReg = scratch.gpr();
            m_jit.move(valueOp.gpr(), scratchReg);
            if (rounding == ClampRounding) {
                ASSERT(elementSize == 1);
                compileClampIntegerToByte(m_jit, scratchReg);
            }
            value.adopt(scratch);
            valueGPR = scratchReg;
            break;
        }
            
        case NumberUse: {
            if (rounding == ClampRounding) {
                ASSERT(elementSize == 1);
                SpeculateDoubleOperand valueOp(this, valueUse);
                GPRTemporary result(this);
                FPRTemporary floatScratch(this);
                FPRReg fpr = valueOp.fpr();
                GPRReg gpr = result.gpr();
                compileClampDoubleToByte(m_jit, gpr, fpr, floatScratch.fpr());
                value.adopt(result);
                valueGPR = gpr;
            } else {
                SpeculateDoubleOperand valueOp(this, valueUse);
                GPRTemporary result(this);
                FPRReg fpr = valueOp.fpr();
                GPRReg gpr = result.gpr();
                MacroAssembler::Jump notNaN = m_jit.branchDouble(MacroAssembler::DoubleEqual, fpr, fpr);
                m_jit.xorPtr(gpr, gpr);
                MacroAssembler::Jump fixed = m_jit.jump();
                notNaN.link(&m_jit);
                
                MacroAssembler::Jump failed;
                if (signedness == SignedTypedArray)
                    failed = m_jit.branchTruncateDoubleToInt32(fpr, gpr, MacroAssembler::BranchIfTruncateFailed);
                else
                    failed = m_jit.branchTruncateDoubleToUint32(fpr, gpr, MacroAssembler::BranchIfTruncateFailed);
                
                addSlowPathGenerator(slowPathCall(failed, this, toInt32, gpr, fpr));
                
                fixed.link(&m_jit);
                value.adopt(result);
                valueGPR = gpr;
            }
            break;
        }
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
    }
    
    ASSERT_UNUSED(valueGPR, valueGPR != property);
    ASSERT(valueGPR != base);
    ASSERT(valueGPR != storageReg);
    MacroAssembler::Jump outOfBounds;
    if (node->op() == PutByVal)
        outOfBounds = m_jit.branch32(MacroAssembler::AboveOrEqual, property, MacroAssembler::Address(base, descriptor.m_lengthOffset));

    switch (elementSize) {
    case 1:
        m_jit.store8(value.gpr(), MacroAssembler::BaseIndex(storageReg, property, MacroAssembler::TimesOne));
        break;
    case 2:
        m_jit.store16(value.gpr(), MacroAssembler::BaseIndex(storageReg, property, MacroAssembler::TimesTwo));
        break;
    case 4:
        m_jit.store32(value.gpr(), MacroAssembler::BaseIndex(storageReg, property, MacroAssembler::TimesFour));
        break;
    default:
        CRASH();
    }
    if (node->op() == PutByVal)
        outOfBounds.link(&m_jit);
    noResult(node);
}

void SpeculativeJIT::compileGetByValOnFloatTypedArray(const TypedArrayDescriptor& descriptor, Node* node, size_t elementSize)
{
    SpeculateCellOperand base(this, node->child1());
    SpeculateStrictInt32Operand property(this, node->child2());
    StorageOperand storage(this, node->child3());

    GPRReg baseReg = base.gpr();
    GPRReg propertyReg = property.gpr();
    GPRReg storageReg = storage.gpr();

    ASSERT(node->arrayMode().alreadyChecked(m_jit.graph(), node, m_state.forNode(node->child1())));

    FPRTemporary result(this);
    FPRReg resultReg = result.fpr();
    speculationCheck(
        Uncountable, JSValueRegs(), 0,
        m_jit.branch32(
            MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(baseReg, descriptor.m_lengthOffset)));
    switch (elementSize) {
    case 4:
        m_jit.loadFloat(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesFour), resultReg);
        m_jit.convertFloatToDouble(resultReg, resultReg);
        break;
    case 8: {
        m_jit.loadDouble(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight), resultReg);
        break;
    }
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
    
    MacroAssembler::Jump notNaN = m_jit.branchDouble(MacroAssembler::DoubleEqual, resultReg, resultReg);
    static const double NaN = QNaN;
    m_jit.loadDouble(&NaN, resultReg);
    notNaN.link(&m_jit);
    
    doubleResult(resultReg, node);
}

void SpeculativeJIT::compilePutByValForFloatTypedArray(const TypedArrayDescriptor& descriptor, GPRReg base, GPRReg property, Node* node, size_t elementSize)
{
    StorageOperand storage(this, m_jit.graph().varArgChild(node, 3));
    GPRReg storageReg = storage.gpr();
    
    Edge baseUse = m_jit.graph().varArgChild(node, 0);
    Edge valueUse = m_jit.graph().varArgChild(node, 2);

    SpeculateDoubleOperand valueOp(this, valueUse);
    FPRTemporary scratch(this);
    FPRReg valueFPR = valueOp.fpr();
    FPRReg scratchFPR = scratch.fpr();

    ASSERT_UNUSED(baseUse, node->arrayMode().alreadyChecked(m_jit.graph(), node, m_state.forNode(baseUse)));
    
    MacroAssembler::Jump outOfBounds;
    if (node->op() == PutByVal)
        outOfBounds = m_jit.branch32(MacroAssembler::AboveOrEqual, property, MacroAssembler::Address(base, descriptor.m_lengthOffset));
    
    switch (elementSize) {
    case 4: {
        m_jit.moveDouble(valueFPR, scratchFPR);
        m_jit.convertDoubleToFloat(valueFPR, scratchFPR);
        m_jit.storeFloat(scratchFPR, MacroAssembler::BaseIndex(storageReg, property, MacroAssembler::TimesFour));
        break;
    }
    case 8:
        m_jit.storeDouble(valueFPR, MacroAssembler::BaseIndex(storageReg, property, MacroAssembler::TimesEight));
        break;
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
    if (node->op() == PutByVal)
        outOfBounds.link(&m_jit);
    noResult(node);
}

void SpeculativeJIT::compileInstanceOfForObject(Node*, GPRReg valueReg, GPRReg prototypeReg, GPRReg scratchReg)
{
    // Check that prototype is an object.
    m_jit.loadPtr(MacroAssembler::Address(prototypeReg, JSCell::structureOffset()), scratchReg);
    speculationCheck(BadType, JSValueRegs(), 0, m_jit.branchIfNotObject(scratchReg));
    
    // Initialize scratchReg with the value being checked.
    m_jit.move(valueReg, scratchReg);
    
    // Walk up the prototype chain of the value (in scratchReg), comparing to prototypeReg.
    MacroAssembler::Label loop(&m_jit);
    m_jit.loadPtr(MacroAssembler::Address(scratchReg, JSCell::structureOffset()), scratchReg);
#if USE(JSVALUE64)
    m_jit.load64(MacroAssembler::Address(scratchReg, Structure::prototypeOffset()), scratchReg);
    MacroAssembler::Jump isInstance = m_jit.branch64(MacroAssembler::Equal, scratchReg, prototypeReg);
    m_jit.branchTest64(MacroAssembler::Zero, scratchReg, GPRInfo::tagMaskRegister).linkTo(loop, &m_jit);
#else
    m_jit.load32(MacroAssembler::Address(scratchReg, Structure::prototypeOffset() + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), scratchReg);
    MacroAssembler::Jump isInstance = m_jit.branchPtr(MacroAssembler::Equal, scratchReg, prototypeReg);
    m_jit.branchTest32(MacroAssembler::NonZero, scratchReg).linkTo(loop, &m_jit);
#endif
    
    // No match - result is false.
#if USE(JSVALUE64)
    m_jit.move(MacroAssembler::TrustedImm64(JSValue::encode(jsBoolean(false))), scratchReg);
#else
    m_jit.move(MacroAssembler::TrustedImm32(0), scratchReg);
#endif
    MacroAssembler::Jump putResult = m_jit.jump();
    
    isInstance.link(&m_jit);
#if USE(JSVALUE64)
    m_jit.move(MacroAssembler::TrustedImm64(JSValue::encode(jsBoolean(true))), scratchReg);
#else
    m_jit.move(MacroAssembler::TrustedImm32(1), scratchReg);
#endif
    
    putResult.link(&m_jit);
}

void SpeculativeJIT::compileInstanceOf(Node* node)
{
    if (node->child1().useKind() == UntypedUse) {
        // It might not be a cell. Speculate less aggressively.
        // Or: it might only be used once (i.e. by us), so we get zero benefit
        // from speculating any more aggressively than we absolutely need to.
        
        JSValueOperand value(this, node->child1());
        SpeculateCellOperand prototype(this, node->child2());
        GPRTemporary scratch(this);
        
        GPRReg prototypeReg = prototype.gpr();
        GPRReg scratchReg = scratch.gpr();
        
#if USE(JSVALUE64)
        GPRReg valueReg = value.gpr();
        MacroAssembler::Jump isCell = m_jit.branchTest64(MacroAssembler::Zero, valueReg, GPRInfo::tagMaskRegister);
        m_jit.move(MacroAssembler::TrustedImm64(JSValue::encode(jsBoolean(false))), scratchReg);
#else
        GPRReg valueTagReg = value.tagGPR();
        GPRReg valueReg = value.payloadGPR();
        MacroAssembler::Jump isCell = m_jit.branch32(MacroAssembler::Equal, valueTagReg, TrustedImm32(JSValue::CellTag));
        m_jit.move(MacroAssembler::TrustedImm32(0), scratchReg);
#endif

        MacroAssembler::Jump done = m_jit.jump();
        
        isCell.link(&m_jit);
        
        compileInstanceOfForObject(node, valueReg, prototypeReg, scratchReg);
        
        done.link(&m_jit);

#if USE(JSVALUE64)
        jsValueResult(scratchReg, node, DataFormatJSBoolean);
#else
        booleanResult(scratchReg, node);
#endif
        return;
    }
    
    SpeculateCellOperand value(this, node->child1());
    SpeculateCellOperand prototype(this, node->child2());
    
    GPRTemporary scratch(this);
    
    GPRReg valueReg = value.gpr();
    GPRReg prototypeReg = prototype.gpr();
    GPRReg scratchReg = scratch.gpr();
    
    compileInstanceOfForObject(node, valueReg, prototypeReg, scratchReg);

#if USE(JSVALUE64)
    jsValueResult(scratchReg, node, DataFormatJSBoolean);
#else
    booleanResult(scratchReg, node);
#endif
}

void SpeculativeJIT::compileSoftModulo(Node* node)
{
    // In the fast path, the dividend value could be the final result
    // (in case of |dividend| < |divisor|), so we speculate it as strict int32.
    SpeculateStrictInt32Operand op1(this, node->child1());
#if CPU(X86) || CPU(X86_64)
    if (isInt32Constant(node->child2().node())) {
        int32_t divisor = valueOfInt32Constant(node->child2().node());
        if (divisor) {
            GPRReg op1Gpr = op1.gpr();

            GPRTemporary eax(this, X86Registers::eax);
            GPRTemporary edx(this, X86Registers::edx);
            GPRTemporary scratch(this);
            GPRReg scratchGPR = scratch.gpr();

            GPRReg op1SaveGPR;
            if (op1Gpr == X86Registers::eax || op1Gpr == X86Registers::edx) {
                op1SaveGPR = allocate();
                ASSERT(op1Gpr != op1SaveGPR);
                m_jit.move(op1Gpr, op1SaveGPR);
            } else
                op1SaveGPR = op1Gpr;
            ASSERT(op1SaveGPR != X86Registers::eax);
            ASSERT(op1SaveGPR != X86Registers::edx);

            m_jit.move(op1Gpr, eax.gpr());
            m_jit.move(TrustedImm32(divisor), scratchGPR);
            if (divisor == -1)
                speculationCheck(Overflow, JSValueRegs(), 0, m_jit.branch32(JITCompiler::Equal, eax.gpr(), TrustedImm32(-2147483647-1)));
            m_jit.assembler().cdq();
            m_jit.assembler().idivl_r(scratchGPR);
            if (!nodeCanIgnoreNegativeZero(node->arithNodeFlags())) {
                // Check that we're not about to create negative zero.
                JITCompiler::Jump numeratorPositive = m_jit.branch32(JITCompiler::GreaterThanOrEqual, op1SaveGPR, TrustedImm32(0));
                speculationCheck(NegativeZero, JSValueRegs(), 0, m_jit.branchTest32(JITCompiler::Zero, edx.gpr()));
                numeratorPositive.link(&m_jit);
            }
            if (op1SaveGPR != op1Gpr)
                unlock(op1SaveGPR);

            integerResult(edx.gpr(), node);
            return;
        }
    }
#elif CPU(APPLE_ARMV7S) || CPU(ARM_THUMB2)
    if (isInt32Constant(node->child2().node())) {
        int32_t divisor = valueOfInt32Constant(node->child2().node());
        if (divisor > 0 && hasOneBitSet(divisor)) { // If power of 2 then just mask
            GPRReg dividendGPR = op1.gpr();
            GPRTemporary result(this);
            GPRReg resultGPR = result.gpr();

            m_jit.assembler().cmp(dividendGPR, ARMThumbImmediate::makeEncodedImm(0));
            m_jit.assembler().it(ARMv7Assembler::ConditionLT, false);
            m_jit.assembler().neg(resultGPR, dividendGPR);
            m_jit.assembler().mov(resultGPR, dividendGPR);
            m_jit.and32(TrustedImm32(divisor - 1), resultGPR);
            m_jit.assembler().it(ARMv7Assembler::ConditionLT);
            m_jit.assembler().neg(resultGPR, resultGPR);

            if (!nodeCanIgnoreNegativeZero(node->arithNodeFlags())) {
                // Check that we're not about to create negative zero.
                JITCompiler::Jump numeratorPositive = m_jit.branch32(JITCompiler::GreaterThanOrEqual, dividendGPR, TrustedImm32(0));
                speculationCheck(NegativeZero, JSValueRegs(), 0, m_jit.branchTest32(JITCompiler::Zero, resultGPR));
                numeratorPositive.link(&m_jit);
            }
            integerResult(resultGPR, node);
            return;
        }
    }
#endif

    SpeculateIntegerOperand op2(this, node->child2());
#if CPU(X86) || CPU(X86_64)
    GPRTemporary eax(this, X86Registers::eax);
    GPRTemporary edx(this, X86Registers::edx);
    GPRReg op1GPR = op1.gpr();
    GPRReg op2GPR = op2.gpr();
    
    GPRReg op2TempGPR;
    GPRReg temp;
    GPRReg op1SaveGPR;
    
    if (op2GPR == X86Registers::eax || op2GPR == X86Registers::edx) {
        op2TempGPR = allocate();
        temp = op2TempGPR;
    } else {
        op2TempGPR = InvalidGPRReg;
        if (op1GPR == X86Registers::eax)
            temp = X86Registers::edx;
        else
            temp = X86Registers::eax;
    }
    
    if (op1GPR == X86Registers::eax || op1GPR == X86Registers::edx) {
        op1SaveGPR = allocate();
        ASSERT(op1GPR != op1SaveGPR);
        m_jit.move(op1GPR, op1SaveGPR);
    } else
        op1SaveGPR = op1GPR;
    
    ASSERT(temp != op1GPR);
    ASSERT(temp != op2GPR);
    ASSERT(op1SaveGPR != X86Registers::eax);
    ASSERT(op1SaveGPR != X86Registers::edx);
    
    m_jit.add32(JITCompiler::TrustedImm32(1), op2GPR, temp);
    
    JITCompiler::Jump safeDenominator = m_jit.branch32(JITCompiler::Above, temp, JITCompiler::TrustedImm32(1));
    
    JITCompiler::Jump done;
    // FIXME: if the node is not used as number then we can do this more easily.
    speculationCheck(Overflow, JSValueRegs(), 0, m_jit.branchTest32(JITCompiler::Zero, op2GPR));
    speculationCheck(Overflow, JSValueRegs(), 0, m_jit.branch32(JITCompiler::Equal, op1GPR, TrustedImm32(-2147483647-1)));
    
    safeDenominator.link(&m_jit);
            
    if (op2TempGPR != InvalidGPRReg) {
        m_jit.move(op2GPR, op2TempGPR);
        op2GPR = op2TempGPR;
    }
            
    m_jit.move(op1GPR, eax.gpr());
    m_jit.assembler().cdq();
    m_jit.assembler().idivl_r(op2GPR);
            
    if (op2TempGPR != InvalidGPRReg)
        unlock(op2TempGPR);

    if (!nodeCanIgnoreNegativeZero(node->arithNodeFlags())) {
        // Check that we're not about to create negative zero.
        JITCompiler::Jump numeratorPositive = m_jit.branch32(JITCompiler::GreaterThanOrEqual, op1SaveGPR, TrustedImm32(0));
        speculationCheck(NegativeZero, JSValueRegs(), 0, m_jit.branchTest32(JITCompiler::Zero, edx.gpr()));
        numeratorPositive.link(&m_jit);
    }
    
    if (op1SaveGPR != op1GPR)
        unlock(op1SaveGPR);
            
    integerResult(edx.gpr(), node);

#elif CPU(APPLE_ARMV7S)
    GPRTemporary temp(this);
    GPRTemporary quotientThenRemainder(this);
    GPRTemporary multiplyAnswer(this);
    GPRReg dividendGPR = op1.gpr();
    GPRReg divisorGPR = op2.gpr();
    GPRReg quotientThenRemainderGPR = quotientThenRemainder.gpr();
    GPRReg multiplyAnswerGPR = multiplyAnswer.gpr();

    m_jit.assembler().sdiv(quotientThenRemainderGPR, dividendGPR, divisorGPR);
    speculationCheck(Overflow, JSValueRegs(), 0, m_jit.branchMul32(JITCompiler::Overflow, quotientThenRemainderGPR, divisorGPR, multiplyAnswerGPR));
    m_jit.assembler().sub(quotientThenRemainderGPR, dividendGPR, multiplyAnswerGPR);

    // If the user cares about negative zero, then speculate that we're not about
    // to produce negative zero.
    if (!nodeCanIgnoreNegativeZero(node->arithNodeFlags())) {
        // Check that we're not about to create negative zero.
        JITCompiler::Jump numeratorPositive = m_jit.branch32(JITCompiler::GreaterThanOrEqual, dividendGPR, TrustedImm32(0));
        speculationCheck(NegativeZero, JSValueRegs(), 0, m_jit.branchTest32(JITCompiler::Zero, quotientThenRemainderGPR));
        numeratorPositive.link(&m_jit);
    }

    integerResult(quotientThenRemainderGPR, node);
#else // not architecture that can do integer division
    // Do this the *safest* way possible: call out to a C function that will do the modulo,
    // and then attempt to convert back.
    GPRReg op1GPR = op1.gpr();
    GPRReg op2GPR = op2.gpr();
    
    FPRResult result(this);
    
    flushRegisters();
    if (!nodeCanIgnoreNegativeZero(node->arithNodeFlags())) {
        // NegativeZero check will need op1GPR and fmod call is likely to clobber it.
        m_jit.push(op1GPR);
        callOperation(operationFModOnInts, result.fpr(), op1GPR, op2GPR);
        m_jit.pop(op1GPR);
    } else
        callOperation(operationFModOnInts, result.fpr(), op1GPR, op2GPR);
    
    FPRTemporary scratch(this);
    GPRTemporary intResult(this);
    JITCompiler::JumpList failureCases;
    m_jit.branchConvertDoubleToInt32(result.fpr(), intResult.gpr(), failureCases, scratch.fpr(), false);
    speculationCheck(Overflow, JSValueRegs(), 0, failureCases);
    if (!nodeCanIgnoreNegativeZero(node->arithNodeFlags())) {
        // Check that we're not about to create negative zero.
        JITCompiler::Jump numeratorPositive = m_jit.branch32(JITCompiler::GreaterThanOrEqual, op1GPR, TrustedImm32(0));
        speculationCheck(NegativeZero, JSValueRegs(), 0, m_jit.branchTest32(JITCompiler::Zero, intResult.gpr()));
        numeratorPositive.link(&m_jit);
    }
    
    integerResult(intResult.gpr(), node);
#endif // CPU(X86) || CPU(X86_64)
}

void SpeculativeJIT::compileAdd(Node* node)
{
    switch (node->binaryUseKind()) {
    case Int32Use: {
        if (isNumberConstant(node->child1().node())) {
            int32_t imm1 = valueOfInt32Constant(node->child1().node());
            SpeculateIntegerOperand op2(this, node->child2());
            GPRTemporary result(this);

            if (nodeCanTruncateInteger(node->arithNodeFlags())) {
                m_jit.move(op2.gpr(), result.gpr());
                m_jit.add32(Imm32(imm1), result.gpr());
            } else
                speculationCheck(Overflow, JSValueRegs(), 0, m_jit.branchAdd32(MacroAssembler::Overflow, op2.gpr(), Imm32(imm1), result.gpr()));

            integerResult(result.gpr(), node);
            return;
        }
                
        if (isNumberConstant(node->child2().node())) {
            SpeculateIntegerOperand op1(this, node->child1());
            int32_t imm2 = valueOfInt32Constant(node->child2().node());
            GPRTemporary result(this);
                
            if (nodeCanTruncateInteger(node->arithNodeFlags())) {
                m_jit.move(op1.gpr(), result.gpr());
                m_jit.add32(Imm32(imm2), result.gpr());
            } else
                speculationCheck(Overflow, JSValueRegs(), 0, m_jit.branchAdd32(MacroAssembler::Overflow, op1.gpr(), Imm32(imm2), result.gpr()));

            integerResult(result.gpr(), node);
            return;
        }
                
        SpeculateIntegerOperand op1(this, node->child1());
        SpeculateIntegerOperand op2(this, node->child2());
        GPRTemporary result(this, op1, op2);

        GPRReg gpr1 = op1.gpr();
        GPRReg gpr2 = op2.gpr();
        GPRReg gprResult = result.gpr();

        if (nodeCanTruncateInteger(node->arithNodeFlags())) {
            if (gpr1 == gprResult)
                m_jit.add32(gpr2, gprResult);
            else {
                m_jit.move(gpr2, gprResult);
                m_jit.add32(gpr1, gprResult);
            }
        } else {
            MacroAssembler::Jump check = m_jit.branchAdd32(MacroAssembler::Overflow, gpr1, gpr2, gprResult);
                
            if (gpr1 == gprResult)
                speculationCheck(Overflow, JSValueRegs(), 0, check, SpeculationRecovery(SpeculativeAdd, gprResult, gpr2));
            else if (gpr2 == gprResult)
                speculationCheck(Overflow, JSValueRegs(), 0, check, SpeculationRecovery(SpeculativeAdd, gprResult, gpr1));
            else
                speculationCheck(Overflow, JSValueRegs(), 0, check);
        }

        integerResult(gprResult, node);
        return;
    }
    
    case NumberUse: {
        SpeculateDoubleOperand op1(this, node->child1());
        SpeculateDoubleOperand op2(this, node->child2());
        FPRTemporary result(this, op1, op2);

        FPRReg reg1 = op1.fpr();
        FPRReg reg2 = op2.fpr();
        m_jit.addDouble(reg1, reg2, result.fpr());

        doubleResult(result.fpr(), node);
        return;
    }
        
    case UntypedUse: {
        RELEASE_ASSERT(node->op() == ValueAdd);
        compileValueAdd(node);
        return;
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
}

void SpeculativeJIT::compileMakeRope(Node* node)
{
    ASSERT(node->child1().useKind() == KnownStringUse);
    ASSERT(node->child2().useKind() == KnownStringUse);
    ASSERT(!node->child3() || node->child3().useKind() == KnownStringUse);
    
    SpeculateCellOperand op1(this, node->child1());
    SpeculateCellOperand op2(this, node->child2());
    SpeculateCellOperand op3(this, node->child3());
    GPRTemporary result(this);
    GPRTemporary allocator(this);
    GPRTemporary scratch(this);
    
    GPRReg opGPRs[3];
    unsigned numOpGPRs;
    opGPRs[0] = op1.gpr();
    opGPRs[1] = op2.gpr();
    if (node->child3()) {
        opGPRs[2] = op3.gpr();
        numOpGPRs = 3;
    } else {
        opGPRs[2] = InvalidGPRReg;
        numOpGPRs = 2;
    }
    GPRReg resultGPR = result.gpr();
    GPRReg allocatorGPR = allocator.gpr();
    GPRReg scratchGPR = scratch.gpr();
    
    JITCompiler::JumpList slowPath;
    MarkedAllocator& markedAllocator = m_jit.vm()->heap.allocatorForObjectWithImmortalStructureDestructor(sizeof(JSRopeString));
    m_jit.move(TrustedImmPtr(&markedAllocator), allocatorGPR);
    emitAllocateJSCell(resultGPR, allocatorGPR, TrustedImmPtr(m_jit.vm()->stringStructure.get()), scratchGPR, slowPath);
        
    m_jit.storePtr(TrustedImmPtr(0), JITCompiler::Address(resultGPR, JSString::offsetOfValue()));
    for (unsigned i = 0; i < numOpGPRs; ++i)
        m_jit.storePtr(opGPRs[i], JITCompiler::Address(resultGPR, JSRopeString::offsetOfFibers() + sizeof(WriteBarrier<JSString>) * i));
    for (unsigned i = numOpGPRs; i < JSRopeString::s_maxInternalRopeLength; ++i)
        m_jit.storePtr(TrustedImmPtr(0), JITCompiler::Address(resultGPR, JSRopeString::offsetOfFibers() + sizeof(WriteBarrier<JSString>) * i));
    m_jit.load32(JITCompiler::Address(opGPRs[0], JSString::offsetOfFlags()), scratchGPR);
    m_jit.load32(JITCompiler::Address(opGPRs[0], JSString::offsetOfLength()), allocatorGPR);
    for (unsigned i = 1; i < numOpGPRs; ++i) {
        m_jit.and32(JITCompiler::Address(opGPRs[i], JSString::offsetOfFlags()), scratchGPR);
        m_jit.add32(JITCompiler::Address(opGPRs[i], JSString::offsetOfLength()), allocatorGPR);
    }
    m_jit.and32(JITCompiler::TrustedImm32(JSString::Is8Bit), scratchGPR);
    m_jit.store32(scratchGPR, JITCompiler::Address(resultGPR, JSString::offsetOfFlags()));
    m_jit.store32(allocatorGPR, JITCompiler::Address(resultGPR, JSString::offsetOfLength()));
    
    switch (numOpGPRs) {
    case 2:
        addSlowPathGenerator(slowPathCall(
            slowPath, this, operationMakeRope2, resultGPR, opGPRs[0], opGPRs[1]));
        break;
    case 3:
        addSlowPathGenerator(slowPathCall(
            slowPath, this, operationMakeRope3, resultGPR, opGPRs[0], opGPRs[1], opGPRs[2]));
        break;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
        
    cellResult(resultGPR, node);
}

void SpeculativeJIT::compileArithSub(Node* node)
{
    switch (node->binaryUseKind()) {
    case Int32Use: {
        if (isNumberConstant(node->child2().node())) {
            SpeculateIntegerOperand op1(this, node->child1());
            int32_t imm2 = valueOfInt32Constant(node->child2().node());
            GPRTemporary result(this);

            if (nodeCanTruncateInteger(node->arithNodeFlags())) {
                m_jit.move(op1.gpr(), result.gpr());
                m_jit.sub32(Imm32(imm2), result.gpr());
            } else {
#if ENABLE(JIT_CONSTANT_BLINDING)
                GPRTemporary scratch(this);
                speculationCheck(Overflow, JSValueRegs(), 0, m_jit.branchSub32(MacroAssembler::Overflow, op1.gpr(), Imm32(imm2), result.gpr(), scratch.gpr()));
#else
                speculationCheck(Overflow, JSValueRegs(), 0, m_jit.branchSub32(MacroAssembler::Overflow, op1.gpr(), Imm32(imm2), result.gpr()));
#endif
            }

            integerResult(result.gpr(), node);
            return;
        }
            
        if (isNumberConstant(node->child1().node())) {
            int32_t imm1 = valueOfInt32Constant(node->child1().node());
            SpeculateIntegerOperand op2(this, node->child2());
            GPRTemporary result(this);
                
            m_jit.move(Imm32(imm1), result.gpr());
            if (nodeCanTruncateInteger(node->arithNodeFlags()))
                m_jit.sub32(op2.gpr(), result.gpr());
            else
                speculationCheck(Overflow, JSValueRegs(), 0, m_jit.branchSub32(MacroAssembler::Overflow, op2.gpr(), result.gpr()));
                
            integerResult(result.gpr(), node);
            return;
        }
            
        SpeculateIntegerOperand op1(this, node->child1());
        SpeculateIntegerOperand op2(this, node->child2());
        GPRTemporary result(this);

        if (nodeCanTruncateInteger(node->arithNodeFlags())) {
            m_jit.move(op1.gpr(), result.gpr());
            m_jit.sub32(op2.gpr(), result.gpr());
        } else
            speculationCheck(Overflow, JSValueRegs(), 0, m_jit.branchSub32(MacroAssembler::Overflow, op1.gpr(), op2.gpr(), result.gpr()));

        integerResult(result.gpr(), node);
        return;
    }
        
    case NumberUse: {
        SpeculateDoubleOperand op1(this, node->child1());
        SpeculateDoubleOperand op2(this, node->child2());
        FPRTemporary result(this, op1);

        FPRReg reg1 = op1.fpr();
        FPRReg reg2 = op2.fpr();
        m_jit.subDouble(reg1, reg2, result.fpr());

        doubleResult(result.fpr(), node);
        return;
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return;
    }
}

void SpeculativeJIT::compileArithNegate(Node* node)
{
    switch (node->child1().useKind()) {
    case Int32Use: {
        SpeculateIntegerOperand op1(this, node->child1());
        GPRTemporary result(this);

        m_jit.move(op1.gpr(), result.gpr());

        if (nodeCanTruncateInteger(node->arithNodeFlags()))
            m_jit.neg32(result.gpr());
        else {
            speculationCheck(Overflow, JSValueRegs(), 0, m_jit.branchNeg32(MacroAssembler::Overflow, result.gpr()));
            if (!nodeCanIgnoreNegativeZero(node->arithNodeFlags()))
                speculationCheck(NegativeZero, JSValueRegs(), 0, m_jit.branchTest32(MacroAssembler::Zero, result.gpr()));
        }

        integerResult(result.gpr(), node);
        return;
    }
        
    case NumberUse: {
        SpeculateDoubleOperand op1(this, node->child1());
        FPRTemporary result(this);
        
        m_jit.negateDouble(op1.fpr(), result.fpr());
        
        doubleResult(result.fpr(), node);
        return;
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return;
    }
}
void SpeculativeJIT::compileArithIMul(Node* node)
{
    SpeculateIntegerOperand op1(this, node->child1());
    SpeculateIntegerOperand op2(this, node->child2());
    GPRTemporary result(this);

    GPRReg reg1 = op1.gpr();
    GPRReg reg2 = op2.gpr();

    m_jit.move(reg1, result.gpr());
    m_jit.mul32(reg2, result.gpr());
    integerResult(result.gpr(), node);
    return;
}

void SpeculativeJIT::compileArithMul(Node* node)
{
    switch (node->binaryUseKind()) {
    case Int32Use: {
        SpeculateIntegerOperand op1(this, node->child1());
        SpeculateIntegerOperand op2(this, node->child2());
        GPRTemporary result(this);

        GPRReg reg1 = op1.gpr();
        GPRReg reg2 = op2.gpr();

        // We can perform truncated multiplications if we get to this point, because if the
        // fixup phase could not prove that it would be safe, it would have turned us into
        // a double multiplication.
        if (nodeCanTruncateInteger(node->arithNodeFlags())) {
            m_jit.move(reg1, result.gpr());
            m_jit.mul32(reg2, result.gpr());
        } else {
            speculationCheck(
                Overflow, JSValueRegs(), 0,
                m_jit.branchMul32(MacroAssembler::Overflow, reg1, reg2, result.gpr()));
        }
            
        // Check for negative zero, if the users of this node care about such things.
        if (!nodeCanIgnoreNegativeZero(node->arithNodeFlags())) {
            MacroAssembler::Jump resultNonZero = m_jit.branchTest32(MacroAssembler::NonZero, result.gpr());
            speculationCheck(NegativeZero, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::LessThan, reg1, TrustedImm32(0)));
            speculationCheck(NegativeZero, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::LessThan, reg2, TrustedImm32(0)));
            resultNonZero.link(&m_jit);
        }

        integerResult(result.gpr(), node);
        return;
    }
        
    case NumberUse: {
        SpeculateDoubleOperand op1(this, node->child1());
        SpeculateDoubleOperand op2(this, node->child2());
        FPRTemporary result(this, op1, op2);
        
        FPRReg reg1 = op1.fpr();
        FPRReg reg2 = op2.fpr();
        
        m_jit.mulDouble(reg1, reg2, result.fpr());
        
        doubleResult(result.fpr(), node);
        return;
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return;
    }
}

#if CPU(X86) || CPU(X86_64)
void SpeculativeJIT::compileIntegerArithDivForX86(Node* node)
{
    SpeculateIntegerOperand op1(this, node->child1());
    SpeculateIntegerOperand op2(this, node->child2());
    GPRTemporary eax(this, X86Registers::eax);
    GPRTemporary edx(this, X86Registers::edx);
    GPRReg op1GPR = op1.gpr();
    GPRReg op2GPR = op2.gpr();
    
    GPRReg op2TempGPR;
    GPRReg temp;
    if (op2GPR == X86Registers::eax || op2GPR == X86Registers::edx) {
        op2TempGPR = allocate();
        temp = op2TempGPR;
    } else {
        op2TempGPR = InvalidGPRReg;
        if (op1GPR == X86Registers::eax)
            temp = X86Registers::edx;
        else
            temp = X86Registers::eax;
    }
    
    ASSERT(temp != op1GPR);
    ASSERT(temp != op2GPR);
    
    m_jit.add32(JITCompiler::TrustedImm32(1), op2GPR, temp);
    
    JITCompiler::Jump safeDenominator = m_jit.branch32(JITCompiler::Above, temp, JITCompiler::TrustedImm32(1));
    
    JITCompiler::JumpList done;
    if (nodeUsedAsNumber(node->arithNodeFlags())) {
        speculationCheck(Overflow, JSValueRegs(), 0, m_jit.branchTest32(JITCompiler::Zero, op2GPR));
        speculationCheck(Overflow, JSValueRegs(), 0, m_jit.branch32(JITCompiler::Equal, op1GPR, TrustedImm32(-2147483647-1)));
    } else {
        // This is the case where we convert the result to an int after we're done, and we
        // already know that the denominator is either -1 or 0. So, if the denominator is
        // zero, then the result should be zero. If the denominator is not zero (i.e. it's
        // -1) and the numerator is -2^31 then the result should be -2^31. Otherwise we
        // are happy to fall through to a normal division, since we're just dividing
        // something by negative 1.
        
        JITCompiler::Jump notZero = m_jit.branchTest32(JITCompiler::NonZero, op2GPR);
        m_jit.move(TrustedImm32(0), eax.gpr());
        done.append(m_jit.jump());
        
        notZero.link(&m_jit);
        JITCompiler::Jump notNeg2ToThe31 =
            m_jit.branch32(JITCompiler::NotEqual, op1GPR, TrustedImm32(-2147483647-1));
        m_jit.move(op1GPR, eax.gpr());
        done.append(m_jit.jump());
        
        notNeg2ToThe31.link(&m_jit);
    }
    
    safeDenominator.link(&m_jit);
    
    // If the user cares about negative zero, then speculate that we're not about
    // to produce negative zero.
    if (!nodeCanIgnoreNegativeZero(node->arithNodeFlags())) {
        MacroAssembler::Jump numeratorNonZero = m_jit.branchTest32(MacroAssembler::NonZero, op1GPR);
        speculationCheck(NegativeZero, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::LessThan, op2GPR, TrustedImm32(0)));
        numeratorNonZero.link(&m_jit);
    }
    
    if (op2TempGPR != InvalidGPRReg) {
        m_jit.move(op2GPR, op2TempGPR);
        op2GPR = op2TempGPR;
    }
            
    m_jit.move(op1GPR, eax.gpr());
    m_jit.assembler().cdq();
    m_jit.assembler().idivl_r(op2GPR);
            
    if (op2TempGPR != InvalidGPRReg)
        unlock(op2TempGPR);

    // Check that there was no remainder. If there had been, then we'd be obligated to
    // produce a double result instead.
    if (nodeUsedAsNumber(node->arithNodeFlags()))
        speculationCheck(Overflow, JSValueRegs(), 0, m_jit.branchTest32(JITCompiler::NonZero, edx.gpr()));
    else
        done.link(&m_jit);
            
    integerResult(eax.gpr(), node);
}
#elif CPU(APPLE_ARMV7S)
void SpeculativeJIT::compileIntegerArithDivForARMv7s(Node* node)
{
    SpeculateIntegerOperand op1(this, node->child1());
    SpeculateIntegerOperand op2(this, node->child2());
    GPRReg op1GPR = op1.gpr();
    GPRReg op2GPR = op2.gpr();
    GPRTemporary quotient(this);
    GPRTemporary multiplyAnswer(this);

    // If the user cares about negative zero, then speculate that we're not about
    // to produce negative zero.
    if (!nodeCanIgnoreNegativeZero(node->arithNodeFlags())) {
        MacroAssembler::Jump numeratorNonZero = m_jit.branchTest32(MacroAssembler::NonZero, op1GPR);
        speculationCheck(NegativeZero, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::LessThan, op2GPR, TrustedImm32(0)));
        numeratorNonZero.link(&m_jit);
    }

    m_jit.assembler().sdiv(quotient.gpr(), op1GPR, op2GPR);

    // Check that there was no remainder. If there had been, then we'd be obligated to
    // produce a double result instead.
    if (nodeUsedAsNumber(node->arithNodeFlags())) {
        speculationCheck(Overflow, JSValueRegs(), 0, m_jit.branchMul32(JITCompiler::Overflow, quotient.gpr(), op2GPR, multiplyAnswer.gpr()));
        speculationCheck(Overflow, JSValueRegs(), 0, m_jit.branch32(JITCompiler::NotEqual, multiplyAnswer.gpr(), op1GPR));
    }

    integerResult(quotient.gpr(), node);
}
#endif

void SpeculativeJIT::compileArithMod(Node* node)
{
    switch (node->binaryUseKind()) {
    case Int32Use: {
        compileSoftModulo(node);
        return;
    }
        
    case NumberUse: {
        SpeculateDoubleOperand op1(this, node->child1());
        SpeculateDoubleOperand op2(this, node->child2());
        
        FPRReg op1FPR = op1.fpr();
        FPRReg op2FPR = op2.fpr();
        
        flushRegisters();
        
        FPRResult result(this);
        
        callOperation(fmodAsDFGOperation, result.fpr(), op1FPR, op2FPR);
        
        doubleResult(result.fpr(), node);
        return;
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return;
    }
}

// Returns true if the compare is fused with a subsequent branch.
bool SpeculativeJIT::compare(Node* node, MacroAssembler::RelationalCondition condition, MacroAssembler::DoubleCondition doubleCondition, S_DFGOperation_EJJ operation)
{
    if (compilePeepHoleBranch(node, condition, doubleCondition, operation))
        return true;

    if (node->isBinaryUseKind(Int32Use)) {
        compileIntegerCompare(node, condition);
        return false;
    }

    if (node->isBinaryUseKind(NumberUse)) {
        compileDoubleCompare(node, doubleCondition);
        return false;
    }
    
    if (node->op() == CompareEq) {
        if (node->isBinaryUseKind(StringUse)) {
            compileStringEquality(node);
            return false;
        }
        
        if (node->isBinaryUseKind(BooleanUse)) {
            compileBooleanCompare(node, condition);
            return false;
        }

        if (node->isBinaryUseKind(ObjectUse)) {
            compileObjectEquality(node);
            return false;
        }
        
        if (node->child1().useKind() == ObjectUse && node->child2().useKind() == ObjectOrOtherUse) {
            compileObjectToObjectOrOtherEquality(node->child1(), node->child2());
            return false;
        }
        
        if (node->child1().useKind() == ObjectOrOtherUse && node->child2().useKind() == ObjectUse) {
            compileObjectToObjectOrOtherEquality(node->child2(), node->child1());
            return false;
        }
    }
    
    nonSpeculativeNonPeepholeCompare(node, condition, operation);
    return false;
}

bool SpeculativeJIT::compileStrictEqForConstant(Node* node, Edge value, JSValue constant)
{
    JSValueOperand op1(this, value);
    
    // FIXME: This code is wrong for the case that the constant is null or undefined,
    // and the value is an object that MasqueradesAsUndefined.
    // https://bugs.webkit.org/show_bug.cgi?id=109487
    
    unsigned branchIndexInBlock = detectPeepHoleBranch();
    if (branchIndexInBlock != UINT_MAX) {
        Node* branchNode = m_jit.graph().m_blocks[m_block]->at(branchIndexInBlock);
        BlockIndex taken = branchNode->takenBlockIndex();
        BlockIndex notTaken = branchNode->notTakenBlockIndex();
        MacroAssembler::RelationalCondition condition = MacroAssembler::Equal;
        
        // The branch instruction will branch to the taken block.
        // If taken is next, switch taken with notTaken & invert the branch condition so we can fall through.
        if (taken == nextBlock()) {
            condition = MacroAssembler::NotEqual;
            BlockIndex tmp = taken;
            taken = notTaken;
            notTaken = tmp;
        }

#if USE(JSVALUE64)
        branch64(condition, op1.gpr(), MacroAssembler::TrustedImm64(JSValue::encode(constant)), taken);
#else
        GPRReg payloadGPR = op1.payloadGPR();
        GPRReg tagGPR = op1.tagGPR();
        if (condition == MacroAssembler::Equal) {
            // Drop down if not equal, go elsewhere if equal.
            MacroAssembler::Jump notEqual = m_jit.branch32(MacroAssembler::NotEqual, tagGPR, MacroAssembler::Imm32(constant.tag()));
            branch32(MacroAssembler::Equal, payloadGPR, MacroAssembler::Imm32(constant.payload()), taken);
            notEqual.link(&m_jit);
        } else {
            // Drop down if equal, go elsehwere if not equal.
            branch32(MacroAssembler::NotEqual, tagGPR, MacroAssembler::Imm32(constant.tag()), taken);
            branch32(MacroAssembler::NotEqual, payloadGPR, MacroAssembler::Imm32(constant.payload()), taken);
        }
#endif
        
        jump(notTaken);
        
        use(node->child1());
        use(node->child2());
        m_indexInBlock = branchIndexInBlock;
        m_currentNode = branchNode;
        return true;
    }
    
    GPRTemporary result(this);
    
#if USE(JSVALUE64)
    GPRReg op1GPR = op1.gpr();
    GPRReg resultGPR = result.gpr();
    m_jit.move(MacroAssembler::TrustedImm64(ValueFalse), resultGPR);
    MacroAssembler::Jump notEqual = m_jit.branch64(MacroAssembler::NotEqual, op1GPR, MacroAssembler::TrustedImm64(JSValue::encode(constant)));
    m_jit.or32(MacroAssembler::TrustedImm32(1), resultGPR);
    notEqual.link(&m_jit);
    jsValueResult(resultGPR, node, DataFormatJSBoolean);
#else
    GPRReg op1PayloadGPR = op1.payloadGPR();
    GPRReg op1TagGPR = op1.tagGPR();
    GPRReg resultGPR = result.gpr();
    m_jit.move(TrustedImm32(0), resultGPR);
    MacroAssembler::JumpList notEqual;
    notEqual.append(m_jit.branch32(MacroAssembler::NotEqual, op1TagGPR, MacroAssembler::Imm32(constant.tag())));
    notEqual.append(m_jit.branch32(MacroAssembler::NotEqual, op1PayloadGPR, MacroAssembler::Imm32(constant.payload())));
    m_jit.move(TrustedImm32(1), resultGPR);
    notEqual.link(&m_jit);
    booleanResult(resultGPR, node);
#endif
    
    return false;
}

bool SpeculativeJIT::compileStrictEq(Node* node)
{
    switch (node->binaryUseKind()) {
    case BooleanUse: {
        unsigned branchIndexInBlock = detectPeepHoleBranch();
        if (branchIndexInBlock != UINT_MAX) {
            Node* branchNode = m_jit.graph().m_blocks[m_block]->at(branchIndexInBlock);
            compilePeepHoleBooleanBranch(node, branchNode, MacroAssembler::Equal);
            use(node->child1());
            use(node->child2());
            m_indexInBlock = branchIndexInBlock;
            m_currentNode = branchNode;
            return true;
        }
        compileBooleanCompare(node, MacroAssembler::Equal);
        return false;
    }

    case Int32Use: {
        unsigned branchIndexInBlock = detectPeepHoleBranch();
        if (branchIndexInBlock != UINT_MAX) {
            Node* branchNode = m_jit.graph().m_blocks[m_block]->at(branchIndexInBlock);
            compilePeepHoleIntegerBranch(node, branchNode, MacroAssembler::Equal);
            use(node->child1());
            use(node->child2());
            m_indexInBlock = branchIndexInBlock;
            m_currentNode = branchNode;
            return true;
        }
        compileIntegerCompare(node, MacroAssembler::Equal);
        return false;
    }
        
    case NumberUse: {
        unsigned branchIndexInBlock = detectPeepHoleBranch();
        if (branchIndexInBlock != UINT_MAX) {
            Node* branchNode = m_jit.graph().m_blocks[m_block]->at(branchIndexInBlock);
            compilePeepHoleDoubleBranch(node, branchNode, MacroAssembler::DoubleEqual);
            use(node->child1());
            use(node->child2());
            m_indexInBlock = branchIndexInBlock;
            m_currentNode = branchNode;
            return true;
        }
        compileDoubleCompare(node, MacroAssembler::DoubleEqual);
        return false;
    }
        
    case StringUse: {
        compileStringEquality(node);
        return false;
    }
        
    case ObjectUse: {
        unsigned branchIndexInBlock = detectPeepHoleBranch();
        if (branchIndexInBlock != UINT_MAX) {
            Node* branchNode = m_jit.graph().m_blocks[m_block]->at(branchIndexInBlock);
            compilePeepHoleObjectEquality(node, branchNode);
            use(node->child1());
            use(node->child2());
            m_indexInBlock = branchIndexInBlock;
            m_currentNode = branchNode;
            return true;
        }
        compileObjectEquality(node);
        return false;
    }
        
    case UntypedUse: {
        return nonSpeculativeStrictEq(node);
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return false;
    }
}

void SpeculativeJIT::compileBooleanCompare(Node* node, MacroAssembler::RelationalCondition condition)
{
    SpeculateBooleanOperand op1(this, node->child1());
    SpeculateBooleanOperand op2(this, node->child2());
    GPRTemporary result(this);
    
    m_jit.compare32(condition, op1.gpr(), op2.gpr(), result.gpr());
    
    // If we add a DataFormatBool, we should use it here.
#if USE(JSVALUE32_64)
    booleanResult(result.gpr(), node);
#else
    m_jit.or32(TrustedImm32(ValueFalse), result.gpr());
    jsValueResult(result.gpr(), m_currentNode, DataFormatJSBoolean);
#endif
}

void SpeculativeJIT::compileStringEquality(Node* node)
{
    SpeculateCellOperand left(this, node->child1());
    SpeculateCellOperand right(this, node->child2());
    GPRTemporary length(this);
    GPRTemporary leftTemp(this);
    GPRTemporary rightTemp(this);
    GPRTemporary leftTemp2(this, left);
    GPRTemporary rightTemp2(this, right);
    
    GPRReg leftGPR = left.gpr();
    GPRReg rightGPR = right.gpr();
    GPRReg lengthGPR = length.gpr();
    GPRReg leftTempGPR = leftTemp.gpr();
    GPRReg rightTempGPR = rightTemp.gpr();
    GPRReg leftTemp2GPR = leftTemp2.gpr();
    GPRReg rightTemp2GPR = rightTemp2.gpr();
    
    JITCompiler::JumpList trueCase;
    JITCompiler::JumpList falseCase;
    JITCompiler::JumpList slowCase;
    
    DFG_TYPE_CHECK(
        JSValueSource::unboxedCell(leftGPR), node->child1(), SpecString, m_jit.branchPtr(
            MacroAssembler::NotEqual,
            MacroAssembler::Address(leftGPR, JSCell::structureOffset()),
            MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
    
    // It's safe to branch around the type check below, since proving that the values are
    // equal does indeed prove that the right value is a string.
    trueCase.append(m_jit.branchPtr(MacroAssembler::Equal, leftGPR, rightGPR));
    
    DFG_TYPE_CHECK(
        JSValueSource::unboxedCell(rightGPR), node->child2(), SpecString, m_jit.branchPtr(
            MacroAssembler::NotEqual,
            MacroAssembler::Address(rightGPR, JSCell::structureOffset()),
            MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));

    m_jit.load32(MacroAssembler::Address(leftGPR, JSString::offsetOfLength()), lengthGPR);
    
    falseCase.append(m_jit.branch32(
        MacroAssembler::NotEqual,
        MacroAssembler::Address(rightGPR, JSString::offsetOfLength()),
        lengthGPR));
    
    trueCase.append(m_jit.branchTest32(MacroAssembler::Zero, lengthGPR));
    
    m_jit.loadPtr(MacroAssembler::Address(leftGPR, JSString::offsetOfValue()), leftTempGPR);
    m_jit.loadPtr(MacroAssembler::Address(rightGPR, JSString::offsetOfValue()), rightTempGPR);
    
    slowCase.append(m_jit.branchTestPtr(MacroAssembler::Zero, leftTempGPR));
    slowCase.append(m_jit.branchTestPtr(MacroAssembler::Zero, rightTempGPR));
    
    slowCase.append(m_jit.branchTest32(
        MacroAssembler::Zero,
        MacroAssembler::Address(leftTempGPR, StringImpl::flagsOffset()),
        TrustedImm32(StringImpl::flagIs8Bit())));
    slowCase.append(m_jit.branchTest32(
        MacroAssembler::Zero,
        MacroAssembler::Address(rightTempGPR, StringImpl::flagsOffset()),
        TrustedImm32(StringImpl::flagIs8Bit())));
    
    m_jit.loadPtr(MacroAssembler::Address(leftTempGPR, StringImpl::dataOffset()), leftTempGPR);
    m_jit.loadPtr(MacroAssembler::Address(rightTempGPR, StringImpl::dataOffset()), rightTempGPR);
    
    MacroAssembler::Label loop = m_jit.label();
    
    m_jit.sub32(TrustedImm32(1), lengthGPR);

    // This isn't going to generate the best code on x86. But that's OK, it's still better
    // than not inlining.
    m_jit.load8(MacroAssembler::BaseIndex(leftTempGPR, lengthGPR, MacroAssembler::TimesOne), leftTemp2GPR);
    m_jit.load8(MacroAssembler::BaseIndex(rightTempGPR, lengthGPR, MacroAssembler::TimesOne), rightTemp2GPR);
    falseCase.append(m_jit.branch32(MacroAssembler::NotEqual, leftTemp2GPR, rightTemp2GPR));
    
    m_jit.branchTest32(MacroAssembler::NonZero, lengthGPR).linkTo(loop, &m_jit);
    
    trueCase.link(&m_jit);
#if USE(JSVALUE64)
    m_jit.move(TrustedImm64(ValueTrue), leftTempGPR);
#else
    m_jit.move(TrustedImm32(true), leftTempGPR);
#endif
    
    JITCompiler::Jump done = m_jit.jump();

    falseCase.link(&m_jit);
#if USE(JSVALUE64)
    m_jit.move(TrustedImm64(ValueFalse), leftTempGPR);
#else
    m_jit.move(TrustedImm32(false), leftTempGPR);
#endif
    
    done.link(&m_jit);
    addSlowPathGenerator(
        slowPathCall(
            slowCase, this, operationCompareStringEq, leftTempGPR, leftGPR, rightGPR));
    
#if USE(JSVALUE64)
    jsValueResult(leftTempGPR, node, DataFormatJSBoolean);
#else
    booleanResult(leftTempGPR, node);
#endif
}

void SpeculativeJIT::compileGetIndexedPropertyStorage(Node* node)
{
    SpeculateCellOperand base(this, node->child1());
    GPRReg baseReg = base.gpr();
    
    GPRTemporary storage(this);
    GPRReg storageReg = storage.gpr();
    
    const TypedArrayDescriptor* descriptor = typedArrayDescriptor(node->arrayMode());
    
    switch (node->arrayMode().type()) {
    case Array::String:
        m_jit.loadPtr(MacroAssembler::Address(baseReg, JSString::offsetOfValue()), storageReg);
        
        addSlowPathGenerator(
            slowPathCall(
                m_jit.branchTest32(MacroAssembler::Zero, storageReg),
                this, operationResolveRope, storageReg, baseReg));

        m_jit.loadPtr(MacroAssembler::Address(storageReg, StringImpl::dataOffset()), storageReg);
        break;
        
    default:
        ASSERT(descriptor);
        m_jit.loadPtr(MacroAssembler::Address(baseReg, descriptor->m_storageOffset), storageReg);
        break;
    }
    
    storageResult(storageReg, node);
}

void SpeculativeJIT::compileGetByValOnArguments(Node* node)
{
    SpeculateCellOperand base(this, node->child1());
    SpeculateStrictInt32Operand property(this, node->child2());
    GPRTemporary result(this);
#if USE(JSVALUE32_64)
    GPRTemporary resultTag(this);
#endif
    GPRTemporary scratch(this);
    
    GPRReg baseReg = base.gpr();
    GPRReg propertyReg = property.gpr();
    GPRReg resultReg = result.gpr();
#if USE(JSVALUE32_64)
    GPRReg resultTagReg = resultTag.gpr();
#endif
    GPRReg scratchReg = scratch.gpr();
    
    if (!m_compileOkay)
        return;
  
    ASSERT(ArrayMode(Array::Arguments).alreadyChecked(m_jit.graph(), node, m_state.forNode(node->child1())));
    
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
    
    m_jit.move(propertyReg, resultReg);
    m_jit.neg32(resultReg);
    m_jit.signExtend32ToPtr(resultReg, resultReg);
    m_jit.loadPtr(
        MacroAssembler::Address(baseReg, OBJECT_OFFSETOF(Arguments, m_registers)),
        scratchReg);
    
#if USE(JSVALUE32_64)
    m_jit.load32(
        MacroAssembler::BaseIndex(
            scratchReg, resultReg, MacroAssembler::TimesEight,
            CallFrame::thisArgumentOffset() * sizeof(Register) - sizeof(Register) +
            OBJECT_OFFSETOF(JSValue, u.asBits.tag)),
        resultTagReg);
    m_jit.load32(
        MacroAssembler::BaseIndex(
            scratchReg, resultReg, MacroAssembler::TimesEight,
            CallFrame::thisArgumentOffset() * sizeof(Register) - sizeof(Register) +
            OBJECT_OFFSETOF(JSValue, u.asBits.payload)),
        resultReg);
    jsValueResult(resultTagReg, resultReg, node);
#else
    m_jit.load64(
        MacroAssembler::BaseIndex(
            scratchReg, resultReg, MacroAssembler::TimesEight,
            CallFrame::thisArgumentOffset() * sizeof(Register) - sizeof(Register)),
        resultReg);
    jsValueResult(resultReg, node);
#endif
}

void SpeculativeJIT::compileGetArgumentsLength(Node* node)
{
    SpeculateCellOperand base(this, node->child1());
    GPRTemporary result(this, base);
    
    GPRReg baseReg = base.gpr();
    GPRReg resultReg = result.gpr();
    
    if (!m_compileOkay)
        return;
    
    ASSERT(ArrayMode(Array::Arguments).alreadyChecked(m_jit.graph(), node, m_state.forNode(node->child1())));
    
    speculationCheck(
        Uncountable, JSValueSource(), 0,
        m_jit.branchTest8(
            MacroAssembler::NonZero,
            MacroAssembler::Address(baseReg, OBJECT_OFFSETOF(Arguments, m_overrodeLength))));
    
    m_jit.load32(
        MacroAssembler::Address(baseReg, OBJECT_OFFSETOF(Arguments, m_numArguments)),
        resultReg);
    integerResult(resultReg, node);
}

void SpeculativeJIT::compileGetArrayLength(Node* node)
{
    const TypedArrayDescriptor* descriptor = typedArrayDescriptor(node->arrayMode());

    switch (node->arrayMode().type()) {
    case Array::Int32:
    case Array::Double:
    case Array::Contiguous: {
        StorageOperand storage(this, node->child2());
        GPRTemporary result(this, storage);
        GPRReg storageReg = storage.gpr();
        GPRReg resultReg = result.gpr();
        m_jit.load32(MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength()), resultReg);
            
        integerResult(resultReg, node);
        break;
    }
    case Array::ArrayStorage:
    case Array::SlowPutArrayStorage: {
        StorageOperand storage(this, node->child2());
        GPRTemporary result(this, storage);
        GPRReg storageReg = storage.gpr();
        GPRReg resultReg = result.gpr();
        m_jit.load32(MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength()), resultReg);
            
        speculationCheck(Uncountable, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::LessThan, resultReg, MacroAssembler::TrustedImm32(0)));
            
        integerResult(resultReg, node);
        break;
    }
    case Array::String: {
        SpeculateCellOperand base(this, node->child1());
        GPRTemporary result(this, base);
        GPRReg baseGPR = base.gpr();
        GPRReg resultGPR = result.gpr();
        m_jit.load32(MacroAssembler::Address(baseGPR, JSString::offsetOfLength()), resultGPR);
        integerResult(resultGPR, node);
        break;
    }
    case Array::Arguments: {
        compileGetArgumentsLength(node);
        break;
    }
    default:
        SpeculateCellOperand base(this, node->child1());
        GPRTemporary result(this, base);
        GPRReg baseGPR = base.gpr();
        GPRReg resultGPR = result.gpr();
        ASSERT(descriptor);
        m_jit.load32(MacroAssembler::Address(baseGPR, descriptor->m_lengthOffset), resultGPR);
        integerResult(resultGPR, node);
        break;
    }
}

void SpeculativeJIT::compileNewFunctionNoCheck(Node* node)
{
    GPRResult result(this);
    GPRReg resultGPR = result.gpr();
    flushRegisters();
    callOperation(
        operationNewFunctionNoCheck, resultGPR, m_jit.codeBlock()->functionDecl(node->functionDeclIndex()));
    cellResult(resultGPR, node);
}

void SpeculativeJIT::compileNewFunctionExpression(Node* node)
{
    GPRResult result(this);
    GPRReg resultGPR = result.gpr();
    flushRegisters();
    callOperation(
        operationNewFunctionExpression,
        resultGPR,
        m_jit.codeBlock()->functionExpr(node->functionExprIndex()));
    cellResult(resultGPR, node);
}

bool SpeculativeJIT::compileRegExpExec(Node* node)
{
    unsigned branchIndexInBlock = detectPeepHoleBranch();
    if (branchIndexInBlock == UINT_MAX)
        return false;
    Node* branchNode = m_jit.graph().m_blocks[m_block]->at(branchIndexInBlock);
    ASSERT(node->adjustedRefCount() == 1);

    BlockIndex taken = branchNode->takenBlockIndex();
    BlockIndex notTaken = branchNode->notTakenBlockIndex();
    
    bool invert = false;
    if (taken == nextBlock()) {
        invert = true;
        BlockIndex tmp = taken;
        taken = notTaken;
        notTaken = tmp;
    }

    SpeculateCellOperand base(this, node->child1());
    SpeculateCellOperand argument(this, node->child2());
    GPRReg baseGPR = base.gpr();
    GPRReg argumentGPR = argument.gpr();
    
    flushRegisters();
    GPRResult result(this);
    callOperation(operationRegExpTest, result.gpr(), baseGPR, argumentGPR);

    branchTest32(invert ? JITCompiler::Zero : JITCompiler::NonZero, result.gpr(), taken);
    jump(notTaken);

    use(node->child1());
    use(node->child2());
    m_indexInBlock = branchIndexInBlock;
    m_currentNode = branchNode;

    return true;
}

void SpeculativeJIT::compileAllocatePropertyStorage(Node* node)
{
    if (hasIndexingHeader(node->structureTransitionData().previousStructure->indexingType())) {
        SpeculateCellOperand base(this, node->child1());
        
        GPRReg baseGPR = base.gpr();
        
        flushRegisters();

        GPRResult result(this);
        callOperation(operationReallocateButterflyToHavePropertyStorageWithInitialCapacity, result.gpr(), baseGPR);
        
        storageResult(result.gpr(), node);
        return;
    }
    
    SpeculateCellOperand base(this, node->child1());
    GPRTemporary scratch(this);
        
    GPRReg baseGPR = base.gpr();
    GPRReg scratchGPR = scratch.gpr();
        
    ASSERT(!node->structureTransitionData().previousStructure->outOfLineCapacity());
    ASSERT(initialOutOfLineCapacity == node->structureTransitionData().newStructure->outOfLineCapacity());
    
    JITCompiler::Jump slowPath =
        emitAllocateBasicStorage(
            TrustedImm32(initialOutOfLineCapacity * sizeof(JSValue)), scratchGPR);

    m_jit.addPtr(JITCompiler::TrustedImm32(sizeof(JSValue)), scratchGPR);
        
    addSlowPathGenerator(
        slowPathCall(slowPath, this, operationAllocatePropertyStorageWithInitialCapacity, scratchGPR));
        
    m_jit.storePtr(scratchGPR, JITCompiler::Address(baseGPR, JSObject::butterflyOffset()));
        
    storageResult(scratchGPR, node);
}

void SpeculativeJIT::compileReallocatePropertyStorage(Node* node)
{
    size_t oldSize = node->structureTransitionData().previousStructure->outOfLineCapacity() * sizeof(JSValue);
    size_t newSize = oldSize * outOfLineGrowthFactor;
    ASSERT(newSize == node->structureTransitionData().newStructure->outOfLineCapacity() * sizeof(JSValue));

    if (hasIndexingHeader(node->structureTransitionData().previousStructure->indexingType())) {
        SpeculateCellOperand base(this, node->child1());
        
        GPRReg baseGPR = base.gpr();
        
        flushRegisters();

        GPRResult result(this);
        callOperation(operationReallocateButterflyToGrowPropertyStorage, result.gpr(), baseGPR, newSize / sizeof(JSValue));
        
        storageResult(result.gpr(), node);
        return;
    }
    
    SpeculateCellOperand base(this, node->child1());
    StorageOperand oldStorage(this, node->child2());
    GPRTemporary scratch1(this);
    GPRTemporary scratch2(this);
        
    GPRReg baseGPR = base.gpr();
    GPRReg oldStorageGPR = oldStorage.gpr();
    GPRReg scratchGPR1 = scratch1.gpr();
    GPRReg scratchGPR2 = scratch2.gpr();
        
    JITCompiler::Jump slowPath =
        emitAllocateBasicStorage(TrustedImm32(newSize), scratchGPR2);

    m_jit.addPtr(JITCompiler::TrustedImm32(sizeof(JSValue)), scratchGPR2);
        
    addSlowPathGenerator(
        slowPathCall(slowPath, this, operationAllocatePropertyStorage, scratchGPR2, newSize / sizeof(JSValue)));
    // We have scratchGPR2 = new storage, scratchGPR1 = scratch
    for (ptrdiff_t offset = 0; offset < static_cast<ptrdiff_t>(oldSize); offset += sizeof(void*)) {
        m_jit.loadPtr(JITCompiler::Address(oldStorageGPR, -(offset + sizeof(JSValue) + sizeof(void*))), scratchGPR1);
        m_jit.storePtr(scratchGPR1, JITCompiler::Address(scratchGPR2, -(offset + sizeof(JSValue) + sizeof(void*))));
    }
    m_jit.storePtr(scratchGPR2, JITCompiler::Address(baseGPR, JSObject::butterflyOffset()));
    
    storageResult(scratchGPR2, node);
}

GPRReg SpeculativeJIT::temporaryRegisterForPutByVal(GPRTemporary& temporary, ArrayMode arrayMode)
{
    if (!putByValWillNeedExtraRegister(arrayMode))
        return InvalidGPRReg;
    
    GPRTemporary realTemporary(this);
    temporary.adopt(realTemporary);
    return temporary.gpr();
}

void SpeculativeJIT::compileToStringOnCell(Node* node)
{
    SpeculateCellOperand op1(this, node->child1());
    GPRReg op1GPR = op1.gpr();
    
    switch (node->child1().useKind()) {
    case StringObjectUse: {
        GPRTemporary result(this);
        GPRReg resultGPR = result.gpr();
        
        speculateStringObject(node->child1(), op1GPR);
        m_state.forNode(node->child1()).filter(SpecStringObject);
        m_jit.loadPtr(JITCompiler::Address(op1GPR, JSWrapperObject::internalValueCellOffset()), resultGPR);
        cellResult(resultGPR, node);
        break;
    }
        
    case StringOrStringObjectUse: {
        GPRTemporary result(this);
        GPRReg resultGPR = result.gpr();
        
        m_jit.loadPtr(JITCompiler::Address(op1GPR, JSCell::structureOffset()), resultGPR);
        JITCompiler::Jump isString = m_jit.branchPtr(
            JITCompiler::Equal, resultGPR, TrustedImmPtr(m_jit.vm()->stringStructure.get()));
        
        speculateStringObjectForStructure(node->child1(), resultGPR);
        
        m_jit.loadPtr(JITCompiler::Address(op1GPR, JSWrapperObject::internalValueCellOffset()), resultGPR);
        
        JITCompiler::Jump done = m_jit.jump();
        isString.link(&m_jit);
        m_jit.move(op1GPR, resultGPR);
        done.link(&m_jit);
        
        m_state.forNode(node->child1()).filter(SpecString | SpecStringObject);
        
        cellResult(resultGPR, node);
        break;
    }
        
    case CellUse: {
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();
        
        // We flush registers instead of silent spill/fill because in this mode we
        // believe that most likely the input is not a string, and we need to take
        // slow path.
        flushRegisters();
        JITCompiler::Jump done;
        if (node->child1()->prediction() & SpecString) {
            JITCompiler::Jump needCall = m_jit.branchPtr(
                JITCompiler::NotEqual,
                JITCompiler::Address(op1GPR, JSCell::structureOffset()),
                TrustedImmPtr(m_jit.vm()->stringStructure.get()));
            m_jit.move(op1GPR, resultGPR);
            done = m_jit.jump();
            needCall.link(&m_jit);
        }
        callOperation(operationToStringOnCell, resultGPR, op1GPR);
        if (done.isSet())
            done.link(&m_jit);
        cellResult(resultGPR, node);
        break;
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}

void SpeculativeJIT::compileNewStringObject(Node* node)
{
    SpeculateCellOperand operand(this, node->child1());
    
    GPRTemporary result(this);
    GPRTemporary scratch1(this);
    GPRTemporary scratch2(this);

    GPRReg operandGPR = operand.gpr();
    GPRReg resultGPR = result.gpr();
    GPRReg scratch1GPR = scratch1.gpr();
    GPRReg scratch2GPR = scratch2.gpr();
    
    JITCompiler::JumpList slowPath;
    
    emitAllocateJSObject<StringObject>(
        resultGPR, TrustedImmPtr(node->structure()), TrustedImmPtr(0), scratch1GPR, scratch2GPR,
        slowPath);
    
    m_jit.storePtr(
        TrustedImmPtr(&StringObject::s_info),
        JITCompiler::Address(resultGPR, JSDestructibleObject::classInfoOffset()));
#if USE(JSVALUE64)
    m_jit.store64(
        operandGPR, JITCompiler::Address(resultGPR, JSWrapperObject::internalValueOffset()));
#else
    m_jit.store32(
        TrustedImm32(JSValue::CellTag),
        JITCompiler::Address(resultGPR, JSWrapperObject::internalValueOffset() + OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
    m_jit.store32(
        operandGPR,
        JITCompiler::Address(resultGPR, JSWrapperObject::internalValueOffset() + OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
#endif
    
    addSlowPathGenerator(slowPathCall(
        slowPath, this, operationNewStringObject, resultGPR, operandGPR, node->structure()));
    
    cellResult(resultGPR, node);
}

void SpeculativeJIT::speculateInt32(Edge edge)
{
    if (!needsTypeCheck(edge, SpecInt32))
        return;
    
    (SpeculateIntegerOperand(this, edge)).gpr();
}

void SpeculativeJIT::speculateNumber(Edge edge)
{
    if (!needsTypeCheck(edge, SpecNumber))
        return;
    
    (SpeculateDoubleOperand(this, edge)).fpr();
}

void SpeculativeJIT::speculateRealNumber(Edge edge)
{
    if (!needsTypeCheck(edge, SpecRealNumber))
        return;
    
    SpeculateDoubleOperand operand(this, edge);
    FPRReg fpr = operand.fpr();
    DFG_TYPE_CHECK(
        JSValueRegs(), edge, SpecRealNumber,
        m_jit.branchDouble(
            MacroAssembler::DoubleNotEqualOrUnordered, fpr, fpr));
}

void SpeculativeJIT::speculateBoolean(Edge edge)
{
    if (!needsTypeCheck(edge, SpecBoolean))
        return;
    
    (SpeculateBooleanOperand(this, edge)).gpr();
}

void SpeculativeJIT::speculateCell(Edge edge)
{
    if (!needsTypeCheck(edge, SpecCell))
        return;
    
    (SpeculateCellOperand(this, edge)).gpr();
}

void SpeculativeJIT::speculateObject(Edge edge)
{
    if (!needsTypeCheck(edge, SpecObject))
        return;
    
    SpeculateCellOperand operand(this, edge);
    GPRReg gpr = operand.gpr();
    DFG_TYPE_CHECK(
        JSValueSource::unboxedCell(gpr), edge, SpecObject, m_jit.branchPtr(
            MacroAssembler::Equal, 
            MacroAssembler::Address(gpr, JSCell::structureOffset()), 
            MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
}

void SpeculativeJIT::speculateObjectOrOther(Edge edge)
{
    if (!needsTypeCheck(edge, SpecObject | SpecOther))
        return;
    
    JSValueOperand operand(this, edge, ManualOperandSpeculation);
    GPRTemporary temp(this);
    GPRReg tempGPR = temp.gpr();
#if USE(JSVALUE64)
    GPRReg gpr = operand.gpr();
    MacroAssembler::Jump notCell = m_jit.branchTest64(
        MacroAssembler::NonZero, gpr, GPRInfo::tagMaskRegister);
    DFG_TYPE_CHECK(
        JSValueRegs(gpr), edge, (~SpecCell) | SpecObject, m_jit.branchPtr(
            MacroAssembler::Equal, 
            MacroAssembler::Address(gpr, JSCell::structureOffset()), 
            MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
    MacroAssembler::Jump done = m_jit.jump();
    notCell.link(&m_jit);
    if (needsTypeCheck(edge, SpecCell | SpecOther)) {
        m_jit.move(gpr, tempGPR);
        m_jit.and64(MacroAssembler::TrustedImm32(~TagBitUndefined), tempGPR);
        
        typeCheck(
            JSValueRegs(gpr), edge, SpecCell | SpecOther,
            m_jit.branch64(
                MacroAssembler::NotEqual, tempGPR,
                MacroAssembler::TrustedImm64(ValueNull)));
    }
    done.link(&m_jit);
#else
    GPRReg tagGPR = operand.tagGPR();
    GPRReg payloadGPR = operand.payloadGPR();
    MacroAssembler::Jump notCell =
        m_jit.branch32(MacroAssembler::NotEqual, tagGPR, TrustedImm32(JSValue::CellTag));
    DFG_TYPE_CHECK(
        JSValueRegs(tagGPR, payloadGPR), edge, (~SpecCell) | SpecObject, m_jit.branchPtr(
            MacroAssembler::Equal, 
            MacroAssembler::Address(payloadGPR, JSCell::structureOffset()), 
            MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
    MacroAssembler::Jump done = m_jit.jump();
    notCell.link(&m_jit);
    if (needsTypeCheck(edge, SpecCell | SpecOther)) {
        m_jit.move(tagGPR, tempGPR);
        m_jit.or32(TrustedImm32(1), tempGPR);
        
        typeCheck(
            JSValueRegs(tagGPR, payloadGPR), edge, SpecCell | SpecOther,
            m_jit.branch32(
                MacroAssembler::NotEqual, tempGPR,
                MacroAssembler::TrustedImm32(JSValue::NullTag)));
    }
    done.link(&m_jit);
#endif
}

void SpeculativeJIT::speculateString(Edge edge)
{
    if (!needsTypeCheck(edge, SpecString))
        return;
    
    SpeculateCellOperand operand(this, edge);
    GPRReg gpr = operand.gpr();
    DFG_TYPE_CHECK(
        JSValueSource::unboxedCell(gpr), edge, SpecString, m_jit.branchPtr(
            MacroAssembler::NotEqual, 
            MacroAssembler::Address(gpr, JSCell::structureOffset()), 
            MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
}

void SpeculativeJIT::speculateStringObject(Edge edge, GPRReg gpr)
{
    speculateStringObjectForStructure(edge, JITCompiler::Address(gpr, JSCell::structureOffset()));
}

void SpeculativeJIT::speculateStringObject(Edge edge)
{
    if (!needsTypeCheck(edge, SpecStringObject))
        return;
    
    SpeculateCellOperand operand(this, edge);
    GPRReg gpr = operand.gpr();
    if (!needsTypeCheck(edge, SpecStringObject))
        return;
    
    speculateStringObject(edge, gpr);
    m_state.forNode(edge).filter(SpecStringObject);
}

void SpeculativeJIT::speculateStringOrStringObject(Edge edge)
{
    if (!needsTypeCheck(edge, SpecString | SpecStringObject))
        return;
    
    SpeculateCellOperand operand(this, edge);
    GPRReg gpr = operand.gpr();
    if (!needsTypeCheck(edge, SpecString | SpecStringObject))
        return;
    
    GPRTemporary structure(this);
    GPRReg structureGPR = structure.gpr();
    
    m_jit.loadPtr(JITCompiler::Address(gpr, JSCell::structureOffset()), structureGPR);
    
    JITCompiler::Jump isString = m_jit.branchPtr(
        JITCompiler::Equal, structureGPR, TrustedImmPtr(m_jit.vm()->stringStructure.get()));
    
    speculateStringObjectForStructure(edge, structureGPR);
    
    isString.link(&m_jit);
    
    m_state.forNode(edge).filter(SpecString | SpecStringObject);
}

void SpeculativeJIT::speculateNotCell(Edge edge)
{
    if (!needsTypeCheck(edge, ~SpecCell))
        return;
    
    JSValueOperand operand(this, edge, ManualOperandSpeculation);
#if USE(JSVALUE64)
    typeCheck(
        JSValueRegs(operand.gpr()), edge, ~SpecCell,
        m_jit.branchTest64(
            JITCompiler::Zero, operand.gpr(), GPRInfo::tagMaskRegister));
#else
    typeCheck(
        JSValueRegs(operand.tagGPR(), operand.payloadGPR()), edge, ~SpecCell,
        m_jit.branch32(
            JITCompiler::Equal, operand.tagGPR(), TrustedImm32(JSValue::CellTag)));
#endif
}

void SpeculativeJIT::speculateOther(Edge edge)
{
    if (!needsTypeCheck(edge, SpecOther))
        return;
    
    JSValueOperand operand(this, edge, ManualOperandSpeculation);
    GPRTemporary temp(this);
    GPRReg tempGPR = temp.gpr();
#if USE(JSVALUE64)
    m_jit.move(operand.gpr(), tempGPR);
    m_jit.and64(MacroAssembler::TrustedImm32(~TagBitUndefined), tempGPR);
    typeCheck(
        JSValueRegs(operand.gpr()), edge, SpecOther,
        m_jit.branch64(
            MacroAssembler::NotEqual, tempGPR,
            MacroAssembler::TrustedImm64(ValueNull)));
#else
    m_jit.move(operand.tagGPR(), tempGPR);
    m_jit.or32(TrustedImm32(1), tempGPR);
    typeCheck(
        JSValueRegs(operand.tagGPR(), operand.payloadGPR()), edge, SpecOther,
        m_jit.branch32(MacroAssembler::NotEqual, tempGPR, TrustedImm32(JSValue::NullTag)));
#endif    
}

void SpeculativeJIT::speculate(Node*, Edge edge)
{
    switch (edge.useKind()) {
    case UntypedUse:
        break;
    case KnownInt32Use:
        ASSERT(!needsTypeCheck(edge, SpecInt32));
        break;
    case KnownNumberUse:
        ASSERT(!needsTypeCheck(edge, SpecNumber));
        break;
    case KnownCellUse:
        ASSERT(!needsTypeCheck(edge, SpecCell));
        break;
    case KnownStringUse:
        ASSERT(!needsTypeCheck(edge, SpecString));
        break;
    case Int32Use:
        speculateInt32(edge);
        break;
    case RealNumberUse:
        speculateRealNumber(edge);
        break;
    case NumberUse:
        speculateNumber(edge);
        break;
    case BooleanUse:
        speculateBoolean(edge);
        break;
    case CellUse:
        speculateCell(edge);
        break;
    case ObjectUse:
        speculateObject(edge);
        break;
    case ObjectOrOtherUse:
        speculateObjectOrOther(edge);
        break;
    case StringUse:
        speculateString(edge);
        break;
    case StringObjectUse:
        speculateStringObject(edge);
        break;
    case StringOrStringObjectUse:
        speculateStringOrStringObject(edge);
        break;
    case NotCellUse:
        speculateNotCell(edge);
        break;
    case OtherUse:
        speculateOther(edge);
        break;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
}

} } // namespace JSC::DFG

#endif
