/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

//#define QTEXTSTREAM_DEBUG
static const int QTEXTSTREAM_BUFFERSIZE = 16384;

/*!
    \class QTextStream

    \brief The QTextStream class provides a convenient interface for
    reading and writing text.

    \ingroup io
    \ingroup string-processing
    \reentrant

    QTextStream can operate on a QIODevice, a QByteArray or a
    QString. Using QTextStream's streaming operators, you can
    conveniently read and write words, lines and numbers. For
    generating text, QTextStream supports formatting options for field
    padding and alignment, and formatting of numbers. Example:

    \snippet doc/src/snippets/code/src_corelib_io_qtextstream.cpp 0

    It's also common to use QTextStream to read console input and write
    console output. QTextStream is locale aware, and will automatically decode
    standard input using the correct codec. Example:

    \snippet doc/src/snippets/code/src_corelib_io_qtextstream.cpp 1

    Besides using QTextStream's constructors, you can also set the
    device or string QTextStream operates on by calling setDevice() or
    setString(). You can seek to a position by calling seek(), and
    atEnd() will return true when there is no data left to be read. If
    you call flush(), QTextStream will empty all data from its write
    buffer into the device and call flush() on the device.

    Internally, QTextStream uses a Unicode based buffer, and
    QTextCodec is used by QTextStream to automatically support
    different character sets. By default, QTextCodec::codecForLocale()
    is used for reading and writing, but you can also set the codec by
    calling setCodec(). Automatic Unicode detection is also
    supported. When this feature is enabled (the default behavior),
    QTextStream will detect the UTF-16 or the UTF-32 BOM (Byte Order Mark) and
    switch to the appropriate UTF codec when reading. QTextStream
    does not write a BOM by default, but you can enable this by calling
    setGenerateByteOrderMark(true). When QTextStream operates on a QString
    directly, the codec is disabled.

    There are three general ways to use QTextStream when reading text
    files:

    \list

    \o Chunk by chunk, by calling readLine() or readAll().

    \o Word by word. QTextStream supports streaming into QStrings,
    QByteArrays and char* buffers. Words are delimited by space, and
    leading white space is automatically skipped.

    \o Character by character, by streaming into QChar or char types.
    This method is often used for convenient input handling when
    parsing files, independent of character encoding and end-of-line
    semantics. To skip white space, call skipWhiteSpace().

    \endlist

    Since the text stream uses a buffer, you should not read from
    the stream using the implementation of a superclass. For instance,
    if you have a QFile and read from it directly using
    QFile::readLine() instead of using the stream, the text stream's
    internal position will be out of sync with the file's position.

    By default, when reading numbers from a stream of text,
    QTextStream will automatically detect the number's base
    representation. For example, if the number starts with "0x", it is
    assumed to be in hexadecimal form. If it starts with the digits
    1-9, it is assumed to be in decimal form, and so on. You can set
    the integer base, thereby disabling the automatic detection, by
    calling setIntegerBase(). Example:

    \snippet doc/src/snippets/code/src_corelib_io_qtextstream.cpp 2

    QTextStream supports many formatting options for generating text.
    You can set the field width and pad character by calling
    setFieldWidth() and setPadChar(). Use setFieldAlignment() to set
    the alignment within each field. For real numbers, call
    setRealNumberNotation() and setRealNumberPrecision() to set the
    notation (SmartNotation, ScientificNotation, FixedNotation) and precision in
    digits of the generated number. Some extra number formatting
    options are also available through setNumberFlags().

    \keyword QTextStream manipulators

    Like \c <iostream> in the standard C++ library, QTextStream also
    defines several global manipulator functions:

    \table
    \header \o Manipulator        \o Description
    \row    \o \c bin             \o Same as setIntegerBase(2).
    \row    \o \c oct             \o Same as setIntegerBase(8).
    \row    \o \c dec             \o Same as setIntegerBase(10).
    \row    \o \c hex             \o Same as setIntegerBase(16).
    \row    \o \c showbase        \o Same as setNumberFlags(numberFlags() | ShowBase).
    \row    \o \c forcesign       \o Same as setNumberFlags(numberFlags() | ForceSign).
    \row    \o \c forcepoint      \o Same as setNumberFlags(numberFlags() | ForcePoint).
    \row    \o \c noshowbase      \o Same as setNumberFlags(numberFlags() & ~ShowBase).
    \row    \o \c noforcesign     \o Same as setNumberFlags(numberFlags() & ~ForceSign).
    \row    \o \c noforcepoint    \o Same as setNumberFlags(numberFlags() & ~ForcePoint).
    \row    \o \c uppercasebase   \o Same as setNumberFlags(numberFlags() | UppercaseBase).
    \row    \o \c uppercasedigits \o Same as setNumberFlags(numberFlags() | UppercaseDigits).
    \row    \o \c lowercasebase   \o Same as setNumberFlags(numberFlags() & ~UppercaseBase).
    \row    \o \c lowercasedigits \o Same as setNumberFlags(numberFlags() & ~UppercaseDigits).
    \row    \o \c fixed           \o Same as setRealNumberNotation(FixedNotation).
    \row    \o \c scientific      \o Same as setRealNumberNotation(ScientificNotation).
    \row    \o \c left            \o Same as setFieldAlignment(AlignLeft).
    \row    \o \c right           \o Same as setFieldAlignment(AlignRight).
    \row    \o \c center          \o Same as setFieldAlignment(AlignCenter).
    \row    \o \c endl            \o Same as operator<<('\n') and flush().
    \row    \o \c flush           \o Same as flush().
    \row    \o \c reset           \o Same as reset().
    \row    \o \c ws              \o Same as skipWhiteSpace().
    \row    \o \c bom             \o Same as setGenerateByteOrderMark(true).
    \endtable

    In addition, Qt provides three global manipulators that take a
    parameter: qSetFieldWidth(), qSetPadChar(), and
    qSetRealNumberPrecision().

    \sa QDataStream, QIODevice, QFile, QBuffer, QTcpSocket, {Codecs Example}
*/

/*! \enum QTextStream::RealNumberNotation

    This enum specifies which notations to use for expressing \c
    float and \c double as strings.

    \value ScientificNotation Scientific notation (\c{printf()}'s \c %e flag).
    \value FixedNotation Fixed-point notation (\c{printf()}'s \c %f flag).
    \value SmartNotation Scientific or fixed-point notation, depending on which makes most sense (\c{printf()}'s \c %g flag).

    \sa setRealNumberNotation()
*/

/*! \enum QTextStream::FieldAlignment

    This enum specifies how to align text in fields when the field is
    wider than the text that occupies it.

    \value AlignLeft  Pad on the right side of fields.
    \value AlignRight  Pad on the left side of fields.
    \value AlignCenter  Pad on both sides of field.
    \value AlignAccountingStyle  Same as AlignRight, except that the
                                 sign of a number is flush left.

    \sa setFieldAlignment()
*/

/*! \enum QTextStream::NumberFlag

    This enum specifies various flags that can be set to affect the
    output of integers, \c{float}s, and \c{double}s.

    \value ShowBase  Show the base as a prefix if the base
                     is 16 ("0x"), 8 ("0"), or 2 ("0b").
    \value ForcePoint  Always put the decimal separator in numbers, even if
                       there are no decimals.
    \value ForceSign  Always put the sign in numbers, even for positive numbers.
    \value UppercaseBase  Use uppercase versions of base prefixes ("0X", "0B").
    \value UppercaseDigits  Use uppercase letters for expressing
                            digits 10 to 35 instead of lowercase.

    \sa setNumberFlags()
*/

/*! \enum QTextStream::Status

    This enum describes the current status of the text stream.

    \value Ok               The text stream is operating normally.
    \value ReadPastEnd      The text stream has read past the end of the
                            data in the underlying device.
    \value ReadCorruptData  The text stream has read corrupt data.
    \value WriteFailed      The text stream cannot write to the underlying device.

    \sa status()
*/

#include "qtextstream.h"
#include "qbuffer.h"
#include "qfile.h"
#include "qnumeric.h"
#ifndef QT_NO_TEXTCODEC
#include "qtextcodec.h"
#endif
#ifndef Q_OS_WINCE
#include <locale.h>
#endif
#include "private/qlocale_p.h"

#include <stdlib.h>
#include <limits.h>
#include <new>

#if defined QTEXTSTREAM_DEBUG
#include <ctype.h>

QT_BEGIN_NAMESPACE

// Returns a human readable representation of the first \a len
// characters in \a data.
static QByteArray qt_prettyDebug(const char *data, int len, int maxSize)
{
    if (!data) return "(null)";
    QByteArray out;
    for (int i = 0; i < len; ++i) {
        char c = data[i];
        if (isprint(int(uchar(c)))) {
            out += c;
        } else switch (c) {
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            QString tmp;
            tmp.sprintf("\\x%x", (unsigned int)(unsigned char)c);
            out += tmp.toLatin1();
        }
    }

    if (len < maxSize)
        out += "...";

    return out;
}
QT_END_NAMESPACE

#endif

// A precondition macro
#define Q_VOID
#define CHECK_VALID_STREAM(x) do { \
    if (!d->string && !d->device) { \
        qWarning("QTextStream: No device"); \
        return x; \
    } } while (0)

// Base implementations of operator>> for ints and reals
#define IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(type) do { \
    Q_D(QTextStream); \
    CHECK_VALID_STREAM(*this); \
    qulonglong tmp; \
    switch (d->getNumber(&tmp)) { \
    case QTextStreamPrivate::npsOk: \
        i = (type)tmp; \
        break; \
    case QTextStreamPrivate::npsMissingDigit: \
    case QTextStreamPrivate::npsInvalidPrefix: \
        i = (type)0; \
        setStatus(atEnd() ? QTextStream::ReadPastEnd : QTextStream::ReadCorruptData); \
        break; \
    } \
    return *this; } while (0)

#define IMPLEMENT_STREAM_RIGHT_REAL_OPERATOR(type) do { \
    Q_D(QTextStream); \
    CHECK_VALID_STREAM(*this); \
    double tmp; \
    if (d->getReal(&tmp)) { \
        f = (type)tmp; \
    } else { \
        f = (type)0; \
        setStatus(atEnd() ? QTextStream::ReadPastEnd : QTextStream::ReadCorruptData); \
    } \
    return *this; } while (0)

QT_BEGIN_NAMESPACE

#ifndef QT_NO_QOBJECT
class QDeviceClosedNotifier : public QObject
{
    Q_OBJECT
public:
    inline QDeviceClosedNotifier()
    { }

    inline void setupDevice(QTextStream *stream, QIODevice *device)
    {
        disconnect();
        if (device)
            connect(device, SIGNAL(aboutToClose()), this, SLOT(flushStream()));
        this->stream = stream;
    }

public Q_SLOTS:
    inline void flushStream() { stream->flush(); }

private:
    QTextStream *stream;
};
#endif

//-------------------------------------------------------------------
class QTextStreamPrivate
{
    Q_DECLARE_PUBLIC(QTextStream)
public:
    QTextStreamPrivate(QTextStream *q_ptr);
    ~QTextStreamPrivate();
    void reset();

    // device
    QIODevice *device;
#ifndef QT_NO_QOBJECT
    QDeviceClosedNotifier deviceClosedNotifier;
#endif
    bool deleteDevice;

    // string
    QString *string;
    int stringOffset;
    QIODevice::OpenMode stringOpenMode;

#ifndef QT_NO_TEXTCODEC
    // codec
    QTextCodec *codec;
    QTextCodec::ConverterState readConverterState;
    QTextCodec::ConverterState writeConverterState;
    QTextCodec::ConverterState *readConverterSavedState;
    bool autoDetectUnicode;
#endif

    // i/o
    enum TokenDelimiter {
        Space,
        NotSpace,
        EndOfLine
    };

    QString read(int maxlen);
    bool scan(const QChar **ptr, int *tokenLength,
              int maxlen, TokenDelimiter delimiter);
    inline const QChar *readPtr() const;
    inline void consumeLastToken();
    inline void consume(int nchars);
    void saveConverterState(qint64 newPos);
    void restoreToSavedConverterState();
    int lastTokenSize;

    // Return value type for getNumber()
    enum NumberParsingStatus {
        npsOk,
        npsMissingDigit,
        npsInvalidPrefix
    };

    inline bool getChar(QChar *ch);
    inline void ungetChar(const QChar &ch);
    NumberParsingStatus getNumber(qulonglong *l);
    bool getReal(double *f);

    inline void write(const QString &data);
    inline void putString(const QString &ch, bool number = false);
    void putNumber(qulonglong number, bool negative);

    // buffers
    bool fillReadBuffer(qint64 maxBytes = -1);
    void resetReadBuffer();
    void flushWriteBuffer();
    QString writeBuffer;
    QString readBuffer;
    int readBufferOffset;
    int readConverterSavedStateOffset; //the offset between readBufferStartDevicePos and that start of the buffer
    qint64 readBufferStartDevicePos;

    // streaming parameters
    int realNumberPrecision;
    int integerBase;
    int fieldWidth;
    QChar padChar;
    QTextStream::FieldAlignment fieldAlignment;
    QTextStream::RealNumberNotation realNumberNotation;
    QTextStream::NumberFlags numberFlags;

    // status
    QTextStream::Status status;

    QLocale locale;

    QTextStream *q_ptr;
};

/*! \internal
*/
QTextStreamPrivate::QTextStreamPrivate(QTextStream *q_ptr)
    :
#ifndef QT_NO_TEXTCODEC
    readConverterSavedState(0),
#endif
    readConverterSavedStateOffset(0),
    locale(QLocale::c())
{
    this->q_ptr = q_ptr;
    reset();
}

/*! \internal
*/
QTextStreamPrivate::~QTextStreamPrivate()
{
    if (deleteDevice) {
#ifndef QT_NO_QOBJECT
        device->blockSignals(true);
#endif
        delete device;
    }
#ifndef QT_NO_TEXTCODEC
    delete readConverterSavedState;
#endif
}

#ifndef QT_NO_TEXTCODEC
static void resetCodecConverterStateHelper(QTextCodec::ConverterState *state)
{
    state->~ConverterState();
    new (state) QTextCodec::ConverterState;
}

static void copyConverterStateHelper(QTextCodec::ConverterState *dest,
    const QTextCodec::ConverterState *src)
{
    // ### QTextCodec::ConverterState's copy constructors and assignments are
    // private. This function copies the structure manually.
    Q_ASSERT(!src->d);
    dest->flags = src->flags;
    dest->invalidChars = src->invalidChars;
    dest->state_data[0] = src->state_data[0];
    dest->state_data[1] = src->state_data[1];
    dest->state_data[2] = src->state_data[2];
}
#endif

/*! \internal
*/
void QTextStreamPrivate::reset()
{
    realNumberPrecision = 6;
    integerBase = 0;
    fieldWidth = 0;
    padChar = QLatin1Char(' ');
    fieldAlignment = QTextStream::AlignRight;
    realNumberNotation = QTextStream::SmartNotation;
    numberFlags = 0;

    device = 0;
    deleteDevice = false;
    string = 0;
    stringOffset = 0;
    stringOpenMode = QIODevice::NotOpen;

    readBufferOffset = 0;
    readBufferStartDevicePos = 0;
    lastTokenSize = 0;

#ifndef QT_NO_TEXTCODEC
    codec = QTextCodec::codecForLocale();
    resetCodecConverterStateHelper(&readConverterState);
    resetCodecConverterStateHelper(&writeConverterState);
    delete readConverterSavedState;
    readConverterSavedState = 0;
    writeConverterState.flags |= QTextCodec::IgnoreHeader;
    autoDetectUnicode = true;
#endif
}

/*! \internal
*/
bool QTextStreamPrivate::fillReadBuffer(qint64 maxBytes)
{
    // no buffer next to the QString itself; this function should only
    // be called internally, for devices.
    Q_ASSERT(!string);
    Q_ASSERT(device);

    // handle text translation and bypass the Text flag in the device.
    bool textModeEnabled = device->isTextModeEnabled();
    if (textModeEnabled)
        device->setTextModeEnabled(false);

    // read raw data into a temporary buffer
    char buf[QTEXTSTREAM_BUFFERSIZE];
    qint64 bytesRead = 0;
#if defined(Q_OS_WIN)
    // On Windows, there is no non-blocking stdin - so we fall back to reading
    // lines instead. If there is no QOBJECT, we read lines for all sequential
    // devices; otherwise, we read lines only for stdin.
    QFile *file = 0;
    Q_UNUSED(file);
    if (device->isSequential()
#if !defined(QT_NO_QOBJECT)
        && (file = qobject_cast<QFile *>(device)) && file->handle() == 0
#endif
        ) {
        if (maxBytes != -1)
            bytesRead = device->readLine(buf, qMin<qint64>(sizeof(buf), maxBytes));
        else
            bytesRead = device->readLine(buf, sizeof(buf));
    } else
#endif
    {
        if (maxBytes != -1)
            bytesRead = device->read(buf, qMin<qint64>(sizeof(buf), maxBytes));
        else
            bytesRead = device->read(buf, sizeof(buf));
    }

#ifndef QT_NO_TEXTCODEC
    // codec auto detection, explicitly defaults to locale encoding if the
    // codec has been set to 0.
    if (!codec || autoDetectUnicode) {
        autoDetectUnicode = false;

        codec = QTextCodec::codecForUtfText(QByteArray::fromRawData(buf, bytesRead), codec);
        if (!codec) {
            codec = QTextCodec::codecForLocale();
            writeConverterState.flags |= QTextCodec::IgnoreHeader;
        }
    }
#if defined (QTEXTSTREAM_DEBUG)
    qDebug("QTextStreamPrivate::fillReadBuffer(), using %s codec",
           codec->name().constData());
#endif
#endif

#if defined (QTEXTSTREAM_DEBUG)
    qDebug("QTextStreamPrivate::fillReadBuffer(), device->read(\"%s\", %d) == %d",
           qt_prettyDebug(buf, qMin(32,int(bytesRead)) , int(bytesRead)).constData(), sizeof(buf), int(bytesRead));
#endif

    if (bytesRead <= 0)
        return false;

    int oldReadBufferSize = readBuffer.size();
#ifndef QT_NO_TEXTCODEC
    // convert to unicode
    readBuffer += codec->toUnicode(buf, bytesRead, &readConverterState);
#else
    readBuffer += QString::fromLatin1(QByteArray(buf, bytesRead).constData());
#endif

    // reset the Text flag.
    if (textModeEnabled)
        device->setTextModeEnabled(true);

    // remove all '\r\n' in the string.
    if (readBuffer.size() > oldReadBufferSize && textModeEnabled) {
        QChar CR = QLatin1Char('\r');
        QChar *writePtr = readBuffer.data() + oldReadBufferSize;
        QChar *readPtr = readBuffer.data() + oldReadBufferSize;
        QChar *endPtr = readBuffer.data() + readBuffer.size();

        int n = oldReadBufferSize;
        if (readPtr < endPtr) {
            // Cut-off to avoid unnecessary self-copying.
            while (*readPtr++ != CR) {
                ++n;
                if (++writePtr == endPtr)
                    break;
            }
        }
        while (readPtr < endPtr) {
            QChar ch = *readPtr++;
            if (ch != CR) {
                *writePtr++ = ch;
            } else {
                if (n < readBufferOffset)
                    --readBufferOffset;
                --bytesRead;
            }
            ++n;
        }
        readBuffer.resize(writePtr - readBuffer.data());
    }

#if defined (QTEXTSTREAM_DEBUG)
    qDebug("QTextStreamPrivate::fillReadBuffer() read %d bytes from device. readBuffer = [%s]", int(bytesRead),
           qt_prettyDebug(readBuffer.toLatin1(), readBuffer.size(), readBuffer.size()).data());
#endif
    return true;
}

/*! \internal
*/
void QTextStreamPrivate::resetReadBuffer()
{
    readBuffer.clear();
    readBufferOffset = 0;
    readBufferStartDevicePos = (device ? device->pos() : 0);
}

/*! \internal
*/
void QTextStreamPrivate::flushWriteBuffer()
{
    // no buffer next to the QString itself; this function should only
    // be called internally, for devices.
    if (string || !device)
        return;

    // Stream went bye-bye already. Appending further data may succeed again,
    // but would create a corrupted stream anyway.
    if (status != QTextStream::Ok)
        return;

    if (writeBuffer.isEmpty())
        return;

#if defined (Q_OS_WIN)
    // handle text translation and bypass the Text flag in the device.
    bool textModeEnabled = device->isTextModeEnabled();
    if (textModeEnabled) {
        device->setTextModeEnabled(false);
        writeBuffer.replace(QLatin1Char('\n'), QLatin1String("\r\n"));
    }
#endif

#ifndef QT_NO_TEXTCODEC
    if (!codec)
        codec = QTextCodec::codecForLocale();
#if defined (QTEXTSTREAM_DEBUG)
    qDebug("QTextStreamPrivate::flushWriteBuffer(), using %s codec (%s generating BOM)",
           codec->name().constData(), writeConverterState.flags & QTextCodec::IgnoreHeader ? "not" : "");
#endif

    // convert from unicode to raw data
    QByteArray data = codec->fromUnicode(writeBuffer.data(), writeBuffer.size(), &writeConverterState);
#else
    QByteArray data = writeBuffer.toLocal8Bit();
#endif
    writeBuffer.clear();

    // write raw data to the device
    qint64 bytesWritten = device->write(data);
#if defined (QTEXTSTREAM_DEBUG)
    qDebug("QTextStreamPrivate::flushWriteBuffer(), device->write(\"%s\") == %d",
           qt_prettyDebug(data.constData(), qMin(data.size(),32), data.size()).constData(), int(bytesWritten));
#endif
    if (bytesWritten <= 0) {
        status = QTextStream::WriteFailed;
        return;
    }

#if defined (Q_OS_WIN)
    // replace the text flag
    if (textModeEnabled)
        device->setTextModeEnabled(true);
#endif

    // flush the file
#ifndef QT_NO_QOBJECT
    QFile *file = qobject_cast<QFile *>(device);
    bool flushed = !file || file->flush();
#else
    bool flushed = true;
#endif

#if defined (QTEXTSTREAM_DEBUG)
    qDebug("QTextStreamPrivate::flushWriteBuffer() wrote %d bytes",
           int(bytesWritten));
#endif
    if (!flushed || bytesWritten != qint64(data.size()))
        status = QTextStream::WriteFailed;
}

QString QTextStreamPrivate::read(int maxlen)
{
    QString ret;
    if (string) {
        lastTokenSize = qMin(maxlen, string->size() - stringOffset);
        ret = string->mid(stringOffset, lastTokenSize);
    } else {
        while (readBuffer.size() - readBufferOffset < maxlen && fillReadBuffer()) ;
        lastTokenSize = qMin(maxlen, readBuffer.size() - readBufferOffset);
        ret = readBuffer.mid(readBufferOffset, lastTokenSize);
    }
    consumeLastToken();

#if defined (QTEXTSTREAM_DEBUG)
    qDebug("QTextStreamPrivate::read() maxlen = %d, token length = %d", maxlen, ret.length());
#endif
    return ret;
}

/*! \internal

    Scans no more than \a maxlen QChars in the current buffer for the
    first \a delimiter. Stores a pointer to the start offset of the
    token in \a ptr, and the length in QChars in \a length.
*/
bool QTextStreamPrivate::scan(const QChar **ptr, int *length, int maxlen, TokenDelimiter delimiter)
{
    int totalSize = 0;
    int delimSize = 0;
    bool consumeDelimiter = false;
    bool foundToken = false;
    int startOffset = device ? readBufferOffset : stringOffset;
    QChar lastChar;

    bool canStillReadFromDevice = true;
    do {
        int endOffset;
        const QChar *chPtr;
        if (device) {
            chPtr = readBuffer.constData();
            endOffset = readBuffer.size();
        } else {
            chPtr = string->constData();
            endOffset = string->size();
        }
        chPtr += startOffset;

        for (; !foundToken && startOffset < endOffset && (!maxlen || totalSize < maxlen); ++startOffset) {
            const QChar ch = *chPtr++;
            ++totalSize;

            switch (delimiter) {
            case Space:
                if (ch.isSpace()) {
                    foundToken = true;
                    delimSize = 1;
                }
                break;
            case NotSpace:
                if (!ch.isSpace()) {
                    foundToken = true;
                    delimSize = 1;
                }
                break;
            case EndOfLine:
                if (ch == QLatin1Char('\n')) {
                    foundToken = true;
                    delimSize = (lastChar == QLatin1Char('\r')) ? 2 : 1;
                    consumeDelimiter = true;
                }
                lastChar = ch;
                break;
            }
        }
    } while (!foundToken
             && (!maxlen || totalSize < maxlen)
             && (device && (canStillReadFromDevice = fillReadBuffer())));

    // if the token was not found, but we reached the end of input,
    // then we accept what we got. if we are not at the end of input,
    // we return false.
    if (!foundToken && (!maxlen || totalSize < maxlen)
        && (totalSize == 0
            || (string && stringOffset + totalSize < string->size())
            || (device && !device->atEnd() && canStillReadFromDevice))) {
#if defined (QTEXTSTREAM_DEBUG)
        qDebug("QTextStreamPrivate::scan() did not find the token.");
#endif
        return false;
    }

    // if we find a '\r' at the end of the data when reading lines,
    // don't make it part of the line.
    if (delimiter == EndOfLine && totalSize > 0 && !foundToken) {
        if (((string && stringOffset + totalSize == string->size()) || (device && device->atEnd()))
            && lastChar == QLatin1Char('\r')) {
            consumeDelimiter = true;
            ++delimSize;
        }
    }

    // set the read offset and length of the token
    if (length)
        *length = totalSize - delimSize;
    if (ptr)
        *ptr = readPtr();

    // update last token size. the callee will call consumeLastToken() when
    // done.
    lastTokenSize = totalSize;
    if (!consumeDelimiter)
        lastTokenSize -= delimSize;

#if defined (QTEXTSTREAM_DEBUG)
    qDebug("QTextStreamPrivate::scan(%p, %p, %d, %x) token length = %d, delimiter = %d",
           ptr, length, maxlen, (int)delimiter, totalSize - delimSize, delimSize);
#endif
    return true;
}

/*! \internal
*/
inline const QChar *QTextStreamPrivate::readPtr() const
{
    Q_ASSERT(readBufferOffset <= readBuffer.size());
    if (string)
        return string->constData() + stringOffset;
    return readBuffer.constData() + readBufferOffset;
}

/*! \internal
*/
inline void QTextStreamPrivate::consumeLastToken()
{
    if (lastTokenSize)
        consume(lastTokenSize);
    lastTokenSize = 0;
}

/*! \internal
*/
inline void QTextStreamPrivate::consume(int size)
{
#if defined (QTEXTSTREAM_DEBUG)
    qDebug("QTextStreamPrivate::consume(%d)", size);
#endif
    if (string) {
        stringOffset += size;
        if (stringOffset > string->size())
            stringOffset = string->size();
    } else {
        readBufferOffset += size;
        if (readBufferOffset >= readBuffer.size()) {
            readBufferOffset = 0;
            readBuffer.clear();
            saveConverterState(device->pos());
        } else if (readBufferOffset > QTEXTSTREAM_BUFFERSIZE) {
            readBuffer = readBuffer.remove(0,readBufferOffset);
            readConverterSavedStateOffset += readBufferOffset;
            readBufferOffset = 0;
        }
    }
}

/*! \internal
*/
inline void QTextStreamPrivate::saveConverterState(qint64 newPos)
{
#ifndef QT_NO_TEXTCODEC
    if (readConverterState.d) {
        // converter cannot be copied, so don't save anything
        // don't update readBufferStartDevicePos either
        return;
    }

    if (!readConverterSavedState)
        readConverterSavedState = new QTextCodec::ConverterState;
    copyConverterStateHelper(readConverterSavedState, &readConverterState);
#endif

    readBufferStartDevicePos = newPos;
    readConverterSavedStateOffset = 0;
}

/*! \internal
*/
inline void QTextStreamPrivate::restoreToSavedConverterState()
{
#ifndef QT_NO_TEXTCODEC
    if (readConverterSavedState) {
        // we have a saved state
        // that means the converter can be copied
        copyConverterStateHelper(&readConverterState, readConverterSavedState);
    } else {
        // the only state we could save was the initial
        // so reset to that
        resetCodecConverterStateHelper(&readConverterState);
    }
#endif
}

/*! \internal
*/
inline void QTextStreamPrivate::write(const QString &data)
{
    if (string) {
        // ### What about seek()??
        string->append(data);
    } else {
        writeBuffer += data;
        if (writeBuffer.size() > QTEXTSTREAM_BUFFERSIZE)
            flushWriteBuffer();
    }
}

/*! \internal
*/
inline bool QTextStreamPrivate::getChar(QChar *ch)
{
    if ((string && stringOffset == string->size())
        || (device && readBuffer.isEmpty() && !fillReadBuffer())) {
        if (ch)
            *ch = 0;
        return false;
    }
    if (ch)
        *ch = *readPtr();
    consume(1);
    return true;
}

/*! \internal
*/
inline void QTextStreamPrivate::ungetChar(const QChar &ch)
{
    if (string) {
        if (stringOffset == 0)
            string->prepend(ch);
        else
            (*string)[--stringOffset] = ch;
        return;
    }

    if (readBufferOffset == 0) {
        readBuffer.prepend(ch);
        return;
    }

    readBuffer[--readBufferOffset] = ch;
}

/*! \internal
*/
inline void QTextStreamPrivate::putString(const QString &s, bool number)
{
    QString tmp = s;

    // handle padding
    int padSize = fieldWidth - s.size();
    if (padSize > 0) {
        QString pad(padSize, padChar);
        if (fieldAlignment == QTextStream::AlignLeft) {
            tmp.append(QString(padSize, padChar));
        } else if (fieldAlignment == QTextStream::AlignRight
                   || fieldAlignment == QTextStream::AlignAccountingStyle) {
            tmp.prepend(QString(padSize, padChar));
            if (fieldAlignment == QTextStream::AlignAccountingStyle && number) {
                const QChar sign = s.size() > 0 ? s.at(0) : QChar();
                if (sign == locale.negativeSign() || sign == locale.positiveSign()) {
                    QChar *data = tmp.data();
                    data[padSize] = tmp.at(0);
                    data[0] = sign;
                }
           }
        } else if (fieldAlignment == QTextStream::AlignCenter) {
            tmp.prepend(QString(padSize/2, padChar));
            tmp.append(QString(padSize - padSize/2, padChar));
        }
    }

#if defined (QTEXTSTREAM_DEBUG)
    QByteArray a = s.toUtf8();
    QByteArray b = tmp.toUtf8();
    qDebug("QTextStreamPrivate::putString(\"%s\") calls write(\"%s\")",
           qt_prettyDebug(a.constData(), a.size(), qMax(16, a.size())).constData(),
           qt_prettyDebug(b.constData(), b.size(), qMax(16, b.size())).constData());
#endif
    write(tmp);
}

/*!
    Constructs a QTextStream. Before you can use it for reading or
    writing, you must assign a device or a string.

    \sa setDevice(), setString()
*/
QTextStream::QTextStream()
    : d_ptr(new QTextStreamPrivate(this))
{
#if defined (QTEXTSTREAM_DEBUG)
    qDebug("QTextStream::QTextStream()");
#endif
    Q_D(QTextStream);
    d->status = Ok;
}

/*!
    Constructs a QTextStream that operates on \a device.
*/
QTextStream::QTextStream(QIODevice *device)
    : d_ptr(new QTextStreamPrivate(this))
{
#if defined (QTEXTSTREAM_DEBUG)
    qDebug("QTextStream::QTextStream(QIODevice *device == *%p)",
           device);
#endif
    Q_D(QTextStream);
    d->device = device;
#ifndef QT_NO_QOBJECT
    d->deviceClosedNotifier.setupDevice(this, d->device);
#endif
    d->status = Ok;
}

/*!
    Constructs a QTextStream that operates on \a string, using \a
    openMode to define the open mode.
*/
QTextStream::QTextStream(QString *string, QIODevice::OpenMode openMode)
    : d_ptr(new QTextStreamPrivate(this))
{
#if defined (QTEXTSTREAM_DEBUG)
    qDebug("QTextStream::QTextStream(QString *string == *%p, openMode = %d)",
           string, int(openMode));
#endif
    Q_D(QTextStream);
    d->string = string;
    d->stringOpenMode = openMode;
    d->status = Ok;
}

/*!
    Constructs a QTextStream that operates on \a array, using \a
    openMode to define the open mode. Internally, the array is wrapped
    by a QBuffer.
*/
QTextStream::QTextStream(QByteArray *array, QIODevice::OpenMode openMode)
    : d_ptr(new QTextStreamPrivate(this))
{
#if defined (QTEXTSTREAM_DEBUG)
    qDebug("QTextStream::QTextStream(QByteArray *array == *%p, openMode = %d)",
           array, int(openMode));
#endif
    Q_D(QTextStream);
    d->device = new QBuffer(array);
    d->device->open(openMode);
    d->deleteDevice = true;
#ifndef QT_NO_QOBJECT
    d->deviceClosedNotifier.setupDevice(this, d->device);
#endif
    d->status = Ok;
}

/*!
    Constructs a QTextStream that operates on \a array, using \a
    openMode to define the open mode. The array is accessed as
    read-only, regardless of the values in \a openMode.

    This constructor is convenient for working on constant
    strings. Example:

    \snippet doc/src/snippets/code/src_corelib_io_qtextstream.cpp 3
*/
QTextStream::QTextStream(const QByteArray &array, QIODevice::OpenMode openMode)
    : d_ptr(new QTextStreamPrivate(this))
{
#if defined (QTEXTSTREAM_DEBUG)
    qDebug("QTextStream::QTextStream(const QByteArray &array == *(%p), openMode = %d)",
           &array, int(openMode));
#endif
    QBuffer *buffer = new QBuffer;
    buffer->setData(array);
    buffer->open(openMode);

    Q_D(QTextStream);
    d->device = buffer;
    d->deleteDevice = true;
#ifndef QT_NO_QOBJECT
    d->deviceClosedNotifier.setupDevice(this, d->device);
#endif
    d->status = Ok;
}

/*!
    Constructs a QTextStream that operates on \a fileHandle, using \a
    openMode to define the open mode. Internally, a QFile is created
    to handle the FILE pointer.

    This constructor is useful for working directly with the common
    FILE based input and output streams: stdin, stdout and stderr. Example:

    \snippet doc/src/snippets/code/src_corelib_io_qtextstream.cpp 4
*/

QTextStream::QTextStream(FILE *fileHandle, QIODevice::OpenMode openMode)
    : d_ptr(new QTextStreamPrivate(this))
{
#if defined (QTEXTSTREAM_DEBUG)
    qDebug("QTextStream::QTextStream(FILE *fileHandle = %p, openMode = %d)",
           fileHandle, int(openMode));
#endif
    QFile *file = new QFile;
    file->open(fileHandle, openMode);

    Q_D(QTextStream);
    d->device = file;
    d->deleteDevice = true;
#ifndef QT_NO_QOBJECT
    d->deviceClosedNotifier.setupDevice(this, d->device);
#endif
    d->status = Ok;
}

/*!
    Destroys the QTextStream.

    If the stream operates on a device, flush() will be called
    implicitly. Otherwise, the device is unaffected.
*/
QTextStream::~QTextStream()
{
    Q_D(QTextStream);
#if defined (QTEXTSTREAM_DEBUG)
    qDebug("QTextStream::~QTextStream()");
#endif
    if (!d->writeBuffer.isEmpty())
        d->flushWriteBuffer();
}

/*!
    Resets QTextStream's formatting options, bringing it back to its
    original constructed state. The device, string and any buffered
    data is left untouched.
*/
void QTextStream::reset()
{
    Q_D(QTextStream);

    d->realNumberPrecision = 6;
    d->integerBase = 0;
    d->fieldWidth = 0;
    d->padChar = QLatin1Char(' ');
    d->fieldAlignment = QTextStream::AlignRight;
    d->realNumberNotation = QTextStream::SmartNotation;
    d->numberFlags = 0;
}

/*!
    Flushes any buffered data waiting to be written to the device.

    If QTextStream operates on a string, this function does nothing.
*/
void QTextStream::flush()
{
    Q_D(QTextStream);
    d->flushWriteBuffer();
}

/*!
    Seeks to the position \a pos in the device. Returns true on
    success; otherwise returns false.
*/
bool QTextStream::seek(qint64 pos)
{
    Q_D(QTextStream);
    d->lastTokenSize = 0;

    if (d->device) {
        // Empty the write buffer
        d->flushWriteBuffer();
        if (!d->device->seek(pos))
            return false;
        d->resetReadBuffer();

#ifndef QT_NO_TEXTCODEC
        // Reset the codec converter states.
        resetCodecConverterStateHelper(&d->readConverterState);
        resetCodecConverterStateHelper(&d->writeConverterState);
        delete d->readConverterSavedState;
        d->readConverterSavedState = 0;
        d->writeConverterState.flags |= QTextCodec::IgnoreHeader;
#endif
        return true;
    }

    // string
    if (d->string && pos <= d->string->size()) {
        d->stringOffset = int(pos);
        return true;
    }
    return false;
}

/*!
    \since 4.2

    Returns the device position corresponding to the current position of the
    stream, or -1 if an error occurs (e.g., if there is no device or string,
    or if there's a device error).

    Because QTextStream is buffered, this function may have to
    seek the device to reconstruct a valid device position. This
    operation can be expensive, so you may want to avoid calling this
    function in a tight loop.

    \sa seek()
*/
qint64 QTextStream::pos() const
{
    Q_D(const QTextStream);
    if (d->device) {
        // Cutoff
        if (d->readBuffer.isEmpty())
            return d->device->pos();
        if (d->device->isSequential())
            return 0;

        // Seek the device
        if (!d->device->seek(d->readBufferStartDevicePos))
            return qint64(-1);

        // Reset the read buffer
        QTextStreamPrivate *thatd = const_cast<QTextStreamPrivate *>(d);
        thatd->readBuffer.clear();

#ifndef QT_NO_TEXTCODEC
        thatd->restoreToSavedConverterState();
        if (d->readBufferStartDevicePos == 0)
            thatd->autoDetectUnicode = true;
#endif

        // Rewind the device to get to the current position Ensure that
        // readBufferOffset is unaffected by fillReadBuffer()
        int oldReadBufferOffset = d->readBufferOffset + d->readConverterSavedStateOffset;
        while (d->readBuffer.size() < oldReadBufferOffset) {
            if (!thatd->fillReadBuffer(1))
                return qint64(-1);
        }
        thatd->readBufferOffset = oldReadBufferOffset;
        thatd->readConverterSavedStateOffset = 0;

        // Return the device position.
        return d->device->pos();
    }

    if (d->string)
        return d->stringOffset;

    qWarning("QTextStream::pos: no device");
    return qint64(-1);
}

/*!
    Reads and discards whitespace from the stream until either a
    non-space character is detected, or until atEnd() returns
    true. This function is useful when reading a stream character by
    character.

    Whitespace characters are all characters for which
    QChar::isSpace() returns true.

    \sa operator>>()
*/
void QTextStream::skipWhiteSpace()
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(Q_VOID);
    d->scan(0, 0, 0, QTextStreamPrivate::NotSpace);
    d->consumeLastToken();
}

/*!
    Sets the current device to \a device. If a device has already been
    assigned, QTextStream will call flush() before the old device is
    replaced.

    \note This function resets locale to the default locale ('C')
    and codec to the default codec, QTextCodec::codecForLocale().

    \sa device(), setString()
*/
void QTextStream::setDevice(QIODevice *device)
{
    Q_D(QTextStream);
    flush();
    if (d->deleteDevice) {
#ifndef QT_NO_QOBJECT
        d->deviceClosedNotifier.disconnect();
#endif
        delete d->device;
        d->deleteDevice = false;
    }

    d->reset();
    d->status = Ok;
    d->device = device;
    d->resetReadBuffer();
#ifndef QT_NO_QOBJECT
    d->deviceClosedNotifier.setupDevice(this, d->device);
#endif
}

/*!
    Returns the current device associated with the QTextStream,
    or 0 if no device has been assigned.

    \sa setDevice(), string()
*/
QIODevice *QTextStream::device() const
{
    Q_D(const QTextStream);
    return d->device;
}

/*!
    Sets the current string to \a string, using the given \a
    openMode. If a device has already been assigned, QTextStream will
    call flush() before replacing it.

    \sa string(), setDevice()
*/
void QTextStream::setString(QString *string, QIODevice::OpenMode openMode)
{
    Q_D(QTextStream);
    flush();
    if (d->deleteDevice) {
#ifndef QT_NO_QOBJECT
        d->deviceClosedNotifier.disconnect();
        d->device->blockSignals(true);
#endif
        delete d->device;
        d->deleteDevice = false;
    }

    d->reset();
    d->status = Ok;
    d->string = string;
    d->stringOpenMode = openMode;
}

/*!
    Returns the current string assigned to the QTextStream, or 0 if no
    string has been assigned.

    \sa setString(), device()
*/
QString *QTextStream::string() const
{
    Q_D(const QTextStream);
    return d->string;
}

/*!
    Sets the field alignment to \a mode. When used together with
    setFieldWidth(), this function allows you to generate formatted
    output with text aligned to the left, to the right or center
    aligned.

    \sa fieldAlignment(), setFieldWidth()
*/
void QTextStream::setFieldAlignment(FieldAlignment mode)
{
    Q_D(QTextStream);
    d->fieldAlignment = mode;
}

/*!
    Returns the current field alignment.

    \sa setFieldAlignment(), fieldWidth()
*/
QTextStream::FieldAlignment QTextStream::fieldAlignment() const
{
    Q_D(const QTextStream);
    return d->fieldAlignment;
}

/*!
    Sets the pad character to \a ch. The default value is the ASCII
    space character (' '), or QChar(0x20). This character is used to
    fill in the space in fields when generating text.

    Example:

    \snippet doc/src/snippets/code/src_corelib_io_qtextstream.cpp 5

    The string \c s contains:

    \snippet doc/src/snippets/code/src_corelib_io_qtextstream.cpp 6

    \sa padChar(), setFieldWidth()
*/
void QTextStream::setPadChar(QChar ch)
{
    Q_D(QTextStream);
    d->padChar = ch;
}

/*!
    Returns the current pad character.

    \sa setPadChar(), setFieldWidth()
*/
QChar QTextStream::padChar() const
{
    Q_D(const QTextStream);
    return d->padChar;
}

/*!
    Sets the current field width to \a width. If \a width is 0 (the
    default), the field width is equal to the length of the generated
    text.

    \note The field width applies to every element appended to this
    stream after this function has been called (e.g., it also pads
    endl). This behavior is different from similar classes in the STL,
    where the field width only applies to the next element.

    \sa fieldWidth(), setPadChar()
*/
void QTextStream::setFieldWidth(int width)
{
    Q_D(QTextStream);
    d->fieldWidth = width;
}

/*!
    Returns the current field width.

    \sa setFieldWidth()
*/
int QTextStream::fieldWidth() const
{
    Q_D(const QTextStream);
    return d->fieldWidth;
}

/*!
    Sets the current number flags to \a flags. \a flags is a set of
    flags from the NumberFlag enum, and describes options for
    formatting generated code (e.g., whether or not to always write
    the base or sign of a number).

    \sa numberFlags(), setIntegerBase(), setRealNumberNotation()
*/
void QTextStream::setNumberFlags(NumberFlags flags)
{
    Q_D(QTextStream);
    d->numberFlags = flags;
}

/*!
    Returns the current number flags.

    \sa setNumberFlags(), integerBase(), realNumberNotation()
*/
QTextStream::NumberFlags QTextStream::numberFlags() const
{
    Q_D(const QTextStream);
    return d->numberFlags;
}

/*!
    Sets the base of integers to \a base, both for reading and for
    generating numbers. \a base can be either 2 (binary), 8 (octal),
    10 (decimal) or 16 (hexadecimal). If \a base is 0, QTextStream
    will attempt to detect the base by inspecting the data on the
    stream. When generating numbers, QTextStream assumes base is 10
    unless the base has been set explicitly.

    \sa integerBase(), QString::number(), setNumberFlags()
*/
void QTextStream::setIntegerBase(int base)
{
    Q_D(QTextStream);
    d->integerBase = base;
}

/*!
    Returns the current base of integers. 0 means that the base is
    detected when reading, or 10 (decimal) when generating numbers.

    \sa setIntegerBase(), QString::number(), numberFlags()
*/
int QTextStream::integerBase() const
{
    Q_D(const QTextStream);
    return d->integerBase;
}

/*!
    Sets the real number notation to \a notation (SmartNotation,
    FixedNotation, ScientificNotation). When reading and generating
    numbers, QTextStream uses this value to detect the formatting of
    real numbers.

    \sa realNumberNotation(), setRealNumberPrecision(), setNumberFlags(), setIntegerBase()
*/
void QTextStream::setRealNumberNotation(RealNumberNotation notation)
{
    Q_D(QTextStream);
    d->realNumberNotation = notation;
}

/*!
    Returns the current real number notation.

    \sa setRealNumberNotation(), realNumberPrecision(), numberFlags(), integerBase()
*/
QTextStream::RealNumberNotation QTextStream::realNumberNotation() const
{
    Q_D(const QTextStream);
    return d->realNumberNotation;
}

/*!
    Sets the precision of real numbers to \a precision. This value
    describes the number of fraction digits QTextStream should
    write when generating real numbers.

    The precision cannot be a negative value. The default value is 6.

    \sa realNumberPrecision(), setRealNumberNotation()
*/
void QTextStream::setRealNumberPrecision(int precision)
{
    Q_D(QTextStream);
    if (precision < 0) {
        qWarning("QTextStream::setRealNumberPrecision: Invalid precision (%d)", precision);
        d->realNumberPrecision = 6;
        return;
    }
    d->realNumberPrecision = precision;
}

/*!
    Returns the current real number precision, or the number of fraction
    digits QTextStream will write when generating real numbers.

    \sa setRealNumberNotation(), realNumberNotation(), numberFlags(), integerBase()
*/
int QTextStream::realNumberPrecision() const
{
    Q_D(const QTextStream);
    return d->realNumberPrecision;
}

/*!
    Returns the status of the text stream.

    \sa QTextStream::Status, setStatus(), resetStatus()
*/

QTextStream::Status QTextStream::status() const
{
    Q_D(const QTextStream);
    return d->status;
}

/*!
    \since 4.1

    Resets the status of the text stream.

    \sa QTextStream::Status, status(), setStatus()
*/
void QTextStream::resetStatus()
{
    Q_D(QTextStream);
    d->status = Ok;
}

/*!
    \since 4.1

    Sets the status of the text stream to the \a status given.

    Subsequent calls to setStatus() are ignored until resetStatus()
    is called.

    \sa Status status() resetStatus()
*/
void QTextStream::setStatus(Status status)
{
    Q_D(QTextStream);
    if (d->status == Ok)
        d->status = status;
}

/*!
    Returns true if there is no more data to be read from the
    QTextStream; otherwise returns false. This is similar to, but not
    the same as calling QIODevice::atEnd(), as QTextStream also takes
    into account its internal Unicode buffer.
*/
bool QTextStream::atEnd() const
{
    Q_D(const QTextStream);
    CHECK_VALID_STREAM(true);

    if (d->string)
        return d->string->size() == d->stringOffset;
    return d->readBuffer.isEmpty() && d->device->atEnd();
}

/*!
    Reads the entire content of the stream, and returns it as a
    QString. Avoid this function when working on large files, as it
    will consume a significant amount of memory.

    Calling readLine() is better if you do not know how much data is
    available.

    \sa readLine()
*/
QString QTextStream::readAll()
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(QString());

    return d->read(INT_MAX);
}

/*!
    Reads one line of text from the stream, and returns it as a
    QString. The maximum allowed line length is set to \a maxlen. If
    the stream contains lines longer than this, then the lines will be
    split after \a maxlen characters and returned in parts.

    If \a maxlen is 0, the lines can be of any length. A common value
    for \a maxlen is 75.

    The returned line has no trailing end-of-line characters ("\\n"
    or "\\r\\n"), so calling QString::trimmed() is unnecessary.

    If the stream has read to the end of the file, readLine() will return a
    null QString. For strings, or for devices that support it, you can
    explicitly test for the end of the stream using atEnd().

    \sa readAll(), QIODevice::readLine()
*/
QString QTextStream::readLine(qint64 maxlen)
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(QString());

    const QChar *readPtr;
    int length;
    if (!d->scan(&readPtr, &length, int(maxlen), QTextStreamPrivate::EndOfLine))
        return QString();

    QString tmp = QString(readPtr, length);
    d->consumeLastToken();
    return tmp;
}

/*!
    \since 4.1

    Reads at most \a maxlen characters from the stream, and returns the data
    read as a QString.

    \sa readAll(), readLine(), QIODevice::read()
*/
QString QTextStream::read(qint64 maxlen)
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(QString());

    if (maxlen <= 0)
        return QString::fromLatin1("");     // empty, not null

    return d->read(int(maxlen));
}

/*! \internal
*/
QTextStreamPrivate::NumberParsingStatus QTextStreamPrivate::getNumber(qulonglong *ret)
{
    scan(0, 0, 0, NotSpace);
    consumeLastToken();

    // detect int encoding
    int base = integerBase;
    if (base == 0) {
        QChar ch;
        if (!getChar(&ch))
            return npsInvalidPrefix;
        if (ch == QLatin1Char('0')) {
            QChar ch2;
            if (!getChar(&ch2)) {
                // Result is the number 0
                *ret = 0;
                return npsOk;
            }
            ch2 = ch2.toLower();

            if (ch2 == QLatin1Char('x')) {
                base = 16;
            } else if (ch2 == QLatin1Char('b')) {
                base = 2;
            } else if (ch2.isDigit() && ch2.digitValue() >= 0 && ch2.digitValue() <= 7) {
                base = 8;
            } else {
                base = 10;
            }
            ungetChar(ch2);
        } else if (ch == locale.negativeSign() || ch == locale.positiveSign() || ch.isDigit()) {
            base = 10;
        } else {
            ungetChar(ch);
            return npsInvalidPrefix;
        }
        ungetChar(ch);
        // State of the stream is now the same as on entry
        // (cursor is at prefix),
        // and local variable 'base' has been set appropriately.
    }

    qulonglong val=0;
    switch (base) {
    case 2: {
        QChar pf1, pf2, dig;
        // Parse prefix '0b'
        if (!getChar(&pf1) || pf1 != QLatin1Char('0'))
            return npsInvalidPrefix;
        if (!getChar(&pf2) || pf2.toLower() != QLatin1Char('b'))
            return npsInvalidPrefix;
        // Parse digits
        int ndigits = 0;
        while (getChar(&dig)) {
            int n = dig.toLower().unicode();
            if (n == '0' || n == '1') {
                val <<= 1;
                val += n - '0';
            } else {
                ungetChar(dig);
                break;
            }
            ndigits++;
        }
        if (ndigits == 0) {
            // Unwind the prefix and abort
            ungetChar(pf2);
            ungetChar(pf1);
            return npsMissingDigit;
        }
        break;
    }
    case 8: {
        QChar pf, dig;
        // Parse prefix '0'
        if (!getChar(&pf) || pf != QLatin1Char('0'))
            return npsInvalidPrefix;
        // Parse digits
        int ndigits = 0;
        while (getChar(&dig)) {
            int n = dig.toLower().unicode();
            if (n >= '0' && n <= '7') {
                val *= 8;
                val += n - '0';
            } else {
                ungetChar(dig);
                break;
            }
            ndigits++;
        }
        if (ndigits == 0) {
            // Unwind the prefix and abort
            ungetChar(pf);
            return npsMissingDigit;
        }
        break;
    }
    case 10: {
        // Parse sign (or first digit)
        QChar sign;
        int ndigits = 0;
        if (!getChar(&sign))
            return npsMissingDigit;
        if (sign != locale.negativeSign() && sign != locale.positiveSign()) {
            if (!sign.isDigit()) {
                ungetChar(sign);
                return npsMissingDigit;
            }
            val += sign.digitValue();
            ndigits++;
        }
        // Parse digits
        QChar ch;
        while (getChar(&ch)) {
            if (ch.isDigit()) {
                val *= 10;
                val += ch.digitValue();
            } else if (locale != QLocale::c() && ch == locale.groupSeparator()) {
                continue;
            } else {
                ungetChar(ch);
                break;
            }
            ndigits++;
        }
        if (ndigits == 0)
            return npsMissingDigit;
        if (sign == locale.negativeSign()) {
            qlonglong ival = qlonglong(val);
            if (ival > 0)
                ival = -ival;
            val = qulonglong(ival);
        }
        break;
    }
    case 16: {
        QChar pf1, pf2, dig;
        // Parse prefix ' 0x'
        if (!getChar(&pf1) || pf1 != QLatin1Char('0'))
            return npsInvalidPrefix;
        if (!getChar(&pf2) || pf2.toLower() != QLatin1Char('x'))
            return npsInvalidPrefix;
        // Parse digits
        int ndigits = 0;
        while (getChar(&dig)) {
            int n = dig.toLower().unicode();
            if (n >= '0' && n <= '9') {
                val <<= 4;
                val += n - '0';
            } else if (n >= 'a' && n <= 'f') {
                val <<= 4;
                val += 10 + (n - 'a');
            } else {
                ungetChar(dig);
                break;
            }
            ndigits++;
        }
        if (ndigits == 0) {
            return npsMissingDigit;
        }
        break;
    }
    default:
        // Unsupported integerBase
        return npsInvalidPrefix;
    }

    if (ret)
        *ret = val;
    return npsOk;
}

/*! \internal
    (hihi)
*/
bool QTextStreamPrivate::getReal(double *f)
{
    // We use a table-driven FSM to parse floating point numbers
    // strtod() cannot be used directly since we may be reading from a
    // QIODevice.
    enum ParserState {
        Init = 0,
        Sign = 1,
        Mantissa = 2,
        Dot = 3,
        Abscissa = 4,
        ExpMark = 5,
        ExpSign = 6,
        Exponent = 7,
        Nan1 = 8,
        Nan2 = 9,
        Inf1 = 10,
        Inf2 = 11,
        NanInf = 12,
        Done = 13
    };
    enum InputToken {
        None = 0,
        InputSign = 1,
        InputDigit = 2,
        InputDot = 3,
        InputExp = 4,
        InputI = 5,
        InputN = 6,
        InputF = 7,
        InputA = 8,
        InputT = 9
    };

    static const uchar table[13][10] = {
        // None InputSign InputDigit InputDot InputExp InputI    InputN    InputF    InputA    InputT
        { 0,    Sign,     Mantissa,  Dot,     0,       Inf1,     Nan1,     0,        0,        0      }, // 0  Init
        { 0,    0,        Mantissa,  Dot,     0,       Inf1,     Nan1,     0,        0,        0      }, // 1  Sign
        { Done, Done,     Mantissa,  Dot,     ExpMark, 0,        0,        0,        0,        0      }, // 2  Mantissa
        { 0,    0,        Abscissa,  0,       0,       0,        0,        0,        0,        0      }, // 3  Dot
        { Done, Done,     Abscissa,  Done,    ExpMark, 0,        0,        0,        0,        0      }, // 4  Abscissa
        { 0,    ExpSign,  Exponent,  0,       0,       0,        0,        0,        0,        0      }, // 5  ExpMark
        { 0,    0,        Exponent,  0,       0,       0,        0,        0,        0,        0      }, // 6  ExpSign
        { Done, Done,     Exponent,  Done,    Done,    0,        0,        0,        0,        0      }, // 7  Exponent
        { 0,    0,        0,         0,       0,       0,        0,        0,        Nan2,     0      }, // 8  Nan1
        { 0,    0,        0,         0,       0,       0,        NanInf,   0,        0,        0      }, // 9  Nan2
        { 0,    0,        0,         0,       0,       0,        Inf2,     0,        0,        0      }, // 10 Inf1
        { 0,    0,        0,         0,       0,       0,        0,        NanInf,   0,        0      }, // 11 Inf2
        { Done, 0,        0,         0,       0,       0,        0,        0,        0,        0      }, // 11 NanInf
    };

    ParserState state = Init;
    InputToken input = None;

    scan(0, 0, 0, NotSpace);
    consumeLastToken();

    const int BufferSize = 128;
    char buf[BufferSize];
    int i = 0;

    QChar c;
    while (getChar(&c)) {
        switch (c.unicode()) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            input = InputDigit;
            break;
        case 'i': case 'I':
            input = InputI;
            break;
        case 'n': case 'N':
            input = InputN;
            break;
        case 'f': case 'F':
            input = InputF;
            break;
        case 'a': case 'A':
            input = InputA;
            break;
        case 't': case 'T':
            input = InputT;
            break;
        default: {
            QChar lc = c.toLower();
            if (lc == locale.decimalPoint().toLower())
                input = InputDot;
            else if (lc == locale.exponential().toLower())
                input = InputExp;
            else if (lc == locale.negativeSign().toLower()
                     || lc == locale.positiveSign().toLower())
                input = InputSign;
            else if (locale != QLocale::c() // backward-compatibility
                     && lc == locale.groupSeparator().toLower())
                input = InputDigit; // well, it isn't a digit, but no one cares.
            else
                input = None;
        }
            break;
        }

        state = ParserState(table[state][input]);

        if  (state == Init || state == Done || i > (BufferSize - 5)) {
            ungetChar(c);
            if (i > (BufferSize - 5)) { // ignore rest of digits
                while (getChar(&c)) {
                    if (!c.isDigit()) {
                        ungetChar(c);
                        break;
                    }
                }
            }
            break;
        }

        buf[i++] = c.toLatin1();
    }

    if (i == 0)
        return false;
    if (!f)
        return true;
    buf[i] = '\0';

    // backward-compatibility. Old implementation supported +nan/-nan
    // for some reason. QLocale only checks for lower-case
    // nan/+inf/-inf, so here we also check for uppercase and mixed
    // case versions.
    if (!qstricmp(buf, "nan") || !qstricmp(buf, "+nan") || !qstricmp(buf, "-nan")) {
        *f = qSNaN();
        return true;
    } else if (!qstricmp(buf, "+inf") || !qstricmp(buf, "inf")) {
        *f = qInf();
        return true;
    } else if (!qstricmp(buf, "-inf")) {
        *f = -qInf();
        return true;
    }
    bool ok;
    *f = locale.toDouble(QString::fromLatin1(buf), &ok);
    return ok;
}

/*!
    Reads a character from the stream and stores it in \a c. Returns a
    reference to the QTextStream, so several operators can be
    nested. Example:

    \snippet doc/src/snippets/code/src_corelib_io_qtextstream.cpp 7

    Whitespace is \e not skipped.
*/

QTextStream &QTextStream::operator>>(QChar &c)
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(*this);
    d->scan(0, 0, 0, QTextStreamPrivate::NotSpace);
    if (!d->getChar(&c))
        setStatus(ReadPastEnd);
    return *this;
}

/*!
    \overload

    Reads a character from the stream and stores it in \a c. The
    character from the stream is converted to ISO-5589-1 before it is
    stored.

    \sa QChar::toLatin1()
*/
QTextStream &QTextStream::operator>>(char &c)
{
    QChar ch;
    *this >> ch;
    c = ch.toLatin1();
    return *this;
}

/*!
    Reads an integer from the stream and stores it in \a i, then
    returns a reference to the QTextStream. The number is cast to
    the correct type before it is stored. If no number was detected on
    the stream, \a i is set to 0.

    By default, QTextStream will attempt to detect the base of the
    number using the following rules:

    \table
    \header \o Prefix                \o Base
    \row    \o "0b" or "0B"          \o 2 (binary)
    \row    \o "0" followed by "0-7" \o 8 (octal)
    \row    \o "0" otherwise         \o 10 (decimal)
    \row    \o "0x" or "0X"          \o 16 (hexadecimal)
    \row    \o "1" to "9"            \o 10 (decimal)
    \endtable

    By calling setIntegerBase(), you can specify the integer base
    explicitly. This will disable the auto-detection, and speed up
    QTextStream slightly.

    Leading whitespace is skipped.
*/
QTextStream &QTextStream::operator>>(signed short &i)
{
    IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(signed short);
}

/*!
    \overload

    Stores the integer in the unsigned short \a i.
*/
QTextStream &QTextStream::operator>>(unsigned short &i)
{
    IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(unsigned short);
}

/*!
    \overload

    Stores the integer in the signed int \a i.
*/
QTextStream &QTextStream::operator>>(signed int &i)
{
    IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(signed int);
}

/*!
    \overload

    Stores the integer in the unsigned int \a i.
*/
QTextStream &QTextStream::operator>>(unsigned int &i)
{
    IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(unsigned int);
}

/*!
    \overload

    Stores the integer in the signed long \a i.
*/
QTextStream &QTextStream::operator>>(signed long &i)
{
    IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(signed long);
}

/*!
    \overload

    Stores the integer in the unsigned long \a i.
*/
QTextStream &QTextStream::operator>>(unsigned long &i)
{
    IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(unsigned long);
}

/*!
    \overload

    Stores the integer in the qlonglong \a i.
*/
QTextStream &QTextStream::operator>>(qlonglong &i)
{
    IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(qlonglong);
}

/*!
    \overload

    Stores the integer in the qulonglong \a i.
*/
QTextStream &QTextStream::operator>>(qulonglong &i)
{
    IMPLEMENT_STREAM_RIGHT_INT_OPERATOR(qulonglong);
}

/*!
    Reads a real number from the stream and stores it in \a f, then
    returns a reference to the QTextStream. The number is cast to
    the correct type. If no real number is detect on the stream, \a f
    is set to 0.0.

    As a special exception, QTextStream allows the strings "nan" and "inf" to
    represent NAN and INF floats or doubles.

    Leading whitespace is skipped.
*/
QTextStream &QTextStream::operator>>(float &f)
{
    IMPLEMENT_STREAM_RIGHT_REAL_OPERATOR(float);
}

/*!
    \overload

    Stores the real number in the double \a f.
*/
QTextStream &QTextStream::operator>>(double &f)
{
    IMPLEMENT_STREAM_RIGHT_REAL_OPERATOR(double);
}

/*!
    Reads a word from the stream and stores it in \a str, then returns
    a reference to the stream. Words are separated by whitespace
    (i.e., all characters for which QChar::isSpace() returns true).

    Leading whitespace is skipped.
*/
QTextStream &QTextStream::operator>>(QString &str)
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(*this);

    str.clear();
    d->scan(0, 0, 0, QTextStreamPrivate::NotSpace);
    d->consumeLastToken();

    const QChar *ptr;
    int length;
    if (!d->scan(&ptr, &length, 0, QTextStreamPrivate::Space)) {
        setStatus(ReadPastEnd);
        return *this;
    }

    str = QString(ptr, length);
    d->consumeLastToken();
    return *this;
}

/*!
    \overload

    Converts the word to ISO-8859-1, then stores it in \a array.

    \sa QString::toLatin1()
*/
QTextStream &QTextStream::operator>>(QByteArray &array)
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(*this);

    array.clear();
    d->scan(0, 0, 0, QTextStreamPrivate::NotSpace);
    d->consumeLastToken();

    const QChar *ptr;
    int length;
    if (!d->scan(&ptr, &length, 0, QTextStreamPrivate::Space)) {
        setStatus(ReadPastEnd);
        return *this;
    }

    for (int i = 0; i < length; ++i)
        array += ptr[i].toLatin1();

    d->consumeLastToken();
    return *this;
}

/*!
    \overload

    Stores the word in \a c, terminated by a '\0' character. If no word is
    available, only the '\0' character is stored.

    Warning: Although convenient, this operator is dangerous and must
    be used with care. QTextStream assumes that \a c points to a
    buffer with enough space to hold the word. If the buffer is too
    small, your application may crash.

    If possible, use the QByteArray operator instead.
*/
QTextStream &QTextStream::operator>>(char *c)
{
    Q_D(QTextStream);
    *c = 0;
    CHECK_VALID_STREAM(*this);
    d->scan(0, 0, 0, QTextStreamPrivate::NotSpace);
    d->consumeLastToken();

    const QChar *ptr;
    int length;
    if (!d->scan(&ptr, &length, 0, QTextStreamPrivate::Space)) {
        setStatus(ReadPastEnd);
        return *this;
    }

    for (int i = 0; i < length; ++i)
        *c++ = ptr[i].toLatin1();
    *c = '\0';
    d->consumeLastToken();
    return *this;
}

/*! \internal
 */
void QTextStreamPrivate::putNumber(qulonglong number, bool negative)
{
    QString result;

    unsigned flags = 0;
    if (numberFlags & QTextStream::ShowBase)
        flags |= QLocalePrivate::ShowBase;
    if (numberFlags & QTextStream::ForceSign)
        flags |= QLocalePrivate::AlwaysShowSign;
    if (numberFlags & QTextStream::UppercaseBase)
        flags |= QLocalePrivate::UppercaseBase;
    if (numberFlags & QTextStream::UppercaseDigits)
        flags |= QLocalePrivate::CapitalEorX;

    // add thousands group separators. For backward compatibility we
    // don't add a group separator for C locale.
    if (locale != QLocale::c())
        flags |= QLocalePrivate::ThousandsGroup;

    const QLocalePrivate *dd = locale.d();
    int base = integerBase ? integerBase : 10;
    if (negative && base == 10) {
        result = dd->longLongToString(-static_cast<qlonglong>(number), -1,
                                      base, -1, flags);
    } else if (negative) {
        // Workaround for backward compatibility for writing negative
        // numbers in octal and hex:
        // QTextStream(result) << showbase << hex << -1 << oct << -1
        // should output: -0x1 -0b1
        result = dd->unsLongLongToString(number, -1, base, -1, flags);
        result.prepend(locale.negativeSign());
    } else {
        result = dd->unsLongLongToString(number, -1, base, -1, flags);
        // workaround for backward compatibility - in octal form with
        // ShowBase flag set zero should be written as '00'
        if (number == 0 && base == 8 && numberFlags & QTextStream::ShowBase
            && result == QLatin1String("0")) {
            result.prepend(QLatin1Char('0'));
        }
    }
    putString(result, true);
}

/*!
    \internal
    \overload
*/
QTextStream &QTextStream::operator<<(QBool b)
{
    return *this << bool(b);
}

/*!
    Writes the character \a c to the stream, then returns a reference
    to the QTextStream.

    \sa setFieldWidth()
*/
QTextStream &QTextStream::operator<<(QChar c)
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(*this);
    d->putString(QString(c));
    return *this;
}

/*!
    \overload

    Converts \a c from ASCII to a QChar, then writes it to the stream.
*/
QTextStream &QTextStream::operator<<(char c)
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(*this);
    d->putString(QString(QChar::fromAscii(c)));
    return *this;
}

/*!
    Writes the integer number \a i to the stream, then returns a
    reference to the QTextStream. By default, the number is stored in
    decimal form, but you can also set the base by calling
    setIntegerBase().

    \sa setFieldWidth(), setNumberFlags()
*/
QTextStream &QTextStream::operator<<(signed short i)
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(*this);
    d->putNumber((qulonglong)qAbs(qlonglong(i)), i < 0);
    return *this;
}

/*!
    \overload

    Writes the unsigned short \a i to the stream.
*/
QTextStream &QTextStream::operator<<(unsigned short i)
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(*this);
    d->putNumber((qulonglong)i, false);
    return *this;
}

/*!
    \overload

    Writes the signed int \a i to the stream.
*/
QTextStream &QTextStream::operator<<(signed int i)
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(*this);
    d->putNumber((qulonglong)qAbs(qlonglong(i)), i < 0);
    return *this;
}

/*!
    \overload

    Writes the unsigned int \a i to the stream.
*/
QTextStream &QTextStream::operator<<(unsigned int i)
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(*this);
    d->putNumber((qulonglong)i, false);
    return *this;
}

/*!
    \overload

    Writes the signed long \a i to the stream.
*/
QTextStream &QTextStream::operator<<(signed long i)
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(*this);
    d->putNumber((qulonglong)qAbs(qlonglong(i)), i < 0);
    return *this;
}

/*!
    \overload

    Writes the unsigned long \a i to the stream.
*/
QTextStream &QTextStream::operator<<(unsigned long i)
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(*this);
    d->putNumber((qulonglong)i, false);
    return *this;
}

/*!
    \overload

    Writes the qlonglong \a i to the stream.
*/
QTextStream &QTextStream::operator<<(qlonglong i)
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(*this);
    d->putNumber((qulonglong)qAbs(i), i < 0);
    return *this;
}

/*!
    \overload

    Writes the qulonglong \a i to the stream.
*/
QTextStream &QTextStream::operator<<(qulonglong i)
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(*this);
    d->putNumber(i, false);
    return *this;
}

/*!
    Writes the real number \a f to the stream, then returns a
    reference to the QTextStream. By default, QTextStream stores it
    using SmartNotation, with up to 6 digits of precision. You can
    change the textual representation QTextStream will use for real
    numbers by calling setRealNumberNotation(),
    setRealNumberPrecision() and setNumberFlags().

    \sa setFieldWidth(), setRealNumberNotation(),
    setRealNumberPrecision(), setNumberFlags()
*/
QTextStream &QTextStream::operator<<(float f)
{
    return *this << double(f);
}

/*!
    \overload

    Writes the double \a f to the stream.
*/
QTextStream &QTextStream::operator<<(double f)
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(*this);

    QLocalePrivate::DoubleForm form = QLocalePrivate::DFDecimal;
    switch (realNumberNotation()) {
    case FixedNotation:
        form = QLocalePrivate::DFDecimal;
        break;
    case ScientificNotation:
        form = QLocalePrivate::DFExponent;
        break;
    case SmartNotation:
        form = QLocalePrivate::DFSignificantDigits;
        break;
    }

    uint flags = 0;
    if (numberFlags() & ShowBase)
        flags |= QLocalePrivate::ShowBase;
    if (numberFlags() & ForceSign)
        flags |= QLocalePrivate::AlwaysShowSign;
    if (numberFlags() & UppercaseBase)
        flags |= QLocalePrivate::UppercaseBase;
    if (numberFlags() & UppercaseDigits)
        flags |= QLocalePrivate::CapitalEorX;
    if (numberFlags() & ForcePoint)
        flags |= QLocalePrivate::Alternate;

    const QLocalePrivate *dd = d->locale.d();
    QString num = dd->doubleToString(f, d->realNumberPrecision, form, -1, flags);
    d->putString(num, true);
    return *this;
}

/*!
    Writes the string \a string to the stream, and returns a reference
    to the QTextStream. The string is first encoded using the assigned
    codec (the default codec is QTextCodec::codecForLocale()) before
    it is written to the stream.

    \sa setFieldWidth(), setCodec()
*/
QTextStream &QTextStream::operator<<(const QString &string)
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(*this);
    d->putString(string);
    return *this;
}

/*!
    \overload

    Writes \a array to the stream. The contents of \a array are
    converted with QString::fromAscii().
*/
QTextStream &QTextStream::operator<<(const QByteArray &array)
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(*this);
    d->putString(QString::fromAscii(array.constData(), array.length()));
    return *this;
}

/*!
    \overload

    Writes the constant string pointed to by \a string to the stream. \a
    string is assumed to be in ISO-8859-1 encoding. This operator
    is convenient when working with constant string data. Example:

    \snippet doc/src/snippets/code/src_corelib_io_qtextstream.cpp 8

    Warning: QTextStream assumes that \a string points to a string of
    text, terminated by a '\0' character. If there is no terminating
    '\0' character, your application may crash.
*/
QTextStream &QTextStream::operator<<(const char *string)
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(*this);
    d->putString(QLatin1String(string));
    return *this;
}

/*!
    \overload

    Writes \a ptr to the stream as a hexadecimal number with a base.
*/

QTextStream &QTextStream::operator<<(const void *ptr)
{
    Q_D(QTextStream);
    CHECK_VALID_STREAM(*this);
    int oldBase = d->integerBase;
    NumberFlags oldFlags = d->numberFlags;
    d->integerBase = 16;
    d->numberFlags |= ShowBase;
    d->putNumber(reinterpret_cast<quintptr>(ptr), false);
    d->integerBase = oldBase;
    d->numberFlags = oldFlags;
    return *this;
}

/*!
    \relates QTextStream

    Calls QTextStream::setIntegerBase(2) on \a stream and returns \a
    stream.

    \sa oct(), dec(), hex(), {QTextStream manipulators}
*/
QTextStream &bin(QTextStream &stream)
{
    stream.setIntegerBase(2);
    return stream;
}

/*!
    \relates QTextStream

    Calls QTextStream::setIntegerBase(8) on \a stream and returns \a
    stream.

    \sa bin(), dec(), hex(), {QTextStream manipulators}
*/
QTextStream &oct(QTextStream &stream)
{
    stream.setIntegerBase(8);
    return stream;
}

/*!
    \relates QTextStream

    Calls QTextStream::setIntegerBase(10) on \a stream and returns \a
    stream.

    \sa bin(), oct(), hex(), {QTextStream manipulators}
*/
QTextStream &dec(QTextStream &stream)
{
    stream.setIntegerBase(10);
    return stream;
}

/*!
    \relates QTextStream

    Calls QTextStream::setIntegerBase(16) on \a stream and returns \a
    stream.

    \note The hex modifier can only be used for writing to streams.
    \sa bin(), oct(), dec(), {QTextStream manipulators}
*/
QTextStream &hex(QTextStream &stream)
{
    stream.setIntegerBase(16);
    return stream;
}

/*!
    \relates QTextStream

    Calls QTextStream::setNumberFlags(QTextStream::numberFlags() |
    QTextStream::ShowBase) on \a stream and returns \a stream.

    \sa noshowbase(), forcesign(), forcepoint(), {QTextStream manipulators}
*/
QTextStream &showbase(QTextStream &stream)
{
    stream.setNumberFlags(stream.numberFlags() | QTextStream::ShowBase);
    return stream;
}

/*!
    \relates QTextStream

    Calls QTextStream::setNumberFlags(QTextStream::numberFlags() |
    QTextStream::ForceSign) on \a stream and returns \a stream.

    \sa noforcesign(), forcepoint(), showbase(), {QTextStream manipulators}
*/
QTextStream &forcesign(QTextStream &stream)
{
    stream.setNumberFlags(stream.numberFlags() | QTextStream::ForceSign);
    return stream;
}

/*!
    \relates QTextStream

    Calls QTextStream::setNumberFlags(QTextStream::numberFlags() |
    QTextStream::ForcePoint) on \a stream and returns \a stream.

    \sa noforcepoint(), forcesign(), showbase(), {QTextStream manipulators}
*/
QTextStream &forcepoint(QTextStream &stream)
{
    stream.setNumberFlags(stream.numberFlags() | QTextStream::ForcePoint);
    return stream;
}

/*!
    \relates QTextStream

    Calls QTextStream::setNumberFlags(QTextStream::numberFlags() &
    ~QTextStream::ShowBase) on \a stream and returns \a stream.

    \sa showbase(), noforcesign(), noforcepoint(), {QTextStream manipulators}
*/
QTextStream &noshowbase(QTextStream &stream)
{
    stream.setNumberFlags(stream.numberFlags() &= ~QTextStream::ShowBase);
    return stream;
}

/*!
    \relates QTextStream

    Calls QTextStream::setNumberFlags(QTextStream::numberFlags() &
    ~QTextStream::ForceSign) on \a stream and returns \a stream.

    \sa forcesign(), noforcepoint(), noshowbase(), {QTextStream manipulators}
*/
QTextStream &noforcesign(QTextStream &stream)
{
    stream.setNumberFlags(stream.numberFlags() &= ~QTextStream::ForceSign);
    return stream;
}

/*!
    \relates QTextStream

    Calls QTextStream::setNumberFlags(QTextStream::numberFlags() &
    ~QTextStream::ForcePoint) on \a stream and returns \a stream.

    \sa forcepoint(), noforcesign(), noshowbase(), {QTextStream manipulators}
*/
QTextStream &noforcepoint(QTextStream &stream)
{
    stream.setNumberFlags(stream.numberFlags() &= ~QTextStream::ForcePoint);
    return stream;
}

/*!
    \relates QTextStream

    Calls QTextStream::setNumberFlags(QTextStream::numberFlags() |
    QTextStream::UppercaseBase) on \a stream and returns \a stream.

    \sa lowercasebase(), uppercasedigits(), {QTextStream manipulators}
*/
QTextStream &uppercasebase(QTextStream &stream)
{
    stream.setNumberFlags(stream.numberFlags() | QTextStream::UppercaseBase);
    return stream;
}

/*!
    \relates QTextStream

    Calls QTextStream::setNumberFlags(QTextStream::numberFlags() |
    QTextStream::UppercaseDigits) on \a stream and returns \a stream.

    \sa lowercasedigits(), uppercasebase(), {QTextStream manipulators}
*/
QTextStream &uppercasedigits(QTextStream &stream)
{
    stream.setNumberFlags(stream.numberFlags() | QTextStream::UppercaseDigits);
    return stream;
}

/*!
    \relates QTextStream

    Calls QTextStream::setNumberFlags(QTextStream::numberFlags() &
    ~QTextStream::UppercaseBase) on \a stream and returns \a stream.

    \sa uppercasebase(), lowercasedigits(), {QTextStream manipulators}
*/
QTextStream &lowercasebase(QTextStream &stream)
{
    stream.setNumberFlags(stream.numberFlags() & ~QTextStream::UppercaseBase);
    return stream;
}

/*!
    \relates QTextStream

    Calls QTextStream::setNumberFlags(QTextStream::numberFlags() &
    ~QTextStream::UppercaseDigits) on \a stream and returns \a stream.

    \sa uppercasedigits(), lowercasebase(), {QTextStream manipulators}
*/
QTextStream &lowercasedigits(QTextStream &stream)
{
    stream.setNumberFlags(stream.numberFlags() & ~QTextStream::UppercaseDigits);
    return stream;
}

/*!
    \relates QTextStream

    Calls QTextStream::setRealNumberNotation(QTextStream::FixedNotation)
    on \a stream and returns \a stream.

    \sa scientific(), {QTextStream manipulators}
*/
QTextStream &fixed(QTextStream &stream)
{
    stream.setRealNumberNotation(QTextStream::FixedNotation);
    return stream;
}

/*!
    \relates QTextStream

    Calls QTextStream::setRealNumberNotation(QTextStream::ScientificNotation)
    on \a stream and returns \a stream.

    \sa fixed(), {QTextStream manipulators}
*/
QTextStream &scientific(QTextStream &stream)
{
    stream.setRealNumberNotation(QTextStream::ScientificNotation);
    return stream;
}

/*!
    \relates QTextStream

    Calls QTextStream::setFieldAlignment(QTextStream::AlignLeft)
    on \a stream and returns \a stream.

    \sa right(), center(), {QTextStream manipulators}
*/
QTextStream &left(QTextStream &stream)
{
    stream.setFieldAlignment(QTextStream::AlignLeft);
    return stream;
}

/*!
    \relates QTextStream

    Calls QTextStream::setFieldAlignment(QTextStream::AlignRight)
    on \a stream and returns \a stream.

    \sa left(), center(), {QTextStream manipulators}
*/
QTextStream &right(QTextStream &stream)
{
    stream.setFieldAlignment(QTextStream::AlignRight);
    return stream;
}

/*!
    \relates QTextStream

    Calls QTextStream::setFieldAlignment(QTextStream::AlignCenter)
    on \a stream and returns \a stream.

    \sa left(), right(), {QTextStream manipulators}
*/
QTextStream &center(QTextStream &stream)
{
    stream.setFieldAlignment(QTextStream::AlignCenter);
    return stream;
}

/*!
    \relates QTextStream

    Writes '\n' to the \a stream and flushes the stream.

    Equivalent to

    \snippet doc/src/snippets/code/src_corelib_io_qtextstream.cpp 9

    Note: On Windows, all '\n' characters are written as '\r\n' if
    QTextStream's device or string is opened using the QIODevice::Text flag.

    \sa flush(), reset(), {QTextStream manipulators}
*/
QTextStream &endl(QTextStream &stream)
{
    return stream << QLatin1Char('\n') << flush;
}

/*!
    \relates QTextStream

    Calls QTextStream::flush() on \a stream and returns \a stream.

    \sa endl(), reset(), {QTextStream manipulators}
*/
QTextStream &flush(QTextStream &stream)
{
    stream.flush();
    return stream;
}

/*!
    \relates QTextStream

    Calls QTextStream::reset() on \a stream and returns \a stream.

    \sa flush(), {QTextStream manipulators}
*/
QTextStream &reset(QTextStream &stream)
{
    stream.reset();
    return stream;
}

/*!
    \relates QTextStream

    Calls skipWhiteSpace() on \a stream and returns \a stream.

    \sa {QTextStream manipulators}
*/
QTextStream &ws(QTextStream &stream)
{
    stream.skipWhiteSpace();
    return stream;
}

/*!
    \fn QTextStreamManipulator qSetFieldWidth(int width)
    \relates QTextStream

    Equivalent to QTextStream::setFieldWidth(\a width).
*/

/*!
    \fn QTextStreamManipulator qSetPadChar(QChar ch)
    \relates QTextStream

    Equivalent to QTextStream::setPadChar(\a ch).
*/

/*!
    \fn QTextStreamManipulator qSetRealNumberPrecision(int precision)
    \relates QTextStream

    Equivalent to QTextStream::setRealNumberPrecision(\a precision).
*/

#ifndef QT_NO_TEXTCODEC
/*!
    \relates QTextStream

    Toggles insertion of the Byte Order Mark on \a stream when QTextStream is
    used with a UTF codec.

    \sa QTextStream::setGenerateByteOrderMark(), {QTextStream manipulators}
*/
QTextStream &bom(QTextStream &stream)
{
    stream.setGenerateByteOrderMark(true);
    return stream;
}

/*!
    Sets the codec for this stream to \a codec. The codec is used for
    decoding any data that is read from the assigned device, and for
    encoding any data that is written. By default,
    QTextCodec::codecForLocale() is used, and automatic unicode
    detection is enabled.

    If QTextStream operates on a string, this function does nothing.

    \warning If you call this function while the text stream is reading
    from an open sequential socket, the internal buffer may still contain
    text decoded using the old codec.

    \sa codec(), setAutoDetectUnicode(), setLocale()
*/
void QTextStream::setCodec(QTextCodec *codec)
{
    Q_D(QTextStream);
    qint64 seekPos = -1;
    if (!d->readBuffer.isEmpty()) {
        if (!d->device->isSequential()) {
            seekPos = pos();
        }
    }
    d->codec = codec;
    if (seekPos >=0 && !d->readBuffer.isEmpty())
        seek(seekPos);
}

/*!
    Sets the codec for this stream to the QTextCodec for the encoding
    specified by \a codecName. Common values for \c codecName include
    "ISO 8859-1", "UTF-8", and "UTF-16". If the encoding isn't
    recognized, nothing happens.

    Example:

    \snippet doc/src/snippets/code/src_corelib_io_qtextstream.cpp 10

    \sa QTextCodec::codecForName(), setLocale()
*/
void QTextStream::setCodec(const char *codecName)
{
    QTextCodec *codec = QTextCodec::codecForName(codecName);
    if (codec)
        setCodec(codec);
}

/*!
    Returns the codec that is current assigned to the stream.

    \sa setCodec(), setAutoDetectUnicode(), locale()
*/
QTextCodec *QTextStream::codec() const
{
    Q_D(const QTextStream);
    return d->codec;
}

/*!
    If \a enabled is true, QTextStream will attempt to detect Unicode
    encoding by peeking into the stream data to see if it can find the
    UTF-16 or UTF-32 BOM (Byte Order Mark). If this mark is found, QTextStream
    will replace the current codec with the UTF codec.

    This function can be used together with setCodec(). It is common
    to set the codec to UTF-8, and then enable UTF-16 detection.

    \sa autoDetectUnicode(), setCodec()
*/
void QTextStream::setAutoDetectUnicode(bool enabled)
{
    Q_D(QTextStream);
    d->autoDetectUnicode = enabled;
}

/*!
    Returns true if automatic Unicode detection is enabled, otherwise
    returns false. Automatic Unicode detection is enabled by default.

    \sa setAutoDetectUnicode(), setCodec()
*/
bool QTextStream::autoDetectUnicode() const
{
    Q_D(const QTextStream);
    return d->autoDetectUnicode;
}

/*!
    If \a generate is true and a UTF codec is used, QTextStream will insert
    the BOM (Byte Order Mark) before any data has been written to the
    device. If \a generate is false, no BOM will be inserted. This function
    must be called before any data is written. Otherwise, it does nothing.

    \sa generateByteOrderMark(), bom()
*/
void QTextStream::setGenerateByteOrderMark(bool generate)
{
    Q_D(QTextStream);
    if (d->writeBuffer.isEmpty()) {
        if (generate)
            d->writeConverterState.flags &= ~QTextCodec::IgnoreHeader;
        else
            d->writeConverterState.flags |= QTextCodec::IgnoreHeader;
    }
}

/*!
    Returns true if QTextStream is set to generate the UTF BOM (Byte Order
    Mark) when using a UTF codec; otherwise returns false. UTF BOM generation is
    set to false by default.

    \sa setGenerateByteOrderMark()
*/
bool QTextStream::generateByteOrderMark() const
{
    Q_D(const QTextStream);
    return (d->writeConverterState.flags & QTextCodec::IgnoreHeader) == 0;
}

#endif

/*!
    \since 4.5

    Sets the locale for this stream to \a locale. The specified locale is
    used for conversions between numbers and their string representations.

    The default locale is C and it is a special case - the thousands
    group separator is not used for backward compatibility reasons.

    \sa locale()
*/
void QTextStream::setLocale(const QLocale &locale)
{
    Q_D(QTextStream);
    d->locale = locale;
}

/*!
    \since 4.5

    Returns the locale for this stream. The default locale is C.

    \sa setLocale()
*/
QLocale QTextStream::locale() const
{
    Q_D(const QTextStream);
    return d->locale;
}

#ifdef QT3_SUPPORT
/*!
    \class QTextIStream
    \brief The QTextIStream class is a convenience class for input streams.

    \compat
    \reentrant

    Use QTextStream instead.
*/

/*!
    \fn QTextIStream::QTextIStream(const QString *string)

    Use QTextStream(&\a{string}, QIODevice::ReadOnly) instead.
*/
/*!
    \fn QTextIStream::QTextIStream(QByteArray *byteArray)

    Use QTextStream(&\a{byteArray}, QIODevice::ReadOnly) instead.
*/
/*!
    \fn QTextIStream::QTextIStream(FILE *file)

    Use QTextStream(\a{file}, QIODevice::ReadOnly) instead.
*/

/*!
    \class QTextOStream
    \brief The QTextOStream class is a convenience class for output streams.

    \compat
    \reentrant

    Use QTextStream instead.
*/

/*!
    \fn QTextOStream::QTextOStream(QString *string)

    Use QTextStream(&\a{string}, QIODevice::WriteOnly) instead.
*/
/*!
    \fn QTextOStream::QTextOStream(QByteArray *byteArray)

    Use QTextStream(&\a{byteArray}, QIODevice::WriteOnly) instead.
*/
/*!
    \fn QTextOStream::QTextOStream(FILE *file)

    Use QTextStream(\a{file}, QIODevice::WriteOnly) instead.
*/

/*! \internal
*/
int QTextStream::flagsInternal() const
{
    Q_D(const QTextStream);

    int f = 0;
    switch (d->fieldAlignment) {
    case AlignLeft: f |= left; break;
    case AlignRight: f |= right; break;
    case AlignCenter: f |= internal; break;
    default:
        break;
    }
    switch (d->integerBase) {
    case 2: f |= bin; break;
    case 8: f |= oct; break;
    case 10: f |= dec; break;
    case 16: f |= hex; break;
    default:
        break;
    }
    switch (d->realNumberNotation) {
    case FixedNotation: f |= fixed; break;
    case ScientificNotation: f |= scientific; break;
    default:
        break;
    }
    if (d->numberFlags & ShowBase)
        f |= showbase;
    if (d->numberFlags & ForcePoint)
        f |= showpoint;
    if (d->numberFlags & ForceSign)
        f |= showpos;
    if (d->numberFlags & UppercaseBase)
        f |= uppercase;
    return f;
}

/*! \internal
*/
int QTextStream::flagsInternal(int newFlags)
{
    int oldFlags = flagsInternal();

    if (newFlags & left)
        setFieldAlignment(AlignLeft);
    else if (newFlags & right)
        setFieldAlignment(AlignRight);
    else if (newFlags & internal)
        setFieldAlignment(AlignCenter);

    if (newFlags & bin)
        setIntegerBase(2);
    else if (newFlags & oct)
        setIntegerBase(8);
    else if (newFlags & dec)
        setIntegerBase(10);
    else if (newFlags & hex)
        setIntegerBase(16);

    if (newFlags & showbase)
        setNumberFlags(numberFlags() | ShowBase);
    if (newFlags & showpos)
        setNumberFlags(numberFlags() | ForceSign);
    if (newFlags & showpoint)
        setNumberFlags(numberFlags() | ForcePoint);
    if (newFlags & uppercase)
        setNumberFlags(numberFlags() | UppercaseBase);

    if (newFlags & fixed)
        setRealNumberNotation(FixedNotation);
    else if (newFlags & scientific)
        setRealNumberNotation(ScientificNotation);

    return oldFlags;
}

#ifndef QT_NO_TEXTCODEC
/*!
    Use setCodec() and setAutoDetectUnicode() instead.
*/
void QTextStream::setEncoding(Encoding encoding)
{
    Q_D(QTextStream);
    resetCodecConverterStateHelper(&d->readConverterState);
    resetCodecConverterStateHelper(&d->writeConverterState);

    switch (encoding) {
    case Locale:
        d->writeConverterState.flags |= QTextCodec::IgnoreHeader;
        setCodec(QTextCodec::codecForLocale());
        d->autoDetectUnicode = true;
        break;
    case Latin1:
        d->readConverterState.flags |= QTextCodec::IgnoreHeader;
        d->writeConverterState.flags |= QTextCodec::IgnoreHeader;
        setCodec(QTextCodec::codecForName("ISO-8859-1"));
        d->autoDetectUnicode = false;
        break;
    case Unicode:
        setCodec(QTextCodec::codecForName("UTF-16"));
        d->autoDetectUnicode = false;
        break;
    case RawUnicode:
        d->readConverterState.flags |= QTextCodec::IgnoreHeader;
        d->writeConverterState.flags |= QTextCodec::IgnoreHeader;
        setCodec(QTextCodec::codecForName("UTF-16"));
        d->autoDetectUnicode = false;
        break;
    case UnicodeNetworkOrder:
        d->readConverterState.flags |= QTextCodec::IgnoreHeader;
        d->writeConverterState.flags |= QTextCodec::IgnoreHeader;
        setCodec(QTextCodec::codecForName("UTF-16BE"));
        d->autoDetectUnicode = false;
        break;
    case UnicodeReverse:
        d->readConverterState.flags |= QTextCodec::IgnoreHeader;
        d->writeConverterState.flags |= QTextCodec::IgnoreHeader;
        setCodec(QTextCodec::codecForName("UTF-16LE"));
        d->autoDetectUnicode = false;
        break;
    case UnicodeUTF8:
        d->writeConverterState.flags |= QTextCodec::IgnoreHeader;
        setCodec(QTextCodec::codecForName("UTF-8"));
        d->autoDetectUnicode = true;
        break;
    }
}
#endif

/*!
    \enum QTextStream::Encoding
    \compat

    \value Latin1  Use setCodec(QTextCodec::codecForName("ISO-8859-1")) instead.
    \value Locale  Use setCodec(QTextCodec::codecForLocale()) instead.
    \value RawUnicode  Use setCodec(QTextCodec::codecForName("UTF-16")) instead.
    \value Unicode  Use setCodec(QTextCodec::codecForName("UTF-16")) instead.
    \value UnicodeNetworkOrder  Use setCodec(QTextCodec::codecForName("UTF-16BE")) instead.
    \value UnicodeReverse  Use setCodec(QTextCodec::codecForName("UTF-16LE")) instead.
    \value UnicodeUTF8  Use setCodec(QTextCodec::codecForName("UTF-8")) instead.

    Also, for all encodings except QTextStream::Latin1 and
    QTextStream::UTF8, you need to call setAutoDetectUnicode(false)
    to obtain the Qt 3 behavior in addition to the setCodec() call.

    \sa setCodec(), setAutoDetectUnicode()
*/

/*!
    \fn int QTextStream::flags() const

    Use fieldAlignment(), padChar(), fieldWidth(), numberFlags(),
    integerBase(), realNumberNotation(), and realNumberNotation
    instead.
*/

/*!
    \fn int QTextStream::flags(int)

    Use setFieldAlignment(), setPadChar(), setFieldWidth(),
    setNumberFlags(), setIntegerBase(), setRealNumberNotation(), and
    setRealNumberNotation instead.
*/

/*!
    \fn int QTextStream::setf(int)

    Use setFieldAlignment(), setPadChar(), setFieldWidth(),
    setNumberFlags(), setIntegerBase(), setRealNumberNotation(), and
    setRealNumberNotation instead.
*/

/*!
    \fn int QTextStream::setf(int, int)

    Use setFieldAlignment(), setPadChar(), setFieldWidth(),
    setNumberFlags(), setIntegerBase(), setRealNumberNotation(), and
    setRealNumberNotation instead.
*/

/*!
    \fn int QTextStream::unsetf(int)

    Use setFieldAlignment(), setPadChar(), setFieldWidth(),
    setNumberFlags(), setIntegerBase(), setRealNumberNotation(), and
    setRealNumberNotation instead.
*/

/*!
    \fn int QTextStream::width(int)

    Use setFieldWidth() instead.
*/

/*!
    \fn int QTextStream::fill(int)

    Use setPadChar() instead.
*/

/*!
    \fn int QTextStream::precision(int)

    Use setRealNumberPrecision() instead.
*/

/*!
    \fn int QTextStream::read()

    Use readAll() or readLine() instead.
*/

/*!
    \fn int QTextStream::unsetDevice()

    Use setDevice(0) instead.
*/

/*!
    \variable QTextStream::skipws
    \variable QTextStream::left
    \variable QTextStream::right
    \variable QTextStream::internal
    \variable QTextStream::bin
    \variable QTextStream::oct
    \variable QTextStream::dec
    \variable QTextStream::hex
    \variable QTextStream::showbase
    \variable QTextStream::showpoint
    \variable QTextStream::uppercase
    \variable QTextStream::showpos
    \variable QTextStream::scientific
    \variable QTextStream::fixed
    \variable QTextStream::basefield
    \variable QTextStream::adjustfield
    \variable QTextStream::floatfield
    \compat

    Use the new \l{QTextStream manipulators} instead.
*/

#endif

QT_END_NAMESPACE

#ifndef QT_NO_QOBJECT
#include "qtextstream.moc"
#endif

