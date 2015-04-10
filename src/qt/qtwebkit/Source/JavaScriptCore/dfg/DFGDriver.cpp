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
#include "DFGDriver.h"

#include "JSObject.h"
#include "JSString.h"


#if ENABLE(DFG_JIT)

#include "DFGArgumentsSimplificationPhase.h"
#include "DFGBackwardsPropagationPhase.h"
#include "DFGByteCodeParser.h"
#include "DFGCFAPhase.h"
#include "DFGCFGSimplificationPhase.h"
#include "DFGCPSRethreadingPhase.h"
#include "DFGCSEPhase.h"
#include "DFGConstantFoldingPhase.h"
#include "DFGDCEPhase.h"
#include "DFGFixupPhase.h"
#include "DFGJITCompiler.h"
#include "DFGPredictionInjectionPhase.h"
#include "DFGPredictionPropagationPhase.h"
#include "DFGTypeCheckHoistingPhase.h"
#include "DFGUnificationPhase.h"
#include "DFGValidate.h"
#include "DFGVirtualRegisterAllocationPhase.h"
#include "Operations.h"
#include "Options.h"

namespace JSC { namespace DFG {

static unsigned numCompilations;

unsigned getNumCompilations()
{
    return numCompilations;
}

enum CompileMode { CompileFunction, CompileOther };
inline bool compile(CompileMode compileMode, ExecState* exec, CodeBlock* codeBlock, JITCode& jitCode, MacroAssemblerCodePtr* jitCodeWithArityCheck, unsigned osrEntryBytecodeIndex)
{
    SamplingRegion samplingRegion("DFG Compilation (Driver)");
    
    numCompilations++;
    
    ASSERT(codeBlock);
    ASSERT(codeBlock->alternative());
    ASSERT(codeBlock->alternative()->getJITType() == JITCode::BaselineJIT);
    
    ASSERT(osrEntryBytecodeIndex != UINT_MAX);

    if (!Options::useDFGJIT())
        return false;

    if (!Options::bytecodeRangeToDFGCompile().isInRange(codeBlock->instructionCount()))
        return false;

    if (logCompilationChanges())
        dataLog("DFG compiling ", *codeBlock, ", number of instructions = ", codeBlock->instructionCount(), "\n");
    
    // Derive our set of must-handle values. The compilation must be at least conservative
    // enough to allow for OSR entry with these values.
    unsigned numVarsWithValues;
    if (osrEntryBytecodeIndex)
        numVarsWithValues = codeBlock->m_numVars;
    else
        numVarsWithValues = 0;
    Operands<JSValue> mustHandleValues(codeBlock->numParameters(), numVarsWithValues);
    for (size_t i = 0; i < mustHandleValues.size(); ++i) {
        int operand = mustHandleValues.operandForIndex(i);
        if (operandIsArgument(operand)
            && !operandToArgument(operand)
            && compileMode == CompileFunction
            && codeBlock->specializationKind() == CodeForConstruct) {
            // Ugh. If we're in a constructor, the 'this' argument may hold garbage. It will
            // also never be used. It doesn't matter what we put into the value for this,
            // but it has to be an actual value that can be grokked by subsequent DFG passes,
            // so we sanitize it here by turning it into Undefined.
            mustHandleValues[i] = jsUndefined();
        } else
            mustHandleValues[i] = exec->uncheckedR(operand).jsValue();
    }
    
    Graph dfg(exec->vm(), codeBlock, osrEntryBytecodeIndex, mustHandleValues);
    if (!parse(exec, dfg))
        return false;
    
    // By this point the DFG bytecode parser will have potentially mutated various tables
    // in the CodeBlock. This is a good time to perform an early shrink, which is more
    // powerful than a late one. It's safe to do so because we haven't generated any code
    // that references any of the tables directly, yet.
    codeBlock->shrinkToFit(CodeBlock::EarlyShrink);

    if (validationEnabled())
        validate(dfg);
    
    performCPSRethreading(dfg);
    performUnification(dfg);
    performPredictionInjection(dfg);
    
    if (validationEnabled())
        validate(dfg);
    
    performBackwardsPropagation(dfg);
    performPredictionPropagation(dfg);
    performFixup(dfg);
    performTypeCheckHoisting(dfg);
    
    dfg.m_fixpointState = FixpointNotConverged;

    performCSE(dfg);
    performArgumentsSimplification(dfg);
    performCPSRethreading(dfg); // This should usually be a no-op since CSE rarely dethreads, and arguments simplification rarely does anything.
    performCFA(dfg);
    performConstantFolding(dfg);
    performCFGSimplification(dfg);

    dfg.m_fixpointState = FixpointConverged;

    performStoreElimination(dfg);
    performCPSRethreading(dfg);
    performDCE(dfg);
    performVirtualRegisterAllocation(dfg);

    GraphDumpMode modeForFinalValidate = DumpGraph;
    if (verboseCompilationEnabled()) {
        dataLogF("Graph after optimization:\n");
        dfg.dump();
        modeForFinalValidate = DontDumpGraph;
    }
    if (validationEnabled())
        validate(dfg, modeForFinalValidate);
    
    JITCompiler dataFlowJIT(dfg);
    bool result;
    if (compileMode == CompileFunction) {
        ASSERT(jitCodeWithArityCheck);
        
        result = dataFlowJIT.compileFunction(jitCode, *jitCodeWithArityCheck);
    } else {
        ASSERT(compileMode == CompileOther);
        ASSERT(!jitCodeWithArityCheck);
        
        result = dataFlowJIT.compile(jitCode);
    }
    
    return result;
}

bool tryCompile(ExecState* exec, CodeBlock* codeBlock, JITCode& jitCode, unsigned bytecodeIndex)
{
    return compile(CompileOther, exec, codeBlock, jitCode, 0, bytecodeIndex);
}

bool tryCompileFunction(ExecState* exec, CodeBlock* codeBlock, JITCode& jitCode, MacroAssemblerCodePtr& jitCodeWithArityCheck, unsigned bytecodeIndex)
{
    return compile(CompileFunction, exec, codeBlock, jitCode, &jitCodeWithArityCheck, bytecodeIndex);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

