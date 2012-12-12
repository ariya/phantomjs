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

#ifndef QATOMIC_ARMV6_H
#define QATOMIC_ARMV6_H

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

#ifndef Q_CC_RVCT

#ifndef Q_DATA_MEMORY_BARRIER
# define Q_DATA_MEMORY_BARRIER asm volatile("":::"memory")
#endif
#ifndef Q_COMPILER_MEMORY_BARRIER
# define Q_COMPILER_MEMORY_BARRIER asm volatile("":::"memory")
#endif

inline bool QBasicAtomicInt::ref()
{
    register int newValue;
    register int result;
    asm volatile("0:\n"
                 "ldrex %[newValue], [%[_q_value]]\n"
                 "add %[newValue], %[newValue], #1\n"
                 "strex %[result], %[newValue], [%[_q_value]]\n"
                 "teq %[result], #0\n"
                 "bne 0b\n"
                 : [newValue] "=&r" (newValue),
                   [result] "=&r" (result),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value)
                 : "cc", "memory");
    return newValue != 0;
}

inline bool QBasicAtomicInt::deref()
{
    register int newValue;
    register int result;
    asm volatile("0:\n"
                 "ldrex %[newValue], [%[_q_value]]\n"
                 "sub %[newValue], %[newValue], #1\n"
                 "strex %[result], %[newValue], [%[_q_value]]\n"
                 "teq %[result], #0\n"
                 "bne 0b\n"
                 : [newValue] "=&r" (newValue),
                   [result] "=&r" (result),
                   "+m" (_q_value)
                 : [_q_value] "r" (&_q_value)
                 : "cc", "memory");
    return newValue != 0;
}

inline bool QBasicAtomicInt::testAndSetRelaxed(int expectedValue, int newValue)
{
    register int result;
    asm volatile("0:\n"
                 "ldrex %[result], [%[_q_value]]\n"
                 "eors %[result], %[result], %[expectedValue]\n"
                 "itt eq\n"
                 "strexeq %[result], %[newValue], [%[_q_value]]\n"
                 "teqeq %[result], #1\n"
                 "beq 0b\n"
                 : [result] "=&r" (result),
                   "+m" (_q_value)
                 : [expectedValue] "r" (expectedValue),
                   [newValue] "r" (newValue),
                   [_q_value] "r" (&_q_value)
                 : "cc");
    return result == 0;
}

inline int QBasicAtomicInt::fetchAndStoreRelaxed(int newValue)
{
    register int originalValue;
    register int result;
    asm volatile("0:\n"
                 "ldrex %[originalValue], [%[_q_value]]\n"
                 "strex %[result], %[newValue], [%[_q_value]]\n"
                 "teq %[result], #0\n"
                 "bne 0b\n"
                 : [originalValue] "=&r" (originalValue),
                   [result] "=&r" (result),
                   "+m" (_q_value)
                 : [newValue] "r" (newValue),
                   [_q_value] "r" (&_q_value)
                 : "cc");
    return originalValue;
}

inline int QBasicAtomicInt::fetchAndAddRelaxed(int valueToAdd)
{
    register int originalValue;
    register int newValue;
    register int result;
    asm volatile("0:\n"
                 "ldrex %[originalValue], [%[_q_value]]\n"
                 "add %[newValue], %[originalValue], %[valueToAdd]\n"
                 "strex %[result], %[newValue], [%[_q_value]]\n"
                 "teq %[result], #0\n"
                 "bne 0b\n"
                 : [originalValue] "=&r" (originalValue),
                   [newValue] "=&r" (newValue),
                   [result] "=&r" (result),
                   "+m" (_q_value)
                 : [valueToAdd] "r" (valueToAdd),
                   [_q_value] "r" (&_q_value)
                 : "cc");
    return originalValue;
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetRelaxed(T *expectedValue, T *newValue)
{
    register T *result;
    asm volatile("0:\n"
                 "ldrex %[result], [%[_q_value]]\n"
                 "eors %[result], %[result], %[expectedValue]\n"
                 "itt eq\n"
                 "strexeq %[result], %[newValue], [%[_q_value]]\n"
                 "teqeq %[result], #1\n"
                 "beq 0b\n"
                 : [result] "=&r" (result),
                   "+m" (_q_value)
                 : [expectedValue] "r" (expectedValue),
                   [newValue] "r" (newValue),
                   [_q_value] "r" (&_q_value)
                 : "cc");
    return result == 0;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreRelaxed(T *newValue)
{
    register T *originalValue;
    register int result;
    asm volatile("0:\n"
                 "ldrex %[originalValue], [%[_q_value]]\n"
                 "strex %[result], %[newValue], [%[_q_value]]\n"
                 "teq %[result], #0\n"
                 "bne 0b\n"
                 : [originalValue] "=&r" (originalValue),
                   [result] "=&r" (result),
                   "+m" (_q_value)
                 : [newValue] "r" (newValue),
                   [_q_value] "r" (&_q_value)
                 : "cc");
    return originalValue;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddRelaxed(qptrdiff valueToAdd)
{
    register T *originalValue;
    register T *newValue;
    register int result;
    asm volatile("0:\n"
                 "ldrex %[originalValue], [%[_q_value]]\n"
                 "add %[newValue], %[originalValue], %[valueToAdd]\n"
                 "strex %[result], %[newValue], [%[_q_value]]\n"
                 "teq %[result], #0\n"
                 "bne 0b\n"
                 : [originalValue] "=&r" (originalValue),
                   [newValue] "=&r" (newValue),
                   [result] "=&r" (result),
                   "+m" (_q_value)
                 : [valueToAdd] "r" (valueToAdd * sizeof(T)),
                   [_q_value] "r" (&_q_value)
                 : "cc");
    return originalValue;
}

#else
// This is Q_CC_RVCT

// RVCT inline assembly documentation:
// http://www.keil.com/support/man/docs/armcc/armcc_chdcffdb.htm
// RVCT embedded assembly documentation:
// http://www.keil.com/support/man/docs/armcc/armcc_chddbeib.htm

#if __TARGET_ARCH_THUMB-0 < 4
// save our pragma state and switch to ARM mode (unless using Thumb2)
# pragma push
# pragma arm
#endif

#ifndef Q_DATA_MEMORY_BARRIER
# define Q_DATA_MEMORY_BARRIER __schedule_barrier()
#endif
#ifndef Q_COMPILER_MEMORY_BARRIER
# define Q_COMPILER_MEMORY_BARRIER __schedule_barrier()
#endif

inline bool QBasicAtomicInt::ref()
{
    register int newValue;
    register int result;
    retry:
    __asm {
        ldrex   newValue, [&_q_value]
        add     newValue, newValue, #1
        strex   result, newValue, [&_q_value]
        teq     result, #0
        bne     retry
    }
    return newValue != 0;
}

inline bool QBasicAtomicInt::deref()
{
    register int newValue;
    register int result;
    retry:
    __asm {
        ldrex   newValue, [&_q_value]
        sub     newValue, newValue, #1
        strex   result, newValue, [&_q_value]
        teq     result, #0
        bne     retry
    }
    return newValue != 0;
}

inline bool QBasicAtomicInt::testAndSetRelaxed(int expectedValue, int newValue)
{
    register int result;
    retry:
    __asm {
        ldrex   result, [&_q_value]
        eors    result, result, expectedValue
        strexeq result, newValue, [&_q_value]
        teqeq   result, #1
        beq     retry
    }
    return result == 0;
}

inline int QBasicAtomicInt::fetchAndStoreRelaxed(int newValue)
{
    register int originalValue;
    register int result;
    retry:
    __asm {
        ldrex   originalValue, [&_q_value]
        strex   result, newValue, [&_q_value]
        teq     result, #0
        bne     retry
    }
    return originalValue;
}

inline int QBasicAtomicInt::fetchAndAddRelaxed(int valueToAdd)
{
    register int originalValue;
    register int newValue;
    register int result;
    retry:
    __asm {
        ldrex   originalValue, [&_q_value]
        add     newValue, originalValue, valueToAdd
        strex   result, newValue, [&_q_value]
        teq     result, #0
        bne     retry
    }
    return originalValue;
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetRelaxed(T *expectedValue, T *newValue)
{
    register T *result;
    retry:
    __asm {
        ldrex   result, [&_q_value]
        eors    result, result, expectedValue
        strexeq result, newValue, [&_q_value]
        teqeq   result, #1
        beq     retry
    }
    return result == 0;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreRelaxed(T *newValue)
{
    register T *originalValue;
    register int result;
    retry:
    __asm {
        ldrex   originalValue, [&_q_value]
        strex   result, newValue, [&_q_value]
        teq     result, #0
        bne     retry
    }
    return originalValue;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddRelaxed(qptrdiff valueToAdd)
{
    register T *originalValue;
    register T *newValue;
    register int result;
    retry:
    __asm {
        ldrex   originalValue, [&_q_value]
        add     newValue, originalValue, valueToAdd * sizeof(T)
        strex   result, newValue, [&_q_value]
        teq     result, #0
        bne     retry
    }
    return originalValue;
}

#if __TARGET_ARCH_THUMB-0 < 4
# pragma pop
#endif

#endif

// common code

inline bool QBasicAtomicInt::testAndSetAcquire(int expectedValue, int newValue)
{
    bool returnValue = testAndSetRelaxed(expectedValue, newValue);
    Q_DATA_MEMORY_BARRIER;
    return returnValue;
}

inline bool QBasicAtomicInt::testAndSetRelease(int expectedValue, int newValue)
{
    Q_DATA_MEMORY_BARRIER;
    return testAndSetRelaxed(expectedValue, newValue);
}

inline bool QBasicAtomicInt::testAndSetOrdered(int expectedValue, int newValue)
{
    Q_DATA_MEMORY_BARRIER;
    bool returnValue = testAndSetRelaxed(expectedValue, newValue);
    Q_COMPILER_MEMORY_BARRIER;
    return returnValue;
}

inline int QBasicAtomicInt::fetchAndStoreAcquire(int newValue)
{
    int returnValue = fetchAndStoreRelaxed(newValue);
    Q_DATA_MEMORY_BARRIER;
    return returnValue;
}

inline int QBasicAtomicInt::fetchAndStoreRelease(int newValue)
{
    Q_DATA_MEMORY_BARRIER;
    return fetchAndStoreRelaxed(newValue);
}

inline int QBasicAtomicInt::fetchAndStoreOrdered(int newValue)
{
    Q_DATA_MEMORY_BARRIER;
    int returnValue = fetchAndStoreRelaxed(newValue);
    Q_COMPILER_MEMORY_BARRIER;
    return returnValue;
}


inline int QBasicAtomicInt::fetchAndAddAcquire(int valueToAdd)
{
    int returnValue = fetchAndAddRelaxed(valueToAdd);
    Q_DATA_MEMORY_BARRIER;
    return returnValue;
}

inline int QBasicAtomicInt::fetchAndAddRelease(int valueToAdd)
{
    Q_DATA_MEMORY_BARRIER;
    return fetchAndAddRelaxed(valueToAdd);
}

inline int QBasicAtomicInt::fetchAndAddOrdered(int valueToAdd)
{
    Q_DATA_MEMORY_BARRIER;
    int returnValue = fetchAndAddRelaxed(valueToAdd);
    Q_COMPILER_MEMORY_BARRIER;
    return returnValue;
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetAcquire(T *expectedValue, T *newValue)
{
    bool returnValue = testAndSetRelaxed(expectedValue, newValue);
    Q_DATA_MEMORY_BARRIER;
    return returnValue;
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetRelease(T *expectedValue, T *newValue)
{
    Q_DATA_MEMORY_BARRIER;
    return testAndSetRelaxed(expectedValue, newValue);
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetOrdered(T *expectedValue, T *newValue)
{
    Q_DATA_MEMORY_BARRIER;
    bool returnValue = testAndSetAcquire(expectedValue, newValue);
    Q_COMPILER_MEMORY_BARRIER;
    return returnValue;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreAcquire(T *newValue)
{
    T *returnValue = fetchAndStoreRelaxed(newValue);
    Q_DATA_MEMORY_BARRIER;
    return returnValue;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreRelease(T *newValue)
{
    Q_DATA_MEMORY_BARRIER;
    return fetchAndStoreRelaxed(newValue);
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreOrdered(T *newValue)
{
    Q_DATA_MEMORY_BARRIER;
    T *returnValue = fetchAndStoreRelaxed(newValue);
    Q_COMPILER_MEMORY_BARRIER;
    return returnValue;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddAcquire(qptrdiff valueToAdd)
{
    T *returnValue = fetchAndAddRelaxed(valueToAdd);
    Q_DATA_MEMORY_BARRIER;
    return returnValue;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddRelease(qptrdiff valueToAdd)
{
    Q_DATA_MEMORY_BARRIER;
    return fetchAndAddRelaxed(valueToAdd);
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddOrdered(qptrdiff valueToAdd)
{
    Q_DATA_MEMORY_BARRIER;
    T *returnValue = fetchAndAddRelaxed(valueToAdd);
    Q_COMPILER_MEMORY_BARRIER;
    return returnValue;
}

#undef Q_DATA_MEMORY_BARRIER
#undef Q_COMPILER_MEMORY_BARRIER

QT_END_NAMESPACE

QT_END_HEADER

#endif // QATOMIC_ARMV6_H
