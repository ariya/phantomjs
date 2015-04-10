/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2012 Intel Corporation
** Copyright (C) 2012 Olivier Goffart <ogoffart@woboq.com>
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
#include <qdebug.h>

#ifndef QT_NO_THREAD
#include "qatomic.h"
#include "qelapsedtimer.h"
#include "qthread.h"
#include "qmutex_p.h"
#include "qtypetraits.h"

#ifndef QT_LINUX_FUTEX
#include "private/qfreelist_p.h"
#endif

QT_BEGIN_NAMESPACE

static inline bool isRecursive(QMutexData *d)
{
    quintptr u = quintptr(d);
    if (Q_LIKELY(u <= 0x3))
        return false;
#ifdef QT_LINUX_FUTEX
    Q_ASSERT(d->recursive);
    return true;
#else
    return d->recursive;
#endif
}

class QRecursiveMutexPrivate : public QMutexData
{
public:
    QRecursiveMutexPrivate()
        : QMutexData(QMutex::Recursive), owner(0), count(0) {}

    // written to by the thread that first owns 'mutex';
    // read during attempts to acquire ownership of 'mutex' from any other thread:
    QAtomicPointer<QtPrivate::remove_pointer<Qt::HANDLE>::type> owner;

    // only ever accessed from the thread that owns 'mutex':
    uint count;

    QMutex mutex;

    bool lock(int timeout) QT_MUTEX_LOCK_NOEXCEPT;
    void unlock() Q_DECL_NOTHROW;
};

/*
    \class QBasicMutex
    \inmodule QtCore
    \brief QMutex POD
    \internal

    \ingroup thread

    - Can be used as global static object.
    - Always non-recursive
    - Do not use tryLock with timeout > 0, else you can have a leak (see the ~QMutex destructor)
*/

/*!
    \class QMutex
    \inmodule QtCore
    \brief The QMutex class provides access serialization between threads.

    \threadsafe

    \ingroup thread

    The purpose of a QMutex is to protect an object, data structure or
    section of code so that only one thread can access it at a time
    (this is similar to the Java \c synchronized keyword). It is
    usually best to use a mutex with a QMutexLocker since this makes
    it easy to ensure that locking and unlocking are performed
    consistently.

    For example, say there is a method that prints a message to the
    user on two lines:

    \snippet code/src_corelib_thread_qmutex.cpp 0

    If these two methods are called in succession, the following happens:

    \snippet code/src_corelib_thread_qmutex.cpp 1

    If these two methods are called simultaneously from two threads then the
    following sequence could result:

    \snippet code/src_corelib_thread_qmutex.cpp 2

    If we add a mutex, we should get the result we want:

    \snippet code/src_corelib_thread_qmutex.cpp 3

    Then only one thread can modify \c number at any given time and
    the result is correct. This is a trivial example, of course, but
    applies to any other case where things need to happen in a
    particular sequence.

    When you call lock() in a thread, other threads that try to call
    lock() in the same place will block until the thread that got the
    lock calls unlock(). A non-blocking alternative to lock() is
    tryLock().

    \sa QMutexLocker, QReadWriteLock, QSemaphore, QWaitCondition
*/

/*!
    \enum QMutex::RecursionMode

    \value Recursive  In this mode, a thread can lock the same mutex
                      multiple times and the mutex won't be unlocked
                      until a corresponding number of unlock() calls
                      have been made.

    \value NonRecursive  In this mode, a thread may only lock a mutex
                         once.

    \sa QMutex()
*/

/*!
    Constructs a new mutex. The mutex is created in an unlocked state.

    If \a mode is QMutex::Recursive, a thread can lock the same mutex
    multiple times and the mutex won't be unlocked until a
    corresponding number of unlock() calls have been made. Otherwise
    a thread may only lock a mutex once. The default is
    QMutex::NonRecursive.

    \sa lock(), unlock()
*/
QMutex::QMutex(RecursionMode mode)
{
    d_ptr.store(mode == Recursive ? new QRecursiveMutexPrivate : 0);
}

/*!
    Destroys the mutex.

    \warning Destroying a locked mutex may result in undefined behavior.
*/
QMutex::~QMutex()
{
    QMutexData *d = d_ptr.load();
    if (isRecursive()) {
        delete static_cast<QRecursiveMutexPrivate *>(d);
    } else if (d) {
#ifndef QT_LINUX_FUTEX
        if (d != dummyLocked() && static_cast<QMutexPrivate *>(d)->possiblyUnlocked.load()
            && tryLock()) {
            unlock();
            return;
        }
#endif
        qWarning("QMutex: destroying locked mutex");
    }
}

/*! \fn void QMutex::lock()
    Locks the mutex. If another thread has locked the mutex then this
    call will block until that thread has unlocked it.

    Calling this function multiple times on the same mutex from the
    same thread is allowed if this mutex is a
    \l{QMutex::Recursive}{recursive mutex}. If this mutex is a
    \l{QMutex::NonRecursive}{non-recursive mutex}, this function will
    \e dead-lock when the mutex is locked recursively.

    \sa unlock()
*/
void QMutex::lock() QT_MUTEX_LOCK_NOEXCEPT
{
    QMutexData *current;
    if (fastTryLock(current))
        return;
    if (QT_PREPEND_NAMESPACE(isRecursive)(current))
        static_cast<QRecursiveMutexPrivate *>(current)->lock(-1);
    else
        lockInternal();
}

/*! \fn bool QMutex::tryLock(int timeout)

    Attempts to lock the mutex. This function returns \c true if the lock
    was obtained; otherwise it returns \c false. If another thread has
    locked the mutex, this function will wait for at most \a timeout
    milliseconds for the mutex to become available.

    Note: Passing a negative number as the \a timeout is equivalent to
    calling lock(), i.e. this function will wait forever until mutex
    can be locked if \a timeout is negative.

    If the lock was obtained, the mutex must be unlocked with unlock()
    before another thread can successfully lock it.

    Calling this function multiple times on the same mutex from the
    same thread is allowed if this mutex is a
    \l{QMutex::Recursive}{recursive mutex}. If this mutex is a
    \l{QMutex::NonRecursive}{non-recursive mutex}, this function will
    \e always return false when attempting to lock the mutex
    recursively.

    \sa lock(), unlock()
*/
bool QMutex::tryLock(int timeout) QT_MUTEX_LOCK_NOEXCEPT
{
    QMutexData *current;
    if (fastTryLock(current))
        return true;
    if (QT_PREPEND_NAMESPACE(isRecursive)(current))
        return static_cast<QRecursiveMutexPrivate *>(current)->lock(timeout);
    else
        return lockInternal(timeout);
}

/*! \fn void QMutex::unlock()
    Unlocks the mutex. Attempting to unlock a mutex in a different
    thread to the one that locked it results in an error. Unlocking a
    mutex that is not locked results in undefined behavior.

    \sa lock()
*/
void QMutex::unlock() Q_DECL_NOTHROW
{
    QMutexData *current;
    if (fastTryUnlock(current))
        return;
    if (QT_PREPEND_NAMESPACE(isRecursive)(current))
        static_cast<QRecursiveMutexPrivate *>(current)->unlock();
    else
        unlockInternal();
}

/*!
    \fn void QMutex::isRecursive()
    \since 5.0

    Returns \c true if the mutex is recursive

*/
bool QBasicMutex::isRecursive()
{
    return QT_PREPEND_NAMESPACE(isRecursive)(d_ptr.loadAcquire());
}


/*!
    \class QMutexLocker
    \inmodule QtCore
    \brief The QMutexLocker class is a convenience class that simplifies
    locking and unlocking mutexes.

    \threadsafe

    \ingroup thread

    Locking and unlocking a QMutex in complex functions and
    statements or in exception handling code is error-prone and
    difficult to debug. QMutexLocker can be used in such situations
    to ensure that the state of the mutex is always well-defined.

    QMutexLocker should be created within a function where a
    QMutex needs to be locked. The mutex is locked when QMutexLocker
    is created. You can unlock and relock the mutex with \c unlock()
    and \c relock(). If locked, the mutex will be unlocked when the
    QMutexLocker is destroyed.

    For example, this complex function locks a QMutex upon entering
    the function and unlocks the mutex at all the exit points:

    \snippet code/src_corelib_thread_qmutex.cpp 4

    This example function will get more complicated as it is
    developed, which increases the likelihood that errors will occur.

    Using QMutexLocker greatly simplifies the code, and makes it more
    readable:

    \snippet code/src_corelib_thread_qmutex.cpp 5

    Now, the mutex will always be unlocked when the QMutexLocker
    object is destroyed (when the function returns since \c locker is
    an auto variable).

    The same principle applies to code that throws and catches
    exceptions. An exception that is not caught in the function that
    has locked the mutex has no way of unlocking the mutex before the
    exception is passed up the stack to the calling function.

    QMutexLocker also provides a \c mutex() member function that returns
    the mutex on which the QMutexLocker is operating. This is useful
    for code that needs access to the mutex, such as
    QWaitCondition::wait(). For example:

    \snippet code/src_corelib_thread_qmutex.cpp 6

    \sa QReadLocker, QWriteLocker, QMutex
*/

/*!
    \fn QMutexLocker::QMutexLocker(QMutex *mutex)

    Constructs a QMutexLocker and locks \a mutex. The mutex will be
    unlocked when the QMutexLocker is destroyed. If \a mutex is zero,
    QMutexLocker does nothing.

    \sa QMutex::lock()
*/

/*!
    \fn QMutexLocker::~QMutexLocker()

    Destroys the QMutexLocker and unlocks the mutex that was locked
    in the constructor.

    \sa QMutex::unlock()
*/

/*!
    \fn void QMutexLocker::unlock()

    Unlocks this mutex locker. You can use \c relock() to lock
    it again. It does not need to be locked when destroyed.

    \sa relock()
*/

/*!
    \fn void QMutexLocker::relock()

    Relocks an unlocked mutex locker.

    \sa unlock()
*/

/*!
    \fn QMutex *QMutexLocker::mutex()

    Returns the mutex on which the QMutexLocker is operating.

*/

#ifndef QT_LINUX_FUTEX //linux implementation is in qmutex_linux.cpp

/*
  For a rough introduction on how this works, refer to
  http://woboq.com/blog/internals-of-qmutex-in-qt5.html
  which explains a slightly simplified version of it.
  The differences are that here we try to work with timeout (requires the
  possiblyUnlocked flag) and that we only wake one thread when unlocking
  (requires maintaining the waiters count)
  We also support recursive mutexes which always have a valid d_ptr.

  The waiters flag represents the number of threads that are waiting or about
  to wait on the mutex. There are two tricks to keep in mind:
  We don't want to increment waiters after we checked no threads are waiting
  (waiters == 0). That's why we atomically set the BigNumber flag on waiters when
  we check waiters. Similarly, if waiters is decremented right after we checked,
  the mutex would be unlocked (d->wakeUp() has (or will) be called), but there is
  no thread waiting. This is only happening if there was a timeout in tryLock at the
  same time as the mutex is unlocked. So when there was a timeout, we set the
  possiblyUnlocked flag.
*/

/*!
    \internal helper for lock()
 */
void QBasicMutex::lockInternal() QT_MUTEX_LOCK_NOEXCEPT
{
    lockInternal(-1);
}

/*!
    \internal helper for lock(int)
 */
bool QBasicMutex::lockInternal(int timeout) QT_MUTEX_LOCK_NOEXCEPT
{
    Q_ASSERT(!isRecursive());

    while (!fastTryLock()) {
        QMutexData *copy = d_ptr.loadAcquire();
        if (!copy) // if d is 0, the mutex is unlocked
            continue;

        if (copy == dummyLocked()) {
            if (timeout == 0)
                return false;
            // The mutex is locked but does not have a QMutexPrivate yet.
            // we need to allocate a QMutexPrivate
            QMutexPrivate *newD = QMutexPrivate::allocate();
            if (!d_ptr.testAndSetOrdered(dummyLocked(), newD)) {
                //Either the mutex is already unlocked, or another thread already set it.
                newD->deref();
                continue;
            }
            copy = newD;
            //the d->refCount is already 1 the deref will occurs when we unlock
        }

        QMutexPrivate *d = static_cast<QMutexPrivate *>(copy);
        if (timeout == 0 && !d->possiblyUnlocked.load())
            return false;

        // At this point we have a pointer to a QMutexPrivate. But the other thread
        // may unlock the mutex at any moment and release the QMutexPrivate to the pool.
        // We will try to reference it to avoid unlock to release it to the pool to make
        // sure it won't be released. But if the refcount is already 0 it has been released.
        if (!d->ref())
            continue; //that QMutexData was already released

        // We now hold a reference to the QMutexPrivate. It won't be released and re-used.
        // But it is still possible that it was already re-used by another QMutex right before
        // we did the ref(). So check if we still hold a pointer to the right mutex.
        if (d != d_ptr.loadAcquire()) {
            //Either the mutex is already unlocked, or relocked with another mutex
            d->deref();
            continue;
        }

        // In this part, we will try to increment the waiters count.
        // We just need to take care of the case in which the old_waiters
        // is set to the BigNumber magic value set in unlockInternal()
        int old_waiters;
        do {
            old_waiters = d->waiters.load();
            if (old_waiters == -QMutexPrivate::BigNumber) {
                // we are unlocking, and the thread that unlocks is about to change d to 0
                // we try to acquire the mutex by changing to dummyLocked()
                if (d_ptr.testAndSetAcquire(d, dummyLocked())) {
                    // Mutex acquired
                    d->deref();
                    return true;
                } else {
                    Q_ASSERT(d != d_ptr.load()); //else testAndSetAcquire should have succeeded
                    // Mutex is likely to bo 0, we should continue the outer-loop,
                    //  set old_waiters to the magic value of BigNumber
                    old_waiters = QMutexPrivate::BigNumber;
                    break;
                }
            }
        } while (!d->waiters.testAndSetRelaxed(old_waiters, old_waiters + 1));

        if (d != d_ptr.loadAcquire()) {
            // The mutex was unlocked before we incremented waiters.
            if (old_waiters != QMutexPrivate::BigNumber) {
                //we did not break the previous loop
                Q_ASSERT(d->waiters.load() >= 1);
                d->waiters.deref();
            }
            d->deref();
            continue;
        }

        if (d->wait(timeout)) {
            // reset the possiblyUnlocked flag if needed (and deref its corresponding reference)
            if (d->possiblyUnlocked.load() && d->possiblyUnlocked.testAndSetRelaxed(true, false))
                d->deref();
            d->derefWaiters(1);
            //we got the lock. (do not deref)
            Q_ASSERT(d == d_ptr.load());
            return true;
        } else {
            Q_ASSERT(timeout >= 0);
            //timeout
            d->derefWaiters(1);
            //There may be a race in which the mutex is unlocked right after we timed out,
            // and before we deref the waiters, so maybe the mutex is actually unlocked.
            // Set the possiblyUnlocked flag to indicate this possibility.
            if (!d->possiblyUnlocked.testAndSetRelaxed(false, true)) {
                // We keep a reference when possiblyUnlocked is true.
                // but if possiblyUnlocked was already true, we don't need to keep the reference.
                d->deref();
            }
            return false;
        }
    }
    Q_ASSERT(d_ptr.load() != 0);
    return true;
}

/*!
    \internal
*/
void QBasicMutex::unlockInternal() Q_DECL_NOTHROW
{
    QMutexData *copy = d_ptr.loadAcquire();
    Q_ASSERT(copy); //we must be locked
    Q_ASSERT(copy != dummyLocked()); // testAndSetRelease(dummyLocked(), 0) failed
    Q_ASSERT(!isRecursive());

    QMutexPrivate *d = reinterpret_cast<QMutexPrivate *>(copy);

    // If no one is waiting for the lock anymore, we should reset d to 0x0.
    // Using fetchAndAdd, we atomically check that waiters was equal to 0, and add a flag
    // to the waiters variable (BigNumber). That way, we avoid the race in which waiters is
    // incremented right after we checked, because we won't increment waiters if is
    // equal to -BigNumber
    if (d->waiters.fetchAndAddRelease(-QMutexPrivate::BigNumber) == 0) {
        //there is no one waiting on this mutex anymore, set the mutex as unlocked (d = 0)
        if (d_ptr.testAndSetRelease(d, 0)) {
            // reset the possiblyUnlocked flag if needed (and deref its corresponding reference)
            if (d->possiblyUnlocked.load() && d->possiblyUnlocked.testAndSetRelaxed(true, false))
                d->deref();
        }
        d->derefWaiters(0);
    } else {
        d->derefWaiters(0);
        //there are thread waiting, transfer the lock.
        d->wakeUp();
    }
    d->deref();
}

//The freelist management
namespace {
struct FreeListConstants : QFreeListDefaultConstants {
    enum { BlockCount = 4, MaxIndex=0xffff };
    static const int Sizes[BlockCount];
};
const int FreeListConstants::Sizes[FreeListConstants::BlockCount] = {
    16,
    128,
    1024,
    FreeListConstants::MaxIndex - (16-128-1024)
};

typedef QFreeList<QMutexPrivate, FreeListConstants> FreeList;
Q_GLOBAL_STATIC(FreeList, freelist);
}

QMutexPrivate *QMutexPrivate::allocate()
{
    int i = freelist()->next();
    QMutexPrivate *d = &(*freelist())[i];
    d->id = i;
    Q_ASSERT(d->refCount.load() == 0);
    Q_ASSERT(!d->recursive);
    Q_ASSERT(!d->possiblyUnlocked.load());
    Q_ASSERT(d->waiters.load() == 0);
    d->refCount.store(1);
    return d;
}

void QMutexPrivate::release()
{
    Q_ASSERT(!recursive);
    Q_ASSERT(refCount.load() == 0);
    Q_ASSERT(!possiblyUnlocked.load());
    Q_ASSERT(waiters.load() == 0);
    freelist()->release(id);
}

// atomically subtract "value" to the waiters, and remove the QMutexPrivate::BigNumber flag
void QMutexPrivate::derefWaiters(int value) Q_DECL_NOTHROW
{
    int old_waiters;
    int new_waiters;
    do {
        old_waiters = waiters.load();
        new_waiters = old_waiters;
        if (new_waiters < 0) {
            new_waiters += QMutexPrivate::BigNumber;
        }
        new_waiters -= value;
    } while (!waiters.testAndSetRelaxed(old_waiters, new_waiters));
}
#endif

/*!
   \internal
 */
inline bool QRecursiveMutexPrivate::lock(int timeout) QT_MUTEX_LOCK_NOEXCEPT
{
    Qt::HANDLE self = QThread::currentThreadId();
    if (owner.load() == self) {
        ++count;
        Q_ASSERT_X(count != 0, "QMutex::lock", "Overflow in recursion counter");
        return true;
    }
    bool success = true;
    if (timeout == -1) {
        mutex.QBasicMutex::lock();
    } else {
        success = mutex.tryLock(timeout);
    }

    if (success)
        owner.store(self);
    return success;
}

/*!
   \internal
 */
inline void QRecursiveMutexPrivate::unlock() Q_DECL_NOTHROW
{
    if (count > 0) {
        count--;
    } else {
        owner.store(0);
        mutex.QBasicMutex::unlock();
    }
}

QT_END_NAMESPACE

#ifdef QT_LINUX_FUTEX
#  include "qmutex_linux.cpp"
#elif defined(Q_OS_MAC)
#  include "qmutex_mac.cpp"
#elif defined(Q_OS_WIN)
#  include "qmutex_win.cpp"
#else
#  include "qmutex_unix.cpp"
#endif

#endif // QT_NO_THREAD
