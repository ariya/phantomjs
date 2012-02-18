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

#ifndef QTHREADSTORAGE_H
#define QTHREADSTORAGE_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_THREAD

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

class Q_CORE_EXPORT QThreadStorageData
{
public:
    explicit QThreadStorageData(void (*func)(void *));
    ~QThreadStorageData();

    void** get() const;
    void** set(void* p);

    static void finish(void**);
    int id;
};

#if !defined(QT_MOC_CPP)
// MOC_SKIP_BEGIN

// pointer specialization
template <typename T>
inline
T *&qThreadStorage_localData(QThreadStorageData &d, T **)
{
    void **v = d.get();
    if (!v) v = d.set(0);
    return *(reinterpret_cast<T**>(v));
}

template <typename T>
inline
T *qThreadStorage_localData_const(const QThreadStorageData &d, T **)
{
    void **v = d.get();
    return v ? *(reinterpret_cast<T**>(v)) : 0;
}

template <typename T>
inline
void qThreadStorage_setLocalData(QThreadStorageData &d, T **t)
{ (void) d.set(*t); }

template <typename T>
inline
void qThreadStorage_deleteData(void *d, T **)
{ delete static_cast<T *>(d); }

// value-based specialization
template <typename T>
inline
T &qThreadStorage_localData(QThreadStorageData &d, T *)
{
    void **v = d.get();
    if (!v) v = d.set(new T());
    return *(reinterpret_cast<T*>(*v));
}

template <typename T>
inline
T qThreadStorage_localData_const(const QThreadStorageData &d, T *)
{
    void **v = d.get();
    return v ? *(reinterpret_cast<T*>(*v)) : T();
}

template <typename T>
inline
void qThreadStorage_setLocalData(QThreadStorageData &d, T *t)
{ (void) d.set(new T(*t)); }

template <typename T>
inline
void qThreadStorage_deleteData(void *d, T *)
{ delete static_cast<T *>(d); }


// MOC_SKIP_END
#endif

template <class T>
class QThreadStorage
{
private:
    QThreadStorageData d;

    Q_DISABLE_COPY(QThreadStorage)

    static inline void deleteData(void *x)
    { qThreadStorage_deleteData(x, reinterpret_cast<T*>(0)); }

public:
    inline QThreadStorage() : d(deleteData) { }
    inline ~QThreadStorage() { }

    inline bool hasLocalData() const
    { return d.get() != 0; }

    inline T& localData()
    { return qThreadStorage_localData(d, reinterpret_cast<T*>(0)); }
    inline T localData() const
    { return qThreadStorage_localData_const(d, reinterpret_cast<T*>(0)); }

    inline void setLocalData(T t)
    { qThreadStorage_setLocalData(d, &t); }
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_THREAD

#endif // QTHREADSTORAGE_H
