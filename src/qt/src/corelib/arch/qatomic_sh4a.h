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

#ifndef QATOMIC_SH4A_H
#define QATOMIC_SH4A_H

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


QT_END_NAMESPACE

QT_END_HEADER

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

QT_BEGIN_NAMESPACE

#if !defined(Q_CC_GNU)
#  error "SH-4A support has not been added for this compiler"
#else

inline bool QBasicAtomicInt::ref()
{
    register int newValue asm("r0");
    asm volatile("0:\n"
                 "movli.l @%[_q_value], %[newValue]\n"
                 "add #1,%[newValue]\n"
                 "movco.l %[newValue], @%[_q_value]\n"
                 "bf 0b\n"
                 : [newValue] "=&r" (newValue),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value)
                 : "cc", "memory");
    return newValue != 0;
}

inline bool QBasicAtomicInt::deref()
{
    register int newValue asm("r0");
    asm volatile("0:\n"
                 "movli.l @%[_q_value], %[newValue]\n"
                 "add #-1,%[newValue]\n"
                 "movco.l %[newValue], @%[_q_value]\n"
                 "bf 0b\n"
                 : [newValue] "=&r" (newValue),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value)
                 : "cc", "memory");
    return newValue != 0;
}

inline bool QBasicAtomicInt::testAndSetRelaxed(int expectedValue, int newValue)
{
    register int result;
    asm volatile("0:\n"
                 "movli.l @%[_q_value], r0\n"
                 "xor %[expectedValue], r0\n"
                 "cmp/eq #0, r0\n"
                 "bf/s 0f\n"
                 "mov r0, %[result]\n"
                 "mov %[newValue], r0\n"
                 "movco.l r0, @%[_q_value]\n"
                 "bf 0b\n"
                 "0:\n"
                 : [result] "=&r" (result),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value),
                   [expectedValue] "r" (expectedValue),
                   [newValue] "r" (newValue)
                 : "r0", "cc", "memory");
    return result == 0;
}

inline bool QBasicAtomicInt::testAndSetAcquire(int expectedValue, int newValue)
{
    register int result;
    asm volatile("0:\n"
                 "movli.l @%[_q_value], r0\n"
                 "xor %[expectedValue], r0\n"
                 "cmp/eq #0, r0\n"
                 "bf/s 0f\n"
                 "mov r0, %[result]\n"
                 "mov %[newValue], r0\n"
                 "movco.l r0, @%[_q_value]\n"
                 "bf 0b\n"
                 "synco\n"
                 "0:\n"
                 : [result] "=&r" (result),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value),
                   [expectedValue] "r" (expectedValue),
                   [newValue] "r" (newValue)
                 : "r0", "cc", "memory");
    return result == 0;
}

inline bool QBasicAtomicInt::testAndSetRelease(int expectedValue, int newValue)
{
    register int result;
    asm volatile("synco\n"
                 "0:\n"
                 "movli.l @%[_q_value], r0\n"
                 "xor %[expectedValue], r0\n"
                 "cmp/eq #0, r0\n"
                 "bf/s 0f\n"
                 "mov r0, %[result]\n"
                 "mov %[newValue], r0\n"
                 "movco.l r0, @%[_q_value]\n"
                 "bf 0b\n"
                 "0:\n"
                 : [result] "=&r" (result),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value),
                   [expectedValue] "r" (expectedValue),
                   [newValue] "r" (newValue)
                 : "r0", "cc", "memory");
    return result == 0;
}

inline bool QBasicAtomicInt::testAndSetOrdered(int expectedValue, int newValue)
{
    return testAndSetAcquire(expectedValue, newValue);
}

inline int QBasicAtomicInt::fetchAndStoreRelaxed(int newValue)
{
    register int originalValue;
    asm volatile("0:\n"
                 "movli.l @%[_q_value], r0\n"
                 "mov r0, %[originalValue]\n"
                 "mov %[newValue], r0\n"
                 "movco.l r0, @%[_q_value]\n"
                 "bf 0b\n"
                 : [originalValue] "=&r" (originalValue),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value),
                   [newValue] "r" (newValue)
                 : "r0", "cc", "memory");
    return originalValue;
}

inline int QBasicAtomicInt::fetchAndStoreAcquire(int newValue)
{
    register int originalValue;
    asm volatile("0:\n"
                 "movli.l @%[_q_value], r0\n"
                 "mov r0, %[originalValue]\n"
                 "mov %[newValue], r0\n"
                 "movco.l r0, @%[_q_value]\n"
                 "bf 0b\n"
                 "synco\n"
                 : [originalValue] "=&r" (originalValue),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value),
                   [newValue] "r" (newValue)
                 : "r0", "cc", "memory");
    return originalValue;
}

inline int QBasicAtomicInt::fetchAndStoreRelease(int newValue)
{
    register int originalValue;
    asm volatile("synco\n"
                 "0:\n"
                 "movli.l @%[_q_value], r0\n"
                 "mov r0, %[originalValue]\n"
                 "mov %[newValue], r0\n"
                 "movco.l r0, @%[_q_value]\n"
                 "bf 0b\n"
                 : [originalValue] "=&r" (originalValue),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value),
                   [newValue] "r" (newValue)
                 : "r0", "cc", "memory");
    return originalValue;
}

inline int QBasicAtomicInt::fetchAndStoreOrdered(int newValue)
{
    return fetchAndStoreAcquire(newValue);
}

inline int QBasicAtomicInt::fetchAndAddRelaxed(int valueToAdd)
{
    register int originalValue;
    asm volatile("0:\n"
                 "movli.l @%[_q_value], r0\n"
                 "mov r0, %[originalValue]\n"
                 "add %[valueToAdd], r0\n"
                 "movco.l r0, @%[_q_value]\n"
                 "bf 0b\n"
                 : [originalValue] "=&r" (originalValue),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value),
                   [valueToAdd] "r" (valueToAdd)
                 : "r0", "cc", "memory");
    return originalValue;
}

inline int QBasicAtomicInt::fetchAndAddAcquire(int valueToAdd)
{
    register int originalValue;
    asm volatile("0:\n"
                 "movli.l @%[_q_value], r0\n"
                 "mov r0, %[originalValue]\n"
                 "add %[valueToAdd], r0\n"
                 "movco.l r0, @%[_q_value]\n"
                 "bf 0b\n"
                 "synco\n"
                 : [originalValue] "=&r" (originalValue),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value),
                   [valueToAdd] "r" (valueToAdd)
                 : "r0", "cc", "memory");
    return originalValue;
}

inline int QBasicAtomicInt::fetchAndAddRelease(int valueToAdd)
{
    register int originalValue;
    asm volatile("synco\n"
                 "0:\n"
                 "movli.l @%[_q_value], r0\n"
                 "mov r0, %[originalValue]\n"
                 "add %[valueToAdd], r0\n"
                 "movco.l r0, @%[_q_value]\n"
                 "bf 0b\n"
                 : [originalValue] "=&r" (originalValue),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value),
                   [valueToAdd] "r" (valueToAdd)
                 : "r0", "cc", "memory");
    return originalValue;
}

inline int QBasicAtomicInt::fetchAndAddOrdered(int valueToAdd)
{
    return fetchAndAddAcquire(valueToAdd);
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetRelaxed(T *expectedValue, T *newValue)
{
    register T *result;
    asm volatile("0:\n"
                 "movli.l @%[_q_value], r0\n"
                 "xor %[expectedValue], r0\n"
                 "cmp/eq #0, r0\n"
                 "bf/s 0f\n"
                 "mov r0, %[result]\n"
                 "mov %[newValue], r0\n"
                 "movco.l r0, @%[_q_value]\n"
                 "bf 0b\n"
                 "0:\n"
                 : [result] "=&r" (result),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value),
                   [expectedValue] "r" (expectedValue),
                   [newValue] "r" (newValue)
                 : "r0", "cc", "memory");
    return result == 0;
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetAcquire(T *expectedValue, T *newValue)
{
    register T *result;
    asm volatile("0:\n"
                 "movli.l @%[_q_value], r0\n"
                 "xor %[expectedValue], r0\n"
                 "cmp/eq #0, r0\n"
                 "bf/s 0f\n"
                 "mov r0, %[result]\n"
                 "mov %[newValue], r0\n"
                 "movco.l r0, @%[_q_value]\n"
                 "bf 0b\n"
                 "synco\n"
                 "0:\n"
                 : [result] "=&r" (result),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value),
                   [expectedValue] "r" (expectedValue),
                   [newValue] "r" (newValue)
                 : "r0", "cc", "memory");
    return result == 0;
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetRelease(T *expectedValue, T *newValue)
{
    register T *result;
    asm volatile("synco\n"
                 "0:\n"
                 "movli.l @%[_q_value], r0\n"
                 "xor %[expectedValue], r0\n"
                 "cmp/eq #0, r0\n"
                 "bf/s 0f\n"
                 "mov r0, %[result]\n"
                 "mov %[newValue], r0\n"
                 "movco.l r0, @%[_q_value]\n"
                 "bf 0b\n"
                 "0:\n"
                 : [result] "=&r" (result),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value),
                   [expectedValue] "r" (expectedValue),
                   [newValue] "r" (newValue)
                 : "r0", "cc", "memory");
    return result == 0;
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetOrdered(T *expectedValue, T *newValue)
{
    return testAndSetAcquire(expectedValue, newValue);
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreRelaxed(T *newValue)
{
    register T *originalValue;
    asm volatile("0:\n"
                 "movli.l @%[_q_value], r0\n"
                 "mov r0, %[originalValue]\n"
                 "mov %[newValue], r0\n"
                 "movco.l r0, @%[_q_value]\n"
                 "bf 0b\n"
                 : [originalValue] "=&r" (originalValue),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value),
                   [newValue] "r" (newValue)
                 : "r0", "cc", "memory");
    return originalValue;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreAcquire(T *newValue)
{
    register T *originalValue;
    asm volatile("0:\n"
                 "movli.l @%[_q_value], r0\n"
                 "mov r0, %[originalValue]\n"
                 "mov %[newValue], r0\n"
                 "movco.l r0, @%[_q_value]\n"
                 "bf 0b\n"
                 "synco\n"
                 : [originalValue] "=&r" (originalValue),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value),
                   [newValue] "r" (newValue)
                 : "r0", "cc", "memory");
    return originalValue;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreRelease(T *newValue)
{
    register T *originalValue;
    asm volatile("synco\n"
                 "0:\n"
                 "movli.l @%[_q_value], r0\n"
                 "mov r0, %[originalValue]\n"
                 "mov %[newValue], r0\n"
                 "movco.l r0, @%[_q_value]\n"
                 "bf 0b\n"
                 : [originalValue] "=&r" (originalValue),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value),
                   [newValue] "r" (newValue)
                 : "r0", "cc", "memory");
    return originalValue;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreOrdered(T *newValue)
{
    return fetchAndStoreAcquire(newValue);
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddRelaxed(qptrdiff valueToAdd)
{
    register T *originalValue;
    asm volatile("0:\n"
                 "movli.l @%[_q_value], r0\n"
                 "mov r0, %[originalValue]\n"
                 "add %[valueToAdd], r0\n"
                 "movco.l r0, @%[_q_value]\n"
                 "bf 0b\n"
                 : [originalValue] "=&r" (originalValue),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value),
                   [valueToAdd] "r" (valueToAdd * sizeof(T))
                 : "r0", "cc", "memory");
    return originalValue;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddAcquire(qptrdiff valueToAdd)
{
    register T *originalValue;
    asm volatile("0:\n"
                 "movli.l @%[_q_value], r0\n"
                 "mov r0, %[originalValue]\n"
                 "add %[valueToAdd], r0\n"
                 "movco.l r0, @%[_q_value]\n"
                 "bf 0b\n"
                 "synco\n"
                 : [originalValue] "=&r" (originalValue),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value),
                   [valueToAdd] "r" (valueToAdd * sizeof(T))
                 : "r0", "cc", "memory");
    return originalValue;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddRelease(qptrdiff valueToAdd)
{
    register T *originalValue;
    asm volatile("synco\n"
                 "0:\n"
                 "movli.l @%[_q_value], r0\n"
                 "mov r0, %[originalValue]\n"
                 "add %[valueToAdd], r0\n"
                 "movco.l r0, @%[_q_value]\n"
                 "bf 0b\n"
                 : [originalValue] "=&r" (originalValue),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value),
                   [valueToAdd] "r" (valueToAdd * sizeof(T))
                 : "r0", "cc", "memory");
    return originalValue;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddOrdered(qptrdiff valueToAdd)
{
    return fetchAndAddAcquire(valueToAdd);
}

#endif // Q_CC_GNU

#endif // QATOMIC_SH4A_H
