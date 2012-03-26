/*
 * Copyright (C) 2008, 2009 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "JSMessagePort.h"

#include "Event.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "JSDOMGlobalObject.h"
#include "JSEvent.h"
#include "JSEventListener.h"
#include "JSMessagePortCustom.h"
#include "MessagePort.h"
#include <runtime/Error.h>
#include <wtf/text/AtomicString.h>

using namespace JSC;

namespace WebCore {

void JSMessagePort::visitChildren(SlotVisitor& visitor)
{
    Base::visitChildren(visitor);

    // If we have a locally entangled port, we can directly mark it as reachable. Ports that are remotely entangled are marked in-use by markActiveObjectsForContext().
    if (MessagePort* port = m_impl->locallyEntangledPort())
        visitor.addOpaqueRoot(port);

    m_impl->visitJSEventListeners(visitor);
}

JSC::JSValue JSMessagePort::postMessage(JSC::ExecState* exec)
{
    return handlePostMessage(exec, impl());
}

void fillMessagePortArray(JSC::ExecState* exec, JSC::JSValue value, MessagePortArray& portArray)
{
    // Convert from the passed-in JS array-like object to a MessagePortArray.
    // Also validates the elements per sections 4.1.13 and 4.1.15 of the WebIDL spec and section 8.3.3 of the HTML5 spec.
    if (value.isUndefinedOrNull()) {
        portArray.resize(0);
        return;
    }

    // Validation of sequence types, per WebIDL spec 4.1.13.
    unsigned length;
    JSObject* object = toJSSequence(exec, value, length);
    if (exec->hadException())
        return;

    for (unsigned i = 0 ; i < length; ++i) {
        JSValue value = object->get(exec, i);
        if (exec->hadException())
            return;
        // Validation of non-null objects, per HTML5 spec 8.3.3.
        if (value.isUndefinedOrNull()) {
            setDOMException(exec, INVALID_STATE_ERR);
            return;
        }

        // Validation of Objects implementing an interface, per WebIDL spec 4.1.15.
        RefPtr<MessagePort> port = toMessagePort(value);
        if (!port) {
            throwTypeError(exec);
            return;
        }
        portArray.append(port.release());
    }
}

} // namespace WebCore
