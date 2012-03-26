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

#ifndef RealtimeAnalyserNode_h
#define RealtimeAnalyserNode_h

#include "AudioNode.h"
#include "RealtimeAnalyser.h"

namespace WebCore {

class RealtimeAnalyserNode : public AudioNode {
public:
    static PassRefPtr<RealtimeAnalyserNode> create(AudioContext* context, double sampleRate)
    {
        return adoptRef(new RealtimeAnalyserNode(context, sampleRate));      
    }

    virtual ~RealtimeAnalyserNode();
    
    // AudioNode
    virtual void process(size_t framesToProcess);
    virtual void pullInputs(size_t framesToProcess);
    virtual void reset();

    // Javascript bindings
    unsigned int fftSize() const { return m_analyser.fftSize(); }
    void setFftSize(unsigned int size) { m_analyser.setFftSize(size); }

    unsigned frequencyBinCount() const { return m_analyser.frequencyBinCount(); }

    void setMinDecibels(float k) { m_analyser.setMinDecibels(k); }
    float minDecibels() const { return m_analyser.minDecibels(); }

    void setMaxDecibels(float k) { m_analyser.setMaxDecibels(k); }
    float maxDecibels() const { return m_analyser.maxDecibels(); }

    void setSmoothingTimeConstant(float k) { m_analyser.setSmoothingTimeConstant(k); }
    float smoothingTimeConstant() const { return m_analyser.smoothingTimeConstant(); }

#if ENABLE(WEBGL)
    void getFloatFrequencyData(Float32Array* array) { m_analyser.getFloatFrequencyData(array); }
    void getByteFrequencyData(Uint8Array* array) { m_analyser.getByteFrequencyData(array); }
    void getByteTimeDomainData(Uint8Array* array) { m_analyser.getByteTimeDomainData(array); }
#endif

private:
    RealtimeAnalyserNode(AudioContext*, double sampleRate);

    RealtimeAnalyser m_analyser;
};

} // namespace WebCore

#endif // RealtimeAnalyserNode_h
