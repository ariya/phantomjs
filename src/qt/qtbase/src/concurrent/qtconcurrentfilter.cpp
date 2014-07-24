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

/*!
    \page qtconcurrentfilter.html
    \title Concurrent Filter and Filter-Reduce
    \ingroup thread

    The QtConcurrent::filter(), QtConcurrent::filtered() and
    QtConcurrent::filteredReduced() functions filter items in a sequence such
    as a QList or a QVector in parallel. QtConcurrent::filter() modifies a
    sequence in-place, QtConcurrent::filtered() returns a new sequence
    containing the filtered content, and QtConcurrent::filteredReduced()
    returns a single result.

    These functions are a part of the \l {Qt Concurrent} framework.

    Each of the above functions have a blocking variant that returns the final
    result instead of a QFuture. You use them in the same way as the
    asynchronous variants.

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 6

    Note that the result types above are not QFuture objects, but real result
    types (in this case, QStringList and QSet<QString>).

    \section1 Concurrent Filter

    QtConcurrent::filtered() takes an input sequence and a filter function.
    This filter function is then called for each item in the sequence, and a
    new sequence containing the filtered values is returned.

    The filter function must be of the form:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 0

    T must match the type stored in the sequence. The function returns \c true if
    the item should be kept, false if it should be discarded.

    This example shows how to keep strings that are all lower-case from a
    QStringList:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 1

    The results of the filter are made available through QFuture. See the
    QFuture and QFutureWatcher documentation for more information on how to
    use QFuture in your applications.

    If you want to modify a sequence in-place, use QtConcurrent::filter():

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 2

    Since the sequence is modified in place, QtConcurrent::filter() does not
    return any results via QFuture. However, you can still use QFuture and
    QFutureWatcher to monitor the status of the filter.

    \section1 Concurrent Filter-Reduce

    QtConcurrent::filteredReduced() is similar to QtConcurrent::filtered(),
    but instead of returing a sequence with the filtered results, the results
    are combined into a single value using a reduce function.

    The reduce function must be of the form:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 3

    T is the type of the final result, U is the type of items being filtered.
    Note that the return value and return type of the reduce function are not
    used.

    Call QtConcurrent::filteredReduced() like this:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 4

    The reduce function will be called once for each result kept by the filter
    function, and should merge the \e{intermediate} into the \e{result}
    variable. QtConcurrent::filteredReduced() guarantees that only one thread
    will call reduce at a time, so using a mutex to lock the result variable
    is not necessary. The QtConcurrent::ReduceOptions enum provides a way to
    control the order in which the reduction is done.

    \section1 Additional API Features

    \section2 Using Iterators instead of Sequence

    Each of the above functions has a variant that takes an iterator range
    instead of a sequence. You use them in the same way as the sequence
    variants:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 5


    \section2 Using Member Functions

    QtConcurrent::filter(), QtConcurrent::filtered(), and
    QtConcurrent::filteredReduced() accept pointers to member functions.
    The member function class type must match the type stored in the sequence:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 7

    Note that when using QtConcurrent::filteredReduced(), you can mix the use of
    normal and member functions freely:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 8

    \section2 Using Function Objects

    QtConcurrent::filter(), QtConcurrent::filtered(), and
    QtConcurrent::filteredReduced() accept function objects, which can be used to
    add state to a function call. The result_type typedef must define the
    result type of the function call operator:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 13

    \section2 Using Bound Function Arguments

    If you want to use a filter function takes more than one argument, you can
    use std::bind() to transform it onto a function that takes one argument. If
    C++11 support is not available, \l{http://www.boost.org/libs/bind/bind.html}
    {boost::bind()} or \l{http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2005/n1836.pdf}
    {std::tr1::bind()} are suitable replacements.

    As an example, we use QString::contains():

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 9

    QString::contains() takes 2 arguments (including the "this" pointer) and
    can't be used with QtConcurrent::filtered() directly, because
    QtConcurrent::filtered() expects a function that takes one argument. To
    use QString::contains() with QtConcurrent::filtered() we have to provide a
    value for the \e regexp argument:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 10

    The return value from std::bind() is a function object (functor) with
    the following signature:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 11

    This matches what QtConcurrent::filtered() expects, and the complete
    example becomes:

    \snippet code/src_concurrent_qtconcurrentfilter.cpp 12
*/

/*!
    \fn QFuture<void> QtConcurrent::filter(Sequence &sequence, FilterFunction filterFunction)

    Calls \a filterFunction once for each item in \a sequence. If
    \a filterFunction returns \c true, the item is kept in \a sequence;
    otherwise, the item is removed from \a sequence.
*/

/*!
    \fn QFuture<T> QtConcurrent::filtered(const Sequence &sequence, FilterFunction filterFunction)

    Calls \a filterFunction once for each item in \a sequence and returns a
    new Sequence of kept items. If \a filterFunction returns \c true, a copy of
    the item is put in the new Sequence. Otherwise, the item will \e not
    appear in the new Sequence.
*/

/*!
    \fn QFuture<T> QtConcurrent::filtered(ConstIterator begin, ConstIterator end, FilterFunction filterFunction)

    Calls \a filterFunction once for each item from \a begin to \a end and
    returns a new Sequence of kept items. If \a filterFunction returns \c true, a
    copy of the item is put in the new Sequence. Otherwise, the item will
    \e not appear in the new Sequence.
*/

/*!
    \fn QFuture<T> QtConcurrent::filteredReduced(const Sequence &sequence, FilterFunction filterFunction, ReduceFunction reduceFunction, QtConcurrent::ReduceOptions reduceOptions)

    Calls \a filterFunction once for each item in \a sequence. If
    \a filterFunction returns \c true for an item, that item is then passed to
    \a reduceFunction. In other words, the return value is the result of
    \a reduceFunction for each item where \a filterFunction returns \c true.

    Note that while \a filterFunction is called concurrently, only one thread
    at a time will call \a reduceFunction. The order in which \a reduceFunction
    is called is undefined if \a reduceOptions is
    QtConcurrent::UnorderedReduce. If \a reduceOptions is
    QtConcurrent::OrderedReduce, \a reduceFunction is called in the order of
    the original sequence.
*/

/*!
    \fn QFuture<T> QtConcurrent::filteredReduced(ConstIterator begin, ConstIterator end, FilterFunction filterFunction, ReduceFunction reduceFunction, QtConcurrent::ReduceOptions reduceOptions)

    Calls \a filterFunction once for each item from \a begin to \a end. If
    \a filterFunction returns \c true for an item, that item is then passed to
    \a reduceFunction. In other words, the return value is the result of
    \a reduceFunction for each item where \a filterFunction returns \c true.

    Note that while \a filterFunction is called concurrently, only one thread
    at a time will call \a reduceFunction. The order in which
    \a reduceFunction is called is undefined if \a reduceOptions is
    QtConcurrent::UnorderedReduce. If \a reduceOptions is
    QtConcurrent::OrderedReduce, the \a reduceFunction is called in the order
    of the original sequence.
*/

/*!
  \fn void QtConcurrent::blockingFilter(Sequence &sequence, FilterFunction filterFunction)

  Calls \a filterFunction once for each item in \a sequence. If
  \a filterFunction returns \c true, the item is kept in \a sequence;
  otherwise, the item is removed from \a sequence.

  \note This function will block until all items in the sequence have been processed.
*/

/*!
  \fn Sequence QtConcurrent::blockingFiltered(const Sequence &sequence, FilterFunction filterFunction)

  Calls \a filterFunction once for each item in \a sequence and returns a
  new Sequence of kept items. If \a filterFunction returns \c true, a copy of
  the item is put in the new Sequence. Otherwise, the item will \e not
  appear in the new Sequence.

  \note This function will block until all items in the sequence have been processed.

  \sa filtered()
*/

/*!
  \fn Sequence QtConcurrent::blockingFiltered(ConstIterator begin, ConstIterator end, FilterFunction filterFunction)

  Calls \a filterFunction once for each item from \a begin to \a end and
  returns a new Sequence of kept items. If \a filterFunction returns \c true, a
  copy of the item is put in the new Sequence. Otherwise, the item will
  \e not appear in the new Sequence.

  \note This function will block until the iterator reaches the end of the
  sequence being processed.

  \sa filtered()
*/

/*!
  \fn T QtConcurrent::blockingFilteredReduced(const Sequence &sequence, FilterFunction filterFunction, ReduceFunction reduceFunction, QtConcurrent::ReduceOptions reduceOptions)

  Calls \a filterFunction once for each item in \a sequence. If
  \a filterFunction returns \c true for an item, that item is then passed to
  \a reduceFunction. In other words, the return value is the result of
  \a reduceFunction for each item where \a filterFunction returns \c true.

  Note that while \a filterFunction is called concurrently, only one thread
  at a time will call \a reduceFunction. The order in which \a reduceFunction
  is called is undefined if \a reduceOptions is
  QtConcurrent::UnorderedReduce. If \a reduceOptions is
  QtConcurrent::OrderedReduce, \a reduceFunction is called in the order of
  the original sequence.

  \note This function will block until all items in the sequence have been processed.

  \sa filteredReduced()
*/

/*!
  \fn T QtConcurrent::blockingFilteredReduced(ConstIterator begin, ConstIterator end, FilterFunction filterFunction, ReduceFunction reduceFunction, QtConcurrent::ReduceOptions reduceOptions)

  Calls \a filterFunction once for each item from \a begin to \a end. If
  \a filterFunction returns \c true for an item, that item is then passed to
  \a reduceFunction. In other words, the return value is the result of
  \a reduceFunction for each item where \a filterFunction returns \c true.

  Note that while \a filterFunction is called concurrently, only one thread
  at a time will call \a reduceFunction. The order in which
  \a reduceFunction is called is undefined if \a reduceOptions is
  QtConcurrent::UnorderedReduce. If \a reduceOptions is
  QtConcurrent::OrderedReduce, the \a reduceFunction is called in the order
  of the original sequence.

  \note This function will block until the iterator reaches the end of the
  sequence being processed.

  \sa filteredReduced()
*/
