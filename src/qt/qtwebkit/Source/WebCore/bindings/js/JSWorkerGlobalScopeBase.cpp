/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All Rights Reserved.
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

#include "JSWorkerGlobalScopeBase.h"

#include "DOMWrapperWorld.h"
#include "JSDedicatedWorkerGlobalScope.h"
#include "JSWorkerGlobalScope.h"
#include "WorkerGlobalScope.h"

#if ENABLE(SHARED_WORKERS)
#include "JSSharedWorkerGlobalScope.h"
#endif

using namespace JSC;

namespace WebCore {

const ClassInfo JSWorkerGlobalScopeBase::s_info = { "WorkerGlobalScope", &JSDOMGlobalObject::s_info, 0, 0, CREATE_METHOD_TABLE(JSWorkerGlobalScopeBase) };

JSWorkerGlobalScopeBase::JSWorkerGlobalScopeBase(JSC::VM& vm, JSC::Structure* structure, PassRefPtr<WorkerGlobalScope> impl)
    : JSDOMGlobalObject(vm, structure, normalWorld(vm))
    , m_impl(impl)
{
}

void JSWorkerGlobalScopeBase::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    ASSERT(inherits(&s_info));
}

void JSWorkerGlobalScopeBase::destroy(JSCell* cell)
{
    static_cast<JSWorkerGlobalScopeBase*>(cell)->JSWorkerGlobalScopeBase::~JSWorkerGlobalScopeBase();
}

ScriptExecutionContext* JSWorkerGlobalScopeBase::scriptExecutionContext() const
{
    return m_impl.get();
}

JSValue toJS(ExecState* exec, JSDOMGlobalObject*, WorkerGlobalScope* workerGlobalScope)
{
    return toJS(exec, workerGlobalScope);
}

JSValue toJS(ExecState*, WorkerGlobalScope* workerGlobalScope)
{
    if (!workerGlobalScope)
        return jsNull();
    WorkerScriptController* script = workerGlobalScope->script();
    if (!script)
        return jsNull();
    JSWorkerGlobalScope* contextWrapper = script->workerGlobalScopeWrapper();
    ASSERT(contextWrapper);
    return contextWrapper;
}

JSDedicatedWorkerGlobalScope* toJSDedicatedWorkerGlobalScope(JSValue value)
{
    if (!value.isObject())
        return 0;
    const ClassInfo* classInfo = asObject(value)->classInfo();
    if (classInfo == &JSDedicatedWorkerGlobalScope::s_info)
        return jsCast<JSDedicatedWorkerGlobalScope*>(asObject(value));
    return 0;
}

#if ENABLE(SHARED_WORKERS)
JSSharedWorkerGlobalScope* toJSSharedWorkerGlobalScope(JSValue value)
{
    if (!value.isObject())
        return 0;
    const ClassInfo* classInfo = asObject(value)->classInfo();
    if (classInfo == &JSSharedWorkerGlobalScope::s_info)
        return jsCast<JSSharedWorkerGlobalScope*>(asObject(value));
    return 0;
}
#endif

JSWorkerGlobalScope* toJSWorkerGlobalScope(JSValue value)
{
    JSWorkerGlobalScope* context = toJSDedicatedWorkerGlobalScope(value);
#if ENABLE(SHARED_WORKERS)
    if (!context)
        context = toJSSharedWorkerGlobalScope(value);
#endif
    return context;
}

} // namespace WebCore

#endif // ENABLE(WORKERS)
