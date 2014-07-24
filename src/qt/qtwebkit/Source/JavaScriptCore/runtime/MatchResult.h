/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef MatchResult_h
#define MatchResult_h

typedef uint64_t EncodedMatchResult;

struct MatchResult {
    ALWAYS_INLINE MatchResult(size_t start, size_t end)
        : start(start)
        , end(end)
    {
    }

    explicit ALWAYS_INLINE MatchResult(EncodedMatchResult encoded)
    {
        union u {
            uint64_t encoded;
            struct s {
                size_t start;
                size_t end;
            } split;
        } value;
        value.encoded = encoded;
        start = value.split.start;
        end = value.split.end;
    }

    ALWAYS_INLINE static MatchResult failed()
    {
        return MatchResult(WTF::notFound, 0);
    }

    ALWAYS_INLINE operator bool()
    {
        return start != WTF::notFound;
    }

    ALWAYS_INLINE bool empty()
    {
        return start == end;
    }

    size_t start;
    size_t end;
};

#endif
