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

#if ENABLE(INSPECTOR)

#include "InspectorRuntimeAgent.h"

#include "InjectedScript.h"
#include "InjectedScriptManager.h"
#include "InspectorValues.h"
#include "JSDOMWindowBase.h"
#include <parser/ParserError.h>
#include <parser/SourceCode.h>
#include <runtime/Completion.h>
#include <runtime/JSLock.h>
#include <wtf/PassRefPtr.h>

#if ENABLE(JAVASCRIPT_DEBUGGER)
#include "ScriptDebugServer.h"
#endif

using namespace JSC;

namespace WebCore {

static bool asBool(const bool* const b)
{
    return b ? *b : false;
}

InspectorRuntimeAgent::InspectorRuntimeAgent(InstrumentingAgents* instrumentingAgents, InspectorCompositeState* state, InjectedScriptManager* injectedScriptManager)
    : InspectorBaseAgent<InspectorRuntimeAgent>("Runtime", instrumentingAgents, state)
    , m_enabled(false)
    , m_injectedScriptManager(injectedScriptManager)
#if ENABLE(JAVASCRIPT_DEBUGGER)
    , m_scriptDebugServer(0)
#endif
{
}

InspectorRuntimeAgent::~InspectorRuntimeAgent()
{
}

#if ENABLE(JAVASCRIPT_DEBUGGER)
static ScriptDebugServer::PauseOnExceptionsState setPauseOnExceptionsState(ScriptDebugServer* scriptDebugServer, ScriptDebugServer::PauseOnExceptionsState newState)
{
    ASSERT(scriptDebugServer);
    ScriptDebugServer::PauseOnExceptionsState presentState = scriptDebugServer->pauseOnExceptionsState();
    if (presentState != newState)
        scriptDebugServer->setPauseOnExceptionsState(newState);
    return presentState;
}
#endif

static PassRefPtr<TypeBuilder::Runtime::ErrorRange> buildErrorRangeObject(const JSTokenLocation& tokenLocation)
{
    RefPtr<TypeBuilder::Runtime::ErrorRange> result = TypeBuilder::Runtime::ErrorRange::create()
        .setStartOffset(tokenLocation.startOffset)
        .setEndOffset(tokenLocation.endOffset);
    return result.release();
}

void InspectorRuntimeAgent::parse(ErrorString*, const String& expression, TypeBuilder::Runtime::SyntaxErrorType::Enum* result, TypeBuilder::OptOutput<String>* message, RefPtr<TypeBuilder::Runtime::ErrorRange>& range)
{
    VM* vm = JSDOMWindowBase::commonVM();
    JSLockHolder lock(vm);

    ParserError error;
    checkSyntax(*vm, JSC::makeSource(expression), error);

    switch (error.m_syntaxErrorType) {
    case ParserError::SyntaxErrorNone:
        *result = TypeBuilder::Runtime::SyntaxErrorType::None;
        break;
    case ParserError::SyntaxErrorIrrecoverable:
        *result = TypeBuilder::Runtime::SyntaxErrorType::Irrecoverable;
        break;
    case ParserError::SyntaxErrorUnterminatedLiteral:
        *result = TypeBuilder::Runtime::SyntaxErrorType::Unterminated_literal;
        break;
    case ParserError::SyntaxErrorRecoverable:
        *result = TypeBuilder::Runtime::SyntaxErrorType::Recoverable;
        break;
    }

    if (error.m_syntaxErrorType != ParserError::SyntaxErrorNone) {
        *message = error.m_message;
        range = buildErrorRangeObject(error.m_token.m_location);
    }
}

void InspectorRuntimeAgent::evaluate(ErrorString* errorString, const String& expression, const String* const objectGroup, const bool* const includeCommandLineAPI, const bool* const doNotPauseOnExceptionsAndMuteConsole, const int* executionContextId, const bool* const returnByValue, const bool* generatePreview, RefPtr<TypeBuilder::Runtime::RemoteObject>& result, TypeBuilder::OptOutput<bool>* wasThrown)
{
    InjectedScript injectedScript = injectedScriptForEval(errorString, executionContextId);
    if (injectedScript.hasNoValue())
        return;
#if ENABLE(JAVASCRIPT_DEBUGGER)
    ScriptDebugServer::PauseOnExceptionsState previousPauseOnExceptionsState = ScriptDebugServer::DontPauseOnExceptions;
    if (asBool(doNotPauseOnExceptionsAndMuteConsole))
        previousPauseOnExceptionsState = setPauseOnExceptionsState(m_scriptDebugServer, ScriptDebugServer::DontPauseOnExceptions);
#endif
    if (asBool(doNotPauseOnExceptionsAndMuteConsole))
        muteConsole();

    injectedScript.evaluate(errorString, expression, objectGroup ? *objectGroup : "", asBool(includeCommandLineAPI), asBool(returnByValue), asBool(generatePreview), &result, wasThrown);

    if (asBool(doNotPauseOnExceptionsAndMuteConsole)) {
        unmuteConsole();
#if ENABLE(JAVASCRIPT_DEBUGGER)
        setPauseOnExceptionsState(m_scriptDebugServer, previousPauseOnExceptionsState);
#endif
    }
}

void InspectorRuntimeAgent::callFunctionOn(ErrorString* errorString, const String& objectId, const String& expression, const RefPtr<InspectorArray>* const optionalArguments, const bool* const doNotPauseOnExceptionsAndMuteConsole, const bool* const returnByValue, const bool* generatePreview, RefPtr<TypeBuilder::Runtime::RemoteObject>& result, TypeBuilder::OptOutput<bool>* wasThrown)
{
    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptForObjectId(objectId);
    if (injectedScript.hasNoValue()) {
        *errorString = "Inspected frame has gone";
        return;
    }
    String arguments;
    if (optionalArguments)
        arguments = (*optionalArguments)->toJSONString();

#if ENABLE(JAVASCRIPT_DEBUGGER)
    ScriptDebugServer::PauseOnExceptionsState previousPauseOnExceptionsState = ScriptDebugServer::DontPauseOnExceptions;
    if (asBool(doNotPauseOnExceptionsAndMuteConsole))
        previousPauseOnExceptionsState = setPauseOnExceptionsState(m_scriptDebugServer, ScriptDebugServer::DontPauseOnExceptions);
#endif
    if (asBool(doNotPauseOnExceptionsAndMuteConsole))
        muteConsole();

    injectedScript.callFunctionOn(errorString, objectId, expression, arguments, asBool(returnByValue), asBool(generatePreview), &result, wasThrown);

    if (asBool(doNotPauseOnExceptionsAndMuteConsole)) {
        unmuteConsole();
#if ENABLE(JAVASCRIPT_DEBUGGER)
        setPauseOnExceptionsState(m_scriptDebugServer, previousPauseOnExceptionsState);
#endif
    }
}

void InspectorRuntimeAgent::getProperties(ErrorString* errorString, const String& objectId, const bool* const ownProperties, RefPtr<TypeBuilder::Array<TypeBuilder::Runtime::PropertyDescriptor> >& result, RefPtr<TypeBuilder::Array<TypeBuilder::Runtime::InternalPropertyDescriptor> >& internalProperties)
{
    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptForObjectId(objectId);
    if (injectedScript.hasNoValue()) {
        *errorString = "Inspected frame has gone";
        return;
    }

#if ENABLE(JAVASCRIPT_DEBUGGER)
    ScriptDebugServer::PauseOnExceptionsState previousPauseOnExceptionsState = setPauseOnExceptionsState(m_scriptDebugServer, ScriptDebugServer::DontPauseOnExceptions);
#endif
    muteConsole();

    injectedScript.getProperties(errorString, objectId, ownProperties ? *ownProperties : false, &result);
    injectedScript.getInternalProperties(errorString, objectId, &internalProperties);

    unmuteConsole();
#if ENABLE(JAVASCRIPT_DEBUGGER)
    setPauseOnExceptionsState(m_scriptDebugServer, previousPauseOnExceptionsState);
#endif
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

void InspectorRuntimeAgent::run(ErrorString*)
{
}

#if ENABLE(JAVASCRIPT_DEBUGGER)
void InspectorRuntimeAgent::setScriptDebugServer(ScriptDebugServer* scriptDebugServer)
{
    m_scriptDebugServer = scriptDebugServer;
}
#endif // ENABLE(JAVASCRIPT_DEBUGGER)

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
