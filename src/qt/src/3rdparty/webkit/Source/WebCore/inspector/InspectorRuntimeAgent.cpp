/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "InspectorRuntimeAgent.h"

#if ENABLE(INSPECTOR)

#include "InjectedScript.h"
#include "InjectedScriptManager.h"
#include "InspectorValues.h"
#include <wtf/PassRefPtr.h>

namespace WebCore {

InspectorRuntimeAgent::InspectorRuntimeAgent(InjectedScriptManager* injectedScriptManager)
    : m_injectedScriptManager(injectedScriptManager)
{
}

InspectorRuntimeAgent::~InspectorRuntimeAgent()
{
}

void InspectorRuntimeAgent::evaluate(ErrorString* errorString, const String& expression, const String* const objectGroup, const bool* const includeCommandLineAPI, RefPtr<InspectorObject>* result, bool* wasThrown)
{
    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptFor(getDefaultInspectedState());
    if (injectedScript.hasNoValue()) {
        *errorString = "Inspected frame has gone";
        return;
    }
    injectedScript.evaluate(errorString, expression, objectGroup ? *objectGroup : "", includeCommandLineAPI ? *includeCommandLineAPI : false, result, wasThrown);
}

void InspectorRuntimeAgent::evaluateOn(ErrorString* errorString, const String& objectId, const String& expression, RefPtr<InspectorObject>* result, bool* wasThrown)
{
    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptForObjectId(objectId);
    if (injectedScript.hasNoValue()) {
        *errorString = "Inspected frame has gone";
        return;
    }
    injectedScript.evaluateOn(errorString, objectId, expression, result, wasThrown);
}

void InspectorRuntimeAgent::getProperties(ErrorString* errorString, const String& objectId, bool ignoreHasOwnProperty, RefPtr<InspectorArray>* result)
{
    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptForObjectId(objectId);
    if (injectedScript.hasNoValue()) {
        *errorString = "Inspected frame has gone";
        return;
    }
    injectedScript.getProperties(errorString, objectId, ignoreHasOwnProperty, result);
}

void InspectorRuntimeAgent::setPropertyValue(ErrorString* errorString, const String& objectId, const String& propertyName, const String& expression)
{
    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptForObjectId(objectId);
    if (!injectedScript.hasNoValue())
        injectedScript.setPropertyValue(errorString, objectId, propertyName, expression);
    else
        *errorString = "No injected script found";
}

void InspectorRuntimeAgent::releaseObject(ErrorString*, const String& objectId)
{
    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptForObjectId(objectId);
    if (!injectedScript.hasNoValue())
        injectedScript.releaseObject(objectId);
}

void InspectorRuntimeAgent::releaseObjectGroup(ErrorString*, const String& objectGroup)
{
    m_injectedScriptManager->releaseObjectGroup(objectGroup);
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
