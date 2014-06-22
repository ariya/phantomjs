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

#ifndef AudioFIFO_h
#define AudioFIFO_h

#include "AudioBus.h"

namespace WebCore {

class AudioFIFO {
public:
    // Create a FIFO large enough to hold |fifoLength| frames of data of |numberOfChannels| channels.
    AudioFIFO(unsigned numberOfChannels, size_t fifoLength);

    // Push the data from the bus into the FIFO.
    void push(const AudioBus*);

    // Consume |framesToConsume| frames of data from the FIFO and put them in |destination|. The
    // corresponding frames are removed from the FIFO.
    void consume(AudioBus* destination, size_t framesToConsume);

    // Number of frames of data that are currently in the FIFO.
    size_t framesInFifo() const { return m_framesInFifo; }

private:
    // Update the FIFO index by the step, with appropriate wrapping around the endpoint.
    int updateIndex(int index, int step) { return (index + step) % m_fifoLength; }

    void findWrapLengths(size_t index, size_t providerSize, size_t& part1Length, size_t& part2Length);
    
    // The FIFO itself. In reality, the FIFO is a circular buffer.
    RefPtr<AudioBus> m_fifoAudioBus;

    // The total available space in the FIFO.
    size_t m_fifoLength;

    // The number of actual elements in the FIFO
    size_t m_framesInFifo;

    // Where to start reading from the FIFO.
    size_t m_readIndex;

    // Where to start writing to the FIFO.
    size_t m_writeIndex;
};

} // namespace WebCore

#endif // AudioFIFO.h
