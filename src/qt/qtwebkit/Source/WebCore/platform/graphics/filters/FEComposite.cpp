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

#include "FECompositeArithmeticNEON.h"
#include "Filter.h"
#include "GraphicsContext.h"
#include "RenderTreeAsText.h"
#include "TextStream.h"

#include <wtf/Uint8ClampedArray.h>

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

void FEComposite::correctFilterResultIfNeeded()
{
    if (m_type != FECOMPOSITE_OPERATOR_ARITHMETIC)
        return;

    forceValidPreMultipliedPixels();
}

template <int b1, int b4>
static inline void computeArithmeticPixels(unsigned char* source, unsigned char* destination, int pixelArrayLength,
                                    float k1, float k2, float k3, float k4)
{
    float scaledK1;
    float scaledK4;
    if (b1)
        scaledK1 = k1 / 255.0f;
    if (b4)
        scaledK4 = k4 * 255.0f;

    while (--pixelArrayLength >= 0) {
        unsigned char i1 = *source;
        unsigned char i2 = *destination;
        float result = k2 * i1 + k3 * i2;
        if (b1)
            result += scaledK1 * i1 * i2;
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

// computeArithmeticPixelsUnclamped is a faster version of computeArithmeticPixels for the common case where clamping
// is not necessary. This enables aggresive compiler optimizations such as auto-vectorization.
template <int b1, int b4>
static inline void computeArithmeticPixelsUnclamped(unsigned char* source, unsigned char* destination, int pixelArrayLength, float k1, float k2, float k3, float k4)
{
    float scaledK1;
    float scaledK4;
    if (b1)
        scaledK1 = k1 / 255.0f;
    if (b4)
        scaledK4 = k4 * 255.0f;

    while (--pixelArrayLength >= 0) {
        unsigned char i1 = *source;
        unsigned char i2 = *destination;
        float result = k2 * i1 + k3 * i2;
        if (b1)
            result += scaledK1 * i1 * i2;
        if (b4)
            result += scaledK4;

        *destination = result;
        ++source;
        ++destination;
    }
}

static inline void arithmeticSoftware(unsigned char* source, unsigned char* destination, int pixelArrayLength, float k1, float k2, float k3, float k4)
{
    float upperLimit = std::max(0.0f, k1) + std::max(0.0f, k2) + std::max(0.0f, k3) + k4;
    float lowerLimit = std::min(0.0f, k1) + std::min(0.0f, k2) + std::min(0.0f, k3) + k4;
    if ((k4 >= 0.0f && k4 <= 1.0f) && (upperLimit >= 0.0f && upperLimit <= 1.0f) && (lowerLimit >= 0.0f && lowerLimit <= 1.0f)) {
        if (k4) {
            if (k1)
                computeArithmeticPixelsUnclamped<1, 1>(source, destination, pixelArrayLength, k1, k2, k3, k4);
            else
                computeArithmeticPixelsUnclamped<0, 1>(source, destination, pixelArrayLength, k1, k2, k3, k4);
        } else {
            if (k1)
                computeArithmeticPixelsUnclamped<1, 0>(source, destination, pixelArrayLength, k1, k2, k3, k4);
            else
                computeArithmeticPixelsUnclamped<0, 0>(source, destination, pixelArrayLength, k1, k2, k3, k4);
        }
        return;
    }

    if (k4) {
        if (k1)
            computeArithmeticPixels<1, 1>(source, destination, pixelArrayLength, k1, k2, k3, k4);
        else
            computeArithmeticPixels<0, 1>(source, destination, pixelArrayLength, k1, k2, k3, k4);
    } else {
        if (k1)
            computeArithmeticPixels<1, 0>(source, destination, pixelArrayLength, k1, k2, k3, k4);
        else
            computeArithmeticPixels<0, 0>(source, destination, pixelArrayLength, k1, k2, k3, k4);
    }
}

inline void FEComposite::platformArithmeticSoftware(Uint8ClampedArray* source, Uint8ClampedArray* destination,
    float k1, float k2, float k3, float k4)
{
    int length = source->length();
    ASSERT(length == static_cast<int>(destination->length()));
    // The selection here eventually should happen dynamically.
#if HAVE(ARM_NEON_INTRINSICS)
    ASSERT(!(length & 0x3));
    platformArithmeticNeon(source->data(), destination->data(), length, k1, k2, k3, k4);
#else
    arithmeticSoftware(source->data(), destination->data(), length, k1, k2, k3, k4);
#endif
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
        setAbsolutePaintRect(enclosingIntRect(maxEffectRect()));
        return;
    default:
        // Take the union of both input effects.
        FilterEffect::determineAbsolutePaintRect();
        return;
    }
}

void FEComposite::platformApplySoftware()
{
    FilterEffect* in = inputEffect(0);
    FilterEffect* in2 = inputEffect(1);

    if (m_type == FECOMPOSITE_OPERATOR_ARITHMETIC) {
        Uint8ClampedArray* dstPixelArray = createPremultipliedImageResult();
        if (!dstPixelArray)
            return;

        IntRect effectADrawingRect = requestedRegionOfInputImageData(in->absolutePaintRect());
        RefPtr<Uint8ClampedArray> srcPixelArray = in->asPremultipliedImage(effectADrawingRect);

        IntRect effectBDrawingRect = requestedRegionOfInputImageData(in2->absolutePaintRect());
        in2->copyPremultipliedImage(dstPixelArray, effectBDrawingRect);

        platformArithmeticSoftware(srcPixelArray.get(), dstPixelArray, m_k1, m_k2, m_k3, m_k4);
        return;
    }

    ImageBuffer* resultImage = createImageBufferResult();
    if (!resultImage)
        return;
    GraphicsContext* filterContext = resultImage->context();

    ImageBuffer* imageBuffer = in->asImageBuffer();
    ImageBuffer* imageBuffer2 = in2->asImageBuffer();
    ASSERT(imageBuffer);
    ASSERT(imageBuffer2);

    switch (m_type) {
    case FECOMPOSITE_OPERATOR_OVER:
        filterContext->drawImageBuffer(imageBuffer2, ColorSpaceDeviceRGB, drawingRegionOfInputImage(in2->absolutePaintRect()));
        filterContext->drawImageBuffer(imageBuffer, ColorSpaceDeviceRGB, drawingRegionOfInputImage(in->absolutePaintRect()));
        break;
    case FECOMPOSITE_OPERATOR_IN: {
        // Applies only to the intersected region.
        IntRect destinationRect = in->absolutePaintRect();
        destinationRect.intersect(in2->absolutePaintRect());
        destinationRect.intersect(absolutePaintRect());
        if (destinationRect.isEmpty())
            break;
        IntPoint destinationPoint(destinationRect.x() - absolutePaintRect().x(), destinationRect.y() - absolutePaintRect().y());
        IntRect sourceRect(IntPoint(destinationRect.x() - in->absolutePaintRect().x(),
                                    destinationRect.y() - in->absolutePaintRect().y()), destinationRect.size());
        IntRect source2Rect(IntPoint(destinationRect.x() - in2->absolutePaintRect().x(),
                                     destinationRect.y() - in2->absolutePaintRect().y()), destinationRect.size());
        filterContext->drawImageBuffer(imageBuffer2, ColorSpaceDeviceRGB, destinationPoint, source2Rect);
        filterContext->drawImageBuffer(imageBuffer, ColorSpaceDeviceRGB, destinationPoint, sourceRect, CompositeSourceIn);
        break;
    }
    case FECOMPOSITE_OPERATOR_OUT:
        filterContext->drawImageBuffer(imageBuffer, ColorSpaceDeviceRGB, drawingRegionOfInputImage(in->absolutePaintRect()));
        filterContext->drawImageBuffer(imageBuffer2, ColorSpaceDeviceRGB, drawingRegionOfInputImage(in2->absolutePaintRect()), IntRect(IntPoint(), imageBuffer2->logicalSize()), CompositeDestinationOut);
        break;
    case FECOMPOSITE_OPERATOR_ATOP:
        filterContext->drawImageBuffer(imageBuffer2, ColorSpaceDeviceRGB, drawingRegionOfInputImage(in2->absolutePaintRect()));
        filterContext->drawImageBuffer(imageBuffer, ColorSpaceDeviceRGB, drawingRegionOfInputImage(in->absolutePaintRect()), IntRect(IntPoint(), imageBuffer->logicalSize()), CompositeSourceAtop);
        break;
    case FECOMPOSITE_OPERATOR_XOR:
        filterContext->drawImageBuffer(imageBuffer2, ColorSpaceDeviceRGB, drawingRegionOfInputImage(in2->absolutePaintRect()));
        filterContext->drawImageBuffer(imageBuffer, ColorSpaceDeviceRGB, drawingRegionOfInputImage(in->absolutePaintRect()), IntRect(IntPoint(), imageBuffer->logicalSize()), CompositeXOR);
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
