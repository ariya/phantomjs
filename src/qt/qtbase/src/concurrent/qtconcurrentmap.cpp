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
    \namespace QtConcurrent
    \inmodule QtConcurrent
    \since 4.4
    \brief The QtConcurrent namespace provides high-level APIs that make it
    possible to write multi-threaded programs without using low-level
    threading primitives.

    See the \l {Qt Concurrent} module documentation for an overview of available
    functions, or see below for detailed information on each function.

    \inheaderfile QtConcurrent
    \ingroup thread
*/

/*!
    \enum QtConcurrent::ReduceOption
    This enum specifies the order of which results from the map or filter
    function are passed to the reduce function.

    \value UnorderedReduce Reduction is done in an arbitrary order.
    \value OrderedReduce Reduction is done in the order of the
    original sequence.
    \value SequentialReduce Reduction is done sequentially: only one
    thread will enter the reduce function at a time. (Parallel reduction
    might be supported in a future version of Qt Concurrent.)
*/

/*!
    \page qtconcurrentmap.html
    \title Concurrent Map and Map-Reduce
    \ingroup thread

    The QtConcurrent::map(), QtConcurrent::mapped() and
    QtConcurrent::mappedReduced() functions run computations in parallel on
    the items in a sequence such as a QList or a QVector. QtConcurrent::map()
    modifies a sequence in-place, QtConcurrent::mapped() returns a new
    sequence containing the modified content, and QtConcurrent::mappedReduced()
    returns a single result.

    These functions are a part of the \l {Qt Concurrent} framework.

    Each of the above functions has a blocking variant that returns
    the final result instead of a QFuture. You use them in the same
    way as the asynchronous variants.

    \snippet code/src_concurrent_qtconcurrentmap.cpp 7

    Note that the result types above are not QFuture objects, but real result
    types (in this case, QList<QImage> and QImage).

    \section1 Concurrent Map

    QtConcurrent::mapped() takes an input sequence and a map function. This map
    function is then called for each item in the sequence, and a new sequence
    containing the return values from the map function is returned.

    The map function must be of the form:

    \snippet code/src_concurrent_qtconcurrentmap.cpp 0

    T and U can be any type (and they can even be the same type), but T must
    match the type stored in the sequence. The function returns the modified
    or \e mapped content.

    This example shows how to apply a scale function to all the items
    in a sequence:

    \snippet code/src_concurrent_qtconcurrentmap.cpp 1

    The results of the map are made available through QFuture.  See the
    QFuture and QFutureWatcher documentation for more information on how to
    use QFuture in your applications.

    If you want to modify a sequence in-place, use QtConcurrent::map(). The
    map function must then be of the form:

    \snippet code/src_concurrent_qtconcurrentmap.cpp 2

    Note that the return value and return type of the map function are not
    used.

    Using QtConcurrent::map() is similar to using QtConcurrent::mapped():

    \snippet code/src_concurrent_qtconcurrentmap.cpp 3

    Since the sequence is modified in place, QtConcurrent::map() does not
    return any results via QFuture. However, you can still use QFuture and
    QFutureWatcher to monitor the status of the map.

    \section1 Concurrent Map-Reduce

    QtConcurrent::mappedReduced() is similar to QtConcurrent::mapped(), but
    instead of returning a sequence with the new results, the results are
    combined into a single value using a reduce function.

    The reduce function must be of the form:

    \snippet code/src_concurrent_qtconcurrentmap.cpp 4

    T is the type of the final result, U is the return type of the map
    function. Note that the return value and return type of the reduce
    function are not used.

    Call QtConcurrent::mappedReduced() like this:

    \snippet code/src_concurrent_qtconcurrentmap.cpp 5

    The reduce function will be called once for each result returned by the map
    function, and should merge the \e{intermediate} into the \e{result}
    variable.  QtConcurrent::mappedReduced() guarantees that only one thread
    will call reduce at a time, so using a mutex to lock the result variable
    is not necessary. The QtConcurrent::ReduceOptions enum provides a way to
    control the order in which the reduction is done. If
    QtConcurrent::UnorderedReduce is used (the default), the order is
    undefined, while QtConcurrent::OrderedReduce ensures that the reduction
    is done in the order of the original sequence.

    \section1 Additional API Features

    \section2 Using Iterators instead of Sequence

    Each of the above functions has a variant that takes an iterator range
    instead of a sequence. You use them in the same way as the sequence
    variants:

    \snippet code/src_concurrent_qtconcurrentmap.cpp 6

    \section2 Blocking Variants

    Each of the above functions has a blocking variant that returns
    the final result instead of a QFuture. You use them in the same
    way as the asynchronous variants.

    \snippet code/src_concurrent_qtconcurrentmap.cpp 7

    Note that the result types above are not QFuture objects, but real result
    types (in this case, QList<QImage> and QImage).

    \section2 Using Member Functions

    QtConcurrent::map(), QtConcurrent::mapped(), and
    QtConcurrent::mappedReduced() accept pointers to member functions.
    The member function class type must match the type stored in the sequence:

    \snippet code/src_concurrent_qtconcurrentmap.cpp 8

    Note that when using QtConcurrent::mappedReduced(), you can mix the use of
    normal and member functions freely:

    \snippet code/src_concurrent_qtconcurrentmap.cpp 9

    \section2 Using Function Objects

    QtConcurrent::map(), QtConcurrent::mapped(), and
    QtConcurrent::mappedReduced() accept function objects, which can be used to
    add state to a function call. The result_type typedef must define the
    result type of the function call operator:

    \snippet code/src_concurrent_qtconcurrentmap.cpp 14

    \section2 Using Bound Function Arguments

    If you want to use a map function that takes more than one argument you can
    use std::bind() to transform it onto a function that takes one argument. If
    C++11 support is not available, \l{http://www.boost.org/libs/bind/bind.html}
    {boost::bind()} or \l{http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2005/n1836.pdf}
    {std::tr1::bind()} are suitable replacements.

    As an example, we'll use QImage::scaledToWidth():

    \snippet code/src_concurrent_qtconcurrentmap.cpp 10

    scaledToWidth takes three arguments (including the "this" pointer) and
    can't be used with QtConcurrent::mapped() directly, because
    QtConcurrent::mapped() expects a function that takes one argument. To use
    QImage::scaledToWidth() with QtConcurrent::mapped() we have to provide a
    value for the \e{width} and the \e{transformation mode}:

    \snippet code/src_concurrent_qtconcurrentmap.cpp 11

    The return value from std::bind() is a function object (functor) with
    the following signature:

    \snippet code/src_concurrent_qtconcurrentmap.cpp 12

    This matches what QtConcurrent::mapped() expects, and the complete example
    becomes:

    \snippet code/src_concurrent_qtconcurrentmap.cpp 13
*/

/*!
    \fn QFuture<void> QtConcurrent::map(Sequence &sequence, MapFunction function)

    Calls \a function once for each item in \a sequence. The \a function is
    passed a reference to the item, so that any modifications done to the item
    will appear in \a sequence.
*/

/*!
    \fn QFuture<void> QtConcurrent::map(Iterator begin, Iterator end, MapFunction function)

    Calls \a function once for each item from \a begin to \a end. The
    \a function is passed a reference to the item, so that any modifications
    done to the item will appear in the sequence which the iterators belong to.
*/

/*!
    \fn QFuture<T> QtConcurrent::mapped(const Sequence &sequence, MapFunction function)

    Calls \a function once for each item in \a sequence and returns a future
    with each mapped item as a result. You can use QFuture::const_iterator or
    QFutureIterator to iterate through the results.
*/

/*!
    \fn QFuture<T> QtConcurrent::mapped(ConstIterator begin, ConstIterator end, MapFunction function)

    Calls \a function once for each item from \a begin to \a end and returns a
    future with each mapped item as a result. You can use
    QFuture::const_iterator or QFutureIterator to iterate through the results.
*/

/*!
    \fn QFuture<T> QtConcurrent::mappedReduced(const Sequence &sequence,
    MapFunction mapFunction, ReduceFunction reduceFunction,
    QtConcurrent::ReduceOptions reduceOptions)

    Calls \a mapFunction once for each item in \a sequence. The return value of
    each \a mapFunction is passed to \a reduceFunction.

    Note that while \a mapFunction is called concurrently, only one thread at a
    time will call \a reduceFunction. The order in which \a reduceFunction is
    called is determined by \a reduceOptions.
*/

/*!
    \fn QFuture<T> QtConcurrent::mappedReduced(ConstIterator begin,
    ConstIterator end, MapFunction mapFunction, ReduceFunction reduceFunction,
    QtConcurrent::ReduceOptions reduceOptions)

    Calls \a mapFunction once for each item from \a begin to \a end. The return
    value of each \a mapFunction is passed to \a reduceFunction.

    Note that while \a mapFunction is called concurrently, only one thread at a
    time will call \a reduceFunction. By default, the order in which
    \a reduceFunction is called is undefined.

    \note QtConcurrent::OrderedReduce results in the ordered reduction.
*/

/*!
  \fn void QtConcurrent::blockingMap(Sequence &sequence, MapFunction function)

  Calls \a function once for each item in \a sequence. The \a function is
  passed a reference to the item, so that any modifications done to the item
  will appear in \a sequence.

  \note This function will block until all items in the sequence have been processed.

  \sa map()
*/

/*!
  \fn void QtConcurrent::blockingMap(Iterator begin, Iterator end, MapFunction function)

  Calls \a function once for each item from \a begin to \a end. The
  \a function is passed a reference to the item, so that any modifications
  done to the item will appear in the sequence which the iterators belong to.

  \note This function will block until the iterator reaches the end of the
  sequence being processed.

  \sa map()
*/

/*!
  \fn T QtConcurrent::blockingMapped(const Sequence &sequence, MapFunction function)

  Calls \a function once for each item in \a sequence and returns a Sequence containing
  the results. The type of the results will match the type returned my the MapFunction.

  \note This function will block until all items in the sequence have been processed.

  \sa mapped()
*/

/*!
  \fn T QtConcurrent::blockingMapped(ConstIterator begin, ConstIterator end, MapFunction function)

  Calls \a function once for each item from \a begin to \a end and returns a
  container with the results. Specify the type of container as the a template
  argument, like this:

  \code
     QList<int> ints = QtConcurrent::blockingMapped<QList<int> >(beginIterator, endIterator, fn);
  \endcode

  \note This function will block until the iterator reaches the end of the
  sequence being processed.

  \sa mapped()
*/

/*!
  \fn T QtConcurrent::blockingMappedReduced(const Sequence &sequence, MapFunction mapFunction, ReduceFunction reduceFunction, QtConcurrent::ReduceOptions reduceOptions)

  Calls \a mapFunction once for each item in \a sequence. The return value of
  each \a mapFunction is passed to \a reduceFunction.

  Note that while \a mapFunction is called concurrently, only one thread at a
  time will call \a reduceFunction. The order in which \a reduceFunction is
  called is determined by \a reduceOptions.

  \note This function will block until all items in the sequence have been processed.

  \sa mapped()
*/

/*!
  \fn T QtConcurrent::blockingMappedReduced(ConstIterator begin, ConstIterator end, MapFunction mapFunction, ReduceFunction reduceFunction, QtConcurrent::ReduceOptions reduceOptions)

  Calls \a mapFunction once for each item from \a begin to \a end. The return
  value of each \a mapFunction is passed to \a reduceFunction.

  Note that while \a mapFunction is called concurrently, only one thread at a
  time will call \a reduceFunction. The order in which \a reduceFunction is
  called is undefined.

  \note This function will block until the iterator reaches the end of the
  sequence being processed.

  \sa blockingMappedReduced()
*/
