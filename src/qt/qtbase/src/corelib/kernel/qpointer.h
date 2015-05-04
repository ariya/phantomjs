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

#ifndef QPOINTER_H
#define QPOINTER_H

#include <QtCore/qsharedpointer.h>

#ifndef QT_NO_QOBJECT

QT_BEGIN_NAMESPACE

class QVariant;

template <class T>
class QPointer
{
    template<typename U>
    struct TypeSelector
    {
        typedef QObject Type;
    };
    template<typename U>
    struct TypeSelector<const U>
    {
        typedef const QObject Type;
    };
    typedef typename TypeSelector<T>::Type QObjectType;
    QWeakPointer<QObjectType> wp;
public:
    inline QPointer() { }
    inline QPointer(T *p) : wp(p, true) { }
    // compiler-generated copy/move ctor/assignment operators are fine!
    inline ~QPointer() { }

    inline QPointer<T> &operator=(T* p)
    { wp.assign(static_cast<QObjectType*>(p)); return *this; }

    inline T* data() const
    { return static_cast<T*>( wp.data()); }
    inline T* operator->() const
    { return data(); }
    inline T& operator*() const
    { return *data(); }
    inline operator T*() const
    { return data(); }

    inline bool isNull() const
    { return wp.isNull(); }

    inline void clear()
    { wp.clear(); }
};
template <class T> Q_DECLARE_TYPEINFO_BODY(QPointer<T>, Q_MOVABLE_TYPE);

template <class T>
inline bool operator==(const T *o, const QPointer<T> &p)
{ return o == p.operator->(); }

template<class T>
inline bool operator==(const QPointer<T> &p, const T *o)
{ return p.operator->() == o; }

template <class T>
inline bool operator==(T *o, const QPointer<T> &p)
{ return o == p.operator->(); }

template<class T>
inline bool operator==(const QPointer<T> &p, T *o)
{ return p.operator->() == o; }

template<class T>
inline bool operator==(const QPointer<T> &p1, const QPointer<T> &p2)
{ return p1.operator->() == p2.operator->(); }

template <class T>
inline bool operator!=(const T *o, const QPointer<T> &p)
{ return o != p.operator->(); }

template<class T>
inline bool operator!= (const QPointer<T> &p, const T *o)
{ return p.operator->() != o; }

template <class T>
inline bool operator!=(T *o, const QPointer<T> &p)
{ return o != p.operator->(); }

template<class T>
inline bool operator!= (const QPointer<T> &p, T *o)
{ return p.operator->() != o; }

template<class T>
inline bool operator!= (const QPointer<T> &p1, const QPointer<T> &p2)
{ return p1.operator->() != p2.operator->() ; }

template<typename T>
QPointer<T>
qPointerFromVariant(const QVariant &variant)
{
    return QPointer<T>(qobject_cast<T*>(QtSharedPointer::weakPointerFromVariant_internal(variant).data()));
}

QT_END_NAMESPACE

#endif // QT_NO_QOBJECT

#endif // QPOINTER_H
