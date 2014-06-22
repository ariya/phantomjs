/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2012 Intel Corporation
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
#include "qmutex.h"

#ifndef QT_NO_THREAD
#include "qatomic.h"
#include "qmutex_p.h"
#include "qelapsedtimer.h"

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <errno.h>
#include <asm/unistd.h>

#ifndef QT_LINUX_FUTEX
# error "Qt build is broken: qmutex_linux.cpp is being built but futex support is not wanted"
#endif

QT_BEGIN_NAMESPACE

/*
 * QBasicMutex implementation on Linux with futexes
 *
 * QBasicMutex contains one pointer value, which can contain one of four
 * different values:
 *    0x0       unlocked, non-recursive mutex
 *    0x1       locked non-recursive mutex, no waiters
 *    0x3       locked non-recursive mutex, at least one waiter
 *   > 0x3      recursive mutex, points to a QMutexPrivate object
 *
 * LOCKING (non-recursive):
 *
 * A non-recursive mutex starts in the 0x0 state, indicating that it's
 * unlocked. When the first thread attempts to lock it, it will perform a
 * testAndSetAcquire from 0x0 to 0x1. If that succeeds, the caller concludes
 * that it successfully locked the mutex. That happens in fastTryLock().
 *
 * If that testAndSetAcquire fails, QBasicMutex::lockInternal is called.
 *
 * lockInternal will examine the value of the pointer. Otherwise, it will use
 * futexes to sleep and wait for another thread to unlock. To do that, it needs
 * to set a pointer value of 0x3, which indicates that thread is waiting. It
 * does that by a simple fetchAndStoreAcquire operation.
 *
 * If the pointer value was 0x0, it means we succeeded in acquiring the mutex.
 * For other values, it will then call FUTEX_WAIT and with an expected value of
 * 0x3.
 *
 * If the pointer value changed before futex(2) managed to sleep, it will
 * return -1 / EWOULDBLOCK, in which case we have to start over. And even if we
 * are woken up directly by a FUTEX_WAKE, we need to acquire the mutex, so we
 * start over again.
 *
 * UNLOCKING (non-recursive):
 *
 * To unlock, we need to set a value of 0x0 to indicate it's unlocked. The
 * first attempt is a testAndSetRelease operation from 0x1 to 0x0. If that
 * succeeds, we're done.
 *
 * If it fails, unlockInternal() is called. The only possibility is that the
 * mutex value was 0x3, which indicates some other thread is waiting or was
 * waiting in the past. We then set the mutex to 0x0 and perform a FUTEX_WAKE.
 */

static QBasicAtomicInt futexFlagSupport = Q_BASIC_ATOMIC_INITIALIZER(-1);

static int checkFutexPrivateSupport()
{
    int value = 0;
#if defined(FUTEX_PRIVATE_FLAG)
    // check if the kernel supports extra futex flags
    // FUTEX_PRIVATE_FLAG appeared in v2.6.22
    Q_STATIC_ASSERT(FUTEX_PRIVATE_FLAG != 0x80000000);

    // try an operation that has no side-effects: wake up 42 threads
    // futex will return -1 (errno==ENOSYS) if the flag isn't supported
    // there should be no other error conditions
    value = syscall(__NR_futex, &futexFlagSupport,
                    FUTEX_WAKE | FUTEX_PRIVATE_FLAG,
                    42, 0, 0, 0);
    if (value != -1)
        value = FUTEX_PRIVATE_FLAG;
    else
        value = 0;

#else
    value = 0;
#endif
    futexFlagSupport.store(value);
    return value;
}

static inline int futexFlags()
{
    int value = futexFlagSupport.load();
    if (Q_LIKELY(value != -1))
        return value;
    return checkFutexPrivateSupport();
}

static inline int _q_futex(void *addr, int op, int val, const struct timespec *timeout) Q_DECL_NOTHROW
{
    volatile int *int_addr = reinterpret_cast<volatile int *>(addr);
#if Q_BYTE_ORDER == Q_BIG_ENDIAN && QT_POINTER_SIZE == 8
    int_addr++; //We want a pointer to the 32 least significant bit of QMutex::d
#endif
    int *addr2 = 0;
    int val2 = 0;

    // we use __NR_futex because some libcs (like Android's bionic) don't
    // provide SYS_futex etc.
    return syscall(__NR_futex, int_addr, op | futexFlags(), val, timeout, addr2, val2);
}

static inline QMutexData *dummyFutexValue()
{
    return reinterpret_cast<QMutexData *>(quintptr(3));
}

template <bool IsTimed> static inline
bool lockInternal_helper(QBasicAtomicPointer<QMutexData> &d_ptr, int timeout = -1, QElapsedTimer *elapsedTimer = 0) Q_DECL_NOTHROW
{
    if (!IsTimed)
        timeout = -1;

    // we're here because fastTryLock() has just failed
    if (timeout == 0)
        return false;

    struct timespec ts, *pts = 0;
    if (IsTimed && timeout > 0) {
        ts.tv_sec = timeout / 1000;
        ts.tv_nsec = (timeout % 1000) * 1000 * 1000;
    }

    // the mutex is locked already, set a bit indicating we're waiting
    while (d_ptr.fetchAndStoreAcquire(dummyFutexValue()) != 0) {
        if (IsTimed && pts == &ts) {
            // recalculate the timeout
            qint64 xtimeout = qint64(timeout) * 1000 * 1000;
            xtimeout -= elapsedTimer->nsecsElapsed();
            if (xtimeout <= 0) {
                // timer expired after we returned
                return false;
            }
            ts.tv_sec = xtimeout / Q_INT64_C(1000) / 1000 / 1000;
            ts.tv_nsec = xtimeout % (Q_INT64_C(1000) * 1000 * 1000);
        }
        if (IsTimed && timeout > 0)
            pts = &ts;

        // successfully set the waiting bit, now sleep
        int r = _q_futex(&d_ptr, FUTEX_WAIT, quintptr(dummyFutexValue()), pts);
        if (IsTimed && r != 0 && errno == ETIMEDOUT)
            return false;

        // we got woken up, so try to acquire the mutex
        // note we must set to dummyFutexValue because there could be other threads
        // also waiting
    }

    Q_ASSERT(d_ptr.load());
    return true;
}

void QBasicMutex::lockInternal() Q_DECL_NOTHROW
{
    Q_ASSERT(!isRecursive());
    lockInternal_helper<false>(d_ptr);
}

bool QBasicMutex::lockInternal(int timeout) Q_DECL_NOTHROW
{
    Q_ASSERT(!isRecursive());
    QElapsedTimer elapsedTimer;
    elapsedTimer.start();
    return lockInternal_helper<true>(d_ptr, timeout, &elapsedTimer);
}

void QBasicMutex::unlockInternal() Q_DECL_NOTHROW
{
    QMutexData *d = d_ptr.load();
    Q_ASSERT(d); //we must be locked
    Q_ASSERT(d != dummyLocked()); // testAndSetRelease(dummyLocked(), 0) failed
    Q_UNUSED(d);
    Q_ASSERT(!isRecursive());

    d_ptr.storeRelease(0);
    _q_futex(&d_ptr, FUTEX_WAKE, 1, 0);
}


QT_END_NAMESPACE

#endif // QT_NO_THREAD
