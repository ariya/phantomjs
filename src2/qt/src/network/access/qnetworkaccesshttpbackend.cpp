/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

//#define QNETWORKACCESSHTTPBACKEND_DEBUG

#include "qnetworkaccesshttpbackend_p.h"
#include "qnetworkaccessmanager_p.h"
#include "qnetworkaccesscache_p.h"
#include "qabstractnetworkcache.h"
#include "qnetworkrequest.h"
#include "qnetworkreply.h"
#include "QtNetwork/private/qnetworksession_p.h"
#include "qnetworkrequest_p.h"
#include "qnetworkcookie_p.h"
#include "QtCore/qdatetime.h"
#include "QtCore/qelapsedtimer.h"
#include "QtNetwork/qsslconfiguration.h"
#include "qhttpthreaddelegate_p.h"
#include "qthread.h"

#ifndef QT_NO_HTTP

#include <string.h>             // for strchr

Q_DECLARE_METATYPE(QSharedPointer<char>)

QT_BEGIN_NAMESPACE

class QNetworkProxy;

static inline bool isSeparator(register char c)
{
    static const char separators[] = "()<>@,;:\\\"/[]?={}";
    return isLWS(c) || strchr(separators, c) != 0;
}

// ### merge with nextField in cookiejar.cpp
static QHash<QByteArray, QByteArray> parseHttpOptionHeader(const QByteArray &header)
{
    // The HTTP header is of the form:
    // header          = #1(directives)
    // directives      = token | value-directive
    // value-directive = token "=" (token | quoted-string)
    QHash<QByteArray, QByteArray> result;

    int pos = 0;
    while (true) {
        // skip spaces
        pos = nextNonWhitespace(header, pos);
        if (pos == header.length())
            return result;      // end of parsing

        // pos points to a non-whitespace
        int comma = header.indexOf(',', pos);
        int equal = header.indexOf('=', pos);
        if (comma == pos || equal == pos)
            // huh? Broken header.
            return result;

        // The key name is delimited by either a comma, an equal sign or the end
        // of the header, whichever comes first
        int end = comma;
        if (end == -1)
            end = header.length();
        if (equal != -1 && end > equal)
            end = equal;        // equal sign comes before comma/end
        QByteArray key = QByteArray(header.constData() + pos, end - pos).trimmed().toLower();
        pos = end + 1;

        if (uint(equal) < uint(comma)) {
            // case: token "=" (token | quoted-string)
            // skip spaces
            pos = nextNonWhitespace(header, pos);
            if (pos == header.length())
                // huh? Broken header
                return result;

            QByteArray value;
            value.reserve(header.length() - pos);
            if (header.at(pos) == '"') {
                // case: quoted-string
                // quoted-string  = ( <"> *(qdtext | quoted-pair ) <"> )
                // qdtext         = <any TEXT except <">>
                // quoted-pair    = "\" CHAR
                ++pos;
                while (pos < header.length()) {
                    register char c = header.at(pos);
                    if (c == '"') {
                        // end of quoted text
                        break;
                    } else if (c == '\\') {
                        ++pos;
                        if (pos >= header.length())
                            // broken header
                            return result;
                        c = header.at(pos);
                    }

                    value += c;
                    ++pos;
                }
            } else {
                // case: token
                while (pos < header.length()) {
                    register char c = header.at(pos);
                    if (isSeparator(c))
                        break;
                    value += c;
                    ++pos;
                }
            }

            result.insert(key, value);

            // find the comma now:
            comma = header.indexOf(',', pos);
            if (comma == -1)
                return result;  // end of parsing
            pos = comma + 1;
        } else {
            // case: token
            // key is already set
            result.insert(key, QByteArray());
        }
    }
}

QNetworkAccessBackend *
QNetworkAccessHttpBackendFactory::create(QNetworkAccessManager::Operation op,
                                         const QNetworkRequest &request) const
{
    // check the operation
    switch (op) {
    case QNetworkAccessManager::GetOperation:
    case QNetworkAccessManager::PostOperation:
    case QNetworkAccessManager::HeadOperation:
    case QNetworkAccessManager::PutOperation:
    case QNetworkAccessManager::DeleteOperation:
    case QNetworkAccessManager::CustomOperation:
        break;

    default:
        // no, we can't handle this request
        return 0;
    }

    QUrl url = request.url();
    QString scheme = url.scheme().toLower();
    if (scheme == QLatin1String("http") || scheme == QLatin1String("https"))
        return new QNetworkAccessHttpBackend;

    return 0;
}

QNetworkAccessHttpBackend::QNetworkAccessHttpBackend()
    : QNetworkAccessBackend()
    , statusCode(0)
    , pendingDownloadDataEmissions(new QAtomicInt())
    , pendingDownloadProgressEmissions(new QAtomicInt())
    , loadingFromCache(false)
    , usingZerocopyDownloadBuffer(false)
#ifndef QT_NO_OPENSSL
    , pendingSslConfiguration(0), pendingIgnoreAllSslErrors(false)
#endif
    , resumeOffset(0)
{
}

QNetworkAccessHttpBackend::~QNetworkAccessHttpBackend()
{
    // This will do nothing if the request was already finished or aborted
    emit abortHttpRequest();

#ifndef QT_NO_OPENSSL
    delete pendingSslConfiguration;
#endif
}

/*
    For a given httpRequest
    1) If AlwaysNetwork, return
    2) If we have a cache entry for this url populate headers so the server can return 304
    3) Calculate if response_is_fresh and if so send the cache and set loadedFromCache to true
 */
bool QNetworkAccessHttpBackend::loadFromCacheIfAllowed(QHttpNetworkRequest &httpRequest)
{
    QNetworkRequest::CacheLoadControl CacheLoadControlAttribute =
        (QNetworkRequest::CacheLoadControl)request().attribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferNetwork).toInt();
    if (CacheLoadControlAttribute == QNetworkRequest::AlwaysNetwork) {
        // If the request does not already specify preferred cache-control
        // force reload from the network and tell any caching proxy servers to reload too
        if (!request().rawHeaderList().contains("Cache-Control")) {
            httpRequest.setHeaderField("Cache-Control", "no-cache");
            httpRequest.setHeaderField("Pragma", "no-cache");
        }
        return false;
    }

    // The disk cache API does not currently support partial content retrieval.
    // That is why we don't use the disk cache for any such requests.
    if (request().hasRawHeader("Range"))
        return false;

    QAbstractNetworkCache *nc = networkCache();
    if (!nc)
        return false;                 // no local cache

    QNetworkCacheMetaData metaData = nc->metaData(url());
    if (!metaData.isValid())
        return false;                 // not in cache

    if (!metaData.saveToDisk())
        return false;

    QNetworkHeadersPrivate cacheHeaders;
    QNetworkHeadersPrivate::RawHeadersList::ConstIterator it;
    cacheHeaders.setAllRawHeaders(metaData.rawHeaders());

    it = cacheHeaders.findRawHeader("etag");
    if (it != cacheHeaders.rawHeaders.constEnd())
        httpRequest.setHeaderField("If-None-Match", it->second);

    QDateTime lastModified = metaData.lastModified();
    if (lastModified.isValid())
        httpRequest.setHeaderField("If-Modified-Since", QNetworkHeadersPrivate::toHttpDate(lastModified));

    it = cacheHeaders.findRawHeader("Cache-Control");
    if (it != cacheHeaders.rawHeaders.constEnd()) {
        QHash<QByteArray, QByteArray> cacheControl = parseHttpOptionHeader(it->second);
        if (cacheControl.contains("must-revalidate"))
            return false;
    }

    QDateTime currentDateTime = QDateTime::currentDateTime();
    QDateTime expirationDate = metaData.expirationDate();

#if 0
    /*
     * age_value
     *      is the value of Age: header received by the cache with
     *              this response.
     * date_value
     *      is the value of the origin server's Date: header
     * request_time
     *      is the (local) time when the cache made the request
     *              that resulted in this cached response
     * response_time
     *      is the (local) time when the cache received the
     *              response
     * now
     *      is the current (local) time
     */
    int age_value = 0;
    it = cacheHeaders.findRawHeader("age");
    if (it != cacheHeaders.rawHeaders.constEnd())
        age_value = it->second.toInt();

    QDateTime dateHeader;
    int date_value = 0;
    it = cacheHeaders.findRawHeader("date");
    if (it != cacheHeaders.rawHeaders.constEnd()) {
        dateHeader = QNetworkHeadersPrivate::fromHttpDate(it->second);
        date_value = dateHeader.toTime_t();
    }

    int now = currentDateTime.toUTC().toTime_t();
    int request_time = now;
    int response_time = now;

    // Algorithm from RFC 2616 section 13.2.3
    int apparent_age = qMax(0, response_time - date_value);
    int corrected_received_age = qMax(apparent_age, age_value);
    int response_delay = response_time - request_time;
    int corrected_initial_age = corrected_received_age + response_delay;
    int resident_time = now - response_time;
    int current_age   = corrected_initial_age + resident_time;

    // RFC 2616 13.2.4 Expiration Calculations
    if (!expirationDate.isValid()) {
        if (lastModified.isValid()) {
            int diff = currentDateTime.secsTo(lastModified);
            expirationDate = lastModified;
            expirationDate.addSecs(diff / 10);
            if (httpRequest.headerField("Warning").isEmpty()) {
                QDateTime dt;
                dt.setTime_t(current_age);
                if (dt.daysTo(currentDateTime) > 1)
                    httpRequest.setHeaderField("Warning", "113");
            }
        }
    }

    // the cache-saving code below sets the expirationDate with date+max_age
    // if "max-age" is present, or to Expires otherwise
    int freshness_lifetime = dateHeader.secsTo(expirationDate);
    bool response_is_fresh = (freshness_lifetime > current_age);
#else
    bool response_is_fresh = currentDateTime.secsTo(expirationDate) >= 0;
#endif

    if (!response_is_fresh)
        return false;

#if defined(QNETWORKACCESSHTTPBACKEND_DEBUG)
    qDebug() << "response_is_fresh" << CacheLoadControlAttribute;
#endif
    return sendCacheContents(metaData);
}

static QHttpNetworkRequest::Priority convert(const QNetworkRequest::Priority& prio)
{
    switch (prio) {
    case QNetworkRequest::LowPriority:
        return QHttpNetworkRequest::LowPriority;
    case QNetworkRequest::HighPriority:
        return QHttpNetworkRequest::HighPriority;
    case QNetworkRequest::NormalPriority:
    default:
        return QHttpNetworkRequest::NormalPriority;
    }
}

void QNetworkAccessHttpBackend::postRequest()
{
    QThread *thread = 0;
    if (isSynchronous()) {
        // A synchronous HTTP request uses its own thread
        thread = new QThread();
        QObject::connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
        thread->start();
    } else if (!manager->httpThread) {
        // We use the manager-global thread.
        // At some point we could switch to having multiple threads if it makes sense.
        manager->httpThread = new QThread();
        QObject::connect(manager->httpThread, SIGNAL(finished()), manager->httpThread, SLOT(deleteLater()));
        manager->httpThread->start();
#ifndef QT_NO_NETWORKPROXY
        qRegisterMetaType<QNetworkProxy>("QNetworkProxy");
#endif
#ifndef QT_NO_OPENSSL
        qRegisterMetaType<QList<QSslError> >("QList<QSslError>");
        qRegisterMetaType<QSslConfiguration>("QSslConfiguration");
#endif
        qRegisterMetaType<QList<QPair<QByteArray,QByteArray> > >("QList<QPair<QByteArray,QByteArray> >");
        qRegisterMetaType<QHttpNetworkRequest>("QHttpNetworkRequest");
        qRegisterMetaType<QNetworkReply::NetworkError>("QNetworkReply::NetworkError");
        qRegisterMetaType<QSharedPointer<char> >("QSharedPointer<char>");

        thread = manager->httpThread;
    } else {
        // Asynchronous request, thread already exists
        thread = manager->httpThread;
    }

    QUrl url = request().url();
    httpRequest.setUrl(url);

    bool ssl = url.scheme().toLower() == QLatin1String("https");
    setAttribute(QNetworkRequest::ConnectionEncryptedAttribute, ssl);
    httpRequest.setSsl(ssl);


#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy transparentProxy, cacheProxy;

    foreach (const QNetworkProxy &p, proxyList()) {
        // use the first proxy that works
        // for non-encrypted connections, any transparent or HTTP proxy
        // for encrypted, only transparent proxies
        if (!ssl
            && (p.capabilities() & QNetworkProxy::CachingCapability)
            && (p.type() == QNetworkProxy::HttpProxy ||
                p.type() == QNetworkProxy::HttpCachingProxy)) {
            cacheProxy = p;
            transparentProxy = QNetworkProxy::NoProxy;
            break;
        }
        if (p.isTransparentProxy()) {
            transparentProxy = p;
            cacheProxy = QNetworkProxy::NoProxy;
            break;
        }
    }

    // check if at least one of the proxies
    if (transparentProxy.type() == QNetworkProxy::DefaultProxy &&
        cacheProxy.type() == QNetworkProxy::DefaultProxy) {
        // unsuitable proxies
        QMetaObject::invokeMethod(this, "error", isSynchronous() ? Qt::DirectConnection : Qt::QueuedConnection,
                                  Q_ARG(QNetworkReply::NetworkError, QNetworkReply::ProxyNotFoundError),
                                  Q_ARG(QString, tr("No suitable proxy found")));
        QMetaObject::invokeMethod(this, "finished", isSynchronous() ? Qt::DirectConnection : Qt::QueuedConnection);
        return;
    }
#endif


    bool loadedFromCache = false;
    httpRequest.setPriority(convert(request().priority()));

    switch (operation()) {
    case QNetworkAccessManager::GetOperation:
        httpRequest.setOperation(QHttpNetworkRequest::Get);
        loadedFromCache = loadFromCacheIfAllowed(httpRequest);
        break;

    case QNetworkAccessManager::HeadOperation:
        httpRequest.setOperation(QHttpNetworkRequest::Head);
        loadedFromCache = loadFromCacheIfAllowed(httpRequest);
        break;

    case QNetworkAccessManager::PostOperation:
        invalidateCache();
        httpRequest.setOperation(QHttpNetworkRequest::Post);
        createUploadByteDevice();
        break;

    case QNetworkAccessManager::PutOperation:
        invalidateCache();
        httpRequest.setOperation(QHttpNetworkRequest::Put);
        createUploadByteDevice();
        break;

    case QNetworkAccessManager::DeleteOperation:
        invalidateCache();
        httpRequest.setOperation(QHttpNetworkRequest::Delete);
        break;

    case QNetworkAccessManager::CustomOperation:
        invalidateCache(); // for safety reasons, we don't know what the operation does
        httpRequest.setOperation(QHttpNetworkRequest::Custom);
        createUploadByteDevice();
        httpRequest.setCustomVerb(request().attribute(
                QNetworkRequest::CustomVerbAttribute).toByteArray());
        break;

    default:
        break;                  // can't happen
    }

    if (loadedFromCache) {
        // commented this out since it will be called later anyway
        // by copyFinished()
        //QNetworkAccessBackend::finished();
        return;    // no need to send the request! :)
    }

    QList<QByteArray> headers = request().rawHeaderList();
    if (resumeOffset != 0) {
        if (headers.contains("Range")) {
            // Need to adjust resume offset for user specified range

            headers.removeOne("Range");

            // We've already verified that requestRange starts with "bytes=", see canResume.
            QByteArray requestRange = request().rawHeader("Range").mid(6);

            int index = requestRange.indexOf('-');

            quint64 requestStartOffset = requestRange.left(index).toULongLong();
            quint64 requestEndOffset = requestRange.mid(index + 1).toULongLong();

            requestRange = "bytes=" + QByteArray::number(resumeOffset + requestStartOffset) +
                           '-' + QByteArray::number(requestEndOffset);

            httpRequest.setHeaderField("Range", requestRange);
        } else {
            httpRequest.setHeaderField("Range", "bytes=" + QByteArray::number(resumeOffset) + '-');
        }
    }

    foreach (const QByteArray &header, headers)
        httpRequest.setHeaderField(header, request().rawHeader(header));

    if (request().attribute(QNetworkRequest::HttpPipeliningAllowedAttribute).toBool() == true)
        httpRequest.setPipeliningAllowed(true);

    if (static_cast<QNetworkRequest::LoadControl>
        (request().attribute(QNetworkRequest::AuthenticationReuseAttribute,
                             QNetworkRequest::Automatic).toInt()) == QNetworkRequest::Manual)
        httpRequest.setWithCredentials(false);


    // Create the HTTP thread delegate
    QHttpThreadDelegate *delegate = new QHttpThreadDelegate;
#ifndef QT_NO_BEARERMANAGEMENT
    QVariant v(property("_q_networksession"));
    if (v.isValid())
        delegate->networkSession = qvariant_cast<QSharedPointer<QNetworkSession> >(v);
#endif

    // For the synchronous HTTP, this is the normal way the delegate gets deleted
    // For the asynchronous HTTP this is a safety measure, the delegate deletes itself when HTTP is finished
    connect(thread, SIGNAL(finished()), delegate, SLOT(deleteLater()));

    // Set the properties it needs
    delegate->httpRequest = httpRequest;
#ifndef QT_NO_NETWORKPROXY
    delegate->cacheProxy = cacheProxy;
    delegate->transparentProxy = transparentProxy;
#endif
    delegate->ssl = ssl;
#ifndef QT_NO_OPENSSL
    if (ssl)
        delegate->incomingSslConfiguration = request().sslConfiguration();
#endif

    // Do we use synchronous HTTP?
    delegate->synchronous = isSynchronous();

    // The authentication manager is used to avoid the BlockingQueuedConnection communication
    // from HTTP thread to user thread in some cases.
    delegate->authenticationManager = manager->authenticationManager;

    if (!isSynchronous()) {
        // Tell our zerocopy policy to the delegate
        delegate->downloadBufferMaximumSize =
                request().attribute(QNetworkRequest::MaximumDownloadBufferSizeAttribute).toLongLong();

        // These atomic integers are used for signal compression
        delegate->pendingDownloadData = pendingDownloadDataEmissions;
        delegate->pendingDownloadProgress = pendingDownloadProgressEmissions;

        // Connect the signals of the delegate to us
        connect(delegate, SIGNAL(downloadData(QByteArray)),
                this, SLOT(replyDownloadData(QByteArray)),
                Qt::QueuedConnection);
        connect(delegate, SIGNAL(downloadFinished()),
                this, SLOT(replyFinished()),
                Qt::QueuedConnection);
        connect(delegate, SIGNAL(downloadMetaData(QList<QPair<QByteArray,QByteArray> >,int,QString,bool,QSharedPointer<char>,qint64)),
                this, SLOT(replyDownloadMetaData(QList<QPair<QByteArray,QByteArray> >,int,QString,bool,QSharedPointer<char>,qint64)),
                Qt::QueuedConnection);
        connect(delegate, SIGNAL(downloadProgress(qint64,qint64)),
                this, SLOT(replyDownloadProgressSlot(qint64,qint64)),
                Qt::QueuedConnection);
        connect(delegate, SIGNAL(error(QNetworkReply::NetworkError,QString)),
                this, SLOT(httpError(QNetworkReply::NetworkError, const QString)),
                Qt::QueuedConnection);
#ifndef QT_NO_OPENSSL
        connect(delegate, SIGNAL(sslConfigurationChanged(QSslConfiguration)),
                this, SLOT(replySslConfigurationChanged(QSslConfiguration)),
                Qt::QueuedConnection);
#endif
        // Those need to report back, therefire BlockingQueuedConnection
        connect(delegate, SIGNAL(authenticationRequired(QHttpNetworkRequest,QAuthenticator*)),
                this, SLOT(httpAuthenticationRequired(QHttpNetworkRequest,QAuthenticator*)),
                Qt::BlockingQueuedConnection);
#ifndef QT_NO_NETWORKPROXY
        connect (delegate, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
                 this, SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
                 Qt::BlockingQueuedConnection);
#endif
#ifndef QT_NO_OPENSSL
        connect(delegate, SIGNAL(sslErrors(QList<QSslError>,bool*,QList<QSslError>*)),
                this, SLOT(replySslErrors(const QList<QSslError> &, bool *, QList<QSslError> *)),
                Qt::BlockingQueuedConnection);
#endif
        // This signal we will use to start the request.
        connect(this, SIGNAL(startHttpRequest()), delegate, SLOT(startRequest()));
        connect(this, SIGNAL(abortHttpRequest()), delegate, SLOT(abortRequest()));

        // To throttle the connection.
        QObject::connect(this, SIGNAL(readBufferSizeChanged(qint64)), delegate, SLOT(readBufferSizeChanged(qint64)));
        QObject::connect(this, SIGNAL(readBufferFreed(qint64)), delegate, SLOT(readBufferFreed(qint64)));

        if (uploadByteDevice) {
            QNonContiguousByteDeviceThreadForwardImpl *forwardUploadDevice =
                    new QNonContiguousByteDeviceThreadForwardImpl(uploadByteDevice->atEnd(), uploadByteDevice->size());
            if (uploadByteDevice->isResetDisabled())
                forwardUploadDevice->disableReset();
            forwardUploadDevice->setParent(delegate); // needed to make sure it is moved on moveToThread()
            delegate->httpRequest.setUploadByteDevice(forwardUploadDevice);

            // From main thread to user thread:
            QObject::connect(this, SIGNAL(haveUploadData(QByteArray, bool, qint64)),
                             forwardUploadDevice, SLOT(haveDataSlot(QByteArray, bool, qint64)), Qt::QueuedConnection);
            QObject::connect(uploadByteDevice.data(), SIGNAL(readyRead()),
                             forwardUploadDevice, SIGNAL(readyRead()),
                             Qt::QueuedConnection);

            // From http thread to user thread:
            QObject::connect(forwardUploadDevice, SIGNAL(wantData(qint64)),
                             this, SLOT(wantUploadDataSlot(qint64)));
            QObject::connect(forwardUploadDevice, SIGNAL(processedData(qint64)),
                             this, SLOT(sentUploadDataSlot(qint64)));
            connect(forwardUploadDevice, SIGNAL(resetData(bool*)),
                    this, SLOT(resetUploadDataSlot(bool*)),
                    Qt::BlockingQueuedConnection); // this is the only one with BlockingQueued!
        }
    } else if (isSynchronous()) {
        connect(this, SIGNAL(startHttpRequestSynchronously()), delegate, SLOT(startRequestSynchronously()), Qt::BlockingQueuedConnection);

        if (uploadByteDevice) {
            // For the synchronous HTTP use case the use thread (this one here) is blocked
            // so we cannot use the asynchronous upload architecture.
            // We therefore won't use the QNonContiguousByteDeviceThreadForwardImpl but directly
            // use the uploadByteDevice provided to us by the QNetworkReplyImpl.
            // The code that is in QNetworkReplyImplPrivate::setup() makes sure it is safe to use from a thread
            // since it only wraps a QRingBuffer
            delegate->httpRequest.setUploadByteDevice(uploadByteDevice.data());
        }
    }


    // Move the delegate to the http thread
    delegate->moveToThread(thread);
    // This call automatically moves the uploadDevice too for the asynchronous case.

    // Send an signal to the delegate so it starts working in the other thread
    if (isSynchronous()) {
        emit startHttpRequestSynchronously(); // This one is BlockingQueuedConnection, so it will return when all work is done

        if (delegate->incomingErrorCode != QNetworkReply::NoError) {
            replyDownloadMetaData
                    (delegate->incomingHeaders,
                     delegate->incomingStatusCode,
                     delegate->incomingReasonPhrase,
                     delegate->isPipeliningUsed,
                     QSharedPointer<char>(),
                     delegate->incomingContentLength);
            replyDownloadData(delegate->synchronousDownloadData);
            httpError(delegate->incomingErrorCode, delegate->incomingErrorDetail);
        } else {
            replyDownloadMetaData
                    (delegate->incomingHeaders,
                     delegate->incomingStatusCode,
                     delegate->incomingReasonPhrase,
                     delegate->isPipeliningUsed,
                     QSharedPointer<char>(),
                     delegate->incomingContentLength);
            replyDownloadData(delegate->synchronousDownloadData);
        }

        // End the thread. It will delete itself from the finished() signal
        thread->quit();
        thread->wait(5000);

        finished();
    } else {
        emit startHttpRequest(); // Signal to the HTTP thread and go back to user.
    }
}

void QNetworkAccessHttpBackend::invalidateCache()
{
    QAbstractNetworkCache *nc = networkCache();
    if (nc)
        nc->remove(url());
}

void QNetworkAccessHttpBackend::open()
{
    postRequest();
}

void QNetworkAccessHttpBackend::closeDownstreamChannel()
{
    // FIXME Maybe we can get rid of this whole architecture part
}

void QNetworkAccessHttpBackend::downstreamReadyWrite()
{
    // FIXME Maybe we can get rid of this whole architecture part
}

void QNetworkAccessHttpBackend::setDownstreamLimited(bool b)
{
    Q_UNUSED(b);
    // We know that readBuffer maximum size limiting is broken since quite a while.
    // The task to fix this is QTBUG-15065
}

void QNetworkAccessHttpBackend::setReadBufferSize(qint64 size)
{
    emit readBufferSizeChanged(size);
}

void QNetworkAccessHttpBackend::emitReadBufferFreed(qint64 size)
{
    emit readBufferFreed(size);
}

void QNetworkAccessHttpBackend::replyDownloadData(QByteArray d)
{
    int pendingSignals = (int)pendingDownloadDataEmissions->fetchAndAddAcquire(-1) - 1;

    if (pendingSignals > 0) {
        // Some more signal emissions to this slot are pending.
        // Instead of writing the downstream data, we wait
        // and do it in the next call we get
        // (signal comppression)
        pendingDownloadData.append(d);
        return;
    }

    pendingDownloadData.append(d);
    d.clear();
    // We need to usa a copy for calling writeDownstreamData as we could
    // possibly recurse into this this function when we call
    // appendDownstreamDataSignalEmissions because the user might call
    // processEvents() or spin an event loop when this occur.
    QByteDataBuffer pendingDownloadDataCopy = pendingDownloadData;
    pendingDownloadData.clear();
    writeDownstreamData(pendingDownloadDataCopy);
}

void QNetworkAccessHttpBackend::replyFinished()
{
    // We are already loading from cache, we still however
    // got this signal because it was posted already
    if (loadingFromCache)
        return;

    finished();
}

void QNetworkAccessHttpBackend::checkForRedirect(const int statusCode)
{
    switch (statusCode) {
    case 301:                   // Moved Permanently
    case 302:                   // Found
    case 303:                   // See Other
    case 307:                   // Temporary Redirect
        // What do we do about the caching of the HTML note?
        // The response to a 303 MUST NOT be cached, while the response to
        // all of the others is cacheable if the headers indicate it to be
        QByteArray header = rawHeader("location");
        QUrl url = QUrl::fromEncoded(header);
        if (!url.isValid())
            url = QUrl(QLatin1String(header));
        redirectionRequested(url);
    }
}

void QNetworkAccessHttpBackend::replyDownloadMetaData
        (QList<QPair<QByteArray,QByteArray> > hm,
         int sc,QString rp,bool pu,
         QSharedPointer<char> db,
         qint64 contentLength)
{
    statusCode = sc;
    reasonPhrase = rp;

    // Download buffer
    if (!db.isNull()) {
        reply->setDownloadBuffer(db, contentLength);
        usingZerocopyDownloadBuffer = true;
    } else {
        usingZerocopyDownloadBuffer = false;
    }

    setAttribute(QNetworkRequest::HttpPipeliningWasUsedAttribute, pu);

    // reconstruct the HTTP header
    QList<QPair<QByteArray, QByteArray> > headerMap = hm;
    QList<QPair<QByteArray, QByteArray> >::ConstIterator it = headerMap.constBegin(),
                                                        end = headerMap.constEnd();
    QByteArray header;

    for (; it != end; ++it) {
        QByteArray value = rawHeader(it->first);
        if (!value.isEmpty()) {
            if (qstricmp(it->first.constData(), "set-cookie") == 0)
                value += '\n';
            else
                value += ", ";
        }
        value += it->second;
        setRawHeader(it->first, value);
    }

    setAttribute(QNetworkRequest::HttpStatusCodeAttribute, statusCode);
    setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, reasonPhrase);

    // is it a redirection?
    checkForRedirect(statusCode);

    if (statusCode >= 500 && statusCode < 600) {
        QAbstractNetworkCache *nc = networkCache();
        if (nc) {
            QNetworkCacheMetaData metaData = nc->metaData(url());
            QNetworkHeadersPrivate cacheHeaders;
            cacheHeaders.setAllRawHeaders(metaData.rawHeaders());
            QNetworkHeadersPrivate::RawHeadersList::ConstIterator it;
            it = cacheHeaders.findRawHeader("Cache-Control");
            bool mustReValidate = false;
            if (it != cacheHeaders.rawHeaders.constEnd()) {
                QHash<QByteArray, QByteArray> cacheControl = parseHttpOptionHeader(it->second);
                if (cacheControl.contains("must-revalidate"))
                    mustReValidate = true;
            }
            if (!mustReValidate && sendCacheContents(metaData))
                return;
        }
    }

    if (statusCode == 304) {
#if defined(QNETWORKACCESSHTTPBACKEND_DEBUG)
        qDebug() << "Received a 304 from" << url();
#endif
        QAbstractNetworkCache *nc = networkCache();
        if (nc) {
            QNetworkCacheMetaData oldMetaData = nc->metaData(url());
            QNetworkCacheMetaData metaData = fetchCacheMetaData(oldMetaData);
            if (oldMetaData != metaData)
                nc->updateMetaData(metaData);
            if (sendCacheContents(metaData))
                return;
        }
    }


    if (statusCode != 304 && statusCode != 303) {
        if (!isCachingEnabled())
            setCachingEnabled(true);
    }

    metaDataChanged();
}

void QNetworkAccessHttpBackend::replyDownloadProgressSlot(qint64 received,  qint64 total)
{
    // we can be sure here that there is a download buffer

    int pendingSignals = (int)pendingDownloadProgressEmissions->fetchAndAddAcquire(-1) - 1;
    if (pendingSignals > 0) {
        // Let's ignore this signal and look at the next one coming in
        // (signal comppression)
        return;
    }

    // Now do the actual notification of new bytes
    writeDownstreamDataDownloadBuffer(received, total);
}

void QNetworkAccessHttpBackend::httpAuthenticationRequired(const QHttpNetworkRequest &,
                                                           QAuthenticator *auth)
{
    authenticationRequired(auth);
}

void QNetworkAccessHttpBackend::httpError(QNetworkReply::NetworkError errorCode,
                                          const QString &errorString)
{
#if defined(QNETWORKACCESSHTTPBACKEND_DEBUG)
    qDebug() << "http error!" << errorCode << errorString;
#endif

    error(errorCode, errorString);
}

#ifndef QT_NO_OPENSSL
void QNetworkAccessHttpBackend::replySslErrors(
        const QList<QSslError> &list, bool *ignoreAll, QList<QSslError> *toBeIgnored)
{
    // Go to generic backend
    sslErrors(list);
    // Check if the callback set any ignore and return this here to http thread
    if (pendingIgnoreAllSslErrors)
        *ignoreAll = true;
    if (!pendingIgnoreSslErrorsList.isEmpty())
        *toBeIgnored = pendingIgnoreSslErrorsList;
}

void QNetworkAccessHttpBackend::replySslConfigurationChanged(const QSslConfiguration &c)
{
    // Receiving the used SSL configuration from the HTTP thread
    if (pendingSslConfiguration)
        *pendingSslConfiguration = c;
    else if (!c.isNull())
        pendingSslConfiguration = new QSslConfiguration(c);
}
#endif

// Coming from QNonContiguousByteDeviceThreadForwardImpl in HTTP thread
void QNetworkAccessHttpBackend::resetUploadDataSlot(bool *r)
{
    *r = uploadByteDevice->reset();
}

// Coming from QNonContiguousByteDeviceThreadForwardImpl in HTTP thread
void QNetworkAccessHttpBackend::sentUploadDataSlot(qint64 amount)
{
    uploadByteDevice->advanceReadPointer(amount);
}

// Coming from QNonContiguousByteDeviceThreadForwardImpl in HTTP thread
void QNetworkAccessHttpBackend::wantUploadDataSlot(qint64 maxSize)
{
    // call readPointer
    qint64 currentUploadDataLength = 0;
    char *data = const_cast<char*>(uploadByteDevice->readPointer(maxSize, currentUploadDataLength));
    // Let's make a copy of this data
    QByteArray dataArray(data, currentUploadDataLength);

    // Communicate back to HTTP thread
    emit haveUploadData(dataArray, uploadByteDevice->atEnd(), uploadByteDevice->size());
}

/*
    A simple web page that can be used to test us: http://www.procata.com/cachetest/
 */
bool QNetworkAccessHttpBackend::sendCacheContents(const QNetworkCacheMetaData &metaData)
{
    setCachingEnabled(false);
    if (!metaData.isValid())
        return false;

    QAbstractNetworkCache *nc = networkCache();
    Q_ASSERT(nc);
    QIODevice *contents = nc->data(url());
    if (!contents) {
#if defined(QNETWORKACCESSHTTPBACKEND_DEBUG)
        qDebug() << "Can not send cache, the contents are 0" << url();
#endif
        return false;
    }
    contents->setParent(this);

    QNetworkCacheMetaData::AttributesMap attributes = metaData.attributes();
    int status = attributes.value(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status < 100)
        status = 200;           // fake it

    setAttribute(QNetworkRequest::HttpStatusCodeAttribute, status);
    setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, attributes.value(QNetworkRequest::HttpReasonPhraseAttribute));
    setAttribute(QNetworkRequest::SourceIsFromCacheAttribute, true);

    QNetworkCacheMetaData::RawHeaderList rawHeaders = metaData.rawHeaders();
    QNetworkCacheMetaData::RawHeaderList::ConstIterator it = rawHeaders.constBegin(),
                                                       end = rawHeaders.constEnd();
    for ( ; it != end; ++it)
        setRawHeader(it->first, it->second);

    checkForRedirect(status);

    // This needs to be emitted in the event loop because it can be reached at
    // the direct code path of qnam.get(...) before the user has a chance
    // to connect any signals.
    QMetaObject::invokeMethod(this, "metaDataChanged", isSynchronous() ? Qt::DirectConnection : Qt::QueuedConnection);
    qRegisterMetaType<QIODevice*>("QIODevice*");
    QMetaObject::invokeMethod(this, "writeDownstreamData", isSynchronous() ? Qt::DirectConnection : Qt::QueuedConnection, Q_ARG(QIODevice*, contents));


#if defined(QNETWORKACCESSHTTPBACKEND_DEBUG)
    qDebug() << "Successfully sent cache:" << url() << contents->size() << "bytes";
#endif

    // Set the following flag so we can ignore some signals from HTTP thread
    // that would still come
    loadingFromCache = true;
    return true;
}

void QNetworkAccessHttpBackend::copyFinished(QIODevice *dev)
{
    delete dev;
    finished();
}

#ifndef QT_NO_OPENSSL
void QNetworkAccessHttpBackend::ignoreSslErrors()
{
    pendingIgnoreAllSslErrors = true;
}

void QNetworkAccessHttpBackend::ignoreSslErrors(const QList<QSslError> &errors)
{
    // the pending list is set if QNetworkReply::ignoreSslErrors(const QList<QSslError> &errors)
    // is called before QNetworkAccessManager::get() (or post(), etc.)
    pendingIgnoreSslErrorsList = errors;
}

void QNetworkAccessHttpBackend::fetchSslConfiguration(QSslConfiguration &config) const
{
    if (pendingSslConfiguration)
        config = *pendingSslConfiguration;
    else
        config = request().sslConfiguration();
}

void QNetworkAccessHttpBackend::setSslConfiguration(const QSslConfiguration &newconfig)
{
    // Setting a SSL configuration on a reply is not supported. The user needs to set
    // her/his QSslConfiguration on the QNetworkRequest.
    Q_UNUSED(newconfig);
}
#endif

QNetworkCacheMetaData QNetworkAccessHttpBackend::fetchCacheMetaData(const QNetworkCacheMetaData &oldMetaData) const
{
    QNetworkCacheMetaData metaData = oldMetaData;

    QNetworkHeadersPrivate cacheHeaders;
    cacheHeaders.setAllRawHeaders(metaData.rawHeaders());
    QNetworkHeadersPrivate::RawHeadersList::ConstIterator it;

    QList<QByteArray> newHeaders = rawHeaderList();
    foreach (QByteArray header, newHeaders) {
        QByteArray originalHeader = header;
        header = header.toLower();
        bool hop_by_hop =
            (header == "connection"
             || header == "keep-alive"
             || header == "proxy-authenticate"
             || header == "proxy-authorization"
             || header == "te"
             || header == "trailers"
             || header == "transfer-encoding"
             || header ==  "upgrade");
        if (hop_by_hop)
            continue;

        // we are currently not using the date header to determine the expiration time of a page,
        // but only the "Expires", "max-age" and "s-maxage" headers, see
        // QNetworkAccessHttpBackend::validateCache() and below ("metaData.setExpirationDate()").
        if (header == "date")
            continue;

        // Don't store Warning 1xx headers
        if (header == "warning") {
            QByteArray v = rawHeader(header);
            if (v.length() == 3
                && v[0] == '1'
                && v[1] >= '0' && v[1] <= '9'
                && v[2] >= '0' && v[2] <= '9')
                continue;
        }

        it = cacheHeaders.findRawHeader(header);
        if (it != cacheHeaders.rawHeaders.constEnd()) {
            // Match the behavior of Firefox and assume Cache-Control: "no-transform"
            if (header == "content-encoding"
                || header == "content-range"
                || header == "content-type")
                continue;

            // For MS servers that send "Content-Length: 0" on 304 responses
            // ignore this too
            if (header == "content-length")
                continue;
        }

#if defined(QNETWORKACCESSHTTPBACKEND_DEBUG)
        QByteArray n = rawHeader(header);
        QByteArray o;
        if (it != cacheHeaders.rawHeaders.constEnd())
            o = (*it).second;
        if (n != o && header != "date") {
            qDebug() << "replacing" << header;
            qDebug() << "new" << n;
            qDebug() << "old" << o;
        }
#endif
        cacheHeaders.setRawHeader(originalHeader, rawHeader(header));
    }
    metaData.setRawHeaders(cacheHeaders.rawHeaders);

    bool checkExpired = true;

    QHash<QByteArray, QByteArray> cacheControl;
    it = cacheHeaders.findRawHeader("Cache-Control");
    if (it != cacheHeaders.rawHeaders.constEnd()) {
        cacheControl = parseHttpOptionHeader(it->second);
        QByteArray maxAge = cacheControl.value("max-age");
        if (!maxAge.isEmpty()) {
            checkExpired = false;
            QDateTime dt = QDateTime::currentDateTime();
            dt = dt.addSecs(maxAge.toInt());
            metaData.setExpirationDate(dt);
        }
    }
    if (checkExpired) {
        it = cacheHeaders.findRawHeader("expires");
        if (it != cacheHeaders.rawHeaders.constEnd()) {
            QDateTime expiredDateTime = QNetworkHeadersPrivate::fromHttpDate(it->second);
            metaData.setExpirationDate(expiredDateTime);
        }
    }

    it = cacheHeaders.findRawHeader("last-modified");
    if (it != cacheHeaders.rawHeaders.constEnd())
        metaData.setLastModified(QNetworkHeadersPrivate::fromHttpDate(it->second));

    bool canDiskCache;
    // only cache GET replies by default, all other replies (POST, PUT, DELETE)
    //  are not cacheable by default (according to RFC 2616 section 9)
    if (httpRequest.operation() == QHttpNetworkRequest::Get) {

        canDiskCache = true;
        // 14.32
        // HTTP/1.1 caches SHOULD treat "Pragma: no-cache" as if the client
        // had sent "Cache-Control: no-cache".
        it = cacheHeaders.findRawHeader("pragma");
        if (it != cacheHeaders.rawHeaders.constEnd()
            && it->second == "no-cache")
            canDiskCache = false;

        // HTTP/1.1. Check the Cache-Control header
        if (cacheControl.contains("no-cache"))
            canDiskCache = false;
        else if (cacheControl.contains("no-store"))
            canDiskCache = false;

    // responses to POST might be cacheable
    } else if (httpRequest.operation() == QHttpNetworkRequest::Post) {

        canDiskCache = false;
        // some pages contain "expires:" and "cache-control: no-cache" field,
        // so we only might cache POST requests if we get "cache-control: max-age ..."
        if (cacheControl.contains("max-age"))
            canDiskCache = true;

    // responses to PUT and DELETE are not cacheable
    } else {
        canDiskCache = false;
    }

    metaData.setSaveToDisk(canDiskCache);
    QNetworkCacheMetaData::AttributesMap attributes;
    if (statusCode != 304) {
        // update the status code
        attributes.insert(QNetworkRequest::HttpStatusCodeAttribute, statusCode);
        attributes.insert(QNetworkRequest::HttpReasonPhraseAttribute, reasonPhrase);
    } else {
        // this is a redirection, keep the attributes intact
        attributes = oldMetaData.attributes();
    }
    metaData.setAttributes(attributes);
    return metaData;
}

bool QNetworkAccessHttpBackend::canResume() const
{
    // Only GET operation supports resuming.
    if (operation() != QNetworkAccessManager::GetOperation)
        return false;

    // Can only resume if server/resource supports Range header.
    QByteArray acceptRangesheaderName("Accept-Ranges");
    if (!hasRawHeader(acceptRangesheaderName) || rawHeader(acceptRangesheaderName) == "none")
        return false;

    // We only support resuming for byte ranges.
    if (request().hasRawHeader("Range")) {
        QByteArray range = request().rawHeader("Range");
        if (!range.startsWith("bytes="))
            return false;
    }

    // If we're using a download buffer then we don't support resuming/migration
    // right now. Too much trouble.
    if (usingZerocopyDownloadBuffer)
        return false;

    return true;
}

void QNetworkAccessHttpBackend::setResumeOffset(quint64 offset)
{
    resumeOffset = offset;
}

QT_END_NAMESPACE

#endif // QT_NO_HTTP
