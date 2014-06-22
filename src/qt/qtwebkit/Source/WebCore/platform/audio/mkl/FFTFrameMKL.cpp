/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// FFTFrame implementation using Intel's Math Kernel Library (MKL),
// suitable for use on Windows and Linux.

#include "config.h"

#if ENABLE(WEB_AUDIO)

#if !OS(DARWIN) && USE(WEBAUDIO_MKL)

#include "FFTFrame.h"

#include "mkl_vml.h"
#include <wtf/MathExtras.h>

namespace {

DFTI_DESCRIPTOR_HANDLE createDescriptorHandle(int fftSize)
{
    DFTI_DESCRIPTOR_HANDLE handle = 0;

    // Create DFTI descriptor for 1D single precision transform.
    MKL_LONG status = DftiCreateDescriptor(&handle, DFTI_SINGLE, DFTI_REAL, 1, fftSize);
    ASSERT(DftiErrorClass(status, DFTI_NO_ERROR));

    // Set placement of result to DFTI_NOT_INPLACE.
    status = DftiSetValue(handle, DFTI_PLACEMENT, DFTI_NOT_INPLACE);
    ASSERT(DftiErrorClass(status, DFTI_NO_ERROR));

    // Set packing format to PERM; this produces the layout which
    // matches Accelerate.framework's on the Mac, though interleaved.
    status = DftiSetValue(handle, DFTI_PACKED_FORMAT, DFTI_PERM_FORMAT);
    ASSERT(DftiErrorClass(status, DFTI_NO_ERROR));

    // Set the forward scale factor to 2 to match Accelerate.framework's.
    // FIXME: FFTFrameMac's scaling factor could be fixed to be 1.0,
    // in which case this code would need to be changed as well.
    status = DftiSetValue(handle, DFTI_FORWARD_SCALE, 2.0);
    ASSERT(DftiErrorClass(status, DFTI_NO_ERROR));

    // Set the backward scale factor to 1 / 2n to match Accelerate.framework's.
    // FIXME: if the above scaling factor is fixed then this needs to be as well.
    double scale = 1.0 / (2.0 * fftSize);
    status = DftiSetValue(handle, DFTI_BACKWARD_SCALE, scale);
    ASSERT(DftiErrorClass(status, DFTI_NO_ERROR));

    // Use the default DFTI_CONJUGATE_EVEN_STORAGE = DFTI_COMPLEX_REAL.

    // Commit DFTI descriptor.
    status = DftiCommitDescriptor(handle);
    ASSERT(DftiErrorClass(status, DFTI_NO_ERROR));

    return handle;
}

} // anonymous namespace

namespace WebCore {

const int kMaxFFTPow2Size = 24;

DFTI_DESCRIPTOR_HANDLE* FFTFrame::descriptorHandles = 0;

// Normal constructor: allocates for a given fftSize.
FFTFrame::FFTFrame(unsigned fftSize)
    : m_FFTSize(fftSize)
    , m_log2FFTSize(static_cast<unsigned>(log2(fftSize)))
    , m_handle(0)
    , m_complexData(fftSize)
    , m_realData(fftSize / 2)
    , m_imagData(fftSize / 2)
{
    // We only allow power of two.
    ASSERT(1UL << m_log2FFTSize == m_FFTSize);

    m_handle = descriptorHandleForSize(fftSize);
}

// Creates a blank/empty frame (interpolate() must later be called).
FFTFrame::FFTFrame()
    : m_FFTSize(0)
    , m_log2FFTSize(0)
    , m_handle(0)
{
}

// Copy constructor.
FFTFrame::FFTFrame(const FFTFrame& frame)
    : m_FFTSize(frame.m_FFTSize)
    , m_log2FFTSize(frame.m_log2FFTSize)
    , m_handle(0)
    , m_complexData(frame.m_FFTSize)
    , m_realData(frame.m_FFTSize / 2)
    , m_imagData(frame.m_FFTSize / 2)
{
    m_handle = descriptorHandleForSize(m_FFTSize);

    // Copy/setup frame data.
    unsigned nbytes = sizeof(float) * (m_FFTSize / 2);
    memcpy(realData(), frame.realData(), nbytes);
    memcpy(imagData(), frame.imagData(), nbytes);
}

FFTFrame::~FFTFrame()
{
}

void FFTFrame::multiply(const FFTFrame& frame)
{
    FFTFrame& frame1 = *this;
    FFTFrame& frame2 = const_cast<FFTFrame&>(frame);

    float* realP1 = frame1.realData();
    float* imagP1 = frame1.imagData();
    const float* realP2 = frame2.realData();
    const float* imagP2 = frame2.imagData();

    // Scale accounts for vecLib's peculiar scaling.
    // This ensures the right scaling all the way back to inverse FFT.
    // FIXME: this scaling factor will be 1.0f if the above 2.0 -> 1.0
    // scaling factor is fixed.
    float scale = 0.5f;

    // Multiply packed DC/nyquist component.
    realP1[0] *= scale * realP2[0];
    imagP1[0] *= scale * imagP2[0];

    // Multiply the rest, skipping packed DC/Nyquist components.
    float* interleavedData1 = frame1.getUpToDateComplexData();
    float* interleavedData2 = frame2.getUpToDateComplexData();

    unsigned halfSize = m_FFTSize / 2;

    // Complex multiply.
    vcMul(halfSize - 1,
          reinterpret_cast<MKL_Complex8*>(interleavedData1) + 1,
          reinterpret_cast<MKL_Complex8*>(interleavedData2) + 1,
          reinterpret_cast<MKL_Complex8*>(interleavedData1) + 1);

    // De-interleave and scale the rest of the data.
    // FIXME: find an MKL routine to do at least the scaling more efficiently.
    for (unsigned i = 1; i < halfSize; ++i) {
        int baseComplexIndex = 2 * i;
        realP1[i] = scale * interleavedData1[baseComplexIndex];
        imagP1[i] = scale * interleavedData1[baseComplexIndex + 1];
    }
}

void FFTFrame::doFFT(const float* data)
{
    // Compute Forward transform.
    MKL_LONG status = DftiComputeForward(m_handle, data, m_complexData.data());
    ASSERT_UNUSED(status, DftiErrorClass(status, DFTI_NO_ERROR));

    // De-interleave to separate real and complex arrays. FIXME:
    // figure out if it's possible to get MKL to use split-complex
    // form for 1D real-to-complex out-of-place FFTs.
    int len = m_FFTSize / 2;
    for (int i = 0; i < len; ++i) {
        int baseComplexIndex = 2 * i;
        // m_realData[0] is the DC component and m_imagData[0] the
        // Nyquist component since the interleaved complex data is
        // packed.
        m_realData[i] = m_complexData[baseComplexIndex];
        m_imagData[i] = m_complexData[baseComplexIndex + 1];
    }
}

void FFTFrame::doInverseFFT(float* data)
{
    // Prepare interleaved data. FIXME: figure out if it's possible to
    // get MKL to use split-complex form for 1D backward
    // (complex-to-real) out-of-place FFTs.
    float* interleavedData = getUpToDateComplexData();

    // Compute backward transform.
    MKL_LONG status = DftiComputeBackward(m_handle, interleavedData, data);
    ASSERT_UNUSED(status, DftiErrorClass(status, DFTI_NO_ERROR));
}

void FFTFrame::initialize()
{
}

void FFTFrame::cleanup()
{
    if (!descriptorHandles)
        return;

    for (int i = 0; i < kMaxFFTPow2Size; ++i) {
        if (descriptorHandles[i]) {
            MKL_LONG status = DftiFreeDescriptor(&descriptorHandles[i]);
            ASSERT_UNUSED(status, DftiErrorClass(status, DFTI_NO_ERROR));
        }
    }

    delete[] descriptorHandles;
    descriptorHandles = 0;
}

float* FFTFrame::realData() const
{
    return const_cast<float*>(m_realData.data());
}

float* FFTFrame::imagData() const
{
    return const_cast<float*>(m_imagData.data());
}

float* FFTFrame::getUpToDateComplexData()
{
    // FIXME: if we can't completely get rid of this method, SSE
    // optimization could be considered if it shows up hot on profiles.
    int len = m_FFTSize / 2;
    for (int i = 0; i < len; ++i) {
        int baseComplexIndex = 2 * i;
        m_complexData[baseComplexIndex] = m_realData[i];
        m_complexData[baseComplexIndex + 1] = m_imagData[i];
    }
    return const_cast<float*>(m_complexData.data());
}

DFTI_DESCRIPTOR_HANDLE FFTFrame::descriptorHandleForSize(unsigned fftSize)
{
    if (!descriptorHandles) {
        descriptorHandles = new DFTI_DESCRIPTOR_HANDLE[kMaxFFTPow2Size];
        for (int i = 0; i < kMaxFFTPow2Size; ++i)
            descriptorHandles[i] = 0;
    }

    ASSERT(fftSize);
    int pow2size = static_cast<int>(log2(fftSize));
    ASSERT(pow2size < kMaxFFTPow2Size);
    if (!descriptorHandles[pow2size])
        descriptorHandles[pow2size] = createDescriptorHandle(fftSize);
    return descriptorHandles[pow2size];
}

} // namespace WebCore

#endif // !OS(DARWIN) && USE(WEBAUDIO_MKL)

#endif // ENABLE(WEB_AUDIO)
