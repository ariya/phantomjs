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

#if ENABLE(INSPECTOR) && ENABLE(WORKERS)

#include "WorkerRuntimeAgent.h"

#include "InjectedScript.h"
#include "InstrumentingAgents.h"
#include "ScriptState.h"
#include "WorkerDebuggerAgent.h"
#include "WorkerGlobalScope.h"
#include "WorkerRunLoop.h"
#include "WorkerThread.h"

namespace WebCore {

WorkerRuntimeAgent::WorkerRuntimeAgent(InstrumentingAgents* instrumentingAgents, InspectorCompositeState* state, InjectedScriptManager* injectedScriptManager, WorkerGlobalScope* workerGlobalScope)
    : InspectorRuntimeAgent(instrumentingAgents, state, injectedScriptManager)
    , m_workerGlobalScope(workerGlobalScope)
    , m_paused(false)
{
    m_instrumentingAgents->setWorkerRuntimeAgent(this);
}

WorkerRuntimeAgent::~WorkerRuntimeAgent()
{
    m_instrumentingAgents->setWorkerRuntimeAgent(0);
}

InjectedScript WorkerRuntimeAgent::injectedScriptForEval(ErrorString* error, const int* executionContextId)
{
    if (executionContextId) {
        *error = "Execution context id is not supported for workers as there is only one execution context.";
        return InjectedScript();
    }
    ScriptState* scriptState = scriptStateFromWorkerGlobalScope(m_workerGlobalScope);
    return injectedScriptManager()->injectedScriptFor(scriptState);
}

void WorkerRuntimeAgent::muteConsole()
{
    // We don't need to mute console for workers.
}

void WorkerRuntimeAgent::unmuteConsole()
{
    // We don't need to mute console for workers.
}

void WorkerRuntimeAgent::run(ErrorString*)
{
    m_paused = false;
}

#if ENABLE(JAVASCRIPT_DEBUGGER)
void WorkerRuntimeAgent::pauseWorkerGlobalScope(WorkerGlobalScope* context)
{
    m_paused = true;
    MessageQueueWaitResult result;
    do {
        result = context->thread()->runLoop().runInMode(context, WorkerDebuggerAgent::debuggerTaskMode);
    // Keep waiting until execution is resumed.
    } while (result == MessageQueueMessageReceived && m_paused);
}
#endif // ENABLE(JAVASCRIPT_DEBUGGER)

} // namespace WebCore

#endif // ENABLE(INSPECTOR) && ENABLE(WORKERS)
