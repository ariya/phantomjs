/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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
#include "FEComposite.h"

#include "Filter.h"
#include "GraphicsContext.h"
#include "RenderTreeAsText.h"
#include "TextStream.h"

#include <wtf/ByteArray.h>

namespace WebCore {

FEComposite::FEComposite(Filter* filter, const CompositeOperationType& type, float k1, float k2, float k3, float k4)
    : FilterEffect(filter)
    , m_type(type)
    , m_k1(k1)
    , m_k2(k2)
    , m_k3(k3)
    , m_k4(k4)
{
}

PassRefPtr<FEComposite> FEComposite::create(Filter* filter, const CompositeOperationType& type, float k1, float k2, float k3, float k4)
{
    return adoptRef(new FEComposite(filter, type, k1, k2, k3, k4));
}

CompositeOperationType FEComposite::operation() const
{
    return m_type;
}

bool FEComposite::setOperation(CompositeOperationType type)
{
    if (m_type == type)
        return false;
    m_type = type;
    return true;
}

float FEComposite::k1() const
{
    return m_k1;
}

bool FEComposite::setK1(float k1)
{
    if (m_k1 == k1)
        return false;
    m_k1 = k1;
    return true;
}

float FEComposite::k2() const
{
    return m_k2;
}

bool FEComposite::setK2(float k2)
{
    if (m_k2 == k2)
        return false;
    m_k2 = k2;
    return true;
}

float FEComposite::k3() const
{
    return m_k3;
}

bool FEComposite::setK3(float k3)
{
    if (m_k3 == k3)
        return false;
    m_k3 = k3;
    return true;
}

float FEComposite::k4() const
{
    return m_k4;
}

bool FEComposite::setK4(float k4)
{
    if (m_k4 == k4)
        return false;
    m_k4 = k4;
    return true;
}

template <int b1, int b2, int b3, int b4>
inline void computeArithmeticPixels(unsigned char* source, unsigned char* destination, int pixelArrayLength,
                                    float k1, float k2, float k3, float k4)
{
    float scaledK4;
    float scaledK1;
    if (b1)
        scaledK1 = k1 / 255.f;
    if (b4)
        scaledK4 = k4 * 255.f;

    while (--pixelArrayLength >= 0) {
        unsigned char i1 = *source;
        unsigned char i2 = *destination;
        float result = 0;
        if (b1)
            result += scaledK1 * i1 * i2;
        if (b2)
            result += k2 * i1;
        if (b3)
            result += k3 * i2;
        if (b4)
            result += scaledK4;

        if (result <= 0)
            *destination = 0;
        else if (result >= 255)
            *destination = 255;
        else
            *destination = result;
        ++source;
        ++destination;
    }
}

inline void arithmetic(ByteArray* srcPixelArrayA, ByteArray* srcPixelArrayB,
                       float k1, float k2, float k3, float k4)
{
    int pixelArrayLength = srcPixelArrayA->length();
    ASSERT(pixelArrayLength == static_cast<int>(srcPixelArrayB->length()));
    unsigned char* source = srcPixelArrayA->data();
    unsigned char* destination = srcPixelArrayB->data();

    if (!k4) {
        if (!k1) {
            computeArithmeticPixels<0, 1, 1, 0>(source, destination, pixelArrayLength, k1, k2, k3, k4);
            return;
        }

        computeArithmeticPixels<1, 1, 1, 0>(source, destination, pixelArrayLength, k1, k2, k3, k4);
        return;
    }

    if (!k1) {
        computeArithmeticPixels<0, 1, 1, 1>(source, destination, pixelArrayLength, k1, k2, k3, k4);
        return;
    }
    computeArithmeticPixels<1, 1, 1, 1>(source, destination, pixelArrayLength, k1, k2, k3, k4);
}

void FEComposite::determineAbsolutePaintRect()
{
    switch (m_type) {
    case FECOMPOSITE_OPERATOR_IN:
    case FECOMPOSITE_OPERATOR_ATOP:
        // For In and Atop the first effect just influences the result of
        // the second effect. So just use the absolute paint rect of the second effect here.
        setAbsolutePaintRect(inputEffect(1)->absolutePaintRect());
        return;
    case FECOMPOSITE_OPERATOR_ARITHMETIC:
        // Arithmetic may influnce the compele filter primitive region. So we can't
        // optimize the paint region here.
        setAbsolutePaintRect(maxEffectRect());
        return;
    default:
        // Take the union of both input effects.
        FilterEffect::determineAbsolutePaintRect();
        return;
    }
}

void FEComposite::apply()
{
    if (hasResult())
        return;
    FilterEffect* in = inputEffect(0);
    FilterEffect* in2 = inputEffect(1);
    in->apply();
    in2->apply();
    if (!in->hasResult() || !in2->hasResult())
        return;

    if (m_type == FECOMPOSITE_OPERATOR_ARITHMETIC) {
        ByteArray* dstPixelArray = createPremultipliedImageResult();
        if (!dstPixelArray)
            return;

        IntRect effectADrawingRect = requestedRegionOfInputImageData(in->absolutePaintRect());
        RefPtr<ByteArray> srcPixelArray = in->asPremultipliedImage(effectADrawingRect);

        IntRect effectBDrawingRect = requestedRegionOfInputImageData(in2->absolutePaintRect());
        in2->copyPremultipliedImage(dstPixelArray, effectBDrawingRect);

        arithmetic(srcPixelArray.get(), dstPixelArray, m_k1, m_k2, m_k3, m_k4);
        return;
    }

    ImageBuffer* resultImage = createImageBufferResult();
    if (!resultImage)
        return;
    GraphicsContext* filterContext = resultImage->context();

    FloatRect srcRect = FloatRect(0, 0, -1, -1);
    switch (m_type) {
    case FECOMPOSITE_OPERATOR_OVER:
        filterContext->drawImageBuffer(in2->asImageBuffer(), ColorSpaceDeviceRGB, drawingRegionOfInputImage(in2->absolutePaintRect()));
        filterContext->drawImageBuffer(in->asImageBuffer(), ColorSpaceDeviceRGB, drawingRegionOfInputImage(in->absolutePaintRect()));
        break;
    case FECOMPOSITE_OPERATOR_IN: {
        GraphicsContextStateSaver stateSaver(*filterContext);
        filterContext->clipToImageBuffer(in2->asImageBuffer(), drawingRegionOfInputImage(in2->absolutePaintRect()));
        filterContext->drawImageBuffer(in->asImageBuffer(), ColorSpaceDeviceRGB, drawingRegionOfInputImage(in->absolutePaintRect()));
        break;
    }
    case FECOMPOSITE_OPERATOR_OUT:
        filterContext->drawImageBuffer(in->asImageBuffer(), ColorSpaceDeviceRGB, drawingRegionOfInputImage(in->absolutePaintRect()));
        filterContext->drawImageBuffer(in2->asImageBuffer(), ColorSpaceDeviceRGB, drawingRegionOfInputImage(in2->absolutePaintRect()), srcRect, CompositeDestinationOut);
        break;
    case FECOMPOSITE_OPERATOR_ATOP:
        filterContext->drawImageBuffer(in2->asImageBuffer(), ColorSpaceDeviceRGB, drawingRegionOfInputImage(in2->absolutePaintRect()));
        filterContext->drawImageBuffer(in->asImageBuffer(), ColorSpaceDeviceRGB, drawingRegionOfInputImage(in->absolutePaintRect()), srcRect, CompositeSourceAtop);
        break;
    case FECOMPOSITE_OPERATOR_XOR:
        filterContext->drawImageBuffer(in2->asImageBuffer(), ColorSpaceDeviceRGB, drawingRegionOfInputImage(in2->absolutePaintRect()));
        filterContext->drawImageBuffer(in->asImageBuffer(), ColorSpaceDeviceRGB, drawingRegionOfInputImage(in->absolutePaintRect()), srcRect, CompositeXOR);
        break;
    default:
        break;
    }
}

void FEComposite::dump()
{
}

static TextStream& operator<<(TextStream& ts, const CompositeOperationType& type)
{
    switch (type) {
    case FECOMPOSITE_OPERATOR_UNKNOWN:
        ts << "UNKNOWN";
        break;
    case FECOMPOSITE_OPERATOR_OVER:
        ts << "OVER";
        break;
    case FECOMPOSITE_OPERATOR_IN:
        ts << "IN";
        break;
    case FECOMPOSITE_OPERATOR_OUT:
        ts << "OUT";
        break;
    case FECOMPOSITE_OPERATOR_ATOP:
        ts << "ATOP";
        break;
    case FECOMPOSITE_OPERATOR_XOR:
        ts << "XOR";
        break;
    case FECOMPOSITE_OPERATOR_ARITHMETIC:
        ts << "ARITHMETIC";
        break;
    }
    return ts;
}

TextStream& FEComposite::externalRepresentation(TextStream& ts, int indent) const
{
    writeIndent(ts, indent);
    ts << "[feComposite";
    FilterEffect::externalRepresentation(ts);
    ts << " operation=\"" << m_type << "\"";
    if (m_type == FECOMPOSITE_OPERATOR_ARITHMETIC)
        ts << " k1=\"" << m_k1 << "\" k2=\"" << m_k2 << "\" k3=\"" << m_k3 << "\" k4=\"" << m_k4 << "\"";
    ts << "]\n";
    inputEffect(0)->externalRepresentation(ts, indent + 1);
    inputEffect(1)->externalRepresentation(ts, indent + 1);
    return ts;
}

} // namespace WebCore

#endif // ENABLE(FILTERS)
