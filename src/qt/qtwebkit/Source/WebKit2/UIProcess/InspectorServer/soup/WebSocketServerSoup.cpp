/*
 * Copyright (C) 2012 Samsung Electronics Ltd. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#if ENABLE(INSPECTOR_SERVER)
#include "WebSocketServer.h"

#include "Logging.h"
#include "WebSocketServerConnection.h"
#include <WebCore/SocketStreamHandle.h>
#include <gio/gio.h>
#include <glib.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>

using namespace WebCore;

namespace WebKit {

static gboolean connectionCallback(GSocketService* /*service*/, GSocketConnection* connection, GObject* /*sourceObject*/, WebSocketServer* server)
{
#if !LOG_DISABLED
    GRefPtr<GSocketAddress> socketAddress = adoptGRef(g_socket_connection_get_remote_address(connection, 0));
    GOwnPtr<gchar> addressString(g_inet_address_to_string(g_inet_socket_address_get_address(G_INET_SOCKET_ADDRESS(socketAddress.get()))));
    LOG(InspectorServer, "New Connection from %s:%d.", addressString.get(), g_inet_socket_address_get_port(G_INET_SOCKET_ADDRESS(socketAddress.get())));
#endif

    OwnPtr<WebSocketServerConnection> webSocketConnection = adoptPtr(new WebSocketServerConnection(server->client(), server));
    webSocketConnection->setSocketHandle(SocketStreamHandle::create(connection, webSocketConnection.get()));
    server->didAcceptConnection(webSocketConnection.release());

    return TRUE;
}

void WebSocketServer::platformInitialize()
{
    m_socketService = adoptGRef(g_socket_service_new());
    g_signal_connect(m_socketService.get(), "incoming", G_CALLBACK(connectionCallback), this);
    g_socket_service_start(m_socketService.get());
}

bool WebSocketServer::platformListen(const String& bindAddress, unsigned short port)
{
    LOG(InspectorServer, "Listen to address=%s, port=%d.", bindAddress.utf8().data(), port);
    GRefPtr<GInetAddress> address = adoptGRef(g_inet_address_new_from_string(bindAddress.utf8().data()));
    GRefPtr<GSocketAddress> socketAddress = adoptGRef(g_inet_socket_address_new(address.get(), port));
    return g_socket_listener_add_address(G_SOCKET_LISTENER(m_socketService.get()), socketAddress.get(), G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP, 0, 0, 0);
}

void WebSocketServer::platformClose()
{
    g_socket_service_stop(m_socketService.get());
}

}

#endif // ENABLE(INSPECTOR_SERVER)
