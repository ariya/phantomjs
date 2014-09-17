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

#ifndef WebSocket_h
#define WebSocket_h

#if ENABLE(WEB_SOCKETS)

#include "ActiveDOMObject.h"
#include "EventListener.h"
#include "EventNames.h"
#include "EventTarget.h"
#include "KURL.h"
#include "WebSocketChannelClient.h"
#include <wtf/Forward.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

    class ThreadableWebSocketChannel;

    class WebSocket : public RefCounted<WebSocket>, public EventTarget, public ActiveDOMObject, public WebSocketChannelClient {
    public:
        static void setIsAvailable(bool);
        static bool isAvailable();
        static PassRefPtr<WebSocket> create(ScriptExecutionContext* context) { return adoptRef(new WebSocket(context)); }
        virtual ~WebSocket();

        enum State {
            CONNECTING = 0,
            OPEN = 1,
            CLOSED = 2
        };

        void connect(const KURL&, ExceptionCode&);
        void connect(const KURL&, const String& protocol, ExceptionCode&);

        bool send(const String& message, ExceptionCode&);

        void close();

        const KURL& url() const;
        State readyState() const;
        unsigned long bufferedAmount() const;

        DEFINE_ATTRIBUTE_EVENT_LISTENER(open);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(message);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(error);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(close);

        // EventTarget
        virtual WebSocket* toWebSocket() { return this; }

        virtual ScriptExecutionContext* scriptExecutionContext() const;
        virtual void contextDestroyed();
        virtual bool canSuspend() const;
        virtual void suspend(ReasonForSuspension);
        virtual void resume();
        virtual void stop();

        using RefCounted<WebSocket>::ref;
        using RefCounted<WebSocket>::deref;

        // WebSocketChannelClient
        virtual void didConnect();
        virtual void didReceiveMessage(const String& message);
        virtual void didReceiveMessageError();
        virtual void didClose(unsigned long unhandledBufferedAmount);

    private:
        WebSocket(ScriptExecutionContext*);

        virtual void refEventTarget() { ref(); }
        virtual void derefEventTarget() { deref(); }
        virtual EventTargetData* eventTargetData();
        virtual EventTargetData* ensureEventTargetData();

        RefPtr<ThreadableWebSocketChannel> m_channel;

        State m_state;
        KURL m_url;
        String m_protocol;
        EventTargetData m_eventTargetData;
        unsigned long m_bufferedAmountAfterClose;
    };

} // namespace WebCore

#endif // ENABLE(WEB_SOCKETS)

#endif // WebSocket_h
