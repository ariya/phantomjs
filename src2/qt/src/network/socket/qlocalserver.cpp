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

#include "qlocalserver.h"
#include "qlocalserver_p.h"
#include "qlocalsocket.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_LOCALSERVER

/*!
    \class QLocalServer
    \since 4.4

    \brief The QLocalServer class provides a local socket based server.

    This class makes it possible to accept incoming local socket
    connections.

    Call listen() to have the server start listening
    for incoming connections on a specified key.  The
    newConnection() signal is then emitted each time a client
    connects to the server.

    Call nextPendingConnection() to accept the pending connection
    as a connected QLocalSocket.  The function returns a pointer to a
    QLocalSocket that can be used for communicating with the client.

    If an error occurs, serverError() returns the type of error, and
    errorString() can be called to get a human readable description
    of what happened.

    When listening for connections, the name which the server is
    listening on is available through serverName().

    Calling close() makes QLocalServer stop listening for incoming connections.

    Although QLocalServer is designed for use with an event loop, it's possible
    to use it without one. In that case, you must use waitForNewConnection(),
    which blocks until either a connection is available or a timeout expires.

    \sa QLocalSocket, QTcpServer
*/

/*!
    Create a new local socket server with the given \a parent.

    \sa listen()
 */
QLocalServer::QLocalServer(QObject *parent)
        : QObject(*new QLocalServerPrivate, parent)
{
    Q_D(QLocalServer);
    d->init();
}

/*!
    Destroys the QLocalServer object.  If the server is listening for
    connections, it is automatically closed.

    Any client QLocalSockets that are still connected must either
    disconnect or be reparented before the server is deleted.

    \sa close()
 */
QLocalServer::~QLocalServer()
{
    if (isListening())
	close();
}

/*!
    Stop listening for incoming connections.  Existing connections are not
    effected, but any new connections will be refused.

    \sa isListening(), listen()
 */
void QLocalServer::close()
{
    Q_D(QLocalServer);
    if (!isListening())
        return;
    qDeleteAll(d->pendingConnections);
    d->pendingConnections.clear();
    d->closeServer();
    d->serverName.clear();
    d->fullServerName.clear();
    d->errorString.clear();
    d->error = QAbstractSocket::UnknownSocketError;
}

/*!
    Returns the human-readable message appropriate to the current error
    reported by serverError(). If no suitable string is available, an empty
    string is returned.

    \sa serverError()
 */
QString QLocalServer::errorString() const
{
    Q_D(const QLocalServer);
    return d->errorString;
}

/*!
    Returns true if the server has a pending connection; otherwise
    returns false.

    \sa nextPendingConnection(), setMaxPendingConnections()
 */
bool QLocalServer::hasPendingConnections() const
{
    Q_D(const QLocalServer);
    return !(d->pendingConnections.isEmpty());
}

/*!
    This virtual function is called by QLocalServer when a new connection
    is available. \a socketDescriptor is the native socket descriptor for
    the accepted connection.

    The base implementation creates a QLocalSocket, sets the socket descriptor
    and then stores the QLocalSocket in an internal list of pending
    connections. Finally newConnection() is emitted.

    Reimplement this function to alter the server's behavior
    when a connection is available.

    \sa newConnection(), nextPendingConnection(),
    QLocalSocket::setSocketDescriptor()
 */
void QLocalServer::incomingConnection(quintptr socketDescriptor)
{
    Q_D(QLocalServer);
    QLocalSocket *socket = new QLocalSocket(this);
    socket->setSocketDescriptor(socketDescriptor);
    d->pendingConnections.enqueue(socket);
    emit newConnection();
}

/*!
    Returns true if the server is listening for incoming connections
    otherwise false.

    \sa listen(), close()
 */
bool QLocalServer::isListening() const
{
    Q_D(const QLocalServer);
    return !(d->serverName.isEmpty());
}

/*!
    Tells the server to listen for incoming connections on \a name.
    If the server is currently listening then it will return false.
    Return true on success otherwise false.

    \a name can be a single name and QLocalServer will determine
    the correct platform specific path.  serverName() will return
    the name that is passed into listen.

    Usually you would just pass in a name like "foo", but on Unix this
    could also be a path such as "/tmp/foo" and on Windows this could
    be a pipe path such as "\\\\.\\pipe\\foo"

    Note:
    On Unix if the server crashes without closing listen will fail
    with AddressInUseError.  To create a new server the file should be removed.
    On Windows two local servers can listen to the same pipe at the same
    time, but any connections will go to one of the server.

    \sa serverName(), isListening(), close()
 */
bool QLocalServer::listen(const QString &name)
{
    Q_D(QLocalServer);
    if (isListening()) {
        qWarning("QLocalServer::listen() called when already listening");
        return false;
    }

    if (name.isEmpty()) {
        d->error = QAbstractSocket::HostNotFoundError;
        QString function = QLatin1String("QLocalServer::listen");
        d->errorString = tr("%1: Name error").arg(function);
        return false;
    }

    if (!d->listen(name)) {
        d->serverName.clear();
        d->fullServerName.clear();
        return false;
    }

    d->serverName = name;
    return true;
}

/*!
    Returns the maximum number of pending accepted connections.
    The default is 30.

    \sa setMaxPendingConnections(), hasPendingConnections()
 */
int QLocalServer::maxPendingConnections() const
{
    Q_D(const QLocalServer);
    return d->maxPendingConnections;
}

/*!
    \fn void QLocalServer::newConnection()

    This signal is emitted every time a new connection is available.

    \sa hasPendingConnections(), nextPendingConnection()
*/

/*!
    Returns the next pending connection as a connected QLocalSocket object.

    The socket is created as a child of the server, which means that it is
    automatically deleted when the QLocalServer object is destroyed. It is
    still a good idea to delete the object explicitly when you are done with
    it, to avoid wasting memory.

    0 is returned if this function is called when there are no pending
    connections.

    \sa hasPendingConnections(), newConnection(), incomingConnection()
 */
QLocalSocket *QLocalServer::nextPendingConnection()
{
    Q_D(QLocalServer);
    if (d->pendingConnections.isEmpty())
        return 0;
    QLocalSocket *nextSocket = d->pendingConnections.dequeue();
#ifndef QT_LOCALSOCKET_TCP
#ifdef Q_OS_SYMBIAN
    if(!d->socketNotifier)
        return nextSocket;
#endif
    if (d->pendingConnections.size() <= d->maxPendingConnections)
#ifndef Q_OS_WIN
        d->socketNotifier->setEnabled(true);
#else
        d->connectionEventNotifier->setEnabled(true);
#endif
#endif
    return nextSocket;
}

/*!
    \since 4.5

    Removes any server instance that might cause a call to listen() to fail
    and returns true if successful; otherwise returns false.
    This function is meant to recover from a crash, when the previous server
    instance has not been cleaned up.

    On Windows, this function does nothing; on Unix, it removes the socket file
    given by \a name.

    \warning Be careful to avoid removing sockets of running instances.
*/
bool QLocalServer::removeServer(const QString &name)
{
    return QLocalServerPrivate::removeServer(name);
}

/*!
    Returns the server name if the server is listening for connections;
    otherwise returns QString()

    \sa listen(), fullServerName()
 */
QString QLocalServer::serverName() const
{
    Q_D(const QLocalServer);
    return d->serverName;
}

/*!
    Returns the full path that the server is listening on.

    Note: This is platform specific

    \sa listen(), serverName()
 */
QString QLocalServer::fullServerName() const
{
    Q_D(const QLocalServer);
    return d->fullServerName;
}

/*!
    Returns the type of error that occurred last or NoError.

    \sa errorString()
 */
QAbstractSocket::SocketError QLocalServer::serverError() const
{
    Q_D(const QLocalServer);
    return d->error;
}

/*!
    Sets the maximum number of pending accepted connections to
    \a numConnections.  QLocalServer will accept no more than
    \a numConnections incoming connections before nextPendingConnection()
    is called.

    Note: Even though QLocalServer will stop accepting new connections
    after it has reached its maximum number of pending connections,
    the operating system may still keep them in queue which will result
    in clients signaling that it is connected.

    \sa maxPendingConnections(), hasPendingConnections()
 */
void QLocalServer::setMaxPendingConnections(int numConnections)
{
    Q_D(QLocalServer);
    d->maxPendingConnections = numConnections;
}

/*!
    Waits for at most \a msec milliseconds or until an incoming connection
    is available.  Returns true if a connection is available; otherwise
    returns false.  If the operation timed out and \a timedOut is not 0,
    *timedOut will be set to true.

    This is a blocking function call. Its use is ill-advised in a
    single-threaded GUI application, since the whole application will stop
    responding until the function returns. waitForNewConnection() is mostly
    useful when there is no event loop available.

    The non-blocking alternative is to connect to the newConnection() signal.

    If msec is -1, this function will not time out.

    \sa hasPendingConnections(), nextPendingConnection()
 */
bool QLocalServer::waitForNewConnection(int msec, bool *timedOut)
{
    Q_D(QLocalServer);
    if (timedOut)
        *timedOut = false;

    if (!isListening())
        return false;

    d->waitForNewConnection(msec, timedOut);

    return !d->pendingConnections.isEmpty();
}

#endif

QT_END_NAMESPACE

#include "moc_qlocalserver.cpp"

