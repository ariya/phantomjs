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

#include "qwindowspipereader_p.h"
#include "qwinoverlappedionotifier_p.h"
#include <qdebug.h>
#include <qelapsedtimer.h>
#include <qeventloop.h>
#include <qtimer.h>

QT_BEGIN_NAMESPACE

QWindowsPipeReader::QWindowsPipeReader(QObject *parent)
    : QObject(parent),
      handle(INVALID_HANDLE_VALUE),
      readBufferMaxSize(0),
      actualReadBufferSize(0),
      readSequenceStarted(false),
      emitReadyReadTimer(new QTimer(this)),
      pipeBroken(false),
      readyReadEmitted(false)
{
    emitReadyReadTimer->setSingleShot(true);
    connect(emitReadyReadTimer, SIGNAL(timeout()), SIGNAL(readyRead()));
    dataReadNotifier = new QWinOverlappedIoNotifier(this);
    connect(dataReadNotifier, &QWinOverlappedIoNotifier::notified, this, &QWindowsPipeReader::notified);
}

static bool qt_cancelIo(HANDLE handle, OVERLAPPED *overlapped)
{
    typedef BOOL (WINAPI *PtrCancelIoEx)(HANDLE, LPOVERLAPPED);
    static PtrCancelIoEx ptrCancelIoEx = 0;
    if (!ptrCancelIoEx) {
        HMODULE kernel32 = GetModuleHandleA("kernel32");
        if (kernel32)
            ptrCancelIoEx = PtrCancelIoEx(GetProcAddress(kernel32, "CancelIoEx"));
    }
    if (ptrCancelIoEx)
        return ptrCancelIoEx(handle, overlapped);
    else
        return CancelIo(handle);
}

QWindowsPipeReader::~QWindowsPipeReader()
{
    if (readSequenceStarted) {
        if (qt_cancelIo(handle, &overlapped))
            dataReadNotifier->waitForNotified(-1, &overlapped);
        else
            qErrnoWarning("QWindowsPipeReader: qt_cancelIo on handle %x failed.", handle);
    }
}

/*!
    Sets the handle to read from. The handle must be valid.
 */
void QWindowsPipeReader::setHandle(HANDLE hPipeReadEnd)
{
    readBuffer.clear();
    actualReadBufferSize = 0;
    handle = hPipeReadEnd;
    ZeroMemory(&overlapped, sizeof(overlapped));
    pipeBroken = false;
    readyReadEmitted = false;
    if (hPipeReadEnd != INVALID_HANDLE_VALUE) {
        dataReadNotifier->setHandle(hPipeReadEnd);
        dataReadNotifier->setEnabled(true);
    }
}

/*!
    Stops the asynchronous read sequence.
    This function assumes that the file already has been closed.
    It does not cancel any I/O operation.
 */
void QWindowsPipeReader::stop()
{
    dataReadNotifier->setEnabled(false);
    readSequenceStarted = false;
    handle = INVALID_HANDLE_VALUE;
}

/*!
    Returns the number of bytes we've read so far.
 */
qint64 QWindowsPipeReader::bytesAvailable() const
{
    return actualReadBufferSize;
}

/*!
    Stops the asynchronous read sequence.
 */
qint64 QWindowsPipeReader::read(char *data, qint64 maxlen)
{
    if (pipeBroken && actualReadBufferSize == 0)
        return -1;  // signal EOF

    qint64 readSoFar;
    // If startAsyncRead() has read data, copy it to its destination.
    if (maxlen == 1 && actualReadBufferSize > 0) {
        *data = readBuffer.getChar();
        actualReadBufferSize--;
        readSoFar = 1;
    } else {
        qint64 bytesToRead = qMin(qint64(actualReadBufferSize), maxlen);
        readSoFar = 0;
        while (readSoFar < bytesToRead) {
            const char *ptr = readBuffer.readPointer();
            int bytesToReadFromThisBlock = qMin(bytesToRead - readSoFar,
                                                qint64(readBuffer.nextDataBlockSize()));
            memcpy(data + readSoFar, ptr, bytesToReadFromThisBlock);
            readSoFar += bytesToReadFromThisBlock;
            readBuffer.free(bytesToReadFromThisBlock);
            actualReadBufferSize -= bytesToReadFromThisBlock;
        }
    }

    if (!pipeBroken) {
        if (!actualReadBufferSize)
            emitReadyReadTimer->stop();
        if (!readSequenceStarted)
            startAsyncRead();
    }

    return readSoFar;
}

bool QWindowsPipeReader::canReadLine() const
{
    return readBuffer.indexOf('\n', actualReadBufferSize) >= 0;
}

/*!
    \internal
    Will be called whenever the read operation completes.
 */
void QWindowsPipeReader::notified(quint32 numberOfBytesRead, quint32 errorCode,
                                  OVERLAPPED *notifiedOverlapped)
{
    if (&overlapped != notifiedOverlapped)
        return;
    if (!completeAsyncRead(numberOfBytesRead, errorCode)) {
        pipeBroken = true;
        emit pipeClosed();
        return;
    }
    startAsyncRead();
    emitReadyReadTimer->stop();
    readyReadEmitted = true;
    emit readyRead();
}

/*!
    \internal
    Reads data from the socket into the readbuffer
 */
void QWindowsPipeReader::startAsyncRead()
{
    const DWORD minReadBufferSize = 4096;
    DWORD bytesToRead = qMax(checkPipeState(), minReadBufferSize);
    if (pipeBroken)
        return;

    if (readBufferMaxSize && bytesToRead > (readBufferMaxSize - readBuffer.size())) {
        bytesToRead = readBufferMaxSize - readBuffer.size();
        if (bytesToRead == 0) {
            // Buffer is full. User must read data from the buffer
            // before we can read more from the pipe.
            return;
        }
    }

    char *ptr = readBuffer.reserve(bytesToRead);

    readSequenceStarted = true;
    if (ReadFile(handle, ptr, bytesToRead, NULL, &overlapped)) {
        // We get notified by the QWinOverlappedIoNotifier - even in the synchronous case.
        return;
    } else {
        const DWORD dwError = GetLastError();
        switch (dwError) {
        case ERROR_IO_PENDING:
            // This is not an error. We're getting notified, when data arrives.
            return;
        case ERROR_MORE_DATA:
            // This is not an error. The synchronous read succeeded.
            // We're connected to a message mode pipe and the message
            // didn't fit into the pipe's system buffer.
            // We're getting notified by the QWinOverlappedIoNotifier.
            break;
        case ERROR_BROKEN_PIPE:
        case ERROR_PIPE_NOT_CONNECTED:
            {
                // It may happen, that the other side closes the connection directly
                // after writing data. Then we must set the appropriate socket state.
                readSequenceStarted = false;
                pipeBroken = true;
                emit pipeClosed();
                return;
            }
        default:
            readSequenceStarted = false;
            emit winError(dwError, QLatin1String("QWindowsPipeReader::startAsyncRead"));
            return;
        }
    }
}

/*!
    \internal
    Sets the correct size of the read buffer after a read operation.
    Returns \c false, if an error occurred or the connection dropped.
 */
bool QWindowsPipeReader::completeAsyncRead(DWORD bytesRead, DWORD errorCode)
{
    readSequenceStarted = false;

    switch (errorCode) {
    case ERROR_SUCCESS:
        break;
    case ERROR_MORE_DATA:
        // This is not an error. We're connected to a message mode
        // pipe and the message didn't fit into the pipe's system
        // buffer. We will read the remaining data in the next call.
        break;
    case ERROR_BROKEN_PIPE:
    case ERROR_PIPE_NOT_CONNECTED:
        return false;
    default:
        emit winError(errorCode, QLatin1String("QWindowsPipeReader::completeAsyncRead"));
        return false;
    }

    actualReadBufferSize += bytesRead;
    readBuffer.truncate(actualReadBufferSize);
    if (!emitReadyReadTimer->isActive())
        emitReadyReadTimer->start();
    return true;
}

/*!
    \internal
    Returns the number of available bytes in the pipe.
    Sets QWindowsPipeReader::pipeBroken to true if the connection is broken.
 */
DWORD QWindowsPipeReader::checkPipeState()
{
    DWORD bytes;
    if (PeekNamedPipe(handle, NULL, 0, NULL, &bytes, NULL)) {
        return bytes;
    } else {
        if (!pipeBroken) {
            pipeBroken = true;
            emit pipeClosed();
        }
    }
    return 0;
}

/*!
    Waits for the completion of the asynchronous read operation.
    Returns \c true, if we've emitted the readyRead signal.
 */
bool QWindowsPipeReader::waitForReadyRead(int msecs)
{
    if (!readSequenceStarted)
        return false;
    readyReadEmitted = false;
    dataReadNotifier->waitForNotified(msecs, &overlapped);
    return readyReadEmitted;
}

/*!
    Waits until the pipe is closed.
 */
bool QWindowsPipeReader::waitForPipeClosed(int msecs)
{
    const int sleepTime = 10;
    QElapsedTimer stopWatch;
    stopWatch.start();
    forever {
        checkPipeState();
        if (pipeBroken)
            return true;
        if (stopWatch.hasExpired(msecs - sleepTime))
            return false;
        Sleep(sleepTime);
    }
}

QT_END_NAMESPACE
