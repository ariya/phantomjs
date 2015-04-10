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

#ifndef ChildProcessProxy_h
#define ChildProcessProxy_h

#include "Connection.h"
#include "MessageReceiverMap.h"
#include "ProcessLauncher.h"

#include <wtf/ThreadSafeRefCounted.h>

namespace WebKit {

class ChildProcessProxy : ProcessLauncher::Client, public CoreIPC::Connection::Client, public ThreadSafeRefCounted<ChildProcessProxy> {
    WTF_MAKE_NONCOPYABLE(ChildProcessProxy);

public:
    ChildProcessProxy();
    virtual ~ChildProcessProxy();

    // FIXME: This function does an unchecked upcast, and it is only used in a deprecated code path. Would like to get rid of it.
    static ChildProcessProxy* fromConnection(CoreIPC::Connection*);

    void connect();
    void terminate();

    template<typename T> bool send(const T& message, uint64_t destinationID, unsigned messageSendFlags = 0);
    template<typename U> bool sendSync(const U& message, const typename U::Reply&, uint64_t destinationID, double timeout = 1);
    
    CoreIPC::Connection* connection() const
    {
        ASSERT(m_connection);
        return m_connection.get();
    }

    void addMessageReceiver(CoreIPC::StringReference messageReceiverName, CoreIPC::MessageReceiver*);
    void addMessageReceiver(CoreIPC::StringReference messageReceiverName, uint64_t destinationID, CoreIPC::MessageReceiver*);
    void removeMessageReceiver(CoreIPC::StringReference messageReceiverName, uint64_t destinationID);

    bool isValid() const { return m_connection; }
    bool isLaunching() const;
    bool canSendMessage() const { return isValid() || isLaunching(); }

    PlatformProcessIdentifier processIdentifier() const { return m_processLauncher->processIdentifier(); }

protected:
    void clearConnection();
    void abortProcessLaunchIfNeeded();

    // ProcessLauncher::Client
    virtual void didFinishLaunching(ProcessLauncher*, CoreIPC::Connection::Identifier) OVERRIDE;

    bool dispatchMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&);
    bool dispatchSyncMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&, OwnPtr<CoreIPC::MessageEncoder>&);

private:
    virtual void getLaunchOptions(ProcessLauncher::LaunchOptions&) = 0;
    virtual void connectionWillOpen(CoreIPC::Connection*);
    virtual void connectionWillClose(CoreIPC::Connection*);

    bool sendMessage(PassOwnPtr<CoreIPC::MessageEncoder>, unsigned messageSendFlags);

    Vector<std::pair<OwnPtr<CoreIPC::MessageEncoder>, unsigned> > m_pendingMessages;
    RefPtr<ProcessLauncher> m_processLauncher;
    RefPtr<CoreIPC::Connection> m_connection;
    CoreIPC::MessageReceiverMap m_messageReceiverMap;
};

template<typename T>
bool ChildProcessProxy::send(const T& message, uint64_t destinationID, unsigned messageSendFlags)
{
    COMPILE_ASSERT(!T::isSync, AsyncMessageExpected);

    OwnPtr<CoreIPC::MessageEncoder> encoder = CoreIPC::MessageEncoder::create(T::receiverName(), T::name(), destinationID);
    encoder->encode(message);

    return sendMessage(encoder.release(), messageSendFlags);
}

template<typename U> 
bool ChildProcessProxy::sendSync(const U& message, const typename U::Reply& reply, uint64_t destinationID, double timeout)
{
    COMPILE_ASSERT(U::isSync, SyncMessageExpected);

    if (!m_connection)
        return false;

    return connection()->sendSync(message, reply, destinationID, timeout);
}

} // namespace WebKit

#endif // ChildProcessProxy_h
