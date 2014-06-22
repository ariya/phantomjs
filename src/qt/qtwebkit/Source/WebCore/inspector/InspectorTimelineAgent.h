/*
* Copyright (C) 2012 Google Inc. All rights reserved.
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

#include "InspectorBaseAgent.h"
#include "InspectorFrontend.h"
#include "InspectorValues.h"
#include "LayoutRect.h"
#include "PlatformInstrumentation.h"
#include "ScriptGCEvent.h"
#include "ScriptGCEventListener.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>
#include <wtf/WeakPtr.h>

namespace WebCore {
class Event;
class FloatQuad;
class Frame;
class InspectorClient;
class InspectorFrontend;
class InspectorMemoryAgent;
class InspectorPageAgent;
class InspectorState;
class InstrumentingAgents;
class IntRect;
class KURL;
class Page;
class RenderObject;
class ResourceRequest;
class ResourceResponse;
class TimelineTraceEventProcessor;

typedef String ErrorString;

namespace TimelineRecordType {
extern const char DecodeImage[];
extern const char Rasterize[];
};

class TimelineTimeConverter {
public:
    TimelineTimeConverter()
        : m_startOffset(0)
    {
    }
    double fromMonotonicallyIncreasingTime(double time) const  { return (time - m_startOffset) * 1000.0; }
    void reset();

private:
    double m_startOffset;
};

class InspectorTimelineAgent
    : public InspectorBaseAgent<InspectorTimelineAgent>,
      public ScriptGCEventListener,
      public InspectorBackendDispatcher::TimelineCommandHandler,
      public PlatformInstrumentationClient {
    WTF_MAKE_NONCOPYABLE(InspectorTimelineAgent);
public:
    enum InspectorType { PageInspector, WorkerInspector };

    static PassOwnPtr<InspectorTimelineAgent> create(InstrumentingAgents* instrumentingAgents, InspectorPageAgent* pageAgent, InspectorMemoryAgent* memoryAgent, InspectorCompositeState* state, InspectorType type, InspectorClient* client)
    {
        return adoptPtr(new InspectorTimelineAgent(instrumentingAgents, pageAgent, memoryAgent, state, type, client));
    }

    ~InspectorTimelineAgent();

    virtual void setFrontend(InspectorFrontend*);
    virtual void clearFrontend();
    virtual void restore();

    virtual void start(ErrorString*, const int* maxCallStackDepth, const bool* includeDomCounters, const bool* includeNativeMemoryStatistics);
    virtual void stop(ErrorString*);
    virtual void canMonitorMainThread(ErrorString*, bool*);
    virtual void supportsFrameInstrumentation(ErrorString*, bool*);

    int id() const { return m_id; }

    void didCommitLoad();

    // Methods called from WebCore.
    void willCallFunction(const String& scriptName, int scriptLine, Frame*);
    void didCallFunction();

    void willDispatchEvent(const Event&, Frame*);
    void didDispatchEvent();

    void didBeginFrame();
    void didCancelFrame();

    void didInvalidateLayout(Frame*);
    void willLayout(Frame*);
    void didLayout(RenderObject*);

    void didScheduleStyleRecalculation(Frame*);
    void willRecalculateStyle(Frame*);
    void didRecalculateStyle();

    void willPaint(Frame*);
    void didPaint(RenderObject*, const LayoutRect&);

    void willScroll(Frame*);
    void didScroll();

    void willComposite();
    void didComposite();

    void willWriteHTML(unsigned startLine, Frame*);
    void didWriteHTML(unsigned endLine);

    void didInstallTimer(int timerId, int timeout, bool singleShot, Frame*);
    void didRemoveTimer(int timerId, Frame*);
    void willFireTimer(int timerId, Frame*);
    void didFireTimer();

    void willDispatchXHRReadyStateChangeEvent(const String&, int, Frame*);
    void didDispatchXHRReadyStateChangeEvent();
    void willDispatchXHRLoadEvent(const String&, Frame*);
    void didDispatchXHRLoadEvent();

    void willEvaluateScript(const String&, int, Frame*);
    void didEvaluateScript();

    void didTimeStamp(Frame*, const String&);
    void didMarkDOMContentEvent(Frame*);
    void didMarkLoadEvent(Frame*);

    void time(Frame*, const String&);
    void timeEnd(Frame*, const String&);

    void didScheduleResourceRequest(const String& url, Frame*);
    void willSendResourceRequest(unsigned long, const ResourceRequest&, Frame*);
    void willReceiveResourceResponse(unsigned long, const ResourceResponse&, Frame*);
    void didReceiveResourceResponse();
    void didFinishLoadingResource(unsigned long, bool didFail, double finishTime, Frame*);
    void willReceiveResourceData(unsigned long identifier, Frame*, int length);
    void didReceiveResourceData();

    void didRequestAnimationFrame(int callbackId, Frame*);
    void didCancelAnimationFrame(int callbackId, Frame*);
    void willFireAnimationFrame(int callbackId, Frame*);
    void didFireAnimationFrame();

    void willProcessTask();
    void didProcessTask();

#if ENABLE(WEB_SOCKETS)
    void didCreateWebSocket(unsigned long identifier, const KURL&, const String& protocol, Frame*);
    void willSendWebSocketHandshakeRequest(unsigned long identifier, Frame*);
    void didReceiveWebSocketHandshakeResponse(unsigned long identifier, Frame*);
    void didDestroyWebSocket(unsigned long identifier, Frame*);
#endif

    // ScriptGCEventListener methods.
    virtual void didGC(double, double, size_t);

    // PlatformInstrumentationClient methods.
    virtual void willDecodeImage(const String& imageType) OVERRIDE;
    virtual void didDecodeImage() OVERRIDE;
    virtual void willResizeImage(bool shouldCache) OVERRIDE;
    virtual void didResizeImage() OVERRIDE;

private:
    friend class TimelineRecordStack;
    friend class TimelineTraceEventProcessor;

    struct TimelineRecordEntry {
        TimelineRecordEntry(PassRefPtr<InspectorObject> record, PassRefPtr<InspectorObject> data, PassRefPtr<InspectorArray> children, const String& type, size_t usedHeapSizeAtStart)
            : record(record), data(data), children(children), type(type), usedHeapSizeAtStart(usedHeapSizeAtStart)
        {
        }
        RefPtr<InspectorObject> record;
        RefPtr<InspectorObject> data;
        RefPtr<InspectorArray> children;
        String type;
        size_t usedHeapSizeAtStart;
    };
        
    InspectorTimelineAgent(InstrumentingAgents*, InspectorPageAgent*, InspectorMemoryAgent*, InspectorCompositeState*, InspectorType, InspectorClient*);

    void sendEvent(PassRefPtr<InspectorObject>);
    void appendRecord(PassRefPtr<InspectorObject> data, const String& type, bool captureCallStack, Frame*);
    void pushCurrentRecord(PassRefPtr<InspectorObject>, const String& type, bool captureCallStack, Frame*, bool hasLowLevelDetails = false);

    void setDOMCounters(TypeBuilder::Timeline::TimelineEvent* record);
    void setNativeHeapStatistics(TypeBuilder::Timeline::TimelineEvent* record);
    void setFrameIdentifier(InspectorObject* record, Frame*);
    void pushGCEventRecords();

    void didCompleteCurrentRecord(const String& type);

    void setHeapSizeStatistics(InspectorObject* record);
    void commitFrameRecord();

    void addRecordToTimeline(PassRefPtr<InspectorObject>, const String& type);
    void innerAddRecordToTimeline(PassRefPtr<InspectorObject>, const String& type);
    void clearRecordStack();

    void localToPageQuad(const RenderObject& renderer, const LayoutRect&, FloatQuad*);
    const TimelineTimeConverter& timeConverter() const { return m_timeConverter; }
    double timestamp();
    Page* page();

    InspectorPageAgent* m_pageAgent;
    InspectorMemoryAgent* m_memoryAgent;
    TimelineTimeConverter m_timeConverter;

    InspectorFrontend::Timeline* m_frontend;
    double m_timestampOffset;

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
    int m_maxCallStackDepth;
    unsigned m_platformInstrumentationClientInstalledAtStackDepth;
    RefPtr<InspectorObject> m_pendingFrameRecord;
    InspectorType m_inspectorType;
    InspectorClient* m_client;
    WeakPtrFactory<InspectorTimelineAgent> m_weakFactory;
    RefPtr<TimelineTraceEventProcessor> m_traceEventProcessor;
};

} // namespace WebCore

#endif // !ENABLE(INSPECTOR)
#endif // !defined(InspectorTimelineAgent_h)
