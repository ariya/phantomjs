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

#ifndef QVARIANT_P_H
#define QVARIANT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

// takes a type, returns the internal void* pointer cast
// to a pointer of the input type

#include <QtCore/qglobal.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

#ifdef Q_CC_SUN // Sun CC picks the wrong overload, so introduce awful hack

template <typename T>
inline T *v_cast(const QVariant::Private *nd, T * = 0)
{
    QVariant::Private *d = const_cast<QVariant::Private *>(nd);
    return ((sizeof(T) > sizeof(QVariant::Private::Data))
            ? static_cast<T *>(d->data.shared->ptr)
            : static_cast<T *>(static_cast<void *>(&d->data.c)));
}

#else // every other compiler in this world

template <typename T>
inline const T *v_cast(const QVariant::Private *d, T * = 0)
{
    return ((sizeof(T) > sizeof(QVariant::Private::Data))
            ? static_cast<const T *>(d->data.shared->ptr)
            : static_cast<const T *>(static_cast<const void *>(&d->data.c)));
}

template <typename T>
inline T *v_cast(QVariant::Private *d, T * = 0)
{
    return ((sizeof(T) > sizeof(QVariant::Private::Data))
            ? static_cast<T *>(d->data.shared->ptr)
            : static_cast<T *>(static_cast<void *>(&d->data.c)));
}

#endif


//a simple template that avoids to allocate 2 memory chunks when creating a QVariant
template <class T> class QVariantPrivateSharedEx : public QVariant::PrivateShared
{
public:
    QVariantPrivateSharedEx() : QVariant::PrivateShared(&m_t) { }
    QVariantPrivateSharedEx(const T&t) : QVariant::PrivateShared(&m_t), m_t(t) { }

private:
    T m_t;
};

// constructs a new variant if copy is 0, otherwise copy-constructs
template <class T>
inline void v_construct(QVariant::Private *x, const void *copy, T * = 0)
{
    if (sizeof(T) > sizeof(QVariant::Private::Data)) {
        x->data.shared = copy ? new QVariantPrivateSharedEx<T>(*static_cast<const T *>(copy))
                              : new QVariantPrivateSharedEx<T>;
        x->is_shared = true;
    } else {
        if (copy)
            new (&x->data.ptr) T(*static_cast<const T *>(copy));
        else
            new (&x->data.ptr) T;
    }
}

template <class T>
inline void v_construct(QVariant::Private *x, const T &t)
{
    if (sizeof(T) > sizeof(QVariant::Private::Data)) {
        x->data.shared = new QVariantPrivateSharedEx<T>(t);
        x->is_shared = true;
    } else {
        new (&x->data.ptr) T(t);
    }
}

// deletes the internal structures
template <class T>
inline void v_clear(QVariant::Private *d, T* = 0)
{
    
    if (sizeof(T) > sizeof(QVariant::Private::Data)) {
        //now we need to cast
        //because QVariant::PrivateShared doesn't have a virtual destructor
        delete static_cast<QVariantPrivateSharedEx<T>*>(d->data.shared);
    } else {
        v_cast<T>(d)->~T();
    }

}

Q_CORE_EXPORT const QVariant::Handler *qcoreVariantHandler();

QT_END_NAMESPACE

#endif // QVARIANT_P_H
