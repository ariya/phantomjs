/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef OfflineStorageProcess_h
#define OfflineStorageProcess_h

#include "ChildProcess.h"

namespace WebKit {

struct OfflineStorageProcessCreationParameters;

class OfflineStorageProcess : public ChildProcess {
    WTF_MAKE_NONCOPYABLE(OfflineStorageProcess);
public:
    static OfflineStorageProcess& shared();

private:
    OfflineStorageProcess();
    ~OfflineStorageProcess();

    // ChildProcess
    virtual void initializeProcess(const ChildProcessInitializationParameters&) OVERRIDE;
    virtual void initializeProcessName(const ChildProcessInitializationParameters&) OVERRIDE;
    virtual void initializeSandbox(const ChildProcessInitializationParameters&, SandboxInitializationParameters&) OVERRIDE;
    virtual bool shouldTerminate() OVERRIDE;

    // CoreIPC::Connection::Client
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;
    virtual void didReceiveSyncMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&, OwnPtr<CoreIPC::MessageEncoder>&);
    virtual void didClose(CoreIPC::Connection*) OVERRIDE;
    virtual void didReceiveInvalidMessage(CoreIPC::Connection*, CoreIPC::StringReference messageReceiverName, CoreIPC::StringReference messageName) OVERRIDE;

    // Message Handlers
    void didReceiveOfflineStorageProcessMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&);
    void initializeOfflineStorageProcess(const OfflineStorageProcessCreationParameters&);
};

} // namespace WebKit

#endif // OfflineStorageProcess_h
