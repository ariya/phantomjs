/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qsharedmemory.h"
#include "qsharedmemory_p.h"

#include <qdebug.h>

#ifndef QT_NO_SHAREDMEMORY

//#define QSHAREDMEMORY_DEBUG

QT_BEGIN_NAMESPACE

QSharedMemoryPrivate::QSharedMemoryPrivate()
    : QObjectPrivate(), memory(0), size(0), error(QSharedMemory::NoError),
#ifndef QT_NO_SYSTEMSEMAPHORE
      systemSemaphore(QString()), lockedByMe(false),
#endif
      hand(0)
{
}

void QSharedMemoryPrivate::setErrorString(const QString &function)
{
    DWORD windowsError = GetLastError();
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
#ifdef QSHAREDMEMORY_DEBUG
        qDebug() << errorString << "key" << key;
#endif
        break;
    }
}

HANDLE QSharedMemoryPrivate::handle()
{
    if (!hand) {
        // don't allow making handles on empty keys
        if (nativeKey.isEmpty()) {
            error = QSharedMemory::KeyError;
            errorString = QSharedMemory::tr("%1: key is empty").arg(QLatin1String("QSharedMemory::handle"));
            return false;
        }
#ifndef Q_OS_WINCE
        hand = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, (wchar_t*)nativeKey.utf16());
#else
        // This works for opening a mapping too, but always opens it with read/write access in
        // attach as it seems.
        hand = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, 0, (wchar_t*)nativeKey.utf16());
#endif
        if (!hand)
            setErrorString(QLatin1String("QSharedMemory::handle"));
    }

    return hand;
}

void QSharedMemoryPrivate::cleanHandle()
{
    if (hand != 0 && !CloseHandle(hand))
        setErrorString(QLatin1String("QSharedMemory::cleanHandle"));
    hand = 0;
}

bool QSharedMemoryPrivate::create(int size)
{
    if (nativeKey.isEmpty()) {
        error = QSharedMemory::KeyError;
        errorString = QSharedMemory::tr("%1: key is empty").arg(QLatin1String("QSharedMemory::create"));
        return false;
    }

    // Create the file mapping.
    hand = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, size, (wchar_t*)nativeKey.utf16());
    setErrorString(QLatin1String("QSharedMemory::create"));

    // hand is valid when it already exists unlike unix so explicitly check
    return !(error == QSharedMemory::AlreadyExists || !hand);
}

bool QSharedMemoryPrivate::attach(QSharedMemory::AccessMode mode)
{
    // Grab a pointer to the memory block
    int permissions = (mode == QSharedMemory::ReadOnly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS);
    memory = (void *)MapViewOfFile(handle(), permissions, 0, 0, 0);
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
        errorString = QSharedMemory::tr("%1: size query failed").arg(QLatin1String("QSharedMemory::attach"));
        return false;
    }
    size = info.RegionSize;

    return true;
}

bool QSharedMemoryPrivate::detach()
{
    // umap memory
    if (!UnmapViewOfFile(memory)) {
        setErrorString(QLatin1String("QSharedMemory::detach"));
        return false;
    }
    memory = 0;
    size = 0;

    // close handle
    cleanHandle();

    return true;
}

QT_END_NAMESPACE

#endif // QT_NO_SHAREDMEMORY
