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

#ifndef QATOMIC_IA64_H
#define QATOMIC_IA64_H

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

#define Q_ATOMIC_INT32_IS_SUPPORTED

#define Q_ATOMIC_INT32_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT32_REFERENCE_COUNTING_IS_WAIT_FREE

#define Q_ATOMIC_INT32_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT32_TEST_AND_SET_IS_WAIT_FREE

#define Q_ATOMIC_INT32_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT32_FETCH_AND_STORE_IS_WAIT_FREE

#define Q_ATOMIC_INT32_FETCH_AND_ADD_IS_ALWAYS_NATIVE

#define Q_ATOMIC_POINTER_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_POINTER_TEST_AND_SET_IS_WAIT_FREE

#define Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_WAIT_FREE

#define Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_ALWAYS_NATIVE

#define Q_ATOMIC_INT8_IS_SUPPORTED

#define Q_ATOMIC_INT8_REFERENCE_COUNTING_IS_ALWAYS_NATIVE

#define Q_ATOMIC_INT8_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT8_TEST_AND_SET_IS_WAIT_FREE

#define Q_ATOMIC_INT8_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT8_FETCH_AND_STORE_IS_WAIT_FREE

#define Q_ATOMIC_INT8_FETCH_AND_ADD_IS_ALWAYS_NATIVE

#define Q_ATOMIC_INT16_IS_SUPPORTED

#define Q_ATOMIC_INT16_REFERENCE_COUNTING_IS_ALWAYS_NATIVE

#define Q_ATOMIC_INT16_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT16_TEST_AND_SET_IS_WAIT_FREE

#define Q_ATOMIC_INT16_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT16_FETCH_AND_STORE_IS_WAIT_FREE

#define Q_ATOMIC_INT16_FETCH_AND_ADD_IS_ALWAYS_NATIVE

#define Q_ATOMIC_INT64_IS_SUPPORTED

#define Q_ATOMIC_INT64_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT64_REFERENCE_COUNTING_IS_WAIT_FREE

#define Q_ATOMIC_INT64_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT64_TEST_AND_SET_IS_WAIT_FREE

#define Q_ATOMIC_INT64_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT64_FETCH_AND_STORE_IS_WAIT_FREE

#define Q_ATOMIC_INT64_FETCH_AND_ADD_IS_ALWAYS_NATIVE

template<> struct QAtomicOpsSupport<1> { enum { IsSupported = 1 }; };
template<> struct QAtomicOpsSupport<2> { enum { IsSupported = 1 }; };
template<> struct QAtomicOpsSupport<8> { enum { IsSupported = 1 }; };

template <int size> struct QBasicAtomicOps: QGenericAtomicOps<QBasicAtomicOps<size> >
{
    template <typename T>
    static void orderedMemoryFence(const T &) Q_DECL_NOTHROW;

    template <typename T> static inline
    T loadAcquire(const T &_q_value) Q_DECL_NOTHROW
    {
        return *static_cast<const volatile T *>(&_q_value);
    }

    template <typename T> static inline
    void storeRelease(T &_q_value, T newValue) Q_DECL_NOTHROW
    {
        *static_cast<volatile T *>(&_q_value) = newValue;
    }
    static inline Q_DECL_CONSTEXPR bool isReferenceCountingNative() Q_DECL_NOTHROW { return true; }
    static inline Q_DECL_CONSTEXPR bool isReferenceCountingWaitFree() Q_DECL_NOTHROW { return size == 4 || size == 8; }
    template <typename T> static bool ref(T &_q_value) Q_DECL_NOTHROW;
    template <typename T> static bool deref(T &_q_value) Q_DECL_NOTHROW;

    static inline Q_DECL_CONSTEXPR bool isTestAndSetNative() Q_DECL_NOTHROW { return true; }
    static inline Q_DECL_CONSTEXPR bool isTestAndSetWaitFree() Q_DECL_NOTHROW { return true; }
    template <typename T> static bool testAndSetRelaxed(T &_q_value, T expectedValue, T newValue, T *currentValue = 0) Q_DECL_NOTHROW;
    template <typename T> static bool testAndSetAcquire(T &_q_value, T expectedValue, T newValue, T *currentValue = 0) Q_DECL_NOTHROW;
    template <typename T> static bool testAndSetRelease(T &_q_value, T expectedValue, T newValue, T *currentValue = 0) Q_DECL_NOTHROW;
    template <typename T> static bool testAndSetOrdered(T &_q_value, T expectedValue, T newValue, T *currentValue = 0) Q_DECL_NOTHROW;

    static inline Q_DECL_CONSTEXPR bool isFetchAndStoreNative() Q_DECL_NOTHROW { return true; }
    static inline Q_DECL_CONSTEXPR bool isFetchAndStoreWaitFree() Q_DECL_NOTHROW { return true; }
    template <typename T> static T fetchAndStoreRelaxed(T &_q_value, T newValue) Q_DECL_NOTHROW;
    template <typename T> static T fetchAndStoreAcquire(T &_q_value, T newValue) Q_DECL_NOTHROW;
    template <typename T> static T fetchAndStoreRelease(T &_q_value, T newValue) Q_DECL_NOTHROW;
    template <typename T> static T fetchAndStoreOrdered(T &_q_value, T newValue) Q_DECL_NOTHROW;

    static inline Q_DECL_CONSTEXPR bool isFetchAndAddNative() Q_DECL_NOTHROW { return true; }
    static inline Q_DECL_CONSTEXPR bool isFetchAndAddWaitFree() Q_DECL_NOTHROW { return false; }
    template <typename T> static
    T fetchAndAddRelaxed(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW;
    template <typename T> static
    T fetchAndAddAcquire(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW;
    template <typename T> static
    T fetchAndAddRelease(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW;
    template <typename T> static
    T fetchAndAddOrdered(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW;
};

template <typename T> struct QAtomicOps : QBasicAtomicOps<sizeof(T)>
{
    typedef T Type;
};

inline bool _q_ia64_fetchadd_immediate(int value)
{
    return value == 1 || value == -1
        || value == 4 || value == -4
        || value == 8 || value == -8
        || value == 16 || value == -16;
}

#if defined(Q_CC_INTEL)

// intrinsics provided by the Intel C++ Compiler
#include <ia64intrin.h>

template<int size> template <typename T> inline
void QBasicAtomicOps<size>::orderedMemoryFence(const T &)
{
    __memory_barrier();
}

inline int QBasicAtomicInt::fetchAndStoreAcquire(int newValue)
{
    return static_cast<int>(_InterlockedExchange(&_q_value, newValue));
}

inline bool QBasicAtomicInt::testAndSetRelaxed(int expectedValue, int newValue)
{
    int expectedValueCopy = expectedValue;
    return (static_cast<int>(_InterlockedCompareExchange(&_q_value,
                                                         newValue,
                                                         expectedValueCopy))
            == expectedValue);
}

inline bool QBasicAtomicInt::testAndSetAcquire(int expectedValue, int newValue)
{
    int expectedValueCopy = expectedValue;
    return (static_cast<int>(_InterlockedCompareExchange_acq(reinterpret_cast<volatile uint *>(&_q_value),
                                                             newValue,
                                                             expectedValueCopy))
            == expectedValue);
}

inline bool QBasicAtomicInt::testAndSetRelease(int expectedValue, int newValue)
{
    int expectedValueCopy = expectedValue;
    return (static_cast<int>(_InterlockedCompareExchange_rel(reinterpret_cast<volatile uint *>(&_q_value),
                                                             newValue,
                                                             expectedValueCopy))
            == expectedValue);
}

inline int QBasicAtomicInt::fetchAndAddAcquire(int valueToAdd)
{
    if (__builtin_constant_p(valueToAdd)) {
        if (valueToAdd == 1)
            return __fetchadd4_acq((unsigned int *)&_q_value, 1);
        if (valueToAdd == -1)
            return __fetchadd4_acq((unsigned int *)&_q_value, -1);
    }
    return _InterlockedExchangeAdd(&_q_value, valueToAdd);
}

inline int QBasicAtomicInt::fetchAndAddRelease(int valueToAdd)
{
    if (__builtin_constant_p(valueToAdd)) {
        if (valueToAdd == 1)
            return __fetchadd4_rel((unsigned int *)&_q_value, 1);
        if (valueToAdd == -1)
            return __fetchadd4_rel((unsigned int *)&_q_value, -1);
    }
    __memory_barrier();
    return _InterlockedExchangeAdd(&_q_value, valueToAdd);
}

inline bool QBasicAtomicInt::ref()
{
    return _InterlockedIncrement(&_q_value) != 0;
}

inline bool QBasicAtomicInt::deref()
{
    return _InterlockedDecrement(&_q_value) != 0;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreAcquire(T *newValue)
{
    return (T *)_InterlockedExchangePointer(reinterpret_cast<void * volatile*>(&_q_value), newValue);
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetRelaxed(T *expectedValue, T *newValue)
{
    T *expectedValueCopy = expectedValue;
    return (_InterlockedCompareExchangePointer(reinterpret_cast<void * volatile*>(&_q_value),
                                               newValue,
                                               expectedValueCopy)
            == expectedValue);
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetAcquire(T *expectedValue, T *newValue)
{
    union {
        volatile void *x;
        volatile unsigned long *p;
    };
    x = &_q_value;
    T *expectedValueCopy = expectedValue;
    return (_InterlockedCompareExchange64_acq(p, quintptr(newValue), quintptr(expectedValueCopy))
            == quintptr(expectedValue));
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetRelease(T *expectedValue, T *newValue)
{
    union {
        volatile void *x;
        volatile unsigned long *p;
    };
    x = &_q_value;
    T *expectedValueCopy = expectedValue;
    return (_InterlockedCompareExchange64_rel(p, quintptr(newValue), quintptr(expectedValueCopy))
            == quintptr(expectedValue));
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddAcquire(qptrdiff valueToAdd)
{
    return (T *)_InterlockedExchangeAdd64((volatile long *)&_q_value,
                                          valueToAdd * sizeof(T));
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddRelease(qptrdiff valueToAdd)
{
    __memory_barrier();
    return (T *)_InterlockedExchangeAdd64((volatile long *)&_q_value,
                                          valueToAdd * sizeof(T));
}

#elif defined(Q_CC_GNU)

template<int size> template <typename T> inline
void QBasicAtomicOps<size>::orderedMemoryFence(const T &) Q_DECL_NOTHROW
{
    asm volatile("mf" ::: "memory");
}

template<> template<typename T> inline
bool QBasicAtomicOps<4>::ref(T &_q_value) Q_DECL_NOTHROW
{
    T ret;
    asm volatile("fetchadd4.acq %0=%1,1\n"
                 : "=r" (ret), "+m" (_q_value)
                 :
                 : "memory");
    return ret != -1;
}

template<> template<typename T> inline
bool QBasicAtomicOps<4>::deref(T &_q_value) Q_DECL_NOTHROW
{
    T ret;
    asm volatile("fetchadd4.rel %0=%1,-1\n"
                 : "=r" (ret), "+m" (_q_value)
                 :
                 : "memory");
    return ret != 1;
}

template<> template<typename T> inline
bool QBasicAtomicOps<8>::ref(T &_q_value) Q_DECL_NOTHROW
{
    T ret;
    asm volatile("fetchadd8.acq %0=%1,1\n"
                 : "=r" (ret), "+m" (_q_value)
                 :
                 : "memory");
    return ret != -1;
}

template<> template<typename T> inline
bool QBasicAtomicOps<8>::deref(T &_q_value) Q_DECL_NOTHROW
{
    T ret;
    asm volatile("fetchadd8.rel %0=%1,-1\n"
                 : "=r" (ret), "+m" (_q_value)
                 :
                 : "memory");
    return ret != 1;
}

template<> template <typename T> inline
bool QBasicAtomicOps<1>::testAndSetAcquire(T &_q_value, T expectedValue, T newValue, T *currentValue) Q_DECL_NOTHROW
{
    T ret;
    asm volatile("mov ar.ccv=%2\n"
                 ";;\n"
                 "cmpxchg1.acq %0=%1,%3,ar.ccv\n"
                 : "=r" (ret), "+m" (_q_value)
                 : "r" (expectedValue), "r" (newValue)
                 : "memory");
    if (currentValue)
        *currentValue = ret;
    return ret == expectedValue;
}

template<> template <typename T> inline
bool QBasicAtomicOps<1>::testAndSetRelease(T &_q_value, T expectedValue, T newValue, T *currentValue) Q_DECL_NOTHROW
{
    T ret;
    asm volatile("mov ar.ccv=%2\n"
                 ";;\n"
                 "cmpxchg1.rel %0=%1,%3,ar.ccv\n"
                 : "=r" (ret), "+m" (_q_value)
                 : "r" (expectedValue), "r" (newValue)
                 : "memory");
    if (currentValue)
        *currentValue = ret;
    return ret == expectedValue;
}

template<> template <typename T> inline
bool QBasicAtomicOps<2>::testAndSetAcquire(T &_q_value, T expectedValue, T newValue, T *currentValue) Q_DECL_NOTHROW
{
    T ret;
    asm volatile("mov ar.ccv=%2\n"
                 ";;\n"
                 "cmpxchg2.acq %0=%1,%3,ar.ccv\n"
                 : "=r" (ret), "+m" (_q_value)
                 : "r" (expectedValue), "r" (newValue)
                 : "memory");
    if (currentValue)
        *currentValue = ret;
    return ret == expectedValue;
}

template<> template <typename T> inline
bool QBasicAtomicOps<2>::testAndSetRelease(T &_q_value, T expectedValue, T newValue, T *currentValue) Q_DECL_NOTHROW
{
    T ret;
    asm volatile("mov ar.ccv=%2\n"
                 ";;\n"
                 "cmpxchg2.rel %0=%1,%3,ar.ccv\n"
                 : "=r" (ret), "+m" (_q_value)
                 : "r" (expectedValue), "r" (newValue)
                 : "memory");
    if (currentValue)
        *currentValue = ret;
    return ret == expectedValue;
}

template<> template <typename T> inline
bool QBasicAtomicOps<4>::testAndSetAcquire(T &_q_value, T expectedValue, T newValue, T *currentValue) Q_DECL_NOTHROW
{
    T ret;
    asm volatile("mov ar.ccv=%2\n"
                 ";;\n"
                 "cmpxchg4.acq %0=%1,%3,ar.ccv\n"
                 : "=r" (ret), "+m" (_q_value)
                 : "r" (expectedValue), "r" (newValue)
                 : "memory");
    if (currentValue)
        *currentValue = ret;
    return ret == expectedValue;
}

template<> template <typename T> inline
bool QBasicAtomicOps<4>::testAndSetRelease(T &_q_value, T expectedValue, T newValue, T *currentValue) Q_DECL_NOTHROW
{
    T ret;
    asm volatile("mov ar.ccv=%2\n"
                 ";;\n"
                 "cmpxchg4.rel %0=%1,%3,ar.ccv\n"
                 : "=r" (ret), "+m" (_q_value)
                 : "r" (expectedValue), "r" (newValue)
                 : "memory");
    if (currentValue)
        *currentValue = ret;
    return ret == expectedValue;
}

template<> template <typename T> inline
bool QBasicAtomicOps<8>::testAndSetAcquire(T &_q_value, T expectedValue, T newValue, T *currentValue) Q_DECL_NOTHROW
{
    T ret;
    asm volatile("mov ar.ccv=%2\n"
                 ";;\n"
                 "cmpxchg8.acq %0=%1,%3,ar.ccv\n"
                 : "=r" (ret), "+m" (_q_value)
                 : "r" (expectedValue), "r" (newValue)
                 : "memory");
    if (currentValue)
        *currentValue = ret;
    return ret == expectedValue;
}

template<> template <typename T> inline
bool QBasicAtomicOps<8>::testAndSetRelease(T &_q_value, T expectedValue, T newValue, T *currentValue) Q_DECL_NOTHROW
{
    T ret;
    asm volatile("mov ar.ccv=%2\n"
                 ";;\n"
                 "cmpxchg8.rel %0=%1,%3,ar.ccv\n"
                 : "=r" (ret), "+m" (_q_value)
                 : "r" (expectedValue), "r" (newValue)
                 : "memory");
    if (currentValue)
        *currentValue = ret;
    return ret == expectedValue;
}

template<> template <typename T> inline
T QBasicAtomicOps<1>::fetchAndStoreAcquire(T &_q_value, T newValue) Q_DECL_NOTHROW
{
    T ret;
    asm volatile("xchg1 %0=%1,%2\n"
                 : "=r" (ret), "+m" (_q_value)
                 : "r" (newValue)
                 : "memory");
    return ret;
}

template<> template <typename T> inline
T QBasicAtomicOps<2>::fetchAndStoreAcquire(T &_q_value, T newValue) Q_DECL_NOTHROW
{
    T ret;
    asm volatile("xchg2 %0=%1,%2\n"
                 : "=r" (ret), "+m" (_q_value)
                 : "r" (newValue)
                 : "memory");
    return ret;
}

template<> template <typename T> inline
T QBasicAtomicOps<4>::fetchAndStoreAcquire(T &_q_value, T newValue) Q_DECL_NOTHROW
{
    T ret;
    asm volatile("xchg4 %0=%1,%2\n"
                 : "=r" (ret), "+m" (_q_value)
                 : "r" (newValue)
                 : "memory");
    return ret;
}

template<> template <typename T> inline
T QBasicAtomicOps<8>::fetchAndStoreAcquire(T &_q_value, T newValue) Q_DECL_NOTHROW
{
    T ret;
    asm volatile("xchg8 %0=%1,%2\n"
                 : "=r" (ret), "+m" (_q_value)
                 : "r" (newValue)
                 : "memory");
    return ret;
}

template<> template <typename T> inline
T QBasicAtomicOps<1>::fetchAndAddAcquire(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    T ret;
    valueToAdd *= QAtomicAdditiveType<T>::AddScale;

    ret = _q_value;
    asm volatile("0:\n"
                 "      mov           r9=%0\n"
                 "      mov           ar.ccv=%0\n"
                 "      add           %0=%0, %2\n"
                 "      ;;\n"
                 "      cmpxchg1.acq %0=%1,%0,ar.ccv\n"
                 "      ;;\n"
                 "      cmp.ne       p6,p0 = %0, r9\n"
                 "(p6)  br.dptk      0b\n"
                 "1:\n"
                 : "+r" (ret), "+m" (_q_value)
                 : "r" (valueToAdd)
                 : "r9", "p6", "memory");
    return ret;
}

template<> template <typename T> inline
T QBasicAtomicOps<1>::fetchAndAddRelease(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    T ret;
    valueToAdd *= QAtomicAdditiveType<T>::AddScale;

    ret = _q_value;
    asm volatile("0:\n"
                 "      mov           r9=%0\n"
                 "      mov           ar.ccv=%0\n"
                 "      add           %0=%0, %2\n"
                 "      ;;\n"
                 "      cmpxchg1.rel %0=%1,%0,ar.ccv\n"
                 "      ;;\n"
                 "      cmp.ne       p6,p0 = %0, r9\n"
                 "(p6)  br.dptk      0b\n"
                 "1:\n"
                 : "+r" (ret), "+m" (_q_value)
                 : "r" (valueToAdd)
                 : "r9", "p6", "memory");
    return ret;
}

template<> template <typename T> inline
T QBasicAtomicOps<2>::fetchAndAddAcquire(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    T ret;
    valueToAdd *= QAtomicAdditiveType<T>::AddScale;

    ret = _q_value;
    asm volatile("0:\n"
                 "      mov           r9=%0\n"
                 "      mov           ar.ccv=%0\n"
                 "      add           %0=%0, %2\n"
                 "      ;;\n"
                 "      cmpxchg2.acq %0=%1,%0,ar.ccv\n"
                 "      ;;\n"
                 "      cmp.ne       p6,p0 = %0, r9\n"
                 "(p6)  br.dptk      0b\n"
                 "1:\n"
                 : "+r" (ret), "+m" (_q_value)
                 : "r" (valueToAdd)
                 : "r9", "p6", "memory");
    return ret;
}

template<> template <typename T> inline
T QBasicAtomicOps<2>::fetchAndAddRelease(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    T ret;
    valueToAdd *= QAtomicAdditiveType<T>::AddScale;

    ret = _q_value;
    asm volatile("0:\n"
                 "      mov           r9=%0\n"
                 "      mov           ar.ccv=%0\n"
                 "      add           %0=%0, %2\n"
                 "      ;;\n"
                 "      cmpxchg2.rel %0=%1,%0,ar.ccv\n"
                 "      ;;\n"
                 "      cmp.ne       p6,p0 = %0, r9\n"
                 "(p6)  br.dptk      0b\n"
                 "1:\n"
                 : "+r" (ret), "+m" (_q_value)
                 : "r" (valueToAdd)
                 : "r9", "p6", "memory");
    return ret;
}

template<> template <typename T> inline
T QBasicAtomicOps<4>::fetchAndAddAcquire(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    T ret;
    valueToAdd *= QAtomicAdditiveType<T>::AddScale;

#if (__GNUC__ >= 4)
    // We implement a fast fetch-and-add when we can
    if (__builtin_constant_p(valueToAdd) && _q_ia64_fetchadd_immediate(valueToAdd)) {
        asm volatile("fetchadd4.acq  %0=%1,%2\n"
                     : "=r" (ret), "+m" (_q_value)
                     : "i" (valueToAdd)
                     : "memory");
        return ret;
    }
#endif

    // otherwise, use a loop around test-and-set
    ret = _q_value;
    asm volatile("0:\n"
                 "      mov           r9=%0\n"
                 "      mov           ar.ccv=%0\n"
                 "      add           %0=%0, %2\n"
                 "      ;;\n"
                 "      cmpxchg4.acq %0=%1,%0,ar.ccv\n"
                 "      ;;\n"
                 "      cmp.ne       p6,p0 = %0, r9\n"
                 "(p6)  br.dptk      0b\n"
                 "1:\n"
                 : "+r" (ret), "+m" (_q_value)
                 : "r" (valueToAdd)
                 : "r9", "p6", "memory");
    return ret;
}

template<> template <typename T> inline
T QBasicAtomicOps<4>::fetchAndAddRelease(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    T ret;
    valueToAdd *= QAtomicAdditiveType<T>::AddScale;

#if (__GNUC__ >= 4)
    // We implement a fast fetch-and-add when we can
    if (__builtin_constant_p(valueToAdd) && _q_ia64_fetchadd_immediate(valueToAdd)) {
        asm volatile("fetchadd4.rel  %0=%1,%2\n"
                     : "=r" (ret), "+m" (_q_value)
                     : "i" (valueToAdd)
                     : "memory");
        return ret;
    }
#endif

    // otherwise, use a loop around test-and-set
    ret = _q_value;
    asm volatile("0:\n"
                 "      mov           r9=%0\n"
                 "      mov           ar.ccv=%0\n"
                 "      add           %0=%0, %2\n"
                 "      ;;\n"
                 "      cmpxchg4.rel %0=%1,%0,ar.ccv\n"
                 "      ;;\n"
                 "      cmp.ne       p6,p0 = %0, r9\n"
                 "(p6)  br.dptk      0b\n"
                 "1:\n"
                 : "+r" (ret), "+m" (_q_value)
                 : "r" (valueToAdd)
                 : "r9", "p6", "memory");
    return ret;
}

template<> template <typename T> inline
T QBasicAtomicOps<8>::fetchAndAddAcquire(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    T ret;
    valueToAdd *= QAtomicAdditiveType<T>::AddScale;

#if (__GNUC__ >= 4)
    // We implement a fast fetch-and-add when we can
    if (__builtin_constant_p(valueToAdd) && _q_ia64_fetchadd_immediate(valueToAdd)) {
        asm volatile("fetchadd8.acq  %0=%1,%2\n"
                     : "=r" (ret), "+m" (_q_value)
                     : "i" (valueToAdd)
                     : "memory");
        return ret;
    }
#endif

    // otherwise, use a loop around test-and-set
    ret = _q_value;
    asm volatile("0:\n"
                 "      mov           r9=%0\n"
                 "      mov           ar.ccv=%0\n"
                 "      add           %0=%0, %2\n"
                 "      ;;\n"
                 "      cmpxchg8.acq %0=%1,%0,ar.ccv\n"
                 "      ;;\n"
                 "      cmp.ne       p6,p0 = %0, r9\n"
                 "(p6)  br.dptk      0b\n"
                 "1:\n"
                 : "+r" (ret), "+m" (_q_value)
                 : "r" (valueToAdd)
                 : "r9", "p6", "memory");
    return ret;
}

template<> template <typename T> inline
T QBasicAtomicOps<8>::fetchAndAddRelease(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    T ret;
    valueToAdd *= QAtomicAdditiveType<T>::AddScale;

#if (__GNUC__ >= 4)
    // We implement a fast fetch-and-add when we can
    if (__builtin_constant_p(valueToAdd) && _q_ia64_fetchadd_immediate(valueToAdd)) {
        asm volatile("fetchadd8.rel  %0=%1,%2\n"
                     : "=r" (ret), "+m" (_q_value)
                     : "i" (valueToAdd)
                     : "memory");
        return ret;
    }
#endif

    // otherwise, use a loop around test-and-set
    ret = _q_value;
    asm volatile("0:\n"
                 "      mov           r9=%0\n"
                 "      mov           ar.ccv=%0\n"
                 "      add           %0=%0, %2\n"
                 "      ;;\n"
                 "      cmpxchg8.rel %0=%1,%0,ar.ccv\n"
                 "      ;;\n"
                 "      cmp.ne       p6,p0 = %0, r9\n"
                 "(p6)  br.dptk      0b\n"
                 "1:\n"
                 : "+r" (ret), "+m" (_q_value)
                 : "r" (valueToAdd)
                 : "r9", "p6", "memory");
    return ret;
}

#elif defined Q_CC_HPACC

QT_BEGIN_INCLUDE_NAMESPACE
#include <ia64/sys/inline.h>
QT_END_INCLUDE_NAMESPACE

#define FENCE (_Asm_fence)(_UP_CALL_FENCE | _UP_SYS_FENCE | _DOWN_CALL_FENCE | _DOWN_SYS_FENCE)

template <int size> inline
void QBasicAtomicOps<size>::orderedMemoryFence() Q_DECL_NOTHROW
{
    _Asm_mf(FENCE);
}

template<> template<typename T> inline
bool QBasicAtomicOps<4>::ref(T &_q_value) Q_DECL_NOTHROW
{
    return (T)_Asm_fetchadd((_Asm_fasz)_FASZ_W, (_Asm_sem)_SEM_ACQ,
                              &_q_value, 1, (_Asm_ldhint)_LDHINT_NONE, FENCE) != -1;
}

template<> template<typename T> inline
bool QBasicAtomicOps<4>::deref(T &_q_value) Q_DECL_NOTHROW
{
    return (T)_Asm_fetchadd((_Asm_fasz)_FASZ_W, (_Asm_sem)_SEM_REL,
                              &_q_value, -1, (_Asm_ldhint)_LDHINT_NONE, FENCE) != 1;
}

template<> template<typename T> inline
bool QBasicAtomicOps<8>::ref(T &_q_value) Q_DECL_NOTHROW
{
    return (T)_Asm_fetchadd((_Asm_fasz)_FASZ_D, (_Asm_sem)_SEM_ACQ,
                              &_q_value, 1, (_Asm_ldhint)_LDHINT_NONE, FENCE) != -1;
}

template<> template<typename T> inline
bool QBasicAtomicOps<8>::deref(T &_q_value) Q_DECL_NOTHROW
{
    return (T)_Asm_fetchadd((_Asm_fasz)_FASZ_D, (_Asm_sem)_SEM_REL,
                              &_q_value, -1, (_Asm_ldhint)_LDHINT_NONE, FENCE) != 1;
}

template<> template <typename T> inline
bool QBasicAtomicOps<1>::testAndSetAcquire(T &_q_value, T expectedValue, T newValue) Q_DECL_NOTHROW
{
    _Asm_mov_to_ar((_Asm_app_reg)_AREG_CCV, (quint8)expectedValue, FENCE);
    T ret = (T)_Asm_cmpxchg((_Asm_sz)_SZ_B, (_Asm_sem)_SEM_ACQ,
                            &_q_value, (quint8)newValue, (_Asm_ldhint)_LDHINT_NONE);
    return ret == expectedValue;
}

template<> template <typename T> inline
bool QBasicAtomicOps<1>::testAndSetRelease(T &_q_value, T expectedValue, T newValue) Q_DECL_NOTHROW
{
    _Asm_mov_to_ar((_Asm_app_reg)_AREG_CCV, (quint8)expectedValue, FENCE);
    T ret = (T)_Asm_cmpxchg((_Asm_sz)_SZ_B, (_Asm_sem)_SEM_REL,
                            &_q_value, (quint8)newValue, (_Asm_ldhint)_LDHINT_NONE);
    return ret == expectedValue;
}

template<> template <typename T> inline
bool QBasicAtomicOps<2>::testAndSetAcquire(T &_q_value, T expectedValue, T newValue) Q_DECL_NOTHROW
{
    _Asm_mov_to_ar((_Asm_app_reg)_AREG_CCV, (quint16)expectedValue, FENCE);
    T ret = (T)_Asm_cmpxchg((_Asm_sz)_SZ_H, (_Asm_sem)_SEM_ACQ,
                            &_q_value, (quint16)newValue, (_Asm_ldhint)_LDHINT_NONE);
    return ret == expectedValue;
}

template<> template <typename T> inline
bool QBasicAtomicOps<2>::testAndSetRelease(T &_q_value, T expectedValue, T newValue) Q_DECL_NOTHROW
{
    _Asm_mov_to_ar((_Asm_app_reg)_AREG_CCV, (quint16)expectedValue, FENCE);
    T ret = (T)_Asm_cmpxchg((_Asm_sz)_SZ_H, (_Asm_sem)_SEM_REL,
                            &_q_value, (quint16)newValue, (_Asm_ldhint)_LDHINT_NONE);
    return ret == expectedValue;
}

template<> template <typename T> inline
bool QBasicAtomicOps<4>::testAndSetAcquire(T &_q_value, T expectedValue, T newValue) Q_DECL_NOTHROW
{
    _Asm_mov_to_ar((_Asm_app_reg)_AREG_CCV, (unsigned)expectedValue, FENCE);
    T ret = (T)_Asm_cmpxchg((_Asm_sz)_SZ_W, (_Asm_sem)_SEM_ACQ,
                            &_q_value, (unsigned)newValue, (_Asm_ldhint)_LDHINT_NONE);
    return ret == expectedValue;
}

template<> template <typename T> inline
bool QBasicAtomicOps<4>::testAndSetRelease(T &_q_value, T expectedValue, T newValue) Q_DECL_NOTHROW
{
    _Asm_mov_to_ar((_Asm_app_reg)_AREG_CCV, (unsigned)expectedValue, FENCE);
    T ret = (T)_Asm_cmpxchg((_Asm_sz)_SZ_W, (_Asm_sem)_SEM_REL,
                            &_q_value, newValue, (_Asm_ldhint)_LDHINT_NONE);
    return ret == expectedValue;
}

template<> template <typename T> inline
bool QBasicAtomicOps<8>::testAndSetAcquire(T &_q_value, T expectedValue, T newValue) Q_DECL_NOTHROW
{
    _Asm_mov_to_ar((_Asm_app_reg)_AREG_CCV, (quint64)expectedValue, FENCE);
    T ret = (T)_Asm_cmpxchg((_Asm_sz)_SZ_D, (_Asm_sem)_SEM_ACQ,
                            &_q_value, (quint64)newValue, (_Asm_ldhint)_LDHINT_NONE);
    return ret == expectedValue;
}

template<> template <typename T> inline
bool QBasicAtomicOps<8>::testAndSetRelease(T &_q_value, T expectedValue, T newValue) Q_DECL_NOTHROW
{
    _Asm_mov_to_ar((_Asm_app_reg)_AREG_CCV, (quint64)expectedValue, FENCE);
    T ret = (T)_Asm_cmpxchg((_Asm_sz)_SZ_D, (_Asm_sem)_SEM_REL,
                            &_q_value, (quint64)newValue, (_Asm_ldhint)_LDHINT_NONE);
    return ret == expectedValue;
}

template<> template <typename T> inline
T QBasicAtomicOps<1>::fetchAndStoreAcquire(T &_q_value, T newValue) Q_DECL_NOTHROW
{
    return (T)_Asm_xchg((_Asm_sz)_SZ_B, &_q_value, (quint8)newValue,
                        (_Asm_ldhint)_LDHINT_NONE, FENCE);
}

template<> template <typename T> inline
T QBasicAtomicOps<2>::fetchAndStoreAcquire(T &_q_value, T newValue) Q_DECL_NOTHROW
{
    return (T)_Asm_xchg((_Asm_sz)_SZ_H, &_q_value, (quint16)newValue,
                        (_Asm_ldhint)_LDHINT_NONE, FENCE);
}

template<> template <typename T> inline
T QBasicAtomicOps<4>::fetchAndStoreAcquire(T &_q_value, T newValue) Q_DECL_NOTHROW
{
    return (T)_Asm_xchg((_Asm_sz)_SZ_W, &_q_value, (unsigned)newValue,
                        (_Asm_ldhint)_LDHINT_NONE, FENCE);
}

template<> template <typename T> inline
T QBasicAtomicOps<8>::fetchAndStoreAcquire(T &_q_value, T newValue) Q_DECL_NOTHROW
{
    return (T)_Asm_xchg((_Asm_sz)_SZ_D, &_q_value, (quint64)newValue,
                        (_Asm_ldhint)_LDHINT_NONE, FENCE);
}

template<> template <typename T> inline
T QBasicAtomicOps<1>::fetchAndAddAcquire(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    valueToAdd *= QAtomicAdditiveType<T>::AddScale;
    // implement the test-and-set loop
    T old, ret;
    do {
        old = _q_value;
        _Asm_mov_to_ar((_Asm_app_reg)_AREG_CCV, (quint8)old, FENCE);
        ret = _Asm_cmpxchg((_Asm_sz)_SZ_B, (_Asm_sem)_SEM_ACQ,
                           &_q_value, old + valueToAdd, (_Asm_ldhint)_LDHINT_NONE);
    } while (ret != old);
    return old;
}

template<> template <typename T> inline
T QBasicAtomicOps<1>::fetchAndAddRelaxed(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    // implement the test-and-set loop
    T old, ret;
    do {
        old = _q_value;
        _Asm_mov_to_ar((_Asm_app_reg)_AREG_CCV, (quint8)old, FENCE);
        ret = _Asm_cmpxchg((_Asm_sz)_SZ_B, (_Asm_sem)_SEM_REL,
                           &_q_value, old + valueToAdd, (_Asm_ldhint)_LDHINT_NONE);
    } while (ret != old);
    return old;
}

template<> template <typename T> inline
T QBasicAtomicOps<2>::fetchAndAddAcquire(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    valueToAdd *= QAtomicAdditiveType<T>::AddScale;
    // implement the test-and-set loop
    T old, ret;
    do {
        old = _q_value;
        _Asm_mov_to_ar((_Asm_app_reg)_AREG_CCV, (quint16)old, FENCE);
        ret = _Asm_cmpxchg((_Asm_sz)_SZ_H, (_Asm_sem)_SEM_ACQ,
                           &_q_value, old + valueToAdd, (_Asm_ldhint)_LDHINT_NONE);
    } while (ret != old);
    return old;
}

template<> template <typename T> inline
T QBasicAtomicOps<2>::fetchAndAddRelaxed(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    // implement the test-and-set loop
    T old, ret;
    do {
        old = _q_value;
        _Asm_mov_to_ar((_Asm_app_reg)_AREG_CCV, (quint16)old, FENCE);
        ret = _Asm_cmpxchg((_Asm_sz)_SZ_H, (_Asm_sem)_SEM_REL,
                           &_q_value, old + valueToAdd, (_Asm_ldhint)_LDHINT_NONE);
    } while (ret != old);
    return old;
}

template<> template <typename T> inline
T QBasicAtomicOps<4>::fetchAndAddAcquire(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    valueToAdd *= QAtomicAdditiveType<T>::AddScale;
    // implement the test-and-set loop
    T old, ret;
    do {
        old = _q_value;
        _Asm_mov_to_ar((_Asm_app_reg)_AREG_CCV, (unsigned)old, FENCE);
        ret = _Asm_cmpxchg((_Asm_sz)_SZ_W, (_Asm_sem)_SEM_ACQ,
                           &_q_value, old + valueToAdd, (_Asm_ldhint)_LDHINT_NONE);
    } while (ret != old);
    return old;
}

template<> template <typename T> inline
T QBasicAtomicOps<4>::fetchAndAddRelaxed(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    // implement the test-and-set loop
    T old, ret;
    do {
        old = _q_value;
        _Asm_mov_to_ar((_Asm_app_reg)_AREG_CCV, (unsigned)old, FENCE);
        ret = _Asm_cmpxchg((_Asm_sz)_SZ_W, (_Asm_sem)_SEM_REL,
                           &_q_value, old + valueToAdd, (_Asm_ldhint)_LDHINT_NONE);
    } while (ret != old);
    return old;
}

template<> template <typename T> inline
T QBasicAtomicOps<8>::fetchAndAddAcquire(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    valueToAdd *= QAtomicAdditiveType<T>::AddScale;
    // implement the test-and-set loop
    T old, ret;
    do {
        old = _q_value;
        _Asm_mov_to_ar((_Asm_app_reg)_AREG_CCV, (quint64)old, FENCE);
        ret = _Asm_cmpxchg((_Asm_sz)_SZ_D, (_Asm_sem)_SEM_ACQ,
                           &_q_value, old + valueToAdd, (_Asm_ldhint)_LDHINT_NONE);
    } while (ret != old);
    return old;
}

template<> template <typename T> inline
T QBasicAtomicOps<8>::fetchAndAddRelaxed(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    // implement the test-and-set loop
    T old, ret;
    do {
        old = _q_value;
        _Asm_mov_to_ar((_Asm_app_reg)_AREG_CCV, (quint64)old, FENCE);
        ret = _Asm_cmpxchg((_Asm_sz)_SZ_D, (_Asm_sem)_SEM_REL,
                           &_q_value, old + valueToAdd, (_Asm_ldhint)_LDHINT_NONE);
    } while (ret != old);
    return old;
}

#endif

template<int size> template<typename T> inline
bool QBasicAtomicOps<size>::ref(T &_q_value) Q_DECL_NOTHROW
{
    // no fetchadd for 1 or 2 bytes
    return fetchAndAddRelaxed(_q_value, 1) == -1;
}

template<int size> template<typename T> inline
bool QBasicAtomicOps<size>::deref(T &_q_value) Q_DECL_NOTHROW
{
    // no fetchadd for 1 or 2 bytes
    return fetchAndAddRelaxed(_q_value, -1) == 1;
}

template<int size> template <typename T> inline
bool QBasicAtomicOps<size>::testAndSetRelaxed(T &_q_value, T expectedValue, T newValue) Q_DECL_NOTHROW
{
    return testAndSetAcquire(_q_value, expectedValue, newValue);
}

template<int size> template <typename T> inline
bool QBasicAtomicOps<size>::testAndSetOrdered(T &_q_value, T expectedValue, T newValue) Q_DECL_NOTHROW
{
    orderedMemoryFence(_q_value);
    return testAndSetAcquire(_q_value, expectedValue, newValue);
}

template<int size> template <typename T> inline
T QBasicAtomicOps<size>::fetchAndStoreRelaxed(T &_q_value, T newValue) Q_DECL_NOTHROW
{
    return fetchAndStoreAcquire(_q_value, newValue);
}

template<int size> template <typename T> inline
T QBasicAtomicOps<size>::fetchAndStoreRelease(T &_q_value, T newValue) Q_DECL_NOTHROW
{
    orderedMemoryFence(_q_value);
    return fetchAndStoreAcquire(_q_value, newValue);
}

template<int size> template <typename T> inline
T QBasicAtomicOps<size>::fetchAndStoreOrdered(T &_q_value, T newValue) Q_DECL_NOTHROW
{
    return fetchAndStoreRelease(_q_value, newValue);
}

template<int size> template <typename T> inline
T QBasicAtomicOps<size>::fetchAndAddRelaxed(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    return fetchAndAddAcquire(_q_value, valueToAdd);
}

template<int size> template <typename T> inline
T QBasicAtomicOps<size>::fetchAndAddOrdered(T &_q_value, typename QAtomicAdditiveType<T>::AdditiveT valueToAdd) Q_DECL_NOTHROW
{
    orderedMemoryFence(_q_value);
    return fetchAndAddRelease(_q_value, valueToAdd);
}

QT_END_NAMESPACE

#endif // QATOMIC_IA64_H
