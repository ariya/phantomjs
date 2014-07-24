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

#ifndef WaveShaperDSPKernel_h
#define WaveShaperDSPKernel_h

#include "AudioArray.h"
#include "AudioDSPKernel.h"
#include "DownSampler.h"
#include "UpSampler.h"
#include "WaveShaperProcessor.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

class WaveShaperProcessor;

// WaveShaperDSPKernel is an AudioDSPKernel and is responsible for non-linear distortion on one channel.

class WaveShaperDSPKernel : public AudioDSPKernel {
public:
    explicit WaveShaperDSPKernel(WaveShaperProcessor*);

    // AudioDSPKernel
    virtual void process(const float* source, float* dest, size_t framesToProcess);
    virtual void reset();
    virtual double tailTime() const OVERRIDE { return 0; }
    virtual double latencyTime() const OVERRIDE;

    // Oversampling requires more resources, so let's only allocate them if needed.
    void lazyInitializeOversampling();

protected:
    // Apply the shaping curve.
    void processCurve(const float* source, float* dest, size_t framesToProcess);

    // Use up-sampling, process at the higher sample-rate, then down-sample.
    void processCurve2x(const float* source, float* dest, size_t framesToProcess);
    void processCurve4x(const float* source, float* dest, size_t framesToProcess);

    WaveShaperProcessor* waveShaperProcessor() { return static_cast<WaveShaperProcessor*>(processor()); }

    // Oversampling.
    OwnPtr<AudioFloatArray> m_tempBuffer;
    OwnPtr<AudioFloatArray> m_tempBuffer2;
    OwnPtr<UpSampler> m_upSampler;
    OwnPtr<DownSampler> m_downSampler;
    OwnPtr<UpSampler> m_upSampler2;
    OwnPtr<DownSampler> m_downSampler2;
};

} // namespace WebCore

#endif // WaveShaperDSPKernel_h
