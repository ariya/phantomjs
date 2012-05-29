/*
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef RenderTable_h
#define RenderTable_h

#include "CSSPropertyNames.h"
#include "RenderBlock.h"
#include <wtf/Vector.h>

namespace WebCore {

class CollapsedBorderValue;
class RenderTableCol;
class RenderTableCell;
class RenderTableSection;
class TableLayout;

class RenderTable : public RenderBlock {
public:
    explicit RenderTable(Node*);
    virtual ~RenderTable();

    int getColumnPos(int col) const { return m_columnPos[col]; }

    int hBorderSpacing() const { return m_hSpacing; }
    int vBorderSpacing() const { return m_vSpacing; }
    
    bool collapseBorders() const { return style()->borderCollapse(); }

    int borderStart() const { return m_borderStart; }
    int borderEnd() const { return m_borderEnd; }
    int borderBefore() const;
    int borderAfter() const;

    int borderLeft() const
    {
        if (style()->isHorizontalWritingMode())
            return style()->isLeftToRightDirection() ? borderStart() : borderEnd();
        return style()->isFlippedBlocksWritingMode() ? borderAfter() : borderBefore();
    }

    int borderRight() const
    {
        if (style()->isHorizontalWritingMode())
            return style()->isLeftToRightDirection() ? borderEnd() : borderStart();
        return style()->isFlippedBlocksWritingMode() ? borderBefore() : borderAfter();
    }

    int borderTop() const
    {
        if (style()->isHorizontalWritingMode())
            return style()->isFlippedBlocksWritingMode() ? borderAfter() : borderBefore();
        return style()->isLeftToRightDirection() ? borderStart() : borderEnd();
    }

    int borderBottom() const
    {
        if (style()->isHorizontalWritingMode())
            return style()->isFlippedBlocksWritingMode() ? borderBefore() : borderAfter();
        return style()->isLeftToRightDirection() ? borderEnd() : borderStart();
    }

    const Color bgColor() const { return style()->visitedDependentColor(CSSPropertyBackgroundColor); }

    int outerBorderBefore() const;
    int outerBorderAfter() const;
    int outerBorderStart() const;
    int outerBorderEnd() const;

    int outerBorderLeft() const
    {
        if (style()->isHorizontalWritingMode())
            return style()->isLeftToRightDirection() ? outerBorderStart() : outerBorderEnd();
        return style()->isFlippedBlocksWritingMode() ? outerBorderAfter() : outerBorderBefore();
    }

    int outerBorderRight() const
    {
        if (style()->isHorizontalWritingMode())
            return style()->isLeftToRightDirection() ? outerBorderEnd() : outerBorderStart();
        return style()->isFlippedBlocksWritingMode() ? outerBorderBefore() : outerBorderAfter();
    }

    int outerBorderTop() const
    {
        if (style()->isHorizontalWritingMode())
            return style()->isFlippedBlocksWritingMode() ? outerBorderAfter() : outerBorderBefore();
        return style()->isLeftToRightDirection() ? outerBorderStart() : outerBorderEnd();
    }

    int outerBorderBottom() const
    {
        if (style()->isHorizontalWritingMode())
            return style()->isFlippedBlocksWritingMode() ? outerBorderBefore() : outerBorderAfter();
        return style()->isLeftToRightDirection() ? outerBorderEnd() : outerBorderStart();
    }

    int calcBorderStart() const;
    int calcBorderEnd() const;
    void recalcBordersInRowDirection();

    virtual void addChild(RenderObject* child, RenderObject* beforeChild = 0);

    struct ColumnStruct {
        enum {
            WidthUndefined = 0xffff
        };

        ColumnStruct()
            : span(1)
            , width(WidthUndefined)
        {
        }

        unsigned span;
        unsigned width; // the calculated position of the column
    };

    Vector<ColumnStruct>& columns() { return m_columns; }
    Vector<int>& columnPositions() { return m_columnPos; }
    RenderTableSection* header() const { return m_head; }
    RenderTableSection* footer() const { return m_foot; }
    RenderTableSection* firstBody() const { return m_firstBody; }

    void splitColumn(int pos, int firstSpan);
    void appendColumn(int span);
    int numEffCols() const { return m_columns.size(); }
    int spanOfEffCol(int effCol) const { return m_columns[effCol].span; }
    
    int colToEffCol(int col) const
    {
        int i = 0;
        int effCol = numEffCols();
        for (int c = 0; c < col && i < effCol; ++i)
            c += m_columns[i].span;
        return i;
    }
    
    int effColToCol(int effCol) const
    {
        int c = 0;
        for (int i = 0; i < effCol; i++)
            c += m_columns[i].span;
        return c;
    }

    int bordersPaddingAndSpacingInRowDirection() const
    {
        return borderStart() + borderEnd() +
               (collapseBorders() ? 0 : (paddingStart() + paddingEnd() + (numEffCols() + 1) * hBorderSpacing()));
    }

    RenderTableCol* colElement(int col, bool* startEdge = 0, bool* endEdge = 0) const;
    RenderTableCol* nextColElement(RenderTableCol* current) const;

    bool needsSectionRecalc() const { return m_needsSectionRecalc; }
    void setNeedsSectionRecalc()
    {
        if (documentBeingDestroyed())
            return;
        m_needsSectionRecalc = true;
        setNeedsLayout(true);
    }

    RenderTableSection* sectionAbove(const RenderTableSection*, bool skipEmptySections = false) const;
    RenderTableSection* sectionBelow(const RenderTableSection*, bool skipEmptySections = false) const;

    RenderTableCell* cellAbove(const RenderTableCell*) const;
    RenderTableCell* cellBelow(const RenderTableCell*) const;
    RenderTableCell* cellBefore(const RenderTableCell*) const;
    RenderTableCell* cellAfter(const RenderTableCell*) const;
 
    const CollapsedBorderValue* currentBorderStyle() const { return m_currentBorder; }
    
    bool hasSections() const { return m_head || m_foot || m_firstBody; }

    void recalcSectionsIfNeeded() const
    {
        if (m_needsSectionRecalc)
            recalcSections();
    }

protected:
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);

private:
    virtual const char* renderName() const { return "RenderTable"; }

    virtual bool isTable() const { return true; }

    virtual bool avoidsFloats() const { return true; }

    virtual void removeChild(RenderObject* oldChild);

    virtual void paint(PaintInfo&, int tx, int ty);
    virtual void paintObject(PaintInfo&, int tx, int ty);
    virtual void paintBoxDecorations(PaintInfo&, int tx, int ty);
    virtual void paintMask(PaintInfo&, int tx, int ty);
    virtual void layout();
    virtual void computePreferredLogicalWidths();
    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int xPos, int yPos, int tx, int ty, HitTestAction);
    
    virtual int firstLineBoxBaseline() const;

    virtual RenderBlock* firstLineBlock() const;
    virtual void updateFirstLetter();
    
    virtual void setCellLogicalWidths();

    virtual void computeLogicalWidth();

    virtual IntRect overflowClipRect(int tx, int ty, OverlayScrollbarSizeRelevancy relevancy = IgnoreOverlayScrollbarSize);

    virtual void addOverflowFromChildren();

    void subtractCaptionRect(IntRect&) const;

    void recalcCaption(RenderBlock*) const;
    void recalcSections() const;
    void adjustLogicalHeightForCaption();

    mutable Vector<int> m_columnPos;
    mutable Vector<ColumnStruct> m_columns;

    mutable RenderBlock* m_caption;
    mutable RenderTableSection* m_head;
    mutable RenderTableSection* m_foot;
    mutable RenderTableSection* m_firstBody;

    OwnPtr<TableLayout> m_tableLayout;

    const CollapsedBorderValue* m_currentBorder;
    
    mutable bool m_hasColElements : 1;
    mutable bool m_needsSectionRecalc : 1;
    
    short m_hSpacing;
    short m_vSpacing;
    int m_borderStart;
    int m_borderEnd;
};

inline RenderTable* toRenderTable(RenderObject* object)
{
    ASSERT(!object || object->isTable());
    return static_cast<RenderTable*>(object);
}

inline const RenderTable* toRenderTable(const RenderObject* object)
{
    ASSERT(!object || object->isTable());
    return static_cast<const RenderTable*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderTable(const RenderTable*);

} // namespace WebCore

#endif // RenderTable_h
