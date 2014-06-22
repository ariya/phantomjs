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

#ifndef SharedWorkerProcess_h
#define SharedWorkerProcess_h

#if ENABLE(SHARED_WORKER_PROCESS)

#include "ChildProcess.h"
#include <wtf/Forward.h>

namespace WebKit {

class WebProcessConnection;
struct SharedWorkerProcessCreationParameters;

class SharedWorkerProcess : public ChildProcess {
    WTF_MAKE_NONCOPYABLE(SharedWorkerProcess);
public:
    static SharedWorkerProcess& shared();

    void removeWebProcessConnection(WebProcessConnection*);

private:
    SharedWorkerProcess();
    ~SharedWorkerProcess();

    // ChildProcess
    virtual void initializeProcess(const ChildProcessInitializationParameters&) OVERRIDE;
    virtual void initializeProcessName(const ChildProcessInitializationParameters&) OVERRIDE;
    virtual bool shouldTerminate() OVERRIDE;

    // CoreIPC::Connection::Client
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&);
    virtual void didClose(CoreIPC::Connection*);
    virtual void didReceiveInvalidMessage(CoreIPC::Connection*, CoreIPC::StringReference messageReceiverName, CoreIPC::StringReference messageName) OVERRIDE;

    // Message handlers.
    void didReceiveSharedWorkerProcessMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&);
    void initializeSharedWorkerProcess(const SharedWorkerProcessCreationParameters&);
    void createWebProcessConnection();

    void setMinimumLifetime(double);
    void minimumLifetimeTimerFired();

    // Our web process connections.
    Vector<RefPtr<WebProcessConnection>> m_webProcessConnections;

    WebCore::RunLoop::Timer<SharedWorkerProcess> m_minimumLifetimeTimer;
};

} // namespace WebKit

#endif // ENABLE(SHARED_WORKER_PROCESS)

#endif // SharedWorkerProcess_h
