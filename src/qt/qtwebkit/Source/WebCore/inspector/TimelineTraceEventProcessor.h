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

#ifndef TimelineTraceEventProcessor_h
#define TimelineTraceEventProcessor_h

#if ENABLE(INSPECTOR)

#include "InspectorTimelineAgent.h"
#include "InspectorValues.h"

#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>
#include <wtf/WeakPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class InspectorClient;
class InspectorTimelineAgent;
class Page;

class TimelineRecordStack {
private:
    struct Entry {
        Entry(PassRefPtr<InspectorObject> record)
            : record(record)
            , children(InspectorArray::create())
        {
        }

        RefPtr<InspectorObject> record;
        RefPtr<InspectorArray> children;
    };

public:
    TimelineRecordStack() { }
    TimelineRecordStack(WeakPtr<InspectorTimelineAgent>);

    void addScopedRecord(PassRefPtr<InspectorObject> record);
    void closeScopedRecord(double endTime);
    void addInstantRecord(PassRefPtr<InspectorObject> record);

#ifndef NDEBUG
    bool isOpenRecordOfType(const String& type);
#endif

private:
    void send(PassRefPtr<InspectorObject>);

    WeakPtr<InspectorTimelineAgent> m_timelineAgent;
    Vector<Entry> m_stack;
};

class TimelineTraceEventProcessor : public ThreadSafeRefCounted<TimelineTraceEventProcessor> {
public:
    // FIXME: re-use definitions in TraceEvent.h once it's promoted to all platforms.
    enum TraceEventPhase {
        TracePhaseBegin = 'B',
        TracePhaseEnd = 'E',
        TracePhaseInstant = 'I',
        TracePhaseCreateObject = 'N',
        TracePhaseDeleteObject = 'D'
    };

    TimelineTraceEventProcessor(WeakPtr<InspectorTimelineAgent>, InspectorClient*);
    ~TimelineTraceEventProcessor();

    void shutdown();
    void processEventOnAnyThread(TraceEventPhase, const char* name, unsigned long long id,
        int numArgs, const char* const* argNames, const unsigned char* argTypes, const unsigned long long* argValues,
        unsigned char flags);

private:
    // FIXME: use the definition in TraceEvent.h once we expose the latter to all plaforms.
    union TraceValueUnion {
        bool m_bool;
        unsigned long long m_uint;
        long long m_int;
        double m_double;
        const void* m_pointer;
        const char* m_string;
    };

    enum TraceValueTypes {
        TypeBool = 1,
        TypeUInt = 2,
        TypeInt = 3,
        TypeDouble = 4,
        TypePointer = 5,
        TypeString = 6,
        TypeCopyString = 7
    };

    struct TimelineThreadState {
        TimelineThreadState() { }

        TimelineThreadState(WeakPtr<InspectorTimelineAgent> timelineAgent)
            : recordStack(timelineAgent)
            , inRasterizeEvent(false)
        {
        }

        TimelineRecordStack recordStack;
        bool inRasterizeEvent;
    };

    class TraceEvent {
    public:
        TraceEvent()
            : m_name(0)
            , m_argumentCount(0)
        {
        }

        TraceEvent(double timestamp, TraceEventPhase phase, const char* name, unsigned long long id, ThreadIdentifier threadIdentifier,
            int argumentCount, const char* const* argumentNames, const unsigned char* argumentTypes, const unsigned long long* argumentValues)
            : m_timestamp(timestamp)
            , m_phase(phase)
            , m_name(name)
            , m_id(id)
            , m_threadIdentifier(threadIdentifier)
            , m_argumentCount(argumentCount)
        {
            if (m_argumentCount > MaxArguments) {
                ASSERT_NOT_REACHED();
                m_argumentCount = MaxArguments;
            }
            for (int i = 0; i < m_argumentCount; ++i) {
                m_argumentNames[i] = argumentNames[i];
                m_argumentTypes[i] = argumentTypes[i];
                m_argumentValues[i] = argumentValues[i];
            }
        }

        double timestamp() const { return m_timestamp; }
        TraceEventPhase phase() const { return m_phase; }
        const char* name() const { return m_name; }
        unsigned long long id() const { return m_id; }
        ThreadIdentifier threadIdentifier() const { return m_threadIdentifier; }
        int argumentCount() const { return m_argumentCount; }
        bool isNull() const { return !m_name; }

        bool asBool(const char* name) const
        {
            return parameter(name, TypeBool).m_bool;
        }
        long long asInt(const char* name) const
        {
            size_t index = findParameter(name);
            if (index == notFound || (m_argumentTypes[index] != TypeInt && m_argumentTypes[index] != TypeUInt)) {
                ASSERT_NOT_REACHED();
                return 0;
            }
            return reinterpret_cast<const TraceValueUnion*>(m_argumentValues + index)->m_int;
        }
        unsigned long long asUInt(const char* name) const
        {
            return asInt(name);
        }
        double asDouble(const char* name) const
        {
            return parameter(name, TypeDouble).m_double;
        }
        const char* asString(const char* name) const
        {
            return parameter(name, TypeString).m_string;
        }

    private:
        enum { MaxArguments = 2 };

        size_t findParameter(const char*) const;
        const TraceValueUnion& parameter(const char* name, TraceValueTypes expectedType) const;

        double m_timestamp;
        TraceEventPhase m_phase;
        const char* m_name;
        unsigned long long m_id;
        ThreadIdentifier m_threadIdentifier;
        int m_argumentCount;
        const char* m_argumentNames[MaxArguments];
        unsigned char m_argumentTypes[MaxArguments];
        unsigned long long m_argumentValues[MaxArguments];
    };

    typedef void (TimelineTraceEventProcessor::*TraceEventHandler)(const TraceEvent&);

    TimelineThreadState& threadState(ThreadIdentifier thread)
    {
        ThreadStateMap::iterator it = m_threadStates.find(thread);
        if (it != m_threadStates.end())
            return it->value;
        return m_threadStates.add(thread, TimelineThreadState(m_timelineAgent)).iterator->value;
    }

    void processBackgroundEvents();
    PassRefPtr<InspectorObject> createRecord(const TraceEvent&, const String& recordType, PassRefPtr<InspectorObject> data = 0);

    void registerHandler(const char* name, TraceEventPhase, TraceEventHandler);

    void onBeginFrame(const TraceEvent&);
    void onPaintLayerBegin(const TraceEvent&);
    void onPaintLayerEnd(const TraceEvent&);
    void onRasterTaskBegin(const TraceEvent&);
    void onRasterTaskEnd(const TraceEvent&);
    void onImageDecodeBegin(const TraceEvent&);
    void onImageDecodeEnd(const TraceEvent&);
    void onLayerDeleted(const TraceEvent&);
    void onPaint(const TraceEvent&);

    WeakPtr<InspectorTimelineAgent> m_timelineAgent;
    TimelineTimeConverter m_timeConverter;
    InspectorClient* m_inspectorClient;
    unsigned long long m_pageId;

    typedef HashMap<std::pair<String, int>, TraceEventHandler> HandlersMap;
    HandlersMap m_handlersByType;
    Mutex m_backgroundEventsMutex;
    Vector<TraceEvent> m_backgroundEvents;

    typedef HashMap<ThreadIdentifier, TimelineThreadState> ThreadStateMap;
    ThreadStateMap m_threadStates;

    HashSet<unsigned long long> m_knownLayers;
    unsigned long long m_layerId;
};

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
#endif // !defined(TimelineTraceEventProcessor_h)
