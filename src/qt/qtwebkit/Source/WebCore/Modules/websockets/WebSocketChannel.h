/*
 * Copyright (C) 2011, 2012 Google Inc.  All rights reserved.
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

#include "FileReaderLoaderClient.h"
#include "SocketStreamHandleClient.h"
#include "ThreadableWebSocketChannel.h"
#include "Timer.h"
#include "WebSocketDeflateFramer.h"
#include "WebSocketFrame.h"
#include "WebSocketHandshake.h"
#include <wtf/Deque.h>
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>

namespace WebCore {

class Blob;
class Document;
class FileReaderLoader;
class SocketStreamHandle;
class SocketStreamError;
class WebSocketChannelClient;

class WebSocketChannel : public RefCounted<WebSocketChannel>, public SocketStreamHandleClient, public ThreadableWebSocketChannel
#if ENABLE(BLOB)
                       , public FileReaderLoaderClient
#endif
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassRefPtr<WebSocketChannel> create(Document* document, WebSocketChannelClient* client) { return adoptRef(new WebSocketChannel(document, client)); }
    virtual ~WebSocketChannel();

    bool send(const char* data, int length);

    // ThreadableWebSocketChannel functions.
    virtual void connect(const KURL&, const String& protocol) OVERRIDE;
    virtual String subprotocol() OVERRIDE;
    virtual String extensions() OVERRIDE;
    virtual ThreadableWebSocketChannel::SendResult send(const String& message) OVERRIDE;
    virtual ThreadableWebSocketChannel::SendResult send(const ArrayBuffer&, unsigned byteOffset, unsigned byteLength) OVERRIDE;
    virtual ThreadableWebSocketChannel::SendResult send(const Blob&) OVERRIDE;
    virtual unsigned long bufferedAmount() const OVERRIDE;
    virtual void close(int code, const String& reason) OVERRIDE; // Start closing handshake.
    virtual void fail(const String& reason) OVERRIDE;
    virtual void disconnect() OVERRIDE;

    virtual void suspend() OVERRIDE;
    virtual void resume() OVERRIDE;

    // SocketStreamHandleClient functions.
    virtual void willOpenSocketStream(SocketStreamHandle*) OVERRIDE;
    virtual void didOpenSocketStream(SocketStreamHandle*) OVERRIDE;
    virtual void didCloseSocketStream(SocketStreamHandle*) OVERRIDE;
    virtual void didReceiveSocketStreamData(SocketStreamHandle*, const char*, int) OVERRIDE;
    virtual void didUpdateBufferedAmount(SocketStreamHandle*, size_t bufferedAmount) OVERRIDE;
    virtual void didFailSocketStream(SocketStreamHandle*, const SocketStreamError&) OVERRIDE;
    virtual void didReceiveAuthenticationChallenge(SocketStreamHandle*, const AuthenticationChallenge&) OVERRIDE;
    virtual void didCancelAuthenticationChallenge(SocketStreamHandle*, const AuthenticationChallenge&) OVERRIDE;

    enum CloseEventCode {
        CloseEventCodeNotSpecified = -1,
        CloseEventCodeNormalClosure = 1000,
        CloseEventCodeGoingAway = 1001,
        CloseEventCodeProtocolError = 1002,
        CloseEventCodeUnsupportedData = 1003,
        CloseEventCodeFrameTooLarge = 1004,
        CloseEventCodeNoStatusRcvd = 1005,
        CloseEventCodeAbnormalClosure = 1006,
        CloseEventCodeInvalidFramePayloadData = 1007,
        CloseEventCodePolicyViolation = 1008,
        CloseEventCodeMessageTooBig = 1009,
        CloseEventCodeMandatoryExt = 1010,
        CloseEventCodeInternalError = 1011,
        CloseEventCodeTLSHandshake = 1015,
        CloseEventCodeMinimumUserDefined = 3000,
        CloseEventCodeMaximumUserDefined = 4999
    };

#if ENABLE(BLOB)
    // FileReaderLoaderClient functions.
    virtual void didStartLoading();
    virtual void didReceiveData();
    virtual void didFinishLoading();
    virtual void didFail(int errorCode);
#endif

    using RefCounted<WebSocketChannel>::ref;
    using RefCounted<WebSocketChannel>::deref;

protected:
    virtual void refThreadableWebSocketChannel() { ref(); }
    virtual void derefThreadableWebSocketChannel() { deref(); }

private:
    WebSocketChannel(Document*, WebSocketChannelClient*);

    bool appendToBuffer(const char* data, size_t len);
    void skipBuffer(size_t len);
    bool processBuffer();
    void resumeTimerFired(Timer<WebSocketChannel>*);
    void startClosingHandshake(int code, const String& reason);
    void closingTimerFired(Timer<WebSocketChannel>*);

    bool processFrame();

    // It is allowed to send a Blob as a binary frame if hybi-10 protocol is in use. Sending a Blob
    // can be delayed because it must be read asynchronously. Other types of data (String or
    // ArrayBuffer) may also be blocked by preceding sending request of a Blob.
    //
    // To address this situation, messages to be sent need to be stored in a queue. Whenever a new
    // data frame is going to be sent, it first must go to the queue. Items in the queue are processed
    // in the order they were put into the queue. Sending request of a Blob blocks further processing
    // until the Blob is completely read and sent to the socket stream.
    enum QueuedFrameType {
        QueuedFrameTypeString,
        QueuedFrameTypeVector,
        QueuedFrameTypeBlob
    };
    struct QueuedFrame {
        WebSocketFrame::OpCode opCode;
        QueuedFrameType frameType;
        // Only one of the following items is used, according to the value of frameType.
        CString stringData;
        Vector<char> vectorData;
        RefPtr<Blob> blobData;
    };
    void enqueueTextFrame(const CString&);
    void enqueueRawFrame(WebSocketFrame::OpCode, const char* data, size_t dataLength);
    void enqueueBlobFrame(WebSocketFrame::OpCode, const Blob&);

    void processOutgoingFrameQueue();
    void abortOutgoingFrameQueue();

    enum OutgoingFrameQueueStatus {
        // It is allowed to put a new item into the queue.
        OutgoingFrameQueueOpen,
        // Close frame has already been put into the queue but may not have been sent yet;
        // m_handle->close() will be called as soon as the queue is cleared. It is not
        // allowed to put a new item into the queue.
        OutgoingFrameQueueClosing,
        // Close frame has been sent or the queue was aborted. It is not allowed to put
        // a new item to the queue.
        OutgoingFrameQueueClosed
    };

    // If you are going to send a hybi-10 frame, you need to use the outgoing frame queue
    // instead of call sendFrame() directly.
    bool sendFrame(WebSocketFrame::OpCode, const char* data, size_t dataLength);

#if ENABLE(BLOB)
    enum BlobLoaderStatus {
        BlobLoaderNotStarted,
        BlobLoaderStarted,
        BlobLoaderFinished,
        BlobLoaderFailed
    };
#endif

    Document* m_document;
    WebSocketChannelClient* m_client;
    OwnPtr<WebSocketHandshake> m_handshake;
    RefPtr<SocketStreamHandle> m_handle;
    Vector<char> m_buffer;

    Timer<WebSocketChannel> m_resumeTimer;
    bool m_suspended;
    bool m_closing;
    bool m_receivedClosingHandshake;
    Timer<WebSocketChannel> m_closingTimer;
    bool m_closed;
    bool m_shouldDiscardReceivedData;
    unsigned long m_unhandledBufferedAmount;

    unsigned long m_identifier; // m_identifier == 0 means that we could not obtain a valid identifier.

    // Private members only for hybi-10 protocol.
    bool m_hasContinuousFrame;
    WebSocketFrame::OpCode m_continuousFrameOpCode;
    Vector<char> m_continuousFrameData;
    unsigned short m_closeEventCode;
    String m_closeEventReason;

    Deque<OwnPtr<QueuedFrame> > m_outgoingFrameQueue;
    OutgoingFrameQueueStatus m_outgoingFrameQueueStatus;

#if ENABLE(BLOB)
    // FIXME: Load two or more Blobs simultaneously for better performance.
    OwnPtr<FileReaderLoader> m_blobLoader;
    BlobLoaderStatus m_blobLoaderStatus;
#endif

    WebSocketDeflateFramer m_deflateFramer;
};

} // namespace WebCore

#endif // ENABLE(WEB_SOCKETS)

#endif // WebSocketChannel_h
