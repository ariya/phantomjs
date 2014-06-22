/*
 * Copyright (C) 2013 University of Szeged
 * Copyright (C) 2013 Tamas Czene <tczene@inf.u-szeged.hu>
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
#include "FEMerge.h"

#include "FilterContextOpenCL.h"

namespace WebCore {

static const char* mergeKernelProgram =
PROGRAM(
const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

__kernel void copy(__write_only image2d_t destination, __read_only image2d_t source, int x, int y)
{
    float4 destinationPixel = read_imagef(source, sampler, (int2) (get_global_id(0) + x, get_global_id(1) + y));
    write_imagef(destination, (int2) (get_global_id(0), get_global_id(1)), destinationPixel);
}

__kernel void merge(__write_only image2d_t destination, __read_only image2d_t previousDestination, __read_only image2d_t sourceA, __read_only image2d_t sourceB, int xA, int yA, int xB, int yB)
{
    int2 destinationCoord = (int2) (get_global_id(0), get_global_id(1));
    int2 sourceCoordA = (int2) (destinationCoord.x + xA, destinationCoord.y + yA);
    int2 sourceCoordB = (int2) (destinationCoord.x + xB, destinationCoord.y + yB);
    float4 destinationPixel = read_imagef(previousDestination, sampler, destinationCoord);
    float4 sourcePixelA = read_imagef(sourceA, sampler, sourceCoordA);
    float4 sourcePixelB = read_imagef(sourceB, sampler, sourceCoordB);

    destinationPixel = sourcePixelA + destinationPixel * (1 - sourcePixelA.w);
    destinationPixel = sourcePixelB + destinationPixel * (1 - sourcePixelB.w);

    write_imagef(destination, destinationCoord, destinationPixel);
}
); // End of OpenCL kernels

inline bool FilterContextOpenCL::compileFEMerge()
{
    if (m_mergeWasCompiled || inError())
        return !inError();

    m_mergeWasCompiled = true;

    if (isResourceAllocationFailed((m_mergeProgram = compileProgram(mergeKernelProgram))))
        return false;
    if (isResourceAllocationFailed((m_mergeCopyOperation = kernelByName(m_mergeProgram, "copy"))))
        return false;
    if (isResourceAllocationFailed((m_mergeOperation = kernelByName(m_mergeProgram, "merge"))))
        return false;
    return true;
}

inline void FilterContextOpenCL::applyFEMergeCopy(OpenCLHandle destination, IntSize destinationSize, OpenCLHandle source, IntPoint& relativeSourcePoint)
{
    RunKernel kernel(this, m_mergeCopyOperation, destinationSize.width(), destinationSize.height());
    kernel.addArgument(destination);
    kernel.addArgument(source);
    kernel.addArgument(relativeSourcePoint.x());
    kernel.addArgument(relativeSourcePoint.y());
    kernel.run();
}

inline void FilterContextOpenCL::applyFEMerge(OpenCLHandle destination, OpenCLHandle previousDestination, OpenCLHandle sourceA, OpenCLHandle sourceB, IntSize destinationSize, IntPoint& relativeSourcePointA, IntPoint& relativeSourcePointB)
{
    RunKernel kernel(this, m_mergeOperation, destinationSize.width(), destinationSize.height());
    kernel.addArgument(destination);
    kernel.addArgument(previousDestination);
    kernel.addArgument(sourceA);
    kernel.addArgument(sourceB);
    kernel.addArgument(relativeSourcePointA.x());
    kernel.addArgument(relativeSourcePointA.y());
    kernel.addArgument(relativeSourcePointB.x());
    kernel.addArgument(relativeSourcePointB.y());
    kernel.run();
}

bool FEMerge::platformApplyOpenCL()
{
    FilterContextOpenCL* context = FilterContextOpenCL::context();
    if (!context)
        return false;

    context->compileFEMerge();

    unsigned size = numberOfEffectInputs();
    ASSERT(size > 0);

    OpenCLHandle destination = createOpenCLImageResult();
    OpenCLHandle sourceA = 0;
    OpenCLHandle sourceB = 0;
    FilterEffect* in;

    int i = 0;

    if (size & 1) {
        in = inputEffect(i++);
        sourceA = in->openCLImage();
        IntPoint relativeSourcePoint(in->absolutePaintRect().location());
        relativeSourcePoint.setX(absolutePaintRect().x() - relativeSourcePoint.x());
        relativeSourcePoint.setY(absolutePaintRect().y() - relativeSourcePoint.y());

        context->applyFEMergeCopy(destination, absolutePaintRect().size(), sourceA, relativeSourcePoint);
        if (size == 1)
            return true;
    } else
        context->fill(destination, absolutePaintRect().size(), Color(0.0f, 0.0f, 0.0f, 0.0f));

    OpenCLHandle previousDestination = context->createOpenCLImage(absolutePaintRect().size());

    while (i < size) {
        OpenCLHandle temp = previousDestination;
        previousDestination = destination;
        destination = temp;

        in = inputEffect(i++);
        sourceA = in->openCLImage();
        IntPoint relativeSourcePointA(in->absolutePaintRect().location());
        relativeSourcePointA.setX(absolutePaintRect().x() - relativeSourcePointA.x());
        relativeSourcePointA.setY(absolutePaintRect().y() - relativeSourcePointA.y());

        in = inputEffect(i++);
        sourceB = in->openCLImage();
        IntPoint relativeSourcePointB(in->absolutePaintRect().location());
        relativeSourcePointB.setX(absolutePaintRect().x() - relativeSourcePointB.x());
        relativeSourcePointB.setY(absolutePaintRect().y() - relativeSourcePointB.y());

        context->applyFEMerge(destination, previousDestination, sourceA, sourceB, absolutePaintRect().size(), relativeSourcePointA, relativeSourcePointB);
    }
    setOpenCLImage(destination);
    previousDestination.clear();
    return true;
}

} // namespace WebCore

#endif // ENABLE(FILTERS) && ENABLE(OPENCL)
