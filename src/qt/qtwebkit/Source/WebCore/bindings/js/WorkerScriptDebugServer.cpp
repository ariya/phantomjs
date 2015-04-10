/*
 * Copyright (c) 2011 Google Inc. All rights reserved.
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

#if ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(WORKERS)
#include "WorkerScriptDebugServer.h"

#include "WorkerDebuggerAgent.h"
#include "WorkerGlobalScope.h"
#include "WorkerRunLoop.h"
#include "WorkerThread.h"
#include <runtime/VM.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

WorkerScriptDebugServer::WorkerScriptDebugServer(WorkerGlobalScope* context, const String& mode)
    : ScriptDebugServer()
    , m_workerGlobalScope(context)
    , m_debuggerTaskMode(mode)
{
}

void WorkerScriptDebugServer::addListener(ScriptDebugListener* listener)
{
    if (!listener)
        return;

    if (m_listeners.isEmpty())
        m_workerGlobalScope->script()->attachDebugger(this);
    m_listeners.add(listener);
    recompileAllJSFunctions(0);
}

void WorkerScriptDebugServer::willExecuteProgram(const JSC::DebuggerCallFrame& debuggerCallFrame, intptr_t sourceID, int lineNumber, int columnNumber)
{
    if (!m_paused)
        createCallFrame(debuggerCallFrame, sourceID, lineNumber, columnNumber);
}

void WorkerScriptDebugServer::recompileAllJSFunctions(Timer<ScriptDebugServer>*)
{
    JSC::VM* vm = m_workerGlobalScope->script()->vm();

    JSC::JSLockHolder lock(vm);
    // If JavaScript stack is not empty postpone recompilation.
    if (vm->dynamicGlobalObject)
        recompileAllJSFunctionsSoon();
    else
        JSC::Debugger::recompileAllJSFunctions(vm);
}

void WorkerScriptDebugServer::removeListener(ScriptDebugListener* listener)
{
    if (!listener)
        return;

    m_listeners.remove(listener);
    if (m_listeners.isEmpty())
        m_workerGlobalScope->script()->detachDebugger(this);
}

void WorkerScriptDebugServer::runEventLoopWhilePaused()
{
    MessageQueueWaitResult result;
    do {
        result = m_workerGlobalScope->thread()->runLoop().runInMode(m_workerGlobalScope, m_debuggerTaskMode);
    // Keep waiting until execution is resumed.
    } while (result != MessageQueueTerminated && !m_doneProcessingDebuggerEvents);
}

void WorkerScriptDebugServer::interruptAndRunTask(PassOwnPtr<ScriptDebugServer::Task>)
{
}

} // namespace WebCore

#endif // ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(WORKERS)
