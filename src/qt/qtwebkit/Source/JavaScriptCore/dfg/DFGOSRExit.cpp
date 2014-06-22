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
#include "DFGOSRExit.h"

#if ENABLE(DFG_JIT)

#include "DFGAssemblyHelpers.h"
#include "DFGSpeculativeJIT.h"
#include "JSCellInlines.h"

namespace JSC { namespace DFG {

OSRExit::OSRExit(ExitKind kind, JSValueSource jsValueSource, MethodOfGettingAValueProfile valueProfile, SpeculativeJIT* jit, unsigned streamIndex, unsigned recoveryIndex)
    : m_jsValueSource(jsValueSource)
    , m_valueProfile(valueProfile)
    , m_patchableCodeOffset(0)
    , m_codeOrigin(jit->m_codeOriginForOSR)
    , m_codeOriginForExitProfile(m_codeOrigin)
    , m_recoveryIndex(recoveryIndex)
    , m_watchpointIndex(std::numeric_limits<unsigned>::max())
    , m_kind(kind)
    , m_count(0)
    , m_streamIndex(streamIndex)
    , m_lastSetOperand(jit->m_lastSetOperand)
{
    ASSERT(m_codeOrigin.isSet());
}

void OSRExit::setPatchableCodeOffset(MacroAssembler::PatchableJump check)
{
    m_patchableCodeOffset = check.m_jump.m_label.m_offset;
}

MacroAssembler::Jump OSRExit::getPatchableCodeOffsetAsJump() const
{
    return MacroAssembler::Jump(AssemblerLabel(m_patchableCodeOffset));
}

CodeLocationJump OSRExit::codeLocationForRepatch(CodeBlock* dfgCodeBlock) const
{
    return CodeLocationJump(dfgCodeBlock->getJITCode().dataAddressAtOffset(m_patchableCodeOffset));
}

void OSRExit::correctJump(LinkBuffer& linkBuffer)
{
    MacroAssembler::Label label;
    label.m_label.m_offset = m_patchableCodeOffset;
    m_patchableCodeOffset = linkBuffer.offsetOf(label);
}

bool OSRExit::considerAddingAsFrequentExitSiteSlow(CodeBlock* profiledCodeBlock)
{
    FrequentExitSite exitSite;
    
    if (m_kind == ArgumentsEscaped) {
        // Count this one globally. It doesn't matter where in the code block the arguments excaped;
        // the fact that they did is not associated with any particular instruction.
        exitSite = FrequentExitSite(m_kind);
    } else
        exitSite = FrequentExitSite(m_codeOriginForExitProfile.bytecodeIndex, m_kind);
    
    return baselineCodeBlockForOriginAndBaselineCodeBlock(m_codeOriginForExitProfile, profiledCodeBlock)->addFrequentExitSite(exitSite);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)
