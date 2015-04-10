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
#include <QtCore/private/qsystemerror_p.h>

#include <qdebug.h>

#include <aclapi.h>
#include <accctrl.h>
#include <sddl.h>

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

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = FALSE;      //non inheritable handle, same as default
    sa.lpSecurityDescriptor = 0;    //default security descriptor

    QScopedPointer<SECURITY_DESCRIPTOR> pSD;
    PSID worldSID = 0;
    QByteArray aclBuffer;
    QByteArray tokenUserBuffer;
    QByteArray tokenGroupBuffer;

    // create security descriptor if access options were specified
    if ((socketOptions & QLocalServer::WorldAccessOption)) {
        pSD.reset(new SECURITY_DESCRIPTOR);
        if (!InitializeSecurityDescriptor(pSD.data(), SECURITY_DESCRIPTOR_REVISION)) {
            setError(QLatin1String("QLocalServerPrivate::addListener"));
            return false;
        }
        HANDLE hToken = NULL;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
            return false;
        DWORD dwBufferSize = 0;
        GetTokenInformation(hToken, TokenUser, 0, 0, &dwBufferSize);
        tokenUserBuffer.fill(0, dwBufferSize);
        PTOKEN_USER pTokenUser = (PTOKEN_USER)tokenUserBuffer.data();
        if (!GetTokenInformation(hToken, TokenUser, pTokenUser, dwBufferSize, &dwBufferSize)) {
            setError(QLatin1String("QLocalServerPrivate::addListener"));
            CloseHandle(hToken);
            return false;
        }

        dwBufferSize = 0;
        GetTokenInformation(hToken, TokenPrimaryGroup, 0, 0, &dwBufferSize);
        tokenGroupBuffer.fill(0, dwBufferSize);
        PTOKEN_PRIMARY_GROUP pTokenGroup = (PTOKEN_PRIMARY_GROUP)tokenGroupBuffer.data();
        if (!GetTokenInformation(hToken, TokenPrimaryGroup, pTokenGroup, dwBufferSize, &dwBufferSize)) {
            setError(QLatin1String("QLocalServerPrivate::addListener"));
            CloseHandle(hToken);
            return false;
        }
        CloseHandle(hToken);

#ifdef QLOCALSERVER_DEBUG
        DWORD groupNameSize;
        DWORD domainNameSize;
        SID_NAME_USE groupNameUse;
        LPWSTR groupNameSid;
        LookupAccountSid(0, pTokenGroup->PrimaryGroup, 0, &groupNameSize, 0, &domainNameSize, &groupNameUse);
        QScopedPointer<wchar_t, QScopedPointerArrayDeleter<wchar_t>> groupName(new wchar_t[groupNameSize]);
        QScopedPointer<wchar_t, QScopedPointerArrayDeleter<wchar_t>> domainName(new wchar_t[domainNameSize]);
        if (LookupAccountSid(0, pTokenGroup->PrimaryGroup, groupName.data(), &groupNameSize, domainName.data(), &domainNameSize, &groupNameUse)) {
            qDebug() << "primary group" << QString::fromWCharArray(domainName.data()) << "\\" << QString::fromWCharArray(groupName.data()) << "type=" << groupNameUse;
        }
        if (ConvertSidToStringSid(pTokenGroup->PrimaryGroup, &groupNameSid)) {
            qDebug() << "primary group SID" << QString::fromWCharArray(groupNameSid) << "valid" << IsValidSid(pTokenGroup->PrimaryGroup);
            LocalFree(groupNameSid);
        }
#endif

        SID_IDENTIFIER_AUTHORITY WorldAuth = { SECURITY_WORLD_SID_AUTHORITY };
        if (!AllocateAndInitializeSid(&WorldAuth, 1, SECURITY_WORLD_RID,
            0, 0, 0, 0, 0, 0, 0,
            &worldSID)) {
            setError(QLatin1String("QLocalServerPrivate::addListener"));
            return false;
        }

        //calculate size of ACL buffer
        DWORD aclSize = sizeof(ACL) + ((sizeof(ACCESS_ALLOWED_ACE)) * 3);
        aclSize += GetLengthSid(pTokenUser->User.Sid) - sizeof(DWORD);
        aclSize += GetLengthSid(pTokenGroup->PrimaryGroup) - sizeof(DWORD);
        aclSize += GetLengthSid(worldSID) - sizeof(DWORD);
        aclSize = (aclSize + (sizeof(DWORD) - 1)) & 0xfffffffc;

        aclBuffer.fill(0, aclSize);
        PACL acl = (PACL)aclBuffer.data();
        InitializeAcl(acl, aclSize, ACL_REVISION_DS);

        if (socketOptions & QLocalServer::UserAccessOption) {
            if (!AddAccessAllowedAce(acl, ACL_REVISION, FILE_ALL_ACCESS, pTokenUser->User.Sid)) {
                setError(QLatin1String("QLocalServerPrivate::addListener"));
                FreeSid(worldSID);
                return false;
            }
        }
        if (socketOptions & QLocalServer::GroupAccessOption) {
            if (!AddAccessAllowedAce(acl, ACL_REVISION, FILE_ALL_ACCESS, pTokenGroup->PrimaryGroup)) {
                setError(QLatin1String("QLocalServerPrivate::addListener"));
                FreeSid(worldSID);
                return false;
            }
        }
        if (socketOptions & QLocalServer::OtherAccessOption) {
            if (!AddAccessAllowedAce(acl, ACL_REVISION, FILE_ALL_ACCESS, worldSID)) {
                setError(QLatin1String("QLocalServerPrivate::addListener"));
                FreeSid(worldSID);
                return false;
            }
        }
        SetSecurityDescriptorOwner(pSD.data(), pTokenUser->User.Sid, FALSE);
        SetSecurityDescriptorGroup(pSD.data(), pTokenGroup->PrimaryGroup, FALSE);
        if (!SetSecurityDescriptorDacl(pSD.data(), TRUE, acl, FALSE)) {
            setError(QLatin1String("QLocalServerPrivate::addListener"));
            FreeSid(worldSID);
            return false;
        }

        sa.lpSecurityDescriptor = pSD.data();
    }

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
                 &sa);

    if (listener.handle == INVALID_HANDLE_VALUE) {
        setError(QLatin1String("QLocalServerPrivate::addListener"));
        listeners.removeLast();
        return false;
    }

    if (worldSID)
        FreeSid(worldSID);

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

bool QLocalServerPrivate::listen(qintptr)
{
    qWarning("QLocalServer::listen(qintptr) is not supported on Windows QTBUG-24230");
    return false;
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
