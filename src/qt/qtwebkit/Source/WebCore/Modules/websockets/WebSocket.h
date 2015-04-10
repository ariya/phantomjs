/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
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
#include "WebSocketChannel.h"
#include "WebSocketChannelClient.h"
#include <wtf/Forward.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

class Blob;
class ThreadableWebSocketChannel;

class WebSocket : public RefCounted<WebSocket>, public EventTarget, public ActiveDOMObject, public WebSocketChannelClient {
public:
    static void setIsAvailable(bool);
    static bool isAvailable();
    static const char* subProtocolSeperator();
    static PassRefPtr<WebSocket> create(ScriptExecutionContext*);
    static PassRefPtr<WebSocket> create(ScriptExecutionContext*, const String& url, ExceptionCode&);
    static PassRefPtr<WebSocket> create(ScriptExecutionContext*, const String& url, const String& protocol, ExceptionCode&);
    static PassRefPtr<WebSocket> create(ScriptExecutionContext*, const String& url, const Vector<String>& protocols, ExceptionCode&);
    virtual ~WebSocket();

    enum State {
        CONNECTING = 0,
        OPEN = 1,
        CLOSING = 2,
        CLOSED = 3
    };

    void connect(const String& url, ExceptionCode&);
    void connect(const String& url, const String& protocol, ExceptionCode&);
    void connect(const String& url, const Vector<String>& protocols, ExceptionCode&);

    void send(const String& message, ExceptionCode&);
    void send(ArrayBuffer*, ExceptionCode&);
    void send(ArrayBufferView*, ExceptionCode&);
    void send(Blob*, ExceptionCode&);

    void close(int code, const String& reason, ExceptionCode&);
    void close(ExceptionCode& ec) { close(WebSocketChannel::CloseEventCodeNotSpecified, String(), ec); }
    void close(int code, ExceptionCode& ec) { close(code, String(), ec); }

    const KURL& url() const;
    State readyState() const;
    unsigned long bufferedAmount() const;

    String protocol() const;
    String extensions() const;

    String binaryType() const;
    void setBinaryType(const String&);

    DEFINE_ATTRIBUTE_EVENT_LISTENER(open);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(message);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(error);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(close);

    // EventTarget functions.
    virtual const AtomicString& interfaceName() const OVERRIDE;
    virtual ScriptExecutionContext* scriptExecutionContext() const OVERRIDE;

    // ActiveDOMObject functions.
    virtual void contextDestroyed() OVERRIDE;
    virtual bool canSuspend() const OVERRIDE;
    virtual void suspend(ReasonForSuspension) OVERRIDE;
    virtual void resume() OVERRIDE;
    virtual void stop() OVERRIDE;

    using RefCounted<WebSocket>::ref;
    using RefCounted<WebSocket>::deref;

    // WebSocketChannelClient functions.
    virtual void didConnect() OVERRIDE;
    virtual void didReceiveMessage(const String& message) OVERRIDE;
    virtual void didReceiveBinaryData(PassOwnPtr<Vector<char> >) OVERRIDE;
    virtual void didReceiveMessageError() OVERRIDE;
    virtual void didUpdateBufferedAmount(unsigned long bufferedAmount) OVERRIDE;
    virtual void didStartClosingHandshake() OVERRIDE;
    virtual void didClose(unsigned long unhandledBufferedAmount, ClosingHandshakeCompletionStatus, unsigned short code, const String& reason) OVERRIDE;

private:
    explicit WebSocket(ScriptExecutionContext*);

    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }
    virtual EventTargetData* eventTargetData();
    virtual EventTargetData* ensureEventTargetData();

    size_t getFramingOverhead(size_t payloadSize);

    enum BinaryType {
        BinaryTypeBlob,
        BinaryTypeArrayBuffer
    };

    RefPtr<ThreadableWebSocketChannel> m_channel;

    State m_state;
    KURL m_url;
    EventTargetData m_eventTargetData;
    unsigned long m_bufferedAmount;
    unsigned long m_bufferedAmountAfterClose;
    BinaryType m_binaryType;
    String m_subprotocol;
    String m_extensions;
};

} // namespace WebCore

#endif // ENABLE(WEB_SOCKETS)

#endif // WebSocket_h
