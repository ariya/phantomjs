/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2011, 2012 Google Inc. All Rights Reserved.
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

#include "JSDOMBinding.h"
#include "JSDedicatedWorkerGlobalScope.h"
#include "ScriptSourceCode.h"
#include "ScriptValue.h"
#include "WebCoreJSClientData.h"
#include "WorkerGlobalScope.h"
#include "WorkerObjectProxy.h"
#include "WorkerScriptDebugServer.h"
#include "WorkerThread.h"
#include <heap/StrongInlines.h>
#include <interpreter/Interpreter.h>
#include <runtime/Completion.h>
#include <runtime/ExceptionHelpers.h>
#include <runtime/Error.h>
#include <runtime/JSLock.h>

#if ENABLE(SHARED_WORKERS)
#include "JSSharedWorkerGlobalScope.h"
#endif

using namespace JSC;

namespace WebCore {

WorkerScriptController::WorkerScriptController(WorkerGlobalScope* workerGlobalScope)
    : m_vm(VM::create())
    , m_workerGlobalScope(workerGlobalScope)
    , m_workerGlobalScopeWrapper(*m_vm)
    , m_executionForbidden(false)
{
    initNormalWorldClientData(m_vm.get());
}

WorkerScriptController::~WorkerScriptController()
{
    JSLockHolder lock(vm());
    m_workerGlobalScopeWrapper.clear();
    m_vm.clear();
}

void WorkerScriptController::initScript()
{
    ASSERT(!m_workerGlobalScopeWrapper);

    JSLockHolder lock(m_vm.get());

    // Explicitly protect the global object's prototype so it isn't collected
    // when we allocate the global object. (Once the global object is fully
    // constructed, it can mark its own prototype.)
    Structure* workerGlobalScopePrototypeStructure = JSWorkerGlobalScopePrototype::createStructure(*m_vm, 0, jsNull());
    Strong<JSWorkerGlobalScopePrototype> workerGlobalScopePrototype(*m_vm, JSWorkerGlobalScopePrototype::create(*m_vm, 0, workerGlobalScopePrototypeStructure));

    if (m_workerGlobalScope->isDedicatedWorkerGlobalScope()) {
        Structure* dedicatedContextPrototypeStructure = JSDedicatedWorkerGlobalScopePrototype::createStructure(*m_vm, 0, workerGlobalScopePrototype.get());
        Strong<JSDedicatedWorkerGlobalScopePrototype> dedicatedContextPrototype(*m_vm, JSDedicatedWorkerGlobalScopePrototype::create(*m_vm, 0, dedicatedContextPrototypeStructure));
        Structure* structure = JSDedicatedWorkerGlobalScope::createStructure(*m_vm, 0, dedicatedContextPrototype.get());

        m_workerGlobalScopeWrapper.set(*m_vm, JSDedicatedWorkerGlobalScope::create(*m_vm, structure, static_cast<DedicatedWorkerGlobalScope*>(m_workerGlobalScope)));
        workerGlobalScopePrototypeStructure->setGlobalObject(*m_vm, m_workerGlobalScopeWrapper.get());
        dedicatedContextPrototypeStructure->setGlobalObject(*m_vm, m_workerGlobalScopeWrapper.get());
        ASSERT(structure->globalObject() == m_workerGlobalScopeWrapper);
        ASSERT(m_workerGlobalScopeWrapper->structure()->globalObject() == m_workerGlobalScopeWrapper);
        workerGlobalScopePrototype->structure()->setGlobalObject(*m_vm, m_workerGlobalScopeWrapper.get());
        dedicatedContextPrototype->structure()->setGlobalObject(*m_vm, m_workerGlobalScopeWrapper.get());
#if ENABLE(SHARED_WORKERS)
    } else {
        ASSERT(m_workerGlobalScope->isSharedWorkerGlobalScope());
        Structure* sharedContextPrototypeStructure = JSSharedWorkerGlobalScopePrototype::createStructure(*m_vm, 0, workerGlobalScopePrototype.get());
        Strong<JSSharedWorkerGlobalScopePrototype> sharedContextPrototype(*m_vm, JSSharedWorkerGlobalScopePrototype::create(*m_vm, 0, sharedContextPrototypeStructure));
        Structure* structure = JSSharedWorkerGlobalScope::createStructure(*m_vm, 0, sharedContextPrototype.get());

        m_workerGlobalScopeWrapper.set(*m_vm, JSSharedWorkerGlobalScope::create(*m_vm, structure, static_cast<SharedWorkerGlobalScope*>(m_workerGlobalScope)));
        workerGlobalScopePrototype->structure()->setGlobalObject(*m_vm, m_workerGlobalScopeWrapper.get());
        sharedContextPrototype->structure()->setGlobalObject(*m_vm, m_workerGlobalScopeWrapper.get());
#endif
    }
    ASSERT(m_workerGlobalScopeWrapper->globalObject() == m_workerGlobalScopeWrapper);
    ASSERT(asObject(m_workerGlobalScopeWrapper->prototype())->globalObject() == m_workerGlobalScopeWrapper);
}

void WorkerScriptController::evaluate(const ScriptSourceCode& sourceCode)
{
    if (isExecutionForbidden())
        return;

    ScriptValue exception;
    evaluate(sourceCode, &exception);
    if (exception.jsValue()) {
        JSLockHolder lock(vm());
        reportException(m_workerGlobalScopeWrapper->globalExec(), exception.jsValue());
    }
}

void WorkerScriptController::evaluate(const ScriptSourceCode& sourceCode, ScriptValue* exception)
{
    if (isExecutionForbidden())
        return;

    initScriptIfNeeded();

    ExecState* exec = m_workerGlobalScopeWrapper->globalExec();
    JSLockHolder lock(exec);

    JSValue evaluationException;
    JSC::evaluate(exec, sourceCode.jsSourceCode(), m_workerGlobalScopeWrapper.get(), &evaluationException);

    if ((evaluationException && isTerminatedExecutionException(evaluationException)) ||  m_workerGlobalScopeWrapper->vm().watchdog.didFire()) {
        forbidExecution();
        return;
    }

    if (evaluationException) {
        String errorMessage;
        int lineNumber = 0;
        int columnNumber = 0;
        String sourceURL = sourceCode.url().string();
        if (m_workerGlobalScope->sanitizeScriptError(errorMessage, lineNumber, columnNumber, sourceURL, sourceCode.cachedScript()))
            *exception = ScriptValue(*m_vm, throwError(exec, createError(exec, errorMessage.impl())));
        else
            *exception = ScriptValue(*m_vm, evaluationException);
    }
}

void WorkerScriptController::setException(const ScriptValue& exception)
{
    throwError(m_workerGlobalScopeWrapper->globalExec(), exception.jsValue());
}

void WorkerScriptController::scheduleExecutionTermination()
{
    // The mutex provides a memory barrier to ensure that once
    // termination is scheduled, isExecutionTerminating will
    // accurately reflect that state when called from another thread.
    MutexLocker locker(m_scheduledTerminationMutex);
    m_vm->watchdog.fire();
}

bool WorkerScriptController::isExecutionTerminating() const
{
    // See comments in scheduleExecutionTermination regarding mutex usage.
    MutexLocker locker(m_scheduledTerminationMutex);
    return m_vm->watchdog.didFire();
}

void WorkerScriptController::forbidExecution()
{
    ASSERT(m_workerGlobalScope->isContextThread());
    m_executionForbidden = true;
}

bool WorkerScriptController::isExecutionForbidden() const
{
    ASSERT(m_workerGlobalScope->isContextThread());
    return m_executionForbidden;
}

void WorkerScriptController::disableEval(const String& errorMessage)
{
    initScriptIfNeeded();
    JSLockHolder lock(vm());

    m_workerGlobalScopeWrapper->setEvalEnabled(false, errorMessage);
}

void WorkerScriptController::attachDebugger(JSC::Debugger* debugger)
{
    initScriptIfNeeded();
    debugger->attach(m_workerGlobalScopeWrapper->globalObject());
}

void WorkerScriptController::detachDebugger(JSC::Debugger* debugger)
{
    debugger->detach(m_workerGlobalScopeWrapper->globalObject());
}

} // namespace WebCore

#endif // ENABLE(WORKERS)
