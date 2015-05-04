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

#ifndef QIMAGEIOHANDLER_H
#define QIMAGEIOHANDLER_H

#include <QtCore/qiodevice.h>
#include <QtCore/qplugin.h>
#include <QtCore/qfactoryinterface.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE


class QImage;
class QRect;
class QSize;
class QVariant;

class QImageIOHandlerPrivate;
class Q_GUI_EXPORT QImageIOHandler
{
    Q_DECLARE_PRIVATE(QImageIOHandler)
public:
    QImageIOHandler();
    virtual ~QImageIOHandler();

    void setDevice(QIODevice *device);
    QIODevice *device() const;

    void setFormat(const QByteArray &format);
    void setFormat(const QByteArray &format) const;
    QByteArray format() const;

    virtual QByteArray name() const;

    virtual bool canRead() const = 0;
    virtual bool read(QImage *image) = 0;
    virtual bool write(const QImage &image);

    enum ImageOption {
        Size,
        ClipRect,
        Description,
        ScaledClipRect,
        ScaledSize,
        CompressionRatio,
        Gamma,
        Quality,
        Name,
        SubType,
        IncrementalReading,
        Endianness,
        Animation,
        BackgroundColor,
        ImageFormat,
        SupportedSubTypes
    };
    virtual QVariant option(ImageOption option) const;
    virtual void setOption(ImageOption option, const QVariant &value);
    virtual bool supportsOption(ImageOption option) const;

    // incremental loading
    virtual bool jumpToNextImage();
    virtual bool jumpToImage(int imageNumber);
    virtual int loopCount() const;
    virtual int imageCount() const;
    virtual int nextImageDelay() const;
    virtual int currentImageNumber() const;
    virtual QRect currentImageRect() const;

protected:
    QImageIOHandler(QImageIOHandlerPrivate &dd);
    QScopedPointer<QImageIOHandlerPrivate> d_ptr;
private:
    Q_DISABLE_COPY(QImageIOHandler)
};

#ifndef QT_NO_IMAGEFORMATPLUGIN

#define QImageIOHandlerFactoryInterface_iid "org.qt-project.Qt.QImageIOHandlerFactoryInterface"

class Q_GUI_EXPORT QImageIOPlugin : public QObject
{
    Q_OBJECT
public:
    explicit QImageIOPlugin(QObject *parent = 0);
    virtual ~QImageIOPlugin();

    enum Capability {
        CanRead = 0x1,
        CanWrite = 0x2,
        CanReadIncremental = 0x4
    };
    Q_DECLARE_FLAGS(Capabilities, Capability)

    virtual Capabilities capabilities(QIODevice *device, const QByteArray &format) const = 0;
    virtual QImageIOHandler *create(QIODevice *device, const QByteArray &format = QByteArray()) const = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QImageIOPlugin::Capabilities)

#endif // QT_NO_IMAGEFORMATPLUGIN

QT_END_NAMESPACE

#endif // QIMAGEIOHANDLER_H
