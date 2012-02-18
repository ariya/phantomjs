/*
 * Copyright (C) 2009 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ThreadableWebSocketChannelClientWrapper_h
#define ThreadableWebSocketChannelClientWrapper_h

#if ENABLE(WEB_SOCKETS)

#include "PlatformString.h"
#include "WebSocketChannelClient.h"
#include <wtf/Forward.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>

namespace WebCore {

class ThreadableWebSocketChannelClientWrapper : public ThreadSafeRefCounted<ThreadableWebSocketChannelClientWrapper> {
public:
    static PassRefPtr<ThreadableWebSocketChannelClientWrapper> create(WebSocketChannelClient* client)
    {
        return adoptRef(new ThreadableWebSocketChannelClientWrapper(client));
    }

    void clearSyncMethodDone()
    {
        m_syncMethodDone = false;
    }
    void setSyncMethodDone()
    {
        m_syncMethodDone = true;
    }

    bool syncMethodDone() const
    {
        return m_syncMethodDone;
    }

    bool sent() const
    {
        return m_sent;
    }
    void setSent(bool sent)
    {
        m_sent = sent;
        m_syncMethodDone = true;
    }

    unsigned long bufferedAmount() const
    {
        return m_bufferedAmount;
    }
    void setBufferedAmount(unsigned long bufferedAmount)
    {
        m_bufferedAmount = bufferedAmount;
        m_syncMethodDone = true;
    }

    void clearClient()
    {
        m_client = 0;
    }

    void didConnect()
    {
        m_pendingConnected = true;
        if (!m_suspended)
            processPendingEvents();
    }

    void didReceiveMessage(const String& msg)
    {
        m_pendingMessages.append(msg);
        if (!m_suspended)
            processPendingEvents();
    }

    void didClose(unsigned long unhandledBufferedAmount)
    {
        m_pendingClosed = true;
        m_bufferedAmount = unhandledBufferedAmount;
        if (!m_suspended)
            processPendingEvents();
    }

    void suspend()
    {
        m_suspended = true;
    }

    void resume()
    {
        m_suspended = false;
        processPendingEvents();
    }

protected:
    ThreadableWebSocketChannelClientWrapper(WebSocketChannelClient* client)
        : m_client(client)
        , m_syncMethodDone(false)
        , m_sent(false)
        , m_bufferedAmount(0)
        , m_suspended(false)
        , m_pendingConnected(false)
        , m_pendingClosed(false)
    {
    }

    void processPendingEvents()
    {
        ASSERT(!m_suspended);
        if (m_pendingConnected) {
            m_pendingConnected = false;
            if (m_client)
                m_client->didConnect();
        }

        Vector<String> messages;
        messages.swap(m_pendingMessages);
        for (Vector<String>::const_iterator iter = messages.begin(); iter != messages.end(); ++iter) {
            if (m_client)
                m_client->didReceiveMessage(*iter);
        }

        if (m_pendingClosed) {
            m_pendingClosed = false;
            if (m_client)
                m_client->didClose(m_bufferedAmount);
        }
    }

    WebSocketChannelClient* m_client;
    bool m_syncMethodDone;
    bool m_sent;
    unsigned long m_bufferedAmount;
    bool m_suspended;
    bool m_pendingConnected;
    Vector<String> m_pendingMessages;
    bool m_pendingClosed;
};

} // namespace WebCore

#endif // ENABLE(WEB_SOCKETS)

#endif // ThreadableWebSocketChannelClientWrapper_h
