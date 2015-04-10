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
#include "DFGOSRExitCompiler.h"

#if ENABLE(DFG_JIT)

#include "CallFrame.h"
#include "DFGCommon.h"
#include "LinkBuffer.h"
#include "Operations.h"
#include "RepatchBuffer.h"
#include <wtf/StringPrintStream.h>

namespace JSC { namespace DFG {

extern "C" {

void compileOSRExit(ExecState* exec)
{
    SamplingRegion samplingRegion("DFG OSR Exit Compilation");
    
    CodeBlock* codeBlock = exec->codeBlock();
    
    ASSERT(codeBlock);
    ASSERT(codeBlock->getJITType() == JITCode::DFGJIT);
    
    VM* vm = &exec->vm();
    
    uint32_t exitIndex = vm->osrExitIndex;
    OSRExit& exit = codeBlock->osrExit(exitIndex);
    
    // Make sure all code on our inline stack is JIT compiled. This is necessary since
    // we may opt to inline a code block even before it had ever been compiled by the
    // JIT, but our OSR exit infrastructure currently only works if the target of the
    // OSR exit is JIT code. This could be changed since there is nothing particularly
    // hard about doing an OSR exit into the interpreter, but for now this seems to make
    // sense in that if we're OSR exiting from inlined code of a DFG code block, then
    // probably it's a good sign that the thing we're exiting into is hot. Even more
    // interestingly, since the code was inlined, it may never otherwise get JIT
    // compiled since the act of inlining it may ensure that it otherwise never runs.
    for (CodeOrigin codeOrigin = exit.m_codeOrigin; codeOrigin.inlineCallFrame; codeOrigin = codeOrigin.inlineCallFrame->caller) {
        static_cast<FunctionExecutable*>(codeOrigin.inlineCallFrame->executable.get())
            ->baselineCodeBlockFor(codeOrigin.inlineCallFrame->isCall ? CodeForCall : CodeForConstruct)
            ->jitCompile(exec);
    }
    
    // Compute the value recoveries.
    Operands<ValueRecovery> operands;
    codeBlock->variableEventStream().reconstruct(codeBlock, exit.m_codeOrigin, codeBlock->minifiedDFG(), exit.m_streamIndex, operands);
    
    // There may be an override, for forward speculations.
    if (!!exit.m_valueRecoveryOverride) {
        operands.setOperand(
            exit.m_valueRecoveryOverride->operand, exit.m_valueRecoveryOverride->recovery);
    }
    
    SpeculationRecovery* recovery = 0;
    if (exit.m_recoveryIndex)
        recovery = &codeBlock->speculationRecovery(exit.m_recoveryIndex - 1);

#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLog(
        "Generating OSR exit #", exitIndex, " (seq#", exit.m_streamIndex,
        ", bc#", exit.m_codeOrigin.bytecodeIndex, ", ",
        exit.m_kind, ") for ", *codeBlock, ".\n");
#endif

    {
        CCallHelpers jit(vm, codeBlock);
        OSRExitCompiler exitCompiler(jit);

        jit.jitAssertHasValidCallFrame();
        
        if (vm->m_perBytecodeProfiler && codeBlock->compilation()) {
            Profiler::Database& database = *vm->m_perBytecodeProfiler;
            Profiler::Compilation* compilation = codeBlock->compilation();
            
            Profiler::OSRExit* profilerExit = compilation->addOSRExit(
                exitIndex, Profiler::OriginStack(database, codeBlock, exit.m_codeOrigin),
                exit.m_kind,
                exit.m_watchpointIndex != std::numeric_limits<unsigned>::max());
            jit.add64(CCallHelpers::TrustedImm32(1), CCallHelpers::AbsoluteAddress(profilerExit->counterAddress()));
        }
        
        exitCompiler.compileExit(exit, operands, recovery);
        
        LinkBuffer patchBuffer(*vm, &jit, codeBlock);
        exit.m_code = FINALIZE_CODE_IF(
            shouldShowDisassembly(),
            patchBuffer,
            ("DFG OSR exit #%u (bc#%u, %s) from %s",
                exitIndex, exit.m_codeOrigin.bytecodeIndex,
                exitKindToString(exit.m_kind), toCString(*codeBlock).data()));
    }
    
    {
        RepatchBuffer repatchBuffer(codeBlock);
        repatchBuffer.relink(exit.codeLocationForRepatch(codeBlock), CodeLocationLabel(exit.m_code.code()));
    }
    
    vm->osrExitJumpDestination = exit.m_code.code().executableAddress();
}

} // extern "C"

void OSRExitCompiler::handleExitCounts(const OSRExit& exit)
{
    m_jit.add32(AssemblyHelpers::TrustedImm32(1), AssemblyHelpers::AbsoluteAddress(&exit.m_count));
    
    m_jit.move(AssemblyHelpers::TrustedImmPtr(m_jit.codeBlock()), GPRInfo::regT0);
    
    AssemblyHelpers::Jump tooFewFails;
    
    m_jit.load32(AssemblyHelpers::Address(GPRInfo::regT0, CodeBlock::offsetOfOSRExitCounter()), GPRInfo::regT2);
    m_jit.add32(AssemblyHelpers::TrustedImm32(1), GPRInfo::regT2);
    m_jit.store32(GPRInfo::regT2, AssemblyHelpers::Address(GPRInfo::regT0, CodeBlock::offsetOfOSRExitCounter()));
    m_jit.move(AssemblyHelpers::TrustedImmPtr(m_jit.baselineCodeBlock()), GPRInfo::regT0);
    tooFewFails = m_jit.branch32(AssemblyHelpers::BelowOrEqual, GPRInfo::regT2, AssemblyHelpers::TrustedImm32(m_jit.codeBlock()->exitCountThresholdForReoptimization()));
    
    // Reoptimize as soon as possible.
#if !NUMBER_OF_ARGUMENT_REGISTERS
    m_jit.poke(GPRInfo::regT0);
#else
    m_jit.move(GPRInfo::regT0, GPRInfo::argumentGPR0);
    ASSERT(GPRInfo::argumentGPR0 != GPRInfo::regT1);
#endif
    m_jit.move(AssemblyHelpers::TrustedImmPtr(bitwise_cast<void*>(triggerReoptimizationNow)), GPRInfo::regT1);
    m_jit.call(GPRInfo::regT1);
    AssemblyHelpers::Jump doneAdjusting = m_jit.jump();
    
    tooFewFails.link(&m_jit);
    
    // Adjust the execution counter such that the target is to only optimize after a while.
    int32_t activeThreshold =
        m_jit.baselineCodeBlock()->counterValueForOptimizeAfterLongWarmUp();
    int32_t targetValue = ExecutionCounter::applyMemoryUsageHeuristicsAndConvertToInt(
        activeThreshold, m_jit.baselineCodeBlock());
    int32_t clippedValue =
        ExecutionCounter::clippedThreshold(m_jit.codeBlock()->globalObject(), targetValue);
    m_jit.store32(AssemblyHelpers::TrustedImm32(-clippedValue), AssemblyHelpers::Address(GPRInfo::regT0, CodeBlock::offsetOfJITExecuteCounter()));
    m_jit.store32(AssemblyHelpers::TrustedImm32(activeThreshold), AssemblyHelpers::Address(GPRInfo::regT0, CodeBlock::offsetOfJITExecutionActiveThreshold()));
    m_jit.store32(AssemblyHelpers::TrustedImm32(ExecutionCounter::formattedTotalCount(clippedValue)), AssemblyHelpers::Address(GPRInfo::regT0, CodeBlock::offsetOfJITExecutionTotalCount()));
    
    doneAdjusting.link(&m_jit);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)
