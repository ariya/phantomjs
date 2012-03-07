/*
  Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "InspectorServerQt.h"

#include "InspectorClientQt.h"
#include "InspectorController.h"
#include "Base64.h"
#include "SHA1.h"
#include "Page.h"
#include "qwebpage.h"
#include "qwebpage_p.h"
#include <QFile>
#include <QHttpHeader>
#include <QHttpRequestHeader>
#include <QHttpResponseHeader>
#include <QString>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>
#include <QWidget>
#include <qendian.h>
#include <wtf/text/CString.h>

namespace WebCore {

/*!
    Computes the WebSocket handshake response given the two challenge numbers and key3.
 */
static QByteArray generateWebSocketChallengeResponse(const QByteArray& key)
{
    SHA1 sha1;
    Vector<uint8_t, 20> digest;
    Vector<char> encoded;
    QByteArray toHash("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    toHash.prepend(key);
    sha1.addBytes((uint8_t*)toHash.data(), toHash.size());
    sha1.computeHash(digest);
    base64Encode((char*)digest.data(), digest.size(), encoded);
    return QByteArray(encoded.data(), encoded.size());
}

static InspectorServerQt* s_inspectorServer;

InspectorServerQt* InspectorServerQt::server()
{
    // s_inspectorServer is deleted in unregisterClient() when the last client is unregistered.
    if (!s_inspectorServer)
        s_inspectorServer = new InspectorServerQt();

    return s_inspectorServer;
}

InspectorServerQt::InspectorServerQt()
    : QObject()
    , m_tcpServer(0)
    , m_pageNumber(1)
{
}

InspectorServerQt::~InspectorServerQt()
{
    close();
}

void InspectorServerQt::listen(quint16 port)
{
    if (m_tcpServer)
        return;

    m_tcpServer = new QTcpServer();
    m_tcpServer->listen(QHostAddress::Any, port);
    connect(m_tcpServer, SIGNAL(newConnection()), SLOT(newConnection()));
}

void InspectorServerQt::close()
{
    if (m_tcpServer) {
        m_tcpServer->close();
        delete m_tcpServer;
    }
    m_tcpServer = 0;
}

InspectorClientQt* InspectorServerQt::inspectorClientForPage(int pageNum)
{
    InspectorClientQt* client = m_inspectorClients.value(pageNum);
    return client;
}

void InspectorServerQt::registerClient(InspectorClientQt* client)
{
    if (!m_inspectorClients.key(client))
        m_inspectorClients.insert(m_pageNumber++, client);
}

void InspectorServerQt::unregisterClient(InspectorClientQt* client)
{
    int pageNum = m_inspectorClients.key(client, -1);
    if (pageNum >= 0)
        m_inspectorClients.remove(pageNum);
    if (!m_inspectorClients.size()) {
        // s_inspectorServer points to this.
        s_inspectorServer = 0;
        close();
        deleteLater();
    }
}

void InspectorServerQt::newConnection()
{
    QTcpSocket* tcpConnection = m_tcpServer->nextPendingConnection();
    InspectorServerRequestHandlerQt* handler = new InspectorServerRequestHandlerQt(tcpConnection, this);
    handler->setParent(this);
}

InspectorServerRequestHandlerQt::InspectorServerRequestHandlerQt(QTcpSocket* tcpConnection, InspectorServerQt* server)
    : QObject(server)
    , m_tcpConnection(tcpConnection)
    , m_server(server)
    , m_inspectorClient(0)
{
    m_endOfHeaders = false;    
    m_contentLength = 0;

    connect(m_tcpConnection, SIGNAL(readyRead()), SLOT(tcpReadyRead()));
    connect(m_tcpConnection, SIGNAL(disconnected()), SLOT(tcpConnectionDisconnected()));
}

InspectorServerRequestHandlerQt::~InspectorServerRequestHandlerQt()
{
}

void InspectorServerRequestHandlerQt::tcpReadyRead()
{
    QHttpRequestHeader header;
    bool isWebSocket = false;
    if (!m_tcpConnection)
        return;

    if (!m_endOfHeaders) {
        while (m_tcpConnection->bytesAvailable() && !m_endOfHeaders) {
            QByteArray line = m_tcpConnection->readLine();
            m_data.append(line);
            if (line == "\r\n")
                m_endOfHeaders = true;
        }
        if (m_endOfHeaders) {
            header = QHttpRequestHeader(QString::fromLatin1(m_data));
            if (header.isValid()) {
                m_path = header.path();
                m_contentType = header.contentType().toLatin1();
                m_contentLength = header.contentLength();
                if (header.hasKey(QLatin1String("Upgrade")) && (header.value(QLatin1String("Upgrade")) == QLatin1String("websocket")))
                    isWebSocket = true;

                m_data.clear();
            }
        }
    }

    if (m_endOfHeaders) {
        QStringList pathAndQuery = m_path.split(QLatin1Char('?'));
        m_path = pathAndQuery[0];
        QStringList words = m_path.split(QLatin1Char('/'));

        if (isWebSocket) {
            // switch to websocket-style WebSocketService messaging
            if (m_tcpConnection) {
                m_tcpConnection->disconnect(SIGNAL(readyRead()));
                connect(m_tcpConnection, SIGNAL(readyRead()), SLOT(webSocketReadyRead()), Qt::QueuedConnection);
                
                QByteArray key = header.value(QLatin1String("Sec-WebSocket-Key")).toLatin1();
                QString accept = QString::fromLatin1(generateWebSocketChallengeResponse(key));

                QHttpResponseHeader responseHeader(101, QLatin1String("WebSocket Protocol Handshake"), 1, 1);
                responseHeader.setValue(QLatin1String("Upgrade"), header.value(QLatin1String("Upgrade")));
                responseHeader.setValue(QLatin1String("Connection"), header.value(QLatin1String("Connection")));
                responseHeader.setValue(QLatin1String("Sec-WebSocket-Accept"), accept);
                // responseHeader.setValue(QLatin1String("Sec-WebSocket-Origin"), header.value(QLatin1String("Sec-WebSocket-Origin")));
                // responseHeader.setValue(QLatin1String("Sec-WebSocket-Location"), (QLatin1String("ws://") + header.value(QLatin1String("Host")) + m_path));
                m_tcpConnection->write(responseHeader.toString().toLatin1());
                //m_tcpConnection->write(response);
                m_tcpConnection->flush();

                if ((words.size() == 4)
                    && (words[1] == QString::fromLatin1("devtools"))
                    && (words[2] == QString::fromLatin1("page"))) {
                    int pageNum = words[3].toInt();

                    m_inspectorClient = m_server->inspectorClientForPage(pageNum);
                    // Attach remoteFrontendChannel to inspector, also transferring ownership.
                    if (m_inspectorClient)
                        m_inspectorClient->attachAndReplaceRemoteFrontend(new RemoteFrontendChannel(this));
                }

            }

            return;
        }
        if (m_contentLength && (m_tcpConnection->bytesAvailable() < m_contentLength))
            return;

        QByteArray content = m_tcpConnection->read(m_contentLength);
        m_endOfHeaders = false;

        QByteArray response;
        int code = 200;
        QString text = QString::fromLatin1("OK");

        // If no path is specified, generate an index page.
        if (m_path.isEmpty() || (m_path == QString(QLatin1Char('/')))) {
            QString indexHtml = QLatin1String("<html><head><title>Remote Web Inspector</title></head><body><ul>\n");
            for (QMap<int, InspectorClientQt* >::const_iterator it = m_server->m_inspectorClients.begin();
                 it != m_server->m_inspectorClients.end(); 
                 ++it) {
                indexHtml.append(QString::fromLatin1("<li><a href=\"/webkit/inspector/inspector.html?page=%1\">%2</li>\n")
                                 .arg(it.key())
                                 .arg(it.value()->m_inspectedWebPage->mainFrame()->url().toString()));
            }
            indexHtml.append(QLatin1String("</ul></body></html>"));
            response = indexHtml.toLatin1();
        } else {
            QString path = QString::fromLatin1(":%1").arg(m_path);
            QFile file(path);
            // It seems that there should be an enum or define for these status codes somewhere in Qt or WebKit,
            // but grep fails to turn one up.
            // QNetwork uses the numeric values directly.
            if (file.exists()) {
                file.open(QIODevice::ReadOnly);
                response = file.readAll();
            } else {
                code = 404;
                text = QString::fromLatin1("Not OK");
            }
        }

        QHttpResponseHeader responseHeader(code, text, 1, 0);
        responseHeader.setContentLength(response.size());
        if (!m_contentType.isEmpty())
            responseHeader.setContentType(QString::fromLatin1(m_contentType));

        QByteArray asciiHeader = responseHeader.toString().toAscii();
        m_tcpConnection->write(asciiHeader);

        m_tcpConnection->write(response);
        m_tcpConnection->flush();
        m_tcpConnection->close();

        return;
    }
}

void InspectorServerRequestHandlerQt::tcpConnectionDisconnected()
{
    if (m_inspectorClient)
        m_inspectorClient->detachRemoteFrontend();
    m_tcpConnection->deleteLater();
    m_tcpConnection = 0;
}

int InspectorServerRequestHandlerQt::webSocketSend(const QString& message)
{
    QByteArray payload = message.toUtf8();
    return webSocketSend(payload.data(), payload.size());
}

int InspectorServerRequestHandlerQt::webSocketSend(const char* data, size_t length)
{
    Q_ASSERT(m_tcpConnection);
    m_tcpConnection->putChar(0x81);
    if (length <= 125)
        m_tcpConnection->putChar(static_cast<uint8_t>(length));
    else if (length <= pow(2,16)) {
        m_tcpConnection->putChar(126);
        quint16 length16 = qToBigEndian<quint16>(static_cast<quint16>(length));
        m_tcpConnection->write(reinterpret_cast<char*>(&length16), 2);
    } else {
        m_tcpConnection->putChar(127);
        quint64 length64 = qToBigEndian<quint64>(static_cast<quint64>(length));
        m_tcpConnection->write(reinterpret_cast<char*>(&length64), 8);
    }
    int nBytes = m_tcpConnection->write(data, length);
    m_tcpConnection->flush();
    return nBytes;
}

static QByteArray applyMask(const QByteArray& payload, const QByteArray& maskingKey)
{
    Q_ASSERT(maskingKey.size() == 4);
    QByteArray unmaskedPayload;
    for (int i = 0; i < payload.size(); ++i) {
        char unmaskedByte = payload[i] ^ maskingKey[i % 4];
        unmaskedPayload.append(unmaskedByte);
    }
    return unmaskedPayload;
}

#define BYTETOBINARYPATTERN "%d%d%d%d%d%d%d%d"
#define BYTETOBINARY(byte)  \
  (byte & 0x80 ? 1 : 0), \
  (byte & 0x40 ? 1 : 0), \
  (byte & 0x20 ? 1 : 0), \
  (byte & 0x10 ? 1 : 0), \
  (byte & 0x08 ? 1 : 0), \
  (byte & 0x04 ? 1 : 0), \
  (byte & 0x02 ? 1 : 0), \
  (byte & 0x01 ? 1 : 0) 
  
void InspectorServerRequestHandlerQt::webSocketReadyRead()
{
    Q_ASSERT(m_tcpConnection);
    if (!m_tcpConnection->bytesAvailable())
        return;
    QByteArray content = m_tcpConnection->read(m_tcpConnection->bytesAvailable());
    m_data.append(content);
    while (m_data.size() > 0) {
        bool isMasked = m_data[1] & 0x80;
        quint64 payloadLen = m_data[1] & 0x7F;
        int pos = 2;

        if (payloadLen == 126) {
            payloadLen = qFromBigEndian<quint16>(*reinterpret_cast<quint16*>(m_data.mid(pos, 2).data()));
            pos = 4;
        } else if (payloadLen == 127) {
            payloadLen = qFromBigEndian<quint64>(*reinterpret_cast<quint64*>(m_data.mid(pos, 8).data()));
            pos = 8;
        }

        QByteArray payload;
        if (isMasked) {
            QByteArray maskingKey = m_data.mid(pos, 4);
            pos += 4;
            payload = applyMask(m_data.mid(pos, payloadLen), maskingKey);
        } else {
            payload = m_data.mid(pos, payloadLen);
        }

        // Handle fragmentation
        if ((m_data[0] & 0x80) == 0) { // Non-last fragmented payload
            m_fragmentedPayload.append(payload);
            
            m_data = m_data.mid(pos + payloadLen);
            continue;
        } else {
            if ((m_data[0] & 0x0F) == 0) { // Last fragment
                 m_fragmentedPayload.append(payload);   
                 payload = m_fragmentedPayload;
                 m_fragmentedPayload.clear();
            }
        }
        
        // Remove this WebSocket message from m_data (payload, start-of-frame byte, end-of-frame byte).
        m_data = m_data.mid(pos + payloadLen);
#if ENABLE(INSPECTOR)
        if (m_inspectorClient) {
          InspectorController* inspectorController = m_inspectorClient->m_inspectedWebPage->d->page->inspectorController();
          inspectorController->dispatchMessageFromFrontend(QString::fromUtf8(payload));
        }
#endif
    }
}

RemoteFrontendChannel::RemoteFrontendChannel(InspectorServerRequestHandlerQt* requestHandler)
    : QObject(requestHandler)
    , m_requestHandler(requestHandler)
{
}

bool RemoteFrontendChannel::sendMessageToFrontend(const String& message)
{
    if (!m_requestHandler)
        return false;
    CString cstr = message.utf8();
    return m_requestHandler->webSocketSend(cstr.data(), cstr.length());
}

}
