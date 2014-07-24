/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#if ENABLE(FILTERS)
#include "FEBlend.h"
#include "FEBlendNEON.h"

#include "Filter.h"
#include "FloatPoint.h"
#include "GraphicsContext.h"
#include "RenderTreeAsText.h"
#include "TextStream.h"

#include <wtf/Uint8ClampedArray.h>

typedef unsigned char (*BlendType)(unsigned char colorA, unsigned char colorB, unsigned char alphaA, unsigned char alphaB);

namespace WebCore {

FEBlend::FEBlend(Filter* filter, BlendModeType mode)
    : FilterEffect(filter)
    , m_mode(mode)
{
}

PassRefPtr<FEBlend> FEBlend::create(Filter* filter, BlendModeType mode)
{
    return adoptRef(new FEBlend(filter, mode));
}

BlendModeType FEBlend::blendMode() const
{
    return m_mode;
}

bool FEBlend::setBlendMode(BlendModeType mode)
{
    if (m_mode == mode)
        return false;
    m_mode = mode;
    return true;
}

inline unsigned char feBlendNormal(unsigned char colorA, unsigned char colorB, unsigned char alphaA, unsigned char)
{
    return fastDivideBy255((255 - alphaA) * colorB + colorA * 255);
}

inline unsigned char feBlendMultiply(unsigned char colorA, unsigned char colorB, unsigned char alphaA, unsigned char alphaB)
{
    return fastDivideBy255((255 - alphaA) * colorB + (255 - alphaB + colorB) * colorA);
}

inline unsigned char feBlendScreen(unsigned char colorA, unsigned char colorB, unsigned char, unsigned char)
{
    return fastDivideBy255((colorB + colorA) * 255 - colorA * colorB);
}

inline unsigned char feBlendDarken(unsigned char colorA, unsigned char colorB, unsigned char alphaA, unsigned char alphaB)
{
    return fastDivideBy255(std::min((255 - alphaA) * colorB + colorA * 255, (255 - alphaB) * colorA + colorB * 255));
}

inline unsigned char feBlendLighten(unsigned char colorA, unsigned char colorB, unsigned char alphaA, unsigned char alphaB)
{
    return fastDivideBy255(std::max((255 - alphaA) * colorB + colorA * 255, (255 - alphaB) * colorA + colorB * 255));
}

inline unsigned char feBlendUnknown(unsigned char, unsigned char, unsigned char, unsigned char)
{
    return 0;
}

template<BlendType BlendFunction>
static void platformApply(unsigned char* sourcePixelA, unsigned char* sourcePixelB,
                          unsigned char* destinationPixel, unsigned pixelArrayLength)
{
    unsigned len = pixelArrayLength / 4;
    for (unsigned pixelOffset = 0; pixelOffset < len; pixelOffset++) {
        unsigned char alphaA = sourcePixelA[3];
        unsigned char alphaB = sourcePixelB[3];
        destinationPixel[0] = BlendFunction(sourcePixelA[0], sourcePixelB[0], alphaA, alphaB);
        destinationPixel[1] = BlendFunction(sourcePixelA[1], sourcePixelB[1], alphaA, alphaB);
        destinationPixel[2] = BlendFunction(sourcePixelA[2], sourcePixelB[2], alphaA, alphaB);
        destinationPixel[3] = 255 - fastDivideBy255((255 - alphaA) * (255 - alphaB));
        sourcePixelA += 4;
        sourcePixelB += 4;
        destinationPixel += 4;
    }
}

void FEBlend::platformApplyGeneric(unsigned char* sourcePixelA, unsigned char* sourcePixelB,
                                   unsigned char* destinationPixel, unsigned pixelArrayLength)
{
    switch (m_mode) {
    case FEBLEND_MODE_NORMAL:
        platformApply<feBlendNormal>(sourcePixelA, sourcePixelB, destinationPixel, pixelArrayLength);
        break;
    case FEBLEND_MODE_MULTIPLY:
        platformApply<feBlendMultiply>(sourcePixelA, sourcePixelB, destinationPixel, pixelArrayLength);
        break;
    case FEBLEND_MODE_SCREEN:
        platformApply<feBlendScreen>(sourcePixelA, sourcePixelB, destinationPixel, pixelArrayLength);
        break;
    case FEBLEND_MODE_DARKEN:
        platformApply<feBlendDarken>(sourcePixelA, sourcePixelB, destinationPixel, pixelArrayLength);
        break;
    case FEBLEND_MODE_LIGHTEN:
        platformApply<feBlendLighten>(sourcePixelA, sourcePixelB, destinationPixel, pixelArrayLength);
        break;
    case FEBLEND_MODE_UNKNOWN:
        platformApply<feBlendUnknown>(sourcePixelA, sourcePixelB, destinationPixel, pixelArrayLength);
        break;
    }
}

void FEBlend::platformApplySoftware()
{
    FilterEffect* in = inputEffect(0);
    FilterEffect* in2 = inputEffect(1);

    ASSERT(m_mode > FEBLEND_MODE_UNKNOWN);
    ASSERT(m_mode <= FEBLEND_MODE_LIGHTEN);

    Uint8ClampedArray* dstPixelArray = createPremultipliedImageResult();
    if (!dstPixelArray)
        return;

    IntRect effectADrawingRect = requestedRegionOfInputImageData(in->absolutePaintRect());
    RefPtr<Uint8ClampedArray> srcPixelArrayA = in->asPremultipliedImage(effectADrawingRect);

    IntRect effectBDrawingRect = requestedRegionOfInputImageData(in2->absolutePaintRect());
    RefPtr<Uint8ClampedArray> srcPixelArrayB = in2->asPremultipliedImage(effectBDrawingRect);

    unsigned pixelArrayLength = srcPixelArrayA->length();
    ASSERT(pixelArrayLength == srcPixelArrayB->length());

#if HAVE(ARM_NEON_INTRINSICS)
    if (pixelArrayLength >= 8)
        platformApplyNEON(srcPixelArrayA->data(), srcPixelArrayB->data(), dstPixelArray->data(), pixelArrayLength);
    else { // If there is just one pixel we expand it to two.
        ASSERT(pixelArrayLength > 0);
        uint32_t sourceA[2] = {0, 0};
        uint32_t sourceBAndDest[2] = {0, 0};

        sourceA[0] = reinterpret_cast<uint32_t*>(srcPixelArrayA->data())[0];
        sourceBAndDest[0] = reinterpret_cast<uint32_t*>(srcPixelArrayB->data())[0];
        platformApplyNEON(reinterpret_cast<uint8_t*>(sourceA), reinterpret_cast<uint8_t*>(sourceBAndDest), reinterpret_cast<uint8_t*>(sourceBAndDest), 8);
        reinterpret_cast<uint32_t*>(dstPixelArray->data())[0] = sourceBAndDest[0];
    }
#else
    platformApplyGeneric(srcPixelArrayA->data(), srcPixelArrayB->data(), dstPixelArray->data(), pixelArrayLength);
#endif
}

void FEBlend::dump()
{
}

static TextStream& operator<<(TextStream& ts, const BlendModeType& type)
{
    switch (type) {
    case FEBLEND_MODE_UNKNOWN:
        ts << "UNKNOWN";
        break;
    case FEBLEND_MODE_NORMAL:
        ts << "NORMAL";
        break;
    case FEBLEND_MODE_MULTIPLY:
        ts << "MULTIPLY";
        break;
    case FEBLEND_MODE_SCREEN:
        ts << "SCREEN";
        break;
    case FEBLEND_MODE_DARKEN:
        ts << "DARKEN";
        break;
    case FEBLEND_MODE_LIGHTEN:
        ts << "LIGHTEN";
        break;
    }
    return ts;
}

TextStream& FEBlend::externalRepresentation(TextStream& ts, int indent) const
{
    writeIndent(ts, indent);
    ts << "[feBlend";
    FilterEffect::externalRepresentation(ts);
    ts << " mode=\"" << m_mode << "\"]\n";
    inputEffect(0)->externalRepresentation(ts, indent + 1);
    inputEffect(1)->externalRepresentation(ts, indent + 1);
    return ts;
}

} // namespace WebCore

#endif // ENABLE(FILTERS)
