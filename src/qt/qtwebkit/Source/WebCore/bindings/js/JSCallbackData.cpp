/*
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "JSCallbackData.h"

#include "Document.h"
#include "JSDOMBinding.h"
#include "JSMainThreadExecState.h"

using namespace JSC;
    
namespace WebCore {

void JSCallbackData::deleteData(void* context)
{
    delete static_cast<JSCallbackData*>(context);
}

JSValue JSCallbackData::invokeCallback(MarkedArgumentBuffer& args, bool* raisedException)
{
    ASSERT(callback());
    return invokeCallback(callback(), args, raisedException);
}

JSValue JSCallbackData::invokeCallback(JSValue thisValue, MarkedArgumentBuffer& args, bool* raisedException)
{
    ASSERT(callback());
    ASSERT(globalObject());

    ExecState* exec = globalObject()->globalExec();
    JSValue function = callback();

    CallData callData;
    CallType callType = callback()->methodTable()->getCallData(callback(), callData);
    if (callType == CallTypeNone) {
        function = callback()->get(exec, Identifier(exec, "handleEvent"));
        callType = getCallData(function, callData);
        if (callType == CallTypeNone)
            return JSValue();
    }

    ScriptExecutionContext* context = globalObject()->scriptExecutionContext();
    // We will fail to get the context if the frame has been detached.
    if (!context)
        return JSValue();

    InspectorInstrumentationCookie cookie = JSMainThreadExecState::instrumentFunctionCall(context, callType, callData);

    bool contextIsDocument = context->isDocument();
    JSValue result = contextIsDocument
        ? JSMainThreadExecState::call(exec, function, callType, callData, thisValue, args)
        : JSC::call(exec, function, callType, callData, thisValue, args);

    InspectorInstrumentation::didCallFunction(cookie);

    if (contextIsDocument)
        Document::updateStyleForAllDocuments();

    if (exec->hadException()) {
        reportCurrentException(exec);
        if (raisedException)
            *raisedException = true;
        return result;
    }

    return result;
}

} // namespace WebCore
