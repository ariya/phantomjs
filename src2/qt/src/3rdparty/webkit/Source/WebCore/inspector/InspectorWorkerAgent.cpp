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
#include "InspectorValues.h"
#include "InstrumentingAgents.h"
#include "WorkerContextInspectorProxy.h"
#include "WorkerContextProxy.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class InspectorWorkerAgent::WorkerFrontendChannel : public InspectorFrontendChannel {
public:
    explicit WorkerFrontendChannel(InspectorFrontend* frontend, WorkerContextInspectorProxy* inspectorProxy)
        : m_frontend(frontend)
        , m_inspectorProxy(inspectorProxy)
        , m_id(s_nextId++)
    {
    }
    virtual ~WorkerFrontendChannel() { }

    int id() const { return m_id; }
    WorkerContextInspectorProxy* inspectorProxy() const { return m_inspectorProxy; }

private:
    virtual bool sendMessageToFrontend(const String& message)
    {
        RefPtr<InspectorValue> value = InspectorValue::parseJSON(message);
        if (!value)
            return false;
        RefPtr<InspectorObject> messageObject = value->asObject();
        if (!messageObject)
            return false;
        m_frontend->worker()->dispatchMessageFromWorker(m_id, messageObject);
        return true;
    }

    InspectorFrontend* m_frontend;
    WorkerContextInspectorProxy* m_inspectorProxy;
    int m_id;
    static int s_nextId;
};

int InspectorWorkerAgent::WorkerFrontendChannel::s_nextId = 1;

PassOwnPtr<InspectorWorkerAgent> InspectorWorkerAgent::create(InstrumentingAgents* instrumentingAgents)
{
    return adoptPtr(new InspectorWorkerAgent(instrumentingAgents));
}

InspectorWorkerAgent::InspectorWorkerAgent(InstrumentingAgents* instrumentingAgents)
    : m_instrumentingAgents(instrumentingAgents)
    , m_inspectorFrontend(0)
{
}

InspectorWorkerAgent::~InspectorWorkerAgent()
{
}

void InspectorWorkerAgent::setFrontend(InspectorFrontend* frontend)
{
    m_inspectorFrontend = frontend;
    // Disabled for now.
    // m_instrumentingAgents->setInspectorWorkerAgent(this);
}

void InspectorWorkerAgent::clearFrontend()
{
    m_inspectorFrontend = 0;
    m_instrumentingAgents->setInspectorWorkerAgent(0);
}

void InspectorWorkerAgent::sendMessageToWorker(ErrorString* error, int workerId, PassRefPtr<InspectorObject> message)
{
    WorkerFrontendChannel* channel = m_idToChannel.get(workerId);
    if (channel)
        channel->inspectorProxy()->sendMessageToWorkerContextInspector(message->toJSONString());
    else
        *error = "Worker is gone";
}

void InspectorWorkerAgent::didStartWorkerContext(WorkerContextProxy* workerContextProxy)
{
    WorkerFrontendChannel* channel = new WorkerFrontendChannel(m_inspectorFrontend, workerContextProxy->inspectorProxy());
    m_idToChannel.set(channel->id(), channel);

    workerContextProxy->inspectorProxy()->connectFrontend(channel);
    if (m_inspectorFrontend)
        m_inspectorFrontend->worker()->workerCreated(channel->id());
}

} // namespace WebCore

#endif // ENABLE(WORKERS) && ENABLE(INSPECTOR)
