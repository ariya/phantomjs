/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
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

#ifndef WebVTTTokenizer_h
#define WebVTTTokenizer_h

#if ENABLE(VIDEO_TRACK)

#include "InputStreamPreprocessor.h"
#include "WebVTTToken.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class WebVTTTokenizerState {
public:
    enum State {
        DataState,
        EscapeState,
        TagState,
        StartTagState,
        StartTagClassState,
        StartTagAnnotationState,
        EndTagState,
        EndTagOpenState,
        TimestampTagState,
    };
};

class WebVTTTokenizer {
    WTF_MAKE_NONCOPYABLE(WebVTTTokenizer);
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<WebVTTTokenizer> create() { return adoptPtr(new WebVTTTokenizer); }

    typedef WebVTTTokenizerState State;

    void reset();
    
    bool nextToken(SegmentedString&, WebVTTToken&);

    inline bool haveBufferedCharacterToken()
    {
        return m_token->type() == WebVTTToken::Type::Character;
    }

    inline void bufferCharacter(UChar character)
    {
        ASSERT(character != kEndOfFileMarker);
        m_token->ensureIsCharacterToken();
        m_token->appendToCharacter(character);
    }

    inline bool emitAndResumeIn(SegmentedString& source, State::State state)
    {
        m_state = state;
        source.advanceAndUpdateLineNumber();
        return true;
    }

    inline bool emitEndOfFile(SegmentedString& source)
    {
        if (haveBufferedCharacterToken())
            return true;
        m_state = State::DataState;
        source.advanceAndUpdateLineNumber();
        m_token->clear();
        m_token->makeEndOfFile();
        return true;
    }

    bool shouldSkipNullCharacters() const { return true; }

private:
    WebVTTTokenizer();

    // m_token is owned by the caller. If nextToken is not on the stack,
    // this member might be pointing to unallocated memory.
    WebVTTToken* m_token;
    WebVTTTokenizerState::State m_state;

    Vector<LChar, 32> m_buffer;

    // ://www.whatwg.org/specs/web-apps/current-work/#preprocessing-the-input-stream
    InputStreamPreprocessor<WebVTTTokenizer> m_inputStreamPreprocessor;
};

}

#endif
#endif
