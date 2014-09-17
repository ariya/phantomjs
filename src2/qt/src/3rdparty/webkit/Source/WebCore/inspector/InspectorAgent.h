/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef InspectorAgent_h
#define InspectorAgent_h

#include "CharacterData.h"
#include "Console.h"
#include "Page.h"
#include "PlatformString.h"
#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class CharacterData;
class Database;
class DOMWrapperWorld;
class Document;
class DocumentLoader;
class FloatRect;
class HTTPHeaderMap;
class InjectedScript;
class InjectedScriptManager;
class InspectorArray;
class InspectorDOMDebuggerAgent;
class InspectorClient;
class InspectorConsoleAgent;
class InspectorCSSAgent;
class InspectorDOMAgent;
class InspectorDOMStorageAgent;
class InspectorDatabaseAgent;
class InspectorDatabaseResource;
class InspectorDebuggerAgent;
class InspectorFrontend;
class InspectorFrontendClient;
class InspectorObject;
class InspectorPageAgent;
class InspectorProfilerAgent;
class InspectorResourceAgent;
class InspectorRuntimeAgent;
class InspectorState;
class InspectorStorageAgent;
class InspectorTimelineAgent;
class InspectorValue;
class InspectorWorkerAgent;
class InspectorWorkerResource;
class InstrumentingAgents;
class IntRect;
class KURL;
class Node;
class Page;
class ResourceRequest;
class ResourceResponse;
class ResourceError;
class ScriptArguments;
class ScriptCallStack;
class ScriptProfile;
class SharedBuffer;
class StorageArea;

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
class InspectorApplicationCacheAgent;
#endif

typedef String ErrorString;

class InspectorAgent {
    WTF_MAKE_NONCOPYABLE(InspectorAgent);
    WTF_MAKE_FAST_ALLOCATED;
public:
    InspectorAgent(Page*, InspectorClient*, InjectedScriptManager*);
    virtual ~InspectorAgent();

    InspectorClient* inspectorClient() { return m_client; }

    void inspectedPageDestroyed();

    bool enabled() const;

    Page* inspectedPage() const { return m_inspectedPage; }
    KURL inspectedURL() const;
    KURL inspectedURLWithoutFragment() const;
    void reloadPage(ErrorString*, bool ignoreCache);
    void showConsole();

    void restoreInspectorStateFromCookie(const String& inspectorCookie);

    void setFrontend(InspectorFrontend*);
    InspectorFrontend* frontend() const { return m_frontend; }
    void disconnectFrontend();

    InstrumentingAgents* instrumentingAgents() const { return m_instrumentingAgents.get(); }

    InspectorPageAgent* pageAgent() { return m_pageAgent.get(); }
    InspectorConsoleAgent* consoleAgent() { return m_consoleAgent.get(); }
    InspectorCSSAgent* cssAgent() { return m_cssAgent.get(); }
    InspectorDOMAgent* domAgent() { return m_domAgent.get(); }
    InspectorRuntimeAgent* runtimeAgent() { return m_runtimeAgent.get(); }
    InspectorTimelineAgent* timelineAgent() { return m_timelineAgent.get(); }
    InspectorResourceAgent* resourceAgent() { return m_resourceAgent.get(); }
#if ENABLE(DATABASE)
    InspectorDatabaseAgent* databaseAgent() { return m_databaseAgent.get(); }
#endif
#if ENABLE(DOM_STORAGE)
    InspectorDOMStorageAgent* domStorageAgent() { return m_domStorageAgent.get(); }
#endif
#if ENABLE(JAVASCRIPT_DEBUGGER)
    InspectorDOMDebuggerAgent* domDebuggerAgent() const { return m_domDebuggerAgent.get(); }
    InspectorDebuggerAgent* debuggerAgent() const { return m_debuggerAgent.get(); }
    InspectorProfilerAgent* profilerAgent() const { return m_profilerAgent.get(); }
#endif
#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    InspectorApplicationCacheAgent* applicationCacheAgent() { return m_applicationCacheAgent.get(); }
#endif
#if ENABLE(WORKERS)
    InspectorWorkerAgent* workerAgent() { return m_workerAgent.get(); }
#endif

    void didClearWindowObjectInWorld(Frame*, DOMWrapperWorld*);

    void didCommitLoad();
    void domContentLoadedEventFired();

#if ENABLE(WORKERS)
    enum WorkerAction { WorkerCreated, WorkerDestroyed };

    void postWorkerNotificationToFrontend(const InspectorWorkerResource&, WorkerAction);
    void didCreateWorker(intptr_t, const String& url, bool isSharedWorker);
    void didDestroyWorker(intptr_t);
#endif

    bool hasFrontend() const { return m_frontend; }


#if ENABLE(JAVASCRIPT_DEBUGGER)
    void showProfilesPanel();
#endif

    // Generic code called from custom implementations.
    void evaluateForTestInFrontend(long testCallId, const String& script);

    void setInspectorExtensionAPI(const String& source);

    InspectorState* state() { return m_state.get(); }

    // InspectorAgent API
    void getInspectorState(RefPtr<InspectorObject>* state);
    void setMonitoringXHREnabled(bool enabled, bool* newState);

private:
    void showPanel(const String& panel);
    void unbindAllResources();

#if ENABLE(JAVASCRIPT_DEBUGGER)
    void toggleRecordButton(bool);
#endif

    bool isMainResourceLoader(DocumentLoader*, const KURL& requestUrl);
    void issueEvaluateForTestCommands();

    Page* m_inspectedPage;
    InspectorClient* m_client;
    InspectorFrontend* m_frontend;
    OwnPtr<InstrumentingAgents> m_instrumentingAgents;
    InjectedScriptManager* m_injectedScriptManager;
    OwnPtr<InspectorState> m_state;
    OwnPtr<InspectorPageAgent> m_pageAgent;
    OwnPtr<InspectorDOMAgent> m_domAgent;
    OwnPtr<InspectorCSSAgent> m_cssAgent;

#if ENABLE(DATABASE)
    OwnPtr<InspectorDatabaseAgent> m_databaseAgent;
#endif

#if ENABLE(DOM_STORAGE)
    OwnPtr<InspectorDOMStorageAgent> m_domStorageAgent;
#endif

    OwnPtr<InspectorTimelineAgent> m_timelineAgent;

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    OwnPtr<InspectorApplicationCacheAgent> m_applicationCacheAgent;
#endif

    RefPtr<InspectorResourceAgent> m_resourceAgent;
    OwnPtr<InspectorRuntimeAgent> m_runtimeAgent;

    OwnPtr<InspectorConsoleAgent> m_consoleAgent;

    Vector<pair<long, String> > m_pendingEvaluateTestCommands;
    String m_showPanelAfterVisible;
    String m_inspectorExtensionAPI;
#if ENABLE(JAVASCRIPT_DEBUGGER)
    OwnPtr<InspectorDebuggerAgent> m_debuggerAgent;
    OwnPtr<InspectorDOMDebuggerAgent> m_domDebuggerAgent;
    OwnPtr<InspectorProfilerAgent> m_profilerAgent;
#endif
#if ENABLE(WORKERS)
    typedef HashMap<intptr_t, RefPtr<InspectorWorkerResource> > WorkersMap;
    WorkersMap m_workers;
    OwnPtr<InspectorWorkerAgent> m_workerAgent;
#endif
    bool m_canIssueEvaluateForTestInFrontend;
};

} // namespace WebCore

#endif // !defined(InspectorAgent_h)
