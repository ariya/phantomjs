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

#if ENABLE(WORKERS) && ENABLE(INSPECTOR)

#include "InspectorWorkerAgent.h"

#include "InspectorFrontend.h"
#include "InspectorFrontendChannel.h"
#include "InspectorState.h"
#include "InspectorValues.h"
#include "InstrumentingAgents.h"
#include "KURL.h"
#include "WorkerGlobalScopeProxy.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

namespace WorkerAgentState {
static const char workerInspectionEnabled[] = "workerInspectionEnabled";
static const char autoconnectToWorkers[] = "autoconnectToWorkers";
};

class InspectorWorkerAgent::WorkerFrontendChannel : public WorkerGlobalScopeProxy::PageInspector {
    WTF_MAKE_FAST_ALLOCATED;
public:
    explicit WorkerFrontendChannel(InspectorFrontend* frontend, WorkerGlobalScopeProxy* proxy)
        : m_frontend(frontend)
        , m_proxy(proxy)
        , m_id(s_nextId++)
        , m_connected(false)
    {
    }
    virtual ~WorkerFrontendChannel()
    {
        disconnectFromWorkerGlobalScope();
    }

    int id() const { return m_id; }
    WorkerGlobalScopeProxy* proxy() const { return m_proxy; }

    void connectToWorkerGlobalScope()
    {
        if (m_connected)
            return;
        m_connected = true;
        m_proxy->connectToInspector(this);
    }

    void disconnectFromWorkerGlobalScope()
    {
        if (!m_connected)
            return;
        m_connected = false;
        m_proxy->disconnectFromInspector();
    }

private:
    // WorkerGlobalScopeProxy::PageInspector implementation
    virtual void dispatchMessageFromWorker(const String& message)
    {
        RefPtr<InspectorValue> value = InspectorValue::parseJSON(message);
        if (!value)
            return;
        RefPtr<InspectorObject> messageObject = value->asObject();
        if (!messageObject)
            return;
        m_frontend->worker()->dispatchMessageFromWorker(m_id, messageObject);
    }

    InspectorFrontend* m_frontend;
    WorkerGlobalScopeProxy* m_proxy;
    int m_id;
    bool m_connected;
    static int s_nextId;
};

int InspectorWorkerAgent::WorkerFrontendChannel::s_nextId = 1;

PassOwnPtr<InspectorWorkerAgent> InspectorWorkerAgent::create(InstrumentingAgents* instrumentingAgents, InspectorCompositeState* inspectorState)
{
    return adoptPtr(new InspectorWorkerAgent(instrumentingAgents, inspectorState));
}

InspectorWorkerAgent::InspectorWorkerAgent(InstrumentingAgents* instrumentingAgents, InspectorCompositeState* inspectorState)
    : InspectorBaseAgent<InspectorWorkerAgent>("Worker", instrumentingAgents, inspectorState)
    , m_inspectorFrontend(0)
{
    m_instrumentingAgents->setInspectorWorkerAgent(this);
}

InspectorWorkerAgent::~InspectorWorkerAgent()
{
    m_instrumentingAgents->setInspectorWorkerAgent(0);
}

void InspectorWorkerAgent::setFrontend(InspectorFrontend* frontend)
{
    m_inspectorFrontend = frontend;
}

void InspectorWorkerAgent::restore()
{
    if (m_state->getBoolean(WorkerAgentState::workerInspectionEnabled))
        createWorkerFrontendChannelsForExistingWorkers();
}

void InspectorWorkerAgent::clearFrontend()
{
    m_state->setBoolean(WorkerAgentState::autoconnectToWorkers, false);
    disable(0);
    m_inspectorFrontend = 0;
}

void InspectorWorkerAgent::enable(ErrorString*)
{
    m_state->setBoolean(WorkerAgentState::workerInspectionEnabled, true);
    if (!m_inspectorFrontend)
        return;
    createWorkerFrontendChannelsForExistingWorkers();
}

void InspectorWorkerAgent::disable(ErrorString*)
{
    m_state->setBoolean(WorkerAgentState::workerInspectionEnabled, false);
    if (!m_inspectorFrontend)
        return;
    destroyWorkerFrontendChannels();
}

void InspectorWorkerAgent::canInspectWorkers(ErrorString*, bool* result)
{
#if ENABLE(WORKERS)
    *result = true;
#else
    *result = false;
#endif
}

void InspectorWorkerAgent::connectToWorker(ErrorString* error, int workerId)
{
    WorkerFrontendChannel* channel = m_idToChannel.get(workerId);
    if (channel)
        channel->connectToWorkerGlobalScope();
    else
        *error = "Worker is gone";
}

void InspectorWorkerAgent::disconnectFromWorker(ErrorString* error, int workerId)
{
    WorkerFrontendChannel* channel = m_idToChannel.get(workerId);
    if (channel)
        channel->disconnectFromWorkerGlobalScope();
    else
        *error = "Worker is gone";
}

void InspectorWorkerAgent::sendMessageToWorker(ErrorString* error, int workerId, const RefPtr<InspectorObject>& message)
{
    WorkerFrontendChannel* channel = m_idToChannel.get(workerId);
    if (channel)
        channel->proxy()->sendMessageToInspector(message->toJSONString());
    else
        *error = "Worker is gone";
}

void InspectorWorkerAgent::setAutoconnectToWorkers(ErrorString*, bool value)
{
    m_state->setBoolean(WorkerAgentState::autoconnectToWorkers, value);
}

bool InspectorWorkerAgent::shouldPauseDedicatedWorkerOnStart()
{
    return m_state->getBoolean(WorkerAgentState::autoconnectToWorkers);
}

void InspectorWorkerAgent::didStartWorkerGlobalScope(WorkerGlobalScopeProxy* workerGlobalScopeProxy, const KURL& url)
{
    m_dedicatedWorkers.set(workerGlobalScopeProxy, url.string());
    if (m_inspectorFrontend && m_state->getBoolean(WorkerAgentState::workerInspectionEnabled))
        createWorkerFrontendChannel(workerGlobalScopeProxy, url.string());
}

void InspectorWorkerAgent::workerGlobalScopeTerminated(WorkerGlobalScopeProxy* proxy)
{
    m_dedicatedWorkers.remove(proxy);
    for (WorkerChannels::iterator it = m_idToChannel.begin(); it != m_idToChannel.end(); ++it) {
        if (proxy == it->value->proxy()) {
            m_inspectorFrontend->worker()->workerTerminated(it->key);
            delete it->value;
            m_idToChannel.remove(it);
            return;
        }
    }
}

void InspectorWorkerAgent::createWorkerFrontendChannelsForExistingWorkers()
{
    for (DedicatedWorkers::iterator it = m_dedicatedWorkers.begin(); it != m_dedicatedWorkers.end(); ++it)
        createWorkerFrontendChannel(it->key, it->value);
}

void InspectorWorkerAgent::destroyWorkerFrontendChannels()
{
    for (WorkerChannels::iterator it = m_idToChannel.begin(); it != m_idToChannel.end(); ++it) {
        it->value->disconnectFromWorkerGlobalScope();
        delete it->value;
    }
    m_idToChannel.clear();
}

void InspectorWorkerAgent::createWorkerFrontendChannel(WorkerGlobalScopeProxy* workerGlobalScopeProxy, const String& url)
{
    WorkerFrontendChannel* channel = new WorkerFrontendChannel(m_inspectorFrontend, workerGlobalScopeProxy);
    m_idToChannel.set(channel->id(), channel);

    ASSERT(m_inspectorFrontend);
    bool autoconnectToWorkers = m_state->getBoolean(WorkerAgentState::autoconnectToWorkers);
    if (autoconnectToWorkers)
        channel->connectToWorkerGlobalScope();
    m_inspectorFrontend->worker()->workerCreated(channel->id(), url, autoconnectToWorkers);
}

} // namespace WebCore

#endif // ENABLE(WORKERS) && ENABLE(INSPECTOR)
