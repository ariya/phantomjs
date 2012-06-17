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

#include "qlinkedlist.h"

QT_BEGIN_NAMESPACE

QLinkedListData QLinkedListData::shared_null = {
    &QLinkedListData::shared_null, &QLinkedListData::shared_null,
    Q_BASIC_ATOMIC_INITIALIZER(1), 0, true
};

/*! \class QLinkedList
    \brief The QLinkedList class is a template class that provides linked lists.

    \ingroup tools
    \ingroup shared

    \reentrant

    QLinkedList\<T\> is one of Qt's generic \l{container classes}. It
    stores a list of values and provides iterator-based access as
    well as \l{constant time} insertions and removals.

    QList\<T\>, QLinkedList\<T\>, and QVector\<T\> provide similar
    functionality. Here's an overview:

    \list
    \i For most purposes, QList is the right class to use. Its
       index-based API is more convenient than QLinkedList's
       iterator-based API, and it is usually faster than
       QVector because of the way it stores its items in
       memory (see \l{Algorithmic Complexity} for details).
       It also expands to less code in your executable.
    \i If you need a real linked list, with guarantees of \l{constant
       time} insertions in the middle of the list and iterators to
       items rather than indexes, use QLinkedList.
    \i If you want the items to occupy adjacent memory positions,
       use QVector.
    \endlist

    Here's an example of a QLinkedList that stores integers and a
    QLinkedList that stores QTime values:

    \snippet doc/src/snippets/code/src_corelib_tools_qlinkedlist.cpp 0

    QLinkedList stores a list of items. The default constructor
    creates an empty list. To insert items into the list, you can use
    operator<<():

    \snippet doc/src/snippets/code/src_corelib_tools_qlinkedlist.cpp 1

    If you want to get the first or last item in a linked list, use
    first() or last(). If you want to remove an item from either end
    of the list, use removeFirst() or removeLast(). If you want to
    remove all occurrences of a given value in the list, use
    removeAll().

    A common requirement is to remove the first or last item in the
    list and do something with it. For this, QLinkedList provides
    takeFirst() and takeLast(). Here's a loop that removes the items
    from a list one at a time and calls \c delete on them:
    \snippet doc/src/snippets/code/src_corelib_tools_qlinkedlist.cpp 2

    QLinkedList's value type must be an \l {assignable data type}. This
    covers most data types that are commonly used, but the compiler
    won't let you, for example, store a QWidget as a value; instead,
    store a QWidget *. A few functions have additional requirements;
    for example, contains() and removeAll() expect the value type to
    support \c operator==().  These requirements are documented on a
    per-function basis.

    If you want to insert, modify, or remove items in the middle of
    the list, you must use an iterator. QLinkedList provides both
    \l{Java-style iterators} (QLinkedListIterator and
    QMutableLinkedListIterator) and \l{STL-style iterators}
    (QLinkedList::const_iterator and QLinkedList::iterator). See the
    documentation for these classes for details.

    \sa QLinkedListIterator, QMutableLinkedListIterator, QList, QVector
*/

/*! \fn QLinkedList::QLinkedList()

    Constructs an empty list.
*/

/*! \fn QLinkedList::QLinkedList(const QLinkedList<T> &other)

    Constructs a copy of \a other.

    This operation occurs in \l{constant time}, because QLinkedList
    is \l{implicitly shared}. This makes returning a QLinkedList from
    a function very fast. If a shared instance is modified, it will
    be copied (copy-on-write), and this takes \l{linear time}.

    \sa operator=()
*/

/*! \fn QLinkedList::~QLinkedList()

    Destroys the list. References to the values in the list, and all
    iterators over this list, become invalid.
*/

/*! \fn QLinkedList<T> &QLinkedList::operator=(const QLinkedList<T> &other)

    Assigns \a other to this list and returns a reference to this
    list.
*/

/*! \fn void QLinkedList::swap(QLinkedList<T> &other)
    \since 4.8

    Swaps list \a other with this list. This operation is very
    fast and never fails.
*/

/*! \fn bool QLinkedList::operator==(const QLinkedList<T> &other) const

    Returns true if \a other is equal to this list; otherwise returns
    false.

    Two lists are considered equal if they contain the same values in
    the same order.

    This function requires the value type to implement \c
    operator==().

    \sa operator!=()
*/

/*! \fn bool QLinkedList::operator!=(const QLinkedList<T> &other) const

    Returns true if \a other is not equal to this list; otherwise
    returns false.

    Two lists are considered equal if they contain the same values in
    the same order.

    This function requires the value type to implement \c
    operator==().

    \sa operator==()
*/

/*! \fn int QLinkedList::size() const

    Returns the number of items in the list.

    \sa isEmpty(), count()
*/

/*! \fn void QLinkedList::detach()

    \internal
*/

/*! \fn bool QLinkedList::isDetached() const

    \internal
*/

/*! \fn void QLinkedList::setSharable(bool sharable)

    \internal
*/

/*! \fn bool QLinkedList::isSharedWith(const QLinkedList<T> &other) const

    \internal
*/

/*! \fn bool QLinkedList::isEmpty() const

    Returns true if the list contains no items; otherwise returns
    false.

    \sa size()
*/

/*! \fn void QLinkedList::clear()

    Removes all the items in the list.

    \sa removeAll()
*/

/*! \fn void QLinkedList::append(const T &value)

    Inserts \a value at the end of the list.

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qlinkedlist.cpp 3

    This is the same as list.insert(end(), \a value).

    \sa operator<<(), prepend(), insert()
*/

/*! \fn void QLinkedList::prepend(const T &value)

    Inserts \a value at the beginning of the list.

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qlinkedlist.cpp 4

    This is the same as list.insert(begin(), \a value).

    \sa append(), insert()
*/

/*! \fn int QLinkedList::removeAll(const T &value)

    Removes all occurrences of \a value in the list.

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qlinkedlist.cpp 5

    This function requires the value type to have an implementation of
    \c operator==().

    \sa insert()
*/

/*!
    \fn bool QLinkedList::removeOne(const T &value)
    \since 4.4

    Removes the first occurrences of \a value in the list. Returns true on
    success; otherwise returns false.

    Example:
    \snippet doc/src/snippets/code/src_corelib_tools_qlinkedlist.cpp 6

    This function requires the value type to have an implementation of
    \c operator==().

    \sa insert()
*/

/*! \fn bool QLinkedList::contains(const T &value) const

    Returns true if the list contains an occurrence of \a value;
    otherwise returns false.

    This function requires the value type to have an implementation of
    \c operator==().

    \sa QLinkedListIterator::findNext(), QLinkedListIterator::findPrevious()
*/

/*! \fn int QLinkedList::count(const T &value) const

    Returns the number of occurrences of \a value in the list.

    This function requires the value type to have an implementation of
    \c operator==().

    \sa contains()
*/

/*! \fn bool QLinkedList::startsWith(const T &value) const
    \since 4.5

    Returns true if the list is not empty and its first
    item is equal to \a value; otherwise returns false.

    \sa isEmpty(), first()
*/

/*! \fn bool QLinkedList::endsWith(const T &value) const
    \since 4.5

    Returns true if the list is not empty and its last
    item is equal to \a value; otherwise returns false.

    \sa isEmpty(), last()
*/

/*! \fn QLinkedList::iterator QLinkedList::begin()

    Returns an \l{STL-style iterator} pointing to the first item in
    the list.

    \sa constBegin(), end()
*/

/*! \fn QLinkedList::const_iterator QLinkedList::begin() const

    \overload
*/

/*! \fn QLinkedList::const_iterator QLinkedList::constBegin() const

    Returns a const \l{STL-style iterator} pointing to the first item
    in the list.

    \sa begin(), constEnd()
*/

/*! \fn QLinkedList::iterator QLinkedList::end()

    Returns an \l{STL-style iterator} pointing to the imaginary item
    after the last item in the list.

    \sa begin(), constEnd()
*/

/*! \fn QLinkedList::const_iterator QLinkedList::end() const

    \overload
*/

/*! \fn QLinkedList::const_iterator QLinkedList::constEnd() const

    Returns a const \l{STL-style iterator} pointing to the imaginary
    item after the last item in the list.

    \sa constBegin(), end()
*/

/*! \fn QLinkedList::iterator QLinkedList::insert(iterator before, const T &value)

    Inserts \a value in front of the item pointed to by the iterator
    \a before. Returns an iterator pointing at the inserted item.

    \sa erase()
*/

/*! \fn QLinkedList::iterator QLinkedList::erase(iterator pos)

    Removes the item pointed to by the iterator \a pos from the list,
    and returns an iterator to the next item in the list (which may be
    end()).

    \sa insert()
*/

/*! \fn QLinkedList::iterator QLinkedList::erase(iterator begin, iterator end)

    \overload

    Removes all the items from \a begin up to (but not including) \a
    end.
*/

/*! \typedef QLinkedList::Iterator

    Qt-style synonym for QLinkedList::iterator.
*/

/*! \typedef QLinkedList::ConstIterator

    Qt-style synonym for QLinkedList::const_iterator.
*/

/*!
    \typedef QLinkedList::size_type

    Typedef for int. Provided for STL compatibility.
*/

/*!
    \typedef QLinkedList::value_type

    Typedef for T. Provided for STL compatibility.
*/

/*!
    \typedef QLinkedList::pointer

    Typedef for T *. Provided for STL compatibility.
*/

/*!
    \typedef QLinkedList::const_pointer

    Typedef for const T *. Provided for STL compatibility.
*/

/*!
    \typedef QLinkedList::reference

    Typedef for T &. Provided for STL compatibility.
*/

/*!
    \typedef QLinkedList::const_reference

    Typedef for const T &. Provided for STL compatibility.
*/

/*!
    \typedef QLinkedList::difference_type

    Typedef for ptrdiff_t. Provided for STL compatibility.
*/

/*! \fn int QLinkedList::count() const

    Same as size().
*/

/*! \fn T& QLinkedList::first()

    Returns a reference to the first item in the list. This function
    assumes that the list isn't empty.

    \sa last(), isEmpty()
*/

/*! \fn const T& QLinkedList::first() const

    \overload
*/

/*! \fn T& QLinkedList::last()

    Returns a reference to the last item in the list. This function
    assumes that the list isn't empty.

    \sa first(), isEmpty()
*/

/*! \fn const T& QLinkedList::last() const

    \overload
*/

/*! \fn void QLinkedList::removeFirst()

    Removes the first item in the list.

    This is the same as erase(begin()).

    \sa removeLast(), erase()
*/

/*! \fn void QLinkedList::removeLast()

    Removes the last item in the list.

    \sa removeFirst(), erase()
*/

/*! \fn T QLinkedList::takeFirst()

    Removes the first item in the list and returns it.

    If you don't use the return value, removeFirst() is more
    efficient.

    \sa takeLast(), removeFirst()
*/

/*! \fn T QLinkedList::takeLast()

    Removes the last item in the list and returns it.

    If you don't use the return value, removeLast() is more
    efficient.

    \sa takeFirst(), removeLast()
*/

/*! \fn void QLinkedList::push_back(const T &value)

    This function is provided for STL compatibility. It is equivalent
    to append(\a value).
*/

/*! \fn void QLinkedList::push_front(const T &value)

    This function is provided for STL compatibility. It is equivalent
    to prepend(\a value).
*/

/*! \fn T& QLinkedList::front()

    This function is provided for STL compatibility. It is equivalent
    to first().
*/

/*! \fn const T& QLinkedList::front() const

    \overload
*/

/*! \fn T& QLinkedList::back()

    This function is provided for STL compatibility. It is equivalent
    to last().
*/

/*! \fn const T& QLinkedList::back() const

    \overload
*/

/*! \fn void QLinkedList::pop_front()

    This function is provided for STL compatibility. It is equivalent
    to removeFirst().
*/

/*! \fn void QLinkedList::pop_back()

    This function is provided for STL compatibility. It is equivalent
    to removeLast().
*/

/*! \fn bool QLinkedList::empty() const

    This function is provided for STL compatibility. It is equivalent
    to isEmpty() and returns true if the list is empty.
*/

/*! \fn QLinkedList<T> &QLinkedList::operator+=(const QLinkedList<T> &other)

    Appends the items of the \a other list to this list and returns a
    reference to this list.

    \sa operator+(), append()
*/

/*! \fn void QLinkedList::operator+=(const T &value)

    \overload

    Appends \a value to the list.
*/

/*! \fn QLinkedList<T> QLinkedList::operator+(const QLinkedList<T> &other) const

    Returns a list that contains all the items in this list followed
    by all the items in the \a other list.

    \sa operator+=()
*/

/*! \fn QLinkedList<T> &QLinkedList::operator<<(const QLinkedList<T> &other)

    Appends the items of the \a other list to this list and returns a
    reference to this list.

    \sa operator+=(), append()
*/

/*! \fn QLinkedList<T> &QLinkedList::operator<<(const T &value)

    \overload

    Appends \a value to the list.
*/

/*! \class QLinkedList::iterator
    \brief The QLinkedList::iterator class provides an STL-style non-const iterator for QLinkedList.

    QLinkedList features both \l{STL-style iterators} and
    \l{Java-style iterators}. The STL-style iterators are more
    low-level and more cumbersome to use; on the other hand, they are
    slightly faster and, for developers who already know STL, have
    the advantage of familiarity.

    QLinkedList\<T\>::iterator allows you to iterate over a
    QLinkedList\<T\> and to modify the list item associated with the
    iterator. If you want to iterate over a const QLinkedList, use
    QLinkedList::const_iterator instead. It is generally good
    practice to use QLinkedList::const_iterator on a non-const
    QLinkedList as well, unless you need to change the QLinkedList
    through the iterator. Const iterators are slightly faster, and
    can improve code readability.

    The default QLinkedList::iterator constructor creates an
    uninitialized iterator. You must initialize it using a
    function like QLinkedList::begin(), QLinkedList::end(), or
    QLinkedList::insert() before you can start iterating. Here's a
    typical loop that prints all the items stored in a list:

    \snippet doc/src/snippets/code/src_corelib_tools_qlinkedlist.cpp 7

    STL-style iterators can be used as arguments to \l{generic
    algorithms}. For example, here's how to find an item in the list
    using the qFind() algorithm:

    \snippet doc/src/snippets/code/src_corelib_tools_qlinkedlist.cpp 8

    Let's see a few examples of things we can do with a
    QLinkedList::iterator that we cannot do with a QLinkedList::const_iterator.
    Here's an example that increments every value stored in a
    QLinkedList\<int\> by 2:

    \snippet doc/src/snippets/code/src_corelib_tools_qlinkedlist.cpp 9

    Here's an example that removes all the items that start with an
    underscore character in a QLinkedList\<QString\>:

    \snippet doc/src/snippets/code/src_corelib_tools_qlinkedlist.cpp 10

    The call to QLinkedList::erase() removes the item pointed to by
    the iterator from the list, and returns an iterator to the next
    item. Here's another way of removing an item while iterating:

    \snippet doc/src/snippets/code/src_corelib_tools_qlinkedlist.cpp 11

    It might be tempting to write code like this:

    \snippet doc/src/snippets/code/src_corelib_tools_qlinkedlist.cpp 12

    However, this will potentially crash in \c{++i}, because \c i is
    a dangling iterator after the call to erase().

    Multiple iterators can be used on the same list. If you add items
    to the list, existing iterators will remain valid. If you remove
    items from the list, iterators that point to the removed items
    will become dangling iterators. However, because of how \l{implicit
    sharing} works, you must not take a copy of a container while
    iterators are active on that container.

    \sa QLinkedList::const_iterator, QMutableLinkedListIterator
*/

/*! \fn QLinkedList::iterator::iterator()

    Constructs an uninitialized iterator.

    Functions like operator*() and operator++() should not be called
    on an uninitialized iterator. Use operator=() to assign a value
    to it before using it.

    \sa QLinkedList::begin() QLinkedList::end()
*/

/*! \fn QLinkedList::iterator::iterator(Node *node)

    \internal
*/

/*! \typedef QLinkedList::iterator::iterator_category

    \internal
*/

/*! \typedef QLinkedList::iterator::difference_type

    \internal
*/

/*! \typedef QLinkedList::iterator::value_type

    \internal
*/

/*! \typedef QLinkedList::iterator::pointer

    \internal
*/

/*! \typedef QLinkedList::iterator::reference

    \internal
*/

/*! \fn QLinkedList::iterator::iterator(const iterator &other)

    Constructs a copy of \a other.
*/

/*! \fn QLinkedList::iterator &QLinkedList::iterator::operator=(const iterator &other)

    Assigns \a other to this iterator.
*/

/*! \fn T &QLinkedList::iterator::operator*() const

    Returns a modifiable reference to the current item.

    You can change the value of an item by using operator*() on the
    left side of an assignment, for example:

    \snippet doc/src/snippets/code/src_corelib_tools_qlinkedlist.cpp 13

    \sa operator->()
*/

/*! \fn T *QLinkedList::iterator::operator->() const

    Returns a pointer to the current item.

    \sa operator*()
*/

/*!
    \fn bool QLinkedList::iterator::operator==(const iterator &other) const
    \fn bool QLinkedList::iterator::operator==(const const_iterator &other) const

    Returns true if \a other points to the same item as this
    iterator; otherwise returns false.

    \sa operator!=()
*/

/*!
    \fn bool QLinkedList::iterator::operator!=(const iterator &other) const
    \fn bool QLinkedList::iterator::operator!=(const const_iterator &other) const

    Returns true if \a other points to a different item than this
    iterator; otherwise returns false.

    \sa operator==()
*/

/*! \fn QLinkedList::iterator &QLinkedList::iterator::operator++()

    The prefix ++ operator (\c{++it}) advances the iterator to the
    next item in the list and returns an iterator to the new current
    item.

    Calling this function on QLinkedList::end() leads to undefined
    results.

    \sa operator--()
*/

/*! \fn QLinkedList::iterator QLinkedList::iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{it++}) advances the iterator to the
    next item in the list and returns an iterator to the previously
    current item.
*/

/*! \fn QLinkedList::iterator &QLinkedList::iterator::operator--()

    The prefix -- operator (\c{--it}) makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QLinkedList::begin() leads to undefined
    results.

    \sa operator++()
*/

/*! \fn QLinkedList::iterator QLinkedList::iterator::operator--(int)

    \overload

    The postfix -- operator (\c{it--}) makes the preceding item
    current and returns an iterator to the previously current item.
*/

/*! \fn QLinkedList::iterator QLinkedList::iterator::operator+(int j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. (If \a j is negative, the iterator goes backward.)

    This operation can be slow for large \a j values.

    \sa operator-()

*/

/*! \fn QLinkedList::iterator QLinkedList::iterator::operator-(int j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator. (If \a j is negative, the iterator goes forward.)

    This operation can be slow for large \a j values.

    \sa operator+()
*/

/*! \fn QLinkedList::iterator &QLinkedList::iterator::operator+=(int j)

    Advances the iterator by \a j items. (If \a j is negative, the
    iterator goes backward.)

    \sa operator-=(), operator+()
*/

/*! \fn QLinkedList::iterator &QLinkedList::iterator::operator-=(int j)

    Makes the iterator go back by \a j items. (If \a j is negative,
    the iterator goes forward.)

    \sa operator+=(), operator-()
*/

/*! \class QLinkedList::const_iterator
    \brief The QLinkedList::const_iterator class provides an STL-style const iterator for QLinkedList.

    QLinkedList features both \l{STL-style iterators} and
    \l{Java-style iterators}. The STL-style iterators are more
    low-level and more cumbersome to use; on the other hand, they are
    slightly faster and, for developers who already know STL, have
    the advantage of familiarity.

    QLinkedList\<T\>::const_iterator allows you to iterate over a
    QLinkedList\<T\>. If you want modify the QLinkedList as you iterate
    over it, you must use QLinkedList::iterator instead. It is
    generally good practice to use QLinkedList::const_iterator on a
    non-const QLinkedList as well, unless you need to change the
    QLinkedList through the iterator. Const iterators are slightly
    faster, and can improve code readability.

    The default QLinkedList::const_iterator constructor creates an
    uninitialized iterator. You must initialize it using a function
    like QLinkedList::constBegin(), QLinkedList::constEnd(), or
    QLinkedList::insert() before you can start iterating. Here's a
    typical loop that prints all the items stored in a list:

    \snippet doc/src/snippets/code/src_corelib_tools_qlinkedlist.cpp 14

    STL-style iterators can be used as arguments to \l{generic
    algorithms}. For example, here's how to find an item in the list
    using the qFind() algorithm:

    \snippet doc/src/snippets/code/src_corelib_tools_qlinkedlist.cpp 15

    Multiple iterators can be used on the same list. If you add items
    to the list, existing iterators will remain valid. If you remove
    items from the list, iterators that point to the removed items
    will become dangling iterators.

    \sa QLinkedList::iterator, QLinkedListIterator
*/

/*! \fn QLinkedList::const_iterator::const_iterator()

    Constructs an uninitialized iterator.

    Functions like operator*() and operator++() should not be called
    on an uninitialized iterator. Use operator=() to assign a value
    to it before using it.

    \sa QLinkedList::constBegin() QLinkedList::constEnd()
*/

/*! \fn QLinkedList::const_iterator::const_iterator(Node *node)

    \internal
*/

/*! \typedef QLinkedList::const_iterator::iterator_category

    \internal
*/

/*! \typedef QLinkedList::const_iterator::difference_type

    \internal
*/

/*! \typedef QLinkedList::const_iterator::value_type

    \internal
*/

/*! \typedef QLinkedList::const_iterator::pointer

    \internal
*/

/*! \typedef QLinkedList::const_iterator::reference

    \internal
*/

/*! \fn QLinkedList::const_iterator::const_iterator(const const_iterator &other)

    Constructs a copy of \a other.
*/

/*! \fn QLinkedList::const_iterator::const_iterator(iterator other)

    Constructs a copy of \a other.
*/

/*! \fn QLinkedList::const_iterator &QLinkedList::const_iterator::operator=( \
            const const_iterator &other)

    Assigns \a other to this iterator.
*/

/*! \fn const T &QLinkedList::const_iterator::operator*() const

    Returns a reference to the current item.

    \sa operator->()
*/

/*! \fn const T *QLinkedList::const_iterator::operator->() const

    Returns a pointer to the current item.

    \sa operator*()
*/

/*! \fn bool QLinkedList::const_iterator::operator==(const const_iterator &other) const

    Returns true if \a other points to the same item as this
    iterator; otherwise returns false.

    \sa operator!=()
*/

/*! \fn bool QLinkedList::const_iterator::operator!=(const const_iterator &other) const

    Returns true if \a other points to a different item than this
    iterator; otherwise returns false.

    \sa operator==()
*/

/*! \fn QLinkedList::const_iterator &QLinkedList::const_iterator::operator++()

    The prefix ++ operator (\c{++it}) advances the iterator to the
    next item in the list and returns an iterator to the new current
    item.

    Calling this function on QLinkedList::constEnd() leads to
    undefined results.

    \sa operator--()
*/

/*! \fn QLinkedList::const_iterator QLinkedList::const_iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{it++}) advances the iterator to the
    next item in the list and returns an iterator to the previously
    current item.
*/

/*! \fn QLinkedList::const_iterator &QLinkedList::const_iterator::operator--()

    The prefix -- operator (\c{--it}) makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QLinkedList::begin() leads to undefined
    results.

    \sa operator++()
*/

/*! \fn QLinkedList::const_iterator QLinkedList::const_iterator::operator--(int)

    \overload

    The postfix -- operator (\c{it--}) makes the preceding item
    current and returns an iterator to the previously current item.
*/

/*! \fn QLinkedList::const_iterator QLinkedList::const_iterator::operator+(int j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. (If \a j is negative, the iterator goes backward.)

    This operation can be slow for large \a j values.

    \sa operator-()
*/

/*! \fn QLinkedList::const_iterator QLinkedList::const_iterator::operator-(int j) const

    This function returns an iterator to the item at \a j positions backward from
    this iterator. (If \a j is negative, the iterator goes forward.)

    This operation can be slow for large \a j values.

    \sa operator+()
*/

/*! \fn QLinkedList::const_iterator &QLinkedList::const_iterator::operator+=(int j)

    Advances the iterator by \a j items. (If \a j is negative, the
    iterator goes backward.)

    This operation can be slow for large \a j values.

    \sa operator-=(), operator+()
*/

/*! \fn QLinkedList::const_iterator &QLinkedList::const_iterator::operator-=(int j)

    Makes the iterator go back by \a j items. (If \a j is negative,
    the iterator goes forward.)

    This operation can be slow for large \a j values.

    \sa operator+=(), operator-()
*/

/*! \fn QDataStream &operator<<(QDataStream &out, const QLinkedList<T> &list)
    \relates QLinkedList

    Writes the linked list \a list to stream \a out.

    This function requires the value type to implement \c
    operator<<().

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

/*! \fn QDataStream &operator>>(QDataStream &in, QLinkedList<T> &list)
    \relates QLinkedList

    Reads a linked list from stream \a in into \a list.

    This function requires the value type to implement \c operator>>().

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

/*!
    \fn iterator QLinkedList::remove(iterator pos)

    Use erase() instead.
*/

/*!
    \fn int QLinkedList::findIndex(const T& t) const

    If you need indexes then QList or QVector are better choices than
    QLinkedList.

    \oldcode
    int index = list->findIndex(value);
    \newcode
    int index = 0;
    bool found = false;
    for (const_iterator i = list->begin(); i != list->end(); ++i; ++index)
        if (*i == value) {
            found = true;
            break;
        }
    if (!found)
        index = -1;
    \endcode
*/

/*!
    \fn iterator QLinkedList::find(iterator from, const T& t)

    If you need random access to a data structure then QList, QVector,
    QMap, or QHash, are all better choices than QLinkedList.

    \oldcode
    QLinkedList::iterator i = list->find(from, value);
    \newcode
    QLinkedList::iterator i = from;
    while (i != list->end() && *i != value)
        ++i;
    \endcode
*/

/*!
    \fn iterator QLinkedList::find(const T& t)

    If you need random access to a data structure then QList, QVector,
    QMap, or QHash, are all better choices than QLinkedList.

    \oldcode
    QLinkedList::iterator i = list->find(value);
    \newcode
    QLinkedList::iterator i = list->begin();
    while (i != list->end() && *i != value)
        ++i;
    \endcode
*/

/*!
    \fn const_iterator QLinkedList::find(const_iterator from, const T& t) const

    If you need random access to a data structure then QList, QVector,
    QMap, or QHash, are all better choices than QLinkedList.

    \oldcode
    QLinkedList::const_iterator i = list->find(from, value);
    \newcode
    QLinkedList::const_iterator i = from;
    while (i != list->end() && *i != value)
        ++i;
    \endcode
*/

/*!
    \fn const_iterator QLinkedList::find(const T& t) const

    If you need random access to a data structure then QList, QVector,
    QMap, or QHash, are all better choices than QLinkedList.

    \oldcode
    QLinkedList::const_iterator i = list->find(value);
    \newcode
    QLinkedList::const_iterator i = list->begin();
    while (i != list->end() && *i != value)
        ++i;
    \endcode
*/

/*!
    \since 4.1
    \fn QLinkedList<T> QLinkedList<T>::fromStdList(const std::list<T> &list)

    Returns a QLinkedList object with the data contained in \a list.
    The order of the elements in the QLinkedList is the same as in \a
    list.

    Example:

    \snippet doc/src/snippets/code/src_corelib_tools_qlinkedlist.cpp 16

    \sa toStdList()
*/

/*!
    \since 4.1
    \fn std::list<T> QLinkedList<T>::toStdList() const

    Returns a std::list object with the data contained in this
    QLinkedList. Example:

    \snippet doc/src/snippets/code/src_corelib_tools_qlinkedlist.cpp 17

    \sa fromStdList()
*/

QT_END_NAMESPACE
