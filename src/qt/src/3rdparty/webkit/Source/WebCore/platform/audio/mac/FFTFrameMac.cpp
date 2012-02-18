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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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

// Mac OS X - specific FFTFrame implementation

#include "config.h"

#if ENABLE(WEB_AUDIO)

#if OS(DARWIN) && !USE(WEBAUDIO_FFMPEG)

#include "FFTFrame.h"

namespace WebCore {

const int kMaxFFTPow2Size = 24;

FFTSetup* FFTFrame::fftSetups = 0;

// Normal constructor: allocates for a given fftSize
FFTFrame::FFTFrame(unsigned fftSize)
    : m_realData(fftSize)
    , m_imagData(fftSize)
{
    m_FFTSize = fftSize;
    m_log2FFTSize = static_cast<unsigned>(log2(fftSize));

    // We only allow power of two
    ASSERT(1UL << m_log2FFTSize == m_FFTSize);

    // Lazily create and share fftSetup with other frames
    m_FFTSetup = fftSetupForSize(fftSize);

    // Setup frame data
    m_frame.realp = m_realData.data();
    m_frame.imagp = m_imagData.data();
}

// Creates a blank/empty frame (interpolate() must later be called)
FFTFrame::FFTFrame()
    : m_realData(0)
    , m_imagData(0)
{
    // Later will be set to correct values when interpolate() is called
    m_frame.realp = 0;
    m_frame.imagp = 0;

    m_FFTSize = 0;
    m_log2FFTSize = 0;
}

// Copy constructor
FFTFrame::FFTFrame(const FFTFrame& frame)
    : m_FFTSize(frame.m_FFTSize)
    , m_log2FFTSize(frame.m_log2FFTSize)
    , m_FFTSetup(frame.m_FFTSetup)
    , m_realData(frame.m_FFTSize)
    , m_imagData(frame.m_FFTSize)
{
    // Setup frame data
    m_frame.realp = m_realData.data();
    m_frame.imagp = m_imagData.data();

    // Copy/setup frame data
    unsigned nbytes = sizeof(float) * m_FFTSize;
    memcpy(realData(), frame.m_frame.realp, nbytes);
    memcpy(imagData(), frame.m_frame.imagp, nbytes);
}

FFTFrame::~FFTFrame()
{
}

void FFTFrame::multiply(const FFTFrame& frame)
{
    FFTFrame& frame1 = *this;
    const FFTFrame& frame2 = frame;

    float* realP1 = frame1.realData();
    float* imagP1 = frame1.imagData();
    const float* realP2 = frame2.realData();
    const float* imagP2 = frame2.imagData();

    // Scale accounts for vecLib's peculiar scaling
    // This ensures the right scaling all the way back to inverse FFT
    float scale = 0.5f;

    // Multiply packed DC/nyquist component
    realP1[0] *= scale * realP2[0];
    imagP1[0] *= scale * imagP2[0];

    // Multiply the rest, skipping packed DC/Nyquist components
    DSPSplitComplex sc1 = frame1.dspSplitComplex();
    sc1.realp++;
    sc1.imagp++;

    DSPSplitComplex sc2 = frame2.dspSplitComplex();
    sc2.realp++;
    sc2.imagp++;

    unsigned halfSize = m_FFTSize / 2;

    // Complex multiply
    vDSP_zvmul(&sc1, 1, &sc2, 1, &sc1, 1, halfSize - 1, 1 /* normal multiplication */);

    // We've previously scaled the packed part, now scale the rest.....
    vDSP_vsmul(sc1.realp, 1, &scale, sc1.realp, 1, halfSize - 1);
    vDSP_vsmul(sc1.imagp, 1, &scale, sc1.imagp, 1, halfSize - 1);
}

void FFTFrame::doFFT(float* data)
{
    vDSP_ctoz((DSPComplex*)data, 2, &m_frame, 1, m_FFTSize / 2);
    vDSP_fft_zrip(m_FFTSetup, &m_frame, 1, m_log2FFTSize, FFT_FORWARD);
}

void FFTFrame::doInverseFFT(float* data)
{
    vDSP_fft_zrip(m_FFTSetup, &m_frame, 1, m_log2FFTSize, FFT_INVERSE);
    vDSP_ztoc(&m_frame, 1, (DSPComplex*)data, 2, m_FFTSize / 2);

    // Do final scaling so that x == IFFT(FFT(x))
    float scale = 0.5f / m_FFTSize;
    vDSP_vsmul(data, 1, &scale, data, 1, m_FFTSize);
}

FFTSetup FFTFrame::fftSetupForSize(unsigned fftSize)
{
    if (!fftSetups) {
        fftSetups = (FFTSetup*)malloc(sizeof(FFTSetup) * kMaxFFTPow2Size);
        memset(fftSetups, 0, sizeof(FFTSetup) * kMaxFFTPow2Size);
    }

    int pow2size = static_cast<int>(log2(fftSize));
    ASSERT(pow2size < kMaxFFTPow2Size);
    if (!fftSetups[pow2size])
        fftSetups[pow2size] = vDSP_create_fftsetup(pow2size, FFT_RADIX2);

    return fftSetups[pow2size];
}

void FFTFrame::initialize()
{
}

void FFTFrame::cleanup()
{
    if (!fftSetups)
        return;

    for (int i = 0; i < kMaxFFTPow2Size; ++i) {
        if (fftSetups[i])
            vDSP_destroy_fftsetup(fftSetups[i]);
    }

    free(fftSetups);
    fftSetups = 0;
}

float* FFTFrame::realData() const
{
    return m_frame.realp;
}
    
float* FFTFrame::imagData() const
{
    return m_frame.imagp;
}

} // namespace WebCore

#endif // #if OS(DARWIN) && !USE(WEBAUDIO_FFMPEG)

#endif // ENABLE(WEB_AUDIO)
