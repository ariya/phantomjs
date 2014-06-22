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

#ifndef UpSampler_h
#define UpSampler_h

#include "AudioArray.h"
#include "DirectConvolver.h"

namespace WebCore {

// UpSampler up-samples the source stream by a factor of 2x.

class UpSampler {
public:
    UpSampler(size_t inputBlockSize);

    // The destination buffer |destP| is of size sourceFramesToProcess * 2.
    void process(const float* sourceP, float* destP, size_t sourceFramesToProcess);

    void reset();

    // Latency based on the source sample-rate.
    size_t latencyFrames() const;

private:
    enum { DefaultKernelSize = 128 };

    size_t m_inputBlockSize;

    // Computes ideal band-limited filter coefficients to sample in between each source sample-frame.
    // This filter will be used to compute the odd sample-frames of the output.
    void initializeKernel();
    AudioFloatArray m_kernel;

    // Computes the odd sample-frames of the output.
    DirectConvolver m_convolver;

    AudioFloatArray m_tempBuffer;

    // Delay line for generating the even sample-frames of the output.
    // The source samples are delayed exactly to match the linear phase delay of the FIR filter (convolution)
    // used to generate the odd sample-frames of the output.
    AudioFloatArray m_inputBuffer;
};

} // namespace WebCore

#endif // UpSampler_h
