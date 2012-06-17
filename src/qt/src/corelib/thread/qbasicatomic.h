/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QBASICATOMIC_H
#define QBASICATOMIC_H

#include <QtCore/qglobal.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

class Q_CORE_EXPORT QBasicAtomicInt
{
public:
#ifdef QT_ARCH_PARISC
    int _q_lock[4];
#endif
#if defined(QT_ARCH_WINDOWS) || defined(QT_ARCH_WINDOWSCE)
    union { // needed for Q_BASIC_ATOMIC_INITIALIZER
        volatile long _q_value;
    };
#else
    volatile int _q_value;
#endif

    // Non-atomic API
    inline bool operator==(int value) const
    {
        return _q_value == value;
    }

    inline bool operator!=(int value) const
    {
        return _q_value != value;
    }

    inline bool operator!() const
    {
        return _q_value == 0;
    }

    inline operator int() const
    {
        return _q_value;
    }

    inline QBasicAtomicInt &operator=(int value)
    {
#ifdef QT_ARCH_PARISC
        this->_q_lock[0] = this->_q_lock[1] = this->_q_lock[2] = this->_q_lock[3] = -1;
#endif
        _q_value = value;
        return *this;
    }

    // Atomic API, implemented in qatomic_XXX.h

    static bool isReferenceCountingNative();
    static bool isReferenceCountingWaitFree();

    bool ref();
    bool deref();

    static bool isTestAndSetNative();
    static bool isTestAndSetWaitFree();

    bool testAndSetRelaxed(int expectedValue, int newValue);
    bool testAndSetAcquire(int expectedValue, int newValue);
    bool testAndSetRelease(int expectedValue, int newValue);
    bool testAndSetOrdered(int expectedValue, int newValue);

    static bool isFetchAndStoreNative();
    static bool isFetchAndStoreWaitFree();

    int fetchAndStoreRelaxed(int newValue);
    int fetchAndStoreAcquire(int newValue);
    int fetchAndStoreRelease(int newValue);
    int fetchAndStoreOrdered(int newValue);

    static bool isFetchAndAddNative();
    static bool isFetchAndAddWaitFree();

    int fetchAndAddRelaxed(int valueToAdd);
    int fetchAndAddAcquire(int valueToAdd);
    int fetchAndAddRelease(int valueToAdd);
    int fetchAndAddOrdered(int valueToAdd);
};

template <typename T>
class QBasicAtomicPointer
{
public:
#ifdef QT_ARCH_PARISC
    int _q_lock[4];
#endif
#if defined(QT_ARCH_WINDOWS) || defined(QT_ARCH_WINDOWSCE)
    union {
        T * volatile _q_value;
#  if !defined(Q_OS_WINCE) && !defined(__i386__) && !defined(_M_IX86)
        qint64
#  else
        long
#  endif
        volatile _q_value_integral;
    };
#else
    T * volatile _q_value;
#endif

    // Non-atomic API
    inline bool operator==(T *value) const
    {
        return _q_value == value;
    }

    inline bool operator!=(T *value) const
    {
        return !operator==(value);
    }

    inline bool operator!() const
    {
        return operator==(0);
    }

    inline operator T *() const
    {
        return _q_value;
    }

    inline T *operator->() const
    {
        return _q_value;
    }

    inline QBasicAtomicPointer<T> &operator=(T *value)
    {
#ifdef QT_ARCH_PARISC
        this->_q_lock[0] = this->_q_lock[1] = this->_q_lock[2] = this->_q_lock[3] = -1;
#endif
        _q_value = value;
        return *this;
    }

    // Atomic API, implemented in qatomic_XXX.h

    static bool isTestAndSetNative();
    static bool isTestAndSetWaitFree();

    bool testAndSetRelaxed(T *expectedValue, T *newValue);
    bool testAndSetAcquire(T *expectedValue, T *newValue);
    bool testAndSetRelease(T *expectedValue, T *newValue);
    bool testAndSetOrdered(T *expectedValue, T *newValue);

    static bool isFetchAndStoreNative();
    static bool isFetchAndStoreWaitFree();

    T *fetchAndStoreRelaxed(T *newValue);
    T *fetchAndStoreAcquire(T *newValue);
    T *fetchAndStoreRelease(T *newValue);
    T *fetchAndStoreOrdered(T *newValue);

    static bool isFetchAndAddNative();
    static bool isFetchAndAddWaitFree();

    T *fetchAndAddRelaxed(qptrdiff valueToAdd);
    T *fetchAndAddAcquire(qptrdiff valueToAdd);
    T *fetchAndAddRelease(qptrdiff valueToAdd);
    T *fetchAndAddOrdered(qptrdiff valueToAdd);
};

#ifdef QT_ARCH_PARISC
#  define Q_BASIC_ATOMIC_INITIALIZER(a) {{-1,-1,-1,-1},(a)}
#elif defined(QT_ARCH_WINDOWS) || defined(QT_ARCH_WINDOWSCE)
#  define Q_BASIC_ATOMIC_INITIALIZER(a) { {(a)} }
#else
#  define Q_BASIC_ATOMIC_INITIALIZER(a) { (a) }
#endif

QT_END_NAMESPACE
QT_END_HEADER

#if defined(QT_MOC) || defined(QT_BUILD_QMAKE) || defined(QT_RCC) || defined(QT_UIC) || defined(QT_BOOTSTRAPPED)
#  include <QtCore/qatomic_bootstrap.h>
#else
#  include <QtCore/qatomic_arch.h>
#endif

#endif // QBASIC_ATOMIC
