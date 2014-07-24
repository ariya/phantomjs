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

#if ENABLE(OPENCL)
#include "FilterContextOpenCL.h"

namespace WebCore {

FilterContextOpenCL* FilterContextOpenCL::m_context = 0;
int FilterContextOpenCL::m_alreadyInitialized = 0;

FilterContextOpenCL* FilterContextOpenCL::context()
{
    if (m_context)
        return m_context;
    if (m_alreadyInitialized)
        return 0;

    m_alreadyInitialized = true;
    FilterContextOpenCL* localContext = new FilterContextOpenCL();

    // Initializing the context.
    cl_int errorNumber;
    cl_device_id* devices;
    cl_platform_id firstPlatformId;
    size_t deviceBufferSize = 0;

    errorNumber = clGetPlatformIDs(1, &firstPlatformId, 0);
    cl_context_properties contextProperties[] = { CL_CONTEXT_PLATFORM, (cl_context_properties)firstPlatformId, 0};
    localContext->m_deviceContext = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_GPU, 0, 0, &errorNumber);
    if (errorNumber != CL_SUCCESS) {
        localContext->m_deviceContext = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_CPU, 0, 0, &errorNumber);
        if (errorNumber != CL_SUCCESS)
            return 0;
    }

    errorNumber = clGetContextInfo(localContext->m_deviceContext, CL_CONTEXT_DEVICES, 0, 0, &deviceBufferSize);
    if (errorNumber != CL_SUCCESS)
        return 0;

    if (!deviceBufferSize)
        return 0;

    devices = reinterpret_cast<cl_device_id*>(fastMalloc(deviceBufferSize));
    errorNumber = clGetContextInfo(localContext->m_deviceContext, CL_CONTEXT_DEVICES, deviceBufferSize, devices, 0);
    if (errorNumber != CL_SUCCESS)
        return 0;

    localContext->m_commandQueue = clCreateCommandQueue(localContext->m_deviceContext, devices[0], 0, 0);
    if (!localContext->m_commandQueue)
        return 0;

    localContext->m_deviceId = devices[0];
    fastFree(devices);

    cl_bool imageSupport = CL_FALSE;
    clGetDeviceInfo(localContext->m_deviceId, CL_DEVICE_IMAGE_SUPPORT, sizeof(cl_bool), &imageSupport, 0);
    if (imageSupport != CL_TRUE)
        return 0;

    m_context = localContext;
    return m_context;
}

void FilterContextOpenCL::freeResources()
{
    clFinish(m_commandQueue);

    if (m_colorMatrixWasCompiled) {
        freeResource(m_matrixOperation);
        freeResource(m_saturateAndHueRotateOperation);
        freeResource(m_luminanceOperation);
        freeResource(m_colorMatrixProgram);
    }
    m_colorMatrixWasCompiled = false;

    if (m_turbulenceWasCompiled) {
        freeResource(m_turbulenceOperation);
        freeResource(m_turbulenceProgram);
    }    
    m_turbulenceWasCompiled = false;

    if (m_transformColorSpaceWasCompiled) {
        freeResource(m_transformColorSpaceKernel);
        freeResource(m_transformColorSpaceProgram);
    }
    m_transformColorSpaceWasCompiled = false;
}

void FilterContextOpenCL::destroyContext()
{
    freeResources();

    if (m_commandQueue)
        clReleaseCommandQueue(m_commandQueue);
    m_commandQueue = 0;

    if (m_deviceContext)
        clReleaseContext(m_deviceContext);
    m_deviceContext = 0;

    m_context = 0;
}

OpenCLHandle FilterContextOpenCL::createOpenCLImage(IntSize paintSize)
{
    FilterContextOpenCL* context = FilterContextOpenCL::context();

    cl_image_format clImageFormat;
    clImageFormat.image_channel_order = CL_RGBA;
    clImageFormat.image_channel_data_type = CL_UNORM_INT8;

#ifdef CL_API_SUFFIX__VERSION_1_2
    cl_image_desc imageDescriptor = { CL_MEM_OBJECT_IMAGE2D, paintSize.width(), paintSize.height(), 0, 0, 0, 0, 0, 0, 0};
    OpenCLHandle image = clCreateImage(context->deviceContext(), CL_MEM_READ_WRITE, &clImageFormat, &imageDescriptor, 0, 0);
#else
    OpenCLHandle image = clCreateImage2D(context->deviceContext(), CL_MEM_READ_WRITE, &clImageFormat,
        paintSize.width(), paintSize.height(), 0, 0, 0);
#endif
    return image;
}

static const char* transformColorSpaceKernelProgram =
PROGRAM(
const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

__kernel void transformColorSpace(__read_only image2d_t source, __write_only image2d_t destination, __constant float *clLookUpTable)
{
    int2 sourceCoord = (int2) (get_global_id(0), get_global_id(1));
    float4 pixel = read_imagef(source, sampler, sourceCoord);

    pixel = (float4) (clLookUpTable[(int)(round(pixel.x * 255))], clLookUpTable[(int)(round(pixel.y * 255))],
        clLookUpTable[(int) (round(pixel.z * 255))], pixel.w);

    write_imagef(destination, sourceCoord, pixel);
}
); // End of OpenCL kernels

inline bool FilterContextOpenCL::compileTransformColorSpaceProgram()
{
    if (m_transformColorSpaceWasCompiled || inError())
        return !inError();

    m_transformColorSpaceWasCompiled = true;

    if (isResourceAllocationFailed((m_transformColorSpaceProgram = compileProgram(transformColorSpaceKernelProgram))))
        return false;
    if (isResourceAllocationFailed((m_transformColorSpaceKernel = kernelByName(m_transformColorSpaceProgram, "transformColorSpace"))))
        return false;
    return true;
}

void FilterContextOpenCL::openCLTransformColorSpace(OpenCLHandle& source, IntRect sourceSize, ColorSpace srcColorSpace, ColorSpace dstColorSpace)
{
    DEFINE_STATIC_LOCAL(OpenCLHandle, deviceRgbLUT, ());
    DEFINE_STATIC_LOCAL(OpenCLHandle, linearRgbLUT, ());

    if (srcColorSpace == dstColorSpace || inError())
        return;

    if ((srcColorSpace != ColorSpaceLinearRGB && srcColorSpace != ColorSpaceDeviceRGB)
        || (dstColorSpace != ColorSpaceLinearRGB && dstColorSpace != ColorSpaceDeviceRGB))
        return;

    if (!compileTransformColorSpaceProgram())
        return;

    OpenCLHandle destination = createOpenCLImage(sourceSize.size());

    RunKernel kernel(this, m_transformColorSpaceKernel, sourceSize.width(), sourceSize.height());
    kernel.addArgument(source);
    kernel.addArgument(destination);

    if (dstColorSpace == ColorSpaceLinearRGB) {
        if (!linearRgbLUT) {
            Vector<float> lookUpTable;
            for (unsigned i = 0; i < 256; i++) {
                float color = i  / 255.0f;
                color = (color <= 0.04045f ? color / 12.92f : pow((color + 0.055f) / 1.055f, 2.4f));
                color = std::max(0.0f, color);
                color = std::min(1.0f, color);
                lookUpTable.append((round(color * 255)) / 255);
            }
            linearRgbLUT = kernel.addArgument(lookUpTable.data(), sizeof(float) * 256);
        } else
            kernel.addArgument(linearRgbLUT);
    } else if (dstColorSpace == ColorSpaceDeviceRGB) {
        if (!deviceRgbLUT) {
            Vector<float> lookUpTable;
            for (unsigned i = 0; i < 256; i++) {
                float color = i / 255.0f;
                color = (powf(color, 1.0f / 2.4f) * 1.055f) - 0.055f;
                color = std::max(0.0f, color);
                color = std::min(1.0f, color);
                lookUpTable.append((round(color * 255)) / 255);
            }
            deviceRgbLUT = kernel.addArgument(lookUpTable.data(), sizeof(float) * 256);
        } else
            kernel.addArgument(deviceRgbLUT);
    }

    kernel.run();
    source.clear();
    source = destination;
}

static const char* fillKernelProgram =
PROGRAM_STR(
__kernel void fill(__write_only image2d_t destination, float r, float g, float b, float a)
{
    float4 sourcePixel = (float4)(r, g, b, a);
    write_imagef(destination, (int2)(get_global_id(0), get_global_id(1)), sourcePixel);
}
);

inline bool FilterContextOpenCL::compileFill()
{
    if (m_fillWasCompiled || inError())
        return !inError();

    m_fillWasCompiled = true;

    if (isResourceAllocationFailed((m_fillProgram = compileProgram(fillKernelProgram))))
        return false;
    if (isResourceAllocationFailed((m_fill = kernelByName(m_fillProgram, "fill"))))
        return false;
    return true;
}

void FilterContextOpenCL::fill(cl_mem image, IntSize imageSize, Color color)
{
    if (!m_context || inError())
        return;

    compileFill();

    float r, g, b, a;

    color.getRGBA(r, g, b, a);

    RunKernel kernel(this, m_fill, imageSize.width(), imageSize.height());
    kernel.addArgument(image);
    kernel.addArgument(r);
    kernel.addArgument(g);
    kernel.addArgument(b);
    kernel.addArgument(a);
    kernel.run();
}

cl_program FilterContextOpenCL::compileProgram(const char* source)
{
    cl_program program;
    cl_int errorNumber = 0;

    program = clCreateProgramWithSource(m_deviceContext, 1, (const char**) &source, 0, &errorNumber);
    if (isFailed(errorNumber))
        return 0;

    if (isFailed(clBuildProgram(program, 0, 0, 0, 0, 0)))
        return 0;

    return program;
}

void FilterContextOpenCL::freeResource(cl_kernel& handle)
{
    if (handle) {
        clReleaseKernel(handle);
        handle = 0;
    }
}

void FilterContextOpenCL::freeResource(cl_program& handle)
{
    if (handle) {
        clReleaseProgram(handle);
        handle = 0;
    }
}
} // namespace WebCore

#endif
