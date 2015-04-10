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

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "WaveShaperProcessor.h"

#include "WaveShaperDSPKernel.h"

namespace WebCore {
    
WaveShaperProcessor::WaveShaperProcessor(float sampleRate, size_t numberOfChannels)
    : AudioDSPKernelProcessor(sampleRate, numberOfChannels)
    , m_oversample(OverSampleNone)
{
}

WaveShaperProcessor::~WaveShaperProcessor()
{
    if (isInitialized())
        uninitialize();
}

PassOwnPtr<AudioDSPKernel> WaveShaperProcessor::createKernel()
{
    return adoptPtr(new WaveShaperDSPKernel(this));
}

void WaveShaperProcessor::setCurve(Float32Array* curve)
{
    // This synchronizes with process().
    MutexLocker processLocker(m_processLock);
    
    m_curve = curve;
}

void WaveShaperProcessor::setOversample(OverSampleType oversample)
{
    // This synchronizes with process().
    MutexLocker processLocker(m_processLock);

    m_oversample = oversample;

    if (oversample != OverSampleNone) {
        for (unsigned i = 0; i < m_kernels.size(); ++i) {
            WaveShaperDSPKernel* kernel = static_cast<WaveShaperDSPKernel*>(m_kernels[i].get());
            kernel->lazyInitializeOversampling();
        }
    }
}

void WaveShaperProcessor::process(const AudioBus* source, AudioBus* destination, size_t framesToProcess)
{
    if (!isInitialized()) {
        destination->zero();
        return;
    }

    bool channelCountMatches = source->numberOfChannels() == destination->numberOfChannels() && source->numberOfChannels() == m_kernels.size();
    ASSERT(channelCountMatches);
    if (!channelCountMatches)
        return;

    // The audio thread can't block on this lock, so we call tryLock() instead.
    MutexTryLocker tryLocker(m_processLock);
    if (tryLocker.locked()) {        
        // For each channel of our input, process using the corresponding WaveShaperDSPKernel into the output channel.
        for (unsigned i = 0; i < m_kernels.size(); ++i)
            m_kernels[i]->process(source->channel(i)->data(), destination->channel(i)->mutableData(), framesToProcess);
    } else {
        // Too bad - the tryLock() failed. We must be in the middle of a setCurve() call.
        destination->zero();
    }
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
