/*
 * Copyright (C) 2012 University of Szeged
 * Copyright (C) 2012 Tamas Czene <tczene@inf.u-szeged.hu>
 * All rights reserved.
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
#include "config.h"

#if ENABLE(FILTERS) && ENABLE(OPENCL)
#include "FETurbulence.h"

#include "FETurbulence.cpp"
#include "FilterContextOpenCL.h"
#include "SVGFilter.h"

namespace WebCore {

static const char* turbulenceKernelProgram =
PROGRAM(
__constant int s_perlinNoise = 4096;
__constant int s_blockSize = 256;
__constant int s_blockMask = 255;

typedef struct {
    int noisePositionIntegerValue;
    float noisePositionFractionValue;
} Noise;

typedef struct {
    int width;
    int wrapX;
    int height;
    int wrapY;
} StitchData;

float linearInterpolation(float t, float a, float b)
{
    return mad(b - a, t, a);
}

float noise2D(__constant float *component, __constant int *latticeSelector, StitchData stitchData, float noiseVectorX, float noiseVectorY, int stitchTiles)
{
    Noise noiseX;
    noiseX.noisePositionIntegerValue = (int)(noiseVectorX + s_perlinNoise);
    noiseX.noisePositionFractionValue = (noiseVectorX + s_perlinNoise) - noiseX.noisePositionIntegerValue;
    Noise noiseY;
    noiseY.noisePositionIntegerValue = (int)(noiseVectorY + s_perlinNoise);
    noiseY.noisePositionFractionValue = (noiseVectorY + s_perlinNoise) - noiseY.noisePositionIntegerValue;

    // If stitching, adjust lattice points accordingly.
    if (stitchTiles) {
        if (noiseX.noisePositionIntegerValue >= stitchData.wrapX)
            noiseX.noisePositionIntegerValue -= stitchData.width;
        if (noiseX.noisePositionIntegerValue >= stitchData.wrapX - 1)
            noiseX.noisePositionIntegerValue -= stitchData.width - 1;
        if (noiseY.noisePositionIntegerValue >= stitchData.wrapY)
            noiseY.noisePositionIntegerValue -= stitchData.height;
        if (noiseY.noisePositionIntegerValue >= stitchData.wrapY - 1)
            noiseY.noisePositionIntegerValue -= stitchData.height - 1;
    }

    noiseX.noisePositionIntegerValue &= s_blockMask;
    noiseY.noisePositionIntegerValue &= s_blockMask;
    int latticeIndex = latticeSelector[noiseX.noisePositionIntegerValue];
    int nextLatticeIndex = latticeSelector[(noiseX.noisePositionIntegerValue + 1) & s_blockMask];

    float sx = noiseX.noisePositionFractionValue * noiseX.noisePositionFractionValue * (3 - 2 * noiseX.noisePositionFractionValue);
    float sy = noiseY.noisePositionFractionValue * noiseY.noisePositionFractionValue * (3 - 2 * noiseY.noisePositionFractionValue);

    // This is taken 1:1 from SVG spec: http://www.w3.org/TR/SVG11/filters.html#feTurbulenceElement.
    int temp = latticeSelector[latticeIndex + noiseY.noisePositionIntegerValue];
    float u = noiseX.noisePositionFractionValue * component[temp * 2] + noiseY.noisePositionFractionValue * component[temp * 2 + 1];
    temp = latticeSelector[nextLatticeIndex + noiseY.noisePositionIntegerValue];
    float v = (noiseX.noisePositionFractionValue - 1) * component[temp * 2] + noiseY.noisePositionFractionValue * component[temp * 2 + 1];
    float a = linearInterpolation(sx, u, v);
    temp = latticeSelector[latticeIndex + noiseY.noisePositionIntegerValue + 1];
    u = noiseX.noisePositionFractionValue * component[temp * 2] + (noiseY.noisePositionFractionValue - 1) * component[temp * 2 + 1];
    temp = latticeSelector[nextLatticeIndex + noiseY.noisePositionIntegerValue + 1];
    v = (noiseX.noisePositionFractionValue - 1) * component[temp * 2] + (noiseY.noisePositionFractionValue - 1) * component[temp * 2 + 1];
    float b = linearInterpolation(sx, u, v);
    return linearInterpolation(sy, a, b);
}

__kernel void Turbulence(__write_only image2d_t destination, __constant float *transform, __constant float *redComponent,
    __constant float *greenComponent, __constant float *blueComponent, __constant float *alphaComponent,
    __constant int *latticeSelector, __private int offsetX, __private int offsetY, __private int tileWidth,
    __private int tileHeight, __private float baseFrequencyX, __private float baseFrequencyY, __private int stitchTiles,
    __private int numOctaves, __private int type, __private int filter_height)
{
    StitchData stitchData = { 0, 0, 0, 0 };
    // Adjust the base frequencies if necessary for stitching.
    if (stitchTiles) {
        // When stitching tiled turbulence, the frequencies must be adjusted
        // so that the tile borders will be continuous.
        if (baseFrequencyX) {
            float lowFrequency = floor(tileWidth * baseFrequencyX) / tileWidth;
            float highFrequency = ceil(tileWidth * baseFrequencyX) / tileWidth;
            // BaseFrequency should be non-negative according to the standard.
            baseFrequencyX = (baseFrequencyX / lowFrequency < highFrequency / baseFrequencyX) ? lowFrequency : highFrequency;
        }
        if (baseFrequencyY) {
            float lowFrequency = floor(tileHeight * baseFrequencyY) / tileHeight;
            float highFrequency = ceil(tileHeight * baseFrequencyY) / tileHeight;
            baseFrequencyY = (baseFrequencyY / lowFrequency < highFrequency / baseFrequencyY) ? lowFrequency : highFrequency;
        }
        // Set up TurbulenceInitial stitch values.
        stitchData.width = round(tileWidth * baseFrequencyX);
        stitchData.wrapX = s_perlinNoise + stitchData.width;
        stitchData.height = round(tileHeight * baseFrequencyY);
        stitchData.wrapY = s_perlinNoise + stitchData.height;
    }
    float4 turbulenceFunctionResult = (float4)(0, 0, 0, 0);
    float x = (get_global_id(0) + offsetX) * baseFrequencyX;
    float y = (get_global_id(1) + offsetY) * baseFrequencyY;

    float noiseVectorX = transform[0] * x + transform[2] * y + transform[4];
    float noiseVectorY = transform[1] * x + transform[3] * y + transform[5];

    float ratio = 1;
    for (int octave = 0; octave < numOctaves; ++octave) {
        float4 noise2DResult = (float4)( noise2D(redComponent, latticeSelector, stitchData, noiseVectorX, noiseVectorY, stitchTiles) / ratio,
    noise2D(greenComponent, latticeSelector, stitchData, noiseVectorX, noiseVectorY, stitchTiles) / ratio,
    noise2D(blueComponent, latticeSelector, stitchData, noiseVectorX, noiseVectorY, stitchTiles) / ratio,
    noise2D(alphaComponent, latticeSelector, stitchData, noiseVectorX, noiseVectorY, stitchTiles) / ratio);

        turbulenceFunctionResult += (type == 1) ? noise2DResult : fabs(noise2DResult);

        noiseVectorX *= 2;
        noiseVectorY *= 2;
        ratio *= 2;
        if (stitchTiles) {
            // Update stitch values. Subtracting s_perlinNoiseoise before the multiplication and
            // adding it afterward simplifies to subtracting it once.
            stitchData.width *= 2;
            stitchData.wrapX = 2 * stitchData.wrapX - s_perlinNoise;
            stitchData.height *= 2;
            stitchData.wrapY = 2 * stitchData.wrapY - s_perlinNoise;
        }
    }

    if (type == 1)
        turbulenceFunctionResult = mad(0.5f, turbulenceFunctionResult, 0.5f);
    // Clamp result.
    turbulenceFunctionResult = clamp(turbulenceFunctionResult, 0.0f, 1.0f);

    write_imagef(destination, (int2)(get_global_id(0), get_global_id(1)), turbulenceFunctionResult);
}
); // End of OpenCL kernels

inline bool FilterContextOpenCL::compileFETurbulence()
{
    if (m_turbulenceWasCompiled || inError())
        return !inError();

    m_turbulenceWasCompiled = true;

    if (isResourceAllocationFailed((m_turbulenceProgram = compileProgram(turbulenceKernelProgram))))
        return false;
    if (isResourceAllocationFailed((m_turbulenceOperation = kernelByName(m_turbulenceProgram, "Turbulence"))))
        return false;
    return true;
}

inline void FilterContextOpenCL::applyFETurbulence(OpenCLHandle destination,
    IntSize destinationSize, int totalBlockSize,
    void* transform, void* redComponent, void* greenComponent,
    void* blueComponent, void* alphaComponent,
    int* latticeSelector, int offsetX, int offsetY, int tileWidth, int tileHeight,
    float baseFrequencyX, float baseFrequencyY, bool stitchTiles, int numOctaves, int type)
{
    RunKernel kernel(this, m_turbulenceOperation, destinationSize.width(), destinationSize.height());

    kernel.addArgument(destination);
    OpenCLHandle transformHandle(kernel.addArgument(transform, sizeof(float) * 6));
    OpenCLHandle redComponentHandle(kernel.addArgument(redComponent, sizeof(float) * totalBlockSize * 2));
    OpenCLHandle greenComponentHandle(kernel.addArgument(greenComponent, sizeof(float) * totalBlockSize * 2));
    OpenCLHandle blueComponentHandle(kernel.addArgument(blueComponent, sizeof(float) * totalBlockSize * 2));
    OpenCLHandle alphaComponentHandle(kernel.addArgument(alphaComponent, sizeof(float) * totalBlockSize * 2));
    OpenCLHandle latticeSelectorHandle(kernel.addArgument(latticeSelector, sizeof(int) * totalBlockSize));
    kernel.addArgument(offsetX);
    kernel.addArgument(offsetY);
    kernel.addArgument(tileWidth);
    kernel.addArgument(tileHeight);
    kernel.addArgument(baseFrequencyX);
    kernel.addArgument(baseFrequencyY);
    kernel.addArgument(stitchTiles);
    kernel.addArgument(numOctaves);
    kernel.addArgument(type);
    kernel.addArgument(destinationSize.height());

    kernel.run();

    transformHandle.clear();
    redComponentHandle.clear();
    greenComponentHandle.clear();
    blueComponentHandle.clear();
    alphaComponentHandle.clear();
    latticeSelectorHandle.clear();
}

bool FETurbulence::platformApplyOpenCL()
{
    FilterContextOpenCL* context = FilterContextOpenCL::context();
    if (!context)
        return false;

    if (!context->compileFETurbulence())
        return true;

    OpenCLHandle destination = createOpenCLImageResult();

    PaintingData paintingData(m_seed, roundedIntSize(filterPrimitiveSubregion().size()));
    initPaint(paintingData);

    AffineTransform invertedTransform = reinterpret_cast<SVGFilter*>(filter())->absoluteTransform().inverse();
    float transformComponents[6] = { invertedTransform.a(), invertedTransform.b(), invertedTransform.c(), invertedTransform.d(), invertedTransform.e(), invertedTransform.f() };

    context->applyFETurbulence(destination, absolutePaintRect().size(), 2 * s_blockSize + 2, transformComponents, paintingData.gradient,
        paintingData.gradient + 1, paintingData.gradient + 2, paintingData.gradient + 3, paintingData.latticeSelector,
        absolutePaintRect().x(), absolutePaintRect().y(), paintingData.filterSize.width(), paintingData.filterSize.height(),
        m_baseFrequencyX, m_baseFrequencyY, m_stitchTiles, m_numOctaves, m_type);

    return true;
}

} // namespace WebCore

#endif // ENABLE(FILTERS) && ENABLE(OPENCL)
