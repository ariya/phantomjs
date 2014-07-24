/*
 * Copyright (C) 2010, 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SharedWorkerProcessProxy.h"

#if ENABLE(SHARED_WORKER_PROCESS)

#include "SharedWorkerProcessCreationParameters.h"
#include "SharedWorkerProcessManager.h"
#include "SharedWorkerProcessMessages.h"
#include "WebContext.h"
#include "WebCoreArgumentCoders.h"
#include "WebProcessMessages.h"
#include "WebProcessProxy.h"
#include <WebCore/NotImplemented.h>
#include <WebCore/RunLoop.h>

#if PLATFORM(MAC)
#include "MachPort.h"
#endif

using namespace WebCore;

namespace WebKit {

// FIXME: Are these relevant to shared worker process at all?
static const double minimumLifetime = 30 * 60;
static const double shutdownTimeout = 10 * 60;

PassRefPtr<SharedWorkerProcessProxy> SharedWorkerProcessProxy::create(SharedWorkerProcessManager* sharedWorkerProcessManager, const String& url, const String& name)
{
    return adoptRef(new SharedWorkerProcessProxy(sharedWorkerProcessManager, url, name));
}

SharedWorkerProcessProxy::SharedWorkerProcessProxy(SharedWorkerProcessManager* sharedWorkerProcessManager, const String& url, const String& name)
    : m_sharedWorkerProcessManager(sharedWorkerProcessManager)
    , m_numPendingConnectionRequests(0)
{
    ProcessLauncher::LaunchOptions launchOptions;
    launchOptions.processType = ProcessLauncher::SharedWorkerProcess;
    // FIXME: Pass URL down to the process to load.
#if PLATFORM(MAC)
#if HAVE(XPC)
    launchOptions.useXPC = false;
#endif
#endif

    m_processLauncher = ProcessLauncher::create(this, launchOptions);
}

SharedWorkerProcessProxy::~SharedWorkerProcessProxy()
{
}

// Asks the shared worker process to create a new connection to a web process. The connection identifier will be
// encoded in the given argument encoder and sent back to the connection of the given web process.
void SharedWorkerProcessProxy::getSharedWorkerProcessConnection(PassRefPtr<Messages::WebProcessProxy::GetSharedWorkerProcessConnection::DelayedReply> reply)
{
    m_pendingConnectionReplies.append(reply);

    if (m_processLauncher->isLaunching()) {
        m_numPendingConnectionRequests++;
        return;
    }
    
    // Ask the shared worker process to create a connection. Since the shared worker can be waiting for a synchronous reply
    // we need to make sure that this message is always processed, even when the shared worker is waiting for a synchronus reply.
    m_connection->send(Messages::SharedWorkerProcess::CreateWebProcessConnection(), 0, CoreIPC::DispatchMessageEvenWhenWaitingForSyncReply);
}

void SharedWorkerProcessProxy::terminate()
{
    m_processLauncher->terminateProcess();
}

void SharedWorkerProcessProxy::sharedWorkerProcessCrashedOrFailedToLaunch()
{
    // The shared worker process must have crashed or exited, send any pending sync replies we might have.
    while (!m_pendingConnectionReplies.isEmpty()) {
        RefPtr<Messages::WebProcessProxy::GetSharedWorkerProcessConnection::DelayedReply> reply = m_pendingConnectionReplies.takeFirst();

#if PLATFORM(MAC)
        reply->send(CoreIPC::Attachment(0, MACH_MSG_TYPE_MOVE_SEND));
#elif USE(UNIX_DOMAIN_SOCKETS)
        reply->send(CoreIPC::Attachment());
#else
        notImplemented();
#endif
    }

    // Tell the shared worker process manager to forget about this shared worker process proxy. This may cause us to be deleted.
    m_sharedWorkerProcessManager->removeSharedWorkerProcessProxy(this);
}

void SharedWorkerProcessProxy::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageDecoder& decoder)
{
    didReceiveSharedWorkerProcessProxyMessage(connection, decoder);
}

void SharedWorkerProcessProxy::didClose(CoreIPC::Connection*)
{
    // FIXME: Notify web processes.

    // This will cause us to be deleted.
    sharedWorkerProcessCrashedOrFailedToLaunch();
}

void SharedWorkerProcessProxy::didReceiveInvalidMessage(CoreIPC::Connection*, CoreIPC::StringReference, CoreIPC::StringReference)
{
}

void SharedWorkerProcessProxy::didFinishLaunching(ProcessLauncher*, CoreIPC::Connection::Identifier connectionIdentifier)
{
    ASSERT(!m_connection);

    if (CoreIPC::Connection::identifierIsNull(connectionIdentifier)) {
        sharedWorkerProcessCrashedOrFailedToLaunch();
        return;
    }

    m_connection = CoreIPC::Connection::createServerConnection(connectionIdentifier, this, RunLoop::main());
#if PLATFORM(MAC)
    m_connection->setShouldCloseConnectionOnMachExceptions();
#elif PLATFORM(QT)
    m_connection->setShouldCloseConnectionOnProcessTermination(m_processLauncher->processIdentifier());
#endif

    m_connection->open();
    
    SharedWorkerProcessCreationParameters parameters;

    parameters.minimumLifetime = minimumLifetime;
    parameters.terminationTimeout = shutdownTimeout;

    platformInitializeSharedWorkerProcess(parameters);

    // Initialize the shared worker host process.
    m_connection->send(Messages::SharedWorkerProcess::InitializeSharedWorkerProcess(parameters), 0);

    for (unsigned i = 0; i < m_numPendingConnectionRequests; ++i)
        m_connection->send(Messages::SharedWorkerProcess::CreateWebProcessConnection(), 0);
    
    m_numPendingConnectionRequests = 0;

#if PLATFORM(MAC)
    if (WebContext::canEnableProcessSuppressionForGlobalChildProcesses())
        setProcessSuppressionEnabled(true);
#endif
}

void SharedWorkerProcessProxy::didCreateWebProcessConnection(const CoreIPC::Attachment& connectionIdentifier)
{
    ASSERT(!m_pendingConnectionReplies.isEmpty());

    // Grab the first pending connection reply.
    RefPtr<Messages::WebProcessProxy::GetSharedWorkerProcessConnection::DelayedReply> reply = m_pendingConnectionReplies.takeFirst();

#if PLATFORM(MAC)
    reply->send(CoreIPC::Attachment(connectionIdentifier.port(), MACH_MSG_TYPE_MOVE_SEND));
#elif USE(UNIX_DOMAIN_SOCKETS)
    reply->send(connectionIdentifier);
#else
    notImplemented();
#endif
}

} // namespace WebKit

#endif // ENABLE(SHARED_WORKER_PROCESS)
