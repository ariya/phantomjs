/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <private/qdrawhelper_p.h>
#include <private/qguiapplication_p.h>
#include <private/qsimd_p.h>

#include <private/qimage_p.h>

QT_BEGIN_NAMESPACE

// table to flip bits
static const uchar bitflip[256] = {
    /*
        open OUT, "| fmt";
        for $i (0..255) {
            print OUT (($i >> 7) & 0x01) | (($i >> 5) & 0x02) |
                      (($i >> 3) & 0x04) | (($i >> 1) & 0x08) |
                      (($i << 7) & 0x80) | (($i << 5) & 0x40) |
                      (($i << 3) & 0x20) | (($i << 1) & 0x10), ", ";
        }
        close OUT;
    */
    0, 128, 64, 192, 32, 160, 96, 224, 16, 144, 80, 208, 48, 176, 112, 240,
    8, 136, 72, 200, 40, 168, 104, 232, 24, 152, 88, 216, 56, 184, 120, 248,
    4, 132, 68, 196, 36, 164, 100, 228, 20, 148, 84, 212, 52, 180, 116, 244,
    12, 140, 76, 204, 44, 172, 108, 236, 28, 156, 92, 220, 60, 188, 124, 252,
    2, 130, 66, 194, 34, 162, 98, 226, 18, 146, 82, 210, 50, 178, 114, 242,
    10, 138, 74, 202, 42, 170, 106, 234, 26, 154, 90, 218, 58, 186, 122, 250,
    6, 134, 70, 198, 38, 166, 102, 230, 22, 150, 86, 214, 54, 182, 118, 246,
    14, 142, 78, 206, 46, 174, 110, 238, 30, 158, 94, 222, 62, 190, 126, 254,
    1, 129, 65, 193, 33, 161, 97, 225, 17, 145, 81, 209, 49, 177, 113, 241,
    9, 137, 73, 201, 41, 169, 105, 233, 25, 153, 89, 217, 57, 185, 121, 249,
    5, 133, 69, 197, 37, 165, 101, 229, 21, 149, 85, 213, 53, 181, 117, 245,
    13, 141, 77, 205, 45, 173, 109, 237, 29, 157, 93, 221, 61, 189, 125, 253,
    3, 131, 67, 195, 35, 163, 99, 227, 19, 147, 83, 211, 51, 179, 115, 243,
    11, 139, 75, 203, 43, 171, 107, 235, 27, 155, 91, 219, 59, 187, 123, 251,
    7, 135, 71, 199, 39, 167, 103, 231, 23, 151, 87, 215, 55, 183, 119, 247,
    15, 143, 79, 207, 47, 175, 111, 239, 31, 159, 95, 223, 63, 191, 127, 255
};

const uchar *qt_get_bitflip_array()
{
    return bitflip;
}

void qGamma_correct_back_to_linear_cs(QImage *image)
{
    const QDrawHelperGammaTables *tables = QGuiApplicationPrivate::instance()->gammaTables();
    if (!tables)
        return;
    const uchar *gamma = tables->qt_pow_rgb_gamma;
    // gamma correct the pixels back to linear color space...
    int h = image->height();
    int w = image->width();

    for (int y=0; y<h; ++y) {
        uint *pixels = (uint *) image->scanLine(y);
        for (int x=0; x<w; ++x) {
            uint p = pixels[x];
            uint r = gamma[qRed(p)];
            uint g = gamma[qGreen(p)];
            uint b = gamma[qBlue(p)];
            pixels[x] = (r << 16) | (g << 8) | b | 0xff000000;
        }
    }
}

/*****************************************************************************
  Internal routines for converting image depth.
 *****************************************************************************/

// Cannot be used with indexed formats.
void convert_generic(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(dest->format > QImage::Format_Indexed8);
    Q_ASSERT(src->format > QImage::Format_Indexed8);
    const int buffer_size = 2048;
    uint buffer[buffer_size];
    const QPixelLayout *srcLayout = &qPixelLayouts[src->format];
    const QPixelLayout *destLayout = &qPixelLayouts[dest->format];
    const uchar *srcData = src->data;
    uchar *destData = dest->data;

    FetchPixelsFunc fetch = qFetchPixels[srcLayout->bpp];
    StorePixelsFunc store = qStorePixels[destLayout->bpp];

    for (int y = 0; y < src->height; ++y) {
        int x = 0;
        while (x < src->width) {
            int l = qMin(src->width - x, buffer_size);
            const uint *ptr = fetch(buffer, srcData, x, l);
            ptr = srcLayout->convertToARGB32PM(buffer, ptr, l, srcLayout, 0);
            ptr = destLayout->convertFromARGB32PM(buffer, ptr, l, destLayout, 0);
            store(destData, ptr, x, l);
            x += l;
        }
        srcData += src->bytes_per_line;
        destData += dest->bytes_per_line;
    }
}

// Cannot be used with indexed formats or between formats with different pixel depths.
bool convert_generic_inplace(QImageData *data, QImage::Format dst_format, Qt::ImageConversionFlags)
{
    Q_ASSERT(dst_format > QImage::Format_Indexed8);
    Q_ASSERT(data->format > QImage::Format_Indexed8);
    if (data->depth != qt_depthForFormat(dst_format))
        return false;

    const int buffer_size = 2048;
    uint buffer[buffer_size];
    const QPixelLayout *srcLayout = &qPixelLayouts[data->format];
    const QPixelLayout *destLayout = &qPixelLayouts[dst_format];

    uchar *srcData = data->data;

    FetchPixelsFunc fetch = qFetchPixels[srcLayout->bpp];
    StorePixelsFunc store = qStorePixels[destLayout->bpp];

    for (int y = 0; y < data->height; ++y) {
        int x = 0;
        while (x < data->width) {
            int l = qMin(data->width - x, buffer_size);
            const uint *ptr = fetch(buffer, srcData, x, l);
            ptr = srcLayout->convertToARGB32PM(buffer, ptr, l, srcLayout, 0);
            ptr = destLayout->convertFromARGB32PM(buffer, ptr, l, destLayout, 0);
            store(srcData, ptr, x, l);
            x += l;
        }
        srcData += data->bytes_per_line;
    }
    data->format = dst_format;
    return true;
}

static void convert_ARGB_to_ARGB_PM(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_ARGB32 || src->format == QImage::Format_RGBA8888);
    Q_ASSERT(dest->format == QImage::Format_ARGB32_Premultiplied || dest->format == QImage::Format_RGBA8888_Premultiplied);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const int src_pad = (src->bytes_per_line >> 2) - src->width;
    const int dest_pad = (dest->bytes_per_line >> 2) - dest->width;
    const QRgb *src_data = (QRgb *) src->data;
    QRgb *dest_data = (QRgb *) dest->data;

    for (int i = 0; i < src->height; ++i) {
        const QRgb *end = src_data + src->width;
        while (src_data < end) {
            *dest_data = qPremultiply(*src_data);
            ++src_data;
            ++dest_data;
        }
        src_data += src_pad;
        dest_data += dest_pad;
    }
}

extern bool convert_ARGB_to_ARGB_PM_inplace_sse2(QImageData *data, Qt::ImageConversionFlags);

#ifndef __SSE2__
static bool convert_ARGB_to_ARGB_PM_inplace(QImageData *data, Qt::ImageConversionFlags)
{
    Q_ASSERT(data->format == QImage::Format_ARGB32);

    const int pad = (data->bytes_per_line >> 2) - data->width;
    QRgb *rgb_data = (QRgb *) data->data;

    for (int i = 0; i < data->height; ++i) {
        const QRgb *end = rgb_data + data->width;
        while (rgb_data < end) {
            *rgb_data = PREMUL(*rgb_data);
            ++rgb_data;
        }
        rgb_data += pad;
    }
    data->format = QImage::Format_ARGB32_Premultiplied;
    return true;
}
#endif

static void convert_ARGB_to_RGBx(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_ARGB32);
    Q_ASSERT(dest->format == QImage::Format_RGBX8888);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const int src_pad = (src->bytes_per_line >> 2) - src->width;
    const int dest_pad = (dest->bytes_per_line >> 2) - dest->width;
    const quint32 *src_data = (quint32 *) src->data;
    quint32 *dest_data = (quint32 *) dest->data;

    for (int i = 0; i < src->height; ++i) {
        const quint32 *end = src_data + src->width;
        while (src_data < end) {
            *dest_data = ARGB2RGBA(0xff000000 | *src_data);
            ++src_data;
            ++dest_data;
        }
        src_data += src_pad;
        dest_data += dest_pad;
    }
}

static void convert_ARGB_to_RGBA(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_ARGB32 || src->format == QImage::Format_ARGB32_Premultiplied);
    Q_ASSERT(dest->format == QImage::Format_RGBA8888 || dest->format == QImage::Format_RGBA8888_Premultiplied);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const int src_pad = (src->bytes_per_line >> 2) - src->width;
    const int dest_pad = (dest->bytes_per_line >> 2) - dest->width;
    const quint32 *src_data = (quint32 *) src->data;
    quint32 *dest_data = (quint32 *) dest->data;

    for (int i = 0; i < src->height; ++i) {
        const quint32 *end = src_data + src->width;
        while (src_data < end) {
            *dest_data = ARGB2RGBA(*src_data);
            ++src_data;
            ++dest_data;
        }
        src_data += src_pad;
        dest_data += dest_pad;
    }
}

static bool convert_ARGB_to_RGBA_inplace(QImageData *data, Qt::ImageConversionFlags)
{
    Q_ASSERT(data->format == QImage::Format_ARGB32 || data->format == QImage::Format_ARGB32_Premultiplied);

    const int pad = (data->bytes_per_line >> 2) - data->width;
    quint32 *rgb_data = (quint32 *) data->data;

    for (int i = 0; i < data->height; ++i) {
        const quint32 *end = rgb_data + data->width;
        while (rgb_data < end) {
            *rgb_data = ARGB2RGBA(*rgb_data);
            ++rgb_data;
        }
        rgb_data += pad;
    }
    if (data->format == QImage::Format_ARGB32)
        data->format = QImage::Format_RGBA8888;
    else
        data->format = QImage::Format_RGBA8888_Premultiplied;
    return true;
}

static void convert_ARGB_to_RGBA_PM(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_ARGB32);
    Q_ASSERT(dest->format == QImage::Format_RGBA8888_Premultiplied);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const int src_pad = (src->bytes_per_line >> 2) - src->width;
    const int dest_pad = (dest->bytes_per_line >> 2) - dest->width;
    const quint32 *src_data = (quint32 *) src->data;
    quint32 *dest_data = (quint32 *) dest->data;

    for (int i = 0; i < src->height; ++i) {
        const quint32 *end = src_data + src->width;
        while (src_data < end) {
            *dest_data = ARGB2RGBA(qPremultiply(*src_data));
            ++src_data;
            ++dest_data;
        }
        src_data += src_pad;
        dest_data += dest_pad;
    }
}

static void convert_RGBA_to_ARGB(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_RGBX8888 || src->format == QImage::Format_RGBA8888 || src->format == QImage::Format_RGBA8888_Premultiplied);
    Q_ASSERT(dest->format == QImage::Format_ARGB32 || dest->format == QImage::Format_ARGB32_Premultiplied);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const int src_pad = (src->bytes_per_line >> 2) - src->width;
    const int dest_pad = (dest->bytes_per_line >> 2) - dest->width;
    const quint32 *src_data = (quint32 *) src->data;
    quint32 *dest_data = (quint32 *) dest->data;

    for (int i = 0; i < src->height; ++i) {
        const quint32 *end = src_data + src->width;
        while (src_data < end) {
            *dest_data = RGBA2ARGB(*src_data);
            ++src_data;
            ++dest_data;
        }
        src_data += src_pad;
        dest_data += dest_pad;
    }
}

static bool convert_RGBA_to_ARGB_inplace(QImageData *data, Qt::ImageConversionFlags)
{
    Q_ASSERT(data->format == QImage::Format_RGBX8888 || data->format == QImage::Format_RGBA8888 || data->format == QImage::Format_RGBA8888_Premultiplied);

    const int pad = (data->bytes_per_line >> 2) - data->width;
    QRgb *rgb_data = (QRgb *) data->data;

    for (int i = 0; i < data->height; ++i) {
        const QRgb *end = rgb_data + data->width;
        while (rgb_data < end) {
            *rgb_data = RGBA2ARGB(*rgb_data);
            ++rgb_data;
        }
        rgb_data += pad;
    }
    if (data->format == QImage::Format_RGBA8888_Premultiplied)
        data->format = QImage::Format_ARGB32_Premultiplied;
    else if (data->format == QImage::Format_RGBX8888)
        data->format = QImage::Format_RGB32;
    else
        data->format = QImage::Format_ARGB32;
    return true;
}

static void convert_RGBA_to_ARGB_PM(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_RGBA8888);
    Q_ASSERT(dest->format == QImage::Format_ARGB32_Premultiplied);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const int src_pad = (src->bytes_per_line >> 2) - src->width;
    const int dest_pad = (dest->bytes_per_line >> 2) - dest->width;
    const quint32 *src_data = (quint32 *) src->data;
    quint32 *dest_data = (quint32 *) dest->data;

    for (int i = 0; i < src->height; ++i) {
        const quint32 *end = src_data + src->width;
        while (src_data < end) {
            *dest_data = qPremultiply(RGBA2ARGB(*src_data));
            ++src_data;
            ++dest_data;
        }
        src_data += src_pad;
        dest_data += dest_pad;
    }
}

static bool convert_RGBA_to_ARGB_PM_inplace(QImageData *data, Qt::ImageConversionFlags)
{
    Q_ASSERT(data->format == QImage::Format_RGBA8888);

    const int pad = (data->bytes_per_line >> 2) - data->width;
    QRgb *rgb_data = (QRgb *) data->data;

    for (int i = 0; i < data->height; ++i) {
        const QRgb *end = rgb_data + data->width;
        while (rgb_data < end) {
            *rgb_data = qPremultiply(RGBA2ARGB(*rgb_data));
            ++rgb_data;
        }
        rgb_data += pad;
    }
    data->format = QImage::Format_ARGB32_Premultiplied;
    return true;
}

static bool convert_indexed8_to_ARGB_PM_inplace(QImageData *data, Qt::ImageConversionFlags)
{
    Q_ASSERT(data->format == QImage::Format_Indexed8);
    const int depth = 32;

    const int dst_bytes_per_line = ((data->width * depth + 31) >> 5) << 2;
    const int nbytes = dst_bytes_per_line * data->height;
    uchar *const newData = (uchar *)realloc(data->data, nbytes);
    if (!newData)
        return false;

    data->data = newData;

    // start converting from the end because the end image is bigger than the source
    uchar *src_data = newData + data->nbytes; // end of src
    quint32 *dest_data = (quint32 *) (newData + nbytes); // end of dest > end of src
    const int width = data->width;
    const int src_pad = data->bytes_per_line - width;
    const int dest_pad = (dst_bytes_per_line >> 2) - width;
    if (data->colortable.size() == 0) {
        data->colortable.resize(256);
        for (int i = 0; i < 256; ++i)
            data->colortable[i] = qRgb(i, i, i);
    } else {
        for (int i = 0; i < data->colortable.size(); ++i)
            data->colortable[i] = qPremultiply(data->colortable.at(i));

        // Fill the rest of the table in case src_data > colortable.size()
        const int oldSize = data->colortable.size();
        const QRgb lastColor = data->colortable.at(oldSize - 1);
        data->colortable.insert(oldSize, 256 - oldSize, lastColor);
    }

    for (int i = 0; i < data->height; ++i) {
        src_data -= src_pad;
        dest_data -= dest_pad;
        for (int pixI = 0; pixI < width; ++pixI) {
            --src_data;
            --dest_data;
            *dest_data = data->colortable.at(*src_data);
        }
    }

    data->colortable = QVector<QRgb>();
    data->format = QImage::Format_ARGB32_Premultiplied;
    data->bytes_per_line = dst_bytes_per_line;
    data->depth = depth;
    data->nbytes = nbytes;

    return true;
}

static bool convert_indexed8_to_RGB_inplace(QImageData *data, Qt::ImageConversionFlags)
{
    Q_ASSERT(data->format == QImage::Format_Indexed8);
    const int depth = 32;

    const int dst_bytes_per_line = ((data->width * depth + 31) >> 5) << 2;
    const int nbytes = dst_bytes_per_line * data->height;
    uchar *const newData = (uchar *)realloc(data->data, nbytes);
    if (!newData)
        return false;

    data->data = newData;

    // start converting from the end because the end image is bigger than the source
    uchar *src_data = newData + data->nbytes;
    quint32 *dest_data = (quint32 *) (newData + nbytes);
    const int width = data->width;
    const int src_pad = data->bytes_per_line - width;
    const int dest_pad = (dst_bytes_per_line >> 2) - width;
    if (data->colortable.size() == 0) {
        data->colortable.resize(256);
        for (int i = 0; i < 256; ++i)
            data->colortable[i] = qRgb(i, i, i);
    } else {
        // Fill the rest of the table in case src_data > colortable.size()
        const int oldSize = data->colortable.size();
        const QRgb lastColor = data->colortable.at(oldSize - 1);
        data->colortable.insert(oldSize, 256 - oldSize, lastColor);
    }

    for (int i = 0; i < data->height; ++i) {
        src_data -= src_pad;
        dest_data -= dest_pad;
        for (int pixI = 0; pixI < width; ++pixI) {
            --src_data;
            --dest_data;
            *dest_data = (quint32) data->colortable.at(*src_data);
        }
    }

    data->colortable = QVector<QRgb>();
    data->format = QImage::Format_RGB32;
    data->bytes_per_line = dst_bytes_per_line;
    data->depth = depth;
    data->nbytes = nbytes;

    return true;
}

static bool convert_indexed8_to_RGB16_inplace(QImageData *data, Qt::ImageConversionFlags)
{
    Q_ASSERT(data->format == QImage::Format_Indexed8);
    const int depth = 16;

    const int dst_bytes_per_line = ((data->width * depth + 31) >> 5) << 2;
    const int nbytes = dst_bytes_per_line * data->height;
    uchar *const newData = (uchar *)realloc(data->data, nbytes);
    if (!newData)
        return false;

    data->data = newData;

    // start converting from the end because the end image is bigger than the source
    uchar *src_data = newData + data->nbytes;
    quint16 *dest_data = (quint16 *) (newData + nbytes);
    const int width = data->width;
    const int src_pad = data->bytes_per_line - width;
    const int dest_pad = (dst_bytes_per_line >> 1) - width;

    quint16 colorTableRGB16[256];
    if (data->colortable.isEmpty()) {
        for (int i = 0; i < 256; ++i)
            colorTableRGB16[i] = qConvertRgb32To16(qRgb(i, i, i));
    } else {
        // 1) convert the existing colors to RGB16
        const int tableSize = data->colortable.size();
        for (int i = 0; i < tableSize; ++i)
            colorTableRGB16[i] = qConvertRgb32To16(data->colortable.at(i));
        data->colortable = QVector<QRgb>();

        // 2) fill the rest of the table in case src_data > colortable.size()
        const quint16 lastColor = colorTableRGB16[tableSize - 1];
        for (int i = tableSize; i < 256; ++i)
            colorTableRGB16[i] = lastColor;
    }

    for (int i = 0; i < data->height; ++i) {
        src_data -= src_pad;
        dest_data -= dest_pad;
        for (int pixI = 0; pixI < width; ++pixI) {
            --src_data;
            --dest_data;
            *dest_data = colorTableRGB16[*src_data];
        }
    }

    data->format = QImage::Format_RGB16;
    data->bytes_per_line = dst_bytes_per_line;
    data->depth = depth;
    data->nbytes = nbytes;

    return true;
}

static bool convert_RGB_to_RGB16_inplace(QImageData *data, Qt::ImageConversionFlags)
{
    Q_ASSERT(data->format == QImage::Format_RGB32);
    const int depth = 16;

    const int dst_bytes_per_line = ((data->width * depth + 31) >> 5) << 2;
    const int src_bytes_per_line = data->bytes_per_line;
    quint32 *src_data = (quint32 *) data->data;
    quint16 *dst_data = (quint16 *) data->data;

    for (int i = 0; i < data->height; ++i) {
        for (int j = 0; j < data->width; ++j)
            dst_data[j] = qConvertRgb32To16(src_data[j]);
        src_data = (quint32 *) (((char*)src_data) + src_bytes_per_line);
        dst_data = (quint16 *) (((char*)dst_data) + dst_bytes_per_line);
    }
    data->format = QImage::Format_RGB16;
    data->bytes_per_line = dst_bytes_per_line;
    data->depth = depth;
    data->nbytes = dst_bytes_per_line * data->height;
    uchar *const newData = (uchar *)realloc(data->data, data->nbytes);
    if (newData) {
        data->data = newData;
        return true;
    } else {
        return false;
    }
}

static void convert_ARGB_PM_to_ARGB(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_ARGB32_Premultiplied || src->format == QImage::Format_RGBA8888_Premultiplied);
    Q_ASSERT(dest->format == QImage::Format_ARGB32 || dest->format == QImage::Format_RGBA8888);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const int src_pad = (src->bytes_per_line >> 2) - src->width;
    const int dest_pad = (dest->bytes_per_line >> 2) - dest->width;
    const QRgb *src_data = (QRgb *) src->data;
    QRgb *dest_data = (QRgb *) dest->data;

    for (int i = 0; i < src->height; ++i) {
        const QRgb *end = src_data + src->width;
        while (src_data < end) {
            *dest_data = qUnpremultiply(*src_data);
            ++src_data;
            ++dest_data;
        }
        src_data += src_pad;
        dest_data += dest_pad;
    }
}

static void convert_ARGB_PM_to_RGB(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_ARGB32_Premultiplied || src->format == QImage::Format_RGBA8888_Premultiplied);
    Q_ASSERT(dest->format == QImage::Format_RGB32 || dest->format == QImage::Format_RGBX8888);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const int src_pad = (src->bytes_per_line >> 2) - src->width;
    const int dest_pad = (dest->bytes_per_line >> 2) - dest->width;
    const QRgb *src_data = (QRgb *) src->data;
    QRgb *dest_data = (QRgb *) dest->data;

    for (int i = 0; i < src->height; ++i) {
        const QRgb *end = src_data + src->width;
        while (src_data < end) {
            *dest_data = 0xff000000 | qUnpremultiply(*src_data);
            ++src_data;
            ++dest_data;
        }
        src_data += src_pad;
        dest_data += dest_pad;
    }
}

static void convert_ARGB_PM_to_RGBx(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_ARGB32_Premultiplied);
    Q_ASSERT(dest->format == QImage::Format_RGBX8888);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const int src_pad = (src->bytes_per_line >> 2) - src->width;
    const int dest_pad = (dest->bytes_per_line >> 2) - dest->width;
    const QRgb *src_data = (QRgb *) src->data;
    QRgb *dest_data = (QRgb *) dest->data;

    for (int i = 0; i < src->height; ++i) {
        const QRgb *end = src_data + src->width;
        while (src_data < end) {
            *dest_data = ARGB2RGBA(0xff000000 | qUnpremultiply(*src_data));
            ++src_data;
            ++dest_data;
        }
        src_data += src_pad;
        dest_data += dest_pad;
    }
}

static void convert_ARGB_PM_to_RGBA(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_ARGB32_Premultiplied);
    Q_ASSERT(dest->format == QImage::Format_RGBA8888);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const int src_pad = (src->bytes_per_line >> 2) - src->width;
    const int dest_pad = (dest->bytes_per_line >> 2) - dest->width;
    const QRgb *src_data = (QRgb *) src->data;
    QRgb *dest_data = (QRgb *) dest->data;

    for (int i = 0; i < src->height; ++i) {
        const QRgb *end = src_data + src->width;
        while (src_data < end) {
            *dest_data = ARGB2RGBA(qUnpremultiply(*src_data));
            ++src_data;
            ++dest_data;
        }
        src_data += src_pad;
        dest_data += dest_pad;
    }
}

static void convert_RGBA_to_RGB(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_RGBA8888 || src->format == QImage::Format_RGBX8888);
    Q_ASSERT(dest->format == QImage::Format_RGB32);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const int src_pad = (src->bytes_per_line >> 2) - src->width;
    const int dest_pad = (dest->bytes_per_line >> 2) - dest->width;
    const uint *src_data = (const uint *)src->data;
    uint *dest_data = (uint *)dest->data;

    for (int i = 0; i < src->height; ++i) {
        const uint *end = src_data + src->width;
        while (src_data < end) {
            *dest_data = RGBA2ARGB(*src_data) | 0xff000000;
            ++src_data;
            ++dest_data;
        }
        src_data += src_pad;
        dest_data += dest_pad;
    }
}

static void convert_RGB_to_RGBA(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_RGB32);
    Q_ASSERT(dest->format == QImage::Format_RGBX8888 || dest->format == QImage::Format_RGBA8888 || dest->format == QImage::Format_RGBA8888_Premultiplied);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const int src_pad = (src->bytes_per_line >> 2) - src->width;
    const int dest_pad = (dest->bytes_per_line >> 2) - dest->width;
    const uint *src_data = (const uint *)src->data;
    uint *dest_data = (uint *)dest->data;

    for (int i = 0; i < src->height; ++i) {
        const uint *end = src_data + src->width;
        while (src_data < end) {
            *dest_data = ARGB2RGBA(*src_data | 0xff000000);
            ++src_data;
            ++dest_data;
        }
        src_data += src_pad;
        dest_data += dest_pad;
    }
}

static void convert_RGBA_PM_to_ARGB(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_RGBA8888_Premultiplied);
    Q_ASSERT(dest->format == QImage::Format_ARGB32);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const int src_pad = (src->bytes_per_line >> 2) - src->width;
    const int dest_pad = (dest->bytes_per_line >> 2) - dest->width;
    const QRgb *src_data = (QRgb *) src->data;
    QRgb *dest_data = (QRgb *) dest->data;

    for (int i = 0; i < src->height; ++i) {
        const QRgb *end = src_data + src->width;
        while (src_data < end) {
            *dest_data = qUnpremultiply(RGBA2ARGB(*src_data));
            ++src_data;
            ++dest_data;
        }
        src_data += src_pad;
        dest_data += dest_pad;
    }
}

static void convert_RGBA_PM_to_RGB(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_RGBA8888_Premultiplied);
    Q_ASSERT(dest->format == QImage::Format_RGB32);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const int src_pad = (src->bytes_per_line >> 2) - src->width;
    const int dest_pad = (dest->bytes_per_line >> 2) - dest->width;
    const QRgb *src_data = (QRgb *) src->data;
    QRgb *dest_data = (QRgb *) dest->data;

    for (int i = 0; i < src->height; ++i) {
        const QRgb *end = src_data + src->width;
        while (src_data < end) {
            *dest_data = 0xff000000 | qUnpremultiply(RGBA2ARGB(*src_data));
            ++src_data;
            ++dest_data;
        }
        src_data += src_pad;
        dest_data += dest_pad;
    }
}

static void swap_bit_order(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_Mono || src->format == QImage::Format_MonoLSB);
    Q_ASSERT(dest->format == QImage::Format_Mono || dest->format == QImage::Format_MonoLSB);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);
    Q_ASSERT(src->nbytes == dest->nbytes);
    Q_ASSERT(src->bytes_per_line == dest->bytes_per_line);

    dest->colortable = src->colortable;

    const uchar *src_data = src->data;
    const uchar *end = src->data + src->nbytes;
    uchar *dest_data = dest->data;
    while (src_data < end) {
        *dest_data = bitflip[*src_data];
        ++src_data;
        ++dest_data;
    }
}

static void mask_alpha_converter(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const int src_pad = (src->bytes_per_line >> 2) - src->width;
    const int dest_pad = (dest->bytes_per_line >> 2) - dest->width;
    const uint *src_data = (const uint *)src->data;
    uint *dest_data = (uint *)dest->data;

    for (int i = 0; i < src->height; ++i) {
        const uint *end = src_data + src->width;
        while (src_data < end) {
            *dest_data = *src_data | 0xff000000;
            ++src_data;
            ++dest_data;
        }
        src_data += src_pad;
        dest_data += dest_pad;
    }
}

static void mask_alpha_converter_RGBx(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags flags)
{
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    return mask_alpha_converter(dest, src, flags);
#else
    Q_UNUSED(flags);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const int src_pad = (src->bytes_per_line >> 2) - src->width;
    const int dest_pad = (dest->bytes_per_line >> 2) - dest->width;
    const uint *src_data = (const uint *)src->data;
    uint *dest_data = (uint *)dest->data;

    for (int i = 0; i < src->height; ++i) {
        const uint *end = src_data + src->width;
        while (src_data < end) {
            *dest_data = *src_data | 0x000000ff;
            ++src_data;
            ++dest_data;
        }
        src_data += src_pad;
        dest_data += dest_pad;
    }
#endif
}

static QVector<QRgb> fix_color_table(const QVector<QRgb> &ctbl, QImage::Format format)
{
    QVector<QRgb> colorTable = ctbl;
    if (format == QImage::Format_RGB32) {
        // check if the color table has alpha
        for (int i = 0; i < colorTable.size(); ++i)
            if (qAlpha(colorTable.at(i) != 0xff))
                colorTable[i] = colorTable.at(i) | 0xff000000;
    } else if (format == QImage::Format_ARGB32_Premultiplied) {
        // check if the color table has alpha
        for (int i = 0; i < colorTable.size(); ++i)
            colorTable[i] = qPremultiply(colorTable.at(i));
    }
    return colorTable;
}

//
// dither_to_1:  Uses selected dithering algorithm.
//

void dither_to_Mono(QImageData *dst, const QImageData *src,
                           Qt::ImageConversionFlags flags, bool fromalpha)
{
    Q_ASSERT(src->width == dst->width);
    Q_ASSERT(src->height == dst->height);
    Q_ASSERT(dst->format == QImage::Format_Mono || dst->format == QImage::Format_MonoLSB);

    dst->colortable.clear();
    dst->colortable.append(0xffffffff);
    dst->colortable.append(0xff000000);

    enum { Threshold, Ordered, Diffuse } dithermode;

    if (fromalpha) {
        if ((flags & Qt::AlphaDither_Mask) == Qt::DiffuseAlphaDither)
            dithermode = Diffuse;
        else if ((flags & Qt::AlphaDither_Mask) == Qt::OrderedAlphaDither)
            dithermode = Ordered;
        else
            dithermode = Threshold;
    } else {
        if ((flags & Qt::Dither_Mask) == Qt::ThresholdDither)
            dithermode = Threshold;
        else if ((flags & Qt::Dither_Mask) == Qt::OrderedDither)
            dithermode = Ordered;
        else
            dithermode = Diffuse;
    }

    int          w = src->width;
    int          h = src->height;
    int          d = src->depth;
    uchar gray[256];                                // gray map for 8 bit images
    bool  use_gray = (d == 8);
    if (use_gray) {                                // make gray map
        if (fromalpha) {
            // Alpha 0x00 -> 0 pixels (white)
            // Alpha 0xFF -> 1 pixels (black)
            for (int i = 0; i < src->colortable.size(); i++)
                gray[i] = (255 - (src->colortable.at(i) >> 24));
        } else {
            // Pixel 0x00 -> 1 pixels (black)
            // Pixel 0xFF -> 0 pixels (white)
            for (int i = 0; i < src->colortable.size(); i++)
                gray[i] = qGray(src->colortable.at(i));
        }
    }

    uchar *dst_data = dst->data;
    int dst_bpl = dst->bytes_per_line;
    const uchar *src_data = src->data;
    int src_bpl = src->bytes_per_line;

    switch (dithermode) {
    case Diffuse: {
        QScopedArrayPointer<int> lineBuffer(new int[w * 2]);
        int *line1 = lineBuffer.data();
        int *line2 = lineBuffer.data() + w;
        int bmwidth = (w+7)/8;

        int *b1, *b2;
        int wbytes = w * (d/8);
        const uchar *p = src->data;
        const uchar *end = p + wbytes;
        b2 = line2;
        if (use_gray) {                        // 8 bit image
            while (p < end)
                *b2++ = gray[*p++];
        } else {                                // 32 bit image
            if (fromalpha) {
                while (p < end) {
                    *b2++ = 255 - (*(uint*)p >> 24);
                    p += 4;
                }
            } else {
                while (p < end) {
                    *b2++ = qGray(*(uint*)p);
                    p += 4;
                }
            }
        }
        for (int y=0; y<h; y++) {                        // for each scan line...
            int *tmp = line1; line1 = line2; line2 = tmp;
            bool not_last_line = y < h - 1;
            if (not_last_line) {                // calc. grayvals for next line
                p = src->data + (y+1)*src->bytes_per_line;
                end = p + wbytes;
                b2 = line2;
                if (use_gray) {                // 8 bit image
                    while (p < end)
                        *b2++ = gray[*p++];
                } else {                        // 24 bit image
                    if (fromalpha) {
                        while (p < end) {
                            *b2++ = 255 - (*(uint*)p >> 24);
                            p += 4;
                        }
                    } else {
                        while (p < end) {
                            *b2++ = qGray(*(uint*)p);
                            p += 4;
                        }
                    }
                }
            }

            int err;
            uchar *p = dst->data + y*dst->bytes_per_line;
            memset(p, 0, bmwidth);
            b1 = line1;
            b2 = line2;
            int bit = 7;
            for (int x=1; x<=w; x++) {
                if (*b1 < 128) {                // black pixel
                    err = *b1++;
                    *p |= 1 << bit;
                } else {                        // white pixel
                    err = *b1++ - 255;
                }
                if (bit == 0) {
                    p++;
                    bit = 7;
                } else {
                    bit--;
                }
                if (x < w)
                    *b1 += (err*7)>>4;                // spread error to right pixel
                if (not_last_line) {
                    b2[0] += (err*5)>>4;        // pixel below
                    if (x > 1)
                        b2[-1] += (err*3)>>4;        // pixel below left
                    if (x < w)
                        b2[1] += err>>4;        // pixel below right
                }
                b2++;
            }
        }
    } break;
    case Ordered: {

        memset(dst->data, 0, dst->nbytes);
        if (d == 32) {
            for (int i=0; i<h; i++) {
                const uint *p = (const uint *)src_data;
                const uint *end = p + w;
                uchar *m = dst_data;
                int bit = 7;
                int j = 0;
                if (fromalpha) {
                    while (p < end) {
                        if ((*p++ >> 24) >= qt_bayer_matrix[j++&15][i&15])
                            *m |= 1 << bit;
                        if (bit == 0) {
                            m++;
                            bit = 7;
                        } else {
                            bit--;
                        }
                    }
                } else {
                    while (p < end) {
                        if ((uint)qGray(*p++) < qt_bayer_matrix[j++&15][i&15])
                            *m |= 1 << bit;
                        if (bit == 0) {
                            m++;
                            bit = 7;
                        } else {
                            bit--;
                        }
                    }
                }
                dst_data += dst_bpl;
                src_data += src_bpl;
            }
        } else
            /* (d == 8) */ {
            for (int i=0; i<h; i++) {
                const uchar *p = src_data;
                const uchar *end = p + w;
                uchar *m = dst_data;
                int bit = 7;
                int j = 0;
                while (p < end) {
                    if ((uint)gray[*p++] < qt_bayer_matrix[j++&15][i&15])
                        *m |= 1 << bit;
                    if (bit == 0) {
                        m++;
                        bit = 7;
                    } else {
                        bit--;
                    }
                }
                dst_data += dst_bpl;
                src_data += src_bpl;
            }
        }
    } break;
    default: { // Threshold:
        memset(dst->data, 0, dst->nbytes);
        if (d == 32) {
            for (int i=0; i<h; i++) {
                const uint *p = (const uint *)src_data;
                const uint *end = p + w;
                uchar *m = dst_data;
                int bit = 7;
                if (fromalpha) {
                    while (p < end) {
                        if ((*p++ >> 24) >= 128)
                            *m |= 1 << bit;        // Set mask "on"
                        if (bit == 0) {
                            m++;
                            bit = 7;
                        } else {
                            bit--;
                        }
                    }
                } else {
                    while (p < end) {
                        if (qGray(*p++) < 128)
                            *m |= 1 << bit;        // Set pixel "black"
                        if (bit == 0) {
                            m++;
                            bit = 7;
                        } else {
                            bit--;
                        }
                    }
                }
                dst_data += dst_bpl;
                src_data += src_bpl;
            }
        } else
            if (d == 8) {
                for (int i=0; i<h; i++) {
                    const uchar *p = src_data;
                    const uchar *end = p + w;
                    uchar *m = dst_data;
                    int bit = 7;
                    while (p < end) {
                        if (gray[*p++] < 128)
                            *m |= 1 << bit;                // Set mask "on"/ pixel "black"
                        if (bit == 0) {
                            m++;
                            bit = 7;
                        } else {
                            bit--;
                        }
                    }
                    dst_data += dst_bpl;
                    src_data += src_bpl;
                }
            }
        }
    }

    if (dst->format == QImage::Format_MonoLSB) {
        // need to swap bit order
        uchar *sl = dst->data;
        int bpl = (dst->width + 7) * dst->depth / 8;
        int pad = dst->bytes_per_line - bpl;
        for (int y=0; y<dst->height; ++y) {
            for (int x=0; x<bpl; ++x) {
                *sl = bitflip[*sl];
                ++sl;
            }
            sl += pad;
        }
    }
}

static void convert_X_to_Mono(QImageData *dst, const QImageData *src, Qt::ImageConversionFlags flags)
{
    dither_to_Mono(dst, src, flags, false);
}

static void convert_ARGB_PM_to_Mono(QImageData *dst, const QImageData *src, Qt::ImageConversionFlags flags)
{
    QScopedPointer<QImageData> tmp(QImageData::create(QSize(src->width, src->height), QImage::Format_ARGB32));
    convert_ARGB_PM_to_ARGB(tmp.data(), src, flags);
    dither_to_Mono(dst, tmp.data(), flags, false);
}

//
// convert_32_to_8:  Converts a 32 bits depth (true color) to an 8 bit
// image with a colormap. If the 32 bit image has more than 256 colors,
// we convert the red,green and blue bytes into a single byte encoded
// as 6 shades of each of red, green and blue.
//
// if dithering is needed, only 1 color at most is available for alpha.
//
struct QRgbMap {
    inline QRgbMap() : used(0) { }
    uchar  pix;
    uchar used;
    QRgb  rgb;
};

static void convert_RGB_to_Indexed8(QImageData *dst, const QImageData *src, Qt::ImageConversionFlags flags)
{
    Q_ASSERT(src->format == QImage::Format_RGB32 || src->format == QImage::Format_ARGB32);
    Q_ASSERT(dst->format == QImage::Format_Indexed8);
    Q_ASSERT(src->width == dst->width);
    Q_ASSERT(src->height == dst->height);

    bool    do_quant = (flags & Qt::DitherMode_Mask) == Qt::PreferDither
                       || src->format == QImage::Format_ARGB32;
    uint alpha_mask = src->format == QImage::Format_RGB32 ? 0xff000000 : 0;

    const int tablesize = 997; // prime
    QRgbMap table[tablesize];
    int   pix=0;

    if (!dst->colortable.isEmpty()) {
        QVector<QRgb> ctbl = dst->colortable;
        dst->colortable.resize(256);
        // Preload palette into table.
        // Almost same code as pixel insertion below
        for (int i = 0; i < dst->colortable.size(); ++i) {
            // Find in table...
            QRgb p = ctbl.at(i) | alpha_mask;
            int hash = p % tablesize;
            for (;;) {
                if (table[hash].used) {
                    if (table[hash].rgb == p) {
                        // Found previous insertion - use it
                        break;
                    } else {
                        // Keep searching...
                        if (++hash == tablesize) hash = 0;
                    }
                } else {
                    // Cannot be in table
                    Q_ASSERT (pix != 256);        // too many colors
                    // Insert into table at this unused position
                    dst->colortable[pix] = p;
                    table[hash].pix = pix++;
                    table[hash].rgb = p;
                    table[hash].used = 1;
                    break;
                }
            }
        }
    }

    if ((flags & Qt::DitherMode_Mask) != Qt::PreferDither) {
        dst->colortable.resize(256);
        const uchar *src_data = src->data;
        uchar *dest_data = dst->data;
        for (int y = 0; y < src->height; y++) {        // check if <= 256 colors
            const QRgb *s = (const QRgb *)src_data;
            uchar *b = dest_data;
            for (int x = 0; x < src->width; ++x) {
                QRgb p = s[x] | alpha_mask;
                int hash = p % tablesize;
                for (;;) {
                    if (table[hash].used) {
                        if (table[hash].rgb == (p)) {
                            // Found previous insertion - use it
                            break;
                        } else {
                            // Keep searching...
                            if (++hash == tablesize) hash = 0;
                        }
                    } else {
                        // Cannot be in table
                        if (pix == 256) {        // too many colors
                            do_quant = true;
                            // Break right out
                            x = src->width;
                            y = src->height;
                        } else {
                            // Insert into table at this unused position
                            dst->colortable[pix] = p;
                            table[hash].pix = pix++;
                            table[hash].rgb = p;
                            table[hash].used = 1;
                        }
                        break;
                    }
                }
                *b++ = table[hash].pix;                // May occur once incorrectly
            }
            src_data += src->bytes_per_line;
            dest_data += dst->bytes_per_line;
        }
    }
    int numColors = do_quant ? 256 : pix;

    dst->colortable.resize(numColors);

    if (do_quant) {                                // quantization needed

#define MAX_R 5
#define MAX_G 5
#define MAX_B 5
#define INDEXOF(r,g,b) (((r)*(MAX_G+1)+(g))*(MAX_B+1)+(b))

        for (int rc=0; rc<=MAX_R; rc++)                // build 6x6x6 color cube
            for (int gc=0; gc<=MAX_G; gc++)
                for (int bc=0; bc<=MAX_B; bc++)
                    dst->colortable[INDEXOF(rc,gc,bc)] = 0xff000000 | qRgb(rc*255/MAX_R, gc*255/MAX_G, bc*255/MAX_B);

        const uchar *src_data = src->data;
        uchar *dest_data = dst->data;
        if ((flags & Qt::Dither_Mask) == Qt::ThresholdDither) {
            for (int y = 0; y < src->height; y++) {
                const QRgb *p = (const QRgb *)src_data;
                const QRgb *end = p + src->width;
                uchar *b = dest_data;

                while (p < end) {
#define DITHER(p,m) ((uchar) ((p * (m) + 127) / 255))
                    *b++ =
                        INDEXOF(
                            DITHER(qRed(*p), MAX_R),
                            DITHER(qGreen(*p), MAX_G),
                            DITHER(qBlue(*p), MAX_B)
                            );
#undef DITHER
                    p++;
                }
                src_data += src->bytes_per_line;
                dest_data += dst->bytes_per_line;
            }
        } else if ((flags & Qt::Dither_Mask) == Qt::DiffuseDither) {
            int* line1[3];
            int* line2[3];
            int* pv[3];
            QScopedArrayPointer<int> lineBuffer(new int[src->width * 9]);
            line1[0] = lineBuffer.data();
            line2[0] = lineBuffer.data() + src->width;
            line1[1] = lineBuffer.data() + src->width * 2;
            line2[1] = lineBuffer.data() + src->width * 3;
            line1[2] = lineBuffer.data() + src->width * 4;
            line2[2] = lineBuffer.data() + src->width * 5;
            pv[0] = lineBuffer.data() + src->width * 6;
            pv[1] = lineBuffer.data() + src->width * 7;
            pv[2] = lineBuffer.data() + src->width * 8;

            int endian = (QSysInfo::ByteOrder == QSysInfo::BigEndian);
            for (int y = 0; y < src->height; y++) {
                const uchar* q = src_data;
                const uchar* q2 = y < src->height - 1 ? q + src->bytes_per_line : src->data;
                uchar *b = dest_data;
                for (int chan = 0; chan < 3; chan++) {
                    int *l1 = (y&1) ? line2[chan] : line1[chan];
                    int *l2 = (y&1) ? line1[chan] : line2[chan];
                    if (y == 0) {
                        for (int i = 0; i < src->width; i++)
                            l1[i] = q[i*4+chan+endian];
                    }
                    if (y+1 < src->height) {
                        for (int i = 0; i < src->width; i++)
                            l2[i] = q2[i*4+chan+endian];
                    }
                    // Bi-directional error diffusion
                    if (y&1) {
                        for (int x = 0; x < src->width; x++) {
                            int pix = qMax(qMin(5, (l1[x] * 5 + 128)/ 255), 0);
                            int err = l1[x] - pix * 255 / 5;
                            pv[chan][x] = pix;

                            // Spread the error around...
                            if (x + 1< src->width) {
                                l1[x+1] += (err*7)>>4;
                                l2[x+1] += err>>4;
                            }
                            l2[x]+=(err*5)>>4;
                            if (x>1)
                                l2[x-1]+=(err*3)>>4;
                        }
                    } else {
                        for (int x = src->width; x-- > 0;) {
                            int pix = qMax(qMin(5, (l1[x] * 5 + 128)/ 255), 0);
                            int err = l1[x] - pix * 255 / 5;
                            pv[chan][x] = pix;

                            // Spread the error around...
                            if (x > 0) {
                                l1[x-1] += (err*7)>>4;
                                l2[x-1] += err>>4;
                            }
                            l2[x]+=(err*5)>>4;
                            if (x + 1 < src->width)
                                l2[x+1]+=(err*3)>>4;
                        }
                    }
                }
                if (endian) {
                    for (int x = 0; x < src->width; x++) {
                        *b++ = INDEXOF(pv[0][x],pv[1][x],pv[2][x]);
                    }
                } else {
                    for (int x = 0; x < src->width; x++) {
                        *b++ = INDEXOF(pv[2][x],pv[1][x],pv[0][x]);
                    }
                }
                src_data += src->bytes_per_line;
                dest_data += dst->bytes_per_line;
            }
        } else { // OrderedDither
            for (int y = 0; y < src->height; y++) {
                const QRgb *p = (const QRgb *)src_data;
                const QRgb *end = p + src->width;
                uchar *b = dest_data;

                int x = 0;
                while (p < end) {
                    uint d = qt_bayer_matrix[y & 15][x & 15] << 8;

#define DITHER(p, d, m) ((uchar) ((((256 * (m) + (m) + 1)) * (p) + (d)) >> 16))
                    *b++ =
                        INDEXOF(
                            DITHER(qRed(*p), d, MAX_R),
                            DITHER(qGreen(*p), d, MAX_G),
                            DITHER(qBlue(*p), d, MAX_B)
                            );
#undef DITHER

                    p++;
                    x++;
                }
                src_data += src->bytes_per_line;
                dest_data += dst->bytes_per_line;
            }
        }

        if (src->format != QImage::Format_RGB32
            && src->format != QImage::Format_RGB16) {
            const int trans = 216;
            Q_ASSERT(dst->colortable.size() > trans);
            dst->colortable[trans] = 0;
            QScopedPointer<QImageData> mask(QImageData::create(QSize(src->width, src->height), QImage::Format_Mono));
            dither_to_Mono(mask.data(), src, flags, true);
            uchar *dst_data = dst->data;
            const uchar *mask_data = mask->data;
            for (int y = 0; y < src->height; y++) {
                for (int x = 0; x < src->width ; x++) {
                    if (!(mask_data[x>>3] & (0x80 >> (x & 7))))
                        dst_data[x] = trans;
                }
                mask_data += mask->bytes_per_line;
                dst_data += dst->bytes_per_line;
            }
            dst->has_alpha_clut = true;
        }

#undef MAX_R
#undef MAX_G
#undef MAX_B
#undef INDEXOF

    }
}

static void convert_ARGB_PM_to_Indexed8(QImageData *dst, const QImageData *src, Qt::ImageConversionFlags flags)
{
    QScopedPointer<QImageData> tmp(QImageData::create(QSize(src->width, src->height), QImage::Format_ARGB32));
    convert_ARGB_PM_to_ARGB(tmp.data(), src, flags);
    convert_RGB_to_Indexed8(dst, tmp.data(), flags);
}

static void convert_ARGB_to_Indexed8(QImageData *dst, const QImageData *src, Qt::ImageConversionFlags flags)
{
    convert_RGB_to_Indexed8(dst, src, flags);
}

static void convert_Indexed8_to_X32(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_Indexed8);
    Q_ASSERT(dest->format == QImage::Format_RGB32
             || dest->format == QImage::Format_ARGB32
             || dest->format == QImage::Format_ARGB32_Premultiplied);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    QVector<QRgb> colorTable = fix_color_table(src->colortable, dest->format);
    if (colorTable.size() == 0) {
        colorTable.resize(256);
        for (int i=0; i<256; ++i)
            colorTable[i] = qRgb(i, i, i);
    }

    int w = src->width;
    const uchar *src_data = src->data;
    uchar *dest_data = dest->data;
    int tableSize = colorTable.size() - 1;
    for (int y = 0; y < src->height; y++) {
        uint *p = (uint *)dest_data;
        const uchar *b = src_data;
        uint *end = p + w;

        while (p < end)
            *p++ = colorTable.at(qMin<int>(tableSize, *b++));

        src_data += src->bytes_per_line;
        dest_data += dest->bytes_per_line;
    }
}

static void convert_Mono_to_X32(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_Mono || src->format == QImage::Format_MonoLSB);
    Q_ASSERT(dest->format == QImage::Format_RGB32
             || dest->format == QImage::Format_ARGB32
             || dest->format == QImage::Format_ARGB32_Premultiplied);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    QVector<QRgb> colorTable = fix_color_table(src->colortable, dest->format);

    // Default to black / white colors
    if (colorTable.size() < 2) {
        if (colorTable.size() == 0)
            colorTable << 0xff000000;
        colorTable << 0xffffffff;
    }

    const uchar *src_data = src->data;
    uchar *dest_data = dest->data;
    if (src->format == QImage::Format_Mono) {
        for (int y = 0; y < dest->height; y++) {
            uint *p = (uint *)dest_data;
            for (int x = 0; x < dest->width; x++)
                *p++ = colorTable.at((src_data[x>>3] >> (7 - (x & 7))) & 1);

            src_data += src->bytes_per_line;
            dest_data += dest->bytes_per_line;
        }
    } else {
        for (int y = 0; y < dest->height; y++) {
            uint *p = (uint *)dest_data;
            for (int x = 0; x < dest->width; x++)
                *p++ = colorTable.at((src_data[x>>3] >> (x & 7)) & 1);

            src_data += src->bytes_per_line;
            dest_data += dest->bytes_per_line;
        }
    }
}


static void convert_Mono_to_Indexed8(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_Mono || src->format == QImage::Format_MonoLSB);
    Q_ASSERT(dest->format == QImage::Format_Indexed8);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    QVector<QRgb> ctbl = src->colortable;
    if (ctbl.size() > 2) {
        ctbl.resize(2);
    } else if (ctbl.size() < 2) {
        if (ctbl.size() == 0)
            ctbl << 0xff000000;
        ctbl << 0xffffffff;
    }
    dest->colortable = ctbl;
    dest->has_alpha_clut = src->has_alpha_clut;


    const uchar *src_data = src->data;
    uchar *dest_data = dest->data;
    if (src->format == QImage::Format_Mono) {
        for (int y = 0; y < dest->height; y++) {
            uchar *p = dest_data;
            for (int x = 0; x < dest->width; x++)
                *p++ = (src_data[x>>3] >> (7 - (x & 7))) & 1;
            src_data += src->bytes_per_line;
            dest_data += dest->bytes_per_line;
        }
    } else {
        for (int y = 0; y < dest->height; y++) {
            uchar *p = dest_data;
            for (int x = 0; x < dest->width; x++)
                *p++ = (src_data[x>>3] >> (x & 7)) & 1;
            src_data += src->bytes_per_line;
            dest_data += dest->bytes_per_line;
        }
    }
}

// first index source, second dest
Image_Converter qimage_converter_map[QImage::NImageFormats][QImage::NImageFormats] =
{
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    },
    {
        0,
        0,
        swap_bit_order,
        convert_Mono_to_Indexed8,
        convert_Mono_to_X32,
        convert_Mono_to_X32,
        convert_Mono_to_X32,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_Mono

    {
        0,
        swap_bit_order,
        0,
        convert_Mono_to_Indexed8,
        convert_Mono_to_X32,
        convert_Mono_to_X32,
        convert_Mono_to_X32,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_MonoLSB

    {
        0,
        convert_X_to_Mono,
        convert_X_to_Mono,
        0,
        convert_Indexed8_to_X32,
        convert_Indexed8_to_X32,
        convert_Indexed8_to_X32,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_Indexed8

    {
        0,
        convert_X_to_Mono,
        convert_X_to_Mono,
        convert_RGB_to_Indexed8,
        0,
        mask_alpha_converter,
        mask_alpha_converter,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        convert_RGB_to_RGBA,
        convert_RGB_to_RGBA,
        convert_RGB_to_RGBA
    }, // Format_RGB32

    {
        0,
        convert_X_to_Mono,
        convert_X_to_Mono,
        convert_ARGB_to_Indexed8,
        mask_alpha_converter,
        0,
        convert_ARGB_to_ARGB_PM,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        convert_ARGB_to_RGBx,
        convert_ARGB_to_RGBA,
        convert_ARGB_to_RGBA_PM,
    }, // Format_ARGB32

    {
        0,
        convert_ARGB_PM_to_Mono,
        convert_ARGB_PM_to_Mono,
        convert_ARGB_PM_to_Indexed8,
        convert_ARGB_PM_to_RGB,
        convert_ARGB_PM_to_ARGB,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        convert_ARGB_PM_to_RGBx,
        convert_ARGB_PM_to_RGBA,
        convert_ARGB_to_RGBA,
    },  // Format_ARGB32_Premultiplied

    {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_RGB16

    {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_ARGB8565_Premultiplied

    {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_RGB666

    {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_ARGB6666_Premultiplied

    {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_RGB555

    {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_ARGB8555_Premultiplied

    {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_RGB888

    {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_RGB444

    {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    }, // Format_ARGB4444_Premultiplied
    {
        0,
        0,
        0,
        0,
        convert_RGBA_to_RGB,
        convert_RGBA_to_ARGB,
        convert_RGBA_to_ARGB,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        mask_alpha_converter_RGBx,
        mask_alpha_converter_RGBx,
    }, // Format_RGBX8888
    {
        0,
        0,
        0,
        0,
        convert_RGBA_to_RGB,
        convert_RGBA_to_ARGB,
        convert_RGBA_to_ARGB_PM,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        mask_alpha_converter_RGBx,
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        0,
        convert_ARGB_to_ARGB_PM,
#else
        0,
        0
#endif
    }, // Format_RGBA8888

    {
        0,
        0,
        0,
        0,
        convert_RGBA_PM_to_RGB,
        convert_RGBA_PM_to_ARGB,
        convert_RGBA_to_ARGB,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        convert_ARGB_PM_to_RGB,
        convert_ARGB_PM_to_ARGB,
        0,
#else
        0,
        0,
        0
#endif
    } // Format_RGBA8888_Premultiplied
};

InPlace_Image_Converter qimage_inplace_converter_map[QImage::NImageFormats][QImage::NImageFormats] =
{
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    },
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }, // Format_Mono
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }, // Format_MonoLSB
    {
        0,
        0,
        0,
        0,
        0,
        convert_indexed8_to_RGB_inplace,
        convert_indexed8_to_ARGB_PM_inplace,
        convert_indexed8_to_RGB16_inplace,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    }, // Format_Indexed8
    {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        convert_RGB_to_RGB16_inplace,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    }, // Format_RGB32
    {
        0,
        0,
        0,
        0,
        0,
        0,
#ifdef __SSE2__
        convert_ARGB_to_ARGB_PM_inplace_sse2,
#else
        convert_ARGB_to_ARGB_PM_inplace,
#endif
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        convert_ARGB_to_RGBA_inplace,
        0,
    }, // Format_ARGB32
    {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        convert_ARGB_to_RGBA_inplace
    },  // Format_ARGB32_Premultiplied
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }, // Format_RGB16
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }, // Format_ARGB8565_Premultiplied
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }, // Format_RGB666
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }, // Format_ARGB6666_Premultiplied
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }, // Format_RGB555
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }, // Format_ARGB8555_Premultiplied
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }, // Format_RGB888
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }, // Format_RGB444
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }, // Format_ARGB4444_Premultiplied
    {
        0,
        0,
        0,
        0,
        0,
        convert_RGBA_to_ARGB_inplace,
        convert_RGBA_to_ARGB_inplace,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    }, // Format_RGBX8888
    {
        0,
        0,
        0,
        0,
        0,
        0,
        convert_RGBA_to_ARGB_inplace,
        convert_RGBA_to_ARGB_PM_inplace,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    }, // Format_RGBA8888
    {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        convert_RGBA_to_ARGB_inplace,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    }  // Format_RGBA8888_Premultiplied
};

void qInitImageConversions()
{
#if defined(__SSE2__) && defined(QT_COMPILER_SUPPORTS_SSSE3)
    if (qCpuHasFeature(SSSE3)) {
        extern void convert_RGB888_to_RGB32_ssse3(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags);
        qimage_converter_map[QImage::Format_RGB888][QImage::Format_RGB32] = convert_RGB888_to_RGB32_ssse3;
        qimage_converter_map[QImage::Format_RGB888][QImage::Format_ARGB32] = convert_RGB888_to_RGB32_ssse3;
        qimage_converter_map[QImage::Format_RGB888][QImage::Format_ARGB32_Premultiplied] = convert_RGB888_to_RGB32_ssse3;
    }
#endif

#ifdef __ARM_NEON__
    extern void convert_RGB888_to_RGB32_neon(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags);
    qimage_converter_map[QImage::Format_RGB888][QImage::Format_RGB32] = convert_RGB888_to_RGB32_neon;
    qimage_converter_map[QImage::Format_RGB888][QImage::Format_ARGB32] = convert_RGB888_to_RGB32_neon;
    qimage_converter_map[QImage::Format_RGB888][QImage::Format_ARGB32_Premultiplied] = convert_RGB888_to_RGB32_neon;
#endif

#ifdef QT_COMPILER_SUPPORTS_MIPS_DSPR2
    extern bool convert_ARGB_to_ARGB_PM_inplace_mips_dspr2(QImageData *data, Qt::ImageConversionFlags);
    inplace_converter_map[QImage::Format_ARGB32][QImage::Format_ARGB32_Premultiplied] = convert_ARGB_to_ARGB_PM_inplace_mips_dspr2;
    return;
#endif
}

QT_END_NAMESPACE
