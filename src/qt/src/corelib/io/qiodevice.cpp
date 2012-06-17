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

//#define QIODEVICE_DEBUG

#include "qbytearray.h"
#include "qdebug.h"
#include "qiodevice_p.h"
#include "qfile.h"
#include "qstringlist.h"
#include <limits.h>

#ifdef QIODEVICE_DEBUG
#  include <ctype.h>
#endif

QT_BEGIN_NAMESPACE

#ifdef QIODEVICE_DEBUG
void debugBinaryString(const QByteArray &input)
{
    QByteArray tmp;
    int startOffset = 0;
    for (int i = 0; i < input.size(); ++i) {
        tmp += input[i];

        if ((i % 16) == 15 || i == (input.size() - 1)) {
            printf("\n%15d:", startOffset);
            startOffset += tmp.size();

            for (int j = 0; j < tmp.size(); ++j)
                printf(" %02x", int(uchar(tmp[j])));
            for (int j = tmp.size(); j < 16 + 1; ++j)
                printf("   ");
            for (int j = 0; j < tmp.size(); ++j)
                printf("%c", isprint(int(uchar(tmp[j]))) ? tmp[j] : '.');
            tmp.clear();
        }
    }
    printf("\n\n");
}

void debugBinaryString(const char *data, qint64 maxlen)
{
    debugBinaryString(QByteArray(data, maxlen));
}
#endif

#define Q_VOID

#define CHECK_MAXLEN(function, returnType) \
    do { \
        if (maxSize < 0) { \
            qWarning("QIODevice::"#function": Called with maxSize < 0"); \
            return returnType; \
        } \
    } while (0)

#define CHECK_WRITABLE(function, returnType) \
   do { \
       if ((d->openMode & WriteOnly) == 0) { \
           if (d->openMode == NotOpen) \
               return returnType; \
           qWarning("QIODevice::"#function": ReadOnly device"); \
           return returnType; \
       } \
   } while (0)

#define CHECK_READABLE(function, returnType) \
   do { \
       if ((d->openMode & ReadOnly) == 0) { \
           if (d->openMode == NotOpen) \
               return returnType; \
           qWarning("QIODevice::"#function": WriteOnly device"); \
           return returnType; \
       } \
   } while (0)

/*! \internal
 */
QIODevicePrivate::QIODevicePrivate()
    : openMode(QIODevice::NotOpen), buffer(QIODEVICE_BUFFERSIZE),
      pos(0), devicePos(0), seqDumpPos(0)
       , pPos(&pos), pDevicePos(&devicePos)
       , baseReadLineDataCalled(false)
       , firstRead(true)
       , accessMode(Unset)
#ifdef QT_NO_QOBJECT
       , q_ptr(0)
#endif
{
}

/*! \internal
 */
QIODevicePrivate::~QIODevicePrivate()
{
}

/*!
    \class QIODevice
    \reentrant

    \brief The QIODevice class is the base interface class of all I/O
    devices in Qt.

    \ingroup io

    QIODevice provides both a common implementation and an abstract
    interface for devices that support reading and writing of blocks
    of data, such as QFile, QBuffer and QTcpSocket. QIODevice is
    abstract and can not be instantiated, but it is common to use the
    interface it defines to provide device-independent I/O features.
    For example, Qt's XML classes operate on a QIODevice pointer,
    allowing them to be used with various devices (such as files and
    buffers).

    Before accessing the device, open() must be called to set the
    correct OpenMode (such as ReadOnly or ReadWrite). You can then
    write to the device with write() or putChar(), and read by calling
    either read(), readLine(), or readAll(). Call close() when you are
    done with the device.

    QIODevice distinguishes between two types of devices:
    random-access devices and sequential devices.

    \list
    \o Random-access devices support seeking to arbitrary
    positions using seek(). The current position in the file is
    available by calling pos(). QFile and QBuffer are examples of
    random-access devices.

    \o Sequential devices don't support seeking to arbitrary
    positions. The data must be read in one pass. The functions
    pos() and size() don't work for sequential devices.
    QTcpSocket and QProcess are examples of sequential devices.
    \endlist

    You can use isSequential() to determine the type of device.

    QIODevice emits readyRead() when new data is available for
    reading; for example, if new data has arrived on the network or if
    additional data is appended to a file that you are reading
    from. You can call bytesAvailable() to determine the number of
    bytes that are currently available for reading. It's common to use
    bytesAvailable() together with the readyRead() signal when
    programming with asynchronous devices such as QTcpSocket, where
    fragments of data can arrive at arbitrary points in
    time. QIODevice emits the bytesWritten() signal every time a
    payload of data has been written to the device. Use bytesToWrite()
    to determine the current amount of data waiting to be written.

    Certain subclasses of QIODevice, such as QTcpSocket and QProcess,
    are asynchronous. This means that I/O functions such as write()
    or read() always return immediately, while communication with the
    device itself may happen when control goes back to the event loop.
    QIODevice provides functions that allow you to force these
    operations to be performed immediately, while blocking the
    calling thread and without entering the event loop. This allows
    QIODevice subclasses to be used without an event loop, or in
    a separate thread:

    \list
    \o waitForReadyRead() - This function suspends operation in the
    calling thread until new data is available for reading.

    \o waitForBytesWritten() - This function suspends operation in the
    calling thread until one payload of data has been written to the
    device.

    \o waitFor....() - Subclasses of QIODevice implement blocking
    functions for device-specific operations. For example, QProcess
    has a function called waitForStarted() which suspends operation in
    the calling thread until the process has started.
    \endlist

    Calling these functions from the main, GUI thread, may cause your
    user interface to freeze. Example:

    \snippet doc/src/snippets/code/src_corelib_io_qiodevice.cpp 0

    By subclassing QIODevice, you can provide the same interface to
    your own I/O devices. Subclasses of QIODevice are only required to
    implement the protected readData() and writeData() functions.
    QIODevice uses these functions to implement all its convenience
    functions, such as getChar(), readLine() and write(). QIODevice
    also handles access control for you, so you can safely assume that
    the device is opened in write mode if writeData() is called.

    Some subclasses, such as QFile and QTcpSocket, are implemented
    using a memory buffer for intermediate storing of data. This
    reduces the number of required device accessing calls, which are
    often very slow. Buffering makes functions like getChar() and
    putChar() fast, as they can operate on the memory buffer instead
    of directly on the device itself. Certain I/O operations, however,
    don't work well with a buffer. For example, if several users open
    the same device and read it character by character, they may end
    up reading the same data when they meant to read a separate chunk
    each. For this reason, QIODevice allows you to bypass any
    buffering by passing the Unbuffered flag to open(). When
    subclassing QIODevice, remember to bypass any buffer you may use
    when the device is open in Unbuffered mode.

    \sa QBuffer QFile QTcpSocket
*/

/*!
    \typedef QIODevice::Offset
    \compat

    Use \c qint64 instead.
*/

/*!
    \typedef QIODevice::Status
    \compat

    Use QIODevice::OpenMode instead, or see the documentation for
    specific devices.
*/

/*!
    \enum QIODevice::OpenModeFlag

    This enum is used with open() to describe the mode in which a device
    is opened. It is also returned by openMode().

    \value NotOpen   The device is not open.
    \value ReadOnly  The device is open for reading.
    \value WriteOnly The device is open for writing.
    \value ReadWrite The device is open for reading and writing.
    \value Append    The device is opened in append mode, so that all data is
                     written to the end of the file.
    \value Truncate  If possible, the device is truncated before it is opened.
                     All earlier contents of the device are lost.
    \value Text      When reading, the end-of-line terminators are
                     translated to '\n'. When writing, the end-of-line
                     terminators are translated to the local encoding, for
                     example '\r\n' for Win32.
    \value Unbuffered Any buffer in the device is bypassed.

    Certain flags, such as \c Unbuffered and \c Truncate, are
    meaningless when used with some subclasses. Some of these
    restrictions are implied by the type of device that is represented
    by a subclass. In other cases, the restriction may be due to the
    implementation, or may be imposed by the underlying platform; for
    example, QTcpSocket does not support \c Unbuffered mode, and
    limitations in the native API prevent QFile from supporting \c
    Unbuffered on Windows.
*/

/*!     \fn QIODevice::bytesWritten(qint64 bytes)

    This signal is emitted every time a payload of data has been
    written to the device. The \a bytes argument is set to the number
    of bytes that were written in this payload.

    bytesWritten() is not emitted recursively; if you reenter the event loop
    or call waitForBytesWritten() inside a slot connected to the
    bytesWritten() signal, the signal will not be reemitted (although
    waitForBytesWritten() may still return true).

    \sa readyRead()
*/

/*!
    \fn QIODevice::readyRead()

    This signal is emitted once every time new data is available for
    reading from the device. It will only be emitted again once new
    data is available, such as when a new payload of network data has
    arrived on your network socket, or when a new block of data has
    been appended to your device.

    readyRead() is not emitted recursively; if you reenter the event loop or
    call waitForReadyRead() inside a slot connected to the readyRead() signal,
    the signal will not be reemitted (although waitForReadyRead() may still
    return true).

    Note for developers implementing classes derived from QIODevice:
    you should always emit readyRead() when new data has arrived (do not
    emit it only because there's data still to be read in your
    buffers). Do not emit readyRead() in other conditions.

    \sa bytesWritten()
*/

/*! \fn QIODevice::aboutToClose()

    This signal is emitted when the device is about to close. Connect
    this signal if you have operations that need to be performed
    before the device closes (e.g., if you have data in a separate
    buffer that needs to be written to the device).
*/

/*!
    \fn QIODevice::readChannelFinished()
    \since 4.4

    This signal is emitted when the input (reading) stream is closed
    in this device. It is emitted as soon as the closing is detected,
    which means that there might still be data available for reading
    with read().

    \sa atEnd(), read()
*/

#ifdef QT_NO_QOBJECT
QIODevice::QIODevice()
    : d_ptr(new QIODevicePrivate)
{
    d_ptr->q_ptr = this;
}

/*! \internal
*/
QIODevice::QIODevice(QIODevicePrivate &dd)
    : d_ptr(&dd)
{
    d_ptr->q_ptr = this;
}
#else

/*!
    Constructs a QIODevice object.
*/

QIODevice::QIODevice()
    : QObject(*new QIODevicePrivate, 0)
{
#if defined QIODEVICE_DEBUG
    QFile *file = qobject_cast<QFile *>(this);
    printf("%p QIODevice::QIODevice(\"%s\") %s\n", this, metaObject()->className(),
           qPrintable(file ? file->fileName() : QString()));
#endif
}

/*!
    Constructs a QIODevice object with the given \a parent.
*/

QIODevice::QIODevice(QObject *parent)
    : QObject(*new QIODevicePrivate, parent)
{
#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::QIODevice(%p \"%s\")\n", this, parent, metaObject()->className());
#endif
}

/*! \internal
*/
QIODevice::QIODevice(QIODevicePrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
}
#endif


/*!
  The destructor is virtual, and QIODevice is an abstract base
  class. This destructor does not call close(), but the subclass
  destructor might. If you are in doubt, call close() before
  destroying the QIODevice.
*/
QIODevice::~QIODevice()
{
#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::~QIODevice()\n", this);
#endif
}

/*!
    Returns true if this device is sequential; otherwise returns
    false.

    Sequential devices, as opposed to a random-access devices, have no
    concept of a start, an end, a size, or a current position, and they
    do not support seeking. You can only read from the device when it
    reports that data is available. The most common example of a
    sequential device is a network socket. On Unix, special files such
    as /dev/zero and fifo pipes are sequential.

    Regular files, on the other hand, do support random access. They
    have both a size and a current position, and they also support
    seeking backwards and forwards in the data stream. Regular files
    are non-sequential.

    The QIODevice implementation returns false.

    \sa bytesAvailable()
*/
bool QIODevice::isSequential() const
{
    return false;
}

/*!
    Returns the mode in which the device has been opened;
    i.e. ReadOnly or WriteOnly.

    \sa OpenMode
*/
QIODevice::OpenMode QIODevice::openMode() const
{
    return d_func()->openMode;
}

/*!
    Sets the OpenMode of the device to \a openMode. Call this
    function to set the open mode if the flags change after the device
    has been opened.

    \sa openMode() OpenMode
*/
void QIODevice::setOpenMode(OpenMode openMode)
{
    Q_D(QIODevice);
#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::setOpenMode(0x%x)\n", this, int(openMode));
#endif
    d->openMode = openMode;
    d->accessMode = QIODevicePrivate::Unset;
    d->firstRead = true;
    if (!isReadable())
        d->buffer.clear();
}

/*!
    If \a enabled is true, this function sets the \l Text flag on the device;
    otherwise the \l Text flag is removed. This feature is useful for classes
    that provide custom end-of-line handling on a QIODevice.

    The IO device should be opened before calling this function.

    \sa open(), setOpenMode()
 */
void QIODevice::setTextModeEnabled(bool enabled)
{
    Q_D(QIODevice);
    if (!isOpen()) {
        qWarning("QIODevice::setTextModeEnabled: The device is not open");
        return;
    }
    if (enabled)
        d->openMode |= Text;
    else
        d->openMode &= ~Text;
}

/*!
    Returns true if the \l Text flag is enabled; otherwise returns false.

    \sa setTextModeEnabled()
*/
bool QIODevice::isTextModeEnabled() const
{
    return d_func()->openMode & Text;
}

/*!
    Returns true if the device is open; otherwise returns false. A
    device is open if it can be read from and/or written to. By
    default, this function returns false if openMode() returns
    \c NotOpen.

    \sa openMode() OpenMode
*/
bool QIODevice::isOpen() const
{
    return d_func()->openMode != NotOpen;
}

/*!
    Returns true if data can be read from the device; otherwise returns
    false. Use bytesAvailable() to determine how many bytes can be read.

    This is a convenience function which checks if the OpenMode of the
    device contains the ReadOnly flag.

    \sa openMode() OpenMode
*/
bool QIODevice::isReadable() const
{
    return (openMode() & ReadOnly) != 0;
}

/*!
    Returns true if data can be written to the device; otherwise returns
    false.

    This is a convenience function which checks if the OpenMode of the
    device contains the WriteOnly flag.

    \sa openMode() OpenMode
*/
bool QIODevice::isWritable() const
{
    return (openMode() & WriteOnly) != 0;
}

/*!
    Opens the device and sets its OpenMode to \a mode. Returns true if successful;
    otherwise returns false. This function should be called from any
    reimplementations of open() or other functions that open the device.

    \sa openMode() OpenMode
*/
bool QIODevice::open(OpenMode mode)
{
    Q_D(QIODevice);
    d->openMode = mode;
    d->pos = (mode & Append) ? size() : qint64(0);
    d->buffer.clear();
    d->accessMode = QIODevicePrivate::Unset;
    d->firstRead = true;
#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::open(0x%x)\n", this, quint32(mode));
#endif
    return true;
}

/*!
    First emits aboutToClose(), then closes the device and sets its
    OpenMode to NotOpen. The error string is also reset.

    \sa setOpenMode() OpenMode
*/
void QIODevice::close()
{
    Q_D(QIODevice);
    if (d->openMode == NotOpen)
        return;

#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::close()\n", this);
#endif

#ifndef QT_NO_QOBJECT
    emit aboutToClose();
#endif
    d->openMode = NotOpen;
    d->errorString.clear();
    d->pos = 0;
    d->seqDumpPos = 0;
    d->buffer.clear();
    d->firstRead = true;
}

/*!
    For random-access devices, this function returns the position that
    data is written to or read from. For sequential devices or closed
    devices, where there is no concept of a "current position", 0 is
    returned.

    The current read/write position of the device is maintained internally by
    QIODevice, so reimplementing this function is not necessary. When
    subclassing QIODevice, use QIODevice::seek() to notify QIODevice about
    changes in the device position.

    \sa isSequential(), seek()
*/
qint64 QIODevice::pos() const
{
    Q_D(const QIODevice);
#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::pos() == %d\n", this, int(d->pos));
#endif
    return d->pos;
}

/*!
    For open random-access devices, this function returns the size of the
    device. For open sequential devices, bytesAvailable() is returned.

    If the device is closed, the size returned will not reflect the actual
    size of the device.

    \sa isSequential(), pos()
*/
qint64 QIODevice::size() const
{
    return d_func()->isSequential() ?  bytesAvailable() : qint64(0);
}

/*!
    For random-access devices, this function sets the current position
    to \a pos, returning true on success, or false if an error occurred.
    For sequential devices, the default behavior is to do nothing and
    return false.

    When subclassing QIODevice, you must call QIODevice::seek() at the
    start of your function to ensure integrity with QIODevice's
    built-in buffer. The base implementation always returns true.

    \sa pos(), isSequential()
*/
bool QIODevice::seek(qint64 pos)
{
    Q_D(QIODevice);
    if (d->openMode == NotOpen) {
        qWarning("QIODevice::seek: The device is not open");
        return false;
    }
    if (pos < 0) {
        qWarning("QIODevice::seek: Invalid pos: %d", int(pos));
        return false;
    }

#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::seek(%d), before: d->pos = %d, d->buffer.size() = %d\n",
           this, int(pos), int(d->pos), d->buffer.size());
#endif

    qint64 offset = pos - d->pos;
    if (!d->isSequential()) {
        d->pos = pos;
        d->devicePos = pos;
    }

    if (offset < 0
            || offset >= qint64(d->buffer.size()))
        // When seeking backwards, an operation that is only allowed for
        // random-access devices, the buffer is cleared. The next read
        // operation will then refill the buffer. We can optimize this, if we
        // find that seeking backwards becomes a significant performance hit.
        d->buffer.clear();
    else if (!d->buffer.isEmpty())
        d->buffer.skip(int(offset));

#if defined QIODEVICE_DEBUG
    printf("%p \tafter: d->pos == %d, d->buffer.size() == %d\n", this, int(d->pos),
           d->buffer.size());
#endif
    return true;
}

/*!
    Returns true if the current read and write position is at the end
    of the device (i.e. there is no more data available for reading on
    the device); otherwise returns false.

    For some devices, atEnd() can return true even though there is more data
    to read. This special case only applies to devices that generate data in
    direct response to you calling read() (e.g., \c /dev or \c /proc files on
    Unix and Mac OS X, or console input / \c stdin on all platforms).

    \sa bytesAvailable(), read(), isSequential()
*/
bool QIODevice::atEnd() const
{
    Q_D(const QIODevice);
#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::atEnd() returns %s, d->openMode == %d, d->pos == %d\n", this, (d->openMode == NotOpen || d->pos == size()) ? "true" : "false",
           int(d->openMode), int(d->pos));
#endif
    return d->openMode == NotOpen || (d->buffer.isEmpty() && bytesAvailable() == 0);
}

/*!
    Seeks to the start of input for random-access devices. Returns
    true on success; otherwise returns false (for example, if the
    device is not open).

    Note that when using a QTextStream on a QFile, calling reset() on
    the QFile will not have the expected result because QTextStream
    buffers the file. Use the QTextStream::seek() function instead.

    \sa seek()
*/
bool QIODevice::reset()
{
#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::reset()\n", this);
#endif
    return seek(0);
}

/*!
    Returns the number of bytes that are available for reading. This
    function is commonly used with sequential devices to determine the
    number of bytes to allocate in a buffer before reading.

    Subclasses that reimplement this function must call the base
    implementation in order to include the size of QIODevices' buffer. Example:

    \snippet doc/src/snippets/code/src_corelib_io_qiodevice.cpp 1

    \sa bytesToWrite(), readyRead(), isSequential()
*/
qint64 QIODevice::bytesAvailable() const
{
    Q_D(const QIODevice);
    if (!d->isSequential())
        return qMax(size() - d->pos, qint64(0));
    return d->buffer.size();
}

/*!
    For buffered devices, this function returns the number of bytes
    waiting to be written. For devices with no buffer, this function
    returns 0.

    \sa bytesAvailable(), bytesWritten(), isSequential()
*/
qint64 QIODevice::bytesToWrite() const
{
    return qint64(0);
}

#ifdef Q_CC_RVCT
// arm mode makes the 64-bit integer operations much faster in RVCT 2.2
#pragma push
#pragma arm
#endif

/*!
    Reads at most \a maxSize bytes from the device into \a data, and
    returns the number of bytes read. If an error occurs, such as when
    attempting to read from a device opened in WriteOnly mode, this
    function returns -1.

    0 is returned when no more data is available for reading. However,
    reading past the end of the stream is considered an error, so this
    function returns -1 in those cases (that is, reading on a closed
    socket or after a process has died).

    \sa readData() readLine() write()
*/
qint64 QIODevice::read(char *data, qint64 maxSize)
{
    Q_D(QIODevice);

#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::read(%p, %d), d->pos = %d, d->buffer.size() = %d\n",
           this, data, int(maxSize), int(d->pos), int(d->buffer.size()));
#endif

    // Short circuit for getChar()
    if (maxSize == 1) {
        int chint;
        while ((chint = d->buffer.getChar()) != -1) {
            ++(*d->pPos);

            char c = char(uchar(chint));
            if (c == '\r' && (d->openMode & Text))
                continue;
            *data = c;
#if defined QIODEVICE_DEBUG
            printf("%p \tread 0x%hhx (%c) returning 1 (shortcut)\n", this,
                   int(c), isprint(c) ? c : '?');
#endif
            return qint64(1);
        }
    }

    CHECK_MAXLEN(read, qint64(-1));
    qint64 readSoFar = 0;
    bool moreToRead = true;
    do {
        // Try reading from the buffer.
        int lastReadChunkSize = d->buffer.read(data, maxSize);
        if (lastReadChunkSize > 0) {
            *d->pPos += lastReadChunkSize;
            readSoFar += lastReadChunkSize;
            // fast exit when satisfied by buffer
            if (lastReadChunkSize == maxSize && !(d->openMode & Text))
                return readSoFar;

            data += lastReadChunkSize;
            maxSize -= lastReadChunkSize;
#if defined QIODEVICE_DEBUG
            printf("%p \treading %d bytes from buffer into position %d\n", this, lastReadChunkSize,
                   int(readSoFar) - lastReadChunkSize);
#endif
        } else {
            if (d->firstRead) {
                // this is the first time the file has been read, check it's valid and set up pos pointers
                // for fast pos updates.
                CHECK_READABLE(read, qint64(-1));
                d->firstRead = false;
                if (d->isSequential()) {
                    d->pPos = &d->seqDumpPos;
                    d->pDevicePos = &d->seqDumpPos;
                }
            }

            if (!maxSize)
                return readSoFar;

            if ((d->openMode & Unbuffered) == 0 && maxSize < QIODEVICE_BUFFERSIZE) {
                // In buffered mode, we try to fill up the QIODevice buffer before
                // we do anything else.
                // buffer is empty at this point, try to fill it
                int bytesToBuffer = QIODEVICE_BUFFERSIZE;
                char *writePointer = d->buffer.reserve(bytesToBuffer);

                // Make sure the device is positioned correctly.
                if (d->pos != d->devicePos && !d->isSequential() && !seek(d->pos))
                    return readSoFar ? readSoFar : qint64(-1);
                qint64 readFromDevice = readData(writePointer, bytesToBuffer);
                d->buffer.chop(bytesToBuffer - (readFromDevice < 0 ? 0 : int(readFromDevice)));

                if (readFromDevice > 0) {
                    *d->pDevicePos += readFromDevice;
#if defined QIODEVICE_DEBUG
                    printf("%p \treading %d from device into buffer\n", this, int(readFromDevice));
#endif

                    if (!d->buffer.isEmpty()) {
                        lastReadChunkSize = d->buffer.read(data, maxSize);
                        readSoFar += lastReadChunkSize;
                        data += lastReadChunkSize;
                        maxSize -= lastReadChunkSize;
                        *d->pPos += lastReadChunkSize;
#if defined QIODEVICE_DEBUG
                        printf("%p \treading %d bytes from buffer at position %d\n", this,
                               lastReadChunkSize, int(readSoFar));
#endif
                    }
                }
            }
        }

        // If we need more, try reading from the device.
        if (maxSize > 0) {
            // Make sure the device is positioned correctly.
            if (d->pos != d->devicePos && !d->isSequential() && !seek(d->pos))
                return readSoFar ? readSoFar : qint64(-1);
            qint64 readFromDevice = readData(data, maxSize);
#if defined QIODEVICE_DEBUG
            printf("%p \treading %d bytes from device (total %d)\n", this, int(readFromDevice), int(readSoFar));
#endif
            if (readFromDevice == -1 && readSoFar == 0) {
                // error and we haven't read anything: return immediately
                return -1;
            }
            if (readFromDevice > 0) {
                lastReadChunkSize += int(readFromDevice);
                readSoFar += readFromDevice;
                data += readFromDevice;
                maxSize -= readFromDevice;
                *d->pPos += readFromDevice;
                *d->pDevicePos += readFromDevice;
            }
        }
        // Best attempt has been made to read data, don't try again except for text mode adjustment below
        moreToRead = false;

        if (readSoFar && d->openMode & Text) {
            char *readPtr = data - lastReadChunkSize;
            const char *endPtr = data;

            if (readPtr < endPtr) {
                // optimization to avoid initial self-assignment
                while (*readPtr != '\r') {
                    if (++readPtr == endPtr)
                        return readSoFar;
                }

                char *writePtr = readPtr;

                while (readPtr < endPtr) {
                    char ch = *readPtr++;
                    if (ch != '\r')
                        *writePtr++ = ch;
                    else {
                        --readSoFar;
                        --data;
                        ++maxSize;
                    }
                }

                // Make sure we get more data if there is room for more. This
                // is very important for when someone seeks to the start of a
                // '\r\n' and reads one character - they should get the '\n'.
                moreToRead = (readPtr != writePtr);
            }
        }
    } while (moreToRead);

#if defined QIODEVICE_DEBUG
    printf("%p \treturning %d, d->pos == %d, d->buffer.size() == %d\n", this,
           int(readSoFar), int(d->pos), d->buffer.size());
    debugBinaryString(data - readSoFar, readSoFar);
#endif
    return readSoFar;
}

#ifdef Q_CC_RVCT
#pragma pop
#endif

/*!
    \overload

    Reads at most \a maxSize bytes from the device, and returns the
    data read as a QByteArray.

    This function has no way of reporting errors; returning an empty
    QByteArray() can mean either that no data was currently available
    for reading, or that an error occurred.
*/
QByteArray QIODevice::read(qint64 maxSize)
{
    Q_D(QIODevice);
    QByteArray result;

    CHECK_MAXLEN(read, result);

#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::read(%d), d->pos = %d, d->buffer.size() = %d\n",
           this, int(maxSize), int(d->pos), int(d->buffer.size()));
#else
    Q_UNUSED(d);
#endif

    if (maxSize != qint64(int(maxSize))) {
        qWarning("QIODevice::read: maxSize argument exceeds QByteArray size limit");
        maxSize = INT_MAX;
    }

    qint64 readBytes = 0;
    if (maxSize) {
        result.resize(int(maxSize));
        if (!result.size()) {
            // If resize fails, read incrementally.
            qint64 readResult;
            do {
                result.resize(int(qMin(maxSize, result.size() + QIODEVICE_BUFFERSIZE)));
                readResult = read(result.data() + readBytes, result.size() - readBytes);
                if (readResult > 0 || readBytes == 0)
                    readBytes += readResult;
            } while (readResult == QIODEVICE_BUFFERSIZE);
        } else {
            readBytes = read(result.data(), result.size());
        }
    }

    if (readBytes <= 0)
        result.clear();
    else
        result.resize(int(readBytes));

    return result;
}

/*!
    \overload

    Reads all available data from the device, and returns it as a
    QByteArray.

    This function has no way of reporting errors; returning an empty
    QByteArray() can mean either that no data was currently available
    for reading, or that an error occurred.
*/
QByteArray QIODevice::readAll()
{
    Q_D(QIODevice);
#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::readAll(), d->pos = %d, d->buffer.size() = %d\n",
           this, int(d->pos), int(d->buffer.size()));
#endif

    QByteArray result;
    qint64 readBytes = 0;

    // flush internal read buffer
    if (!(d->openMode & Text) && !d->buffer.isEmpty()) {
        result = d->buffer.readAll();
        readBytes = result.size();
        d->pos += readBytes;
    }

    qint64 theSize;
    if (d->isSequential() || (theSize = size()) == 0) {
        // Size is unknown, read incrementally.
        qint64 readResult;
        do {
            result.resize(result.size() + QIODEVICE_BUFFERSIZE);
            readResult = read(result.data() + readBytes, result.size() - readBytes);
            if (readResult > 0 || readBytes == 0)
                readBytes += readResult;
        } while (readResult > 0);
    } else {
        // Read it all in one go.
        // If resize fails, don't read anything.
        result.resize(int(readBytes + theSize - d->pos));
        readBytes += read(result.data() + readBytes, result.size() - readBytes);
    }

    if (readBytes <= 0)
        result.clear();
    else
        result.resize(int(readBytes));

    return result;
}

#ifdef Q_CC_RVCT
// arm mode makes the 64-bit integer operations much faster in RVCT 2.2
#pragma push
#pragma arm
#endif

/*!
    This function reads a line of ASCII characters from the device, up
    to a maximum of \a maxSize - 1 bytes, stores the characters in \a
    data, and returns the number of bytes read. If a line could not be
    read but no error ocurred, this function returns 0. If an error
    occurs, this function returns the length of what could be read, or
    -1 if nothing was read.

    A terminating '\0' byte is always appended to \a data, so \a
    maxSize must be larger than 1.

    Data is read until either of the following conditions are met:

    \list
    \o The first '\n' character is read.
    \o \a maxSize - 1 bytes are read.
    \o The end of the device data is detected.
    \endlist

    For example, the following code reads a line of characters from a
    file:

    \snippet doc/src/snippets/code/src_corelib_io_qiodevice.cpp 2

    The newline character ('\n') is included in the buffer. If a
    newline is not encountered before maxSize - 1 bytes are read, a
    newline will not be inserted into the buffer. On windows newline
    characters are replaced with '\n'.

    This function calls readLineData(), which is implemented using
    repeated calls to getChar(). You can provide a more efficient
    implementation by reimplementing readLineData() in your own
    subclass.

    \sa getChar(), read(), write()
*/
qint64 QIODevice::readLine(char *data, qint64 maxSize)
{
    Q_D(QIODevice);
    if (maxSize < 2) {
        qWarning("QIODevice::readLine: Called with maxSize < 2");
        return qint64(-1);
    }

#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::readLine(%p, %d), d->pos = %d, d->buffer.size() = %d\n",
           this, data, int(maxSize), int(d->pos), int(d->buffer.size()));
#endif

    // Leave room for a '\0'
    --maxSize;

    const bool sequential = d->isSequential();

    qint64 readSoFar = 0;
    if (!d->buffer.isEmpty()) {
        readSoFar = d->buffer.readLine(data, maxSize);
        if (!sequential)
            d->pos += readSoFar;
#if defined QIODEVICE_DEBUG
        printf("%p \tread from buffer: %d bytes, last character read: %hhx\n", this,
               int(readSoFar), data[int(readSoFar) - 1]);
        if (readSoFar)
            debugBinaryString(data, int(readSoFar));
#endif
#if defined(Q_OS_SYMBIAN)
        // Open C fgets strips '\r' but readSoFar gets returned as if it was still there
        if ((d->openMode & Text) &&
            readSoFar > 1 &&
            data[readSoFar - 1] == '\0' &&
            data[readSoFar - 2] == '\n') {
            --readSoFar;
        }
#endif
        if (readSoFar && data[readSoFar - 1] == '\n') {
            if (d->openMode & Text) {
                // QRingBuffer::readLine() isn't Text aware.
                if (readSoFar > 1 && data[readSoFar - 2] == '\r') {
                    --readSoFar;
                    data[readSoFar - 1] = '\n';
                }
            }
            data[readSoFar] = '\0';
            return readSoFar;
        }
    }

    if (d->pos != d->devicePos && !sequential && !seek(d->pos))
        return qint64(-1);
    d->baseReadLineDataCalled = false;
    qint64 readBytes = readLineData(data + readSoFar, maxSize - readSoFar);
#if defined QIODEVICE_DEBUG
    printf("%p \tread from readLineData: %d bytes, readSoFar = %d bytes\n", this,
           int(readBytes), int(readSoFar));
    if (readBytes > 0) {
        debugBinaryString(data, int(readSoFar + readBytes));
    }
#endif
    if (readBytes < 0) {
        data[readSoFar] = '\0';
        return readSoFar ? readSoFar : -1;
    }
    readSoFar += readBytes;
    if (!d->baseReadLineDataCalled && !sequential) {
        d->pos += readBytes;
        // If the base implementation was not called, then we must
        // assume the device position is invalid and force a seek.
        d->devicePos = qint64(-1);
    }
    data[readSoFar] = '\0';

    if (d->openMode & Text) {
#if defined(Q_OS_SYMBIAN)
        // Open C fgets strips '\r' but readSoFar gets returned as if it was still there
        if (readSoFar > 1 && data[readSoFar - 1] == '\0' && data[readSoFar - 2] == '\n') {
            --readSoFar;
        }
#endif
        if (readSoFar > 1 && data[readSoFar - 1] == '\n' && data[readSoFar - 2] == '\r') {
            data[readSoFar - 2] = '\n';
            data[readSoFar - 1] = '\0';
            --readSoFar;
        }
    }

#if defined QIODEVICE_DEBUG
    printf("%p \treturning %d, d->pos = %d, d->buffer.size() = %d, size() = %d\n",
           this, int(readSoFar), int(d->pos), d->buffer.size(), int(size()));
    debugBinaryString(data, int(readSoFar));
#endif
    return readSoFar;
}

/*!
    \overload

    Reads a line from the device, but no more than \a maxSize characters,
    and returns the result as a QByteArray.

    This function has no way of reporting errors; returning an empty
    QByteArray() can mean either that no data was currently available
    for reading, or that an error occurred.
*/
QByteArray QIODevice::readLine(qint64 maxSize)
{
    Q_D(QIODevice);
    QByteArray result;

    CHECK_MAXLEN(readLine, result);

#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::readLine(%d), d->pos = %d, d->buffer.size() = %d\n",
           this, int(maxSize), int(d->pos), int(d->buffer.size()));
#else
    Q_UNUSED(d);
#endif

    if (maxSize > INT_MAX) {
        qWarning("QIODevice::read: maxSize argument exceeds QByteArray size limit");
        maxSize = INT_MAX;
    }

    result.resize(int(maxSize));
    qint64 readBytes = 0;
    if (!result.size()) {
        // If resize fails or maxSize == 0, read incrementally
        if (maxSize == 0)
            maxSize = INT_MAX;

        // The first iteration needs to leave an extra byte for the terminating null
        result.resize(1);

        qint64 readResult;
        do {
            result.resize(int(qMin(maxSize, result.size() + QIODEVICE_BUFFERSIZE)));
            readResult = readLine(result.data() + readBytes, result.size() - readBytes);
            if (readResult > 0 || readBytes == 0)
                readBytes += readResult;
        } while (readResult == QIODEVICE_BUFFERSIZE
                && result[int(readBytes - 1)] != '\n');
    } else
        readBytes = readLine(result.data(), result.size());

    if (readBytes <= 0)
        result.clear();
    else
        result.resize(readBytes);

    return result;
}

/*!
    Reads up to \a maxSize characters into \a data and returns the
    number of characters read.

    This function is called by readLine(), and provides its base
    implementation, using getChar(). Buffered devices can improve the
    performance of readLine() by reimplementing this function.

    readLine() appends a '\0' byte to \a data; readLineData() does not
    need to do this.

    If you reimplement this function, be careful to return the correct
    value: it should return the number of bytes read in this line,
    including the terminating newline, or 0 if there is no line to be
    read at this point. If an error occurs, it should return -1 if and
    only if no bytes were read. Reading past EOF is considered an error.
*/
qint64 QIODevice::readLineData(char *data, qint64 maxSize)
{
    Q_D(QIODevice);
    qint64 readSoFar = 0;
    char c;
    int lastReadReturn = 0;
    d->baseReadLineDataCalled = true;

    while (readSoFar < maxSize && (lastReadReturn = read(&c, 1)) == 1) {
        *data++ = c;
        ++readSoFar;
        if (c == '\n')
            break;
    }

#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::readLineData(%p, %d), d->pos = %d, d->buffer.size() = %d, returns %d\n",
           this, data, int(maxSize), int(d->pos), int(d->buffer.size()), int(readSoFar));
#endif
    if (lastReadReturn != 1 && readSoFar == 0)
        return isSequential() ? lastReadReturn : -1;
    return readSoFar;
}

#ifdef Q_CC_RVCT
#pragma pop
#endif

/*!
    Returns true if a complete line of data can be read from the device;
    otherwise returns false.

    Note that unbuffered devices, which have no way of determining what
    can be read, always return false.

    This function is often called in conjunction with the readyRead()
    signal.

    Subclasses that reimplement this function must call the base
    implementation in order to include the contents of the QIODevice's buffer. Example:

    \snippet doc/src/snippets/code/src_corelib_io_qiodevice.cpp 3

    \sa readyRead(), readLine()
*/
bool QIODevice::canReadLine() const
{
    return d_func()->buffer.canReadLine();
}

/*!
    Writes at most \a maxSize bytes of data from \a data to the
    device. Returns the number of bytes that were actually written, or
    -1 if an error occurred.

    \sa read() writeData()
*/
qint64 QIODevice::write(const char *data, qint64 maxSize)
{
    Q_D(QIODevice);
    CHECK_WRITABLE(write, qint64(-1));
    CHECK_MAXLEN(write, qint64(-1));

    const bool sequential = d->isSequential();
    // Make sure the device is positioned correctly.
    if (d->pos != d->devicePos && !sequential && !seek(d->pos))
        return qint64(-1);

#ifdef Q_OS_WIN
    if (d->openMode & Text) {
        const char *endOfData = data + maxSize;
        const char *startOfBlock = data;

        qint64 writtenSoFar = 0;

        forever {
            const char *endOfBlock = startOfBlock;
            while (endOfBlock < endOfData && *endOfBlock != '\n')
                ++endOfBlock;

            qint64 blockSize = endOfBlock - startOfBlock;
            if (blockSize > 0) {
                qint64 ret = writeData(startOfBlock, blockSize);
                if (ret <= 0) {
                    if (writtenSoFar && !sequential)
                        d->buffer.skip(writtenSoFar);
                    return writtenSoFar ? writtenSoFar : ret;
                }
                if (!sequential) {
                    d->pos += ret;
                    d->devicePos += ret;
                }
                writtenSoFar += ret;
            }

            if (endOfBlock == endOfData)
                break;

            qint64 ret = writeData("\r\n", 2);
            if (ret <= 0) {
                if (writtenSoFar && !sequential)
                    d->buffer.skip(writtenSoFar);
                return writtenSoFar ? writtenSoFar : ret;
            }
            if (!sequential) {
                d->pos += ret;
                d->devicePos += ret;
            }
            ++writtenSoFar;

            startOfBlock = endOfBlock + 1;
        }

        if (writtenSoFar && !sequential)
            d->buffer.skip(writtenSoFar);
        return writtenSoFar;
    }
#endif

    qint64 written = writeData(data, maxSize);
    if (written > 0) {
        if (!sequential) {
            d->pos += written;
            d->devicePos += written;
        }
        if (!d->buffer.isEmpty() && !sequential)
            d->buffer.skip(written);
    }
    return written;
}

/*!
    \since 4.5

    \overload
 
    Writes data from a zero-terminated string of 8-bit characters to the
    device. Returns the number of bytes that were actually written, or
    -1 if an error occurred. This is equivalent to
    \code
    ...
    QIODevice::write(data, qstrlen(data));
    ...
    \endcode

    \sa read() writeData()
*/
qint64 QIODevice::write(const char *data)
{
    return write(data, qstrlen(data));
}

/*! \fn qint64 QIODevice::write(const QByteArray &byteArray)

    \overload

    Writes the content of \a byteArray to the device. Returns the number of
    bytes that were actually written, or -1 if an error occurred.

    \sa read() writeData()
*/

/*!
    Puts the character \a c back into the device, and decrements the
    current position unless the position is 0. This function is
    usually called to "undo" a getChar() operation, such as when
    writing a backtracking parser.

    If \a c was not previously read from the device, the behavior is
    undefined.
*/
void QIODevice::ungetChar(char c)
{
    Q_D(QIODevice);
    CHECK_READABLE(read, Q_VOID);

#if defined QIODEVICE_DEBUG
    printf("%p QIODevice::ungetChar(0x%hhx '%c')\n", this, c, isprint(c) ? c : '?');
#endif

    d->buffer.ungetChar(c);
    if (!d->isSequential())
        --d->pos;
}

/*! \fn bool QIODevice::putChar(char c)

    Writes the character \a c to the device. Returns true on success;
    otherwise returns false.

    \sa write() getChar() ungetChar()
*/
bool QIODevice::putChar(char c)
{
    return d_func()->putCharHelper(c);
}

/*!
    \internal
*/
bool QIODevicePrivate::putCharHelper(char c)
{
    return q_func()->write(&c, 1) == 1;
}

/*!
    \internal
*/
qint64 QIODevicePrivate::peek(char *data, qint64 maxSize)
{
    qint64 readBytes = q_func()->read(data, maxSize);
    if (readBytes <= 0)
        return readBytes;

    buffer.ungetBlock(data, readBytes);
    *pPos -= readBytes;
    return readBytes;
}

/*!
    \internal
*/
QByteArray QIODevicePrivate::peek(qint64 maxSize)
{
    QByteArray result = q_func()->read(maxSize);

    if (result.isEmpty())
        return result;

    buffer.ungetBlock(result.constData(), result.size());
    *pPos -= result.size();
    return result;
}

/*! \fn bool QIODevice::getChar(char *c)

    Reads one character from the device and stores it in \a c. If \a c
    is 0, the character is discarded. Returns true on success;
    otherwise returns false.

    \sa read() putChar() ungetChar()
*/
bool QIODevice::getChar(char *c)
{
    // readability checked in read()
    char ch;
    return (1 == read(c ? c : &ch, 1));
}

/*!
    \since 4.1

    Reads at most \a maxSize bytes from the device into \a data, without side
    effects (i.e., if you call read() after peek(), you will get the same
    data).  Returns the number of bytes read. If an error occurs, such as
    when attempting to peek a device opened in WriteOnly mode, this function
    returns -1.

    0 is returned when no more data is available for reading.

    Example:

    \snippet doc/src/snippets/code/src_corelib_io_qiodevice.cpp 4

    \sa read()
*/
qint64 QIODevice::peek(char *data, qint64 maxSize)
{
    return d_func()->peek(data, maxSize);
}

/*!
    \since 4.1
    \overload

    Peeks at most \a maxSize bytes from the device, returning the data peeked
    as a QByteArray.

    Example:

    \snippet doc/src/snippets/code/src_corelib_io_qiodevice.cpp 5

    This function has no way of reporting errors; returning an empty
    QByteArray() can mean either that no data was currently available
    for peeking, or that an error occurred.

    \sa read()
*/
QByteArray QIODevice::peek(qint64 maxSize)
{
    return d_func()->peek(maxSize);
}

/*!
    Blocks until new data is available for reading and the readyRead()
    signal has been emitted, or until \a msecs milliseconds have
    passed. If msecs is -1, this function will not time out.

    Returns true if new data is available for reading; otherwise returns
    false (if the operation timed out or if an error occurred).

    This function can operate without an event loop. It is
    useful when writing non-GUI applications and when performing
    I/O operations in a non-GUI thread.

    If called from within a slot connected to the readyRead() signal,
    readyRead() will not be reemitted.

    Reimplement this function to provide a blocking API for a custom
    device. The default implementation does nothing, and returns false.

    \warning Calling this function from the main (GUI) thread
    might cause your user interface to freeze.

    \sa waitForBytesWritten()
*/
bool QIODevice::waitForReadyRead(int msecs)
{
    Q_UNUSED(msecs);
    return false;
}

/*!
    For buffered devices, this function waits until a payload of
    buffered written data has been written to the device and the
    bytesWritten() signal has been emitted, or until \a msecs
    milliseconds have passed. If msecs is -1, this function will
    not time out. For unbuffered devices, it returns immediately.

    Returns true if a payload of data was written to the device;
    otherwise returns false (i.e. if the operation timed out, or if an
    error occurred).

    This function can operate without an event loop. It is
    useful when writing non-GUI applications and when performing
    I/O operations in a non-GUI thread.

    If called from within a slot connected to the bytesWritten() signal,
    bytesWritten() will not be reemitted.

    Reimplement this function to provide a blocking API for a custom
    device. The default implementation does nothing, and returns false.

    \warning Calling this function from the main (GUI) thread
    might cause your user interface to freeze.

    \sa waitForReadyRead()
*/
bool QIODevice::waitForBytesWritten(int msecs)
{
    Q_UNUSED(msecs);
    return false;
}

/*!
    Sets the human readable description of the last device error that
    occurred to \a str.

    \sa errorString()
*/
void QIODevice::setErrorString(const QString &str)
{
    d_func()->errorString = str;
}

/*!
    Returns a human-readable description of the last device error that
    occurred.

    \sa setErrorString()
*/
QString QIODevice::errorString() const
{
    Q_D(const QIODevice);
    if (d->errorString.isEmpty()) {
#ifdef QT_NO_QOBJECT
        return QLatin1String(QT_TRANSLATE_NOOP(QIODevice, "Unknown error"));
#else
        return tr("Unknown error");
#endif
    }
    return d->errorString;
}

/*!
    \fn qint64 QIODevice::readData(char *data, qint64 maxSize)

    Reads up to \a maxSize bytes from the device into \a data, and
    returns the number of bytes read or -1 if an error occurred.

    If there are no bytes to be read and there can never be more bytes
    available (examples include socket closed, pipe closed, sub-process
    finished), this function returns -1.

    This function is called by QIODevice. Reimplement this function
    when creating a subclass of QIODevice.

    When reimplementing this function it is important that this function
    reads all the required data before returning. This is required in order
    for QDataStream to be able to operate on the class. QDataStream assumes
    all the requested information was read and therefore does not retry reading
    if there was a problem.

    \sa read() readLine() writeData()
*/

/*!
    \fn qint64 QIODevice::writeData(const char *data, qint64 maxSize)

    Writes up to \a maxSize bytes from \a data to the device. Returns
    the number of bytes written, or -1 if an error occurred.

    This function is called by QIODevice. Reimplement this function
    when creating a subclass of QIODevice.

    When reimplementing this function it is important that this function
    writes all the data available before returning. This is required in order
    for QDataStream to be able to operate on the class. QDataStream assumes
    all the information was written and therefore does not retry writing if
    there was a problem.

    \sa read() write()
*/

/*!
    \fn QIODevice::Offset QIODevice::status() const

    For device specific error handling, please refer to the
    individual device documentation.

    \sa qobject_cast()
*/

/*!
    \fn QIODevice::Offset QIODevice::at() const

    Use pos() instead.
*/

/*!
    \fn bool QIODevice::at(Offset offset)

    Use seek(\a offset) instead.
*/

/*! \fn int QIODevice::flags() const

    Use openMode() instead.
*/

/*! \fn int QIODevice::getch()

    Use getChar() instead.
*/

/*!
    \fn bool QIODevice::isAsynchronous() const

    This functionality is no longer available. This function always
    returns true.
*/

/*!
    \fn bool QIODevice::isBuffered() const

    Use !(openMode() & QIODevice::Unbuffered) instead.
*/

/*!
    \fn bool QIODevice::isCombinedAccess() const

    Use openMode() instead.
*/

/*!
    \fn bool QIODevice::isDirectAccess() const

    Use !isSequential() instead.
*/

/*!
    \fn bool QIODevice::isInactive() const

    Use isOpen(), isReadable(), or isWritable() instead.
*/

/*!
    \fn bool QIODevice::isRaw() const

    Use openMode() instead.
*/

/*!
    \fn bool QIODevice::isSequentialAccess() const

    Use isSequential() instead.
*/

/*!
    \fn bool QIODevice::isSynchronous() const

    This functionality is no longer available. This function always
    returns false.
*/

/*!
    \fn bool QIODevice::isTranslated() const

    Use openMode() instead.
*/

/*!
    \fn bool QIODevice::mode() const

    Use openMode() instead.
*/

/*! \fn int QIODevice::putch(int ch)

    Use putChar(\a ch) instead.
*/

/*! \fn int QIODevice::ungetch(int ch)

    Use ungetChar(\a ch) instead.
*/

/*!
    \fn quint64 QIODevice::readBlock(char *data, quint64 size)

    Use read(\a data, \a size) instead.
*/

/*! \fn int QIODevice::state() const

    Use isOpen() instead.
*/

/*!
    \fn qint64 QIODevice::writeBlock(const char *data, quint64 size)

    Use write(\a data, \a size) instead.
*/

/*!
    \fn qint64 QIODevice::writeBlock(const QByteArray &data)

    Use write(\a data) instead.
*/

#if defined QT3_SUPPORT
QIODevice::Status QIODevice::status() const
{
#if !defined(QT_NO_QOBJECT)
    const QFile *f = qobject_cast<const QFile *>(this);
    if (f) return (int) f->error();
#endif
    return isOpen() ? 0 /* IO_Ok */ : 8 /* IO_UnspecifiedError */;
}

/*!
    For device specific error handling, please refer to the
    individual device documentation.

    \sa qobject_cast()
*/
void QIODevice::resetStatus()
{
#if !defined(QT_NO_QOBJECT)
    QFile *f = qobject_cast<QFile *>(this);
    if (f) f->unsetError();
#endif
}
#endif

#if !defined(QT_NO_DEBUG_STREAM)
QDebug operator<<(QDebug debug, QIODevice::OpenMode modes)
{
    debug << "OpenMode(";
    QStringList modeList;
    if (modes == QIODevice::NotOpen) {
        modeList << QLatin1String("NotOpen");
    } else {
        if (modes & QIODevice::ReadOnly)
            modeList << QLatin1String("ReadOnly");
        if (modes & QIODevice::WriteOnly)
            modeList << QLatin1String("WriteOnly");
        if (modes & QIODevice::Append)
            modeList << QLatin1String("Append");
        if (modes & QIODevice::Truncate)
            modeList << QLatin1String("Truncate");
        if (modes & QIODevice::Text)
            modeList << QLatin1String("Text");
        if (modes & QIODevice::Unbuffered)
            modeList << QLatin1String("Unbuffered");
    }
    qSort(modeList);
    debug << modeList.join(QLatin1String("|"));
    debug << ')';
    return debug;
}
#endif

QT_END_NAMESPACE
