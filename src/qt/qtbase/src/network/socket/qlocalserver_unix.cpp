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

#include "qlocalserver.h"
#include "qlocalserver_p.h"
#include "qlocalsocket.h"
#include "qlocalsocket_p.h"
#include "qnet_unix_p.h"
#include "qtemporarydir.h"

#ifndef QT_NO_LOCALSERVER

#include <sys/socket.h>
#include <sys/un.h>

#include <qdebug.h>
#include <qdir.h>
#include <qdatetime.h>

#ifdef Q_OS_VXWORKS
#  include <selectLib.h>
#endif

QT_BEGIN_NAMESPACE

void QLocalServerPrivate::init()
{
}

bool QLocalServerPrivate::removeServer(const QString &name)
{
    QString fileName;
    if (name.startsWith(QLatin1Char('/'))) {
        fileName = name;
    } else {
        fileName = QDir::cleanPath(QDir::tempPath());
        fileName += QLatin1Char('/') + name;
    }
    if (QFile::exists(fileName))
        return QFile::remove(fileName);
    else
        return true;
}

bool QLocalServerPrivate::listen(const QString &requestedServerName)
{
    Q_Q(QLocalServer);

    // determine the full server path
    if (requestedServerName.startsWith(QLatin1Char('/'))) {
        fullServerName = requestedServerName;
    } else {
        fullServerName = QDir::cleanPath(QDir::tempPath());
        fullServerName += QLatin1Char('/') + requestedServerName;
    }
    serverName = requestedServerName;

    QString tempPath;
    QScopedPointer<QTemporaryDir> tempDir;

    // Check any of the flags
    if (socketOptions & QLocalServer::WorldAccessOption) {
        tempDir.reset(new QTemporaryDir(fullServerName));
        if (!tempDir->isValid()) {
            setError(QLatin1String("QLocalServer::listen"));
            return false;
        }
        tempPath = tempDir->path();
        tempPath += QLatin1Char('/') + requestedServerName;
    }

    // create the unix socket
    listenSocket = qt_safe_socket(PF_UNIX, SOCK_STREAM, 0);
    if (-1 == listenSocket) {
        setError(QLatin1String("QLocalServer::listen"));
        closeServer();
        return false;
    }

    // Construct the unix address
    struct ::sockaddr_un addr;
    addr.sun_family = PF_UNIX;
    if (sizeof(addr.sun_path) < (uint)fullServerName.toLatin1().size() + 1) {
        setError(QLatin1String("QLocalServer::listen"));
        closeServer();
        return false;
    }

    if (socketOptions & QLocalServer::WorldAccessOption) {
        if (sizeof(addr.sun_path) < (uint)tempPath.toLatin1().size() + 1) {
            setError(QLatin1String("QLocalServer::listen"));
            closeServer();
            return false;
        }
        ::memcpy(addr.sun_path, tempPath.toLatin1().data(),
                 tempPath.toLatin1().size() + 1);

        if (-1 == ::fchmod(listenSocket, 0)) {
            setError(QLatin1String("QLocalServer::listen"));
            closeServer();
            return false;
        }

    } else {
        ::memcpy(addr.sun_path, fullServerName.toLatin1().data(),
                 fullServerName.toLatin1().size() + 1);
    }

    // bind
    if(-1 == QT_SOCKET_BIND(listenSocket, (sockaddr *)&addr, sizeof(sockaddr_un))) {
        setError(QLatin1String("QLocalServer::listen"));
        // if address is in use already, just close the socket, but do not delete the file
        if(errno == EADDRINUSE)
            QT_CLOSE(listenSocket);
        // otherwise, close the socket and delete the file
        else
            closeServer();
        listenSocket = -1;
        return false;
    }

    // listen for connections
    if (-1 == qt_safe_listen(listenSocket, 50)) {
        setError(QLatin1String("QLocalServer::listen"));
        closeServer();
        listenSocket = -1;
        if (error != QAbstractSocket::AddressInUseError)
            QFile::remove(fullServerName);
        return false;
    }

    if (socketOptions & QLocalServer::WorldAccessOption) {
            mode_t mode = 000;

            if (socketOptions & QLocalServer::UserAccessOption) {
                mode |= S_IRWXU;
            }
            if (socketOptions & QLocalServer::GroupAccessOption) {
                mode |= S_IRWXG;
            }
            if (socketOptions & QLocalServer::OtherAccessOption) {
                mode |= S_IRWXO;
            }

            if (mode) {
                if (-1 == ::chmod(tempPath.toLatin1(), mode)) {
                    setError(QLatin1String("QLocalServer::listen"));
                    closeServer();
                    return false;
                }
            }
            if (-1 == ::rename(tempPath.toLatin1(), fullServerName.toLatin1())){
                setError(QLatin1String("QLocalServer::listen"));
                closeServer();
                return false;
            }
    }

    Q_ASSERT(!socketNotifier);
    socketNotifier = new QSocketNotifier(listenSocket,
                                         QSocketNotifier::Read, q);
    q->connect(socketNotifier, SIGNAL(activated(int)),
               q, SLOT(_q_onNewConnection()));
    socketNotifier->setEnabled(maxPendingConnections > 0);
    return true;
}

bool QLocalServerPrivate::listen(qintptr socketDescriptor)
{
    Q_Q(QLocalServer);

    // Attach to the localsocket
    listenSocket = socketDescriptor;

    ::fcntl(listenSocket, F_SETFD, FD_CLOEXEC);
    ::fcntl(listenSocket, F_SETFL, ::fcntl(listenSocket, F_GETFL) | O_NONBLOCK);

#ifdef Q_OS_LINUX
    struct ::sockaddr_un addr;
    QT_SOCKLEN_T len = sizeof(addr);
    memset(&addr, 0, sizeof(addr));
    if (0 == ::getsockname(listenSocket, (sockaddr *)&addr, &len)) {
        // check for absract sockets
        if (addr.sun_family == PF_UNIX && addr.sun_path[0] == 0) {
            addr.sun_path[0] = '@';
        }
        QString name = QString::fromLatin1(addr.sun_path);
        if (!name.isEmpty()) {
            fullServerName = name;
            serverName = fullServerName.mid(fullServerName.lastIndexOf(QLatin1String("/"))+1);
            if (serverName.isEmpty()) {
                serverName = fullServerName;
            }
        }
    }
#else
    serverName.clear();
    fullServerName.clear();
#endif

    Q_ASSERT(!socketNotifier);
    socketNotifier = new QSocketNotifier(listenSocket,
                                         QSocketNotifier::Read, q);
    q->connect(socketNotifier, SIGNAL(activated(int)),
               q, SLOT(_q_onNewConnection()));
    socketNotifier->setEnabled(maxPendingConnections > 0);
    return true;
}

/*!
    \internal

    \sa QLocalServer::closeServer()
 */
void QLocalServerPrivate::closeServer()
{
    if (socketNotifier) {
        socketNotifier->setEnabled(false); // Otherwise, closed socket is checked before deleter runs
        socketNotifier->deleteLater();
        socketNotifier = 0;
    }

    if (-1 != listenSocket)
        QT_CLOSE(listenSocket);
    listenSocket = -1;

    if (!fullServerName.isEmpty())
        QFile::remove(fullServerName);
}

/*!
    \internal

    We have received a notification that we can read on the listen socket.
    Accept the new socket.
 */
void QLocalServerPrivate::_q_onNewConnection()
{
    Q_Q(QLocalServer);
    if (-1 == listenSocket)
        return;

    ::sockaddr_un addr;
    QT_SOCKLEN_T length = sizeof(sockaddr_un);
    int connectedSocket = qt_safe_accept(listenSocket, (sockaddr *)&addr, &length);
    if(-1 == connectedSocket) {
        setError(QLatin1String("QLocalSocket::activated"));
        closeServer();
    } else {
        socketNotifier->setEnabled(pendingConnections.size()
                                   <= maxPendingConnections);
        q->incomingConnection(connectedSocket);
    }
}

void QLocalServerPrivate::waitForNewConnection(int msec, bool *timedOut)
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(listenSocket, &readfds);

    struct timespec timeout;
    timeout.tv_sec = msec / 1000;
    timeout.tv_nsec = (msec % 1000) * 1000 * 1000;

    int result = -1;
    result = qt_safe_select(listenSocket + 1, &readfds, 0, 0, (msec == -1) ? 0 : &timeout);
    if (-1 == result) {
        setError(QLatin1String("QLocalServer::waitForNewConnection"));
        closeServer();
    }
    if (result > 0)
        _q_onNewConnection();
    if (timedOut)
        *timedOut = (result == 0);
}

void QLocalServerPrivate::setError(const QString &function)
{
    if (EAGAIN == errno)
        return;

    switch (errno) {
    case EACCES:
        errorString = QLocalServer::tr("%1: Permission denied").arg(function);
        error = QAbstractSocket::SocketAccessError;
        break;
    case ELOOP:
    case ENOENT:
    case ENAMETOOLONG:
    case EROFS:
    case ENOTDIR:
        errorString = QLocalServer::tr("%1: Name error").arg(function);
        error = QAbstractSocket::HostNotFoundError;
        break;
    case EADDRINUSE:
        errorString = QLocalServer::tr("%1: Address in use").arg(function);
        error = QAbstractSocket::AddressInUseError;
        break;

    default:
        errorString = QLocalServer::tr("%1: Unknown error %2")
                      .arg(function).arg(errno);
        error = QAbstractSocket::UnknownSocketError;
#if defined QLOCALSERVER_DEBUG
        qWarning() << errorString << "fullServerName:" << fullServerName;
#endif
    }
}

QT_END_NAMESPACE

#endif // QT_NO_LOCALSERVER
