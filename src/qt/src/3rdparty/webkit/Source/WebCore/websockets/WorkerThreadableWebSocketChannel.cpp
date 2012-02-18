/*
 * Copyright (C) 2009, 2010 Google Inc.  All rights reserved.
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

#if ENABLE(WEB_SOCKETS) && ENABLE(WORKERS)

#include "WorkerThreadableWebSocketChannel.h"

#include "CrossThreadTask.h"
#include "PlatformString.h"
#include "ScriptExecutionContext.h"
#include "ThreadableWebSocketChannelClientWrapper.h"
#include "WebSocketChannel.h"
#include "WebSocketChannelClient.h"
#include "WorkerContext.h"
#include "WorkerLoaderProxy.h"
#include "WorkerRunLoop.h"
#include "WorkerThread.h"

#include <wtf/PassRefPtr.h>

namespace WebCore {

WorkerThreadableWebSocketChannel::WorkerThreadableWebSocketChannel(WorkerContext* context, WebSocketChannelClient* client, const String& taskMode, const KURL& url, const String& protocol)
    : m_workerContext(context)
    , m_workerClientWrapper(ThreadableWebSocketChannelClientWrapper::create(client))
    , m_bridge(Bridge::create(m_workerClientWrapper, m_workerContext, taskMode, url, protocol))
{
}

WorkerThreadableWebSocketChannel::~WorkerThreadableWebSocketChannel()
{
    if (m_bridge)
        m_bridge->disconnect();
}

void WorkerThreadableWebSocketChannel::connect()
{
    if (m_bridge)
        m_bridge->connect();
}

bool WorkerThreadableWebSocketChannel::send(const String& message)
{
    if (!m_bridge)
        return false;
    return m_bridge->send(message);
}

unsigned long WorkerThreadableWebSocketChannel::bufferedAmount() const
{
    if (!m_bridge)
        return 0;
    return m_bridge->bufferedAmount();
}

void WorkerThreadableWebSocketChannel::close()
{
    if (m_bridge)
        m_bridge->close();
}

void WorkerThreadableWebSocketChannel::disconnect()
{
    m_bridge->disconnect();
    m_bridge.clear();
}

void WorkerThreadableWebSocketChannel::suspend()
{
    m_workerClientWrapper->suspend();
    if (m_bridge)
        m_bridge->suspend();
}

void WorkerThreadableWebSocketChannel::resume()
{
    m_workerClientWrapper->resume();
    if (m_bridge)
        m_bridge->resume();
}

WorkerThreadableWebSocketChannel::Peer::Peer(RefPtr<ThreadableWebSocketChannelClientWrapper> clientWrapper, WorkerLoaderProxy& loaderProxy, ScriptExecutionContext* context, const String& taskMode, const KURL& url, const String& protocol)
    : m_workerClientWrapper(clientWrapper)
    , m_loaderProxy(loaderProxy)
    , m_mainWebSocketChannel(WebSocketChannel::create(context, this, url, protocol))
    , m_taskMode(taskMode)
{
    ASSERT(isMainThread());
}

WorkerThreadableWebSocketChannel::Peer::~Peer()
{
    ASSERT(isMainThread());
    if (m_mainWebSocketChannel)
        m_mainWebSocketChannel->disconnect();
}

void WorkerThreadableWebSocketChannel::Peer::connect()
{
    ASSERT(isMainThread());
    if (!m_mainWebSocketChannel)
        return;
    m_mainWebSocketChannel->connect();
}

static void workerContextDidSend(ScriptExecutionContext* context, RefPtr<ThreadableWebSocketChannelClientWrapper> workerClientWrapper, bool sent)
{
    ASSERT_UNUSED(context, context->isWorkerContext());
    workerClientWrapper->setSent(sent);
}

void WorkerThreadableWebSocketChannel::Peer::send(const String& message)
{
    ASSERT(isMainThread());
    if (!m_mainWebSocketChannel || !m_workerClientWrapper)
        return;
    bool sent = m_mainWebSocketChannel->send(message);
    m_loaderProxy.postTaskForModeToWorkerContext(createCallbackTask(&workerContextDidSend, m_workerClientWrapper, sent), m_taskMode);
}

static void workerContextDidGetBufferedAmount(ScriptExecutionContext* context, RefPtr<ThreadableWebSocketChannelClientWrapper> workerClientWrapper, unsigned long bufferedAmount)
{
    ASSERT_UNUSED(context, context->isWorkerContext());
    workerClientWrapper->setBufferedAmount(bufferedAmount);
}

void WorkerThreadableWebSocketChannel::Peer::bufferedAmount()
{
    ASSERT(isMainThread());
    if (!m_mainWebSocketChannel || !m_workerClientWrapper)
        return;
    unsigned long bufferedAmount = m_mainWebSocketChannel->bufferedAmount();
    m_loaderProxy.postTaskForModeToWorkerContext(createCallbackTask(&workerContextDidGetBufferedAmount, m_workerClientWrapper, bufferedAmount), m_taskMode);
}

void WorkerThreadableWebSocketChannel::Peer::close()
{
    ASSERT(isMainThread());
    if (!m_mainWebSocketChannel)
        return;
    m_mainWebSocketChannel->close();
    m_mainWebSocketChannel = 0;
}

void WorkerThreadableWebSocketChannel::Peer::disconnect()
{
    ASSERT(isMainThread());
    if (!m_mainWebSocketChannel)
        return;
    m_mainWebSocketChannel->disconnect();
    m_mainWebSocketChannel = 0;
}

void WorkerThreadableWebSocketChannel::Peer::suspend()
{
    ASSERT(isMainThread());
    if (!m_mainWebSocketChannel)
        return;
    m_mainWebSocketChannel->suspend();
}

void WorkerThreadableWebSocketChannel::Peer::resume()
{
    ASSERT(isMainThread());
    if (!m_mainWebSocketChannel)
        return;
    m_mainWebSocketChannel->resume();
}

static void workerContextDidConnect(ScriptExecutionContext* context, RefPtr<ThreadableWebSocketChannelClientWrapper> workerClientWrapper)
{
    ASSERT_UNUSED(context, context->isWorkerContext());
    workerClientWrapper->didConnect();
}

void WorkerThreadableWebSocketChannel::Peer::didConnect()
{
    ASSERT(isMainThread());
    m_loaderProxy.postTaskForModeToWorkerContext(createCallbackTask(&workerContextDidConnect, m_workerClientWrapper), m_taskMode);
}

static void workerContextDidReceiveMessage(ScriptExecutionContext* context, RefPtr<ThreadableWebSocketChannelClientWrapper> workerClientWrapper, const String& message)
{
    ASSERT_UNUSED(context, context->isWorkerContext());
    workerClientWrapper->didReceiveMessage(message);
}

void WorkerThreadableWebSocketChannel::Peer::didReceiveMessage(const String& message)
{
    ASSERT(isMainThread());
    m_loaderProxy.postTaskForModeToWorkerContext(createCallbackTask(&workerContextDidReceiveMessage, m_workerClientWrapper, message), m_taskMode);
}

static void workerContextDidClose(ScriptExecutionContext* context, RefPtr<ThreadableWebSocketChannelClientWrapper> workerClientWrapper, unsigned long unhandledBufferedAmount)
{
    ASSERT_UNUSED(context, context->isWorkerContext());
    workerClientWrapper->didClose(unhandledBufferedAmount);
}

void WorkerThreadableWebSocketChannel::Peer::didClose(unsigned long unhandledBufferedAmount)
{
    ASSERT(isMainThread());
    m_mainWebSocketChannel = 0;
    m_loaderProxy.postTaskForModeToWorkerContext(createCallbackTask(&workerContextDidClose, m_workerClientWrapper, unhandledBufferedAmount), m_taskMode);
}

void WorkerThreadableWebSocketChannel::Bridge::setWebSocketChannel(ScriptExecutionContext* context, Bridge* thisPtr, Peer* peer, RefPtr<ThreadableWebSocketChannelClientWrapper> workerClientWrapper)
{
    ASSERT_UNUSED(context, context->isWorkerContext());
    thisPtr->m_peer = peer;
    workerClientWrapper->setSyncMethodDone();
}

void WorkerThreadableWebSocketChannel::Bridge::mainThreadCreateWebSocketChannel(ScriptExecutionContext* context, Bridge* thisPtr, RefPtr<ThreadableWebSocketChannelClientWrapper> clientWrapper, const String& taskMode, const KURL& url, const String& protocol)
{
    ASSERT(isMainThread());
    ASSERT_UNUSED(context, context->isDocument());

    Peer* peer = Peer::create(clientWrapper, thisPtr->m_loaderProxy, context, taskMode, url, protocol);
    thisPtr->m_loaderProxy.postTaskForModeToWorkerContext(
        createCallbackTask(&Bridge::setWebSocketChannel,
                           AllowCrossThreadAccess(thisPtr),
                           AllowCrossThreadAccess(peer), clientWrapper), taskMode);
}

WorkerThreadableWebSocketChannel::Bridge::Bridge(PassRefPtr<ThreadableWebSocketChannelClientWrapper> workerClientWrapper, PassRefPtr<WorkerContext> workerContext, const String& taskMode, const KURL& url, const String& protocol)
    : m_workerClientWrapper(workerClientWrapper)
    , m_workerContext(workerContext)
    , m_loaderProxy(m_workerContext->thread()->workerLoaderProxy())
    , m_taskMode(taskMode)
    , m_peer(0)
{
    ASSERT(m_workerClientWrapper.get());
    setMethodNotCompleted();
    m_loaderProxy.postTaskToLoader(
        createCallbackTask(&Bridge::mainThreadCreateWebSocketChannel,
                           AllowCrossThreadAccess(this), m_workerClientWrapper, m_taskMode, url, protocol));
    waitForMethodCompletion();
    ASSERT(m_peer);
}

WorkerThreadableWebSocketChannel::Bridge::~Bridge()
{
    disconnect();
}

void WorkerThreadableWebSocketChannel::mainThreadConnect(ScriptExecutionContext* context, Peer* peer)
{
    ASSERT(isMainThread());
    ASSERT_UNUSED(context, context->isDocument());
    ASSERT(peer);

    peer->connect();
}

void WorkerThreadableWebSocketChannel::Bridge::connect()
{
    ASSERT(m_workerClientWrapper);
    ASSERT(m_peer);
    m_loaderProxy.postTaskToLoader(
        createCallbackTask(&WorkerThreadableWebSocketChannel::mainThreadConnect, AllowCrossThreadAccess(m_peer)));
}

void WorkerThreadableWebSocketChannel::mainThreadSend(ScriptExecutionContext* context, Peer* peer, const String& message)
{
    ASSERT(isMainThread());
    ASSERT_UNUSED(context, context->isDocument());
    ASSERT(peer);

    peer->send(message);
}

bool WorkerThreadableWebSocketChannel::Bridge::send(const String& message)
{
    if (!m_workerClientWrapper)
        return false;
    ASSERT(m_peer);
    setMethodNotCompleted();
    m_loaderProxy.postTaskToLoader(
        createCallbackTask(&WorkerThreadableWebSocketChannel::mainThreadSend,
                           AllowCrossThreadAccess(m_peer), message));
    RefPtr<Bridge> protect(this);
    waitForMethodCompletion();
    ThreadableWebSocketChannelClientWrapper* clientWrapper = m_workerClientWrapper.get();
    return clientWrapper && clientWrapper->sent();
}

void WorkerThreadableWebSocketChannel::mainThreadBufferedAmount(ScriptExecutionContext* context, Peer* peer)
{
    ASSERT(isMainThread());
    ASSERT_UNUSED(context, context->isDocument());
    ASSERT(peer);

    peer->bufferedAmount();
}

unsigned long WorkerThreadableWebSocketChannel::Bridge::bufferedAmount()
{
    if (!m_workerClientWrapper)
        return 0;
    ASSERT(m_peer);
    setMethodNotCompleted();
    m_loaderProxy.postTaskToLoader(
        createCallbackTask(&WorkerThreadableWebSocketChannel::mainThreadBufferedAmount, AllowCrossThreadAccess(m_peer)));
    RefPtr<Bridge> protect(this);
    waitForMethodCompletion();
    ThreadableWebSocketChannelClientWrapper* clientWrapper = m_workerClientWrapper.get();
    if (clientWrapper)
        return clientWrapper->bufferedAmount();
    return 0;
}

void WorkerThreadableWebSocketChannel::mainThreadClose(ScriptExecutionContext* context, Peer* peer)
{
    ASSERT(isMainThread());
    ASSERT_UNUSED(context, context->isDocument());
    ASSERT(peer);

    peer->close();
}

void WorkerThreadableWebSocketChannel::Bridge::close()
{
    ASSERT(m_peer);
    m_loaderProxy.postTaskToLoader(
        createCallbackTask(&WorkerThreadableWebSocketChannel::mainThreadClose, AllowCrossThreadAccess(m_peer)));
}

void WorkerThreadableWebSocketChannel::mainThreadDestroy(ScriptExecutionContext* context, Peer* peer)
{
    ASSERT(isMainThread());
    ASSERT_UNUSED(context, context->isDocument());
    ASSERT(peer);

    delete peer;
}

void WorkerThreadableWebSocketChannel::Bridge::disconnect()
{
    clearClientWrapper();
    if (m_peer) {
        Peer* peer = m_peer;
        m_peer = 0;
        m_loaderProxy.postTaskToLoader(
            createCallbackTask(&mainThreadDestroy, AllowCrossThreadAccess(peer)));
    }
    m_workerContext = 0;
}

void WorkerThreadableWebSocketChannel::mainThreadSuspend(ScriptExecutionContext* context, Peer* peer)
{
    ASSERT(isMainThread());
    ASSERT_UNUSED(context, context->isDocument());
    ASSERT(peer);

    peer->suspend();
}

void WorkerThreadableWebSocketChannel::Bridge::suspend()
{
    ASSERT(m_peer);
    m_loaderProxy.postTaskToLoader(
        createCallbackTask(&WorkerThreadableWebSocketChannel::mainThreadSuspend, AllowCrossThreadAccess(m_peer)));
}

void WorkerThreadableWebSocketChannel::mainThreadResume(ScriptExecutionContext* context, Peer* peer)
{
    ASSERT(isMainThread());
    ASSERT_UNUSED(context, context->isDocument());
    ASSERT(peer);

    peer->resume();
}

void WorkerThreadableWebSocketChannel::Bridge::resume()
{
    ASSERT(m_peer);
    m_loaderProxy.postTaskToLoader(
        createCallbackTask(&WorkerThreadableWebSocketChannel::mainThreadResume, AllowCrossThreadAccess(m_peer)));
}

void WorkerThreadableWebSocketChannel::Bridge::clearClientWrapper()
{
    m_workerClientWrapper->clearClient();
}

void WorkerThreadableWebSocketChannel::Bridge::setMethodNotCompleted()
{
    ASSERT(m_workerClientWrapper);
    m_workerClientWrapper->clearSyncMethodDone();
}

// Caller of this function should hold a reference to the bridge, because this function may call WebSocket::didClose() in the end,
// which causes the bridge to get disconnected from the WebSocket and deleted if there is no other reference.
void WorkerThreadableWebSocketChannel::Bridge::waitForMethodCompletion()
{
    if (!m_workerContext)
        return;
    WorkerRunLoop& runLoop = m_workerContext->thread()->runLoop();
    MessageQueueWaitResult result = MessageQueueMessageReceived;
    ThreadableWebSocketChannelClientWrapper* clientWrapper = m_workerClientWrapper.get();
    while (m_workerContext && clientWrapper && !clientWrapper->syncMethodDone() && result != MessageQueueTerminated) {
        result = runLoop.runInMode(m_workerContext.get(), m_taskMode); // May cause this bridge to get disconnected, which makes m_workerContext become null.
        clientWrapper = m_workerClientWrapper.get();
    }
}

} // namespace WebCore

#endif // ENABLE(WEB_SOCKETS)
