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

#include "Reverb.h"

#include "AudioBus.h"
#include "AudioFileReader.h"
#include "ReverbConvolver.h"
#include "VectorMath.h"
#include <math.h>
#include <wtf/MathExtras.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

#if OS(DARWIN)
using namespace std;
#endif

namespace WebCore {

using namespace VectorMath;

// Empirical gain calibration tested across many impulse responses to ensure perceived volume is same as dry (unprocessed) signal
const float GainCalibration = -58;
const float GainCalibrationSampleRate = 44100;

// A minimum power value to when normalizing a silent (or very quiet) impulse response
const float MinPower = 0.000125f;
    
static float calculateNormalizationScale(AudioBus* response)
{
    // Normalize by RMS power
    size_t numberOfChannels = response->numberOfChannels();
    size_t length = response->length();

    float power = 0;

    for (size_t i = 0; i < numberOfChannels; ++i) {
        float channelPower = 0;
        vsvesq(response->channel(i)->data(), 1, &channelPower, length);
        power += channelPower;
    }

    power = sqrt(power / (numberOfChannels * length));

    // Protect against accidental overload
    if (std::isinf(power) || std::isnan(power) || power < MinPower)
        power = MinPower;

    float scale = 1 / power;

    scale *= powf(10, GainCalibration * 0.05f); // calibrate to make perceived volume same as unprocessed

    // Scale depends on sample-rate.
    if (response->sampleRate())
        scale *= GainCalibrationSampleRate / response->sampleRate();

    // True-stereo compensation
    if (response->numberOfChannels() == 4)
        scale *= 0.5f;

    return scale;
}

Reverb::Reverb(AudioBus* impulseResponse, size_t renderSliceSize, size_t maxFFTSize, size_t numberOfChannels, bool useBackgroundThreads, bool normalize)
{
    float scale = 1;

    if (normalize) {
        scale = calculateNormalizationScale(impulseResponse);

        if (scale)
            impulseResponse->scale(scale);
    }

    initialize(impulseResponse, renderSliceSize, maxFFTSize, numberOfChannels, useBackgroundThreads);

    // Undo scaling since this shouldn't be a destructive operation on impulseResponse.
    // FIXME: What about roundoff? Perhaps consider making a temporary scaled copy
    // instead of scaling and unscaling in place.
    if (normalize && scale)
        impulseResponse->scale(1 / scale);
}

void Reverb::initialize(AudioBus* impulseResponseBuffer, size_t renderSliceSize, size_t maxFFTSize, size_t numberOfChannels, bool useBackgroundThreads)
{
    m_impulseResponseLength = impulseResponseBuffer->length();

    // The reverb can handle a mono impulse response and still do stereo processing
    size_t numResponseChannels = impulseResponseBuffer->numberOfChannels();
    m_convolvers.reserveCapacity(numberOfChannels);

    int convolverRenderPhase = 0;
    for (size_t i = 0; i < numResponseChannels; ++i) {
        AudioChannel* channel = impulseResponseBuffer->channel(i);

        OwnPtr<ReverbConvolver> convolver = adoptPtr(new ReverbConvolver(channel, renderSliceSize, maxFFTSize, convolverRenderPhase, useBackgroundThreads));
        m_convolvers.append(convolver.release());

        convolverRenderPhase += renderSliceSize;
    }

    // For "True" stereo processing we allocate a temporary buffer to avoid repeatedly allocating it in the process() method.
    // It can be bad to allocate memory in a real-time thread.
    if (numResponseChannels == 4)
        m_tempBuffer = AudioBus::create(2, MaxFrameSize);
}

void Reverb::process(const AudioBus* sourceBus, AudioBus* destinationBus, size_t framesToProcess)
{
    // Do a fairly comprehensive sanity check.
    // If these conditions are satisfied, all of the source and destination pointers will be valid for the various matrixing cases.
    bool isSafeToProcess = sourceBus && destinationBus && sourceBus->numberOfChannels() > 0 && destinationBus->numberOfChannels() > 0
        && framesToProcess <= MaxFrameSize && framesToProcess <= sourceBus->length() && framesToProcess <= destinationBus->length(); 
    
    ASSERT(isSafeToProcess);
    if (!isSafeToProcess)
        return;

    // For now only handle mono or stereo output
    if (destinationBus->numberOfChannels() > 2) {
        destinationBus->zero();
        return;
    }

    AudioChannel* destinationChannelL = destinationBus->channel(0);
    const AudioChannel* sourceChannelL = sourceBus->channel(0);

    // Handle input -> output matrixing...
    size_t numInputChannels = sourceBus->numberOfChannels();
    size_t numOutputChannels = destinationBus->numberOfChannels();
    size_t numReverbChannels = m_convolvers.size();

    if (numInputChannels == 2 && numReverbChannels == 2 && numOutputChannels == 2) {
        // 2 -> 2 -> 2
        const AudioChannel* sourceChannelR = sourceBus->channel(1);
        AudioChannel* destinationChannelR = destinationBus->channel(1);
        m_convolvers[0]->process(sourceChannelL, destinationChannelL, framesToProcess);
        m_convolvers[1]->process(sourceChannelR, destinationChannelR, framesToProcess);
    } else  if (numInputChannels == 1 && numOutputChannels == 2 && numReverbChannels == 2) {
        // 1 -> 2 -> 2
        for (int i = 0; i < 2; ++i) {
            AudioChannel* destinationChannel = destinationBus->channel(i);
            m_convolvers[i]->process(sourceChannelL, destinationChannel, framesToProcess);
        }
    } else if (numInputChannels == 1 && numReverbChannels == 1 && numOutputChannels == 2) {
        // 1 -> 1 -> 2
        m_convolvers[0]->process(sourceChannelL, destinationChannelL, framesToProcess);

        // simply copy L -> R
        AudioChannel* destinationChannelR = destinationBus->channel(1);
        bool isCopySafe = destinationChannelL->data() && destinationChannelR->data() && destinationChannelL->length() >= framesToProcess && destinationChannelR->length() >= framesToProcess;
        ASSERT(isCopySafe);
        if (!isCopySafe)
            return;
        memcpy(destinationChannelR->mutableData(), destinationChannelL->data(), sizeof(float) * framesToProcess);
    } else if (numInputChannels == 1 && numReverbChannels == 1 && numOutputChannels == 1) {
        // 1 -> 1 -> 1
        m_convolvers[0]->process(sourceChannelL, destinationChannelL, framesToProcess);
    } else if (numInputChannels == 2 && numReverbChannels == 4 && numOutputChannels == 2) {
        // 2 -> 4 -> 2 ("True" stereo)
        const AudioChannel* sourceChannelR = sourceBus->channel(1);
        AudioChannel* destinationChannelR = destinationBus->channel(1);

        AudioChannel* tempChannelL = m_tempBuffer->channel(0);
        AudioChannel* tempChannelR = m_tempBuffer->channel(1);

        // Process left virtual source
        m_convolvers[0]->process(sourceChannelL, destinationChannelL, framesToProcess);
        m_convolvers[1]->process(sourceChannelL, destinationChannelR, framesToProcess);

        // Process right virtual source
        m_convolvers[2]->process(sourceChannelR, tempChannelL, framesToProcess);
        m_convolvers[3]->process(sourceChannelR, tempChannelR, framesToProcess);

        destinationBus->sumFrom(*m_tempBuffer);
    } else if (numInputChannels == 1 && numReverbChannels == 4 && numOutputChannels == 2) {
        // 1 -> 4 -> 2 (Processing mono with "True" stereo impulse response)
        // This is an inefficient use of a four-channel impulse response, but we should handle the case.
        AudioChannel* destinationChannelR = destinationBus->channel(1);

        AudioChannel* tempChannelL = m_tempBuffer->channel(0);
        AudioChannel* tempChannelR = m_tempBuffer->channel(1);

        // Process left virtual source
        m_convolvers[0]->process(sourceChannelL, destinationChannelL, framesToProcess);
        m_convolvers[1]->process(sourceChannelL, destinationChannelR, framesToProcess);

        // Process right virtual source
        m_convolvers[2]->process(sourceChannelL, tempChannelL, framesToProcess);
        m_convolvers[3]->process(sourceChannelL, tempChannelR, framesToProcess);

        destinationBus->sumFrom(*m_tempBuffer);
    } else {
        // Handle gracefully any unexpected / unsupported matrixing
        // FIXME: add code for 5.1 support...
        destinationBus->zero();
    }
}

void Reverb::reset()
{
    for (size_t i = 0; i < m_convolvers.size(); ++i)
        m_convolvers[i]->reset();
}

size_t Reverb::latencyFrames() const
{
    return !m_convolvers.isEmpty() ? m_convolvers.first()->latencyFrames() : 0;
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
