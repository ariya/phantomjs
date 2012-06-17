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

#ifndef QHTTPNETWORKCONNECTIONCHANNEL_H
#define QHTTPNETWORKCONNECTIONCHANNEL_H

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

#include <private/qobject_p.h>
#include <qauthenticator.h>
#include <qnetworkproxy.h>
#include <qbuffer.h>

#include <private/qhttpnetworkheader_p.h>
#include <private/qhttpnetworkrequest_p.h>
#include <private/qhttpnetworkreply_p.h>

#include <private/qhttpnetworkconnection_p.h>

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

#ifndef HttpMessagePair
typedef QPair<QHttpNetworkRequest, QHttpNetworkReply*> HttpMessagePair;
#endif

class QHttpNetworkConnectionChannel : public QObject {
    Q_OBJECT
public:
    enum ChannelState {
        IdleState = 0,          // ready to send request
        ConnectingState = 1,    // connecting to host
        WritingState = 2,       // writing the data
        WaitingState = 4,       // waiting for reply
        ReadingState = 8,       // reading the reply
        ClosingState = 16,
        BusyState = (ConnectingState|WritingState|WaitingState|ReadingState|ClosingState)
    };
    QAbstractSocket *socket;
    bool ssl;
    ChannelState state;
    QHttpNetworkRequest request; // current request
    QHttpNetworkReply *reply; // current reply for this request
    qint64 written;
    qint64 bytesTotal;
    bool resendCurrent;
    int lastStatus; // last status received on this channel
    bool pendingEncrypt; // for https (send after encrypted)
    int reconnectAttempts; // maximum 2 reconnection attempts
    QAuthenticatorPrivate::Method authMethod;
    QAuthenticatorPrivate::Method proxyAuthMethod;
    QAuthenticator authenticator;
    QAuthenticator proxyAuthenticator;
    bool authenticationCredentialsSent;
    bool proxyCredentialsSent;
#ifndef QT_NO_OPENSSL
    bool ignoreAllSslErrors;
    QList<QSslError> ignoreSslErrorsList;
#endif
#ifndef QT_NO_BEARERMANAGEMENT
    QSharedPointer<QNetworkSession> networkSession;
#endif

    // HTTP pipelining -> http://en.wikipedia.org/wiki/Http_pipelining
    enum PipeliningSupport {
        PipeliningSupportUnknown, // default for a new connection
        PipeliningProbablySupported, // after having received a server response that indicates support
        PipeliningNotSupported // currently not used
    };
    PipeliningSupport pipeliningSupported;
    QList<HttpMessagePair> alreadyPipelinedRequests;
    QByteArray pipeline; // temporary buffer that gets sent to socket in pipelineFlush
    void pipelineInto(HttpMessagePair &pair);
    void pipelineFlush();
    void requeueCurrentlyPipelinedRequests();
    void detectPipeliningSupport();

    QHttpNetworkConnectionChannel();
    
    void setConnection(QHttpNetworkConnection *c);
    QPointer<QHttpNetworkConnection> connection;

    void init();
    void close();

    bool sendRequest();

    bool ensureConnection();

    bool expand(bool dataComplete);
    void allDone(); // reply header + body have been read
    void handleStatus(); // called from allDone()

    bool resetUploadData(); // return true if resetting worked or there is no upload data

    void handleUnexpectedEOF();
    void closeAndResendCurrentRequest();

    bool isSocketBusy() const;
    bool isSocketWriting() const;
    bool isSocketWaiting() const;
    bool isSocketReading() const;

    friend class QNetworkAccessHttpBackend;

    protected slots:
    void _q_receiveReply();
    void _q_bytesWritten(qint64 bytes); // proceed sending
    void _q_readyRead(); // pending data to read
    void _q_disconnected(); // disconnected from host
    void _q_connected(); // start sending request
    void _q_error(QAbstractSocket::SocketError); // error from socket
#ifndef QT_NO_NETWORKPROXY
    void _q_proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *auth); // from transparent proxy
#endif

    void _q_uploadDataReadyRead();

#ifndef QT_NO_OPENSSL
    void _q_encrypted(); // start sending request (https)
    void _q_sslErrors(const QList<QSslError> &errors); // ssl errors from the socket
    void _q_encryptedBytesWritten(qint64 bytes); // proceed sending
#endif
};

QT_END_NAMESPACE

#endif // QT_NO_HTTP

#endif
