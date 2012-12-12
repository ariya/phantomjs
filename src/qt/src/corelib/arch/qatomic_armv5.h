/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QATOMIC_ARMV5_H
#define QATOMIC_ARMV5_H

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

#define Q_ATOMIC_INT_REFERENCE_COUNTING_IS_NOT_NATIVE

inline bool QBasicAtomicInt::isReferenceCountingNative()
{ return false; }
inline bool QBasicAtomicInt::isReferenceCountingWaitFree()
{ return false; }

#define Q_ATOMIC_INT_TEST_AND_SET_IS_NOT_NATIVE

inline bool QBasicAtomicInt::isTestAndSetNative()
{ return false; }
inline bool QBasicAtomicInt::isTestAndSetWaitFree()
{ return false; }

#define Q_ATOMIC_INT_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT_FETCH_AND_STORE_IS_WAIT_FREE

inline bool QBasicAtomicInt::isFetchAndStoreNative()
{ return true; }
inline bool QBasicAtomicInt::isFetchAndStoreWaitFree()
{ return true; }

#define Q_ATOMIC_INT_FETCH_AND_ADD_IS_NOT_NATIVE

inline bool QBasicAtomicInt::isFetchAndAddNative()
{ return false; }
inline bool QBasicAtomicInt::isFetchAndAddWaitFree()
{ return false; }

#define Q_ATOMIC_POINTER_TEST_AND_SET_IS_NOT_NATIVE

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isTestAndSetNative()
{ return false; }
template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isTestAndSetWaitFree()
{ return false; }

#define Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_WAIT_FREE

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isFetchAndStoreNative()
{ return true; }
template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isFetchAndStoreWaitFree()
{ return true; }

#define Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_NOT_NATIVE

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isFetchAndAddNative()
{ return false; }
template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isFetchAndAddWaitFree()
{ return false; }

#ifndef QT_NO_ARM_EABI

// kernel places a restartable cmpxchg implementation at a fixed address
extern "C" typedef int (qt_atomic_eabi_cmpxchg_int_t)(int oldval, int newval, volatile int *ptr);
extern "C" typedef int (qt_atomic_eabi_cmpxchg_ptr_t)(const void *oldval, const void *newval, volatile void *ptr);
#define qt_atomic_eabi_cmpxchg_int (*reinterpret_cast<qt_atomic_eabi_cmpxchg_int_t *>(0xffff0fc0))
#define qt_atomic_eabi_cmpxchg_ptr (*reinterpret_cast<qt_atomic_eabi_cmpxchg_ptr_t *>(0xffff0fc0)) 

#else

extern Q_CORE_EXPORT char q_atomic_lock;
Q_CORE_EXPORT void qt_atomic_yield(int *);

#ifdef Q_CC_RVCT

Q_CORE_EXPORT __asm char q_atomic_swp(volatile char *ptr, char newval);

#else

inline char q_atomic_swp(volatile char *ptr, char newval)
{
    register char ret;
    asm volatile("swpb %0,%2,[%3]"
                 : "=&r"(ret), "=m" (*ptr)
                 : "r"(newval), "r"(ptr)
                 : "cc", "memory");
    return ret;
}

#endif // Q_CC_RVCT

#endif // QT_NO_ARM_EABI

// Reference counting

inline bool QBasicAtomicInt::ref()
{
#ifndef QT_NO_ARM_EABI
    register int originalValue;
    register int newValue;
    do {
        originalValue = _q_value;
        newValue = originalValue + 1;
    } while (qt_atomic_eabi_cmpxchg_int(originalValue, newValue, &_q_value) != 0);
    return newValue != 0;
#else
    int count = 0;
    while (q_atomic_swp(&q_atomic_lock, ~0) != 0)
        qt_atomic_yield(&count);
    int originalValue = _q_value++;
    q_atomic_swp(&q_atomic_lock, 0);
    return originalValue != -1;
#endif
}

inline bool QBasicAtomicInt::deref()
{
#ifndef QT_NO_ARM_EABI
    register int originalValue;
    register int newValue;
    do {
        originalValue = _q_value;
        newValue = originalValue - 1;
    } while (qt_atomic_eabi_cmpxchg_int(originalValue, newValue, &_q_value) != 0);
    return newValue != 0;
#else
    int count = 0;
    while (q_atomic_swp(&q_atomic_lock, ~0) != 0)
        qt_atomic_yield(&count);
    int originalValue = _q_value--;
    q_atomic_swp(&q_atomic_lock, 0);
    return originalValue != 1;
#endif
}

// Test and set for integers

inline bool QBasicAtomicInt::testAndSetOrdered(int expectedValue, int newValue)
{
#ifndef QT_NO_ARM_EABI
    register int originalValue;
    do {
        originalValue = _q_value;
        if (originalValue != expectedValue)
            return false;
    } while (qt_atomic_eabi_cmpxchg_int(expectedValue, newValue, &_q_value) != 0);
    return true;
#else
    bool returnValue = false;
    int count = 0;
    while (q_atomic_swp(&q_atomic_lock, ~0) != 0)
        qt_atomic_yield(&count);
    if (_q_value == expectedValue) {
	_q_value = newValue;
	returnValue = true;
    }
    q_atomic_swp(&q_atomic_lock, 0);
    return returnValue;
#endif
}

inline bool QBasicAtomicInt::testAndSetRelaxed(int expectedValue, int newValue)
{
    return testAndSetOrdered(expectedValue, newValue);
}

inline bool QBasicAtomicInt::testAndSetAcquire(int expectedValue, int newValue)
{
    return testAndSetOrdered(expectedValue, newValue);
}

inline bool QBasicAtomicInt::testAndSetRelease(int expectedValue, int newValue)
{
    return testAndSetOrdered(expectedValue, newValue);
}

// Fetch and store for integers

#ifndef Q_CC_RVCT

inline int QBasicAtomicInt::fetchAndStoreOrdered(int newValue)
{
    int originalValue;
#ifndef QT_NO_ARM_EABI
    asm volatile("swp %0,%2,[%3]"
                 : "=&r"(originalValue), "=m" (_q_value)
                 : "r"(newValue), "r"(&_q_value)
                 : "cc", "memory");
#else
    while (q_atomic_swp(&q_atomic_lock, ~0) != 0)
        qt_atomic_yield(&count);
    originalValue=_q_value;
    _q_value = newValue;
    q_atomic_swp(&q_atomic_lock, 0);
#endif
    return originalValue;
}

#endif

inline int QBasicAtomicInt::fetchAndStoreRelaxed(int newValue)
{
    return fetchAndStoreOrdered(newValue);
}

inline int QBasicAtomicInt::fetchAndStoreAcquire(int newValue)
{
    return fetchAndStoreOrdered(newValue);
}

inline int QBasicAtomicInt::fetchAndStoreRelease(int newValue)
{
    return fetchAndStoreOrdered(newValue);
}

// Fetch and add for integers

inline int QBasicAtomicInt::fetchAndAddOrdered(int valueToAdd)
{
#ifndef QT_NO_ARM_EABI
    register int originalValue;
    register int newValue;
    do {
        originalValue = _q_value;
        newValue = originalValue + valueToAdd;
    } while (qt_atomic_eabi_cmpxchg_int(originalValue, newValue, &_q_value) != 0);
    return originalValue;
#else
    int count = 0;
    while (q_atomic_swp(&q_atomic_lock, ~0) != 0)
        qt_atomic_yield(&count);
    int originalValue = _q_value;
    _q_value += valueToAdd;
    q_atomic_swp(&q_atomic_lock, 0);
    return originalValue;
#endif
}

inline int QBasicAtomicInt::fetchAndAddRelaxed(int valueToAdd)
{
    return fetchAndAddOrdered(valueToAdd);
}

inline int QBasicAtomicInt::fetchAndAddAcquire(int valueToAdd)
{
    return fetchAndAddOrdered(valueToAdd);
}

inline int QBasicAtomicInt::fetchAndAddRelease(int valueToAdd)
{
    return fetchAndAddOrdered(valueToAdd);
}

// Test and set for pointers

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetOrdered(T *expectedValue, T *newValue)
{
#ifndef QT_NO_ARM_EABI
    register T *originalValue;
    do {
        originalValue = _q_value;
        if (originalValue != expectedValue)
            return false;
    } while (qt_atomic_eabi_cmpxchg_ptr(expectedValue, newValue, &_q_value) != 0);
    return true;
#else
    bool returnValue = false;
    int count = 0;
    while (q_atomic_swp(&q_atomic_lock, ~0) != 0)
        qt_atomic_yield(&count);
    if (_q_value == expectedValue) {
	_q_value = newValue;
	returnValue = true;
    }
    q_atomic_swp(&q_atomic_lock, 0);
    return returnValue;
#endif
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetRelaxed(T *expectedValue, T *newValue)
{
    return testAndSetOrdered(expectedValue, newValue);
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetAcquire(T *expectedValue, T *newValue)
{
    return testAndSetOrdered(expectedValue, newValue);
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetRelease(T *expectedValue, T *newValue)
{
    return testAndSetOrdered(expectedValue, newValue);
}

// Fetch and store for pointers

#ifdef Q_CC_RVCT

template <typename T>
__asm T *QBasicAtomicPointer<T>::fetchAndStoreOrdered(T *newValue)
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

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreOrdered(T *newValue)
{
    T *originalValue;
#ifndef QT_NO_ARM_EABI
    asm volatile("swp %0,%2,[%3]"
                 : "=&r"(originalValue), "=m" (_q_value)
                 : "r"(newValue), "r"(&_q_value)
                 : "cc", "memory");
#else
    while (q_atomic_swp(&q_atomic_lock, ~0) != 0)
        qt_atomic_yield(&count);
    originalValue=_q_value;
    _q_value = newValue;
    q_atomic_swp(&q_atomic_lock, 0);
#endif
    return originalValue;
}

#endif // Q_CC_RVCT

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreRelaxed(T *newValue)
{
    return fetchAndStoreOrdered(newValue);
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreAcquire(T *newValue)
{
    return fetchAndStoreOrdered(newValue);
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreRelease(T *newValue)
{
    return fetchAndStoreOrdered(newValue);
}

// Fetch and add for pointers

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddOrdered(qptrdiff valueToAdd)
{
#ifndef QT_NO_ARM_EABI
    register T *originalValue;
    register T *newValue;
    do {
        originalValue = _q_value;
        newValue = originalValue + valueToAdd;
    } while (qt_atomic_eabi_cmpxchg_ptr(originalValue, newValue, &_q_value) != 0);
    return originalValue;
#else
    int count = 0;
    while (q_atomic_swp(&q_atomic_lock, ~0) != 0)
        qt_atomic_yield(&count);
    T *originalValue = (_q_value);
    _q_value += valueToAdd;
    q_atomic_swp(&q_atomic_lock, 0);
    return originalValue;
#endif
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddRelaxed(qptrdiff valueToAdd)
{
    return fetchAndAddOrdered(valueToAdd);
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddAcquire(qptrdiff valueToAdd)
{
    return fetchAndAddOrdered(valueToAdd);
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddRelease(qptrdiff valueToAdd)
{
    return fetchAndAddOrdered(valueToAdd);
}

QT_END_NAMESPACE

QT_END_HEADER

#endif // QATOMIC_ARMV5_H
