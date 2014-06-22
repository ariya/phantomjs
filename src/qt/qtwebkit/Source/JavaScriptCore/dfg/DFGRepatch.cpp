/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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
#include "DFGRepatch.h"

#if ENABLE(DFG_JIT)

#include "DFGCCallHelpers.h"
#include "DFGScratchRegisterAllocator.h"
#include "DFGSpeculativeJIT.h"
#include "DFGThunks.h"
#include "GCAwareJITStubRoutine.h"
#include "LinkBuffer.h"
#include "Operations.h"
#include "PolymorphicPutByIdList.h"
#include "RepatchBuffer.h"
#include "StructureRareDataInlines.h"
#include <wtf/StringPrintStream.h>

namespace JSC { namespace DFG {

static void dfgRepatchCall(CodeBlock* codeblock, CodeLocationCall call, FunctionPtr newCalleeFunction)
{
    RepatchBuffer repatchBuffer(codeblock);
    repatchBuffer.relink(call, newCalleeFunction);
}

static void dfgRepatchByIdSelfAccess(CodeBlock* codeBlock, StructureStubInfo& stubInfo, Structure* structure, PropertyOffset offset, const FunctionPtr &slowPathFunction, bool compact)
{
    RepatchBuffer repatchBuffer(codeBlock);

    // Only optimize once!
    repatchBuffer.relink(stubInfo.callReturnLocation, slowPathFunction);

    // Patch the structure check & the offset of the load.
    repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabelPtrAtOffset(-(intptr_t)stubInfo.patch.dfg.deltaCheckImmToCall), structure);
    repatchBuffer.setLoadInstructionIsActive(stubInfo.callReturnLocation.convertibleLoadAtOffset(stubInfo.patch.dfg.deltaCallToStorageLoad), isOutOfLineOffset(offset));
#if USE(JSVALUE64)
    if (compact)
        repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabelCompactAtOffset(stubInfo.patch.dfg.deltaCallToLoadOrStore), offsetRelativeToPatchedStorage(offset));
    else
        repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabel32AtOffset(stubInfo.patch.dfg.deltaCallToLoadOrStore), offsetRelativeToPatchedStorage(offset));
#elif USE(JSVALUE32_64)
    if (compact) {
        repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabelCompactAtOffset(stubInfo.patch.dfg.deltaCallToTagLoadOrStore), offsetRelativeToPatchedStorage(offset) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
        repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabelCompactAtOffset(stubInfo.patch.dfg.deltaCallToPayloadLoadOrStore), offsetRelativeToPatchedStorage(offset) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
    } else {
        repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabel32AtOffset(stubInfo.patch.dfg.deltaCallToTagLoadOrStore), offsetRelativeToPatchedStorage(offset) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
        repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabel32AtOffset(stubInfo.patch.dfg.deltaCallToPayloadLoadOrStore), offsetRelativeToPatchedStorage(offset) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
    }
#endif
}

static void addStructureTransitionCheck(
    JSCell* object, Structure* structure, CodeBlock* codeBlock, StructureStubInfo& stubInfo,
    MacroAssembler& jit, MacroAssembler::JumpList& failureCases, GPRReg scratchGPR)
{
    if (object->structure() == structure && structure->transitionWatchpointSetIsStillValid()) {
        structure->addTransitionWatchpoint(stubInfo.addWatchpoint(codeBlock));
#if DFG_ENABLE(JIT_ASSERT)
        // If we execute this code, the object must have the structure we expect. Assert
        // this in debug modes.
        jit.move(MacroAssembler::TrustedImmPtr(object), scratchGPR);
        MacroAssembler::Jump ok = jit.branchPtr(
            MacroAssembler::Equal,
            MacroAssembler::Address(scratchGPR, JSCell::structureOffset()),
            MacroAssembler::TrustedImmPtr(structure));
        jit.breakpoint();
        ok.link(&jit);
#endif
        return;
    }
    
    jit.move(MacroAssembler::TrustedImmPtr(object), scratchGPR);
    failureCases.append(
        jit.branchPtr(
            MacroAssembler::NotEqual,
            MacroAssembler::Address(scratchGPR, JSCell::structureOffset()),
            MacroAssembler::TrustedImmPtr(structure)));
}

static void addStructureTransitionCheck(
    JSValue prototype, CodeBlock* codeBlock, StructureStubInfo& stubInfo,
    MacroAssembler& jit, MacroAssembler::JumpList& failureCases, GPRReg scratchGPR)
{
    if (prototype.isNull())
        return;
    
    ASSERT(prototype.isCell());
    
    addStructureTransitionCheck(
        prototype.asCell(), prototype.asCell()->structure(), codeBlock, stubInfo, jit,
        failureCases, scratchGPR);
}

static void replaceWithJump(RepatchBuffer& repatchBuffer, StructureStubInfo& stubInfo, const MacroAssemblerCodePtr target)
{
    if (MacroAssembler::canJumpReplacePatchableBranchPtrWithPatch()) {
        repatchBuffer.replaceWithJump(
            RepatchBuffer::startOfPatchableBranchPtrWithPatchOnAddress(
                stubInfo.callReturnLocation.dataLabelPtrAtOffset(
                    -(intptr_t)stubInfo.patch.dfg.deltaCheckImmToCall)),
            CodeLocationLabel(target));
        return;
    }
    
    repatchBuffer.relink(
        stubInfo.callReturnLocation.jumpAtOffset(
            stubInfo.patch.dfg.deltaCallToStructCheck),
        CodeLocationLabel(target));
}

static void emitRestoreScratch(MacroAssembler& stubJit, bool needToRestoreScratch, GPRReg scratchGPR, MacroAssembler::Jump& success, MacroAssembler::Jump& fail, MacroAssembler::JumpList failureCases)
{
    if (needToRestoreScratch) {
        stubJit.pop(scratchGPR);
        
        success = stubJit.jump();
        
        // link failure cases here, so we can pop scratchGPR, and then jump back.
        failureCases.link(&stubJit);
        
        stubJit.pop(scratchGPR);
        
        fail = stubJit.jump();
        return;
    }
    
    success = stubJit.jump();
}

static void linkRestoreScratch(LinkBuffer& patchBuffer, bool needToRestoreScratch, MacroAssembler::Jump success, MacroAssembler::Jump fail, MacroAssembler::JumpList failureCases, CodeLocationLabel successLabel, CodeLocationLabel slowCaseBegin)
{
    patchBuffer.link(success, successLabel);
        
    if (needToRestoreScratch) {
        patchBuffer.link(fail, slowCaseBegin);
        return;
    }
    
    // link failure cases directly back to normal path
    patchBuffer.link(failureCases, slowCaseBegin);
}

static void linkRestoreScratch(LinkBuffer& patchBuffer, bool needToRestoreScratch, StructureStubInfo& stubInfo, MacroAssembler::Jump success, MacroAssembler::Jump fail, MacroAssembler::JumpList failureCases)
{
    linkRestoreScratch(patchBuffer, needToRestoreScratch, success, fail, failureCases, stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.dfg.deltaCallToDone), stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.dfg.deltaCallToSlowCase));
}

static void generateProtoChainAccessStub(ExecState* exec, StructureStubInfo& stubInfo, StructureChain* chain, size_t count, PropertyOffset offset, Structure* structure, CodeLocationLabel successLabel, CodeLocationLabel slowCaseLabel, RefPtr<JITStubRoutine>& stubRoutine)
{
    VM* vm = &exec->vm();

    MacroAssembler stubJit;
        
    GPRReg baseGPR = static_cast<GPRReg>(stubInfo.patch.dfg.baseGPR);
#if USE(JSVALUE32_64)
    GPRReg resultTagGPR = static_cast<GPRReg>(stubInfo.patch.dfg.valueTagGPR);
#endif
    GPRReg resultGPR = static_cast<GPRReg>(stubInfo.patch.dfg.valueGPR);
    GPRReg scratchGPR = RegisterSet(stubInfo.patch.dfg.usedRegisters).getFreeGPR();
    bool needToRestoreScratch = false;
    
    if (scratchGPR == InvalidGPRReg) {
#if USE(JSVALUE64)
        scratchGPR = SpeculativeJIT::selectScratchGPR(baseGPR, resultGPR);
#else
        scratchGPR = SpeculativeJIT::selectScratchGPR(baseGPR, resultGPR, resultTagGPR);
#endif
        stubJit.push(scratchGPR);
        needToRestoreScratch = true;
    }
    
    MacroAssembler::JumpList failureCases;
    
    failureCases.append(stubJit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(baseGPR, JSCell::structureOffset()), MacroAssembler::TrustedImmPtr(structure)));
    
    Structure* currStructure = structure;
    WriteBarrier<Structure>* it = chain->head();
    JSObject* protoObject = 0;
    for (unsigned i = 0; i < count; ++i, ++it) {
        protoObject = asObject(currStructure->prototypeForLookup(exec));
        addStructureTransitionCheck(
            protoObject, protoObject->structure(), exec->codeBlock(), stubInfo, stubJit,
            failureCases, scratchGPR);
        currStructure = it->get();
    }
    
    if (isInlineOffset(offset)) {
#if USE(JSVALUE64)
        stubJit.load64(protoObject->locationForOffset(offset), resultGPR);
#elif USE(JSVALUE32_64)
        stubJit.move(MacroAssembler::TrustedImmPtr(protoObject->locationForOffset(offset)), resultGPR);
        stubJit.load32(MacroAssembler::Address(resultGPR, OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)), resultTagGPR);
        stubJit.load32(MacroAssembler::Address(resultGPR, OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)), resultGPR);
#endif
    } else {
        stubJit.loadPtr(protoObject->butterflyAddress(), resultGPR);
#if USE(JSVALUE64)
        stubJit.load64(MacroAssembler::Address(resultGPR, offsetInButterfly(offset) * sizeof(WriteBarrier<Unknown>)), resultGPR);
#elif USE(JSVALUE32_64)
        stubJit.load32(MacroAssembler::Address(resultGPR, offsetInButterfly(offset) * sizeof(WriteBarrier<Unknown>) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)), resultTagGPR);
        stubJit.load32(MacroAssembler::Address(resultGPR, offsetInButterfly(offset) * sizeof(WriteBarrier<Unknown>) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)), resultGPR);
#endif
    }

    MacroAssembler::Jump success, fail;
    
    emitRestoreScratch(stubJit, needToRestoreScratch, scratchGPR, success, fail, failureCases);
    
    LinkBuffer patchBuffer(*vm, &stubJit, exec->codeBlock());
    
    linkRestoreScratch(patchBuffer, needToRestoreScratch, success, fail, failureCases, successLabel, slowCaseLabel);
    
    stubRoutine = FINALIZE_CODE_FOR_DFG_STUB(
        patchBuffer,
        ("DFG prototype chain access stub for %s, return point %p",
            toCString(*exec->codeBlock()).data(), successLabel.executableAddress()));
}

static bool tryCacheGetByID(ExecState* exec, JSValue baseValue, const Identifier& propertyName, const PropertySlot& slot, StructureStubInfo& stubInfo)
{
    // FIXME: Write a test that proves we need to check for recursion here just
    // like the interpreter does, then add a check for recursion.

    CodeBlock* codeBlock = exec->codeBlock();
    VM* vm = &exec->vm();
    
    if (isJSArray(baseValue) && propertyName == exec->propertyNames().length) {
        GPRReg baseGPR = static_cast<GPRReg>(stubInfo.patch.dfg.baseGPR);
#if USE(JSVALUE32_64)
        GPRReg resultTagGPR = static_cast<GPRReg>(stubInfo.patch.dfg.valueTagGPR);
#endif
        GPRReg resultGPR = static_cast<GPRReg>(stubInfo.patch.dfg.valueGPR);
        GPRReg scratchGPR = RegisterSet(stubInfo.patch.dfg.usedRegisters).getFreeGPR();
        bool needToRestoreScratch = false;
        
        MacroAssembler stubJit;
        
        if (scratchGPR == InvalidGPRReg) {
#if USE(JSVALUE64)
            scratchGPR = SpeculativeJIT::selectScratchGPR(baseGPR, resultGPR);
#else
            scratchGPR = SpeculativeJIT::selectScratchGPR(baseGPR, resultGPR, resultTagGPR);
#endif
            stubJit.push(scratchGPR);
            needToRestoreScratch = true;
        }
        
        MacroAssembler::JumpList failureCases;
       
        stubJit.loadPtr(MacroAssembler::Address(baseGPR, JSCell::structureOffset()), scratchGPR); 
        stubJit.load8(MacroAssembler::Address(scratchGPR, Structure::indexingTypeOffset()), scratchGPR);
        failureCases.append(stubJit.branchTest32(MacroAssembler::Zero, scratchGPR, MacroAssembler::TrustedImm32(IsArray)));
        failureCases.append(stubJit.branchTest32(MacroAssembler::Zero, scratchGPR, MacroAssembler::TrustedImm32(IndexingShapeMask)));
        
        stubJit.loadPtr(MacroAssembler::Address(baseGPR, JSObject::butterflyOffset()), scratchGPR);
        stubJit.load32(MacroAssembler::Address(scratchGPR, ArrayStorage::lengthOffset()), scratchGPR);
        failureCases.append(stubJit.branch32(MacroAssembler::LessThan, scratchGPR, MacroAssembler::TrustedImm32(0)));

#if USE(JSVALUE64)
        stubJit.or64(GPRInfo::tagTypeNumberRegister, scratchGPR, resultGPR);
#elif USE(JSVALUE32_64)
        stubJit.move(scratchGPR, resultGPR);
        stubJit.move(JITCompiler::TrustedImm32(0xffffffff), resultTagGPR); // JSValue::Int32Tag
#endif

        MacroAssembler::Jump success, fail;
        
        emitRestoreScratch(stubJit, needToRestoreScratch, scratchGPR, success, fail, failureCases);
        
        LinkBuffer patchBuffer(*vm, &stubJit, codeBlock);
        
        linkRestoreScratch(patchBuffer, needToRestoreScratch, stubInfo, success, fail, failureCases);
        
        stubInfo.stubRoutine = FINALIZE_CODE_FOR_DFG_STUB(
            patchBuffer,
            ("DFG GetById array length stub for %s, return point %p",
                toCString(*exec->codeBlock()).data(), stubInfo.callReturnLocation.labelAtOffset(
                    stubInfo.patch.dfg.deltaCallToDone).executableAddress()));
        
        RepatchBuffer repatchBuffer(codeBlock);
        replaceWithJump(repatchBuffer, stubInfo, stubInfo.stubRoutine->code().code());
        repatchBuffer.relink(stubInfo.callReturnLocation, operationGetById);
        
        return true;
    }
    
    // FIXME: should support length access for String.

    // FIXME: Cache property access for immediates.
    if (!baseValue.isCell())
        return false;
    JSCell* baseCell = baseValue.asCell();
    Structure* structure = baseCell->structure();
    if (!slot.isCacheable())
        return false;
    if (!structure->propertyAccessesAreCacheable())
        return false;

    // Optimize self access.
    if (slot.slotBase() == baseValue) {
        if ((slot.cachedPropertyType() != PropertySlot::Value)
            || !MacroAssembler::isCompactPtrAlignedAddressOffset(maxOffsetRelativeToPatchedStorage(slot.cachedOffset()))) {
            dfgRepatchCall(codeBlock, stubInfo.callReturnLocation, operationGetByIdBuildList);
            return true;
        }

        dfgRepatchByIdSelfAccess(codeBlock, stubInfo, structure, slot.cachedOffset(), operationGetByIdBuildList, true);
        stubInfo.initGetByIdSelf(*vm, codeBlock->ownerExecutable(), structure);
        return true;
    }
    
    if (structure->isDictionary())
        return false;
    
    // FIXME: optimize getters and setters
    if (slot.cachedPropertyType() != PropertySlot::Value)
        return false;
    
    PropertyOffset offset = slot.cachedOffset();
    size_t count = normalizePrototypeChainForChainAccess(exec, baseValue, slot.slotBase(), propertyName, offset);
    if (count == InvalidPrototypeChain)
        return false;

    StructureChain* prototypeChain = structure->prototypeChain(exec);
    
    ASSERT(slot.slotBase().isObject());
    
    generateProtoChainAccessStub(exec, stubInfo, prototypeChain, count, offset, structure, stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.dfg.deltaCallToDone), stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.dfg.deltaCallToSlowCase), stubInfo.stubRoutine);
    
    RepatchBuffer repatchBuffer(codeBlock);
    replaceWithJump(repatchBuffer, stubInfo, stubInfo.stubRoutine->code().code());
    repatchBuffer.relink(stubInfo.callReturnLocation, operationGetByIdProtoBuildList);
    
    stubInfo.initGetByIdChain(*vm, codeBlock->ownerExecutable(), structure, prototypeChain, count, true);
    return true;
}

void dfgRepatchGetByID(ExecState* exec, JSValue baseValue, const Identifier& propertyName, const PropertySlot& slot, StructureStubInfo& stubInfo)
{
    bool cached = tryCacheGetByID(exec, baseValue, propertyName, slot, stubInfo);
    if (!cached)
        dfgRepatchCall(exec->codeBlock(), stubInfo.callReturnLocation, operationGetById);
}

static bool tryBuildGetByIDList(ExecState* exec, JSValue baseValue, const Identifier& ident, const PropertySlot& slot, StructureStubInfo& stubInfo)
{
    if (!baseValue.isCell()
        || !slot.isCacheable()
        || baseValue.asCell()->structure()->isUncacheableDictionary()
        || slot.slotBase() != baseValue)
        return false;
    
    if (!stubInfo.patch.dfg.registersFlushed) {
        // We cannot do as much inline caching if the registers were not flushed prior to this GetById. In particular,
        // non-Value cached properties require planting calls, which requires registers to have been flushed. Thus,
        // if registers were not flushed, don't do non-Value caching.
        if (slot.cachedPropertyType() != PropertySlot::Value)
            return false;
    }
    
    CodeBlock* codeBlock = exec->codeBlock();
    JSCell* baseCell = baseValue.asCell();
    Structure* structure = baseCell->structure();
    VM* vm = &exec->vm();
    
    ASSERT(slot.slotBase().isObject());
    
    PolymorphicAccessStructureList* polymorphicStructureList;
    int listIndex;
    
    if (stubInfo.accessType == access_unset) {
        ASSERT(!stubInfo.stubRoutine);
        polymorphicStructureList = new PolymorphicAccessStructureList();
        stubInfo.initGetByIdSelfList(polymorphicStructureList, 0);
        listIndex = 0;
    } else if (stubInfo.accessType == access_get_by_id_self) {
        ASSERT(!stubInfo.stubRoutine);
        polymorphicStructureList = new PolymorphicAccessStructureList(*vm, codeBlock->ownerExecutable(), JITStubRoutine::createSelfManagedRoutine(stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.dfg.deltaCallToSlowCase)), stubInfo.u.getByIdSelf.baseObjectStructure.get(), true);
        stubInfo.initGetByIdSelfList(polymorphicStructureList, 1);
        listIndex = 1;
    } else {
        polymorphicStructureList = stubInfo.u.getByIdSelfList.structureList;
        listIndex = stubInfo.u.getByIdSelfList.listSize;
    }
    
    if (listIndex < POLYMORPHIC_LIST_CACHE_SIZE) {
        stubInfo.u.getByIdSelfList.listSize++;
        
        GPRReg baseGPR = static_cast<GPRReg>(stubInfo.patch.dfg.baseGPR);
#if USE(JSVALUE32_64)
        GPRReg resultTagGPR = static_cast<GPRReg>(stubInfo.patch.dfg.valueTagGPR);
#endif
        GPRReg resultGPR = static_cast<GPRReg>(stubInfo.patch.dfg.valueGPR);
        GPRReg scratchGPR = RegisterSet(stubInfo.patch.dfg.usedRegisters).getFreeGPR();
        
        CCallHelpers stubJit(vm, codeBlock);
        
        MacroAssembler::Jump wrongStruct = stubJit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(baseGPR, JSCell::structureOffset()), MacroAssembler::TrustedImmPtr(structure));
        
        // The strategy we use for stubs is as follows:
        // 1) Call DFG helper that calls the getter.
        // 2) Check if there was an exception, and if there was, call yet another
        //    helper.
        
        bool isDirect = false;
        MacroAssembler::Call operationCall;
        MacroAssembler::Call handlerCall;
        FunctionPtr operationFunction;
        MacroAssembler::Jump success;
        
        if (slot.cachedPropertyType() == PropertySlot::Getter
            || slot.cachedPropertyType() == PropertySlot::Custom) {
            if (slot.cachedPropertyType() == PropertySlot::Getter) {
                ASSERT(scratchGPR != InvalidGPRReg);
                ASSERT(baseGPR != scratchGPR);
                if (isInlineOffset(slot.cachedOffset())) {
#if USE(JSVALUE64)
                    stubJit.load64(MacroAssembler::Address(baseGPR, offsetRelativeToBase(slot.cachedOffset())), scratchGPR);
#else
                    stubJit.load32(MacroAssembler::Address(baseGPR, offsetRelativeToBase(slot.cachedOffset())), scratchGPR);
#endif
                } else {
                    stubJit.loadPtr(MacroAssembler::Address(baseGPR, JSObject::butterflyOffset()), scratchGPR);
#if USE(JSVALUE64)
                    stubJit.load64(MacroAssembler::Address(scratchGPR, offsetRelativeToBase(slot.cachedOffset())), scratchGPR);
#else
                    stubJit.load32(MacroAssembler::Address(scratchGPR, offsetRelativeToBase(slot.cachedOffset())), scratchGPR);
#endif
                }
                stubJit.setupArgumentsWithExecState(baseGPR, scratchGPR);
                operationFunction = operationCallGetter;
            } else {
                stubJit.setupArgumentsWithExecState(
                    baseGPR,
                    MacroAssembler::TrustedImmPtr(FunctionPtr(slot.customGetter()).executableAddress()),
                    MacroAssembler::TrustedImmPtr(const_cast<Identifier*>(&ident)));
                operationFunction = operationCallCustomGetter;
            }
            
            // Need to make sure that whenever this call is made in the future, we remember the
            // place that we made it from. It just so happens to be the place that we are at
            // right now!
            stubJit.store32(
                MacroAssembler::TrustedImm32(exec->codeOriginIndexForDFG()),
                CCallHelpers::tagFor(static_cast<VirtualRegister>(JSStack::ArgumentCount)));
            
            operationCall = stubJit.call();
#if USE(JSVALUE64)
            stubJit.move(GPRInfo::returnValueGPR, resultGPR);
#else
            stubJit.setupResults(resultGPR, resultTagGPR);
#endif
            success = stubJit.emitExceptionCheck(CCallHelpers::InvertedExceptionCheck);
            
            stubJit.setupArgumentsWithExecState(
                MacroAssembler::TrustedImmPtr(&stubInfo));
            handlerCall = stubJit.call();
            stubJit.jump(GPRInfo::returnValueGPR2);
        } else {
            if (isInlineOffset(slot.cachedOffset())) {
#if USE(JSVALUE64)
                stubJit.load64(MacroAssembler::Address(baseGPR, offsetRelativeToBase(slot.cachedOffset())), resultGPR);
#else
                if (baseGPR == resultTagGPR) {
                    stubJit.load32(MacroAssembler::Address(baseGPR, offsetRelativeToBase(slot.cachedOffset()) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)), resultGPR);
                    stubJit.load32(MacroAssembler::Address(baseGPR, offsetRelativeToBase(slot.cachedOffset()) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)), resultTagGPR);
                } else {
                    stubJit.load32(MacroAssembler::Address(baseGPR, offsetRelativeToBase(slot.cachedOffset()) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)), resultTagGPR);
                    stubJit.load32(MacroAssembler::Address(baseGPR, offsetRelativeToBase(slot.cachedOffset()) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)), resultGPR);
                }
#endif
            } else {
                stubJit.loadPtr(MacroAssembler::Address(baseGPR, JSObject::butterflyOffset()), resultGPR);
#if USE(JSVALUE64)
                stubJit.load64(MacroAssembler::Address(resultGPR, offsetRelativeToBase(slot.cachedOffset())), resultGPR);
#else
                stubJit.load32(MacroAssembler::Address(resultGPR, offsetRelativeToBase(slot.cachedOffset()) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)), resultTagGPR);
                stubJit.load32(MacroAssembler::Address(resultGPR, offsetRelativeToBase(slot.cachedOffset()) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)), resultGPR);
#endif
            }
            success = stubJit.jump();
            isDirect = true;
        }

        LinkBuffer patchBuffer(*vm, &stubJit, codeBlock);
        
        CodeLocationLabel lastProtoBegin;
        if (listIndex)
            lastProtoBegin = CodeLocationLabel(polymorphicStructureList->list[listIndex - 1].stubRoutine->code().code());
        else
            lastProtoBegin = stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.dfg.deltaCallToSlowCase);
        ASSERT(!!lastProtoBegin);
        
        patchBuffer.link(wrongStruct, lastProtoBegin);
        patchBuffer.link(success, stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.dfg.deltaCallToDone));
        if (!isDirect) {
            patchBuffer.link(operationCall, operationFunction);
            patchBuffer.link(handlerCall, lookupExceptionHandlerInStub);
        }
        
        RefPtr<JITStubRoutine> stubRoutine =
            createJITStubRoutine(
                FINALIZE_DFG_CODE(
                    patchBuffer,
                    ("DFG GetById polymorphic list access for %s, return point %p",
                        toCString(*exec->codeBlock()).data(), stubInfo.callReturnLocation.labelAtOffset(
                            stubInfo.patch.dfg.deltaCallToDone).executableAddress())),
                *vm,
                codeBlock->ownerExecutable(),
                slot.cachedPropertyType() == PropertySlot::Getter
                || slot.cachedPropertyType() == PropertySlot::Custom);
        
        polymorphicStructureList->list[listIndex].set(*vm, codeBlock->ownerExecutable(), stubRoutine, structure, isDirect);
        
        RepatchBuffer repatchBuffer(codeBlock);
        repatchBuffer.relink(
            stubInfo.callReturnLocation.jumpAtOffset(
                stubInfo.patch.dfg.deltaCallToStructCheck),
            CodeLocationLabel(stubRoutine->code().code()));
        
        if (listIndex < (POLYMORPHIC_LIST_CACHE_SIZE - 1))
            return true;
    }
    
    return false;
}

void dfgBuildGetByIDList(ExecState* exec, JSValue baseValue, const Identifier& propertyName, const PropertySlot& slot, StructureStubInfo& stubInfo)
{
    bool dontChangeCall = tryBuildGetByIDList(exec, baseValue, propertyName, slot, stubInfo);
    if (!dontChangeCall)
        dfgRepatchCall(exec->codeBlock(), stubInfo.callReturnLocation, operationGetById);
}

static bool tryBuildGetByIDProtoList(ExecState* exec, JSValue baseValue, const Identifier& propertyName, const PropertySlot& slot, StructureStubInfo& stubInfo)
{
    if (!baseValue.isCell()
        || !slot.isCacheable()
        || baseValue.asCell()->structure()->isDictionary()
        || baseValue.asCell()->structure()->typeInfo().prohibitsPropertyCaching()
        || slot.slotBase() == baseValue
        || slot.cachedPropertyType() != PropertySlot::Value)
        return false;
    
    ASSERT(slot.slotBase().isObject());
    
    PropertyOffset offset = slot.cachedOffset();
    size_t count = normalizePrototypeChainForChainAccess(exec, baseValue, slot.slotBase(), propertyName, offset);
    if (count == InvalidPrototypeChain)
        return false;

    Structure* structure = baseValue.asCell()->structure();
    StructureChain* prototypeChain = structure->prototypeChain(exec);
    CodeBlock* codeBlock = exec->codeBlock();
    VM* vm = &exec->vm();
    
    PolymorphicAccessStructureList* polymorphicStructureList;
    int listIndex = 1;
    
    if (stubInfo.accessType == access_get_by_id_chain) {
        ASSERT(!!stubInfo.stubRoutine);
        polymorphicStructureList = new PolymorphicAccessStructureList(*vm, codeBlock->ownerExecutable(), stubInfo.stubRoutine, stubInfo.u.getByIdChain.baseObjectStructure.get(), stubInfo.u.getByIdChain.chain.get(), true);
        stubInfo.stubRoutine.clear();
        stubInfo.initGetByIdProtoList(polymorphicStructureList, 1);
    } else {
        ASSERT(stubInfo.accessType == access_get_by_id_proto_list);
        polymorphicStructureList = stubInfo.u.getByIdProtoList.structureList;
        listIndex = stubInfo.u.getByIdProtoList.listSize;
    }
    
    if (listIndex < POLYMORPHIC_LIST_CACHE_SIZE) {
        stubInfo.u.getByIdProtoList.listSize++;
        
        CodeLocationLabel lastProtoBegin = CodeLocationLabel(polymorphicStructureList->list[listIndex - 1].stubRoutine->code().code());
        ASSERT(!!lastProtoBegin);

        RefPtr<JITStubRoutine> stubRoutine;
        
        generateProtoChainAccessStub(exec, stubInfo, prototypeChain, count, offset, structure, stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.dfg.deltaCallToDone), lastProtoBegin, stubRoutine);
        
        polymorphicStructureList->list[listIndex].set(*vm, codeBlock->ownerExecutable(), stubRoutine, structure, true);
        
        RepatchBuffer repatchBuffer(codeBlock);
        replaceWithJump(repatchBuffer, stubInfo, stubRoutine->code().code());
        
        if (listIndex < (POLYMORPHIC_LIST_CACHE_SIZE - 1))
            return true;
    }
    
    return false;
}

void dfgBuildGetByIDProtoList(ExecState* exec, JSValue baseValue, const Identifier& propertyName, const PropertySlot& slot, StructureStubInfo& stubInfo)
{
    bool dontChangeCall = tryBuildGetByIDProtoList(exec, baseValue, propertyName, slot, stubInfo);
    if (!dontChangeCall)
        dfgRepatchCall(exec->codeBlock(), stubInfo.callReturnLocation, operationGetById);
}

static V_DFGOperation_EJCI appropriateGenericPutByIdFunction(const PutPropertySlot &slot, PutKind putKind)
{
    if (slot.isStrictMode()) {
        if (putKind == Direct)
            return operationPutByIdDirectStrict;
        return operationPutByIdStrict;
    }
    if (putKind == Direct)
        return operationPutByIdDirectNonStrict;
    return operationPutByIdNonStrict;
}

static V_DFGOperation_EJCI appropriateListBuildingPutByIdFunction(const PutPropertySlot &slot, PutKind putKind)
{
    if (slot.isStrictMode()) {
        if (putKind == Direct)
            return operationPutByIdDirectStrictBuildList;
        return operationPutByIdStrictBuildList;
    }
    if (putKind == Direct)
        return operationPutByIdDirectNonStrictBuildList;
    return operationPutByIdNonStrictBuildList;
}

static void emitPutReplaceStub(
    ExecState* exec,
    JSValue,
    const Identifier&,
    const PutPropertySlot& slot,
    StructureStubInfo& stubInfo,
    PutKind,
    Structure* structure,
    CodeLocationLabel failureLabel,
    RefPtr<JITStubRoutine>& stubRoutine)
{
    VM* vm = &exec->vm();
    GPRReg baseGPR = static_cast<GPRReg>(stubInfo.patch.dfg.baseGPR);
#if USE(JSVALUE32_64)
    GPRReg valueTagGPR = static_cast<GPRReg>(stubInfo.patch.dfg.valueTagGPR);
#endif
    GPRReg valueGPR = static_cast<GPRReg>(stubInfo.patch.dfg.valueGPR);
    GPRReg scratchGPR = RegisterSet(stubInfo.patch.dfg.usedRegisters).getFreeGPR();
    bool needToRestoreScratch = false;
#if ENABLE(WRITE_BARRIER_PROFILING)
    GPRReg scratchGPR2;
    const bool writeBarrierNeeded = true;
#else
    const bool writeBarrierNeeded = false;
#endif
    
    MacroAssembler stubJit;
    
    if (scratchGPR == InvalidGPRReg && (writeBarrierNeeded || isOutOfLineOffset(slot.cachedOffset()))) {
#if USE(JSVALUE64)
        scratchGPR = SpeculativeJIT::selectScratchGPR(baseGPR, valueGPR);
#else
        scratchGPR = SpeculativeJIT::selectScratchGPR(baseGPR, valueGPR, valueTagGPR);
#endif
        needToRestoreScratch = true;
        stubJit.push(scratchGPR);
    }

    MacroAssembler::Jump badStructure = stubJit.branchPtr(
        MacroAssembler::NotEqual,
        MacroAssembler::Address(baseGPR, JSCell::structureOffset()),
        MacroAssembler::TrustedImmPtr(structure));
    
#if ENABLE(WRITE_BARRIER_PROFILING)
#if USE(JSVALUE64)
    scratchGPR2 = SpeculativeJIT::selectScratchGPR(baseGPR, valueGPR, scratchGPR);
#else
    scratchGPR2 = SpeculativeJIT::selectScratchGPR(baseGPR, valueGPR, valueTagGPR, scratchGPR);
#endif
    stubJit.push(scratchGPR2);
    SpeculativeJIT::writeBarrier(stubJit, baseGPR, scratchGPR, scratchGPR2, WriteBarrierForPropertyAccess);
    stubJit.pop(scratchGPR2);
#endif
    
#if USE(JSVALUE64)
    if (isInlineOffset(slot.cachedOffset()))
        stubJit.store64(valueGPR, MacroAssembler::Address(baseGPR, JSObject::offsetOfInlineStorage() + offsetInInlineStorage(slot.cachedOffset()) * sizeof(JSValue)));
    else {
        stubJit.loadPtr(MacroAssembler::Address(baseGPR, JSObject::butterflyOffset()), scratchGPR);
        stubJit.store64(valueGPR, MacroAssembler::Address(scratchGPR, offsetInButterfly(slot.cachedOffset()) * sizeof(JSValue)));
    }
#elif USE(JSVALUE32_64)
    if (isInlineOffset(slot.cachedOffset())) {
        stubJit.store32(valueGPR, MacroAssembler::Address(baseGPR, JSObject::offsetOfInlineStorage() + offsetInInlineStorage(slot.cachedOffset()) * sizeof(JSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
        stubJit.store32(valueTagGPR, MacroAssembler::Address(baseGPR, JSObject::offsetOfInlineStorage() + offsetInInlineStorage(slot.cachedOffset()) * sizeof(JSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
    } else {
        stubJit.loadPtr(MacroAssembler::Address(baseGPR, JSObject::butterflyOffset()), scratchGPR);
        stubJit.store32(valueGPR, MacroAssembler::Address(scratchGPR, offsetInButterfly(slot.cachedOffset()) * sizeof(JSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
        stubJit.store32(valueTagGPR, MacroAssembler::Address(scratchGPR, offsetInButterfly(slot.cachedOffset()) * sizeof(JSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
    }
#endif
    
    MacroAssembler::Jump success;
    MacroAssembler::Jump failure;
    
    if (needToRestoreScratch) {
        stubJit.pop(scratchGPR);
        success = stubJit.jump();
        
        badStructure.link(&stubJit);
        stubJit.pop(scratchGPR);
        failure = stubJit.jump();
    } else {
        success = stubJit.jump();
        failure = badStructure;
    }
    
    LinkBuffer patchBuffer(*vm, &stubJit, exec->codeBlock());
    patchBuffer.link(success, stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.dfg.deltaCallToDone));
    patchBuffer.link(failure, failureLabel);
            
    stubRoutine = FINALIZE_CODE_FOR_DFG_STUB(
        patchBuffer,
        ("DFG PutById replace stub for %s, return point %p",
            toCString(*exec->codeBlock()).data(), stubInfo.callReturnLocation.labelAtOffset(
                stubInfo.patch.dfg.deltaCallToDone).executableAddress()));
}

static void emitPutTransitionStub(
    ExecState* exec,
    JSValue,
    const Identifier&,
    const PutPropertySlot& slot,
    StructureStubInfo& stubInfo,
    PutKind putKind,
    Structure* structure,
    Structure* oldStructure,
    StructureChain* prototypeChain,
    CodeLocationLabel failureLabel,
    RefPtr<JITStubRoutine>& stubRoutine)
{
    VM* vm = &exec->vm();

    GPRReg baseGPR = static_cast<GPRReg>(stubInfo.patch.dfg.baseGPR);
#if USE(JSVALUE32_64)
    GPRReg valueTagGPR = static_cast<GPRReg>(stubInfo.patch.dfg.valueTagGPR);
#endif
    GPRReg valueGPR = static_cast<GPRReg>(stubInfo.patch.dfg.valueGPR);
    
    ScratchRegisterAllocator allocator(stubInfo.patch.dfg.usedRegisters);
    allocator.lock(baseGPR);
#if USE(JSVALUE32_64)
    allocator.lock(valueTagGPR);
#endif
    allocator.lock(valueGPR);
    
    CCallHelpers stubJit(vm);
            
    GPRReg scratchGPR1 = allocator.allocateScratchGPR();
    ASSERT(scratchGPR1 != baseGPR);
    ASSERT(scratchGPR1 != valueGPR);
    
    bool needSecondScratch = false;
    bool needThirdScratch = false;
#if ENABLE(WRITE_BARRIER_PROFILING)
    needSecondScratch = true;
#endif
    if (structure->outOfLineCapacity() != oldStructure->outOfLineCapacity()
        && oldStructure->outOfLineCapacity()) {
        needSecondScratch = true;
        needThirdScratch = true;
    }

    GPRReg scratchGPR2;
    if (needSecondScratch) {
        scratchGPR2 = allocator.allocateScratchGPR();
        ASSERT(scratchGPR2 != baseGPR);
        ASSERT(scratchGPR2 != valueGPR);
        ASSERT(scratchGPR2 != scratchGPR1);
    } else
        scratchGPR2 = InvalidGPRReg;
    GPRReg scratchGPR3;
    if (needThirdScratch) {
        scratchGPR3 = allocator.allocateScratchGPR();
        ASSERT(scratchGPR3 != baseGPR);
        ASSERT(scratchGPR3 != valueGPR);
        ASSERT(scratchGPR3 != scratchGPR1);
        ASSERT(scratchGPR3 != scratchGPR2);
    } else
        scratchGPR3 = InvalidGPRReg;
            
    allocator.preserveReusedRegistersByPushing(stubJit);

    MacroAssembler::JumpList failureCases;
            
    ASSERT(oldStructure->transitionWatchpointSetHasBeenInvalidated());
    
    failureCases.append(stubJit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(baseGPR, JSCell::structureOffset()), MacroAssembler::TrustedImmPtr(oldStructure)));
    
    addStructureTransitionCheck(
        oldStructure->storedPrototype(), exec->codeBlock(), stubInfo, stubJit, failureCases,
        scratchGPR1);
            
    if (putKind == NotDirect) {
        for (WriteBarrier<Structure>* it = prototypeChain->head(); *it; ++it) {
            addStructureTransitionCheck(
                (*it)->storedPrototype(), exec->codeBlock(), stubInfo, stubJit, failureCases,
                scratchGPR1);
        }
    }

#if ENABLE(WRITE_BARRIER_PROFILING)
    ASSERT(needSecondScratch);
    ASSERT(scratchGPR2 != InvalidGPRReg);
    // Must always emit this write barrier as the structure transition itself requires it
    SpeculativeJIT::writeBarrier(stubJit, baseGPR, scratchGPR1, scratchGPR2, WriteBarrierForPropertyAccess);
#endif
    
    MacroAssembler::JumpList slowPath;
    
    bool scratchGPR1HasStorage = false;
    
    if (structure->outOfLineCapacity() != oldStructure->outOfLineCapacity()) {
        size_t newSize = structure->outOfLineCapacity() * sizeof(JSValue);
        CopiedAllocator* copiedAllocator = &vm->heap.storageAllocator();
        
        if (!oldStructure->outOfLineCapacity()) {
            stubJit.loadPtr(&copiedAllocator->m_currentRemaining, scratchGPR1);
            slowPath.append(stubJit.branchSubPtr(MacroAssembler::Signed, MacroAssembler::TrustedImm32(newSize), scratchGPR1));
            stubJit.storePtr(scratchGPR1, &copiedAllocator->m_currentRemaining);
            stubJit.negPtr(scratchGPR1);
            stubJit.addPtr(MacroAssembler::AbsoluteAddress(&copiedAllocator->m_currentPayloadEnd), scratchGPR1);
            stubJit.addPtr(MacroAssembler::TrustedImm32(sizeof(JSValue)), scratchGPR1);
        } else {
            size_t oldSize = oldStructure->outOfLineCapacity() * sizeof(JSValue);
            ASSERT(newSize > oldSize);
            
            stubJit.loadPtr(MacroAssembler::Address(baseGPR, JSObject::butterflyOffset()), scratchGPR3);
            stubJit.loadPtr(&copiedAllocator->m_currentRemaining, scratchGPR1);
            slowPath.append(stubJit.branchSubPtr(MacroAssembler::Signed, MacroAssembler::TrustedImm32(newSize), scratchGPR1));
            stubJit.storePtr(scratchGPR1, &copiedAllocator->m_currentRemaining);
            stubJit.negPtr(scratchGPR1);
            stubJit.addPtr(MacroAssembler::AbsoluteAddress(&copiedAllocator->m_currentPayloadEnd), scratchGPR1);
            stubJit.addPtr(MacroAssembler::TrustedImm32(sizeof(JSValue)), scratchGPR1);
            // We have scratchGPR1 = new storage, scratchGPR3 = old storage, scratchGPR2 = available
            for (ptrdiff_t offset = 0; offset < static_cast<ptrdiff_t>(oldSize); offset += sizeof(void*)) {
                stubJit.loadPtr(MacroAssembler::Address(scratchGPR3, -(offset + sizeof(JSValue) + sizeof(void*))), scratchGPR2);
                stubJit.storePtr(scratchGPR2, MacroAssembler::Address(scratchGPR1, -(offset + sizeof(JSValue) + sizeof(void*))));
            }
        }
        
        stubJit.storePtr(scratchGPR1, MacroAssembler::Address(baseGPR, JSObject::butterflyOffset()));
        scratchGPR1HasStorage = true;
    }

    stubJit.storePtr(MacroAssembler::TrustedImmPtr(structure), MacroAssembler::Address(baseGPR, JSCell::structureOffset()));
#if USE(JSVALUE64)
    if (isInlineOffset(slot.cachedOffset()))
        stubJit.store64(valueGPR, MacroAssembler::Address(baseGPR, JSObject::offsetOfInlineStorage() + offsetInInlineStorage(slot.cachedOffset()) * sizeof(JSValue)));
    else {
        if (!scratchGPR1HasStorage)
            stubJit.loadPtr(MacroAssembler::Address(baseGPR, JSObject::butterflyOffset()), scratchGPR1);
        stubJit.store64(valueGPR, MacroAssembler::Address(scratchGPR1, offsetInButterfly(slot.cachedOffset()) * sizeof(JSValue)));
    }
#elif USE(JSVALUE32_64)
    if (isInlineOffset(slot.cachedOffset())) {
        stubJit.store32(valueGPR, MacroAssembler::Address(baseGPR, JSObject::offsetOfInlineStorage() + offsetInInlineStorage(slot.cachedOffset()) * sizeof(JSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
        stubJit.store32(valueTagGPR, MacroAssembler::Address(baseGPR, JSObject::offsetOfInlineStorage() + offsetInInlineStorage(slot.cachedOffset()) * sizeof(JSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
    } else {
        if (!scratchGPR1HasStorage)
            stubJit.loadPtr(MacroAssembler::Address(baseGPR, JSObject::butterflyOffset()), scratchGPR1);
        stubJit.store32(valueGPR, MacroAssembler::Address(scratchGPR1, offsetInButterfly(slot.cachedOffset()) * sizeof(JSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
        stubJit.store32(valueTagGPR, MacroAssembler::Address(scratchGPR1, offsetInButterfly(slot.cachedOffset()) * sizeof(JSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
    }
#endif
            
    MacroAssembler::Jump success;
    MacroAssembler::Jump failure;
            
    if (allocator.didReuseRegisters()) {
        allocator.restoreReusedRegistersByPopping(stubJit);
        success = stubJit.jump();

        failureCases.link(&stubJit);
        allocator.restoreReusedRegistersByPopping(stubJit);
        failure = stubJit.jump();
    } else
        success = stubJit.jump();
    
    MacroAssembler::Call operationCall;
    MacroAssembler::Jump successInSlowPath;
    
    if (structure->outOfLineCapacity() != oldStructure->outOfLineCapacity()) {
        slowPath.link(&stubJit);
        
        allocator.restoreReusedRegistersByPopping(stubJit);
        ScratchBuffer* scratchBuffer = vm->scratchBufferForSize(allocator.desiredScratchBufferSize());
        allocator.preserveUsedRegistersToScratchBuffer(stubJit, scratchBuffer, scratchGPR1);
#if USE(JSVALUE64)
        stubJit.setupArgumentsWithExecState(baseGPR, MacroAssembler::TrustedImmPtr(structure), MacroAssembler::TrustedImm32(slot.cachedOffset()), valueGPR);
#else
        stubJit.setupArgumentsWithExecState(baseGPR, MacroAssembler::TrustedImmPtr(structure), MacroAssembler::TrustedImm32(slot.cachedOffset()), valueGPR, valueTagGPR);
#endif
        operationCall = stubJit.call();
        allocator.restoreUsedRegistersFromScratchBuffer(stubJit, scratchBuffer, scratchGPR1);
        successInSlowPath = stubJit.jump();
    }
    
    LinkBuffer patchBuffer(*vm, &stubJit, exec->codeBlock());
    patchBuffer.link(success, stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.dfg.deltaCallToDone));
    if (allocator.didReuseRegisters())
        patchBuffer.link(failure, failureLabel);
    else
        patchBuffer.link(failureCases, failureLabel);
    if (structure->outOfLineCapacity() != oldStructure->outOfLineCapacity()) {
        patchBuffer.link(operationCall, operationReallocateStorageAndFinishPut);
        patchBuffer.link(successInSlowPath, stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.dfg.deltaCallToDone));
    }
    
    stubRoutine =
        createJITStubRoutine(
            FINALIZE_DFG_CODE(
                patchBuffer,
                ("DFG PutById %stransition stub (%p -> %p) for %s, return point %p",
                    structure->outOfLineCapacity() != oldStructure->outOfLineCapacity() ? "reallocating " : "",
                    oldStructure, structure,
                    toCString(*exec->codeBlock()).data(), stubInfo.callReturnLocation.labelAtOffset(
                        stubInfo.patch.dfg.deltaCallToDone).executableAddress())),
            *vm,
            exec->codeBlock()->ownerExecutable(),
            structure->outOfLineCapacity() != oldStructure->outOfLineCapacity(),
            structure);
}

static bool tryCachePutByID(ExecState* exec, JSValue baseValue, const Identifier& ident, const PutPropertySlot& slot, StructureStubInfo& stubInfo, PutKind putKind)
{
    CodeBlock* codeBlock = exec->codeBlock();
    VM* vm = &exec->vm();

    if (!baseValue.isCell())
        return false;
    JSCell* baseCell = baseValue.asCell();
    Structure* structure = baseCell->structure();
    Structure* oldStructure = structure->previousID();
    
    if (!slot.isCacheable())
        return false;
    if (structure->isUncacheableDictionary())
        return false;

    // Optimize self access.
    if (slot.base() == baseValue) {
        if (slot.type() == PutPropertySlot::NewProperty) {
            if (structure->isDictionary())
                return false;
            
            // Skip optimizing the case where we need a realloc, if we don't have
            // enough registers to make it happen.
            if (GPRInfo::numberOfRegisters < 6
                && oldStructure->outOfLineCapacity() != structure->outOfLineCapacity()
                && oldStructure->outOfLineCapacity())
                return false;
            
            // Skip optimizing the case where we need realloc, and the structure has
            // indexing storage.
            if (hasIndexingHeader(oldStructure->indexingType()))
                return false;
            
            if (normalizePrototypeChain(exec, baseCell) == InvalidPrototypeChain)
                return false;
            
            StructureChain* prototypeChain = structure->prototypeChain(exec);
            
            emitPutTransitionStub(
                exec, baseValue, ident, slot, stubInfo, putKind,
                structure, oldStructure, prototypeChain,
                stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.dfg.deltaCallToSlowCase),
                stubInfo.stubRoutine);
            
            RepatchBuffer repatchBuffer(codeBlock);
            repatchBuffer.relink(
                stubInfo.callReturnLocation.jumpAtOffset(
                    stubInfo.patch.dfg.deltaCallToStructCheck),
                CodeLocationLabel(stubInfo.stubRoutine->code().code()));
            repatchBuffer.relink(stubInfo.callReturnLocation, appropriateListBuildingPutByIdFunction(slot, putKind));
            
            stubInfo.initPutByIdTransition(*vm, codeBlock->ownerExecutable(), oldStructure, structure, prototypeChain, putKind == Direct);
            
            return true;
        }

        dfgRepatchByIdSelfAccess(codeBlock, stubInfo, structure, slot.cachedOffset(), appropriateListBuildingPutByIdFunction(slot, putKind), false);
        stubInfo.initPutByIdReplace(*vm, codeBlock->ownerExecutable(), structure);
        return true;
    }

    return false;
}

void dfgRepatchPutByID(ExecState* exec, JSValue baseValue, const Identifier& propertyName, const PutPropertySlot& slot, StructureStubInfo& stubInfo, PutKind putKind)
{
    bool cached = tryCachePutByID(exec, baseValue, propertyName, slot, stubInfo, putKind);
    if (!cached)
        dfgRepatchCall(exec->codeBlock(), stubInfo.callReturnLocation, appropriateGenericPutByIdFunction(slot, putKind));
}

static bool tryBuildPutByIdList(ExecState* exec, JSValue baseValue, const Identifier& propertyName, const PutPropertySlot& slot, StructureStubInfo& stubInfo, PutKind putKind)
{
    CodeBlock* codeBlock = exec->codeBlock();
    VM* vm = &exec->vm();

    if (!baseValue.isCell())
        return false;
    JSCell* baseCell = baseValue.asCell();
    Structure* structure = baseCell->structure();
    Structure* oldStructure = structure->previousID();
    
    if (!slot.isCacheable())
        return false;
    if (structure->isUncacheableDictionary())
        return false;

    // Optimize self access.
    if (slot.base() == baseValue) {
        PolymorphicPutByIdList* list;
        RefPtr<JITStubRoutine> stubRoutine;
        
        if (slot.type() == PutPropertySlot::NewProperty) {
            if (structure->isDictionary())
                return false;
            
            // Skip optimizing the case where we need a realloc, if we don't have
            // enough registers to make it happen.
            if (GPRInfo::numberOfRegisters < 6
                && oldStructure->outOfLineCapacity() != structure->outOfLineCapacity()
                && oldStructure->outOfLineCapacity())
                return false;
            
            // Skip optimizing the case where we need realloc, and the structure has
            // indexing storage.
            if (hasIndexingHeader(oldStructure->indexingType()))
                return false;
            
            if (normalizePrototypeChain(exec, baseCell) == InvalidPrototypeChain)
                return false;
            
            StructureChain* prototypeChain = structure->prototypeChain(exec);
            
            // We're now committed to creating the stub. Mogrify the meta-data accordingly.
            list = PolymorphicPutByIdList::from(
                putKind, stubInfo,
                stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.dfg.deltaCallToSlowCase));
            
            emitPutTransitionStub(
                exec, baseValue, propertyName, slot, stubInfo, putKind,
                structure, oldStructure, prototypeChain,
                CodeLocationLabel(list->currentSlowPathTarget()),
                stubRoutine);
            
            list->addAccess(
                PutByIdAccess::transition(
                    *vm, codeBlock->ownerExecutable(),
                    oldStructure, structure, prototypeChain,
                    stubRoutine));
        } else {
            // We're now committed to creating the stub. Mogrify the meta-data accordingly.
            list = PolymorphicPutByIdList::from(
                putKind, stubInfo,
                stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.dfg.deltaCallToSlowCase));
            
            emitPutReplaceStub(
                exec, baseValue, propertyName, slot, stubInfo, putKind,
                structure, CodeLocationLabel(list->currentSlowPathTarget()), stubRoutine);
            
            list->addAccess(
                PutByIdAccess::replace(
                    *vm, codeBlock->ownerExecutable(),
                    structure, stubRoutine));
        }
        
        RepatchBuffer repatchBuffer(codeBlock);
        repatchBuffer.relink(stubInfo.callReturnLocation.jumpAtOffset(stubInfo.patch.dfg.deltaCallToStructCheck), CodeLocationLabel(stubRoutine->code().code()));
        
        if (list->isFull())
            repatchBuffer.relink(stubInfo.callReturnLocation, appropriateGenericPutByIdFunction(slot, putKind));
        
        return true;
    }
    
    return false;
}

void dfgBuildPutByIdList(ExecState* exec, JSValue baseValue, const Identifier& propertyName, const PutPropertySlot& slot, StructureStubInfo& stubInfo, PutKind putKind)
{
    bool cached = tryBuildPutByIdList(exec, baseValue, propertyName, slot, stubInfo, putKind);
    if (!cached)
        dfgRepatchCall(exec->codeBlock(), stubInfo.callReturnLocation, appropriateGenericPutByIdFunction(slot, putKind));
}

static void linkSlowFor(RepatchBuffer& repatchBuffer, VM* vm, CallLinkInfo& callLinkInfo, CodeSpecializationKind kind)
{
    if (kind == CodeForCall) {
        repatchBuffer.relink(callLinkInfo.callReturnLocation, vm->getCTIStub(virtualCallThunkGenerator).code());
        return;
    }
    ASSERT(kind == CodeForConstruct);
    repatchBuffer.relink(callLinkInfo.callReturnLocation, vm->getCTIStub(virtualConstructThunkGenerator).code());
}

void dfgLinkFor(ExecState* exec, CallLinkInfo& callLinkInfo, CodeBlock* calleeCodeBlock, JSFunction* callee, MacroAssemblerCodePtr codePtr, CodeSpecializationKind kind)
{
    ASSERT(!callLinkInfo.stub);
    
    CodeBlock* callerCodeBlock = exec->callerFrame()->codeBlock();
    VM* vm = callerCodeBlock->vm();
    
    RepatchBuffer repatchBuffer(callerCodeBlock);
    
    ASSERT(!callLinkInfo.isLinked());
    callLinkInfo.callee.set(exec->callerFrame()->vm(), callLinkInfo.hotPathBegin, callerCodeBlock->ownerExecutable(), callee);
    callLinkInfo.lastSeenCallee.set(exec->callerFrame()->vm(), callerCodeBlock->ownerExecutable(), callee);
    repatchBuffer.relink(callLinkInfo.hotPathOther, codePtr);
    
    if (calleeCodeBlock)
        calleeCodeBlock->linkIncomingCall(&callLinkInfo);
    
    if (kind == CodeForCall) {
        repatchBuffer.relink(callLinkInfo.callReturnLocation, vm->getCTIStub(linkClosureCallThunkGenerator).code());
        return;
    }
    
    ASSERT(kind == CodeForConstruct);
    linkSlowFor(repatchBuffer, vm, callLinkInfo, CodeForConstruct);
}

void dfgLinkSlowFor(ExecState* exec, CallLinkInfo& callLinkInfo, CodeSpecializationKind kind)
{
    CodeBlock* callerCodeBlock = exec->callerFrame()->codeBlock();
    VM* vm = callerCodeBlock->vm();
    
    RepatchBuffer repatchBuffer(callerCodeBlock);
    
    linkSlowFor(repatchBuffer, vm, callLinkInfo, kind);
}

void dfgLinkClosureCall(ExecState* exec, CallLinkInfo& callLinkInfo, CodeBlock* calleeCodeBlock, Structure* structure, ExecutableBase* executable, MacroAssemblerCodePtr codePtr)
{
    ASSERT(!callLinkInfo.stub);
    
    CodeBlock* callerCodeBlock = exec->callerFrame()->codeBlock();
    VM* vm = callerCodeBlock->vm();
    
    GPRReg calleeGPR = static_cast<GPRReg>(callLinkInfo.calleeGPR);
    
    CCallHelpers stubJit(vm, callerCodeBlock);
    
    CCallHelpers::JumpList slowPath;
    
#if USE(JSVALUE64)
    slowPath.append(
        stubJit.branchTest64(
            CCallHelpers::NonZero, calleeGPR, GPRInfo::tagMaskRegister));
#else
    // We would have already checked that the callee is a cell.
#endif
    
    slowPath.append(
        stubJit.branchPtr(
            CCallHelpers::NotEqual,
            CCallHelpers::Address(calleeGPR, JSCell::structureOffset()),
            CCallHelpers::TrustedImmPtr(structure)));
    
    slowPath.append(
        stubJit.branchPtr(
            CCallHelpers::NotEqual,
            CCallHelpers::Address(calleeGPR, JSFunction::offsetOfExecutable()),
            CCallHelpers::TrustedImmPtr(executable)));
    
    stubJit.loadPtr(
        CCallHelpers::Address(calleeGPR, JSFunction::offsetOfScopeChain()),
        GPRInfo::returnValueGPR);

#if USE(JSVALUE64)
    stubJit.store64(
        GPRInfo::returnValueGPR,
        CCallHelpers::Address(GPRInfo::callFrameRegister, static_cast<ptrdiff_t>(sizeof(Register) * JSStack::ScopeChain)));
#else
    stubJit.storePtr(
        GPRInfo::returnValueGPR,
        CCallHelpers::Address(GPRInfo::callFrameRegister, static_cast<ptrdiff_t>(sizeof(Register) * JSStack::ScopeChain) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
    stubJit.store32(
        CCallHelpers::TrustedImm32(JSValue::CellTag),
        CCallHelpers::Address(GPRInfo::callFrameRegister, static_cast<ptrdiff_t>(sizeof(Register) * JSStack::ScopeChain) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
#endif
    
    JITCompiler::Call call = stubJit.nearCall();
    JITCompiler::Jump done = stubJit.jump();
    
    slowPath.link(&stubJit);
    stubJit.move(calleeGPR, GPRInfo::nonArgGPR0);
#if USE(JSVALUE32_64)
    stubJit.move(CCallHelpers::TrustedImm32(JSValue::CellTag), GPRInfo::nonArgGPR1);
#endif
    stubJit.move(CCallHelpers::TrustedImmPtr(callLinkInfo.callReturnLocation.executableAddress()), GPRInfo::nonArgGPR2);
    stubJit.restoreReturnAddressBeforeReturn(GPRInfo::nonArgGPR2);
    JITCompiler::Jump slow = stubJit.jump();
    
    LinkBuffer patchBuffer(*vm, &stubJit, callerCodeBlock);
    
    patchBuffer.link(call, FunctionPtr(codePtr.executableAddress()));
    patchBuffer.link(done, callLinkInfo.callReturnLocation.labelAtOffset(0));
    patchBuffer.link(slow, CodeLocationLabel(vm->getCTIStub(virtualCallThunkGenerator).code()));
    
    RefPtr<ClosureCallStubRoutine> stubRoutine = adoptRef(new ClosureCallStubRoutine(
        FINALIZE_DFG_CODE(
            patchBuffer,
            ("DFG closure call stub for %s, return point %p, target %p (%s)",
                toCString(*callerCodeBlock).data(), callLinkInfo.callReturnLocation.labelAtOffset(0).executableAddress(),
                codePtr.executableAddress(), toCString(pointerDump(calleeCodeBlock)).data())),
        *vm, callerCodeBlock->ownerExecutable(), structure, executable, callLinkInfo.codeOrigin));
    
    RepatchBuffer repatchBuffer(callerCodeBlock);
    
    repatchBuffer.replaceWithJump(
        RepatchBuffer::startOfBranchPtrWithPatchOnRegister(callLinkInfo.hotPathBegin),
        CodeLocationLabel(stubRoutine->code().code()));
    linkSlowFor(repatchBuffer, vm, callLinkInfo, CodeForCall);
    
    callLinkInfo.stub = stubRoutine.release();
    
    ASSERT(!calleeCodeBlock || calleeCodeBlock->isIncomingCallAlreadyLinked(&callLinkInfo));
}

void dfgResetGetByID(RepatchBuffer& repatchBuffer, StructureStubInfo& stubInfo)
{
    repatchBuffer.relink(stubInfo.callReturnLocation, operationGetByIdOptimize);
    CodeLocationDataLabelPtr structureLabel = stubInfo.callReturnLocation.dataLabelPtrAtOffset(-(intptr_t)stubInfo.patch.dfg.deltaCheckImmToCall);
    if (MacroAssembler::canJumpReplacePatchableBranchPtrWithPatch()) {
        repatchBuffer.revertJumpReplacementToPatchableBranchPtrWithPatch(
            RepatchBuffer::startOfPatchableBranchPtrWithPatchOnAddress(structureLabel),
            MacroAssembler::Address(
                static_cast<MacroAssembler::RegisterID>(stubInfo.patch.dfg.baseGPR),
                JSCell::structureOffset()),
            reinterpret_cast<void*>(unusedPointer));
    }
    repatchBuffer.repatch(structureLabel, reinterpret_cast<void*>(unusedPointer));
#if USE(JSVALUE64)
    repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabelCompactAtOffset(stubInfo.patch.dfg.deltaCallToLoadOrStore), 0);
#else
    repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabelCompactAtOffset(stubInfo.patch.dfg.deltaCallToTagLoadOrStore), 0);
    repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabelCompactAtOffset(stubInfo.patch.dfg.deltaCallToPayloadLoadOrStore), 0);
#endif
    repatchBuffer.relink(stubInfo.callReturnLocation.jumpAtOffset(stubInfo.patch.dfg.deltaCallToStructCheck), stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.dfg.deltaCallToSlowCase));
}

void dfgResetPutByID(RepatchBuffer& repatchBuffer, StructureStubInfo& stubInfo)
{
    V_DFGOperation_EJCI unoptimizedFunction = bitwise_cast<V_DFGOperation_EJCI>(MacroAssembler::readCallTarget(stubInfo.callReturnLocation).executableAddress());
    V_DFGOperation_EJCI optimizedFunction;
    if (unoptimizedFunction == operationPutByIdStrict || unoptimizedFunction == operationPutByIdStrictBuildList)
        optimizedFunction = operationPutByIdStrictOptimize;
    else if (unoptimizedFunction == operationPutByIdNonStrict || unoptimizedFunction == operationPutByIdNonStrictBuildList)
        optimizedFunction = operationPutByIdNonStrictOptimize;
    else if (unoptimizedFunction == operationPutByIdDirectStrict || unoptimizedFunction == operationPutByIdDirectStrictBuildList)
        optimizedFunction = operationPutByIdDirectStrictOptimize;
    else {
        ASSERT(unoptimizedFunction == operationPutByIdDirectNonStrict || unoptimizedFunction == operationPutByIdDirectNonStrictBuildList);
        optimizedFunction = operationPutByIdDirectNonStrictOptimize;
    }
    repatchBuffer.relink(stubInfo.callReturnLocation, optimizedFunction);
    CodeLocationDataLabelPtr structureLabel = stubInfo.callReturnLocation.dataLabelPtrAtOffset(-(intptr_t)stubInfo.patch.dfg.deltaCheckImmToCall);
    if (MacroAssembler::canJumpReplacePatchableBranchPtrWithPatch()) {
        repatchBuffer.revertJumpReplacementToPatchableBranchPtrWithPatch(
            RepatchBuffer::startOfPatchableBranchPtrWithPatchOnAddress(structureLabel),
            MacroAssembler::Address(
                static_cast<MacroAssembler::RegisterID>(stubInfo.patch.dfg.baseGPR),
                JSCell::structureOffset()),
            reinterpret_cast<void*>(unusedPointer));
    }
    repatchBuffer.repatch(structureLabel, reinterpret_cast<void*>(unusedPointer));
#if USE(JSVALUE64)
    repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabel32AtOffset(stubInfo.patch.dfg.deltaCallToLoadOrStore), 0);
#else
    repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabel32AtOffset(stubInfo.patch.dfg.deltaCallToTagLoadOrStore), 0);
    repatchBuffer.repatch(stubInfo.callReturnLocation.dataLabel32AtOffset(stubInfo.patch.dfg.deltaCallToPayloadLoadOrStore), 0);
#endif
    repatchBuffer.relink(stubInfo.callReturnLocation.jumpAtOffset(stubInfo.patch.dfg.deltaCallToStructCheck), stubInfo.callReturnLocation.labelAtOffset(stubInfo.patch.dfg.deltaCallToSlowCase));
}

} } // namespace JSC::DFG

#endif
