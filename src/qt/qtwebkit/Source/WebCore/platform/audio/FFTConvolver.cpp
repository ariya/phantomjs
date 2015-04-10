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

#include "FFTConvolver.h"

#include "VectorMath.h"

namespace WebCore {

using namespace VectorMath;
    
FFTConvolver::FFTConvolver(size_t fftSize)
    : m_frame(fftSize)
    , m_readWriteIndex(0)
    , m_inputBuffer(fftSize) // 2nd half of buffer is always zeroed
    , m_outputBuffer(fftSize)
    , m_lastOverlapBuffer(fftSize / 2)
{
}

void FFTConvolver::process(FFTFrame* fftKernel, const float* sourceP, float* destP, size_t framesToProcess)
{
    size_t halfSize = fftSize() / 2;

    // framesToProcess must be an exact multiple of halfSize,
    // or halfSize is a multiple of framesToProcess when halfSize > framesToProcess.
    bool isGood = !(halfSize % framesToProcess && framesToProcess % halfSize);
    ASSERT(isGood);
    if (!isGood)
        return;

    size_t numberOfDivisions = halfSize <= framesToProcess ? (framesToProcess / halfSize) : 1;
    size_t divisionSize = numberOfDivisions == 1 ? framesToProcess : halfSize;

    for (size_t i = 0; i < numberOfDivisions; ++i, sourceP += divisionSize, destP += divisionSize) {
        // Copy samples to input buffer (note contraint above!)
        float* inputP = m_inputBuffer.data();

        // Sanity check
        bool isCopyGood1 = sourceP && inputP && m_readWriteIndex + divisionSize <= m_inputBuffer.size();
        ASSERT(isCopyGood1);
        if (!isCopyGood1)
            return;

        memcpy(inputP + m_readWriteIndex, sourceP, sizeof(float) * divisionSize);

        // Copy samples from output buffer
        float* outputP = m_outputBuffer.data();

        // Sanity check
        bool isCopyGood2 = destP && outputP && m_readWriteIndex + divisionSize <= m_outputBuffer.size();
        ASSERT(isCopyGood2);
        if (!isCopyGood2)
            return;

        memcpy(destP, outputP + m_readWriteIndex, sizeof(float) * divisionSize);
        m_readWriteIndex += divisionSize;

        // Check if it's time to perform the next FFT
        if (m_readWriteIndex == halfSize) {
            // The input buffer is now filled (get frequency-domain version)
            m_frame.doFFT(m_inputBuffer.data());
            m_frame.multiply(*fftKernel);
            m_frame.doInverseFFT(m_outputBuffer.data());

            // Overlap-add 1st half from previous time
            vadd(m_outputBuffer.data(), 1, m_lastOverlapBuffer.data(), 1, m_outputBuffer.data(), 1, halfSize);

            // Finally, save 2nd half of result
            bool isCopyGood3 = m_outputBuffer.size() == 2 * halfSize && m_lastOverlapBuffer.size() == halfSize;
            ASSERT(isCopyGood3);
            if (!isCopyGood3)
                return;

            memcpy(m_lastOverlapBuffer.data(), m_outputBuffer.data() + halfSize, sizeof(float) * halfSize);

            // Reset index back to start for next time
            m_readWriteIndex = 0;
        }
    }
}

void FFTConvolver::reset()
{
    m_lastOverlapBuffer.zero();
    m_readWriteIndex = 0;
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
