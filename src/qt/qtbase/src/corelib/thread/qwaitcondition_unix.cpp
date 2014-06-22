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

#include "qplatformdefs.h"
#include "qwaitcondition.h"
#include "qmutex.h"
#include "qreadwritelock.h"
#include "qatomic.h"
#include "qstring.h"
#include "qelapsedtimer.h"
#include "private/qcore_unix_p.h"

#include "qmutex_p.h"
#include "qreadwritelock_p.h"

#include <errno.h>
#include <sys/time.h>
#include <time.h>

#ifndef QT_NO_THREAD

QT_BEGIN_NAMESPACE

#ifdef Q_OS_ANDROID
// Android lacks pthread_condattr_setclock, but it does have a nice function
// for relative waits. Use weakref so we can determine at runtime whether it is
// present.
static int local_cond_timedwait_relative(pthread_cond_t*, pthread_mutex_t *, const timespec *)
__attribute__((weakref("__pthread_cond_timedwait_relative")));
#endif

static void report_error(int code, const char *where, const char *what)
{
    if (code != 0)
        qWarning("%s: %s failure: %s", where, what, qPrintable(qt_error_string(code)));
}

void qt_initialize_pthread_cond(pthread_cond_t *cond, const char *where)
{
    pthread_condattr_t condattr;

    pthread_condattr_init(&condattr);
#if !defined(Q_OS_MAC) && !defined(Q_OS_ANDROID) && (_POSIX_MONOTONIC_CLOCK-0 >= 0)
    if (QElapsedTimer::clockType() == QElapsedTimer::MonotonicClock)
        pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
#endif
    report_error(pthread_cond_init(cond, &condattr), where, "cv init");
    pthread_condattr_destroy(&condattr);
}

void qt_abstime_for_timeout(timespec *ts, int timeout)
{
#ifdef Q_OS_MAC
    // on Mac, qt_gettime() (on qelapsedtimer_mac.cpp) returns ticks related to the Mach absolute time
    // that doesn't work with pthread
    // Mac also doesn't have clock_gettime
    struct timeval tv;
    gettimeofday(&tv, 0);
    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;
#else
    *ts = qt_gettime();
#endif

    ts->tv_sec += timeout / 1000;
    ts->tv_nsec += timeout % 1000 * Q_UINT64_C(1000) * 1000;
    normalizedTimespec(*ts);
}

class QWaitConditionPrivate {
public:
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int waiters;
    int wakeups;

    int wait_relative(unsigned long time)
    {
        timespec ti;
#ifdef Q_OS_ANDROID
        if (Q_LIKELY(local_cond_timedwait_relative)) {
            ti.tv_sec = time / 1000;
            ti.tv_nsec = time % 1000 * Q_UINT64_C(1000) * 1000;
            return local_cond_timedwait_relative(&cond, &mutex, &ti);
        }
#endif
        qt_abstime_for_timeout(&ti, time);
        return pthread_cond_timedwait(&cond, &mutex, &ti);
    }

    bool wait(unsigned long time)
    {
        int code;
        forever {
            if (time != ULONG_MAX) {
                code = wait_relative(time);
            } else {
                code = pthread_cond_wait(&cond, &mutex);
            }
            if (code == 0 && wakeups == 0) {
                // many vendors warn of spurious wakeups from
                // pthread_cond_wait(), especially after signal delivery,
                // even though POSIX doesn't allow for it... sigh
                continue;
            }
            break;
        }

        Q_ASSERT_X(waiters > 0, "QWaitCondition::wait", "internal error (waiters)");
        --waiters;
        if (code == 0) {
            Q_ASSERT_X(wakeups > 0, "QWaitCondition::wait", "internal error (wakeups)");
            --wakeups;
        }
        report_error(pthread_mutex_unlock(&mutex), "QWaitCondition::wait()", "mutex unlock");

        if (code && code != ETIMEDOUT)
            report_error(code, "QWaitCondition::wait()", "cv wait");

        return (code == 0);
    }
};


QWaitCondition::QWaitCondition()
{
    d = new QWaitConditionPrivate;
    report_error(pthread_mutex_init(&d->mutex, NULL), "QWaitCondition", "mutex init");
    qt_initialize_pthread_cond(&d->cond, "QWaitCondition");
    d->waiters = d->wakeups = 0;
}


QWaitCondition::~QWaitCondition()
{
    report_error(pthread_cond_destroy(&d->cond), "QWaitCondition", "cv destroy");
    report_error(pthread_mutex_destroy(&d->mutex), "QWaitCondition", "mutex destroy");
    delete d;
}

void QWaitCondition::wakeOne()
{
    report_error(pthread_mutex_lock(&d->mutex), "QWaitCondition::wakeOne()", "mutex lock");
    d->wakeups = qMin(d->wakeups + 1, d->waiters);
    report_error(pthread_cond_signal(&d->cond), "QWaitCondition::wakeOne()", "cv signal");
    report_error(pthread_mutex_unlock(&d->mutex), "QWaitCondition::wakeOne()", "mutex unlock");
}

void QWaitCondition::wakeAll()
{
    report_error(pthread_mutex_lock(&d->mutex), "QWaitCondition::wakeAll()", "mutex lock");
    d->wakeups = d->waiters;
    report_error(pthread_cond_broadcast(&d->cond), "QWaitCondition::wakeAll()", "cv broadcast");
    report_error(pthread_mutex_unlock(&d->mutex), "QWaitCondition::wakeAll()", "mutex unlock");
}

bool QWaitCondition::wait(QMutex *mutex, unsigned long time)
{
    if (! mutex)
        return false;
    if (mutex->isRecursive()) {
        qWarning("QWaitCondition: cannot wait on recursive mutexes");
        return false;
    }

    report_error(pthread_mutex_lock(&d->mutex), "QWaitCondition::wait()", "mutex lock");
    ++d->waiters;
    mutex->unlock();

    bool returnValue = d->wait(time);

    mutex->lock();

    return returnValue;
}

bool QWaitCondition::wait(QReadWriteLock *readWriteLock, unsigned long time)
{
    if (!readWriteLock || readWriteLock->d->accessCount == 0)
        return false;
    if (readWriteLock->d->accessCount < -1) {
        qWarning("QWaitCondition: cannot wait on QReadWriteLocks with recursive lockForWrite()");
        return false;
    }

    report_error(pthread_mutex_lock(&d->mutex), "QWaitCondition::wait()", "mutex lock");
    ++d->waiters;

    int previousAccessCount = readWriteLock->d->accessCount;
    readWriteLock->unlock();

    bool returnValue = d->wait(time);

    if (previousAccessCount < 0)
        readWriteLock->lockForWrite();
    else
        readWriteLock->lockForRead();

    return returnValue;
}

QT_END_NAMESPACE

#endif // QT_NO_THREAD
