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

#ifndef AudioPullFIFO_h
#define AudioPullFIFO_h

#include "AudioBus.h"
#include "AudioFIFO.h"
#include "AudioSourceProvider.h"

namespace WebCore {

// A FIFO (First In First Out) buffer to handle mismatches in buffer sizes between a provider and
// receiver. The receiver will "pull" data from this FIFO. If data is already available in the
// FIFO, it is provided to the receiver. If insufficient data is available to satisfy the request,
// the FIFO will ask the provider for more data when necessary to fulfill a request. Contrast this
// with a "push" FIFO, where the sender pushes data to the FIFO which will itself push the data to
// the receiver when the FIFO is full.
class AudioPullFIFO {
public:
    // Create a FIFO that gets data from |provider|. The FIFO will be large enough to hold
    // |fifoLength| frames of data of |numberOfChannels| channels. The AudioSourceProvider will be
    // asked to produce |providerSize| frames when the FIFO needs more data.
    AudioPullFIFO(AudioSourceProvider& audioProvider, unsigned numberOfChannels, size_t fifoLength, size_t providerSize);

    // Read |framesToConsume| frames from the FIFO into the destination. If the FIFO does not have
    // enough data, we ask the |provider| to get more data to fulfill the request.
    void consume(AudioBus* destination, size_t framesToConsume);

private:
    // Fill the FIFO buffer with at least |numberOfFrames| more data.
    void fillBuffer(size_t numberOfFrames);

    // The provider of the data in our FIFO.
    AudioSourceProvider& m_provider;

    // The actual FIFO
    AudioFIFO m_fifo;

    // Number of frames of data that the provider will produce per call.
    unsigned int m_providerSize;

    // Temporary workspace to hold the data from the provider.
    RefPtr<AudioBus> m_tempBus;
};

} // namespace WebCore

#endif // AudioPullFIFO.h
