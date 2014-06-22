/****************************************************************************
**
** Copyright (C) 2012 Collabora Ltd, author <robin.burchell@collabora.co.uk>
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
#include <qdebug.h>

#ifndef QT_NO_SHAREDMEMORY
QT_BEGIN_NAMESPACE

QSharedMemoryPrivate::QSharedMemoryPrivate()
    : QObjectPrivate(), memory(0), size(0), error(QSharedMemory::NoError),
#ifndef QT_NO_SYSTEMSEMAPHORE
      systemSemaphore(QString()), lockedByMe(false),
#endif
      unix_key(0)
{
}

void QSharedMemoryPrivate::setErrorString(const QString &function)
{
    Q_UNUSED(function);
    qWarning() << Q_FUNC_INFO << "Not yet implemented on Android";
}

key_t QSharedMemoryPrivate::handle()
{
    qWarning() << Q_FUNC_INFO << "Not yet implemented on Android";
    return 0;
}

#endif // QT_NO_SHAREDMEMORY

#if !(defined(QT_NO_SHAREDMEMORY) && defined(QT_NO_SYSTEMSEMAPHORE))
int QSharedMemoryPrivate::createUnixKeyFile(const QString &fileName)
{
    Q_UNUSED(fileName);
    qWarning() << Q_FUNC_INFO << "Not yet implemented on Android";
    return 0;
}
#endif // QT_NO_SHAREDMEMORY && QT_NO_SYSTEMSEMAPHORE

#ifndef QT_NO_SHAREDMEMORY

bool QSharedMemoryPrivate::cleanHandle()
{
    qWarning() << Q_FUNC_INFO << "Not yet implemented on Android";
    return true;
}

bool QSharedMemoryPrivate::create(int size)
{
    Q_UNUSED(size);
    qWarning() << Q_FUNC_INFO << "Not yet implemented on Android";
    return false;
}

bool QSharedMemoryPrivate::attach(QSharedMemory::AccessMode mode)
{
    Q_UNUSED(mode);
    qWarning() << Q_FUNC_INFO << "Not yet implemented on Android";
    return false;
}

bool QSharedMemoryPrivate::detach()
{
    qWarning() << Q_FUNC_INFO << "Not yet implemented on Android";
    return false;
}


QT_END_NAMESPACE

#endif // QT_NO_SHAREDMEMORY
