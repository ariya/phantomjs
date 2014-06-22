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

#ifndef SharedWorkerProcessProxy_h
#define SharedWorkerProcessProxy_h

#if ENABLE(SHARED_WORKER_PROCESS)

#include "Connection.h"
#include "ProcessLauncher.h"
#include "WebProcessProxyMessages.h"
#include <wtf/Deque.h>

// FIXME: This is platform specific.
namespace CoreIPC {
    class MachPort;
}

namespace WebKit {

class SharedWorkerProcessManager;
class WebProcessProxy;
struct SharedWorkerProcessCreationParameters;

class SharedWorkerProcessProxy : public RefCounted<SharedWorkerProcessProxy>, CoreIPC::Connection::Client, ProcessLauncher::Client {
public:
    static PassRefPtr<SharedWorkerProcessProxy> create(SharedWorkerProcessManager*, const String& url, const String& name);
    ~SharedWorkerProcessProxy();

    // Asks the shared worker process to create a new connection to a web process. The connection identifier will be
    // encoded in the given argument encoder and sent back to the connection of the given web process.
    void getSharedWorkerProcessConnection(PassRefPtr<Messages::WebProcessProxy::GetSharedWorkerProcessConnection::DelayedReply>);
    
    // Terminates the shared worker process.
    void terminate();

    bool isValid() const { return m_connection; }

#if PLATFORM(MAC)
    void setProcessSuppressionEnabled(bool);
#endif

private:
    SharedWorkerProcessProxy(SharedWorkerProcessManager*, const String& url, const String& name);

    void sharedWorkerProcessCrashedOrFailedToLaunch();

    // CoreIPC::Connection::Client
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&);
    virtual void didClose(CoreIPC::Connection*);
    virtual void didReceiveInvalidMessage(CoreIPC::Connection*, CoreIPC::StringReference messageReceiverName, CoreIPC::StringReference messageName);

    // ProcessLauncher::Client
    virtual void didFinishLaunching(ProcessLauncher*, CoreIPC::Connection::Identifier);

    // Message handlers
    void didReceiveSharedWorkerProcessProxyMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&);
    void didCreateWebProcessConnection(const CoreIPC::Attachment&);

    void platformInitializeSharedWorkerProcess(SharedWorkerProcessCreationParameters&);

    // The shared worker host process manager.
    SharedWorkerProcessManager* m_sharedWorkerProcessManager;

    // The connection to the plug-in host process.
    RefPtr<CoreIPC::Connection> m_connection;

    // The process launcher for the plug-in host process.
    RefPtr<ProcessLauncher> m_processLauncher;

    Deque<RefPtr<Messages::WebProcessProxy::GetSharedWorkerProcessConnection::DelayedReply>> m_pendingConnectionReplies;

    // If createPluginConnection is called while the process is still launching we'll keep count of it and send a bunch of requests
    // when the process finishes launching.
    unsigned m_numPendingConnectionRequests;
};

} // namespace WebKit

#endif // ENABLE(SHARED_WORKER_PROCESS)

#endif // SharedWorkerProcessProxy_h
