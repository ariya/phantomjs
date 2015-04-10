/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef ColumnInfo_h
#define ColumnInfo_h

#include "LayoutUnit.h"
#include <wtf/Vector.h>

namespace WebCore {

class ColumnInfo {
    WTF_MAKE_NONCOPYABLE(ColumnInfo); WTF_MAKE_FAST_ALLOCATED;
public:
    ColumnInfo()
        : m_desiredColumnWidth(0)
        , m_desiredColumnCount(1)
        , m_progressionAxis(InlineAxis)
        , m_progressionIsReversed(false)
        , m_columnCount(1)
        , m_columnHeight(0)
        , m_minimumColumnHeight(0)
        , m_forcedBreaks(0)
        , m_maximumDistanceBetweenForcedBreaks(0)
        , m_forcedBreakOffset(0)
        , m_paginationUnit(Column)
    {
    }

    LayoutUnit desiredColumnWidth() const { return m_desiredColumnWidth; }
    void setDesiredColumnWidth(LayoutUnit width) { m_desiredColumnWidth = width; }
    
    unsigned desiredColumnCount() const { return m_desiredColumnCount; }
    void setDesiredColumnCount(unsigned count) { m_desiredColumnCount = count; }

    enum Axis { InlineAxis, BlockAxis };

    Axis progressionAxis() const { return m_progressionAxis; }
    void setProgressionAxis(Axis progressionAxis) { m_progressionAxis = progressionAxis; }

    bool progressionIsReversed() const { return m_progressionIsReversed; }
    void setProgressionIsReversed(bool reversed) { m_progressionIsReversed = reversed; }

    unsigned columnCount() const { return m_columnCount; }
    LayoutUnit columnHeight() const { return m_columnHeight; }

    // Set our count and height.  This is enough info for a RenderBlock to compute page rects
    // dynamically.
    void setColumnCountAndHeight(int count, LayoutUnit height)
    { 
        m_columnCount = count;
        m_columnHeight = height;
    }
    void setColumnHeight(LayoutUnit height) { m_columnHeight = height; }

    void updateMinimumColumnHeight(LayoutUnit height) { m_minimumColumnHeight = std::max(height, m_minimumColumnHeight); }
    LayoutUnit minimumColumnHeight() const { return m_minimumColumnHeight; }

    int forcedBreaks() const { return m_forcedBreaks; }
    LayoutUnit forcedBreakOffset() const { return m_forcedBreakOffset; }
    LayoutUnit maximumDistanceBetweenForcedBreaks() const { return m_maximumDistanceBetweenForcedBreaks; }
    void clearForcedBreaks()
    { 
        m_forcedBreaks = 0;
        m_maximumDistanceBetweenForcedBreaks = 0;
        m_forcedBreakOffset = 0;
    }
    void addForcedBreak(LayoutUnit offsetFromFirstPage)
    { 
        ASSERT(!m_columnHeight);
        LayoutUnit distanceFromLastBreak = offsetFromFirstPage - m_forcedBreakOffset;
        if (!distanceFromLastBreak)
            return;
        m_forcedBreaks++;
        m_maximumDistanceBetweenForcedBreaks = std::max(m_maximumDistanceBetweenForcedBreaks, distanceFromLastBreak);
        m_forcedBreakOffset = offsetFromFirstPage;
    }

    enum PaginationUnit { Column, Page };
    PaginationUnit paginationUnit() const { return m_paginationUnit; }
    void setPaginationUnit(PaginationUnit paginationUnit) { m_paginationUnit = paginationUnit; }

private:
    LayoutUnit m_desiredColumnWidth;
    unsigned m_desiredColumnCount;
    Axis m_progressionAxis;
    bool m_progressionIsReversed;

    unsigned m_columnCount;
    LayoutUnit m_columnHeight;
    LayoutUnit m_minimumColumnHeight;
    int m_forcedBreaks; // FIXME: We will ultimately need to cache more information to balance around forced breaks properly.
    LayoutUnit m_maximumDistanceBetweenForcedBreaks;
    LayoutUnit m_forcedBreakOffset;
    PaginationUnit m_paginationUnit;
};

}

#endif
