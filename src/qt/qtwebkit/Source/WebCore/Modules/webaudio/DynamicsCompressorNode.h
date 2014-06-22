/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
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

#ifndef DynamicsCompressorNode_h
#define DynamicsCompressorNode_h

#include "AudioNode.h"
#include "AudioParam.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

class DynamicsCompressor;

class DynamicsCompressorNode : public AudioNode {
public:
    static PassRefPtr<DynamicsCompressorNode> create(AudioContext* context, float sampleRate)
    {
        return adoptRef(new DynamicsCompressorNode(context, sampleRate));
    }

    virtual ~DynamicsCompressorNode();

    // AudioNode
    virtual void process(size_t framesToProcess);
    virtual void reset();
    virtual void initialize();
    virtual void uninitialize();

    // Static compression curve parameters.
    AudioParam* threshold() { return m_threshold.get(); }
    AudioParam* knee() { return m_knee.get(); }
    AudioParam* ratio() { return m_ratio.get(); }
    AudioParam* attack() { return m_attack.get(); }
    AudioParam* release() { return m_release.get(); }

    // Amount by which the compressor is currently compressing the signal in decibels.
    AudioParam* reduction() { return m_reduction.get(); }

private:
    virtual double tailTime() const OVERRIDE;
    virtual double latencyTime() const OVERRIDE;

    DynamicsCompressorNode(AudioContext*, float sampleRate);

    OwnPtr<DynamicsCompressor> m_dynamicsCompressor;
    RefPtr<AudioParam> m_threshold;
    RefPtr<AudioParam> m_knee;
    RefPtr<AudioParam> m_ratio;
    RefPtr<AudioParam> m_reduction;
    RefPtr<AudioParam> m_attack;
    RefPtr<AudioParam> m_release;
};

} // namespace WebCore

#endif // DynamicsCompressorNode_h
