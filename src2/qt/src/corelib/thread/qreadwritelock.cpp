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

#include "qplatformdefs.h"
#include "qreadwritelock.h"

#ifndef QT_NO_THREAD
#include "qmutex.h"
#include "qthread.h"
#include "qwaitcondition.h"

#include "qreadwritelock_p.h"

QT_BEGIN_NAMESPACE

/*! \class QReadWriteLock
    \brief The QReadWriteLock class provides read-write locking.

    \threadsafe

    \ingroup thread

    A read-write lock is a synchronization tool for protecting
    resources that can be accessed for reading and writing. This type
    of lock is useful if you want to allow multiple threads to have
    simultaneous read-only access, but as soon as one thread wants to
    write to the resource, all other threads must be blocked until
    the writing is complete.

    In many cases, QReadWriteLock is a direct competitor to QMutex.
    QReadWriteLock is a good choice if there are many concurrent
    reads and writing occurs infrequently.

    Example:

    \snippet doc/src/snippets/code/src_corelib_thread_qreadwritelock.cpp 0

    To ensure that writers aren't blocked forever by readers, readers
    attempting to obtain a lock will not succeed if there is a blocked
    writer waiting for access, even if the lock is currently only
    accessed by other readers. Also, if the lock is accessed by a
    writer and another writer comes in, that writer will have
    priority over any readers that might also be waiting.

    Like QMutex, a QReadWriteLock can be recursively locked by the
    same thread when constructed in
    \l{QReadWriteLock::RecursionMode}. In such cases,
    unlock() must be called the same number of times lockForWrite() or
    lockForRead() was called. Note that the lock type cannot be
    changed when trying to lock recursively, i.e. it is not possible
    to lock for reading in a thread that already has locked for
    writing (and vice versa).

    \sa QReadLocker, QWriteLocker, QMutex, QSemaphore
*/

/*! 
    \enum QReadWriteLock::RecursionMode
    \since 4.4

    \value Recursive In this mode, a thread can lock the same
    QReadWriteLock multiple times and the mutex won't be unlocked
    until a corresponding number of unlock() calls have been made.

    \value NonRecursive In this mode, a thread may only lock a
    QReadWriteLock once.

    \sa QReadWriteLock()
*/

/*!
    Constructs a QReadWriteLock object in NonRecursive mode.

    \sa lockForRead(), lockForWrite()
*/
QReadWriteLock::QReadWriteLock()
    :d(new QReadWriteLockPrivate(NonRecursive))
{ }

/*!
    \since 4.4

    Constructs a QReadWriteLock object in the given \a recursionMode.

    \sa lockForRead(), lockForWrite(), RecursionMode
*/
QReadWriteLock::QReadWriteLock(RecursionMode recursionMode)
    : d(new QReadWriteLockPrivate(recursionMode))
{ }

/*!
    Destroys the QReadWriteLock object.

    \warning Destroying a read-write lock that is in use may result
    in undefined behavior.
*/
QReadWriteLock::~QReadWriteLock()
{
    delete d;
}

/*!
    Locks the lock for reading. This function will block the current
    thread if any thread (including the current) has locked for
    writing.

    \sa unlock() lockForWrite() tryLockForRead()
*/
void QReadWriteLock::lockForRead()
{
    QMutexLocker lock(&d->mutex);

    Qt::HANDLE self = 0;
    if (d->recursive) {
        self = QThread::currentThreadId();

        QHash<Qt::HANDLE, int>::iterator it = d->currentReaders.find(self);
        if (it != d->currentReaders.end()) {
            ++it.value();
            ++d->accessCount;
            Q_ASSERT_X(d->accessCount > 0, "QReadWriteLock::lockForRead()",
                       "Overflow in lock counter");
            return;
        }
    }

    while (d->accessCount < 0 || d->waitingWriters) {
        ++d->waitingReaders;
        d->readerWait.wait(&d->mutex);
        --d->waitingReaders;
    }
    if (d->recursive)
        d->currentReaders.insert(self, 1);

    ++d->accessCount;
    Q_ASSERT_X(d->accessCount > 0, "QReadWriteLock::lockForRead()", "Overflow in lock counter");
}

/*!
    Attempts to lock for reading. If the lock was obtained, this
    function returns true, otherwise it returns false instead of
    waiting for the lock to become available, i.e. it does not block.

    The lock attempt will fail if another thread has locked for
    writing.

    If the lock was obtained, the lock must be unlocked with unlock()
    before another thread can successfully lock it.

    \sa unlock() lockForRead()
*/
bool QReadWriteLock::tryLockForRead()
{
    QMutexLocker lock(&d->mutex);

    Qt::HANDLE self = 0;
    if (d->recursive) {
        self = QThread::currentThreadId();

        QHash<Qt::HANDLE, int>::iterator it = d->currentReaders.find(self);
        if (it != d->currentReaders.end()) {
            ++it.value();
            ++d->accessCount;
            Q_ASSERT_X(d->accessCount > 0, "QReadWriteLock::tryLockForRead()",
                       "Overflow in lock counter");
            return true;
        }
    }

    if (d->accessCount < 0)
        return false;
    if (d->recursive)
        d->currentReaders.insert(self, 1);

    ++d->accessCount;
    Q_ASSERT_X(d->accessCount > 0, "QReadWriteLock::tryLockForRead()", "Overflow in lock counter");

    return true;
}

/*! \overload

    Attempts to lock for reading. This function returns true if the
    lock was obtained; otherwise it returns false. If another thread
    has locked for writing, this function will wait for at most \a
    timeout milliseconds for the lock to become available.

    Note: Passing a negative number as the \a timeout is equivalent to
    calling lockForRead(), i.e. this function will wait forever until
    lock can be locked for reading when \a timeout is negative.

    If the lock was obtained, the lock must be unlocked with unlock()
    before another thread can successfully lock it.

    \sa unlock() lockForRead()
*/
bool QReadWriteLock::tryLockForRead(int timeout)
{
    QMutexLocker lock(&d->mutex);

    Qt::HANDLE self = 0;
    if (d->recursive) {
        self = QThread::currentThreadId();

        QHash<Qt::HANDLE, int>::iterator it = d->currentReaders.find(self);
        if (it != d->currentReaders.end()) {
            ++it.value();
            ++d->accessCount;
            Q_ASSERT_X(d->accessCount > 0, "QReadWriteLock::tryLockForRead()",
                       "Overflow in lock counter");
            return true;
        }
    }

    while (d->accessCount < 0 || d->waitingWriters) {
        ++d->waitingReaders;
        bool success = d->readerWait.wait(&d->mutex, timeout < 0 ? ULONG_MAX : ulong(timeout));
        --d->waitingReaders;
        if (!success)
            return false;
    }
    if (d->recursive)
        d->currentReaders.insert(self, 1);

    ++d->accessCount;
    Q_ASSERT_X(d->accessCount > 0, "QReadWriteLock::tryLockForRead()", "Overflow in lock counter");

    return true;
}

/*!
    Locks the lock for writing. This function will block the current
    thread if another thread has locked for reading or writing.

    \sa unlock() lockForRead() tryLockForWrite()
*/
void QReadWriteLock::lockForWrite()
{
    QMutexLocker lock(&d->mutex);

    Qt::HANDLE self = 0;
    if (d->recursive) {
        self = QThread::currentThreadId();

        if (d->currentWriter == self) {
            --d->accessCount;
            Q_ASSERT_X(d->accessCount < 0, "QReadWriteLock::lockForWrite()",
                       "Overflow in lock counter");
            return;
        }
    }

    while (d->accessCount != 0) {
        ++d->waitingWriters;
        d->writerWait.wait(&d->mutex);
        --d->waitingWriters;
    }
    if (d->recursive)
        d->currentWriter = self;

    --d->accessCount;
    Q_ASSERT_X(d->accessCount < 0, "QReadWriteLock::lockForWrite()", "Overflow in lock counter");
}

/*!
    Attempts to lock for writing. If the lock was obtained, this
    function returns true; otherwise, it returns false immediately.

    The lock attempt will fail if another thread has locked for
    reading or writing.

    If the lock was obtained, the lock must be unlocked with unlock()
    before another thread can successfully lock it.

    \sa unlock() lockForWrite()
*/
bool QReadWriteLock::tryLockForWrite()
{
    QMutexLocker lock(&d->mutex);

    Qt::HANDLE self = 0;
    if (d->recursive) {
        self = QThread::currentThreadId();

        if (d->currentWriter == self) {
            --d->accessCount;
            Q_ASSERT_X(d->accessCount < 0, "QReadWriteLock::lockForWrite()",
                       "Overflow in lock counter");
            return true;
        }
    }

    if (d->accessCount != 0)
        return false;
    if (d->recursive)
        d->currentWriter = self;

    --d->accessCount;
    Q_ASSERT_X(d->accessCount < 0, "QReadWriteLock::tryLockForWrite()",
               "Overflow in lock counter");

    return true;
}

/*! \overload

    Attempts to lock for writing. This function returns true if the
    lock was obtained; otherwise it returns false. If another thread
    has locked for reading or writing, this function will wait for at
    most \a timeout milliseconds for the lock to become available.

    Note: Passing a negative number as the \a timeout is equivalent to
    calling lockForWrite(), i.e. this function will wait forever until
    lock can be locked for writing when \a timeout is negative.

    If the lock was obtained, the lock must be unlocked with unlock()
    before another thread can successfully lock it.

    \sa unlock() lockForWrite()
*/
bool QReadWriteLock::tryLockForWrite(int timeout)
{
    QMutexLocker lock(&d->mutex);

    Qt::HANDLE self = 0;
    if (d->recursive) {
        self = QThread::currentThreadId();

        if (d->currentWriter == self) {
            --d->accessCount;
            Q_ASSERT_X(d->accessCount < 0, "QReadWriteLock::lockForWrite()",
                       "Overflow in lock counter");
            return true;
        }
    }

    while (d->accessCount != 0) {
        ++d->waitingWriters;
        bool success = d->writerWait.wait(&d->mutex, timeout < 0 ? ULONG_MAX : ulong(timeout));
        --d->waitingWriters;

        if (!success)
            return false;
    }
    if (d->recursive)
        d->currentWriter = self;

    --d->accessCount;
    Q_ASSERT_X(d->accessCount < 0, "QReadWriteLock::tryLockForWrite()",
               "Overflow in lock counter");

    return true;
}

/*!
    Unlocks the lock.

    Attempting to unlock a lock that is not locked is an error, and will result
    in program termination.

    \sa lockForRead() lockForWrite() tryLockForRead() tryLockForWrite()
*/
void QReadWriteLock::unlock()
{
    QMutexLocker lock(&d->mutex);

    Q_ASSERT_X(d->accessCount != 0, "QReadWriteLock::unlock()", "Cannot unlock an unlocked lock");

    bool unlocked = false;
    if (d->accessCount > 0) {
        // releasing a read lock
        if (d->recursive) {
            Qt::HANDLE self = QThread::currentThreadId();
            QHash<Qt::HANDLE, int>::iterator it = d->currentReaders.find(self);
            if (it != d->currentReaders.end()) {
                if (--it.value() <= 0)
                    d->currentReaders.erase(it);
            }
        }

        unlocked = --d->accessCount == 0;
    } else if (d->accessCount < 0 && ++d->accessCount == 0) {
        // released a write lock
        unlocked = true;
        d->currentWriter = 0;
    }

    if (unlocked) {
        if (d->waitingWriters) {
            d->writerWait.wakeOne();
        } else if (d->waitingReaders) {
            d->readerWait.wakeAll();
        }
    }
}

/*!
    \class QReadLocker
    \brief The QReadLocker class is a convenience class that
    simplifies locking and unlocking read-write locks for read access.

    \threadsafe

    \ingroup thread

    The purpose of QReadLocker (and QWriteLocker) is to simplify
    QReadWriteLock locking and unlocking. Locking and unlocking
    statements or in exception handling code is error-prone and
    difficult to debug. QReadLocker can be used in such situations
    to ensure that the state of the lock is always well-defined.

    Here's an example that uses QReadLocker to lock and unlock a
    read-write lock for reading:

    \snippet doc/src/snippets/code/src_corelib_thread_qreadwritelock.cpp 1

    It is equivalent to the following code:

    \snippet doc/src/snippets/code/src_corelib_thread_qreadwritelock.cpp 2

    The QMutexLocker documentation shows examples where the use of a
    locker object greatly simplifies programming.

    \sa QWriteLocker, QReadWriteLock
*/

/*!
    \fn QReadLocker::QReadLocker(QReadWriteLock *lock)

    Constructs a QReadLocker and locks \a lock for reading. The lock
    will be unlocked when the QReadLocker is destroyed. If \c lock is
    zero, QReadLocker does nothing.

    \sa QReadWriteLock::lockForRead()
*/

/*!
    \fn QReadLocker::~QReadLocker()

    Destroys the QReadLocker and unlocks the lock that was passed to
    the constructor.

    \sa QReadWriteLock::unlock()
*/

/*!
    \fn void QReadLocker::unlock()

    Unlocks the lock associated with this locker.

    \sa QReadWriteLock::unlock()
*/

/*!
    \fn void QReadLocker::relock()

    Relocks an unlocked lock.

    \sa unlock()
*/

/*!
    \fn QReadWriteLock *QReadLocker::readWriteLock() const

    Returns a pointer to the read-write lock that was passed
    to the constructor.
*/

/*!
    \class QWriteLocker
    \brief The QWriteLocker class is a convenience class that
    simplifies locking and unlocking read-write locks for write access.

    \threadsafe

    \ingroup thread

    The purpose of QWriteLocker (and QReadLocker is to simplify
    QReadWriteLock locking and unlocking. Locking and unlocking
    statements or in exception handling code is error-prone and
    difficult to debug. QWriteLocker can be used in such situations
    to ensure that the state of the lock is always well-defined.

    Here's an example that uses QWriteLocker to lock and unlock a
    read-write lock for writing:

    \snippet doc/src/snippets/code/src_corelib_thread_qreadwritelock.cpp 3

    It is equivalent to the following code:

    \snippet doc/src/snippets/code/src_corelib_thread_qreadwritelock.cpp 4

    The QMutexLocker documentation shows examples where the use of a
    locker object greatly simplifies programming.

    \sa QReadLocker, QReadWriteLock
*/

/*!
    \fn QWriteLocker::QWriteLocker(QReadWriteLock *lock)

    Constructs a QWriteLocker and locks \a lock for writing. The lock
    will be unlocked when the QWriteLocker is destroyed. If \c lock is
    zero, QWriteLocker does nothing.

    \sa QReadWriteLock::lockForWrite()
*/

/*!
    \fn QWriteLocker::~QWriteLocker()

    Destroys the QWriteLocker and unlocks the lock that was passed to
    the constructor.

    \sa QReadWriteLock::unlock()
*/

/*!
    \fn void QWriteLocker::unlock()

    Unlocks the lock associated with this locker.

    \sa QReadWriteLock::unlock()
*/

/*!
    \fn void QWriteLocker::relock()

    Relocks an unlocked lock.

    \sa unlock()
*/

/*!
    \fn QReadWriteLock *QWriteLocker::readWriteLock() const

    Returns a pointer to the read-write lock that was passed
    to the constructor.
*/

QT_END_NAMESPACE

#endif // QT_NO_THREAD
