/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#include "UpSampler.h"

#include <wtf/MathExtras.h>

namespace WebCore {

UpSampler::UpSampler(size_t inputBlockSize)
    : m_inputBlockSize(inputBlockSize)
    , m_kernel(DefaultKernelSize)
    , m_convolver(inputBlockSize)
    , m_tempBuffer(inputBlockSize)
    , m_inputBuffer(inputBlockSize * 2)
{
    initializeKernel();
}

void UpSampler::initializeKernel()
{
    // Blackman window parameters.
    double alpha = 0.16;
    double a0 = 0.5 * (1.0 - alpha);
    double a1 = 0.5;
    double a2 = 0.5 * alpha;

    int n = m_kernel.size();
    int halfSize = n / 2;
    double subsampleOffset = -0.5;

    for (int i = 0; i < n; ++i) {
        // Compute the sinc() with offset.
        double s = piDouble * (i - halfSize - subsampleOffset);
        double sinc = !s ? 1.0 : sin(s) / s;

        // Compute Blackman window, matching the offset of the sinc().
        double x = (i - subsampleOffset) / n;
        double window = a0 - a1 * cos(2.0 * piDouble * x) + a2 * cos(4.0 * piDouble * x);

        // Window the sinc() function.
        m_kernel[i] = sinc * window;
    }
}

void UpSampler::process(const float* sourceP, float* destP, size_t sourceFramesToProcess)
{
    bool isInputBlockSizeGood = sourceFramesToProcess == m_inputBlockSize;
    ASSERT(isInputBlockSizeGood);
    if (!isInputBlockSizeGood)
        return;

    bool isTempBufferGood = sourceFramesToProcess == m_tempBuffer.size();
    ASSERT(isTempBufferGood);
    if (!isTempBufferGood)
        return;

    bool isKernelGood = m_kernel.size() == DefaultKernelSize;
    ASSERT(isKernelGood);
    if (!isKernelGood)
        return;

    size_t halfSize = m_kernel.size() / 2;

    // Copy source samples to 2nd half of input buffer.
    bool isInputBufferGood = m_inputBuffer.size() == sourceFramesToProcess * 2 && halfSize <= sourceFramesToProcess;
    ASSERT(isInputBufferGood);
    if (!isInputBufferGood)
        return;

    float* inputP = m_inputBuffer.data() + sourceFramesToProcess;
    memcpy(inputP, sourceP, sizeof(float) * sourceFramesToProcess);

    // Copy even sample-frames 0,2,4,6... (delayed by the linear phase delay) directly into destP.
    for (unsigned i = 0; i < sourceFramesToProcess; ++i)
        destP[i * 2] = *((inputP - halfSize) + i);

    // Compute odd sample-frames 1,3,5,7...
    float* oddSamplesP = m_tempBuffer.data();
    m_convolver.process(&m_kernel, sourceP, oddSamplesP, sourceFramesToProcess);

    for (unsigned i = 0; i < sourceFramesToProcess; ++i)
        destP[i * 2 + 1] = oddSamplesP[i];

    // Copy 2nd half of input buffer to 1st half.
    memcpy(m_inputBuffer.data(), inputP, sizeof(float) * sourceFramesToProcess);
}

void UpSampler::reset()
{
    m_convolver.reset();
    m_inputBuffer.zero();
}

size_t UpSampler::latencyFrames() const
{
    // Divide by two since this is a linear phase kernel and the delay is at the center of the kernel.
    return m_kernel.size() / 2;
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
