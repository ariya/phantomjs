/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2011 Google Inc. All Rights Reserved.
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

#include "config.h"

#if ENABLE(WORKERS)

#include "WorkerScriptController.h"

#include "JSDedicatedWorkerContext.h"
#include "JSSharedWorkerContext.h"
#include "ScriptSourceCode.h"
#include "ScriptValue.h"
#include "WebCoreJSClientData.h"
#include "WorkerContext.h"
#include "WorkerObjectProxy.h"
#include "WorkerThread.h"
#include <interpreter/Interpreter.h>
#include <runtime/Completion.h>
#include <runtime/Completion.h>
#include <runtime/Error.h>
#include <runtime/JSLock.h>

using namespace JSC;

namespace WebCore {

WorkerScriptController::WorkerScriptController(WorkerContext* workerContext)
    : m_globalData(JSGlobalData::create(ThreadStackTypeSmall))
    , m_workerContext(workerContext)
    , m_workerContextWrapper(*m_globalData)
    , m_executionForbidden(false)
{
    initNormalWorldClientData(m_globalData.get());
}

WorkerScriptController::~WorkerScriptController()
{
    m_workerContextWrapper.clear(); // Unprotect the global object.
    m_globalData->clearBuiltinStructures();
    m_globalData->heap.destroy();
}

void WorkerScriptController::initScript()
{
    ASSERT(!m_workerContextWrapper);

    JSLock lock(SilenceAssertionsOnly);

    // Explicitly protect the global object's prototype so it isn't collected
    // when we allocate the global object. (Once the global object is fully
    // constructed, it can mark its own prototype.)
    Structure* workerContextPrototypeStructure = JSWorkerContextPrototype::createStructure(*m_globalData, jsNull());
    Strong<JSWorkerContextPrototype> workerContextPrototype(*m_globalData, new (m_globalData.get()) JSWorkerContextPrototype(*m_globalData, 0, workerContextPrototypeStructure));

    if (m_workerContext->isDedicatedWorkerContext()) {
        Structure* dedicatedContextPrototypeStructure = JSDedicatedWorkerContextPrototype::createStructure(*m_globalData, workerContextPrototype.get());
        Strong<JSDedicatedWorkerContextPrototype> dedicatedContextPrototype(*m_globalData, new (m_globalData.get()) JSDedicatedWorkerContextPrototype(*m_globalData, 0, dedicatedContextPrototypeStructure));
        Structure* structure = JSDedicatedWorkerContext::createStructure(*m_globalData, dedicatedContextPrototype.get());

        m_workerContextWrapper.set(*m_globalData, new (m_globalData.get()) JSDedicatedWorkerContext(*m_globalData, structure, m_workerContext->toDedicatedWorkerContext()));
        workerContextPrototype->putAnonymousValue(*m_globalData, 0, m_workerContextWrapper.get());
        dedicatedContextPrototype->putAnonymousValue(*m_globalData, 0, m_workerContextWrapper.get());
#if ENABLE(SHARED_WORKERS)
    } else {
        ASSERT(m_workerContext->isSharedWorkerContext());
        Structure* sharedContextPrototypeStructure = JSSharedWorkerContextPrototype::createStructure(*m_globalData, workerContextPrototype.get());
        Strong<JSSharedWorkerContextPrototype> sharedContextPrototype(*m_globalData, new (m_globalData.get()) JSSharedWorkerContextPrototype(*m_globalData, 0, sharedContextPrototypeStructure));
        Structure* structure = JSSharedWorkerContext::createStructure(*m_globalData, sharedContextPrototype.get());

        m_workerContextWrapper.set(*m_globalData, new (m_globalData.get()) JSSharedWorkerContext(*m_globalData, structure, m_workerContext->toSharedWorkerContext()));
        workerContextPrototype->putAnonymousValue(*m_globalData, 0, m_workerContextWrapper.get());
        sharedContextPrototype->putAnonymousValue(*m_globalData, 0, m_workerContextWrapper.get());
#endif
    }
}

ScriptValue WorkerScriptController::evaluate(const ScriptSourceCode& sourceCode)
{
    if (isExecutionForbidden())
        return ScriptValue();

    ScriptValue exception;
    ScriptValue result(evaluate(sourceCode, &exception));
    if (exception.jsValue()) {
        JSLock lock(SilenceAssertionsOnly);
        reportException(m_workerContextWrapper->globalExec(), exception.jsValue());
    }
    return result;
}

ScriptValue WorkerScriptController::evaluate(const ScriptSourceCode& sourceCode, ScriptValue* exception)
{
    if (isExecutionForbidden())
        return ScriptValue();

    initScriptIfNeeded();
    JSLock lock(SilenceAssertionsOnly);

    ExecState* exec = m_workerContextWrapper->globalExec();
    m_workerContextWrapper->globalData().timeoutChecker.start();
    Completion comp = JSC::evaluate(exec, exec->dynamicGlobalObject()->globalScopeChain(), sourceCode.jsSourceCode(), m_workerContextWrapper.get());
    m_workerContextWrapper->globalData().timeoutChecker.stop();


    ComplType completionType = comp.complType();

    if (completionType == Terminated || m_workerContextWrapper->globalData().terminator.shouldTerminate()) {
        forbidExecution();
        return ScriptValue();
    }

    if (completionType == Normal || completionType == ReturnValue)
        return ScriptValue(*m_globalData, comp.value());

    if (completionType == Throw) {
        String errorMessage;
        int lineNumber = 0;
        String sourceURL = sourceCode.url().string();
        if (m_workerContext->sanitizeScriptError(errorMessage, lineNumber, sourceURL))
            *exception = ScriptValue(*m_globalData, throwError(exec, createError(exec, errorMessage.impl())));
        else
            *exception = ScriptValue(*m_globalData, comp.value());
    }
    return ScriptValue();
}

void WorkerScriptController::setException(ScriptValue exception)
{
    throwError(m_workerContextWrapper->globalExec(), exception.jsValue());
}

void WorkerScriptController::scheduleExecutionTermination()
{
    m_globalData->terminator.terminateSoon();
}

void WorkerScriptController::forbidExecution()
{
    ASSERT(m_workerContext->isContextThread());
    m_executionForbidden = true;
}

bool WorkerScriptController::isExecutionForbidden() const
{
    ASSERT(m_workerContext->isContextThread());
    return m_executionForbidden;
}

} // namespace WebCore

#endif // ENABLE(WORKERS)
