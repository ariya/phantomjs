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

#include "DownSampler.h"

#include <wtf/MathExtras.h>

namespace WebCore {

DownSampler::DownSampler(size_t inputBlockSize)
    : m_inputBlockSize(inputBlockSize)
    , m_reducedKernel(DefaultKernelSize / 2)
    , m_convolver(inputBlockSize / 2) // runs at 1/2 source sample-rate
    , m_tempBuffer(inputBlockSize / 2)
    , m_inputBuffer(inputBlockSize * 2)
{
    initializeKernel();
}

void DownSampler::initializeKernel()
{
    // Blackman window parameters.
    double alpha = 0.16;
    double a0 = 0.5 * (1.0 - alpha);
    double a1 = 0.5;
    double a2 = 0.5 * alpha;

    int n = DefaultKernelSize;
    int halfSize = n / 2;

    // Half-band filter.
    double sincScaleFactor = 0.5;

    // Compute only the odd terms because the even ones are zero, except
    // right in the middle at halfSize, which is 0.5 and we'll handle specially during processing
    // after doing the main convolution using m_reducedKernel.
    for (int i = 1; i < n; i += 2) {
        // Compute the sinc() with offset.
        double s = sincScaleFactor * piDouble * (i - halfSize);
        double sinc = !s ? 1.0 : sin(s) / s;
        sinc *= sincScaleFactor;

        // Compute Blackman window, matching the offset of the sinc().
        double x = static_cast<double>(i) / n;
        double window = a0 - a1 * cos(2.0 * piDouble * x) + a2 * cos(4.0 * piDouble * x);

        // Window the sinc() function.
        // Then store only the odd terms in the kernel.
        // In a sense, this is shifting forward in time by one sample-frame at the destination sample-rate.
        m_reducedKernel[(i - 1) / 2] = sinc * window;
    }
}

void DownSampler::process(const float* sourceP, float* destP, size_t sourceFramesToProcess)
{
    bool isInputBlockSizeGood = sourceFramesToProcess == m_inputBlockSize;
    ASSERT(isInputBlockSizeGood);
    if (!isInputBlockSizeGood)
        return;

    size_t destFramesToProcess = sourceFramesToProcess / 2;

    bool isTempBufferGood = destFramesToProcess == m_tempBuffer.size();
    ASSERT(isTempBufferGood);
    if (!isTempBufferGood)
        return;

    bool isReducedKernelGood = m_reducedKernel.size() == DefaultKernelSize / 2;
    ASSERT(isReducedKernelGood);
    if (!isReducedKernelGood)
        return;

    size_t halfSize = DefaultKernelSize / 2;

    // Copy source samples to 2nd half of input buffer.
    bool isInputBufferGood = m_inputBuffer.size() == sourceFramesToProcess * 2 && halfSize <= sourceFramesToProcess;
    ASSERT(isInputBufferGood);
    if (!isInputBufferGood)
        return;

    float* inputP = m_inputBuffer.data() + sourceFramesToProcess;
    memcpy(inputP, sourceP, sizeof(float) * sourceFramesToProcess);

    // Copy the odd sample-frames from sourceP, delayed by one sample-frame (destination sample-rate)
    // to match shifting forward in time in m_reducedKernel.
    float* oddSamplesP = m_tempBuffer.data();
    for (unsigned i = 0; i < destFramesToProcess; ++i)
        oddSamplesP[i] = *((inputP - 1) + i * 2);

    // Actually process oddSamplesP with m_reducedKernel for efficiency.
    // The theoretical kernel is double this size with 0 values for even terms (except center).
    m_convolver.process(&m_reducedKernel, oddSamplesP, destP, destFramesToProcess);

    // Now, account for the 0.5 term right in the middle of the kernel.
    // This amounts to a delay-line of length halfSize (at the source sample-rate),
    // scaled by 0.5.

    // Sum into the destination.
    for (unsigned i = 0; i < destFramesToProcess; ++i)
        destP[i] += 0.5 * *((inputP - halfSize) + i * 2);

    // Copy 2nd half of input buffer to 1st half.
    memcpy(m_inputBuffer.data(), inputP, sizeof(float) * sourceFramesToProcess);
}

void DownSampler::reset()
{
    m_convolver.reset();
    m_inputBuffer.zero();
}

size_t DownSampler::latencyFrames() const
{
    // Divide by two since this is a linear phase kernel and the delay is at the center of the kernel.
    return m_reducedKernel.size() / 2;
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
