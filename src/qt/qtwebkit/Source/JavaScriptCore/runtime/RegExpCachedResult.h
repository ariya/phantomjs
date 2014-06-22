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

#ifndef RegExpCachedResult_h
#define RegExpCachedResult_h

#include "RegExpObject.h"

namespace JSC {

    class JSString;
    class RegExpMatchesArray;

    // RegExpCachedResult is used to track the cached results of the last
    // match, stores on the RegExp constructor (e.g. $&, $_, $1, $2 ...).
    // These values will be lazily generated on demand, so the cached result
    // may be in a lazy or reified state. A lazy state is indicated by a
    // value of m_result indicating a successful match, and a reified state
    // is indicated by setting m_result to MatchResult::failed().
    // Following a successful match, m_result, m_lastInput and m_lastRegExp
    // can be used to reify the results from the match, following reification
    // m_reifiedResult and m_reifiedInput hold the cached results.
    class RegExpCachedResult {
    public:
        RegExpCachedResult(VM& vm, JSObject* owner, RegExp* emptyRegExp)
            : m_result(0, 0)
        {
            m_lastInput.set(vm, owner, jsEmptyString(&vm));
            m_lastRegExp.set(vm, owner, emptyRegExp);
        }

        ALWAYS_INLINE void record(VM& vm, JSObject* owner, RegExp* regExp, JSString* input, MatchResult result)
        {
            m_lastRegExp.set(vm, owner, regExp);
            m_lastInput.set(vm, owner, input);
            m_result = result;
        }

        RegExpMatchesArray* lastResult(ExecState*, JSObject* owner);
        void setInput(ExecState*, JSObject* owner, JSString*);

        JSString* input()
        {
            // If m_result showas a match then we're in a lazy state, so m_lastInput
            // is the most recent value of the input property. If not then we have
            // reified, in which case m_reifiedInput will contain the correct value.
            return m_result ? m_lastInput.get() : m_reifiedInput.get();
        }

        void visitChildren(SlotVisitor&);

    private:
        MatchResult m_result;
        WriteBarrier<JSString> m_lastInput;
        WriteBarrier<RegExp> m_lastRegExp;
        WriteBarrier<RegExpMatchesArray> m_reifiedResult;
        WriteBarrier<JSString> m_reifiedInput;
    };

} // namespace JSC

#endif // RegExpCachedResult_h
