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

#ifndef QITERATOR_H
#define QITERATOR_H

#include <QtCore/qglobal.h>

QT_BEGIN_HEADER

namespace std {
    struct bidirectional_iterator_tag;
    struct random_access_iterator_tag;
}

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#define Q_DECLARE_SEQUENTIAL_ITERATOR(C) \
\
template <class T> \
class Q##C##Iterator \
{ \
    typedef typename Q##C<T>::const_iterator const_iterator; \
    Q##C<T> c; \
    const_iterator i; \
public: \
    inline Q##C##Iterator(const Q##C<T> &container) \
        : c(container), i(c.constBegin()) {} \
    inline Q##C##Iterator &operator=(const Q##C<T> &container) \
    { c = container; i = c.constBegin(); return *this; } \
    inline void toFront() { i = c.constBegin(); } \
    inline void toBack() { i = c.constEnd(); } \
    inline bool hasNext() const { return i != c.constEnd(); } \
    inline const T &next() { return *i++; } \
    inline const T &peekNext() const { return *i; } \
    inline bool hasPrevious() const { return i != c.constBegin(); } \
    inline const T &previous() { return *--i; } \
    inline const T &peekPrevious() const { const_iterator p = i; return *--p; } \
    inline bool findNext(const T &t) \
    { while (i != c.constEnd()) if (*i++ == t) return true; return false; } \
    inline bool findPrevious(const T &t) \
    { while (i != c.constBegin()) if (*(--i) == t) return true; \
      return false;  } \
};

#define Q_DECLARE_MUTABLE_SEQUENTIAL_ITERATOR(C) \
\
template <class T> \
class QMutable##C##Iterator \
{ \
    typedef typename Q##C<T>::iterator iterator; \
    typedef typename Q##C<T>::const_iterator const_iterator; \
    Q##C<T> *c; \
    iterator i, n; \
    inline bool item_exists() const { return const_iterator(n) != c->constEnd(); } \
public: \
    inline QMutable##C##Iterator(Q##C<T> &container) \
        : c(&container) \
    { c->setSharable(false); i = c->begin(); n = c->end(); } \
    inline ~QMutable##C##Iterator() \
    { c->setSharable(true); } \
    inline QMutable##C##Iterator &operator=(Q##C<T> &container) \
    { c->setSharable(true); c = &container; c->setSharable(false); \
      i = c->begin(); n = c->end(); return *this; } \
    inline void toFront() { i = c->begin(); n = c->end(); } \
    inline void toBack() { i = c->end(); n = i; } \
    inline bool hasNext() const { return c->constEnd() != const_iterator(i); } \
    inline T &next() { n = i++; return *n; } \
    inline T &peekNext() const { return *i; } \
    inline bool hasPrevious() const { return c->constBegin() != const_iterator(i); } \
    inline T &previous() { n = --i; return *n; } \
    inline T &peekPrevious() const { iterator p = i; return *--p; } \
    inline void remove() \
    { if (c->constEnd() != const_iterator(n)) { i = c->erase(n); n = c->end(); } } \
    inline void setValue(const T &t) const { if (c->constEnd() != const_iterator(n)) *n = t; } \
    inline T &value() { Q_ASSERT(item_exists()); return *n; } \
    inline const T &value() const { Q_ASSERT(item_exists()); return *n; } \
    inline void insert(const T &t) { n = i = c->insert(i, t); ++i; } \
    inline bool findNext(const T &t) \
    { while (c->constEnd() != const_iterator(n = i)) if (*i++ == t) return true; return false; } \
    inline bool findPrevious(const T &t) \
    { while (c->constBegin() != const_iterator(i)) if (*(n = --i) == t) return true; \
      n = c->end(); return false;  } \
};

#define Q_DECLARE_ASSOCIATIVE_ITERATOR(C) \
\
template <class Key, class T> \
class Q##C##Iterator \
{ \
    typedef typename Q##C<Key,T>::const_iterator const_iterator; \
    typedef const_iterator Item; \
    Q##C<Key,T> c; \
    const_iterator i, n; \
    inline bool item_exists() const { return n != c.constEnd(); } \
public: \
    inline Q##C##Iterator(const Q##C<Key,T> &container) \
        : c(container), i(c.constBegin()), n(c.constEnd()) {} \
    inline Q##C##Iterator &operator=(const Q##C<Key,T> &container) \
    { c = container; i = c.constBegin(); n = c.constEnd(); return *this; } \
    inline void toFront() { i = c.constBegin(); n = c.constEnd(); } \
    inline void toBack() { i = c.constEnd(); n = c.constEnd(); } \
    inline bool hasNext() const { return i != c.constEnd(); } \
    inline Item next() { n = i++; return n; } \
    inline Item peekNext() const { return i; } \
    inline bool hasPrevious() const { return i != c.constBegin(); } \
    inline Item previous() { n = --i; return n; } \
    inline Item peekPrevious() const { const_iterator p = i; return --p; } \
    inline const T &value() const { Q_ASSERT(item_exists()); return *n; } \
    inline const Key &key() const { Q_ASSERT(item_exists()); return n.key(); } \
    inline bool findNext(const T &t) \
    { while ((n = i) != c.constEnd()) if (*i++ == t) return true; return false; } \
    inline bool findPrevious(const T &t) \
    { while (i != c.constBegin()) if (*(n = --i) == t) return true; \
      n = c.constEnd(); return false; } \
};

#define Q_DECLARE_MUTABLE_ASSOCIATIVE_ITERATOR(C) \
\
template <class Key, class T> \
class QMutable##C##Iterator \
{ \
    typedef typename Q##C<Key,T>::iterator iterator; \
    typedef typename Q##C<Key,T>::const_iterator const_iterator; \
    typedef iterator Item; \
    Q##C<Key,T> *c; \
    iterator i, n; \
    inline bool item_exists() const { return const_iterator(n) != c->constEnd(); } \
public: \
    inline QMutable##C##Iterator(Q##C<Key,T> &container) \
        : c(&container) \
    { c->setSharable(false); i = c->begin(); n = c->end(); } \
    inline ~QMutable##C##Iterator() \
    { c->setSharable(true); } \
    inline QMutable##C##Iterator &operator=(Q##C<Key,T> &container) \
    { c->setSharable(true); c = &container; c->setSharable(false); i = c->begin(); n = c->end(); return *this; } \
    inline void toFront() { i = c->begin(); n = c->end(); } \
    inline void toBack() { i = c->end(); n = c->end(); } \
    inline bool hasNext() const { return const_iterator(i) != c->constEnd(); } \
    inline Item next() { n = i++; return n; } \
    inline Item peekNext() const { return i; } \
    inline bool hasPrevious() const { return const_iterator(i) != c->constBegin(); } \
    inline Item previous() { n = --i; return n; } \
    inline Item peekPrevious() const { iterator p = i; return --p; } \
    inline void remove() \
    { if (const_iterator(n) != c->constEnd()) { i = c->erase(n); n = c->end(); } } \
    inline void setValue(const T &t) { if (const_iterator(n) != c->constEnd()) *n = t; } \
    inline T &value() { Q_ASSERT(item_exists()); return *n; } \
    inline const T &value() const { Q_ASSERT(item_exists()); return *n; } \
    inline const Key &key() const { Q_ASSERT(item_exists()); return n.key(); } \
    inline bool findNext(const T &t) \
    { while (const_iterator(n = i) != c->constEnd()) if (*i++ == t) return true; return false; } \
    inline bool findPrevious(const T &t) \
    { while (const_iterator(i) != c->constBegin()) if (*(n = --i) == t) return true; \
      n = c->end(); return false; } \
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QITERATOR_H
