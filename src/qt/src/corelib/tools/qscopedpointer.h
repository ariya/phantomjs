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

#ifndef QSCOPEDPOINTER_H
#define QSCOPEDPOINTER_H

#include <QtCore/qglobal.h>

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE
QT_MODULE(Core)

template <typename T>
struct QScopedPointerDeleter
{
    static inline void cleanup(T *pointer)
    {
        // Enforce a complete type.
        // If you get a compile error here, read the section on forward declared
        // classes in the QScopedPointer documentation.
        typedef char IsIncompleteType[ sizeof(T) ? 1 : -1 ];
        (void) sizeof(IsIncompleteType);

        delete pointer;
    }
};

template <typename T>
struct QScopedPointerArrayDeleter
{
    static inline void cleanup(T *pointer)
    {
        // Enforce a complete type.
        // If you get a compile error here, read the section on forward declared
        // classes in the QScopedPointer documentation.
        typedef char IsIncompleteType[ sizeof(T) ? 1 : -1 ];
        (void) sizeof(IsIncompleteType);

        delete [] pointer;
    }
};

struct QScopedPointerPodDeleter
{
    static inline void cleanup(void *pointer) { if (pointer) qFree(pointer); }
};

template <typename T, typename Cleanup = QScopedPointerDeleter<T> >
class QScopedPointer
{
#ifndef Q_CC_NOKIAX86
    typedef T *QScopedPointer:: *RestrictedBool;
#endif
public:
    explicit inline QScopedPointer(T *p = 0) : d(p)
    {
    }

    inline ~QScopedPointer()
    {
        T *oldD = this->d;
        Cleanup::cleanup(oldD);
        this->d = 0;
    }

    inline T &operator*() const
    {
        Q_ASSERT(d);
        return *d;
    }

    inline T *operator->() const
    {
        Q_ASSERT(d);
        return d;
    }

    inline bool operator!() const
    {
        return !d;
    }

#if defined(Q_CC_NOKIAX86) || defined(Q_QDOC)
    inline operator bool() const
    {
        return isNull() ? 0 : &QScopedPointer::d;
    }
#else
    inline operator RestrictedBool() const
    {
        return isNull() ? 0 : &QScopedPointer::d;
    }
#endif

    inline T *data() const
    {
        return d;
    }

    inline bool isNull() const
    {
        return !d;
    }

    inline void reset(T *other = 0)
    {
        if (d == other)
            return;
        T *oldD = d;
        d = other;
        Cleanup::cleanup(oldD);
    }

    inline T *take()
    {
        T *oldD = d;
        d = 0;
        return oldD;
    }

    inline void swap(QScopedPointer<T, Cleanup> &other)
    {
        qSwap(d, other.d);
    }

    typedef T *pointer;

protected:
    T *d;

private:
    Q_DISABLE_COPY(QScopedPointer)
};

template <class T, class Cleanup>
inline bool operator==(const QScopedPointer<T, Cleanup> &lhs, const QScopedPointer<T, Cleanup> &rhs)
{
    return lhs.data() == rhs.data();
}

template <class T, class Cleanup>
inline bool operator!=(const QScopedPointer<T, Cleanup> &lhs, const QScopedPointer<T, Cleanup> &rhs)
{
    return lhs.data() != rhs.data();
}

template <class T, class Cleanup>
Q_INLINE_TEMPLATE void qSwap(QScopedPointer<T, Cleanup> &p1, QScopedPointer<T, Cleanup> &p2)
{ p1.swap(p2); }

#ifndef QT_NO_STL
QT_END_NAMESPACE
namespace std {
    template <class T, class Cleanup>
    Q_INLINE_TEMPLATE void swap(QT_PREPEND_NAMESPACE(QScopedPointer)<T, Cleanup> &p1, QT_PREPEND_NAMESPACE(QScopedPointer)<T, Cleanup> &p2)
    { p1.swap(p2); }
}
QT_BEGIN_NAMESPACE
#endif



namespace QtPrivate {
    template <typename X, typename Y> struct QScopedArrayEnsureSameType;
    template <typename X> struct QScopedArrayEnsureSameType<X,X> { typedef X* Type; };
    template <typename X> struct QScopedArrayEnsureSameType<const X, X> { typedef X* Type; };
}

template <typename T, typename Cleanup = QScopedPointerArrayDeleter<T> >
class QScopedArrayPointer : public QScopedPointer<T, Cleanup>
{
public:
    inline QScopedArrayPointer() : QScopedPointer<T, Cleanup>(0) {}

    template <typename D>
    explicit inline QScopedArrayPointer(D *p, typename QtPrivate::QScopedArrayEnsureSameType<T,D>::Type = 0)
        : QScopedPointer<T, Cleanup>(p)
    {
    }

    inline T &operator[](int i)
    {
        return this->d[i];
    }

    inline const T &operator[](int i) const
    {
        return this->d[i];
    }

private:
    explicit inline QScopedArrayPointer(void *) {
        // Enforce the same type.

        // If you get a compile error here, make sure you declare
        // QScopedArrayPointer with the same template type as you pass to the
        // constructor. See also the QScopedPointer documentation.

        // Storing a scalar array as a pointer to a different type is not
        // allowed and results in undefined behavior.
    }

    Q_DISABLE_COPY(QScopedArrayPointer)
};

QT_END_NAMESPACE
QT_END_HEADER

#endif // QSCOPEDPOINTER_H
