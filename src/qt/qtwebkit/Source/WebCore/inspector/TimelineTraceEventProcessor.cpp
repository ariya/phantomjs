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

#if ENABLE(INSPECTOR)

#include "TimelineTraceEventProcessor.h"

#include "InspectorClient.h"
#include "InspectorInstrumentation.h"
#include "TimelineRecordFactory.h"

#include <wtf/CurrentTime.h>
#include <wtf/MainThread.h>
#include <wtf/ThreadSpecific.h>
#include <wtf/Vector.h>

namespace WebCore {

namespace {

class TraceEventDispatcher {
    WTF_MAKE_NONCOPYABLE(TraceEventDispatcher);
public:
    static TraceEventDispatcher* instance()
    {
        DEFINE_STATIC_LOCAL(TraceEventDispatcher, instance, ());
        return &instance;
    }

    void addProcessor(TimelineTraceEventProcessor* processor, InspectorClient* client)
    {
        MutexLocker locker(m_mutex);

        m_processors.append(processor);
        if (m_processors.size() == 1)
            client->setTraceEventCallback(dispatchEventOnAnyThread);
    }

    void removeProcessor(TimelineTraceEventProcessor* processor, InspectorClient* client)
    {
        MutexLocker locker(m_mutex);

        size_t index = m_processors.find(processor);
        if (index == notFound) {
            ASSERT_NOT_REACHED();
            return;
        }
        m_processors.remove(index);
        if (m_processors.isEmpty())
            client->setTraceEventCallback(0);
    }

private:
    TraceEventDispatcher() { }

    static void dispatchEventOnAnyThread(char phase, const unsigned char*, const char* name, unsigned long long id,
        int numArgs, const char* const* argNames, const unsigned char* argTypes, const unsigned long long* argValues,
        unsigned char flags)
    {
        TraceEventDispatcher* self = instance();
        Vector<RefPtr<TimelineTraceEventProcessor> > processors;
        {
            MutexLocker locker(self->m_mutex);
            processors = self->m_processors;
        }
        for (int i = 0, size = processors.size(); i < size; ++i) {
            processors[i]->processEventOnAnyThread(static_cast<TimelineTraceEventProcessor::TraceEventPhase>(phase),
                name, id, numArgs, argNames, argTypes, argValues, flags);
        }
    }

    Mutex m_mutex;
    Vector<RefPtr<TimelineTraceEventProcessor> > m_processors;
};

} // namespce


TimelineRecordStack::TimelineRecordStack(WeakPtr<InspectorTimelineAgent> timelineAgent)
    : m_timelineAgent(timelineAgent)
{
}

void TimelineRecordStack::addScopedRecord(PassRefPtr<InspectorObject> record)
{
    m_stack.append(Entry(record));
}

void TimelineRecordStack::closeScopedRecord(double endTime)
{
    if (m_stack.isEmpty())
        return;
    Entry last = m_stack.last();
    m_stack.removeLast();
    last.record->setNumber("endTime", endTime);
    if (last.children->length())
        last.record->setArray("children", last.children);
    addInstantRecord(last.record);
}

void TimelineRecordStack::addInstantRecord(PassRefPtr<InspectorObject> record)
{
    if (m_stack.isEmpty())
        send(record);
    else
        m_stack.last().children->pushObject(record);
}

#ifndef NDEBUG
bool TimelineRecordStack::isOpenRecordOfType(const String& type)
{
    String lastRecordType;
    return m_stack.isEmpty() || (m_stack.last().record->getString("type", &lastRecordType) && type == lastRecordType);
}
#endif

void TimelineRecordStack::send(PassRefPtr<InspectorObject> record)
{
    InspectorTimelineAgent* timelineAgent = m_timelineAgent.get();
    if (!timelineAgent)
        return;
    timelineAgent->sendEvent(record);
}

TimelineTraceEventProcessor::TimelineTraceEventProcessor(WeakPtr<InspectorTimelineAgent> timelineAgent, InspectorClient *client)
    : m_timelineAgent(timelineAgent)
    , m_timeConverter(timelineAgent.get()->timeConverter())
    , m_inspectorClient(client)
    , m_pageId(reinterpret_cast<unsigned long long>(m_timelineAgent.get()->page()))
    , m_layerId(0)
{
    registerHandler(InstrumentationEvents::BeginFrame, TracePhaseInstant, &TimelineTraceEventProcessor::onBeginFrame);
    registerHandler(InstrumentationEvents::PaintLayer, TracePhaseBegin, &TimelineTraceEventProcessor::onPaintLayerBegin);
    registerHandler(InstrumentationEvents::PaintLayer, TracePhaseEnd, &TimelineTraceEventProcessor::onPaintLayerEnd);
    registerHandler(InstrumentationEvents::RasterTask, TracePhaseBegin, &TimelineTraceEventProcessor::onRasterTaskBegin);
    registerHandler(InstrumentationEvents::RasterTask, TracePhaseEnd, &TimelineTraceEventProcessor::onRasterTaskEnd);
    registerHandler(InstrumentationEvents::Layer, TracePhaseDeleteObject, &TimelineTraceEventProcessor::onLayerDeleted);
    registerHandler(InstrumentationEvents::Paint, TracePhaseInstant, &TimelineTraceEventProcessor::onPaint);
    registerHandler(PlatformInstrumentation::ImageDecodeEvent, TracePhaseBegin, &TimelineTraceEventProcessor::onImageDecodeBegin);
    registerHandler(PlatformInstrumentation::ImageDecodeEvent, TracePhaseEnd, &TimelineTraceEventProcessor::onImageDecodeEnd);

    TraceEventDispatcher::instance()->addProcessor(this, m_inspectorClient);
}

TimelineTraceEventProcessor::~TimelineTraceEventProcessor()
{
}

void TimelineTraceEventProcessor::registerHandler(const char* name, TraceEventPhase phase, TraceEventHandler handler)
{
    m_handlersByType.set(std::make_pair(name, phase), handler);
}

void TimelineTraceEventProcessor::shutdown()
{
    TraceEventDispatcher::instance()->removeProcessor(this, m_inspectorClient);
}

size_t TimelineTraceEventProcessor::TraceEvent::findParameter(const char* name) const
{
    for (int i = 0; i < m_argumentCount; ++i) {
        if (!strcmp(name, m_argumentNames[i]))
            return i;
    }
    return notFound;
}

const TimelineTraceEventProcessor::TraceValueUnion& TimelineTraceEventProcessor::TraceEvent::parameter(const char* name, TraceValueTypes expectedType) const
{
    static TraceValueUnion missingValue;
    size_t index = findParameter(name);
    if (index == notFound || m_argumentTypes[index] != expectedType) {
        ASSERT_NOT_REACHED();
        return missingValue;
    }
    return *reinterpret_cast<const TraceValueUnion*>(m_argumentValues + index);
}

void TimelineTraceEventProcessor::processEventOnAnyThread(TraceEventPhase phase, const char* name, unsigned long long id,
    int numArgs, const char* const* argNames, const unsigned char* argTypes, const unsigned long long* argValues,
    unsigned char)
{
    HandlersMap::iterator it = m_handlersByType.find(std::make_pair(name, phase));
    if (it == m_handlersByType.end())
        return;

    TraceEvent event(WTF::monotonicallyIncreasingTime(), phase, name, id, currentThread(), numArgs, argNames, argTypes, argValues);

    if (!isMainThread()) {
        MutexLocker locker(m_backgroundEventsMutex);
        m_backgroundEvents.append(event);
        return;
    }
    (this->*(it->value))(event);
}

void TimelineTraceEventProcessor::onBeginFrame(const TraceEvent&)
{
    processBackgroundEvents();
}

void TimelineTraceEventProcessor::onPaintLayerBegin(const TraceEvent& event)
{
    m_layerId = event.asUInt(InstrumentationEventArguments::LayerId);
    ASSERT(m_layerId);
}

void TimelineTraceEventProcessor::onPaintLayerEnd(const TraceEvent&)
{
    m_layerId = 0;
}

void TimelineTraceEventProcessor::onRasterTaskBegin(const TraceEvent& event)
{
    unsigned long long layerId = event.asUInt(InstrumentationEventArguments::LayerId);
    if (!m_knownLayers.contains(layerId))
        return;
    TimelineThreadState& state = threadState(event.threadIdentifier());
    ASSERT(!state.inRasterizeEvent);
    state.inRasterizeEvent = true;
    RefPtr<InspectorObject> record = createRecord(event, TimelineRecordType::Rasterize);
    state.recordStack.addScopedRecord(record.release());
}

void TimelineTraceEventProcessor::onRasterTaskEnd(const TraceEvent& event)
{
    TimelineThreadState& state = threadState(event.threadIdentifier());
    if (!state.inRasterizeEvent)
        return;
    ASSERT(state.recordStack.isOpenRecordOfType(TimelineRecordType::Rasterize));
    state.recordStack.closeScopedRecord(m_timeConverter.fromMonotonicallyIncreasingTime(event.timestamp()));
    state.inRasterizeEvent = false;
}

void TimelineTraceEventProcessor::onImageDecodeBegin(const TraceEvent& event)
{
    TimelineThreadState& state = threadState(event.threadIdentifier());
    if (!state.inRasterizeEvent)
        return;
    state.recordStack.addScopedRecord(createRecord(event, TimelineRecordType::DecodeImage));
}

void TimelineTraceEventProcessor::onImageDecodeEnd(const TraceEvent& event)
{
    TimelineThreadState& state = threadState(event.threadIdentifier());
    if (!state.inRasterizeEvent)
        return;
    ASSERT(state.recordStack.isOpenRecordOfType(TimelineRecordType::DecodeImage));
    state.recordStack.closeScopedRecord(m_timeConverter.fromMonotonicallyIncreasingTime(event.timestamp()));
}

void TimelineTraceEventProcessor::onLayerDeleted(const TraceEvent& event)
{
    unsigned long long id = event.id();
    ASSERT(id);
    processBackgroundEvents();
    m_knownLayers.remove(id);
}

void TimelineTraceEventProcessor::onPaint(const TraceEvent& event)
{
    if (!m_layerId)
        return;

    unsigned long long pageId = event.asUInt(InstrumentationEventArguments::PageId);
    if (pageId == m_pageId)
        m_knownLayers.add(m_layerId);
}

PassRefPtr<InspectorObject> TimelineTraceEventProcessor::createRecord(const TraceEvent& event, const String& recordType, PassRefPtr<InspectorObject> data)
{
    double startTime = m_timeConverter.fromMonotonicallyIncreasingTime(event.timestamp());
    RefPtr<InspectorObject> record = TimelineRecordFactory::createBackgroundRecord(startTime, String::number(event.threadIdentifier()));
    record->setString("type", recordType);
    record->setObject("data", data ? data : InspectorObject::create());
    return record.release();
}

void TimelineTraceEventProcessor::processBackgroundEvents()
{
    ASSERT(isMainThread());
    Vector<TraceEvent> events;
    {
        MutexLocker locker(m_backgroundEventsMutex);
        events.reserveCapacity(m_backgroundEvents.capacity());
        m_backgroundEvents.swap(events);
    }
    for (size_t i = 0, size = events.size(); i < size; ++i) {
        const TraceEvent& event = events[i];
        HandlersMap::iterator it = m_handlersByType.find(std::make_pair(event.name(), event.phase()));
        ASSERT(it != m_handlersByType.end() && it->value);
        (this->*(it->value))(event);
    }
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
