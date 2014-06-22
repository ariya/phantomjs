/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2011 Thiago Macieira <thiago@kde.org>
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

#ifndef QATOMIC_X86_H
#define QATOMIC_X86_H

#include <QtCore/qgenericatomic.h>

QT_BEGIN_NAMESPACE

#if 0
// silence syncqt warnings
QT_END_NAMESPACE
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#define Q_ATOMIC_INT_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT_REFERENCE_COUNTING_IS_WAIT_FREE

#define Q_ATOMIC_INT_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT_TEST_AND_SET_IS_WAIT_FREE

#define Q_ATOMIC_INT_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT_FETCH_AND_STORE_IS_WAIT_FREE

#define Q_ATOMIC_INT_FETCH_AND_ADD_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT_FETCH_AND_ADD_IS_WAIT_FREE

#define Q_ATOMIC_INT32_IS_SUPPORTED

#define Q_ATOMIC_INT32_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT32_REFERENCE_COUNTING_IS_WAIT_FREE

#define Q_ATOMIC_INT32_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT32_TEST_AND_SET_IS_WAIT_FREE

#define Q_ATOMIC_INT32_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT32_FETCH_AND_STORE_IS_WAIT_FREE

#define Q_ATOMIC_INT32_FETCH_AND_ADD_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT32_FETCH_AND_ADD_IS_WAIT_FREE

#define Q_ATOMIC_POINTER_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_POINTER_TEST_AND_SET_IS_WAIT_FREE

#define Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_WAIT_FREE

#define Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_ALWAYS_NATIVE
#define Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_WAIT_FREE

template <int size> struct QBasicAtomicOps: QGenericAtomicOps<QBasicAtomicOps<size> >
{
    static inline Q_DECL_CONSTEXPR bool isReferenceCountingNative() Q_DECL_NOTHROW { return true; }
    static inline Q_DECL_CONSTEXPR bool isReferenceCountingWaitFree() Q_DECL_NOTHROW { return true; }
    template <typename T> static bool ref(T &_q_value) Q_DECL_NOTHROW;
    template <typename T> static bool deref(T &_q_value) Q_DECL_NOTHROW;

    static inline Q_DECL_CONSTEXPR bool isTestAndSetNative() Q_DECL_NOTHROW { return true; }
    static inline Q_DECL_CONSTEXPR bool isTestAndSetWaitFree() Q_DECL_NOTHROW { return true; }
    template <typename T> static bool testAndSetRelaxed(T &_q_value, T expectedValue, T newValue) Q_DECL_NOTHROW;
    template <typename T> static bool
    testAndSetRelaxed(T &_q_value, T expectedValue, T newValue, T *currentValue) Q_DECL_NOTHROW;

    static inline Q_DECL_CONSTEXPR bool isFetchAndStoreNative() Q_DECL_NOTHROW { return true; }
    static inline Q_DECL_CONSTEXPR bool isFetchAndStoreWaitFree() Q_DECL_NOTHROW { return true; }
    template <typename T> static T fetchAndStoreRelaxed(T &_q_value, T newValue) Q_DECL_NOTHROW;

    static inline Q_DECL_CONSTEXPR bool isFetchAndAddNative() Q_DECL_NOTHROW { return true; }
    static inline Q_DECL_CONSTEXPR bool isFetchAndAddWaitFree() Q_DECL_NOTHROW { return true; }
    template <typename T> static
    T fetchAndAddRelaxed(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW;
};

template <typename T> struct QAtomicOps : QBasicAtomicOps<sizeof(T)>
{
    typedef T Type;
};

#if defined(Q_CC_GNU)

template<> struct QAtomicOpsSupport<1> { enum { IsSupported = 1 }; };
template<> struct QAtomicOpsSupport<2> { enum { IsSupported = 1 }; };
template<> struct QAtomicOpsSupport<8> { enum { IsSupported = 1 }; };

/*
 * Guide for the inline assembly below:
 *
 * x86 instructions are in the form "{opcode}{length} {source}, {destination}",
 * where the length is one of the letters "b" (byte), "w" (word, 16-bit), "l"
 * (dword, 32-bit), "q" (qword, 64-bit).
 *
 * In most cases, we can omit the length because it's inferred from one of the
 * registers. For example, "xchg %0,%1" doesn't need the length suffix because
 * we can only exchange data of the same size and one of the operands must be a
 * register.
 *
 * The exception is the increment and decrement functions, where we add and
 * subtract an immediate value (1). For those, we need to specify the length.
 * GCC and ICC support the syntax "add%z0 $1, %0", where "%z0" expands to the
 * length of the operand. Unfortunately, clang as of 3.0 doesn't support that.
 * For that reason, the ref() and deref() functions are rolled out for all
 * sizes.
 *
 * The functions are also rolled out for the 1-byte operations since those
 * require a special register constraint "q" to force the compiler to schedule
 * one of the 8-bit registers. It's probably a compiler bug that it tries to
 * use a register that doesn't exist.
 *
 * Finally, 64-bit operations are supported via the cmpxchg8b instruction on
 * 32-bit processors, via specialisation below.
 */

template<> template<typename T> inline
bool QBasicAtomicOps<1>::ref(T &_q_value) Q_DECL_NOTHROW
{
    unsigned char ret;
    asm volatile("lock\n"
                 "addb  $1, %0\n"
                 "setne %1"
                 : "=m" (_q_value), "=qm" (ret)
                 : "m" (_q_value)
                 : "memory");
    return ret != 0;
}

template<> template<typename T> inline
bool QBasicAtomicOps<2>::ref(T &_q_value) Q_DECL_NOTHROW
{
    unsigned char ret;
    asm volatile("lock\n"
                 "incw %0\n"
                 "setne %1"
                 : "=m" (_q_value), "=qm" (ret)
                 : "m" (_q_value)
                 : "memory");
    return ret != 0;
}

template<> template<typename T> inline
bool QBasicAtomicOps<4>::ref(T &_q_value) Q_DECL_NOTHROW
{
    unsigned char ret;
    asm volatile("lock\n"
                 "addl $1, %0\n"
                 "setne %1"
                 : "=m" (_q_value), "=qm" (ret)
                 : "m" (_q_value)
                 : "memory");
    return ret != 0;
}

template<> template <typename T> inline
bool QBasicAtomicOps<1>::deref(T &_q_value) Q_DECL_NOTHROW
{
    unsigned char ret;
    asm volatile("lock\n"
                 "subb $1, %0\n"
                 "setne %1"
                 : "=m" (_q_value), "=qm" (ret)
                 : "m" (_q_value)
                 : "memory");
    return ret != 0;
}

template<> template <typename T> inline
bool QBasicAtomicOps<2>::deref(T &_q_value) Q_DECL_NOTHROW
{
    unsigned char ret;
    asm volatile("lock\n"
                 "decw %0\n"
                 "setne %1"
                 : "=m" (_q_value), "=qm" (ret)
                 : "m" (_q_value)
                 : "memory");
    return ret != 0;
}
template<> template <typename T> inline
bool QBasicAtomicOps<4>::deref(T &_q_value) Q_DECL_NOTHROW
{
    unsigned char ret;
    asm volatile("lock\n"
                 "subl $1, %0\n"
                 "setne %1"
                 : "=m" (_q_value), "=qm" (ret)
                 : "m" (_q_value)
                 : "memory");
    return ret != 0;
}

template<int size> template <typename T> inline
bool QBasicAtomicOps<size>::testAndSetRelaxed(T &_q_value, T expectedValue, T newValue) Q_DECL_NOTHROW
{
    unsigned char ret;
    asm volatile("lock\n"
                 "cmpxchg %3,%2\n"
                 "sete %1\n"
                 : "=a" (newValue), "=qm" (ret), "+m" (_q_value)
                 : "r" (newValue), "0" (expectedValue)
                 : "memory");
    return ret != 0;
}

template<> template <typename T> inline
bool QBasicAtomicOps<1>::testAndSetRelaxed(T &_q_value, T expectedValue, T newValue) Q_DECL_NOTHROW
{
    unsigned char ret;
    asm volatile("lock\n"
                 "cmpxchg %3,%2\n"
                 "sete %1\n"
                 : "=a" (newValue), "=qm" (ret), "+m" (_q_value)
                 : "q" (newValue), "0" (expectedValue)
                 : "memory");
    return ret != 0;
}

template<int size> template <typename T> inline
bool QBasicAtomicOps<size>::testAndSetRelaxed(T &_q_value, T expectedValue,
                                              T newValue, T *currentValue) Q_DECL_NOTHROW
{
    unsigned char ret;
    asm volatile("lock\n"
                 "cmpxchg %3,%2\n"
                 "sete %1\n"
                 : "=a" (newValue), "=qm" (ret), "+m" (_q_value)
                 : "r" (newValue), "0" (expectedValue)
                 : "memory");
    *currentValue = newValue;
    return ret != 0;
}

template<> template <typename T> inline
bool QBasicAtomicOps<1>::testAndSetRelaxed(T &_q_value, T expectedValue,
                                           T newValue, T *currentValue) Q_DECL_NOTHROW
{
    unsigned char ret;
    asm volatile("lock\n"
                 "cmpxchg %3,%2\n"
                 "sete %1\n"
                 : "=a" (newValue), "=qm" (ret), "+m" (_q_value)
                 : "q" (newValue), "0" (expectedValue)
                 : "memory");
    *currentValue = newValue;
    return ret != 0;
}

template<int size> template <typename T> inline
T QBasicAtomicOps<size>::fetchAndStoreRelaxed(T &_q_value, T newValue) Q_DECL_NOTHROW
{
    asm volatile("xchg %0,%1"
                 : "=r" (newValue), "+m" (_q_value)
                 : "0" (newValue)
                 : "memory");
    return newValue;
}

template<> template <typename T> inline
T QBasicAtomicOps<1>::fetchAndStoreRelaxed(T &_q_value, T newValue) Q_DECL_NOTHROW
{
    asm volatile("xchg %0,%1"
                 : "=q" (newValue), "+m" (_q_value)
                 : "0" (newValue)
                 : "memory");
    return newValue;
}

template<int size> template <typename T> inline
T QBasicAtomicOps<size>::fetchAndAddRelaxed(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    T result;
    asm volatile("lock\n"
                 "xadd %0,%1"
                 : "=r" (result), "+m" (_q_value)
                 : "0" (T(valueToAdd * QAtomicAdditiveType<T>::AddScale))
                 : "memory");
    return result;
}

template<> template <typename T> inline
T QBasicAtomicOps<1>::fetchAndAddRelaxed(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    T result;
    asm volatile("lock\n"
                 "xadd %0,%1"
                 : "=q" (result), "+m" (_q_value)
                 : "0" (T(valueToAdd * QAtomicAdditiveType<T>::AddScale))
                 : "memory");
    return result;
}

#define Q_ATOMIC_INT8_IS_SUPPORTED

#define Q_ATOMIC_INT8_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT8_REFERENCE_COUNTING_IS_WAIT_FREE

#define Q_ATOMIC_INT8_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT8_TEST_AND_SET_IS_WAIT_FREE

#define Q_ATOMIC_INT8_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT8_FETCH_AND_STORE_IS_WAIT_FREE

#define Q_ATOMIC_INT8_FETCH_AND_ADD_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT8_FETCH_AND_ADD_IS_WAIT_FREE

#define Q_ATOMIC_INT16_IS_SUPPORTED

#define Q_ATOMIC_INT16_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT16_REFERENCE_COUNTING_IS_WAIT_FREE

#define Q_ATOMIC_INT16_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT16_TEST_AND_SET_IS_WAIT_FREE

#define Q_ATOMIC_INT16_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT16_FETCH_AND_STORE_IS_WAIT_FREE

#define Q_ATOMIC_INT16_FETCH_AND_ADD_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT16_FETCH_AND_ADD_IS_WAIT_FREE

#ifdef Q_PROCESSOR_X86_64

#define Q_ATOMIC_INT64_IS_SUPPORTED

#define Q_ATOMIC_INT64_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT64_REFERENCE_COUNTING_IS_WAIT_FREE

#define Q_ATOMIC_INT64_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT64_TEST_AND_SET_IS_WAIT_FREE

#define Q_ATOMIC_INT64_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT64_FETCH_AND_STORE_IS_WAIT_FREE

#define Q_ATOMIC_INT64_FETCH_AND_ADD_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT64_FETCH_AND_ADD_IS_WAIT_FREE

// native support for 64-bit types
template<> template<typename T> inline
bool QBasicAtomicOps<8>::ref(T &_q_value) Q_DECL_NOTHROW
{
    unsigned char ret;
    asm volatile("lock\n"
                 "addq $1, %0\n"
                 "setne %1"
                 : "=m" (_q_value), "=qm" (ret)
                 : "m" (_q_value)
                 : "memory");
    return ret != 0;
}

template<> template <typename T> inline
bool QBasicAtomicOps<8>::deref(T &_q_value) Q_DECL_NOTHROW
{
    unsigned char ret;
    asm volatile("lock\n"
                 "subq $1, %0\n"
                 "setne %1"
                 : "=m" (_q_value), "=qm" (ret)
                 : "m" (_q_value)
                 : "memory");
    return ret != 0;
}
#else
// i386 architecture, emulate 64-bit support via cmpxchg8b
template <> struct QBasicAtomicOps<8>: QGenericAtomicOps<QBasicAtomicOps<8> >
{
    static inline Q_DECL_CONSTEXPR bool isTestAndSetNative() Q_DECL_NOTHROW { return true; }
    static inline Q_DECL_CONSTEXPR bool isTestAndSetWaitFree() Q_DECL_NOTHROW { return true; }
    template <typename T> static inline
    bool testAndSetRelaxed(T &_q_value, T expectedValue, T newValue) Q_DECL_NOTHROW
    {
#ifdef __PIC__
# define EBX_reg "r"
# define EBX_load(reg) "xchg " reg ", %%ebx\n"
#else
# define EBX_reg "b"
# define EBX_load(reg)
#endif
        quint32 highExpectedValue = quint32(newValue >> 32); // ECX
        asm volatile(EBX_load("%3")
                     "lock\n"
                     "cmpxchg8b %0\n"
                     EBX_load("%3")
                     "sete %%cl\n"
                     : "+m" (_q_value), "+c" (highExpectedValue), "+&A" (expectedValue)
                     : EBX_reg (quint32(newValue & 0xffffffff))
                     : "memory");
        // if the comparison failed, expectedValue here contains the current value
        return quint8(highExpectedValue) != 0;
#undef EBX_reg
#undef EBX_load
    }
};
#endif

#else
#  error "This compiler for x86 is not supported"
#endif


QT_END_NAMESPACE

#endif // QATOMIC_X86_H
