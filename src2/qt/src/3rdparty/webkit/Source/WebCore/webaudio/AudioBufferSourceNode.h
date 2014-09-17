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

#ifndef AudioBufferSourceNode_h
#define AudioBufferSourceNode_h

#include "AudioBuffer.h"
#include "AudioBus.h"
#include "AudioGain.h"
#include "AudioPannerNode.h"
#include "AudioResampler.h"
#include "AudioSourceNode.h"
#include "AudioSourceProvider.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Threading.h>

namespace WebCore {

class AudioContext;

// AudioBufferSourceNode is an AudioNode representing an audio source from an in-memory audio asset represented by an AudioBuffer.
// It generally will be used for short sounds which require a high degree of scheduling flexibility (can playback in rhythmically perfect ways).

class AudioBufferSourceNode : public AudioSourceNode, public AudioSourceProvider {
public:
    static PassRefPtr<AudioBufferSourceNode> create(AudioContext*, double sampleRate);

    virtual ~AudioBufferSourceNode();
    
    // AudioNode
    virtual void process(size_t framesToProcess);
    virtual void reset();

    // AudioSourceProvider
    // When process() is called, the resampler calls provideInput (in the audio thread) to gets its input stream.
    virtual void provideInput(AudioBus*, size_t numberOfFrames);
    
    // setBuffer() is called on the main thread.  This is the buffer we use for playback.
    void setBuffer(AudioBuffer*);
    AudioBuffer* buffer() { return m_buffer.get(); }
                    
    // numberOfChannels() returns the number of output channels.  This value equals the number of channels from the buffer.
    // If a new buffer is set with a different number of channels, then this value will dynamically change.
    unsigned numberOfChannels();
                    
    // Play-state
    // noteOn(), noteGrainOn(), and noteOff() must all be called from the main thread.
    void noteOn(double when);
    void noteGrainOn(double when, double grainOffset, double grainDuration);
    void noteOff(double when);

    bool looping() const { return m_isLooping; }
    void setLooping(bool looping) { m_isLooping = looping; }
    
    AudioGain* gain() { return m_gain.get(); }                                        
    AudioParam* playbackRate() { return m_playbackRate.get(); }

    // If a panner node is set, then we can incorporate doppler shift into the playback pitch rate.
    void setPannerNode(PassRefPtr<AudioPannerNode> pannerNode) { m_pannerNode = pannerNode; }

private:
    AudioBufferSourceNode(AudioContext*, double sampleRate);

    // m_buffer holds the sample data which this node outputs.
    RefPtr<AudioBuffer> m_buffer;

    // Used for the "gain" and "playbackRate" attributes.
    RefPtr<AudioGain> m_gain;
    RefPtr<AudioParam> m_playbackRate;

    // m_isPlaying is set to true when noteOn() or noteGrainOn() is called.
    bool m_isPlaying;

    // If m_isLooping is false, then this node will be done playing and become inactive after it reaches the end of the sample data in the buffer.
    // If true, it will wrap around to the start of the buffer each time it reaches the end.
    bool m_isLooping;

    // This node is considered finished when it reaches the end of the buffer's sample data after noteOn() has been called.
    // This will only be set to true if m_isLooping == false.
    bool m_hasFinished;

    // m_startTime is the time to start playing based on the context's timeline (0.0 or a time less than the context's current time means "now").
    double m_startTime; // in seconds

    // m_schedulingFrameDelay is the sample-accurate scheduling offset.
    // It's used so that we start rendering audio samples at a very precise point in time.
    // It will only be a non-zero value the very first render quantum that we render from the buffer.
    int m_schedulingFrameDelay;

    // m_readIndex is a sample-frame index into our buffer representing the current playback position.
    unsigned m_readIndex;

    // Granular playback
    bool m_isGrain;
    double m_grainOffset; // in seconds
    double m_grainDuration; // in seconds
    int m_grainFrameCount; // keeps track of which frame in the grain we're currently rendering

    // totalPitchRate() returns the instantaneous pitch rate (non-time preserving).
    // It incorporates the base pitch rate, any sample-rate conversion factor from the buffer, and any doppler shift from an associated panner node.
    double totalPitchRate();

    // m_resampler performs the pitch rate changes to the buffer playback.
    AudioResampler m_resampler;

    // m_lastGain provides continuity when we dynamically adjust the gain.
    double m_lastGain;
    
    // We optionally keep track of a panner node which has a doppler shift that is incorporated into the pitch rate.
    RefPtr<AudioPannerNode> m_pannerNode;

    // This synchronizes process() with setBuffer() which can cause dynamic channel count changes.
    mutable Mutex m_processLock;

    // Reads the next framesToProcess sample-frames from the AudioBuffer into destinationBus.
    // A grain envelope will be applied if m_isGrain is set to true.
    void readFromBuffer(AudioBus* destinationBus, size_t framesToProcess);

    // readFromBufferWithGrainEnvelope() is a low-level blitter which reads from the AudioBuffer and applies a grain envelope.
    void readFromBufferWithGrainEnvelope(float* sourceL, float* sourceR, float* destinationL, float* destinationR, size_t framesToProcess);
};

} // namespace WebCore

#endif // AudioBufferSourceNode_h
