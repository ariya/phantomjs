/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "AudioResampler.h"

#include "AudioBus.h"
#include <algorithm>
#include <wtf/MathExtras.h>

using namespace std;
 
namespace WebCore {

const double AudioResampler::MaxRate = 8.0;

AudioResampler::AudioResampler()
    : m_rate(1.0)
{
    m_kernels.append(adoptPtr(new AudioResamplerKernel(this)));
    m_sourceBus = AudioBus::create(1, 0, false);
}

AudioResampler::AudioResampler(unsigned numberOfChannels)
    : m_rate(1.0)
{
    for (unsigned i = 0; i < numberOfChannels; ++i)
        m_kernels.append(adoptPtr(new AudioResamplerKernel(this)));

    m_sourceBus = AudioBus::create(numberOfChannels, 0, false);
}

void AudioResampler::configureChannels(unsigned numberOfChannels)
{
    unsigned currentSize = m_kernels.size();
    if (numberOfChannels == currentSize)
        return; // already setup

    // First deal with adding or removing kernels.
    if (numberOfChannels > currentSize) {
        for (unsigned i = currentSize; i < numberOfChannels; ++i)
            m_kernels.append(adoptPtr(new AudioResamplerKernel(this)));
    } else
        m_kernels.resize(numberOfChannels);

    // Reconfigure our source bus to the new channel size.
    m_sourceBus = AudioBus::create(numberOfChannels, 0, false);
}

void AudioResampler::process(AudioSourceProvider* provider, AudioBus* destinationBus, size_t framesToProcess)
{
    ASSERT(provider);
    if (!provider)
        return;
        
    unsigned numberOfChannels = m_kernels.size();

    // Make sure our configuration matches the bus we're rendering to.
    bool channelsMatch = (destinationBus && destinationBus->numberOfChannels() == numberOfChannels);
    ASSERT(channelsMatch);
    if (!channelsMatch)
        return;

    // Setup the source bus.
    for (unsigned i = 0; i < numberOfChannels; ++i) {
        // Figure out how many frames we need to get from the provider, and a pointer to the buffer.
        size_t framesNeeded;
        float* fillPointer = m_kernels[i]->getSourcePointer(framesToProcess, &framesNeeded);
        ASSERT(fillPointer);
        if (!fillPointer)
            return;
            
        m_sourceBus->setChannelMemory(i, fillPointer, framesNeeded);
    }

    // Ask the provider to supply the desired number of source frames.
    provider->provideInput(m_sourceBus.get(), m_sourceBus->length());

    // Now that we have the source data, resample each channel into the destination bus.
    // FIXME: optimize for the common stereo case where it's faster to process both left/right channels in the same inner loop.
    for (unsigned i = 0; i < numberOfChannels; ++i) {
        float* destination = destinationBus->channel(i)->mutableData();
        m_kernels[i]->process(destination, framesToProcess);
    }
}

void AudioResampler::setRate(double rate)
{
    if (std::isnan(rate) || std::isinf(rate) || rate <= 0.0)
        return;
    
    m_rate = min(AudioResampler::MaxRate, rate);
}

void AudioResampler::reset()
{
    unsigned numberOfChannels = m_kernels.size();
    for (unsigned i = 0; i < numberOfChannels; ++i)
        m_kernels[i]->reset();
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
