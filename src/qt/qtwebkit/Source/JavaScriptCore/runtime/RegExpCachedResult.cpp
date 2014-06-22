/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "RegExpCachedResult.h"

#include "Operations.h"
#include "RegExpMatchesArray.h"

namespace JSC {

void RegExpCachedResult::visitChildren(SlotVisitor& visitor)
{
    visitor.append(&m_lastInput);
    visitor.append(&m_lastRegExp);
    visitor.append(&m_reifiedInput);
    visitor.append(&m_reifiedResult);
}

RegExpMatchesArray* RegExpCachedResult::lastResult(ExecState* exec, JSObject* owner)
{
    if (m_result) {
        m_reifiedInput.set(exec->vm(), owner, m_lastInput.get());
        m_reifiedResult.set(exec->vm(), owner, RegExpMatchesArray::create(exec, m_lastInput.get(), m_lastRegExp.get(), m_result));
        m_result = MatchResult::failed();
    }
    return m_reifiedResult.get();
}

void RegExpCachedResult::setInput(ExecState* exec, JSObject* owner, JSString* input)
{
    // Make sure we're reified, otherwise m_reifiedInput will be ignored.
    lastResult(exec, owner);
    ASSERT(!m_result);
    m_reifiedInput.set(exec->vm(), owner, input);
}

} // namespace JSC
