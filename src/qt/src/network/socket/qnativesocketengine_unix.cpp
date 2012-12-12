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

//#define QNATIVESOCKETENGINE_DEBUG
#include "qnativesocketengine_p.h"
#include "private/qnet_unix_p.h"
#include "qiodevice.h"
#include "qhostaddress.h"
#include "qelapsedtimer.h"
#include "qvarlengtharray.h"
#include "qnetworkinterface.h"
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#ifndef QT_NO_IPV6IFNAME
#include <net/if.h>
#endif
#ifndef QT_NO_IPV6IFNAME
#include <net/if.h>
#endif
#ifdef QT_LINUXBASE
#include <arpa/inet.h>
#endif

#if defined QNATIVESOCKETENGINE_DEBUG
#include <qstring.h>
#include <ctype.h>
#endif

#include <netinet/tcp.h>

QT_BEGIN_NAMESPACE

#if defined QNATIVESOCKETENGINE_DEBUG

/*
    Returns a human readable representation of the first \a len
    characters in \a data.
*/
static QByteArray qt_prettyDebug(const char *data, int len, int maxSize)
{
    if (!data) return "(null)";
    QByteArray out;
    for (int i = 0; i < len; ++i) {
        char c = data[i];
        if (isprint(c)) {
            out += c;
        } else switch (c) {
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            QString tmp;
            tmp.sprintf("\\%o", c);
            out += tmp.toLatin1();
        }
    }

    if (len < maxSize)
        out += "...";

    return out;
}
#endif

static void qt_ignore_sigpipe()
{
#ifndef Q_NO_POSIX_SIGNALS
    // Set to ignore SIGPIPE once only.
    static QBasicAtomicInt atom = Q_BASIC_ATOMIC_INITIALIZER(0);
    if (atom.testAndSetRelaxed(0, 1)) {
        struct sigaction noaction;
        memset(&noaction, 0, sizeof(noaction));
        noaction.sa_handler = SIG_IGN;
        ::sigaction(SIGPIPE, &noaction, 0);
    }
#else
    // Posix signals are not supported by the underlying platform
    // so we don't need to ignore sigpipe signal explicitly
#endif
}

/*
    Extracts the port and address from a sockaddr, and stores them in
    \a port and \a addr if they are non-null.
*/
static inline void qt_socket_getPortAndAddress(const qt_sockaddr *s, quint16 *port, QHostAddress *addr)
{
#if !defined(QT_NO_IPV6)
    if (s->a.sa_family == AF_INET6) {
        Q_IPV6ADDR tmp;
        memcpy(&tmp, &s->a6.sin6_addr, sizeof(tmp));
        if (addr) {
            QHostAddress tmpAddress;
            tmpAddress.setAddress(tmp);
            *addr = tmpAddress;
#ifndef QT_NO_IPV6IFNAME
            char scopeid[IFNAMSIZ];
            if (::if_indextoname(s->a6.sin6_scope_id, scopeid)) {
                addr->setScopeId(QLatin1String(scopeid));
            } else
#endif
            addr->setScopeId(QString::number(s->a6.sin6_scope_id));
        }
        if (port)
            *port = ntohs(s->a6.sin6_port);
        return;
    }
#endif
    if (port)
        *port = ntohs(s->a4.sin_port);
    if (addr) {
        QHostAddress tmpAddress;
        tmpAddress.setAddress(ntohl(s->a4.sin_addr.s_addr));
        *addr = tmpAddress;
    }
}

/*! \internal

    Creates and returns a new socket descriptor of type \a socketType
    and \a socketProtocol.  Returns -1 on failure.
*/
bool QNativeSocketEnginePrivate::createNewSocket(QAbstractSocket::SocketType socketType,
                                         QAbstractSocket::NetworkLayerProtocol socketProtocol)
{
#ifndef QT_NO_IPV6
    int protocol = (socketProtocol == QAbstractSocket::IPv6Protocol) ? AF_INET6 : AF_INET;
#else
    Q_UNUSED(socketProtocol);
    int protocol = AF_INET;
#endif
    int type = (socketType == QAbstractSocket::UdpSocket) ? SOCK_DGRAM : SOCK_STREAM;

	int socket = qt_safe_socket(protocol, type, 0);

    if (socket <= 0) {
        switch (errno) {
        case EPROTONOSUPPORT:
        case EAFNOSUPPORT:
        case EINVAL:
            setError(QAbstractSocket::UnsupportedSocketOperationError, ProtocolUnsupportedErrorString);
            break;
        case ENFILE:
        case EMFILE:
        case ENOBUFS:
        case ENOMEM:
            setError(QAbstractSocket::SocketResourceError, ResourceErrorString);
            break;
        case EACCES:
            setError(QAbstractSocket::SocketAccessError, AccessErrorString);
            break;
        default:
            break;
        }

        return false;
    }

    socketDescriptor = socket;
    return true;
}

/*
    Returns the value of the socket option \a opt.
*/
int QNativeSocketEnginePrivate::option(QNativeSocketEngine::SocketOption opt) const
{
    Q_Q(const QNativeSocketEngine);
    if (!q->isValid())
        return -1;

    int n = -1;
    int level = SOL_SOCKET; // default

    switch (opt) {
    case QNativeSocketEngine::ReceiveBufferSocketOption:
        n = SO_RCVBUF;
        break;
    case QNativeSocketEngine::SendBufferSocketOption:
        n = SO_SNDBUF;
        break;
    case QNativeSocketEngine::NonBlockingSocketOption:
        break;
    case QNativeSocketEngine::BroadcastSocketOption:
        break;
    case QNativeSocketEngine::AddressReusable:
        n = SO_REUSEADDR;
        break;
    case QNativeSocketEngine::BindExclusively:
        return true;
    case QNativeSocketEngine::ReceiveOutOfBandData:
        n = SO_OOBINLINE;
        break;
    case QNativeSocketEngine::LowDelayOption:
        level = IPPROTO_TCP;
        n = TCP_NODELAY;
        break;
    case QNativeSocketEngine::KeepAliveOption:
        n = SO_KEEPALIVE;
        break;
    case QNativeSocketEngine::MulticastTtlOption:
#ifndef QT_NO_IPV6
        if (socketProtocol == QAbstractSocket::IPv6Protocol) {
            level = IPPROTO_IPV6;
            n = IPV6_MULTICAST_HOPS;
        } else
#endif
        {
            level = IPPROTO_IP;
            n = IP_MULTICAST_TTL;
        }
        break;
    case QNativeSocketEngine::MulticastLoopbackOption:
#ifndef QT_NO_IPV6
        if (socketProtocol == QAbstractSocket::IPv6Protocol) {
            level = IPPROTO_IPV6;
            n = IPV6_MULTICAST_LOOP;
        } else
#endif
        {
            level = IPPROTO_IP;
            n = IP_MULTICAST_LOOP;
        }
        break;
    }

    int v = -1;
    QT_SOCKOPTLEN_T len = sizeof(v);
    if (::getsockopt(socketDescriptor, level, n, (char *) &v, &len) != -1)
        return v;

    return -1;
}


/*
    Sets the socket option \a opt to \a v.
*/
bool QNativeSocketEnginePrivate::setOption(QNativeSocketEngine::SocketOption opt, int v)
{
    Q_Q(QNativeSocketEngine);
    if (!q->isValid())
        return false;

    int n = 0;
    int level = SOL_SOCKET; // default

    switch (opt) {
    case QNativeSocketEngine::ReceiveBufferSocketOption:
        n = SO_RCVBUF;
        break;
    case QNativeSocketEngine::SendBufferSocketOption:
        n = SO_SNDBUF;
        break;
    case QNativeSocketEngine::BroadcastSocketOption:
        n = SO_BROADCAST;
        break;
    case QNativeSocketEngine::NonBlockingSocketOption: {
        // Make the socket nonblocking.
#if !defined(Q_OS_VXWORKS)
        int flags = ::fcntl(socketDescriptor, F_GETFL, 0);
        if (flags == -1) {
#ifdef QNATIVESOCKETENGINE_DEBUG
            perror("QNativeSocketEnginePrivate::setOption(): fcntl(F_GETFL) failed");
#endif
            return false;
        }
        if (::fcntl(socketDescriptor, F_SETFL, flags | O_NONBLOCK) == -1) {
#ifdef QNATIVESOCKETENGINE_DEBUG
            perror("QNativeSocketEnginePrivate::setOption(): fcntl(F_SETFL) failed");
#endif
            return false;
        }
#else // Q_OS_VXWORKS
        int onoff = 1;

        if (qt_safe_ioctl(socketDescriptor, FIONBIO, &onoff) < 0) {

#ifdef QNATIVESOCKETENGINE_DEBUG
            perror("QNativeSocketEnginePrivate::setOption(): ioctl(FIONBIO, 1) failed");
#endif
            return false;
        }
#endif // Q_OS_VXWORKS
        return true;
    }
    case QNativeSocketEngine::AddressReusable:
#if defined(SO_REUSEPORT)
        // on OS X, SO_REUSEADDR isn't sufficient to allow multiple binds to the
        // same port (which is useful for multicast UDP). SO_REUSEPORT is, but
        // we most definitely do not want to use this for TCP. See QTBUG-6305.
        if (socketType == QAbstractSocket::UdpSocket)
            n = SO_REUSEPORT;
        else
            n = SO_REUSEADDR;
#else
        n = SO_REUSEADDR;
#endif
        break;
    case QNativeSocketEngine::BindExclusively:
        return true;
    case QNativeSocketEngine::ReceiveOutOfBandData:
        n = SO_OOBINLINE;
        break;
    case QNativeSocketEngine::LowDelayOption:
        level = IPPROTO_TCP;
        n = TCP_NODELAY;
        break;
    case QNativeSocketEngine::KeepAliveOption:
        n = SO_KEEPALIVE;
        break;
    case QNativeSocketEngine::MulticastTtlOption:
#ifndef QT_NO_IPV6
        if (socketProtocol == QAbstractSocket::IPv6Protocol) {
            level = IPPROTO_IPV6;
            n = IPV6_MULTICAST_HOPS;
        } else
#endif
        {
            level = IPPROTO_IP;
            n = IP_MULTICAST_TTL;
        }
        break;
    case QNativeSocketEngine::MulticastLoopbackOption:
#ifndef QT_NO_IPV6
        if (socketProtocol == QAbstractSocket::IPv6Protocol) {
            level = IPPROTO_IPV6;
            n = IPV6_MULTICAST_LOOP;
        } else
#endif
        {
            level = IPPROTO_IP;
            n = IP_MULTICAST_LOOP;
        }
        break;
    }

    return ::setsockopt(socketDescriptor, level, n, (char *) &v, sizeof(v)) == 0;
}

bool QNativeSocketEnginePrivate::nativeConnect(const QHostAddress &addr, quint16 port)
{
#ifdef QNATIVESOCKETENGINE_DEBUG
    qDebug("QNativeSocketEnginePrivate::nativeConnect() : %d ", socketDescriptor);
#endif

    struct sockaddr_in sockAddrIPv4;
    struct sockaddr *sockAddrPtr = 0;
    QT_SOCKLEN_T sockAddrSize = 0;

#if !defined(QT_NO_IPV6)
    struct sockaddr_in6 sockAddrIPv6;

    if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
        memset(&sockAddrIPv6, 0, sizeof(sockAddrIPv6));
        sockAddrIPv6.sin6_family = AF_INET6;
        sockAddrIPv6.sin6_port = htons(port);

        QString scopeid = addr.scopeId();
        bool ok;
        sockAddrIPv6.sin6_scope_id = scopeid.toInt(&ok);
#ifndef QT_NO_IPV6IFNAME
        if (!ok)
            sockAddrIPv6.sin6_scope_id = ::if_nametoindex(scopeid.toLatin1());
#endif
        Q_IPV6ADDR ip6 = addr.toIPv6Address();
        memcpy(&sockAddrIPv6.sin6_addr.s6_addr, &ip6, sizeof(ip6));

        sockAddrSize = sizeof(sockAddrIPv6);
        sockAddrPtr = (struct sockaddr *) &sockAddrIPv6;
    } else
#if 0
    {}
#endif
#endif
    if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
        memset(&sockAddrIPv4, 0, sizeof(sockAddrIPv4));
        sockAddrIPv4.sin_family = AF_INET;
        sockAddrIPv4.sin_port = htons(port);
        sockAddrIPv4.sin_addr.s_addr = htonl(addr.toIPv4Address());

        sockAddrSize = sizeof(sockAddrIPv4);
        sockAddrPtr = (struct sockaddr *) &sockAddrIPv4;
    } else {
        // unreachable
    }

    int connectResult = qt_safe_connect(socketDescriptor, sockAddrPtr, sockAddrSize);
    if (connectResult == -1) {
        switch (errno) {
        case EISCONN:
            socketState = QAbstractSocket::ConnectedState;
            break;
        case ECONNREFUSED:
        case EINVAL:
            setError(QAbstractSocket::ConnectionRefusedError, ConnectionRefusedErrorString);
            socketState = QAbstractSocket::UnconnectedState;
            break;
        case ETIMEDOUT:
            setError(QAbstractSocket::NetworkError, ConnectionTimeOutErrorString);
            break;
        case EHOSTUNREACH:
            setError(QAbstractSocket::NetworkError, HostUnreachableErrorString);
            socketState = QAbstractSocket::UnconnectedState;
            break;
        case ENETUNREACH:
            setError(QAbstractSocket::NetworkError, NetworkUnreachableErrorString);
            socketState = QAbstractSocket::UnconnectedState;
            break;
        case EADDRINUSE:
            setError(QAbstractSocket::NetworkError, AddressInuseErrorString);
            break;
        case EINPROGRESS:
        case EALREADY:
            setError(QAbstractSocket::UnfinishedSocketOperationError, InvalidSocketErrorString);
            socketState = QAbstractSocket::ConnectingState;
            break;
        case EAGAIN:
            setError(QAbstractSocket::UnfinishedSocketOperationError, InvalidSocketErrorString);
            setError(QAbstractSocket::SocketResourceError, ResourceErrorString);
            break;
        case EACCES:
        case EPERM:
            setError(QAbstractSocket::SocketAccessError, AccessErrorString);
            socketState = QAbstractSocket::UnconnectedState;
            break;
        case EAFNOSUPPORT:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            socketState = QAbstractSocket::UnconnectedState;
        default:
            break;
        }

        if (socketState != QAbstractSocket::ConnectedState) {
#if defined (QNATIVESOCKETENGINE_DEBUG)
            qDebug("QNativeSocketEnginePrivate::nativeConnect(%s, %i) == false (%s)",
                   addr.toString().toLatin1().constData(), port,
                   socketState == QAbstractSocket::ConnectingState
                   ? "Connection in progress" : socketErrorString.toLatin1().constData());
#endif
            return false;
        }
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeConnect(%s, %i) == true",
           addr.toString().toLatin1().constData(), port);
#endif

    socketState = QAbstractSocket::ConnectedState;
    return true;
}

bool QNativeSocketEnginePrivate::nativeBind(const QHostAddress &address, quint16 port)
{
    struct sockaddr_in sockAddrIPv4;
    struct sockaddr *sockAddrPtr = 0;
    QT_SOCKLEN_T sockAddrSize = 0;

#if !defined(QT_NO_IPV6)
    struct sockaddr_in6 sockAddrIPv6;

    if (address.protocol() == QAbstractSocket::IPv6Protocol) {
        memset(&sockAddrIPv6, 0, sizeof(sockAddrIPv6));
        sockAddrIPv6.sin6_family = AF_INET6;
        sockAddrIPv6.sin6_port = htons(port);
#ifndef QT_NO_IPV6IFNAME
        sockAddrIPv6.sin6_scope_id = ::if_nametoindex(address.scopeId().toLatin1().data());
#else
        sockAddrIPv6.sin6_scope_id = address.scopeId().toInt();
#endif
        Q_IPV6ADDR tmp = address.toIPv6Address();
        memcpy(&sockAddrIPv6.sin6_addr.s6_addr, &tmp, sizeof(tmp));
        sockAddrSize = sizeof(sockAddrIPv6);
        sockAddrPtr = (struct sockaddr *) &sockAddrIPv6;
    } else
#endif
        if (address.protocol() == QAbstractSocket::IPv4Protocol) {
            memset(&sockAddrIPv4, 0, sizeof(sockAddrIPv4));
            sockAddrIPv4.sin_family = AF_INET;
            sockAddrIPv4.sin_port = htons(port);
            sockAddrIPv4.sin_addr.s_addr = htonl(address.toIPv4Address());
            sockAddrSize = sizeof(sockAddrIPv4);
            sockAddrPtr = (struct sockaddr *) &sockAddrIPv4;
        } else {
            // unreachable
        }

    int bindResult = QT_SOCKET_BIND(socketDescriptor, sockAddrPtr, sockAddrSize);

    if (bindResult < 0) {
        switch(errno) {
        case EADDRINUSE:
            setError(QAbstractSocket::AddressInUseError, AddressInuseErrorString);
            break;
        case EACCES:
            setError(QAbstractSocket::SocketAccessError, AddressProtectedErrorString);
            break;
        case EINVAL:
            setError(QAbstractSocket::UnsupportedSocketOperationError, OperationUnsupportedErrorString);
            break;
        case EADDRNOTAVAIL:
            setError(QAbstractSocket::SocketAddressNotAvailableError, AddressNotAvailableErrorString);
            break;
        default:
            break;
        }

#if defined (QNATIVESOCKETENGINE_DEBUG)
        qDebug("QNativeSocketEnginePrivate::nativeBind(%s, %i) == false (%s)",
               address.toString().toLatin1().constData(), port, socketErrorString.toLatin1().constData());
#endif

        return false;
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeBind(%s, %i) == true",
           address.toString().toLatin1().constData(), port);
#endif
    socketState = QAbstractSocket::BoundState;
    return true;
}

bool QNativeSocketEnginePrivate::nativeListen(int backlog)
{
    if (qt_safe_listen(socketDescriptor, backlog) < 0) {
        switch (errno) {
        case EADDRINUSE:
            setError(QAbstractSocket::AddressInUseError,
                     PortInuseErrorString);
            break;
        default:
            break;
        }

#if defined (QNATIVESOCKETENGINE_DEBUG)
        qDebug("QNativeSocketEnginePrivate::nativeListen(%i) == false (%s)",
               backlog, socketErrorString.toLatin1().constData());
#endif
        return false;
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeListen(%i) == true", backlog);
#endif

    socketState = QAbstractSocket::ListeningState;
    return true;
}

int QNativeSocketEnginePrivate::nativeAccept()
{
    int acceptedDescriptor = qt_safe_accept(socketDescriptor, 0, 0);

    return acceptedDescriptor;
}

#ifndef QT_NO_NETWORKINTERFACE

static bool multicastMembershipHelper(QNativeSocketEnginePrivate *d,
                                      int how6,
                                      int how4,
                                      const QHostAddress &groupAddress,
                                      const QNetworkInterface &interface)
{
    int level = 0;
    int sockOpt = 0;
    void *sockArg;
    int sockArgSize;

    ip_mreq mreq4;
#ifndef QT_NO_IPV6
    ipv6_mreq mreq6;

    if (groupAddress.protocol() == QAbstractSocket::IPv6Protocol) {
        level = IPPROTO_IPV6;
        sockOpt = how6;
        sockArg = &mreq6;
        sockArgSize = sizeof(mreq6);
        memset(&mreq6, 0, sizeof(mreq6));
        Q_IPV6ADDR ip6 = groupAddress.toIPv6Address();
        memcpy(&mreq6.ipv6mr_multiaddr, &ip6, sizeof(ip6));
        mreq6.ipv6mr_interface = interface.index();
    } else
#endif
    if (groupAddress.protocol() == QAbstractSocket::IPv4Protocol) {
        level = IPPROTO_IP;
        sockOpt = how4;
        sockArg = &mreq4;
        sockArgSize = sizeof(mreq4);
        memset(&mreq4, 0, sizeof(mreq4));
        mreq4.imr_multiaddr.s_addr = htonl(groupAddress.toIPv4Address());

        if (interface.isValid()) {
            QList<QNetworkAddressEntry> addressEntries = interface.addressEntries();
            if (!addressEntries.isEmpty()) {
                QHostAddress firstIP = addressEntries.first().ip();
                mreq4.imr_interface.s_addr = htonl(firstIP.toIPv4Address());
            } else {
                d->setError(QAbstractSocket::NetworkError,
                            QNativeSocketEnginePrivate::NetworkUnreachableErrorString);
                return false;
            }
        } else {
            mreq4.imr_interface.s_addr = INADDR_ANY;
        }
    } else {
        // unreachable
        d->setError(QAbstractSocket::UnsupportedSocketOperationError,
                    QNativeSocketEnginePrivate::ProtocolUnsupportedErrorString);
        return false;
    }

    int res = setsockopt(d->socketDescriptor, level, sockOpt, sockArg, sockArgSize);
    if (res == -1) {
        switch (errno) {
        case ENOPROTOOPT:
            d->setError(QAbstractSocket::UnsupportedSocketOperationError,
                        QNativeSocketEnginePrivate::OperationUnsupportedErrorString);
            break;
        case EADDRNOTAVAIL:
            d->setError(QAbstractSocket::SocketAddressNotAvailableError,
                        QNativeSocketEnginePrivate::AddressNotAvailableErrorString);
            break;
        default:
            d->setError(QAbstractSocket::UnknownSocketError,
                        QNativeSocketEnginePrivate::UnknownSocketErrorString);
            break;
        }
        return false;
    }
    return true;
}

bool QNativeSocketEnginePrivate::nativeJoinMulticastGroup(const QHostAddress &groupAddress,
                                                          const QNetworkInterface &interface)
{
    return multicastMembershipHelper(this,
#ifndef QT_NO_IPV6
                                     IPV6_JOIN_GROUP,
#else
                                     0,
#endif
                                     IP_ADD_MEMBERSHIP,
                                     groupAddress,
                                     interface);
}

bool QNativeSocketEnginePrivate::nativeLeaveMulticastGroup(const QHostAddress &groupAddress,
                                                           const QNetworkInterface &interface)
{
    return multicastMembershipHelper(this,
#ifndef QT_NO_IPV6
                                     IPV6_LEAVE_GROUP,
#else
                                     0,
#endif
                                     IP_DROP_MEMBERSHIP,
                                     groupAddress,
                                     interface);
}

QNetworkInterface QNativeSocketEnginePrivate::nativeMulticastInterface() const
{
#ifndef QT_NO_IPV6
    if (socketProtocol == QAbstractSocket::IPv6Protocol) {
        uint v;
        QT_SOCKOPTLEN_T sizeofv = sizeof(v);
        if (::getsockopt(socketDescriptor, IPPROTO_IPV6, IPV6_MULTICAST_IF, &v, &sizeofv) == -1)
            return QNetworkInterface();
        return QNetworkInterface::interfaceFromIndex(v);
    }
#endif

    struct in_addr v = { 0 };
    QT_SOCKOPTLEN_T sizeofv = sizeof(v);
    if (::getsockopt(socketDescriptor, IPPROTO_IP, IP_MULTICAST_IF, &v, &sizeofv) == -1)
        return QNetworkInterface();
    if (v.s_addr != 0 && sizeofv >= sizeof(v)) {
        QHostAddress ipv4(ntohl(v.s_addr));
        QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
        for (int i = 0; i < ifaces.count(); ++i) {
            const QNetworkInterface &iface = ifaces.at(i);
            QList<QNetworkAddressEntry> entries = iface.addressEntries();
            for (int j = 0; j < entries.count(); ++j) {
                const QNetworkAddressEntry &entry = entries.at(j);
                if (entry.ip() == ipv4)
                    return iface;
            }
        }
    }
    return QNetworkInterface();
}

bool QNativeSocketEnginePrivate::nativeSetMulticastInterface(const QNetworkInterface &iface)
{
#ifndef QT_NO_IPV6
    if (socketProtocol == QAbstractSocket::IPv6Protocol) {
        uint v = iface.index();
        return (::setsockopt(socketDescriptor, IPPROTO_IPV6, IPV6_MULTICAST_IF, &v, sizeof(v)) != -1);
    }
#endif

    struct in_addr v;
    if (iface.isValid()) {
        QList<QNetworkAddressEntry> entries = iface.addressEntries();
        for (int i = 0; i < entries.count(); ++i) {
            const QNetworkAddressEntry &entry = entries.at(i);
            const QHostAddress &ip = entry.ip();
            if (ip.protocol() == QAbstractSocket::IPv4Protocol) {
                v.s_addr = htonl(ip.toIPv4Address());
                int r = ::setsockopt(socketDescriptor, IPPROTO_IP, IP_MULTICAST_IF, &v, sizeof(v));
                if (r != -1)
                    return true;
            }
        }
        return false;
    }

    v.s_addr = INADDR_ANY;
    return (::setsockopt(socketDescriptor, IPPROTO_IP, IP_MULTICAST_IF, &v, sizeof(v)) != -1);
}

#endif // QT_NO_NETWORKINTERFACE

qint64 QNativeSocketEnginePrivate::nativeBytesAvailable() const
{
    int nbytes = 0;
    // gives shorter than true amounts on Unix domain sockets.
    qint64 available = 0;
    if (qt_safe_ioctl(socketDescriptor, FIONREAD, (char *) &nbytes) >= 0)
        available = (qint64) nbytes;

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeBytesAvailable() == %lli", available);
#endif
    return available;
}

bool QNativeSocketEnginePrivate::nativeHasPendingDatagrams() const
{
    // Create a sockaddr struct and reset its port number.
    qt_sockaddr storage;
    QT_SOCKLEN_T storageSize = sizeof(storage);
    memset(&storage, 0, storageSize);

    // Peek 0 bytes into the next message. The size of the message may
    // well be 0, so we can't check recvfrom's return value.
    ssize_t readBytes;
    do {
        char c;
        readBytes = ::recvfrom(socketDescriptor, &c, 1, MSG_PEEK, &storage.a, &storageSize);
    } while (readBytes == -1 && errno == EINTR);

    // If there's no error, or if our buffer was too small, there must be a
    // pending datagram.
    bool result = (readBytes != -1) || errno == EMSGSIZE;

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeHasPendingDatagrams() == %s",
           result ? "true" : "false");
#endif
    return result;
}

qint64 QNativeSocketEnginePrivate::nativePendingDatagramSize() const
{
    QVarLengthArray<char, 8192> udpMessagePeekBuffer(8192);
    ssize_t recvResult = -1;

    for (;;) {
        // the data written to udpMessagePeekBuffer is discarded, so
        // this function is still reentrant although it might not look
        // so.
        recvResult = ::recv(socketDescriptor, udpMessagePeekBuffer.data(),
            udpMessagePeekBuffer.size(), MSG_PEEK);
        if (recvResult == -1 && errno == EINTR)
            continue;

        if (recvResult != (ssize_t) udpMessagePeekBuffer.size())
            break;

        udpMessagePeekBuffer.resize(udpMessagePeekBuffer.size() * 2);
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativePendingDatagramSize() == %i", recvResult);
#endif

    return qint64(recvResult);
}

qint64 QNativeSocketEnginePrivate::nativeReceiveDatagram(char *data, qint64 maxSize,
                                                    QHostAddress *address, quint16 *port)
{
    qt_sockaddr aa;
    memset(&aa, 0, sizeof(aa));
    QT_SOCKLEN_T sz;
    sz = sizeof(aa);

    ssize_t recvFromResult = 0;
    do {
        char c;
        recvFromResult = ::recvfrom(socketDescriptor, maxSize ? data : &c, maxSize ? maxSize : 1,
                                    0, &aa.a, &sz);
    } while (recvFromResult == -1 && errno == EINTR);

    if (recvFromResult == -1) {
        setError(QAbstractSocket::NetworkError, ReceiveDatagramErrorString);
    } else if (port || address) {
        qt_socket_getPortAndAddress(&aa, port, address);
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeReceiveDatagram(%p \"%s\", %lli, %s, %i) == %lli",
           data, qt_prettyDebug(data, qMin(recvFromResult, ssize_t(16)), recvFromResult).data(), maxSize,
           address ? address->toString().toLatin1().constData() : "(nil)",
           port ? *port : 0, (qint64) recvFromResult);
#endif

    return qint64(maxSize ? recvFromResult : recvFromResult == -1 ? -1 : 0);
}

qint64 QNativeSocketEnginePrivate::nativeSendDatagram(const char *data, qint64 len,
                                                   const QHostAddress &host, quint16 port)
{
    struct sockaddr_in sockAddrIPv4;
    struct sockaddr *sockAddrPtr = 0;
    QT_SOCKLEN_T sockAddrSize = 0;

#if !defined(QT_NO_IPV6)
    struct sockaddr_in6 sockAddrIPv6;
    if (host.protocol() == QAbstractSocket::IPv6Protocol) {
        memset(&sockAddrIPv6, 0, sizeof(sockAddrIPv6));
        sockAddrIPv6.sin6_family = AF_INET6;
        sockAddrIPv6.sin6_port = htons(port);

        Q_IPV6ADDR tmp = host.toIPv6Address();
        memcpy(&sockAddrIPv6.sin6_addr.s6_addr, &tmp, sizeof(tmp));
        QString scopeid = host.scopeId();
        bool ok;
        sockAddrIPv6.sin6_scope_id = scopeid.toInt(&ok);
#ifndef QT_NO_IPV6IFNAME
        if (!ok)
            sockAddrIPv6.sin6_scope_id = ::if_nametoindex(scopeid.toLatin1());
#endif
        sockAddrSize = sizeof(sockAddrIPv6);
        sockAddrPtr = (struct sockaddr *)&sockAddrIPv6;
    } else
#endif
    if (host.protocol() == QAbstractSocket::IPv4Protocol) {
        memset(&sockAddrIPv4, 0, sizeof(sockAddrIPv4));
        sockAddrIPv4.sin_family = AF_INET;
        sockAddrIPv4.sin_port = htons(port);
        sockAddrIPv4.sin_addr.s_addr = htonl(host.toIPv4Address());
        sockAddrSize = sizeof(sockAddrIPv4);
        sockAddrPtr = (struct sockaddr *)&sockAddrIPv4;
    }

    // ignore the SIGPIPE signal
    qt_ignore_sigpipe();
    ssize_t sentBytes = qt_safe_sendto(socketDescriptor, data, len,
                                       0, sockAddrPtr, sockAddrSize);

    if (sentBytes < 0) {
        switch (errno) {
        case EMSGSIZE:
            setError(QAbstractSocket::DatagramTooLargeError, DatagramTooLargeErrorString);
            break;
        default:
            setError(QAbstractSocket::NetworkError, SendDatagramErrorString);
        }
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEngine::sendDatagram(%p \"%s\", %lli, \"%s\", %i) == %lli", data,
           qt_prettyDebug(data, qMin<int>(len, 16), len).data(), len, host.toString().toLatin1().constData(),
           port, (qint64) sentBytes);
#endif

    return qint64(sentBytes);
}

bool QNativeSocketEnginePrivate::fetchConnectionParameters()
{
    localPort = 0;
    localAddress.clear();
    peerPort = 0;
    peerAddress.clear();

    if (socketDescriptor == -1)
        return false;

    qt_sockaddr sa;
    QT_SOCKLEN_T sockAddrSize = sizeof(sa);

    // Determine local address
    memset(&sa, 0, sizeof(sa));
    if (::getsockname(socketDescriptor, &sa.a, &sockAddrSize) == 0) {
        qt_socket_getPortAndAddress(&sa, &localPort, &localAddress);

        // Determine protocol family
        switch (sa.a.sa_family) {
        case AF_INET:
            socketProtocol = QAbstractSocket::IPv4Protocol;
            break;
#if !defined (QT_NO_IPV6)
        case AF_INET6:
            socketProtocol = QAbstractSocket::IPv6Protocol;
            break;
#endif
        default:
            socketProtocol = QAbstractSocket::UnknownNetworkLayerProtocol;
            break;
        }

    } else if (errno == EBADF) {
        setError(QAbstractSocket::UnsupportedSocketOperationError, InvalidSocketErrorString);
        return false;
    }

    // Determine the remote address
    if (!::getpeername(socketDescriptor, &sa.a, &sockAddrSize))
        qt_socket_getPortAndAddress(&sa, &peerPort, &peerAddress);

    // Determine the socket type (UDP/TCP)
    int value = 0;
    QT_SOCKOPTLEN_T valueSize = sizeof(int);
    if (::getsockopt(socketDescriptor, SOL_SOCKET, SO_TYPE, &value, &valueSize) == 0) {
        if (value == SOCK_STREAM)
            socketType = QAbstractSocket::TcpSocket;
        else if (value == SOCK_DGRAM)
            socketType = QAbstractSocket::UdpSocket;
        else
            socketType = QAbstractSocket::UnknownSocketType;
    }
#if defined (QNATIVESOCKETENGINE_DEBUG)
    QString socketProtocolStr = "UnknownProtocol";
    if (socketProtocol == QAbstractSocket::IPv4Protocol) socketProtocolStr = "IPv4Protocol";
    else if (socketProtocol == QAbstractSocket::IPv6Protocol) socketProtocolStr = "IPv6Protocol";

    QString socketTypeStr = "UnknownSocketType";
    if (socketType == QAbstractSocket::TcpSocket) socketTypeStr = "TcpSocket";
    else if (socketType == QAbstractSocket::UdpSocket) socketTypeStr = "UdpSocket";

    qDebug("QNativeSocketEnginePrivate::fetchConnectionParameters() local == %s:%i,"
           " peer == %s:%i, socket == %s - %s",
           localAddress.toString().toLatin1().constData(), localPort,
           peerAddress.toString().toLatin1().constData(), peerPort,socketTypeStr.toLatin1().constData(),
           socketProtocolStr.toLatin1().constData());
#endif
    return true;
}

void QNativeSocketEnginePrivate::nativeClose()
{
#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEngine::nativeClose()");
#endif

    qt_safe_close(socketDescriptor);
}

qint64 QNativeSocketEnginePrivate::nativeWrite(const char *data, qint64 len)
{
    Q_Q(QNativeSocketEngine);

    // ignore the SIGPIPE signal
    qt_ignore_sigpipe();

    ssize_t writtenBytes;
    writtenBytes = qt_safe_write(socketDescriptor, data, len);

    if (writtenBytes < 0) {
        switch (errno) {
        case EPIPE:
        case ECONNRESET:
            writtenBytes = -1;
            setError(QAbstractSocket::RemoteHostClosedError, RemoteHostClosedErrorString);
            q->close();
            break;
        case EAGAIN:
            writtenBytes = 0;
            break;
        case EMSGSIZE:
            setError(QAbstractSocket::DatagramTooLargeError, DatagramTooLargeErrorString);
            break;
        default:
            break;
        }
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeWrite(%p \"%s\", %llu) == %i",
           data, qt_prettyDebug(data, qMin((int) len, 16),
                                (int) len).data(), len, (int) writtenBytes);
#endif

    return qint64(writtenBytes);
}
/*
*/
qint64 QNativeSocketEnginePrivate::nativeRead(char *data, qint64 maxSize)
{
    Q_Q(QNativeSocketEngine);
    if (!q->isValid()) {
        qWarning("QNativeSocketEngine::nativeRead: Invalid socket");
        return -1;
    }

    ssize_t r = 0;
    r = qt_safe_read(socketDescriptor, data, maxSize);

    if (r < 0) {
        r = -1;
        switch (errno) {
#if EWOULDBLOCK-0 && EWOULDBLOCK != EAGAIN
        case EWOULDBLOCK:
#endif
        case EAGAIN:
            // No data was available for reading
            r = -2;
            break;
        case EBADF:
        case EINVAL:
        case EIO:
            //error string is now set in read(), not here in nativeRead()
            break;
        case ECONNRESET:
#if defined(Q_OS_VXWORKS)
        case ESHUTDOWN:
#endif
            r = 0;
            break;
        default:
            break;
        }
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QNativeSocketEnginePrivate::nativeRead(%p \"%s\", %llu) == %i",
           data, qt_prettyDebug(data, qMin(r, ssize_t(16)), r).data(),
           maxSize, r);
#endif

    return qint64(r);
}

int QNativeSocketEnginePrivate::nativeSelect(int timeout, bool selectForRead) const
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(socketDescriptor, &fds);

    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    int retval;
    if (selectForRead)
        retval = qt_safe_select(socketDescriptor + 1, &fds, 0, 0, timeout < 0 ? 0 : &tv);
    else
        retval = qt_safe_select(socketDescriptor + 1, 0, &fds, 0, timeout < 0 ? 0 : &tv);

    return retval;
}

int QNativeSocketEnginePrivate::nativeSelect(int timeout, bool checkRead, bool checkWrite,
                       bool *selectForRead, bool *selectForWrite) const
{
    fd_set fdread;
    FD_ZERO(&fdread);
    if (checkRead)
        FD_SET(socketDescriptor, &fdread);

    fd_set fdwrite;
    FD_ZERO(&fdwrite);
    if (checkWrite)
        FD_SET(socketDescriptor, &fdwrite);

    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    int ret;
    ret = qt_safe_select(socketDescriptor + 1, &fdread, &fdwrite, 0, timeout < 0 ? 0 : &tv);

    if (ret <= 0)
        return ret;
    *selectForRead = FD_ISSET(socketDescriptor, &fdread);
    *selectForWrite = FD_ISSET(socketDescriptor, &fdwrite);

    return ret;
}

QT_END_NAMESPACE
