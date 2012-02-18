/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010-2011 Google Inc. All rights reserved.
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
#include "InspectorDebuggerAgent.h"

#if ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(INSPECTOR)
#include "InjectedScript.h"
#include "InjectedScriptManager.h"
#include "InspectorFrontend.h"
#include "InspectorState.h"
#include "InspectorValues.h"
#include "InstrumentingAgents.h"
#include "PlatformString.h"
#include "ScriptDebugServer.h"
#include "ScriptObject.h"
#include <wtf/text/StringConcatenate.h>

namespace WebCore {

namespace DebuggerAgentState {
static const char debuggerEnabled[] = "debuggerEnabled";
static const char javaScriptBreakpoints[] = "javaScriptBreakopints";
};

InspectorDebuggerAgent::InspectorDebuggerAgent(InstrumentingAgents* instrumentingAgents, InspectorState* inspectorState, InjectedScriptManager* injectedScriptManager)
    : m_instrumentingAgents(instrumentingAgents)
    , m_inspectorState(inspectorState)
    , m_injectedScriptManager(injectedScriptManager)
    , m_frontend(0)
    , m_pausedScriptState(0)
    , m_javaScriptPauseScheduled(false)
    , m_listener(0)
{
}

InspectorDebuggerAgent::~InspectorDebuggerAgent()
{
    ASSERT(!m_instrumentingAgents->inspectorDebuggerAgent());
}

void InspectorDebuggerAgent::enable(bool restoringFromState)
{
    ASSERT(m_frontend);
    if (!restoringFromState && enabled())
        return;
    m_inspectorState->setBoolean(DebuggerAgentState::debuggerEnabled, true);
    m_instrumentingAgents->setInspectorDebuggerAgent(this);

    scriptDebugServer().clearBreakpoints();
    // FIXME(WK44513): breakpoints activated flag should be synchronized between all front-ends
    scriptDebugServer().setBreakpointsActivated(true);
    startListeningScriptDebugServer();

    m_frontend->debuggerWasEnabled();
    if (m_listener)
        m_listener->debuggerWasEnabled();
}

void InspectorDebuggerAgent::disable()
{
    if (!enabled())
        return;
    m_inspectorState->setBoolean(DebuggerAgentState::debuggerEnabled, false);
    m_inspectorState->setObject(DebuggerAgentState::javaScriptBreakpoints, InspectorObject::create());
    m_instrumentingAgents->setInspectorDebuggerAgent(0);

    stopListeningScriptDebugServer();
    clear();

    if (m_frontend)
        m_frontend->debuggerWasDisabled();
    if (m_listener)
        m_listener->debuggerWasDisabled();
}

bool InspectorDebuggerAgent::enabled()
{
    return m_inspectorState->getBoolean(DebuggerAgentState::debuggerEnabled);
}

void InspectorDebuggerAgent::restore()
{
    if (m_inspectorState->getBoolean(DebuggerAgentState::debuggerEnabled))
        enable(true);
}

void InspectorDebuggerAgent::setFrontend(InspectorFrontend* frontend)
{
    m_frontend = frontend->debugger();
}

void InspectorDebuggerAgent::clearFrontend()
{
    m_frontend = 0;

    if (!enabled())
        return;
    // If the window is being closed with the debugger enabled,
    // remember this state to re-enable debugger on the next window
    // opening.
    disable();
}

void InspectorDebuggerAgent::setBreakpointsActive(ErrorString*, bool active)
{
    if (active)
        scriptDebugServer().activateBreakpoints();
    else
        scriptDebugServer().deactivateBreakpoints();
}

void InspectorDebuggerAgent::inspectedURLChanged(const String&)
{
    m_scripts.clear();
    m_breakpointIdToDebugServerBreakpointIds.clear();
}

static PassRefPtr<InspectorObject> buildObjectForBreakpointCookie(const String& url, int lineNumber, int columnNumber, const String& condition)
{
    RefPtr<InspectorObject> breakpointObject = InspectorObject::create();
    breakpointObject->setString("url", url);
    breakpointObject->setNumber("lineNumber", lineNumber);
    breakpointObject->setNumber("columnNumber", columnNumber);
    breakpointObject->setString("condition", condition);
    return breakpointObject;
}

void InspectorDebuggerAgent::setBreakpointByUrl(ErrorString*, const String& url, int lineNumber, const int* const optionalColumnNumber, const String* const optionalCondition, String* outBreakpointId, RefPtr<InspectorArray>* locations)
{
    int columnNumber = optionalColumnNumber ? *optionalColumnNumber : 0;
    String condition = optionalCondition ? *optionalCondition : "";

    String breakpointId = makeString(url, ":", String::number(lineNumber), ":", String::number(columnNumber));
    RefPtr<InspectorObject> breakpointsCookie = m_inspectorState->getObject(DebuggerAgentState::javaScriptBreakpoints);
    if (breakpointsCookie->find(breakpointId) != breakpointsCookie->end())
        return;
    breakpointsCookie->setObject(breakpointId, buildObjectForBreakpointCookie(url, lineNumber, columnNumber, condition));
    m_inspectorState->setObject(DebuggerAgentState::javaScriptBreakpoints, breakpointsCookie);

    ScriptBreakpoint breakpoint(lineNumber, columnNumber, condition);
    for (ScriptsMap::iterator it = m_scripts.begin(); it != m_scripts.end(); ++it) {
        if (it->second.url != url)
            continue;
        RefPtr<InspectorObject> location = resolveBreakpoint(breakpointId, it->first, breakpoint);
        if (location)
            (*locations)->pushObject(location);
    }
    *outBreakpointId = breakpointId;
}

static bool parseLocation(ErrorString* errorString, RefPtr<InspectorObject> location, String* sourceId, int* lineNumber, int* columnNumber)
{
    if (!location->getString("sourceID", sourceId) || !location->getNumber("lineNumber", lineNumber)) {
        // FIXME: replace with input validation.
        *errorString = "sourceId and lineNumber are required.";
        return false;
    }
    *columnNumber = 0;
    location->getNumber("columnNumber", columnNumber);
    return true;
}

void InspectorDebuggerAgent::setBreakpoint(ErrorString* errorString, PassRefPtr<InspectorObject> location, const String* const optionalCondition, String* outBreakpointId, RefPtr<InspectorObject>* actualLocation)
{
    String sourceId;
    int lineNumber;
    int columnNumber;

    if (!parseLocation(errorString, location, &sourceId, &lineNumber, &columnNumber))
        return;

    String condition = optionalCondition ? *optionalCondition : "";

    String breakpointId = makeString(sourceId, ":", String::number(lineNumber), ":", String::number(columnNumber));
    if (m_breakpointIdToDebugServerBreakpointIds.find(breakpointId) != m_breakpointIdToDebugServerBreakpointIds.end())
        return;
    ScriptBreakpoint breakpoint(lineNumber, columnNumber, condition);
    *actualLocation = resolveBreakpoint(breakpointId, sourceId, breakpoint);
    if (*actualLocation)
        *outBreakpointId = breakpointId;
    else
        *errorString = "Could not resolve breakpoint";
}

void InspectorDebuggerAgent::removeBreakpoint(ErrorString*, const String& breakpointId)
{
    RefPtr<InspectorObject> breakpointsCookie = m_inspectorState->getObject(DebuggerAgentState::javaScriptBreakpoints);
    breakpointsCookie->remove(breakpointId);
    m_inspectorState->setObject(DebuggerAgentState::javaScriptBreakpoints, breakpointsCookie);

    BreakpointIdToDebugServerBreakpointIdsMap::iterator debugServerBreakpointIdsIterator = m_breakpointIdToDebugServerBreakpointIds.find(breakpointId);
    if (debugServerBreakpointIdsIterator == m_breakpointIdToDebugServerBreakpointIds.end())
        return;
    for (size_t i = 0; i < debugServerBreakpointIdsIterator->second.size(); ++i)
        scriptDebugServer().removeBreakpoint(debugServerBreakpointIdsIterator->second[i]);
    m_breakpointIdToDebugServerBreakpointIds.remove(debugServerBreakpointIdsIterator);
}

void InspectorDebuggerAgent::continueToLocation(ErrorString* errorString, PassRefPtr<InspectorObject> location)
{
    if (!m_continueToLocationBreakpointId.isEmpty()) {
        scriptDebugServer().removeBreakpoint(m_continueToLocationBreakpointId);
        m_continueToLocationBreakpointId = "";
    }

    String sourceId;
    int lineNumber;
    int columnNumber;

    if (!parseLocation(errorString, location, &sourceId, &lineNumber, &columnNumber))
        return;

    ScriptBreakpoint breakpoint(lineNumber, columnNumber, "");
    m_continueToLocationBreakpointId = scriptDebugServer().setBreakpoint(sourceId, breakpoint, &lineNumber, &columnNumber);
    resume(errorString);
}

PassRefPtr<InspectorObject> InspectorDebuggerAgent::resolveBreakpoint(const String& breakpointId, const String& sourceId, const ScriptBreakpoint& breakpoint)
{
    ScriptsMap::iterator scriptIterator = m_scripts.find(sourceId);
    if (scriptIterator == m_scripts.end())
        return 0;
    Script& script = scriptIterator->second;
    if (breakpoint.lineNumber < script.startLine || script.endLine <= breakpoint.lineNumber)
        return 0;

    int actualLineNumber;
    int actualColumnNumber;
    String debugServerBreakpointId = scriptDebugServer().setBreakpoint(sourceId, breakpoint, &actualLineNumber, &actualColumnNumber);
    if (debugServerBreakpointId.isEmpty())
        return 0;

    BreakpointIdToDebugServerBreakpointIdsMap::iterator debugServerBreakpointIdsIterator = m_breakpointIdToDebugServerBreakpointIds.find(breakpointId);
    if (debugServerBreakpointIdsIterator == m_breakpointIdToDebugServerBreakpointIds.end())
        debugServerBreakpointIdsIterator = m_breakpointIdToDebugServerBreakpointIds.set(breakpointId, Vector<String>()).first;
    debugServerBreakpointIdsIterator->second.append(debugServerBreakpointId);

    RefPtr<InspectorObject> location = InspectorObject::create();
    location->setString("sourceID", sourceId);
    location->setNumber("lineNumber", actualLineNumber);
    location->setNumber("columnNumber", actualColumnNumber);
    return location;
}

void InspectorDebuggerAgent::editScriptSource(ErrorString* error, const String& sourceID, const String& newContent, RefPtr<InspectorArray>* newCallFrames)
{
    if (scriptDebugServer().editScriptSource(sourceID, newContent, error, &m_currentCallStack))
        *newCallFrames = currentCallFrames();
}

void InspectorDebuggerAgent::getScriptSource(ErrorString*, const String& sourceID, String* scriptSource)
{
    *scriptSource = m_scripts.get(sourceID).data;
}

void InspectorDebuggerAgent::schedulePauseOnNextStatement(DebuggerEventType type, PassRefPtr<InspectorValue> data)
{
    if (m_javaScriptPauseScheduled)
        return;
    m_breakProgramDetails = InspectorObject::create();
    m_breakProgramDetails->setNumber("eventType", type);
    m_breakProgramDetails->setValue("eventData", data);
    scriptDebugServer().setPauseOnNextStatement(true);
}

void InspectorDebuggerAgent::cancelPauseOnNextStatement()
{
    if (m_javaScriptPauseScheduled)
        return;
    m_breakProgramDetails = 0;
    scriptDebugServer().setPauseOnNextStatement(false);
}

void InspectorDebuggerAgent::pause(ErrorString*)
{
    schedulePauseOnNextStatement(JavaScriptPauseEventType, InspectorObject::create());
    m_javaScriptPauseScheduled = true;
}

void InspectorDebuggerAgent::resume(ErrorString*)
{
    m_injectedScriptManager->releaseObjectGroup("backtrace");
    scriptDebugServer().continueProgram();
}

void InspectorDebuggerAgent::stepOver(ErrorString*)
{
    scriptDebugServer().stepOverStatement();
}

void InspectorDebuggerAgent::stepInto(ErrorString*)
{
    scriptDebugServer().stepIntoStatement();
}

void InspectorDebuggerAgent::stepOut(ErrorString*)
{
    scriptDebugServer().stepOutOfFunction();
}

void InspectorDebuggerAgent::setPauseOnExceptions(ErrorString* errorString, const String& stringPauseState)
{
    ScriptDebugServer::PauseOnExceptionsState pauseState;
    if (stringPauseState == "none")
        pauseState = ScriptDebugServer::DontPauseOnExceptions;
    else if (stringPauseState == "all")
        pauseState = ScriptDebugServer::PauseOnAllExceptions;
    else if (stringPauseState == "uncaught")
        pauseState = ScriptDebugServer::PauseOnUncaughtExceptions;
    else {
        *errorString = "Unknown pause on exceptions mode: " + stringPauseState;
        return;
    }
    scriptDebugServer().setPauseOnExceptionsState(static_cast<ScriptDebugServer::PauseOnExceptionsState>(pauseState));
    if (scriptDebugServer().pauseOnExceptionsState() != pauseState)
        *errorString = "Internal error. Could not change pause on exceptions state";
}

void InspectorDebuggerAgent::evaluateOnCallFrame(ErrorString* errorString, const String& callFrameId, const String& expression, const String* const objectGroup, const bool* const includeCommandLineAPI, RefPtr<InspectorObject>* result, bool* wasThrown)
{
    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptForObjectId(callFrameId);
    if (injectedScript.hasNoValue()) {
        *errorString = "Inspected frame has gone";
        return;
    }
    injectedScript.evaluateOnCallFrame(errorString, m_currentCallStack, callFrameId, expression, objectGroup ? *objectGroup : "", includeCommandLineAPI ? *includeCommandLineAPI : false, result, wasThrown);
}

PassRefPtr<InspectorArray> InspectorDebuggerAgent::currentCallFrames()
{
    if (!m_pausedScriptState)
        return InspectorArray::create();
    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptFor(m_pausedScriptState);
    if (injectedScript.hasNoValue()) {
        ASSERT_NOT_REACHED();
        return InspectorArray::create();
    }
    return injectedScript.wrapCallFrames(m_currentCallStack);
}

// JavaScriptDebugListener functions

void InspectorDebuggerAgent::didParseSource(const String& sourceID, const String& url, const String& data, int startLine, int startColumn, int endLine, int endColumn, bool isContentScript)
{
    // Don't send script content to the front end until it's really needed.
    m_frontend->scriptParsed(sourceID, url, startLine, startColumn, endLine, endColumn, isContentScript);

    m_scripts.set(sourceID, Script(url, data, startLine, startColumn, endLine, endColumn));

    if (url.isEmpty())
        return;

    RefPtr<InspectorObject> breakpointsCookie = m_inspectorState->getObject(DebuggerAgentState::javaScriptBreakpoints);
    for (InspectorObject::iterator it = breakpointsCookie->begin(); it != breakpointsCookie->end(); ++it) {
        RefPtr<InspectorObject> breakpointObject = it->second->asObject();
        String breakpointURL;
        breakpointObject->getString("url", &breakpointURL);
        if (breakpointURL != url)
            continue;
        ScriptBreakpoint breakpoint;
        breakpointObject->getNumber("lineNumber", &breakpoint.lineNumber);
        breakpointObject->getNumber("columnNumber", &breakpoint.columnNumber);
        breakpointObject->getString("condition", &breakpoint.condition);
        RefPtr<InspectorObject> location = resolveBreakpoint(it->first, sourceID, breakpoint);
        if (location)
            m_frontend->breakpointResolved(it->first, location);
    }
}

void InspectorDebuggerAgent::failedToParseSource(const String& url, const String& data, int firstLine, int errorLine, const String& errorMessage)
{
    m_frontend->scriptFailedToParse(url, data, firstLine, errorLine, errorMessage);
}

void InspectorDebuggerAgent::didPause(ScriptState* scriptState, const ScriptValue& callFrames, const ScriptValue& exception)
{
    ASSERT(scriptState && !m_pausedScriptState);
    m_pausedScriptState = scriptState;
    m_currentCallStack = callFrames;

    if (!m_breakProgramDetails)
        m_breakProgramDetails = InspectorObject::create();
    m_breakProgramDetails->setValue("callFrames", currentCallFrames());

    if (!exception.hasNoValue()) {
        InjectedScript injectedScript = m_injectedScriptManager->injectedScriptFor(scriptState);
        if (!injectedScript.hasNoValue())
            m_breakProgramDetails->setValue("exception", injectedScript.wrapObject(exception, "backtrace"));
    }

    m_frontend->paused(m_breakProgramDetails);
    m_javaScriptPauseScheduled = false;

    if (!m_continueToLocationBreakpointId.isEmpty()) {
        scriptDebugServer().removeBreakpoint(m_continueToLocationBreakpointId);
        m_continueToLocationBreakpointId = "";
    }
}

void InspectorDebuggerAgent::didContinue()
{
    m_pausedScriptState = 0;
    m_currentCallStack = ScriptValue();
    m_breakProgramDetails = 0;
    m_frontend->resumed();
}

void InspectorDebuggerAgent::breakProgram(DebuggerEventType type, PassRefPtr<InspectorValue> data)
{
    m_breakProgramDetails = InspectorObject::create();
    m_breakProgramDetails->setNumber("eventType", type);
    m_breakProgramDetails->setValue("eventData", data);
    scriptDebugServer().breakProgram();
}

void InspectorDebuggerAgent::clear()
{
    m_pausedScriptState = 0;
    m_currentCallStack = ScriptValue();
    m_scripts.clear();
    m_breakpointIdToDebugServerBreakpointIds.clear();
    m_continueToLocationBreakpointId = String();
    m_breakProgramDetails.clear();
    m_javaScriptPauseScheduled = false;
}

} // namespace WebCore

#endif // ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(INSPECTOR)
