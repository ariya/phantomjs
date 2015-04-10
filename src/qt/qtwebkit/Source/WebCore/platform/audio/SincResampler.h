/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef SincResampler_h
#define SincResampler_h

#include "AudioArray.h"
#include "AudioSourceProvider.h"

namespace WebCore {

// SincResampler is a high-quality sample-rate converter.

class SincResampler {
public:   
    // scaleFactor == sourceSampleRate / destinationSampleRate
    // kernelSize can be adjusted for quality (higher is better)
    // numberOfKernelOffsets is used for interpolation and is the number of sub-sample kernel shifts.
    SincResampler(double scaleFactor, unsigned kernelSize = 32, unsigned numberOfKernelOffsets = 32);
    
    // Processes numberOfSourceFrames from source to produce numberOfSourceFrames / scaleFactor frames in destination.
    void process(const float* source, float* destination, unsigned numberOfSourceFrames);

    // Process with input source callback function for streaming applications.
    void process(AudioSourceProvider*, float* destination, size_t framesToProcess);

protected:
    void initializeKernel();
    void consumeSource(float* buffer, unsigned numberOfSourceFrames);
    
    double m_scaleFactor;
    unsigned m_kernelSize;
    unsigned m_numberOfKernelOffsets;

    // m_kernelStorage has m_numberOfKernelOffsets kernels back-to-back, each of size m_kernelSize.
    // The kernel offsets are sub-sample shifts of a windowed sinc() shifted from 0.0 to 1.0 sample.
    AudioFloatArray m_kernelStorage;
    
    // m_virtualSourceIndex is an index on the source input buffer with sub-sample precision.
    // It must be double precision to avoid drift.
    double m_virtualSourceIndex;
    
    // This is the number of destination frames we generate per processing pass on the buffer.
    unsigned m_blockSize;

    // Source is copied into this buffer for each processing pass.
    AudioFloatArray m_inputBuffer;

    const float* m_source;
    unsigned m_sourceFramesAvailable;
    
    // m_sourceProvider is used to provide the audio input stream to the resampler.
    AudioSourceProvider* m_sourceProvider;    

    // The buffer is primed once at the very beginning of processing.
    bool m_isBufferPrimed;
};

} // namespace WebCore

#endif // SincResampler_h
