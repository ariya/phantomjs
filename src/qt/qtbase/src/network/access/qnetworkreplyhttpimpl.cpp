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

//#define QNETWORKACCESSHTTPBACKEND_DEBUG

#include "qnetworkreplyhttpimpl_p.h"
#include "qnetworkaccessmanager_p.h"
#include "qnetworkaccesscache_p.h"
#include "qabstractnetworkcache.h"
#include "qnetworkrequest.h"
#include "qnetworkreply.h"
#include "qnetworkrequest_p.h"
#include "qnetworkcookie.h"
#include "qnetworkcookie_p.h"
#include "QtCore/qdatetime.h"
#include "QtCore/qelapsedtimer.h"
#include "QtNetwork/qsslconfiguration.h"
#include "qhttpthreaddelegate_p.h"
#include "qthread.h"
#include "QtCore/qcoreapplication.h"

#include "qnetworkcookiejar.h"

#ifndef QT_NO_HTTP

#include <string.h>             // for strchr

QT_BEGIN_NAMESPACE

class QNetworkProxy;

static inline bool isSeparator(char c)
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
                    char c = header.at(pos);
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
                    char c = header.at(pos);
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

QNetworkReplyHttpImpl::QNetworkReplyHttpImpl(QNetworkAccessManager* const manager,
                                             const QNetworkRequest& request,
                                             QNetworkAccessManager::Operation& operation,
                                             QIODevice* outgoingData)
    : QNetworkReply(*new QNetworkReplyHttpImplPrivate, manager)
{
    Q_D(QNetworkReplyHttpImpl);
    d->manager = manager;
    d->managerPrivate = manager->d_func();
    d->request = request;
    d->operation = operation;
    d->outgoingData = outgoingData;
    d->url = request.url();
#ifndef QT_NO_SSL
    d->sslConfiguration = request.sslConfiguration();
#endif

    // FIXME Later maybe set to Unbuffered, especially if it is zerocopy or from cache?
    QIODevice::open(QIODevice::ReadOnly);


    // Internal code that does a HTTP reply for the synchronous Ajax
    // in Qt WebKit.
    QVariant synchronousHttpAttribute = request.attribute(
            static_cast<QNetworkRequest::Attribute>(QNetworkRequest::SynchronousRequestAttribute));
    if (synchronousHttpAttribute.isValid()) {
        d->synchronous = synchronousHttpAttribute.toBool();
        if (d->synchronous && outgoingData) {
            // The synchronous HTTP is a corner case, we will put all upload data in one big QByteArray in the outgoingDataBuffer.
            // Yes, this is not the most efficient thing to do, but on the other hand synchronous XHR needs to die anyway.
            d->outgoingDataBuffer = QSharedPointer<QRingBuffer>(new QRingBuffer());
            qint64 previousDataSize = 0;
            do {
                previousDataSize = d->outgoingDataBuffer->size();
                d->outgoingDataBuffer->append(d->outgoingData->readAll());
            } while (d->outgoingDataBuffer->size() != previousDataSize);
            d->_q_startOperation();
            return;
        }
    }


    if (outgoingData) {
        // there is data to be uploaded, e.g. HTTP POST.

        if (!d->outgoingData->isSequential()) {
            // fixed size non-sequential (random-access)
            // just start the operation
            QMetaObject::invokeMethod(this, "_q_startOperation", Qt::QueuedConnection);
            // FIXME make direct call?
        } else {
            bool bufferingDisallowed =
                    request.attribute(QNetworkRequest::DoNotBufferUploadDataAttribute,
                                  false).toBool();

            if (bufferingDisallowed) {
                // if a valid content-length header for the request was supplied, we can disable buffering
                // if not, we will buffer anyway
                if (request.header(QNetworkRequest::ContentLengthHeader).isValid()) {
                    QMetaObject::invokeMethod(this, "_q_startOperation", Qt::QueuedConnection);
                    // FIXME make direct call?
                } else {
                    d->state = d->Buffering;
                    QMetaObject::invokeMethod(this, "_q_bufferOutgoingData", Qt::QueuedConnection);
                }
            } else {
                // _q_startOperation will be called when the buffering has finished.
                d->state = d->Buffering;
                QMetaObject::invokeMethod(this, "_q_bufferOutgoingData", Qt::QueuedConnection);
            }
        }
    } else {
        // No outgoing data (POST, ..)
        d->_q_startOperation();
    }
}

QNetworkReplyHttpImpl::~QNetworkReplyHttpImpl()
{
    // Most work is done in private destructor
}

void QNetworkReplyHttpImpl::close()
{
    Q_D(QNetworkReplyHttpImpl);

    if (d->state == QNetworkReplyHttpImplPrivate::Aborted ||
        d->state == QNetworkReplyHttpImplPrivate::Finished)
        return;

    // According to the documentation close only stops the download
    // by closing we can ignore the download part and continue uploading.
    QNetworkReply::close();

    // call finished which will emit signals
    // FIXME shouldn't this be emitted Queued?
    d->error(OperationCanceledError, tr("Operation canceled"));
    d->finished();
}

void QNetworkReplyHttpImpl::abort()
{
    Q_D(QNetworkReplyHttpImpl);
    // FIXME
    if (d->state == QNetworkReplyHttpImplPrivate::Finished || d->state == QNetworkReplyHttpImplPrivate::Aborted)
        return;

    QNetworkReply::close();

    if (d->state != QNetworkReplyHttpImplPrivate::Finished) {
        // call finished which will emit signals
        // FIXME shouldn't this be emitted Queued?
        d->error(OperationCanceledError, tr("Operation canceled"));
        d->finished();
    }

    d->state = QNetworkReplyHttpImplPrivate::Aborted;

    emit abortHttpRequest();
}

qint64 QNetworkReplyHttpImpl::bytesAvailable() const
{
    Q_D(const QNetworkReplyHttpImpl);

    // if we load from cache device
    if (d->cacheLoadDevice) {
        return QNetworkReply::bytesAvailable() + d->cacheLoadDevice->bytesAvailable() + d->downloadMultiBuffer.byteAmount();
    }

    // zerocopy buffer
    if (d->downloadZerocopyBuffer) {
        return QNetworkReply::bytesAvailable() + d->downloadBufferCurrentSize - d->downloadBufferReadPosition;
    }

    // normal buffer
    return QNetworkReply::bytesAvailable() + d->downloadMultiBuffer.byteAmount();
}

bool QNetworkReplyHttpImpl::isSequential () const
{
    // FIXME In the cache of a cached load or the zero-copy buffer we could actually be non-sequential.
    // FIXME however this requires us to implement stuff like seek() too.
    return true;
}

qint64 QNetworkReplyHttpImpl::size() const
{
    // FIXME At some point, this could return a proper value, e.g. if we're non-sequential.
    return QNetworkReply::size();
}

qint64 QNetworkReplyHttpImpl::readData(char* data, qint64 maxlen)
{
    Q_D(QNetworkReplyHttpImpl);

    // cacheload device
    if (d->cacheLoadDevice) {
        // FIXME bytesdownloaded, position etc?

        // There is something already in the buffer we buffered before because the user did not read()
        // anything, so we read there first:
        if (!d->downloadMultiBuffer.isEmpty()) {
            return d->downloadMultiBuffer.read(data, maxlen);
        }

        qint64 ret = d->cacheLoadDevice->read(data, maxlen);
        return ret;
    }

    // zerocopy buffer
    if (d->downloadZerocopyBuffer) {
        // FIXME bytesdownloaded, position etc?

        qint64 howMuch = qMin(maxlen, (d->downloadBufferCurrentSize - d->downloadBufferReadPosition));
        memcpy(data, d->downloadZerocopyBuffer + d->downloadBufferReadPosition, howMuch);
        d->downloadBufferReadPosition += howMuch;
        return howMuch;

    }

    // normal buffer
    if (d->downloadMultiBuffer.isEmpty()) {
        if (d->state == d->Finished || d->state == d->Aborted)
            return -1;
        return 0;
    }

    if (maxlen == 1) {
        // optimization for getChar()
        *data = d->downloadMultiBuffer.getChar();
        if (readBufferSize())
            emit readBufferFreed(1);
        return 1;
    }

    maxlen = qMin<qint64>(maxlen, d->downloadMultiBuffer.byteAmount());
    qint64 bytesRead = d->downloadMultiBuffer.read(data, maxlen);
    if (readBufferSize())
        emit readBufferFreed(bytesRead);
    return bytesRead;
}

void QNetworkReplyHttpImpl::setReadBufferSize(qint64 size)
{
    QNetworkReply::setReadBufferSize(size);
    emit readBufferSizeChanged(size);
    return;
}

bool QNetworkReplyHttpImpl::canReadLine () const
{
    Q_D(const QNetworkReplyHttpImpl);

    if (QNetworkReply::canReadLine())
        return true;

    if (d->cacheLoadDevice)
        return d->cacheLoadDevice->canReadLine() || d->downloadMultiBuffer.canReadLine();

    if (d->downloadZerocopyBuffer)
        return memchr(d->downloadZerocopyBuffer + d->downloadBufferReadPosition, '\n', d->downloadBufferCurrentSize - d->downloadBufferReadPosition);

    return d->downloadMultiBuffer.canReadLine();
}

#ifndef QT_NO_SSL
void QNetworkReplyHttpImpl::ignoreSslErrors()
{
    Q_D(QNetworkReplyHttpImpl);

    d->pendingIgnoreAllSslErrors = true;
}

void QNetworkReplyHttpImpl::ignoreSslErrorsImplementation(const QList<QSslError> &errors)
{
    Q_D(QNetworkReplyHttpImpl);

    // the pending list is set if QNetworkReply::ignoreSslErrors(const QList<QSslError> &errors)
    // is called before QNetworkAccessManager::get() (or post(), etc.)
    d->pendingIgnoreSslErrorsList = errors;
}

void QNetworkReplyHttpImpl::setSslConfigurationImplementation(const QSslConfiguration &newconfig)
{
    // Setting a SSL configuration on a reply is not supported. The user needs to set
    // her/his QSslConfiguration on the QNetworkRequest.
    Q_UNUSED(newconfig);
}

void QNetworkReplyHttpImpl::sslConfigurationImplementation(QSslConfiguration &configuration) const
{
    Q_D(const QNetworkReplyHttpImpl);
    configuration = d->sslConfiguration;
}
#endif

QNetworkReplyHttpImplPrivate::QNetworkReplyHttpImplPrivate()
    : QNetworkReplyPrivate()
    , manager(0)
    , managerPrivate(0)
    , synchronous(false)
    , state(Idle)
    , statusCode(0)
    , outgoingData(0)
    , bytesUploaded(-1)
    , cacheLoadDevice(0)
    , loadingFromCache(false)
    , cacheSaveDevice(0)
    , cacheEnabled(false)
    , resumeOffset(0)
    , preMigrationDownloaded(-1)
    , bytesDownloaded(0)
    , downloadBufferReadPosition(0)
    , downloadBufferCurrentSize(0)
    , downloadZerocopyBuffer(0)
    , pendingDownloadDataEmissions(new QAtomicInt())
    , pendingDownloadProgressEmissions(new QAtomicInt())
    #ifndef QT_NO_SSL
    , pendingIgnoreAllSslErrors(false)
    #endif

{
}

QNetworkReplyHttpImplPrivate::~QNetworkReplyHttpImplPrivate()
{
    Q_Q(QNetworkReplyHttpImpl);
    // This will do nothing if the request was already finished or aborted
    emit q->abortHttpRequest();
}

/*
    For a given httpRequest
    1) If AlwaysNetwork, return
    2) If we have a cache entry for this url populate headers so the server can return 304
    3) Calculate if response_is_fresh and if so send the cache and set loadedFromCache to true
 */
bool QNetworkReplyHttpImplPrivate::loadFromCacheIfAllowed(QHttpNetworkRequest &httpRequest)
{
    QNetworkRequest::CacheLoadControl CacheLoadControlAttribute =
        (QNetworkRequest::CacheLoadControl)request.attribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferNetwork).toInt();
    if (CacheLoadControlAttribute == QNetworkRequest::AlwaysNetwork) {
        // If the request does not already specify preferred cache-control
        // force reload from the network and tell any caching proxy servers to reload too
        if (!request.rawHeaderList().contains("Cache-Control")) {
            httpRequest.setHeaderField("Cache-Control", "no-cache");
            httpRequest.setHeaderField("Pragma", "no-cache");
        }
        return false;
    }

    // The disk cache API does not currently support partial content retrieval.
    // That is why we don't use the disk cache for any such requests.
    if (request.hasRawHeader("Range"))
        return false;

    QAbstractNetworkCache *nc = managerPrivate->networkCache;
    if (!nc)
        return false;                 // no local cache

    QNetworkCacheMetaData metaData = nc->metaData(request.url());
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

    bool response_is_fresh;
    if (!expirationDate.isValid()) {
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
                expirationDate = lastModified.addSecs(diff / 10);
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
        response_is_fresh = (freshness_lifetime > current_age);
    } else {
        // expiration date was calculated earlier (e.g. when storing object to the cache)
        response_is_fresh = currentDateTime.secsTo(expirationDate) >= 0;
    }

    if (!response_is_fresh)
        return false;

#if defined(QNETWORKACCESSHTTPBACKEND_DEBUG)
    qDebug() << "response_is_fresh" << CacheLoadControlAttribute;
#endif
    return sendCacheContents(metaData);
}

QHttpNetworkRequest::Priority QNetworkReplyHttpImplPrivate::convert(const QNetworkRequest::Priority& prio)
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

void QNetworkReplyHttpImplPrivate::postRequest()
{
    Q_Q(QNetworkReplyHttpImpl);

    QThread *thread = 0;
    if (synchronous) {
        // A synchronous HTTP request uses its own thread
        thread = new QThread();
        thread->setObjectName(QStringLiteral("Qt HTTP synchronous thread"));
        QObject::connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
        thread->start();
    } else if (!managerPrivate->httpThread) {
        // We use the manager-global thread.
        // At some point we could switch to having multiple threads if it makes sense.
        managerPrivate->httpThread = new QThread();
        managerPrivate->httpThread->setObjectName(QStringLiteral("Qt HTTP thread"));
        managerPrivate->httpThread->start();

        thread = managerPrivate->httpThread;
    } else {
        // Asynchronous request, thread already exists
        thread = managerPrivate->httpThread;
    }

    QUrl url = request.url();
    httpRequest.setUrl(url);

    QString scheme = url.scheme().toLower();
    bool ssl = (scheme == QLatin1String("https")
                || scheme == QLatin1String("preconnect-https"));
    q->setAttribute(QNetworkRequest::ConnectionEncryptedAttribute, ssl);
    httpRequest.setSsl(ssl);

    bool preConnect = (scheme == QLatin1String("preconnect-http")
                       || scheme == QLatin1String("preconnect-https"));
    httpRequest.setPreConnect(preConnect);

#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy transparentProxy, cacheProxy;

    // FIXME the proxy stuff should be done in the HTTP thread
    foreach (const QNetworkProxy &p, managerPrivate->queryProxy(QNetworkProxyQuery(request.url()))) {
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
        QMetaObject::invokeMethod(q, "_q_error", synchronous ? Qt::DirectConnection : Qt::QueuedConnection,
                                  Q_ARG(QNetworkReply::NetworkError, QNetworkReply::ProxyNotFoundError),
                                  Q_ARG(QString, QNetworkReplyHttpImpl::tr("No suitable proxy found")));
        QMetaObject::invokeMethod(q, "_q_finished", synchronous ? Qt::DirectConnection : Qt::QueuedConnection);
        return;
    }
#endif


    bool loadedFromCache = false;
    httpRequest.setPriority(convert(request.priority()));

    switch (operation) {
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
        httpRequest.setCustomVerb(request.attribute(
                QNetworkRequest::CustomVerbAttribute).toByteArray());
        break;

    default:
        break;                  // can't happen
    }

    if (loadedFromCache) {
        return;    // no need to send the request! :)
    }

    QList<QByteArray> headers = request.rawHeaderList();
    if (resumeOffset != 0) {
        if (headers.contains("Range")) {
            // Need to adjust resume offset for user specified range

            headers.removeOne("Range");

            // We've already verified that requestRange starts with "bytes=", see canResume.
            QByteArray requestRange = request.rawHeader("Range").mid(6);

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
        httpRequest.setHeaderField(header, request.rawHeader(header));

    if (request.attribute(QNetworkRequest::HttpPipeliningAllowedAttribute).toBool() == true)
        httpRequest.setPipeliningAllowed(true);

    if (request.attribute(QNetworkRequest::SpdyAllowedAttribute).toBool() == true)
        httpRequest.setSPDYAllowed(true);

    if (static_cast<QNetworkRequest::LoadControl>
        (request.attribute(QNetworkRequest::AuthenticationReuseAttribute,
                             QNetworkRequest::Automatic).toInt()) == QNetworkRequest::Manual)
        httpRequest.setWithCredentials(false);


    // Create the HTTP thread delegate
    QHttpThreadDelegate *delegate = new QHttpThreadDelegate;
#ifndef QT_NO_BEARERMANAGEMENT
    delegate->networkSession = managerPrivate->getNetworkSession();
#endif

    // For the synchronous HTTP, this is the normal way the delegate gets deleted
    // For the asynchronous HTTP this is a safety measure, the delegate deletes itself when HTTP is finished
    QObject::connect(thread, SIGNAL(finished()), delegate, SLOT(deleteLater()));

    // Set the properties it needs
    delegate->httpRequest = httpRequest;
#ifndef QT_NO_NETWORKPROXY
    delegate->cacheProxy = cacheProxy;
    delegate->transparentProxy = transparentProxy;
#endif
    delegate->ssl = ssl;
#ifndef QT_NO_SSL
    if (ssl)
        delegate->incomingSslConfiguration = request.sslConfiguration();
#endif

    // Do we use synchronous HTTP?
    delegate->synchronous = synchronous;

    // The authentication manager is used to avoid the BlockingQueuedConnection communication
    // from HTTP thread to user thread in some cases.
    delegate->authenticationManager = managerPrivate->authenticationManager;

    if (!synchronous) {
        // Tell our zerocopy policy to the delegate
        QVariant downloadBufferMaximumSizeAttribute = request.attribute(QNetworkRequest::MaximumDownloadBufferSizeAttribute);
        if (downloadBufferMaximumSizeAttribute.isValid()) {
            delegate->downloadBufferMaximumSize = downloadBufferMaximumSizeAttribute.toLongLong();
        } else {
            // If there is no MaximumDownloadBufferSizeAttribute set (which is for the majority
            // of QNetworkRequest) then we can assume we'll do it anyway for small HTTP replies.
            // This helps with performance and memory fragmentation.
            delegate->downloadBufferMaximumSize = 128*1024;
        }


        // These atomic integers are used for signal compression
        delegate->pendingDownloadData = pendingDownloadDataEmissions;
        delegate->pendingDownloadProgress = pendingDownloadProgressEmissions;

        // Connect the signals of the delegate to us
        QObject::connect(delegate, SIGNAL(downloadData(QByteArray)),
                q, SLOT(replyDownloadData(QByteArray)),
                Qt::QueuedConnection);
        QObject::connect(delegate, SIGNAL(downloadFinished()),
                q, SLOT(replyFinished()),
                Qt::QueuedConnection);
        QObject::connect(delegate, SIGNAL(downloadMetaData(QList<QPair<QByteArray,QByteArray> >,
                                                           int, QString, bool,
                                                           QSharedPointer<char>, qint64, bool)),
                q, SLOT(replyDownloadMetaData(QList<QPair<QByteArray,QByteArray> >,
                                              int, QString, bool,
                                              QSharedPointer<char>, qint64, bool)),
                Qt::QueuedConnection);
        QObject::connect(delegate, SIGNAL(downloadProgress(qint64,qint64)),
                q, SLOT(replyDownloadProgressSlot(qint64,qint64)),
                Qt::QueuedConnection);
        QObject::connect(delegate, SIGNAL(error(QNetworkReply::NetworkError,QString)),
                q, SLOT(httpError(QNetworkReply::NetworkError,QString)),
                Qt::QueuedConnection);
#ifndef QT_NO_SSL
        QObject::connect(delegate, SIGNAL(sslConfigurationChanged(QSslConfiguration)),
                q, SLOT(replySslConfigurationChanged(QSslConfiguration)),
                Qt::QueuedConnection);
#endif
        // Those need to report back, therefire BlockingQueuedConnection
        QObject::connect(delegate, SIGNAL(authenticationRequired(QHttpNetworkRequest,QAuthenticator*)),
                q, SLOT(httpAuthenticationRequired(QHttpNetworkRequest,QAuthenticator*)),
                Qt::BlockingQueuedConnection);
#ifndef QT_NO_NETWORKPROXY
        QObject::connect(delegate, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
                 q, SLOT(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
                 Qt::BlockingQueuedConnection);
#endif
#ifndef QT_NO_SSL
        QObject::connect(delegate, SIGNAL(encrypted()), q, SLOT(replyEncrypted()),
                Qt::BlockingQueuedConnection);
        QObject::connect(delegate, SIGNAL(sslErrors(QList<QSslError>,bool*,QList<QSslError>*)),
                q, SLOT(replySslErrors(QList<QSslError>,bool*,QList<QSslError>*)),
                Qt::BlockingQueuedConnection);
#endif
        // This signal we will use to start the request.
        QObject::connect(q, SIGNAL(startHttpRequest()), delegate, SLOT(startRequest()));
        QObject::connect(q, SIGNAL(abortHttpRequest()), delegate, SLOT(abortRequest()));

        // To throttle the connection.
        QObject::connect(q, SIGNAL(readBufferSizeChanged(qint64)), delegate, SLOT(readBufferSizeChanged(qint64)));
        QObject::connect(q, SIGNAL(readBufferFreed(qint64)), delegate, SLOT(readBufferFreed(qint64)));

        if (uploadByteDevice) {
            QNonContiguousByteDeviceThreadForwardImpl *forwardUploadDevice =
                    new QNonContiguousByteDeviceThreadForwardImpl(uploadByteDevice->atEnd(), uploadByteDevice->size());
            if (uploadByteDevice->isResetDisabled())
                forwardUploadDevice->disableReset();
            forwardUploadDevice->setParent(delegate); // needed to make sure it is moved on moveToThread()
            delegate->httpRequest.setUploadByteDevice(forwardUploadDevice);

            // From main thread to user thread:
            QObject::connect(q, SIGNAL(haveUploadData(QByteArray,bool,qint64)),
                             forwardUploadDevice, SLOT(haveDataSlot(QByteArray,bool,qint64)), Qt::QueuedConnection);
            QObject::connect(uploadByteDevice.data(), SIGNAL(readyRead()),
                             forwardUploadDevice, SIGNAL(readyRead()),
                             Qt::QueuedConnection);

            // From http thread to user thread:
            QObject::connect(forwardUploadDevice, SIGNAL(wantData(qint64)),
                             q, SLOT(wantUploadDataSlot(qint64)));
            QObject::connect(forwardUploadDevice, SIGNAL(processedData(qint64)),
                             q, SLOT(sentUploadDataSlot(qint64)));
            QObject::connect(forwardUploadDevice, SIGNAL(resetData(bool*)),
                    q, SLOT(resetUploadDataSlot(bool*)),
                    Qt::BlockingQueuedConnection); // this is the only one with BlockingQueued!
        }
    } else if (synchronous) {
        QObject::connect(q, SIGNAL(startHttpRequestSynchronously()), delegate, SLOT(startRequestSynchronously()), Qt::BlockingQueuedConnection);

        if (uploadByteDevice) {
            // For the synchronous HTTP use case the use thread (this one here) is blocked
            // so we cannot use the asynchronous upload architecture.
            // We therefore won't use the QNonContiguousByteDeviceThreadForwardImpl but directly
            // use the uploadByteDevice provided to us by the QNetworkReplyImpl.
            // The code that is in start() makes sure it is safe to use from a thread
            // since it only wraps a QRingBuffer
            delegate->httpRequest.setUploadByteDevice(uploadByteDevice.data());
        }
    }


    // Move the delegate to the http thread
    delegate->moveToThread(thread);
    // This call automatically moves the uploadDevice too for the asynchronous case.

    // Prepare timers for progress notifications
    downloadProgressSignalChoke.start();
    uploadProgressSignalChoke.invalidate();

    // Send an signal to the delegate so it starts working in the other thread
    if (synchronous) {
        emit q->startHttpRequestSynchronously(); // This one is BlockingQueuedConnection, so it will return when all work is done

        if (delegate->incomingErrorCode != QNetworkReply::NoError) {
            replyDownloadMetaData
                    (delegate->incomingHeaders,
                     delegate->incomingStatusCode,
                     delegate->incomingReasonPhrase,
                     delegate->isPipeliningUsed,
                     QSharedPointer<char>(),
                     delegate->incomingContentLength,
                     delegate->isSpdyUsed);
            replyDownloadData(delegate->synchronousDownloadData);
            httpError(delegate->incomingErrorCode, delegate->incomingErrorDetail);
        } else {
            replyDownloadMetaData
                    (delegate->incomingHeaders,
                     delegate->incomingStatusCode,
                     delegate->incomingReasonPhrase,
                     delegate->isPipeliningUsed,
                     QSharedPointer<char>(),
                     delegate->incomingContentLength,
                     delegate->isSpdyUsed);
            replyDownloadData(delegate->synchronousDownloadData);
        }

        thread->quit();
        thread->wait(5000);
        if (thread->isFinished())
            delete thread;
        else
            QObject::connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

        finished();
    } else {
        emit q->startHttpRequest(); // Signal to the HTTP thread and go back to user.
    }
}

void QNetworkReplyHttpImplPrivate::invalidateCache()
{
    QAbstractNetworkCache *nc = managerPrivate->networkCache;
    if (nc)
        nc->remove(request.url());
}

void QNetworkReplyHttpImplPrivate::initCacheSaveDevice()
{
    Q_Q(QNetworkReplyHttpImpl);

    // The disk cache does not support partial content, so don't even try to
    // save any such content into the cache.
    if (q->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 206) {
        cacheEnabled = false;
        return;
    }

    // save the meta data
    QNetworkCacheMetaData metaData;
    metaData.setUrl(url);
    metaData = fetchCacheMetaData(metaData);

    // save the redirect request also in the cache
    QVariant redirectionTarget = q->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (redirectionTarget.isValid()) {
        QNetworkCacheMetaData::AttributesMap attributes = metaData.attributes();
        attributes.insert(QNetworkRequest::RedirectionTargetAttribute, redirectionTarget);
        metaData.setAttributes(attributes);
    }

    cacheSaveDevice = managerPrivate->networkCache->prepare(metaData);

    if (!cacheSaveDevice || (cacheSaveDevice && !cacheSaveDevice->isOpen())) {
        if (cacheSaveDevice && !cacheSaveDevice->isOpen())
            qCritical("QNetworkReplyImpl: network cache returned a device that is not open -- "
                  "class %s probably needs to be fixed",
                  managerPrivate->networkCache->metaObject()->className());

        managerPrivate->networkCache->remove(url);
        cacheSaveDevice = 0;
        cacheEnabled = false;
    }
}

void QNetworkReplyHttpImplPrivate::replyDownloadData(QByteArray d)
{
    Q_Q(QNetworkReplyHttpImpl);

    // If we're closed just ignore this data
    if (!q->isOpen())
        return;

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

    if (cacheEnabled && isCachingAllowed() && !cacheSaveDevice) {
        initCacheSaveDevice();
    }

    qint64 bytesWritten = 0;
    for (int i = 0; i < pendingDownloadDataCopy.bufferCount(); i++) {
        QByteArray const &item = pendingDownloadDataCopy[i];

        if (cacheSaveDevice)
            cacheSaveDevice->write(item.constData(), item.size());
        downloadMultiBuffer.append(item);

        bytesWritten += item.size();
    }
    pendingDownloadDataCopy.clear();

    bytesDownloaded += bytesWritten;


    QVariant totalSize = cookedHeaders.value(QNetworkRequest::ContentLengthHeader);
    if (preMigrationDownloaded != Q_INT64_C(-1))
        totalSize = totalSize.toLongLong() + preMigrationDownloaded;

    emit q->readyRead();
    // emit readyRead before downloadProgress incase this will cause events to be
    // processed and we get into a recursive call (as in QProgressDialog).
    if (downloadProgressSignalChoke.elapsed() >= progressSignalInterval) {
        downloadProgressSignalChoke.restart();
        emit q->downloadProgress(bytesDownloaded,
                             totalSize.isNull() ? Q_INT64_C(-1) : totalSize.toLongLong());
    }

}

void QNetworkReplyHttpImplPrivate::replyFinished()
{
    // We are already loading from cache, we still however
    // got this signal because it was posted already
    if (loadingFromCache)
        return;

    finished();
}

void QNetworkReplyHttpImplPrivate::checkForRedirect(const int statusCode)
{
    Q_Q(QNetworkReplyHttpImpl);
    switch (statusCode) {
    case 301:                   // Moved Permanently
    case 302:                   // Found
    case 303:                   // See Other
    case 307:                   // Temporary Redirect
        // What do we do about the caching of the HTML note?
        // The response to a 303 MUST NOT be cached, while the response to
        // all of the others is cacheable if the headers indicate it to be
        QByteArray header = q->rawHeader("location");
        QUrl url = QUrl(QString::fromUtf8(header));
        if (!url.isValid())
            url = QUrl(QLatin1String(header));
        q->setAttribute(QNetworkRequest::RedirectionTargetAttribute, url);
    }
}

void QNetworkReplyHttpImplPrivate::replyDownloadMetaData
        (QList<QPair<QByteArray,QByteArray> > hm,
         int sc,QString rp,bool pu,
         QSharedPointer<char> db,
         qint64 contentLength, bool spdyWasUsed)
{
    Q_Q(QNetworkReplyHttpImpl);
    Q_UNUSED(contentLength);

    statusCode = sc;
    reasonPhrase = rp;

    // Download buffer
    if (!db.isNull()) {
        downloadBufferPointer = db;
        downloadZerocopyBuffer = downloadBufferPointer.data();
        downloadBufferCurrentSize = 0;
        q->setAttribute(QNetworkRequest::DownloadBufferAttribute, QVariant::fromValue<QSharedPointer<char> > (downloadBufferPointer));
    }

    q->setAttribute(QNetworkRequest::HttpPipeliningWasUsedAttribute, pu);
    q->setAttribute(QNetworkRequest::SpdyWasUsedAttribute, spdyWasUsed);

    // reconstruct the HTTP header
    QList<QPair<QByteArray, QByteArray> > headerMap = hm;
    QList<QPair<QByteArray, QByteArray> >::ConstIterator it = headerMap.constBegin(),
                                                        end = headerMap.constEnd();
    for (; it != end; ++it) {
        QByteArray value = q->rawHeader(it->first);
        if (!value.isEmpty()) {
            if (qstricmp(it->first.constData(), "set-cookie") == 0)
                value += '\n';
            else
                value += ", ";
        }
        value += it->second;
        q->setRawHeader(it->first, value);
    }

    q->setAttribute(QNetworkRequest::HttpStatusCodeAttribute, statusCode);
    q->setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, reasonPhrase);

    // is it a redirection?
    checkForRedirect(statusCode);

    if (statusCode >= 500 && statusCode < 600) {
        QAbstractNetworkCache *nc = managerPrivate->networkCache;
        if (nc) {
            QNetworkCacheMetaData metaData = nc->metaData(request.url());
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
        QAbstractNetworkCache *nc = managerPrivate->networkCache;
        if (nc) {
            QNetworkCacheMetaData oldMetaData = nc->metaData(request.url());
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

void QNetworkReplyHttpImplPrivate::replyDownloadProgressSlot(qint64 bytesReceived,  qint64 bytesTotal)
{
    Q_Q(QNetworkReplyHttpImpl);

    // If we're closed just ignore this data
    if (!q->isOpen())
        return;

    // we can be sure here that there is a download buffer

    int pendingSignals = (int)pendingDownloadProgressEmissions->fetchAndAddAcquire(-1) - 1;
    if (pendingSignals > 0) {
        // Let's ignore this signal and look at the next one coming in
        // (signal comppression)
        return;
    }

    if (!q->isOpen())
        return;

    if (cacheEnabled && isCachingAllowed() && bytesReceived == bytesTotal) {
        // Write everything in one go if we use a download buffer. might be more performant.
        initCacheSaveDevice();
        // need to check again if cache enabled and device exists
        if (cacheSaveDevice && cacheEnabled)
            cacheSaveDevice->write(downloadZerocopyBuffer, bytesTotal);
        // FIXME where is it closed?
    }

    bytesDownloaded = bytesReceived;

    downloadBufferCurrentSize = bytesReceived;

    // Only emit readyRead when actual data is there
    // emit readyRead before downloadProgress incase this will cause events to be
    // processed and we get into a recursive call (as in QProgressDialog).
    if (bytesDownloaded > 0)
        emit q->readyRead();
    if (downloadProgressSignalChoke.elapsed() >= progressSignalInterval) {
        downloadProgressSignalChoke.restart();
        emit q->downloadProgress(bytesDownloaded, bytesTotal);
    }
}

void QNetworkReplyHttpImplPrivate::httpAuthenticationRequired(const QHttpNetworkRequest &request,
                                                           QAuthenticator *auth)
{
    managerPrivate->authenticationRequired(auth, q_func(), synchronous, url, &urlForLastAuthentication, request.withCredentials());
}

#ifndef QT_NO_NETWORKPROXY
void QNetworkReplyHttpImplPrivate::proxyAuthenticationRequired(const QNetworkProxy &proxy,
                                                        QAuthenticator *authenticator)
{
    managerPrivate->proxyAuthenticationRequired(proxy, synchronous, authenticator, &lastProxyAuthentication);
}
#endif

void QNetworkReplyHttpImplPrivate::httpError(QNetworkReply::NetworkError errorCode,
                                          const QString &errorString)
{
#if defined(QNETWORKACCESSHTTPBACKEND_DEBUG)
    qDebug() << "http error!" << errorCode << errorString;
#endif

    // FIXME?
    error(errorCode, errorString);
}

#ifndef QT_NO_SSL
void QNetworkReplyHttpImplPrivate::replyEncrypted()
{
    Q_Q(QNetworkReplyHttpImpl);
    emit q->encrypted();
}

void QNetworkReplyHttpImplPrivate::replySslErrors(
        const QList<QSslError> &list, bool *ignoreAll, QList<QSslError> *toBeIgnored)
{
    Q_Q(QNetworkReplyHttpImpl);
    emit q->sslErrors(list);
    // Check if the callback set any ignore and return this here to http thread
    if (pendingIgnoreAllSslErrors)
        *ignoreAll = true;
    if (!pendingIgnoreSslErrorsList.isEmpty())
        *toBeIgnored = pendingIgnoreSslErrorsList;
}

void QNetworkReplyHttpImplPrivate::replySslConfigurationChanged(const QSslConfiguration &sslConfiguration)
{
    // Receiving the used SSL configuration from the HTTP thread
    this->sslConfiguration = sslConfiguration;
}
#endif

// Coming from QNonContiguousByteDeviceThreadForwardImpl in HTTP thread
void QNetworkReplyHttpImplPrivate::resetUploadDataSlot(bool *r)
{
    *r = uploadByteDevice->reset();
}

// Coming from QNonContiguousByteDeviceThreadForwardImpl in HTTP thread
void QNetworkReplyHttpImplPrivate::sentUploadDataSlot(qint64 amount)
{
    uploadByteDevice->advanceReadPointer(amount);
}

// Coming from QNonContiguousByteDeviceThreadForwardImpl in HTTP thread
void QNetworkReplyHttpImplPrivate::wantUploadDataSlot(qint64 maxSize)
{
    Q_Q(QNetworkReplyHttpImpl);

    // call readPointer
    qint64 currentUploadDataLength = 0;
    char *data = const_cast<char*>(uploadByteDevice->readPointer(maxSize, currentUploadDataLength));
    // Let's make a copy of this data
    QByteArray dataArray(data, currentUploadDataLength);

    // Communicate back to HTTP thread
    emit q->haveUploadData(dataArray, uploadByteDevice->atEnd(), uploadByteDevice->size());
}

/*
    A simple web page that can be used to test us: http://www.procata.com/cachetest/
 */
bool QNetworkReplyHttpImplPrivate::sendCacheContents(const QNetworkCacheMetaData &metaData)
{
    Q_Q(QNetworkReplyHttpImpl);

    setCachingEnabled(false);
    if (!metaData.isValid())
        return false;

    QAbstractNetworkCache *nc = managerPrivate->networkCache;
    Q_ASSERT(nc);
    QIODevice *contents = nc->data(url);
    if (!contents) {
#if defined(QNETWORKACCESSHTTPBACKEND_DEBUG)
        qDebug() << "Can not send cache, the contents are 0" << url;
#endif
        return false;
    }
    contents->setParent(q);

    QNetworkCacheMetaData::AttributesMap attributes = metaData.attributes();
    int status = attributes.value(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status < 100)
        status = 200;           // fake it

    q->setAttribute(QNetworkRequest::HttpStatusCodeAttribute, status);
    q->setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, attributes.value(QNetworkRequest::HttpReasonPhraseAttribute));
    q->setAttribute(QNetworkRequest::SourceIsFromCacheAttribute, true);

    QNetworkCacheMetaData::RawHeaderList rawHeaders = metaData.rawHeaders();
    QNetworkCacheMetaData::RawHeaderList::ConstIterator it = rawHeaders.constBegin(),
                                                       end = rawHeaders.constEnd();
    for ( ; it != end; ++it)
        setRawHeader(it->first, it->second);

    checkForRedirect(status);

    cacheLoadDevice = contents;
    q->connect(cacheLoadDevice, SIGNAL(readyRead()), SLOT(_q_cacheLoadReadyRead()));
    q->connect(cacheLoadDevice, SIGNAL(readChannelFinished()), SLOT(_q_cacheLoadReadyRead()));

    // This needs to be emitted in the event loop because it can be reached at
    // the direct code path of qnam.get(...) before the user has a chance
    // to connect any signals.
    QMetaObject::invokeMethod(q, "metaDataChanged", Qt::QueuedConnection);
    QMetaObject::invokeMethod(q, "_q_cacheLoadReadyRead", Qt::QueuedConnection);


#if defined(QNETWORKACCESSHTTPBACKEND_DEBUG)
    qDebug() << "Successfully sent cache:" << url << contents->size() << "bytes";
#endif

    // Set the following flag so we can ignore some signals from HTTP thread
    // that would still come
    loadingFromCache = true;
    return true;
}

QNetworkCacheMetaData QNetworkReplyHttpImplPrivate::fetchCacheMetaData(const QNetworkCacheMetaData &oldMetaData) const
{
    Q_Q(const QNetworkReplyHttpImpl);

    QNetworkCacheMetaData metaData = oldMetaData;

    QNetworkHeadersPrivate cacheHeaders;
    cacheHeaders.setAllRawHeaders(metaData.rawHeaders());
    QNetworkHeadersPrivate::RawHeadersList::ConstIterator it;

    QList<QByteArray> newHeaders = q->rawHeaderList();
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

        // for 4.6.0, we were planning to not store the date header in the
        // cached resource; through that we planned to reduce the number
        // of writes to disk when using a QNetworkDiskCache (i.e. don't
        // write to disk when only the date changes).
        // However, without the date we cannot calculate the age of the page
        // anymore.
        //if (header == "date")
            //continue;

        // Don't store Warning 1xx headers
        if (header == "warning") {
            QByteArray v = q->rawHeader(header);
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
        cacheHeaders.setRawHeader(originalHeader, q->rawHeader(header));
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

bool QNetworkReplyHttpImplPrivate::canResume() const
{
    Q_Q(const QNetworkReplyHttpImpl);

    // Only GET operation supports resuming.
    if (operation != QNetworkAccessManager::GetOperation)
        return false;

    // Can only resume if server/resource supports Range header.
    QByteArray acceptRangesheaderName("Accept-Ranges");
    if (!q->hasRawHeader(acceptRangesheaderName) || q->rawHeader(acceptRangesheaderName) == "none")
        return false;

    // We only support resuming for byte ranges.
    if (request.hasRawHeader("Range")) {
        QByteArray range = request.rawHeader("Range");
        if (!range.startsWith("bytes="))
            return false;
    }

    // If we're using a download buffer then we don't support resuming/migration
    // right now. Too much trouble.
    if (downloadZerocopyBuffer)
        return false;

    return true;
}

void QNetworkReplyHttpImplPrivate::setResumeOffset(quint64 offset)
{
    resumeOffset = offset;
}

/*!
    Starts the backend.  Returns \c true if the backend is started.  Returns \c false if the backend
    could not be started due to an unopened or roaming session.  The caller should recall this
    function once the session has been opened or the roaming process has finished.
*/
bool QNetworkReplyHttpImplPrivate::start()
{
#ifndef QT_NO_BEARERMANAGEMENT
    QSharedPointer<QNetworkSession> networkSession(managerPrivate->getNetworkSession());
    if (!networkSession) {
#endif
        postRequest();
        return true;
#ifndef QT_NO_BEARERMANAGEMENT
    }

    // This is not ideal.
    const QString host = url.host();
    if (host == QLatin1String("localhost") ||
        QHostAddress(host).isLoopback()) {
        // Don't need an open session for localhost access.
        postRequest();
        return true;
    }

    if (networkSession->isOpen() &&
        networkSession->state() == QNetworkSession::Connected) {
        Q_Q(QNetworkReplyHttpImpl);
        QObject::connect(networkSession.data(), SIGNAL(usagePoliciesChanged(QNetworkSession::UsagePolicies)),
                            q, SLOT(_q_networkSessionUsagePoliciesChanged(QNetworkSession::UsagePolicies)));
        postRequest();
        return true;
    }

    return false;
#endif
}

void QNetworkReplyHttpImplPrivate::_q_startOperation()
{
    Q_Q(QNetworkReplyHttpImpl);

    // ensure this function is only being called once
    if (state == Working) {
        qDebug("QNetworkReplyImpl::_q_startOperation was called more than once");
        return;
    }
    state = Working;

#ifndef QT_NO_BEARERMANAGEMENT
    // Do not start background requests if they are not allowed by session policy
    QSharedPointer<QNetworkSession> session(manager->d_func()->getNetworkSession());
    QVariant isBackground = request.attribute(QNetworkRequest::BackgroundRequestAttribute, QVariant::fromValue(false));
    if (isBackground.toBool() && session && session->usagePolicies().testFlag(QNetworkSession::NoBackgroundTrafficPolicy)) {
        QMetaObject::invokeMethod(q, "_q_error", synchronous ? Qt::DirectConnection : Qt::QueuedConnection,
            Q_ARG(QNetworkReply::NetworkError, QNetworkReply::BackgroundRequestNotAllowedError),
            Q_ARG(QString, QCoreApplication::translate("QNetworkReply", "Background request not allowed.")));
        QMetaObject::invokeMethod(q, "_q_finished", synchronous ? Qt::DirectConnection : Qt::QueuedConnection);
        return;
    }
#endif

    if (!start()) {
#ifndef QT_NO_BEARERMANAGEMENT
        // backend failed to start because the session state is not Connected.
        // QNetworkAccessManager will call reply->backend->start() again for us when the session
        // state changes.
        state = WaitingForSession;

        if (session) {
            QObject::connect(session.data(), SIGNAL(error(QNetworkSession::SessionError)),
                             q, SLOT(_q_networkSessionFailed()), Qt::QueuedConnection);

            if (!session->isOpen()) {
                session->setSessionProperty(QStringLiteral("ConnectInBackground"), isBackground);
                session->open();
            }
        } else {
            qWarning("Backend is waiting for QNetworkSession to connect, but there is none!");
            QMetaObject::invokeMethod(q, "_q_error", synchronous ? Qt::DirectConnection : Qt::QueuedConnection,
                Q_ARG(QNetworkReply::NetworkError, QNetworkReply::NetworkSessionFailedError),
                Q_ARG(QString, QCoreApplication::translate("QNetworkReply", "Network session error.")));
            QMetaObject::invokeMethod(q, "_q_finished", synchronous ? Qt::DirectConnection : Qt::QueuedConnection);
            return;
        }
#else
        qWarning("Backend start failed");
        QMetaObject::invokeMethod(q, "_q_error", synchronous ? Qt::DirectConnection : Qt::QueuedConnection,
            Q_ARG(QNetworkReply::NetworkError, QNetworkReply::UnknownNetworkError),
            Q_ARG(QString, QCoreApplication::translate("QNetworkReply", "backend start error.")));
        QMetaObject::invokeMethod(q, "_q_finished", synchronous ? Qt::DirectConnection : Qt::QueuedConnection);
        return;
#endif
    }

    if (synchronous) {
        state = Finished;
        q_func()->setFinished(true);
    } else {
        if (state != Finished) {

        }
    }
}

void QNetworkReplyHttpImplPrivate::_q_cacheLoadReadyRead()
{
    Q_Q(QNetworkReplyHttpImpl);

    if (state != Working)
        return;
    if (!cacheLoadDevice || !q->isOpen() || !cacheLoadDevice->bytesAvailable())
        return;

    // FIXME Optimize to use zerocopy download buffer if it is a QBuffer.
    // Needs to be done where sendCacheContents() (?) of HTTP is emitting
    // metaDataChanged ?


    QVariant totalSize = cookedHeaders.value(QNetworkRequest::ContentLengthHeader);

    // emit readyRead before downloadProgress incase this will cause events to be
    // processed and we get into a recursive call (as in QProgressDialog).

    // This readyRead() goes to the user. The user then may or may not read() anything.
    emit q->readyRead();
    if (downloadProgressSignalChoke.elapsed() >= progressSignalInterval) {
        downloadProgressSignalChoke.restart();
        emit q->downloadProgress(bytesDownloaded,
                             totalSize.isNull() ? Q_INT64_C(-1) : totalSize.toLongLong());
    }

    // If there are still bytes available in the cacheLoadDevice then the user did not read
    // in response to the readyRead() signal. This means we have to load from the cacheLoadDevice
    // and buffer that stuff. This is needed to be able to properly emit finished() later.
    while (cacheLoadDevice->bytesAvailable()) {
        downloadMultiBuffer.append(cacheLoadDevice->readAll());
    }

    if (cacheLoadDevice->isSequential()) {
        // check if end and we can read the EOF -1
        char c;
        qint64 actualCount = cacheLoadDevice->read(&c, 1);
        if (actualCount < 0) {
            cacheLoadDevice->deleteLater();
            cacheLoadDevice = 0;
            QMetaObject::invokeMethod(q, "_q_finished", Qt::QueuedConnection);
        } else if (actualCount == 1) {
            // This is most probably not happening since most QIODevice returned something proper for bytesAvailable()
            // and had already been "emptied".
            cacheLoadDevice->ungetChar(c);
        }
    } else if ((!cacheLoadDevice->isSequential() && cacheLoadDevice->atEnd())) {
        // This codepath is in case the cache device is a QBuffer, e.g. from QNetworkDiskCache.
        cacheLoadDevice->deleteLater();
        cacheLoadDevice = 0;
        QMetaObject::invokeMethod(q, "_q_finished", Qt::QueuedConnection);
    }

}


void QNetworkReplyHttpImplPrivate::_q_bufferOutgoingDataFinished()
{
    Q_Q(QNetworkReplyHttpImpl);

    // make sure this is only called once, ever.
    //_q_bufferOutgoingData may call it or the readChannelFinished emission
    if (state != Buffering)
        return;

    // disconnect signals
    QObject::disconnect(outgoingData, SIGNAL(readyRead()), q, SLOT(_q_bufferOutgoingData()));
    QObject::disconnect(outgoingData, SIGNAL(readChannelFinished()), q, SLOT(_q_bufferOutgoingDataFinished()));

    // finally, start the request
    QMetaObject::invokeMethod(q, "_q_startOperation", Qt::QueuedConnection);
}

void QNetworkReplyHttpImplPrivate::_q_bufferOutgoingData()
{
    Q_Q(QNetworkReplyHttpImpl);

    if (!outgoingDataBuffer) {
        // first call, create our buffer
        outgoingDataBuffer = QSharedPointer<QRingBuffer>(new QRingBuffer());

        QObject::connect(outgoingData, SIGNAL(readyRead()), q, SLOT(_q_bufferOutgoingData()));
        QObject::connect(outgoingData, SIGNAL(readChannelFinished()), q, SLOT(_q_bufferOutgoingDataFinished()));
    }

    qint64 bytesBuffered = 0;
    qint64 bytesToBuffer = 0;

    // read data into our buffer
    forever {
        bytesToBuffer = outgoingData->bytesAvailable();
        // unknown? just try 2 kB, this also ensures we always try to read the EOF
        if (bytesToBuffer <= 0)
            bytesToBuffer = 2*1024;

        char *dst = outgoingDataBuffer->reserve(bytesToBuffer);
        bytesBuffered = outgoingData->read(dst, bytesToBuffer);

        if (bytesBuffered == -1) {
            // EOF has been reached.
            outgoingDataBuffer->chop(bytesToBuffer);

            _q_bufferOutgoingDataFinished();
            break;
        } else if (bytesBuffered == 0) {
            // nothing read right now, just wait until we get called again
            outgoingDataBuffer->chop(bytesToBuffer);

            break;
        } else {
            // don't break, try to read() again
            outgoingDataBuffer->chop(bytesToBuffer - bytesBuffered);
        }
    }
}

#ifndef QT_NO_BEARERMANAGEMENT
void QNetworkReplyHttpImplPrivate::_q_networkSessionConnected()
{
    Q_Q(QNetworkReplyHttpImpl);

    if (!manager)
        return;

    QSharedPointer<QNetworkSession> session = managerPrivate->getNetworkSession();
    if (!session)
        return;

    if (session->state() != QNetworkSession::Connected)
        return;

    switch (state) {
    case QNetworkReplyImplPrivate::Buffering:
    case QNetworkReplyImplPrivate::Working:
    case QNetworkReplyImplPrivate::Reconnecting:
        // Migrate existing downloads to new network connection.
        migrateBackend();
        break;
    case QNetworkReplyImplPrivate::WaitingForSession:
        // Start waiting requests.
        QMetaObject::invokeMethod(q, "_q_startOperation", Qt::QueuedConnection);
        break;
    default:
        ;
    }
}

void QNetworkReplyHttpImplPrivate::_q_networkSessionFailed()
{
    // Abort waiting and working replies.
    if (state == WaitingForSession || state == Working) {
        state = Working;
        QSharedPointer<QNetworkSession> session(manager->d_func()->getNetworkSession());
        QString errorStr;
        if (session)
            errorStr = session->errorString();
        else
            errorStr = QCoreApplication::translate("QNetworkReply", "Network session error.");
        error(QNetworkReplyImpl::NetworkSessionFailedError, errorStr);
        finished();
    }
}

void QNetworkReplyHttpImplPrivate::_q_networkSessionUsagePoliciesChanged(QNetworkSession::UsagePolicies newPolicies)
{
    if (request.attribute(QNetworkRequest::BackgroundRequestAttribute).toBool()) {
        if (newPolicies & QNetworkSession::NoBackgroundTrafficPolicy) {
            // Abort waiting and working replies.
            if (state == WaitingForSession || state == Working) {
                state = Working;
                error(QNetworkReply::BackgroundRequestNotAllowedError,
                    QCoreApplication::translate("QNetworkReply", "Background request not allowed."));
                finished();
            }
            // ### if canResume(), then we could resume automatically
        }
    }

}
#endif


// need to have this function since the reply is a private member variable
// and the special backends need to access this.
void QNetworkReplyHttpImplPrivate::emitReplyUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    Q_Q(QNetworkReplyHttpImpl);
    if (isFinished)
        return;

    //choke signal emissions, except the first and last signals which are unconditional
    if (uploadProgressSignalChoke.isValid()) {
        if (bytesSent != bytesTotal && uploadProgressSignalChoke.elapsed() < progressSignalInterval) {
            return;
        }
        uploadProgressSignalChoke.restart();
    } else {
        uploadProgressSignalChoke.start();
    }

    emit q->uploadProgress(bytesSent, bytesTotal);
}

QNonContiguousByteDevice* QNetworkReplyHttpImplPrivate::createUploadByteDevice()
{
    Q_Q(QNetworkReplyHttpImpl);

    if (outgoingDataBuffer)
        uploadByteDevice = QSharedPointer<QNonContiguousByteDevice>(QNonContiguousByteDeviceFactory::create(outgoingDataBuffer));
    else if (outgoingData) {
        uploadByteDevice = QSharedPointer<QNonContiguousByteDevice>(QNonContiguousByteDeviceFactory::create(outgoingData));
    } else {
        return 0;
    }

    bool bufferDisallowed =
            request.attribute(QNetworkRequest::DoNotBufferUploadDataAttribute,
                          QVariant(false)) == QVariant(true);
    if (bufferDisallowed)
        uploadByteDevice->disableReset();

    // We want signal emissions only for normal asynchronous uploads
    if (!synchronous)
        QObject::connect(uploadByteDevice.data(), SIGNAL(readProgress(qint64,qint64)),
                         q, SLOT(emitReplyUploadProgress(qint64,qint64)));

    return uploadByteDevice.data();
}

void QNetworkReplyHttpImplPrivate::_q_finished()
{
    // This gets called queued, just forward to real call then
    finished();
}

void QNetworkReplyHttpImplPrivate::finished()
{
    Q_Q(QNetworkReplyHttpImpl);

    if (state == Finished || state == Aborted || state == WaitingForSession)
        return;

    QVariant totalSize = cookedHeaders.value(QNetworkRequest::ContentLengthHeader);
    if (preMigrationDownloaded != Q_INT64_C(-1))
        totalSize = totalSize.toLongLong() + preMigrationDownloaded;

    if (manager) {
#ifndef QT_NO_BEARERMANAGEMENT
        QSharedPointer<QNetworkSession> session = managerPrivate->getNetworkSession();
        if (session && session->state() == QNetworkSession::Roaming &&
            state == Working && errorCode != QNetworkReply::OperationCanceledError) {
            // only content with a known size will fail with a temporary network failure error
            if (!totalSize.isNull()) {
                if (bytesDownloaded != totalSize) {
                    if (migrateBackend()) {
                        // either we are migrating or the request is finished/aborted
                        if (state == Reconnecting || state == WaitingForSession) {
                            return; // exit early if we are migrating.
                        }
                    } else {
                        error(QNetworkReply::TemporaryNetworkFailureError,
                              QNetworkReply::tr("Temporary network failure."));
                    }
                }
            }
        }
#endif
    }

    state = Finished;
    q->setFinished(true);

    if (totalSize.isNull() || totalSize == -1) {
        emit q->downloadProgress(bytesDownloaded, bytesDownloaded);
    } else {
        emit q->downloadProgress(bytesDownloaded, totalSize.toLongLong());
    }

    if (bytesUploaded == -1 && (outgoingData || outgoingDataBuffer))
        emit q->uploadProgress(0, 0);

    // if we don't know the total size of or we received everything save the cache
    if (totalSize.isNull() || totalSize == -1 || bytesDownloaded == totalSize)
        completeCacheSave();

    emit q->readChannelFinished();
    emit q->finished();
}

void QNetworkReplyHttpImplPrivate::_q_error(QNetworkReplyImpl::NetworkError code, const QString &errorMessage)
{
    this->error(code, errorMessage);
}


void QNetworkReplyHttpImplPrivate::error(QNetworkReplyImpl::NetworkError code, const QString &errorMessage)
{
    Q_Q(QNetworkReplyHttpImpl);
    // Can't set and emit multiple errors.
    if (errorCode != QNetworkReply::NoError) {
        qWarning("QNetworkReplyImplPrivate::error: Internal problem, this method must only be called once.");
        return;
    }

    errorCode = code;
    q->setErrorString(errorMessage);

    // note: might not be a good idea, since users could decide to delete us
    // which would delete the backend too...
    // maybe we should protect the backend
    emit q->error(code);
}

void QNetworkReplyHttpImplPrivate::metaDataChanged()
{
    // FIXME merge this with replyDownloadMetaData(); ?

    Q_Q(QNetworkReplyHttpImpl);
    // 1. do we have cookies?
    // 2. are we allowed to set them?
    if (cookedHeaders.contains(QNetworkRequest::SetCookieHeader) && manager
        && (static_cast<QNetworkRequest::LoadControl>
            (request.attribute(QNetworkRequest::CookieSaveControlAttribute,
                               QNetworkRequest::Automatic).toInt()) == QNetworkRequest::Automatic)) {
        QList<QNetworkCookie> cookies =
            qvariant_cast<QList<QNetworkCookie> >(cookedHeaders.value(QNetworkRequest::SetCookieHeader));
        QNetworkCookieJar *jar = manager->cookieJar();
        if (jar)
            jar->setCookiesFromUrl(cookies, url);
    }
    emit q->metaDataChanged();
}

/*
    Migrates the backend of the QNetworkReply to a new network connection if required.  Returns
    true if the reply is migrated or it is not required; otherwise returns \c false.
*/
bool QNetworkReplyHttpImplPrivate::migrateBackend()
{
    Q_Q(QNetworkReplyHttpImpl);

    // Network reply is already finished or aborted, don't need to migrate.
    if (state == Finished || state == Aborted)
        return true;

    // Backend does not support resuming download.
    if (!canResume())
        return false;

    // Request has outgoing data, not migrating.
    if (outgoingData)
        return false;

    // Request is serviced from the cache, don't need to migrate.
    if (cacheLoadDevice)
        return true;

    state = Reconnecting;

    cookedHeaders.clear();
    rawHeaders.clear();

    preMigrationDownloaded = bytesDownloaded;

    setResumeOffset(bytesDownloaded);

    emit q->abortHttpRequest();

    QMetaObject::invokeMethod(q, "_q_startOperation", Qt::QueuedConnection);

    return true;
}


void QNetworkReplyHttpImplPrivate::createCache()
{
    // check if we can save and if we're allowed to
    if (!managerPrivate->networkCache
        || !request.attribute(QNetworkRequest::CacheSaveControlAttribute, true).toBool())
        return;
    cacheEnabled = true;
}

bool QNetworkReplyHttpImplPrivate::isCachingEnabled() const
{
    return (cacheEnabled && managerPrivate->networkCache != 0);
}

void QNetworkReplyHttpImplPrivate::setCachingEnabled(bool enable)
{
    if (!enable && !cacheEnabled)
        return;                 // nothing to do
    if (enable && cacheEnabled)
        return;                 // nothing to do either!

    if (enable) {
        if (bytesDownloaded) {
            qDebug() << "setCachingEnabled: " << bytesDownloaded << " bytesDownloaded";
            // refuse to enable in this case
            qCritical("QNetworkReplyImpl: backend error: caching was enabled after some bytes had been written");
            return;
        }

        createCache();
    } else {
        // someone told us to turn on, then back off?
        // ok... but you should make up your mind
        qDebug("QNetworkReplyImpl: setCachingEnabled(true) called after setCachingEnabled(false)");
        managerPrivate->networkCache->remove(url);
        cacheSaveDevice = 0;
        cacheEnabled = false;
    }
}

bool QNetworkReplyHttpImplPrivate::isCachingAllowed() const
{
    return operation == QNetworkAccessManager::GetOperation || operation == QNetworkAccessManager::HeadOperation;
}

void QNetworkReplyHttpImplPrivate::completeCacheSave()
{
    if (cacheEnabled && errorCode != QNetworkReplyImpl::NoError) {
        managerPrivate->networkCache->remove(url);
    } else if (cacheEnabled && cacheSaveDevice) {
        managerPrivate->networkCache->insert(cacheSaveDevice);
    }
    cacheSaveDevice = 0;
    cacheEnabled = false;
}

QT_END_NAMESPACE

#endif // QT_NO_HTTP
