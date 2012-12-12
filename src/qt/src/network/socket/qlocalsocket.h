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

#ifndef QLOCALSOCKET_H
#define QLOCALSOCKET_H

#include <QtCore/qiodevice.h>
#include <QtNetwork/qabstractsocket.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Network)

#ifndef QT_NO_LOCALSOCKET

class QLocalSocketPrivate;

class Q_NETWORK_EXPORT QLocalSocket : public QIODevice
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QLocalSocket)

public:
    enum LocalSocketError
    {
        ConnectionRefusedError = QAbstractSocket::ConnectionRefusedError,
        PeerClosedError = QAbstractSocket::RemoteHostClosedError,
        ServerNotFoundError = QAbstractSocket::HostNotFoundError,
        SocketAccessError = QAbstractSocket::SocketAccessError,
        SocketResourceError = QAbstractSocket::SocketResourceError,
        SocketTimeoutError = QAbstractSocket::SocketTimeoutError,
        DatagramTooLargeError = QAbstractSocket::DatagramTooLargeError,
        ConnectionError = QAbstractSocket::NetworkError,
        UnsupportedSocketOperationError = QAbstractSocket::UnsupportedSocketOperationError,
        UnknownSocketError = QAbstractSocket::UnknownSocketError
    };

    enum LocalSocketState
    {
        UnconnectedState = QAbstractSocket::UnconnectedState,
        ConnectingState = QAbstractSocket::ConnectingState,
        ConnectedState = QAbstractSocket::ConnectedState,
        ClosingState = QAbstractSocket::ClosingState
    };

    QLocalSocket(QObject *parent = 0);
    ~QLocalSocket();

    void connectToServer(const QString &name, OpenMode openMode = ReadWrite);
    void disconnectFromServer();

    QString serverName() const;
    QString fullServerName() const;

    void abort();
    virtual bool isSequential() const;
    virtual qint64 bytesAvailable() const;
    virtual qint64 bytesToWrite() const;
    virtual bool canReadLine() const;
    virtual void close();
    LocalSocketError error() const;
    bool flush();
    bool isValid() const;
    qint64 readBufferSize() const;
    void setReadBufferSize(qint64 size);

    bool setSocketDescriptor(quintptr socketDescriptor,
                             LocalSocketState socketState = ConnectedState,
                             OpenMode openMode = ReadWrite);
    quintptr socketDescriptor() const;

    LocalSocketState state() const;
    bool waitForBytesWritten(int msecs = 30000);
    bool waitForConnected(int msecs = 30000);
    bool waitForDisconnected(int msecs = 30000);
    bool waitForReadyRead(int msecs = 30000);

Q_SIGNALS:
    void connected();
    void disconnected();
    void error(QLocalSocket::LocalSocketError socketError);
    void stateChanged(QLocalSocket::LocalSocketState socketState);

protected:
    virtual qint64 readData(char*, qint64);
    virtual qint64 writeData(const char*, qint64);

private:
    Q_DISABLE_COPY(QLocalSocket)
#if defined(QT_LOCALSOCKET_TCP)
    Q_PRIVATE_SLOT(d_func(), void _q_stateChanged(QAbstractSocket::SocketState))
    Q_PRIVATE_SLOT(d_func(), void _q_error(QAbstractSocket::SocketError))
#elif defined(Q_OS_WIN)
    Q_PRIVATE_SLOT(d_func(), void _q_notified())
    Q_PRIVATE_SLOT(d_func(), void _q_canWrite())
    Q_PRIVATE_SLOT(d_func(), void _q_pipeClosed())
    Q_PRIVATE_SLOT(d_func(), void _q_emitReadyRead())
#else
    Q_PRIVATE_SLOT(d_func(), void _q_stateChanged(QAbstractSocket::SocketState))
    Q_PRIVATE_SLOT(d_func(), void _q_error(QAbstractSocket::SocketError))
    Q_PRIVATE_SLOT(d_func(), void _q_connectToSocket())
    Q_PRIVATE_SLOT(d_func(), void _q_abortConnectionAttempt())
#endif
};

#ifndef QT_NO_DEBUG_STREAM
#include <QtCore/qdebug.h>
Q_NETWORK_EXPORT QDebug operator<<(QDebug, QLocalSocket::LocalSocketError);
Q_NETWORK_EXPORT QDebug operator<<(QDebug, QLocalSocket::LocalSocketState);
#endif

#endif // QT_NO_LOCALSOCKET

QT_END_NAMESPACE

QT_END_HEADER

#endif // QLOCALSOCKET_H
