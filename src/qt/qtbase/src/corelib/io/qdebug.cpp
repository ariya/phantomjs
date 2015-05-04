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

#ifdef QT_NO_DEBUG
#undef QT_NO_DEBUG
#endif
#ifdef qDebug
#undef qDebug
#endif

#include "qdebug.h"
#include <private/qtextstream_p.h>

QT_BEGIN_NAMESPACE

// This file is needed to force compilation of QDebug into the kernel library.

/*!
    \class QDebug
    \inmodule QtCore
    \ingroup shared

    \brief The QDebug class provides an output stream for debugging information.

    QDebug is used whenever the developer needs to write out debugging or tracing
    information to a device, file, string or console.

    \section1 Basic Use

    In the common case, it is useful to call the qDebug() function to obtain a
    default QDebug object to use for writing debugging information.

    \snippet qdebug/qdebugsnippet.cpp 1

    This constructs a QDebug object using the constructor that accepts a QtMsgType
    value of QtDebugMsg. Similarly, the qWarning(), qCritical() and qFatal()
    functions also return QDebug objects for the corresponding message types.

    The class also provides several constructors for other situations, including
    a constructor that accepts a QFile or any other QIODevice subclass that is
    used to write debugging information to files and other devices. The constructor
    that accepts a QString is used to write to a string for display or serialization.

    \section1 Formatting Options

    QDebug formats output so that it's easily readable. It automatically adds spaces
    between arguments, and adds quotes around QString, QByteArray, QChar arguments.

    You can tweak these options through the space(), nospace() and quote(), noquote()
    methods. Furthermore, \l{QTextStream manipulators} can be piped into a QDebug
    stream.

    QDebugStateSaver limits changes to the formatting to the current scope.
    resetFormat() resets the options to the default ones.

    \section1 Writing Custom Types to a Stream

    Many standard types can be written to QDebug objects, and Qt provides support for
    most Qt value types. To add support for custom types, you need to implement a
    streaming operator, as in the following example:

    \snippet qdebug/qdebugsnippet.cpp 0

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
// Has been defined in the header / inlined before Qt 5.4
QDebug::~QDebug()
{
    if (!--stream->ref) {
        if (stream->space && stream->buffer.endsWith(QLatin1Char(' ')))
            stream->buffer.chop(1);
        if (stream->message_output) {
            qt_message_output(stream->type,
                              stream->context,
                              stream->buffer);
        }
        delete stream;
    }
}

/*!
    \fn QDebug::swap(QDebug &other)
    \since 5.0

    Swaps this debug stream instance with \a other. This function is
    very fast and never fails.
*/

/*!
    Resets the stream formatting options, bringing it back to its original constructed state.

    \sa space(), quote()
    \since 5.4
*/
QDebug &QDebug::resetFormat()
{
    stream->ts.reset();
    stream->space = true;
    if (stream->context.version > 1)
        stream->flags = 0;
    return *this;
}

/*!
    \fn QDebug &QDebug::space()

    Writes a space character to the debug stream and returns a reference to
    the stream.

    The stream remembers that automatic insertion of spaces is
    enabled for future writes.

    \sa nospace(), maybeSpace()
*/

/*!
    \fn QDebug &QDebug::nospace()

    Disables automatic insertion of spaces and returns a reference to the stream.

    \sa space(), maybeSpace()
*/

/*!
    \fn QDebug &QDebug::maybeSpace()

    Writes a space character to the debug stream, depending on the current
    setting for automatic insertion of spaces, and returns a reference to the stream.

    \sa space(), nospace()
*/

/*!
    \fn bool QDebug::autoInsertSpaces() const

    Returns \c true if this QDebug instance will automatically insert spaces
    between writes.

    \since 5.0

    \sa QDebugStateSaver
*/

/*!
    \fn void QDebug::setAutoInsertSpaces(bool b)

    Enables automatic insertion of spaces between writes if \a b is true; otherwise
    automatic insertion of spaces is disabled.

    \since 5.0

    \sa QDebugStateSaver
*/


/*!
    \fn QDebug &QDebug::quote()
    \since 5.4

    Enables automatic insertion of quotation characters around QChar, QString and QByteArray
    contents and returns a reference to the stream.

    Quoting is enabled by default.

    \sa noquote(), maybeQuote()
*/

/*!
    \fn QDebug &QDebug::noquote()
    \since 5.4

    Disables automatic insertion of quotation characters around QChar, QString and QByteArray
    contents and returns a reference to the stream.

    \sa quote(), maybeQuote()
*/

/*!
    \fn QDebug &QDebug::maybeQuote(char c)
    \since 5.4

    Writes a character \a c to the debug stream, depending on the
    current setting for automatic insertion of quotes, and returns a reference to the stream.

    The default character is a double quote \c{"}.

    \sa quote(), noquote()
*/

/*!
    \fn QDebug &QDebug::operator<<(QChar t)

    Writes the character, \a t, to the stream and returns a reference to the
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

    Writes the '\\0'-terminated string, \a s, to the stream and returns a
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
    \fn QDebug &QDebug::operator<<(QLatin1String s)

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

/*!
    \class QDebugStateSaver

    \brief Convenience class for custom QDebug operators

    Saves the settings used by QDebug, and restores them upon destruction,
    then calls \l {QDebug::maybeSpace()}{maybeSpace()}, to separate arguments with a space if
    \l {QDebug::autoInsertSpaces()}{autoInsertSpaces()} was true at the time of constructing the QDebugStateSaver.

    The automatic insertion of spaces between writes is one of the settings
    that QDebugStateSaver stores for the duration of the current block.

    The settings of the internal QTextStream are also saved and restored,
    so that using << hex in a QDebug operator doesn't affect other QDebug
    operators.

    \since 5.1
*/

class QDebugStateSaverPrivate
{
public:
    QDebugStateSaverPrivate(QDebug &dbg)
        : m_dbg(dbg),
          m_spaces(dbg.autoInsertSpaces()),
          m_flags(0),
          m_streamParams(dbg.stream->ts.d_ptr->params)
    {
        if (m_dbg.stream->context.version > 1)
            m_flags = m_dbg.stream->flags;
    }
    void restoreState()
    {
        const bool currentSpaces = m_dbg.autoInsertSpaces();
        if (currentSpaces && !m_spaces)
            if (m_dbg.stream->buffer.endsWith(QLatin1Char(' ')))
                m_dbg.stream->buffer.chop(1);

        m_dbg.setAutoInsertSpaces(m_spaces);
        m_dbg.stream->ts.d_ptr->params = m_streamParams;
        if (m_dbg.stream->context.version > 1)
            m_dbg.stream->flags = m_flags;

        if (!currentSpaces && m_spaces)
            m_dbg.stream->ts << ' ';
    }

    QDebug &m_dbg;

    // QDebug state
    const bool m_spaces;
    int m_flags;

    // QTextStream state
    const QTextStreamPrivate::Params m_streamParams;
};


/*!
    Creates a QDebugStateSaver instance, which saves the settings
    currently used by \a dbg.

    \sa QDebug::setAutoInsertSpaces(), QDebug::autoInsertSpaces()
*/
QDebugStateSaver::QDebugStateSaver(QDebug &dbg)
    : d(new QDebugStateSaverPrivate(dbg))
{
}

/*!
    Destroys a QDebugStateSaver instance, which restores the settings
    used when the QDebugStateSaver instance was created.

    \sa QDebug::setAutoInsertSpaces(), QDebug::autoInsertSpaces()
*/
QDebugStateSaver::~QDebugStateSaver()
{
    d->restoreState();
}

QT_END_NAMESPACE
