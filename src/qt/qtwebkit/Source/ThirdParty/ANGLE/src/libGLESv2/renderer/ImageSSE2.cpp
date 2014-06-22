#include "precompiled.h"
//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ImageSSE2.cpp: Implements SSE2-based functions of rx::Image class. It's
// in a separated file for GCC, which can enable SSE usage only per-file,
// not for code blocks that use SSE2 explicitly.

#include "libGLESv2/Texture.h"
#include "libGLESv2/renderer/Image.h"

namespace rx
{

void Image::loadRGBAUByteDataToBGRASSE2(GLsizei width, GLsizei height,
                                        int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const unsigned int *source = NULL;
    unsigned int *dest = NULL;
    __m128i brMask = _mm_set1_epi32(0x00ff00ff);

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const unsigned int*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<unsigned int*>(static_cast<unsigned char*>(output) + y * outputPitch);
        int x = 0;

        // Make output writes aligned
        for (x = 0; ((reinterpret_cast<intptr_t>(&dest[x]) & 15) != 0) && x < width; x++)
        {
            unsigned int rgba = source[x];
            dest[x] = (_rotl(rgba, 16) & 0x00ff00ff) | (rgba & 0xff00ff00);
        }

        for (; x + 3 < width; x += 4)
        {
            __m128i sourceData = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&source[x]));
            // Mask out g and a, which don't change
            __m128i gaComponents = _mm_andnot_si128(brMask, sourceData);
            // Mask out b and r
            __m128i brComponents = _mm_and_si128(sourceData, brMask);
            // Swap b and r
            __m128i brSwapped = _mm_shufflehi_epi16(_mm_shufflelo_epi16(brComponents, _MM_SHUFFLE(2, 3, 0, 1)), _MM_SHUFFLE(2, 3, 0, 1));
            __m128i result = _mm_or_si128(gaComponents, brSwapped);
            _mm_store_si128(reinterpret_cast<__m128i*>(&dest[x]), result);
        }

        // Perform leftover writes
        for (; x < width; x++)
        {
            unsigned int rgba = source[x];
            dest[x] = (_rotl(rgba, 16) & 0x00ff00ff) | (rgba & 0xff00ff00);
        }
    }
}

void Image::loadAlphaDataToBGRASSE2(GLsizei width, GLsizei height,
                                    int inputPitch, const void *input, size_t outputPitch, void *output)
{
    const unsigned char *source = NULL;
    unsigned int *dest = NULL;
    __m128i zeroWide = _mm_setzero_si128();

    for (int y = 0; y < height; y++)
    {
        source = static_cast<const unsigned char*>(input) + y * inputPitch;
        dest = reinterpret_cast<unsigned int*>(static_cast<unsigned char*>(output) + y * outputPitch);

        int x;
        // Make output writes aligned
        for (x = 0; ((reinterpret_cast<intptr_t>(&dest[x]) & 0xF) != 0 && x < width); x++)
        {
            dest[x] = static_cast<unsigned int>(source[x]) << 24;
        }

        for (; x + 7 < width; x += 8)
        {
            __m128i sourceData = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(&source[x]));
            // Interleave each byte to 16bit, make the lower byte to zero
            sourceData = _mm_unpacklo_epi8(zeroWide, sourceData);
            // Interleave each 16bit to 32bit, make the lower 16bit to zero
            __m128i lo = _mm_unpacklo_epi16(zeroWide, sourceData);
            __m128i hi = _mm_unpackhi_epi16(zeroWide, sourceData);

            _mm_store_si128(reinterpret_cast<__m128i*>(&dest[x]), lo);
            _mm_store_si128(reinterpret_cast<__m128i*>(&dest[x + 4]), hi);
        }

        // Handle the remainder
        for (; x < width; x++)
        {
            dest[x] = static_cast<unsigned int>(source[x]) << 24;
        }
    }
}

}
