/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Matt Lilek <webkit@mattlilek.com>
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "InspectorAgent.h"

#if ENABLE(INSPECTOR)

#include "Document.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "GraphicsContext.h"
#include "InjectedScriptHost.h"
#include "InjectedScriptManager.h"
#include "InspectorDOMDebuggerAgent.h"
#include "InspectorCSSAgent.h"
#include "InspectorClient.h"
#include "InspectorConsoleAgent.h"
#include "InspectorController.h"
#include "InspectorDOMAgent.h"
#include "InspectorFrontend.h"
#include "InspectorInstrumentation.h"
#include "InspectorPageAgent.h"
#include "InspectorProfilerAgent.h"
#include "InspectorResourceAgent.h"
#include "InspectorRuntimeAgent.h"
#include "InspectorState.h"
#include "InspectorTimelineAgent.h"
#include "InspectorValues.h"
#include "InspectorWorkerAgent.h"
#include "InspectorWorkerResource.h"
#include "InstrumentingAgents.h"
#include "Page.h"
#include "PageDebuggerAgent.h"
#include "ResourceRequest.h"
#include "ScriptFunctionCall.h"
#include "ScriptObject.h"
#include "ScriptState.h"
#include "Settings.h"

#if ENABLE(DATABASE)
#include "InspectorDatabaseAgent.h"
#endif

#if ENABLE(DOM_STORAGE)
#include "InspectorDOMStorageAgent.h"
#endif

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
#include "InspectorApplicationCacheAgent.h"
#endif

using namespace std;

namespace WebCore {

namespace InspectorAgentState {
static const char timelineProfilerEnabled[] = "timelineProfilerEnabled";
static const char debuggerEnabled[] = "debuggerEnabled";
}

static const char scriptsPanelName[] = "scripts";
static const char consolePanelName[] = "console";
static const char profilesPanelName[] = "profiles";

namespace {

class PageRuntimeAgent : public InspectorRuntimeAgent {
public:
    PageRuntimeAgent(InjectedScriptManager* injectedScriptManager, Page* page)
        : InspectorRuntimeAgent(injectedScriptManager)
        , m_inspectedPage(page) { }
    virtual ~PageRuntimeAgent() { }

private:
    virtual ScriptState* getDefaultInspectedState() { return mainWorldScriptState(m_inspectedPage->mainFrame()); }
    Page* m_inspectedPage;
};

}

InspectorAgent::InspectorAgent(Page* page, InspectorClient* client, InjectedScriptManager* injectedScriptManager)
    : m_inspectedPage(page)
    , m_client(client)
    , m_frontend(0)
    , m_instrumentingAgents(adoptPtr(new InstrumentingAgents))
    , m_injectedScriptManager(injectedScriptManager)
    , m_state(adoptPtr(new InspectorState(client)))
    , m_pageAgent(InspectorPageAgent::create(m_instrumentingAgents.get(), page, injectedScriptManager))
    , m_domAgent(InspectorDOMAgent::create(m_instrumentingAgents.get(), m_pageAgent.get(), m_client, m_state.get(), injectedScriptManager))
    , m_cssAgent(adoptPtr(new InspectorCSSAgent(m_instrumentingAgents.get(), m_domAgent.get())))
#if ENABLE(DATABASE)
    , m_databaseAgent(InspectorDatabaseAgent::create(m_instrumentingAgents.get(), m_state.get()))
#endif
#if ENABLE(DOM_STORAGE)
    , m_domStorageAgent(InspectorDOMStorageAgent::create(m_instrumentingAgents.get(), m_state.get()))
#endif
    , m_timelineAgent(InspectorTimelineAgent::create(m_instrumentingAgents.get(), m_state.get()))
#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    , m_applicationCacheAgent(adoptPtr(new InspectorApplicationCacheAgent(m_instrumentingAgents.get(), page)))
#endif
    , m_resourceAgent(InspectorResourceAgent::create(m_instrumentingAgents.get(), m_pageAgent.get(), m_state.get()))
    , m_runtimeAgent(adoptPtr(new PageRuntimeAgent(m_injectedScriptManager, page)))
    , m_consoleAgent(adoptPtr(new InspectorConsoleAgent(m_instrumentingAgents.get(), this, m_state.get(), injectedScriptManager, m_domAgent.get())))
#if ENABLE(JAVASCRIPT_DEBUGGER)
    , m_debuggerAgent(PageDebuggerAgent::create(m_instrumentingAgents.get(), m_state.get(), page, injectedScriptManager))
    , m_domDebuggerAgent(InspectorDOMDebuggerAgent::create(m_instrumentingAgents.get(), m_state.get(), m_domAgent.get(), m_debuggerAgent.get(), this))
    , m_profilerAgent(InspectorProfilerAgent::create(m_instrumentingAgents.get(), m_consoleAgent.get(), page, m_state.get()))
#endif
#if ENABLE(WORKERS)
    , m_workerAgent(InspectorWorkerAgent::create(m_instrumentingAgents.get()))
#endif
    , m_canIssueEvaluateForTestInFrontend(false)
{
    ASSERT_ARG(page, page);
    ASSERT_ARG(client, client);
    InspectorInstrumentation::bindInspectorAgent(m_inspectedPage, this);
    m_instrumentingAgents->setInspectorAgent(this);

    m_injectedScriptManager->injectedScriptHost()->init(this
        , m_consoleAgent.get()
#if ENABLE(DATABASE)
        , m_databaseAgent.get()
#endif
#if ENABLE(DOM_STORAGE)
        , m_domStorageAgent.get()
#endif
    );
}

InspectorAgent::~InspectorAgent()
{
    m_instrumentingAgents->setInspectorAgent(0);

    // These should have been cleared in inspectedPageDestroyed().
    ASSERT(!m_client);
    ASSERT(!m_inspectedPage);
}

void InspectorAgent::inspectedPageDestroyed()
{
    if (m_frontend) {
        m_frontend->inspector()->disconnectFromBackend();
        disconnectFrontend();
    }

#if ENABLE(JAVASCRIPT_DEBUGGER)
    m_domDebuggerAgent.clear();
    m_debuggerAgent.clear();
#endif

    ASSERT(m_inspectedPage);
    InspectorInstrumentation::unbindInspectorAgent(m_inspectedPage);
    m_inspectedPage = 0;

    m_injectedScriptManager->disconnect();

    m_client->inspectorDestroyed();
    m_client = 0;
}

void InspectorAgent::restoreInspectorStateFromCookie(const String& inspectorStateCookie)
{
    m_state->loadFromCookie(inspectorStateCookie);

    m_frontend->inspector()->frontendReused();

    m_domAgent->restore();
    m_resourceAgent->restore();
    m_timelineAgent->restore();

#if ENABLE(DATABASE)
    m_databaseAgent->restore();
#endif

#if ENABLE(DOM_STORAGE)
    m_domStorageAgent->restore();
#endif

#if ENABLE(JAVASCRIPT_DEBUGGER)
    m_debuggerAgent->restore();
    m_profilerAgent->restore();
#endif
}

void InspectorAgent::didClearWindowObjectInWorld(Frame* frame, DOMWrapperWorld* world)
{
    if (world != mainThreadNormalWorld())
        return;

    if (!m_inspectorExtensionAPI.isEmpty())
        m_injectedScriptManager->injectScript(m_inspectorExtensionAPI, mainWorldScriptState(frame));
}

void InspectorAgent::setFrontend(InspectorFrontend* inspectorFrontend)
{
    // We can reconnect to existing front-end -> unmute state.
    m_state->unmute();

    m_frontend = inspectorFrontend;

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    m_applicationCacheAgent->setFrontend(m_frontend);
#endif
    m_pageAgent->setFrontend(m_frontend);
    m_domAgent->setFrontend(m_frontend);
    m_consoleAgent->setFrontend(m_frontend);
    m_timelineAgent->setFrontend(m_frontend);
    m_resourceAgent->setFrontend(m_frontend);
#if ENABLE(JAVASCRIPT_DEBUGGER)
    m_debuggerAgent->setFrontend(m_frontend);
    m_profilerAgent->setFrontend(m_frontend);
#endif
#if ENABLE(DATABASE)
    m_databaseAgent->setFrontend(m_frontend);
#endif
#if ENABLE(DOM_STORAGE)
    m_domStorageAgent->setFrontend(m_frontend);
#endif

    if (!m_showPanelAfterVisible.isEmpty()) {
        m_frontend->inspector()->showPanel(m_showPanelAfterVisible);
        m_showPanelAfterVisible = String();
    }
#if ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(WORKERS)
    WorkersMap::iterator workersEnd = m_workers.end();
    for (WorkersMap::iterator it = m_workers.begin(); it != workersEnd; ++it) {
        InspectorWorkerResource* worker = it->second.get();
        m_frontend->inspector()->didCreateWorker(worker->id(), worker->url(), worker->isSharedWorker());
    }
#endif
#if ENABLE(WORKERS)
    m_workerAgent->setFrontend(m_frontend);
#endif

    // Dispatch pending frontend commands
    issueEvaluateForTestCommands();
}

void InspectorAgent::disconnectFrontend()
{
    if (!m_frontend)
        return;

    m_canIssueEvaluateForTestInFrontend = false;
    m_pendingEvaluateTestCommands.clear();

    // Destroying agents would change the state, but we don't want that.
    // Pre-disconnect state will be used to restore inspector agents.
    m_state->mute();

    m_frontend = 0;

#if ENABLE(JAVASCRIPT_DEBUGGER)
    m_debuggerAgent->clearFrontend();
    m_domDebuggerAgent->clearFrontend();
    m_profilerAgent->clearFrontend();
#endif

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    m_applicationCacheAgent->clearFrontend();
#endif

    m_consoleAgent->clearFrontend();
    m_domAgent->clearFrontend();
    m_timelineAgent->clearFrontend();
    m_resourceAgent->clearFrontend();
#if ENABLE(DATABASE)
    m_databaseAgent->clearFrontend();
#endif
#if ENABLE(DOM_STORAGE)
    m_domStorageAgent->clearFrontend();
#endif
    m_pageAgent->clearFrontend();
#if ENABLE(WORKERS)
    m_workerAgent->clearFrontend();
#endif
}

void InspectorAgent::didCommitLoad()
{
    if (m_frontend)
        m_frontend->inspector()->reset();

    m_injectedScriptManager->discardInjectedScripts();
#if ENABLE(WORKERS)
    m_workers.clear();
#endif
}

void InspectorAgent::domContentLoadedEventFired()
{
    m_injectedScriptManager->injectedScriptHost()->clearInspectedNodes();
}

bool InspectorAgent::isMainResourceLoader(DocumentLoader* loader, const KURL& requestUrl)
{
    return loader->frame() == m_inspectedPage->mainFrame() && requestUrl == loader->requestURL();
}

#if ENABLE(WORKERS)
class PostWorkerNotificationToFrontendTask : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<PostWorkerNotificationToFrontendTask> create(PassRefPtr<InspectorWorkerResource> worker, InspectorAgent::WorkerAction action)
    {
        return adoptPtr(new PostWorkerNotificationToFrontendTask(worker, action));
    }

private:
    PostWorkerNotificationToFrontendTask(PassRefPtr<InspectorWorkerResource> worker, InspectorAgent::WorkerAction action)
        : m_worker(worker)
        , m_action(action)
    {
    }

    virtual void performTask(ScriptExecutionContext* scriptContext)
    {
        if (scriptContext->isDocument()) {
            if (InspectorAgent* inspectorAgent = static_cast<Document*>(scriptContext)->page()->inspectorController()->m_inspectorAgent.get())
                inspectorAgent->postWorkerNotificationToFrontend(*m_worker, m_action);
        }
    }

private:
    RefPtr<InspectorWorkerResource> m_worker;
    InspectorAgent::WorkerAction m_action;
};

void InspectorAgent::postWorkerNotificationToFrontend(const InspectorWorkerResource& worker, InspectorAgent::WorkerAction action)
{
    if (!m_frontend)
        return;
#if ENABLE(JAVASCRIPT_DEBUGGER)
    switch (action) {
    case InspectorAgent::WorkerCreated:
        m_frontend->inspector()->didCreateWorker(worker.id(), worker.url(), worker.isSharedWorker());
        break;
    case InspectorAgent::WorkerDestroyed:
        m_frontend->inspector()->didDestroyWorker(worker.id());
        break;
    }
#endif
}

void InspectorAgent::didCreateWorker(intptr_t id, const String& url, bool isSharedWorker)
{
    if (!enabled())
        return;

    RefPtr<InspectorWorkerResource> workerResource(InspectorWorkerResource::create(id, url, isSharedWorker));
    m_workers.set(id, workerResource);
    if (m_inspectedPage && m_frontend)
        m_inspectedPage->mainFrame()->document()->postTask(PostWorkerNotificationToFrontendTask::create(workerResource, InspectorAgent::WorkerCreated));
}

void InspectorAgent::didDestroyWorker(intptr_t id)
{
    if (!enabled())
        return;

    WorkersMap::iterator workerResource = m_workers.find(id);
    if (workerResource == m_workers.end())
        return;
    if (m_inspectedPage && m_frontend)
        m_inspectedPage->mainFrame()->document()->postTask(PostWorkerNotificationToFrontendTask::create(workerResource->second, InspectorAgent::WorkerDestroyed));
    m_workers.remove(workerResource);
}
#endif // ENABLE(WORKERS)

#if ENABLE(JAVASCRIPT_DEBUGGER)
void InspectorAgent::showProfilesPanel()
{
    showPanel(profilesPanelName);
}
#endif

void InspectorAgent::evaluateForTestInFrontend(long callId, const String& script)
{
    m_pendingEvaluateTestCommands.append(pair<long, String>(callId, script));
    if (m_canIssueEvaluateForTestInFrontend)
        issueEvaluateForTestCommands();
}

void InspectorAgent::setInspectorExtensionAPI(const String& source)
{
    m_inspectorExtensionAPI = source;
}

KURL InspectorAgent::inspectedURL() const
{
    return m_inspectedPage->mainFrame()->document()->url();
}

KURL InspectorAgent::inspectedURLWithoutFragment() const
{
    KURL url = inspectedURL();
    url.removeFragmentIdentifier();
    return url;
}

bool InspectorAgent::enabled() const
{
    if (!m_inspectedPage)
        return false;
    return m_inspectedPage->settings()->developerExtrasEnabled();
}

void InspectorAgent::showConsole()
{
    showPanel(consolePanelName);
}

void InspectorAgent::showPanel(const String& panel)
{
    if (!m_frontend) {
        m_showPanelAfterVisible = panel;
        return;
    }
    m_frontend->inspector()->showPanel(panel);
}

void InspectorAgent::issueEvaluateForTestCommands()
{
    if (m_frontend) {
        Vector<pair<long, String> > copy = m_pendingEvaluateTestCommands;
        m_pendingEvaluateTestCommands.clear();
        for (Vector<pair<long, String> >::iterator it = copy.begin(); m_frontend && it != copy.end(); ++it)
            m_frontend->inspector()->evaluateForTestInFrontend((*it).first, (*it).second);
        m_canIssueEvaluateForTestInFrontend = true;
    }
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
