/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QBUFFER_H
#define QBUFFER_H

#include <QtCore/qiodevice.h>
#include <QtCore/qbytearray.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

class QObject;
class QBufferPrivate;

class Q_CORE_EXPORT QBuffer : public QIODevice
{
#ifndef QT_NO_QOBJECT
    Q_OBJECT
#endif

public:
#ifndef QT_NO_QOBJECT
     explicit QBuffer(QObject *parent = 0);
     QBuffer(QByteArray *buf, QObject *parent = 0);
#else
     QBuffer();
     explicit QBuffer(QByteArray *buf);
#endif
    ~QBuffer();

    QByteArray &buffer();
    const QByteArray &buffer() const;
    void setBuffer(QByteArray *a);

    void setData(const QByteArray &data);
    inline void setData(const char *data, int len);
    const QByteArray &data() const;

    bool open(OpenMode openMode);

    void close();
    qint64 size() const;
    qint64 pos() const;
    bool seek(qint64 off);
    bool atEnd() const;
    bool canReadLine() const;

protected:
#ifndef QT_NO_QOBJECT
    void connectNotify(const char*);
    void disconnectNotify(const char*);
#endif
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

private:
    Q_DECLARE_PRIVATE(QBuffer)
    Q_DISABLE_COPY(QBuffer)

    Q_PRIVATE_SLOT(d_func(), void _q_emitSignals())
};

inline void QBuffer::setData(const char *adata, int alen)
{ setData(QByteArray(adata, alen)); }

QT_END_NAMESPACE

QT_END_HEADER

#endif // QBUFFER_H
