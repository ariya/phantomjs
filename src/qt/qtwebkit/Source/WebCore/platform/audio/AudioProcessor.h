/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AudioProcessor_h
#define AudioProcessor_h

namespace WebCore {

class AudioBus;

// AudioProcessor is an abstract base class representing an audio signal processing object with a single input and a single output,
// where the number of input channels equals the number of output channels.  It can be used as one part of a complex DSP algorithm,
// or as the processor for a basic (one input - one output) AudioNode.

class AudioProcessor {
public:
    AudioProcessor(float sampleRate, unsigned numberOfChannels)
        : m_initialized(false)
        , m_numberOfChannels(numberOfChannels)
        , m_sampleRate(sampleRate)
    {
    }

    virtual ~AudioProcessor() { }

    // Full initialization can be done here instead of in the constructor.
    virtual void initialize() = 0;
    virtual void uninitialize() = 0;

    // Processes the source to destination bus.  The number of channels must match in source and destination.
    virtual void process(const AudioBus* source, AudioBus* destination, size_t framesToProcess) = 0;

    // Resets filter state
    virtual void reset() = 0;

    virtual void setNumberOfChannels(unsigned) = 0;
    virtual unsigned numberOfChannels() const = 0;

    bool isInitialized() const { return m_initialized; }

    float sampleRate() const { return m_sampleRate; }

    virtual double tailTime() const = 0;
    virtual double latencyTime() const = 0;

protected:
    bool m_initialized;
    unsigned m_numberOfChannels;
    float m_sampleRate;
};

} // namespace WebCore

#endif // AudioProcessor_h
