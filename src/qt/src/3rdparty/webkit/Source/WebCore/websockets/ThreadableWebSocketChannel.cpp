/*
 * Copyright (C) 2009 Google Inc.  All rights reserved.
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

#if ENABLE(WEB_SOCKETS)

#include "ThreadableWebSocketChannel.h"

#include "PlatformString.h"
#include "ScriptExecutionContext.h"
#include "ThreadableWebSocketChannelClientWrapper.h"
#include "WebSocketChannel.h"
#include "WebSocketChannelClient.h"
#include "WorkerContext.h"
#include "WorkerRunLoop.h"
#include "WorkerThread.h"
#include "WorkerThreadableWebSocketChannel.h"

#include <wtf/PassRefPtr.h>

namespace WebCore {

static const char webSocketChannelMode[] = "webSocketChannelMode";

PassRefPtr<ThreadableWebSocketChannel> ThreadableWebSocketChannel::create(ScriptExecutionContext* context, WebSocketChannelClient* client, const KURL& url, const String& protocol)
{
    ASSERT(context);
    ASSERT(client);

#if ENABLE(WORKERS)
    if (context->isWorkerContext()) {
        WorkerContext* workerContext = static_cast<WorkerContext*>(context);
        WorkerRunLoop& runLoop = workerContext->thread()->runLoop();
        String mode = webSocketChannelMode;
        mode.append(String::number(runLoop.createUniqueId()));
        return WorkerThreadableWebSocketChannel::create(workerContext, client, mode, url, protocol);
    }
#endif // ENABLE(WORKERS)

    ASSERT(context->isDocument());
    return WebSocketChannel::create(context, client, url, protocol);
}

} // namespace WebCore

#endif // ENABLE(WEB_SOCKETS)
