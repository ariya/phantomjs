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

#ifndef QATOMIC_UNIX_H
#define QATOMIC_UNIX_H

#include <QtCore/qgenericatomic.h>

QT_BEGIN_NAMESPACE

#if 0
// silence syncqt warnings
QT_END_NAMESPACE
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#define Q_ATOMIC_INT_REFERENCE_COUNTING_IS_NOT_NATIVE
#define Q_ATOMIC_INT_TEST_AND_SET_IS_NOT_NATIVE
#define Q_ATOMIC_INT_FETCH_AND_STORE_IS_NOT_NATIVE
#define Q_ATOMIC_INT_FETCH_AND_ADD_IS_NOT_NATIVE

#define Q_ATOMIC_INT32_IS_SUPPORTED
#define Q_ATOMIC_INT32_REFERENCE_COUNTING_IS_NOT_NATIVE
#define Q_ATOMIC_INT32_TEST_AND_SET_IS_NOT_NATIVE
#define Q_ATOMIC_INT32_FETCH_AND_STORE_IS_NOT_NATIVE
#define Q_ATOMIC_INT32_FETCH_AND_ADD_IS_NOT_NATIVE

#define Q_ATOMIC_INT64_IS_SUPPORTED
#define Q_ATOMIC_INT64_REFERENCE_COUNTING_IS_NOT_NATIVE
#define Q_ATOMIC_INT64_TEST_AND_SET_IS_NOT_NATIVE
#define Q_ATOMIC_INT64_FETCH_AND_STORE_IS_NOT_NATIVE
#define Q_ATOMIC_INT64_FETCH_AND_ADD_IS_NOT_NATIVE

#define Q_ATOMIC_POINTER_TEST_AND_SET_IS_NOT_NATIVE
#define Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_NOT_NATIVE
#define Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_NOT_NATIVE

// No definition, needs specialization
template <typename T> struct QAtomicOps;

// 32-bit version
template <>
struct QAtomicOps<int> : QGenericAtomicOps<QAtomicOps<int> >
{
    typedef int Type;

    static inline Q_DECL_CONSTEXPR bool isTestAndSetNative() Q_DECL_NOTHROW { return false; }
    static inline Q_DECL_CONSTEXPR bool isTestAndSetWaitFree() Q_DECL_NOTHROW { return false; }
    Q_CORE_EXPORT static bool testAndSetRelaxed(int &_q_value, int expectedValue, int newValue) Q_DECL_NOTHROW;
};

// 64-bit version
template <>
struct QAtomicOps<long long> : QGenericAtomicOps<QAtomicOps<long long> >
{
    typedef long long Type;

    static inline Q_DECL_CONSTEXPR bool isTestAndSetNative() Q_DECL_NOTHROW { return false; }
    static inline Q_DECL_CONSTEXPR bool isTestAndSetWaitFree() Q_DECL_NOTHROW { return false; }
    Q_CORE_EXPORT static bool testAndSetRelaxed(Type &_q_value, Type expectedValue, Type newValue) Q_DECL_NOTHROW;
};

// pointer version
template <>
struct QAtomicOps<void *> : QGenericAtomicOps<QAtomicOps<void *> >
{
    typedef void *Type;

    static inline Q_DECL_CONSTEXPR bool isTestAndSetNative() Q_DECL_NOTHROW { return false; }
    static inline Q_DECL_CONSTEXPR bool isTestAndSetWaitFree() Q_DECL_NOTHROW { return false; }
    Q_CORE_EXPORT static bool testAndSetRelaxed(void *&_q_value, void *expectedValue, void *newValue) Q_DECL_NOTHROW;
};

template <typename T>
struct QAtomicOps<T *> : QGenericAtomicOps<QAtomicOps<T *> >
{
    typedef T *Type;

    // helper to strip cv qualifiers
    static inline void *nocv(const T *p) { return const_cast<void *>(static_cast<const volatile void *>(p)); }

    static inline Q_DECL_CONSTEXPR bool isTestAndSetNative() Q_DECL_NOTHROW { return false; }
    static inline Q_DECL_CONSTEXPR bool isTestAndSetWaitFree() Q_DECL_NOTHROW { return false; }
    static inline bool testAndSetRelaxed(T *&_q_value, T *expectedValue, T *newValue) Q_DECL_NOTHROW
    {
        // forward to the void* specialization
        void *voidp = nocv(_q_value);
        bool returnValue = QAtomicOps<void *>::testAndSetRelaxed(voidp, nocv(expectedValue), nocv(newValue));
        _q_value = reinterpret_cast<T *>(voidp);
        return returnValue;
    }
};

// 32- and 64-bit unsigned versions
template <> struct QAtomicOps<unsigned> : QAtomicOps<int>
{
    typedef unsigned Type;
    Q_CORE_EXPORT static bool testAndSetRelaxed(Type &_q_value, Type expectedValue, Type newValue) Q_DECL_NOTHROW
    {
        return QAtomicOps<int>::testAndSetRelaxed(reinterpret_cast<int &>(_q_value), int(expectedValue), int(newValue));
    }
};
template <> struct QAtomicOps<unsigned long long> : QAtomicOps<long long>
{
    typedef unsigned long longType;
    Q_CORE_EXPORT static bool testAndSetRelaxed(Type &_q_value, Type expectedValue, Type newValue) Q_DECL_NOTHROW
    {
        return QAtomicOps<long long>::testAndSetRelaxed(reinterpret_cast<long long &>(_q_value), int(expectedValue), int(newValue));
    }
};

QT_END_NAMESPACE
#endif // QATOMIC_UNIX_H
