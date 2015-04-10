/*
 * Copyright (c) 2011 Google Inc. All rights reserved.
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

#ifndef TextChecking_h
#define TextChecking_h

#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

enum TextCheckingType {
    TextCheckingTypeNone        = 0,
    TextCheckingTypeSpelling    = 1 << 1,
    TextCheckingTypeGrammar     = 1 << 2,
    TextCheckingTypeLink        = 1 << 5,
    TextCheckingTypeQuote       = 1 << 6,
    TextCheckingTypeDash        = 1 << 7,
    TextCheckingTypeReplacement = 1 << 8,
    TextCheckingTypeCorrection  = 1 << 9,
    TextCheckingTypeShowCorrectionPanel = 1 << 10
};

typedef unsigned TextCheckingTypeMask;

enum TextCheckingProcessType {
    TextCheckingProcessBatch,
    TextCheckingProcessIncremental
};

struct GrammarDetail {
    int location;
    int length;
    Vector<String> guesses;
    String userDescription;
};

struct TextCheckingResult {
    TextCheckingType type;
    int location;
    int length;
    Vector<GrammarDetail> details;
    String replacement;
};

const int unrequestedTextCheckingSequence = -1;

class TextCheckingRequestData {
    friend class SpellCheckRequest; // For access to m_sequence.
public:
    TextCheckingRequestData()
        : m_sequence(unrequestedTextCheckingSequence)
        , m_mask(TextCheckingTypeNone)
        , m_processType(TextCheckingProcessIncremental)
    { }
    TextCheckingRequestData(int sequence, const String& text, TextCheckingTypeMask mask, TextCheckingProcessType processType)
        : m_sequence(sequence)
        , m_text(text)
        , m_mask(mask)
        , m_processType(processType)
    { }

    int sequence() const { return m_sequence; }
    String text() const { return m_text; }
    TextCheckingTypeMask mask() const { return m_mask; }
    TextCheckingProcessType processType() const { return m_processType; }

private:
    int m_sequence;
    String m_text;
    TextCheckingTypeMask m_mask;
    TextCheckingProcessType m_processType;
};

class TextCheckingRequest : public RefCounted<TextCheckingRequest> {
public:
    virtual ~TextCheckingRequest() { }

    virtual const TextCheckingRequestData& data() const = 0;
    virtual void didSucceed(const Vector<TextCheckingResult>&) = 0;
    virtual void didCancel() = 0;
};

}

#endif // TextChecking_h
