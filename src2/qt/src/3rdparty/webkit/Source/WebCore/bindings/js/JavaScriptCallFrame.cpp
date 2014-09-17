/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
#include "JavaScriptCallFrame.h"

#include "JSDOMBinding.h"

#if ENABLE(JAVASCRIPT_DEBUGGER) && USE(JSC)

#include "PlatformString.h"
#include <debugger/DebuggerCallFrame.h>
#include <runtime/Completion.h>
#include <runtime/JSGlobalObject.h>
#include <runtime/JSLock.h>
#include <runtime/JSObject.h>
#include <runtime/JSValue.h>

using namespace JSC;

namespace WebCore {
    
JavaScriptCallFrame::JavaScriptCallFrame(const DebuggerCallFrame& debuggerCallFrame, PassRefPtr<JavaScriptCallFrame> caller, intptr_t sourceID, const TextPosition0& textPosition)
    : m_debuggerCallFrame(debuggerCallFrame)
    , m_caller(caller)
    , m_sourceID(sourceID)
    , m_textPosition(textPosition)
    , m_isValid(true)
{
}

JavaScriptCallFrame* JavaScriptCallFrame::caller()
{
    return m_caller.get();
}

JSC::ScopeChainNode* JavaScriptCallFrame::scopeChain() const
{
    ASSERT(m_isValid);
    if (!m_isValid)
        return 0;
    return m_debuggerCallFrame.scopeChain();
}

JSC::JSGlobalObject* JavaScriptCallFrame::dynamicGlobalObject() const
{
    ASSERT(m_isValid);
    if (!m_isValid)
        return 0;
    return m_debuggerCallFrame.dynamicGlobalObject();
}

String JavaScriptCallFrame::functionName() const
{
    ASSERT(m_isValid);
    if (!m_isValid)
        return String();
    UString functionName = m_debuggerCallFrame.calculatedFunctionName();
    if (functionName.isEmpty())
        return String();
    return ustringToString(functionName);
}

DebuggerCallFrame::Type JavaScriptCallFrame::type() const
{
    ASSERT(m_isValid);
    if (!m_isValid)
        return DebuggerCallFrame::ProgramType;
    return m_debuggerCallFrame.type();
}

JSObject* JavaScriptCallFrame::thisObject() const
{
    ASSERT(m_isValid);
    if (!m_isValid)
        return 0;
    return m_debuggerCallFrame.thisObject();
}

// Evaluate some JavaScript code in the scope of this frame.
JSValue JavaScriptCallFrame::evaluate(const UString& script, JSValue& exception) const
{
    ASSERT(m_isValid);
    if (!m_isValid)
        return jsNull();

    JSLock lock(SilenceAssertionsOnly);
    return m_debuggerCallFrame.evaluate(script, exception);
}

} // namespace WebCore

#endif // ENABLE(JAVASCRIPT_DEBUGGER)
