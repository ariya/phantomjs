/*
 * Copyright (C) 2011 University of Szeged
 * Copyright (C) 2011 Zoltan Herczeg
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY UNIVERSITY OF SZEGED ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL UNIVERSITY OF SZEGED OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FEGaussianBlurNEON_h
#define FEGaussianBlurNEON_h

#include <wtf/Platform.h>

#if CPU(ARM_NEON) && COMPILER(GCC)

#include "FEGaussianBlur.h"

namespace WebCore {

struct FEGaussianBlurPaintingDataForNeon {
    int stride;
    int strideWidth;
    int strideLine;
    int strideLineWidth;
    int remainingStrides;
    int distanceLeft;
    int distanceRight;
    float invertedKernelSize;
    unsigned char* paintingConstants;
};

unsigned char* feGaussianBlurConstantsForNeon();

extern "C" {
void neonDrawAllChannelGaussianBlur(unsigned char* source, unsigned char* destination, FEGaussianBlurPaintingDataForNeon*);
void neonDrawAlphaChannelGaussianBlur(unsigned char* source, unsigned char* destination, FEGaussianBlurPaintingDataForNeon*);
}

inline void FEGaussianBlur::platformApplyNeon(ByteArray* srcPixelArray, ByteArray* tmpPixelArray, unsigned kernelSizeX, unsigned kernelSizeY, IntSize& paintSize)
{
    const int widthMultipliedByFour = 4 * paintSize.width();
    FEGaussianBlurPaintingDataForNeon argumentsX = {
        4,
        widthMultipliedByFour,
        widthMultipliedByFour,
        (isAlphaImage() ? ((paintSize.height() >> 2) << 2) : paintSize.height()) * widthMultipliedByFour,
        isAlphaImage() ? (paintSize.height() & 0x3) : 0,
        0,
        0,
        0,
        isAlphaImage() ? 0 : feGaussianBlurConstantsForNeon()
    };
    FEGaussianBlurPaintingDataForNeon argumentsY = {
        widthMultipliedByFour,
        widthMultipliedByFour * paintSize.height(),
        4,
        (isAlphaImage() ? ((paintSize.width() >> 2) << 2) : paintSize.width()) * 4,
        isAlphaImage() ? (paintSize.width() & 0x3) : 0,
        0,
        0,
        0,
        isAlphaImage() ? 0 : feGaussianBlurConstantsForNeon()
    };

    for (int i = 0; i < 3; ++i) {
        if (kernelSizeX) {
            kernelPosition(i, kernelSizeX, argumentsX.distanceLeft, argumentsX.distanceRight);
            argumentsX.invertedKernelSize = 1 / static_cast<float>(kernelSizeX);
            if (isAlphaImage())
                neonDrawAlphaChannelGaussianBlur(srcPixelArray->data(), tmpPixelArray->data(), &argumentsX);
            else
                neonDrawAllChannelGaussianBlur(srcPixelArray->data(), tmpPixelArray->data(), &argumentsX);
        } else {
            ByteArray* auxPixelArray = tmpPixelArray;
            tmpPixelArray = srcPixelArray;
            srcPixelArray = auxPixelArray;
        }

        if (kernelSizeY) {
            kernelPosition(i, kernelSizeY, argumentsY.distanceLeft, argumentsY.distanceRight);
            argumentsY.invertedKernelSize = 1 / static_cast<float>(kernelSizeY);
            if (isAlphaImage())
                neonDrawAlphaChannelGaussianBlur(tmpPixelArray->data(), srcPixelArray->data(), &argumentsY);
            else
                neonDrawAllChannelGaussianBlur(tmpPixelArray->data(), srcPixelArray->data(), &argumentsY);
        } else {
            ByteArray* auxPixelArray = tmpPixelArray;
            tmpPixelArray = srcPixelArray;
            srcPixelArray = auxPixelArray;
        }
    }
}

} // namespace WebCore

#endif // CPU(ARM_NEON) && COMPILER(GCC)

#endif // FEGaussianBlurNEON_h
