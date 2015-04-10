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
#include "ProcessLauncher.h"

#include "WorkQueue.h"
#include <wtf/StdLibExtras.h>

namespace WebKit {

static WorkQueue* processLauncherWorkQueue()
{
    static WorkQueue* processLauncherWorkQueue = WorkQueue::create("com.apple.WebKit.ProcessLauncher").leakRef();
    return processLauncherWorkQueue;
}

ProcessLauncher::ProcessLauncher(Client* client, const LaunchOptions& launchOptions)
    : m_client(client)
    , m_launchOptions(launchOptions)
    , m_processIdentifier(0)
{
    // Launch the process.
    m_isLaunching = true;
    processLauncherWorkQueue()->dispatch(bind(&ProcessLauncher::launchProcess, this));
}

void ProcessLauncher::didFinishLaunchingProcess(PlatformProcessIdentifier processIdentifier, CoreIPC::Connection::Identifier identifier)
{
    m_processIdentifier = processIdentifier;
    m_isLaunching = false;
    
    if (!m_client) {
        // FIXME: Make Identifier a move-only object and release port rights/connections in the destructor.
#if PLATFORM(MAC)
        if (identifier.port)
            mach_port_mod_refs(mach_task_self(), identifier.port, MACH_PORT_RIGHT_RECEIVE, -1);

#if HAVE(XPC)
        if (identifier.xpcConnection) {
            xpc_release(identifier.xpcConnection);
            identifier.xpcConnection = 0;
        }
#endif
#endif
        return;
    }
    
    m_client->didFinishLaunching(this, identifier);
}

void ProcessLauncher::invalidate()
{
    m_client = 0;
    platformInvalidate();
}

const char* ProcessLauncher::processTypeAsString(ProcessType processType)
{
    switch (processType) {
    case WebProcess:
        return "webprocess";
#if ENABLE(PLUGIN_PROCESS)
    case PluginProcess:
        return "pluginprocess";
#endif
#if ENABLE(NETWORK_PROCESS)
    case NetworkProcess:
        return "networkprocess";
#endif
#if ENABLE(SHARED_WORKER_PROCESS)
    case SharedWorkerProcess:
        return "sharedworkerprocess";
#endif
    }

    ASSERT_NOT_REACHED();
    return 0;
}

bool ProcessLauncher::getProcessTypeFromString(const char* string, ProcessType& processType)
{
    if (!strcmp(string, "webprocess")) {
        processType = WebProcess;
        return true;
    }

#if ENABLE(PLUGIN_PROCESS)
    if (!strcmp(string, "pluginprocess")) {
        processType = PluginProcess;
        return true;
    }
#endif

#if ENABLE(NETWORK_PROCESS)
    if (!strcmp(string, "networkprocess")) {
        processType = NetworkProcess;
        return true;
    }
#endif

#if ENABLE(SHARED_WORKER_PROCESS)
    if (!strcmp(string, "sharedworkerprocess")) {
        processType = SharedWorkerProcess;
        return true;
    }
#endif

    return false;
}

} // namespace WebKit
