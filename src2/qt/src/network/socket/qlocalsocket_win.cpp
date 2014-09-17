/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#include "qlocalsocket_p.h"

#include <private/qthread_p.h>
#include <qcoreapplication.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

void QLocalSocketPrivate::init()
{
    Q_Q(QLocalSocket);
    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    dataReadNotifier = new QWinEventNotifier(overlapped.hEvent, q);
    q->connect(dataReadNotifier, SIGNAL(activated(HANDLE)), q, SLOT(_q_notified()));
}

void QLocalSocketPrivate::setErrorString(const QString &function)
{
    Q_Q(QLocalSocket);
    BOOL windowsError = GetLastError();
    QLocalSocket::LocalSocketState currentState = state;

    // If the connectToServer fails due to WaitNamedPipe() time-out, assume ConnectionError  
    if (state == QLocalSocket::ConnectingState && windowsError == ERROR_SEM_TIMEOUT)
        windowsError = ERROR_NO_DATA;

    switch (windowsError) {
    case ERROR_PIPE_NOT_CONNECTED:
    case ERROR_BROKEN_PIPE:
    case ERROR_NO_DATA:
        error = QLocalSocket::ConnectionError;
        errorString = QLocalSocket::tr("%1: Connection error").arg(function);
        state = QLocalSocket::UnconnectedState;
        break;
    case ERROR_FILE_NOT_FOUND:
        error = QLocalSocket::ServerNotFoundError;
        errorString = QLocalSocket::tr("%1: Invalid name").arg(function);
        state = QLocalSocket::UnconnectedState;
        break;
    case ERROR_ACCESS_DENIED:
        error = QLocalSocket::SocketAccessError;
        errorString = QLocalSocket::tr("%1: Access denied").arg(function);
        state = QLocalSocket::UnconnectedState;
        break;
    default:
        error = QLocalSocket::UnknownSocketError;
        errorString = QLocalSocket::tr("%1: Unknown error %2").arg(function).arg(windowsError);
#if defined QLOCALSOCKET_DEBUG
        qWarning() << "QLocalSocket error not handled:" << errorString;
#endif
        state = QLocalSocket::UnconnectedState;
    }

    if (currentState != state) {
        q->emit stateChanged(state);
        if (state == QLocalSocket::UnconnectedState)
            q->emit disconnected();
    }
    emit q->error(error);
}

QLocalSocketPrivate::QLocalSocketPrivate() : QIODevicePrivate(),
       handle(INVALID_HANDLE_VALUE),
       pipeWriter(0),
       readBufferMaxSize(0),
       actualReadBufferSize(0),
       error(QLocalSocket::UnknownSocketError),
       readSequenceStarted(false),
       pendingReadyRead(false),
       pipeClosed(false),
       state(QLocalSocket::UnconnectedState)
{
}

QLocalSocketPrivate::~QLocalSocketPrivate()
{
    destroyPipeHandles();
    CloseHandle(overlapped.hEvent);
}

void QLocalSocketPrivate::destroyPipeHandles()
{
    if (handle != INVALID_HANDLE_VALUE) {
        DisconnectNamedPipe(handle);
        CloseHandle(handle);
    }
}

void QLocalSocket::connectToServer(const QString &name, OpenMode openMode)
{
    Q_D(QLocalSocket);
    if (state() == ConnectedState || state() == ConnectingState)
        return;

    d->error = QLocalSocket::UnknownSocketError;
    d->errorString = QString();
    d->state = ConnectingState;
    emit stateChanged(d->state);
    if (name.isEmpty()) {
        d->error = QLocalSocket::ServerNotFoundError;
        setErrorString(QLocalSocket::tr("%1: Invalid name").arg(QLatin1String("QLocalSocket::connectToServer")));
        d->state = UnconnectedState;
        emit error(d->error);
        emit stateChanged(d->state);
        return;
    }

    QString pipePath = QLatin1String("\\\\.\\pipe\\");
    if (name.startsWith(pipePath))
        d->fullServerName = name;
    else
        d->fullServerName = pipePath + name;
    // Try to open a named pipe
    HANDLE localSocket;
    forever {
        DWORD permissions = (openMode & QIODevice::ReadOnly) ? GENERIC_READ : 0;
        permissions |= (openMode & QIODevice::WriteOnly) ? GENERIC_WRITE : 0;
        localSocket = CreateFile((const wchar_t *)d->fullServerName.utf16(),   // pipe name
                                 permissions,
                                 0,              // no sharing
                                 NULL,           // default security attributes
                                 OPEN_EXISTING,  // opens existing pipe
                                 FILE_FLAG_OVERLAPPED,
                                 NULL);          // no template file

        if (localSocket != INVALID_HANDLE_VALUE)
            break;
        DWORD error = GetLastError();
        // It is really an error only if it is not ERROR_PIPE_BUSY
        if (ERROR_PIPE_BUSY != error) {
            break;
        }

        // All pipe instances are busy, so wait until connected or up to 5 seconds.
        if (!WaitNamedPipe((const wchar_t *)d->fullServerName.utf16(), 5000))
            break;
    }

    if (localSocket == INVALID_HANDLE_VALUE) {
        d->setErrorString(QLatin1String("QLocalSocket::connectToServer"));
        d->fullServerName = QString();
        return;
    }

    // we have a valid handle
    d->serverName = name;
    if (setSocketDescriptor((quintptr)localSocket, ConnectedState, openMode)) {
        d->handle = localSocket;
        emit connected();
    }
}

// This is reading from the buffer
qint64 QLocalSocket::readData(char *data, qint64 maxSize)
{
    Q_D(QLocalSocket);

    if (d->pipeClosed && d->actualReadBufferSize == 0)
        return -1;  // signal EOF

    qint64 readSoFar;
    // If startAsyncRead() read data, copy it to its destination.
    if (maxSize == 1 && d->actualReadBufferSize > 0) {
        *data = d->readBuffer.getChar();
        d->actualReadBufferSize--;
        readSoFar = 1;
    } else {
        qint64 bytesToRead = qMin(qint64(d->actualReadBufferSize), maxSize);
        readSoFar = 0;
        while (readSoFar < bytesToRead) {
            const char *ptr = d->readBuffer.readPointer();
            int bytesToReadFromThisBlock = qMin(bytesToRead - readSoFar,
                                                qint64(d->readBuffer.nextDataBlockSize()));
            memcpy(data + readSoFar, ptr, bytesToReadFromThisBlock);
            readSoFar += bytesToReadFromThisBlock;
            d->readBuffer.free(bytesToReadFromThisBlock);
            d->actualReadBufferSize -= bytesToReadFromThisBlock;
        }
    }

    if (d->pipeClosed) {
        if (d->actualReadBufferSize == 0)
            QTimer::singleShot(0, this, SLOT(_q_pipeClosed()));
    } else {
        if (!d->readSequenceStarted)
            d->startAsyncRead();
        d->checkReadyRead();
    }

    return readSoFar;
}

/*!
    \internal
    Schedules or cancels a readyRead() emission depending on actual data availability
 */
void QLocalSocketPrivate::checkReadyRead()
{
    if (actualReadBufferSize > 0) {
        if (!pendingReadyRead) {
            Q_Q(QLocalSocket);
            QTimer::singleShot(0, q, SLOT(_q_emitReadyRead()));
            pendingReadyRead = true;
        }
    } else {
        pendingReadyRead = false;
    }
}

/*!
    \internal
    Reads data from the socket into the readbuffer
 */
void QLocalSocketPrivate::startAsyncRead()
{
    do {
        DWORD bytesToRead = checkPipeState();
        if (pipeClosed)
            return;

        if (bytesToRead == 0) {
            // There are no bytes in the pipe but we need to
            // start the overlapped read with some buffer size.
            bytesToRead = initialReadBufferSize;
        }

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
            completeAsyncRead();
        } else {
            switch (GetLastError()) {
                case ERROR_IO_PENDING:
                    // This is not an error. We're getting notified, when data arrives.
                    return;
                case ERROR_MORE_DATA:
                    // This is not an error. The synchronous read succeeded.
                    // We're connected to a message mode pipe and the message
                    // didn't fit into the pipe's system buffer.
                    completeAsyncRead();
                    break;
                case ERROR_PIPE_NOT_CONNECTED:
                    {
                        // It may happen, that the other side closes the connection directly
                        // after writing data. Then we must set the appropriate socket state.
                        pipeClosed = true;
                        Q_Q(QLocalSocket);
                        emit q->readChannelFinished();
                        return;
                    }
                default:
                    setErrorString(QLatin1String("QLocalSocketPrivate::startAsyncRead"));
                    return;
            }
        }
    } while (!readSequenceStarted);
}

/*!
    \internal
    Sets the correct size of the read buffer after a read operation.
    Returns false, if an error occurred or the connection dropped.
 */
bool QLocalSocketPrivate::completeAsyncRead()
{
    ResetEvent(overlapped.hEvent);
    readSequenceStarted = false;

    DWORD bytesRead;
    if (!GetOverlappedResult(handle, &overlapped, &bytesRead, TRUE)) {
        switch (GetLastError()) {
        case ERROR_MORE_DATA:
            // This is not an error. We're connected to a message mode
            // pipe and the message didn't fit into the pipe's system
            // buffer. We will read the remaining data in the next call.
            break;
        case ERROR_PIPE_NOT_CONNECTED:
            return false;
        default:
            setErrorString(QLatin1String("QLocalSocketPrivate::completeAsyncRead"));
            return false;
        }
    }

    actualReadBufferSize += bytesRead;
    readBuffer.truncate(actualReadBufferSize);
    return true;
}

qint64 QLocalSocket::writeData(const char *data, qint64 maxSize)
{
    Q_D(QLocalSocket);
    if (!d->pipeWriter) {
        d->pipeWriter = new QWindowsPipeWriter(d->handle, this);
        connect(d->pipeWriter, SIGNAL(canWrite()), this, SLOT(_q_canWrite()));
        connect(d->pipeWriter, SIGNAL(bytesWritten(qint64)), this, SIGNAL(bytesWritten(qint64)));
        d->pipeWriter->start();
    }
    return d->pipeWriter->write(data, maxSize);
}

void QLocalSocket::abort()
{
    Q_D(QLocalSocket);
    if (d->pipeWriter) {
        delete d->pipeWriter;
        d->pipeWriter = 0;
    }
    close();
}

/*!
    \internal
    Returns the number of available bytes in the pipe.
    Sets QLocalSocketPrivate::pipeClosed to true if the connection is broken.
 */
DWORD QLocalSocketPrivate::checkPipeState()
{
    Q_Q(QLocalSocket);
    DWORD bytes;
    if (PeekNamedPipe(handle, NULL, 0, NULL, &bytes, NULL)) {
        return bytes;
    } else {
        if (!pipeClosed) {
            pipeClosed = true;
            emit q->readChannelFinished();
            if (actualReadBufferSize == 0)
                QTimer::singleShot(0, q, SLOT(_q_pipeClosed()));
        }
    }
    return 0;
}

void QLocalSocketPrivate::_q_pipeClosed()
{
    Q_Q(QLocalSocket);
    q->close();
}

qint64 QLocalSocket::bytesAvailable() const
{
    Q_D(const QLocalSocket);
    qint64 available = QIODevice::bytesAvailable();
    available += (qint64) d->actualReadBufferSize;
    return available;
}

qint64 QLocalSocket::bytesToWrite() const
{
    Q_D(const QLocalSocket);
    return (d->pipeWriter) ? d->pipeWriter->bytesToWrite() : 0;
}

bool QLocalSocket::canReadLine() const
{
    Q_D(const QLocalSocket);
    if (state() != ConnectedState)
        return false;
    return (QIODevice::canReadLine()
            || d->readBuffer.indexOf('\n', d->actualReadBufferSize) != -1);
}

void QLocalSocket::close()
{
    Q_D(QLocalSocket);
    if (state() == UnconnectedState)
        return;

    QIODevice::close();
    d->state = ClosingState;
    emit stateChanged(d->state);
    if (!d->pipeClosed)
        emit readChannelFinished();
    d->serverName = QString();
    d->fullServerName = QString();

    if (state() != UnconnectedState && bytesToWrite() > 0) {
        disconnectFromServer();
        return;
    }
    d->readSequenceStarted = false;
    d->pendingReadyRead = false;
    d->pipeClosed = false;
    d->destroyPipeHandles();
    d->handle = INVALID_HANDLE_VALUE;
    ResetEvent(d->overlapped.hEvent);
    d->state = UnconnectedState;
    emit stateChanged(d->state);
    emit disconnected();
    if (d->pipeWriter) {
        delete d->pipeWriter;
        d->pipeWriter = 0;
    }
}

bool QLocalSocket::flush()
{
    Q_D(QLocalSocket);
    if (d->pipeWriter)
        return d->pipeWriter->waitForWrite(0);
    return false;
}

void QLocalSocket::disconnectFromServer()
{
    Q_D(QLocalSocket);

    // Are we still connected?
    if (!isValid()) {
        // If we have unwritten data, the pipeWriter is still present.
        // It must be destroyed before close() to prevent an infinite loop.
        delete d->pipeWriter;
        d->pipeWriter = 0;
    }

    flush();
    if (d->pipeWriter && d->pipeWriter->bytesToWrite() != 0) {
        d->state = QLocalSocket::ClosingState;
        emit stateChanged(d->state);
    } else {
        close();
    }
}

QLocalSocket::LocalSocketError QLocalSocket::error() const
{
    Q_D(const QLocalSocket);
    return d->error;
}

bool QLocalSocket::setSocketDescriptor(quintptr socketDescriptor,
              LocalSocketState socketState, OpenMode openMode)
{
    Q_D(QLocalSocket);
    d->readBuffer.clear();
    d->actualReadBufferSize = 0;
    QIODevice::open(openMode);
    d->handle = (int*)socketDescriptor;
    d->state = socketState;
    emit stateChanged(d->state);
    if (d->state == ConnectedState && openMode.testFlag(QIODevice::ReadOnly)) {
        d->startAsyncRead();
        d->checkReadyRead();
    }
    return true;
}

void QLocalSocketPrivate::_q_canWrite()
{
    Q_Q(QLocalSocket);
    if (state == QLocalSocket::ClosingState)
        q->close();
}

void QLocalSocketPrivate::_q_notified()
{
    Q_Q(QLocalSocket);
    if (!completeAsyncRead()) {
        pipeClosed = true;
        emit q->readChannelFinished();
        if (actualReadBufferSize == 0)
            QTimer::singleShot(0, q, SLOT(_q_pipeClosed()));
        return;
    }
    startAsyncRead();
    pendingReadyRead = false;
    emit q->readyRead();
}

void QLocalSocketPrivate::_q_emitReadyRead()
{
    if (pendingReadyRead) {
        Q_Q(QLocalSocket);
        pendingReadyRead = false;
        emit q->readyRead();
    }
}

quintptr QLocalSocket::socketDescriptor() const
{
    Q_D(const QLocalSocket);
    return (quintptr)d->handle;
}

qint64 QLocalSocket::readBufferSize() const
{
    Q_D(const QLocalSocket);
    return d->readBufferMaxSize;
}

void QLocalSocket::setReadBufferSize(qint64 size)
{
    Q_D(QLocalSocket);
    d->readBufferMaxSize = size;
}

bool QLocalSocket::waitForConnected(int msecs)
{
    Q_UNUSED(msecs);
    return (state() == ConnectedState);
}

bool QLocalSocket::waitForDisconnected(int msecs)
{
    Q_D(QLocalSocket);
    if (state() == UnconnectedState)
        return false;
    if (!openMode().testFlag(QIODevice::ReadOnly)) {
        qWarning("QLocalSocket::waitForDisconnected isn't supported for write only pipes.");
        return false;
    }
    QIncrementalSleepTimer timer(msecs);
    forever {
        d->checkPipeState();
        if (d->pipeClosed)
            close();
        if (state() == UnconnectedState)
            return true;
        Sleep(timer.nextSleepTime());
        if (timer.hasTimedOut())
            break;
    }

    return false;
}

bool QLocalSocket::isValid() const
{
    Q_D(const QLocalSocket);
    return d->handle != INVALID_HANDLE_VALUE;
}

bool QLocalSocket::waitForReadyRead(int msecs)
{
    Q_D(QLocalSocket);

    if (bytesAvailable() > 0)
        return true;

    if (d->state != QLocalSocket::ConnectedState)
        return false;

    // We already know that the pipe is gone, but did not enter the event loop yet.
    if (d->pipeClosed) {
        close();
        return false;
    }

    Q_ASSERT(d->readSequenceStarted);
    DWORD result = WaitForSingleObject(d->overlapped.hEvent, msecs == -1 ? INFINITE : msecs);
    switch (result) {
        case WAIT_OBJECT_0:
            d->_q_notified();
            // We just noticed that the pipe is gone.
            if (d->pipeClosed && !bytesAvailable()) {
                close();
                return false;
            }
            return true;
        case WAIT_TIMEOUT:
            return false;
    }

    qWarning("QLocalSocket::waitForReadyRead WaitForSingleObject failed with error code %d.", int(GetLastError()));
    return false;
}

bool QLocalSocket::waitForBytesWritten(int msecs)
{
    Q_D(const QLocalSocket);
    if (!d->pipeWriter)
        return false;

    // Wait for the pipe writer to acknowledge that it has
    // written. This will succeed if either the pipe writer has
    // already written the data, or if it manages to write data
    // within the given timeout.
    return d->pipeWriter->waitForWrite(msecs);
}

QT_END_NAMESPACE
