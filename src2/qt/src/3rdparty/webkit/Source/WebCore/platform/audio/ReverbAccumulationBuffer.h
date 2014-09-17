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

#ifndef ReverbAccumulationBuffer_h
#define ReverbAccumulationBuffer_h

#include "AudioArray.h"

namespace WebCore {

// ReverbAccumulationBuffer is a circular delay buffer with one client reading from it and multiple clients
// writing/accumulating to it at different delay offsets from the read position.  The read operation will zero the memory
// just read from the buffer, so it will be ready for accumulation the next time around.
class ReverbAccumulationBuffer {
public:
    ReverbAccumulationBuffer(size_t length);

    // This will read from, then clear-out numberOfFrames
    void readAndClear(float* destination, size_t numberOfFrames);

    // Each ReverbConvolverStage will accumulate its output at the appropriate delay from the read position.
    // We need to pass in and update readIndex here, since each ReverbConvolverStage may be running in
    // a different thread than the realtime thread calling ReadAndClear() and maintaining m_readIndex
    // Returns the writeIndex where the accumulation took place
    int accumulate(float* source, size_t numberOfFrames, int* readIndex, size_t delayFrames);

    size_t readIndex() const { return m_readIndex; }
    void updateReadIndex(int* readIndex, size_t numberOfFrames) const;

    size_t readTimeFrame() const { return m_readTimeFrame; }

    void reset();

private:
    AudioFloatArray m_buffer;
    size_t m_readIndex;
    size_t m_readTimeFrame; // for debugging (frame on continuous timeline)
};

} // namespace WebCore

#endif // ReverbAccumulationBuffer_h
