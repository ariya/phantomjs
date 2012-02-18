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
#include "InspectorConsoleAgent.h"

#if ENABLE(INSPECTOR)
#include "InstrumentingAgents.h"
#include "Console.h"
#include "ConsoleMessage.h"
#include "InjectedScriptHost.h"
#include "InjectedScriptManager.h"
#include "InspectorAgent.h"
#include "InspectorDOMAgent.h"
#include "InspectorFrontend.h"
#include "InspectorState.h"
#include "ResourceError.h"
#include "ResourceResponse.h"
#include "ScriptArguments.h"
#include "ScriptCallFrame.h"
#include "ScriptCallStack.h"
#include <wtf/CurrentTime.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/StringConcatenate.h>

namespace WebCore {

static const unsigned maximumConsoleMessages = 1000;
static const int expireConsoleMessagesStep = 100;

namespace ConsoleAgentState {
static const char monitoringXHR[] = "monitoringXHR";
static const char consoleMessagesEnabled[] = "consoleMessagesEnabled";
}

InspectorConsoleAgent::InspectorConsoleAgent(InstrumentingAgents* instrumentingAgents, InspectorAgent* inspectorAgent, InspectorState* state, InjectedScriptManager* injectedScriptManager, InspectorDOMAgent* domAgent)
    : m_instrumentingAgents(instrumentingAgents)
    , m_inspectorAgent(inspectorAgent)
    , m_inspectorState(state)
    , m_injectedScriptManager(injectedScriptManager)
    , m_inspectorDOMAgent(domAgent)
    , m_frontend(0)
    , m_previousMessage(0)
    , m_expiredConsoleMessageCount(0)
{
    m_instrumentingAgents->setInspectorConsoleAgent(this);
}

InspectorConsoleAgent::~InspectorConsoleAgent()
{
    m_instrumentingAgents->setInspectorConsoleAgent(0);
    m_instrumentingAgents = 0;
    m_inspectorAgent = 0;
    m_inspectorState = 0;
    m_injectedScriptManager = 0;
    m_inspectorDOMAgent = 0;
}

void InspectorConsoleAgent::enable(ErrorString*, int* consoleMessageExpireCount)
{
    *consoleMessageExpireCount = m_expiredConsoleMessageCount;

    m_inspectorState->setBoolean(ConsoleAgentState::consoleMessagesEnabled, true);

    size_t messageCount = m_consoleMessages.size();
    for (size_t i = 0; i < messageCount; ++i)
        m_consoleMessages[i]->addToFrontend(m_frontend, m_injectedScriptManager);
}

void InspectorConsoleAgent::disable(ErrorString*)
{
    m_inspectorState->setBoolean(ConsoleAgentState::consoleMessagesEnabled, false);
}

void InspectorConsoleAgent::clearConsoleMessages(ErrorString*)
{
    m_consoleMessages.clear();
    m_expiredConsoleMessageCount = 0;
    m_previousMessage = 0;
    m_injectedScriptManager->releaseObjectGroup("console");
    m_inspectorDOMAgent->releaseDanglingNodes();
    if (m_frontend)
        m_frontend->messagesCleared();
}

void InspectorConsoleAgent::reset()
{
    ErrorString error;
    clearConsoleMessages(&error);
    m_times.clear();
    m_counts.clear();
}

void InspectorConsoleAgent::setFrontend(InspectorFrontend* frontend)
{
    m_frontend = frontend->console();
}

void InspectorConsoleAgent::clearFrontend()
{
    m_frontend = 0;
}

void InspectorConsoleAgent::addMessageToConsole(MessageSource source, MessageType type, MessageLevel level, const String& message, PassRefPtr<ScriptArguments> arguments, PassRefPtr<ScriptCallStack> callStack)
{
    if (!m_inspectorAgent->enabled())
        return;
    addConsoleMessage(adoptPtr(new ConsoleMessage(source, type, level, message, arguments, callStack)));
}

void InspectorConsoleAgent::addMessageToConsole(MessageSource source, MessageType type, MessageLevel level, const String& message, unsigned lineNumber, const String& sourceID)
{
    if (!m_inspectorAgent->enabled())
        return;
    addConsoleMessage(adoptPtr(new ConsoleMessage(source, type, level, message, lineNumber, sourceID)));
}

void InspectorConsoleAgent::startTiming(const String& title)
{
    // Follow Firebug's behavior of requiring a title that is not null or
    // undefined for timing functions
    if (title.isNull())
        return;

    m_times.add(title, currentTime() * 1000);
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

    double startTime = it->second;
    m_times.remove(it);

    double elapsed = currentTime() * 1000 - startTime;
    String message = title + String::format(": %.0fms", elapsed);
    const ScriptCallFrame& lastCaller = callStack->at(0);
    addMessageToConsole(JSMessageSource, LogMessageType, LogMessageLevel, message, lastCaller.lineNumber(), lastCaller.sourceURL());
}

void InspectorConsoleAgent::count(PassRefPtr<ScriptArguments> arguments, PassRefPtr<ScriptCallStack> callStack)
{
    const ScriptCallFrame& lastCaller = callStack->at(0);
    // Follow Firebug's behavior of counting with null and undefined title in
    // the same bucket as no argument
    String title;
    arguments->getFirstArgumentAsString(title);
    String identifier = makeString(title, '@', lastCaller.sourceURL(), ':', String::number(lastCaller.lineNumber()));

    HashMap<String, unsigned>::iterator it = m_counts.find(identifier);
    int count;
    if (it == m_counts.end())
        count = 1;
    else {
        count = it->second + 1;
        m_counts.remove(it);
    }

    m_counts.add(identifier, count);

    String message = makeString(title, ": ", String::number(count));
    addMessageToConsole(JSMessageSource, LogMessageType, LogMessageLevel, message, lastCaller.lineNumber(), lastCaller.sourceURL());
}

void InspectorConsoleAgent::resourceRetrievedByXMLHttpRequest(const String& url, const String& sendURL, unsigned sendLineNumber)
{
    if (!m_inspectorAgent->enabled())
        return;
    if (m_inspectorState->getBoolean(ConsoleAgentState::monitoringXHR))
        addMessageToConsole(JSMessageSource, LogMessageType, LogMessageLevel, "XHR finished loading: \"" + url + "\".", sendLineNumber, sendURL);
}

void InspectorConsoleAgent::didReceiveResponse(unsigned long identifier, const ResourceResponse& response)
{
    if (!m_inspectorAgent->enabled())
        return;

    if (response.httpStatusCode() >= 400) {
        String message = makeString("Failed to load resource: the server responded with a status of ", String::number(response.httpStatusCode()), " (", response.httpStatusText(), ')');
        addConsoleMessage(adoptPtr(new ConsoleMessage(OtherMessageSource, NetworkErrorMessageType, ErrorMessageLevel, message, response.url().string(), identifier)));
    }
}

void InspectorConsoleAgent::didFailLoading(unsigned long identifier, const ResourceError& error)
{
    if (!m_inspectorAgent->enabled())
        return;
    if (error.isCancellation()) // Report failures only.
        return;
    String message = "Failed to load resource";
    if (!error.localizedDescription().isEmpty())
        message += ": " + error.localizedDescription();
    addConsoleMessage(adoptPtr(new ConsoleMessage(OtherMessageSource, NetworkErrorMessageType, ErrorMessageLevel, message, error.failingURL(), identifier)));
}

void InspectorConsoleAgent::setMonitoringXHREnabled(ErrorString*, bool enabled)
{
    m_inspectorState->setBoolean(ConsoleAgentState::monitoringXHR, enabled);
}

void InspectorConsoleAgent::addInspectedNode(ErrorString*, int nodeId)
{
    Node* node = m_inspectorDOMAgent->nodeForId(nodeId);
    if (!node)
        return;
    m_injectedScriptManager->injectedScriptHost()->addInspectedNode(node);
}

void InspectorConsoleAgent::addConsoleMessage(PassOwnPtr<ConsoleMessage> consoleMessage)
{
    ASSERT(m_inspectorAgent->enabled());
    ASSERT_ARG(consoleMessage, consoleMessage);

    if (m_previousMessage && m_previousMessage->type() != EndGroupMessageType && m_previousMessage->isEqual(consoleMessage.get())) {
        m_previousMessage->incrementCount();
        if (m_inspectorState->getBoolean(ConsoleAgentState::consoleMessagesEnabled) && m_frontend)
            m_previousMessage->updateRepeatCountInConsole(m_frontend);
    } else {
        m_previousMessage = consoleMessage.get();
        m_consoleMessages.append(consoleMessage);
        if (m_inspectorState->getBoolean(ConsoleAgentState::consoleMessagesEnabled) && m_frontend)
            m_previousMessage->addToFrontend(m_frontend, m_injectedScriptManager);
    }

    if (!m_frontend && m_consoleMessages.size() >= maximumConsoleMessages) {
        m_expiredConsoleMessageCount += expireConsoleMessagesStep;
        m_consoleMessages.remove(0, expireConsoleMessagesStep);
    }
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
