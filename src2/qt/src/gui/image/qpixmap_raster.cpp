/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qpixmap.h"

#include <private/qfont_p.h>

#include "qpixmap_raster_p.h"
#include "qnativeimage_p.h"
#include "qimage_p.h"
#include "qpaintengine.h"

#include "qbitmap.h"
#include "qimage.h"
#include <QBuffer>
#include <QImageReader>
#include <private/qimage_p.h>
#include <private/qsimd_p.h>
#include <private/qwidget_p.h>
#include <private/qdrawhelper_p.h>

QT_BEGIN_NAMESPACE

const uchar qt_pixmap_bit_mask[] = { 0x01, 0x02, 0x04, 0x08,
                                     0x10, 0x20, 0x40, 0x80 };

QPixmap qt_toRasterPixmap(const QImage &image)
{
    QPixmapData *data =
        new QRasterPixmapData(image.depth() == 1
                           ? QPixmapData::BitmapType
                           : QPixmapData::PixmapType);

    data->fromImage(image, Qt::AutoColor);

    return QPixmap(data);
}

QPixmap qt_toRasterPixmap(const QPixmap &pixmap)
{
    if (pixmap.isNull())
        return QPixmap();

    if (QPixmap(pixmap).data_ptr()->classId() == QPixmapData::RasterClass)
        return pixmap;

    return qt_toRasterPixmap(pixmap.toImage());
}

QRasterPixmapData::QRasterPixmapData(PixelType type)
    : QPixmapData(type, RasterClass)
{
}

QRasterPixmapData::~QRasterPixmapData()
{
}

QPixmapData *QRasterPixmapData::createCompatiblePixmapData() const
{
    return new QRasterPixmapData(pixelType());
}

void QRasterPixmapData::resize(int width, int height)
{
    QImage::Format format;
#ifdef Q_WS_QWS
    if (pixelType() == BitmapType) {
        format = QImage::Format_Mono;
    } else {
        format = QScreen::instance()->pixelFormat();
        if (format == QImage::Format_Invalid)
            format = QImage::Format_ARGB32_Premultiplied;
        else if (format == QImage::Format_Indexed8) // currently not supported
            format = QImage::Format_RGB444;
    }
#else
    if (pixelType() == BitmapType)
        format = QImage::Format_MonoLSB;
    else
        format = QNativeImage::systemFormat();
#endif

    image = QImage(width, height, format);
    w = width;
    h = height;
    d = image.depth();
    is_null = (w <= 0 || h <= 0);

    if (pixelType() == BitmapType && !image.isNull()) {
        image.setColorCount(2);
        image.setColor(0, QColor(Qt::color0).rgba());
        image.setColor(1, QColor(Qt::color1).rgba());
    }

    setSerialNumber(image.serialNumber());
}

bool QRasterPixmapData::fromData(const uchar *buffer, uint len, const char *format,
                      Qt::ImageConversionFlags flags)
{
    QByteArray a = QByteArray::fromRawData(reinterpret_cast<const char *>(buffer), len);
    QBuffer b(&a);
    b.open(QIODevice::ReadOnly);
    QImage image = QImageReader(&b, format).read();
    if (image.isNull())
        return false;

    createPixmapForImage(image, flags, /* inplace = */true);
    return !isNull();
}

void QRasterPixmapData::fromImage(const QImage &sourceImage,
                                  Qt::ImageConversionFlags flags)
{
    Q_UNUSED(flags);
    QImage image = sourceImage;
    createPixmapForImage(image, flags, /* inplace = */false);
}

void QRasterPixmapData::fromImageReader(QImageReader *imageReader,
                                        Qt::ImageConversionFlags flags)
{
    Q_UNUSED(flags);
    QImage image = imageReader->read();
    if (image.isNull())
        return;

    createPixmapForImage(image, flags, /* inplace = */true);
}

// from qwindowsurface.cpp
extern void qt_scrollRectInImage(QImage &img, const QRect &rect, const QPoint &offset);

void QRasterPixmapData::copy(const QPixmapData *data, const QRect &rect)
{
    fromImage(data->toImage(rect).copy(), Qt::NoOpaqueDetection);
}

bool QRasterPixmapData::scroll(int dx, int dy, const QRect &rect)
{
    if (!image.isNull())
        qt_scrollRectInImage(image, rect, QPoint(dx, dy));
    return true;
}

void QRasterPixmapData::fill(const QColor &color)
{
    uint pixel;

    if (image.depth() == 1) {
        int gray = qGray(color.rgba());
        // Pick the best approximate color in the image's colortable.
        if (qAbs(qGray(image.color(0)) - gray) < qAbs(qGray(image.color(1)) - gray))
            pixel = 0;
        else
            pixel = 1;
    } else if (image.depth() >= 15) {
        int alpha = color.alpha();
        if (alpha != 255) {
            if (!image.hasAlphaChannel()) {
                QImage::Format toFormat;
#if !(defined(QT_HAVE_NEON) || defined(QT_ALWAYS_HAVE_SSE2))
                if (image.format() == QImage::Format_RGB16)
                    toFormat = QImage::Format_ARGB8565_Premultiplied;
                else if (image.format() == QImage::Format_RGB666)
                    toFormat = QImage::Format_ARGB6666_Premultiplied;
                else if (image.format() == QImage::Format_RGB555)
                    toFormat = QImage::Format_ARGB8555_Premultiplied;
                else if (image.format() == QImage::Format_RGB444)
                    toFormat = QImage::Format_ARGB4444_Premultiplied;
                else
#endif
                    toFormat = QImage::Format_ARGB32_Premultiplied;

                if (!image.isNull() && qt_depthForFormat(image.format()) == qt_depthForFormat(toFormat)) {
                    image.detach();
                    image.d->format = toFormat;
                } else {
                    image = QImage(image.width(), image.height(), toFormat);
                }
            }

            switch (image.format()) {
            case QImage::Format_ARGB8565_Premultiplied:
                pixel = qargb8565(color.rgba()).rawValue();
                break;
            case QImage::Format_ARGB8555_Premultiplied:
                pixel = qargb8555(color.rgba()).rawValue();
                break;
            case QImage::Format_ARGB6666_Premultiplied:
                pixel = qargb6666(color.rgba()).rawValue();
                break;
            case QImage::Format_ARGB4444_Premultiplied:
                pixel = qargb4444(color.rgba()).rawValue();
                break;
            default:
                pixel = PREMUL(color.rgba());
                break;
            }
        } else {
            switch (image.format()) {
            case QImage::Format_RGB16:
                pixel = qt_colorConvert<quint16, quint32>(color.rgba(), 0);
                break;
            case QImage::Format_RGB444:
                pixel = qrgb444(color.rgba()).rawValue();
                break;
            case QImage::Format_RGB555:
                pixel = qrgb555(color.rgba()).rawValue();
                break;
            case QImage::Format_RGB666:
                pixel = qrgb666(color.rgba()).rawValue();
                break;
            case QImage::Format_RGB888:
                pixel = qrgb888(color.rgba()).rawValue();
                break;
            default:
                pixel = color.rgba();
                break;
            }
        }
    } else {
        pixel = 0;
        // ### what about 8 bits
    }

    image.fill(pixel);
}

void QRasterPixmapData::setMask(const QBitmap &mask)
{
    if (mask.size().isEmpty()) {
        if (image.depth() != 1) { // hw: ????
            image = image.convertToFormat(QImage::Format_RGB32);
        }
    } else {
        const int w = image.width();
        const int h = image.height();

        switch (image.depth()) {
        case 1: {
            const QImage imageMask = mask.toImage().convertToFormat(image.format());
            for (int y = 0; y < h; ++y) {
                const uchar *mscan = imageMask.scanLine(y);
                uchar *tscan = image.scanLine(y);
                int bytesPerLine = image.bytesPerLine();
                for (int i = 0; i < bytesPerLine; ++i)
                    tscan[i] &= mscan[i];
            }
            break;
        }
        default: {
            const QImage imageMask = mask.toImage().convertToFormat(QImage::Format_MonoLSB);
            image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            for (int y = 0; y < h; ++y) {
                const uchar *mscan = imageMask.scanLine(y);
                QRgb *tscan = (QRgb *)image.scanLine(y);
                for (int x = 0; x < w; ++x) {
                    if (!(mscan[x>>3] & qt_pixmap_bit_mask[x&7]))
                        tscan[x] = 0;
                }
            }
            break;
        }
        }
    }
}

bool QRasterPixmapData::hasAlphaChannel() const
{
    return image.hasAlphaChannel();
}

QImage QRasterPixmapData::toImage() const
{
    if (!image.isNull()) {
        QImageData *data = const_cast<QImage &>(image).data_ptr();
        if (data->paintEngine && data->paintEngine->isActive()
            && data->paintEngine->paintDevice() == &image)
        {
            return image.copy();
        }
    }

    return image;
}

QImage QRasterPixmapData::toImage(const QRect &rect) const
{
    if (rect.isNull())
        return image;

    QRect clipped = rect.intersected(QRect(0, 0, w, h));
    if (d % 8 == 0)
        return QImage(image.scanLine(clipped.y()) + clipped.x() * (d / 8),
                      clipped.width(), clipped.height(),
                      image.bytesPerLine(), image.format());
    else
        return image.copy(clipped);
}

void QRasterPixmapData::setAlphaChannel(const QPixmap &alphaChannel)
{
    image.setAlphaChannel(alphaChannel.toImage());
}

QPaintEngine* QRasterPixmapData::paintEngine() const
{
    return image.paintEngine();
}

int QRasterPixmapData::metric(QPaintDevice::PaintDeviceMetric metric) const
{
    QImageData *d = image.d;
    if (!d)
        return 0;

    // override the image dpi with the screen dpi when rendering to a pixmap
    switch (metric) {
    case QPaintDevice::PdmWidth:
        return w;
    case QPaintDevice::PdmHeight:
        return h;
    case QPaintDevice::PdmWidthMM:
        return qRound(d->width * 25.4 / qt_defaultDpiX());
    case QPaintDevice::PdmHeightMM:
        return qRound(d->height * 25.4 / qt_defaultDpiY());
    case QPaintDevice::PdmNumColors:
        return d->colortable.size();
    case QPaintDevice::PdmDepth:
        return this->d;
    case QPaintDevice::PdmDpiX: // fall-through
    case QPaintDevice::PdmPhysicalDpiX:
        return qt_defaultDpiX();
    case QPaintDevice::PdmDpiY: // fall-through
    case QPaintDevice::PdmPhysicalDpiY:
        return qt_defaultDpiY();
    default:
        qWarning("QRasterPixmapData::metric(): Unhandled metric type %d", metric);
        break;
    }

    return 0;
}

void QRasterPixmapData::createPixmapForImage(QImage &sourceImage, Qt::ImageConversionFlags flags, bool inPlace)
{
    QImage::Format format;
    if (flags & Qt::NoFormatConversion)
        format = sourceImage.format();
    else
#ifdef Q_WS_QWS
    if (pixelType() == BitmapType) {
        format = QImage::Format_Mono;
    } else {
        format = QScreen::instance()->pixelFormat();
        if (format == QImage::Format_Invalid)
            format = QImage::Format_ARGB32_Premultiplied;
        else if (format == QImage::Format_Indexed8) // currently not supported
            format = QImage::Format_RGB444;
    }

    if (sourceImage.hasAlphaChannel()
        && ((flags & Qt::NoOpaqueDetection)
            || const_cast<QImage &>(sourceImage).data_ptr()->checkForAlphaPixels())) {
        switch (format) {
            case QImage::Format_RGB16:
                format = QImage::Format_ARGB8565_Premultiplied;
                break;
            case QImage::Format_RGB666:
                format = QImage::Format_ARGB6666_Premultiplied;
                break;
            case QImage::Format_RGB555:
                format = QImage::Format_ARGB8555_Premultiplied;
                break;
            case QImage::Format_RGB444:
                format = QImage::Format_ARGB4444_Premultiplied;
                break;
            default:
                format = QImage::Format_ARGB32_Premultiplied;
                break;
        }
    } else if (format == QImage::Format_Invalid) {
        format = QImage::Format_ARGB32_Premultiplied;
    }
#else
    if (pixelType() == BitmapType) {
        format = QImage::Format_MonoLSB;
    } else {
        if (sourceImage.depth() == 1) {
            format = sourceImage.hasAlphaChannel()
                    ? QImage::Format_ARGB32_Premultiplied
                    : QImage::Format_RGB32;
        } else {
            QImage::Format opaqueFormat = QNativeImage::systemFormat();
            QImage::Format alphaFormat = QImage::Format_ARGB32_Premultiplied;

#if !defined(QT_HAVE_NEON) && !defined(QT_ALWAYS_HAVE_SSE2)
            switch (opaqueFormat) {
            case QImage::Format_RGB16:
                alphaFormat = QImage::Format_ARGB8565_Premultiplied;
                break;
            default: // We don't care about the others...
                break;
            }
#endif

            if (!sourceImage.hasAlphaChannel()) {
                format = opaqueFormat;
            } else if ((flags & Qt::NoOpaqueDetection) == 0
                       && !const_cast<QImage &>(sourceImage).data_ptr()->checkForAlphaPixels())
            {
                // image has alpha format but is really opaque, so try to do a
                // more efficient conversion
                if (sourceImage.format() == QImage::Format_ARGB32
                    || sourceImage.format() == QImage::Format_ARGB32_Premultiplied)
                {
                    if (!inPlace)
                        sourceImage.detach();
                    sourceImage.d->format = QImage::Format_RGB32;
                }
                format = opaqueFormat;
            } else {
                format = alphaFormat;
            }
        }
    }
#endif

    if (inPlace && sourceImage.d->convertInPlace(format, flags)) {
        image = sourceImage;
    } else {
        image = sourceImage.convertToFormat(format);
    }

    if (image.d) {
        w = image.d->width;
        h = image.d->height;
        d = image.d->depth;
    } else {
        w = h = d = 0;
    }
    is_null = (w <= 0 || h <= 0);

    setSerialNumber(image.serialNumber());
}

QImage* QRasterPixmapData::buffer()
{
    return &image;
}

QT_END_NAMESPACE
