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

#ifndef QSYMBIANSOCKETENGINE_P_H
#define QSYMBIANSOCKETENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//
#include "QtNetwork/qhostaddress.h"
#include "private/qabstractsocketengine_p.h"
#include "qplatformdefs.h"

#include <private/qeventdispatcher_symbian_p.h>
#include <unistd.h>
#include <es_sock.h>
#include <in_sock.h>

QT_BEGIN_NAMESPACE


class QSymbianSocketEnginePrivate;
class QNetworkInterface;

class Q_AUTOTEST_EXPORT QSymbianSocketEngine : public QAbstractSocketEngine
{
    Q_OBJECT
    friend class QAsyncSelect;
public:
    QSymbianSocketEngine(QObject *parent = 0);
    ~QSymbianSocketEngine();

    bool initialize(QAbstractSocket::SocketType type, QAbstractSocket::NetworkLayerProtocol protocol = QAbstractSocket::IPv4Protocol);
    bool initialize(int socketDescriptor, QAbstractSocket::SocketState socketState = QAbstractSocket::ConnectedState);

    int socketDescriptor() const;

    bool isValid() const;

    bool connectToHost(const QHostAddress &address, quint16 port);
    bool connectToHostByName(const QString &name, quint16 port);
    bool bind(const QHostAddress &address, quint16 port);
    bool listen();
    int accept();
    void close();

    bool joinMulticastGroup(const QHostAddress &groupAddress,
                            const QNetworkInterface &iface);
    bool leaveMulticastGroup(const QHostAddress &groupAddress,
                             const QNetworkInterface &iface);
    QNetworkInterface multicastInterface() const;
    bool setMulticastInterface(const QNetworkInterface &iface);

    qint64 bytesAvailable() const;

    qint64 read(char *data, qint64 maxlen);
    qint64 write(const char *data, qint64 len);

    qint64 readDatagram(char *data, qint64 maxlen, QHostAddress *addr = 0,
                            quint16 *port = 0);
    qint64 writeDatagram(const char *data, qint64 len, const QHostAddress &addr,
                             quint16 port);
    bool hasPendingDatagrams() const;
    qint64 pendingDatagramSize() const;

    qint64 bytesToWrite() const;

    qint64 receiveBufferSize() const;
    void setReceiveBufferSize(qint64 bufferSize);

    qint64 sendBufferSize() const;
    void setSendBufferSize(qint64 bufferSize);

    int option(SocketOption option) const;
    bool setOption(SocketOption option, int value);

    bool waitForRead(int msecs = 30000, bool *timedOut = 0);
    bool waitForWrite(int msecs = 30000, bool *timedOut = 0);
    bool waitForReadOrWrite(bool *readyToRead, bool *readyToWrite,
                bool checkRead, bool checkWrite,
                int msecs = 30000, bool *timedOut = 0);

    bool isReadNotificationEnabled() const;
    void setReadNotificationEnabled(bool enable);
    bool isWriteNotificationEnabled() const;
    void setWriteNotificationEnabled(bool enable);
    bool isExceptionNotificationEnabled() const;
    void setExceptionNotificationEnabled(bool enable);

    bool event(QEvent* ev);

    Q_INVOKABLE void startNotifications();

    void connectionComplete();

private:
    Q_DECLARE_PRIVATE(QSymbianSocketEngine)
    Q_DISABLE_COPY(QSymbianSocketEngine)
};

class QSocketNotifier;

class QReadNotifier;
class QWriteNotifier;
class QExceptionNotifier;
class QAsyncSelect : public QActiveObject
{
public:
    QAsyncSelect(QEventDispatcherSymbian *dispatcher, RSocket& sock, QSymbianSocketEngine *parent);
    ~QAsyncSelect();

    void deleteLater();
    void IssueRequest();

    void refresh();

protected:
    void DoCancel();
    void RunL();
    void run();
    TInt RunError(TInt aError);

private:
    bool m_inSocketEvent;
    bool m_deleteLater;
    RSocket &m_socket;

    TUint m_selectFlags;
    TPckgBuf<TUint> m_selectBuf; //in & out IPC buffer
    QSymbianSocketEngine *engine;
};

class QSymbianSocketEnginePrivate : public QAbstractSocketEnginePrivate
{
    Q_DECLARE_PUBLIC(QSymbianSocketEngine)
public:
    QSymbianSocketEnginePrivate();
    ~QSymbianSocketEnginePrivate();

    int socketDescriptor;
    mutable RSocket nativeSocket;
    // From QtCore:
    RSocketServ& socketServer;
    mutable RTimer selectTimer;

    bool readNotificationsEnabled;
    bool writeNotificationsEnabled;
    bool exceptNotificationsEnabled;
    QAsyncSelect* asyncSelect;

    mutable QByteArray receivedDataBuffer;
    mutable bool hasReceivedBufferedDatagram;
    enum ErrorString {
        NonBlockingInitFailedErrorString,
        BroadcastingInitFailedErrorString,
        NoIpV6ErrorString,
        RemoteHostClosedErrorString,
        TimeOutErrorString,
        ResourceErrorString,
        OperationUnsupportedErrorString,
        ProtocolUnsupportedErrorString,
        InvalidSocketErrorString,
        HostUnreachableErrorString,
        NetworkUnreachableErrorString,
        AccessErrorString,
        ConnectionTimeOutErrorString,
        ConnectionRefusedErrorString,
        AddressInuseErrorString,
        AddressNotAvailableErrorString,
        AddressProtectedErrorString,
        DatagramTooLargeErrorString,
        SendDatagramErrorString,
        ReceiveDatagramErrorString,
        WriteErrorString,
        ReadErrorString,
        PortInuseErrorString,
        NotSocketErrorString,
        InvalidProxyTypeString,
        //symbian specific
        InvalidAddressErrorString,
        SessionNotOpenErrorString,

        UnknownSocketErrorString = -1
    };
    void setError(QAbstractSocket::SocketError error, ErrorString errorString) const;

    void getPortAndAddress(const TInetAddr& a, quint16 *port, QHostAddress *addr);
    void setPortAndAddress(TInetAddr& nativeAddr, quint16 port, const QHostAddress &addr);
    void setError(TInt symbianError);

    int nativeSelect(int timeout, bool selectForRead) const;
    int nativeSelect(int timeout, bool checkRead, bool checkWrite,
                           bool *selectForRead, bool *selectForWrite) const;

    bool createNewSocket(QAbstractSocket::SocketType socketType,
                                             QAbstractSocket::NetworkLayerProtocol socketProtocol);

    bool checkProxy(const QHostAddress &address);
    bool fetchConnectionParameters();

    bool multicastGroupMembershipHelper(const QHostAddress &groupAddress,
                              const QNetworkInterface &iface,
                              TUint operation);
    static bool translateSocketOption(QAbstractSocketEngine::SocketOption opt, TUint &n, TUint &level);
};

QT_END_NAMESPACE

#endif // QSYMBIANSOCKETENGINE_P_H
