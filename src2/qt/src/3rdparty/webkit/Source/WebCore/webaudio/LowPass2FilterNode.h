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

#ifndef LowPass2FilterNode_h
#define LowPass2FilterNode_h

#include "AudioBasicProcessorNode.h"
#include "BiquadProcessor.h"

namespace WebCore {

class AudioParam;
    
class LowPass2FilterNode : public AudioBasicProcessorNode {
public:
    static PassRefPtr<LowPass2FilterNode> create(AudioContext* context, double sampleRate)
    {
        return adoptRef(new LowPass2FilterNode(context, sampleRate));      
    }

    AudioParam* cutoff() { return biquadProcessor()->parameter1(); }
    AudioParam* resonance() { return biquadProcessor()->parameter2(); }
    
private:
    LowPass2FilterNode(AudioContext*, double sampleRate);

    BiquadProcessor* biquadProcessor() { return static_cast<BiquadProcessor*>(processor()); }
};

} // namespace WebCore

#endif // LowPass2FilterNode_h
