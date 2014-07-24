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

#ifndef InspectorHeapProfilerAgent_h
#define InspectorHeapProfilerAgent_h

#if ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(INSPECTOR)

#include "InspectorBaseAgent.h"
#include "InspectorFrontend.h"
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class InjectedScriptManager;
class ScriptHeapSnapshot;
class ScriptProfile;

typedef String ErrorString;

class InspectorHeapProfilerAgent : public InspectorBaseAgent<InspectorHeapProfilerAgent>, public InspectorBackendDispatcher::HeapProfilerCommandHandler {
    WTF_MAKE_NONCOPYABLE(InspectorHeapProfilerAgent); WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<InspectorHeapProfilerAgent> create(InstrumentingAgents*, InspectorCompositeState*, InjectedScriptManager*);
    virtual ~InspectorHeapProfilerAgent();

    virtual void collectGarbage(ErrorString*);
    virtual void clearProfiles(ErrorString*) { resetState(); }
    void resetState();

    virtual void hasHeapProfiler(ErrorString*, bool*);

    virtual void getProfileHeaders(ErrorString*, RefPtr<TypeBuilder::Array<TypeBuilder::HeapProfiler::ProfileHeader> >&);
    virtual void getHeapSnapshot(ErrorString*, int uid);
    virtual void removeProfile(ErrorString*, int uid);

    virtual void setFrontend(InspectorFrontend*);
    virtual void clearFrontend();
    virtual void restore();

    virtual void takeHeapSnapshot(ErrorString*, const bool* reportProgress);

    virtual void getObjectByHeapObjectId(ErrorString*, const String& heapSnapshotObjectId, const String* objectGroup, RefPtr<TypeBuilder::Runtime::RemoteObject>& result);
    virtual void getHeapObjectId(ErrorString*, const String& objectId, String* heapSnapshotObjectId);

private:
    InspectorHeapProfilerAgent(InstrumentingAgents*, InspectorCompositeState*, InjectedScriptManager*);

    typedef HashMap<unsigned, RefPtr<ScriptHeapSnapshot> > IdToHeapSnapshotMap;

    void resetFrontendProfiles();

    PassRefPtr<TypeBuilder::HeapProfiler::ProfileHeader> createSnapshotHeader(const ScriptHeapSnapshot&);

    InjectedScriptManager* m_injectedScriptManager;
    InspectorFrontend::HeapProfiler* m_frontend;
    unsigned m_nextUserInitiatedHeapSnapshotNumber;
    IdToHeapSnapshotMap m_snapshots;
};

} // namespace WebCore

#endif // ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(INSPECTOR)

#endif // !defined(InspectorHeapProfilerAgent_h)
