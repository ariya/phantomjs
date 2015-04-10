/*
 * Copyright (C) 2012, Google Inc. All rights reserved.
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

#ifndef OscillatorNode_h
#define OscillatorNode_h

#include "AudioBus.h"
#include "AudioParam.h"
#include "AudioScheduledSourceNode.h"
#include <wtf/OwnArrayPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Threading.h>

namespace WebCore {

class AudioContext;
class PeriodicWave;

// OscillatorNode is an audio generator of periodic waveforms.

class OscillatorNode : public AudioScheduledSourceNode {
public:
    // The waveform type.
    // These must be defined as in the .idl file.
    enum {
        SINE = 0,
        SQUARE = 1,
        SAWTOOTH = 2,
        TRIANGLE = 3,
        CUSTOM = 4
    };

    static PassRefPtr<OscillatorNode> create(AudioContext*, float sampleRate);

    virtual ~OscillatorNode();
    
    // AudioNode
    virtual void process(size_t framesToProcess);
    virtual void reset();

    String type() const;

    bool setType(unsigned); // Returns true on success.
    void setType(const String&);

    AudioParam* frequency() { return m_frequency.get(); }
    AudioParam* detune() { return m_detune.get(); }

    void setPeriodicWave(PeriodicWave*);

private:
    OscillatorNode(AudioContext*, float sampleRate);

    virtual double tailTime() const OVERRIDE { return 0; }
    virtual double latencyTime() const OVERRIDE { return 0; }

    // Returns true if there are sample-accurate timeline parameter changes.
    bool calculateSampleAccuratePhaseIncrements(size_t framesToProcess);

    virtual bool propagatesSilence() const OVERRIDE;

    // One of the waveform types defined in the enum.
    unsigned short m_type;
    
    // Frequency value in Hertz.
    RefPtr<AudioParam> m_frequency;

    // Detune value (deviating from the frequency) in Cents.
    RefPtr<AudioParam> m_detune;

    bool m_firstRender;

    // m_virtualReadIndex is a sample-frame index into our buffer representing the current playback position.
    // Since it's floating-point, it has sub-sample accuracy.
    double m_virtualReadIndex;

    // This synchronizes process().
    mutable Mutex m_processLock;

    // Stores sample-accurate values calculated according to frequency and detune.
    AudioFloatArray m_phaseIncrements;
    AudioFloatArray m_detuneValues;
    
    RefPtr<PeriodicWave> m_periodicWave;

    // Cache the wave tables for different waveform types, except CUSTOM.
    static PeriodicWave* s_periodicWaveSine;
    static PeriodicWave* s_periodicWaveSquare;
    static PeriodicWave* s_periodicWaveSawtooth;
    static PeriodicWave* s_periodicWaveTriangle;
};

} // namespace WebCore

#endif // OscillatorNode_h
