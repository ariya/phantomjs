/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#if ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(INSPECTOR)

#include "InspectorProfilerAgent.h"

#include "Console.h"
#include "ConsoleAPITypes.h"
#include "ConsoleTypes.h"
#include "InjectedScript.h"
#include "InjectedScriptHost.h"
#include "InspectorConsoleAgent.h"
#include "InspectorFrontend.h"
#include "InspectorState.h"
#include "InspectorValues.h"
#include "InstrumentingAgents.h"
#include "KURL.h"
#include "Page.h"
#include "PageScriptDebugServer.h"
#include "ScriptHeapSnapshot.h"
#include "ScriptObject.h"
#include "ScriptProfile.h"
#include "ScriptProfiler.h"
#include "WorkerScriptDebugServer.h"
#include <wtf/CurrentTime.h>
#include <wtf/OwnPtr.h>
#include <wtf/text/StringConcatenate.h>

namespace WebCore {

namespace ProfilerAgentState {
static const char userInitiatedProfiling[] = "userInitiatedProfiling";
static const char profilerEnabled[] = "profilerEnabled";
static const char profileHeadersRequested[] = "profileHeadersRequested";
}

static const char* const UserInitiatedProfileName = "org.webkit.profiles.user-initiated";
static const char* const CPUProfileType = "CPU";
static const char* const HeapProfileType = "HEAP";


class PageProfilerAgent : public InspectorProfilerAgent {
public:
    PageProfilerAgent(InstrumentingAgents* instrumentingAgents, InspectorConsoleAgent* consoleAgent, Page* inspectedPage, InspectorCompositeState* state, InjectedScriptManager* injectedScriptManager)
        : InspectorProfilerAgent(instrumentingAgents, consoleAgent, state, injectedScriptManager), m_inspectedPage(inspectedPage) { }
    virtual ~PageProfilerAgent() { }

private:
    virtual void recompileScript()
    {
        PageScriptDebugServer::shared().recompileAllJSFunctionsSoon();
    }

    virtual void startProfiling(const String& title)
    {
        ScriptProfiler::startForPage(m_inspectedPage, title);
    }

    virtual PassRefPtr<ScriptProfile> stopProfiling(const String& title)
    {
        return ScriptProfiler::stopForPage(m_inspectedPage, title);
    }

    Page* m_inspectedPage;
};

PassOwnPtr<InspectorProfilerAgent> InspectorProfilerAgent::create(InstrumentingAgents* instrumentingAgents, InspectorConsoleAgent* consoleAgent, Page* inspectedPage, InspectorCompositeState* inspectorState, InjectedScriptManager* injectedScriptManager)
{
    return adoptPtr(new PageProfilerAgent(instrumentingAgents, consoleAgent, inspectedPage, inspectorState, injectedScriptManager));
}

#if ENABLE(WORKERS)
class WorkerProfilerAgent : public InspectorProfilerAgent {
public:
    WorkerProfilerAgent(InstrumentingAgents* instrumentingAgents, InspectorConsoleAgent* consoleAgent, WorkerGlobalScope* workerGlobalScope, InspectorCompositeState* state, InjectedScriptManager* injectedScriptManager)
        : InspectorProfilerAgent(instrumentingAgents, consoleAgent, state, injectedScriptManager), m_workerGlobalScope(workerGlobalScope) { }
    virtual ~WorkerProfilerAgent() { }

private:
    virtual void recompileScript() { }

    virtual void startProfiling(const String& title)
    {
        ScriptProfiler::startForWorkerGlobalScope(m_workerGlobalScope, title);
    }

    virtual PassRefPtr<ScriptProfile> stopProfiling(const String& title)
    {
        return ScriptProfiler::stopForWorkerGlobalScope(m_workerGlobalScope, title);
    }

    WorkerGlobalScope* m_workerGlobalScope;
};

PassOwnPtr<InspectorProfilerAgent> InspectorProfilerAgent::create(InstrumentingAgents* instrumentingAgents, InspectorConsoleAgent* consoleAgent, WorkerGlobalScope* workerGlobalScope, InspectorCompositeState* inspectorState, InjectedScriptManager* injectedScriptManager)
{
    return adoptPtr(new WorkerProfilerAgent(instrumentingAgents, consoleAgent, workerGlobalScope, inspectorState, injectedScriptManager));
}
#endif

InspectorProfilerAgent::InspectorProfilerAgent(InstrumentingAgents* instrumentingAgents, InspectorConsoleAgent* consoleAgent, InspectorCompositeState* inspectorState, InjectedScriptManager* injectedScriptManager)
    : InspectorBaseAgent<InspectorProfilerAgent>("Profiler", instrumentingAgents, inspectorState)
    , m_consoleAgent(consoleAgent)
    , m_injectedScriptManager(injectedScriptManager)
    , m_frontend(0)
    , m_enabled(false)
    , m_recordingCPUProfile(false)
    , m_currentUserInitiatedProfileNumber(-1)
    , m_nextUserInitiatedProfileNumber(1)
    , m_nextUserInitiatedHeapSnapshotNumber(1)
    , m_profileNameIdleTimeMap(ScriptProfiler::currentProfileNameIdleTimeMap())
    , m_previousTaskEndTime(0.0)
{
    m_instrumentingAgents->setInspectorProfilerAgent(this);
}

InspectorProfilerAgent::~InspectorProfilerAgent()
{
    m_instrumentingAgents->setInspectorProfilerAgent(0);
}

void InspectorProfilerAgent::addProfile(PassRefPtr<ScriptProfile> prpProfile, unsigned lineNumber, unsigned columnNumber, const String& sourceURL)
{
    RefPtr<ScriptProfile> profile = prpProfile;
    m_profiles.add(profile->uid(), profile);
    if (m_frontend && m_state->getBoolean(ProfilerAgentState::profileHeadersRequested))
        m_frontend->addProfileHeader(createProfileHeader(*profile));
    addProfileFinishedMessageToConsole(profile, lineNumber, columnNumber, sourceURL);
}

void InspectorProfilerAgent::addProfileFinishedMessageToConsole(PassRefPtr<ScriptProfile> prpProfile, unsigned lineNumber, unsigned columnNumber, const String& sourceURL)
{
    if (!m_frontend)
        return;
    RefPtr<ScriptProfile> profile = prpProfile;
    String message = makeString(profile->title(), '#', String::number(profile->uid()));
    m_consoleAgent->addMessageToConsole(ConsoleAPIMessageSource, ProfileEndMessageType, DebugMessageLevel, message, sourceURL, lineNumber, columnNumber);
}

void InspectorProfilerAgent::addStartProfilingMessageToConsole(const String& title, unsigned lineNumber, unsigned columnNumber, const String& sourceURL)
{
    if (!m_frontend)
        return;
    m_consoleAgent->addMessageToConsole(ConsoleAPIMessageSource, ProfileMessageType, DebugMessageLevel, title, sourceURL, lineNumber, columnNumber);
}

void InspectorProfilerAgent::collectGarbage(WebCore::ErrorString*)
{
    ScriptProfiler::collectGarbage();
}

PassRefPtr<TypeBuilder::Profiler::ProfileHeader> InspectorProfilerAgent::createProfileHeader(const ScriptProfile& profile)
{
    return TypeBuilder::Profiler::ProfileHeader::create()
        .setTypeId(TypeBuilder::Profiler::ProfileHeader::TypeId::CPU)
        .setUid(profile.uid())
        .setTitle(profile.title())
        .release();
}

PassRefPtr<TypeBuilder::Profiler::ProfileHeader> InspectorProfilerAgent::createSnapshotHeader(const ScriptHeapSnapshot& snapshot)
{
    RefPtr<TypeBuilder::Profiler::ProfileHeader> header = TypeBuilder::Profiler::ProfileHeader::create()
        .setTypeId(TypeBuilder::Profiler::ProfileHeader::TypeId::HEAP)
        .setUid(snapshot.uid())
        .setTitle(snapshot.title());
    header->setMaxJSObjectId(snapshot.maxSnapshotJSObjectId());
    return header.release();
}

void InspectorProfilerAgent::causesRecompilation(ErrorString*, bool* result)
{
    *result = ScriptProfiler::causesRecompilation();
}

void InspectorProfilerAgent::isSampling(ErrorString*, bool* result)
{
    *result = ScriptProfiler::isSampling();
}

void InspectorProfilerAgent::hasHeapProfiler(ErrorString*, bool* result)
{
    *result = ScriptProfiler::hasHeapProfiler();
}

void InspectorProfilerAgent::enable(ErrorString*)
{
    if (enabled())
        return;
    m_state->setBoolean(ProfilerAgentState::profilerEnabled, true);
    enable(false);
}

void InspectorProfilerAgent::disable(ErrorString*)
{
    m_state->setBoolean(ProfilerAgentState::profilerEnabled, false);
    disable();
}

void InspectorProfilerAgent::disable()
{
    if (!m_enabled)
        return;
    m_enabled = false;
    m_state->setBoolean(ProfilerAgentState::profileHeadersRequested, false);
    recompileScript();
}

void InspectorProfilerAgent::enable(bool skipRecompile)
{
    if (m_enabled)
        return;
    m_enabled = true;
    if (!skipRecompile)
        recompileScript();
}

String InspectorProfilerAgent::getCurrentUserInitiatedProfileName(bool incrementProfileNumber)
{
    if (incrementProfileNumber)
        m_currentUserInitiatedProfileNumber = m_nextUserInitiatedProfileNumber++;

    return makeString(UserInitiatedProfileName, '.', String::number(m_currentUserInitiatedProfileNumber));
}

void InspectorProfilerAgent::getProfileHeaders(ErrorString*, RefPtr<TypeBuilder::Array<TypeBuilder::Profiler::ProfileHeader> >& headers)
{
    m_state->setBoolean(ProfilerAgentState::profileHeadersRequested, true);
    headers = TypeBuilder::Array<TypeBuilder::Profiler::ProfileHeader>::create();

    ProfilesMap::iterator profilesEnd = m_profiles.end();
    for (ProfilesMap::iterator it = m_profiles.begin(); it != profilesEnd; ++it)
        headers->addItem(createProfileHeader(*it->value));
    HeapSnapshotsMap::iterator snapshotsEnd = m_snapshots.end();
    for (HeapSnapshotsMap::iterator it = m_snapshots.begin(); it != snapshotsEnd; ++it)
        headers->addItem(createSnapshotHeader(*it->value));
}

namespace {

class OutputStream : public ScriptHeapSnapshot::OutputStream {
public:
    OutputStream(InspectorFrontend::Profiler* frontend, unsigned uid)
        : m_frontend(frontend), m_uid(uid) { }
    void Write(const String& chunk) { m_frontend->addHeapSnapshotChunk(m_uid, chunk); }
    void Close() { m_frontend->finishHeapSnapshot(m_uid); }
private:
    InspectorFrontend::Profiler* m_frontend;
    int m_uid;
};

} // namespace

void InspectorProfilerAgent::getCPUProfile(ErrorString* errorString, int rawUid, RefPtr<TypeBuilder::Profiler::CPUProfile>& profileObject)
{
    unsigned uid = static_cast<unsigned>(rawUid);
    ProfilesMap::iterator it = m_profiles.find(uid);
    if (it == m_profiles.end()) {
        *errorString = "Profile wasn't found";
        return;
    }
    profileObject = TypeBuilder::Profiler::CPUProfile::create();
    profileObject->setHead(it->value->buildInspectorObjectForHead());
    profileObject->setIdleTime(it->value->idleTime());
}

void InspectorProfilerAgent::getHeapSnapshot(ErrorString* errorString, int rawUid)
{
    unsigned uid = static_cast<unsigned>(rawUid);
    HeapSnapshotsMap::iterator it = m_snapshots.find(uid);
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

void InspectorProfilerAgent::removeProfile(ErrorString*, const String& type, int rawUid)
{
    unsigned uid = static_cast<unsigned>(rawUid);
    if (type == CPUProfileType) {
        if (m_profiles.contains(uid))
            m_profiles.remove(uid);
    } else if (type == HeapProfileType) {
        if (m_snapshots.contains(uid))
            m_snapshots.remove(uid);
    }
}

void InspectorProfilerAgent::resetState()
{
    stop();
    m_profiles.clear();
    m_snapshots.clear();
    m_currentUserInitiatedProfileNumber = 1;
    m_nextUserInitiatedProfileNumber = 1;
    m_nextUserInitiatedHeapSnapshotNumber = 1;
    resetFrontendProfiles();
    m_injectedScriptManager->injectedScriptHost()->clearInspectedObjects();
}

void InspectorProfilerAgent::resetFrontendProfiles()
{
    if (!m_frontend)
        return;
    if (!m_state->getBoolean(ProfilerAgentState::profileHeadersRequested))
        return;
    if (m_profiles.isEmpty() && m_snapshots.isEmpty())
        m_frontend->resetProfiles();
}

void InspectorProfilerAgent::setFrontend(InspectorFrontend* frontend)
{
    m_frontend = frontend->profiler();
}

void InspectorProfilerAgent::clearFrontend()
{
    m_frontend = 0;
    stop();
    ErrorString error;
    disable(&error);
}

void InspectorProfilerAgent::restore()
{
    // Need to restore enablement state here as in setFrontend m_state wasn't loaded yet.
    restoreEnablement();
    resetFrontendProfiles();
    if (m_state->getBoolean(ProfilerAgentState::userInitiatedProfiling))
        start();
}

void InspectorProfilerAgent::restoreEnablement()
{
    if (m_state->getBoolean(ProfilerAgentState::profilerEnabled)) {
        ErrorString error;
        enable(&error);
    }
}

void InspectorProfilerAgent::start(ErrorString*)
{
    if (m_recordingCPUProfile)
        return;
    if (!enabled()) {
        enable(true);
        PageScriptDebugServer::shared().recompileAllJSFunctions(0);
    }
    m_recordingCPUProfile = true;
    String title = getCurrentUserInitiatedProfileName(true);
    startProfiling(title);
    addStartProfilingMessageToConsole(title, 0, 0, String());
    toggleRecordButton(true);
    m_state->setBoolean(ProfilerAgentState::userInitiatedProfiling, true);
}

void InspectorProfilerAgent::stop(ErrorString*)
{
    if (!m_recordingCPUProfile)
        return;
    m_recordingCPUProfile = false;
    String title = getCurrentUserInitiatedProfileName();
    RefPtr<ScriptProfile> profile = stopProfiling(title);
    if (profile)
        addProfile(profile, 0, 0, String());
    toggleRecordButton(false);
    m_state->setBoolean(ProfilerAgentState::userInitiatedProfiling, false);
}

namespace {

class HeapSnapshotProgress: public ScriptProfiler::HeapSnapshotProgress {
public:
    explicit HeapSnapshotProgress(InspectorFrontend::Profiler* frontend)
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
    InspectorFrontend::Profiler* m_frontend;
    int m_totalWork;
};

};

void InspectorProfilerAgent::takeHeapSnapshot(ErrorString*, const bool* reportProgress)
{
    String title = makeString(UserInitiatedProfileName, '.', String::number(m_nextUserInitiatedHeapSnapshotNumber));
    ++m_nextUserInitiatedHeapSnapshotNumber;

    HeapSnapshotProgress progress(reportProgress && *reportProgress ? m_frontend : 0);
    RefPtr<ScriptHeapSnapshot> snapshot = ScriptProfiler::takeHeapSnapshot(title, &progress);
    if (snapshot) {
        m_snapshots.add(snapshot->uid(), snapshot);
        if (m_frontend)
            m_frontend->addProfileHeader(createSnapshotHeader(*snapshot));
    }
}

void InspectorProfilerAgent::toggleRecordButton(bool isProfiling)
{
    if (m_frontend)
        m_frontend->setRecordingProfile(isProfiling);
}

void InspectorProfilerAgent::getObjectByHeapObjectId(ErrorString* error, const String& heapSnapshotObjectId, const String* objectGroup, RefPtr<TypeBuilder::Runtime::RemoteObject>& result)
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

void InspectorProfilerAgent::getHeapObjectId(ErrorString* errorString, const String& objectId, String* heapSnapshotObjectId)
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

void InspectorProfilerAgent::willProcessTask()
{
    if (!m_profileNameIdleTimeMap || !m_profileNameIdleTimeMap->size())
        return;
    if (!m_previousTaskEndTime)
        return;

    double idleTime = WTF::monotonicallyIncreasingTime() - m_previousTaskEndTime;
    m_previousTaskEndTime = 0.0;
    ProfileNameIdleTimeMap::iterator end = m_profileNameIdleTimeMap->end();
    for (ProfileNameIdleTimeMap::iterator it = m_profileNameIdleTimeMap->begin(); it != end; ++it)
        it->value += idleTime;
}

void InspectorProfilerAgent::didProcessTask()
{
    if (!m_profileNameIdleTimeMap || !m_profileNameIdleTimeMap->size())
        return;
    m_previousTaskEndTime = WTF::monotonicallyIncreasingTime();
}

} // namespace WebCore

#endif // ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(INSPECTOR)
