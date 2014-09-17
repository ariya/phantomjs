/*
 * Copyright (C) 2008, 2009, 2011 Apple Inc. All Rights Reserved.
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

#if ENABLE(WORKERS)

#include "JSWorkerContext.h"

#include "ExceptionCode.h"
#include "JSDOMBinding.h"
#include "JSDOMGlobalObject.h"
#include "JSEventListener.h"
#include "JSEventSource.h"
#include "JSMessageChannel.h"
#include "JSMessagePort.h"
#include "JSWorkerLocation.h"
#include "JSWorkerNavigator.h"
#include "JSXMLHttpRequest.h"
#include "ScheduledAction.h"
#include "WorkerContext.h"
#include "WorkerLocation.h"
#include "WorkerNavigator.h"
#include <interpreter/Interpreter.h>

#if ENABLE(WEB_SOCKETS)
#include "JSWebSocket.h"
#endif

using namespace JSC;

namespace WebCore {

void JSWorkerContext::visitChildren(SlotVisitor& visitor)
{
    Base::visitChildren(visitor);

    if (WorkerLocation* location = impl()->optionalLocation())
        visitor.addOpaqueRoot(location);
    if (WorkerNavigator* navigator = impl()->optionalNavigator())
        visitor.addOpaqueRoot(navigator);

    impl()->visitJSEventListeners(visitor);
}

bool JSWorkerContext::getOwnPropertySlotDelegate(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    // Look for overrides before looking at any of our own properties.
    if (JSGlobalObject::getOwnPropertySlot(exec, propertyName, slot))
        return true;
    return false;
}

bool JSWorkerContext::getOwnPropertyDescriptorDelegate(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    // Look for overrides before looking at any of our own properties.
    if (JSGlobalObject::getOwnPropertyDescriptor(exec, propertyName, descriptor))
        return true;
    return false;
}

#if ENABLE(EVENTSOURCE)
JSValue JSWorkerContext::eventSource(ExecState* exec) const
{
    return getDOMConstructor<JSEventSourceConstructor>(exec, this);
}
#endif

JSValue JSWorkerContext::xmlHttpRequest(ExecState* exec) const
{
    return getDOMConstructor<JSXMLHttpRequestConstructor>(exec, this);
}

#if ENABLE(WEB_SOCKETS)
JSValue JSWorkerContext::webSocket(ExecState* exec) const
{
    return getDOMConstructor<JSWebSocketConstructor>(exec, this);
}
#endif

JSValue JSWorkerContext::importScripts(ExecState* exec)
{
    if (!exec->argumentCount())
        return jsUndefined();

    Vector<String> urls;
    for (unsigned i = 0; i < exec->argumentCount(); i++) {
        urls.append(ustringToString(exec->argument(i).toString(exec)));
        if (exec->hadException())
            return jsUndefined();
    }
    ExceptionCode ec = 0;

    impl()->importScripts(urls, ec);
    setDOMException(exec, ec);
    return jsUndefined();
}

JSValue JSWorkerContext::setTimeout(ExecState* exec)
{
    // FIXME: Should we enforce a Content-Security-Policy on workers?
    OwnPtr<ScheduledAction> action = ScheduledAction::create(exec, currentWorld(exec), 0);
    if (exec->hadException())
        return jsUndefined();
    int delay = exec->argument(1).toInt32(exec);
    return jsNumber(impl()->setTimeout(action.release(), delay));
}

JSValue JSWorkerContext::setInterval(ExecState* exec)
{
    // FIXME: Should we enforce a Content-Security-Policy on workers?
    OwnPtr<ScheduledAction> action = ScheduledAction::create(exec, currentWorld(exec), 0);
    if (exec->hadException())
        return jsUndefined();
    int delay = exec->argument(1).toInt32(exec);
    return jsNumber(impl()->setInterval(action.release(), delay));
}


#if ENABLE(CHANNEL_MESSAGING)
JSValue JSWorkerContext::messageChannel(ExecState* exec) const
{
    return getDOMConstructor<JSMessageChannelConstructor>(exec, this);
}
#endif

} // namespace WebCore

#endif // ENABLE(WORKERS)
