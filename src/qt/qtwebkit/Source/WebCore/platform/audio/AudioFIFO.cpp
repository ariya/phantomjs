/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#include "AudioFIFO.h"

namespace WebCore {

AudioFIFO::AudioFIFO(unsigned numberOfChannels, size_t fifoLength)
    : m_fifoAudioBus(AudioBus::create(numberOfChannels, fifoLength))
    , m_fifoLength(fifoLength)
    , m_framesInFifo(0)
    , m_readIndex(0)
    , m_writeIndex(0)
{
}

void AudioFIFO::consume(AudioBus* destination, size_t framesToConsume)
{
    bool isGood = destination && (framesToConsume <= m_fifoLength) && (framesToConsume <= m_framesInFifo) && (destination->length() >= framesToConsume);
    ASSERT(isGood);
    if (!isGood)
        return;

    // Copy the requested number of samples to the destination.

    size_t part1Length;
    size_t part2Length;
    findWrapLengths(m_readIndex, framesToConsume, part1Length, part2Length);

    size_t numberOfChannels = m_fifoAudioBus->numberOfChannels();

    for (size_t channelIndex = 0; channelIndex < numberOfChannels; ++channelIndex) {
        float* destinationData = destination->channel(channelIndex)->mutableData();
        const float* sourceData = m_fifoAudioBus->channel(channelIndex)->data();

        bool isCopyGood = ((m_readIndex < m_fifoLength)
                           && (m_readIndex + part1Length) <= m_fifoLength
                           && (part1Length <= destination->length())
                           && (part1Length + part2Length) <= destination->length());
        ASSERT(isCopyGood);
        if (!isCopyGood)
            return;

        memcpy(destinationData, sourceData + m_readIndex, part1Length * sizeof(*sourceData));
        // Handle wrap around of the FIFO, if needed.
        if (part2Length)
            memcpy(destinationData + part1Length, sourceData, part2Length * sizeof(*sourceData));
    }
    m_readIndex = updateIndex(m_readIndex, framesToConsume);
    ASSERT(m_framesInFifo >= framesToConsume);
    m_framesInFifo -= framesToConsume;
}

void AudioFIFO::push(const AudioBus* sourceBus)
{
    // Copy the sourceBus into the FIFO buffer.

    bool isGood = sourceBus && (m_framesInFifo + sourceBus->length() <= m_fifoLength);
    if (!isGood)
        return;

    size_t sourceLength = sourceBus->length();
    size_t part1Length;
    size_t part2Length;
    findWrapLengths(m_writeIndex, sourceLength, part1Length, part2Length);

    size_t numberOfChannels = m_fifoAudioBus->numberOfChannels();

    for (size_t channelIndex = 0; channelIndex < numberOfChannels; ++channelIndex) {
        float* destination = m_fifoAudioBus->channel(channelIndex)->mutableData();
        const float* source = sourceBus->channel(channelIndex)->data();

        bool isCopyGood = ((m_writeIndex < m_fifoLength)
                           && (m_writeIndex + part1Length) <= m_fifoLength
                           && part2Length < m_fifoLength
                           && part1Length + part2Length <= sourceLength);
        ASSERT(isCopyGood);
        if (!isCopyGood)
            return;

        memcpy(destination + m_writeIndex, source, part1Length * sizeof(*destination));

        // Handle wrap around of the FIFO, if needed.
        if (part2Length)
            memcpy(destination, source + part1Length, part2Length * sizeof(*destination));
    }
        
    m_framesInFifo += sourceLength;
    ASSERT(m_framesInFifo <= m_fifoLength);
    m_writeIndex = updateIndex(m_writeIndex, sourceLength);
}

void AudioFIFO::findWrapLengths(size_t index, size_t size, size_t& part1Length, size_t& part2Length)
{
    ASSERT_WITH_SECURITY_IMPLICATION(index < m_fifoLength && size <= m_fifoLength);
    if (index < m_fifoLength && size <= m_fifoLength) {
        if (index + size > m_fifoLength) {
            // Need to wrap. Figure out the length of each piece.
            part1Length = m_fifoLength - index;
            part2Length = size - part1Length;
        } else {
            // No wrap needed.
            part1Length = size;
            part2Length = 0;
        }
    } else {
        // Invalid values for index or size. Set the part lengths to zero so nothing is copied.
        part1Length = 0;
        part2Length = 0;
    }
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
