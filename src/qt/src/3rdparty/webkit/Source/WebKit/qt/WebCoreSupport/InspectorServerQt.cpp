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
#include "MD5.h"
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
static void generateWebSocketChallengeResponse(uint32_t number1, uint32_t number2, const unsigned char key3[8], unsigned char response[16])
{
    uint8_t challenge[16];
    qToBigEndian<qint32>(number1, &challenge[0]);
    qToBigEndian<qint32>(number2, &challenge[4]);
    memcpy(&challenge[8], key3, 8);
    MD5 md5;
    md5.addBytes(challenge, sizeof(challenge));
    Vector<uint8_t, 16> digest;
    md5.checksum(digest);
    memcpy(response, digest.data(), 16);
}

/*!
    Parses and returns a WebSocket challenge number according to the
    method specified in the WebSocket protocol.

    The field contains numeric digits interspersed with spaces and
    non-numeric digits. The protocol ignores the characters that are
    neither digits nor spaces. The digits are concatenated and
    interpreted as a long int. The result is this number divided by
    the number of spaces.
 */
static quint32 parseWebSocketChallengeNumber(QString field)
{
    QString nString;
    int numSpaces = 0;
    for (int i = 0; i < field.size(); i++) {
        QChar c = field[i];
        if (c == QLatin1Char(' '))
            numSpaces++;
        else if ((c >= QLatin1Char('0')) && (c <= QLatin1Char('9')))
            nString.append(c);
    }
    quint32 num = nString.toLong();
    quint32 result = (numSpaces ? (num / numSpaces) : num);
    return result;
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
                if (header.hasKey(QLatin1String("Upgrade")) && (header.value(QLatin1String("Upgrade")) == QLatin1String("WebSocket")))
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
                connect(m_tcpConnection, SIGNAL(readyRead()), SLOT(webSocketReadyRead()));

                QByteArray key3 = m_tcpConnection->read(8);

                quint32 number1 = parseWebSocketChallengeNumber(header.value(QLatin1String("Sec-WebSocket-Key1")));
                quint32 number2 = parseWebSocketChallengeNumber(header.value(QLatin1String("Sec-WebSocket-Key2")));

                char responseData[16];
                generateWebSocketChallengeResponse(number1, number2, (unsigned char*)key3.data(), (unsigned char*)responseData);
                QByteArray response(responseData, sizeof(responseData));

                QHttpResponseHeader responseHeader(101, QLatin1String("WebSocket Protocol Handshake"), 1, 1);
                responseHeader.setValue(QLatin1String("Upgrade"), header.value(QLatin1String("Upgrade")));
                responseHeader.setValue(QLatin1String("Connection"), header.value(QLatin1String("Connection")));
                responseHeader.setValue(QLatin1String("Sec-WebSocket-Origin"), header.value(QLatin1String("Origin")));
                responseHeader.setValue(QLatin1String("Sec-WebSocket-Location"), (QLatin1String("ws://") + header.value(QLatin1String("Host")) + m_path));
                responseHeader.setContentLength(response.size());
                m_tcpConnection->write(responseHeader.toString().toLatin1());
                m_tcpConnection->write(response);
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

int InspectorServerRequestHandlerQt::webSocketSend(QByteArray payload)
{
    Q_ASSERT(m_tcpConnection);
    m_tcpConnection->putChar(0x00);
    int nBytes = m_tcpConnection->write(payload);
    m_tcpConnection->putChar(0xFF);
    m_tcpConnection->flush();
    return nBytes;
}

int InspectorServerRequestHandlerQt::webSocketSend(const char* data, size_t length)
{
    Q_ASSERT(m_tcpConnection);
    m_tcpConnection->putChar(0x00);
    int nBytes = m_tcpConnection->write(data, length);
    m_tcpConnection->putChar(0xFF);
    m_tcpConnection->flush();
    return nBytes;
}

void InspectorServerRequestHandlerQt::webSocketReadyRead()
{
    Q_ASSERT(m_tcpConnection);
    if (!m_tcpConnection->bytesAvailable())
        return;
    QByteArray content = m_tcpConnection->read(m_tcpConnection->bytesAvailable());
    m_data.append(content);
    while (m_data.size() > 0) {
        // first byte in websocket frame should be 0
        Q_ASSERT(!m_data[0]);

        // Start of WebSocket frame is indicated by 0
        if (m_data[0]) {
            qCritical() <<  "webSocketReadyRead: unknown frame type" << m_data[0];
            m_data.clear();
            m_tcpConnection->close();
            return;
        }

        // End of WebSocket frame indicated by 0xff.
        int pos = m_data.indexOf(0xff, 1);
        if (pos < 1)
            return;

        // After above checks, length will be >= 0.
        size_t length = pos - 1;
        if (length <= 0)
            return;

        QByteArray payload = m_data.mid(1, length);

#if ENABLE(INSPECTOR)
        if (m_inspectorClient) {
          InspectorController* inspectorController = m_inspectorClient->m_inspectedWebPage->d->page->inspectorController();
          inspectorController->dispatchMessageFromFrontend(QString::fromUtf8(payload));
        }
#endif

        // Remove this WebSocket message from m_data (payload, start-of-frame byte, end-of-frame byte).
        m_data = m_data.mid(length + 2);
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
