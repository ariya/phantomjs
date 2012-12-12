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

#ifndef QATOMIC_ALPHA_H
#define QATOMIC_ALPHA_H

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

inline bool QBasicAtomicInt::ref()
{
    register int old, tmp;
    asm volatile("1:\n"
                 "ldl_l %0,%2\n"   /* old=*ptr;                               */
                 "addl  %0,1,%1\n" /* tmp=old+1;                              */
                 "stl_c %1,%2\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %1,2f\n"   /* if (tmp == 0) goto 2;                   */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (old), "=&r" (tmp), "+m"(_q_value)
                 :
                 : "memory");
    return old != -1;
}

inline bool QBasicAtomicInt::deref()
{
    register int old, tmp;
    asm volatile("1:\n"
                 "ldl_l %0,%2\n"   /* old=*ptr;                               */
                 "subl  %0,1,%1\n" /* tmp=old-1;                              */
                 "stl_c %1,%2\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %1,2f\n"   /* if (tmp==0) goto 2;                     */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (old), "=&r" (tmp), "+m"(_q_value)
                 :
                 : "memory");
    return old != 1;
}

inline bool QBasicAtomicInt::testAndSetRelaxed(int expectedValue, int newValue)
{
    register int ret;
    asm volatile("1:\n"
                 "ldl_l %0,%1\n"   /* ret=*ptr;                               */
                 "cmpeq %0,%2,%0\n"/* if (ret==expected) ret=0; else ret=1;   */
                 "beq   %0,3f\n"   /* if (ret==0) goto 3;                     */
                 "mov   %3,%0\n"   /* ret=newval;                             */
                 "stl_c %0,%1\n"   /* if ((*ptr=ret)!=ret) ret=0; else ret=1; */
                 "beq   %0,2f\n"   /* if (ret==0) goto 2;                     */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (ret), "+m" (_q_value)
                 : "r" (expectedValue), "r" (newValue)
                 : "memory");
    return ret != 0;
}

inline bool QBasicAtomicInt::testAndSetAcquire(int expectedValue, int newValue)
{
    register int ret;
    asm volatile("1:\n"
                 "ldl_l %0,%1\n"   /* ret=*ptr;                               */
                 "cmpeq %0,%2,%0\n"/* if (ret==expected) ret=0; else ret=1;   */
                 "beq   %0,3f\n"   /* if (ret==0) goto 3;                     */
                 "mov   %3,%0\n"   /* ret=newval;                             */
                 "stl_c %0,%1\n"   /* if ((*ptr=ret)!=ret) ret=0; else ret=1; */
                 "beq   %0,2f\n"   /* if (ret==0) goto 2;                     */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 "mb\n"
                 : "=&r" (ret), "+m" (_q_value)
                 : "r" (expectedValue), "r" (newValue)
                 : "memory");
    return ret != 0;
}

inline bool QBasicAtomicInt::testAndSetRelease(int expectedValue, int newValue)
{
    register int ret;
    asm volatile("mb\n"
                 "1:\n"
                 "ldl_l %0,%1\n"   /* ret=*ptr;                               */
                 "cmpeq %0,%2,%0\n"/* if (ret==expected) ret=0; else ret=1;   */
                 "beq   %0,3f\n"   /* if (ret==0) goto 3;                     */
                 "mov   %3,%0\n"   /* ret=newval;                             */
                 "stl_c %0,%1\n"   /* if ((*ptr=ret)!=ret) ret=0; else ret=1; */
                 "beq   %0,2f\n"   /* if (ret==0) goto 2;                     */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (ret), "+m" (_q_value)
                 : "r" (expectedValue), "r" (newValue)
                 : "memory");
    return ret != 0;
}

inline int QBasicAtomicInt::fetchAndStoreRelaxed(int newValue)
{
    register int old, tmp;
    asm volatile("1:\n"
                 "ldl_l %0,%2\n"   /* old=*ptr;                               */
                 "mov   %3,%1\n"   /* tmp=newval;                             */
                 "stl_c %1,%2\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %1,2f\n"   /* if (tmp==0) goto 2;                     */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (old), "=&r" (tmp), "+m" (_q_value)
                 : "r" (newValue)
                 : "memory");
    return old;
}

inline int QBasicAtomicInt::fetchAndStoreAcquire(int newValue)
{
    register int old, tmp;
    asm volatile("1:\n"
                 "ldl_l %0,%2\n"   /* old=*ptr;                               */
                 "mov   %3,%1\n"   /* tmp=newval;                             */
                 "stl_c %1,%2\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %1,2f\n"   /* if (tmp==0) goto 2;                     */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 "mb\n"
                 : "=&r" (old), "=&r" (tmp), "+m" (_q_value)
                 : "r" (newValue)
                 : "memory");
    return old;
}

inline int QBasicAtomicInt::fetchAndStoreRelease(int newValue)
{
    register int old, tmp;
    asm volatile("mb\n"
                 "1:\n"
                 "ldl_l %0,%2\n"   /* old=*ptr;                               */
                 "mov   %3,%1\n"   /* tmp=newval;                             */
                 "stl_c %1,%2\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %1,2f\n"   /* if (tmp==0) goto 2;                     */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (old), "=&r" (tmp), "+m" (_q_value)
                 : "r" (newValue)
                 : "memory");
    return old;
}

inline int QBasicAtomicInt::fetchAndAddRelaxed(int valueToAdd)
{
    register int old, tmp;
    asm volatile("1:\n"
                 "ldl_l %0,%2\n"   /* old=*ptr;                               */
                 "addl  %0,%3,%1\n"/* tmp=old+value;                          */
                 "stl_c %1,%2\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %1,2f\n"   /* if (tmp == 0) goto 2;                   */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (old), "=&r" (tmp), "+m"(_q_value)
                 : "r" (valueToAdd)
                 : "memory");
    return old;
}

inline int QBasicAtomicInt::fetchAndAddAcquire(int valueToAdd)
{
    register int old, tmp;
    asm volatile("1:\n"
                 "ldl_l %0,%2\n"   /* old=*ptr;                               */
                 "addl  %0,%3,%1\n"/* tmp=old+value;                          */
                 "stl_c %1,%2\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %1,2f\n"   /* if (tmp == 0) goto 2;                   */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 "mb\n"
                 : "=&r" (old), "=&r" (tmp), "+m"(_q_value)
                 : "r" (valueToAdd)
                 : "memory");
    return old;
}

inline int QBasicAtomicInt::fetchAndAddRelease(int valueToAdd)
{
    register int old, tmp;
    asm volatile("mb\n"
                 "1:\n"
                 "ldl_l %0,%2\n"   /* old=*ptr;                               */
                 "addl  %0,%3,%1\n"/* tmp=old+value;                          */
                 "stl_c %1,%2\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %1,2f\n"   /* if (tmp == 0) goto 2;                   */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (old), "=&r" (tmp), "+m"(_q_value)
                 : "r" (valueToAdd)
                 : "memory");
    return old;
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetRelaxed(T *expectedValue, T *newValue)
{
    register void *ret;
    asm volatile("1:\n"
                 "ldq_l %0,%1\n"   /* ret=*ptr;                               */
                 "cmpeq %0,%2,%0\n"/* if (ret==expected) tmp=0; else tmp=1;   */
                 "beq   %0,3f\n"   /* if (tmp==0) goto 3;                     */
                 "mov   %3,%0\n"   /* tmp=newval;                             */
                 "stq_c %0,%1\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %0,2f\n"   /* if (ret==0) goto 2;                     */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (ret), "+m" (_q_value)
                 : "r" (expectedValue), "r" (newValue)
                 : "memory");
    return ret != 0;
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetAcquire(T *expectedValue, T *newValue)
{
    register void *ret;
    asm volatile("1:\n"
                 "ldq_l %0,%1\n"   /* ret=*ptr;                               */
                 "cmpeq %0,%2,%0\n"/* if (ret==expected) tmp=0; else tmp=1;   */
                 "beq   %0,3f\n"   /* if (tmp==0) goto 3;                     */
                 "mov   %3,%0\n"   /* tmp=newval;                             */
                 "stq_c %0,%1\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %0,2f\n"   /* if (ret==0) goto 2;                     */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 "mb\n"
                 : "=&r" (ret), "+m" (_q_value)
                 : "r" (expectedValue), "r" (newValue)
                 : "memory");
    return ret != 0;
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetRelease(T *expectedValue, T *newValue)
{
    register void *ret;
    asm volatile("mb\n"
                 "1:\n"
                 "ldq_l %0,%1\n"   /* ret=*ptr;                               */
                 "cmpeq %0,%2,%0\n"/* if (ret==expected) tmp=0; else tmp=1;   */
                 "beq   %0,3f\n"   /* if (tmp==0) goto 3;                     */
                 "mov   %3,%0\n"   /* tmp=newval;                             */
                 "stq_c %0,%1\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %0,2f\n"   /* if (ret==0) goto 2;                     */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (ret), "+m" (_q_value)
                 : "r" (expectedValue), "r" (newValue)
                 : "memory");
    return ret != 0;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreRelaxed(T *newValue)
{
    register T *old, *tmp;
    asm volatile("1:\n"
                 "ldq_l %0,%2\n"   /* old=*ptr;                               */
                 "mov   %3,%1\n"   /* tmp=newval;                             */
                 "stq_c %1,%2\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %1,2f\n"   /* if (tmp==0) goto 2;                     */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (old), "=&r" (tmp), "+m" (_q_value)
                 : "r" (newValue)
                 : "memory");
    return old;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreAcquire(T *newValue)
{
    register T *old, *tmp;
    asm volatile("1:\n"
                 "ldq_l %0,%2\n"   /* old=*ptr;                               */
                 "mov   %3,%1\n"   /* tmp=newval;                             */
                 "stq_c %1,%2\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %1,2f\n"   /* if (tmp==0) goto 2;                     */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 "mb\n"
                 : "=&r" (old), "=&r" (tmp), "+m" (_q_value)
                 : "r" (newValue)
                 : "memory");
    return old;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreRelease(T *newValue)
{
    register T *old, *tmp;
    asm volatile("mb\n"
                 "1:\n"
                 "ldq_l %0,%2\n"   /* old=*ptr;                               */
                 "mov   %3,%1\n"   /* tmp=newval;                             */
                 "stq_c %1,%2\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %1,2f\n"   /* if (tmp==0) goto 2;                     */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (old), "=&r" (tmp), "+m" (_q_value)
                 : "r" (newValue)
                 : "memory");
    return old;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddRelaxed(qptrdiff valueToAdd)
{
    register T *old, *tmp;
    asm volatile("1:\n"
                 "ldq_l %0,%2\n"   /* old=*ptr;                               */
                 "addq  %0,%3,%1\n"/* tmp=old+value;                          */
                 "stq_c %1,%2\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %1,2f\n"   /* if (tmp == 0) goto 2;                   */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (old), "=&r" (tmp), "+m"(_q_value)
                 : "r" (valueToAdd)
                 : "memory");
    return reinterpret_cast<T *>(old);
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddAcquire(qptrdiff valueToAdd)
{
    register T *old, *tmp;
    asm volatile("1:\n"
                 "ldq_l %0,%2\n"   /* old=*ptr;                               */
                 "addq  %0,%3,%1\n"/* tmp=old+value;                          */
                 "stq_c %1,%2\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %1,2f\n"   /* if (tmp == 0) goto 2;                   */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 "mb\n"
                 : "=&r" (old), "=&r" (tmp), "+m"(_q_value)
                 : "r" (valueToAdd)
                 : "memory");
    return reinterpret_cast<T *>(old);
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddRelease(qptrdiff valueToAdd)
{
    register T *old, *tmp;
    asm volatile("mb\n"
                 "1:\n"
                 "ldq_l %0,%2\n"   /* old=*ptr;                               */
                 "addq  %0,%3,%1\n"/* tmp=old+value;                          */
                 "stq_c %1,%2\n"   /* if ((*ptr=tmp)!=tmp) tmp=0; else tmp=1; */
                 "beq   %1,2f\n"   /* if (tmp == 0) goto 2;                   */
                 "br    3f\n"      /* goto 3;                                 */
                 "2: br 1b\n"      /* goto 1;                                 */
                 "3:\n"
                 : "=&r" (old), "=&r" (tmp), "+m"(_q_value)
                 : "r" (valueToAdd)
                 : "memory");
    return reinterpret_cast<T *>(old);
}

#else // !Q_CC_GNU

extern "C" {
    Q_CORE_EXPORT int q_atomic_test_and_set_int(volatile int *ptr, int expected, int newval);
    Q_CORE_EXPORT int q_atomic_test_and_set_acquire_int(volatile int *ptr, int expected, int newval);
    Q_CORE_EXPORT int q_atomic_test_and_set_release_int(volatile int *ptr, int expected, int newval);
    Q_CORE_EXPORT int q_atomic_test_and_set_ptr(volatile void *ptr, void *expected, void *newval);
    Q_CORE_EXPORT int q_atomic_increment(volatile int *ptr);
    Q_CORE_EXPORT int q_atomic_decrement(volatile int *ptr);
    Q_CORE_EXPORT int q_atomic_set_int(volatile int *ptr, int newval);
    Q_CORE_EXPORT void *q_atomic_set_ptr(volatile void *ptr, void *newval);
    Q_CORE_EXPORT int q_atomic_fetch_and_add_int(volatile int *ptr, int value);
    Q_CORE_EXPORT int q_atomic_fetch_and_add_acquire_int(volatile int *ptr, int value);
    Q_CORE_EXPORT int q_atomic_fetch_and_add_release_int(volatile int *ptr, int value);
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
    return reinterpret_cast<T *>(q_atomic_fetch_and_add_ptr(&_q_value, newValue));
}
template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddAcquire(qptrdiff valueToAdd)
{
    return reinterpret_cast<T *>(q_atomic_fetch_and_add_acquire_ptr(&_q_value, newValue));
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddRelease(qptrdiff valueToAdd)
{
    return reinterpret_cast<T *>(q_atomic_fetch_and_add_release_ptr(&_q_value, newValue));
}

#endif // Q_CC_GNU

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

#endif // QATOMIC_ALPHA_H
