/*
 *  Copyright (C) 2012 Igalia S.L
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

// FFTFrame implementation using the GStreamer FFT library.

#include "config.h"

#if USE(WEBAUDIO_GSTREAMER)

#include "FFTFrame.h"

#include "VectorMath.h"
#include <wtf/FastAllocBase.h>

namespace {

size_t unpackedFFTDataSize(unsigned fftSize)
{
    return fftSize / 2 + 1;
}

} // anonymous namespace

namespace WebCore {

// Normal constructor: allocates for a given fftSize.
FFTFrame::FFTFrame(unsigned fftSize)
    : m_FFTSize(fftSize)
    , m_log2FFTSize(static_cast<unsigned>(log2(fftSize)))
    , m_realData(unpackedFFTDataSize(m_FFTSize))
    , m_imagData(unpackedFFTDataSize(m_FFTSize))
{
    m_complexData = WTF::fastNewArray<GstFFTF32Complex>(unpackedFFTDataSize(m_FFTSize));

    int fftLength = gst_fft_next_fast_length(m_FFTSize);
    m_fft = gst_fft_f32_new(fftLength, FALSE);
    m_inverseFft = gst_fft_f32_new(fftLength, TRUE);
}

// Creates a blank/empty frame (interpolate() must later be called).
FFTFrame::FFTFrame()
    : m_FFTSize(0)
    , m_log2FFTSize(0)
    , m_complexData(0)
{
    int fftLength = gst_fft_next_fast_length(m_FFTSize);
    m_fft = gst_fft_f32_new(fftLength, FALSE);
    m_inverseFft = gst_fft_f32_new(fftLength, TRUE);
}

// Copy constructor.
FFTFrame::FFTFrame(const FFTFrame& frame)
    : m_FFTSize(frame.m_FFTSize)
    , m_log2FFTSize(frame.m_log2FFTSize)
    , m_realData(unpackedFFTDataSize(frame.m_FFTSize))
    , m_imagData(unpackedFFTDataSize(frame.m_FFTSize))
{
    m_complexData = WTF::fastNewArray<GstFFTF32Complex>(unpackedFFTDataSize(m_FFTSize));

    int fftLength = gst_fft_next_fast_length(m_FFTSize);
    m_fft = gst_fft_f32_new(fftLength, FALSE);
    m_inverseFft = gst_fft_f32_new(fftLength, TRUE);

    // Copy/setup frame data.
    memcpy(realData(), frame.realData(), sizeof(float) * unpackedFFTDataSize(m_FFTSize));
    memcpy(imagData(), frame.imagData(), sizeof(float) * unpackedFFTDataSize(m_FFTSize));
}

void FFTFrame::initialize()
{
}

void FFTFrame::cleanup()
{
}

FFTFrame::~FFTFrame()
{
    if (!m_fft)
        return;

    gst_fft_f32_free(m_fft);
    m_fft = 0;

    gst_fft_f32_free(m_inverseFft);
    m_inverseFft = 0;

    WTF::fastDeleteArray(m_complexData);
}

void FFTFrame::multiply(const FFTFrame& frame)
{
    FFTFrame& frame1 = *this;
    FFTFrame& frame2 = const_cast<FFTFrame&>(frame);

    float* realP1 = frame1.realData();
    float* imagP1 = frame1.imagData();
    const float* realP2 = frame2.realData();
    const float* imagP2 = frame2.imagData();

    size_t size = unpackedFFTDataSize(m_FFTSize);
    VectorMath::zvmul(realP1, imagP1, realP2, imagP2, realP1, imagP1, size);

    // Scale accounts the peculiar scaling of vecLib on the Mac.
    // This ensures the right scaling all the way back to inverse FFT.
    // FIXME: if we change the scaling on the Mac then this scale
    // factor will need to change too.
    float scale = 0.5f;

    VectorMath::vsmul(realP1, 1, &scale, realP1, 1, size);
    VectorMath::vsmul(imagP1, 1, &scale, imagP1, 1, size);
}

void FFTFrame::doFFT(const float* data)
{
    gst_fft_f32_fft(m_fft, data, m_complexData);

    // Scale the frequency domain data to match vecLib's scale factor
    // on the Mac. FIXME: if we change the definition of FFTFrame to
    // eliminate this scale factor then this code will need to change.
    // Also, if this loop turns out to be hot then we should use SSE
    // or other intrinsics to accelerate it.
    float scaleFactor = 2;

    float* imagData = m_imagData.data();
    float* realData = m_realData.data();
    for (unsigned i = 0; i < unpackedFFTDataSize(m_FFTSize); ++i) {
        imagData[i] = m_complexData[i].i * scaleFactor;
        realData[i] = m_complexData[i].r * scaleFactor;
    }
}

void FFTFrame::doInverseFFT(float* data)
{
    //  Merge the real and imaginary vectors to complex vector.
    float* realData = m_realData.data();
    float* imagData = m_imagData.data();

    for (size_t i = 0; i < unpackedFFTDataSize(m_FFTSize); ++i) {
        m_complexData[i].i = imagData[i];
        m_complexData[i].r = realData[i];
    }

    gst_fft_f32_inverse_fft(m_inverseFft, m_complexData, data);

    // Scale so that a forward then inverse FFT yields exactly the original data.
    const float scaleFactor = 1.0 / (2 * m_FFTSize);
    VectorMath::vsmul(data, 1, &scaleFactor, data, 1, m_FFTSize);
}

float* FFTFrame::realData() const
{
    return const_cast<float*>(m_realData.data());
}

float* FFTFrame::imagData() const
{
    return const_cast<float*>(m_imagData.data());
}

} // namespace WebCore

#endif // USE(WEBAUDIO_GSTREAMER)
