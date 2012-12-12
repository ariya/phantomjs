/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QSYSTEMSEMAPHORE_P_H
#define QSYSTEMSEMAPHORE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qsystemsemaphore.h"

#ifndef QT_NO_SYSTEMSEMAPHORE

#include "qsharedmemory_p.h"
#ifndef Q_OS_WINCE
#  include <sys/types.h>
#endif
#ifdef QT_POSIX_IPC
#  include <semaphore.h>
#endif

#ifdef Q_OS_SYMBIAN
class RSemaphore;
#endif

QT_BEGIN_NAMESPACE

class QSystemSemaphorePrivate
{
public:
    QSystemSemaphorePrivate();

    inline QString makeKeyFileName() const
    {
        return QSharedMemoryPrivate::makePlatformSafeKey(key, QLatin1String("qipc_systemsem_"));
    }

#ifdef Q_OS_WIN
    HANDLE handle(QSystemSemaphore::AccessMode mode = QSystemSemaphore::Open);
    void setErrorString(const QString &function);
#elif defined(Q_OS_SYMBIAN)
    int handle(QSystemSemaphore::AccessMode mode = QSystemSemaphore::Open);
    void setErrorString(const QString &function,int err = 0);
#elif defined(QT_POSIX_IPC)
    bool handle(QSystemSemaphore::AccessMode mode = QSystemSemaphore::Open);
    void setErrorString(const QString &function);
#else
    key_t handle(QSystemSemaphore::AccessMode mode = QSystemSemaphore::Open);
    void setErrorString(const QString &function);
#endif
    void cleanHandle();
    bool modifySemaphore(int count);

    QString key;
    QString fileName;
    int initialValue;
#ifdef Q_OS_WIN
    HANDLE semaphore;
    HANDLE semaphoreLock;
#elif defined(Q_OS_SYMBIAN)
    RSemaphore semaphore;
#elif defined(QT_POSIX_IPC)
    sem_t *semaphore;
    bool createdSemaphore;
#else
    key_t unix_key;
    int semaphore;
    bool createdFile;
    bool createdSemaphore;
#endif
    QString errorString;
    QSystemSemaphore::SystemSemaphoreError error;
};

QT_END_NAMESPACE

#endif // QT_NO_SYSTEMSEMAPHORE

#endif // QSYSTEMSEMAPHORE_P_H
