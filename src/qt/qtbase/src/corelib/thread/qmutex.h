/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QMUTEX_H
#define QMUTEX_H

#include <QtCore/qglobal.h>
#include <QtCore/qatomic.h>
#include <new>

QT_BEGIN_NAMESPACE


#if !defined(QT_NO_THREAD) && !defined(Q_QDOC)

#ifdef Q_OS_LINUX
# define QT_MUTEX_LOCK_NOEXCEPT Q_DECL_NOTHROW
#else
# define QT_MUTEX_LOCK_NOEXCEPT
#endif

class QMutexData;

class Q_CORE_EXPORT QBasicMutex
{
public:
    inline void lock() QT_MUTEX_LOCK_NOEXCEPT {
        if (!fastTryLock())
            lockInternal();
    }

    inline void unlock() Q_DECL_NOTHROW {
        Q_ASSERT(d_ptr.load()); //mutex must be locked
        if (!fastTryUnlock())
            unlockInternal();
    }

    bool tryLock() Q_DECL_NOTHROW {
        return fastTryLock();
    }

    bool isRecursive(); //### Qt6: mark const

private:
    inline bool fastTryLock() Q_DECL_NOTHROW {
        return d_ptr.testAndSetAcquire(0, dummyLocked());
    }
    inline bool fastTryUnlock() Q_DECL_NOTHROW {
        return d_ptr.testAndSetRelease(dummyLocked(), 0);
    }
    inline bool fastTryLock(QMutexData *&current) Q_DECL_NOTHROW {
        return d_ptr.testAndSetAcquire(0, dummyLocked(), current);
    }
    inline bool fastTryUnlock(QMutexData *&current) Q_DECL_NOTHROW {
        return d_ptr.testAndSetRelease(dummyLocked(), 0, current);
    }

    void lockInternal() QT_MUTEX_LOCK_NOEXCEPT;
    bool lockInternal(int timeout) QT_MUTEX_LOCK_NOEXCEPT;
    void unlockInternal() Q_DECL_NOTHROW;

    QBasicAtomicPointer<QMutexData> d_ptr;
    static inline QMutexData *dummyLocked() {
        return reinterpret_cast<QMutexData *>(quintptr(1));
    }

    friend class QMutex;
    friend class QMutexData;
};

class Q_CORE_EXPORT QMutex : public QBasicMutex {
public:
    enum RecursionMode { NonRecursive, Recursive };
    explicit QMutex(RecursionMode mode = NonRecursive);
    ~QMutex();

    void lock() QT_MUTEX_LOCK_NOEXCEPT;
    bool tryLock(int timeout = 0) QT_MUTEX_LOCK_NOEXCEPT;
    void unlock() Q_DECL_NOTHROW;

    using QBasicMutex::isRecursive;

private:
    Q_DISABLE_COPY(QMutex)
    friend class QMutexLocker;
};

class Q_CORE_EXPORT QMutexLocker
{
public:
    inline explicit QMutexLocker(QBasicMutex *m) QT_MUTEX_LOCK_NOEXCEPT
    {
        Q_ASSERT_X((reinterpret_cast<quintptr>(m) & quintptr(1u)) == quintptr(0),
                   "QMutexLocker", "QMutex pointer is misaligned");
        val = quintptr(m);
        if (Q_LIKELY(m)) {
            // call QMutex::lock() instead of QBasicMutex::lock()
            static_cast<QMutex *>(m)->lock();
            val |= 1;
        }
    }
    inline ~QMutexLocker() { unlock(); }

    inline void unlock() Q_DECL_NOTHROW
    {
        if ((val & quintptr(1u)) == quintptr(1u)) {
            val &= ~quintptr(1u);
            mutex()->unlock();
        }
    }

    inline void relock() QT_MUTEX_LOCK_NOEXCEPT
    {
        if (val) {
            if ((val & quintptr(1u)) == quintptr(0u)) {
                mutex()->lock();
                val |= quintptr(1u);
            }
        }
    }

#if defined(Q_CC_MSVC)
#pragma warning( push )
#pragma warning( disable : 4312 ) // ignoring the warning from /Wp64
#endif

    inline QMutex *mutex() const
    {
        return reinterpret_cast<QMutex *>(val & ~quintptr(1u));
    }

#if defined(Q_CC_MSVC)
#pragma warning( pop )
#endif

private:
    Q_DISABLE_COPY(QMutexLocker)

    quintptr val;
};

#else // QT_NO_THREAD or Q_QDOC

class Q_CORE_EXPORT QMutex
{
public:
    enum RecursionMode { NonRecursive, Recursive };

    inline explicit QMutex(RecursionMode mode = NonRecursive) { Q_UNUSED(mode); }

    inline void lock() {}
    inline bool tryLock(int timeout = 0) { Q_UNUSED(timeout); return true; }
    inline void unlock() {}
    inline bool isRecursive() { return true; }

private:
    Q_DISABLE_COPY(QMutex)
};

class Q_CORE_EXPORT QMutexLocker
{
public:
    inline explicit QMutexLocker(QMutex *) {}
    inline ~QMutexLocker() {}

    inline void unlock() {}
    void relock() {}
    inline QMutex *mutex() const { return 0; }

private:
    Q_DISABLE_COPY(QMutexLocker)
};

typedef QMutex QBasicMutex;

#endif // QT_NO_THREAD or Q_QDOC

QT_END_NAMESPACE

#endif // QMUTEX_H
