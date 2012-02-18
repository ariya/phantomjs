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

#ifndef QSET_H
#define QSET_H

#include <QtCore/qhash.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

template <class T>
class QSet
{
    typedef QHash<T, QHashDummyValue> Hash;

public:
    inline QSet() {}
    inline QSet(const QSet<T> &other) : q_hash(other.q_hash) {}

    inline QSet<T> &operator=(const QSet<T> &other)
        { q_hash = other.q_hash; return *this; }
#ifdef Q_COMPILER_RVALUE_REFS
    inline QSet<T> &operator=(QSet<T> &&other)
        { qSwap(q_hash, other.q_hash); return *this; }
#endif
    inline void swap(QSet<T> &other) { q_hash.swap(other.q_hash); }

    inline bool operator==(const QSet<T> &other) const
        { return q_hash == other.q_hash; }
    inline bool operator!=(const QSet<T> &other) const
        { return q_hash != other.q_hash; }

    inline int size() const { return q_hash.size(); }

    inline bool isEmpty() const { return q_hash.isEmpty(); }

    inline int capacity() const { return q_hash.capacity(); }
    inline void reserve(int size);
    inline void squeeze() { q_hash.squeeze(); }

    inline void detach() { q_hash.detach(); }
    inline bool isDetached() const { return q_hash.isDetached(); }
    inline void setSharable(bool sharable) { q_hash.setSharable(sharable); }

    inline void clear() { q_hash.clear(); }

    inline bool remove(const T &value) { return q_hash.remove(value) != 0; }

    inline bool contains(const T &value) const { return q_hash.contains(value); }

    bool contains(const QSet<T> &set) const;

    class const_iterator;

    class iterator
    {
        typedef QHash<T, QHashDummyValue> Hash;
        typename Hash::iterator i;
        friend class const_iterator;

    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef qptrdiff difference_type;
        typedef T value_type;
        typedef const T *pointer;
        typedef const T &reference;

        inline iterator() {}
        inline iterator(typename Hash::iterator o) : i(o) {}
        inline iterator(const iterator &o) : i(o.i) {}
        inline iterator &operator=(const iterator &o) { i = o.i; return *this; }
        inline const T &operator*() const { return i.key(); }
        inline const T *operator->() const { return &i.key(); }
        inline bool operator==(const iterator &o) const { return i == o.i; }
        inline bool operator!=(const iterator &o) const { return i != o.i; }
        inline bool operator==(const const_iterator &o) const
            { return i == o.i; }
        inline bool operator!=(const const_iterator &o) const
            { return i != o.i; }
        inline iterator &operator++() { ++i; return *this; }
        inline iterator operator++(int) { iterator r = *this; ++i; return r; }
        inline iterator &operator--() { --i; return *this; }
        inline iterator operator--(int) { iterator r = *this; --i; return r; }
        inline iterator operator+(int j) const { return i + j; }
        inline iterator operator-(int j) const { return i - j; }
        inline iterator &operator+=(int j) { i += j; return *this; }
        inline iterator &operator-=(int j) { i -= j; return *this; }
    };

    class const_iterator
    {
        typedef QHash<T, QHashDummyValue> Hash;
        typename Hash::const_iterator i;
        friend class iterator;

    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef qptrdiff difference_type;
        typedef T value_type;
        typedef const T *pointer;
        typedef const T &reference;

        inline const_iterator() {}
        inline const_iterator(typename Hash::const_iterator o) : i(o) {}
        inline const_iterator(const const_iterator &o) : i(o.i) {}
        inline const_iterator(const iterator &o)
            : i(o.i) {}
        inline const_iterator &operator=(const const_iterator &o) { i = o.i; return *this; }
        inline const T &operator*() const { return i.key(); }
        inline const T *operator->() const { return &i.key(); }
        inline bool operator==(const const_iterator &o) const { return i == o.i; }
        inline bool operator!=(const const_iterator &o) const { return i != o.i; }
        inline const_iterator &operator++() { ++i; return *this; }
        inline const_iterator operator++(int) { const_iterator r = *this; ++i; return r; }
        inline const_iterator &operator--() { --i; return *this; }
        inline const_iterator operator--(int) { const_iterator r = *this; --i; return r; }
        inline const_iterator operator+(int j) const { return i + j; }
        inline const_iterator operator-(int j) const { return i - j; }
        inline const_iterator &operator+=(int j) { i += j; return *this; }
        inline const_iterator &operator-=(int j) { i -= j; return *this; }
    };

    // STL style
    inline iterator begin() { return q_hash.begin(); }
    inline const_iterator begin() const { return q_hash.begin(); }
    inline const_iterator constBegin() const { return q_hash.constBegin(); }
    inline iterator end() { return q_hash.end(); }
    inline const_iterator end() const { return q_hash.end(); }
    inline const_iterator constEnd() const { return q_hash.constEnd(); }
    iterator erase(iterator i)
        { return q_hash.erase(reinterpret_cast<typename Hash::iterator &>(i)); }

    // more Qt
    typedef iterator Iterator;
    typedef const_iterator ConstIterator;
    inline int count() const { return q_hash.count(); }
    inline const_iterator insert(const T &value) // ### Qt 5: should return an 'iterator'
        { return static_cast<typename Hash::const_iterator>(q_hash.insert(value,
                                                                          QHashDummyValue())); }
    iterator find(const T &value) { return q_hash.find(value); }
    const_iterator find(const T &value) const { return q_hash.find(value); }
    inline const_iterator constFind(const T &value) const { return find(value); }
    QSet<T> &unite(const QSet<T> &other);
    QSet<T> &intersect(const QSet<T> &other);
    QSet<T> &subtract(const QSet<T> &other);

    // STL compatibility
    typedef T key_type;
    typedef T value_type;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef qptrdiff difference_type;
    typedef int size_type;

    inline bool empty() const { return isEmpty(); }
    // comfort
    inline QSet<T> &operator<<(const T &value) { insert(value); return *this; }
    inline QSet<T> &operator|=(const QSet<T> &other) { unite(other); return *this; }
    inline QSet<T> &operator|=(const T &value) { insert(value); return *this; }
    inline QSet<T> &operator&=(const QSet<T> &other) { intersect(other); return *this; }
    inline QSet<T> &operator&=(const T &value)
        { QSet<T> result; if (contains(value)) result.insert(value); return (*this = result); }
    inline QSet<T> &operator+=(const QSet<T> &other) { unite(other); return *this; }
    inline QSet<T> &operator+=(const T &value) { insert(value); return *this; }
    inline QSet<T> &operator-=(const QSet<T> &other) { subtract(other); return *this; }
    inline QSet<T> &operator-=(const T &value) { remove(value); return *this; }
    inline QSet<T> operator|(const QSet<T> &other) const
        { QSet<T> result = *this; result |= other; return result; }
    inline QSet<T> operator&(const QSet<T> &other) const
        { QSet<T> result = *this; result &= other; return result; }
    inline QSet<T> operator+(const QSet<T> &other) const
        { QSet<T> result = *this; result += other; return result; }
    inline QSet<T> operator-(const QSet<T> &other) const
        { QSet<T> result = *this; result -= other; return result; }
#if QT_VERSION < 0x050000
    // ### Qt 5: remove
    inline QSet<T> operator|(const QSet<T> &other)
        { QSet<T> result = *this; result |= other; return result; }
    inline QSet<T> operator&(const QSet<T> &other)
        { QSet<T> result = *this; result &= other; return result; }
    inline QSet<T> operator+(const QSet<T> &other)
        { QSet<T> result = *this; result += other; return result; }
    inline QSet<T> operator-(const QSet<T> &other)
        { QSet<T> result = *this; result -= other; return result; }
#endif

    QList<T> toList() const;
    inline QList<T> values() const { return toList(); }

    static QSet<T> fromList(const QList<T> &list);

private:
    Hash q_hash;
};

template <class T>
Q_INLINE_TEMPLATE void QSet<T>::reserve(int asize) { q_hash.reserve(asize); }

template <class T>
Q_INLINE_TEMPLATE QSet<T> &QSet<T>::unite(const QSet<T> &other)
{
    QSet<T> copy(other);
    typename QSet<T>::const_iterator i = copy.constEnd();
    while (i != copy.constBegin()) {
        --i;
        insert(*i);
    }
    return *this;
}

template <class T>
Q_INLINE_TEMPLATE QSet<T> &QSet<T>::intersect(const QSet<T> &other)
{
    QSet<T> copy1(*this);
    QSet<T> copy2(other);
    typename QSet<T>::const_iterator i = copy1.constEnd();
    while (i != copy1.constBegin()) {
        --i;
        if (!copy2.contains(*i))
            remove(*i);
    }
    return *this;
}

template <class T>
Q_INLINE_TEMPLATE QSet<T> &QSet<T>::subtract(const QSet<T> &other)
{
    QSet<T> copy1(*this);
    QSet<T> copy2(other);
    typename QSet<T>::const_iterator i = copy1.constEnd();
    while (i != copy1.constBegin()) {
        --i;
        if (copy2.contains(*i))
            remove(*i);
    }
    return *this;
}

template <class T>
Q_INLINE_TEMPLATE bool QSet<T>::contains(const QSet<T> &other) const
{
    typename QSet<T>::const_iterator i = other.constBegin();
    while (i != other.constEnd()) {
        if (!contains(*i))
            return false;
        ++i;
    }
    return true;
}

template <typename T>
Q_OUTOFLINE_TEMPLATE QList<T> QSet<T>::toList() const
{
    QList<T> result;
    result.reserve(size());
    typename QSet<T>::const_iterator i = constBegin();
    while (i != constEnd()) {
        result.append(*i);
        ++i;
    }
    return result;
}

template <typename T>
Q_OUTOFLINE_TEMPLATE QSet<T> QList<T>::toSet() const
{
    QSet<T> result;
    result.reserve(size());
    for (int i = 0; i < size(); ++i)
        result.insert(at(i));
    return result;
}

template <typename T>
QSet<T> QSet<T>::fromList(const QList<T> &list)
{
    return list.toSet();
}

template <typename T>
QList<T> QList<T>::fromSet(const QSet<T> &set)
{
    return set.toList();
}

Q_DECLARE_SEQUENTIAL_ITERATOR(Set)

template <typename T>
class QMutableSetIterator
{
    typedef typename QSet<T>::iterator iterator;
    QSet<T> *c;
    iterator i, n;
    inline bool item_exists() const { return c->constEnd() != n; }

public:
    inline QMutableSetIterator(QSet<T> &container)
        : c(&container)
    { c->setSharable(false); i = c->begin(); n = c->end(); }
    inline ~QMutableSetIterator()
    { c->setSharable(true); }
    inline QMutableSetIterator &operator=(QSet<T> &container)
    { c->setSharable(true); c = &container; c->setSharable(false);
      i = c->begin(); n = c->end(); return *this; }
    inline void toFront() { i = c->begin(); n = c->end(); }
    inline void toBack() { i = c->end(); n = i; }
    inline bool hasNext() const { return c->constEnd() != i; }
    inline const T &next() { n = i++; return *n; }
    inline const T &peekNext() const { return *i; }
    inline bool hasPrevious() const { return c->constBegin() != i; }
    inline const T &previous() { n = --i; return *n; }
    inline const T &peekPrevious() const { iterator p = i; return *--p; }
    inline void remove()
    { if (c->constEnd() != n) { i = c->erase(n); n = c->end(); } }
    inline const T &value() const { Q_ASSERT(item_exists()); return *n; }
    inline bool findNext(const T &t)
    { while (c->constEnd() != (n = i)) if (*i++ == t) return true; return false; }
    inline bool findPrevious(const T &t)
    { while (c->constBegin() != i) if (*(n = --i) == t) return true;
      n = c->end(); return false;  }
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSET_H
