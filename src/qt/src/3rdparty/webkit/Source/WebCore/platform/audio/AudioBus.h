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

#ifndef AudioBus_h
#define AudioBus_h

#include "AudioChannel.h"
#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

// An AudioBus represents a collection of one or more AudioChannels.
// The data layout is "planar" as opposed to "interleaved".
// An AudioBus with one channel is mono, an AudioBus with two channels is stereo, etc.
class AudioBus {
    WTF_MAKE_NONCOPYABLE(AudioBus);
public:
    enum {
        ChannelLeft = 0,
        ChannelRight = 1,
        ChannelCenter = 2, // center and mono are the same
        ChannelMono = 2,
        ChannelLFE = 3,
        ChannelSurroundLeft = 4,
        ChannelSurroundRight = 5,
    };

    enum {
        LayoutCanonical = 0
        // Can define non-standard layouts here
    };

    // allocate indicates whether or not to initially have the AudioChannels created with managed storage.
    // Normal usage is to pass true here, in which case the AudioChannels will memory-manage their own storage.
    // If allocate is false then setChannelMemory() has to be called later on for each channel before the AudioBus is useable...
    AudioBus(unsigned numberOfChannels, size_t length, bool allocate = true);

    // Tells the given channel to use an externally allocated buffer.
    void setChannelMemory(unsigned channelIndex, float* storage, size_t length);

    // Channels
    unsigned numberOfChannels() const { return m_channels.size(); }

    AudioChannel* channel(unsigned channel) { return m_channels[channel].get(); }
    const AudioChannel* channel(unsigned channel) const { return const_cast<AudioBus*>(this)->m_channels[channel].get(); }
    AudioChannel* channelByType(unsigned type);

    // Number of sample-frames
    size_t length() const { return m_length; }

    // Sample-rate : 0.0 if unknown or "don't care"
    double sampleRate() const { return m_sampleRate; }
    void setSampleRate(double sampleRate) { m_sampleRate = sampleRate; }

    // Zeroes all channels.
    void zero();

    // Returns true if the channel count and frame-size match.
    bool topologyMatches(const AudioBus &sourceBus) const;

    // Creates a new buffer from a range in the source buffer.
    // 0 may be returned if the range does not fit in the sourceBuffer
    static PassOwnPtr<AudioBus> createBufferFromRange(AudioBus* sourceBuffer, unsigned startFrame, unsigned endFrame);


#if !PLATFORM(MAC)
    // Creates a new AudioBus by sample-rate converting sourceBus to the newSampleRate.
    // setSampleRate() must have been previously called on sourceBus.
    // Note: sample-rate conversion is already handled in the file-reading code for the mac port, so we don't need this.
    static PassOwnPtr<AudioBus> createBySampleRateConverting(AudioBus* sourceBus, bool mixToMono, double newSampleRate);
#endif

    // Creates a new AudioBus by mixing all the channels down to mono.
    // If sourceBus is already mono, then the returned AudioBus will simply be a copy.
    static PassOwnPtr<AudioBus> createByMixingToMono(AudioBus* sourceBus);

    // Scales all samples by the same amount.
    void scale(double scale);

    // Master gain for this bus - used with sumWithGainFrom() below
    void setGain(double gain) { m_busGain = gain; }
    double gain() { return m_busGain; }

    void reset() { m_isFirstTime = true; } // for de-zippering

    // Assuming sourceBus has the same topology, copies sample data from each channel of sourceBus to our corresponding channel.
    void copyFrom(const AudioBus &sourceBus);

    // Sums the sourceBus into our bus with unity gain.
    // Our own internal gain m_busGain is ignored.
    void sumFrom(const AudioBus &sourceBus);

    // Copy or sum each channel from sourceBus into our corresponding channel.
    // We scale by targetGain (and our own internal gain m_busGain), performing "de-zippering" to smoothly change from *lastMixGain to (targetGain*m_busGain).
    // The caller is responsible for setting up lastMixGain to point to storage which is unique for every "stream" which will be summed to this bus.
    // This represents the dezippering memory.
    void copyWithGainFrom(const AudioBus &sourceBus, double* lastMixGain, double targetGain);
    void sumWithGainFrom(const AudioBus &sourceBus, double* lastMixGain, double targetGain);

    // Returns maximum absolute value across all channels (useful for normalization).
    float maxAbsValue() const;

    // Makes maximum absolute value == 1.0 (if possible).
    void normalize();

    static PassOwnPtr<AudioBus> loadPlatformResource(const char* name, double sampleRate);

protected:
    AudioBus() { };

    void processWithGainFrom(const AudioBus &sourceBus, double* lastMixGain, double targetGain, bool sumToBus);
    void processWithGainFromMonoStereo(const AudioBus &sourceBus, double* lastMixGain, double targetGain, bool sumToBus);

    size_t m_length;

    Vector<OwnPtr<AudioChannel> > m_channels;

    int m_layout;

    double m_busGain;
    bool m_isFirstTime;
    double m_sampleRate; // 0.0 if unknown or N/A
};

} // WebCore

#endif // AudioBus_h
