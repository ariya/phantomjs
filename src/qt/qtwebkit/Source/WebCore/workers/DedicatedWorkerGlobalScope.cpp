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
#include "DedicatedWorkerGlobalScope.h"

#if ENABLE(WORKERS)
#include "DOMWindow.h"
#include "DedicatedWorkerThread.h"
#include "MessageEvent.h"
#include "SecurityOrigin.h"
#include "WorkerObjectProxy.h"

namespace WebCore {

PassRefPtr<DedicatedWorkerGlobalScope> DedicatedWorkerGlobalScope::create(const KURL& url, const String& userAgent, PassOwnPtr<GroupSettings> settings, DedicatedWorkerThread* thread, const String& contentSecurityPolicy, ContentSecurityPolicy::HeaderType contentSecurityPolicyType, PassRefPtr<SecurityOrigin> topOrigin)
{
    RefPtr<DedicatedWorkerGlobalScope> context = adoptRef(new DedicatedWorkerGlobalScope(url, userAgent, settings, thread, topOrigin));
    context->applyContentSecurityPolicyFromString(contentSecurityPolicy, contentSecurityPolicyType);
    return context.release();
}

DedicatedWorkerGlobalScope::DedicatedWorkerGlobalScope(const KURL& url, const String& userAgent, PassOwnPtr<GroupSettings> settings, DedicatedWorkerThread* thread, PassRefPtr<SecurityOrigin> topOrigin)
    : WorkerGlobalScope(url, userAgent, settings, thread, topOrigin)
{
}

DedicatedWorkerGlobalScope::~DedicatedWorkerGlobalScope()
{
}

const AtomicString& DedicatedWorkerGlobalScope::interfaceName() const
{
    return eventNames().interfaceForDedicatedWorkerGlobalScope;
}

void DedicatedWorkerGlobalScope::postMessage(PassRefPtr<SerializedScriptValue> message, MessagePort* port, ExceptionCode& ec)
{
    MessagePortArray ports;
    if (port)
        ports.append(port);
    postMessage(message, &ports, ec);
}

void DedicatedWorkerGlobalScope::postMessage(PassRefPtr<SerializedScriptValue> message, const MessagePortArray* ports, ExceptionCode& ec)
{
    // Disentangle the port in preparation for sending it to the remote context.
    OwnPtr<MessagePortChannelArray> channels = MessagePort::disentanglePorts(ports, ec);
    if (ec)
        return;
    thread()->workerObjectProxy().postMessageToWorkerObject(message, channels.release());
}

void DedicatedWorkerGlobalScope::importScripts(const Vector<String>& urls, ExceptionCode& ec)
{
    Base::importScripts(urls, ec);
    thread()->workerObjectProxy().reportPendingActivity(hasPendingActivity());
}

DedicatedWorkerThread* DedicatedWorkerGlobalScope::thread()
{
    return static_cast<DedicatedWorkerThread*>(Base::thread());
}

} // namespace WebCore

#endif // ENABLE(WORKERS)
