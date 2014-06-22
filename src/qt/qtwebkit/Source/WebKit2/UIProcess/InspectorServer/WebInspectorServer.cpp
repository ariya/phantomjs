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

#include "WebInspectorServer.h"

#include "HTTPRequest.h"
#include "WebInspectorProxy.h"
#include "WebSocketServerConnection.h"

using namespace WebCore;

namespace WebKit {

static unsigned pageIdFromRequestPath(const String& path)
{
    size_t start = path.reverseFind('/');
    String numberString = path.substring(start + 1, path.length() - start - 1);

    bool ok = false;
    unsigned number = numberString.toUIntStrict(&ok);
    if (!ok)
        return 0;
    return number;
}

WebInspectorServer& WebInspectorServer::shared()
{
    static WebInspectorServer& server = *new WebInspectorServer;
    return server;
}

WebInspectorServer::WebInspectorServer()
    : WebSocketServer(this)
    , m_nextAvailablePageId(1)
{
}

WebInspectorServer::~WebInspectorServer()
{
    // Close any remaining open connections.
    HashMap<unsigned, WebSocketServerConnection*>::iterator end = m_connectionMap.end();
    for (HashMap<unsigned, WebSocketServerConnection*>::iterator it = m_connectionMap.begin(); it != end; ++it) {
        WebSocketServerConnection* connection = it->value;
        WebInspectorProxy* client = m_clientMap.get(connection->identifier());
        closeConnection(client, connection);
    }
}

int WebInspectorServer::registerPage(WebInspectorProxy* client)
{
#ifndef ASSERT_DISABLED
    ClientMap::iterator end = m_clientMap.end();
    for (ClientMap::iterator it = m_clientMap.begin(); it != end; ++it)
        ASSERT(it->value != client);
#endif

    int pageId = m_nextAvailablePageId++;
    m_clientMap.set(pageId, client);
    return pageId;
}

void WebInspectorServer::unregisterPage(int pageId)
{
    m_clientMap.remove(pageId);
    WebSocketServerConnection* connection = m_connectionMap.get(pageId);
    if (connection)
        closeConnection(0, connection);
}

#if !PLATFORM(QT)
String WebInspectorServer::inspectorUrlForPageID(int)
{
    return String();
}
#endif

void WebInspectorServer::sendMessageOverConnection(unsigned pageIdForConnection, const String& message)
{
    WebSocketServerConnection* connection = m_connectionMap.get(pageIdForConnection);
    if (connection)
        connection->sendWebSocketMessage(message);
}

void WebInspectorServer::didReceiveUnrecognizedHTTPRequest(WebSocketServerConnection* connection, PassRefPtr<HTTPRequest> request)
{
    // request->url() contains only the path extracted from the HTTP request line
    // and KURL is poor at parsing incomplete URLs, so extract the interesting parts manually.
    String path = request->url();
    size_t pathEnd = path.find('?');
    if (pathEnd == notFound)
        pathEnd = path.find('#');
    if (pathEnd != notFound)
        path.truncate(pathEnd);

    // Ask for the complete payload in memory for the sake of simplicity. A more efficient way would be
    // to ask for header data and then let the platform abstraction write the payload straight on the connection.
    Vector<char> body;
    String contentType;
    bool found = platformResourceForPath(path, body, contentType);

    HTTPHeaderMap headerFields;
    headerFields.set("Connection", "close");
    headerFields.set("Content-Length", String::number(body.size()));
    if (found)
        headerFields.set("Content-Type", contentType);

    // Send when ready and close immediately afterwards.
    connection->sendHTTPResponseHeader(found ? 200 : 404, found ? "OK" : "Not Found", headerFields);
    connection->sendRawData(body.data(), body.size());
    connection->shutdownAfterSendOrNow();
}

bool WebInspectorServer::didReceiveWebSocketUpgradeHTTPRequest(WebSocketServerConnection*, PassRefPtr<HTTPRequest> request)
{
    String path = request->url();

    // NOTE: Keep this in sync with WebCore/inspector/front-end/inspector.js.
    DEFINE_STATIC_LOCAL(const String, inspectorWebSocketConnectionPathPrefix, (ASCIILiteral("/devtools/page/")));

    // Unknown path requested.
    if (!path.startsWith(inspectorWebSocketConnectionPathPrefix))
        return false;

    int pageId = pageIdFromRequestPath(path);
    // Invalid page id.
    if (!pageId)
        return false;

    // There is no client for that page id.
    WebInspectorProxy* client = m_clientMap.get(pageId);
    if (!client)
        return false;

    return true;
}

void WebInspectorServer::didEstablishWebSocketConnection(WebSocketServerConnection* connection, PassRefPtr<HTTPRequest> request)
{
    String path = request->url();
    unsigned pageId = pageIdFromRequestPath(path);
    ASSERT(pageId);

    // Ignore connections to a page that already have a remote inspector connected.
    if (m_connectionMap.contains(pageId)) {
        LOG_ERROR("A remote inspector connection already exist for page ID %d. Ignoring.", pageId);
        connection->shutdownNow();
        return;
    }

    // Map the pageId to the connection in case we need to close the connection locally.
    connection->setIdentifier(pageId);
    m_connectionMap.set(pageId, connection);

    WebInspectorProxy* client = m_clientMap.get(pageId);
    client->remoteFrontendConnected();
}

void WebInspectorServer::didReceiveWebSocketMessage(WebSocketServerConnection* connection, const String& message)
{
    // Dispatch incoming remote message locally.
    unsigned pageId = connection->identifier();
    ASSERT(pageId);
    WebInspectorProxy* client = m_clientMap.get(pageId);
    client->dispatchMessageFromRemoteFrontend(message);
}

void WebInspectorServer::didCloseWebSocketConnection(WebSocketServerConnection* connection)
{
    // Connection has already shut down.
    unsigned pageId = connection->identifier();
    if (!pageId)
        return;

    // The socket closing means the remote side has caused the close.
    WebInspectorProxy* client = m_clientMap.get(pageId);
    closeConnection(client, connection);
}

void WebInspectorServer::closeConnection(WebInspectorProxy* client, WebSocketServerConnection* connection)
{
    // Local side cleanup.
    if (client)
        client->remoteFrontendDisconnected();

    // Remote side cleanup.
    m_connectionMap.remove(connection->identifier());
    connection->setIdentifier(0);
    connection->shutdownNow();
}

}

#endif // ENABLE(INSPECTOR_SERVER)
