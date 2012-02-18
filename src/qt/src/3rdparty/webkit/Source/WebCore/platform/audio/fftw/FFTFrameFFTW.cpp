/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

// FFTFrame implementation using the FFTW library.

#include "config.h"

#if ENABLE(WEB_AUDIO)

#if !OS(DARWIN) && USE(WEBAUDIO_FFTW)

#include "FFTFrame.h"

#include <wtf/MathExtras.h>

namespace WebCore {

const int kMaxFFTPow2Size = 24;

fftwf_plan* FFTFrame::fftwForwardPlans = 0;
fftwf_plan* FFTFrame::fftwBackwardPlans = 0;

Mutex* FFTFrame::s_planLock = 0;

namespace {

unsigned unpackedFFTWDataSize(unsigned fftSize)
{
    return fftSize / 2 + 1;
}

} // anonymous namespace


// Normal constructor: allocates for a given fftSize.
FFTFrame::FFTFrame(unsigned fftSize)
    : m_FFTSize(fftSize)
    , m_log2FFTSize(static_cast<unsigned>(log2(fftSize)))
    , m_forwardPlan(0)
    , m_backwardPlan(0)
    , m_data(2 * (3 + unpackedFFTWDataSize(fftSize))) // enough space for real and imaginary data plus 16-byte alignment padding
{
    // We only allow power of two.
    ASSERT(1UL << m_log2FFTSize == m_FFTSize);

    // FFTW won't create a plan without being able to look at non-null
    // pointers for the input and output data; it wants to be able to
    // see whether these arrays are aligned properly for vector
    // operations. Ideally we would use fftw_malloc and fftw_free for
    // the input and output arrays to ensure proper alignment for SIMD
    // operations, so that we don't have to specify FFTW_UNALIGNED
    // when creating the plan. However, since we don't have control
    // over the alignment of the array passed to doFFT / doInverseFFT,
    // we would need to memcpy it in to or out of the FFTFrame, adding
    // overhead. For the time being, we just assume unaligned data and
    // pass a temporary pointer down.

    float temporary;
    m_forwardPlan = fftwPlanForSize(fftSize, Forward,
                                    &temporary, realData(), imagData());
    m_backwardPlan = fftwPlanForSize(fftSize, Backward,
                                     realData(), imagData(), &temporary);
}

// Creates a blank/empty frame (interpolate() must later be called).
FFTFrame::FFTFrame()
    : m_FFTSize(0)
    , m_log2FFTSize(0)
    , m_forwardPlan(0)
    , m_backwardPlan(0)
{
}

// Copy constructor.
FFTFrame::FFTFrame(const FFTFrame& frame)
    : m_FFTSize(frame.m_FFTSize)
    , m_log2FFTSize(frame.m_log2FFTSize)
    , m_forwardPlan(0)
    , m_backwardPlan(0)
    , m_data(2 * (3 + unpackedFFTWDataSize(fftSize()))) // enough space for real and imaginary data plus 16-byte alignment padding
{
    // See the normal constructor for an explanation of the temporary pointer.
    float temporary;
    m_forwardPlan = fftwPlanForSize(m_FFTSize, Forward,
                                    &temporary, realData(), imagData());
    m_backwardPlan = fftwPlanForSize(m_FFTSize, Backward,
                                     realData(), imagData(), &temporary);

    // Copy/setup frame data.
    size_t nbytes = sizeof(float) * unpackedFFTWDataSize(fftSize());
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

    // Scale accounts the peculiar scaling of vecLib on the Mac.
    // This ensures the right scaling all the way back to inverse FFT.
    // FIXME: if we change the scaling on the Mac then this scale
    // factor will need to change too.
    float scale = 0.5f;

    // Multiply the packed DC/nyquist component
    realP1[0] *= scale * realP2[0];
    imagP1[0] *= scale * imagP2[0];

    // Complex multiplication. If this loop turns out to be hot then
    // we should use SSE or other intrinsics to accelerate it.
    unsigned halfSize = fftSize() / 2;

    for (unsigned i = 1; i < halfSize; ++i) {
        float realResult = realP1[i] * realP2[i] - imagP1[i] * imagP2[i];
        float imagResult = realP1[i] * imagP2[i] + imagP1[i] * realP2[i];

        realP1[i] = scale * realResult;
        imagP1[i] = scale * imagResult;
    }
}

void FFTFrame::doFFT(float* data)
{
    fftwf_execute_split_dft_r2c(m_forwardPlan, data, realData(), imagData());

    // Scale the frequency domain data to match vecLib's scale factor
    // on the Mac. FIXME: if we change the definition of FFTFrame to
    // eliminate this scale factor then this code will need to change.
    // Also, if this loop turns out to be hot then we should use SSE
    // or other intrinsics to accelerate it.
    float scaleFactor = 2;
    unsigned length = unpackedFFTWDataSize(fftSize());
    float* realData = this->realData();
    float* imagData = this->imagData();

    for (unsigned i = 0; i < length; ++i) {
        realData[i] = realData[i] * scaleFactor;
        imagData[i] = imagData[i] * scaleFactor;
    }

    // Move the Nyquist component to the location expected by the
    // FFTFrame API.
    imagData[0] = realData[length - 1];
}

void FFTFrame::doInverseFFT(float* data)
{
    unsigned length = unpackedFFTWDataSize(fftSize());
    float* realData = this->realData();
    float* imagData = this->imagData();

    // Move the Nyquist component to the location expected by FFTW.
    realData[length - 1] = imagData[0];
    imagData[length - 1] = 0;
    imagData[0] = 0;

    fftwf_execute_split_dft_c2r(m_backwardPlan, realData, imagData, data);

    // Restore the original scaling of the time domain data.
    // FIXME: if we change the definition of FFTFrame to eliminate the
    // scale factor then this code will need to change. Also, if this
    // loop turns out to be hot then we should use SSE or other
    // intrinsics to accelerate it.
    float scaleFactor = 1.0 / (2.0 * fftSize());
    unsigned n = fftSize();
    for (unsigned i = 0; i < n; ++i)
        data[i] *= scaleFactor;

    // Move the Nyquist component back to the location expected by the
    // FFTFrame API.
    imagData[0] = realData[length - 1];
}

void FFTFrame::initialize()
{
    if (!fftwForwardPlans) {
        fftwForwardPlans = new fftwf_plan[kMaxFFTPow2Size];
        fftwBackwardPlans = new fftwf_plan[kMaxFFTPow2Size];
        for (int i = 0; i < kMaxFFTPow2Size; ++i) {
            fftwForwardPlans[i] = 0;
            fftwBackwardPlans[i] = 0;
        }
    }

    if (!s_planLock)
        s_planLock = new Mutex();
}

void FFTFrame::cleanup()
{
    if (!fftwForwardPlans)
        return;

    for (int i = 0; i < kMaxFFTPow2Size; ++i) {
        if (fftwForwardPlans[i])
            fftwf_destroy_plan(fftwForwardPlans[i]);
        if (fftwBackwardPlans[i])
            fftwf_destroy_plan(fftwBackwardPlans[i]);
    }

    delete[] fftwForwardPlans;
    delete[] fftwBackwardPlans;

    fftwForwardPlans = 0;
    fftwBackwardPlans = 0;
    
    delete s_planLock;
    s_planLock = 0;
}

float* FFTFrame::realData() const
{
    return const_cast<float*>(m_data.data());
}

float* FFTFrame::imagData() const
{
    // Imaginary data is stored following the real data with enough padding for 16-byte alignment.
    return const_cast<float*>(realData() + unpackedFFTWDataSize(fftSize()) + 3);
}

fftwf_plan FFTFrame::fftwPlanForSize(unsigned fftSize, Direction direction,
                                     float* data1, float* data2, float* data3)
{
    // initialize() must be called first.
    ASSERT(fftwForwardPlans);
    if (!fftwForwardPlans)
        return 0;
        
    ASSERT(s_planLock);
    if (!s_planLock)
        return 0;
    MutexLocker locker(*s_planLock);    
        
    ASSERT(fftSize);
    int pow2size = static_cast<int>(log2(fftSize));
    ASSERT(pow2size < kMaxFFTPow2Size);
    fftwf_plan* plans = (direction == Forward) ? fftwForwardPlans : fftwBackwardPlans;
    if (!plans[pow2size]) {
        fftwf_iodim dimension;
        dimension.n = fftSize;
        dimension.is = 1;
        dimension.os = 1;

        // For the time being, we do not take the input data into
        // account when choosing a plan, so that we can most easily
        // reuse plans with different input data.

        // FIXME: allocate input and output data inside this class to
        // be able to take advantage of alignment and SIMD optimizations.
        unsigned flags = FFTW_ESTIMATE | FFTW_PRESERVE_INPUT | FFTW_UNALIGNED;
        switch (direction) {
        case Forward:
            plans[pow2size] = fftwf_plan_guru_split_dft_r2c(1, &dimension, 0, 0,
                                                            data1, data2, data3,
                                                            flags);
            break;
        case Backward:
            plans[pow2size] = fftwf_plan_guru_split_dft_c2r(1, &dimension, 0, 0,
                                                            data1, data2, data3,
                                                            flags);
            break;
        }
    }
    ASSERT(plans[pow2size]);
    return plans[pow2size];
}

} // namespace WebCore

#endif // !OS(DARWIN) && USE(WEBAUDIO_FFTW)

#endif // ENABLE(WEB_AUDIO)
