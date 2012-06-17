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

#include "qtconcurrentexception.h"

#ifndef QT_NO_QFUTURE
#ifndef QT_NO_EXCEPTIONS

QT_BEGIN_NAMESPACE

/*! 
    \class QtConcurrent::Exception
    \brief The Exception class provides a base class for exceptions that can transferred across threads.
    \since 4.4

    Qt Concurrent supports throwing and catching exceptions across thread
    boundaries, provided that the exception inherit from QtConcurrent::Exception
    and implement two helper functions:

    \snippet doc/src/snippets/code/src_corelib_concurrent_qtconcurrentexception.cpp 0

    QtConcurrent::Exception subclasses must be thrown by value and
    caught by reference:

    \snippet doc/src/snippets/code/src_corelib_concurrent_qtconcurrentexception.cpp 1

    If you throw an exception that is not a subclass of QtConcurrent::Exception,
    the Qt Concurrent functions will throw a QtConcurrent::UnhandledException
    in the receiver thread.

    When using QFuture, transferred exceptions will be thrown when calling the following functions:
    \list
    \o QFuture::waitForFinished()
    \o QFuture::result()
    \o QFuture::resultAt()
    \o QFuture::results()
    \endlist
*/

/*!
    \fn QtConcurrent::Exception::raise() const 
    In your QtConcurrent::Exception subclass, reimplement raise() like this:
    
    \snippet doc/src/snippets/code/src_corelib_concurrent_qtconcurrentexception.cpp 2
*/

/*!
    \fn QtConcurrent::Exception::clone() const
    In your QtConcurrent::Exception subclass, reimplement clone() like this:
    
    \snippet doc/src/snippets/code/src_corelib_concurrent_qtconcurrentexception.cpp 3
*/

/*! 
    \class QtConcurrent::UnhandledException

    \brief The UnhandledException class represents an unhandled exception in a worker thread.
    \since 4.4

    If a worker thread throws an exception that is not a subclass of QtConcurrent::Exception,
    the Qt Concurrent functions will throw a QtConcurrent::UnhandledException
    on the receiver thread side.

    Inheriting from this class is not supported.
*/

/*!
    \fn QtConcurrent::UnhandledException::raise() const
    \internal
*/

/*!
    \fn QtConcurrent::UnhandledException::clone() const
    \internal
*/

namespace QtConcurrent
{

void Exception::raise() const
{
    Exception e = *this;
    throw e;
}

Exception *Exception::clone() const
{
    return new Exception(*this);
}

void UnhandledException::raise() const
{
    UnhandledException e = *this;
    throw e;
}

Exception *UnhandledException::clone() const
{
    return new UnhandledException(*this);
}

#ifndef qdoc

namespace internal {

class Base
{
public:
    Base(Exception *exception)
    : exception(exception), refCount(1), hasThrown(false) { }
    ~Base() { delete exception; }

    Exception *exception;
    QAtomicInt refCount;
    bool hasThrown;
};

ExceptionHolder::ExceptionHolder(Exception *exception)
: base(new Base(exception)) {}

ExceptionHolder::ExceptionHolder(const ExceptionHolder &other)
: base(other.base)
{
    base->refCount.ref();
}

void ExceptionHolder::operator=(const ExceptionHolder &other)
{
    if (base == other.base)
        return;

    if (base->refCount.deref() == false)
        delete base;

    base = other.base;
    base->refCount.ref();
}

ExceptionHolder::~ExceptionHolder()
{
    if (base->refCount.deref() == 0)
        delete base;
}

Exception *ExceptionHolder::exception() const
{
    return base->exception;
}

void ExceptionStore::setException(const Exception &e)
{
    if (hasException() == false)
        exceptionHolder = ExceptionHolder(e.clone());
}

bool ExceptionStore::hasException() const
{
    return (exceptionHolder.exception() != 0);
}

ExceptionHolder ExceptionStore::exception()
{
    return exceptionHolder;
}

void ExceptionStore::throwPossibleException()
{
    if (hasException() ) {
        exceptionHolder.base->hasThrown = true;
        exceptionHolder.exception()->raise();
    }
}

bool ExceptionStore::hasThrown() const { return exceptionHolder.base->hasThrown; }

} // namespace internal

#endif //qdoc

} // namespace QtConcurrent

QT_END_NAMESPACE

#endif // QT_NO_EXCEPTIONS
#endif // QT_NO_CONCURRENT
