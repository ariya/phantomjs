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

#ifndef QNONCONTIGUOUSBYTEDEVICE_P_H
#define QNONCONTIGUOUSBYTEDEVICE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qbuffer.h>
#include <QtCore/qiodevice.h>
#include <QtCore/QSharedPointer>
#include "private/qringbuffer_p.h"

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QNonContiguousByteDevice : public QObject
{
    Q_OBJECT
public:
    virtual const char* readPointer(qint64 maximumLength, qint64 &len) = 0;
    virtual bool advanceReadPointer(qint64 amount) = 0;
    virtual bool atEnd() = 0;
    virtual bool reset() = 0;
    void disableReset();
    bool isResetDisabled() { return resetDisabled; }
    virtual qint64 size() = 0;

    virtual ~QNonContiguousByteDevice();

protected:
    QNonContiguousByteDevice();


    bool resetDisabled;
Q_SIGNALS:
    void readyRead();
    void readProgress(qint64 current, qint64 total);
};

class Q_CORE_EXPORT QNonContiguousByteDeviceFactory
{
public:
    static QNonContiguousByteDevice* create(QIODevice *device);
    static QNonContiguousByteDevice* create(QByteArray *byteArray);
    static QNonContiguousByteDevice* create(QSharedPointer<QRingBuffer> ringBuffer);
    static QIODevice* wrap(QNonContiguousByteDevice* byteDevice);
};

// the actual implementations
//

class QNonContiguousByteDeviceByteArrayImpl : public QNonContiguousByteDevice
{
public:
    QNonContiguousByteDeviceByteArrayImpl(QByteArray *ba);
    ~QNonContiguousByteDeviceByteArrayImpl();
    const char* readPointer(qint64 maximumLength, qint64 &len);
    bool advanceReadPointer(qint64 amount);
    bool atEnd();
    bool reset();
    qint64 size();
protected:
    QByteArray* byteArray;
    qint64 currentPosition;
};

class QNonContiguousByteDeviceRingBufferImpl : public QNonContiguousByteDevice
{
public:
    QNonContiguousByteDeviceRingBufferImpl(QSharedPointer<QRingBuffer> rb);
    ~QNonContiguousByteDeviceRingBufferImpl();
    const char* readPointer(qint64 maximumLength, qint64 &len);
    bool advanceReadPointer(qint64 amount);
    bool atEnd();
    bool reset();
    qint64 size();
protected:
    QSharedPointer<QRingBuffer> ringBuffer;
    qint64 currentPosition;
};


class QNonContiguousByteDeviceIoDeviceImpl : public QNonContiguousByteDevice
{
    Q_OBJECT
public:
    QNonContiguousByteDeviceIoDeviceImpl(QIODevice *d);
    ~QNonContiguousByteDeviceIoDeviceImpl();
    const char* readPointer(qint64 maximumLength, qint64 &len);
    bool advanceReadPointer(qint64 amount);
    bool atEnd();
    bool reset();
    qint64 size();
protected:
    QIODevice* device;
    QByteArray* currentReadBuffer;
    qint64 currentReadBufferSize;
    qint64 currentReadBufferAmount;
    qint64 currentReadBufferPosition;
    qint64 totalAdvancements;
    bool eof;
    qint64 initialPosition;
};

class QNonContiguousByteDeviceBufferImpl : public QNonContiguousByteDevice
{
    Q_OBJECT
public:
    QNonContiguousByteDeviceBufferImpl(QBuffer *b);
    ~QNonContiguousByteDeviceBufferImpl();
    const char* readPointer(qint64 maximumLength, qint64 &len);
    bool advanceReadPointer(qint64 amount);
    bool atEnd();
    bool reset();
    qint64 size();
protected:
    QBuffer* buffer;
    QByteArray byteArray;
    QNonContiguousByteDeviceByteArrayImpl* arrayImpl;
};

// ... and the reverse thing
class QByteDeviceWrappingIoDevice : public QIODevice
{
public:
    QByteDeviceWrappingIoDevice (QNonContiguousByteDevice *bd);
    ~QByteDeviceWrappingIoDevice ();
    virtual bool isSequential () const;
    virtual bool atEnd () const;
    virtual bool reset ();
    virtual qint64 size () const;
protected:
     virtual qint64 readData ( char * data, qint64 maxSize );
     virtual qint64 writeData ( const char * data, qint64 maxSize );

     QNonContiguousByteDevice *byteDevice;
};

QT_END_NAMESPACE

#endif
