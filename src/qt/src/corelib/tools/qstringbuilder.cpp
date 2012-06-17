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

#include "qstringbuilder.h"
#include <QtCore/qtextcodec.h>

QT_BEGIN_NAMESPACE

/*!
    \class QLatin1Literal
    \internal
    \reentrant
    \since 4.6

    \brief The QLatin1Literal class provides a thin wrapper around string
    literals used in source code.

    \ingroup tools
    \ingroup shared
    \ingroup string-processing


    Unlike \c QLatin1String, a \c QLatin1Literal can retrieve its size
    without iterating over the literal.

    The main use of \c QLatin1Literal is in conjunction with \c QStringBuilder
    to reduce the number of reallocations needed to build up a string from
    smaller chunks.

    \sa QStringBuilder, QLatin1String, QString, QStringRef
*/

/*! \fn int QLatin1Literal::size() const
 
    Returns the number of characters in the literal \e{excluding} the trailing
    NULL char.
*/

/*! \fn QLatin1Literal::QLatin1Literal(const char str)
 
    Constructs a new literal from the string \a str.
*/

/*! \fn const char *QLatin1Literal::data() const
 
    Returns a pointer to the first character of the string literal.
    The string literal is terminated by a NUL character.
*/

/*!
    \class QStringBuilder
    \internal
    \reentrant
    \since 4.6

    \brief The QStringBuilder class is a template class that provides a facility to build up QStrings from smaller chunks.

    \ingroup tools
    \ingroup shared
    \ingroup string-processing


    To build a QString by multiple concatenations, QString::operator+()
    is typically used. This causes \e{n - 1} reallocations when building
    a string from \e{n} chunks.

    QStringBuilder uses expression templates to collect the individual
    chunks, compute the total size, allocate the required amount of
    memory for the final QString object, and copy the chunks into the
    allocated memory.

    The QStringBuilder class is not to be used explicitly in user
    code.  Instances of the class are created as return values of the
    operator%() function, acting on objects of type QString,
    QLatin1String, QLatin1Literal, QStringRef, QChar, QCharRef,
    QLatin1Char, and \c char.

    Concatenating strings with operator%() generally yields better
    performance then using \c QString::operator+() on the same chunks
    if there are three or more of them, and performs equally well in other
    cases.

    \sa QLatin1Literal, QString
*/

/*! \fn QStringBuilder::QStringBuilder(const A &a, const B &b)
  Constructs a QStringBuilder from \a a and \a b.
 */

/* \fn QStringBuilder::operator%(const A &a, const B &b)

    Returns a \c QStringBuilder object that is converted to a QString object
    when assigned to a variable of QString type or passed to a function that
    takes a QString parameter.

    This function is usable with arguments of type \c QString,
    \c QLatin1String, \c QLatin1Literal, \c QStringRef, 
    \c QChar, \c QCharRef, \c QLatin1Char, and \c char.
*/

/*! \fn QByteArray QStringBuilder::toLatin1() const
  Returns a Latin-1 representation of the string as a QByteArray.  The
  returned byte array is undefined if the string contains non-Latin1
  characters.
 */

/*!
    \fn operator QStringBuilder::QString() const
 
    Converts the \c QLatin1Literal into a \c QString object.
*/

/*! \internal
   Note: The len contains the ending \0
 */
void QAbstractConcatenable::convertFromAscii(const char *a, int len, QChar *&out)
{
#ifndef QT_NO_TEXTCODEC
    if (QString::codecForCStrings && len) {
        QString tmp = QString::fromAscii(a, len > 0 ? len - 1 : -1);
        memcpy(out, reinterpret_cast<const char *>(tmp.constData()), sizeof(QChar) * tmp.size());
        out += tmp.length();
        return;
    }
#endif
    if (len == -1) {
        if (!a)
            return;
        while (*a)
            *out++ = QLatin1Char(*a++);
    } else {
        for (int i = 0; i < len - 1; ++i)
            *out++ = QLatin1Char(a[i]);
    }
}

/*! \internal */
void QAbstractConcatenable::convertToAscii(const QChar* a, int len, char*& out) 
{
#ifndef QT_NO_TEXTCODEC
    if (QString::codecForCStrings) {
        QByteArray tmp = QString::codecForCStrings->fromUnicode(a, len);
        memcpy(out, tmp.constData(), tmp.size());
        out += tmp.length();
        return;
    }
#endif
    if (len == -1) {
        while (a->unicode())
            convertToLatin1(*a++, out);
    } else {
        for (int i = 0; i < len; ++i)
            convertToLatin1(a[i], out);
    }
}


QT_END_NAMESPACE
