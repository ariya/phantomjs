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

#ifndef QABSTRACTSOCKET_H
#define QABSTRACTSOCKET_H

#include <QtCore/qiodevice.h>
#include <QtCore/qobject.h>
#ifndef QT_NO_DEBUG_STREAM
#include <QtCore/qdebug.h>
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Network)

class QHostAddress;
#ifndef QT_NO_NETWORKPROXY
class QNetworkProxy;
#endif
class QAbstractSocketPrivate;
class QAuthenticator;

class Q_NETWORK_EXPORT QAbstractSocket : public QIODevice
{
    Q_OBJECT
    Q_ENUMS(SocketType NetworkLayerProtocol SocketError SocketState SocketOption)
public:
    enum SocketType {
        TcpSocket,
        UdpSocket,
        UnknownSocketType = -1
    };
    enum NetworkLayerProtocol {
        IPv4Protocol,
        IPv6Protocol,
        UnknownNetworkLayerProtocol = -1
    };
    enum SocketError {
        ConnectionRefusedError,
        RemoteHostClosedError,
        HostNotFoundError,
        SocketAccessError,
        SocketResourceError,
        SocketTimeoutError,                     /* 5 */
        DatagramTooLargeError,
        NetworkError,
        AddressInUseError,
        SocketAddressNotAvailableError,
        UnsupportedSocketOperationError,        /* 10 */
        UnfinishedSocketOperationError,
        ProxyAuthenticationRequiredError,
        SslHandshakeFailedError,
        ProxyConnectionRefusedError,
        ProxyConnectionClosedError,             /* 15 */
        ProxyConnectionTimeoutError,
        ProxyNotFoundError,
        ProxyProtocolError,

        UnknownSocketError = -1
    };
    enum SocketState {
        UnconnectedState,
        HostLookupState,
        ConnectingState,
        ConnectedState,
        BoundState,
        ListeningState,
        ClosingState
#ifdef QT3_SUPPORT
        ,
        Idle = UnconnectedState,
        HostLookup = HostLookupState,
        Connecting = ConnectingState,
        Connected = ConnectedState,
        Closing = ClosingState,
        Connection = ConnectedState
#endif
    };
    enum SocketOption {
        LowDelayOption, // TCP_NODELAY
        KeepAliveOption, // SO_KEEPALIVE
        MulticastTtlOption, // IP_MULTICAST_TTL
        MulticastLoopbackOption // IP_MULTICAST_LOOPBACK
    };

    QAbstractSocket(SocketType socketType, QObject *parent);
    virtual ~QAbstractSocket();

    // ### Qt 5: Make connectToHost() and disconnectFromHost() virtual.
    void connectToHost(const QString &hostName, quint16 port, OpenMode mode = ReadWrite);
    void connectToHost(const QHostAddress &address, quint16 port, OpenMode mode = ReadWrite);
    void disconnectFromHost();

    bool isValid() const;

    qint64 bytesAvailable() const;
    qint64 bytesToWrite() const;

    bool canReadLine() const;

    quint16 localPort() const;
    QHostAddress localAddress() const;
    quint16 peerPort() const;
    QHostAddress peerAddress() const;
    QString peerName() const;

    // ### Qt 5: Make setReadBufferSize() virtual
    qint64 readBufferSize() const;
    void setReadBufferSize(qint64 size);

    void abort();

    // ### Qt 5: Make socketDescriptor() and setSocketDescriptor() virtual.
    int socketDescriptor() const;
    bool setSocketDescriptor(int socketDescriptor, SocketState state = ConnectedState,
                             OpenMode openMode = ReadWrite);

    // ### Qt 5: Make virtual?
    void setSocketOption(QAbstractSocket::SocketOption option, const QVariant &value);
    QVariant socketOption(QAbstractSocket::SocketOption option);

    SocketType socketType() const;
    SocketState state() const;
    SocketError error() const;

    // from QIODevice
    void close();
    bool isSequential() const;
    bool atEnd() const;
    bool flush();

    // for synchronous access
    // ### Qt 5: Make waitForConnected() and waitForDisconnected() virtual.
    bool waitForConnected(int msecs = 30000);
    bool waitForReadyRead(int msecs = 30000);
    bool waitForBytesWritten(int msecs = 30000);
    bool waitForDisconnected(int msecs = 30000);

#ifndef QT_NO_NETWORKPROXY
    void setProxy(const QNetworkProxy &networkProxy);
    QNetworkProxy proxy() const;
#endif

Q_SIGNALS:
    void hostFound();
    void connected();
    void disconnected();
    void stateChanged(QAbstractSocket::SocketState);
    void error(QAbstractSocket::SocketError);
#ifndef QT_NO_NETWORKPROXY
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator);
#endif

protected Q_SLOTS:
    void connectToHostImplementation(const QString &hostName, quint16 port, OpenMode mode = ReadWrite);
    void disconnectFromHostImplementation();

protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 readLineData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

    void setSocketState(SocketState state);
    void setSocketError(SocketError socketError);
    void setLocalPort(quint16 port);
    void setLocalAddress(const QHostAddress &address);
    void setPeerPort(quint16 port);
    void setPeerAddress(const QHostAddress &address);
    void setPeerName(const QString &name);

    QAbstractSocket(SocketType socketType, QAbstractSocketPrivate &dd, QObject *parent = 0);

private:
    Q_DECLARE_PRIVATE(QAbstractSocket)
    Q_DISABLE_COPY(QAbstractSocket)

    Q_PRIVATE_SLOT(d_func(), void _q_connectToNextAddress())
    Q_PRIVATE_SLOT(d_func(), void _q_startConnecting(const QHostInfo &))
    Q_PRIVATE_SLOT(d_func(), void _q_abortConnectionAttempt())
    Q_PRIVATE_SLOT(d_func(), void _q_testConnection())
    Q_PRIVATE_SLOT(d_func(), void _q_forceDisconnect())

#ifdef QT3_SUPPORT
public:
    enum Error {
        ErrConnectionRefused = ConnectionRefusedError,
        ErrHostNotFound = HostNotFoundError,
        ErrSocketRead = UnknownSocketError
    };
    inline QT3_SUPPORT int socket() const { return socketDescriptor(); }
    inline QT3_SUPPORT void setSocket(int socket) { setSocketDescriptor(socket); }
    inline QT3_SUPPORT qulonglong waitForMore(int msecs, bool *timeout = 0) const
    {
        QAbstractSocket *that = const_cast<QAbstractSocket *>(this);
        if (that->waitForReadyRead(msecs))
            return qulonglong(bytesAvailable());
        if (error() == SocketTimeoutError && timeout)
            *timeout = true;
        return 0;
    }
    typedef SocketState State;
Q_SIGNALS:
    QT_MOC_COMPAT void connectionClosed(); // same as disconnected()
    QT_MOC_COMPAT void delayedCloseFinished(); // same as disconnected()


#endif
};

#ifndef QT_NO_DEBUG_STREAM
Q_NETWORK_EXPORT QDebug operator<<(QDebug, QAbstractSocket::SocketError);
Q_NETWORK_EXPORT QDebug operator<<(QDebug, QAbstractSocket::SocketState);
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QABSTRACTSOCKET_H
