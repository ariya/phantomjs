/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QHTTPNETWORKCONNECTION_H
#define QHTTPNETWORKCONNECTION_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/qabstractsocket.h>
#include <QtNetwork/qnetworksession.h>

#include <private/qobject_p.h>
#include <qauthenticator.h>
#include <qnetworkproxy.h>
#include <qbuffer.h>
#include <qtimer.h>
#include <qsharedpointer.h>

#include <private/qhttpnetworkheader_p.h>
#include <private/qhttpnetworkrequest_p.h>
#include <private/qhttpnetworkreply_p.h>

#include <private/qhttpnetworkconnectionchannel_p.h>

#ifndef QT_NO_HTTP

#ifndef QT_NO_SSL
#ifndef QT_NO_OPENSSL
#    include <private/qsslcontext_openssl_p.h>
#endif
#    include <private/qsslsocket_p.h>
#    include <QtNetwork/qsslsocket.h>
#    include <QtNetwork/qsslerror.h>
#else
#   include <QtNetwork/qtcpsocket.h>
#endif

QT_BEGIN_NAMESPACE

class QHttpNetworkRequest;
class QHttpNetworkReply;
class QByteArray;
class QHostInfo;

class QHttpNetworkConnectionPrivate;
class Q_AUTOTEST_EXPORT QHttpNetworkConnection : public QObject
{
    Q_OBJECT
public:

    enum ConnectionType {
        ConnectionTypeHTTP,
        ConnectionTypeSPDY
    };

#ifndef QT_NO_BEARERMANAGEMENT
    explicit QHttpNetworkConnection(const QString &hostName, quint16 port = 80, bool encrypt = false,
                                    ConnectionType connectionType = ConnectionTypeHTTP,
                                    QObject *parent = 0, QSharedPointer<QNetworkSession> networkSession
                                    = QSharedPointer<QNetworkSession>());
    QHttpNetworkConnection(quint16 channelCount, const QString &hostName, quint16 port = 80,
                           bool encrypt = false, QObject *parent = 0,
                           QSharedPointer<QNetworkSession> networkSession = QSharedPointer<QNetworkSession>(),
                           ConnectionType connectionType = ConnectionTypeHTTP);
#else
    explicit QHttpNetworkConnection(const QString &hostName, quint16 port = 80, bool encrypt = false,
                                    ConnectionType connectionType = ConnectionTypeHTTP,
                                    QObject *parent = 0);
    QHttpNetworkConnection(quint16 channelCount, const QString &hostName, quint16 port = 80,
                           bool encrypt = false, QObject *parent = 0,
                           ConnectionType connectionType = ConnectionTypeHTTP);
#endif
    ~QHttpNetworkConnection();

    //The hostname to which this is connected to.
    QString hostName() const;
    //The HTTP port in use.
    quint16 port() const;

    //add a new HTTP request through this connection
    QHttpNetworkReply* sendRequest(const QHttpNetworkRequest &request);

#ifndef QT_NO_NETWORKPROXY
    //set the proxy for this connection
    void setCacheProxy(const QNetworkProxy &networkProxy);
    QNetworkProxy cacheProxy() const;
    void setTransparentProxy(const QNetworkProxy &networkProxy);
    QNetworkProxy transparentProxy() const;
#endif

    bool isSsl() const;

    QHttpNetworkConnectionChannel *channels() const;

    ConnectionType connectionType();
    void setConnectionType(ConnectionType type);

#ifndef QT_NO_SSL
    void setSslConfiguration(const QSslConfiguration &config);
    void ignoreSslErrors(int channel = -1);
    void ignoreSslErrors(const QList<QSslError> &errors, int channel = -1);
    QSharedPointer<QSslContext> sslContext();
    void setSslContext(QSharedPointer<QSslContext> context);
#endif

    void preConnectFinished();

private:
    Q_DECLARE_PRIVATE(QHttpNetworkConnection)
    Q_DISABLE_COPY(QHttpNetworkConnection)
    friend class QHttpNetworkReply;
    friend class QHttpNetworkReplyPrivate;
    friend class QHttpNetworkConnectionChannel;
    friend class QHttpProtocolHandler;
    friend class QSpdyProtocolHandler;

    Q_PRIVATE_SLOT(d_func(), void _q_startNextRequest())
    Q_PRIVATE_SLOT(d_func(), void _q_hostLookupFinished(QHostInfo))
    Q_PRIVATE_SLOT(d_func(), void _q_connectDelayedChannel())
};


// private classes
typedef QPair<QHttpNetworkRequest, QHttpNetworkReply*> HttpMessagePair;


class QHttpNetworkConnectionPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QHttpNetworkConnection)
public:
    static const int defaultHttpChannelCount;
    static const int defaultPipelineLength;
    static const int defaultRePipelineLength;

    enum ConnectionState {
        RunningState = 0,
        PausedState = 1
    };

    enum NetworkLayerPreferenceState {
        Unknown,
        HostLookupPending,
        IPv4,
        IPv6,
        IPv4or6
    };

    QHttpNetworkConnectionPrivate(const QString &hostName, quint16 port, bool encrypt,
                                  QHttpNetworkConnection::ConnectionType type);
    QHttpNetworkConnectionPrivate(quint16 channelCount, const QString &hostName, quint16 port, bool encrypt,
                                  QHttpNetworkConnection::ConnectionType type);
    ~QHttpNetworkConnectionPrivate();
    void init();

    void pauseConnection();
    void resumeConnection();
    ConnectionState state;
    NetworkLayerPreferenceState networkLayerState;

    enum { ChunkSize = 4096 };

    int indexOf(QAbstractSocket *socket) const;

    QHttpNetworkReply *queueRequest(const QHttpNetworkRequest &request);
    void requeueRequest(const HttpMessagePair &pair); // e.g. after pipeline broke
    bool dequeueRequest(QAbstractSocket *socket);
    void prepareRequest(HttpMessagePair &request);
    QHttpNetworkRequest predictNextRequest();

    void fillPipeline(QAbstractSocket *socket);
    bool fillPipeline(QList<HttpMessagePair> &queue, QHttpNetworkConnectionChannel &channel);

    // read more HTTP body after the next event loop spin
    void readMoreLater(QHttpNetworkReply *reply);

    void copyCredentials(int fromChannel, QAuthenticator *auth, bool isProxy);

    void startHostInfoLookup();
    void startNetworkLayerStateLookup();
    void networkLayerDetected(QAbstractSocket::NetworkLayerProtocol protocol);

    // private slots
    void _q_startNextRequest(); // send the next request from the queue

    void _q_hostLookupFinished(QHostInfo info);
    void _q_connectDelayedChannel();

    void createAuthorization(QAbstractSocket *socket, QHttpNetworkRequest &request);

    QString errorDetail(QNetworkReply::NetworkError errorCode, QAbstractSocket *socket,
                        const QString &extraDetail = QString());

    void removeReply(QHttpNetworkReply *reply);

    QString hostName;
    quint16 port;
    bool encrypt;
    bool delayIpv4;

    const int channelCount;
    QTimer delayedConnectionTimer;
    QHttpNetworkConnectionChannel *channels; // parallel connections to the server
    bool shouldEmitChannelError(QAbstractSocket *socket);

    qint64 uncompressedBytesAvailable(const QHttpNetworkReply &reply) const;
    qint64 uncompressedBytesAvailableNextBlock(const QHttpNetworkReply &reply) const;


    void emitReplyError(QAbstractSocket *socket, QHttpNetworkReply *reply, QNetworkReply::NetworkError errorCode);
    bool handleAuthenticateChallenge(QAbstractSocket *socket, QHttpNetworkReply *reply, bool isProxy, bool &resend);

#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy networkProxy;
    void emitProxyAuthenticationRequired(const QHttpNetworkConnectionChannel *chan, const QNetworkProxy &proxy, QAuthenticator* auth);
#endif

    //The request queues
    QList<HttpMessagePair> highPriorityQueue;
    QList<HttpMessagePair> lowPriorityQueue;

    int preConnectRequests;

    QHttpNetworkConnection::ConnectionType connectionType;

#ifndef QT_NO_SSL
    QSharedPointer<QSslContext> sslContext;
#endif

#ifndef QT_NO_BEARERMANAGEMENT
    QSharedPointer<QNetworkSession> networkSession;
#endif

    friend class QHttpNetworkConnectionChannel;
};



QT_END_NAMESPACE

#endif // QT_NO_HTTP

#endif
