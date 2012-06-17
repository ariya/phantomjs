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

#ifndef QIODEVICE_H
#define QIODEVICE_H

#ifndef QT_NO_QOBJECT
#include <QtCore/qobject.h>
#else
#include <QtCore/qobjectdefs.h>
#include <QtCore/qscopedpointer.h>
#endif
#include <QtCore/qstring.h>

#ifdef open
#error qiodevice.h must be included before any header file that defines open
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

class QByteArray;
class QIODevicePrivate;

class Q_CORE_EXPORT QIODevice
#ifndef QT_NO_QOBJECT
    : public QObject
#endif
{
#ifndef QT_NO_QOBJECT
    Q_OBJECT
#endif
public:
    enum OpenModeFlag {
        NotOpen = 0x0000,
        ReadOnly = 0x0001,
        WriteOnly = 0x0002,
        ReadWrite = ReadOnly | WriteOnly,
        Append = 0x0004,
        Truncate = 0x0008,
        Text = 0x0010,
        Unbuffered = 0x0020
    };
    Q_DECLARE_FLAGS(OpenMode, OpenModeFlag)

    QIODevice();
#ifndef QT_NO_QOBJECT
    explicit QIODevice(QObject *parent);
#endif
    virtual ~QIODevice();

    OpenMode openMode() const;

    void setTextModeEnabled(bool enabled);
    bool isTextModeEnabled() const;

    bool isOpen() const;
    bool isReadable() const;
    bool isWritable() const;
    virtual bool isSequential() const;

    virtual bool open(OpenMode mode);
    virtual void close();

    // ### Qt 5: pos() and seek() should not be virtual, and
    // ### seek() should call a virtual seekData() function.
    virtual qint64 pos() const;
    virtual qint64 size() const;
    virtual bool seek(qint64 pos);
    virtual bool atEnd() const;
    virtual bool reset();

    virtual qint64 bytesAvailable() const;
    virtual qint64 bytesToWrite() const;

    qint64 read(char *data, qint64 maxlen);
    QByteArray read(qint64 maxlen);
    QByteArray readAll();
    qint64 readLine(char *data, qint64 maxlen);
    QByteArray readLine(qint64 maxlen = 0);
    virtual bool canReadLine() const;

    qint64 write(const char *data, qint64 len);
    qint64 write(const char *data);
    inline qint64 write(const QByteArray &data)
    { return write(data.constData(), data.size()); }

    qint64 peek(char *data, qint64 maxlen);
    QByteArray peek(qint64 maxlen);

    virtual bool waitForReadyRead(int msecs);
    virtual bool waitForBytesWritten(int msecs);

    void ungetChar(char c);
    bool putChar(char c);
    bool getChar(char *c);

    QString errorString() const;

#ifndef QT_NO_QOBJECT
Q_SIGNALS:
    void readyRead();
    void bytesWritten(qint64 bytes);
    void aboutToClose();
    void readChannelFinished();
#endif

protected:
#ifdef QT_NO_QOBJECT
    QIODevice(QIODevicePrivate &dd);
#else
    QIODevice(QIODevicePrivate &dd, QObject *parent = 0);
#endif
    virtual qint64 readData(char *data, qint64 maxlen) = 0;
    virtual qint64 readLineData(char *data, qint64 maxlen);
    virtual qint64 writeData(const char *data, qint64 len) = 0;

    void setOpenMode(OpenMode openMode);

    void setErrorString(const QString &errorString);

#ifdef QT_NO_QOBJECT
    QScopedPointer<QIODevicePrivate> d_ptr;
#endif

private:
    Q_DECLARE_PRIVATE(QIODevice)
    Q_DISABLE_COPY(QIODevice)

#ifdef QT3_SUPPORT
public:
    typedef qint64 Offset;

    inline QT3_SUPPORT int flags() const { return static_cast<int>(openMode()); }
    inline QT3_SUPPORT int mode() const { return static_cast<int>(openMode()); }
    inline QT3_SUPPORT int state() const;

    inline QT3_SUPPORT bool isDirectAccess() const { return !isSequential(); }
    inline QT3_SUPPORT bool isSequentialAccess() const { return isSequential(); }
    inline QT3_SUPPORT bool isCombinedAccess() const { return false; }
    inline QT3_SUPPORT bool isBuffered() const { return true; }
    inline QT3_SUPPORT bool isRaw() const { return false; }
    inline QT3_SUPPORT bool isSynchronous() const { return true; }
    inline QT3_SUPPORT bool isAsynchronous() const { return false; }
    inline QT3_SUPPORT bool isTranslated() const { return (openMode() & Text) != 0; }
    inline QT3_SUPPORT bool isInactive() const { return !isOpen(); }

    typedef int Status;
    QT3_SUPPORT Status status() const;
    QT3_SUPPORT void resetStatus();

    inline QT3_SUPPORT Offset at() const { return pos(); }
    inline QT3_SUPPORT bool at(Offset offset) { return seek(offset); }

    inline QT3_SUPPORT qint64 readBlock(char *data, quint64 maxlen) { return read(data, maxlen); }
    inline QT3_SUPPORT qint64 writeBlock(const char *data, quint64 len) { return write(data, len); }
    inline QT3_SUPPORT qint64 writeBlock(const QByteArray &data) { return write(data); }

    inline QT3_SUPPORT int getch() { char c; return getChar(&c) ? int(uchar(c)) : -1; }
    inline QT3_SUPPORT int putch(int c) { return putChar(char(c)) ? int(uchar(c)) : -1; }
    inline QT3_SUPPORT int ungetch(int c) { ungetChar(uchar(c)); return c; }
#endif
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QIODevice::OpenMode)

#ifdef QT3_SUPPORT
static QT3_SUPPORT_VARIABLE const uint IO_Direct = 0x0100;
static QT3_SUPPORT_VARIABLE const uint IO_Sequential = 0x0200;
static QT3_SUPPORT_VARIABLE const uint IO_Combined = 0x0300;
static QT3_SUPPORT_VARIABLE const uint IO_TypeMask = 0x0300;

static QT3_SUPPORT_VARIABLE const uint IO_Raw = 0x0000;
static QT3_SUPPORT_VARIABLE const uint IO_Async = 0x0000;

#define IO_ReadOnly QIODevice::ReadOnly
#define IO_WriteOnly QIODevice::WriteOnly
#define IO_ReadWrite QIODevice::ReadWrite
#define IO_Append QIODevice::Append
#define IO_Truncate QIODevice::Truncate
#define IO_Translate QIODevice::Text
#define IO_ModeMask 0x00ff

static QT3_SUPPORT_VARIABLE const uint IO_Open = 0x1000;
static QT3_SUPPORT_VARIABLE const uint IO_StateMask = 0xf000;

static QT3_SUPPORT_VARIABLE const uint IO_Ok = 0;
static QT3_SUPPORT_VARIABLE const uint IO_ReadError = 1;
static QT3_SUPPORT_VARIABLE const uint IO_WriteError = 2;
static QT3_SUPPORT_VARIABLE const uint IO_FatalError = 3;
static QT3_SUPPORT_VARIABLE const uint IO_ResourceError = 4;
static QT3_SUPPORT_VARIABLE const uint IO_OpenError = 5;
static QT3_SUPPORT_VARIABLE const uint IO_ConnectError = 5;
static QT3_SUPPORT_VARIABLE const uint IO_AbortError = 6;
static QT3_SUPPORT_VARIABLE const uint IO_TimeOutError = 7;
static QT3_SUPPORT_VARIABLE const uint IO_UnspecifiedError	= 8;

inline QT3_SUPPORT int QIODevice::state() const
{
    return isOpen() ? 0x1000 : 0;
}
#endif

#if !defined(QT_NO_DEBUG_STREAM)
class QDebug;
Q_CORE_EXPORT QDebug operator<<(QDebug debug, QIODevice::OpenMode modes);
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QIODEVICE_H
