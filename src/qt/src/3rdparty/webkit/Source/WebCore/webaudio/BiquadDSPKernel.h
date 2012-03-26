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

#ifndef BiquadDSPKernel_h
#define BiquadDSPKernel_h

#include "AudioDSPKernel.h"
#include "Biquad.h"
#include "BiquadProcessor.h"

namespace WebCore {

class BiquadProcessor;

// BiquadDSPKernel is an AudioDSPKernel and is responsible for filtering one channel of a BiquadProcessor using a Biquad object.

class BiquadDSPKernel : public AudioDSPKernel {
public:  
    BiquadDSPKernel(BiquadProcessor* processor)
    : AudioDSPKernel(processor)
    {
    }
    
    // AudioDSPKernel
    virtual void process(const float* source, float* dest, size_t framesToProcess);
    virtual void reset() { m_biquad.reset(); }
    
protected:
    Biquad m_biquad;
    BiquadProcessor* biquadProcessor() { return static_cast<BiquadProcessor*>(processor()); }
};

} // namespace WebCore

#endif // BiquadDSPKernel_h
