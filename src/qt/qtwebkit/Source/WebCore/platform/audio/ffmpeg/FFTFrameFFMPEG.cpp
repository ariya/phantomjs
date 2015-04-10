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

// FFTFrame implementation using FFmpeg's RDFT algorithm,
// suitable for use on Windows and Linux.

#include "config.h"

#if ENABLE(WEB_AUDIO)

#if USE(WEBAUDIO_FFMPEG)

#include "FFTFrame.h"

#include "VectorMath.h"

extern "C" {
    #include <libavcodec/avfft.h>
}

#include <wtf/MathExtras.h>

namespace {

struct FFTComplexProxy {
    int16_t re;
    int16_t im;
};

struct FFTContextProxy {
    int nbits;
    int inverse;
    uint16_t* revtab;
    FFTComplexProxy* tmpBuf;
    int mdctSize;
    int mdctBits;
    void* tcos;
    void* tsin;
    void (*fftPermute)();
    void (*fftCalc)();
    void (*imdctCalc)();
    void (*imdctHalf)();
    void (*mdctCalc)();
    void (*mdctCalcw)();
    int fftPermutation;
    int mdctPermutation;
};

struct RDFTContextProxy {
    int nbits;
    int inverse;
    int signConvention;
    const void* tcos;
    const void* tsin;
    FFTContextProxy fft;
    void (*rdft_calc)();
};

}

namespace WebCore {

const int kMaxFFTPow2Size = 24;

// Normal constructor: allocates for a given fftSize.
FFTFrame::FFTFrame(unsigned fftSize)
    : m_FFTSize(fftSize)
    , m_log2FFTSize(static_cast<unsigned>(log2(fftSize)))
    , m_forwardContext(0)
    , m_inverseContext(0)
    , m_complexData(fftSize)
    , m_realData(fftSize / 2)
    , m_imagData(fftSize / 2)
{
    // We only allow power of two.
    ASSERT(1UL << m_log2FFTSize == m_FFTSize);

    m_forwardContext = contextForSize(fftSize, DFT_R2C);
    m_inverseContext = contextForSize(fftSize, IDFT_C2R);
}

// Creates a blank/empty frame (interpolate() must later be called).
FFTFrame::FFTFrame()
    : m_FFTSize(0)
    , m_log2FFTSize(0)
    , m_forwardContext(0)
    , m_inverseContext(0)
{
}

// Copy constructor.
FFTFrame::FFTFrame(const FFTFrame& frame)
    : m_FFTSize(frame.m_FFTSize)
    , m_log2FFTSize(frame.m_log2FFTSize)
    , m_forwardContext(0)
    , m_inverseContext(0)
    , m_complexData(frame.m_FFTSize)
    , m_realData(frame.m_FFTSize / 2)
    , m_imagData(frame.m_FFTSize / 2)
{
    m_forwardContext = contextForSize(m_FFTSize, DFT_R2C);
    m_inverseContext = contextForSize(m_FFTSize, IDFT_C2R);

    // Copy/setup frame data.
    unsigned nbytes = sizeof(float) * (m_FFTSize / 2);
    memcpy(realData(), frame.realData(), nbytes);
    memcpy(imagData(), frame.imagData(), nbytes);
}

void FFTFrame::initialize()
{
}

void FFTFrame::cleanup()
{
}

FFTFrame::~FFTFrame()
{
    av_rdft_end(m_forwardContext);
    av_rdft_end(m_inverseContext);
}

void FFTFrame::multiply(const FFTFrame& frame)
{
    FFTFrame& frame1 = *this;
    FFTFrame& frame2 = const_cast<FFTFrame&>(frame);

    float* realP1 = frame1.realData();
    float* imagP1 = frame1.imagData();
    const float* realP2 = frame2.realData();
    const float* imagP2 = frame2.imagData();

    unsigned halfSize = fftSize() / 2;
    float real0 = realP1[0];
    float imag0 = imagP1[0];

    VectorMath::zvmul(realP1, imagP1, realP2, imagP2, realP1, imagP1, halfSize); 

    // Multiply the packed DC/nyquist component
    realP1[0] = real0 * realP2[0];
    imagP1[0] = imag0 * imagP2[0];

    // Scale accounts the peculiar scaling of vecLib on the Mac.
    // This ensures the right scaling all the way back to inverse FFT.
    // FIXME: if we change the scaling on the Mac then this scale
    // factor will need to change too.
    float scale = 0.5f;

    VectorMath::vsmul(realP1, 1, &scale, realP1, 1, halfSize);
    VectorMath::vsmul(imagP1, 1, &scale, imagP1, 1, halfSize);
}

void FFTFrame::doFFT(const float* data)
{
    // Copy since processing is in-place.
    float* p = m_complexData.data();
    memcpy(p, data, sizeof(float) * m_FFTSize);

    // Compute Forward transform.
    av_rdft_calc(m_forwardContext, p);

    // De-interleave to separate real and complex arrays.
    int len = m_FFTSize / 2;

    // FIXME: see above comment in multiply() about scaling.
    const float scale = 2.0f;

    for (int i = 0; i < len; ++i) {
        int baseComplexIndex = 2 * i;
        // m_realData[0] is the DC component and m_imagData[0] is the nyquist component
        // since the interleaved complex data is packed.
        m_realData[i] = scale * p[baseComplexIndex];
        m_imagData[i] = scale * p[baseComplexIndex + 1];
    }
}

void FFTFrame::doInverseFFT(float* data)
{
    // Prepare interleaved data.
    float* interleavedData = getUpToDateComplexData();

    // Compute inverse transform.
    av_rdft_calc(m_inverseContext, interleavedData);

    // Scale so that a forward then inverse FFT yields exactly the original data.
    const float scale = 1.0 / m_FFTSize;
    VectorMath::vsmul(interleavedData, 1, &scale, data, 1, m_FFTSize);
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

RDFTContext* FFTFrame::contextForSize(unsigned fftSize, int trans)
{
    // FIXME: This is non-optimal. Ideally, we'd like to share the contexts for FFTFrames of the same size.
    // But FFmpeg's RDFT uses a scratch buffer inside the context and so they are not thread-safe.
    // We could improve this by sharing the FFTFrames on a per-thread basis.
    ASSERT(fftSize);
    int pow2size = static_cast<int>(log2(fftSize));
    ASSERT(pow2size < kMaxFFTPow2Size);

    RDFTContext* context = av_rdft_init(pow2size, (RDFTransformType)trans);
    return context;
}

} // namespace WebCore

#endif // !OS(DARWIN) && USE(WEBAUDIO_FFMPEG)

#endif // ENABLE(WEB_AUDIO)
