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

#ifndef ReverbInputBuffer_h
#define ReverbInputBuffer_h

#include "AudioArray.h"

namespace WebCore {

// ReverbInputBuffer is used to buffer input samples for deferred processing by the background threads.
class ReverbInputBuffer {
public:
    ReverbInputBuffer(size_t length);

    // The realtime audio thread keeps writing samples here.
    // The assumption is that the buffer's length is evenly divisible by numberOfFrames (for nearly all cases this will be fine).
    // FIXME: remove numberOfFrames restriction...
    void write(const float* sourceP, size_t numberOfFrames);

    // Background threads can call this to check if there's anything to read...
    size_t writeIndex() const { return m_writeIndex; }

    // The individual background threads read here (and hope that they can keep up with the buffer writing).
    // readIndex is updated with the next readIndex to read from...
    // The assumption is that the buffer's length is evenly divisible by numberOfFrames.
    // FIXME: remove numberOfFrames restriction...
    float* directReadFrom(int* readIndex, size_t numberOfFrames);

    void reset();

private:
    AudioFloatArray m_buffer;
    size_t m_writeIndex;
};

} // namespace WebCore

#endif // ReverbInputBuffer_h
