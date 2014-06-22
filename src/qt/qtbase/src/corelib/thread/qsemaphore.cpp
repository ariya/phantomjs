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

#include "qsemaphore.h"

#ifndef QT_NO_THREAD
#include "qmutex.h"
#include "qwaitcondition.h"
#include "qelapsedtimer.h"
#include "qdatetime.h"

QT_BEGIN_NAMESPACE

/*!
    \class QSemaphore
    \inmodule QtCore
    \brief The QSemaphore class provides a general counting semaphore.

    \threadsafe

    \ingroup thread

    A semaphore is a generalization of a mutex. While a mutex can
    only be locked once, it's possible to acquire a semaphore
    multiple times. Semaphores are typically used to protect a
    certain number of identical resources.

    Semaphores support two fundamental operations, acquire() and
    release():

    \list
    \li acquire(\e{n}) tries to acquire \e n resources. If there aren't
       that many resources available, the call will block until this
       is the case.
    \li release(\e{n}) releases \e n resources.
    \endlist

    There's also a tryAcquire() function that returns immediately if
    it cannot acquire the resources, and an available() function that
    returns the number of available resources at any time.

    Example:

    \snippet code/src_corelib_thread_qsemaphore.cpp 0

    A typical application of semaphores is for controlling access to
    a circular buffer shared by a producer thread and a consumer
    thread. The \l{Semaphores Example} shows how
    to use QSemaphore to solve that problem.

    A non-computing example of a semaphore would be dining at a
    restaurant. A semaphore is initialized with the number of chairs
    in the restaurant. As people arrive, they want a seat. As seats
    are filled, available() is decremented. As people leave, the
    available() is incremented, allowing more people to enter. If a
    party of 10 people want to be seated, but there are only 9 seats,
    those 10 people will wait, but a party of 4 people would be
    seated (taking the available seats to 5, making the party of 10
    people wait longer).

    \sa QMutex, QWaitCondition, QThread, {Semaphores Example}
*/

class QSemaphorePrivate {
public:
    inline QSemaphorePrivate(int n) : avail(n) { }

    QMutex mutex;
    QWaitCondition cond;

    int avail;
};

/*!
    Creates a new semaphore and initializes the number of resources
    it guards to \a n (by default, 0).

    \sa release(), available()
*/
QSemaphore::QSemaphore(int n)
{
    Q_ASSERT_X(n >= 0, "QSemaphore", "parameter 'n' must be non-negative");
    d = new QSemaphorePrivate(n);
}

/*!
    Destroys the semaphore.

    \warning Destroying a semaphore that is in use may result in
    undefined behavior.
*/
QSemaphore::~QSemaphore()
{ delete d; }

/*!
    Tries to acquire \c n resources guarded by the semaphore. If \a n
    > available(), this call will block until enough resources are
    available.

    \sa release(), available(), tryAcquire()
*/
void QSemaphore::acquire(int n)
{
    Q_ASSERT_X(n >= 0, "QSemaphore::acquire", "parameter 'n' must be non-negative");
    QMutexLocker locker(&d->mutex);
    while (n > d->avail)
        d->cond.wait(locker.mutex());
    d->avail -= n;
}

/*!
    Releases \a n resources guarded by the semaphore.

    This function can be used to "create" resources as well. For
    example:

    \snippet code/src_corelib_thread_qsemaphore.cpp 1

    \sa acquire(), available()
*/
void QSemaphore::release(int n)
{
    Q_ASSERT_X(n >= 0, "QSemaphore::release", "parameter 'n' must be non-negative");
    QMutexLocker locker(&d->mutex);
    d->avail += n;
    d->cond.wakeAll();
}

/*!
    Returns the number of resources currently available to the
    semaphore. This number can never be negative.

    \sa acquire(), release()
*/
int QSemaphore::available() const
{
    QMutexLocker locker(&d->mutex);
    return d->avail;
}

/*!
    Tries to acquire \c n resources guarded by the semaphore and
    returns \c true on success. If available() < \a n, this call
    immediately returns \c false without acquiring any resources.

    Example:

    \snippet code/src_corelib_thread_qsemaphore.cpp 2

    \sa acquire()
*/
bool QSemaphore::tryAcquire(int n)
{
    Q_ASSERT_X(n >= 0, "QSemaphore::tryAcquire", "parameter 'n' must be non-negative");
    QMutexLocker locker(&d->mutex);
    if (n > d->avail)
        return false;
    d->avail -= n;
    return true;
}

/*!
    Tries to acquire \c n resources guarded by the semaphore and
    returns \c true on success. If available() < \a n, this call will
    wait for at most \a timeout milliseconds for resources to become
    available.

    Note: Passing a negative number as the \a timeout is equivalent to
    calling acquire(), i.e. this function will wait forever for
    resources to become available if \a timeout is negative.

    Example:

    \snippet code/src_corelib_thread_qsemaphore.cpp 3

    \sa acquire()
*/
bool QSemaphore::tryAcquire(int n, int timeout)
{
    Q_ASSERT_X(n >= 0, "QSemaphore::tryAcquire", "parameter 'n' must be non-negative");
    QMutexLocker locker(&d->mutex);
    if (timeout < 0) {
        while (n > d->avail)
            d->cond.wait(locker.mutex());
    } else {
        QElapsedTimer timer;
        timer.start();
        while (n > d->avail) {
            const qint64 elapsed = timer.elapsed();
            if (timeout - elapsed <= 0
                || !d->cond.wait(locker.mutex(), timeout - elapsed))
                return false;
        }
    }
    d->avail -= n;
    return true;


}

QT_END_NAMESPACE

#endif // QT_NO_THREAD
