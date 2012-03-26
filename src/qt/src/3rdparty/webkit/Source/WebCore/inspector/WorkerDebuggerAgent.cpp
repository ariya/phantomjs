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
#include "WorkerDebuggerAgent.h"

#if ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(INSPECTOR) && ENABLE(WORKERS)
#include "ScriptDebugServer.h"
#include "WorkerContext.h"

namespace WebCore {

PassOwnPtr<WorkerDebuggerAgent> WorkerDebuggerAgent::create(InstrumentingAgents* instrumentingAgents, InspectorState* inspectorState, WorkerContext* inspectedWorkerContext, InjectedScriptManager* injectedScriptManager)
{
    return adoptPtr(new WorkerDebuggerAgent(instrumentingAgents, inspectorState, inspectedWorkerContext, injectedScriptManager));
}

WorkerDebuggerAgent::WorkerDebuggerAgent(InstrumentingAgents* instrumentingAgents, InspectorState* inspectorState, WorkerContext* inspectedWorkerContext, InjectedScriptManager* injectedScriptManager)
    : InspectorDebuggerAgent(instrumentingAgents, inspectorState, injectedScriptManager)
    , m_inspectedWorkerContext(inspectedWorkerContext)
{
}

WorkerDebuggerAgent::~WorkerDebuggerAgent()
{
}

void WorkerDebuggerAgent::startListeningScriptDebugServer()
{
    scriptDebugServer().addListener(this, m_inspectedWorkerContext);
}

void WorkerDebuggerAgent::stopListeningScriptDebugServer()
{
    scriptDebugServer().removeListener(this, m_inspectedWorkerContext);
}

WorkerScriptDebugServer& WorkerDebuggerAgent::scriptDebugServer()
{
    return m_scriptDebugServer;
}

} // namespace WebCore

#endif // ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(INSPECTOR) && ENABLE(WORKERS)
