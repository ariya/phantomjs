/*
 * This file is part of the render object implementation for KHTML.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003 Apple Computer, Inc.
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

#include "config.h"
#include "RenderDeprecatedFlexibleBox.h"

#include "FeatureObserver.h"
#include "Font.h"
#include "LayoutRepainter.h"
#include "RenderLayer.h"
#include "RenderView.h"
#include <wtf/StdLibExtras.h>
#include <wtf/unicode/CharacterNames.h>

using namespace std;

namespace WebCore {

class FlexBoxIterator {
public:
    FlexBoxIterator(RenderDeprecatedFlexibleBox* parent)
        : m_box(parent)
        , m_largestOrdinal(1)
    {
        if (m_box->style()->boxOrient() == HORIZONTAL && !m_box->style()->isLeftToRightDirection())
            m_forward = m_box->style()->boxDirection() != BNORMAL;
        else
            m_forward = m_box->style()->boxDirection() == BNORMAL;
        if (!m_forward) {
            // No choice, since we're going backwards, we have to find out the highest ordinal up front.
            RenderBox* child = m_box->firstChildBox();
            while (child) {
                if (child->style()->boxOrdinalGroup() > m_largestOrdinal)
                    m_largestOrdinal = child->style()->boxOrdinalGroup();
                child = child->nextSiblingBox();
            }
        }

        reset();
    }

    void reset()
    {
        m_currentChild = 0;
        m_ordinalIteration = -1;
    }

    RenderBox* first()
    {
        reset();
        return next();
    }

    RenderBox* next()
    {
        do {
            if (!m_currentChild) {
                ++m_ordinalIteration;

                if (!m_ordinalIteration)
                    m_currentOrdinal = m_forward ? 1 : m_largestOrdinal;
                else {
                    if (m_ordinalIteration >= m_ordinalValues.size() + 1)
                        return 0;

                    // Only copy+sort the values once per layout even if the iterator is reset.
                    if (static_cast<size_t>(m_ordinalValues.size()) != m_sortedOrdinalValues.size()) {
                        copyToVector(m_ordinalValues, m_sortedOrdinalValues);
                        sort(m_sortedOrdinalValues.begin(), m_sortedOrdinalValues.end());
                    }
                    m_currentOrdinal = m_forward ? m_sortedOrdinalValues[m_ordinalIteration - 1] : m_sortedOrdinalValues[m_sortedOrdinalValues.size() - m_ordinalIteration];
                }

                m_currentChild = m_forward ? m_box->firstChildBox() : m_box->lastChildBox();
            } else
                m_currentChild = m_forward ? m_currentChild->nextSiblingBox() : m_currentChild->previousSiblingBox();

            if (m_currentChild && notFirstOrdinalValue())
                m_ordinalValues.add(m_currentChild->style()->boxOrdinalGroup());
        } while (!m_currentChild || (!m_currentChild->isAnonymous()
                 && m_currentChild->style()->boxOrdinalGroup() != m_currentOrdinal));
        return m_currentChild;
    }

private:
    bool notFirstOrdinalValue()
    {
        unsigned int firstOrdinalValue = m_forward ? 1 : m_largestOrdinal;
        return m_currentOrdinal == firstOrdinalValue && m_currentChild->style()->boxOrdinalGroup() != firstOrdinalValue;
    }

    RenderDeprecatedFlexibleBox* m_box;
    RenderBox* m_currentChild;
    bool m_forward;
    unsigned int m_currentOrdinal;
    unsigned int m_largestOrdinal;
    HashSet<unsigned int> m_ordinalValues;
    Vector<unsigned int> m_sortedOrdinalValues;
    int m_ordinalIteration;
};

RenderDeprecatedFlexibleBox::RenderDeprecatedFlexibleBox(Element* element)
    : RenderBlock(element)
{
    setChildrenInline(false); // All of our children must be block-level
    m_stretchingChildren = false;
    if (!isAnonymous()) {
        const KURL& url = document()->url();
        if (url.protocolIs("chrome"))
            FeatureObserver::observe(document(), FeatureObserver::DeprecatedFlexboxChrome);
        else if (url.protocolIs("chrome-extension"))
            FeatureObserver::observe(document(), FeatureObserver::DeprecatedFlexboxChromeExtension);
        else
            FeatureObserver::observe(document(), FeatureObserver::DeprecatedFlexboxWebContent);
    }
}

RenderDeprecatedFlexibleBox::~RenderDeprecatedFlexibleBox()
{
}

RenderDeprecatedFlexibleBox* RenderDeprecatedFlexibleBox::createAnonymous(Document* document)
{
    RenderDeprecatedFlexibleBox* renderer = new (document->renderArena()) RenderDeprecatedFlexibleBox(0);
    renderer->setDocumentForAnonymous(document);
    return renderer;
}

static LayoutUnit marginWidthForChild(RenderBox* child)
{
    // A margin basically has three types: fixed, percentage, and auto (variable).
    // Auto and percentage margins simply become 0 when computing min/max width.
    // Fixed margins can be added in as is.
    Length marginLeft = child->style()->marginLeft();
    Length marginRight = child->style()->marginRight();
    LayoutUnit margin = 0;
    if (marginLeft.isFixed())
        margin += marginLeft.value();
    if (marginRight.isFixed())
        margin += marginRight.value();
    return margin;
}

static bool childDoesNotAffectWidthOrFlexing(RenderObject* child)
{
    // Positioned children and collapsed children don't affect the min/max width.
    return child->isOutOfFlowPositioned() || child->style()->visibility() == COLLAPSE;
}

static LayoutUnit contentWidthForChild(RenderBox* child)
{
    if (child->hasOverrideWidth())
        return child->overrideLogicalContentWidth();
    return child->logicalWidth() - child->borderAndPaddingLogicalWidth();
}

static LayoutUnit contentHeightForChild(RenderBox* child)
{
    if (child->hasOverrideHeight())
        return child->overrideLogicalContentHeight();
    return child->logicalHeight() - child->borderAndPaddingLogicalHeight();
}

void RenderDeprecatedFlexibleBox::styleWillChange(StyleDifference diff, const RenderStyle* newStyle)
{
    RenderStyle* oldStyle = style();
    if (oldStyle && !oldStyle->lineClamp().isNone() && newStyle->lineClamp().isNone())
        clearLineClamp();

    RenderBlock::styleWillChange(diff, newStyle);
}

void RenderDeprecatedFlexibleBox::computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const
{
    if (hasMultipleLines() || isVertical()) {
        for (RenderBox* child = firstChildBox(); child; child = child->nextSiblingBox()) {
            if (childDoesNotAffectWidthOrFlexing(child))
                continue;

            LayoutUnit margin = marginWidthForChild(child);
            LayoutUnit width = child->minPreferredLogicalWidth() + margin;
            minLogicalWidth = max(width, minLogicalWidth);

            width = child->maxPreferredLogicalWidth() + margin;
            maxLogicalWidth = max(width, maxLogicalWidth);
        }
    } else {
        for (RenderBox* child = firstChildBox(); child; child = child->nextSiblingBox()) {
            if (childDoesNotAffectWidthOrFlexing(child))
                continue;

            LayoutUnit margin = marginWidthForChild(child);
            minLogicalWidth += child->minPreferredLogicalWidth() + margin;
            maxLogicalWidth += child->maxPreferredLogicalWidth() + margin;
        }
    }

    maxLogicalWidth = max(minLogicalWidth, maxLogicalWidth);

    LayoutUnit scrollbarWidth = instrinsicScrollbarLogicalWidth();
    maxLogicalWidth += scrollbarWidth;
    minLogicalWidth += scrollbarWidth;
}

void RenderDeprecatedFlexibleBox::computePreferredLogicalWidths()
{
    ASSERT(preferredLogicalWidthsDirty());

    m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth = 0;
    if (style()->width().isFixed() && style()->width().value() > 0)
        m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth = adjustContentBoxLogicalWidthForBoxSizing(style()->width().value());
    else
        computeIntrinsicLogicalWidths(m_minPreferredLogicalWidth, m_maxPreferredLogicalWidth);

    if (style()->minWidth().isFixed() && style()->minWidth().value() > 0) {
        m_maxPreferredLogicalWidth = max(m_maxPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(style()->minWidth().value()));
        m_minPreferredLogicalWidth = max(m_minPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(style()->minWidth().value()));
    }

    if (style()->maxWidth().isFixed()) {
        m_maxPreferredLogicalWidth = min(m_maxPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(style()->maxWidth().value()));
        m_minPreferredLogicalWidth = min(m_minPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(style()->maxWidth().value()));
    }

    LayoutUnit borderAndPadding = borderAndPaddingLogicalWidth();
    m_minPreferredLogicalWidth += borderAndPadding;
    m_maxPreferredLogicalWidth += borderAndPadding;

    setPreferredLogicalWidthsDirty(false);
}

// Use an inline capacity of 8, since flexbox containers usually have less than 8 children.
typedef Vector<LayoutRect, 8> ChildFrameRects;
typedef Vector<LayoutSize, 8> ChildLayoutDeltas;

static void appendChildFrameRects(RenderDeprecatedFlexibleBox* box, ChildFrameRects& childFrameRects)
{
    FlexBoxIterator iterator(box);
    for (RenderBox* child = iterator.first(); child; child = iterator.next()) {
        if (!child->isOutOfFlowPositioned())
            childFrameRects.append(child->frameRect());
    }
}

static void appendChildLayoutDeltas(RenderDeprecatedFlexibleBox* box, ChildLayoutDeltas& childLayoutDeltas)
{
    FlexBoxIterator iterator(box);
    for (RenderBox* child = iterator.first(); child; child = iterator.next()) {
        if (!child->isOutOfFlowPositioned())
            childLayoutDeltas.append(LayoutSize());
    }
}

static void repaintChildrenDuringLayoutIfMoved(RenderDeprecatedFlexibleBox* box, const ChildFrameRects& oldChildRects)
{
    size_t childIndex = 0;
    FlexBoxIterator iterator(box);
    for (RenderBox* child = iterator.first(); child; child = iterator.next()) {
        if (child->isOutOfFlowPositioned())
            continue;

        // If the child moved, we have to repaint it as well as any floating/positioned
        // descendants. An exception is if we need a layout. In this case, we know we're going to
        // repaint ourselves (and the child) anyway.
        if (!box->selfNeedsLayout() && child->checkForRepaintDuringLayout())
            child->repaintDuringLayoutIfMoved(oldChildRects[childIndex]);
        
        ++childIndex;
    }
    ASSERT(childIndex == oldChildRects.size());
}

void RenderDeprecatedFlexibleBox::layoutBlock(bool relayoutChildren, LayoutUnit)
{
    ASSERT(needsLayout());

    if (!relayoutChildren && simplifiedLayout())
        return;

    LayoutRepainter repainter(*this, checkForRepaintDuringLayout());
    LayoutStateMaintainer statePusher(view(), this, locationOffset(), hasTransform() || hasReflection() || style()->isFlippedBlocksWritingMode());

    // Regions changing widths can force us to relayout our children.
    RenderFlowThread* flowThread = flowThreadContainingBlock();
    if (logicalWidthChangedInRegions(flowThread))
        relayoutChildren = true;
    if (updateRegionsAndShapesBeforeChildLayout(flowThread))
        relayoutChildren = true;

    LayoutSize previousSize = size();

    updateLogicalWidth();
    updateLogicalHeight();

    if (previousSize != size()
        || (parent()->isDeprecatedFlexibleBox() && parent()->style()->boxOrient() == HORIZONTAL
        && parent()->style()->boxAlign() == BSTRETCH))
        relayoutChildren = true;

    setHeight(0);

    m_stretchingChildren = false;

    initMaxMarginValues();

#if !ASSERT_DISABLED
    LayoutSize oldLayoutDelta = view()->layoutDelta();
#endif

    ChildFrameRects oldChildRects;
    appendChildFrameRects(this, oldChildRects);
    
    dirtyForLayoutFromPercentageHeightDescendants();

    if (isHorizontal())
        layoutHorizontalBox(relayoutChildren);
    else
        layoutVerticalBox(relayoutChildren);

    repaintChildrenDuringLayoutIfMoved(this, oldChildRects);
    ASSERT(view()->layoutDeltaMatches(oldLayoutDelta));

    LayoutUnit oldClientAfterEdge = clientLogicalBottom();
    updateLogicalHeight();

    if (previousSize.height() != height())
        relayoutChildren = true;

    layoutPositionedObjects(relayoutChildren || isRoot());

    updateRegionsAndShapesAfterChildLayout(flowThread);

    if (!isFloatingOrOutOfFlowPositioned() && height() == 0) {
        // We are a block with no border and padding and a computed height
        // of 0.  The CSS spec states that zero-height blocks collapse their margins
        // together.
        // When blocks are self-collapsing, we just use the top margin values and set the
        // bottom margin max values to 0.  This way we don't factor in the values
        // twice when we collapse with our previous vertically adjacent and
        // following vertically adjacent blocks.
        LayoutUnit pos = maxPositiveMarginBefore();
        LayoutUnit neg = maxNegativeMarginBefore();
        if (maxPositiveMarginAfter() > pos)
            pos = maxPositiveMarginAfter();
        if (maxNegativeMarginAfter() > neg)
            neg = maxNegativeMarginAfter();
        setMaxMarginBeforeValues(pos, neg);
        setMaxMarginAfterValues(0, 0);
    }

    computeOverflow(oldClientAfterEdge);

    statePusher.pop();

    updateLayerTransform();

    if (view()->layoutState()->pageLogicalHeight())
        setPageLogicalOffset(view()->layoutState()->pageLogicalOffset(this, logicalTop()));

    // Update our scrollbars if we're overflow:auto/scroll/hidden now that we know if
    // we overflow or not.
    updateScrollInfoAfterLayout();

    // Repaint with our new bounds if they are different from our old bounds.
    repainter.repaintAfterLayout();

    setNeedsLayout(false);
}

// The first walk over our kids is to find out if we have any flexible children.
static void gatherFlexChildrenInfo(FlexBoxIterator& iterator, bool relayoutChildren, unsigned int& highestFlexGroup, unsigned int& lowestFlexGroup, bool& haveFlex)
{
    for (RenderBox* child = iterator.first(); child; child = iterator.next()) {
        // Check to see if this child flexes.
        if (!childDoesNotAffectWidthOrFlexing(child) && child->style()->boxFlex() > 0.0f) {
            // We always have to lay out flexible objects again, since the flex distribution
            // may have changed, and we need to reallocate space.
            child->clearOverrideSize();
            if (!relayoutChildren)
                child->setChildNeedsLayout(true, MarkOnlyThis);
            haveFlex = true;
            unsigned int flexGroup = child->style()->boxFlexGroup();
            if (lowestFlexGroup == 0)
                lowestFlexGroup = flexGroup;
            if (flexGroup < lowestFlexGroup)
                lowestFlexGroup = flexGroup;
            if (flexGroup > highestFlexGroup)
                highestFlexGroup = flexGroup;
        }
    }
}

static void layoutChildIfNeededApplyingDelta(RenderBox* child, const LayoutSize& layoutDelta)
{
    if (!child->needsLayout())
        return;
    
    child->view()->addLayoutDelta(layoutDelta);
    child->layoutIfNeeded();
    child->view()->addLayoutDelta(-layoutDelta);
}

void RenderDeprecatedFlexibleBox::layoutHorizontalBox(bool relayoutChildren)
{
    LayoutUnit toAdd = borderBottom() + paddingBottom() + horizontalScrollbarHeight();
    LayoutUnit yPos = borderTop() + paddingTop();
    LayoutUnit xPos = borderLeft() + paddingLeft();
    bool heightSpecified = false;
    LayoutUnit oldHeight = 0;

    LayoutUnit remainingSpace = 0;

    FlexBoxIterator iterator(this);
    unsigned int highestFlexGroup = 0;
    unsigned int lowestFlexGroup = 0;
    bool haveFlex = false, flexingChildren = false; 
    gatherFlexChildrenInfo(iterator, relayoutChildren, highestFlexGroup, lowestFlexGroup, haveFlex);

    RenderBlock::startDelayUpdateScrollInfo();

    ChildLayoutDeltas childLayoutDeltas;
    appendChildLayoutDeltas(this, childLayoutDeltas);

    // We do 2 passes.  The first pass is simply to lay everyone out at
    // their preferred widths. The subsequent passes handle flexing the children.
    // The first pass skips flexible objects completely.
    do {
        // Reset our height.
        setHeight(yPos);

        xPos = borderLeft() + paddingLeft();

        size_t childIndex = 0;

        // Our first pass is done without flexing.  We simply lay the children
        // out within the box.  We have to do a layout first in order to determine
        // our box's intrinsic height.
        LayoutUnit maxAscent = 0, maxDescent = 0;
        for (RenderBox* child = iterator.first(); child; child = iterator.next()) {
            if (relayoutChildren)
                child->setChildNeedsLayout(true, MarkOnlyThis);

            if (child->isOutOfFlowPositioned())
                continue;
            
            LayoutSize& childLayoutDelta = childLayoutDeltas[childIndex++];
            
            // Compute the child's vertical margins.
            child->computeAndSetBlockDirectionMargins(this);

            if (!child->needsLayout())
                child->markForPaginationRelayoutIfNeeded();
            
            // Apply the child's current layout delta.
            layoutChildIfNeededApplyingDelta(child, childLayoutDelta);
            
            // Now do the layout.
            layoutChildIfNeededApplyingDelta(child, childLayoutDelta);

            // Update our height and overflow height.
            if (style()->boxAlign() == BBASELINE) {
                LayoutUnit ascent = child->firstLineBoxBaseline();
                if (ascent == -1)
                    ascent = child->height() + child->marginBottom();
                ascent += child->marginTop();
                LayoutUnit descent = (child->height() + child->marginHeight()) - ascent;

                // Update our maximum ascent.
                maxAscent = max(maxAscent, ascent);

                // Update our maximum descent.
                maxDescent = max(maxDescent, descent);

                // Now update our height.
                setHeight(max(yPos + maxAscent + maxDescent, height()));
            }
            else
                setHeight(max(height(), yPos + child->height() + child->marginHeight()));
        }
        ASSERT(childIndex == childLayoutDeltas.size());

        if (!iterator.first() && hasLineIfEmpty())
            setHeight(height() + lineHeight(true, style()->isHorizontalWritingMode() ? HorizontalLine : VerticalLine, PositionOfInteriorLineBoxes));

        setHeight(height() + toAdd);

        oldHeight = height();
        updateLogicalHeight();

        relayoutChildren = false;
        if (oldHeight != height())
            heightSpecified = true;

        // Now that our height is actually known, we can place our boxes.
        childIndex = 0;
        m_stretchingChildren = (style()->boxAlign() == BSTRETCH);
        for (RenderBox* child = iterator.first(); child; child = iterator.next()) {
            if (child->isOutOfFlowPositioned()) {
                child->containingBlock()->insertPositionedObject(child);
                RenderLayer* childLayer = child->layer();
                childLayer->setStaticInlinePosition(xPos); // FIXME: Not right for regions.
                if (childLayer->staticBlockPosition() != yPos) {
                    childLayer->setStaticBlockPosition(yPos);
                    if (child->style()->hasStaticBlockPosition(style()->isHorizontalWritingMode()))
                        child->setChildNeedsLayout(true, MarkOnlyThis);
                }
                continue;
            }
            
            LayoutSize& childLayoutDelta = childLayoutDeltas[childIndex++];
            
            if (child->style()->visibility() == COLLAPSE) {
                // visibility: collapsed children do not participate in our positioning.
                // But we need to lay them out.
                layoutChildIfNeededApplyingDelta(child, childLayoutDelta);
                continue;
            }

            // We need to see if this child's height has changed, since we make block elements
            // fill the height of a containing box by default.
            // Now do a layout.
            LayoutUnit oldChildHeight = child->height();
            child->updateLogicalHeight();
            if (oldChildHeight != child->height())
                child->setChildNeedsLayout(true, MarkOnlyThis);

            if (!child->needsLayout())
                child->markForPaginationRelayoutIfNeeded();

            layoutChildIfNeededApplyingDelta(child, childLayoutDelta);

            // We can place the child now, using our value of box-align.
            xPos += child->marginLeft();
            LayoutUnit childY = yPos;
            switch (style()->boxAlign()) {
                case BCENTER:
                    childY += child->marginTop() + max<LayoutUnit>(0, (contentHeight() - (child->height() + child->marginHeight())) / 2);
                    break;
                case BBASELINE: {
                    LayoutUnit ascent = child->firstLineBoxBaseline();
                    if (ascent == -1)
                        ascent = child->height() + child->marginBottom();
                    ascent += child->marginTop();
                    childY += child->marginTop() + (maxAscent - ascent);
                    break;
                }
                case BEND:
                    childY += contentHeight() - child->marginBottom() - child->height();
                    break;
                default: // BSTART
                    childY += child->marginTop();
                    break;
            }

            placeChild(child, LayoutPoint(xPos, childY), &childLayoutDelta);

            xPos += child->width() + child->marginRight();
        }
        ASSERT(childIndex == childLayoutDeltas.size());

        remainingSpace = borderLeft() + paddingLeft() + contentWidth() - xPos;

        m_stretchingChildren = false;
        if (flexingChildren)
            haveFlex = false; // We're done.
        else if (haveFlex) {
            // We have some flexible objects.  See if we need to grow/shrink them at all.
            if (!remainingSpace)
                break;

            // Allocate the remaining space among the flexible objects.  If we are trying to
            // grow, then we go from the lowest flex group to the highest flex group.  For shrinking,
            // we go from the highest flex group to the lowest group.
            bool expanding = remainingSpace > 0;
            unsigned int start = expanding ? lowestFlexGroup : highestFlexGroup;
            unsigned int end = expanding? highestFlexGroup : lowestFlexGroup;
            for (unsigned int i = start; i <= end && remainingSpace; i++) {
                // Always start off by assuming the group can get all the remaining space.
                LayoutUnit groupRemainingSpace = remainingSpace;
                do {
                    // Flexing consists of multiple passes, since we have to change ratios every time an object hits its max/min-width
                    // For a given pass, we always start off by computing the totalFlex of all objects that can grow/shrink at all, and
                    // computing the allowed growth before an object hits its min/max width (and thus
                    // forces a totalFlex recomputation).
                    LayoutUnit groupRemainingSpaceAtBeginning = groupRemainingSpace;
                    float totalFlex = 0.0f;
                    for (RenderBox* child = iterator.first(); child; child = iterator.next()) {
                        if (allowedChildFlex(child, expanding, i))
                            totalFlex += child->style()->boxFlex();
                    }
                    LayoutUnit spaceAvailableThisPass = groupRemainingSpace;
                    for (RenderBox* child = iterator.first(); child; child = iterator.next()) {
                        LayoutUnit allowedFlex = allowedChildFlex(child, expanding, i);
                        if (allowedFlex) {
                            LayoutUnit projectedFlex = (allowedFlex == LayoutUnit::max()) ? allowedFlex : LayoutUnit(allowedFlex * (totalFlex / child->style()->boxFlex()));
                            spaceAvailableThisPass = expanding ? min(spaceAvailableThisPass, projectedFlex) : max(spaceAvailableThisPass, projectedFlex);
                        }
                    }

                    // The flex groups may not have any flexible objects this time around.
                    if (!spaceAvailableThisPass || totalFlex == 0.0f) {
                        // If we just couldn't grow/shrink any more, then it's time to transition to the next flex group.
                        groupRemainingSpace = 0;
                        continue;
                    }

                    // Now distribute the space to objects.
                    for (RenderBox* child = iterator.first(); child && spaceAvailableThisPass && totalFlex; child = iterator.next()) {
                        if (child->style()->visibility() == COLLAPSE)
                            continue;

                        if (allowedChildFlex(child, expanding, i)) {
                            LayoutUnit spaceAdd = LayoutUnit(spaceAvailableThisPass * (child->style()->boxFlex() / totalFlex));
                            if (spaceAdd) {
                                child->setOverrideLogicalContentWidth(contentWidthForChild(child) + spaceAdd);
                                flexingChildren = true;
                                relayoutChildren = true;
                            }

                            spaceAvailableThisPass -= spaceAdd;
                            remainingSpace -= spaceAdd;
                            groupRemainingSpace -= spaceAdd;

                            totalFlex -= child->style()->boxFlex();
                        }
                    }
                    if (groupRemainingSpace == groupRemainingSpaceAtBeginning) {
                        // This is not advancing, avoid getting stuck by distributing the remaining pixels.
                        LayoutUnit spaceAdd = groupRemainingSpace > 0 ? 1 : -1;
                        for (RenderBox* child = iterator.first(); child && groupRemainingSpace; child = iterator.next()) {
                            if (allowedChildFlex(child, expanding, i)) {
                                child->setOverrideLogicalContentWidth(contentWidthForChild(child) + spaceAdd);
                                flexingChildren = true;
                                relayoutChildren = true;
                                remainingSpace -= spaceAdd;
                                groupRemainingSpace -= spaceAdd;
                            }
                        }
                    }
                } while (absoluteValue(groupRemainingSpace) >= 1);
            }

            // We didn't find any children that could grow.
            if (haveFlex && !flexingChildren)
                haveFlex = false;
        }
    } while (haveFlex);

    RenderBlock::finishDelayUpdateScrollInfo();

    if (remainingSpace > 0 && ((style()->isLeftToRightDirection() && style()->boxPack() != Start)
        || (!style()->isLeftToRightDirection() && style()->boxPack() != End))) {
        // Children must be repositioned.
        LayoutUnit offset = 0;
        if (style()->boxPack() == Justify) {
            // Determine the total number of children.
            int totalChildren = 0;
            for (RenderBox* child = iterator.first(); child; child = iterator.next()) {
                if (childDoesNotAffectWidthOrFlexing(child))
                    continue;
                ++totalChildren;
            }

            // Iterate over the children and space them out according to the
            // justification level.
            if (totalChildren > 1) {
                --totalChildren;
                bool firstChild = true;
                for (RenderBox* child = iterator.first(); child; child = iterator.next()) {
                    if (childDoesNotAffectWidthOrFlexing(child))
                        continue;

                    if (firstChild) {
                        firstChild = false;
                        continue;
                    }

                    offset += remainingSpace/totalChildren;
                    remainingSpace -= (remainingSpace/totalChildren);
                    --totalChildren;

                    placeChild(child, child->location() + LayoutSize(offset, 0));
                }
            }
        } else {
            if (style()->boxPack() == Center)
                offset += remainingSpace / 2;
            else // END for LTR, START for RTL
                offset += remainingSpace;
            for (RenderBox* child = iterator.first(); child; child = iterator.next()) {
                if (childDoesNotAffectWidthOrFlexing(child))
                    continue;

                placeChild(child, child->location() + LayoutSize(offset, 0));
            }
        }
    }

    // So that the computeLogicalHeight in layoutBlock() knows to relayout positioned objects because of
    // a height change, we revert our height back to the intrinsic height before returning.
    if (heightSpecified)
        setHeight(oldHeight);
}

void RenderDeprecatedFlexibleBox::layoutVerticalBox(bool relayoutChildren)
{
    LayoutUnit yPos = borderTop() + paddingTop();
    LayoutUnit toAdd = borderBottom() + paddingBottom() + horizontalScrollbarHeight();
    bool heightSpecified = false;
    LayoutUnit oldHeight = 0;

    LayoutUnit remainingSpace = 0;

    FlexBoxIterator iterator(this);
    unsigned int highestFlexGroup = 0;
    unsigned int lowestFlexGroup = 0;
    bool haveFlex = false, flexingChildren = false; 
    gatherFlexChildrenInfo(iterator, relayoutChildren, highestFlexGroup, lowestFlexGroup, haveFlex);

    // We confine the line clamp ugliness to vertical flexible boxes (thus keeping it out of
    // mainstream block layout); this is not really part of the XUL box model.
    bool haveLineClamp = !style()->lineClamp().isNone();
    if (haveLineClamp)
        applyLineClamp(iterator, relayoutChildren);

    RenderBlock::startDelayUpdateScrollInfo();

    ChildLayoutDeltas childLayoutDeltas;
    appendChildLayoutDeltas(this, childLayoutDeltas);

    // We do 2 passes.  The first pass is simply to lay everyone out at
    // their preferred widths.  The second pass handles flexing the children.
    // Our first pass is done without flexing.  We simply lay the children
    // out within the box.
    do {
        setHeight(borderTop() + paddingTop());
        LayoutUnit minHeight = height() + toAdd;

        size_t childIndex = 0;
        for (RenderBox* child = iterator.first(); child; child = iterator.next()) {
            // Make sure we relayout children if we need it.
            if (!haveLineClamp && relayoutChildren)
                child->setChildNeedsLayout(true, MarkOnlyThis);

            if (child->isOutOfFlowPositioned()) {
                child->containingBlock()->insertPositionedObject(child);
                RenderLayer* childLayer = child->layer();
                childLayer->setStaticInlinePosition(borderStart() + paddingStart()); // FIXME: Not right for regions.
                if (childLayer->staticBlockPosition() != height()) {
                    childLayer->setStaticBlockPosition(height());
                    if (child->style()->hasStaticBlockPosition(style()->isHorizontalWritingMode()))
                        child->setChildNeedsLayout(true, MarkOnlyThis);
                }
                continue;
            }
            
            LayoutSize& childLayoutDelta = childLayoutDeltas[childIndex++];

            if (child->style()->visibility() == COLLAPSE) {
                // visibility: collapsed children do not participate in our positioning.
                // But we need to lay them down.
                layoutChildIfNeededApplyingDelta(child, childLayoutDelta);
                continue;
            }

            // Compute the child's vertical margins.
            child->computeAndSetBlockDirectionMargins(this);

            // Add in the child's marginTop to our height.
            setHeight(height() + child->marginTop());

            if (!child->needsLayout())
                child->markForPaginationRelayoutIfNeeded();

            // Now do a layout.
            layoutChildIfNeededApplyingDelta(child, childLayoutDelta);

            // We can place the child now, using our value of box-align.
            LayoutUnit childX = borderLeft() + paddingLeft();
            switch (style()->boxAlign()) {
                case BCENTER:
                case BBASELINE: // Baseline just maps to center for vertical boxes
                    childX += child->marginLeft() + max<LayoutUnit>(0, (contentWidth() - (child->width() + child->marginWidth())) / 2);
                    break;
                case BEND:
                    if (!style()->isLeftToRightDirection())
                        childX += child->marginLeft();
                    else
                        childX += contentWidth() - child->marginRight() - child->width();
                    break;
                default: // BSTART/BSTRETCH
                    if (style()->isLeftToRightDirection())
                        childX += child->marginLeft();
                    else
                        childX += contentWidth() - child->marginRight() - child->width();
                    break;
            }

            // Place the child.
            placeChild(child, LayoutPoint(childX, height()), &childLayoutDelta);
            setHeight(height() + child->height() + child->marginBottom());
        }
        ASSERT(childIndex == childLayoutDeltas.size());

        yPos = height();

        if (!iterator.first() && hasLineIfEmpty())
            setHeight(height() + lineHeight(true, style()->isHorizontalWritingMode() ? HorizontalLine : VerticalLine, PositionOfInteriorLineBoxes));

        setHeight(height() + toAdd);

        // Negative margins can cause our height to shrink below our minimal height (border/padding).
        // If this happens, ensure that the computed height is increased to the minimal height.
        if (height() < minHeight)
            setHeight(minHeight);

        // Now we have to calc our height, so we know how much space we have remaining.
        oldHeight = height();
        updateLogicalHeight();
        if (oldHeight != height())
            heightSpecified = true;

        remainingSpace = borderTop() + paddingTop() + contentHeight() - yPos;

        if (flexingChildren)
            haveFlex = false; // We're done.
        else if (haveFlex) {
            // We have some flexible objects.  See if we need to grow/shrink them at all.
            if (!remainingSpace)
                break;

            // Allocate the remaining space among the flexible objects.  If we are trying to
            // grow, then we go from the lowest flex group to the highest flex group.  For shrinking,
            // we go from the highest flex group to the lowest group.
            bool expanding = remainingSpace > 0;
            unsigned int start = expanding ? lowestFlexGroup : highestFlexGroup;
            unsigned int end = expanding? highestFlexGroup : lowestFlexGroup;
            for (unsigned int i = start; i <= end && remainingSpace; i++) {
                // Always start off by assuming the group can get all the remaining space.
                LayoutUnit groupRemainingSpace = remainingSpace;
                do {
                    // Flexing consists of multiple passes, since we have to change ratios every time an object hits its max/min-width
                    // For a given pass, we always start off by computing the totalFlex of all objects that can grow/shrink at all, and
                    // computing the allowed growth before an object hits its min/max width (and thus
                    // forces a totalFlex recomputation).
                    LayoutUnit groupRemainingSpaceAtBeginning = groupRemainingSpace;
                    float totalFlex = 0.0f;
                    for (RenderBox* child = iterator.first(); child; child = iterator.next()) {
                        if (allowedChildFlex(child, expanding, i))
                            totalFlex += child->style()->boxFlex();
                    }
                    LayoutUnit spaceAvailableThisPass = groupRemainingSpace;
                    for (RenderBox* child = iterator.first(); child; child = iterator.next()) {
                        LayoutUnit allowedFlex = allowedChildFlex(child, expanding, i);
                        if (allowedFlex) {
                            LayoutUnit projectedFlex = (allowedFlex == LayoutUnit::max()) ? allowedFlex : static_cast<LayoutUnit>(allowedFlex * (totalFlex / child->style()->boxFlex()));
                            spaceAvailableThisPass = expanding ? min(spaceAvailableThisPass, projectedFlex) : max(spaceAvailableThisPass, projectedFlex);
                        }
                    }

                    // The flex groups may not have any flexible objects this time around.
                    if (!spaceAvailableThisPass || totalFlex == 0.0f) {
                        // If we just couldn't grow/shrink any more, then it's time to transition to the next flex group.
                        groupRemainingSpace = 0;
                        continue;
                    }

                    // Now distribute the space to objects.
                    for (RenderBox* child = iterator.first(); child && spaceAvailableThisPass && totalFlex; child = iterator.next()) {
                        if (allowedChildFlex(child, expanding, i)) {
                            LayoutUnit spaceAdd = static_cast<LayoutUnit>(spaceAvailableThisPass * (child->style()->boxFlex() / totalFlex));
                            if (spaceAdd) {
                                child->setOverrideLogicalContentHeight(contentHeightForChild(child) + spaceAdd);
                                flexingChildren = true;
                                relayoutChildren = true;
                            }

                            spaceAvailableThisPass -= spaceAdd;
                            remainingSpace -= spaceAdd;
                            groupRemainingSpace -= spaceAdd;

                            totalFlex -= child->style()->boxFlex();
                        }
                    }
                    if (groupRemainingSpace == groupRemainingSpaceAtBeginning) {
                        // This is not advancing, avoid getting stuck by distributing the remaining pixels.
                        LayoutUnit spaceAdd = groupRemainingSpace > 0 ? 1 : -1;
                        for (RenderBox* child = iterator.first(); child && groupRemainingSpace; child = iterator.next()) {
                            if (allowedChildFlex(child, expanding, i)) {
                                child->setOverrideLogicalContentHeight(contentHeightForChild(child) + spaceAdd);
                                flexingChildren = true;
                                relayoutChildren = true;
                                remainingSpace -= spaceAdd;
                                groupRemainingSpace -= spaceAdd;
                            }
                        }
                    }
                } while (absoluteValue(groupRemainingSpace) >= 1);
            }

            // We didn't find any children that could grow.
            if (haveFlex && !flexingChildren)
                haveFlex = false;
        }
    } while (haveFlex);

    RenderBlock::finishDelayUpdateScrollInfo();

    if (style()->boxPack() != Start && remainingSpace > 0) {
        // Children must be repositioned.
        LayoutUnit offset = 0;
        if (style()->boxPack() == Justify) {
            // Determine the total number of children.
            int totalChildren = 0;
            for (RenderBox* child = iterator.first(); child; child = iterator.next()) {
                if (childDoesNotAffectWidthOrFlexing(child))
                    continue;

                ++totalChildren;
            }

            // Iterate over the children and space them out according to the
            // justification level.
            if (totalChildren > 1) {
                --totalChildren;
                bool firstChild = true;
                for (RenderBox* child = iterator.first(); child; child = iterator.next()) {
                    if (childDoesNotAffectWidthOrFlexing(child))
                        continue;

                    if (firstChild) {
                        firstChild = false;
                        continue;
                    }

                    offset += remainingSpace/totalChildren;
                    remainingSpace -= (remainingSpace/totalChildren);
                    --totalChildren;
                    placeChild(child, child->location() + LayoutSize(0, offset));
                }
            }
        } else {
            if (style()->boxPack() == Center)
                offset += remainingSpace / 2;
            else // END
                offset += remainingSpace;
            for (RenderBox* child = iterator.first(); child; child = iterator.next()) {
                if (childDoesNotAffectWidthOrFlexing(child))
                    continue;
                placeChild(child, child->location() + LayoutSize(0, offset));
            }
        }
    }

    // So that the computeLogicalHeight in layoutBlock() knows to relayout positioned objects because of
    // a height change, we revert our height back to the intrinsic height before returning.
    if (heightSpecified)
        setHeight(oldHeight);
}

void RenderDeprecatedFlexibleBox::applyLineClamp(FlexBoxIterator& iterator, bool relayoutChildren)
{
    int maxLineCount = 0;
    for (RenderBox* child = iterator.first(); child; child = iterator.next()) {
        if (childDoesNotAffectWidthOrFlexing(child))
            continue;

        child->clearOverrideSize();
        if (relayoutChildren || (child->isReplaced() && (child->style()->width().isPercent() || child->style()->height().isPercent()))
            || (child->style()->height().isAuto() && child->isBlockFlow())) {
            child->setChildNeedsLayout(true, MarkOnlyThis);

            // Dirty all the positioned objects.
            if (child->isRenderBlock()) {
                toRenderBlock(child)->markPositionedObjectsForLayout();
                toRenderBlock(child)->clearTruncation();
            }
        }
        child->layoutIfNeeded();
        if (child->style()->height().isAuto() && child->isBlockFlow())
            maxLineCount = max(maxLineCount, toRenderBlock(child)->lineCount());
    }

    // Get the number of lines and then alter all block flow children with auto height to use the
    // specified height. We always try to leave room for at least one line.
    LineClampValue lineClamp = style()->lineClamp();
    int numVisibleLines = lineClamp.isPercentage() ? max(1, (maxLineCount + 1) * lineClamp.value() / 100) : lineClamp.value();
    if (numVisibleLines >= maxLineCount)
        return;

    for (RenderBox* child = iterator.first(); child; child = iterator.next()) {
        if (childDoesNotAffectWidthOrFlexing(child) || !child->style()->height().isAuto() || !child->isBlockFlow())
            continue;

        RenderBlock* blockChild = toRenderBlock(child);
        int lineCount = blockChild->lineCount();
        if (lineCount <= numVisibleLines)
            continue;

        LayoutUnit newHeight = blockChild->heightForLineCount(numVisibleLines);
        if (newHeight == child->height())
            continue;

        child->setChildNeedsLayout(true, MarkOnlyThis);
        child->setOverrideLogicalContentHeight(newHeight - child->borderAndPaddingHeight());
        child->layoutIfNeeded();

        // FIXME: For now don't support RTL.
        if (style()->direction() != LTR)
            continue;

        // Get the last line
        RootInlineBox* lastLine = blockChild->lineAtIndex(lineCount - 1);
        if (!lastLine)
            continue;

        RootInlineBox* lastVisibleLine = blockChild->lineAtIndex(numVisibleLines - 1);
        if (!lastVisibleLine)
            continue;

        const UChar ellipsisAndSpace[2] = { horizontalEllipsis, ' ' };
        DEFINE_STATIC_LOCAL(AtomicString, ellipsisAndSpaceStr, (ellipsisAndSpace, 2));
        DEFINE_STATIC_LOCAL(AtomicString, ellipsisStr, (&horizontalEllipsis, 1));
        const Font& font = style(numVisibleLines == 1)->font();

        // Get ellipsis width, and if the last child is an anchor, it will go after the ellipsis, so add in a space and the anchor width too
        LayoutUnit totalWidth;
        InlineBox* anchorBox = lastLine->lastChild();
        if (anchorBox && anchorBox->renderer()->style()->isLink())
            totalWidth = anchorBox->logicalWidth() + font.width(constructTextRun(this, font, ellipsisAndSpace, 2, style()));
        else {
            anchorBox = 0;
            totalWidth = font.width(constructTextRun(this, font, &horizontalEllipsis, 1, style()));
        }

        // See if this width can be accommodated on the last visible line
        RenderBlock* destBlock = toRenderBlock(lastVisibleLine->renderer());
        RenderBlock* srcBlock = toRenderBlock(lastLine->renderer());

        // FIXME: Directions of src/destBlock could be different from our direction and from one another.
        if (!srcBlock->style()->isLeftToRightDirection())
            continue;

        bool leftToRight = destBlock->style()->isLeftToRightDirection();
        if (!leftToRight)
            continue;

        LayoutUnit blockRightEdge = destBlock->logicalRightOffsetForLine(lastVisibleLine->y(), false);
        if (!lastVisibleLine->lineCanAccommodateEllipsis(leftToRight, blockRightEdge, lastVisibleLine->x() + lastVisibleLine->logicalWidth(), totalWidth))
            continue;

        // Let the truncation code kick in.
        // FIXME: the text alignment should be recomputed after the width changes due to truncation.
        LayoutUnit blockLeftEdge = destBlock->logicalLeftOffsetForLine(lastVisibleLine->y(), false);
        lastVisibleLine->placeEllipsis(anchorBox ? ellipsisAndSpaceStr : ellipsisStr, leftToRight, blockLeftEdge, blockRightEdge, totalWidth, anchorBox);
        destBlock->setHasMarkupTruncation(true);
    }
}

void RenderDeprecatedFlexibleBox::clearLineClamp()
{
    FlexBoxIterator iterator(this);
    for (RenderBox* child = iterator.first(); child; child = iterator.next()) {
        if (childDoesNotAffectWidthOrFlexing(child))
            continue;

        child->clearOverrideSize();
        if ((child->isReplaced() && (child->style()->width().isPercent() || child->style()->height().isPercent()))
            || (child->style()->height().isAuto() && child->isBlockFlow())) {
            child->setChildNeedsLayout(true);

            if (child->isRenderBlock()) {
                toRenderBlock(child)->markPositionedObjectsForLayout();
                toRenderBlock(child)->clearTruncation();
            }
        }
    }
}

void RenderDeprecatedFlexibleBox::placeChild(RenderBox* child, const LayoutPoint& location, LayoutSize* childLayoutDelta)
{
    // Place the child and track the layout delta so we can apply it if we do another layout.
    if (childLayoutDelta)
        *childLayoutDelta += LayoutSize(child->x() - location.x(), child->y() - location.y());
    child->setLocation(location);
}

LayoutUnit RenderDeprecatedFlexibleBox::allowedChildFlex(RenderBox* child, bool expanding, unsigned int group)
{
    if (childDoesNotAffectWidthOrFlexing(child) || child->style()->boxFlex() == 0.0f || child->style()->boxFlexGroup() != group)
        return 0;

    if (expanding) {
        if (isHorizontal()) {
            // FIXME: For now just handle fixed values.
            LayoutUnit maxWidth = LayoutUnit::max();
            LayoutUnit width = contentWidthForChild(child);
            if (!child->style()->maxWidth().isUndefined() && child->style()->maxWidth().isFixed())
                maxWidth = child->style()->maxWidth().value();
            else if (child->style()->maxWidth().type() == Intrinsic)
                maxWidth = child->maxPreferredLogicalWidth();
            else if (child->style()->maxWidth().type() == MinIntrinsic)
                maxWidth = child->minPreferredLogicalWidth();
            if (maxWidth == LayoutUnit::max())
                return maxWidth;
            return max<LayoutUnit>(0, maxWidth - width);
        } else {
            // FIXME: For now just handle fixed values.
            LayoutUnit maxHeight = LayoutUnit::max();
            LayoutUnit height = contentHeightForChild(child);
            if (!child->style()->maxHeight().isUndefined() && child->style()->maxHeight().isFixed())
                maxHeight = child->style()->maxHeight().value();
            if (maxHeight == LayoutUnit::max())
                return maxHeight;
            return max<LayoutUnit>(0, maxHeight - height);
        }
    }

    // FIXME: For now just handle fixed values.
    if (isHorizontal()) {
        LayoutUnit minWidth = child->minPreferredLogicalWidth();
        LayoutUnit width = contentWidthForChild(child);
        if (child->style()->minWidth().isFixed())
            minWidth = child->style()->minWidth().value();
        else if (child->style()->minWidth().type() == Intrinsic)
            minWidth = child->maxPreferredLogicalWidth();
        else if (child->style()->minWidth().type() == MinIntrinsic)
            minWidth = child->minPreferredLogicalWidth();
        else if (child->style()->minWidth().type() == Auto)
            minWidth = 0;

        LayoutUnit allowedShrinkage = min<LayoutUnit>(0, minWidth - width);
        return allowedShrinkage;
    } else {
        Length minHeight = child->style()->minHeight();
        if (minHeight.isFixed() || minHeight.isAuto()) {
            LayoutUnit minHeight = child->style()->minHeight().value();
            LayoutUnit height = contentHeightForChild(child);
            LayoutUnit allowedShrinkage = min<LayoutUnit>(0, minHeight - height);
            return allowedShrinkage;
        }
    }

    return 0;
}

const char* RenderDeprecatedFlexibleBox::renderName() const
{
    if (isFloating())
        return "RenderDeprecatedFlexibleBox (floating)";
    if (isOutOfFlowPositioned())
        return "RenderDeprecatedFlexibleBox (positioned)";
    // FIXME: Temporary hack while the new generated content system is being implemented.
    if (isPseudoElement())
        return "RenderDeprecatedFlexibleBox (generated)";
    if (isAnonymous())
        return "RenderDeprecatedFlexibleBox (generated)";
    if (isRelPositioned())
        return "RenderDeprecatedFlexibleBox (relative positioned)";
    return "RenderDeprecatedFlexibleBox";
}

} // namespace WebCore
