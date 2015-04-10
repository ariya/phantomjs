/*
 * Copyright (C) 2003, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef RootInlineBox_h
#define RootInlineBox_h

#include "BidiContext.h"
#include "InlineFlowBox.h"

namespace WebCore {

class EllipsisBox;
class HitTestResult;
class LogicalSelectionOffsetCaches;
class RenderRegion;

struct BidiStatus;
struct GapRects;

class RootInlineBox : public InlineFlowBox {
public:
    explicit RootInlineBox(RenderBlock*);

    virtual void destroy(RenderArena*) FINAL;

    virtual bool isRootInlineBox() const FINAL { return true; }

    void detachEllipsisBox(RenderArena*);

    RootInlineBox* nextRootBox() const { return static_cast<RootInlineBox*>(m_nextLineBox); }
    RootInlineBox* prevRootBox() const { return static_cast<RootInlineBox*>(m_prevLineBox); }

    virtual void adjustPosition(float dx, float dy) FINAL;

    LayoutUnit lineTop() const { return m_lineTop; }
    LayoutUnit lineBottom() const { return m_lineBottom; }

    LayoutUnit lineTopWithLeading() const { return m_lineTopWithLeading; }
    LayoutUnit lineBottomWithLeading() const { return m_lineBottomWithLeading; }
    
    LayoutUnit paginationStrut() const { return m_fragmentationData ? m_fragmentationData->m_paginationStrut : LayoutUnit(0); }
    void setPaginationStrut(LayoutUnit strut) { ensureLineFragmentationData()->m_paginationStrut = strut; }

    bool isFirstAfterPageBreak() const { return m_fragmentationData ? m_fragmentationData->m_isFirstAfterPageBreak : false; }
    void setIsFirstAfterPageBreak(bool isFirstAfterPageBreak) { ensureLineFragmentationData()->m_isFirstAfterPageBreak = isFirstAfterPageBreak; }

    LayoutUnit paginatedLineWidth() const { return m_fragmentationData ? m_fragmentationData->m_paginatedLineWidth : LayoutUnit(0); }
    void setPaginatedLineWidth(LayoutUnit width) { ensureLineFragmentationData()->m_paginatedLineWidth = width; }

    RenderRegion* containingRegion() const;
    void setContainingRegion(RenderRegion*);

    LayoutUnit selectionTop() const;
    LayoutUnit selectionBottom() const;
    LayoutUnit selectionHeight() const { return max<LayoutUnit>(0, selectionBottom() - selectionTop()); }

    LayoutUnit selectionTopAdjustedForPrecedingBlock() const;
    LayoutUnit selectionHeightAdjustedForPrecedingBlock() const { return max<LayoutUnit>(0, selectionBottom() - selectionTopAdjustedForPrecedingBlock()); }

    int blockDirectionPointInLine() const;

    LayoutUnit alignBoxesInBlockDirection(LayoutUnit heightOfBlock, GlyphOverflowAndFallbackFontsMap&, VerticalPositionCache&);
    void setLineTopBottomPositions(LayoutUnit top, LayoutUnit bottom, LayoutUnit topWithLeading, LayoutUnit bottomWithLeading)
    { 
        m_lineTop = top; 
        m_lineBottom = bottom;
        m_lineTopWithLeading = topWithLeading;
        m_lineBottomWithLeading = bottomWithLeading;
    }

    virtual RenderLineBoxList* rendererLineBoxes() const FINAL;

    RenderObject* lineBreakObj() const { return m_lineBreakObj; }
    BidiStatus lineBreakBidiStatus() const;
    void setLineBreakInfo(RenderObject*, unsigned breakPos, const BidiStatus&);

    unsigned lineBreakPos() const { return m_lineBreakPos; }
    void setLineBreakPos(unsigned p) { m_lineBreakPos = p; }

    using InlineBox::endsWithBreak;
    using InlineBox::setEndsWithBreak;

    void childRemoved(InlineBox* box);

    bool lineCanAccommodateEllipsis(bool ltr, int blockEdge, int lineBoxEdge, int ellipsisWidth);
    // Return the truncatedWidth, the width of the truncated text + ellipsis.
    float placeEllipsis(const AtomicString& ellipsisStr, bool ltr, float blockLeftEdge, float blockRightEdge, float ellipsisWidth, InlineBox* markupBox = 0);
    // Return the position of the EllipsisBox or -1.
    virtual float placeEllipsisBox(bool ltr, float blockLeftEdge, float blockRightEdge, float ellipsisWidth, float &truncatedWidth, bool& foundBox) OVERRIDE FINAL;

    using InlineBox::hasEllipsisBox;
    EllipsisBox* ellipsisBox() const;

    void paintEllipsisBox(PaintInfo&, const LayoutPoint&, LayoutUnit lineTop, LayoutUnit lineBottom) const;

    virtual void clearTruncation() OVERRIDE FINAL;

    bool isHyphenated() const;

    virtual int baselinePosition(FontBaseline baselineType) const FINAL;
    virtual LayoutUnit lineHeight() const FINAL;

#if PLATFORM(MAC)
    void addHighlightOverflow();
    void paintCustomHighlight(PaintInfo&, const LayoutPoint&, const AtomicString& highlightType);
#endif

    virtual void paint(PaintInfo&, const LayoutPoint&, LayoutUnit lineTop, LayoutUnit lineBottom);
    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, LayoutUnit lineTop, LayoutUnit lineBottom) OVERRIDE FINAL;

    using InlineBox::hasSelectedChildren;
    using InlineBox::setHasSelectedChildren;

    virtual RenderObject::SelectionState selectionState() FINAL;
    InlineBox* firstSelectedBox();
    InlineBox* lastSelectedBox();

    GapRects lineSelectionGap(RenderBlock* rootBlock, const LayoutPoint& rootBlockPhysicalPosition, const LayoutSize& offsetFromRootBlock,
        LayoutUnit selTop, LayoutUnit selHeight, const LogicalSelectionOffsetCaches&, const PaintInfo*);

    RenderBlock* block() const;

    InlineBox* closestLeafChildForPoint(const IntPoint&, bool onlyEditableLeaves);
    InlineBox* closestLeafChildForLogicalLeftPosition(int, bool onlyEditableLeaves = false);

    void appendFloat(RenderBox* floatingBox)
    {
        ASSERT(!isDirty());
        if (m_floats)
            m_floats->append(floatingBox);
        else
            m_floats= adoptPtr(new Vector<RenderBox*>(1, floatingBox));
    }

    Vector<RenderBox*>* floatsPtr() { ASSERT(!isDirty()); return m_floats.get(); }

    virtual void extractLineBoxFromRenderObject() FINAL;
    virtual void attachLineBoxToRenderObject() FINAL;
    virtual void removeLineBoxFromRenderObject() FINAL;
    
    FontBaseline baselineType() const { return static_cast<FontBaseline>(m_baselineType); }

    bool hasAnnotationsBefore() const { return m_hasAnnotationsBefore; }
    bool hasAnnotationsAfter() const { return m_hasAnnotationsAfter; }

    LayoutRect paddedLayoutOverflowRect(LayoutUnit endPadding) const;

    void ascentAndDescentForBox(InlineBox*, GlyphOverflowAndFallbackFontsMap&, int& ascent, int& descent, bool& affectsAscent, bool& affectsDescent) const;
    LayoutUnit verticalPositionForBox(InlineBox*, VerticalPositionCache&);
    bool includeLeadingForBox(InlineBox*) const;
    bool includeFontForBox(InlineBox*) const;
    bool includeGlyphsForBox(InlineBox*) const;
    bool includeMarginForBox(InlineBox*) const;
    bool fitsToGlyphs() const;
    bool includesRootLineBoxFontOrLeading() const;
    
    LayoutUnit logicalTopVisualOverflow() const
    {
        return InlineFlowBox::logicalTopVisualOverflow(lineTop());
    }
    LayoutUnit logicalBottomVisualOverflow() const
    {
        return InlineFlowBox::logicalBottomVisualOverflow(lineBottom());
    }
    LayoutUnit logicalTopLayoutOverflow() const
    {
        return InlineFlowBox::logicalTopLayoutOverflow(lineTop());
    }
    LayoutUnit logicalBottomLayoutOverflow() const
    {
        return InlineFlowBox::logicalBottomLayoutOverflow(lineBottom());
    }

#if ENABLE(CSS3_TEXT)
    // Used to calculate the underline offset for TextUnderlinePositionUnder.
    float maxLogicalTop() const;
#endif // CSS3_TEXT

    Node* getLogicalStartBoxWithNode(InlineBox*&) const;
    Node* getLogicalEndBoxWithNode(InlineBox*&) const;

#ifndef NDEBUG
    virtual const char* boxName() const;
#endif
private:
    LayoutUnit lineSnapAdjustment(LayoutUnit delta = 0) const;

    LayoutUnit beforeAnnotationsAdjustment() const;

    struct LineFragmentationData;
    LineFragmentationData* ensureLineFragmentationData()
    {
        if (!m_fragmentationData)
            m_fragmentationData = adoptPtr(new LineFragmentationData());

        return m_fragmentationData.get();
    }

    // This folds into the padding at the end of InlineFlowBox on 64-bit.
    unsigned m_lineBreakPos;

    // Where this line ended.  The exact object and the position within that object are stored so that
    // we can create an InlineIterator beginning just after the end of this line.
    RenderObject* m_lineBreakObj;
    RefPtr<BidiContext> m_lineBreakContext;

    LayoutUnit m_lineTop;
    LayoutUnit m_lineBottom;

    LayoutUnit m_lineTopWithLeading;
    LayoutUnit m_lineBottomWithLeading;

    struct LineFragmentationData {
        WTF_MAKE_NONCOPYABLE(LineFragmentationData); WTF_MAKE_FAST_ALLOCATED;
    public:
        LineFragmentationData()
            : m_containingRegion(0)
            , m_paginationStrut(0)
            , m_paginatedLineWidth(0)
            , m_isFirstAfterPageBreak(false)
        {

        }

        // It should not be assumed the |containingRegion| is always valid.
        // It can also be 0 if the flow has no region chain.
        RenderRegion* m_containingRegion;
        LayoutUnit m_paginationStrut;
        LayoutUnit m_paginatedLineWidth;
        bool m_isFirstAfterPageBreak;
    };

    OwnPtr<LineFragmentationData> m_fragmentationData;

    // Floats hanging off the line are pushed into this vector during layout. It is only
    // good for as long as the line has not been marked dirty.
    OwnPtr<Vector<RenderBox*> > m_floats;
};

} // namespace WebCore

#endif // RootInlineBox_h
