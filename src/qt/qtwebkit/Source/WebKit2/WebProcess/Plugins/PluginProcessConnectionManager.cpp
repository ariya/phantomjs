/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "PluginProcessConnectionManager.h"

#if ENABLE(PLUGIN_PROCESS)

#include "ArgumentDecoder.h"
#include "ArgumentEncoder.h"
#include "PluginProcessConnection.h"
#include "PluginProcessConnectionManagerMessages.h"
#include "WebCoreArgumentCoders.h"
#include "WebProcess.h"
#include "WebProcessProxyMessages.h"

#if PLATFORM(MAC)
#include "MachPort.h"
#endif

namespace WebKit {

PassRefPtr<PluginProcessConnectionManager> PluginProcessConnectionManager::create()
{
    return adoptRef(new PluginProcessConnectionManager);
}

PluginProcessConnectionManager::PluginProcessConnectionManager()
    : m_queue(WorkQueue::create("com.apple.WebKit.PluginProcessConnectionManager"))
{
}

PluginProcessConnectionManager::~PluginProcessConnectionManager()
{
}

void PluginProcessConnectionManager::initializeConnection(CoreIPC::Connection* connection)
{
    connection->addWorkQueueMessageReceiver(Messages::PluginProcessConnectionManager::messageReceiverName(), m_queue.get(), this);
}

PluginProcessConnection* PluginProcessConnectionManager::getPluginProcessConnection(uint64_t pluginProcessToken)
{
    for (size_t i = 0; i < m_pluginProcessConnections.size(); ++i) {
        if (m_pluginProcessConnections[i]->pluginProcessToken() == pluginProcessToken)
            return m_pluginProcessConnections[i].get();
    }

    CoreIPC::Attachment encodedConnectionIdentifier;
    bool supportsAsynchronousInitialization;
    if (!WebProcess::shared().parentProcessConnection()->sendSync(Messages::WebProcessProxy::GetPluginProcessConnection(pluginProcessToken),
                                                     Messages::WebProcessProxy::GetPluginProcessConnection::Reply(encodedConnectionIdentifier, supportsAsynchronousInitialization), 0))
        return 0;

#if PLATFORM(MAC)
    CoreIPC::Connection::Identifier connectionIdentifier(encodedConnectionIdentifier.port());
    if (CoreIPC::Connection::identifierIsNull(connectionIdentifier))
        return 0;
#elif USE(UNIX_DOMAIN_SOCKETS)
    CoreIPC::Connection::Identifier connectionIdentifier = encodedConnectionIdentifier.fileDescriptor();
    if (connectionIdentifier == -1)
        return 0;
#endif

    RefPtr<PluginProcessConnection> pluginProcessConnection = PluginProcessConnection::create(this, pluginProcessToken, connectionIdentifier, supportsAsynchronousInitialization);
    m_pluginProcessConnections.append(pluginProcessConnection);

    {
        MutexLocker locker(m_tokensAndConnectionsMutex);
        ASSERT(!m_tokensAndConnections.contains(pluginProcessToken));

        m_tokensAndConnections.set(pluginProcessToken, pluginProcessConnection->connection());
    }

    return pluginProcessConnection.get();
}

void PluginProcessConnectionManager::removePluginProcessConnection(PluginProcessConnection* pluginProcessConnection)
{
    size_t vectorIndex = m_pluginProcessConnections.find(pluginProcessConnection);
    ASSERT(vectorIndex != notFound);

    {
        MutexLocker locker(m_tokensAndConnectionsMutex);
        ASSERT(m_tokensAndConnections.contains(pluginProcessConnection->pluginProcessToken()));
        
        m_tokensAndConnections.remove(pluginProcessConnection->pluginProcessToken());
    }

    m_pluginProcessConnections.remove(vectorIndex);
}

void PluginProcessConnectionManager::pluginProcessCrashed(uint64_t pluginProcessToken)
{
    MutexLocker locker(m_tokensAndConnectionsMutex);
    CoreIPC::Connection* connection = m_tokensAndConnections.get(pluginProcessToken);

    // It's OK for connection to be null here; it will happen if this web process doesn't know
    // anything about the plug-in process.
    if (!connection)
        return;

    connection->postConnectionDidCloseOnConnectionWorkQueue();
}

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)
