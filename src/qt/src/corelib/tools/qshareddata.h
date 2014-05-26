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

#ifndef QSHAREDDATA_H
#define QSHAREDDATA_H

#include <QtCore/qglobal.h>
#include <QtCore/qatomic.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

template <class T> class QSharedDataPointer;

class Q_CORE_EXPORT QSharedData
{
public:
    mutable QAtomicInt ref;

    inline QSharedData() : ref(0) { }
    inline QSharedData(const QSharedData &) : ref(0) { }

private:
    // using the assignment operator would lead to corruption in the ref-counting
    QSharedData &operator=(const QSharedData &);
};

template <class T> class QSharedDataPointer
{
public:
    typedef T Type;
    typedef T *pointer;

    inline void detach() { if (d && d->ref != 1) detach_helper(); }
    inline T &operator*() { detach(); return *d; }
    inline const T &operator*() const { return *d; }
    inline T *operator->() { detach(); return d; }
    inline const T *operator->() const { return d; }
    inline operator T *() { detach(); return d; }
    inline operator const T *() const { return d; }
    inline T *data() { detach(); return d; }
    inline const T *data() const { return d; }
    inline const T *constData() const { return d; }

    inline bool operator==(const QSharedDataPointer<T> &other) const { return d == other.d; }
    inline bool operator!=(const QSharedDataPointer<T> &other) const { return d != other.d; }

    inline QSharedDataPointer() { d = 0; }
    inline ~QSharedDataPointer() { if (d && !d->ref.deref()) delete d; }

    explicit QSharedDataPointer(T *data);
    inline QSharedDataPointer(const QSharedDataPointer<T> &o) : d(o.d) { if (d) d->ref.ref(); }
    inline QSharedDataPointer<T> & operator=(const QSharedDataPointer<T> &o) {
        if (o.d != d) {
            if (o.d)
                o.d->ref.ref();
            T *old = d;
            d = o.d;
            if (old && !old->ref.deref())
                delete old;
        }
        return *this;
    }
    inline QSharedDataPointer &operator=(T *o) {
        if (o != d) {
            if (o)
                o->ref.ref();
            T *old = d;
            d = o;
            if (old && !old->ref.deref())
                delete old;
        }
        return *this;
    }
#ifdef Q_COMPILER_RVALUE_REFS
    QSharedDataPointer(QSharedDataPointer &&o) : d(o.d) { o.d = 0; }
    inline QSharedDataPointer<T> &operator=(QSharedDataPointer<T> &&other)
    { qSwap(d, other.d); return *this; }
#endif

    inline bool operator!() const { return !d; }

    inline void swap(QSharedDataPointer &other)
    { qSwap(d, other.d); }

protected:
    T *clone();

private:
    void detach_helper();

    T *d;
};

template <class T> class QExplicitlySharedDataPointer
{
public:
    typedef T Type;
    typedef T *pointer;

    inline T &operator*() const { return *d; }
    inline T *operator->() { return d; }
    inline T *operator->() const { return d; }
    inline T *data() const { return d; }
    inline const T *constData() const { return d; }

    inline void detach() { if (d && d->ref != 1) detach_helper(); }

    inline void reset()
    {
        if(d && !d->ref.deref())
            delete d;

        d = 0;
    }

    inline operator bool () const { return d != 0; }

    inline bool operator==(const QExplicitlySharedDataPointer<T> &other) const { return d == other.d; }
    inline bool operator!=(const QExplicitlySharedDataPointer<T> &other) const { return d != other.d; }
    inline bool operator==(const T *ptr) const { return d == ptr; }
    inline bool operator!=(const T *ptr) const { return d != ptr; }

    inline QExplicitlySharedDataPointer() { d = 0; }
    inline ~QExplicitlySharedDataPointer() { if (d && !d->ref.deref()) delete d; }

    explicit QExplicitlySharedDataPointer(T *data);
    inline QExplicitlySharedDataPointer(const QExplicitlySharedDataPointer<T> &o) : d(o.d) { if (d) d->ref.ref(); }

    template<class X>
    inline QExplicitlySharedDataPointer(const QExplicitlySharedDataPointer<X> &o) : d(static_cast<T *>(o.data()))
    {
        if(d)
            d->ref.ref();
    }

    inline QExplicitlySharedDataPointer<T> & operator=(const QExplicitlySharedDataPointer<T> &o) {
        if (o.d != d) {
            if (o.d)
                o.d->ref.ref();
            T *old = d;
            d = o.d;
            if (old && !old->ref.deref())
                delete old;
        }
        return *this;
    }
    inline QExplicitlySharedDataPointer &operator=(T *o) {
        if (o != d) {
            if (o)
                o->ref.ref();
            T *old = d;
            d = o;
            if (old && !old->ref.deref())
                delete old;
        }
        return *this;
    }
#ifdef Q_COMPILER_RVALUE_REFS
    inline QExplicitlySharedDataPointer(QExplicitlySharedDataPointer &&o) : d(o.d) { o.d = 0; }
    inline QExplicitlySharedDataPointer<T> &operator=(QExplicitlySharedDataPointer<T> &&other)
    { qSwap(d, other.d); return *this; }
#endif

    inline bool operator!() const { return !d; }

    inline void swap(QExplicitlySharedDataPointer &other)
    { qSwap(d, other.d); }

protected:
    T *clone();

private:
    void detach_helper();

    T *d;
};

template <class T>
Q_INLINE_TEMPLATE QSharedDataPointer<T>::QSharedDataPointer(T *adata) : d(adata)
{ if (d) d->ref.ref(); }

template <class T>
Q_INLINE_TEMPLATE T *QSharedDataPointer<T>::clone()
{
    return new T(*d);
}

template <class T>
Q_OUTOFLINE_TEMPLATE void QSharedDataPointer<T>::detach_helper()
{
    T *x = clone();
    x->ref.ref();
    if (!d->ref.deref())
        delete d;
    d = x;
}

template <class T>
Q_INLINE_TEMPLATE T *QExplicitlySharedDataPointer<T>::clone()
{
    return new T(*d);
}

template <class T>
Q_OUTOFLINE_TEMPLATE void QExplicitlySharedDataPointer<T>::detach_helper()
{
    T *x = clone();
    x->ref.ref();
    if (!d->ref.deref())
        delete d;
    d = x;
}

template <class T>
Q_INLINE_TEMPLATE QExplicitlySharedDataPointer<T>::QExplicitlySharedDataPointer(T *adata) : d(adata)
{ if (d) d->ref.ref(); }

template <class T>
Q_INLINE_TEMPLATE void qSwap(QSharedDataPointer<T> &p1, QSharedDataPointer<T> &p2)
{ p1.swap(p2); }

template <class T>
Q_INLINE_TEMPLATE void qSwap(QExplicitlySharedDataPointer<T> &p1, QExplicitlySharedDataPointer<T> &p2)
{ p1.swap(p2); }

#ifndef QT_NO_STL
QT_END_NAMESPACE
namespace std {
    template <class T>
    Q_INLINE_TEMPLATE void swap(QT_PREPEND_NAMESPACE(QSharedDataPointer)<T> &p1, QT_PREPEND_NAMESPACE(QSharedDataPointer)<T> &p2)
    { p1.swap(p2); }

    template <class T>
    Q_INLINE_TEMPLATE void swap(QT_PREPEND_NAMESPACE(QExplicitlySharedDataPointer)<T> &p1, QT_PREPEND_NAMESPACE(QExplicitlySharedDataPointer)<T> &p2)
    { p1.swap(p2); }
}
QT_BEGIN_NAMESPACE
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSHAREDDATA_H
