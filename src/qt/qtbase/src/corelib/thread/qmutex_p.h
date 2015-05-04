/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2012 Intel Corporation
** Copyright (C) 2012 Olivier Goffart <ogoffart@woboq.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMUTEX_P_H
#define QMUTEX_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qmutex.cpp, qmutex_unix.cpp, and qmutex_win.cpp.  This header
// file may change from version to version without notice, or even be
// removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qmutex.h>
#include <QtCore/qatomic.h>

#if defined(Q_OS_MAC)
# include <mach/semaphore.h>
#endif

#if defined(Q_OS_LINUX) && !defined(QT_LINUXBASE)
// use Linux mutexes everywhere except for LSB builds
#  define QT_LINUX_FUTEX
#endif

struct timespec;

QT_BEGIN_NAMESPACE

class QMutexData
{
public:
    bool recursive;
    QMutexData(QMutex::RecursionMode mode = QMutex::NonRecursive)
        : recursive(mode == QMutex::Recursive) {}
};

#if !defined(QT_LINUX_FUTEX)
class QMutexPrivate : public QMutexData
{
public:
    ~QMutexPrivate();
    QMutexPrivate();

    bool wait(int timeout = -1);
    void wakeUp() Q_DECL_NOTHROW;

    // Control the lifetime of the privates
    QAtomicInt refCount;
    int id;

    bool ref() {
        Q_ASSERT(refCount.load() >= 0);
        int c;
        do {
            c = refCount.load();
            if (c == 0)
                return false;
        } while (!refCount.testAndSetRelaxed(c, c + 1));
        Q_ASSERT(refCount.load() >= 0);
        return true;
    }
    void deref() {
        Q_ASSERT(refCount.load() >= 0);
        if (!refCount.deref())
            release();
        Q_ASSERT(refCount.load() >= 0);
    }
    void release();
    static QMutexPrivate *allocate();

    QAtomicInt waiters; // Number of threads waiting on this mutex. (may be offset by -BigNumber)
    QAtomicInt possiblyUnlocked; /* Boolean indicating that a timed wait timed out.
                                    When it is true, a reference is held.
                                    It is there to avoid a race that happens if unlock happens right
                                    when the mutex is unlocked.
                                  */
    enum { BigNumber = 0x100000 }; //Must be bigger than the possible number of waiters (number of threads)
    void derefWaiters(int value) Q_DECL_NOTHROW;

    //platform specific stuff
#if defined(Q_OS_MAC)
    semaphore_t mach_semaphore;
#elif defined(Q_OS_UNIX)
    bool wakeup;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
#elif defined(Q_OS_WIN)
    Qt::HANDLE event;
#endif
};
#endif //QT_LINUX_FUTEX


#ifdef Q_OS_UNIX
// helper functions for qmutex_unix.cpp and qwaitcondition_unix.cpp
// they are in qwaitcondition_unix.cpp actually
void qt_initialize_pthread_cond(pthread_cond_t *cond, const char *where);
void qt_abstime_for_timeout(struct timespec *ts, int timeout);
#endif

QT_END_NAMESPACE

#endif // QMUTEX_P_H
