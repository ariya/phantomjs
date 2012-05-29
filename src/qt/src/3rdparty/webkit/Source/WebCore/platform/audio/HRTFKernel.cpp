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

#include "HRTFKernel.h"

#include "AudioChannel.h"
#include "Biquad.h"
#include "FFTFrame.h"
#include <wtf/MathExtras.h>

using namespace std;

namespace WebCore {

// Takes the input AudioChannel as an input impulse response and calculates the average group delay.
// This represents the initial delay before the most energetic part of the impulse response.
// The sample-frame delay is removed from the impulseP impulse response, and this value  is returned.
// the length of the passed in AudioChannel must be a power of 2.
static double extractAverageGroupDelay(AudioChannel* channel, size_t analysisFFTSize)
{
    ASSERT(channel);
        
    float* impulseP = channel->data();
    
    ASSERT(channel->length() >= analysisFFTSize);
    
    // Check for power-of-2.
    ASSERT(1UL << static_cast<unsigned>(log2(analysisFFTSize)) == analysisFFTSize);

    FFTFrame estimationFrame(analysisFFTSize);
    estimationFrame.doFFT(impulseP);

    double frameDelay = estimationFrame.extractAverageGroupDelay();
    estimationFrame.doInverseFFT(impulseP);

    return frameDelay;
}

HRTFKernel::HRTFKernel(AudioChannel* channel, size_t fftSize, double sampleRate, bool bassBoost)
    : m_frameDelay(0.0)
    , m_sampleRate(sampleRate)
{
    ASSERT(channel);

    // Determine the leading delay (average group delay) for the response.
    m_frameDelay = extractAverageGroupDelay(channel, fftSize / 2);

    float* impulseResponse = channel->data();
    size_t responseLength = channel->length();

    if (bassBoost) {
        // Run through some post-processing to boost the bass a little -- the HRTF's seem to be a little bass-deficient.
        // FIXME: this post-processing should have already been applied to the HRTF file resources.  Once the files are put into this form,
        // then this code path can be removed along with the bassBoost parameter.
        Biquad filter;
        filter.setLowShelfParams(700.0 / nyquist(), 6.0); // boost 6dB at 700Hz
        filter.process(impulseResponse, impulseResponse, responseLength);
    }

    // We need to truncate to fit into 1/2 the FFT size (with zero padding) in order to do proper convolution.
    size_t truncatedResponseLength = min(responseLength, fftSize / 2); // truncate if necessary to max impulse response length allowed by FFT

    // Quick fade-out (apply window) at truncation point
    unsigned numberOfFadeOutFrames = static_cast<unsigned>(sampleRate / 4410); // 10 sample-frames @44.1KHz sample-rate
    ASSERT(numberOfFadeOutFrames < truncatedResponseLength);
    if (numberOfFadeOutFrames < truncatedResponseLength) {
        for (unsigned i = truncatedResponseLength - numberOfFadeOutFrames; i < truncatedResponseLength; ++i) {
            float x = 1.0f - static_cast<float>(i - (truncatedResponseLength - numberOfFadeOutFrames)) / numberOfFadeOutFrames;
            impulseResponse[i] *= x;
        }
    }

    m_fftFrame = adoptPtr(new FFTFrame(fftSize));
    m_fftFrame->doPaddedFFT(impulseResponse, truncatedResponseLength);
}

PassOwnPtr<AudioChannel> HRTFKernel::createImpulseResponse()
{
    OwnPtr<AudioChannel> channel = adoptPtr(new AudioChannel(fftSize()));
    FFTFrame fftFrame(*m_fftFrame);

    // Add leading delay back in.
    fftFrame.addConstantGroupDelay(m_frameDelay);
    fftFrame.doInverseFFT(channel->data());

    return channel.release();
}

// Interpolates two kernels with x: 0 -> 1 and returns the result.
PassRefPtr<HRTFKernel> HRTFKernel::createInterpolatedKernel(HRTFKernel* kernel1, HRTFKernel* kernel2, double x)
{
    ASSERT(kernel1 && kernel2);
    if (!kernel1 || !kernel2)
        return 0;
 
    ASSERT(x >= 0.0 && x < 1.0);
    x = min(1.0, max(0.0, x));
    
    double sampleRate1 = kernel1->sampleRate();
    double sampleRate2 = kernel2->sampleRate();
    ASSERT(sampleRate1 == sampleRate2);
    if (sampleRate1 != sampleRate2)
        return 0;
    
    double frameDelay = (1.0 - x) * kernel1->frameDelay() + x * kernel2->frameDelay();
    
    OwnPtr<FFTFrame> interpolatedFrame = FFTFrame::createInterpolatedFrame(*kernel1->fftFrame(), *kernel2->fftFrame(), x);
    return HRTFKernel::create(interpolatedFrame.release(), frameDelay, sampleRate1);
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
