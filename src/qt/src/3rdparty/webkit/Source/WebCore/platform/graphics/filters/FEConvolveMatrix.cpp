/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Zoltan Herczeg <zherczeg@webkit.org>
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
#include "FEConvolveMatrix.h"

#include "Filter.h"
#include "RenderTreeAsText.h"
#include "TextStream.h"

#include <wtf/ByteArray.h>

namespace WebCore {

FEConvolveMatrix::FEConvolveMatrix(Filter* filter, const IntSize& kernelSize,
    float divisor, float bias, const IntPoint& targetOffset, EdgeModeType edgeMode,
    const FloatPoint& kernelUnitLength, bool preserveAlpha, const Vector<float>& kernelMatrix)
    : FilterEffect(filter)
    , m_kernelSize(kernelSize)
    , m_divisor(divisor)
    , m_bias(bias)
    , m_targetOffset(targetOffset)
    , m_edgeMode(edgeMode)
    , m_kernelUnitLength(kernelUnitLength)
    , m_preserveAlpha(preserveAlpha)
    , m_kernelMatrix(kernelMatrix)
{
}

PassRefPtr<FEConvolveMatrix> FEConvolveMatrix::create(Filter* filter, const IntSize& kernelSize,
    float divisor, float bias, const IntPoint& targetOffset, EdgeModeType edgeMode,
    const FloatPoint& kernelUnitLength, bool preserveAlpha, const Vector<float>& kernelMatrix)
{
    return adoptRef(new FEConvolveMatrix(filter, kernelSize, divisor, bias, targetOffset, edgeMode, kernelUnitLength,
        preserveAlpha, kernelMatrix));
}


IntSize FEConvolveMatrix::kernelSize() const
{
    return m_kernelSize;
}

void FEConvolveMatrix::setKernelSize(const IntSize& kernelSize)
{
    m_kernelSize = kernelSize; 
}

const Vector<float>& FEConvolveMatrix::kernel() const
{
    return m_kernelMatrix; 
}

void FEConvolveMatrix::setKernel(const Vector<float>& kernel)
{
    m_kernelMatrix = kernel; 
}

float FEConvolveMatrix::divisor() const
{
    return m_divisor; 
}

bool FEConvolveMatrix::setDivisor(float divisor)
{
    if (m_divisor == divisor)
        return false;
    m_divisor = divisor;
    return true;
}

float FEConvolveMatrix::bias() const
{
    return m_bias; 
}

bool FEConvolveMatrix::setBias(float bias)
{
    if (m_bias == bias)
        return false;
    m_bias = bias;
    return true;
}

IntPoint FEConvolveMatrix::targetOffset() const
{
    return m_targetOffset; 
}

bool FEConvolveMatrix::setTargetOffset(const IntPoint& targetOffset)
{
    if (m_targetOffset == targetOffset)
        return false;
    m_targetOffset = targetOffset;
    return true;
}

EdgeModeType FEConvolveMatrix::edgeMode() const
{
    return m_edgeMode;
}

bool FEConvolveMatrix::setEdgeMode(EdgeModeType edgeMode)
{
    if (m_edgeMode == edgeMode)
        return false;
    m_edgeMode = edgeMode;
    return true;
}

FloatPoint FEConvolveMatrix::kernelUnitLength() const
{
    return m_kernelUnitLength; 
}

bool FEConvolveMatrix::setKernelUnitLength(const FloatPoint& kernelUnitLength)
{
    if (m_kernelUnitLength == kernelUnitLength)
        return false;
    m_kernelUnitLength = kernelUnitLength;
    return true;
}

bool FEConvolveMatrix::preserveAlpha() const
{
    return m_preserveAlpha; 
}

bool FEConvolveMatrix::setPreserveAlpha(bool preserveAlpha)
{
    if (m_preserveAlpha == preserveAlpha)
        return false;
    m_preserveAlpha = preserveAlpha;
    return true;
}

/*
   -----------------------------------
      ConvolveMatrix implementation
   -----------------------------------

   The image rectangle is split in the following way:

      +---------------------+
      |          A          |
      +---------------------+
      |   |             |   |
      | B |      C      | D |
      |   |             |   |
      +---------------------+
      |          E          |
      +---------------------+

   Where region C contains those pixels, whose values
   can be calculated without crossing the edge of the rectangle.

   Example:
      Image size: width: 10, height: 10

      Order (kernel matrix size): width: 3, height 4
      Target: x:1, y:3

      The following figure shows the target inside the kernel matrix:

        ...
        ...
        ...
        .X.

   The regions in this case are the following:
      Note: (x1, y1) top-left and (x2, y2) is the bottom-right corner
      Note: row x2 and column y2 is not part of the region
            only those (x, y) pixels, where x1 <= x < x2 and y1 <= y < y2

      Region A: x1: 0, y1: 0, x2: 10, y2: 3
      Region B: x1: 0, y1: 3, x2: 1, y2: 10
      Region C: x1: 1, y1: 3, x2: 9, y2: 10
      Region D: x1: 9, y1: 3, x2: 10, y2: 10
      Region E: x1: 0, y1: 10, x2: 10, y2: 10 (empty region)

   Since region C (often) contains most of the pixels, we implemented
   a fast algoritm to calculate these values, called fastSetInteriorPixels.
   For other regions, fastSetOuterPixels is used, which calls getPixelValue,
   to handle pixels outside of the image. In a rare situations, when
   kernel matrix is bigger than the image, all pixels are calculated by this
   function.

   Although these two functions have lot in common, I decided not to make
   common a template for them, since there are key differences as well,
   and would make it really hard to understand.
*/

static ALWAYS_INLINE unsigned char clampRGBAValue(float channel, unsigned char max = 255)
{
    if (channel <= 0)
        return 0;
    if (channel >= max)
        return max;
    return channel;
}

template<bool preserveAlphaValues>
ALWAYS_INLINE void setDestinationPixels(ByteArray* image, int& pixel, float* totals, float divisor, float bias, ByteArray* src)
{
    unsigned char maxAlpha = preserveAlphaValues ? 255 : clampRGBAValue(totals[3] / divisor + bias);
    for (int i = 0; i < 3; ++i)
        image->set(pixel++, clampRGBAValue(totals[i] / divisor + bias, maxAlpha));

    if (preserveAlphaValues) {
        image->set(pixel, src->get(pixel));
        ++pixel;
    } else
        image->set(pixel++, maxAlpha);
}

// Only for region C
template<bool preserveAlphaValues>
ALWAYS_INLINE void FEConvolveMatrix::fastSetInteriorPixels(PaintingData& paintingData, int clipRight, int clipBottom)
{
    // edge mode does not affect these pixels
    int pixel = (m_targetOffset.y() * paintingData.width + m_targetOffset.x()) * 4;
    int startKernelPixel = 0;
    int kernelIncrease = clipRight * 4;
    int xIncrease = (m_kernelSize.width() - 1) * 4;
    // Contains the sum of rgb(a) components
    float totals[3 + (preserveAlphaValues ? 0 : 1)];

    // m_divisor cannot be 0, SVGFEConvolveMatrixElement ensures this
    ASSERT(m_divisor);

    for (int y = clipBottom + 1; y > 0; --y) {
        for (int x = clipRight + 1; x > 0; --x) {
            int kernelValue = m_kernelMatrix.size() - 1;
            int kernelPixel = startKernelPixel;
            int width = m_kernelSize.width();

            totals[0] = 0;
            totals[1] = 0;
            totals[2] = 0;
            if (!preserveAlphaValues)
                totals[3] = 0;

            while (kernelValue >= 0) {
                totals[0] += m_kernelMatrix[kernelValue] * static_cast<float>(paintingData.srcPixelArray->get(kernelPixel++));
                totals[1] += m_kernelMatrix[kernelValue] * static_cast<float>(paintingData.srcPixelArray->get(kernelPixel++));
                totals[2] += m_kernelMatrix[kernelValue] * static_cast<float>(paintingData.srcPixelArray->get(kernelPixel++));
                if (!preserveAlphaValues)
                    totals[3] += m_kernelMatrix[kernelValue] * static_cast<float>(paintingData.srcPixelArray->get(kernelPixel));
                ++kernelPixel;
                --kernelValue;
                if (!--width) {
                    kernelPixel += kernelIncrease;
                    width = m_kernelSize.width();
                }
            }

            setDestinationPixels<preserveAlphaValues>(paintingData.dstPixelArray, pixel, totals, m_divisor, paintingData.bias, paintingData.srcPixelArray);
            startKernelPixel += 4;
        }
        pixel += xIncrease;
        startKernelPixel += xIncrease;
    }
}

ALWAYS_INLINE int FEConvolveMatrix::getPixelValue(PaintingData& paintingData, int x, int y)
{
    if (x >= 0 && x < paintingData.width && x >= 0 && y < paintingData.height)
        return (y * paintingData.width + x) << 2;

    switch (m_edgeMode) {
    default: // EDGEMODE_NONE
        return -1;
    case EDGEMODE_DUPLICATE:
        if (x < 0)
            x = 0;
        else if (x >= paintingData.width)
            x = paintingData.width - 1;
        if (y < 0)
            y = 0;
        else if (y >= paintingData.height)
            y = paintingData.height - 1;
        return (y * paintingData.width + x) << 2;
    case EDGEMODE_WRAP:
        while (x < 0)
            x += paintingData.width;
        x %= paintingData.width;
        while (y < 0)
            y += paintingData.height;
        y %= paintingData.height;
        return (y * paintingData.width + x) << 2;
    }
}

// For other regions than C
template<bool preserveAlphaValues>
void FEConvolveMatrix::fastSetOuterPixels(PaintingData& paintingData, int x1, int y1, int x2, int y2)
{
    int pixel = (y1 * paintingData.width + x1) * 4;
    int height = y2 - y1;
    int width = x2 - x1;
    int beginKernelPixelX = x1 - m_targetOffset.x();
    int startKernelPixelX = beginKernelPixelX;
    int startKernelPixelY = y1 - m_targetOffset.y();
    int xIncrease = (paintingData.width - width) * 4;
    // Contains the sum of rgb(a) components
    float totals[3 + (preserveAlphaValues ? 0 : 1)];

    // m_divisor cannot be 0, SVGFEConvolveMatrixElement ensures this
    ASSERT(m_divisor);

    for (int y = height; y > 0; --y) {
        for (int x = width; x > 0; --x) {
            int kernelValue = m_kernelMatrix.size() - 1;
            int kernelPixelX = startKernelPixelX;
            int kernelPixelY = startKernelPixelY;
            int width = m_kernelSize.width();

            totals[0] = 0;
            totals[1] = 0;
            totals[2] = 0;
            if (!preserveAlphaValues)
                totals[3] = 0;

            while (kernelValue >= 0) {
                int pixelIndex = getPixelValue(paintingData, kernelPixelX, kernelPixelY);
                if (pixelIndex >= 0) {
                    totals[0] += m_kernelMatrix[kernelValue] * static_cast<float>(paintingData.srcPixelArray->get(pixelIndex));
                    totals[1] += m_kernelMatrix[kernelValue] * static_cast<float>(paintingData.srcPixelArray->get(pixelIndex + 1));
                    totals[2] += m_kernelMatrix[kernelValue] * static_cast<float>(paintingData.srcPixelArray->get(pixelIndex + 2));
                }
                if (!preserveAlphaValues && pixelIndex >= 0)
                    totals[3] += m_kernelMatrix[kernelValue] * static_cast<float>(paintingData.srcPixelArray->get(pixelIndex + 3));
                ++kernelPixelX;
                --kernelValue;
                if (!--width) {
                    kernelPixelX = startKernelPixelX;
                    ++kernelPixelY;
                    width = m_kernelSize.width();
                }
            }

            setDestinationPixels<preserveAlphaValues>(paintingData.dstPixelArray, pixel, totals, m_divisor, paintingData.bias, paintingData.srcPixelArray);
            ++startKernelPixelX;
        }
        pixel += xIncrease;
        startKernelPixelX = beginKernelPixelX;
        ++startKernelPixelY;
    }
}

ALWAYS_INLINE void FEConvolveMatrix::setInteriorPixels(PaintingData& paintingData, int clipRight, int clipBottom)
{
    // Must be implemented here, since it refers another ALWAYS_INLINE
    // function, which defined in this C++ source file as well
    if (m_preserveAlpha)
        fastSetInteriorPixels<true>(paintingData, clipRight, clipBottom);
    else
        fastSetInteriorPixels<false>(paintingData, clipRight, clipBottom);
}

ALWAYS_INLINE void FEConvolveMatrix::setOuterPixels(PaintingData& paintingData, int x1, int y1, int x2, int y2)
{
    // Although this function can be moved to the header, it is implemented here
    // because setInteriorPixels is also implemented here
    if (m_preserveAlpha)
        fastSetOuterPixels<true>(paintingData, x1, y1, x2, y2);
    else
        fastSetOuterPixels<false>(paintingData, x1, y1, x2, y2);
}

void FEConvolveMatrix::apply()
{
    if (hasResult())
        return;
    FilterEffect* in = inputEffect(0);
    in->apply();
    if (!in->hasResult())
        return;

    ByteArray* resultImage;
    if (m_preserveAlpha)
        resultImage = createUnmultipliedImageResult();
    else
        resultImage = createPremultipliedImageResult();
    if (!resultImage)
        return;

    IntRect effectDrawingRect = requestedRegionOfInputImageData(in->absolutePaintRect());

    RefPtr<ByteArray> srcPixelArray;
    if (m_preserveAlpha)
        srcPixelArray = in->asUnmultipliedImage(effectDrawingRect);
    else
        srcPixelArray = in->asPremultipliedImage(effectDrawingRect);

    IntSize paintSize = absolutePaintRect().size();
    PaintingData paintingData;
    paintingData.srcPixelArray = srcPixelArray.get();
    paintingData.dstPixelArray = resultImage;
    paintingData.width = paintSize.width();
    paintingData.height = paintSize.height();
    paintingData.bias = m_bias * 255;

    // Drawing fully covered pixels
    int clipRight = paintSize.width() - m_kernelSize.width();
    int clipBottom = paintSize.height() - m_kernelSize.height();

    if (clipRight >= 0 && clipBottom >= 0) {
        setInteriorPixels(paintingData, clipRight, clipBottom);

        clipRight += m_targetOffset.x() + 1;
        clipBottom += m_targetOffset.y() + 1;
        if (m_targetOffset.y() > 0)
            setOuterPixels(paintingData, 0, 0, paintSize.width(), m_targetOffset.y());
        if (clipBottom < paintSize.height())
            setOuterPixels(paintingData, 0, clipBottom, paintSize.width(), paintSize.height());
        if (m_targetOffset.x() > 0)
            setOuterPixels(paintingData, 0, m_targetOffset.y(), m_targetOffset.x(), clipBottom);
        if (clipRight < paintSize.width())
            setOuterPixels(paintingData, clipRight, m_targetOffset.y(), paintSize.width(), clipBottom);
    } else {
        // Rare situation, not optimizied for speed
        setOuterPixels(paintingData, 0, 0, paintSize.width(), paintSize.height());
    }
}

void FEConvolveMatrix::dump()
{
}

static TextStream& operator<<(TextStream& ts, const EdgeModeType& type)
{
    switch (type) {
    case EDGEMODE_UNKNOWN:
        ts << "UNKNOWN";
        break;
    case EDGEMODE_DUPLICATE:
        ts << "DUPLICATE";
        break;
    case EDGEMODE_WRAP:
        ts << "WRAP";
        break;
    case EDGEMODE_NONE:
        ts << "NONE";
        break;
    }
    return ts;
}

TextStream& FEConvolveMatrix::externalRepresentation(TextStream& ts, int indent) const
{
    writeIndent(ts, indent);
    ts << "[feConvolveMatrix";
    FilterEffect::externalRepresentation(ts);
    ts << " order=\"" << m_kernelSize << "\" "
       << "kernelMatrix=\"" << m_kernelMatrix  << "\" "
       << "divisor=\"" << m_divisor << "\" "
       << "bias=\"" << m_bias << "\" "
       << "target=\"" << m_targetOffset << "\" "
       << "edgeMode=\"" << m_edgeMode << "\" "
       << "kernelUnitLength=\"" << m_kernelUnitLength << "\" "
       << "preserveAlpha=\"" << m_preserveAlpha << "\"]\n";
    inputEffect(0)->externalRepresentation(ts, indent + 1);
    return ts;
}

}; // namespace WebCore

#endif // ENABLE(FILTERS)
