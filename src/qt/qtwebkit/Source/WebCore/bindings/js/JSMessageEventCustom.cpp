/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
#include "JSMessageEvent.h"

#include "JSArrayBuffer.h"
#include "JSBlob.h"
#include "JSDOMBinding.h"
#include "JSDOMWindow.h"
#include "JSEventTarget.h"
#include "JSMessagePortCustom.h"
#include "MessageEvent.h"
#include <runtime/JSArray.h>

using namespace JSC;

namespace WebCore {

JSValue JSMessageEvent::data(ExecState* exec) const
{
    if (JSValue cachedValue = m_data.get())
        return cachedValue;

    MessageEvent* event = static_cast<MessageEvent*>(impl());
    JSValue result;
    switch (event->dataType()) {
    case MessageEvent::DataTypeScriptValue: {
        ScriptValue scriptValue = event->dataAsScriptValue();
        if (scriptValue.hasNoValue())
            result = jsNull();
        else
            result = scriptValue.jsValue();
        break;
    }

    case MessageEvent::DataTypeSerializedScriptValue:
        if (RefPtr<SerializedScriptValue> serializedValue = event->dataAsSerializedScriptValue()) {
            MessagePortArray ports = static_cast<MessageEvent*>(impl())->ports();
            result = serializedValue->deserialize(exec, globalObject(), &ports, NonThrowing);
        }
        else
            result = jsNull();
        break;

    case MessageEvent::DataTypeString:
        result = jsStringWithCache(exec, event->dataAsString());
        break;

    case MessageEvent::DataTypeBlob:
        result = toJS(exec, globalObject(), event->dataAsBlob());
        break;

    case MessageEvent::DataTypeArrayBuffer:
        result = toJS(exec, globalObject(), event->dataAsArrayBuffer());
        break;
    }

    // Save the result so we don't have to deserialize the value again.
    const_cast<JSMessageEvent*>(this)->m_data.set(exec->vm(), this, result);
    return result;
}

static JSC::JSValue handleInitMessageEvent(JSMessageEvent* jsEvent, JSC::ExecState* exec)
{
    const String& typeArg = exec->argument(0).toString(exec)->value(exec);
    bool canBubbleArg = exec->argument(1).toBoolean(exec);
    bool cancelableArg = exec->argument(2).toBoolean(exec);
    const String originArg = exec->argument(4).toString(exec)->value(exec);
    const String lastEventIdArg = exec->argument(5).toString(exec)->value(exec);
    DOMWindow* sourceArg = toDOMWindow(exec->argument(6));
    OwnPtr<MessagePortArray> messagePorts;
    OwnPtr<ArrayBufferArray> arrayBuffers;
    if (!exec->argument(7).isUndefinedOrNull()) {
        messagePorts = adoptPtr(new MessagePortArray);
        arrayBuffers = adoptPtr(new ArrayBufferArray);
        fillMessagePortArray(exec, exec->argument(7), *messagePorts, *arrayBuffers);
        if (exec->hadException())
            return jsUndefined();
    }
    ScriptValue dataArg = ScriptValue(exec->vm(), exec->argument(3));
    if (exec->hadException())
        return jsUndefined();

    MessageEvent* event = static_cast<MessageEvent*>(jsEvent->impl());
    event->initMessageEvent(typeArg, canBubbleArg, cancelableArg, dataArg, originArg, lastEventIdArg, sourceArg, messagePorts.release());
    jsEvent->m_data.set(exec->vm(), jsEvent, dataArg.jsValue());
    return jsUndefined();
}

JSC::JSValue JSMessageEvent::initMessageEvent(JSC::ExecState* exec)
{
    return handleInitMessageEvent(this, exec);
}

JSC::JSValue JSMessageEvent::webkitInitMessageEvent(JSC::ExecState* exec)
{
    return handleInitMessageEvent(this, exec);
}

} // namespace WebCore
