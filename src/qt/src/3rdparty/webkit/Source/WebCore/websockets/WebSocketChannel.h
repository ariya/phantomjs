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

#ifndef WebSocketChannel_h
#define WebSocketChannel_h

#if ENABLE(WEB_SOCKETS)

#include "SocketStreamHandleClient.h"
#include "ThreadableWebSocketChannel.h"
#include "Timer.h"
#include "WebSocketHandshake.h"
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {

    class ScriptExecutionContext;
    class SocketStreamHandle;
    class SocketStreamError;
    class WebSocketChannelClient;

    class WebSocketChannel : public RefCounted<WebSocketChannel>, public SocketStreamHandleClient, public ThreadableWebSocketChannel {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        static PassRefPtr<WebSocketChannel> create(ScriptExecutionContext* context, WebSocketChannelClient* client, const KURL& url, const String& protocol) { return adoptRef(new WebSocketChannel(context, client, url, protocol)); }
        virtual ~WebSocketChannel();

        virtual void connect();
        virtual bool send(const String& message);
        virtual unsigned long bufferedAmount() const;
        virtual void close();
        virtual void disconnect(); // Will suppress didClose().

        virtual void suspend();
        virtual void resume();

        virtual void didOpen(SocketStreamHandle*);
        virtual void didClose(SocketStreamHandle*);
        virtual void didReceiveData(SocketStreamHandle*, const char*, int);
        virtual void didFail(SocketStreamHandle*, const SocketStreamError&);
        virtual void didReceiveAuthenticationChallenge(SocketStreamHandle*, const AuthenticationChallenge&);
        virtual void didCancelAuthenticationChallenge(SocketStreamHandle*, const AuthenticationChallenge&);

        using RefCounted<WebSocketChannel>::ref;
        using RefCounted<WebSocketChannel>::deref;

    protected:
        virtual void refThreadableWebSocketChannel() { ref(); }
        virtual void derefThreadableWebSocketChannel() { deref(); }

    private:
        WebSocketChannel(ScriptExecutionContext*, WebSocketChannelClient*, const KURL&, const String& protocol);

        bool appendToBuffer(const char* data, size_t len);
        void skipBuffer(size_t len);
        bool processBuffer();
        void resumeTimerFired(Timer<WebSocketChannel>* timer);

        ScriptExecutionContext* m_context;
        WebSocketChannelClient* m_client;
        WebSocketHandshake m_handshake;
        RefPtr<SocketStreamHandle> m_handle;
        char* m_buffer;
        size_t m_bufferSize;

        Timer<WebSocketChannel> m_resumeTimer;
        bool m_suspended;
        bool m_closed;
        bool m_shouldDiscardReceivedData;
        unsigned long m_unhandledBufferedAmount;

        unsigned long m_identifier; // m_identifier == 0 means that we could not obtain a valid identifier.
    };

} // namespace WebCore

#endif // ENABLE(WEB_SOCKETS)

#endif // WebSocketChannel_h
