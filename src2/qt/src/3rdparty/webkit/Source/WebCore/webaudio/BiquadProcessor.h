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

#ifndef BiquadProcessor_h
#define BiquadProcessor_h

#include "AudioDSPKernel.h"
#include "AudioDSPKernelProcessor.h"
#include "AudioNode.h"
#include "AudioParam.h"
#include "Biquad.h"
#include <wtf/RefPtr.h>

namespace WebCore {

// BiquadProcessor is an AudioDSPKernelProcessor which uses Biquad objects to implement several common filters.

class BiquadProcessor : public AudioDSPKernelProcessor {
public:
    enum FilterType {
        LowPass2,
        HighPass2,
        Peaking,
        Allpass,
        LowShelf,
        HighShelf
    };
    
    BiquadProcessor(FilterType, double sampleRate, size_t numberOfChannels, bool autoInitialize = true);
    virtual ~BiquadProcessor();
    
    virtual PassOwnPtr<AudioDSPKernel> createKernel();
        
    virtual void process(AudioBus* source, AudioBus* destination, size_t framesToProcess);

    bool filterCoefficientsDirty() const { return m_filterCoefficientsDirty; }

    AudioParam* parameter1() { return m_parameter1.get(); }
    AudioParam* parameter2() { return m_parameter2.get(); }
    AudioParam* parameter3() { return m_parameter3.get(); }

    FilterType type() const { return m_type; }

private:
    FilterType m_type;

    RefPtr<AudioParam> m_parameter1;
    RefPtr<AudioParam> m_parameter2;
    RefPtr<AudioParam> m_parameter3;

    // so DSP kernels know when to re-compute coefficients
    bool m_filterCoefficientsDirty;
};

} // namespace WebCore

#endif // BiquadProcessor_h
