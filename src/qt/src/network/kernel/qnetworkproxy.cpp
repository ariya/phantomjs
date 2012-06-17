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


/*!
    \class QNetworkProxy

    \since 4.1

    \brief The QNetworkProxy class provides a network layer proxy.

    \reentrant
    \ingroup network
    \inmodule QtNetwork

    QNetworkProxy provides the method for configuring network layer
    proxy support to the Qt network classes. The currently supported
    classes are QAbstractSocket, QTcpSocket, QUdpSocket, QTcpServer,
    QNetworkAccessManager and QFtp. The proxy support is designed to
    be as transparent as possible. This means that existing
    network-enabled applications that you have written should
    automatically support network proxy using the following code.

    \snippet doc/src/snippets/code/src_network_kernel_qnetworkproxy.cpp 0

    An alternative to setting an application wide proxy is to specify
    the proxy for individual sockets using QAbstractSocket::setProxy()
    and QTcpServer::setProxy(). In this way, it is possible to disable
    the use of a proxy for specific sockets using the following code:

    \snippet doc/src/snippets/code/src_network_kernel_qnetworkproxy.cpp 1

    Network proxy is not used if the address used in \l
    {QAbstractSocket::connectToHost()}{connectToHost()}, \l
    {QUdpSocket::bind()}{bind()} or \l
    {QTcpServer::listen()}{listen()} is equivalent to
    QHostAddress::LocalHost or QHostAddress::LocalHostIPv6.

    Each type of proxy support has certain restrictions associated with it.
    You should read the \l{ProxyType} documentation carefully before
    selecting a proxy type to use.

    \note Changes made to currently connected sockets do not take effect.
    If you need to change a connected socket, you should reconnect it.

    \section1 SOCKS5

    The SOCKS5 support in Qt 4 is based on \l{RFC 1928} and \l{RFC 1929}.
    The supported authentication methods are no authentication and
    username/password authentication.  Both IPv4 and IPv6 are
    supported. Domain names are resolved through the SOCKS5 server if
    the QNetworkProxy::HostNameLookupCapability is enabled, otherwise
    they are resolved locally and the IP address is sent to the
    server. There are several things to remember when using SOCKS5
    with QUdpSocket and QTcpServer:

    With QUdpSocket, a call to \l {QUdpSocket::bind()}{bind()} may fail
    with a timeout error. If a port number other than 0 is passed to
    \l {QUdpSocket::bind()}{bind()}, it is not guaranteed that it is the
    specified port that will be used.
    Use \l{QUdpSocket::localPort()}{localPort()} and
    \l{QUdpSocket::localAddress()}{localAddress()} to get the actual
    address and port number in use. Because proxied UDP goes through
    two UDP connections, it is more likely that packets will be dropped.

    With QTcpServer a call to \l{QTcpServer::listen()}{listen()} may
    fail with a timeout error. If a port number other than 0 is passed
    to \l{QTcpServer::listen()}{listen()}, then it is not guaranteed
    that it is the specified port that will be used.
    Use \l{QTcpServer::serverPort()}{serverPort()} and
    \l{QTcpServer::serverAddress()}{serverAddress()} to get the actual
    address and port used to listen for connections. SOCKS5 only supports
    one accepted connection per call to \l{QTcpServer::listen()}{listen()},
    and each call is likely to result in a different
    \l{QTcpServer::serverPort()}{serverPort()} being used.

    \sa QAbstractSocket, QTcpServer
*/

/*!
    \enum QNetworkProxy::ProxyType

    This enum describes the types of network proxying provided in Qt.

    There are two types of proxies that Qt understands:
    transparent proxies and caching proxies. The first group consists
    of proxies that can handle any arbitrary data transfer, while the
    second can only handle specific requests. The caching proxies only
    make sense for the specific classes where they can be used.

    \value NoProxy No proxying is used
    \value DefaultProxy Proxy is determined based on the application proxy set using setApplicationProxy()
    \value Socks5Proxy \l Socks5 proxying is used
    \value HttpProxy HTTP transparent proxying is used (This value was introduced in 4.3.)
    \value HttpCachingProxy Proxying for HTTP requests only (This value was introduced in 4.4.)
    \value FtpCachingProxy Proxying for FTP requests only (This value was introduced in 4.4.)

    The table below lists different proxy types and their
    capabilities. Since each proxy type has different capabilities, it
    is important to understand them before choosing a proxy type.

    \table
    \header
        \o Proxy type
        \o Description
        \o Default capabilities

    \row
        \o SOCKS 5
        \o Generic proxy for any kind of connection. Supports TCP,
           UDP, binding to a port (incoming connections) and
           authentication.
        \o TunnelingCapability, ListeningCapability,
           UdpTunnelingCapability, HostNameLookupCapability

    \row
        \o HTTP
        \o Implemented using the "CONNECT" command, supports only
           outgoing TCP connections; supports authentication.
        \o TunnelingCapability, CachingCapability, HostNameLookupCapability

    \row
        \o Caching-only HTTP
        \o Implemented using normal HTTP commands, it is useful only
           in the context of HTTP requests (see QNetworkAccessManager)
        \o CachingCapability, HostNameLookupCapability

    \row
        \o Caching FTP
        \o Implemented using an FTP proxy, it is useful only in the
           context of FTP requests (see QFtp,
           QNetworkAccessManager)
        \o CachingCapability, HostNameLookupCapability

    \endtable

    Also note that you shouldn't set the application default proxy
    (setApplicationProxy()) to a proxy that doesn't have the
    TunnelingCapability capability. If you do, QTcpSocket will not
    know how to open connections.

    \sa setType(), type(), capabilities(), setCapabilities()
*/

/*!
    \enum QNetworkProxy::Capability
    \since 4.5

    These flags indicate the capabilities that a given proxy server
    supports.

    QNetworkProxy sets different capabilities by default when the
    object is created (see QNetworkProxy::ProxyType for a list of the
    defaults). However, it is possible to change the capabitilies
    after the object has been created with setCapabilities().

    The capabilities that QNetworkProxy supports are:

    \value TunnelingCapability Ability to open transparent, tunneled
    TCP connections to a remote host. The proxy server relays the
    transmission verbatim from one side to the other and does no
    caching.

    \value ListeningCapability Ability to create a listening socket
    and wait for an incoming TCP connection from a remote host.

    \value UdpTunnelingCapability Ability to relay UDP datagrams via
    the proxy server to and from a remote host.

    \value CachingCapability Ability to cache the contents of the
    transfer. This capability is specific to each protocol and proxy
    type. For example, HTTP proxies can cache the contents of web data
    transferred with "GET" commands.

    \value HostNameLookupCapability Ability to connect to perform the
    lookup on a remote host name and connect to it, as opposed to
    requiring the application to perform the name lookup and request
    connection to IP addresses only.
*/

#include "qnetworkproxy.h"

#ifndef QT_NO_NETWORKPROXY

#include "private/qnetworkproxy_p.h"
#include "private/qsocks5socketengine_p.h"
#include "private/qhttpsocketengine_p.h"
#include "qauthenticator.h"
#include "qhash.h"
#include "qmutex.h"
#include "qurl.h"

#ifndef QT_NO_BEARERMANAGEMENT
#include <QtNetwork/QNetworkConfiguration>
#endif

QT_BEGIN_NAMESPACE

class QSocks5SocketEngineHandler;
class QHttpSocketEngineHandler;

class QGlobalNetworkProxy
{
public:
    QGlobalNetworkProxy()
        : mutex(QMutex::Recursive)
        , applicationLevelProxy(0)
        , applicationLevelProxyFactory(0)
        , socks5SocketEngineHandler(0)
        , httpSocketEngineHandler(0)
    {
#ifndef QT_NO_SOCKS5
        socks5SocketEngineHandler = new QSocks5SocketEngineHandler();
#endif
#ifndef QT_NO_HTTP
        httpSocketEngineHandler = new QHttpSocketEngineHandler();
#endif
    }

    ~QGlobalNetworkProxy()
    {
        delete applicationLevelProxy;
        delete applicationLevelProxyFactory;
        delete socks5SocketEngineHandler;
        delete httpSocketEngineHandler;
    }

    void setApplicationProxy(const QNetworkProxy &proxy)
    {
        QMutexLocker lock(&mutex);
        if (!applicationLevelProxy)
            applicationLevelProxy = new QNetworkProxy;
        *applicationLevelProxy = proxy;
        delete applicationLevelProxyFactory;
        applicationLevelProxyFactory = 0;
    }

    void setApplicationProxyFactory(QNetworkProxyFactory *factory)
    {
        QMutexLocker lock(&mutex);
        if (applicationLevelProxy)
            *applicationLevelProxy = QNetworkProxy();
        delete applicationLevelProxyFactory;
        applicationLevelProxyFactory = factory;
    }

    QNetworkProxy applicationProxy()
    {
        return proxyForQuery(QNetworkProxyQuery()).first();
    }

    QList<QNetworkProxy> proxyForQuery(const QNetworkProxyQuery &query);

private:
    QMutex mutex;
    QNetworkProxy *applicationLevelProxy;
    QNetworkProxyFactory *applicationLevelProxyFactory;
    QSocks5SocketEngineHandler *socks5SocketEngineHandler;
    QHttpSocketEngineHandler *httpSocketEngineHandler;
};

QList<QNetworkProxy> QGlobalNetworkProxy::proxyForQuery(const QNetworkProxyQuery &query)
{
    QMutexLocker locker(&mutex);

    QList<QNetworkProxy> result;
    if (!applicationLevelProxyFactory) {
        if (applicationLevelProxy
            && applicationLevelProxy->type() != QNetworkProxy::DefaultProxy)
            result << *applicationLevelProxy;
        else
            result << QNetworkProxy(QNetworkProxy::NoProxy);
        return result;
    }

    // we have a factory
    result = applicationLevelProxyFactory->queryProxy(query);
    if (result.isEmpty()) {
        qWarning("QNetworkProxyFactory: factory %p has returned an empty result set",
                 applicationLevelProxyFactory);
        result << QNetworkProxy(QNetworkProxy::NoProxy);
    }
    return result;
}

Q_GLOBAL_STATIC(QGlobalNetworkProxy, globalNetworkProxy)

namespace {
    template<bool> struct StaticAssertTest;
    template<> struct StaticAssertTest<true> { enum { Value = 1 }; };
}

static inline void qt_noop_with_arg(int) {}
#define q_static_assert(expr)   qt_noop_with_arg(sizeof(StaticAssertTest< expr >::Value))

static QNetworkProxy::Capabilities defaultCapabilitiesForType(QNetworkProxy::ProxyType type)
{
    q_static_assert(int(QNetworkProxy::DefaultProxy) == 0);
    q_static_assert(int(QNetworkProxy::FtpCachingProxy) == 5);
    static const int defaults[] =
    {
        /* [QNetworkProxy::DefaultProxy] = */
        (int(QNetworkProxy::ListeningCapability) |
         int(QNetworkProxy::TunnelingCapability) |
         int(QNetworkProxy::UdpTunnelingCapability)),
        /* [QNetworkProxy::Socks5Proxy] = */
        (int(QNetworkProxy::TunnelingCapability) |
         int(QNetworkProxy::ListeningCapability) |
         int(QNetworkProxy::UdpTunnelingCapability) |
         int(QNetworkProxy::HostNameLookupCapability)),
        // it's weird to talk about the proxy capabilities of a "not proxy"...
        /* [QNetworkProxy::NoProxy] = */
        (int(QNetworkProxy::ListeningCapability) |
         int(QNetworkProxy::TunnelingCapability) |
         int(QNetworkProxy::UdpTunnelingCapability)),
        /* [QNetworkProxy::HttpProxy] = */
        (int(QNetworkProxy::TunnelingCapability) |
         int(QNetworkProxy::CachingCapability) |
         int(QNetworkProxy::HostNameLookupCapability)),
        /* [QNetworkProxy::HttpCachingProxy] = */
        (int(QNetworkProxy::CachingCapability) |
         int(QNetworkProxy::HostNameLookupCapability)),
        /* [QNetworkProxy::FtpCachingProxy] = */
        (int(QNetworkProxy::CachingCapability) |
         int(QNetworkProxy::HostNameLookupCapability)),
    };

    if (int(type) < 0 || int(type) > int(QNetworkProxy::FtpCachingProxy))
        type = QNetworkProxy::DefaultProxy;
    return QNetworkProxy::Capabilities(defaults[int(type)]);
}

class QNetworkProxyPrivate: public QSharedData
{
public:
    QString hostName;
    QString user;
    QString password;
    QNetworkProxy::Capabilities capabilities;
    quint16 port;
    QNetworkProxy::ProxyType type;
    bool capabilitiesSet;

    inline QNetworkProxyPrivate(QNetworkProxy::ProxyType t = QNetworkProxy::DefaultProxy,
                                const QString &h = QString(), quint16 p = 0,
                                const QString &u = QString(), const QString &pw = QString())
        : hostName(h),
          user(u),
          password(pw),
          capabilities(defaultCapabilitiesForType(t)),
          port(p),
          type(t),
          capabilitiesSet(false)
    { }

    inline bool operator==(const QNetworkProxyPrivate &other) const
    {
        return type == other.type &&
            port == other.port &&
            hostName == other.hostName &&
            user == other.user &&
            password == other.password &&
            capabilities == other.capabilities;
    }
};

template<> void QSharedDataPointer<QNetworkProxyPrivate>::detach()
{
    if (d && d->ref == 1)
        return;
    QNetworkProxyPrivate *x = (d ? new QNetworkProxyPrivate(*d)
                               : new QNetworkProxyPrivate);
    x->ref.ref();
    if (d && !d->ref.deref())
        delete d;
    d = x;
}

/*!
    Constructs a QNetworkProxy with DefaultProxy type; the proxy type is
    determined by applicationProxy(), which defaults to NoProxy.

    \sa setType(), setApplicationProxy()
*/
QNetworkProxy::QNetworkProxy()
    : d(0)
{
    // make sure we have QGlobalNetworkProxy singleton created, otherwise
    // you don't have any socket engine handler created when directly setting
    // a proxy to the socket
    globalNetworkProxy();
}

/*!
    Constructs a QNetworkProxy with \a type, \a hostName, \a port,
    \a user and \a password.

    The default capabilities for proxy type \a type are set automatically.

    \sa capabilities()
*/
QNetworkProxy::QNetworkProxy(ProxyType type, const QString &hostName, quint16 port,
                  const QString &user, const QString &password)
    : d(new QNetworkProxyPrivate(type, hostName, port, user, password))
{
    // make sure we have QGlobalNetworkProxy singleton created, otherwise
    // you don't have any socket engine handler created when directly setting
    // a proxy to a socket
    globalNetworkProxy();
}

/*!
    Constructs a copy of \a other.
*/
QNetworkProxy::QNetworkProxy(const QNetworkProxy &other)
    : d(other.d)
{
}

/*!
    Destroys the QNetworkProxy object.
*/
QNetworkProxy::~QNetworkProxy()
{
    // QSharedDataPointer takes care of deleting for us
}

/*!
    \since 4.4

    Compares the value of this network proxy to \a other and returns true
    if they are equal (same proxy type, server as well as username and password)
*/
bool QNetworkProxy::operator==(const QNetworkProxy &other) const
{
    return d == other.d || (d && other.d && *d == *other.d);
}

/*!
    \fn bool QNetworkProxy::operator!=(const QNetworkProxy &other) const
    \since 4.4

    Compares the value of this network proxy to \a other and returns true
    if they differ.
\*/

/*!
    \since 4.2

    Assigns the value of the network proxy \a other to this network proxy.
*/
QNetworkProxy &QNetworkProxy::operator=(const QNetworkProxy &other)
{
    d = other.d;
    return *this;
}

/*!
    Sets the proxy type for this instance to be \a type.

    Note that changing the type of a proxy does not change
    the set of capabilities this QNetworkProxy object holds if any
    capabilities have been set with setCapabilities().

    \sa type(), setCapabilities()
*/
void QNetworkProxy::setType(QNetworkProxy::ProxyType type)
{
    d->type = type;
    if (!d->capabilitiesSet)
        d->capabilities = defaultCapabilitiesForType(type);
}

/*!
    Returns the proxy type for this instance.

    \sa setType()
*/
QNetworkProxy::ProxyType QNetworkProxy::type() const
{
    return d ? d->type : DefaultProxy;
}

/*!
    \since 4.5

    Sets the capabilities of this proxy to \a capabilities.

    \sa setType(), capabilities()
*/
void QNetworkProxy::setCapabilities(Capabilities capabilities)
{
    d->capabilities = capabilities;
    d->capabilitiesSet = true;
}

/*!
    \since 4.5

    Returns the capabilities of this proxy server.

    \sa setCapabilities(), type()
*/
QNetworkProxy::Capabilities QNetworkProxy::capabilities() const
{
    return d ? d->capabilities : defaultCapabilitiesForType(DefaultProxy);
}

/*!
    \since 4.4

    Returns true if this proxy supports the
    QNetworkProxy::CachingCapability capability.

    In Qt 4.4, the capability was tied to the proxy type, but since Qt
    4.5 it is possible to remove the capability of caching from a
    proxy by calling setCapabilities().

    \sa capabilities(), type(), isTransparentProxy()
*/
bool QNetworkProxy::isCachingProxy() const
{
    return capabilities() & CachingCapability;
}

/*!
    \since 4.4

    Returns true if this proxy supports transparent tunneling of TCP
    connections. This matches the QNetworkProxy::TunnelingCapability
    capability.

    In Qt 4.4, the capability was tied to the proxy type, but since Qt
    4.5 it is possible to remove the capability of caching from a
    proxy by calling setCapabilities().

    \sa capabilities(), type(), isCachingProxy()
*/
bool QNetworkProxy::isTransparentProxy() const
{
    return capabilities() & TunnelingCapability;
}

/*!
    Sets the user name for proxy authentication to be \a user.

    \sa user(), setPassword(), password()
*/
void QNetworkProxy::setUser(const QString &user)
{
    d->user = user;
}

/*!
    Returns the user name used for authentication.

    \sa setUser(), setPassword(), password()
*/
QString QNetworkProxy::user() const
{
    return d ? d->user : QString();
}

/*!
    Sets the password for proxy authentication to be \a password.

    \sa user(), setUser(), password()
*/
void QNetworkProxy::setPassword(const QString &password)
{
    d->password = password;
}

/*!
    Returns the password used for authentication.

    \sa user(), setPassword(), setUser()
*/
QString QNetworkProxy::password() const
{
    return d ? d->password : QString();
}

/*!
    Sets the host name of the proxy host to be \a hostName.

    \sa hostName(), setPort(), port()
*/
void QNetworkProxy::setHostName(const QString &hostName)
{
    d->hostName = hostName;
}

/*!
    Returns the host name of the proxy host.

    \sa setHostName(), setPort(), port()
*/
QString QNetworkProxy::hostName() const
{
    return d ? d->hostName : QString();
}

/*!
    Sets the port of the proxy host to be \a port.

    \sa hostName(), setHostName(), port()
*/
void QNetworkProxy::setPort(quint16 port)
{
    d->port = port;
}

/*!
    Returns the port of the proxy host.

    \sa setHostName(), setPort(), hostName()
*/
quint16 QNetworkProxy::port() const
{
    return d ? d->port : 0;
}

/*!
    Sets the application level network proxying to be \a networkProxy.

    If a QAbstractSocket or QTcpSocket has the
    QNetworkProxy::DefaultProxy type, then the QNetworkProxy set with
    this function is used. If you want more flexibility in determining
    which the proxy, use the QNetworkProxyFactory class.

    Setting a default proxy value with this function will override the
    application proxy factory set with
    QNetworkProxyFactory::setApplicationProxyFactory.

    \sa QNetworkProxyFactory, applicationProxy(), QAbstractSocket::setProxy(), QTcpServer::setProxy()
*/
void QNetworkProxy::setApplicationProxy(const QNetworkProxy &networkProxy)
{
    if (globalNetworkProxy()) {
        // don't accept setting the proxy to DefaultProxy
        if (networkProxy.type() == DefaultProxy)
            globalNetworkProxy()->setApplicationProxy(QNetworkProxy::NoProxy);
        else
            globalNetworkProxy()->setApplicationProxy(networkProxy);
    }
}

/*!
    Returns the application level network proxying.

    If a QAbstractSocket or QTcpSocket has the
    QNetworkProxy::DefaultProxy type, then the QNetworkProxy returned
    by this function is used.

    \sa QNetworkProxyFactory, setApplicationProxy(), QAbstractSocket::proxy(), QTcpServer::proxy()
*/
QNetworkProxy QNetworkProxy::applicationProxy()
{
    if (globalNetworkProxy())
        return globalNetworkProxy()->applicationProxy();
    return QNetworkProxy();
}

class QNetworkProxyQueryPrivate: public QSharedData
{
public:
    inline QNetworkProxyQueryPrivate()
        : localPort(-1), type(QNetworkProxyQuery::TcpSocket)
    { }

    bool operator==(const QNetworkProxyQueryPrivate &other) const
    {
        return type == other.type &&
            localPort == other.localPort &&
            remote == other.remote;
    }

    QUrl remote;
    int localPort;
    QNetworkProxyQuery::QueryType type;
#ifndef QT_NO_BEARERMANAGEMENT
    QNetworkConfiguration config;
#endif
};

template<> void QSharedDataPointer<QNetworkProxyQueryPrivate>::detach()
{
    if (d && d->ref == 1)
        return;
    QNetworkProxyQueryPrivate *x = (d ? new QNetworkProxyQueryPrivate(*d)
                                    : new QNetworkProxyQueryPrivate);
    x->ref.ref();
    if (d && !d->ref.deref())
        delete d;
    d = x;
}

/*!
    \class QNetworkProxyQuery
    \since 4.5
    \inmodule QtNetwork
    \brief The QNetworkProxyQuery class is used to query the proxy
    settings for a socket

    QNetworkProxyQuery holds the details of a socket being created or
    request being made. It is used by QNetworkProxy and
    QNetworkProxyFactory to allow applications to have a more
    fine-grained control over which proxy servers are used, depending
    on the details of the query. This allows an application to apply
    different settings, according to the protocol or destination
    hostname, for instance.

    QNetworkProxyQuery supports the following criteria for selecting
    the proxy:

    \list
      \o the type of query
      \o the local port number to use
      \o the destination host name
      \o the destination port number
      \o the protocol name, such as "http" or "ftp"
      \o the URL being requested
    \endlist

    The destination host name is the host in the connection in the
    case of outgoing connection sockets. It is the \c hostName
    parameter passed to QTcpSocket::connectToHost() or the host
    component of a URL requested with QNetworkRequest.

    The destination port number is the requested port to connect to in
    the case of outgoing sockets, while the local port number is the
    port the socket wishes to use locally before attempting the
    external connection. In most cases, the local port number is used
    by listening sockets only (QTcpSocket) or by datagram sockets
    (QUdpSocket).

    The protocol name is an arbitrary string that indicates the type
    of connection being attempted. For example, it can match the
    scheme of a URL, like "http", "https" and "ftp". In most cases,
    the proxy selection will not change depending on the protocol, but
    this information is provided in case a better choice can be made,
    like choosing an caching HTTP proxy for HTTP-based connections,
    but a more powerful SOCKSv5 proxy for all others.

    The network configuration specifies which configuration to use,
    when bearer management is used. For example on a mobile phone
    the proxy settings are likely to be different for the cellular
    network vs WLAN.

    Some of the criteria may not make sense in all of the types of
    query. The following table lists the criteria that are most
    commonly used, according to the type of query.

    \table
    \header
      \o Query type
      \o Description

    \row
      \o TcpSocket
      \o Normal sockets requesting a connection to a remote server,
         like QTcpSocket. The peer hostname and peer port match the
         values passed to QTcpSocket::connectToHost(). The local port
         is usually -1, indicating the socket has no preference in
         which port should be used. The URL component is not used.

    \row
      \o UdpSocket
      \o Datagram-based sockets, which can both send and
         receive. The local port, remote host or remote port fields
         can all be used or be left unused, depending on the
         characteristics of the socket. The URL component is not used.

    \row
      \o TcpServer
      \o Passive server sockets that listen on a port and await
         incoming connections from the network. Normally, only the
         local port is used, but the remote address could be used in
         specific circumstances, for example to indicate which remote
         host a connection is expected from. The URL component is not used.

    \row
      \o UrlRequest
      \o A more high-level request, such as those coming from
         QNetworkAccessManager. These requests will inevitably use an
         outgoing TCP socket, but the this query type is provided to
         indicate that more detailed information is present in the URL
         component. For ease of implementation, the URL's host and
         port are set as the destination address.
    \endtable

    It should be noted that any of the criteria may be missing or
    unknown (an empty QString for the hostname or protocol name, -1
    for the port numbers). If that happens, the functions executing
    the query should make their best guess or apply some
    implementation-defined default values.

    \sa QNetworkProxy, QNetworkProxyFactory, QNetworkAccessManager,
        QAbstractSocket::setProxy()
*/

/*!
    \enum QNetworkProxyQuery::QueryType

    Describes the type of one QNetworkProxyQuery query.

    \value TcpSocket    a normal, outgoing TCP socket
    \value UdpSocket    a datagram-based UDP socket, which could send
                        to multiple destinations
    \value TcpServer    a TCP server that listens for incoming
                        connections from the network
    \value UrlRequest   a more complex request which involves loading
                        of a URL

    \sa queryType(), setQueryType()
*/

/*!
    Constructs a default QNetworkProxyQuery object. By default, the
    query type will be QNetworkProxyQuery::TcpSocket.
*/
QNetworkProxyQuery::QNetworkProxyQuery()
{
}

/*!
    Constructs a QNetworkProxyQuery with the URL \a requestUrl and
    sets the query type to \a queryType.

    \sa protocolTag(), peerHostName(), peerPort()
*/
QNetworkProxyQuery::QNetworkProxyQuery(const QUrl &requestUrl, QueryType queryType)
{
    d->remote = requestUrl;
    d->type = queryType;
}

/*!
    Constructs a QNetworkProxyQuery of type \a queryType and sets the
    protocol tag to be \a protocolTag. This constructor is suitable
    for QNetworkProxyQuery::TcpSocket queries, because it sets the
    peer hostname to \a hostname and the peer's port number to \a
    port.
*/
QNetworkProxyQuery::QNetworkProxyQuery(const QString &hostname, int port,
                                       const QString &protocolTag,
                                       QueryType queryType)
{
    d->remote.setScheme(protocolTag);
    d->remote.setHost(hostname);
    d->remote.setPort(port);
    d->type = queryType;
}

/*!
    Constructs a QNetworkProxyQuery of type \a queryType and sets the
    protocol tag to be \a protocolTag. This constructor is suitable
    for QNetworkProxyQuery::TcpSocket queries because it sets the
    local port number to \a bindPort.

    Note that \a bindPort is of type quint16 to indicate the exact
    port number that is requested. The value of -1 (unknown) is not
    allowed in this context.

    \sa localPort()
*/
QNetworkProxyQuery::QNetworkProxyQuery(quint16 bindPort, const QString &protocolTag,
                                       QueryType queryType)
{
    d->remote.setScheme(protocolTag);
    d->localPort = bindPort;
    d->type = queryType;
}

#ifndef QT_NO_BEARERMANAGEMENT
/*!
    \since 4.8

    Constructs a QNetworkProxyQuery with the URL \a requestUrl and
    sets the query type to \a queryType. The specified \a networkConfiguration
    is used to resolve the proxy settings.

    \sa protocolTag(), peerHostName(), peerPort(), networkConfiguration()
*/
QNetworkProxyQuery::QNetworkProxyQuery(const QNetworkConfiguration &networkConfiguration,
                                       const QUrl &requestUrl, QueryType queryType)
{
    d->config = networkConfiguration;
    d->remote = requestUrl;
    d->type = queryType;
}

/*!
    \since 4.8

    Constructs a QNetworkProxyQuery of type \a queryType and sets the
    protocol tag to be \a protocolTag. This constructor is suitable
    for QNetworkProxyQuery::TcpSocket queries, because it sets the
    peer hostname to \a hostname and the peer's port number to \a
    port. The specified \a networkConfiguration
    is used to resolve the proxy settings.

    \sa networkConfiguration()
*/
QNetworkProxyQuery::QNetworkProxyQuery(const QNetworkConfiguration &networkConfiguration,
                                       const QString &hostname, int port,
                                       const QString &protocolTag,
                                       QueryType queryType)
{
    d->config = networkConfiguration;
    d->remote.setScheme(protocolTag);
    d->remote.setHost(hostname);
    d->remote.setPort(port);
    d->type = queryType;
}

/*!
    \since 4.8

    Constructs a QNetworkProxyQuery of type \a queryType and sets the
    protocol tag to be \a protocolTag. This constructor is suitable
    for QNetworkProxyQuery::TcpSocket queries because it sets the
    local port number to \a bindPort. The specified \a networkConfiguration
    is used to resolve the proxy settings.

    Note that \a bindPort is of type quint16 to indicate the exact
    port number that is requested. The value of -1 (unknown) is not
    allowed in this context.

    \sa localPort(), networkConfiguration()
*/
QNetworkProxyQuery::QNetworkProxyQuery(const QNetworkConfiguration &networkConfiguration,
                                       quint16 bindPort, const QString &protocolTag,
                                       QueryType queryType)
{
    d->config = networkConfiguration;
    d->remote.setScheme(protocolTag);
    d->localPort = bindPort;
    d->type = queryType;
}
#endif

/*!
    Constructs a QNetworkProxyQuery object that is a copy of \a other.
*/
QNetworkProxyQuery::QNetworkProxyQuery(const QNetworkProxyQuery &other)
    : d(other.d)
{
}

/*!
    Destroys this QNetworkProxyQuery object.
*/
QNetworkProxyQuery::~QNetworkProxyQuery()
{
    // QSharedDataPointer automatically deletes
}

/*!
    Copies the contents of \a other.
*/
QNetworkProxyQuery &QNetworkProxyQuery::operator=(const QNetworkProxyQuery &other)
{
    d = other.d;
    return *this;
}

/*!
    Returns true if this QNetworkProxyQuery object contains the same
    data as \a other.
*/
bool QNetworkProxyQuery::operator==(const QNetworkProxyQuery &other) const
{
    return d == other.d || (d && other.d && *d == *other.d);
}

/*!
    \fn bool QNetworkProxyQuery::operator!=(const QNetworkProxyQuery &other) const

    Returns true if this QNetworkProxyQuery object does not contain
    the same data as \a other.
*/

/*!
    Returns the query type.
*/
QNetworkProxyQuery::QueryType QNetworkProxyQuery::queryType() const
{
    return d ? d->type : TcpSocket;
}

/*!
    Sets the query type of this object to be \a type.
*/
void QNetworkProxyQuery::setQueryType(QueryType type)
{
    d->type = type;
}

/*!
    Returns the port number for the outgoing request or -1 if the port
    number is not known.

    If the query type is QNetworkProxyQuery::UrlRequest, this function
    returns the port number of the URL being requested. In general,
    frameworks will fill in the port number from their default values.

    \sa peerHostName(), localPort(), setPeerPort()
*/
int QNetworkProxyQuery::peerPort() const
{
    return d ? d->remote.port() : -1;
}

/*!
    Sets the requested port number for the outgoing connection to be
    \a port. Valid values are 1 to 65535, or -1 to indicate that the
    remote port number is unknown.

    The peer port number can also be used to indicate the expected
    port number of an incoming connection in the case of
    QNetworkProxyQuery::UdpSocket or QNetworkProxyQuery::TcpServer
    query types.

    \sa peerPort(), setPeerHostName(), setLocalPort()
*/
void QNetworkProxyQuery::setPeerPort(int port)
{
    d->remote.setPort(port);
}

/*!
    Returns the host name or IP address being of the outgoing
    connection being requested, or an empty string if the remote
    hostname is not known.

    If the query type is QNetworkProxyQuery::UrlRequest, this function
    returns the host component of the URL being requested.

    \sa peerPort(), localPort(), setPeerHostName()
*/
QString QNetworkProxyQuery::peerHostName() const
{
    return d ? d->remote.host() : QString();
}

/*!
    Sets the hostname of the outgoing connection being requested to \a
    hostname.  An empty hostname can be used to indicate that the
    remote host is unknown.

    The peer host name can also be used to indicate the expected
    source address of an incoming connection in the case of
    QNetworkProxyQuery::UdpSocket or QNetworkProxyQuery::TcpServer
    query types.

    \sa peerHostName(), setPeerPort(), setLocalPort()
*/
void QNetworkProxyQuery::setPeerHostName(const QString &hostname)
{
    d->remote.setHost(hostname);
}

/*!
    Returns the port number of the socket that will accept incoming
    packets from remote servers or -1 if the port is not known.

    \sa peerPort(), peerHostName(), setLocalPort()
*/
int QNetworkProxyQuery::localPort() const
{
    return d ? d->localPort : -1;
}

/*!
    Sets the port number that the socket wishes to use locally to
    accept incoming packets from remote servers to \a port. The local
    port is most often used with the QNetworkProxyQuery::TcpServer
    and QNetworkProxyQuery::UdpSocket query types.

    Valid values are 0 to 65535 (with 0 indicating that any port
    number will be acceptable) or -1, which means the local port
    number is unknown or not applicable.

    In some circumstances, for special protocols, it's the local port
    number can also be used with a query of type
    QNetworkProxyQuery::TcpSocket. When that happens, the socket is
    indicating it wishes to use the port number \a port when
    connecting to a remote host.

    \sa localPort(), setPeerPort(), setPeerHostName()
*/
void QNetworkProxyQuery::setLocalPort(int port)
{
    d->localPort = port;
}

/*!
    Returns the protocol tag for this QNetworkProxyQuery object, or an
    empty QString in case the protocol tag is unknown.

    In the case of queries of type QNetworkProxyQuery::UrlRequest,
    this function returns the value of the scheme component of the
    URL.

    \sa setProtocolTag(), url()
*/
QString QNetworkProxyQuery::protocolTag() const
{
    return d ? d->remote.scheme() : QString();
}

/*!
    Sets the protocol tag for this QNetworkProxyQuery object to be \a
    protocolTag.

    The protocol tag is an arbitrary string that indicates which
    protocol is being talked over the socket, such as "http", "xmpp",
    "telnet", etc. The protocol tag is used by the backend to
    return a request that is more specific to the protocol in
    question: for example, a HTTP connection could be use a caching
    HTTP proxy server, while all other connections use a more powerful
    SOCKSv5 proxy server.

    \sa protocolTag()
*/
void QNetworkProxyQuery::setProtocolTag(const QString &protocolTag)
{
    d->remote.setScheme(protocolTag);
}

/*!
    Returns the URL component of this QNetworkProxyQuery object in
    case of a query of type QNetworkProxyQuery::UrlRequest.

    \sa setUrl()
*/
QUrl QNetworkProxyQuery::url() const
{
    return d ? d->remote : QUrl();
}

/*!
    Sets the URL component of this QNetworkProxyQuery object to be \a
    url. Setting the URL will also set the protocol tag, the remote
    host name and port number. This is done so as to facilitate the
    implementation of the code that determines the proxy server to be
    used.

    \sa url(), peerHostName(), peerPort()
*/
void QNetworkProxyQuery::setUrl(const QUrl &url)
{
    d->remote = url;
}

#ifndef QT_NO_BEARERMANAGEMENT
/*!
    Returns the network configuration of the proxy query.

    \sa setNetworkConfiguration()
*/
QNetworkConfiguration QNetworkProxyQuery::networkConfiguration() const
{
    return d ? d->config : QNetworkConfiguration();
}

/*!
    \since 4.8

    Sets the network configuration component of this QNetworkProxyQuery
    object to be \a networkConfiguration. The network configuration can
    be used to return different proxy settings based on the network in
    use, for example WLAN vs cellular networks on a mobile phone.

    In the case of "user choice" or "service network" configurations,
    you should first start the QNetworkSession and obtain the active
    configuration from its properties.

    \sa networkConfiguration()
*/
void QNetworkProxyQuery::setNetworkConfiguration(const QNetworkConfiguration &networkConfiguration)
{
    d->config = networkConfiguration;
}
#endif

/*!
    \class QNetworkProxyFactory
    \brief The QNetworkProxyFactory class provides fine-grained proxy selection.
    \since 4.5

    \ingroup network
    \inmodule QtNetwork

    QNetworkProxyFactory is an extension to QNetworkProxy, allowing
    applications to have a more fine-grained control over which proxy
    servers are used, depending on the socket requesting the
    proxy. This allows an application to apply different settings,
    according to the protocol or destination hostname, for instance.

    QNetworkProxyFactory can be set globally for an application, in
    which case it will override any global proxies set with
    QNetworkProxy::setApplicationProxy(). If set globally, any sockets
    created with Qt will query the factory to determine the proxy to
    be used.

    A factory can also be set in certain frameworks that support
    multiple connections, such as QNetworkAccessManager. When set on
    such object, the factory will be queried for sockets created by
    that framework only.

    \section1 System Proxies

    You can configure a factory to use the system proxy's settings.
    Call the setUseSystemConfiguration() function with true to enable
    this behavior, or false to disable it.

    Similarly, you can use a factory to make queries directly to the
    system proxy by calling its systemProxyForQuery() function.

    \warning Depending on the configuration of the user's system, the
    use of system proxy features on certain platforms may be subject
    to limitations. The systemProxyForQuery() documentation contains a
    list of these limitations for those platforms that are affected.
*/

/*!
    Creates a QNetworkProxyFactory object.

    Since QNetworkProxyFactory is an abstract class, you cannot create
    objects of type QNetworkProxyFactory directly.
*/
QNetworkProxyFactory::QNetworkProxyFactory()
{
}

/*!
    Destroys the QNetworkProxyFactory object.
*/
QNetworkProxyFactory::~QNetworkProxyFactory()
{
}


/*!
    \since 4.6

    Enables the use of the platform-specific proxy settings, and only those.
    See systemProxyForQuery() for more information.

    Internally, this method (when called with \a enable set to true)
    sets an application-wide proxy factory. For this reason, this method
    is mutually exclusive with setApplicationProxyFactory(): calling
    setApplicationProxyFactory() overrides the use of the system-wide proxy,
    and calling setUseSystemConfiguration() overrides any
    application proxy or proxy factory that was previously set.

    \note See the systemProxyForQuery() documentation for a list of
    limitations related to the use of system proxies.
*/
void QNetworkProxyFactory::setUseSystemConfiguration(bool enable)
{
    if (enable) {
        setApplicationProxyFactory(new QSystemConfigurationProxyFactory);
    } else {
        setApplicationProxyFactory(0);
    }
}

/*!
    Sets the application-wide proxy factory to be \a factory. This
    function will take ownership of that object and will delete it
    when necessary.

    The application-wide proxy is used as a last-resort when all other
    proxy selection requests returned QNetworkProxy::DefaultProxy. For
    example, QTcpSocket objects can have a proxy set with
    QTcpSocket::setProxy, but if none is set, the proxy factory class
    set with this function will be queried.

    If you set a proxy factory with this function, any application
    level proxies set with QNetworkProxy::setApplicationProxy will be
    overridden.

    \sa QNetworkProxy::setApplicationProxy(),
        QAbstractSocket::proxy(), QAbstractSocket::setProxy()
*/
void QNetworkProxyFactory::setApplicationProxyFactory(QNetworkProxyFactory *factory)
{
    if (globalNetworkProxy())
        globalNetworkProxy()->setApplicationProxyFactory(factory);
}

/*!
    \fn QList<QNetworkProxy> QNetworkProxyFactory::queryProxy(const QNetworkProxyQuery &query)

    This function takes the query request, \a query,
    examines the details of the type of socket or request and returns
    a list of QNetworkProxy objects that indicate the proxy servers to
    be used, in order of preference.

    When reimplementing this class, take care to return at least one
    element.

    If you cannot determine a better proxy alternative, use
    QNetworkProxy::DefaultProxy, which tells the code querying for a
    proxy to use a higher alternative. For example, if this factory is
    set to a QNetworkAccessManager object, DefaultProxy will tell it
    to query the application-level proxy settings.

    If this factory is set as the application proxy factory,
    DefaultProxy and NoProxy will have the same meaning.
*/

/*!
    \fn QList<QNetworkProxy> QNetworkProxyFactory::systemProxyForQuery(const QNetworkProxyQuery &query)

    This function takes the query request, \a query,
    examines the details of the type of socket or request and returns
    a list of QNetworkProxy objects that indicate the proxy servers to
    be used, in order of preference.

    This function can be used to determine the platform-specific proxy
    settings. This function will use the libraries provided by the
    operating system to determine the proxy for a given connection, if
    such libraries exist. If they don't, this function will just return a
    QNetworkProxy of type QNetworkProxy::NoProxy.

    On Windows, this function will use the WinHTTP DLL functions. Despite
    its name, Microsoft suggests using it for all applications that
    require network connections, not just HTTP. This will respect the
    proxy settings set on the registry with the proxycfg.exe tool. If
    those settings are not found, this function will attempt to obtain
    Internet Explorer's settings and use them.

    On MacOS X, this function will obtain the proxy settings using the
    SystemConfiguration framework from Apple. It will apply the FTP,
    HTTP and HTTPS proxy configurations for queries that contain the
    protocol tag "ftp", "http" and "https", respectively. If the SOCKS
    proxy is enabled in that configuration, this function will use the
    SOCKS server for all queries. If SOCKS isn't enabled, it will use
    the HTTPS proxy for all TcpSocket and UrlRequest queries.

    On other systems, this function will pick up proxy settings from
    the "http_proxy" environment variable. This variable must be a URL
    using one of the following schemes: "http", "socks5" or "socks5h".

    \section1 Limitations

    These are the limitations for the current version of this
    function. Future versions of Qt may lift some of the limitations
    listed here.

    \list
    \o On MacOS X, this function will ignore the Proxy Auto Configuration
    settings, since it cannot execute the associated ECMAScript code.

    \o On Windows platforms, this function may take several seconds to
    execute depending on the configuration of the user's system.
    \endlist
*/

/*!
    This function takes the query request, \a query,
    examines the details of the type of socket or request and returns
    a list of QNetworkProxy objects that indicate the proxy servers to
    be used, in order of preference.
*/
QList<QNetworkProxy> QNetworkProxyFactory::proxyForQuery(const QNetworkProxyQuery &query)
{
    if (!globalNetworkProxy())
        return QList<QNetworkProxy>() << QNetworkProxy(QNetworkProxy::NoProxy);
    return globalNetworkProxy()->proxyForQuery(query);
}

QT_END_NAMESPACE

#endif // QT_NO_NETWORKPROXY
