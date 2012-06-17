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

#include "qnetworkaccessmanager.h"
#include "qnetworkaccessmanager_p.h"
#include "qnetworkrequest.h"
#include "qnetworkreply.h"
#include "qnetworkreply_p.h"
#include "qnetworkcookie.h"
#include "qabstractnetworkcache.h"

#include "QtNetwork/qnetworksession.h"
#include "QtNetwork/private/qsharednetworksession_p.h"

#include "qnetworkaccesshttpbackend_p.h"
#include "qnetworkaccessftpbackend_p.h"
#include "qnetworkaccessfilebackend_p.h"
#include "qnetworkaccessdebugpipebackend_p.h"
#include "qnetworkaccesscachebackend_p.h"
#include "qnetworkreplydataimpl_p.h"
#include "qnetworkreplyfileimpl_p.h"

#include "QtCore/qbuffer.h"
#include "QtCore/qurl.h"
#include "QtCore/qvector.h"
#include "QtNetwork/private/qauthenticator_p.h"
#include "QtNetwork/qsslconfiguration.h"
#include "QtNetwork/qnetworkconfigmanager.h"
#include "QtNetwork/qhttpmultipart.h"
#include "qhttpmultipart_p.h"

#include "qthread.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_HTTP
Q_GLOBAL_STATIC(QNetworkAccessHttpBackendFactory, httpBackend)
#endif // QT_NO_HTTP
Q_GLOBAL_STATIC(QNetworkAccessFileBackendFactory, fileBackend)
#ifndef QT_NO_FTP
Q_GLOBAL_STATIC(QNetworkAccessFtpBackendFactory, ftpBackend)
#endif // QT_NO_FTP

#ifdef QT_BUILD_INTERNAL
Q_GLOBAL_STATIC(QNetworkAccessDebugPipeBackendFactory, debugpipeBackend)
#endif

static void ensureInitialized()
{
#ifndef QT_NO_HTTP
    (void) httpBackend();
#endif // QT_NO_HTTP

#ifndef QT_NO_FTP
    (void) ftpBackend();
#endif

#ifdef QT_BUILD_INTERNAL
    (void) debugpipeBackend();
#endif

    // leave this one last since it will query the special QAbstractFileEngines
    (void) fileBackend();
}

/*!
    \class QNetworkAccessManager
    \brief The QNetworkAccessManager class allows the application to
    send network requests and receive replies
    \since 4.4

    \ingroup network
    \inmodule QtNetwork
    \reentrant

    The Network Access API is constructed around one QNetworkAccessManager
    object, which holds the common configuration and settings for the requests
    it sends. It contains the proxy and cache configuration, as well as the
    signals related to such issues, and reply signals that can be used to
    monitor the progress of a network operation. One QNetworkAccessManager
    should be enough for the whole Qt application.

    Once a QNetworkAccessManager object has been created, the application can
    use it to send requests over the network. A group of standard functions
    are supplied that take a request and optional data, and each return a
    QNetworkReply object. The returned object is used to obtain any data
    returned in response to the corresponding request.

    A simple download off the network could be accomplished with:
    \snippet doc/src/snippets/code/src_network_access_qnetworkaccessmanager.cpp 0

    QNetworkAccessManager has an asynchronous API.
    When the \tt replyFinished slot above is called, the parameter it
    takes is the QNetworkReply object containing the downloaded data
    as well as meta-data (headers, etc.).

    \note After the request has finished, it is the responsibility of the user
    to delete the QNetworkReply object at an appropriate time. Do not directly
    delete it inside the slot connected to finished(). You can use the
    deleteLater() function.

    \note QNetworkAccessManager queues the requests it receives. The number
    of requests executed in parallel is dependent on the protocol.
    Currently, for the HTTP protocol on desktop platforms, 6 requests are
    executed in parallel for one host/port combination.

    A more involved example, assuming the manager is already existent,
    can be:
    \snippet doc/src/snippets/code/src_network_access_qnetworkaccessmanager.cpp 1

    \section1 Network and Roaming support

    With the addition of the \l {Bearer Management} API to Qt 4.7
    QNetworkAccessManager gained the ability to manage network connections.
    QNetworkAccessManager can start the network interface if the device is
    offline and terminates the interface if the current process is the last
    one to use the uplink. Note that some platform utilize grace periods from
    when the last application stops using a uplink until the system actually
    terminates the connectivity link. Roaming is equally transparent. Any
    queued/pending network requests are automatically transferred to new
    access point.

    Clients wanting to utilize this feature should not require any changes. In fact
    it is likely that existing platform specific connection code can simply be
    removed from the application.

    \note The network and roaming support in QNetworkAccessManager is conditional
    upon the platform supporting connection management. The
    \l QNetworkConfigurationManager::NetworkSessionRequired can be used to
    detect whether QNetworkAccessManager utilizes this feature. Currently only
    Meego/Harmattan and Symbian platforms provide connection management support.

    \note This feature cannot be used in combination with the Bearer Management
    API as provided by QtMobility. Applications have to migrate to the Qt version
    of Bearer Management.

    \section1 Symbian Platform Security Requirements

    On Symbian, processes which use this class must have the
    \c NetworkServices platform security capability. If the client
    process lacks this capability, operations will result in a panic.

    Platform security capabilities are added via the
    \l{qmake-variable-reference.html#target-capability}{TARGET.CAPABILITY}
    qmake variable.

    \sa QNetworkRequest, QNetworkReply, QNetworkProxy
*/

/*!
    \enum QNetworkAccessManager::Operation

    Indicates the operation this reply is processing.

    \value HeadOperation        retrieve headers operation (created
    with head())

    \value GetOperation         retrieve headers and download contents
    (created with get())

    \value PutOperation         upload contents operation (created
    with put())

    \value PostOperation        send the contents of an HTML form for
    processing via HTTP POST (created with post())

    \value DeleteOperation      delete contents operation (created with
    deleteResource())

    \value CustomOperation      custom operation (created with
    sendCustomRequest())    \since 4.7

    \omitvalue UnknownOperation

    \sa QNetworkReply::operation()
*/

/*!
    \enum QNetworkAccessManager::NetworkAccessibility

    Indicates whether the network is accessible via this network access manager.

    \value UnknownAccessibility     The network accessibility cannot be determined.
    \value NotAccessible            The network is not currently accessible, either because there
                                    is currently no network coverage or network access has been
                                    explicitly disabled by a call to setNetworkAccessible().
    \value Accessible               The network is accessible.

    \sa networkAccessible
*/

/*!
    \property QNetworkAccessManager::networkAccessible
    \brief whether the network is currently accessible via this network access manager.

    \since 4.7

    If the network is \l {NotAccessible}{not accessible} the network access manager will not
    process any new network requests, all such requests will fail with an error.  Requests with
    URLs with the file:// scheme will still be processed.

    By default the value of this property reflects the physical state of the device.  Applications
    may override it to disable all network requests via this network access manager by calling

    \snippet doc/src/snippets/code/src_network_access_qnetworkaccessmanager.cpp 4

    Network requests can be reenabled again by calling

    \snippet doc/src/snippets/code/src_network_access_qnetworkaccessmanager.cpp 5

    \note Calling setNetworkAccessible() does not change the network state.
*/

/*!
    \fn void QNetworkAccessManager::networkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility accessible)

    This signal is emitted when the value of the \l networkAccessible property changes.
    \a accessible is the new network accessibility.
*/

/*!
    \fn void QNetworkAccessManager::networkSessionConnected()

    \since 4.7

    \internal

    This signal is emitted when the status of the network session changes into a usable (Connected)
    state. It is used to signal to QNetworkReplys to start or migrate their network operation once
    the network session has been opened or finished roaming.
*/

/*!
    \fn void QNetworkAccessManager::proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator)

    This signal is emitted whenever a proxy requests authentication
    and QNetworkAccessManager cannot find a valid, cached
    credential. The slot connected to this signal should fill in the
    credentials for the proxy \a proxy in the \a authenticator object.

    QNetworkAccessManager will cache the credentials internally. The
    next time the proxy requests authentication, QNetworkAccessManager
    will automatically send the same credential without emitting the
    proxyAuthenticationRequired signal again.

    If the proxy rejects the credentials, QNetworkAccessManager will
    emit the signal again.

    \sa proxy(), setProxy(), authenticationRequired()
*/

/*!
    \fn void QNetworkAccessManager::authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)

    This signal is emitted whenever a final server requests
    authentication before it delivers the requested contents. The slot
    connected to this signal should fill the credentials for the
    contents (which can be determined by inspecting the \a reply
    object) in the \a authenticator object.

    QNetworkAccessManager will cache the credentials internally and
    will send the same values if the server requires authentication
    again, without emitting the authenticationRequired() signal. If it
    rejects the credentials, this signal will be emitted again.

    \note It is not possible to use a QueuedConnection to connect to
    this signal, as the connection will fail if the authenticator has
    not been filled in with new information when the signal returns.

    \sa proxyAuthenticationRequired()
*/

/*!
    \fn void QNetworkAccessManager::finished(QNetworkReply *reply)

    This signal is emitted whenever a pending network reply is
    finished. The \a reply parameter will contain a pointer to the
    reply that has just finished. This signal is emitted in tandem
    with the QNetworkReply::finished() signal.

    See QNetworkReply::finished() for information on the status that
    the object will be in.

    \note Do not delete the \a reply object in the slot connected to this
    signal. Use deleteLater().

    \sa QNetworkReply::finished(), QNetworkReply::error()
*/

/*!
    \fn void QNetworkAccessManager::sslErrors(QNetworkReply *reply, const QList<QSslError> &errors)

    This signal is emitted if the SSL/TLS session encountered errors
    during the set up, including certificate verification errors. The
    \a errors parameter contains the list of errors and \a reply is
    the QNetworkReply that is encountering these errors.

    To indicate that the errors are not fatal and that the connection
    should proceed, the QNetworkReply::ignoreSslErrors() function should be called
    from the slot connected to this signal. If it is not called, the
    SSL session will be torn down before any data is exchanged
    (including the URL).

    This signal can be used to display an error message to the user
    indicating that security may be compromised and display the
    SSL settings (see sslConfiguration() to obtain it). If the user
    decides to proceed after analyzing the remote certificate, the
    slot should call ignoreSslErrors().

    \sa QSslSocket::sslErrors(), QNetworkReply::sslErrors(),
    QNetworkReply::sslConfiguration(), QNetworkReply::ignoreSslErrors()
*/


/*!
    Constructs a QNetworkAccessManager object that is the center of
    the Network Access API and sets \a parent as the parent object.
*/
QNetworkAccessManager::QNetworkAccessManager(QObject *parent)
    : QObject(*new QNetworkAccessManagerPrivate, parent)
{
    ensureInitialized();

    qRegisterMetaType<QNetworkReply::NetworkError>("QNetworkReply::NetworkError");
}

/*!
    Destroys the QNetworkAccessManager object and frees up any
    resources. Note that QNetworkReply objects that are returned from
    this class have this object set as their parents, which means that
    they will be deleted along with it if you don't call
    QObject::setParent() on them.
*/
QNetworkAccessManager::~QNetworkAccessManager()
{
#ifndef QT_NO_NETWORKPROXY
    delete d_func()->proxyFactory;
#endif

    // Delete the QNetworkReply children first.
    // Else a QAbstractNetworkCache might get deleted in ~QObject
    // before a QNetworkReply that accesses the QAbstractNetworkCache
    // object in its destructor.
    qDeleteAll(findChildren<QNetworkReply *>());
    // The other children will be deleted in this ~QObject
    // FIXME instead of this "hack" make the QNetworkReplyImpl
    // properly watch the cache deletion, e.g. via a QWeakPointer.
}

#ifndef QT_NO_NETWORKPROXY
/*!
    Returns the QNetworkProxy that the requests sent using this
    QNetworkAccessManager object will use. The default value for the
    proxy is QNetworkProxy::DefaultProxy.

    \sa setProxy(), setProxyFactory(), proxyAuthenticationRequired()
*/
QNetworkProxy QNetworkAccessManager::proxy() const
{
    return d_func()->proxy;
}

/*!
    Sets the proxy to be used in future requests to be \a proxy. This
    does not affect requests that have already been sent. The
    proxyAuthenticationRequired() signal will be emitted if the proxy
    requests authentication.

    A proxy set with this function will be used for all requests
    issued by QNetworkAccessManager. In some cases, it might be
    necessary to select different proxies depending on the type of
    request being sent or the destination host. If that's the case,
    you should consider using setProxyFactory().

    \sa proxy(), proxyAuthenticationRequired()
*/
void QNetworkAccessManager::setProxy(const QNetworkProxy &proxy)
{
    Q_D(QNetworkAccessManager);
    delete d->proxyFactory;
    d->proxy = proxy;
    d->proxyFactory = 0;
}

/*!
    \fn QNetworkProxyFactory *QNetworkAccessManager::proxyFactory() const
    \since 4.5

    Returns the proxy factory that this QNetworkAccessManager object
    is using to determine the proxies to be used for requests.

    Note that the pointer returned by this function is managed by
    QNetworkAccessManager and could be deleted at any time.

    \sa setProxyFactory(), proxy()
*/
QNetworkProxyFactory *QNetworkAccessManager::proxyFactory() const
{
    return d_func()->proxyFactory;
}

/*!
    \since 4.5

    Sets the proxy factory for this class to be \a factory. A proxy
    factory is used to determine a more specific list of proxies to be
    used for a given request, instead of trying to use the same proxy
    value for all requests.

    All queries sent by QNetworkAccessManager will have type
    QNetworkProxyQuery::UrlRequest.

    For example, a proxy factory could apply the following rules:
    \list
      \o if the target address is in the local network (for example,
         if the hostname contains no dots or if it's an IP address in
         the organization's range), return QNetworkProxy::NoProxy
      \o if the request is FTP, return an FTP proxy
      \o if the request is HTTP or HTTPS, then return an HTTP proxy
      \o otherwise, return a SOCKSv5 proxy server
    \endlist

    The lifetime of the object \a factory will be managed by
    QNetworkAccessManager. It will delete the object when necessary.

    \note If a specific proxy is set with setProxy(), the factory will not
    be used.

    \sa proxyFactory(), setProxy(), QNetworkProxyQuery
*/
void QNetworkAccessManager::setProxyFactory(QNetworkProxyFactory *factory)
{
    Q_D(QNetworkAccessManager);
    delete d->proxyFactory;
    d->proxyFactory = factory;
    d->proxy = QNetworkProxy();
}
#endif

/*!
    \since 4.5

    Returns the cache that is used to store data obtained from the network.

    \sa setCache()
*/
QAbstractNetworkCache *QNetworkAccessManager::cache() const
{
    Q_D(const QNetworkAccessManager);
    return d->networkCache;
}

/*!
    \since 4.5

    Sets the manager's network cache to be the \a cache specified. The cache
    is used for all requests dispatched by the manager.

    Use this function to set the network cache object to a class that implements
    additional features, like saving the cookies to permanent storage.

    \note QNetworkAccessManager takes ownership of the \a cache object.

    QNetworkAccessManager by default does not have a set cache.
    Qt provides a simple disk cache, QNetworkDiskCache, which can be used.

    \sa cache(), QNetworkRequest::CacheLoadControl
*/
void QNetworkAccessManager::setCache(QAbstractNetworkCache *cache)
{
    Q_D(QNetworkAccessManager);
    if (d->networkCache != cache) {
        delete d->networkCache;
        d->networkCache = cache;
        if (d->networkCache)
            d->networkCache->setParent(this);
    }
}

/*!
    Returns the QNetworkCookieJar that is used to store cookies
    obtained from the network as well as cookies that are about to be
    sent.

    \sa setCookieJar()
*/
QNetworkCookieJar *QNetworkAccessManager::cookieJar() const
{
    Q_D(const QNetworkAccessManager);
    if (!d->cookieJar)
        d->createCookieJar();
    return d->cookieJar;
}

/*!
    Sets the manager's cookie jar to be the \a cookieJar specified.
    The cookie jar is used by all requests dispatched by the manager.

    Use this function to set the cookie jar object to a class that
    implements additional features, like saving the cookies to permanent
    storage.

    \note QNetworkAccessManager takes ownership of the \a cookieJar object.

    If \a cookieJar is in the same thread as this QNetworkAccessManager,
    it will set the parent of the \a cookieJar
    so that the cookie jar is deleted when this
    object is deleted as well. If you want to share cookie jars
    between different QNetworkAccessManager objects, you may want to
    set the cookie jar's parent to 0 after calling this function.

    QNetworkAccessManager by default does not implement any cookie
    policy of its own: it accepts all cookies sent by the server, as
    long as they are well formed and meet the minimum security
    requirements (cookie domain matches the request's and cookie path
    matches the request's). In order to implement your own security
    policy, override the QNetworkCookieJar::cookiesForUrl() and
    QNetworkCookieJar::setCookiesFromUrl() virtual functions. Those
    functions are called by QNetworkAccessManager when it detects a
    new cookie.

    \sa cookieJar(), QNetworkCookieJar::cookiesForUrl(), QNetworkCookieJar::setCookiesFromUrl()
*/
void QNetworkAccessManager::setCookieJar(QNetworkCookieJar *cookieJar)
{
    Q_D(QNetworkAccessManager);
    d->cookieJarCreated = true;
    if (d->cookieJar != cookieJar) {
        if (d->cookieJar && d->cookieJar->parent() == this)
            delete d->cookieJar;
        d->cookieJar = cookieJar;
        if (thread() == cookieJar->thread())
            d->cookieJar->setParent(this);
    }
}

/*!
    Posts a request to obtain the network headers for \a request
    and returns a new QNetworkReply object which will contain such headers.

    The function is named after the HTTP request associated (HEAD).
*/
QNetworkReply *QNetworkAccessManager::head(const QNetworkRequest &request)
{
    return d_func()->postProcess(createRequest(QNetworkAccessManager::HeadOperation, request));
}

/*!
    Posts a request to obtain the contents of the target \a request
    and returns a new QNetworkReply object opened for reading which emits the 
    \l{QIODevice::readyRead()}{readyRead()} signal whenever new data 
    arrives.

    The contents as well as associated headers will be downloaded.

    \sa post(), put(), deleteResource(), sendCustomRequest()
*/
QNetworkReply *QNetworkAccessManager::get(const QNetworkRequest &request)
{
    return d_func()->postProcess(createRequest(QNetworkAccessManager::GetOperation, request));
}

/*!
    Sends an HTTP POST request to the destination specified by \a request
    and returns a new QNetworkReply object opened for reading that will 
    contain the reply sent by the server. The contents of  the \a data 
    device will be uploaded to the server.

    \a data must be open for reading and must remain valid until the 
    finished() signal is emitted for this reply.

    \note Sending a POST request on protocols other than HTTP and
    HTTPS is undefined and will probably fail.

    \sa get(), put(), deleteResource(), sendCustomRequest()
*/
QNetworkReply *QNetworkAccessManager::post(const QNetworkRequest &request, QIODevice *data)
{
    return d_func()->postProcess(createRequest(QNetworkAccessManager::PostOperation, request, data));
}

/*!
    \overload

    Sends the contents of the \a data byte array to the destination 
    specified by \a request.
*/
QNetworkReply *QNetworkAccessManager::post(const QNetworkRequest &request, const QByteArray &data)
{
    QBuffer *buffer = new QBuffer;
    buffer->setData(data);
    buffer->open(QIODevice::ReadOnly);

    QNetworkReply *reply = post(request, buffer);
    buffer->setParent(reply);
    return reply;
}

/*!
    \since 4.8

    \overload

    Sends the contents of the \a multiPart message to the destination
    specified by \a request.

    This can be used for sending MIME multipart messages over HTTP.

    \sa QHttpMultiPart, QHttpPart, put()
*/
QNetworkReply *QNetworkAccessManager::post(const QNetworkRequest &request, QHttpMultiPart *multiPart)
{
    QNetworkRequest newRequest = d_func()->prepareMultipart(request, multiPart);
    QIODevice *device = multiPart->d_func()->device;
    QNetworkReply *reply = post(newRequest, device);
    return reply;
}

/*!
    \since 4.8

    \overload

    Sends the contents of the \a multiPart message to the destination
    specified by \a request.

    This can be used for sending MIME multipart messages over HTTP.

    \sa QHttpMultiPart, QHttpPart, post()
*/
QNetworkReply *QNetworkAccessManager::put(const QNetworkRequest &request, QHttpMultiPart *multiPart)
{
    QNetworkRequest newRequest = d_func()->prepareMultipart(request, multiPart);
    QIODevice *device = multiPart->d_func()->device;
    QNetworkReply *reply = put(newRequest, device);
    return reply;
}

/*!
    Uploads the contents of \a data to the destination \a request and
    returnes a new QNetworkReply object that will be open for reply.

    \a data must be opened for reading when this function is called
    and must remain valid until the finished() signal is emitted for
    this reply.

    Whether anything will be available for reading from the returned
    object is protocol dependent. For HTTP, the server may send a 
    small HTML page indicating the upload was successful (or not). 
    Other protocols will probably have content in their replies.

    \note For HTTP, this request will send a PUT request, which most servers
    do not allow. Form upload mechanisms, including that of uploading
    files through HTML forms, use the POST mechanism.

    \sa get(), post(), deleteResource(), sendCustomRequest()
*/
QNetworkReply *QNetworkAccessManager::put(const QNetworkRequest &request, QIODevice *data)
{
    return d_func()->postProcess(createRequest(QNetworkAccessManager::PutOperation, request, data));
}

/*!
    \overload

    Sends the contents of the \a data byte array to the destination
    specified by \a request.
*/
QNetworkReply *QNetworkAccessManager::put(const QNetworkRequest &request, const QByteArray &data)
{
    QBuffer *buffer = new QBuffer;
    buffer->setData(data);
    buffer->open(QIODevice::ReadOnly);

    QNetworkReply *reply = put(request, buffer);
    buffer->setParent(reply);
    return reply;
}

/*!
    \since 4.6

    Sends a request to delete the resource identified by the URL of \a request.

    \note This feature is currently available for HTTP only, performing an 
    HTTP DELETE request.

    \sa get(), post(), put(), sendCustomRequest()
*/
QNetworkReply *QNetworkAccessManager::deleteResource(const QNetworkRequest &request)
{
    return d_func()->postProcess(createRequest(QNetworkAccessManager::DeleteOperation, request));
}

#ifndef QT_NO_BEARERMANAGEMENT

/*!
    \since 4.7

    Sets the network configuration that will be used when creating the
    \l {QNetworkSession}{network session} to \a config.

    The network configuration is used to create and open a network session before any request that
    requires network access is process.  If no network configuration is explicitly set via this
    function the network configuration returned by
    QNetworkConfigurationManager::defaultConfiguration() will be used.

    To restore the default network configuration set the network configuration to the value
    returned from QNetworkConfigurationManager::defaultConfiguration().

    \snippet doc/src/snippets/code/src_network_access_qnetworkaccessmanager.cpp 2

    If an invalid network configuration is set, a network session will not be created.  In this
    case network requests will be processed regardless, but may fail.  For example:

    \snippet doc/src/snippets/code/src_network_access_qnetworkaccessmanager.cpp 3

    \sa configuration(), QNetworkSession
*/
void QNetworkAccessManager::setConfiguration(const QNetworkConfiguration &config)
{
    d_func()->createSession(config);
}

/*!
    \since 4.7

    Returns the network configuration that will be used to create the
    \l {QNetworkSession}{network session} which will be used when processing network requests.

    \sa setConfiguration(), activeConfiguration()
*/
QNetworkConfiguration QNetworkAccessManager::configuration() const
{
    Q_D(const QNetworkAccessManager);

    if (d->networkSession)
        return d->networkSession->configuration();
    else
        return QNetworkConfiguration();
}

/*!
    \since 4.7

    Returns the current active network configuration.

    If the network configuration returned by configuration() is of type
    QNetworkConfiguration::ServiceNetwork this function will return the current active child
    network configuration of that configuration.  Otherwise returns the same network configuration
    as configuration().

    Use this function to return the actual network configuration currently in use by the network
    session.

    \sa configuration()
*/
QNetworkConfiguration QNetworkAccessManager::activeConfiguration() const
{
    Q_D(const QNetworkAccessManager);

    if (d->networkSession) {
        QNetworkConfigurationManager manager;

        return manager.configurationFromIdentifier(
            d->networkSession->sessionProperty(QLatin1String("ActiveConfiguration")).toString());
    } else {
        return QNetworkConfiguration();
    }
}

/*!
    \since 4.7

    Overrides the reported network accessibility.  If \a accessible is NotAccessible the reported
    network accessiblity will always be NotAccessible.  Otherwise the reported network
    accessibility will reflect the actual device state.
*/
void QNetworkAccessManager::setNetworkAccessible(QNetworkAccessManager::NetworkAccessibility accessible)
{
    Q_D(QNetworkAccessManager);

    if (d->networkAccessible != accessible) {
        NetworkAccessibility previous = networkAccessible();
        d->networkAccessible = accessible;
        NetworkAccessibility current = networkAccessible();
        if (previous != current)
            emit networkAccessibleChanged(current);
    }
}

/*!
    \since 4.7

    Returns the current network accessibility.
*/
QNetworkAccessManager::NetworkAccessibility QNetworkAccessManager::networkAccessible() const
{
    Q_D(const QNetworkAccessManager);

    if (d->networkSession) {
        // d->online holds online/offline state of this network session.
        if (d->online)
            return d->networkAccessible;
        else
            return NotAccessible;
    } else {
        // Network accessibility is either disabled or unknown.
        return (d->networkAccessible == NotAccessible) ? NotAccessible : UnknownAccessibility;
    }
}

#endif // QT_NO_BEARERMANAGEMENT

/*!
    \since 4.7

    Sends a custom request to the server identified by the URL of \a request.

    It is the user's responsibility to send a \a verb to the server that is valid
    according to the HTTP specification.

    This method provides means to send verbs other than the common ones provided
    via get() or post() etc., for instance sending an HTTP OPTIONS command.

    If \a data is not empty, the contents of the \a data
    device will be uploaded to the server; in that case, data must be open for
    reading and must remain valid until the finished() signal is emitted for this reply.

    \note This feature is currently available for HTTP(S) only.

    \sa get(), post(), put(), deleteResource()
*/
QNetworkReply *QNetworkAccessManager::sendCustomRequest(const QNetworkRequest &request, const QByteArray &verb, QIODevice *data)
{
    QNetworkRequest newRequest(request);
    newRequest.setAttribute(QNetworkRequest::CustomVerbAttribute, verb);
    return d_func()->postProcess(createRequest(QNetworkAccessManager::CustomOperation, newRequest, data));
}

/*!
    Returns a new QNetworkReply object to handle the operation \a op
    and request \a req. The device \a outgoingData is always 0 for Get and
    Head requests, but is the value passed to post() and put() in
    those operations (the QByteArray variants will pass a QBuffer
    object).

    The default implementation calls QNetworkCookieJar::cookiesForUrl()
    on the cookie jar set with setCookieJar() to obtain the cookies to
    be sent to the remote server.

    The returned object must be in an open state.
*/
QNetworkReply *QNetworkAccessManager::createRequest(QNetworkAccessManager::Operation op,
                                                    const QNetworkRequest &req,
                                                    QIODevice *outgoingData)
{
    Q_D(QNetworkAccessManager);

    bool isLocalFile = req.url().isLocalFile();
    QString scheme = req.url().scheme().toLower();

    // fast path for GET on file:// URLs
    // The QNetworkAccessFileBackend will right now only be used for PUT
    if ((op == QNetworkAccessManager::GetOperation || op == QNetworkAccessManager::HeadOperation)
        && (isLocalFile || scheme == QLatin1String("qrc"))) {
        return new QNetworkReplyFileImpl(this, req, op);
    }

    if ((op == QNetworkAccessManager::GetOperation || op == QNetworkAccessManager::HeadOperation)
            && scheme == QLatin1String("data")) {
        return new QNetworkReplyDataImpl(this, req, op);
    }

    // A request with QNetworkRequest::AlwaysCache does not need any bearer management
    QNetworkRequest::CacheLoadControl mode =
        static_cast<QNetworkRequest::CacheLoadControl>(
            req.attribute(QNetworkRequest::CacheLoadControlAttribute,
                              QNetworkRequest::PreferNetwork).toInt());
    if (mode == QNetworkRequest::AlwaysCache
        && (op == QNetworkAccessManager::GetOperation
        || op == QNetworkAccessManager::HeadOperation)) {
        // FIXME Implement a QNetworkReplyCacheImpl instead, see QTBUG-15106
        QNetworkReplyImpl *reply = new QNetworkReplyImpl(this);
        QNetworkReplyImplPrivate *priv = reply->d_func();
        priv->manager = this;
        priv->backend = new QNetworkAccessCacheBackend();
        priv->backend->manager = this->d_func();
        priv->backend->setParent(reply);
        priv->backend->reply = priv;
        priv->setup(op, req, outgoingData);
        return reply;
    }

#ifndef QT_NO_BEARERMANAGEMENT
    // Return a disabled network reply if network access is disabled.
    // Except if the scheme is empty or file://.
    if (!d->networkAccessible && !isLocalFile) {
        return new QDisabledNetworkReply(this, req, op);
    }

    if (!d->networkSession && (d->initializeSession || !d->networkConfiguration.isEmpty())) {
        QNetworkConfigurationManager manager;
        if (!d->networkConfiguration.isEmpty()) {
            d->createSession(manager.configurationFromIdentifier(d->networkConfiguration));
        } else {
            if (manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired)
                d->createSession(manager.defaultConfiguration());
            else
                d->initializeSession = false;
        }
    }

    if (d->networkSession)
        d->networkSession->setSessionProperty(QLatin1String("AutoCloseSessionTimeout"), -1);
#endif

    QNetworkRequest request = req;
    if (!request.header(QNetworkRequest::ContentLengthHeader).isValid() &&
        outgoingData && !outgoingData->isSequential()) {
        // request has no Content-Length
        // but the data that is outgoing is random-access
        request.setHeader(QNetworkRequest::ContentLengthHeader, outgoingData->size());
    }

    if (static_cast<QNetworkRequest::LoadControl>
        (request.attribute(QNetworkRequest::CookieLoadControlAttribute,
                           QNetworkRequest::Automatic).toInt()) == QNetworkRequest::Automatic) {
        if (d->cookieJar) {
            QList<QNetworkCookie> cookies = d->cookieJar->cookiesForUrl(request.url());
            if (!cookies.isEmpty())
                request.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(cookies));
        }
    }

    // first step: create the reply
    QUrl url = request.url();
    QNetworkReplyImpl *reply = new QNetworkReplyImpl(this);
#ifndef QT_NO_BEARERMANAGEMENT
    if (!isLocalFile) {
        connect(this, SIGNAL(networkSessionConnected()),
                reply, SLOT(_q_networkSessionConnected()));
    }
#endif
    QNetworkReplyImplPrivate *priv = reply->d_func();
    priv->manager = this;

    // second step: fetch cached credentials
    // This is not done for the time being, we should use signal emissions to request
    // the credentials from cache.

    // third step: find a backend
    priv->backend = d->findBackend(op, request);

    if (priv->backend) {
        priv->backend->setParent(reply);
        priv->backend->reply = priv;
    }

#ifndef QT_NO_OPENSSL
    reply->setSslConfiguration(request.sslConfiguration());
#endif

    // fourth step: setup the reply
    priv->setup(op, request, outgoingData);

    return reply;
}

void QNetworkAccessManagerPrivate::_q_replyFinished()
{
    Q_Q(QNetworkAccessManager);

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(q->sender());
    if (reply)
        emit q->finished(reply);

#ifndef QT_NO_BEARERMANAGEMENT
    if (networkSession && q->findChildren<QNetworkReply *>().count() == 1)
        networkSession->setSessionProperty(QLatin1String("AutoCloseSessionTimeout"), 120000);
#endif
}

void QNetworkAccessManagerPrivate::_q_replySslErrors(const QList<QSslError> &errors)
{
#ifndef QT_NO_OPENSSL
    Q_Q(QNetworkAccessManager);
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(q->sender());
    if (reply)
        emit q->sslErrors(reply, errors);
#else
    Q_UNUSED(errors);
#endif
}

QNetworkReply *QNetworkAccessManagerPrivate::postProcess(QNetworkReply *reply)
{
    Q_Q(QNetworkAccessManager);
    QNetworkReplyPrivate::setManager(reply, q);
    q->connect(reply, SIGNAL(finished()), SLOT(_q_replyFinished()));
#ifndef QT_NO_OPENSSL
    /* In case we're compiled without SSL support, we don't have this signal and we need to
     * avoid getting a connection error. */
    q->connect(reply, SIGNAL(sslErrors(QList<QSslError>)), SLOT(_q_replySslErrors(QList<QSslError>)));
#endif

    return reply;
}

void QNetworkAccessManagerPrivate::createCookieJar() const
{
    if (!cookieJarCreated) {
        // keep the ugly hack in here
        QNetworkAccessManagerPrivate *that = const_cast<QNetworkAccessManagerPrivate *>(this);
        that->cookieJar = new QNetworkCookieJar(that->q_func());
        that->cookieJarCreated = true;
    }
}

void QNetworkAccessManagerPrivate::authenticationRequired(QNetworkAccessBackend *backend,
                                                          QAuthenticator *authenticator)
{
    Q_Q(QNetworkAccessManager);

    // FIXME: Add support for domains (i.e., the leading path)
    QUrl url = backend->reply->url;

    // don't try the cache for the same URL twice in a row
    // being called twice for the same URL means the authentication failed
    // also called when last URL is empty, e.g. on first call
    if (backend->reply->urlForLastAuthentication.isEmpty()
            || url != backend->reply->urlForLastAuthentication) {
        // if credentials are included in the url, then use them
        if (!url.userName().isEmpty()
            && !url.password().isEmpty()) {
            authenticator->setUser(url.userName());
            authenticator->setPassword(url.password());
            backend->reply->urlForLastAuthentication = url;
            authenticationManager->cacheCredentials(url, authenticator);
            return;
        }

        QNetworkAuthenticationCredential cred = authenticationManager->fetchCachedCredentials(url, authenticator);
        if (!cred.isNull()) {
            authenticator->setUser(cred.user);
            authenticator->setPassword(cred.password);
            backend->reply->urlForLastAuthentication = url;
            return;
        }
    }

    // if we emit a signal here in synchronous mode, the user might spin
    // an event loop, which might recurse and lead to problems
    if (backend->isSynchronous())
        return;

    backend->reply->urlForLastAuthentication = url;
    emit q->authenticationRequired(backend->reply->q_func(), authenticator);
    authenticationManager->cacheCredentials(url, authenticator);
}

#ifndef QT_NO_NETWORKPROXY
void QNetworkAccessManagerPrivate::proxyAuthenticationRequired(QNetworkAccessBackend *backend,
                                                               const QNetworkProxy &proxy,
                                                               QAuthenticator *authenticator)
{
    Q_Q(QNetworkAccessManager);
    QAuthenticatorPrivate *priv = QAuthenticatorPrivate::getPrivate(*authenticator);
    if (proxy != backend->reply->lastProxyAuthentication && (!priv || !priv->hasFailed)) {
        QNetworkAuthenticationCredential cred = authenticationManager->fetchCachedProxyCredentials(proxy);
        if (!cred.isNull()) {
            authenticator->setUser(cred.user);
            authenticator->setPassword(cred.password);
            return;
        }
    }

    // if we emit a signal here in synchronous mode, the user might spin
    // an event loop, which might recurse and lead to problems
    if (backend->isSynchronous())
        return;

    backend->reply->lastProxyAuthentication = proxy;
    emit q->proxyAuthenticationRequired(proxy, authenticator);
    authenticationManager->cacheProxyCredentials(proxy, authenticator);
}

QList<QNetworkProxy> QNetworkAccessManagerPrivate::queryProxy(const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> proxies;
    if (proxyFactory) {
        proxies = proxyFactory->queryProxy(query);
        if (proxies.isEmpty()) {
            qWarning("QNetworkAccessManager: factory %p has returned an empty result set",
                     proxyFactory);
            proxies << QNetworkProxy::NoProxy;
        }
    } else if (proxy.type() == QNetworkProxy::DefaultProxy) {
        // no proxy set, query the application
        return QNetworkProxyFactory::proxyForQuery(query);
    } else {
        proxies << proxy;
    }

    return proxies;
}
#endif

void QNetworkAccessManagerPrivate::clearCache(QNetworkAccessManager *manager)
{
    manager->d_func()->objectCache.clear();
    manager->d_func()->authenticationManager->clearCache();

    if (manager->d_func()->httpThread) {
        // The thread will deleteLater() itself from its finished() signal
        manager->d_func()->httpThread->quit();
        manager->d_func()->httpThread->wait(5000);
        manager->d_func()->httpThread = 0;
    }
}

QNetworkAccessManagerPrivate::~QNetworkAccessManagerPrivate()
{
    if (httpThread) {
        // The thread will deleteLater() itself from its finished() signal
        httpThread->quit();
        httpThread->wait(5000);
        httpThread = 0;
    }
}

#ifndef QT_NO_BEARERMANAGEMENT
void QNetworkAccessManagerPrivate::createSession(const QNetworkConfiguration &config)
{
    Q_Q(QNetworkAccessManager);

    initializeSession = false;

    QSharedPointer<QNetworkSession> newSession;
    if (config.isValid())
        newSession = QSharedNetworkSessionManager::getSession(config);

    if (networkSession) {
        //do nothing if new and old session are the same
        if (networkSession == newSession)
            return;
        //disconnect from old session
        QObject::disconnect(networkSession.data(), SIGNAL(opened()), q, SIGNAL(networkSessionConnected()));
        QObject::disconnect(networkSession.data(), SIGNAL(closed()), q, SLOT(_q_networkSessionClosed()));
        QObject::disconnect(networkSession.data(), SIGNAL(stateChanged(QNetworkSession::State)),
            q, SLOT(_q_networkSessionStateChanged(QNetworkSession::State)));
    }

    //switch to new session (null if config was invalid)
    networkSession = newSession;

    if (!networkSession) {
        online = false;

        if (networkAccessible == QNetworkAccessManager::NotAccessible)
            emit q->networkAccessibleChanged(QNetworkAccessManager::NotAccessible);
        else
            emit q->networkAccessibleChanged(QNetworkAccessManager::UnknownAccessibility);

        return;
    }

    //connect to new session
    QObject::connect(networkSession.data(), SIGNAL(opened()), q, SIGNAL(networkSessionConnected()), Qt::QueuedConnection);
    //QueuedConnection is used to avoid deleting the networkSession inside its closed signal
    QObject::connect(networkSession.data(), SIGNAL(closed()), q, SLOT(_q_networkSessionClosed()), Qt::QueuedConnection);
    QObject::connect(networkSession.data(), SIGNAL(stateChanged(QNetworkSession::State)),
                     q, SLOT(_q_networkSessionStateChanged(QNetworkSession::State)), Qt::QueuedConnection);

    _q_networkSessionStateChanged(networkSession->state());
}

void QNetworkAccessManagerPrivate::_q_networkSessionClosed()
{
    Q_Q(QNetworkAccessManager);
    if (networkSession) {
        networkConfiguration = networkSession->configuration().identifier();

        //disconnect from old session
        QObject::disconnect(networkSession.data(), SIGNAL(opened()), q, SIGNAL(networkSessionConnected()));
        QObject::disconnect(networkSession.data(), SIGNAL(closed()), q, SLOT(_q_networkSessionClosed()));
        QObject::disconnect(networkSession.data(), SIGNAL(stateChanged(QNetworkSession::State)),
            q, SLOT(_q_networkSessionStateChanged(QNetworkSession::State)));
        networkSession.clear();
    }
}

void QNetworkAccessManagerPrivate::_q_networkSessionStateChanged(QNetworkSession::State state)
{
    Q_Q(QNetworkAccessManager);

    //Do not emit the networkSessionConnected signal here, except for roaming -> connected
    //transition, otherwise it is emitted twice in a row when opening a connection.
    if (state == QNetworkSession::Connected && lastSessionState == QNetworkSession::Roaming)
        emit q->networkSessionConnected();
    lastSessionState = state;

    if (online) {
        if (state != QNetworkSession::Connected && state != QNetworkSession::Roaming) {
            online = false;
            emit q->networkAccessibleChanged(QNetworkAccessManager::NotAccessible);
        }
    } else {
        if (state == QNetworkSession::Connected || state == QNetworkSession::Roaming) {
            online = true;
            emit q->networkAccessibleChanged(networkAccessible);
        }
    }
}
#endif // QT_NO_BEARERMANAGEMENT

QNetworkRequest QNetworkAccessManagerPrivate::prepareMultipart(const QNetworkRequest &request, QHttpMultiPart *multiPart)
{
    // copy the request, we probably need to add some headers
    QNetworkRequest newRequest(request);

    // add Content-Type header if not there already
    if (!request.header(QNetworkRequest::ContentTypeHeader).isValid()) {
        QByteArray contentType;
        contentType.reserve(34 + multiPart->d_func()->boundary.count());
        contentType += "multipart/";
        switch (multiPart->d_func()->contentType) {
        case QHttpMultiPart::RelatedType:
            contentType += "related";
            break;
        case QHttpMultiPart::FormDataType:
            contentType += "form-data";
            break;
        case QHttpMultiPart::AlternativeType:
            contentType += "alternative";
            break;
        default:
            contentType += "mixed";
            break;
        }
        // putting the boundary into quotes, recommended in RFC 2046 section 5.1.1
        contentType += "; boundary=\"" + multiPart->d_func()->boundary + "\"";
        newRequest.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(contentType));
    }

    // add MIME-Version header if not there already (we must include the header
    // if the message conforms to RFC 2045, see section 4 of that RFC)
    QByteArray mimeHeader("MIME-Version");
    if (!request.hasRawHeader(mimeHeader))
        newRequest.setRawHeader(mimeHeader, QByteArray("1.0"));

    QIODevice *device = multiPart->d_func()->device;
    if (!device->isReadable()) {
        if (!device->isOpen()) {
            if (!device->open(QIODevice::ReadOnly))
                qWarning("could not open device for reading");
        } else {
            qWarning("device is not readable");
        }
    }

    return newRequest;
}

QT_END_NAMESPACE

#include "moc_qnetworkaccessmanager.cpp"
