/*
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ImageBuffer.h"

#include "IntRect.h"
#include <wtf/MathExtras.h>

namespace WebCore {

#if !USE(CG)
void ImageBuffer::transformColorSpace(ColorSpace srcColorSpace, ColorSpace dstColorSpace)
{
    DEFINE_STATIC_LOCAL(Vector<int>, deviceRgbLUT, ());
    DEFINE_STATIC_LOCAL(Vector<int>, linearRgbLUT, ());

    if (srcColorSpace == dstColorSpace)
        return;

    // only sRGB <-> linearRGB are supported at the moment
    if ((srcColorSpace != ColorSpaceLinearRGB && srcColorSpace != ColorSpaceDeviceRGB)
        || (dstColorSpace != ColorSpaceLinearRGB && dstColorSpace != ColorSpaceDeviceRGB))
        return;

    if (dstColorSpace == ColorSpaceLinearRGB) {
        if (linearRgbLUT.isEmpty()) {
            for (unsigned i = 0; i < 256; i++) {
                float color = i  / 255.0f;
                color = (color <= 0.04045f ? color / 12.92f : pow((color + 0.055f) / 1.055f, 2.4f));
                color = std::max(0.0f, color);
                color = std::min(1.0f, color);
                linearRgbLUT.append(static_cast<int>(round(color * 255)));
            }
        }
        platformTransformColorSpace(linearRgbLUT);
    } else if (dstColorSpace == ColorSpaceDeviceRGB) {
        if (deviceRgbLUT.isEmpty()) {
            for (unsigned i = 0; i < 256; i++) {
                float color = i / 255.0f;
                color = (powf(color, 1.0f / 2.4f) * 1.055f) - 0.055f;
                color = std::max(0.0f, color);
                color = std::min(1.0f, color);
                deviceRgbLUT.append(static_cast<int>(round(color * 255)));
            }
        }
        platformTransformColorSpace(deviceRgbLUT);
    }
}
#endif // USE(CG)

inline void ImageBuffer::genericConvertToLuminanceMask()
{
    IntRect luminanceRect(IntPoint(), internalSize());
    RefPtr<Uint8ClampedArray> srcPixelArray = getUnmultipliedImageData(luminanceRect);
    
    unsigned pixelArrayLength = srcPixelArray->length();
    for (unsigned pixelOffset = 0; pixelOffset < pixelArrayLength; pixelOffset += 4) {
        unsigned char a = srcPixelArray->item(pixelOffset + 3);
        if (!a)
            continue;
        unsigned char r = srcPixelArray->item(pixelOffset);
        unsigned char g = srcPixelArray->item(pixelOffset + 1);
        unsigned char b = srcPixelArray->item(pixelOffset + 2);
        
        double luma = (r * 0.2125 + g * 0.7154 + b * 0.0721) * ((double)a / 255.0);
        srcPixelArray->set(pixelOffset + 3, luma);
    }
    putByteArray(Unmultiplied, srcPixelArray.get(), luminanceRect.size(), luminanceRect, IntPoint());
}

void ImageBuffer::convertToLuminanceMask()
{
    // Add platform specific functions with platformConvertToLuminanceMask here later.
    genericConvertToLuminanceMask();
}

#if USE(ACCELERATED_COMPOSITING) && !USE(CAIRO) && !PLATFORM(BLACKBERRY)
PlatformLayer* ImageBuffer::platformLayer() const
{
    return 0;
}
#endif

bool ImageBuffer::copyToPlatformTexture(GraphicsContext3D&, Platform3DObject, GC3Denum, bool, bool)
{
    return false;
}

PassOwnPtr<ImageBuffer> ImageBuffer::createCompatibleBuffer(const IntSize& size, float resolutionScale, ColorSpace colorSpace, const GraphicsContext* context, bool)
{
    return create(size, resolutionScale, colorSpace, context->isAcceleratedContext() ? Accelerated : Unaccelerated);
}

}
