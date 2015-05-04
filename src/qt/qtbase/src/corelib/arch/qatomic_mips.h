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

#ifndef QATOMIC_MIPS_H
#define QATOMIC_MIPS_H

#include <QtCore/qgenericatomic.h>

QT_BEGIN_NAMESPACE

#if 0
// silence syncqt warnings
QT_END_NAMESPACE
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#define Q_ATOMIC_INT_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT_FETCH_AND_ADD_IS_ALWAYS_NATIVE

#define Q_ATOMIC_INT32_IS_SUPPORTED
#define Q_ATOMIC_INT32_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT32_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT32_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT32_FETCH_AND_ADD_IS_ALWAYS_NATIVE

#define Q_ATOMIC_POINTER_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_ALWAYS_NATIVE

template <int size> struct QBasicAtomicOps: QGenericAtomicOps<QBasicAtomicOps<size> >
{
    template <typename T>
    static void acquireMemoryFence(const T &) Q_DECL_NOTHROW;
    template <typename T>
    static void releaseMemoryFence(const T &) Q_DECL_NOTHROW;
    template <typename T>
    static void orderedMemoryFence(const T &) Q_DECL_NOTHROW;

    static inline Q_DECL_CONSTEXPR bool isReferenceCountingNative() Q_DECL_NOTHROW { return true; }
    template <typename T> static bool ref(T &_q_value) Q_DECL_NOTHROW;
    template <typename T> static bool deref(T &_q_value) Q_DECL_NOTHROW;

    static inline Q_DECL_CONSTEXPR bool isTestAndSetNative() Q_DECL_NOTHROW { return true; }
    static inline Q_DECL_CONSTEXPR bool isTestAndSetWaitFree() Q_DECL_NOTHROW { return false; }
    template <typename T> static bool
    testAndSetRelaxed(T &_q_value, T expectedValue, T newValue, T *currentValue = 0) Q_DECL_NOTHROW;

    static inline Q_DECL_CONSTEXPR bool isFetchAndStoreNative() Q_DECL_NOTHROW { return true; }
    template <typename T> static T fetchAndStoreRelaxed(T &_q_value, T newValue) Q_DECL_NOTHROW;

    static inline Q_DECL_CONSTEXPR bool isFetchAndAddNative() Q_DECL_NOTHROW { return true; }
    template <typename T> static
    T fetchAndAddRelaxed(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW;
};

template <typename T> struct QAtomicOps : QBasicAtomicOps<sizeof(T)>
{
    typedef T Type;
};

#if defined(Q_CC_GNU)

#if defined(_MIPS_ARCH_MIPS1) || (!defined(Q_CC_CLANG) && defined(__mips) && __mips - 0 == 1)
# error "Sorry, the MIPS1 architecture is not supported"
# error "please set '-march=' to your architecture (e.g., -march=mips32)"
#endif

template <int size> template <typename T> inline
void QBasicAtomicOps<size>::acquireMemoryFence(const T &) Q_DECL_NOTHROW
{
    asm volatile (".set push\n"
                  ".set mips32\n"
                  "sync 0x11\n"
                  ".set pop\n" ::: "memory");
}

template <int size> template <typename T> inline
void QBasicAtomicOps<size>::releaseMemoryFence(const T &) Q_DECL_NOTHROW
{
    asm volatile (".set push\n"
                  ".set mips32\n"
                  "sync 0x12\n"
                  ".set pop\n" ::: "memory");
}

template <int size> template <typename T> inline
void QBasicAtomicOps<size>::orderedMemoryFence(const T &) Q_DECL_NOTHROW
{
    asm volatile ("sync 0" ::: "memory");
}

template<> template<typename T> inline
bool QBasicAtomicOps<4>::ref(T &_q_value) Q_DECL_NOTHROW
{
    T originalValue;
    T newValue;
    asm volatile("0:\n"
                 "ll %[originalValue], %[_q_value]\n"
                 "addiu %[newValue], %[originalValue], %[one]\n"
                 "sc %[newValue], %[_q_value]\n"
                 "beqz %[newValue], 0b\n"
                 "nop\n"
                 : [originalValue] "=&r" (originalValue),
                   [_q_value] "+m" (_q_value),
                   [newValue] "=&r" (newValue)
                 : [one] "i" (1)
                 : "cc", "memory");
    return originalValue != T(-1);
}

template<> template<typename T> inline
bool QBasicAtomicOps<4>::deref(T &_q_value) Q_DECL_NOTHROW
{
    T originalValue;
    T newValue;
    asm volatile("0:\n"
                 "ll %[originalValue], %[_q_value]\n"
                 "addiu %[newValue], %[originalValue], %[minusOne]\n"
                 "sc %[newValue], %[_q_value]\n"
                 "beqz %[newValue], 0b\n"
                 "nop\n"
                 : [originalValue] "=&r" (originalValue),
                   [_q_value] "+m" (_q_value),
                   [newValue] "=&r" (newValue)
                 : [minusOne] "i" (-1)
                 : "cc", "memory");
    return originalValue != 1;
}

template<> template <typename T> inline
bool QBasicAtomicOps<4>::testAndSetRelaxed(T &_q_value, T expectedValue, T newValue, T *currentValue) Q_DECL_NOTHROW
{
    T result;
    T tempValue;
    asm volatile("0:\n"
                 "ll %[tempValue], %[_q_value]\n"
                 "xor %[result], %[tempValue], %[expectedValue]\n"
                 "bnez %[result], 0f\n"
                 "nop\n"
                 "move %[tempValue], %[newValue]\n"
                 "sc %[tempValue], %[_q_value]\n"
                 "beqz %[tempValue], 0b\n"
                 "nop\n"
                 "0:\n"
                 : [result] "=&r" (result),
                   [tempValue] "=&r" (tempValue),
                   [_q_value] "+m" (_q_value)
                 : [expectedValue] "r" (expectedValue),
                   [newValue] "r" (newValue)
                 : "cc", "memory");
    if (currentValue)
        *currentValue = tempValue;
    return result == 0;
}

template<> template <typename T> inline
T QBasicAtomicOps<4>::fetchAndStoreRelaxed(T &_q_value, T newValue) Q_DECL_NOTHROW
{
    T originalValue;
    T tempValue;
    asm volatile("0:\n"
                 "ll %[originalValue], %[_q_value]\n"
                 "move %[tempValue], %[newValue]\n"
                 "sc %[tempValue], %[_q_value]\n"
                 "beqz %[tempValue], 0b\n"
                 "nop\n"
                 : [originalValue] "=&r" (originalValue),
                   [tempValue] "=&r" (tempValue),
                   [_q_value] "+m" (_q_value)
                 : [newValue] "r" (newValue)
                 : "cc", "memory");
    return originalValue;
}

template<> template <typename T> inline
T QBasicAtomicOps<4>::fetchAndAddRelaxed(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    T originalValue;
    T newValue;
    asm volatile("0:\n"
                 "ll %[originalValue], %[_q_value]\n"
                 "addu %[newValue], %[originalValue], %[valueToAdd]\n"
                 "sc %[newValue], %[_q_value]\n"
                 "beqz %[newValue], 0b\n"
                 "nop\n"
                 : [originalValue] "=&r" (originalValue),
                   [_q_value] "+m" (_q_value),
                   [newValue] "=&r" (newValue)
                 : [valueToAdd] "r" (valueToAdd * QAtomicAdditiveType<T>::AddScale)
                 : "cc", "memory");
    return originalValue;
}

#if defined(Q_PROCESSOR_MIPS_64)

#define Q_ATOMIC_INT64_IS_SUPPORTED
#define Q_ATOMIC_INT64_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT64_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT64_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT64_FETCH_AND_ADD_IS_ALWAYS_NATIVE

template<> struct QAtomicOpsSupport<8> { enum { IsSupported = 1 }; };

template<> template<typename T> inline
bool QBasicAtomicOps<8>::ref(T &_q_value) Q_DECL_NOTHROW
{
    T originalValue;
    T newValue;
    asm volatile("0:\n"
                 "lld %[originalValue], %[_q_value]\n"
                 "addiu %[newValue], %[originalValue], %[one]\n"
                 "scd %[newValue], %[_q_value]\n"
                 "beqz %[newValue], 0b\n"
                 "nop\n"
                 : [originalValue] "=&r" (originalValue),
                   [_q_value] "+m" (_q_value),
                   [newValue] "=&r" (newValue)
                 : [one] "i" (1)
                 : "cc", "memory");
    return originalValue != T(-1);
}

template<> template<typename T> inline
bool QBasicAtomicOps<8>::deref(T &_q_value) Q_DECL_NOTHROW
{
    T originalValue;
    T newValue;
    asm volatile("0:\n"
                 "lld %[originalValue], %[_q_value]\n"
                 "addiu %[newValue], %[originalValue], %[minusOne]\n"
                 "scd %[newValue], %[_q_value]\n"
                 "beqz %[newValue], 0b\n"
                 "nop\n"
                 : [originalValue] "=&r" (originalValue),
                   [_q_value] "+m" (_q_value),
                   [newValue] "=&r" (newValue)
                 : [minusOne] "i" (-1)
                 : "cc", "memory");
    return originalValue != 1;
}

template<> template <typename T> inline
bool QBasicAtomicOps<8>::testAndSetRelaxed(T &_q_value, T expectedValue, T newValue, T *currentValue) Q_DECL_NOTHROW
{
    T result;
    T tempValue;
    asm volatile("0:\n"
                 "lld %[tempValue], %[_q_value]\n"
                 "xor %[result], %[tempValue], %[expectedValue]\n"
                 "bnez %[result], 0f\n"
                 "nop\n"
                 "move %[tempValue], %[newValue]\n"
                 "scd %[tempValue], %[_q_value]\n"
                 "beqz %[tempValue], 0b\n"
                 "nop\n"
                 "0:\n"
                 : [result] "=&r" (result),
                   [tempValue] "=&r" (tempValue),
                   [_q_value] "+m" (_q_value)
                 : [expectedValue] "r" (expectedValue),
                   [newValue] "r" (newValue)
                 : "cc", "memory");
    if (currentValue)
        *currentValue = tempValue;
    return result == 0;
}

template<> template <typename T> inline
T QBasicAtomicOps<8>::fetchAndStoreRelaxed(T &_q_value, T newValue) Q_DECL_NOTHROW
{
    T originalValue;
    T tempValue;
    asm volatile("0:\n"
                 "lld %[originalValue], %[_q_value]\n"
                 "move %[tempValue], %[newValue]\n"
                 "scd %[tempValue], %[_q_value]\n"
                 "beqz %[tempValue], 0b\n"
                 "nop\n"
                 : [originalValue] "=&r" (originalValue),
                   [tempValue] "=&r" (tempValue),
                   [_q_value] "+m" (_q_value)
                 : [newValue] "r" (newValue)
                 : "cc", "memory");
    return originalValue;
}

template<> template <typename T> inline
T QBasicAtomicOps<8>::fetchAndAddRelaxed(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    T originalValue;
    T newValue;
    asm volatile("0:\n"
                 "lld %[originalValue], %[_q_value]\n"
                 "addu %[newValue], %[originalValue], %[valueToAdd]\n"
                 "scd %[newValue], %[_q_value]\n"
                 "beqz %[newValue], 0b\n"
                 "nop\n"
                 : [originalValue] "=&r" (originalValue),
                   [_q_value] "+m" (_q_value),
                   [newValue] "=&r" (newValue)
                 : [valueToAdd] "r" (valueToAdd * QAtomicAdditiveType<T>::AddScale)
                 : "cc", "memory");
    return originalValue;
}

#endif // MIPS64

#else
# error "This compiler for MIPS is not supported"
#endif // Q_CC_GNU

QT_END_NAMESPACE

#endif // QATOMIC_MIPS_H
