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

#if ENABLE(SHARED_WORKERS)

#include "SharedWorkerGlobalScope.h"

#include "DOMWindow.h"
#include "EventNames.h"
#include "MessageEvent.h"
#include "NotImplemented.h"
#include "ScriptCallStack.h"
#include "SecurityOrigin.h"
#include "SharedWorkerThread.h"

namespace WebCore {

PassRefPtr<MessageEvent> createConnectEvent(PassRefPtr<MessagePort> port)
{
    RefPtr<MessageEvent> event = MessageEvent::create(adoptPtr(new MessagePortArray(1, port)));
    event->initEvent(eventNames().connectEvent, false, false);
    return event.release();
}

// static
PassRefPtr<SharedWorkerGlobalScope> SharedWorkerGlobalScope::create(const String& name, const KURL& url, const String& userAgent, PassOwnPtr<GroupSettings> settings, SharedWorkerThread* thread, const String& contentSecurityPolicy, ContentSecurityPolicy::HeaderType contentSecurityPolicyType)
{
    RefPtr<SharedWorkerGlobalScope> context = adoptRef(new SharedWorkerGlobalScope(name, url, userAgent, settings, thread));
    context->applyContentSecurityPolicyFromString(contentSecurityPolicy, contentSecurityPolicyType);
    return context.release();
}

SharedWorkerGlobalScope::SharedWorkerGlobalScope(const String& name, const KURL& url, const String& userAgent, PassOwnPtr<GroupSettings> settings, SharedWorkerThread* thread)
    : WorkerGlobalScope(url, userAgent, settings, thread, 0)
    , m_name(name)
{
}

SharedWorkerGlobalScope::~SharedWorkerGlobalScope()
{
}

const AtomicString& SharedWorkerGlobalScope::interfaceName() const
{
    return eventNames().interfaceForSharedWorkerGlobalScope;
}

SharedWorkerThread* SharedWorkerGlobalScope::thread()
{
    return static_cast<SharedWorkerThread*>(Base::thread());
}

void SharedWorkerGlobalScope::logExceptionToConsole(const String& errorMessage, const String& sourceURL, int lineNumber, int columnNumber, PassRefPtr<ScriptCallStack> callStack)
{
    WorkerGlobalScope::logExceptionToConsole(errorMessage, sourceURL, lineNumber, columnNumber, callStack);
    addMessageToWorkerConsole(JSMessageSource, ErrorMessageLevel, errorMessage, sourceURL, lineNumber, columnNumber, callStack);
}

} // namespace WebCore

#endif // ENABLE(SHARED_WORKERS)
