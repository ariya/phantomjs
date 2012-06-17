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

//#define QHTTPTHREADDELEGATE_DEBUG
#include "qhttpthreaddelegate_p.h"

#include <QThread>
#include <QTimer>
#include <QAuthenticator>
#include <QEventLoop>

#include "private/qhttpnetworkreply_p.h"
#include "private/qnetworkaccesscache_p.h"
#include "private/qnoncontiguousbytedevice_p.h"

#ifndef QT_NO_HTTP

QT_BEGIN_NAMESPACE

static QNetworkReply::NetworkError statusCodeFromHttp(int httpStatusCode, const QUrl &url)
{
    QNetworkReply::NetworkError code;
    // we've got an error
    switch (httpStatusCode) {
    case 401:               // Authorization required
        code = QNetworkReply::AuthenticationRequiredError;
        break;

    case 403:               // Access denied
        code = QNetworkReply::ContentOperationNotPermittedError;
        break;

    case 404:               // Not Found
        code = QNetworkReply::ContentNotFoundError;
        break;

    case 405:               // Method Not Allowed
        code = QNetworkReply::ContentOperationNotPermittedError;
        break;

    case 407:
        code = QNetworkReply::ProxyAuthenticationRequiredError;
        break;

    case 418:               // I'm a teapot
        code = QNetworkReply::ProtocolInvalidOperationError;
        break;


    default:
        if (httpStatusCode > 500) {
            // some kind of server error
            code = QNetworkReply::ProtocolUnknownError;
        } else if (httpStatusCode >= 400) {
            // content error we did not handle above
            code = QNetworkReply::UnknownContentError;
        } else {
            qWarning("QNetworkAccess: got HTTP status code %d which is not expected from url: \"%s\"",
                     httpStatusCode, qPrintable(url.toString()));
            code = QNetworkReply::ProtocolFailure;
        }
    }

    return code;
}


static QByteArray makeCacheKey(QUrl &url, QNetworkProxy *proxy)
{
    QByteArray result;
    QUrl copy = url;
    bool isEncrypted = copy.scheme().toLower() == QLatin1String("https");
    copy.setPort(copy.port(isEncrypted ? 443 : 80));
    result = copy.toEncoded(QUrl::RemoveUserInfo | QUrl::RemovePath |
                            QUrl::RemoveQuery | QUrl::RemoveFragment);

#ifndef QT_NO_NETWORKPROXY
    if (proxy && proxy->type() != QNetworkProxy::NoProxy) {
        QUrl key;

        switch (proxy->type()) {
        case QNetworkProxy::Socks5Proxy:
            key.setScheme(QLatin1String("proxy-socks5"));
            break;

        case QNetworkProxy::HttpProxy:
        case QNetworkProxy::HttpCachingProxy:
            key.setScheme(QLatin1String("proxy-http"));
            break;

        default:
            break;
        }

        if (!key.scheme().isEmpty()) {
            key.setUserName(proxy->user());
            key.setHost(proxy->hostName());
            key.setPort(proxy->port());
            key.setEncodedQuery(result);
            result = key.toEncoded();
        }
    }
#else
    Q_UNUSED(proxy)
#endif

    return "http-connection:" + result;
}

class QNetworkAccessCachedHttpConnection: public QHttpNetworkConnection,
                                      public QNetworkAccessCache::CacheableObject
{
    // Q_OBJECT
public:
#ifdef QT_NO_BEARERMANAGEMENT
    QNetworkAccessCachedHttpConnection(const QString &hostName, quint16 port, bool encrypt)
        : QHttpNetworkConnection(hostName, port, encrypt)
#else
    QNetworkAccessCachedHttpConnection(const QString &hostName, quint16 port, bool encrypt, QSharedPointer<QNetworkSession> networkSession)
        : QHttpNetworkConnection(hostName, port, encrypt, /*parent=*/0, networkSession)
#endif
    {
        setExpires(true);
        setShareable(true);
    }

    virtual void dispose()
    {
#if 0  // sample code; do this right with the API
        Q_ASSERT(!isWorking());
#endif
        delete this;
    }
};


QThreadStorage<QNetworkAccessCache *> QHttpThreadDelegate::connections;


QHttpThreadDelegate::~QHttpThreadDelegate()
{
    // It could be that the main thread has asked us to shut down, so we need to delete the HTTP reply
    if (httpReply) {
        delete httpReply;
    }

    // Get the object cache that stores our QHttpNetworkConnection objects
    // and release the entry for this QHttpNetworkConnection
    if (connections.hasLocalData() && !cacheKey.isEmpty()) {
        connections.localData()->releaseEntry(cacheKey);
    }
}


QHttpThreadDelegate::QHttpThreadDelegate(QObject *parent) :
    QObject(parent)
    , ssl(false)
    , downloadBufferMaximumSize(0)
    , pendingDownloadData(0)
    , pendingDownloadProgress(0)
    , synchronous(false)
    , incomingStatusCode(0)
    , isPipeliningUsed(false)
    , incomingContentLength(-1)
    , incomingErrorCode(QNetworkReply::NoError)
    , downloadBuffer(0)
    , httpConnection(0)
    , httpReply(0)
    , synchronousRequestLoop(0)
{
}

// This is invoked as BlockingQueuedConnection from QNetworkAccessHttpBackend in the user thread
void QHttpThreadDelegate::startRequestSynchronously()
{
#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::startRequestSynchronously() thread=" << QThread::currentThreadId();
#endif
    synchronous = true;

    QEventLoop synchronousRequestLoop;
    this->synchronousRequestLoop = &synchronousRequestLoop;

    // Worst case timeout
    QTimer::singleShot(30*1000, this, SLOT(abortRequest()));

    QMetaObject::invokeMethod(this, "startRequest", Qt::QueuedConnection);
    synchronousRequestLoop.exec();

    connections.localData()->releaseEntry(cacheKey);
    connections.setLocalData(0);

#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::startRequestSynchronously() thread=" << QThread::currentThreadId() << "finished";
#endif
}


// This is invoked as QueuedConnection from QNetworkAccessHttpBackend in the user thread
void QHttpThreadDelegate::startRequest()
{
#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::startRequest() thread=" << QThread::currentThreadId();
#endif
    // Check QThreadStorage for the QNetworkAccessCache
    // If not there, create this connection cache
    if (!connections.hasLocalData()) {
        connections.setLocalData(new QNetworkAccessCache());
    }

    // check if we have an open connection to this host
    QUrl urlCopy = httpRequest.url();
    urlCopy.setPort(urlCopy.port(ssl ? 443 : 80));

#ifndef QT_NO_NETWORKPROXY
    if (transparentProxy.type() != QNetworkProxy::NoProxy)
        cacheKey = makeCacheKey(urlCopy, &transparentProxy);
    else if (cacheProxy.type() != QNetworkProxy::NoProxy)
        cacheKey = makeCacheKey(urlCopy, &cacheProxy);
    else
#endif
        cacheKey = makeCacheKey(urlCopy, 0);


    // the http object is actually a QHttpNetworkConnection
    httpConnection = static_cast<QNetworkAccessCachedHttpConnection *>(connections.localData()->requestEntryNow(cacheKey));
    if (httpConnection == 0) {
        // no entry in cache; create an object
        // the http object is actually a QHttpNetworkConnection
#ifdef QT_NO_BEARERMANAGEMENT
        httpConnection = new QNetworkAccessCachedHttpConnection(urlCopy.host(), urlCopy.port(), ssl);
#else
        httpConnection = new QNetworkAccessCachedHttpConnection(urlCopy.host(), urlCopy.port(), ssl, networkSession);
#endif
#ifndef QT_NO_OPENSSL
        // Set the QSslConfiguration from this QNetworkRequest.
        if (ssl && incomingSslConfiguration != QSslConfiguration::defaultConfiguration()) {
            httpConnection->setSslConfiguration(incomingSslConfiguration);
        }
#endif

#ifndef QT_NO_NETWORKPROXY
        httpConnection->setTransparentProxy(transparentProxy);
        httpConnection->setCacheProxy(cacheProxy);
#endif

        // cache the QHttpNetworkConnection corresponding to this cache key
        connections.localData()->addEntry(cacheKey, httpConnection);
    }


    // Send the request to the connection
    httpReply = httpConnection->sendRequest(httpRequest);
    httpReply->setParent(this);

    // Connect the reply signals that we need to handle and then forward
    if (synchronous) {
        connect(httpReply,SIGNAL(headerChanged()), this, SLOT(synchronousHeaderChangedSlot()));
        connect(httpReply,SIGNAL(finished()), this, SLOT(synchronousFinishedSlot()));
        connect(httpReply,SIGNAL(finishedWithError(QNetworkReply::NetworkError, const QString)),
                this, SLOT(synchronousFinishedWithErrorSlot(QNetworkReply::NetworkError,QString)));

        connect(httpReply, SIGNAL(authenticationRequired(QHttpNetworkRequest,QAuthenticator*)),
                this, SLOT(synchronousAuthenticationRequiredSlot(QHttpNetworkRequest,QAuthenticator*)));
        connect(httpReply, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
                this, SLOT(synchronousProxyAuthenticationRequiredSlot(QNetworkProxy,QAuthenticator*)));

        // Don't care about ignored SSL errors for now in the synchronous HTTP case.
    } else if (!synchronous) {
        connect(httpReply,SIGNAL(headerChanged()), this, SLOT(headerChangedSlot()));
        connect(httpReply,SIGNAL(finished()), this, SLOT(finishedSlot()));
        connect(httpReply,SIGNAL(finishedWithError(QNetworkReply::NetworkError, const QString)),
                this, SLOT(finishedWithErrorSlot(QNetworkReply::NetworkError,QString)));
        // some signals are only interesting when normal asynchronous style is used
        connect(httpReply,SIGNAL(readyRead()), this, SLOT(readyReadSlot()));
        connect(httpReply,SIGNAL(dataReadProgress(int, int)), this, SLOT(dataReadProgressSlot(int,int)));
#ifndef QT_NO_OPENSSL
        connect(httpReply,SIGNAL(sslErrors(const QList<QSslError>)), this, SLOT(sslErrorsSlot(QList<QSslError>)));
#endif

        // In the asynchronous HTTP case we can just forward those signals
        // Connect the reply signals that we can directly forward
        connect(httpReply, SIGNAL(authenticationRequired(QHttpNetworkRequest,QAuthenticator*)),
                this, SIGNAL(authenticationRequired(QHttpNetworkRequest,QAuthenticator*)));
        connect(httpReply, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
                this, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
    }

    connect(httpReply, SIGNAL(cacheCredentials(QHttpNetworkRequest,QAuthenticator*)),
            this, SLOT(cacheCredentialsSlot(QHttpNetworkRequest,QAuthenticator*)));
}

// This gets called from the user thread or by the synchronous HTTP timeout timer
void QHttpThreadDelegate::abortRequest()
{
#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::abortRequest() thread=" << QThread::currentThreadId() << "sync=" << synchronous;
#endif
    if (httpReply) {
        delete httpReply;
        httpReply = 0;
    }

    // Got aborted by the timeout timer
    if (synchronous) {
        incomingErrorCode = QNetworkReply::TimeoutError;
        QMetaObject::invokeMethod(synchronousRequestLoop, "quit", Qt::QueuedConnection);
    } else {
        //only delete this for asynchronous mode or QNetworkAccessHttpBackend will crash - see QNetworkAccessHttpBackend::postRequest()
        this->deleteLater();
    }
}

void QHttpThreadDelegate::readyReadSlot()
{
    // Don't do in zerocopy case
    if (!downloadBuffer.isNull())
        return;

    while (httpReply->readAnyAvailable()) {
        pendingDownloadData->fetchAndAddRelease(1);
        emit downloadData(httpReply->readAny());
    }
}

void QHttpThreadDelegate::finishedSlot()
{
    if (!httpReply) {
        qWarning("QHttpThreadDelegate::finishedSlot: HTTP reply had already been deleted, internal problem. Please report.");
        return;
    }
#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::finishedSlot() thread=" << QThread::currentThreadId() << "result=" << httpReply->statusCode();
#endif

    // If there is still some data left emit that now
    while (httpReply->readAnyAvailable()) {
        pendingDownloadData->fetchAndAddRelease(1);
        emit downloadData(httpReply->readAny());
    }

#ifndef QT_NO_OPENSSL
    if (ssl)
        emit sslConfigurationChanged(httpReply->sslConfiguration());
#endif

    if (httpReply->statusCode() >= 400) {
            // it's an error reply
            QString msg = QLatin1String(QT_TRANSLATE_NOOP("QNetworkReply",
                                                          "Error downloading %1 - server replied: %2"));
            msg = msg.arg(QString::fromAscii(httpRequest.url().toEncoded()), httpReply->reasonPhrase());
            emit error(statusCodeFromHttp(httpReply->statusCode(), httpRequest.url()), msg);
        }

    emit downloadFinished();

    QMetaObject::invokeMethod(httpReply, "deleteLater", Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, "deleteLater", Qt::QueuedConnection);
    httpReply = 0;
}

void QHttpThreadDelegate::synchronousFinishedSlot()
{
#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::synchronousFinishedSlot() thread=" << QThread::currentThreadId() << "result=" << httpReply->statusCode();
#endif
    if (httpReply->statusCode() >= 400) {
            // it's an error reply
            QString msg = QLatin1String(QT_TRANSLATE_NOOP("QNetworkReply",
                                                          "Error downloading %1 - server replied: %2"));
            incomingErrorDetail = msg.arg(QString::fromAscii(httpRequest.url().toEncoded()), httpReply->reasonPhrase());
            incomingErrorCode = statusCodeFromHttp(httpReply->statusCode(), httpRequest.url());
    }

    synchronousDownloadData = httpReply->readAll();

    QMetaObject::invokeMethod(httpReply, "deleteLater", Qt::QueuedConnection);
    QMetaObject::invokeMethod(synchronousRequestLoop, "quit", Qt::QueuedConnection);
    httpReply = 0;
}

void QHttpThreadDelegate::finishedWithErrorSlot(QNetworkReply::NetworkError errorCode, const QString &detail)
{
    if (!httpReply) {
        qWarning("QHttpThreadDelegate::finishedWithErrorSlot: HTTP reply had already been deleted, internal problem. Please report.");
        return;
    }
#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::finishedWithErrorSlot() thread=" << QThread::currentThreadId() << "error=" << errorCode << detail;
#endif

#ifndef QT_NO_OPENSSL
    if (ssl)
        emit sslConfigurationChanged(httpReply->sslConfiguration());
#endif
    emit error(errorCode,detail);
    emit downloadFinished();


    QMetaObject::invokeMethod(httpReply, "deleteLater", Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, "deleteLater", Qt::QueuedConnection);
    httpReply = 0;
}


void QHttpThreadDelegate::synchronousFinishedWithErrorSlot(QNetworkReply::NetworkError errorCode, const QString &detail)
{
#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::synchronousFinishedWithErrorSlot() thread=" << QThread::currentThreadId() << "error=" << errorCode << detail;
#endif
    incomingErrorCode = errorCode;
    incomingErrorDetail = detail;

    QMetaObject::invokeMethod(httpReply, "deleteLater", Qt::QueuedConnection);
    QMetaObject::invokeMethod(synchronousRequestLoop, "quit", Qt::QueuedConnection);
    httpReply = 0;
}

static void downloadBufferDeleter(char *ptr)
{
    delete[] ptr;
}

void QHttpThreadDelegate::headerChangedSlot()
{
#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::headerChangedSlot() thread=" << QThread::currentThreadId();
#endif

#ifndef QT_NO_OPENSSL
    if (ssl)
        emit sslConfigurationChanged(httpReply->sslConfiguration());
#endif

    // Is using a zerocopy buffer allowed by user and possible with this reply?
    if (httpReply->supportsUserProvidedDownloadBuffer()
        && downloadBufferMaximumSize > 0) {
        char *buf = new char[httpReply->contentLength()]; // throws if allocation fails
        if (buf) {
            downloadBuffer = QSharedPointer<char>(buf, downloadBufferDeleter);
            httpReply->setUserProvidedDownloadBuffer(buf);
        }
    }

    // We fetch this into our own
    incomingHeaders = httpReply->header();
    incomingStatusCode = httpReply->statusCode();
    incomingReasonPhrase = httpReply->reasonPhrase();
    isPipeliningUsed = httpReply->isPipeliningUsed();
    incomingContentLength = httpReply->contentLength();

    emit downloadMetaData(incomingHeaders,
                          incomingStatusCode,
                          incomingReasonPhrase,
                          isPipeliningUsed,
                          downloadBuffer,
                          incomingContentLength);
}

void QHttpThreadDelegate::synchronousHeaderChangedSlot()
{
#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::synchronousHeaderChangedSlot() thread=" << QThread::currentThreadId();
#endif
    // Store the information we need in this object, the QNetworkAccessHttpBackend will later read it
    incomingHeaders = httpReply->header();
    incomingStatusCode = httpReply->statusCode();
    incomingReasonPhrase = httpReply->reasonPhrase();
    isPipeliningUsed = httpReply->isPipeliningUsed();
    incomingContentLength = httpReply->contentLength();
}


void QHttpThreadDelegate::dataReadProgressSlot(int done, int total)
{
    // If we don't have a download buffer don't attempt to go this codepath
    // It is not used by QNetworkAccessHttpBackend
    if (downloadBuffer.isNull())
        return;

    pendingDownloadProgress->fetchAndAddRelease(1);
    emit downloadProgress(done, total);
}

void QHttpThreadDelegate::cacheCredentialsSlot(const QHttpNetworkRequest &request, QAuthenticator *authenticator)
{
    authenticationManager->cacheCredentials(request.url(), authenticator);
}


#ifndef QT_NO_OPENSSL
void QHttpThreadDelegate::sslErrorsSlot(const QList<QSslError> &errors)
{
    emit sslConfigurationChanged(httpReply->sslConfiguration());

    bool ignoreAll = false;
    QList<QSslError> specificErrors;
    emit sslErrors(errors, &ignoreAll, &specificErrors);
    if (ignoreAll)
        httpReply->ignoreSslErrors();
    if (!specificErrors.isEmpty())
        httpReply->ignoreSslErrors(specificErrors);
}
#endif

void QHttpThreadDelegate::synchronousAuthenticationRequiredSlot(const QHttpNetworkRequest &request, QAuthenticator *a)
{
    Q_UNUSED(request);
#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::synchronousAuthenticationRequiredSlot() thread=" << QThread::currentThreadId();
#endif

    // Ask the credential cache
    QNetworkAuthenticationCredential credential = authenticationManager->fetchCachedCredentials(httpRequest.url(), a);
    if (!credential.isNull()) {
        a->setUser(credential.user);
        a->setPassword(credential.password);
    }

    // Disconnect this connection now since we only want to ask the authentication cache once.
    QObject::disconnect(httpReply, SIGNAL(authenticationRequired(QHttpNetworkRequest,QAuthenticator*)),
        this, SLOT(synchronousAuthenticationRequiredSlot(QHttpNetworkRequest,QAuthenticator*)));
}

#ifndef QT_NO_NETWORKPROXY
void  QHttpThreadDelegate::synchronousProxyAuthenticationRequiredSlot(const QNetworkProxy &p, QAuthenticator *a)
{
#ifdef QHTTPTHREADDELEGATE_DEBUG
    qDebug() << "QHttpThreadDelegate::synchronousProxyAuthenticationRequiredSlot() thread=" << QThread::currentThreadId();
#endif
    // Ask the credential cache
    QNetworkAuthenticationCredential credential = authenticationManager->fetchCachedProxyCredentials(p, a);
    if (!credential.isNull()) {
        a->setUser(credential.user);
        a->setPassword(credential.password);
    }

    // Disconnect this connection now since we only want to ask the authentication cache once.
    QObject::disconnect(httpReply, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
        this, SLOT(synchronousProxyAuthenticationRequiredSlot(QNetworkProxy,QAuthenticator*)));
}

#endif

#endif // QT_NO_HTTP

QT_END_NAMESPACE
