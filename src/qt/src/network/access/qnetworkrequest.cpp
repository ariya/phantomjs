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

#include "qplatformdefs.h"
#include "qnetworkrequest.h"
#include "qnetworkcookie.h"
#include "qnetworkrequest_p.h"
#include "qsslconfiguration.h"
#include "QtCore/qshareddata.h"
#include "QtCore/qlocale.h"
#include "QtCore/qdatetime.h"

#include <ctype.h>
#ifndef QT_NO_DATESTRING
# include <stdio.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \class QNetworkRequest
    \brief The QNetworkRequest class holds a request to be sent with QNetworkAccessManager.
    \since 4.4

    \ingroup network
    \inmodule QtNetwork

    QNetworkRequest is part of the Network Access API and is the class
    holding the information necessary to send a request over the
    network. It contains a URL and some ancillary information that can
    be used to modify the request.

    \sa QNetworkReply, QNetworkAccessManager
*/

/*!
    \enum QNetworkRequest::KnownHeaders

    List of known header types that QNetworkRequest parses. Each known
    header is also represented in raw form with its full HTTP name.

    \value ContentTypeHeader    corresponds to the HTTP Content-Type
    header and contains a string containing the media (MIME) type and
    any auxiliary data (for instance, charset)

    \value ContentLengthHeader  corresponds to the HTTP Content-Length
    header and contains the length in bytes of the data transmitted.

    \value LocationHeader       corresponds to the HTTP Location
    header and contains a URL representing the actual location of the
    data, including the destination URL in case of redirections.

    \value LastModifiedHeader   corresponds to the HTTP Last-Modified
    header and contains a QDateTime representing the last modification
    date of the contents

    \value CookieHeader         corresponds to the HTTP Cookie header
    and contains a QList<QNetworkCookie> representing the cookies to
    be sent back to the server

    \value SetCookieHeader      corresponds to the HTTP Set-Cookie
    header and contains a QList<QNetworkCookie> representing the
    cookies sent by the server to be stored locally

    \sa header(), setHeader(), rawHeader(), setRawHeader()
*/

/*!
    \enum QNetworkRequest::Attribute
    \since 4.7
    
    Attribute codes for the QNetworkRequest and QNetworkReply.

    Attributes are extra meta-data that are used to control the
    behavior of the request and to pass further information from the
    reply back to the application. Attributes are also extensible,
    allowing custom implementations to pass custom values.

    The following table explains what the default attribute codes are,
    the QVariant types associated, the default value if said attribute
    is missing and whether it's used in requests or replies.

    \value HttpStatusCodeAttribute
        Replies only, type: QVariant::Int (no default)
        Indicates the HTTP status code received from the HTTP server
        (like 200, 304, 404, 401, etc.). If the connection was not
        HTTP-based, this attribute will not be present.

    \value HttpReasonPhraseAttribute
        Replies only, type: QVariant::ByteArray (no default)
        Indicates the HTTP reason phrase as received from the HTTP
        server (like "Ok", "Found", "Not Found", "Access Denied",
        etc.) This is the human-readable representation of the status
        code (see above). If the connection was not HTTP-based, this
        attribute will not be present.

    \value RedirectionTargetAttribute
        Replies only, type: QVariant::Url (no default)
        If present, it indicates that the server is redirecting the
        request to a different URL. The Network Access API does not by
        default follow redirections: it's up to the application to
        determine if the requested redirection should be allowed,
        according to its security policies.
        The returned URL might be relative. Use QUrl::resolved()
        to create an absolute URL out of it.

    \value ConnectionEncryptedAttribute
        Replies only, type: QVariant::Bool (default: false)
        Indicates whether the data was obtained through an encrypted
        (secure) connection.

    \value CacheLoadControlAttribute
        Requests only, type: QVariant::Int (default: QNetworkRequest::PreferNetwork)
        Controls how the cache should be accessed. The possible values
        are those of QNetworkRequest::CacheLoadControl. Note that the
        default QNetworkAccessManager implementation does not support
        caching. However, this attribute may be used by certain
        backends to modify their requests (for example, for caching proxies).

    \value CacheSaveControlAttribute
        Requests only, type: QVariant::Bool (default: true)
        Controls if the data obtained should be saved to cache for
        future uses. If the value is false, the data obtained will not
        be automatically cached. If true, data may be cached, provided
        it is cacheable (what is cacheable depends on the protocol
        being used).

    \value SourceIsFromCacheAttribute
        Replies only, type: QVariant::Bool (default: false)
        Indicates whether the data was obtained from cache
        or not.

    \value DoNotBufferUploadDataAttribute
        Requests only, type: QVariant::Bool (default: false)
        Indicates whether the QNetworkAccessManager code is
        allowed to buffer the upload data, e.g. when doing a HTTP POST.
        When using this flag with sequential upload data, the ContentLengthHeader
        header must be set.

    \value HttpPipeliningAllowedAttribute
        Requests only, type: QVariant::Bool (default: false)
        Indicates whether the QNetworkAccessManager code is
        allowed to use HTTP pipelining with this request.

    \value HttpPipeliningWasUsedAttribute
        Replies only, type: QVariant::Bool
        Indicates whether the HTTP pipelining was used for receiving
        this reply.

    \value CustomVerbAttribute
       Requests only, type: QVariant::ByteArray
       Holds the value for the custom HTTP verb to send (destined for usage
       of other verbs than GET, POST, PUT and DELETE). This verb is set
       when calling QNetworkAccessManager::sendCustomRequest().

    \value CookieLoadControlAttribute
        Requests only, type: QVariant::Int (default: QNetworkRequest::Automatic)
        Indicates whether to send 'Cookie' headers in the request.
        This attribute is set to false by QtWebKit when creating a cross-origin
        XMLHttpRequest where withCredentials has not been set explicitly to true by the
        Javascript that created the request.
        See \l{http://www.w3.org/TR/XMLHttpRequest2/#credentials-flag}{here} for more information.
        (This value was introduced in 4.7.)

    \value CookieSaveControlAttribute
        Requests only, type: QVariant::Int (default: QNetworkRequest::Automatic)
        Indicates whether to save 'Cookie' headers received from the server in reply
        to the request.
        This attribute is set to false by QtWebKit when creating a cross-origin
        XMLHttpRequest where withCredentials has not been set explicitly to true by the
        Javascript that created the request.
        See \l{http://www.w3.org/TR/XMLHttpRequest2/#credentials-flag} {here} for more information.
        (This value was introduced in 4.7.)

    \value AuthenticationReuseAttribute
        Requests only, type: QVariant::Int (default: QNetworkRequest::Automatic)
        Indicates whether to use cached authorization credentials in the request,
        if available. If this is set to QNetworkRequest::Manual and the authentication
        mechanism is 'Basic' or 'Digest', Qt will not send an an 'Authorization' HTTP
        header with any cached credentials it may have for the request's URL.
        This attribute is set to QNetworkRequest::Manual by QtWebKit when creating a cross-origin
        XMLHttpRequest where withCredentials has not been set explicitly to true by the
        Javascript that created the request.
        See \l{http://www.w3.org/TR/XMLHttpRequest2/#credentials-flag} {here} for more information.
        (This value was introduced in 4.7.)

    \omitvalue MaximumDownloadBufferSizeAttribute

    \omitvalue DownloadBufferAttribute

    \omitvalue SynchronousRequestAttribute

    \value User
        Special type. Additional information can be passed in
        QVariants with types ranging from User to UserMax. The default
        implementation of Network Access will ignore any request
        attributes in this range and it will not produce any
        attributes in this range in replies. The range is reserved for
        extensions of QNetworkAccessManager.

    \value UserMax
        Special type. See User.
*/

/*!
    \enum QNetworkRequest::CacheLoadControl

    Controls the caching mechanism of QNetworkAccessManager.

    \value AlwaysNetwork        always load from network and do not
    check if the cache has a valid entry (similar to the
    "Reload" feature in browsers)

    \value PreferNetwork        default value; load from the network
    if the cached entry is older than the network entry

    \value PreferCache          load from cache if available,
    otherwise load from network. Note that this can return possibly
    stale (but not expired) items from cache.

    \value AlwaysCache          only load from cache, indicating error
    if the item was not cached (i.e., off-line mode)
*/

/*!
    \enum QNetworkRequest::LoadControl
    \since 4.7

    Indicates if an aspect of the request's loading mechanism has been
    manually overridden, e.g. by QtWebKit.

    \value Automatic            default value: indicates default behaviour.

    \value Manual               indicates behaviour has been manually overridden.
*/

class QNetworkRequestPrivate: public QSharedData, public QNetworkHeadersPrivate
{
public:
    inline QNetworkRequestPrivate()
        : priority(QNetworkRequest::NormalPriority)
#ifndef QT_NO_OPENSSL
        , sslConfiguration(0)
#endif
    { qRegisterMetaType<QNetworkRequest>(); }
    ~QNetworkRequestPrivate()
    {
#ifndef QT_NO_OPENSSL
        delete sslConfiguration;
#endif
    }


    QNetworkRequestPrivate(const QNetworkRequestPrivate &other)
        : QSharedData(other), QNetworkHeadersPrivate(other)
    {
        url = other.url;
        priority = other.priority;

#ifndef QT_NO_OPENSSL
        sslConfiguration = 0;
        if (other.sslConfiguration)
            sslConfiguration = new QSslConfiguration(*other.sslConfiguration);
#endif
    }

    inline bool operator==(const QNetworkRequestPrivate &other) const
    {
        return url == other.url &&
            priority == other.priority &&
            rawHeaders == other.rawHeaders &&
            attributes == other.attributes;
        // don't compare cookedHeaders
    }

    QUrl url;
    QNetworkRequest::Priority priority;
#ifndef QT_NO_OPENSSL
    mutable QSslConfiguration *sslConfiguration;
#endif
};

/*!
    Constructs a QNetworkRequest object with \a url as the URL to be
    requested.

    \sa url(), setUrl()
*/
QNetworkRequest::QNetworkRequest(const QUrl &url)
    : d(new QNetworkRequestPrivate)
{
    d->url = url;
}

/*!
    Creates a copy of \a other.
*/
QNetworkRequest::QNetworkRequest(const QNetworkRequest &other)
    : d(other.d)
{
}

/*!
    Disposes of the QNetworkRequest object.
*/
QNetworkRequest::~QNetworkRequest()
{
    // QSharedDataPointer auto deletes
    d = 0;
}

/*!
    Returns true if this object is the same as \a other (i.e., if they
    have the same URL, same headers and same meta-data settings).

    \sa operator!=()
*/
bool QNetworkRequest::operator==(const QNetworkRequest &other) const
{
    return d == other.d || *d == *other.d;
}

/*!
    \fn bool QNetworkRequest::operator!=(const QNetworkRequest &other) const

    Returns false if this object is not the same as \a other.

    \sa operator==()
*/

/*!
    Creates a copy of \a other
*/
QNetworkRequest &QNetworkRequest::operator=(const QNetworkRequest &other)
{
    d = other.d;
    return *this;
}

/*!
    Returns the URL this network request is referring to.

    \sa setUrl()
*/
QUrl QNetworkRequest::url() const
{
    return d->url;
}

/*!
    Sets the URL this network request is referring to to be \a url.

    \sa url()
*/
void QNetworkRequest::setUrl(const QUrl &url)
{
    d->url = url;
}

/*!
    Returns the value of the known network header \a header if it is
    present in this request. If it is not present, returns QVariant()
    (i.e., an invalid variant).

    \sa KnownHeaders, rawHeader(), setHeader()
*/
QVariant QNetworkRequest::header(KnownHeaders header) const
{
    return d->cookedHeaders.value(header);
}

/*!
    Sets the value of the known header \a header to be \a value,
    overriding any previously set headers. This operation also sets
    the equivalent raw HTTP header.

    \sa KnownHeaders, setRawHeader(), header()
*/
void QNetworkRequest::setHeader(KnownHeaders header, const QVariant &value)
{
    d->setCookedHeader(header, value);
}

/*!
    Returns true if the raw header \a headerName is present in this
    network request.

    \sa rawHeader(), setRawHeader()
*/
bool QNetworkRequest::hasRawHeader(const QByteArray &headerName) const
{
    return d->findRawHeader(headerName) != d->rawHeaders.constEnd();
}

/*!
    Returns the raw form of header \a headerName. If no such header is
    present, an empty QByteArray is returned, which may be
    indistinguishable from a header that is present but has no content
    (use hasRawHeader() to find out if the header exists or not).

    Raw headers can be set with setRawHeader() or with setHeader().

    \sa header(), setRawHeader()
*/
QByteArray QNetworkRequest::rawHeader(const QByteArray &headerName) const
{
    QNetworkHeadersPrivate::RawHeadersList::ConstIterator it =
        d->findRawHeader(headerName);
    if (it != d->rawHeaders.constEnd())
        return it->second;
    return QByteArray();
}

/*!
    Returns a list of all raw headers that are set in this network
    request. The list is in the order that the headers were set.

    \sa hasRawHeader(), rawHeader()
*/
QList<QByteArray> QNetworkRequest::rawHeaderList() const
{
    return d->rawHeadersKeys();
}

/*!
    Sets the header \a headerName to be of value \a headerValue. If \a
    headerName corresponds to a known header (see
    QNetworkRequest::KnownHeaders), the raw format will be parsed and
    the corresponding "cooked" header will be set as well.

    For example:
    \snippet doc/src/snippets/code/src_network_access_qnetworkrequest.cpp 0

    will also set the known header LastModifiedHeader to be the
    QDateTime object of the parsed date.

    Note: setting the same header twice overrides the previous
    setting. To accomplish the behaviour of multiple HTTP headers of
    the same name, you should concatenate the two values, separating
    them with a comma (",") and set one single raw header.

    \sa KnownHeaders, setHeader(), hasRawHeader(), rawHeader()
*/
void QNetworkRequest::setRawHeader(const QByteArray &headerName, const QByteArray &headerValue)
{
    d->setRawHeader(headerName, headerValue);
}

/*!
    Returns the attribute associated with the code \a code. If the
    attribute has not been set, it returns \a defaultValue.

    Note: this function does not apply the defaults listed in
    QNetworkRequest::Attribute.

    \sa setAttribute(), QNetworkRequest::Attribute
*/
QVariant QNetworkRequest::attribute(Attribute code, const QVariant &defaultValue) const
{
    return d->attributes.value(code, defaultValue);
}

/*!
    Sets the attribute associated with code \a code to be value \a
    value. If the attribute is already set, the previous value is
    discarded. In special, if \a value is an invalid QVariant, the
    attribute is unset.

    \sa attribute(), QNetworkRequest::Attribute
*/
void QNetworkRequest::setAttribute(Attribute code, const QVariant &value)
{
    if (value.isValid())
        d->attributes.insert(code, value);
    else
        d->attributes.remove(code);
}

#ifndef QT_NO_OPENSSL
/*!
    Returns this network request's SSL configuration. By default, no
    SSL settings are specified.

    \sa setSslConfiguration()
*/
QSslConfiguration QNetworkRequest::sslConfiguration() const
{
    if (!d->sslConfiguration)
        d->sslConfiguration = new QSslConfiguration(QSslConfiguration::defaultConfiguration());
    return *d->sslConfiguration;
}

/*!
    Sets this network request's SSL configuration to be \a config. The
    settings that apply are the private key, the local certificate,
    the SSL protocol (SSLv2, SSLv3, TLSv1 where applicable), the CA
    certificates and the ciphers that the SSL backend is allowed to
    use.

    By default, no SSL configuration is set, which allows the backends
    to choose freely what configuration is best for them.

    \sa sslConfiguration(), QSslConfiguration::defaultConfiguration()
*/
void QNetworkRequest::setSslConfiguration(const QSslConfiguration &config)
{
    if (!d->sslConfiguration)
        d->sslConfiguration = new QSslConfiguration(config);
    else
        *d->sslConfiguration = config;
}
#endif

/*!
    \since 4.6

    Allows setting a reference to the \a object initiating
    the request.

    For example QtWebKit sets the originating object to the
    QWebFrame that initiated the request.

    \sa originatingObject()
*/
void QNetworkRequest::setOriginatingObject(QObject *object)
{
    d->originatingObject = object;
}

/*!
    \since 4.6

    Returns a reference to the object that initiated this
    network request; returns 0 if not set or the object has
    been destroyed.

    \sa setOriginatingObject()
*/
QObject *QNetworkRequest::originatingObject() const
{
    return d->originatingObject.data();
}

/*!
    \since 4.7

    Return the priority of this request.

    \sa setPriority()
*/
QNetworkRequest::Priority QNetworkRequest::priority() const
{
    return d->priority;
}

/*! \enum QNetworkRequest::Priority

  \since 4.7
  
  This enum lists the possible network request priorities.

  \value HighPriority   High priority
  \value NormalPriority Normal priority
  \value LowPriority    Low priority
 */

/*!
    \since 4.7

    Set the priority of this request to \a priority.

    \note The \a priority is only a hint to the network access
    manager.  It can use it or not. Currently it is used for HTTP to
    decide which request should be sent first to a server.

    \sa priority()
*/
void QNetworkRequest::setPriority(Priority priority)
{
    d->priority = priority;
}

static QByteArray headerName(QNetworkRequest::KnownHeaders header)
{
    switch (header) {
    case QNetworkRequest::ContentTypeHeader:
        return "Content-Type";

    case QNetworkRequest::ContentLengthHeader:
        return "Content-Length";

    case QNetworkRequest::LocationHeader:
        return "Location";

    case QNetworkRequest::LastModifiedHeader:
        return "Last-Modified";

    case QNetworkRequest::CookieHeader:
        return "Cookie";

    case QNetworkRequest::SetCookieHeader:
        return "Set-Cookie";

    case QNetworkRequest::ContentDispositionHeader:
        return "Content-Disposition";

    // no default:
    // if new values are added, this will generate a compiler warning
    }

    return QByteArray();
}

static QByteArray headerValue(QNetworkRequest::KnownHeaders header, const QVariant &value)
{
    switch (header) {
    case QNetworkRequest::ContentTypeHeader:
    case QNetworkRequest::ContentLengthHeader:
    case QNetworkRequest::ContentDispositionHeader:
        return value.toByteArray();

    case QNetworkRequest::LocationHeader:
        switch (value.type()) {
        case QVariant::Url:
            return value.toUrl().toEncoded();

        default:
            return value.toByteArray();
        }

    case QNetworkRequest::LastModifiedHeader:
        switch (value.type()) {
        case QVariant::Date:
        case QVariant::DateTime:
            // generate RFC 1123/822 dates:
            return QNetworkHeadersPrivate::toHttpDate(value.toDateTime());

        default:
            return value.toByteArray();
        }

    case QNetworkRequest::CookieHeader: {
        QList<QNetworkCookie> cookies = qvariant_cast<QList<QNetworkCookie> >(value);
        if (cookies.isEmpty() && value.userType() == qMetaTypeId<QNetworkCookie>())
            cookies << qvariant_cast<QNetworkCookie>(value);

        QByteArray result;
        bool first = true;
        foreach (const QNetworkCookie &cookie, cookies) {
            if (!first)
                result += "; ";
            first = false;
            result += cookie.toRawForm(QNetworkCookie::NameAndValueOnly);
        }
        return result;
    }

    case QNetworkRequest::SetCookieHeader: {
        QList<QNetworkCookie> cookies = qvariant_cast<QList<QNetworkCookie> >(value);
        if (cookies.isEmpty() && value.userType() == qMetaTypeId<QNetworkCookie>())
            cookies << qvariant_cast<QNetworkCookie>(value);

        QByteArray result;
        bool first = true;
        foreach (const QNetworkCookie &cookie, cookies) {
            if (!first)
                result += ", ";
            first = false;
            result += cookie.toRawForm(QNetworkCookie::Full);
        }
        return result;
    }
    }

    return QByteArray();
}

static QNetworkRequest::KnownHeaders parseHeaderName(const QByteArray &headerName)
{
    // headerName is not empty here

    switch (tolower(headerName.at(0))) {
    case 'c':
        if (qstricmp(headerName.constData(), "content-type") == 0)
            return QNetworkRequest::ContentTypeHeader;
        else if (qstricmp(headerName.constData(), "content-length") == 0)
            return QNetworkRequest::ContentLengthHeader;
        else if (qstricmp(headerName.constData(), "cookie") == 0)
            return QNetworkRequest::CookieHeader;
        break;

    case 'l':
        if (qstricmp(headerName.constData(), "location") == 0)
            return QNetworkRequest::LocationHeader;
        else if (qstricmp(headerName.constData(), "last-modified") == 0)
            return QNetworkRequest::LastModifiedHeader;
        break;

    case 's':
        if (qstricmp(headerName.constData(), "set-cookie") == 0)
            return QNetworkRequest::SetCookieHeader;
        break;
    }

    return QNetworkRequest::KnownHeaders(-1); // nothing found
}

static QVariant parseHttpDate(const QByteArray &raw)
{
    QDateTime dt = QNetworkHeadersPrivate::fromHttpDate(raw);
    if (dt.isValid())
        return dt;
    return QVariant();          // transform an invalid QDateTime into a null QVariant
}

static QVariant parseCookieHeader(const QByteArray &raw)
{
    QList<QNetworkCookie> result;
    QList<QByteArray> cookieList = raw.split(';');
    foreach (const QByteArray &cookie, cookieList) {
        QList<QNetworkCookie> parsed = QNetworkCookie::parseCookies(cookie.trimmed());
        if (parsed.count() != 1)
            return QVariant();  // invalid Cookie: header

        result += parsed;
    }

    return QVariant::fromValue(result);
}

static QVariant parseHeaderValue(QNetworkRequest::KnownHeaders header, const QByteArray &value)
{
    // header is always a valid value
    switch (header) {
    case QNetworkRequest::ContentTypeHeader:
        // copy exactly, convert to QString
        return QString::fromLatin1(value);

    case QNetworkRequest::ContentLengthHeader: {
        bool ok;
        qint64 result = value.trimmed().toLongLong(&ok);
        if (ok)
            return result;
        return QVariant();
    }

    case QNetworkRequest::LocationHeader: {
        QUrl result = QUrl::fromEncoded(value, QUrl::StrictMode);
        if (result.isValid() && !result.scheme().isEmpty())
            return result;
        return QVariant();
    }

    case QNetworkRequest::LastModifiedHeader:
        return parseHttpDate(value);

    case QNetworkRequest::CookieHeader:
        return parseCookieHeader(value);

    case QNetworkRequest::SetCookieHeader:
        return QVariant::fromValue(QNetworkCookie::parseCookies(value));

    default:
        Q_ASSERT(0);
    }
    return QVariant();
}

QNetworkHeadersPrivate::RawHeadersList::ConstIterator
QNetworkHeadersPrivate::findRawHeader(const QByteArray &key) const
{
    RawHeadersList::ConstIterator it = rawHeaders.constBegin();
    RawHeadersList::ConstIterator end = rawHeaders.constEnd();
    for ( ; it != end; ++it)
        if (qstricmp(it->first.constData(), key.constData()) == 0)
            return it;

    return end;                 // not found
}

QNetworkHeadersPrivate::RawHeadersList QNetworkHeadersPrivate::allRawHeaders() const
{
    return rawHeaders;
}

QList<QByteArray> QNetworkHeadersPrivate::rawHeadersKeys() const
{
    QList<QByteArray> result;
    RawHeadersList::ConstIterator it = rawHeaders.constBegin(),
                                 end = rawHeaders.constEnd();
    for ( ; it != end; ++it)
        result << it->first;

    return result;
}

void QNetworkHeadersPrivate::setRawHeader(const QByteArray &key, const QByteArray &value)
{
    if (key.isEmpty())
        // refuse to accept an empty raw header
        return;

    setRawHeaderInternal(key, value);
    parseAndSetHeader(key, value);
}

/*!
    \internal
    Sets the internal raw headers list to match \a list. The cooked headers
    will also be updated.

    If \a list contains duplicates, they will be stored, but only the first one
    is usually accessed.
*/
void QNetworkHeadersPrivate::setAllRawHeaders(const RawHeadersList &list)
{
    cookedHeaders.clear();
    rawHeaders = list;

    RawHeadersList::ConstIterator it = rawHeaders.constBegin();
    RawHeadersList::ConstIterator end = rawHeaders.constEnd();
    for ( ; it != end; ++it)
        parseAndSetHeader(it->first, it->second);
}

void QNetworkHeadersPrivate::setCookedHeader(QNetworkRequest::KnownHeaders header,
                                             const QVariant &value)
{
    QByteArray name = headerName(header);
    if (name.isEmpty()) {
        // headerName verifies that \a header is a known value
        qWarning("QNetworkRequest::setHeader: invalid header value KnownHeader(%d) received", header);
        return;
    }

    if (value.isNull()) {
        setRawHeaderInternal(name, QByteArray());
        cookedHeaders.remove(header);
    } else {
        QByteArray rawValue = headerValue(header, value);
        if (rawValue.isEmpty()) {
            qWarning("QNetworkRequest::setHeader: QVariant of type %s cannot be used with header %s",
                     value.typeName(), name.constData());
            return;
        }

        setRawHeaderInternal(name, rawValue);
        cookedHeaders.insert(header, value);
    }
}

void QNetworkHeadersPrivate::setRawHeaderInternal(const QByteArray &key, const QByteArray &value)
{
    RawHeadersList::Iterator it = rawHeaders.begin();
    while (it != rawHeaders.end()) {
        if (qstricmp(it->first.constData(), key.constData()) == 0)
            it = rawHeaders.erase(it);
        else
            ++it;
    }

    if (value.isNull())
        return;                 // only wanted to erase key

    RawHeaderPair pair;
    pair.first = key;
    pair.second = value;
    rawHeaders.append(pair);
}

void QNetworkHeadersPrivate::parseAndSetHeader(const QByteArray &key, const QByteArray &value)
{
    // is it a known header?
    QNetworkRequest::KnownHeaders parsedKey = parseHeaderName(key);
    if (parsedKey != QNetworkRequest::KnownHeaders(-1)) {
        if (value.isNull()) {
            cookedHeaders.remove(parsedKey);
        } else if (parsedKey == QNetworkRequest::ContentLengthHeader
                 && cookedHeaders.contains(QNetworkRequest::ContentLengthHeader)) {
            // Only set the cooked header "Content-Length" once.
            // See bug QTBUG-15311
        } else {
            cookedHeaders.insert(parsedKey, parseHeaderValue(parsedKey, value));
        }

    }
}

// Fast month string to int conversion. This code
// assumes that the Month name is correct and that
// the string is at least three chars long.
static int name_to_month(const char* month_str)
{
    switch (month_str[0]) {
    case 'J':
        switch (month_str[1]) {
        case 'a':
            return 1;
            break;
        case 'u':
            switch (month_str[2] ) {
            case 'n':
                return 6;
                break;
            case 'l':
                return 7;
                break;
            }
        }
        break;
    case 'F':
        return 2;
        break;
    case 'M':
        switch (month_str[2] ) {
        case 'r':
            return 3;
            break;
        case 'y':
            return 5;
            break;
        }
        break;
    case 'A':
        switch (month_str[1]) {
        case 'p':
            return 4;
            break;
        case 'u':
            return 8;
            break;
        }
        break;
    case 'O':
        return 10;
        break;
    case 'S':
        return 9;
        break;
    case 'N':
        return 11;
        break;
    case 'D':
        return 12;
        break;
    }

    return 0;
}

QDateTime QNetworkHeadersPrivate::fromHttpDate(const QByteArray &value)
{
    // HTTP dates have three possible formats:
    //  RFC 1123/822      -   ddd, dd MMM yyyy hh:mm:ss "GMT"
    //  RFC 850           -   dddd, dd-MMM-yy hh:mm:ss "GMT"
    //  ANSI C's asctime  -   ddd MMM d hh:mm:ss yyyy
    // We only handle them exactly. If they deviate, we bail out.

    int pos = value.indexOf(',');
    QDateTime dt;
#ifndef QT_NO_DATESTRING
    if (pos == -1) {
        // no comma -> asctime(3) format
        dt = QDateTime::fromString(QString::fromLatin1(value), Qt::TextDate);
    } else {
        // Use sscanf over QLocal/QDateTimeParser for speed reasons. See the
        // QtWebKit performance benchmarks to get an idea.
        if (pos == 3) {
            char month_name[4];
            int day, year, hour, minute, second;
            if (sscanf(value.constData(), "%*3s, %d %3s %d %d:%d:%d 'GMT'", &day, month_name, &year, &hour, &minute, &second) == 6)
                dt = QDateTime(QDate(year, name_to_month(month_name), day), QTime(hour, minute, second));
        } else {
            QLocale c = QLocale::c();
            // eat the weekday, the comma and the space following it
            QString sansWeekday = QString::fromLatin1(value.constData() + pos + 2);
            // must be RFC 850 date
            dt = c.toDateTime(sansWeekday, QLatin1String("dd-MMM-yy hh:mm:ss 'GMT'"));
        }
    }
#endif // QT_NO_DATESTRING

    if (dt.isValid())
        dt.setTimeSpec(Qt::UTC);
    return dt;
}

QByteArray QNetworkHeadersPrivate::toHttpDate(const QDateTime &dt)
{
    return QLocale::c().toString(dt, QLatin1String("ddd, dd MMM yyyy hh:mm:ss 'GMT'"))
        .toLatin1();
}

QT_END_NAMESPACE
