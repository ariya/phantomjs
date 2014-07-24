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

#include "config.h"

#if ENABLE(INSPECTOR)

#include "InstrumentingAgents.h"

#include "InspectorController.h"
#include "Page.h"
#include "WorkerGlobalScope.h"
#include "WorkerInspectorController.h"
#include <wtf/MainThread.h>

namespace WebCore {

InstrumentingAgents::InstrumentingAgents()
    : m_inspectorAgent(0)
    , m_inspectorPageAgent(0)
    , m_inspectorCSSAgent(0)
#if USE(ACCELERATED_COMPOSITING)
    , m_inspectorLayerTreeAgent(0)
#endif
    , m_inspectorConsoleAgent(0)
    , m_inspectorDOMAgent(0)
    , m_inspectorResourceAgent(0)
    , m_pageRuntimeAgent(0)
    , m_workerRuntimeAgent(0)
    , m_inspectorTimelineAgent(0)
    , m_inspectorDOMStorageAgent(0)
#if ENABLE(SQL_DATABASE)
    , m_inspectorDatabaseAgent(0)
#endif
#if ENABLE(FILE_SYSTEM)
    , m_inspectorFileSystemAgent(0)
#endif
    , m_inspectorApplicationCacheAgent(0)
#if ENABLE(JAVASCRIPT_DEBUGGER)
    , m_inspectorDebuggerAgent(0)
    , m_pageDebuggerAgent(0)
    , m_inspectorDOMDebuggerAgent(0)
    , m_inspectorProfilerAgent(0)
#endif
#if ENABLE(WORKERS)
    , m_inspectorWorkerAgent(0)
#endif
    , m_inspectorCanvasAgent(0)
{
}

void InstrumentingAgents::reset()
{
    m_inspectorAgent = 0;
    m_inspectorPageAgent = 0;
    m_inspectorCSSAgent = 0;
#if USE(ACCELERATED_COMPOSITING)
    m_inspectorLayerTreeAgent = 0;
#endif
    m_inspectorConsoleAgent = 0;
    m_inspectorDOMAgent = 0;
    m_inspectorResourceAgent = 0;
    m_pageRuntimeAgent = 0;
    m_workerRuntimeAgent = 0;
    m_inspectorTimelineAgent = 0;
    m_inspectorDOMStorageAgent = 0;
#if ENABLE(SQL_DATABASE)
    m_inspectorDatabaseAgent = 0;
#endif
#if ENABLE(FILE_SYSTEM)
    m_inspectorFileSystemAgent = 0;
#endif
    m_inspectorApplicationCacheAgent = 0;
#if ENABLE(JAVASCRIPT_DEBUGGER)
    m_inspectorDebuggerAgent = 0;
    m_pageDebuggerAgent = 0;
    m_inspectorDOMDebuggerAgent = 0;
    m_inspectorProfilerAgent = 0;
#endif
#if ENABLE(WORKERS)
    m_inspectorWorkerAgent = 0;
#endif
    m_inspectorCanvasAgent = 0;
}

InstrumentingAgents* instrumentationForPage(Page* page)
{
    ASSERT(isMainThread());
    if (InspectorController* controller = page->inspectorController())
        return controller->m_instrumentingAgents.get();
    return 0;
}

#if ENABLE(WORKERS)
InstrumentingAgents* instrumentationForWorkerGlobalScope(WorkerGlobalScope* workerGlobalScope)
{
    if (WorkerInspectorController* controller = workerGlobalScope->workerInspectorController())
        return controller->m_instrumentingAgents.get();
    return 0;
}
#endif

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
