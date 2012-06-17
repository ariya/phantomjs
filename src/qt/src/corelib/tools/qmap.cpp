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

#include "qmap.h"

#include <stdlib.h>

#ifdef QT_QMAP_DEBUG
# include <qstring.h>
# include <qvector.h>
#endif

QT_BEGIN_NAMESPACE

QMapData QMapData::shared_null = {
    &shared_null,
    { &shared_null, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    Q_BASIC_ATOMIC_INITIALIZER(1), 0, 0, 0, false, true, false, 0
};

QMapData *QMapData::createData()
{
    return createData(0);
}

QMapData *QMapData::createData(int alignment)
{
    QMapData *d = new QMapData;
    Q_CHECK_PTR(d);
    Node *e = reinterpret_cast<Node *>(d);
    e->backward = e;
    e->forward[0] = e;
    d->ref = 1;
    d->topLevel = 0;
    d->size = 0;
    d->randomBits = 0;
    d->insertInOrder = false;
    d->sharable = true;
    d->strictAlignment = alignment > 8;
    d->reserved = 0;
    return d;
}

void QMapData::continueFreeData(int offset)
{
    Node *e = reinterpret_cast<Node *>(this);
    Node *cur = e->forward[0];
    Node *prev;
    while (cur != e) {
        prev = cur;
        cur = cur->forward[0];
        if (strictAlignment)
            qFreeAligned(reinterpret_cast<char *>(prev) - offset);
        else
            qFree(reinterpret_cast<char *>(prev) - offset);
    }
    delete this;
}

QMapData::Node *QMapData::node_create(Node *update[], int offset)
{
    return node_create(update, offset, 0);
}

/*!
    Creates a new node inside the data structure.

    \a update is an array with pointers to the node after which the new node
    should be inserted. Because of the strange skip list data structure there
    could be several pointers to this node on different levels.
    \a offset is an amount of bytes that needs to reserved just before the
    QMapData::Node structure.

    \a alignment dictates the alignment for the data.

    \internal
    \since 4.6
*/
QMapData::Node *QMapData::node_create(Node *update[], int offset, int alignment)
{
    int level = 0;
    uint mask = (1 << Sparseness) - 1;

    while ((randomBits & mask) == mask && level < LastLevel) {
        ++level;
        mask <<= Sparseness;
    }

    if (level > topLevel) {
        Node *e = reinterpret_cast<Node *>(this);
        level = ++topLevel;
        e->forward[level] = e;
        update[level] = e;
    }

    ++randomBits;
    if (level == 3 && !insertInOrder)
        randomBits = qrand();

    void *concreteNode = strictAlignment ?
                         qMallocAligned(offset + sizeof(Node) + level * sizeof(Node *), alignment) :
                         qMalloc(offset + sizeof(Node) + level * sizeof(Node *));
    Q_CHECK_PTR(concreteNode);

    Node *abstractNode = reinterpret_cast<Node *>(reinterpret_cast<char *>(concreteNode) + offset);

    abstractNode->backward = update[0];
    update[0]->forward[0]->backward = abstractNode;

    for (int i = level; i >= 0; i--) {
        abstractNode->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = abstractNode;
        update[i] = abstractNode;
    }
    ++size;
    return abstractNode;
}

void QMapData::node_delete(Node *update[], int offset, Node *node)
{
    node->forward[0]->backward = node->backward;

    for (int i = 0; i <= topLevel; ++i) {
        if (update[i]->forward[i] != node)
            break;
        update[i]->forward[i] = node->forward[i];
    }
    --size;
    if (strictAlignment)
        qFreeAligned(reinterpret_cast<char *>(node) - offset);
    else
        qFree(reinterpret_cast<char *>(node) - offset);
}

#ifdef QT_QMAP_DEBUG

uint QMapData::adjust_ptr(Node *node)
{
    if (node == reinterpret_cast<Node *>(this)) {
       return (uint)0xDEADBEEF;
    } else {
        return (uint)node;
    }
}

void QMapData::dump()
{
    qDebug("Map data (ref = %d, size = %d, randomBits = %#.8x)", int(ref), size, randomBits);

    QString preOutput;
    QVector<QString> output(topLevel + 1);
    Node *e = reinterpret_cast<Node *>(this);

    QString str;
    str.sprintf("    %.8x", adjust_ptr(reinterpret_cast<Node *>(this)));
    preOutput += str;

    Node *update[LastLevel + 1];
    for (int i = 0; i <= topLevel; ++i) {
        str.sprintf("%d: [%.8x] -", i, adjust_ptr(reinterpret_cast<Node *>(forward[i])));
        output[i] += str;
        update[i] = reinterpret_cast<Node *>(forward[i]);
    }

    Node *node = reinterpret_cast<Node *>(forward[0]);
    while (node != e) {
        int level = 0;
        while (level < topLevel && update[level + 1] == node)
            ++level;

        str.sprintf("       %.8x", adjust_ptr(node));
        preOutput += str;

        for (int i = 0; i <= level; ++i) {
            str.sprintf("-> [%.8x] -", adjust_ptr(node->forward[i]));
            output[i] += str;
            update[i] = node->forward[i];
        }
        for (int j = level + 1; j <= topLevel; ++j)
            output[j] += QLatin1String("---------------");
        node = node->forward[0];
    }

    qDebug("%s", preOutput.ascii());
    for (int i = 0; i <= topLevel; ++i)
        qDebug("%s", output[i].ascii());
}
#endif

/*!
    \class QMap
    \brief The QMap class is a template class that provides a skip-list-based dictionary.

    \ingroup tools
    \ingroup shared

    \reentrant

    QMap\<Key, T\> is one of Qt's generic \l{container classes}. It
    stores (key, value) pairs and provides fast lookup of the
    value associated with a key.

    QMap and QHash provide very similar functionality. The
    differences are:

    \list
    \i QHash provides faster lookups than QMap. (See \l{Algorithmic
       Complexity} for details.)
    \i When iterating over a QHash, the items are arbitrarily ordered.
       With QMap, the items are always sorted by key.
    \i The key type of a QHash must provide operator==() and a global
       qHash(Key) function. The key type of a QMap must provide
       operator<() specifying a total order.
    \endlist

    Here's an example QMap with QString keys and \c int values:
    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 0

    To insert a (key, value) pair into the map, you can use operator[]():

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 1

    This inserts the following three (key, value) pairs into the
    QMap: ("one", 1), ("three", 3), and ("seven", 7). Another way to
    insert items into the map is to use insert():

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 2

    To look up a value, use operator[]() or value():

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 3

    If there is no item with the specified key in the map, these
    functions return a \l{default-constructed value}.

    If you want to check whether the map contains a certain key, use
    contains():

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 4

    There is also a value() overload that uses its second argument as
    a default value if there is no item with the specified key:

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 5

    In general, we recommend that you use contains() and value()
    rather than operator[]() for looking up a key in a map. The
    reason is that operator[]() silently inserts an item into the
    map if no item exists with the same key (unless the map is
    const). For example, the following code snippet will create 1000
    items in memory:

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 6

    To avoid this problem, replace \c map[i] with \c map.value(i)
    in the code above.

    If you want to navigate through all the (key, value) pairs stored
    in a QMap, you can use an iterator. QMap provides both
    \l{Java-style iterators} (QMapIterator and QMutableMapIterator)
    and \l{STL-style iterators} (QMap::const_iterator and
    QMap::iterator). Here's how to iterate over a QMap<QString, int>
    using a Java-style iterator:

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 7

    Here's the same code, but using an STL-style iterator this time:

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 8

    The items are traversed in ascending key order.

    Normally, a QMap allows only one value per key. If you call
    insert() with a key that already exists in the QMap, the
    previous value will be erased. For example:

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 9

    However, you can store multiple values per key by using
    insertMulti() instead of insert() (or using the convenience
    subclass QMultiMap). If you want to retrieve all the values for a
    single key, you can use values(const Key &key), which returns a
    QList<T>:

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 10

    The items that share the same key are available from most
    recently to least recently inserted. Another approach is to call
    find() to get the STL-style iterator for the first item with a
    key and iterate from there:

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 11

    If you only need to extract the values from a map (not the keys),
    you can also use \l{foreach}:

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 12

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
    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 13

    In the example, we start by comparing the employees' names. If
    they're equal, we compare their dates of birth to break the tie.

    \sa QMapIterator, QMutableMapIterator, QHash, QSet
*/

/*! \fn QMap::QMap()

    Constructs an empty map.

    \sa clear()
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

    Returns true if \a other is equal to this map; otherwise returns
    false.

    Two maps are considered equal if they contain the same (key,
    value) pairs.

    This function requires the value type to implement \c
    operator==().

    \sa operator!=()
*/

/*! \fn bool QMap::operator!=(const QMap<Key, T> &other) const

    Returns true if \a other is not equal to this map; otherwise
    returns false.

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

    Returns true if the map contains no items; otherwise returns
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

    Returns true if the map's internal data isn't shared with any
    other map object; otherwise returns false.

    \sa detach()
*/

/*! \fn void QMap::setSharable(bool sharable)

    \internal
*/

/*! \fn bool QMap::isSharedWith(const QMap<Key, T> &other) const

    \internal
*/

/*! \fn void QMap::setInsertInOrder(bool sharable)

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

    Returns true if the map contains an item with key \a key;
    otherwise returns false.

    \sa count(), QMultiMap::contains()
*/

/*! \fn const T QMap::value(const Key &key) const

    Returns the value associated with the key \a key.

    If the map contains no item with key \a key, the function
    returns a \l{default-constructed value}. If there are multiple
    items for \a key in the map, the value of the most recently
    inserted one is returned.

    \sa key(), values(), contains(), operator[]()
*/

/*! \fn const T QMap::value(const Key &key, const T &defaultValue) const

    \overload

    If the map contains no item with key \a key, the function returns
    \a defaultValue.
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

/*! \fn Key QMap::key(const T &value) const

    Returns the first key with value \a value.

    If the map contains no item with value \a value, the function
    returns a \link {default-constructed value} default-constructed
    key \endlink.

    This function can be slow (\l{linear time}), because QMap's
    internal data structure is optimized for fast lookup by key, not
    by value.

    \sa value(), keys()
*/

/*!
    \fn Key QMap::key(const T &value, const Key &defaultKey) const
    \since 4.3
    \overload

    Returns the first key with value \a value, or \a defaultKey if
    the map contains no item with value \a value.

    This function can be slow (\l{linear time}), because QMap's
    internal data structure is optimized for fast lookup by key, not
    by value.
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

    Returns an \l{STL-style iterator} pointing to the first item in
    the map.

    \sa constBegin(), end()
*/

/*! \fn QMap::const_iterator QMap::begin() const

    \overload
*/

/*! \fn QMap::const_iterator QMap::constBegin() const

    Returns a const \l{STL-style iterator} pointing to the first item
    in the map.

    \sa begin(), constEnd()
*/

/*! \fn QMap::iterator QMap::end()

    Returns an \l{STL-style iterator} pointing to the imaginary item
    after the last item in the map.

    \sa begin(), constEnd()
*/

/*! \fn QMap::const_iterator QMap::end() const

    \overload
*/

/*! \fn QMap::const_iterator QMap::constEnd() const

    Returns a const \l{STL-style iterator} pointing to the imaginary
    item after the last item in the map.

    \sa constBegin(), end()
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

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 14

    \sa constFind(), value(), values(), lowerBound(), upperBound(), QMultiMap::find()
*/

/*! \fn QMap::const_iterator QMap::find(const Key &key) const

    \overload
*/

/*! \fn QMap::iterator QMap::constFind(const Key &key) const
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
    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 15

    If the map contains multiple items with key \a key, this
    function returns an iterator that points to the most recently
    inserted value. The other values are accessible by incrementing
    the iterator. For example, here's some code that iterates over all
    the items with the same key:

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 16

    \sa qLowerBound(), upperBound(), find()
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
    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 17

    \sa qUpperBound(), lowerBound(), find()
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

/*! \fn QMap::iterator QMap::insertMulti(const Key &key, const T &value)

    Inserts a new item with the key \a key and a value of \a value.

    If there is already an item with the same key in the map, this
    function will simply create a new one. (This behavior is
    different from insert(), which overwrites the value of an
    existing item.)

    \sa insert(), values()
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

/*! \class QMap::iterator
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

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 18

    Unlike QHash, which stores its items in an arbitrary order, QMap
    stores its items ordered by key. Items that share the same key
    (because they were inserted using QMap::insertMulti(), or due to a
    unite()) will appear consecutively, from the most recently to the
    least recently inserted value.

    Let's see a few examples of things we can do with a
    QMap::iterator that we cannot do with a QMap::const_iterator.
    Here's an example that increments every value stored in the QMap
    by 2:

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 19

    Here's an example that removes all the items whose key is a
    string that starts with an underscore character:

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 20

    The call to QMap::erase() removes the item pointed to by the
    iterator from the map, and returns an iterator to the next item.
    Here's another way of removing an item while iterating:

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 21

    It might be tempting to write code like this:

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 22

    However, this will potentially crash in \c{++i}, because \c i is
    a dangling iterator after the call to erase().

    Multiple iterators can be used on the same map. If you add items
    to the map, existing iterators will remain valid. If you remove
    items from the map, iterators that point to the removed items
    will become dangling iterators.

    \sa QMap::const_iterator, QMutableMapIterator
*/

/*! \fn QMap::iterator::operator QMapData::Node *() const

    \internal
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

    \sa QMap::begin() QMap::end()
*/

/*! \fn QMap::iterator::iterator(QMapData::Node *node)

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

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 23

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

    Returns true if \a other points to the same item as this
    iterator; otherwise returns false.

    \sa operator!=()
*/

/*!
    \fn bool QMap::iterator::operator!=(const iterator &other) const
    \fn bool QMap::iterator::operator!=(const const_iterator &other) const

    Returns true if \a other points to a different item than this
    iterator; otherwise returns false.

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

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 24

    Unlike QHash, which stores its items in an arbitrary order, QMap
    stores its items ordered by key. Items that share the same key
    (because they were inserted using QMap::insertMulti()) will
    appear consecutively, from the most recently to the least
    recently inserted value.

    Multiple iterators can be used on the same map. If you add items
    to the map, existing iterators will remain valid. If you remove
    items from the map, iterators that point to the removed items
    will become dangling iterators.

    \sa QMap::iterator, QMapIterator
*/

/*! \fn QMap::const_iterator::operator QMapData::Node *() const

    \internal
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

    \sa QMap::constBegin() QMap::constEnd()
*/

/*! \fn QMap::const_iterator::const_iterator(QMapData::Node *node)

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

    Returns true if \a other points to the same item as this
    iterator; otherwise returns false.

    \sa operator!=()
*/

/*! \fn bool QMap::const_iterator::operator!=(const const_iterator &other) const

    Returns true if \a other points to a different item than this
    iterator; otherwise returns false.

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

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

/*! \fn QDataStream &operator>>(QDataStream &in, QMap<Key, T> &map)
    \relates QMap

    Reads a map from stream \a in into \a map.

    This function requires the key and value types to implement \c
    operator>>().

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

/*! \class QMultiMap
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
    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 25

    Unlike QMap, QMultiMap provides no operator[]. Use value() or
    replace() if you want to access the most recently inserted item
    with a certain key.

    If you want to retrieve all the values for a single key, you can
    use values(const Key &key), which returns a QList<T>:

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 26

    The items that share the same key are available from most
    recently to least recently inserted.

    If you prefer the STL-style iterators, you can call find() to get
    the iterator for the first item with a key and iterate from
    there:

    \snippet doc/src/snippets/code/src_corelib_tools_qmap.cpp 27

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

    Returns true if the map contains an item with key \a key and
    value \a value; otherwise returns false.

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

/*!
    \fn T &QMap::iterator::data() const

    Use value() instead.
*/

/*!
    \fn const T &QMap::const_iterator::data() const

    Use value() instead.
*/

/*!
    \fn iterator QMap::remove(iterator it)

    Use erase(\a it) instead.
*/

/*!
    \fn void QMap::erase(const Key &key)

    Use remove(\a key) instead.
*/

/*!
    \fn iterator QMap::insert(const Key &key, const T &value, bool overwrite);

    Use the two-argument insert() overload instead. If you don't want
    to overwrite, call contains() beforehand.

    \oldcode
        QMap<QString, int> map;
        ...
        map.insert("delay", 30000, false);
    \newcode
        QMap<QString, int> map;
        ...
        if (!map.contains("delay"))
            map.insert("delay", 30000);
    \endcode
*/

/*!
    \fn iterator QMap::replace(const Key &key, const T &value)

    Use remove() then insert().
*/

QT_END_NAMESPACE
