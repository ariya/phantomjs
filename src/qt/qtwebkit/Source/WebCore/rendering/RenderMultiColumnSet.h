/*
 * Copyright (C) 2012 Apple Inc.  All rights reserved.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef RenderMultiColumnSet_h
#define RenderMultiColumnSet_h

#include "RenderRegionSet.h"

namespace WebCore {

// RenderMultiColumnSet represents a set of columns that all have the same width and height. By combining runs of same-size columns into a single
// object, we significantly reduce the number of unique RenderObjects required to represent columns.
//
// A simple multi-column block will have exactly one RenderMultiColumnSet child. A simple paginated multi-column block will have three
// RenderMultiColumnSet children: one for the content at the bottom of the first page (whose columns will have a shorter height), one
// for the 2nd to n-1 pages, and then one last column set that will hold the shorter columns on the final page (that may have to be balanced
// as well).
//
// Column spans result in the creation of new column sets as well, since a spanning region has to be placed in between the column sets that
// come before and after the span.
class RenderMultiColumnSet : public RenderRegionSet {
public:
    static RenderMultiColumnSet* createAnonymous(RenderFlowThread*);

    virtual bool isRenderMultiColumnSet() const OVERRIDE { return true; }

    unsigned computedColumnCount() const { return m_computedColumnCount; }
    LayoutUnit computedColumnWidth() const { return m_computedColumnWidth; }
    LayoutUnit computedColumnHeight() const { return m_computedColumnHeight; }

    void setComputedColumnWidthAndCount(LayoutUnit width, unsigned count)
    {
        m_computedColumnWidth = width;
        m_computedColumnCount = count;
    }

    LayoutUnit heightAdjustedForSetOffset(LayoutUnit height) const;

    void updateMinimumColumnHeight(LayoutUnit height) { m_minimumColumnHeight = std::max(height, m_minimumColumnHeight); }
    LayoutUnit minimumColumnHeight() const { return m_minimumColumnHeight; }

    unsigned forcedBreaksCount() const { return m_forcedBreaksCount; }
    LayoutUnit forcedBreakOffset() const { return m_forcedBreakOffset; }
    LayoutUnit maximumDistanceBetweenForcedBreaks() const { return m_maximumDistanceBetweenForcedBreaks; }
    void clearForcedBreaks()
    { 
        m_forcedBreaksCount = 0;
        m_maximumDistanceBetweenForcedBreaks = 0;
        m_forcedBreakOffset = 0;
    }
    void addForcedBreak(LayoutUnit offsetFromFirstPage)
    { 
        ASSERT(!computedColumnHeight());
        LayoutUnit distanceFromLastBreak = offsetFromFirstPage - m_forcedBreakOffset;
        if (!distanceFromLastBreak)
            return;
        m_forcedBreaksCount++;
        m_maximumDistanceBetweenForcedBreaks = std::max(m_maximumDistanceBetweenForcedBreaks, distanceFromLastBreak);
        m_forcedBreakOffset = offsetFromFirstPage;
    }

    // Calculate the column height when contents are supposed to be balanced. If 'initial' is set,
    // guess an initial column height; otherwise, stretch the column height a tad. Return true if
    // column height changed and another layout pass is required.
    bool calculateBalancedHeight(bool initial);

    // Record space shortage (the amount of space that would have been enough to prevent some
    // element from being moved to the next column) at a column break. The smallest amount of space
    // shortage we find is the amount with which we will stretch the column height, if it turns out
    // after layout that the columns weren't tall enough.
    void recordSpaceShortage(LayoutUnit spaceShortage);

    virtual void updateLogicalWidth() OVERRIDE;

    void prepareForLayout();

private:
    RenderMultiColumnSet(RenderFlowThread*);

    virtual void computeLogicalHeight(LayoutUnit logicalHeight, LayoutUnit logicalTop, LogicalExtentComputedValues&) const OVERRIDE;

    virtual void paintObject(PaintInfo&, const LayoutPoint& paintOffset) OVERRIDE;

    virtual LayoutUnit pageLogicalWidth() const OVERRIDE { return m_computedColumnWidth; }
    virtual LayoutUnit pageLogicalHeight() const OVERRIDE { return m_computedColumnHeight; }

    virtual LayoutUnit pageLogicalTopForOffset(LayoutUnit offset) const OVERRIDE;
    
    // FIXME: This will change once we have column sets constrained by enclosing pages, etc.
    virtual LayoutUnit logicalHeightOfAllFlowThreadContent() const OVERRIDE { return m_computedColumnHeight; }

    // FIXME: For now we return false, but it's likely we will leverage the auto height region code to do column
    // balancing. That's why we have an override of this function that is distinct from RenderRegionSet's override.
    virtual bool shouldHaveAutoLogicalHeight() const OVERRIDE { return false; }
    
    virtual void repaintFlowThreadContent(const LayoutRect& repaintRect, bool immediate) const OVERRIDE;

    virtual void collectLayerFragments(LayerFragments&, const LayoutRect& layerBoundingBox, const LayoutRect& dirtyRect) OVERRIDE;

    virtual const char* renderName() const;
    
    void paintColumnRules(PaintInfo&, const LayoutPoint& paintOffset);

    LayoutUnit columnGap() const;
    LayoutRect columnRectAt(unsigned index) const;
    unsigned columnCount() const;

    LayoutRect flowThreadPortionRectAt(unsigned index) const;
    LayoutRect flowThreadPortionOverflowRect(const LayoutRect& flowThreadPortion, unsigned index, unsigned colCount, LayoutUnit colGap) const;

    enum ColumnIndexCalculationMode {
        ClampToExistingColumns, // Stay within the range of already existing columns.
        AssumeNewColumns // Allow column indices outside the range of already existing columns.
    };
    unsigned columnIndexAtOffset(LayoutUnit, ColumnIndexCalculationMode = ClampToExistingColumns) const;

    void setAndConstrainColumnHeight(LayoutUnit);

    unsigned m_computedColumnCount;
    LayoutUnit m_computedColumnWidth;
    LayoutUnit m_computedColumnHeight;
    
    // The following variables are used when balancing the column set.
    LayoutUnit m_maxColumnHeight; // Maximum column height allowed.
    LayoutUnit m_minSpaceShortage; // The smallest amout of space shortage that caused a column break.
    LayoutUnit m_minimumColumnHeight;
    unsigned m_forcedBreaksCount; // FIXME: We will ultimately need to cache more information to balance around forced breaks properly.
    LayoutUnit m_maximumDistanceBetweenForcedBreaks;
    LayoutUnit m_forcedBreakOffset;
};

inline RenderMultiColumnSet* toRenderMultiColumnSet(RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isRenderMultiColumnSet());
    return static_cast<RenderMultiColumnSet*>(object);
}

inline const RenderMultiColumnSet* toRenderMultiColumnSet(const RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isRenderMultiColumnSet());
    return static_cast<const RenderMultiColumnSet*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderMultiColumnSet(const RenderMultiColumnSet*);

} // namespace WebCore

#endif // RenderMultiColumnSet_h

