/****************************************************************************
**
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

#include <QtCore/qglobal.h>

#ifndef QGLOBALSTATIC_H
#define QGLOBALSTATIC_H

#include <QtCore/qatomic.h>

QT_BEGIN_NAMESPACE

namespace QtGlobalStatic {
enum GuardValues {
    Destroyed = -2,
    Initialized = -1,
    Uninitialized = 0,
    Initializing = 1
};
}

#if defined(QT_NO_THREAD) || (defined(Q_CC_GNU) && !defined(Q_OS_MAC))
// some compilers support thread-safe statics
// The IA-64 C++ ABI requires this, so we know that all GCC versions since 3.4
// support it. C++11 also requires this behavior.
// Clang and Intel CC masquerade as GCC when compiling on Linux.
//
// Apple's libc++abi however uses a global lock for initializing local statics,
// which will block other threads also trying to initialize a local static
// until the constructor returns ...
// We better avoid these kind of problems by using our own locked implementation.

#define Q_GLOBAL_STATIC_INTERNAL(ARGS)                          \
    Q_DECL_HIDDEN inline Type *innerFunction()                  \
    {                                                           \
        struct HolderBase {                                     \
            ~HolderBase() Q_DECL_NOTHROW                        \
            { if (guard.load() == QtGlobalStatic::Initialized)  \
                  guard.store(QtGlobalStatic::Destroyed); }     \
        };                                                      \
        static struct Holder : public HolderBase {              \
            Type value;                                         \
            Holder()                                            \
                Q_DECL_NOEXCEPT_EXPR(noexcept(Type ARGS))       \
                : value ARGS                                    \
            { guard.store(QtGlobalStatic::Initialized); }       \
        } holder;                                               \
        return &holder.value;                                   \
    }
#else
// We don't know if this compiler supports thread-safe global statics
// so use our own locked implementation

QT_END_NAMESPACE
#include <QtCore/qmutex.h>
QT_BEGIN_NAMESPACE

#define Q_GLOBAL_STATIC_INTERNAL(ARGS)                                  \
    Q_DECL_HIDDEN inline Type *innerFunction()                          \
    {                                                                   \
        static Type *d;                                                 \
        static QBasicMutex mutex;                                       \
        int x = guard.loadAcquire();                                    \
        if (Q_UNLIKELY(x >= QtGlobalStatic::Uninitialized)) {           \
            QMutexLocker locker(&mutex);                                \
            if (guard.load() == QtGlobalStatic::Uninitialized) {        \
                d = new Type ARGS;                                      \
                static struct Cleanup {                                 \
                    ~Cleanup() {                                        \
                        delete d;                                       \
                        guard.store(QtGlobalStatic::Destroyed);         \
                    }                                                   \
                } cleanup;                                              \
                guard.store(QtGlobalStatic::Initialized);               \
            }                                                           \
        }                                                               \
        return d;                                                       \
    }
#endif

// this class must be POD, unless the compiler supports thread-safe statics
template <typename T, T *(&innerFunction)(), QBasicAtomicInt &guard>
struct QGlobalStatic
{
    typedef T Type;

    bool isDestroyed() const { return guard.load() <= QtGlobalStatic::Destroyed; }
    bool exists() const { return guard.load() == QtGlobalStatic::Initialized; }
    operator Type *() { if (isDestroyed()) return 0; return innerFunction(); }
    Type *operator()() { if (isDestroyed()) return 0; return innerFunction(); }
    Type *operator->()
    {
      Q_ASSERT_X(!isDestroyed(), "Q_GLOBAL_STATIC", "The global static was used after being destroyed");
      return innerFunction();
    }
    Type &operator*()
    {
      Q_ASSERT_X(!isDestroyed(), "Q_GLOBAL_STATIC", "The global static was used after being destroyed");
      return *innerFunction();
    }
};

#define Q_GLOBAL_STATIC_WITH_ARGS(TYPE, NAME, ARGS)                         \
    namespace { namespace Q_QGS_ ## NAME {                                  \
        typedef TYPE Type;                                                  \
        QBasicAtomicInt guard = Q_BASIC_ATOMIC_INITIALIZER(QtGlobalStatic::Uninitialized); \
        Q_GLOBAL_STATIC_INTERNAL(ARGS)                                      \
    } }                                                                     \
    static QGlobalStatic<TYPE,                                              \
                         Q_QGS_ ## NAME::innerFunction,                     \
                         Q_QGS_ ## NAME::guard> NAME;

#define Q_GLOBAL_STATIC(TYPE, NAME)                                         \
    Q_GLOBAL_STATIC_WITH_ARGS(TYPE, NAME, ())

QT_END_NAMESPACE
#endif // QGLOBALSTATIC_H
