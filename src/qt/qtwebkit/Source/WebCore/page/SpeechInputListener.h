/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SpeechInputListener_h
#define SpeechInputListener_h

#if ENABLE(INPUT_SPEECH)

#include "SpeechInputResult.h"
#include <wtf/Forward.h>

namespace WebCore {

// Interface to be implemented by the element which invokes SpeechInput.
class SpeechInputListener {
public:
    // Informs that audio recording has completed and recognition is underway.
    virtual void didCompleteRecording(int requestId) = 0;

    // Informs that speech recognition has completed. This gets invoked irrespective of whether
    // recognition was succesful or not, whether setRecognitionResult() was invoked or not. The
    // handler typically frees up any temporary resources allocated and waits for the next speech
    // recognition request.
    virtual void didCompleteRecognition(int requestId) = 0;

    // Gives results from speech recognition, either partial or the final results.
    // This method can potentially get called multiple times if there are partial results
    // available as the user keeps speaking. If the speech could not be recognized properly
    // or if there was any other errors in the process, this method may never be called.
    virtual void setRecognitionResult(int requestId, const SpeechInputResultArray&) = 0;

protected:
    virtual ~SpeechInputListener() { }
};

} // namespace WebCore

#endif // ENABLE(INPUT_SPEECH)

#endif // SpeechInputListener_h
