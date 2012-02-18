/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DebuggerCallFrame.h"

#include "JSFunction.h"
#include "CodeBlock.h"
#include "Interpreter.h"
#include "Parser.h"

namespace JSC {

const UString* DebuggerCallFrame::functionName() const
{
    if (!m_callFrame->codeBlock())
        return 0;

    if (!m_callFrame->callee())
        return 0;

    JSObject* function = m_callFrame->callee();
    if (!function || !function->inherits(&JSFunction::s_info))
        return 0;
    return &asFunction(function)->name(m_callFrame);
}
    
UString DebuggerCallFrame::calculatedFunctionName() const
{
    if (!m_callFrame->codeBlock())
        return UString();

    JSObject* function = m_callFrame->callee();
    if (!function || !function->inherits(&JSFunction::s_info))
        return UString();

    return asFunction(function)->calculatedDisplayName(m_callFrame);
}

DebuggerCallFrame::Type DebuggerCallFrame::type() const
{
    if (m_callFrame->callee())
        return FunctionType;

    return ProgramType;
}

JSObject* DebuggerCallFrame::thisObject() const
{
    CodeBlock* codeBlock = m_callFrame->codeBlock();
    if (!codeBlock)
        return 0;

    JSValue thisValue = m_callFrame->uncheckedR(codeBlock->thisRegister()).jsValue();
    if (!thisValue.isObject())
        return 0;

    return asObject(thisValue);
}

JSValue DebuggerCallFrame::evaluate(const UString& script, JSValue& exception) const
{
    if (!m_callFrame->codeBlock())
        return JSValue();
    
    JSGlobalData& globalData = m_callFrame->globalData();
    EvalExecutable* eval = EvalExecutable::create(m_callFrame, makeSource(script), m_callFrame->codeBlock()->isStrictMode());
    if (globalData.exception) {
        exception = globalData.exception;
        globalData.exception = JSValue();
    }

    JSValue result = globalData.interpreter->execute(eval, m_callFrame, thisObject(), m_callFrame->scopeChain());
    if (globalData.exception) {
        exception = globalData.exception;
        globalData.exception = JSValue();
    }
    ASSERT(result);
    return result;
}

} // namespace JSC
