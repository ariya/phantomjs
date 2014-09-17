/*
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
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

#if !USE(CG)

#include <math.h>

namespace WebCore {

void ImageBuffer::transformColorSpace(ColorSpace srcColorSpace, ColorSpace dstColorSpace)
{
    if (srcColorSpace == dstColorSpace)
        return;

    // only sRGB <-> linearRGB are supported at the moment
    if ((srcColorSpace != ColorSpaceLinearRGB && srcColorSpace != ColorSpaceDeviceRGB) 
        || (dstColorSpace != ColorSpaceLinearRGB && dstColorSpace != ColorSpaceDeviceRGB))
        return;

    if (dstColorSpace == ColorSpaceLinearRGB) {
        if (m_linearRgbLUT.isEmpty()) {
            for (unsigned i = 0; i < 256; i++) {
                float color = i  / 255.0f;
                color = (color <= 0.04045f ? color / 12.92f : pow((color + 0.055f) / 1.055f, 2.4f));
                color = std::max(0.0f, color);
                color = std::min(1.0f, color);
                m_linearRgbLUT.append(static_cast<int>(color * 255));
            }
        }
        platformTransformColorSpace(m_linearRgbLUT);
    } else if (dstColorSpace == ColorSpaceDeviceRGB) {
        if (m_deviceRgbLUT.isEmpty()) {
            for (unsigned i = 0; i < 256; i++) {
                float color = i / 255.0f;
                color = (powf(color, 1.0f / 2.4f) * 1.055f) - 0.055f;
                color = std::max(0.0f, color);
                color = std::min(1.0f, color);
                m_deviceRgbLUT.append(static_cast<int>(color * 255));
            }
        }
        platformTransformColorSpace(m_deviceRgbLUT);
    }
}

}

#endif // USE(CG)
