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

#ifndef QATOMIC_GENERIC_H
#define QATOMIC_GENERIC_H

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

#define Q_ATOMIC_INT_FETCH_AND_STORE_IS_NOT_NATIVE

inline bool QBasicAtomicInt::isFetchAndStoreNative()
{ return false; }
inline bool QBasicAtomicInt::isFetchAndStoreWaitFree()
{ return false; }

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

#define Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_NOT_NATIVE

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isFetchAndStoreNative()
{ return false; }
template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isFetchAndStoreWaitFree()
{ return false; }

#define Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_NOT_NATIVE

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isFetchAndAddNative()
{ return false; }
template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isFetchAndAddWaitFree()
{ return false; }

Q_CORE_EXPORT bool QBasicAtomicInt_testAndSetOrdered(volatile int *, int, int);
Q_CORE_EXPORT int QBasicAtomicInt_fetchAndStoreOrdered(volatile int *, int);
Q_CORE_EXPORT int QBasicAtomicInt_fetchAndAddOrdered(volatile int *, int);

Q_CORE_EXPORT bool QBasicAtomicPointer_testAndSetOrdered(void * volatile *, void *, void *);
Q_CORE_EXPORT void *QBasicAtomicPointer_fetchAndStoreOrdered(void * volatile *, void *);
Q_CORE_EXPORT void *QBasicAtomicPointer_fetchAndAddOrdered(void * volatile *, qptrdiff);

// Reference counting

inline bool QBasicAtomicInt::ref()
{
    return QBasicAtomicInt_fetchAndAddOrdered(&_q_value, 1) != -1;
}

inline bool QBasicAtomicInt::deref()
{
    return QBasicAtomicInt_fetchAndAddOrdered(&_q_value, -1) != 1;
}

// Test and set for integers

inline bool QBasicAtomicInt::testAndSetOrdered(int expectedValue, int newValue)
{
    return QBasicAtomicInt_testAndSetOrdered(&_q_value, expectedValue, newValue);
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

inline int QBasicAtomicInt::fetchAndStoreOrdered(int newValue)
{
    return QBasicAtomicInt_fetchAndStoreOrdered(&_q_value, newValue);
}

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
    return QBasicAtomicInt_fetchAndAddOrdered(&_q_value, valueToAdd);
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
    union { T * volatile * typed; void * volatile * voidp; } pointer;
    pointer.typed = &_q_value;
    return QBasicAtomicPointer_testAndSetOrdered(pointer.voidp, expectedValue, newValue);
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

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreOrdered(T *newValue)
{
    union { T * volatile * typed; void * volatile * voidp; } pointer;
    union { T *typed; void *voidp; } returnValue;
    pointer.typed = &_q_value;
    returnValue.voidp = QBasicAtomicPointer_fetchAndStoreOrdered(pointer.voidp, newValue);
    return returnValue.typed;
}

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
    union { T * volatile *typed; void * volatile *voidp; } pointer;
    union { T *typed; void *voidp; } returnValue;
    pointer.typed = &_q_value;
    returnValue.voidp = QBasicAtomicPointer_fetchAndAddOrdered(pointer.voidp, valueToAdd * sizeof(T));
    return returnValue.typed;
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

#endif // QATOMIC_GENERIC_H
