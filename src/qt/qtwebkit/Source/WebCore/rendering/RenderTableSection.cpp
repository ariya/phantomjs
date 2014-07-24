/*
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@nypop.com)
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
#include "RenderTableSection.h"
#include "Document.h"
#include "HitTestResult.h"
#include "HTMLNames.h"
#include "PaintInfo.h"
#include "RenderTableCell.h"
#include "RenderTableCol.h"
#include "RenderTableRow.h"
#include "RenderView.h"
#include "StyleInheritedData.h"
#include <limits>
#include <wtf/HashSet.h>
#include <wtf/StackStats.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;

// Those 2 variables are used to balance the memory consumption vs the repaint time on big tables.
static unsigned gMinTableSizeToUseFastPaintPathWithOverflowingCell = 75 * 75;
static float gMaxAllowedOverflowingCellRatioForFastPaintPath = 0.1f;

static inline void setRowLogicalHeightToRowStyleLogicalHeightIfNotRelative(RenderTableSection::RowStruct& row)
{
    ASSERT(row.rowRenderer);
    row.logicalHeight = row.rowRenderer->style()->logicalHeight();
    if (row.logicalHeight.isRelative())
        row.logicalHeight = Length();
}

static inline void updateLogicalHeightForCell(RenderTableSection::RowStruct& row, const RenderTableCell* cell)
{
    // We ignore height settings on rowspan cells.
    if (cell->rowSpan() != 1)
        return;

    Length logicalHeight = cell->style()->logicalHeight();
    if (logicalHeight.isPositive() || (logicalHeight.isRelative() && logicalHeight.value() >= 0)) {
        Length cRowLogicalHeight = row.logicalHeight;
        switch (logicalHeight.type()) {
        case Percent:
            if (!(cRowLogicalHeight.isPercent())
                || (cRowLogicalHeight.isPercent() && cRowLogicalHeight.percent() < logicalHeight.percent()))
                row.logicalHeight = logicalHeight;
            break;
        case Fixed:
            if (cRowLogicalHeight.type() < Percent
                || (cRowLogicalHeight.isFixed() && cRowLogicalHeight.value() < logicalHeight.value()))
                row.logicalHeight = logicalHeight;
            break;
        case Relative:
        default:
            break;
        }
    }
}


RenderTableSection::RenderTableSection(Element* element)
    : RenderBox(element)
    , m_cCol(0)
    , m_cRow(0)
    , m_outerBorderStart(0)
    , m_outerBorderEnd(0)
    , m_outerBorderBefore(0)
    , m_outerBorderAfter(0)
    , m_needsCellRecalc(false)
    , m_hasMultipleCellLevels(false)
{
    // init RenderObject attributes
    setInline(false); // our object is not Inline
}

RenderTableSection::~RenderTableSection()
{
}

void RenderTableSection::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBox::styleDidChange(diff, oldStyle);
    propagateStyleToAnonymousChildren();

    // If border was changed, notify table.
    RenderTable* table = this->table();
    if (table && !table->selfNeedsLayout() && !table->normalChildNeedsLayout() && oldStyle && oldStyle->border() != style()->border())
        table->invalidateCollapsedBorders();
}

void RenderTableSection::willBeRemovedFromTree()
{
    RenderBox::willBeRemovedFromTree();

    // Preventively invalidate our cells as we may be re-inserted into
    // a new table which would require us to rebuild our structure.
    setNeedsCellRecalc();
}

void RenderTableSection::addChild(RenderObject* child, RenderObject* beforeChild)
{
    if (!child->isTableRow()) {
        RenderObject* last = beforeChild;
        if (!last)
            last = lastChild();
        if (last && last->isAnonymous() && !last->isBeforeOrAfterContent()) {
            if (beforeChild == last)
                beforeChild = last->firstChild();
            last->addChild(child, beforeChild);
            return;
        }

        if (beforeChild && !beforeChild->isAnonymous() && beforeChild->parent() == this) {
            RenderObject* row = beforeChild->previousSibling();
            if (row && row->isTableRow() && row->isAnonymous()) {
                row->addChild(child);
                return;
            }
        }

        // If beforeChild is inside an anonymous cell/row, insert into the cell or into
        // the anonymous row containing it, if there is one.
        RenderObject* lastBox = last;
        while (lastBox && lastBox->parent()->isAnonymous() && !lastBox->isTableRow())
            lastBox = lastBox->parent();
        if (lastBox && lastBox->isAnonymous() && !lastBox->isBeforeOrAfterContent()) {
            lastBox->addChild(child, beforeChild);
            return;
        }

        RenderObject* row = RenderTableRow::createAnonymousWithParentRenderer(this);
        addChild(row, beforeChild);
        row->addChild(child);
        return;
    }

    if (beforeChild)
        setNeedsCellRecalc();

    unsigned insertionRow = m_cRow;
    ++m_cRow;
    m_cCol = 0;

    ensureRows(m_cRow);

    RenderTableRow* row = toRenderTableRow(child);
    m_grid[insertionRow].rowRenderer = row;
    row->setRowIndex(insertionRow);

    if (!beforeChild)
        setRowLogicalHeightToRowStyleLogicalHeightIfNotRelative(m_grid[insertionRow]);

    if (beforeChild && beforeChild->parent() != this)
        beforeChild = splitAnonymousBoxesAroundChild(beforeChild);

    ASSERT(!beforeChild || beforeChild->isTableRow());
    RenderBox::addChild(child, beforeChild);
}

void RenderTableSection::ensureRows(unsigned numRows)
{
    if (numRows <= m_grid.size())
        return;

    unsigned oldSize = m_grid.size();
    m_grid.grow(numRows);

    unsigned effectiveColumnCount = max(1u, table()->numEffCols());
    for (unsigned row = oldSize; row < m_grid.size(); ++row)
        m_grid[row].row.grow(effectiveColumnCount);
}

void RenderTableSection::addCell(RenderTableCell* cell, RenderTableRow* row)
{
    // We don't insert the cell if we need cell recalc as our internal columns' representation
    // will have drifted from the table's representation. Also recalcCells will call addCell
    // at a later time after sync'ing our columns' with the table's.
    if (needsCellRecalc())
        return;

    unsigned rSpan = cell->rowSpan();
    unsigned cSpan = cell->colSpan();
    const Vector<RenderTable::ColumnStruct>& columns = table()->columns();
    unsigned nCols = columns.size();
    unsigned insertionRow = row->rowIndex();

    // ### mozilla still seems to do the old HTML way, even for strict DTD
    // (see the annotation on table cell layouting in the CSS specs and the testcase below:
    // <TABLE border>
    // <TR><TD>1 <TD rowspan="2">2 <TD>3 <TD>4
    // <TR><TD colspan="2">5
    // </TABLE>
    while (m_cCol < nCols && (cellAt(insertionRow, m_cCol).hasCells() || cellAt(insertionRow, m_cCol).inColSpan))
        m_cCol++;

    updateLogicalHeightForCell(m_grid[insertionRow], cell);

    ensureRows(insertionRow + rSpan);

    m_grid[insertionRow].rowRenderer = row;

    unsigned col = m_cCol;
    // tell the cell where it is
    bool inColSpan = false;
    while (cSpan) {
        unsigned currentSpan;
        if (m_cCol >= nCols) {
            table()->appendColumn(cSpan);
            currentSpan = cSpan;
        } else {
            if (cSpan < columns[m_cCol].span)
                table()->splitColumn(m_cCol, cSpan);
            currentSpan = columns[m_cCol].span;
        }
        for (unsigned r = 0; r < rSpan; r++) {
            CellStruct& c = cellAt(insertionRow + r, m_cCol);
            ASSERT(cell);
            c.cells.append(cell);
            // If cells overlap then we take the slow path for painting.
            if (c.cells.size() > 1)
                m_hasMultipleCellLevels = true;
            if (inColSpan)
                c.inColSpan = true;
        }
        m_cCol++;
        cSpan -= currentSpan;
        inColSpan = true;
    }
    cell->setCol(table()->effColToCol(col));
}

int RenderTableSection::calcRowLogicalHeight()
{
#ifndef NDEBUG
    SetLayoutNeededForbiddenScope layoutForbiddenScope(this);
#endif

    ASSERT(!needsLayout());

    RenderTableCell* cell;

    int spacing = table()->vBorderSpacing();
    
    RenderView* viewRenderer = view();
    LayoutStateMaintainer statePusher(viewRenderer);

    m_rowPos.resize(m_grid.size() + 1);
    m_rowPos[0] = spacing;

    unsigned totalRows = m_grid.size();

    for (unsigned r = 0; r < totalRows; r++) {
        m_grid[r].baseline = 0;
        LayoutUnit baselineDescent = 0;

        // Our base size is the biggest logical height from our cells' styles (excluding row spanning cells).
        m_rowPos[r + 1] = max(m_rowPos[r] + minimumValueForLength(m_grid[r].logicalHeight, 0, viewRenderer).round(), 0);

        Row& row = m_grid[r].row;
        unsigned totalCols = row.size();

        for (unsigned c = 0; c < totalCols; c++) {
            CellStruct& current = cellAt(r, c);
            for (unsigned i = 0; i < current.cells.size(); i++) {
                cell = current.cells[i];
                if (current.inColSpan && cell->rowSpan() == 1)
                    continue;

                // FIXME: We are always adding the height of a rowspan to the last rows which doesn't match
                // other browsers. See webkit.org/b/52185 for example.
                if ((cell->rowIndex() + cell->rowSpan() - 1) != r) {
                    // We will apply the height of the rowspan to the current row if next row is not valid.
                    if ((r + 1) < totalRows) {
                        unsigned col = 0;
                        CellStruct nextRowCell = cellAt(r + 1, col);

                        // We are trying to find that next row is valid or not.
                        while (nextRowCell.cells.size() && nextRowCell.cells[0]->rowSpan() > 1 && nextRowCell.cells[0]->rowIndex() < (r + 1)) {
                            col++;
                            if (col < totalCols)
                                nextRowCell = cellAt(r + 1, col);
                            else
                                break;
                        }

                        // We are adding the height of the rowspan to the current row if next row is not valid.
                        if (col < totalCols && nextRowCell.cells.size())
                            continue;
                    }
                }

                // For row spanning cells, |r| is the last row in the span.
                unsigned cellStartRow = cell->rowIndex();

                if (cell->hasOverrideHeight()) {
                    if (!statePusher.didPush()) {
                        // Technically, we should also push state for the row, but since
                        // rows don't push a coordinate transform, that's not necessary.
                        statePusher.push(this, locationOffset());
                    }
                    cell->clearIntrinsicPadding();
                    cell->clearOverrideSize();
                    cell->setChildNeedsLayout(true, MarkOnlyThis);
                    cell->layoutIfNeeded();
                }

                int cellLogicalHeight = cell->logicalHeightForRowSizing();
                m_rowPos[r + 1] = max(m_rowPos[r + 1], m_rowPos[cellStartRow] + cellLogicalHeight);

                // Find out the baseline. The baseline is set on the first row in a rowspan.
                if (cell->isBaselineAligned()) {
                    LayoutUnit baselinePosition = cell->cellBaselinePosition();
                    if (baselinePosition > cell->borderAndPaddingBefore()) {
                        m_grid[cellStartRow].baseline = max(m_grid[cellStartRow].baseline, baselinePosition);
                        // The descent of a cell that spans multiple rows does not affect the height of the first row it spans, so don't let it
                        // become the baseline descent applied to the rest of the row. Also we don't account for the baseline descent of
                        // non-spanning cells when computing a spanning cell's extent.
                        int cellStartRowBaselineDescent = 0;
                        if (cell->rowSpan() == 1) {
                            baselineDescent = max(baselineDescent, cellLogicalHeight - (baselinePosition - cell->intrinsicPaddingBefore()));
                            cellStartRowBaselineDescent = baselineDescent;
                        }
                        m_rowPos[cellStartRow + 1] = max<int>(m_rowPos[cellStartRow + 1], m_rowPos[cellStartRow] + m_grid[cellStartRow].baseline + cellStartRowBaselineDescent);
                    }
                }
            }
        }

        // Add the border-spacing to our final position.
        m_rowPos[r + 1] += m_grid[r].rowRenderer ? spacing : 0;
        m_rowPos[r + 1] = max(m_rowPos[r + 1], m_rowPos[r]);
    }

    ASSERT(!needsLayout());

    statePusher.pop();

    return m_rowPos[m_grid.size()];
}

void RenderTableSection::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;
    ASSERT(needsLayout());
    ASSERT(!needsCellRecalc());
    ASSERT(!table()->needsSectionRecalc());

    // addChild may over-grow m_grid but we don't want to throw away the memory too early as addChild
    // can be called in a loop (e.g during parsing). Doing it now ensures we have a stable-enough structure.
    m_grid.shrinkToFit();

    LayoutStateMaintainer statePusher(view(), this, locationOffset(), hasTransform() || hasReflection() || style()->isFlippedBlocksWritingMode());

    const Vector<int>& columnPos = table()->columnPositions();

    for (unsigned r = 0; r < m_grid.size(); ++r) {
        Row& row = m_grid[r].row;
        unsigned cols = row.size();
        // First, propagate our table layout's information to the cells. This will mark the row as needing layout
        // if there was a column logical width change.
        for (unsigned startColumn = 0; startColumn < cols; ++startColumn) {
            CellStruct& current = row[startColumn];
            RenderTableCell* cell = current.primaryCell();
            if (!cell || current.inColSpan)
                continue;

            unsigned endCol = startColumn;
            unsigned cspan = cell->colSpan();
            while (cspan && endCol < cols) {
                ASSERT(endCol < table()->columns().size());
                cspan -= table()->columns()[endCol].span;
                endCol++;
            }
            int tableLayoutLogicalWidth = columnPos[endCol] - columnPos[startColumn] - table()->hBorderSpacing();
            cell->setCellLogicalWidth(tableLayoutLogicalWidth);
        }

        if (RenderTableRow* rowRenderer = m_grid[r].rowRenderer)
            rowRenderer->layoutIfNeeded();
    }

    statePusher.pop();
    setNeedsLayout(false);
}

void RenderTableSection::distributeExtraLogicalHeightToPercentRows(int& extraLogicalHeight, int totalPercent)
{
    if (!totalPercent)
        return;

    unsigned totalRows = m_grid.size();
    int totalHeight = m_rowPos[totalRows] + extraLogicalHeight;
    int totalLogicalHeightAdded = 0;
    totalPercent = min(totalPercent, 100);
    int rowHeight = m_rowPos[1] - m_rowPos[0];
    for (unsigned r = 0; r < totalRows; ++r) {
        if (totalPercent > 0 && m_grid[r].logicalHeight.isPercent()) {
            int toAdd = min<int>(extraLogicalHeight, (totalHeight * m_grid[r].logicalHeight.percent() / 100) - rowHeight);
            // If toAdd is negative, then we don't want to shrink the row (this bug
            // affected Outlook Web Access).
            toAdd = max(0, toAdd);
            totalLogicalHeightAdded += toAdd;
            extraLogicalHeight -= toAdd;
            totalPercent -= m_grid[r].logicalHeight.percent();
        }
        ASSERT(totalRows >= 1);
        if (r < totalRows - 1)
            rowHeight = m_rowPos[r + 2] - m_rowPos[r + 1];
        m_rowPos[r + 1] += totalLogicalHeightAdded;
    }
}

void RenderTableSection::distributeExtraLogicalHeightToAutoRows(int& extraLogicalHeight, unsigned autoRowsCount)
{
    if (!autoRowsCount)
        return;

    int totalLogicalHeightAdded = 0;
    for (unsigned r = 0; r < m_grid.size(); ++r) {
        if (autoRowsCount > 0 && m_grid[r].logicalHeight.isAuto()) {
            // Recomputing |extraLogicalHeightForRow| guarantees that we properly ditribute round |extraLogicalHeight|.
            int extraLogicalHeightForRow = extraLogicalHeight / autoRowsCount;
            totalLogicalHeightAdded += extraLogicalHeightForRow;
            extraLogicalHeight -= extraLogicalHeightForRow;
            --autoRowsCount;
        }
        m_rowPos[r + 1] += totalLogicalHeightAdded;
    }
}

void RenderTableSection::distributeRemainingExtraLogicalHeight(int& extraLogicalHeight)
{
    unsigned totalRows = m_grid.size();

    if (extraLogicalHeight <= 0 || !m_rowPos[totalRows])
        return;

    // FIXME: m_rowPos[totalRows] - m_rowPos[0] is the total rows' size.
    int totalRowSize = m_rowPos[totalRows];
    int totalLogicalHeightAdded = 0;
    int previousRowPosition = m_rowPos[0];
    for (unsigned r = 0; r < totalRows; r++) {
        // weight with the original height
        totalLogicalHeightAdded += extraLogicalHeight * (m_rowPos[r + 1] - previousRowPosition) / totalRowSize;
        previousRowPosition = m_rowPos[r + 1];
        m_rowPos[r + 1] += totalLogicalHeightAdded;
    }

    extraLogicalHeight -= totalLogicalHeightAdded;
}

int RenderTableSection::distributeExtraLogicalHeightToRows(int extraLogicalHeight)
{
    if (!extraLogicalHeight)
        return extraLogicalHeight;

    unsigned totalRows = m_grid.size();
    if (!totalRows)
        return extraLogicalHeight;

    if (!m_rowPos[totalRows] && nextSibling())
        return extraLogicalHeight;

    unsigned autoRowsCount = 0;
    int totalPercent = 0;
    for (unsigned r = 0; r < totalRows; r++) {
        if (m_grid[r].logicalHeight.isAuto())
            ++autoRowsCount;
        else if (m_grid[r].logicalHeight.isPercent())
            totalPercent += m_grid[r].logicalHeight.percent();
    }

    int remainingExtraLogicalHeight = extraLogicalHeight;
    distributeExtraLogicalHeightToPercentRows(remainingExtraLogicalHeight, totalPercent);
    distributeExtraLogicalHeightToAutoRows(remainingExtraLogicalHeight, autoRowsCount);
    distributeRemainingExtraLogicalHeight(remainingExtraLogicalHeight);
    return extraLogicalHeight - remainingExtraLogicalHeight;
}

void RenderTableSection::layoutRows(int headHeight, int footHeight)
{
#ifndef NDEBUG
    SetLayoutNeededForbiddenScope layoutForbiddenScope(this);
#endif

    ASSERT(!needsLayout());

    unsigned totalRows = m_grid.size();

    // Set the width of our section now.  The rows will also be this width.
    setLogicalWidth(table()->contentLogicalWidth());
    m_forceSlowPaintPathWithOverflowingCell = false;

    int vspacing = table()->vBorderSpacing();
    unsigned nEffCols = table()->numEffCols();

    LayoutStateMaintainer statePusher(view(), this, locationOffset(), hasTransform() || style()->isFlippedBlocksWritingMode());

    WTF::Vector<int> logicalHeightsForPrinting;
    // make sure that rows do not overlap a page break
    if (view()->layoutState()->pageLogicalHeight()) {
        logicalHeightsForPrinting.resize(totalRows);
        int pageOffset = 0;
        for(int r = 0; r < totalRows; ++r) {
            const int childLogicalHeight = m_rowPos[r + 1] - m_rowPos[r] - (m_grid[r].rowRenderer ? vspacing : 0);
            logicalHeightsForPrinting[r] = childLogicalHeight;
            LayoutState* layoutState = view()->layoutState();
            const int pageLogicalHeight = layoutState->m_pageLogicalHeight;
            if (childLogicalHeight < pageLogicalHeight - footHeight) {
                const LayoutSize delta = layoutState->m_layoutOffset - layoutState->m_pageOffset;
                const int logicalOffset = m_rowPos[r] + pageOffset;
                const int offset = isHorizontalWritingMode() ? delta.height() : delta.width();
                const int remainingLogicalHeight = (pageLogicalHeight - (offset + logicalOffset) % pageLogicalHeight) % pageLogicalHeight;
                if (remainingLogicalHeight - footHeight < childLogicalHeight) {
                    pageOffset += remainingLogicalHeight + headHeight;
                }
            }
            m_rowPos[r] += pageOffset;
        }
        m_rowPos[totalRows] += pageOffset;
    }

    for (unsigned r = 0; r < totalRows; r++) {
        // Set the row's x/y position and width/height.
        if (RenderTableRow* rowRenderer = m_grid[r].rowRenderer) {
            rowRenderer->setLocation(LayoutPoint(0, m_rowPos[r]));
            rowRenderer->setLogicalWidth(logicalWidth());
            if (view()->layoutState()->pageLogicalHeight()) {
                rowRenderer->setLogicalHeight(logicalHeightsForPrinting[r]);
            } else {
                rowRenderer->setLogicalHeight(m_rowPos[r + 1] - m_rowPos[r] - vspacing);
            }
            rowRenderer->updateLayerTransform();
        }

        int rowHeightIncreaseForPagination = 0;

        for (unsigned c = 0; c < nEffCols; c++) {
            CellStruct& cs = cellAt(r, c);
            RenderTableCell* cell = cs.primaryCell();

            if (!cell || cs.inColSpan)
                continue;

            int rowIndex = cell->rowIndex();
            int rHeight = 0;
            if (view()->layoutState()->pageLogicalHeight() && cell->rowSpan() == 1) {
                rHeight = logicalHeightsForPrinting[rowIndex];
            } else {
                rHeight = m_rowPos[rowIndex + cell->rowSpan()] - m_rowPos[rowIndex] - vspacing;
            }
            // Force percent height children to lay themselves out again.
            // This will cause these children to grow to fill the cell.
            // FIXME: There is still more work to do here to fully match WinIE (should
            // it become necessary to do so).  In quirks mode, WinIE behaves like we
            // do, but it will clip the cells that spill out of the table section.  In
            // strict mode, Mozilla and WinIE both regrow the table to accommodate the
            // new height of the cell (thus letting the percentages cause growth one
            // time only).  We may also not be handling row-spanning cells correctly.
            //
            // Note also the oddity where replaced elements always flex, and yet blocks/tables do
            // not necessarily flex.  WinIE is crazy and inconsistent, and we can't hope to
            // match the behavior perfectly, but we'll continue to refine it as we discover new
            // bugs. :)
            bool cellChildrenFlex = false;
            bool flexAllChildren = cell->style()->logicalHeight().isFixed()
                || (!table()->style()->logicalHeight().isAuto() && rHeight != cell->logicalHeight());

            for (RenderObject* o = cell->firstChild(); o; o = o->nextSibling()) {
                if (!o->isText() && o->style()->logicalHeight().isPercent() && (flexAllChildren || ((o->isReplaced() || (o->isBox() && toRenderBox(o)->scrollsOverflow())) && !o->isTextControl()))) {
                    // Tables with no sections do not flex.
                    if (!o->isTable() || toRenderTable(o)->hasSections()) {
                        o->setNeedsLayout(true, MarkOnlyThis);
                        cellChildrenFlex = true;
                    }
                }
            }

            if (TrackedRendererListHashSet* percentHeightDescendants = cell->percentHeightDescendants()) {
                TrackedRendererListHashSet::iterator end = percentHeightDescendants->end();
                for (TrackedRendererListHashSet::iterator it = percentHeightDescendants->begin(); it != end; ++it) {
                    RenderBox* box = *it;
                    if (!box->isReplaced() && !box->scrollsOverflow() && !flexAllChildren)
                        continue;

                    while (box != cell) {
                        if (box->normalChildNeedsLayout())
                            break;
                        box->setChildNeedsLayout(true, MarkOnlyThis);
                        box = box->containingBlock();
                        ASSERT(box);
                        if (!box)
                            break;
                    }
                    cellChildrenFlex = true;
                }
            }

            if (cellChildrenFlex) {
                cell->setChildNeedsLayout(true, MarkOnlyThis);
                // Alignment within a cell is based off the calculated
                // height, which becomes irrelevant once the cell has
                // been resized based off its percentage.
                cell->setOverrideLogicalContentHeightFromRowHeight(rHeight);
                cell->layoutIfNeeded();

                // If the baseline moved, we may have to update the data for our row. Find out the new baseline.
                if (cell->isBaselineAligned()) {
                    LayoutUnit baseline = cell->cellBaselinePosition();
                    if (baseline > cell->borderAndPaddingBefore())
                        m_grid[r].baseline = max(m_grid[r].baseline, baseline);
                }
            }

            cell->computeIntrinsicPadding(rHeight);

            LayoutRect oldCellRect = cell->frameRect();

            setLogicalPositionForCell(cell, c);

            if (!cell->needsLayout() && view()->layoutState()->pageLogicalHeight() && view()->layoutState()->pageLogicalOffset(cell, cell->logicalTop()) != cell->pageLogicalOffset())
                cell->setChildNeedsLayout(true, MarkOnlyThis);

            cell->layoutIfNeeded();

            // FIXME: Make pagination work with vertical tables.
            if (view()->layoutState()->pageLogicalHeight() && cell->logicalHeight() != rHeight) {
                // FIXME: Pagination might have made us change size. For now just shrink or grow the cell to fit without doing a relayout.
                // We'll also do a basic increase of the row height to accommodate the cell if it's bigger, but this isn't quite right
                // either. It's at least stable though and won't result in an infinite # of relayouts that may never stabilize.
                if (cell->logicalHeight() > rHeight)
                    rowHeightIncreaseForPagination = max<int>(rowHeightIncreaseForPagination, cell->logicalHeight() - rHeight);
                cell->setLogicalHeight(rHeight);
            }

            LayoutSize childOffset(cell->location() - oldCellRect.location());
            if (childOffset.width() || childOffset.height()) {
                view()->addLayoutDelta(childOffset);

                // If the child moved, we have to repaint it as well as any floating/positioned
                // descendants.  An exception is if we need a layout.  In this case, we know we're going to
                // repaint ourselves (and the child) anyway.
                if (!table()->selfNeedsLayout() && cell->checkForRepaintDuringLayout())
                    cell->repaintDuringLayoutIfMoved(oldCellRect);
            }
        }
        if (rowHeightIncreaseForPagination) {
            for (unsigned rowIndex = r + 1; rowIndex <= totalRows; rowIndex++)
                m_rowPos[rowIndex] += rowHeightIncreaseForPagination;
            for (unsigned c = 0; c < nEffCols; ++c) {
                Vector<RenderTableCell*, 1>& cells = cellAt(r, c).cells;
                for (size_t i = 0; i < cells.size(); ++i)
                    cells[i]->setLogicalHeight(cells[i]->logicalHeight() + rowHeightIncreaseForPagination);
            }
        }
    }

    ASSERT(!needsLayout());

    setLogicalHeight(m_rowPos[totalRows]);

    computeOverflowFromCells(totalRows, nEffCols);

    statePusher.pop();
}

void RenderTableSection::computeOverflowFromCells()
{
    unsigned totalRows = m_grid.size();
    unsigned nEffCols = table()->numEffCols();
    computeOverflowFromCells(totalRows, nEffCols);
}

void RenderTableSection::computeOverflowFromCells(unsigned totalRows, unsigned nEffCols)
{
    m_overflow.clear();
    m_overflowingCells.clear();
    unsigned totalCellsCount = nEffCols * totalRows;
    int maxAllowedOverflowingCellsCount = totalCellsCount < gMinTableSizeToUseFastPaintPathWithOverflowingCell ? 0 : gMaxAllowedOverflowingCellRatioForFastPaintPath * totalCellsCount;

#ifndef NDEBUG
    bool hasOverflowingCell = false;
#endif
    // Now that our height has been determined, add in overflow from cells.
    for (unsigned r = 0; r < totalRows; r++) {
        for (unsigned c = 0; c < nEffCols; c++) {
            CellStruct& cs = cellAt(r, c);
            RenderTableCell* cell = cs.primaryCell();
            if (!cell || cs.inColSpan)
                continue;
            if (r < totalRows - 1 && cell == primaryCellAt(r + 1, c))
                continue;
            addOverflowFromChild(cell);
#ifndef NDEBUG
            hasOverflowingCell |= cell->hasVisualOverflow();
#endif
            if (cell->hasVisualOverflow() && !m_forceSlowPaintPathWithOverflowingCell) {
                m_overflowingCells.add(cell);
                if (m_overflowingCells.size() > maxAllowedOverflowingCellsCount) {
                    // We need to set m_forcesSlowPaintPath only if there is a least one overflowing cells as the hit testing code rely on this information.
                    m_forceSlowPaintPathWithOverflowingCell = true;
                    // The slow path does not make any use of the overflowing cells info, don't hold on to the memory.
                    m_overflowingCells.clear();
                }
            }
        }
    }

    ASSERT(hasOverflowingCell == this->hasOverflowingCell());
}

int RenderTableSection::calcOuterBorderBefore() const
{
    unsigned totalCols = table()->numEffCols();
    if (!m_grid.size() || !totalCols)
        return 0;

    unsigned borderWidth = 0;

    const BorderValue& sb = style()->borderBefore();
    if (sb.style() == BHIDDEN)
        return -1;
    if (sb.style() > BHIDDEN)
        borderWidth = sb.width();

    const BorderValue& rb = firstChild()->style()->borderBefore();
    if (rb.style() == BHIDDEN)
        return -1;
    if (rb.style() > BHIDDEN && rb.width() > borderWidth)
        borderWidth = rb.width();

    bool allHidden = true;
    for (unsigned c = 0; c < totalCols; c++) {
        const CellStruct& current = cellAt(0, c);
        if (current.inColSpan || !current.hasCells())
            continue;
        const BorderValue& cb = current.primaryCell()->style()->borderBefore(); // FIXME: Make this work with perpendicular and flipped cells.
        // FIXME: Don't repeat for the same col group
        RenderTableCol* colGroup = table()->colElement(c);
        if (colGroup) {
            const BorderValue& gb = colGroup->style()->borderBefore();
            if (gb.style() == BHIDDEN || cb.style() == BHIDDEN)
                continue;
            allHidden = false;
            if (gb.style() > BHIDDEN && gb.width() > borderWidth)
                borderWidth = gb.width();
            if (cb.style() > BHIDDEN && cb.width() > borderWidth)
                borderWidth = cb.width();
        } else {
            if (cb.style() == BHIDDEN)
                continue;
            allHidden = false;
            if (cb.style() > BHIDDEN && cb.width() > borderWidth)
                borderWidth = cb.width();
        }
    }
    if (allHidden)
        return -1;

    return borderWidth / 2;
}

int RenderTableSection::calcOuterBorderAfter() const
{
    unsigned totalCols = table()->numEffCols();
    if (!m_grid.size() || !totalCols)
        return 0;

    unsigned borderWidth = 0;

    const BorderValue& sb = style()->borderAfter();
    if (sb.style() == BHIDDEN)
        return -1;
    if (sb.style() > BHIDDEN)
        borderWidth = sb.width();

    const BorderValue& rb = lastChild()->style()->borderAfter();
    if (rb.style() == BHIDDEN)
        return -1;
    if (rb.style() > BHIDDEN && rb.width() > borderWidth)
        borderWidth = rb.width();

    bool allHidden = true;
    for (unsigned c = 0; c < totalCols; c++) {
        const CellStruct& current = cellAt(m_grid.size() - 1, c);
        if (current.inColSpan || !current.hasCells())
            continue;
        const BorderValue& cb = current.primaryCell()->style()->borderAfter(); // FIXME: Make this work with perpendicular and flipped cells.
        // FIXME: Don't repeat for the same col group
        RenderTableCol* colGroup = table()->colElement(c);
        if (colGroup) {
            const BorderValue& gb = colGroup->style()->borderAfter();
            if (gb.style() == BHIDDEN || cb.style() == BHIDDEN)
                continue;
            allHidden = false;
            if (gb.style() > BHIDDEN && gb.width() > borderWidth)
                borderWidth = gb.width();
            if (cb.style() > BHIDDEN && cb.width() > borderWidth)
                borderWidth = cb.width();
        } else {
            if (cb.style() == BHIDDEN)
                continue;
            allHidden = false;
            if (cb.style() > BHIDDEN && cb.width() > borderWidth)
                borderWidth = cb.width();
        }
    }
    if (allHidden)
        return -1;

    return (borderWidth + 1) / 2;
}

int RenderTableSection::calcOuterBorderStart() const
{
    unsigned totalCols = table()->numEffCols();
    if (!m_grid.size() || !totalCols)
        return 0;

    unsigned borderWidth = 0;

    const BorderValue& sb = style()->borderStart();
    if (sb.style() == BHIDDEN)
        return -1;
    if (sb.style() > BHIDDEN)
        borderWidth = sb.width();

    if (RenderTableCol* colGroup = table()->colElement(0)) {
        const BorderValue& gb = colGroup->style()->borderStart();
        if (gb.style() == BHIDDEN)
            return -1;
        if (gb.style() > BHIDDEN && gb.width() > borderWidth)
            borderWidth = gb.width();
    }

    bool allHidden = true;
    for (unsigned r = 0; r < m_grid.size(); r++) {
        const CellStruct& current = cellAt(r, 0);
        if (!current.hasCells())
            continue;
        // FIXME: Don't repeat for the same cell
        const BorderValue& cb = current.primaryCell()->style()->borderStart(); // FIXME: Make this work with perpendicular and flipped cells.
        const BorderValue& rb = current.primaryCell()->parent()->style()->borderStart();
        if (cb.style() == BHIDDEN || rb.style() == BHIDDEN)
            continue;
        allHidden = false;
        if (cb.style() > BHIDDEN && cb.width() > borderWidth)
            borderWidth = cb.width();
        if (rb.style() > BHIDDEN && rb.width() > borderWidth)
            borderWidth = rb.width();
    }
    if (allHidden)
        return -1;

    return (borderWidth + (table()->style()->isLeftToRightDirection() ? 0 : 1)) / 2;
}

int RenderTableSection::calcOuterBorderEnd() const
{
    unsigned totalCols = table()->numEffCols();
    if (!m_grid.size() || !totalCols)
        return 0;

    unsigned borderWidth = 0;

    const BorderValue& sb = style()->borderEnd();
    if (sb.style() == BHIDDEN)
        return -1;
    if (sb.style() > BHIDDEN)
        borderWidth = sb.width();

    if (RenderTableCol* colGroup = table()->colElement(totalCols - 1)) {
        const BorderValue& gb = colGroup->style()->borderEnd();
        if (gb.style() == BHIDDEN)
            return -1;
        if (gb.style() > BHIDDEN && gb.width() > borderWidth)
            borderWidth = gb.width();
    }

    bool allHidden = true;
    for (unsigned r = 0; r < m_grid.size(); r++) {
        const CellStruct& current = cellAt(r, totalCols - 1);
        if (!current.hasCells())
            continue;
        // FIXME: Don't repeat for the same cell
        const BorderValue& cb = current.primaryCell()->style()->borderEnd(); // FIXME: Make this work with perpendicular and flipped cells.
        const BorderValue& rb = current.primaryCell()->parent()->style()->borderEnd();
        if (cb.style() == BHIDDEN || rb.style() == BHIDDEN)
            continue;
        allHidden = false;
        if (cb.style() > BHIDDEN && cb.width() > borderWidth)
            borderWidth = cb.width();
        if (rb.style() > BHIDDEN && rb.width() > borderWidth)
            borderWidth = rb.width();
    }
    if (allHidden)
        return -1;

    return (borderWidth + (table()->style()->isLeftToRightDirection() ? 1 : 0)) / 2;
}

void RenderTableSection::recalcOuterBorder()
{
    m_outerBorderBefore = calcOuterBorderBefore();
    m_outerBorderAfter = calcOuterBorderAfter();
    m_outerBorderStart = calcOuterBorderStart();
    m_outerBorderEnd = calcOuterBorderEnd();
}

int RenderTableSection::firstLineBoxBaseline() const
{
    if (!m_grid.size())
        return -1;

    int firstLineBaseline = m_grid[0].baseline;
    if (firstLineBaseline)
        return firstLineBaseline + m_rowPos[0];

    firstLineBaseline = -1;
    const Row& firstRow = m_grid[0].row;
    for (size_t i = 0; i < firstRow.size(); ++i) {
        const CellStruct& cs = firstRow.at(i);
        const RenderTableCell* cell = cs.primaryCell();
        // Only cells with content have a baseline
        if (cell && cell->contentLogicalHeight())
            firstLineBaseline = max<int>(firstLineBaseline, cell->logicalTop() + cell->borderAndPaddingBefore() + cell->contentLogicalHeight());
    }

    return firstLineBaseline;
}

void RenderTableSection::paint(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    // put this back in when all layout tests can handle it
    // ASSERT(!needsLayout());
    // avoid crashing on bugs that cause us to paint with dirty layout
    if (needsLayout())
        return;
    
    unsigned totalRows = m_grid.size();
    unsigned totalCols = table()->columns().size();

    if (!totalRows || !totalCols)
        return;

    LayoutPoint adjustedPaintOffset = paintOffset + location();

    PaintPhase phase = paintInfo.phase;
    bool pushedClip = pushContentsClip(paintInfo, adjustedPaintOffset);
    paintObject(paintInfo, adjustedPaintOffset);
    if (pushedClip)
        popContentsClip(paintInfo, phase, adjustedPaintOffset);

    if ((phase == PaintPhaseOutline || phase == PaintPhaseSelfOutline) && style()->visibility() == VISIBLE)
        paintOutline(paintInfo, LayoutRect(adjustedPaintOffset, size()));
}

static inline bool compareCellPositions(RenderTableCell* elem1, RenderTableCell* elem2)
{
    return elem1->rowIndex() < elem2->rowIndex();
}

// This comparison is used only when we have overflowing cells as we have an unsorted array to sort. We thus need
// to sort both on rows and columns to properly repaint.
static inline bool compareCellPositionsWithOverflowingCells(RenderTableCell* elem1, RenderTableCell* elem2)
{
    if (elem1->rowIndex() != elem2->rowIndex())
        return elem1->rowIndex() < elem2->rowIndex();

    return elem1->col() < elem2->col();
}

void RenderTableSection::paintCell(RenderTableCell* cell, PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    LayoutPoint cellPoint = flipForWritingModeForChild(cell, paintOffset);
    PaintPhase paintPhase = paintInfo.phase;
    RenderTableRow* row = toRenderTableRow(cell->parent());

    if (paintPhase == PaintPhaseBlockBackground || paintPhase == PaintPhaseChildBlockBackground) {
        // We need to handle painting a stack of backgrounds.  This stack (from bottom to top) consists of
        // the column group, column, row group, row, and then the cell.
        RenderTableCol* column = table()->colElement(cell->col());
        RenderTableCol* columnGroup = column ? column->enclosingColumnGroup() : 0;

        // Column groups and columns first.
        // FIXME: Columns and column groups do not currently support opacity, and they are being painted "too late" in
        // the stack, since we have already opened a transparency layer (potentially) for the table row group.
        // Note that we deliberately ignore whether or not the cell has a layer, since these backgrounds paint "behind" the
        // cell.
        cell->paintBackgroundsBehindCell(paintInfo, cellPoint, columnGroup);
        cell->paintBackgroundsBehindCell(paintInfo, cellPoint, column);

        // Paint the row group next.
        cell->paintBackgroundsBehindCell(paintInfo, cellPoint, this);

        // Paint the row next, but only if it doesn't have a layer.  If a row has a layer, it will be responsible for
        // painting the row background for the cell.
        if (!row->hasSelfPaintingLayer())
            cell->paintBackgroundsBehindCell(paintInfo, cellPoint, row);
    }
    if ((!cell->hasSelfPaintingLayer() && !row->hasSelfPaintingLayer()))
        cell->paint(paintInfo, cellPoint);
}

LayoutRect RenderTableSection::logicalRectForWritingModeAndDirection(const LayoutRect& rect) const
{
    LayoutRect tableAlignedRect(rect);

    flipForWritingMode(tableAlignedRect);

    if (!style()->isHorizontalWritingMode())
        tableAlignedRect = tableAlignedRect.transposedRect();

    const Vector<int>& columnPos = table()->columnPositions();
    // FIXME: The table's direction should determine our row's direction, not the section's (see bug 96691).
    if (!style()->isLeftToRightDirection())
        tableAlignedRect.setX(columnPos[columnPos.size() - 1] - tableAlignedRect.maxX());

    return tableAlignedRect;
}

CellSpan RenderTableSection::dirtiedRows(const LayoutRect& damageRect) const
{
    if (m_forceSlowPaintPathWithOverflowingCell) 
        return fullTableRowSpan();

    CellSpan coveredRows = spannedRows(damageRect);

    // To repaint the border we might need to repaint first or last row even if they are not spanned themselves.
    if (coveredRows.start() >= m_rowPos.size() - 1 && m_rowPos[m_rowPos.size() - 1] + table()->outerBorderAfter() >= damageRect.y())
        --coveredRows.start();

    if (!coveredRows.end() && m_rowPos[0] - table()->outerBorderBefore() <= damageRect.maxY())
        ++coveredRows.end();

    return coveredRows;
}

CellSpan RenderTableSection::dirtiedColumns(const LayoutRect& damageRect) const
{
    if (m_forceSlowPaintPathWithOverflowingCell) 
        return fullTableColumnSpan();

    CellSpan coveredColumns = spannedColumns(damageRect);

    const Vector<int>& columnPos = table()->columnPositions();
    // To repaint the border we might need to repaint first or last column even if they are not spanned themselves.
    if (coveredColumns.start() >= columnPos.size() - 1 && columnPos[columnPos.size() - 1] + table()->outerBorderEnd() >= damageRect.x())
        --coveredColumns.start();

    if (!coveredColumns.end() && columnPos[0] - table()->outerBorderStart() <= damageRect.maxX())
        ++coveredColumns.end();

    return coveredColumns;
}

CellSpan RenderTableSection::spannedRows(const LayoutRect& flippedRect) const
{
    // Find the first row that starts after rect top.
    unsigned nextRow = std::upper_bound(m_rowPos.begin(), m_rowPos.end(), flippedRect.y()) - m_rowPos.begin();

    if (nextRow == m_rowPos.size())
        return CellSpan(m_rowPos.size() - 1, m_rowPos.size() - 1); // After all rows.

    unsigned startRow = nextRow > 0 ? nextRow - 1 : 0;

    // Find the first row that starts after rect bottom.
    unsigned endRow;
    if (m_rowPos[nextRow] >= flippedRect.maxY())
        endRow = nextRow;
    else {
        endRow = std::upper_bound(m_rowPos.begin() + nextRow, m_rowPos.end(), flippedRect.maxY()) - m_rowPos.begin();
        if (endRow == m_rowPos.size())
            endRow = m_rowPos.size() - 1;
    }

    return CellSpan(startRow, endRow);
}

CellSpan RenderTableSection::spannedColumns(const LayoutRect& flippedRect) const
{
    const Vector<int>& columnPos = table()->columnPositions();

    // Find the first column that starts after rect left.
    // lower_bound doesn't handle the edge between two cells properly as it would wrongly return the
    // cell on the logical top/left.
    // upper_bound on the other hand properly returns the cell on the logical bottom/right, which also
    // matches the behavior of other browsers.
    unsigned nextColumn = std::upper_bound(columnPos.begin(), columnPos.end(), flippedRect.x()) - columnPos.begin();

    if (nextColumn == columnPos.size())
        return CellSpan(columnPos.size() - 1, columnPos.size() - 1); // After all columns.

    unsigned startColumn = nextColumn > 0 ? nextColumn - 1 : 0;

    // Find the first column that starts after rect right.
    unsigned endColumn;
    if (columnPos[nextColumn] >= flippedRect.maxX())
        endColumn = nextColumn;
    else {
        endColumn = std::upper_bound(columnPos.begin() + nextColumn, columnPos.end(), flippedRect.maxX()) - columnPos.begin();
        if (endColumn == columnPos.size())
            endColumn = columnPos.size() - 1;
    }

    return CellSpan(startColumn, endColumn);
}


void RenderTableSection::paintObject(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    PaintPhase paintPhase = paintInfo.phase;

    LayoutRect localRepaintRect = paintInfo.rect;
    localRepaintRect.moveBy(-paintOffset);
    localRepaintRect.inflate(maximalOutlineSize(paintPhase));

    LayoutRect tableAlignedRect = logicalRectForWritingModeAndDirection(localRepaintRect);

    CellSpan dirtiedRows = this->dirtiedRows(tableAlignedRect);
    CellSpan dirtiedColumns = this->dirtiedColumns(tableAlignedRect);

    if (dirtiedColumns.start() < dirtiedColumns.end()) {
        if (!m_hasMultipleCellLevels && !m_overflowingCells.size()) {
            if (paintInfo.phase == PaintPhaseCollapsedTableBorders) {
                // Collapsed borders are painted from the bottom right to the top left so that precedence
                // due to cell position is respected. We need to paint one row beyond the topmost dirtied
                // row to calculate its collapsed border value.
                unsigned startRow = dirtiedRows.start() ? dirtiedRows.start() - 1 : 0;
                for (unsigned r = dirtiedRows.end(); r > startRow; r--) {
                    unsigned row = r - 1;
                    for (unsigned c = dirtiedColumns.end(); c > dirtiedColumns.start(); c--) {
                        unsigned col = c - 1;
                        CellStruct& current = cellAt(row, col);
                        RenderTableCell* cell = current.primaryCell();
                        if (!cell || (row > dirtiedRows.start() && primaryCellAt(row - 1, col) == cell) || (col > dirtiedColumns.start() && primaryCellAt(row, col - 1) == cell))
                            continue;
                        LayoutPoint cellPoint = flipForWritingModeForChild(cell, paintOffset);
                        cell->paintCollapsedBorders(paintInfo, cellPoint);
                    }
                }
            } else {
                // Draw the dirty cells in the order that they appear.
                for (unsigned r = dirtiedRows.start(); r < dirtiedRows.end(); r++) {
                    RenderTableRow* row = m_grid[r].rowRenderer;
                    if (row && !row->hasSelfPaintingLayer())
                        row->paintOutlineForRowIfNeeded(paintInfo, paintOffset);
                    for (unsigned c = dirtiedColumns.start(); c < dirtiedColumns.end(); c++) {
                        CellStruct& current = cellAt(r, c);
                        RenderTableCell* cell = current.primaryCell();
                        if (!cell || (r > dirtiedRows.start() && primaryCellAt(r - 1, c) == cell) || (c > dirtiedColumns.start() && primaryCellAt(r, c - 1) == cell))
                            continue;
                        paintCell(cell, paintInfo, paintOffset);
                    }
                }
            }
        } else {
            // The overflowing cells should be scarce to avoid adding a lot of cells to the HashSet.
#ifndef NDEBUG
            unsigned totalRows = m_grid.size();
            unsigned totalCols = table()->columns().size();
            ASSERT(m_overflowingCells.size() < totalRows * totalCols * gMaxAllowedOverflowingCellRatioForFastPaintPath);
#endif

            // To make sure we properly repaint the section, we repaint all the overflowing cells that we collected.
            Vector<RenderTableCell*> cells;
            copyToVector(m_overflowingCells, cells);

            HashSet<RenderTableCell*> spanningCells;

            for (unsigned r = dirtiedRows.start(); r < dirtiedRows.end(); r++) {
                RenderTableRow* row = m_grid[r].rowRenderer;
                if (row && !row->hasSelfPaintingLayer())
                    row->paintOutlineForRowIfNeeded(paintInfo, paintOffset);
                for (unsigned c = dirtiedColumns.start(); c < dirtiedColumns.end(); c++) {
                    CellStruct& current = cellAt(r, c);
                    if (!current.hasCells())
                        continue;
                    for (unsigned i = 0; i < current.cells.size(); ++i) {
                        if (m_overflowingCells.contains(current.cells[i]))
                            continue;

                        if (current.cells[i]->rowSpan() > 1 || current.cells[i]->colSpan() > 1) {
                            if (!spanningCells.add(current.cells[i]).isNewEntry)
                                continue;
                        }

                        cells.append(current.cells[i]);
                    }
                }
            }

            // Sort the dirty cells by paint order.
            if (!m_overflowingCells.size())
                std::stable_sort(cells.begin(), cells.end(), compareCellPositions);
            else
                std::sort(cells.begin(), cells.end(), compareCellPositionsWithOverflowingCells);

            if (paintInfo.phase == PaintPhaseCollapsedTableBorders) {
                for (unsigned i = cells.size(); i > 0; --i) {
                    LayoutPoint cellPoint = flipForWritingModeForChild(cells[i - 1], paintOffset);
                    cells[i - 1]->paintCollapsedBorders(paintInfo, cellPoint);
                }
            } else {
                for (unsigned i = 0; i < cells.size(); ++i)
                    paintCell(cells[i], paintInfo, paintOffset);
            }
        }
    }
}

void RenderTableSection::imageChanged(WrappedImagePtr, const IntRect*)
{
    // FIXME: Examine cells and repaint only the rect the image paints in.
    repaint();
}

void RenderTableSection::recalcCells()
{
    ASSERT(m_needsCellRecalc);
    // We reset the flag here to ensure that |addCell| works. This is safe to do as
    // fillRowsWithDefaultStartingAtPosition makes sure we match the table's columns
    // representation.
    m_needsCellRecalc = false;

    m_cCol = 0;
    m_cRow = 0;
    m_grid.clear();

    for (RenderObject* row = firstChild(); row; row = row->nextSibling()) {
        if (row->isTableRow()) {
            unsigned insertionRow = m_cRow;
            m_cRow++;
            m_cCol = 0;
            ensureRows(m_cRow);

            RenderTableRow* tableRow = toRenderTableRow(row);
            m_grid[insertionRow].rowRenderer = tableRow;
            tableRow->setRowIndex(insertionRow);
            setRowLogicalHeightToRowStyleLogicalHeightIfNotRelative(m_grid[insertionRow]);

            for (RenderObject* cell = row->firstChild(); cell; cell = cell->nextSibling()) {
                if (!cell->isTableCell())
                    continue;

                RenderTableCell* tableCell = toRenderTableCell(cell);
                addCell(tableCell, tableRow);
            }
        }
    }

    m_grid.shrinkToFit();
    setNeedsLayout(true);
}

// FIXME: This function could be made O(1) in certain cases (like for the non-most-constrainive cells' case).
void RenderTableSection::rowLogicalHeightChanged(unsigned rowIndex)
{
    if (needsCellRecalc())
        return;

    setRowLogicalHeightToRowStyleLogicalHeightIfNotRelative(m_grid[rowIndex]);

    for (RenderObject* cell = m_grid[rowIndex].rowRenderer->firstChild(); cell; cell = cell->nextSibling()) {
        if (!cell->isTableCell())
            continue;

        updateLogicalHeightForCell(m_grid[rowIndex], toRenderTableCell(cell));
    }
}

void RenderTableSection::setNeedsCellRecalc()
{
    m_needsCellRecalc = true;
    if (RenderTable* t = table())
        t->setNeedsSectionRecalc();
}

unsigned RenderTableSection::numColumns() const
{
    unsigned result = 0;
    
    for (unsigned r = 0; r < m_grid.size(); ++r) {
        for (unsigned c = result; c < table()->numEffCols(); ++c) {
            const CellStruct& cell = cellAt(r, c);
            if (cell.hasCells() || cell.inColSpan)
                result = c;
        }
    }
    
    return result + 1;
}

const BorderValue& RenderTableSection::borderAdjoiningStartCell(const RenderTableCell* cell) const
{
    ASSERT(cell->isFirstOrLastCellInRow());
    return hasSameDirectionAs(cell) ? style()->borderStart() : style()->borderEnd();
}

const BorderValue& RenderTableSection::borderAdjoiningEndCell(const RenderTableCell* cell) const
{
    ASSERT(cell->isFirstOrLastCellInRow());
    return hasSameDirectionAs(cell) ? style()->borderEnd() : style()->borderStart();
}

const RenderTableCell* RenderTableSection::firstRowCellAdjoiningTableStart() const
{
    unsigned adjoiningStartCellColumnIndex = hasSameDirectionAs(table()) ? 0 : table()->lastColumnIndex();
    return cellAt(0, adjoiningStartCellColumnIndex).primaryCell();
}

const RenderTableCell* RenderTableSection::firstRowCellAdjoiningTableEnd() const
{
    unsigned adjoiningEndCellColumnIndex = hasSameDirectionAs(table()) ? table()->lastColumnIndex() : 0;
    return cellAt(0, adjoiningEndCellColumnIndex).primaryCell();
}

void RenderTableSection::appendColumn(unsigned pos)
{
    ASSERT(!m_needsCellRecalc);

    for (unsigned row = 0; row < m_grid.size(); ++row)
        m_grid[row].row.resize(pos + 1);
}

void RenderTableSection::splitColumn(unsigned pos, unsigned first)
{
    ASSERT(!m_needsCellRecalc);

    if (m_cCol > pos)
        m_cCol++;
    for (unsigned row = 0; row < m_grid.size(); ++row) {
        Row& r = m_grid[row].row;
        r.insert(pos + 1, CellStruct());
        if (r[pos].hasCells()) {
            r[pos + 1].cells.appendVector(r[pos].cells);
            RenderTableCell* cell = r[pos].primaryCell();
            ASSERT(cell);
            ASSERT(cell->colSpan() >= (r[pos].inColSpan ? 1u : 0));
            unsigned colleft = cell->colSpan() - r[pos].inColSpan;
            if (first > colleft)
              r[pos + 1].inColSpan = 0;
            else
              r[pos + 1].inColSpan = first + r[pos].inColSpan;
        } else {
            r[pos + 1].inColSpan = 0;
        }
    }
}

// Hit Testing
bool RenderTableSection::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction action)
{
    // If we have no children then we have nothing to do.
    if (!firstChild())
        return false;

    // Table sections cannot ever be hit tested.  Effectively they do not exist.
    // Just forward to our children always.
    LayoutPoint adjustedLocation = accumulatedOffset + location();

    if (hasOverflowClip() && !locationInContainer.intersects(overflowClipRect(adjustedLocation, locationInContainer.region())))
        return false;

    if (hasOverflowingCell()) {
        for (RenderObject* child = lastChild(); child; child = child->previousSibling()) {
            // FIXME: We have to skip over inline flows, since they can show up inside table rows
            // at the moment (a demoted inline <form> for example). If we ever implement a
            // table-specific hit-test method (which we should do for performance reasons anyway),
            // then we can remove this check.
            if (child->isBox() && !toRenderBox(child)->hasSelfPaintingLayer()) {
                LayoutPoint childPoint = flipForWritingModeForChild(toRenderBox(child), adjustedLocation);
                if (child->nodeAtPoint(request, result, locationInContainer, childPoint, action)) {
                    updateHitTestResult(result, toLayoutPoint(locationInContainer.point() - childPoint));
                    return true;
                }
            }
        }
        return false;
    }

    recalcCellsIfNeeded();

    LayoutRect hitTestRect = locationInContainer.boundingBox();
    hitTestRect.moveBy(-adjustedLocation);

    LayoutRect tableAlignedRect = logicalRectForWritingModeAndDirection(hitTestRect);
    CellSpan rowSpan = spannedRows(tableAlignedRect);
    CellSpan columnSpan = spannedColumns(tableAlignedRect);

    // Now iterate over the spanned rows and columns.
    for (unsigned hitRow = rowSpan.start(); hitRow < rowSpan.end(); ++hitRow) {
        for (unsigned hitColumn = columnSpan.start(); hitColumn < columnSpan.end(); ++hitColumn) {
            CellStruct& current = cellAt(hitRow, hitColumn);

            // If the cell is empty, there's nothing to do
            if (!current.hasCells())
                continue;

            for (unsigned i = current.cells.size() ; i; ) {
                --i;
                RenderTableCell* cell = current.cells[i];
                LayoutPoint cellPoint = flipForWritingModeForChild(cell, adjustedLocation);
                if (static_cast<RenderObject*>(cell)->nodeAtPoint(request, result, locationInContainer, cellPoint, action)) {
                    updateHitTestResult(result, locationInContainer.point() - toLayoutSize(cellPoint));
                    return true;
                }
            }
            if (!result.isRectBasedTest())
                break;
        }
        if (!result.isRectBasedTest())
            break;
    }

    return false;
}

void RenderTableSection::removeCachedCollapsedBorders(const RenderTableCell* cell)
{
    if (!table()->collapseBorders())
        return;
    
    for (int side = CBSBefore; side <= CBSEnd; ++side)
        m_cellsCollapsedBorders.remove(make_pair(cell, side));
}

void RenderTableSection::setCachedCollapsedBorder(const RenderTableCell* cell, CollapsedBorderSide side, CollapsedBorderValue border)
{
    ASSERT(table()->collapseBorders());
    m_cellsCollapsedBorders.set(make_pair(cell, side), border);
}

CollapsedBorderValue& RenderTableSection::cachedCollapsedBorder(const RenderTableCell* cell, CollapsedBorderSide side)
{
    ASSERT(table()->collapseBorders());
    HashMap<pair<const RenderTableCell*, int>, CollapsedBorderValue>::iterator it = m_cellsCollapsedBorders.find(make_pair(cell, side));
    ASSERT(it != m_cellsCollapsedBorders.end());
    return it->value;
}

RenderTableSection* RenderTableSection::createAnonymousWithParentRenderer(const RenderObject* parent)
{
    RefPtr<RenderStyle> newStyle = RenderStyle::createAnonymousStyleWithDisplay(parent->style(), TABLE_ROW_GROUP);
    RenderTableSection* newSection = new (parent->renderArena()) RenderTableSection(0);
    newSection->setDocumentForAnonymous(parent->document());
    newSection->setStyle(newStyle.release());
    return newSection;
}

void RenderTableSection::setLogicalPositionForCell(RenderTableCell* cell, unsigned effectiveColumn) const
{
    LayoutPoint oldCellLocation = cell->location();

    LayoutPoint cellLocation(0, m_rowPos[cell->rowIndex()]);
    int horizontalBorderSpacing = table()->hBorderSpacing();

    // FIXME: The table's direction should determine our row's direction, not the section's (see bug 96691).
    if (!style()->isLeftToRightDirection())
        cellLocation.setX(table()->columnPositions()[table()->numEffCols()] - table()->columnPositions()[table()->colToEffCol(cell->col() + cell->colSpan())] + horizontalBorderSpacing);
    else
        cellLocation.setX(table()->columnPositions()[effectiveColumn] + horizontalBorderSpacing);

    cell->setLogicalLocation(cellLocation);
    view()->addLayoutDelta(oldCellLocation - cell->location());
}

} // namespace WebCore
