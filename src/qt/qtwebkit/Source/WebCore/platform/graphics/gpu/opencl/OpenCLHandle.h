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
#if ENABLE(OPENCL)

#ifndef OpenCLHandle_h
#define OpenCLHandle_h

#include "CL/cl.h"

namespace WebCore {

class OpenCLHandle {
public:
    OpenCLHandle() : m_openCLMemory(0) { }
    OpenCLHandle(cl_mem openCLMemory) : m_openCLMemory(openCLMemory) { }

    operator cl_mem() { return m_openCLMemory; }

    void operator=(OpenCLHandle openCLMemory) { m_openCLMemory = openCLMemory; }

    // This conversion operator allows implicit conversion to bool but not to other integer types.
    typedef cl_mem (OpenCLHandle::*UnspecifiedBoolType);
    operator UnspecifiedBoolType() const { return m_openCLMemory ? &OpenCLHandle::m_openCLMemory : 0; }

    void* handleAddress() { return reinterpret_cast<void*>(&m_openCLMemory); }

    void clear()
    {
        if (m_openCLMemory)
            clReleaseMemObject(m_openCLMemory);
        m_openCLMemory = 0;
    }

private:
    cl_mem m_openCLMemory;
};

}

#endif
#endif // ENABLE(OPENCL)
