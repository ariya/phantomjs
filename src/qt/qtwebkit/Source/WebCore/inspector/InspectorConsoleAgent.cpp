/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "config.h"

#if ENABLE(INSPECTOR)
#include "InspectorConsoleAgent.h"

#include "InstrumentingAgents.h"
#include "Console.h"
#include "ConsoleMessage.h"
#include "DOMWindow.h"
#include "InjectedScriptHost.h"
#include "InjectedScriptManager.h"
#include "InspectorFrontend.h"
#include "InspectorState.h"
#include "ResourceError.h"
#include "ResourceResponse.h"
#include "ScriptArguments.h"
#include "ScriptCallFrame.h"
#include "ScriptCallStack.h"
#include "ScriptCallStackFactory.h"
#include "ScriptController.h"
#include "ScriptObject.h"
#include "ScriptProfiler.h"
#include <wtf/CurrentTime.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

static const unsigned maximumConsoleMessages = 1000;
static const int expireConsoleMessagesStep = 100;

namespace ConsoleAgentState {
static const char monitoringXHR[] = "monitoringXHR";
static const char consoleMessagesEnabled[] = "consoleMessagesEnabled";
}

int InspectorConsoleAgent::s_enabledAgentCount = 0;

InspectorConsoleAgent::InspectorConsoleAgent(InstrumentingAgents* instrumentingAgents, InspectorCompositeState* state, InjectedScriptManager* injectedScriptManager)
    : InspectorBaseAgent<InspectorConsoleAgent>("Console", instrumentingAgents, state)
    , m_injectedScriptManager(injectedScriptManager)
    , m_frontend(0)
    , m_previousMessage(0)
    , m_expiredConsoleMessageCount(0)
    , m_enabled(false)
{
    m_instrumentingAgents->setInspectorConsoleAgent(this);
}

InspectorConsoleAgent::~InspectorConsoleAgent()
{
    m_instrumentingAgents->setInspectorConsoleAgent(0);
    m_instrumentingAgents = 0;
    m_state = 0;
    m_injectedScriptManager = 0;
}

void InspectorConsoleAgent::enable(ErrorString*)
{
    if (m_enabled)
        return;
    m_enabled = true;
    if (!s_enabledAgentCount)
        ScriptController::setCaptureCallStackForUncaughtExceptions(true);
    ++s_enabledAgentCount;

    m_state->setBoolean(ConsoleAgentState::consoleMessagesEnabled, true);

    if (m_expiredConsoleMessageCount) {
        ConsoleMessage expiredMessage(!isWorkerAgent(), OtherMessageSource, LogMessageType, WarningMessageLevel, String::format("%d console messages are not shown.", m_expiredConsoleMessageCount));
        expiredMessage.addToFrontend(m_frontend, m_injectedScriptManager, false);
    }

    size_t messageCount = m_consoleMessages.size();
    for (size_t i = 0; i < messageCount; ++i)
        m_consoleMessages[i]->addToFrontend(m_frontend, m_injectedScriptManager, false);
}

void InspectorConsoleAgent::disable(ErrorString*)
{
    if (!m_enabled)
        return;
    m_enabled = false;
    if (!(--s_enabledAgentCount))
        ScriptController::setCaptureCallStackForUncaughtExceptions(false);
    m_state->setBoolean(ConsoleAgentState::consoleMessagesEnabled, false);
}

void InspectorConsoleAgent::clearMessages(ErrorString*)
{
    m_consoleMessages.clear();
    m_expiredConsoleMessageCount = 0;
    m_previousMessage = 0;
    m_injectedScriptManager->releaseObjectGroup("console");
    if (m_frontend && m_enabled)
        m_frontend->messagesCleared();
}

void InspectorConsoleAgent::reset()
{
    ErrorString error;
    clearMessages(&error);
    m_times.clear();
    m_counts.clear();
}

void InspectorConsoleAgent::restore()
{
    if (m_state->getBoolean(ConsoleAgentState::consoleMessagesEnabled)) {
        m_frontend->messagesCleared();
        ErrorString error;
        enable(&error);
    }
}

void InspectorConsoleAgent::setFrontend(InspectorFrontend* frontend)
{
    m_frontend = frontend->console();
}

void InspectorConsoleAgent::clearFrontend()
{
    m_frontend = 0;
    String errorString;
    disable(&errorString);
}

void InspectorConsoleAgent::addMessageToConsole(MessageSource source, MessageType type, MessageLevel level, const String& message, ScriptCallStack* callStack, unsigned long requestIdentifier)
{
    if (!developerExtrasEnabled())
        return;

    if (type == ClearMessageType) {
        ErrorString error;
        clearMessages(&error);
    }

    addConsoleMessage(adoptPtr(new ConsoleMessage(!isWorkerAgent(), source, type, level, message, callStack, requestIdentifier)));
}

void InspectorConsoleAgent::addMessageToConsole(MessageSource source, MessageType type, MessageLevel level, const String& message, ScriptState* state, ScriptArguments* arguments, unsigned long requestIdentifier)
{
    if (!developerExtrasEnabled())
        return;

    if (type == ClearMessageType) {
        ErrorString error;
        clearMessages(&error);
    }

    addConsoleMessage(adoptPtr(new ConsoleMessage(!isWorkerAgent(), source, type, level, message, arguments, state, requestIdentifier)));
}

void InspectorConsoleAgent::addMessageToConsole(MessageSource source, MessageType type, MessageLevel level, const String& message, const String& scriptId, unsigned lineNumber, unsigned columnNumber, ScriptState* state, unsigned long requestIdentifier)
{
    if (!developerExtrasEnabled())
        return;

    if (type == ClearMessageType) {
        ErrorString error;
        clearMessages(&error);
    }

    bool canGenerateCallStack = !isWorkerAgent() && m_frontend;
    addConsoleMessage(adoptPtr(new ConsoleMessage(canGenerateCallStack, source, type, level, message, scriptId, lineNumber, columnNumber, state, requestIdentifier)));
}

Vector<unsigned> InspectorConsoleAgent::consoleMessageArgumentCounts()
{
    Vector<unsigned> result(m_consoleMessages.size());
    for (size_t i = 0; i < m_consoleMessages.size(); i++)
        result[i] = m_consoleMessages[i]->argumentCount();
    return result;
}

void InspectorConsoleAgent::startTiming(const String& title)
{
    // Follow Firebug's behavior of requiring a title that is not null or
    // undefined for timing functions
    if (title.isNull())
        return;

    m_times.add(title, monotonicallyIncreasingTime());
}

void InspectorConsoleAgent::stopTiming(const String& title, PassRefPtr<ScriptCallStack> callStack)
{
    // Follow Firebug's behavior of requiring a title that is not null or
    // undefined for timing functions
    if (title.isNull())
        return;

    HashMap<String, double>::iterator it = m_times.find(title);
    if (it == m_times.end())
        return;

    double startTime = it->value;
    m_times.remove(it);

    double elapsed = monotonicallyIncreasingTime() - startTime;
    String message = title + String::format(": %.3fms", elapsed * 1000);
    addMessageToConsole(ConsoleAPIMessageSource, TimingMessageType, DebugMessageLevel, message, callStack.get());
}

void InspectorConsoleAgent::count(ScriptState* state, PassRefPtr<ScriptArguments> arguments)
{
    RefPtr<ScriptCallStack> callStack(createScriptCallStackForConsole(state));
    const ScriptCallFrame& lastCaller = callStack->at(0);
    // Follow Firebug's behavior of counting with null and undefined title in
    // the same bucket as no argument
    String title;
    arguments->getFirstArgumentAsString(title);
    String identifier = title + '@' + lastCaller.sourceURL() + ':' + String::number(lastCaller.lineNumber());

    HashMap<String, unsigned>::iterator it = m_counts.find(identifier);
    int count;
    if (it == m_counts.end())
        count = 1;
    else {
        count = it->value + 1;
        m_counts.remove(it);
    }

    m_counts.add(identifier, count);

    String message = title + ": " + String::number(count);
    addMessageToConsole(ConsoleAPIMessageSource, LogMessageType, DebugMessageLevel, message, callStack.get());
}

void InspectorConsoleAgent::frameWindowDiscarded(DOMWindow* window)
{
    size_t messageCount = m_consoleMessages.size();
    for (size_t i = 0; i < messageCount; ++i)
        m_consoleMessages[i]->windowCleared(window);
    m_injectedScriptManager->discardInjectedScriptsFor(window);
}

void InspectorConsoleAgent::didFinishXHRLoading(unsigned long requestIdentifier, const String& url, const String& sendURL, unsigned sendLineNumber)
{
    if (!developerExtrasEnabled())
        return;
    if (m_frontend && m_state->getBoolean(ConsoleAgentState::monitoringXHR)) {
        String message = "XHR finished loading: \"" + url + "\".";
        // FIXME: <http://webkit.org/b/114316> InspectorConsoleAgent::didFinishXHRLoading ConsoleMessage should include a column number
        addMessageToConsole(NetworkMessageSource, LogMessageType, DebugMessageLevel, message, sendURL, sendLineNumber, 0, 0, requestIdentifier);
    }
}

void InspectorConsoleAgent::didReceiveResponse(unsigned long requestIdentifier, const ResourceResponse& response)
{
    if (!developerExtrasEnabled())
        return;

    if (response.httpStatusCode() >= 400) {
        String message = "Failed to load resource: the server responded with a status of " + String::number(response.httpStatusCode()) + " (" + response.httpStatusText() + ')';
        addMessageToConsole(NetworkMessageSource, LogMessageType, ErrorMessageLevel, message, response.url().string(), 0, 0, 0, requestIdentifier);
    }
}

void InspectorConsoleAgent::didFailLoading(unsigned long requestIdentifier, const ResourceError& error)
{
    if (!developerExtrasEnabled())
        return;
    if (error.isCancellation()) // Report failures only.
        return;
    StringBuilder message;
    message.appendLiteral("Failed to load resource");
    if (!error.localizedDescription().isEmpty()) {
        message.appendLiteral(": ");
        message.append(error.localizedDescription());
    }
    addMessageToConsole(NetworkMessageSource, LogMessageType, ErrorMessageLevel, message.toString(), error.failingURL(), 0, 0, 0, requestIdentifier);
}

void InspectorConsoleAgent::setMonitoringXHREnabled(ErrorString*, bool enabled)
{
    m_state->setBoolean(ConsoleAgentState::monitoringXHR, enabled);
}

static bool isGroupMessage(MessageType type)
{
    return type == StartGroupMessageType
        || type ==  StartGroupCollapsedMessageType
        || type == EndGroupMessageType;
}

void InspectorConsoleAgent::addConsoleMessage(PassOwnPtr<ConsoleMessage> consoleMessage)
{
    ASSERT(developerExtrasEnabled());
    ASSERT_ARG(consoleMessage, consoleMessage);

    if (m_previousMessage && !isGroupMessage(m_previousMessage->type()) && m_previousMessage->isEqual(consoleMessage.get())) {
        m_previousMessage->incrementCount();
        if (m_frontend && m_enabled)
            m_previousMessage->updateRepeatCountInConsole(m_frontend);
    } else {
        m_previousMessage = consoleMessage.get();
        m_consoleMessages.append(consoleMessage);
        if (m_frontend && m_enabled)
            m_previousMessage->addToFrontend(m_frontend, m_injectedScriptManager, true);
    }

    if (!m_frontend && m_consoleMessages.size() >= maximumConsoleMessages) {
        m_expiredConsoleMessageCount += expireConsoleMessagesStep;
        m_consoleMessages.remove(0, expireConsoleMessagesStep);
    }
}

class InspectableHeapObject : public InjectedScriptHost::InspectableObject {
public:
    explicit InspectableHeapObject(int heapObjectId) : m_heapObjectId(heapObjectId) { }
    virtual ScriptValue get(ScriptState*)
    {
        return ScriptProfiler::objectByHeapObjectId(m_heapObjectId);
    }
private:
    int m_heapObjectId;
};

void InspectorConsoleAgent::addInspectedHeapObject(ErrorString*, int inspectedHeapObjectId)
{
    m_injectedScriptManager->injectedScriptHost()->addInspectedObject(adoptPtr(new InspectableHeapObject(inspectedHeapObjectId)));
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
