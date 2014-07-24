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

#include "AudioBuffer.h"

#include "AudioBus.h"
#include "AudioContext.h"
#include "AudioFileReader.h"
#include "ExceptionCode.h"
#include "ExceptionCodePlaceholder.h"

namespace WebCore {

PassRefPtr<AudioBuffer> AudioBuffer::create(unsigned numberOfChannels, size_t numberOfFrames, float sampleRate)
{
    if (sampleRate < 22050 || sampleRate > 96000 || numberOfChannels > AudioContext::maxNumberOfChannels() || !numberOfFrames)
        return 0;
    
    return adoptRef(new AudioBuffer(numberOfChannels, numberOfFrames, sampleRate));
}

PassRefPtr<AudioBuffer> AudioBuffer::createFromAudioFileData(const void* data, size_t dataSize, bool mixToMono, float sampleRate)
{
    RefPtr<AudioBus> bus = createBusFromInMemoryAudioFile(data, dataSize, mixToMono, sampleRate);
    if (bus.get())
        return adoptRef(new AudioBuffer(bus.get()));

    return 0;
}

AudioBuffer::AudioBuffer(unsigned numberOfChannels, size_t numberOfFrames, float sampleRate)
    : m_gain(1.0)
    , m_sampleRate(sampleRate)
    , m_length(numberOfFrames)
{
    m_channels.reserveCapacity(numberOfChannels);

    for (unsigned i = 0; i < numberOfChannels; ++i) {
        RefPtr<Float32Array> channelDataArray = Float32Array::create(m_length);
        channelDataArray->setNeuterable(false);
        m_channels.append(channelDataArray);
    }
}

AudioBuffer::AudioBuffer(AudioBus* bus)
    : m_gain(1.0)
    , m_sampleRate(bus->sampleRate())
    , m_length(bus->length())
{
    // Copy audio data from the bus to the Float32Arrays we manage.
    unsigned numberOfChannels = bus->numberOfChannels();
    m_channels.reserveCapacity(numberOfChannels);
    for (unsigned i = 0; i < numberOfChannels; ++i) {
        RefPtr<Float32Array> channelDataArray = Float32Array::create(m_length);
        channelDataArray->setNeuterable(false);
        channelDataArray->setRange(bus->channel(i)->data(), m_length, 0);
        m_channels.append(channelDataArray);
    }
}

void AudioBuffer::releaseMemory()
{
    m_channels.clear();
}

PassRefPtr<Float32Array> AudioBuffer::getChannelData(unsigned channelIndex, ExceptionCode& ec)
{
    if (channelIndex >= m_channels.size()) {
        ec = SYNTAX_ERR;
        return 0;
    }

    Float32Array* channelData = m_channels[channelIndex].get();
    return Float32Array::create(channelData->buffer(), channelData->byteOffset(), channelData->length());
}

Float32Array* AudioBuffer::getChannelData(unsigned channelIndex)
{
    if (channelIndex >= m_channels.size())
        return 0;

    return m_channels[channelIndex].get();
}

void AudioBuffer::zero()
{
    for (unsigned i = 0; i < m_channels.size(); ++i) {
        if (getChannelData(i))
            getChannelData(i)->zeroRange(0, length());
    }
}

size_t AudioBuffer::memoryCost() const
{
    size_t cost = 0;
    for (unsigned i = 0; i < m_channels.size() ; ++i)
        cost += m_channels[i]->byteLength();
    return cost;
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
