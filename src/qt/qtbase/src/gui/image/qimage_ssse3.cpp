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

#include <qimage.h>
#include <private/qimage_p.h>
#include <private/qsimd_p.h>

#ifdef QT_COMPILER_SUPPORTS_SSSE3

QT_BEGIN_NAMESPACE

// Convert a scanline of RGB888 (src) to RGB32 (dst)
// src must be at least len * 3 bytes
// dst must be at least len * 4 bytes
Q_GUI_EXPORT void QT_FASTCALL qt_convert_rgb888_to_rgb32_ssse3(quint32 *dst, const uchar *src, int len)
{
    quint32 *const end = dst + len;

    // Prologue, align dst to 16 bytes. The alignment is done on dst because it has 4 store()
    // for each 3 load() of src.
    const int offsetToAlignOn16Bytes = (4 - ((reinterpret_cast<quintptr>(dst) >> 2) & 0x3)) & 0x3;
    const int prologLength = qMin(len, offsetToAlignOn16Bytes);

    for (int i = 0; i < prologLength; ++i) {
        *dst++ = qRgb(src[0], src[1], src[2]);
        src += 3;
    }

    // Mask the 4 first colors of the RGB888 vector
    const __m128i shuffleMask = _mm_set_epi8(char(0xff), 9, 10, 11, char(0xff), 6, 7, 8, char(0xff), 3, 4, 5, char(0xff), 0, 1, 2);

    // Mask the 4 last colors of a RGB888 vector with an offset of 1 (so the last 3 bytes are RGB)
    const __m128i shuffleMaskEnd = _mm_set_epi8(char(0xff), 13, 14, 15, char(0xff), 10, 11, 12, char(0xff), 7, 8, 9, char(0xff), 4, 5, 6);

    // Mask to have alpha = 0xff
    const __m128i alphaMask = _mm_set1_epi32(0xff000000);

    __m128i *inVectorPtr = (__m128i *)src;
    __m128i *dstVectorPtr = (__m128i *)dst;

    const int simdRoundCount = (len - prologLength) / 16; // one iteration in the loop converts 16 pixels
    for (int i = 0; i < simdRoundCount; ++i) {
        /*
         RGB888 has 5 pixels per vector, + 1 byte from the next pixel. The idea here is
         to load vectors of RGB888 and use palignr to select a vector out of two vectors.

         After 3 loads of RGB888 and 3 stores of RGB32, we have 4 pixels left in the last
         vector of RGB888, we can mask it directly to get a last store or RGB32. After that,
         the first next byte is a R, and we can loop for the next 16 pixels.

         The conversion itself is done with a byte permutation (pshufb).
         */
        __m128i firstSrcVector = _mm_lddqu_si128(inVectorPtr);
        __m128i outputVector = _mm_shuffle_epi8(firstSrcVector, shuffleMask);
        _mm_store_si128(dstVectorPtr, _mm_or_si128(outputVector, alphaMask));
        ++inVectorPtr;
        ++dstVectorPtr;

        // There are 4 unused bytes left in srcVector, we need to load the next 16 bytes
        // and load the next input with palignr
        __m128i secondSrcVector = _mm_lddqu_si128(inVectorPtr);
        __m128i srcVector = _mm_alignr_epi8(secondSrcVector, firstSrcVector, 12);
        outputVector = _mm_shuffle_epi8(srcVector, shuffleMask);
        _mm_store_si128(dstVectorPtr, _mm_or_si128(outputVector, alphaMask));
        ++inVectorPtr;
        ++dstVectorPtr;
        firstSrcVector = secondSrcVector;

        // We now have 8 unused bytes left in firstSrcVector
        secondSrcVector = _mm_lddqu_si128(inVectorPtr);
        srcVector = _mm_alignr_epi8(secondSrcVector, firstSrcVector, 8);
        outputVector = _mm_shuffle_epi8(srcVector, shuffleMask);
        _mm_store_si128(dstVectorPtr, _mm_or_si128(outputVector, alphaMask));
        ++inVectorPtr;
        ++dstVectorPtr;

        // There are now 12 unused bytes in firstSrcVector.
        // We can mask them directly, almost there.
        outputVector = _mm_shuffle_epi8(secondSrcVector, shuffleMaskEnd);
        _mm_store_si128(dstVectorPtr, _mm_or_si128(outputVector, alphaMask));
        ++dstVectorPtr;
    }
    src = (uchar *)inVectorPtr;
    dst = (quint32 *)dstVectorPtr;

    while (dst != end) {
        *dst++ = qRgb(src[0], src[1], src[2]);
        src += 3;
    }
}

void convert_RGB888_to_RGB32_ssse3(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_RGB888);
    Q_ASSERT(dest->format == QImage::Format_RGB32 || dest->format == QImage::Format_ARGB32 || dest->format == QImage::Format_ARGB32_Premultiplied);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const uchar *src_data = (uchar *) src->data;
    quint32 *dest_data = (quint32 *) dest->data;

    for (int i = 0; i < src->height; ++i) {
        qt_convert_rgb888_to_rgb32_ssse3(dest_data, src_data, src->width);
        src_data += src->bytes_per_line;
        dest_data = (quint32 *)((uchar*)dest_data + dest->bytes_per_line);
    }
}

QT_END_NAMESPACE

#endif // QT_COMPILER_SUPPORTS_SSSE3
