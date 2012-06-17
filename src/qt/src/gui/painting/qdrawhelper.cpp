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

#include <private/qdrawhelper_p.h>
#include <private/qpaintengine_raster_p.h>
#include <private/qpainter_p.h>
#include <private/qdrawhelper_x86_p.h>
#ifdef QT_HAVE_ARM_SIMD
#include <private/qdrawhelper_arm_simd_p.h>
#endif
#include <private/qdrawhelper_neon_p.h>
#include <private/qmath_p.h>
#include <qmath.h>

QT_BEGIN_NAMESPACE


#define MASK(src, a) src = BYTE_MUL(src, a)

#if defined(Q_OS_IRIX) && defined(Q_CC_GNU) && __GNUC__ == 3 && __GNUC__ < 4 && QT_POINTER_SIZE == 8
#define Q_IRIX_GCC3_3_WORKAROUND
//
// work around http://gcc.gnu.org/bugzilla/show_bug.cgi?id=14484
//
static uint gccBug(uint value) __attribute__((noinline));
static uint gccBug(uint value)
{
    return value;
}
#endif

/*
  constants and structures
*/

enum {
    fixed_scale = 1 << 16,
    half_point = 1 << 15
};

// must be multiple of 4 for easier SIMD implementations
static const int buffer_size = 2048;

/*
  Destination fetch. This is simple as we don't have to do bounds checks or
  transformations
*/

static uint * QT_FASTCALL destFetchMono(uint *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length)
{
    uchar *data = (uchar *)rasterBuffer->scanLine(y);
    uint *start = buffer;
    const uint *end = buffer + length;
    while (buffer < end) {
        *buffer = data[x>>3] & (0x80 >> (x & 7)) ? rasterBuffer->destColor1 : rasterBuffer->destColor0;
        ++buffer;
        ++x;
    }
    return start;
}

static uint * QT_FASTCALL destFetchMonoLsb(uint *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length)
{
    uchar *data = (uchar *)rasterBuffer->scanLine(y);
    uint *start = buffer;
    const uint *end = buffer + length;
    while (buffer < end) {
        *buffer = data[x>>3] & (0x1 << (x & 7)) ? rasterBuffer->destColor1 : rasterBuffer->destColor0;
        ++buffer;
        ++x;
    }
    return start;
}

static uint * QT_FASTCALL destFetchARGB32(uint *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length)
{
    const uint *data = (const uint *)rasterBuffer->scanLine(y) + x;
    for (int i = 0; i < length; ++i)
        buffer[i] = PREMUL(data[i]);
    return buffer;
}

static uint * QT_FASTCALL destFetchARGB32P(uint *, QRasterBuffer *rasterBuffer, int x, int y, int)
{
    return (uint *)rasterBuffer->scanLine(y) + x;
}

static uint * QT_FASTCALL destFetchRGB16(uint *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length)
{
    const ushort *data = (const ushort *)rasterBuffer->scanLine(y) + x;
    for (int i = 0; i < length; ++i)
        buffer[i] = qConvertRgb16To32(data[i]);
    return buffer;
}

template <class DST>
Q_STATIC_TEMPLATE_FUNCTION uint * QT_FASTCALL destFetch(uint *buffer, QRasterBuffer *rasterBuffer,
                                    int x, int y, int length)
{
    const DST *src = reinterpret_cast<DST*>(rasterBuffer->scanLine(y)) + x;
    quint32 *dest = reinterpret_cast<quint32*>(buffer);
    while (length--)
        *dest++ = *src++;
    return buffer;
}

# define SPANFUNC_POINTER_DESTFETCH(Arg) destFetch<Arg>

static DestFetchProc destFetchProc[QImage::NImageFormats] =
{
    0, // Format_Invalid
    destFetchMono, // Format_Mono,
    destFetchMonoLsb, // Format_MonoLSB
    0, // Format_Indexed8
    destFetchARGB32P, // Format_RGB32
    destFetchARGB32, // Format_ARGB32,
    destFetchARGB32P, // Format_ARGB32_Premultiplied
    destFetchRGB16,   // Format_RGB16
    SPANFUNC_POINTER_DESTFETCH(qargb8565), // Format_ARGB8565_Premultiplied
    SPANFUNC_POINTER_DESTFETCH(qrgb666),   // Format_RGB666
    SPANFUNC_POINTER_DESTFETCH(qargb6666), // Format_ARGB6666_Premultiplied
    SPANFUNC_POINTER_DESTFETCH(qrgb555),   // Format_RGB555
    SPANFUNC_POINTER_DESTFETCH(qargb8555), // Format_ARGB8555_Premultiplied
    SPANFUNC_POINTER_DESTFETCH(qrgb888),   // Format_RGB888
    SPANFUNC_POINTER_DESTFETCH(qrgb444),   // Format_RGB444
    SPANFUNC_POINTER_DESTFETCH(qargb4444)  // Format_ARGB4444_Premultiplied
};

/*
   Returns the color in the mono destination color table
   that is the "nearest" to /color/.
*/
static inline QRgb findNearestColor(QRgb color, QRasterBuffer *rbuf)
{
    QRgb color_0 = PREMUL(rbuf->destColor0);
    QRgb color_1 = PREMUL(rbuf->destColor1);
    color = PREMUL(color);

    int r = qRed(color);
    int g = qGreen(color);
    int b = qBlue(color);
    int rx, gx, bx;
    int dist_0, dist_1;

    rx = r - qRed(color_0);
    gx = g - qGreen(color_0);
    bx = b - qBlue(color_0);
    dist_0 = rx*rx + gx*gx + bx*bx;

    rx = r - qRed(color_1);
    gx = g - qGreen(color_1);
    bx = b - qBlue(color_1);
    dist_1 = rx*rx + gx*gx + bx*bx;

    if (dist_0 < dist_1)
        return color_0;
    return color_1;
}

/*
  Destination store.
*/

static void QT_FASTCALL destStoreMono(QRasterBuffer *rasterBuffer, int x, int y, const uint *buffer, int length)
{
    uchar *data = (uchar *)rasterBuffer->scanLine(y);
    if (rasterBuffer->monoDestinationWithClut) {
        for (int i = 0; i < length; ++i) {
            if (buffer[i] == rasterBuffer->destColor0) {
                data[x >> 3] &= ~(0x80 >> (x & 7));
            } else if (buffer[i] == rasterBuffer->destColor1) {
                data[x >> 3] |= 0x80 >> (x & 7);
            } else if (findNearestColor(buffer[i], rasterBuffer) == rasterBuffer->destColor0) {
                data[x >> 3] &= ~(0x80 >> (x & 7));
            } else {
                data[x >> 3] |= 0x80 >> (x & 7);
            }
            ++x;
        }
    } else {
        for (int i = 0; i < length; ++i) {
            if (qGray(buffer[i]) < int(qt_bayer_matrix[y & 15][x & 15]))
                data[x >> 3] |= 0x80 >> (x & 7);
            else
                data[x >> 3] &= ~(0x80 >> (x & 7));
            ++x;
        }
    }
}

static void QT_FASTCALL destStoreMonoLsb(QRasterBuffer *rasterBuffer, int x, int y, const uint *buffer, int length)
{
    uchar *data = (uchar *)rasterBuffer->scanLine(y);
    if (rasterBuffer->monoDestinationWithClut) {
        for (int i = 0; i < length; ++i) {
            if (buffer[i] == rasterBuffer->destColor0) {
                data[x >> 3] &= ~(1 << (x & 7));
            } else if (buffer[i] == rasterBuffer->destColor1) {
                data[x >> 3] |= 1 << (x & 7);
            } else if (findNearestColor(buffer[i], rasterBuffer) == rasterBuffer->destColor0) {
                data[x >> 3] &= ~(1 << (x & 7));
            } else {
                data[x >> 3] |= 1 << (x & 7);
            }
            ++x;
        }
    } else {
        for (int i = 0; i < length; ++i) {
            if (qGray(buffer[i]) < int(qt_bayer_matrix[y & 15][x & 15]))
                data[x >> 3] |= 1 << (x & 7);
            else
                data[x >> 3] &= ~(1 << (x & 7));
            ++x;
        }
    }
}

static void QT_FASTCALL destStoreARGB32(QRasterBuffer *rasterBuffer, int x, int y, const uint *buffer, int length)
{
    uint *data = (uint *)rasterBuffer->scanLine(y) + x;
    for (int i = 0; i < length; ++i) {
        int p = buffer[i];
        int alpha = qAlpha(p);
        if (alpha == 255)
            data[i] = p;
        else if (alpha == 0)
            data[i] = 0;
        else {
            int inv_alpha = 0xff0000/qAlpha(buffer[i]);
            data[i] = (p & 0xff000000)
                      | ((qRed(p)*inv_alpha) & 0xff0000)
                      | (((qGreen(p)*inv_alpha) >> 8) & 0xff00)
                      | ((qBlue(p)*inv_alpha) >> 16);
        }
    }
}

static void QT_FASTCALL destStoreRGB16(QRasterBuffer *rasterBuffer, int x, int y, const uint *buffer, int length)
{
    quint16 *data = (quint16*)rasterBuffer->scanLine(y) + x;
    qt_memconvert<quint16, quint32>(data, buffer, length);
}

template <class DST>
Q_STATIC_TEMPLATE_FUNCTION void QT_FASTCALL destStore(QRasterBuffer *rasterBuffer,
                                  int x, int y,
                                  const uint *buffer, int length)
{
    DST *dest = reinterpret_cast<DST*>(rasterBuffer->scanLine(y)) + x;
    const quint32p *src = reinterpret_cast<const quint32p*>(buffer);
    while (length--)
        *dest++ = DST(*src++);
}

# define SPANFUNC_POINTER_DESTSTORE(DEST) destStore<DEST>

static DestStoreProc destStoreProc[QImage::NImageFormats] =
{
    0, // Format_Invalid
    destStoreMono, // Format_Mono,
    destStoreMonoLsb, // Format_MonoLSB
    0, // Format_Indexed8
    0, // Format_RGB32
    destStoreARGB32, // Format_ARGB32,
    0, // Format_ARGB32_Premultiplied
    destStoreRGB16, // Format_RGB16
    SPANFUNC_POINTER_DESTSTORE(qargb8565), // Format_ARGB8565_Premultiplied
    SPANFUNC_POINTER_DESTSTORE(qrgb666),   // Format_RGB666
    SPANFUNC_POINTER_DESTSTORE(qargb6666), // Format_ARGB6666_Premultiplied
    SPANFUNC_POINTER_DESTSTORE(qrgb555),   // Format_RGB555
    SPANFUNC_POINTER_DESTSTORE(qargb8555), // Format_ARGB8555_Premultiplied
    SPANFUNC_POINTER_DESTSTORE(qrgb888),   // Format_RGB888
    SPANFUNC_POINTER_DESTSTORE(qrgb444),   // Format_RGB444
    SPANFUNC_POINTER_DESTSTORE(qargb4444)  // Format_ARGB4444_Premultiplied
};

/*
  Source fetches

  This is a bit more complicated, as we need several fetch routines for every surface type

  We need 5 fetch methods per surface type:
  untransformed
  transformed (tiled and not tiled)
  transformed bilinear (tiled and not tiled)

  We don't need bounds checks for untransformed, but we need them for the other ones.

  The generic implementation does pixel by pixel fetches
*/

template <QImage::Format format>
Q_STATIC_TEMPLATE_FUNCTION uint QT_FASTCALL qt_fetchPixel(const uchar *scanLine, int x, const QVector<QRgb> *rgb);

template<>
Q_STATIC_TEMPLATE_SPECIALIZATION
uint QT_FASTCALL qt_fetchPixel<QImage::Format_Mono>(const uchar *scanLine,
                                                 int x, const QVector<QRgb> *rgb)
{
    bool pixel = scanLine[x>>3] & (0x80 >> (x & 7));
    if (rgb) return PREMUL(rgb->at(pixel ? 1 : 0));
    return pixel ? 0xff000000 : 0xffffffff;
}

template<>
Q_STATIC_TEMPLATE_SPECIALIZATION
uint QT_FASTCALL qt_fetchPixel<QImage::Format_MonoLSB>(const uchar *scanLine,
                                                    int x, const QVector<QRgb> *rgb)
{
    bool pixel = scanLine[x>>3] & (0x1 << (x & 7));
    if (rgb) return PREMUL(rgb->at(pixel ? 1 : 0));
    return pixel ? 0xff000000 : 0xffffffff;
}

template<>
Q_STATIC_TEMPLATE_SPECIALIZATION
uint QT_FASTCALL qt_fetchPixel<QImage::Format_Indexed8>(const uchar *scanLine,
                                                     int x, const QVector<QRgb> *rgb)
{
    return PREMUL(rgb->at(scanLine[x]));
}

template<>
Q_STATIC_TEMPLATE_SPECIALIZATION
uint QT_FASTCALL qt_fetchPixel<QImage::Format_ARGB32>(const uchar *scanLine,
                                                   int x, const QVector<QRgb> *)
{
    return PREMUL(((const uint *)scanLine)[x]);
}

template<>
Q_STATIC_TEMPLATE_SPECIALIZATION
uint QT_FASTCALL qt_fetchPixel<QImage::Format_ARGB32_Premultiplied>(const uchar *scanLine,
                                                                 int x, const QVector<QRgb> *)
{
    return ((const uint *)scanLine)[x];
}

template<>
Q_STATIC_TEMPLATE_SPECIALIZATION
uint QT_FASTCALL qt_fetchPixel<QImage::Format_RGB16>(const uchar *scanLine,
                                                  int x, const QVector<QRgb> *)
{
    return qConvertRgb16To32(((const ushort *)scanLine)[x]);
}

template<>
Q_STATIC_TEMPLATE_SPECIALIZATION
uint QT_FASTCALL qt_fetchPixel<QImage::Format_ARGB8565_Premultiplied>(const uchar *scanLine,
                                                     int x,
                                                     const QVector<QRgb> *)
{
    const qargb8565 color = reinterpret_cast<const qargb8565*>(scanLine)[x];
    return qt_colorConvert<quint32, qargb8565>(color, 0);
}

template<>
Q_STATIC_TEMPLATE_SPECIALIZATION
uint QT_FASTCALL qt_fetchPixel<QImage::Format_RGB666>(const uchar *scanLine,
                                                   int x,
                                                   const QVector<QRgb> *)
{
    const qrgb666 color = reinterpret_cast<const qrgb666*>(scanLine)[x];
    return qt_colorConvert<quint32, qrgb666>(color, 0);
}

template<>
Q_STATIC_TEMPLATE_SPECIALIZATION
uint QT_FASTCALL qt_fetchPixel<QImage::Format_ARGB6666_Premultiplied>(const uchar *scanLine,
                                                   int x,
                                                   const QVector<QRgb> *)
{
    const qargb6666 color = reinterpret_cast<const qargb6666*>(scanLine)[x];
    return qt_colorConvert<quint32, qargb6666>(color, 0);
}

template<>
Q_STATIC_TEMPLATE_SPECIALIZATION
uint QT_FASTCALL qt_fetchPixel<QImage::Format_RGB555>(const uchar *scanLine,
                                                   int x,
                                                   const QVector<QRgb> *)
{
    const qrgb555 color = reinterpret_cast<const qrgb555*>(scanLine)[x];
    return qt_colorConvert<quint32, qrgb555>(color, 0);
}

template<>
Q_STATIC_TEMPLATE_SPECIALIZATION
uint QT_FASTCALL qt_fetchPixel<QImage::Format_ARGB8555_Premultiplied>(const uchar *scanLine,
                                                     int x,
                                                     const QVector<QRgb> *)
{
    const qargb8555 color = reinterpret_cast<const qargb8555*>(scanLine)[x];
    return qt_colorConvert<quint32, qargb8555>(color, 0);
}

template<>
Q_STATIC_TEMPLATE_SPECIALIZATION
uint QT_FASTCALL qt_fetchPixel<QImage::Format_RGB888>(const uchar *scanLine,
                                                   int x,
                                                   const QVector<QRgb> *)
{
    const qrgb888 color = reinterpret_cast<const qrgb888*>(scanLine)[x];
    return qt_colorConvert<quint32, qrgb888>(color, 0);
}

template<>
Q_STATIC_TEMPLATE_SPECIALIZATION
uint QT_FASTCALL qt_fetchPixel<QImage::Format_RGB444>(const uchar *scanLine,
                                                   int x,
                                                   const QVector<QRgb> *)
{
    const qrgb444 color = reinterpret_cast<const qrgb444*>(scanLine)[x];
    return qt_colorConvert<quint32, qrgb444>(color, 0);
}

template<>
Q_STATIC_TEMPLATE_SPECIALIZATION
uint QT_FASTCALL qt_fetchPixel<QImage::Format_ARGB4444_Premultiplied>(const uchar *scanLine,
                                                     int x,
                                                     const QVector<QRgb> *)
{
    const qargb4444 color = reinterpret_cast<const qargb4444*>(scanLine)[x];
    return qt_colorConvert<quint32, qargb4444>(color, 0);
}

template<>
Q_STATIC_TEMPLATE_SPECIALIZATION
uint QT_FASTCALL qt_fetchPixel<QImage::Format_Invalid>(const uchar *,
                                                     int ,
                                                     const QVector<QRgb> *)
{
    return 0;
}

typedef uint (QT_FASTCALL *FetchPixelProc)(const uchar *scanLine, int x, const QVector<QRgb> *);

#define SPANFUNC_POINTER_FETCHPIXEL(Arg) qt_fetchPixel<QImage::Arg>


static const FetchPixelProc fetchPixelProc[QImage::NImageFormats] =
{
    0,
    SPANFUNC_POINTER_FETCHPIXEL(Format_Mono),
    SPANFUNC_POINTER_FETCHPIXEL(Format_MonoLSB),
    SPANFUNC_POINTER_FETCHPIXEL(Format_Indexed8),
    SPANFUNC_POINTER_FETCHPIXEL(Format_ARGB32_Premultiplied),
    SPANFUNC_POINTER_FETCHPIXEL(Format_ARGB32),
    SPANFUNC_POINTER_FETCHPIXEL(Format_ARGB32_Premultiplied),
    SPANFUNC_POINTER_FETCHPIXEL(Format_RGB16),
    SPANFUNC_POINTER_FETCHPIXEL(Format_ARGB8565_Premultiplied),
    SPANFUNC_POINTER_FETCHPIXEL(Format_RGB666),
    SPANFUNC_POINTER_FETCHPIXEL(Format_ARGB6666_Premultiplied),
    SPANFUNC_POINTER_FETCHPIXEL(Format_RGB555),
    SPANFUNC_POINTER_FETCHPIXEL(Format_ARGB8555_Premultiplied),
    SPANFUNC_POINTER_FETCHPIXEL(Format_RGB888),
    SPANFUNC_POINTER_FETCHPIXEL(Format_RGB444),
    SPANFUNC_POINTER_FETCHPIXEL(Format_ARGB4444_Premultiplied)
};

enum TextureBlendType {
    BlendUntransformed,
    BlendTiled,
    BlendTransformed,
    BlendTransformedTiled,
    BlendTransformedBilinear,
    BlendTransformedBilinearTiled,
    NBlendTypes
};

template <QImage::Format format>
Q_STATIC_TEMPLATE_FUNCTION const uint * QT_FASTCALL qt_fetchUntransformed(uint *buffer, const Operator *, const QSpanData *data,
                                             int y, int x, int length)
{
    const uchar *scanLine = data->texture.scanLine(y);
    for (int i = 0; i < length; ++i)
        buffer[i] = qt_fetchPixel<format>(scanLine, x + i, data->texture.colorTable);
    return buffer;
}

template <>
Q_STATIC_TEMPLATE_SPECIALIZATION const uint * QT_FASTCALL
qt_fetchUntransformed<QImage::Format_ARGB32_Premultiplied>(uint *, const Operator *,
                                                         const QSpanData *data,
                                                         int y, int x, int)
{
    const uchar *scanLine = data->texture.scanLine(y);
    return ((const uint *)scanLine) + x;
}

template<TextureBlendType blendType>  /* either BlendTransformed or BlendTransformedTiled */
Q_STATIC_TEMPLATE_FUNCTION
const uint * QT_FASTCALL fetchTransformed(uint *buffer, const Operator *, const QSpanData *data,
                                                         int y, int x, int length)
{
    FetchPixelProc fetch = fetchPixelProc[data->texture.format];

    int image_width = data->texture.width;
    int image_height = data->texture.height;

    const qreal cx = x + qreal(0.5);
    const qreal cy = y + qreal(0.5);

    const uint *end = buffer + length;
    uint *b = buffer;
    if (data->fast_matrix) {
        // The increment pr x in the scanline
        int fdx = (int)(data->m11 * fixed_scale);
        int fdy = (int)(data->m12 * fixed_scale);

        int fx = int((data->m21 * cy
                      + data->m11 * cx + data->dx) * fixed_scale);
        int fy = int((data->m22 * cy
                      + data->m12 * cx + data->dy) * fixed_scale);

        while (b < end) {
            int px = fx >> 16;
            int py = fy >> 16;

            if (blendType == BlendTransformedTiled) {
                px %= image_width;
                py %= image_height;
                if (px < 0) px += image_width;
                if (py < 0) py += image_height;

                const uchar *scanLine = data->texture.scanLine(py);
                *b = fetch(scanLine, px, data->texture.colorTable);
            } else {
                if ((px < 0) || (px >= image_width)
                    || (py < 0) || (py >= image_height)) {
                    *b = uint(0);
                } else {
                    const uchar *scanLine = data->texture.scanLine(py);
                    *b = fetch(scanLine, px, data->texture.colorTable);
                }
            }
            fx += fdx;
            fy += fdy;
            ++b;
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        qreal fx = data->m21 * cy + data->m11 * cx + data->dx;
        qreal fy = data->m22 * cy + data->m12 * cx + data->dy;
        qreal fw = data->m23 * cy + data->m13 * cx + data->m33;

        while (b < end) {
            const qreal iw = fw == 0 ? 1 : 1 / fw;
            const qreal tx = fx * iw;
            const qreal ty = fy * iw;
            int px = int(tx) - (tx < 0);
            int py = int(ty) - (ty < 0);

            if (blendType == BlendTransformedTiled) {
                px %= image_width;
                py %= image_height;
                if (px < 0) px += image_width;
                if (py < 0) py += image_height;

                const uchar *scanLine = data->texture.scanLine(py);
                *b = fetch(scanLine, px, data->texture.colorTable);
            } else {
                if ((px < 0) || (px >= image_width)
                    || (py < 0) || (py >= image_height)) {
                    *b = uint(0);
                } else {
                    const uchar *scanLine = data->texture.scanLine(py);
                    *b = fetch(scanLine, px, data->texture.colorTable);
                }
            }
            fx += fdx;
            fy += fdy;
            fw += fdw;
            //force increment to avoid /0
            if (!fw) {
                fw += fdw;
            }
            ++b;
        }
    }

    return buffer;
}

/** \internal
  interpolate 4 argb pixels with the distx and disty factor.
  distx and disty bust be between 0 and 16
 */
static inline uint interpolate_4_pixels_16(uint tl, uint tr, uint bl, uint br, int distx, int disty)
{
    uint distxy = distx * disty;
    //idistx * disty = (16-distx) * disty = 16*disty - distxy
    //idistx * idisty = (16-distx) * (16-disty) = 16*16 - 16*distx -16*dity + distxy
    uint tlrb = (tl & 0x00ff00ff)         * (16*16 - 16*distx - 16*disty + distxy);
    uint tlag = ((tl & 0xff00ff00) >> 8)  * (16*16 - 16*distx - 16*disty + distxy);
    uint trrb = ((tr & 0x00ff00ff)        * (distx*16 - distxy));
    uint trag = (((tr & 0xff00ff00) >> 8) * (distx*16 - distxy));
    uint blrb = ((bl & 0x00ff00ff)        * (disty*16 - distxy));
    uint blag = (((bl & 0xff00ff00) >> 8) * (disty*16 - distxy));
    uint brrb = ((br & 0x00ff00ff)        * (distxy));
    uint brag = (((br & 0xff00ff00) >> 8) * (distxy));
    return (((tlrb + trrb + blrb + brrb) >> 8) & 0x00ff00ff) | ((tlag + trag + blag + brag) & 0xff00ff00);
}

#if defined(QT_ALWAYS_HAVE_SSE2)
#define interpolate_4_pixels_16_sse2(tl, tr, bl, br, distx, disty, colorMask, v_256, b)  \
{ \
    const __m128i dxdy = _mm_mullo_epi16 (distx, disty); \
    const __m128i distx_ = _mm_slli_epi16(distx, 4); \
    const __m128i disty_ = _mm_slli_epi16(disty, 4); \
    const __m128i idxidy =  _mm_add_epi16(dxdy, _mm_sub_epi16(v_256, _mm_add_epi16(distx_, disty_))); \
    const __m128i dxidy =  _mm_sub_epi16(distx_, dxdy); \
    const __m128i idxdy =  _mm_sub_epi16(disty_, dxdy); \
 \
    __m128i tlAG = _mm_srli_epi16(tl, 8); \
    __m128i tlRB = _mm_and_si128(tl, colorMask); \
    __m128i trAG = _mm_srli_epi16(tr, 8); \
    __m128i trRB = _mm_and_si128(tr, colorMask); \
    __m128i blAG = _mm_srli_epi16(bl, 8); \
    __m128i blRB = _mm_and_si128(bl, colorMask); \
    __m128i brAG = _mm_srli_epi16(br, 8); \
    __m128i brRB = _mm_and_si128(br, colorMask); \
 \
    tlAG = _mm_mullo_epi16(tlAG, idxidy); \
    tlRB = _mm_mullo_epi16(tlRB, idxidy); \
    trAG = _mm_mullo_epi16(trAG, dxidy); \
    trRB = _mm_mullo_epi16(trRB, dxidy); \
    blAG = _mm_mullo_epi16(blAG, idxdy); \
    blRB = _mm_mullo_epi16(blRB, idxdy); \
    brAG = _mm_mullo_epi16(brAG, dxdy); \
    brRB = _mm_mullo_epi16(brRB, dxdy); \
 \
    /* Add the values, and shift to only keep 8 significant bits per colors */ \
    __m128i rAG =_mm_add_epi16(_mm_add_epi16(tlAG, trAG), _mm_add_epi16(blAG, brAG)); \
    __m128i rRB =_mm_add_epi16(_mm_add_epi16(tlRB, trRB), _mm_add_epi16(blRB, brRB)); \
    rAG = _mm_andnot_si128(colorMask, rAG); \
    rRB = _mm_srli_epi16(rRB, 8); \
    _mm_storeu_si128((__m128i*)(b), _mm_or_si128(rAG, rRB)); \
}
#endif

#if defined(QT_ALWAYS_HAVE_NEON)
#define interpolate_4_pixels_16_neon(tl, tr, bl, br, distx, disty, disty_, colorMask, invColorMask, v_256, b)  \
{ \
    const int16x8_t dxdy = vmulq_s16(distx, disty); \
    const int16x8_t distx_ = vshlq_n_s16(distx, 4); \
    const int16x8_t idxidy =  vaddq_s16(dxdy, vsubq_s16(v_256, vaddq_s16(distx_, disty_))); \
    const int16x8_t dxidy =  vsubq_s16(distx_, dxdy); \
    const int16x8_t idxdy =  vsubq_s16(disty_, dxdy); \
 \
    int16x8_t tlAG = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(tl), 8)); \
    int16x8_t tlRB = vandq_s16(tl, colorMask); \
    int16x8_t trAG = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(tr), 8)); \
    int16x8_t trRB = vandq_s16(tr, colorMask); \
    int16x8_t blAG = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(bl), 8)); \
    int16x8_t blRB = vandq_s16(bl, colorMask); \
    int16x8_t brAG = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(br), 8)); \
    int16x8_t brRB = vandq_s16(br, colorMask); \
 \
    int16x8_t rAG = vmulq_s16(tlAG, idxidy); \
    int16x8_t rRB = vmulq_s16(tlRB, idxidy); \
    rAG = vmlaq_s16(rAG, trAG, dxidy); \
    rRB = vmlaq_s16(rRB, trRB, dxidy); \
    rAG = vmlaq_s16(rAG, blAG, idxdy); \
    rRB = vmlaq_s16(rRB, blRB, idxdy); \
    rAG = vmlaq_s16(rAG, brAG, dxdy); \
    rRB = vmlaq_s16(rRB, brRB, dxdy); \
 \
    rAG = vandq_s16(invColorMask, rAG); \
    rRB = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(rRB), 8)); \
    vst1q_s16((int16_t*)(b), vorrq_s16(rAG, rRB)); \
}
#endif

template<TextureBlendType blendType>
Q_STATIC_TEMPLATE_FUNCTION inline void fetchTransformedBilinear_pixelBounds(int max, int l1, int l2, int &v1, int &v2)
{
    if (blendType == BlendTransformedBilinearTiled) {
        v1 %= max;
        if (v1 < 0) v1 += max;
        v2 = v1 + 1;
        v2 %= max;
    } else {
        if (v1 < l1) {
            v2 = v1 = l1;
        } else if (v1 >= l2) {
            v2 = v1 = l2;
        } else {
            v2 = v1 + 1;
        }
    }

    Q_ASSERT(v1 >= 0 && v1 < max);
    Q_ASSERT(v2 >= 0 && v2 < max);
}

template<TextureBlendType blendType, QImage::Format format> /* blendType = BlendTransformedBilinear or BlendTransformedBilinearTiled */
Q_STATIC_TEMPLATE_FUNCTION
const uint * QT_FASTCALL fetchTransformedBilinear(uint *buffer, const Operator *, const QSpanData *data,
                                                  int y, int x, int length)
{
#ifdef Q_CC_RVCT // needed to avoid compiler crash in RVCT 2.2
    FetchPixelProc fetch;
    if (format != QImage::Format_Invalid)
        fetch = qt_fetchPixel<format>;
    else
        fetch = fetchPixelProc[data->texture.format];
#else
    FetchPixelProc fetch = (format != QImage::Format_Invalid) ? FetchPixelProc(qt_fetchPixel<format>) : fetchPixelProc[data->texture.format];
#endif

    int image_width = data->texture.width;
    int image_height = data->texture.height;

    int image_x1 = data->texture.x1;
    int image_y1 = data->texture.y1;
    int image_x2 = data->texture.x2 - 1;
    int image_y2 = data->texture.y2 - 1;

    const qreal cx = x + qreal(0.5);
    const qreal cy = y + qreal(0.5);

    uint *end = buffer + length;
    uint *b = buffer;
    if (data->fast_matrix) {
        // The increment pr x in the scanline
        int fdx = (int)(data->m11 * fixed_scale);
        int fdy = (int)(data->m12 * fixed_scale);

        int fx = int((data->m21 * cy
                      + data->m11 * cx + data->dx) * fixed_scale);
        int fy = int((data->m22 * cy
                      + data->m12 * cx + data->dy) * fixed_scale);

        fx -= half_point;
        fy -= half_point;

        if (fdy == 0) { //simple scale, no rotation
            int y1 = (fy >> 16);
            int y2;
            fetchTransformedBilinear_pixelBounds<blendType>(image_height, image_y1, image_y2, y1, y2);
            const uchar *s1 = data->texture.scanLine(y1);
            const uchar *s2 = data->texture.scanLine(y2);

            if (fdx <= fixed_scale && fdx > 0) { // scale up on X
                int disty = (fy & 0x0000ffff) >> 8;
                int idisty = 256 - disty;
                int x = fx >> 16;

                // The idea is first to do the interpolation between the row s1 and the row s2
                // into an intermediate buffer, then we interpolate between two pixel of this buffer.

                // intermediate_buffer[0] is a buffer of red-blue component of the pixel, in the form 0x00RR00BB
                // intermediate_buffer[1] is the alpha-green component of the pixel, in the form 0x00AA00GG
                quint32 intermediate_buffer[2][buffer_size + 2];
                // count is the size used in the intermediate_buffer.
                int count = qCeil(length * data->m11) + 2; //+1 for the last pixel to interpolate with, and +1 for rounding errors.
                Q_ASSERT(count <= buffer_size + 2); //length is supposed to be <= buffer_size and data->m11 < 1 in this case
                int f = 0;
                int lim = count;
                if (blendType == BlendTransformedBilinearTiled) {
                    x %= image_width;
                    if (x < 0) x += image_width;
                } else {
                    lim = qMin(count, image_x2-x+1);
                    if (x < image_x1) {
                        Q_ASSERT(x <= image_x2);
                        uint t = fetch(s1, image_x1, data->texture.colorTable);
                        uint b = fetch(s2, image_x1, data->texture.colorTable);
                        quint32 rb = (((t & 0xff00ff) * idisty + (b & 0xff00ff) * disty) >> 8) & 0xff00ff;
                        quint32 ag = ((((t>>8) & 0xff00ff) * idisty + ((b>>8) & 0xff00ff) * disty) >> 8) & 0xff00ff;
                        do {
                            intermediate_buffer[0][f] = rb;
                            intermediate_buffer[1][f] = ag;
                            f++;
                            x++;
                        } while (x < image_x1 && f < lim);
                    }
                }

                if (blendType != BlendTransformedBilinearTiled &&
                        (format == QImage::Format_ARGB32_Premultiplied || format == QImage::Format_RGB32)) {
#if defined(QT_ALWAYS_HAVE_SSE2)
                    const __m128i disty_ = _mm_set1_epi16(disty);
                    const __m128i idisty_ = _mm_set1_epi16(idisty);
                    const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);

                    lim -= 3;
                    for (; f < lim; x += 4, f += 4) {
                        // Load 4 pixels from s1, and split the alpha-green and red-blue component
                        __m128i top = _mm_loadu_si128((__m128i*)((const uint *)(s1)+x));
                        __m128i topAG = _mm_srli_epi16(top, 8);
                        __m128i topRB = _mm_and_si128(top, colorMask);
                        // Multiplies each colour component by idisty
                        topAG = _mm_mullo_epi16 (topAG, idisty_);
                        topRB = _mm_mullo_epi16 (topRB, idisty_);

                        // Same for the s2 vector
                        __m128i bottom = _mm_loadu_si128((__m128i*)((const uint *)(s2)+x));
                        __m128i bottomAG = _mm_srli_epi16(bottom, 8);
                        __m128i bottomRB = _mm_and_si128(bottom, colorMask);
                        bottomAG = _mm_mullo_epi16 (bottomAG, disty_);
                        bottomRB = _mm_mullo_epi16 (bottomRB, disty_);

                        // Add the values, and shift to only keep 8 significant bits per colors
                        __m128i rAG =_mm_add_epi16(topAG, bottomAG);
                        rAG = _mm_srli_epi16(rAG, 8);
                        _mm_storeu_si128((__m128i*)(&intermediate_buffer[1][f]), rAG);
                        __m128i rRB =_mm_add_epi16(topRB, bottomRB);
                        rRB = _mm_srli_epi16(rRB, 8);
                        _mm_storeu_si128((__m128i*)(&intermediate_buffer[0][f]), rRB);
                    }
#elif defined(QT_ALWAYS_HAVE_NEON)
                    const int16x8_t disty_ = vdupq_n_s16(disty);
                    const int16x8_t idisty_ = vdupq_n_s16(idisty);
                    const int16x8_t colorMask = vdupq_n_s16(0x00ff);

                    lim -= 3;
                    for (; f < lim; x += 4, f += 4) {
                        // Load 4 pixels from s1, and split the alpha-green and red-blue component
                        int16x8_t top = vld1q_s16((int16_t*)((const uint *)(s1)+x));
                        int16x8_t topAG = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(top), 8));
                        int16x8_t topRB = vandq_s16(top, colorMask);
                        // Multiplies each colour component by idisty
                        topAG = vmulq_s16(topAG, idisty_);
                        topRB = vmulq_s16(topRB, idisty_);

                        // Same for the s2 vector
                        int16x8_t bottom = vld1q_s16((int16_t*)((const uint *)(s2)+x));
                        int16x8_t bottomAG = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(bottom), 8));
                        int16x8_t bottomRB = vandq_s16(bottom, colorMask);
                        bottomAG = vmulq_s16(bottomAG, disty_);
                        bottomRB = vmulq_s16(bottomRB, disty_);

                        // Add the values, and shift to only keep 8 significant bits per colors
                        int16x8_t rAG = vaddq_s16(topAG, bottomAG);
                        rAG = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(rAG), 8));
                        vst1q_s16((int16_t*)(&intermediate_buffer[1][f]), rAG);
                        int16x8_t rRB = vaddq_s16(topRB, bottomRB);
                        rRB = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(rRB), 8));
                        vst1q_s16((int16_t*)(&intermediate_buffer[0][f]), rRB);
                    }
#endif
                }
                for (; f < count; f++) { // Same as above but without sse2
                    if (blendType == BlendTransformedBilinearTiled) {
                        if (x >= image_width) x -= image_width;
                    } else {
                        x = qMin(x, image_x2);
                    }

                    uint t = fetch(s1, x, data->texture.colorTable);
                    uint b = fetch(s2, x, data->texture.colorTable);

                    intermediate_buffer[0][f] = (((t & 0xff00ff) * idisty + (b & 0xff00ff) * disty) >> 8) & 0xff00ff;
                    intermediate_buffer[1][f] = ((((t>>8) & 0xff00ff) * idisty + ((b>>8) & 0xff00ff) * disty) >> 8) & 0xff00ff;
                    x++;
                }
                // Now interpolate the values from the intermediate_buffer to get the final result.
                fx &= fixed_scale - 1;
                Q_ASSERT((fx >> 16) == 0);
                while (b < end) {
                    register int x1 = (fx >> 16);
                    register int x2 = x1 + 1;
                    Q_ASSERT(x1 >= 0);
                    Q_ASSERT(x2 < count);

                    register int distx = (fx & 0x0000ffff) >> 8;
                    register int idistx = 256 - distx;
                    int rb = ((intermediate_buffer[0][x1] * idistx + intermediate_buffer[0][x2] * distx) >> 8) & 0xff00ff;
                    int ag = (intermediate_buffer[1][x1] * idistx + intermediate_buffer[1][x2] * distx) & 0xff00ff00;
                    *b = rb | ag;
                    b++;
                    fx += fdx;
                }
            } else if ((fdx < 0 && fdx > -(fixed_scale / 8)) || fabs(data->m22) < (1./8.)) { // scale up more than 8x
                int y1 = (fy >> 16);
                int y2;
                fetchTransformedBilinear_pixelBounds<blendType>(image_height, image_y1, image_y2, y1, y2);
                const uchar *s1 = data->texture.scanLine(y1);
                const uchar *s2 = data->texture.scanLine(y2);
                int disty = (fy & 0x0000ffff) >> 8;
                int idisty = 256 - disty;
                while (b < end) {
                    int x1 = (fx >> 16);
                    int x2;
                    fetchTransformedBilinear_pixelBounds<blendType>(image_width, image_x1, image_x2, x1, x2);
                    uint tl = fetch(s1, x1, data->texture.colorTable);
                    uint tr = fetch(s1, x2, data->texture.colorTable);
                    uint bl = fetch(s2, x1, data->texture.colorTable);
                    uint br = fetch(s2, x2, data->texture.colorTable);

                    int distx = (fx & 0x0000ffff) >> 8;
                    int idistx = 256 - distx;

                    uint xtop = INTERPOLATE_PIXEL_256(tl, idistx, tr, distx);
                    uint xbot = INTERPOLATE_PIXEL_256(bl, idistx, br, distx);
                    *b = INTERPOLATE_PIXEL_256(xtop, idisty, xbot, disty);

                    fx += fdx;
                    ++b;
                }
            } else { //scale down
                int y1 = (fy >> 16);
                int y2;
                fetchTransformedBilinear_pixelBounds<blendType>(image_height, image_y1, image_y2, y1, y2);
                const uchar *s1 = data->texture.scanLine(y1);
                const uchar *s2 = data->texture.scanLine(y2);
                int disty = (fy & 0x0000ffff) >> 12;

                if (blendType != BlendTransformedBilinearTiled &&
                    (format == QImage::Format_ARGB32_Premultiplied || format == QImage::Format_RGB32)) {

#define BILINEAR_DOWNSCALE_BOUNDS_PROLOG \
                    while (b < end) { \
                        int x1 = (fx >> 16); \
                        int x2; \
                        fetchTransformedBilinear_pixelBounds<blendType>(image_width, image_x1, image_x2, x1, x2); \
                        if (x1 != x2) \
                            break; \
                        uint tl = fetch(s1, x1, data->texture.colorTable); \
                        uint tr = fetch(s1, x2, data->texture.colorTable); \
                        uint bl = fetch(s2, x1, data->texture.colorTable); \
                        uint br = fetch(s2, x2, data->texture.colorTable); \
                        int distx = (fx & 0x0000ffff) >> 12; \
                        *b = interpolate_4_pixels_16(tl, tr, bl, br, distx, disty); \
                        fx += fdx; \
                        ++b; \
                    } \
                    uint *boundedEnd; \
                    if (fdx > 0) \
                        boundedEnd = qMin(end, buffer + uint((image_x2 - (fx >> 16)) / data->m11)); \
                    else \
                        boundedEnd = qMin(end, buffer + uint((image_x1 - (fx >> 16)) / data->m11)); \
                    boundedEnd -= 3;

#if defined(QT_ALWAYS_HAVE_SSE2)
                    BILINEAR_DOWNSCALE_BOUNDS_PROLOG

                    const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
                    const __m128i v_256 = _mm_set1_epi16(256);
                    const __m128i v_disty = _mm_set1_epi16(disty);
                    __m128i v_fdx = _mm_set1_epi32(fdx*4);

                    ptrdiff_t secondLine = reinterpret_cast<const uint *>(s2) - reinterpret_cast<const uint *>(s1);

                    union Vect_buffer { __m128i vect; quint32 i[4]; };
                    Vect_buffer v_fx;

                    for (int i = 0; i < 4; i++) {
                        v_fx.i[i] = fx;
                        fx += fdx;
                    }

                    while (b < boundedEnd) {

                        Vect_buffer tl, tr, bl, br;

                        for (int i = 0; i < 4; i++) {
                            int x1 = v_fx.i[i] >> 16;
                            const uint *addr_tl = reinterpret_cast<const uint *>(s1) + x1;
                            const uint *addr_tr = addr_tl + 1;
                            tl.i[i] = *addr_tl;
                            tr.i[i] = *addr_tr;
                            bl.i[i] = *(addr_tl+secondLine);
                            br.i[i] = *(addr_tr+secondLine);
                        }
                        __m128i v_distx = _mm_srli_epi16(v_fx.vect, 12);
                        v_distx = _mm_shufflehi_epi16(v_distx, _MM_SHUFFLE(2,2,0,0));
                        v_distx = _mm_shufflelo_epi16(v_distx, _MM_SHUFFLE(2,2,0,0));

                        interpolate_4_pixels_16_sse2(tl.vect, tr.vect, bl.vect, br.vect, v_distx, v_disty, colorMask, v_256, b);
                        b+=4;
                        v_fx.vect = _mm_add_epi32(v_fx.vect, v_fdx);
                    }
                    fx = v_fx.i[0];
#elif defined(QT_ALWAYS_HAVE_NEON)
                    BILINEAR_DOWNSCALE_BOUNDS_PROLOG

                    const int16x8_t colorMask = vdupq_n_s16(0x00ff);
                    const int16x8_t invColorMask = vmvnq_s16(colorMask);
                    const int16x8_t v_256 = vdupq_n_s16(256);
                    const int16x8_t v_disty = vdupq_n_s16(disty);
                    const int16x8_t v_disty_ = vshlq_n_s16(v_disty, 4);
                    int32x4_t v_fdx = vdupq_n_s32(fdx*4);

                    ptrdiff_t secondLine = reinterpret_cast<const uint *>(s2) - reinterpret_cast<const uint *>(s1);

                    union Vect_buffer { int32x4_t vect; quint32 i[4]; };
                    Vect_buffer v_fx;

                    for (int i = 0; i < 4; i++) {
                        v_fx.i[i] = fx;
                        fx += fdx;
                    }

                    const int32x4_t v_ffff_mask = vdupq_n_s32(0x0000ffff);

                    while (b < boundedEnd) {

                        Vect_buffer tl, tr, bl, br;

                        Vect_buffer v_fx_shifted;
                        v_fx_shifted.vect = vshrq_n_s32(v_fx.vect, 16);

                        int32x4_t v_distx = vshrq_n_s32(vandq_s32(v_fx.vect, v_ffff_mask), 12);

                        for (int i = 0; i < 4; i++) {
                            int x1 = v_fx_shifted.i[i];
                            const uint *addr_tl = reinterpret_cast<const uint *>(s1) + x1;
                            const uint *addr_tr = addr_tl + 1;
                            tl.i[i] = *addr_tl;
                            tr.i[i] = *addr_tr;
                            bl.i[i] = *(addr_tl+secondLine);
                            br.i[i] = *(addr_tr+secondLine);
                        }

                        v_distx = vorrq_s32(v_distx, vshlq_n_s32(v_distx, 16));

                        interpolate_4_pixels_16_neon(vreinterpretq_s16_s32(tl.vect), vreinterpretq_s16_s32(tr.vect), vreinterpretq_s16_s32(bl.vect), vreinterpretq_s16_s32(br.vect), vreinterpretq_s16_s32(v_distx), v_disty, v_disty_, colorMask, invColorMask, v_256, b);
                        b+=4;
                        v_fx.vect = vaddq_s32(v_fx.vect, v_fdx);
                    }
                    fx = v_fx.i[0];
#endif
                }

                while (b < end) {
                    int x1 = (fx >> 16);
                    int x2;
                    fetchTransformedBilinear_pixelBounds<blendType>(image_width, image_x1, image_x2, x1, x2);
                    uint tl = fetch(s1, x1, data->texture.colorTable);
                    uint tr = fetch(s1, x2, data->texture.colorTable);
                    uint bl = fetch(s2, x1, data->texture.colorTable);
                    uint br = fetch(s2, x2, data->texture.colorTable);
                    int distx = (fx & 0x0000ffff) >> 12;
                    *b = interpolate_4_pixels_16(tl, tr, bl, br, distx, disty);
                    fx += fdx;
                    ++b;
                }
            }
        } else { //rotation
            if (fabs(data->m11) > 8 || fabs(data->m22) > 8) {
                //if we are zooming more than 8 times, we use 8bit precision for the position.
                while (b < end) {
                    int x1 = (fx >> 16);
                    int x2;
                    int y1 = (fy >> 16);
                    int y2;

                    fetchTransformedBilinear_pixelBounds<blendType>(image_width, image_x1, image_x2, x1, x2);
                    fetchTransformedBilinear_pixelBounds<blendType>(image_height, image_y1, image_y2, y1, y2);

                    const uchar *s1 = data->texture.scanLine(y1);
                    const uchar *s2 = data->texture.scanLine(y2);

                    uint tl = fetch(s1, x1, data->texture.colorTable);
                    uint tr = fetch(s1, x2, data->texture.colorTable);
                    uint bl = fetch(s2, x1, data->texture.colorTable);
                    uint br = fetch(s2, x2, data->texture.colorTable);

                    int distx = (fx & 0x0000ffff) >> 8;
                    int disty = (fy & 0x0000ffff) >> 8;
                    int idistx = 256 - distx;
                    int idisty = 256 - disty;

                    uint xtop = INTERPOLATE_PIXEL_256(tl, idistx, tr, distx);
                    uint xbot = INTERPOLATE_PIXEL_256(bl, idistx, br, distx);
                    *b = INTERPOLATE_PIXEL_256(xtop, idisty, xbot, disty);

                    fx += fdx;
                    fy += fdy;
                    ++b;
                }
            } else {
                //we are zooming less than 8x, use 4bit precision
                while (b < end) {
                    int x1 = (fx >> 16);
                    int x2;
                    int y1 = (fy >> 16);
                    int y2;

                    fetchTransformedBilinear_pixelBounds<blendType>(image_width, image_x1, image_x2, x1, x2);
                    fetchTransformedBilinear_pixelBounds<blendType>(image_height, image_y1, image_y2, y1, y2);

                    const uchar *s1 = data->texture.scanLine(y1);
                    const uchar *s2 = data->texture.scanLine(y2);

                    uint tl = fetch(s1, x1, data->texture.colorTable);
                    uint tr = fetch(s1, x2, data->texture.colorTable);
                    uint bl = fetch(s2, x1, data->texture.colorTable);
                    uint br = fetch(s2, x2, data->texture.colorTable);

                    int distx = (fx & 0x0000ffff) >> 12;
                    int disty = (fy & 0x0000ffff) >> 12;

                    *b = interpolate_4_pixels_16(tl, tr, bl, br, distx, disty);

                    fx += fdx;
                    fy += fdy;
                    ++b;
                }
            }
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        qreal fx = data->m21 * cy + data->m11 * cx + data->dx;
        qreal fy = data->m22 * cy + data->m12 * cx + data->dy;
        qreal fw = data->m23 * cy + data->m13 * cx + data->m33;

        while (b < end) {
            const qreal iw = fw == 0 ? 1 : 1 / fw;
            const qreal px = fx * iw - qreal(0.5);
            const qreal py = fy * iw - qreal(0.5);

            int x1 = int(px) - (px < 0);
            int x2;
            int y1 = int(py) - (py < 0);
            int y2;

            int distx = int((px - x1) * 256);
            int disty = int((py - y1) * 256);
            int idistx = 256 - distx;
            int idisty = 256 - disty;

            fetchTransformedBilinear_pixelBounds<blendType>(image_width, image_x1, image_x2, x1, x2);
            fetchTransformedBilinear_pixelBounds<blendType>(image_height, image_y1, image_y2, y1, y2);

            const uchar *s1 = data->texture.scanLine(y1);
            const uchar *s2 = data->texture.scanLine(y2);

            uint tl = fetch(s1, x1, data->texture.colorTable);
            uint tr = fetch(s1, x2, data->texture.colorTable);
            uint bl = fetch(s2, x1, data->texture.colorTable);
            uint br = fetch(s2, x2, data->texture.colorTable);

            uint xtop = INTERPOLATE_PIXEL_256(tl, idistx, tr, distx);
            uint xbot = INTERPOLATE_PIXEL_256(bl, idistx, br, distx);
            *b = INTERPOLATE_PIXEL_256(xtop, idisty, xbot, disty);

            fx += fdx;
            fy += fdy;
            fw += fdw;
            //force increment to avoid /0
            if (!fw) {
                fw += fdw;
            }
            ++b;
        }
    }

    return buffer;
}

#define SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Arg) qt_fetchUntransformed<QImage::Arg>

static const SourceFetchProc sourceFetch[NBlendTypes][QImage::NImageFormats] = {
    // Untransformed
    {
        0, // Invalid
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_Mono),   // Mono
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_MonoLSB),   // MonoLsb
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_Indexed8),   // Indexed8
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_ARGB32_Premultiplied),   // RGB32
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_ARGB32),   // ARGB32
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_ARGB32_Premultiplied),   // ARGB32_Premultiplied
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_RGB16),   // RGB16
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_ARGB8565_Premultiplied),// ARGB8565_Premultiplied
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_RGB666),  // RGB666
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_ARGB6666_Premultiplied),// ARGB6666_Premultiplied
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_RGB555),  // RGB555
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_ARGB8555_Premultiplied),// ARGB8555_Premultiplied
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_RGB888),  // RGB888
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_RGB444),  // RGB444
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_ARGB4444_Premultiplied) // ARGB4444_Premultiplied
    },
    // Tiled
    {
        0, // Invalid
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_Mono),   // Mono
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_MonoLSB),   // MonoLsb
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_Indexed8),   // Indexed8
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_ARGB32_Premultiplied),   // RGB32
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_ARGB32),   // ARGB32
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_ARGB32_Premultiplied),   // ARGB32_Premultiplied
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_RGB16),   // RGB16
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_ARGB8565_Premultiplied),// ARGB8565_Premultiplied
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_RGB666),  // RGB666
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_ARGB6666_Premultiplied),// ARGB6666_Premultiplied
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_RGB555),  // RGB555
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_ARGB8555_Premultiplied),// ARGB8555_Premultiplied
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_RGB888),  // RGB888
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_RGB444),  // RGB444
        SPANFUNC_POINTER_FETCHHUNTRANSFORMED(Format_ARGB4444_Premultiplied) // ARGB4444_Premultiplied
    },
    // Transformed
    {
        0, // Invalid
        fetchTransformed<BlendTransformed>,   // Mono
        fetchTransformed<BlendTransformed>,   // MonoLsb
        fetchTransformed<BlendTransformed>,   // Indexed8
        fetchTransformed<BlendTransformed>,   // RGB32
        fetchTransformed<BlendTransformed>,   // ARGB32
        fetchTransformed<BlendTransformed>,   // ARGB32_Premultiplied
        fetchTransformed<BlendTransformed>,   // RGB16
        fetchTransformed<BlendTransformed>,   // ARGB8565_Premultiplied
        fetchTransformed<BlendTransformed>,   // RGB666
        fetchTransformed<BlendTransformed>,   // ARGB6666_Premultiplied
        fetchTransformed<BlendTransformed>,   // RGB555
        fetchTransformed<BlendTransformed>,   // ARGB8555_Premultiplied
        fetchTransformed<BlendTransformed>,   // RGB888
        fetchTransformed<BlendTransformed>,   // RGB444
        fetchTransformed<BlendTransformed>,   // ARGB4444_Premultiplied
    },
    {
        0, // TransformedTiled
        fetchTransformed<BlendTransformedTiled>,   // Mono
        fetchTransformed<BlendTransformedTiled>,   // MonoLsb
        fetchTransformed<BlendTransformedTiled>,   // Indexed8
        fetchTransformed<BlendTransformedTiled>,   // RGB32
        fetchTransformed<BlendTransformedTiled>,   // ARGB32
        fetchTransformed<BlendTransformedTiled>,   // ARGB32_Premultiplied
        fetchTransformed<BlendTransformedTiled>,   // RGB16
        fetchTransformed<BlendTransformedTiled>,   // ARGB8565_Premultiplied
        fetchTransformed<BlendTransformedTiled>,   // RGB666
        fetchTransformed<BlendTransformedTiled>,   // ARGB6666_Premultiplied
        fetchTransformed<BlendTransformedTiled>,   // RGB555
        fetchTransformed<BlendTransformedTiled>,   // ARGB8555_Premultiplied
        fetchTransformed<BlendTransformedTiled>,   // RGB888
        fetchTransformed<BlendTransformedTiled>,   // RGB444
        fetchTransformed<BlendTransformedTiled>,   // ARGB4444_Premultiplied
    },
    {
        0, // Bilinear
        fetchTransformedBilinear<BlendTransformedBilinear, QImage::Format_Invalid>,   // Mono
        fetchTransformedBilinear<BlendTransformedBilinear, QImage::Format_Invalid>,   // MonoLsb
        fetchTransformedBilinear<BlendTransformedBilinear, QImage::Format_Invalid>,   // Indexed8
        fetchTransformedBilinear<BlendTransformedBilinear, QImage::Format_ARGB32_Premultiplied>,   // RGB32
        fetchTransformedBilinear<BlendTransformedBilinear, QImage::Format_ARGB32>,   // ARGB32
        fetchTransformedBilinear<BlendTransformedBilinear, QImage::Format_ARGB32_Premultiplied>,   // ARGB32_Premultiplied
        fetchTransformedBilinear<BlendTransformedBilinear, QImage::Format_Invalid>,   // RGB16
        fetchTransformedBilinear<BlendTransformedBilinear, QImage::Format_Invalid>,   // ARGB8565_Premultiplied
        fetchTransformedBilinear<BlendTransformedBilinear, QImage::Format_Invalid>,   // RGB666
        fetchTransformedBilinear<BlendTransformedBilinear, QImage::Format_Invalid>,   // ARGB6666_Premultiplied
        fetchTransformedBilinear<BlendTransformedBilinear, QImage::Format_Invalid>,   // RGB555
        fetchTransformedBilinear<BlendTransformedBilinear, QImage::Format_Invalid>,   // ARGB8555_Premultiplied
        fetchTransformedBilinear<BlendTransformedBilinear, QImage::Format_Invalid>,   // RGB888
        fetchTransformedBilinear<BlendTransformedBilinear, QImage::Format_Invalid>,   // RGB444
        fetchTransformedBilinear<BlendTransformedBilinear, QImage::Format_Invalid>    // ARGB4444_Premultiplied
    },
    {
        0, // BilinearTiled
        fetchTransformedBilinear<BlendTransformedBilinearTiled, QImage::Format_Invalid>,   // Mono
        fetchTransformedBilinear<BlendTransformedBilinearTiled, QImage::Format_Invalid>,   // MonoLsb
        fetchTransformedBilinear<BlendTransformedBilinearTiled, QImage::Format_Invalid>,   // Indexed8
        fetchTransformedBilinear<BlendTransformedBilinearTiled, QImage::Format_ARGB32_Premultiplied>,   // RGB32
        fetchTransformedBilinear<BlendTransformedBilinearTiled, QImage::Format_ARGB32>,   // ARGB32
        fetchTransformedBilinear<BlendTransformedBilinearTiled, QImage::Format_ARGB32_Premultiplied>,   // ARGB32_Premultiplied
        fetchTransformedBilinear<BlendTransformedBilinearTiled, QImage::Format_Invalid>,   // RGB16
        fetchTransformedBilinear<BlendTransformedBilinearTiled, QImage::Format_Invalid>,   // ARGB8565_Premultiplied
        fetchTransformedBilinear<BlendTransformedBilinearTiled, QImage::Format_Invalid>,   // RGB666
        fetchTransformedBilinear<BlendTransformedBilinearTiled, QImage::Format_Invalid>,   // ARGB6666_Premultiplied
        fetchTransformedBilinear<BlendTransformedBilinearTiled, QImage::Format_Invalid>,   // RGB555
        fetchTransformedBilinear<BlendTransformedBilinearTiled, QImage::Format_Invalid>,   // ARGB8555_Premultiplied
        fetchTransformedBilinear<BlendTransformedBilinearTiled, QImage::Format_Invalid>,   // RGB888
        fetchTransformedBilinear<BlendTransformedBilinearTiled, QImage::Format_Invalid>,   // RGB444
        fetchTransformedBilinear<BlendTransformedBilinearTiled, QImage::Format_Invalid>    // ARGB4444_Premultiplied
    },
};

#define FIXPT_BITS 8
#define FIXPT_SIZE (1<<FIXPT_BITS)

static uint qt_gradient_pixel_fixed(const QGradientData *data, int fixed_pos)
{
    int ipos = (fixed_pos + (FIXPT_SIZE / 2)) >> FIXPT_BITS;
    return data->colorTable[qt_gradient_clamp(data, ipos)];
}

static void QT_FASTCALL getLinearGradientValues(LinearGradientValues *v, const QSpanData *data)
{
    v->dx = data->gradient.linear.end.x - data->gradient.linear.origin.x;
    v->dy = data->gradient.linear.end.y - data->gradient.linear.origin.y;
    v->l = v->dx * v->dx + v->dy * v->dy;
    v->off = 0;
    if (v->l != 0) {
        v->dx /= v->l;
        v->dy /= v->l;
        v->off = -v->dx * data->gradient.linear.origin.x - v->dy * data->gradient.linear.origin.y;
    }
}

static const uint * QT_FASTCALL qt_fetch_linear_gradient(uint *buffer, const Operator *op, const QSpanData *data,
                                                         int y, int x, int length)
{
    const uint *b = buffer;
    qreal t, inc;

    bool affine = true;
    qreal rx=0, ry=0;
    if (op->linear.l == 0) {
        t = inc = 0;
    } else {
        rx = data->m21 * (y + qreal(0.5)) + data->m11 * (x + qreal(0.5)) + data->dx;
        ry = data->m22 * (y + qreal(0.5)) + data->m12 * (x + qreal(0.5)) + data->dy;
        t = op->linear.dx*rx + op->linear.dy*ry + op->linear.off;
        inc = op->linear.dx * data->m11 + op->linear.dy * data->m12;
        affine = !data->m13 && !data->m23;

        if (affine) {
            t *= (GRADIENT_STOPTABLE_SIZE - 1);
            inc *= (GRADIENT_STOPTABLE_SIZE - 1);
        }
    }

    const uint *end = buffer + length;
    if (affine) {
        if (inc > qreal(-1e-5) && inc < qreal(1e-5)) {
            QT_MEMFILL_UINT(buffer, length, qt_gradient_pixel_fixed(&data->gradient, int(t * FIXPT_SIZE)));
        } else {
            if (t+inc*length < qreal(INT_MAX >> (FIXPT_BITS + 1)) &&
                t+inc*length > qreal(INT_MIN >> (FIXPT_BITS + 1))) {
                // we can use fixed point math
                int t_fixed = int(t * FIXPT_SIZE);
                int inc_fixed = int(inc * FIXPT_SIZE);
                while (buffer < end) {
                    *buffer = qt_gradient_pixel_fixed(&data->gradient, t_fixed);
                    t_fixed += inc_fixed;
                    ++buffer;
                }
            } else {
                // we have to fall back to float math
                while (buffer < end) {
                    *buffer = qt_gradient_pixel(&data->gradient, t/GRADIENT_STOPTABLE_SIZE);
                    t += inc;
                    ++buffer;
                }
            }
        }
    } else { // fall back to float math here as well
        qreal rw = data->m23 * (y + qreal(0.5)) + data->m13 * (x + qreal(0.5)) + data->m33;
        while (buffer < end) {
            qreal x = rx/rw;
            qreal y = ry/rw;
            t = (op->linear.dx*x + op->linear.dy *y) + op->linear.off;

            *buffer = qt_gradient_pixel(&data->gradient, t);
            rx += data->m11;
            ry += data->m12;
            rw += data->m13;
            if (!rw) {
                rw += data->m13;
            }
            ++buffer;
        }
    }

    return b;
}

static void QT_FASTCALL getRadialGradientValues(RadialGradientValues *v, const QSpanData *data)
{
    v->dx = data->gradient.radial.center.x - data->gradient.radial.focal.x;
    v->dy = data->gradient.radial.center.y - data->gradient.radial.focal.y;

    v->dr = data->gradient.radial.center.radius - data->gradient.radial.focal.radius;
    v->sqrfr = data->gradient.radial.focal.radius * data->gradient.radial.focal.radius;

    v->a = v->dr * v->dr - v->dx*v->dx - v->dy*v->dy;
    v->inv2a = 1 / (2 * v->a);

    v->extended = !qFuzzyIsNull(data->gradient.radial.focal.radius) || v->a <= 0;
}

class RadialFetchPlain
{
public:
    static inline void fetch(uint *buffer, uint *end, const Operator *op, const QSpanData *data, qreal det,
                             qreal delta_det, qreal delta_delta_det, qreal b, qreal delta_b)
    {
        if (op->radial.extended) {
            while (buffer < end) {
                quint32 result = 0;
                if (det >= 0) {
                    qreal w = qSqrt(det) - b;
                    if (data->gradient.radial.focal.radius + op->radial.dr * w >= 0)
                        result = qt_gradient_pixel(&data->gradient, w);
                }

                *buffer = result;

                det += delta_det;
                delta_det += delta_delta_det;
                b += delta_b;

                ++buffer;
            }
        } else {
            while (buffer < end) {
                *buffer++ = qt_gradient_pixel(&data->gradient, qSqrt(det) - b);

                det += delta_det;
                delta_det += delta_delta_det;
                b += delta_b;
            }
        }
    }
};

const uint * QT_FASTCALL qt_fetch_radial_gradient_plain(uint *buffer, const Operator *op, const QSpanData *data,
                                                        int y, int x, int length)
{
    return qt_fetch_radial_gradient_template<RadialFetchPlain>(buffer, op, data, y, x, length);
}

static SourceFetchProc qt_fetch_radial_gradient = qt_fetch_radial_gradient_plain;

static const uint * QT_FASTCALL qt_fetch_conical_gradient(uint *buffer, const Operator *, const QSpanData *data,
                                                          int y, int x, int length)
{
    const uint *b = buffer;
    qreal rx = data->m21 * (y + qreal(0.5))
               + data->dx + data->m11 * (x + qreal(0.5));
    qreal ry = data->m22 * (y + qreal(0.5))
               + data->dy + data->m12 * (x + qreal(0.5));
    bool affine = !data->m13 && !data->m23;

    const uint *end = buffer + length;
    if (affine) {
        rx -= data->gradient.conical.center.x;
        ry -= data->gradient.conical.center.y;
        while (buffer < end) {
            qreal angle = qAtan2(ry, rx) + data->gradient.conical.angle;

            *buffer = qt_gradient_pixel(&data->gradient, 1 - angle / (2*Q_PI));

            rx += data->m11;
            ry += data->m12;
            ++buffer;
        }
    } else {
        qreal rw = data->m23 * (y + qreal(0.5))
                   + data->m33 + data->m13 * (x + qreal(0.5));
        if (!rw)
            rw = 1;
        while (buffer < end) {
            qreal angle = qAtan2(ry/rw - data->gradient.conical.center.x,
                                rx/rw - data->gradient.conical.center.y)
                          + data->gradient.conical.angle;

            *buffer = qt_gradient_pixel(&data->gradient, 1. - angle / (2*Q_PI));

            rx += data->m11;
            ry += data->m12;
            rw += data->m13;
            if (!rw) {
                rw += data->m13;
            }
            ++buffer;
        }
    }
    return b;
}

#if defined(Q_CC_RVCT)
// Force ARM code generation for comp_func_* -methods
#  pragma push
#  pragma arm
#  if defined(QT_HAVE_ARMV6)
static __forceinline void preload(const uint *start)
{
    asm( "pld [start]" );
}
static const uint L2CacheLineLength = 32;
static const uint L2CacheLineLengthInInts = L2CacheLineLength/sizeof(uint);
#    define PRELOAD_INIT(x) preload(x);
#    define PRELOAD_INIT2(x,y) PRELOAD_INIT(x) PRELOAD_INIT(y)
#    define PRELOAD_COND(x) if (((uint)&x[i])%L2CacheLineLength == 0) preload(&x[i] + L2CacheLineLengthInInts);
// Two consecutive preloads stall, so space them out a bit by using different modulus.
#    define PRELOAD_COND2(x,y) if (((uint)&x[i])%L2CacheLineLength == 0) preload(&x[i] + L2CacheLineLengthInInts); \
         if (((uint)&y[i])%L2CacheLineLength == 16) preload(&y[i] + L2CacheLineLengthInInts);
#  endif // QT_HAVE_ARMV6
#endif // Q_CC_RVCT

#if !defined(Q_CC_RVCT) || !defined(QT_HAVE_ARMV6)
#    define PRELOAD_INIT(x)
#    define PRELOAD_INIT2(x,y)
#    define PRELOAD_COND(x)
#    define PRELOAD_COND2(x,y)
#endif

/* The constant alpha factor describes an alpha factor that gets applied
   to the result of the composition operation combining it with the destination.

   The intent is that if const_alpha == 0. we get back dest, and if const_alpha == 1.
   we get the unmodified operation

   result = src op dest
   dest = result * const_alpha + dest * (1. - const_alpha)

   This means that in the comments below, the first line is the const_alpha==255 case, the
   second line the general one.

   In the lines below:
   s == src, sa == alpha(src), sia = 1 - alpha(src)
   d == dest, da == alpha(dest), dia = 1 - alpha(dest)
   ca = const_alpha, cia = 1 - const_alpha

   The methods exist in two variants. One where we have a constant source, the other
   where the source is an array of pixels.
*/

/*
  result = 0
  d = d * cia
*/
#define comp_func_Clear_impl(dest, length, const_alpha)\
{\
    if (const_alpha == 255) {\
        QT_MEMFILL_UINT(dest, length, 0);\
    } else {\
        int ialpha = 255 - const_alpha;\
        PRELOAD_INIT(dest)\
        for (int i = 0; i < length; ++i) {\
            PRELOAD_COND(dest)\
            dest[i] = BYTE_MUL(dest[i], ialpha);\
        }\
    }\
}

void QT_FASTCALL comp_func_solid_Clear(uint *dest, int length, uint, uint const_alpha)
{
    comp_func_Clear_impl(dest, length, const_alpha);
}

void QT_FASTCALL comp_func_Clear(uint *dest, const uint *, int length, uint const_alpha)
{
    comp_func_Clear_impl(dest, length, const_alpha);
}

/*
  result = s
  dest = s * ca + d * cia
*/
void QT_FASTCALL comp_func_solid_Source(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255) {
        QT_MEMFILL_UINT(dest, length, color);
    } else {
        int ialpha = 255 - const_alpha;
        color = BYTE_MUL(color, const_alpha);
        PRELOAD_INIT(dest)
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND(dest)
            dest[i] = color + BYTE_MUL(dest[i], ialpha);
        }
    }
}

void QT_FASTCALL comp_func_Source(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        ::memcpy(dest, src, length * sizeof(uint));
    } else {
        int ialpha = 255 - const_alpha;
        PRELOAD_INIT2(dest, src)
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            dest[i] = INTERPOLATE_PIXEL_255(src[i], const_alpha, dest[i], ialpha);
        }
    }
}

void QT_FASTCALL comp_func_solid_Destination(uint *, int, uint, uint)
{
}

void QT_FASTCALL comp_func_Destination(uint *, const uint *, int, uint)
{
}

/*
  result = s + d * sia
  dest = (s + d * sia) * ca + d * cia
       = s * ca + d * (sia * ca + cia)
       = s * ca + d * (1 - sa*ca)
*/
void QT_FASTCALL comp_func_solid_SourceOver(uint *dest, int length, uint color, uint const_alpha)
{
    if ((const_alpha & qAlpha(color)) == 255) {
        QT_MEMFILL_UINT(dest, length, color);
    } else {
        if (const_alpha != 255)
            color = BYTE_MUL(color, const_alpha);
        PRELOAD_INIT(dest)
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND(dest)
            dest[i] = color + BYTE_MUL(dest[i], qAlpha(~color));
        }
    }
}

void QT_FASTCALL comp_func_SourceOver(uint *dest, const uint *src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint s = src[i];
            if (s >= 0xff000000)
                dest[i] = s;
            else if (s != 0)
                dest[i] = s + BYTE_MUL(dest[i], qAlpha(~s));
        }
    } else {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint s = BYTE_MUL(src[i], const_alpha);
            dest[i] = s + BYTE_MUL(dest[i], qAlpha(~s));
        }
    }
}

/*
  result = d + s * dia
  dest = (d + s * dia) * ca + d * cia
       = d + s * dia * ca
*/
void QT_FASTCALL comp_func_solid_DestinationOver(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha != 255)
        color = BYTE_MUL(color, const_alpha);
    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        dest[i] = d + BYTE_MUL(color, qAlpha(~d));
    }
}

void QT_FASTCALL comp_func_DestinationOver(uint *dest, const uint *src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint d = dest[i];
            dest[i] = d + BYTE_MUL(src[i], qAlpha(~d));
        }
    } else {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint d = dest[i];
            uint s = BYTE_MUL(src[i], const_alpha);
            dest[i] = d + BYTE_MUL(s, qAlpha(~d));
        }
    }
}

/*
  result = s * da
  dest = s * da * ca + d * cia
*/
void QT_FASTCALL comp_func_solid_SourceIn(uint *dest, int length, uint color, uint const_alpha)
{
    PRELOAD_INIT(dest)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND(dest)
            dest[i] = BYTE_MUL(color, qAlpha(dest[i]));
        }
    } else {
        color = BYTE_MUL(color, const_alpha);
        uint cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND(dest)
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(color, qAlpha(d), d, cia);
        }
    }
}

void QT_FASTCALL comp_func_SourceIn(uint *dest, const uint *src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            dest[i] = BYTE_MUL(src[i], qAlpha(dest[i]));
        }
    } else {
        uint cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint d = dest[i];
            uint s = BYTE_MUL(src[i], const_alpha);
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(d), d, cia);
        }
    }
}

/*
  result = d * sa
  dest = d * sa * ca + d * cia
       = d * (sa * ca + cia)
*/
void QT_FASTCALL comp_func_solid_DestinationIn(uint *dest, int length, uint color, uint const_alpha)
{
    uint a = qAlpha(color);
    if (const_alpha != 255) {
        a = BYTE_MUL(a, const_alpha) + 255 - const_alpha;
    }
    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        dest[i] = BYTE_MUL(dest[i], a);
    }
}

void QT_FASTCALL comp_func_DestinationIn(uint *dest, const uint *src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            dest[i] = BYTE_MUL(dest[i], qAlpha(src[i]));
        }
    } else {
        int cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint a = BYTE_MUL(qAlpha(src[i]), const_alpha) + cia;
            dest[i] = BYTE_MUL(dest[i], a);
        }
    }
}

/*
  result = s * dia
  dest = s * dia * ca + d * cia
*/

void QT_FASTCALL comp_func_solid_SourceOut(uint *dest, int length, uint color, uint const_alpha)
{
    PRELOAD_INIT(dest)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND(dest)
            dest[i] = BYTE_MUL(color, qAlpha(~dest[i]));
        }
    } else {
        color = BYTE_MUL(color, const_alpha);
        int cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND(dest)
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(color, qAlpha(~d), d, cia);
        }
    }
}

void QT_FASTCALL comp_func_SourceOut(uint *dest, const uint *src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            dest[i] = BYTE_MUL(src[i], qAlpha(~dest[i]));
        }
    } else {
        int cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint s = BYTE_MUL(src[i], const_alpha);
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(~d), d, cia);
        }
    }
}

/*
  result = d * sia
  dest = d * sia * ca + d * cia
       = d * (sia * ca + cia)
*/
void QT_FASTCALL comp_func_solid_DestinationOut(uint *dest, int length, uint color, uint const_alpha)
{
    uint a = qAlpha(~color);
    if (const_alpha != 255)
        a = BYTE_MUL(a, const_alpha) + 255 - const_alpha;
    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        dest[i] = BYTE_MUL(dest[i], a);
    }
}

void QT_FASTCALL comp_func_DestinationOut(uint *dest, const uint *src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            dest[i] = BYTE_MUL(dest[i], qAlpha(~src[i]));
        }
    } else {
        int cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint sia = BYTE_MUL(qAlpha(~src[i]), const_alpha) + cia;
            dest[i] = BYTE_MUL(dest[i], sia);
        }
    }
}

/*
  result = s*da + d*sia
  dest = s*da*ca + d*sia*ca + d *cia
       = s*ca * da + d * (sia*ca + cia)
       = s*ca * da + d * (1 - sa*ca)
*/
void QT_FASTCALL comp_func_solid_SourceAtop(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha != 255) {
        color = BYTE_MUL(color, const_alpha);
    }
    uint sia = qAlpha(~color);
    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        dest[i] = INTERPOLATE_PIXEL_255(color, qAlpha(dest[i]), dest[i], sia);
    }
}

void QT_FASTCALL comp_func_SourceAtop(uint *dest, const uint *src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint s = src[i];
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(d), d, qAlpha(~s));
        }
    } else {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint s = BYTE_MUL(src[i], const_alpha);
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(d), d, qAlpha(~s));
        }
    }
}

/*
  result = d*sa + s*dia
  dest = d*sa*ca + s*dia*ca + d *cia
       = s*ca * dia + d * (sa*ca + cia)
*/
void QT_FASTCALL comp_func_solid_DestinationAtop(uint *dest, int length, uint color, uint const_alpha)
{
    uint a = qAlpha(color);
    if (const_alpha != 255) {
        color = BYTE_MUL(color, const_alpha);
        a = qAlpha(color) + 255 - const_alpha;
    }
    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        dest[i] = INTERPOLATE_PIXEL_255(d, a, color, qAlpha(~d));
    }
}

void QT_FASTCALL comp_func_DestinationAtop(uint *dest, const uint *src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint s = src[i];
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(d, qAlpha(s), s, qAlpha(~d));
        }
    } else {
        int cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint s = BYTE_MUL(src[i], const_alpha);
            uint d = dest[i];
            uint a = qAlpha(s) + cia;
            dest[i] = INTERPOLATE_PIXEL_255(d, a, s, qAlpha(~d));
        }
    }
}

/*
  result = d*sia + s*dia
  dest = d*sia*ca + s*dia*ca + d *cia
       = s*ca * dia + d * (sia*ca + cia)
       = s*ca * dia + d * (1 - sa*ca)
*/
void QT_FASTCALL comp_func_solid_XOR(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha != 255)
        color = BYTE_MUL(color, const_alpha);
    uint sia = qAlpha(~color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        dest[i] = INTERPOLATE_PIXEL_255(color, qAlpha(~d), d, sia);
    }
}

void QT_FASTCALL comp_func_XOR(uint *dest, const uint *src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint d = dest[i];
            uint s = src[i];
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(~d), d, qAlpha(~s));
        }
    } else {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint d = dest[i];
            uint s = BYTE_MUL(src[i], const_alpha);
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(~d), d, qAlpha(~s));
        }
    }
}

struct QFullCoverage {
    inline void store(uint *dest, const uint src) const
    {
        *dest = src;
    }
};

struct QPartialCoverage {
    inline QPartialCoverage(uint const_alpha)
        : ca(const_alpha)
        , ica(255 - const_alpha)
    {
    }

    inline void store(uint *dest, const uint src) const
    {
        *dest = INTERPOLATE_PIXEL_255(src, ca, *dest, ica);
    }

private:
    const uint ca;
    const uint ica;
};

static inline int mix_alpha(int da, int sa)
{
    return 255 - ((255 - sa) * (255 - da) >> 8);
}

/*
    Dca' = Sca.Da + Dca.Sa + Sca.(1 - Da) + Dca.(1 - Sa)
         = Sca + Dca
*/
template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Plus_impl(uint *dest, int length, uint color, const T &coverage)
{
    uint s = color;

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        d = comp_func_Plus_one_pixel(d, s);
        coverage.store(&dest[i], d);
    }
}

void QT_FASTCALL comp_func_solid_Plus(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Plus_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Plus_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Plus_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        d = comp_func_Plus_one_pixel(d, s);

        coverage.store(&dest[i], d);
    }
}

void QT_FASTCALL comp_func_Plus(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Plus_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Plus_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
    Dca' = Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int multiply_op(int dst, int src, int da, int sa)
{
    return qt_div_255(src * dst + src * (255 - da) + dst * (255 - sa));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Multiply_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) multiply_op(a, b, da, sa)
        int r = OP(  qRed(d), sr);
        int b = OP( qBlue(d), sb);
        int g = OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Multiply(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Multiply_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Multiply_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Multiply_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) multiply_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Multiply(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Multiply_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Multiply_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
    Dca' = (Sca.Da + Dca.Sa - Sca.Dca) + Sca.(1 - Da) + Dca.(1 - Sa)
         = Sca + Dca - Sca.Dca
*/
template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Screen_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) 255 - qt_div_255((255-a) * (255-b))
        int r = OP(  qRed(d), sr);
        int b = OP( qBlue(d), sb);
        int g = OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Screen(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Screen_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Screen_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Screen_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) 255 - (((255-a) * (255-b)) >> 8)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Screen(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Screen_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Screen_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
    if 2.Dca < Da
        Dca' = 2.Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
    otherwise
        Dca' = Sa.Da - 2.(Da - Dca).(Sa - Sca) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int overlay_op(int dst, int src, int da, int sa)
{
    const int temp = src * (255 - da) + dst * (255 - sa);
    if (2 * dst < da)
        return qt_div_255(2 * src * dst + temp);
    else
        return qt_div_255(sa * da - 2 * (da - dst) * (sa - src) + temp);
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Overlay_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) overlay_op(a, b, da, sa)
        int r = OP(  qRed(d), sr);
        int b = OP( qBlue(d), sb);
        int g = OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Overlay(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Overlay_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Overlay_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Overlay_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) overlay_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Overlay(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Overlay_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Overlay_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
    Dca' = min(Sca.Da, Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
    Da'  = Sa + Da - Sa.Da
*/
static inline int darken_op(int dst, int src, int da, int sa)
{
    return qt_div_255(qMin(src * da, dst * sa) + src * (255 - da) + dst * (255 - sa));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Darken_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) darken_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Darken(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Darken_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Darken_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Darken_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) darken_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Darken(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Darken_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Darken_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
   Dca' = max(Sca.Da, Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
   Da'  = Sa + Da - Sa.Da
*/
static inline int lighten_op(int dst, int src, int da, int sa)
{
    return qt_div_255(qMax(src * da, dst * sa) + src * (255 - da) + dst * (255 - sa));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Lighten_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) lighten_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Lighten(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Lighten_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Lighten_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Lighten_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) lighten_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Lighten(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Lighten_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Lighten_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
   if Sca.Da + Dca.Sa >= Sa.Da
       Dca' = Sa.Da + Sca.(1 - Da) + Dca.(1 - Sa)
   otherwise
       Dca' = Dca.Sa/(1-Sca/Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int color_dodge_op(int dst, int src, int da, int sa)
{
    const int sa_da = sa * da;
    const int dst_sa = dst * sa;
    const int src_da = src * da;

    const int temp = src * (255 - da) + dst * (255 - sa);
    if (src_da + dst_sa >= sa_da)
        return qt_div_255(sa_da + temp);
    else
        return qt_div_255(255 * dst_sa / (255 - 255 * src / sa) + temp);
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_ColorDodge_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a,b) color_dodge_op(a, b, da, sa)
        int r = OP(  qRed(d), sr);
        int b = OP( qBlue(d), sb);
        int g = OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_ColorDodge(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_ColorDodge_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_ColorDodge_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_ColorDodge_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) color_dodge_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_ColorDodge(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_ColorDodge_impl(dest, src, length, QFullCoverage());
    else
        comp_func_ColorDodge_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
   if Sca.Da + Dca.Sa <= Sa.Da
       Dca' = Sca.(1 - Da) + Dca.(1 - Sa)
   otherwise
       Dca' = Sa.(Sca.Da + Dca.Sa - Sa.Da)/Sca + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int color_burn_op(int dst, int src, int da, int sa)
{
    const int src_da = src * da;
    const int dst_sa = dst * sa;
    const int sa_da = sa * da;

    const int temp = src * (255 - da) + dst * (255 - sa);

    if (src == 0 || src_da + dst_sa <= sa_da)
        return qt_div_255(temp);
    return qt_div_255(sa * (src_da + dst_sa - sa_da) / src + temp);
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_ColorBurn_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) color_burn_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_ColorBurn(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_ColorBurn_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_ColorBurn_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_ColorBurn_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) color_burn_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_ColorBurn(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_ColorBurn_impl(dest, src, length, QFullCoverage());
    else
        comp_func_ColorBurn_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
    if 2.Sca < Sa
        Dca' = 2.Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
    otherwise
        Dca' = Sa.Da - 2.(Da - Dca).(Sa - Sca) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline uint hardlight_op(int dst, int src, int da, int sa)
{
    const uint temp = src * (255 - da) + dst * (255 - sa);

    if (2 * src < sa)
        return qt_div_255(2 * src * dst + temp);
    else
        return qt_div_255(sa * da - 2 * (da - dst) * (sa - src) + temp);
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_HardLight_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) hardlight_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_HardLight(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_HardLight_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_HardLight_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_HardLight_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) hardlight_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_HardLight(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_HardLight_impl(dest, src, length, QFullCoverage());
    else
        comp_func_HardLight_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
    if 2.Sca <= Sa
        Dca' = Dca.(Sa + (2.Sca - Sa).(1 - Dca/Da)) + Sca.(1 - Da) + Dca.(1 - Sa)
    otherwise if 2.Sca > Sa and 4.Dca <= Da
        Dca' = Dca.Sa + Da.(2.Sca - Sa).(4.Dca/Da.(4.Dca/Da + 1).(Dca/Da - 1) + 7.Dca/Da) + Sca.(1 - Da) + Dca.(1 - Sa)
    otherwise if 2.Sca > Sa and 4.Dca > Da
        Dca' = Dca.Sa + Da.(2.Sca - Sa).((Dca/Da)^0.5 - Dca/Da) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int soft_light_op(int dst, int src, int da, int sa)
{
    const int src2 = src << 1;
    const int dst_np = da != 0 ? (255 * dst) / da : 0;
    const int temp = (src * (255 - da) + dst * (255 - sa)) * 255;

    if (src2 < sa)
        return (dst * (sa * 255 + (src2 - sa) * (255 - dst_np)) + temp) / 65025;
    else if (4 * dst <= da)
        return (dst * sa * 255 + da * (src2 - sa) * ((((16 * dst_np - 12 * 255) * dst_np + 3 * 65025) * dst_np) / 65025) + temp) / 65025;
    else {
#   ifdef Q_CC_RVCT // needed to avoid compiler crash in RVCT 2.2
        return (dst * sa * 255 + da * (src2 - sa) * (qIntSqrtInt(dst_np * 255) - dst_np) + temp) / 65025;
#   else
        return (dst * sa * 255 + da * (src2 - sa) * (int(qSqrt(qreal(dst_np * 255))) - dst_np) + temp) / 65025;
#   endif
    }
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_SoftLight_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) soft_light_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_SoftLight(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_SoftLight_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_SoftLight_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_SoftLight_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) soft_light_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_SoftLight(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_SoftLight_impl(dest, src, length, QFullCoverage());
    else
        comp_func_SoftLight_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
   Dca' = abs(Dca.Sa - Sca.Da) + Sca.(1 - Da) + Dca.(1 - Sa)
        = Sca + Dca - 2.min(Sca.Da, Dca.Sa)
*/
static inline int difference_op(int dst, int src, int da, int sa)
{
    return src + dst - qt_div_255(2 * qMin(src * da, dst * sa));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Difference_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) difference_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Difference(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Difference_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Difference_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Difference_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) difference_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Difference(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Difference_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Difference_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
    Dca' = (Sca.Da + Dca.Sa - 2.Sca.Dca) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void QT_FASTCALL comp_func_solid_Exclusion_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) (a + b - qt_div_255(2*(a*b)))
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Exclusion(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Exclusion_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Exclusion_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Exclusion_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) (a + b - ((a*b) >> 7))
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Exclusion(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Exclusion_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Exclusion_impl(dest, src, length, QPartialCoverage(const_alpha));
}

#if defined(Q_CC_RVCT)
// Restore pragma state from previous #pragma arm
#  pragma pop
#endif

void QT_FASTCALL rasterop_solid_SourceOrDestination(uint *dest,
                                                    int length,
                                                    uint color,
                                                    uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--)
        *dest++ |= color;
}

void QT_FASTCALL rasterop_SourceOrDestination(uint *dest,
                                              const uint *src,
                                              int length,
                                              uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--)
        *dest++ |= *src++;
}

void QT_FASTCALL rasterop_solid_SourceAndDestination(uint *dest,
                                                     int length,
                                                     uint color,
                                                     uint const_alpha)
{
    Q_UNUSED(const_alpha);
    color |= 0xff000000;
    while (length--)
        *dest++ &= color;
}

void QT_FASTCALL rasterop_SourceAndDestination(uint *dest,
                                               const uint *src,
                                               int length,
                                               uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (*src & *dest) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_SourceXorDestination(uint *dest,
                                                     int length,
                                                     uint color,
                                                     uint const_alpha)
{
    Q_UNUSED(const_alpha);
    color &= 0x00ffffff;
    while (length--)
        *dest++ ^= color;
}

void QT_FASTCALL rasterop_SourceXorDestination(uint *dest,
                                               const uint *src,
                                               int length,
                                               uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (*src ^ *dest) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_NotSourceAndNotDestination(uint *dest,
                                                           int length,
                                                           uint color,
                                                           uint const_alpha)
{
    Q_UNUSED(const_alpha);
    color = ~color;
    while (length--) {
        *dest = (color & ~(*dest)) | 0xff000000;
        ++dest;
    }
}

void QT_FASTCALL rasterop_NotSourceAndNotDestination(uint *dest,
                                                     const uint *src,
                                                     int length,
                                                     uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (~(*src) & ~(*dest)) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_NotSourceOrNotDestination(uint *dest,
                                                          int length,
                                                          uint color,
                                                          uint const_alpha)
{
    Q_UNUSED(const_alpha);
    color = ~color | 0xff000000;
    while (length--) {
        *dest = color | ~(*dest);
        ++dest;
    }
}

void QT_FASTCALL rasterop_NotSourceOrNotDestination(uint *dest,
                                                    const uint *src,
                                                    int length,
                                                    uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = ~(*src) | ~(*dest) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_NotSourceXorDestination(uint *dest,
                                                        int length,
                                                        uint color,
                                                        uint const_alpha)
{
    Q_UNUSED(const_alpha);
    color = ~color & 0x00ffffff;
    while (length--) {
        *dest = color ^ (*dest);
        ++dest;
    }
}

void QT_FASTCALL rasterop_NotSourceXorDestination(uint *dest,
                                                  const uint *src,
                                                  int length,
                                                  uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = ((~(*src)) ^ (*dest)) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_NotSource(uint *dest, int length,
                                          uint color, uint const_alpha)
{
    Q_UNUSED(const_alpha);
    qt_memfill(dest, ~color | 0xff000000, length);
}

void QT_FASTCALL rasterop_NotSource(uint *dest, const uint *src,
                                    int length, uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--)
        *dest++ = ~(*src++) | 0xff000000;
}

void QT_FASTCALL rasterop_solid_NotSourceAndDestination(uint *dest,
                                                        int length,
                                                        uint color,
                                                        uint const_alpha)
{
    Q_UNUSED(const_alpha);
    color = ~color | 0xff000000;
    while (length--) {
        *dest = color & *dest;
        ++dest;
    }
}

void QT_FASTCALL rasterop_NotSourceAndDestination(uint *dest,
                                                  const uint *src,
                                                  int length,
                                                  uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (~(*src) & *dest) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_SourceAndNotDestination(uint *dest,
                                                        int length,
                                                        uint color,
                                                        uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (color & ~(*dest)) | 0xff000000;
        ++dest;
    }
}

void QT_FASTCALL rasterop_SourceAndNotDestination(uint *dest,
                                                  const uint *src,
                                                  int length,
                                                  uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (*src & ~(*dest)) | 0xff000000;
        ++dest; ++src;
    }
}

static CompositionFunctionSolid functionForModeSolid_C[] = {
        comp_func_solid_SourceOver,
        comp_func_solid_DestinationOver,
        comp_func_solid_Clear,
        comp_func_solid_Source,
        comp_func_solid_Destination,
        comp_func_solid_SourceIn,
        comp_func_solid_DestinationIn,
        comp_func_solid_SourceOut,
        comp_func_solid_DestinationOut,
        comp_func_solid_SourceAtop,
        comp_func_solid_DestinationAtop,
        comp_func_solid_XOR,
        comp_func_solid_Plus,
        comp_func_solid_Multiply,
        comp_func_solid_Screen,
        comp_func_solid_Overlay,
        comp_func_solid_Darken,
        comp_func_solid_Lighten,
        comp_func_solid_ColorDodge,
        comp_func_solid_ColorBurn,
        comp_func_solid_HardLight,
        comp_func_solid_SoftLight,
        comp_func_solid_Difference,
        comp_func_solid_Exclusion,
        rasterop_solid_SourceOrDestination,
        rasterop_solid_SourceAndDestination,
        rasterop_solid_SourceXorDestination,
        rasterop_solid_NotSourceAndNotDestination,
        rasterop_solid_NotSourceOrNotDestination,
        rasterop_solid_NotSourceXorDestination,
        rasterop_solid_NotSource,
        rasterop_solid_NotSourceAndDestination,
        rasterop_solid_SourceAndNotDestination
};

static const CompositionFunctionSolid *functionForModeSolid = functionForModeSolid_C;

static CompositionFunction functionForMode_C[] = {
        comp_func_SourceOver,
        comp_func_DestinationOver,
        comp_func_Clear,
        comp_func_Source,
        comp_func_Destination,
        comp_func_SourceIn,
        comp_func_DestinationIn,
        comp_func_SourceOut,
        comp_func_DestinationOut,
        comp_func_SourceAtop,
        comp_func_DestinationAtop,
        comp_func_XOR,
        comp_func_Plus,
        comp_func_Multiply,
        comp_func_Screen,
        comp_func_Overlay,
        comp_func_Darken,
        comp_func_Lighten,
        comp_func_ColorDodge,
        comp_func_ColorBurn,
        comp_func_HardLight,
        comp_func_SoftLight,
        comp_func_Difference,
        comp_func_Exclusion,
        rasterop_SourceOrDestination,
        rasterop_SourceAndDestination,
        rasterop_SourceXorDestination,
        rasterop_NotSourceAndNotDestination,
        rasterop_NotSourceOrNotDestination,
        rasterop_NotSourceXorDestination,
        rasterop_NotSource,
        rasterop_NotSourceAndDestination,
        rasterop_SourceAndNotDestination
};

static const CompositionFunction *functionForMode = functionForMode_C;

static TextureBlendType getBlendType(const QSpanData *data)
{
    TextureBlendType ft;
    if (data->txop <= QTransform::TxTranslate)
        if (data->texture.type == QTextureData::Tiled)
            ft = BlendTiled;
        else
            ft = BlendUntransformed;
    else if (data->bilinear)
        if (data->texture.type == QTextureData::Tiled)
            ft = BlendTransformedBilinearTiled;
        else
            ft = BlendTransformedBilinear;
    else
        if (data->texture.type == QTextureData::Tiled)
            ft = BlendTransformedTiled;
        else
            ft = BlendTransformed;
    return ft;
}

static inline Operator getOperator(const QSpanData *data, const QSpan *spans, int spanCount)
{
    Operator op;
    bool solidSource = false;

    switch(data->type) {
    case QSpanData::Solid:
        solidSource = (qAlpha(data->solid.color) == 255);
        break;
    case QSpanData::LinearGradient:
        solidSource = !data->gradient.alphaColor;
        getLinearGradientValues(&op.linear, data);
        op.src_fetch = qt_fetch_linear_gradient;
        break;
    case QSpanData::RadialGradient:
        solidSource = !data->gradient.alphaColor;
        getRadialGradientValues(&op.radial, data);
        op.src_fetch = qt_fetch_radial_gradient;
        break;
    case QSpanData::ConicalGradient:
        solidSource = !data->gradient.alphaColor;
        op.src_fetch = qt_fetch_conical_gradient;
        break;
    case QSpanData::Texture:
        op.src_fetch = sourceFetch[getBlendType(data)][data->texture.format];
        solidSource = !data->texture.hasAlpha;
    default:
        break;
    }

    op.mode = data->rasterBuffer->compositionMode;
    if (op.mode == QPainter::CompositionMode_SourceOver && solidSource)
        op.mode = QPainter::CompositionMode_Source;

    op.dest_fetch = destFetchProc[data->rasterBuffer->format];
    if (op.mode == QPainter::CompositionMode_Source) {
        switch (data->rasterBuffer->format) {
        case QImage::Format_RGB32:
        case QImage::Format_ARGB32_Premultiplied:
            // don't clear dest_fetch as it sets up the pointer correctly to save one copy
            break;
        default: {
            const QSpan *lastSpan = spans + spanCount;
            bool alphaSpans = false;
            while (spans < lastSpan) {
                if (spans->coverage != 255) {
                    alphaSpans = true;
                    break;
                }
                ++spans;
            }
            if (!alphaSpans)
                op.dest_fetch = 0;
        }
        }
    }

    op.dest_store = destStoreProc[data->rasterBuffer->format];

    op.funcSolid = functionForModeSolid[op.mode];
    op.func = functionForMode[op.mode];

    return op;
}



// -------------------- blend methods ---------------------

enum SpanMethod {
    RegularSpans,
    CallbackSpans
};

#if !defined(Q_CC_SUN)
static
#endif
void drawBufferSpan(QSpanData *data, const uint *buffer, int bufsize,
                           int x, int y, int length, uint const_alpha)
{
#if defined (Q_WS_QWS) && !defined(QT_NO_RASTERCALLBACKS)
    data->rasterEngine->drawBufferSpan(buffer, bufsize, x, y, length, const_alpha);
#else
    Q_UNUSED(data);
    Q_UNUSED(buffer);
    Q_UNUSED(bufsize);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(length);
    Q_UNUSED(const_alpha);
#endif
}

#if !defined(Q_CC_SUN)
static
#endif
void blend_color_generic(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    uint buffer[buffer_size];
    Operator op = getOperator(data, spans, count);

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        while (length) {
            int l = qMin(buffer_size, length);
            uint *dest = op.dest_fetch ? op.dest_fetch(buffer, data->rasterBuffer, x, spans->y, l) : buffer;
            op.funcSolid(dest, l, data->solid.color, spans->coverage);
            if (op.dest_store)
                op.dest_store(data->rasterBuffer, x, spans->y, dest, l);
            length -= l;
            x += l;
        }
        ++spans;
    }
}

#if defined (Q_WS_QWS) && !defined(QT_NO_RASTERCALLBACKS)
static void blend_color_generic_callback(int count, const QSpan *spans, void *userData)
{
    // ### Falcon
    Q_UNUSED(count);
    Q_UNUSED(spans);
    Q_UNUSED(userData);
//     QSpanData *data = reinterpret_cast<QSpanData*>(userData);
//     data->rasterEngine->drawColorSpans(spans, count, data->solid.color);
}
#endif // QT_NO_RASTERCALLBACKS

static void blend_color_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    Operator op = getOperator(data, spans, count);

    if (op.mode == QPainter::CompositionMode_Source) {
        // inline for performance
        while (count--) {
            uint *target = ((uint *)data->rasterBuffer->scanLine(spans->y)) + spans->x;
            if (spans->coverage == 255) {
                QT_MEMFILL_UINT(target, spans->len, data->solid.color);
            } else {
                uint c = BYTE_MUL(data->solid.color, spans->coverage);
                int ialpha = 255 - spans->coverage;
                for (int i = 0; i < spans->len; ++i)
                    target[i] = c + BYTE_MUL(target[i], ialpha);
            }
            ++spans;
        }
        return;
    }

    while (count--) {
        uint *target = ((uint *)data->rasterBuffer->scanLine(spans->y)) + spans->x;
        op.funcSolid(target, spans->len, data->solid.color, spans->coverage);
        ++spans;
    }
}

template <class T>
Q_STATIC_TEMPLATE_FUNCTION void blendColor(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    Operator op = getOperator(data, spans, count);

    if (op.mode == QPainter::CompositionMode_Source) {
        const T c = qt_colorConvert<T, quint32p>(quint32p::fromRawData(data->solid.color), 0);
        while (count--) {
            T *target = ((T*)data->rasterBuffer->scanLine(spans->y))
                        + spans->x;
            if (spans->coverage == 255) {
                qt_memfill(target, c, spans->len);
            } else {
                const quint8 alpha = T::alpha(spans->coverage);
                const T color = c.byte_mul(alpha);
                const int ialpha = T::ialpha(spans->coverage);
                const T *end = target + spans->len;
                while (target < end) {
                    *target = color + target->byte_mul(ialpha);
                    ++target;
                }
            }
            ++spans;
        }
        return;
    }

    if (op.mode == QPainter::CompositionMode_SourceOver) {
        while (count--) {
            const quint32 color = BYTE_MUL(data->solid.color, spans->coverage);
            const T c = qt_colorConvert<T, quint32p>(quint32p::fromRawData(color), 0);
            const quint8 ialpha = T::alpha(qAlpha(~color));
            T *target = ((T*)data->rasterBuffer->scanLine(spans->y)) + spans->x;
            const T *end = target + spans->len;
            while (target != end) {
                *target = c + target->byte_mul(ialpha);
                ++target;
            }
            ++spans;
        }
        return;
    }

    blend_color_generic(count, spans, userData);
}

#define SPANFUNC_POINTER_BLENDCOLOR(DST) blendColor<DST>

static void blend_color_rgb16(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    /*
        We duplicate a little logic from getOperator() and calculate the
        composition mode directly.  This allows blend_color_rgb16 to be used
        from qt_gradient_quint16 with minimal overhead.
     */
    QPainter::CompositionMode mode = data->rasterBuffer->compositionMode;
    if (mode == QPainter::CompositionMode_SourceOver &&
        qAlpha(data->solid.color) == 255)
        mode = QPainter::CompositionMode_Source;

    if (mode == QPainter::CompositionMode_Source) {
        // inline for performance
        ushort c = qConvertRgb32To16(data->solid.color);
        while (count--) {
            ushort *target = ((ushort *)data->rasterBuffer->scanLine(spans->y)) + spans->x;
            if (spans->coverage == 255) {
                QT_MEMFILL_USHORT(target, spans->len, c);
            } else {
                ushort color = BYTE_MUL_RGB16(c, spans->coverage);
                int ialpha = 255 - spans->coverage;
                const ushort *end = target + spans->len;
                while (target < end) {
                    *target = color + BYTE_MUL_RGB16(*target, ialpha);
                    ++target;
                }
            }
            ++spans;
        }
        return;
    }

    if (mode == QPainter::CompositionMode_SourceOver) {
        while (count--) {
            uint color = BYTE_MUL(data->solid.color, spans->coverage);
            int ialpha = qAlpha(~color);
            ushort c = qConvertRgb32To16(color);
            ushort *target = ((ushort *)data->rasterBuffer->scanLine(spans->y)) + spans->x;
            int len = spans->len;
            bool pre = (((quintptr)target) & 0x3) != 0;
            bool post = false;
            if (pre) {
                // skip to word boundary
                *target = c + BYTE_MUL_RGB16(*target, ialpha);
                ++target;
                --len;
            }
            if (len & 0x1) {
                post = true;
                --len;
            }
            uint *target32 = (uint*)target;
            uint c32 = c | (c<<16);
            len >>= 1;
            uint salpha = (ialpha+1) >> 3; // calculate here rather than in loop
            while (len--) {
                // blend full words
                *target32 = c32 + BYTE_MUL_RGB16_32(*target32, salpha);
                ++target32;
                target += 2;
            }
            if (post) {
                // one last pixel beyond a full word
                *target = c + BYTE_MUL_RGB16(*target, ialpha);
            }
            ++spans;
        }
        return;
    }

    blend_color_generic(count, spans, userData);
}

template <typename T>
void handleSpans(int count, const QSpan *spans, const QSpanData *data, T &handler)
{
    uint const_alpha = 256;
    if (data->type == QSpanData::Texture)
        const_alpha = data->texture.const_alpha;

    int coverage = 0;
    while (count) {
        int x = spans->x;
        const int y = spans->y;
        int right = x + spans->len;

        // compute length of adjacent spans
        for (int i = 1; i < count && spans[i].y == y && spans[i].x == right; ++i)
            right += spans[i].len;
        int length = right - x;

        while (length) {
            int l = qMin(buffer_size, length);
            length -= l;

            int process_length = l;
            int process_x = x;

            const uint *src = handler.fetch(process_x, y, process_length);
            int offset = 0;
            while (l > 0) {
                if (x == spans->x) // new span?
                    coverage = (spans->coverage * const_alpha) >> 8;

                int right = spans->x + spans->len;
                int len = qMin(l, right - x);

                handler.process(x, y, len, coverage, src, offset);

                l -= len;
                x += len;
                offset += len;

                if (x == right) { // done with current span?
                    ++spans;
                    --count;
                }
            }
            handler.store(process_x, y, process_length);
        }
    }
}

struct QBlendBase
{
    QBlendBase(QSpanData *d, Operator o)
        : data(d)
        , op(o)
        , dest(0)
    {
    }

    QSpanData *data;
    Operator op;

    uint *dest;

    uint buffer[buffer_size];
    uint src_buffer[buffer_size];
};

template <SpanMethod spanMethod>
class BlendSrcGeneric : public QBlendBase
{
public:
    BlendSrcGeneric(QSpanData *d, Operator o)
        : QBlendBase(d, o)
    {
    }

    const uint *fetch(int x, int y, int len)
    {
        if (spanMethod == RegularSpans)
            dest = op.dest_fetch ? op.dest_fetch(buffer, data->rasterBuffer, x, y, len) : buffer;

        return op.src_fetch(src_buffer, &op, data, y, x, len);
    }

    void process(int x, int y, int len, int coverage, const uint *src, int offset)
    {
        if (spanMethod == RegularSpans)
            op.func(dest + offset, src + offset, len, coverage);
        else
            drawBufferSpan(data, src + offset, len, x, y, len, coverage);
    }

    void store(int x, int y, int len)
    {
        if (spanMethod == RegularSpans && op.dest_store) {
            op.dest_store(data->rasterBuffer, x, y, dest, len);
        }
    }
};

template <SpanMethod spanMethod>
Q_STATIC_TEMPLATE_FUNCTION void blend_src_generic(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    BlendSrcGeneric<spanMethod> blend(data, getOperator(data, spans, count));
    handleSpans(count, spans, data, blend);
}

template <SpanMethod spanMethod>
Q_STATIC_TEMPLATE_FUNCTION void blend_untransformed_generic(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    uint buffer[buffer_size];
    uint src_buffer[buffer_size];
    Operator op = getOperator(data, spans, count);

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx);
    int yoff = -qRound(-data->dy);

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = xoff + x;
        int sy = yoff + spans->y;
        if (sy >= 0 && sy < image_height && sx < image_width) {
            if (sx < 0) {
                x -= sx;
                length += sx;
                sx = 0;
            }
            if (sx + length > image_width)
                length = image_width - sx;
            if (length > 0) {
                const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
                while (length) {
                    int l = qMin(buffer_size, length);
                    const uint *src = op.src_fetch(src_buffer, &op, data, sy, sx, l);
                    if (spanMethod == RegularSpans) {
                        uint *dest = op.dest_fetch ? op.dest_fetch(buffer, data->rasterBuffer, x, spans->y, l) : buffer;
                        op.func(dest, src, l, coverage);
                        if (op.dest_store)
                            op.dest_store(data->rasterBuffer, x, spans->y, dest, l);
                    } else {
                        drawBufferSpan(data, src, l, x, spans->y,
                                       l, coverage);
                    }
                    x += l;
                    sx += l;
                    length -= l;
                }
            }
        }
        ++spans;
    }
}

template <SpanMethod spanMethod>
Q_STATIC_TEMPLATE_FUNCTION void blend_untransformed_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_untransformed_generic<spanMethod>(count, spans, userData);
        return;
    }

    Operator op = getOperator(data, spans, count);

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx);
    int yoff = -qRound(-data->dy);

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = xoff + x;
        int sy = yoff + spans->y;
        if (sy >= 0 && sy < image_height && sx < image_width) {
            if (sx < 0) {
                x -= sx;
                length += sx;
                sx = 0;
            }
            if (sx + length > image_width)
                length = image_width - sx;
            if (length > 0) {
                const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
                const uint *src = (uint *)data->texture.scanLine(sy) + sx;
                if (spanMethod == RegularSpans) {
                    uint *dest = ((uint *)data->rasterBuffer->scanLine(spans->y)) + x;
                    op.func(dest, src, length, coverage);
                } else {
                    drawBufferSpan(data, src, length, x,
                                   spans->y, length, coverage);
                }
            }
        }
        ++spans;
    }
}

static inline quint16 interpolate_pixel_rgb16_255(quint16 x, quint8 a,
                                                  quint16 y, quint8 b)
{
    quint16 t = ((((x & 0x07e0) * a) + ((y & 0x07e0) * b)) >> 5) & 0x07e0;
    t |= ((((x & 0xf81f) * a) + ((y & 0xf81f) * b)) >> 5) & 0xf81f;

    return t;
}

static inline quint32 interpolate_pixel_rgb16x2_255(quint32 x, quint8 a,
                                                    quint32 y, quint8 b)
{
    uint t;
    t = ((((x & 0xf81f07e0) >> 5) * a) + (((y & 0xf81f07e0) >> 5) * b)) & 0xf81f07e0;
    t |= ((((x & 0x07e0f81f) * a) + ((y & 0x07e0f81f) * b)) >> 5) & 0x07e0f81f;
    return t;
}

static inline void blend_sourceOver_rgb16_rgb16(quint16 *dest,
                                                const quint16 *src,
                                                int length,
                                                const quint8 alpha,
                                                const quint8 ialpha)
{
    const int dstAlign = ((quintptr)dest) & 0x3;
    if (dstAlign) {
        *dest = interpolate_pixel_rgb16_255(*src, alpha, *dest, ialpha);
        ++dest;
        ++src;
        --length;
    }
    const int srcAlign = ((quintptr)src) & 0x3;
    int length32 = length >> 1;
    if (length32 && srcAlign == 0) {
        while (length32--) {
            const quint32 *src32 = reinterpret_cast<const quint32*>(src);
            quint32 *dest32 = reinterpret_cast<quint32*>(dest);
            *dest32 = interpolate_pixel_rgb16x2_255(*src32, alpha,
                                                    *dest32, ialpha);
            dest += 2;
            src += 2;
        }
        length &= 0x1;
    }
    while (length--) {
        *dest = interpolate_pixel_rgb16_255(*src, alpha, *dest, ialpha);
        ++dest;
        ++src;
    }
}

template <class DST, class SRC>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline void madd_2(DST *dest, const quint16 alpha, const SRC *src)
{
    Q_ASSERT((quintptr(dest) & 0x3) == 0);
    Q_ASSERT((quintptr(src) & 0x3) == 0);
    dest[0] = dest[0].byte_mul(alpha >> 8) + DST(src[0]);
    dest[1] = dest[1].byte_mul(alpha & 0xff) + DST(src[1]);
}

template <class DST, class SRC>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline void madd_4(DST *dest, const quint32 alpha, const SRC *src)
{
    Q_ASSERT((quintptr(dest) & 0x3) == 0);
    Q_ASSERT((quintptr(src) & 0x3) == 0);
    dest[0] = dest[0].byte_mul(alpha >> 24) + DST(src[0]);
    dest[1] = dest[1].byte_mul((alpha >> 16) & 0xff) + DST(src[1]);
    dest[2] = dest[2].byte_mul((alpha >> 8) & 0xff) + DST(src[2]);
    dest[3] = dest[3].byte_mul(alpha & 0xff) + DST(src[3]);
}

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline void madd_4(qargb8565 *dest, const quint32 a, const qargb8565 *src)
{
    Q_ASSERT((quintptr(dest) & 0x3) == 0);
    Q_ASSERT((quintptr(src) & 0x3) == 0);

    const quint32 *src32 = reinterpret_cast<const quint32*>(src);
    quint32 *dest32 = reinterpret_cast<quint32*>(dest);
    quint32 x, y, t;
    quint8 a8;

    {
        x = dest32[0];
        y = src32[0];

        a8 = a >> 24;

        // a0,g0
        t = ((((x & 0x0007e0ff) * a8) >> 5) & 0x0007e0ff) + (y & 0x0007c0f8);

        // r0,b0
        t |= ((((x & 0x00f81f00) * a8) >> 5) & 0x00f81f00) + (y & 0x00f81f00);

        a8 = (a >> 16) & 0xff;

        // a1
        t |= ((((x & 0xff000000) >> 5) * a8) & 0xff000000) + (y & 0xf8000000);

        dest32[0] = t;
    }
    {
        x = dest32[1];
        y = src32[1];

        // r1,b1
        t = ((((x & 0x0000f81f) * a8) >> 5) & 0x0000f81f) + (y & 0x0000f81f);

        // g1
        t |= ((((x & 0x000007e0) * a8) >> 5) & 0x000007e0) + (y & 0x000007c0);

        a8 = (a >> 8) & 0xff;

        // a2
        t |= ((((x & 0x00ff0000) * a8) >> 5)  & 0x00ff0000) + (y & 0x00f80000);

        {
            // rgb2
            quint16 x16 = (x >> 24) | ((dest32[2] & 0x000000ff) << 8);
            quint16 y16 = (y >> 24) | ((src32[2] & 0x000000ff) << 8);
            quint16 t16;

            t16 = ((((x16 & 0xf81f) * a8) >> 5) & 0xf81f)  + (y16 & 0xf81f);
            t16 |= ((((x16 & 0x07e0) * a8) >> 5) & 0x07e0)  + (y16 & 0x07c0);

            // rg2
            t |= ((t16 & 0x00ff) << 24);

            dest32[1] = t;

            x = dest32[2];
            y = src32[2];

            // gb2
            t = (t16 >> 8);
        }
    }
    {
        a8 = a & 0xff;

        // g3,a3
        t |= ((((x & 0x07e0ff00) * a8) >> 5) & 0x07e0ff00) + (y & 0x07c0f800);

        // r3,b3
        t |= ((((x & 0xf81f0000) >> 5) * a8) & 0xf81f0000)+ (y & 0xf81f0000);

        dest32[2] = t;
    }
}
#endif

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline void madd_4(qargb8555 *dest, const quint32 a, const qargb8555 *src)
{
    Q_ASSERT((quintptr(dest) & 0x3) == 0);
    Q_ASSERT((quintptr(src) & 0x3) == 0);

    const quint32 *src32 = reinterpret_cast<const quint32*>(src);
    quint32 *dest32 = reinterpret_cast<quint32*>(dest);
    quint32 x, y, t;
    quint8 a8;

    {
        x = dest32[0];
        y = src32[0];

        a8 = a >> 24;

        // a0,g0
        t = ((((x & 0x0003e0ff) * a8) >> 5) & 0x0003e0ff) + (y & 0x0003e0f8);

        // r0,b0
        t |= ((((x & 0x007c1f00) * a8) >> 5) & 0x007c1f00) + (y & 0x007c1f00);

        a8 = (a >> 16) & 0xff;

        // a1
        t |= ((((x & 0xff000000) >> 5) * a8) & 0xff000000) + (y & 0xf8000000);

        dest32[0] = t;
    }
    {
        x = dest32[1];
        y = src32[1];

        // r1,b1
        t = ((((x & 0x00007c1f) * a8) >> 5) & 0x00007c1f) + (y & 0x00007c1f);

        // g1
        t |= ((((x & 0x000003e0) * a8) >> 5) & 0x000003e0) + (y & 0x000003e0);

        a8 = (a >> 8) & 0xff;

        // a2
        t |= ((((x & 0x00ff0000) * a8) >> 5)  & 0x00ff0000) + (y & 0x00f80000);

        {
            // rgb2
            quint16 x16 = (x >> 24) | ((dest32[2] & 0x000000ff) << 8);
            quint16 y16 = (y >> 24) | ((src32[2] & 0x000000ff) << 8);
            quint16 t16;

            t16 = ((((x16 & 0x7c1f) * a8) >> 5) & 0x7c1f)  + (y16 & 0x7c1f);
            t16 |= ((((x16 & 0x03e0) * a8) >> 5) & 0x03e0)  + (y16 & 0x03e0);

            // rg2
            t |= ((t16 & 0x00ff) << 24);

            dest32[1] = t;

            x = dest32[2];
            y = src32[2];

            // gb2
            t = (t16 >> 8);
        }
    }
    {
        a8 = a & 0xff;

        // g3,a3
        t |= ((((x & 0x03e0ff00) * a8) >> 5) & 0x03e0ff00) + (y & 0x03e0f800);

        // r3,b3
        t |= ((((x & 0x7c1f0000) >> 5) * a8) & 0x7c1f0000)+ (y & 0x7c1f0000);

        dest32[2] = t;
    }
}
#endif

template <class T>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint16 alpha_2(const T *src)
{
    Q_ASSERT((quintptr(src) & 0x3) == 0);

    if (T::hasAlpha())
        return (src[0].alpha() << 8) | src[1].alpha();
    else
        return 0xffff;
}

template <class T>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint32 alpha_4(const T *src)
{
    Q_ASSERT((quintptr(src) & 0x3) == 0);

    if (T::hasAlpha()) {
        return (src[0].alpha() << 24) | (src[1].alpha() << 16)
            | (src[2].alpha() << 8) | src[3].alpha();
    } else {
        return 0xffffffff;
    }
}

template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint32 alpha_4(const qargb8565 *src)
{
    const quint8 *src8 = reinterpret_cast<const quint8*>(src);
    return src8[0] << 24 | src8[3] << 16 | src8[6] << 8 | src8[9];
}

template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint32 alpha_4(const qargb6666 *src)
{
    const quint8 *src8 = reinterpret_cast<const quint8*>(src);
    return ((src8[2] & 0xfc) | (src8[2] >> 6)) << 24
        | ((src8[5] & 0xfc) | (src8[5] >> 6))  << 16
        | ((src8[8] & 0xfc) | (src8[8] >> 6)) << 8
        | ((src8[11] & 0xfc) | (src8[11] >> 6));
}

template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint32 alpha_4(const qargb8555 *src)
{
    Q_ASSERT((quintptr(src) & 0x3) == 0);
    const quint8 *src8 = reinterpret_cast<const quint8*>(src);
    return src8[0] << 24 | src8[3] << 16 | src8[6] << 8 | src8[9];
}

template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint16 alpha_2(const qargb4444 *src)
{
    const quint32 *src32 = reinterpret_cast<const quint32*>(src);
    const quint32 t = (*src32 & 0xf000f000) |
                      ((*src32 & 0xf000f000) >> 4);
    return (t >> 24) | (t & 0xff00);
}

template <class T>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint16 eff_alpha_2(quint16 alpha, const T*)
{
    return (T::alpha((alpha >> 8) & 0xff) << 8)
        | T::alpha(alpha & 0xff);
}

template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint16 eff_alpha_2(quint16 a, const qrgb565*)
{
    return ((((a & 0xff00) + 0x0100) >> 3) & 0xff00)
        | ((((a & 0x00ff) + 0x0001) >> 3) & 0x00ff);
}

template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint16 eff_alpha_2(quint16 a, const qrgb444*)
{
    return (((a & 0x00ff) + 0x0001) >> 4)
        | ((((a & 0xff00) + 0x0100) >> 4) & 0xff00);
}

template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint16 eff_alpha_2(quint16 a, const qargb4444*)
{
    return (((a & 0x00ff) + 0x0001) >> 4)
        | ((((a & 0xff00) + 0x0100) >> 4) & 0xff00);
}

template <class T>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint16 eff_ialpha_2(quint16 alpha, const T*)
{
    return (T::ialpha((alpha >> 8) & 0xff) << 8)
        | T::ialpha(alpha & 0xff);
}

template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint16 eff_ialpha_2(quint16 a, const qrgb565 *dummy)
{
    return 0x2020 - eff_alpha_2(a, dummy);
}

template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint16 eff_ialpha_2(quint16 a, const qargb4444 *dummy)
{
    return 0x1010 - eff_alpha_2(a, dummy);
}

template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint16 eff_ialpha_2(quint16 a, const qrgb444 *dummy)
{
    return 0x1010 - eff_alpha_2(a, dummy);
}

template <class T>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint32 eff_alpha_4(quint32 alpha, const T*)
{
    return (T::alpha(alpha >> 24) << 24)
        | (T::alpha((alpha >> 16) & 0xff) << 16)
        | (T::alpha((alpha >> 8) & 0xff) << 8)
        | T::alpha(alpha & 0xff);
}

template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint32 eff_alpha_4(quint32 a, const qrgb888*)
{
    return a;
}

template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint32 eff_alpha_4(quint32 a, const qargb8565*)
{
    return ((((a & 0xff00ff00) + 0x01000100) >> 3) & 0xff00ff00)
        | ((((a & 0x00ff00ff) + 0x00010001) >> 3) & 0x00ff00ff);
}

template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint32 eff_alpha_4(quint32 a, const qargb6666*)
{
    return ((((a & 0xff00ff00) >> 2) + 0x00400040) & 0xff00ff00)
        | ((((a & 0x00ff00ff) + 0x00010001) >> 2) & 0x00ff00ff);
}

template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint32 eff_alpha_4(quint32 a, const qrgb666*)
{
    return ((((a & 0xff00ff00) >> 2) + 0x00400040) & 0xff00ff00)
        | ((((a & 0x00ff00ff) + 0x00010001) >> 2) & 0x00ff00ff);
}

template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint32 eff_alpha_4(quint32 a, const qargb8555*)
{
    return ((((a & 0xff00ff00) + 0x01000100) >> 3) & 0xff00ff00)
        | ((((a & 0x00ff00ff) + 0x00010001) >> 3) & 0x00ff00ff);
}

template <class T>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint32 eff_ialpha_4(quint32 alpha, const T*)
{
    return (T::ialpha(alpha >> 24) << 24)
        | (T::ialpha((alpha >> 16) & 0xff) << 16)
        | (T::ialpha((alpha >> 8) & 0xff) << 8)
        | T::ialpha(alpha & 0xff);
}

template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint32 eff_ialpha_4(quint32 a, const qrgb888*)
{
    return ~a;
}

template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint32 eff_ialpha_4(quint32 a, const qargb8565 *dummy)
{
    return 0x20202020 - eff_alpha_4(a, dummy);
}

template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint32 eff_ialpha_4(quint32 a, const qargb6666 *dummy)
{
    return 0x40404040 - eff_alpha_4(a, dummy);
}

template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint32 eff_ialpha_4(quint32 a, const qrgb666 *dummy)
{
    return 0x40404040 - eff_alpha_4(a, dummy);
}

template <>
Q_STATIC_TEMPLATE_SPECIALIZATION
inline quint32 eff_ialpha_4(quint32 a, const qargb8555 *dummy)
{
    return 0x20202020 - eff_alpha_4(a, dummy);
}

template <class DST, class SRC>
inline void interpolate_pixel_unaligned_2(DST *dest, const SRC *src,
                                          quint16 alpha)
{
    const quint16 a = eff_alpha_2(alpha, dest);
    const quint16 ia = eff_ialpha_2(alpha, dest);
    dest[0] = DST(src[0]).byte_mul(a >> 8) + dest[0].byte_mul(ia >> 8);
    dest[1] = DST(src[1]).byte_mul(a & 0xff) + dest[1].byte_mul(ia & 0xff);
}

template <class DST, class SRC>
inline void interpolate_pixel_2(DST *dest, const SRC *src, quint16 alpha)
{
    Q_ASSERT((quintptr(dest) & 0x3) == 0);
    Q_ASSERT((quintptr(src) & 0x3) == 0);

    const quint16 a = eff_alpha_2(alpha, dest);
    const quint16 ia = eff_ialpha_2(alpha, dest);

    dest[0] = DST(src[0]).byte_mul(a >> 8) + dest[0].byte_mul(ia >> 8);
    dest[1] = DST(src[1]).byte_mul(a & 0xff) + dest[1].byte_mul(ia & 0xff);
}

template <class DST, class SRC>
inline void interpolate_pixel(DST &dest, quint8 a, const SRC &src, quint8 b)
{
    if (SRC::hasAlpha() && !DST::hasAlpha())
        interpolate_pixel(dest, a, DST(src), b);
    else
        dest = dest.byte_mul(a) + DST(src).byte_mul(b);
}

template <>
inline void interpolate_pixel(qargb8565 &dest, quint8 a,
                              const qargb8565 &src, quint8 b)
{
    quint8 *d = reinterpret_cast<quint8*>(&dest);
    const quint8 *s = reinterpret_cast<const quint8*>(&src);
    d[0] = (d[0] * a + s[0] * b) >> 5;

    const quint16 x = (d[2] << 8) | d[1];
    const quint16 y = (s[2] << 8) | s[1];
    quint16 t = (((x & 0x07e0) * a + (y & 0x07e0) * b) >> 5) & 0x07e0;
    t |= (((x & 0xf81f) * a + (y & 0xf81f) * b) >> 5) & 0xf81f;

    d[1] = t & 0xff;
    d[2] = t >> 8;
}

template <>
inline void interpolate_pixel(qrgb565 &dest, quint8 a,
                              const qrgb565 &src, quint8 b)
{
    const quint16 x = dest.rawValue();
    const quint16 y = src.rawValue();
    quint16 t = (((x & 0x07e0) * a + (y & 0x07e0) * b) >> 5) & 0x07e0;
    t |= (((x & 0xf81f) * a + (y & 0xf81f) * b) >> 5) & 0xf81f;
    dest = t;
}

template <>
inline void interpolate_pixel(qrgb555 &dest, quint8 a,
                              const qrgb555 &src, quint8 b)
{
    const quint16 x = dest.rawValue();
    const quint16 y = src.rawValue();
    quint16 t = (((x & 0x03e0) * a + (y & 0x03e0) * b) >> 5) & 0x03e0;
    t |= ((((x & 0x7c1f) * a) + ((y & 0x7c1f) * b)) >> 5) & 0x7c1f;
    dest = t;
}

template <>
inline void interpolate_pixel(qrgb444 &dest, quint8 a,
                              const qrgb444 &src, quint8 b)
{
    const quint16 x = dest.rawValue();
    const quint16 y = src.rawValue();
    quint16 t = ((x & 0x00f0) * a + (y & 0x00f0) * b) & 0x0f00;
    t |= ((x & 0x0f0f) * a + (y & 0x0f0f) * b) & 0xf0f0;
    quint16 *d = reinterpret_cast<quint16*>(&dest);
    *d = (t >> 4);
}

template <class DST, class SRC>
inline void interpolate_pixel_2(DST *dest, quint8 a,
                                const SRC *src, quint8 b)
{
    Q_ASSERT((quintptr(dest) & 0x3) == 0);
    Q_ASSERT((quintptr(src) & 0x3) == 0);

    Q_ASSERT(!SRC::hasAlpha());

    dest[0] = dest[0].byte_mul(a) + DST(src[0]).byte_mul(b);
    dest[1] = dest[1].byte_mul(a) + DST(src[1]).byte_mul(b);
}

template <>
inline void interpolate_pixel_2(qrgb565 *dest, quint8 a,
                                const qrgb565 *src, quint8 b)
{
    quint32 *x = reinterpret_cast<quint32*>(dest);
    const quint32 *y = reinterpret_cast<const quint32*>(src);
    quint32 t = (((*x & 0xf81f07e0) >> 5) * a +
                 ((*y & 0xf81f07e0) >> 5) * b) & 0xf81f07e0;
    t |= (((*x & 0x07e0f81f) * a
           + (*y & 0x07e0f81f) * b) >> 5) & 0x07e0f81f;
    *x = t;
}

template <>
inline void interpolate_pixel_2(qrgb555 *dest, quint8 a,
                                const qrgb555 *src, quint8 b)
{
    quint32 *x = reinterpret_cast<quint32*>(dest);
    const quint32 *y = reinterpret_cast<const quint32*>(src);
    quint32 t = (((*x & 0x7c1f03e0) >> 5) * a +
                 ((*y & 0x7c1f03e0) >> 5) * b) & 0x7c1f03e0;
    t |= (((*x & 0x03e07c1f) * a
           + (*y & 0x03e07c1f) * b) >> 5) & 0x03e07c1f;
    *x = t;
}

template <>
inline void interpolate_pixel_2(qrgb444 *dest, quint8 a,
                                const qrgb444 *src, quint8 b)
{
    quint32 *x = reinterpret_cast<quint32*>(dest);
    const quint32 *y = reinterpret_cast<const quint32*>(src);
    quint32 t = ((*x & 0x0f0f0f0f) * a + (*y & 0x0f0f0f0f) * b) & 0xf0f0f0f0;
    t |= ((*x & 0x00f000f0) * a + (*y & 0x00f000f0) * b) & 0x0f000f00;
    *x = t >> 4;
}

template <class DST, class SRC>
inline void interpolate_pixel_4(DST *dest, const SRC *src, quint32 alpha)
{
    Q_ASSERT((quintptr(dest) & 0x3) == 0);
    Q_ASSERT((quintptr(src) & 0x3) == 0);

    const quint32 a = eff_alpha_4(alpha, dest);
    const quint32 ia = eff_ialpha_4(alpha, dest);
    dest[0] = DST(src[0]).byte_mul(a >> 24)
              + dest[0].byte_mul(ia >> 24);
    dest[1] = DST(src[1]).byte_mul((a >> 16) & 0xff)
              + dest[1].byte_mul((ia >> 16) & 0xff);
    dest[2] = DST(src[2]).byte_mul((a >> 8) & 0xff)
              + dest[2].byte_mul((ia >> 8) & 0xff);
    dest[3] = DST(src[3]).byte_mul(a & 0xff)
              + dest[3].byte_mul(ia & 0xff);
}

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
template <>
inline void interpolate_pixel_4(qargb8565 *dest, const qargb8565 *src,
                                quint32 alpha)
{
    Q_ASSERT((quintptr(dest) & 0x3) == 0);
    Q_ASSERT((quintptr(src) & 0x3) == 0);

    const quint32 a = eff_alpha_4(alpha, dest);
    const quint32 ia = eff_ialpha_4(alpha, dest);
    const quint32 *src32 = reinterpret_cast<const quint32*>(src);
    quint32 *dest32 = reinterpret_cast<quint32*>(dest);

    quint32 x, y, t;
    quint8 a8, ia8;
    {
        x = src32[0];
        y = dest32[0];

        a8 = a >> 24;
        ia8 = ia >> 24;

        // a0,g0
        t = (((x & 0x0007e0ff) * a8 + (y & 0x0007e0ff) * ia8) >> 5)
            & 0x0007e0ff;

        // r0,b0
        t |= (((x & 0x00f81f00) * a8 + (y & 0x00f81f00) * ia8) >> 5)
             & 0x00f81f00;

        a8 = (a >> 16) & 0xff;
        ia8 = (ia >> 16) & 0xff;

        // a1
        t |= (((x & 0xff000000) >> 5) * a8 + ((y & 0xff000000) >> 5) * ia8)
             & 0xff000000;

        dest32[0] = t;
    }
    {
        x = src32[1];
        y = dest32[1];

        // r1,b1
        t = (((x & 0x0000f81f) * a8 + (y & 0x0000f81f) * ia8) >> 5)
            & 0x0000f81f;

        // g1
        t |= (((x & 0x000007e0) * a8 + (y & 0x000007e0) * ia8) >> 5)
             & 0x000007e0;

        a8 = (a >> 8) & 0xff;
        ia8 = (ia >> 8) & 0xff;

        // a2
        t |= (((x & 0x00ff0000) * a8 + (y & 0x00ff0000) * ia8) >> 5)
             & 0x00ff0000;

        {
            // rgb2
            quint16 x16 = (x >> 24) | ((src32[2] & 0x000000ff) << 8);
            quint16 y16 = (y >> 24) | ((dest32[2] & 0x000000ff) << 8);
            quint16 t16;

            t16 = (((x16 & 0xf81f) * a8 + (y16 & 0xf81f) * ia8) >> 5) & 0xf81f;
            t16 |= (((x16 & 0x07e0) * a8 + (y16 & 0x07e0) * ia8) >> 5) & 0x07e0;

            // rg2
            t |= ((t16 & 0x00ff) << 24);

            dest32[1] = t;

            x = src32[2];
            y = dest32[2];

            // gb2
            t = (t16 >> 8);
        }
    }
    {
        a8 = a & 0xff;
        ia8 = ia & 0xff;

        // g3,a3
        t |= (((x & 0x07e0ff00) * a8 + (y & 0x07e0ff00) * ia8) >> 5)
             & 0x07e0ff00;

        // r3,b3
        t |= (((x & 0xf81f0000) >> 5) * a8 + ((y & 0xf81f0000) >> 5) * ia8)
             & 0xf81f0000;

        dest32[2] = t;
    }
}
#endif

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
template <>
inline void interpolate_pixel_4(qargb8555 *dest, const qargb8555 *src,
                                quint32 alpha)
{
    Q_ASSERT((quintptr(dest) & 0x3) == 0);
    Q_ASSERT((quintptr(src) & 0x3) == 0);


    const quint32 a = eff_alpha_4(alpha, dest);
    const quint32 ia = eff_ialpha_4(alpha, dest);
    const quint32 *src32 = reinterpret_cast<const quint32*>(src);
    quint32 *dest32 = reinterpret_cast<quint32*>(dest);

    quint32 x, y, t;
    quint8 a8, ia8;
    {
        x = src32[0];
        y = dest32[0];

        a8 = a >> 24;
        ia8 = ia >> 24;

        // a0,g0
        t = (((x & 0x0003e0ff) * a8 + (y & 0x0003e0ff) * ia8) >> 5)
            & 0x0003e0ff;

        // r0,b0
        t |= (((x & 0x007c1f00) * a8 + (y & 0x007c1f00) * ia8) >> 5)
             & 0x007c1f00;

        a8 = (a >> 16) & 0xff;
        ia8 = (ia >> 16) & 0xff;

        // a1
        t |= (((x & 0xff000000) >> 5) * a8 + ((y & 0xff000000) >> 5) * ia8)
             & 0xff000000;

        dest32[0] = t;
    }
    {
        x = src32[1];
        y = dest32[1];

        // r1,b1
        t = (((x & 0x00007c1f) * a8 + (y & 0x00007c1f) * ia8) >> 5)
            & 0x00007c1f;

        // g1
        t |= (((x & 0x000003e0) * a8 + (y & 0x000003e0) * ia8) >> 5)
             & 0x000003e0;

        a8 = (a >> 8) & 0xff;
        ia8 = (ia >> 8) & 0xff;

        // a2
        t |= (((x & 0x00ff0000) * a8 + (y & 0x00ff0000) * ia8) >> 5)
             & 0x00ff0000;

        {
            // rgb2
            quint16 x16 = (x >> 24) | ((src32[2] & 0x000000ff) << 8);
            quint16 y16 = (y >> 24) | ((dest32[2] & 0x000000ff) << 8);
            quint16 t16;

            t16 = (((x16 & 0x7c1f) * a8 + (y16 & 0x7c1f) * ia8) >> 5) & 0x7c1f;
            t16 |= (((x16 & 0x03e0) * a8 + (y16 & 0x03e0) * ia8) >> 5) & 0x03e0;

            // rg2
            t |= ((t16 & 0x00ff) << 24);

            dest32[1] = t;

            x = src32[2];
            y = dest32[2];

            // gb2
            t = (t16 >> 8);
        }
    }
    {
        a8 = a & 0xff;
        ia8 = ia & 0xff;

        // g3,a3
        t |= (((x & 0x03e0ff00) * a8 + (y & 0x03e0ff00) * ia8) >> 5)
             & 0x03e0ff00;

        // r3,b3
        t |= (((x & 0x7c1f0000) >> 5) * a8 + ((y & 0x7c1f0000) >> 5) * ia8)
             & 0x7c1f0000;

        dest32[2] = t;
    }
}
#endif

template <>
inline void interpolate_pixel_4(qrgb888 *dest, const qrgb888 *src,
                                quint32 alpha)
{
    Q_ASSERT((quintptr(dest) & 0x3) == 0);
    Q_ASSERT((quintptr(src) & 0x3) == 0);

    const quint32 a = eff_alpha_4(alpha, dest);
    const quint32 ia = eff_ialpha_4(alpha, dest);
    const quint32 *src32 = reinterpret_cast<const quint32*>(src);
    quint32 *dest32 = reinterpret_cast<quint32*>(dest);

    {
        quint32 x = src32[0];
        quint32 y = dest32[0];

        quint32 t;
        t = ((x >> 8) & 0xff00ff) * (a >> 24)
            + ((y >> 8) & 0xff00ff) * (ia >> 24);
        t = (t + ((t >> 8) & 0xff00ff) + 0x800080);
        t &= 0xff00ff00;

        x = (x & 0xff0000) * (a >> 24)
            + (x & 0x0000ff) * ((a >> 16) & 0xff)
            + (y & 0xff0000) * (ia >> 24)
            + (y & 0x0000ff) * ((ia >> 16) & 0xff);
        x = (x + ((x >> 8) & 0xff00ff) + 0x800080) >> 8;
        x &= 0x00ff00ff;

        dest32[0] = x | t;
    }
    {
        quint32 x = src32[1];
        quint32 y = dest32[1];

        quint32 t;
        t = ((x >> 8) & 0xff0000) * ((a >> 16) & 0xff)
            + ((x >> 8) & 0x0000ff) * ((a >> 8) & 0xff)
            + ((y >> 8) & 0xff0000) * ((ia >> 16) & 0xff)
            + ((y >> 8) & 0x0000ff) * ((ia >> 8) & 0xff);
        t = (t + ((t >> 8) & 0xff00ff) + 0x800080);
        t &= 0xff00ff00;

        x = (x & 0xff0000) * ((a >> 16) & 0xff)
            + (x & 0x0000ff) * ((a >> 8) & 0xff)
            + (y & 0xff0000) * ((ia >> 16) & 0xff)
            + (y & 0x0000ff) * ((ia >> 8) & 0xff);
        x = (x + ((x >> 8) & 0xff00ff) + 0x800080) >> 8;
        x &= 0x00ff00ff;

        dest32[1] = x | t;
    }
    {
        quint32 x = src32[2];
        quint32 y = dest32[2];

        quint32 t;
        t = ((x >> 8) & 0xff0000) * ((a >> 8) & 0xff)
            + ((x >> 8) & 0x0000ff) * (a & 0xff)
            + ((y >> 8) & 0xff0000) * ((ia >> 8) & 0xff)
            + ((y >> 8) & 0x0000ff) * (ia & 0xff);
        t = (t + ((t >> 8) & 0xff00ff) + 0x800080);
        t &= 0xff00ff00;

        x = (x & 0xff00ff) * (a & 0xff)
            + (y & 0xff00ff) * (ia & 0xff);
        x = (x + ((x >> 8) & 0xff00ff) + 0x800080) >> 8;
        x &= 0x00ff00ff;

        dest32[2] = x | t;
    }
}

template <class DST, class SRC>
inline void interpolate_pixel_4(DST *dest, quint8 a,
                                const SRC *src, quint8 b)
{
    Q_ASSERT((quintptr(dest) & 0x3) == 0);
    Q_ASSERT((quintptr(src) & 0x3) == 0);

    dest[0] = dest[0].byte_mul(a) + DST(src[0]).byte_mul(b);
    dest[1] = dest[1].byte_mul(a) + DST(src[1]).byte_mul(b);
    dest[2] = dest[2].byte_mul(a) + DST(src[2]).byte_mul(b);
    dest[3] = dest[3].byte_mul(a) + DST(src[3]).byte_mul(b);
}

template <class DST, class SRC>
inline void blend_sourceOver_4(DST *dest, const SRC *src)
{
    Q_ASSERT((quintptr(dest) & 0x3) == 0);
    Q_ASSERT((quintptr(src) & 0x3) == 0);

    const quint32 a = alpha_4(src);
    if (a == 0xffffffff) {
        qt_memconvert(dest, src, 4);
    } else if (a > 0) {
        quint32 buf[3]; // array of quint32 to get correct alignment
        qt_memconvert((DST*)(void*)buf, src, 4);
        madd_4(dest, eff_ialpha_4(a, dest), (DST*)(void*)buf);
    }
}

template <>
inline void blend_sourceOver_4(qargb8565 *dest, const qargb8565 *src)
{
    Q_ASSERT((quintptr(dest) & 0x3) == 0);
    Q_ASSERT((quintptr(src) & 0x3) == 0);

    const quint32 a = alpha_4(src);
    if (a == 0xffffffff) {
        qt_memconvert(dest, src, 4);
    } else if (a > 0) {
        madd_4(dest, eff_ialpha_4(a, dest), src);
    }
}

template <>
inline void blend_sourceOver_4(qargb8555 *dest, const qargb8555 *src)
{
    Q_ASSERT((quintptr(dest) & 0x3) == 0);
    Q_ASSERT((quintptr(src) & 0x3) == 0);

    const quint32 a = alpha_4(src);
    if (a == 0xffffffff) {
        qt_memconvert(dest, src, 4);
    } else if (a > 0) {
        madd_4(dest, eff_ialpha_4(a, dest), src);
    }
}

template <>
inline void blend_sourceOver_4(qargb6666 *dest, const qargb6666 *src)
{
    Q_ASSERT((quintptr(dest) & 0x3) == 0);
    Q_ASSERT((quintptr(src) & 0x3) == 0);

    const quint32 a = alpha_4(src);
    if (a == 0xffffffff) {
        qt_memconvert(dest, src, 4);
    } else if (a > 0) {
        madd_4(dest, eff_ialpha_4(a, dest), src);
    }
}

template <class DST, class SRC>
void QT_FASTCALL blendUntransformed_unaligned(DST *dest, const SRC *src,
                                              quint8 coverage, int length)
{
    Q_ASSERT(coverage > 0);

    if (coverage < 255) {
        if (SRC::hasAlpha()) {
            for (int i = 0; i < length; ++i) {
                if (src[i].alpha()) {
                    const quint8 alpha = qt_div_255(int(src[i].alpha()) * int(coverage));
                    interpolate_pixel(dest[i], DST::ialpha(alpha),
                            src[i], DST::alpha(alpha));
                }
            }
        } else {
            const quint8 alpha = DST::alpha(coverage);
            const quint8 ialpha = DST::ialpha(coverage);
            if (alpha) {
                for (int i = 0; i < length; ++i)
                    interpolate_pixel(dest[i], ialpha, src[i], alpha);
            }
        }
        return;
    }

    Q_ASSERT(coverage == 0xff);
    Q_ASSERT(SRC::hasAlpha());

    if (SRC::hasAlpha()) {
        for (int i = 0; i < length; ++i) {
            const quint8 a = src->alpha();
            if (a == 0xff)
                *dest = DST(*src);
            else if (a > 0) {
                if (DST::hasAlpha())
                    *dest = DST(*src).truncedAlpha() + dest->byte_mul(DST::ialpha(a));
                else
                    *dest = DST(SRC(*src).truncedAlpha()) + dest->byte_mul(DST::ialpha(a));
            }
            ++src;
            ++dest;
        }
    }
}

template <class DST, class SRC>
void QT_FASTCALL blendUntransformed_dest16(DST *dest, const SRC *src,
                                           quint8 coverage, int length)
{
    Q_ASSERT(sizeof(DST) == 2);
    Q_ASSERT(sizeof(SRC) == 2);
    Q_ASSERT((quintptr(dest) & 0x3) == (quintptr(src) & 0x3));
    Q_ASSERT(coverage > 0);

    const int align = quintptr(dest) & 0x3;

    if (coverage < 255) {
        // align
        if (align) {
            const quint8 alpha = SRC::hasAlpha()
                                 ? qt_div_255(int(src->alpha()) * int(coverage))
                                 : coverage;
            if (alpha) {
                interpolate_pixel(*dest, DST::ialpha(alpha),
                                  *src, DST::alpha(alpha));
            }
            ++dest;
            ++src;
            --length;
        }

        if (SRC::hasAlpha()) {
            while (length >= 2) {
                const quint16 alpha16 = BYTE_MUL(uint(alpha_2(src)), uint(coverage));
                interpolate_pixel_2(dest, src, alpha16);
                length -= 2;
                src += 2;
                dest += 2;
            }
        } else {
            const quint8 alpha = DST::alpha(coverage);
            const quint8 ialpha = DST::ialpha(coverage);

            while (length >= 2) {
                interpolate_pixel_2(dest, ialpha, src, alpha);
                length -= 2;
                src += 2;
                dest += 2;
            }
        }

        // tail
        if (length) {
            const quint8 alpha = SRC::hasAlpha()
                                 ? qt_div_255(int(src->alpha()) * int(coverage))
                                 : coverage;
            if (alpha) {
                interpolate_pixel(*dest, DST::ialpha(alpha),
                                  *src, DST::alpha(alpha));
            }
        }

        return;
    }

    Q_ASSERT(SRC::hasAlpha());
    if (SRC::hasAlpha()) {
        if (align) {
            const quint8 alpha = src->alpha();
            if (alpha == 0xff)
                *dest = DST(*src);
            else if (alpha > 0)
                *dest = DST(*src).truncedAlpha() + dest->byte_mul(DST::ialpha(alpha));
            ++dest;
            ++src;
            --length;
        }

        while (length >= 2) {
            Q_ASSERT((quintptr(dest) & 3) == 0);
            Q_ASSERT((quintptr(src) & 3) == 0);

            const quint16 a = alpha_2(src);
            if (a == 0xffff) {
                qt_memconvert(dest, src, 2);
            } else if (a > 0) {
                quint32 buf;
                if (sizeof(DST) == 2)
                    qt_memconvert((DST*)(void*)&buf, src, 2);
                madd_2(dest, eff_ialpha_2(a, dest), (DST*)(void*)&buf);
            }

            length -= 2;
            src += 2;
            dest += 2;
        }

        if (length) {
            const quint8 alpha = src->alpha();
            if (alpha == 0xff)
                *dest = DST(*src);
            else if (alpha > 0)
                *dest = DST(*src).truncedAlpha() + dest->byte_mul(DST::ialpha(alpha));
        }
    }
}

template <class DST, class SRC>
void QT_FASTCALL blendUntransformed_dest24(DST *dest, const SRC *src,
                                           quint8 coverage, int length)
{
    Q_ASSERT((quintptr(dest) & 0x3) == (quintptr(src) & 0x3));
    Q_ASSERT(sizeof(DST) == 3);
    Q_ASSERT(coverage > 0);

    const int align = quintptr(dest) & 0x3;

    if (coverage < 255) {
        // align
        for (int i = 0; i < align; ++i) {
            if (SRC::hasAlpha()) {
                const quint8 alpha = qt_div_255(int(src->alpha()) * int(coverage));
                if (alpha)
                    interpolate_pixel(*dest, DST::ialpha(alpha),
                                      *src, DST::alpha(alpha));
            } else {
                interpolate_pixel(*dest, DST::ialpha(coverage),
                                  *src, DST::alpha(coverage));
            }
            ++dest;
            ++src;
            --length;
        }

        if (SRC::hasAlpha()) {
            while (length >= 4) {
                const quint32 alpha = QT_PREPEND_NAMESPACE(BYTE_MUL)(uint(alpha_4(src)), uint(coverage));
                if (alpha)
                    interpolate_pixel_4(dest, src, alpha);
                length -= 4;
                src += 4;
                dest += 4;
            }
        } else {
            const quint8 alpha = DST::alpha(coverage);
            const quint8 ialpha = DST::ialpha(coverage);
            while (length >= 4) {
                interpolate_pixel_4(dest, ialpha, src, alpha);
                length -= 4;
                src += 4;
                dest += 4;
            }
        }

        // tail
        while (length--) {
            if (SRC::hasAlpha()) {
                const quint8 alpha = qt_div_255(int(src->alpha()) * int(coverage));
                if (alpha)
                    interpolate_pixel(*dest, DST::ialpha(alpha),
                                      *src, DST::alpha(alpha));
            } else {
                interpolate_pixel(*dest, DST::ialpha(coverage),
                                  *src, DST::alpha(coverage));
            }
            ++dest;
            ++src;
        }

        return;
    }


    Q_ASSERT(coverage == 255);
    Q_ASSERT(SRC::hasAlpha());

    if (SRC::hasAlpha()) {
        // align
        for (int i = 0; i < align; ++i) {
            const quint8 a = src->alpha();
            if (a == 0xff) {
                *dest = DST(*src);
            } else if (a > 0) {
                *dest = DST(*src).truncedAlpha() + dest->byte_mul(DST::ialpha(a));
            }
            ++dest;
            ++src;
            --length;
        }

        while (length >= 4) {
            blend_sourceOver_4(dest, src);
            length -= 4;
            src += 4;
            dest += 4;
        }

        // tail
        while (length--) {
            const quint8 a = src->alpha();
            if (a == 0xff) {
                *dest = DST(*src);
            } else if (a > 0) {
                *dest = DST(*src).truncedAlpha() + dest->byte_mul(DST::ialpha(a));
            }
            ++dest;
            ++src;
        }
    }
}

template <class DST, class SRC>
Q_STATIC_TEMPLATE_SPECIALIZATION
void QT_FASTCALL blendUntransformed(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData*>(userData);
    QPainter::CompositionMode mode = data->rasterBuffer->compositionMode;

    if (mode != QPainter::CompositionMode_SourceOver &&
        mode != QPainter::CompositionMode_Source)
    {
        blend_src_generic<RegularSpans>(count, spans, userData);
        return;
    }

    const bool modeSource = !SRC::hasAlpha() ||
                            mode == QPainter::CompositionMode_Source;
    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx);
    int yoff = -qRound(-data->dy);

    while (count--) {
        const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
        if (coverage == 0) {
            ++spans;
            continue;
        }

        int x = spans->x;
        int length = spans->len;
        int sx = xoff + x;
        int sy = yoff + spans->y;
        if (sy >= 0 && sy < image_height && sx < image_width) {
            if (sx < 0) {
                x -= sx;
                length += sx;
                sx = 0;
            }
            if (sx + length > image_width)
                length = image_width - sx;
            if (length > 0) {
                DST *dest = ((DST*)data->rasterBuffer->scanLine(spans->y)) + x;
                const SRC *src = (SRC*)data->texture.scanLine(sy) + sx;
                if (modeSource && coverage == 255) {
                    qt_memconvert<DST, SRC>(dest, src, length);
                } else if (sizeof(DST) == 3 && sizeof(SRC) == 3 && length >= 3 &&
                           (quintptr(dest) & 3) == (quintptr(src) & 3))
                {
                    blendUntransformed_dest24(dest, src, coverage, length);
                } else if (sizeof(DST) == 2 && sizeof(SRC) == 2 && length >= 3 &&
                           (quintptr(dest) & 3) == (quintptr(src) & 3))
                {
                    blendUntransformed_dest16(dest, src, coverage, length);
                } else {
                    blendUntransformed_unaligned(dest, src, coverage, length);
                }
            }
        }
        ++spans;
    }
}

static void blend_untransformed_rgb888(int count, const QSpan *spans,
                                       void *userData)
{
#if defined(QT_QWS_DEPTH_24)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_RGB888)
        blendUntransformed<qrgb888, qrgb888>(count, spans, userData);
    else
#endif
        blend_untransformed_generic<RegularSpans>(count, spans, userData);
}

static void blend_untransformed_argb6666(int count, const QSpan *spans,
                                         void *userData)
{
#if defined(QT_QWS_DEPTH_18)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB6666_Premultiplied)
        blendUntransformed<qargb6666, qargb6666>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB666)
        blendUntransformed<qargb6666, qrgb666>(count, spans, userData);
    else
#endif
        blend_untransformed_generic<RegularSpans>(count, spans, userData);
}

static void blend_untransformed_rgb666(int count, const QSpan *spans,
                                       void *userData)
{
#if defined(QT_QWS_DEPTH_18)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB6666_Premultiplied)
        blendUntransformed<qrgb666, qargb6666>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB666)
        blendUntransformed<qrgb666, qrgb666>(count, spans, userData);
    else
#endif
        blend_untransformed_generic<RegularSpans>(count, spans, userData);
}

static void blend_untransformed_argb8565(int count, const QSpan *spans,
                                         void *userData)
{
#if defined(QT_QWS_DEPTH_16)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB8565_Premultiplied)
        blendUntransformed<qargb8565, qargb8565>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB16)
        blendUntransformed<qargb8565, qrgb565>(count, spans, userData);
    else
#endif
        blend_untransformed_generic<RegularSpans>(count, spans, userData);
}

static void blend_untransformed_rgb565(int count, const QSpan *spans,
                                       void *userData)
{
#if !defined(Q_WS_QWS) || defined(QT_QWS_DEPTH_16)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB8565_Premultiplied)
        blendUntransformed<qrgb565, qargb8565>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB16)
        blendUntransformed<qrgb565, qrgb565>(count, spans, userData);
    else
#endif
        blend_untransformed_generic<RegularSpans>(count, spans, userData);
}

static void blend_untransformed_argb8555(int count, const QSpan *spans,
                                         void *userData)
{
#if defined(QT_QWS_DEPTH_15)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB8555_Premultiplied)
        blendUntransformed<qargb8555, qargb8555>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB555)
        blendUntransformed<qargb8555, qrgb555>(count, spans, userData);
    else
#endif
        blend_untransformed_generic<RegularSpans>(count, spans, userData);
}

static void blend_untransformed_rgb555(int count, const QSpan *spans,
                                       void *userData)
{
#if defined(QT_QWS_DEPTH_15)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB8555_Premultiplied)
        blendUntransformed<qrgb555, qargb8555>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB555)
        blendUntransformed<qrgb555, qrgb555>(count, spans, userData);
    else
#endif
        blend_untransformed_generic<RegularSpans>(count, spans, userData);
}

static void blend_untransformed_argb4444(int count, const QSpan *spans,
                                         void *userData)
{
#if defined(QT_QWS_DEPTH_12)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB4444_Premultiplied)
        blendUntransformed<qargb4444, qargb4444>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB444)
        blendUntransformed<qargb4444, qrgb444>(count, spans, userData);
    else
#endif
        blend_untransformed_generic<RegularSpans>(count, spans, userData);
}

static void blend_untransformed_rgb444(int count, const QSpan *spans,
                                       void *userData)
{
#if defined(QT_QWS_DEPTH_12)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB4444_Premultiplied)
        blendUntransformed<qrgb444, qargb4444>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB444)
        blendUntransformed<qrgb444, qrgb444>(count, spans, userData);
    else
#endif
        blend_untransformed_generic<RegularSpans>(count, spans, userData);
}

template <SpanMethod spanMethod>
Q_STATIC_TEMPLATE_FUNCTION void blend_tiled_generic(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    uint buffer[buffer_size];
    uint src_buffer[buffer_size];
    Operator op = getOperator(data, spans, count);

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx) % image_width;
    int yoff = -qRound(-data->dy) % image_height;

    if (xoff < 0)
        xoff += image_width;
    if (yoff < 0)
        yoff += image_height;

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = (xoff + spans->x) % image_width;
        int sy = (spans->y + yoff) % image_height;
        if (sx < 0)
            sx += image_width;
        if (sy < 0)
            sy += image_height;

        const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
        while (length) {
            int l = qMin(image_width - sx, length);
            if (buffer_size < l)
                l = buffer_size;
            const uint *src = op.src_fetch(src_buffer, &op, data, sy, sx, l);
            if (spanMethod == RegularSpans) {
                uint *dest = op.dest_fetch ? op.dest_fetch(buffer, data->rasterBuffer, x, spans->y, l) : buffer;
                op.func(dest, src, l, coverage);
                if (op.dest_store)
                    op.dest_store(data->rasterBuffer, x, spans->y, dest, l);
            } else {
                drawBufferSpan(data, src, l, x, spans->y, l,
                               coverage);
            }
            x += l;
            sx += l;
            length -= l;
            if (sx >= image_width)
                sx = 0;
        }
        ++spans;
    }
}

template <SpanMethod spanMethod>
Q_STATIC_TEMPLATE_FUNCTION void blend_tiled_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_tiled_generic<spanMethod>(count, spans, userData);
        return;
    }

    Operator op = getOperator(data, spans, count);

    int image_width = data->texture.width;
    int image_height = data->texture.height;
    int xoff = -qRound(-data->dx) % image_width;
    int yoff = -qRound(-data->dy) % image_height;

    if (xoff < 0)
        xoff += image_width;
    if (yoff < 0)
        yoff += image_height;

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = (xoff + spans->x) % image_width;
        int sy = (spans->y + yoff) % image_height;
        if (sx < 0)
            sx += image_width;
        if (sy < 0)
            sy += image_height;

        const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
        while (length) {
            int l = qMin(image_width - sx, length);
            if (buffer_size < l)
                l = buffer_size;
            const uint *src = (uint *)data->texture.scanLine(sy) + sx;
            if (spanMethod == RegularSpans) {
                uint *dest = ((uint *)data->rasterBuffer->scanLine(spans->y)) + x;
                op.func(dest, src, l, coverage);
            } else {
                drawBufferSpan(data, src, buffer_size,
                               x, spans->y, l, coverage);
            }
            x += l;
            length -= l;
            sx = 0;
        }
        ++spans;
    }
}

template <class DST, class SRC>
Q_STATIC_TEMPLATE_FUNCTION void blendTiled(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData*>(userData);
    QPainter::CompositionMode mode = data->rasterBuffer->compositionMode;

    if (mode != QPainter::CompositionMode_SourceOver &&
        mode != QPainter::CompositionMode_Source)
    {
        blend_src_generic<RegularSpans>(count, spans, userData);
        return;
    }

    const bool modeSource = !SRC::hasAlpha() ||
                            mode == QPainter::CompositionMode_Source;
    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx) % image_width;
    int yoff = -qRound(-data->dy) % image_height;

    if (xoff < 0)
        xoff += image_width;
    if (yoff < 0)
        yoff += image_height;

    while (count--) {
        const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
        if (coverage == 0) {
            ++spans;
            continue;
        }

        int x = spans->x;
        int length = spans->len;
        int sx = (xoff + spans->x) % image_width;
        int sy = (spans->y + yoff) % image_height;
        if (sx < 0)
            sx += image_width;
        if (sy < 0)
            sy += image_height;

        if (modeSource && coverage == 255) {
            // Copy the first texture block
            length = qMin(image_width,length);
            int tx = x;
            while (length) {
                int l = qMin(image_width - sx, length);
                if (buffer_size < l)
                    l = buffer_size;
                DST *dest = ((DST*)data->rasterBuffer->scanLine(spans->y)) + tx;
                const SRC *src = (SRC*)data->texture.scanLine(sy) + sx;

                qt_memconvert<DST, SRC>(dest, src, l);
                length -= l;
                tx += l;
                sx = 0;
            }

            // Now use the rasterBuffer as the source of the texture,
            // We can now progressively copy larger blocks
            // - Less cpu time in code figuring out what to copy
            // We are dealing with one block of data
            // - More likely to fit in the cache
            // - can use memcpy
            int copy_image_width = qMin(image_width, int(spans->len));
            length = spans->len - copy_image_width;
            DST *src = ((DST*)data->rasterBuffer->scanLine(spans->y)) + x;
            DST *dest = src + copy_image_width;
            while (copy_image_width < length) {
                qt_memconvert(dest, src, copy_image_width);
                dest += copy_image_width;
                length -= copy_image_width;
                copy_image_width *= 2;
            }
            if (length > 0)
                qt_memconvert(dest, src, length);
        } else {
            while (length) {
                int l = qMin(image_width - sx, length);
                if (buffer_size < l)
                    l = buffer_size;
                DST *dest = ((DST*)data->rasterBuffer->scanLine(spans->y)) + x;
                const SRC *src = (SRC*)data->texture.scanLine(sy) + sx;
                if (sizeof(DST) == 3 && sizeof(SRC) == 3 && l >= 4 &&
                           (quintptr(dest) & 3) == (quintptr(src) & 3))
                {
                    blendUntransformed_dest24(dest, src, coverage, l);
                } else if (sizeof(DST) == 2 && sizeof(SRC) == 2 && l >= 2 &&
                           (quintptr(dest) & 3) == (quintptr(src) & 3))
                {
                    blendUntransformed_dest16(dest, src, coverage, l);
                } else {
                    blendUntransformed_unaligned(dest, src, coverage, l);
                }

                x += l;
                length -= l;
                sx = 0;
            }
        }
        ++spans;
    }
}

static void blend_tiled_rgb888(int count, const QSpan *spans, void *userData)
{
#if defined(QT_QWS_DEPTH_24)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_RGB888)
        blendTiled<qrgb888, qrgb888>(count, spans, userData);
    else
#endif
        blend_tiled_generic<RegularSpans>(count, spans, userData);
}

static void blend_tiled_argb6666(int count, const QSpan *spans, void *userData)
{
#if defined(QT_QWS_DEPTH_18)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB6666_Premultiplied)
        blendTiled<qargb6666, qargb6666>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB666)
        blendTiled<qargb6666, qrgb666>(count, spans, userData);
    else
#endif
        blend_tiled_generic<RegularSpans>(count, spans, userData);
}

static void blend_tiled_rgb666(int count, const QSpan *spans, void *userData)
{
#if defined(QT_QWS_DEPTH_18)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB6666_Premultiplied)
        blendTiled<qrgb666, qargb6666>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB666)
        blendTiled<qrgb666, qrgb666>(count, spans, userData);
    else
#endif
        blend_tiled_generic<RegularSpans>(count, spans, userData);
}

static void blend_tiled_argb8565(int count, const QSpan *spans, void *userData)
{
#if defined(QT_QWS_DEPTH_16)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB8565_Premultiplied)
        blendTiled<qargb8565, qargb8565>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB16)
        blendTiled<qargb8565, qrgb565>(count, spans, userData);
    else
#endif
        blend_tiled_generic<RegularSpans>(count, spans, userData);
}

static void blend_tiled_rgb565(int count, const QSpan *spans, void *userData)
{
#if !defined(Q_WS_QWS) || defined(QT_QWS_DEPTH_16)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB8565_Premultiplied)
        blendTiled<qrgb565, qargb8565>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB16)
        blendTiled<qrgb565, qrgb565>(count, spans, userData);
    else
#endif
        blend_tiled_generic<RegularSpans>(count, spans, userData);
}

static void blend_tiled_argb8555(int count, const QSpan *spans, void *userData)
{
#if defined(QT_QWS_DEPTH_15)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB8555_Premultiplied)
        blendTiled<qargb8555, qargb8555>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB555)
        blendTiled<qargb8555, qrgb555>(count, spans, userData);
    else
#endif
        blend_tiled_generic<RegularSpans>(count, spans, userData);
}

static void blend_tiled_rgb555(int count, const QSpan *spans, void *userData)
{
#if defined(QT_QWS_DEPTH_15)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB8555_Premultiplied)
        blendTiled<qrgb555, qargb8555>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB555)
        blendTiled<qrgb555, qrgb555>(count, spans, userData);
    else
#endif
        blend_tiled_generic<RegularSpans>(count, spans, userData);
}

static void blend_tiled_argb4444(int count, const QSpan *spans, void *userData)
{
#if defined(QT_QWS_DEPTH_12)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB4444_Premultiplied)
        blendTiled<qargb4444, qargb4444>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB444)
        blendTiled<qargb4444, qrgb444>(count, spans, userData);
    else
#endif
        blend_tiled_generic<RegularSpans>(count, spans, userData);
}

static void blend_tiled_rgb444(int count, const QSpan *spans, void *userData)
{
#if defined(QT_QWS_DEPTH_12)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB4444_Premultiplied)
        blendTiled<qrgb444, qargb4444>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB444)
        blendTiled<qrgb444, qrgb444>(count, spans, userData);
    else
#endif
        blend_tiled_generic<RegularSpans>(count, spans, userData);
}

template <class DST, class SRC>
Q_STATIC_TEMPLATE_FUNCTION void blendTransformedBilinear(int count, const QSpan *spans,
                                     void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData*>(userData);
    QPainter::CompositionMode mode = data->rasterBuffer->compositionMode;


    if (mode != QPainter::CompositionMode_SourceOver) {
        blend_src_generic<RegularSpans>(count, spans, userData);
        return;
    }

    SRC buffer[buffer_size];

    const int src_minx = data->texture.x1;
    const int src_miny = data->texture.y1;
    const int src_maxx = data->texture.x2 - 1;
    const int src_maxy = data->texture.y2 - 1;

    if (data->fast_matrix) {
        // The increment pr x in the scanline
        const int fdx = (int)(data->m11 * fixed_scale);
        const int fdy = (int)(data->m12 * fixed_scale);

        while (count--) {
            const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            if (coverage == 0) {
                ++spans;
                continue;
            }

            DST *dest = (DST*)data->rasterBuffer->scanLine(spans->y)
                        + spans->x;
            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);
            int x = int((data->m21 * cy
                         + data->m11 * cx + data->dx) * fixed_scale) - half_point;
            int y = int((data->m22 * cy
                         + data->m12 * cx + data->dy) * fixed_scale) - half_point;
            int length = spans->len;

            while (length) {
                const int l = qMin(length, buffer_size);

                const SRC *end = buffer + l;
                SRC *b = buffer;
                while (b < end) {
                    int x1 = (x >> 16);
                    int x2;
                    int y1 = (y >> 16);
                    int y2;

                    const int distx = (x & 0x0000ffff) >> 8;
                    const int disty = (y & 0x0000ffff) >> 8;

                    if (x1 < src_minx) {
                        x2 = x1 = src_minx;
                    } else if (x1 >= src_maxx) {
                        x2 = x1 = src_maxx;
                    } else {
                        x2 = x1 + 1;
                    }
                    if (y1 < src_miny) {
                        y2 = y1 = src_miny;
                    } else if (y1 >= src_maxy) {
                        y2 = y1 = src_maxy;
                    } else {
                        y2 = y1 + 1;
                    }
#if 0
                    if (x1 == x2) {
                        if (y1 == y2) {
                            *b = ((SRC*)data->texture.scanLine(y1))[x1];
                        } else {
                            *b = ((SRC*)data->texture.scanLine(y1))[x1];
                            const SRC t = data->texture.scanLine(y2)[x1];
                            interpolate_pixel(*b, SRC::ialpha(disty),
                                              t, SRC::alpha(disty));
                        }
                    } else if (y1 == y2) {
                        *b = ((SRC*)data->texture.scanLine(y1))[x1];
                        const SRC t = ((SRC*)data->texture.scanLine(y1))[x2];
                        interpolate_pixel(*b, SRC::ialpha(distx),
                                          t, SRC::alpha(distx));
                    } else
#endif
                    {
                        const SRC *src1 = (SRC*)data->texture.scanLine(y1);
                        const SRC *src2 = (SRC*)data->texture.scanLine(y2);
                        SRC tl = src1[x1];
                        const SRC tr = src1[x2];
                        SRC bl = src2[x1];
                        const SRC br = src2[x2];
                        const quint8 ax = SRC::alpha(distx);
                        const quint8 iax = SRC::ialpha(distx);

                        interpolate_pixel(tl, iax, tr, ax);
                        interpolate_pixel(bl, iax, br, ax);
                        interpolate_pixel(tl, SRC::ialpha(disty),
                                          bl, SRC::alpha(disty));
                        *b = tl;
                    }
                    ++b;

                    x += fdx;
                    y += fdy;
                }

                if (!SRC::hasAlpha() && coverage == 255) {
                    qt_memconvert(dest, buffer, l);
                } else if (sizeof(DST) == 3 && l >= 4 &&
                           (quintptr(dest) & 3) == (quintptr(buffer) & 3))
                {
                    blendUntransformed_dest24(dest, buffer, coverage, l);
                } else if (sizeof(DST) == 2 && sizeof(SRC) == 2 && l >= 2 &&
                           (quintptr(dest) & 3) == (quintptr(buffer) & 3)) {
                    blendUntransformed_dest16(dest, buffer, coverage, l);
                } else {
                    blendUntransformed_unaligned(dest, buffer, coverage, l);
                }

                dest += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        while (count--) {
            const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            if (coverage == 0) {
                ++spans;
                continue;
            }

            DST *dest = (DST*)data->rasterBuffer->scanLine(spans->y)
                        + spans->x;

            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);

            qreal x = data->m21 * cy + data->m11 * cx + data->dx;
            qreal y = data->m22 * cy + data->m12 * cx + data->dy;
            qreal w = data->m23 * cy + data->m13 * cx + data->m33;

            int length = spans->len;
            while (length) {
                const int l = qMin(length, buffer_size);
                const SRC *end = buffer + l;
                SRC *b = buffer;
                while (b < end) {
                    const qreal iw = w == 0 ? 1 : 1 / w;
                    const qreal px = x * iw - qreal(0.5);
                    const qreal py = y * iw - qreal(0.5);

                    int x1 = int(px) - (px < 0);
                    int x2;
                    int y1 = int(py) - (py < 0);
                    int y2;

                    const int distx = int((px - x1) * 256);
                    const int disty = int((py - y1) * 256);

                    if (x1 < src_minx) {
                        x2 = x1 = src_minx;
                    } else if (x1 >= src_maxx) {
                        x2 = x1 = src_maxx;
                    } else {
                        x2 = x1 + 1;
                    }
                    if (y1 < src_miny) {
                        y2 = y1 = src_miny;
                    } else if (y1 >= src_maxy) {
                        y2 = y1 = src_maxy;
                    } else {
                        y2 = y1 + 1;
                    }

                    const SRC *src1 = (SRC*)data->texture.scanLine(y1);
                    const SRC *src2 = (SRC*)data->texture.scanLine(y2);
                    SRC tl = src1[x1];
                    const SRC tr = src1[x2];
                    SRC bl = src2[x1];
                    const SRC br = src2[x2];
                    const quint8 ax = SRC::alpha(distx);
                    const quint8 iax = SRC::ialpha(distx);

                    interpolate_pixel(tl, iax, tr, ax);
                    interpolate_pixel(bl, iax, br, ax);
                    interpolate_pixel(tl, SRC::ialpha(disty),
                                      bl, SRC::alpha(disty));
                    *b = tl;
                    ++b;

                    x += fdx;
                    y += fdy;
                    w += fdw;
                }
                if (!SRC::hasAlpha() && coverage == 255) {
                    qt_memconvert(dest, buffer, l);
                } else if (sizeof(DST) == 3 && l >= 4 &&
                           (quintptr(dest) & 3) == (quintptr(buffer) & 3))
                {
                    blendUntransformed_dest24(dest, buffer, coverage, l);
                } else if (sizeof(DST) == 2 && sizeof(SRC) == 2 && l >= 2 &&
                           (quintptr(dest) & 3) == (quintptr(buffer) & 3)) {
                    blendUntransformed_dest16(dest, buffer, coverage, l);
                } else {
                    blendUntransformed_unaligned(dest, buffer, coverage, l);
                }

                dest += l;
                length -= l;
            }
            ++spans;
        }
    }
}

static void blend_transformed_bilinear_rgb888(int count, const QSpan *spans, void *userData)
{
#if defined(QT_QWS_DEPTH_24)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_RGB888)
        blendTransformedBilinear<qrgb888, qrgb888>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_bilinear_argb6666(int count, const QSpan *spans, void *userData)
{
#if defined(QT_QWS_DEPTH_18)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB6666_Premultiplied)
        blendTransformedBilinear<qargb6666, qargb6666>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB666)
        blendTransformedBilinear<qargb6666, qrgb666>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_bilinear_rgb666(int count, const QSpan *spans, void *userData)
{
#if defined(QT_QWS_DEPTH_18)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB6666_Premultiplied)
        blendTransformedBilinear<qrgb666, qargb6666>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB666)
        blendTransformedBilinear<qrgb666, qrgb666>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_bilinear_argb8565(int count, const QSpan *spans, void *userData)
{
#if defined(QT_QWS_DEPTH_16)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB8565_Premultiplied)
        blendTransformedBilinear<qargb8565, qargb8565>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB16)
        blendTransformedBilinear<qargb8565, qrgb565>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_bilinear_rgb565(int count, const QSpan *spans,
                                              void *userData)
{
#if !defined(Q_WS_QWS) || defined(QT_QWS_DEPTH_16)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_RGB16)
        blendTransformedBilinear<qrgb565, qrgb565>(count, spans, userData);
    else if (data->texture.format == QImage::Format_ARGB8565_Premultiplied)
        blendTransformedBilinear<qrgb565, qargb8565>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_bilinear_argb8555(int count, const QSpan *spans, void *userData)
{
#if defined(QT_QWS_DEPTH_15)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB8555_Premultiplied)
        blendTransformedBilinear<qargb8555, qargb8555>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB555)
        blendTransformedBilinear<qargb8555, qrgb555>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_bilinear_rgb555(int count, const QSpan *spans, void *userData)
{
#if defined(QT_QWS_DEPTH_15)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB8555_Premultiplied)
        blendTransformedBilinear<qrgb555, qargb8555>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB555)
        blendTransformedBilinear<qrgb555, qrgb555>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_bilinear_argb4444(int count, const QSpan *spans, void *userData)
{
#if defined(QT_QWS_DEPTH_12)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB4444_Premultiplied)
        blendTransformedBilinear<qargb4444, qargb4444>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB444)
        blendTransformedBilinear<qargb4444, qrgb444>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_bilinear_rgb444(int count, const QSpan *spans, void *userData)
{
#if defined(QT_QWS_DEPTH_12)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB4444_Premultiplied)
        blendTransformedBilinear<qrgb444, qargb4444>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB444)
        blendTransformedBilinear<qrgb444, qrgb444>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

template <SpanMethod spanMethod>
Q_STATIC_TEMPLATE_FUNCTION void blend_transformed_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_src_generic<spanMethod>(count, spans, userData);
        return;
    }

    CompositionFunction func = functionForMode[data->rasterBuffer->compositionMode];
    uint buffer[buffer_size];

    int image_width = data->texture.width;
    int image_height = data->texture.height;
    const int scanline_offset = data->texture.bytesPerLine / 4;

    if (data->fast_matrix) {
        // The increment pr x in the scanline
        int fdx = (int)(data->m11 * fixed_scale);
        int fdy = (int)(data->m12 * fixed_scale);

        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;

            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);

            int x = int((data->m21 * cy
                         + data->m11 * cx + data->dx) * fixed_scale);
            int y = int((data->m22 * cy
                         + data->m12 * cx + data->dy) * fixed_scale);

            int length = spans->len;
            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    int px = x >> 16;
                    int py = y >> 16;

                    bool out = (px < 0) || (px >= image_width)
                               || (py < 0) || (py >= image_height);

                    int y_offset = py * scanline_offset;
                    *b = out ? uint(0) : image_bits[y_offset + px];
                    x += fdx;
                    y += fdy;
                    ++b;
                }
                if (spanMethod == RegularSpans)
                    func(target, buffer, l, coverage);
                else
                    drawBufferSpan(data, buffer, buffer_size,
                                   spans->x + spans->len - length,
                                   spans->y, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;

            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);

            qreal x = data->m21 * cy + data->m11 * cx + data->dx;
            qreal y = data->m22 * cy + data->m12 * cx + data->dy;
            qreal w = data->m23 * cy + data->m13 * cx + data->m33;

            int length = spans->len;
            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    const qreal iw = w == 0 ? 1 : 1 / w;
                    const qreal tx = x * iw;
                    const qreal ty = y * iw;
                    const int px = int(tx) - (tx < 0);
                    const int py = int(ty) - (ty < 0);

                    bool out = (px < 0) || (px >= image_width)
                               || (py < 0) || (py >= image_height);

                    int y_offset = py * scanline_offset;
                    *b = out ? uint(0) : image_bits[y_offset + px];
                    x += fdx;
                    y += fdy;
                    w += fdw;

                    ++b;
                }
                if (spanMethod == RegularSpans)
                    func(target, buffer, l, coverage);
                else
                    drawBufferSpan(data, buffer, buffer_size,
                                   spans->x + spans->len - length,
                                   spans->y, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    }
}

template <class DST, class SRC>
Q_STATIC_TEMPLATE_FUNCTION void blendTransformed(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData*>(userData);
    QPainter::CompositionMode mode = data->rasterBuffer->compositionMode;

    if (mode != QPainter::CompositionMode_SourceOver) {
        blend_src_generic<RegularSpans>(count, spans, userData);
        return;
    }

    SRC buffer[buffer_size];
    const int image_width = data->texture.width;
    const int image_height = data->texture.height;

    if (data->fast_matrix) {
        // The increment pr x in the scanline
        const int fdx = (int)(data->m11 * fixed_scale);
        const int fdy = (int)(data->m12 * fixed_scale);

        while (count--) {
            const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            if (coverage == 0) {
                ++spans;
                continue;
            }

            DST *dest = (DST*)data->rasterBuffer->scanLine(spans->y)
                        + spans->x;
            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);
            int x = int((data->m21 * cy
                         + data->m11 * cx + data->dx) * fixed_scale);
            int y = int((data->m22 * cy
                         + data->m12 * cx + data->dy) * fixed_scale);
            int length = spans->len;

            while (length) {
                const int l = qMin(length, buffer_size);

                const SRC *end = buffer + l;
                SRC *b = buffer;
                while (b < end) {
                    const int px = (x >> 16);
                    const int py = (y >> 16);

                    if ((px < 0) || (px >= image_width) ||
                        (py < 0) || (py >= image_height))
                    {
                        *b = 0;
                    } else {
                        *b = ((SRC*)data->texture.scanLine(py))[px];
                    }
                    ++b;

                    x += fdx;
                    y += fdy;
                }

                if (!SRC::hasAlpha() && coverage == 255) {
                    qt_memconvert(dest, buffer, l);
                } else if (sizeof(DST) == 3 && sizeof(SRC) == 3 && l >= 4 &&
                           (quintptr(dest) & 3) == (quintptr(buffer) & 3))
                {
                    blendUntransformed_dest24(dest, buffer, coverage, l);
                } else if (sizeof(DST) == 2 && sizeof(SRC) == 2 && l >= 2 &&
                           (quintptr(dest) & 3) == (quintptr(buffer) & 3)) {
                    blendUntransformed_dest16(dest, buffer, coverage, l);
                } else {
                    blendUntransformed_unaligned(dest, buffer, coverage, l);
                }

                dest += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        while (count--) {
            const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            if (coverage == 0) {
                ++spans;
                continue;
            }

            DST *dest = (DST*)data->rasterBuffer->scanLine(spans->y)
                        + spans->x;

            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);

            qreal x = data->m21 * cy + data->m11 * cx + data->dx;
            qreal y = data->m22 * cy + data->m12 * cx + data->dy;
            qreal w = data->m23 * cy + data->m13 * cx + data->m33;

            int length = spans->len;
            while (length) {
                const int l = qMin(length, buffer_size);
                const SRC *end = buffer + l;
                SRC *b = buffer;
                while (b < end) {
                    const qreal iw = w == 0 ? 1 : 1 / w;
                    const qreal tx = x * iw;
                    const qreal ty = y * iw;

                    const int px = int(tx) - (tx < 0);
                    const int py = int(ty) - (ty < 0);

                    if ((px < 0) || (px >= image_width) ||
                        (py < 0) || (py >= image_height))
                    {
                        *b = 0;
                    } else {
                        *b = ((SRC*)data->texture.scanLine(py))[px];
                    }
                    ++b;

                    x += fdx;
                    y += fdy;
                    w += fdw;
                }
                if (!SRC::hasAlpha() && coverage == 255) {
                    qt_memconvert(dest, buffer, l);
                } else if (sizeof(DST) == 3 && sizeof(SRC) == 3 && l >= 4 &&
                           (quintptr(dest) & 3) == (quintptr(buffer) & 3))
                {
                    blendUntransformed_dest24(dest, buffer, coverage, l);
                } else if (sizeof(DST) == 2 && sizeof(SRC) == 2 && l >= 2 &&
                           (quintptr(dest) & 3) == (quintptr(buffer) & 3)) {
                    blendUntransformed_dest16(dest, buffer, coverage, l);
                } else {
                    blendUntransformed_unaligned(dest, buffer, coverage, l);
                }

                dest += l;
                length -= l;
            }
            ++spans;
        }
    }
}

static void blend_transformed_rgb888(int count, const QSpan *spans,
                                     void *userData)
{
#if defined(QT_QWS_DEPTH_24)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_RGB888)
        blendTransformed<qrgb888, qrgb888>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_argb6666(int count, const QSpan *spans,
                                       void *userData)
{
#if defined(QT_QWS_DEPTH_18)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB6666_Premultiplied)
        blendTransformed<qargb6666, qargb6666>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB666)
        blendTransformed<qargb6666, qrgb666>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_rgb666(int count, const QSpan *spans,
                                       void *userData)
{
#if defined(QT_QWS_DEPTH_18)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB6666_Premultiplied)
        blendTransformed<qrgb666, qargb6666>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB666)
        blendTransformed<qrgb666, qrgb666>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_argb8565(int count, const QSpan *spans,
                                         void *userData)
{
#if defined(QT_QWS_DEPTH_16)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB8565_Premultiplied)
        blendTransformed<qargb8565, qargb8565>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB16)
        blendTransformed<qargb8565, qrgb565>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_rgb565(int count, const QSpan *spans,
                                       void *userData)
{
#if !defined(Q_WS_QWS) || defined(QT_QWS_DEPTH_16)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB8565_Premultiplied)
        blendTransformed<qrgb565, qargb8565>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB16)
        blendTransformed<qrgb565, qrgb565>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_argb8555(int count, const QSpan *spans,
                                         void *userData)
{
#if defined(QT_QWS_DEPTH_15)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB8555_Premultiplied)
        blendTransformed<qargb8555, qargb8555>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB555)
        blendTransformed<qargb8555, qrgb555>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_rgb555(int count, const QSpan *spans,
                                       void *userData)
{
#if defined(QT_QWS_DEPTH_15)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB8555_Premultiplied)
        blendTransformed<qrgb555, qargb8555>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB555)
        blendTransformed<qrgb555, qrgb555>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_argb4444(int count, const QSpan *spans,
                                         void *userData)
{
#if defined(QT_QWS_DEPTH_12)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB4444_Premultiplied)
        blendTransformed<qargb4444, qargb4444>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB444)
        blendTransformed<qargb4444, qrgb444>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_rgb444(int count, const QSpan *spans,
                                       void *userData)
{
#if defined(QT_QWS_DEPTH_12)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB4444_Premultiplied)
        blendTransformed<qrgb444, qargb4444>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB444)
        blendTransformed<qrgb444, qrgb444>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

template <SpanMethod spanMethod>
Q_STATIC_TEMPLATE_FUNCTION void blend_transformed_tiled_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_src_generic<spanMethod>(count, spans, userData);
        return;
    }

    CompositionFunction func = functionForMode[data->rasterBuffer->compositionMode];
    uint buffer[buffer_size];

    int image_width = data->texture.width;
    int image_height = data->texture.height;
    const int scanline_offset = data->texture.bytesPerLine / 4;

    if (data->fast_matrix) {
        // The increment pr x in the scanline
        int fdx = (int)(data->m11 * fixed_scale);
        int fdy = (int)(data->m12 * fixed_scale);

        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;

            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);

            int x = int((data->m21 * cy
                         + data->m11 * cx + data->dx) * fixed_scale);
            int y = int((data->m22 * cy
                         + data->m12 * cx + data->dy) * fixed_scale);

            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            int length = spans->len;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    int px = x >> 16;
                    int py = y >> 16;
                    px %= image_width;
                    py %= image_height;
                    if (px < 0) px += image_width;
                    if (py < 0) py += image_height;
                    int y_offset = py * scanline_offset;

                    Q_ASSERT(px >= 0 && px < image_width);
                    Q_ASSERT(py >= 0 && py < image_height);

                    *b = image_bits[y_offset + px];
                    x += fdx;
                    y += fdy;
                    ++b;
                }
                if (spanMethod == RegularSpans)
                    func(target, buffer, l, coverage);
                else
                    drawBufferSpan(data, buffer, buffer_size,
                                   spans->x + spans->len - length,
                                   spans->y, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;

            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);

            qreal x = data->m21 * cy + data->m11 * cx + data->dx;
            qreal y = data->m22 * cy + data->m12 * cx + data->dy;
            qreal w = data->m23 * cy + data->m13 * cx + data->m33;

            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            int length = spans->len;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    const qreal iw = w == 0 ? 1 : 1 / w;
                    const qreal tx = x * iw;
                    const qreal ty = y * iw;
                    int px = int(tx) - (tx < 0);
                    int py = int(ty) - (ty < 0);

                    px %= image_width;
                    py %= image_height;
                    if (px < 0) px += image_width;
                    if (py < 0) py += image_height;
                    int y_offset = py * scanline_offset;

                    Q_ASSERT(px >= 0 && px < image_width);
                    Q_ASSERT(py >= 0 && py < image_height);

                    *b = image_bits[y_offset + px];
                    x += fdx;
                    y += fdy;
                    w += fdw;
                    //force increment to avoid /0
                    if (!w) {
                        w += fdw;
                    }
                    ++b;
                }
                if (spanMethod == RegularSpans)
                    func(target, buffer, l, coverage);
                else
                    drawBufferSpan(data, buffer, buffer_size,
                                   spans->x + spans->len - length,
                                   spans->y, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    }
}

template <class DST, class SRC>
Q_STATIC_TEMPLATE_FUNCTION void blendTransformedTiled(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData*>(userData);
    QPainter::CompositionMode mode = data->rasterBuffer->compositionMode;

    if (mode != QPainter::CompositionMode_SourceOver) {
        blend_src_generic<RegularSpans>(count, spans, userData);
        return;
    }

    SRC buffer[buffer_size];
    const int image_width = data->texture.width;
    const int image_height = data->texture.height;

    if (data->fast_matrix) {
        // The increment pr x in the scanline
        const int fdx = (int)(data->m11 * fixed_scale);
        const int fdy = (int)(data->m12 * fixed_scale);

        while (count--) {
            const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            if (coverage == 0) {
                ++spans;
                continue;
            }

            DST *dest = (DST*)data->rasterBuffer->scanLine(spans->y)
                        + spans->x;
            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);
            int x = int((data->m21 * cy
                         + data->m11 * cx + data->dx) * fixed_scale);
            int y = int((data->m22 * cy
                         + data->m12 * cx + data->dy) * fixed_scale);
            int length = spans->len;

            while (length) {
                const int l = qMin(length, buffer_size);

                const SRC *end = buffer + l;
                SRC *b = buffer;
                while (b < end) {
                    int px = (x >> 16) % image_width;
                    int py = (y >> 16) % image_height;

                    if (px < 0)
                        px += image_width;
                    if (py < 0)
                        py += image_height;

                    *b = ((SRC*)data->texture.scanLine(py))[px];
                    ++b;

                    x += fdx;
                    y += fdy;
                }

                if (!SRC::hasAlpha() && coverage == 255) {
                    qt_memconvert(dest, buffer, l);
                } else if (sizeof(DST) == 3 && sizeof(SRC) == 3 && l >= 4 &&
                           (quintptr(dest) & 3) == (quintptr(buffer) & 3))
                {
                    blendUntransformed_dest24(dest, buffer, coverage, l);
                } else if (sizeof(DST) == 2 && sizeof(SRC) == 2 && l >= 2 &&
                           (quintptr(dest) & 3) == (quintptr(buffer) & 3)) {
                    blendUntransformed_dest16(dest, buffer, coverage, l);
                } else {
                    blendUntransformed_unaligned(dest, buffer, coverage, l);
                }

                dest += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        while (count--) {
            const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            if (coverage == 0) {
                ++spans;
                continue;
            }

            DST *dest = (DST*)data->rasterBuffer->scanLine(spans->y)
                        + spans->x;

            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);

            qreal x = data->m21 * cy + data->m11 * cx + data->dx;
            qreal y = data->m22 * cy + data->m12 * cx + data->dy;
            qreal w = data->m23 * cy + data->m13 * cx + data->m33;

            int length = spans->len;
            while (length) {
                const int l = qMin(length, buffer_size);
                const SRC *end = buffer + l;
                SRC *b = buffer;
                while (b < end) {
                    const qreal iw = w == 0 ? 1 : 1 / w;
                    const qreal tx = x * iw;
                    const qreal ty = y * iw;

                    int px = int(tx) - (tx < 0);
                    int py = int(ty) - (ty < 0);

                    px %= image_width;
                    py %= image_height;
                    if (px < 0)
                        px += image_width;
                    if (py < 0)
                        py += image_height;

                    *b = ((SRC*)data->texture.scanLine(py))[px];
                    ++b;

                    x += fdx;
                    y += fdy;
                    w += fdw;
                    // force increment to avoid /0
                    if (!w)
                        w += fdw;
                }
                if (!SRC::hasAlpha() && coverage == 255) {
                    qt_memconvert(dest, buffer, l);
                } else if (sizeof(DST) == 3 && sizeof(SRC) == 3 && l >= 4 &&
                           (quintptr(dest) & 3) == (quintptr(buffer) & 3))
                {
                    blendUntransformed_dest24(dest, buffer, coverage, l);
                } else if (sizeof(DST) == 2 && sizeof(SRC) == 2 && l >= 2 &&
                           (quintptr(dest) & 3) == (quintptr(buffer) & 3)) {
                    blendUntransformed_dest16(dest, buffer, coverage, l);
                } else {
                    blendUntransformed_unaligned(dest, buffer, coverage, l);
                }

                dest += l;
                length -= l;
            }
            ++spans;
        }
    }
}

static void blend_transformed_tiled_rgb888(int count, const QSpan *spans,
                                           void *userData)
{
#if defined(QT_QWS_DEPTH_24)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_RGB888)
        blendTransformedTiled<qrgb888, qrgb888>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_tiled_argb6666(int count, const QSpan *spans,
                                             void *userData)
{
#if defined(QT_QWS_DEPTH_18)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB6666_Premultiplied)
        blendTransformedTiled<qargb6666, qargb6666>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB666)
        blendTransformedTiled<qargb6666, qrgb666>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_tiled_rgb666(int count, const QSpan *spans,
                                           void *userData)
{
#if defined(QT_QWS_DEPTH_18)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB6666_Premultiplied)
        blendTransformedTiled<qrgb666, qargb6666>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB666)
        blendTransformedTiled<qrgb666, qrgb666>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_tiled_argb8565(int count, const QSpan *spans,
                                             void *userData)
{
#if defined(QT_QWS_DEPTH_16)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB8565_Premultiplied)
        blendTransformedTiled<qargb8565, qargb8565>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB16)
        blendTransformedTiled<qargb8565, qrgb565>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_tiled_rgb565(int count, const QSpan *spans,
                                           void *userData)
{
#if !defined(Q_WS_QWS) || defined(QT_QWS_DEPTH_16)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB8565_Premultiplied)
        blendTransformedTiled<qrgb565, qargb8565>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB16)
        blendTransformedTiled<qrgb565, qrgb565>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_tiled_argb8555(int count, const QSpan *spans,
                                             void *userData)
{
#if defined(QT_QWS_DEPTH_15)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB8555_Premultiplied)
        blendTransformedTiled<qargb8555, qargb8555>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB555)
        blendTransformedTiled<qargb8555, qrgb555>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_tiled_rgb555(int count, const QSpan *spans,
                                           void *userData)
{
#if defined(QT_QWS_DEPTH_15)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB8555_Premultiplied)
        blendTransformedTiled<qrgb555, qargb8555>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB555)
        blendTransformedTiled<qrgb555, qrgb555>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_tiled_argb4444(int count, const QSpan *spans,
                                             void *userData)
{
#if defined(QT_QWS_DEPTH_12)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB4444_Premultiplied)
        blendTransformedTiled<qargb4444, qargb4444>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB444)
        blendTransformedTiled<qargb4444, qrgb444>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

static void blend_transformed_tiled_rgb444(int count, const QSpan *spans,
                                           void *userData)
{
#if defined(QT_QWS_DEPTH_12)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    if (data->texture.format == QImage::Format_ARGB4444_Premultiplied)
        blendTransformedTiled<qrgb444, qargb4444>(count, spans, userData);
    else if (data->texture.format == QImage::Format_RGB444)
        blendTransformedTiled<qrgb444, qrgb444>(count, spans, userData);
    else
#endif
        blend_src_generic<RegularSpans>(count, spans, userData);
}

# define SPANFUNC_POINTER(Name, Arg) Name<Arg>


/* Image formats here are target formats */
static const ProcessSpans processTextureSpans[NBlendTypes][QImage::NImageFormats] = {
    // Untransformed
    {
        0, // Invalid
        SPANFUNC_POINTER(blend_untransformed_generic, RegularSpans), // Mono
        SPANFUNC_POINTER(blend_untransformed_generic, RegularSpans), // MonoLsb
        SPANFUNC_POINTER(blend_untransformed_generic, RegularSpans), // Indexed8
        SPANFUNC_POINTER(blend_untransformed_generic, RegularSpans), // RGB32
        SPANFUNC_POINTER(blend_untransformed_generic, RegularSpans), // ARGB32
        SPANFUNC_POINTER(blend_untransformed_argb, RegularSpans), // ARGB32_Premultiplied
        blend_untransformed_rgb565,
        blend_untransformed_argb8565,
        blend_untransformed_rgb666,
        blend_untransformed_argb6666,
        blend_untransformed_rgb555,
        blend_untransformed_argb8555,
        blend_untransformed_rgb888,
        blend_untransformed_rgb444,
        blend_untransformed_argb4444,
    },
    // Tiled
    {
        0, // Invalid
        SPANFUNC_POINTER(blend_tiled_generic, RegularSpans), // Mono
        SPANFUNC_POINTER(blend_tiled_generic, RegularSpans), // MonoLsb
        SPANFUNC_POINTER(blend_tiled_generic, RegularSpans), // Indexed8
        SPANFUNC_POINTER(blend_tiled_generic, RegularSpans), // RGB32
        SPANFUNC_POINTER(blend_tiled_generic, RegularSpans), // ARGB32
        SPANFUNC_POINTER(blend_tiled_argb, RegularSpans), // ARGB32_Premultiplied
        blend_tiled_rgb565,
        blend_tiled_argb8565,
        blend_tiled_rgb666,
        blend_tiled_argb6666,
        blend_tiled_rgb555,
        blend_tiled_argb8555,
        blend_tiled_rgb888,
        blend_tiled_rgb444,
        blend_tiled_argb4444,
    },
    // Transformed
    {
        0, // Invalid
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // Mono
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // MonoLsb
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // Indexed8
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // RGB32
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // ARGB32
        SPANFUNC_POINTER(blend_transformed_argb, RegularSpans), // ARGB32_Premultiplied
        blend_transformed_rgb565,
        blend_transformed_argb8565,
        blend_transformed_rgb666,
        blend_transformed_argb6666,
        blend_transformed_rgb555,
        blend_transformed_argb8555,
        blend_transformed_rgb888,
        blend_transformed_rgb444,
        blend_transformed_argb4444,
    },
     // TransformedTiled
    {
        0,
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // Mono
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // MonoLsb
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // Indexed8
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // RGB32
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // ARGB32
        SPANFUNC_POINTER(blend_transformed_tiled_argb, RegularSpans), // ARGB32_Premultiplied
        blend_transformed_tiled_rgb565,
        blend_transformed_tiled_argb8565,
        blend_transformed_tiled_rgb666,
        blend_transformed_tiled_argb6666,
        blend_transformed_tiled_rgb555,
        blend_transformed_tiled_argb8555,
        blend_transformed_tiled_rgb888,
        blend_transformed_tiled_rgb444,
        blend_transformed_tiled_argb4444
    },
    // Bilinear
    {
        0,
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // Mono
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // MonoLsb
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // Indexed8
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // RGB32
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // ARGB32
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // ARGB32_Premultiplied
        blend_transformed_bilinear_rgb565,
        blend_transformed_bilinear_argb8565,
        blend_transformed_bilinear_rgb666,
        blend_transformed_bilinear_argb6666,
        blend_transformed_bilinear_rgb555,
        blend_transformed_bilinear_argb8555,
        blend_transformed_bilinear_rgb888,
        blend_transformed_bilinear_rgb444,
        blend_transformed_bilinear_argb4444,
    },
    // BilinearTiled
    {
        0,
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // Mono
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // MonoLsb
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // Indexed8
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // RGB32
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // ARGB32
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // ARGB32_Premultiplied
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // RGB16
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // ARGB8565_Premultiplied
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // RGB666
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // ARGB6666_Premultiplied
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // RGB555
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // ARGB8555_Premultiplied
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // RGB888
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // RGB444
        SPANFUNC_POINTER(blend_src_generic, RegularSpans), // ARGB4444_Premultiplied
    }
};

#if defined (Q_WS_QWS) && !defined(QT_NO_RASTERCALLBACKS)
static const ProcessSpans processTextureSpansCallback[NBlendTypes][QImage::NImageFormats] = {
    // Untransformed
    {
        0, // Invalid
        blend_untransformed_generic<CallbackSpans>,   // Mono
        blend_untransformed_generic<CallbackSpans>,   // MonoLsb
        blend_untransformed_generic<CallbackSpans>,   // Indexed8
        blend_untransformed_generic<CallbackSpans>,   // RGB32
        blend_untransformed_generic<CallbackSpans>,   // ARGB32
        blend_untransformed_argb<CallbackSpans>,      // ARGB32_Premultiplied
        blend_untransformed_generic<CallbackSpans>,   // RGB16
        blend_untransformed_generic<CallbackSpans>,   // ARGB8565_Premultiplied
        blend_untransformed_generic<CallbackSpans>,   // RGB666
        blend_untransformed_generic<CallbackSpans>,   // ARGB6666_Premultiplied
        blend_untransformed_generic<CallbackSpans>,   // RGB555
        blend_untransformed_generic<CallbackSpans>,   // ARGB8555_Premultiplied
        blend_untransformed_generic<CallbackSpans>,   // RGB888
        blend_untransformed_generic<CallbackSpans>,   // RGB444
        blend_untransformed_generic<CallbackSpans>    // ARGB4444_Premultiplied
    },
    // Tiled
    {
        0, // Invalid
        blend_tiled_generic<CallbackSpans>,   // Mono
        blend_tiled_generic<CallbackSpans>,   // MonoLsb
        blend_tiled_generic<CallbackSpans>,   // Indexed8
        blend_tiled_generic<CallbackSpans>,   // RGB32
        blend_tiled_generic<CallbackSpans>,   // ARGB32
        blend_tiled_argb<CallbackSpans>,      // ARGB32_Premultiplied
        blend_tiled_generic<CallbackSpans>,   // RGB16
        blend_tiled_generic<CallbackSpans>,   // ARGB8565_Premultiplied
        blend_tiled_generic<CallbackSpans>,   // RGB666
        blend_tiled_generic<CallbackSpans>,   // ARGB6666_Premultiplied
        blend_tiled_generic<CallbackSpans>,   // RGB555
        blend_tiled_generic<CallbackSpans>,   // ARGB8555_Premultiplied
        blend_tiled_generic<CallbackSpans>,   // RGB888
        blend_tiled_generic<CallbackSpans>,   // RGB444
        blend_tiled_generic<CallbackSpans>    // ARGB4444_Premultiplied
    },
    // Transformed
    {
        0, // Invalid
        blend_src_generic<CallbackSpans>,   // Mono
        blend_src_generic<CallbackSpans>,   // MonoLsb
        blend_src_generic<CallbackSpans>,   // Indexed8
        blend_src_generic<CallbackSpans>,   // RGB32
        blend_src_generic<CallbackSpans>,   // ARGB32
        blend_transformed_argb<CallbackSpans>, // ARGB32_Premultiplied
        blend_src_generic<CallbackSpans>,   // RGB16
        blend_src_generic<CallbackSpans>,   // ARGB8565_Premultiplied
        blend_src_generic<CallbackSpans>,   // RGB666
        blend_src_generic<CallbackSpans>,   // ARGB6666_Premultiplied
        blend_src_generic<CallbackSpans>,   // RGB555
        blend_src_generic<CallbackSpans>,   // ARGB8555_Premultiplied
        blend_src_generic<CallbackSpans>,   // RGB888
        blend_src_generic<CallbackSpans>,   // RGB444
        blend_src_generic<CallbackSpans>,   // ARGB4444_Premultiplied
    },
     // TransformedTiled
    {
        0,
        blend_src_generic<CallbackSpans>,   // Mono
        blend_src_generic<CallbackSpans>,   // MonoLsb
        blend_src_generic<CallbackSpans>,   // Indexed8
        blend_src_generic<CallbackSpans>,   // RGB32
        blend_src_generic<CallbackSpans>,   // ARGB32
        blend_transformed_tiled_argb<CallbackSpans>, // ARGB32_Premultiplied
        blend_src_generic<CallbackSpans>,   // RGB16
        blend_src_generic<CallbackSpans>,   // ARGB8565_Premultiplied
        blend_src_generic<CallbackSpans>,   // RGB666
        blend_src_generic<CallbackSpans>,   // ARGB6666_Premultiplied
        blend_src_generic<CallbackSpans>,   // RGB555
        blend_src_generic<CallbackSpans>,   // ARGB8555_Premultiplied
        blend_src_generic<CallbackSpans>,   // RGB888
        blend_src_generic<CallbackSpans>,   // RGB444
        blend_src_generic<CallbackSpans>    // ARGB4444_Premultiplied
    },
    // Bilinear
    {
        0,
        blend_src_generic<CallbackSpans>,   // Mono
        blend_src_generic<CallbackSpans>,   // MonoLsb
        blend_src_generic<CallbackSpans>,   // Indexed8
        blend_src_generic<CallbackSpans>,   // RGB32
        blend_src_generic<CallbackSpans>,   // ARGB32
        blend_src_generic<CallbackSpans>, // ARGB32_Premultiplied
        blend_src_generic<CallbackSpans>,   // RGB16
        blend_src_generic<CallbackSpans>,   // ARGB8565_Premultiplied
        blend_src_generic<CallbackSpans>,   // RGB666
        blend_src_generic<CallbackSpans>,   // ARGB6666_Premultiplied
        blend_src_generic<CallbackSpans>,   // RGB555
        blend_src_generic<CallbackSpans>,   // ARGB8555_Premultiplied
        blend_src_generic<CallbackSpans>,   // RGB888
        blend_src_generic<CallbackSpans>,   // RGB444
        blend_src_generic<CallbackSpans>    // ARGB4444_Premultiplied
    },
    // BilinearTiled
    {
        0,
        blend_src_generic<CallbackSpans>,   // Mono
        blend_src_generic<CallbackSpans>,   // MonoLsb
        blend_src_generic<CallbackSpans>,   // Indexed8
        blend_src_generic<CallbackSpans>,   // RGB32
        blend_src_generic<CallbackSpans>,   // ARGB32
        blend_src_generic<CallbackSpans>, // ARGB32_Premultiplied
        blend_src_generic<CallbackSpans>,   // RGB16
        blend_src_generic<CallbackSpans>,   // ARGB8565_Premultiplied
        blend_src_generic<CallbackSpans>,   // RGB666
        blend_src_generic<CallbackSpans>,   // ARGB6666_Premultiplied
        blend_src_generic<CallbackSpans>,   // RGB555
        blend_src_generic<CallbackSpans>,   // ARGB8555_Premultiplied
        blend_src_generic<CallbackSpans>,   // RGB888
        blend_src_generic<CallbackSpans>,   // RGB444
        blend_src_generic<CallbackSpans>    // ARGB4444_Premultiplied
    }
};
#endif // QT_NO_RASTERCALLBACKS

void qBlendTexture(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    ProcessSpans proc = processTextureSpans[getBlendType(data)][data->rasterBuffer->format];
    proc(count, spans, userData);
}

#if defined (Q_WS_QWS) &&  !defined(QT_NO_RASTERCALLBACKS)
void qBlendTextureCallback(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    ProcessSpans proc = processTextureSpansCallback[getBlendType(data)][data->rasterBuffer->format];
    proc(count, spans, userData);
}
#endif // QT_NO_RASTERCALLBACKS

template <class DST>
inline void qt_bitmapblit_template(QRasterBuffer *rasterBuffer,
                                   int x, int y, quint32 color,
                                   const uchar *map,
                                   int mapWidth, int mapHeight, int mapStride,
                                   DST dummy = 0)
{
    Q_UNUSED(dummy);
    const DST c = qt_colorConvert<DST, quint32>(color, 0);
    DST *dest = reinterpret_cast<DST*>(rasterBuffer->scanLine(y)) + x;
    const int destStride = rasterBuffer->bytesPerLine() / sizeof(DST);

    if (mapWidth > 8) {
        while (mapHeight--) {
            int x0 = 0;
            int n = 0;
            for (int x = 0; x < mapWidth; x += 8) {
                uchar s = map[x >> 3];
                for (int i = 0; i < 8; ++i) {
                    if (s & 0x80) {
                        ++n;
                    } else {
                        if (n) {
                            qt_memfill(dest + x0, c, n);
                            x0 += n + 1;
                            n = 0;
                        } else {
                            ++x0;
                        }
                        if (!s) {
                            x0 += 8 - 1 - i;
                            break;
                        }
                    }
                    s <<= 1;
                }
            }
            if (n)
                qt_memfill(dest + x0, c, n);
            dest += destStride;
            map += mapStride;
        }
    } else {
        while (mapHeight--) {
            int x0 = 0;
            int n = 0;
            for (uchar s = *map; s; s <<= 1) {
                if (s & 0x80) {
                    ++n;
                } else if (n) {
                    qt_memfill(dest + x0, c, n);
                    x0 += n + 1;
                    n = 0;
                } else {
                    ++x0;
                }
            }
            if (n)
                qt_memfill(dest + x0, c, n);
            dest += destStride;
            map += mapStride;
        }
    }
}

static void qt_gradient_quint32(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    bool isVerticalGradient =
        data->txop <= QTransform::TxScale &&
        data->type == QSpanData::LinearGradient &&
        data->gradient.linear.end.x == data->gradient.linear.origin.x;

    if (isVerticalGradient) {
        LinearGradientValues linear;
        getLinearGradientValues(&linear, data);

        CompositionFunctionSolid funcSolid =
            functionForModeSolid[data->rasterBuffer->compositionMode];

        /*
            The logic for vertical gradient calculations is a mathematically
            reduced copy of that in fetchLinearGradient() - which is basically:

                qreal ry = data->m22 * (y + 0.5) + data->dy;
                qreal t = linear.dy*ry + linear.off;
                t *= (GRADIENT_STOPTABLE_SIZE - 1);
                quint32 color =
                    qt_gradient_pixel_fixed(&data->gradient,
                                            int(t * FIXPT_SIZE));

            This has then been converted to fixed point to improve performance.
         */
        const int gss = GRADIENT_STOPTABLE_SIZE - 1;
        int yinc = int((linear.dy * data->m22 * gss) * FIXPT_SIZE);
        int off = int((((linear.dy * (data->m22 * qreal(0.5) + data->dy) + linear.off) * gss) * FIXPT_SIZE));

        while (count--) {
            int y = spans->y;
            int x = spans->x;

            quint32 *dst = (quint32 *)(data->rasterBuffer->scanLine(y)) + x;
            quint32 color =
                qt_gradient_pixel_fixed(&data->gradient, yinc * y + off);

            funcSolid(dst, spans->len, color, spans->coverage);
            ++spans;
        }

    } else {
        blend_src_generic<RegularSpans>(count, spans, userData);
    }
}

static void qt_gradient_quint16(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    bool isVerticalGradient =
        data->txop <= QTransform::TxScale &&
        data->type == QSpanData::LinearGradient &&
        data->gradient.linear.end.x == data->gradient.linear.origin.x;

    if (isVerticalGradient) {

        LinearGradientValues linear;
        getLinearGradientValues(&linear, data);

        /*
            The logic for vertical gradient calculations is a mathematically
            reduced copy of that in fetchLinearGradient() - which is basically:

                qreal ry = data->m22 * (y + 0.5) + data->dy;
                qreal t = linear.dy*ry + linear.off;
                t *= (GRADIENT_STOPTABLE_SIZE - 1);
                quint32 color =
                    qt_gradient_pixel_fixed(&data->gradient,
                                            int(t * FIXPT_SIZE));

            This has then been converted to fixed point to improve performance.
         */
        const int gss = GRADIENT_STOPTABLE_SIZE - 1;
        int yinc = int((linear.dy * data->m22 * gss) * FIXPT_SIZE);
        int off = int((((linear.dy * (data->m22 * qreal(0.5) + data->dy) + linear.off) * gss) * FIXPT_SIZE));

        uint oldColor = data->solid.color;
        while (count--) {
            int y = spans->y;

            quint32 color = qt_gradient_pixel_fixed(&data->gradient, yinc * y + off);

            data->solid.color = color;
            blend_color_rgb16(1, spans, userData);
            ++spans;
        }
        data->solid.color = oldColor;

    } else {
        blend_src_generic<RegularSpans>(count, spans, userData);
    }
}

inline static void qt_bitmapblit_quint32(QRasterBuffer *rasterBuffer,
                                   int x, int y, quint32 color,
                                   const uchar *map,
                                   int mapWidth, int mapHeight, int mapStride)
{
    qt_bitmapblit_template<quint32>(rasterBuffer, x,  y,  color,
                                    map, mapWidth, mapHeight, mapStride);
}

inline static void qt_bitmapblit_quint16(QRasterBuffer *rasterBuffer,
                                   int x, int y, quint32 color,
                                   const uchar *map,
                                   int mapWidth, int mapHeight, int mapStride)
{
    qt_bitmapblit_template<quint16>(rasterBuffer, x,  y,  color,
                                    map, mapWidth, mapHeight, mapStride);
}


uchar qt_pow_rgb_gamma[256];
uchar qt_pow_rgb_invgamma[256];

uint qt_pow_gamma[256];
uchar qt_pow_invgamma[2048];

static void qt_alphamapblit_quint16(QRasterBuffer *rasterBuffer,
                                    int x, int y, quint32 color,
                                    const uchar *map,
                                    int mapWidth, int mapHeight, int mapStride,
                                    const QClipData *)
{
    const quint16 c = qt_colorConvert<quint16, quint32>(color, 0);
    quint16 *dest = reinterpret_cast<quint16*>(rasterBuffer->scanLine(y)) + x;
    const int destStride = rasterBuffer->bytesPerLine() / sizeof(quint16);

    while (mapHeight--) {
        for (int i = 0; i < mapWidth; ++i) {
            const int coverage = map[i];

            if (coverage == 0) {
                // nothing
            } else if (coverage == 255) {
                dest[i] = c;
            } else {
                int ialpha = 255 - coverage;
                dest[i] = BYTE_MUL_RGB16(c, coverage)
                          + BYTE_MUL_RGB16(dest[i], ialpha);
            }
        }
        dest += destStride;
        map += mapStride;
    }
}

void qt_build_pow_tables() {
    qreal smoothing = qreal(1.7);

#ifdef Q_WS_MAC
    // decided by testing a few things on an iMac, should probably get this from the
    // system...
    smoothing = qreal(2.0);
#endif

#ifdef Q_WS_WIN
    extern qreal qt_fontsmoothing_gamma; // qapplication_win.cpp
    smoothing = qt_fontsmoothing_gamma;
#endif

#ifdef Q_WS_X11
    Q_UNUSED(smoothing);
    for (int i=0; i<256; ++i) {
        qt_pow_rgb_gamma[i] = uchar(i);
        qt_pow_rgb_invgamma[i] = uchar(i);
    }
#else
    for (int i=0; i<256; ++i) {
        qt_pow_rgb_gamma[i] = uchar(qRound(qPow(i / qreal(255.0), smoothing) * 255));
        qt_pow_rgb_invgamma[i] = uchar(qRound(qPow(i / qreal(255.), 1 / smoothing) * 255));
    }
#endif

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    const qreal gray_gamma = 2.31;
    for (int i=0; i<256; ++i)
        qt_pow_gamma[i] = uint(qRound(qPow(i / qreal(255.), gray_gamma) * 2047));
    for (int i=0; i<2048; ++i)
        qt_pow_invgamma[i] = uchar(qRound(qPow(i / qreal(2047.0), 1 / gray_gamma) * 255));
#endif
}

static inline void rgbBlendPixel(quint32 *dst, int coverage, int sr, int sg, int sb)
{
    // Do a gray alphablend...
    int da = qAlpha(*dst);
    int dr = qRed(*dst);
    int dg = qGreen(*dst);
    int db = qBlue(*dst);

    if (da != 255
#if defined (Q_WS_WIN)
        // Work around GDI messing up alpha channel
        && qRed(*dst) <= da && qBlue(*dst) <= da && qGreen(*dst) <= da
#endif
        ) {

        int a = qGray(coverage);
        sr = qt_div_255(qt_pow_rgb_invgamma[sr] * a);
        sg = qt_div_255(qt_pow_rgb_invgamma[sg] * a);
        sb = qt_div_255(qt_pow_rgb_invgamma[sb] * a);

        int ia = 255 - a;
        dr = qt_div_255(dr * ia);
        dg = qt_div_255(dg * ia);
        db = qt_div_255(db * ia);

        *dst = ((a + qt_div_255((255 - a) * da)) << 24)
            |  ((sr + dr) << 16)
            |  ((sg + dg) << 8)
            |  ((sb + db));
        return;
    }

    int mr = qRed(coverage);
    int mg = qGreen(coverage);
    int mb = qBlue(coverage);

    dr = qt_pow_rgb_gamma[dr];
    dg = qt_pow_rgb_gamma[dg];
    db = qt_pow_rgb_gamma[db];

    int nr = qt_div_255((sr - dr) * mr) + dr;
    int ng = qt_div_255((sg - dg) * mg) + dg;
    int nb = qt_div_255((sb - db) * mb) + db;

    nr = qt_pow_rgb_invgamma[nr];
    ng = qt_pow_rgb_invgamma[ng];
    nb = qt_pow_rgb_invgamma[nb];

    *dst = qRgb(nr, ng, nb);
}

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
static inline void grayBlendPixel(quint32 *dst, int coverage, int sr, int sg, int sb)
{
    // Do a gammacorrected gray alphablend...
    int dr = qRed(*dst);
    int dg = qGreen(*dst);
    int db = qBlue(*dst);

    dr = qt_pow_gamma[dr];
    dg = qt_pow_gamma[dg];
    db = qt_pow_gamma[db];

    int alpha = coverage;
    int ialpha = 255 - alpha;
    int nr = (sr * alpha + ialpha * dr) / 255;
    int ng = (sg * alpha + ialpha * dg) / 255;
    int nb = (sb * alpha + ialpha * db) / 255;

    nr = qt_pow_invgamma[nr];
    ng = qt_pow_invgamma[ng];
    nb = qt_pow_invgamma[nb];

    *dst = qRgb(nr, ng, nb);
}
#endif

static void qt_alphamapblit_quint32(QRasterBuffer *rasterBuffer,
                                    int x, int y, quint32 color,
                                    const uchar *map,
                                    int mapWidth, int mapHeight, int mapStride,
                                    const QClipData *clip)
{
    const quint32 c = color;
    const int destStride = rasterBuffer->bytesPerLine() / sizeof(quint32);

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    sr = qt_pow_gamma[sr];
    sg = qt_pow_gamma[sg];
    sb = qt_pow_gamma[sb];
    bool opaque_src = (qAlpha(color) == 255);
#endif

    if (!clip) {
        quint32 *dest = reinterpret_cast<quint32*>(rasterBuffer->scanLine(y)) + x;
        while (mapHeight--) {
            for (int i = 0; i < mapWidth; ++i) {
                const int coverage = map[i];

                if (coverage == 0) {
                    // nothing
                } else if (coverage == 255) {
                    dest[i] = c;
                } else {
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
                    if (QSysInfo::WindowsVersion >= QSysInfo::WV_XP && opaque_src
                        && qAlpha(dest[i]) == 255) {
                        grayBlendPixel(dest+i, coverage, sr, sg, sb);
                    } else
#endif
                    {
                        int ialpha = 255 - coverage;
                        dest[i] = INTERPOLATE_PIXEL_255(c, coverage, dest[i], ialpha);
                    }
                }
            }
            dest += destStride;
            map += mapStride;
        }
    } else {
        int bottom = qMin(y + mapHeight, rasterBuffer->height());

        int top = qMax(y, 0);
        map += (top - y) * mapStride;

        const_cast<QClipData *>(clip)->initialize();
        for (int yp = top; yp<bottom; ++yp) {
            const QClipData::ClipLine &line = clip->m_clipLines[yp];

            quint32 *dest = reinterpret_cast<quint32 *>(rasterBuffer->scanLine(yp));

            for (int i=0; i<line.count; ++i) {
                const QSpan &clip = line.spans[i];

                int start = qMax<int>(x, clip.x);
                int end = qMin<int>(x + mapWidth, clip.x + clip.len);

                for (int xp=start; xp<end; ++xp) {
                    const int coverage = map[xp - x];

                    if (coverage == 0) {
                        // nothing
                    } else if (coverage == 255) {
                        dest[xp] = c;
                    } else {
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
                        if (QSysInfo::WindowsVersion >= QSysInfo::WV_XP && opaque_src
                            && qAlpha(dest[xp]) == 255) {
                            grayBlendPixel(dest+xp, coverage, sr, sg, sb);
                        } else
#endif
                        {
                            int ialpha = 255 - coverage;
                            dest[xp] = INTERPOLATE_PIXEL_255(c, coverage, dest[xp], ialpha);
                        }
                    }

                } // for (i -> line.count)
            } // for (yp -> bottom)
            map += mapStride;
        }
    }
}

static void qt_alphargbblit_quint32(QRasterBuffer *rasterBuffer,
                                    int x, int y, quint32 color,
                                    const uint *src, int mapWidth, int mapHeight, int srcStride,
                                    const QClipData *clip)
{
    const quint32 c = color;

    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);
    int sa = qAlpha(color);

    sr = qt_pow_rgb_gamma[sr];
    sg = qt_pow_rgb_gamma[sg];
    sb = qt_pow_rgb_gamma[sb];

    if (sa == 0)
        return;

    if (!clip) {
        quint32 *dst = reinterpret_cast<quint32*>(rasterBuffer->scanLine(y)) + x;
        const int destStride = rasterBuffer->bytesPerLine() / sizeof(quint32);
        while (mapHeight--) {
            for (int i = 0; i < mapWidth; ++i) {
                const uint coverage = src[i];
                if (coverage == 0xffffffff) {
                    dst[i] = c;
                } else if (coverage != 0xff000000) {
                    rgbBlendPixel(dst+i, coverage, sr, sg, sb);
                }
            }

            dst += destStride;
            src += srcStride;
        }
    } else {
        int bottom = qMin(y + mapHeight, rasterBuffer->height());

        int top = qMax(y, 0);
        src += (top - y) * srcStride;

        const_cast<QClipData *>(clip)->initialize();
        for (int yp = top; yp<bottom; ++yp) {
            const QClipData::ClipLine &line = clip->m_clipLines[yp];

            quint32 *dst = reinterpret_cast<quint32 *>(rasterBuffer->scanLine(yp));

            for (int i=0; i<line.count; ++i) {
                const QSpan &clip = line.spans[i];

                int start = qMax<int>(x, clip.x);
                int end = qMin<int>(x + mapWidth, clip.x + clip.len);

                for (int xp=start; xp<end; ++xp) {
                    const uint coverage = src[xp - x];
                    if (coverage == 0xffffffff) {
                        dst[xp] = c;
                    } else if (coverage != 0xff000000) {
                        rgbBlendPixel(dst+xp, coverage, sr, sg, sb);
                    }
                }
            } // for (i -> line.count)
            src += srcStride;
        } // for (yp -> bottom)

    }
}

template <class T>
inline void qt_rectfill_template(QRasterBuffer *rasterBuffer,
                                 int x, int y, int width, int height,
                                 quint32 color, T dummy = 0)
{
    Q_UNUSED(dummy);

    qt_rectfill<T>(reinterpret_cast<T*>(rasterBuffer->buffer()),
                   qt_colorConvert<T, quint32p>(quint32p::fromRawData(color), 0),
                   x, y, width, height, rasterBuffer->bytesPerLine());
}

#define QT_RECTFILL(T)                                                  \
    inline static void qt_rectfill_##T(QRasterBuffer *rasterBuffer,     \
                                       int x, int y, int width, int height, \
                                       quint32 color)                   \
    {                                                                   \
        qt_rectfill_template<T>(rasterBuffer, x, y, width, height, color); \
    }

QT_RECTFILL(quint32)
QT_RECTFILL(quint16)
QT_RECTFILL(qargb8565)
QT_RECTFILL(qrgb666)
QT_RECTFILL(qargb6666)
QT_RECTFILL(qrgb555)
QT_RECTFILL(qargb8555)
QT_RECTFILL(qrgb888)
QT_RECTFILL(qrgb444)
QT_RECTFILL(qargb4444)
#undef QT_RECTFILL

inline static void qt_rectfill_nonpremul_quint32(QRasterBuffer *rasterBuffer,
                                                 int x, int y, int width, int height,
                                                 quint32 color)
{
    qt_rectfill<quint32>(reinterpret_cast<quint32 *>(rasterBuffer->buffer()),
                         INV_PREMUL(color), x, y, width, height, rasterBuffer->bytesPerLine());
}


// Map table for destination image format. Contains function pointers
// for blends of various types unto the destination

DrawHelper qDrawHelper[QImage::NImageFormats] =
{
    // Format_Invalid,
    { 0, 0, 0, 0, 0, 0 },
    // Format_Mono,
    {
        blend_color_generic,
        SPANFUNC_POINTER(blend_src_generic, RegularSpans),
        0, 0, 0, 0
    },
    // Format_MonoLSB,
    {
        blend_color_generic,
        SPANFUNC_POINTER(blend_src_generic, RegularSpans),
        0, 0, 0, 0
    },
    // Format_Indexed8,
    {
        blend_color_generic,
        SPANFUNC_POINTER(blend_src_generic, RegularSpans),
        0, 0, 0, 0
    },
    // Format_RGB32,
    {
        blend_color_argb,
        qt_gradient_quint32,
        qt_bitmapblit_quint32,
        qt_alphamapblit_quint32,
        qt_alphargbblit_quint32,
        qt_rectfill_quint32
    },
    // Format_ARGB32,
    {
        blend_color_generic,
        qt_gradient_quint32,
        qt_bitmapblit_quint32,
        qt_alphamapblit_quint32,
        qt_alphargbblit_quint32,
        qt_rectfill_nonpremul_quint32
    },
    // Format_ARGB32_Premultiplied
    {
        blend_color_argb,
        qt_gradient_quint32,
        qt_bitmapblit_quint32,
        qt_alphamapblit_quint32,
        qt_alphargbblit_quint32,
        qt_rectfill_quint32
    },
    // Format_RGB16
    {
        blend_color_rgb16,
        qt_gradient_quint16,
        qt_bitmapblit_quint16,
        qt_alphamapblit_quint16,
        0,
        qt_rectfill_quint16
    },
    // Format_ARGB8565_Premultiplied
    {
        SPANFUNC_POINTER_BLENDCOLOR(qargb8565),
        SPANFUNC_POINTER(blend_src_generic, RegularSpans),
        0, 0, 0,
        qt_rectfill_qargb8565
    },
    // Format_RGB666
    {
        SPANFUNC_POINTER_BLENDCOLOR(qrgb666),
        SPANFUNC_POINTER(blend_src_generic, RegularSpans),
        0, 0, 0,
        qt_rectfill_qrgb666
    },
    // Format_ARGB6666_Premultiplied
    {
        SPANFUNC_POINTER_BLENDCOLOR(qargb6666),
        SPANFUNC_POINTER(blend_src_generic, RegularSpans),
        0, 0, 0,
        qt_rectfill_qargb6666
    },
    // Format_RGB555
    {
        SPANFUNC_POINTER_BLENDCOLOR(qrgb555),
        SPANFUNC_POINTER(blend_src_generic, RegularSpans),
        0, 0, 0,
        qt_rectfill_qrgb555
    },
    // Format_ARGB8555_Premultiplied
    {
        SPANFUNC_POINTER_BLENDCOLOR(qargb8555),
        SPANFUNC_POINTER(blend_src_generic, RegularSpans),
        0, 0, 0,
        qt_rectfill_qargb8555
    },
    // Format_RGB888
    {
        SPANFUNC_POINTER_BLENDCOLOR(qrgb888),
        SPANFUNC_POINTER(blend_src_generic, RegularSpans),
        0, 0, 0,
        qt_rectfill_qrgb888
    },
    // Format_RGB444
    {
        SPANFUNC_POINTER_BLENDCOLOR(qrgb444),
        SPANFUNC_POINTER(blend_src_generic, RegularSpans),
        0, 0, 0,
        qt_rectfill_qrgb444
    },
    // Format_ARGB4444_Premultiplied
    {
        SPANFUNC_POINTER_BLENDCOLOR(qargb4444),
        SPANFUNC_POINTER(blend_src_generic, RegularSpans),
        0, 0, 0,
        qt_rectfill_qargb4444
    }
};

#if defined (Q_WS_QWS) && !defined(QT_NO_RASTERCALLBACKS)
DrawHelper qDrawHelperCallback[QImage::NImageFormats] =
{
    // Format_Invalid,
    { 0, 0, 0, 0, 0, 0 },
    // Format_Mono,
    {
        blend_color_generic_callback,
        blend_src_generic<CallbackSpans>,
        0, 0, 0, 0
    },
    // Format_MonoLSB,
    {
        blend_color_generic_callback,
        blend_src_generic<CallbackSpans>,
        0, 0, 0, 0
    },
    // Format_Indexed8,
    {
        blend_color_generic_callback,
        blend_src_generic<CallbackSpans>,
        0, 0, 0, 0
    },
    // Format_RGB32,
    {
        blend_color_generic_callback,
        blend_src_generic<CallbackSpans>,
        0, 0, 0, 0
    },
    // Format_ARGB32,
    {
        blend_color_generic_callback,
        blend_src_generic<CallbackSpans>,
        0, 0, 0, 0
    },
    // Format_ARGB32_Premultiplied
    {
        blend_color_generic_callback,
        blend_src_generic<CallbackSpans>,
        0, 0, 0, 0
    },
    // Format_RGB16
    {
        blend_color_generic_callback,
        blend_src_generic<CallbackSpans>,
        0, 0, 0, 0
    },
    // Format_ARGB8565_Premultiplied
    {
        blend_color_generic_callback,
        blend_src_generic<CallbackSpans>,
        0, 0, 0, 0
    },
    // Format_RGB666
    {
        blend_color_generic_callback,
        blend_src_generic<CallbackSpans>,
        0, 0, 0, 0
    },
    // Format_ARGB6666_Premultiplied
    {
        blend_color_generic_callback,
        blend_src_generic<CallbackSpans>,
        0, 0, 0, 0
    },
    // Format_RGB555
    {
        blend_color_generic_callback,
        blend_src_generic<CallbackSpans>,
        0, 0, 0, 0
    },
    // Format_ARGB8555_Premultiplied
    {
        blend_color_generic_callback,
        blend_src_generic<CallbackSpans>,
        0, 0, 0, 0
    },
    // Format_RGB888
    {
        blend_color_generic_callback,
        blend_src_generic<CallbackSpans>,
        0, 0, 0, 0
    },
    // Format_RGB444
    {
        blend_color_generic_callback,
        blend_src_generic<CallbackSpans>,
        0, 0, 0, 0
    },
    // Format_ARGB4444_Premultiplied
    {
        blend_color_generic_callback,
        blend_src_generic<CallbackSpans>,
        0, 0, 0, 0
    }
};
#endif



#if defined(Q_CC_MSVC) && !defined(_MIPS_)
template <class DST, class SRC>
inline void qt_memfill_template(DST *dest, SRC color, int count)
{
    const DST c = qt_colorConvert<DST, SRC>(color, 0);
    while (count--)
        *dest++ = c;
}

#else

template <class DST, class SRC>
inline void qt_memfill_template(DST *dest, SRC color, int count)
{
    const DST c = qt_colorConvert<DST, SRC>(color, 0);
    int n = (count + 7) / 8;
    switch (count & 0x07)
    {
    case 0: do { *dest++ = c;
    case 7:      *dest++ = c;
    case 6:      *dest++ = c;
    case 5:      *dest++ = c;
    case 4:      *dest++ = c;
    case 3:      *dest++ = c;
    case 2:      *dest++ = c;
    case 1:      *dest++ = c;
    } while (--n > 0);
    }
}

template <>
inline void qt_memfill_template(quint16 *dest, quint16 value, int count)
{
    if (count < 3) {
        switch (count) {
        case 2: *dest++ = value;
        case 1: *dest = value;
        }
        return;
    }

    const int align = (quintptr)(dest) & 0x3;
    switch (align) {
    case 2: *dest++ = value; --count;
    }

    const quint32 value32 = (value << 16) | value;
    qt_memfill(reinterpret_cast<quint32*>(dest), value32, count / 2);
    if (count & 0x1)
        dest[count - 1] = value;
}
#endif

static void qt_memfill_quint16(quint16 *dest, quint16 color, int count)
{
    qt_memfill_template<quint16, quint16>(dest, color, count);
}

typedef void (*qt_memfill32_func)(quint32 *dest, quint32 value, int count);
typedef void (*qt_memfill16_func)(quint16 *dest, quint16 value, int count);
static void qt_memfill32_setup(quint32 *dest, quint32 value, int count);
static void qt_memfill16_setup(quint16 *dest, quint16 value, int count);

qt_memfill32_func qt_memfill32 = qt_memfill32_setup;
qt_memfill16_func qt_memfill16 = qt_memfill16_setup;

void qInitDrawhelperAsm()
{

    qt_memfill32 = qt_memfill_template<quint32, quint32>;
    qt_memfill16 = qt_memfill_quint16; //qt_memfill_template<quint16, quint16>;

    CompositionFunction *functionForModeAsm = 0;
    CompositionFunctionSolid *functionForModeSolidAsm = 0;

    const uint features = qDetectCPUFeatures();
    if (false) {
#ifdef QT_HAVE_SSE2
    } else if (features & SSE2) {
        qt_memfill32 = qt_memfill32_sse2;
        qt_memfill16 = qt_memfill16_sse2;
        qDrawHelper[QImage::Format_RGB32].bitmapBlit = qt_bitmapblit32_sse2;
        qDrawHelper[QImage::Format_ARGB32].bitmapBlit = qt_bitmapblit32_sse2;
        qDrawHelper[QImage::Format_ARGB32_Premultiplied].bitmapBlit = qt_bitmapblit32_sse2;
        qDrawHelper[QImage::Format_RGB16].bitmapBlit = qt_bitmapblit16_sse2;
#endif
#ifdef QT_HAVE_SSE
    } else if (features & SSE) {
//        qt_memfill32 = qt_memfill32_sse;
        qDrawHelper[QImage::Format_RGB16].bitmapBlit = qt_bitmapblit16_sse;
#ifdef QT_HAVE_3DNOW
        if (features & MMX3DNOW) {
            qt_memfill32 = qt_memfill32_sse3dnow;
            qDrawHelper[QImage::Format_RGB16].bitmapBlit = qt_bitmapblit16_sse3dnow;
        }
#endif
#endif // SSE
    }
#ifdef QT_HAVE_MMX
    if (features & MMX) {
        functionForModeAsm = qt_functionForMode_MMX;

        functionForModeSolidAsm = qt_functionForModeSolid_MMX;
        qDrawHelper[QImage::Format_ARGB32_Premultiplied].blendColor = qt_blend_color_argb_mmx;
#ifdef QT_HAVE_3DNOW
        if (features & MMX3DNOW) {
            functionForModeAsm = qt_functionForMode_MMX3DNOW;
            functionForModeSolidAsm = qt_functionForModeSolid_MMX3DNOW;
            qDrawHelper[QImage::Format_ARGB32_Premultiplied].blendColor = qt_blend_color_argb_mmx3dnow;
        }
#endif // 3DNOW

        extern void qt_blend_rgb32_on_rgb32_mmx(uchar *destPixels, int dbpl,
                                                const uchar *srcPixels, int sbpl,
                                                int w, int h,
                                                int const_alpha);
        extern void qt_blend_argb32_on_argb32_mmx(uchar *destPixels, int dbpl,
                                                  const uchar *srcPixels, int sbpl,
                                                  int w, int h,
                                                  int const_alpha);

        qBlendFunctions[QImage::Format_RGB32][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_mmx;
        qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_mmx;
        qBlendFunctions[QImage::Format_RGB32][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_mmx;
        qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_mmx;

    }
#endif // MMX

#ifdef QT_HAVE_SSE
    if (features & SSE) {
        extern void qt_blend_rgb32_on_rgb32_sse(uchar *destPixels, int dbpl,
                                                const uchar *srcPixels, int sbpl,
                                                int w, int h,
                                                int const_alpha);
        extern void qt_blend_argb32_on_argb32_sse(uchar *destPixels, int dbpl,
                                                  const uchar *srcPixels, int sbpl,
                                                  int w, int h,
                                                  int const_alpha);

        qBlendFunctions[QImage::Format_RGB32][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_sse;
        qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_sse;
        qBlendFunctions[QImage::Format_RGB32][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_sse;
        qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_sse;
    }
#endif // SSE

#ifdef QT_HAVE_SSE2
    if (features & SSE2) {
        extern void qt_blend_rgb32_on_rgb32_sse2(uchar *destPixels, int dbpl,
                                                 const uchar *srcPixels, int sbpl,
                                                 int w, int h,
                                                 int const_alpha);
        extern void qt_blend_argb32_on_argb32_sse2(uchar *destPixels, int dbpl,
                                                   const uchar *srcPixels, int sbpl,
                                                   int w, int h,
                                                   int const_alpha);

        qBlendFunctions[QImage::Format_RGB32][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_sse2;
        qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_sse2;
        qBlendFunctions[QImage::Format_RGB32][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_sse2;
        qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_sse2;

        extern const uint * QT_FASTCALL qt_fetch_radial_gradient_sse2(uint *buffer, const Operator *op, const QSpanData *data,
                                                                      int y, int x, int length);

        qt_fetch_radial_gradient = qt_fetch_radial_gradient_sse2;
    }

#ifdef QT_HAVE_SSSE3
    if (features & SSSE3) {
        extern void qt_blend_argb32_on_argb32_ssse3(uchar *destPixels, int dbpl,
                                                    const uchar *srcPixels, int sbpl,
                                                    int w, int h,
                                                    int const_alpha);

        qBlendFunctions[QImage::Format_RGB32][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_ssse3;
        qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_ssse3;
    }
#endif // SSSE3

#endif // SSE2

#ifdef QT_HAVE_SSE
    if (features & SSE) {
        functionForModeAsm = qt_functionForMode_SSE;
        functionForModeSolidAsm = qt_functionForModeSolid_SSE;
        qDrawHelper[QImage::Format_ARGB32_Premultiplied].blendColor = qt_blend_color_argb_sse;
#ifdef QT_HAVE_3DNOW
        if (features & MMX3DNOW) {
            functionForModeAsm = qt_functionForMode_SSE3DNOW;
            functionForModeSolidAsm = qt_functionForModeSolid_SSE3DNOW;
            qDrawHelper[QImage::Format_ARGB32_Premultiplied].blendColor = qt_blend_color_argb_sse3dnow;
        }
#endif // 3DNOW


#ifdef QT_HAVE_SSE2
        if (features & SSE2) {
            extern void QT_FASTCALL comp_func_SourceOver_sse2(uint *destPixels,
                                                              const uint *srcPixels,
                                                              int length,
                                                              uint const_alpha);
            extern void QT_FASTCALL comp_func_solid_SourceOver_sse2(uint *destPixels, int length, uint color, uint const_alpha);
            extern void QT_FASTCALL comp_func_Plus_sse2(uint *dst, const uint *src, int length, uint const_alpha);
            extern void QT_FASTCALL comp_func_Source_sse2(uint *dst, const uint *src, int length, uint const_alpha);

            functionForModeAsm[0] = comp_func_SourceOver_sse2;
            functionForModeAsm[QPainter::CompositionMode_Source] = comp_func_Source_sse2;
            functionForModeAsm[QPainter::CompositionMode_Plus] = comp_func_Plus_sse2;
            functionForModeSolidAsm[0] = comp_func_solid_SourceOver_sse2;
        }
#endif
    }
#elif defined(QT_HAVE_SSE2)
    // this is the special case when SSE2 is usable but MMX/SSE is not usable (e.g.: Windows x64 + visual studio)
    if (features & SSE2) {
        functionForModeAsm = qt_functionForMode_onlySSE2;
        functionForModeSolidAsm = qt_functionForModeSolid_onlySSE2;
    }
#endif

#ifdef QT_HAVE_IWMMXT
    if (features & IWMMXT) {
        functionForModeAsm = qt_functionForMode_IWMMXT;
        functionForModeSolidAsm = qt_functionForModeSolid_IWMMXT;
        qDrawHelper[QImage::Format_ARGB32_Premultiplied].blendColor = qt_blend_color_argb_iwmmxt;
    }
#endif // IWMMXT

#if defined(QT_HAVE_ARM_SIMD)
    qBlendFunctions[QImage::Format_RGB32][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_arm_simd;
    qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_arm_simd;
    qBlendFunctions[QImage::Format_RGB32][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_arm_simd;
    qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_arm_simd;
#elif defined(QT_HAVE_NEON)
    if (features & NEON) {
        qBlendFunctions[QImage::Format_RGB32][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_neon;
        qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_neon;
        qBlendFunctions[QImage::Format_RGB32][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_neon;
        qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_neon;
        qBlendFunctions[QImage::Format_RGB16][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_rgb16_neon;
        qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_RGB16] = qt_blend_rgb16_on_argb32_neon;
        qBlendFunctions[QImage::Format_RGB16][QImage::Format_RGB16] = qt_blend_rgb16_on_rgb16_neon;

        qScaleFunctions[QImage::Format_RGB16][QImage::Format_ARGB32_Premultiplied] = qt_scale_image_argb32_on_rgb16_neon;
        qScaleFunctions[QImage::Format_RGB16][QImage::Format_RGB16] = qt_scale_image_rgb16_on_rgb16_neon;

        qTransformFunctions[QImage::Format_RGB16][QImage::Format_ARGB32_Premultiplied] = qt_transform_image_argb32_on_rgb16_neon;
        qTransformFunctions[QImage::Format_RGB16][QImage::Format_RGB16] = qt_transform_image_rgb16_on_rgb16_neon;

        qDrawHelper[QImage::Format_RGB16].alphamapBlit = qt_alphamapblit_quint16_neon;

        functionForMode_C[QPainter::CompositionMode_SourceOver] = qt_blend_argb32_on_argb32_scanline_neon;
        functionForModeSolid_C[QPainter::CompositionMode_SourceOver] = comp_func_solid_SourceOver_neon;
        functionForMode_C[QPainter::CompositionMode_Plus] = comp_func_Plus_neon;
        destFetchProc[QImage::Format_RGB16] = qt_destFetchRGB16_neon;
        destStoreProc[QImage::Format_RGB16] = qt_destStoreRGB16_neon;

        qMemRotateFunctions[QImage::Format_RGB16][0] = qt_memrotate90_16_neon;
        qMemRotateFunctions[QImage::Format_RGB16][2] = qt_memrotate270_16_neon;
        qt_memfill32 = qt_memfill32_neon;

        extern const uint * QT_FASTCALL qt_fetch_radial_gradient_neon(uint *buffer, const Operator *op, const QSpanData *data,
                                                                      int y, int x, int length);

        qt_fetch_radial_gradient = qt_fetch_radial_gradient_neon;
    }
#endif

    if (functionForModeSolidAsm) {
        const int destinationMode = QPainter::CompositionMode_Destination;
        functionForModeSolidAsm[destinationMode] = functionForModeSolid_C[destinationMode];

        // use the default qdrawhelper implementation for the
        // extended composition modes
        for (int mode = 12; mode < 24; ++mode)
            functionForModeSolidAsm[mode] = functionForModeSolid_C[mode];

        functionForModeSolid = functionForModeSolidAsm;
    }
    if (functionForModeAsm)
        functionForMode = functionForModeAsm;

    qt_build_pow_tables();
}

static void qt_memfill32_setup(quint32 *dest, quint32 value, int count)
{
    qInitDrawhelperAsm();
    qt_memfill32(dest, value, count);
}

static void qt_memfill16_setup(quint16 *dest, quint16 value, int count)
{
    qInitDrawhelperAsm();
    qt_memfill16(dest, value, count);
}

#ifdef QT_QWS_DEPTH_GENERIC

int qrgb::bpp = 0;
int qrgb::len_red = 0;
int qrgb::len_green = 0;
int qrgb::len_blue = 0;
int qrgb::len_alpha = 0;
int qrgb::off_red = 0;
int qrgb::off_green = 0;
int qrgb::off_blue = 0;
int qrgb::off_alpha = 0;

template <typename SRC>
Q_STATIC_TEMPLATE_FUNCTION inline void qt_rectconvert_rgb(qrgb *dest, const SRC *src,
                                      int x, int y, int width, int height,
                                      int dstStride, int srcStride)
{
    quint8 *dest8 = reinterpret_cast<quint8*>(dest)
                    + y * dstStride + x * qrgb::bpp;

    srcStride = srcStride / sizeof(SRC) - width;
    dstStride -= (width * qrgb::bpp);

    for (int j = 0;  j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            const quint32 v = qt_convertToRgb<SRC>(*src++);
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
            for (int j = qrgb::bpp - 1; j >= 0; --j)
                *dest8++ = (v >> (8 * j)) & 0xff;
#else
            for (int j = 0; j < qrgb::bpp; ++j)
                *dest8++ = (v >> (8 * j)) & 0xff;
#endif
        }

        dest8 += dstStride;
        src += srcStride;
    }
}

template <>
void qt_rectconvert(qrgb *dest, const quint32 *src,
                    int x, int y, int width, int height,
                    int dstStride, int srcStride)
{
    qt_rectconvert_rgb<quint32>(dest, src, x, y, width, height,
                                dstStride, srcStride);
}

template <>
void qt_rectconvert(qrgb *dest, const quint16 *src,
                    int x, int y, int width, int height,
                    int dstStride, int srcStride)
{
    qt_rectconvert_rgb<quint16>(dest, src, x, y, width, height,
                                dstStride, srcStride);
}

#endif // QT_QWS_DEPTH_GENERIC

QT_END_NAMESPACE
