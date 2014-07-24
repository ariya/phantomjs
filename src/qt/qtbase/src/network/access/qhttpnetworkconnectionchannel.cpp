/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qhttpnetworkconnectionchannel_p.h"
#include "qhttpnetworkconnection_p.h"
#include "private/qnoncontiguousbytedevice_p.h"

#include <qpair.h>
#include <qdebug.h>

#ifndef QT_NO_HTTP

#include <private/qhttpprotocolhandler_p.h>
#include <private/qspdyprotocolhandler_p.h>

#ifndef QT_NO_SSL
#    include <QtNetwork/qsslkey.h>
#    include <QtNetwork/qsslcipher.h>
#    include <QtNetwork/qsslconfiguration.h>
#endif

#ifndef QT_NO_BEARERMANAGEMENT
#include "private/qnetworksession_p.h"
#endif

QT_BEGIN_NAMESPACE

// TODO: Put channel specific stuff here so it does not polute qhttpnetworkconnection.cpp

QHttpNetworkConnectionChannel::QHttpNetworkConnectionChannel()
    : socket(0)
    , ssl(false)
    , isInitialized(false)
    , state(IdleState)
    , reply(0)
    , written(0)
    , bytesTotal(0)
    , resendCurrent(false)
    , lastStatus(0)
    , pendingEncrypt(false)
    , reconnectAttempts(2)
    , authMethod(QAuthenticatorPrivate::None)
    , proxyAuthMethod(QAuthenticatorPrivate::None)
    , authenticationCredentialsSent(false)
    , proxyCredentialsSent(false)
    , protocolHandler(0)
#ifndef QT_NO_SSL
    , ignoreAllSslErrors(false)
#endif
    , pipeliningSupported(PipeliningSupportUnknown)
    , networkLayerPreference(QAbstractSocket::AnyIPProtocol)
    , connection(0)
{
    // Inlining this function in the header leads to compiler error on
    // release-armv5, on at least timebox 9.2 and 10.1.
}

void QHttpNetworkConnectionChannel::init()
{
#ifndef QT_NO_SSL
    if (connection->d_func()->encrypt)
        socket = new QSslSocket;
    else
        socket = new QTcpSocket;
#else
    socket = new QTcpSocket;
#endif
#ifndef QT_NO_BEARERMANAGEMENT
    //push session down to socket
    if (networkSession)
        socket->setProperty("_q_networksession", QVariant::fromValue(networkSession));
#endif
#ifndef QT_NO_NETWORKPROXY
    // Set by QNAM anyway, but let's be safe here
    socket->setProxy(QNetworkProxy::NoProxy);
#endif

    QObject::connect(socket, SIGNAL(bytesWritten(qint64)),
                     this, SLOT(_q_bytesWritten(qint64)),
                     Qt::DirectConnection);
    QObject::connect(socket, SIGNAL(connected()),
                     this, SLOT(_q_connected()),
                     Qt::DirectConnection);
    QObject::connect(socket, SIGNAL(readyRead()),
                     this, SLOT(_q_readyRead()),
                     Qt::DirectConnection);

    // The disconnected() and error() signals may already come
    // while calling connectToHost().
    // In case of a cached hostname or an IP this
    // will then emit a signal to the user of QNetworkReply
    // but cannot be caught because the user did not have a chance yet
    // to connect to QNetworkReply's signals.
    qRegisterMetaType<QAbstractSocket::SocketError>();
    QObject::connect(socket, SIGNAL(disconnected()),
                     this, SLOT(_q_disconnected()),
                     Qt::QueuedConnection);
    QObject::connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
                     this, SLOT(_q_error(QAbstractSocket::SocketError)),
                     Qt::QueuedConnection);


#ifndef QT_NO_NETWORKPROXY
    QObject::connect(socket, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
                     this, SLOT(_q_proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
                     Qt::DirectConnection);
#endif

#ifndef QT_NO_SSL
    QSslSocket *sslSocket = qobject_cast<QSslSocket*>(socket);
    if (sslSocket) {
        // won't be a sslSocket if encrypt is false
        QObject::connect(sslSocket, SIGNAL(encrypted()),
                         this, SLOT(_q_encrypted()),
                         Qt::DirectConnection);
        QObject::connect(sslSocket, SIGNAL(sslErrors(QList<QSslError>)),
                         this, SLOT(_q_sslErrors(QList<QSslError>)),
                         Qt::DirectConnection);
        QObject::connect(sslSocket, SIGNAL(encryptedBytesWritten(qint64)),
                         this, SLOT(_q_encryptedBytesWritten(qint64)),
                         Qt::DirectConnection);

        if (ignoreAllSslErrors)
            sslSocket->ignoreSslErrors();

        if (!ignoreSslErrorsList.isEmpty())
            sslSocket->ignoreSslErrors(ignoreSslErrorsList);

        if (!sslConfiguration.isNull())
           sslSocket->setSslConfiguration(sslConfiguration);
    } else {
#endif // QT_NO_SSL
        protocolHandler.reset(new QHttpProtocolHandler(this));
#ifndef QT_NO_SSL
    }
#endif

#ifndef QT_NO_NETWORKPROXY
    if (proxy.type() != QNetworkProxy::NoProxy)
        socket->setProxy(proxy);
#endif
    isInitialized = true;
}


void QHttpNetworkConnectionChannel::close()
{
    if (!socket)
        state = QHttpNetworkConnectionChannel::IdleState;
    else if (socket->state() == QAbstractSocket::UnconnectedState)
        state = QHttpNetworkConnectionChannel::IdleState;
    else
        state = QHttpNetworkConnectionChannel::ClosingState;

    // pendingEncrypt must only be true in between connected and encrypted states
    pendingEncrypt = false;

    if (socket)
        socket->close();
}


bool QHttpNetworkConnectionChannel::sendRequest()
{
    Q_ASSERT(!protocolHandler.isNull());
    return protocolHandler->sendRequest();
}


void QHttpNetworkConnectionChannel::_q_receiveReply()
{
    Q_ASSERT(!protocolHandler.isNull());
    protocolHandler->_q_receiveReply();
}

void QHttpNetworkConnectionChannel::_q_readyRead()
{
    Q_ASSERT(!protocolHandler.isNull());
    protocolHandler->_q_readyRead();
}

// called when unexpectedly reading a -1 or when data is expected but socket is closed
void QHttpNetworkConnectionChannel::handleUnexpectedEOF()
{
    Q_ASSERT(reply);
    if (reconnectAttempts <= 0) {
        // too many errors reading/receiving/parsing the status, close the socket and emit error
        requeueCurrentlyPipelinedRequests();
        close();
        reply->d_func()->errorString = connection->d_func()->errorDetail(QNetworkReply::RemoteHostClosedError, socket);
        emit reply->finishedWithError(QNetworkReply::RemoteHostClosedError, reply->d_func()->errorString);
        QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
    } else {
        reconnectAttempts--;
        reply->d_func()->clear();
        reply->d_func()->connection = connection;
        reply->d_func()->connectionChannel = this;
        closeAndResendCurrentRequest();
    }
}

bool QHttpNetworkConnectionChannel::ensureConnection()
{
    if (!isInitialized)
        init();

    QAbstractSocket::SocketState socketState = socket->state();

    // resend this request after we receive the disconnected signal
    if (socketState == QAbstractSocket::ClosingState) {
        if (reply)
            resendCurrent = true;
        return false;
    }

    // already trying to connect?
    if (socketState == QAbstractSocket::HostLookupState ||
        socketState == QAbstractSocket::ConnectingState) {
        return false;
    }

    // make sure that this socket is in a connected state, if not initiate
    // connection to the host.
    if (socketState != QAbstractSocket::ConnectedState) {
        // connect to the host if not already connected.
        state = QHttpNetworkConnectionChannel::ConnectingState;
        pendingEncrypt = ssl;

        // reset state
        pipeliningSupported = PipeliningSupportUnknown;
        authenticationCredentialsSent = false;
        proxyCredentialsSent = false;
        authenticator.detach();
        QAuthenticatorPrivate *priv = QAuthenticatorPrivate::getPrivate(authenticator);
        priv->hasFailed = false;
        proxyAuthenticator.detach();
        priv = QAuthenticatorPrivate::getPrivate(proxyAuthenticator);
        priv->hasFailed = false;

        // This workaround is needed since we use QAuthenticator for NTLM authentication. The "phase == Done"
        // is the usual criteria for emitting authentication signals. The "phase" is set to "Done" when the
        // last header for Authorization is generated by the QAuthenticator. Basic & Digest logic does not
        // check the "phase" for generating the Authorization header. NTLM authentication is a two stage
        // process & needs the "phase". To make sure the QAuthenticator uses the current username/password
        // the phase is reset to Start.
        priv = QAuthenticatorPrivate::getPrivate(authenticator);
        if (priv && priv->phase == QAuthenticatorPrivate::Done)
            priv->phase = QAuthenticatorPrivate::Start;
        priv = QAuthenticatorPrivate::getPrivate(proxyAuthenticator);
        if (priv && priv->phase == QAuthenticatorPrivate::Done)
            priv->phase = QAuthenticatorPrivate::Start;

        QString connectHost = connection->d_func()->hostName;
        qint16 connectPort = connection->d_func()->port;

#ifndef QT_NO_NETWORKPROXY
        // HTTPS always use transparent proxy.
        if (connection->d_func()->networkProxy.type() != QNetworkProxy::NoProxy && !ssl) {
            connectHost = connection->d_func()->networkProxy.hostName();
            connectPort = connection->d_func()->networkProxy.port();
        }
        if (socket->proxy().type() == QNetworkProxy::HttpProxy) {
            // Make user-agent field available to HTTP proxy socket engine (QTBUG-17223)
            QByteArray value;
            // ensureConnection is called before any request has been assigned, but can also be called again if reconnecting
            if (request.url().isEmpty())
                value = connection->d_func()->predictNextRequest().headerField("user-agent");
            else
                value = request.headerField("user-agent");
            if (!value.isEmpty()) {
                QNetworkProxy proxy(socket->proxy());
                proxy.setRawHeader("User-Agent", value); //detaches
                socket->setProxy(proxy);
            }
        }
#endif
        if (ssl) {
#ifndef QT_NO_SSL
            QSslSocket *sslSocket = qobject_cast<QSslSocket*>(socket);

            // check whether we can re-use an existing SSL session
            // (meaning another socket in this connection has already
            // performed a full handshake)
            if (!connection->sslContext().isNull())
                QSslSocketPrivate::checkSettingSslContext(sslSocket, connection->sslContext());

            sslSocket->connectToHostEncrypted(connectHost, connectPort, QIODevice::ReadWrite, networkLayerPreference);
            if (ignoreAllSslErrors)
                sslSocket->ignoreSslErrors();
            sslSocket->ignoreSslErrors(ignoreSslErrorsList);

            // limit the socket read buffer size. we will read everything into
            // the QHttpNetworkReply anyway, so let's grow only that and not
            // here and there.
            socket->setReadBufferSize(64*1024);
#else
            // Need to dequeue the request so that we can emit the error.
            if (!reply)
                connection->d_func()->dequeueRequest(socket);
            connection->d_func()->emitReplyError(socket, reply, QNetworkReply::ProtocolUnknownError);
#endif
        } else {
            // In case of no proxy we can use the Unbuffered QTcpSocket
#ifndef QT_NO_NETWORKPROXY
            if (connection->d_func()->networkProxy.type() == QNetworkProxy::NoProxy
                    && connection->cacheProxy().type() == QNetworkProxy::NoProxy
                    && connection->transparentProxy().type() == QNetworkProxy::NoProxy) {
#endif
                socket->connectToHost(connectHost, connectPort, QIODevice::ReadWrite | QIODevice::Unbuffered, networkLayerPreference);
                // For an Unbuffered QTcpSocket, the read buffer size has a special meaning.
                socket->setReadBufferSize(1*1024);
#ifndef QT_NO_NETWORKPROXY
            } else {
                socket->connectToHost(connectHost, connectPort, QIODevice::ReadWrite, networkLayerPreference);

                // limit the socket read buffer size. we will read everything into
                // the QHttpNetworkReply anyway, so let's grow only that and not
                // here and there.
                socket->setReadBufferSize(64*1024);
            }
#endif
        }
        return false;
    }
    return true;
}

void QHttpNetworkConnectionChannel::allDone()
{
    Q_ASSERT(reply);

    if (!reply) {
        qWarning() << "QHttpNetworkConnectionChannel::allDone() called without reply. Please report at http://bugreports.qt-project.org/";
        return;
    }

    // while handling 401 & 407, we might reset the status code, so save this.
    bool emitFinished = reply->d_func()->shouldEmitSignals();
    bool connectionCloseEnabled = reply->d_func()->isConnectionCloseEnabled();
    detectPipeliningSupport();

    handleStatus();
    // handleStatus() might have removed the reply because it already called connection->emitReplyError()

    // queue the finished signal, this is required since we might send new requests from
    // slot connected to it. The socket will not fire readyRead signal, if we are already
    // in the slot connected to readyRead
    if (reply && emitFinished)
        QMetaObject::invokeMethod(reply, "finished", Qt::QueuedConnection);


    // reset the reconnection attempts after we receive a complete reply.
    // in case of failures, each channel will attempt two reconnects before emitting error.
    reconnectAttempts = 2;

    // now the channel can be seen as free/idle again, all signal emissions for the reply have been done
    if (state != QHttpNetworkConnectionChannel::ClosingState)
        state = QHttpNetworkConnectionChannel::IdleState;

    // if it does not need to be sent again we can set it to 0
    // the previous code did not do that and we had problems with accidental re-sending of a
    // finished request.
    // Note that this may trigger a segfault at some other point. But then we can fix the underlying
    // problem.
    if (!resendCurrent) {
        request = QHttpNetworkRequest();
        reply = 0;
        protocolHandler->setReply(0);
    }

    // move next from pipeline to current request
    if (!alreadyPipelinedRequests.isEmpty()) {
        if (resendCurrent || connectionCloseEnabled || socket->state() != QAbstractSocket::ConnectedState) {
            // move the pipelined ones back to the main queue
            requeueCurrentlyPipelinedRequests();
            close();
        } else {
            // there were requests pipelined in and we can continue
            HttpMessagePair messagePair = alreadyPipelinedRequests.takeFirst();

            request = messagePair.first;
            reply = messagePair.second;
            protocolHandler->setReply(messagePair.second);
            state = QHttpNetworkConnectionChannel::ReadingState;
            resendCurrent = false;

            written = 0; // message body, excluding the header, irrelevant here
            bytesTotal = 0; // message body total, excluding the header, irrelevant here

            // pipeline even more
            connection->d_func()->fillPipeline(socket);

            // continue reading
            //_q_receiveReply();
            // this was wrong, allDone gets called from that function anyway.
        }
    } else if (alreadyPipelinedRequests.isEmpty() && socket->bytesAvailable() > 0) {
        // this is weird. we had nothing pipelined but still bytes available. better close it.
        close();

        QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
    } else if (alreadyPipelinedRequests.isEmpty()) {
        if (connectionCloseEnabled)
            if (socket->state() != QAbstractSocket::UnconnectedState)
                close();
        if (qobject_cast<QHttpNetworkConnection*>(connection))
            QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
    }
}

void QHttpNetworkConnectionChannel::detectPipeliningSupport()
{
    Q_ASSERT(reply);
    // detect HTTP Pipelining support
    QByteArray serverHeaderField;
    if (
            // check for HTTP/1.1
            (reply->d_func()->majorVersion == 1 && reply->d_func()->minorVersion == 1)
            // check for not having connection close
            && (!reply->d_func()->isConnectionCloseEnabled())
            // check if it is still connected
            && (socket->state() == QAbstractSocket::ConnectedState)
            // check for broken servers in server reply header
            // this is adapted from http://mxr.mozilla.org/firefox/ident?i=SupportsPipelining
            && (serverHeaderField = reply->headerField("Server"), !serverHeaderField.contains("Microsoft-IIS/4."))
            && (!serverHeaderField.contains("Microsoft-IIS/5."))
            && (!serverHeaderField.contains("Netscape-Enterprise/3."))
            // this is adpoted from the knowledge of the Nokia 7.x browser team (DEF143319)
            && (!serverHeaderField.contains("WebLogic"))
            && (!serverHeaderField.startsWith("Rocket")) // a Python Web Server, see Web2py.com
            ) {
        pipeliningSupported = QHttpNetworkConnectionChannel::PipeliningProbablySupported;
    } else {
        pipeliningSupported = QHttpNetworkConnectionChannel::PipeliningSupportUnknown;
    }
}

// called when the connection broke and we need to queue some pipelined requests again
void QHttpNetworkConnectionChannel::requeueCurrentlyPipelinedRequests()
{
    for (int i = 0; i < alreadyPipelinedRequests.length(); i++)
        connection->d_func()->requeueRequest(alreadyPipelinedRequests.at(i));
    alreadyPipelinedRequests.clear();

    // only run when the QHttpNetworkConnection is not currently being destructed, e.g.
    // this function is called from _q_disconnected which is called because
    // of ~QHttpNetworkConnectionPrivate
    if (qobject_cast<QHttpNetworkConnection*>(connection))
        QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
}

void QHttpNetworkConnectionChannel::handleStatus()
{
    Q_ASSERT(socket);
    Q_ASSERT(reply);

    int statusCode = reply->statusCode();
    bool resend = false;

    switch (statusCode) {
    case 401: // auth required
    case 407: // proxy auth required
        if (connection->d_func()->handleAuthenticateChallenge(socket, reply, (statusCode == 407), resend)) {
            if (resend) {
                if (!resetUploadData())
                    break;

                reply->d_func()->eraseData();

                if (alreadyPipelinedRequests.isEmpty()) {
                    // this does a re-send without closing the connection
                    resendCurrent = true;
                    QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
                } else {
                    // we had requests pipelined.. better close the connection in closeAndResendCurrentRequest
                    closeAndResendCurrentRequest();
                    QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
                }
            } else {
                //authentication cancelled, close the channel.
                close();
            }
        } else {
            emit reply->headerChanged();
            emit reply->readyRead();
            QNetworkReply::NetworkError errorCode = (statusCode == 407)
                ? QNetworkReply::ProxyAuthenticationRequiredError
                : QNetworkReply::AuthenticationRequiredError;
            reply->d_func()->errorString = connection->d_func()->errorDetail(errorCode, socket);
            emit reply->finishedWithError(errorCode, reply->d_func()->errorString);
        }
        break;
    default:
        if (qobject_cast<QHttpNetworkConnection*>(connection))
            QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
    }
}

bool QHttpNetworkConnectionChannel::resetUploadData()
{
    if (!reply) {
        //this happens if server closes connection while QHttpNetworkConnectionPrivate::_q_startNextRequest is pending
        return false;
    }
    QNonContiguousByteDevice* uploadByteDevice = request.uploadByteDevice();
    if (!uploadByteDevice)
        return true;

    if (uploadByteDevice->reset()) {
        written = 0;
        return true;
    } else {
        connection->d_func()->emitReplyError(socket, reply, QNetworkReply::ContentReSendError);
        return false;
    }
}

#ifndef QT_NO_NETWORKPROXY

void QHttpNetworkConnectionChannel::setProxy(const QNetworkProxy &networkProxy)
{
    if (socket)
        socket->setProxy(networkProxy);

    proxy = networkProxy;
}

#endif

#ifndef QT_NO_SSL

void QHttpNetworkConnectionChannel::ignoreSslErrors()
{
    if (socket)
        static_cast<QSslSocket *>(socket)->ignoreSslErrors();

    ignoreAllSslErrors = true;
}


void QHttpNetworkConnectionChannel::ignoreSslErrors(const QList<QSslError> &errors)
{
    if (socket)
        static_cast<QSslSocket *>(socket)->ignoreSslErrors(errors);

    ignoreSslErrorsList = errors;
}

void QHttpNetworkConnectionChannel::setSslConfiguration(const QSslConfiguration &config)
{
    if (socket)
        static_cast<QSslSocket *>(socket)->setSslConfiguration(config);

    sslConfiguration = config;
}

#endif

void QHttpNetworkConnectionChannel::pipelineInto(HttpMessagePair &pair)
{
    // this is only called for simple GET

    QHttpNetworkRequest &request = pair.first;
    QHttpNetworkReply *reply = pair.second;
    reply->d_func()->clear();
    reply->d_func()->connection = connection;
    reply->d_func()->connectionChannel = this;
    reply->d_func()->autoDecompress = request.d->autoDecompress;
    reply->d_func()->pipeliningUsed = true;

#ifndef QT_NO_NETWORKPROXY
    pipeline.append(QHttpNetworkRequestPrivate::header(request,
                                                           (connection->d_func()->networkProxy.type() != QNetworkProxy::NoProxy)));
#else
    pipeline.append(QHttpNetworkRequestPrivate::header(request, false));
#endif

    alreadyPipelinedRequests.append(pair);

    // pipelineFlush() needs to be called at some point afterwards
}

void QHttpNetworkConnectionChannel::pipelineFlush()
{
    if (pipeline.isEmpty())
        return;

    // The goal of this is so that we have everything in one TCP packet.
    // For the Unbuffered QTcpSocket this is manually needed, the buffered
    // QTcpSocket does it automatically.
    // Also, sometimes the OS does it for us (Nagle's algorithm) but that
    // happens only sometimes.
    socket->write(pipeline);
    pipeline.clear();
}


void QHttpNetworkConnectionChannel::closeAndResendCurrentRequest()
{
    requeueCurrentlyPipelinedRequests();
    close();
    if (reply)
        resendCurrent = true;
    if (qobject_cast<QHttpNetworkConnection*>(connection))
        QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
}

bool QHttpNetworkConnectionChannel::isSocketBusy() const
{
    return (state & QHttpNetworkConnectionChannel::BusyState);
}

bool QHttpNetworkConnectionChannel::isSocketWriting() const
{
    return (state & QHttpNetworkConnectionChannel::WritingState);
}

bool QHttpNetworkConnectionChannel::isSocketWaiting() const
{
    return (state & QHttpNetworkConnectionChannel::WaitingState);
}

bool QHttpNetworkConnectionChannel::isSocketReading() const
{
    return (state & QHttpNetworkConnectionChannel::ReadingState);
}

void QHttpNetworkConnectionChannel::_q_bytesWritten(qint64 bytes)
{
    Q_UNUSED(bytes);
    // bytes have been written to the socket. write even more of them :)
    if (isSocketWriting())
        sendRequest();
    // otherwise we do nothing
}

void QHttpNetworkConnectionChannel::_q_disconnected()
{
    if (state == QHttpNetworkConnectionChannel::ClosingState) {
        state = QHttpNetworkConnectionChannel::IdleState;
        QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
        return;
    }

    // read the available data before closing
    if (isSocketWaiting() || isSocketReading()) {
        if (reply) {
            state = QHttpNetworkConnectionChannel::ReadingState;
            _q_receiveReply();
        }
    } else if (state == QHttpNetworkConnectionChannel::IdleState && resendCurrent) {
        // re-sending request because the socket was in ClosingState
        QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
    }
    state = QHttpNetworkConnectionChannel::IdleState;

    requeueCurrentlyPipelinedRequests();
    close();
}


void QHttpNetworkConnectionChannel::_q_connected()
{
    // For the Happy Eyeballs we need to check if this is the first channel to connect.
    if (connection->d_func()->networkLayerState == QHttpNetworkConnectionPrivate::HostLookupPending || connection->d_func()->networkLayerState == QHttpNetworkConnectionPrivate::IPv4or6) {
        if (connection->d_func()->delayedConnectionTimer.isActive())
            connection->d_func()->delayedConnectionTimer.stop();
        if (networkLayerPreference == QAbstractSocket::IPv4Protocol)
            connection->d_func()->networkLayerState = QHttpNetworkConnectionPrivate::IPv4;
        else if (networkLayerPreference == QAbstractSocket::IPv6Protocol)
            connection->d_func()->networkLayerState = QHttpNetworkConnectionPrivate::IPv6;
        else {
            if (socket->peerAddress().protocol() == QAbstractSocket::IPv4Protocol)
                connection->d_func()->networkLayerState = QHttpNetworkConnectionPrivate::IPv4;
            else
                connection->d_func()->networkLayerState = QHttpNetworkConnectionPrivate::IPv6;
        }
        connection->d_func()->networkLayerDetected(networkLayerPreference);
    } else {
        if (((connection->d_func()->networkLayerState == QHttpNetworkConnectionPrivate::IPv4) && (networkLayerPreference != QAbstractSocket::IPv4Protocol))
            || ((connection->d_func()->networkLayerState == QHttpNetworkConnectionPrivate::IPv6) && (networkLayerPreference != QAbstractSocket::IPv6Protocol))) {
            close();
            // This is the second connection so it has to be closed and we can schedule it for another request.
            QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
            return;
        }
        //The connections networkLayerState had already been decided.
    }

    // improve performance since we get the request sent by the kernel ASAP
    //socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    // We have this commented out now. It did not have the effect we wanted. If we want to
    // do this properly, Qt has to combine multiple HTTP requests into one buffer
    // and send this to the kernel in one syscall and then the kernel immediately sends
    // it as one TCP packet because of TCP_NODELAY.
    // However, this code is currently not in Qt, so we rely on the kernel combining
    // the requests into one TCP packet.

    // not sure yet if it helps, but it makes sense
    socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

    pipeliningSupported = QHttpNetworkConnectionChannel::PipeliningSupportUnknown;

    // ### FIXME: if the server closes the connection unexpectedly, we shouldn't send the same broken request again!
    //channels[i].reconnectAttempts = 2;
    if (pendingEncrypt) {
#ifndef QT_NO_SSL
        if (connection->sslContext().isNull()) {
            // this socket is making the 1st handshake for this connection,
            // we need to set the SSL context so new sockets can reuse it
            QSharedPointer<QSslContext> socketSslContext = QSslSocketPrivate::sslContext(static_cast<QSslSocket*>(socket));
            if (!socketSslContext.isNull())
                connection->setSslContext(socketSslContext);
        }
#endif
    } else {
        state = QHttpNetworkConnectionChannel::IdleState;
        if (!reply)
            connection->d_func()->dequeueRequest(socket);
        if (reply)
            sendRequest();
    }
}


void QHttpNetworkConnectionChannel::_q_error(QAbstractSocket::SocketError socketError)
{
    if (!socket)
        return;
    QNetworkReply::NetworkError errorCode = QNetworkReply::UnknownNetworkError;

    switch (socketError) {
    case QAbstractSocket::HostNotFoundError:
        errorCode = QNetworkReply::HostNotFoundError;
        break;
    case QAbstractSocket::ConnectionRefusedError:
        errorCode = QNetworkReply::ConnectionRefusedError;
        break;
    case QAbstractSocket::RemoteHostClosedError:
        // try to reconnect/resend before sending an error.
        // while "Reading" the _q_disconnected() will handle this.
        if (state != QHttpNetworkConnectionChannel::IdleState && state != QHttpNetworkConnectionChannel::ReadingState) {
            if (reconnectAttempts-- > 0) {
                closeAndResendCurrentRequest();
                return;
            } else {
                errorCode = QNetworkReply::RemoteHostClosedError;
            }
        } else if (state == QHttpNetworkConnectionChannel::ReadingState) {
            if (!reply)
                break;

            if (!reply->d_func()->expectContent()) {
                // No content expected, this is a valid way to have the connection closed by the server
                return;
            }
            if (reply->contentLength() == -1 && !reply->d_func()->isChunked()) {
                // There was no content-length header and it's not chunked encoding,
                // so this is a valid way to have the connection closed by the server
                return;
            }
            // ok, we got a disconnect even though we did not expect it
            // Try to read everything from the socket before we emit the error.
            if (socket->bytesAvailable()) {
                // Read everything from the socket into the reply buffer.
                // we can ignore the readbuffersize as the data is already
                // in memory and we will not receive more data on the socket.
                reply->setReadBufferSize(0);
                _q_receiveReply();
#ifndef QT_NO_SSL
                if (ssl) {
                    // QT_NO_OPENSSL. The QSslSocket can still have encrypted bytes in the plainsocket.
                    // So we need to check this if the socket is a QSslSocket. When the socket is flushed
                    // it will force a decrypt of the encrypted data in the plainsocket.
                    QSslSocket *sslSocket = static_cast<QSslSocket*>(socket);
                    qint64 beforeFlush = sslSocket->encryptedBytesAvailable();
                    while (sslSocket->encryptedBytesAvailable()) {
                        sslSocket->flush();
                        _q_receiveReply();
                        qint64 afterFlush = sslSocket->encryptedBytesAvailable();
                        if (afterFlush == beforeFlush)
                            break;
                        beforeFlush = afterFlush;
                    }
                }
#endif
            }

            errorCode = QNetworkReply::RemoteHostClosedError;
        } else {
            errorCode = QNetworkReply::RemoteHostClosedError;
        }
        break;
    case QAbstractSocket::SocketTimeoutError:
        // try to reconnect/resend before sending an error.
        if (state == QHttpNetworkConnectionChannel::WritingState && (reconnectAttempts-- > 0)) {
            closeAndResendCurrentRequest();
            return;
        }
        errorCode = QNetworkReply::TimeoutError;
        break;
    case QAbstractSocket::ProxyAuthenticationRequiredError:
        errorCode = QNetworkReply::ProxyAuthenticationRequiredError;
        break;
    case QAbstractSocket::SslHandshakeFailedError:
        errorCode = QNetworkReply::SslHandshakeFailedError;
        break;
    case QAbstractSocket::ProxyConnectionClosedError:
        // try to reconnect/resend before sending an error.
        if (reconnectAttempts-- > 0) {
            closeAndResendCurrentRequest();
            return;
        }
        errorCode = QNetworkReply::ProxyConnectionClosedError;
        break;
    case QAbstractSocket::ProxyConnectionTimeoutError:
        // try to reconnect/resend before sending an error.
        if (reconnectAttempts-- > 0) {
            closeAndResendCurrentRequest();
            return;
        }
        errorCode = QNetworkReply::ProxyTimeoutError;
        break;
    default:
        // all other errors are treated as NetworkError
        errorCode = QNetworkReply::UnknownNetworkError;
        break;
    }
    QPointer<QHttpNetworkConnection> that = connection;
    QString errorString = connection->d_func()->errorDetail(errorCode, socket, socket->errorString());

    // In the HostLookupPending state the channel should not emit the error.
    // This will instead be handled by the connection.
    if (!connection->d_func()->shouldEmitChannelError(socket))
        return;

    // emit error for all waiting replies
    do {
        // Need to dequeu the request so that we can emit the error.
        if (!reply)
            connection->d_func()->dequeueRequest(socket);

        if (reply) {
            reply->d_func()->errorString = errorString;
            emit reply->finishedWithError(errorCode, errorString);
            reply = 0;
            if (protocolHandler)
                protocolHandler->setReply(0);
        }
    } while (!connection->d_func()->highPriorityQueue.isEmpty()
             || !connection->d_func()->lowPriorityQueue.isEmpty());
#ifndef QT_NO_SSL
    if (connection->connectionType() == QHttpNetworkConnection::ConnectionTypeSPDY) {
        QList<HttpMessagePair> spdyPairs = spdyRequestsToSend.values();
        for (int a = 0; a < spdyPairs.count(); ++a) {
            // emit error for all replies
            QHttpNetworkReply *currentReply = spdyPairs.at(a).second;
            Q_ASSERT(currentReply);
            emit currentReply->finishedWithError(errorCode, errorString);
        }
    }
#endif // QT_NO_SSL

    // send the next request
    QMetaObject::invokeMethod(that, "_q_startNextRequest", Qt::QueuedConnection);

    if (that) //signal emission triggered event loop
        close();
}

#ifndef QT_NO_NETWORKPROXY
void QHttpNetworkConnectionChannel::_q_proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator* auth)
{
#ifndef QT_NO_SSL
    if (connection->connectionType() == QHttpNetworkConnection::ConnectionTypeSPDY) {
        connection->d_func()->emitProxyAuthenticationRequired(this, proxy, auth);
    } else { // HTTP
#endif // QT_NO_SSL
        // Need to dequeue the request before we can emit the error.
        if (!reply)
            connection->d_func()->dequeueRequest(socket);
        if (reply)
            connection->d_func()->emitProxyAuthenticationRequired(this, proxy, auth);
#ifndef QT_NO_SSL
    }
#endif // QT_NO_SSL
}
#endif

void QHttpNetworkConnectionChannel::_q_uploadDataReadyRead()
{
    sendRequest();
}

#ifndef QT_NO_SSL
void QHttpNetworkConnectionChannel::_q_encrypted()
{
    QSslSocket *sslSocket = qobject_cast<QSslSocket *>(socket);
    Q_ASSERT(sslSocket);

    if (!protocolHandler) {
        switch (sslSocket->sslConfiguration().nextProtocolNegotiationStatus()) {
        case QSslConfiguration::NextProtocolNegotiationNegotiated: {
            QByteArray nextProtocol = sslSocket->sslConfiguration().nextNegotiatedProtocol();
            if (nextProtocol == QSslConfiguration::NextProtocolHttp1_1) {
                // fall through to create a QHttpProtocolHandler
            } else if (nextProtocol == QSslConfiguration::NextProtocolSpdy3_0) {
                protocolHandler.reset(new QSpdyProtocolHandler(this));
                connection->setConnectionType(QHttpNetworkConnection::ConnectionTypeSPDY);
                // no need to re-queue requests, if SPDY was enabled on the request it
                // has gone to the SPDY queue already
                break;
            } else {
                emitFinishedWithError(QNetworkReply::SslHandshakeFailedError,
                                      "detected unknown Next Protocol Negotiation protocol");
                break;
            }
        }
        case QSslConfiguration::NextProtocolNegotiationNone:
            protocolHandler.reset(new QHttpProtocolHandler(this));
            connection->setConnectionType(QHttpNetworkConnection::ConnectionTypeHTTP);
            // re-queue requests from SPDY queue to HTTP queue, if any
            requeueSpdyRequests();
            break;
        case QSslConfiguration::NextProtocolNegotiationUnsupported:
            emitFinishedWithError(QNetworkReply::SslHandshakeFailedError,
                                  "chosen Next Protocol Negotiation value unsupported");
            break;
        default:
            emitFinishedWithError(QNetworkReply::SslHandshakeFailedError,
                                  "detected unknown Next Protocol Negotiation protocol");
        }
    }

    if (!socket)
        return; // ### error
    state = QHttpNetworkConnectionChannel::IdleState;
    pendingEncrypt = false;

    if (connection->connectionType() == QHttpNetworkConnection::ConnectionTypeSPDY) {
        // we call setSpdyWasUsed(true) on the replies in the SPDY handler when the request is sent
        if (spdyRequestsToSend.count() > 0)
            // wait for data from the server first (e.g. initial window, max concurrent requests)
            QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
    } else { // HTTP
        if (!reply)
            connection->d_func()->dequeueRequest(socket);
        if (reply) {
            reply->setSpdyWasUsed(false);
            emit reply->encrypted();
        }
        if (reply)
            sendRequest();
    }
}

void QHttpNetworkConnectionChannel::requeueSpdyRequests()
{
    QList<HttpMessagePair> spdyPairs = spdyRequestsToSend.values();
    for (int a = 0; a < spdyPairs.count(); ++a) {
        connection->d_func()->requeueRequest(spdyPairs.at(a));
    }
    spdyRequestsToSend.clear();
}

void QHttpNetworkConnectionChannel::emitFinishedWithError(QNetworkReply::NetworkError error,
                                                          const char *message)
{
    if (reply)
        emit reply->finishedWithError(error, QHttpNetworkConnectionChannel::tr(message));
    QList<HttpMessagePair> spdyPairs = spdyRequestsToSend.values();
    for (int a = 0; a < spdyPairs.count(); ++a) {
        QHttpNetworkReply *currentReply = spdyPairs.at(a).second;
        Q_ASSERT(currentReply);
        emit currentReply->finishedWithError(error, QHttpNetworkConnectionChannel::tr(message));
    }
}

void QHttpNetworkConnectionChannel::_q_sslErrors(const QList<QSslError> &errors)
{
    if (!socket)
        return;
    //QNetworkReply::NetworkError errorCode = QNetworkReply::ProtocolFailure;
    // Also pause the connection because socket notifiers may fire while an user
    // dialog is displaying
    connection->d_func()->pauseConnection();
    if (pendingEncrypt && !reply)
        connection->d_func()->dequeueRequest(socket);
    if (connection->connectionType() == QHttpNetworkConnection::ConnectionTypeHTTP) {
        if (reply)
            emit reply->sslErrors(errors);
    }
#ifndef QT_NO_SSL
    else { // SPDY
        QList<HttpMessagePair> spdyPairs = spdyRequestsToSend.values();
        for (int a = 0; a < spdyPairs.count(); ++a) {
            // emit SSL errors for all replies
            QHttpNetworkReply *currentReply = spdyPairs.at(a).second;
            Q_ASSERT(currentReply);
            emit currentReply->sslErrors(errors);
        }
    }
#endif // QT_NO_SSL
    connection->d_func()->resumeConnection();
}

void QHttpNetworkConnectionChannel::_q_encryptedBytesWritten(qint64 bytes)
{
    Q_UNUSED(bytes);
    // bytes have been written to the socket. write even more of them :)
    if (isSocketWriting())
        sendRequest();
    // otherwise we do nothing
}

#endif

void QHttpNetworkConnectionChannel::setConnection(QHttpNetworkConnection *c)
{
    // Inlining this function in the header leads to compiler error on
    // release-armv5, on at least timebox 9.2 and 10.1.
    connection = c;
}

QT_END_NAMESPACE

#include "moc_qhttpnetworkconnectionchannel_p.cpp"

#endif // QT_NO_HTTP
