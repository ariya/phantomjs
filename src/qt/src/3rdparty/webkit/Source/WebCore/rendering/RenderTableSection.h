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

class RenderTableCell;
class RenderTableRow;

class RenderTableSection : public RenderBox {
public:
    RenderTableSection(Node*);
    virtual ~RenderTableSection();

    const RenderObjectChildList* children() const { return &m_children; }
    RenderObjectChildList* children() { return &m_children; }

    virtual void addChild(RenderObject* child, RenderObject* beforeChild = 0);

    virtual int firstLineBoxBaseline() const;

    void addCell(RenderTableCell*, RenderTableRow* row);

    void setCellLogicalWidths();
    int calcRowLogicalHeight();
    int layoutRows(int logicalHeight);

    RenderTable* table() const { return toRenderTable(parent()); }

    struct CellStruct {
        Vector<RenderTableCell*, 1> cells; 
        bool inColSpan; // true for columns after the first in a colspan

        CellStruct():
          inColSpan(false) {}
        
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
        Row* row;
        RenderTableRow* rowRenderer;
        int baseline;
        Length logicalHeight;
    };

    CellStruct& cellAt(int row,  int col) { return (*m_grid[row].row)[col]; }
    const CellStruct& cellAt(int row, int col) const { return (*m_grid[row].row)[col]; }
    RenderTableCell* primaryCellAt(int row, int col)
    {
        CellStruct& c = (*m_grid[row].row)[col];
        return c.primaryCell();
    }

    void appendColumn(int pos);
    void splitColumn(int pos, int first);

    int calcOuterBorderBefore() const;
    int calcOuterBorderAfter() const;
    int calcOuterBorderStart() const;
    int calcOuterBorderEnd() const;
    void recalcOuterBorder();

    int outerBorderBefore() const { return m_outerBorderBefore; }
    int outerBorderAfter() const { return m_outerBorderAfter; }
    int outerBorderStart() const { return m_outerBorderStart; }
    int outerBorderEnd() const { return m_outerBorderEnd; }

    int numRows() const { return m_gridRows; }
    int numColumns() const;
    void recalcCells();
    void recalcCellsIfNeeded()
    {
        if (m_needsCellRecalc)
            recalcCells();
    }

    bool needsCellRecalc() const { return m_needsCellRecalc; }
    void setNeedsCellRecalc();

    int getBaseline(int row) { return m_grid[row].baseline; }

protected:
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);

private:
    virtual RenderObjectChildList* virtualChildren() { return children(); }
    virtual const RenderObjectChildList* virtualChildren() const { return children(); }

    virtual const char* renderName() const { return isAnonymous() ? "RenderTableSection (anonymous)" : "RenderTableSection"; }

    virtual bool isTableSection() const { return true; }

    virtual void destroy();

    virtual void layout();

    virtual void removeChild(RenderObject* oldChild);

    virtual void paint(PaintInfo&, int tx, int ty);
    virtual void paintCell(RenderTableCell*, PaintInfo&, int tx, int ty);
    virtual void paintObject(PaintInfo&, int tx, int ty);

    virtual void imageChanged(WrappedImagePtr, const IntRect* = 0);

    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int x, int y, int tx, int ty, HitTestAction);

    bool ensureRows(int);
    void clearGrid();

    RenderObjectChildList m_children;

    Vector<RowStruct> m_grid;
    Vector<int> m_rowPos;

    int m_gridRows;

    // the current insertion position
    int m_cCol;
    int m_cRow;

    int m_outerBorderStart;
    int m_outerBorderEnd;
    int m_outerBorderBefore;
    int m_outerBorderAfter;

    bool m_needsCellRecalc;
    bool m_hasOverflowingCell;

    bool m_hasMultipleCellLevels;
};

inline RenderTableSection* toRenderTableSection(RenderObject* object)
{
    ASSERT(!object || object->isTableSection());
    return static_cast<RenderTableSection*>(object);
}

inline const RenderTableSection* toRenderTableSection(const RenderObject* object)
{
    ASSERT(!object || object->isTableSection());
    return static_cast<const RenderTableSection*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderTableSection(const RenderTableSection*);

} // namespace WebCore

#endif // RenderTableSection_h
