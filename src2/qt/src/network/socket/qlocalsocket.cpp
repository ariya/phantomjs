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

#include "qlocalsocket.h"
#include "qlocalsocket_p.h"

#ifndef QT_NO_LOCALSOCKET

QT_BEGIN_NAMESPACE

/*!
    \class QLocalSocket
    \since 4.4

    \brief The QLocalSocket class provides a local socket.

    On Windows this is a named pipe and on Unix this is a local domain socket.

    If an error occurs, socketError() returns the type of error, and
    errorString() can be called to get a human readable description
    of what happened.

    Although QLocalSocket is designed for use with an event loop, it's possible
    to use it without one. In that case, you must use waitForConnected(),
    waitForReadyRead(), waitForBytesWritten(), and waitForDisconnected()
    which blocks until the operation is complete or the timeout expires.

    Note that this feature is not supported on versions of Windows earlier than
    Windows XP.

    \sa QLocalServer
*/

/*!
    \fn void QLocalSocket::connectToServer(const QString &name, OpenMode openMode)

    Attempts to make a connection to \a name.

    The socket is opened in the given \a openMode and first enters ConnectingState.
    It then attempts to connect to the address or addresses returned by the lookup.
    Finally, if a connection is established, QLocalSocket enters ConnectedState
    and emits connected().

    At any point, the socket can emit error() to signal that an error occurred.

    See also state(), serverName(), and waitForConnected().
*/

/*!
    \fn void QLocalSocket::connected()

    This signal is emitted after connectToServer() has been called and
    a connection has been successfully established.

    \sa connectToServer(), disconnected()
*/

/*!
    \fn bool QLocalSocket::setSocketDescriptor(quintptr socketDescriptor,
        LocalSocketState socketState, OpenMode openMode)

    Initializes QLocalSocket with the native socket descriptor
    \a socketDescriptor. Returns true if socketDescriptor is accepted
    as a valid socket descriptor; otherwise returns false. The socket is
    opened in the mode specified by \a openMode, and enters the socket state
    specified by \a socketState.

    \note It is not possible to initialize two local sockets with the same
    native socket descriptor.

    \sa socketDescriptor(), state(), openMode()
*/

/*!
    \fn quintptr QLocalSocket::socketDescriptor() const

    Returns the native socket descriptor of the QLocalSocket object if
    this is available; otherwise returns -1.

    The socket descriptor is not available when QLocalSocket
    is in UnconnectedState.

    \sa setSocketDescriptor()
*/

/*!
    \fn qint64 QLocalSocket::readData(char *data, qint64 c)
    \reimp
*/

/*!
    \fn qint64 QLocalSocket::writeData(const char *data, qint64 c)
    \reimp
*/

/*!
    \fn void QLocalSocket::abort()

    Aborts the current connection and resets the socket.
    Unlike disconnectFromServer(), this function immediately closes the socket,
    clearing any pending data in the write buffer.

    \sa disconnectFromServer(), close()
*/

/*!
    \fn qint64 QLocalSocket::bytesAvailable() const
    \reimp
*/

/*!
    \fn qint64 QLocalSocket::bytesToWrite() const
    \reimp
*/

/*!
    \fn bool QLocalSocket::canReadLine() const
    \reimp
*/

/*!
    \fn void QLocalSocket::close()
    \reimp
*/

/*!
    \fn bool QLocalSocket::waitForBytesWritten(int msecs)
    \reimp
*/

/*!
    \fn bool QLocalSocket::flush()

    This function writes as much as possible from the internal write buffer
    to the socket, without blocking.  If any data was written, this function
    returns true; otherwise false is returned.

    Call this function if you need QLocalSocket to start sending buffered data
    immediately. The number of bytes successfully written depends on the
    operating system. In most cases, you do not need to call this function,
    because QLocalSocket will start sending data automatically once control
    goes back to the event loop. In the absence of an event loop, call
    waitForBytesWritten() instead.

    \sa write(), waitForBytesWritten()
*/

/*!
    \fn void QLocalSocket::disconnectFromServer()

    Attempts to close the socket. If there is pending data waiting to be
    written, QLocalSocket will enter ClosingState and wait until all data
    has been written. Eventually, it will enter UnconnectedState and emit
    the disconnectedFromServer() signal.

    \sa connectToServer()
*/

/*!
    \fn QLocalSocket::LocalSocketError QLocalSocket::error() const

    Returns the type of error that last occurred.

    \sa state(), errorString()
*/

/*!
    \fn bool QLocalSocket::isValid() const

    Returns true if the socket is valid and ready for use; otherwise
    returns false.

    \note The socket's state must be ConnectedState before reading
    and writing can occur.

    \sa state(), connectToServer()
*/

/*!
    \fn qint64 QLocalSocket::readBufferSize() const

    Returns the size of the internal read buffer. This limits the amount of
    data that the client can receive before you call read() or readAll().
    A read buffer size of 0 (the default) means that the buffer has no size
    limit, ensuring that no data is lost.

    \sa setReadBufferSize(), read()
*/

/*!
    \fn void QLocalSocket::setReadBufferSize(qint64 size)

    Sets the size of QLocalSocket's internal read buffer to be \a size bytes.

    If the buffer size is limited to a certain size, QLocalSocket won't
    buffer more than this size of data. Exceptionally, a buffer size of 0
    means that the read buffer is unlimited and all incoming data is buffered.
    This is the default.

    This option is useful if you only read the data at certain points in
    time (e.g., in a real-time streaming application) or if you want to
    protect your socket against receiving too much data, which may eventually
    cause your application to run out of memory.

    \sa readBufferSize(), read()
*/

/*!
    \fn bool QLocalSocket::waitForConnected(int msecs)

    Waits until the socket is connected, up to \a msecs milliseconds. If the
    connection has been established, this function returns true; otherwise
    it returns false. In the case where it returns false, you can call
    error() to determine the cause of the error.

    The following example waits up to one second for a connection
    to be established:

    \snippet doc/src/snippets/code/src_network_socket_qlocalsocket_unix.cpp 0

    If \a msecs is -1, this function will not time out.

    \sa connectToServer(), connected()
*/

/*!
    \fn bool QLocalSocket::waitForDisconnected(int msecs)

    Waits until the socket has disconnected, up to \a msecs
    milliseconds. If the connection has been disconnected, this
    function returns true; otherwise it returns false. In the case
    where it returns false, you can call error() to determine
    the cause of the error.

    The following example waits up to one second for a connection
    to be closed:

    \snippet doc/src/snippets/code/src_network_socket_qlocalsocket_unix.cpp 1

    If \a msecs is -1, this function will not time out.

    \sa disconnectFromServer(), close()
*/

/*!
    \fn bool QLocalSocket::waitForReadyRead(int msecs)

    This function blocks until data is available for reading and the
    \l{QIODevice::}{readyRead()} signal has been emitted. The function
    will timeout after \a msecs milliseconds; the default timeout is
    30000 milliseconds.

    The function returns true if data is available for reading;
    otherwise it returns false (if an error occurred or the
    operation timed out).

    \sa waitForBytesWritten()
*/

/*!
    \fn void QLocalSocket::disconnected()

    This signal is emitted when the socket has been disconnected.

    \sa connectToServer(), disconnectFromServer(), abort(), connected()
*/

/*!
    \fn void QLocalSocket::error(QLocalSocket::LocalSocketError socketError)

    This signal is emitted after an error occurred. The \a socketError
    parameter describes the type of error that occurred.

    QLocalSocket::LocalSocketError is not a registered metatype, so for queued
    connections, you will have to register it with Q_DECLARE_METATYPE() and
    qRegisterMetaType().

    \sa error(), errorString(), {Creating Custom Qt Types}
*/

/*!
    \fn void QLocalSocket::stateChanged(QLocalSocket::LocalSocketState socketState)

    This signal is emitted whenever QLocalSocket's state changes.
    The \a socketState parameter is the new state.

    QLocalSocket::SocketState is not a registered metatype, so for queued
    connections, you will have to register it with Q_DECLARE_METATYPE() and
    qRegisterMetaType().

    \sa state(), {Creating Custom Qt Types}
*/

/*!
    Creates a new local socket. The \a parent argument is passed to
    QObject's constructor.
 */
QLocalSocket::QLocalSocket(QObject * parent)
    : QIODevice(*new QLocalSocketPrivate, parent)
{
    Q_D(QLocalSocket);
    d->init();
}

/*!
    Destroys the socket, closing the connection if necessary.
 */
QLocalSocket::~QLocalSocket()
{
    close();
#if !defined(Q_OS_WIN) && !defined(QT_LOCALSOCKET_TCP)
    Q_D(QLocalSocket);
    d->unixSocket.setParent(0);
#endif
}

/*!
    Returns the name of the peer as specified by connectToServer(), or an
    empty QString if connectToServer() has not been called or it failed.

    \sa connectToServer(), fullServerName()

 */
QString QLocalSocket::serverName() const
{
    Q_D(const QLocalSocket);
    return d->serverName;
}

/*!
    Returns the server path that the socket is connected to.

    \note The return value of this function is platform specific.

    \sa connectToServer(), serverName()
 */
QString QLocalSocket::fullServerName() const
{
    Q_D(const QLocalSocket);
    return d->fullServerName;
}

/*!
    Returns the state of the socket.

    \sa error()
 */
QLocalSocket::LocalSocketState QLocalSocket::state() const
{
    Q_D(const QLocalSocket);
    return d->state;
}

/*! \reimp
*/
bool QLocalSocket::isSequential() const
{
    return true;
}

/*!
    \enum QLocalSocket::LocalSocketError

    The LocalServerError enumeration represents the errors that can occur.
    The most recent error can be retrieved through a call to
    \l QLocalSocket::error().

    \value ConnectionRefusedError The connection was refused by
        the peer (or timed out).
    \value PeerClosedError  The remote socket closed the connection.
        Note that the client socket (i.e., this socket) will be closed
        after the remote close notification has been sent.
    \value ServerNotFoundError  The local socket name was not found.
    \value SocketAccessError The socket operation failed because the
        application lacked the required privileges.
    \value SocketResourceError The local system ran out of resources
        (e.g., too many sockets).
    \value SocketTimeoutError The socket operation timed out.
    \value DatagramTooLargeError The datagram was larger than the operating
        system's limit (which can be as low as 8192 bytes).
    \value ConnectionError An error occurred with the connection.
    \value UnsupportedSocketOperationError The requested socket operation
        is not supported by the local operating system.
    \value UnknownSocketError An unidentified error occurred.
 */

/*!
    \enum QLocalSocket::LocalSocketState

    This enum describes the different states in which a socket can be.

    \sa QLocalSocket::state()

    \value UnconnectedState The socket is not connected.
    \value ConnectingState The socket has started establishing a connection.
    \value ConnectedState A connection is established.
    \value ClosingState The socket is about to close
        (data may still be waiting to be written).
 */

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, QLocalSocket::LocalSocketError error)
{
    switch (error) {
    case QLocalSocket::ConnectionRefusedError:
        debug << "QLocalSocket::ConnectionRefusedError";
        break;
    case QLocalSocket::PeerClosedError:
        debug << "QLocalSocket::PeerClosedError";
        break;
    case QLocalSocket::ServerNotFoundError:
        debug << "QLocalSocket::ServerNotFoundError";
        break;
    case QLocalSocket::SocketAccessError:
        debug << "QLocalSocket::SocketAccessError";
        break;
    case QLocalSocket::SocketResourceError:
        debug << "QLocalSocket::SocketResourceError";
        break;
    case QLocalSocket::SocketTimeoutError:
        debug << "QLocalSocket::SocketTimeoutError";
        break;
    case QLocalSocket::DatagramTooLargeError:
        debug << "QLocalSocket::DatagramTooLargeError";
        break;
    case QLocalSocket::ConnectionError:
        debug << "QLocalSocket::ConnectionError";
        break;
    case QLocalSocket::UnsupportedSocketOperationError:
        debug << "QLocalSocket::UnsupportedSocketOperationError";
        break;
    case QLocalSocket::UnknownSocketError:
        debug << "QLocalSocket::UnknownSocketError";
        break;
    default:
        debug << "QLocalSocket::SocketError(" << int(error) << ')';
        break;
    }
    return debug;
}

QDebug operator<<(QDebug debug, QLocalSocket::LocalSocketState state)
{
    switch (state) {
    case QLocalSocket::UnconnectedState:
        debug << "QLocalSocket::UnconnectedState";
        break;
    case QLocalSocket::ConnectingState:
        debug << "QLocalSocket::ConnectingState";
        break;
    case QLocalSocket::ConnectedState:
        debug << "QLocalSocket::ConnectedState";
        break;
    case QLocalSocket::ClosingState:
        debug << "QLocalSocket::ClosingState";
        break;
    default:
        debug << "QLocalSocket::SocketState(" << int(state) << ')';
        break;
    }
    return debug;
}
#endif

QT_END_NAMESPACE

#endif

#include "moc_qlocalsocket.cpp"
