/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2011 Thiago Macieira <thiago@kde.org>
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

#ifndef QATOMIC_ARMV5_H
#define QATOMIC_ARMV5_H

#include <QtCore/qgenericatomic.h>

QT_BEGIN_NAMESPACE

#if 0
// silence syncqt warnings
QT_END_NAMESPACE
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#define Q_ATOMIC_INT32_IS_SUPPORTED
#define Q_ATOMIC_INT_REFERENCE_COUNTING_IS_NOT_NATIVE
#define Q_ATOMIC_INT_TEST_AND_SET_IS_NOT_NATIVE
#define Q_ATOMIC_INT_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT_FETCH_AND_STORE_IS_WAIT_FREE
#define Q_ATOMIC_INT_FETCH_AND_ADD_IS_NOT_NATIVE

#define Q_ATOMIC_POINTER_TEST_AND_SET_IS_NOT_NATIVE
#define Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_WAIT_FREE
#define Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_NOT_NATIVE

#ifdef QT_NO_ARM_EABI
# error "Sorry, ARM without EABI is no longer supported"
#endif
#ifndef Q_OS_LINUX
# error "Qt is misconfigured: this ARMv5 implementation is only possible on Linux"
#endif

template <int size> struct QBasicAtomicOps: QGenericAtomicOps<QBasicAtomicOps<size> >
{
    // kernel places a restartable cmpxchg implementation at a fixed address
    template <typename T>
    static int _q_cmpxchg(T oldval, T newval, volatile T *ptr) Q_DECL_NOTHROW
    {
        typedef int (* kernel_cmpxchg_t)(T oldval, T newval, volatile T *ptr);
        kernel_cmpxchg_t kernel_cmpxchg = *reinterpret_cast<kernel_cmpxchg_t>(0xffff0fc0);
        return kernel_cmpxchg(oldval, newval, ptr);
    }
    static void _q_dmb() Q_DECL_NOTHROW
    {
        typedef void (* kernel_dmb_t)();
        kernel_dmb_t kernel_dmb = *reinterpret_cast<kernel_dmb_t>(0xffff0fa0);
        kernel_dmb();
    }

    template <typename T>
    static void orderedMemoryFence(const T &) Q_DECL_NOTHROW { _q_dmb(); }

    template <typename T> static bool ref(T &_q_value) Q_DECL_NOTHROW;
    template <typename T> static bool deref(T &_q_value) Q_DECL_NOTHROW;

    static Q_DECL_CONSTEXPR bool isTestAndSetNative() Q_DECL_NOTHROW { return false; }
    static Q_DECL_CONSTEXPR bool isTestAndSetWaitFree() Q_DECL_NOTHROW { return false; }
    template <typename T> static bool testAndSetRelaxed(T &_q_value, T expectedValue, T newValue) Q_DECL_NOTHROW;
    template <typename T> static bool testAndSetRelaxed(T &_q_value, T expectedValue, T newValue, T *currentValue) Q_DECL_NOTHROW;
    template <typename T> static T fetchAndStoreRelaxed(T &_q_value, T newValue) Q_DECL_NOTHROW;
    template <typename T> static
    T fetchAndAddRelaxed(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW;
};

template <typename T> struct QAtomicOps : QBasicAtomicOps<sizeof(T)>
{
    typedef T Type;
};

template<> template<typename T> inline
bool QBasicAtomicOps<4>::ref(T &_q_value) Q_DECL_NOTHROW
{
    T originalValue;
    T newValue;
    do {
        originalValue = _q_value;
        newValue = originalValue + 1;
    } while (_q_cmpxchg(originalValue, newValue, &_q_value) != 0);
    return newValue != 0;
}

template<> template <typename T> inline
bool QBasicAtomicOps<4>::deref(T &_q_value) Q_DECL_NOTHROW
{
    T originalValue;
    T newValue;
    do {
        originalValue = _q_value;
        newValue = originalValue - 1;
    } while (_q_cmpxchg(originalValue, newValue, &_q_value) != 0);
    return newValue != 0;
}

template<> template <typename T> inline
bool QBasicAtomicOps<4>::testAndSetRelaxed(T &_q_value, T expectedValue, T newValue) Q_DECL_NOTHROW
{
    T originalValue;
    do {
        originalValue = _q_value;
        if (originalValue != expectedValue)
            return false;
    } while (_q_cmpxchg(expectedValue, newValue, &_q_value) != 0);
    return true;
}

template<> template <typename T> inline
bool QBasicAtomicOps<4>::testAndSetRelaxed(T &_q_value, T expectedValue, T newValue, T *currentValue) Q_DECL_NOTHROW
{
    T originalValue;
    do {
        originalValue = _q_value;
        if (originalValue != expectedValue) {
            if (currentValue)
                *currentValue = originalValue;
            return false;
        }
    } while (_q_cmpxchg(expectedValue, newValue, &_q_value) != 0);
    return true;
}

// Fetch and store for integers
#ifdef Q_CC_RVCT
template<> template <typename T> inline
__asm T QBasicAtomicOps<4>::fetchAndStoreRelaxed(T &_q_value, T newValue) Q_DECL_NOTHROW
{
    add r2, pc, #0
    bx r2
    arm
    swp r2,r1,[r0]
    mov r0, r2
    bx lr
    thumb
}
#else
template<> template <typename T> inline
T QBasicAtomicOps<4>::fetchAndStoreRelaxed(T &_q_value, T newValue) Q_DECL_NOTHROW
{
#if defined(__thumb__)
    T originalValue;
    do {
        originalValue = _q_value;
    } while (_q_cmpxchg(originalValue, newValue, &_q_value) != 0);
    return originalValue;
#else
    T originalValue;
    asm volatile("swp %0,%2,[%3]"
                 : "=&r"(originalValue), "=m" (_q_value)
                 : "r"(newValue), "r"(&_q_value)
                 : "cc", "memory");
    return originalValue;
#endif
}
#endif // Q_CC_RVCT

template<> template <typename T> inline
T QBasicAtomicOps<4>::fetchAndAddRelaxed(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    T originalValue;
    T newValue;
    do {
        originalValue = _q_value;
        newValue = originalValue + valueToAdd;
    } while (_q_cmpxchg(originalValue, newValue, &_q_value) != 0);
    return originalValue;
}

QT_END_NAMESPACE

#endif // QATOMIC_ARMV5_H
