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
#include "FEColorMatrix.h"

#include "FilterContextOpenCL.h"

namespace WebCore {

#define COLOR_MATRIX_KERNEL(...) \
        int2 sourceCoord = (int2) (get_global_id(0) + x, get_global_id(1) + y); \
        float4 sourcePixel = read_imagef(source, sampler, sourceCoord); \
        float4 destinationPixel = (float4) (__VA_ARGS__); \
        write_imagef(destination, (int2) (get_global_id(0), get_global_id(1)), destinationPixel);

static const char* colorMatrixKernelProgram =
PROGRAM(
const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

__kernel void matrix(__write_only image2d_t destination, __read_only image2d_t source, float x, float y, __constant float *values)
{
    COLOR_MATRIX_KERNEL(values[0] * sourcePixel.x + values[1] * sourcePixel.y + values[2] * sourcePixel.z + values[3] * sourcePixel.w + values[4],
        values[5] * sourcePixel.x + values[6] * sourcePixel.y + values[7] * sourcePixel.z + values[8] * sourcePixel.w + values[9],
        values[10] * sourcePixel.x + values[11] * sourcePixel.y + values[12] * sourcePixel.z + values[13] * sourcePixel.w + values[14],
        values[15] * sourcePixel.x + values[16] * sourcePixel.y + values[17] * sourcePixel.z + values[18] * sourcePixel.w + values[19])
}

__kernel void saturateAndHueRotate(__write_only image2d_t destination, __read_only image2d_t source, float x, float y, __constant float *components)
{
    COLOR_MATRIX_KERNEL(sourcePixel.x * components[0] + sourcePixel.y * components[1] + sourcePixel.z * components[2],
        sourcePixel.x * components[3] + sourcePixel.y * components[4] + sourcePixel.z * components[5],
        sourcePixel.x * components[6] + sourcePixel.y * components[7] + sourcePixel.z * components[8],
        sourcePixel.w)
}

__kernel void luminance(__write_only image2d_t destination, __read_only image2d_t source, float x, float y)
{
    COLOR_MATRIX_KERNEL(0, 0, 0, 0.2125 * sourcePixel.x + 0.7154 * sourcePixel.y + 0.0721 * sourcePixel.z)
}
); // End of OpenCL kernels

inline bool FilterContextOpenCL::compileFEColorMatrix()
{
    if (m_colorMatrixWasCompiled || inError())
        return !inError();

    m_colorMatrixWasCompiled = true;

    if (isResourceAllocationFailed((m_colorMatrixProgram = compileProgram(colorMatrixKernelProgram))))
        return false;
    if (isResourceAllocationFailed((m_matrixOperation = kernelByName(m_colorMatrixProgram, "matrix"))))
        return false;
    if (isResourceAllocationFailed((m_saturateAndHueRotateOperation = kernelByName(m_colorMatrixProgram, "saturateAndHueRotate"))))
        return false;
    if (isResourceAllocationFailed((m_saturateAndHueRotateOperation = kernelByName(m_colorMatrixProgram, "saturateAndHueRotate"))))
        return false;
    if (isResourceAllocationFailed((m_luminanceOperation = kernelByName(m_colorMatrixProgram, "luminance"))))
        return false;
    return true;
}

inline void FilterContextOpenCL::applyFEColorMatrix(OpenCLHandle destination, IntSize destinationSize, OpenCLHandle source, IntPoint relativeSourceLocation, float* values, int type)
{
    cl_kernel colorMatrix;
    OpenCLHandle clValues;

    switch (type) {
    case FECOLORMATRIX_TYPE_MATRIX:
        colorMatrix = m_matrixOperation;
        break;
    case FECOLORMATRIX_TYPE_SATURATE:
        colorMatrix = m_saturateAndHueRotateOperation;
        break;
    case FECOLORMATRIX_TYPE_HUEROTATE:
        colorMatrix = m_saturateAndHueRotateOperation;
        break;
    case FECOLORMATRIX_TYPE_LUMINANCETOALPHA:
        colorMatrix = m_luminanceOperation;
        break;
    default:
        ASSERT_NOT_REACHED();
        return;
    }

    RunKernel kernel(this, colorMatrix, destinationSize.width(), destinationSize.height());
    kernel.addArgument(destination);
    kernel.addArgument(source);
    kernel.addArgument(relativeSourceLocation.x());
    kernel.addArgument(relativeSourceLocation.y());
    if (type == FECOLORMATRIX_TYPE_MATRIX)
        clValues = kernel.addArgument(values, sizeof(float) * 20);
    else if (type == FECOLORMATRIX_TYPE_SATURATE || type == FECOLORMATRIX_TYPE_HUEROTATE)
        clValues = kernel.addArgument(values, sizeof(float) * 9);
    kernel.run();

    clValues.clear();
}

bool FEColorMatrix::platformApplyOpenCL()
{
    FilterContextOpenCL* context = FilterContextOpenCL::context();
    if (!context)
        return false;

    if (!context->compileFEColorMatrix())
        return true;

    FilterEffect* in = inputEffect(0);
    OpenCLHandle source = in->openCLImage();
    OpenCLHandle destination = createOpenCLImageResult();

    IntPoint relativeSourceLocation(
        absolutePaintRect().x() - in->absolutePaintRect().location().x(),
        absolutePaintRect().y() - in->absolutePaintRect().location().y());

    float components[9];
    if (FECOLORMATRIX_TYPE_SATURATE == m_type)
        calculateSaturateComponents(components, m_values[0]);
    else if (FECOLORMATRIX_TYPE_HUEROTATE == m_type)
        calculateHueRotateComponents(components, m_values[0]);

    context->applyFEColorMatrix(destination, absolutePaintRect().size(), source, relativeSourceLocation, (FECOLORMATRIX_TYPE_MATRIX == m_type) ? m_values.data() : components, m_type);

    return true;
}

} // namespace WebCore

#endif // ENABLE(FILTERS) && ENABLE(OPENCL)
