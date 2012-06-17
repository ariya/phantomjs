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

#ifdef QT_NO_DEBUG
#undef QT_NO_DEBUG
#endif
#ifdef qDebug
#undef qDebug
#endif

#include "qdebug.h"

// This file is needed to force compilation of QDebug into the kernel library.

/*!
    \class QDebug

    \brief The QDebug class provides an output stream for debugging information.

    QDebug is used whenever the developer needs to write out debugging or tracing
    information to a device, file, string or console.

    \section1 Basic Use

    In the common case, it is useful to call the qDebug() function to obtain a
    default QDebug object to use for writing debugging information.

    \snippet doc/src/snippets/qdebug/qdebugsnippet.cpp 1

    This constructs a QDebug object using the constructor that accepts a QtMsgType
    value of QtDebugMsg. Similarly, the qWarning(), qCritical() and qFatal()
    functions also return QDebug objects for the corresponding message types.

    The class also provides several constructors for other situations, including
    a constructor that accepts a QFile or any other QIODevice subclass that is
    used to write debugging information to files and other devices. The constructor
    that accepts a QString is used to write to a string for display or serialization.

    \section1 Writing Custom Types to a Stream

    Many standard types can be written to QDebug objects, and Qt provides support for
    most Qt value types. To add support for custom types, you need to implement a
    streaming operator, as in the following example:

    \snippet doc/src/snippets/qdebug/qdebugsnippet.cpp 0

    This is described in the \l{Debugging Techniques} and
    \l{Creating Custom Qt Types#Making the Type Printable}{Creating Custom Qt Types}
    documents.
*/

/*!
    \fn QDebug::QDebug(QIODevice *device)

    Constructs a debug stream that writes to the given \a device.
*/

/*!
    \fn QDebug::QDebug(QString *string)

    Constructs a debug stream that writes to the given \a string.
*/

/*!
    \fn QDebug::QDebug(QtMsgType type)

    Constructs a debug stream that writes to the handler for the message type specified by \a type.
*/

/*!
    \fn QDebug::QDebug(const QDebug &other)

    Constructs a copy of the \a other debug stream.
*/

/*!
    \fn QDebug &QDebug::operator=(const QDebug &other)

    Assigns the \a other debug stream to this stream and returns a reference to
    this stream.
*/

/*!
    \fn QDebug::~QDebug()

    Flushes any pending data to be written and destroys the debug stream.
*/

/*!
    \fn QDebug &QDebug::space()

    Writes a space character to the debug stream and returns a reference to
    the stream.

    The stream will record that the last character sent to the stream was a
    space.

    \sa nospace(), maybeSpace()
*/

/*!
    \fn QDebug &QDebug::nospace()

    Clears the stream's internal flag that records whether the last character
    was a space and returns a reference to the stream.

    \sa space(), maybeSpace()
*/

/*!
    \fn QDebug &QDebug::maybeSpace()

    Writes a space character to the debug stream, depending on the last
    character sent to the stream, and returns a reference to the stream.

    If the last character was a space character, this function writes a space
    character to the stream; otherwise, no characters are written to the stream.

    \sa space(), nospace()
*/

/*!
    \fn QDebug &QDebug::operator<<(QChar t)

    Writes the character, \a t, to the stream and returns a reference to the
    stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(QBool t)
    \internal

    Writes the boolean value, \a t, to the stream and returns a reference to the
    stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(bool t)

    Writes the boolean value, \a t, to the stream and returns a reference to the
    stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(char t)

    Writes the character, \a t, to the stream and returns a reference to the
    stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(signed short i)

    Writes the signed short integer, \a i, to the stream and returns a reference
    to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(unsigned short i)

    Writes then unsigned short integer, \a i, to the stream and returns a
    reference to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(signed int i)

    Writes the signed integer, \a i, to the stream and returns a reference
    to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(unsigned int i)

    Writes then unsigned integer, \a i, to the stream and returns a reference to
    the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(signed long l)

    Writes the signed long integer, \a l, to the stream and returns a reference
    to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(unsigned long l)

    Writes then unsigned long integer, \a l, to the stream and returns a reference
    to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(qint64 i)

    Writes the signed 64-bit integer, \a i, to the stream and returns a reference
    to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(quint64 i)

    Writes then unsigned 64-bit integer, \a i, to the stream and returns a
    reference to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(float f)

    Writes the 32-bit floating point number, \a f, to the stream and returns a
    reference to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(double f)

    Writes the 64-bit floating point number, \a f, to the stream and returns a
    reference to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(const char *s)

    Writes the '\0'-terminated string, \a s, to the stream and returns a
    reference to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(const QString &s)

    Writes the string, \a s, to the stream and returns a reference to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(const QStringRef &s)

    Writes the string reference, \a s, to the stream and returns a reference to
    the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(const QLatin1String &s)

    Writes the Latin1-encoded string, \a s, to the stream and returns a reference
    to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(const QByteArray &b)

    Writes the byte array, \a b, to the stream and returns a reference to the
    stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(const void *p)

    Writes a pointer, \a p, to the stream and returns a reference to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(QTextStreamFunction f)
    \internal
*/

/*!
    \fn QDebug &QDebug::operator<<(QTextStreamManipulator m)
    \internal
*/
