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
#include "SharedWorkerProcess.h"

#if ENABLE(SHARED_WORKER_PROCESS)

#include "ArgumentCoders.h"
#include "Attachment.h"
#include "SharedWorkerProcessCreationParameters.h"
#include "SharedWorkerProcessProxyMessages.h"
#include "WebProcessConnection.h"
#include <WebCore/NotImplemented.h>
#include <WebCore/RunLoop.h>

#if PLATFORM(MAC)
#include <crt_externs.h>
#endif

#if USE(UNIX_DOMAIN_SOCKETS)
#include <errno.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wtf/UniStdExtras.h>

#ifdef SOCK_SEQPACKET
#define SOCKET_TYPE SOCK_SEQPACKET
#else
#if PLATFORM(GTK)
#define SOCKET_TYPE SOCK_STREAM
#else
#define SOCKET_TYPE SOCK_DGRAM
#endif
#endif // SOCK_SEQPACKET
#endif // USE(UNIX_DOMAIN_SOCKETS)

using namespace WebCore;

namespace WebKit {

SharedWorkerProcess& SharedWorkerProcess::shared()
{
    DEFINE_STATIC_LOCAL(SharedWorkerProcess, process, ());
    return process;
}

SharedWorkerProcess::SharedWorkerProcess()
    : m_minimumLifetimeTimer(RunLoop::main(), this, &SharedWorkerProcess::minimumLifetimeTimerFired)
{
}

SharedWorkerProcess::~SharedWorkerProcess()
{
}

void SharedWorkerProcess::removeWebProcessConnection(WebProcessConnection* webProcessConnection)
{
    size_t vectorIndex = m_webProcessConnections.find(webProcessConnection);
    ASSERT(vectorIndex != notFound);

    m_webProcessConnections.remove(vectorIndex);
    
    enableTermination();
}

bool SharedWorkerProcess::shouldTerminate()
{
    ASSERT(m_webProcessConnections.isEmpty());

    return true;
}

void SharedWorkerProcess::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageDecoder& decoder)
{
    didReceiveSharedWorkerProcessMessage(connection, decoder);
}

void SharedWorkerProcess::didClose(CoreIPC::Connection*)
{
    // The UI process has crashed, just go ahead and quit.
    RunLoop::current()->stop();
}

void SharedWorkerProcess::didReceiveInvalidMessage(CoreIPC::Connection*, CoreIPC::StringReference, CoreIPC::StringReference)
{
}

void SharedWorkerProcess::initializeSharedWorkerProcess(const SharedWorkerProcessCreationParameters& parameters)
{
    setMinimumLifetime(parameters.minimumLifetime);
    setTerminationTimeout(parameters.terminationTimeout);
}

void SharedWorkerProcess::createWebProcessConnection()
{
#if PLATFORM(MAC)
    // Create the listening port.
    mach_port_t listeningPort;
    mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &listeningPort);

    // Create a listening connection.
    RefPtr<WebProcessConnection> connection = WebProcessConnection::create(CoreIPC::Connection::Identifier(listeningPort));
    m_webProcessConnections.append(connection.release());

    CoreIPC::Attachment clientPort(listeningPort, MACH_MSG_TYPE_MAKE_SEND);
    parentProcessConnection()->send(Messages::SharedWorkerProcessProxy::DidCreateWebProcessConnection(clientPort), 0);
#elif USE(UNIX_DOMAIN_SOCKETS)
    int sockets[2];
    if (socketpair(AF_UNIX, SOCKET_TYPE, 0, sockets) == -1) {
        ASSERT_NOT_REACHED();
        return;
    }

    // Don't expose the shared worker process socket to the web process.
    while (fcntl(sockets[1], F_SETFD, FD_CLOEXEC)  == -1) {
        if (errno != EINTR) {
            ASSERT_NOT_REACHED();
            closeWithRetry(sockets[0]);
            closeWithRetry(sockets[1]);
            return;
        }
    }

    // Don't expose the shared worker process socket to possible future web processes.
    while (fcntl(sockets[0], F_SETFD, FD_CLOEXEC) == -1) {
        if (errno != EINTR) {
            ASSERT_NOT_REACHED();
            closeWithRetry(sockets[0]);
            closeWithRetry(sockets[1]);
            return;
        }
    }

    RefPtr<WebProcessConnection> connection = WebProcessConnection::create(sockets[1]);
    m_webProcessConnections.append(connection.release());

    CoreIPC::Attachment clientSocket(sockets[0]);
    parentProcessConnection()->send(Messages::SharedWorkerProcessProxy::DidCreateWebProcessConnection(clientSocket), 0);
#else
    notImplemented();
#endif

    disableTermination();
}

void SharedWorkerProcess::setMinimumLifetime(double lifetime)
{
    if (lifetime <= 0.0)
        return;
    
    disableTermination();
    
    m_minimumLifetimeTimer.startOneShot(lifetime);
}

void SharedWorkerProcess::minimumLifetimeTimerFired()
{
    enableTermination();
}

#if !PLATFORM(MAC)
void SharedWorkerProcess::initializeProcess(const ChildProcessInitializationParameters&)
{
}

void SharedWorkerProcess::initializeProcessName(const ChildProcessInitializationParameters&)
{
}
#endif

} // namespace WebKit

#endif // ENABLE(SHARED_WORKER_PROCESS)
