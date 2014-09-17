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

#ifndef QATOMIC_SYMBIAN_H
#define QATOMIC_SYMBIAN_H

#include <QtCore/qglobal.h>
#include <e32std.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#define Q_ATOMIC_INT_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE

inline bool QBasicAtomicInt::isReferenceCountingWaitFree()
{ return false; }

#define Q_ATOMIC_INT_TEST_AND_SET_IS_SOMETIMES_NATIVE

inline bool QBasicAtomicInt::isTestAndSetWaitFree()
{ return false; }

#define Q_ATOMIC_INT_FETCH_AND_STORE_IS_SOMETIMES_NATIVE

inline bool QBasicAtomicInt::isFetchAndStoreWaitFree()
{ return false; }

#define Q_ATOMIC_INT_FETCH_AND_ADD_IS_SOMETIMES_NATIVE

inline bool QBasicAtomicInt::isFetchAndAddWaitFree()
{ return false; }

#define Q_ATOMIC_POINTER_TEST_AND_SET_IS_SOMETIMES_NATIVE

Q_CORE_EXPORT bool QBasicAtomicPointer_isTestAndSetNative();
template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isTestAndSetNative()
{ return QBasicAtomicPointer_isTestAndSetNative(); }
template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isTestAndSetWaitFree()
{ return false; }

#define Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_SOMETIMES_NATIVE

Q_CORE_EXPORT bool QBasicAtomicPointer_isFetchAndStoreNative();
template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isFetchAndStoreNative()
{ return QBasicAtomicPointer_isFetchAndStoreNative(); }
template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isFetchAndStoreWaitFree()
{ return false; }

#define Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_SOMETIMES_NATIVE

Q_CORE_EXPORT bool QBasicAtomicPointer_isFetchAndAddNative();
template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isFetchAndAddNative()
{ return QBasicAtomicPointer_isFetchAndAddNative(); }
template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isFetchAndAddWaitFree()
{ return false; }

Q_CORE_EXPORT bool QBasicAtomicInt_testAndSetOrdered(volatile int *, int, int);
Q_CORE_EXPORT int QBasicAtomicInt_fetchAndStoreOrdered(volatile int *, int);
Q_CORE_EXPORT int QBasicAtomicInt_fetchAndAddOrdered(volatile int *, int);
Q_CORE_EXPORT bool QBasicAtomicInt_testAndSetRelaxed(volatile int *, int, int);
Q_CORE_EXPORT int QBasicAtomicInt_fetchAndStoreRelaxed(volatile int *, int);
Q_CORE_EXPORT int QBasicAtomicInt_fetchAndAddRelaxed(volatile int *, int);
Q_CORE_EXPORT bool QBasicAtomicInt_testAndSetAcquire(volatile int *, int, int);
Q_CORE_EXPORT int QBasicAtomicInt_fetchAndStoreAcquire(volatile int *, int);
Q_CORE_EXPORT int QBasicAtomicInt_fetchAndAddAcquire(volatile int *, int);
Q_CORE_EXPORT bool QBasicAtomicInt_testAndSetRelease(volatile int *, int, int);
Q_CORE_EXPORT int QBasicAtomicInt_fetchAndStoreRelease(volatile int *, int);
Q_CORE_EXPORT int QBasicAtomicInt_fetchAndAddRelease(volatile int *, int);

Q_CORE_EXPORT bool QBasicAtomicPointer_testAndSetOrdered(void * volatile *, void *, void *);
Q_CORE_EXPORT void *QBasicAtomicPointer_fetchAndStoreOrdered(void * volatile *, void *);
Q_CORE_EXPORT void *QBasicAtomicPointer_fetchAndAddOrdered(void * volatile *, qptrdiff);
Q_CORE_EXPORT bool QBasicAtomicPointer_testAndSetRelaxed(void * volatile *, void *, void *);
Q_CORE_EXPORT void *QBasicAtomicPointer_fetchAndStoreRelaxed(void * volatile *, void *);
Q_CORE_EXPORT void *QBasicAtomicPointer_fetchAndAddRelaxed(void * volatile *, qptrdiff);
Q_CORE_EXPORT bool QBasicAtomicPointer_testAndSetAcquire(void * volatile *, void *, void *);
Q_CORE_EXPORT void *QBasicAtomicPointer_fetchAndStoreAcquire(void * volatile *, void *);
Q_CORE_EXPORT void *QBasicAtomicPointer_fetchAndAddAcquire(void * volatile *, qptrdiff);
Q_CORE_EXPORT bool QBasicAtomicPointer_testAndSetRelease(void * volatile *, void *, void *);
Q_CORE_EXPORT void *QBasicAtomicPointer_fetchAndStoreRelease(void * volatile *, void *);
Q_CORE_EXPORT void *QBasicAtomicPointer_fetchAndAddRelease(void * volatile *, qptrdiff);

// Reference counting

//LockedInc and LockedDec are machine coded for ARMv6 (and future proof)
inline bool QBasicAtomicInt::ref()
{
    int original = User::LockedInc((TInt&)_q_value);
    return original != -1;
}

inline bool QBasicAtomicInt::deref()
{
    int original = User::LockedDec((TInt&)_q_value);
    return original != 1;
}

// Test and set for integers

inline bool QBasicAtomicInt::testAndSetOrdered(int expectedValue, int newValue)
{
    return QBasicAtomicInt_testAndSetOrdered(&_q_value, expectedValue, newValue);
}

inline bool QBasicAtomicInt::testAndSetRelaxed(int expectedValue, int newValue)
{
    return QBasicAtomicInt_testAndSetRelaxed(&_q_value, expectedValue, newValue);
}

inline bool QBasicAtomicInt::testAndSetAcquire(int expectedValue, int newValue)
{
    return QBasicAtomicInt_testAndSetAcquire(&_q_value, expectedValue, newValue);
}

inline bool QBasicAtomicInt::testAndSetRelease(int expectedValue, int newValue)
{
    return QBasicAtomicInt_testAndSetRelease(&_q_value, expectedValue, newValue);
}

// Fetch and store for integers

inline int QBasicAtomicInt::fetchAndStoreOrdered(int newValue)
{
    return QBasicAtomicInt_fetchAndStoreOrdered(&_q_value, newValue);
}

inline int QBasicAtomicInt::fetchAndStoreRelaxed(int newValue)
{
    return QBasicAtomicInt_fetchAndStoreRelaxed(&_q_value, newValue);
}

inline int QBasicAtomicInt::fetchAndStoreAcquire(int newValue)
{
    return QBasicAtomicInt_fetchAndStoreAcquire(&_q_value, newValue);
}

inline int QBasicAtomicInt::fetchAndStoreRelease(int newValue)
{
    return QBasicAtomicInt_fetchAndStoreRelease(&_q_value, newValue);
}

// Fetch and add for integers

inline int QBasicAtomicInt::fetchAndAddOrdered(int valueToAdd)
{
    return QBasicAtomicInt_fetchAndAddOrdered(&_q_value, valueToAdd);
}

inline int QBasicAtomicInt::fetchAndAddRelaxed(int valueToAdd)
{
    return QBasicAtomicInt_fetchAndAddRelaxed(&_q_value, valueToAdd);
}

inline int QBasicAtomicInt::fetchAndAddAcquire(int valueToAdd)
{
    return QBasicAtomicInt_fetchAndAddAcquire(&_q_value, valueToAdd);
}

inline int QBasicAtomicInt::fetchAndAddRelease(int valueToAdd)
{
    return QBasicAtomicInt_fetchAndAddRelease(&_q_value, valueToAdd);
}

// Test and set for pointers

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetOrdered(T *expectedValue, T *newValue)
{
    return QBasicAtomicPointer_testAndSetOrdered(reinterpret_cast<void * volatile *>(&_q_value),
        expectedValue, newValue);
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetRelaxed(T *expectedValue, T *newValue)
{
    return QBasicAtomicPointer_testAndSetRelaxed(reinterpret_cast<void * volatile *>(&_q_value),
        expectedValue, newValue);
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetAcquire(T *expectedValue, T *newValue)
{
    return QBasicAtomicPointer_testAndSetAcquire(reinterpret_cast<void * volatile *>(&_q_value),
        expectedValue, newValue);
}

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetRelease(T *expectedValue, T *newValue)
{
    return QBasicAtomicPointer_testAndSetRelease(reinterpret_cast<void * volatile *>(&_q_value),
        expectedValue, newValue);
}

// Fetch and store for pointers

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreOrdered(T *newValue)
{
    return static_cast<T*>(QBasicAtomicPointer_fetchAndStoreOrdered(
        reinterpret_cast<void * volatile *>(&_q_value)
        , newValue));
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreRelaxed(T *newValue)
{
    return static_cast<T*>(QBasicAtomicPointer_fetchAndStoreRelaxed(
        reinterpret_cast<void * volatile *>(&_q_value)
        , newValue));
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreAcquire(T *newValue)
{
    return static_cast<T*>(QBasicAtomicPointer_fetchAndStoreAcquire(
        reinterpret_cast<void * volatile *>(&_q_value)
        , newValue));
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreRelease(T *newValue)
{
    return static_cast<T*>(QBasicAtomicPointer_fetchAndStoreRelease(
        reinterpret_cast<void * volatile *>(&_q_value)
        , newValue));
}

// Fetch and add for pointers

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddOrdered(qptrdiff valueToAdd)
{
    return static_cast<T*>(QBasicAtomicPointer_fetchAndAddOrdered(
        reinterpret_cast<void * volatile *>(&_q_value),
        valueToAdd * sizeof(T)));
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddRelaxed(qptrdiff valueToAdd)
{
    return static_cast<T*>(QBasicAtomicPointer_fetchAndAddRelaxed(
        reinterpret_cast<void * volatile *>(&_q_value),
        valueToAdd * sizeof(T)));
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddAcquire(qptrdiff valueToAdd)
{
    return static_cast<T*>(QBasicAtomicPointer_fetchAndAddAcquire(
        reinterpret_cast<void * volatile *>(&_q_value),
        valueToAdd * sizeof(T)));
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddRelease(qptrdiff valueToAdd)
{
    return static_cast<T*>(QBasicAtomicPointer_fetchAndAddRelease(
        reinterpret_cast<void * volatile *>(&_q_value),
        valueToAdd * sizeof(T)));
}

QT_END_NAMESPACE

QT_END_HEADER

#endif // QATOMIC_SYMBIAN_H
