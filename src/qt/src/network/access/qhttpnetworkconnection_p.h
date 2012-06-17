/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
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

#include <private/qhttpnetworkheader_p.h>
#include <private/qhttpnetworkrequest_p.h>
#include <private/qhttpnetworkreply_p.h>

#include <private/qhttpnetworkconnectionchannel_p.h>

#ifndef QT_NO_HTTP

#ifndef QT_NO_OPENSSL
#    include <QtNetwork/qsslsocket.h>
#    include <QtNetwork/qsslerror.h>
#else
#   include <QtNetwork/qtcpsocket.h>
#endif

QT_BEGIN_NAMESPACE

class QHttpNetworkRequest;
class QHttpNetworkReply;
class QByteArray;

class QHttpNetworkConnectionPrivate;
class Q_AUTOTEST_EXPORT QHttpNetworkConnection : public QObject
{
    Q_OBJECT
public:

#ifndef QT_NO_BEARERMANAGEMENT
    QHttpNetworkConnection(const QString &hostName, quint16 port = 80, bool encrypt = false, QObject *parent = 0, QSharedPointer<QNetworkSession> networkSession = QSharedPointer<QNetworkSession>());
    QHttpNetworkConnection(quint16 channelCount, const QString &hostName, quint16 port = 80, bool encrypt = false, QObject *parent = 0, QSharedPointer<QNetworkSession> networkSession = QSharedPointer<QNetworkSession>());
#else
    QHttpNetworkConnection(const QString &hostName, quint16 port = 80, bool encrypt = false, QObject *parent = 0);
    QHttpNetworkConnection(quint16 channelCount, const QString &hostName, quint16 port = 80, bool encrypt = false, QObject *parent = 0);
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

#ifndef QT_NO_OPENSSL
    void setSslConfiguration(const QSslConfiguration &config);
    void ignoreSslErrors(int channel = -1);
    void ignoreSslErrors(const QList<QSslError> &errors, int channel = -1);
#endif

private:
    Q_DECLARE_PRIVATE(QHttpNetworkConnection)
    Q_DISABLE_COPY(QHttpNetworkConnection)
    friend class QHttpNetworkReply;
    friend class QHttpNetworkReplyPrivate;
    friend class QHttpNetworkConnectionChannel;

    Q_PRIVATE_SLOT(d_func(), void _q_startNextRequest())
};


// private classes
typedef QPair<QHttpNetworkRequest, QHttpNetworkReply*> HttpMessagePair;


class QHttpNetworkConnectionPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QHttpNetworkConnection)
public:
    static const int defaultChannelCount;
    static const int defaultPipelineLength;
    static const int defaultRePipelineLength;

    enum ConnectionState {
        RunningState = 0,
        PausedState = 1,
    };

    QHttpNetworkConnectionPrivate(const QString &hostName, quint16 port, bool encrypt);
    QHttpNetworkConnectionPrivate(quint16 channelCount, const QString &hostName, quint16 port, bool encrypt);
    ~QHttpNetworkConnectionPrivate();
    void init();

    void pauseConnection();
    void resumeConnection();
    ConnectionState state;

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

    // private slots
    void _q_startNextRequest(); // send the next request from the queue

    void createAuthorization(QAbstractSocket *socket, QHttpNetworkRequest &request);

    QString errorDetail(QNetworkReply::NetworkError errorCode, QAbstractSocket *socket,
                        const QString &extraDetail = QString());

#ifndef QT_NO_COMPRESS
    bool expand(QAbstractSocket *socket, QHttpNetworkReply *reply, bool dataComplete);
#endif
    void removeReply(QHttpNetworkReply *reply);

    QString hostName;
    quint16 port;
    bool encrypt;

    const int channelCount;
    QHttpNetworkConnectionChannel *channels; // parallel connections to the server

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

#ifndef QT_NO_BEARERMANAGEMENT
    QSharedPointer<QNetworkSession> networkSession;
#endif

    friend class QHttpNetworkConnectionChannel;
};



QT_END_NAMESPACE

#endif // QT_NO_HTTP

#endif
