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

#include "qplatformdefs.h"
#include "qmutex.h"
#include "qstring.h"

#ifndef QT_NO_THREAD
#include "qatomic.h"
#include "qmutex_p.h"

#include <errno.h>

#if defined(Q_OS_VXWORKS) && defined(wakeup)
#undef wakeup
#endif

#if defined(Q_OS_MAC)
# include <mach/mach.h>
# include <mach/task.h>
#elif defined(Q_OS_LINUX)
# include <linux/futex.h>
# include <sys/syscall.h>
# include <unistd.h>
# include <QtCore/qelapsedtimer.h>
#endif

QT_BEGIN_NAMESPACE

#if !defined(Q_OS_LINUX)
static void report_error(int code, const char *where, const char *what)
{
    if (code != 0)
        qWarning("%s: %s failure: %s", where, what, qPrintable(qt_error_string(code)));
}
#endif


QMutexPrivate::QMutexPrivate(QMutex::RecursionMode mode)
    : QMutexData(mode), maximumSpinTime(MaximumSpinTimeThreshold), averageWaitTime(0), owner(0), count(0)
{
#if !defined(Q_OS_LINUX)
    wakeup = false;
    report_error(pthread_mutex_init(&mutex, NULL), "QMutex", "mutex init");
    report_error(pthread_cond_init(&cond, NULL), "QMutex", "cv init");
#endif
}

QMutexPrivate::~QMutexPrivate()
{
#if !defined(Q_OS_LINUX)
    report_error(pthread_cond_destroy(&cond), "QMutex", "cv destroy");
    report_error(pthread_mutex_destroy(&mutex), "QMutex", "mutex destroy");
#endif
}

#if defined(Q_OS_LINUX)

static inline int _q_futex(volatile int *addr, int op, int val, const struct timespec *timeout, int *addr2, int val2)
{
    return syscall(SYS_futex, addr, op, val, timeout, addr2, val2);
}

bool QMutexPrivate::wait(int timeout)
{
    struct timespec ts, *pts = 0;
    QElapsedTimer timer;
    if (timeout >= 0) {
        ts.tv_nsec = ((timeout % 1000) * 1000) * 1000;
        ts.tv_sec = (timeout / 1000);
        pts = &ts;
        timer.start();
    }
    while (contenders.fetchAndStoreAcquire(2) > 0) {
        int r = _q_futex(&contenders._q_value, FUTEX_WAIT, 2, pts, 0, 0);
        if (r != 0 && errno == ETIMEDOUT)
            return false;

        if (pts) {
            // recalculate the timeout
            qint64 xtimeout = timeout * 1000 * 1000;
            xtimeout -= timer.nsecsElapsed();
            if (xtimeout < 0) {
                // timer expired after we returned
                return false;
            }

            ts.tv_sec = xtimeout / Q_INT64_C(1000) / 1000 / 1000;
            ts.tv_nsec = xtimeout % (Q_INT64_C(1000) * 1000 * 1000);
        }
    }
    return true;
}

void QMutexPrivate::wakeUp()
{
    (void) contenders.fetchAndStoreRelease(0);
    (void) _q_futex(&contenders._q_value, FUTEX_WAKE, 1, 0, 0, 0);
}

#else // !Q_OS_LINUX

bool QMutexPrivate::wait(int timeout)
{
    if (contenders.fetchAndAddAcquire(1) == 0) {
        // lock acquired without waiting
        return true;
    }
    report_error(pthread_mutex_lock(&mutex), "QMutex::lock", "mutex lock");
    int errorCode = 0;
    while (!wakeup) {
        if (timeout < 0) {
            errorCode = pthread_cond_wait(&cond, &mutex);
        } else {
            struct timeval tv;
            gettimeofday(&tv, 0);

            timespec ti;
            ti.tv_nsec = (tv.tv_usec + (timeout % 1000) * 1000) * 1000;
            ti.tv_sec = tv.tv_sec + (timeout / 1000) + (ti.tv_nsec / 1000000000);
            ti.tv_nsec %= 1000000000;

            errorCode = pthread_cond_timedwait(&cond, &mutex, &ti);
        }
        if (errorCode) {
            if (errorCode == ETIMEDOUT) {
                if (wakeup)
                    errorCode = 0;
                break;
            }
            report_error(errorCode, "QMutex::lock()", "cv wait");
        }
    }
    wakeup = false;
    report_error(pthread_mutex_unlock(&mutex), "QMutex::lock", "mutex unlock");
    contenders.deref();
    return errorCode == 0;
}

void QMutexPrivate::wakeUp()
{
    report_error(pthread_mutex_lock(&mutex), "QMutex::unlock", "mutex lock");
    wakeup = true;
    report_error(pthread_cond_signal(&cond), "QMutex::unlock", "cv signal");
    report_error(pthread_mutex_unlock(&mutex), "QMutex::unlock", "mutex unlock");
}

#endif // !Q_OS_LINUX

QT_END_NAMESPACE

#endif // QT_NO_THREAD
