/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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
#include "ScriptDebugServer.h"

#if ENABLE(JAVASCRIPT_DEBUGGER)

#include "EventLoop.h"
#include "Frame.h"
#include "JSJavaScriptCallFrame.h"
#include "JavaScriptCallFrame.h"
#include "ScriptBreakpoint.h"
#include "ScriptDebugListener.h"
#include "ScriptValue.h"
#include <debugger/DebuggerCallFrame.h>
#include <parser/SourceProvider.h>
#include <runtime/JSLock.h>
#include <wtf/MainThread.h>
#include <wtf/text/StringConcatenate.h>

using namespace JSC;

namespace WebCore {

ScriptDebugServer::ScriptDebugServer()
    : m_callingListeners(false)
    , m_pauseOnExceptionsState(DontPauseOnExceptions)
    , m_pauseOnNextStatement(false)
    , m_paused(false)
    , m_doneProcessingDebuggerEvents(true)
    , m_breakpointsActivated(true)
    , m_pauseOnCallFrame(0)
    , m_recompileTimer(this, &ScriptDebugServer::recompileAllJSFunctions)
{
}

ScriptDebugServer::~ScriptDebugServer()
{
    deleteAllValues(m_pageListenersMap);
}

String ScriptDebugServer::setBreakpoint(const String& sourceID, const ScriptBreakpoint& scriptBreakpoint, int* actualLineNumber, int* actualColumnNumber)
{
    intptr_t sourceIDValue = sourceID.toIntPtr();
    if (!sourceIDValue)
        return "";
    SourceIdToBreakpointsMap::iterator it = m_sourceIdToBreakpoints.find(sourceIDValue);
    if (it == m_sourceIdToBreakpoints.end())
        it = m_sourceIdToBreakpoints.set(sourceIDValue, LineToBreakpointMap()).first;
    if (it->second.contains(scriptBreakpoint.lineNumber + 1))
        return "";
    it->second.set(scriptBreakpoint.lineNumber + 1, scriptBreakpoint);
    *actualLineNumber = scriptBreakpoint.lineNumber;
    // FIXME(WK53003): implement setting breakpoints by line:column.
    *actualColumnNumber = 0;
    return makeString(sourceID, ":", String::number(scriptBreakpoint.lineNumber));
}

void ScriptDebugServer::removeBreakpoint(const String& breakpointId)
{
    Vector<String> tokens;
    breakpointId.split(":", tokens);
    if (tokens.size() != 2)
        return;
    bool success;
    intptr_t sourceIDValue = tokens[0].toIntPtr(&success);
    if (!success)
        return;
    unsigned lineNumber = tokens[1].toUInt(&success);
    if (!success)
        return;
    SourceIdToBreakpointsMap::iterator it = m_sourceIdToBreakpoints.find(sourceIDValue);
    if (it != m_sourceIdToBreakpoints.end())
        it->second.remove(lineNumber + 1);
}

bool ScriptDebugServer::hasBreakpoint(intptr_t sourceID, const TextPosition0& position) const
{
    if (!m_breakpointsActivated)
        return false;

    SourceIdToBreakpointsMap::const_iterator it = m_sourceIdToBreakpoints.find(sourceID);
    if (it == m_sourceIdToBreakpoints.end())
        return false;
    int lineNumber = position.m_line.convertAsOneBasedInt();
    if (lineNumber <= 0)
        return false;
    LineToBreakpointMap::const_iterator breakIt = it->second.find(lineNumber);
    if (breakIt == it->second.end())
        return false;

    // An empty condition counts as no condition which is equivalent to "true".
    if (breakIt->second.condition.isEmpty())
        return true;

    JSValue exception;
    JSValue result = m_currentCallFrame->evaluate(stringToUString(breakIt->second.condition), exception);
    if (exception) {
        // An erroneous condition counts as "false".
        return false;
    }
    return result.toBoolean(m_currentCallFrame->scopeChain()->globalObject->globalExec());
}

void ScriptDebugServer::clearBreakpoints()
{
    m_sourceIdToBreakpoints.clear();
}

void ScriptDebugServer::setBreakpointsActivated(bool activated)
{
    m_breakpointsActivated = activated;
}

void ScriptDebugServer::setPauseOnExceptionsState(PauseOnExceptionsState pause)
{
    m_pauseOnExceptionsState = pause;
}

void ScriptDebugServer::setPauseOnNextStatement(bool pause)
{
    m_pauseOnNextStatement = pause;
}

void ScriptDebugServer::breakProgram()
{
    // FIXME(WK43332): implement this.
}

void ScriptDebugServer::continueProgram()
{
    if (!m_paused)
        return;

    m_pauseOnNextStatement = false;
    m_doneProcessingDebuggerEvents = true;
}

void ScriptDebugServer::stepIntoStatement()
{
    if (!m_paused)
        return;

    m_pauseOnNextStatement = true;
    m_doneProcessingDebuggerEvents = true;
}

void ScriptDebugServer::stepOverStatement()
{
    if (!m_paused)
        return;

    m_pauseOnCallFrame = m_currentCallFrame.get();
    m_doneProcessingDebuggerEvents = true;
}

void ScriptDebugServer::stepOutOfFunction()
{
    if (!m_paused)
        return;

    m_pauseOnCallFrame = m_currentCallFrame ? m_currentCallFrame->caller() : 0;
    m_doneProcessingDebuggerEvents = true;
}

bool ScriptDebugServer::editScriptSource(const String&, const String&, String*, ScriptValue*)
{
    // FIXME(40300): implement this.
    return false;
}

void ScriptDebugServer::dispatchDidPause(ScriptDebugListener* listener)
{
    ASSERT(m_paused);
    JSGlobalObject* globalObject = m_currentCallFrame->scopeChain()->globalObject.get();
    ScriptState* state = globalObject->globalExec();
    JSValue jsCallFrame;
    {
        if (m_currentCallFrame->isValid() && globalObject->inherits(&JSDOMGlobalObject::s_info)) {
            JSDOMGlobalObject* domGlobalObject = static_cast<JSDOMGlobalObject*>(globalObject);
            JSLock lock(SilenceAssertionsOnly);
            jsCallFrame = toJS(state, domGlobalObject, m_currentCallFrame.get());
        } else
            jsCallFrame = jsUndefined();
    }
    listener->didPause(state, ScriptValue(state->globalData(), jsCallFrame), ScriptValue());
}

void ScriptDebugServer::dispatchDidContinue(ScriptDebugListener* listener)
{
    listener->didContinue();
}

void ScriptDebugServer::dispatchDidParseSource(const ListenerSet& listeners, SourceProvider* sourceProvider, bool isContentScript)
{
    String sourceID = ustringToString(JSC::UString::number(sourceProvider->asID()));
    String url = ustringToString(sourceProvider->url());
    String data = ustringToString(JSC::UString(sourceProvider->data(), sourceProvider->length()));
    int lineOffset = sourceProvider->startPosition().m_line.convertAsZeroBasedInt();
    int columnOffset = sourceProvider->startPosition().m_column.convertAsZeroBasedInt();

    int lineCount = 1;
    int lastLineStart = 0;
    for (size_t i = 0; i < data.length() - 1; ++i) {
        if (data[i] == '\n') {
            lineCount += 1;
            lastLineStart = i + 1;
        }
    }

    int endLine = lineOffset + lineCount - 1;
    int endColumn;
    if (lineCount == 1)
        endColumn = data.length() + columnOffset;
    else
        endColumn = data.length() - lastLineStart;

    Vector<ScriptDebugListener*> copy;
    copyToVector(listeners, copy);
    for (size_t i = 0; i < copy.size(); ++i)
        copy[i]->didParseSource(sourceID, url, data, lineOffset, columnOffset, endLine, endColumn, isContentScript);
}

void ScriptDebugServer::dispatchFailedToParseSource(const ListenerSet& listeners, SourceProvider* sourceProvider, int errorLine, const String& errorMessage)
{
    String url = ustringToString(sourceProvider->url());
    String data = ustringToString(JSC::UString(sourceProvider->data(), sourceProvider->length()));
    int firstLine = sourceProvider->startPosition().m_line.oneBasedInt();

    Vector<ScriptDebugListener*> copy;
    copyToVector(listeners, copy);
    for (size_t i = 0; i < copy.size(); ++i)
        copy[i]->failedToParseSource(url, data, firstLine, errorLine, errorMessage);
}

static bool isContentScript(ExecState* exec)
{
    return currentWorld(exec) != mainThreadNormalWorld();
}

void ScriptDebugServer::detach(JSGlobalObject* globalObject)
{
    // If we're detaching from the currently executing global object, manually tear down our
    // stack, since we won't get further debugger callbacks to do so. Also, resume execution,
    // since there's no point in staying paused once a window closes.
    if (m_currentCallFrame && m_currentCallFrame->dynamicGlobalObject() == globalObject) {
        m_currentCallFrame = 0;
        m_pauseOnCallFrame = 0;
        continueProgram();
    }
    Debugger::detach(globalObject);
}

void ScriptDebugServer::sourceParsed(ExecState* exec, SourceProvider* sourceProvider, int errorLine, const UString& errorMessage)
{
    if (m_callingListeners)
        return;

    ListenerSet* listeners = getListenersForGlobalObject(exec->lexicalGlobalObject());
    if (!listeners)
        return;
    ASSERT(!listeners->isEmpty());

    m_callingListeners = true;

    bool isError = errorLine != -1;
    if (isError)
        dispatchFailedToParseSource(*listeners, sourceProvider, errorLine, ustringToString(errorMessage));
    else
        dispatchDidParseSource(*listeners, sourceProvider, isContentScript(exec));

    m_callingListeners = false;
}

void ScriptDebugServer::dispatchFunctionToListeners(const ListenerSet& listeners, JavaScriptExecutionCallback callback)
{
    Vector<ScriptDebugListener*> copy;
    copyToVector(listeners, copy);
    for (size_t i = 0; i < copy.size(); ++i)
        (this->*callback)(copy[i]);
}

void ScriptDebugServer::dispatchFunctionToListeners(JavaScriptExecutionCallback callback, JSGlobalObject* globalObject)
{
    if (m_callingListeners)
        return;

    m_callingListeners = true;

    if (ListenerSet* listeners = getListenersForGlobalObject(globalObject)) {
        ASSERT(!listeners->isEmpty());
        dispatchFunctionToListeners(*listeners, callback);
    }

    m_callingListeners = false;
}

void ScriptDebugServer::createCallFrameAndPauseIfNeeded(const DebuggerCallFrame& debuggerCallFrame, intptr_t sourceID, int lineNumber)
{
    TextPosition0 textPosition(WTF::OneBasedNumber::fromOneBasedInt(lineNumber).convertToZeroBased(), WTF::ZeroBasedNumber::base());
    m_currentCallFrame = JavaScriptCallFrame::create(debuggerCallFrame, m_currentCallFrame, sourceID, textPosition);
    pauseIfNeeded(debuggerCallFrame.dynamicGlobalObject());
}

void ScriptDebugServer::updateCallFrameAndPauseIfNeeded(const DebuggerCallFrame& debuggerCallFrame, intptr_t sourceID, int lineNumber)
{
    ASSERT(m_currentCallFrame);
    if (!m_currentCallFrame)
        return;

    TextPosition0 textPosition(WTF::OneBasedNumber::fromOneBasedInt(lineNumber).convertToZeroBased(), WTF::ZeroBasedNumber::base());
    m_currentCallFrame->update(debuggerCallFrame, sourceID, textPosition);
    pauseIfNeeded(debuggerCallFrame.dynamicGlobalObject());
}

void ScriptDebugServer::pauseIfNeeded(JSGlobalObject* dynamicGlobalObject)
{
    if (m_paused)
        return;
 
    if (!getListenersForGlobalObject(dynamicGlobalObject))
        return;

    bool pauseNow = m_pauseOnNextStatement;
    pauseNow |= (m_pauseOnCallFrame == m_currentCallFrame);
    pauseNow |= hasBreakpoint(m_currentCallFrame->sourceID(), m_currentCallFrame->position());
    if (!pauseNow)
        return;

    m_pauseOnCallFrame = 0;
    m_pauseOnNextStatement = false;
    m_paused = true;

    dispatchFunctionToListeners(&ScriptDebugServer::dispatchDidPause, dynamicGlobalObject);
    didPause(dynamicGlobalObject);

    TimerBase::fireTimersInNestedEventLoop();

    EventLoop loop;
    m_doneProcessingDebuggerEvents = false;
    while (!m_doneProcessingDebuggerEvents && !loop.ended())
        loop.cycle();

    didContinue(dynamicGlobalObject);
    dispatchFunctionToListeners(&ScriptDebugServer::dispatchDidContinue, dynamicGlobalObject);

    m_paused = false;
}

void ScriptDebugServer::callEvent(const DebuggerCallFrame& debuggerCallFrame, intptr_t sourceID, int lineNumber)
{
    if (!m_paused)
        createCallFrameAndPauseIfNeeded(debuggerCallFrame, sourceID, lineNumber);
}

void ScriptDebugServer::atStatement(const DebuggerCallFrame& debuggerCallFrame, intptr_t sourceID, int lineNumber)
{
    if (!m_paused)
        updateCallFrameAndPauseIfNeeded(debuggerCallFrame, sourceID, lineNumber);
}

void ScriptDebugServer::returnEvent(const DebuggerCallFrame& debuggerCallFrame, intptr_t sourceID, int lineNumber)
{
    if (m_paused)
        return;

    updateCallFrameAndPauseIfNeeded(debuggerCallFrame, sourceID, lineNumber);

    // detach may have been called during pauseIfNeeded
    if (!m_currentCallFrame)
        return;

    // Treat stepping over a return statement like stepping out.
    if (m_currentCallFrame == m_pauseOnCallFrame)
        m_pauseOnCallFrame = m_currentCallFrame->caller();
    m_currentCallFrame = m_currentCallFrame->caller();
}

void ScriptDebugServer::exception(const DebuggerCallFrame& debuggerCallFrame, intptr_t sourceID, int lineNumber, bool hasHandler)
{
    if (m_paused)
        return;

    if (m_pauseOnExceptionsState == PauseOnAllExceptions || (m_pauseOnExceptionsState == PauseOnUncaughtExceptions && !hasHandler))
        m_pauseOnNextStatement = true;

    updateCallFrameAndPauseIfNeeded(debuggerCallFrame, sourceID, lineNumber);
}

void ScriptDebugServer::willExecuteProgram(const DebuggerCallFrame& debuggerCallFrame, intptr_t sourceID, int lineNumber)
{
    if (!m_paused)
        createCallFrameAndPauseIfNeeded(debuggerCallFrame, sourceID, lineNumber);
}

void ScriptDebugServer::didExecuteProgram(const DebuggerCallFrame& debuggerCallFrame, intptr_t sourceID, int lineNumber)
{
    if (m_paused)
        return;

    updateCallFrameAndPauseIfNeeded(debuggerCallFrame, sourceID, lineNumber);

    // Treat stepping over the end of a program like stepping out.
    if (m_currentCallFrame == m_pauseOnCallFrame)
        m_pauseOnCallFrame = m_currentCallFrame->caller();
    m_currentCallFrame = m_currentCallFrame->caller();
}

void ScriptDebugServer::didReachBreakpoint(const DebuggerCallFrame& debuggerCallFrame, intptr_t sourceID, int lineNumber)
{
    if (m_paused)
        return;

    m_pauseOnNextStatement = true;
    updateCallFrameAndPauseIfNeeded(debuggerCallFrame, sourceID, lineNumber);
}

void ScriptDebugServer::recompileAllJSFunctionsSoon()
{
    m_recompileTimer.startOneShot(0);
}

} // namespace WebCore

#endif // ENABLE(JAVASCRIPT_DEBUGGER)
