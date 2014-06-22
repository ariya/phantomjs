/*
 * Copyright (C) 2012 Intel Inc. All rights reserved.
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

#include "DirectConvolver.h"

#if OS(DARWIN)
#include <Accelerate/Accelerate.h>
#endif

#include "VectorMath.h"

namespace WebCore {

using namespace VectorMath;
    
DirectConvolver::DirectConvolver(size_t inputBlockSize)
    : m_inputBlockSize(inputBlockSize)
#if USE(WEBAUDIO_IPP)
    , m_overlayBuffer(inputBlockSize)
#endif // USE(WEBAUDIO_IPP)
    , m_buffer(inputBlockSize * 2)
{
}

void DirectConvolver::process(AudioFloatArray* convolutionKernel, const float* sourceP, float* destP, size_t framesToProcess)
{
    ASSERT(framesToProcess == m_inputBlockSize);
    if (framesToProcess != m_inputBlockSize)
        return;

    // Only support kernelSize <= m_inputBlockSize
    size_t kernelSize = convolutionKernel->size();
    ASSERT(kernelSize <= m_inputBlockSize);
    if (kernelSize > m_inputBlockSize)
        return;

    float* kernelP = convolutionKernel->data();

    // Sanity check
    bool isCopyGood = kernelP && sourceP && destP && m_buffer.data();
    ASSERT(isCopyGood);
    if (!isCopyGood)
        return;

#if USE(WEBAUDIO_IPP)
    float* outputBuffer = m_buffer.data();
    float* overlayBuffer = m_overlayBuffer.data();
    bool isCopyGood2 = overlayBuffer && m_overlayBuffer.size() >= kernelSize && m_buffer.size() == m_inputBlockSize * 2;
    ASSERT(isCopyGood2);
    if (!isCopyGood2)
        return;

    ippsConv_32f(static_cast<const Ipp32f*>(sourceP), framesToProcess, static_cast<Ipp32f*>(kernelP), kernelSize, static_cast<Ipp32f*>(outputBuffer));

    vadd(outputBuffer, 1, overlayBuffer, 1, destP, 1, framesToProcess);
    memcpy(overlayBuffer, outputBuffer + m_inputBlockSize, sizeof(float) * kernelSize);
#else
    float* inputP = m_buffer.data() + m_inputBlockSize;

    // Copy samples to 2nd half of input buffer.
    memcpy(inputP, sourceP, sizeof(float) * framesToProcess);

#if OS(DARWIN)
#if defined(__ppc__) || defined(__i386__)
    conv(inputP - kernelSize + 1, 1, kernelP + kernelSize - 1, -1, destP, 1, framesToProcess, kernelSize);
#else
    vDSP_conv(inputP - kernelSize + 1, 1, kernelP + kernelSize - 1, -1, destP, 1, framesToProcess, kernelSize);
#endif // defined(__ppc__) || defined(__i386__)
#else
    // FIXME: The macro can be further optimized to avoid pipeline stalls. One possibility is to maintain 4 separate sums and change the macro to CONVOLVE_FOUR_SAMPLES.
#define CONVOLVE_ONE_SAMPLE             \
    sum += inputP[i - j] * kernelP[j];  \
    j++;

    size_t i = 0;
    while (i < framesToProcess) {
        size_t j = 0;
        float sum = 0;
        
        // FIXME: SSE optimization may be applied here.
        if (kernelSize == 32) {
            CONVOLVE_ONE_SAMPLE // 1
            CONVOLVE_ONE_SAMPLE // 2
            CONVOLVE_ONE_SAMPLE // 3
            CONVOLVE_ONE_SAMPLE // 4
            CONVOLVE_ONE_SAMPLE // 5
            CONVOLVE_ONE_SAMPLE // 6
            CONVOLVE_ONE_SAMPLE // 7
            CONVOLVE_ONE_SAMPLE // 8
            CONVOLVE_ONE_SAMPLE // 9
            CONVOLVE_ONE_SAMPLE // 10

            CONVOLVE_ONE_SAMPLE // 11
            CONVOLVE_ONE_SAMPLE // 12
            CONVOLVE_ONE_SAMPLE // 13
            CONVOLVE_ONE_SAMPLE // 14
            CONVOLVE_ONE_SAMPLE // 15
            CONVOLVE_ONE_SAMPLE // 16
            CONVOLVE_ONE_SAMPLE // 17
            CONVOLVE_ONE_SAMPLE // 18
            CONVOLVE_ONE_SAMPLE // 19
            CONVOLVE_ONE_SAMPLE // 20

            CONVOLVE_ONE_SAMPLE // 21
            CONVOLVE_ONE_SAMPLE // 22
            CONVOLVE_ONE_SAMPLE // 23
            CONVOLVE_ONE_SAMPLE // 24
            CONVOLVE_ONE_SAMPLE // 25
            CONVOLVE_ONE_SAMPLE // 26
            CONVOLVE_ONE_SAMPLE // 27
            CONVOLVE_ONE_SAMPLE // 28
            CONVOLVE_ONE_SAMPLE // 29
            CONVOLVE_ONE_SAMPLE // 30

            CONVOLVE_ONE_SAMPLE // 31
            CONVOLVE_ONE_SAMPLE // 32

        } else if (kernelSize == 64) {
            CONVOLVE_ONE_SAMPLE // 1
            CONVOLVE_ONE_SAMPLE // 2
            CONVOLVE_ONE_SAMPLE // 3
            CONVOLVE_ONE_SAMPLE // 4
            CONVOLVE_ONE_SAMPLE // 5
            CONVOLVE_ONE_SAMPLE // 6
            CONVOLVE_ONE_SAMPLE // 7
            CONVOLVE_ONE_SAMPLE // 8
            CONVOLVE_ONE_SAMPLE // 9
            CONVOLVE_ONE_SAMPLE // 10

            CONVOLVE_ONE_SAMPLE // 11
            CONVOLVE_ONE_SAMPLE // 12
            CONVOLVE_ONE_SAMPLE // 13
            CONVOLVE_ONE_SAMPLE // 14
            CONVOLVE_ONE_SAMPLE // 15
            CONVOLVE_ONE_SAMPLE // 16
            CONVOLVE_ONE_SAMPLE // 17
            CONVOLVE_ONE_SAMPLE // 18
            CONVOLVE_ONE_SAMPLE // 19
            CONVOLVE_ONE_SAMPLE // 20

            CONVOLVE_ONE_SAMPLE // 21
            CONVOLVE_ONE_SAMPLE // 22
            CONVOLVE_ONE_SAMPLE // 23
            CONVOLVE_ONE_SAMPLE // 24
            CONVOLVE_ONE_SAMPLE // 25
            CONVOLVE_ONE_SAMPLE // 26
            CONVOLVE_ONE_SAMPLE // 27
            CONVOLVE_ONE_SAMPLE // 28
            CONVOLVE_ONE_SAMPLE // 29
            CONVOLVE_ONE_SAMPLE // 30

            CONVOLVE_ONE_SAMPLE // 31
            CONVOLVE_ONE_SAMPLE // 32
            CONVOLVE_ONE_SAMPLE // 33
            CONVOLVE_ONE_SAMPLE // 34
            CONVOLVE_ONE_SAMPLE // 35
            CONVOLVE_ONE_SAMPLE // 36
            CONVOLVE_ONE_SAMPLE // 37
            CONVOLVE_ONE_SAMPLE // 38
            CONVOLVE_ONE_SAMPLE // 39
            CONVOLVE_ONE_SAMPLE // 40

            CONVOLVE_ONE_SAMPLE // 41
            CONVOLVE_ONE_SAMPLE // 42
            CONVOLVE_ONE_SAMPLE // 43
            CONVOLVE_ONE_SAMPLE // 44
            CONVOLVE_ONE_SAMPLE // 45
            CONVOLVE_ONE_SAMPLE // 46
            CONVOLVE_ONE_SAMPLE // 47
            CONVOLVE_ONE_SAMPLE // 48
            CONVOLVE_ONE_SAMPLE // 49
            CONVOLVE_ONE_SAMPLE // 50

            CONVOLVE_ONE_SAMPLE // 51
            CONVOLVE_ONE_SAMPLE // 52
            CONVOLVE_ONE_SAMPLE // 53
            CONVOLVE_ONE_SAMPLE // 54
            CONVOLVE_ONE_SAMPLE // 55
            CONVOLVE_ONE_SAMPLE // 56
            CONVOLVE_ONE_SAMPLE // 57
            CONVOLVE_ONE_SAMPLE // 58
            CONVOLVE_ONE_SAMPLE // 59
            CONVOLVE_ONE_SAMPLE // 60

            CONVOLVE_ONE_SAMPLE // 61
            CONVOLVE_ONE_SAMPLE // 62
            CONVOLVE_ONE_SAMPLE // 63
            CONVOLVE_ONE_SAMPLE // 64

        } else if (kernelSize == 128) {
            CONVOLVE_ONE_SAMPLE // 1
            CONVOLVE_ONE_SAMPLE // 2
            CONVOLVE_ONE_SAMPLE // 3
            CONVOLVE_ONE_SAMPLE // 4
            CONVOLVE_ONE_SAMPLE // 5
            CONVOLVE_ONE_SAMPLE // 6
            CONVOLVE_ONE_SAMPLE // 7
            CONVOLVE_ONE_SAMPLE // 8
            CONVOLVE_ONE_SAMPLE // 9
            CONVOLVE_ONE_SAMPLE // 10

            CONVOLVE_ONE_SAMPLE // 11
            CONVOLVE_ONE_SAMPLE // 12
            CONVOLVE_ONE_SAMPLE // 13
            CONVOLVE_ONE_SAMPLE // 14
            CONVOLVE_ONE_SAMPLE // 15
            CONVOLVE_ONE_SAMPLE // 16
            CONVOLVE_ONE_SAMPLE // 17
            CONVOLVE_ONE_SAMPLE // 18
            CONVOLVE_ONE_SAMPLE // 19
            CONVOLVE_ONE_SAMPLE // 20

            CONVOLVE_ONE_SAMPLE // 21
            CONVOLVE_ONE_SAMPLE // 22
            CONVOLVE_ONE_SAMPLE // 23
            CONVOLVE_ONE_SAMPLE // 24
            CONVOLVE_ONE_SAMPLE // 25
            CONVOLVE_ONE_SAMPLE // 26
            CONVOLVE_ONE_SAMPLE // 27
            CONVOLVE_ONE_SAMPLE // 28
            CONVOLVE_ONE_SAMPLE // 29
            CONVOLVE_ONE_SAMPLE // 30

            CONVOLVE_ONE_SAMPLE // 31
            CONVOLVE_ONE_SAMPLE // 32
            CONVOLVE_ONE_SAMPLE // 33
            CONVOLVE_ONE_SAMPLE // 34
            CONVOLVE_ONE_SAMPLE // 35
            CONVOLVE_ONE_SAMPLE // 36
            CONVOLVE_ONE_SAMPLE // 37
            CONVOLVE_ONE_SAMPLE // 38
            CONVOLVE_ONE_SAMPLE // 39
            CONVOLVE_ONE_SAMPLE // 40

            CONVOLVE_ONE_SAMPLE // 41
            CONVOLVE_ONE_SAMPLE // 42
            CONVOLVE_ONE_SAMPLE // 43
            CONVOLVE_ONE_SAMPLE // 44
            CONVOLVE_ONE_SAMPLE // 45
            CONVOLVE_ONE_SAMPLE // 46
            CONVOLVE_ONE_SAMPLE // 47
            CONVOLVE_ONE_SAMPLE // 48
            CONVOLVE_ONE_SAMPLE // 49
            CONVOLVE_ONE_SAMPLE // 50

            CONVOLVE_ONE_SAMPLE // 51
            CONVOLVE_ONE_SAMPLE // 52
            CONVOLVE_ONE_SAMPLE // 53
            CONVOLVE_ONE_SAMPLE // 54
            CONVOLVE_ONE_SAMPLE // 55
            CONVOLVE_ONE_SAMPLE // 56
            CONVOLVE_ONE_SAMPLE // 57
            CONVOLVE_ONE_SAMPLE // 58
            CONVOLVE_ONE_SAMPLE // 59
            CONVOLVE_ONE_SAMPLE // 60

            CONVOLVE_ONE_SAMPLE // 61
            CONVOLVE_ONE_SAMPLE // 62
            CONVOLVE_ONE_SAMPLE // 63
            CONVOLVE_ONE_SAMPLE // 64
            CONVOLVE_ONE_SAMPLE // 65
            CONVOLVE_ONE_SAMPLE // 66
            CONVOLVE_ONE_SAMPLE // 67
            CONVOLVE_ONE_SAMPLE // 68
            CONVOLVE_ONE_SAMPLE // 69
            CONVOLVE_ONE_SAMPLE // 70

            CONVOLVE_ONE_SAMPLE // 71
            CONVOLVE_ONE_SAMPLE // 72
            CONVOLVE_ONE_SAMPLE // 73
            CONVOLVE_ONE_SAMPLE // 74
            CONVOLVE_ONE_SAMPLE // 75
            CONVOLVE_ONE_SAMPLE // 76
            CONVOLVE_ONE_SAMPLE // 77
            CONVOLVE_ONE_SAMPLE // 78
            CONVOLVE_ONE_SAMPLE // 79
            CONVOLVE_ONE_SAMPLE // 80

            CONVOLVE_ONE_SAMPLE // 81
            CONVOLVE_ONE_SAMPLE // 82
            CONVOLVE_ONE_SAMPLE // 83
            CONVOLVE_ONE_SAMPLE // 84
            CONVOLVE_ONE_SAMPLE // 85
            CONVOLVE_ONE_SAMPLE // 86
            CONVOLVE_ONE_SAMPLE // 87
            CONVOLVE_ONE_SAMPLE // 88
            CONVOLVE_ONE_SAMPLE // 89
            CONVOLVE_ONE_SAMPLE // 90

            CONVOLVE_ONE_SAMPLE // 91
            CONVOLVE_ONE_SAMPLE // 92
            CONVOLVE_ONE_SAMPLE // 93
            CONVOLVE_ONE_SAMPLE // 94
            CONVOLVE_ONE_SAMPLE // 95
            CONVOLVE_ONE_SAMPLE // 96
            CONVOLVE_ONE_SAMPLE // 97
            CONVOLVE_ONE_SAMPLE // 98
            CONVOLVE_ONE_SAMPLE // 99
            CONVOLVE_ONE_SAMPLE // 100

            CONVOLVE_ONE_SAMPLE // 101
            CONVOLVE_ONE_SAMPLE // 102
            CONVOLVE_ONE_SAMPLE // 103
            CONVOLVE_ONE_SAMPLE // 104
            CONVOLVE_ONE_SAMPLE // 105
            CONVOLVE_ONE_SAMPLE // 106
            CONVOLVE_ONE_SAMPLE // 107
            CONVOLVE_ONE_SAMPLE // 108
            CONVOLVE_ONE_SAMPLE // 109
            CONVOLVE_ONE_SAMPLE // 110

            CONVOLVE_ONE_SAMPLE // 111
            CONVOLVE_ONE_SAMPLE // 112
            CONVOLVE_ONE_SAMPLE // 113
            CONVOLVE_ONE_SAMPLE // 114
            CONVOLVE_ONE_SAMPLE // 115
            CONVOLVE_ONE_SAMPLE // 116
            CONVOLVE_ONE_SAMPLE // 117
            CONVOLVE_ONE_SAMPLE // 118
            CONVOLVE_ONE_SAMPLE // 119
            CONVOLVE_ONE_SAMPLE // 120

            CONVOLVE_ONE_SAMPLE // 121
            CONVOLVE_ONE_SAMPLE // 122
            CONVOLVE_ONE_SAMPLE // 123
            CONVOLVE_ONE_SAMPLE // 124
            CONVOLVE_ONE_SAMPLE // 125
            CONVOLVE_ONE_SAMPLE // 126
            CONVOLVE_ONE_SAMPLE // 127
            CONVOLVE_ONE_SAMPLE // 128
        } else {
            while (j < kernelSize) {
                // Non-optimized using actual while loop.
                CONVOLVE_ONE_SAMPLE
            }
        }
        destP[i++] = sum;
    }
#endif // OS(DARWIN)

    // Copy 2nd half of input buffer to 1st half.
    memcpy(m_buffer.data(), inputP, sizeof(float) * framesToProcess);
#endif
}

void DirectConvolver::reset()
{
    m_buffer.zero();
#if USE(WEBAUDIO_IPP)
    m_overlayBuffer.zero();
#endif // USE(WEBAUDIO_IPP)
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
