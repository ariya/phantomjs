/*
 * Copyright (C) 2013 Google, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef BackgroundHTMLInputStream_h
#define BackgroundHTMLInputStream_h

#if ENABLE(THREADED_HTML_PARSER)

#include "SegmentedString.h"
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

typedef size_t HTMLInputCheckpoint;

class BackgroundHTMLInputStream {
    WTF_MAKE_NONCOPYABLE(BackgroundHTMLInputStream);
public:
    BackgroundHTMLInputStream();

    void append(const String&);
    void close();

    SegmentedString& current() { return m_current; }

    // An HTMLInputCheckpoint is valid until the next call to rewindTo, at which
    // point all outstanding checkpoints are invalidated.
    HTMLInputCheckpoint createCheckpoint();
    void rewindTo(HTMLInputCheckpoint, const String& unparsedInput);
    void invalidateCheckpointsBefore(HTMLInputCheckpoint);

    size_t outstandingCheckpointCount() const { return m_checkpoints.size() - m_firstValidCheckpointIndex; }

private:
    struct Checkpoint {
        Checkpoint(const SegmentedString& i, size_t n) : input(i), numberOfSegmentsAlreadyAppended(n) { }

        SegmentedString input;
        size_t numberOfSegmentsAlreadyAppended;

#ifndef NDEBUG
        bool isNull() const { return input.isEmpty() && !numberOfSegmentsAlreadyAppended; }
#endif
        void clear() { input.clear(); numberOfSegmentsAlreadyAppended = 0; }
    };

    SegmentedString m_current;
    Vector<String> m_segments;
    Vector<Checkpoint> m_checkpoints;

    // Note: These indicies may === vector.size(), in which case there are no valid checkpoints/segments at this time.
    size_t m_firstValidCheckpointIndex;
    size_t m_firstValidSegmentIndex;
};

}

#endif // ENABLE(THREADED_HTML_PARSER)

#endif
