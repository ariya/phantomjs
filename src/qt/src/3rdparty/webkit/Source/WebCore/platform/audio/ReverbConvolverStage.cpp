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

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "ReverbConvolverStage.h"

#include "VectorMath.h"
#include "ReverbAccumulationBuffer.h"
#include "ReverbConvolver.h"
#include "ReverbInputBuffer.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

using namespace VectorMath;

ReverbConvolverStage::ReverbConvolverStage(float* impulseResponse, size_t responseLength, size_t reverbTotalLatency, size_t stageOffset, size_t stageLength,
                                           size_t fftSize, size_t renderPhase, size_t renderSliceSize, ReverbAccumulationBuffer* accumulationBuffer)
    : m_fftKernel(fftSize)
    , m_accumulationBuffer(accumulationBuffer)
    , m_accumulationReadIndex(0)
    , m_inputReadIndex(0)
    , m_impulseResponseLength(responseLength)
{
    ASSERT(impulseResponse);
    ASSERT(accumulationBuffer);
    
    m_fftKernel.doPaddedFFT(impulseResponse + stageOffset, stageLength);
    m_convolver = adoptPtr(new FFTConvolver(fftSize));
    m_temporaryBuffer.resize(renderSliceSize);

    // The convolution stage at offset stageOffset needs to have a corresponding delay to cancel out the offset.
    size_t totalDelay = stageOffset + reverbTotalLatency;

    // But, the FFT convolution itself incurs fftSize / 2 latency, so subtract this out...
    size_t halfSize = fftSize / 2;
    ASSERT(totalDelay >= halfSize);
    if (totalDelay >= halfSize)
        totalDelay -= halfSize;

    // We divide up the total delay, into pre and post delay sections so that we can schedule at exactly the moment when the FFT will happen.
    // This is coordinated with the other stages, so they don't all do their FFTs at the same time...
    int maxPreDelayLength = std::min(halfSize, totalDelay);
    m_preDelayLength = totalDelay > 0 ? renderPhase % maxPreDelayLength : 0;
    if (m_preDelayLength > totalDelay)
        m_preDelayLength = 0;

    m_postDelayLength = totalDelay - m_preDelayLength;
    m_preReadWriteIndex = 0;
    m_framesProcessed = 0; // total frames processed so far

    m_preDelayBuffer.resize(m_preDelayLength < fftSize ? fftSize : m_preDelayLength);
}

void ReverbConvolverStage::processInBackground(ReverbConvolver* convolver, size_t framesToProcess)
{
    ReverbInputBuffer* inputBuffer = convolver->inputBuffer();
    float* source = inputBuffer->directReadFrom(&m_inputReadIndex, framesToProcess);
    process(source, framesToProcess);
}

void ReverbConvolverStage::process(float* source, size_t framesToProcess)
{
    ASSERT(source);
    if (!source)
        return;
    
    // Deal with pre-delay stream : note special handling of zero delay.

    float* preDelayedSource;
    float* temporaryBuffer;
    bool isTemporaryBufferSafe = false;
    if (m_preDelayLength > 0) {
        // Handles both the read case (call to process() ) and the write case (memcpy() )
        bool isPreDelaySafe = m_preReadWriteIndex + framesToProcess <= m_preDelayBuffer.size();
        ASSERT(isPreDelaySafe);
        if (!isPreDelaySafe)
            return;

        isTemporaryBufferSafe = framesToProcess <= m_temporaryBuffer.size();

        preDelayedSource = m_preDelayBuffer.data() + m_preReadWriteIndex;
        temporaryBuffer = m_temporaryBuffer.data();        
    } else {
        // Zero delay
        preDelayedSource = source;
        temporaryBuffer = m_preDelayBuffer.data();
        
        isTemporaryBufferSafe = framesToProcess <= m_preDelayBuffer.size();
    }
    
    ASSERT(isTemporaryBufferSafe);
    if (!isTemporaryBufferSafe)
        return;

    int writeIndex = 0;

    if (m_framesProcessed < m_preDelayLength) {
        // For the first m_preDelayLength frames don't process the convolver, instead simply buffer in the pre-delay.
        // But while buffering the pre-delay, we still need to update our index.
        m_accumulationBuffer->updateReadIndex(&m_accumulationReadIndex, framesToProcess);
    } else {
        // Now, run the convolution (into the delay buffer).
        // An expensive FFT will happen every fftSize / 2 frames.
        // We process in-place here...
        m_convolver->process(&m_fftKernel, preDelayedSource, temporaryBuffer, framesToProcess);

        // Now accumulate into reverb's accumulation buffer.
        writeIndex = m_accumulationBuffer->accumulate(temporaryBuffer, framesToProcess, &m_accumulationReadIndex, m_postDelayLength);
    }

    // Finally copy input to pre-delay.
    if (m_preDelayLength > 0) {
        memcpy(preDelayedSource, source, sizeof(float) * framesToProcess);
        m_preReadWriteIndex += framesToProcess;

        ASSERT(m_preReadWriteIndex <= m_preDelayLength);
        if (m_preReadWriteIndex >= m_preDelayLength)
            m_preReadWriteIndex = 0;
    }

    m_framesProcessed += framesToProcess;
}

void ReverbConvolverStage::reset()
{
    m_convolver->reset();
    m_preDelayBuffer.zero();
    m_accumulationReadIndex = 0;
    m_inputReadIndex = 0;
    m_framesProcessed = 0;
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
