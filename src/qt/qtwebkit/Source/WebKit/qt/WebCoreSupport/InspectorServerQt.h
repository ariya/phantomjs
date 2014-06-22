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

#ifndef InspectorServerQt_h
#define InspectorServerQt_h

#include <QList>
#include <QMap>
#include <QObject>
#include <QPair>
#include <QString>
#include <wtf/Forward.h>

QT_BEGIN_NAMESPACE
class QTcpServer;
class QTcpSocket;
QT_END_NAMESPACE
class QWebPage;

namespace WebCore {
class InspectorServerRequestHandlerQt;
class InspectorClientQt;

class InspectorServerQt : public QObject {
    Q_OBJECT
public:

    static InspectorServerQt* server();

    void listen(quint16 port);

    void registerClient(InspectorClientQt*);
    void unregisterClient(InspectorClientQt*);

    void close();
    InspectorClientQt* inspectorClientForPage(int pageNum);

protected:
    InspectorServerQt();
    virtual ~InspectorServerQt();

private Q_SLOTS:
    void newConnection();

private:
    QTcpServer* m_tcpServer;
    QMap<int, InspectorClientQt*> m_inspectorClients;
    int m_pageNumber;

    friend class InspectorServerRequestHandlerQt;
};

class InspectorServerRequestHandlerQt : public QObject {
    Q_OBJECT
public:

    InspectorServerRequestHandlerQt(QTcpSocket* tcpConnection, InspectorServerQt*);
    virtual ~InspectorServerRequestHandlerQt();
    virtual int webSocketSend(const QString& message);
    virtual int webSocketSend(const char *payload, size_t length);

private Q_SLOTS:
    void tcpReadyRead();
    void tcpConnectionDisconnected();
    void webSocketReadyRead();

private:
    QTcpSocket* m_tcpConnection;
    InspectorServerQt* m_server;

    QString m_path;
    QByteArray m_contentType;
    int m_contentLength;
    bool m_endOfHeaders;
    QByteArray m_data;
    QByteArray m_fragmentedPayload;
    InspectorClientQt* m_inspectorClient;

    void handleInspectorRequest(QStringList words);
    void handleFromFrontendRequest();
    void handleResourceRequest(QStringList words);

};

}
#endif
