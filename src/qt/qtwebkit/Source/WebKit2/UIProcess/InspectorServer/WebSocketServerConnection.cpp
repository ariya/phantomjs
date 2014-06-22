/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(INSPECTOR_SERVER)

#include "WebSocketServerConnection.h"

#include "HTTPRequest.h"
#include "WebSocketServer.h"
#include "WebSocketServerClient.h"
#include <WebCore/NotImplemented.h>
#include <WebCore/SocketStreamError.h>
#include <WebCore/SocketStreamHandle.h>
#include <WebCore/WebSocketChannel.h>
#include <WebCore/WebSocketHandshake.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

using namespace WebCore;

namespace WebKit {

WebSocketServerConnection::WebSocketServerConnection(WebSocketServerClient* client, WebSocketServer* server)
    : m_identifier(0)
    , m_mode(HTTP)
    , m_server(server)
    , m_client(client)
    , m_shutdownAfterSend(false)
{
}

WebSocketServerConnection::~WebSocketServerConnection()
{
    shutdownNow();
}

void WebSocketServerConnection::setSocketHandle(PassRefPtr<WebCore::SocketStreamHandle> socket)
{
    ASSERT(!m_socket);
    m_socket = socket;
}

void WebSocketServerConnection::shutdownNow()
{
    if (!m_socket)
        return;
    RefPtr<SocketStreamHandle> socket = m_socket.release();
    socket->close();
    m_shutdownAfterSend = false;
}

void WebSocketServerConnection::shutdownAfterSendOrNow()
{
    if (m_socket->bufferedAmount()) {
        m_shutdownAfterSend = true;
        return;
    }

    shutdownNow();
}

void WebSocketServerConnection::sendWebSocketMessage(const String& message)
{
    CString payload = message.utf8();
    const bool final = true, compress = false, masked = false;
    WebSocketFrame frame(WebSocketFrame::OpCodeText, final, compress, masked, payload.data(), payload.length());

    Vector<char> frameData;
    frame.makeFrameData(frameData);

    m_socket->send(frameData.data(), frameData.size());
}

void WebSocketServerConnection::sendHTTPResponseHeader(int statusCode, const String& statusText, const HTTPHeaderMap& headerFields)
{
    StringBuilder builder;
    builder.appendLiteral("HTTP/1.1 ");
    builder.appendNumber(statusCode);
    builder.append(' ');
    builder.append(statusText);
    builder.appendLiteral("\r\n");
    HTTPHeaderMap::const_iterator end = headerFields.end();
    for (HTTPHeaderMap::const_iterator it = headerFields.begin(); it != end; ++it) {
        builder.append(it->key);
        builder.appendLiteral(": ");
        builder.append(it->value);
        builder.appendLiteral("\r\n");
    }
    builder.appendLiteral("\r\n");

    CString header = builder.toString().latin1();
    m_socket->send(header.data(), header.length());
}

void WebSocketServerConnection::sendRawData(const char* data, size_t length)
{
    m_socket->send(data, length);
}

void WebSocketServerConnection::didCloseSocketStream(SocketStreamHandle*)
{
    // Destroy the SocketStreamHandle now to prevent closing an already closed socket later.
    m_socket.clear();

    // Web Socket Mode.
    if (m_mode == WebSocket)
        m_client->didCloseWebSocketConnection(this);

    // Tell the server to get rid of this.
    m_server->didCloseWebSocketServerConnection(this);
}

void WebSocketServerConnection::didReceiveSocketStreamData(SocketStreamHandle*, const char* data, int length)
{
    // Each didReceiveData call adds more data to our buffer.
    // We clear the buffer when we have handled data from it.
    m_bufferedData.append(data, length);

    switch (m_mode) {
    case HTTP:
        readHTTPMessage();
        break;
    case WebSocket:
        readWebSocketFrames();
        break;
    default:
        // For any new modes added in the future.
        ASSERT_NOT_REACHED();
    }
}

void WebSocketServerConnection::didUpdateBufferedAmount(WebCore::SocketStreamHandle*, size_t)
{
    if (m_shutdownAfterSend && !m_socket->bufferedAmount())
        shutdownNow();
}

void WebSocketServerConnection::didFailSocketStream(SocketStreamHandle*, const SocketStreamError&)
{
    // Possible read or write error.
}

void WebSocketServerConnection::readHTTPMessage()
{
    String failureReason;
    RefPtr<HTTPRequest> request = HTTPRequest::parseHTTPRequestFromBuffer(m_bufferedData.data(), m_bufferedData.size(), failureReason);
    if (!request)
        return;

    // Assume all the input has been read if we are reading an HTTP Request.
    m_bufferedData.clear();

    // If this is a WebSocket request, perform the WebSocket Handshake.
    const HTTPHeaderMap& headers = request->headerFields();
    String upgradeHeaderValue = headers.get("Upgrade");
    if (upgradeHeaderValue == "websocket") {
        upgradeToWebSocketServerConnection(request);
        return;
    }
    if (upgradeHeaderValue == "WebSocket") {
        LOG_ERROR("WebSocket protocol version < Hybi-10 not supported. Upgrade your client.");
        return;
    }

    // Otherwise, this is an HTTP Request we don't know how to deal with.
    m_client->didReceiveUnrecognizedHTTPRequest(this, request);
}

void WebSocketServerConnection::upgradeToWebSocketServerConnection(PassRefPtr<HTTPRequest> request)
{
    ASSERT(request);
    ASSERT(m_mode == HTTP);
    m_mode = WebSocket;
    RefPtr<HTTPRequest> protectedRequest(request);

    // Ask the client if we should upgrade for this or not.
    if (!m_client->didReceiveWebSocketUpgradeHTTPRequest(this, protectedRequest)) {
        shutdownNow();
        return;
    }

    // Build and send the WebSocket handshake response.
    const HTTPHeaderMap& requestHeaders = protectedRequest->headerFields();
    String accept = WebSocketHandshake::getExpectedWebSocketAccept(requestHeaders.get("Sec-WebSocket-Key"));
    HTTPHeaderMap responseHeaders;
    responseHeaders.add("Upgrade", requestHeaders.get("Upgrade"));
    responseHeaders.add("Connection", requestHeaders.get("Connection"));
    responseHeaders.add("Sec-WebSocket-Accept", accept);

    sendHTTPResponseHeader(101, "WebSocket Protocol Handshake", responseHeaders);

    m_client->didEstablishWebSocketConnection(this, protectedRequest);
}

void WebSocketServerConnection::readWebSocketFrames()
{
    while (true) {
        bool didReadOneFrame = readWebSocketFrame();
        if (!didReadOneFrame)
            break;
        if (m_bufferedData.isEmpty())
            break;
    }
}

bool WebSocketServerConnection::readWebSocketFrame()
{
    WebSocketFrame frame;
    const char* frameEnd;
    String errorString;
    WebSocketFrame::ParseFrameResult result = WebSocketFrame::parseFrame(m_bufferedData.data(), m_bufferedData.size(), frame, frameEnd, errorString);

    // Incomplete frame. Wait to receive more data.
    if (result == WebSocketFrame::FrameIncomplete)
        return false;

    if (result == WebSocketFrame::FrameError) {
        shutdownNow();
    } else if (frame.opCode == WebSocketFrame::OpCodeText) {
        // Delegate Text frames to our client.
        String msg = String::fromUTF8(frame.payload, frame.payloadLength);
        m_client->didReceiveWebSocketMessage(this, msg);
    } else
        notImplemented();

    // Remove the frame from our buffer.
    m_bufferedData.remove(0, frameEnd - m_bufferedData.data());

    return true;
}

}

#endif // ENABLE(INSPECTOR_SERVER)
