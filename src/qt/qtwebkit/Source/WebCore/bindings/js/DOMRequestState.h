/*
 * Copyright (C) 2012 Michael Pruett <michael@68k.org>
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

#ifndef DOMRequestState_h
#define DOMRequestState_h

#include "DOMWrapperWorld.h"
#include "Document.h"
#include "ScriptState.h"
#if ENABLE(WORKERS)
#include "WorkerGlobalScope.h"
#endif

#include <JavaScriptCore/APIShims.h>

namespace WebCore {

class ScriptExecutionContext;

class DOMRequestState {
public:
    explicit DOMRequestState(ScriptExecutionContext* scriptExecutionContext)
        : m_scriptExecutionContext(scriptExecutionContext)
        , m_exec(0)
    {
        if (m_scriptExecutionContext->isDocument()) {
            Document* document = toDocument(m_scriptExecutionContext);
            m_exec = scriptStateFromPage(mainThreadNormalWorld(), document->page());
        } else {
#if ENABLE(WORKERS)
            WorkerGlobalScope* workerGlobalScope = static_cast<WorkerGlobalScope*>(m_scriptExecutionContext);
            m_exec = scriptStateFromWorkerGlobalScope(workerGlobalScope);
#endif
        }
    }

    void clear()
    {
        m_scriptExecutionContext = 0;
        m_exec = 0;
    }

    class Scope {
    public:
        explicit Scope(DOMRequestState& state)
            : m_entryShim(state.exec())
        {
        }
    private:
        JSC::APIEntryShim m_entryShim;
    };

    JSC::ExecState* exec()
    {
        return m_exec;
    }

private:
    ScriptExecutionContext* m_scriptExecutionContext;
    JSC::ExecState* m_exec;
};

}
#endif
