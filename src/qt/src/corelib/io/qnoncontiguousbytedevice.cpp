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

#include "qnoncontiguousbytedevice_p.h"
#include <qbuffer.h>
#include <qdebug.h>
#include <qfile.h>

QT_BEGIN_NAMESPACE

/*!
    \class QNonContiguousByteDevice
    \brief A QNonContiguousByteDevice is a representation of a
    file, array or buffer that allows access with a read pointer.
    \since 4.6

    \inmodule QtCore

    The goal of this class is to have a data representation that
    allows us to avoid doing a memcpy as we have to do with QIODevice.

    \sa QNonContiguousByteDeviceFactory

    \internal
*/
/*!
    \fn virtual const char* QNonContiguousByteDevice::readPointer(qint64 maximumLength, qint64 &len)

    Return a byte pointer for at most \a maximumLength bytes of that device.
    if \a maximumLength is -1, the caller does not care about the length and
    the device may return what it desires to.
    The actual number of bytes the pointer is valid for is returned in
    the \a len variable.
    \a len will be -1 if EOF or an error occurs.
    If it was really EOF can then afterwards be checked with atEnd()
    Returns 0 if it is not possible to read at that position.

    \sa atEnd()

    \internal
*/
/*!
    \fn virtual bool QNonContiguousByteDevice::advanceReadPointer(qint64 amount)

     will advance the internal read pointer by \a amount bytes.
     The old readPointer is invalid after this call.

    \sa readPointer()

    \internal
*/
/*!
    \fn virtual bool QNonContiguousByteDevice::atEnd()

     Returns true if everything has been read and the read
     pointer cannot be advanced anymore.

    \sa readPointer(), advanceReadPointer(), reset()

    \internal
*/
/*!
    \fn virtual bool QNonContiguousByteDevice::reset()

    Moves the internal read pointer back to the beginning.
    Returns false if this was not possible.

    \sa atEnd(), disableReset()

    \internal
*/
/*!
    \fn void QNonContiguousByteDevice::disableReset()

    Disable the reset() call, e.g. it will always
    do nothing and return false.

    \sa reset()

    \internal
*/
/*!
    \fn virtual qint64 QNonContiguousByteDevice::size()

    Returns the size of the complete device or -1 if unknown.
    May also return less/more than what can be actually read with readPointer()

    \internal
*/
/*!
    \fn void QNonContiguousByteDevice::readyRead()

    Emitted when there is data available

    \internal
*/
/*!
    \fn void QNonContiguousByteDevice::readProgress(qint64 current, qint64 total)

    Emitted when data has been "read" by advancing the read pointer

    \internal
*/

QNonContiguousByteDevice::QNonContiguousByteDevice() : QObject((QObject*)0), resetDisabled(false)
{
}

QNonContiguousByteDevice::~QNonContiguousByteDevice()
{
}

void QNonContiguousByteDevice::disableReset()
{
    resetDisabled = true;
}

// FIXME we should scrap this whole implementation and instead change the ByteArrayImpl to be able to cope with sub-arrays?
QNonContiguousByteDeviceBufferImpl::QNonContiguousByteDeviceBufferImpl(QBuffer *b) : QNonContiguousByteDevice()
{
    buffer = b;
    byteArray = QByteArray::fromRawData(buffer->buffer().constData() + buffer->pos(), buffer->size() - buffer->pos());
    arrayImpl = new QNonContiguousByteDeviceByteArrayImpl(&byteArray);
    arrayImpl->setParent(this);
    connect(arrayImpl, SIGNAL(readyRead()), SIGNAL(readyRead()));
    connect(arrayImpl, SIGNAL(readProgress(qint64,qint64)), SIGNAL(readProgress(qint64,qint64)));
}

QNonContiguousByteDeviceBufferImpl::~QNonContiguousByteDeviceBufferImpl()
{
}

const char* QNonContiguousByteDeviceBufferImpl::readPointer(qint64 maximumLength, qint64 &len)
{
    return arrayImpl->readPointer(maximumLength, len);
}

bool QNonContiguousByteDeviceBufferImpl::advanceReadPointer(qint64 amount)
{
    return arrayImpl->advanceReadPointer(amount);
}

bool QNonContiguousByteDeviceBufferImpl::atEnd()
{
    return arrayImpl->atEnd();
}

bool QNonContiguousByteDeviceBufferImpl::reset()
{
    if (resetDisabled)
        return false;
    return arrayImpl->reset();
}

qint64 QNonContiguousByteDeviceBufferImpl::size()
{
    return arrayImpl->size();
}

QNonContiguousByteDeviceByteArrayImpl::QNonContiguousByteDeviceByteArrayImpl(QByteArray *ba) : QNonContiguousByteDevice(), currentPosition(0)
{
    byteArray = ba;
}

QNonContiguousByteDeviceByteArrayImpl::~QNonContiguousByteDeviceByteArrayImpl()
{
}

const char* QNonContiguousByteDeviceByteArrayImpl::readPointer(qint64 maximumLength, qint64 &len)
{
    if (atEnd()) {
        len = -1;
        return 0;
    }

    if (maximumLength != -1)
        len = qMin(maximumLength, size() - currentPosition);
    else
        len = size() - currentPosition;

    return byteArray->constData() + currentPosition;
}

bool QNonContiguousByteDeviceByteArrayImpl::advanceReadPointer(qint64 amount)
{
    currentPosition += amount;
    emit readProgress(currentPosition, size());
    return true;
}

bool QNonContiguousByteDeviceByteArrayImpl::atEnd()
{
    return currentPosition >= size();
}

bool QNonContiguousByteDeviceByteArrayImpl::reset()
{
    if (resetDisabled)
        return false;

    currentPosition = 0;
    return true;
}

qint64 QNonContiguousByteDeviceByteArrayImpl::size()
{
    return byteArray->size();
}

QNonContiguousByteDeviceRingBufferImpl::QNonContiguousByteDeviceRingBufferImpl(QSharedPointer<QRingBuffer> rb)
    : QNonContiguousByteDevice(), currentPosition(0)
{
    ringBuffer = rb;
}

QNonContiguousByteDeviceRingBufferImpl::~QNonContiguousByteDeviceRingBufferImpl()
{
}

const char* QNonContiguousByteDeviceRingBufferImpl::readPointer(qint64 maximumLength, qint64 &len)
{
    if (atEnd()) {
        len = -1;
        return 0;
    }

    const char *returnValue = ringBuffer->readPointerAtPosition(currentPosition, len);

    if (maximumLength != -1)
        len = qMin(len, maximumLength);

    return returnValue;
}

bool QNonContiguousByteDeviceRingBufferImpl::advanceReadPointer(qint64 amount)
{
    currentPosition += amount;
    emit readProgress(currentPosition, size());
    return true;
}

bool QNonContiguousByteDeviceRingBufferImpl::atEnd()
{
    return currentPosition >= size();
}

bool QNonContiguousByteDeviceRingBufferImpl::reset()
{
    if (resetDisabled)
        return false;

    currentPosition = 0;
    return true;
}

qint64 QNonContiguousByteDeviceRingBufferImpl::size()
{
    return ringBuffer->size();
}

QNonContiguousByteDeviceIoDeviceImpl::QNonContiguousByteDeviceIoDeviceImpl(QIODevice *d)
    : QNonContiguousByteDevice(),
    currentReadBuffer(0), currentReadBufferSize(16*1024),
    currentReadBufferAmount(0), currentReadBufferPosition(0), totalAdvancements(0),
    eof(false)
{
    device = d;
    initialPosition = d->pos();
    connect(device, SIGNAL(readyRead()), this, SIGNAL(readyRead()), Qt::QueuedConnection);
    connect(device, SIGNAL(readChannelFinished()), this, SIGNAL(readyRead()), Qt::QueuedConnection);
}

QNonContiguousByteDeviceIoDeviceImpl::~QNonContiguousByteDeviceIoDeviceImpl()
{
    delete currentReadBuffer;
}

const char* QNonContiguousByteDeviceIoDeviceImpl::readPointer(qint64 maximumLength, qint64 &len)
{
    if (eof == true) {
        len = -1;
        return 0;
    }

    if (currentReadBuffer == 0)
        currentReadBuffer = new QByteArray(currentReadBufferSize, '\0'); // lazy alloc

    if (maximumLength == -1)
        maximumLength = currentReadBufferSize;

    if (currentReadBufferAmount - currentReadBufferPosition > 0) {
        len = currentReadBufferAmount - currentReadBufferPosition;
        return currentReadBuffer->data() + currentReadBufferPosition;
    }

    qint64 haveRead = device->read(currentReadBuffer->data(), qMin(maximumLength, currentReadBufferSize));

    if ((haveRead == -1) || (haveRead == 0 && device->atEnd() && !device->isSequential())) {
        eof = true;
        len = -1;
        // size was unknown before, emit a readProgress with the final size
        if (size() == -1)
            emit readProgress(totalAdvancements, totalAdvancements);
        return 0;
    }

    currentReadBufferAmount = haveRead;
    currentReadBufferPosition = 0;

    len = haveRead;
    return currentReadBuffer->data();
}

bool QNonContiguousByteDeviceIoDeviceImpl::advanceReadPointer(qint64 amount)
{
    totalAdvancements += amount;

    // normal advancement
    currentReadBufferPosition += amount;

    if (size() == -1)
        emit readProgress(totalAdvancements, totalAdvancements);
    else
        emit readProgress(totalAdvancements, size());

    // advancing over that what has actually been read before
    if (currentReadBufferPosition > currentReadBufferAmount) {
        qint64 i = currentReadBufferPosition - currentReadBufferAmount;
        while (i > 0) {
            if (device->getChar(0) == false) {
                emit readProgress(totalAdvancements - i, size());
                return false; // ### FIXME handle eof
            }
            i--;
        }

        currentReadBufferPosition = 0;
        currentReadBufferAmount = 0;
    }


    return true;
}

bool QNonContiguousByteDeviceIoDeviceImpl::atEnd()
{
    return eof == true;
}

bool QNonContiguousByteDeviceIoDeviceImpl::reset()
{
    if (resetDisabled)
        return false;

    if (device->seek(initialPosition)) {
        eof = false; // assume eof is false, it will be true after a read has been attempted
        totalAdvancements = 0; //reset the progress counter
        return true;
    }

    return false;
}

qint64 QNonContiguousByteDeviceIoDeviceImpl::size()
{
    // note that this is different from the size() implementation of QIODevice!

    if (device->isSequential())
        return -1;

    return device->size() - initialPosition;
}

QByteDeviceWrappingIoDevice::QByteDeviceWrappingIoDevice(QNonContiguousByteDevice *bd) : QIODevice((QObject*)0)
{
    byteDevice = bd;
    connect(bd, SIGNAL(readyRead()), SIGNAL(readyRead()));

    open(ReadOnly);
}

QByteDeviceWrappingIoDevice::~QByteDeviceWrappingIoDevice()
{

}

bool QByteDeviceWrappingIoDevice::isSequential() const
{
    return (byteDevice->size() == -1);
}

bool QByteDeviceWrappingIoDevice::atEnd() const
{
    return byteDevice->atEnd();
}

bool QByteDeviceWrappingIoDevice::reset()
{
    return byteDevice->reset();
}

qint64 QByteDeviceWrappingIoDevice::size() const
{
    if (isSequential())
        return 0;

    return byteDevice->size();
}


qint64 QByteDeviceWrappingIoDevice::readData( char * data, qint64 maxSize)
{
    qint64 len;
    const char *readPointer = byteDevice->readPointer(maxSize, len);
    if (len == -1)
        return -1;

    memcpy(data, readPointer, len);
    byteDevice->advanceReadPointer(len);
    return len;
}

qint64 QByteDeviceWrappingIoDevice::writeData( const char* data, qint64 maxSize)
{
    Q_UNUSED(data);
    Q_UNUSED(maxSize);
    return -1;
}

/*!
    \class QNonContiguousByteDeviceFactory
    \since 4.6

    \inmodule QtCore

    Creates a QNonContiguousByteDevice out of a QIODevice,
    QByteArray etc.

    \sa QNonContiguousByteDevice

    \internal
*/

/*!
    \fn static QNonContiguousByteDevice* QNonContiguousByteDeviceFactory::create(QIODevice *device);

    Create a QNonContiguousByteDevice out of a QIODevice.
    For QFile, QBuffer and all other QIoDevice, sequential or not.

    \internal
*/
QNonContiguousByteDevice* QNonContiguousByteDeviceFactory::create(QIODevice *device)
{
    // shortcut if it is a QBuffer
    if (QBuffer* buffer = qobject_cast<QBuffer*>(device)) {
        return new QNonContiguousByteDeviceBufferImpl(buffer);
    }

    // ### FIXME special case if device is a QFile that supports map()
    // then we can actually deal with the file without using read/peek

    // generic QIODevice
    return new QNonContiguousByteDeviceIoDeviceImpl(device); // FIXME
}

/*!
    Create a QNonContiguousByteDevice out of a QRingBuffer.

    \internal
*/
QNonContiguousByteDevice* QNonContiguousByteDeviceFactory::create(QSharedPointer<QRingBuffer> ringBuffer)
{
    return new QNonContiguousByteDeviceRingBufferImpl(ringBuffer);
}

/*!
    \fn static QNonContiguousByteDevice* QNonContiguousByteDeviceFactory::create(QByteArray *byteArray);

    Create a QNonContiguousByteDevice out of a QByteArray.

    \internal
*/
QNonContiguousByteDevice* QNonContiguousByteDeviceFactory::create(QByteArray *byteArray)
{
    return new QNonContiguousByteDeviceByteArrayImpl(byteArray);
}

/*!
    \fn static QIODevice* QNonContiguousByteDeviceFactory::wrap(QNonContiguousByteDevice* byteDevice);

    Wrap the \a byteDevice (possibly again) into a QIODevice.

    \internal
*/
QIODevice* QNonContiguousByteDeviceFactory::wrap(QNonContiguousByteDevice* byteDevice)
{
    // ### FIXME if it already has been based on QIoDevice, we could that one out again
    // and save some calling

    // needed for FTP backend

    return new QByteDeviceWrappingIoDevice(byteDevice);
}

QT_END_NAMESPACE

