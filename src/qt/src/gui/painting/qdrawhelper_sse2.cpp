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

#include <private/qdrawhelper_x86_p.h>

#ifdef QT_HAVE_SSE2

#include <private/qdrawingprimitive_sse2_p.h>
#include <private/qpaintengine_raster_p.h>

QT_BEGIN_NAMESPACE

void qt_blend_argb32_on_argb32_sse2(uchar *destPixels, int dbpl,
                                    const uchar *srcPixels, int sbpl,
                                    int w, int h,
                                    int const_alpha)
{
    const quint32 *src = (const quint32 *) srcPixels;
    quint32 *dst = (quint32 *) destPixels;
    if (const_alpha == 256) {
        const __m128i alphaMask = _mm_set1_epi32(0xff000000);
        const __m128i nullVector = _mm_set1_epi32(0);
        const __m128i half = _mm_set1_epi16(0x80);
        const __m128i one = _mm_set1_epi16(0xff);
        const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
        for (int y = 0; y < h; ++y) {
            BLEND_SOURCE_OVER_ARGB32_SSE2(dst, src, w, nullVector, half, one, colorMask, alphaMask);
            dst = (quint32 *)(((uchar *) dst) + dbpl);
            src = (const quint32 *)(((const uchar *) src) + sbpl);
        }
    } else if (const_alpha != 0) {
        // dest = (s + d * sia) * ca + d * cia
        //      = s * ca + d * (sia * ca + cia)
        //      = s * ca + d * (1 - sa*ca)
        const_alpha = (const_alpha * 255) >> 8;
        const __m128i nullVector = _mm_set1_epi32(0);
        const __m128i half = _mm_set1_epi16(0x80);
        const __m128i one = _mm_set1_epi16(0xff);
        const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
        const __m128i constAlphaVector = _mm_set1_epi16(const_alpha);
        for (int y = 0; y < h; ++y) {
            BLEND_SOURCE_OVER_ARGB32_WITH_CONST_ALPHA_SSE2(dst, src, w, nullVector, half, one, colorMask, constAlphaVector)
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

void qt_blend_rgb32_on_rgb32_sse2(uchar *destPixels, int dbpl,
                                 const uchar *srcPixels, int sbpl,
                                 int w, int h,
                                 int const_alpha)
{
    const quint32 *src = (const quint32 *) srcPixels;
    quint32 *dst = (quint32 *) destPixels;
    if (const_alpha != 256) {
        if (const_alpha != 0) {
            const __m128i nullVector = _mm_set1_epi32(0);
            const __m128i half = _mm_set1_epi16(0x80);
            const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);

            const_alpha = (const_alpha * 255) >> 8;
            int one_minus_const_alpha = 255 - const_alpha;
            const __m128i constAlphaVector = _mm_set1_epi16(const_alpha);
            const __m128i oneMinusConstAlpha =  _mm_set1_epi16(one_minus_const_alpha);
            for (int y = 0; y < h; ++y) {
                int x = 0;

                // First, align dest to 16 bytes:
                ALIGNMENT_PROLOGUE_16BYTES(dst, x, w) {
                    dst[x] = INTERPOLATE_PIXEL_255(src[x], const_alpha, dst[x], one_minus_const_alpha);
                }

                for (; x < w-3; x += 4) {
                    __m128i srcVector = _mm_loadu_si128((__m128i *)&src[x]);
                    if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVector, nullVector)) != 0xffff) {
                        const __m128i dstVector = _mm_load_si128((__m128i *)&dst[x]);
                        __m128i result;
                        INTERPOLATE_PIXEL_255_SSE2(result, srcVector, dstVector, constAlphaVector, oneMinusConstAlpha, colorMask, half);
                        _mm_store_si128((__m128i *)&dst[x], result);
                    }
                }
                for (; x<w; ++x) {
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

void QT_FASTCALL comp_func_SourceOver_sse2(uint *destPixels, const uint *srcPixels, int length, uint const_alpha)
{
    Q_ASSERT(const_alpha < 256);

    const quint32 *src = (const quint32 *) srcPixels;
    quint32 *dst = (quint32 *) destPixels;

    const __m128i nullVector = _mm_set1_epi32(0);
    const __m128i half = _mm_set1_epi16(0x80);
    const __m128i one = _mm_set1_epi16(0xff);
    const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
    if (const_alpha == 255) {
        const __m128i alphaMask = _mm_set1_epi32(0xff000000);
        BLEND_SOURCE_OVER_ARGB32_SSE2(dst, src, length, nullVector, half, one, colorMask, alphaMask);
    } else {
        const __m128i constAlphaVector = _mm_set1_epi16(const_alpha);
        BLEND_SOURCE_OVER_ARGB32_WITH_CONST_ALPHA_SSE2(dst, src, length, nullVector, half, one, colorMask, constAlphaVector);
    }
}

void QT_FASTCALL comp_func_Plus_sse2(uint *dst, const uint *src, int length, uint const_alpha)
{
    int x = 0;

    if (const_alpha == 255) {
        // 1) Prologue: align destination on 16 bytes
        ALIGNMENT_PROLOGUE_16BYTES(dst, x, length)
            dst[x] = comp_func_Plus_one_pixel(dst[x], src[x]);

        // 2) composition with SSE2
        for (; x < length - 3; x += 4) {
            const __m128i srcVector = _mm_loadu_si128((__m128i *)&src[x]);
            const __m128i dstVector = _mm_load_si128((__m128i *)&dst[x]);

            const __m128i result = _mm_adds_epu8(srcVector, dstVector);
            _mm_store_si128((__m128i *)&dst[x], result);
        }

        // 3) Epilogue:
        for (; x < length; ++x)
            dst[x] = comp_func_Plus_one_pixel(dst[x], src[x]);
    } else {
        const int one_minus_const_alpha = 255 - const_alpha;
        const __m128i constAlphaVector = _mm_set1_epi16(const_alpha);
        const __m128i oneMinusConstAlpha =  _mm_set1_epi16(one_minus_const_alpha);

        // 1) Prologue: align destination on 16 bytes
        ALIGNMENT_PROLOGUE_16BYTES(dst, x, length)
            dst[x] = comp_func_Plus_one_pixel_const_alpha(dst[x], src[x], const_alpha, one_minus_const_alpha);

        const __m128i half = _mm_set1_epi16(0x80);
        const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
        // 2) composition with SSE2
        for (; x < length - 3; x += 4) {
            const __m128i srcVector = _mm_loadu_si128((__m128i *)&src[x]);
            const __m128i dstVector = _mm_load_si128((__m128i *)&dst[x]);

            __m128i result = _mm_adds_epu8(srcVector, dstVector);
            INTERPOLATE_PIXEL_255_SSE2(result, result, dstVector, constAlphaVector, oneMinusConstAlpha, colorMask, half)
            _mm_store_si128((__m128i *)&dst[x], result);
        }

        // 3) Epilogue:
        for (; x < length; ++x)
            dst[x] = comp_func_Plus_one_pixel_const_alpha(dst[x], src[x], const_alpha, one_minus_const_alpha);
    }
}

void QT_FASTCALL comp_func_Source_sse2(uint *dst, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        ::memcpy(dst, src, length * sizeof(uint));
    } else {
        const int ialpha = 255 - const_alpha;

        int x = 0;

        // 1) prologue, align on 16 bytes
        ALIGNMENT_PROLOGUE_16BYTES(dst, x, length)
            dst[x] = INTERPOLATE_PIXEL_255(src[x], const_alpha, dst[x], ialpha);

        // 2) interpolate pixels with SSE2
        const __m128i half = _mm_set1_epi16(0x80);
        const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
        const __m128i constAlphaVector = _mm_set1_epi16(const_alpha);
        const __m128i oneMinusConstAlpha =  _mm_set1_epi16(ialpha);
        for (; x < length - 3; x += 4) {
            const __m128i srcVector = _mm_loadu_si128((__m128i *)&src[x]);
            __m128i dstVector = _mm_load_si128((__m128i *)&dst[x]);
            INTERPOLATE_PIXEL_255_SSE2(dstVector, srcVector, dstVector, constAlphaVector, oneMinusConstAlpha, colorMask, half)
            _mm_store_si128((__m128i *)&dst[x], dstVector);
        }

        // 3) Epilogue
        for (; x < length; ++x)
            dst[x] = INTERPOLATE_PIXEL_255(src[x], const_alpha, dst[x], ialpha);
    }
}

void qt_memfill32_sse2(quint32 *dest, quint32 value, int count)
{
    if (count < 7) {
        switch (count) {
        case 6: *dest++ = value;
        case 5: *dest++ = value;
        case 4: *dest++ = value;
        case 3: *dest++ = value;
        case 2: *dest++ = value;
        case 1: *dest   = value;
        }
        return;
    };

    const int align = (quintptr)(dest) & 0xf;
    switch (align) {
    case 4:  *dest++ = value; --count;
    case 8:  *dest++ = value; --count;
    case 12: *dest++ = value; --count;
    }

    int count128 = count / 4;
    __m128i *dst128 = reinterpret_cast<__m128i*>(dest);
    const __m128i value128 = _mm_set_epi32(value, value, value, value);

    int n = (count128 + 3) / 4;
    switch (count128 & 0x3) {
    case 0: do { _mm_stream_si128(dst128++, value128);
    case 3:      _mm_stream_si128(dst128++, value128);
    case 2:      _mm_stream_si128(dst128++, value128);
    case 1:      _mm_stream_si128(dst128++, value128);
    } while (--n > 0);
    }

    const int rest = count & 0x3;
    if (rest) {
        switch (rest) {
        case 3: dest[count - 3] = value;
        case 2: dest[count - 2] = value;
        case 1: dest[count - 1] = value;
        }
    }
}

void QT_FASTCALL comp_func_solid_SourceOver_sse2(uint *destPixels, int length, uint color, uint const_alpha)
{
    if ((const_alpha & qAlpha(color)) == 255) {
        qt_memfill32_sse2(destPixels, color, length);
    } else {
        if (const_alpha != 255)
            color = BYTE_MUL(color, const_alpha);

        const quint32 minusAlphaOfColor = qAlpha(~color);
        int x = 0;

        quint32 *dst = (quint32 *) destPixels;
        const __m128i colorVector = _mm_set1_epi32(color);
        const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
        const __m128i half = _mm_set1_epi16(0x80);
        const __m128i minusAlphaOfColorVector = _mm_set1_epi16(minusAlphaOfColor);

        ALIGNMENT_PROLOGUE_16BYTES(dst, x, length)
            destPixels[x] = color + BYTE_MUL(destPixels[x], minusAlphaOfColor);

        for (; x < length-3; x += 4) {
            __m128i dstVector = _mm_load_si128((__m128i *)&dst[x]);
            BYTE_MUL_SSE2(dstVector, dstVector, minusAlphaOfColorVector, colorMask, half);
            dstVector = _mm_add_epi8(colorVector, dstVector);
            _mm_store_si128((__m128i *)&dst[x], dstVector);
        }
        for (;x < length; ++x)
            destPixels[x] = color + BYTE_MUL(destPixels[x], minusAlphaOfColor);
    }
}

CompositionFunctionSolid qt_functionForModeSolid_onlySSE2[numCompositionFunctions] = {
    comp_func_solid_SourceOver_sse2,
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

CompositionFunction qt_functionForMode_onlySSE2[numCompositionFunctions] = {
    comp_func_SourceOver_sse2,
    comp_func_DestinationOver,
    comp_func_Clear,
    comp_func_Source_sse2,
    comp_func_Destination,
    comp_func_SourceIn,
    comp_func_DestinationIn,
    comp_func_SourceOut,
    comp_func_DestinationOut,
    comp_func_SourceAtop,
    comp_func_DestinationAtop,
    comp_func_XOR,
    comp_func_Plus_sse2,
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

void qt_memfill16_sse2(quint16 *dest, quint16 value, int count)
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
    qt_memfill32_sse2(reinterpret_cast<quint32*>(dest), value32, count / 2);

    if (count & 0x1)
        dest[count - 1] = value;
}

void qt_bitmapblit32_sse2(QRasterBuffer *rasterBuffer, int x, int y,
                          quint32 color,
                          const uchar *src, int width, int height, int stride)
{
    quint32 *dest = reinterpret_cast<quint32*>(rasterBuffer->scanLine(y)) + x;
    const int destStride = rasterBuffer->bytesPerLine() / sizeof(quint32);

    const __m128i c128 = _mm_set1_epi32(color);
    const __m128i maskmask1 = _mm_set_epi32(0x10101010, 0x20202020,
                                            0x40404040, 0x80808080);
    const __m128i maskadd1 = _mm_set_epi32(0x70707070, 0x60606060,
                                           0x40404040, 0x00000000);

    if (width > 4) {
        const __m128i maskmask2 = _mm_set_epi32(0x01010101, 0x02020202,
                                                0x04040404, 0x08080808);
        const __m128i maskadd2 = _mm_set_epi32(0x7f7f7f7f, 0x7e7e7e7e,
                                               0x7c7c7c7c, 0x78787878);
        while (height--) {
            for (int x = 0; x < width; x += 8) {
                const quint8 s = src[x >> 3];
                if (!s)
                    continue;
                __m128i mask1 = _mm_set1_epi8(s);
                __m128i mask2 = mask1;

                mask1 = _mm_and_si128(mask1, maskmask1);
                mask1 = _mm_add_epi8(mask1, maskadd1);
                _mm_maskmoveu_si128(c128, mask1, (char*)(dest + x));
                mask2 = _mm_and_si128(mask2, maskmask2);
                mask2 = _mm_add_epi8(mask2, maskadd2);
                _mm_maskmoveu_si128(c128, mask2, (char*)(dest + x + 4));
            }
            dest += destStride;
            src += stride;
        }
    } else {
        while (height--) {
            const quint8 s = *src;
            if (s) {
                __m128i mask1 = _mm_set1_epi8(s);
                mask1 = _mm_and_si128(mask1, maskmask1);
                mask1 = _mm_add_epi8(mask1, maskadd1);
                _mm_maskmoveu_si128(c128, mask1, (char*)(dest));
            }
            dest += destStride;
            src += stride;
        }
    }
}

void qt_bitmapblit16_sse2(QRasterBuffer *rasterBuffer, int x, int y,
                          quint32 color,
                          const uchar *src, int width, int height, int stride)
{
    const quint16 c = qt_colorConvert<quint16, quint32>(color, 0);
    quint16 *dest = reinterpret_cast<quint16*>(rasterBuffer->scanLine(y)) + x;
    const int destStride = rasterBuffer->bytesPerLine() / sizeof(quint16);

    const __m128i c128 = _mm_set1_epi16(c);
#if defined(Q_CC_MSVC)
#  pragma warning(disable: 4309) // truncation of constant value
#endif
    const __m128i maskmask = _mm_set_epi16(0x0101, 0x0202, 0x0404, 0x0808,
                                           0x1010, 0x2020, 0x4040, 0x8080);
    const __m128i maskadd = _mm_set_epi16(0x7f7f, 0x7e7e, 0x7c7c, 0x7878,
                                          0x7070, 0x6060, 0x4040, 0x0000);

    while (height--) {
        for (int x = 0; x < width; x += 8) {
            const quint8 s = src[x >> 3];
            if (!s)
                continue;
            __m128i mask = _mm_set1_epi8(s);
            mask = _mm_and_si128(mask, maskmask);
            mask = _mm_add_epi8(mask, maskadd);
            _mm_maskmoveu_si128(c128, mask, (char*)(dest + x));
        }
        dest += destStride;
        src += stride;
    }
}

class QSimdSse2
{
public:
    typedef __m128i Int32x4;
    typedef __m128 Float32x4;

    union Vect_buffer_i { Int32x4 v; int i[4]; };
    union Vect_buffer_f { Float32x4 v; float f[4]; };

    static inline Float32x4 v_dup(float x) { return _mm_set1_ps(x); }
    static inline Float32x4 v_dup(double x) { return _mm_set1_ps(x); }
    static inline Int32x4 v_dup(int x) { return _mm_set1_epi32(x); }
    static inline Int32x4 v_dup(uint x) { return _mm_set1_epi32(x); }

    static inline Float32x4 v_add(Float32x4 a, Float32x4 b) { return _mm_add_ps(a, b); }
    static inline Int32x4 v_add(Int32x4 a, Int32x4 b) { return _mm_add_epi32(a, b); }

    static inline Float32x4 v_max(Float32x4 a, Float32x4 b) { return _mm_max_ps(a, b); }
    static inline Float32x4 v_min(Float32x4 a, Float32x4 b) { return _mm_min_ps(a, b); }
    static inline Int32x4 v_min_16(Int32x4 a, Int32x4 b) { return _mm_min_epi16(a, b); }

    static inline Int32x4 v_and(Int32x4 a, Int32x4 b) { return _mm_and_si128(a, b); }

    static inline Float32x4 v_sub(Float32x4 a, Float32x4 b) { return _mm_sub_ps(a, b); }
    static inline Int32x4 v_sub(Int32x4 a, Int32x4 b) { return _mm_sub_epi32(a, b); }

    static inline Float32x4 v_mul(Float32x4 a, Float32x4 b) { return _mm_mul_ps(a, b); }

    static inline Float32x4 v_sqrt(Float32x4 x) { return _mm_sqrt_ps(x); }

    static inline Int32x4 v_toInt(Float32x4 x) { return _mm_cvttps_epi32(x); }

    // pre-VS 2008 doesn't have cast intrinsics, whereas 2008 and later requires it
#if defined(Q_CC_MSVC) && _MSC_VER < 1500
    static inline Int32x4 v_greaterOrEqual(Float32x4 a, Float32x4 b)
    {
        union Convert { Int32x4 vi; Float32x4 vf; } convert;
        convert.vf = _mm_cmpgt_ps(a, b);
        return convert.vi;
    }
#else
    static inline Int32x4 v_greaterOrEqual(Float32x4 a, Float32x4 b) { return _mm_castps_si128(_mm_cmpgt_ps(a, b)); }
#endif
};

const uint * QT_FASTCALL qt_fetch_radial_gradient_sse2(uint *buffer, const Operator *op, const QSpanData *data,
                                                       int y, int x, int length)
{
    return qt_fetch_radial_gradient_template<QRadialFetchSimd<QSimdSse2> >(buffer, op, data, y, x, length);
}


QT_END_NAMESPACE

#endif // QT_HAVE_SSE2
