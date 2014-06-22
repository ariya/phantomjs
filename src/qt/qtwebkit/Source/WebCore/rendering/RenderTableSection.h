/*
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2009 Apple Inc. All rights reserved.
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
 */

#ifndef RenderTableSection_h
#define RenderTableSection_h

#include "RenderTable.h"
#include <wtf/Vector.h>

namespace WebCore {

enum CollapsedBorderSide {
    CBSBefore,
    CBSAfter,
    CBSStart,
    CBSEnd
};

// Helper class for paintObject.
class CellSpan {
public:
    CellSpan(unsigned start, unsigned end)
        : m_start(start)
        , m_end(end)
    {
    }

    unsigned start() const { return m_start; }
    unsigned end() const { return m_end; }

    unsigned& start() { return m_start; }
    unsigned& end() { return m_end; }

private:
    unsigned m_start;
    unsigned m_end;
};

class RenderTableCell;
class RenderTableRow;

class RenderTableSection : public RenderBox {
public:
    RenderTableSection(Element*);
    virtual ~RenderTableSection();

    RenderObject* firstChild() const { ASSERT(children() == virtualChildren()); return children()->firstChild(); }
    RenderObject* lastChild() const { ASSERT(children() == virtualChildren()); return children()->lastChild(); }

    const RenderObjectChildList* children() const { return &m_children; }
    RenderObjectChildList* children() { return &m_children; }

    virtual void addChild(RenderObject* child, RenderObject* beforeChild = 0);

    virtual int firstLineBoxBaseline() const OVERRIDE;

    void addCell(RenderTableCell*, RenderTableRow* row);

    int calcRowLogicalHeight();
    void layoutRows(int headHeight, int footHeight);
    void computeOverflowFromCells();

    RenderTable* table() const { return toRenderTable(parent()); }

    struct CellStruct {
        Vector<RenderTableCell*, 1> cells; 
        bool inColSpan; // true for columns after the first in a colspan

        CellStruct()
            : inColSpan(false)
        {
        }

        RenderTableCell* primaryCell()
        {
            return hasCells() ? cells[cells.size() - 1] : 0;
        }

        const RenderTableCell* primaryCell() const
        {
            return hasCells() ? cells[cells.size() - 1] : 0;
        }

        bool hasCells() const { return cells.size() > 0; }
    };

    typedef Vector<CellStruct> Row;

    struct RowStruct {
        RowStruct()
            : rowRenderer(0)
            , baseline()
        {
        }

        Row row;
        RenderTableRow* rowRenderer;
        LayoutUnit baseline;
        Length logicalHeight;
    };

    const BorderValue& borderAdjoiningTableStart() const
    {
        if (hasSameDirectionAs(table()))
            return style()->borderStart();

        return style()->borderEnd();
    }

    const BorderValue& borderAdjoiningTableEnd() const
    {
        if (hasSameDirectionAs(table()))
            return style()->borderEnd();

        return style()->borderStart();
    }

    const BorderValue& borderAdjoiningStartCell(const RenderTableCell*) const;
    const BorderValue& borderAdjoiningEndCell(const RenderTableCell*) const;

    const RenderTableCell* firstRowCellAdjoiningTableStart() const;
    const RenderTableCell* firstRowCellAdjoiningTableEnd() const;

    CellStruct& cellAt(unsigned row,  unsigned col) { return m_grid[row].row[col]; }
    const CellStruct& cellAt(unsigned row, unsigned col) const { return m_grid[row].row[col]; }
    RenderTableCell* primaryCellAt(unsigned row, unsigned col)
    {
        CellStruct& c = m_grid[row].row[col];
        return c.primaryCell();
    }

    RenderTableRow* rowRendererAt(unsigned row) const { return m_grid[row].rowRenderer; }

    void appendColumn(unsigned pos);
    void splitColumn(unsigned pos, unsigned first);

    int calcOuterBorderBefore() const;
    int calcOuterBorderAfter() const;
    int calcOuterBorderStart() const;
    int calcOuterBorderEnd() const;
    void recalcOuterBorder();

    int outerBorderBefore() const { return m_outerBorderBefore; }
    int outerBorderAfter() const { return m_outerBorderAfter; }
    int outerBorderStart() const { return m_outerBorderStart; }
    int outerBorderEnd() const { return m_outerBorderEnd; }

    unsigned numRows() const { return m_grid.size(); }
    unsigned numColumns() const;
    void recalcCells();
    void recalcCellsIfNeeded()
    {
        if (m_needsCellRecalc)
            recalcCells();
    }

    bool needsCellRecalc() const { return m_needsCellRecalc; }
    void setNeedsCellRecalc();

    LayoutUnit rowBaseline(unsigned row) { return m_grid[row].baseline; }

    void rowLogicalHeightChanged(unsigned rowIndex);

    void removeCachedCollapsedBorders(const RenderTableCell*);
    void setCachedCollapsedBorder(const RenderTableCell*, CollapsedBorderSide, CollapsedBorderValue);
    CollapsedBorderValue& cachedCollapsedBorder(const RenderTableCell*, CollapsedBorderSide);

    // distributeExtraLogicalHeightToRows methods return the *consumed* extra logical height.
    // FIXME: We may want to introduce a structure holding the in-flux layout information.
    int distributeExtraLogicalHeightToRows(int extraLogicalHeight);

    static RenderTableSection* createAnonymousWithParentRenderer(const RenderObject*);
    virtual RenderBox* createAnonymousBoxWithSameTypeAs(const RenderObject* parent) const OVERRIDE
    {
        return createAnonymousWithParentRenderer(parent);
    }
    
    virtual void paint(PaintInfo&, const LayoutPoint&) OVERRIDE;

protected:
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);

private:
    virtual RenderObjectChildList* virtualChildren() { return children(); }
    virtual const RenderObjectChildList* virtualChildren() const { return children(); }

    virtual const char* renderName() const { return (isAnonymous() || isPseudoElement()) ? "RenderTableSection (anonymous)" : "RenderTableSection"; }

    virtual bool isTableSection() const { return true; }

    virtual void willBeRemovedFromTree() OVERRIDE;

    virtual void layout();

    virtual void paintCell(RenderTableCell*, PaintInfo&, const LayoutPoint&);
    virtual void paintObject(PaintInfo&, const LayoutPoint&);

    virtual void imageChanged(WrappedImagePtr, const IntRect* = 0);

    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction) OVERRIDE;

    void ensureRows(unsigned);

    void distributeExtraLogicalHeightToPercentRows(int& extraLogicalHeight, int totalPercent);
    void distributeExtraLogicalHeightToAutoRows(int& extraLogicalHeight, unsigned autoRowsCount);
    void distributeRemainingExtraLogicalHeight(int& extraLogicalHeight);

    bool hasOverflowingCell() const { return m_overflowingCells.size() || m_forceSlowPaintPathWithOverflowingCell; }
    void computeOverflowFromCells(unsigned totalRows, unsigned nEffCols);

    CellSpan fullTableRowSpan() const { return CellSpan(0, m_grid.size()); }
    CellSpan fullTableColumnSpan() const { return CellSpan(0, table()->columns().size()); }

    // Flip the rect so it aligns with the coordinates used by the rowPos and columnPos vectors.
    LayoutRect logicalRectForWritingModeAndDirection(const LayoutRect&) const;

    CellSpan dirtiedRows(const LayoutRect& repaintRect) const;
    CellSpan dirtiedColumns(const LayoutRect& repaintRect) const;

    // These two functions take a rectangle as input that has been flipped by logicalRectForWritingModeAndDirection.
    // The returned span of rows or columns is end-exclusive, and empty if start==end.
    CellSpan spannedRows(const LayoutRect& flippedRect) const;
    CellSpan spannedColumns(const LayoutRect& flippedRect) const;

    void setLogicalPositionForCell(RenderTableCell*, unsigned effectiveColumn) const;

    RenderObjectChildList m_children;

    Vector<RowStruct> m_grid;
    Vector<int> m_rowPos;

    // the current insertion position
    unsigned m_cCol;
    unsigned m_cRow;

    int m_outerBorderStart;
    int m_outerBorderEnd;
    int m_outerBorderBefore;
    int m_outerBorderAfter;

    bool m_needsCellRecalc;

    // This HashSet holds the overflowing cells for faster painting.
    // If we have more than gMaxAllowedOverflowingCellRatio * total cells, it will be empty
    // and m_forceSlowPaintPathWithOverflowingCell will be set to save memory.
    HashSet<RenderTableCell*> m_overflowingCells;
    bool m_forceSlowPaintPathWithOverflowingCell;

    bool m_hasMultipleCellLevels;

    // This map holds the collapsed border values for cells with collapsed borders.
    // It is held at RenderTableSection level to spare memory consumption by table cells.
    HashMap<pair<const RenderTableCell*, int>, CollapsedBorderValue > m_cellsCollapsedBorders;
};

inline RenderTableSection* toRenderTableSection(RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isTableSection());
    return static_cast<RenderTableSection*>(object);
}

inline const RenderTableSection* toRenderTableSection(const RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isTableSection());
    return static_cast<const RenderTableSection*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderTableSection(const RenderTableSection*);

} // namespace WebCore

#endif // RenderTableSection_h
