/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Matt Lilek <webkit@mattlilek.com>
 * Copyright (C) 2010-2011 Google Inc. All rights reserved.
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
#include "InjectedScriptManager.h"

#if ENABLE(INSPECTOR)

#include "ExceptionCode.h"
#include "InjectedScript.h"
#include "JSDOMWindow.h"
#include "JSDOMWindowCustom.h"
#include "JSInjectedScriptHost.h"
#include "JSMainThreadExecState.h"
#include <parser/SourceCode.h>
#include <runtime/JSLock.h>

using namespace JSC;

namespace WebCore {

ScriptObject InjectedScriptManager::createInjectedScript(const String& source, ScriptState* scriptState, long id)
{
    SourceCode sourceCode = makeSource(stringToUString(source));
    JSLock lock(SilenceAssertionsOnly);
    JSDOMGlobalObject* globalObject = static_cast<JSDOMGlobalObject*>(scriptState->lexicalGlobalObject());
    JSValue globalThisValue = scriptState->globalThisValue();
    Completion comp = JSMainThreadExecState::evaluate(scriptState, globalObject->globalScopeChain(), sourceCode, globalThisValue);
    if (comp.complType() != JSC::Normal && comp.complType() != JSC::ReturnValue)
        return ScriptObject();
    JSValue functionValue = comp.value();
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

void InjectedScriptManager::discardInjectedScript(ScriptState* scriptState)
{
    JSDOMGlobalObject* globalObject = static_cast<JSDOMGlobalObject*>(scriptState->lexicalGlobalObject());
    globalObject->setInjectedScript(0);
}

InjectedScript InjectedScriptManager::injectedScriptFor(ScriptState* scriptState)
{
    JSLock lock(SilenceAssertionsOnly);
    JSDOMGlobalObject* globalObject = static_cast<JSDOMGlobalObject*>(scriptState->lexicalGlobalObject());
    JSObject* injectedScript = globalObject->injectedScript();
    if (injectedScript)
        return InjectedScript(ScriptObject(scriptState, injectedScript), m_inspectedStateAccessCheck);

    if (!m_inspectedStateAccessCheck(scriptState))
        return InjectedScript();

    pair<long, ScriptObject> injectedScriptObject = injectScript(injectedScriptSource(), scriptState);
    globalObject->setInjectedScript(injectedScriptObject.second.jsObject());
    InjectedScript result(injectedScriptObject.second, m_inspectedStateAccessCheck);
    m_idToInjectedScript.set(injectedScriptObject.first, result);
    return result;
}

bool InjectedScriptManager::canAccessInspectedWindow(ScriptState* scriptState)
{
    JSLock lock(SilenceAssertionsOnly);
    JSDOMWindow* inspectedWindow = toJSDOMWindow(scriptState->lexicalGlobalObject());
    if (!inspectedWindow)
        return false;
    return inspectedWindow->allowsAccessFromNoErrorMessage(scriptState);
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
