/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef InstrumentingAgents_h
#define InstrumentingAgents_h

#include <wtf/FastAllocBase.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class InspectorAgent;
class InspectorApplicationCacheAgent;
class InspectorCSSAgent;
class InspectorCanvasAgent;
class InspectorConsoleAgent;
class InspectorDOMAgent;
class InspectorDOMDebuggerAgent;
class InspectorDOMStorageAgent;
class InspectorDatabaseAgent;
class InspectorDebuggerAgent;
class InspectorFileSystemAgent;
class InspectorHeapProfilerAgent;
class InspectorLayerTreeAgent;
class InspectorPageAgent;
class InspectorProfilerAgent;
class InspectorResourceAgent;
class InspectorTimelineAgent;
class InspectorWorkerAgent;
class Page;
class PageDebuggerAgent;
class PageRuntimeAgent;
class WorkerGlobalScope;
class WorkerRuntimeAgent;

class InstrumentingAgents : public RefCounted<InstrumentingAgents> {
    WTF_MAKE_NONCOPYABLE(InstrumentingAgents);
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassRefPtr<InstrumentingAgents> create()
    {
        return adoptRef(new InstrumentingAgents());
    }
    ~InstrumentingAgents() { }
    void reset();

    InspectorAgent* inspectorAgent() const { return m_inspectorAgent; }
    void setInspectorAgent(InspectorAgent* agent) { m_inspectorAgent = agent; }

    InspectorPageAgent* inspectorPageAgent() const { return m_inspectorPageAgent; }
    void setInspectorPageAgent(InspectorPageAgent* agent) { m_inspectorPageAgent = agent; }

    InspectorCSSAgent* inspectorCSSAgent() const { return m_inspectorCSSAgent; }
    void setInspectorCSSAgent(InspectorCSSAgent* agent) { m_inspectorCSSAgent = agent; }

    InspectorConsoleAgent* inspectorConsoleAgent() const { return m_inspectorConsoleAgent; }
    void setInspectorConsoleAgent(InspectorConsoleAgent* agent) { m_inspectorConsoleAgent = agent; }

    InspectorDOMAgent* inspectorDOMAgent() const { return m_inspectorDOMAgent; }
    void setInspectorDOMAgent(InspectorDOMAgent* agent) { m_inspectorDOMAgent = agent; }

    InspectorResourceAgent* inspectorResourceAgent() const { return m_inspectorResourceAgent; }
    void setInspectorResourceAgent(InspectorResourceAgent* agent) { m_inspectorResourceAgent = agent; }

    PageRuntimeAgent* pageRuntimeAgent() const { return m_pageRuntimeAgent; }
    void setPageRuntimeAgent(PageRuntimeAgent* agent) { m_pageRuntimeAgent = agent; }

    WorkerRuntimeAgent* workerRuntimeAgent() const { return m_workerRuntimeAgent; }
    void setWorkerRuntimeAgent(WorkerRuntimeAgent* agent) { m_workerRuntimeAgent = agent; }

    InspectorTimelineAgent* inspectorTimelineAgent() const { return m_inspectorTimelineAgent; }
    void setInspectorTimelineAgent(InspectorTimelineAgent* agent) { m_inspectorTimelineAgent = agent; }

    InspectorDOMStorageAgent* inspectorDOMStorageAgent() const { return m_inspectorDOMStorageAgent; }
    void setInspectorDOMStorageAgent(InspectorDOMStorageAgent* agent) { m_inspectorDOMStorageAgent = agent; }

#if ENABLE(SQL_DATABASE)
    InspectorDatabaseAgent* inspectorDatabaseAgent() const { return m_inspectorDatabaseAgent; }
    void setInspectorDatabaseAgent(InspectorDatabaseAgent* agent) { m_inspectorDatabaseAgent = agent; }
#endif

#if ENABLE(FILE_SYSTEM)
    InspectorFileSystemAgent* inspectorFileSystemAgent() const { return m_inspectorFileSystemAgent; }
    void setInspectorFileSystemAgent(InspectorFileSystemAgent* agent) { m_inspectorFileSystemAgent = agent; }
#endif

    InspectorApplicationCacheAgent* inspectorApplicationCacheAgent() const { return m_inspectorApplicationCacheAgent; }
    void setInspectorApplicationCacheAgent(InspectorApplicationCacheAgent* agent) { m_inspectorApplicationCacheAgent = agent; }

#if ENABLE(JAVASCRIPT_DEBUGGER)
    InspectorDebuggerAgent* inspectorDebuggerAgent() const { return m_inspectorDebuggerAgent; }
    void setInspectorDebuggerAgent(InspectorDebuggerAgent* agent) { m_inspectorDebuggerAgent = agent; }

    PageDebuggerAgent* pageDebuggerAgent() const { return m_pageDebuggerAgent; }
    void setPageDebuggerAgent(PageDebuggerAgent* agent) { m_pageDebuggerAgent = agent; }

    InspectorDOMDebuggerAgent* inspectorDOMDebuggerAgent() const { return m_inspectorDOMDebuggerAgent; }
    void setInspectorDOMDebuggerAgent(InspectorDOMDebuggerAgent* agent) { m_inspectorDOMDebuggerAgent = agent; }

    InspectorProfilerAgent* inspectorProfilerAgent() const { return m_inspectorProfilerAgent; }
    void setInspectorProfilerAgent(InspectorProfilerAgent* agent) { m_inspectorProfilerAgent = agent; }

    InspectorHeapProfilerAgent* inspectorHeapProfilerAgent() const { return m_inspectorHeapProfilerAgent; }
    void setInspectorHeapProfilerAgent(InspectorHeapProfilerAgent* agent) { m_inspectorHeapProfilerAgent = agent; }
#endif

#if ENABLE(WORKERS)
    InspectorWorkerAgent* inspectorWorkerAgent() const { return m_inspectorWorkerAgent; }
    void setInspectorWorkerAgent(InspectorWorkerAgent* agent) { m_inspectorWorkerAgent = agent; }
#endif

    InspectorCanvasAgent* inspectorCanvasAgent() const { return m_inspectorCanvasAgent; }
    void setInspectorCanvasAgent(InspectorCanvasAgent* agent) { m_inspectorCanvasAgent = agent; }

#if USE(ACCELERATED_COMPOSITING)
    InspectorLayerTreeAgent* inspectorLayerTreeAgent() const { return m_inspectorLayerTreeAgent; }
    void setInspectorLayerTreeAgent(InspectorLayerTreeAgent* agent) { m_inspectorLayerTreeAgent = agent; }
#endif

private:
    InstrumentingAgents();

    InspectorAgent* m_inspectorAgent;
    InspectorPageAgent* m_inspectorPageAgent;
    InspectorCSSAgent* m_inspectorCSSAgent;
#if USE(ACCELERATED_COMPOSITING)
    InspectorLayerTreeAgent* m_inspectorLayerTreeAgent;
#endif
    InspectorConsoleAgent* m_inspectorConsoleAgent;
    InspectorDOMAgent* m_inspectorDOMAgent;
    InspectorResourceAgent* m_inspectorResourceAgent;
    PageRuntimeAgent* m_pageRuntimeAgent;
    WorkerRuntimeAgent* m_workerRuntimeAgent;
    InspectorTimelineAgent* m_inspectorTimelineAgent;
    InspectorDOMStorageAgent* m_inspectorDOMStorageAgent;
#if ENABLE(SQL_DATABASE)
    InspectorDatabaseAgent* m_inspectorDatabaseAgent;
#endif
#if ENABLE(FILE_SYSTEM)
    InspectorFileSystemAgent* m_inspectorFileSystemAgent;
#endif
    InspectorApplicationCacheAgent* m_inspectorApplicationCacheAgent;
#if ENABLE(JAVASCRIPT_DEBUGGER)
    InspectorDebuggerAgent* m_inspectorDebuggerAgent;
    PageDebuggerAgent* m_pageDebuggerAgent;
    InspectorDOMDebuggerAgent* m_inspectorDOMDebuggerAgent;
    InspectorProfilerAgent* m_inspectorProfilerAgent;
    InspectorHeapProfilerAgent* m_inspectorHeapProfilerAgent;
#endif
#if ENABLE(WORKERS)
    InspectorWorkerAgent* m_inspectorWorkerAgent;
#endif
    InspectorCanvasAgent* m_inspectorCanvasAgent;
};

InstrumentingAgents* instrumentationForPage(Page*);
#if ENABLE(WORKERS)
InstrumentingAgents* instrumentationForWorkerGlobalScope(WorkerGlobalScope*);
#endif

}

#endif // !defined(InstrumentingAgents_h)
