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

#include "qmap.h"

#include <stdlib.h>

#ifdef QT_QMAP_DEBUG
# include <qstring.h>
# include <qvector.h>
#endif

QT_BEGIN_NAMESPACE

const QMapDataBase QMapDataBase::shared_null = { Q_REFCOUNT_INITIALIZE_STATIC, 0, { 0, 0, 0 }, 0 };

const QMapNodeBase *QMapNodeBase::nextNode() const
{
    const QMapNodeBase *n = this;
    if (n->right) {
        n = n->right;
        while (n->left)
            n = n->left;
    } else {
        const QMapNodeBase *y = n->parent();
        while (y && n == y->right) {
            n = y;
            y = n->parent();
        }
        n = y;
    }
    return n;
}

const QMapNodeBase *QMapNodeBase::previousNode() const
{
    const QMapNodeBase *n = this;
    if (n->left) {
        n = n->left;
        while (n->right)
            n = n->right;
    } else {
        const QMapNodeBase *y = n->parent();
        while (y && n == y->left) {
            n = y;
            y = n->parent();
        }
        n = y;
    }
    return n;
}


void QMapDataBase::rotateLeft(QMapNodeBase *x)
{
    QMapNodeBase *&root = header.left;
    QMapNodeBase *y = x->right;
    x->right = y->left;
    if (y->left != 0)
        y->left->setParent(x);
    y->setParent(x->parent());
    if (x == root)
        root = y;
    else if (x == x->parent()->left)
        x->parent()->left = y;
    else
        x->parent()->right = y;
    y->left = x;
    x->setParent(y);
}


void QMapDataBase::rotateRight(QMapNodeBase *x)
{
    QMapNodeBase *&root = header.left;
    QMapNodeBase *y = x->left;
    x->left = y->right;
    if (y->right != 0)
        y->right->setParent(x);
    y->setParent(x->parent());
    if (x == root)
        root = y;
    else if (x == x->parent()->right)
        x->parent()->right = y;
    else
        x->parent()->left = y;
    y->right = x;
    x->setParent(y);
}


void QMapDataBase::rebalance(QMapNodeBase *x)
{
    QMapNodeBase *&root = header.left;
    x->setColor(QMapNodeBase::Red);
    while (x != root && x->parent()->color() == QMapNodeBase::Red) {
        if (x->parent() == x->parent()->parent()->left) {
            QMapNodeBase *y = x->parent()->parent()->right;
            if (y && y->color() == QMapNodeBase::Red) {
                x->parent()->setColor(QMapNodeBase::Black);
                y->setColor(QMapNodeBase::Black);
                x->parent()->parent()->setColor(QMapNodeBase::Red);
                x = x->parent()->parent();
            } else {
                if (x == x->parent()->right) {
                    x = x->parent();
                    rotateLeft(x);
                }
                x->parent()->setColor(QMapNodeBase::Black);
                x->parent()->parent()->setColor(QMapNodeBase::Red);
                rotateRight (x->parent()->parent());
            }
        } else {
            QMapNodeBase *y = x->parent()->parent()->left;
            if (y && y->color() == QMapNodeBase::Red) {
                x->parent()->setColor(QMapNodeBase::Black);
                y->setColor(QMapNodeBase::Black);
                x->parent()->parent()->setColor(QMapNodeBase::Red);
                x = x->parent()->parent();
            } else {
                if (x == x->parent()->left) {
                    x = x->parent();
                    rotateRight(x);
                }
                x->parent()->setColor(QMapNodeBase::Black);
                x->parent()->parent()->setColor(QMapNodeBase::Red);
                rotateLeft(x->parent()->parent());
            }
        }
    }
    root->setColor(QMapNodeBase::Black);
}

void QMapDataBase::freeNodeAndRebalance(QMapNodeBase *z)
{
    QMapNodeBase *&root = header.left;
    QMapNodeBase *y = z;
    QMapNodeBase *x;
    QMapNodeBase *x_parent;
    if (y->left == 0) {
        x = y->right;
        if (y == mostLeftNode) {
            if (x)
                mostLeftNode = x; // It cannot have (left) children due the red black invariant.
            else
                mostLeftNode = y->parent();
        }
    } else {
        if (y->right == 0) {
            x = y->left;
        } else {
            y = y->right;
            while (y->left != 0)
                y = y->left;
            x = y->right;
        }
    }
    if (y != z) {
        z->left->setParent(y);
        y->left = z->left;
        if (y != z->right) {
            x_parent = y->parent();
            if (x)
                x->setParent(y->parent());
            y->parent()->left = x;
            y->right = z->right;
            z->right->setParent(y);
        } else {
            x_parent = y;
        }
        if (root == z)
            root = y;
        else if (z->parent()->left == z)
            z->parent()->left = y;
        else
            z->parent()->right = y;
        y->setParent(z->parent());
        // Swap the colors
        QMapNodeBase::Color c = y->color();
        y->setColor(z->color());
        z->setColor(c);
        y = z;
    } else {
        x_parent = y->parent();
        if (x)
            x->setParent(y->parent());
        if (root == z)
            root = x;
        else if (z->parent()->left == z)
            z->parent()->left = x;
        else
            z->parent()->right = x;
    }
    if (y->color() != QMapNodeBase::Red) {
        while (x != root && (x == 0 || x->color() == QMapNodeBase::Black)) {
            if (x == x_parent->left) {
                QMapNodeBase *w = x_parent->right;
                if (w->color() == QMapNodeBase::Red) {
                    w->setColor(QMapNodeBase::Black);
                    x_parent->setColor(QMapNodeBase::Red);
                    rotateLeft(x_parent);
                    w = x_parent->right;
                }
                if ((w->left == 0 || w->left->color() == QMapNodeBase::Black) &&
                    (w->right == 0 || w->right->color() == QMapNodeBase::Black)) {
                    w->setColor(QMapNodeBase::Red);
                    x = x_parent;
                    x_parent = x_parent->parent();
                } else {
                    if (w->right == 0 || w->right->color() == QMapNodeBase::Black) {
                        if (w->left)
                            w->left->setColor(QMapNodeBase::Black);
                        w->setColor(QMapNodeBase::Red);
                        rotateRight(w);
                        w = x_parent->right;
                    }
                    w->setColor(x_parent->color());
                    x_parent->setColor(QMapNodeBase::Black);
                    if (w->right)
                        w->right->setColor(QMapNodeBase::Black);
                    rotateLeft(x_parent);
                    break;
                }
            } else {
            QMapNodeBase *w = x_parent->left;
            if (w->color() == QMapNodeBase::Red) {
                w->setColor(QMapNodeBase::Black);
                x_parent->setColor(QMapNodeBase::Red);
                rotateRight(x_parent);
                w = x_parent->left;
            }
            if ((w->right == 0 || w->right->color() == QMapNodeBase::Black) &&
                (w->left == 0 || w->left->color() == QMapNodeBase::Black)) {
                w->setColor(QMapNodeBase::Red);
                x = x_parent;
                x_parent = x_parent->parent();
            } else {
                if (w->left == 0 || w->left->color() == QMapNodeBase::Black) {
                    if (w->right)
                        w->right->setColor(QMapNodeBase::Black);
                    w->setColor(QMapNodeBase::Red);
                    rotateLeft(w);
                    w = x_parent->left;
                }
                w->setColor(x_parent->color());
                x_parent->setColor(QMapNodeBase::Black);
                if (w->left)
                    w->left->setColor(QMapNodeBase::Black);
                rotateRight(x_parent);
                break;
            }
        }
    }
    if (x)
        x->setColor(QMapNodeBase::Black);
    }
    free(y);
    --size;
}

void QMapDataBase::recalcMostLeftNode()
{
    mostLeftNode = &header;
    while (mostLeftNode->left)
        mostLeftNode = mostLeftNode->left;
}

static inline int qMapAlignmentThreshold()
{
    // malloc on 32-bit platforms should return pointers that are 8-byte
    // aligned or more while on 64-bit platforms they should be 16-byte aligned
    // or more
    return 2 * sizeof(void*);
}

static inline void *qMapAllocate(int alloc, int alignment)
{
    return alignment > qMapAlignmentThreshold()
        ? qMallocAligned(alloc, alignment)
        : ::malloc(alloc);
}

static inline void qMapDeallocate(QMapNodeBase *node, int alignment)
{
    if (alignment > qMapAlignmentThreshold())
        qFreeAligned(node);
    else
        ::free(node);
}

QMapNodeBase *QMapDataBase::createNode(int alloc, int alignment, QMapNodeBase *parent, bool left)
{
    QMapNodeBase *node = static_cast<QMapNodeBase *>(qMapAllocate(alloc, alignment));
    Q_CHECK_PTR(node);

    memset(node, 0, alloc);
    ++size;

    if (parent) {
        if (left) {
            parent->left = node;
            if (parent == mostLeftNode)
                mostLeftNode = node;
        } else {
            parent->right = node;
        }
        node->setParent(parent);
        rebalance(node);
    }
    return node;
}

void QMapDataBase::freeTree(QMapNodeBase *root, int alignment)
{
    if (root->left)
        freeTree(root->left, alignment);
    if (root->right)
        freeTree(root->right, alignment);
    qMapDeallocate(root, alignment);
}

QMapDataBase *QMapDataBase::createData()
{
    QMapDataBase *d = new QMapDataBase;

    d->ref.initializeOwned();
    d->size = 0;

    d->header.p = 0;
    d->header.left = 0;
    d->header.right = 0;
    d->mostLeftNode = &(d->header);

    return d;
}

void QMapDataBase::freeData(QMapDataBase *d)
{
    delete d;
}

/*!
    \class QMap
    \inmodule QtCore
    \brief The QMap class is a template class that provides a red-black-tree-based dictionary.

    \ingroup tools
    \ingroup shared

    \reentrant

    QMap\<Key, T\> is one of Qt's generic \l{container classes}. It
    stores (key, value) pairs and provides fast lookup of the
    value associated with a key.

    QMap and QHash provide very similar functionality. The
    differences are:

    \list
    \li QHash provides average faster lookups than QMap. (See \l{Algorithmic
       Complexity} for details.)
    \li When iterating over a QHash, the items are arbitrarily ordered.
       With QMap, the items are always sorted by key.
    \li The key type of a QHash must provide operator==() and a global
       qHash(Key) function. The key type of a QMap must provide
       operator<() specifying a total order.
    \endlist

    Here's an example QMap with QString keys and \c int values:
    \snippet code/src_corelib_tools_qmap.cpp 0

    To insert a (key, value) pair into the map, you can use operator[]():

    \snippet code/src_corelib_tools_qmap.cpp 1

    This inserts the following three (key, value) pairs into the
    QMap: ("one", 1), ("three", 3), and ("seven", 7). Another way to
    insert items into the map is to use insert():

    \snippet code/src_corelib_tools_qmap.cpp 2

    To look up a value, use operator[]() or value():

    \snippet code/src_corelib_tools_qmap.cpp 3

    If there is no item with the specified key in the map, these
    functions return a \l{default-constructed value}.

    If you want to check whether the map contains a certain key, use
    contains():

    \snippet code/src_corelib_tools_qmap.cpp 4

    There is also a value() overload that uses its second argument as
    a default value if there is no item with the specified key:

    \snippet code/src_corelib_tools_qmap.cpp 5

    In general, we recommend that you use contains() and value()
    rather than operator[]() for looking up a key in a map. The
    reason is that operator[]() silently inserts an item into the
    map if no item exists with the same key (unless the map is
    const). For example, the following code snippet will create 1000
    items in memory:

    \snippet code/src_corelib_tools_qmap.cpp 6

    To avoid this problem, replace \c map[i] with \c map.value(i)
    in the code above.

    If you want to navigate through all the (key, value) pairs stored
    in a QMap, you can use an iterator. QMap provides both
    \l{Java-style iterators} (QMapIterator and QMutableMapIterator)
    and \l{STL-style iterators} (QMap::const_iterator and
    QMap::iterator). Here's how to iterate over a QMap<QString, int>
    using a Java-style iterator:

    \snippet code/src_corelib_tools_qmap.cpp 7

    Here's the same code, but using an STL-style iterator this time:

    \snippet code/src_corelib_tools_qmap.cpp 8

    The items are traversed in ascending key order.

    Normally, a QMap allows only one value per key. If you call
    insert() with a key that already exists in the QMap, the
    previous value will be erased. For example:

    \snippet code/src_corelib_tools_qmap.cpp 9

    However, you can store multiple values per key by using
    insertMulti() instead of insert() (or using the convenience
    subclass QMultiMap). If you want to retrieve all the values for a
    single key, you can use values(const Key &key), which returns a
    QList<T>:

    \snippet code/src_corelib_tools_qmap.cpp 10

    The items that share the same key are available from most
    recently to least recently inserted. Another approach is to call
    find() to get the STL-style iterator for the first item with a
    key and iterate from there:

    \snippet code/src_corelib_tools_qmap.cpp 11

    If you only need to extract the values from a map (not the keys),
    you can also use \l{foreach}:

    \snippet code/src_corelib_tools_qmap.cpp 12

    Items can be removed from the map in several ways. One way is to
    call remove(); this will remove any item with the given key.
    Another way is to use QMutableMapIterator::remove(). In addition,
    you can clear the entire map using clear().

    QMap's key and value data types must be \l{assignable data
    types}. This covers most data types you are likely to encounter,
    but the compiler won't let you, for example, store a QWidget as a
    value; instead, store a QWidget *. In addition, QMap's key type
    must provide operator<(). QMap uses it to keep its items sorted,
    and assumes that two keys \c x and \c y are equal if neither \c{x
    < y} nor \c{y < x} is true.

    Example:
    \snippet code/src_corelib_tools_qmap.cpp 13

    In the example, we start by comparing the employees' names. If
    they're equal, we compare their dates of birth to break the tie.

    \sa QMapIterator, QMutableMapIterator, QHash, QSet
*/

/*! \fn QMap::QMap()

    Constructs an empty map.

    \sa clear()
*/

/*!
    \fn QMap::QMap(QMap<Key, T> &&other)

    Move-constructs a QMap instance, making it point at the same
    object that \a other was pointing to.

    \since 5.2
*/

/*! \fn QMap::QMap(const QMap<Key, T> &other)

    Constructs a copy of \a other.

    This operation occurs in \l{constant time}, because QMap is
    \l{implicitly shared}. This makes returning a QMap from a
    function very fast. If a shared instance is modified, it will be
    copied (copy-on-write), and this takes \l{linear time}.

    \sa operator=()
*/

/*! \fn QMap::QMap(const std::map<Key, T> & other)

    Constructs a copy of \a other.

    This function is only available if Qt is configured with STL
    compatibility enabled.

    \sa toStdMap()
*/

/*! \fn QMap::QMap(std::initializer_list<std::pair<Key,T> > list)
    \since 5.1

    Constructs a map with a copy of each of the elements in the
    initializer list \a list.

    This function is only available if the program is being
    compiled in C++11 mode.
*/

/*! \fn std::map<Key, T> QMap::toStdMap() const

    Returns an STL map equivalent to this QMap.

    This function is only available if Qt is configured with STL
    compatibility enabled.
*/

/*! \fn QMap::~QMap()

    Destroys the map. References to the values in the map, and all
    iterators over this map, become invalid.
*/

/*! \fn QMap<Key, T> &QMap::operator=(const QMap<Key, T> &other)

    Assigns \a other to this map and returns a reference to this map.
*/

/*!
    \fn QMap<Key, T> &QMap::operator=(QMap<Key, T> &&other)

    Move-assigns \a other to this QMap instance.

    \since 5.2
*/

/*! \fn void QMap::swap(QMap<Key, T> &other)
    \since 4.8

    Swaps map \a other with this map. This operation is very
    fast and never fails.
*/

/*! \fn void QMultiMap::swap(QMultiMap<Key, T> &other)
    \since 4.8

    Swaps map \a other with this map. This operation is very
    fast and never fails.
*/

/*! \fn bool QMap::operator==(const QMap<Key, T> &other) const

    Returns \c true if \a other is equal to this map; otherwise returns
    false.

    Two maps are considered equal if they contain the same (key,
    value) pairs.

    This function requires the value type to implement \c
    operator==().

    \sa operator!=()
*/

/*! \fn bool QMap::operator!=(const QMap<Key, T> &other) const

    Returns \c true if \a other is not equal to this map; otherwise
    returns \c false.

    Two maps are considered equal if they contain the same (key,
    value) pairs.

    This function requires the value type to implement \c
    operator==().

    \sa operator==()
*/

/*! \fn int QMap::size() const

    Returns the number of (key, value) pairs in the map.

    \sa isEmpty(), count()
*/

/*!
    \fn bool QMap::isEmpty() const

    Returns \c true if the map contains no items; otherwise returns
    false.

    \sa size()
*/

/*! \fn void QMap::detach()

    \internal

    Detaches this map from any other maps with which it may share
    data.

    \sa isDetached()
*/

/*! \fn bool QMap::isDetached() const

    \internal

    Returns \c true if the map's internal data isn't shared with any
    other map object; otherwise returns \c false.

    \sa detach()
*/

/*! \fn void QMap::setSharable(bool sharable)

    \internal
*/

/*! \fn bool QMap::isSharedWith(const QMap<Key, T> &other) const

    \internal
*/

/*! \fn void QMap::clear()

    Removes all items from the map.

    \sa remove()
*/

/*! \fn int QMap::remove(const Key &key)

    Removes all the items that have the key \a key from the map.
    Returns the number of items removed which is usually 1 but will be
    0 if the key isn't in the map, or \> 1 if insertMulti() has been
    used with the \a key.

    \sa clear(), take(), QMultiMap::remove()
*/

/*! \fn T QMap::take(const Key &key)

    Removes the item with the key \a key from the map and returns
    the value associated with it.

    If the item does not exist in the map, the function simply
    returns a \l{default-constructed value}. If there are multiple
    items for \a key in the map, only the most recently inserted one
    is removed and returned.

    If you don't use the return value, remove() is more efficient.

    \sa remove()
*/

/*! \fn bool QMap::contains(const Key &key) const

    Returns \c true if the map contains an item with key \a key;
    otherwise returns \c false.

    \sa count(), QMultiMap::contains()
*/

/*! \fn const T QMap::value(const Key &key, const T &defaultValue) const

    Returns the value associated with the key \a key.

    If the map contains no item with key \a key, the function returns
    \a defaultValue. If no \a defaultValue is specified, the function
    returns a \l{default-constructed value}. If there are multiple
    items for \a key in the map, the value of the most recently
    inserted one is returned.

    \sa key(), values(), contains(), operator[]()
*/

/*! \fn T &QMap::operator[](const Key &key)

    Returns the value associated with the key \a key as a modifiable
    reference.

    If the map contains no item with key \a key, the function inserts
    a \l{default-constructed value} into the map with key \a key, and
    returns a reference to it. If the map contains multiple items
    with key \a key, this function returns a reference to the most
    recently inserted value.

    \sa insert(), value()
*/

/*! \fn const T QMap::operator[](const Key &key) const

    \overload

    Same as value().
*/

/*! \fn QList<Key> QMap::uniqueKeys() const
    \since 4.2

    Returns a list containing all the keys in the map in ascending
    order. Keys that occur multiple times in the map (because items
    were inserted with insertMulti(), or unite() was used) occur only
    once in the returned list.

    \sa keys(), values()
*/

/*! \fn QList<Key> QMap::keys() const

    Returns a list containing all the keys in the map in ascending
    order. Keys that occur multiple times in the map (because items
    were inserted with insertMulti(), or unite() was used) also
    occur multiple times in the list.

    To obtain a list of unique keys, where each key from the map only
    occurs once, use uniqueKeys().

    The order is guaranteed to be the same as that used by values().

    \sa uniqueKeys(), values(), key()
*/

/*! \fn QList<Key> QMap::keys(const T &value) const

    \overload

    Returns a list containing all the keys associated with value \a
    value in ascending order.

    This function can be slow (\l{linear time}), because QMap's
    internal data structure is optimized for fast lookup by key, not
    by value.
*/

/*!
    \fn Key QMap::key(const T &value, const Key &defaultKey) const
    \since 4.3
    \overload

    Returns the first key with value \a value, or \a defaultKey if
    the map contains no item with value \a value. If no \a defaultKey
    is provided the function returns a
    \l{default-constructed value}{default-constructed key}.

    This function can be slow (\l{linear time}), because QMap's
    internal data structure is optimized for fast lookup by key, not
    by value.

    \sa value(), keys()
*/

/*! \fn QList<T> QMap::values() const

    Returns a list containing all the values in the map, in ascending
    order of their keys. If a key is associated with multiple values,
    all of its values will be in the list, and not just the most
    recently inserted one.

    \sa keys(), value()
*/

/*! \fn QList<T> QMap::values(const Key &key) const

    \overload

    Returns a list containing all the values associated with key
    \a key, from the most recently inserted to the least recently
    inserted one.

    \sa count(), insertMulti()
*/

/*! \fn int QMap::count(const Key &key) const

    Returns the number of items associated with key \a key.

    \sa contains(), insertMulti(), QMultiMap::count()
*/

/*! \fn int QMap::count() const

    \overload

    Same as size().
*/

/*! \fn QMap::iterator QMap::begin()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the first item in
    the map.

    \sa constBegin(), end()
*/

/*! \fn QMap::const_iterator QMap::begin() const

    \overload
*/

/*! \fn QMap::const_iterator QMap::cbegin() const
    \since 5.0

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first item
    in the map.

    \sa begin(), cend()
*/

/*! \fn QMap::const_iterator QMap::constBegin() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first item
    in the map.

    \sa begin(), constEnd()
*/

/*! \fn QMap::iterator QMap::end()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the imaginary item
    after the last item in the map.

    \sa begin(), constEnd()
*/

/*! \fn QMap::const_iterator QMap::end() const

    \overload
*/

/*! \fn QMap::const_iterator QMap::cend() const
    \since 5.0

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    item after the last item in the map.

    \sa cbegin(), end()
*/

/*! \fn QMap::const_iterator QMap::constEnd() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    item after the last item in the map.

    \sa constBegin(), end()
*/

/*! \fn const Key &QMap::firstKey() const
    \since 5.2

    Returns a reference to the smallest key in the map.
    This function assumes that the map is not empty.

    This executes in \l{constant time}.

    \sa lastKey(), first(), isEmpty()
*/

/*! \fn const Key &QMap::lastKey() const
    \since 5.2

    Returns a reference to the largest key in the map.
    This function assumes that the map is not empty.

    This executes in \l{logarithmic time}.

    \sa firstKey(), last(), isEmpty()
*/

/*! \fn T &QMap::first()
    \since 5.2

    Returns a reference to the first value in the map, that is the value mapped
    to the smallest key. This function assumes that the map is not empty.

    When unshared (or const version is called), this executes in \l{constant time}.

    \sa last(), firstKey(), isEmpty()
*/

/*! \fn const T &QMap::first() const
    \since 5.2

    \overload
*/

/*! \fn T &QMap::last()
    \since 5.2

    Returns a reference to the last value in the map, that is the value mapped
    to the largest key. This function assumes that the map is not empty.

    When unshared (or const version is called), this executes in \l{logarithmic time}.

    \sa first(), lastKey(), isEmpty()
*/

/*! \fn const T &QMap::last() const
    \since 5.2

    \overload
*/

/*! \fn QMap::iterator QMap::erase(iterator pos)

    Removes the (key, value) pair pointed to by the iterator \a pos
    from the map, and returns an iterator to the next item in the
    map.

    \sa remove()
*/

/*! \fn QMap::iterator QMap::find(const Key &key)

    Returns an iterator pointing to the item with key \a key in the
    map.

    If the map contains no item with key \a key, the function
    returns end().

    If the map contains multiple items with key \a key, this
    function returns an iterator that points to the most recently
    inserted value. The other values are accessible by incrementing
    the iterator. For example, here's some code that iterates over all
    the items with the same key:

    \snippet code/src_corelib_tools_qmap.cpp 14

    \sa constFind(), value(), values(), lowerBound(), upperBound(), QMultiMap::find()
*/

/*! \fn QMap::const_iterator QMap::find(const Key &key) const

    \overload
*/

/*! \fn QMap::const_iterator QMap::constFind(const Key &key) const
    \since 4.1

    Returns an const iterator pointing to the item with key \a key in the
    map.

    If the map contains no item with key \a key, the function
    returns constEnd().

    \sa find(), QMultiMap::constFind()
*/

/*! \fn QMap::iterator QMap::lowerBound(const Key &key)

    Returns an iterator pointing to the first item with key \a key in
    the map. If the map contains no item with key \a key, the
    function returns an iterator to the nearest item with a greater
    key.

    Example:
    \snippet code/src_corelib_tools_qmap.cpp 15

    If the map contains multiple items with key \a key, this
    function returns an iterator that points to the most recently
    inserted value. The other values are accessible by incrementing
    the iterator. For example, here's some code that iterates over all
    the items with the same key:

    \snippet code/src_corelib_tools_qmap.cpp 16

    \sa upperBound(), find()
*/

/*! \fn QMap::const_iterator QMap::lowerBound(const Key &key) const

    \overload
*/

/*! \fn QMap::iterator QMap::upperBound(const Key &key)

    Returns an iterator pointing to the item that immediately follows
    the last item with key \a key in the map. If the map contains no
    item with key \a key, the function returns an iterator to the
    nearest item with a greater key.

    Example:
    \snippet code/src_corelib_tools_qmap.cpp 17

    \sa lowerBound(), find()
*/

/*! \fn QMap::const_iterator QMap::upperBound(const Key &key) const

    \overload
*/

/*! \fn QMap::iterator QMap::insert(const Key &key, const T &value)

    Inserts a new item with the key \a key and a value of \a value.

    If there is already an item with the key \a key, that item's value
    is replaced with \a value.

    If there are multiple items with the key \a key, the most
    recently inserted item's value is replaced with \a value.

    \sa insertMulti()
*/

/*! \fn QMap::iterator QMap::insert(const_iterator pos, const Key &key, const T &value)
    \overload
    \since 5.1
    Inserts a new item with the key \a key and value \a value and with hint \a pos
    suggesting where to do the insert.

    If constBegin() is used as hint it indicates that the \a key is less than any key in the map
    while constEnd() suggests that the \a key is (strictly) larger than any key in the map.
    Otherwise the hint should meet the condition (\a pos - 1).key() < \a key <= pos.key().
    If the hint \a pos is wrong it is ignored and a regular insert is done.

    If there is already an item with the key \a key, that item's value
    is replaced with \a value.

    If there are multiple items with the key \a key, then exactly one of them
    is replaced with \a value.

    If the hint is correct and the map is unshared, the insert executes in amortized \l{constant time}.

    When creating a map from sorted data inserting the largest key first with constBegin()
    is faster than inserting in sorted order with constEnd(), since constEnd() - 1 (which is needed
    to check if the hint is valid) needs \l{logarithmic time}.

    \b {Note:} Be careful with the hint. Providing an iterator from an older shared instance might
    crash but there is also a risk that it will silently corrupt both the map and the \a pos map.

    \sa insertMulti()
*/

/*! \fn QMap::iterator QMap::insertMulti(const Key &key, const T &value)

    Inserts a new item with the key \a key and a value of \a value.

    If there is already an item with the same key in the map, this
    function will simply create a new one. (This behavior is
    different from insert(), which overwrites the value of an
    existing item.)

    \sa insert(), values()
*/

/*! \fn QMap::iterator QMap::insertMulti(const_iterator pos, const Key &key, const T &value)
    \overload
    \since 5.1
    Inserts a new item with the key \a key and value \a value and with hint \a pos
    suggesting where to do the insert.

    If constBegin() is used as hint it indicates that the \a key is less than any key in the map
    while constEnd() suggests that the \a key is larger than any key in the map.
    Otherwise the hint should meet the condition (\a pos - 1).key() < \a key <= pos.key().
    If the hint \a pos is wrong it is ignored and a regular insertMulti is done.

    If there is already an item with the same key in the map, this function will simply create a new one.

    \b {Note:} Be careful with the hint. Providing an iterator from an older shared instance might
    crash but there is also a risk that it will silently corrupt both the map and the \a pos map.

    \sa insert()
*/


/*! \fn QMap<Key, T> &QMap::unite(const QMap<Key, T> &other)

    Inserts all the items in the \a other map into this map. If a
    key is common to both maps, the resulting map will contain the
    key multiple times.

    \sa insertMulti()
*/

/*! \typedef QMap::Iterator

    Qt-style synonym for QMap::iterator.
*/

/*! \typedef QMap::ConstIterator

    Qt-style synonym for QMap::const_iterator.
*/

/*! \typedef QMap::difference_type

    Typedef for ptrdiff_t. Provided for STL compatibility.
*/

/*! \typedef QMap::key_type

    Typedef for Key. Provided for STL compatibility.
*/

/*! \typedef QMap::mapped_type

    Typedef for T. Provided for STL compatibility.
*/

/*! \typedef QMap::size_type

    Typedef for int. Provided for STL compatibility.
*/

/*!
    \fn bool QMap::empty() const

    This function is provided for STL compatibility. It is equivalent
    to isEmpty(), returning true if the map is empty; otherwise
    returning false.
*/

/*!
  \fn QPair<iterator, iterator> QMap::equal_range(const Key &key)

  Returns a pair of iterators delimiting the range of values that
  are stored under \a key.
*/


/*! \class QMap::iterator
    \inmodule QtCore
    \brief The QMap::iterator class provides an STL-style non-const iterator for QMap and QMultiMap.

    QMap features both \l{STL-style iterators} and \l{Java-style
    iterators}. The STL-style iterators are more low-level and more
    cumbersome to use; on the other hand, they are slightly faster
    and, for developers who already know STL, have the advantage of
    familiarity.

    QMap\<Key, T\>::iterator allows you to iterate over a QMap (or
    QMultiMap) and to modify the value (but not the key) stored under
    a particular key. If you want to iterate over a const QMap, you
    should use QMap::const_iterator. It is generally good practice to
    use QMap::const_iterator on a non-const QMap as well, unless you
    need to change the QMap through the iterator. Const iterators are
    slightly faster, and can improve code readability.

    The default QMap::iterator constructor creates an uninitialized
    iterator. You must initialize it using a QMap function like
    QMap::begin(), QMap::end(), or QMap::find() before you can
    start iterating. Here's a typical loop that prints all the (key,
    value) pairs stored in a map:

    \snippet code/src_corelib_tools_qmap.cpp 18

    Unlike QHash, which stores its items in an arbitrary order, QMap
    stores its items ordered by key. Items that share the same key
    (because they were inserted using QMap::insertMulti(), or due to a
    unite()) will appear consecutively, from the most recently to the
    least recently inserted value.

    Let's see a few examples of things we can do with a
    QMap::iterator that we cannot do with a QMap::const_iterator.
    Here's an example that increments every value stored in the QMap
    by 2:

    \snippet code/src_corelib_tools_qmap.cpp 19

    Here's an example that removes all the items whose key is a
    string that starts with an underscore character:

    \snippet code/src_corelib_tools_qmap.cpp 20

    The call to QMap::erase() removes the item pointed to by the
    iterator from the map, and returns an iterator to the next item.
    Here's another way of removing an item while iterating:

    \snippet code/src_corelib_tools_qmap.cpp 21

    It might be tempting to write code like this:

    \snippet code/src_corelib_tools_qmap.cpp 22

    However, this will potentially crash in \c{++i}, because \c i is
    a dangling iterator after the call to erase().

    Multiple iterators can be used on the same map. If you add items
    to the map, existing iterators will remain valid. If you remove
    items from the map, iterators that point to the removed items
    will become dangling iterators.

    \warning Iterators on implicitly shared containers do not work
    exactly like STL-iterators. You should avoid copying a container
    while iterators are active on that container. For more information,
    read \l{Implicit sharing iterator problem}.

    \sa QMap::const_iterator, QMutableMapIterator
*/

/*! \typedef QMap::iterator::difference_type

    \internal
*/

/*! \typedef QMap::iterator::iterator_category

  A synonym for \e {std::bidirectional_iterator_tag} indicating
  this iterator is a bidirectional iterator.
*/

/*! \typedef QMap::iterator::pointer

    \internal
*/

/*! \typedef QMap::iterator::reference

    \internal
*/

/*! \typedef QMap::iterator::value_type

    \internal
*/

/*! \fn QMap::iterator::iterator()

    Constructs an uninitialized iterator.

    Functions like key(), value(), and operator++() must not be
    called on an uninitialized iterator. Use operator=() to assign a
    value to it before using it.

    \sa QMap::begin(), QMap::end()
*/

/*! \fn QMap::iterator::iterator(Node *)

    \internal
*/

/*! \fn const Key &QMap::iterator::key() const

    Returns the current item's key as a const reference.

    There is no direct way of changing an item's key through an
    iterator, although it can be done by calling QMap::erase()
    followed by QMap::insert() or QMap::insertMulti().

    \sa value()
*/

/*! \fn T &QMap::iterator::value() const

    Returns a modifiable reference to the current item's value.

    You can change the value of an item by using value() on
    the left side of an assignment, for example:

    \snippet code/src_corelib_tools_qmap.cpp 23

    \sa key(), operator*()
*/

/*! \fn T &QMap::iterator::operator*() const

    Returns a modifiable reference to the current item's value.

    Same as value().

    \sa key()
*/

/*! \fn T *QMap::iterator::operator->() const

    Returns a pointer to the current item's value.

    \sa value()
*/

/*!
    \fn bool QMap::iterator::operator==(const iterator &other) const
    \fn bool QMap::iterator::operator==(const const_iterator &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*!
    \fn bool QMap::iterator::operator!=(const iterator &other) const
    \fn bool QMap::iterator::operator!=(const const_iterator &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*! \fn QMap::iterator QMap::iterator::operator++()

    The prefix ++ operator (\c{++i}) advances the iterator to the
    next item in the map and returns an iterator to the new current
    item.

    Calling this function on QMap::end() leads to undefined results.

    \sa operator--()
*/

/*! \fn QMap::iterator QMap::iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{i++}) advances the iterator to the
    next item in the map and returns an iterator to the previously
    current item.
*/

/*! \fn QMap::iterator QMap::iterator::operator--()

    The prefix -- operator (\c{--i}) makes the preceding item
    current and returns an iterator pointing to the new current item.

    Calling this function on QMap::begin() leads to undefined
    results.

    \sa operator++()
*/

/*! \fn QMap::iterator QMap::iterator::operator--(int)

    \overload

    The postfix -- operator (\c{i--}) makes the preceding item
    current and returns an iterator pointing to the previously
    current item.
*/

/*! \fn QMap::iterator QMap::iterator::operator+(int j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. (If \a j is negative, the iterator goes backward.)

    This operation can be slow for large \a j values.

    \sa operator-()

*/

/*! \fn QMap::iterator QMap::iterator::operator-(int j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator. (If \a j is negative, the iterator goes forward.)

    This operation can be slow for large \a j values.

    \sa operator+()
*/

/*! \fn QMap::iterator &QMap::iterator::operator+=(int j)

    Advances the iterator by \a j items. (If \a j is negative, the
    iterator goes backward.)

    \sa operator-=(), operator+()
*/

/*! \fn QMap::iterator &QMap::iterator::operator-=(int j)

    Makes the iterator go back by \a j items. (If \a j is negative,
    the iterator goes forward.)

    \sa operator+=(), operator-()
*/

/*! \class QMap::const_iterator
    \inmodule QtCore
    \brief The QMap::const_iterator class provides an STL-style const iterator for QMap and QMultiMap.

    QMap features both \l{STL-style iterators} and \l{Java-style
    iterators}. The STL-style iterators are more low-level and more
    cumbersome to use; on the other hand, they are slightly faster
    and, for developers who already know STL, have the advantage of
    familiarity.

    QMap\<Key, T\>::const_iterator allows you to iterate over a QMap
    (or a QMultiMap). If you want to modify the QMap as you iterate
    over it, you must use QMap::iterator instead. It is generally
    good practice to use QMap::const_iterator on a non-const QMap as
    well, unless you need to change the QMap through the iterator.
    Const iterators are slightly faster, and can improve code
    readability.

    The default QMap::const_iterator constructor creates an
    uninitialized iterator. You must initialize it using a QMap
    function like QMap::constBegin(), QMap::constEnd(), or
    QMap::find() before you can start iterating. Here's a typical
    loop that prints all the (key, value) pairs stored in a map:

    \snippet code/src_corelib_tools_qmap.cpp 24

    Unlike QHash, which stores its items in an arbitrary order, QMap
    stores its items ordered by key. Items that share the same key
    (because they were inserted using QMap::insertMulti()) will
    appear consecutively, from the most recently to the least
    recently inserted value.

    Multiple iterators can be used on the same map. If you add items
    to the map, existing iterators will remain valid. If you remove
    items from the map, iterators that point to the removed items
    will become dangling iterators.

    \warning Iterators on implicitly shared containers do not work
    exactly like STL-iterators. You should avoid copying a container
    while iterators are active on that container. For more information,
    read \l{Implicit sharing iterator problem}.

    \sa QMap::iterator, QMapIterator
*/

/*! \typedef QMap::const_iterator::difference_type

    \internal
*/

/*! \typedef QMap::const_iterator::iterator_category

  A synonym for \e {std::bidirectional_iterator_tag} indicating
  this iterator is a bidirectional iterator.
*/

/*! \typedef QMap::const_iterator::pointer

    \internal
*/

/*! \typedef QMap::const_iterator::reference

    \internal
*/

/*! \typedef QMap::const_iterator::value_type

    \internal
*/

/*! \fn QMap::const_iterator::const_iterator()

    Constructs an uninitialized iterator.

    Functions like key(), value(), and operator++() must not be
    called on an uninitialized iterator. Use operator=() to assign a
    value to it before using it.

    \sa QMap::constBegin(), QMap::constEnd()
*/

/*! \fn QMap::const_iterator::const_iterator(const Node *)

    \internal
*/

/*! \fn QMap::const_iterator::const_iterator(const iterator &other)

    Constructs a copy of \a other.
*/

/*! \fn const Key &QMap::const_iterator::key() const

    Returns the current item's key.

    \sa value()
*/

/*! \fn const T &QMap::const_iterator::value() const

    Returns the current item's value.

    \sa key(), operator*()
*/

/*! \fn const T &QMap::const_iterator::operator*() const

    Returns the current item's value.

    Same as value().

    \sa key()
*/

/*! \fn const T *QMap::const_iterator::operator->() const

    Returns a pointer to the current item's value.

    \sa value()
*/

/*! \fn bool QMap::const_iterator::operator==(const const_iterator &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*! \fn bool QMap::const_iterator::operator!=(const const_iterator &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*! \fn QMap::const_iterator QMap::const_iterator::operator++()

    The prefix ++ operator (\c{++i}) advances the iterator to the
    next item in the map and returns an iterator to the new current
    item.

    Calling this function on QMap::end() leads to undefined results.

    \sa operator--()
*/

/*! \fn QMap::const_iterator QMap::const_iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{i++}) advances the iterator to the
    next item in the map and returns an iterator to the previously
    current item.
*/

/*! \fn QMap::const_iterator &QMap::const_iterator::operator--()

    The prefix -- operator (\c{--i}) makes the preceding item
    current and returns an iterator pointing to the new current item.

    Calling this function on QMap::begin() leads to undefined
    results.

    \sa operator++()
*/

/*! \fn QMap::const_iterator QMap::const_iterator::operator--(int)

    \overload

    The postfix -- operator (\c{i--}) makes the preceding item
    current and returns an iterator pointing to the previously
    current item.
*/

/*! \fn QMap::const_iterator QMap::const_iterator::operator+(int j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. (If \a j is negative, the iterator goes backward.)

    This operation can be slow for large \a j values.

    \sa operator-()
*/

/*! \fn QMap::const_iterator QMap::const_iterator::operator-(int j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator. (If \a j is negative, the iterator goes forward.)

    This operation can be slow for large \a j values.

    \sa operator+()
*/

/*! \fn QMap::const_iterator &QMap::const_iterator::operator+=(int j)

    Advances the iterator by \a j items. (If \a j is negative, the
    iterator goes backward.)

    This operation can be slow for large \a j values.

    \sa operator-=(), operator+()
*/

/*! \fn QMap::const_iterator &QMap::const_iterator::operator-=(int j)

    Makes the iterator go back by \a j items. (If \a j is negative,
    the iterator goes forward.)

    This operation can be slow for large \a j values.

    \sa operator+=(), operator-()
*/

/*! \fn QDataStream &operator<<(QDataStream &out, const QMap<Key, T> &map)
    \relates QMap

    Writes the map \a map to stream \a out.

    This function requires the key and value types to implement \c
    operator<<().

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/

/*! \fn QDataStream &operator>>(QDataStream &in, QMap<Key, T> &map)
    \relates QMap

    Reads a map from stream \a in into \a map.

    This function requires the key and value types to implement \c
    operator>>().

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/

/*! \class QMultiMap
    \inmodule QtCore
    \brief The QMultiMap class is a convenience QMap subclass that provides multi-valued maps.

    \ingroup tools
    \ingroup shared

    \reentrant

    QMultiMap\<Key, T\> is one of Qt's generic \l{container classes}.
    It inherits QMap and extends it with a few convenience functions
    that make it more suitable than QMap for storing multi-valued
    maps. A multi-valued map is a map that allows multiple values
    with the same key; QMap normally doesn't allow that, unless you
    call QMap::insertMulti().

    Because QMultiMap inherits QMap, all of QMap's functionality also
    applies to QMultiMap. For example, you can use isEmpty() to test
    whether the map is empty, and you can traverse a QMultiMap using
    QMap's iterator classes (for example, QMapIterator). But in
    addition, it provides an insert() function that corresponds to
    QMap::insertMulti(), and a replace() function that corresponds to
    QMap::insert(). It also provides convenient operator+() and
    operator+=().

    Example:
    \snippet code/src_corelib_tools_qmap.cpp 25

    Unlike QMap, QMultiMap provides no operator[]. Use value() or
    replace() if you want to access the most recently inserted item
    with a certain key.

    If you want to retrieve all the values for a single key, you can
    use values(const Key &key), which returns a QList<T>:

    \snippet code/src_corelib_tools_qmap.cpp 26

    The items that share the same key are available from most
    recently to least recently inserted.

    If you prefer the STL-style iterators, you can call find() to get
    the iterator for the first item with a key and iterate from
    there:

    \snippet code/src_corelib_tools_qmap.cpp 27

    QMultiMap's key and value data types must be \l{assignable data
    types}. This covers most data types you are likely to encounter,
    but the compiler won't let you, for example, store a QWidget as a
    value; instead, store a QWidget *. In addition, QMultiMap's key type
    must provide operator<(). See the QMap documentation for details.

    \sa QMap, QMapIterator, QMutableMapIterator, QMultiHash
*/

/*! \fn QMultiMap::QMultiMap()

    Constructs an empty map.
*/

/*! \fn QMultiMap::QMultiMap(std::initializer_list<std::pair<Key,T> > list)
    \since 5.1

    Constructs a multi map with a copy of each of the elements in the
    initializer list \a list.

    This function is only available if the program is being
    compiled in C++11 mode.
*/

/*! \fn QMultiMap::QMultiMap(const QMap<Key, T> &other)

    Constructs a copy of \a other (which can be a QMap or a
    QMultiMap).

    \sa operator=()
*/

/*! \fn QMultiMap::iterator QMultiMap::replace(const Key &key, const T &value)

    Inserts a new item with the key \a key and a value of \a value.

    If there is already an item with the key \a key, that item's value
    is replaced with \a value.

    If there are multiple items with the key \a key, the most
    recently inserted item's value is replaced with \a value.

    \sa insert()
*/

/*! \fn QMultiMap::iterator QMultiMap::insert(const Key &key, const T &value)

    Inserts a new item with the key \a key and a value of \a value.

    If there is already an item with the same key in the map, this
    function will simply create a new one. (This behavior is
    different from replace(), which overwrites the value of an
    existing item.)

    \sa replace()
*/

/*! \fn QMultiMap::iterator QMultiMap::insert(QMap<Key, T>::const_iterator pos, const Key &key, const T &value)

    \since 5.1
    Inserts a new item with the key \a key and value \a value and with hint \a pos
    suggesting where to do the insert.

    If constBegin() is used as hint it indicates that the \a key is less than any key in the map
    while constEnd() suggests that the \a key is larger than any key in the map.
    Otherwise the hint should meet the condition (\a pos - 1).key() < \a key <= pos.key().
    If the hint \a pos is wrong it is ignored and a regular insert is done.

    If there is already an item with the same key in the map, this function will simply create a new one.

    \b {Note:} Be careful with the hint. Providing an iterator from an older shared instance might
    crash but there is also a risk that it will silently corrupt both the map and the \a pos map.
*/

/*! \fn QMultiMap &QMultiMap::operator+=(const QMultiMap &other)

    Inserts all the items in the \a other map into this map and
    returns a reference to this map.

    \sa insert(), operator+()
*/

/*! \fn QMultiMap QMultiMap::operator+(const QMultiMap &other) const

    Returns a map that contains all the items in this map in
    addition to all the items in \a other. If a key is common to both
    maps, the resulting map will contain the key multiple times.

    \sa operator+=()
*/

/*!
    \fn bool QMultiMap::contains(const Key &key, const T &value) const
    \since 4.3

    Returns \c true if the map contains an item with key \a key and
    value \a value; otherwise returns \c false.

    \sa QMap::contains()
*/

/*!
    \fn bool QMultiMap::contains(const Key &key) const
    \overload
    \sa QMap::contains()
*/

/*!
    \fn int QMultiMap::remove(const Key &key, const T &value)
    \since 4.3

    Removes all the items that have the key \a key and the value \a
    value from the map. Returns the number of items removed.

    \sa QMap::remove()
*/

/*!
    \fn int QMultiMap::remove(const Key &key)
    \overload
    \sa QMap::remove()
*/

/*!
    \fn int QMultiMap::count(const Key &key, const T &value) const
    \since 4.3

    Returns the number of items with key \a key and value \a value.

    \sa QMap::count()
*/

/*!
    \fn int QMultiMap::count(const Key &key) const
    \overload
    \sa QMap::count()
*/

/*!
    \fn int QMultiMap::count() const
    \overload
    \sa QMap::count()
*/

/*!
    \fn typename QMap<Key, T>::iterator QMultiMap::find(const Key &key, const T &value)
    \since 4.3

    Returns an iterator pointing to the item with key \a key and
    value \a value in the map.

    If the map contains no such item, the function returns end().

    If the map contains multiple items with key \a key, this
    function returns an iterator that points to the most recently
    inserted value.

    \sa QMap::find()
*/

/*!
    \fn typename QMap<Key, T>::iterator QMultiMap::find(const Key &key)
    \overload
    \sa QMap::find()
*/

/*!
    \fn typename QMap<Key, T>::const_iterator QMultiMap::find(const Key &key, const T &value) const
    \since 4.3
    \overload

    Returns a const iterator pointing to the item with the given \a key and
    \a value in the map.

    If the map contains no such item, the function returns end().

    If the map contains multiple items with the specified \a key, this
    function returns a const iterator that points to the most recently
    inserted value.

    \sa QMap::find()
*/

/*!
    \fn typename QMap<Key, T>::const_iterator QMultiMap::find(const Key &key) const
    \since 4.3
    \overload
    \sa QMap::find()
*/

/*!
    \fn typename QMap<Key, T>::const_iterator QMultiMap::constFind(const Key &key, const T &value) const
    \since 4.3

    Returns an iterator pointing to the item with key \a key and the
    value \a value in the map.

    If the map contains no such item, the function returns
    constEnd().

    \sa QMap::constFind()
*/

/*!
    \fn typename QMap<Key, T>::const_iterator QMultiMap::constFind(const Key &key) const
    \overload
    \sa QMap::constFind()
*/

QT_END_NAMESPACE
