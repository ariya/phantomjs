/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QATOMIC_POWERPC_H
#define QATOMIC_POWERPC_H

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

#define Q_ATOMIC_INT_REFERENCE_COUNTING_IS_ALWAYS_NATIVE

inline bool QBasicAtomicInt::isReferenceCountingNative()
{ return true; }
inline bool QBasicAtomicInt::isReferenceCountingWaitFree()
{ return false; }

#define Q_ATOMIC_INT_TEST_AND_SET_IS_ALWAYS_NATIVE

inline bool QBasicAtomicInt::isTestAndSetNative()
{ return true; }
inline bool QBasicAtomicInt::isTestAndSetWaitFree()
{ return false; }

#define Q_ATOMIC_INT_FETCH_AND_STORE_IS_ALWAYS_NATIVE

inline bool QBasicAtomicInt::isFetchAndStoreNative()
{ return true; }
inline bool QBasicAtomicInt::isFetchAndStoreWaitFree()
{ return false; }

#define Q_ATOMIC_INT_FETCH_AND_ADD_IS_ALWAYS_NATIVE

inline bool QBasicAtomicInt::isFetchAndAddNative()
{ return true; }
inline bool QBasicAtomicInt::isFetchAndAddWaitFree()
{ return false; }

#define Q_ATOMIC_POINTER_TEST_AND_SET_IS_ALWAYS_NATIVE

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isTestAndSetNative()
{ return true; }
template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isTestAndSetWaitFree()
{ return false; }

#define Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_ALWAYS_NATIVE

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isFetchAndStoreNative()
{ return true; }
template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isFetchAndStoreWaitFree()
{ return false; }

#define Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_ALWAYS_NATIVE

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isFetchAndAddNative()
{ return true; }
template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isFetchAndAddWaitFree()
{ return false; }

#if defined(Q_CC_GNU)

#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 2) \
    || (!defined(__64BIT__) && !defined(__powerpc64__) && !defined(__ppc64__))
#  define _Q_VALUE "0, %[_q_value]"
#  define _Q_VALUE_MEMORY_OPERAND "+m" (_q_value)
#  define _Q_VALUE_REGISTER_OPERAND [_q_value] "r" (&_q_value),
#else
// On 64-bit with gcc >= 4.2
#  define _Q_VALUE "%y[_q_value]"
#  define _Q_VALUE_MEMORY_OPERAND [_q_value] "+Z" (_q_value)
#  define _Q_VALUE_REGISTER_OPERAND
#endif

inline bool QBasicAtomicInt::ref()
{
    register int originalValue;
    register int newValue;
    asm volatile("lwarx  %[originalValue]," _Q_VALUE "\n"
                 "addi   %[newValue], %[originalValue], %[one]\n"
                 "stwcx. %[newValue]," _Q_VALUE "\n"
                 "bne-   $-12\n"
                 : [originalValue] "=&b" (originalValue),
                   [newValue] "=&r" (newValue),
                   _Q_VALUE_MEMORY_OPERAND
                 : _Q_VALUE_REGISTER_OPERAND
                   [one] "i" (1)
                 : "cc", "memory");
    return newValue != 0;
}

inline bool QBasicAtomicInt::deref()
{
    register int originalValue;
    register int newValue;
    asm volatile("lwarx  %[originalValue]," _Q_VALUE "\n"
                 "addi   %[newValue], %[originalValue], %[minusOne]\n"
                 "stwcx. %[newValue]," _Q_VALUE "\n"
                 "bne-   $-12\n"
                 : [originalValue] "=&b" (originalValue),
                   [newValue] "=&r" (newValue),
                   _Q_VALUE_MEMORY_OPERAND
                 : _Q_VALUE_REGISTER_OPERAND
                   [minusOne] "i" (-1)
                 : "cc", "memory");
    return newValue != 0;
}

inline bool QBasicAtomicInt::testAndSetRelaxed(int expectedValue, int newValue)
{
    register int result;
    asm volatile("lwarx  %[result]," _Q_VALUE "\n"
                 "xor.   %[result], %[result], %[expectedValue]\n"
                 "bne    $+12\n"
                 "stwcx. %[newValue]," _Q_VALUE "\n"
                 "bne-   $-16\n"
                 : [result] "=&r" (result),
                   _Q_VALUE_MEMORY_OPERAND
                 : _Q_VALUE_REGISTER_OPERAND
                   [expectedValue] "r" (expectedValue),
                   [newValue] "r" (newValue)
                 : "cc", "memory");
    return result == 0;
}

inline bool QBasicAtomicInt::testAndSetAcquire(int expectedValue, int newValue)
{
    register int result;
    asm volatile("lwarx  %[result]," _Q_VALUE "\n"
                 "xor.   %[result], %[result], %[expectedValue]\n"
                 "bne    $+16\n"
                 "stwcx. %[newValue]," _Q_VALUE "\n"
                 "bne-   $-16\n"
                 "isync\n"
                 : [result] "=&r" (result),
                   _Q_VALUE_MEMORY_OPERAND
                 : _Q_VALUE_REGISTER_OPERAND
                   [expectedValue] "r" (expectedValue),
                   [newValue] "r" (newValue)
                 : "cc", "memory");
    return result == 0;
}

inline bool QBasicAtomicInt::testAndSetRelease(int expectedValue, int newValue)
{
    register int result;
    asm volatile("eieio\n"
                 "lwarx  %[result]," _Q_VALUE "\n"
                 "xor.   %[result], %[result], %[expectedValue]\n"
                 "bne    $+12\n"
                 "stwcx. %[newValue]," _Q_VALUE "\n"
                 "bne-   $-16\n"
                 : [result] "=&r" (result),
                   _Q_VALUE_MEMORY_OPERAND
                 : _Q_VALUE_REGISTER_OPERAND
                   [expectedValue] "r" (expectedValue),
                   [newValue] "r" (newValue)
                 : "cc", "memory");
    return result == 0;
}

inline int QBasicAtomicInt::fetchAndStoreRelaxed(int newValue)
{
    register int originalValue;
    asm volatile("lwarx  %[originalValue]," _Q_VALUE "\n"
                 "stwcx. %[newValue]," _Q_VALUE "\n"
                 "bne-   $-8\n"
                 : [originalValue] "=&r" (originalValue),
                   _Q_VALUE_MEMORY_OPERAND
                 : _Q_VALUE_REGISTER_OPERAND
                   [newValue] "r" (newValue)
                 : "cc", "memory");
    return originalValue;
}

inline int QBasicAtomicInt::fetchAndStoreAcquire(int newValue)
{
    register int originalValue;
    asm volatile("lwarx  %[originalValue]," _Q_VALUE "\n"
                 "stwcx. %[newValue]," _Q_VALUE "\n"
                 "bne-   $-8\n"
                 "isync\n"
                 : [originalValue] "=&r" (originalValue),
                   _Q_VALUE_MEMORY_OPERAND
                 : _Q_VALUE_REGISTER_OPERAND
                   [newValue] "r" (newValue)
                 : "cc", "memory");
    return originalValue;
}

inline int QBasicAtomicInt::fetchAndStoreRelease(int newValue)
{
    register int originalValue;
    asm volatile("eieio\n"
                 "lwarx  %[originalValue]," _Q_VALUE "\n"
                 "stwcx. %[newValue]," _Q_VALUE "\n"
                 "bne-   $-8\n"
                 : [originalValue] "=&r" (originalValue),
                   _Q_VALUE_MEMORY_OPERAND
                 : _Q_VALUE_REGISTER_OPERAND
                   [newValue] "r" (newValue)
                 : "cc", "memory");
    return originalValue;
}

inline int QBasicAtomicInt::fetchAndAddRelaxed(int valueToAdd)
{
    register int originalValue;
    register int newValue;
    asm volatile("lwarx  %[originalValue]," _Q_VALUE "\n"
                 "add    %[newValue], %[originalValue], %[valueToAdd]\n"
                 "stwcx. %[newValue]," _Q_VALUE "\n"
                 "bne-   $-12\n"
                 : [originalValue] "=&r" (originalValue),
                   [newValue] "=&r" (newValue),
                   _Q_VALUE_MEMORY_OPERAND
                 : _Q_VALUE_REGISTER_OPERAND
                   [valueToAdd] "r" (valueToAdd)
                 : "cc", "memory");
    return originalValue;
}

inline int QBasicAtomicInt::fetchAndAddAcquire(int valueToAdd)
{
    register int originalValue;
    register int newValue;
    asm volatile("lwarx  %[originalValue]," _Q_VALUE "\n"
                 "add    %[newValue], %[originalValue], %[valueToAdd]\n"
                 "stwcx. %[newValue]," _Q_VALUE "\n"
                 "bne-   $-12\n"
                 "isync\n"
                 : [originalValue] "=&r" (originalValue),
                   [newValue] "=&r" (newValue),
                   _Q_VALUE_MEMORY_OPERAND
                 : _Q_VALUE_REGISTER_OPERAND
                   [valueToAdd] "r" (valueToAdd)
                 : "cc", "memory");
    return originalValue;
}

inline int QBasicAtomicInt::fetchAndAddRelease(int valueToAdd)
{
    register int originalValue;
    register int newValue;
    asm volatile("eieio\n"
                 "lwarx  %[originalValue]," _Q_VALUE "\n"
                 "add    %[newValue], %[originalValue], %[valueToAdd]\n"
                 "stwcx. %[newValue]," _Q_VALUE "\n"
                 "bne-   $-12\n"
                 : [originalValue] "=&r" (originalValue),
                   [newValue] "=&r" (newValue),
                   _Q_VALUE_MEMORY_OPERAND
                 : _Q_VALUE_REGISTER_OPERAND
                   [valueToAdd] "r" (valueToAdd)
                 : "cc", "memory");
    return originalValue;
}

#if defined(__64BIT__) || defined(__powerpc64__) || defined(__ppc64__)
#  define LPARX "ldarx"
#  define STPCX "stdcx."
#else
#  define LPARX "lwarx"
#  define STPCX "stwcx."
#endif

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetRelaxed(T *expectedValue, T *newValue)
{
    register void *result;
    asm volatile(LPARX"  %[result]," _Q_VALUE "\n"
                 "xor.   %[result], %[result], %[expectedValue]\n"
                 "bne    $+12\n"
                 STPCX"  %[newValue]," _Q_VALUE "\n"
                 "bne-   $-16\n"
                 : [result] "=&r" (result),
                   _Q_VALUE_MEMORY_OPERAND
                 : _Q_VALUE_REGISTER_OPERAND
                   [expectedValue] "r" (expectedValue),
                   [newValue] "r" (newValue)
                 : "cc", "memory");
    return result == 0;
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetAcquire(T *expectedValue, T *newValue)
{
    register void *result;
    asm volatile(LPARX"  %[result]," _Q_VALUE "\n"
                 "xor.   %[result], %[result], %[expectedValue]\n"
                 "bne    $+16\n"
                 STPCX"  %[newValue]," _Q_VALUE "\n"
                 "bne-   $-16\n"
                 "isync\n"
                 : [result] "=&r" (result),
                   _Q_VALUE_MEMORY_OPERAND
                 : _Q_VALUE_REGISTER_OPERAND
                   [expectedValue] "r" (expectedValue),
                   [newValue] "r" (newValue)
                 : "cc", "memory");
    return result == 0;
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetRelease(T *expectedValue, T *newValue)
{
    register void *result;
    asm volatile("eieio\n"
                 LPARX"  %[result]," _Q_VALUE "\n"
                 "xor.   %[result], %[result], %[expectedValue]\n"
                 "bne    $+12\n"
                 STPCX"  %[newValue]," _Q_VALUE "\n"
                 "bne-   $-16\n"
                 : [result] "=&r" (result),
                   _Q_VALUE_MEMORY_OPERAND
                 : _Q_VALUE_REGISTER_OPERAND
                   [expectedValue] "r" (expectedValue),
                   [newValue] "r" (newValue)
                 : "cc", "memory");
    return result == 0;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreRelaxed(T *newValue)
{
    register T *originalValue;
    asm volatile(LPARX"  %[originalValue]," _Q_VALUE "\n"
                 STPCX"  %[newValue]," _Q_VALUE "\n"
                 "bne-   $-8\n"
                 : [originalValue] "=&r" (originalValue),
                   _Q_VALUE_MEMORY_OPERAND
                 : _Q_VALUE_REGISTER_OPERAND
                   [newValue] "r" (newValue)
                 : "cc", "memory");
    return originalValue;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreAcquire(T *newValue)
{
    register T *originalValue;
    asm volatile(LPARX"  %[originalValue]," _Q_VALUE "\n"
                 STPCX"  %[newValue]," _Q_VALUE "\n"
                 "bne-   $-8\n"
                 "isync\n"
                 : [originalValue] "=&r" (originalValue),
                   _Q_VALUE_MEMORY_OPERAND
                 : _Q_VALUE_REGISTER_OPERAND
                   [newValue] "r" (newValue)
                 : "cc", "memory");
    return originalValue;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreRelease(T *newValue)
{
    register T *originalValue;
    asm volatile("eieio\n"
                 LPARX"  %[originalValue]," _Q_VALUE "\n"
                 STPCX"  %[newValue]," _Q_VALUE "\n"
                 "bne-   $-8\n"
                 : [originalValue] "=&r" (originalValue),
                   _Q_VALUE_MEMORY_OPERAND
                 : _Q_VALUE_REGISTER_OPERAND
                   [newValue] "r" (newValue)
                 : "cc", "memory");
    return originalValue;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddRelaxed(qptrdiff valueToAdd)
{
    register T *originalValue;
    register T *newValue;
    asm volatile(LPARX"  %[originalValue]," _Q_VALUE "\n"
                 "add    %[newValue], %[originalValue], %[valueToAdd]\n"
                 STPCX"  %[newValue]," _Q_VALUE "\n"
                 "bne-   $-12\n"
                 : [originalValue] "=&r" (originalValue),
                   [newValue] "=&r" (newValue),
                   _Q_VALUE_MEMORY_OPERAND
                 : _Q_VALUE_REGISTER_OPERAND
                   [valueToAdd] "r" (valueToAdd * sizeof(T))
                 : "cc", "memory");
    return originalValue;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddAcquire(qptrdiff valueToAdd)
{
    register T *originalValue;
    register T *newValue;
    asm volatile(LPARX"  %[originalValue]," _Q_VALUE "\n"
                 "add    %[newValue], %[originalValue], %[valueToAdd]\n"
                 STPCX"  %[newValue]," _Q_VALUE "\n"
                 "bne-   $-12\n"
                 "isync\n"
                 : [originalValue] "=&r" (originalValue),
                   [newValue] "=&r" (newValue),
                   _Q_VALUE_MEMORY_OPERAND
                 : _Q_VALUE_REGISTER_OPERAND
                   [valueToAdd] "r" (valueToAdd * sizeof(T))
                 : "cc", "memory");
    return originalValue;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddRelease(qptrdiff valueToAdd)
{
    register T *originalValue;
    register T *newValue;
    asm volatile("eieio\n"
                 LPARX"  %[originalValue]," _Q_VALUE "\n"
                 "add    %[newValue], %[originalValue], %[valueToAdd]\n"
                 STPCX"  %[newValue]," _Q_VALUE "\n"
                 "bne-   $-12\n"
                 : [originalValue] "=&r" (originalValue),
                   [newValue] "=&r" (newValue),
                   _Q_VALUE_MEMORY_OPERAND
                 : _Q_VALUE_REGISTER_OPERAND
                   [valueToAdd] "r" (valueToAdd * sizeof(T))
                 : "cc", "memory");
    return originalValue;
}

#undef LPARX
#undef STPCX
#undef _Q_VALUE
#undef _Q_VALUE_MEMORY_OPERAND
#undef _Q_VALUE_REGISTER_OPERAND

#else

extern "C" {
    int q_atomic_test_and_set_int(volatile int *ptr, int expectedValue, int newValue);
    int q_atomic_test_and_set_acquire_int(volatile int *ptr, int expectedValue, int newValue);
    int q_atomic_test_and_set_release_int(volatile int *ptr, int expectedValue, int newValue);
    int q_atomic_test_and_set_ptr(volatile void *ptr, void *expectedValue, void *newValue);
    int q_atomic_test_and_set_acquire_ptr(volatile void *ptr, void *expectedValue, void *newValue);
    int q_atomic_test_and_set_release_ptr(volatile void *ptr, void *expectedValue, void *newValue);
    int q_atomic_increment(volatile int *);
    int q_atomic_decrement(volatile int *);
    int q_atomic_set_int(volatile int *, int);
    int q_atomic_fetch_and_store_acquire_int(volatile int *ptr, int newValue);
    int q_atomic_fetch_and_store_release_int(volatile int *ptr, int newValue);
    void *q_atomic_set_ptr(volatile void *, void *);
    int q_atomic_fetch_and_store_acquire_ptr(volatile void *ptr, void *newValue);
    int q_atomic_fetch_and_store_release_ptr(volatile void *ptr, void *newValue);
    int q_atomic_fetch_and_add_int(volatile int *ptr, int valueToAdd);
    int q_atomic_fetch_and_add_acquire_int(volatile int *ptr, int valueToAdd);
    int q_atomic_fetch_and_add_release_int(volatile int *ptr, int valueToAdd);
    void *q_atomic_fetch_and_add_ptr(volatile void *ptr, qptrdiff valueToAdd);
    void *q_atomic_fetch_and_add_acquire_ptr(volatile void *ptr, qptrdiff valueToAdd);
    void *q_atomic_fetch_and_add_release_ptr(volatile void *ptr, qptrdiff valueToAdd);
} // extern "C"


inline bool QBasicAtomicInt::ref()
{
    return q_atomic_increment(&_q_value) != 0;
}

inline bool QBasicAtomicInt::deref()
{
    return q_atomic_decrement(&_q_value) != 0;
}

inline bool QBasicAtomicInt::testAndSetRelaxed(int expectedValue, int newValue)
{
    return q_atomic_test_and_set_int(&_q_value, expectedValue, newValue) != 0;
}

inline bool QBasicAtomicInt::testAndSetAcquire(int expectedValue, int newValue)
{
    return q_atomic_test_and_set_acquire_int(&_q_value, expectedValue, newValue) != 0;
}

inline bool QBasicAtomicInt::testAndSetRelease(int expectedValue, int newValue)
{
    return q_atomic_test_and_set_release_int(&_q_value, expectedValue, newValue) != 0;
}

inline int QBasicAtomicInt::fetchAndStoreRelaxed(int newValue)
{
    return q_atomic_set_int(&_q_value, newValue);
}

inline int QBasicAtomicInt::fetchAndStoreAcquire(int newValue)
{
    return q_atomic_fetch_and_store_acquire_int(&_q_value, newValue);
}

inline int QBasicAtomicInt::fetchAndStoreRelease(int newValue)
{
    return q_atomic_fetch_and_store_release_int(&_q_value, newValue);
}

inline int QBasicAtomicInt::fetchAndAddRelaxed(int valueToAdd)
{
    return q_atomic_fetch_and_add_int(&_q_value, valueToAdd);
}

inline int QBasicAtomicInt::fetchAndAddAcquire(int valueToAdd)
{
    return q_atomic_fetch_and_add_acquire_int(&_q_value, valueToAdd);
}

inline int QBasicAtomicInt::fetchAndAddRelease(int valueToAdd)
{
    return q_atomic_fetch_and_add_release_int(&_q_value, valueToAdd);
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetRelaxed(T *expectedValue, T *newValue)
{
    return q_atomic_test_and_set_ptr(&_q_value, expectedValue, newValue) != 0;
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetAcquire(T *expectedValue, T *newValue)
{
    return q_atomic_test_and_set_acquire_ptr(&_q_value, expectedValue, newValue) != 0;
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetRelease(T *expectedValue, T *newValue)
{
    return q_atomic_test_and_set_release_ptr(&_q_value, expectedValue, newValue) != 0;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreRelaxed(T *newValue)
{
    return reinterpret_cast<T *>(q_atomic_set_ptr(&_q_value, newValue));
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreAcquire(T *newValue)
{
    return reinterpret_cast<T *>(q_atomic_fetch_and_store_acquire_ptr(&_q_value, newValue));
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreRelease(T *newValue)
{
    return reinterpret_cast<T *>(q_atomic_fetch_and_store_release_ptr(&_q_value, newValue));
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddRelaxed(qptrdiff valueToAdd)
{
    return reinterpret_cast<T *>(q_atomic_fetch_and_add_ptr(&_q_value, valueToAdd * sizeof(T)));
}
template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddAcquire(qptrdiff valueToAdd)
{
    return reinterpret_cast<T *>(q_atomic_fetch_and_add_acquire_ptr(&_q_value, valueToAdd * sizeof(T)));
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddRelease(qptrdiff valueToAdd)
{
    return reinterpret_cast<T *>(q_atomic_fetch_and_add_release_ptr(&_q_value, valueToAdd * sizeof(T)));
}

#endif

inline bool QBasicAtomicInt::testAndSetOrdered(int expectedValue, int newValue)
{
    return testAndSetAcquire(expectedValue, newValue);
}

inline int QBasicAtomicInt::fetchAndStoreOrdered(int newValue)
{
    return fetchAndStoreAcquire(newValue);
}

inline int QBasicAtomicInt::fetchAndAddOrdered(int valueToAdd)
{
    return fetchAndAddAcquire(valueToAdd);
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetOrdered(T *expectedValue, T *newValue)
{
    return testAndSetAcquire(expectedValue, newValue);
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreOrdered(T *newValue)
{
    return fetchAndStoreAcquire(newValue);
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddOrdered(qptrdiff valueToAdd)
{
    return fetchAndAddAcquire(valueToAdd);
}

QT_END_NAMESPACE

QT_END_HEADER

#endif // QATOMIC_POWERPC_H
