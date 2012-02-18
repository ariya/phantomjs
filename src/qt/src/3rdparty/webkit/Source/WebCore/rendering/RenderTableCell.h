/*
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2009 Apple Inc. All rights reserved.
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

#ifndef RenderTableCell_h
#define RenderTableCell_h

#include "RenderTableSection.h"

namespace WebCore {

class RenderTableCell : public RenderBlock {
public:
    explicit RenderTableCell(Node*);

    // FIXME: need to implement cellIndex
    int cellIndex() const { return 0; }
    void setCellIndex(int) { }

    int colSpan() const { return m_columnSpan; }
    void setColSpan(int c) { m_columnSpan = c; }

    int rowSpan() const { return m_rowSpan; }
    void setRowSpan(int r) { m_rowSpan = r; }

    int col() const { return m_column; }
    void setCol(int col) { m_column = col; }
    int row() const { return m_row; }
    void setRow(int row) { m_row = row; }

    RenderTableSection* section() const { return toRenderTableSection(parent()->parent()); }
    RenderTable* table() const { return toRenderTable(parent()->parent()->parent()); }

    Length styleOrColLogicalWidth() const;

    virtual void computePreferredLogicalWidths();

    void updateLogicalWidth(int);

    virtual int borderLeft() const;
    virtual int borderRight() const;
    virtual int borderTop() const;
    virtual int borderBottom() const;
    virtual int borderStart() const;
    virtual int borderEnd() const;
    virtual int borderBefore() const;
    virtual int borderAfter() const;

    int borderHalfLeft(bool outer) const;
    int borderHalfRight(bool outer) const;
    int borderHalfTop(bool outer) const;
    int borderHalfBottom(bool outer) const;

    int borderHalfStart(bool outer) const;
    int borderHalfEnd(bool outer) const;
    int borderHalfBefore(bool outer) const;
    int borderHalfAfter(bool outer) const;

    CollapsedBorderValue collapsedStartBorder() const;
    CollapsedBorderValue collapsedEndBorder() const;
    CollapsedBorderValue collapsedBeforeBorder() const;
    CollapsedBorderValue collapsedAfterBorder() const;

    CollapsedBorderValue collapsedLeftBorder() const;
    CollapsedBorderValue collapsedRightBorder() const;
    CollapsedBorderValue collapsedTopBorder() const;
    CollapsedBorderValue collapsedBottomBorder() const;

    typedef Vector<CollapsedBorderValue, 100> CollapsedBorderStyles;
    void collectBorderStyles(CollapsedBorderStyles&) const;
    static void sortBorderStyles(CollapsedBorderStyles&);

    virtual void updateFromElement();

    virtual void layout();

    virtual void paint(PaintInfo&, int tx, int ty);

    void paintBackgroundsBehindCell(PaintInfo&, int tx, int ty, RenderObject* backgroundObject);

    int cellBaselinePosition() const;

    void setIntrinsicPaddingBefore(int p) { m_intrinsicPaddingBefore = p; }
    void setIntrinsicPaddingAfter(int p) { m_intrinsicPaddingAfter = p; }
    void setIntrinsicPadding(int before, int after) { setIntrinsicPaddingBefore(before); setIntrinsicPaddingAfter(after); }
    void clearIntrinsicPadding() { setIntrinsicPadding(0, 0); }

    int intrinsicPaddingBefore() const { return m_intrinsicPaddingBefore; }
    int intrinsicPaddingAfter() const { return m_intrinsicPaddingAfter; }

    virtual int paddingTop(bool includeIntrinsicPadding = true) const;
    virtual int paddingBottom(bool includeIntrinsicPadding = true) const;
    virtual int paddingLeft(bool includeIntrinsicPadding = true) const;
    virtual int paddingRight(bool includeIntrinsicPadding = true) const;
    
    // FIXME: For now we just assume the cell has the same block flow direction as the table.  It's likely we'll
    // create an extra anonymous RenderBlock to handle mixing directionality anyway, in which case we can lock
    // the block flow directionality of the cells to the table's directionality.
    virtual int paddingBefore(bool includeIntrinsicPadding = true) const;
    virtual int paddingAfter(bool includeIntrinsicPadding = true) const;

    virtual void setOverrideSize(int);
    void setOverrideSizeFromRowHeight(int);

    bool hasVisualOverflow() const { return m_overflow && !borderBoxRect().contains(m_overflow->visualOverflowRect()); }

    virtual void scrollbarsChanged(bool horizontalScrollbarChanged, bool verticalScrollbarChanged);

    bool cellWidthChanged() const { return m_cellWidthChanged; }
    void setCellWidthChanged(bool b = true) { m_cellWidthChanged = b; }

protected:
    virtual void styleWillChange(StyleDifference, const RenderStyle* newStyle);
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);

private:
    virtual const char* renderName() const { return isAnonymous() ? "RenderTableCell (anonymous)" : "RenderTableCell"; }

    virtual bool isTableCell() const { return true; }

    virtual void destroy();

    virtual void computeLogicalWidth();

    virtual void paintBoxDecorations(PaintInfo&, int tx, int ty);
    virtual void paintMask(PaintInfo&, int tx, int ty);

    virtual IntSize offsetFromContainer(RenderObject*, const IntPoint&) const;
    virtual IntRect clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer);
    virtual void computeRectForRepaint(RenderBoxModelObject* repaintContainer, IntRect&, bool fixed = false);

    void paintCollapsedBorder(GraphicsContext*, int x, int y, int w, int h);

    int m_row;
    int m_column;
    int m_rowSpan;
    int m_columnSpan : 31;
    bool m_cellWidthChanged : 1;
    int m_intrinsicPaddingBefore;
    int m_intrinsicPaddingAfter;
};

inline RenderTableCell* toRenderTableCell(RenderObject* object)
{
    ASSERT(!object || object->isTableCell());
    return static_cast<RenderTableCell*>(object);
}

inline const RenderTableCell* toRenderTableCell(const RenderObject* object)
{
    ASSERT(!object || object->isTableCell());
    return static_cast<const RenderTableCell*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderTableCell(const RenderTableCell*);

} // namespace WebCore

#endif // RenderTableCell_h
