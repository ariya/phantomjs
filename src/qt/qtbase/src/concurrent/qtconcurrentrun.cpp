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
    \page qtconcurrentrun.html
    \title Concurrent Run
    \ingroup thread

    The QtConcurrent::run() function runs a function in a separate thread.
    The return value of the function is made available through the QFuture API.

    This function is a part of the \l {Qt Concurrent} framework.

    \section1 Running a Function in a Separate Thread

    To run a function in another thread, use QtConcurrent::run():

    \snippet code/src_concurrent_qtconcurrentrun.cpp 0

    This will run \e aFunction in a separate thread obtained from the default
    QThreadPool. You can use the QFuture and QFutureWatcher classes to monitor
    the status of the function.

    \section1 Passing Arguments to the Function

    Passing arguments to the function is done by adding them to the
    QtConcurrent::run() call immediately after the function name. For example:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 1

    A copy of each argument is made at the point where QtConcurrent::run() is
    called, and these values are passed to the thread when it begins executing
    the function. Changes made to the arguments after calling
    QtConcurrent::run() are \e not visible to the thread.

    \section1 Returning Values from the Function

    Any return value from the function is available via QFuture:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 2

    As documented above, passing arguments is done like this:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 3

    Note that the QFuture::result() function blocks and waits for the result
    to become available. Use QFutureWatcher to get notification when the
    function has finished execution and the result is available.

    \section1 Additional API Features

    \section2 Using Member Functions

    QtConcurrent::run() also accepts pointers to member functions. The first
    argument must be either a const reference or a pointer to an instance of
    the class. Passing by const reference is useful when calling const member
    functions; passing by pointer is useful for calling non-const member
    functions that modify the instance.

    For example, calling QByteArray::split() (a const member function) in a
    separate thread is done like this:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 4

    Calling a non-const member function is done like this:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 5

    \section2 Using Bound Function Arguments

    You can use std::bind() to \e bind a number of arguments to a function when
    called. If C++11 support is not available, \l{http://www.boost.org/libs/bind/bind.html}
    {boost::bind()} or \l{http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2005/n1836.pdf}
    {std::tr1::bind()} are suitable replacements.

    There are number of reasons for binding:

    \list
    \li To call a function that takes more than 5 arguments.
    \li To simplify calling a function with constant arguments.
    \li Changing the order of arguments.
    \endlist

    See the documentation for the relevant functions for details on how to use
    the bind API.

    Calling a bound function is done like this:

    \snippet code/src_concurrent_qtconcurrentrun.cpp 6
*/

/*!
    \fn QFuture<T> QtConcurrent::run(Function function, ...);

    Runs \a function in a separate thread. The thread is taken from the global
    QThreadPool. Note that the function may not run immediately; the function
    will only be run when a thread is available.

    T is the same type as the return value of \a function. Non-void return
    values can be accessed via the QFuture::result() function.

    Note that the QFuture returned by QtConcurrent::run() does not support
    canceling, pausing, or progress reporting. The QFuture returned can only
    be used to query for the running/finished status and the return value of
    the function.
*/
