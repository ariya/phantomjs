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
#include "DFGOSRExitCompiler.h"

#if ENABLE(DFG_JIT) && USE(JSVALUE32_64)

#include "DFGOperations.h"
#include "Operations.h"
#include <wtf/DataLog.h>

namespace JSC { namespace DFG {

void OSRExitCompiler::compileExit(const OSRExit& exit, const Operands<ValueRecovery>& operands, SpeculationRecovery* recovery)
{
    // 1) Pro-forma stuff.
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLogF("OSR exit (");
    for (CodeOrigin codeOrigin = exit.m_codeOrigin; ; codeOrigin = codeOrigin.inlineCallFrame->caller) {
        dataLogF("bc#%u", codeOrigin.bytecodeIndex);
        if (!codeOrigin.inlineCallFrame)
            break;
        dataLogF(" -> %p ", codeOrigin.inlineCallFrame->executable.get());
    }
    dataLogF(") at JIT offset 0x%x  ", m_jit.debugOffset());
    dumpOperands(operands, WTF::dataFile());
#endif
    
    if (Options::printEachOSRExit()) {
        SpeculationFailureDebugInfo* debugInfo = new SpeculationFailureDebugInfo;
        debugInfo->codeBlock = m_jit.codeBlock();
        
        m_jit.debugCall(debugOperationPrintSpeculationFailure, debugInfo);
    }
    
#if DFG_ENABLE(JIT_BREAK_ON_SPECULATION_FAILURE)
    m_jit.breakpoint();
#endif
    
#if DFG_ENABLE(SUCCESS_STATS)
    static SamplingCounter counter("SpeculationFailure");
    m_jit.emitCount(counter);
#endif

    // 2) Perform speculation recovery. This only comes into play when an operation
    //    starts mutating state before verifying the speculation it has already made.
    
    if (recovery) {
        switch (recovery->type()) {
        case SpeculativeAdd:
            m_jit.sub32(recovery->src(), recovery->dest());
            break;
            
        case BooleanSpeculationCheck:
            break;
            
        default:
            break;
        }
    }

    // 3) Refine some value profile, if appropriate.
    
    if (!!exit.m_jsValueSource) {
        if (exit.m_kind == BadCache || exit.m_kind == BadIndexingType) {
            // If the instruction that this originated from has an array profile, then
            // refine it. If it doesn't, then do nothing. The latter could happen for
            // hoisted checks, or checks emitted for operations that didn't have array
            // profiling - either ops that aren't array accesses at all, or weren't
            // known to be array acceses in the bytecode. The latter case is a FIXME
            // while the former case is an outcome of a CheckStructure not knowing why
            // it was emitted (could be either due to an inline cache of a property
            // property access, or due to an array profile).
            
            // Note: We are free to assume that the jsValueSource is already known to
            // be a cell since both BadCache and BadIndexingType exits occur after
            // the cell check would have already happened.
            
            CodeOrigin codeOrigin = exit.m_codeOriginForExitProfile;
            if (ArrayProfile* arrayProfile = m_jit.baselineCodeBlockFor(codeOrigin)->getArrayProfile(codeOrigin.bytecodeIndex)) {
                GPRReg usedRegister1;
                GPRReg usedRegister2;
                if (exit.m_jsValueSource.isAddress()) {
                    usedRegister1 = exit.m_jsValueSource.base();
                    usedRegister2 = InvalidGPRReg;
                } else {
                    usedRegister1 = exit.m_jsValueSource.payloadGPR();
                    if (exit.m_jsValueSource.hasKnownTag())
                        usedRegister2 = InvalidGPRReg;
                    else
                        usedRegister2 = exit.m_jsValueSource.tagGPR();
                }
                
                GPRReg scratch1;
                GPRReg scratch2;
                scratch1 = AssemblyHelpers::selectScratchGPR(usedRegister1, usedRegister2);
                scratch2 = AssemblyHelpers::selectScratchGPR(usedRegister1, usedRegister2, scratch1);
                
                m_jit.push(scratch1);
                m_jit.push(scratch2);
                
                GPRReg value;
                if (exit.m_jsValueSource.isAddress()) {
                    value = scratch1;
                    m_jit.loadPtr(AssemblyHelpers::Address(exit.m_jsValueSource.asAddress()), value);
                } else
                    value = exit.m_jsValueSource.payloadGPR();
                
                m_jit.loadPtr(AssemblyHelpers::Address(value, JSCell::structureOffset()), scratch1);
                m_jit.storePtr(scratch1, arrayProfile->addressOfLastSeenStructure());
                m_jit.load8(AssemblyHelpers::Address(scratch1, Structure::indexingTypeOffset()), scratch1);
                m_jit.move(AssemblyHelpers::TrustedImm32(1), scratch2);
                m_jit.lshift32(scratch1, scratch2);
                m_jit.or32(scratch2, AssemblyHelpers::AbsoluteAddress(arrayProfile->addressOfArrayModes()));
                
                m_jit.pop(scratch2);
                m_jit.pop(scratch1);
            }
        }
        
        if (!!exit.m_valueProfile) {
            EncodedJSValue* bucket = exit.m_valueProfile.getSpecFailBucket(0);
        
            if (exit.m_jsValueSource.isAddress()) {
                // Save a register so we can use it.
                GPRReg scratch = AssemblyHelpers::selectScratchGPR(exit.m_jsValueSource.base());
                
                m_jit.push(scratch);

                m_jit.load32(exit.m_jsValueSource.asAddress(OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)), scratch);
                m_jit.store32(scratch, &bitwise_cast<EncodedValueDescriptor*>(bucket)->asBits.tag);
                m_jit.load32(exit.m_jsValueSource.asAddress(OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)), scratch);
                m_jit.store32(scratch, &bitwise_cast<EncodedValueDescriptor*>(bucket)->asBits.payload);
                
                m_jit.pop(scratch);
            } else if (exit.m_jsValueSource.hasKnownTag()) {
                m_jit.store32(AssemblyHelpers::TrustedImm32(exit.m_jsValueSource.tag()), &bitwise_cast<EncodedValueDescriptor*>(bucket)->asBits.tag);
                m_jit.store32(exit.m_jsValueSource.payloadGPR(), &bitwise_cast<EncodedValueDescriptor*>(bucket)->asBits.payload);
            } else {
                m_jit.store32(exit.m_jsValueSource.tagGPR(), &bitwise_cast<EncodedValueDescriptor*>(bucket)->asBits.tag);
                m_jit.store32(exit.m_jsValueSource.payloadGPR(), &bitwise_cast<EncodedValueDescriptor*>(bucket)->asBits.payload);
            }
        }
    }
    
    // 4) Figure out how many scratch slots we'll need. We need one for every GPR/FPR
    //    whose destination is now occupied by a DFG virtual register, and we need
    //    one for every displaced virtual register if there are more than
    //    GPRInfo::numberOfRegisters of them. Also see if there are any constants,
    //    any undefined slots, any FPR slots, and any unboxed ints.
            
    Vector<bool> poisonedVirtualRegisters(operands.numberOfLocals());
    for (unsigned i = 0; i < poisonedVirtualRegisters.size(); ++i)
        poisonedVirtualRegisters[i] = false;

    unsigned numberOfPoisonedVirtualRegisters = 0;
    unsigned numberOfDisplacedVirtualRegisters = 0;
    
    // Booleans for fast checks. We expect that most OSR exits do not have to rebox
    // Int32s, have no FPRs, and have no constants. If there are constants, we
    // expect most of them to be jsUndefined(); if that's true then we handle that
    // specially to minimize code size and execution time.
    bool haveUnboxedInt32InJSStack = false;
    bool haveUnboxedCellInJSStack = false;
    bool haveUnboxedBooleanInJSStack = false;
    bool haveUInt32s = false;
    bool haveFPRs = false;
    bool haveConstants = false;
    bool haveUndefined = false;
    bool haveArguments = false;
    
    for (size_t index = 0; index < operands.size(); ++index) {
        const ValueRecovery& recovery = operands[index];
        switch (recovery.technique()) {
        case DisplacedInJSStack:
        case Int32DisplacedInJSStack:
        case CellDisplacedInJSStack:
        case BooleanDisplacedInJSStack:
            numberOfDisplacedVirtualRegisters++;
            ASSERT((int)recovery.virtualRegister() >= 0);
            
            // See if we might like to store to this virtual register before doing
            // virtual register shuffling. If so, we say that the virtual register
            // is poisoned: it cannot be stored to until after displaced virtual
            // registers are handled. We track poisoned virtual register carefully
            // to ensure this happens efficiently. Note that we expect this case
            // to be rare, so the handling of it is optimized for the cases in
            // which it does not happen.
            if (recovery.virtualRegister() < (int)operands.numberOfLocals()) {
                switch (operands.local(recovery.virtualRegister()).technique()) {
                case InGPR:
                case UnboxedInt32InGPR:
                case UnboxedBooleanInGPR:
                case UInt32InGPR:
                case InPair:
                case InFPR:
                    if (!poisonedVirtualRegisters[recovery.virtualRegister()]) {
                        poisonedVirtualRegisters[recovery.virtualRegister()] = true;
                        numberOfPoisonedVirtualRegisters++;
                    }
                    break;
                default:
                    break;
                }
            }
            break;
            
        case UInt32InGPR:
            haveUInt32s = true;
            break;

        case AlreadyInJSStackAsUnboxedInt32:
            haveUnboxedInt32InJSStack = true;
            break;
            
        case AlreadyInJSStackAsUnboxedCell:
            haveUnboxedCellInJSStack = true;
            break;
            
        case AlreadyInJSStackAsUnboxedBoolean:
            haveUnboxedBooleanInJSStack = true;
            break;
            
        case InFPR:
            haveFPRs = true;
            break;
            
        case Constant:
            haveConstants = true;
            if (recovery.constant().isUndefined())
                haveUndefined = true;
            break;
            
        case ArgumentsThatWereNotCreated:
            haveArguments = true;
            break;
            
        default:
            break;
        }
    }
    
    unsigned scratchBufferLengthBeforeUInt32s = numberOfPoisonedVirtualRegisters + ((numberOfDisplacedVirtualRegisters * 2) <= GPRInfo::numberOfRegisters ? 0 : numberOfDisplacedVirtualRegisters);
    ScratchBuffer* scratchBuffer = m_jit.vm()->scratchBufferForSize(sizeof(EncodedJSValue) * (scratchBufferLengthBeforeUInt32s + (haveUInt32s ? 2 : 0)));
    EncodedJSValue* scratchDataBuffer = scratchBuffer ? static_cast<EncodedJSValue*>(scratchBuffer->dataBuffer()) : 0;

    // From here on, the code assumes that it is profitable to maximize the distance
    // between when something is computed and when it is stored.
    
    // 5) Perform all reboxing of integers and cells, except for those in registers.

    if (haveUnboxedInt32InJSStack || haveUnboxedCellInJSStack || haveUnboxedBooleanInJSStack) {
        for (size_t index = 0; index < operands.size(); ++index) {
            const ValueRecovery& recovery = operands[index];
            switch (recovery.technique()) {
            case AlreadyInJSStackAsUnboxedInt32:
                m_jit.store32(AssemblyHelpers::TrustedImm32(JSValue::Int32Tag), AssemblyHelpers::tagFor(static_cast<VirtualRegister>(operands.operandForIndex(index))));
                break;

            case AlreadyInJSStackAsUnboxedCell:
                m_jit.store32(AssemblyHelpers::TrustedImm32(JSValue::CellTag), AssemblyHelpers::tagFor(static_cast<VirtualRegister>(operands.operandForIndex(index))));
                break;

            case AlreadyInJSStackAsUnboxedBoolean:
                m_jit.store32(AssemblyHelpers::TrustedImm32(JSValue::BooleanTag), AssemblyHelpers::tagFor(static_cast<VirtualRegister>(operands.operandForIndex(index))));
                break;

            default:
                break;
            }
        }
    }

    // 6) Dump all non-poisoned GPRs. For poisoned GPRs, save them into the scratch storage.
    //    Note that GPRs do not have a fast change (like haveFPRs) because we expect that
    //    most OSR failure points will have at least one GPR that needs to be dumped.
    
    initializePoisoned(operands.numberOfLocals());
    unsigned currentPoisonIndex = 0;
    
    for (size_t index = 0; index < operands.size(); ++index) {
        const ValueRecovery& recovery = operands[index];
        int operand = operands.operandForIndex(index);
        switch (recovery.technique()) {
        case InGPR:
        case UnboxedInt32InGPR:
        case UnboxedBooleanInGPR:
            if (operands.isVariable(index) && poisonedVirtualRegisters[operands.variableForIndex(index)]) {
                m_jit.store32(recovery.gpr(), reinterpret_cast<char*>(scratchDataBuffer + currentPoisonIndex) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
                m_poisonScratchIndices[operands.variableForIndex(index)] = currentPoisonIndex;
                currentPoisonIndex++;
            } else {
                uint32_t tag = JSValue::EmptyValueTag;
                if (recovery.technique() == InGPR)
                    tag = JSValue::CellTag;
                else if (recovery.technique() == UnboxedInt32InGPR)
                    tag = JSValue::Int32Tag;
                else
                    tag = JSValue::BooleanTag;
                m_jit.store32(AssemblyHelpers::TrustedImm32(tag), AssemblyHelpers::tagFor((VirtualRegister)operand));
                m_jit.store32(recovery.gpr(), AssemblyHelpers::payloadFor((VirtualRegister)operand));
            }
            break;
        case InPair:
            if (operands.isVariable(index) && poisonedVirtualRegisters[operands.variableForIndex(index)]) {
                m_jit.store32(recovery.tagGPR(), reinterpret_cast<char*>(scratchDataBuffer + currentPoisonIndex) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
                m_jit.store32(recovery.payloadGPR(), reinterpret_cast<char*>(scratchDataBuffer + currentPoisonIndex) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
                m_poisonScratchIndices[operands.variableForIndex(index)] = currentPoisonIndex;
                currentPoisonIndex++;
            } else {
                m_jit.store32(recovery.tagGPR(), AssemblyHelpers::tagFor((VirtualRegister)operand));
                m_jit.store32(recovery.payloadGPR(), AssemblyHelpers::payloadFor((VirtualRegister)operand));
            }
            break;
        case UInt32InGPR: {
            EncodedJSValue* myScratch = scratchDataBuffer + scratchBufferLengthBeforeUInt32s;
            
            GPRReg addressGPR = GPRInfo::regT0;
            if (addressGPR == recovery.gpr())
                addressGPR = GPRInfo::regT1;
            
            m_jit.storePtr(addressGPR, myScratch);
            m_jit.move(AssemblyHelpers::TrustedImmPtr(myScratch + 1), addressGPR);
            m_jit.storeDouble(FPRInfo::fpRegT0, addressGPR);
            
            AssemblyHelpers::Jump positive = m_jit.branch32(AssemblyHelpers::GreaterThanOrEqual, recovery.gpr(), AssemblyHelpers::TrustedImm32(0));
            
            m_jit.convertInt32ToDouble(recovery.gpr(), FPRInfo::fpRegT0);
            m_jit.addDouble(AssemblyHelpers::AbsoluteAddress(&AssemblyHelpers::twoToThe32), FPRInfo::fpRegT0);
            if (operands.isVariable(index) && poisonedVirtualRegisters[operands.variableForIndex(index)]) {
                m_jit.move(AssemblyHelpers::TrustedImmPtr(scratchDataBuffer + currentPoisonIndex), addressGPR);
                m_jit.storeDouble(FPRInfo::fpRegT0, addressGPR);
            } else
                m_jit.storeDouble(FPRInfo::fpRegT0, AssemblyHelpers::addressFor((VirtualRegister)operand));
            
            AssemblyHelpers::Jump done = m_jit.jump();
            
            positive.link(&m_jit);
            
            if (operands.isVariable(index) && poisonedVirtualRegisters[operands.variableForIndex(index)]) {
                m_jit.store32(recovery.gpr(), reinterpret_cast<char*>(scratchDataBuffer + currentPoisonIndex) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
                m_jit.store32(AssemblyHelpers::TrustedImm32(JSValue::Int32Tag), reinterpret_cast<char*>(scratchDataBuffer + currentPoisonIndex) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
            } else {
                m_jit.store32(recovery.gpr(), AssemblyHelpers::payloadFor((VirtualRegister)operand));
                m_jit.store32(AssemblyHelpers::TrustedImm32(JSValue::Int32Tag), AssemblyHelpers::tagFor((VirtualRegister)operand));
            }
            
            done.link(&m_jit);
            
            m_jit.move(AssemblyHelpers::TrustedImmPtr(myScratch + 1), addressGPR);
            m_jit.loadDouble(addressGPR, FPRInfo::fpRegT0);
            m_jit.loadPtr(myScratch, addressGPR);
                              
            if (operands.isVariable(index) && poisonedVirtualRegisters[operands.variableForIndex(index)]) {
                m_poisonScratchIndices[operands.variableForIndex(index)] = currentPoisonIndex;
                currentPoisonIndex++;
            }
            break;
        }
        default:
            break;
        }
    }
    
    // 7) Dump all doubles into the stack, or to the scratch storage if the
    //    destination virtual register is poisoned.
    if (haveFPRs) {
        for (size_t index = 0; index < operands.size(); ++index) {
            const ValueRecovery& recovery = operands[index];
            if (recovery.technique() != InFPR)
                continue;
            if (operands.isVariable(index) && poisonedVirtualRegisters[operands.variableForIndex(index)]) {
                m_jit.storeDouble(recovery.fpr(), scratchDataBuffer + currentPoisonIndex);
                m_poisonScratchIndices[operands.variableForIndex(index)] = currentPoisonIndex;
                currentPoisonIndex++;
            } else
                m_jit.storeDouble(recovery.fpr(), AssemblyHelpers::addressFor((VirtualRegister)operands.operandForIndex(index)));
        }
    }
    
    // At this point all GPRs are available for scratch use.
    
    ASSERT(currentPoisonIndex == numberOfPoisonedVirtualRegisters);
    
    // 8) Reshuffle displaced virtual registers. Optimize for the case that
    //    the number of displaced virtual registers is not more than the number
    //    of available physical registers.
    
    if (numberOfDisplacedVirtualRegisters) {
        if (numberOfDisplacedVirtualRegisters * 2 <= GPRInfo::numberOfRegisters) {
            // So far this appears to be the case that triggers all the time, but
            // that is far from guaranteed.
        
            unsigned displacementIndex = 0;
            for (size_t index = 0; index < operands.size(); ++index) {
                const ValueRecovery& recovery = operands[index];
                switch (recovery.technique()) {
                case DisplacedInJSStack:
                    m_jit.load32(AssemblyHelpers::payloadFor(recovery.virtualRegister()), GPRInfo::toRegister(displacementIndex++));
                    m_jit.load32(AssemblyHelpers::tagFor(recovery.virtualRegister()), GPRInfo::toRegister(displacementIndex++));
                    break;
                case Int32DisplacedInJSStack:
                    m_jit.load32(AssemblyHelpers::payloadFor(recovery.virtualRegister()), GPRInfo::toRegister(displacementIndex++));
                    m_jit.move(AssemblyHelpers::TrustedImm32(JSValue::Int32Tag), GPRInfo::toRegister(displacementIndex++));
                    break;
                case CellDisplacedInJSStack:
                    m_jit.load32(AssemblyHelpers::payloadFor(recovery.virtualRegister()), GPRInfo::toRegister(displacementIndex++));
                    m_jit.move(AssemblyHelpers::TrustedImm32(JSValue::CellTag), GPRInfo::toRegister(displacementIndex++));
                    break;
                case BooleanDisplacedInJSStack:
                    m_jit.load32(AssemblyHelpers::payloadFor(recovery.virtualRegister()), GPRInfo::toRegister(displacementIndex++));
                    m_jit.move(AssemblyHelpers::TrustedImm32(JSValue::BooleanTag), GPRInfo::toRegister(displacementIndex++));
                    break;
                default:
                    break;
                }
            }
        
            displacementIndex = 0;
            for (size_t index = 0; index < operands.size(); ++index) {
                const ValueRecovery& recovery = operands[index];
                switch (recovery.technique()) {
                case DisplacedInJSStack:
                case Int32DisplacedInJSStack:
                case CellDisplacedInJSStack:
                case BooleanDisplacedInJSStack:
                    m_jit.store32(GPRInfo::toRegister(displacementIndex++), AssemblyHelpers::payloadFor((VirtualRegister)operands.operandForIndex(index)));
                    m_jit.store32(GPRInfo::toRegister(displacementIndex++), AssemblyHelpers::tagFor((VirtualRegister)operands.operandForIndex(index)));
                    break;
                default:
                    break;
                }
            }
        } else {
            // FIXME: This should use the shuffling algorithm that we use
            // for speculative->non-speculative jumps, if we ever discover that
            // some hot code with lots of live values that get displaced and
            // spilled really enjoys frequently failing speculation.
        
            // For now this code is engineered to be correct but probably not
            // super. In particular, it correctly handles cases where for example
            // the displacements are a permutation of the destination values, like
            //
            // 1 -> 2
            // 2 -> 1
            //
            // It accomplishes this by simply lifting all of the virtual registers
            // from their old (DFG JIT) locations and dropping them in a scratch
            // location in memory, and then transferring from that scratch location
            // to their new (old JIT) locations.
        
            unsigned scratchIndex = numberOfPoisonedVirtualRegisters;
            for (size_t index = 0; index < operands.size(); ++index) {
                const ValueRecovery& recovery = operands[index];
                switch (recovery.technique()) {
                case DisplacedInJSStack:
                    m_jit.load32(AssemblyHelpers::payloadFor(recovery.virtualRegister()), GPRInfo::regT0);
                    m_jit.load32(AssemblyHelpers::tagFor(recovery.virtualRegister()), GPRInfo::regT1);
                    m_jit.store32(GPRInfo::regT0, reinterpret_cast<char*>(scratchDataBuffer + scratchIndex) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
                    m_jit.store32(GPRInfo::regT1, reinterpret_cast<char*>(scratchDataBuffer + scratchIndex) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
                    scratchIndex++;
                    break;
                case Int32DisplacedInJSStack:
                case CellDisplacedInJSStack:
                case BooleanDisplacedInJSStack:
                    m_jit.load32(AssemblyHelpers::payloadFor(recovery.virtualRegister()), GPRInfo::regT0);
                    m_jit.store32(GPRInfo::regT0, reinterpret_cast<char*>(scratchDataBuffer + scratchIndex++) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
                    break;
                default:
                    break;
                }
            }
        
            scratchIndex = numberOfPoisonedVirtualRegisters;
            for (size_t index = 0; index < operands.size(); ++index) {
                const ValueRecovery& recovery = operands[index];
                switch (recovery.technique()) {
                case DisplacedInJSStack:
                    m_jit.load32(reinterpret_cast<char*>(scratchDataBuffer + scratchIndex) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload), GPRInfo::regT0);
                    m_jit.load32(reinterpret_cast<char*>(scratchDataBuffer + scratchIndex) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag), GPRInfo::regT1);
                    m_jit.store32(GPRInfo::regT0, AssemblyHelpers::payloadFor((VirtualRegister)operands.operandForIndex(index)));
                    m_jit.store32(GPRInfo::regT1, AssemblyHelpers::tagFor((VirtualRegister)operands.operandForIndex(index)));
                    scratchIndex++;
                    break;
                case Int32DisplacedInJSStack:
                    m_jit.load32(reinterpret_cast<char*>(scratchDataBuffer + scratchIndex++) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload), GPRInfo::regT0);
                    m_jit.store32(AssemblyHelpers::TrustedImm32(JSValue::Int32Tag), AssemblyHelpers::tagFor((VirtualRegister)operands.operandForIndex(index)));
                    m_jit.store32(GPRInfo::regT0, AssemblyHelpers::payloadFor((VirtualRegister)operands.operandForIndex(index)));
                    break;
                case CellDisplacedInJSStack:
                    m_jit.load32(reinterpret_cast<char*>(scratchDataBuffer + scratchIndex++) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload), GPRInfo::regT0);
                    m_jit.store32(AssemblyHelpers::TrustedImm32(JSValue::CellTag), AssemblyHelpers::tagFor((VirtualRegister)operands.operandForIndex(index)));
                    m_jit.store32(GPRInfo::regT0, AssemblyHelpers::payloadFor((VirtualRegister)operands.operandForIndex(index)));
                    break;
                case BooleanDisplacedInJSStack:
                    m_jit.load32(reinterpret_cast<char*>(scratchDataBuffer + scratchIndex++) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload), GPRInfo::regT0);
                    m_jit.store32(AssemblyHelpers::TrustedImm32(JSValue::BooleanTag), AssemblyHelpers::tagFor((VirtualRegister)operands.operandForIndex(index)));
                    m_jit.store32(GPRInfo::regT0, AssemblyHelpers::payloadFor((VirtualRegister)operands.operandForIndex(index)));
                    break;
                default:
                    break;
                }
            }
        
            ASSERT(scratchIndex == numberOfPoisonedVirtualRegisters + numberOfDisplacedVirtualRegisters);
        }
    }
    
    // 9) Dump all poisoned virtual registers.
    
    if (numberOfPoisonedVirtualRegisters) {
        for (int virtualRegister = 0; virtualRegister < (int)operands.numberOfLocals(); ++virtualRegister) {
            if (!poisonedVirtualRegisters[virtualRegister])
                continue;
            
            const ValueRecovery& recovery = operands.local(virtualRegister);
            switch (recovery.technique()) {
            case InGPR:
            case UnboxedInt32InGPR:
            case UnboxedBooleanInGPR: {
                m_jit.load32(reinterpret_cast<char*>(scratchDataBuffer + poisonIndex(virtualRegister)) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload), GPRInfo::regT0);
                m_jit.store32(GPRInfo::regT0, AssemblyHelpers::payloadFor((VirtualRegister)virtualRegister));
                uint32_t tag = JSValue::EmptyValueTag;
                if (recovery.technique() == InGPR)
                    tag = JSValue::CellTag;
                else if (recovery.technique() == UnboxedInt32InGPR)
                    tag = JSValue::Int32Tag;
                else
                    tag = JSValue::BooleanTag;
                m_jit.store32(AssemblyHelpers::TrustedImm32(tag), AssemblyHelpers::tagFor((VirtualRegister)virtualRegister));
                break;
            }

            case InFPR:
            case InPair:
            case UInt32InGPR:
                m_jit.load32(reinterpret_cast<char*>(scratchDataBuffer + poisonIndex(virtualRegister)) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload), GPRInfo::regT0);
                m_jit.load32(reinterpret_cast<char*>(scratchDataBuffer + poisonIndex(virtualRegister)) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag), GPRInfo::regT1);
                m_jit.store32(GPRInfo::regT0, AssemblyHelpers::payloadFor((VirtualRegister)virtualRegister));
                m_jit.store32(GPRInfo::regT1, AssemblyHelpers::tagFor((VirtualRegister)virtualRegister));
                break;
                
            default:
                break;
            }
        }
    }
    
    // 10) Dump all constants. Optimize for Undefined, since that's a constant we see
    //     often.

    if (haveConstants) {
        if (haveUndefined) {
            m_jit.move(AssemblyHelpers::TrustedImm32(jsUndefined().payload()), GPRInfo::regT0);
            m_jit.move(AssemblyHelpers::TrustedImm32(jsUndefined().tag()), GPRInfo::regT1);
        }
        
        for (size_t index = 0; index < operands.size(); ++index) {
            const ValueRecovery& recovery = operands[index];
            if (recovery.technique() != Constant)
                continue;
            if (recovery.constant().isUndefined()) {
                m_jit.store32(GPRInfo::regT0, AssemblyHelpers::payloadFor((VirtualRegister)operands.operandForIndex(index)));
                m_jit.store32(GPRInfo::regT1, AssemblyHelpers::tagFor((VirtualRegister)operands.operandForIndex(index)));
            } else {
                m_jit.store32(AssemblyHelpers::TrustedImm32(recovery.constant().payload()), AssemblyHelpers::payloadFor((VirtualRegister)operands.operandForIndex(index)));
                m_jit.store32(AssemblyHelpers::TrustedImm32(recovery.constant().tag()), AssemblyHelpers::tagFor((VirtualRegister)operands.operandForIndex(index)));
            }
        }
    }
    
    // 12) Adjust the old JIT's execute counter. Since we are exiting OSR, we know
    //     that all new calls into this code will go to the new JIT, so the execute
    //     counter only affects call frames that performed OSR exit and call frames
    //     that were still executing the old JIT at the time of another call frame's
    //     OSR exit. We want to ensure that the following is true:
    //
    //     (a) Code the performs an OSR exit gets a chance to reenter optimized
    //         code eventually, since optimized code is faster. But we don't
    //         want to do such reentery too aggressively (see (c) below).
    //
    //     (b) If there is code on the call stack that is still running the old
    //         JIT's code and has never OSR'd, then it should get a chance to
    //         perform OSR entry despite the fact that we've exited.
    //
    //     (c) Code the performs an OSR exit should not immediately retry OSR
    //         entry, since both forms of OSR are expensive. OSR entry is
    //         particularly expensive.
    //
    //     (d) Frequent OSR failures, even those that do not result in the code
    //         running in a hot loop, result in recompilation getting triggered.
    //
    //     To ensure (c), we'd like to set the execute counter to
    //     counterValueForOptimizeAfterWarmUp(). This seems like it would endanger
    //     (a) and (b), since then every OSR exit would delay the opportunity for
    //     every call frame to perform OSR entry. Essentially, if OSR exit happens
    //     frequently and the function has few loops, then the counter will never
    //     become non-negative and OSR entry will never be triggered. OSR entry
    //     will only happen if a loop gets hot in the old JIT, which does a pretty
    //     good job of ensuring (a) and (b). But that doesn't take care of (d),
    //     since each speculation failure would reset the execute counter.
    //     So we check here if the number of speculation failures is significantly
    //     larger than the number of successes (we want 90% success rate), and if
    //     there have been a large enough number of failures. If so, we set the
    //     counter to 0; otherwise we set the counter to
    //     counterValueForOptimizeAfterWarmUp().
    
    handleExitCounts(exit);
    
    // 13) Reify inlined call frames.
    
    ASSERT(m_jit.baselineCodeBlock()->getJITType() == JITCode::BaselineJIT);
    m_jit.storePtr(AssemblyHelpers::TrustedImmPtr(m_jit.baselineCodeBlock()), AssemblyHelpers::addressFor((VirtualRegister)JSStack::CodeBlock));
    
    for (CodeOrigin codeOrigin = exit.m_codeOrigin; codeOrigin.inlineCallFrame; codeOrigin = codeOrigin.inlineCallFrame->caller) {
        InlineCallFrame* inlineCallFrame = codeOrigin.inlineCallFrame;
        CodeBlock* baselineCodeBlock = m_jit.baselineCodeBlockFor(codeOrigin);
        CodeBlock* baselineCodeBlockForCaller = m_jit.baselineCodeBlockFor(inlineCallFrame->caller);
        Vector<BytecodeAndMachineOffset>& decodedCodeMap = m_jit.decodedCodeMapFor(baselineCodeBlockForCaller);
        unsigned returnBytecodeIndex = inlineCallFrame->caller.bytecodeIndex + OPCODE_LENGTH(op_call);
        BytecodeAndMachineOffset* mapping = binarySearch<BytecodeAndMachineOffset, unsigned>(decodedCodeMap, decodedCodeMap.size(), returnBytecodeIndex, BytecodeAndMachineOffset::getBytecodeIndex);
        
        ASSERT(mapping);
        ASSERT(mapping->m_bytecodeIndex == returnBytecodeIndex);
        
        void* jumpTarget = baselineCodeBlockForCaller->getJITCode().executableAddressAtOffset(mapping->m_machineCodeOffset);

        GPRReg callerFrameGPR;
        if (inlineCallFrame->caller.inlineCallFrame) {
            m_jit.add32(AssemblyHelpers::TrustedImm32(inlineCallFrame->caller.inlineCallFrame->stackOffset * sizeof(EncodedJSValue)), GPRInfo::callFrameRegister, GPRInfo::regT3);
            callerFrameGPR = GPRInfo::regT3;
        } else
            callerFrameGPR = GPRInfo::callFrameRegister;
        
        m_jit.storePtr(AssemblyHelpers::TrustedImmPtr(baselineCodeBlock), AssemblyHelpers::addressFor((VirtualRegister)(inlineCallFrame->stackOffset + JSStack::CodeBlock)));
        m_jit.store32(AssemblyHelpers::TrustedImm32(JSValue::CellTag), AssemblyHelpers::tagFor((VirtualRegister)(inlineCallFrame->stackOffset + JSStack::ScopeChain)));
        if (!inlineCallFrame->isClosureCall())
            m_jit.storePtr(AssemblyHelpers::TrustedImmPtr(inlineCallFrame->callee->scope()), AssemblyHelpers::payloadFor((VirtualRegister)(inlineCallFrame->stackOffset + JSStack::ScopeChain)));
        m_jit.store32(AssemblyHelpers::TrustedImm32(JSValue::CellTag), AssemblyHelpers::tagFor((VirtualRegister)(inlineCallFrame->stackOffset + JSStack::CallerFrame)));
        m_jit.storePtr(callerFrameGPR, AssemblyHelpers::payloadFor((VirtualRegister)(inlineCallFrame->stackOffset + JSStack::CallerFrame)));
        m_jit.storePtr(AssemblyHelpers::TrustedImmPtr(jumpTarget), AssemblyHelpers::payloadFor((VirtualRegister)(inlineCallFrame->stackOffset + JSStack::ReturnPC)));
        m_jit.store32(AssemblyHelpers::TrustedImm32(inlineCallFrame->arguments.size()), AssemblyHelpers::payloadFor((VirtualRegister)(inlineCallFrame->stackOffset + JSStack::ArgumentCount)));
        m_jit.store32(AssemblyHelpers::TrustedImm32(JSValue::CellTag), AssemblyHelpers::tagFor((VirtualRegister)(inlineCallFrame->stackOffset + JSStack::Callee)));
        if (!inlineCallFrame->isClosureCall())
            m_jit.storePtr(AssemblyHelpers::TrustedImmPtr(inlineCallFrame->callee.get()), AssemblyHelpers::payloadFor((VirtualRegister)(inlineCallFrame->stackOffset + JSStack::Callee)));
    }
    
    // 14) Create arguments if necessary and place them into the appropriate aliased
    //     registers.
    
    if (haveArguments) {
        HashSet<InlineCallFrame*, DefaultHash<InlineCallFrame*>::Hash,
            NullableHashTraits<InlineCallFrame*> > didCreateArgumentsObject;

        for (size_t index = 0; index < operands.size(); ++index) {
            const ValueRecovery& recovery = operands[index];
            if (recovery.technique() != ArgumentsThatWereNotCreated)
                continue;
            int operand = operands.operandForIndex(index);
            // Find the right inline call frame.
            InlineCallFrame* inlineCallFrame = 0;
            for (InlineCallFrame* current = exit.m_codeOrigin.inlineCallFrame;
                 current;
                 current = current->caller.inlineCallFrame) {
                if (current->stackOffset <= operand) {
                    inlineCallFrame = current;
                    break;
                }
            }

            if (!m_jit.baselineCodeBlockFor(inlineCallFrame)->usesArguments())
                continue;
            int argumentsRegister = m_jit.argumentsRegisterFor(inlineCallFrame);
            if (didCreateArgumentsObject.add(inlineCallFrame).isNewEntry) {
                // We know this call frame optimized out an arguments object that
                // the baseline JIT would have created. Do that creation now.
                if (inlineCallFrame) {
                    m_jit.setupArgumentsWithExecState(
                        AssemblyHelpers::TrustedImmPtr(inlineCallFrame));
                    m_jit.move(
                        AssemblyHelpers::TrustedImmPtr(
                            bitwise_cast<void*>(operationCreateInlinedArguments)),
                        GPRInfo::nonArgGPR0);
                } else {
                    m_jit.setupArgumentsExecState();
                    m_jit.move(
                        AssemblyHelpers::TrustedImmPtr(
                            bitwise_cast<void*>(operationCreateArguments)),
                        GPRInfo::nonArgGPR0);
                }
                m_jit.call(GPRInfo::nonArgGPR0);
                m_jit.store32(
                    AssemblyHelpers::TrustedImm32(JSValue::CellTag),
                    AssemblyHelpers::tagFor(argumentsRegister));
                m_jit.store32(
                    GPRInfo::returnValueGPR,
                    AssemblyHelpers::payloadFor(argumentsRegister));
                m_jit.store32(
                    AssemblyHelpers::TrustedImm32(JSValue::CellTag),
                    AssemblyHelpers::tagFor(unmodifiedArgumentsRegister(argumentsRegister)));
                m_jit.store32(
                    GPRInfo::returnValueGPR,
                    AssemblyHelpers::payloadFor(unmodifiedArgumentsRegister(argumentsRegister)));
                m_jit.move(GPRInfo::returnValueGPR, GPRInfo::regT0); // no-op move on almost all platforms.
            }

            m_jit.load32(AssemblyHelpers::payloadFor(argumentsRegister), GPRInfo::regT0);
            m_jit.store32(
                AssemblyHelpers::TrustedImm32(JSValue::CellTag),
                AssemblyHelpers::tagFor(operand));
            m_jit.store32(GPRInfo::regT0, AssemblyHelpers::payloadFor(operand));
        }
    }
    
    // 15) Load the result of the last bytecode operation into regT0.
    
    if (exit.m_lastSetOperand != std::numeric_limits<int>::max()) {
        m_jit.load32(AssemblyHelpers::payloadFor((VirtualRegister)exit.m_lastSetOperand), GPRInfo::cachedResultRegister);
        m_jit.load32(AssemblyHelpers::tagFor((VirtualRegister)exit.m_lastSetOperand), GPRInfo::cachedResultRegister2);
    }
    
    // 16) Adjust the call frame pointer.
    
    if (exit.m_codeOrigin.inlineCallFrame)
        m_jit.addPtr(AssemblyHelpers::TrustedImm32(exit.m_codeOrigin.inlineCallFrame->stackOffset * sizeof(EncodedJSValue)), GPRInfo::callFrameRegister);

    // 17) Jump into the corresponding baseline JIT code.
    
    CodeBlock* baselineCodeBlock = m_jit.baselineCodeBlockFor(exit.m_codeOrigin);
    Vector<BytecodeAndMachineOffset>& decodedCodeMap = m_jit.decodedCodeMapFor(baselineCodeBlock);
    
    BytecodeAndMachineOffset* mapping = binarySearch<BytecodeAndMachineOffset, unsigned>(decodedCodeMap, decodedCodeMap.size(), exit.m_codeOrigin.bytecodeIndex, BytecodeAndMachineOffset::getBytecodeIndex);
    
    ASSERT(mapping);
    ASSERT(mapping->m_bytecodeIndex == exit.m_codeOrigin.bytecodeIndex);
    
    void* jumpTarget = baselineCodeBlock->getJITCode().executableAddressAtOffset(mapping->m_machineCodeOffset);
    
    ASSERT(GPRInfo::regT2 != GPRInfo::cachedResultRegister && GPRInfo::regT2 != GPRInfo::cachedResultRegister2);
    
    m_jit.move(AssemblyHelpers::TrustedImmPtr(jumpTarget), GPRInfo::regT2);
    m_jit.jump(GPRInfo::regT2);

#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLogF("   -> %p\n", jumpTarget);
#endif
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT) && USE(JSVALUE32_64)
