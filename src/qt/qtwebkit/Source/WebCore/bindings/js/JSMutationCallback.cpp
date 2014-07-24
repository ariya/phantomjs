/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "config.h"
#include "JSMutationCallback.h"

#include "JSDOMGlobalObject.h"
#include "JSMainThreadExecState.h"
#include "JSMutationObserver.h"
#include "JSMutationRecord.h"
#include "ScriptExecutionContext.h"
#include <heap/WeakInlines.h>
#include <runtime/JSLock.h>

using namespace JSC;

namespace WebCore {

JSMutationCallback::JSMutationCallback(JSObject* callback, JSDOMGlobalObject* globalObject)
    : ActiveDOMCallback(globalObject->scriptExecutionContext())
    , m_callback(PassWeak<JSObject>(callback))
    , m_isolatedWorld(globalObject->world())
{
}

JSMutationCallback::~JSMutationCallback()
{
}

void JSMutationCallback::call(const Vector<RefPtr<MutationRecord> >& mutations, MutationObserver* observer)
{
    if (!canInvokeCallback())
        return;

    RefPtr<JSMutationCallback> protect(this);

    JSLockHolder lock(m_isolatedWorld->vm());

    if (!m_callback)
        return;

    JSValue callback = m_callback.get();
    CallData callData;
    CallType callType = getCallData(callback, callData);
    if (callType == CallTypeNone) {
        ASSERT_NOT_REACHED();
        return;
    }

    ScriptExecutionContext* context = scriptExecutionContext();
    if (!context)
        return;
    ASSERT(context->isDocument());

    JSDOMGlobalObject* globalObject = toJSDOMGlobalObject(context, m_isolatedWorld.get());
    ExecState* exec = globalObject->globalExec();

    JSValue jsObserver = toJS(exec, globalObject, observer);

    MarkedArgumentBuffer args;
    args.append(jsArray(exec, globalObject, mutations));
    args.append(jsObserver);

    InspectorInstrumentationCookie cookie = JSMainThreadExecState::instrumentFunctionCall(context, callType, callData);

    JSMainThreadExecState::call(exec, callback, callType, callData, jsObserver, args);

    InspectorInstrumentation::didCallFunction(cookie);

    if (exec->hadException())
        reportCurrentException(exec);
}

} // namespace WebCore
