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

#include "qexception.h"
#include "QtCore/qshareddata.h"

#ifndef QT_NO_QFUTURE
#ifndef QT_NO_EXCEPTIONS

QT_BEGIN_NAMESPACE

/*!
    \class QException
    \inmodule QtCore
    \brief The QException class provides a base class for exceptions that can transferred across threads.
    \since 5.0

    Qt Concurrent supports throwing and catching exceptions across thread
    boundaries, provided that the exception inherit from QException
    and implement two helper functions:

    \snippet code/src_corelib_thread_qexception.cpp 0

    QException subclasses must be thrown by value and
    caught by reference:

    \snippet code/src_corelib_thread_qexception.cpp 1

    If you throw an exception that is not a subclass of QException,
    the Qt functions will throw a QUnhandledException
    in the receiver thread.

    When using QFuture, transferred exceptions will be thrown when calling the following functions:
    \list
    \li QFuture::waitForFinished()
    \li QFuture::result()
    \li QFuture::resultAt()
    \li QFuture::results()
    \endlist
*/

/*!
    \fn QException::raise() const
    In your QException subclass, reimplement raise() like this:

    \snippet code/src_corelib_thread_qexception.cpp 2
*/

/*!
    \fn QException::clone() const
    In your QException subclass, reimplement clone() like this:

    \snippet code/src_corelib_thread_qexception.cpp 3
*/

/*!
    \class QUnhandledException
    \inmodule QtCore

    \brief The UnhandledException class represents an unhandled exception in a worker thread.
    \since 5.0

    If a worker thread throws an exception that is not a subclass of QException,
    the Qt functions will throw a QUnhandledException
    on the receiver thread side.

    Inheriting from this class is not supported.
*/

/*!
    \fn QUnhandledException::raise() const
    \internal
*/

/*!
    \fn QUnhandledException::clone() const
    \internal
*/

void QException::raise() const
{
    QException e = *this;
    throw e;
}

QException *QException::clone() const
{
    return new QException(*this);
}

void QUnhandledException::raise() const
{
    QUnhandledException e = *this;
    throw e;
}

QUnhandledException *QUnhandledException::clone() const
{
    return new QUnhandledException(*this);
}

#ifndef Q_QDOC

namespace QtPrivate {

class Base : public QSharedData
{
public:
    Base(QException *exception)
    : exception(exception), hasThrown(false) { }
    ~Base() { delete exception; }

    QException *exception;
    bool hasThrown;
};

ExceptionHolder::ExceptionHolder(QException *exception)
: base(new Base(exception)) {}

ExceptionHolder::ExceptionHolder(const ExceptionHolder &other)
: base(other.base)
{}

void ExceptionHolder::operator=(const ExceptionHolder &other)
{
    base = other.base;
}

ExceptionHolder::~ExceptionHolder()
{}

QException *ExceptionHolder::exception() const
{
    return base->exception;
}

void ExceptionStore::setException(const QException &e)
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

} // namespace QtPrivate

#endif //Q_QDOC

QT_END_NAMESPACE

#endif // QT_NO_EXCEPTIONS
#endif // QT_NO_QFUTURE
