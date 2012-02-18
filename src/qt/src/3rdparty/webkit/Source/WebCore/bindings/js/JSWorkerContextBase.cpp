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

#include "JSWorkerContextBase.h"

#include "DOMWrapperWorld.h"
#include "JSDedicatedWorkerContext.h"
#include "JSSharedWorkerContext.h"
#include "JSWorkerContext.h"
#include "WorkerContext.h"

using namespace JSC;

namespace WebCore {

ASSERT_CLASS_FITS_IN_CELL(JSWorkerContextBase);

const ClassInfo JSWorkerContextBase::s_info = { "WorkerContext", &JSDOMGlobalObject::s_info, 0, 0 };

JSWorkerContextBase::JSWorkerContextBase(JSC::JSGlobalData& globalData, JSC::Structure* structure, PassRefPtr<WorkerContext> impl)
    : JSDOMGlobalObject(globalData, structure, normalWorld(globalData), this)
    , m_impl(impl)
{
    ASSERT(inherits(&s_info));
}

JSWorkerContextBase::~JSWorkerContextBase()
{
}

ScriptExecutionContext* JSWorkerContextBase::scriptExecutionContext() const
{
    return m_impl.get();
}

JSValue toJS(ExecState* exec, JSDOMGlobalObject*, WorkerContext* workerContext)
{
    return toJS(exec, workerContext);
}

JSValue toJS(ExecState*, WorkerContext* workerContext)
{
    if (!workerContext)
        return jsNull();
    WorkerScriptController* script = workerContext->script();
    if (!script)
        return jsNull();
    JSWorkerContext* contextWrapper = script->workerContextWrapper();
    ASSERT(contextWrapper);
    return contextWrapper;
}

JSDedicatedWorkerContext* toJSDedicatedWorkerContext(JSValue value)
{
    if (!value.isObject())
        return 0;
    const ClassInfo* classInfo = asObject(value)->classInfo();
    if (classInfo == &JSDedicatedWorkerContext::s_info)
        return static_cast<JSDedicatedWorkerContext*>(asObject(value));
    return 0;
}

#if ENABLE(SHARED_WORKERS)
JSSharedWorkerContext* toJSSharedWorkerContext(JSValue value)
{
    if (!value.isObject())
        return 0;
    const ClassInfo* classInfo = asObject(value)->classInfo();
    if (classInfo == &JSSharedWorkerContext::s_info)
        return static_cast<JSSharedWorkerContext*>(asObject(value));
    return 0;
}
#endif

JSWorkerContext* toJSWorkerContext(JSValue value)
{
    JSWorkerContext* context = toJSDedicatedWorkerContext(value);
#if ENABLE(SHARED_WORKERS)
    if (!context)
        context = toJSSharedWorkerContext(value);
#endif
    return context;
}

} // namespace WebCore

#endif // ENABLE(WORKERS)
