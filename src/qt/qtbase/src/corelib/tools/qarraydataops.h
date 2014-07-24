/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QARRAYDATAOPS_H
#define QARRAYDATAOPS_H

#include <QtCore/qarraydata.h>

#include <new>
#include <string.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

template <class T>
struct QPodArrayOps
    : QTypedArrayData<T>
{
    void appendInitialize(size_t newSize)
    {
        Q_ASSERT(!this->ref.isShared());
        Q_ASSERT(newSize > uint(this->size));
        Q_ASSERT(newSize <= this->alloc);

        ::memset(this->end(), 0, (newSize - this->size) * sizeof(T));
        this->size = int(newSize);
    }

    void copyAppend(const T *b, const T *e)
    {
        Q_ASSERT(!this->ref.isShared());
        Q_ASSERT(b < e);
        Q_ASSERT(size_t(e - b) <= this->alloc - uint(this->size));

        ::memcpy(this->end(), b, (e - b) * sizeof(T));
        this->size += e - b;
    }

    void copyAppend(size_t n, const T &t)
    {
        Q_ASSERT(!this->ref.isShared());
        Q_ASSERT(n <= this->alloc - uint(this->size));

        T *iter = this->end();
        const T *const end = iter + n;
        for (; iter != end; ++iter)
            ::memcpy(iter, &t, sizeof(T));
        this->size += int(n);
    }

    void truncate(size_t newSize)
    {
        Q_ASSERT(!this->ref.isShared());
        Q_ASSERT(newSize < size_t(this->size));

        this->size = int(newSize);
    }

    void destroyAll() // Call from destructors, ONLY!
    {
        Q_ASSERT(this->ref.atomic.load() == 0);

        // As this is to be called only from destructor, it doesn't need to be
        // exception safe; size not updated.
    }

    void insert(T *where, const T *b, const T *e)
    {
        Q_ASSERT(!this->ref.isShared());
        Q_ASSERT(where >= this->begin() && where < this->end()); // Use copyAppend at end
        Q_ASSERT(b < e);
        Q_ASSERT(e <= where || b > this->end()); // No overlap
        Q_ASSERT(size_t(e - b) <= this->alloc - uint(this->size));

        ::memmove(where + (e - b), where, (static_cast<const T*>(this->end()) - where) * sizeof(T));
        ::memcpy(where, b, (e - b) * sizeof(T));
        this->size += (e - b);
    }

    void erase(T *b, T *e)
    {
        Q_ASSERT(b < e);
        Q_ASSERT(b >= this->begin() && b < this->end());
        Q_ASSERT(e > this->begin() && e < this->end());

        ::memmove(b, e, (static_cast<T *>(this->end()) - e) * sizeof(T));
        this->size -= (e - b);
    }
};

template <class T>
struct QGenericArrayOps
    : QTypedArrayData<T>
{
    void appendInitialize(size_t newSize)
    {
        Q_ASSERT(!this->ref.isShared());
        Q_ASSERT(newSize > uint(this->size));
        Q_ASSERT(newSize <= this->alloc);

        T *const begin = this->begin();
        do {
            new (begin + this->size) T();
        } while (uint(++this->size) != newSize);
    }

    void copyAppend(const T *b, const T *e)
    {
        Q_ASSERT(!this->ref.isShared());
        Q_ASSERT(b < e);
        Q_ASSERT(size_t(e - b) <= this->alloc - uint(this->size));

        T *iter = this->end();
        for (; b != e; ++iter, ++b) {
            new (iter) T(*b);
            ++this->size;
        }
    }

    void copyAppend(size_t n, const T &t)
    {
        Q_ASSERT(!this->ref.isShared());
        Q_ASSERT(n <= this->alloc - uint(this->size));

        T *iter = this->end();
        const T *const end = iter + n;
        for (; iter != end; ++iter) {
            new (iter) T(t);
            ++this->size;
        }
    }

    void truncate(size_t newSize)
    {
        Q_ASSERT(!this->ref.isShared());
        Q_ASSERT(newSize < size_t(this->size));

        const T *const b = this->begin();
        do {
            (b + --this->size)->~T();
        } while (uint(this->size) != newSize);
    }

    void destroyAll() // Call from destructors, ONLY
    {
        // As this is to be called only from destructor, it doesn't need to be
        // exception safe; size not updated.

        Q_ASSERT(this->ref.atomic.load() == 0);

        const T *const b = this->begin();
        const T *i = this->end();

        while (i != b)
            (--i)->~T();
    }

    void insert(T *where, const T *b, const T *e)
    {
        Q_ASSERT(!this->ref.isShared());
        Q_ASSERT(where >= this->begin() && where < this->end()); // Use copyAppend at end
        Q_ASSERT(b < e);
        Q_ASSERT(e <= where || b > this->end()); // No overlap
        Q_ASSERT(size_t(e - b) <= this->alloc - uint(this->size));

        // Array may be truncated at where in case of exceptions

        T *const end = this->end();
        const T *readIter = end;
        T *writeIter = end + (e - b);

        const T *const step1End = where + qMax(e - b, end - where);

        struct Destructor
        {
            Destructor(T *&it)
                : iter(&it)
                , end(it)
            {
            }

            void commit()
            {
                iter = &end;
            }

            ~Destructor()
            {
                for (; *iter != end; --*iter)
                    (*iter)->~T();
            }

            T **iter;
            T *end;
        } destroyer(writeIter);

        // Construct new elements in array
        do {
            --readIter, --writeIter;
            new (writeIter) T(*readIter);
        } while (writeIter != step1End);

        while (writeIter != end) {
            --e, --writeIter;
            new (writeIter) T(*e);
        }

        destroyer.commit();
        this->size += destroyer.end - end;

        // Copy assign over existing elements
        while (readIter != where) {
            --readIter, --writeIter;
            *writeIter = *readIter;
        }

        while (writeIter != where) {
            --e, --writeIter;
            *writeIter = *e;
        }
    }

    void erase(T *b, T *e)
    {
        Q_ASSERT(b < e);
        Q_ASSERT(b >= this->begin() && b < this->end());
        Q_ASSERT(e > this->begin() && e < this->end());

        const T *const end = this->end();

        do {
            *b = *e;
            ++b, ++e;
        } while (e != end);

        do {
            (--e)->~T();
            --this->size;
        } while (e != b);
    }
};

template <class T>
struct QMovableArrayOps
    : QGenericArrayOps<T>
{
    // using QGenericArrayOps<T>::appendInitialize;
    // using QGenericArrayOps<T>::copyAppend;
    // using QGenericArrayOps<T>::truncate;
    // using QGenericArrayOps<T>::destroyAll;

    void insert(T *where, const T *b, const T *e)
    {
        Q_ASSERT(!this->ref.isShared());
        Q_ASSERT(where >= this->begin() && where < this->end()); // Use copyAppend at end
        Q_ASSERT(b < e);
        Q_ASSERT(e <= where || b > this->end()); // No overlap
        Q_ASSERT(size_t(e - b) <= this->alloc - uint(this->size));

        // Provides strong exception safety guarantee,
        // provided T::~T() nothrow

        struct ReversibleDisplace
        {
            ReversibleDisplace(T *start, T *finish, size_t diff)
                : begin(start)
                , end(finish)
                , displace(diff)
            {
                ::memmove(begin + displace, begin, (end - begin) * sizeof(T));
            }

            void commit() { displace = 0; }

            ~ReversibleDisplace()
            {
                if (displace)
                    ::memmove(begin, begin + displace, (end - begin) * sizeof(T));
            }

            T *const begin;
            T *const end;
            size_t displace;

        } displace(where, this->end(), size_t(e - b));

        struct CopyConstructor
        {
            CopyConstructor(T *w) : where(w) {}

            void copy(const T *src, const T *const srcEnd)
            {
                n = 0;
                for (; src != srcEnd; ++src) {
                    new (where + n) T(*src);
                    ++n;
                }
                n = 0;
            }

            ~CopyConstructor()
            {
                while (n)
                    where[--n].~T();
            }

            T *const where;
            size_t n;
        } copier(where);

        copier.copy(b, e);
        displace.commit();
        this->size += (e - b);
    }

    void erase(T *b, T *e)
    {
        Q_ASSERT(b < e);
        Q_ASSERT(b >= this->begin() && b < this->end());
        Q_ASSERT(e > this->begin() && e < this->end());

        struct Mover
        {
            Mover(T *&start, const T *finish, int &sz)
                : destination(start)
                , source(start)
                , n(finish - start)
                , size(sz)
            {
            }

            ~Mover()
            {
                ::memmove(destination, source, n * sizeof(T));
                size -= (source - destination);
            }

            T *&destination;
            const T *const source;
            size_t n;
            int &size;
        } mover(e, this->end(), this->size);

        do {
            // Exceptions or not, dtor called once per instance
            (--e)->~T();
        } while (e != b);
    }
};

template <class T, class = void>
struct QArrayOpsSelector
{
    typedef QGenericArrayOps<T> Type;
};

template <class T>
struct QArrayOpsSelector<T,
    typename QEnableIf<
        !QTypeInfo<T>::isComplex && !QTypeInfo<T>::isStatic
    >::Type>
{
    typedef QPodArrayOps<T> Type;
};

template <class T>
struct QArrayOpsSelector<T,
    typename QEnableIf<
        QTypeInfo<T>::isComplex && !QTypeInfo<T>::isStatic
    >::Type>
{
    typedef QMovableArrayOps<T> Type;
};

} // namespace QtPrivate

template <class T>
struct QArrayDataOps
    : QtPrivate::QArrayOpsSelector<T>::Type
{
};

QT_END_NAMESPACE

#endif // include guard
