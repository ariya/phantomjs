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
#include "PluginProcess.h"

#if ENABLE(PLUGIN_PROCESS)

#include "ArgumentCoders.h"
#include "Attachment.h"
#include "NetscapePlugin.h"
#include "NetscapePluginModule.h"
#include "PluginProcessConnectionMessages.h"
#include "PluginProcessCreationParameters.h"
#include "PluginProcessProxyMessages.h"
#include "WebProcessConnection.h"
#include <WebCore/MemoryPressureHandler.h>
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

PluginProcess& PluginProcess::shared()
{
    DEFINE_STATIC_LOCAL(PluginProcess, pluginProcess, ());
    return pluginProcess;
}

PluginProcess::PluginProcess()
    : m_supportsAsynchronousPluginInitialization(false)
    , m_minimumLifetimeTimer(RunLoop::main(), this, &PluginProcess::minimumLifetimeTimerFired)
#if PLATFORM(MAC)
    , m_compositingRenderServerPort(MACH_PORT_NULL)
#endif
{
    NetscapePlugin::setSetExceptionFunction(WebProcessConnection::setGlobalException);
}

PluginProcess::~PluginProcess()
{
}

void PluginProcess::lowMemoryHandler(bool critical)
{
    UNUSED_PARAM(critical);
    if (shared().shouldTerminate())
        shared().terminate();
}

void PluginProcess::initializeProcess(const ChildProcessInitializationParameters& parameters)
{
    m_pluginPath = parameters.extraInitializationData.get("plugin-path");
    platformInitializeProcess(parameters);

    memoryPressureHandler().setLowMemoryHandler(lowMemoryHandler);
    memoryPressureHandler().install();
}

void PluginProcess::removeWebProcessConnection(WebProcessConnection* webProcessConnection)
{
    size_t vectorIndex = m_webProcessConnections.find(webProcessConnection);
    ASSERT(vectorIndex != notFound);

    m_webProcessConnections.remove(vectorIndex);
    
    if (m_webProcessConnections.isEmpty() && m_pluginModule) {
        // Decrement the load count. This is balanced by a call to incrementLoadCount in createWebProcessConnection.
        m_pluginModule->decrementLoadCount();
    }        

    enableTermination();
}

NetscapePluginModule* PluginProcess::netscapePluginModule()
{
    if (!m_pluginModule) {
        ASSERT(!m_pluginPath.isNull());
        m_pluginModule = NetscapePluginModule::getOrCreate(m_pluginPath);

#if PLATFORM(MAC)
        if (m_pluginModule) {
            if (m_pluginModule->pluginQuirks().contains(PluginQuirks::PrognameShouldBeWebKitPluginHost))
                *const_cast<const char**>(_NSGetProgname()) = "WebKitPluginHost";
        }
#endif
    }

    return m_pluginModule.get();
}

bool PluginProcess::shouldTerminate()
{
    return m_webProcessConnections.isEmpty();
}

void PluginProcess::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageDecoder& decoder)
{
    didReceivePluginProcessMessage(connection, decoder);
}

void PluginProcess::didClose(CoreIPC::Connection*)
{
    // The UI process has crashed, just go ahead and quit.
    // FIXME: If the plug-in is spinning in the main loop, we'll never get this message.
    RunLoop::current()->stop();
}

void PluginProcess::didReceiveInvalidMessage(CoreIPC::Connection*, CoreIPC::StringReference, CoreIPC::StringReference)
{
}

void PluginProcess::initializePluginProcess(const PluginProcessCreationParameters& parameters)
{
    ASSERT(!m_pluginModule);

    m_supportsAsynchronousPluginInitialization = parameters.supportsAsynchronousPluginInitialization;
    setMinimumLifetime(parameters.minimumLifetime);
    setTerminationTimeout(parameters.terminationTimeout);

    platformInitializePluginProcess(parameters);
}

void PluginProcess::createWebProcessConnection()
{
    bool didHaveAnyWebProcessConnections = !m_webProcessConnections.isEmpty();

#if PLATFORM(MAC)
    // Create the listening port.
    mach_port_t listeningPort;
    mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &listeningPort);

    // Create a listening connection.
    RefPtr<WebProcessConnection> connection = WebProcessConnection::create(CoreIPC::Connection::Identifier(listeningPort));
    m_webProcessConnections.append(connection.release());

    CoreIPC::Attachment clientPort(listeningPort, MACH_MSG_TYPE_MAKE_SEND);
    parentProcessConnection()->send(Messages::PluginProcessProxy::DidCreateWebProcessConnection(clientPort, m_supportsAsynchronousPluginInitialization), 0);
#elif USE(UNIX_DOMAIN_SOCKETS)
    int sockets[2];
    if (socketpair(AF_UNIX, SOCKET_TYPE, 0, sockets) == -1) {
        ASSERT_NOT_REACHED();
        return;
    }

    // Don't expose the plugin process socket to the web process.
    while (fcntl(sockets[1], F_SETFD, FD_CLOEXEC)  == -1) {
        if (errno != EINTR) {
            ASSERT_NOT_REACHED();
            closeWithRetry(sockets[0]);
            closeWithRetry(sockets[1]);
            return;
        }
    }

    // Don't expose the web process socket to possible future web processes.
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
    parentProcessConnection()->send(Messages::PluginProcessProxy::DidCreateWebProcessConnection(clientSocket, m_supportsAsynchronousPluginInitialization), 0);
#else
    notImplemented();
#endif

    if (NetscapePluginModule* module = netscapePluginModule()) {
        if (!didHaveAnyWebProcessConnections) {
            // Increment the load count. This is matched by a call to decrementLoadCount in removeWebProcessConnection.
            // We do this so that the plug-in module's NP_Shutdown won't be called until right before exiting.
            module->incrementLoadCount();
        }
    }

    disableTermination();
}

void PluginProcess::getSitesWithData(uint64_t callbackID)
{
    Vector<String> sites;
    if (NetscapePluginModule* module = netscapePluginModule())
        sites = module->sitesWithData();

    parentProcessConnection()->send(Messages::PluginProcessProxy::DidGetSitesWithData(sites, callbackID), 0);
}

void PluginProcess::clearSiteData(const Vector<String>& sites, uint64_t flags, uint64_t maxAgeInSeconds, uint64_t callbackID)
{
    if (NetscapePluginModule* module = netscapePluginModule()) {
        if (sites.isEmpty()) {
            // Clear everything.
            module->clearSiteData(String(), flags, maxAgeInSeconds);
        } else {
            for (size_t i = 0; i < sites.size(); ++i)
                module->clearSiteData(sites[i], flags, maxAgeInSeconds);
        }
    }

    parentProcessConnection()->send(Messages::PluginProcessProxy::DidClearSiteData(callbackID), 0);
}

void PluginProcess::setMinimumLifetime(double lifetime)
{
    if (lifetime <= 0.0)
        return;
    
    disableTermination();
    
    m_minimumLifetimeTimer.startOneShot(lifetime);
}

void PluginProcess::minimumLifetimeTimerFired()
{
    enableTermination();
}

#if !PLATFORM(MAC)
void PluginProcess::initializeProcessName(const ChildProcessInitializationParameters&)
{
}

void PluginProcess::initializeSandbox(const ChildProcessInitializationParameters&, SandboxInitializationParameters&)
{
}
#endif

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)

