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

#include <qmath.h>
#include "qblendfunctions_p.h"

QT_BEGIN_NAMESPACE

struct SourceOnlyAlpha
{
    inline uchar alpha(uchar src) const { return src; }
    inline quint16 bytemul(quint16 spix) const { return spix; }
};


struct SourceAndConstAlpha
{
    SourceAndConstAlpha(int a) : m_alpha256(a) {
        m_alpha255 = (m_alpha256 * 255) >> 8;
    };
    inline uchar alpha(uchar src) const { return (src * m_alpha256) >> 8; }
    inline quint16 bytemul(quint16 x) const {
        uint t = (((x & 0x07e0)*m_alpha255) >> 8) & 0x07e0;
        t |= (((x & 0xf81f)*(m_alpha255>>2)) >> 6) & 0xf81f;
        return t;
    }
    int m_alpha255;
    int m_alpha256;
};


/************************************************************************
                       RGB16 (565) format target format
 ************************************************************************/

struct Blend_RGB16_on_RGB16_NoAlpha {
    inline void write(quint16 *dst, quint16 src) { *dst = src; }

    inline void flush(void *) {}
};

struct Blend_RGB16_on_RGB16_ConstAlpha {
    inline Blend_RGB16_on_RGB16_ConstAlpha(quint32 alpha) {
        m_alpha = (alpha * 255) >> 8;
        m_ialpha = 255 - m_alpha;
    }

    inline void write(quint16 *dst, quint16 src) {
        *dst = BYTE_MUL_RGB16(src, m_alpha) + BYTE_MUL_RGB16(*dst, m_ialpha);
    }

    inline void flush(void *) {}

    quint32 m_alpha;
    quint32 m_ialpha;
};

struct Blend_ARGB32_on_RGB16_SourceAlpha {
    inline void write(quint16 *dst, quint32 src) {
        const quint8 alpha = qAlpha(src);
        if (alpha) {
            quint16 s = qConvertRgb32To16(src);
            if(alpha < 255)
                s += BYTE_MUL_RGB16(*dst, 255 - alpha);
            *dst = s;
        }
    }

    inline void flush(void *) {}
};

struct Blend_ARGB32_on_RGB16_SourceAndConstAlpha {
    inline Blend_ARGB32_on_RGB16_SourceAndConstAlpha(quint32 alpha) {
        m_alpha = (alpha * 255) >> 8;
    }

    inline void write(quint16 *dst, quint32 src) {
        src = BYTE_MUL(src, m_alpha);
        const quint8 alpha = qAlpha(src);
        if(alpha) {
            quint16 s = qConvertRgb32To16(src);
            if(alpha < 255)
                s += BYTE_MUL_RGB16(*dst, 255 - alpha);
            *dst = s;
        }
    }

    inline void flush(void *) {}

    quint32 m_alpha;
};

void qt_scale_image_rgb16_on_rgb16(uchar *destPixels, int dbpl,
                                   const uchar *srcPixels, int sbpl, int srch,
                                   const QRectF &targetRect,
                                   const QRectF &sourceRect,
                                   const QRect &clip,
                                   int const_alpha)
{
#ifdef QT_DEBUG_DRAW
    printf("qt_scale_rgb16_on_rgb16: dst=(%p, %d), src=(%p, %d), target=(%d, %d), [%d x %d], src=(%d, %d) [%d x %d] alpha=%d\n",
           destPixels, dbpl, srcPixels, sbpl,
           targetRect.x(), targetRect.y(), targetRect.width(), targetRect.height(),
           sourceRect.x(), sourceRect.y(), sourceRect.width(), sourceRect.height(),
           const_alpha);
#endif
    if (const_alpha == 256) {
        Blend_RGB16_on_RGB16_NoAlpha noAlpha;
        qt_scale_image_16bit<quint16>(destPixels, dbpl, srcPixels, sbpl, srch,
                                      targetRect, sourceRect, clip, noAlpha);
    } else {
        Blend_RGB16_on_RGB16_ConstAlpha constAlpha(const_alpha);
        qt_scale_image_16bit<quint16>(destPixels, dbpl, srcPixels, sbpl, srch,
                                     targetRect, sourceRect, clip, constAlpha);
    }
}

void qt_scale_image_argb32_on_rgb16(uchar *destPixels, int dbpl,
                                    const uchar *srcPixels, int sbpl, int srch,
                                    const QRectF &targetRect,
                                    const QRectF &sourceRect,
                                    const QRect &clip,
                                    int const_alpha)
{
#ifdef QT_DEBUG_DRAW
    printf("qt_scale_argb32_on_rgb16: dst=(%p, %d), src=(%p, %d), target=(%d, %d), [%d x %d], src=(%d, %d) [%d x %d] alpha=%d\n",
           destPixels, dbpl, srcPixels, sbpl,
           targetRect.x(), targetRect.y(), targetRect.width(), targetRect.height(),
           sourceRect.x(), sourceRect.y(), sourceRect.width(), sourceRect.height(),
           const_alpha);
#endif
    if (const_alpha == 256) {
        Blend_ARGB32_on_RGB16_SourceAlpha noAlpha;
        qt_scale_image_16bit<quint32>(destPixels, dbpl, srcPixels, sbpl, srch,
                                      targetRect, sourceRect, clip, noAlpha);
    } else {
        Blend_ARGB32_on_RGB16_SourceAndConstAlpha constAlpha(const_alpha);
        qt_scale_image_16bit<quint32>(destPixels, dbpl, srcPixels, sbpl, srch,
                                     targetRect, sourceRect, clip, constAlpha);
    }
}

void qt_blend_rgb16_on_rgb16(uchar *dst, int dbpl,
                             const uchar *src, int sbpl,
                             int w, int h,
                             int const_alpha)
{
#ifdef QT_DEBUG_DRAW
    printf("qt_blend_rgb16_on_rgb16: dst=(%p, %d), src=(%p, %d), dim=(%d, %d) alpha=%d\n",
           dst, dbpl, src, sbpl, w, h, const_alpha);
#endif

    if (const_alpha == 256) {
        if (w <= 64) {
            while (h--) {
                QT_MEMCPY_USHORT(dst, src, w);
                dst += dbpl;
                src += sbpl;
            }
        } else {
            int length = w << 1;
            while (h--) {
                memcpy(dst, src, length);
                dst += dbpl;
                src += sbpl;
            }
        }
    } else if (const_alpha != 0) {
        quint16 *d = (quint16 *) dst;
        const quint16 *s = (const quint16 *) src;
        quint8 a = (255 * const_alpha) >> 8;
        quint8 ia = 255 - a;
        while (h--) {
            for (int x=0; x<w; ++x) {
                d[x] = BYTE_MUL_RGB16(s[x], a) + BYTE_MUL_RGB16(d[x], ia);
            }
            d = (quint16 *)(((uchar *) d) + dbpl);
            s = (const quint16 *)(((const uchar *) s) + sbpl);
        }
    }
}


void qt_blend_argb32_on_rgb16_const_alpha(uchar *destPixels, int dbpl,
                                          const uchar *srcPixels, int sbpl,
                                          int w, int h,
                                          int const_alpha)
{
    quint16 *dst = (quint16 *) destPixels;
    const quint32 *src = (const quint32 *) srcPixels;

    const_alpha = (const_alpha * 255) >> 8;
    for (int y=0; y<h; ++y) {
        for (int i = 0; i < w; ++i) {
            uint s = src[i];
            s = BYTE_MUL(s, const_alpha);
            int alpha = qAlpha(s);
            s = qConvertRgb32To16(s);
            s += BYTE_MUL_RGB16(dst[i], 255 - alpha);
            dst[i] = s;
        }
        dst = (quint16 *)(((uchar *) dst) + dbpl);
        src = (const quint32 *)(((const uchar *) src) + sbpl);
    }
}

static void qt_blend_argb32_on_rgb16(uchar *destPixels, int dbpl,
                                     const uchar *srcPixels, int sbpl,
                                     int w, int h,
                                     int const_alpha)
{
    if (const_alpha != 256) {
        qt_blend_argb32_on_rgb16_const_alpha(destPixels, dbpl, srcPixels, sbpl, w, h, const_alpha);
        return;
    }

    quint16 *dst = (quint16 *) destPixels;
    quint32 *src = (quint32 *) srcPixels;

    for (int y=0; y<h; ++y) {
        for (int x=0; x<w; ++x) {

            quint32 spix = src[x];
            quint32 alpha = spix >> 24;

            if (alpha == 255) {
                dst[x] = qConvertRgb32To16(spix);
            } else if (alpha != 0) {
                quint32 dpix = dst[x];

                quint32 sia = 255 - alpha;

                quint32 sr = (spix >> 8) & 0xf800;
                quint32 sg = (spix >> 5) & 0x07e0;
                quint32 sb = (spix >> 3) & 0x001f;

                quint32 dr = (dpix & 0x0000f800);
                quint32 dg = (dpix & 0x000007e0);
                quint32 db = (dpix & 0x0000001f);

                quint32 siar = dr * sia;
                quint32 siag = dg * sia;
                quint32 siab = db * sia;

                quint32 rr = sr + ((siar + (siar>>8) + (0x80 << 8)) >> 8);
                quint32 rg = sg + ((siag + (siag>>8) + (0x80 << 3)) >> 8);
                quint32 rb = sb + ((siab + (siab>>8) + (0x80 >> 3)) >> 8);

                dst[x] = (rr & 0xf800)
                         | (rg & 0x07e0)
                         | (rb);
            }
        }
        dst = (quint16 *) (((uchar *) dst) + dbpl);
        src = (quint32 *) (((uchar *) src) + sbpl);
    }
}


static void qt_blend_rgb32_on_rgb16(uchar *destPixels, int dbpl,
                                    const uchar *srcPixels, int sbpl,
                                    int w, int h,
                                    int const_alpha)
{
#ifdef QT_DEBUG_DRAW
    printf("qt_blend_rgb32_on_rgb16: dst=(%p, %d), src=(%p, %d), dim=(%d, %d) alpha=%d\n",
           destPixels, dbpl, srcPixels, sbpl, w, h, const_alpha);
#endif

    if (const_alpha != 256) {
        qt_blend_argb32_on_rgb16(destPixels, dbpl, srcPixels, sbpl, w, h, const_alpha);
        return;
    }

    const quint32 *src = (const quint32 *) srcPixels;
    int srcExtraStride = (sbpl >> 2) - w;

    int dstJPL = dbpl / 2;

    quint16 *dst = (quint16 *) destPixels;
    quint16 *dstEnd = dst + dstJPL * h;

    int dstExtraStride = dstJPL - w;

    while (dst < dstEnd) {
        const quint32 *srcEnd = src + w;
        while (src < srcEnd) {
            *dst = qConvertRgb32To16(*src);
            ++dst;
            ++src;
        }
        dst += dstExtraStride;
        src += srcExtraStride;
    }
}



/************************************************************************
                       RGB32 (-888) format target format
 ************************************************************************/

static void qt_blend_argb32_on_argb32(uchar *destPixels, int dbpl,
                                      const uchar *srcPixels, int sbpl,
                                      int w, int h,
                                      int const_alpha)
{
#ifdef QT_DEBUG_DRAW
    fprintf(stdout, "qt_blend_argb32_on_argb32: dst=(%p, %d), src=(%p, %d), dim=(%d, %d) alpha=%d\n",
            destPixels, dbpl, srcPixels, sbpl, w, h, const_alpha);
    fflush(stdout);
#endif

    const uint *src = (const uint *) srcPixels;
    uint *dst = (uint *) destPixels;
    if (const_alpha == 256) {
        for (int y=0; y<h; ++y) {
            for (int x=0; x<w; ++x) {
                uint s = src[x];
                if (s >= 0xff000000)
                    dst[x] = s;
                else if (s != 0)
                    dst[x] = s + BYTE_MUL(dst[x], qAlpha(~s));
            }
            dst = (quint32 *)(((uchar *) dst) + dbpl);
            src = (const quint32 *)(((const uchar *) src) + sbpl);
        }
    } else if (const_alpha != 0) {
        const_alpha = (const_alpha * 255) >> 8;
        for (int y=0; y<h; ++y) {
            for (int x=0; x<w; ++x) {
                uint s = BYTE_MUL(src[x], const_alpha);
                dst[x] = s + BYTE_MUL(dst[x], qAlpha(~s));
            }
            dst = (quint32 *)(((uchar *) dst) + dbpl);
            src = (const quint32 *)(((const uchar *) src) + sbpl);
        }
    }
}


void qt_blend_rgb32_on_rgb32(uchar *destPixels, int dbpl,
                             const uchar *srcPixels, int sbpl,
                             int w, int h,
                             int const_alpha)
{
#ifdef QT_DEBUG_DRAW
    fprintf(stdout, "qt_blend_rgb32_on_rgb32: dst=(%p, %d), src=(%p, %d), dim=(%d, %d) alpha=%d\n",
            destPixels, dbpl, srcPixels, sbpl, w, h, const_alpha);
    fflush(stdout);
#endif

    if (const_alpha != 256) {
        qt_blend_argb32_on_argb32(destPixels, dbpl, srcPixels, sbpl, w, h, const_alpha);
        return;
    }

    const uint *src = (const uint *) srcPixels;
    uint *dst = (uint *) destPixels;
    int len = w * 4;
    for (int y=0; y<h; ++y) {
        memcpy(dst, src, len);
        dst = (quint32 *)(((uchar *) dst) + dbpl);
        src = (const quint32 *)(((const uchar *) src) + sbpl);
    }
}



struct Blend_RGB32_on_RGB32_NoAlpha {
    inline void write(quint32 *dst, quint32 src) { *dst = src; }

    inline void flush(void *) {}
};

struct Blend_RGB32_on_RGB32_ConstAlpha {
    inline Blend_RGB32_on_RGB32_ConstAlpha(quint32 alpha) {
        m_alpha = (alpha * 255) >> 8;
        m_ialpha = 255 - m_alpha;
    }

    inline void write(quint32 *dst, quint32 src) {
        *dst = BYTE_MUL(src, m_alpha) + BYTE_MUL(*dst, m_ialpha);
    }

    inline void flush(void *) {}

    quint32 m_alpha;
    quint32 m_ialpha;
};

struct Blend_ARGB32_on_ARGB32_SourceAlpha {
    inline void write(quint32 *dst, quint32 src) {
        *dst = src + BYTE_MUL(*dst, qAlpha(~src));
    }

    inline void flush(void *) {}
};

struct Blend_ARGB32_on_ARGB32_SourceAndConstAlpha {
    inline Blend_ARGB32_on_ARGB32_SourceAndConstAlpha(quint32 alpha) {
        m_alpha = (alpha * 255) >> 8;
        m_ialpha = 255 - m_alpha;
    }

    inline void write(quint32 *dst, quint32 src) {
        src = BYTE_MUL(src, m_alpha);
        *dst = src + BYTE_MUL(*dst, qAlpha(~src));
    }

    inline void flush(void *) {}

    quint32 m_alpha;
    quint32 m_ialpha;
};

void qt_scale_image_rgb32_on_rgb32(uchar *destPixels, int dbpl,
                                   const uchar *srcPixels, int sbpl, int srch,
                                   const QRectF &targetRect,
                                   const QRectF &sourceRect,
                                   const QRect &clip,
                                   int const_alpha)
{
#ifdef QT_DEBUG_DRAW
    printf("qt_scale_rgb32_on_rgb32: dst=(%p, %d), src=(%p, %d), target=(%d, %d), [%d x %d], src=(%d, %d) [%d x %d] alpha=%d\n",
           destPixels, dbpl, srcPixels, sbpl,
           targetRect.x(), targetRect.y(), targetRect.width(), targetRect.height(),
           sourceRect.x(), sourceRect.y(), sourceRect.width(), sourceRect.height(),
           const_alpha);
#endif
    if (const_alpha == 256) {
        Blend_RGB32_on_RGB32_NoAlpha noAlpha;
        qt_scale_image_32bit(destPixels, dbpl, srcPixels, sbpl, srch,
                             targetRect, sourceRect, clip, noAlpha);
    } else {
        Blend_RGB32_on_RGB32_ConstAlpha constAlpha(const_alpha);
        qt_scale_image_32bit(destPixels, dbpl, srcPixels, sbpl, srch,
                             targetRect, sourceRect, clip, constAlpha);
    }
}

void qt_scale_image_argb32_on_argb32(uchar *destPixels, int dbpl,
                                     const uchar *srcPixels, int sbpl, int srch,
                                     const QRectF &targetRect,
                                     const QRectF &sourceRect,
                                     const QRect &clip,
                                     int const_alpha)
{
#ifdef QT_DEBUG_DRAW
    printf("qt_scale_argb32_on_argb32: dst=(%p, %d), src=(%p, %d), target=(%d, %d), [%d x %d], src=(%d, %d) [%d x %d] alpha=%d\n",
           destPixels, dbpl, srcPixels, sbpl,
           targetRect.x(), targetRect.y(), targetRect.width(), targetRect.height(),
           sourceRect.x(), sourceRect.y(), sourceRect.width(), sourceRect.height(),
           const_alpha);
#endif
    if (const_alpha == 256) {
        Blend_ARGB32_on_ARGB32_SourceAlpha sourceAlpha;
        qt_scale_image_32bit(destPixels, dbpl, srcPixels, sbpl, srch,
                             targetRect, sourceRect, clip, sourceAlpha);
    } else {
        Blend_ARGB32_on_ARGB32_SourceAndConstAlpha constAlpha(const_alpha);
        qt_scale_image_32bit(destPixels, dbpl, srcPixels, sbpl, srch,
                             targetRect, sourceRect, clip, constAlpha);
    }
}

void qt_transform_image_rgb16_on_rgb16(uchar *destPixels, int dbpl,
                                       const uchar *srcPixels, int sbpl,
                                       const QRectF &targetRect,
                                       const QRectF &sourceRect,
                                       const QRect &clip,
                                       const QTransform &targetRectTransform,
                                       int const_alpha)
{
    if (const_alpha == 256) {
        Blend_RGB16_on_RGB16_NoAlpha noAlpha;
        qt_transform_image(reinterpret_cast<quint16 *>(destPixels), dbpl,
                           reinterpret_cast<const quint16 *>(srcPixels), sbpl,
                           targetRect, sourceRect, clip, targetRectTransform, noAlpha);
    } else {
        Blend_RGB16_on_RGB16_ConstAlpha constAlpha(const_alpha);
        qt_transform_image(reinterpret_cast<quint16 *>(destPixels), dbpl,
                           reinterpret_cast<const quint16 *>(srcPixels), sbpl,
                           targetRect, sourceRect, clip, targetRectTransform, constAlpha);
    }
}

void qt_transform_image_argb32_on_rgb16(uchar *destPixels, int dbpl,
                                        const uchar *srcPixels, int sbpl,
                                        const QRectF &targetRect,
                                        const QRectF &sourceRect,
                                        const QRect &clip,
                                        const QTransform &targetRectTransform,
                                        int const_alpha)
{
    if (const_alpha == 256) {
        Blend_ARGB32_on_RGB16_SourceAlpha noAlpha;
        qt_transform_image(reinterpret_cast<quint16 *>(destPixels), dbpl,
                           reinterpret_cast<const quint32 *>(srcPixels), sbpl,
                           targetRect, sourceRect, clip, targetRectTransform, noAlpha);
    } else {
        Blend_ARGB32_on_RGB16_SourceAndConstAlpha constAlpha(const_alpha);
        qt_transform_image(reinterpret_cast<quint16 *>(destPixels), dbpl,
                           reinterpret_cast<const quint32 *>(srcPixels), sbpl,
                           targetRect, sourceRect, clip, targetRectTransform, constAlpha);
    }
}


void qt_transform_image_rgb32_on_rgb32(uchar *destPixels, int dbpl,
                                       const uchar *srcPixels, int sbpl,
                                       const QRectF &targetRect,
                                       const QRectF &sourceRect,
                                       const QRect &clip,
                                       const QTransform &targetRectTransform,
                                       int const_alpha)
{
    if (const_alpha == 256) {
        Blend_RGB32_on_RGB32_NoAlpha noAlpha;
        qt_transform_image(reinterpret_cast<quint32 *>(destPixels), dbpl,
                           reinterpret_cast<const quint32 *>(srcPixels), sbpl,
                           targetRect, sourceRect, clip, targetRectTransform, noAlpha);
    } else {
        Blend_RGB32_on_RGB32_ConstAlpha constAlpha(const_alpha);
        qt_transform_image(reinterpret_cast<quint32 *>(destPixels), dbpl,
                           reinterpret_cast<const quint32 *>(srcPixels), sbpl,
                           targetRect, sourceRect, clip, targetRectTransform, constAlpha);
    }
}

void qt_transform_image_argb32_on_argb32(uchar *destPixels, int dbpl,
                                         const uchar *srcPixels, int sbpl,
                                         const QRectF &targetRect,
                                         const QRectF &sourceRect,
                                         const QRect &clip,
                                         const QTransform &targetRectTransform,
                                         int const_alpha)
{
    if (const_alpha == 256) {
        Blend_ARGB32_on_ARGB32_SourceAlpha sourceAlpha;
        qt_transform_image(reinterpret_cast<quint32 *>(destPixels), dbpl,
                           reinterpret_cast<const quint32 *>(srcPixels), sbpl,
                           targetRect, sourceRect, clip, targetRectTransform, sourceAlpha);
    } else {
        Blend_ARGB32_on_ARGB32_SourceAndConstAlpha constAlpha(const_alpha);
        qt_transform_image(reinterpret_cast<quint32 *>(destPixels), dbpl,
                           reinterpret_cast<const quint32 *>(srcPixels), sbpl,
                           targetRect, sourceRect, clip, targetRectTransform, constAlpha);
    }
}

SrcOverScaleFunc qScaleFunctions[QImage::NImageFormats][QImage::NImageFormats] = {
    {   // Format_Invalid
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_Mono
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_MonoLSB
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_Indexed8
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGB32
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        qt_scale_image_rgb32_on_rgb32,      // Format_RGB32,
        0,      // Format_ARGB32,
        qt_scale_image_argb32_on_argb32,    // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_ARGB32
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_ARGB32_Premultiplied
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        qt_scale_image_rgb32_on_rgb32,          // Format_RGB32,
        0,      // Format_ARGB32,
        qt_scale_image_argb32_on_argb32,        // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGB16
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        qt_scale_image_argb32_on_rgb16,       // Format_ARGB32_Premultiplied,
        qt_scale_image_rgb16_on_rgb16,        // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_ARGB8565_Premultiplied
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGB666
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_ARGB6666_Premultiplied
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGB555
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_ARGB8555_Premultiplied
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGB888
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGB444
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_ARGB4444_Premultiplied
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGBX8888
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        qt_scale_image_rgb32_on_rgb32,          // Format_RGBX8888,
        0,      // Format_RGBA8888,
        qt_scale_image_argb32_on_argb32,        // Format_RGBA8888_Premultiplied,
#else
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
#endif
    },
    {   // Format_RGBA8888
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGBA8888_Premultiplied
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        qt_scale_image_rgb32_on_rgb32,          // Format_RGBX8888,
        0,      // Format_RGBA8888,
        qt_scale_image_argb32_on_argb32,        // Format_RGBA8888_Premultiplied,
#else
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
#endif
    }
};


SrcOverBlendFunc qBlendFunctions[QImage::NImageFormats][QImage::NImageFormats] = {
    {   // Format_Invalid
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_Mono
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_MonoLSB
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_Indexed8
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGB32
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        qt_blend_rgb32_on_rgb32,        // Format_RGB32,
        0,      // Format_ARGB32,
        qt_blend_argb32_on_argb32,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_ARGB32
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_ARGB32_Premultiplied
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        qt_blend_rgb32_on_rgb32,        // Format_RGB32,
        0,      // Format_ARGB32,
        qt_blend_argb32_on_argb32,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGB16
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        qt_blend_rgb32_on_rgb16,  // Format_RGB32,
        0,      // Format_ARGB32,
        qt_blend_argb32_on_rgb16, // Format_ARGB32_Premultiplied,
        qt_blend_rgb16_on_rgb16,  // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_ARGB8565_Premultiplied
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGB666
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_ARGB6666_Premultiplied
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGB555
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_ARGB8555_Premultiplied
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGB888
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGB444
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_ARGB4444_Premultiplied
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGBX8888
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        qt_blend_rgb32_on_rgb32,        // Format_RGBX8888,
        0,      // Format_RGBA8888,
        qt_blend_argb32_on_argb32,      // Format_RGBA8888_Premultiplied,
#else
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
#endif
    },
    {   // Format_RGBA8888
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGBA8888_Premultiplied
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        qt_blend_rgb32_on_rgb32,        // Format_RGBX8888,
        0,      // Format_RGBA8888,
        qt_blend_argb32_on_argb32,      // Format_RGBA8888_Premultiplied,
#else
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
#endif
    }
};

SrcOverTransformFunc qTransformFunctions[QImage::NImageFormats][QImage::NImageFormats] = {
    {   // Format_Invalid
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_Mono
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_MonoLSB
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_Indexed8
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGB32
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        qt_transform_image_rgb32_on_rgb32,      // Format_RGB32,
        0,      // Format_ARGB32,
        qt_transform_image_argb32_on_argb32,    // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_ARGB32
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_ARGB32_Premultiplied
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        qt_transform_image_rgb32_on_rgb32,          // Format_RGB32,
        0,      // Format_ARGB32,
        qt_transform_image_argb32_on_argb32,        // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGB16
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        qt_transform_image_argb32_on_rgb16,       // Format_ARGB32_Premultiplied,
        qt_transform_image_rgb16_on_rgb16,        // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_ARGB8565_Premultiplied
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGB666
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_ARGB6666_Premultiplied
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGB555
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_ARGB8555_Premultiplied
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGB888
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGB444
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_ARGB4444_Premultiplied
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGBX8888
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        qt_transform_image_rgb32_on_rgb32,        // Format_RGBX8888,
        0,      // Format_RGBA8888,
        qt_transform_image_argb32_on_argb32,      // Format_RGBA8888_Premultiplied,
#else
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
#endif
    },
    {   // Format_RGBA8888
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
    },
    {   // Format_RGBA8888_Premultiplied
        0,      // Format_Invalid,
        0,      // Format_Mono,
        0,      // Format_MonoLSB,
        0,      // Format_Indexed8,
        0,      // Format_RGB32,
        0,      // Format_ARGB32,
        0,      // Format_ARGB32_Premultiplied,
        0,      // Format_RGB16,
        0,      // Format_ARGB8565_Premultiplied,
        0,      // Format_RGB666,
        0,      // Format_ARGB6666_Premultiplied,
        0,      // Format_RGB555,
        0,      // Format_ARGB8555_Premultiplied,
        0,      // Format_RGB888,
        0,      // Format_RGB444,
        0,      // Format_ARGB4444_Premultiplied,
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        qt_transform_image_rgb32_on_rgb32,        // Format_RGBX8888,
        0,      // Format_RGBA8888,
        qt_transform_image_argb32_on_argb32,      // Format_RGBA8888_Premultiplied,
#else
        0,      // Format_RGBX8888,
        0,      // Format_RGBA8888,
        0       // Format_RGBA8888_Premultiplied,
#endif
    }
};

QT_END_NAMESPACE
