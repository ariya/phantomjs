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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Pagination_h
#define Pagination_h

namespace WebCore {

struct Pagination {
    enum Mode { Unpaginated, LeftToRightPaginated, RightToLeftPaginated, TopToBottomPaginated, BottomToTopPaginated };

    Pagination()
        : mode(Unpaginated)
        , behavesLikeColumns(false)
        , pageLength(0)
        , gap(0)
    {
    };

    bool operator==(const Pagination& other) const
    {
        return mode == other.mode && behavesLikeColumns == other.behavesLikeColumns && pageLength == other.pageLength && gap == other.gap;
    }

    bool operator!=(const Pagination& other) const
    {
        return mode != other.mode || behavesLikeColumns != other.behavesLikeColumns || pageLength != other.pageLength || gap != other.gap;
    }

    Mode mode;
    bool behavesLikeColumns;
    unsigned pageLength;
    unsigned gap;
};

} // namespace WebCore

#endif
