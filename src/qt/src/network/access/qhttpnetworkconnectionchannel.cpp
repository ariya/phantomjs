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

#include "qhttpnetworkconnection_p.h"
#include "qhttpnetworkconnectionchannel_p.h"
#include "private/qnoncontiguousbytedevice_p.h"

#include <qpair.h>
#include <qdebug.h>

#ifndef QT_NO_HTTP

#ifndef QT_NO_OPENSSL
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
#ifndef QT_NO_OPENSSL
    , ignoreAllSslErrors(false)
#endif
    , pipeliningSupported(PipeliningSupportUnknown)
    , connection(0)
{
    // Inlining this function in the header leads to compiler error on
    // release-armv5, on at least timebox 9.2 and 10.1.
}

void QHttpNetworkConnectionChannel::init()
{
#ifndef QT_NO_OPENSSL
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
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
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

#ifndef QT_NO_OPENSSL
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
    }
#endif
}


void QHttpNetworkConnectionChannel::close()
{
    if (socket->state() == QAbstractSocket::UnconnectedState)
        state = QHttpNetworkConnectionChannel::IdleState;
    else
        state = QHttpNetworkConnectionChannel::ClosingState;

    socket->close();
}


bool QHttpNetworkConnectionChannel::sendRequest()
{
    if (!reply) {
        // heh, how should that happen!
        qWarning() << "QHttpNetworkConnectionChannel::sendRequest() called without QHttpNetworkReply";
        state = QHttpNetworkConnectionChannel::IdleState;
        return false;
    }

    switch (state) {
    case QHttpNetworkConnectionChannel::IdleState: { // write the header
        if (!ensureConnection()) {
            // wait for the connection (and encryption) to be done
            // sendRequest will be called again from either
            // _q_connected or _q_encrypted
            return false;
        }
        written = 0; // excluding the header
        bytesTotal = 0;

        QHttpNetworkReplyPrivate *replyPrivate = reply->d_func();
        replyPrivate->clear();
        replyPrivate->connection = connection;
        replyPrivate->connectionChannel = this;
        replyPrivate->autoDecompress = request.d->autoDecompress;
        replyPrivate->pipeliningUsed = false;

        // if the url contains authentication parameters, use the new ones
        // both channels will use the new authentication parameters
        if (!request.url().userInfo().isEmpty() && request.withCredentials()) {
            QUrl url = request.url();
            QAuthenticator &auth = authenticator;
            if (url.userName() != auth.user()
                || (!url.password().isEmpty() && url.password() != auth.password())) {
                auth.setUser(url.userName());
                auth.setPassword(url.password());
                connection->d_func()->copyCredentials(connection->d_func()->indexOf(socket), &auth, false);
            }
            // clear the userinfo,  since we use the same request for resending
            // userinfo in url can conflict with the one in the authenticator
            url.setUserInfo(QString());
            request.setUrl(url);
        }
        // Will only be false if QtWebKit is performing a cross-origin XMLHttpRequest
        // and withCredentials has not been set to true.
        if (request.withCredentials())
            connection->d_func()->createAuthorization(socket, request);
#ifndef QT_NO_NETWORKPROXY
        QByteArray header = QHttpNetworkRequestPrivate::header(request,
            (connection->d_func()->networkProxy.type() != QNetworkProxy::NoProxy));
#else
        QByteArray header = QHttpNetworkRequestPrivate::header(request, false);
#endif
        socket->write(header);
        // flushing is dangerous (QSslSocket calls transmit which might read or error)
//        socket->flush();
        QNonContiguousByteDevice* uploadByteDevice = request.uploadByteDevice();
        if (uploadByteDevice) {
            // connect the signals so this function gets called again
            QObject::connect(uploadByteDevice, SIGNAL(readyRead()),this, SLOT(_q_uploadDataReadyRead()));

            bytesTotal = request.contentLength();

            state = QHttpNetworkConnectionChannel::WritingState; // start writing data
            sendRequest(); //recurse
        } else {
            state = QHttpNetworkConnectionChannel::WaitingState; // now wait for response
            sendRequest(); //recurse
        }

        break;
    }
    case QHttpNetworkConnectionChannel::WritingState:
    {
        // write the data
        QNonContiguousByteDevice* uploadByteDevice = request.uploadByteDevice();
        if (!uploadByteDevice || bytesTotal == written) {
            if (uploadByteDevice)
                emit reply->dataSendProgress(written, bytesTotal);
            state = QHttpNetworkConnectionChannel::WaitingState; // now wait for response
            sendRequest(); // recurse
            break;
        }

        // only feed the QTcpSocket buffer when there is less than 32 kB in it
        const qint64 socketBufferFill = 32*1024;
        const qint64 socketWriteMaxSize = 16*1024;


#ifndef QT_NO_OPENSSL
        QSslSocket *sslSocket = qobject_cast<QSslSocket*>(socket);
        // if it is really an ssl socket, check more than just bytesToWrite()
        while ((socket->bytesToWrite() + (sslSocket ? sslSocket->encryptedBytesToWrite() : 0))
                <= socketBufferFill && bytesTotal != written)
#else
        while (socket->bytesToWrite() <= socketBufferFill
               && bytesTotal != written)
#endif
        {
            // get pointer to upload data
            qint64 currentReadSize = 0;
            qint64 desiredReadSize = qMin(socketWriteMaxSize, bytesTotal - written);
            const char *readPointer = uploadByteDevice->readPointer(desiredReadSize, currentReadSize);

            if (currentReadSize == -1) {
                // premature eof happened
                connection->d_func()->emitReplyError(socket, reply, QNetworkReply::UnknownNetworkError);
                return false;
                break;
            } else if (readPointer == 0 || currentReadSize == 0) {
                // nothing to read currently, break the loop
                break;
            } else {
                qint64 currentWriteSize = socket->write(readPointer, currentReadSize);
                if (currentWriteSize == -1 || currentWriteSize != currentReadSize) {
                    // socket broke down
                    connection->d_func()->emitReplyError(socket, reply, QNetworkReply::UnknownNetworkError);
                    return false;
                } else {
                    written += currentWriteSize;
                    uploadByteDevice->advanceReadPointer(currentWriteSize);

                    emit reply->dataSendProgress(written, bytesTotal);

                    if (written == bytesTotal) {
                        // make sure this function is called once again
                        state = QHttpNetworkConnectionChannel::WaitingState;
                        sendRequest();
                        break;
                    }
                }
            }
        }
        break;
    }

    case QHttpNetworkConnectionChannel::WaitingState:
    {
        QNonContiguousByteDevice* uploadByteDevice = request.uploadByteDevice();
        if (uploadByteDevice) {
            QObject::disconnect(uploadByteDevice, SIGNAL(readyRead()), this, SLOT(_q_uploadDataReadyRead()));
        }

        // HTTP pipelining
        //connection->d_func()->fillPipeline(socket);
        //socket->flush();

        // ensure we try to receive a reply in all cases, even if _q_readyRead_ hat not been called
        // this is needed if the sends an reply before we have finished sending the request. In that
        // case receiveReply had been called before but ignored the server reply
        if (socket->bytesAvailable())
            QMetaObject::invokeMethod(this, "_q_receiveReply", Qt::QueuedConnection);
        break;
    }
    case QHttpNetworkConnectionChannel::ReadingState:
        // ignore _q_bytesWritten in these states
        // fall through
    default:
        break;
    }
    return true;
}


void QHttpNetworkConnectionChannel::_q_receiveReply()
{
    Q_ASSERT(socket);

    if (!reply) {
        if (socket->bytesAvailable() > 0)
            qWarning() << "QHttpNetworkConnectionChannel::_q_receiveReply() called without QHttpNetworkReply,"
                       << socket->bytesAvailable() << "bytes on socket.";

        close();
        return;
    }

    // only run when the QHttpNetworkConnection is not currently being destructed, e.g.
    // this function is called from _q_disconnected which is called because
    // of ~QHttpNetworkConnectionPrivate
    if (!qobject_cast<QHttpNetworkConnection*>(connection)) {
        return;
    }

    QAbstractSocket::SocketState socketState = socket->state();

    // connection might be closed to signal the end of data
    if (socketState == QAbstractSocket::UnconnectedState) {
        if (socket->bytesAvailable() <= 0) {
            if (reply->d_func()->state == QHttpNetworkReplyPrivate::ReadingDataState) {
                // finish this reply. this case happens when the server did not send a content length
                reply->d_func()->state = QHttpNetworkReplyPrivate::AllDoneState;
                allDone();
                return;
            } else {
                handleUnexpectedEOF();
                return;
            }
        } else {
            // socket not connected but still bytes for reading.. just continue in this function
        }
    }

    // read loop for the response
    qint64 bytes = 0;
    qint64 lastBytes = bytes;
    do {
        lastBytes = bytes;

        QHttpNetworkReplyPrivate::ReplyState state = reply->d_func()->state;
        switch (state) {
        case QHttpNetworkReplyPrivate::NothingDoneState: {
            state = reply->d_func()->state = QHttpNetworkReplyPrivate::ReadingStatusState;
            // fallthrough
        }
        case QHttpNetworkReplyPrivate::ReadingStatusState: {
            qint64 statusBytes = reply->d_func()->readStatus(socket);
            if (statusBytes == -1) {
                // connection broke while reading status. also handled if later _q_disconnected is called
                handleUnexpectedEOF();
                return;
            }
            bytes += statusBytes;
            lastStatus = reply->d_func()->statusCode;
            break;
        }
        case QHttpNetworkReplyPrivate::ReadingHeaderState: {
            QHttpNetworkReplyPrivate *replyPrivate = reply->d_func();
            qint64 headerBytes = replyPrivate->readHeader(socket);
            if (headerBytes == -1) {
                // connection broke while reading headers. also handled if later _q_disconnected is called
                handleUnexpectedEOF();
                return;
            }
            bytes += headerBytes;
            // If headers were parsed successfully now it is the ReadingDataState
            if (replyPrivate->state == QHttpNetworkReplyPrivate::ReadingDataState) {
                if (replyPrivate->isGzipped() && replyPrivate->autoDecompress) {
                    // remove the Content-Length from header
                    replyPrivate->removeAutoDecompressHeader();
                } else {
                    replyPrivate->autoDecompress = false;
                }
                if (replyPrivate->statusCode == 100) {
                    replyPrivate->clearHttpLayerInformation();
                    replyPrivate->state = QHttpNetworkReplyPrivate::ReadingStatusState;
                    break; // ignore
                }
                if (replyPrivate->shouldEmitSignals())
                    emit reply->headerChanged();
                // After headerChanged had been emitted
                // we can suddenly have a replyPrivate->userProvidedDownloadBuffer
                // this is handled in the ReadingDataState however

                if (!replyPrivate->expectContent()) {
                    replyPrivate->state = QHttpNetworkReplyPrivate::AllDoneState;
                    allDone();
                    break;
                }
            }
            break;
        }
        case QHttpNetworkReplyPrivate::ReadingDataState: {
           QHttpNetworkReplyPrivate *replyPrivate = reply->d_func();
           if (socket->state() == QAbstractSocket::ConnectedState &&
               replyPrivate->downstreamLimited && !replyPrivate->responseData.isEmpty() && replyPrivate->shouldEmitSignals()) {
               // (only do the following when still connected, not when we have already been disconnected and there is still data)
               // We already have some HTTP body data. We don't read more from the socket until
               // this is fetched by QHttpNetworkAccessHttpBackend. If we would read more,
               // we could not limit our read buffer usage.
               // We only do this when shouldEmitSignals==true because our HTTP parsing
               // always needs to parse the 401/407 replies. Therefore they don't really obey
               // to the read buffer maximum size, but we don't care since they should be small.
               return;
           }

           if (replyPrivate->userProvidedDownloadBuffer) {
               // the user provided a direct buffer where we should put all our data in.
               // this only works when we can tell the user the content length and he/she can allocate
               // the buffer in that size.
               // note that this call will read only from the still buffered data
               qint64 haveRead = replyPrivate->readBodyVeryFast(socket, replyPrivate->userProvidedDownloadBuffer + replyPrivate->totalProgress);
               bytes += haveRead;
               replyPrivate->totalProgress += haveRead;

               // the user will get notified of it via progress signal
               if (haveRead > 0)
                   emit reply->dataReadProgress(replyPrivate->totalProgress, replyPrivate->bodyLength);
           } else if (!replyPrivate->isChunked() && !replyPrivate->autoDecompress
                 && replyPrivate->bodyLength > 0) {
                 // bulk files like images should fulfill these properties and
                 // we can therefore save on memory copying
                qint64 haveRead = replyPrivate->readBodyFast(socket, &replyPrivate->responseData);
                bytes += haveRead;
                replyPrivate->totalProgress += haveRead;
                if (replyPrivate->shouldEmitSignals()) {
                    emit reply->readyRead();
                    emit reply->dataReadProgress(replyPrivate->totalProgress, replyPrivate->bodyLength);
                }
            }
            else
            {
                // use the traditional slower reading (for compressed encoding, chunked encoding,
                // no content-length etc)
                QByteDataBuffer byteDatas;
                qint64 haveRead = replyPrivate->readBody(socket, &byteDatas);
                if (haveRead) {
                    bytes += haveRead;
                    if (replyPrivate->autoDecompress)
                        replyPrivate->appendCompressedReplyData(byteDatas);
                    else
                        replyPrivate->appendUncompressedReplyData(byteDatas);

                    if (!replyPrivate->autoDecompress) {
                        replyPrivate->totalProgress += bytes;
                        if (replyPrivate->shouldEmitSignals()) {
                            // important: At the point of this readyRead(), the byteDatas list must be empty,
                            // else implicit sharing will trigger memcpy when the user is reading data!
                            emit reply->readyRead();
                            emit reply->dataReadProgress(replyPrivate->totalProgress, replyPrivate->bodyLength);
                        }
                    }
#ifndef QT_NO_COMPRESS
                    else if (!expand(false)) { // expand a chunk if possible
                        // If expand() failed we can just return, it had already called connection->emitReplyError()
                        return;
                    }
#endif
                }
            }
            // still in ReadingDataState? This function will be called again by the socket's readyRead
            if (replyPrivate->state == QHttpNetworkReplyPrivate::ReadingDataState)
                break;

            // everything done, fall through
            }
      case QHttpNetworkReplyPrivate::AllDoneState:
            allDone();
            break;
        default:
            break;
        }
    } while (bytes != lastBytes && reply);
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
            if (!value.isEmpty())
                socket->setProperty("_q_user-agent", value);
        }
#endif
        if (ssl) {
#ifndef QT_NO_OPENSSL
            QSslSocket *sslSocket = qobject_cast<QSslSocket*>(socket);
            sslSocket->connectToHostEncrypted(connectHost, connectPort);
            if (ignoreAllSslErrors)
                sslSocket->ignoreSslErrors();
            sslSocket->ignoreSslErrors(ignoreSslErrorsList);

            // limit the socket read buffer size. we will read everything into
            // the QHttpNetworkReply anyway, so let's grow only that and not
            // here and there.
            socket->setReadBufferSize(64*1024);
#else
            connection->d_func()->emitReplyError(socket, reply, QNetworkReply::ProtocolUnknownError);
#endif
        } else {
            // In case of no proxy we can use the Unbuffered QTcpSocket
#ifndef QT_NO_NETWORKPROXY
            if (connection->d_func()->networkProxy.type() == QNetworkProxy::NoProxy
                    && connection->cacheProxy().type() == QNetworkProxy::NoProxy
                    && connection->transparentProxy().type() == QNetworkProxy::NoProxy) {
#endif
                socket->connectToHost(connectHost, connectPort, QIODevice::ReadWrite | QIODevice::Unbuffered);
                // For an Unbuffered QTcpSocket, the read buffer size has a special meaning.
                socket->setReadBufferSize(1*1024);
#ifndef QT_NO_NETWORKPROXY
            } else {
                socket->connectToHost(connectHost, connectPort);

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


#ifndef QT_NO_COMPRESS
bool QHttpNetworkConnectionChannel::expand(bool dataComplete)
{
    Q_ASSERT(socket);
    Q_ASSERT(reply);

    qint64 total = reply->d_func()->compressedData.size();
    if (total >= CHUNK || dataComplete) {
         // uncompress the data
        QByteArray content, inflated;
        content = reply->d_func()->compressedData;
        reply->d_func()->compressedData.clear();

        int ret = Z_OK;
        if (content.size())
            ret = reply->d_func()->gunzipBodyPartially(content, inflated);
        if (ret >= Z_OK) {
            if (dataComplete && ret == Z_OK && !reply->d_func()->streamEnd) {
                reply->d_func()->gunzipBodyPartiallyEnd();
                reply->d_func()->streamEnd = true;
            }
            if (inflated.size()) {
                reply->d_func()->totalProgress += inflated.size();
                reply->d_func()->appendUncompressedReplyData(inflated);
                if (reply->d_func()->shouldEmitSignals()) {
                    // important: At the point of this readyRead(), inflated must be cleared,
                    // else implicit sharing will trigger memcpy when the user is reading data!
                    emit reply->readyRead();
                    emit reply->dataReadProgress(reply->d_func()->totalProgress, 0);
                }
            }
        } else {
            connection->d_func()->emitReplyError(socket, reply, QNetworkReply::ProtocolFailure);
            return false;
        }
    }
    return true;
}
#endif


void QHttpNetworkConnectionChannel::allDone()
{
    Q_ASSERT(reply);
#ifndef QT_NO_COMPRESS
    // expand the whole data.
    if (reply->d_func()->expectContent() && reply->d_func()->autoDecompress && !reply->d_func()->streamEnd) {
        bool expandResult = expand(true);
        // If expand() failed we can just return, it had already called connection->emitReplyError()
        if (!expandResult)
            return;
    }
#endif

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

//private slots
void QHttpNetworkConnectionChannel::_q_readyRead()
{
    if (socket->state() == QAbstractSocket::ConnectedState && socket->bytesAvailable() == 0) {
        // We got a readyRead but no bytes are available..
        // This happens for the Unbuffered QTcpSocket
        // Also check if socket is in ConnectedState since
        // this function may also be invoked via the event loop.
        char c;
        qint64  ret = socket->peek(&c, 1);
        if (ret < 0) {
            _q_error(socket->error());
            // We still need to handle the reply so it emits its signals etc.
            if (reply)
                _q_receiveReply();
            return;
        }
    }

    if (isSocketWaiting() || isSocketReading()) {
        state = QHttpNetworkConnectionChannel::ReadingState;
        if (reply)
            _q_receiveReply();
    }
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
    if (!pendingEncrypt) {
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
    default:
        // all other errors are treated as NetworkError
        errorCode = QNetworkReply::UnknownNetworkError;
        break;
    }
    QPointer<QHttpNetworkConnection> that = connection;
    QString errorString = connection->d_func()->errorDetail(errorCode, socket, socket->errorString());

    // Need to dequeu the request so that we can emit the error.
    if (!reply)
        connection->d_func()->dequeueRequest(socket);
    if (reply) {
        reply->d_func()->errorString = errorString;
        emit reply->finishedWithError(errorCode, errorString);
        reply = 0;
    }
    // send the next request
    QMetaObject::invokeMethod(that, "_q_startNextRequest", Qt::QueuedConnection);

    if (that) //signal emission triggered event loop
        close();
}

#ifndef QT_NO_NETWORKPROXY
void QHttpNetworkConnectionChannel::_q_proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator* auth)
{
    // Need to dequeue the request before we can emit the error.
    if (!reply)
        connection->d_func()->dequeueRequest(socket);
    if (reply)
        connection->d_func()->emitProxyAuthenticationRequired(this, proxy, auth);
}
#endif

void QHttpNetworkConnectionChannel::_q_uploadDataReadyRead()
{
    sendRequest();
}

#ifndef QT_NO_OPENSSL
void QHttpNetworkConnectionChannel::_q_encrypted()
{
    if (!socket)
        return; // ### error
    state = QHttpNetworkConnectionChannel::IdleState;
    pendingEncrypt = false;
    if (!reply)
        connection->d_func()->dequeueRequest(socket);
    if (reply)
        sendRequest();
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
    if (reply)
        emit reply->sslErrors(errors);
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
