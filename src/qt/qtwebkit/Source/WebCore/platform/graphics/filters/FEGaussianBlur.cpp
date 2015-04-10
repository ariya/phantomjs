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

#include <wtf/MathExtras.h>
#include <wtf/ParallelJobs.h>
#include <wtf/Uint8ClampedArray.h>

using namespace std;

static inline float gaussianKernelFactor()
{
    return 3 / 4.f * sqrtf(2 * piFloat);
}

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

inline void boxBlur(Uint8ClampedArray* srcPixelArray, Uint8ClampedArray* dstPixelArray,
                    unsigned dx, int dxLeft, int dxRight, int stride, int strideLine, int effectWidth, int effectHeight, bool alphaImage)
{
    for (int y = 0; y < effectHeight; ++y) {
        int line = y * strideLine;
        for (int channel = 3; channel >= 0; --channel) {
            int sum = 0;
            // Fill the kernel
            int maxKernelSize = min(dxRight, effectWidth);
            for (int i = 0; i < maxKernelSize; ++i)
                sum += srcPixelArray->item(line + i * stride + channel);

            // Blurring
            for (int x = 0; x < effectWidth; ++x) {
                int pixelByteOffset = line + x * stride + channel;
                dstPixelArray->set(pixelByteOffset, static_cast<unsigned char>(sum / dx));
                if (x >= dxLeft)
                    sum -= srcPixelArray->item(pixelByteOffset - dxLeft * stride);
                if (x + dxRight < effectWidth)
                    sum += srcPixelArray->item(pixelByteOffset + dxRight * stride);
            }
            if (alphaImage) // Source image is black, it just has different alpha values
                break;
        }
    }
}

inline void FEGaussianBlur::platformApplyGeneric(Uint8ClampedArray* srcPixelArray, Uint8ClampedArray* tmpPixelArray, unsigned kernelSizeX, unsigned kernelSizeY, IntSize& paintSize)
{
    int stride = 4 * paintSize.width();
    int dxLeft = 0;
    int dxRight = 0;
    int dyLeft = 0;
    int dyRight = 0;
    Uint8ClampedArray* src = srcPixelArray;
    Uint8ClampedArray* dst = tmpPixelArray;

    for (int i = 0; i < 3; ++i) {
        if (kernelSizeX) {
            kernelPosition(i, kernelSizeX, dxLeft, dxRight);
#if HAVE(ARM_NEON_INTRINSICS)
            if (!isAlphaImage())
                boxBlurNEON(src, dst, kernelSizeX, dxLeft, dxRight, 4, stride, paintSize.width(), paintSize.height());
            else
                boxBlur(src, dst, kernelSizeX, dxLeft, dxRight, 4, stride, paintSize.width(), paintSize.height(), true);
#else
            boxBlur(src, dst, kernelSizeX, dxLeft, dxRight, 4, stride, paintSize.width(), paintSize.height(), isAlphaImage());
#endif
            swap(src, dst);
        }

        if (kernelSizeY) {
            kernelPosition(i, kernelSizeY, dyLeft, dyRight);
#if HAVE(ARM_NEON_INTRINSICS)
            if (!isAlphaImage())
                boxBlurNEON(src, dst, kernelSizeY, dyLeft, dyRight, stride, 4, paintSize.height(), paintSize.width());
            else
                boxBlur(src, dst, kernelSizeY, dyLeft, dyRight, stride, 4, paintSize.height(), paintSize.width(), true);
#else
            boxBlur(src, dst, kernelSizeY, dyLeft, dyRight, stride, 4, paintSize.height(), paintSize.width(), isAlphaImage());
#endif
            swap(src, dst);
        }
    }

    // The final result should be stored in srcPixelArray.
    if (dst == srcPixelArray) {
        ASSERT(src->length() == dst->length());
        memcpy(dst->data(), src->data(), src->length());
    }

}

void FEGaussianBlur::platformApplyWorker(PlatformApplyParameters* parameters)
{
    IntSize paintSize(parameters->width, parameters->height);
    parameters->filter->platformApplyGeneric(parameters->srcPixelArray.get(), parameters->dstPixelArray.get(),
        parameters->kernelSizeX, parameters->kernelSizeY, paintSize);
}

inline void FEGaussianBlur::platformApply(Uint8ClampedArray* srcPixelArray, Uint8ClampedArray* tmpPixelArray, unsigned kernelSizeX, unsigned kernelSizeY, IntSize& paintSize)
{
    int scanline = 4 * paintSize.width();
    int extraHeight = 3 * kernelSizeY * 0.5f;
    int optimalThreadNumber = (paintSize.width() * paintSize.height()) / (s_minimalRectDimension + extraHeight * paintSize.width());

    if (optimalThreadNumber > 1) {
        WTF::ParallelJobs<PlatformApplyParameters> parallelJobs(&platformApplyWorker, optimalThreadNumber);

        int jobs = parallelJobs.numberOfJobs();
        if (jobs > 1) {
            // Split the job into "blockHeight"-sized jobs but there a few jobs that need to be slightly larger since
            // blockHeight * jobs < total size. These extras are handled by the remainder "jobsWithExtra".
            const int blockHeight = paintSize.height() / jobs;
            const int jobsWithExtra = paintSize.height() % jobs;

            int currentY = 0;
            for (int job = 0; job < jobs; job++) {
                PlatformApplyParameters& params = parallelJobs.parameter(job);
                params.filter = this;

                int startY = !job ? 0 : currentY - extraHeight;
                currentY += job < jobsWithExtra ? blockHeight + 1 : blockHeight;
                int endY = job == jobs - 1 ? currentY : currentY + extraHeight;

                int blockSize = (endY - startY) * scanline;
                if (!job) {
                    params.srcPixelArray = srcPixelArray;
                    params.dstPixelArray = tmpPixelArray;
                } else {
                    params.srcPixelArray = Uint8ClampedArray::createUninitialized(blockSize);
                    params.dstPixelArray = Uint8ClampedArray::createUninitialized(blockSize);
                    memcpy(params.srcPixelArray->data(), srcPixelArray->data() + startY * scanline, blockSize);
                }

                params.width = paintSize.width();
                params.height = endY - startY;
                params.kernelSizeX = kernelSizeX;
                params.kernelSizeY = kernelSizeY;
            }

            parallelJobs.execute();

            // Copy together the parts of the image.
            currentY = 0;
            for (int job = 1; job < jobs; job++) {
                PlatformApplyParameters& params = parallelJobs.parameter(job);
                int sourceOffset;
                int destinationOffset;
                int size;
                int adjustedBlockHeight = job < jobsWithExtra ? blockHeight + 1 : blockHeight;

                currentY += adjustedBlockHeight;
                sourceOffset = extraHeight * scanline;
                destinationOffset = currentY * scanline;
                size = adjustedBlockHeight * scanline;

                memcpy(srcPixelArray->data() + destinationOffset, params.srcPixelArray->data() + sourceOffset, size);
            }
            return;
        }
        // Fallback to single threaded mode.
    }

    // The selection here eventually should happen dynamically on some platforms.
    platformApplyGeneric(srcPixelArray, tmpPixelArray, kernelSizeX, kernelSizeY, paintSize);
}

void FEGaussianBlur::calculateUnscaledKernelSize(unsigned& kernelSizeX, unsigned& kernelSizeY, float stdX, float stdY)
{
    ASSERT(stdX >= 0 && stdY >= 0);

    kernelSizeX = 0;
    if (stdX)
        kernelSizeX = max<unsigned>(2, static_cast<unsigned>(floorf(stdX * gaussianKernelFactor() + 0.5f)));
    kernelSizeY = 0;
    if (stdY)
        kernelSizeY = max<unsigned>(2, static_cast<unsigned>(floorf(stdY * gaussianKernelFactor() + 0.5f)));
    
    // Limit the kernel size to 1000. A bigger radius won't make a big difference for the result image but
    // inflates the absolute paint rect to much. This is compatible with Firefox' behavior.
    if (kernelSizeX > gMaxKernelSize)
        kernelSizeX = gMaxKernelSize;
    if (kernelSizeY > gMaxKernelSize)
        kernelSizeY = gMaxKernelSize;
}

void FEGaussianBlur::calculateKernelSize(Filter* filter, unsigned& kernelSizeX, unsigned& kernelSizeY, float stdX, float stdY)
{
    stdX = filter->applyHorizontalScale(stdX);
    stdY = filter->applyVerticalScale(stdY);

    calculateUnscaledKernelSize(kernelSizeX, kernelSizeY, stdX, stdY);
}

void FEGaussianBlur::determineAbsolutePaintRect()
{
    unsigned kernelSizeX = 0;
    unsigned kernelSizeY = 0;
    calculateKernelSize(filter(), kernelSizeX, kernelSizeY, m_stdX, m_stdY);

    FloatRect absolutePaintRect = inputEffect(0)->absolutePaintRect();

    // We take the half kernel size and multiply it with three, because we run box blur three times.
    absolutePaintRect.inflateX(3 * kernelSizeX * 0.5f);
    absolutePaintRect.inflateY(3 * kernelSizeY * 0.5f);

    if (clipsToBounds())
        absolutePaintRect.intersect(maxEffectRect());
    else
        absolutePaintRect.unite(maxEffectRect());

    setAbsolutePaintRect(enclosingIntRect(absolutePaintRect));
}

void FEGaussianBlur::platformApplySoftware()
{
    FilterEffect* in = inputEffect(0);

    Uint8ClampedArray* srcPixelArray = createPremultipliedImageResult();
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
    RefPtr<Uint8ClampedArray> tmpImageData = Uint8ClampedArray::createUninitialized(paintSize.width() * paintSize.height() * 4);
    Uint8ClampedArray* tmpPixelArray = tmpImageData.get();

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
    return max((radius * 2 / 3.f - 0.5f) / gaussianKernelFactor(), 0.f);
}

} // namespace WebCore

#endif // ENABLE(FILTERS)
