/*
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#include "config.h"
#include "RenderTableCell.h"

#include "CollapsedBorderValue.h"
#include "FloatQuad.h"
#include "GraphicsContext.h"
#include "HTMLNames.h"
#include "HTMLTableCellElement.h"
#include "PaintInfo.h"
#include "RenderTableCol.h"
#include "RenderView.h"
#include "TransformState.h"

using namespace std;

namespace WebCore {

using namespace HTMLNames;

RenderTableCell::RenderTableCell(Node* node)
    : RenderBlock(node)
    , m_row(-1)
    , m_column(-1)
    , m_rowSpan(1)
    , m_columnSpan(1)
    , m_cellWidthChanged(false)
    , m_intrinsicPaddingBefore(0)
    , m_intrinsicPaddingAfter(0)
{
    updateFromElement();
}

void RenderTableCell::destroy()
{
    RenderTableSection* recalcSection = parent() ? section() : 0;

    RenderBlock::destroy();

    if (recalcSection)
        recalcSection->setNeedsCellRecalc();
}

void RenderTableCell::updateFromElement()
{
    Node* n = node();
    if (n && (n->hasTagName(tdTag) || n->hasTagName(thTag))) {
        HTMLTableCellElement* tc = static_cast<HTMLTableCellElement*>(n);
        int oldRSpan = m_rowSpan;
        int oldCSpan = m_columnSpan;

        m_columnSpan = tc->colSpan();
        m_rowSpan = tc->rowSpan();
        if ((oldRSpan != m_rowSpan || oldCSpan != m_columnSpan) && style() && parent()) {
            setNeedsLayoutAndPrefWidthsRecalc();
            if (section())
                section()->setNeedsCellRecalc();
        }
    }
}

Length RenderTableCell::styleOrColLogicalWidth() const
{
    Length w = style()->logicalWidth();
    if (!w.isAuto())
        return w;

    if (RenderTableCol* tableCol = table()->colElement(col())) {
        int colSpanCount = colSpan();

        Length colWidthSum = Length(0, Fixed);
        for (int i = 1; i <= colSpanCount; i++) {
            Length colWidth = tableCol->style()->logicalWidth();

            // Percentage value should be returned only for colSpan == 1.
            // Otherwise we return original width for the cell.
            if (!colWidth.isFixed()) {
                if (colSpanCount > 1)
                    return w;
                return colWidth;
            }

            colWidthSum = Length(colWidthSum.value() + colWidth.value(), Fixed);

            tableCol = table()->nextColElement(tableCol);
            // If no next <col> tag found for the span we just return what we have for now.
            if (!tableCol)
                break;
        }

        // Column widths specified on <col> apply to the border box of the cell.
        // Percentages don't need to be handled since they're always treated this way (even when specified on the cells).
        // See Bugzilla bug 8126 for details.
        if (colWidthSum.isFixed() && colWidthSum.value() > 0)
            colWidthSum = Length(max(0, colWidthSum.value() - borderAndPaddingLogicalWidth()), Fixed);
        return colWidthSum;
    }

    return w;
}

void RenderTableCell::computePreferredLogicalWidths()
{
    // The child cells rely on the grids up in the sections to do their computePreferredLogicalWidths work.  Normally the sections are set up early, as table
    // cells are added, but relayout can cause the cells to be freed, leaving stale pointers in the sections'
    // grids.  We must refresh those grids before the child cells try to use them.
    table()->recalcSectionsIfNeeded();

    RenderBlock::computePreferredLogicalWidths();
    if (node() && style()->autoWrap()) {
        // See if nowrap was set.
        Length w = styleOrColLogicalWidth();
        String nowrap = static_cast<Element*>(node())->getAttribute(nowrapAttr);
        if (!nowrap.isNull() && w.isFixed())
            // Nowrap is set, but we didn't actually use it because of the
            // fixed width set on the cell.  Even so, it is a WinIE/Moz trait
            // to make the minwidth of the cell into the fixed width.  They do this
            // even in strict mode, so do not make this a quirk.  Affected the top
            // of hiptop.com.
            m_minPreferredLogicalWidth = max(w.value(), m_minPreferredLogicalWidth);
    }
}

void RenderTableCell::computeLogicalWidth()
{
}

void RenderTableCell::updateLogicalWidth(int w)
{
    if (w == logicalWidth())
        return;

    setLogicalWidth(w);
    setCellWidthChanged(true);
}

void RenderTableCell::layout()
{
    layoutBlock(cellWidthChanged());
    setCellWidthChanged(false);
}

int RenderTableCell::paddingTop(bool includeIntrinsicPadding) const
{
    int result = RenderBlock::paddingTop();
    if (!includeIntrinsicPadding || !isHorizontalWritingMode())
        return result;
    return result + (style()->writingMode() == TopToBottomWritingMode ? intrinsicPaddingBefore() : intrinsicPaddingAfter());
}

int RenderTableCell::paddingBottom(bool includeIntrinsicPadding) const
{
    int result = RenderBlock::paddingBottom();
    if (!includeIntrinsicPadding || !isHorizontalWritingMode())
        return result;
    return result + (style()->writingMode() == TopToBottomWritingMode ? intrinsicPaddingAfter() : intrinsicPaddingBefore());
}

int RenderTableCell::paddingLeft(bool includeIntrinsicPadding) const
{
    int result = RenderBlock::paddingLeft();
    if (!includeIntrinsicPadding || isHorizontalWritingMode())
        return result;
    return result + (style()->writingMode() == LeftToRightWritingMode ? intrinsicPaddingBefore() : intrinsicPaddingAfter());
    
}

int RenderTableCell::paddingRight(bool includeIntrinsicPadding) const
{   
    int result = RenderBlock::paddingRight();
    if (!includeIntrinsicPadding || isHorizontalWritingMode())
        return result;
    return result + (style()->writingMode() == LeftToRightWritingMode ? intrinsicPaddingAfter() : intrinsicPaddingBefore());
}

int RenderTableCell::paddingBefore(bool includeIntrinsicPadding) const
{
    int result = RenderBlock::paddingBefore();
    if (!includeIntrinsicPadding)
        return result;
    return result + intrinsicPaddingBefore();
}

int RenderTableCell::paddingAfter(bool includeIntrinsicPadding) const
{
    int result = RenderBlock::paddingAfter();
    if (!includeIntrinsicPadding)
        return result;
    return result + intrinsicPaddingAfter();
}

void RenderTableCell::setOverrideSize(int size)
{
    clearIntrinsicPadding();
    RenderBlock::setOverrideSize(size);
}
    
void RenderTableCell::setOverrideSizeFromRowHeight(int rowHeight)
{
    clearIntrinsicPadding();
    RenderBlock::setOverrideSize(max(0, rowHeight - borderBefore() - paddingBefore() - borderAfter() - paddingAfter()));
}

IntSize RenderTableCell::offsetFromContainer(RenderObject* o, const IntPoint& point) const
{
    ASSERT(o == container());

    IntSize offset = RenderBlock::offsetFromContainer(o, point);
    if (parent())
        offset.expand(-parentBox()->x(), -parentBox()->y());

    return offset;
}

IntRect RenderTableCell::clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer)
{
    // If the table grid is dirty, we cannot get reliable information about adjoining cells,
    // so we ignore outside borders. This should not be a problem because it means that
    // the table is going to recalculate the grid, relayout and repaint its current rect, which
    // includes any outside borders of this cell.
    if (!table()->collapseBorders() || table()->needsSectionRecalc())
        return RenderBlock::clippedOverflowRectForRepaint(repaintContainer);

    bool rtl = !table()->style()->isLeftToRightDirection();
    int outlineSize = style()->outlineSize();
    int left = max(borderHalfLeft(true), outlineSize);
    int right = max(borderHalfRight(true), outlineSize);
    int top = max(borderHalfTop(true), outlineSize);
    int bottom = max(borderHalfBottom(true), outlineSize);
    if ((left && !rtl) || (right && rtl)) {
        if (RenderTableCell* before = table()->cellBefore(this)) {
            top = max(top, before->borderHalfTop(true));
            bottom = max(bottom, before->borderHalfBottom(true));
        }
    }
    if ((left && rtl) || (right && !rtl)) {
        if (RenderTableCell* after = table()->cellAfter(this)) {
            top = max(top, after->borderHalfTop(true));
            bottom = max(bottom, after->borderHalfBottom(true));
        }
    }
    if (top) {
        if (RenderTableCell* above = table()->cellAbove(this)) {
            left = max(left, above->borderHalfLeft(true));
            right = max(right, above->borderHalfRight(true));
        }
    }
    if (bottom) {
        if (RenderTableCell* below = table()->cellBelow(this)) {
            left = max(left, below->borderHalfLeft(true));
            right = max(right, below->borderHalfRight(true));
        }
    }
    left = max(left, -minXVisualOverflow());
    top = max(top, -minYVisualOverflow());
    IntRect r(-left, - top, left + max(width() + right, maxXVisualOverflow()), top + max(height() + bottom, maxYVisualOverflow()));

    if (RenderView* v = view()) {
        // FIXME: layoutDelta needs to be applied in parts before/after transforms and
        // repaint containers. https://bugs.webkit.org/show_bug.cgi?id=23308
        r.move(v->layoutDelta());
    }
    computeRectForRepaint(repaintContainer, r);
    return r;
}

void RenderTableCell::computeRectForRepaint(RenderBoxModelObject* repaintContainer, IntRect& r, bool fixed)
{
    if (repaintContainer == this)
        return;
    r.setY(r.y());
    RenderView* v = view();
    if ((!v || !v->layoutStateEnabled() || repaintContainer) && parent())
        r.move(-parentBox()->x(), -parentBox()->y()); // Rows are in the same coordinate space, so don't add their offset in.
    RenderBlock::computeRectForRepaint(repaintContainer, r, fixed);
}

int RenderTableCell::cellBaselinePosition() const
{
    // <http://www.w3.org/TR/2007/CR-CSS21-20070719/tables.html#height-layout>: The baseline of a cell is the baseline of
    // the first in-flow line box in the cell, or the first in-flow table-row in the cell, whichever comes first. If there
    // is no such line box or table-row, the baseline is the bottom of content edge of the cell box.
    int firstLineBaseline = firstLineBoxBaseline();
    if (firstLineBaseline != -1)
        return firstLineBaseline;
    return paddingBefore() + borderBefore() + contentLogicalHeight();
}

void RenderTableCell::styleWillChange(StyleDifference diff, const RenderStyle* newStyle)
{
    if (parent() && section() && style() && style()->height() != newStyle->height())
        section()->setNeedsCellRecalc();

    ASSERT(newStyle->display() == TABLE_CELL);

    RenderBlock::styleWillChange(diff, newStyle);
}

void RenderTableCell::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBlock::styleDidChange(diff, oldStyle);
    setHasBoxDecorations(true);
}

// The following rules apply for resolving conflicts and figuring out which border
// to use.
// (1) Borders with the 'border-style' of 'hidden' take precedence over all other conflicting 
// borders. Any border with this value suppresses all borders at this location.
// (2) Borders with a style of 'none' have the lowest priority. Only if the border properties of all 
// the elements meeting at this edge are 'none' will the border be omitted (but note that 'none' is 
// the default value for the border style.)
// (3) If none of the styles are 'hidden' and at least one of them is not 'none', then narrow borders 
// are discarded in favor of wider ones. If several have the same 'border-width' then styles are preferred 
// in this order: 'double', 'solid', 'dashed', 'dotted', 'ridge', 'outset', 'groove', and the lowest: 'inset'.
// (4) If border styles differ only in color, then a style set on a cell wins over one on a row, 
// which wins over a row group, column, column group and, lastly, table. It is undefined which color 
// is used when two elements of the same type disagree.
static int compareBorders(const CollapsedBorderValue& border1, const CollapsedBorderValue& border2)
{
    // Sanity check the values passed in. The null border have lowest priority.
    if (!border2.exists()) {
        if (!border1.exists())
            return 0;
        return 1;
    }
    if (!border1.exists())
        return -1;

    // Rule #1 above.
    if (border2.style() == BHIDDEN) {
        if (border1.style() == BHIDDEN)
            return 0;
        return -1;
    }
    if (border1.style() == BHIDDEN)
        return 1;
    
    // Rule #2 above.  A style of 'none' has lowest priority and always loses to any other border.
    if (border2.style() == BNONE) {
        if (border1.style() == BNONE)
            return 0;
        return 1;
    }
    if (border1.style() == BNONE)
        return -1;

    // The first part of rule #3 above. Wider borders win.
    if (border1.width() != border2.width())
        return border1.width() < border2.width() ? -1 : 1;
    
    // The borders have equal width.  Sort by border style.
    if (border1.style() != border2.style())
        return border1.style() < border2.style() ? -1 : 1;
    
    // The border have the same width and style.  Rely on precedence (cell over row over row group, etc.)
    if (border1.precedence() == border2.precedence())
        return 0;
    return border1.precedence() < border2.precedence() ? -1 : 1;
}

static CollapsedBorderValue chooseBorder(const CollapsedBorderValue& border1, const CollapsedBorderValue& border2)
{
    const CollapsedBorderValue& border = compareBorders(border1, border2) < 0 ? border2 : border1;
    return border.style() == BHIDDEN ? CollapsedBorderValue() : border;
}

CollapsedBorderValue RenderTableCell::collapsedStartBorder() const
{
    RenderTable* table = this->table();
    bool isStartColumn = col() == 0;

    // For the start border, we need to check, in order of precedence:
    // (1) Our start border.
    int start = CSSProperty::resolveDirectionAwareProperty(CSSPropertyWebkitBorderStartColor, table->style()->direction(), table->style()->writingMode());
    int end = CSSProperty::resolveDirectionAwareProperty(CSSPropertyWebkitBorderEndColor, table->style()->direction(), table->style()->writingMode());
    CollapsedBorderValue result(&style()->borderStart(), style()->visitedDependentColor(start), BCELL);

    // (2) The end border of the preceding cell.
    if (RenderTableCell* prevCell = table->cellBefore(this)) {
        CollapsedBorderValue prevCellBorder = CollapsedBorderValue(&prevCell->style()->borderEnd(), prevCell->style()->visitedDependentColor(end), BCELL);
        result = chooseBorder(prevCellBorder, result);
        if (!result.exists())
            return result;
    } else if (isStartColumn) {
        // (3) Our row's start border.
        result = chooseBorder(result, CollapsedBorderValue(&parent()->style()->borderStart(), parent()->style()->visitedDependentColor(start), BROW));
        if (!result.exists())
            return result;
        
        // (4) Our row group's start border.
        result = chooseBorder(result, CollapsedBorderValue(&section()->style()->borderStart(), section()->style()->visitedDependentColor(start), BROWGROUP));
        if (!result.exists())
            return result;
    }
    
    // (5) Our column and column group's start borders.
    bool startColEdge;
    bool endColEdge;
    RenderTableCol* colElt = table->colElement(col(), &startColEdge, &endColEdge);
    if (colElt && startColEdge) {
        result = chooseBorder(result, CollapsedBorderValue(&colElt->style()->borderStart(), colElt->style()->visitedDependentColor(start), BCOL));
        if (!result.exists())
            return result;
        if (colElt->parent()->isTableCol() && !colElt->previousSibling()) {
            result = chooseBorder(result, CollapsedBorderValue(&colElt->parent()->style()->borderStart(), colElt->parent()->style()->visitedDependentColor(start), BCOLGROUP));
            if (!result.exists())
                return result;
        }
    }
    
    // (6) The end border of the preceding column.
    if (!isStartColumn) {
        colElt = table->colElement(col() -1, &startColEdge, &endColEdge);
        if (colElt && endColEdge) {
            CollapsedBorderValue endBorder = CollapsedBorderValue(&colElt->style()->borderEnd(), colElt->style()->visitedDependentColor(end), BCOL);
            result = chooseBorder(endBorder, result);
            if (!result.exists())
                return result;
        }
    } else {
        // (7) The table's start border.
        result = chooseBorder(result, CollapsedBorderValue(&table->style()->borderStart(), table->style()->visitedDependentColor(start), BTABLE));
        if (!result.exists())
            return result;
    }
    
    return result;
}

CollapsedBorderValue RenderTableCell::collapsedEndBorder() const
{
    RenderTable* table = this->table();
    bool isEndColumn = table->colToEffCol(col() + colSpan() - 1) == table->numEffCols() - 1;

    // For end border, we need to check, in order of precedence:
    // (1) Our end border.
    int start = CSSProperty::resolveDirectionAwareProperty(CSSPropertyWebkitBorderStartColor, table->style()->direction(), table->style()->writingMode());
    int end = CSSProperty::resolveDirectionAwareProperty(CSSPropertyWebkitBorderEndColor, table->style()->direction(), table->style()->writingMode());
    CollapsedBorderValue result = CollapsedBorderValue(&style()->borderEnd(), style()->visitedDependentColor(end), BCELL);
    
    // (2) The start border of the following cell.
    if (!isEndColumn) {
        RenderTableCell* nextCell = table->cellAfter(this);
        if (nextCell && nextCell->style()) {
            CollapsedBorderValue startBorder = CollapsedBorderValue(&nextCell->style()->borderStart(), nextCell->style()->visitedDependentColor(start), BCELL);
            result = chooseBorder(result, startBorder);
            if (!result.exists())
                return result;
        }
    } else {
        // (3) Our row's end border.
        result = chooseBorder(result, CollapsedBorderValue(&parent()->style()->borderEnd(), parent()->style()->visitedDependentColor(end), BROW));
        if (!result.exists())
            return result;
        
        // (4) Our row group's end border.
        result = chooseBorder(result, CollapsedBorderValue(&section()->style()->borderEnd(), section()->style()->visitedDependentColor(end), BROWGROUP));
        if (!result.exists())
            return result;
    }
    
    // (5) Our column and column group's end borders.
    bool startColEdge;
    bool endColEdge;
    RenderTableCol* colElt = table->colElement(col() + colSpan() - 1, &startColEdge, &endColEdge);
    if (colElt && endColEdge) {
        result = chooseBorder(result, CollapsedBorderValue(&colElt->style()->borderEnd(), colElt->style()->visitedDependentColor(end), BCOL));
        if (!result.exists())
            return result;
        if (colElt->parent()->isTableCol() && !colElt->nextSibling()) {
            result = chooseBorder(result, CollapsedBorderValue(&colElt->parent()->style()->borderEnd(), colElt->parent()->style()->visitedDependentColor(end), BCOLGROUP));
            if (!result.exists())
                return result;
        }
    }
    
    // (6) The start border of the next column.
    if (!isEndColumn) {
        colElt = table->colElement(col() + colSpan(), &startColEdge, &endColEdge);
        if (colElt && startColEdge) {
            CollapsedBorderValue startBorder = CollapsedBorderValue(&colElt->style()->borderStart(), colElt->style()->visitedDependentColor(start), BCOL);
            result = chooseBorder(result, startBorder);
            if (!result.exists())
                return result;
        }
    } else {
        // (7) The table's end border.
        result = chooseBorder(result, CollapsedBorderValue(&table->style()->borderEnd(), table->style()->visitedDependentColor(end), BTABLE));
        if (!result.exists())
            return result;
    }
    
    return result;
}

CollapsedBorderValue RenderTableCell::collapsedBeforeBorder() const
{
    RenderTable* table = this->table();

    // For before border, we need to check, in order of precedence:
    // (1) Our before border.
    int before = CSSProperty::resolveDirectionAwareProperty(CSSPropertyWebkitBorderBeforeColor, table->style()->direction(), table->style()->writingMode());
    int after = CSSProperty::resolveDirectionAwareProperty(CSSPropertyWebkitBorderAfterColor, table->style()->direction(), table->style()->writingMode());
    CollapsedBorderValue result = CollapsedBorderValue(&style()->borderBefore(), style()->visitedDependentColor(before), BCELL);
    
    RenderTableCell* prevCell = table->cellAbove(this);
    if (prevCell) {
        // (2) A before cell's after border.
        result = chooseBorder(CollapsedBorderValue(&prevCell->style()->borderAfter(), prevCell->style()->visitedDependentColor(after), BCELL), result);
        if (!result.exists())
            return result;
    }
    
    // (3) Our row's before border.
    result = chooseBorder(result, CollapsedBorderValue(&parent()->style()->borderBefore(), parent()->style()->visitedDependentColor(before), BROW));
    if (!result.exists())
        return result;
    
    // (4) The previous row's after border.
    if (prevCell) {
        RenderObject* prevRow = 0;
        if (prevCell->section() == section())
            prevRow = parent()->previousSibling();
        else
            prevRow = prevCell->section()->lastChild();
    
        if (prevRow) {
            result = chooseBorder(CollapsedBorderValue(&prevRow->style()->borderAfter(), prevRow->style()->visitedDependentColor(after), BROW), result);
            if (!result.exists())
                return result;
        }
    }
    
    // Now check row groups.
    RenderTableSection* currSection = section();
    if (!row()) {
        // (5) Our row group's before border.
        result = chooseBorder(result, CollapsedBorderValue(&currSection->style()->borderBefore(), currSection->style()->visitedDependentColor(before), BROWGROUP));
        if (!result.exists())
            return result;
        
        // (6) Previous row group's after border.
        currSection = table->sectionAbove(currSection);
        if (currSection) {
            result = chooseBorder(CollapsedBorderValue(&currSection->style()->borderAfter(), currSection->style()->visitedDependentColor(after), BROWGROUP), result);
            if (!result.exists())
                return result;
        }
    }
    
    if (!currSection) {
        // (8) Our column and column group's before borders.
        RenderTableCol* colElt = table->colElement(col());
        if (colElt) {
            result = chooseBorder(result, CollapsedBorderValue(&colElt->style()->borderBefore(), colElt->style()->visitedDependentColor(before), BCOL));
            if (!result.exists())
                return result;
            if (colElt->parent()->isTableCol()) {
                result = chooseBorder(result, CollapsedBorderValue(&colElt->parent()->style()->borderBefore(), colElt->parent()->style()->visitedDependentColor(before), BCOLGROUP));
                if (!result.exists())
                    return result;
            }
        }
        
        // (9) The table's before border.
        result = chooseBorder(result, CollapsedBorderValue(&table->style()->borderBefore(), table->style()->visitedDependentColor(before), BTABLE));
        if (!result.exists())
            return result;
    }
    
    return result;
}

CollapsedBorderValue RenderTableCell::collapsedAfterBorder() const
{
    RenderTable* table = this->table();

    // For after border, we need to check, in order of precedence:
    // (1) Our after border.
    int before = CSSProperty::resolveDirectionAwareProperty(CSSPropertyWebkitBorderBeforeColor, table->style()->direction(), table->style()->writingMode());
    int after = CSSProperty::resolveDirectionAwareProperty(CSSPropertyWebkitBorderAfterColor, table->style()->direction(), table->style()->writingMode());
    CollapsedBorderValue result = CollapsedBorderValue(&style()->borderAfter(), style()->visitedDependentColor(after), BCELL);
    
    RenderTableCell* nextCell = table->cellBelow(this);
    if (nextCell) {
        // (2) An after cell's before border.
        result = chooseBorder(result, CollapsedBorderValue(&nextCell->style()->borderBefore(), nextCell->style()->visitedDependentColor(before), BCELL));
        if (!result.exists())
            return result;
    }
    
    // (3) Our row's after border. (FIXME: Deal with rowspan!)
    result = chooseBorder(result, CollapsedBorderValue(&parent()->style()->borderAfter(), parent()->style()->visitedDependentColor(after), BROW));
    if (!result.exists())
        return result;
    
    // (4) The next row's before border.
    if (nextCell) {
        result = chooseBorder(result, CollapsedBorderValue(&nextCell->parent()->style()->borderBefore(), nextCell->parent()->style()->visitedDependentColor(before), BROW));
        if (!result.exists())
            return result;
    }
    
    // Now check row groups.
    RenderTableSection* currSection = section();
    if (row() + rowSpan() >= currSection->numRows()) {
        // (5) Our row group's after border.
        result = chooseBorder(result, CollapsedBorderValue(&currSection->style()->borderAfter(), currSection->style()->visitedDependentColor(after), BROWGROUP));
        if (!result.exists())
            return result;
        
        // (6) Following row group's before border.
        currSection = table->sectionBelow(currSection);
        if (currSection) {
            result = chooseBorder(result, CollapsedBorderValue(&currSection->style()->borderBefore(), currSection->style()->visitedDependentColor(before), BROWGROUP));
            if (!result.exists())
                return result;
        }
    }
    
    if (!currSection) {
        // (8) Our column and column group's after borders.
        RenderTableCol* colElt = table->colElement(col());
        if (colElt) {
            result = chooseBorder(result, CollapsedBorderValue(&colElt->style()->borderAfter(), colElt->style()->visitedDependentColor(after), BCOL));
            if (!result.exists()) return result;
            if (colElt->parent()->isTableCol()) {
                result = chooseBorder(result, CollapsedBorderValue(&colElt->parent()->style()->borderAfter(), colElt->parent()->style()->visitedDependentColor(after), BCOLGROUP));
                if (!result.exists())
                    return result;
            }
        }
        
        // (9) The table's after border.
        result = chooseBorder(result, CollapsedBorderValue(&table->style()->borderAfter(), table->style()->visitedDependentColor(after), BTABLE));
        if (!result.exists())
            return result;
    }
    
    return result;    
}

CollapsedBorderValue RenderTableCell::collapsedLeftBorder() const
{
    RenderStyle* tableStyle = table()->style();
    if (tableStyle->isHorizontalWritingMode())
        return tableStyle->isLeftToRightDirection() ? collapsedStartBorder() : collapsedEndBorder();
    return tableStyle->isFlippedBlocksWritingMode() ? collapsedAfterBorder() : collapsedBeforeBorder();
}

CollapsedBorderValue RenderTableCell::collapsedRightBorder() const
{
    RenderStyle* tableStyle = table()->style();
    if (tableStyle->isHorizontalWritingMode())
        return tableStyle->isLeftToRightDirection() ? collapsedEndBorder() : collapsedStartBorder();
    return tableStyle->isFlippedBlocksWritingMode() ? collapsedBeforeBorder() : collapsedAfterBorder();
}

CollapsedBorderValue RenderTableCell::collapsedTopBorder() const
{
    RenderStyle* tableStyle = table()->style();
    if (tableStyle->isHorizontalWritingMode())
        return tableStyle->isFlippedBlocksWritingMode() ? collapsedAfterBorder() : collapsedBeforeBorder();
    return tableStyle->isLeftToRightDirection() ? collapsedStartBorder() : collapsedEndBorder();
}

CollapsedBorderValue RenderTableCell::collapsedBottomBorder() const
{
    RenderStyle* tableStyle = table()->style();
    if (tableStyle->isHorizontalWritingMode())
        return tableStyle->isFlippedBlocksWritingMode() ? collapsedBeforeBorder() : collapsedAfterBorder();
    return tableStyle->isLeftToRightDirection() ? collapsedEndBorder() : collapsedStartBorder();
}

int RenderTableCell::borderLeft() const
{
    return table()->collapseBorders() ? borderHalfLeft(false) : RenderBlock::borderLeft();
}

int RenderTableCell::borderRight() const
{
    return table()->collapseBorders() ? borderHalfRight(false) : RenderBlock::borderRight();
}

int RenderTableCell::borderTop() const
{
    return table()->collapseBorders() ? borderHalfTop(false) : RenderBlock::borderTop();
}

int RenderTableCell::borderBottom() const
{
    return table()->collapseBorders() ? borderHalfBottom(false) : RenderBlock::borderBottom();
}

// FIXME: https://bugs.webkit.org/show_bug.cgi?id=46191, make the collapsed border drawing
// work with different block flow values instead of being hard-coded to top-to-bottom.
int RenderTableCell::borderStart() const
{
    return table()->collapseBorders() ? borderHalfStart(false) : RenderBlock::borderStart();
}

int RenderTableCell::borderEnd() const
{
    return table()->collapseBorders() ? borderHalfEnd(false) : RenderBlock::borderEnd();
}

int RenderTableCell::borderBefore() const
{
    return table()->collapseBorders() ? borderHalfBefore(false) : RenderBlock::borderBefore();
}

int RenderTableCell::borderAfter() const
{
    return table()->collapseBorders() ? borderHalfAfter(false) : RenderBlock::borderAfter();
}

int RenderTableCell::borderHalfLeft(bool outer) const
{
    RenderStyle* tableStyle = table()->style();
    if (tableStyle->isHorizontalWritingMode())
        return tableStyle->isLeftToRightDirection() ? borderHalfStart(outer) : borderHalfEnd(outer);
    return tableStyle->isFlippedBlocksWritingMode() ? borderHalfAfter(outer) : borderHalfBefore(outer);
}

int RenderTableCell::borderHalfRight(bool outer) const
{
    RenderStyle* tableStyle = table()->style();
    if (tableStyle->isHorizontalWritingMode())
        return tableStyle->isLeftToRightDirection() ? borderHalfEnd(outer) : borderHalfStart(outer);
    return tableStyle->isFlippedBlocksWritingMode() ? borderHalfBefore(outer) : borderHalfAfter(outer);
}

int RenderTableCell::borderHalfTop(bool outer) const
{
    RenderStyle* tableStyle = table()->style();
    if (tableStyle->isHorizontalWritingMode())
        return tableStyle->isFlippedBlocksWritingMode() ? borderHalfAfter(outer) : borderHalfBefore(outer);
    return tableStyle->isLeftToRightDirection() ? borderHalfStart(outer) : borderHalfEnd(outer);
}

int RenderTableCell::borderHalfBottom(bool outer) const
{
    RenderStyle* tableStyle = table()->style();
    if (tableStyle->isHorizontalWritingMode())
        return tableStyle->isFlippedBlocksWritingMode() ? borderHalfBefore(outer) : borderHalfAfter(outer);
    return tableStyle->isLeftToRightDirection() ? borderHalfEnd(outer) : borderHalfStart(outer);
}

int RenderTableCell::borderHalfStart(bool outer) const
{
    CollapsedBorderValue border = collapsedStartBorder();
    if (border.exists())
        return (border.width() + ((table()->style()->isLeftToRightDirection() ^ outer) ? 1 : 0)) / 2; // Give the extra pixel to top and left.
    return 0;
}
    
int RenderTableCell::borderHalfEnd(bool outer) const
{
    CollapsedBorderValue border = collapsedEndBorder();
    if (border.exists())
        return (border.width() + ((table()->style()->isLeftToRightDirection() ^ outer) ? 0 : 1)) / 2;
    return 0;
}

int RenderTableCell::borderHalfBefore(bool outer) const
{
    CollapsedBorderValue border = collapsedBeforeBorder();
    if (border.exists())
        return (border.width() + ((table()->style()->isFlippedBlocksWritingMode() ^ outer) ? 0 : 1)) / 2; // Give the extra pixel to top and left.
    return 0;
}

int RenderTableCell::borderHalfAfter(bool outer) const
{
    CollapsedBorderValue border = collapsedAfterBorder();
    if (border.exists())
        return (border.width() + ((table()->style()->isFlippedBlocksWritingMode() ^ outer) ? 1 : 0)) / 2;
    return 0;
}

void RenderTableCell::paint(PaintInfo& paintInfo, int tx, int ty)
{
    if (paintInfo.phase == PaintPhaseCollapsedTableBorders && style()->visibility() == VISIBLE) {
        if (!paintInfo.shouldPaintWithinRoot(this))
            return;

        tx += x();
        ty += y();
        int os = 2 * maximalOutlineSize(paintInfo.phase);
        if (ty - table()->outerBorderTop() < paintInfo.rect.maxY() + os
            && ty + height() + table()->outerBorderBottom() > paintInfo.rect.y() - os)
            paintCollapsedBorder(paintInfo.context, tx, ty, width(), height());
        return;
    } 
    
    RenderBlock::paint(paintInfo, tx, ty);
}

static EBorderStyle collapsedBorderStyle(EBorderStyle style)
{
    if (style == OUTSET)
        return GROOVE;
    if (style == INSET)
        return RIDGE;
    return style;
}

struct CollapsedBorder {
    CollapsedBorderValue borderValue;
    BoxSide side;
    bool shouldPaint;
    int x1;
    int y1;
    int x2;
    int y2;
    EBorderStyle style;
};

class CollapsedBorders {
public:
    CollapsedBorders()
        : m_count(0)
    {
    }
    
    void addBorder(const CollapsedBorderValue& borderValue, BoxSide borderSide, bool shouldPaint,
                   int x1, int y1, int x2, int y2, EBorderStyle borderStyle)
    {
        if (borderValue.exists() && shouldPaint) {
            m_borders[m_count].borderValue = borderValue;
            m_borders[m_count].side = borderSide;
            m_borders[m_count].shouldPaint = shouldPaint;
            m_borders[m_count].x1 = x1;
            m_borders[m_count].x2 = x2;
            m_borders[m_count].y1 = y1;
            m_borders[m_count].y2 = y2;
            m_borders[m_count].style = borderStyle;
            m_count++;
        }
    }

    CollapsedBorder* nextBorder()
    {
        for (int i = 0; i < m_count; i++) {
            if (m_borders[i].borderValue.exists() && m_borders[i].shouldPaint) {
                m_borders[i].shouldPaint = false;
                return &m_borders[i];
            }
        }
        
        return 0;
    }
    
    CollapsedBorder m_borders[4];
    int m_count;
};

static void addBorderStyle(RenderTableCell::CollapsedBorderStyles& borderStyles, CollapsedBorderValue borderValue)
{
    if (!borderValue.exists())
        return;
    size_t count = borderStyles.size();
    for (size_t i = 0; i < count; ++i)
        if (borderStyles[i] == borderValue)
            return;
    borderStyles.append(borderValue);
}

void RenderTableCell::collectBorderStyles(CollapsedBorderStyles& borderStyles) const
{
    addBorderStyle(borderStyles, collapsedStartBorder());
    addBorderStyle(borderStyles, collapsedEndBorder());
    addBorderStyle(borderStyles, collapsedBeforeBorder());
    addBorderStyle(borderStyles, collapsedAfterBorder());
}

static int compareBorderStylesForQSort(const void* pa, const void* pb)
{
    const CollapsedBorderValue* a = static_cast<const CollapsedBorderValue*>(pa);
    const CollapsedBorderValue* b = static_cast<const CollapsedBorderValue*>(pb);
    if (*a == *b)
        return 0;
    return compareBorders(*a, *b);
}

void RenderTableCell::sortBorderStyles(CollapsedBorderStyles& borderStyles)
{
    qsort(borderStyles.data(), borderStyles.size(), sizeof(CollapsedBorderValue),
        compareBorderStylesForQSort);
}

void RenderTableCell::paintCollapsedBorder(GraphicsContext* graphicsContext, int tx, int ty, int w, int h)
{
    if (!table()->currentBorderStyle() || graphicsContext->paintingDisabled())
        return;
    
    CollapsedBorderValue leftVal = collapsedLeftBorder();
    CollapsedBorderValue rightVal = collapsedRightBorder();
    CollapsedBorderValue topVal = collapsedTopBorder();
    CollapsedBorderValue bottomVal = collapsedBottomBorder();
     
    // Adjust our x/y/width/height so that we paint the collapsed borders at the correct location.
    int topWidth = topVal.width();
    int bottomWidth = bottomVal.width();
    int leftWidth = leftVal.width();
    int rightWidth = rightVal.width();
    
    tx -= leftWidth / 2;
    ty -= topWidth / 2;
    w += leftWidth / 2 + (rightWidth + 1) / 2;
    h += topWidth / 2 + (bottomWidth + 1) / 2;
    
    EBorderStyle topStyle = collapsedBorderStyle(topVal.style());
    EBorderStyle bottomStyle = collapsedBorderStyle(bottomVal.style());
    EBorderStyle leftStyle = collapsedBorderStyle(leftVal.style());
    EBorderStyle rightStyle = collapsedBorderStyle(rightVal.style());
    
    bool renderTop = topStyle > BHIDDEN && !topVal.isTransparent();
    bool renderBottom = bottomStyle > BHIDDEN && !bottomVal.isTransparent();
    bool renderLeft = leftStyle > BHIDDEN && !leftVal.isTransparent();
    bool renderRight = rightStyle > BHIDDEN && !rightVal.isTransparent();

    // We never paint diagonals at the joins.  We simply let the border with the highest
    // precedence paint on top of borders with lower precedence.  
    CollapsedBorders borders;
    borders.addBorder(topVal, BSTop, renderTop, tx, ty, tx + w, ty + topWidth, topStyle);
    borders.addBorder(bottomVal, BSBottom, renderBottom, tx, ty + h - bottomWidth, tx + w, ty + h, bottomStyle);
    borders.addBorder(leftVal, BSLeft, renderLeft, tx, ty, tx + leftWidth, ty + h, leftStyle);
    borders.addBorder(rightVal, BSRight, renderRight, tx + w - rightWidth, ty, tx + w, ty + h, rightStyle);

    const AffineTransform& currentCTM = graphicsContext->getCTM();
    bool antialias = !currentCTM.isIdentityOrTranslationOrFlipped();
    
    for (CollapsedBorder* border = borders.nextBorder(); border; border = borders.nextBorder()) {
        if (border->borderValue == *table()->currentBorderStyle())
            drawLineForBoxSide(graphicsContext, border->x1, border->y1, border->x2, border->y2, border->side, 
                               border->borderValue.color(), border->style, 0, 0, antialias);
    }
}

void RenderTableCell::paintBackgroundsBehindCell(PaintInfo& paintInfo, int tx, int ty, RenderObject* backgroundObject)
{
    if (!paintInfo.shouldPaintWithinRoot(this))
        return;

    if (!backgroundObject)
        return;

    if (style()->visibility() != VISIBLE)
        return;

    RenderTable* tableElt = table();
    if (!tableElt->collapseBorders() && style()->emptyCells() == HIDE && !firstChild())
        return;

    if (backgroundObject != this) {
        tx += x();
        ty += y();
    }

    int w = width();
    int h = height();

    Color c = backgroundObject->style()->visitedDependentColor(CSSPropertyBackgroundColor);
    const FillLayer* bgLayer = backgroundObject->style()->backgroundLayers();

    if (bgLayer->hasImage() || c.isValid()) {
        // We have to clip here because the background would paint
        // on top of the borders otherwise.  This only matters for cells and rows.
        bool shouldClip = backgroundObject->hasLayer() && (backgroundObject == this || backgroundObject == parent()) && tableElt->collapseBorders();
        GraphicsContextStateSaver stateSaver(*paintInfo.context, shouldClip);
        if (shouldClip) {
            IntRect clipRect(tx + borderLeft(), ty + borderTop(),
                w - borderLeft() - borderRight(), h - borderTop() - borderBottom());
            paintInfo.context->clip(clipRect);
        }
        paintFillLayers(paintInfo, c, bgLayer, tx, ty, w, h, BackgroundBleedNone, CompositeSourceOver, backgroundObject);
    }
}

void RenderTableCell::paintBoxDecorations(PaintInfo& paintInfo, int tx, int ty)
{
    if (!paintInfo.shouldPaintWithinRoot(this))
        return;

    RenderTable* tableElt = table();
    if (!tableElt->collapseBorders() && style()->emptyCells() == HIDE && !firstChild())
        return;

    int w = width();
    int h = height();
   
    paintBoxShadow(paintInfo.context, tx, ty, w, h, style(), Normal);
    
    // Paint our cell background.
    paintBackgroundsBehindCell(paintInfo, tx, ty, this);

    paintBoxShadow(paintInfo.context, tx, ty, w, h, style(), Inset);

    if (!style()->hasBorder() || tableElt->collapseBorders())
        return;

    paintBorder(paintInfo.context, tx, ty, w, h, style());
}

void RenderTableCell::paintMask(PaintInfo& paintInfo, int tx, int ty)
{
    if (style()->visibility() != VISIBLE || paintInfo.phase != PaintPhaseMask)
        return;

    RenderTable* tableElt = table();
    if (!tableElt->collapseBorders() && style()->emptyCells() == HIDE && !firstChild())
        return;

    int w = width();
    int h = height();
   
    paintMaskImages(paintInfo, tx, ty, w, h);
}

void RenderTableCell::scrollbarsChanged(bool horizontalScrollbarChanged, bool verticalScrollbarChanged)
{
    int scrollbarHeight = scrollbarLogicalHeight();
    if (!scrollbarHeight)
        return; // Not sure if we should be doing something when a scrollbar goes away or not.
    
    // We only care if the scrollbar that affects our intrinsic padding has been added.
    if ((isHorizontalWritingMode() && !horizontalScrollbarChanged) ||
        (!isHorizontalWritingMode() && !verticalScrollbarChanged))
        return;

    // Shrink our intrinsic padding as much as possible to accommodate the scrollbar.
    if (style()->verticalAlign() == MIDDLE) {
        int totalHeight = logicalHeight();
        int heightWithoutIntrinsicPadding = totalHeight - intrinsicPaddingBefore() - intrinsicPaddingAfter();
        totalHeight -= scrollbarHeight;
        int newBeforePadding = (totalHeight - heightWithoutIntrinsicPadding) / 2;
        int newAfterPadding = totalHeight - heightWithoutIntrinsicPadding - newBeforePadding;
        setIntrinsicPaddingBefore(newBeforePadding);
        setIntrinsicPaddingAfter(newAfterPadding);
    } else
        setIntrinsicPaddingAfter(intrinsicPaddingAfter() - scrollbarHeight);
}

} // namespace WebCore
