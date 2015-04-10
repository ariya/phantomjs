/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Matt Lilek <webkit@mattlilek.com>
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(INSPECTOR)

#include "InjectedScriptManager.h"

#include "BindingSecurity.h"
#include "ExceptionCode.h"
#include "JSDOMWindow.h"
#include "JSDOMWindowCustom.h"
#include "JSInjectedScriptHost.h"
#include "JSMainThreadExecState.h"
#include "ScriptObject.h"
#include <parser/SourceCode.h>
#include <runtime/JSLock.h>

using namespace JSC;

namespace WebCore {

ScriptObject InjectedScriptManager::createInjectedScript(const String& source, ScriptState* scriptState, int id)
{
    JSLockHolder lock(scriptState);

    SourceCode sourceCode = makeSource(source);
    JSDOMGlobalObject* globalObject = jsCast<JSDOMGlobalObject*>(scriptState->lexicalGlobalObject());
    JSValue globalThisValue = scriptState->globalThisValue();

    JSValue evaluationException;
    JSValue evaluationReturnValue;
    if (isMainThread())
        evaluationReturnValue = JSMainThreadExecState::evaluate(scriptState, sourceCode, globalThisValue, &evaluationException);
    else {
        JSC::JSLockHolder lock(scriptState);
        evaluationReturnValue = JSC::evaluate(scriptState, sourceCode, globalThisValue, &evaluationException);
    }
    if (evaluationException)
        return ScriptObject();

    JSValue functionValue = evaluationReturnValue;
    CallData callData;
    CallType callType = getCallData(functionValue, callData);
    if (callType == CallTypeNone)
        return ScriptObject();

    MarkedArgumentBuffer args;
    args.append(toJS(scriptState, globalObject, m_injectedScriptHost.get()));
    args.append(globalThisValue);
    args.append(jsNumber(id));

    JSValue result = JSC::call(scriptState, functionValue, callType, callData, globalThisValue, args);
    if (result.isObject())
        return ScriptObject(scriptState, result.getObject());
    return ScriptObject();
}

bool InjectedScriptManager::canAccessInspectedWindow(ScriptState* scriptState)
{
    JSLockHolder lock(scriptState);
    JSDOMWindow* inspectedWindow = toJSDOMWindow(scriptState->lexicalGlobalObject());
    if (!inspectedWindow)
        return false;
    return BindingSecurity::shouldAllowAccessToDOMWindow(scriptState, inspectedWindow->impl(), DoNotReportSecurityError);
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
