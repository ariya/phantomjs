/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
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

#ifndef AudioResamplerKernel_h
#define AudioResamplerKernel_h

#include "AudioArray.h"

namespace WebCore {

class AudioResampler;

// AudioResamplerKernel does resampling on a single mono channel.
// It uses a simple linear interpolation for good performance.

class AudioResamplerKernel {
public:
    AudioResamplerKernel(AudioResampler*);

    // getSourcePointer() should be called each time before process() is called.
    // Given a number of frames to process (for subsequent call to process()), it returns a pointer and numberOfSourceFramesNeeded
    // where sample data should be copied. This sample data provides the input to the resampler when process() is called.
    // framesToProcess must be less than or equal to MaxFramesToProcess.
    float* getSourcePointer(size_t framesToProcess, size_t* numberOfSourceFramesNeeded);

    // process() resamples framesToProcess frames from the source into destination.
    // Each call to process() must be preceded by a call to getSourcePointer() so that source input may be supplied.
    // framesToProcess must be less than or equal to MaxFramesToProcess.
    void process(float* destination, size_t framesToProcess);

    // Resets the processing state.
    void reset();

    static const size_t MaxFramesToProcess;

private:
    double rate() const;

    AudioResampler* m_resampler;
    AudioFloatArray m_sourceBuffer;
    
    // This is a (floating point) read index on the input stream.
    double m_virtualReadIndex;

    // We need to have continuity from one call of process() to the next.
    // m_lastValues stores the last two sample values from the last call to process().
    // m_fillIndex represents how many buffered samples we have which can be as many as 2.
    // For the first call to process() (or after reset()) there will be no buffered samples.
    float m_lastValues[2];
    unsigned m_fillIndex;
};

} // namespace WebCore

#endif // AudioResamplerKernel_h
