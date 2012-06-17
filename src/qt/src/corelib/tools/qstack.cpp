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

/*!
    \class QStack
    \brief The QStack class is a template class that provides a stack.

    \ingroup tools
    \ingroup shared

    \reentrant

    QStack\<T\> is one of Qt's generic \l{container classes}. It implements
    a stack data structure for items of a same type.

    A stack is a last in, first out (LIFO) structure. Items are added
    to the top of the stack using push() and retrieved from the top
    using pop(). The top() function provides access to the topmost
    item without removing it.

    Example:

    \snippet doc/src/snippets/qstack/main.cpp 0

    The example will output 3, 2, 1 in that order.

    QStack inherits from QVector. All of QVector's functionality also
    applies to QStack. For example, you can use isEmpty() to test
    whether the stack is empty, and you can traverse a QStack using
    QVector's iterator classes (for example, QVectorIterator). But in
    addition, QStack provides three convenience functions that make
    it easy to implement LIFO semantics: push(), pop(), and top().

    QStack's value type must be an \l{assignable data type}. This
    covers most data types that are commonly used, but the compiler
    won't let you, for example, store a QWidget as a value; instead,
    store a QWidget *.

    \sa QVector, QQueue
*/

/*!
    \fn QStack::QStack()

    Constructs an empty stack.
*/

/*!
    \fn QStack::~QStack()

    Destroys the stack. References to the values in the stack, and all
    iterators over this stack, become invalid.
*/

/*!
    \fn void QStack::swap(QStack<T> &other)
    \since 4.8

    Swaps stack \a other with this stack. This operation is very fast and
    never fails.
*/

/*!
    \fn void QStack::push(const T& t)

    Adds element \a t to the top of the stack.

    This is the same as QVector::append().

    \sa pop(), top()
*/

/*!
    \fn T& QStack::top()

    Returns a reference to the stack's top item. This function
    assumes that the stack isn't empty.

    This is the same as QVector::last().

    \sa pop(), push(), isEmpty()
*/

/*!
    \fn const T& QStack::top() const

    \overload

    \sa pop(), push()
*/

/*!
    \fn T QStack::pop()

    Removes the top item from the stack and returns it. This function
    assumes that the stack isn't empty.

    \sa top(), push(), isEmpty()
*/
