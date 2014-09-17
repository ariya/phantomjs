/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 *
 */

#ifndef WorkerScriptController_h
#define WorkerScriptController_h

#if ENABLE(WORKERS)

#include <heap/Strong.h>
#include <wtf/Forward.h>
#include <wtf/Threading.h>

namespace JSC {
    class JSGlobalData;
}

namespace WebCore {

    class JSWorkerContext;
    class ScriptSourceCode;
    class ScriptValue;
    class WorkerContext;

    class WorkerScriptController {
        WTF_MAKE_NONCOPYABLE(WorkerScriptController); WTF_MAKE_FAST_ALLOCATED;
    public:
        WorkerScriptController(WorkerContext*);
        ~WorkerScriptController();

        JSWorkerContext* workerContextWrapper()
        {
            initScriptIfNeeded();
            return m_workerContextWrapper.get();
        }

        ScriptValue evaluate(const ScriptSourceCode&);
        ScriptValue evaluate(const ScriptSourceCode&, ScriptValue* exception);

        void setException(ScriptValue);

        // Async request to terminate a JS run execution. Eventually causes termination
        // exception raised during JS execution, if the worker thread happens to run JS.
        // After JS execution was terminated in this way, the Worker thread has to use
        // forbidExecution()/isExecutionForbidden() to guard against reentry into JS.
        // Can be called from any thread.
        void scheduleExecutionTermination();

        // Called on Worker thread when JS exits with termination exception caused by forbidExecution() request,
        // or by Worker thread termination code to prevent future entry into JS.
        void forbidExecution();
        bool isExecutionForbidden() const;

        JSC::JSGlobalData* globalData() { return m_globalData.get(); }

    private:
        void initScriptIfNeeded()
        {
            if (!m_workerContextWrapper)
                initScript();
        }
        void initScript();

        RefPtr<JSC::JSGlobalData> m_globalData;
        WorkerContext* m_workerContext;
        JSC::Strong<JSWorkerContext> m_workerContextWrapper;
        bool m_executionForbidden;
    };

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // WorkerScriptController_h
