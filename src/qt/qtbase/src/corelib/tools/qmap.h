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

#ifndef QMAP_H
#define QMAP_H

#include <QtCore/qiterator.h>
#include <QtCore/qlist.h>
#include <QtCore/qrefcount.h>
#include <QtCore/qpair.h>

#ifdef Q_MAP_DEBUG
#include <QtCore/qdebug.h>
#endif

#include <map>
#include <new>

#ifdef Q_COMPILER_INITIALIZER_LISTS
#include <initializer_list>
#endif

QT_BEGIN_NAMESPACE

/*
    QMap uses qMapLessThanKey() to compare keys. The default
    implementation uses operator<(). For pointer types,
    qMapLessThanKey() casts the pointers to integers before it
    compares them, because operator<() is undefined on pointers
    that come from different memory blocks. (In practice, this
    is only a problem when running a program such as
    BoundsChecker.)
*/

template <class Key> inline bool qMapLessThanKey(const Key &key1, const Key &key2)
{
    return key1 < key2;
}

template <class Ptr> inline bool qMapLessThanKey(const Ptr *key1, const Ptr *key2)
{
    Q_STATIC_ASSERT(sizeof(quintptr) == sizeof(const Ptr *));
    return quintptr(key1) < quintptr(key2);
}

struct QMapDataBase;
template <class Key, class T> struct QMapData;

struct Q_CORE_EXPORT QMapNodeBase
{
    quintptr p;
    QMapNodeBase *left;
    QMapNodeBase *right;

    enum Color { Red = 0, Black = 1 };
    enum { Mask = 3 }; // reserve the second bit as well

    const QMapNodeBase *nextNode() const;
    QMapNodeBase *nextNode() { return const_cast<QMapNodeBase *>(const_cast<const QMapNodeBase *>(this)->nextNode()); }
    const QMapNodeBase *previousNode() const;
    QMapNodeBase *previousNode() { return const_cast<QMapNodeBase *>(const_cast<const QMapNodeBase *>(this)->previousNode()); }

    Color color() const { return Color(p & 1); }
    void setColor(Color c) { if (c == Black) p |= Black; else p &= ~Black; }
    QMapNodeBase *parent() const { return reinterpret_cast<QMapNodeBase *>(p & ~Mask); }
    void setParent(QMapNodeBase *pp) { p = (p & Mask) | quintptr(pp); }
};

template <class Key, class T>
struct QMapNode : public QMapNodeBase
{
    Key key;
    T value;

    inline QMapNode *leftNode() const { return static_cast<QMapNode *>(left); }
    inline QMapNode *rightNode() const { return static_cast<QMapNode *>(right); }

    inline const QMapNode *nextNode() const { return static_cast<const QMapNode *>(QMapNodeBase::nextNode()); }
    inline const QMapNode *previousNode() const { return static_cast<const QMapNode *>(QMapNodeBase::previousNode()); }
    inline QMapNode *nextNode() { return static_cast<QMapNode *>(QMapNodeBase::nextNode()); }
    inline QMapNode *previousNode() { return static_cast<QMapNode *>(QMapNodeBase::previousNode()); }

    QMapNode<Key, T> *copy(QMapData<Key, T> *d) const;

    void destroySubTree();

    QMapNode<Key, T> *lowerBound(const Key &key);
    QMapNode<Key, T> *upperBound(const Key &key);

private:
    QMapNode() Q_DECL_EQ_DELETE;
    Q_DISABLE_COPY(QMapNode)
};

template <class Key, class T>
inline QMapNode<Key, T> *QMapNode<Key, T>::lowerBound(const Key &akey)
{
    QMapNode<Key, T> *n = this;
    QMapNode<Key, T> *lastNode = 0;
    while (n) {
        if (!qMapLessThanKey(n->key, akey)) {
            lastNode = n;
            n = n->leftNode();
        } else {
            n = n->rightNode();
        }
    }
    return lastNode;
}

template <class Key, class T>
inline QMapNode<Key, T> *QMapNode<Key, T>::upperBound(const Key &akey)
{
    QMapNode<Key, T> *n = this;
    QMapNode<Key, T> *lastNode = 0;
    while (n) {
        if (qMapLessThanKey(akey, n->key)) {
            lastNode = n;
            n = n->leftNode();
        } else {
            n = n->rightNode();
        }
    }
    return lastNode;
}



struct Q_CORE_EXPORT QMapDataBase
{
    QtPrivate::RefCount ref;
    int size;
    QMapNodeBase header;
    QMapNodeBase *mostLeftNode;

    void rotateLeft(QMapNodeBase *x);
    void rotateRight(QMapNodeBase *x);
    void rebalance(QMapNodeBase *x);
    void freeNodeAndRebalance(QMapNodeBase *z);
    void recalcMostLeftNode();

    QMapNodeBase *createNode(int size, int alignment, QMapNodeBase *parent, bool left);
    void freeTree(QMapNodeBase *root, int alignment);

    static const QMapDataBase shared_null;

    static QMapDataBase *createData();
    static void freeData(QMapDataBase *d);
};

template <class Key, class T>
struct QMapData : public QMapDataBase
{
    typedef QMapNode<Key, T> Node;

    Node *root() const { return static_cast<Node *>(header.left); }

    const Node *end() const { return static_cast<const Node *>(&header); }
    Node *end() { return static_cast<Node *>(&header); }
    const Node *begin() const { if (root()) return static_cast<const Node*>(mostLeftNode); return end(); }
    Node *begin() { if (root()) return static_cast<Node*>(mostLeftNode); return end(); }

    void deleteNode(Node *z);
    Node *findNode(const Key &akey) const;
    void nodeRange(const Key &akey, Node **firstNode, Node **lastNode);

    Node *createNode(const Key &k, const T &v, Node *parent = 0, bool left = false)
    {
        Node *n = static_cast<Node *>(QMapDataBase::createNode(sizeof(Node), Q_ALIGNOF(Node),
                                      parent, left));
        QT_TRY {
            new (&n->key) Key(k);
            QT_TRY {
                new (&n->value) T(v);
            } QT_CATCH(...) {
                n->key.~Key();
                QT_RETHROW;
            }
        } QT_CATCH(...) {
            QMapDataBase::freeNodeAndRebalance(n);
            QT_RETHROW;
        }
        return n;
    }

    static QMapData *create() {
        return static_cast<QMapData *>(createData());
    }

    void destroy() {
        if (root()) {
            root()->destroySubTree();
            freeTree(header.left, Q_ALIGNOF(Node));
        }
        freeData(this);
    }
};

template <class Key, class T>
QMapNode<Key, T> *QMapNode<Key, T>::copy(QMapData<Key, T> *d) const
{
    QMapNode<Key, T> *n = d->createNode(key, value);
    n->setColor(color());
    if (left) {
        n->left = leftNode()->copy(d);
        n->left->setParent(n);
    } else {
        n->left = 0;
    }
    if (right) {
        n->right = rightNode()->copy(d);
        n->right->setParent(n);
    } else {
        n->right = 0;
    }
    return n;
}

#if defined(Q_CC_MSVC)
#pragma warning( push )
#pragma warning( disable : 4127 ) // conditional expression is constant
#endif

template <class Key, class T>
void QMapNode<Key, T>::destroySubTree()
{
    if (QTypeInfo<Key>::isComplex)
        key.~Key();
    if (QTypeInfo<T>::isComplex)
        value.~T();
    if (QTypeInfo<Key>::isComplex || QTypeInfo<T>::isComplex) {
        if (left)
            leftNode()->destroySubTree();
        if (right)
            rightNode()->destroySubTree();
    }
}

#if defined(Q_CC_MSVC)
#pragma warning( pop )
#endif

template <class Key, class T>
void QMapData<Key, T>::deleteNode(QMapNode<Key, T> *z)
{
    if (QTypeInfo<Key>::isComplex)
        z->key.~Key();
    if (QTypeInfo<T>::isComplex)
        z->value.~T();
    freeNodeAndRebalance(z);
}

template <class Key, class T>
QMapNode<Key, T> *QMapData<Key, T>::findNode(const Key &akey) const
{
    if (Node *r = root()) {
        Node *lb = r->lowerBound(akey);
        if (lb && !qMapLessThanKey(akey, lb->key))
            return lb;
    }
    return 0;
}


template <class Key, class T>
void QMapData<Key, T>::nodeRange(const Key &akey, QMapNode<Key, T> **firstNode, QMapNode<Key, T> **lastNode)
{
    Node *n = root();
    Node *l = end();
    while (n) {
        if (qMapLessThanKey(akey, n->key)) {
            l = n;
            n = n->leftNode();
        } else if (qMapLessThanKey(n->key, akey)) {
            n = n->rightNode();
        } else {
            *firstNode = n->leftNode() ? n->leftNode()->lowerBound(akey) : 0;
            if (!*firstNode)
                *firstNode = n;
            *lastNode = n->rightNode() ? n->rightNode()->upperBound(akey) : 0;
            if (!*lastNode)
                *lastNode = l;
            return;
        }
    }
    *firstNode = *lastNode = l;
}


template <class Key, class T>
class QMap
{
    typedef QMapNode<Key, T> Node;

    QMapData<Key, T> *d;

public:
    inline QMap() : d(static_cast<QMapData<Key, T> *>(const_cast<QMapDataBase *>(&QMapDataBase::shared_null))) { }
#ifdef Q_COMPILER_INITIALIZER_LISTS
    inline QMap(std::initializer_list<std::pair<Key,T> > list)
        : d(static_cast<QMapData<Key, T> *>(const_cast<QMapDataBase *>(&QMapDataBase::shared_null)))
    {
        for (typename std::initializer_list<std::pair<Key,T> >::const_iterator it = list.begin(); it != list.end(); ++it)
            insert(it->first, it->second);
    }
#endif
    QMap(const QMap<Key, T> &other);

    inline ~QMap() { if (!d->ref.deref()) d->destroy(); }

    QMap<Key, T> &operator=(const QMap<Key, T> &other);
#ifdef Q_COMPILER_RVALUE_REFS
    inline QMap(QMap<Key, T> &&other)
        : d(other.d)
    {
        other.d = static_cast<QMapData<Key, T> *>(
                const_cast<QMapDataBase *>(&QMapDataBase::shared_null));
    }

    inline QMap<Key, T> &operator=(QMap<Key, T> &&other)
    { qSwap(d, other.d); return *this; }
#endif
    inline void swap(QMap<Key, T> &other) { qSwap(d, other.d); }
    explicit QMap(const typename std::map<Key, T> &other);
    std::map<Key, T> toStdMap() const;

    bool operator==(const QMap<Key, T> &other) const;
    inline bool operator!=(const QMap<Key, T> &other) const { return !(*this == other); }

    inline int size() const { return d->size; }

    inline bool isEmpty() const { return d->size == 0; }

    inline void detach() { if (d->ref.isShared()) detach_helper(); }
    inline bool isDetached() const { return !d->ref.isShared(); }
#if QT_SUPPORTS(UNSHARABLE_CONTAINERS)
    inline void setSharable(bool sharable)
    {
        if (sharable == d->ref.isSharable())
            return;
        if (!sharable)
            detach();
        // Don't call on shared_null
        d->ref.setSharable(sharable);
    }
#endif
    inline bool isSharedWith(const QMap<Key, T> &other) const { return d == other.d; }

    void clear();

    int remove(const Key &key);
    T take(const Key &key);

    bool contains(const Key &key) const;
    const Key key(const T &value, const Key &defaultKey = Key()) const;
    const T value(const Key &key, const T &defaultValue = T()) const;
    T &operator[](const Key &key);
    const T operator[](const Key &key) const;

    QList<Key> uniqueKeys() const;
    QList<Key> keys() const;
    QList<Key> keys(const T &value) const;
    QList<T> values() const;
    QList<T> values(const Key &key) const;
    int count(const Key &key) const;

    inline const Key &firstKey() const { Q_ASSERT(!isEmpty()); return constBegin().key(); }
    inline const Key &lastKey() const { Q_ASSERT(!isEmpty()); return (constEnd() - 1).key(); }

    inline T &first() { Q_ASSERT(!isEmpty()); return *begin(); }
    inline const T &first() const { Q_ASSERT(!isEmpty()); return *constBegin(); }
    inline T &last() { Q_ASSERT(!isEmpty()); return *(end() - 1); }
    inline const T &last() const { Q_ASSERT(!isEmpty()); return *(constEnd() - 1); }

    class const_iterator;

    class iterator
    {
        friend class const_iterator;
        Node *i;

    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef qptrdiff difference_type;
        typedef T value_type;
        typedef T *pointer;
        typedef T &reference;

        inline iterator() : i(0) { }
        inline iterator(Node *node) : i(node) { }

        inline const Key &key() const { return i->key; }
        inline T &value() const { return i->value; }
        inline T &operator*() const { return i->value; }
        inline T *operator->() const { return &i->value; }
        inline bool operator==(const iterator &o) const { return i == o.i; }
        inline bool operator!=(const iterator &o) const { return i != o.i; }

        inline iterator &operator++() {
            i = i->nextNode();
            return *this;
        }
        inline iterator operator++(int) {
            iterator r = *this;
            i = i->nextNode();
            return r;
        }
        inline iterator &operator--() {
            i = i->previousNode();
            return *this;
        }
        inline iterator operator--(int) {
            iterator r = *this;
            i = i->previousNode();
            return r;
        }
        inline iterator operator+(int j) const
        { iterator r = *this; if (j > 0) while (j--) ++r; else while (j++) --r; return r; }
        inline iterator operator-(int j) const { return operator+(-j); }
        inline iterator &operator+=(int j) { return *this = *this + j; }
        inline iterator &operator-=(int j) { return *this = *this - j; }

#ifndef QT_STRICT_ITERATORS
    public:
        inline bool operator==(const const_iterator &o) const
            { return i == o.i; }
        inline bool operator!=(const const_iterator &o) const
            { return i != o.i; }
#endif
        friend class QMap<Key, T>;
    };
    friend class iterator;

    class const_iterator
    {
        friend class iterator;
        const Node *i;

    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef qptrdiff difference_type;
        typedef T value_type;
        typedef const T *pointer;
        typedef const T &reference;

        inline const_iterator() : i(0) { }
        inline const_iterator(const Node *node) : i(node) { }
#ifdef QT_STRICT_ITERATORS
        explicit inline const_iterator(const iterator &o)
#else
        inline const_iterator(const iterator &o)
#endif
        { i = o.i; }

        inline const Key &key() const { return i->key; }
        inline const T &value() const { return i->value; }
        inline const T &operator*() const { return i->value; }
        inline const T *operator->() const { return &i->value; }
        inline bool operator==(const const_iterator &o) const { return i == o.i; }
        inline bool operator!=(const const_iterator &o) const { return i != o.i; }

        inline const_iterator &operator++() {
            i = i->nextNode();
            return *this;
        }
        inline const_iterator operator++(int) {
            const_iterator r = *this;
            i = i->nextNode();
            return r;
        }
        inline const_iterator &operator--() {
            i = i->previousNode();
            return *this;
        }
        inline const_iterator operator--(int) {
            const_iterator r = *this;
            i = i->previousNode();
            return r;
        }
        inline const_iterator operator+(int j) const
        { const_iterator r = *this; if (j > 0) while (j--) ++r; else while (j++) --r; return r; }
        inline const_iterator operator-(int j) const { return operator+(-j); }
        inline const_iterator &operator+=(int j) { return *this = *this + j; }
        inline const_iterator &operator-=(int j) { return *this = *this - j; }

#ifdef QT_STRICT_ITERATORS
    private:
        inline bool operator==(const iterator &o) const { return operator==(const_iterator(o)); }
        inline bool operator!=(const iterator &o) const { return operator!=(const_iterator(o)); }
#endif
        friend class QMap<Key, T>;
    };
    friend class const_iterator;

    // STL style
    inline iterator begin() { detach(); return iterator(d->begin()); }
    inline const_iterator begin() const { return const_iterator(d->begin()); }
    inline const_iterator constBegin() const { return const_iterator(d->begin()); }
    inline const_iterator cbegin() const { return const_iterator(d->begin()); }
    inline iterator end() { detach(); return iterator(d->end()); }
    inline const_iterator end() const { return const_iterator(d->end()); }
    inline const_iterator constEnd() const { return const_iterator(d->end()); }
    inline const_iterator cend() const { return const_iterator(d->end()); }
    iterator erase(iterator it);

    // more Qt
    typedef iterator Iterator;
    typedef const_iterator ConstIterator;
    inline int count() const { return d->size; }
    iterator find(const Key &key);
    const_iterator find(const Key &key) const;
    const_iterator constFind(const Key &key) const;
    iterator lowerBound(const Key &key);
    const_iterator lowerBound(const Key &key) const;
    iterator upperBound(const Key &key);
    const_iterator upperBound(const Key &key) const;
    iterator insert(const Key &key, const T &value);
    iterator insert(const_iterator pos, const Key &key, const T &value);
    iterator insertMulti(const Key &key, const T &value);
    iterator insertMulti(const_iterator pos, const Key &akey, const T &avalue);
    QMap<Key, T> &unite(const QMap<Key, T> &other);

    // STL compatibility
    typedef Key key_type;
    typedef T mapped_type;
    typedef qptrdiff difference_type;
    typedef int size_type;
    inline bool empty() const { return isEmpty(); }
    QPair<iterator, iterator> equal_range(const Key &akey);

#ifdef Q_MAP_DEBUG
    void dump() const;
#endif

private:
    void detach_helper();
    bool isValidIterator(const const_iterator &ci) const
    {
#if defined(QT_DEBUG) && !defined(Q_MAP_NO_ITERATOR_DEBUG)
        const QMapNodeBase *n = ci.i;
        while (n->parent())
            n = n->parent();
        return n->left == d->root();
#else
        Q_UNUSED(ci);
        return true;
#endif
    }
};

template <class Key, class T>
inline QMap<Key, T>::QMap(const QMap<Key, T> &other)
{
    if (other.d->ref.ref()) {
        d = other.d;
    } else {
        d = QMapData<Key, T>::create();
        if (other.d->header.left) {
            d->header.left = static_cast<Node *>(other.d->header.left)->copy(d);
            d->header.left->setParent(&d->header);
            d->recalcMostLeftNode();
        }
    }
}

template <class Key, class T>
Q_INLINE_TEMPLATE QMap<Key, T> &QMap<Key, T>::operator=(const QMap<Key, T> &other)
{
    if (d != other.d) {
        QMap<Key, T> tmp(other);
        tmp.swap(*this);
    }
    return *this;
}

template <class Key, class T>
Q_INLINE_TEMPLATE void QMap<Key, T>::clear()
{
    *this = QMap<Key, T>();
}


template <class Key, class T>
Q_INLINE_TEMPLATE const T QMap<Key, T>::value(const Key &akey, const T &adefaultValue) const
{
    Node *n = d->findNode(akey);
    return n ? n->value : adefaultValue;
}

template <class Key, class T>
Q_INLINE_TEMPLATE const T QMap<Key, T>::operator[](const Key &akey) const
{
    return value(akey);
}

template <class Key, class T>
Q_INLINE_TEMPLATE T &QMap<Key, T>::operator[](const Key &akey)
{
    detach();
    Node *n = d->findNode(akey);
    if (!n)
        return *insert(akey, T());
    return n->value;
}

template <class Key, class T>
Q_INLINE_TEMPLATE int QMap<Key, T>::count(const Key &akey) const
{
    Node *firstNode;
    Node *lastNode;
    d->nodeRange(akey, &firstNode, &lastNode);

    const_iterator ci_first(firstNode);
    const const_iterator ci_last(lastNode);
    int cnt = 0;
    while (ci_first != ci_last) {
        ++cnt;
        ++ci_first;
    }
    return cnt;
}

template <class Key, class T>
Q_INLINE_TEMPLATE bool QMap<Key, T>::contains(const Key &akey) const
{
    return d->findNode(akey) != 0;
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename QMap<Key, T>::iterator QMap<Key, T>::insert(const Key &akey, const T &avalue)
{
    detach();
    Node *n = d->root();
    Node *y = d->end();
    Node *lastNode = 0;
    bool  left = true;
    while (n) {
        y = n;
        if (!qMapLessThanKey(n->key, akey)) {
            lastNode = n;
            left = true;
            n = n->leftNode();
        } else {
            left = false;
            n = n->rightNode();
        }
    }
    if (lastNode && !qMapLessThanKey(akey, lastNode->key)) {
        lastNode->value = avalue;
        return iterator(lastNode);
    }
    Node *z = d->createNode(akey, avalue, y, left);
    return iterator(z);
}

template <class Key, class T>
typename QMap<Key, T>::iterator QMap<Key, T>::insert(const_iterator pos, const Key &akey, const T &avalue)
{
    if (d->ref.isShared())
        return this->insert(akey, avalue);

    Q_ASSERT_X(isValidIterator(pos), "QMap::insert", "The specified const_iterator argument 'it' is invalid");

    if (pos == constEnd()) {
        // Hint is that the Node is larger than (or equal to) the largest value.
        Node *n = static_cast<Node *>(pos.i->left);
        if (n) {
            while (n->right)
                n = static_cast<Node *>(n->right);

            if (!qMapLessThanKey(n->key, akey))
                return this->insert(akey, avalue); // ignore hint
            // This can be optimized by checking equal too.
            // we can overwrite if previous node key is strictly smaller
            // (or there is no previous node)

            Node *z = d->createNode(akey, avalue, n, false); // insert right most
            return iterator(z);
        }
        return this->insert(akey, avalue);
    } else {
        // Hint indicates that the node should be less (or equal to) the hint given
        // but larger than the previous value.
        Node *next = const_cast<Node*>(pos.i);
        if (qMapLessThanKey(next->key, akey))
            return this->insert(akey, avalue); // ignore hint

        if (pos == constBegin()) {
            // There is no previous value
            // Maybe overwrite left most value
            if (!qMapLessThanKey(akey, next->key)) {
                next->value = avalue; // overwrite current iterator
                return iterator(next);
            }
            // insert left most.
            Node *z = d->createNode(akey, avalue, begin().i, true);
            return iterator(z);
        } else {
            Node *prev = const_cast<Node*>(pos.i->previousNode());
            if (!qMapLessThanKey(prev->key, akey)) {
                return this->insert(akey, avalue); // ignore hint
            }
            // Hint is ok
            if (!qMapLessThanKey(akey, next->key)) {
                next->value = avalue; // overwrite current iterator
                return iterator(next);
            }

            // we need to insert (not overwrite)
            if (prev->right == 0) {
                Node *z = d->createNode(akey, avalue, prev, false);
                return iterator(z);
            }
            if (next->left == 0) {
                Node *z = d->createNode(akey, avalue, next, true);
                return iterator(z);
            }
            Q_ASSERT(false); // We should have prev->right == 0 or next->left == 0.
            return this->insert(akey, avalue);
        }
    }
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename QMap<Key, T>::iterator QMap<Key, T>::insertMulti(const Key &akey,
                                                                            const T &avalue)
{
    detach();
    Node* y = d->end();
    Node* x = static_cast<Node *>(d->root());
    bool left = true;
    while (x != 0) {
        left = !qMapLessThanKey(x->key, akey);
        y = x;
        x = left ? x->leftNode() : x->rightNode();
    }
    Node *z = d->createNode(akey, avalue, y, left);
    return iterator(z);
}

template <class Key, class T>
typename QMap<Key, T>::iterator QMap<Key, T>::insertMulti(const_iterator pos, const Key &akey, const T &avalue)
{
    if (d->ref.isShared())
        return this->insertMulti(akey, avalue);

    Q_ASSERT_X(isValidIterator(pos), "QMap::insertMulti", "The specified const_iterator argument 'pos' is invalid");

    if (pos == constEnd()) {
        // Hint is that the Node is larger than (or equal to) the largest value.
        Node *n = static_cast<Node *>(pos.i->left);
        if (n) {
            while (n->right)
                n = static_cast<Node *>(n->right);

            if (!qMapLessThanKey(n->key, akey))
                return this->insertMulti(akey, avalue); // ignore hint
            Node *z = d->createNode(akey, avalue, n, false); // insert right most
            return iterator(z);
        }
        return this->insertMulti(akey, avalue);
    } else {
        // Hint indicates that the node should be less (or equal to) the hint given
        // but larger than the previous value.
        Node *next = const_cast<Node*>(pos.i);
        if (qMapLessThanKey(next->key, akey))
            return this->insertMulti(akey, avalue); // ignore hint

        if (pos == constBegin()) {
            // There is no previous value (insert left most)
            Node *z = d->createNode(akey, avalue, begin().i, true);
            return iterator(z);
        } else {
            Node *prev = const_cast<Node*>(pos.i->previousNode());
            if (!qMapLessThanKey(prev->key, akey))
                return this->insertMulti(akey, avalue); // ignore hint

            // Hint is ok - do insert
            if (prev->right == 0) {
                Node *z = d->createNode(akey, avalue, prev, false);
                return iterator(z);
            }
            if (next->left == 0) {
                Node *z = d->createNode(akey, avalue, next, true);
                return iterator(z);
            }
            Q_ASSERT(false); // We should have prev->right == 0 or next->left == 0.
            return this->insertMulti(akey, avalue);
        }
    }
}


template <class Key, class T>
Q_INLINE_TEMPLATE typename QMap<Key, T>::const_iterator QMap<Key, T>::constFind(const Key &akey) const
{
    Node *n = d->findNode(akey);
    return const_iterator(n ? n : d->end());
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename QMap<Key, T>::const_iterator QMap<Key, T>::find(const Key &akey) const
{
    return constFind(akey);
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename QMap<Key, T>::iterator QMap<Key, T>::find(const Key &akey)
{
    detach();
    Node *n = d->findNode(akey);
    return iterator(n ? n : d->end());
}

template <class Key, class T>
Q_INLINE_TEMPLATE QMap<Key, T> &QMap<Key, T>::unite(const QMap<Key, T> &other)
{
    QMap<Key, T> copy(other);
    const_iterator it = copy.constEnd();
    const const_iterator b = copy.constBegin();
    while (it != b) {
        --it;
        insertMulti(it.key(), it.value());
    }
    return *this;
}

template <class Key, class T>
QPair<typename QMap<Key, T>::iterator, typename QMap<Key, T>::iterator> QMap<Key, T>::equal_range(const Key &akey)
{
    detach();
    Node *firstNode, *lastNode;
    d->nodeRange(akey, &firstNode, &lastNode);
    return QPair<iterator, iterator>(iterator(firstNode), iterator(lastNode));
}

#ifdef Q_MAP_DEBUG
template <class Key, class T>
void QMap<Key, T>::dump() const
{
    const_iterator it = begin();
    qDebug() << "map dump:";
    while (it != end()) {
        const QMapNodeBase *n = it.i;
        int depth = 0;
        while (n && n != d->root()) {
            ++depth;
            n = n->parent();
        }
        QByteArray space(4*depth, ' ');
        qDebug() << space << (it.i->color() == Node::Red ? "Red  " : "Black") << it.i << it.i->left << it.i->right
                 << it.key() << it.value();
        ++it;
    }
    qDebug() << "---------";
}
#endif

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE int QMap<Key, T>::remove(const Key &akey)
{
    detach();
    int n = 0;
    while (Node *node = d->findNode(akey)) {
        d->deleteNode(node);
        ++n;
    }
    return n;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE T QMap<Key, T>::take(const Key &akey)
{
    detach();

    Node *node = d->findNode(akey);
    if (node) {
        T t = node->value;
        d->deleteNode(node);
        return t;
    }
    return T();
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE typename QMap<Key, T>::iterator QMap<Key, T>::erase(iterator it)
{
    if (it == iterator(d->end()))
        return it;

    Q_ASSERT_X(isValidIterator(const_iterator(it)), "QMap::erase", "The specified iterator argument 'it' is invalid");

    if (d->ref.isShared()) {
        const_iterator oldBegin = constBegin();
        const_iterator old = const_iterator(it);
        int backStepsWithSameKey = 0;

        while (old != oldBegin) {
            --old;
            if (qMapLessThanKey(old.key(), it.key()))
                break;
            ++backStepsWithSameKey;
        }

        it = find(old.key()); // ensures detach
        Q_ASSERT_X(it != iterator(d->end()), "QMap::erase", "Unable to locate same key in erase after detach.");

        while (backStepsWithSameKey > 0) {
            ++it;
            --backStepsWithSameKey;
        }
    }

    Node *n = it.i;
    ++it;
    d->deleteNode(n);
    return it;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE void QMap<Key, T>::detach_helper()
{
    QMapData<Key, T> *x = QMapData<Key, T>::create();
    if (d->header.left) {
        x->header.left = static_cast<Node *>(d->header.left)->copy(x);
        x->header.left->setParent(&x->header);
    }
    if (!d->ref.deref())
        d->destroy();
    d = x;
    d->recalcMostLeftNode();
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE QList<Key> QMap<Key, T>::uniqueKeys() const
{
    QList<Key> res;
    res.reserve(size()); // May be too much, but assume short lifetime
    const_iterator i = begin();
    if (i != end()) {
        for (;;) {
            const Key &aKey = i.key();
            res.append(aKey);
            do {
                if (++i == end())
                    goto break_out_of_outer_loop;
            } while (!qMapLessThanKey(aKey, i.key()));   // loop while (key == i.key())
        }
    }
break_out_of_outer_loop:
    return res;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE QList<Key> QMap<Key, T>::keys() const
{
    QList<Key> res;
    res.reserve(size());
    const_iterator i = begin();
    while (i != end()) {
        res.append(i.key());
        ++i;
    }
    return res;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE QList<Key> QMap<Key, T>::keys(const T &avalue) const
{
    QList<Key> res;
    const_iterator i = begin();
    while (i != end()) {
        if (i.value() == avalue)
            res.append(i.key());
        ++i;
    }
    return res;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE const Key QMap<Key, T>::key(const T &avalue, const Key &defaultKey) const
{
    const_iterator i = begin();
    while (i != end()) {
        if (i.value() == avalue)
            return i.key();
        ++i;
    }

    return defaultKey;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE QList<T> QMap<Key, T>::values() const
{
    QList<T> res;
    res.reserve(size());
    const_iterator i = begin();
    while (i != end()) {
        res.append(i.value());
        ++i;
    }
    return res;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE QList<T> QMap<Key, T>::values(const Key &akey) const
{
    QList<T> res;
    Node *n = d->findNode(akey);
    if (n) {
        const_iterator it(n);
        do {
            res.append(*it);
            ++it;
        } while (it != constEnd() && !qMapLessThanKey<Key>(akey, it.key()));
    }
    return res;
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename QMap<Key, T>::const_iterator QMap<Key, T>::lowerBound(const Key &akey) const
{
    Node *lb = d->root() ? d->root()->lowerBound(akey) : 0;
    if (!lb)
        lb = d->end();
    return const_iterator(lb);
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename QMap<Key, T>::iterator QMap<Key, T>::lowerBound(const Key &akey)
{
    detach();
    Node *lb = d->root() ? d->root()->lowerBound(akey) : 0;
    if (!lb)
        lb = d->end();
    return iterator(lb);
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename QMap<Key, T>::const_iterator
QMap<Key, T>::upperBound(const Key &akey) const
{
    Node *ub = d->root() ? d->root()->upperBound(akey) : 0;
    if (!ub)
        ub = d->end();
    return const_iterator(ub);
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename QMap<Key, T>::iterator QMap<Key, T>::upperBound(const Key &akey)
{
    detach();
    Node *ub = d->root() ? d->root()->upperBound(akey) : 0;
    if (!ub)
        ub = d->end();
    return iterator(ub);
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE bool QMap<Key, T>::operator==(const QMap<Key, T> &other) const
{
    if (size() != other.size())
        return false;
    if (d == other.d)
        return true;

    const_iterator it1 = begin();
    const_iterator it2 = other.begin();

    while (it1 != end()) {
        if (!(it1.value() == it2.value()) || qMapLessThanKey(it1.key(), it2.key()) || qMapLessThanKey(it2.key(), it1.key()))
            return false;
        ++it2;
        ++it1;
    }
    return true;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE QMap<Key, T>::QMap(const std::map<Key, T> &other)
{
    d = QMapData<Key, T>::create();
    typename std::map<Key,T>::const_iterator it = other.end();
    while (it != other.begin()) {
        --it;
        d->createNode((*it).first, (*it).second, d->begin(), true); // insert on most left node.
    }
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE std::map<Key, T> QMap<Key, T>::toStdMap() const
{
    std::map<Key, T> map;
    const_iterator it = end();
    while (it != begin()) {
        --it;
        map.insert(map.begin(), std::pair<Key, T>(it.key(), it.value()));
    }
    return map;
}

template <class Key, class T>
class QMultiMap : public QMap<Key, T>
{
public:
    QMultiMap() {}
#ifdef Q_COMPILER_INITIALIZER_LISTS
    inline QMultiMap(std::initializer_list<std::pair<Key,T> > list)
    {
        for (typename std::initializer_list<std::pair<Key,T> >::const_iterator it = list.begin(); it != list.end(); ++it)
            insert(it->first, it->second);
    }
#endif
    QMultiMap(const QMap<Key, T> &other) : QMap<Key, T>(other) {}
    inline void swap(QMultiMap<Key, T> &other) { QMap<Key, T>::swap(other); }

    inline typename QMap<Key, T>::iterator replace(const Key &key, const T &value)
    { return QMap<Key, T>::insert(key, value); }
    inline typename QMap<Key, T>::iterator insert(const Key &key, const T &value)
    { return QMap<Key, T>::insertMulti(key, value); }
    inline typename QMap<Key, T>::iterator insert(typename QMap<Key, T>::const_iterator pos, const Key &key, const T &value)
    { return QMap<Key, T>::insertMulti(pos, key, value); }

    inline QMultiMap &operator+=(const QMultiMap &other)
    { this->unite(other); return *this; }
    inline QMultiMap operator+(const QMultiMap &other) const
    { QMultiMap result = *this; result += other; return result; }

#if !defined(Q_NO_USING_KEYWORD) && !defined(Q_CC_RVCT)
    // RVCT compiler doesn't handle using-keyword right when used functions are overloaded in child class
    using QMap<Key, T>::contains;
    using QMap<Key, T>::remove;
    using QMap<Key, T>::count;
    using QMap<Key, T>::find;
    using QMap<Key, T>::constFind;
#else
    inline bool contains(const Key &key) const
    { return QMap<Key, T>::contains(key); }
    inline int remove(const Key &key)
    { return QMap<Key, T>::remove(key); }
    inline int count(const Key &key) const
    { return QMap<Key, T>::count(key); }
    inline int count() const
    { return QMap<Key, T>::count(); }
    inline typename QMap<Key, T>::iterator find(const Key &key)
    { return QMap<Key, T>::find(key); }
    inline typename QMap<Key, T>::const_iterator find(const Key &key) const
    { return QMap<Key, T>::find(key); }
    inline typename QMap<Key, T>::const_iterator constFind(const Key &key) const
    { return QMap<Key, T>::constFind(key); }
#endif

    bool contains(const Key &key, const T &value) const;

    int remove(const Key &key, const T &value);

    int count(const Key &key, const T &value) const;

    typename QMap<Key, T>::iterator find(const Key &key, const T &value) {
        typename QMap<Key, T>::iterator i(find(key));
        typename QMap<Key, T>::iterator end(this->end());
        while (i != end && !qMapLessThanKey<Key>(key, i.key())) {
            if (i.value() == value)
                return i;
            ++i;
        }
        return end;
    }
    typename QMap<Key, T>::const_iterator find(const Key &key, const T &value) const {
        typename QMap<Key, T>::const_iterator i(constFind(key));
        typename QMap<Key, T>::const_iterator end(QMap<Key, T>::constEnd());
        while (i != end && !qMapLessThanKey<Key>(key, i.key())) {
            if (i.value() == value)
                return i;
            ++i;
        }
        return end;
    }
    typename QMap<Key, T>::const_iterator constFind(const Key &key, const T &value) const
        { return find(key, value); }
private:
    T &operator[](const Key &key);
    const T operator[](const Key &key) const;
};

template <class Key, class T>
Q_INLINE_TEMPLATE bool QMultiMap<Key, T>::contains(const Key &key, const T &value) const
{
    return constFind(key, value) != QMap<Key, T>::constEnd();
}

template <class Key, class T>
Q_INLINE_TEMPLATE int QMultiMap<Key, T>::remove(const Key &key, const T &value)
{
    int n = 0;
    typename QMap<Key, T>::iterator i(find(key));
    typename QMap<Key, T>::iterator end(QMap<Key, T>::end());
    while (i != end && !qMapLessThanKey<Key>(key, i.key())) {
        if (i.value() == value) {
            i = this->erase(i);
            ++n;
        } else {
            ++i;
        }
    }
    return n;
}

template <class Key, class T>
Q_INLINE_TEMPLATE int QMultiMap<Key, T>::count(const Key &key, const T &value) const
{
    int n = 0;
    typename QMap<Key, T>::const_iterator i(constFind(key));
    typename QMap<Key, T>::const_iterator end(QMap<Key, T>::constEnd());
    while (i != end && !qMapLessThanKey<Key>(key, i.key())) {
        if (i.value() == value)
            ++n;
        ++i;
    }
    return n;
}

Q_DECLARE_ASSOCIATIVE_ITERATOR(Map)
Q_DECLARE_MUTABLE_ASSOCIATIVE_ITERATOR(Map)

QT_END_NAMESPACE

#endif // QMAP_H
