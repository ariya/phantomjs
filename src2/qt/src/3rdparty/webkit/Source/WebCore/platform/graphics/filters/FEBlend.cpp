/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
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

#include "Filter.h"
#include "FloatPoint.h"
#include "GraphicsContext.h"
#include "RenderTreeAsText.h"
#include "TextStream.h"

#include <wtf/ByteArray.h>

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

static unsigned char unknown(unsigned char, unsigned char, unsigned char, unsigned char)
{
    return 0;
}

static unsigned char normal(unsigned char colorA, unsigned char colorB, unsigned char alphaA, unsigned char)
{
    return (((255 - alphaA) * colorB + colorA * 255) / 255);
}

static unsigned char multiply(unsigned char colorA, unsigned char colorB, unsigned char alphaA, unsigned char alphaB)
{
    return (((255 - alphaA) * colorB + (255 - alphaB + colorB) * colorA) / 255);
}

static unsigned char screen(unsigned char colorA, unsigned char colorB, unsigned char, unsigned char)
{
    return (((colorB + colorA) * 255 - colorA * colorB) / 255);
}

static unsigned char darken(unsigned char colorA, unsigned char colorB, unsigned char alphaA, unsigned char alphaB)
{
    return ((std::min((255 - alphaA) * colorB + colorA * 255, (255 - alphaB) * colorA + colorB * 255)) / 255);
}

static unsigned char lighten(unsigned char colorA, unsigned char colorB, unsigned char alphaA, unsigned char alphaB)
{
    return ((std::max((255 - alphaA) * colorB + colorA * 255, (255 - alphaB) * colorA + colorB * 255)) / 255);
}

void FEBlend::apply()
{
    if (hasResult())
        return;
    FilterEffect* in = inputEffect(0);
    FilterEffect* in2 = inputEffect(1);
    in->apply();
    in2->apply();
    if (!in->hasResult() || !in2->hasResult())
        return;

    if (m_mode <= FEBLEND_MODE_UNKNOWN || m_mode > FEBLEND_MODE_LIGHTEN)
        return;

    ByteArray* dstPixelArray = createPremultipliedImageResult();
    if (!dstPixelArray)
        return;

    IntRect effectADrawingRect = requestedRegionOfInputImageData(in->absolutePaintRect());
    RefPtr<ByteArray> srcPixelArrayA = in->asPremultipliedImage(effectADrawingRect);

    IntRect effectBDrawingRect = requestedRegionOfInputImageData(in2->absolutePaintRect());
    RefPtr<ByteArray> srcPixelArrayB = in2->asPremultipliedImage(effectBDrawingRect);

    // Keep synchronized with BlendModeType
    static const BlendType callEffect[] = {unknown, normal, multiply, screen, darken, lighten};

    unsigned pixelArrayLength = srcPixelArrayA->length();
    ASSERT(pixelArrayLength == srcPixelArrayB->length());
    for (unsigned pixelOffset = 0; pixelOffset < pixelArrayLength; pixelOffset += 4) {
        unsigned char alphaA = srcPixelArrayA->get(pixelOffset + 3);
        unsigned char alphaB = srcPixelArrayB->get(pixelOffset + 3);
        for (unsigned channel = 0; channel < 3; ++channel) {
            unsigned char colorA = srcPixelArrayA->get(pixelOffset + channel);
            unsigned char colorB = srcPixelArrayB->get(pixelOffset + channel);

            unsigned char result = (*callEffect[m_mode])(colorA, colorB, alphaA, alphaB);
            dstPixelArray->set(pixelOffset + channel, result);
        }
        unsigned char alphaR = 255 - ((255 - alphaA) * (255 - alphaB)) / 255;
        dstPixelArray->set(pixelOffset + 3, alphaR);
    }
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
