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
#include "qwaitcondition.h"
#include "qmutex.h"
#include "qreadwritelock.h"
#include "qatomic.h"
#include "qstring.h"
#include "qelapsedtimer.h"

#include "qmutex_p.h"
#include "qreadwritelock_p.h"

#ifndef QT_NO_THREAD

QT_BEGIN_NAMESPACE

static void report_error(int err, const char *where, const char *what)
{
    if (err != KErrNone)
        qWarning("%s: %s failure: %d", where, what, err);
}

class QWaitConditionPrivate {
public:
    RMutex mutex;
    RCondVar cond;
    int waiters;
    int wakeups;

    QWaitConditionPrivate()
    : waiters(0), wakeups(0)
    {
        qt_symbian_throwIfError(mutex.CreateLocal());
        int err = cond.CreateLocal();
        if (err != KErrNone) {
            mutex.Close();
            qt_symbian_throwIfError(err);
        }
    }

    ~QWaitConditionPrivate()
    {
        cond.Close();
        mutex.Close();
    }

    bool wait(unsigned long time)
    {
        TInt err = KErrNone;
        if (time == ULONG_MAX) {
            // untimed wait, loop because RCondVar::Wait may return before the condition is triggered
            do {
                err = cond.Wait(mutex);
            } while (err == KErrNone && wakeups == 0);
        } else {
            unsigned long maxWait = KMaxTInt / 1000;
            QElapsedTimer waitTimer;
            do {
                waitTimer.start();
                unsigned long waitTime = qMin(maxWait, time);
                // wait at least 1ms, as 0 means no wait
                err = cond.TimedWait(mutex, qMax(1ul, waitTime) * 1000);
                // RCondVar::TimedWait may return before the condition is triggered, update the timeout with actual wait time
                time -= qMin((unsigned long)waitTimer.elapsed(), waitTime);
            } while ((err == KErrNone && wakeups == 0) || (err == KErrTimedOut && time > 0));
        }

        Q_ASSERT_X(waiters > 0, "QWaitCondition::wait", "internal error (waiters)");
        --waiters;
        if (err == KErrNone) {
            Q_ASSERT_X(wakeups > 0, "QWaitCondition::wait", "internal error (wakeups)");
            --wakeups;
        }

        // if err is KErrGeneral it signals that the RCondVar is closed along with the mutex and that this has been deleted
        // we must not access any member variables in this case
        if (err != KErrGeneral)
            mutex.Signal();

        if (err && err != KErrTimedOut)
            report_error(err, "QWaitCondition::wait()", "cv wait");
        return err == KErrNone;
    }
};

QWaitCondition::QWaitCondition()
{
    d = new QWaitConditionPrivate;
}

QWaitCondition::~QWaitCondition()
{
    delete d;
}

void QWaitCondition::wakeOne()
{
    d->mutex.Wait();
    d->wakeups = qMin(d->wakeups + 1, d->waiters);
    d->cond.Signal();
    d->mutex.Signal();
}

void QWaitCondition::wakeAll()
{
    d->mutex.Wait();
    d->wakeups = d->waiters;
    d->cond.Broadcast();
    d->mutex.Signal();
}

bool QWaitCondition::wait(QMutex *mutex, unsigned long time)
{
    if (! mutex)
        return false;
    if (mutex->d->recursive) {
        qWarning("QWaitCondition: cannot wait on recursive mutexes");
        return false;
    }

    d->mutex.Wait();
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

    d->mutex.Wait();
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
