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
#include "Document.h"
#include "FixedTableLayout.h"
#include "FrameView.h"
#include "HitTestResult.h"
#include "HTMLNames.h"
#include "HTMLTableElement.h"
#include "LayoutRepainter.h"
#include "RenderLayer.h"
#include "RenderTableCaption.h"
#include "RenderTableCell.h"
#include "RenderTableCol.h"
#include "RenderTableSection.h"
#include "RenderView.h"
#include "StyleInheritedData.h"
#include <wtf/StackStats.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;

RenderTable::RenderTable(Element* element)
    : RenderBlock(element)
    , m_head(0)
    , m_foot(0)
    , m_firstBody(0)
    , m_currentBorder(0)
    , m_collapsedBordersValid(false)
    , m_hasColElements(false)
    , m_needsSectionRecalc(false)
    , m_columnLogicalWidthChanged(false)
    , m_columnRenderersValid(false)
    , m_hSpacing(0)
    , m_vSpacing(0)
    , m_borderStart(0)
    , m_borderEnd(0)
{
    setChildrenInline(false);
    m_columnPos.fill(0, 1);
    
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

    // If border was changed, invalidate collapsed borders cache.
    if (!needsLayout() && oldStyle && oldStyle->border() != style()->border())
        invalidateCollapsedBorders();
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
    bool wrapInAnonymousSection = !child->isOutOfFlowPositioned();

    if (child->isTableCaption())
        wrapInAnonymousSection = false;
    else if (child->isRenderTableCol()) {
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

    if (child->isTableSection())
        setNeedsSectionRecalc();

    if (!wrapInAnonymousSection) {
        if (beforeChild && beforeChild->parent() != this)
            beforeChild = splitAnonymousBoxesAroundChild(beforeChild);

        RenderBox::addChild(child, beforeChild);
        return;
    }

    if (!beforeChild && lastChild() && lastChild()->isTableSection() && lastChild()->isAnonymous() && !lastChild()->isBeforeContent()) {
        lastChild()->addChild(child);
        return;
    }

    if (beforeChild && !beforeChild->isAnonymous() && beforeChild->parent() == this) {
        RenderObject* section = beforeChild->previousSibling();
        if (section && section->isTableSection() && section->isAnonymous()) {
            section->addChild(child);
            return;
        }
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

    RenderTableSection* section = RenderTableSection::createAnonymousWithParentRenderer(this);
    addChild(section, beforeChild);
    section->addChild(child);
}

void RenderTable::addCaption(const RenderTableCaption* caption)
{
    ASSERT(m_captions.find(caption) == notFound);
    m_captions.append(const_cast<RenderTableCaption*>(caption));
}

void RenderTable::removeCaption(const RenderTableCaption* oldCaption)
{
    size_t index = m_captions.find(oldCaption);
    ASSERT(index != notFound);
    if (index == notFound)
        return;

    m_captions.remove(index);
}

void RenderTable::invalidateCachedColumns()
{
    m_columnRenderersValid = false;
    m_columnRenderers.resize(0);
}

void RenderTable::addColumn(const RenderTableCol*)
{
    invalidateCachedColumns();
}

void RenderTable::removeColumn(const RenderTableCol*)
{
    invalidateCachedColumns();
    // We don't really need to recompute our sections, but we need to update our
    // column count and whether we have a column. Currently, we only have one
    // size-fit-all flag but we may have to consider splitting it.
    setNeedsSectionRecalc();
}

void RenderTable::updateLogicalWidth()
{
    recalcSectionsIfNeeded();

    if (isOutOfFlowPositioned()) {
        LogicalExtentComputedValues computedValues;
        computePositionedLogicalWidth(computedValues);
        setLogicalWidth(computedValues.m_extent);
        setLogicalLeft(computedValues.m_position);
        setMarginStart(computedValues.m_margins.m_start);
        setMarginEnd(computedValues.m_margins.m_end);
    }

    RenderBlock* cb = containingBlock();
    RenderView* renderView = view();

    LayoutUnit availableLogicalWidth = containingBlockLogicalWidthForContent();
    bool hasPerpendicularContainingBlock = cb->style()->isHorizontalWritingMode() != style()->isHorizontalWritingMode();
    LayoutUnit containerWidthInInlineDirection = hasPerpendicularContainingBlock ? perpendicularContainingBlockLogicalHeight() : availableLogicalWidth;

    Length styleLogicalWidth = style()->logicalWidth();
    if ((styleLogicalWidth.isSpecified() && styleLogicalWidth.isPositive()) || styleLogicalWidth.isIntrinsic())
        setLogicalWidth(convertStyleLogicalWidthToComputedWidth(styleLogicalWidth, containerWidthInInlineDirection));
    else {
        // Subtract out any fixed margins from our available width for auto width tables.
        LayoutUnit marginStart = minimumValueForLength(style()->marginStart(), availableLogicalWidth, renderView);
        LayoutUnit marginEnd = minimumValueForLength(style()->marginEnd(), availableLogicalWidth, renderView);
        LayoutUnit marginTotal = marginStart + marginEnd;

        // Subtract out our margins to get the available content width.
        LayoutUnit availableContentLogicalWidth = max<LayoutUnit>(0, containerWidthInInlineDirection - marginTotal);
        if (shrinkToAvoidFloats() && cb->containsFloats() && !hasPerpendicularContainingBlock) {
            // FIXME: Work with regions someday.
            availableContentLogicalWidth = shrinkLogicalWidthToAvoidFloats(marginStart, marginEnd, cb, 0);
        }

        // Ensure we aren't bigger than our available width.
        setLogicalWidth(min<int>(availableContentLogicalWidth, maxPreferredLogicalWidth()));
    }

    // Ensure we aren't smaller than our min preferred width.
    setLogicalWidth(max<int>(logicalWidth(), minPreferredLogicalWidth()));

    
    // Ensure we aren't bigger than our max-width style.
    Length styleMaxLogicalWidth = style()->logicalMaxWidth();
    if ((styleMaxLogicalWidth.isSpecified() && !styleMaxLogicalWidth.isNegative()) || styleMaxLogicalWidth.isIntrinsic()) {
        LayoutUnit computedMaxLogicalWidth = convertStyleLogicalWidthToComputedWidth(styleMaxLogicalWidth, availableLogicalWidth);
        setLogicalWidth(min<int>(logicalWidth(), computedMaxLogicalWidth));
    }

    // Ensure we aren't smaller than our min-width style.
    Length styleMinLogicalWidth = style()->logicalMinWidth();
    if ((styleMinLogicalWidth.isSpecified() && !styleMinLogicalWidth.isNegative()) || styleMinLogicalWidth.isIntrinsic()) {
        LayoutUnit computedMinLogicalWidth = convertStyleLogicalWidthToComputedWidth(styleMinLogicalWidth, availableLogicalWidth);
        setLogicalWidth(max<int>(logicalWidth(), computedMinLogicalWidth));
    }

    // Finally, with our true width determined, compute our margins for real.
    setMarginStart(0);
    setMarginEnd(0);
    if (!hasPerpendicularContainingBlock) {
        LayoutUnit containerLogicalWidthForAutoMargins = availableLogicalWidth;
        if (avoidsFloats() && cb->containsFloats())
            containerLogicalWidthForAutoMargins = containingBlockAvailableLineWidthInRegion(0); // FIXME: Work with regions someday.
        ComputedMarginValues marginValues;
        bool hasInvertedDirection =  cb->style()->isLeftToRightDirection() == style()->isLeftToRightDirection();
        computeInlineDirectionMargins(cb, containerLogicalWidthForAutoMargins, logicalWidth(),
            hasInvertedDirection ? marginValues.m_start : marginValues.m_end,
            hasInvertedDirection ? marginValues.m_end : marginValues.m_start);
        setMarginStart(marginValues.m_start);
        setMarginEnd(marginValues.m_end);
    } else {
        setMarginStart(minimumValueForLength(style()->marginStart(), availableLogicalWidth, renderView));
        setMarginEnd(minimumValueForLength(style()->marginEnd(), availableLogicalWidth, renderView));
    }
}

// This method takes a RenderStyle's logical width, min-width, or max-width length and computes its actual value.
LayoutUnit RenderTable::convertStyleLogicalWidthToComputedWidth(const Length& styleLogicalWidth, LayoutUnit availableWidth)
{
    if (styleLogicalWidth.isIntrinsic())
        return computeIntrinsicLogicalWidthUsing(styleLogicalWidth, availableWidth, bordersPaddingAndSpacingInRowDirection());

    // HTML tables' width styles already include borders and paddings, but CSS tables' width styles do not.
    LayoutUnit borders = 0;
    bool isCSSTable = !node() || !isHTMLTableElement(node());
    if (isCSSTable && styleLogicalWidth.isSpecified() && styleLogicalWidth.isPositive() && style()->boxSizing() == CONTENT_BOX)
        borders = borderStart() + borderEnd() + (collapseBorders() ? LayoutUnit() : paddingStart() + paddingEnd());

    return minimumValueForLength(styleLogicalWidth, availableWidth, view()) + borders;
}

LayoutUnit RenderTable::convertStyleLogicalHeightToComputedHeight(const Length& styleLogicalHeight)
{
    LayoutUnit computedLogicalHeight = 0;
    if (styleLogicalHeight.isFixed()) {
        // HTML tables size as though CSS height includes border/padding, CSS tables do not.
        LayoutUnit borders = LayoutUnit();
        // FIXME: We cannot apply box-sizing: content-box on <table> which other browsers allow.
        if ((node() && isHTMLTableElement(node())) || style()->boxSizing() == BORDER_BOX) {
            LayoutUnit borderAndPaddingBefore = borderBefore() + (collapseBorders() ? LayoutUnit() : paddingBefore());
            LayoutUnit borderAndPaddingAfter = borderAfter() + (collapseBorders() ? LayoutUnit() : paddingAfter());
            borders = borderAndPaddingBefore + borderAndPaddingAfter;
        }
        computedLogicalHeight = styleLogicalHeight.value() - borders;
    } else if (styleLogicalHeight.isPercent())
        computedLogicalHeight = computePercentageLogicalHeight(styleLogicalHeight);
    else if (styleLogicalHeight.isViewportPercentage())
        computedLogicalHeight = minimumValueForLength(styleLogicalHeight, 0, view());
    else
        ASSERT_NOT_REACHED();
    return max<LayoutUnit>(0, computedLogicalHeight);
}

void RenderTable::layoutCaption(RenderTableCaption* caption)
{
    LayoutRect captionRect(caption->frameRect());

    if (caption->needsLayout()) {
        // The margins may not be available but ensure the caption is at least located beneath any previous sibling caption
        // so that it does not mistakenly think any floats in the previous caption intrude into it.
        caption->setLogicalLocation(LayoutPoint(caption->marginStart(), caption->marginBefore() + logicalHeight()));
        // If RenderTableCaption ever gets a layout() function, use it here.
        caption->layoutIfNeeded();
    }
    // Apply the margins to the location now that they are definitely available from layout
    caption->setLogicalLocation(LayoutPoint(caption->marginStart(), caption->marginBefore() + logicalHeight()));

    if (!selfNeedsLayout() && caption->checkForRepaintDuringLayout())
        caption->repaintDuringLayoutIfMoved(captionRect);

    setLogicalHeight(logicalHeight() + caption->logicalHeight() + caption->marginBefore() + caption->marginAfter());
}

void RenderTable::distributeExtraLogicalHeight(int extraLogicalHeight)
{
    if (extraLogicalHeight <= 0)
        return;

    // FIXME: Distribute the extra logical height between all table sections instead of giving it all to the first one.
    if (RenderTableSection* section = firstBody())
        extraLogicalHeight -= section->distributeExtraLogicalHeightToRows(extraLogicalHeight);

    // FIXME: We really would like to enable this ASSERT to ensure that all the extra space has been distributed.
    // However our current distribution algorithm does not round properly and thus we can have some remaining height.
    // ASSERT(!topSection() || !extraLogicalHeight);
}

void RenderTable::simplifiedNormalFlowLayout()
{
    for (RenderTableSection* section = topSection(); section; section = sectionBelow(section)) {
        section->layoutIfNeeded();
        section->computeOverflowFromCells();
    }
}

void RenderTable::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;
    ASSERT(needsLayout());

    if (simplifiedLayout())
        return;

    recalcSectionsIfNeeded();
    // FIXME: We should do this recalc lazily in borderStart/borderEnd so that we don't have to make sure
    // to call this before we call borderStart/borderEnd to avoid getting a stale value.
    recalcBordersInRowDirection();
        
    LayoutRepainter repainter(*this, checkForRepaintDuringLayout());
    LayoutStateMaintainer statePusher(view(), this, locationOffset(), hasTransform() || hasReflection() || style()->isFlippedBlocksWritingMode());

    setLogicalHeight(0);

    initMaxMarginValues();
    
    LayoutUnit oldLogicalWidth = logicalWidth();
    updateLogicalWidth();

    if (logicalWidth() != oldLogicalWidth) {
        for (unsigned i = 0; i < m_captions.size(); i++)
            m_captions[i]->setNeedsLayout(true, MarkOnlyThis);
    }
    // FIXME: The optimisation below doesn't work since the internal table
    // layout could have changed.  we need to add a flag to the table
    // layout that tells us if something has changed in the min max
    // calculations to do it correctly.
//     if ( oldWidth != width() || columns.size() + 1 != columnPos.size() )
    m_tableLayout->layout();

    LayoutUnit totalSectionLogicalHeight = 0;
    LayoutUnit oldTableLogicalTop = 0;
    for (unsigned i = 0; i < m_captions.size(); i++)
        oldTableLogicalTop += m_captions[i]->logicalHeight() + m_captions[i]->marginBefore() + m_captions[i]->marginAfter();

    bool collapsing = collapseBorders();
    // repeat header and footer on each page
    int headHeight = 0;
    int footHeight = 0;
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (child->isTableSection()) {
            RenderTableSection* section = toRenderTableSection(child);
            if (m_columnLogicalWidthChanged)
                section->setChildNeedsLayout(true, MarkOnlyThis);
            section->layoutIfNeeded();
            int rowHeight = section->calcRowLogicalHeight();
            if (child == m_head) {
                headHeight = rowHeight;
            } else if (child == m_foot) {
                footHeight = rowHeight;
            }
            totalSectionLogicalHeight += rowHeight;
            if (collapsing)
                section->recalcOuterBorder();
            ASSERT(!section->needsLayout());
        } else if (child->isRenderTableCol()) {
            child->layoutIfNeeded();
            ASSERT(!child->needsLayout());
        }
    }

    // If any table section moved vertically, we will just repaint everything from that
    // section down (it is quite unlikely that any of the following sections
    // did not shift).
    bool sectionMoved = false;
    LayoutUnit movedSectionLogicalTop = 0;

    // FIXME: Collapse caption margin.
    if (!m_captions.isEmpty()) {
        for (unsigned i = 0; i < m_captions.size(); i++) {
            if (m_captions[i]->style()->captionSide() == CAPBOTTOM)
                continue;
            layoutCaption(m_captions[i]);
        }
        if (logicalHeight() != oldTableLogicalTop) {
            sectionMoved = true;
            movedSectionLogicalTop = min(logicalHeight(), oldTableLogicalTop);
        }
    }

    LayoutUnit borderAndPaddingBefore = borderBefore() + (collapsing ? LayoutUnit() : paddingBefore());
    LayoutUnit borderAndPaddingAfter = borderAfter() + (collapsing ? LayoutUnit() : paddingAfter());

    setLogicalHeight(logicalHeight() + borderAndPaddingBefore);

    if (!isOutOfFlowPositioned())
        updateLogicalHeight();

    LayoutUnit computedLogicalHeight = 0;
    
    Length logicalHeightLength = style()->logicalHeight();
    if (logicalHeightLength.isSpecified() && logicalHeightLength.isPositive())
        computedLogicalHeight = convertStyleLogicalHeightToComputedHeight(logicalHeightLength);
    
    Length logicalMaxHeightLength = style()->logicalMaxHeight();
    if (logicalMaxHeightLength.isSpecified() && !logicalMaxHeightLength.isNegative()) {
        LayoutUnit computedMaxLogicalHeight = convertStyleLogicalHeightToComputedHeight(logicalMaxHeightLength);
        computedLogicalHeight = min(computedLogicalHeight, computedMaxLogicalHeight);
    }

    Length logicalMinHeightLength = style()->logicalMinHeight();
    if (logicalMinHeightLength.isSpecified() && !logicalMinHeightLength.isNegative()) {
        LayoutUnit computedMinLogicalHeight = convertStyleLogicalHeightToComputedHeight(logicalMinHeightLength);
        computedLogicalHeight = max(computedLogicalHeight, computedMinLogicalHeight);
    }

    distributeExtraLogicalHeight(floorToInt(computedLogicalHeight - totalSectionLogicalHeight));

    for (RenderTableSection* section = topSection(); section; section = sectionBelow(section))
        section->layoutRows(headHeight, footHeight);

    if (!topSection() && computedLogicalHeight > totalSectionLogicalHeight && !document()->inQuirksMode()) {
        // Completely empty tables (with no sections or anything) should at least honor specified height
        // in strict mode.
        setLogicalHeight(logicalHeight() + computedLogicalHeight);
    }

    LayoutUnit sectionLogicalLeft = style()->isLeftToRightDirection() ? borderStart() : borderEnd();
    if (!collapsing)
        sectionLogicalLeft += style()->isLeftToRightDirection() ? paddingStart() : paddingEnd();

    // position the table sections
    RenderTableSection* section = topSection();
    while (section) {
        if (!sectionMoved && section->logicalTop() != logicalHeight()) {
            sectionMoved = true;
            movedSectionLogicalTop = min(logicalHeight(), section->logicalTop()) + (style()->isHorizontalWritingMode() ? section->visualOverflowRect().y() : section->visualOverflowRect().x());
        }
        section->setLogicalLocation(LayoutPoint(sectionLogicalLeft, logicalHeight()));

        setLogicalHeight(logicalHeight() + section->logicalHeight());
        section = sectionBelow(section);
    }

    setLogicalHeight(logicalHeight() + borderAndPaddingAfter);

    for (unsigned i = 0; i < m_captions.size(); i++) {
        if (m_captions[i]->style()->captionSide() != CAPBOTTOM)
            continue;
        layoutCaption(m_captions[i]);
    }

    if (isOutOfFlowPositioned())
        updateLogicalHeight();

    // table can be containing block of positioned elements.
    // FIXME: Only pass true if width or height changed.
    layoutPositionedObjects(true);

    updateLayerTransform();

    // Layout was changed, so probably borders too.
    invalidateCollapsedBorders();

    computeOverflow(clientLogicalBottom());

    statePusher.pop();

    if (view()->layoutState()->pageLogicalHeight())
        setPageLogicalOffset(view()->layoutState()->pageLogicalOffset(this, logicalTop()));

    bool didFullRepaint = repainter.repaintAfterLayout();
    // Repaint with our new bounds if they are different from our old bounds.
    if (!didFullRepaint && sectionMoved) {
        if (style()->isHorizontalWritingMode())
            repaintRectangle(LayoutRect(visualOverflowRect().x(), movedSectionLogicalTop, visualOverflowRect().width(), visualOverflowRect().maxY() - movedSectionLogicalTop));
        else
            repaintRectangle(LayoutRect(movedSectionLogicalTop, visualOverflowRect().y(), visualOverflowRect().maxX() - movedSectionLogicalTop, visualOverflowRect().height()));
    }

    m_columnLogicalWidthChanged = false;
    setNeedsLayout(false);
}

// Collect all the unique border values that we want to paint in a sorted list.
void RenderTable::recalcCollapsedBorders()
{
    if (m_collapsedBordersValid)
        return;
    m_collapsedBordersValid = true;
    m_collapsedBorders.clear();
    for (RenderObject* section = firstChild(); section; section = section->nextSibling()) {
        if (!section->isTableSection())
            continue;
        for (RenderObject* row = section->firstChild(); row; row = row->nextSibling()) {
            if (!row->isTableRow())
                continue;
            for (RenderObject* cell = row->firstChild(); cell; cell = cell->nextSibling()) {
                if (!cell->isTableCell())
                    continue;
                ASSERT(toRenderTableCell(cell)->table() == this);
                toRenderTableCell(cell)->collectBorderValues(m_collapsedBorders);
            }
        }
    }
    RenderTableCell::sortBorderValues(m_collapsedBorders);
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
        if (borderOverflowRect != pixelSnappedBorderBoxRect()) {
            addLayoutOverflow(borderOverflowRect);
            addVisualOverflow(borderOverflowRect);
        }
    }

    // Add overflow from our caption.
    for (unsigned i = 0; i < m_captions.size(); i++) 
        addOverflowFromChild(m_captions[i]);

    // Add overflow from our sections.
    for (RenderTableSection* section = topSection(); section; section = sectionBelow(section))
        addOverflowFromChild(section);
}

void RenderTable::paint(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    LayoutPoint adjustedPaintOffset = paintOffset + location();

    PaintPhase paintPhase = paintInfo.phase;

    if (!isRoot()) {
        LayoutRect overflowBox = visualOverflowRect();
        flipForWritingMode(overflowBox);
        overflowBox.inflate(maximalOutlineSize(paintInfo.phase));
        overflowBox.moveBy(adjustedPaintOffset);
        if (!overflowBox.intersects(paintInfo.rect))
            return;
    }

    bool pushedClip = pushContentsClip(paintInfo, adjustedPaintOffset);
    paintObject(paintInfo, adjustedPaintOffset);
    if (pushedClip)
        popContentsClip(paintInfo, paintPhase, adjustedPaintOffset);
}

void RenderTable::paintObject(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    PaintPhase paintPhase = paintInfo.phase;
    if ((paintPhase == PaintPhaseBlockBackground || paintPhase == PaintPhaseChildBlockBackground) && hasBoxDecorations() && style()->visibility() == VISIBLE)
        paintBoxDecorations(paintInfo, paintOffset);

    if (paintPhase == PaintPhaseMask) {
        paintMask(paintInfo, paintOffset);
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
    info.updateSubtreePaintRootForChildren(this);

    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (child->isBox() && !toRenderBox(child)->hasSelfPaintingLayer() && (child->isTableSection() || child->isTableCaption())) {
            LayoutPoint childPoint = flipForWritingModeForChild(toRenderBox(child), paintOffset);
            child->paint(info, childPoint);
        }
    }
    
    if (collapseBorders() && paintPhase == PaintPhaseChildBlockBackground && style()->visibility() == VISIBLE) {
        recalcCollapsedBorders();
        // Using our cached sorted styles, we then do individual passes,
        // painting each style of border from lowest precedence to highest precedence.
        info.phase = PaintPhaseCollapsedTableBorders;
        size_t count = m_collapsedBorders.size();
        for (size_t i = 0; i < count; ++i) {
            m_currentBorder = &m_collapsedBorders[i];
            for (RenderTableSection* section = bottomSection(); section; section = sectionAbove(section)) {
                LayoutPoint childPoint = flipForWritingModeForChild(section, paintOffset);
                section->paint(info, childPoint);
            }
        }
        m_currentBorder = 0;
    }

    // Paint outline.
    if ((paintPhase == PaintPhaseOutline || paintPhase == PaintPhaseSelfOutline) && hasOutline() && style()->visibility() == VISIBLE)
        paintOutline(paintInfo, LayoutRect(paintOffset, size()));
}

void RenderTable::subtractCaptionRect(LayoutRect& rect) const
{
    for (unsigned i = 0; i < m_captions.size(); i++) {
        LayoutUnit captionLogicalHeight = m_captions[i]->logicalHeight() + m_captions[i]->marginBefore() + m_captions[i]->marginAfter();
        bool captionIsBefore = (m_captions[i]->style()->captionSide() != CAPBOTTOM) ^ style()->isFlippedBlocksWritingMode();
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
}

void RenderTable::paintBoxDecorations(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (!paintInfo.shouldPaintWithinRoot(this))
        return;

    LayoutRect rect(paintOffset, size());
    subtractCaptionRect(rect);

    BackgroundBleedAvoidance bleedAvoidance = determineBackgroundBleedAvoidance(paintInfo.context);
    if (!boxShadowShouldBeAppliedToBackground(bleedAvoidance))
        paintBoxShadow(paintInfo, rect, style(), Normal);
    paintBackground(paintInfo, rect, bleedAvoidance);
    paintBoxShadow(paintInfo, rect, style(), Inset);

    if (style()->hasBorder() && !collapseBorders())
        paintBorder(paintInfo, rect, style());
}

void RenderTable::paintMask(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (style()->visibility() != VISIBLE || paintInfo.phase != PaintPhaseMask)
        return;

    LayoutRect rect(paintOffset, size());
    subtractCaptionRect(rect);

    paintMaskImages(paintInfo, rect);
}

void RenderTable::computeIntrinsicLogicalWidths(LayoutUnit& minWidth, LayoutUnit& maxWidth) const
{
    recalcSectionsIfNeeded();
    // FIXME: Do the recalc in borderStart/borderEnd and make those const_cast this call.
    // Then m_borderStart/m_borderEnd will be transparent a cache and it removes the possibility
    // of reading out stale values.
    const_cast<RenderTable*>(this)->recalcBordersInRowDirection();
    // FIXME: Restructure the table layout code so that we can make this method const.
    const_cast<RenderTable*>(this)->m_tableLayout->computeIntrinsicLogicalWidths(minWidth, maxWidth);

    // FIXME: We should include captions widths here like we do in computePreferredLogicalWidths.
}

void RenderTable::computePreferredLogicalWidths()
{
    ASSERT(preferredLogicalWidthsDirty());

    computeIntrinsicLogicalWidths(m_minPreferredLogicalWidth, m_maxPreferredLogicalWidth);

    int bordersPaddingAndSpacing = bordersPaddingAndSpacingInRowDirection();
    m_minPreferredLogicalWidth += bordersPaddingAndSpacing;
    m_maxPreferredLogicalWidth += bordersPaddingAndSpacing;

    m_tableLayout->applyPreferredLogicalWidthQuirks(m_minPreferredLogicalWidth, m_maxPreferredLogicalWidth);

    for (unsigned i = 0; i < m_captions.size(); i++)
        m_minPreferredLogicalWidth = max(m_minPreferredLogicalWidth, m_captions[i]->minPreferredLogicalWidth());

    RenderStyle* styleToUse = style();
    // FIXME: This should probably be checking for isSpecified since you should be able to use percentage, calc or viewport relative values for min-width.
    if (styleToUse->logicalMinWidth().isFixed() && styleToUse->logicalMinWidth().value() > 0) {
        m_maxPreferredLogicalWidth = std::max(m_maxPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(styleToUse->logicalMinWidth().value()));
        m_minPreferredLogicalWidth = std::max(m_minPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(styleToUse->logicalMinWidth().value()));
    }

    // FIXME: This should probably be checking for isSpecified since you should be able to use percentage, calc or viewport relative values for maxWidth.
    if (styleToUse->logicalMaxWidth().isFixed()) {
        m_maxPreferredLogicalWidth = std::min(m_maxPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(styleToUse->logicalMaxWidth().value()));
        m_minPreferredLogicalWidth = std::min(m_minPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(styleToUse->logicalMaxWidth().value()));
    }

    // FIXME: We should be adding borderAndPaddingLogicalWidth here, but m_tableLayout->computePreferredLogicalWidths already does,
    // so a bunch of tests break doing this naively.
    setPreferredLogicalWidthsDirty(false);
}

RenderTableSection* RenderTable::topNonEmptySection() const
{
    RenderTableSection* section = topSection();
    if (section && !section->numRows())
        section = sectionBelow(section, SkipEmptySections);
    return section;
}

void RenderTable::splitColumn(unsigned position, unsigned firstSpan)
{
    // We split the column at "position", taking "firstSpan" cells from the span.
    ASSERT(m_columns[position].span > firstSpan);
    m_columns.insert(position, ColumnStruct(firstSpan));
    m_columns[position + 1].span -= firstSpan;

    // Propagate the change in our columns representation to the sections that don't need
    // cell recalc. If they do, they will be synced up directly with m_columns later.
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (!child->isTableSection())
            continue;

        RenderTableSection* section = toRenderTableSection(child);
        if (section->needsCellRecalc())
            continue;

        section->splitColumn(position, firstSpan);
    }

    m_columnPos.grow(numEffCols() + 1);
}

void RenderTable::appendColumn(unsigned span)
{
    unsigned newColumnIndex = m_columns.size();
    m_columns.append(ColumnStruct(span));

    // Propagate the change in our columns representation to the sections that don't need
    // cell recalc. If they do, they will be synced up directly with m_columns later.
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (!child->isTableSection())
            continue;

        RenderTableSection* section = toRenderTableSection(child);
        if (section->needsCellRecalc())
            continue;

        section->appendColumn(newColumnIndex);
    }

    m_columnPos.grow(numEffCols() + 1);
}

RenderTableCol* RenderTable::firstColumn() const
{
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (child->isRenderTableCol())
            return toRenderTableCol(child);

        // We allow only table-captions before columns or column-groups.
        if (!child->isTableCaption())
            return 0;
    }

    return 0;
}

void RenderTable::updateColumnCache() const
{
    ASSERT(m_hasColElements);
    ASSERT(m_columnRenderers.isEmpty());
    ASSERT(!m_columnRenderersValid);

    for (RenderTableCol* columnRenderer = firstColumn(); columnRenderer; columnRenderer = columnRenderer->nextColumn()) {
        if (columnRenderer->isTableColumnGroupWithColumnChildren())
            continue;
        m_columnRenderers.append(columnRenderer);
    }
    m_columnRenderersValid = true;
}

RenderTableCol* RenderTable::slowColElement(unsigned col, bool* startEdge, bool* endEdge) const
{
    ASSERT(m_hasColElements);

    if (!m_columnRenderersValid)
        updateColumnCache();

    unsigned columnCount = 0;
    for (unsigned i = 0; i < m_columnRenderers.size(); i++) {
        RenderTableCol* columnRenderer = m_columnRenderers[i];
        unsigned span = columnRenderer->span();
        unsigned startCol = columnCount;
        ASSERT(span >= 1);
        unsigned endCol = columnCount + span - 1;
        columnCount += span;
        if (columnCount > col) {
            if (startEdge)
                *startEdge = startCol == col;
            if (endEdge)
                *endEdge = endCol == col;
            return columnRenderer;
        }
    }
    return 0;
}

void RenderTable::recalcSections() const
{
    ASSERT(m_needsSectionRecalc);

    m_head = 0;
    m_foot = 0;
    m_firstBody = 0;
    m_hasColElements = false;

    // We need to get valid pointers to caption, head, foot and first body again
    RenderObject* nextSibling;
    for (RenderObject* child = firstChild(); child; child = nextSibling) {
        nextSibling = child->nextSibling();
        switch (child->style()->display()) {
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
    unsigned maxCols = 0;
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (child->isTableSection()) {
            RenderTableSection* section = toRenderTableSection(child);
            unsigned sectionCols = section->numColumns();
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
    if (!collapseBorders())
        return RenderBlock::borderStart();

    // Determined by the first cell of the first row. See the CSS 2.1 spec, section 17.6.2.
    if (!numEffCols())
        return 0;

    unsigned borderWidth = 0;

    const BorderValue& tableStartBorder = style()->borderStart();
    if (tableStartBorder.style() == BHIDDEN)
        return 0;
    if (tableStartBorder.style() > BHIDDEN)
        borderWidth = tableStartBorder.width();

    if (RenderTableCol* column = colElement(0)) {
        // FIXME: We don't account for direction on columns and column groups.
        const BorderValue& columnAdjoiningBorder = column->style()->borderStart();
        if (columnAdjoiningBorder.style() == BHIDDEN)
            return 0;
        if (columnAdjoiningBorder.style() > BHIDDEN)
            borderWidth = max(borderWidth, columnAdjoiningBorder.width());
        // FIXME: This logic doesn't properly account for the first column in the first column-group case.
    }

    if (const RenderTableSection* topNonEmptySection = this->topNonEmptySection()) {
        const BorderValue& sectionAdjoiningBorder = topNonEmptySection->borderAdjoiningTableStart();
        if (sectionAdjoiningBorder.style() == BHIDDEN)
            return 0;

        if (sectionAdjoiningBorder.style() > BHIDDEN)
            borderWidth = max(borderWidth, sectionAdjoiningBorder.width());

        if (const RenderTableCell* adjoiningStartCell = topNonEmptySection->firstRowCellAdjoiningTableStart()) {
            // FIXME: Make this work with perpendicular and flipped cells.
            const BorderValue& startCellAdjoiningBorder = adjoiningStartCell->borderAdjoiningTableStart();
            if (startCellAdjoiningBorder.style() == BHIDDEN)
                return 0;

            const BorderValue& firstRowAdjoiningBorder = adjoiningStartCell->row()->borderAdjoiningTableStart();
            if (firstRowAdjoiningBorder.style() == BHIDDEN)
                return 0;

            if (startCellAdjoiningBorder.style() > BHIDDEN)
                borderWidth = max(borderWidth, startCellAdjoiningBorder.width());
            if (firstRowAdjoiningBorder.style() > BHIDDEN)
                borderWidth = max(borderWidth, firstRowAdjoiningBorder.width());
        }
    }
    return (borderWidth + (style()->isLeftToRightDirection() ? 0 : 1)) / 2;
}

int RenderTable::calcBorderEnd() const
{
    if (!collapseBorders())
        return RenderBlock::borderEnd();

    // Determined by the last cell of the first row. See the CSS 2.1 spec, section 17.6.2.
    if (!numEffCols())
        return 0;

    unsigned borderWidth = 0;

    const BorderValue& tableEndBorder = style()->borderEnd();
    if (tableEndBorder.style() == BHIDDEN)
        return 0;
    if (tableEndBorder.style() > BHIDDEN)
        borderWidth = tableEndBorder.width();

    unsigned endColumn = numEffCols() - 1;
    if (RenderTableCol* column = colElement(endColumn)) {
        // FIXME: We don't account for direction on columns and column groups.
        const BorderValue& columnAdjoiningBorder = column->style()->borderEnd();
        if (columnAdjoiningBorder.style() == BHIDDEN)
            return 0;
        if (columnAdjoiningBorder.style() > BHIDDEN)
            borderWidth = max(borderWidth, columnAdjoiningBorder.width());
        // FIXME: This logic doesn't properly account for the last column in the last column-group case.
    }

    if (const RenderTableSection* topNonEmptySection = this->topNonEmptySection()) {
        const BorderValue& sectionAdjoiningBorder = topNonEmptySection->borderAdjoiningTableEnd();
        if (sectionAdjoiningBorder.style() == BHIDDEN)
            return 0;

        if (sectionAdjoiningBorder.style() > BHIDDEN)
            borderWidth = max(borderWidth, sectionAdjoiningBorder.width());

        if (const RenderTableCell* adjoiningEndCell = topNonEmptySection->firstRowCellAdjoiningTableEnd()) {
            // FIXME: Make this work with perpendicular and flipped cells.
            const BorderValue& endCellAdjoiningBorder = adjoiningEndCell->borderAdjoiningTableEnd();
            if (endCellAdjoiningBorder.style() == BHIDDEN)
                return 0;

            const BorderValue& firstRowAdjoiningBorder = adjoiningEndCell->row()->borderAdjoiningTableEnd();
            if (firstRowAdjoiningBorder.style() == BHIDDEN)
                return 0;

            if (endCellAdjoiningBorder.style() > BHIDDEN)
                borderWidth = max(borderWidth, endCellAdjoiningBorder.width());
            if (firstRowAdjoiningBorder.style() > BHIDDEN)
                borderWidth = max(borderWidth, firstRowAdjoiningBorder.width());
        }
    }
    return (borderWidth + (style()->isLeftToRightDirection() ? 1 : 0)) / 2;
}

void RenderTable::recalcBordersInRowDirection()
{
    // FIXME: We need to compute the collapsed before / after borders in the same fashion.
    m_borderStart = calcBorderStart();
    m_borderEnd = calcBorderEnd();
}

int RenderTable::borderBefore() const
{
    if (collapseBorders()) {
        recalcSectionsIfNeeded();
        return outerBorderBefore();
    }
    return RenderBlock::borderBefore();
}

int RenderTable::borderAfter() const
{
    if (collapseBorders()) {
        recalcSectionsIfNeeded();
        return outerBorderAfter();
    }
    return RenderBlock::borderAfter();
}

int RenderTable::outerBorderBefore() const
{
    if (!collapseBorders())
        return 0;
    int borderWidth = 0;
    if (RenderTableSection* topSection = this->topSection()) {
        borderWidth = topSection->outerBorderBefore();
        if (borderWidth < 0)
            return 0;   // Overridden by hidden
    }
    const BorderValue& tb = style()->borderBefore();
    if (tb.style() == BHIDDEN)
        return 0;
    if (tb.style() > BHIDDEN)
        borderWidth = max<int>(borderWidth, tb.width() / 2);
    return borderWidth;
}

int RenderTable::outerBorderAfter() const
{
    if (!collapseBorders())
        return 0;
    int borderWidth = 0;

    if (RenderTableSection* section = bottomSection()) {
        borderWidth = section->outerBorderAfter();
        if (borderWidth < 0)
            return 0; // Overridden by hidden
    }
    const BorderValue& tb = style()->borderAfter();
    if (tb.style() == BHIDDEN)
        return 0;
    if (tb.style() > BHIDDEN)
        borderWidth = max<int>(borderWidth, (tb.width() + 1) / 2);
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
    for (RenderTableSection* section = topSection(); section; section = sectionBelow(section)) {
        int sw = section->outerBorderStart();
        if (sw < 0)
            continue;
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
    for (RenderTableSection* section = topSection(); section; section = sectionBelow(section)) {
        int sw = section->outerBorderEnd();
        if (sw < 0)
            continue;
        allHidden = false;
        borderWidth = max(borderWidth, sw);
    }
    if (allHidden)
        return 0;

    return borderWidth;
}

RenderTableSection* RenderTable::sectionAbove(const RenderTableSection* section, SkipEmptySectionsValue skipEmptySections) const
{
    recalcSectionsIfNeeded();

    if (section == m_head)
        return 0;

    RenderObject* prevSection = section == m_foot ? lastChild() : section->previousSibling();
    while (prevSection) {
        if (prevSection->isTableSection() && prevSection != m_head && prevSection != m_foot && (skipEmptySections == DoNotSkipEmptySections || toRenderTableSection(prevSection)->numRows()))
            break;
        prevSection = prevSection->previousSibling();
    }
    if (!prevSection && m_head && (skipEmptySections == DoNotSkipEmptySections || m_head->numRows()))
        prevSection = m_head;
    return toRenderTableSection(prevSection);
}

RenderTableSection* RenderTable::sectionBelow(const RenderTableSection* section, SkipEmptySectionsValue skipEmptySections) const
{
    recalcSectionsIfNeeded();

    if (section == m_foot)
        return 0;

    RenderObject* nextSection = section == m_head ? firstChild() : section->nextSibling();
    while (nextSection) {
        if (nextSection->isTableSection() && nextSection != m_head && nextSection != m_foot && (skipEmptySections  == DoNotSkipEmptySections || toRenderTableSection(nextSection)->numRows()))
            break;
        nextSection = nextSection->nextSibling();
    }
    if (!nextSection && m_foot && (skipEmptySections == DoNotSkipEmptySections || m_foot->numRows()))
        nextSection = m_foot;
    return toRenderTableSection(nextSection);
}

RenderTableSection* RenderTable::bottomSection() const
{
    recalcSectionsIfNeeded();

    if (m_foot)
        return m_foot;

    for (RenderObject* child = lastChild(); child; child = child->previousSibling()) {
        if (child->isTableSection())
            return toRenderTableSection(child);
    }

    return 0;
}

RenderTableCell* RenderTable::cellAbove(const RenderTableCell* cell) const
{
    recalcSectionsIfNeeded();

    // Find the section and row to look in
    unsigned r = cell->rowIndex();
    RenderTableSection* section = 0;
    unsigned rAbove = 0;
    if (r > 0) {
        // cell is not in the first row, so use the above row in its own section
        section = cell->section();
        rAbove = r - 1;
    } else {
        section = sectionAbove(cell->section(), SkipEmptySections);
        if (section) {
            ASSERT(section->numRows());
            rAbove = section->numRows() - 1;
        }
    }

    // Look up the cell in the section's grid, which requires effective col index
    if (section) {
        unsigned effCol = colToEffCol(cell->col());
        RenderTableSection::CellStruct& aboveCell = section->cellAt(rAbove, effCol);
        return aboveCell.primaryCell();
    } else
        return 0;
}

RenderTableCell* RenderTable::cellBelow(const RenderTableCell* cell) const
{
    recalcSectionsIfNeeded();

    // Find the section and row to look in
    unsigned r = cell->rowIndex() + cell->rowSpan() - 1;
    RenderTableSection* section = 0;
    unsigned rBelow = 0;
    if (r < cell->section()->numRows() - 1) {
        // The cell is not in the last row, so use the next row in the section.
        section = cell->section();
        rBelow = r + 1;
    } else {
        section = sectionBelow(cell->section(), SkipEmptySections);
        if (section)
            rBelow = 0;
    }

    // Look up the cell in the section's grid, which requires effective col index
    if (section) {
        unsigned effCol = colToEffCol(cell->col());
        RenderTableSection::CellStruct& belowCell = section->cellAt(rBelow, effCol);
        return belowCell.primaryCell();
    } else
        return 0;
}

RenderTableCell* RenderTable::cellBefore(const RenderTableCell* cell) const
{
    recalcSectionsIfNeeded();

    RenderTableSection* section = cell->section();
    unsigned effCol = colToEffCol(cell->col());
    if (!effCol)
        return 0;
    
    // If we hit a colspan back up to a real cell.
    RenderTableSection::CellStruct& prevCell = section->cellAt(cell->rowIndex(), effCol - 1);
    return prevCell.primaryCell();
}

RenderTableCell* RenderTable::cellAfter(const RenderTableCell* cell) const
{
    recalcSectionsIfNeeded();

    unsigned effCol = colToEffCol(cell->col() + cell->colSpan());
    if (effCol >= numEffCols())
        return 0;
    return cell->section()->primaryCellAt(cell->rowIndex(), effCol);
}

RenderBlock* RenderTable::firstLineBlock() const
{
    return 0;
}

void RenderTable::updateFirstLetter()
{
}

int RenderTable::baselinePosition(FontBaseline baselineType, bool firstLine, LineDirectionMode direction, LinePositionMode linePositionMode) const
{
    LayoutUnit baseline = firstLineBoxBaseline();
    if (baseline != -1)
        return baseline;

    return RenderBox::baselinePosition(baselineType, firstLine, direction, linePositionMode);
}

int RenderTable::inlineBlockBaseline(LineDirectionMode) const
{
    // Tables are skipped when computing an inline-block's baseline.
    return -1;
}

int RenderTable::firstLineBoxBaseline() const
{
    // The baseline of a 'table' is the same as the 'inline-table' baseline per CSS 3 Flexbox (CSS 2.1
    // doesn't define the baseline of a 'table' only an 'inline-table').
    // This is also needed to properly determine the baseline of a cell if it has a table child.

    if (isWritingModeRoot())
        return -1;

    recalcSectionsIfNeeded();

    const RenderTableSection* topNonEmptySection = this->topNonEmptySection();
    if (!topNonEmptySection)
        return -1;

    int baseline = topNonEmptySection->firstLineBoxBaseline();
    if (baseline > 0)
        return topNonEmptySection->logicalTop() + baseline;

    // FIXME: A table row always has a baseline per CSS 2.1. Will this return the right value?
    return -1;
}

LayoutRect RenderTable::overflowClipRect(const LayoutPoint& location, RenderRegion* region, OverlayScrollbarSizeRelevancy relevancy, PaintPhase phase)
{
    LayoutRect rect;
    // Don't clip out the table's side of the collapsed borders if we're in the paint phase that will ask the sections to paint them.
    // Likewise, if we're self-painting we avoid clipping them out as the clip rect that will be passed down to child layers from RenderLayer will do that instead.
    if (phase == PaintPhaseChildBlockBackgrounds || layer()->isSelfPaintingLayer()) {
        rect = borderBoxRectInRegion(region);
        rect.setLocation(location + rect.location());
    } else
        rect = RenderBox::overflowClipRect(location, region, relevancy);
    
    // If we have a caption, expand the clip to include the caption.
    // FIXME: Technically this is wrong, but it's virtually impossible to fix this
    // for real until captions have been re-written.
    // FIXME: This code assumes (like all our other caption code) that only top/bottom are
    // supported.  When we actually support left/right and stop mapping them to top/bottom,
    // we might have to hack this code first (depending on what order we do these bug fixes in).
    if (!m_captions.isEmpty()) {
        if (style()->isHorizontalWritingMode()) {
            rect.setHeight(height());
            rect.setY(location.y());
        } else {
            rect.setWidth(width());
            rect.setX(location.x());
        }
    }

    return rect;
}

bool RenderTable::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction action)
{
    LayoutPoint adjustedLocation = accumulatedOffset + location();

    // Check kids first.
    if (!hasOverflowClip() || locationInContainer.intersects(overflowClipRect(adjustedLocation, locationInContainer.region()))) {
        for (RenderObject* child = lastChild(); child; child = child->previousSibling()) {
            if (child->isBox() && !toRenderBox(child)->hasSelfPaintingLayer() && (child->isTableSection() || child->isTableCaption())) {
                LayoutPoint childPoint = flipForWritingModeForChild(toRenderBox(child), adjustedLocation);
                if (child->nodeAtPoint(request, result, locationInContainer, childPoint, action)) {
                    updateHitTestResult(result, toLayoutPoint(locationInContainer.point() - childPoint));
                    return true;
                }
            }
        }
    }

    // Check our bounds next.
    LayoutRect boundsRect(adjustedLocation, size());
    if (visibleToHitTesting() && (action == HitTestBlockBackground || action == HitTestChildBlockBackground) && locationInContainer.intersects(boundsRect)) {
        updateHitTestResult(result, flipForWritingMode(locationInContainer.point() - toLayoutSize(adjustedLocation)));
        if (!result.addNodeToRectBasedTestResult(node(), request, locationInContainer, boundsRect))
            return true;
    }

    return false;
}

RenderTable* RenderTable::createAnonymousWithParentRenderer(const RenderObject* parent)
{
    RefPtr<RenderStyle> newStyle = RenderStyle::createAnonymousStyleWithDisplay(parent->style(), TABLE);
    RenderTable* newTable = new (parent->renderArena()) RenderTable(0);
    newTable->setDocumentForAnonymous(parent->document());
    newTable->setStyle(newStyle.release());
    return newTable;
}

const BorderValue& RenderTable::tableStartBorderAdjoiningCell(const RenderTableCell* cell) const
{
    ASSERT(cell->isFirstOrLastCellInRow());
    if (hasSameDirectionAs(cell->row()))
        return style()->borderStart();

    return style()->borderEnd();
}

const BorderValue& RenderTable::tableEndBorderAdjoiningCell(const RenderTableCell* cell) const
{
    ASSERT(cell->isFirstOrLastCellInRow());
    if (hasSameDirectionAs(cell->row()))
        return style()->borderEnd();

    return style()->borderStart();
}

}
