/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Igalia, S.L.
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
#include "FEGaussianBlur.h"

#include "FEGaussianBlurNEON.h"
#include "Filter.h"
#include "GraphicsContext.h"
#include "RenderTreeAsText.h"
#include "TextStream.h"

#include <wtf/ByteArray.h>
#include <wtf/MathExtras.h>

using std::max;

static const float gGaussianKernelFactor = 3 / 4.f * sqrtf(2 * piFloat);
static const unsigned gMaxKernelSize = 1000;

namespace WebCore {

FEGaussianBlur::FEGaussianBlur(Filter* filter, float x, float y)
    : FilterEffect(filter)
    , m_stdX(x)
    , m_stdY(y)
{
}

PassRefPtr<FEGaussianBlur> FEGaussianBlur::create(Filter* filter, float x, float y)
{
    return adoptRef(new FEGaussianBlur(filter, x, y));
}

float FEGaussianBlur::stdDeviationX() const
{
    return m_stdX;
}

void FEGaussianBlur::setStdDeviationX(float x)
{
    m_stdX = x;
}

float FEGaussianBlur::stdDeviationY() const
{
    return m_stdY;
}

void FEGaussianBlur::setStdDeviationY(float y)
{
    m_stdY = y;
}

inline void boxBlur(ByteArray* srcPixelArray, ByteArray* dstPixelArray,
                    unsigned dx, int dxLeft, int dxRight, int stride, int strideLine, int effectWidth, int effectHeight, bool alphaImage)
{
    for (int y = 0; y < effectHeight; ++y) {
        int line = y * strideLine;
        for (int channel = 3; channel >= 0; --channel) {
            int sum = 0;
            // Fill the kernel
            int maxKernelSize = std::min(dxRight, effectWidth);
            for (int i = 0; i < maxKernelSize; ++i)
                sum += srcPixelArray->get(line + i * stride + channel);

            // Blurring
            for (int x = 0; x < effectWidth; ++x) {
                int pixelByteOffset = line + x * stride + channel;
                dstPixelArray->set(pixelByteOffset, static_cast<unsigned char>(sum / dx));
                if (x >= dxLeft)
                    sum -= srcPixelArray->get(pixelByteOffset - dxLeft * stride);
                if (x + dxRight < effectWidth)
                    sum += srcPixelArray->get(pixelByteOffset + dxRight * stride);
            }
            if (alphaImage) // Source image is black, it just has different alpha values
                break;
        }
    }
}

inline void FEGaussianBlur::platformApplyGeneric(ByteArray* srcPixelArray, ByteArray* tmpPixelArray, unsigned kernelSizeX, unsigned kernelSizeY, IntSize& paintSize)
{
    int stride = 4 * paintSize.width();
    int dxLeft = 0;
    int dxRight = 0;
    int dyLeft = 0;
    int dyRight = 0;
    for (int i = 0; i < 3; ++i) {
        if (kernelSizeX) {
            kernelPosition(i, kernelSizeX, dxLeft, dxRight);
            boxBlur(srcPixelArray, tmpPixelArray, kernelSizeX, dxLeft, dxRight, 4, stride, paintSize.width(), paintSize.height(), isAlphaImage());
        } else {
            ByteArray* auxPixelArray = tmpPixelArray;
            tmpPixelArray = srcPixelArray;
            srcPixelArray = auxPixelArray;
        }

        if (kernelSizeY) {
            kernelPosition(i, kernelSizeY, dyLeft, dyRight);
            boxBlur(tmpPixelArray, srcPixelArray, kernelSizeY, dyLeft, dyRight, stride, 4, paintSize.height(), paintSize.width(), isAlphaImage());
        } else {
            ByteArray* auxPixelArray = tmpPixelArray;
            tmpPixelArray = srcPixelArray;
            srcPixelArray = auxPixelArray;
        }
    }
}

inline void FEGaussianBlur::platformApply(ByteArray* srcPixelArray, ByteArray* tmpPixelArray, unsigned kernelSizeX, unsigned kernelSizeY, IntSize& paintSize)
{
    // The selection here eventually should happen dynamically on some platforms.
#if CPU(ARM_NEON) && COMPILER(GCC)
    platformApplyNeon(srcPixelArray, tmpPixelArray, kernelSizeX, kernelSizeY, paintSize);
#else
    platformApplyGeneric(srcPixelArray, tmpPixelArray, kernelSizeX, kernelSizeY, paintSize);
#endif
}

void FEGaussianBlur::calculateKernelSize(Filter* filter, unsigned& kernelSizeX, unsigned& kernelSizeY, float stdX, float stdY)
{
    stdX = filter->applyHorizontalScale(stdX);
    stdY = filter->applyVerticalScale(stdY);
    
    kernelSizeX = 0;
    if (stdX)
        kernelSizeX = max<unsigned>(2, static_cast<unsigned>(floorf(stdX * gGaussianKernelFactor + 0.5f)));
    kernelSizeY = 0;
    if (stdY)
        kernelSizeY = max<unsigned>(2, static_cast<unsigned>(floorf(stdY * gGaussianKernelFactor + 0.5f)));
    
    // Limit the kernel size to 1000. A bigger radius won't make a big difference for the result image but
    // inflates the absolute paint rect to much. This is compatible with Firefox' behavior.
    if (kernelSizeX > gMaxKernelSize)
        kernelSizeX = gMaxKernelSize;
    if (kernelSizeY > gMaxKernelSize)
        kernelSizeY = gMaxKernelSize;
}

void FEGaussianBlur::determineAbsolutePaintRect()
{
    FloatRect absolutePaintRect = inputEffect(0)->absolutePaintRect();
    absolutePaintRect.intersect(maxEffectRect());

    unsigned kernelSizeX = 0;
    unsigned kernelSizeY = 0;
    calculateKernelSize(filter(), kernelSizeX, kernelSizeY, m_stdX, m_stdY);

    // We take the half kernel size and multiply it with three, because we run box blur three times.
    absolutePaintRect.inflateX(3 * kernelSizeX * 0.5f);
    absolutePaintRect.inflateY(3 * kernelSizeY * 0.5f);
    setAbsolutePaintRect(enclosingIntRect(absolutePaintRect));
}

void FEGaussianBlur::apply()
{
    if (hasResult())
        return;
    FilterEffect* in = inputEffect(0);
    in->apply();
    if (!in->hasResult())
        return;

    ByteArray* srcPixelArray = createPremultipliedImageResult();
    if (!srcPixelArray)
        return;

    setIsAlphaImage(in->isAlphaImage());

    IntRect effectDrawingRect = requestedRegionOfInputImageData(in->absolutePaintRect());
    in->copyPremultipliedImage(srcPixelArray, effectDrawingRect);

    if (!m_stdX && !m_stdY)
        return;

    unsigned kernelSizeX = 0;
    unsigned kernelSizeY = 0;
    calculateKernelSize(filter(), kernelSizeX, kernelSizeY, m_stdX, m_stdY);

    IntSize paintSize = absolutePaintRect().size();
    RefPtr<ByteArray> tmpImageData = ByteArray::create(paintSize.width() * paintSize.height() * 4);
    ByteArray* tmpPixelArray = tmpImageData.get();

    platformApply(srcPixelArray, tmpPixelArray, kernelSizeX, kernelSizeY, paintSize);
}

void FEGaussianBlur::dump()
{
}

TextStream& FEGaussianBlur::externalRepresentation(TextStream& ts, int indent) const
{
    writeIndent(ts, indent);
    ts << "[feGaussianBlur";
    FilterEffect::externalRepresentation(ts);
    ts << " stdDeviation=\"" << m_stdX << ", " << m_stdY << "\"]\n";
    inputEffect(0)->externalRepresentation(ts, indent + 1);
    return ts;
}

float FEGaussianBlur::calculateStdDeviation(float radius)
{
    // Blur radius represents 2/3 times the kernel size, the dest pixel is half of the radius applied 3 times
    return max((radius * 2 / 3.f - 0.5f) / gGaussianKernelFactor, 0.f);
}

} // namespace WebCore

#endif // ENABLE(FILTERS)
