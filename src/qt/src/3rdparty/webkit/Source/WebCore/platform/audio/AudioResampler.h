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

#ifndef AudioResampler_h
#define AudioResampler_h

#include "AudioBus.h"
#include "AudioResamplerKernel.h"
#include "AudioSourceProvider.h"
#include <wtf/OwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

// AudioResampler resamples the audio stream from an AudioSourceProvider.
// The audio stream may be single or multi-channel.
// The default constructor defaults to single-channel (mono).

class AudioResampler {
public:
    AudioResampler();
    AudioResampler(unsigned numberOfChannels);
    ~AudioResampler() { }
    
    // Given an AudioSourceProvider, process() resamples the source stream into destinationBus.
    void process(AudioSourceProvider*, AudioBus* destinationBus, size_t framesToProcess);

    // Resets the processing state.
    void reset();

    void configureChannels(unsigned numberOfChannels);

    // 0 < rate <= MaxRate
    void setRate(double rate);
    double rate() const { return m_rate; }

    static const double MaxRate;

private:
    double m_rate;
    Vector<OwnPtr<AudioResamplerKernel> > m_kernels;
    OwnPtr<AudioBus> m_sourceBus;
};

} // namespace WebCore

#endif // AudioResampler_h
