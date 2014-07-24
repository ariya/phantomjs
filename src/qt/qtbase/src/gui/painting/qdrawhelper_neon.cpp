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
#include <private/qblendfunctions_p.h>
#include <private/qmath_p.h>

#ifdef __ARM_NEON__

#include <private/qdrawhelper_neon_p.h>
#include <private/qpaintengine_raster_p.h>
#include <arm_neon.h>

QT_BEGIN_NAMESPACE

void qt_memfill32(quint32 *dest, quint32 value, int count)
{
    const int epilogueSize = count % 16;
    if (count >= 16) {
        quint32 *const neonEnd = dest + count - epilogueSize;
        register uint32x4_t valueVector1 asm ("q0") = vdupq_n_u32(value);
        register uint32x4_t valueVector2 asm ("q1") = valueVector1;
        while (dest != neonEnd) {
            asm volatile (
                "vst2.32     { d0, d1, d2, d3 }, [%[DST]] !\n\t"
                "vst2.32     { d0, d1, d2, d3 }, [%[DST]] !\n\t"
                : [DST]"+r" (dest)
                : [VALUE1]"w"(valueVector1), [VALUE2]"w"(valueVector2)
                : "memory"
            );
        }
    }

    switch (epilogueSize)
    {
    case 15:     *dest++ = value;
    case 14:     *dest++ = value;
    case 13:     *dest++ = value;
    case 12:     *dest++ = value;
    case 11:     *dest++ = value;
    case 10:     *dest++ = value;
    case 9:      *dest++ = value;
    case 8:      *dest++ = value;
    case 7:      *dest++ = value;
    case 6:      *dest++ = value;
    case 5:      *dest++ = value;
    case 4:      *dest++ = value;
    case 3:      *dest++ = value;
    case 2:      *dest++ = value;
    case 1:      *dest++ = value;
    }
}

static inline uint16x8_t qvdiv_255_u16(uint16x8_t x, uint16x8_t half)
{
    // result = (x + (x >> 8) + 0x80) >> 8

    const uint16x8_t temp = vshrq_n_u16(x, 8); // x >> 8
    const uint16x8_t sum_part = vaddq_u16(x, half); // x + 0x80
    const uint16x8_t sum = vaddq_u16(temp, sum_part);

    return vshrq_n_u16(sum, 8);
}

static inline uint16x8_t qvbyte_mul_u16(uint16x8_t x, uint16x8_t alpha, uint16x8_t half)
{
    // t = qRound(x * alpha / 255.0)

    const uint16x8_t t = vmulq_u16(x, alpha); // t
    return qvdiv_255_u16(t, half);
}

static inline uint16x8_t qvinterpolate_pixel_255(uint16x8_t x, uint16x8_t a, uint16x8_t y, uint16x8_t b, uint16x8_t half)
{
    // t = x * a + y * b

    const uint16x8_t ta = vmulq_u16(x, a);
    const uint16x8_t tb = vmulq_u16(y, b);

    return qvdiv_255_u16(vaddq_u16(ta, tb), half);
}

static inline uint16x8_t qvsource_over_u16(uint16x8_t src16, uint16x8_t dst16, uint16x8_t half, uint16x8_t full)
{
    const uint16x4_t alpha16_high = vdup_lane_u16(vget_high_u16(src16), 3);
    const uint16x4_t alpha16_low = vdup_lane_u16(vget_low_u16(src16), 3);

    const uint16x8_t alpha16 = vsubq_u16(full, vcombine_u16(alpha16_low, alpha16_high));

    return vaddq_u16(src16, qvbyte_mul_u16(dst16, alpha16, half));
}

extern "C" void
pixman_composite_over_8888_0565_asm_neon (int32_t   w,
                                          int32_t   h,
                                          uint16_t *dst,
                                          int32_t   dst_stride,
                                          uint32_t *src,
                                          int32_t   src_stride);

extern "C" void
pixman_composite_over_8888_8888_asm_neon (int32_t   w,
                                          int32_t   h,
                                          uint32_t *dst,
                                          int32_t   dst_stride,
                                          uint32_t *src,
                                          int32_t   src_stride);

extern "C" void
pixman_composite_src_0565_8888_asm_neon (int32_t   w,
                                         int32_t   h,
                                         uint32_t *dst,
                                         int32_t   dst_stride,
                                         uint16_t *src,
                                         int32_t   src_stride);

extern "C" void
pixman_composite_over_n_8_0565_asm_neon (int32_t    w,
                                         int32_t    h,
                                         uint16_t  *dst,
                                         int32_t    dst_stride,
                                         uint32_t   src,
                                         int32_t    unused,
                                         uint8_t   *mask,
                                         int32_t    mask_stride);

extern "C" void
pixman_composite_scanline_over_asm_neon (int32_t         w,
                                         const uint32_t *dst,
                                         const uint32_t *src);

extern "C" void
pixman_composite_src_0565_0565_asm_neon (int32_t   w,
                                         int32_t   h,
                                         uint16_t *dst,
                                         int32_t   dst_stride,
                                         uint16_t *src,
                                         int32_t   src_stride);

// qblendfunctions.cpp
void qt_blend_argb32_on_rgb16_const_alpha(uchar *destPixels, int dbpl,
                                          const uchar *srcPixels, int sbpl,
                                          int w, int h,
                                          int const_alpha);

void qt_blend_rgb16_on_argb32_neon(uchar *destPixels, int dbpl,
                                   const uchar *srcPixels, int sbpl,
                                   int w, int h,
                                   int const_alpha)
{
    dbpl /= 4;
    sbpl /= 2;

    quint32 *dst = (quint32 *) destPixels;
    quint16 *src = (quint16 *) srcPixels;

    if (const_alpha != 256) {
        quint8 a = (255 * const_alpha) >> 8;
        quint8 ia = 255 - a;

        while (h--) {
            for (int x=0; x<w; ++x)
                dst[x] = INTERPOLATE_PIXEL_255(qConvertRgb16To32(src[x]), a, dst[x], ia);
            dst += dbpl;
            src += sbpl;
        }
        return;
    }

    pixman_composite_src_0565_8888_asm_neon(w, h, dst, dbpl, src, sbpl);
}

// qblendfunctions.cpp
void qt_blend_rgb16_on_rgb16(uchar *dst, int dbpl,
                             const uchar *src, int sbpl,
                             int w, int h,
                             int const_alpha);

template <int N>
static inline void scanLineBlit16(quint16 *dst, quint16 *src, int dstride)
{
    if (N >= 2) {
        ((quint32 *)dst)[0] = ((quint32 *)src)[0];
        __builtin_prefetch(dst + dstride, 1, 0);
    }
    for (int i = 1; i < N/2; ++i)
        ((quint32 *)dst)[i] = ((quint32 *)src)[i];
    if (N & 1)
        dst[N-1] = src[N-1];
}

template <int Width>
static inline void blockBlit16(quint16 *dst, quint16 *src, int dstride, int sstride, int h)
{
    union {
        quintptr address;
        quint16 *pointer;
    } u;

    u.pointer = dst;

    if (u.address & 2) {
        while (h--) {
            // align dst
            dst[0] = src[0];
            if (Width > 1)
                scanLineBlit16<Width-1>(dst + 1, src + 1, dstride);
            dst += dstride;
            src += sstride;
        }
    } else {
        while (h--) {
            scanLineBlit16<Width>(dst, src, dstride);

            dst += dstride;
            src += sstride;
        }
    }
}

void qt_blend_rgb16_on_rgb16_neon(uchar *destPixels, int dbpl,
                                  const uchar *srcPixels, int sbpl,
                                  int w, int h,
                                  int const_alpha)
{
    // testing show that the default memcpy is faster for widths 150 and up
    if (const_alpha != 256 || w >= 150) {
        qt_blend_rgb16_on_rgb16(destPixels, dbpl, srcPixels, sbpl, w, h, const_alpha);
        return;
    }

    int dstride = dbpl / 2;
    int sstride = sbpl / 2;

    quint16 *dst = (quint16 *) destPixels;
    quint16 *src = (quint16 *) srcPixels;

    switch (w) {
#define BLOCKBLIT(n) case n: blockBlit16<n>(dst, src, dstride, sstride, h); return;
    BLOCKBLIT(1);
    BLOCKBLIT(2);
    BLOCKBLIT(3);
    BLOCKBLIT(4);
    BLOCKBLIT(5);
    BLOCKBLIT(6);
    BLOCKBLIT(7);
    BLOCKBLIT(8);
    BLOCKBLIT(9);
    BLOCKBLIT(10);
    BLOCKBLIT(11);
    BLOCKBLIT(12);
    BLOCKBLIT(13);
    BLOCKBLIT(14);
    BLOCKBLIT(15);
#undef BLOCKBLIT
    default:
        break;
    }

    pixman_composite_src_0565_0565_asm_neon (w, h, dst, dstride, src, sstride);
}

extern "C" void blend_8_pixels_argb32_on_rgb16_neon(quint16 *dst, const quint32 *src, int const_alpha);

void qt_blend_argb32_on_rgb16_neon(uchar *destPixels, int dbpl,
                                   const uchar *srcPixels, int sbpl,
                                   int w, int h,
                                   int const_alpha)
{
    quint16 *dst = (quint16 *) destPixels;
    quint32 *src = (quint32 *) srcPixels;

    if (const_alpha != 256) {
        for (int y=0; y<h; ++y) {
            int i = 0;
            for (; i < w-7; i += 8)
                blend_8_pixels_argb32_on_rgb16_neon(&dst[i], &src[i], const_alpha);

            if (i < w) {
                int tail = w - i;

                quint16 dstBuffer[8];
                quint32 srcBuffer[8];

                for (int j = 0; j < tail; ++j) {
                    dstBuffer[j] = dst[i + j];
                    srcBuffer[j] = src[i + j];
                }

                blend_8_pixels_argb32_on_rgb16_neon(dstBuffer, srcBuffer, const_alpha);

                for (int j = 0; j < tail; ++j)
                    dst[i + j] = dstBuffer[j];
            }

            dst = (quint16 *)(((uchar *) dst) + dbpl);
            src = (quint32 *)(((uchar *) src) + sbpl);
        }
        return;
    }

    pixman_composite_over_8888_0565_asm_neon(w, h, dst, dbpl / 2, src, sbpl / 4);
}

void qt_blend_argb32_on_argb32_scanline_neon(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        pixman_composite_scanline_over_asm_neon(length, dest, src);
    } else {
        qt_blend_argb32_on_argb32_neon((uchar *)dest, 4 * length, (uchar *)src, 4 * length, length, 1, (const_alpha * 256) / 255);
    }
}

void qt_blend_argb32_on_argb32_neon(uchar *destPixels, int dbpl,
                                    const uchar *srcPixels, int sbpl,
                                    int w, int h,
                                    int const_alpha)
{
    const uint *src = (const uint *) srcPixels;
    uint *dst = (uint *) destPixels;
    uint16x8_t half = vdupq_n_u16(0x80);
    uint16x8_t full = vdupq_n_u16(0xff);
    if (const_alpha == 256) {
        pixman_composite_over_8888_8888_asm_neon(w, h, (uint32_t *)destPixels, dbpl / 4, (uint32_t *)srcPixels, sbpl / 4);
    } else if (const_alpha != 0) {
        const_alpha = (const_alpha * 255) >> 8;
        uint16x8_t const_alpha16 = vdupq_n_u16(const_alpha);
        for (int y = 0; y < h; ++y) {
            int x = 0;
            for (; x < w-3; x += 4) {
                if (src[x] | src[x+1] | src[x+2] | src[x+3]) {
                    uint32x4_t src32 = vld1q_u32((uint32_t *)&src[x]);
                    uint32x4_t dst32 = vld1q_u32((uint32_t *)&dst[x]);

                    const uint8x16_t src8 = vreinterpretq_u8_u32(src32);
                    const uint8x16_t dst8 = vreinterpretq_u8_u32(dst32);

                    const uint8x8_t src8_low = vget_low_u8(src8);
                    const uint8x8_t dst8_low = vget_low_u8(dst8);

                    const uint8x8_t src8_high = vget_high_u8(src8);
                    const uint8x8_t dst8_high = vget_high_u8(dst8);

                    const uint16x8_t src16_low = vmovl_u8(src8_low);
                    const uint16x8_t dst16_low = vmovl_u8(dst8_low);

                    const uint16x8_t src16_high = vmovl_u8(src8_high);
                    const uint16x8_t dst16_high = vmovl_u8(dst8_high);

                    const uint16x8_t srcalpha16_low = qvbyte_mul_u16(src16_low, const_alpha16, half);
                    const uint16x8_t srcalpha16_high = qvbyte_mul_u16(src16_high, const_alpha16, half);

                    const uint16x8_t result16_low = qvsource_over_u16(srcalpha16_low, dst16_low, half, full);
                    const uint16x8_t result16_high = qvsource_over_u16(srcalpha16_high, dst16_high, half, full);

                    const uint32x2_t result32_low = vreinterpret_u32_u8(vmovn_u16(result16_low));
                    const uint32x2_t result32_high = vreinterpret_u32_u8(vmovn_u16(result16_high));

                    vst1q_u32((uint32_t *)&dst[x], vcombine_u32(result32_low, result32_high));
                }
            }
            for (; x<w; ++x) {
                uint s = src[x];
                if (s != 0) {
                    s = BYTE_MUL(s, const_alpha);
                    dst[x] = s + BYTE_MUL(dst[x], qAlpha(~s));
                }
            }
            dst = (quint32 *)(((uchar *) dst) + dbpl);
            src = (const quint32 *)(((const uchar *) src) + sbpl);
        }
    }
}

// qblendfunctions.cpp
void qt_blend_rgb32_on_rgb32(uchar *destPixels, int dbpl,
                             const uchar *srcPixels, int sbpl,
                             int w, int h,
                             int const_alpha);

void qt_blend_rgb32_on_rgb32_neon(uchar *destPixels, int dbpl,
                                  const uchar *srcPixels, int sbpl,
                                  int w, int h,
                                  int const_alpha)
{
    if (const_alpha != 256) {
        if (const_alpha != 0) {
            const uint *src = (const uint *) srcPixels;
            uint *dst = (uint *) destPixels;
            uint16x8_t half = vdupq_n_u16(0x80);
            const_alpha = (const_alpha * 255) >> 8;
            int one_minus_const_alpha = 255 - const_alpha;
            uint16x8_t const_alpha16 = vdupq_n_u16(const_alpha);
            uint16x8_t one_minus_const_alpha16 = vdupq_n_u16(255 - const_alpha);
            for (int y = 0; y < h; ++y) {
                int x = 0;
                for (; x < w-3; x += 4) {
                    uint32x4_t src32 = vld1q_u32((uint32_t *)&src[x]);
                    uint32x4_t dst32 = vld1q_u32((uint32_t *)&dst[x]);

                    const uint8x16_t src8 = vreinterpretq_u8_u32(src32);
                    const uint8x16_t dst8 = vreinterpretq_u8_u32(dst32);

                    const uint8x8_t src8_low = vget_low_u8(src8);
                    const uint8x8_t dst8_low = vget_low_u8(dst8);

                    const uint8x8_t src8_high = vget_high_u8(src8);
                    const uint8x8_t dst8_high = vget_high_u8(dst8);

                    const uint16x8_t src16_low = vmovl_u8(src8_low);
                    const uint16x8_t dst16_low = vmovl_u8(dst8_low);

                    const uint16x8_t src16_high = vmovl_u8(src8_high);
                    const uint16x8_t dst16_high = vmovl_u8(dst8_high);

                    const uint16x8_t result16_low = qvinterpolate_pixel_255(src16_low, const_alpha16, dst16_low, one_minus_const_alpha16, half);
                    const uint16x8_t result16_high = qvinterpolate_pixel_255(src16_high, const_alpha16, dst16_high, one_minus_const_alpha16, half);

                    const uint32x2_t result32_low = vreinterpret_u32_u8(vmovn_u16(result16_low));
                    const uint32x2_t result32_high = vreinterpret_u32_u8(vmovn_u16(result16_high));

                    vst1q_u32((uint32_t *)&dst[x], vcombine_u32(result32_low, result32_high));
                }
                for (; x<w; ++x) {
                    uint s = src[x];
                    s = BYTE_MUL(s, const_alpha);
                    dst[x] = INTERPOLATE_PIXEL_255(src[x], const_alpha, dst[x], one_minus_const_alpha);
                }
                dst = (quint32 *)(((uchar *) dst) + dbpl);
                src = (const quint32 *)(((const uchar *) src) + sbpl);
            }
        }
    } else {
        qt_blend_rgb32_on_rgb32(destPixels, dbpl, srcPixels, sbpl, w, h, const_alpha);
    }
}

void qt_alphamapblit_quint16_neon(QRasterBuffer *rasterBuffer,
                                  int x, int y, quint32 color,
                                  const uchar *bitmap,
                                  int mapWidth, int mapHeight, int mapStride,
                                  const QClipData *)
{
    quint16 *dest = reinterpret_cast<quint16*>(rasterBuffer->scanLine(y)) + x;
    const int destStride = rasterBuffer->bytesPerLine() / sizeof(quint16);

    uchar *mask = const_cast<uchar *>(bitmap);

    pixman_composite_over_n_8_0565_asm_neon(mapWidth, mapHeight, dest, destStride, color, 0, mask, mapStride);
}

extern "C" void blend_8_pixels_rgb16_on_rgb16_neon(quint16 *dst, const quint16 *src, int const_alpha);

template <typename SRC, typename BlendFunc>
struct Blend_on_RGB16_SourceAndConstAlpha_Neon {
    Blend_on_RGB16_SourceAndConstAlpha_Neon(BlendFunc blender, int const_alpha)
        : m_index(0)
        , m_blender(blender)
        , m_const_alpha(const_alpha)
    {
    }

    inline void write(quint16 *dst, quint32 src)
    {
        srcBuffer[m_index++] = src;

        if (m_index == 8) {
            m_blender(dst - 7, srcBuffer, m_const_alpha);
            m_index = 0;
        }
    }

    inline void flush(quint16 *dst)
    {
        if (m_index > 0) {
            quint16 dstBuffer[8];
            for (int i = 0; i < m_index; ++i)
                dstBuffer[i] = dst[i - m_index];

            m_blender(dstBuffer, srcBuffer, m_const_alpha);

            for (int i = 0; i < m_index; ++i)
                dst[i - m_index] = dstBuffer[i];

            m_index = 0;
        }
    }

    SRC srcBuffer[8];

    int m_index;
    BlendFunc m_blender;
    int m_const_alpha;
};

template <typename SRC, typename BlendFunc>
Blend_on_RGB16_SourceAndConstAlpha_Neon<SRC, BlendFunc>
Blend_on_RGB16_SourceAndConstAlpha_Neon_create(BlendFunc blender, int const_alpha)
{
    return Blend_on_RGB16_SourceAndConstAlpha_Neon<SRC, BlendFunc>(blender, const_alpha);
}

void qt_scale_image_argb32_on_rgb16_neon(uchar *destPixels, int dbpl,
                                         const uchar *srcPixels, int sbpl, int srch,
                                         const QRectF &targetRect,
                                         const QRectF &sourceRect,
                                         const QRect &clip,
                                         int const_alpha)
{
    if (const_alpha == 0)
        return;

    qt_scale_image_16bit<quint32>(destPixels, dbpl, srcPixels, sbpl, srch, targetRect, sourceRect, clip,
        Blend_on_RGB16_SourceAndConstAlpha_Neon_create<quint32>(blend_8_pixels_argb32_on_rgb16_neon, const_alpha));
}

void qt_scale_image_rgb16_on_rgb16(uchar *destPixels, int dbpl,
                                   const uchar *srcPixels, int sbpl, int srch,
                                   const QRectF &targetRect,
                                   const QRectF &sourceRect,
                                   const QRect &clip,
                                   int const_alpha);

void qt_scale_image_rgb16_on_rgb16_neon(uchar *destPixels, int dbpl,
                                        const uchar *srcPixels, int sbpl, int srch,
                                        const QRectF &targetRect,
                                        const QRectF &sourceRect,
                                        const QRect &clip,
                                        int const_alpha)
{
    if (const_alpha == 0)
        return;

    if (const_alpha == 256) {
        qt_scale_image_rgb16_on_rgb16(destPixels, dbpl, srcPixels, sbpl, srch, targetRect, sourceRect, clip, const_alpha);
        return;
    }

    qt_scale_image_16bit<quint16>(destPixels, dbpl, srcPixels, sbpl, srch, targetRect, sourceRect, clip,
        Blend_on_RGB16_SourceAndConstAlpha_Neon_create<quint16>(blend_8_pixels_rgb16_on_rgb16_neon, const_alpha));
}

extern void qt_transform_image_rgb16_on_rgb16(uchar *destPixels, int dbpl,
                                              const uchar *srcPixels, int sbpl,
                                              const QRectF &targetRect,
                                              const QRectF &sourceRect,
                                              const QRect &clip,
                                              const QTransform &targetRectTransform,
                                              int const_alpha);

void qt_transform_image_rgb16_on_rgb16_neon(uchar *destPixels, int dbpl,
                                            const uchar *srcPixels, int sbpl,
                                            const QRectF &targetRect,
                                            const QRectF &sourceRect,
                                            const QRect &clip,
                                            const QTransform &targetRectTransform,
                                            int const_alpha)
{
    if (const_alpha == 0)
        return;

    if (const_alpha == 256) {
        qt_transform_image_rgb16_on_rgb16(destPixels, dbpl, srcPixels, sbpl, targetRect, sourceRect, clip, targetRectTransform, const_alpha);
        return;
    }

    qt_transform_image(reinterpret_cast<quint16 *>(destPixels), dbpl,
                       reinterpret_cast<const quint16 *>(srcPixels), sbpl, targetRect, sourceRect, clip, targetRectTransform,
        Blend_on_RGB16_SourceAndConstAlpha_Neon_create<quint16>(blend_8_pixels_rgb16_on_rgb16_neon, const_alpha));
}

void qt_transform_image_argb32_on_rgb16_neon(uchar *destPixels, int dbpl,
                                             const uchar *srcPixels, int sbpl,
                                             const QRectF &targetRect,
                                             const QRectF &sourceRect,
                                             const QRect &clip,
                                             const QTransform &targetRectTransform,
                                             int const_alpha)
{
    if (const_alpha == 0)
        return;

    qt_transform_image(reinterpret_cast<quint16 *>(destPixels), dbpl,
                       reinterpret_cast<const quint32 *>(srcPixels), sbpl, targetRect, sourceRect, clip, targetRectTransform,
        Blend_on_RGB16_SourceAndConstAlpha_Neon_create<quint32>(blend_8_pixels_argb32_on_rgb16_neon, const_alpha));
}

static inline void convert_8_pixels_rgb16_to_argb32(quint32 *dst, const quint16 *src)
{
    asm volatile (
        "vld1.16     { d0, d1 }, [%[SRC]]\n\t"

        /* convert 8 r5g6b5 pixel data from {d0, d1} to planar 8-bit format
           and put data into d4 - red, d3 - green, d2 - blue */
        "vshrn.u16   d4,  q0,  #8\n\t"
        "vshrn.u16   d3,  q0,  #3\n\t"
        "vsli.u16    q0,  q0,  #5\n\t"
        "vsri.u8     d4,  d4,  #5\n\t"
        "vsri.u8     d3,  d3,  #6\n\t"
        "vshrn.u16   d2,  q0,  #2\n\t"

        /* fill d5 - alpha with 0xff */
        "mov         r2, #255\n\t"
        "vdup.8      d5, r2\n\t"

        "vst4.8      { d2, d3, d4, d5 }, [%[DST]]"
        : : [DST]"r" (dst), [SRC]"r" (src)
        : "memory", "r2", "d0", "d1", "d2", "d3", "d4", "d5"
    );
}

uint * QT_FASTCALL qt_destFetchRGB16_neon(uint *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length)
{
    const ushort *data = (const ushort *)rasterBuffer->scanLine(y) + x;

    int i = 0;
    for (; i < length - 7; i += 8)
        convert_8_pixels_rgb16_to_argb32(&buffer[i], &data[i]);

    if (i < length) {
        quint16 srcBuffer[8];
        quint32 dstBuffer[8];

        int tail = length - i;
        for (int j = 0; j < tail; ++j)
            srcBuffer[j] = data[i + j];

        convert_8_pixels_rgb16_to_argb32(dstBuffer, srcBuffer);

        for (int j = 0; j < tail; ++j)
            buffer[i + j] = dstBuffer[j];
    }

    return buffer;
}

static inline void convert_8_pixels_argb32_to_rgb16(quint16 *dst, const quint32 *src)
{
    asm volatile (
        "vld4.8      { d0, d1, d2, d3 }, [%[SRC]]\n\t"

        /* convert to r5g6b5 and store it into {d28, d29} */
        "vshll.u8    q14, d2, #8\n\t"
        "vshll.u8    q8,  d1, #8\n\t"
        "vshll.u8    q9,  d0, #8\n\t"
        "vsri.u16    q14, q8, #5\n\t"
        "vsri.u16    q14, q9, #11\n\t"

        "vst1.16     { d28, d29 }, [%[DST]]"
        : : [DST]"r" (dst), [SRC]"r" (src)
        : "memory", "d0", "d1", "d2", "d3", "d16", "d17", "d18", "d19", "d28", "d29"
    );
}

void QT_FASTCALL qt_destStoreRGB16_neon(QRasterBuffer *rasterBuffer, int x, int y, const uint *buffer, int length)
{
    quint16 *data = (quint16*)rasterBuffer->scanLine(y) + x;

    int i = 0;
    for (; i < length - 7; i += 8)
        convert_8_pixels_argb32_to_rgb16(&data[i], &buffer[i]);

    if (i < length) {
        quint32 srcBuffer[8];
        quint16 dstBuffer[8];

        int tail = length - i;
        for (int j = 0; j < tail; ++j)
            srcBuffer[j] = buffer[i + j];

        convert_8_pixels_argb32_to_rgb16(dstBuffer, srcBuffer);

        for (int j = 0; j < tail; ++j)
            data[i + j] = dstBuffer[j];
    }
}

void QT_FASTCALL comp_func_solid_SourceOver_neon(uint *destPixels, int length, uint color, uint const_alpha)
{
    if ((const_alpha & qAlpha(color)) == 255) {
        QT_MEMFILL_UINT(destPixels, length, color);
    } else {
        if (const_alpha != 255)
            color = BYTE_MUL(color, const_alpha);

        const quint32 minusAlphaOfColor = qAlpha(~color);
        int x = 0;

        uint32_t *dst = (uint32_t *) destPixels;
        const uint32x4_t colorVector = vdupq_n_u32(color);
        uint16x8_t half = vdupq_n_u16(0x80);
        const uint16x8_t minusAlphaOfColorVector = vdupq_n_u16(minusAlphaOfColor);

        for (; x < length-3; x += 4) {
            uint32x4_t dstVector = vld1q_u32(&dst[x]);

            const uint8x16_t dst8 = vreinterpretq_u8_u32(dstVector);

            const uint8x8_t dst8_low = vget_low_u8(dst8);
            const uint8x8_t dst8_high = vget_high_u8(dst8);

            const uint16x8_t dst16_low = vmovl_u8(dst8_low);
            const uint16x8_t dst16_high = vmovl_u8(dst8_high);

            const uint16x8_t result16_low = qvbyte_mul_u16(dst16_low, minusAlphaOfColorVector, half);
            const uint16x8_t result16_high = qvbyte_mul_u16(dst16_high, minusAlphaOfColorVector, half);

            const uint32x2_t result32_low = vreinterpret_u32_u8(vmovn_u16(result16_low));
            const uint32x2_t result32_high = vreinterpret_u32_u8(vmovn_u16(result16_high));

            uint32x4_t blendedPixels = vcombine_u32(result32_low, result32_high);
            uint32x4_t colorPlusBlendedPixels = vaddq_u32(colorVector, blendedPixels);
            vst1q_u32(&dst[x], colorPlusBlendedPixels);
        }

        for (;x < length; ++x)
            destPixels[x] = color + BYTE_MUL(destPixels[x], minusAlphaOfColor);
    }
}

void QT_FASTCALL comp_func_Plus_neon(uint *dst, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        uint *const end = dst + length;
        uint *const neonEnd = end - 3;

        while (dst < neonEnd) {
            asm volatile (
                "vld2.8     { d0, d1 }, [%[SRC]] !\n\t"
                "vld2.8     { d2, d3 }, [%[DST]]\n\t"
                "vqadd.u8 q0, q0, q1\n\t"
                "vst2.8     { d0, d1 }, [%[DST]] !\n\t"
                : [DST]"+r" (dst), [SRC]"+r" (src)
                :
                : "memory", "d0", "d1", "d2", "d3", "q0", "q1"
            );
        }

        while (dst != end) {
            *dst = comp_func_Plus_one_pixel(*dst, *src);
            ++dst;
            ++src;
        }
    } else {
        int x = 0;
        const int one_minus_const_alpha = 255 - const_alpha;
        const uint16x8_t constAlphaVector = vdupq_n_u16(const_alpha);
        const uint16x8_t oneMinusconstAlphaVector = vdupq_n_u16(one_minus_const_alpha);

        const uint16x8_t half = vdupq_n_u16(0x80);
        for (; x < length - 3; x += 4) {
            const uint32x4_t src32 = vld1q_u32((uint32_t *)&src[x]);
            const uint8x16_t src8 = vreinterpretq_u8_u32(src32);
            uint8x16_t dst8 = vld1q_u8((uint8_t *)&dst[x]);
            uint8x16_t result = vqaddq_u8(dst8, src8);

            uint16x8_t result_low = vmovl_u8(vget_low_u8(result));
            uint16x8_t result_high = vmovl_u8(vget_high_u8(result));

            uint16x8_t dst_low = vmovl_u8(vget_low_u8(dst8));
            uint16x8_t dst_high = vmovl_u8(vget_high_u8(dst8));

            result_low = qvinterpolate_pixel_255(result_low, constAlphaVector, dst_low, oneMinusconstAlphaVector, half);
            result_high = qvinterpolate_pixel_255(result_high, constAlphaVector, dst_high, oneMinusconstAlphaVector, half);

            const uint32x2_t result32_low = vreinterpret_u32_u8(vmovn_u16(result_low));
            const uint32x2_t result32_high = vreinterpret_u32_u8(vmovn_u16(result_high));
            vst1q_u32((uint32_t *)&dst[x], vcombine_u32(result32_low, result32_high));
        }

        for (; x < length; ++x)
            dst[x] = comp_func_Plus_one_pixel_const_alpha(dst[x], src[x], const_alpha, one_minus_const_alpha);
    }
}

static const int tileSize = 32;

extern "C" void qt_rotate90_16_neon(quint16 *dst, const quint16 *src, int sstride, int dstride, int count);

void qt_memrotate90_16_neon(const uchar *srcPixels, int w, int h, int sstride, uchar *destPixels, int dstride)
{
    const ushort *src = (const ushort *)srcPixels;
    ushort *dest = (ushort *)destPixels;

    sstride /= sizeof(ushort);
    dstride /= sizeof(ushort);

    const int pack = sizeof(quint32) / sizeof(ushort);
    const int unaligned =
        qMin(uint((quintptr(dest) & (sizeof(quint32)-1)) / sizeof(ushort)), uint(h));
    const int restX = w % tileSize;
    const int restY = (h - unaligned) % tileSize;
    const int unoptimizedY = restY % pack;
    const int numTilesX = w / tileSize + (restX > 0);
    const int numTilesY = (h - unaligned) / tileSize + (restY >= pack);

    for (int tx = 0; tx < numTilesX; ++tx) {
        const int startx = w - tx * tileSize - 1;
        const int stopx = qMax(startx - tileSize, 0);

        if (unaligned) {
            for (int x = startx; x >= stopx; --x) {
                ushort *d = dest + (w - x - 1) * dstride;
                for (int y = 0; y < unaligned; ++y) {
                    *d++ = src[y * sstride + x];
                }
            }
        }

        for (int ty = 0; ty < numTilesY; ++ty) {
            const int starty = ty * tileSize + unaligned;
            const int stopy = qMin(starty + tileSize, h - unoptimizedY);

            int x = startx;
            // qt_rotate90_16_neon writes to eight rows, four pixels at a time
            for (; x >= stopx + 7; x -= 8) {
                ushort *d = dest + (w - x - 1) * dstride + starty;
                const ushort *s = &src[starty * sstride + x - 7];
                qt_rotate90_16_neon(d, s, sstride * 2, dstride * 2, stopy - starty);
            }

            for (; x >= stopx; --x) {
                quint32 *d = reinterpret_cast<quint32*>(dest + (w - x - 1) * dstride + starty);
                for (int y = starty; y < stopy; y += pack) {
                    quint32 c = src[y * sstride + x];
                    for (int i = 1; i < pack; ++i) {
                        const int shift = (sizeof(int) * 8 / pack * i);
                        const ushort color = src[(y + i) * sstride + x];
                        c |= color << shift;
                    }
                    *d++ = c;
                }
            }
        }

        if (unoptimizedY) {
            const int starty = h - unoptimizedY;
            for (int x = startx; x >= stopx; --x) {
                ushort *d = dest + (w - x - 1) * dstride + starty;
                for (int y = starty; y < h; ++y) {
                    *d++ = src[y * sstride + x];
                }
            }
        }
    }
}

extern "C" void qt_rotate270_16_neon(quint16 *dst, const quint16 *src, int sstride, int dstride, int count);

void qt_memrotate270_16_neon(const uchar *srcPixels, int w, int h,
                             int sstride,
                             uchar *destPixels, int dstride)
{
    const ushort *src = (const ushort *)srcPixels;
    ushort *dest = (ushort *)destPixels;

    sstride /= sizeof(ushort);
    dstride /= sizeof(ushort);

    const int pack = sizeof(quint32) / sizeof(ushort);
    const int unaligned =
        qMin(uint((long(dest) & (sizeof(quint32)-1)) / sizeof(ushort)), uint(h));
    const int restX = w % tileSize;
    const int restY = (h - unaligned) % tileSize;
    const int unoptimizedY = restY % pack;
    const int numTilesX = w / tileSize + (restX > 0);
    const int numTilesY = (h - unaligned) / tileSize + (restY >= pack);

    for (int tx = 0; tx < numTilesX; ++tx) {
        const int startx = tx * tileSize;
        const int stopx = qMin(startx + tileSize, w);

        if (unaligned) {
            for (int x = startx; x < stopx; ++x) {
                ushort *d = dest + x * dstride;
                for (int y = h - 1; y >= h - unaligned; --y) {
                    *d++ = src[y * sstride + x];
                }
            }
        }

        for (int ty = 0; ty < numTilesY; ++ty) {
            const int starty = h - 1 - unaligned - ty * tileSize;
            const int stopy = qMax(starty - tileSize, unoptimizedY);

            int x = startx;
            // qt_rotate90_16_neon writes to eight rows, four pixels at a time
            for (; x < stopx - 7; x += 8) {
                ushort *d = dest + x * dstride + h - 1 - starty;
                const ushort *s = &src[starty * sstride + x];
                qt_rotate90_16_neon(d + 7 * dstride, s, -sstride * 2, -dstride * 2, starty - stopy);
            }

            for (; x < stopx; ++x) {
                quint32 *d = reinterpret_cast<quint32*>(dest + x * dstride
                                                        + h - 1 - starty);
                for (int y = starty; y > stopy; y -= pack) {
                    quint32 c = src[y * sstride + x];
                    for (int i = 1; i < pack; ++i) {
                        const int shift = (sizeof(int) * 8 / pack * i);
                        const ushort color = src[(y - i) * sstride + x];
                        c |= color << shift;
                    }
                    *d++ = c;
                }
            }
        }
        if (unoptimizedY) {
            const int starty = unoptimizedY - 1;
            for (int x = startx; x < stopx; ++x) {
                ushort *d = dest + x * dstride + h - 1 - starty;
                for (int y = starty; y >= 0; --y) {
                    *d++ = src[y * sstride + x];
                }
            }
        }
    }
}

class QSimdNeon
{
public:
    typedef int32x4_t Int32x4;
    typedef float32x4_t Float32x4;

    union Vect_buffer_i { Int32x4 v; int i[4]; };
    union Vect_buffer_f { Float32x4 v; float f[4]; };

    static inline Float32x4 v_dup(double x) { return vdupq_n_f32(float(x)); }
    static inline Float32x4 v_dup(float x) { return vdupq_n_f32(x); }
    static inline Int32x4 v_dup(int x) { return vdupq_n_s32(x); }
    static inline Int32x4 v_dup(uint x) { return vdupq_n_s32(x); }

    static inline Float32x4 v_add(Float32x4 a, Float32x4 b) { return vaddq_f32(a, b); }
    static inline Int32x4 v_add(Int32x4 a, Int32x4 b) { return vaddq_s32(a, b); }

    static inline Float32x4 v_max(Float32x4 a, Float32x4 b) { return vmaxq_f32(a, b); }
    static inline Float32x4 v_min(Float32x4 a, Float32x4 b) { return vminq_f32(a, b); }
    static inline Int32x4 v_min_16(Int32x4 a, Int32x4 b) { return vminq_s32(a, b); }

    static inline Int32x4 v_and(Int32x4 a, Int32x4 b) { return vandq_s32(a, b); }

    static inline Float32x4 v_sub(Float32x4 a, Float32x4 b) { return vsubq_f32(a, b); }
    static inline Int32x4 v_sub(Int32x4 a, Int32x4 b) { return vsubq_s32(a, b); }

    static inline Float32x4 v_mul(Float32x4 a, Float32x4 b) { return vmulq_f32(a, b); }

    static inline Float32x4 v_sqrt(Float32x4 x) { Float32x4 y = vrsqrteq_f32(x); y = vmulq_f32(y, vrsqrtsq_f32(x, vmulq_f32(y, y))); return vmulq_f32(x, y); }

    static inline Int32x4 v_toInt(Float32x4 x) { return vcvtq_s32_f32(x); }

    static inline Int32x4 v_greaterOrEqual(Float32x4 a, Float32x4 b) { return vreinterpretq_s32_u32(vcgeq_f32(a, b)); }
};

const uint * QT_FASTCALL qt_fetch_radial_gradient_neon(uint *buffer, const Operator *op, const QSpanData *data,
                                                       int y, int x, int length)
{
    return qt_fetch_radial_gradient_template<QRadialFetchSimd<QSimdNeon> >(buffer, op, data, y, x, length);
}

QT_END_NAMESPACE

#endif // __ARM_NEON__

