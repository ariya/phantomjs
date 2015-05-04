/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QIMAGEWRITER_H
#define QIMAGEWRITER_H

#include <QtCore/qbytearray.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qlist.h>
#include <QtGui/qimageiohandler.h>

QT_BEGIN_NAMESPACE


class QIODevice;
class QImage;

class QImageWriterPrivate;
class Q_GUI_EXPORT QImageWriter
{
    Q_DECLARE_TR_FUNCTIONS(QImageWriter)
public:
    enum ImageWriterError {
        UnknownError,
        DeviceError,
        UnsupportedFormatError
    };

    QImageWriter();
    explicit QImageWriter(QIODevice *device, const QByteArray &format);
    explicit QImageWriter(const QString &fileName, const QByteArray &format = QByteArray());
    ~QImageWriter();

    void setFormat(const QByteArray &format);
    QByteArray format() const;

    void setDevice(QIODevice *device);
    QIODevice *device() const;

    void setFileName(const QString &fileName);
    QString fileName() const;

    void setQuality(int quality);
    int quality() const;

    void setCompression(int compression);
    int compression() const;

    void setGamma(float gamma);
    float gamma() const;

    void setSubType(const QByteArray &type);
    QByteArray subType() const;
    QList<QByteArray> supportedSubTypes() const;

    // Obsolete as of 4.1
    void setDescription(const QString &description);
    QString description() const;

    void setText(const QString &key, const QString &text);

    bool canWrite() const;
    bool write(const QImage &image);

    ImageWriterError error() const;
    QString errorString() const;

    bool supportsOption(QImageIOHandler::ImageOption option) const;

    static QList<QByteArray> supportedImageFormats();
    static QList<QByteArray> supportedMimeTypes();

private:
    Q_DISABLE_COPY(QImageWriter)
    QImageWriterPrivate *d;
};

QT_END_NAMESPACE

#endif // QIMAGEWRITER_H
