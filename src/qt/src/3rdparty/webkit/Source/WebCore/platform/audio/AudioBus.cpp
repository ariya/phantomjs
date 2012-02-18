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

#include "AudioBus.h"

#if !PLATFORM(MAC)
#include "SincResampler.h"
#endif
#include "VectorMath.h"
#include <algorithm>
#include <assert.h>
#include <math.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

using namespace VectorMath;
    
AudioBus::AudioBus(unsigned numberOfChannels, size_t length, bool allocate)
    : m_length(length)
    , m_busGain(1.0)
    , m_isFirstTime(true)
    , m_sampleRate(0.0)
{
    m_channels.reserveInitialCapacity(numberOfChannels);

    for (unsigned i = 0; i < numberOfChannels; ++i) {
        PassOwnPtr<AudioChannel> channel = allocate ? adoptPtr(new AudioChannel(length)) : adoptPtr(new AudioChannel(0, length));
        m_channels.append(channel);
    }

    m_layout = LayoutCanonical; // for now this is the only layout we define
}

void AudioBus::setChannelMemory(unsigned channelIndex, float* storage, size_t length)
{
    if (channelIndex < m_channels.size()) {
        channel(channelIndex)->set(storage, length);
        m_length = length; // FIXME: verify that this length matches all the other channel lengths
    }
}

void AudioBus::zero()
{
    for (unsigned i = 0; i < m_channels.size(); ++i)
        m_channels[i]->zero();
}

AudioChannel* AudioBus::channelByType(unsigned channelType)
{
    // For now we only support canonical channel layouts...
    if (m_layout != LayoutCanonical)
        return 0;

    switch (numberOfChannels()) {
    case 1: // mono
        if (channelType == ChannelMono || channelType == ChannelLeft)
            return channel(0);
        return 0;

    case 2: // stereo
        switch (channelType) {
        case ChannelLeft: return channel(0);
        case ChannelRight: return channel(1);
        default: return 0;
        }

    case 4: // quad
        switch (channelType) {
        case ChannelLeft: return channel(0);
        case ChannelRight: return channel(1);
        case ChannelSurroundLeft: return channel(2);
        case ChannelSurroundRight: return channel(3);
        default: return 0;
        }

    case 5: // 5.0
        switch (channelType) {
        case ChannelLeft: return channel(0);
        case ChannelRight: return channel(1);
        case ChannelCenter: return channel(2);
        case ChannelSurroundLeft: return channel(3);
        case ChannelSurroundRight: return channel(4);
        default: return 0;
        }

    case 6: // 5.1
        switch (channelType) {
        case ChannelLeft: return channel(0);
        case ChannelRight: return channel(1);
        case ChannelCenter: return channel(2);
        case ChannelLFE: return channel(3);
        case ChannelSurroundLeft: return channel(4);
        case ChannelSurroundRight: return channel(5);
        default: return 0;
        }
    }
    
    ASSERT_NOT_REACHED();
    return 0;
}

// Returns true if the channel count and frame-size match.
bool AudioBus::topologyMatches(const AudioBus& bus) const
{
    if (numberOfChannels() != bus.numberOfChannels())
        return false; // channel mismatch

    // Make sure source bus has enough frames.
    if (length() > bus.length())
        return false; // frame-size mismatch

    return true;
}

PassOwnPtr<AudioBus> AudioBus::createBufferFromRange(AudioBus* sourceBuffer, unsigned startFrame, unsigned endFrame)
{
    size_t numberOfSourceFrames = sourceBuffer->length();
    unsigned numberOfChannels = sourceBuffer->numberOfChannels();

    // Sanity checking
    bool isRangeSafe = startFrame < endFrame && endFrame <= numberOfSourceFrames;
    ASSERT(isRangeSafe);
    if (!isRangeSafe)
        return 0;

    size_t rangeLength = endFrame - startFrame;

    OwnPtr<AudioBus> audioBus = adoptPtr(new AudioBus(numberOfChannels, rangeLength));
    audioBus->setSampleRate(sourceBuffer->sampleRate());

    for (unsigned i = 0; i < numberOfChannels; ++i)
        audioBus->channel(i)->copyFromRange(sourceBuffer->channel(i), startFrame, endFrame);

    return audioBus.release();
}

float AudioBus::maxAbsValue() const
{
    float max = 0.0f;
    for (unsigned i = 0; i < numberOfChannels(); ++i) {
        const AudioChannel* channel = this->channel(i);
        max = std::max(max, channel->maxAbsValue());
    }

    return max;
}

void AudioBus::normalize()
{
    float max = maxAbsValue();
    if (max)
        scale(1.0f / max);
}

void AudioBus::scale(double scale)
{
    for (unsigned i = 0; i < numberOfChannels(); ++i)
        channel(i)->scale(scale);
}

// Just copies the samples from the source bus to this one.
// This is just a simple copy if the number of channels match, otherwise a mixup or mixdown is done.
// For now, we just support a mixup from mono -> stereo.
void AudioBus::copyFrom(const AudioBus& sourceBus)
{
    if (&sourceBus == this)
        return;

    if (numberOfChannels() == sourceBus.numberOfChannels()) {
        for (unsigned i = 0; i < numberOfChannels(); ++i)
            channel(i)->copyFrom(sourceBus.channel(i));
    } else if (numberOfChannels() == 2 && sourceBus.numberOfChannels() == 1) {
        // Handle mono -> stereo case (for now simply copy mono channel into both left and right)
        // FIXME: Really we should apply an equal-power scaling factor here, since we're effectively panning center...
        const AudioChannel* sourceChannel = sourceBus.channel(0);
        channel(0)->copyFrom(sourceChannel);
        channel(1)->copyFrom(sourceChannel);
    } else {
        // Case not handled
        ASSERT_NOT_REACHED();
    }
}

void AudioBus::sumFrom(const AudioBus &sourceBus)
{
    if (numberOfChannels() == sourceBus.numberOfChannels()) {
        for (unsigned i = 0; i < numberOfChannels(); ++i)
            channel(i)->sumFrom(sourceBus.channel(i));
    } else if (numberOfChannels() == 2 && sourceBus.numberOfChannels() == 1) {
        // Handle mono -> stereo case (for now simply sum mono channel into both left and right)
        // FIXME: Really we should apply an equal-power scaling factor here, since we're effectively panning center...
        const AudioChannel* sourceChannel = sourceBus.channel(0);
        channel(0)->sumFrom(sourceChannel);
        channel(1)->sumFrom(sourceChannel);
    } else {
        // Case not handled
        ASSERT_NOT_REACHED();
    }
}

void AudioBus::processWithGainFromMonoStereo(const AudioBus &sourceBus, double* lastMixGain, double targetGain, bool sumToBus)
{
    // We don't want to suddenly change the gain from mixing one time slice to the next,
    // so we "de-zipper" by slowly changing the gain each sample-frame until we've achieved the target gain.

    // FIXME: optimize this method (SSE, etc.)
    // FIXME: Need fast path here when gain has converged on targetGain. In this case, de-zippering is no longer needed.
    // FIXME: Need fast path when this==sourceBus && lastMixGain==targetGain==1.0 && sumToBus==false (this is a NOP)

    // Take master bus gain into account as well as the targetGain.
    double totalDesiredGain = m_busGain * targetGain;

    // First time, snap directly to totalDesiredGain.
    double gain = m_isFirstTime ? totalDesiredGain : *lastMixGain;
    m_isFirstTime = false;

    int numberOfSourceChannels = sourceBus.numberOfChannels();
    int numberOfDestinationChannels = numberOfChannels();

    AudioBus& sourceBusSafe = const_cast<AudioBus&>(sourceBus);
    const float* sourceL = sourceBusSafe.channelByType(ChannelLeft)->data();
    const float* sourceR = numberOfSourceChannels > 1 ? sourceBusSafe.channelByType(ChannelRight)->data() : 0;

    float* destinationL = channelByType(ChannelLeft)->data();
    float* destinationR = numberOfDestinationChannels > 1 ? channelByType(ChannelRight)->data() : 0;

    const double DezipperRate = 0.005;
    int framesToProcess = length();

    if (sumToBus) {
        // Sum to our bus
        if (sourceR && destinationR) {
            // Stereo
            while (framesToProcess--) {
                float sampleL = *sourceL++;
                float sampleR = *sourceR++;
                *destinationL++ += static_cast<float>(gain * sampleL);
                *destinationR++ += static_cast<float>(gain * sampleR);

                // Slowly change gain to desired gain.
                gain += (totalDesiredGain - gain) * DezipperRate;
            }
        } else if (destinationR) {
            // Mono -> stereo (mix equally into L and R)
            // FIXME: Really we should apply an equal-power scaling factor here, since we're effectively panning center...
            while (framesToProcess--) {
                float sample = *sourceL++;
                *destinationL++ += static_cast<float>(gain * sample);
                *destinationR++ += static_cast<float>(gain * sample);

                // Slowly change gain to desired gain.
                gain += (totalDesiredGain - gain) * DezipperRate;
            }
        } else {
            // Mono
            while (framesToProcess--) {
                float sampleL = *sourceL++;
                *destinationL++ += static_cast<float>(gain * sampleL);

                // Slowly change gain to desired gain.
                gain += (totalDesiredGain - gain) * DezipperRate;
            }
        }
    } else {
        // Process directly (without summing) to our bus
        if (sourceR && destinationR) {
            // Stereo
            while (framesToProcess--) {
                float sampleL = *sourceL++;
                float sampleR = *sourceR++;
                *destinationL++ = static_cast<float>(gain * sampleL);
                *destinationR++ = static_cast<float>(gain * sampleR);

                // Slowly change gain to desired gain.
                gain += (totalDesiredGain - gain) * DezipperRate;
            }
        } else if (destinationR) {
            // Mono -> stereo (mix equally into L and R)
            // FIXME: Really we should apply an equal-power scaling factor here, since we're effectively panning center...
            while (framesToProcess--) {
                float sample = *sourceL++;
                *destinationL++ = static_cast<float>(gain * sample);
                *destinationR++ = static_cast<float>(gain * sample);

                // Slowly change gain to desired gain.
                gain += (totalDesiredGain - gain) * DezipperRate;
            }
        } else {
            // Mono
            while (framesToProcess--) {
                float sampleL = *sourceL++;
                *destinationL++ = static_cast<float>(gain * sampleL);

                // Slowly change gain to desired gain.
                gain += (totalDesiredGain - gain) * DezipperRate;
            }
        }
    }

    // Save the target gain as the starting point for next time around.
    *lastMixGain = gain;
}

void AudioBus::processWithGainFrom(const AudioBus &sourceBus, double* lastMixGain, double targetGain, bool sumToBus)
{
    // Make sure we're summing from same type of bus.
    // We *are* able to sum from mono -> stereo
    if (sourceBus.numberOfChannels() != 1 && !topologyMatches(sourceBus))
        return;

    // Dispatch for different channel layouts
    switch (numberOfChannels()) {
    case 1: // mono
    case 2: // stereo
        processWithGainFromMonoStereo(sourceBus, lastMixGain, targetGain, sumToBus);
        break;
    case 4: // FIXME: implement quad
    case 5: // FIXME: implement 5.0
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

void AudioBus::copyWithGainFrom(const AudioBus &sourceBus, double* lastMixGain, double targetGain)
{
    processWithGainFrom(sourceBus, lastMixGain, targetGain, false);
}

void AudioBus::sumWithGainFrom(const AudioBus &sourceBus, double* lastMixGain, double targetGain)
{
    processWithGainFrom(sourceBus, lastMixGain, targetGain, true);
}

#if !PLATFORM(MAC)
PassOwnPtr<AudioBus> AudioBus::createBySampleRateConverting(AudioBus* sourceBus, bool mixToMono, double newSampleRate)
{
    // sourceBus's sample-rate must be known.
    ASSERT(sourceBus && sourceBus->sampleRate());
    if (!sourceBus || !sourceBus->sampleRate())
        return 0;

    double sourceSampleRate = sourceBus->sampleRate();
    double destinationSampleRate = newSampleRate;
    unsigned numberOfSourceChannels = sourceBus->numberOfChannels();

    if (numberOfSourceChannels == 1)
        mixToMono = false; // already mono
        
    if (sourceSampleRate == destinationSampleRate) {
        // No sample-rate conversion is necessary.
        if (mixToMono)
            return AudioBus::createByMixingToMono(sourceBus);

        // Return exact copy.
        return AudioBus::createBufferFromRange(sourceBus, 0, sourceBus->length());
    }
    
    // First, mix to mono (if necessary) then sample-rate convert.
    AudioBus* resamplerSourceBus;
    OwnPtr<AudioBus> mixedMonoBus;
    if (mixToMono) {
        mixedMonoBus = AudioBus::createByMixingToMono(sourceBus);
        resamplerSourceBus = mixedMonoBus.get();
    } else {
        // Directly resample without down-mixing.
        resamplerSourceBus = sourceBus;
    }

    // Calculate destination length based on the sample-rates.
    double sampleRateRatio = sourceSampleRate / destinationSampleRate;
    int sourceLength = resamplerSourceBus->length();
    int destinationLength = sourceLength / sampleRateRatio;

    // Create destination bus with same number of channels.
    unsigned numberOfDestinationChannels = resamplerSourceBus->numberOfChannels();
    OwnPtr<AudioBus> destinationBus(adoptPtr(new AudioBus(numberOfDestinationChannels, destinationLength)));

    // Sample-rate convert each channel.
    for (unsigned i = 0; i < numberOfDestinationChannels; ++i) {
        float* source = resamplerSourceBus->channel(i)->data();
        float* destination = destinationBus->channel(i)->data();

        SincResampler resampler(sampleRateRatio);
        resampler.process(source, destination, sourceLength);
    }

    destinationBus->setSampleRate(newSampleRate);    
    return destinationBus.release();
}
#endif // !PLATFORM(MAC)

PassOwnPtr<AudioBus> AudioBus::createByMixingToMono(AudioBus* sourceBus)
{
    switch (sourceBus->numberOfChannels()) {
    case 1:
        // Simply create an exact copy.
        return AudioBus::createBufferFromRange(sourceBus, 0, sourceBus->length());
    case 2:
        {
            unsigned n = sourceBus->length();
            OwnPtr<AudioBus> destinationBus(adoptPtr(new AudioBus(1, n)));

            float* sourceL = sourceBus->channel(0)->data();
            float* sourceR = sourceBus->channel(1)->data();
            float* destination = destinationBus->channel(0)->data();
        
            // Do the mono mixdown.
            for (unsigned i = 0; i < n; ++i)
                destination[i] = 0.5 * (sourceL[i] + sourceR[i]);

            destinationBus->setSampleRate(sourceBus->sampleRate());    
            return destinationBus.release();
        }
    }

    ASSERT_NOT_REACHED();
    return 0;
}

} // WebCore

#endif // ENABLE(WEB_AUDIO)
