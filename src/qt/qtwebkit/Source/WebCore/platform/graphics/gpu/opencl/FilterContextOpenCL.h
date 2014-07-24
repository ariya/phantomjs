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

#ifndef FilterContextOpenCL_h
#define FilterContextOpenCL_h

#if ENABLE(OPENCL)
#include "CL/cl.h"
#include "Color.h"
#include "ColorSpace.h"
#include "IntRect.h"
#include "IntSize.h"
#include "OpenCLHandle.h"

#define PROGRAM_STR(...)  #__VA_ARGS__
#define PROGRAM(...) PROGRAM_STR(__VA_ARGS__)

namespace WebCore {

class FilterContextOpenCL {
public:
    FilterContextOpenCL()
        : m_inError(false)
        , m_deviceId(0)
        , m_deviceContext(0)
        , m_commandQueue(0)
        , m_transformColorSpaceWasCompiled(false)
        , m_transformColorSpaceProgram(0)
        , m_transformColorSpaceKernel(0)
        , m_fillWasCompiled(false)
        , m_fillProgram(0)
        , m_fill(0)
        , m_mergeWasCompiled(false)
        , m_mergeProgram(0)
        , m_mergeCopyOperation(0)
        , m_mergeOperation(0)
        , m_colorMatrixWasCompiled(false)
        , m_colorMatrixProgram(0)
        , m_matrixOperation(0)
        , m_saturateAndHueRotateOperation(0)
        , m_luminanceOperation(0)
        , m_turbulenceWasCompiled(false)
        , m_turbulenceProgram(0)
        , m_turbulenceOperation(0)
    {
    }

    // Returns 0 if initialization failed.
    static FilterContextOpenCL* context();

    cl_device_id deviceId() { return m_deviceId; }
    cl_context deviceContext() { return m_deviceContext; }
    cl_command_queue commandQueue() { return m_commandQueue; }

    inline void setInError(bool errorCode = true) { m_inError = errorCode; }
    inline bool inError() { return m_inError; }
    inline bool isFailed(bool);
    inline bool isResourceAllocationFailed(bool);

    void freeResources();
    void destroyContext();

    OpenCLHandle createOpenCLImage(IntSize);

    inline bool compileFill();
    void fill(cl_mem, IntSize, Color);

    inline bool compileTransformColorSpaceProgram();
    void openCLTransformColorSpace(OpenCLHandle&, IntRect, ColorSpace, ColorSpace);

    inline bool compileFEColorMatrix();
    inline bool compileFETurbulence();
    inline bool compileFEMerge();

    inline void applyFEMergeCopy(OpenCLHandle, IntSize, OpenCLHandle, IntPoint&);
    inline void applyFEMerge(OpenCLHandle, OpenCLHandle, OpenCLHandle, OpenCLHandle, IntSize, IntPoint&, IntPoint&);
    inline void applyFEColorMatrix(OpenCLHandle, IntSize, OpenCLHandle, IntPoint, float*, int);
    inline void applyFETurbulence(OpenCLHandle, IntSize, int, void*, void*, void*, void*, void*,
        int*, int, int, int, int, float, float, bool, int, int);

private:

    class RunKernel {
        public:
            RunKernel(FilterContextOpenCL* context, cl_kernel kernel, size_t width, size_t height)
                : m_context(context)
                , m_kernel(kernel)
                , m_index(0)
                , m_error(context->inError())
            {
                m_globalSize[0] = width;
                m_globalSize[1] = height;
            }

            void addArgument(OpenCLHandle handle)
            {
                if (!m_error)
                    m_error = clSetKernelArg(m_kernel, m_index++, sizeof(OpenCLHandle), handle.handleAddress());
            }

            void addArgument(cl_int value)
            {
                if (!m_error)
                    m_error = clSetKernelArg(m_kernel, m_index++, sizeof(cl_int), reinterpret_cast<void*>(&value));
            }

            void addArgument(cl_float value)
            {
                if (!m_error)
                    m_error = clSetKernelArg(m_kernel, m_index++, sizeof(cl_float), reinterpret_cast<void*>(&value));
            }

            void addArgument(cl_sampler handle)
            {
                if (!m_error)
                    m_error = clSetKernelArg(m_kernel, m_index++, sizeof(cl_sampler), reinterpret_cast<void*>(&handle));
            }

            OpenCLHandle addArgument(void* buffer, int size)
            {
                if (m_error)
                    return 0;
                OpenCLHandle handle(clCreateBuffer(m_context->deviceContext(), CL_MEM_READ_ONLY, size, 0, &m_error));
                if (m_error)
                    return 0;
                m_error = clEnqueueWriteBuffer(m_context->commandQueue(), handle, CL_TRUE, 0, size, buffer, 0, 0, 0);
                if (m_error)
                    return 0;
                m_error = clSetKernelArg(m_kernel, m_index++, sizeof(OpenCLHandle), handle.handleAddress());
                return !m_error ? handle : 0;
            }

            void run()
            {
                if (m_context->isFailed(m_error))
                    return;

                m_error = clFinish(m_context->m_commandQueue);
                if (!m_error)
                    m_error = clEnqueueNDRangeKernel(m_context->m_commandQueue, m_kernel, 2, 0, m_globalSize, 0, 0, 0, 0);
                m_context->isFailed(m_error);
            }

            FilterContextOpenCL* m_context;
            cl_kernel m_kernel;
            size_t m_globalSize[2];
            int m_index;
            int m_error;
        };

    cl_program compileProgram(const char*);
    static inline cl_kernel kernelByName(cl_program program, const char* name) { return clCreateKernel(program, name, 0); }

    static inline void freeResource(cl_kernel&);
    static inline void freeResource(cl_program&);

    static FilterContextOpenCL* m_context;
    static int m_alreadyInitialized;
    bool m_inError;

    cl_device_id m_deviceId;
    cl_context m_deviceContext;
    cl_command_queue m_commandQueue;

    bool m_transformColorSpaceWasCompiled;
    cl_program m_transformColorSpaceProgram;
    cl_kernel m_transformColorSpaceKernel;

    bool m_fillWasCompiled;
    cl_program m_fillProgram;
    cl_kernel m_fill;

    bool m_mergeWasCompiled;
    cl_program m_mergeProgram;
    cl_kernel m_mergeCopyOperation;
    cl_kernel m_mergeOperation;

    bool m_colorMatrixWasCompiled;
    cl_program m_colorMatrixProgram;
    cl_kernel m_matrixOperation;
    cl_kernel m_saturateAndHueRotateOperation;
    cl_kernel m_luminanceOperation;

    bool m_turbulenceWasCompiled;
    cl_program m_turbulenceProgram;
    cl_kernel m_turbulenceOperation;
};

inline bool FilterContextOpenCL::isFailed(bool value)
{
    if (value)
        setInError();
    return value;
}

inline bool FilterContextOpenCL::isResourceAllocationFailed(bool value)
{
    if (!value)
        setInError();
    return !value;
}

} // namespace WebCore

#endif // ENABLE(OPENCL)

#endif
