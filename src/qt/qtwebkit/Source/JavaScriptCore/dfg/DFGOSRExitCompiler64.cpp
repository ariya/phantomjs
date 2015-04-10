/*
 * Copyright (C) 2011, 2013 Apple Inc. All rights reserved.
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

#if ENABLE(DFG_JIT) && USE(JSVALUE64)

#include "DFGOperations.h"
#include "Operations.h"
#include <wtf/DataLog.h>

namespace JSC { namespace DFG {

void OSRExitCompiler::compileExit(const OSRExit& exit, const Operands<ValueRecovery>& operands, SpeculationRecovery* recovery)
{
    // 1) Pro-forma stuff.
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLogF("OSR exit for (");
    for (CodeOrigin codeOrigin = exit.m_codeOrigin; ; codeOrigin = codeOrigin.inlineCallFrame->caller) {
        dataLogF("bc#%u", codeOrigin.bytecodeIndex);
        if (!codeOrigin.inlineCallFrame)
            break;
        dataLogF(" -> %p ", codeOrigin.inlineCallFrame->executable.get());
    }
    dataLogF(")  ");
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
    
    GPRReg alreadyBoxed = InvalidGPRReg;
    
    if (recovery) {
        switch (recovery->type()) {
        case SpeculativeAdd:
            m_jit.sub32(recovery->src(), recovery->dest());
            m_jit.or64(GPRInfo::tagTypeNumberRegister, recovery->dest());
            alreadyBoxed = recovery->dest();
            break;
            
        case BooleanSpeculationCheck:
            m_jit.xor64(AssemblyHelpers::TrustedImm32(static_cast<int32_t>(ValueFalse)), recovery->dest());
            break;
            
        default:
            break;
        }
    }

    // 3) Refine some array and/or value profile, if appropriate.
    
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
            
            CodeOrigin codeOrigin = exit.m_codeOriginForExitProfile;
            if (ArrayProfile* arrayProfile = m_jit.baselineCodeBlockFor(codeOrigin)->getArrayProfile(codeOrigin.bytecodeIndex)) {
                GPRReg usedRegister;
                if (exit.m_jsValueSource.isAddress())
                    usedRegister = exit.m_jsValueSource.base();
                else
                    usedRegister = exit.m_jsValueSource.gpr();
                
                GPRReg scratch1;
                GPRReg scratch2;
                scratch1 = AssemblyHelpers::selectScratchGPR(usedRegister);
                scratch2 = AssemblyHelpers::selectScratchGPR(usedRegister, scratch1);
                
                m_jit.push(scratch1);
                m_jit.push(scratch2);
                
                GPRReg value;
                if (exit.m_jsValueSource.isAddress()) {
                    value = scratch1;
                    m_jit.loadPtr(AssemblyHelpers::Address(exit.m_jsValueSource.asAddress()), value);
                } else
                    value = exit.m_jsValueSource.gpr();
                
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
                // We can't be sure that we have a spare register. So use the tagTypeNumberRegister,
                // since we know how to restore it.
                m_jit.load64(AssemblyHelpers::Address(exit.m_jsValueSource.asAddress()), GPRInfo::tagTypeNumberRegister);
                m_jit.store64(GPRInfo::tagTypeNumberRegister, bucket);
                m_jit.move(AssemblyHelpers::TrustedImm64(TagTypeNumber), GPRInfo::tagTypeNumberRegister);
            } else
                m_jit.store64(exit.m_jsValueSource.gpr(), bucket);
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
    bool haveUnboxedInt32s = false;
    bool haveUnboxedDoubles = false;
    bool haveFPRs = false;
    bool haveConstants = false;
    bool haveUndefined = false;
    bool haveUInt32s = false;
    bool haveArguments = false;
    
    for (size_t index = 0; index < operands.size(); ++index) {
        const ValueRecovery& recovery = operands[index];
        switch (recovery.technique()) {
        case Int32DisplacedInJSStack:
        case DoubleDisplacedInJSStack:
        case DisplacedInJSStack:
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
                case UInt32InGPR:
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
            
        case UnboxedInt32InGPR:
        case AlreadyInJSStackAsUnboxedInt32:
            haveUnboxedInt32s = true;
            break;
            
        case AlreadyInJSStackAsUnboxedDouble:
            haveUnboxedDoubles = true;
            break;
            
        case UInt32InGPR:
            haveUInt32s = true;
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
    
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLogF("  ");
    if (numberOfPoisonedVirtualRegisters)
        dataLogF("Poisoned=%u ", numberOfPoisonedVirtualRegisters);
    if (numberOfDisplacedVirtualRegisters)
        dataLogF("Displaced=%u ", numberOfDisplacedVirtualRegisters);
    if (haveUnboxedInt32s)
        dataLogF("UnboxedInt32 ");
    if (haveUnboxedDoubles)
        dataLogF("UnboxedDoubles ");
    if (haveUInt32s)
        dataLogF("UInt32 ");
    if (haveFPRs)
        dataLogF("FPR ");
    if (haveConstants)
        dataLogF("Constants ");
    if (haveUndefined)
        dataLogF("Undefined ");
    dataLogF(" ");
#endif
    
    ScratchBuffer* scratchBuffer = m_jit.vm()->scratchBufferForSize(sizeof(EncodedJSValue) * std::max(haveUInt32s ? 2u : 0u, numberOfPoisonedVirtualRegisters + (numberOfDisplacedVirtualRegisters <= GPRInfo::numberOfRegisters ? 0 : numberOfDisplacedVirtualRegisters)));
    EncodedJSValue* scratchDataBuffer = scratchBuffer ? static_cast<EncodedJSValue*>(scratchBuffer->dataBuffer()) : 0;

    // From here on, the code assumes that it is profitable to maximize the distance
    // between when something is computed and when it is stored.
    
    // 5) Perform all reboxing of integers.
    
    if (haveUnboxedInt32s || haveUInt32s) {
        for (size_t index = 0; index < operands.size(); ++index) {
            const ValueRecovery& recovery = operands[index];
            switch (recovery.technique()) {
            case UnboxedInt32InGPR:
                if (recovery.gpr() != alreadyBoxed)
                    m_jit.or64(GPRInfo::tagTypeNumberRegister, recovery.gpr());
                break;
                
            case AlreadyInJSStackAsUnboxedInt32:
                m_jit.store32(AssemblyHelpers::TrustedImm32(static_cast<uint32_t>(TagTypeNumber >> 32)), AssemblyHelpers::tagFor(static_cast<VirtualRegister>(operands.operandForIndex(index))));
                break;
                
            case UInt32InGPR: {
                // This occurs when the speculative JIT left an unsigned 32-bit integer
                // in a GPR. If it's positive, we can just box the int. Otherwise we
                // need to turn it into a boxed double.
                
                // We don't try to be clever with register allocation here; we assume
                // that the program is using FPRs and we don't try to figure out which
                // ones it is using. Instead just temporarily save fpRegT0 and then
                // restore it. This makes sense because this path is not cheap to begin
                // with, and should happen very rarely.
                
                GPRReg addressGPR = GPRInfo::regT0;
                if (addressGPR == recovery.gpr())
                    addressGPR = GPRInfo::regT1;
                
                m_jit.store64(addressGPR, scratchDataBuffer);
                m_jit.move(AssemblyHelpers::TrustedImmPtr(scratchDataBuffer + 1), addressGPR);
                m_jit.storeDouble(FPRInfo::fpRegT0, addressGPR);
                
                AssemblyHelpers::Jump positive = m_jit.branch32(AssemblyHelpers::GreaterThanOrEqual, recovery.gpr(), AssemblyHelpers::TrustedImm32(0));

                m_jit.convertInt32ToDouble(recovery.gpr(), FPRInfo::fpRegT0);
                m_jit.addDouble(AssemblyHelpers::AbsoluteAddress(&AssemblyHelpers::twoToThe32), FPRInfo::fpRegT0);
                m_jit.boxDouble(FPRInfo::fpRegT0, recovery.gpr());
                
                AssemblyHelpers::Jump done = m_jit.jump();
                
                positive.link(&m_jit);
                
                m_jit.or64(GPRInfo::tagTypeNumberRegister, recovery.gpr());
                
                done.link(&m_jit);
                
                m_jit.loadDouble(addressGPR, FPRInfo::fpRegT0);
                m_jit.load64(scratchDataBuffer, addressGPR);
                break;
            }
                
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
        case UInt32InGPR:
            if (operands.isVariable(index) && poisonedVirtualRegisters[operands.variableForIndex(index)]) {
                m_jit.store64(recovery.gpr(), scratchDataBuffer + currentPoisonIndex);
                m_poisonScratchIndices[operands.variableForIndex(index)] = currentPoisonIndex;
                currentPoisonIndex++;
            } else
                m_jit.store64(recovery.gpr(), AssemblyHelpers::addressFor((VirtualRegister)operand));
            break;
        default:
            break;
        }
    }
    
    // At this point all GPRs are available for scratch use.
    
    if (haveFPRs) {
        // 7) Box all doubles (relies on there being more GPRs than FPRs)
        
        for (size_t index = 0; index < operands.size(); ++index) {
            const ValueRecovery& recovery = operands[index];
            if (recovery.technique() != InFPR)
                continue;
            FPRReg fpr = recovery.fpr();
            GPRReg gpr = GPRInfo::toRegister(FPRInfo::toIndex(fpr));
            m_jit.boxDouble(fpr, gpr);
        }
        
        // 8) Dump all doubles into the stack, or to the scratch storage if
        //    the destination virtual register is poisoned.
        
        for (size_t index = 0; index < operands.size(); ++index) {
            const ValueRecovery& recovery = operands[index];
            if (recovery.technique() != InFPR)
                continue;
            GPRReg gpr = GPRInfo::toRegister(FPRInfo::toIndex(recovery.fpr()));
            if (operands.isVariable(index) && poisonedVirtualRegisters[operands.variableForIndex(index)]) {
                m_jit.store64(gpr, scratchDataBuffer + currentPoisonIndex);
                m_poisonScratchIndices[operands.variableForIndex(index)] = currentPoisonIndex;
                currentPoisonIndex++;
            } else
                m_jit.store64(gpr, AssemblyHelpers::addressFor((VirtualRegister)operands.operandForIndex(index)));
        }
    }
    
    // At this point all GPRs and FPRs are available for scratch use.
    
    // 9) Box all unboxed doubles in the stack.
    if (haveUnboxedDoubles) {
        for (size_t index = 0; index < operands.size(); ++index) {
            const ValueRecovery& recovery = operands[index];
            if (recovery.technique() != AlreadyInJSStackAsUnboxedDouble)
                continue;
            m_jit.loadDouble(AssemblyHelpers::addressFor((VirtualRegister)operands.operandForIndex(index)), FPRInfo::fpRegT0);
            m_jit.boxDouble(FPRInfo::fpRegT0, GPRInfo::regT0);
            m_jit.store64(GPRInfo::regT0, AssemblyHelpers::addressFor((VirtualRegister)operands.operandForIndex(index)));
        }
    }
    
    ASSERT(currentPoisonIndex == numberOfPoisonedVirtualRegisters);
    
    // 10) Reshuffle displaced virtual registers. Optimize for the case that
    //    the number of displaced virtual registers is not more than the number
    //    of available physical registers.
    
    if (numberOfDisplacedVirtualRegisters) {
        if (numberOfDisplacedVirtualRegisters <= GPRInfo::numberOfRegisters) {
            // So far this appears to be the case that triggers all the time, but
            // that is far from guaranteed.
        
            unsigned displacementIndex = 0;
            for (size_t index = 0; index < operands.size(); ++index) {
                const ValueRecovery& recovery = operands[index];
                switch (recovery.technique()) {
                case DisplacedInJSStack:
                    m_jit.load64(AssemblyHelpers::addressFor(recovery.virtualRegister()), GPRInfo::toRegister(displacementIndex++));
                    break;
                    
                case Int32DisplacedInJSStack: {
                    GPRReg gpr = GPRInfo::toRegister(displacementIndex++);
                    m_jit.load32(AssemblyHelpers::addressFor(recovery.virtualRegister()), gpr);
                    m_jit.or64(GPRInfo::tagTypeNumberRegister, gpr);
                    break;
                }
                    
                case DoubleDisplacedInJSStack: {
                    GPRReg gpr = GPRInfo::toRegister(displacementIndex++);
                    m_jit.load64(AssemblyHelpers::addressFor(recovery.virtualRegister()), gpr);
                    m_jit.sub64(GPRInfo::tagTypeNumberRegister, gpr);
                    break;
                }
                    
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
                case DoubleDisplacedInJSStack:
                    m_jit.store64(GPRInfo::toRegister(displacementIndex++), AssemblyHelpers::addressFor((VirtualRegister)operands.operandForIndex(index)));
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
                    m_jit.load64(AssemblyHelpers::addressFor(recovery.virtualRegister()), GPRInfo::regT0);
                    m_jit.store64(GPRInfo::regT0, scratchDataBuffer + scratchIndex++);
                    break;
                    
                case Int32DisplacedInJSStack: {
                    m_jit.load32(AssemblyHelpers::addressFor(recovery.virtualRegister()), GPRInfo::regT0);
                    m_jit.or64(GPRInfo::tagTypeNumberRegister, GPRInfo::regT0);
                    m_jit.store64(GPRInfo::regT0, scratchDataBuffer + scratchIndex++);
                    break;
                }
                    
                case DoubleDisplacedInJSStack: {
                    m_jit.load64(AssemblyHelpers::addressFor(recovery.virtualRegister()), GPRInfo::regT0);
                    m_jit.sub64(GPRInfo::tagTypeNumberRegister, GPRInfo::regT0);
                    m_jit.store64(GPRInfo::regT0, scratchDataBuffer + scratchIndex++);
                    break;
                }
                    
                default:
                    break;
                }
            }
        
            scratchIndex = numberOfPoisonedVirtualRegisters;
            for (size_t index = 0; index < operands.size(); ++index) {
                const ValueRecovery& recovery = operands[index];
                switch (recovery.technique()) {
                case DisplacedInJSStack:
                case Int32DisplacedInJSStack:
                case DoubleDisplacedInJSStack:
                    m_jit.load64(scratchDataBuffer + scratchIndex++, GPRInfo::regT0);
                    m_jit.store64(GPRInfo::regT0, AssemblyHelpers::addressFor((VirtualRegister)operands.operandForIndex(index)));
                    break;
                    
                default:
                    break;
                }
            }
        
            ASSERT(scratchIndex == numberOfPoisonedVirtualRegisters + numberOfDisplacedVirtualRegisters);
        }
    }
    
    // 11) Dump all poisoned virtual registers.
    
    if (numberOfPoisonedVirtualRegisters) {
        for (int virtualRegister = 0; virtualRegister < (int)operands.numberOfLocals(); ++virtualRegister) {
            if (!poisonedVirtualRegisters[virtualRegister])
                continue;
            
            const ValueRecovery& recovery = operands.local(virtualRegister);
            switch (recovery.technique()) {
            case InGPR:
            case UnboxedInt32InGPR:
            case UInt32InGPR:
            case InFPR:
                m_jit.load64(scratchDataBuffer + poisonIndex(virtualRegister), GPRInfo::regT0);
                m_jit.store64(GPRInfo::regT0, AssemblyHelpers::addressFor((VirtualRegister)virtualRegister));
                break;
                
            default:
                break;
            }
        }
    }
    
    // 12) Dump all constants. Optimize for Undefined, since that's a constant we see
    //     often.

    if (haveConstants) {
        if (haveUndefined)
            m_jit.move(AssemblyHelpers::TrustedImm64(JSValue::encode(jsUndefined())), GPRInfo::regT0);
        
        for (size_t index = 0; index < operands.size(); ++index) {
            const ValueRecovery& recovery = operands[index];
            if (recovery.technique() != Constant)
                continue;
            if (recovery.constant().isUndefined())
                m_jit.store64(GPRInfo::regT0, AssemblyHelpers::addressFor((VirtualRegister)operands.operandForIndex(index)));
            else
                m_jit.store64(AssemblyHelpers::TrustedImm64(JSValue::encode(recovery.constant())), AssemblyHelpers::addressFor((VirtualRegister)operands.operandForIndex(index)));
        }
    }
    
    // 13) Adjust the old JIT's execute counter. Since we are exiting OSR, we know
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
    
    // 14) Reify inlined call frames.
    
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
            m_jit.addPtr(AssemblyHelpers::TrustedImm32(inlineCallFrame->caller.inlineCallFrame->stackOffset * sizeof(EncodedJSValue)), GPRInfo::callFrameRegister, GPRInfo::regT3);
            callerFrameGPR = GPRInfo::regT3;
        } else
            callerFrameGPR = GPRInfo::callFrameRegister;
        
        m_jit.storePtr(AssemblyHelpers::TrustedImmPtr(baselineCodeBlock), AssemblyHelpers::addressFor((VirtualRegister)(inlineCallFrame->stackOffset + JSStack::CodeBlock)));
        if (!inlineCallFrame->isClosureCall())
            m_jit.store64(AssemblyHelpers::TrustedImm64(JSValue::encode(JSValue(inlineCallFrame->callee->scope()))), AssemblyHelpers::addressFor((VirtualRegister)(inlineCallFrame->stackOffset + JSStack::ScopeChain)));
        m_jit.store64(callerFrameGPR, AssemblyHelpers::addressFor((VirtualRegister)(inlineCallFrame->stackOffset + JSStack::CallerFrame)));
        m_jit.storePtr(AssemblyHelpers::TrustedImmPtr(jumpTarget), AssemblyHelpers::addressFor((VirtualRegister)(inlineCallFrame->stackOffset + JSStack::ReturnPC)));
        m_jit.store32(AssemblyHelpers::TrustedImm32(inlineCallFrame->arguments.size()), AssemblyHelpers::payloadFor((VirtualRegister)(inlineCallFrame->stackOffset + JSStack::ArgumentCount)));
        if (!inlineCallFrame->isClosureCall())
            m_jit.store64(AssemblyHelpers::TrustedImm64(JSValue::encode(JSValue(inlineCallFrame->callee.get()))), AssemblyHelpers::addressFor((VirtualRegister)(inlineCallFrame->stackOffset + JSStack::Callee)));
    }
    
    // 15) Create arguments if necessary and place them into the appropriate aliased
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
                    m_jit.addPtr(AssemblyHelpers::TrustedImm32(inlineCallFrame->stackOffset * sizeof(EncodedJSValue)), GPRInfo::callFrameRegister, GPRInfo::regT0);
                    m_jit.setupArguments(GPRInfo::regT0);
                } else
                    m_jit.setupArgumentsExecState();
                m_jit.move(
                    AssemblyHelpers::TrustedImmPtr(
                        bitwise_cast<void*>(operationCreateArguments)),
                    GPRInfo::nonArgGPR0);
                m_jit.call(GPRInfo::nonArgGPR0);
                m_jit.store64(GPRInfo::returnValueGPR, AssemblyHelpers::addressFor(argumentsRegister));
                m_jit.store64(
                    GPRInfo::returnValueGPR,
                    AssemblyHelpers::addressFor(unmodifiedArgumentsRegister(argumentsRegister)));
                m_jit.move(GPRInfo::returnValueGPR, GPRInfo::regT0); // no-op move on almost all platforms.
            }

            m_jit.load64(AssemblyHelpers::addressFor(argumentsRegister), GPRInfo::regT0);
            m_jit.store64(GPRInfo::regT0, AssemblyHelpers::addressFor(operand));
        }
    }
    
    // 16) Load the result of the last bytecode operation into regT0.
    
    if (exit.m_lastSetOperand != std::numeric_limits<int>::max())
        m_jit.load64(AssemblyHelpers::addressFor((VirtualRegister)exit.m_lastSetOperand), GPRInfo::cachedResultRegister);
    
    // 17) Adjust the call frame pointer.
    
    if (exit.m_codeOrigin.inlineCallFrame)
        m_jit.addPtr(AssemblyHelpers::TrustedImm32(exit.m_codeOrigin.inlineCallFrame->stackOffset * sizeof(EncodedJSValue)), GPRInfo::callFrameRegister);
    
    // 18) Jump into the corresponding baseline JIT code.
    
    CodeBlock* baselineCodeBlock = m_jit.baselineCodeBlockFor(exit.m_codeOrigin);
    Vector<BytecodeAndMachineOffset>& decodedCodeMap = m_jit.decodedCodeMapFor(baselineCodeBlock);
    
    BytecodeAndMachineOffset* mapping = binarySearch<BytecodeAndMachineOffset, unsigned>(decodedCodeMap, decodedCodeMap.size(), exit.m_codeOrigin.bytecodeIndex, BytecodeAndMachineOffset::getBytecodeIndex);
    
    ASSERT(mapping);
    ASSERT(mapping->m_bytecodeIndex == exit.m_codeOrigin.bytecodeIndex);
    
    void* jumpTarget = baselineCodeBlock->getJITCode().executableAddressAtOffset(mapping->m_machineCodeOffset);
    
    ASSERT(GPRInfo::regT1 != GPRInfo::cachedResultRegister);
    
    m_jit.move(AssemblyHelpers::TrustedImmPtr(jumpTarget), GPRInfo::regT1);
    
    m_jit.jump(GPRInfo::regT1);

#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLogF("-> %p\n", jumpTarget);
#endif
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT) && USE(JSVALUE64)
