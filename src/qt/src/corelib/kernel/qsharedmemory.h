/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QSHAREDMEMORY_H
#define QSHAREDMEMORY_H

#include <QtCore/qobject.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#ifndef QT_NO_SHAREDMEMORY

class QSharedMemoryPrivate;

class Q_CORE_EXPORT QSharedMemory : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSharedMemory)

public:
    enum AccessMode
    {
        ReadOnly,
        ReadWrite
    };

    enum SharedMemoryError
    {
        NoError,
        PermissionDenied,
        InvalidSize,
        KeyError,
        AlreadyExists,
        NotFound,
        LockError,
        OutOfResources,
        UnknownError
    };

    QSharedMemory(QObject *parent = 0);
    QSharedMemory(const QString &key, QObject *parent = 0);
    ~QSharedMemory();

    void setKey(const QString &key);
    QString key() const;
    void setNativeKey(const QString &key);
    QString nativeKey() const;

    bool create(int size, AccessMode mode = ReadWrite);
    int size() const;

    bool attach(AccessMode mode = ReadWrite);
    bool isAttached() const;
    bool detach();

    void *data();
    const void* constData() const;
    const void *data() const;

#ifndef QT_NO_SYSTEMSEMAPHORE
    bool lock();
    bool unlock();
#endif

    SharedMemoryError error() const;
    QString errorString() const;

private:
    Q_DISABLE_COPY(QSharedMemory)
};

#endif // QT_NO_SHAREDMEMORY

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSHAREDMEMORY_H

