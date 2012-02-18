/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include "SharedWorker.h"

#include "InspectorInstrumentation.h"
#include "KURL.h"
#include "MessageChannel.h"
#include "MessagePort.h"
#include "ScriptExecutionContext.h"
#include "SharedWorkerRepository.h"

namespace WebCore {

inline SharedWorker::SharedWorker(ScriptExecutionContext* context)
    : AbstractWorker(context)
{
}

PassRefPtr<SharedWorker> SharedWorker::create(const String& url, const String& name, ScriptExecutionContext* context, ExceptionCode& ec)
{
    RefPtr<SharedWorker> worker = adoptRef(new SharedWorker(context));

    RefPtr<MessageChannel> channel = MessageChannel::create(context);
    worker->m_port = channel->port1();
    OwnPtr<MessagePortChannel> remotePort = channel->port2()->disentangle(ec);
    ASSERT(remotePort);

    KURL scriptURL = worker->resolveURL(url, ec);
    if (scriptURL.isEmpty())
        return 0;

    SharedWorkerRepository::connect(worker.get(), remotePort.release(), scriptURL, name, ec);

    InspectorInstrumentation::didCreateWorker(context, worker->asID(), scriptURL.string(), true);

    return worker.release();
}

SharedWorker::~SharedWorker()
{
}

} // namespace WebCore

#endif  // ENABLE(SHARED_WORKERS)
