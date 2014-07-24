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

#ifndef ReverbConvolver_h
#define ReverbConvolver_h

#include "AudioArray.h"
#include "DirectConvolver.h"
#include "FFTConvolver.h"
#include "ReverbAccumulationBuffer.h"
#include "ReverbConvolverStage.h"
#include "ReverbInputBuffer.h"
#include <wtf/OwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>

namespace WebCore {

class AudioChannel;

class ReverbConvolver {
public:
    // maxFFTSize can be adjusted (from say 2048 to 32768) depending on how much precision is necessary.
    // For certain tweaky de-convolving applications the phase errors add up quickly and lead to non-sensical results with
    // larger FFT sizes and single-precision floats.  In these cases 2048 is a good size.
    // If not doing multi-threaded convolution, then should not go > 8192.
    ReverbConvolver(AudioChannel* impulseResponse, size_t renderSliceSize, size_t maxFFTSize, size_t convolverRenderPhase, bool useBackgroundThreads);
    ~ReverbConvolver();

    void process(const AudioChannel* sourceChannel, AudioChannel* destinationChannel, size_t framesToProcess);
    void reset();

    size_t impulseResponseLength() const { return m_impulseResponseLength; }

    ReverbInputBuffer* inputBuffer() { return &m_inputBuffer; }

    bool useBackgroundThreads() const { return m_useBackgroundThreads; }
    void backgroundThreadEntry();

    size_t latencyFrames() const;
private:
    Vector<OwnPtr<ReverbConvolverStage> > m_stages;
    Vector<OwnPtr<ReverbConvolverStage> > m_backgroundStages;
    size_t m_impulseResponseLength;

    ReverbAccumulationBuffer m_accumulationBuffer;

    // One or more background threads read from this input buffer which is fed from the realtime thread.
    ReverbInputBuffer m_inputBuffer;

    // First stage will be of size m_minFFTSize.  Each next stage will be twice as big until we hit m_maxFFTSize.
    size_t m_minFFTSize;
    size_t m_maxFFTSize;

    // But don't exceed this size in the real-time thread (if we're doing background processing).
    size_t m_maxRealtimeFFTSize;

    // Background thread and synchronization
    bool m_useBackgroundThreads;
    ThreadIdentifier m_backgroundThread;
    bool m_wantsToExit;
    bool m_moreInputBuffered;
    mutable Mutex m_backgroundThreadLock;
    mutable ThreadCondition m_backgroundThreadCondition;
};

} // namespace WebCore

#endif // ReverbConvolver_h
