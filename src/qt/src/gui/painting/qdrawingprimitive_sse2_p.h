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

#ifndef QDRAWINGPRIMITIVE_SSE2_P_H
#define QDRAWINGPRIMITIVE_SSE2_P_H

#include <private/qsimd_p.h>

#ifdef QT_HAVE_SSE2

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

/*
 * Multiply the components of pixelVector by alphaChannel
 * Each 32bits components of alphaChannel must be in the form 0x00AA00AA
 * colorMask must have 0x00ff00ff on each 32 bits component
 * half must have the value 128 (0x80) for each 32 bits compnent
 */
#define BYTE_MUL_SSE2(result, pixelVector, alphaChannel, colorMask, half) \
{ \
    /* 1. separate the colors in 2 vectors so each color is on 16 bits \
       (in order to be multiplied by the alpha \
       each 32 bit of dstVectorAG are in the form 0x00AA00GG \
       each 32 bit of dstVectorRB are in the form 0x00RR00BB */\
    __m128i pixelVectorAG = _mm_srli_epi16(pixelVector, 8); \
    __m128i pixelVectorRB = _mm_and_si128(pixelVector, colorMask); \
 \
    /* 2. multiply the vectors by the alpha channel */\
    pixelVectorAG = _mm_mullo_epi16(pixelVectorAG, alphaChannel); \
    pixelVectorRB = _mm_mullo_epi16(pixelVectorRB, alphaChannel); \
 \
    /* 3. divide by 255, that's the tricky part. \
       we do it like for BYTE_MUL(), with bit shift: X/255 ~= (X + X/256 + rounding)/256 */ \
    /** so first (X + X/256 + rounding) */\
    pixelVectorRB = _mm_add_epi16(pixelVectorRB, _mm_srli_epi16(pixelVectorRB, 8)); \
    pixelVectorRB = _mm_add_epi16(pixelVectorRB, half); \
    pixelVectorAG = _mm_add_epi16(pixelVectorAG, _mm_srli_epi16(pixelVectorAG, 8)); \
    pixelVectorAG = _mm_add_epi16(pixelVectorAG, half); \
 \
    /** second divide by 256 */\
    pixelVectorRB = _mm_srli_epi16(pixelVectorRB, 8); \
    /** for AG, we could >> 8 to divide followed by << 8 to put the \
        bytes in the correct position. By masking instead, we execute \
        only one instruction */\
    pixelVectorAG = _mm_andnot_si128(colorMask, pixelVectorAG); \
 \
    /* 4. combine the 2 pairs of colors */ \
    result = _mm_or_si128(pixelVectorAG, pixelVectorRB); \
}

/*
 * Each 32bits components of alphaChannel must be in the form 0x00AA00AA
 * oneMinusAlphaChannel must be 255 - alpha for each 32 bits component
 * colorMask must have 0x00ff00ff on each 32 bits component
 * half must have the value 128 (0x80) for each 32 bits compnent
 */
#define INTERPOLATE_PIXEL_255_SSE2(result, srcVector, dstVector, alphaChannel, oneMinusAlphaChannel, colorMask, half) { \
    /* interpolate AG */\
    __m128i srcVectorAG = _mm_srli_epi16(srcVector, 8); \
    __m128i dstVectorAG = _mm_srli_epi16(dstVector, 8); \
    __m128i srcVectorAGalpha = _mm_mullo_epi16(srcVectorAG, alphaChannel); \
    __m128i dstVectorAGoneMinusAlphalpha = _mm_mullo_epi16(dstVectorAG, oneMinusAlphaChannel); \
    __m128i finalAG = _mm_add_epi16(srcVectorAGalpha, dstVectorAGoneMinusAlphalpha); \
    finalAG = _mm_add_epi16(finalAG, _mm_srli_epi16(finalAG, 8)); \
    finalAG = _mm_add_epi16(finalAG, half); \
    finalAG = _mm_andnot_si128(colorMask, finalAG); \
 \
    /* interpolate RB */\
    __m128i srcVectorRB = _mm_and_si128(srcVector, colorMask); \
    __m128i dstVectorRB = _mm_and_si128(dstVector, colorMask); \
    __m128i srcVectorRBalpha = _mm_mullo_epi16(srcVectorRB, alphaChannel); \
    __m128i dstVectorRBoneMinusAlphalpha = _mm_mullo_epi16(dstVectorRB, oneMinusAlphaChannel); \
    __m128i finalRB = _mm_add_epi16(srcVectorRBalpha, dstVectorRBoneMinusAlphalpha); \
    finalRB = _mm_add_epi16(finalRB, _mm_srli_epi16(finalRB, 8)); \
    finalRB = _mm_add_epi16(finalRB, half); \
    finalRB = _mm_srli_epi16(finalRB, 8); \
 \
    /* combine */\
    result = _mm_or_si128(finalAG, finalRB); \
}

// Basically blend src over dst with the const alpha defined as constAlphaVector.
// nullVector, half, one, colorMask are constant across the whole image/texture, and should be defined as:
//const __m128i nullVector = _mm_set1_epi32(0);
//const __m128i half = _mm_set1_epi16(0x80);
//const __m128i one = _mm_set1_epi16(0xff);
//const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
//const __m128i alphaMask = _mm_set1_epi32(0xff000000);
//
// The computation being done is:
// result = s + d * (1-alpha)
// with shortcuts if fully opaque or fully transparent.
#define BLEND_SOURCE_OVER_ARGB32_SSE2(dst, src, length, nullVector, half, one, colorMask, alphaMask) { \
    int x = 0; \
\
    /* First, get dst aligned. */ \
    ALIGNMENT_PROLOGUE_16BYTES(dst, x, length) { \
        uint s = src[x]; \
        if (s >= 0xff000000) \
            dst[x] = s; \
        else if (s != 0) \
            dst[x] = s + BYTE_MUL(dst[x], qAlpha(~s)); \
    } \
\
    for (; x < length-3; x += 4) { \
        const __m128i srcVector = _mm_loadu_si128((__m128i *)&src[x]); \
        const __m128i srcVectorAlpha = _mm_and_si128(srcVector, alphaMask); \
        if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, alphaMask)) == 0xffff) { \
            /* all opaque */ \
            _mm_store_si128((__m128i *)&dst[x], srcVector); \
        } else if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, nullVector)) != 0xffff) { \
            /* not fully transparent */ \
            /* extract the alpha channel on 2 x 16 bits */ \
            /* so we have room for the multiplication */ \
            /* each 32 bits will be in the form 0x00AA00AA */ \
            /* with A being the 1 - alpha */ \
            __m128i alphaChannel = _mm_srli_epi32(srcVector, 24); \
            alphaChannel = _mm_or_si128(alphaChannel, _mm_slli_epi32(alphaChannel, 16)); \
            alphaChannel = _mm_sub_epi16(one, alphaChannel); \
 \
            const __m128i dstVector = _mm_load_si128((__m128i *)&dst[x]); \
            __m128i destMultipliedByOneMinusAlpha; \
            BYTE_MUL_SSE2(destMultipliedByOneMinusAlpha, dstVector, alphaChannel, colorMask, half); \
 \
            /* result = s + d * (1-alpha) */\
            const __m128i result = _mm_add_epi8(srcVector, destMultipliedByOneMinusAlpha); \
            _mm_store_si128((__m128i *)&dst[x], result); \
        } \
    } \
    for (; x < length; ++x) { \
        uint s = src[x]; \
        if (s >= 0xff000000) \
            dst[x] = s; \
        else if (s != 0) \
            dst[x] = s + BYTE_MUL(dst[x], qAlpha(~s)); \
    } \
}

// Basically blend src over dst with the const alpha defined as constAlphaVector.
// nullVector, half, one, colorMask are constant across the whole image/texture, and should be defined as:
//const __m128i nullVector = _mm_set1_epi32(0);
//const __m128i half = _mm_set1_epi16(0x80);
//const __m128i one = _mm_set1_epi16(0xff);
//const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
//
// The computation being done is:
// dest = (s + d * sia) * ca + d * cia
//      = s * ca + d * (sia * ca + cia)
//      = s * ca + d * (1 - sa*ca)
#define BLEND_SOURCE_OVER_ARGB32_WITH_CONST_ALPHA_SSE2(dst, src, length, nullVector, half, one, colorMask, constAlphaVector) \
{ \
    int x = 0; \
\
    ALIGNMENT_PROLOGUE_16BYTES(dst, x, length) { \
        quint32 s = src[x]; \
        if (s != 0) { \
            s = BYTE_MUL(s, const_alpha); \
            dst[x] = s + BYTE_MUL(dst[x], qAlpha(~s)); \
        } \
    } \
\
    for (; x < length-3; x += 4) { \
        __m128i srcVector = _mm_loadu_si128((__m128i *)&src[x]); \
        if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVector, nullVector)) != 0xffff) { \
            BYTE_MUL_SSE2(srcVector, srcVector, constAlphaVector, colorMask, half); \
\
            __m128i alphaChannel = _mm_srli_epi32(srcVector, 24); \
            alphaChannel = _mm_or_si128(alphaChannel, _mm_slli_epi32(alphaChannel, 16)); \
            alphaChannel = _mm_sub_epi16(one, alphaChannel); \
 \
            const __m128i dstVector = _mm_load_si128((__m128i *)&dst[x]); \
            __m128i destMultipliedByOneMinusAlpha; \
            BYTE_MUL_SSE2(destMultipliedByOneMinusAlpha, dstVector, alphaChannel, colorMask, half); \
 \
            const __m128i result = _mm_add_epi8(srcVector, destMultipliedByOneMinusAlpha); \
            _mm_store_si128((__m128i *)&dst[x], result); \
        } \
    } \
    for (; x < length; ++x) { \
        quint32 s = src[x]; \
        if (s != 0) { \
            s = BYTE_MUL(s, const_alpha); \
            dst[x] = s + BYTE_MUL(dst[x], qAlpha(~s)); \
        } \
    } \
}

QT_END_NAMESPACE

#endif // QT_HAVE_SSE2

#endif // QDRAWINGPRIMITIVE_SSE2_P_H
