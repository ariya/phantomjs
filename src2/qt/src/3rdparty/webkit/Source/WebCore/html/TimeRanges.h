/*
 * Copyright (C) 2007, 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef TimeRanges_h
#define TimeRanges_h

#include "ExceptionCode.h"

#include <algorithm>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {

class TimeRanges : public RefCounted<TimeRanges> {
public:
    static PassRefPtr<TimeRanges> create() 
    {
        return adoptRef(new TimeRanges);
    }
    static PassRefPtr<TimeRanges> create(float start, float end)
    {
        return adoptRef(new TimeRanges(start, end));
    }

    PassRefPtr<TimeRanges> copy();

    unsigned length() const { return m_ranges.size(); }
    float start(unsigned index, ExceptionCode&) const;
    float end(unsigned index, ExceptionCode&) const;
    
    void add(float start, float end);
    
    bool contain(float time) const;
    
    float nearest(float time) const;

private:
    TimeRanges() { }
    TimeRanges(float start, float end);
    TimeRanges(const TimeRanges&);

    // We consider all the Ranges to be semi-bounded as follow: [start, end[
    struct Range {
        Range() { }
        Range(float start, float end)
        {
            m_start = start;
            m_end = end;
        }
        float m_start;
        float m_end;

        inline bool isPointInRange(float point) const
        {
            return m_start <= point && point < m_end;
        }
        
        inline bool isOverlappingRange(const Range& range) const
        {
            return isPointInRange(range.m_start) || isPointInRange(range.m_end) || range.isPointInRange(m_start);
        }

        inline bool isContiguousWithRange(const Range& range) const
        {
            return range.m_start == m_end || range.m_end == m_start;
        }
        
        inline Range unionWithOverlappingOrContiguousRange(const Range& range) const
        {
            Range ret;

            ret.m_start = std::min(m_start, range.m_start);
            ret.m_end = std::max(m_end, range.m_end);

            return ret;
        }

        inline bool isBeforeRange(const Range& range) const
        {
            return range.m_start >= m_end;
        }
    };
    
    Vector<Range> m_ranges;
};

} // namespace WebCore

#endif
