/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
#include "JSEventTarget.h"

#include "EventTargetHeaders.h"
#include "EventTargetInterfaces.h"
#include "JSDOMWindowShell.h"
#include "JSEventListener.h"

using namespace JSC;

namespace WebCore {

#define TRY_TO_WRAP_WITH_INTERFACE(interfaceName) \
    if (eventNames().interfaceFor##interfaceName == desiredInterface) \
        return toJS(exec, globalObject, static_cast<interfaceName*>(target));

JSValue toJS(ExecState* exec, JSDOMGlobalObject* globalObject, EventTarget* target)
{
    if (!target)
        return jsNull();

    AtomicString desiredInterface = target->interfaceName();

    // FIXME: Why can't we use toJS for these cases?
#if ENABLE(WORKERS)
    if (eventNames().interfaceForDedicatedWorkerGlobalScope == desiredInterface)
        return toJSDOMGlobalObject(static_cast<DedicatedWorkerGlobalScope*>(target), exec);
#endif
#if ENABLE(SHARED_WORKERS)
    if (eventNames().interfaceForSharedWorkerGlobalScope == desiredInterface)
        return toJSDOMGlobalObject(static_cast<SharedWorkerGlobalScope*>(target), exec);
#endif

    DOM_EVENT_TARGET_INTERFACES_FOR_EACH(TRY_TO_WRAP_WITH_INTERFACE)

    ASSERT_NOT_REACHED();
    return jsNull();
}

#undef TRY_TO_WRAP_WITH_INTERFACE

#define TRY_TO_UNWRAP_WITH_INTERFACE(interfaceName) \
    if (value.inherits(&JS##interfaceName::s_info)) \
        return static_cast<interfaceName*>(jsCast<JS##interfaceName*>(asObject(value))->impl());

EventTarget* toEventTarget(JSC::JSValue value)
{
    if (value.inherits(&JSDOMWindowShell::s_info))
        return jsCast<JSDOMWindowShell*>(asObject(value))->impl();

    TRY_TO_UNWRAP_WITH_INTERFACE(EventTarget)
    // FIXME: Remove this once all event targets extend EventTarget
    DOM_EVENT_TARGET_INTERFACES_FOR_EACH(TRY_TO_UNWRAP_WITH_INTERFACE)
    return 0;
}

#undef TRY_TO_UNWRAP_WITH_INTERFACE

} // namespace WebCore
