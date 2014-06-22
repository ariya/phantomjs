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

#include "qsystemsemaphore.h"
#include "qsystemsemaphore_p.h"

#include <qdebug.h>
#include <qfile.h>
#include <qcoreapplication.h>

#ifndef QT_NO_SYSTEMSEMAPHORE

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <errno.h>

#include "private/qcore_unix_p.h"

// OpenBSD 4.2 doesn't define EIDRM, see BUGS section:
// http://www.openbsd.org/cgi-bin/man.cgi?query=semop&manpath=OpenBSD+4.2
#if defined(Q_OS_OPENBSD) && !defined(EIDRM)
#define EIDRM EINVAL
#endif

QT_BEGIN_NAMESPACE

QSystemSemaphorePrivate::QSystemSemaphorePrivate() :
        semaphore(-1), createdFile(false),
        createdSemaphore(false), unix_key(-1), error(QSystemSemaphore::NoError)
{
}

void QSystemSemaphorePrivate::setErrorString(const QString &function)
{
    // EINVAL is handled in functions so they can give better error strings
    switch (errno) {
    case EPERM:
    case EACCES:
        errorString = QCoreApplication::translate("QSystemSemaphore", "%1: permission denied").arg(function);
        error = QSystemSemaphore::PermissionDenied;
        break;
    case EEXIST:
        errorString = QCoreApplication::translate("QSystemSemaphore", "%1: already exists").arg(function);
        error = QSystemSemaphore::AlreadyExists;
        break;
    case ENOENT:
        errorString = QCoreApplication::translate("QSystemSemaphore", "%1: does not exist").arg(function);
        error = QSystemSemaphore::NotFound;
        break;
    case ERANGE:
    case ENOSPC:
        errorString = QCoreApplication::translate("QSystemSemaphore", "%1: out of resources").arg(function);
        error = QSystemSemaphore::OutOfResources;
        break;
    default:
        errorString = QCoreApplication::translate("QSystemSemaphore", "%1: unknown error %2").arg(function).arg(errno);
        error = QSystemSemaphore::UnknownError;
#if defined QSYSTEMSEMAPHORE_DEBUG
        qDebug() << errorString << "key" << key << "errno" << errno << EINVAL;
#endif
    }
}

/*!
    \internal

    Setup unix_key
 */
key_t QSystemSemaphorePrivate::handle(QSystemSemaphore::AccessMode mode)
{
    if (key.isEmpty()){
        errorString = QCoreApplication::tr("%1: key is empty", "QSystemSemaphore").arg(QLatin1String("QSystemSemaphore::handle:"));
        error = QSystemSemaphore::KeyError;
        return -1;
    }

    // ftok requires that an actual file exists somewhere
    if (-1 != unix_key)
        return unix_key;

    // Create the file needed for ftok
    int built = QSharedMemoryPrivate::createUnixKeyFile(fileName);
    if (-1 == built) {
        errorString = QCoreApplication::tr("%1: unable to make key", "QSystemSemaphore").arg(QLatin1String("QSystemSemaphore::handle:"));
        error = QSystemSemaphore::KeyError;
        return -1;
    }
    createdFile = (1 == built);

    // Get the unix key for the created file
    unix_key = ftok(QFile::encodeName(fileName).constData(), 'Q');
    if (-1 == unix_key) {
        errorString = QCoreApplication::tr("%1: ftok failed", "QSystemSemaphore").arg(QLatin1String("QSystemSemaphore::handle:"));
        error = QSystemSemaphore::KeyError;
        return -1;
    }

    // Get semaphore
    semaphore = semget(unix_key, 1, 0600 | IPC_CREAT | IPC_EXCL);
    if (-1 == semaphore) {
        if (errno == EEXIST)
            semaphore = semget(unix_key, 1, 0600 | IPC_CREAT);
        if (-1 == semaphore) {
            setErrorString(QLatin1String("QSystemSemaphore::handle"));
            cleanHandle();
            return -1;
        }
    } else {
        createdSemaphore = true;
        // Force cleanup of file, it is possible that it can be left over from a crash
        createdFile = true;
    }

    if (mode == QSystemSemaphore::Create) {
        createdSemaphore = true;
        createdFile = true;
    }

    // Created semaphore so initialize its value.
    if (createdSemaphore && initialValue >= 0) {
        qt_semun init_op;
        init_op.val = initialValue;
        if (-1 == semctl(semaphore, 0, SETVAL, init_op)) {
            setErrorString(QLatin1String("QSystemSemaphore::handle"));
            cleanHandle();
            return -1;
        }
    }

    return unix_key;
}

/*!
    \internal

    Cleanup the unix_key
 */
void QSystemSemaphorePrivate::cleanHandle()
{
    unix_key = -1;

    // remove the file if we made it
    if (createdFile) {
        QFile::remove(fileName);
        createdFile = false;
    }

    if (createdSemaphore) {
        if (-1 != semaphore) {
            if (-1 == semctl(semaphore, 0, IPC_RMID, 0)) {
                setErrorString(QLatin1String("QSystemSemaphore::cleanHandle"));
#if defined QSYSTEMSEMAPHORE_DEBUG
                qDebug() << QLatin1String("QSystemSemaphore::cleanHandle semctl failed.");
#endif
            }
            semaphore = -1;
        }
        createdSemaphore = false;
    }
}

/*!
    \internal
 */
bool QSystemSemaphorePrivate::modifySemaphore(int count)
{
    if (-1 == handle())
        return false;

    struct sembuf operation;
    operation.sem_num = 0;
    operation.sem_op = count;
    operation.sem_flg = SEM_UNDO;

    int res;
    EINTR_LOOP(res, semop(semaphore, &operation, 1));
    if (-1 == res) {
        // If the semaphore was removed be nice and create it and then modifySemaphore again
        if (errno == EINVAL || errno == EIDRM) {
            semaphore = -1;
            cleanHandle();
            handle();
            return modifySemaphore(count);
        }
        setErrorString(QLatin1String("QSystemSemaphore::modifySemaphore"));
#if defined QSYSTEMSEMAPHORE_DEBUG
        qDebug() << QLatin1String("QSystemSemaphore::modify failed") << count << semctl(semaphore, 0, GETVAL) << errno << EIDRM << EINVAL;
#endif
        return false;
    }

    clearError();
    return true;
}


QT_END_NAMESPACE

#endif // QT_NO_SYSTEMSEMAPHORE
