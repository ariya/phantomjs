/*
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
#include "RenderTable.h"

#include "AutoTableLayout.h"
#include "CollapsedBorderValue.h"
#include "DeleteButtonController.h"
#include "Document.h"
#include "FixedTableLayout.h"
#include "FrameView.h"
#include "HitTestResult.h"
#include "HTMLNames.h"
#include "RenderLayer.h"
#include "RenderTableCell.h"
#include "RenderTableCol.h"
#include "RenderTableSection.h"
#include "RenderView.h"

using namespace std;

namespace WebCore {

using namespace HTMLNames;

RenderTable::RenderTable(Node* node)
    : RenderBlock(node)
    , m_caption(0)
    , m_head(0)
    , m_foot(0)
    , m_firstBody(0)
    , m_currentBorder(0)
    , m_hasColElements(false)
    , m_needsSectionRecalc(0)
    , m_hSpacing(0)
    , m_vSpacing(0)
    , m_borderStart(0)
    , m_borderEnd(0)
{
    setChildrenInline(false);
    m_columnPos.fill(0, 2);
    m_columns.fill(ColumnStruct(), 1);
    
}

RenderTable::~RenderTable()
{
}

void RenderTable::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBlock::styleDidChange(diff, oldStyle);
    propagateStyleToAnonymousChildren();

    ETableLayout oldTableLayout = oldStyle ? oldStyle->tableLayout() : TAUTO;

    // In the collapsed border model, there is no cell spacing.
    m_hSpacing = collapseBorders() ? 0 : style()->horizontalBorderSpacing();
    m_vSpacing = collapseBorders() ? 0 : style()->verticalBorderSpacing();
    m_columnPos[0] = m_hSpacing;

    if (!m_tableLayout || style()->tableLayout() != oldTableLayout) {
        // According to the CSS2 spec, you only use fixed table layout if an
        // explicit width is specified on the table.  Auto width implies auto table layout.
        if (style()->tableLayout() == TFIXED && !style()->logicalWidth().isAuto())
            m_tableLayout = adoptPtr(new FixedTableLayout(this));
        else
            m_tableLayout = adoptPtr(new AutoTableLayout(this));
    }
}

static inline void resetSectionPointerIfNotBefore(RenderTableSection*& ptr, RenderObject* before)
{
    if (!before || !ptr)
        return;
    RenderObject* o = before->previousSibling();
    while (o && o != ptr)
        o = o->previousSibling();
    if (!o)
        ptr = 0;
}

void RenderTable::addChild(RenderObject* child, RenderObject* beforeChild)
{
    // Make sure we don't append things after :after-generated content if we have it.
    if (!beforeChild && isAfterContent(lastChild()))
        beforeChild = lastChild();

    bool wrapInAnonymousSection = !child->isPositioned();

    if (child->isRenderBlock() && child->style()->display() == TABLE_CAPTION) {
        // First caption wins.
        if (beforeChild && m_caption) {
            RenderObject* o = beforeChild->previousSibling();
            while (o && o != m_caption)
                o = o->previousSibling();
            if (!o) {
                m_caption = 0;
                setNeedsSectionRecalc();
            }
        }
        if (!m_caption)
            m_caption = toRenderBlock(child);
        else
            setNeedsSectionRecalc();
        wrapInAnonymousSection = false;
    } else if (child->isTableCol()) {
        m_hasColElements = true;
        wrapInAnonymousSection = false;
    } else if (child->isTableSection()) {
        switch (child->style()->display()) {
            case TABLE_HEADER_GROUP:
                resetSectionPointerIfNotBefore(m_head, beforeChild);
                if (!m_head) {
                    m_head = toRenderTableSection(child);
                } else {
                    resetSectionPointerIfNotBefore(m_firstBody, beforeChild);
                    if (!m_firstBody) 
                        m_firstBody = toRenderTableSection(child);
                }
                wrapInAnonymousSection = false;
                break;
            case TABLE_FOOTER_GROUP:
                resetSectionPointerIfNotBefore(m_foot, beforeChild);
                if (!m_foot) {
                    m_foot = toRenderTableSection(child);
                    wrapInAnonymousSection = false;
                    break;
                }
                // Fall through.
            case TABLE_ROW_GROUP:
                resetSectionPointerIfNotBefore(m_firstBody, beforeChild);
                if (!m_firstBody)
                    m_firstBody = toRenderTableSection(child);
                wrapInAnonymousSection = false;
                break;
            default:
                ASSERT_NOT_REACHED();
        }
    } else if (child->isTableCell() || child->isTableRow())
        wrapInAnonymousSection = true;
    else
        wrapInAnonymousSection = true;

    if (!wrapInAnonymousSection) {
        // If the next renderer is actually wrapped in an anonymous table section, we need to go up and find that.
        while (beforeChild && beforeChild->parent() != this)
            beforeChild = beforeChild->parent();

        RenderBox::addChild(child, beforeChild);
        return;
    }

    if (!beforeChild && lastChild() && lastChild()->isTableSection() && lastChild()->isAnonymous()) {
        lastChild()->addChild(child);
        return;
    }

    RenderObject* lastBox = beforeChild;
    while (lastBox && lastBox->parent()->isAnonymous() && !lastBox->isTableSection() && lastBox->style()->display() != TABLE_CAPTION && lastBox->style()->display() != TABLE_COLUMN_GROUP)
        lastBox = lastBox->parent();
    if (lastBox && lastBox->isAnonymous() && !isAfterContent(lastBox)) {
        if (beforeChild == lastBox)
            beforeChild = lastBox->firstChild();
        lastBox->addChild(child, beforeChild);
        return;
    }

    if (beforeChild && !beforeChild->isTableSection() && beforeChild->style()->display() != TABLE_CAPTION && beforeChild->style()->display() != TABLE_COLUMN_GROUP)
        beforeChild = 0;
    RenderTableSection* section = new (renderArena()) RenderTableSection(document() /* anonymous */);
    RefPtr<RenderStyle> newStyle = RenderStyle::create();
    newStyle->inheritFrom(style());
    newStyle->setDisplay(TABLE_ROW_GROUP);
    section->setStyle(newStyle.release());
    addChild(section, beforeChild);
    section->addChild(child);
}

void RenderTable::removeChild(RenderObject* oldChild)
{
    RenderBox::removeChild(oldChild);
    
    if (m_caption && oldChild == m_caption && node())
        node()->setNeedsStyleRecalc();
    setNeedsSectionRecalc();
}

void RenderTable::computeLogicalWidth()
{
    if (isPositioned())
        computePositionedLogicalWidth();

    RenderBlock* cb = containingBlock();

    int availableLogicalWidth = containingBlockLogicalWidthForContent();
    bool hasPerpendicularContainingBlock = cb->style()->isHorizontalWritingMode() != style()->isHorizontalWritingMode();
    int containerWidthInInlineDirection = hasPerpendicularContainingBlock ? perpendicularContainingBlockLogicalHeight() : availableLogicalWidth;

    LengthType logicalWidthType = style()->logicalWidth().type();
    if (logicalWidthType > Relative && style()->logicalWidth().isPositive()) {
        // Percent or fixed table
        setLogicalWidth(style()->logicalWidth().calcMinValue(containerWidthInInlineDirection));
        setLogicalWidth(max(minPreferredLogicalWidth(), logicalWidth()));
    } else {
        // Subtract out any fixed margins from our available width for auto width tables.
        int marginTotal = 0;
        if (!style()->marginStart().isAuto())
            marginTotal += style()->marginStart().calcValue(availableLogicalWidth);
        if (!style()->marginEnd().isAuto())
            marginTotal += style()->marginEnd().calcValue(availableLogicalWidth);
            
        // Subtract out our margins to get the available content width.
        int availableContentLogicalWidth = max(0, containerWidthInInlineDirection - marginTotal);
        
        // Ensure we aren't bigger than our max width or smaller than our min width.
        setLogicalWidth(min(availableContentLogicalWidth, maxPreferredLogicalWidth()));
    }

    setLogicalWidth(max(logicalWidth(), minPreferredLogicalWidth()));

    // Finally, with our true width determined, compute our margins for real.
    setMarginStart(0);
    setMarginEnd(0);
    if (!hasPerpendicularContainingBlock)
        computeInlineDirectionMargins(cb, availableLogicalWidth, logicalWidth());
    else {
        setMarginStart(style()->marginStart().calcMinValue(availableLogicalWidth));
        setMarginEnd(style()->marginEnd().calcMinValue(availableLogicalWidth));
    }
}

void RenderTable::adjustLogicalHeightForCaption()
{
    ASSERT(m_caption);
    IntRect captionRect(m_caption->x(), m_caption->y(), m_caption->width(), m_caption->height());

    m_caption->setLogicalLocation(m_caption->marginStart(), logicalHeight());
    if (!selfNeedsLayout() && m_caption->checkForRepaintDuringLayout())
        m_caption->repaintDuringLayoutIfMoved(captionRect);

    setLogicalHeight(logicalHeight() + m_caption->logicalHeight() + m_caption->marginBefore() + m_caption->marginAfter());
}

void RenderTable::layout()
{
    ASSERT(needsLayout());

    if (simplifiedLayout())
        return;

    recalcSectionsIfNeeded();
        
    LayoutRepainter repainter(*this, checkForRepaintDuringLayout());
    LayoutStateMaintainer statePusher(view(), this, IntSize(x(), y()), style()->isFlippedBlocksWritingMode());

    setLogicalHeight(0);
    m_overflow.clear();

    initMaxMarginValues();
    
    int oldLogicalWidth = logicalWidth();
    computeLogicalWidth();

    if (m_caption && logicalWidth() != oldLogicalWidth)
        m_caption->setNeedsLayout(true, false);

    // FIXME: The optimisation below doesn't work since the internal table
    // layout could have changed.  we need to add a flag to the table
    // layout that tells us if something has changed in the min max
    // calculations to do it correctly.
//     if ( oldWidth != width() || columns.size() + 1 != columnPos.size() )
    m_tableLayout->layout();

    setCellLogicalWidths();

    int totalSectionLogicalHeight = 0;
    int oldTableLogicalTop = m_caption ? m_caption->logicalHeight() + m_caption->marginBefore() + m_caption->marginAfter() : 0;

    bool collapsing = collapseBorders();

    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (child->isTableSection()) {
            child->layoutIfNeeded();
            RenderTableSection* section = toRenderTableSection(child);
            totalSectionLogicalHeight += section->calcRowLogicalHeight();
            if (collapsing)
                section->recalcOuterBorder();
            ASSERT(!section->needsLayout());
        } else if (child->isTableCol()) {
            child->layoutIfNeeded();
            ASSERT(!child->needsLayout());
        }
    }

    // Only lay out one caption, since it's the only one we're going to end up painting.
    if (m_caption)
        m_caption->layoutIfNeeded();

    // If any table section moved vertically, we will just repaint everything from that
    // section down (it is quite unlikely that any of the following sections
    // did not shift).
    bool sectionMoved = false;
    int movedSectionLogicalTop = 0;

    // FIXME: Collapse caption margin.
    if (m_caption && m_caption->style()->captionSide() != CAPBOTTOM) {
        adjustLogicalHeightForCaption();
        if (logicalHeight() != oldTableLogicalTop) {
            sectionMoved = true;
            movedSectionLogicalTop = min(logicalHeight(), oldTableLogicalTop);
        }
    }

    int borderAndPaddingBefore = borderBefore() + (collapsing ? 0 : paddingBefore());
    int borderAndPaddingAfter = borderAfter() + (collapsing ? 0 : paddingAfter());

    setLogicalHeight(logicalHeight() + borderAndPaddingBefore);

    if (!isPositioned())
        computeLogicalHeight();

    Length logicalHeightLength = style()->logicalHeight();
    int computedLogicalHeight = 0;
    if (logicalHeightLength.isFixed()) {
        // Tables size as though CSS height includes border/padding.
        computedLogicalHeight = logicalHeightLength.value() - (borderAndPaddingBefore + borderAndPaddingAfter);
    } else if (logicalHeightLength.isPercent())
        computedLogicalHeight = computePercentageLogicalHeight(logicalHeightLength);
    computedLogicalHeight = max(0, computedLogicalHeight);

    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (child->isTableSection())
            // FIXME: Distribute extra height between all table body sections instead of giving it all to the first one.
            toRenderTableSection(child)->layoutRows(child == m_firstBody ? max(0, computedLogicalHeight - totalSectionLogicalHeight) : 0);
    }

    if (!m_firstBody && computedLogicalHeight > totalSectionLogicalHeight && !document()->inQuirksMode()) {
        // Completely empty tables (with no sections or anything) should at least honor specified height
        // in strict mode.
        setLogicalHeight(logicalHeight() + computedLogicalHeight);
    }

    int sectionLogicalLeft = style()->isLeftToRightDirection() ? borderStart() : borderEnd();
    if (!collapsing)
        sectionLogicalLeft += style()->isLeftToRightDirection() ? paddingStart() : paddingEnd();

    // position the table sections
    RenderTableSection* section = m_head ? m_head : (m_firstBody ? m_firstBody : m_foot);
    while (section) {
        if (!sectionMoved && section->logicalTop() != logicalHeight()) {
            sectionMoved = true;
            movedSectionLogicalTop = min(logicalHeight(), section->logicalTop()) + (style()->isHorizontalWritingMode() ? section->minYVisualOverflow() : section->minXVisualOverflow());
        }
        section->setLogicalLocation(sectionLogicalLeft, logicalHeight());

        setLogicalHeight(logicalHeight() + section->logicalHeight());
        section = sectionBelow(section);
    }

    setLogicalHeight(logicalHeight() + borderAndPaddingAfter);

    if (m_caption && m_caption->style()->captionSide() == CAPBOTTOM)
        adjustLogicalHeightForCaption();

    if (isPositioned())
        computeLogicalHeight();

    // table can be containing block of positioned elements.
    // FIXME: Only pass true if width or height changed.
    layoutPositionedObjects(true);

    updateLayerTransform();

    computeOverflow(clientLogicalBottom());

    statePusher.pop();

    if (view()->layoutState()->pageLogicalHeight())
        setPageLogicalOffset(view()->layoutState()->pageLogicalOffset(logicalTop()));

    bool didFullRepaint = repainter.repaintAfterLayout();
    // Repaint with our new bounds if they are different from our old bounds.
    if (!didFullRepaint && sectionMoved) {
        if (style()->isHorizontalWritingMode())
            repaintRectangle(IntRect(minXVisualOverflow(), movedSectionLogicalTop, maxXVisualOverflow() - minXVisualOverflow(), maxYVisualOverflow() - movedSectionLogicalTop));
        else
            repaintRectangle(IntRect(movedSectionLogicalTop, minYVisualOverflow(), maxXVisualOverflow() - movedSectionLogicalTop, maxYVisualOverflow() - minYVisualOverflow()));
    }

    setNeedsLayout(false);
}

void RenderTable::addOverflowFromChildren()
{
    // Add overflow from borders.
    // Technically it's odd that we are incorporating the borders into layout overflow, which is only supposed to be about overflow from our
    // descendant objects, but since tables don't support overflow:auto, this works out fine.
    if (collapseBorders()) {
        int rightBorderOverflow = width() + outerBorderRight() - borderRight();
        int leftBorderOverflow = borderLeft() - outerBorderLeft();
        int bottomBorderOverflow = height() + outerBorderBottom() - borderBottom();
        int topBorderOverflow = borderTop() - outerBorderTop();
        IntRect borderOverflowRect(leftBorderOverflow, topBorderOverflow, rightBorderOverflow - leftBorderOverflow, bottomBorderOverflow - topBorderOverflow);
        if (borderOverflowRect != borderBoxRect()) {
            addLayoutOverflow(borderOverflowRect);
            addVisualOverflow(borderOverflowRect);
        }
    }

    // Add overflow from our caption.
    if (m_caption)
        addOverflowFromChild(m_caption);

    // Add overflow from our sections.
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (child->isTableSection()) {
            RenderTableSection* section = toRenderTableSection(child);
            addOverflowFromChild(section);
        }
    }
}

void RenderTable::setCellLogicalWidths()
{
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (child->isTableSection())
            toRenderTableSection(child)->setCellLogicalWidths();
    }
}

void RenderTable::paint(PaintInfo& paintInfo, int tx, int ty)
{
    tx += x();
    ty += y();

    PaintPhase paintPhase = paintInfo.phase;

    if (!isRoot()) {
        IntRect overflowBox = visualOverflowRect();
        flipForWritingMode(overflowBox);
        overflowBox.inflate(maximalOutlineSize(paintInfo.phase));
        overflowBox.move(tx, ty);
        if (!overflowBox.intersects(paintInfo.rect))
            return;
    }

    bool pushedClip = pushContentsClip(paintInfo, tx, ty);    
    paintObject(paintInfo, tx, ty);
    if (pushedClip)
        popContentsClip(paintInfo, paintPhase, tx, ty);
}

void RenderTable::paintObject(PaintInfo& paintInfo, int tx, int ty)
{
    PaintPhase paintPhase = paintInfo.phase;
    if ((paintPhase == PaintPhaseBlockBackground || paintPhase == PaintPhaseChildBlockBackground) && hasBoxDecorations() && style()->visibility() == VISIBLE)
        paintBoxDecorations(paintInfo, tx, ty);

    if (paintPhase == PaintPhaseMask) {
        paintMask(paintInfo, tx, ty);
        return;
    }

    // We're done.  We don't bother painting any children.
    if (paintPhase == PaintPhaseBlockBackground)
        return;
    
    // We don't paint our own background, but we do let the kids paint their backgrounds.
    if (paintPhase == PaintPhaseChildBlockBackgrounds)
        paintPhase = PaintPhaseChildBlockBackground;

    PaintInfo info(paintInfo);
    info.phase = paintPhase;
    info.updatePaintingRootForChildren(this);

    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (child->isBox() && !toRenderBox(child)->hasSelfPaintingLayer() && (child->isTableSection() || child == m_caption)) {
            IntPoint childPoint = flipForWritingMode(toRenderBox(child), IntPoint(tx, ty), ParentToChildFlippingAdjustment);
            child->paint(info, childPoint.x(), childPoint.y());
        }
    }
    
    if (collapseBorders() && paintPhase == PaintPhaseChildBlockBackground && style()->visibility() == VISIBLE) {
        // Collect all the unique border styles that we want to paint in a sorted list.  Once we
        // have all the styles sorted, we then do individual passes, painting each style of border
        // from lowest precedence to highest precedence.
        info.phase = PaintPhaseCollapsedTableBorders;
        RenderTableCell::CollapsedBorderStyles borderStyles;
        RenderObject* stop = nextInPreOrderAfterChildren();
        for (RenderObject* o = firstChild(); o && o != stop; o = o->nextInPreOrder()) {
            if (o->isTableCell())
                toRenderTableCell(o)->collectBorderStyles(borderStyles);
        }
        RenderTableCell::sortBorderStyles(borderStyles);
        size_t count = borderStyles.size();
        for (size_t i = 0; i < count; ++i) {
            m_currentBorder = &borderStyles[i];
            for (RenderObject* child = firstChild(); child; child = child->nextSibling())
                if (child->isTableSection()) {
                    IntPoint childPoint = flipForWritingMode(toRenderTableSection(child), IntPoint(tx, ty), ParentToChildFlippingAdjustment);
                    child->paint(info, childPoint.x(), childPoint.y());
                }
        }
        m_currentBorder = 0;
    }

    // Paint outline.
    if ((paintPhase == PaintPhaseOutline || paintPhase == PaintPhaseSelfOutline) && hasOutline() && style()->visibility() == VISIBLE)
        paintOutline(paintInfo.context, tx, ty, width(), height());
}

void RenderTable::subtractCaptionRect(IntRect& rect) const
{
    if (!m_caption)
        return;

    int captionLogicalHeight = m_caption->logicalHeight() + m_caption->marginBefore() + m_caption->marginAfter();
    bool captionIsBefore = (m_caption->style()->captionSide() != CAPBOTTOM) ^ style()->isFlippedBlocksWritingMode();
    if (style()->isHorizontalWritingMode()) {
        rect.setHeight(rect.height() - captionLogicalHeight);
        if (captionIsBefore)
            rect.move(0, captionLogicalHeight);
    } else {
        rect.setWidth(rect.width() - captionLogicalHeight);
        if (captionIsBefore)
            rect.move(captionLogicalHeight, 0);
    }
}

void RenderTable::paintBoxDecorations(PaintInfo& paintInfo, int tx, int ty)
{
    if (!paintInfo.shouldPaintWithinRoot(this))
        return;

    IntRect rect(tx, ty, width(), height());
    subtractCaptionRect(rect);

    paintBoxShadow(paintInfo.context, rect.x(), rect.y(), rect.width(), rect.height(), style(), Normal);
    
    if (isRoot())
        paintRootBoxFillLayers(paintInfo);
    else if (!isBody() || document()->documentElement()->renderer()->hasBackground())
        // The <body> only paints its background if the root element has defined a background
        // independent of the body.
        paintFillLayers(paintInfo, style()->visitedDependentColor(CSSPropertyBackgroundColor), style()->backgroundLayers(), rect.x(), rect.y(), rect.width(), rect.height());

    paintBoxShadow(paintInfo.context, rect.x(), rect.y(), rect.width(), rect.height(), style(), Inset);

    if (style()->hasBorder() && !collapseBorders())
        paintBorder(paintInfo.context, rect.x(), rect.y(), rect.width(), rect.height(), style());
}

void RenderTable::paintMask(PaintInfo& paintInfo, int tx, int ty)
{
    if (style()->visibility() != VISIBLE || paintInfo.phase != PaintPhaseMask)
        return;

    IntRect rect(tx, ty, width(), height());
    subtractCaptionRect(rect);

    paintMaskImages(paintInfo, rect.x(), rect.y(), rect.width(), rect.height());
}

void RenderTable::computePreferredLogicalWidths()
{
    ASSERT(preferredLogicalWidthsDirty());

    recalcSectionsIfNeeded();
    recalcBordersInRowDirection();

    m_tableLayout->computePreferredLogicalWidths(m_minPreferredLogicalWidth, m_maxPreferredLogicalWidth);

    if (m_caption)
        m_minPreferredLogicalWidth = max(m_minPreferredLogicalWidth, m_caption->minPreferredLogicalWidth());

    setPreferredLogicalWidthsDirty(false);
}

void RenderTable::splitColumn(int pos, int firstSpan)
{
    // we need to add a new columnStruct
    int oldSize = m_columns.size();
    m_columns.grow(oldSize + 1);
    int oldSpan = m_columns[pos].span;
    ASSERT(oldSpan > firstSpan);
    m_columns[pos].span = firstSpan;
    memmove(m_columns.data() + pos + 1, m_columns.data() + pos, (oldSize - pos) * sizeof(ColumnStruct));
    m_columns[pos + 1].span = oldSpan - firstSpan;

    // change width of all rows.
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (child->isTableSection())
            toRenderTableSection(child)->splitColumn(pos, firstSpan);
    }

    m_columnPos.grow(numEffCols() + 1);
    setNeedsLayoutAndPrefWidthsRecalc();
}

void RenderTable::appendColumn(int span)
{
    // easy case.
    int pos = m_columns.size();
    int newSize = pos + 1;
    m_columns.grow(newSize);
    m_columns[pos].span = span;

    // change width of all rows.
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (child->isTableSection())
            toRenderTableSection(child)->appendColumn(pos);
    }

    m_columnPos.grow(numEffCols() + 1);
    setNeedsLayoutAndPrefWidthsRecalc();
}

RenderTableCol* RenderTable::nextColElement(RenderTableCol* current) const
{
    RenderObject* next = current->firstChild();
    if (!next)
        next = current->nextSibling();
    if (!next && current->parent()->isTableCol())
        next = current->parent()->nextSibling();

    while (next) {
        if (next->isTableCol())
            return toRenderTableCol(next);
        if (next != m_caption)
            return 0;
        next = next->nextSibling();
    }
    
    return 0;
}

RenderTableCol* RenderTable::colElement(int col, bool* startEdge, bool* endEdge) const
{
    if (!m_hasColElements)
        return 0;
    RenderObject* child = firstChild();
    int cCol = 0;

    while (child) {
        if (child->isTableCol())
            break;
        if (child != m_caption)
            return 0;
        child = child->nextSibling();
    }
    if (!child)
        return 0;

    RenderTableCol* colElem = toRenderTableCol(child);
    while (colElem) {
        int span = colElem->span();
        if (!colElem->firstChild()) {
            int startCol = cCol;
            int endCol = cCol + span - 1;
            cCol += span;
            if (cCol > col) {
                if (startEdge)
                    *startEdge = startCol == col;
                if (endEdge)
                    *endEdge = endCol == col;
                return colElem;
            }
        }
        colElem = nextColElement(colElem);
    }

    return 0;
}

void RenderTable::recalcCaption(RenderBlock* caption) const
{
    if (!m_caption) {
        m_caption = caption;
        m_caption->setNeedsLayout(true);
    } else {
        // Make sure to null out the child's renderer.
        if (Node* node = caption->node())
            node->setRenderer(0);

        // Destroy the child now.
        caption->destroy();
    }
}

void RenderTable::recalcSections() const
{
    m_caption = 0;
    m_head = 0;
    m_foot = 0;
    m_firstBody = 0;
    m_hasColElements = false;

    // We need to get valid pointers to caption, head, foot and first body again
    RenderObject* nextSibling;
    for (RenderObject* child = firstChild(); child; child = nextSibling) {
        nextSibling = child->nextSibling();
        switch (child->style()->display()) {
            case TABLE_CAPTION:
                if (child->isRenderBlock())
                    recalcCaption(toRenderBlock(child));
                break;
            case TABLE_COLUMN:
            case TABLE_COLUMN_GROUP:
                m_hasColElements = true;
                break;
            case TABLE_HEADER_GROUP:
                if (child->isTableSection()) {
                    RenderTableSection* section = toRenderTableSection(child);
                    if (!m_head)
                        m_head = section;
                    else if (!m_firstBody)
                        m_firstBody = section;
                    section->recalcCellsIfNeeded();
                }
                break;
            case TABLE_FOOTER_GROUP:
                if (child->isTableSection()) {
                    RenderTableSection* section = toRenderTableSection(child);
                    if (!m_foot)
                        m_foot = section;
                    else if (!m_firstBody)
                        m_firstBody = section;
                    section->recalcCellsIfNeeded();
                }
                break;
            case TABLE_ROW_GROUP:
                if (child->isTableSection()) {
                    RenderTableSection* section = toRenderTableSection(child);
                    if (!m_firstBody)
                        m_firstBody = section;
                    section->recalcCellsIfNeeded();
                }
                break;
            default:
                break;
        }
    }

    // repair column count (addChild can grow it too much, because it always adds elements to the last row of a section)
    int maxCols = 0;
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (child->isTableSection()) {
            RenderTableSection* section = toRenderTableSection(child);
            int sectionCols = section->numColumns();
            if (sectionCols > maxCols)
                maxCols = sectionCols;
        }
    }
    
    m_columns.resize(maxCols);
    m_columnPos.resize(maxCols + 1);

    ASSERT(selfNeedsLayout());

    m_needsSectionRecalc = false;
}

int RenderTable::calcBorderStart() const
{
    if (collapseBorders()) {
        // Determined by the first cell of the first row. See the CSS 2.1 spec, section 17.6.2.
        if (!numEffCols())
            return 0;

        unsigned borderWidth = 0;

        const BorderValue& tb = style()->borderStart();
        if (tb.style() == BHIDDEN)
            return 0;
        if (tb.style() > BHIDDEN)
            borderWidth = tb.width();

        if (RenderTableCol* colGroup = colElement(0)) {
            const BorderValue& gb = colGroup->style()->borderStart();
            if (gb.style() == BHIDDEN)
                return 0;
            if (gb.style() > BHIDDEN)
                borderWidth = max(borderWidth, static_cast<unsigned>(gb.width()));
        }
        
        RenderTableSection* firstNonEmptySection = m_head ? m_head : (m_firstBody ? m_firstBody : m_foot);
        if (firstNonEmptySection && !firstNonEmptySection->numRows())
            firstNonEmptySection = sectionBelow(firstNonEmptySection, true);
        
        if (firstNonEmptySection) {
            const BorderValue& sb = firstNonEmptySection->style()->borderStart();
            if (sb.style() == BHIDDEN)
                return 0;

            if (sb.style() > BHIDDEN)
                borderWidth = max(borderWidth, static_cast<unsigned>(sb.width()));

            const RenderTableSection::CellStruct& cs = firstNonEmptySection->cellAt(0, 0);
            
            if (cs.hasCells()) {
                const BorderValue& cb = cs.primaryCell()->style()->borderStart(); // FIXME: Make this work with perpendicualr and flipped cells.
                if (cb.style() == BHIDDEN)
                    return 0;

                const BorderValue& rb = cs.primaryCell()->parent()->style()->borderStart();
                if (rb.style() == BHIDDEN)
                    return 0;

                if (cb.style() > BHIDDEN)
                    borderWidth = max(borderWidth, static_cast<unsigned>(cb.width()));
                if (rb.style() > BHIDDEN)
                    borderWidth = max(borderWidth, static_cast<unsigned>(rb.width()));
            }
        }
        return (borderWidth + (style()->isLeftToRightDirection() ? 0 : 1)) / 2;
    }
    return RenderBlock::borderStart();
}

int RenderTable::calcBorderEnd() const
{
    if (collapseBorders()) {
        // Determined by the last cell of the first row. See the CSS 2.1 spec, section 17.6.2.
        if (!numEffCols())
            return 0;

        unsigned borderWidth = 0;

        const BorderValue& tb = style()->borderEnd();
        if (tb.style() == BHIDDEN)
            return 0;
        if (tb.style() > BHIDDEN)
            borderWidth = tb.width();

        int endColumn = numEffCols() - 1;
        if (RenderTableCol* colGroup = colElement(endColumn)) {
            const BorderValue& gb = colGroup->style()->borderEnd();
            if (gb.style() == BHIDDEN)
                return 0;
            if (gb.style() > BHIDDEN)
                borderWidth = max(borderWidth, static_cast<unsigned>(gb.width()));
        }
        
        RenderTableSection* firstNonEmptySection = m_head ? m_head : (m_firstBody ? m_firstBody : m_foot);
        if (firstNonEmptySection && !firstNonEmptySection->numRows())
            firstNonEmptySection = sectionBelow(firstNonEmptySection, true);
        
        if (firstNonEmptySection) {
            const BorderValue& sb = firstNonEmptySection->style()->borderEnd();
            if (sb.style() == BHIDDEN)
                return 0;

            if (sb.style() > BHIDDEN)
                borderWidth = max(borderWidth, static_cast<unsigned>(sb.width()));

            const RenderTableSection::CellStruct& cs = firstNonEmptySection->cellAt(0, endColumn);
            
            if (cs.hasCells()) {
                const BorderValue& cb = cs.primaryCell()->style()->borderEnd(); // FIXME: Make this work with perpendicular and flipped cells.
                if (cb.style() == BHIDDEN)
                    return 0;

                const BorderValue& rb = cs.primaryCell()->parent()->style()->borderEnd();
                if (rb.style() == BHIDDEN)
                    return 0;

                if (cb.style() > BHIDDEN)
                    borderWidth = max(borderWidth, static_cast<unsigned>(cb.width()));
                if (rb.style() > BHIDDEN)
                    borderWidth = max(borderWidth, static_cast<unsigned>(rb.width()));
            }
        }
        return (borderWidth + (style()->isLeftToRightDirection() ? 1 : 0)) / 2;
    }
    return RenderBlock::borderEnd();
}

void RenderTable::recalcBordersInRowDirection()
{
    m_borderStart = calcBorderStart();
    m_borderEnd = calcBorderEnd();
}

int RenderTable::borderBefore() const
{
    if (collapseBorders())
        return outerBorderBefore();
    return RenderBlock::borderBefore();
}

int RenderTable::borderAfter() const
{
    if (collapseBorders())
        return outerBorderAfter();
    return RenderBlock::borderAfter();
}

int RenderTable::outerBorderBefore() const
{
    if (!collapseBorders())
        return 0;
    int borderWidth = 0;
    RenderTableSection* topSection;
    if (m_head)
        topSection = m_head;
    else if (m_firstBody)
        topSection = m_firstBody;
    else if (m_foot)
        topSection = m_foot;
    else
        topSection = 0;
    if (topSection) {
        borderWidth = topSection->outerBorderBefore();
        if (borderWidth == -1)
            return 0;   // Overridden by hidden
    }
    const BorderValue& tb = style()->borderBefore();
    if (tb.style() == BHIDDEN)
        return 0;
    if (tb.style() > BHIDDEN)
        borderWidth = max(borderWidth, static_cast<int>(tb.width() / 2));
    return borderWidth;
}

int RenderTable::outerBorderAfter() const
{
    if (!collapseBorders())
        return 0;
    int borderWidth = 0;
    RenderTableSection* bottomSection;
    if (m_foot)
        bottomSection = m_foot;
    else {
        RenderObject* child;
        for (child = lastChild(); child && !child->isTableSection(); child = child->previousSibling()) { }
        bottomSection = child ? toRenderTableSection(child) : 0;
    }
    if (bottomSection) {
        borderWidth = bottomSection->outerBorderAfter();
        if (borderWidth == -1)
            return 0;   // Overridden by hidden
    }
    const BorderValue& tb = style()->borderAfter();
    if (tb.style() == BHIDDEN)
        return 0;
    if (tb.style() > BHIDDEN)
        borderWidth = max(borderWidth, static_cast<int>((tb.width() + 1) / 2));
    return borderWidth;
}

int RenderTable::outerBorderStart() const
{
    if (!collapseBorders())
        return 0;

    int borderWidth = 0;

    const BorderValue& tb = style()->borderStart();
    if (tb.style() == BHIDDEN)
        return 0;
    if (tb.style() > BHIDDEN)
        borderWidth = (tb.width() + (style()->isLeftToRightDirection() ? 0 : 1)) / 2;

    bool allHidden = true;
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (!child->isTableSection())
            continue;
        int sw = toRenderTableSection(child)->outerBorderStart();
        if (sw == -1)
            continue;
        else
            allHidden = false;
        borderWidth = max(borderWidth, sw);
    }
    if (allHidden)
        return 0;

    return borderWidth;
}

int RenderTable::outerBorderEnd() const
{
    if (!collapseBorders())
        return 0;

    int borderWidth = 0;

    const BorderValue& tb = style()->borderEnd();
    if (tb.style() == BHIDDEN)
        return 0;
    if (tb.style() > BHIDDEN)
        borderWidth = (tb.width() + (style()->isLeftToRightDirection() ? 1 : 0)) / 2;

    bool allHidden = true;
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (!child->isTableSection())
            continue;
        int sw = toRenderTableSection(child)->outerBorderEnd();
        if (sw == -1)
            continue;
        else
            allHidden = false;
        borderWidth = max(borderWidth, sw);
    }
    if (allHidden)
        return 0;

    return borderWidth;
}

RenderTableSection* RenderTable::sectionAbove(const RenderTableSection* section, bool skipEmptySections) const
{
    recalcSectionsIfNeeded();

    if (section == m_head)
        return 0;

    RenderObject* prevSection = section == m_foot ? lastChild() : section->previousSibling();
    while (prevSection) {
        if (prevSection->isTableSection() && prevSection != m_head && prevSection != m_foot && (!skipEmptySections || toRenderTableSection(prevSection)->numRows()))
            break;
        prevSection = prevSection->previousSibling();
    }
    if (!prevSection && m_head && (!skipEmptySections || m_head->numRows()))
        prevSection = m_head;
    return toRenderTableSection(prevSection);
}

RenderTableSection* RenderTable::sectionBelow(const RenderTableSection* section, bool skipEmptySections) const
{
    recalcSectionsIfNeeded();

    if (section == m_foot)
        return 0;

    RenderObject* nextSection = section == m_head ? firstChild() : section->nextSibling();
    while (nextSection) {
        if (nextSection->isTableSection() && nextSection != m_head && nextSection != m_foot && (!skipEmptySections || toRenderTableSection(nextSection)->numRows()))
            break;
        nextSection = nextSection->nextSibling();
    }
    if (!nextSection && m_foot && (!skipEmptySections || m_foot->numRows()))
        nextSection = m_foot;
    return toRenderTableSection(nextSection);
}

RenderTableCell* RenderTable::cellAbove(const RenderTableCell* cell) const
{
    recalcSectionsIfNeeded();

    // Find the section and row to look in
    int r = cell->row();
    RenderTableSection* section = 0;
    int rAbove = 0;
    if (r > 0) {
        // cell is not in the first row, so use the above row in its own section
        section = cell->section();
        rAbove = r - 1;
    } else {
        section = sectionAbove(cell->section(), true);
        if (section)
            rAbove = section->numRows() - 1;
    }

    // Look up the cell in the section's grid, which requires effective col index
    if (section) {
        int effCol = colToEffCol(cell->col());
        RenderTableSection::CellStruct& aboveCell = section->cellAt(rAbove, effCol);
        return aboveCell.primaryCell();
    } else
        return 0;
}

RenderTableCell* RenderTable::cellBelow(const RenderTableCell* cell) const
{
    recalcSectionsIfNeeded();

    // Find the section and row to look in
    int r = cell->row() + cell->rowSpan() - 1;
    RenderTableSection* section = 0;
    int rBelow = 0;
    if (r < cell->section()->numRows() - 1) {
        // The cell is not in the last row, so use the next row in the section.
        section = cell->section();
        rBelow = r + 1;
    } else {
        section = sectionBelow(cell->section(), true);
        if (section)
            rBelow = 0;
    }

    // Look up the cell in the section's grid, which requires effective col index
    if (section) {
        int effCol = colToEffCol(cell->col());
        RenderTableSection::CellStruct& belowCell = section->cellAt(rBelow, effCol);
        return belowCell.primaryCell();
    } else
        return 0;
}

RenderTableCell* RenderTable::cellBefore(const RenderTableCell* cell) const
{
    recalcSectionsIfNeeded();

    RenderTableSection* section = cell->section();
    int effCol = colToEffCol(cell->col());
    if (!effCol)
        return 0;
    
    // If we hit a colspan back up to a real cell.
    RenderTableSection::CellStruct& prevCell = section->cellAt(cell->row(), effCol - 1);
    return prevCell.primaryCell();
}

RenderTableCell* RenderTable::cellAfter(const RenderTableCell* cell) const
{
    recalcSectionsIfNeeded();

    int effCol = colToEffCol(cell->col() + cell->colSpan());
    if (effCol >= numEffCols())
        return 0;
    return cell->section()->primaryCellAt(cell->row(), effCol);
}

RenderBlock* RenderTable::firstLineBlock() const
{
    return 0;
}

void RenderTable::updateFirstLetter()
{
}

int RenderTable::firstLineBoxBaseline() const
{
    if (isWritingModeRoot())
        return -1;

    recalcSectionsIfNeeded();

    RenderTableSection* firstNonEmptySection = m_head ? m_head : (m_firstBody ? m_firstBody : m_foot);
    if (firstNonEmptySection && !firstNonEmptySection->numRows())
        firstNonEmptySection = sectionBelow(firstNonEmptySection, true);

    if (!firstNonEmptySection)
        return -1;

    return firstNonEmptySection->logicalTop() + firstNonEmptySection->firstLineBoxBaseline();
}

IntRect RenderTable::overflowClipRect(int tx, int ty, OverlayScrollbarSizeRelevancy relevancy)
{
    IntRect rect = RenderBlock::overflowClipRect(tx, ty, relevancy);
    
    // If we have a caption, expand the clip to include the caption.
    // FIXME: Technically this is wrong, but it's virtually impossible to fix this
    // for real until captions have been re-written.
    // FIXME: This code assumes (like all our other caption code) that only top/bottom are
    // supported.  When we actually support left/right and stop mapping them to top/bottom,
    // we might have to hack this code first (depending on what order we do these bug fixes in).
    if (m_caption) {
        if (style()->isHorizontalWritingMode()) {
            rect.setHeight(height());
            rect.setY(ty);
        } else {
            rect.setWidth(width());
            rect.setX(tx);
        }
    }

    return rect;
}

bool RenderTable::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, int xPos, int yPos, int tx, int ty, HitTestAction action)
{
    tx += x();
    ty += y();

    // Check kids first.
    if (!hasOverflowClip() || overflowClipRect(tx, ty).intersects(result.rectForPoint(xPos, yPos))) {
        for (RenderObject* child = lastChild(); child; child = child->previousSibling()) {
            if (child->isBox() && !toRenderBox(child)->hasSelfPaintingLayer() && (child->isTableSection() || child == m_caption)) {
                IntPoint childPoint = flipForWritingMode(toRenderBox(child), IntPoint(tx, ty), ParentToChildFlippingAdjustment);
                if (child->nodeAtPoint(request, result, xPos, yPos, childPoint.x(), childPoint.y(), action)) {
                    updateHitTestResult(result, IntPoint(xPos - childPoint.x(), yPos - childPoint.y()));
                    return true;
                }
            }
        }
    }

    // Check our bounds next.
    IntRect boundsRect = IntRect(tx, ty, width(), height());
    if (visibleToHitTesting() && (action == HitTestBlockBackground || action == HitTestChildBlockBackground) && boundsRect.intersects(result.rectForPoint(xPos, yPos))) {
        updateHitTestResult(result, flipForWritingMode(IntPoint(xPos - tx, yPos - ty)));
        if (!result.addNodeToRectBasedTestResult(node(), xPos, yPos, boundsRect))
            return true;
    }

    return false;
}

}
