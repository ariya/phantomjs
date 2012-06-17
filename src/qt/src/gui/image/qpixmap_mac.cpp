/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qpixmap.h"
#include "qimage.h"
#include "qapplication.h"
#include "qbitmap.h"
#include "qmatrix.h"
#include "qtransform.h"
#include "qlibrary.h"
#include "qvarlengtharray.h"
#include "qdebug.h"
#include <private/qdrawhelper_p.h>
#include <private/qpixmap_mac_p.h>
#include <private/qpixmap_raster_p.h>
#include <private/qpaintengine_mac_p.h>
#include <private/qt_mac_p.h>
#include <private/qt_cocoa_helpers_mac_p.h>
#include <private/qapplication_p.h>

#include <limits.h>
#include <string.h>

QT_BEGIN_NAMESPACE

/*****************************************************************************
  Externals
 *****************************************************************************/
extern const uchar *qt_get_bitflip_array(); //qimage.cpp
extern CGContextRef qt_mac_cg_context(const QPaintDevice *pdev); //qpaintdevice_mac.cpp
extern RgnHandle qt_mac_get_rgn(); //qregion_mac.cpp
extern void qt_mac_dispose_rgn(RgnHandle r); //qregion_mac.cpp
extern QRegion qt_mac_convert_mac_region(RgnHandle rgn); //qregion_mac.cpp

static int qt_pixmap_serial = 0;

Q_GUI_EXPORT quint32 *qt_mac_pixmap_get_base(const QPixmap *pix)
{
    if (QApplicationPrivate::graphics_system_name == QLatin1String("raster"))
        return reinterpret_cast<quint32 *>(static_cast<QRasterPixmapData*>(pix->data.data())->buffer()->bits());
    else
        return static_cast<QMacPixmapData*>(pix->data.data())->pixels;
}

Q_GUI_EXPORT int qt_mac_pixmap_get_bytes_per_line(const QPixmap *pix)
{
    if (QApplicationPrivate::graphics_system_name == QLatin1String("raster"))
        return static_cast<QRasterPixmapData*>(pix->data.data())->buffer()->bytesPerLine();
    else
        return static_cast<QMacPixmapData*>(pix->data.data())->bytesPerRow;
}

void qt_mac_cgimage_data_free(void *info, const void *memoryToFree, size_t)
{
    QMacPixmapData *pmdata = static_cast<QMacPixmapData *>(info);
    if (!pmdata) {
        free(const_cast<void *>(memoryToFree));
    } else {
        if (QMacPixmapData::validDataPointers.contains(pmdata) == false) {
            free(const_cast<void *>(memoryToFree));
            // mark data as freed
            if (pmdata->pixels == memoryToFree)
                pmdata->pixels = 0;
            if (pmdata->pixelsToFree == memoryToFree)
                pmdata->pixelsToFree = 0;
            return;
        }
        if (pmdata->pixels == pmdata->pixelsToFree) {
            // something we aren't expecting, just free it.
            Q_ASSERT(memoryToFree != pmdata->pixelsToFree);
            free(const_cast<void *>(memoryToFree));
        } else {
            free(pmdata->pixelsToFree);
            pmdata->pixelsToFree = static_cast<quint32 *>(const_cast<void *>(memoryToFree));
        }
        pmdata->cg_dataBeingReleased = 0;
    }
}

CGImageRef qt_mac_image_to_cgimage(const QImage &image)
{
    int bitsPerColor = 8;
    int bitsPerPixel = 32;
    if (image.depth() == 1) {
        bitsPerColor = 1;
        bitsPerPixel = 1;
    }
    QCFType<CGDataProviderRef> provider =
        CGDataProviderCreateWithData(0, image.bits(), image.bytesPerLine() * image.height(),
                                     0);

    uint cgflags = kCGImageAlphaPremultipliedFirst;
#ifdef kCGBitmapByteOrder32Host //only needed because CGImage.h added symbols in the minor version
    cgflags |= kCGBitmapByteOrder32Host;
#endif

    CGImageRef cgImage = CGImageCreate(image.width(), image.height(), bitsPerColor, bitsPerPixel,
                                       image.bytesPerLine(),
                                       QCoreGraphicsPaintEngine::macGenericColorSpace(),
                                       cgflags, provider,
                                       0,
                                       0,
                                       kCGRenderingIntentDefault);

    return cgImage;
}

/*****************************************************************************
  QPixmap member functions
 *****************************************************************************/

static inline QRgb qt_conv16ToRgb(ushort c) {
    static const int qt_rbits = (565/100);
    static const int qt_gbits = (565/10%10);
    static const int qt_bbits = (565%10);
    static const int qt_red_shift = qt_bbits+qt_gbits-(8-qt_rbits);
    static const int qt_green_shift = qt_bbits-(8-qt_gbits);
    static const int qt_neg_blue_shift = 8-qt_bbits;
    static const int qt_blue_mask = (1<<qt_bbits)-1;
    static const int qt_green_mask = (1<<(qt_gbits+qt_bbits))-((1<<qt_bbits)-1);
    static const int qt_red_mask = (1<<(qt_rbits+qt_gbits+qt_bbits))-(1<<(qt_gbits+qt_bbits));

    const int r=(c & qt_red_mask);
    const int g=(c & qt_green_mask);
    const int b=(c & qt_blue_mask);
    const int tr = r >> qt_red_shift;
    const int tg = g >> qt_green_shift;
    const int tb = b << qt_neg_blue_shift;

    return qRgb(tr,tg,tb);
}

QSet<QMacPixmapData*> QMacPixmapData::validDataPointers;

QMacPixmapData::QMacPixmapData(PixelType type)
    : QPixmapData(type, MacClass), has_alpha(0), has_mask(0),
      uninit(true), pixels(0), pixelsSize(0), pixelsToFree(0),
      bytesPerRow(0), cg_data(0), cg_dataBeingReleased(0), cg_mask(0),
      pengine(0)
{
}

QPixmapData *QMacPixmapData::createCompatiblePixmapData() const
{
    return new QMacPixmapData(pixelType());
}

#define BEST_BYTE_ALIGNMENT 16
#define COMPTUE_BEST_BYTES_PER_ROW(bpr) \
    (((bpr) + (BEST_BYTE_ALIGNMENT - 1)) & ~(BEST_BYTE_ALIGNMENT - 1))

void QMacPixmapData::resize(int width, int height)
{
    setSerialNumber(++qt_pixmap_serial);

    w = width;
    h = height;
    is_null = (w <= 0 || h <= 0);
    d = (pixelType() == BitmapType ? 1 : 32);
    bool make_null = w <= 0 || h <= 0;                // create null pixmap
    if (make_null || d == 0) {
        w = 0;
        h = 0;
        is_null = true;
        d = 0;
        if (!make_null)
            qWarning("Qt: QPixmap: Invalid pixmap parameters");
        return;
    }

    if (w < 1 || h < 1)
        return;

    //create the pixels
    bytesPerRow = w * sizeof(quint32);  // Minimum bytes per row.

    // Quartz2D likes things as a multple of 16 (for now).
    bytesPerRow = COMPTUE_BEST_BYTES_PER_ROW(bytesPerRow);
    macCreatePixels();
}

#undef COMPUTE_BEST_BYTES_PER_ROW

void QMacPixmapData::fromImage(const QImage &img,
                               Qt::ImageConversionFlags flags)
{
    setSerialNumber(++qt_pixmap_serial);

    // the conversion code only handles format >=
    // Format_ARGB32_Premultiplied at the moment..
    if (img.format() > QImage::Format_ARGB32_Premultiplied) {
        QImage image;
        if (img.hasAlphaChannel())
            image = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        else
            image = img.convertToFormat(QImage::Format_RGB32);
        fromImage(image, flags);
        return;
    }

    w = img.width();
    h = img.height();
    is_null = (w <= 0 || h <= 0);
    d = (pixelType() == BitmapType ? 1 : img.depth());

    QImage image = img;
    int dd = QPixmap::defaultDepth();
    bool force_mono = (dd == 1 ||
                       (flags & Qt::ColorMode_Mask)==Qt::MonoOnly);
    if (force_mono) {                         // must be monochrome
        if (d != 1) {
            image = image.convertToFormat(QImage::Format_MonoLSB, flags);  // dither
            d = 1;
        }
    } else {                                    // can be both
        bool conv8 = false;
        if(d > 8 && dd <= 8) {               // convert to 8 bit
            if ((flags & Qt::DitherMode_Mask) == Qt::AutoDither)
                flags = (flags & ~Qt::DitherMode_Mask)
                                   | Qt::PreferDither;
            conv8 = true;
        } else if ((flags & Qt::ColorMode_Mask) == Qt::ColorOnly) {
            conv8 = d == 1;                     // native depth wanted
        } else if (d == 1) {
            if (image.colorCount() == 2) {
                QRgb c0 = image.color(0);       // Auto: convert to best
                QRgb c1 = image.color(1);
                conv8 = qMin(c0,c1) != qRgb(0,0,0) || qMax(c0,c1) != qRgb(255,255,255);
            } else {
                // eg. 1-color monochrome images (they do exist).
                conv8 = true;
            }
        }
        if (conv8) {
            image = image.convertToFormat(QImage::Format_Indexed8, flags);
            d = 8;
        }
    }

    if (image.depth()==1) {
        image.setColor(0, QColor(Qt::color0).rgba());
        image.setColor(1, QColor(Qt::color1).rgba());
    }

    if (d == 16 || d == 24) {
        image = image.convertToFormat(QImage::Format_RGB32, flags);
        fromImage(image, flags);
        return;
    }

    // different size or depth, make a new pixmap
    resize(w, h);

    // exit if resize failed
    if (is_null)
        return;

    quint32 *dptr = pixels, *drow;
    const uint dbpr = bytesPerRow;

    const QImage::Format sfmt = image.format();
    const unsigned short sbpr = image.bytesPerLine();

    // use const_cast to prevent a detach
    const uchar *sptr = const_cast<const QImage &>(image).bits(), *srow;

    for (int y = 0; y < h; ++y) {
        drow = dptr + (y * (dbpr / 4));
        srow = sptr + (y * sbpr);
        switch(sfmt) {
        case QImage::Format_MonoLSB:
        case QImage::Format_Mono:{
            for (int x = 0; x < w; ++x) {
                char one_bit = *(srow + (x / 8));
                if (sfmt == QImage::Format_Mono)
                    one_bit = one_bit >> (7 - (x % 8));
                else
                    one_bit = one_bit >> (x % 8);
                if ((one_bit & 0x01))
                    *(drow+x) = 0xFF000000;
                else
                    *(drow+x) = 0xFFFFFFFF;
            }
            break;
        }
        case QImage::Format_Indexed8: {
            int numColors = image.numColors();
            if (numColors > 0) {
                for (int x = 0; x < w; ++x) {
                    int index = *(srow + x);
                    *(drow+x) = PREMUL(image.color(qMin(index, numColors)));
                }
            }
        } break;
        case QImage::Format_RGB32:
            for (int x = 0; x < w; ++x)
                *(drow+x) = *(((quint32*)srow) + x) | 0xFF000000;
            break;
        case QImage::Format_ARGB32:
        case QImage::Format_ARGB32_Premultiplied:
            for (int x = 0; x < w; ++x) {
                if(sfmt == QImage::Format_RGB32)
                    *(drow+x) = 0xFF000000 | (*(((quint32*)srow) + x) & 0x00FFFFFF);
                else if(sfmt == QImage::Format_ARGB32_Premultiplied)
                    *(drow+x) = *(((quint32*)srow) + x);
                else
                    *(drow+x) = PREMUL(*(((quint32*)srow) + x));
            }
            break;
        default:
            qWarning("Qt: internal: Oops: Forgot a format [%d] %s:%d", sfmt,
                     __FILE__, __LINE__);
            break;
        }
    }
    if (sfmt != QImage::Format_RGB32) { //setup the alpha
        bool alphamap = image.depth() == 32;
        if (sfmt == QImage::Format_Indexed8) {
            const QVector<QRgb> rgb = image.colorTable();
            for (int i = 0, count = image.colorCount(); i < count; ++i) {
                const int alpha = qAlpha(rgb[i]);
                if (alpha != 0xff) {
                    alphamap = true;
                    break;
                }
            }
        }
        macSetHasAlpha(alphamap);
    }
    uninit = false;
}

int get_index(QImage * qi,QRgb mycol)
{
    int loopc;
    for(loopc=0;loopc<qi->colorCount();loopc++) {
        if(qi->color(loopc)==mycol)
            return loopc;
    }
    qi->setColorCount(qi->colorCount()+1);
    qi->setColor(qi->colorCount(),mycol);
    return qi->colorCount();
}

QImage QMacPixmapData::toImage() const
{
    QImage::Format format = QImage::Format_MonoLSB;
    if (d != 1) //Doesn't support index color modes
        format = (has_alpha ? QImage::Format_ARGB32_Premultiplied :
                  QImage::Format_RGB32);

    QImage image(w, h, format);
        // exit if image was not created (out of memory)
    if (image.isNull())
        return image;
    quint32 *sptr = pixels, *srow;
    const uint sbpr = bytesPerRow;
    if (format == QImage::Format_MonoLSB) {
        image.fill(0);
        image.setColorCount(2);
        image.setColor(0, QColor(Qt::color0).rgba());
        image.setColor(1, QColor(Qt::color1).rgba());
        for (int y = 0; y < h; ++y) {
            uchar *scanLine = image.scanLine(y);
            srow = sptr + (y * (sbpr/4));
            for (int x = 0; x < w; ++x) {
                if (!(*(srow + x) & RGB_MASK))
                    scanLine[x >> 3] |= (1 << (x & 7));
            }
        }
    } else {
        for (int y = 0; y < h; ++y) {
            srow = sptr + (y * (sbpr / 4));
            memcpy(image.scanLine(y), srow, w * 4);
        }

    }

    return image;
}

void QMacPixmapData::fill(const QColor &fillColor)

{
    { //we don't know what backend to use so we cannot paint here
        quint32 *dptr = pixels;
        Q_ASSERT_X(dptr, "QPixmap::fill", "No dptr");
        const quint32 colr = PREMUL(fillColor.rgba());
        const int nbytes = bytesPerRow * h;
        if (!colr) {
            memset(dptr, 0, nbytes);
        } else {
            for (uint i = 0; i < nbytes / sizeof(quint32); ++i)
                *(dptr + i) = colr;
        }
    }

    // If we had an alpha channel from before, don't
    // switch it off. Only go from no alpha to alpha:
    if (fillColor.alpha() != 255)
        macSetHasAlpha(true);
}

QPixmap QMacPixmapData::alphaChannel() const
{
    if (!has_alpha)
        return QPixmap();

    QMacPixmapData *alpha = new QMacPixmapData(PixmapType);
    alpha->resize(w, h);
    macGetAlphaChannel(alpha, false);
    return QPixmap(alpha);
}

void QMacPixmapData::setAlphaChannel(const QPixmap &alpha)
{
    has_mask = true;
    QMacPixmapData *alphaData = static_cast<QMacPixmapData*>(alpha.data.data());
    macSetAlphaChannel(alphaData, false);
}

QBitmap QMacPixmapData::mask() const
{
    if (!has_mask && !has_alpha)
        return QBitmap();

    QMacPixmapData *mask = new QMacPixmapData(BitmapType);
    mask->resize(w, h);
    macGetAlphaChannel(mask, true);
    return QPixmap(mask);
}

void QMacPixmapData::setMask(const QBitmap &mask)
{
    if (mask.isNull()) {
        QMacPixmapData opaque(PixmapType);
        opaque.resize(w, h);
        opaque.fill(QColor(255, 255, 255, 255));
        macSetAlphaChannel(&opaque, true);
        has_alpha = has_mask = false;
        return;
    }

    has_alpha = false;
    has_mask = true;
    QMacPixmapData *maskData = static_cast<QMacPixmapData*>(mask.data.data());
    macSetAlphaChannel(maskData, true);
}

int QMacPixmapData::metric(QPaintDevice::PaintDeviceMetric theMetric) const
{
    switch (theMetric) {
    case QPaintDevice::PdmWidth:
        return w;
    case QPaintDevice::PdmHeight:
        return h;
    case QPaintDevice::PdmWidthMM:
        return qRound(metric(QPaintDevice::PdmWidth) * 25.4 / qreal(metric(QPaintDevice::PdmDpiX)));
    case QPaintDevice::PdmHeightMM:
        return qRound(metric(QPaintDevice::PdmHeight) * 25.4 / qreal(metric(QPaintDevice::PdmDpiY)));
    case QPaintDevice::PdmNumColors:
        return 1 << d;
    case QPaintDevice::PdmDpiX:
    case QPaintDevice::PdmPhysicalDpiX: {
        extern float qt_mac_defaultDpi_x(); //qpaintdevice_mac.cpp
        return int(qt_mac_defaultDpi_x());
    }
    case QPaintDevice::PdmDpiY:
    case QPaintDevice::PdmPhysicalDpiY: {
        extern float qt_mac_defaultDpi_y(); //qpaintdevice_mac.cpp
        return int(qt_mac_defaultDpi_y());
    }
    case QPaintDevice::PdmDepth:
        return d;
    default:
        qWarning("QPixmap::metric: Invalid metric command");
    }
    return 0;
}

QMacPixmapData::~QMacPixmapData()
{
    validDataPointers.remove(this);
    if (cg_mask) {
        CGImageRelease(cg_mask);
        cg_mask = 0;
    }

    delete pengine;  // Make sure we aren't drawing on the context anymore.
    if (cg_data) {
        CGImageRelease(cg_data);
    }
    if (pixels && (pixels != pixelsToFree))
        free(pixels);
    if (pixelsToFree)
        free(pixelsToFree);
}

void QMacPixmapData::macSetAlphaChannel(const QMacPixmapData *pix, bool asMask)
{
    if (!pixels || !h || !w || pix->w != w || pix->h != h)
        return;

    quint32 *dptr = pixels, *drow;
    const uint dbpr = bytesPerRow;
    const unsigned short sbpr = pix->bytesPerRow;
    quint32 *sptr = pix->pixels, *srow;
    for (int y=0; y < h; ++y) {
        drow = dptr + (y * (dbpr/4));
        srow = sptr + (y * (sbpr/4));
        if(d == 1) {
            for (int x=0; x < w; ++x) {
                if((*(srow+x) & RGB_MASK))
                    *(drow+x) = 0xFFFFFFFF;
            }
        } else if(d == 8) {
            for (int x=0; x < w; ++x)
                *(drow+x) = (*(drow+x) & RGB_MASK) | (*(srow+x) << 24);
        } else if(asMask) {
            for (int x=0; x < w; ++x) {
                if(*(srow+x) & RGB_MASK)
                    *(drow+x) = (*(drow+x) & RGB_MASK);
                else
                    *(drow+x) = (*(drow+x) & RGB_MASK) | 0xFF000000;
                *(drow+x) = PREMUL(*(drow+x));
            }
        } else {
            for (int x=0; x < w; ++x) {
                const uchar alpha = qGray(qRed(*(srow+x)), qGreen(*(srow+x)), qBlue(*(srow+x)));
                const uchar destAlpha = qt_div_255(alpha * qAlpha(*(drow+x)));
#if 1
                *(drow+x) = (*(drow+x) & RGB_MASK) | (destAlpha << 24);
#else
                *(drow+x) = qRgba(qt_div_255(qRed(*(drow+x) * alpha)),
                                  qt_div_255(qGreen(*(drow+x) * alpha)),
                                  qt_div_255(qBlue(*(drow+x) * alpha)), destAlpha);
#endif
                *(drow+x) = PREMUL(*(drow+x));
            }
        }
    }
    macSetHasAlpha(true);
}

void QMacPixmapData::macGetAlphaChannel(QMacPixmapData *pix, bool asMask) const
{
    quint32 *dptr = pix->pixels, *drow;
    const uint dbpr = pix->bytesPerRow;
    const unsigned short sbpr = bytesPerRow;
    quint32 *sptr = pixels, *srow;
    for(int y=0; y < h; ++y) {
        drow = dptr + (y * (dbpr/4));
        srow = sptr + (y * (sbpr/4));
        if(asMask) {
            for (int x = 0; x < w; ++x) {
                if (*(srow + x) & qRgba(0, 0, 0, 255))
                    *(drow + x) = 0x00000000;
                else
                    *(drow + x) = 0xFFFFFFFF;
            }
        } else {
            for (int x = 0; x < w; ++x) {
                const int alpha = qAlpha(*(srow + x));
                *(drow + x) = qRgb(alpha, alpha, alpha);
            }
        }
    }
}

void QMacPixmapData::macSetHasAlpha(bool b)
{
    has_alpha = b;
    macReleaseCGImageRef();
}

void QMacPixmapData::macCreateCGImageRef()
{
    Q_ASSERT(cg_data == 0);
    //create the cg data
    CGColorSpaceRef colorspace = QCoreGraphicsPaintEngine::macDisplayColorSpace();
    QCFType<CGDataProviderRef> provider = CGDataProviderCreateWithData(this,
                                                              pixels, bytesPerRow * h,
                                                              qt_mac_cgimage_data_free);
    validDataPointers.insert(this);
    uint cgflags = kCGImageAlphaPremultipliedFirst;
#ifdef kCGBitmapByteOrder32Host //only needed because CGImage.h added symbols in the minor version
    cgflags |= kCGBitmapByteOrder32Host;
#endif
    cg_data = CGImageCreate(w, h, 8, 32, bytesPerRow, colorspace,
                            cgflags, provider, 0, 0, kCGRenderingIntentDefault);
}

void QMacPixmapData::macReleaseCGImageRef()
{
    if (!cg_data)
        return;  // There's nothing we need to do

    cg_dataBeingReleased = cg_data;
    CGImageRelease(cg_data);
    cg_data = 0;

    if (pixels != pixelsToFree) {
        macCreatePixels();
    } else {
        pixelsToFree = 0;
    }
}


// We create our space in memory to paint on here. If we already have existing pixels
// copy them over. This is to preserve the fact that CGImageRef's are immutable.
void QMacPixmapData::macCreatePixels()
{
    int numBytes = bytesPerRow * h;
    quint32 *base_pixels;
    if (pixelsToFree && pixelsToFree != pixels) {
        // Reuse unused block of memory lying around from a previous callback.
        base_pixels = pixelsToFree;
        pixelsToFree = 0;
    } else {
        // We need a block of memory to do stuff with.
        base_pixels = static_cast<quint32 *>(malloc(numBytes));
        if (!base_pixels) {
            qWarning("Failed to allocate memory for pixmap data, setting to NULL");
            numBytes = 0;
            w = 0;
            h = 0;
            is_null = 0;
            bytesPerRow=0;
        }
    }

    // only copy when both buffers exist
    if (pixels && base_pixels)
        memcpy(base_pixels, pixels, qMin(pixelsSize, (uint) numBytes));

    // free pixels before assigning new memory
    if (pixels)
        free(pixels);

    pixels = base_pixels;
    pixelsSize = numBytes;
}

#if 0
QPixmap QMacPixmapData::transformed(const QTransform &transform,
                                    Qt::TransformationMode mode) const
{
    int w, h;  // size of target pixmap
    const int ws = width();
    const int hs = height();

    QTransform mat(transform.m11(), transform.m12(),
                   transform.m21(), transform.m22(), 0., 0.);
    if (transform.m12() == 0.0F  && transform.m21() == 0.0F &&
        transform.m11() >= 0.0F  && transform.m22() >= 0.0F)
    {
        h = int(qAbs(mat.m22()) * hs + 0.9999);
        w = int(qAbs(mat.m11()) * ws + 0.9999);
        h = qAbs(h);
        w = qAbs(w);
    } else { // rotation or shearing
        QPolygonF a(QRectF(0,0,ws+1,hs+1));
        a = mat.map(a);
        QRectF r = a.boundingRect().normalized();
        w = int(r.width() + 0.9999);
        h = int(r.height() + 0.9999);
    }
    mat = QPixmap::trueMatrix(mat, ws, hs);
    if (!h || !w)
        return QPixmap();

    // create destination
    QMacPixmapData *pm = new QMacPixmapData(pixelType(), w, h);
    const quint32 *sptr = pixels;
    quint32 *dptr = pm->pixels;
    memset(dptr, 0, (pm->bytesPerRow * pm->h));

    // do the transform
    if (mode == Qt::SmoothTransformation) {
#warning QMacPixmapData::transformed not properly implemented
        qWarning("QMacPixmapData::transformed not properly implemented");
#if 0
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.setTransform(mat);
        p.drawPixmap(0, 0, *this);
#endif
    } else {
        bool invertible;
        mat = mat.inverted(&invertible);
        if (!invertible)
            return QPixmap();

        const int bpp = 32;
        const int xbpl = (w * bpp) / 8;
        if (!qt_xForm_helper(mat, 0, QT_XFORM_TYPE_MSBFIRST, bpp,
                             (uchar*)dptr, xbpl, (pm->bytesPerRow) - xbpl,
                             h, (uchar*)sptr, (bytesPerRow), ws, hs)) {
            qWarning("QMacPixmapData::transform(): failure");
            return QPixmap();
        }
    }

    // update the alpha
    pm->macSetHasAlpha(true);
    return QPixmap(pm);
}
#endif

QT_BEGIN_INCLUDE_NAMESPACE
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
QT_END_INCLUDE_NAMESPACE

// Load and resolve the symbols we need from OpenGL manually so QtGui doesn't have to link against the OpenGL framework.
typedef CGLError (*PtrCGLChoosePixelFormat)(const CGLPixelFormatAttribute *, CGLPixelFormatObj *,  long *);
typedef CGLError (*PtrCGLClearDrawable)(CGLContextObj);
typedef CGLError (*PtrCGLCreateContext)(CGLPixelFormatObj, CGLContextObj, CGLContextObj *);
typedef CGLError (*PtrCGLDestroyContext)(CGLContextObj);
typedef CGLError (*PtrCGLDestroyPixelFormat)(CGLPixelFormatObj);
typedef CGLError (*PtrCGLSetCurrentContext)(CGLContextObj);
typedef CGLError (*PtrCGLSetFullScreen)(CGLContextObj);
typedef void (*PtrglFinish)();
typedef void (*PtrglPixelStorei)(GLenum, GLint);
typedef void (*PtrglReadBuffer)(GLenum);
typedef void (*PtrglReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *);

static PtrCGLChoosePixelFormat ptrCGLChoosePixelFormat = 0;
static PtrCGLClearDrawable ptrCGLClearDrawable = 0;
static PtrCGLCreateContext ptrCGLCreateContext = 0;
static PtrCGLDestroyContext ptrCGLDestroyContext = 0;
static PtrCGLDestroyPixelFormat ptrCGLDestroyPixelFormat = 0;
static PtrCGLSetCurrentContext ptrCGLSetCurrentContext = 0;
static PtrCGLSetFullScreen ptrCGLSetFullScreen = 0;
static PtrglFinish ptrglFinish = 0;
static PtrglPixelStorei ptrglPixelStorei = 0;
static PtrglReadBuffer ptrglReadBuffer = 0;
static PtrglReadPixels ptrglReadPixels = 0;

static bool resolveOpenGLSymbols()
{
    static bool triedResolve = false;
    if (!triedResolve) {
        QLibrary library(QLatin1String("/System/Library/Frameworks/OpenGL.framework/OpenGL"));
        ptrCGLChoosePixelFormat = (PtrCGLChoosePixelFormat)(library.resolve("CGLChoosePixelFormat"));
        ptrCGLClearDrawable = (PtrCGLClearDrawable)(library.resolve("CGLClearDrawable"));
        ptrCGLCreateContext = (PtrCGLCreateContext)(library.resolve("CGLCreateContext"));
        ptrCGLDestroyContext = (PtrCGLDestroyContext)(library.resolve("CGLDestroyContext"));
        ptrCGLDestroyPixelFormat = (PtrCGLDestroyPixelFormat)(library.resolve("CGLDestroyPixelFormat"));
        ptrCGLSetCurrentContext = (PtrCGLSetCurrentContext)(library.resolve("CGLSetCurrentContext"));
        ptrCGLSetFullScreen = (PtrCGLSetFullScreen)(library.resolve("CGLSetFullScreen"));
        ptrglFinish = (PtrglFinish)(library.resolve("glFinish"));
        ptrglPixelStorei = (PtrglPixelStorei)(library.resolve("glPixelStorei"));
        ptrglReadBuffer = (PtrglReadBuffer)(library.resolve("glReadBuffer"));
        ptrglReadPixels = (PtrglReadPixels)(library.resolve("glReadPixels"));
        triedResolve = true;
    }
    return ptrCGLChoosePixelFormat && ptrCGLClearDrawable && ptrCGLCreateContext
        && ptrCGLDestroyContext && ptrCGLDestroyPixelFormat && ptrCGLSetCurrentContext
        && ptrCGLSetFullScreen && ptrglFinish && ptrglPixelStorei
        && ptrglReadBuffer && ptrglReadPixels;
}

// Inverts the given pixmap in the y direction.
static void qt_mac_flipPixmap(void *data, int rowBytes, int height)
{
    int bottom = height - 1;
    void *base = data;
    void *buffer = malloc(rowBytes);

    int top = 0;
    while ( top < bottom )
    {
        void *topP = (void *)((top * rowBytes) + (intptr_t)base);
        void *bottomP = (void *)((bottom * rowBytes) + (intptr_t)base);

        bcopy( topP, buffer, rowBytes );
        bcopy( bottomP, topP, rowBytes );
        bcopy( buffer, bottomP, rowBytes );

        ++top;
        --bottom;
    }
    free(buffer);
}

// Grabs displayRect from display and places it into buffer.
static void qt_mac_grabDisplayRect(CGDirectDisplayID display, const QRect &displayRect, void *buffer)
{
    if (display == kCGNullDirectDisplay)
        return;

    CGLPixelFormatAttribute attribs[] = {
        kCGLPFAFullScreen,
        kCGLPFADisplayMask,
        (CGLPixelFormatAttribute)0,    /* Display mask bit goes here */
        (CGLPixelFormatAttribute)0
    };

    attribs[2] = (CGLPixelFormatAttribute)CGDisplayIDToOpenGLDisplayMask(display);

    // Build a full-screen GL context
    CGLPixelFormatObj pixelFormatObj;
    long numPixelFormats;

    ptrCGLChoosePixelFormat( attribs, &pixelFormatObj, &numPixelFormats );

    if (!pixelFormatObj)    // No full screen context support
        return;

    CGLContextObj glContextObj;
    ptrCGLCreateContext(pixelFormatObj, 0, &glContextObj);
    ptrCGLDestroyPixelFormat(pixelFormatObj) ;
    if (!glContextObj)
        return;

    ptrCGLSetCurrentContext(glContextObj);
    ptrCGLSetFullScreen(glContextObj) ;

    ptrglReadBuffer(GL_FRONT);

    ptrglFinish(); // Finish all OpenGL commands
    ptrglPixelStorei(GL_PACK_ALIGNMENT, 4);  // Force 4-byte alignment
    ptrglPixelStorei(GL_PACK_ROW_LENGTH, 0);
    ptrglPixelStorei(GL_PACK_SKIP_ROWS, 0);
    ptrglPixelStorei(GL_PACK_SKIP_PIXELS, 0);

    // Fetch the data in XRGB format, matching the bitmap context.
    ptrglReadPixels(GLint(displayRect.x()), GLint(displayRect.y()),
                    GLint(displayRect.width()), GLint(displayRect.height()),
#ifdef __BIG_ENDIAN__
                    GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer
#else
                    GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, buffer
#endif
        );

    ptrCGLSetCurrentContext(0);
    ptrCGLClearDrawable(glContextObj); // disassociate from full screen
    ptrCGLDestroyContext(glContextObj); // and destroy the context
}

#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6)
// Returns a pixmap containing the screen contents at rect.
static QPixmap qt_mac_grabScreenRect_10_6(const QRect &rect)
{
    const int maxDisplays = 128; // 128 displays should be enough for everyone.
    CGDirectDisplayID displays[maxDisplays];
    CGDisplayCount displayCount;
    const CGRect cgRect = CGRectMake(rect.x(), rect.y(), rect.width(), rect.height());
    const CGDisplayErr err = CGGetDisplaysWithRect(cgRect, maxDisplays, displays, &displayCount);

    if (err && displayCount == 0)
        return QPixmap();
    QPixmap windowPixmap(rect.size());
    for (uint i = 0; i < displayCount; ++i) {
        const CGRect bounds = CGDisplayBounds(displays[i]);
        // Translate to display-local coordinates
        QRect displayRect = rect.translated(qRound(-bounds.origin.x), qRound(-bounds.origin.y));
        QCFType<CGImageRef> image = CGDisplayCreateImageForRect(displays[i],
            CGRectMake(displayRect.x(), displayRect.y(), displayRect.width(), displayRect.height()));
        QPixmap pix = QPixmap::fromMacCGImageRef(image);
        QPainter painter(&windowPixmap);
        painter.drawPixmap(-bounds.origin.x, -bounds.origin.y, pix);
    }
    return windowPixmap;
}
#endif

static QPixmap qt_mac_grabScreenRect(const QRect &rect)
{
    if (!resolveOpenGLSymbols())
        return QPixmap();

    const int maxDisplays = 128; // 128 displays should be enough for everyone.
    CGDirectDisplayID displays[maxDisplays];
    CGDisplayCount displayCount;
    const CGRect cgRect = CGRectMake(rect.x(), rect.y(), rect.width(), rect.height());
    const CGDisplayErr err = CGGetDisplaysWithRect(cgRect, maxDisplays, displays, &displayCount);

    if (err && displayCount == 0)
        return QPixmap();

    long bytewidth = rect.width() * 4; // Assume 4 bytes/pixel for now
    bytewidth = (bytewidth + 3) & ~3; // Align to 4 bytes
    QVarLengthArray<char> buffer(rect.height() * bytewidth);

    for (uint i = 0; i < displayCount; ++i) {
        const CGRect bounds = CGDisplayBounds(displays[i]);
        // Translate to display-local coordinates
        QRect displayRect = rect.translated(qRound(-bounds.origin.x), qRound(-bounds.origin.y));
        // Adjust for inverted y axis.
        displayRect.moveTop(qRound(bounds.size.height) - displayRect.y() - rect.height());
        qt_mac_grabDisplayRect(displays[i], displayRect, buffer.data());
    }

    qt_mac_flipPixmap(buffer.data(), bytewidth, rect.height());
    QCFType<CGContextRef> bitmap = CGBitmapContextCreate(buffer.data(), rect.width(),
                                                         rect.height(), 8, bytewidth,
                                        QCoreGraphicsPaintEngine::macGenericColorSpace(),
                                        kCGImageAlphaNoneSkipFirst);
    QCFType<CGImageRef> image = CGBitmapContextCreateImage(bitmap);
    return QPixmap::fromMacCGImageRef(image);
}

#ifndef QT_MAC_USE_COCOA // no QuickDraw in 64-bit mode
static QPixmap qt_mac_grabScreenRect_10_3(int x, int y, int w, int h, QWidget *widget)
{
    QPixmap pm = QPixmap(w, h);
    extern WindowPtr qt_mac_window_for(const QWidget *); // qwidget_mac.cpp
    const BitMap *windowPort = 0;
    if((widget->windowType() == Qt::Desktop)) {
        GDHandle gdh;
          if(!(gdh=GetMainDevice()))
              qDebug("Qt: internal: Unexpected condition reached: %s:%d", __FILE__, __LINE__);
          windowPort = (BitMap*)(*(*gdh)->gdPMap);
    } else {
        windowPort = GetPortBitMapForCopyBits(GetWindowPort(qt_mac_window_for(widget)));
    }
    const BitMap *pixmapPort = GetPortBitMapForCopyBits(static_cast<GWorldPtr>(pm.macQDHandle()));
    Rect macSrcRect, macDstRect;
    SetRect(&macSrcRect, x, y, x + w, y + h);
    SetRect(&macDstRect, 0, 0, w, h);
    CopyBits(windowPort, pixmapPort, &macSrcRect, &macDstRect, srcCopy, 0);
    return pm;
}
#endif

QPixmap QPixmap::grabWindow(WId window, int x, int y, int w, int h)
{
    QWidget *widget = QWidget::find(window);
    if (widget == 0)
        return QPixmap();

    if(w == -1)
        w = widget->width() - x;
    if(h == -1)
        h = widget->height() - y;

    QPoint globalCoord(0, 0);
    globalCoord = widget->mapToGlobal(globalCoord);
    QRect rect(globalCoord.x() + x, globalCoord.y() + y, w, h);

#ifdef QT_MAC_USE_COCOA
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6)
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_6)
        return qt_mac_grabScreenRect_10_6(rect);
    else
#endif
        return qt_mac_grabScreenRect(rect);
#else
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4)
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_4) {
        return qt_mac_grabScreenRect(rect);
    } else
#endif
   {
        return qt_mac_grabScreenRect_10_3(x, y, w, h, widget);
   }
#endif // ifdef Q_WS_MAC64
}

/*! \internal

    Returns the QuickDraw CGrafPtr of the pixmap. 0 is returned if it can't
    be obtained. Do not hold the pointer around for long as it can be
    relocated.

    \warning This function is only available on Mac OS X.
    \warning As of Qt 4.6, this function \e{always} returns zero.
*/

Qt::HANDLE QPixmap::macQDHandle() const
{
    return 0;
}

/*! \internal

    Returns the QuickDraw CGrafPtr of the pixmap's alpha channel. 0 is
    returned if it can't be obtained. Do not hold the pointer around for
    long as it can be relocated.

    \warning This function is only available on Mac OS X.
    \warning As of Qt 4.6, this function \e{always} returns zero.
*/

Qt::HANDLE QPixmap::macQDAlphaHandle() const
{
    return 0;
}

/*! \internal

    Returns the CoreGraphics CGContextRef of the pixmap. 0 is returned if
    it can't be obtained. It is the caller's responsiblity to
    CGContextRelease the context when finished using it.

    \warning This function is only available on Mac OS X.
*/

Qt::HANDLE QPixmap::macCGHandle() const
{
    if (isNull())
        return 0;

    if (data->classId() == QPixmapData::MacClass) {
        QMacPixmapData *d = static_cast<QMacPixmapData *>(data.data());
        if (!d->cg_data)
            d->macCreateCGImageRef();
        CGImageRef ret = d->cg_data;
        CGImageRetain(ret);
        return ret;
    } else if (data->classId() == QPixmapData::RasterClass) {
        return qt_mac_image_to_cgimage(static_cast<QRasterPixmapData *>(data.data())->image);
    }
    return 0;
}

bool QMacPixmapData::hasAlphaChannel() const
{
    return has_alpha;
}

CGImageRef qt_mac_create_imagemask(const QPixmap &pixmap, const QRectF &sr)
{
    QMacPixmapData *px = static_cast<QMacPixmapData*>(pixmap.data.data());
    if (px->cg_mask) {
        if (px->cg_mask_rect == sr) {
            CGImageRetain(px->cg_mask); //reference for the caller
            return px->cg_mask;
        }
        CGImageRelease(px->cg_mask);
        px->cg_mask = 0;
    }

    const int sx = qRound(sr.x()), sy = qRound(sr.y()), sw = qRound(sr.width()), sh = qRound(sr.height());
    const int sbpr = px->bytesPerRow;
    const uint nbytes = sw * sh;
    //  alpha is always 255 for bitmaps, ignore it in this case.
    const quint32 mask = px->depth() == 1 ? 0x00ffffff : 0xffffffff;
    quint8 *dptr = static_cast<quint8 *>(malloc(nbytes));
    quint32 *sptr = px->pixels, *srow;
    for(int y = sy, offset=0; y < sh; ++y) {
        srow = sptr + (y * (sbpr / 4));
        for(int x = sx; x < sw; ++x)
            *(dptr+(offset++)) = (*(srow+x) & mask) ? 255 : 0;
    }
    QCFType<CGDataProviderRef> provider = CGDataProviderCreateWithData(0, dptr, nbytes, qt_mac_cgimage_data_free);
    px->cg_mask = CGImageMaskCreate(sw, sh, 8, 8, nbytes / sh, provider, 0, 0);
    px->cg_mask_rect = sr;
    CGImageRetain(px->cg_mask); //reference for the caller
    return px->cg_mask;
}

#ifndef QT_MAC_USE_COCOA
IconRef qt_mac_create_iconref(const QPixmap &px)
{
    if (px.isNull())
        return 0;

    //create icon
    IconFamilyHandle iconFamily = reinterpret_cast<IconFamilyHandle>(NewHandle(0));
    //create data
    {
        struct {
            OSType mac_type;
            int width, height, depth;
            bool mask;
        } images[] = {
            { kThumbnail32BitData, 128, 128, 32, false },
            { kThumbnail8BitMask, 128, 128, 8, true },
            { 0, 0, 0, 0, false } //end marker
        };
        for(int i = 0; images[i].mac_type; i++) {
            //get QPixmap data
            QImage scaled_px = px.toImage().scaled(images[i].width, images[i].height);

            quint32 *sptr = (quint32 *) scaled_px.bits();
            quint32 *srow;
            uint sbpr = scaled_px.bytesPerLine();

            //get Handle data
            const int dbpr = images[i].width * (images[i].depth/8);
            Handle hdl = NewHandle(dbpr*images[i].height);
            if(!sptr) { //handle null pixmap
                memset((*hdl), '\0', dbpr*images[i].height);
            } else if(images[i].mask) {
                if(images[i].mac_type == kThumbnail8BitMask) {
                    for(int y = 0, hindex = 0; y < images[i].height; ++y) {
                        srow = sptr + (y * (sbpr/4));
                        for(int x = 0; x < images[i].width; ++x)
                            *((*hdl)+(hindex++)) = qAlpha(*(srow+x));
                    }
                }
            } else {
                char *dest = (*hdl);
#if defined(__i386__)
                if(images[i].depth == 32) {
                    for(int y = 0; y < images[i].height; ++y) {
                        uint *source = (uint*)((const uchar*)sptr+(sbpr*y));
                        for(int x = 0; x < images[i].width; ++x, dest += 4)
                            *((uint*)dest) = CFSwapInt32(*(source + x));
                    }
                } else
#endif
                {
                    for(int y = 0; y < images[i].height; ++y)
                        memcpy(dest+(y*dbpr), ((const uchar*)sptr+(sbpr*y)), dbpr);
                }
            }

            //set the family data to the Handle
            OSStatus set = SetIconFamilyData(iconFamily, images[i].mac_type, hdl);
            if(set != noErr)
                qWarning("%s: %d -- Unable to create icon data[%d]!! %ld",
                         __FILE__, __LINE__, i, long(set));
            DisposeHandle(hdl);
        }
    }

    //acquire and cleanup
    IconRef ret;
    static int counter = 0;
    const OSType kQtCreator = 'CUTE';
    RegisterIconRefFromIconFamily(kQtCreator, (OSType)counter, iconFamily, &ret);
    AcquireIconRef(ret);
    UnregisterIconRef(kQtCreator, (OSType)counter);
    DisposeHandle(reinterpret_cast<Handle>(iconFamily));
    counter++;
    return ret;

}
#endif

/*! \internal */
QPaintEngine* QMacPixmapData::paintEngine() const
{
    if (!pengine) {
        QMacPixmapData *that = const_cast<QMacPixmapData*>(this);
        that->pengine = new QCoreGraphicsPaintEngine();
    }
    return pengine;
}

void QMacPixmapData::copy(const QPixmapData *data, const QRect &rect)
{
    if (data->pixelType() == BitmapType) {
        QBitmap::fromImage(toImage().copy(rect));
        return;
    }

    const QMacPixmapData *macData = static_cast<const QMacPixmapData*>(data);

    resize(rect.width(), rect.height());

    has_alpha = macData->has_alpha;
    has_mask = macData->has_mask;
    uninit = false;

    const int x = rect.x();
    const int y = rect.y();
    char *dest = reinterpret_cast<char*>(pixels);

    // only copy data buffer if destination buffer exists (resize might have failed)
    if (dest) {
        const char *src = reinterpret_cast<const char*>(macData->pixels + x) + y * macData->bytesPerRow;
        for (int i = 0; i < h; ++i) {
            memcpy(dest, src, w * 4);
            dest += bytesPerRow;
            src += macData->bytesPerRow;
        }

        has_alpha = macData->has_alpha;
        has_mask = macData->has_mask;
    }
}

bool QMacPixmapData::scroll(int dx, int dy, const QRect &rect)
{
    Q_UNUSED(dx);
    Q_UNUSED(dy);
    Q_UNUSED(rect);
    return false;
}

/*!
    \since 4.2

    Creates a \c CGImageRef equivalent to the QPixmap. Returns the \c CGImageRef handle.

    It is the caller's responsibility to release the \c CGImageRef data
    after use.

    \warning This function is only available on Mac OS X.

    \sa fromMacCGImageRef()
*/
CGImageRef QPixmap::toMacCGImageRef() const
{
    return (CGImageRef)macCGHandle();
}

/*!
    \since 4.2

    Returns a QPixmap that is equivalent to the given \a image.

    \warning This function is only available on Mac OS X.

    \sa toMacCGImageRef(), {QPixmap#Pixmap Conversion}{Pixmap Conversion}
*/
QPixmap QPixmap::fromMacCGImageRef(CGImageRef image)
{
    const size_t w = CGImageGetWidth(image),
                 h = CGImageGetHeight(image);
    QPixmap ret(w, h);
    ret.fill(Qt::transparent);
    CGRect rect = CGRectMake(0, 0, w, h);
    CGContextRef ctx = qt_mac_cg_context(&ret);
    qt_mac_drawCGImage(ctx, &rect, image);
    CGContextRelease(ctx);
    return ret;
}

QT_END_NAMESPACE
