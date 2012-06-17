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

#ifndef QTEXTSTREAM_H
#define QTEXTSTREAM_H

#include <QtCore/qiodevice.h>
#include <QtCore/qstring.h>
#include <QtCore/qchar.h>
#include <QtCore/qlocale.h>
#include <QtCore/qscopedpointer.h>

#ifndef QT_NO_TEXTCODEC
#  ifdef QT3_SUPPORT
#    include <QtCore/qtextcodec.h>
#  endif
#endif

#include <stdio.h>

#ifdef Status
#error qtextstream.h must be included before any header file that defines Status
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

class QTextCodec;
class QTextDecoder;

class QTextStreamPrivate;
class Q_CORE_EXPORT QTextStream                                // text stream class
{
    Q_DECLARE_PRIVATE(QTextStream)

public:
    enum RealNumberNotation {
        SmartNotation,
        FixedNotation,
        ScientificNotation
    };
    enum FieldAlignment {
        AlignLeft,
        AlignRight,
        AlignCenter,
        AlignAccountingStyle
    };
    enum Status {
        Ok,
        ReadPastEnd,
        ReadCorruptData,
        WriteFailed
    };
    enum NumberFlag {
        ShowBase = 0x1,
        ForcePoint = 0x2,
        ForceSign = 0x4,
        UppercaseBase = 0x8,
        UppercaseDigits = 0x10
    };
    Q_DECLARE_FLAGS(NumberFlags, NumberFlag)

    QTextStream();
    explicit QTextStream(QIODevice *device);
    explicit QTextStream(FILE *fileHandle, QIODevice::OpenMode openMode = QIODevice::ReadWrite);
    explicit QTextStream(QString *string, QIODevice::OpenMode openMode = QIODevice::ReadWrite);
    explicit QTextStream(QByteArray *array, QIODevice::OpenMode openMode = QIODevice::ReadWrite);
    explicit QTextStream(const QByteArray &array, QIODevice::OpenMode openMode = QIODevice::ReadOnly);
    virtual ~QTextStream();

#ifndef QT_NO_TEXTCODEC
    void setCodec(QTextCodec *codec);
    void setCodec(const char *codecName);
    QTextCodec *codec() const;
    void setAutoDetectUnicode(bool enabled);
    bool autoDetectUnicode() const;
    void setGenerateByteOrderMark(bool generate);
    bool generateByteOrderMark() const;
#endif

    void setLocale(const QLocale &locale);
    QLocale locale() const;

    void setDevice(QIODevice *device);
    QIODevice *device() const;

    void setString(QString *string, QIODevice::OpenMode openMode = QIODevice::ReadWrite);
    QString *string() const;

    Status status() const;
    void setStatus(Status status);
    void resetStatus();

    bool atEnd() const;
    void reset();
    void flush();
    bool seek(qint64 pos);
    qint64 pos() const;

    void skipWhiteSpace();

    QString readLine(qint64 maxlen = 0);
    QString readAll();
    QString read(qint64 maxlen);

    void setFieldAlignment(FieldAlignment alignment);
    FieldAlignment fieldAlignment() const;

    void setPadChar(QChar ch);
    QChar padChar() const;

    void setFieldWidth(int width);
    int fieldWidth() const;

    void setNumberFlags(NumberFlags flags);
    NumberFlags numberFlags() const;

    void setIntegerBase(int base);
    int integerBase() const;

    void setRealNumberNotation(RealNumberNotation notation);
    RealNumberNotation realNumberNotation() const;

    void setRealNumberPrecision(int precision);
    int realNumberPrecision() const;

    QTextStream &operator>>(QChar &ch);
    QTextStream &operator>>(char &ch);
    QTextStream &operator>>(signed short &i);
    QTextStream &operator>>(unsigned short &i);
    QTextStream &operator>>(signed int &i);
    QTextStream &operator>>(unsigned int &i);
    QTextStream &operator>>(signed long &i);
    QTextStream &operator>>(unsigned long &i);
    QTextStream &operator>>(qlonglong &i);
    QTextStream &operator>>(qulonglong &i);
    QTextStream &operator>>(float &f);
    QTextStream &operator>>(double &f);
    QTextStream &operator>>(QString &s);
    QTextStream &operator>>(QByteArray &array);
    QTextStream &operator>>(char *c);

    QTextStream &operator<<(QBool b);
    QTextStream &operator<<(QChar ch);
    QTextStream &operator<<(char ch);
    QTextStream &operator<<(signed short i);
    QTextStream &operator<<(unsigned short i);
    QTextStream &operator<<(signed int i);
    QTextStream &operator<<(unsigned int i);
    QTextStream &operator<<(signed long i);
    QTextStream &operator<<(unsigned long i);
    QTextStream &operator<<(qlonglong i);
    QTextStream &operator<<(qulonglong i);
    QTextStream &operator<<(float f);
    QTextStream &operator<<(double f);
    QTextStream &operator<<(const QString &s);
    QTextStream &operator<<(const QByteArray &array);
    QTextStream &operator<<(const char *c);
    QTextStream &operator<<(const void *ptr);

#ifdef QT3_SUPPORT
    // not marked as QT3_SUPPORT to avoid double compiler warnings, as
    // they are used in the QT3_SUPPORT functions below.
    inline QT3_SUPPORT int flags() const { return flagsInternal(); }
    inline QT3_SUPPORT int flags(int f) { return flagsInternal(f); }

    inline QT3_SUPPORT int setf(int bits)
    { int old = flagsInternal(); flagsInternal(flagsInternal() | bits); return old; }
    inline QT3_SUPPORT int setf(int bits, int mask)
    { int old = flagsInternal(); flagsInternal(flagsInternal() | (bits & mask)); return old; }
    inline QT3_SUPPORT int unsetf(int bits)
    { int old = flagsInternal(); flagsInternal(flagsInternal() & ~bits); return old; }

    inline QT3_SUPPORT int width(int w)
    { int old = fieldWidth(); setFieldWidth(w); return old; }
    inline QT3_SUPPORT int fill(int f)
    { QChar ch = padChar(); setPadChar(QChar(f)); return ch.unicode(); }
    inline QT3_SUPPORT int precision(int p)
    { int old = realNumberPrecision(); setRealNumberPrecision(p); return old; }

    enum {
        skipws       = 0x0001,                        // skip whitespace on input
        left         = 0x0002,                        // left-adjust output
        right        = 0x0004,                        // right-adjust output
        internal     = 0x0008,                        // pad after sign
        bin          = 0x0010,                        // binary format integer
        oct          = 0x0020,                        // octal format integer
        dec          = 0x0040,                        // decimal format integer
        hex          = 0x0080,                        // hex format integer
        showbase     = 0x0100,                        // show base indicator
        showpoint    = 0x0200,                        // force decimal point (float)
        uppercase    = 0x0400,                        // upper-case hex output
        showpos      = 0x0800,                        // add '+' to positive integers
        scientific   = 0x1000,                        // scientific float output
        fixed        = 0x2000                         // fixed float output
    };
    enum {
        basefield = bin | oct | dec | hex,
        adjustfield = left | right | internal,
        floatfield = scientific | fixed
    };

#ifndef QT_NO_TEXTCODEC
    enum Encoding { Locale, Latin1, Unicode, UnicodeNetworkOrder,
                    UnicodeReverse, RawUnicode, UnicodeUTF8 };
    QT3_SUPPORT void setEncoding(Encoding encoding);
#endif
    inline QT3_SUPPORT QString read() { return readAll(); }
    inline QT3_SUPPORT void unsetDevice() { setDevice(0); }
#endif

private:
#ifdef QT3_SUPPORT
    int flagsInternal() const;
    int flagsInternal(int flags);
#endif

    Q_DISABLE_COPY(QTextStream)

    QScopedPointer<QTextStreamPrivate> d_ptr;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QTextStream::NumberFlags)

/*****************************************************************************
  QTextStream manipulators
 *****************************************************************************/

typedef QTextStream & (*QTextStreamFunction)(QTextStream &);// manipulator function
typedef void (QTextStream::*QTSMFI)(int); // manipulator w/int argument
typedef void (QTextStream::*QTSMFC)(QChar); // manipulator w/QChar argument

class Q_CORE_EXPORT QTextStreamManipulator
{
public:
    QTextStreamManipulator(QTSMFI m, int a) { mf = m; mc = 0; arg = a; }
    QTextStreamManipulator(QTSMFC m, QChar c) { mf = 0; mc = m; ch = c; arg = -1; }
    void exec(QTextStream &s) { if (mf) { (s.*mf)(arg); } else { (s.*mc)(ch); } }

private:
    QTSMFI mf;                                        // QTextStream member function
    QTSMFC mc;                                        // QTextStream member function
    int arg;                                          // member function argument
    QChar ch;
};

inline QTextStream &operator>>(QTextStream &s, QTextStreamFunction f)
{ return (*f)(s); }

inline QTextStream &operator<<(QTextStream &s, QTextStreamFunction f)
{ return (*f)(s); }

inline QTextStream &operator<<(QTextStream &s, QTextStreamManipulator m)
{ m.exec(s); return s; }

Q_CORE_EXPORT QTextStream &bin(QTextStream &s);
Q_CORE_EXPORT QTextStream &oct(QTextStream &s);
Q_CORE_EXPORT QTextStream &dec(QTextStream &s);
Q_CORE_EXPORT QTextStream &hex(QTextStream &s);

Q_CORE_EXPORT QTextStream &showbase(QTextStream &s);
Q_CORE_EXPORT QTextStream &forcesign(QTextStream &s);
Q_CORE_EXPORT QTextStream &forcepoint(QTextStream &s);
Q_CORE_EXPORT QTextStream &noshowbase(QTextStream &s);
Q_CORE_EXPORT QTextStream &noforcesign(QTextStream &s);
Q_CORE_EXPORT QTextStream &noforcepoint(QTextStream &s);

Q_CORE_EXPORT QTextStream &uppercasebase(QTextStream &s);
Q_CORE_EXPORT QTextStream &uppercasedigits(QTextStream &s);
Q_CORE_EXPORT QTextStream &lowercasebase(QTextStream &s);
Q_CORE_EXPORT QTextStream &lowercasedigits(QTextStream &s);

Q_CORE_EXPORT QTextStream &fixed(QTextStream &s);
Q_CORE_EXPORT QTextStream &scientific(QTextStream &s);

Q_CORE_EXPORT QTextStream &left(QTextStream &s);
Q_CORE_EXPORT QTextStream &right(QTextStream &s);
Q_CORE_EXPORT QTextStream &center(QTextStream &s);

Q_CORE_EXPORT QTextStream &endl(QTextStream &s);
Q_CORE_EXPORT QTextStream &flush(QTextStream &s);
Q_CORE_EXPORT QTextStream &reset(QTextStream &s);

Q_CORE_EXPORT QTextStream &bom(QTextStream &s);

Q_CORE_EXPORT QTextStream &ws(QTextStream &s);

inline QTextStreamManipulator qSetFieldWidth(int width)
{
    QTSMFI func = &QTextStream::setFieldWidth;
    return QTextStreamManipulator(func,width);
}

inline QTextStreamManipulator qSetPadChar(QChar ch)
{
    QTSMFC func = &QTextStream::setPadChar;
    return QTextStreamManipulator(func, ch);
}

inline QTextStreamManipulator qSetRealNumberPrecision(int precision)
{
    QTSMFI func = &QTextStream::setRealNumberPrecision;
    return QTextStreamManipulator(func, precision);
}

#ifdef QT3_SUPPORT
typedef QTextStream QTS;

class Q_CORE_EXPORT QTextIStream : public QTextStream
{
public:
    inline explicit QTextIStream(const QString *s) : QTextStream(const_cast<QString *>(s), QIODevice::ReadOnly) {}
    inline explicit QTextIStream(QByteArray *a) : QTextStream(a, QIODevice::ReadOnly) {}
    inline QTextIStream(FILE *f) : QTextStream(f, QIODevice::ReadOnly) {}

private:
    Q_DISABLE_COPY(QTextIStream)
};

class Q_CORE_EXPORT QTextOStream : public QTextStream
{
public:
    inline explicit QTextOStream(QString *s) : QTextStream(s, QIODevice::WriteOnly) {}
    inline explicit QTextOStream(QByteArray *a) : QTextStream(a, QIODevice::WriteOnly) {}
    inline QTextOStream(FILE *f) : QTextStream(f, QIODevice::WriteOnly) {}

private:
    Q_DISABLE_COPY(QTextOStream)
};
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QTEXTSTREAM_H
