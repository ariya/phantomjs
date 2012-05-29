/*
 * Copyright (C) 2007, 2009, 2010 Apple Inc.  All rights reserved.
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

#include "config.h"

#include "TimeRanges.h"

#include <math.h>

using namespace WebCore;

TimeRanges::TimeRanges(float start, float end)
{
    add(start, end);
}

PassRefPtr<TimeRanges> TimeRanges::copy()
{
    RefPtr<TimeRanges> newSession = TimeRanges::create();
    
    unsigned size = m_ranges.size();
    for (unsigned i = 0; i < size; i++)
        newSession->add(m_ranges[i].m_start, m_ranges[i].m_end);
    
    return newSession.release();
}

float TimeRanges::start(unsigned index, ExceptionCode& ec) const 
{ 
    if (index >= length()) {
        ec = INDEX_SIZE_ERR;
        return 0;
    }
    return m_ranges[index].m_start;
}

float TimeRanges::end(unsigned index, ExceptionCode& ec) const 
{ 
    if (index >= length()) {
        ec = INDEX_SIZE_ERR;
        return 0;
    }
    return m_ranges[index].m_end;
}

void TimeRanges::add(float start, float end) 
{
    ASSERT(start <= end);
    unsigned int overlappingArcIndex;
    Range addedRange(start, end);

    // For each present range check if we need to:
    // - merge with the added range, in case we are overlapping or contiguous
    // - Need to insert in place, we we are completely, not overlapping and not contiguous
    // in between two ranges.
    //
    // TODO: Given that we assume that ranges are correctly ordered, this could be optimized.

    for (overlappingArcIndex = 0; overlappingArcIndex < m_ranges.size(); overlappingArcIndex++) {
        if (addedRange.isOverlappingRange(m_ranges[overlappingArcIndex])
         || addedRange.isContiguousWithRange(m_ranges[overlappingArcIndex])) {
            // We need to merge the addedRange and that range.
            addedRange = addedRange.unionWithOverlappingOrContiguousRange(m_ranges[overlappingArcIndex]);
            m_ranges.remove(overlappingArcIndex);
            overlappingArcIndex--;
        } else {
            // Check the case for which there is no more to do
            if (!overlappingArcIndex) {
                if (addedRange.isBeforeRange(m_ranges[0])) {
                    // First index, and we are completely before that range (and not contiguous, nor overlapping).
                    // We just need to be inserted here.
                    break;
                }
            } else {
                if (m_ranges[overlappingArcIndex - 1].isBeforeRange(addedRange)
                 && addedRange.isBeforeRange(m_ranges[overlappingArcIndex])) {
                    // We are exactly after the current previous range, and before the current range, while
                    // not overlapping with none of them. Insert here.
                    break;
                }
            }
        }
    }

    // Now that we are sure we don't overlap with any range, just add it.
    m_ranges.insert(overlappingArcIndex, addedRange);
}

bool TimeRanges::contain(float time) const
{ 
    ExceptionCode unused;
    for (unsigned n = 0; n < length(); n++) {
        if (time >= start(n, unused) && time <= end(n, unused))
            return true;
    }
    return false;
}

float TimeRanges::nearest(float time) const
{ 
    ExceptionCode unused;
    float closest = 0;
    unsigned count = length();
    for (unsigned ndx = 0; ndx < count; ndx++) {
        float startTime = start(ndx, unused);
        float endTime = end(ndx, unused);
        if (time >= startTime && time <= endTime)
            return time;
        if (fabs(startTime - time) < closest)
            closest = fabsf(startTime - time);
        else if (fabs(endTime - time) < closest)
            closest = fabsf(endTime - time);
    }
    return closest;
}
