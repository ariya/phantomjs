/*
 * Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"
#include "RenderRegion.h"

#include "FlowThreadController.h"
#include "GraphicsContext.h"
#include "HitTestResult.h"
#include "IntRect.h"
#include "LayoutRepainter.h"
#include "PaintInfo.h"
#include "Range.h"
#include "RenderBoxRegionInfo.h"
#include "RenderNamedFlowThread.h"
#include "RenderView.h"
#include "StyleResolver.h"
#include <wtf/StackStats.h>

using namespace std;

namespace WebCore {

RenderRegion::RenderRegion(Element* element, RenderFlowThread* flowThread)
    : RenderBlock(element)
    , m_flowThread(flowThread)
    , m_parentNamedFlowThread(0)
    , m_isValid(false)
    , m_hasCustomRegionStyle(false)
    , m_hasAutoLogicalHeight(false)
    , m_hasComputedAutoHeight(false)
    , m_computedAutoHeight(0)
{
}

LayoutUnit RenderRegion::pageLogicalWidth() const
{
    ASSERT(m_flowThread);
    return m_flowThread->isHorizontalWritingMode() ? contentWidth() : contentHeight();
}

LayoutUnit RenderRegion::pageLogicalHeight() const
{
    ASSERT(m_flowThread);
    if (hasComputedAutoHeight() && !m_flowThread->inConstrainedLayoutPhase()) {
        ASSERT(hasAutoLogicalHeight());
        return computedAutoHeight();
    }
    return m_flowThread->isHorizontalWritingMode() ? contentHeight() : contentWidth();
}

// This method returns the maximum page size of a region with auto-height. This is the initial
// height value for auto-height regions in the first layout phase of the parent named flow.
LayoutUnit RenderRegion::maxPageLogicalHeight() const
{
    ASSERT(m_flowThread);
    ASSERT(hasAutoLogicalHeight() && !m_flowThread->inConstrainedLayoutPhase());
    return style()->logicalMaxHeight().isUndefined() ? RenderFlowThread::maxLogicalHeight() : computeReplacedLogicalHeightUsing(style()->logicalMaxHeight());
}

LayoutUnit RenderRegion::logicalHeightOfAllFlowThreadContent() const
{
    ASSERT(m_flowThread);
    if (hasComputedAutoHeight() && !m_flowThread->inConstrainedLayoutPhase()) {
        ASSERT(hasAutoLogicalHeight());
        return computedAutoHeight();
    }
    return m_flowThread->isHorizontalWritingMode() ? contentHeight() : contentWidth();
}

LayoutRect RenderRegion::flowThreadPortionOverflowRect() const
{
    return overflowRectForFlowThreadPortion(flowThreadPortionRect(), isFirstRegion(), isLastRegion());
}

LayoutRect RenderRegion::overflowRectForFlowThreadPortion(const LayoutRect& flowThreadPortionRect, bool isFirstPortion, bool isLastPortion) const
{
    ASSERT(isValid());

    // FIXME: Would like to just use hasOverflowClip() but we aren't a block yet. When RenderRegion is eliminated and
    // folded into RenderBlock, switch to hasOverflowClip().
    bool clipX = style()->overflowX() != OVISIBLE;
    bool clipY = style()->overflowY() != OVISIBLE;
    bool isLastRegionWithRegionFragmentBreak = (isLastPortion && (style()->regionFragment() == BreakRegionFragment));
    if ((clipX && clipY) || isLastRegionWithRegionFragmentBreak)
        return flowThreadPortionRect;

    LayoutRect flowThreadOverflow = m_flowThread->visualOverflowRect();

    // Only clip along the flow thread axis.
    LayoutUnit outlineSize = maximalOutlineSize(PaintPhaseOutline);
    LayoutRect clipRect;
    if (m_flowThread->isHorizontalWritingMode()) {
        LayoutUnit minY = isFirstPortion ? (flowThreadOverflow.y() - outlineSize) : flowThreadPortionRect.y();
        LayoutUnit maxY = isLastPortion ? max(flowThreadPortionRect.maxY(), flowThreadOverflow.maxY()) + outlineSize : flowThreadPortionRect.maxY();
        LayoutUnit minX = clipX ? flowThreadPortionRect.x() : min(flowThreadPortionRect.x(), flowThreadOverflow.x() - outlineSize);
        LayoutUnit maxX = clipX ? flowThreadPortionRect.maxX() : max(flowThreadPortionRect.maxX(), (flowThreadOverflow.maxX() + outlineSize));
        clipRect = LayoutRect(minX, minY, maxX - minX, maxY - minY);
    } else {
        LayoutUnit minX = isFirstPortion ? (flowThreadOverflow.x() - outlineSize) : flowThreadPortionRect.x();
        LayoutUnit maxX = isLastPortion ? max(flowThreadPortionRect.maxX(), flowThreadOverflow.maxX()) + outlineSize : flowThreadPortionRect.maxX();
        LayoutUnit minY = clipY ? flowThreadPortionRect.y() : min(flowThreadPortionRect.y(), (flowThreadOverflow.y() - outlineSize));
        LayoutUnit maxY = clipY ? flowThreadPortionRect.maxY() : max(flowThreadPortionRect.y(), (flowThreadOverflow.maxY() + outlineSize));
        clipRect = LayoutRect(minX, minY, maxX - minX, maxY - minY);
    }

    return clipRect;
}

RegionOversetState RenderRegion::regionOversetState() const
{
    ASSERT(node());

    if (!isValid())
        return RegionUndefined;

    if (Element* element = toElement(node()))
        return element->regionOversetState();
    
    return RegionUndefined;
}

void RenderRegion::setRegionOversetState(RegionOversetState state)
{
    ASSERT(node());

    if (Element* element = toElement(node()))
        element->setRegionOversetState(state);
}

LayoutUnit RenderRegion::pageLogicalTopForOffset(LayoutUnit /* offset */) const
{
    return flowThread()->isHorizontalWritingMode() ? flowThreadPortionRect().y() : flowThreadPortionRect().x();
}

bool RenderRegion::isFirstRegion() const
{
    ASSERT(isValid());

    return m_flowThread->firstRegion() == this;
}

bool RenderRegion::isLastRegion() const
{
    ASSERT(isValid());

    return m_flowThread->lastRegion() == this;
}

static bool shouldPaintRegionContentsInPhase(PaintPhase phase)
{
    return phase == PaintPhaseBlockBackground
        || phase == PaintPhaseChildBlockBackground
        || phase == PaintPhaseSelection
        || phase == PaintPhaseTextClip;
}

void RenderRegion::paintObject(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (style()->visibility() != VISIBLE)
        return;

    RenderBlock::paintObject(paintInfo, paintOffset);

    if (!isValid())
        return;

    // We do not want to paint a region's contents multiple times (for each paint phase of the region object).
    // Thus, we only paint the region's contents in certain phases.
    if (!shouldPaintRegionContentsInPhase(paintInfo.phase))
        return;

    // Delegate the painting of a region's contents to RenderFlowThread.
    // RenderFlowThread is a self painting layer because it's a positioned object.
    // RenderFlowThread paints its children, the collected objects.
    setRegionObjectsRegionStyle();
    m_flowThread->paintFlowThreadPortionInRegion(paintInfo, this, flowThreadPortionRect(), flowThreadPortionOverflowRect(), LayoutPoint(paintOffset.x() + borderLeft() + paddingLeft(), paintOffset.y() + borderTop() + paddingTop()));
    restoreRegionObjectsOriginalStyle();
}

// Hit Testing
bool RenderRegion::hitTestContents(const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction action)
{
    if (!isValid() || action != HitTestForeground)
        return false;

    LayoutRect boundsRect = borderBoxRectInRegion(locationInContainer.region());
    boundsRect.moveBy(accumulatedOffset);
    if (visibleToHitTesting() && locationInContainer.intersects(boundsRect)) {
        if (m_flowThread->hitTestFlowThreadPortionInRegion(this, flowThreadPortionRect(), flowThreadPortionOverflowRect(), request, result,
            locationInContainer, LayoutPoint(accumulatedOffset.x() + borderLeft() + paddingLeft(), accumulatedOffset.y() + borderTop() + paddingTop())))
            return true;
    }

    return false;
}

void RenderRegion::checkRegionStyle()
{
    ASSERT(m_flowThread);
    bool customRegionStyle = false;

    // FIXME: Region styling doesn't work for pseudo elements.
    if (node()) {
        Element* regionElement = toElement(node());
        customRegionStyle = view()->document()->ensureStyleResolver()->checkRegionStyle(regionElement);
    }
    setHasCustomRegionStyle(customRegionStyle);
    m_flowThread->checkRegionsWithStyling();
}

void RenderRegion::incrementAutoLogicalHeightCount()
{
    ASSERT(isValid());
    ASSERT(m_hasAutoLogicalHeight);

    m_flowThread->incrementAutoLogicalHeightRegions();
}

void RenderRegion::decrementAutoLogicalHeightCount()
{
    ASSERT(isValid());

    m_flowThread->decrementAutoLogicalHeightRegions();
}

void RenderRegion::updateRegionHasAutoLogicalHeightFlag()
{
    ASSERT(m_flowThread);

    if (!isValid())
        return;

    bool didHaveAutoLogicalHeight = m_hasAutoLogicalHeight;
    m_hasAutoLogicalHeight = shouldHaveAutoLogicalHeight();
    if (m_hasAutoLogicalHeight != didHaveAutoLogicalHeight) {
        if (m_hasAutoLogicalHeight)
            incrementAutoLogicalHeightCount();
        else {
            clearComputedAutoHeight();
            decrementAutoLogicalHeightCount();
        }
    }
}

bool RenderRegion::shouldHaveAutoLogicalHeight() const
{
    bool hasSpecifiedEndpointsForHeight = style()->logicalTop().isSpecified() && style()->logicalBottom().isSpecified();
    bool hasAnchoredEndpointsForHeight = isOutOfFlowPositioned() && hasSpecifiedEndpointsForHeight;
    return style()->logicalHeight().isAuto() && !hasAnchoredEndpointsForHeight;
}
    
void RenderRegion::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBlock::styleDidChange(diff, oldStyle);

    // If the region is not attached to any thread, there is no need to check
    // whether the region has region styling since no content will be displayed
    // into the region.
    if (!m_flowThread) {
        setHasCustomRegionStyle(false);
        return;
    }

    checkRegionStyle();
    updateRegionHasAutoLogicalHeightFlag();
}

void RenderRegion::layoutBlock(bool relayoutChildren, LayoutUnit)
{
    StackStats::LayoutCheckPoint layoutCheckPoint;
    RenderBlock::layoutBlock(relayoutChildren);

    if (isValid()) {
        LayoutRect oldRegionRect(flowThreadPortionRect());
        if (!isHorizontalWritingMode())
            oldRegionRect = oldRegionRect.transposedRect();

        if (hasAutoLogicalHeight() && !m_flowThread->inConstrainedLayoutPhase()) {
            m_flowThread->invalidateRegions();
            clearComputedAutoHeight();
            return;
        }

        if (!isRenderRegionSet() && (oldRegionRect.width() != pageLogicalWidth() || oldRegionRect.height() != pageLogicalHeight()))
            // This can happen even if we are in the inConstrainedLayoutPhase and it will trigger a pathological layout of the flow thread.
            m_flowThread->invalidateRegions();
    }

    // FIXME: We need to find a way to set up overflow properly. Our flow thread hasn't gotten a layout
    // yet, so we can't look to it for correct information. It's possible we could wait until after the RenderFlowThread
    // gets a layout, and then try to propagate overflow information back to the region, and then mark for a second layout.
    // That second layout would then be able to use the information from the RenderFlowThread to set up overflow.
    //
    // The big problem though is that overflow needs to be region-specific. We can't simply use the RenderFlowThread's global
    // overflow values, since then we'd always think any narrow region had huge overflow (all the way to the width of the
    // RenderFlowThread itself).
    //
    // We'll need to expand RenderBoxRegionInfo to also hold left and right overflow values.
}

void RenderRegion::repaintFlowThreadContent(const LayoutRect& repaintRect, bool immediate) const
{
    repaintFlowThreadContentRectangle(repaintRect, immediate, flowThreadPortionRect(), flowThreadPortionOverflowRect(), contentBoxRect().location());
}

void RenderRegion::repaintFlowThreadContentRectangle(const LayoutRect& repaintRect, bool immediate, const LayoutRect& flowThreadPortionRect, const LayoutRect& flowThreadPortionOverflowRect, const LayoutPoint& regionLocation) const
{
    ASSERT(isValid());

    // We only have to issue a repaint in this region if the region rect intersects the repaint rect.
    LayoutRect flippedFlowThreadPortionRect(flowThreadPortionRect);
    LayoutRect flippedFlowThreadPortionOverflowRect(flowThreadPortionOverflowRect);
    flowThread()->flipForWritingMode(flippedFlowThreadPortionRect); // Put the region rects into physical coordinates.
    flowThread()->flipForWritingMode(flippedFlowThreadPortionOverflowRect);

    LayoutRect clippedRect(repaintRect);
    clippedRect.intersect(flippedFlowThreadPortionOverflowRect);
    if (clippedRect.isEmpty())
        return;

    // Put the region rect into the region's physical coordinate space.
    clippedRect.setLocation(regionLocation + (clippedRect.location() - flippedFlowThreadPortionRect.location()));

    // Now switch to the region's writing mode coordinate space and let it repaint itself.
    flipForWritingMode(clippedRect);
    
    // Issue the repaint.
    repaintRectangle(clippedRect, immediate);
}

void RenderRegion::installFlowThread()
{
    ASSERT(view());

    m_flowThread = view()->flowThreadController()->ensureRenderFlowThreadWithName(style()->regionThread());

    // By now the flow thread should already be added to the rendering tree,
    // so we go up the rendering parents and check that this region is not part of the same
    // flow that it actually needs to display. It would create a circular reference.
    RenderObject* parentObject = parent();
    m_parentNamedFlowThread = 0;
    for ( ; parentObject; parentObject = parentObject->parent()) {
        if (parentObject->isRenderNamedFlowThread()) {
            m_parentNamedFlowThread = toRenderNamedFlowThread(parentObject);
            // Do not take into account a region that links a flow with itself. The dependency
            // cannot change, so it is not worth adding it to the list.
            if (m_flowThread == m_parentNamedFlowThread)
                m_flowThread = 0;
            break;
        }
    }
}

void RenderRegion::attachRegion()
{
    if (documentBeingDestroyed())
        return;
    
    // A region starts off invalid.
    setIsValid(false);

    // Initialize the flow thread reference and create the flow thread object if needed.
    // The flow thread lifetime is influenced by the number of regions attached to it,
    // and we are attaching the region to the flow thread.
    installFlowThread();
    
    if (!m_flowThread)
        return;

    // Only after adding the region to the thread, the region is marked to be valid.
    m_flowThread->addRegionToThread(this);

    // The region just got attached to the flow thread, lets check whether
    // it has region styling rules associated.
    checkRegionStyle();

    if (!isValid())
        return;

    m_hasAutoLogicalHeight = shouldHaveAutoLogicalHeight();
    if (hasAutoLogicalHeight())
        incrementAutoLogicalHeightCount();
}

void RenderRegion::detachRegion()
{
    if (m_flowThread) {
        m_flowThread->removeRegionFromThread(this);
        if (hasAutoLogicalHeight())
            decrementAutoLogicalHeightCount();
    }
    m_flowThread = 0;
}

RenderBoxRegionInfo* RenderRegion::renderBoxRegionInfo(const RenderBox* box) const
{
    ASSERT(isValid());
    return m_renderBoxRegionInfo.get(box);
}

RenderBoxRegionInfo* RenderRegion::setRenderBoxRegionInfo(const RenderBox* box, LayoutUnit logicalLeftInset, LayoutUnit logicalRightInset,
    bool containingBlockChainIsInset)
{
    ASSERT(isValid());

    OwnPtr<RenderBoxRegionInfo>& boxInfo = m_renderBoxRegionInfo.add(box, nullptr).iterator->value;
    if (boxInfo)
        *boxInfo = RenderBoxRegionInfo(logicalLeftInset, logicalRightInset, containingBlockChainIsInset);
    else
        boxInfo = adoptPtr(new RenderBoxRegionInfo(logicalLeftInset, logicalRightInset, containingBlockChainIsInset));

    return boxInfo.get();
}

PassOwnPtr<RenderBoxRegionInfo> RenderRegion::takeRenderBoxRegionInfo(const RenderBox* box)
{
    return m_renderBoxRegionInfo.take(box);
}

void RenderRegion::removeRenderBoxRegionInfo(const RenderBox* box)
{
    m_renderBoxRegionInfo.remove(box);
}

void RenderRegion::deleteAllRenderBoxRegionInfo()
{
    m_renderBoxRegionInfo.clear();
}

LayoutUnit RenderRegion::logicalTopOfFlowThreadContentRect(const LayoutRect& rect) const
{
    ASSERT(isValid());
    return flowThread()->isHorizontalWritingMode() ? rect.y() : rect.x();
}

LayoutUnit RenderRegion::logicalBottomOfFlowThreadContentRect(const LayoutRect& rect) const
{
    ASSERT(isValid());
    return flowThread()->isHorizontalWritingMode() ? rect.maxY() : rect.maxX();
}

void RenderRegion::setRegionObjectsRegionStyle()
{
    if (!hasCustomRegionStyle())
        return;

    // Start from content nodes and recursively compute the style in region for the render objects below.
    // If the style in region was already computed, used that style instead of computing a new one.
    RenderNamedFlowThread* namedFlow = view()->flowThreadController()->ensureRenderFlowThreadWithName(style()->regionThread());
    const NamedFlowContentNodes& contentNodes = namedFlow->contentNodes();

    for (NamedFlowContentNodes::const_iterator iter = contentNodes.begin(), end = contentNodes.end(); iter != end; ++iter) {
        const Node* node = *iter;
        // The list of content nodes contains also the nodes with display:none.
        if (!node->renderer())
            continue;

        RenderObject* object = node->renderer();
        // If the content node does not flow any of its children in this region,
        // we do not compute any style for them in this region.
        if (!flowThread()->objectInFlowRegion(object, this))
            continue;

        // If the object has style in region, use that instead of computing a new one.
        RenderObjectRegionStyleMap::iterator it = m_renderObjectRegionStyle.find(object);
        RefPtr<RenderStyle> objectStyleInRegion;
        bool objectRegionStyleCached = false;
        if (it != m_renderObjectRegionStyle.end()) {
            objectStyleInRegion = it->value.style;
            ASSERT(it->value.cached);
            objectRegionStyleCached = true;
        } else
            objectStyleInRegion = computeStyleInRegion(object);

        setObjectStyleInRegion(object, objectStyleInRegion, objectRegionStyleCached);

        computeChildrenStyleInRegion(object);
    }
}

void RenderRegion::restoreRegionObjectsOriginalStyle()
{
    if (!hasCustomRegionStyle())
        return;

    RenderObjectRegionStyleMap temp;
    for (RenderObjectRegionStyleMap::iterator iter = m_renderObjectRegionStyle.begin(), end = m_renderObjectRegionStyle.end(); iter != end; ++iter) {
        RenderObject* object = const_cast<RenderObject*>(iter->key);
        RefPtr<RenderStyle> objectRegionStyle = object->style();
        RefPtr<RenderStyle> objectOriginalStyle = iter->value.style;
        object->setStyleInternal(objectOriginalStyle);

        bool shouldCacheRegionStyle = iter->value.cached;
        if (!shouldCacheRegionStyle) {
            // Check whether we should cache the computed style in region.
            unsigned changedContextSensitiveProperties = ContextSensitivePropertyNone;
            StyleDifference styleDiff = objectOriginalStyle->diff(objectRegionStyle.get(), changedContextSensitiveProperties);
            if (styleDiff < StyleDifferenceLayoutPositionedMovementOnly)
                shouldCacheRegionStyle = true;
        }
        if (shouldCacheRegionStyle) {
            ObjectRegionStyleInfo styleInfo;
            styleInfo.style = objectRegionStyle;
            styleInfo.cached = true;
            temp.set(object, styleInfo);
        }
    }

    m_renderObjectRegionStyle.swap(temp);
}

void RenderRegion::insertedIntoTree()
{
    RenderBlock::insertedIntoTree();

    attachRegion();
}

void RenderRegion::willBeRemovedFromTree()
{
    RenderBlock::willBeRemovedFromTree();

    detachRegion();
}

PassRefPtr<RenderStyle> RenderRegion::computeStyleInRegion(const RenderObject* object)
{
    ASSERT(object);
    ASSERT(object->view());
    ASSERT(object->view()->document());
    ASSERT(!object->isAnonymous());
    ASSERT(object->node() && object->node()->isElementNode());

    // FIXME: Region styling fails for pseudo-elements because the renderers don't have a node.
    Element* element = toElement(object->node());
    RefPtr<RenderStyle> renderObjectRegionStyle = object->view()->document()->ensureStyleResolver()->styleForElement(element, 0, DisallowStyleSharing, MatchAllRules, this);

    return renderObjectRegionStyle.release();
}

void RenderRegion::computeChildrenStyleInRegion(const RenderObject* object)
{
    for (RenderObject* child = object->firstChild(); child; child = child->nextSibling()) {

        RenderObjectRegionStyleMap::iterator it = m_renderObjectRegionStyle.find(child);

        RefPtr<RenderStyle> childStyleInRegion;
        bool objectRegionStyleCached = false;
        if (it != m_renderObjectRegionStyle.end()) {
            childStyleInRegion = it->value.style;
            objectRegionStyleCached = true;
        } else {
            if (child->isAnonymous() || child->isInFlowRenderFlowThread())
                childStyleInRegion = RenderStyle::createAnonymousStyleWithDisplay(object->style(), child->style()->display());
            else if (child->isText())
                childStyleInRegion = RenderStyle::clone(object->style());
            else
                childStyleInRegion = computeStyleInRegion(child);
        }

        setObjectStyleInRegion(child, childStyleInRegion, objectRegionStyleCached);

        computeChildrenStyleInRegion(child);
    }
}

void RenderRegion::setObjectStyleInRegion(RenderObject* object, PassRefPtr<RenderStyle> styleInRegion, bool objectRegionStyleCached)
{
    ASSERT(object->flowThreadContainingBlock());

    RefPtr<RenderStyle> objectOriginalStyle = object->style();
    object->setStyleInternal(styleInRegion);

    if (object->isBoxModelObject() && !object->hasBoxDecorations()) {
        bool hasBoxDecorations = object->isTableCell()
        || object->style()->hasBackground()
        || object->style()->hasBorder()
        || object->style()->hasAppearance()
        || object->style()->boxShadow();
        object->setHasBoxDecorations(hasBoxDecorations);
    }

    ObjectRegionStyleInfo styleInfo;
    styleInfo.style = objectOriginalStyle;
    styleInfo.cached = objectRegionStyleCached;
    m_renderObjectRegionStyle.set(object, styleInfo);
}

void RenderRegion::clearObjectStyleInRegion(const RenderObject* object)
{
    ASSERT(object);
    m_renderObjectRegionStyle.remove(object);

    // Clear the style for the children of this object.
    for (RenderObject* child = object->firstChild(); child; child = child->nextSibling())
        clearObjectStyleInRegion(child);
}

void RenderRegion::computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const
{
    if (!isValid()) {
        RenderBlock::computeIntrinsicLogicalWidths(minLogicalWidth, maxLogicalWidth);
        return;
    }

    minLogicalWidth = m_flowThread->minPreferredLogicalWidth();
    maxLogicalWidth = m_flowThread->maxPreferredLogicalWidth();
}

void RenderRegion::computePreferredLogicalWidths()
{
    ASSERT(preferredLogicalWidthsDirty());

    if (!isValid()) {
        RenderBlock::computePreferredLogicalWidths();
        return;
    }

    // FIXME: Currently, the code handles only the <length> case for min-width/max-width.
    // It should also support other values, like percentage, calc or viewport relative.
    m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth = 0;

    RenderStyle* styleToUse = style();
    if (styleToUse->logicalWidth().isFixed() && styleToUse->logicalWidth().value() > 0)
        m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth = adjustContentBoxLogicalWidthForBoxSizing(styleToUse->logicalWidth().value());
    else
        computeIntrinsicLogicalWidths(m_minPreferredLogicalWidth, m_maxPreferredLogicalWidth);

    if (styleToUse->logicalMinWidth().isFixed() && styleToUse->logicalMinWidth().value() > 0) {
        m_maxPreferredLogicalWidth = std::max(m_maxPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(styleToUse->logicalMinWidth().value()));
        m_minPreferredLogicalWidth = std::max(m_minPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(styleToUse->logicalMinWidth().value()));
    }

    if (styleToUse->logicalMaxWidth().isFixed()) {
        m_maxPreferredLogicalWidth = std::min(m_maxPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(styleToUse->logicalMaxWidth().value()));
        m_minPreferredLogicalWidth = std::min(m_minPreferredLogicalWidth, adjustContentBoxLogicalWidthForBoxSizing(styleToUse->logicalMaxWidth().value()));
    }

    LayoutUnit borderAndPadding = borderAndPaddingLogicalWidth();
    m_minPreferredLogicalWidth += borderAndPadding;
    m_maxPreferredLogicalWidth += borderAndPadding;
    setPreferredLogicalWidthsDirty(false);
}

void RenderRegion::getRanges(Vector<RefPtr<Range> >& rangeObjects) const
{
    RenderNamedFlowThread* namedFlow = view()->flowThreadController()->ensureRenderFlowThreadWithName(style()->regionThread());
    namedFlow->getRanges(rangeObjects, this);
}

void RenderRegion::updateLogicalHeight()
{
    RenderBlock::updateLogicalHeight();

    if (!hasAutoLogicalHeight())
        return;

    // We want to update the logical height based on the computed auto-height
    // only if the view is in the layout phase in which all the
    // auto logical height regions have a computed auto-height.
    if (!m_flowThread->inConstrainedLayoutPhase())
        return;

    // There may be regions with auto logical height that during the prerequisite layout phase
    // did not have the chance to layout flow thread content. Because of that, these regions do not
    // have a computedAutoHeight and they will not be able to fragment any flow
    // thread content.
    if (!hasComputedAutoHeight())
        return;

    LayoutUnit newLogicalHeight = computedAutoHeight() + borderAndPaddingLogicalHeight();
    ASSERT(newLogicalHeight < LayoutUnit::max() / 2);
    if (newLogicalHeight > logicalHeight()) {
        setLogicalHeight(newLogicalHeight);
        // Recalculate position of the render block after new logical height is set.
        // (needed in absolute positioning case with bottom alignment for example)
        RenderBlock::updateLogicalHeight();
    }
}

} // namespace WebCore
