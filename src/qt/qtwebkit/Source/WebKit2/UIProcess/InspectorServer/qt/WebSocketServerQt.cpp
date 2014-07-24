/*
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */


#include "config.h"
#include "WebSocketServerQt.h"

#include "WebSocketServer.h"
#include "WebSocketServerConnection.h"
#include <WebCore/SocketStreamHandle.h>
#include <wtf/PassOwnPtr.h>

using namespace WebCore;

namespace WebKit {

void WebSocketServer::platformInitialize()
{
    m_tcpServerHandler = adoptPtr(new QtTcpServerHandler(this));
}

bool WebSocketServer::platformListen(const String& bindAddress, unsigned short port)
{
    return m_tcpServerHandler->listen(bindAddress, port);
}

void WebSocketServer::platformClose()
{
    m_tcpServerHandler->close();
}

QtTcpServerHandler::QtTcpServerHandler(WebSocketServer* webSocketServer)
: m_webSocketServer(webSocketServer)
{
    connect(&m_serverSocket, SIGNAL(newConnection()), SLOT(handleNewConnection()));
}

void QtTcpServerHandler::handleNewConnection()
{
    QTcpSocket* socket = m_serverSocket.nextPendingConnection();
    ASSERT(socket);
    OwnPtr<WebSocketServerConnection> conection = adoptPtr(new WebSocketServerConnection(m_webSocketServer->client(), m_webSocketServer));
    conection->setSocketHandle(SocketStreamHandle::create(socket, conection.get()));
    m_webSocketServer->didAcceptConnection(conection.release());
}

bool QtTcpServerHandler::listen(const String& bindAddress, unsigned short port)
{
    ASSERT(!bindAddress.isEmpty());
    ASSERT(port);
    bool success = m_serverSocket.listen(QHostAddress(bindAddress), port);
    if (!success)
        LOG_ERROR("Can't open server socket on %s:%d [%s]", qPrintable(bindAddress), port, qPrintable(m_serverSocket.errorString()));
    return success;
}

void QtTcpServerHandler::close()
{
    m_serverSocket.close();
}

}

#include "moc_WebSocketServerQt.cpp"
