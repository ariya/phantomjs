/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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
#include "CallLinkStatus.h"

#include "CodeBlock.h"
#include "LLIntCallLinkInfo.h"
#include "Operations.h"
#include <wtf/CommaPrinter.h>

namespace JSC {

CallLinkStatus::CallLinkStatus(JSValue value)
    : m_callTarget(value)
    , m_executable(0)
    , m_structure(0)
    , m_couldTakeSlowPath(false)
    , m_isProved(false)
{
    if (!value || !value.isCell())
        return;
    
    m_structure = value.asCell()->structure();
    
    if (!value.asCell()->inherits(&JSFunction::s_info))
        return;
    
    m_executable = jsCast<JSFunction*>(value.asCell())->executable();
}

JSFunction* CallLinkStatus::function() const
{
    if (!m_callTarget || !m_callTarget.isCell())
        return 0;
    
    if (!m_callTarget.asCell()->inherits(&JSFunction::s_info))
        return 0;
    
    return jsCast<JSFunction*>(m_callTarget.asCell());
}

InternalFunction* CallLinkStatus::internalFunction() const
{
    if (!m_callTarget || !m_callTarget.isCell())
        return 0;
    
    if (!m_callTarget.asCell()->inherits(&InternalFunction::s_info))
        return 0;
    
    return jsCast<InternalFunction*>(m_callTarget.asCell());
}

Intrinsic CallLinkStatus::intrinsicFor(CodeSpecializationKind kind) const
{
    if (!m_executable)
        return NoIntrinsic;
    
    return m_executable->intrinsicFor(kind);
}

CallLinkStatus CallLinkStatus::computeFromLLInt(CodeBlock* profiledBlock, unsigned bytecodeIndex)
{
    UNUSED_PARAM(profiledBlock);
    UNUSED_PARAM(bytecodeIndex);
#if ENABLE(LLINT)
    Instruction* instruction = profiledBlock->instructions().begin() + bytecodeIndex;
    LLIntCallLinkInfo* callLinkInfo = instruction[4].u.callLinkInfo;
    
    return CallLinkStatus(callLinkInfo->lastSeenCallee.get());
#else
    return CallLinkStatus();
#endif
}

CallLinkStatus CallLinkStatus::computeFor(CodeBlock* profiledBlock, unsigned bytecodeIndex)
{
    UNUSED_PARAM(profiledBlock);
    UNUSED_PARAM(bytecodeIndex);
#if ENABLE(JIT) && ENABLE(VALUE_PROFILER)
    if (!profiledBlock->numberOfCallLinkInfos())
        return computeFromLLInt(profiledBlock, bytecodeIndex);
    
    if (profiledBlock->couldTakeSlowCase(bytecodeIndex))
        return CallLinkStatus::takesSlowPath();
    
    CallLinkInfo& callLinkInfo = profiledBlock->getCallLinkInfo(bytecodeIndex);
    if (callLinkInfo.stub)
        return CallLinkStatus(callLinkInfo.stub->executable(), callLinkInfo.stub->structure());
    
    JSFunction* target = callLinkInfo.lastSeenCallee.get();
    if (!target)
        return computeFromLLInt(profiledBlock, bytecodeIndex);
    
    if (callLinkInfo.hasSeenClosure)
        return CallLinkStatus(target->executable(), target->structure());

    return CallLinkStatus(target);
#else
    return CallLinkStatus();
#endif
}

void CallLinkStatus::dump(PrintStream& out) const
{
    if (!isSet()) {
        out.print("Not Set");
        return;
    }
    
    CommaPrinter comma;
    
    if (m_isProved)
        out.print(comma, "Statically Proved");
    
    if (m_couldTakeSlowPath)
        out.print(comma, "Could Take Slow Path");
    
    if (m_callTarget)
        out.print(comma, "Known target: ", m_callTarget);
    
    if (m_executable)
        out.print(comma, "Executable/CallHash: ", RawPointer(m_executable), "/", m_executable->hashFor(CodeForCall));
    
    if (m_structure)
        out.print(comma, "Structure: ", RawPointer(m_structure));
}

} // namespace JSC

