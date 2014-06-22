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

#include "AudioPullFIFO.h"

namespace WebCore {

AudioPullFIFO::AudioPullFIFO(AudioSourceProvider& audioProvider, unsigned numberOfChannels, size_t fifoLength, size_t providerSize)
    : m_provider(audioProvider)
    , m_fifo(numberOfChannels, fifoLength)
    , m_providerSize(providerSize)
    , m_tempBus(AudioBus::create(numberOfChannels, providerSize))
{
}

void AudioPullFIFO::consume(AudioBus* destination, size_t framesToConsume)
{
    if (!destination)
        return;

    if (framesToConsume > m_fifo.framesInFifo()) {
        // We don't have enough data in the FIFO to fulfill the request. Ask for more data.
        fillBuffer(framesToConsume - m_fifo.framesInFifo());
    }

    m_fifo.consume(destination, framesToConsume);
}

void AudioPullFIFO::fillBuffer(size_t numberOfFrames)
{
    // Keep asking the provider to give us data until we have received at least |numberOfFrames| of
    // data. Stuff the data into the FIFO.
    size_t framesProvided = 0;

    while (framesProvided < numberOfFrames) {
        m_provider.provideInput(m_tempBus.get(), m_providerSize);

        m_fifo.push(m_tempBus.get());

        framesProvided += m_providerSize;
    }
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
