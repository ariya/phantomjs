/*
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

#include "InjectedScriptModule.h"

#include "InjectedScript.h"
#include "InjectedScriptManager.h"
#include "ScriptFunctionCall.h"
#include "ScriptObject.h"

namespace WebCore {

InjectedScriptModule::InjectedScriptModule(const String& name)
    : InjectedScriptBase(name)
{
}

void InjectedScriptModule::ensureInjected(InjectedScriptManager* injectedScriptManager, ScriptState* scriptState)
{
    InjectedScript injectedScript = injectedScriptManager->injectedScriptFor(scriptState);
    ASSERT(!injectedScript.hasNoValue());
    if (injectedScript.hasNoValue())
        return;

    // FIXME: Make the InjectedScript a module itself.
    ScriptFunctionCall function(injectedScript.injectedScriptObject(), "module");
    function.appendArgument(name());
    bool hadException = false;
    ScriptValue resultValue = injectedScript.callFunctionWithEvalEnabled(function, hadException);
    ASSERT(!hadException);
    if (hadException || resultValue.hasNoValue() || !resultValue.isObject()) {
        ScriptFunctionCall function(injectedScript.injectedScriptObject(), "injectModule");
        function.appendArgument(name());
        function.appendArgument(source());
        resultValue = injectedScript.callFunctionWithEvalEnabled(function, hadException);
        if (hadException || resultValue.hasNoValue() || !resultValue.isObject()) {
            ASSERT_NOT_REACHED();
            return;
        }
    }

    ScriptObject moduleObject(scriptState, resultValue);
    initialize(moduleObject, injectedScriptManager->inspectedStateAccessCheck());
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
