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

//#define QUDPSOCKET_DEBUG

/*! \class QUdpSocket

    \reentrant
    \brief The QUdpSocket class provides a UDP socket.

    \ingroup network
    \inmodule QtNetwork

    UDP (User Datagram Protocol) is a lightweight, unreliable,
    datagram-oriented, connectionless protocol. It can be used when
    reliability isn't important. QUdpSocket is a subclass of
    QAbstractSocket that allows you to send and receive UDP
    datagrams.

    The most common way to use this class is to bind to an address and port
    using bind(), then call writeDatagram() and readDatagram() to transfer
    data. If you want to use the standard QIODevice functions read(),
    readLine(), write(), etc., you must first connect the socket directly to a
    peer by calling connectToHost().

    The socket emits the bytesWritten() signal every time a datagram
    is written to the network. If you just want to send datagrams,
    you don't need to call bind().

    The readyRead() signal is emitted whenever datagrams arrive. In
    that case, hasPendingDatagrams() returns \c true. Call
    pendingDatagramSize() to obtain the size of the first pending
    datagram, and readDatagram() to read it.

    \note An incoming datagram should be read when you receive the readyRead()
    signal, otherwise this signal will not be emitted for the next datagram.

    Example:

    \snippet code/src_network_socket_qudpsocket.cpp 0

    QUdpSocket also supports UDP multicast. Use joinMulticastGroup() and
    leaveMulticastGroup() to control group membership, and
    QAbstractSocket::MulticastTtlOption and
    QAbstractSocket::MulticastLoopbackOption to set the TTL and loopback socket
    options. Use setMulticastInterface() to control the outgoing interface for
    multicast datagrams, and multicastInterface() to query it.

    With QUdpSocket, you can also establish a virtual connection to a
    UDP server using connectToHost() and then use read() and write()
    to exchange datagrams without specifying the receiver for each
    datagram.

    The \l{broadcastsender}{Broadcast Sender},
    \l{broadcastreceiver}{Broadcast Receiver},
    \l{multicastsender}{Multicast Sender}, and
    \l{multicastreceiver}{Multicast Receiver} examples illustrate how
    to use QUdpSocket in applications.

    \sa QTcpSocket
*/

#include "qudpsocket.h"
#include "qhostaddress.h"
#include "qnetworkinterface.h"
#include "qabstractsocket_p.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_UDPSOCKET

#define QT_CHECK_BOUND(function, a) do { \
    if (!isValid()) { \
        qWarning(function" called on a QUdpSocket when not in QUdpSocket::BoundState"); \
        return (a); \
    } } while (0)

class QUdpSocketPrivate : public QAbstractSocketPrivate
{
    Q_DECLARE_PUBLIC(QUdpSocket)

    bool doEnsureInitialized(const QHostAddress &bindAddress, quint16 bindPort,
                             const QHostAddress &remoteAddress);
public:
    inline bool ensureInitialized(const QHostAddress &bindAddress, quint16 bindPort)
    { return doEnsureInitialized(bindAddress, bindPort, QHostAddress()); }

    inline bool ensureInitialized(const QHostAddress &remoteAddress)
    { return doEnsureInitialized(QHostAddress(), 0, remoteAddress); }
};

bool QUdpSocketPrivate::doEnsureInitialized(const QHostAddress &bindAddress, quint16 bindPort,
                                            const QHostAddress &remoteAddress)
{
    const QHostAddress *address = &bindAddress;
    QAbstractSocket::NetworkLayerProtocol proto = address->protocol();
    if (proto == QUdpSocket::UnknownNetworkLayerProtocol) {
        address = &remoteAddress;
        proto = address->protocol();
    }

    // now check if the socket engine is initialized and to the right type
    if (!socketEngine || !socketEngine->isValid()) {
        resolveProxy(remoteAddress.toString(), bindPort);
        if (!initSocketLayer(address->protocol()))
            return false;
    }

    return true;
}

/*!
    Creates a QUdpSocket object.

    \a parent is passed to the QObject constructor.

    \sa socketType()
*/
QUdpSocket::QUdpSocket(QObject *parent)
    : QAbstractSocket(UdpSocket, *new QUdpSocketPrivate, parent)
{
    d_func()->isBuffered = false;
}

/*!
    Destroys the socket, closing the connection if necessary.

    \sa close()
*/
QUdpSocket::~QUdpSocket()
{
}

#ifndef QT_NO_NETWORKINTERFACE

/*!
    \since 4.8

    Joins the multicast group specified by \a groupAddress on the default
    interface chosen by the operating system. The socket must be in BoundState,
    otherwise an error occurs.

    Note that if you are attempting to join an IPv4 group, your socket must not
    be bound using IPv6 (or in dual mode, using QHostAddress::Any). You must use
    QHostAddress::AnyIPv4 instead.

    This function returns \c true if successful; otherwise it returns \c false
    and sets the socket error accordingly.

    \sa leaveMulticastGroup()
*/
bool QUdpSocket::joinMulticastGroup(const QHostAddress &groupAddress)
{
    return joinMulticastGroup(groupAddress, QNetworkInterface());
}

/*!
    \since 4.8
    \overload

    Joins the multicast group address \a groupAddress on the interface \a
    iface.

    \sa leaveMulticastGroup()
*/
bool QUdpSocket::joinMulticastGroup(const QHostAddress &groupAddress,
                                    const QNetworkInterface &iface)
{
    Q_D(QUdpSocket);
    QT_CHECK_BOUND("QUdpSocket::joinMulticastGroup()", false);
    return d->socketEngine->joinMulticastGroup(groupAddress, iface);
}

/*!
    \since 4.8

    Leaves the multicast group specified by \a groupAddress on the default
    interface chosen by the operating system. The socket must be in BoundState,
    otherwise an error occurs.

   This function returns \c true if successful; otherwise it returns \c false and
   sets the socket error accordingly.

   \sa joinMulticastGroup()
*/
bool QUdpSocket::leaveMulticastGroup(const QHostAddress &groupAddress)
{
    return leaveMulticastGroup(groupAddress, QNetworkInterface());
}

/*!
    \since 4.8
    \overload

    Leaves the multicast group specified by \a groupAddress on the interface \a
    iface.

    \sa joinMulticastGroup()
*/
bool QUdpSocket::leaveMulticastGroup(const QHostAddress &groupAddress,
                                     const QNetworkInterface &iface)
{
    QT_CHECK_BOUND("QUdpSocket::leaveMulticastGroup()", false);
    return d_func()->socketEngine->leaveMulticastGroup(groupAddress, iface);
}

/*!
    \since 4.8

    Returns the interface for the outgoing interface for multicast datagrams.
    This corresponds to the IP_MULTICAST_IF socket option for IPv4 sockets and
    the IPV6_MULTICAST_IF socket option for IPv6 sockets. If no interface has
    been previously set, this function returns an invalid QNetworkInterface.
    The socket must be in BoundState, otherwise an invalid QNetworkInterface is
    returned.

    \sa setMulticastInterface()
*/
QNetworkInterface QUdpSocket::multicastInterface() const
{
    Q_D(const QUdpSocket);
    QT_CHECK_BOUND("QUdpSocket::multicastInterface()", QNetworkInterface());
    return d->socketEngine->multicastInterface();
}

/*!
    \since 4.8

    Sets the outgoing interface for multicast datagrams to the interface \a
    iface. This corresponds to the IP_MULTICAST_IF socket option for IPv4
    sockets and the IPV6_MULTICAST_IF socket option for IPv6 sockets. The
    socket must be in BoundState, otherwise this function does nothing.

    \sa multicastInterface(), joinMulticastGroup(), leaveMulticastGroup()
*/
void QUdpSocket::setMulticastInterface(const QNetworkInterface &iface)
{
    Q_D(QUdpSocket);
    if (!isValid()) {
        qWarning("QUdpSocket::setMulticastInterface() called on a QUdpSocket when not in QUdpSocket::BoundState");
        return;
    }
    d->socketEngine->setMulticastInterface(iface);
}

#endif // QT_NO_NETWORKINTERFACE

/*!
    Returns \c true if at least one datagram is waiting to be read;
    otherwise returns \c false.

    \sa pendingDatagramSize(), readDatagram()
*/
bool QUdpSocket::hasPendingDatagrams() const
{
    QT_CHECK_BOUND("QUdpSocket::hasPendingDatagrams()", false);
    return d_func()->socketEngine->hasPendingDatagrams();
}

/*!
    Returns the size of the first pending UDP datagram. If there is
    no datagram available, this function returns -1.

    \sa hasPendingDatagrams(), readDatagram()
*/
qint64 QUdpSocket::pendingDatagramSize() const
{
    QT_CHECK_BOUND("QUdpSocket::pendingDatagramSize()", -1);
    return d_func()->socketEngine->pendingDatagramSize();
}

/*!
    Sends the datagram at \a data of size \a size to the host
    address \a address at port \a port. Returns the number of
    bytes sent on success; otherwise returns -1.

    Datagrams are always written as one block. The maximum size of a
    datagram is highly platform-dependent, but can be as low as 8192
    bytes. If the datagram is too large, this function will return -1
    and error() will return DatagramTooLargeError.

    Sending datagrams larger than 512 bytes is in general disadvised,
    as even if they are sent successfully, they are likely to be
    fragmented by the IP layer before arriving at their final
    destination.

    \warning Calling this function on a connected UDP socket may
    result in an error and no packet being sent. If you are using a
    connected socket, use write() to send datagrams.

    \sa readDatagram(), write()
*/
qint64 QUdpSocket::writeDatagram(const char *data, qint64 size, const QHostAddress &address,
                                  quint16 port)
{
    Q_D(QUdpSocket);
#if defined QUDPSOCKET_DEBUG
    qDebug("QUdpSocket::writeDatagram(%p, %llu, \"%s\", %i)", data, size,
           address.toString().toLatin1().constData(), port);
#endif
    if (!d->doEnsureInitialized(QHostAddress::Any, 0, address))
        return -1;
    if (state() == UnconnectedState)
        bind();

    qint64 sent = d->socketEngine->writeDatagram(data, size, address, port);
    d->cachedSocketDescriptor = d->socketEngine->socketDescriptor();

    if (sent >= 0) {
        emit bytesWritten(sent);
    } else {
        d->socketError = d->socketEngine->error();
        setErrorString(d->socketEngine->errorString());
        emit error(d->socketError);
    }
    return sent;
}

/*!
    \fn qint64 QUdpSocket::writeDatagram(const QByteArray &datagram,
                                             const QHostAddress &host, quint16 port)
    \overload

    Sends the datagram \a datagram to the host address \a host and at
    port \a port.
*/

/*!
    Receives a datagram no larger than \a maxSize bytes and stores
    it in \a data. The sender's host address and port is stored in
    *\a address and *\a port (unless the pointers are 0).

    Returns the size of the datagram on success; otherwise returns
    -1.

    If \a maxSize is too small, the rest of the datagram will be
    lost. To avoid loss of data, call pendingDatagramSize() to
    determine the size of the pending datagram before attempting to
    read it. If \a maxSize is 0, the datagram will be discarded.

    \sa writeDatagram(), hasPendingDatagrams(), pendingDatagramSize()
*/
qint64 QUdpSocket::readDatagram(char *data, qint64 maxSize, QHostAddress *address,
                                    quint16 *port)
{
    Q_D(QUdpSocket);

#if defined QUDPSOCKET_DEBUG
    qDebug("QUdpSocket::readDatagram(%p, %llu, %p, %p)", data, maxSize, address, port);
#endif
    QT_CHECK_BOUND("QUdpSocket::readDatagram()", -1);
    qint64 readBytes = d->socketEngine->readDatagram(data, maxSize, address, port);
    d_func()->socketEngine->setReadNotificationEnabled(true);
    if (readBytes < 0) {
        d->socketError = d->socketEngine->error();
        setErrorString(d->socketEngine->errorString());
        emit error(d->socketError);
    }
    return readBytes;
}
#endif // QT_NO_UDPSOCKET

QT_END_NAMESPACE
