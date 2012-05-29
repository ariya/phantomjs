/*
* Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef InspectorTimelineAgent_h
#define InspectorTimelineAgent_h

#if ENABLE(INSPECTOR)

#include "InspectorFrontend.h"
#include "InspectorValues.h"
#include "ScriptGCEvent.h"
#include "ScriptGCEventListener.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {
class Event;
class InspectorFrontend;
class InspectorState;
class InstrumentingAgents;
class IntRect;
class ResourceRequest;
class ResourceResponse;

typedef String ErrorString;

class InspectorTimelineAgent : ScriptGCEventListener {
    WTF_MAKE_NONCOPYABLE(InspectorTimelineAgent);
public:
    static PassOwnPtr<InspectorTimelineAgent> create(InstrumentingAgents* instrumentingAgents, InspectorState* state)
    {
        return adoptPtr(new InspectorTimelineAgent(instrumentingAgents, state));
    }

    ~InspectorTimelineAgent();

    void setFrontend(InspectorFrontend*);
    void clearFrontend();
    void restore();

    void start(ErrorString* error);
    void stop(ErrorString* error);
    bool started() const;

    int id() const { return m_id; }

    void didCommitLoad();

    // Methods called from WebCore.
    void willCallFunction(const String& scriptName, int scriptLine);
    void didCallFunction();

    void willDispatchEvent(const Event&);
    void didDispatchEvent();

    void willLayout();
    void didLayout();

    void willRecalculateStyle();
    void didRecalculateStyle();

    void willPaint(const IntRect&);
    void didPaint();

    // FIXME: |length| should be passed in didWrite instead willWrite
    // as the parser can not know how much it will process until it tries.
    void willWriteHTML(unsigned int length, unsigned int startLine);
    void didWriteHTML(unsigned int endLine);

    void didInstallTimer(int timerId, int timeout, bool singleShot);
    void didRemoveTimer(int timerId);
    void willFireTimer(int timerId);
    void didFireTimer();

    void willChangeXHRReadyState(const String&, int);
    void didChangeXHRReadyState();
    void willLoadXHR(const String&);
    void didLoadXHR();

    void willEvaluateScript(const String&, int);
    void didEvaluateScript();

    void didMarkTimeline(const String&);
    void didMarkDOMContentEvent();
    void didMarkLoadEvent();

    void didScheduleResourceRequest(const String& url);
    void willSendResourceRequest(unsigned long, const ResourceRequest&);
    void willReceiveResourceResponse(unsigned long, const ResourceResponse&);
    void didReceiveResourceResponse();
    void didFinishLoadingResource(unsigned long, bool didFail, double finishTime);
    void willReceiveResourceData(unsigned long identifier);
    void didReceiveResourceData();
        
    virtual void didGC(double, double, size_t);

private:
    struct TimelineRecordEntry {
        TimelineRecordEntry(PassRefPtr<InspectorObject> record, PassRefPtr<InspectorObject> data, PassRefPtr<InspectorArray> children, const String& type)
            : record(record), data(data), children(children), type(type)
        {
        }
        RefPtr<InspectorObject> record;
        RefPtr<InspectorObject> data;
        RefPtr<InspectorArray> children;
        String type;
    };
        
    InspectorTimelineAgent(InstrumentingAgents*, InspectorState*);

    void pushCurrentRecord(PassRefPtr<InspectorObject>, const String& type);
    void setHeapSizeStatistic(InspectorObject* record);
        
    void didCompleteCurrentRecord(const String& type);

    void addRecordToTimeline(PassRefPtr<InspectorObject>, const String& type);

    void pushGCEventRecords();
    void clearRecordStack();

    InstrumentingAgents* m_instrumentingAgents;
    InspectorState* m_state;
    InspectorFrontend::Timeline* m_frontend;

    Vector<TimelineRecordEntry> m_recordStack;

    int m_id;
    struct GCEvent {
        GCEvent(double startTime, double endTime, size_t collectedBytes)
            : startTime(startTime), endTime(endTime), collectedBytes(collectedBytes)
        {
        }
        double startTime;
        double endTime;
        size_t collectedBytes;
    };
    typedef Vector<GCEvent> GCEvents;
    GCEvents m_gcEvents;
};

} // namespace WebCore

#endif // !ENABLE(INSPECTOR)
#endif // !defined(InspectorTimelineAgent_h)
