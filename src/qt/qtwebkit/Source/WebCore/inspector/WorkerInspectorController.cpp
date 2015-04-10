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

#if ENABLE(INSPECTOR) && ENABLE(WORKERS)

#include "WorkerInspectorController.h"

#include "InjectedScriptHost.h"
#include "InjectedScriptManager.h"
#include "InspectorBackendDispatcher.h"
#include "InspectorClient.h"
#include "InspectorConsoleAgent.h"
#include "InspectorFrontend.h"
#include "InspectorFrontendChannel.h"
#include "InspectorHeapProfilerAgent.h"
#include "InspectorProfilerAgent.h"
#include "InspectorState.h"
#include "InspectorStateClient.h"
#include "InspectorTimelineAgent.h"
#include "InstrumentingAgents.h"
#include "WorkerConsoleAgent.h"
#include "WorkerDebuggerAgent.h"
#include "WorkerGlobalScope.h"
#include "WorkerReportingProxy.h"
#include "WorkerRuntimeAgent.h"
#include "WorkerThread.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

namespace {

class PageInspectorProxy : public InspectorFrontendChannel {
    WTF_MAKE_FAST_ALLOCATED;
public:
    explicit PageInspectorProxy(WorkerGlobalScope* workerGlobalScope) : m_workerGlobalScope(workerGlobalScope) { }
    virtual ~PageInspectorProxy() { }
private:
    virtual bool sendMessageToFrontend(const String& message)
    {
        m_workerGlobalScope->thread()->workerReportingProxy().postMessageToPageInspector(message);
        return true;
    }
    WorkerGlobalScope* m_workerGlobalScope;
};

class WorkerStateClient : public InspectorStateClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    WorkerStateClient(WorkerGlobalScope* context) : m_workerGlobalScope(context) { }
    virtual ~WorkerStateClient() { }

private:
    virtual bool supportsInspectorStateUpdates() const { return true; }
    virtual void updateInspectorStateCookie(const String& cookie)
    {
        m_workerGlobalScope->thread()->workerReportingProxy().updateInspectorStateCookie(cookie);
    }

    WorkerGlobalScope* m_workerGlobalScope;
};

}

WorkerInspectorController::WorkerInspectorController(WorkerGlobalScope* workerGlobalScope)
    : m_workerGlobalScope(workerGlobalScope)
    , m_stateClient(adoptPtr(new WorkerStateClient(workerGlobalScope)))
    , m_state(adoptPtr(new InspectorCompositeState(m_stateClient.get())))
    , m_instrumentingAgents(InstrumentingAgents::create())
    , m_injectedScriptManager(InjectedScriptManager::createForWorker())
    , m_runtimeAgent(0)
{
    OwnPtr<InspectorRuntimeAgent> runtimeAgent = WorkerRuntimeAgent::create(m_instrumentingAgents.get(), m_state.get(), m_injectedScriptManager.get(), workerGlobalScope);
    m_runtimeAgent = runtimeAgent.get();
    m_agents.append(runtimeAgent.release());

    OwnPtr<InspectorConsoleAgent> consoleAgent = WorkerConsoleAgent::create(m_instrumentingAgents.get(), m_state.get(), m_injectedScriptManager.get());
#if ENABLE(JAVASCRIPT_DEBUGGER)
    OwnPtr<InspectorDebuggerAgent> debuggerAgent = WorkerDebuggerAgent::create(m_instrumentingAgents.get(), m_state.get(), workerGlobalScope, m_injectedScriptManager.get());
    InspectorDebuggerAgent* debuggerAgentPtr = debuggerAgent.get();
    m_runtimeAgent->setScriptDebugServer(&debuggerAgent->scriptDebugServer());
    m_agents.append(debuggerAgent.release());

    m_agents.append(InspectorProfilerAgent::create(m_instrumentingAgents.get(), consoleAgent.get(), workerGlobalScope, m_state.get(), m_injectedScriptManager.get()));
    m_agents.append(InspectorHeapProfilerAgent::create(m_instrumentingAgents.get(), m_state.get(), m_injectedScriptManager.get()));
#endif
    m_agents.append(InspectorTimelineAgent::create(m_instrumentingAgents.get(), 0, 0, m_state.get(), InspectorTimelineAgent::WorkerInspector, 0));
    m_agents.append(consoleAgent.release());

    m_injectedScriptManager->injectedScriptHost()->init(0
        , 0
#if ENABLE(SQL_DATABASE)
        , 0
#endif
        , 0
        , 0
#if ENABLE(JAVASCRIPT_DEBUGGER)
        , debuggerAgentPtr
#endif
    );
}
 
WorkerInspectorController::~WorkerInspectorController()
{
    m_instrumentingAgents->reset();
    disconnectFrontend();
}

void WorkerInspectorController::connectFrontend()
{
    ASSERT(!m_frontend);
    m_state->unmute();
    m_frontendChannel = adoptPtr(new PageInspectorProxy(m_workerGlobalScope));
    m_frontend = adoptPtr(new InspectorFrontend(m_frontendChannel.get()));
    m_backendDispatcher = InspectorBackendDispatcher::create(m_frontendChannel.get());
    m_agents.registerInDispatcher(m_backendDispatcher.get());
    m_agents.setFrontend(m_frontend.get());
}

void WorkerInspectorController::disconnectFrontend()
{
    if (!m_frontend)
        return;
    m_backendDispatcher->clearFrontend();
    m_backendDispatcher.clear();
    // Destroying agents would change the state, but we don't want that.
    // Pre-disconnect state will be used to restore inspector agents.
    m_state->mute();
    m_agents.clearFrontend();
    m_frontend.clear();
    m_frontendChannel.clear();
}

void WorkerInspectorController::restoreInspectorStateFromCookie(const String& inspectorCookie)
{
    ASSERT(!m_frontend);
    connectFrontend();
    m_state->loadFromCookie(inspectorCookie);

    m_agents.restore();
}

void WorkerInspectorController::dispatchMessageFromFrontend(const String& message)
{
    if (m_backendDispatcher)
        m_backendDispatcher->dispatch(message);
}

#if ENABLE(JAVASCRIPT_DEBUGGER)
void WorkerInspectorController::resume()
{
    ErrorString unused;
    m_runtimeAgent->run(&unused);
}
#endif

}

#endif
