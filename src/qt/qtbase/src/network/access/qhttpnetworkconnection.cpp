/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qhttpnetworkconnection_p.h"
#include <private/qabstractsocket_p.h>
#include "qhttpnetworkconnectionchannel_p.h"
#include "private/qnoncontiguousbytedevice_p.h"
#include <private/qnetworkrequest_p.h>
#include <private/qobject_p.h>
#include <private/qauthenticator_p.h>
#include "private/qhostinfo_p.h"
#include <qnetworkproxy.h>
#include <qauthenticator.h>
#include <qcoreapplication.h>

#include <qbuffer.h>
#include <qpair.h>
#include <qdebug.h>

#ifndef QT_NO_HTTP

#ifndef QT_NO_SSL
#    include <private/qsslsocket_p.h>
#    include <QtNetwork/qsslkey.h>
#    include <QtNetwork/qsslcipher.h>
#    include <QtNetwork/qsslconfiguration.h>
#endif



QT_BEGIN_NAMESPACE

const int QHttpNetworkConnectionPrivate::defaultHttpChannelCount = 6;

// The pipeline length. So there will be 4 requests in flight.
const int QHttpNetworkConnectionPrivate::defaultPipelineLength = 3;
// Only re-fill the pipeline if there's defaultRePipelineLength slots free in the pipeline.
// This means that there are 2 requests in flight and 2 slots free that will be re-filled.
const int QHttpNetworkConnectionPrivate::defaultRePipelineLength = 2;


QHttpNetworkConnectionPrivate::QHttpNetworkConnectionPrivate(const QString &hostName,
                                                             quint16 port, bool encrypt,
                                                             QHttpNetworkConnection::ConnectionType type)
: state(RunningState),
  networkLayerState(Unknown),
  hostName(hostName), port(port), encrypt(encrypt), delayIpv4(true)
#ifndef QT_NO_SSL
, channelCount((type == QHttpNetworkConnection::ConnectionTypeSPDY) ? 1 : defaultHttpChannelCount)
#else
, channelCount(defaultHttpChannelCount)
#endif // QT_NO_SSL
#ifndef QT_NO_NETWORKPROXY
  , networkProxy(QNetworkProxy::NoProxy)
#endif
  , preConnectRequests(0)
  , connectionType(type)
{
    channels = new QHttpNetworkConnectionChannel[channelCount];
}

QHttpNetworkConnectionPrivate::QHttpNetworkConnectionPrivate(quint16 channelCount, const QString &hostName,
                                                             quint16 port, bool encrypt,
                                                             QHttpNetworkConnection::ConnectionType type)
: state(RunningState), networkLayerState(Unknown),
  hostName(hostName), port(port), encrypt(encrypt), delayIpv4(true),
  channelCount(channelCount)
#ifndef QT_NO_NETWORKPROXY
  , networkProxy(QNetworkProxy::NoProxy)
#endif
  , preConnectRequests(0)
  , connectionType(type)
{
    channels = new QHttpNetworkConnectionChannel[channelCount];
}



QHttpNetworkConnectionPrivate::~QHttpNetworkConnectionPrivate()
{
    for (int i = 0; i < channelCount; ++i) {
        if (channels[i].socket) {
            channels[i].socket->close();
            delete channels[i].socket;
        }
    }
    delete []channels;
}

void QHttpNetworkConnectionPrivate::init()
{
    Q_Q(QHttpNetworkConnection);
    for (int i = 0; i < channelCount; i++) {
        channels[i].setConnection(this->q_func());
        channels[i].ssl = encrypt;
#ifndef QT_NO_BEARERMANAGEMENT
        //push session down to channels
        channels[i].networkSession = networkSession;
#endif
    }

    delayedConnectionTimer.setSingleShot(true);
    QObject::connect(&delayedConnectionTimer, SIGNAL(timeout()), q, SLOT(_q_connectDelayedChannel()));
}

void QHttpNetworkConnectionPrivate::pauseConnection()
{
    state = PausedState;

    // Disable all socket notifiers
    for (int i = 0; i < channelCount; i++) {
        if (channels[i].socket) {
#ifndef QT_NO_SSL
            if (encrypt)
                QSslSocketPrivate::pauseSocketNotifiers(static_cast<QSslSocket*>(channels[i].socket));
            else
#endif
                QAbstractSocketPrivate::pauseSocketNotifiers(channels[i].socket);
        }
    }
}

void QHttpNetworkConnectionPrivate::resumeConnection()
{
    state = RunningState;
    // Enable all socket notifiers
    for (int i = 0; i < channelCount; i++) {
        if (channels[i].socket) {
#ifndef QT_NO_SSL
            if (encrypt)
                QSslSocketPrivate::resumeSocketNotifiers(static_cast<QSslSocket*>(channels[i].socket));
            else
#endif
                QAbstractSocketPrivate::resumeSocketNotifiers(channels[i].socket);

            // Resume pending upload if needed
            if (channels[i].state == QHttpNetworkConnectionChannel::WritingState)
                QMetaObject::invokeMethod(&channels[i], "_q_uploadDataReadyRead", Qt::QueuedConnection);
        }
    }

    // queue _q_startNextRequest
    QMetaObject::invokeMethod(this->q_func(), "_q_startNextRequest", Qt::QueuedConnection);
}

int QHttpNetworkConnectionPrivate::indexOf(QAbstractSocket *socket) const
{
    for (int i = 0; i < channelCount; ++i)
        if (channels[i].socket == socket)
            return i;

    qFatal("Called with unknown socket object.");
    return 0;
}

// If the connection is in the HostLookupPendening state channel errors should not always be
// emitted. This function will check the status of the connection channels if we
// have not decided the networkLayerState and will return true if the channel error
// should be emitted by the channel.
bool QHttpNetworkConnectionPrivate::shouldEmitChannelError(QAbstractSocket *socket)
{
    Q_Q(QHttpNetworkConnection);

    bool emitError = true;
    int i = indexOf(socket);
    int otherSocket = (i == 0 ? 1 : 0);

    // If the IPv4 connection still isn't started we need to start it now.
    if (delayedConnectionTimer.isActive()) {
        delayedConnectionTimer.stop();
        channels[otherSocket].ensureConnection();
    }

    if (channelCount == 1) {
        if (networkLayerState == HostLookupPending || networkLayerState == IPv4or6)
            networkLayerState = QHttpNetworkConnectionPrivate::Unknown;
        channels[0].close();
        emitError = true;
    } else {
        if (networkLayerState == HostLookupPending || networkLayerState == IPv4or6) {
            if (channels[otherSocket].isSocketBusy() && (channels[otherSocket].state != QHttpNetworkConnectionChannel::ClosingState)) {
                // this was the first socket to fail.
                channels[i].close();
                emitError = false;
            }
            else {
                // Both connection attempts has failed.
                networkLayerState = QHttpNetworkConnectionPrivate::Unknown;
                channels[i].close();
                emitError = true;
            }
        } else {
            if (((networkLayerState == QHttpNetworkConnectionPrivate::IPv4) && (channels[i].networkLayerPreference != QAbstractSocket::IPv4Protocol))
                || ((networkLayerState == QHttpNetworkConnectionPrivate::IPv6) && (channels[i].networkLayerPreference != QAbstractSocket::IPv6Protocol))) {
                // First connection worked so this is the second one to complete and it failed.
                channels[i].close();
                QMetaObject::invokeMethod(q, "_q_startNextRequest", Qt::QueuedConnection);
                emitError = false;
            }
            if (networkLayerState == QHttpNetworkConnectionPrivate::Unknown)
                qWarning() << "We got a connection error when networkLayerState is Unknown";
        }
    }
    return emitError;
}


qint64 QHttpNetworkConnectionPrivate::uncompressedBytesAvailable(const QHttpNetworkReply &reply) const
{
    return reply.d_func()->responseData.byteAmount();
}

qint64 QHttpNetworkConnectionPrivate::uncompressedBytesAvailableNextBlock(const QHttpNetworkReply &reply) const
{
    return reply.d_func()->responseData.sizeNextBlock();
}

void QHttpNetworkConnectionPrivate::prepareRequest(HttpMessagePair &messagePair)
{
    QHttpNetworkRequest &request = messagePair.first;
    QHttpNetworkReply *reply = messagePair.second;

    // add missing fields for the request
    QByteArray value;
    // check if Content-Length is provided
    QNonContiguousByteDevice* uploadByteDevice = request.uploadByteDevice();
    if (uploadByteDevice) {
        if (request.contentLength() != -1 && uploadByteDevice->size() != -1) {
            // both values known, take the smaller one.
            request.setContentLength(qMin(uploadByteDevice->size(), request.contentLength()));
        } else if (request.contentLength() == -1 && uploadByteDevice->size() != -1) {
            // content length not supplied by user, but the upload device knows it
            request.setContentLength(uploadByteDevice->size());
        } else if (request.contentLength() != -1 && uploadByteDevice->size() == -1) {
            // everything OK, the user supplied us the contentLength
        } else if (request.contentLength() == -1 && uploadByteDevice->size() == -1) {
            qFatal("QHttpNetworkConnectionPrivate: Neither content-length nor upload device size were given");
        }
    }
    // set the Connection/Proxy-Connection: Keep-Alive headers
#ifndef QT_NO_NETWORKPROXY
    if (networkProxy.type() == QNetworkProxy::HttpCachingProxy)  {
        value = request.headerField("proxy-connection");
        if (value.isEmpty())
            request.setHeaderField("Proxy-Connection", "Keep-Alive");
    } else {
#endif
        value = request.headerField("connection");
        if (value.isEmpty())
            request.setHeaderField("Connection", "Keep-Alive");
#ifndef QT_NO_NETWORKPROXY
    }
#endif

    // If the request had a accept-encoding set, we better not mess
    // with it. If it was not set, we announce that we understand gzip
    // and remember this fact in request.d->autoDecompress so that
    // we can later decompress the HTTP reply if it has such an
    // encoding.
    value = request.headerField("accept-encoding");
    if (value.isEmpty()) {
#ifndef QT_NO_COMPRESS
        request.setHeaderField("Accept-Encoding", "gzip, deflate");
        request.d->autoDecompress = true;
#else
        // if zlib is not available set this to false always
        request.d->autoDecompress = false;
#endif
    }

    // some websites mandate an accept-language header and fail
    // if it is not sent. This is a problem with the website and
    // not with us, but we work around this by setting
    // one always.
    value = request.headerField("accept-language");
    if (value.isEmpty()) {
        QString systemLocale = QLocale::system().name().replace(QChar::fromLatin1('_'),QChar::fromLatin1('-'));
        QString acceptLanguage;
        if (systemLocale == QLatin1String("C"))
            acceptLanguage = QString::fromLatin1("en,*");
        else if (systemLocale.startsWith(QLatin1String("en-")))
            acceptLanguage = QString::fromLatin1("%1,*").arg(systemLocale);
        else
            acceptLanguage = QString::fromLatin1("%1,en,*").arg(systemLocale);
        request.setHeaderField("Accept-Language", acceptLanguage.toLatin1());
    }

    // set the User Agent
    value = request.headerField("user-agent");
    if (value.isEmpty())
        request.setHeaderField("User-Agent", "Mozilla/5.0");
    // set the host
    value = request.headerField("host");
    if (value.isEmpty()) {
        QHostAddress add;
        QByteArray host;
        if (add.setAddress(hostName)) {
            if (add.protocol() == QAbstractSocket::IPv6Protocol)
                host = "[" + hostName.toLatin1() + "]";//format the ipv6 in the standard way
            else
                host = hostName.toLatin1();

        } else {
            host = QUrl::toAce(hostName);
        }

        int port = request.url().port();
        if (port != -1) {
            host += ':';
            host += QByteArray::number(port);
        }

        request.setHeaderField("Host", host);
    }

    reply->d_func()->requestIsPrepared = true;
}




void QHttpNetworkConnectionPrivate::emitReplyError(QAbstractSocket *socket,
                                                   QHttpNetworkReply *reply,
                                                   QNetworkReply::NetworkError errorCode)
{
    Q_Q(QHttpNetworkConnection);

    int i = 0;
    if (socket)
        i = indexOf(socket);

    if (reply) {
        // this error matters only to this reply
        reply->d_func()->errorString = errorDetail(errorCode, socket);
        emit reply->finishedWithError(errorCode, reply->d_func()->errorString);
        // remove the corrupt data if any
        reply->d_func()->eraseData();

        // Clean the channel
        channels[i].close();
        channels[i].reply = 0;
        if (channels[i].protocolHandler)
            channels[i].protocolHandler->setReply(0);
        channels[i].request = QHttpNetworkRequest();
        if (socket)
            channels[i].requeueCurrentlyPipelinedRequests();

        // send the next request
        QMetaObject::invokeMethod(q, "_q_startNextRequest", Qt::QueuedConnection);
    }
}

void QHttpNetworkConnectionPrivate::copyCredentials(int fromChannel, QAuthenticator *auth, bool isProxy)
{
    Q_ASSERT(auth);

    // NTLM is a multi phase authentication. Copying credentials between authenticators would mess things up.
    if (!isProxy && channels[fromChannel].authMethod == QAuthenticatorPrivate::Ntlm)
        return;
    if (isProxy && channels[fromChannel].proxyAuthMethod == QAuthenticatorPrivate::Ntlm)
        return;


    // select another channel
    QAuthenticator* otherAuth = 0;
    for (int i = 0; i < channelCount; ++i) {
        if (i == fromChannel)
            continue;
        if (isProxy)
            otherAuth = &channels[i].proxyAuthenticator;
        else
            otherAuth = &channels[i].authenticator;
        // if the credentials are different, copy them
        if (otherAuth->user().compare(auth->user()))
            otherAuth->setUser(auth->user());
        if (otherAuth->password().compare(auth->password()))
            otherAuth->setPassword(auth->password());
    }
}


// handles the authentication for one channel and eventually re-starts the other channels
bool QHttpNetworkConnectionPrivate::handleAuthenticateChallenge(QAbstractSocket *socket, QHttpNetworkReply *reply,
                                                                bool isProxy, bool &resend)
{
    Q_ASSERT(socket);
    Q_ASSERT(reply);

    resend = false;
    //create the response header to be used with QAuthenticatorPrivate.
    QList<QPair<QByteArray, QByteArray> > fields = reply->header();

    //find out the type of authentication protocol requested.
    QAuthenticatorPrivate::Method authMethod = reply->d_func()->authenticationMethod(isProxy);
    if (authMethod != QAuthenticatorPrivate::None) {
        int i = indexOf(socket);
        //Use a single authenticator for all domains. ### change later to use domain/realm
        QAuthenticator* auth = 0;
        if (isProxy) {
            auth = &channels[i].proxyAuthenticator;
            channels[i].proxyAuthMethod = authMethod;
        } else {
            auth = &channels[i].authenticator;
            channels[i].authMethod = authMethod;
        }
        //proceed with the authentication.
        if (auth->isNull())
            auth->detach();
        QAuthenticatorPrivate *priv = QAuthenticatorPrivate::getPrivate(*auth);
        priv->parseHttpResponse(fields, isProxy);

        if (priv->phase == QAuthenticatorPrivate::Done) {
            pauseConnection();
            if (!isProxy) {
                if (channels[i].authenticationCredentialsSent) {
                    auth->detach();
                    priv = QAuthenticatorPrivate::getPrivate(*auth);
                    priv->hasFailed = true;
                    priv->phase = QAuthenticatorPrivate::Done;
                    channels[i].authenticationCredentialsSent = false;
                }
                emit reply->authenticationRequired(reply->request(), auth);
#ifndef QT_NO_NETWORKPROXY
            } else {
                if (channels[i].proxyCredentialsSent) {
                    auth->detach();
                    priv = QAuthenticatorPrivate::getPrivate(*auth);
                    priv->hasFailed = true;
                    priv->phase = QAuthenticatorPrivate::Done;
                    channels[i].proxyCredentialsSent = false;
                }
                emit reply->proxyAuthenticationRequired(networkProxy, auth);
#endif
            }
            resumeConnection();

            if (priv->phase != QAuthenticatorPrivate::Done) {
                // send any pending requests
                copyCredentials(i,  auth, isProxy);
            }
        } else if (priv->phase == QAuthenticatorPrivate::Start) {
            // If the url's authenticator has a 'user' set we will end up here (phase is only set to 'Done' by
            // parseHttpResponse above if 'user' is empty). So if credentials were supplied with the request,
            // such as in the case of an XMLHttpRequest, this is our only opportunity to cache them.
            emit reply->cacheCredentials(reply->request(), auth);
        }
        // - Changing values in QAuthenticator will reset the 'phase'. Therefore if it is still "Done"
        //   then nothing was filled in by the user or the cache
        // - If withCredentials has been set to false (e.g. by Qt WebKit for a cross-origin XMLHttpRequest) then
        //   we need to bail out if authentication is required.
        if (priv->phase == QAuthenticatorPrivate::Done || !reply->request().withCredentials()) {
            // Reset authenticator so the next request on that channel does not get messed up
            auth = 0;
            if (isProxy)
                channels[i].proxyAuthenticator = QAuthenticator();
            else
                channels[i].authenticator = QAuthenticator();

            // authentication is cancelled, send the current contents to the user.
            emit channels[i].reply->headerChanged();
            emit channels[i].reply->readyRead();
            QNetworkReply::NetworkError errorCode =
                isProxy
                ? QNetworkReply::ProxyAuthenticationRequiredError
                : QNetworkReply::AuthenticationRequiredError;
            reply->d_func()->errorString = errorDetail(errorCode, socket);
            emit reply->finishedWithError(errorCode, reply->d_func()->errorString);
            // ### at this point the reply could be deleted
            return true;
        }
        //resend the request
        resend = true;
        return true;
    }
    return false;
}

void QHttpNetworkConnectionPrivate::createAuthorization(QAbstractSocket *socket, QHttpNetworkRequest &request)
{
    Q_ASSERT(socket);

    int i = indexOf(socket);

    // Send "Authorization" header, but not if it's NTLM and the socket is already authenticated.
    if (channels[i].authMethod != QAuthenticatorPrivate::None) {
        if ((channels[i].authMethod != QAuthenticatorPrivate::Ntlm && request.headerField("Authorization").isEmpty()) || channels[i].lastStatus == 401) {
            QAuthenticatorPrivate *priv = QAuthenticatorPrivate::getPrivate(channels[i].authenticator);
            if (priv && priv->method != QAuthenticatorPrivate::None) {
                QByteArray response = priv->calculateResponse(request.methodName(), request.uri(false));
                request.setHeaderField("Authorization", response);
                channels[i].authenticationCredentialsSent = true;
            }
        }
    }

    // Send "Proxy-Authorization" header, but not if it's NTLM and the socket is already authenticated.
    if (channels[i].proxyAuthMethod != QAuthenticatorPrivate::None) {
        if (!(channels[i].proxyAuthMethod == QAuthenticatorPrivate::Ntlm && channels[i].lastStatus != 407)) {
            QAuthenticatorPrivate *priv = QAuthenticatorPrivate::getPrivate(channels[i].proxyAuthenticator);
            if (priv && priv->method != QAuthenticatorPrivate::None) {
                QByteArray response = priv->calculateResponse(request.methodName(), request.uri(false));
                request.setHeaderField("Proxy-Authorization", response);
                channels[i].proxyCredentialsSent = true;
            }
        }
    }
}

QHttpNetworkReply* QHttpNetworkConnectionPrivate::queueRequest(const QHttpNetworkRequest &request)
{
    Q_Q(QHttpNetworkConnection);

    // The reply component of the pair is created initially.
    QHttpNetworkReply *reply = new QHttpNetworkReply(request.url());
    reply->setRequest(request);
    reply->d_func()->connection = q;
    reply->d_func()->connectionChannel = &channels[0]; // will have the correct one set later
    HttpMessagePair pair = qMakePair(request, reply);

    if (request.isPreConnect())
        preConnectRequests++;

    if (connectionType == QHttpNetworkConnection::ConnectionTypeHTTP) {
        switch (request.priority()) {
        case QHttpNetworkRequest::HighPriority:
            highPriorityQueue.prepend(pair);
            break;
        case QHttpNetworkRequest::NormalPriority:
        case QHttpNetworkRequest::LowPriority:
            lowPriorityQueue.prepend(pair);
            break;
        }
    }
#ifndef QT_NO_SSL
    else { // SPDY
        if (!pair.second->d_func()->requestIsPrepared)
            prepareRequest(pair);
        channels[0].spdyRequestsToSend.insertMulti(request.priority(), pair);
    }
#endif // QT_NO_SSL

    // For Happy Eyeballs the networkLayerState is set to Unknown
    // untill we have started the first connection attempt. So no
    // request will be started untill we know if IPv4 or IPv6
    // should be used.
    if (networkLayerState == Unknown || networkLayerState == HostLookupPending) {
        startHostInfoLookup();
    } else if ( networkLayerState == IPv4 || networkLayerState == IPv6 ) {
        // this used to be called via invokeMethod and a QueuedConnection
        // It is the only place _q_startNextRequest is called directly without going
        // through the event loop using a QueuedConnection.
        // This is dangerous because of recursion that might occur when emitting
        // signals as DirectConnection from this code path. Therefore all signal
        // emissions that can come out from this code path need to
        // be QueuedConnection.
        // We are currently trying to fine-tune this.
        _q_startNextRequest();
    }
    return reply;
}

void QHttpNetworkConnectionPrivate::requeueRequest(const HttpMessagePair &pair)
{
    Q_Q(QHttpNetworkConnection);

    QHttpNetworkRequest request = pair.first;
    switch (request.priority()) {
    case QHttpNetworkRequest::HighPriority:
        highPriorityQueue.prepend(pair);
        break;
    case QHttpNetworkRequest::NormalPriority:
    case QHttpNetworkRequest::LowPriority:
        lowPriorityQueue.prepend(pair);
        break;
    }

    QMetaObject::invokeMethod(q, "_q_startNextRequest", Qt::QueuedConnection);
}

bool QHttpNetworkConnectionPrivate::dequeueRequest(QAbstractSocket *socket)
{
    int i = 0;
    if (socket)
        i = indexOf(socket);

    if (!highPriorityQueue.isEmpty()) {
        // remove from queue before sendRequest! else we might pipeline the same request again
        HttpMessagePair messagePair = highPriorityQueue.takeLast();
        if (!messagePair.second->d_func()->requestIsPrepared)
            prepareRequest(messagePair);
        channels[i].request = messagePair.first;
        channels[i].reply = messagePair.second;
        return true;
    }

    if (!lowPriorityQueue.isEmpty()) {
        // remove from queue before sendRequest! else we might pipeline the same request again
        HttpMessagePair messagePair = lowPriorityQueue.takeLast();
        if (!messagePair.second->d_func()->requestIsPrepared)
            prepareRequest(messagePair);
        channels[i].request = messagePair.first;
        channels[i].reply = messagePair.second;
        return true;
    }
    return false;
}

QHttpNetworkRequest QHttpNetworkConnectionPrivate::predictNextRequest()
{
    if (!highPriorityQueue.isEmpty())
        return highPriorityQueue.last().first;
    if (!lowPriorityQueue.isEmpty())
        return lowPriorityQueue.last().first;
    return QHttpNetworkRequest();
}

// this is called from _q_startNextRequest and when a request has been sent down a socket from the channel
void QHttpNetworkConnectionPrivate::fillPipeline(QAbstractSocket *socket)
{
    // return fast if there is nothing to pipeline
    if (highPriorityQueue.isEmpty() && lowPriorityQueue.isEmpty())
        return;

    int i = indexOf(socket);

    // return fast if there was no reply right now processed
    if (channels[i].reply == 0)
        return;

    if (! (defaultPipelineLength - channels[i].alreadyPipelinedRequests.length() >= defaultRePipelineLength)) {
        return;
    }

    if (channels[i].pipeliningSupported != QHttpNetworkConnectionChannel::PipeliningProbablySupported)
        return;

    // the current request that is in must already support pipelining
    if (!channels[i].request.isPipeliningAllowed())
        return;

    // the current request must be a idempotent (right now we only check GET)
    if (channels[i].request.operation() != QHttpNetworkRequest::Get)
        return;

    // check if socket is connected
    if (socket->state() != QAbstractSocket::ConnectedState)
        return;

    // check for resendCurrent
    if (channels[i].resendCurrent)
        return;

    // we do not like authentication stuff
    // ### make sure to be OK with this in later releases
    if (!channels[i].authenticator.isNull()
        && (!channels[i].authenticator.user().isEmpty()
            || !channels[i].authenticator.password().isEmpty()))
        return;
    if (!channels[i].proxyAuthenticator.isNull()
        && (!channels[i].proxyAuthenticator.user().isEmpty()
            || !channels[i].proxyAuthenticator.password().isEmpty()))
        return;

    // must be in ReadingState or WaitingState
    if (! (channels[i].state == QHttpNetworkConnectionChannel::WaitingState
           || channels[i].state == QHttpNetworkConnectionChannel::ReadingState))
        return;

    int lengthBefore;
    while (!highPriorityQueue.isEmpty()) {
        lengthBefore = channels[i].alreadyPipelinedRequests.length();
        fillPipeline(highPriorityQueue, channels[i]);

        if (channels[i].alreadyPipelinedRequests.length() >= defaultPipelineLength) {
            channels[i].pipelineFlush();
            return;
        }

        if (lengthBefore == channels[i].alreadyPipelinedRequests.length())
            break; // did not process anything, now do the low prio queue
    }

    while (!lowPriorityQueue.isEmpty()) {
        lengthBefore = channels[i].alreadyPipelinedRequests.length();
        fillPipeline(lowPriorityQueue, channels[i]);

        if (channels[i].alreadyPipelinedRequests.length() >= defaultPipelineLength) {
            channels[i].pipelineFlush();
            return;
        }

        if (lengthBefore == channels[i].alreadyPipelinedRequests.length())
            break; // did not process anything
    }


    channels[i].pipelineFlush();
}

// returns true when the processing of a queue has been done
bool QHttpNetworkConnectionPrivate::fillPipeline(QList<HttpMessagePair> &queue, QHttpNetworkConnectionChannel &channel)
{
    if (queue.isEmpty())
        return true;

    for (int i = queue.count() - 1; i >= 0; --i) {
        HttpMessagePair messagePair = queue.at(i);
        const QHttpNetworkRequest &request = messagePair.first;

        // we currently do not support pipelining if HTTP authentication is used
        if (!request.url().userInfo().isEmpty())
            continue;

        // take only GET requests
        if (request.operation() != QHttpNetworkRequest::Get)
            continue;

        if (!request.isPipeliningAllowed())
            continue;

        // remove it from the queue
        queue.takeAt(i);
        // we modify the queue we iterate over here, but since we return from the function
        // afterwards this is fine.

        // actually send it
        if (!messagePair.second->d_func()->requestIsPrepared)
            prepareRequest(messagePair);
        channel.pipelineInto(messagePair);

        // return false because we processed something and need to process again
        return false;
    }

    // return true, the queue has been processed and not changed
    return true;
}


QString QHttpNetworkConnectionPrivate::errorDetail(QNetworkReply::NetworkError errorCode, QAbstractSocket *socket, const QString &extraDetail)
{
    QString errorString;
    switch (errorCode) {
    case QNetworkReply::HostNotFoundError:
        if (socket)
            errorString = QCoreApplication::translate("QHttp", "Host %1 not found").arg(socket->peerName());
        else
            errorString = QCoreApplication::translate("QHttp", "Host %1 not found").arg(hostName);
        break;
    case QNetworkReply::ConnectionRefusedError:
        errorString = QCoreApplication::translate("QHttp", "Connection refused");
        break;
    case QNetworkReply::RemoteHostClosedError:
        errorString = QCoreApplication::translate("QHttp", "Connection closed");
        break;
    case QNetworkReply::TimeoutError:
        errorString = QCoreApplication::translate("QAbstractSocket", "Socket operation timed out");
        break;
    case QNetworkReply::ProxyAuthenticationRequiredError:
        errorString = QCoreApplication::translate("QHttp", "Proxy requires authentication");
        break;
    case QNetworkReply::AuthenticationRequiredError:
        errorString = QCoreApplication::translate("QHttp", "Host requires authentication");
        break;
    case QNetworkReply::ProtocolFailure:
        errorString = QCoreApplication::translate("QHttp", "Data corrupted");
        break;
    case QNetworkReply::ProtocolUnknownError:
        errorString = QCoreApplication::translate("QHttp", "Unknown protocol specified");
        break;
    case QNetworkReply::SslHandshakeFailedError:
        errorString = QCoreApplication::translate("QHttp", "SSL handshake failed");
        break;
    default:
        // all other errors are treated as QNetworkReply::UnknownNetworkError
        errorString = extraDetail;
        break;
    }
    return errorString;
}

// this is called from the destructor of QHttpNetworkReply. It is called when
// the reply was finished correctly or when it was aborted.
void QHttpNetworkConnectionPrivate::removeReply(QHttpNetworkReply *reply)
{
    Q_Q(QHttpNetworkConnection);

    // check if the reply is currently being processed or it is pipelined in
    for (int i = 0; i < channelCount; ++i) {
        // is the reply associated the currently processing of this channel?
        if (channels[i].reply == reply) {
            channels[i].reply = 0;
            if (channels[i].protocolHandler)
                channels[i].protocolHandler->setReply(0);
            channels[i].request = QHttpNetworkRequest();
            channels[i].resendCurrent = false;

            if (!reply->isFinished() && !channels[i].alreadyPipelinedRequests.isEmpty()) {
                // the reply had to be prematurely removed, e.g. it was not finished
                // therefore we have to requeue the already pipelined requests.
                channels[i].requeueCurrentlyPipelinedRequests();
            }

            // if HTTP mandates we should close
            // or the reply is not finished yet, e.g. it was aborted
            // we have to close that connection
            if (reply->d_func()->isConnectionCloseEnabled() || !reply->isFinished())
                channels[i].close();

            QMetaObject::invokeMethod(q, "_q_startNextRequest", Qt::QueuedConnection);
            return;
        }

        // is the reply inside the pipeline of this channel already?
        for (int j = 0; j < channels[i].alreadyPipelinedRequests.length(); j++) {
            if (channels[i].alreadyPipelinedRequests.at(j).second == reply) {
               // Remove that HttpMessagePair
               channels[i].alreadyPipelinedRequests.removeAt(j);

               channels[i].requeueCurrentlyPipelinedRequests();

               // Since some requests had already been pipelined, but we removed
               // one and re-queued the others
               // we must force a connection close after the request that is
               // currently in processing has been finished.
               if (channels[i].reply)
                   channels[i].reply->d_func()->forceConnectionCloseEnabled = true;

               QMetaObject::invokeMethod(q, "_q_startNextRequest", Qt::QueuedConnection);
               return;
            }
        }
#ifndef QT_NO_SSL
        // is the reply inside the SPDY pipeline of this channel already?
        QMultiMap<int, HttpMessagePair>::iterator it = channels[i].spdyRequestsToSend.begin();
        QMultiMap<int, HttpMessagePair>::iterator end = channels[i].spdyRequestsToSend.end();
        for (; it != end; ++it) {
            if (it.value().second == reply) {
                channels[i].spdyRequestsToSend.remove(it.key());

                QMetaObject::invokeMethod(q, "_q_startNextRequest", Qt::QueuedConnection);
                return;
            }
        }
#endif
    }
    // remove from the high priority queue
    if (!highPriorityQueue.isEmpty()) {
        for (int j = highPriorityQueue.count() - 1; j >= 0; --j) {
            HttpMessagePair messagePair = highPriorityQueue.at(j);
            if (messagePair.second == reply) {
                highPriorityQueue.removeAt(j);
                QMetaObject::invokeMethod(q, "_q_startNextRequest", Qt::QueuedConnection);
                return;
            }
        }
    }
    // remove from the low priority queue
    if (!lowPriorityQueue.isEmpty()) {
        for (int j = lowPriorityQueue.count() - 1; j >= 0; --j) {
            HttpMessagePair messagePair = lowPriorityQueue.at(j);
            if (messagePair.second == reply) {
                lowPriorityQueue.removeAt(j);
                QMetaObject::invokeMethod(q, "_q_startNextRequest", Qt::QueuedConnection);
                return;
            }
        }
    }
}



// This function must be called from the event loop. The only
// exception is documented in QHttpNetworkConnectionPrivate::queueRequest
// although it is called _q_startNextRequest, it will actually start multiple requests when possible
void QHttpNetworkConnectionPrivate::_q_startNextRequest()
{
    // If there is no network layer state decided we should not start any new requests.
    if (networkLayerState == Unknown || networkLayerState == HostLookupPending || networkLayerState == IPv4or6)
        return;

    // If the QHttpNetworkConnection is currently paused then bail out immediately
    if (state == PausedState)
        return;

    //resend the necessary ones.
    for (int i = 0; i < channelCount; ++i) {
        if (channels[i].resendCurrent && (channels[i].state != QHttpNetworkConnectionChannel::ClosingState)) {
            channels[i].resendCurrent = false;
            channels[i].state = QHttpNetworkConnectionChannel::IdleState;

            // if this is not possible, error will be emitted and connection terminated
            if (!channels[i].resetUploadData())
                continue;
            channels[i].sendRequest();
        }
    }

    // dequeue new ones

    switch (connectionType) {
    case QHttpNetworkConnection::ConnectionTypeHTTP: {
        // return fast if there is nothing to do
        if (highPriorityQueue.isEmpty() && lowPriorityQueue.isEmpty())
            return;

        // try to get a free AND connected socket
        for (int i = 0; i < channelCount; ++i) {
            if (channels[i].socket) {
                if (!channels[i].reply && !channels[i].isSocketBusy() && channels[i].socket->state() == QAbstractSocket::ConnectedState) {
                    if (dequeueRequest(channels[i].socket))
                        channels[i].sendRequest();
                }
            }
        }
        break;
    }
    case QHttpNetworkConnection::ConnectionTypeSPDY: {
#ifndef QT_NO_SSL
        if (channels[0].spdyRequestsToSend.isEmpty())
            return;

        if (networkLayerState == IPv4)
            channels[0].networkLayerPreference = QAbstractSocket::IPv4Protocol;
        else if (networkLayerState == IPv6)
            channels[0].networkLayerPreference = QAbstractSocket::IPv6Protocol;
        channels[0].ensureConnection();
        if (channels[0].socket && channels[0].socket->state() == QAbstractSocket::ConnectedState
                && !channels[0].pendingEncrypt)
            channels[0].sendRequest();
#endif // QT_NO_SSL
        break;
    }
    }

    // try to push more into all sockets
    // ### FIXME we should move this to the beginning of the function
    // as soon as QtWebkit is properly using the pipelining
    // (e.g. not for XMLHttpRequest or the first page load)
    // ### FIXME we should also divide the requests more even
    // on the connected sockets
    //tryToFillPipeline(socket);
    // return fast if there is nothing to pipeline
    if (highPriorityQueue.isEmpty() && lowPriorityQueue.isEmpty())
        return;
    for (int i = 0; i < channelCount; i++)
        if (channels[i].socket && channels[i].socket->state() == QAbstractSocket::ConnectedState)
            fillPipeline(channels[i].socket);

    // If there is not already any connected channels we need to connect a new one.
    // We do not pair the channel with the request until we know if it is
    // connected or not. This is to reuse connected channels before we connect new once.
    int queuedRequests = highPriorityQueue.count() + lowPriorityQueue.count();

    // in case we have in-flight preconnect requests and normal requests,
    // we only need one socket for each (preconnect, normal request) pair
    int neededOpenChannels = queuedRequests;
    if (preConnectRequests > 0) {
        int normalRequests = queuedRequests - preConnectRequests;
        neededOpenChannels = qMax(normalRequests, preConnectRequests);
    }
    for (int i = 0; i < channelCount && neededOpenChannels > 0; ++i) {
        bool connectChannel = false;
        if (channels[i].socket) {
            if ((channels[i].socket->state() == QAbstractSocket::ConnectingState)
                    || (channels[i].socket->state() == QAbstractSocket::HostLookupState)
                    || channels[i].pendingEncrypt) // pendingEncrypt == "EncryptingState"
                neededOpenChannels--;

            if (neededOpenChannels <= 0)
                break;
            if (!channels[i].reply && !channels[i].isSocketBusy() && (channels[i].socket->state() == QAbstractSocket::UnconnectedState))
                connectChannel = true;
        } else { // not previously used channel
            connectChannel = true;
        }

        if (connectChannel) {
            if (networkLayerState == IPv4)
                channels[i].networkLayerPreference = QAbstractSocket::IPv4Protocol;
            else if (networkLayerState == IPv6)
                channels[i].networkLayerPreference = QAbstractSocket::IPv6Protocol;
            channels[i].ensureConnection();
            neededOpenChannels--;
        }
    }
}


void QHttpNetworkConnectionPrivate::readMoreLater(QHttpNetworkReply *reply)
{
    for (int i = 0 ; i < channelCount; ++i) {
        if (channels[i].reply ==  reply) {
            // emulate a readyRead() from the socket
            QMetaObject::invokeMethod(&channels[i], "_q_readyRead", Qt::QueuedConnection);
            return;
        }
    }
}



// The first time we start the connection is used we do not know if we
// should use IPv4 or IPv6. So we start a hostlookup to figure this out.
// Later when we do the connection the socket will not need to do another
// lookup as then the hostinfo will already be in the cache.
void QHttpNetworkConnectionPrivate::startHostInfoLookup()
{
    networkLayerState = HostLookupPending;

    // check if we already now can decide if this is IPv4 or IPv6
    QString lookupHost = hostName;
#ifndef QT_NO_NETWORKPROXY
    if (networkProxy.capabilities() & QNetworkProxy::HostNameLookupCapability) {
        lookupHost = networkProxy.hostName();
    } else if (channels[0].proxy.capabilities() & QNetworkProxy::HostNameLookupCapability) {
        lookupHost = channels[0].proxy.hostName();
    }
#endif
    QHostAddress temp;
    if (temp.setAddress(lookupHost)) {
        if (temp.protocol() == QAbstractSocket::IPv4Protocol) {
            networkLayerState = QHttpNetworkConnectionPrivate::IPv4;
            QMetaObject::invokeMethod(this->q_func(), "_q_startNextRequest", Qt::QueuedConnection);
            return;
        } else if (temp.protocol() == QAbstractSocket::IPv6Protocol) {
            networkLayerState = QHttpNetworkConnectionPrivate::IPv6;
            QMetaObject::invokeMethod(this->q_func(), "_q_startNextRequest", Qt::QueuedConnection);
            return;
        }
    } else {
        int hostLookupId;
        bool immediateResultValid = false;
        QHostInfo hostInfo = qt_qhostinfo_lookup(lookupHost,
                                                 this->q_func(),
                                                 SLOT(_q_hostLookupFinished(QHostInfo)),
                                                 &immediateResultValid,
                                                 &hostLookupId);
        if (immediateResultValid) {
            _q_hostLookupFinished(hostInfo);
        }
    }
}


void QHttpNetworkConnectionPrivate::_q_hostLookupFinished(QHostInfo info)
{
    bool bIpv4 = false;
    bool bIpv6 = false;
    bool foundAddress = false;
    if (networkLayerState == IPv4 || networkLayerState == IPv6 || networkLayerState == IPv4or6)
        return;

    foreach (const QHostAddress &address, info.addresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol) {
            if (!foundAddress) {
                foundAddress = true;
                delayIpv4 = false;
            }
            bIpv4 = true;
        } else if (address.protocol() == QAbstractSocket::IPv6Protocol) {
            if (!foundAddress) {
                foundAddress = true;
                delayIpv4 = true;
            }
            bIpv6 = true;
        }
    }

    if (bIpv4 && bIpv6)
        startNetworkLayerStateLookup();
    else if (bIpv4) {
        networkLayerState = QHttpNetworkConnectionPrivate::IPv4;
        QMetaObject::invokeMethod(this->q_func(), "_q_startNextRequest", Qt::QueuedConnection);
    } else if (bIpv6) {
        networkLayerState = QHttpNetworkConnectionPrivate::IPv6;
        QMetaObject::invokeMethod(this->q_func(), "_q_startNextRequest", Qt::QueuedConnection);
    } else {
        if (dequeueRequest(channels[0].socket)) {
            emitReplyError(channels[0].socket, channels[0].reply, QNetworkReply::HostNotFoundError);
            networkLayerState = QHttpNetworkConnectionPrivate::Unknown;
        }
#ifndef QT_NO_SSL
        else if (connectionType == QHttpNetworkConnection::ConnectionTypeSPDY) {
            QList<HttpMessagePair> spdyPairs = channels[0].spdyRequestsToSend.values();
            for (int a = 0; a < spdyPairs.count(); ++a) {
                // emit error for all replies
                QHttpNetworkReply *currentReply = spdyPairs.at(a).second;
                Q_ASSERT(currentReply);
                emitReplyError(channels[0].socket, currentReply, QNetworkReply::HostNotFoundError);
            }
        }
#endif // QT_NO_SSL
        else {
            // Should not happen
            qWarning() << "QHttpNetworkConnectionPrivate::_q_hostLookupFinished could not dequeu request";
            networkLayerState = QHttpNetworkConnectionPrivate::Unknown;
        }
    }
}


// This will be used if the host lookup found both and Ipv4 and
// Ipv6 address. Then we will start up two connections and pick
// the network layer of the one that finish first. The second
// connection will then be disconnected.
void QHttpNetworkConnectionPrivate::startNetworkLayerStateLookup()
{
    if (channelCount > 1) {
        // At this time all channels should be unconnected.
        Q_ASSERT(!channels[0].isSocketBusy());
        Q_ASSERT(!channels[1].isSocketBusy());

        networkLayerState = IPv4or6;

        channels[0].networkLayerPreference = QAbstractSocket::IPv4Protocol;
        channels[1].networkLayerPreference = QAbstractSocket::IPv6Protocol;

        int timeout = 300;
#ifndef QT_NO_BEARERMANAGEMENT
        if (networkSession) {
            if (networkSession->configuration().bearerType() == QNetworkConfiguration::Bearer2G)
                timeout = 800;
            else if (networkSession->configuration().bearerType() == QNetworkConfiguration::BearerCDMA2000)
                timeout = 500;
            else if (networkSession->configuration().bearerType() == QNetworkConfiguration::BearerWCDMA)
                timeout = 500;
            else if (networkSession->configuration().bearerType() == QNetworkConfiguration::BearerHSPA)
                timeout = 400;
        }
#endif
        delayedConnectionTimer.start(timeout);
        if (delayIpv4)
            channels[1].ensureConnection();
        else
            channels[0].ensureConnection();
    } else {
        networkLayerState = IPv4or6;
        channels[0].networkLayerPreference = QAbstractSocket::AnyIPProtocol;
        channels[0].ensureConnection();
    }
}

void QHttpNetworkConnectionPrivate::networkLayerDetected(QAbstractSocket::NetworkLayerProtocol protocol)
{
    for (int i = 0 ; i < channelCount; ++i) {
        if ((channels[i].networkLayerPreference != protocol) && (channels[i].state == QHttpNetworkConnectionChannel::ConnectingState)) {
            channels[i].close();
        }
    }
}

void QHttpNetworkConnectionPrivate::_q_connectDelayedChannel()
{
    if (delayIpv4)
        channels[0].ensureConnection();
    else
        channels[1].ensureConnection();
}

#ifndef QT_NO_BEARERMANAGEMENT
QHttpNetworkConnection::QHttpNetworkConnection(const QString &hostName, quint16 port, bool encrypt,
                                               QHttpNetworkConnection::ConnectionType connectionType,
                                               QObject *parent, QSharedPointer<QNetworkSession> networkSession)
    : QObject(*(new QHttpNetworkConnectionPrivate(hostName, port, encrypt, connectionType)), parent)
{
    Q_D(QHttpNetworkConnection);
    d->networkSession = networkSession;
    d->init();
}

QHttpNetworkConnection::QHttpNetworkConnection(quint16 connectionCount, const QString &hostName,
                                               quint16 port, bool encrypt, QObject *parent,
                                               QSharedPointer<QNetworkSession> networkSession,
                                               QHttpNetworkConnection::ConnectionType connectionType)
     : QObject(*(new QHttpNetworkConnectionPrivate(connectionCount, hostName, port, encrypt,
                                                   connectionType)), parent)
{
    Q_D(QHttpNetworkConnection);
    d->networkSession = networkSession;
    d->init();
}
#else
QHttpNetworkConnection::QHttpNetworkConnection(const QString &hostName, quint16 port, bool encrypt, QObject *parent,
                                               QHttpNetworkConnection::ConnectionType connectionType)
    : QObject(*(new QHttpNetworkConnectionPrivate(hostName, port, encrypt , connectionType)), parent)
{
    Q_D(QHttpNetworkConnection);
    d->init();
}

QHttpNetworkConnection::QHttpNetworkConnection(quint16 connectionCount, const QString &hostName,
                                               quint16 port, bool encrypt, QObject *parent,
                                               QHttpNetworkConnection::ConnectionType connectionType)
     : QObject(*(new QHttpNetworkConnectionPrivate(connectionCount, hostName, port, encrypt,
                                                   connectionType)), parent)
{
    Q_D(QHttpNetworkConnection);
    d->init();
}
#endif

QHttpNetworkConnection::~QHttpNetworkConnection()
{
}

QString QHttpNetworkConnection::hostName() const
{
    Q_D(const QHttpNetworkConnection);
    return d->hostName;
}

quint16 QHttpNetworkConnection::port() const
{
    Q_D(const QHttpNetworkConnection);
    return d->port;
}

QHttpNetworkReply* QHttpNetworkConnection::sendRequest(const QHttpNetworkRequest &request)
{
    Q_D(QHttpNetworkConnection);
    return d->queueRequest(request);
}

bool QHttpNetworkConnection::isSsl() const
{
    Q_D(const QHttpNetworkConnection);
    return d->encrypt;
}

QHttpNetworkConnectionChannel *QHttpNetworkConnection::channels() const
{
    return d_func()->channels;
}

#ifndef QT_NO_NETWORKPROXY
void QHttpNetworkConnection::setCacheProxy(const QNetworkProxy &networkProxy)
{
    Q_D(QHttpNetworkConnection);
    d->networkProxy = networkProxy;
    // update the authenticator
    if (!d->networkProxy.user().isEmpty()) {
        for (int i = 0; i < d->channelCount; ++i) {
            d->channels[i].proxyAuthenticator.setUser(d->networkProxy.user());
            d->channels[i].proxyAuthenticator.setPassword(d->networkProxy.password());
        }
    }
}

QNetworkProxy QHttpNetworkConnection::cacheProxy() const
{
    Q_D(const QHttpNetworkConnection);
    return d->networkProxy;
}

void QHttpNetworkConnection::setTransparentProxy(const QNetworkProxy &networkProxy)
{
    Q_D(QHttpNetworkConnection);
    for (int i = 0; i < d->channelCount; ++i)
        d->channels[i].setProxy(networkProxy);
}

QNetworkProxy QHttpNetworkConnection::transparentProxy() const
{
    Q_D(const QHttpNetworkConnection);
    return d->channels[0].proxy;
}
#endif

QHttpNetworkConnection::ConnectionType QHttpNetworkConnection::connectionType()
{
    Q_D(QHttpNetworkConnection);
    return d->connectionType;
}

void QHttpNetworkConnection::setConnectionType(ConnectionType type)
{
    Q_D(QHttpNetworkConnection);
    d->connectionType = type;
}

// SSL support below
#ifndef QT_NO_SSL
void QHttpNetworkConnection::setSslConfiguration(const QSslConfiguration &config)
{
    Q_D(QHttpNetworkConnection);
    if (!d->encrypt)
        return;

    // set the config on all channels
    for (int i = 0; i < d->channelCount; ++i)
        d->channels[i].setSslConfiguration(config);
}

QSharedPointer<QSslContext> QHttpNetworkConnection::sslContext()
{
    Q_D(QHttpNetworkConnection);
    return d->sslContext;
}

void QHttpNetworkConnection::setSslContext(QSharedPointer<QSslContext> context)
{
    Q_D(QHttpNetworkConnection);
    d->sslContext = context;
}

void QHttpNetworkConnection::ignoreSslErrors(int channel)
{
    Q_D(QHttpNetworkConnection);
    if (!d->encrypt)
        return;

    if (channel == -1) { // ignore for all channels
        for (int i = 0; i < d->channelCount; ++i) {
            d->channels[i].ignoreSslErrors();
        }

    } else {
        d->channels[channel].ignoreSslErrors();
    }
}

void QHttpNetworkConnection::ignoreSslErrors(const QList<QSslError> &errors, int channel)
{
    Q_D(QHttpNetworkConnection);
    if (!d->encrypt)
        return;

    if (channel == -1) { // ignore for all channels
        for (int i = 0; i < d->channelCount; ++i) {
            d->channels[i].ignoreSslErrors(errors);
        }

    } else {
        d->channels[channel].ignoreSslErrors(errors);
    }
}

#endif //QT_NO_SSL

void QHttpNetworkConnection::preConnectFinished()
{
    d_func()->preConnectRequests--;
}

#ifndef QT_NO_NETWORKPROXY
// only called from QHttpNetworkConnectionChannel::_q_proxyAuthenticationRequired, not
// from QHttpNetworkConnectionChannel::handleAuthenticationChallenge
// e.g. it is for SOCKS proxies which require authentication.
void QHttpNetworkConnectionPrivate::emitProxyAuthenticationRequired(const QHttpNetworkConnectionChannel *chan, const QNetworkProxy &proxy, QAuthenticator* auth)
{
    // Also pause the connection because socket notifiers may fire while an user
    // dialog is displaying
    pauseConnection();
    QHttpNetworkReply *reply;
#ifndef QT_NO_SSL
    if (connectionType == QHttpNetworkConnection::ConnectionTypeSPDY) {
        // we choose the reply to emit the proxyAuth signal from somewhat arbitrarily,
        // but that does not matter because the signal will ultimately be emitted
        // by the QNetworkAccessManager.
        Q_ASSERT(chan->spdyRequestsToSend.count() > 0);
        reply = chan->spdyRequestsToSend.values().first().second;
    } else { // HTTP
#endif // QT_NO_SSL
        reply = chan->reply;
#ifndef QT_NO_SSL
    }
#endif // QT_NO_SSL

    Q_ASSERT(reply);
    emit reply->proxyAuthenticationRequired(proxy, auth);
    resumeConnection();
    int i = indexOf(chan->socket);
    copyCredentials(i, auth, true);
}
#endif


QT_END_NAMESPACE

#include "moc_qhttpnetworkconnection_p.cpp"

#endif // QT_NO_HTTP
