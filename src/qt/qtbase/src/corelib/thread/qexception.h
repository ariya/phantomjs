/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTCORE_QEXCEPTION_H
#define QTCORE_QEXCEPTION_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_QFUTURE

#include <QtCore/qatomic.h>
#include <QtCore/qshareddata.h>

#ifndef QT_NO_EXCEPTIONS
#  include <exception>
#endif

QT_BEGIN_NAMESPACE


#ifndef QT_NO_EXCEPTIONS

class Q_CORE_EXPORT QException : public std::exception
{
public:
    virtual void raise() const;
    virtual QException *clone() const;
};

class Q_CORE_EXPORT QUnhandledException : public QException
{
public:
    void raise() const;
    QUnhandledException *clone() const;
};

namespace QtPrivate {

class Base;
class Q_CORE_EXPORT ExceptionHolder
{
public:
    ExceptionHolder(QException *exception = 0);
    ExceptionHolder(const ExceptionHolder &other);
    void operator=(const ExceptionHolder &other);
    ~ExceptionHolder();
    QException *exception() const;
    QExplicitlySharedDataPointer<Base> base;
};

class Q_CORE_EXPORT ExceptionStore
{
public:
    void setException(const QException &e);
    bool hasException() const;
    ExceptionHolder exception();
    void throwPossibleException();
    bool hasThrown() const;
    ExceptionHolder exceptionHolder;
};

} // namespace QtPrivate

#else // QT_NO_EXCEPTIONS

namespace QtPrivate {

class Q_CORE_EXPORT ExceptionStore
{
public:
    ExceptionStore() { }
    inline void throwPossibleException() {}
};

} // namespace QtPrivate

#endif // QT_NO_EXCEPTIONS

QT_END_NAMESPACE

#endif // QT_NO_QFUTURE

#endif
