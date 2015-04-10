/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef GridTrackSize_h
#define GridTrackSize_h

#include "Length.h"

namespace WebCore {

enum GridTrackSizeType {
    LengthTrackSizing,
    MinMaxTrackSizing
};

class GridTrackSize {
public:
    GridTrackSize(LengthType type = Undefined)
        : m_type(LengthTrackSizing)
        , m_minTrackBreadth(type)
        , m_maxTrackBreadth(type)
    {
    }

    const Length& length() const
    {
        ASSERT(m_type == LengthTrackSizing);
        ASSERT(!m_minTrackBreadth.isUndefined());
        ASSERT(m_minTrackBreadth == m_maxTrackBreadth);
        return m_minTrackBreadth;
    }

    void setLength(const Length& length)
    {
        m_type = LengthTrackSizing;
        m_minTrackBreadth = length;
        m_maxTrackBreadth = length;
    }

    const Length& minTrackBreadth() const
    {
        ASSERT(!m_minTrackBreadth.isUndefined());
        if (m_minTrackBreadth.isAuto()) {
            DEFINE_STATIC_LOCAL(Length, minContent, (MinContent));
            return minContent;
        }
        return m_minTrackBreadth;
    }

    const Length& maxTrackBreadth() const
    {
        ASSERT(!m_maxTrackBreadth.isUndefined());
        if (m_maxTrackBreadth.isAuto()) {
            DEFINE_STATIC_LOCAL(Length, maxContent, (MaxContent));
            return maxContent;
        }
        return m_maxTrackBreadth;
    }

    void setMinMax(const Length& minTrackBreadth, const Length& maxTrackBreadth)
    {
        m_type = MinMaxTrackSizing;
        m_minTrackBreadth = minTrackBreadth;
        m_maxTrackBreadth = maxTrackBreadth;
    }

    GridTrackSizeType type() const { return m_type; }

    bool operator==(const GridTrackSize& other) const
    {
        return m_type == other.m_type && m_minTrackBreadth == other.m_minTrackBreadth && m_maxTrackBreadth == other.m_maxTrackBreadth;
    }

    bool hasMinOrMaxContentMinTrackBreadth() const { return minTrackBreadth().isMinContent() || minTrackBreadth().isMaxContent(); }
    bool hasMaxContentMinTrackBreadth() const { return minTrackBreadth().isMaxContent(); }
    bool hasMinOrMaxContentMaxTrackBreadth() const { return maxTrackBreadth().isMinContent() || maxTrackBreadth().isMaxContent(); }
    bool hasMaxContentMaxTrackBreadth() const { return maxTrackBreadth().isMaxContent(); }

private:
    GridTrackSizeType m_type;
    Length m_minTrackBreadth;
    Length m_maxTrackBreadth;
};

} // namespace WebCore

#endif // GridTrackSize_h
