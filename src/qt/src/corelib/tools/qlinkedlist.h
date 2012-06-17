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

#ifndef QLINKEDLIST_H
#define QLINKEDLIST_H

#include <QtCore/qiterator.h>
#include <QtCore/qatomic.h>

#ifndef QT_NO_STL
#include <iterator>
#include <list>
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

struct Q_CORE_EXPORT QLinkedListData
{
    QLinkedListData *n, *p;
    QBasicAtomicInt ref;
    int size;
    uint sharable : 1;

    static QLinkedListData shared_null;
};

template <typename T>
struct QLinkedListNode
{
    inline QLinkedListNode(const T &arg): t(arg) { }
    QLinkedListNode *n, *p;
    T t;
};

template <class T>
class QLinkedList
{
    typedef QLinkedListNode<T> Node;
    union { QLinkedListData *d; QLinkedListNode<T> *e; };

public:
    inline QLinkedList() : d(&QLinkedListData::shared_null) { d->ref.ref(); }
    inline QLinkedList(const QLinkedList<T> &l) : d(l.d) { d->ref.ref(); if (!d->sharable) detach(); }
    ~QLinkedList();
    QLinkedList<T> &operator=(const QLinkedList<T> &);
#ifdef Q_COMPILER_RVALUE_REFS
    inline QLinkedList<T> &operator=(QLinkedList<T> &&other)
    { qSwap(d, other.d); return *this; }
#endif
    inline void swap(QLinkedList<T> &other) { qSwap(d, other.d); }
    bool operator==(const QLinkedList<T> &l) const;
    inline bool operator!=(const QLinkedList<T> &l) const { return !(*this == l); }

    inline int size() const { return d->size; }
    inline void detach()
    { if (d->ref != 1) detach_helper(); }
    inline bool isDetached() const { return d->ref == 1; }
    inline void setSharable(bool sharable) { if (!sharable) detach(); d->sharable = sharable; }
    inline bool isSharedWith(const QLinkedList<T> &other) const { return d == other.d; }

    inline bool isEmpty() const { return d->size == 0; }

    void clear();

    void append(const T &);
    void prepend(const T &);
    T takeFirst();
    T takeLast();
    int removeAll(const T &t);
    bool removeOne(const T &t);
    bool contains(const T &t) const;
    int count(const T &t) const;

    class const_iterator;

    class iterator
    {
    public:
        typedef std::bidirectional_iterator_tag  iterator_category;
        typedef qptrdiff difference_type;
        typedef T value_type;
        typedef T *pointer;
        typedef T &reference;
        Node *i;
        inline iterator() : i(0) {}
        inline iterator(Node *n) : i(n) {}
        inline iterator(const iterator &o) : i(o.i) {}
        inline iterator &operator=(const iterator &o) { i = o.i; return *this; }
        inline T &operator*() const { return i->t; }
        inline T *operator->() const { return &i->t; }
        inline bool operator==(const iterator &o) const { return i == o.i; }
        inline bool operator!=(const iterator &o) const { return i != o.i; }
        inline bool operator==(const const_iterator &o) const
            { return i == o.i; }
        inline bool operator!=(const const_iterator &o) const
            { return i != o.i; }
        inline iterator &operator++() { i = i->n; return *this; }
        inline iterator operator++(int) { Node *n = i; i = i->n; return n; }
        inline iterator &operator--() { i = i->p; return *this; }
        inline iterator operator--(int) { Node *n = i; i = i->p; return n; }
        inline iterator operator+(int j) const
        { Node *n = i; if (j > 0) while (j--) n = n->n; else while (j++) n = n->p; return n; }
        inline iterator operator-(int j) const { return operator+(-j); }
        inline iterator &operator+=(int j) { return *this = *this + j; }
        inline iterator &operator-=(int j) { return *this = *this - j; }
    };
    friend class iterator;

    class const_iterator
    {
    public:
        typedef std::bidirectional_iterator_tag  iterator_category;
        typedef qptrdiff difference_type;
        typedef T value_type;
        typedef const T *pointer;
        typedef const T &reference;
        Node *i;
        inline const_iterator() : i(0) {}
        inline const_iterator(Node *n) : i(n) {}
        inline const_iterator(const const_iterator &o) : i(o.i){}
        inline const_iterator(iterator ci) : i(ci.i){}
	inline const_iterator &operator=(const const_iterator &o) { i = o.i; return *this; }
        inline const T &operator*() const { return i->t; }
        inline const T *operator->() const { return &i->t; }
        inline bool operator==(const const_iterator &o) const { return i == o.i; }
        inline bool operator!=(const const_iterator &o) const { return i != o.i; }
        inline const_iterator &operator++() { i = i->n; return *this; }
        inline const_iterator operator++(int) { Node *n = i; i = i->n; return n; }
        inline const_iterator &operator--() { i = i->p; return *this; }
        inline const_iterator operator--(int) { Node *n = i; i = i->p; return n; }
        inline const_iterator operator+(int j) const
        { Node *n = i; if (j > 0) while (j--) n = n->n; else while (j++) n = n->p; return n; }
        inline const_iterator operator-(int j) const { return operator+(-j); }
        inline const_iterator &operator+=(int j) { return *this = *this + j; }
        inline const_iterator &operator-=(int j) { return *this = *this - j; }
    };
    friend class const_iterator;

    // stl style
    inline iterator begin() { detach(); return e->n; }
    inline const_iterator begin() const { return e->n; }
    inline const_iterator constBegin() const { return e->n; }
    inline iterator end() { detach(); return e; }
    inline const_iterator end() const { return e; }
    inline const_iterator constEnd() const { return e; }
    iterator insert(iterator before, const T &t);
    iterator erase(iterator pos);
    iterator erase(iterator first, iterator last);

    // more Qt
    typedef iterator Iterator;
    typedef const_iterator ConstIterator;
    inline int count() const { return d->size; }
    inline T& first() { Q_ASSERT(!isEmpty()); return *begin(); }
    inline const T& first() const { Q_ASSERT(!isEmpty()); return *begin(); }
    T& last() { Q_ASSERT(!isEmpty()); return *(--end()); }
    const T& last() const { Q_ASSERT(!isEmpty()); return *(--end()); }
    inline void removeFirst() { Q_ASSERT(!isEmpty()); erase(begin()); }
    inline void removeLast() { Q_ASSERT(!isEmpty()); erase(--end()); }
    inline bool startsWith(const T &t) const { return !isEmpty() && first() == t; }
    inline bool endsWith(const T &t) const { return !isEmpty() && last() == t; }

    // stl compatibility
    inline void push_back(const T &t) { append(t); }
    inline void push_front(const T &t) { prepend(t); }
    inline T& front() { return first(); }
    inline const T& front() const { return first(); }
    inline T& back() { return last(); }
    inline const T& back() const { return last(); }
    inline void pop_front() { removeFirst(); }
    inline void pop_back() { removeLast(); }
    inline bool empty() const { return isEmpty(); }
    typedef int size_type;
    typedef T value_type;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef qptrdiff difference_type;

#ifndef QT_NO_STL
    static inline QLinkedList<T> fromStdList(const std::list<T> &list)
    { QLinkedList<T> tmp; qCopy(list.begin(), list.end(), std::back_inserter(tmp)); return tmp; }
    inline std::list<T> toStdList() const
    { std::list<T> tmp; qCopy(constBegin(), constEnd(), std::back_inserter(tmp)); return tmp; }
#endif

#ifdef QT3_SUPPORT
    // compatibility
    inline QT3_SUPPORT iterator remove(iterator pos) { return erase(pos); }
    inline QT3_SUPPORT int findIndex(const T& t) const
    { int i=0; for (const_iterator it = begin(); it != end(); ++it, ++i) if(*it == t) return i; return -1;}
    inline QT3_SUPPORT iterator find(iterator from, const T& t)
    { while (from != end() && !(*from == t)) ++from; return from; }
    inline QT3_SUPPORT iterator find(const T& t)
    { return find(begin(), t); }
    inline QT3_SUPPORT const_iterator find(const_iterator from, const T& t) const
    { while (from != end() && !(*from == t)) ++from; return from; }
    inline QT3_SUPPORT const_iterator find(const T& t) const
    { return find(begin(), t); }
#endif

    // comfort
    QLinkedList<T> &operator+=(const QLinkedList<T> &l);
    QLinkedList<T> operator+(const QLinkedList<T> &l) const;
    inline QLinkedList<T> &operator+=(const T &t) { append(t); return *this; }
    inline QLinkedList<T> &operator<< (const T &t) { append(t); return *this; }
    inline QLinkedList<T> &operator<<(const QLinkedList<T> &l) { *this += l; return *this; }

private:
    void detach_helper();
    void free(QLinkedListData*);
};

template <typename T>
inline QLinkedList<T>::~QLinkedList()
{
    if (!d)
        return;
    if (!d->ref.deref())
        free(d);
}

template <typename T>
void QLinkedList<T>::detach_helper()
{
    union { QLinkedListData *d; Node *e; } x;
    x.d = new QLinkedListData;
    x.d->ref = 1;
    x.d->size = d->size;
    x.d->sharable = true;
    Node *original = e->n;
    Node *copy = x.e;
    while (original != e) {
        QT_TRY {
            copy->n = new Node(original->t);
            copy->n->p = copy;
            original = original->n;
            copy = copy->n;
        } QT_CATCH(...) {
            copy->n = x.e;
            free(x.d);
            QT_RETHROW;
        }
    }
    copy->n = x.e;
    x.e->p = copy;
    if (!d->ref.deref())
        free(d);
    d = x.d;
}

template <typename T>
void QLinkedList<T>::free(QLinkedListData *x)
{
    Node *y = reinterpret_cast<Node*>(x);
    Node *i = y->n;
    if (x->ref == 0) {
        while(i != y) {
            Node *n = i;
            i = i->n;
            delete n;
        }
        delete x;
    }
}

template <typename T>
void QLinkedList<T>::clear()
{
    *this = QLinkedList<T>();
}

template <typename T>
QLinkedList<T> &QLinkedList<T>::operator=(const QLinkedList<T> &l)
{
    if (d != l.d) {
        QLinkedListData *o = l.d;
        o->ref.ref();
        if (!d->ref.deref())
            free(d);
        d = o;
        if (!d->sharable)
            detach_helper();
    }
    return *this;
}

template <typename T>
bool QLinkedList<T>::operator== (const QLinkedList<T> &l) const
{
    if (d->size != l.d->size)
        return false;
    if (e == l.e)
        return true;
    Node *i = e->n;
    Node *il = l.e->n;
    while (i != e) {
        if (! (i->t == il->t))
            return false;
        i = i->n;
        il = il->n;
    }
    return true;
}

template <typename T>
void QLinkedList<T>::append(const T &t)
{
    detach();
    Node *i = new Node(t);
    i->n = e;
    i->p = e->p;
    i->p->n = i;
    e->p = i;
    d->size++;
}

template <typename T>
void QLinkedList<T>::prepend(const T &t)
{
    detach();
    Node *i = new Node(t);
    i->n = e->n;
    i->p = e;
    i->n->p = i;
    e->n = i;
    d->size++;
}

template <typename T>
int QLinkedList<T>::removeAll(const T &_t)
{
    detach();
    const T t = _t;
    Node *i = e->n;
    int c = 0;
    while (i != e) {
        if (i->t == t) {
            Node *n = i;
            i->n->p = i->p;
            i->p->n = i->n;
            i = i->n;
            delete n;
            c++;
        } else {
            i = i->n;
        }
    }
    d->size-=c;
    return c;
}

template <typename T>
bool QLinkedList<T>::removeOne(const T &_t)
{
    detach();
    iterator it = qFind(begin(), end(), _t);
    if (it != end()) {
        erase(it);
        return true;
    }
    return false;
}

template <typename T>
inline T QLinkedList<T>::takeFirst()
{
    T t = first();
    removeFirst();
    return t;
}

template <typename T>
inline T QLinkedList<T>::takeLast()
{
    T t = last();
    removeLast();
    return t;
}

template <typename T>
bool QLinkedList<T>::contains(const T &t) const
{
    Node *i = e;
    while ((i = i->n) != e)
        if (i->t == t)
            return true;
    return false;
}

template <typename T>
int QLinkedList<T>::count(const T &t) const
{
    Node *i = e;
    int c = 0;
    while ((i = i->n) != e)
        if (i->t == t)
            c++;
    return c;
}


template <typename T>
typename QLinkedList<T>::iterator QLinkedList<T>::insert(iterator before, const T &t)
{
    Node *i = before.i;
    Node *m = new Node(t);
    m->n = i;
    m->p = i->p;
    m->p->n = m;
    i->p = m;
    d->size++;
    return m;
}

template <typename T>
typename QLinkedList<T>::iterator QLinkedList<T>::erase(typename QLinkedList<T>::iterator afirst,
                                                         typename QLinkedList<T>::iterator alast)
{
    while (afirst != alast)
        erase(afirst++);
    return alast;
}


template <typename T>
typename QLinkedList<T>::iterator QLinkedList<T>::erase(iterator pos)
{
    detach();
    Node *i = pos.i;
    if (i != e) {
        Node *n = i;
        i->n->p = i->p;
        i->p->n = i->n;
        i = i->n;
        delete n;
        d->size--;
    }
    return i;
}

template <typename T>
QLinkedList<T> &QLinkedList<T>::operator+=(const QLinkedList<T> &l)
{
    detach();
    int n = l.d->size;
    d->size += n;
    Node *original = l.e->n;
    while (n--) {
        QT_TRY {
            Node *copy = new Node(original->t);
            original = original->n;
            copy->n = e;
            copy->p = e->p;
            copy->p->n = copy;
            e->p = copy;
        } QT_CATCH(...) {
            // restore the original list
            while (n++<d->size)
                removeLast();
            QT_RETHROW;
        }
    }
    return *this;
}

template <typename T>
QLinkedList<T> QLinkedList<T>::operator+(const QLinkedList<T> &l) const
{
    QLinkedList<T> n = *this;
    n += l;
    return n;
}

Q_DECLARE_SEQUENTIAL_ITERATOR(LinkedList)
Q_DECLARE_MUTABLE_SEQUENTIAL_ITERATOR(LinkedList)

QT_END_NAMESPACE

QT_END_HEADER

#endif // QLINKEDLIST_H
