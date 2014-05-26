/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QMUTEX_H
#define QMUTEX_H

#include <QtCore/qglobal.h>
#include <QtCore/qatomic.h>
#include <new>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#ifndef QT_NO_THREAD

class QAtomicInt;
class QMutexData;

class Q_CORE_EXPORT QMutex
{
    friend class QWaitCondition;
    friend class QWaitConditionPrivate;

public:
    enum RecursionMode { NonRecursive, Recursive };

    explicit QMutex(RecursionMode mode = NonRecursive);
    ~QMutex();

    void lock();     //### Qt5: make inline;
    inline void lockInline();
    bool tryLock();  //### Qt5: make inline;
    bool tryLock(int timeout);
    inline bool tryLockInline();
    void unlock();     //### Qt5: make inline;
    inline void unlockInline();

#if defined(QT3_SUPPORT)
    inline QT3_SUPPORT bool locked()
    {
        if (!tryLock())
            return true;
        unlock();
        return false;
    }
    inline QT3_SUPPORT_CONSTRUCTOR QMutex(bool recursive)
    {
        new (this) QMutex(recursive ? Recursive : NonRecursive);
    }
#endif

private:
    void lockInternal();
    void unlockInternal();
    Q_DISABLE_COPY(QMutex)

    QMutexData *d;
};

class Q_CORE_EXPORT QMutexLocker
{
public:
    inline explicit QMutexLocker(QMutex *m)
    {
        Q_ASSERT_X((reinterpret_cast<quintptr>(m) & quintptr(1u)) == quintptr(0),
                   "QMutexLocker", "QMutex pointer is misaligned");
        if (m) {
            m->lockInline();
            val = reinterpret_cast<quintptr>(m) | quintptr(1u);
        } else {
            val = 0;
        }
    }
    inline ~QMutexLocker() { unlock(); }

    inline void unlock()
    {
        if ((val & quintptr(1u)) == quintptr(1u)) {
            val &= ~quintptr(1u);
            mutex()->unlockInline();
        }
    }

    inline void relock()
    {
        if (val) {
            if ((val & quintptr(1u)) == quintptr(0u)) {
                mutex()->lockInline();
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

class QMutexData
{
    public:
        QAtomicInt contenders;
        const uint recursive : 1;
        uint reserved : 31;
    protected:
        QMutexData(QMutex::RecursionMode mode);
        ~QMutexData();
};

#ifdef QT_NO_DEBUG
inline void QMutex::unlockInline()
{
    if (d->recursive) {
        unlock();
    } else if (!d->contenders.testAndSetRelease(1, 0)) {
        unlockInternal();
    }
}

inline bool QMutex::tryLockInline()
{
    if (d->recursive) {
        return tryLock();
    } else {
        return d->contenders.testAndSetAcquire(0, 1);
    }
}

inline void QMutex::lockInline()
{
    if (d->recursive) {
        lock();
    } else if(!tryLockInline()) {
        lockInternal();
    }
}
#else // QT_NO_DEBUG
//in debug we do not use inline calls in order to allow debugging tools
// to hook the mutex locking functions.
inline void QMutex::unlockInline() { unlock(); }
inline bool QMutex::tryLockInline() { return tryLock(); }
inline void QMutex::lockInline() { lock(); }
#endif // QT_NO_DEBUG


#else // QT_NO_THREAD


class Q_CORE_EXPORT QMutex
{
public:
    enum RecursionMode { NonRecursive, Recursive };

    inline explicit QMutex(RecursionMode mode = NonRecursive) { Q_UNUSED(mode); }
    inline ~QMutex() {}

    static inline void lock() {}
    static inline void lockInline() {}
    static inline bool tryLock(int timeout = 0) { Q_UNUSED(timeout); return true; }
    static inline bool tryLockInline() { return true; }
    static inline void unlock() {}
    static inline void unlockInline() {}

#if defined(QT3_SUPPORT)
    static inline QT3_SUPPORT bool locked() { return false; }
#endif

private:
    Q_DISABLE_COPY(QMutex)
};

class Q_CORE_EXPORT QMutexLocker
{
public:
    inline explicit QMutexLocker(QMutex *) {}
    inline ~QMutexLocker() {}

    static inline void unlock() {}
    static void relock() {}
    static inline QMutex *mutex() { return 0; }

private:
    Q_DISABLE_COPY(QMutexLocker)
};

#endif // QT_NO_THREAD

QT_END_NAMESPACE

QT_END_HEADER

#endif // QMUTEX_H
