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

#include <qdebug.h>

// The buffer size need to be 0 otherwise data could be
// lost if the socket that has written data closes the connection
// before it is read.  Pipewriter is used for write buffering.
#define BUFSIZE 0

// ###: This should be a property. Should replace the insane 50 on unix as well.
#define SYSTEM_MAX_PENDING_SOCKETS 8

QT_BEGIN_NAMESPACE

bool QLocalServerPrivate::addListener()
{
    // The object must not change its address once the
    // contained OVERLAPPED struct is passed to Windows.
    listeners << Listener();
    Listener &listener = listeners.last();

    listener.handle = CreateNamedPipe(
                 (const wchar_t *)fullServerName.utf16(), // pipe name
                 PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,       // read/write access
                 PIPE_TYPE_BYTE |          // byte type pipe
                 PIPE_READMODE_BYTE |      // byte-read mode
                 PIPE_WAIT,                // blocking mode
                 PIPE_UNLIMITED_INSTANCES, // max. instances
                 BUFSIZE,                  // output buffer size
                 BUFSIZE,                  // input buffer size
                 3000,                     // client time-out
                 NULL);

    if (listener.handle == INVALID_HANDLE_VALUE) {
        setError(QLatin1String("QLocalServerPrivate::addListener"));
        listeners.removeLast();
        return false;
    }

    memset(&listener.overlapped, 0, sizeof(listener.overlapped));
    listener.overlapped.hEvent = eventHandle;
    if (!ConnectNamedPipe(listener.handle, &listener.overlapped)) {
        switch (GetLastError()) {
        case ERROR_IO_PENDING:
            listener.connected = false;
            break;
        case ERROR_PIPE_CONNECTED:
            listener.connected = true;
            SetEvent(eventHandle);
            break;
        default:
            CloseHandle(listener.handle);
            setError(QLatin1String("QLocalServerPrivate::addListener"));
            listeners.removeLast();
            return false;
        }
    } else {
        Q_ASSERT_X(false, "QLocalServerPrivate::addListener", "The impossible happened");
        SetEvent(eventHandle);
    }
    return true;
}

void QLocalServerPrivate::setError(const QString &function)
{
    int windowsError = GetLastError();
    errorString = QString::fromLatin1("%1: %2").arg(function).arg(qt_error_string(windowsError));
    error = QAbstractSocket::UnknownSocketError;
}

void QLocalServerPrivate::init()
{
}

bool QLocalServerPrivate::removeServer(const QString &name)
{
    Q_UNUSED(name);
    return true;
}

bool QLocalServerPrivate::listen(const QString &name)
{
    Q_Q(QLocalServer);

    QString pipePath = QLatin1String("\\\\.\\pipe\\");
    if (name.startsWith(pipePath))
        fullServerName = name;
    else
        fullServerName = pipePath + name;

    // Use only one event for all listeners of one socket.
    // The idea is that listener events are rare, so polling all listeners once in a while is
    // cheap compared to waiting for N additional events in each iteration of the main loop.
    eventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
    connectionEventNotifier = new QWinEventNotifier(eventHandle , q);
    q->connect(connectionEventNotifier, SIGNAL(activated(HANDLE)), q, SLOT(_q_onNewConnection()));

    for (int i = 0; i < SYSTEM_MAX_PENDING_SOCKETS; ++i)
        if (!addListener())
            return false;
    return true;
}

void QLocalServerPrivate::_q_onNewConnection()
{
    Q_Q(QLocalServer);
    DWORD dummy;

    // Reset first, otherwise we could reset an event which was asserted
    // immediately after we checked the conn status.
    ResetEvent(eventHandle);

    // Testing shows that there is indeed absolutely no guarantee which listener gets
    // a client connection first, so there is no way around polling all of them.
    for (int i = 0; i < listeners.size(); ) {
        HANDLE handle = listeners[i].handle;
        if (listeners[i].connected
            || GetOverlappedResult(handle, &listeners[i].overlapped, &dummy, FALSE))
        {
            listeners.removeAt(i);

            addListener();

            if (pendingConnections.size() > maxPendingConnections)
                connectionEventNotifier->setEnabled(false);

            // Make this the last thing so connected slots can wreak the least havoc
            q->incomingConnection((quintptr)handle);
        } else {
            if (GetLastError() != ERROR_IO_INCOMPLETE) {
                q->close();
                setError(QLatin1String("QLocalServerPrivate::_q_onNewConnection"));
                return;
            }

            ++i;
        }
    }
}

void QLocalServerPrivate::closeServer()
{
    connectionEventNotifier->setEnabled(false); // Otherwise, closed handle is checked before deleter runs
    connectionEventNotifier->deleteLater();
    connectionEventNotifier = 0;
    CloseHandle(eventHandle);
    for (int i = 0; i < listeners.size(); ++i)
        CloseHandle(listeners[i].handle);
    listeners.clear();
}

void QLocalServerPrivate::waitForNewConnection(int msecs, bool *timedOut)
{
    Q_Q(QLocalServer);
    if (!pendingConnections.isEmpty() || !q->isListening())
        return;

    DWORD result = WaitForSingleObject(eventHandle, (msecs == -1) ? INFINITE : msecs);
    if (result == WAIT_TIMEOUT) {
        if (timedOut)
            *timedOut = true;
    } else {
        _q_onNewConnection();
    }
}

QT_END_NAMESPACE
