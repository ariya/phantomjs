/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qsharedmemory.h"
#include "qsharedmemory_p.h"
#include "qsystemsemaphore.h"
#include <qdebug.h>
#include <qt_windows.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_SHAREDMEMORY

QSharedMemoryPrivate::QSharedMemoryPrivate() : QObjectPrivate(),
        memory(0), size(0), error(QSharedMemory::NoError),
           systemSemaphore(QString()), lockedByMe(false), hand(0)
{
}

void QSharedMemoryPrivate::setErrorString(const QString &function)
{
    BOOL windowsError = GetLastError();
    if (windowsError == 0)
        return;
    switch (windowsError) {
    case ERROR_ALREADY_EXISTS:
        error = QSharedMemory::AlreadyExists;
        errorString = QSharedMemory::tr("%1: already exists").arg(function);
    break;
    case ERROR_FILE_NOT_FOUND:
#ifdef Q_OS_WINCE
        // This happens on CE only if no file is present as CreateFileMappingW
        // bails out with this error code
    case ERROR_INVALID_PARAMETER:
#endif
        error = QSharedMemory::NotFound;
        errorString = QSharedMemory::tr("%1: doesn't exist").arg(function);
        break;
    case ERROR_COMMITMENT_LIMIT:
        error = QSharedMemory::InvalidSize;
        errorString = QSharedMemory::tr("%1: invalid size").arg(function);
        break;
    case ERROR_NO_SYSTEM_RESOURCES:
    case ERROR_NOT_ENOUGH_MEMORY:
        error = QSharedMemory::OutOfResources;
        errorString = QSharedMemory::tr("%1: out of resources").arg(function);
        break;
    case ERROR_ACCESS_DENIED:
        error = QSharedMemory::PermissionDenied;
        errorString = QSharedMemory::tr("%1: permission denied").arg(function);
        break;
    default:
        errorString = QSharedMemory::tr("%1: unknown error %2").arg(function).arg(windowsError);
        error = QSharedMemory::UnknownError;
#if defined QSHAREDMEMORY_DEBUG
        qDebug() << errorString << "key" << key;
#endif
    }
}

HANDLE QSharedMemoryPrivate::handle()
{
    if (!hand) {
        QString function = QLatin1String("QSharedMemory::handle");
        if (nativeKey.isEmpty()) {
            error = QSharedMemory::KeyError;
            errorString = QSharedMemory::tr("%1: unable to make key").arg(function);
            return 0;
        }
#if defined(Q_OS_WINPHONE)
        Q_UNIMPLEMENTED();
        hand = 0;
#elif defined(Q_OS_WINRT)
        hand = CreateFileMappingFromApp(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, (PCWSTR)nativeKey.utf16());
#elif defined(Q_OS_WINCE)
        // This works for opening a mapping too, but always opens it with read/write access in
        // attach as it seems.
        hand = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, 0, (wchar_t*)nativeKey.utf16());
#else
        hand = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, (wchar_t*)nativeKey.utf16());
#endif
        if (!hand) {
            setErrorString(function);
            return 0;
        }
    }
    return hand;
}

bool QSharedMemoryPrivate::cleanHandle()
{
    if (hand != 0 && !CloseHandle(hand)) {
        hand = 0;
        setErrorString(QLatin1String("QSharedMemory::cleanHandle"));
        return false;
    }
    hand = 0;
    return true;
}

bool QSharedMemoryPrivate::create(int size)
{
    QString function = QLatin1String("QSharedMemory::create");
    if (nativeKey.isEmpty()) {
        error = QSharedMemory::KeyError;
        errorString = QSharedMemory::tr("%1: key error").arg(function);
        return false;
    }

    // Create the file mapping.
#if defined(Q_OS_WINPHONE)
    Q_UNIMPLEMENTED();
    hand = 0;
#elif defined(Q_OS_WINRT)
    hand = CreateFileMappingFromApp(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, size, (PCWSTR)nativeKey.utf16());
#else
    hand = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, size, (wchar_t*)nativeKey.utf16());
#endif
    setErrorString(function);

    // hand is valid when it already exists unlike unix so explicitly check
    if (error == QSharedMemory::AlreadyExists || !hand)
        return false;

    return true;
}

bool QSharedMemoryPrivate::attach(QSharedMemory::AccessMode mode)
{
    // Grab a pointer to the memory block
    int permissions = (mode == QSharedMemory::ReadOnly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS);
#if defined(Q_OS_WINPHONE)
    Q_UNIMPLEMENTED();
    memory = 0;
#elif defined(Q_OS_WINRT)
    memory = (void *)MapViewOfFileFromApp(handle(), permissions, 0, 0);
#else
    memory = (void *)MapViewOfFile(handle(), permissions, 0, 0, 0);
#endif
    if (0 == memory) {
        setErrorString(QLatin1String("QSharedMemory::attach"));
        cleanHandle();
        return false;
    }

    // Grab the size of the memory we have been given (a multiple of 4K on windows)
    MEMORY_BASIC_INFORMATION info;
    if (!VirtualQuery(memory, &info, sizeof(info))) {
        // Windows doesn't set an error code on this one,
        // it should only be a kernel memory error.
        error = QSharedMemory::UnknownError;
        errorString = QSharedMemory::tr("%1: size query failed").arg(QLatin1String("QSharedMemory::attach: "));
        return false;
    }
    size = info.RegionSize;

    return true;
}

bool QSharedMemoryPrivate::detach()
{
    // umap memory
#if defined(Q_OS_WINPHONE)
    Q_UNIMPLEMENTED();
    return false;
#else
    if (!UnmapViewOfFile(memory)) {
        setErrorString(QLatin1String("QSharedMemory::detach"));
        return false;
    }
#endif
    memory = 0;
    size = 0;

    // close handle
    return cleanHandle();
}

#endif //QT_NO_SHAREDMEMORY


QT_END_NAMESPACE
