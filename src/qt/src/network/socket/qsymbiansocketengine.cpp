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

//#define QNATIVESOCKETENGINE_DEBUG
#include "qsymbiansocketengine_p.h"

#include "qiodevice.h"
#include "qhostaddress.h"
#include "qelapsedtimer.h"
#include "qvarlengtharray.h"
#include "qnetworkinterface.h"
#include <private/qnetworksession_p.h>
#include <es_sock.h>
#include <in_sock.h>
#include <net/if.h>

#include <private/qcore_symbian_p.h>

#if !defined(QT_NO_NETWORKPROXY)
# include "qnetworkproxy.h"
# include "qabstractsocket.h"
# include "qtcpserver.h"
#endif

#include <QCoreApplication>

#include <qabstracteventdispatcher.h>
#include <private/qeventdispatcher_symbian_p.h>
#include <qsocketnotifier.h>
#include <qnetworkinterface.h>

#include <private/qthread_p.h>
#include <private/qobject_p.h>
#include <private/qsystemerror_p.h>

#if defined QNATIVESOCKETENGINE_DEBUG
#include <qstring.h>
#include <ctype.h>
#endif

QT_BEGIN_NAMESPACE

#define Q_VOID
// Common constructs
#define Q_CHECK_VALID_SOCKETLAYER(function, returnValue) do { \
    if (!isValid()) { \
        qWarning(""#function" was called on an uninitialized socket device"); \
        return returnValue; \
    } } while (0)
#define Q_CHECK_INVALID_SOCKETLAYER(function, returnValue) do { \
    if (isValid()) { \
        qWarning(""#function" was called on an already initialized socket device"); \
        return returnValue; \
    } } while (0)
#define Q_CHECK_STATE(function, checkState, returnValue) do { \
    if (d->socketState != (checkState)) { \
        qWarning(""#function" was not called in "#checkState); \
        return (returnValue); \
    } } while (0)
#define Q_CHECK_NOT_STATE(function, checkState, returnValue) do { \
    if (d->socketState == (checkState)) { \
        qWarning(""#function" was called in "#checkState); \
        return (returnValue); \
    } } while (0)
#define Q_CHECK_STATES(function, state1, state2, returnValue) do { \
    if (d->socketState != (state1) && d->socketState != (state2)) { \
        qWarning(""#function" was called" \
                 " not in "#state1" or "#state2); \
        return (returnValue); \
    } } while (0)
#define Q_CHECK_TYPE(function, type, returnValue) do { \
    if (d->socketType != (type)) { \
        qWarning(#function" was called by a" \
                 " socket other than "#type""); \
        return (returnValue); \
    } } while (0)

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

void QSymbianSocketEnginePrivate::getPortAndAddress(const TInetAddr& a, quint16 *port, QHostAddress *addr)
{
    if (a.Family() == KAfInet6 && !a.IsV4Compat() && !a.IsV4Mapped()) {
        Q_IPV6ADDR tmp;
        memcpy(&tmp, a.Ip6Address().u.iAddr8, sizeof(tmp));
        if (addr) {
            QHostAddress tmpAddress;
            tmpAddress.setAddress(tmp);
            *addr = tmpAddress;
            TPckgBuf<TSoInetIfQuery> query;
            query().iSrcAddr = a;
            TInt err = nativeSocket.GetOpt(KSoInetIfQueryBySrcAddr, KSolInetIfQuery, query);
            if (!err)
                addr->setScopeId(qt_TDesC2QString(query().iName));
            else
            addr->setScopeId(QString::number(a.Scope()));
        }
        if (port)
            *port = a.Port();
        return;
    }
    if (port)
        *port = a.Port();
    if (addr) {
        QHostAddress tmpAddress;
        tmpAddress.setAddress(a.Address());
        *addr = tmpAddress;
    }
}
/*! \internal

    Creates and returns a new socket descriptor of type \a socketType
    and \a socketProtocol.  Returns -1 on failure.
*/
bool QSymbianSocketEnginePrivate::createNewSocket(QAbstractSocket::SocketType socketType,
                                         QAbstractSocket::NetworkLayerProtocol socketProtocol)
{
    Q_Q(QSymbianSocketEngine);
    TUint family = KAfInet; // KAfInet6 is only used as an address family, not as a protocol family
    TUint type = (socketType == QAbstractSocket::UdpSocket) ? KSockDatagram : KSockStream;
    TUint protocol = (socketType == QAbstractSocket::UdpSocket) ? KProtocolInetUdp : KProtocolInetTcp;

    //Check if there is a user specified session
    QVariant v(q->property("_q_networksession"));
    TInt err;
    if (v.isValid()) {
        QSharedPointer<QNetworkSession> s = qvariant_cast<QSharedPointer<QNetworkSession> >(v);
        err = QNetworkSessionPrivate::nativeOpenSocket(*s, nativeSocket, family, type, protocol);
#ifdef QNATIVESOCKETENGINE_DEBUG
        qDebug() << "QSymbianSocketEnginePrivate::createNewSocket - _q_networksession was set" << err;
#endif
    } else {
#ifdef QNATIVESOCKETENGINE_DEBUG
        qDebug() << "QSymbianSocketEnginePrivate::createNewSocket - _q_networksession was not set, using implicit connection";
#endif
        // using implicit connection allows localhost connections without starting any RConnection, see QTBUG-16155 and QTBUG-16843
        // when a remote address is used, socket server will start the system default connection if there is no route.
        err = nativeSocket.Open(socketServer, family, type, protocol);
    }

    if (err != KErrNone) {
        switch (err) {
        case KErrNotSupported:
        case KErrNotFound:
            setError(QAbstractSocket::UnsupportedSocketOperationError,
                ProtocolUnsupportedErrorString);
            break;
        default:
            setError(err);
            break;
        }

        return false;
    }
#ifdef QNATIVESOCKETENGINE_DEBUG
    qDebug() << "QSymbianSocketEnginePrivate::createNewSocket - created" << nativeSocket.SubSessionHandle();
#endif
    socketDescriptor = QSymbianSocketManager::instance().addSocket(nativeSocket);
#ifdef QNATIVESOCKETENGINE_DEBUG
    qDebug() << " - allocated socket descriptor" << socketDescriptor;
#endif
    return true;
}

void QSymbianSocketEnginePrivate::setPortAndAddress(TInetAddr& nativeAddr, quint16 port, const QHostAddress &addr)
{
    nativeAddr.SetPort(port);
    if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
        TPckgBuf<TSoInetIfQuery> query;
        query().iName = qt_QString2TPtrC(addr.scopeId());
        TInt err = nativeSocket.GetOpt(KSoInetIfQueryByName, KSolInetIfQuery, query);
        if (!err)
            nativeAddr.SetScope(query().iIndex);
        else
            nativeAddr.SetScope(0);
        Q_IPV6ADDR ip6 = addr.toIPv6Address();
        TIp6Addr v6addr;
        memcpy(v6addr.u.iAddr8, ip6.c, 16);
        nativeAddr.SetAddress(v6addr);
    } else if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
        nativeAddr.SetAddress(addr.toIPv4Address());
    } else {
        qWarning("unsupported network protocol (%d)", addr.protocol());
    }
}

QSymbianSocketEnginePrivate::QSymbianSocketEnginePrivate() :
    socketDescriptor(-1),
    socketServer(QSymbianSocketManager::instance().getSocketServer()),
    readNotificationsEnabled(false),
    writeNotificationsEnabled(false),
    exceptNotificationsEnabled(false),
    asyncSelect(0),
    hasReceivedBufferedDatagram(false)
{
}

QSymbianSocketEnginePrivate::~QSymbianSocketEnginePrivate()
{
    selectTimer.Close();
}


QSymbianSocketEngine::QSymbianSocketEngine(QObject *parent)
    : QAbstractSocketEngine(*new QSymbianSocketEnginePrivate(), parent)
{
}


QSymbianSocketEngine::~QSymbianSocketEngine()
{
    close();
}

/*!
    Initializes a QSymbianSocketEngine by creating a new socket of type \a
    socketType and network layer protocol \a protocol. Returns true on
    success; otherwise returns false.

    If the socket was already initialized, this function closes the
    socket before reeinitializing it.

    The new socket is non-blocking, and for UDP sockets it's also
    broadcast enabled.
*/
bool QSymbianSocketEngine::initialize(QAbstractSocket::SocketType socketType, QAbstractSocket::NetworkLayerProtocol protocol)
{
    Q_D(QSymbianSocketEngine);
    if (isValid())
        close();

    // Create the socket
    if (!d->createNewSocket(socketType, protocol)) {
#if defined (QNATIVESOCKETENGINE_DEBUG)
        QString typeStr = QLatin1String("UnknownSocketType");
        if (socketType == QAbstractSocket::TcpSocket) typeStr = QLatin1String("TcpSocket");
        else if (socketType == QAbstractSocket::UdpSocket) typeStr = QLatin1String("UdpSocket");
        QString protocolStr = QLatin1String("UnknownProtocol");
        if (protocol == QAbstractSocket::IPv4Protocol) protocolStr = QLatin1String("IPv4Protocol");
        else if (protocol == QAbstractSocket::IPv6Protocol) protocolStr = QLatin1String("IPv6Protocol");
        qDebug("QSymbianSocketEngine::initialize(type == %s, protocol == %s) failed: %s",
               typeStr.toLatin1().constData(), protocolStr.toLatin1().constData(), d->socketErrorString.toLatin1().constData());
#endif
        return false;
    }

    // Make the socket nonblocking.
    if (!setOption(NonBlockingSocketOption, 1)) {
        d->setError(QAbstractSocket::UnsupportedSocketOperationError,
                    d->NonBlockingInitFailedErrorString);
        close();
        return false;
    }

    // Set the broadcasting flag if it's a UDP socket.
    if (socketType == QAbstractSocket::UdpSocket
        && !setOption(BroadcastSocketOption, 1)) {
        d->setError(QAbstractSocket::UnsupportedSocketOperationError,
                    d->BroadcastingInitFailedErrorString);
        close();
        return false;
    }


    // Make sure we receive out-of-band data
    if (socketType == QAbstractSocket::TcpSocket
        && !setOption(ReceiveOutOfBandData, 1)) {
        qWarning("QSymbianSocketEngine::initialize unable to inline out-of-band data");
    }


    d->socketType = socketType;
    d->socketProtocol = protocol;
    return true;
}

/*! \overload

    Initializes the socket using \a socketDescriptor instead of
    creating a new one. The socket type and network layer protocol are
    determined automatically. The socket's state is set to \a
    socketState.

    If the socket type is either TCP or UDP, it is made non-blocking.
    UDP sockets are also broadcast enabled.
 */
bool QSymbianSocketEngine::initialize(int socketDescriptor, QAbstractSocket::SocketState socketState)
{
    Q_D(QSymbianSocketEngine);

    if (isValid())
        close();

    if (!QSymbianSocketManager::instance().lookupSocket(socketDescriptor, d->nativeSocket)) {
        qWarning("QSymbianSocketEngine::initialize - socket descriptor not found");
        d->setError(QAbstractSocket::UnsupportedSocketOperationError,
            QSymbianSocketEnginePrivate::InvalidSocketErrorString);
        return false;
    }
#ifdef QNATIVESOCKETENGINE_DEBUG
    qDebug() << "QSymbianSocketEngine::initialize - attached to" << d->nativeSocket.SubSessionHandle() << socketDescriptor;
#endif
    Q_ASSERT(d->socketDescriptor == socketDescriptor || d->socketDescriptor == -1);
    d->socketDescriptor = socketDescriptor;

    // determine socket type and protocol
    if (!d->fetchConnectionParameters()) {
#if defined (QNATIVESOCKETENGINE_DEBUG)
        qDebug("QSymbianSocketEngine::initialize(socketDescriptor == %i) failed: %s",
               socketDescriptor, d->socketErrorString.toLatin1().constData());
#endif
        d->socketDescriptor = -1;
        return false;
    }

    if (d->socketType != QAbstractSocket::UnknownSocketType) {
        // Make the socket nonblocking.
        if (!setOption(NonBlockingSocketOption, 1)) {
            d->setError(QAbstractSocket::UnsupportedSocketOperationError,
                d->NonBlockingInitFailedErrorString);
            close();
            return false;
        }

        // Set the broadcasting flag if it's a UDP socket.
        if (d->socketType == QAbstractSocket::UdpSocket
            && !setOption(BroadcastSocketOption, 1)) {
            d->setError(QAbstractSocket::UnsupportedSocketOperationError,
                d->BroadcastingInitFailedErrorString);
            close();
            return false;
        }

        // Make sure we receive out-of-band data
        if (d->socketType == QAbstractSocket::TcpSocket
            && !setOption(ReceiveOutOfBandData, 1)) {
            qWarning("QSymbianSocketEngine::initialize unable to inline out-of-band data");
        }
    }

    d->socketState = socketState;
    return true;
}

/*!
    Returns true if the socket is valid; otherwise returns false. A
    socket is valid if it has not been successfully initialized, or if
    it has been closed.
*/
bool QSymbianSocketEngine::isValid() const
{
    Q_D(const QSymbianSocketEngine);
    return d->socketDescriptor != -1;
}


/*!
    Returns the native socket descriptor. Any use of this descriptor
    stands the risk of being non-portable.
*/
int QSymbianSocketEngine::socketDescriptor() const
{
    Q_D(const QSymbianSocketEngine);
    return d->socketDescriptor;
}

/*
    Sets the socket option \a opt to \a v.
*/
bool QSymbianSocketEngine::setOption(QAbstractSocketEngine::SocketOption opt, int v)
{
    Q_D(QSymbianSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::setOption(), false);

    TUint n = 0;
    TUint level = KSOLSocket; // default

    if (!QSymbianSocketEnginePrivate::translateSocketOption(opt, n, level))
        return false;

    if (!level && !n)
        return true;

    return (KErrNone == d->nativeSocket.SetOpt(n, level, v));
}

/*
    Returns the value of the socket option \a opt.
*/
int QSymbianSocketEngine::option(QAbstractSocketEngine::SocketOption opt) const
{
    Q_D(const QSymbianSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::option(), -1);

    TUint n;
    TUint level = KSOLSocket; // default

    if (!QSymbianSocketEnginePrivate::translateSocketOption(opt, n, level))
        return false;

    if (!level && !n)
        return 1;

    int v = -1;
    //GetOpt() is non const
    TInt err = d->nativeSocket.GetOpt(n, level, v);
    if (!err)
        return v;

    return -1;
}

bool QSymbianSocketEnginePrivate::translateSocketOption(QAbstractSocketEngine::SocketOption opt, TUint &n, TUint &level)
{

    switch (opt) {
    case QAbstractSocketEngine::ReceiveBufferSocketOption:
        n = KSORecvBuf;
        break;
    case QAbstractSocketEngine::SendBufferSocketOption:
        n = KSOSendBuf;
        break;
    case QAbstractSocketEngine::NonBlockingSocketOption:
        n = KSONonBlockingIO;
        break;
    case QAbstractSocketEngine::AddressReusable:
        level = KSolInetIp;
        n = KSoReuseAddr;
        break;
    case QAbstractSocketEngine::BroadcastSocketOption:
    case QAbstractSocketEngine::BindExclusively:
        level = 0;
        n = 0;
        return true;
    case QAbstractSocketEngine::ReceiveOutOfBandData:
        level = KSolInetTcp;
        n = KSoTcpOobInline;
        break;
    case QAbstractSocketEngine::LowDelayOption:
        level = KSolInetTcp;
        n = KSoTcpNoDelay;
        break;
    case QAbstractSocketEngine::KeepAliveOption:
        level = KSolInetTcp;
        n = KSoTcpKeepAlive;
        break;
    case QAbstractSocketEngine::MulticastLoopbackOption:
        level = KSolInetIp;
        n = KSoIp6MulticastLoop;
        break;
    case QAbstractSocketEngine::MulticastTtlOption:
        level = KSolInetIp;
        n = KSoIp6MulticastHops;
        break;
    default:
        return false;
    }
    return true;
}

qint64 QSymbianSocketEngine::receiveBufferSize() const
{
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::receiveBufferSize(), -1);
    return option(ReceiveBufferSocketOption);
}

void QSymbianSocketEngine::setReceiveBufferSize(qint64 size)
{
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::setReceiveBufferSize(), Q_VOID);
    setOption(ReceiveBufferSocketOption, size);
}

qint64 QSymbianSocketEngine::sendBufferSize() const
{
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::setSendBufferSize(), -1);
    return option(SendBufferSocketOption);
}

void QSymbianSocketEngine::setSendBufferSize(qint64 size)
{
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::setSendBufferSize(), Q_VOID);
    setOption(SendBufferSocketOption, size);
}

/*!
    Connects to the remote host name given by \a name on port \a
    port. When this function is called, the upper-level will not
    perform a hostname lookup.

    The native socket engine does not support this operation,
    but some other socket engines (notably proxy-based ones) do.
*/
bool QSymbianSocketEngine::connectToHostByName(const QString &name, quint16 port)
{
    Q_UNUSED(name);
    Q_UNUSED(port);
    Q_D(QSymbianSocketEngine);
    d->setError(QAbstractSocket::UnsupportedSocketOperationError,
        QSymbianSocketEnginePrivate::OperationUnsupportedErrorString);
    return false;
}

/*!
    If there's a connection activity on the socket, process it. Then
    notify our parent if there really was activity.
*/
void QSymbianSocketEngine::connectionComplete()
{
    Q_D(QSymbianSocketEngine);
    Q_ASSERT(state() == QAbstractSocket::ConnectingState);

    // as it was a non blocking connect, call again to find the result.
    connectToHost(d->peerAddress, d->peerPort);
    if (state() != QAbstractSocket::ConnectingState) {
        // we changed states
        QAbstractSocketEngine::connectionNotification();
    }
}


bool QSymbianSocketEngine::connectToHost(const QHostAddress &addr, quint16 port)
{
    Q_D(QSymbianSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::connectToHost(), false);

#ifdef QNATIVESOCKETENGINE_DEBUG
    qDebug("QSymbianSocketEngine::connectToHost() : %d ", d->socketDescriptor);
#endif

    if (!d->checkProxy(addr))
        return false;

    d->peerAddress = addr;
    d->peerPort = port;

    TInetAddr nativeAddr;
    d->setPortAndAddress(nativeAddr, port, addr);
    TRequestStatus status;
    d->nativeSocket.Connect(nativeAddr, status);
    User::WaitForRequest(status);
    TInt err = status.Int();
    //For non blocking connect, KErrAlreadyExists is returned from the second Connect() to indicate
    //the connection is up. So treat this the same as KErrNone which would be returned from the first
    //call if it wouldn't block. (e.g. winsock wrapper in the emulator ignores the nonblocking flag)
    if (err && err != KErrAlreadyExists) {
        switch (err) {
        case KErrWouldBlock:
            d->socketState = QAbstractSocket::ConnectingState;
            break;
        default:
            d->setError(err);
            d->socketState = QAbstractSocket::UnconnectedState;
            break;
        }

        if (d->socketState != QAbstractSocket::ConnectedState) {
#if defined (QNATIVESOCKETENGINE_DEBUG)
            qDebug("QSymbianSocketEngine::connectToHost(%s, %i) == false (%s)",
                   addr.toString().toLatin1().constData(), port,
                   d->socketState == QAbstractSocket::ConnectingState
                   ? "Connection in progress" : d->socketErrorString.toLatin1().constData());
#endif
            return false;
        }
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QSymbianSocketEngine::Connect(%s, %i) == true",
           addr.toString().toLatin1().constData(), port);
#endif

    d->socketState = QAbstractSocket::ConnectedState;
    d->fetchConnectionParameters();
    return true;
}

bool QSymbianSocketEngine::bind(const QHostAddress &address, quint16 port)
{
    Q_D(QSymbianSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::bind(), false);

    if (!d->checkProxy(address))
        return false;

    Q_CHECK_STATE(QSymbianSocketEngine::bind(), QAbstractSocket::UnconnectedState, false);

    TInetAddr nativeAddr;
    if (address == QHostAddress::Any || address == QHostAddress::AnyIPv6) {
        //Should allow both IPv4 and IPv6
        //Listening on "0.0.0.0" accepts ONLY ipv4 connections
        //Listening on "::" accepts ONLY ipv6 connections
        nativeAddr.SetFamily(KAFUnspec);
        nativeAddr.SetPort(port);
    } else {
        d->setPortAndAddress(nativeAddr, port, address);
    }

    TInt err = d->nativeSocket.Bind(nativeAddr);
#ifdef __WINS__
    if (err == KErrArgument) // winsock prt returns wrong error code
        err = KErrInUse;
#endif

    if (err) {
        switch (err) {
        case KErrNotFound:
            // the specified interface was not found - use the error code expected
            d->setError(QAbstractSocket::SocketAddressNotAvailableError, QSymbianSocketEnginePrivate::AddressNotAvailableErrorString);
            break;
        default:
            d->setError(err);
            break;
        }

#if defined (QNATIVESOCKETENGINE_DEBUG)
        qDebug("QSymbianSocketEngine::bind(%s, %i) == false (%s)",
               address.toString().toLatin1().constData(), port, d->socketErrorString.toLatin1().constData());
#endif

        return false;
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QSymbianSocketEngine::bind(%s, %i) == true",
           address.toString().toLatin1().constData(), port);
#endif
    d->socketState = QAbstractSocket::BoundState;

    d->fetchConnectionParameters();

    // When we bind to unspecified address (to get a dual mode socket), report back the
    // same type of address that was requested. This is required for SOCKS proxy to work.
    if (nativeAddr.Family() == KAFUnspec)
        d->localAddress = address;
    return true;
}

bool QSymbianSocketEngine::listen()
{
    Q_D(QSymbianSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::listen(), false);
    Q_CHECK_STATE(QSymbianSocketEngine::listen(), QAbstractSocket::BoundState, false);
    Q_CHECK_TYPE(QSymbianSocketEngine::listen(), QAbstractSocket::TcpSocket, false);
    TInt err = d->nativeSocket.Listen(50);
    if (err) {
        d->setError(err);

#if defined (QNATIVESOCKETENGINE_DEBUG)
        qDebug("QSymbianSocketEngine::listen() == false (%s)",
               d->socketErrorString.toLatin1().constData());
#endif
        return false;
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QSymbianSocketEngine::listen() == true");
#endif

    d->socketState = QAbstractSocket::ListeningState;
    return true;
}

int QSymbianSocketEngine::accept()
{
    Q_D(QSymbianSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::accept(), -1);
    Q_CHECK_STATE(QSymbianSocketEngine::accept(), QAbstractSocket::ListeningState, false);
    Q_CHECK_TYPE(QSymbianSocketEngine::accept(), QAbstractSocket::TcpSocket, false);
    RSocket blankSocket;
    blankSocket.Open(d->socketServer);
    TRequestStatus status;
    d->nativeSocket.Accept(blankSocket, status);
    User::WaitForRequest(status);
    if (status.Int()) {
        blankSocket.Close();
        if (status != KErrWouldBlock)
            qWarning("QSymbianSocketEngine::accept() - error %d", status.Int());
        return -1;
    }

#ifdef QNATIVESOCKETENGINE_DEBUG
    qDebug() << "QSymbianSocketEnginePrivate::accept - created" << blankSocket.SubSessionHandle();
#endif
    int fd = QSymbianSocketManager::instance().addSocket(blankSocket);
#ifdef QNATIVESOCKETENGINE_DEBUG
    qDebug() << " - allocated socket descriptor" << fd;
#endif
    return fd;
}

qint64 QSymbianSocketEngine::bytesAvailable() const
{
    Q_D(const QSymbianSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::bytesAvailable(), -1);
    Q_CHECK_NOT_STATE(QSymbianSocketEngine::bytesAvailable(), QAbstractSocket::UnconnectedState, false);
    int nbytes = 0;
    qint64 available = 0;
    TInt err = d->nativeSocket.GetOpt(KSOReadBytesPending, KSOLSocket, nbytes);
    if (err)
        return 0;
    available = (qint64) nbytes;

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QSymbianSocketEngine::bytesAvailable() == %lli", available);
#endif
    return available;
}

bool QSymbianSocketEngine::hasPendingDatagrams() const
{
    Q_D(const QSymbianSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::hasPendingDatagrams(), false);
    Q_CHECK_NOT_STATE(QSymbianSocketEngine::hasPendingDatagrams(), QAbstractSocket::UnconnectedState, false);
    Q_CHECK_TYPE(QSymbianSocketEngine::hasPendingDatagrams(), QAbstractSocket::UdpSocket, false);
    int nbytes;
    TInt err = d->nativeSocket.GetOpt(KSOReadBytesPending,KSOLSocket, nbytes);
    return err == KErrNone && nbytes > 0;
}

qint64 QSymbianSocketEngine::pendingDatagramSize() const
{
    Q_D(const QSymbianSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::pendingDatagramSize(), false);
    Q_CHECK_TYPE(QSymbianSocketEngine::hasPendingDatagrams(), QAbstractSocket::UdpSocket, false);
    //can only buffer one datagram at a time
    if (d->hasReceivedBufferedDatagram)
        return d->receivedDataBuffer.size();
    int nbytes = 0;
    TInt err = d->nativeSocket.GetOpt(KSOReadBytesPending,KSOLSocket, nbytes);
    if (nbytes > 0) {
        //nbytes includes IP header, which is of variable length (IPv4 with or without options, IPv6...)
        //therefore read the datagram into a buffer to find its true size
        d->receivedDataBuffer.resize(nbytes);
        TPtr8 buffer((TUint8*)d->receivedDataBuffer.data(), nbytes);
        //nbytes = size including IP header, buffer is a pointer descriptor backed by the receivedDataBuffer
        TInetAddr addr;
        TRequestStatus status;
        //RecvFrom copies only the payload (we don't want the header so don't specify the option to retrieve it)
        d->nativeSocket.RecvFrom(buffer, addr, 0, status);
        User::WaitForRequest(status);
        if (status != KErrNone) {
            d->receivedDataBuffer.clear();
            return 0;
        }
        nbytes = buffer.Length();
        //nbytes = size of payload, resize the receivedDataBuffer to the final size
        d->receivedDataBuffer.resize(nbytes);
        d->hasReceivedBufferedDatagram = true;
        //now receivedDataBuffer contains one datagram, which has been removed from the socket's internal buffer
#if defined (QNATIVESOCKETENGINE_DEBUG)
        qDebug() << "QSymbianSocketEngine::pendingDatagramSize buffering" << nbytes << "bytes";
#endif
    }
    return qint64(nbytes);
}


qint64 QSymbianSocketEngine::readDatagram(char *data, qint64 maxSize,
                                                    QHostAddress *address, quint16 *port)
{
    Q_D(QSymbianSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::readDatagram(), -1);
    Q_CHECK_TYPE(QSymbianSocketEngine::readDatagram(), QAbstractSocket::UdpSocket, false);

    // if a datagram was buffered in pendingDatagramSize(), return it now
    if (d->hasReceivedBufferedDatagram) {
        qint64 size = qMin(maxSize, (qint64)d->receivedDataBuffer.size());
        memcpy(data, d->receivedDataBuffer.constData(), size);
        d->receivedDataBuffer.clear();
        d->hasReceivedBufferedDatagram = false;
#if defined (QNATIVESOCKETENGINE_DEBUG)
        qDebug() << "QSymbianSocketEngine::readDatagram returning" << size << "bytes from buffer";
#endif
        return size;
    }

    TPtr8 buffer((TUint8*)data, (int)maxSize);
    TInetAddr addr;
    TRequestStatus status;
    d->nativeSocket.RecvFrom(buffer, addr, 0, status);
    User::WaitForRequest(status); //Non blocking receive

    if (status.Int()) {
        d->setError(QAbstractSocket::NetworkError, d->ReceiveDatagramErrorString);
    } else if (port || address) {
        d->getPortAndAddress(addr, port, address);
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    int len = buffer.Length();
    qDebug("QSymbianSocketEngine::receiveDatagram(%p \"%s\", %lli, %s, %i) == %lli",
           data, qt_prettyDebug(data, qMin(len, ssize_t(16)), len).data(), maxSize,
           address ? address->toString().toLatin1().constData() : "(nil)",
           port ? *port : 0, (qint64) len);
#endif

    if (status.Int())
        return -1;
    return qint64(buffer.Length());
}


qint64 QSymbianSocketEngine::writeDatagram(const char *data, qint64 len,
                                                   const QHostAddress &host, quint16 port)
{
    Q_D(QSymbianSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::writeDatagram(), -1);
    Q_CHECK_TYPE(QSymbianSocketEngine::writeDatagram(), QAbstractSocket::UdpSocket, -1);
    TPtrC8 buffer((TUint8*)data, (int)len);
    TInetAddr addr;
    d->setPortAndAddress(addr, port, host);
    TSockXfrLength sentBytes;
    TRequestStatus status;
    d->nativeSocket.SendTo(buffer, addr, 0, status, sentBytes);
    User::WaitForRequest(status); //Non blocking send
    TInt err = status.Int(); 

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QSymbianSocketEngine::writeDatagram(%p \"%s\", %lli, \"%s\", %i) == %lli (err=%d)", data,
           qt_prettyDebug(data, qMin<int>(len, 16), len).data(), len, host.toString().toLatin1().constData(),
           port, (qint64) sentBytes(), err);
#endif

    if (err) {
        switch (err) {
        case KErrWouldBlock:
            // do not error the socket. (otherwise socket layer is reset)
            // On symbian^1 and earlier, KErrWouldBlock is returned when interface is not up yet
            // On symbian^3, KErrNone is returned but sentBytes = 0
            return 0;
        case KErrTooBig:
            d->setError(QAbstractSocket::DatagramTooLargeError, d->DatagramTooLargeErrorString);
            break;
        default:
            d->setError(QAbstractSocket::NetworkError, d->SendDatagramErrorString);
        }
        return -1;
    }

    if (QSysInfo::s60Version() <= QSysInfo::SV_S60_5_0) {
        // This is evil hack, but for some reason native RSocket::SendTo returns 0,
        // for large datagrams (such as 600 bytes). Based on comments from Open C team
        // this should happen only in platforms <= S60 5.0.
        return len;
    }
    return sentBytes();
}

bool QSymbianSocketEnginePrivate::fetchConnectionParameters()
{
    localPort = 0;
    localAddress.clear();
    peerPort = 0;
    peerAddress.clear();

    if (socketDescriptor == -1)
        return false;

    if (!nativeSocket.SubSessionHandle()) {
        if (!QSymbianSocketManager::instance().lookupSocket(socketDescriptor, nativeSocket)) {
            setError(QAbstractSocket::UnsupportedSocketOperationError, InvalidSocketErrorString);
            return false;
        }
    }

    // Determine local address
    TSockAddr addr;
    nativeSocket.LocalName(addr);
    getPortAndAddress(addr, &localPort, &localAddress);

    // Determine protocol family
    socketProtocol = localAddress.protocol();

    // Determine the remote address
    nativeSocket.RemoteName(addr);
    getPortAndAddress(addr, &peerPort, &peerAddress);

    // Determine the socket type (UDP/TCP)
    TProtocolDesc protocol;
    TInt err = nativeSocket.Info(protocol);
    if (err) {
        setError(err);
        return false;
    } else {
        switch (protocol.iProtocol) {
        case KProtocolInetTcp:
            socketType = QAbstractSocket::TcpSocket;
            break;
        case KProtocolInetUdp:
            socketType = QAbstractSocket::UdpSocket;
            break;
        default:
            socketType = QAbstractSocket::UnknownSocketType;
            break;
        }
    }
#if defined (QNATIVESOCKETENGINE_DEBUG)
    QString socketProtocolStr = QLatin1String("UnknownProtocol");
    if (socketProtocol == QAbstractSocket::IPv4Protocol) socketProtocolStr = QLatin1String("IPv4Protocol");
    else if (socketProtocol == QAbstractSocket::IPv6Protocol) socketProtocolStr = QLatin1String("IPv6Protocol");

    QString socketTypeStr = QLatin1String("UnknownSocketType");
    if (socketType == QAbstractSocket::TcpSocket) socketTypeStr = QLatin1String("TcpSocket");
    else if (socketType == QAbstractSocket::UdpSocket) socketTypeStr = QLatin1String("UdpSocket");

    qDebug("QSymbianSocketEnginePrivate::fetchConnectionParameters() local == %s:%i,"
           " peer == %s:%i, socket == %s - %s",
           localAddress.toString().toLatin1().constData(), localPort,
           peerAddress.toString().toLatin1().constData(), peerPort,socketTypeStr.toLatin1().constData(),
           socketProtocolStr.toLatin1().constData());
#endif
    return true;
}

void QSymbianSocketEngine::close()
{
    if (!isValid())
        return;
    Q_D(QSymbianSocketEngine);
#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QSymbianSocketEngine::close()");
#endif

    d->readNotificationsEnabled = false;
    d->writeNotificationsEnabled = false;
    d->exceptNotificationsEnabled = false;
    if (d->asyncSelect) {
        d->asyncSelect->deleteLater();
        d->asyncSelect = 0;
    }

    //RSocket::Shutdown(EImmediate) performs a fast disconnect. For TCP,
    //this would mean sending RST rather than FIN so we don't do that.
    //Qt's disconnectFromHost() API doesn't expose this choice.
    //RSocket::Close will internally do a normal shutdown of the socket.
    if (d->socketType == QAbstractSocket::UdpSocket) {
        //RSocket::Close has been observed to block for a long time with
        //UDP sockets. Doing an immediate shutdown first works around this
        //problem. Since UDP is connectionless, there should be no difference
        //at the network interface.
        TRequestStatus stat;
        d->nativeSocket.Shutdown(RSocket::EImmediate, stat);
        User::WaitForRequest(stat);
    }
#ifdef QNATIVESOCKETENGINE_DEBUG
    qDebug() << "QSymbianSocketEngine::close - closing socket" << d->nativeSocket.SubSessionHandle() << d->socketDescriptor;
#endif
    //remove must come before close to avoid a race where another thread gets the old subsession handle
    //reused & asserts when calling QSymbianSocketManager::instance->addSocket
    QSymbianSocketManager::instance().removeSocket(d->nativeSocket);
    d->nativeSocket.Close();
    d->socketDescriptor = -1;

    d->socketState = QAbstractSocket::UnconnectedState;
    d->hasSetSocketError = false;
    d->localPort = 0;
    d->localAddress.clear();
    d->peerPort = 0;
    d->peerAddress.clear();

    d->hasReceivedBufferedDatagram = false;
    d->receivedDataBuffer.clear();
}

qint64 QSymbianSocketEngine::write(const char *data, qint64 len)
{
    Q_D(QSymbianSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::write(), -1);
    Q_CHECK_STATE(QSymbianSocketEngine::write(), QAbstractSocket::ConnectedState, -1);
    TPtrC8 buffer((TUint8*)data, (int)len);
    TSockXfrLength sentBytes = 0;
    TRequestStatus status;
    d->nativeSocket.Send(buffer, 0, status, sentBytes);
    User::WaitForRequest(status);
    TInt err = status.Int(); 

    if (err) {
        switch (err) {
        case KErrDisconnected:
        case KErrEof:
            sentBytes = -1;
            d->setError(QAbstractSocket::RemoteHostClosedError, d->RemoteHostClosedErrorString);
            close();
            break;
        case KErrTooBig:
            d->setError(QAbstractSocket::DatagramTooLargeError, d->DatagramTooLargeErrorString);
            break;
        case KErrWouldBlock:
            break;
        default:
            sentBytes = -1;
            d->setError(err);
            close();
            break;
        }
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QSymbianSocketEngine::write(%p \"%s\", %llu) == %i",
           data, qt_prettyDebug(data, qMin((int) len, 16),
                                (int) len).data(), len, (int) sentBytes());
#endif

    return qint64(sentBytes());
}
/*
*/
qint64 QSymbianSocketEngine::read(char *data, qint64 maxSize)
{
    Q_D(QSymbianSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::read(), -1);
    Q_CHECK_STATES(QSymbianSocketEngine::read(), QAbstractSocket::ConnectedState, QAbstractSocket::BoundState, -1);

    // if a datagram was buffered in pendingDatagramSize(), return it now
    if (d->hasReceivedBufferedDatagram) {
        qint64 size = qMin(maxSize, (qint64)d->receivedDataBuffer.size());
        memcpy(data, d->receivedDataBuffer.constData(), size);
        d->receivedDataBuffer.clear();
        d->hasReceivedBufferedDatagram = false;
#if defined (QNATIVESOCKETENGINE_DEBUG)
        qDebug() << "QSymbianSocketEngine::read returning" << size << "bytes from buffer";
#endif
        return size;
    }

    TPtr8 buffer((TUint8*)data, (int)maxSize);
    TSockXfrLength received = 0;
    TRequestStatus status;
    TSockAddr dummy;
    if (d->socketType == QAbstractSocket::UdpSocket) {
        //RecvOneOrMore() can only be used with stream-interfaced connected sockets; datagram interface sockets will return KErrNotSupported.
        d->nativeSocket.RecvFrom(buffer, dummy, 0, status);
    } else {
        d->nativeSocket.RecvOneOrMore(buffer, 0, status, received);
    }
    User::WaitForRequest(status); //Non blocking receive
    TInt err = status.Int();
    int r = buffer.Length();

    if (err == KErrWouldBlock) {
        // No data was available for reading
        r = -2;
    } else if (err != KErrNone) {
        d->setError(err);
        close();
        r = -1;
    }

#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug("QSymbianSocketEngine::read(%p \"%s\", %llu) == %i (err = %d)",
           data, qt_prettyDebug(data, qMin(r, ssize_t(16)), r).data(),
           maxSize, r, err);
#endif

    return qint64(r);
}

int QSymbianSocketEnginePrivate::nativeSelect(int timeout, bool selectForRead) const
{
    bool readyRead = false;
    bool readyWrite = false;
    if (selectForRead)
        return nativeSelect(timeout, true, false, &readyRead, &readyWrite);
    else
        return nativeSelect(timeout, false, true, &readyRead, &readyWrite);
}

/*!
 \internal
 \param timeout timeout in milliseconds
 \param checkRead caller is interested if the socket is ready to read
 \param checkWrite caller is interested if the socket is ready for write
 \param selectForRead (out) should set to true if ready to read
 \param selectForWrite (out) should set to true if ready to write
 \return 0 on timeout, >0 on success, <0 on error
 */
int QSymbianSocketEnginePrivate::nativeSelect(int timeout, bool checkRead, bool checkWrite,
                       bool *selectForRead, bool *selectForWrite) const
{
    //cancel asynchronous notifier (only one IOCTL allowed at a time)
    if (asyncSelect)
        asyncSelect->Cancel();

    TPckgBuf<TUint> selectFlags;
    selectFlags() = KSockSelectExcept;
    if (checkRead)
        selectFlags() |= KSockSelectRead;
    if (checkWrite)
        selectFlags() |= KSockSelectWrite;
    TInt err;
    if (timeout == 0) {
        //if timeout is zero, poll
        err = nativeSocket.GetOpt(KSOSelectPoll, KSOLSocket, selectFlags);
    } else {
        TRequestStatus selectStat;
        nativeSocket.Ioctl(KIOctlSelect, selectStat, &selectFlags, KSOLSocket);

        if (timeout < 0)
            User::WaitForRequest(selectStat); //negative means no timeout
        else {
            if (!selectTimer.Handle())
                qt_symbian_throwIfError(selectTimer.CreateLocal());
            TRequestStatus timerStat;
            selectTimer.HighRes(timerStat, timeout * 1000);
            User::WaitForRequest(timerStat, selectStat);
            if (selectStat == KRequestPending) {
                nativeSocket.CancelIoctl();
                //CancelIoctl completes the request (most likely with KErrCancel)
                //We need to wait for this to keep the thread semaphore balanced (or active scheduler will panic)
                User::WaitForRequest(selectStat);
                //restart asynchronous notifier (only one IOCTL allowed at a time)
                if (asyncSelect)
                    asyncSelect->IssueRequest();
    #ifdef QNATIVESOCKETENGINE_DEBUG
                qDebug() << "QSymbianSocketEnginePrivate::nativeSelect: select timeout";
    #endif
                return 0; //timeout
            } else {
                selectTimer.Cancel();
                User::WaitForRequest(timerStat);
            }
        }

    #ifdef QNATIVESOCKETENGINE_DEBUG
        qDebug() << "QSymbianSocketEnginePrivate::nativeSelect: select status" << selectStat.Int() << (int)selectFlags();
    #endif
        err = selectStat.Int();
    }

    if (!err && (selectFlags() & KSockSelectExcept)) {
        nativeSocket.GetOpt(KSOSelectLastError, KSOLSocket, err);
#ifdef QNATIVESOCKETENGINE_DEBUG
        qDebug() << "QSymbianSocketEnginePrivate::nativeSelect: select last error" <<  err;
#endif
    }
    if (err) {
        //set the error here, because read won't always return the same error again as select.
        const_cast<QSymbianSocketEnginePrivate*>(this)->setError(err);
        //restart asynchronous notifier (only one IOCTL allowed at a time)
        if (asyncSelect)
            asyncSelect->IssueRequest();
        return err;
    }
    if (checkRead && (selectFlags() & KSockSelectRead)) {
        Q_ASSERT(selectForRead);
        *selectForRead = true;
    }
    if (checkWrite && (selectFlags() & KSockSelectWrite)) {
        Q_ASSERT(selectForWrite);
        *selectForWrite = true;
    }
    //restart asynchronous notifier (only one IOCTL allowed at a time)
    if (asyncSelect)
        asyncSelect->IssueRequest();
    return 1;
}

bool QSymbianSocketEngine::joinMulticastGroup(const QHostAddress &groupAddress,
                              const QNetworkInterface &iface)
{
    Q_D(QSymbianSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::joinMulticastGroup(), false);
    Q_CHECK_STATE(QSymbianSocketEngine::joinMulticastGroup(), QAbstractSocket::BoundState, false);
    Q_CHECK_TYPE(QSymbianSocketEngine::joinMulticastGroup(), QAbstractSocket::UdpSocket, false);
    return d->multicastGroupMembershipHelper(groupAddress, iface, KSoIp6JoinGroup);
}

bool QSymbianSocketEngine::leaveMulticastGroup(const QHostAddress &groupAddress,
                               const QNetworkInterface &iface)
{
    Q_D(QSymbianSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::leaveMulticastGroup(), false);
    Q_CHECK_STATE(QSymbianSocketEngine::leaveMulticastGroup(), QAbstractSocket::BoundState, false);
    Q_CHECK_TYPE(QSymbianSocketEngine::leaveMulticastGroup(), QAbstractSocket::UdpSocket, false);
    return d->multicastGroupMembershipHelper(groupAddress, iface, KSoIp6LeaveGroup);
}

bool QSymbianSocketEnginePrivate::multicastGroupMembershipHelper(const QHostAddress &groupAddress,
                          const QNetworkInterface &iface,
                          TUint operation)
{
#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug() << "QSymbianSocketEnginePrivate::multicastGroupMembershipHelper" << groupAddress << iface << operation;
#endif
    //translate address
    TPckgBuf<TIp6Mreq> option;
    if (groupAddress.protocol() == QAbstractSocket::IPv6Protocol) {
        Q_IPV6ADDR ip6 = groupAddress.toIPv6Address();
        memcpy(option().iAddr.u.iAddr8, ip6.c, 16);
    } else {
        TInetAddr wrapped;
        wrapped.SetAddress(groupAddress.toIPv4Address());
        wrapped.ConvertToV4Mapped();
        option().iAddr = wrapped.Ip6Address();
    }
    option().iInterface = iface.index();
    //join or leave group
    TInt err = nativeSocket.SetOpt(operation, KSolInetIp, option);
#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug() << "address" << qt_prettyDebug((const char *)(option().iAddr.u.iAddr8), 16, 16);
    qDebug() << "interface" << option().iInterface;
    qDebug() << "error" << err;
#endif
    if (err) {
        setError(err);
    }
    return (KErrNone == err);
}

QNetworkInterface QSymbianSocketEngine::multicastInterface() const
{
    //### symbian 3 has no API equivalent to this
    const Q_D(QSymbianSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::multicastInterface(), QNetworkInterface());
    Q_CHECK_TYPE(QSymbianSocketEngine::multicastInterface(), QAbstractSocket::UdpSocket, QNetworkInterface());
    return QNetworkInterface();
}

bool QSymbianSocketEngine::setMulticastInterface(const QNetworkInterface &iface)
{
    //### symbian 3 has no API equivalent to this
    //this is possibly a unix'ism as the RConnection on which the socket was created is probably controlling this
    Q_D(QSymbianSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::setMulticastInterface(), false);
    Q_CHECK_TYPE(QSymbianSocketEngine::setMulticastInterface(), QAbstractSocket::UdpSocket, false);
    return false;
}

bool QSymbianSocketEnginePrivate::checkProxy(const QHostAddress &address)
{
    if (address == QHostAddress::LocalHost || address == QHostAddress::LocalHostIPv6)
        return true;

#if !defined(QT_NO_NETWORKPROXY)
    QObject *parent = q_func()->parent();
    QNetworkProxy proxy;
    if (QAbstractSocket *socket = qobject_cast<QAbstractSocket *>(parent)) {
        proxy = socket->proxy();
    } else if (QTcpServer *server = qobject_cast<QTcpServer *>(parent)) {
        proxy = server->proxy();
    } else {
        // no parent -> no proxy
        return true;
    }

    if (proxy.type() == QNetworkProxy::DefaultProxy)
        proxy = QNetworkProxy::applicationProxy();

    if (proxy.type() != QNetworkProxy::DefaultProxy &&
        proxy.type() != QNetworkProxy::NoProxy) {
        // QSymbianSocketEngine doesn't do proxies
        setError(QAbstractSocket::UnsupportedSocketOperationError,
                 InvalidProxyTypeString);
        return false;
    }
#endif

    return true;
}

// ### this is also in QNativeSocketEngine, unify it
/*! \internal

    Sets the error and error string if not set already. The only
    interesting error is the first one that occurred, and not the last
    one.
*/
void QSymbianSocketEnginePrivate::setError(QAbstractSocket::SocketError error, ErrorString errorString) const
{
    if (hasSetSocketError) {
        // Only set socket errors once for one engine; expect the
        // socket to recreate its engine after an error. Note: There's
        // one exception: SocketError(11) bypasses this as it's purely
        // a temporary internal error condition.
        // Another exception is the way the waitFor*() functions set
        // an error when a timeout occurs. After the call to setError()
        // they reset the hasSetSocketError to false
        return;
    }
    if (error != QAbstractSocket::SocketError(11))
        hasSetSocketError = true;

    socketError = error;

    switch (errorString) {
    case NonBlockingInitFailedErrorString:
        socketErrorString = QSymbianSocketEngine::tr("Unable to initialize non-blocking socket");
        break;
    case BroadcastingInitFailedErrorString:
        socketErrorString = QSymbianSocketEngine::tr("Unable to initialize broadcast socket");
        break;
    case NoIpV6ErrorString:
        socketErrorString = QSymbianSocketEngine::tr("Attempt to use IPv6 socket on a platform with no IPv6 support");
        break;
    case RemoteHostClosedErrorString:
        socketErrorString = QSymbianSocketEngine::tr("The remote host closed the connection");
        break;
    case TimeOutErrorString:
        socketErrorString = QSymbianSocketEngine::tr("Network operation timed out");
        break;
    case ResourceErrorString:
        socketErrorString = QSymbianSocketEngine::tr("Out of resources");
        break;
    case OperationUnsupportedErrorString:
        socketErrorString = QSymbianSocketEngine::tr("Unsupported socket operation");
        break;
    case ProtocolUnsupportedErrorString:
        socketErrorString = QSymbianSocketEngine::tr("Protocol type not supported");
        break;
    case InvalidSocketErrorString:
        socketErrorString = QSymbianSocketEngine::tr("Invalid socket descriptor");
        break;
    case HostUnreachableErrorString:
        socketErrorString = QSymbianSocketEngine::tr("Host unreachable");
        break;
    case NetworkUnreachableErrorString:
        socketErrorString = QSymbianSocketEngine::tr("Network unreachable");
        break;
    case AccessErrorString:
        socketErrorString = QSymbianSocketEngine::tr("Permission denied");
        break;
    case ConnectionTimeOutErrorString:
        socketErrorString = QSymbianSocketEngine::tr("Connection timed out");
        break;
    case ConnectionRefusedErrorString:
        socketErrorString = QSymbianSocketEngine::tr("Connection refused");
        break;
    case AddressInuseErrorString:
        socketErrorString = QSymbianSocketEngine::tr("The bound address is already in use");
        break;
    case AddressNotAvailableErrorString:
        socketErrorString = QSymbianSocketEngine::tr("The address is not available");
        break;
    case AddressProtectedErrorString:
        socketErrorString = QSymbianSocketEngine::tr("The address is protected");
        break;
    case DatagramTooLargeErrorString:
        socketErrorString = QSymbianSocketEngine::tr("Datagram was too large to send");
        break;
    case SendDatagramErrorString:
        socketErrorString = QSymbianSocketEngine::tr("Unable to send a message");
        break;
    case ReceiveDatagramErrorString:
        socketErrorString = QSymbianSocketEngine::tr("Unable to receive a message");
        break;
    case WriteErrorString:
        socketErrorString = QSymbianSocketEngine::tr("Unable to write");
        break;
    case ReadErrorString:
        socketErrorString = QSymbianSocketEngine::tr("Network error");
        break;
    case PortInuseErrorString:
        socketErrorString = QSymbianSocketEngine::tr("Another socket is already listening on the same port");
        break;
    case NotSocketErrorString:
        socketErrorString = QSymbianSocketEngine::tr("Operation on non-socket");
        break;
    case InvalidProxyTypeString:
        socketErrorString = QSymbianSocketEngine::tr("The proxy type is invalid for this operation");
        break;
    case InvalidAddressErrorString:
        socketErrorString = QSymbianSocketEngine::tr("The address is invalid for this operation");
        break;
    case SessionNotOpenErrorString:
        socketErrorString = QSymbianSocketEngine::tr("The specified network session is not opened");
        break;
    case UnknownSocketErrorString:
        socketErrorString = QSymbianSocketEngine::tr("Unknown error");
        break;
    }
}

void QSymbianSocketEnginePrivate::setError(TInt symbianError)
{
    switch (symbianError) {
    case KErrDisconnected:
    case KErrEof:
    case KErrConnectionTerminated: //interface stopped externally - RConnection::Stop(EStopAuthoritative)
        setError(QAbstractSocket::RemoteHostClosedError,
                 QSymbianSocketEnginePrivate::RemoteHostClosedErrorString);
        break;
    case KErrNetUnreach:
        setError(QAbstractSocket::NetworkError,
                 QSymbianSocketEnginePrivate::NetworkUnreachableErrorString);
        break;
    case KErrHostUnreach:
        setError(QAbstractSocket::NetworkError,
                 QSymbianSocketEnginePrivate::HostUnreachableErrorString);
        break;
    case KErrNoProtocolOpt:
        setError(QAbstractSocket::NetworkError,
                 QSymbianSocketEnginePrivate::ProtocolUnsupportedErrorString);
        break;
    case KErrInUse:
        setError(QAbstractSocket::AddressInUseError, AddressInuseErrorString);
        break;
    case KErrPermissionDenied:
        setError(QAbstractSocket::SocketAccessError, AccessErrorString);
        break;
    case KErrNotSupported:
        setError(QAbstractSocket::UnsupportedSocketOperationError, OperationUnsupportedErrorString);
        break;
    case KErrNoMemory:
        setError(QAbstractSocket::SocketResourceError, ResourceErrorString);
        break;
    case KErrCouldNotConnect:
        setError(QAbstractSocket::ConnectionRefusedError, ConnectionRefusedErrorString);
        break;
    case KErrTimedOut:
        setError(QAbstractSocket::NetworkError, ConnectionTimeOutErrorString);
        break;
    case KErrBadName:
        setError(QAbstractSocket::NetworkError, InvalidAddressErrorString);
        break;
    default:
        socketError = QAbstractSocket::NetworkError;
        socketErrorString = QSystemError(symbianError, QSystemError::NativeError).toString();
        break;
    }
    hasSetSocketError = true;
}

void QSymbianSocketEngine::startNotifications()
{
    Q_D(QSymbianSocketEngine);
#ifdef QNATIVESOCKETENGINE_DEBUG
        qDebug() << "QSymbianSocketEngine::startNotifications" << d->readNotificationsEnabled << d->writeNotificationsEnabled << d->exceptNotificationsEnabled;
#endif
    if (!d->asyncSelect && (d->readNotificationsEnabled || d->writeNotificationsEnabled || d->exceptNotificationsEnabled)) {
        Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::startNotifications(), Q_VOID);
        if (d->threadData->eventDispatcher) {
            d->asyncSelect = q_check_ptr(new QAsyncSelect(
                static_cast<QEventDispatcherSymbian*> (d->threadData->eventDispatcher), d->nativeSocket,
                this));
        } else {
            // call again when event dispatcher has been created
            QMetaObject::invokeMethod(this, "startNotifications", Qt::QueuedConnection);
        }
    }
    if (d->asyncSelect)
        d->asyncSelect->IssueRequest();
}

bool QSymbianSocketEngine::isReadNotificationEnabled() const
{
    Q_D(const QSymbianSocketEngine);
    return d->readNotificationsEnabled;
}

void QSymbianSocketEngine::setReadNotificationEnabled(bool enable)
{
    Q_D(QSymbianSocketEngine);
#ifdef QNATIVESOCKETENGINE_DEBUG
    qDebug() << "QSymbianSocketEngine::setReadNotificationEnabled" << enable << "socket" << d->socketDescriptor;
#endif
    d->readNotificationsEnabled = enable;
    startNotifications();
}

bool QSymbianSocketEngine::isWriteNotificationEnabled() const
{
    Q_D(const QSymbianSocketEngine);
    return d->writeNotificationsEnabled;
}

void QSymbianSocketEngine::setWriteNotificationEnabled(bool enable)
{
    Q_D(QSymbianSocketEngine);
#ifdef QNATIVESOCKETENGINE_DEBUG
    qDebug() << "QSymbianSocketEngine::setWriteNotificationEnabled" << enable << "socket" << d->socketDescriptor;
#endif
    d->writeNotificationsEnabled = enable;
    startNotifications();
}

bool QSymbianSocketEngine::isExceptionNotificationEnabled() const
{
    Q_D(const QSymbianSocketEngine);
    return d->exceptNotificationsEnabled;
    return false;
}

void QSymbianSocketEngine::setExceptionNotificationEnabled(bool enable)
{
    Q_D(QSymbianSocketEngine);
#ifdef QNATIVESOCKETENGINE_DEBUG
    qDebug() << "QSymbianSocketEngine::setExceptionNotificationEnabled" << enable << "socket" << d->socketDescriptor;
#endif
    d->exceptNotificationsEnabled = enable;
    startNotifications();
}

bool QSymbianSocketEngine::waitForRead(int msecs, bool *timedOut)
{
    Q_D(const QSymbianSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::waitForRead(), false);
    Q_CHECK_NOT_STATE(QSymbianSocketEngine::waitForRead(),
                      QAbstractSocket::UnconnectedState, false);

    if (timedOut)
        *timedOut = false;

    int ret = d->nativeSelect(msecs, true);
    if (ret == 0) {
        if (timedOut)
            *timedOut = true;
        d->setError(QAbstractSocket::SocketTimeoutError,
            d->TimeOutErrorString);
        d->hasSetSocketError = false; // A timeout error is temporary in waitFor functions
        return false;
    } else if (state() == QAbstractSocket::ConnectingState) {
        connectToHost(d->peerAddress, d->peerPort);
    }

    return ret > 0;
}

bool QSymbianSocketEngine::waitForWrite(int msecs, bool *timedOut)
{
    Q_D(QSymbianSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::waitForWrite(), false);
    Q_CHECK_NOT_STATE(QSymbianSocketEngine::waitForWrite(),
                      QAbstractSocket::UnconnectedState, false);

    if (timedOut)
        *timedOut = false;

    int ret = d->nativeSelect(msecs, false);

    if (ret == 0) {
        if (timedOut)
            *timedOut = true;
        d->setError(QAbstractSocket::SocketTimeoutError,
                    d->TimeOutErrorString);
        d->hasSetSocketError = false; // A timeout error is temporary in waitFor functions
        return false;
    } else if (state() == QAbstractSocket::ConnectingState) {
        connectToHost(d->peerAddress, d->peerPort);
    }

    return ret > 0;
}

bool QSymbianSocketEngine::waitForReadOrWrite(bool *readyToRead, bool *readyToWrite,
                                      bool checkRead, bool checkWrite,
                                      int msecs, bool *timedOut)
{
    Q_D(QSymbianSocketEngine);
    Q_CHECK_VALID_SOCKETLAYER(QSymbianSocketEngine::waitForWrite(), false);
    Q_CHECK_NOT_STATE(QSymbianSocketEngine::waitForReadOrWrite(),
                      QAbstractSocket::UnconnectedState, false);

    int ret = d->nativeSelect(msecs, checkRead, checkWrite, readyToRead, readyToWrite);

    if (ret == 0) {
        if (timedOut)
            *timedOut = true;
        d->setError(QAbstractSocket::SocketTimeoutError,
                    d->TimeOutErrorString);
        d->hasSetSocketError = false; // A timeout error is temporary in waitFor functions
        return false;
    } else if (state() == QAbstractSocket::ConnectingState) {
        connectToHost(d->peerAddress, d->peerPort);
    }

    return ret > 0;
}

qint64 QSymbianSocketEngine::bytesToWrite() const
{
    // This is what the QNativeSocketEngine does
    return 0;
}

bool QSymbianSocketEngine::event(QEvent* ev)
{
    Q_D(QSymbianSocketEngine);
    if (ev->type() == QEvent::ThreadChange) {
#ifdef QNATIVESOCKETENGINE_DEBUG
        qDebug() << "QSymbianSocketEngine::event - ThreadChange" << d->readNotificationsEnabled << d->writeNotificationsEnabled << d->exceptNotificationsEnabled;
#endif
        if (d->asyncSelect) {
            delete d->asyncSelect;
            d->asyncSelect = 0;
            // recreate select in new thread (because it is queued, the method is called in the new thread context)
            QMetaObject::invokeMethod(this, "startNotifications", Qt::QueuedConnection);
        }
        d->selectTimer.Close();
        return true;
    }
    return QAbstractSocketEngine::event(ev);
}

QAsyncSelect::QAsyncSelect(QEventDispatcherSymbian *dispatcher, RSocket& sock, QSymbianSocketEngine *parent)
    : QActiveObject(CActive::EPriorityStandard, dispatcher),
      m_inSocketEvent(false),
      m_deleteLater(false),
      m_socket(sock),
      m_selectFlags(0),
      engine(parent)
{
    CActiveScheduler::Add(this);
}

QAsyncSelect::~QAsyncSelect()
{
    Cancel();
}

void QAsyncSelect::DoCancel()
{
    m_socket.CancelIoctl();
}

void QAsyncSelect::RunL()
{
    QT_TRYCATCH_LEAVING(run());
}

//RunError is called by the active scheduler if RunL leaves.
//Typically this will happen if a std::bad_alloc propagates down from the application
TInt QAsyncSelect::RunError(TInt aError)
{
    if (engine) {
        QT_TRY {
            engine->d_func()->setError(aError);
            if (engine->isExceptionNotificationEnabled())
                engine->exceptionNotification();
            if (engine->isReadNotificationEnabled())
                engine->readNotification();
        }
        QT_CATCH(...) {}
    }
#ifdef QNATIVESOCKETENGINE_DEBUG
    qDebug() << "QAsyncSelect::RunError" << aError;
#endif
    return KErrNone;
}

void QAsyncSelect::run()
{
    //when event loop disabled socket events, defer until later
    if (maybeDeferSocketEvent())
        return;
#if defined (QNATIVESOCKETENGINE_DEBUG)
    qDebug() << "QAsyncSelect::run" << m_selectBuf() << m_selectFlags;
#endif
    m_inSocketEvent = true;
    m_selectBuf() &= m_selectFlags; //the select ioctl reports everything, so mask to only what we requested
    //KSockSelectReadContinuation is for reading datagrams in a mode that doesn't discard when the
    //datagram is larger than the read buffer - Qt doesn't need to use this.
    if (engine && engine->isReadNotificationEnabled()
        && ((m_selectBuf() & KSockSelectRead) || iStatus != KErrNone)) {
        engine->readNotification();
    }
    if (engine && engine->isWriteNotificationEnabled()
        && ((m_selectBuf() & KSockSelectWrite) || iStatus != KErrNone)) {
        if (engine->state() == QAbstractSocket::ConnectingState)
            engine->connectionComplete();
        else
            engine->writeNotification();
    }
    if (engine && engine->isExceptionNotificationEnabled()
        && ((m_selectBuf() & KSockSelectExcept) || iStatus != KErrNone)) {
        engine->exceptionNotification();
    }
    m_inSocketEvent = false;
    if (m_deleteLater) {
        delete this;
        return;
    }
    // select again (unless disabled by one of the callbacks)
    IssueRequest();
}

void QAsyncSelect::deleteLater()
{
    if (m_inSocketEvent) {
        engine = 0;
        m_deleteLater = true;
    } else {
        delete this;
    }
}

void QAsyncSelect::IssueRequest()
{
    if (m_inSocketEvent)
        return; //prevent thrashing during a callback - socket engine enables/disables multiple notifiers
    TUint selectFlags = 0;
    if (engine->isReadNotificationEnabled())
        selectFlags |= KSockSelectRead;
    if (engine->isWriteNotificationEnabled())
        selectFlags |= KSockSelectWrite;
    if (engine->isExceptionNotificationEnabled())
        selectFlags |= KSockSelectExcept;
    if (selectFlags != m_selectFlags) {
#ifdef QNATIVESOCKETENGINE_DEBUG
    qDebug() << "QAsyncSelect::IssueRequest() - select flags" << m_selectFlags << "->" << selectFlags;
#endif
        Cancel();
        m_selectFlags = selectFlags;
    }
    if (m_selectFlags && !IsActive()) {
        //always request errors (write notification does not complete on connect errors)
        m_selectBuf() = m_selectFlags | KSockSelectExcept;
        m_socket.Ioctl(KIOctlSelect, iStatus, &m_selectBuf, KSOLSocket);
        SetActive();
    }
#ifdef QNATIVESOCKETENGINE_DEBUG
    qDebug() << "QAsyncSelect::IssueRequest() - IsActive" << IsActive();
#endif
}

QT_END_NAMESPACE
