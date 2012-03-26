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

#ifndef QATOMIC_WINDOWS_H
#define QATOMIC_WINDOWS_H

#ifndef Q_CC_MSVC

// Mingw and other GCC platforms get inline assembly

# ifdef __i386__
#  include "QtCore/qatomic_i386.h"
# else
#  include "QtCore/qatomic_x86_64.h"
# endif

#else // Q_CC_MSVC

////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef Q_OS_WINCE

// use compiler intrinsics for all atomic functions
# define QT_INTERLOCKED_PREFIX _
# define QT_INTERLOCKED_PROTOTYPE __cdecl
# define QT_INTERLOCKED_DECLARE_PROTOTYPES
# define QT_INTERLOCKED_INTRINSIC

#else // Q_OS_WINCE

# if _WIN32_WCE < 0x600 && defined(_X86_)
// For X86 Windows CE, include winbase.h to catch inline functions which
// override the regular definitions inside of coredll.dll.
// Though one could use the original version of Increment/Decrement, others are
// not exported at all.
#  include <winbase.h>

// It's safer to remove the volatile and let the compiler add it as needed.
#  define QT_INTERLOCKED_NO_VOLATILE

# else // _WIN32_WCE >= 0x600 || !_X86_

#  define QT_INTERLOCKED_PROTOTYPE __cdecl
#  define QT_INTERLOCKED_DECLARE_PROTOTYPES

#  if _WIN32_WCE >= 0x600
#   if defined(_X86_)
#    define QT_INTERLOCKED_PREFIX _
#    define QT_INTERLOCKED_INTRINSIC
#   endif
#  else
#   define QT_INTERLOCKED_NO_VOLATILE
#  endif

# endif // _WIN32_WCE >= 0x600 || !_X86_

#endif // Q_OS_WINCE

////////////////////////////////////////////////////////////////////////////////////////////////////
// Prototype declaration

#define QT_INTERLOCKED_CONCAT_I(prefix, suffix) \
    prefix ## suffix
#define QT_INTERLOCKED_CONCAT(prefix, suffix) \
    QT_INTERLOCKED_CONCAT_I(prefix, suffix)

// MSVC intrinsics prefix function names with an underscore. Also, if platform
// SDK headers have been included, the Interlocked names may be defined as
// macros.
// To avoid double underscores, we paste the prefix with Interlocked first and
// then the remainder of the function name.
#define QT_INTERLOCKED_FUNCTION(name) \
    QT_INTERLOCKED_CONCAT( \
        QT_INTERLOCKED_CONCAT(QT_INTERLOCKED_PREFIX, Interlocked), name)

#ifdef QT_INTERLOCKED_NO_VOLATILE
# define QT_INTERLOCKED_VOLATILE
# define QT_INTERLOCKED_REMOVE_VOLATILE(a) qt_interlocked_remove_volatile(a)
#else
# define QT_INTERLOCKED_VOLATILE volatile
# define QT_INTERLOCKED_REMOVE_VOLATILE(a) a
#endif

#ifndef QT_INTERLOCKED_PREFIX
#define QT_INTERLOCKED_PREFIX
#endif

#ifndef QT_INTERLOCKED_PROTOTYPE
#define QT_INTERLOCKED_PROTOTYPE
#endif

#ifdef QT_INTERLOCKED_DECLARE_PROTOTYPES
#undef QT_INTERLOCKED_DECLARE_PROTOTYPES

extern "C" {

    long QT_INTERLOCKED_PROTOTYPE QT_INTERLOCKED_FUNCTION( Increment )(long QT_INTERLOCKED_VOLATILE *);
    long QT_INTERLOCKED_PROTOTYPE QT_INTERLOCKED_FUNCTION( Decrement )(long QT_INTERLOCKED_VOLATILE *);
    long QT_INTERLOCKED_PROTOTYPE QT_INTERLOCKED_FUNCTION( CompareExchange )(long QT_INTERLOCKED_VOLATILE *, long, long);
    long QT_INTERLOCKED_PROTOTYPE QT_INTERLOCKED_FUNCTION( Exchange )(long QT_INTERLOCKED_VOLATILE *, long);
    long QT_INTERLOCKED_PROTOTYPE QT_INTERLOCKED_FUNCTION( ExchangeAdd )(long QT_INTERLOCKED_VOLATILE *, long);

# if !defined(Q_OS_WINCE) && !defined(__i386__) && !defined(_M_IX86)
    void * QT_INTERLOCKED_FUNCTION( CompareExchangePointer )(void * QT_INTERLOCKED_VOLATILE *, void *, void *);
    void * QT_INTERLOCKED_FUNCTION( ExchangePointer )(void * QT_INTERLOCKED_VOLATILE *, void *);
    __int64 QT_INTERLOCKED_FUNCTION( ExchangeAdd64 )(__int64 QT_INTERLOCKED_VOLATILE *, __int64);
# endif

}

#endif // QT_INTERLOCKED_DECLARE_PROTOTYPES

#undef QT_INTERLOCKED_PROTOTYPE

////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef QT_INTERLOCKED_INTRINSIC
#undef QT_INTERLOCKED_INTRINSIC

# pragma intrinsic (_InterlockedIncrement)
# pragma intrinsic (_InterlockedDecrement)
# pragma intrinsic (_InterlockedExchange)
# pragma intrinsic (_InterlockedCompareExchange)
# pragma intrinsic (_InterlockedExchangeAdd)

# if !defined(Q_OS_WINCE) && !defined(_M_IX86)
#  pragma intrinsic (_InterlockedCompareExchangePointer)
#  pragma intrinsic (_InterlockedExchangePointer)
#  pragma intrinsic (_InterlockedExchangeAdd64)
# endif

#endif // QT_INTERLOCKED_INTRINSIC

////////////////////////////////////////////////////////////////////////////////////////////////////
// Interlocked* replacement macros

#define QT_INTERLOCKED_INCREMENT(value) \
    QT_INTERLOCKED_FUNCTION( Increment )( \
            QT_INTERLOCKED_REMOVE_VOLATILE( value ) )

#define QT_INTERLOCKED_DECREMENT(value) \
    QT_INTERLOCKED_FUNCTION( Decrement )( \
            QT_INTERLOCKED_REMOVE_VOLATILE( value ) )

#define QT_INTERLOCKED_COMPARE_EXCHANGE(value, newValue, expectedValue) \
    QT_INTERLOCKED_FUNCTION( CompareExchange )( \
            QT_INTERLOCKED_REMOVE_VOLATILE( value ), \
            newValue, \
            expectedValue )

#define QT_INTERLOCKED_EXCHANGE(value, newValue) \
    QT_INTERLOCKED_FUNCTION( Exchange )( \
            QT_INTERLOCKED_REMOVE_VOLATILE( value ), \
            newValue )

#define QT_INTERLOCKED_EXCHANGE_ADD(value, valueToAdd) \
    QT_INTERLOCKED_FUNCTION( ExchangeAdd )( \
            QT_INTERLOCKED_REMOVE_VOLATILE( value ), \
            valueToAdd )

#if defined(Q_OS_WINCE) || defined(__i386__) || defined(_M_IX86)

# define QT_INTERLOCKED_COMPARE_EXCHANGE_POINTER(value, newValue, expectedValue) \
    reinterpret_cast<void *>( \
        QT_INTERLOCKED_FUNCTION( CompareExchange )( \
                QT_INTERLOCKED_REMOVE_VOLATILE( value ## _integral ), \
                (long)( newValue ), \
                (long)( expectedValue ) ))

# define QT_INTERLOCKED_EXCHANGE_POINTER(value, newValue) \
    QT_INTERLOCKED_FUNCTION( Exchange )( \
            QT_INTERLOCKED_REMOVE_VOLATILE( value ## _integral ), \
            (quintptr)( newValue ) )

# define QT_INTERLOCKED_EXCHANGE_ADD_POINTER(value, valueToAdd) \
    QT_INTERLOCKED_FUNCTION( ExchangeAdd )( \
            QT_INTERLOCKED_REMOVE_VOLATILE( value ## _integral ), \
            valueToAdd )

#else // !defined(Q_OS_WINCE) && !defined(__i386__) && !defined(_M_IX86)

# define QT_INTERLOCKED_COMPARE_EXCHANGE_POINTER(value, newValue, expectedValue) \
    QT_INTERLOCKED_FUNCTION( CompareExchangePointer )( \
            reinterpret_cast<void * QT_INTERLOCKED_VOLATILE *>( QT_INTERLOCKED_REMOVE_VOLATILE( value ) ), \
            newValue, \
            expectedValue )

# define QT_INTERLOCKED_EXCHANGE_POINTER(value, newValue) \
    QT_INTERLOCKED_FUNCTION( ExchangePointer )( \
            reinterpret_cast<void * QT_INTERLOCKED_VOLATILE *>( QT_INTERLOCKED_REMOVE_VOLATILE( value ) ), \
            newValue )

# define QT_INTERLOCKED_EXCHANGE_ADD_POINTER(value, valueToAdd) \
    QT_INTERLOCKED_FUNCTION( ExchangeAdd64 )( \
            QT_INTERLOCKED_REMOVE_VOLATILE( value ## _integral ), \
            valueToAdd )

#endif // !defined(Q_OS_WINCE) && !defined(__i386__) && !defined(_M_IX86)

////////////////////////////////////////////////////////////////////////////////////////////////////

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

#define Q_ATOMIC_INT_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT_REFERENCE_COUNTING_IS_WAIT_FREE

inline bool QBasicAtomicInt::isReferenceCountingNative()
{ return true; }
inline bool QBasicAtomicInt::isReferenceCountingWaitFree()
{ return true; }

#define Q_ATOMIC_INT_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT_TEST_AND_SET_IS_WAIT_FREE

inline bool QBasicAtomicInt::isTestAndSetNative()
{ return true; }
inline bool QBasicAtomicInt::isTestAndSetWaitFree()
{ return true; }

#define Q_ATOMIC_INT_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT_FETCH_AND_STORE_IS_WAIT_FREE

inline bool QBasicAtomicInt::isFetchAndStoreNative()
{ return true; }
inline bool QBasicAtomicInt::isFetchAndStoreWaitFree()
{ return true; }

#define Q_ATOMIC_INT_FETCH_AND_ADD_IS_ALWAYS_NATIVE
#define Q_ATOMIC_INT_FETCH_AND_ADD_IS_WAIT_FREE

inline bool QBasicAtomicInt::isFetchAndAddNative()
{ return true; }
inline bool QBasicAtomicInt::isFetchAndAddWaitFree()
{ return true; }

#define Q_ATOMIC_POINTER_TEST_AND_SET_IS_ALWAYS_NATIVE
#define Q_ATOMIC_POINTER_TEST_AND_SET_IS_WAIT_FREE

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isTestAndSetNative()
{ return true; }
template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isTestAndSetWaitFree()
{ return true; }

#define Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_ALWAYS_NATIVE
#define Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_WAIT_FREE

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isFetchAndStoreNative()
{ return true; }
template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isFetchAndStoreWaitFree()
{ return true; }

#define Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_ALWAYS_NATIVE
#define Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_WAIT_FREE
template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isFetchAndAddNative()
{ return true; }
template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::isFetchAndAddWaitFree()
{ return true; }

////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef QT_INTERLOCKED_NO_VOLATILE
template <class T>
Q_INLINE_TEMPLATE T *qt_interlocked_remove_volatile(T volatile *t)
{
    return const_cast<T *>(t);
}
#endif // !QT_INTERLOCKED_NO_VOLATILE

////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool QBasicAtomicInt::ref()
{
    return QT_INTERLOCKED_INCREMENT(&_q_value) != 0;
}

inline bool QBasicAtomicInt::deref()
{
    return QT_INTERLOCKED_DECREMENT(&_q_value) != 0;
}

inline bool QBasicAtomicInt::testAndSetOrdered(int expectedValue, int newValue)
{
    return QT_INTERLOCKED_COMPARE_EXCHANGE(&_q_value, newValue, expectedValue)
            == expectedValue;
}

inline int QBasicAtomicInt::fetchAndStoreOrdered(int newValue)
{
    return QT_INTERLOCKED_EXCHANGE(&_q_value, newValue);
}

inline int QBasicAtomicInt::fetchAndAddOrdered(int valueToAdd)
{
    return QT_INTERLOCKED_EXCHANGE_ADD(&_q_value, valueToAdd);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
Q_INLINE_TEMPLATE bool QBasicAtomicPointer<T>::testAndSetOrdered(T *expectedValue, T *newValue)
{
    return QT_INTERLOCKED_COMPARE_EXCHANGE_POINTER(&_q_value, newValue, expectedValue)
            == expectedValue;
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndStoreOrdered(T* newValue)
{
    return reinterpret_cast<T *>(
            QT_INTERLOCKED_EXCHANGE_POINTER(&_q_value, newValue));
}

template <typename T>
Q_INLINE_TEMPLATE T *QBasicAtomicPointer<T>::fetchAndAddOrdered(qptrdiff valueToAdd)
{
    return reinterpret_cast<T *>(
            QT_INTERLOCKED_EXCHANGE_ADD_POINTER(&_q_value, valueToAdd * sizeof(T)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////////////////////////
// Cleanup

#undef QT_INTERLOCKED_CONCAT_I
#undef QT_INTERLOCKED_CONCAT
#undef QT_INTERLOCKED_FUNCTION
#undef QT_INTERLOCKED_PREFIX

#undef QT_INTERLOCKED_NO_VOLATILE
#undef QT_INTERLOCKED_VOLATILE
#undef QT_INTERLOCKED_REMOVE_VOLATILE

#undef QT_INTERLOCKED_INCREMENT
#undef QT_INTERLOCKED_DECREMENT
#undef QT_INTERLOCKED_COMPARE_EXCHANGE
#undef QT_INTERLOCKED_EXCHANGE
#undef QT_INTERLOCKED_EXCHANGE_ADD
#undef QT_INTERLOCKED_COMPARE_EXCHANGE_POINTER
#undef QT_INTERLOCKED_EXCHANGE_POINTER
#undef QT_INTERLOCKED_EXCHANGE_ADD_POINTER

QT_END_NAMESPACE

QT_END_HEADER

#endif // Q_CC_MSVC

#endif // QATOMIC_WINDOWS_H
