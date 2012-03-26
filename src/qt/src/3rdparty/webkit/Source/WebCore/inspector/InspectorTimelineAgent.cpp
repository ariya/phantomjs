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

#include "config.h"
#include "InspectorTimelineAgent.h"

#if ENABLE(INSPECTOR)

#include "Event.h"
#include "InspectorFrontend.h"
#include "InspectorState.h"
#include "InstrumentingAgents.h"
#include "IntRect.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "TimelineRecordFactory.h"

#include <wtf/CurrentTime.h>

namespace WebCore {

namespace TimelineAgentState {
static const char timelineAgentEnabled[] = "timelineAgentEnabled";
}

namespace TimelineRecordType {
static const char EventDispatch[] = "EventDispatch";
static const char Layout[] = "Layout";
static const char RecalculateStyles[] = "RecalculateStyles";
static const char Paint[] = "Paint";
static const char ParseHTML[] = "ParseHTML";

static const char TimerInstall[] = "TimerInstall";
static const char TimerRemove[] = "TimerRemove";
static const char TimerFire[] = "TimerFire";

static const char EvaluateScript[] = "EvaluateScript";

static const char MarkLoad[] = "MarkLoad";
static const char MarkDOMContent[] = "MarkDOMContent";
static const char MarkTimeline[] = "MarkTimeline";

static const char ScheduleResourceRequest[] = "ScheduleResourceRequest";
static const char ResourceSendRequest[] = "ResourceSendRequest";
static const char ResourceReceiveResponse[] = "ResourceReceiveResponse";
static const char ResourceReceivedData[] = "ResourceReceivedData";
static const char ResourceFinish[] = "ResourceFinish";

static const char XHRReadyStateChange[] = "XHRReadyStateChange";
static const char XHRLoad[] = "XHRLoad";

static const char FunctionCall[] = "FunctionCall";
static const char GCEvent[] = "GCEvent";
}

void InspectorTimelineAgent::pushGCEventRecords()
{
    if (!m_gcEvents.size())
        return;

    GCEvents events = m_gcEvents;
    m_gcEvents.clear();
    for (GCEvents::iterator i = events.begin(); i != events.end(); ++i) {
        RefPtr<InspectorObject> record = TimelineRecordFactory::createGenericRecord(i->startTime);
        record->setObject("data", TimelineRecordFactory::createGCEventData(i->collectedBytes));
        record->setNumber("endTime", i->endTime);
        addRecordToTimeline(record.release(), TimelineRecordType::GCEvent);
    }
}

void InspectorTimelineAgent::didGC(double startTime, double endTime, size_t collectedBytesCount)
{
    m_gcEvents.append(GCEvent(startTime, endTime, collectedBytesCount));
}

InspectorTimelineAgent::~InspectorTimelineAgent()
{
    clearFrontend();
}

void InspectorTimelineAgent::setFrontend(InspectorFrontend* frontend)
{
    m_frontend = frontend->timeline();
}

void InspectorTimelineAgent::clearFrontend()
{
    ErrorString error;
    stop(&error);
    m_frontend = 0;
}

void InspectorTimelineAgent::restore()
{
    if (m_state->getBoolean(TimelineAgentState::timelineAgentEnabled)) {
        ErrorString error;
        start(&error);
    }
}

void InspectorTimelineAgent::start(ErrorString*)
{
    if (!m_frontend)
        return;
    m_instrumentingAgents->setInspectorTimelineAgent(this);
    ScriptGCEvent::addEventListener(this);
    m_frontend->started();
    m_state->setBoolean(TimelineAgentState::timelineAgentEnabled, true);
}

void InspectorTimelineAgent::stop(ErrorString*)
{
    if (!started())
        return;
    m_instrumentingAgents->setInspectorTimelineAgent(0);
    if (m_frontend)
        m_frontend->stopped();
    ScriptGCEvent::removeEventListener(this);

    clearRecordStack();
    m_gcEvents.clear();

    m_state->setBoolean(TimelineAgentState::timelineAgentEnabled, false);
}

bool InspectorTimelineAgent::started() const
{
    return m_state->getBoolean(TimelineAgentState::timelineAgentEnabled);
}

void InspectorTimelineAgent::willCallFunction(const String& scriptName, int scriptLine)
{
    pushCurrentRecord(TimelineRecordFactory::createFunctionCallData(scriptName, scriptLine), TimelineRecordType::FunctionCall);
}

void InspectorTimelineAgent::didCallFunction()
{
    didCompleteCurrentRecord(TimelineRecordType::FunctionCall);
}

void InspectorTimelineAgent::willDispatchEvent(const Event& event)
{
    pushCurrentRecord(TimelineRecordFactory::createEventDispatchData(event),
        TimelineRecordType::EventDispatch);
}

void InspectorTimelineAgent::didDispatchEvent()
{
    didCompleteCurrentRecord(TimelineRecordType::EventDispatch);
}

void InspectorTimelineAgent::willLayout()
{
    pushCurrentRecord(InspectorObject::create(), TimelineRecordType::Layout);
}

void InspectorTimelineAgent::didLayout()
{
    didCompleteCurrentRecord(TimelineRecordType::Layout);
}

void InspectorTimelineAgent::willRecalculateStyle()
{
    pushCurrentRecord(InspectorObject::create(), TimelineRecordType::RecalculateStyles);
}

void InspectorTimelineAgent::didRecalculateStyle()
{
    didCompleteCurrentRecord(TimelineRecordType::RecalculateStyles);
}

void InspectorTimelineAgent::willPaint(const IntRect& rect)
{
    pushCurrentRecord(TimelineRecordFactory::createPaintData(rect), TimelineRecordType::Paint);
}

void InspectorTimelineAgent::didPaint()
{
    didCompleteCurrentRecord(TimelineRecordType::Paint);
}

void InspectorTimelineAgent::willWriteHTML(unsigned int length, unsigned int startLine)
{
    pushCurrentRecord(TimelineRecordFactory::createParseHTMLData(length, startLine), TimelineRecordType::ParseHTML);
}

void InspectorTimelineAgent::didWriteHTML(unsigned int endLine)
{
    if (!m_recordStack.isEmpty()) {
        TimelineRecordEntry entry = m_recordStack.last();
        entry.data->setNumber("endLine", endLine);
        didCompleteCurrentRecord(TimelineRecordType::ParseHTML);
    }
}

void InspectorTimelineAgent::didInstallTimer(int timerId, int timeout, bool singleShot)
{
    pushGCEventRecords();
    RefPtr<InspectorObject> record = TimelineRecordFactory::createGenericRecord(WTF::currentTimeMS());
    record->setObject("data", TimelineRecordFactory::createTimerInstallData(timerId, timeout, singleShot));
    addRecordToTimeline(record.release(), TimelineRecordType::TimerInstall);
}

void InspectorTimelineAgent::didRemoveTimer(int timerId)
{
    pushGCEventRecords();
    RefPtr<InspectorObject> record = TimelineRecordFactory::createGenericRecord(WTF::currentTimeMS());
    record->setObject("data", TimelineRecordFactory::createGenericTimerData(timerId));
    addRecordToTimeline(record.release(), TimelineRecordType::TimerRemove);
}

void InspectorTimelineAgent::willFireTimer(int timerId)
{
    pushCurrentRecord(TimelineRecordFactory::createGenericTimerData(timerId), TimelineRecordType::TimerFire);
}

void InspectorTimelineAgent::didFireTimer()
{
    didCompleteCurrentRecord(TimelineRecordType::TimerFire);
}

void InspectorTimelineAgent::willChangeXHRReadyState(const String& url, int readyState)
{
    pushCurrentRecord(TimelineRecordFactory::createXHRReadyStateChangeData(url, readyState), TimelineRecordType::XHRReadyStateChange);
}

void InspectorTimelineAgent::didChangeXHRReadyState()
{
    didCompleteCurrentRecord(TimelineRecordType::XHRReadyStateChange);
}

void InspectorTimelineAgent::willLoadXHR(const String& url) 
{
    pushCurrentRecord(TimelineRecordFactory::createXHRLoadData(url), TimelineRecordType::XHRLoad);
}

void InspectorTimelineAgent::didLoadXHR()
{
    didCompleteCurrentRecord(TimelineRecordType::XHRLoad);
}

void InspectorTimelineAgent::willEvaluateScript(const String& url, int lineNumber)
{
    pushCurrentRecord(TimelineRecordFactory::createEvaluateScriptData(url, lineNumber), TimelineRecordType::EvaluateScript);
}
    
void InspectorTimelineAgent::didEvaluateScript()
{
    didCompleteCurrentRecord(TimelineRecordType::EvaluateScript);
}

void InspectorTimelineAgent::didScheduleResourceRequest(const String& url)
{
    pushGCEventRecords();
    RefPtr<InspectorObject> record = TimelineRecordFactory::createGenericRecord(WTF::currentTimeMS());
    record->setObject("data", TimelineRecordFactory::createScheduleResourceRequestData(url));
    record->setString("type", TimelineRecordType::ScheduleResourceRequest);
    addRecordToTimeline(record.release(), TimelineRecordType::ScheduleResourceRequest);
}

void InspectorTimelineAgent::willSendResourceRequest(unsigned long identifier, const ResourceRequest& request)
{
    pushGCEventRecords();
    RefPtr<InspectorObject> record = TimelineRecordFactory::createGenericRecord(WTF::currentTimeMS());
    record->setObject("data", TimelineRecordFactory::createResourceSendRequestData(identifier, request));
    record->setString("type", TimelineRecordType::ResourceSendRequest);
    setHeapSizeStatistic(record.get());
    m_frontend->eventRecorded(record.release());
}

void InspectorTimelineAgent::willReceiveResourceData(unsigned long identifier)
{
    pushCurrentRecord(TimelineRecordFactory::createReceiveResourceData(identifier), TimelineRecordType::ResourceReceivedData);
}

void InspectorTimelineAgent::didReceiveResourceData()
{
    didCompleteCurrentRecord(TimelineRecordType::ResourceReceivedData);
}
    
void InspectorTimelineAgent::willReceiveResourceResponse(unsigned long identifier, const ResourceResponse& response)
{
    pushCurrentRecord(TimelineRecordFactory::createResourceReceiveResponseData(identifier, response), TimelineRecordType::ResourceReceiveResponse);
}

void InspectorTimelineAgent::didReceiveResourceResponse()
{
    didCompleteCurrentRecord(TimelineRecordType::ResourceReceiveResponse);
}

void InspectorTimelineAgent::didFinishLoadingResource(unsigned long identifier, bool didFail, double finishTime)
{
    pushGCEventRecords();
    // Sometimes network stack can provide for us exact finish loading time. In the other case we will use currentTime.
    RefPtr<InspectorObject> record = TimelineRecordFactory::createGenericRecord(WTF::currentTimeMS());
    record->setObject("data", TimelineRecordFactory::createResourceFinishData(identifier, didFail, finishTime * 1000));
    record->setString("type", TimelineRecordType::ResourceFinish);
    setHeapSizeStatistic(record.get());
    m_frontend->eventRecorded(record.release());
}

void InspectorTimelineAgent::didMarkTimeline(const String& message)
{
    pushGCEventRecords();
    RefPtr<InspectorObject> record = TimelineRecordFactory::createGenericRecord(WTF::currentTimeMS());
    record->setObject("data", TimelineRecordFactory::createMarkTimelineData(message));
    addRecordToTimeline(record.release(), TimelineRecordType::MarkTimeline);
}

void InspectorTimelineAgent::didMarkDOMContentEvent()
{
    pushGCEventRecords();
    RefPtr<InspectorObject> record = TimelineRecordFactory::createGenericRecord(WTF::currentTimeMS());
    addRecordToTimeline(record.release(), TimelineRecordType::MarkDOMContent);
}

void InspectorTimelineAgent::didMarkLoadEvent()
{
    pushGCEventRecords();
    RefPtr<InspectorObject> record = TimelineRecordFactory::createGenericRecord(WTF::currentTimeMS());
    addRecordToTimeline(record.release(), TimelineRecordType::MarkLoad);
}

void InspectorTimelineAgent::didCommitLoad()
{
    clearRecordStack();
}

void InspectorTimelineAgent::addRecordToTimeline(PassRefPtr<InspectorObject> prpRecord, const String& type)
{
    RefPtr<InspectorObject> record(prpRecord);
    record->setString("type", type);
    setHeapSizeStatistic(record.get());
    if (m_recordStack.isEmpty())
        m_frontend->eventRecorded(record.release());
    else {
        TimelineRecordEntry parent = m_recordStack.last();
        parent.children->pushObject(record.release());
    }
}

void InspectorTimelineAgent::setHeapSizeStatistic(InspectorObject* record)
{
    size_t usedHeapSize = 0;
    size_t totalHeapSize = 0;
    size_t heapSizeLimit = 0;
    ScriptGCEvent::getHeapSize(usedHeapSize, totalHeapSize, heapSizeLimit);
    record->setNumber("usedHeapSize", usedHeapSize);
    record->setNumber("totalHeapSize", totalHeapSize);
}

void InspectorTimelineAgent::didCompleteCurrentRecord(const String& type)
{
    // An empty stack could merely mean that the timeline agent was turned on in the middle of
    // an event.  Don't treat as an error.
    if (!m_recordStack.isEmpty()) {
        pushGCEventRecords();
        TimelineRecordEntry entry = m_recordStack.last();
        m_recordStack.removeLast();
        ASSERT(entry.type == type);
        entry.record->setObject("data", entry.data);
        entry.record->setArray("children", entry.children);
        entry.record->setNumber("endTime", WTF::currentTimeMS());
        addRecordToTimeline(entry.record, type);
    }
}

InspectorTimelineAgent::InspectorTimelineAgent(InstrumentingAgents* instrumentingAgents, InspectorState* state)
    : m_instrumentingAgents(instrumentingAgents)
    , m_state(state)
    , m_frontend(0)
    , m_id(1)
{
}

void InspectorTimelineAgent::pushCurrentRecord(PassRefPtr<InspectorObject> data, const String& type)
{
    pushGCEventRecords();
    RefPtr<InspectorObject> record = TimelineRecordFactory::createGenericRecord(WTF::currentTimeMS());
    m_recordStack.append(TimelineRecordEntry(record.release(), data, InspectorArray::create(), type));
}

void InspectorTimelineAgent::clearRecordStack()
{
    m_recordStack.clear();
    m_id++;
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
