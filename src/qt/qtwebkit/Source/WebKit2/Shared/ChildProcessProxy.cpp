/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "ChildProcessProxy.h"

#include <WebCore/RunLoop.h>

using namespace WebCore;

namespace WebKit {

ChildProcessProxy::ChildProcessProxy()
{
}

ChildProcessProxy::~ChildProcessProxy()
{
    if (m_connection)
        m_connection->invalidate();

    if (m_processLauncher) {
        m_processLauncher->invalidate();
        m_processLauncher = 0;
    }
}

ChildProcessProxy* ChildProcessProxy::fromConnection(CoreIPC::Connection* connection)
{
    ASSERT(connection);

    ChildProcessProxy* childProcessProxy = static_cast<ChildProcessProxy*>(connection->client());
    ASSERT(childProcessProxy->connection() == connection);

    return childProcessProxy;
}

void ChildProcessProxy::connect()
{
    ASSERT(!m_processLauncher);
    ProcessLauncher::LaunchOptions launchOptions;
    getLaunchOptions(launchOptions);
    m_processLauncher = ProcessLauncher::create(this, launchOptions);
}

void ChildProcessProxy::terminate()
{
    if (m_processLauncher)
        m_processLauncher->terminateProcess();
}

bool ChildProcessProxy::sendMessage(PassOwnPtr<CoreIPC::MessageEncoder> encoder, unsigned messageSendFlags)
{
    // If we're waiting for the web process to launch, we need to stash away the messages so we can send them once we have
    // a CoreIPC connection.
    if (isLaunching()) {
        m_pendingMessages.append(std::make_pair(encoder, messageSendFlags));
        return true;
    }

    // If the web process has exited, connection will be null here.
    if (!m_connection)
        return false;

    return connection()->sendMessage(encoder, messageSendFlags);
}

void ChildProcessProxy::addMessageReceiver(CoreIPC::StringReference messageReceiverName, CoreIPC::MessageReceiver* messageReceiver)
{
    m_messageReceiverMap.addMessageReceiver(messageReceiverName, messageReceiver);
}

void ChildProcessProxy::addMessageReceiver(CoreIPC::StringReference messageReceiverName, uint64_t destinationID, CoreIPC::MessageReceiver* messageReceiver)
{
    m_messageReceiverMap.addMessageReceiver(messageReceiverName, destinationID, messageReceiver);
}

void ChildProcessProxy::removeMessageReceiver(CoreIPC::StringReference messageReceiverName, uint64_t destinationID)
{
    m_messageReceiverMap.removeMessageReceiver(messageReceiverName, destinationID);
}

bool ChildProcessProxy::dispatchMessage(CoreIPC::Connection* connection, CoreIPC::MessageDecoder& decoder)
{
    return m_messageReceiverMap.dispatchMessage(connection, decoder);
}

bool ChildProcessProxy::dispatchSyncMessage(CoreIPC::Connection* connection, CoreIPC::MessageDecoder& decoder, OwnPtr<CoreIPC::MessageEncoder>& replyEncoder)
{
    return m_messageReceiverMap.dispatchSyncMessage(connection, decoder, replyEncoder);
}

bool ChildProcessProxy::isLaunching() const
{
    if (m_processLauncher)
        return m_processLauncher->isLaunching();

    return false;
}

void ChildProcessProxy::didFinishLaunching(ProcessLauncher*, CoreIPC::Connection::Identifier connectionIdentifier)
{
    ASSERT(!m_connection);

    m_connection = CoreIPC::Connection::createServerConnection(connectionIdentifier, this, RunLoop::main());
#if OS(DARWIN)
    m_connection->setShouldCloseConnectionOnMachExceptions();
#elif PLATFORM(QT) && !OS(WINDOWS)
    m_connection->setShouldCloseConnectionOnProcessTermination(processIdentifier());
#endif

    connectionWillOpen(m_connection.get());
    m_connection->open();

    for (size_t i = 0; i < m_pendingMessages.size(); ++i) {
        OwnPtr<CoreIPC::MessageEncoder> message = m_pendingMessages[i].first.release();
        unsigned messageSendFlags = m_pendingMessages[i].second;
        m_connection->sendMessage(message.release(), messageSendFlags);
    }

    m_pendingMessages.clear();
}

void ChildProcessProxy::abortProcessLaunchIfNeeded()
{
    if (!isLaunching())
        return;

    m_processLauncher->invalidate();
    m_processLauncher = 0;
}

void ChildProcessProxy::clearConnection()
{
    if (!m_connection)
        return;

    connectionWillClose(m_connection.get());

    m_connection->invalidate();
    m_connection = nullptr;
}

void ChildProcessProxy::connectionWillOpen(CoreIPC::Connection*)
{
}

void ChildProcessProxy::connectionWillClose(CoreIPC::Connection*)
{
}

} // namespace WebKit
