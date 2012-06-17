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

#ifndef QTCONCURRENT_EXCEPTION_H
#define QTCONCURRENT_EXCEPTION_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_QFUTURE

#include <QtCore/qatomic.h>

#ifndef QT_NO_EXCEPTIONS
#  include <exception>
#endif

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

QT_MODULE(Core)

namespace QtConcurrent
{

#ifndef QT_NO_EXCEPTIONS

class Q_CORE_EXPORT Exception : public std::exception
{
public:
    virtual void raise() const;
    virtual Exception *clone() const;
};

class Q_CORE_EXPORT UnhandledException : public Exception
{
public:
    void raise() const;
    Exception *clone() const;
};

namespace internal {

class Base;
class ExceptionHolder
{
public:
    ExceptionHolder(Exception *exception = 0);
    ExceptionHolder(const ExceptionHolder &other);
    void operator=(const ExceptionHolder &other);
    ~ExceptionHolder();
    Exception *exception() const;
    Base *base;
};

class Q_CORE_EXPORT ExceptionStore
{
public:
    void setException(const Exception &e);
    bool hasException() const;
    ExceptionHolder exception();
    void throwPossibleException();
    bool hasThrown() const;
    ExceptionHolder exceptionHolder;
};

} // namespace internal

#else // QT_NO_EXCEPTIONS

namespace internal {

class Q_CORE_EXPORT ExceptionStore
{
public:
    ExceptionStore() { }
    inline void throwPossibleException() const {}
};

} // namespace internal

#endif

} // namespace QtConcurrent

QT_END_NAMESPACE
QT_END_HEADER

#endif // QT_NO_CONCURRENT

#endif
