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

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "FFTFrame.h"

#include "Logging.h"
#include <complex>
#include <wtf/MathExtras.h>
#include <wtf/OwnPtr.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

namespace WebCore {

void FFTFrame::doPaddedFFT(const float* data, size_t dataSize)
{
    // Zero-pad the impulse response
    AudioFloatArray paddedResponse(fftSize()); // zero-initialized
    paddedResponse.copyToRange(data, 0, dataSize);

    // Get the frequency-domain version of padded response
    doFFT(paddedResponse.data());
}

PassOwnPtr<FFTFrame> FFTFrame::createInterpolatedFrame(const FFTFrame& frame1, const FFTFrame& frame2, double x)
{
    OwnPtr<FFTFrame> newFrame = adoptPtr(new FFTFrame(frame1.fftSize()));

    newFrame->interpolateFrequencyComponents(frame1, frame2, x);

    // In the time-domain, the 2nd half of the response must be zero, to avoid circular convolution aliasing...
    int fftSize = newFrame->fftSize();
    AudioFloatArray buffer(fftSize);
    newFrame->doInverseFFT(buffer.data());
    buffer.zeroRange(fftSize / 2, fftSize);

    // Put back into frequency domain.
    newFrame->doFFT(buffer.data());

    return newFrame.release();
}

void FFTFrame::interpolateFrequencyComponents(const FFTFrame& frame1, const FFTFrame& frame2, double interp)
{
    // FIXME : with some work, this method could be optimized

    float* realP = realData();
    float* imagP = imagData();

    const float* realP1 = frame1.realData();
    const float* imagP1 = frame1.imagData();
    const float* realP2 = frame2.realData();
    const float* imagP2 = frame2.imagData();

    m_FFTSize = frame1.fftSize();
    m_log2FFTSize = frame1.log2FFTSize();

    double s1base = (1.0 - interp);
    double s2base = interp;

    double phaseAccum = 0.0;
    double lastPhase1 = 0.0;
    double lastPhase2 = 0.0;

    realP[0] = static_cast<float>(s1base * realP1[0] + s2base * realP2[0]);
    imagP[0] = static_cast<float>(s1base * imagP1[0] + s2base * imagP2[0]);

    int n = m_FFTSize / 2;

    for (int i = 1; i < n; ++i) {
        std::complex<double> c1(realP1[i], imagP1[i]);
        std::complex<double> c2(realP2[i], imagP2[i]);

        double mag1 = abs(c1);
        double mag2 = abs(c2);

        // Interpolate magnitudes in decibels
        double mag1db = 20.0 * log10(mag1);
        double mag2db = 20.0 * log10(mag2);

        double s1 = s1base;
        double s2 = s2base;

        double magdbdiff = mag1db - mag2db;

        // Empirical tweak to retain higher-frequency zeroes
        double threshold =  (i > 16) ? 5.0 : 2.0;

        if (magdbdiff < -threshold && mag1db < 0.0) {
            s1 = pow(s1, 0.75);
            s2 = 1.0 - s1;
        } else if (magdbdiff > threshold && mag2db < 0.0) {
            s2 = pow(s2, 0.75);
            s1 = 1.0 - s2;
        }

        // Average magnitude by decibels instead of linearly
        double magdb = s1 * mag1db + s2 * mag2db;
        double mag = pow(10.0, 0.05 * magdb);

        // Now, deal with phase
        double phase1 = arg(c1);
        double phase2 = arg(c2);

        double deltaPhase1 = phase1 - lastPhase1;
        double deltaPhase2 = phase2 - lastPhase2;
        lastPhase1 = phase1;
        lastPhase2 = phase2;

        // Unwrap phase deltas
        if (deltaPhase1 > piDouble)
            deltaPhase1 -= 2.0 * piDouble;
        if (deltaPhase1 < -piDouble)
            deltaPhase1 += 2.0 * piDouble;
        if (deltaPhase2 > piDouble)
            deltaPhase2 -= 2.0 * piDouble;
        if (deltaPhase2 < -piDouble)
            deltaPhase2 += 2.0 * piDouble;

        // Blend group-delays
        double deltaPhaseBlend;

        if (deltaPhase1 - deltaPhase2 > piDouble)
            deltaPhaseBlend = s1 * deltaPhase1 + s2 * (2.0 * piDouble + deltaPhase2);
        else if (deltaPhase2 - deltaPhase1 > piDouble)
            deltaPhaseBlend = s1 * (2.0 * piDouble + deltaPhase1) + s2 * deltaPhase2;
        else
            deltaPhaseBlend = s1 * deltaPhase1 + s2 * deltaPhase2;

        phaseAccum += deltaPhaseBlend;

        // Unwrap
        if (phaseAccum > piDouble)
            phaseAccum -= 2.0 * piDouble;
        if (phaseAccum < -piDouble)
            phaseAccum += 2.0 * piDouble;

        std::complex<double> c = std::polar(mag, phaseAccum);

        realP[i] = static_cast<float>(c.real());
        imagP[i] = static_cast<float>(c.imag());
    }
}

double FFTFrame::extractAverageGroupDelay()
{
    float* realP = realData();
    float* imagP = imagData();

    double aveSum = 0.0;
    double weightSum = 0.0;
    double lastPhase = 0.0;

    int halfSize = fftSize() / 2;

    const double kSamplePhaseDelay = (2.0 * piDouble) / double(fftSize());

    // Calculate weighted average group delay
    for (int i = 0; i < halfSize; i++) {
        std::complex<double> c(realP[i], imagP[i]);
        double mag = abs(c);
        double phase = arg(c);

        double deltaPhase = phase - lastPhase;
        lastPhase = phase;

        // Unwrap
        if (deltaPhase < -piDouble)
            deltaPhase += 2.0 * piDouble;
        if (deltaPhase > piDouble)
            deltaPhase -= 2.0 * piDouble;

        aveSum += mag * deltaPhase;
        weightSum += mag;
    }

    // Note how we invert the phase delta wrt frequency since this is how group delay is defined
    double ave = aveSum / weightSum;
    double aveSampleDelay = -ave / kSamplePhaseDelay;

    // Leave 20 sample headroom (for leading edge of impulse)
    if (aveSampleDelay > 20.0)
        aveSampleDelay -= 20.0;

    // Remove average group delay (minus 20 samples for headroom)
    addConstantGroupDelay(-aveSampleDelay);

    // Remove DC offset
    realP[0] = 0.0f;

    return aveSampleDelay;
}

void FFTFrame::addConstantGroupDelay(double sampleFrameDelay)
{
    int halfSize = fftSize() / 2;

    float* realP = realData();
    float* imagP = imagData();

    const double kSamplePhaseDelay = (2.0 * piDouble) / double(fftSize());

    double phaseAdj = -sampleFrameDelay * kSamplePhaseDelay;

    // Add constant group delay
    for (int i = 1; i < halfSize; i++) {
        std::complex<double> c(realP[i], imagP[i]);
        double mag = abs(c);
        double phase = arg(c);

        phase += i * phaseAdj;

        std::complex<double> c2 = std::polar(mag, phase);

        realP[i] = static_cast<float>(c2.real());
        imagP[i] = static_cast<float>(c2.imag());
    }
}

#ifndef NDEBUG
void FFTFrame::print()
{
    FFTFrame& frame = *this;
    float* realP = frame.realData();
    float* imagP = frame.imagData();
    LOG(WebAudio, "**** \n");
    LOG(WebAudio, "DC = %f : nyquist = %f\n", realP[0], imagP[0]);

    int n = m_FFTSize / 2;

    for (int i = 1; i < n; i++) {
        double mag = sqrt(realP[i] * realP[i] + imagP[i] * imagP[i]);
        double phase = atan2(realP[i], imagP[i]);

        LOG(WebAudio, "[%d] (%f %f)\n", i, mag, phase);
    }
    LOG(WebAudio, "****\n");
}
#endif // NDEBUG

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
