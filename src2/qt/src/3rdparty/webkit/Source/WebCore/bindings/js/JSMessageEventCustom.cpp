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

#include "JSDOMBinding.h"
#include "JSDOMWindow.h"
#include "JSEventTarget.h"
#include "JSMessagePortCustom.h"
#include "MessageEvent.h"
#include <runtime/JSArray.h>

using namespace JSC;

namespace WebCore {

JSValue JSMessageEvent::ports(ExecState* exec) const
{
    MessagePortArray* ports = static_cast<MessageEvent*>(impl())->ports();
    if (!ports || ports->isEmpty())
        return jsNull();

    MarkedArgumentBuffer list;
    for (size_t i = 0; i < ports->size(); i++)
        list.append(toJS(exec, globalObject(), (*ports)[i].get()));
    return constructArray(exec, globalObject(), list);
}

JSC::JSValue JSMessageEvent::initMessageEvent(JSC::ExecState* exec)
{
    const UString& typeArg = exec->argument(0).toString(exec);
    bool canBubbleArg = exec->argument(1).toBoolean(exec);
    bool cancelableArg = exec->argument(2).toBoolean(exec);
    PassRefPtr<SerializedScriptValue> dataArg = SerializedScriptValue::create(exec, exec->argument(3));
    if (exec->hadException())
        return jsUndefined();
    const UString& originArg = exec->argument(4).toString(exec);
    const UString& lastEventIdArg = exec->argument(5).toString(exec);
    DOMWindow* sourceArg = toDOMWindow(exec->argument(6));
    OwnPtr<MessagePortArray> messagePorts;
    if (!exec->argument(7).isUndefinedOrNull()) {
        messagePorts = adoptPtr(new MessagePortArray);
        fillMessagePortArray(exec, exec->argument(7), *messagePorts);
        if (exec->hadException())
            return jsUndefined();
    }

    MessageEvent* event = static_cast<MessageEvent*>(this->impl());
    event->initMessageEvent(ustringToAtomicString(typeArg), canBubbleArg, cancelableArg, dataArg, ustringToString(originArg), ustringToString(lastEventIdArg), sourceArg, messagePorts.release());
    return jsUndefined();
}

} // namespace WebCore
