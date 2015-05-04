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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#ifndef QSCOPEDPOINTER_P_H
#define QSCOPEDPOINTER_P_H

#include "QtCore/qscopedpointer.h"

QT_BEGIN_NAMESPACE


/* Internal helper class - exposes the data through data_ptr (legacy from QShared).
   Required for some internal Qt classes, do not use otherwise. */
template <typename T, typename Cleanup = QScopedPointerDeleter<T> >
class QCustomScopedPointer : public QScopedPointer<T, Cleanup>
{
public:
    explicit inline QCustomScopedPointer(T *p = 0)
        : QScopedPointer<T, Cleanup>(p)
    {
    }

    inline T *&data_ptr()
    {
        return this->d;
    }

    inline bool operator==(const QCustomScopedPointer<T, Cleanup> &other) const
    {
        return this->d == other.d;
    }

    inline bool operator!=(const QCustomScopedPointer<T, Cleanup> &other) const
    {
        return this->d != other.d;
    }

private:
    Q_DISABLE_COPY(QCustomScopedPointer)
};

/* Internal helper class - a handler for QShared* classes, to be used in QCustomScopedPointer */
template <typename T>
class QScopedPointerSharedDeleter
{
public:
    static inline void cleanup(T *d)
    {
        if (d && !d->ref.deref())
            delete d;
    }
};

/* Internal.
   This class is basically a scoped pointer pointing to a ref-counted object
 */
template <typename T>
class QScopedSharedPointer : public QCustomScopedPointer<T, QScopedPointerSharedDeleter<T> >
{
public:
    explicit inline QScopedSharedPointer(T *p = 0)
        : QCustomScopedPointer<T, QScopedPointerSharedDeleter<T> >(p)
    {
    }

    inline void detach()
    {
        qAtomicDetach(this->d);
    }

    inline void assign(T *other)
    {
        if (this->d == other)
            return;
        if (other)
            other->ref.ref();
        T *oldD = this->d;
        this->d = other;
        QScopedPointerSharedDeleter<T>::cleanup(oldD);
    }

    inline bool operator==(const QScopedSharedPointer<T> &other) const
    {
        return this->d == other.d;
    }

    inline bool operator!=(const QScopedSharedPointer<T> &other) const
    {
        return this->d != other.d;
    }

private:
    Q_DISABLE_COPY(QScopedSharedPointer)
};


QT_END_NAMESPACE

#endif
