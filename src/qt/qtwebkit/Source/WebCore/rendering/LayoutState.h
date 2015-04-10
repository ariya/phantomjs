/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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

#ifndef LayoutState_h
#define LayoutState_h

#include "ColumnInfo.h"
#include "LayoutRect.h"
#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>

namespace WebCore {

class RenderArena;
class RenderBlock;
class RenderBox;
class RenderObject;
class RenderFlowThread;
#if ENABLE(CSS_SHAPES)
class ShapeInsideInfo;
#endif

class LayoutState {
    WTF_MAKE_NONCOPYABLE(LayoutState);
public:
    LayoutState()
        : m_clipped(false)
        , m_isPaginated(false)
        , m_pageLogicalHeightChanged(false)
#if !ASSERT_DISABLED && ENABLE(SATURATED_LAYOUT_ARITHMETIC)
        , m_layoutDeltaXSaturated(false)
        , m_layoutDeltaYSaturated(false)
#endif
        , m_columnInfo(0)
        , m_lineGrid(0)
        , m_next(0)
#if ENABLE(CSS_SHAPES)
        , m_shapeInsideInfo(0)
#endif
        , m_pageLogicalHeight(0)
#ifndef NDEBUG
        , m_renderer(0)
#endif
    {
    }

    LayoutState(LayoutState*, RenderBox*, const LayoutSize& offset, LayoutUnit pageHeight, bool pageHeightChanged, ColumnInfo*);
    LayoutState(RenderObject*);

    void destroy(RenderArena*);

    // Overloaded new operator.
    void* operator new(size_t, RenderArena*);

    // Overridden to prevent the normal delete from being called.
    void operator delete(void*, size_t);

    void clearPaginationInformation();
    bool isPaginatingColumns() const { return m_columnInfo && m_columnInfo->paginationUnit() == ColumnInfo::Column; }
    bool isPaginated() const { return m_isPaginated; }
    
    // The page logical offset is the object's offset from the top of the page in the page progression
    // direction (so an x-offset in vertical text and a y-offset for horizontal text).
    LayoutUnit pageLogicalOffset(RenderBox*, LayoutUnit childLogicalOffset) const;

    void addForcedColumnBreak(RenderBox*, LayoutUnit childLogicalOffset);
    
    LayoutUnit pageLogicalHeight() const { return m_pageLogicalHeight; }
    bool pageLogicalHeightChanged() const { return m_pageLogicalHeightChanged; }

    RenderBlock* lineGrid() const { return m_lineGrid; }
    LayoutSize lineGridOffset() const { return m_lineGridOffset; }
    LayoutSize lineGridPaginationOrigin() const { return m_lineGridPaginationOrigin; }

    LayoutSize layoutOffset() const { return m_layoutOffset; }

    bool needsBlockDirectionLocationSetBeforeLayout() const { return m_lineGrid || (m_isPaginated && m_pageLogicalHeight); }

#if ENABLE(CSS_SHAPES)
    ShapeInsideInfo* shapeInsideInfo() const { return m_shapeInsideInfo; }
#endif
private:
    // The normal operator new is disallowed.
    void* operator new(size_t) throw();

    void propagateLineGridInfo(RenderBox*);
    void establishLineGrid(RenderBlock*);

    void computeLineGridPaginationOrigin(RenderBox*);

public:
    // Do not add anything apart from bitfields until after m_columnInfo. See https://bugs.webkit.org/show_bug.cgi?id=100173
    bool m_clipped:1;
    bool m_isPaginated:1;
    // If our page height has changed, this will force all blocks to relayout.
    bool m_pageLogicalHeightChanged:1;
#if !ASSERT_DISABLED && ENABLE(SATURATED_LAYOUT_ARITHMETIC)
    bool m_layoutDeltaXSaturated:1;
    bool m_layoutDeltaYSaturated:1;
#endif
    // If the enclosing pagination model is a column model, then this will store column information for easy retrieval/manipulation.
    ColumnInfo* m_columnInfo;
    // The current line grid that we're snapping to and the offset of the start of the grid.
    RenderBlock* m_lineGrid;
    LayoutState* m_next;
#if ENABLE(CSS_SHAPES)
    ShapeInsideInfo* m_shapeInsideInfo;
#endif

    // FIXME: Distinguish between the layout clip rect and the paint clip rect which may be larger,
    // e.g., because of composited scrolling.
    LayoutRect m_clipRect;
    
    // x/y offset from container. Includes relative positioning and scroll offsets.
    LayoutSize m_paintOffset;
    // x/y offset from container. Does not include relative positioning or scroll offsets.
    LayoutSize m_layoutOffset;
    // Transient offset from the final position of the object
    // used to ensure that repaints happen in the correct place.
    // This is a total delta accumulated from the root. 
    LayoutSize m_layoutDelta;

    // The current page height for the pagination model that encloses us.
    LayoutUnit m_pageLogicalHeight;
    // The offset of the start of the first page in the nearest enclosing pagination model.
    LayoutSize m_pageOffset;
    LayoutSize m_lineGridOffset;
    LayoutSize m_lineGridPaginationOrigin;

#ifndef NDEBUG
    RenderObject* m_renderer;
#endif
};

} // namespace WebCore

#endif // LayoutState_h
