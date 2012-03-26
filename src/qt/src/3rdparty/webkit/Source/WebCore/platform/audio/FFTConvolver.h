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

#ifndef FFTConvolver_h
#define FFTConvolver_h

#include "AudioArray.h"
#include "FFTFrame.h"

namespace WebCore {

class FFTConvolver {
public:
    // fftSize must be a power of two
    FFTConvolver(size_t fftSize);

    // For now, with multiple calls to Process(), framesToProcess MUST add up EXACTLY to fftSize / 2
    //
    // FIXME: Later, we can do more sophisticated buffering to relax this requirement...
    //
    // The input to output latency is equal to fftSize / 2
    //
    // Processing in-place is allowed...
    void process(FFTFrame* fftKernel, float* sourceP, float* destP, size_t framesToProcess);

    void reset();

    size_t fftSize() const { return m_frame.fftSize(); }

private:
    FFTFrame m_frame;

    // Buffer input until we get fftSize / 2 samples then do an FFT
    size_t m_readWriteIndex;
    AudioFloatArray m_inputBuffer;

    // Stores output which we read a little at a time
    AudioFloatArray m_outputBuffer;

    // Saves the 2nd half of the FFT buffer, so we can do an overlap-add with the 1st half of the next one
    AudioFloatArray m_lastOverlapBuffer;
};

} // namespace WebCore

#endif // FFTConvolver_h
