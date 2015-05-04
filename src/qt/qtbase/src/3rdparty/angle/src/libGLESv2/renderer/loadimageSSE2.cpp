//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// loadimageSSE2.cpp: Defines image loading functions. It's
// in a separated file for GCC, which can enable SSE usage only per-file,
// not for code blocks that use SSE2 explicitly.

#include "libGLESv2/renderer/loadimage.h"

namespace rx
{

void LoadA8ToBGRA8_SSE2(size_t width, size_t height, size_t depth,
                        const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                        uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
#if defined(_M_ARM)
    // Ensure that this function is reported as not implemented for ARM builds because
    // the instructions below are not present for that architecture.
    UNIMPLEMENTED();
    return;
#else
    __m128i zeroWide = _mm_setzero_si128();

    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint8_t *source = OffsetDataPointer<uint8_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint32_t *dest = OffsetDataPointer<uint32_t>(output, y, z, outputRowPitch, outputDepthPitch);

            size_t x = 0;

            // Make output writes aligned
            for (; ((reinterpret_cast<intptr_t>(&dest[x]) & 0xF) != 0 && x < width); x++)
            {
                dest[x] = static_cast<uint32_t>(source[x]) << 24;
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
                dest[x] = static_cast<uint32_t>(source[x]) << 24;
            }
        }
    }
#endif
}

void LoadRGBA8ToBGRA8_SSE2(size_t width, size_t height, size_t depth,
                           const uint8_t *input, size_t inputRowPitch, size_t inputDepthPitch,
                           uint8_t *output, size_t outputRowPitch, size_t outputDepthPitch)
{
#if defined(_M_ARM)
    // Ensure that this function is reported as not implemented for ARM builds because
    // the instructions below are not present for that architecture.
    UNIMPLEMENTED();
    return;
#else
    __m128i brMask = _mm_set1_epi32(0x00ff00ff);

    for (size_t z = 0; z < depth; z++)
    {
        for (size_t y = 0; y < height; y++)
        {
            const uint32_t *source = OffsetDataPointer<uint32_t>(input, y, z, inputRowPitch, inputDepthPitch);
            uint32_t *dest = OffsetDataPointer<uint32_t>(output, y, z, outputRowPitch, outputDepthPitch);

            size_t x = 0;

            // Make output writes aligned
            for (; ((reinterpret_cast<intptr_t>(&dest[x]) & 15) != 0) && x < width; x++)
            {
                uint32_t rgba = source[x];
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
                uint32_t rgba = source[x];
                dest[x] = (_rotl(rgba, 16) & 0x00ff00ff) | (rgba & 0xff00ff00);
            }
        }
    }
#endif
}

}
