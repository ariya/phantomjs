/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include <new>
#include "qlist.h"
#include "qtools_p.h"
#include <string.h>

QT_BEGIN_NAMESPACE

/*
    QList as an array-list combines the easy-of-use of a random
    access interface with fast list operations and the low memory
    management overhead of an array. Accessing elements by index,
    appending, prepending, and removing elements from both the front
    and the back all happen in constant time O(1). Inserting or
    removing elements at random index positions \ai happens in linear
    time, or more precisly in O(min{i,n-i}) <= O(n/2), with n being
    the number of elements in the list.
*/

QListData::Data QListData::shared_null = { Q_BASIC_ATOMIC_INITIALIZER(1), 0, 0, 0, true, { 0 } };

static int grow(int size)
{
    // dear compiler: don't optimize me out.
    volatile int x = qAllocMore(size * sizeof(void *), QListData::DataHeaderSize) / sizeof(void *);
    return x;
}

/*!
 *  Detaches the QListData by allocating new memory for a list which will be bigger
 *  than the copied one and is expected to grow further.
 *  *idx is the desired insertion point and is clamped to the actual size of the list.
 *  num is the number of new elements to insert at the insertion point.
 *  Returns the old (shared) data, it is up to the caller to deref() and free().
 *  For the new data node_copy needs to be called.
 *
 *  \internal
 */
QListData::Data *QListData::detach_grow(int *idx, int num)
{
    Data *x = d;
    int l = x->end - x->begin;
    int nl = l + num;
    int alloc = grow(nl);
    Data* t = static_cast<Data *>(qMalloc(DataHeaderSize + alloc * sizeof(void *)));
    Q_CHECK_PTR(t);

    t->ref = 1;
    t->sharable = true;
    t->alloc = alloc;
    // The space reservation algorithm's optimization is biased towards appending:
    // Something which looks like an append will put the data at the beginning,
    // while something which looks like a prepend will put it in the middle
    // instead of at the end. That's based on the assumption that prepending
    // is uncommon and even an initial prepend will eventually be followed by
    // at least some appends.
    int bg;
    if (*idx < 0) {
        *idx = 0;
        bg = (alloc - nl) >> 1;
    } else if (*idx > l) {
        *idx = l;
        bg = 0;
    } else if (*idx < (l >> 1)) {
        bg = (alloc - nl) >> 1;
    } else {
        bg = 0;
    }
    t->begin = bg;
    t->end = bg + nl;
    d = t;

    return x;
}

#if QT_VERSION >= 0x050000
#  error "Remove QListData::detach(), it is only required for binary compatibility for 4.0.x to 4.2.x"
#endif
QListData::Data *QListData::detach()
{
    Data *x = static_cast<Data *>(qMalloc(DataHeaderSize + d->alloc * sizeof(void *)));
    Q_CHECK_PTR(x);

    x->ref = 1;
    x->sharable = true;
    x->alloc = d->alloc;
    if (!x->alloc) {
        x->begin = 0;
        x->end = 0;
    } else {
        x->begin = d->begin;
        x->end   = d->end;
    }

    qSwap(d, x);
    if (!x->ref.deref())
        return x;
    return 0;
}

/*!
 *  Detaches the QListData by reallocating new memory.
 *  Returns the old (shared) data, it is up to the caller to deref() and free()
 *  For the new data node_copy needs to be called.
 *
 *  \internal
 */
#if QT_VERSION >= 0x050000
#  error "Remove QListData::detach2(), it is only required for binary compatibility for 4.3.x to 4.5.x"
#endif
QListData::Data *QListData::detach2()
{
    Data *x = d;
    Data* t = static_cast<Data *>(qMalloc(DataHeaderSize + x->alloc * sizeof(void *)));
    Q_CHECK_PTR(t);

    ::memcpy(t, d, DataHeaderSize + d->alloc * sizeof(void *));

    t->ref = 1;
    t->sharable = true;
    t->alloc = x->alloc;
    if (!t->alloc) {
        t->begin = 0;
        t->end = 0;
    } else {
        t->begin = x->begin;
        t->end   = x->end;
    }
    d = t;

    return x;
}

/*!
 *  Detaches the QListData by allocating new memory for a list which possibly
 *  has a different size than the copied one.
 *  Returns the old (shared) data, it is up to the caller to deref() and free()
 *  For the new data node_copy needs to be called.
 *
 *  \internal
 */
QListData::Data *QListData::detach(int alloc)
{
    Data *x = d;
    Data* t = static_cast<Data *>(qMalloc(DataHeaderSize + alloc * sizeof(void *)));
    Q_CHECK_PTR(t);

    t->ref = 1;
    t->sharable = true;
    t->alloc = alloc;
    if (!alloc) {
        t->begin = 0;
        t->end = 0;
    } else {
        t->begin = x->begin;
        t->end   = x->end;
    }
    d = t;

    return x;
}

/*!
 *  Detaches the QListData by reallocating new memory.
 *  Returns the old (shared) data, it is up to the caller to deref() and free()
 *  For the new data node_copy needs to be called.
 *
 *  \internal
 */
#if QT_VERSION >= 0x050000
#  error "Remove QListData::detach3(), it is only required for binary compatibility for 4.5.x to 4.6.x"
#endif
QListData::Data *QListData::detach3()
{
    return detach(d->alloc);
}

void QListData::realloc(int alloc)
{
    Q_ASSERT(d->ref == 1);
    Data *x = static_cast<Data *>(qRealloc(d, DataHeaderSize + alloc * sizeof(void *)));
    Q_CHECK_PTR(x);

    d = x;
    d->alloc = alloc;
    if (!alloc)
        d->begin = d->end = 0;
}

// ensures that enough space is available to append n elements
void **QListData::append(int n)
{
    Q_ASSERT(d->ref == 1);
    int e = d->end;
    if (e + n > d->alloc) {
        int b = d->begin;
        if (b - n >= 2 * d->alloc / 3) {
            // we have enough space. Just not at the end -> move it.
            e -= b;
            ::memmove(d->array, d->array + b, e * sizeof(void *));
            d->begin = 0;
        } else {
            realloc(grow(d->alloc + n));
        }
    }
    d->end = e + n;
    return d->array + e;
}

// ensures that enough space is available to append one element
void **QListData::append()
{
    return append(1);
}

// ensures that enough space is available to append the list
#if QT_VERSION >= 0x050000
#  error "Remove QListData::append(), it is only required for binary compatibility up to 4.5.x"
#endif
void **QListData::append(const QListData& l)
{
    Q_ASSERT(d->ref == 1);
    int e = d->end;
    int n = l.d->end - l.d->begin;
    if (n) {
        if (e + n > d->alloc)
            realloc(grow(e + n));
        ::memcpy(d->array + d->end, l.d->array + l.d->begin, n*sizeof(void*));
        d->end += n;
    }
    return d->array + e;
}

// ensures that enough space is available to append the list
void **QListData::append2(const QListData& l)
{
    return append(l.d->end - l.d->begin);
}

void **QListData::prepend()
{
    Q_ASSERT(d->ref == 1);
    if (d->begin == 0) {
        if (d->end >= d->alloc / 3)
            realloc(grow(d->alloc + 1));

        if (d->end < d->alloc / 3)
            d->begin = d->alloc - 2 * d->end;
        else
            d->begin = d->alloc - d->end;

        ::memmove(d->array + d->begin, d->array, d->end * sizeof(void *));
        d->end += d->begin;
    }
    return d->array + --d->begin;
}

void **QListData::insert(int i)
{
    Q_ASSERT(d->ref == 1);
    if (i <= 0)
        return prepend();
    int size = d->end - d->begin;
    if (i >= size)
        return append();

    bool leftward = false;

    if (d->begin == 0) {
        if (d->end == d->alloc) {
            // If the array is full, we expand it and move some items rightward
            realloc(grow(d->alloc + 1));
        } else {
            // If there is free space at the end of the array, we move some items rightward
        }
    } else {
        if (d->end == d->alloc) {
            // If there is free space at the beginning of the array, we move some items leftward
            leftward = true;
        } else {
            // If there is free space at both ends, we move as few items as possible
            leftward = (i < size - i);
        }
    }

    if (leftward) {
        --d->begin;
        ::memmove(d->array + d->begin, d->array + d->begin + 1, i * sizeof(void *));
    } else {
        ::memmove(d->array + d->begin + i + 1, d->array + d->begin + i,
                  (size - i) * sizeof(void *));
        ++d->end;
    }
    return d->array + d->begin + i;
}

void QListData::remove(int i)
{
    Q_ASSERT(d->ref == 1);
    i += d->begin;
    if (i - d->begin < d->end - i) {
        if (int offset = i - d->begin)
            ::memmove(d->array + d->begin + 1, d->array + d->begin, offset * sizeof(void *));
        d->begin++;
    } else {
        if (int offset = d->end - i - 1)
            ::memmove(d->array + i, d->array + i + 1, offset * sizeof(void *));
        d->end--;
    }
}

void QListData::remove(int i, int n)
{
    Q_ASSERT(d->ref == 1);
    i += d->begin;
    int middle = i + n/2;
    if (middle - d->begin < d->end - middle) {
        ::memmove(d->array + d->begin + n, d->array + d->begin,
                   (i - d->begin) * sizeof(void*));
        d->begin += n;
    } else {
        ::memmove(d->array + i, d->array + i + n,
                   (d->end - i - n) * sizeof(void*));
        d->end -= n;
    }
}

void QListData::move(int from, int to)
{
    Q_ASSERT(d->ref == 1);
    if (from == to)
        return;

    from += d->begin;
    to += d->begin;
    void *t = d->array[from];

    if (from < to) {
        if (d->end == d->alloc || 3 * (to - from) < 2 * (d->end - d->begin)) {
            ::memmove(d->array + from, d->array + from + 1, (to - from) * sizeof(void *));
        } else {
            // optimization
            if (int offset = from - d->begin)
                ::memmove(d->array + d->begin + 1, d->array + d->begin, offset * sizeof(void *));
            if (int offset = d->end - (to + 1))
                ::memmove(d->array + to + 2, d->array + to + 1, offset * sizeof(void *));
            ++d->begin;
            ++d->end;
            ++to;
        }
    } else {
        if (d->begin == 0 || 3 * (from - to) < 2 * (d->end - d->begin)) {
            ::memmove(d->array + to + 1, d->array + to, (from - to) * sizeof(void *));
        } else {
            // optimization
            if (int offset = to - d->begin)
                ::memmove(d->array + d->begin - 1, d->array + d->begin, offset * sizeof(void *));
            if (int offset = d->end - (from + 1))
                ::memmove(d->array + from, d->array + from + 1, offset * sizeof(void *));
            --d->begin;
            --d->end;
            --to;
        }
    }
    d->array[to] = t;
}

void **QListData::erase(void **xi)
{
    Q_ASSERT(d->ref == 1);
    int i = xi - (d->array + d->begin);
    remove(i);
    return d->array + d->begin + i;
}

/*! \class QList
    \brief The QList class is a template class that provides lists.

    \ingroup tools
    \ingroup shared

    \reentrant

    QList\<T\> is one of Qt's generic \l{container classes}. It
    stores a list of values and provides fast index-based access as
    well as fast insertions and removals.

    QList\<T\>, QLinkedList\<T\>, and QVector\<T\> provide similar
    functionality. Here's an overview:

    \list
    \i For most purposes, QList is the right class to use. Its
       index-based API is more convenient than QLinkedList's
       iterator-based API, and it is usually faster than
       QVector because of the way it stores its items in
       memory. It also expands to less code in your executable.
    \i If you need a real linked list, with guarantees of \l{constant
       time} insertions in the middle of the list and iterators to
       items rather than indexes, use QLinkedList.
    \i If you want the items to occupy adjacent memory positions,
       use QVector.
    \endlist


    Internally, QList\<T\> is represented as an array of pointers to
    items of type T. If T is itself a pointer type or a basic type
    that is no larger than a pointer, or if T is one of Qt's \l{shared
    classes}, then QList\<T\> stores the items directly in the pointer
    array. For lists under a thousand items, this array representation
    allows for very fast insertions in the middle, and it allows
    index-based access. Furthermore, operations like prepend() and
    append() are very fast, because QList preallocates memory at both
    ends of its internal array. (See \l{Algorithmic Complexity} for
    details.) Note, however, that for unshared list items that are
    larger than a pointer, each append or insert of a new item
    requires allocating the new item on the heap, and this per item
    allocation might make QVector a better choice in cases that do
    lots of appending or inserting, since QVector allocates memory for
    its items in a single heap allocation.

    Note that the internal array only ever gets bigger over the life
    of the list. It never shrinks. The internal array is deallocated
    by the destructor, by clear(), and by the assignment operator,
    when one list is assigned to another.

    Here's an example of a QList that stores integers and
    a QList that stores QDate values:

    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 0

    Qt includes a QStringList class that inherits QList\<QString\>
    and adds a convenience function QStringList::join().
    (QString::split() creates QStringLists
    from strings.)

    QList stores a list of items. The default constructor creates an
    empty list. To insert items into the list, you can use
    operator<<():

    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 1

    QList provides these basic functions to add, move, and remove
    items: insert(), replace(), removeAt(), move(), and swap(). In
    addition, it provides the following convenience functions:
    append(), prepend(), removeFirst(), and removeLast().

    QList uses 0-based indexes, just like C++ arrays. To access the
    item at a particular index position, you can use operator[](). On
    non-const lists, operator[]() returns a reference to the item and
    can be used on the left side of an assignment:

    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 2

    Because QList is implemented as an array of pointers, this
    operation is very fast (\l{constant time}). For read-only access,
    an alternative syntax is to use at():

    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 3

    at() can be faster than operator[](), because it never causes a
    \l{deep copy} to occur.

    A common requirement is to remove an item from a list and do
    something with it. For this, QList provides takeAt(), takeFirst(),
    and takeLast(). Here's a loop that removes the items from a list
    one at a time and calls \c delete on them:

    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 4

    Inserting and removing items at either ends of the list is very
    fast (\l{constant time} in most cases), because QList
    preallocates extra space on both sides of its internal buffer to
    allow for fast growth at both ends of the list.

    If you want to find all occurrences of a particular value in a
    list, use indexOf() or lastIndexOf(). The former searches forward
    starting from a given index position, the latter searches
    backward. Both return the index of a matching item if they find
    it; otherwise, they return -1. For example:

    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 5

    If you simply want to check whether a list contains a particular
    value, use contains(). If you want to find out how many times a
    particular value occurs in the list, use count(). If you want to
    replace all occurrences of a particular value with another, use
    replace().

    QList's value type must be an \l{assignable data type}. This
    covers most data types that are commonly used, but the compiler
    won't let you, for example, store a QWidget as a value; instead,
    store a QWidget *. A few functions have additional requirements;
    for example, indexOf() and lastIndexOf() expect the value type to
    support \c operator==().  These requirements are documented on a
    per-function basis.

    Like the other container classes, QList provides \l{Java-style
    iterators} (QListIterator and QMutableListIterator) and
    \l{STL-style iterators} (QList::const_iterator and
    QList::iterator). In practice, these are rarely used, because you
    can use indexes into the QList. QList is implemented in such a way
    that direct index-based access is just as fast as using iterators.

    QList does \e not support inserting, prepending, appending or
    replacing with references to its own values. Doing so will cause
    your application to abort with an error message.

    To make QList as efficient as possible, its member functions don't
    validate their input before using it. Except for isEmpty(), member
    functions always assume the list is \e not empty. Member functions
    that take index values as parameters always assume their index
    value parameters are in the valid range. This means QList member
    functions can fail. If you define QT_NO_DEBUG when you compile,
    failures will not be detected. If you \e don't define QT_NO_DEBUG,
    failures will be detected using Q_ASSERT() or Q_ASSERT_X() with an
    appropriate message.

    To avoid failures when your list can be empty, call isEmpty()
    before calling other member functions. If you must pass an index
    value that might not be in the valid range, check that it is less
    than the value returned by size() but \e not less than 0.

    \sa QListIterator, QMutableListIterator, QLinkedList, QVector
*/

/*!
    \fn QList<T> QList<T>::mid(int pos, int length) const

    Returns a list whose elements are copied from this list,
    starting at position \a pos. If \a length is -1 (the default), all
    elements from \a pos are copied; otherwise \a length elements (or
    all remaining elements if there are less than \a length elements)
    are copied.
*/

/*! \fn QList::QList()

    Constructs an empty list.
*/

/*! \fn QList::QList(const QList<T> &other)

    Constructs a copy of \a other.

    This operation takes \l{constant time}, because QList is
    \l{implicitly shared}. This makes returning a QList from a
    function very fast. If a shared instance is modified, it will be
    copied (copy-on-write), and that takes \l{linear time}.

    \sa operator=()
*/

/*! \fn inline QList::QList(std::initializer_list<T> args)
    \since 4.8

    Construct a list from the std::initializer_list specified by \a args.

    This constructor is only enabled if the compiler supports C++0x
*/

/*! \fn QList::~QList()

    Destroys the list. References to the values in the list and all
    iterators of this list become invalid.
*/

/*! \fn QList<T> &QList::operator=(const QList<T> &other)

    Assigns \a other to this list and returns a reference to this
    list.
*/

/*! \fn void QList::swap(QList<T> &other)
    \since 4.8

    Swaps list \a other with this list. This operation is very
    fast and never fails.
*/

/*! \fn bool QList::operator==(const QList<T> &other) const

    Returns true if \a other is equal to this list; otherwise returns
    false.

    Two lists are considered equal if they contain the same values in
    the same order.

    This function requires the value type to have an implementation of
    \c operator==().

    \sa operator!=()
*/

/*! \fn bool QList::operator!=(const QList<T> &other) const

    Returns true if \a other is not equal to this list; otherwise
    returns false.

    Two lists are considered equal if they contain the same values in
    the same order.

    This function requires the value type to have an implementation of
    \c operator==().

    \sa operator==()
*/

/*!
    \fn int QList::size() const

    Returns the number of items in the list.

    \sa isEmpty(), count()
*/

/*! \fn void QList::detach()

    \internal
*/

/*! \fn void QList::detachShared()

    \internal

    like detach(), but does nothing if we're shared_null.
    This prevents needless mallocs, and makes QList more exception safe
    in case of cleanup work done in destructors on empty lists.
*/

/*! \fn bool QList::isDetached() const

    \internal
*/

/*! \fn void QList::setSharable(bool sharable)

    \internal
*/

/*! \fn bool QList::isSharedWith(const QList<T> &other) const

    \internal
*/

/*! \fn bool QList::isEmpty() const

    Returns true if the list contains no items; otherwise returns
    false.

    \sa size()
*/

/*! \fn void QList::clear()

    Removes all items from the list.

    \sa removeAll()
*/

/*! \fn const T &QList::at(int i) const

    Returns the item at index position \a i in the list. \a i must be
    a valid index position in the list (i.e., 0 <= \a i < size()).

    This function is very fast (\l{constant time}).

    \sa value(), operator[]()
*/

/*! \fn T &QList::operator[](int i)

    Returns the item at index position \a i as a modifiable reference.
    \a i must be a valid index position in the list (i.e., 0 <= \a i <
    size()).

    This function is very fast (\l{constant time}).

    \sa at(), value()
*/

/*! \fn const T &QList::operator[](int i) const

    \overload

    Same as at().
*/

/*! \fn QList::reserve(int alloc)

    Reserve space for \a alloc elements.

    If \a alloc is smaller than the current size of the list, nothing will happen.

    Use this function to avoid repetetive reallocation of QList's internal
    data if you can predict how many elements will be appended.
    Note that the reservation applies only to the internal pointer array.

    \since 4.7
*/

/*! \fn void QList::append(const T &value)

    Inserts \a value at the end of the list.

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 6

    This is the same as list.insert(size(), \a value).

    This operation is typically very fast (\l{constant time}),
    because QList preallocates extra space on both sides of its
    internal buffer to allow for fast growth at both ends of the
    list.

    \sa operator<<(), prepend(), insert()
*/

/*! \fn void QList::append(const QList<T> &value)

    \overload

    \since 4.5

    Appends the items of the \a value list to this list.

    \sa operator<<(), operator+=()
*/

/*! \fn void QList::prepend(const T &value)

    Inserts \a value at the beginning of the list.

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 7

    This is the same as list.insert(0, \a value).

    This operation is usually very fast (\l{constant time}), because
    QList preallocates extra space on both sides of its internal
    buffer to allow for fast growth at both ends of the list.

    \sa append(), insert()
*/

/*! \fn void QList::insert(int i, const T &value)

    Inserts \a value at index position \a i in the list. If \a i
    is 0, the value is prepended to the list. If \a i is size(), the
    value is appended to the list.

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 8

    \sa append(), prepend(), replace(), removeAt()
*/

/*! \fn QList::iterator QList::insert(iterator before, const T &value)

    \overload

    Inserts \a value in front of the item pointed to by the
    iterator \a before. Returns an iterator pointing at the inserted
    item. Note that the iterator passed to the function will be
    invalid after the call; the returned iterator should be used
    instead.
*/

/*! \fn void QList::replace(int i, const T &value)

    Replaces the item at index position \a i with \a value. \a i must
    be a valid index position in the list (i.e., 0 <= \a i < size()).

    \sa operator[](), removeAt()
*/

/*!
    \fn int QList::removeAll(const T &value)

    Removes all occurrences of \a value in the list and returns the
    number of entries removed.

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 9

    This function requires the value type to have an implementation of
    \c operator==().

    \sa removeOne(), removeAt(), takeAt(), replace()
*/

/*!
    \fn bool QList::removeOne(const T &value)
    \since 4.4

    Removes the first occurrence of \a value in the list and returns
    true on success; otherwise returns false.

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 10

    This function requires the value type to have an implementation of
    \c operator==().

    \sa removeAll(), removeAt(), takeAt(), replace()
*/

/*! \fn void QList::removeAt(int i)

    Removes the item at index position \a i. \a i must be a valid
    index position in the list (i.e., 0 <= \a i < size()).

    \sa takeAt(), removeFirst(), removeLast(), removeOne()
*/

/*! \fn T QList::takeAt(int i)

    Removes the item at index position \a i and returns it. \a i must
    be a valid index position in the list (i.e., 0 <= \a i < size()).

    If you don't use the return value, removeAt() is more efficient.

    \sa removeAt(), takeFirst(), takeLast()
*/

/*! \fn T QList::takeFirst()

    Removes the first item in the list and returns it. This is the
    same as takeAt(0). This function assumes the list is not empty. To
    avoid failure, call isEmpty() before calling this function.

    This operation takes \l{constant time}.

    If you don't use the return value, removeFirst() is more
    efficient.

    \sa takeLast(), takeAt(), removeFirst()
*/

/*! \fn T QList::takeLast()

    Removes the last item in the list and returns it. This is the
    same as takeAt(size() - 1). This function assumes the list is
    not empty. To avoid failure, call isEmpty() before calling this
    function.

    This operation takes \l{constant time}.

    If you don't use the return value, removeLast() is more
    efficient.

    \sa takeFirst(), takeAt(), removeLast()
*/

/*! \fn void QList::move(int from, int to)

    Moves the item at index position \a from to index position \a to.

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 11

    This is the same as insert(\a{to}, takeAt(\a{from})).This function
    assumes that both \a from and \a to are at least 0 but less than
    size(). To avoid failure, test that both \a from and \a to are at
    least 0 and less than size().

    \sa swap(), insert(), takeAt()
*/

/*! \fn void QList::swap(int i, int j)

    Exchange the item at index position \a i with the item at index
    position \a j. This function assumes that both \a i and \a j are
    at least 0 but less than size(). To avoid failure, test that both
    \a i and \a j are at least 0 and less than size().

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 12

    \sa move()
*/

/*! \fn int QList::indexOf(const T &value, int from = 0) const

    Returns the index position of the first occurrence of \a value in
    the list, searching forward from index position \a from. Returns
    -1 if no item matched.

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 13

    This function requires the value type to have an implementation of
    \c operator==().

    Note that QList uses 0-based indexes, just like C++ arrays. Negative
    indexes are not supported with the exception of the value mentioned
    above.

    \sa lastIndexOf(), contains()
*/

/*! \fn int QList::lastIndexOf(const T &value, int from = -1) const

    Returns the index position of the last occurrence of \a value in
    the list, searching backward from index position \a from. If \a
    from is -1 (the default), the search starts at the last item.
    Returns -1 if no item matched.

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 14

    This function requires the value type to have an implementation of
    \c operator==().

    Note that QList uses 0-based indexes, just like C++ arrays. Negative
    indexes are not supported with the exception of the value mentioned
    above.

    \sa indexOf()
*/

/*! \fn QBool QList::contains(const T &value) const

    Returns true if the list contains an occurrence of \a value;
    otherwise returns false.

    This function requires the value type to have an implementation of
    \c operator==().

    \sa indexOf(), count()
*/

/*! \fn int QList::count(const T &value) const

    Returns the number of occurrences of \a value in the list.

    This function requires the value type to have an implementation of
    \c operator==().

    \sa contains(), indexOf()
*/

/*! \fn bool QList::startsWith(const T &value) const
    \since 4.5

    Returns true if this list is not empty and its first
    item is equal to \a value; otherwise returns false.

    \sa isEmpty(), contains()
*/

/*! \fn bool QList::endsWith(const T &value) const
    \since 4.5

    Returns true if this list is not empty and its last
    item is equal to \a value; otherwise returns false.

    \sa isEmpty(), contains()
*/

/*! \fn QList::iterator QList::begin()

    Returns an \l{STL-style iterator} pointing to the first item in
    the list.

    \sa constBegin(), end()
*/

/*! \fn QList::const_iterator QList::begin() const

    \overload
*/

/*! \fn QList::const_iterator QList::constBegin() const

    Returns a const \l{STL-style iterator} pointing to the first item
    in the list.

    \sa begin(), constEnd()
*/

/*! \fn QList::iterator QList::end()

    Returns an \l{STL-style iterator} pointing to the imaginary item
    after the last item in the list.

    \sa begin(), constEnd()
*/

/*! \fn const_iterator QList::end() const

    \overload
*/

/*! \fn QList::const_iterator QList::constEnd() const

    Returns a const \l{STL-style iterator} pointing to the imaginary
    item after the last item in the list.

    \sa constBegin(), end()
*/

/*! \fn QList::iterator QList::erase(iterator pos)

    Removes the item associated with the iterator \a pos from the
    list, and returns an iterator to the next item in the list (which
    may be end()).

    \sa insert(), removeAt()
*/

/*! \fn QList::iterator QList::erase(iterator begin, iterator end)

    \overload

    Removes all the items from \a begin up to (but not including) \a
    end. Returns an iterator to the same item that \a end referred to
    before the call.
*/

/*! \typedef QList::Iterator

    Qt-style synonym for QList::iterator.
*/

/*! \typedef QList::ConstIterator

    Qt-style synonym for QList::const_iterator.
*/

/*!
    \typedef QList::size_type

    Typedef for int. Provided for STL compatibility.
*/

/*!
    \typedef QList::value_type

    Typedef for T. Provided for STL compatibility.
*/

/*!
    \typedef QList::difference_type

    Typedef for ptrdiff_t. Provided for STL compatibility.
*/

/*!
    \typedef QList::pointer

    Typedef for T *. Provided for STL compatibility.
*/

/*!
    \typedef QList::const_pointer

    Typedef for const T *. Provided for STL compatibility.
*/

/*!
    \typedef QList::reference

    Typedef for T &. Provided for STL compatibility.
*/

/*!
    \typedef QList::const_reference

    Typedef for const T &. Provided for STL compatibility.
*/

/*! \fn int QList::count() const

    Returns the number of items in the list. This is effectively the
    same as size().
*/

/*! \fn int QList::length() const
    \since 4.5

    This function is identical to count().

    \sa count()
*/

/*! \fn T& QList::first()

    Returns a reference to the first item in the list. The list must
    not be empty. If the list can be empty, call isEmpty() before
    calling this function.

    \sa last(), isEmpty()
*/

/*! \fn const T& QList::first() const

    \overload
*/

/*! \fn T& QList::last()

    Returns a reference to the last item in the list.  The list must
    not be empty. If the list can be empty, call isEmpty() before
    calling this function.

    \sa first(), isEmpty()
*/

/*! \fn const T& QList::last() const

    \overload
*/

/*! \fn void QList::removeFirst()

    Removes the first item in the list. Calling this function is
    equivalent to calling removeAt(0). The list must not be empty. If
    the list can be empty, call isEmpty() before calling this
    function.

    \sa removeAt(), takeFirst()
*/

/*! \fn void QList::removeLast()

    Removes the last item in the list. Calling this function is
    equivalent to calling removeAt(size() - 1). The list must not be
    empty. If the list can be empty, call isEmpty() before calling
    this function.

    \sa removeAt(), takeLast()
*/

/*! \fn T QList::value(int i) const

    Returns the value at index position \a i in the list.

    If the index \a i is out of bounds, the function returns a
    \l{default-constructed value}. If you are certain that the index
    is going to be within bounds, you can use at() instead, which is
    slightly faster.

    \sa at(), operator[]()
*/

/*! \fn T QList::value(int i, const T &defaultValue) const

    \overload

    If the index \a i is out of bounds, the function returns
    \a defaultValue.
*/

/*! \fn void QList::push_back(const T &value)

    This function is provided for STL compatibility. It is equivalent
    to \l{QList::append()}{append(\a value)}.
*/

/*! \fn void QList::push_front(const T &value)

    This function is provided for STL compatibility. It is equivalent
    to \l{QList::prepend()}{prepend(\a value)}.
*/

/*! \fn T& QList::front()

    This function is provided for STL compatibility. It is equivalent
    to first(). The list must not be empty. If the list can be empty,
    call isEmpty() before calling this function.
*/

/*! \fn const T& QList::front() const

    \overload
*/

/*! \fn T& QList::back()

    This function is provided for STL compatibility. It is equivalent
    to last(). The list must not be empty. If the list can be empty,
    call isEmpty() before calling this function.
*/

/*! \fn const T& QList::back() const

    \overload
*/

/*! \fn void QList::pop_front()

    This function is provided for STL compatibility. It is equivalent
    to removeFirst(). The list must not be empty. If the list can be
    empty, call isEmpty() before calling this function.
*/

/*! \fn void QList::pop_back()

    This function is provided for STL compatibility. It is equivalent
    to removeLast(). The list must not be empty. If the list can be
    empty, call isEmpty() before calling this function.
*/

/*! \fn bool QList::empty() const

    This function is provided for STL compatibility. It is equivalent
    to isEmpty() and returns true if the list is empty.
*/

/*! \fn QList<T> &QList::operator+=(const QList<T> &other)

    Appends the items of the \a other list to this list and returns a
    reference to this list.

    \sa operator+(), append()
*/

/*! \fn void QList::operator+=(const T &value)

    \overload

    Appends \a value to the list.

    \sa append(), operator<<()
*/

/*! \fn QList<T> QList::operator+(const QList<T> &other) const

    Returns a list that contains all the items in this list followed
    by all the items in the \a other list.

    \sa operator+=()
*/

/*! \fn QList<T> &QList::operator<<(const QList<T> &other)

    Appends the items of the \a other list to this list and returns a
    reference to this list.

    \sa operator+=(), append()
*/

/*! \fn void QList::operator<<(const T &value)

    \overload

    Appends \a value to the list.
*/

/*! \class QList::iterator
    \brief The QList::iterator class provides an STL-style non-const iterator for QList and QQueue.

    QList features both \l{STL-style iterators} and \l{Java-style
    iterators}. The STL-style iterators are more low-level and more
    cumbersome to use; on the other hand, they are slightly faster
    and, for developers who already know STL, have the advantage of
    familiarity.

    QList\<T\>::iterator allows you to iterate over a QList\<T\> (or
    QQueue\<T\>) and to modify the list item associated with the
    iterator. If you want to iterate over a const QList, use
    QList::const_iterator instead. It is generally good practice to
    use QList::const_iterator on a non-const QList as well, unless
    you need to change the QList through the iterator. Const
    iterators are slightly faster, and can improve code readability.

    The default QList::iterator constructor creates an uninitialized
    iterator. You must initialize it using a QList function like
    QList::begin(), QList::end(), or QList::insert() before you can
    start iterating. Here's a typical loop that prints all the items
    stored in a list:

    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 15

    Let's see a few examples of things we can do with a
    QList::iterator that we cannot do with a QList::const_iterator.
    Here's an example that increments every value stored in a
    QList\<int\> by 2:

    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 16

    Most QList functions accept an integer index rather than an
    iterator. For that reason, iterators are rarely useful in
    connection with QList. One place where STL-style iterators do
    make sense is as arguments to \l{generic algorithms}.

    For example, here's how to delete all the widgets stored in a
    QList\<QWidget *\>:

    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 17

    Multiple iterators can be used on the same list. However, be
    aware that any non-const function call performed on the QList
    will render all existing iterators undefined. If you need to keep
    iterators over a long period of time, we recommend that you use
    QLinkedList rather than QList.

    \sa QList::const_iterator, QMutableListIterator
*/

/*! \typedef QList::iterator::iterator_category

  A synonym for \e {std::random_access_iterator_tag} indicating
  this iterator is a random access iterator.
*/

/*! \typedef QList::iterator::difference_type

    \internal
*/

/*! \typedef QList::iterator::value_type

    \internal
*/

/*! \typedef QList::iterator::pointer

    \internal
*/

/*! \typedef QList::iterator::reference

    \internal
*/

/*! \fn QList::iterator::iterator()

    Constructs an uninitialized iterator.

    Functions like operator*() and operator++() should not be called
    on an uninitialized iterator. Use operator=() to assign a value
    to it before using it.

    \sa QList::begin() QList::end()
*/

/*! \fn QList::iterator::iterator(Node *node)

    \internal
*/

/*! \fn QList::iterator::iterator(const iterator &other)

    Constructs a copy of \a other.
*/

/*! \fn T &QList::iterator::operator*() const

    Returns a modifiable reference to the current item.

    You can change the value of an item by using operator*() on the
    left side of an assignment, for example:

    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 18

    \sa operator->()
*/

/*! \fn T *QList::iterator::operator->() const

    Returns a pointer to the current item.

    \sa operator*()
*/

/*! \fn T &QList::iterator::operator[](int j) const

    Returns a modifiable reference to the item at position *this +
    \a{j}.

    This function is provided to make QList iterators behave like C++
    pointers.

    \sa operator+()
*/

/*!
    \fn bool QList::iterator::operator==(const iterator &other) const
    \fn bool QList::iterator::operator==(const const_iterator &other) const

    Returns true if \a other points to the same item as this
    iterator; otherwise returns false.

    \sa operator!=()
*/

/*!
    \fn bool QList::iterator::operator!=(const iterator &other) const
    \fn bool QList::iterator::operator!=(const const_iterator &other) const

    Returns true if \a other points to a different item than this
    iterator; otherwise returns false.

    \sa operator==()
*/

/*!
    \fn bool QList::iterator::operator<(const iterator& other) const
    \fn bool QList::iterator::operator<(const const_iterator& other) const

    Returns true if the item pointed to by this iterator is less than
    the item pointed to by the \a other iterator.
*/

/*!
    \fn bool QList::iterator::operator<=(const iterator& other) const
    \fn bool QList::iterator::operator<=(const const_iterator& other) const

    Returns true if the item pointed to by this iterator is less than
    or equal to the item pointed to by the \a other iterator.
*/

/*!
    \fn bool QList::iterator::operator>(const iterator& other) const
    \fn bool QList::iterator::operator>(const const_iterator& other) const

    Returns true if the item pointed to by this iterator is greater
    than the item pointed to by the \a other iterator.
*/

/*!
    \fn bool QList::iterator::operator>=(const iterator& other) const
    \fn bool QList::iterator::operator>=(const const_iterator& other) const

    Returns true if the item pointed to by this iterator is greater
    than or equal to the item pointed to by the \a other iterator.
*/

/*! \fn QList::iterator &QList::iterator::operator++()

    The prefix ++ operator (\c{++it}) advances the iterator to the
    next item in the list and returns an iterator to the new current
    item.

    Calling this function on QList::end() leads to undefined results.

    \sa operator--()
*/

/*! \fn QList::iterator QList::iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{it++}) advances the iterator to the
    next item in the list and returns an iterator to the previously
    current item.
*/

/*! \fn QList::iterator &QList::iterator::operator--()

    The prefix -- operator (\c{--it}) makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QList::begin() leads to undefined results.

    \sa operator++()
*/

/*! \fn QList::iterator QList::iterator::operator--(int)

    \overload

    The postfix -- operator (\c{it--}) makes the preceding item
    current and returns an iterator to the previously current item.
*/

/*! \fn QList::iterator &QList::iterator::operator+=(int j)

    Advances the iterator by \a j items. (If \a j is negative, the
    iterator goes backward.)

    \sa operator-=(), operator+()
*/

/*! \fn QList::iterator &QList::iterator::operator-=(int j)

    Makes the iterator go back by \a j items. (If \a j is negative,
    the iterator goes forward.)

    \sa operator+=(), operator-()
*/

/*! \fn QList::iterator QList::iterator::operator+(int j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. (If \a j is negative, the iterator goes backward.)

    \sa operator-(), operator+=()
*/

/*! \fn QList::iterator QList::iterator::operator-(int j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator. (If \a j is negative, the iterator goes forward.)

    \sa operator+(), operator-=()
*/

/*! \fn int QList::iterator::operator-(iterator other) const

    Returns the number of items between the item pointed to by \a
    other and the item pointed to by this iterator.
*/

/*! \class QList::const_iterator
    \brief The QList::const_iterator class provides an STL-style const iterator for QList and QQueue.

    QList provides both \l{STL-style iterators} and \l{Java-style
    iterators}. The STL-style iterators are more low-level and more
    cumbersome to use; on the other hand, they are slightly faster
    and, for developers who already know STL, have the advantage of
    familiarity.

    QList\<T\>::const_iterator allows you to iterate over a
    QList\<T\> (or a QQueue\<T\>). If you want to modify the QList as
    you iterate over it, use QList::iterator instead. It is generally
    good practice to use QList::const_iterator on a non-const QList
    as well, unless you need to change the QList through the
    iterator. Const iterators are slightly faster, and can improve
    code readability.

    The default QList::const_iterator constructor creates an
    uninitialized iterator. You must initialize it using a QList
    function like QList::constBegin(), QList::constEnd(), or
    QList::insert() before you can start iterating. Here's a typical
    loop that prints all the items stored in a list:

    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 19

    Most QList functions accept an integer index rather than an
    iterator. For that reason, iterators are rarely useful in
    connection with QList. One place where STL-style iterators do
    make sense is as arguments to \l{generic algorithms}.

    For example, here's how to delete all the widgets stored in a
    QList\<QWidget *\>:

    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 20

    Multiple iterators can be used on the same list. However, be
    aware that any non-const function call performed on the QList
    will render all existing iterators undefined. If you need to keep
    iterators over a long period of time, we recommend that you use
    QLinkedList rather than QList.

    \sa QList::iterator, QListIterator
*/

/*! \fn QList::const_iterator::const_iterator()

    Constructs an uninitialized iterator.

    Functions like operator*() and operator++() should not be called
    on an uninitialized iterator. Use operator=() to assign a value
    to it before using it.

    \sa QList::constBegin() QList::constEnd()
*/

/*! \typedef QList::const_iterator::iterator_category

  A synonym for \e {std::random_access_iterator_tag} indicating
  this iterator is a random access iterator.
*/

/*! \typedef QList::const_iterator::difference_type

    \internal
*/

/*! \typedef QList::const_iterator::value_type

    \internal
*/

/*! \typedef QList::const_iterator::pointer

    \internal
*/

/*! \typedef QList::const_iterator::reference

    \internal
*/

/*! \fn QList::const_iterator::const_iterator(Node *node)

    \internal
*/

/*! \fn QList::const_iterator::const_iterator(const const_iterator &other)

    Constructs a copy of \a other.
*/

/*! \fn QList::const_iterator::const_iterator(const iterator &other)

    Constructs a copy of \a other.
*/

/*! \fn const T &QList::const_iterator::operator*() const

    Returns the current item.

    \sa operator->()
*/

/*! \fn const T *QList::const_iterator::operator->() const

    Returns a pointer to the current item.

    \sa operator*()
*/

/*! \fn const T &QList::const_iterator::operator[](int j) const

    Returns the item at position *this + \a{j}.

    This function is provided to make QList iterators behave like C++
    pointers.

    \sa operator+()
*/

/*! \fn bool QList::const_iterator::operator==(const const_iterator &other) const

    Returns true if \a other points to the same item as this
    iterator; otherwise returns false.

    \sa operator!=()
*/

/*! \fn bool QList::const_iterator::operator!=(const const_iterator &other) const

    Returns true if \a other points to a different item than this
    iterator; otherwise returns false.

    \sa operator==()
*/

/*!
    \fn bool QList::const_iterator::operator<(const const_iterator& other) const

    Returns true if the item pointed to by this iterator is less than
    the item pointed to by the \a other iterator.
*/

/*!
    \fn bool QList::const_iterator::operator<=(const const_iterator& other) const

    Returns true if the item pointed to by this iterator is less than
    or equal to the item pointed to by the \a other iterator.
*/

/*!
    \fn bool QList::const_iterator::operator>(const const_iterator& other) const

    Returns true if the item pointed to by this iterator is greater
    than the item pointed to by the \a other iterator.
*/

/*!
    \fn bool QList::const_iterator::operator>=(const const_iterator& other) const

    Returns true if the item pointed to by this iterator is greater
    than or equal to the item pointed to by the \a other iterator.
*/

/*! \fn QList::const_iterator &QList::const_iterator::operator++()

    The prefix ++ operator (\c{++it}) advances the iterator to the
    next item in the list and returns an iterator to the new current
    item.

    Calling this function on QList::end() leads to undefined results.

    \sa operator--()
*/

/*! \fn QList::const_iterator QList::const_iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{it++}) advances the iterator to the
    next item in the list and returns an iterator to the previously
    current item.
*/

/*! \fn QList::const_iterator &QList::const_iterator::operator--()

    The prefix -- operator (\c{--it}) makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QList::begin() leads to undefined results.

    \sa operator++()
*/

/*! \fn QList::const_iterator QList::const_iterator::operator--(int)

    \overload

    The postfix -- operator (\c{it--}) makes the preceding item
    current and returns an iterator to the previously current item.
*/

/*! \fn QList::const_iterator &QList::const_iterator::operator+=(int j)

    Advances the iterator by \a j items. (If \a j is negative, the
    iterator goes backward.)

    \sa operator-=(), operator+()
*/

/*! \fn QList::const_iterator &QList::const_iterator::operator-=(int j)

    Makes the iterator go back by \a j items. (If \a j is negative,
    the iterator goes forward.)

    \sa operator+=(), operator-()
*/

/*! \fn QList::const_iterator QList::const_iterator::operator+(int j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. (If \a j is negative, the iterator goes backward.)

    \sa operator-(), operator+=()
*/

/*! \fn QList::const_iterator QList::const_iterator::operator-(int j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator. (If \a j is negative, the iterator goes forward.)

    \sa operator+(), operator-=()
*/

/*! \fn int QList::const_iterator::operator-(const_iterator other) const

    Returns the number of items between the item pointed to by \a
    other and the item pointed to by this iterator.
*/

/*! \fn QDataStream &operator<<(QDataStream &out, const QList<T> &list)
    \relates QList

    Writes the list \a list to stream \a out.

    This function requires the value type to implement \c
    operator<<().

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

/*! \fn QDataStream &operator>>(QDataStream &in, QList<T> &list)
    \relates QList

    Reads a list from stream \a in into \a list.

    This function requires the value type to implement \c
    operator>>().

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

/*!
    \fn iterator QList::remove(iterator pos)

    Use erase() instead.
*/

/*!
    \fn int QList::remove(const T &t)

    Use removeAll() instead.
*/

/*!
    \fn int QList::findIndex(const T& t) const

    Use indexOf() instead.
*/

/*!
    \fn iterator QList::find(const T& t)

    Use indexOf() instead.
*/

/*!
    \fn const_iterator QList::find (const T& t) const

    Use indexOf() instead.
*/

/*!
    \fn iterator QList::find(iterator from, const T& t)

    Use indexOf() instead.
*/

/*!
    \fn const_iterator QList::find(const_iterator from, const T& t) const

    Use indexOf() instead.
*/

/*! \fn QList<T> QList<T>::fromVector(const QVector<T> &vector)

    Returns a QList object with the data contained in \a vector.

    Example:

    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 21

    \sa fromSet(), toVector(), QVector::toList()
*/

/*! \fn QVector<T> QList<T>::toVector() const

    Returns a QVector object with the data contained in this QList.

    Example:

    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 22

    \sa toSet(), fromVector(), QVector::fromList()
*/

/*! \fn QList<T> QList<T>::fromSet(const QSet<T> &set)

    Returns a QList object with the data contained in \a set. The
    order of the elements in the QList is undefined.

    Example:

    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 23

    \sa fromVector(), toSet(), QSet::toList(), qSort()
*/

/*! \fn QSet<T> QList<T>::toSet() const

    Returns a QSet object with the data contained in this QList.
    Since QSet doesn't allow duplicates, the resulting QSet might be
    smaller than the original list was.

    Example:

    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 24

    \sa toVector(), fromSet(), QSet::fromList()
*/

/*! \fn QList<T> QList<T>::fromStdList(const std::list<T> &list)

    Returns a QList object with the data contained in \a list. The
    order of the elements in the QList is the same as in \a list.

    Example:

    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 25

    \sa toStdList(), QVector::fromStdVector()
*/

/*! \fn std::list<T> QList<T>::toStdList() const

    Returns a std::list object with the data contained in this QList.
    Example:

    \snippet doc/src/snippets/code/src_corelib_tools_qlistdata.cpp 26

    \sa fromStdList(), QVector::toStdVector()
*/

QT_END_NAMESPACE
