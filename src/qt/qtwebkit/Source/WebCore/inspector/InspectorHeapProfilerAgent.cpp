/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#if ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(INSPECTOR)

#include "InspectorHeapProfilerAgent.h"

#include "InjectedScript.h"
#include "InjectedScriptHost.h"
#include "InspectorState.h"
#include "InstrumentingAgents.h"
#include "ScriptProfiler.h"

namespace WebCore {

namespace HeapProfilerAgentState {
static const char profileHeadersRequested[] = "profileHeadersRequested";
}

static const char* const UserInitiatedProfileNameHeap = "org.webkit.profiles.user-initiated";

PassOwnPtr<InspectorHeapProfilerAgent> InspectorHeapProfilerAgent::create(InstrumentingAgents* instrumentingAgents, InspectorCompositeState* inspectorState, InjectedScriptManager* injectedScriptManager)
{
    return adoptPtr(new InspectorHeapProfilerAgent(instrumentingAgents, inspectorState, injectedScriptManager));
}

InspectorHeapProfilerAgent::InspectorHeapProfilerAgent(InstrumentingAgents* instrumentingAgents, InspectorCompositeState* inspectorState, InjectedScriptManager* injectedScriptManager)
    : InspectorBaseAgent<InspectorHeapProfilerAgent>("HeapProfiler", instrumentingAgents, inspectorState)
    , m_injectedScriptManager(injectedScriptManager)
    , m_frontend(0)
    , m_nextUserInitiatedHeapSnapshotNumber(1)
{
    m_instrumentingAgents->setInspectorHeapProfilerAgent(this);
}

InspectorHeapProfilerAgent::~InspectorHeapProfilerAgent()
{
    m_instrumentingAgents->setInspectorHeapProfilerAgent(0);
}

void InspectorHeapProfilerAgent::resetState()
{
    m_snapshots.clear();
    m_nextUserInitiatedHeapSnapshotNumber = 1;
    resetFrontendProfiles();
    m_injectedScriptManager->injectedScriptHost()->clearInspectedObjects();
}

void InspectorHeapProfilerAgent::resetFrontendProfiles()
{
    if (!m_frontend)
        return;
    if (!m_state->getBoolean(HeapProfilerAgentState::profileHeadersRequested))
        return;
    if (m_snapshots.isEmpty())
        m_frontend->resetProfiles();
}

void InspectorHeapProfilerAgent::setFrontend(InspectorFrontend* frontend)
{
    m_frontend = frontend->heapprofiler();
}

void InspectorHeapProfilerAgent::clearFrontend()
{
    m_state->setBoolean(HeapProfilerAgentState::profileHeadersRequested, false);
    m_frontend = 0;
}

void InspectorHeapProfilerAgent::restore()
{
    resetFrontendProfiles();
}

void InspectorHeapProfilerAgent::collectGarbage(WebCore::ErrorString*)
{
    ScriptProfiler::collectGarbage();
}

PassRefPtr<TypeBuilder::HeapProfiler::ProfileHeader> InspectorHeapProfilerAgent::createSnapshotHeader(const ScriptHeapSnapshot& snapshot)
{
    RefPtr<TypeBuilder::HeapProfiler::ProfileHeader> header = TypeBuilder::HeapProfiler::ProfileHeader::create()
        .setUid(snapshot.uid())
        .setTitle(snapshot.title());
    header->setMaxJSObjectId(snapshot.maxSnapshotJSObjectId());
    return header.release();
}

void InspectorHeapProfilerAgent::hasHeapProfiler(ErrorString*, bool* result)
{
    *result = ScriptProfiler::hasHeapProfiler();
}

void InspectorHeapProfilerAgent::getProfileHeaders(ErrorString*, RefPtr<TypeBuilder::Array<TypeBuilder::HeapProfiler::ProfileHeader> >& headers)
{
    m_state->setBoolean(HeapProfilerAgentState::profileHeadersRequested, true);
    headers = TypeBuilder::Array<TypeBuilder::HeapProfiler::ProfileHeader>::create();

    IdToHeapSnapshotMap::iterator snapshotsEnd = m_snapshots.end();
    for (IdToHeapSnapshotMap::iterator it = m_snapshots.begin(); it != snapshotsEnd; ++it)
        headers->addItem(createSnapshotHeader(*it->value));
}

void InspectorHeapProfilerAgent::getHeapSnapshot(ErrorString* errorString, int rawUid)
{
    class OutputStream : public ScriptHeapSnapshot::OutputStream {
    public:
        OutputStream(InspectorFrontend::HeapProfiler* frontend, unsigned uid)
            : m_frontend(frontend), m_uid(uid) { }
        void Write(const String& chunk) { m_frontend->addHeapSnapshotChunk(m_uid, chunk); }
        void Close() { m_frontend->finishHeapSnapshot(m_uid); }
    private:
        InspectorFrontend::HeapProfiler* m_frontend;
        int m_uid;
    };

    unsigned uid = static_cast<unsigned>(rawUid);
    IdToHeapSnapshotMap::iterator it = m_snapshots.find(uid);
    if (it == m_snapshots.end()) {
        *errorString = "Profile wasn't found";
        return;
    }
    RefPtr<ScriptHeapSnapshot> snapshot = it->value;
    if (m_frontend) {
        OutputStream stream(m_frontend, uid);
        snapshot->writeJSON(&stream);
    }
}

void InspectorHeapProfilerAgent::removeProfile(ErrorString*, int rawUid)
{
    unsigned uid = static_cast<unsigned>(rawUid);
    if (m_snapshots.contains(uid))
        m_snapshots.remove(uid);
}

void InspectorHeapProfilerAgent::takeHeapSnapshot(ErrorString*, const bool* reportProgress)
{
    class HeapSnapshotProgress: public ScriptProfiler::HeapSnapshotProgress {
    public:
        explicit HeapSnapshotProgress(InspectorFrontend::HeapProfiler* frontend)
            : m_frontend(frontend) { }
        void Start(int totalWork)
        {
            m_totalWork = totalWork;
        }
        void Worked(int workDone)
        {
            if (m_frontend)
                m_frontend->reportHeapSnapshotProgress(workDone, m_totalWork);
        }
        void Done() { }
        bool isCanceled() { return false; }
    private:
        InspectorFrontend::HeapProfiler* m_frontend;
        int m_totalWork;
    };

    String title = makeString(UserInitiatedProfileNameHeap, '.', String::number(m_nextUserInitiatedHeapSnapshotNumber));
    ++m_nextUserInitiatedHeapSnapshotNumber;

    HeapSnapshotProgress progress(reportProgress && *reportProgress ? m_frontend : 0);
    RefPtr<ScriptHeapSnapshot> snapshot = ScriptProfiler::takeHeapSnapshot(title, &progress);
    if (snapshot) {
        m_snapshots.add(snapshot->uid(), snapshot);
        if (m_frontend)
            m_frontend->addProfileHeader(createSnapshotHeader(*snapshot));
    }
}

void InspectorHeapProfilerAgent::getObjectByHeapObjectId(ErrorString* error, const String& heapSnapshotObjectId, const String* objectGroup, RefPtr<TypeBuilder::Runtime::RemoteObject>& result)
{
    bool ok;
    unsigned id = heapSnapshotObjectId.toUInt(&ok);
    if (!ok) {
        *error = "Invalid heap snapshot object id";
        return;
    }
    ScriptObject heapObject = ScriptProfiler::objectByHeapObjectId(id);
    if (heapObject.hasNoValue()) {
        *error = "Object is not available";
        return;
    }
    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptFor(heapObject.scriptState());
    if (injectedScript.hasNoValue()) {
        *error = "Object is not available. Inspected context is gone";
        return;
    }
    result = injectedScript.wrapObject(heapObject, objectGroup ? *objectGroup : "");
    if (!result)
        *error = "Failed to wrap object";
}

void InspectorHeapProfilerAgent::getHeapObjectId(ErrorString* errorString, const String& objectId, String* heapSnapshotObjectId)
{
    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptForObjectId(objectId);
    if (injectedScript.hasNoValue()) {
        *errorString = "Inspected context has gone";
        return;
    }
    ScriptValue value = injectedScript.findObjectById(objectId);
    if (value.hasNoValue() || value.isUndefined()) {
        *errorString = "Object with given id not found";
        return;
    }
    unsigned id = ScriptProfiler::getHeapObjectId(value);
    *heapSnapshotObjectId = String::number(id);
}

} // namespace WebCore

#endif // ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(INSPECTOR)
