/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qglobal.h>

#ifndef QATOMIC_H
#define QATOMIC_H

#include <QtCore/qbasicatomic.h>

QT_BEGIN_NAMESPACE


#if defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 406) && !defined(Q_CC_INTEL)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wextra"
#endif

// High-level atomic integer operations
template <typename T>
class QAtomicInteger : public QBasicAtomicInteger<T>
{
public:
    // Non-atomic API
#ifdef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
    constexpr QAtomicInteger(T value = 0) Q_DECL_NOTHROW : QBasicAtomicInteger<T>(value) {}
#else
    inline QAtomicInteger(T value = 0) Q_DECL_NOTHROW
    {
        this->_q_value = value;
    }
#endif

    inline QAtomicInteger(const QAtomicInteger &other) Q_DECL_NOTHROW
#ifdef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
        : QBasicAtomicInteger<T>()
#endif
    {
        this->storeRelease(other.loadAcquire());
    }

    inline QAtomicInteger &operator=(const QAtomicInteger &other) Q_DECL_NOTHROW
    {
        this->storeRelease(other.loadAcquire());
        return *this;
    }

#ifdef Q_QDOC
    T load() const;
    T loadAcquire() const;
    void store(T newValue);
    void storeRelease(T newValue);

    operator T() const;
    QAtomicInteger &operator=(T);

    static Q_DECL_CONSTEXPR bool isReferenceCountingNative();
    static Q_DECL_CONSTEXPR bool isReferenceCountingWaitFree();

    bool ref();
    bool deref();

    static Q_DECL_CONSTEXPR bool isTestAndSetNative();
    static Q_DECL_CONSTEXPR bool isTestAndSetWaitFree();

    bool testAndSetRelaxed(T expectedValue, T newValue);
    bool testAndSetAcquire(T expectedValue, T newValue);
    bool testAndSetRelease(T expectedValue, T newValue);
    bool testAndSetOrdered(T expectedValue, T newValue);

    static Q_DECL_CONSTEXPR bool isFetchAndStoreNative();
    static Q_DECL_CONSTEXPR bool isFetchAndStoreWaitFree();

    T fetchAndStoreRelaxed(T newValue);
    T fetchAndStoreAcquire(T newValue);
    T fetchAndStoreRelease(T newValue);
    T fetchAndStoreOrdered(T newValue);

    static Q_DECL_CONSTEXPR bool isFetchAndAddNative();
    static Q_DECL_CONSTEXPR bool isFetchAndAddWaitFree();

    T fetchAndAddRelaxed(T valueToAdd);
    T fetchAndAddAcquire(T valueToAdd);
    T fetchAndAddRelease(T valueToAdd);
    T fetchAndAddOrdered(T valueToAdd);

    T fetchAndSubRelaxed(T valueToSub);
    T fetchAndSubAcquire(T valueToSub);
    T fetchAndSubRelease(T valueToSub);
    T fetchAndSubOrdered(T valueToSub);

    T fetchAndOrRelaxed(T valueToOr);
    T fetchAndOrAcquire(T valueToOr);
    T fetchAndOrRelease(T valueToOr);
    T fetchAndOrOrdered(T valueToOr);

    T fetchAndAndRelaxed(T valueToAnd);
    T fetchAndAndAcquire(T valueToAnd);
    T fetchAndAndRelease(T valueToAnd);
    T fetchAndAndOrdered(T valueToAnd);

    T fetchAndXorRelaxed(T valueToXor);
    T fetchAndXorAcquire(T valueToXor);
    T fetchAndXorRelease(T valueToXor);
    T fetchAndXorOrdered(T valueToXor);

    T operator++();
    T operator++(int);
    T operator--();
    T operator--(int);
    T operator+=(T value);
    T operator-=(T value);
    T operator|=(T value);
    T operator&=(T value);
    T operator^=(T value);
#endif
};

class QAtomicInt : public QAtomicInteger<int>
{
public:
    // Non-atomic API
    // We could use QT_COMPILER_INHERITING_CONSTRUCTORS, but we need only one;
    // the implicit definition for all the others is fine.
#ifdef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
    constexpr
#endif
    QAtomicInt(int value = 0) Q_DECL_NOTHROW : QAtomicInteger<int>(value) {}
};

// High-level atomic pointer operations
template <typename T>
class QAtomicPointer : public QBasicAtomicPointer<T>
{
public:
#ifdef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
    constexpr QAtomicPointer(T *value = 0) Q_DECL_NOTHROW : QBasicAtomicPointer<T>(value) {}
#else
    inline QAtomicPointer(T *value = 0) Q_DECL_NOTHROW
    {
        this->store(value);
    }
#endif
    inline QAtomicPointer(const QAtomicPointer<T> &other) Q_DECL_NOTHROW
    {
        this->storeRelease(other.loadAcquire());
    }

    inline QAtomicPointer<T> &operator=(const QAtomicPointer<T> &other) Q_DECL_NOTHROW
    {
        this->storeRelease(other.loadAcquire());
        return *this;
    }

#ifdef Q_QDOC
    T *load() const;
    T *loadAcquire() const;
    void store(T *newValue);
    void storeRelease(T *newValue);

    static Q_DECL_CONSTEXPR bool isTestAndSetNative();
    static Q_DECL_CONSTEXPR bool isTestAndSetWaitFree();

    bool testAndSetRelaxed(T *expectedValue, T *newValue);
    bool testAndSetAcquire(T *expectedValue, T *newValue);
    bool testAndSetRelease(T *expectedValue, T *newValue);
    bool testAndSetOrdered(T *expectedValue, T *newValue);

    static Q_DECL_CONSTEXPR bool isFetchAndStoreNative();
    static Q_DECL_CONSTEXPR bool isFetchAndStoreWaitFree();

    T *fetchAndStoreRelaxed(T *newValue);
    T *fetchAndStoreAcquire(T *newValue);
    T *fetchAndStoreRelease(T *newValue);
    T *fetchAndStoreOrdered(T *newValue);

    static Q_DECL_CONSTEXPR bool isFetchAndAddNative();
    static Q_DECL_CONSTEXPR bool isFetchAndAddWaitFree();

    T *fetchAndAddRelaxed(qptrdiff valueToAdd);
    T *fetchAndAddAcquire(qptrdiff valueToAdd);
    T *fetchAndAddRelease(qptrdiff valueToAdd);
    T *fetchAndAddOrdered(qptrdiff valueToAdd);
#endif
};

#if defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 406) && !defined(Q_CC_INTEL)
# pragma GCC diagnostic pop
#endif

#ifdef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
#  undef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
#endif

/*!
    This is a helper for the assignment operators of implicitly
    shared classes. Your assignment operator should look like this:

    \snippet code/src.corelib.thread.qatomic.h 0
*/
template <typename T>
inline void qAtomicAssign(T *&d, T *x)
{
    if (d == x)
        return;
    x->ref.ref();
    if (!d->ref.deref())
        delete d;
    d = x;
}

/*!
    This is a helper for the detach method of implicitly shared
    classes. Your private class needs a copy constructor which copies
    the members and sets the refcount to 1. After that, your detach
    function should look like this:

    \snippet code/src.corelib.thread.qatomic.h 1
*/
template <typename T>
inline void qAtomicDetach(T *&d)
{
    if (d->ref.load() == 1)
        return;
    T *x = d;
    d = new T(*d);
    if (!x->ref.deref())
        delete x;
}

QT_END_NAMESPACE
#endif // QATOMIC_H
