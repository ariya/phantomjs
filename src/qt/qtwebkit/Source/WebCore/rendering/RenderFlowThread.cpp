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

#include "RenderFlowThread.h"

#include "FlowThreadController.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "Node.h"
#include "PODIntervalTree.h"
#include "PaintInfo.h"
#include "RenderBoxRegionInfo.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "RenderRegion.h"
#include "RenderView.h"
#include "TransformState.h"
#include "WebKitNamedFlow.h"
#include <wtf/StackStats.h>

namespace WebCore {

RenderFlowThread::RenderFlowThread()
    : RenderBlock(0)
    , m_previousRegionCount(0)
    , m_autoLogicalHeightRegionsCount(0)
    , m_regionsInvalidated(false)
    , m_regionsHaveUniformLogicalWidth(true)
    , m_regionsHaveUniformLogicalHeight(true)
    , m_hasRegionsWithStyling(false)
    , m_dispatchRegionLayoutUpdateEvent(false)
    , m_dispatchRegionOversetChangeEvent(false)
    , m_pageLogicalSizeChanged(false)
    , m_inConstrainedLayoutPhase(false)
    , m_needsTwoPhasesLayout(false)
{
    setFlowThreadState(InsideOutOfFlowThread);
}

PassRefPtr<RenderStyle> RenderFlowThread::createFlowThreadStyle(RenderStyle* parentStyle)
{
    RefPtr<RenderStyle> newStyle(RenderStyle::create());
    newStyle->inheritFrom(parentStyle);
    newStyle->setDisplay(BLOCK);
    newStyle->setPosition(AbsolutePosition);
    newStyle->setZIndex(0);
    newStyle->setLeft(Length(0, Fixed));
    newStyle->setTop(Length(0, Fixed));
    newStyle->setWidth(Length(100, Percent));
    newStyle->setHeight(Length(100, Percent));
    newStyle->font().update(0);
    
    return newStyle.release();
}

void RenderFlowThread::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBlock::styleDidChange(diff, oldStyle);

    if (oldStyle && oldStyle->writingMode() != style()->writingMode())
        m_regionsInvalidated = true;
}

void RenderFlowThread::removeFlowChildInfo(RenderObject* child)
{
    if (child->isBox())
        removeRenderBoxRegionInfo(toRenderBox(child));
    clearRenderObjectCustomStyle(child);
}

void RenderFlowThread::addRegionToThread(RenderRegion* renderRegion)
{
    ASSERT(renderRegion);
    m_regionList.add(renderRegion);
    renderRegion->setIsValid(true);
}

void RenderFlowThread::removeRegionFromThread(RenderRegion* renderRegion)
{
    ASSERT(renderRegion);
    m_regionList.remove(renderRegion);
}

void RenderFlowThread::invalidateRegions()
{
    if (m_regionsInvalidated) {
        ASSERT(selfNeedsLayout());
        return;
    }

    m_regionRangeMap.clear();
    m_breakBeforeToRegionMap.clear();
    m_breakAfterToRegionMap.clear();
    setNeedsLayout(true);

    m_regionsInvalidated = true;
}

class CurrentRenderFlowThreadDisabler {
    WTF_MAKE_NONCOPYABLE(CurrentRenderFlowThreadDisabler);
public:
    CurrentRenderFlowThreadDisabler(RenderView* view)
        : m_view(view)
        , m_renderFlowThread(0)
    {
        m_renderFlowThread = m_view->flowThreadController()->currentRenderFlowThread();
        if (m_renderFlowThread)
            view->flowThreadController()->setCurrentRenderFlowThread(0);
    }
    ~CurrentRenderFlowThreadDisabler()
    {
        if (m_renderFlowThread)
            m_view->flowThreadController()->setCurrentRenderFlowThread(m_renderFlowThread);
    }
private:
    RenderView* m_view;
    RenderFlowThread* m_renderFlowThread;
};

void RenderFlowThread::validateRegions()
{
    if (m_regionsInvalidated) {
        m_regionsInvalidated = false;
        m_regionsHaveUniformLogicalWidth = true;
        m_regionsHaveUniformLogicalHeight = true;

        if (hasRegions()) {
            LayoutUnit previousRegionLogicalWidth = 0;
            LayoutUnit previousRegionLogicalHeight = 0;
            bool firstRegionVisited = false;
            
            for (RenderRegionList::iterator iter = m_regionList.begin(); iter != m_regionList.end(); ++iter) {
                RenderRegion* region = *iter;
                ASSERT(!region->needsLayout() || region->isRenderRegionSet());

                region->deleteAllRenderBoxRegionInfo();

                // In the normal layout phase we need to initialize the computedAutoHeight for auto-height regions.
                // See initializeRegionsComputedAutoHeight for the explanation.
                // Also, if we have auto-height regions we can't assume m_regionsHaveUniformLogicalHeight to be true in the first phase
                // because the auto-height regions don't have their height computed yet.
                if (!inConstrainedLayoutPhase() && region->hasAutoLogicalHeight()) {
                    region->setComputedAutoHeight(region->maxPageLogicalHeight());
                    m_regionsHaveUniformLogicalHeight = false;
                }

                LayoutUnit regionLogicalWidth = region->pageLogicalWidth();
                LayoutUnit regionLogicalHeight = region->pageLogicalHeight();

                if (!firstRegionVisited)
                    firstRegionVisited = true;
                else {
                    if (m_regionsHaveUniformLogicalWidth && previousRegionLogicalWidth != regionLogicalWidth)
                        m_regionsHaveUniformLogicalWidth = false;
                    if (m_regionsHaveUniformLogicalHeight && previousRegionLogicalHeight != regionLogicalHeight)
                        m_regionsHaveUniformLogicalHeight = false;
                }

                previousRegionLogicalWidth = regionLogicalWidth;
            }
        }
    }

    updateLogicalWidth(); // Called to get the maximum logical width for the region.
    updateRegionsFlowThreadPortionRect();
}

void RenderFlowThread::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;

    m_pageLogicalSizeChanged = m_regionsInvalidated && everHadLayout();

    // In case this is the second pass of the normal phase we need to update the auto-height regions to their initial value.
    // If the region chain was invalidated this will happen anyway.
    if (!m_regionsInvalidated && !inConstrainedLayoutPhase())
        initializeRegionsComputedAutoHeight();

    validateRegions();

    // This is the first phase of the layout and because we have auto-height regions we'll need a second
    // pass to update the flow with the computed auto-height regions.
    m_needsTwoPhasesLayout = !inConstrainedLayoutPhase() && hasAutoLogicalHeightRegions();

    CurrentRenderFlowThreadMaintainer currentFlowThreadSetter(this);
    RenderBlock::layout();

    m_pageLogicalSizeChanged = false;

    if (lastRegion())
        lastRegion()->expandToEncompassFlowThreadContentsIfNeeded();

    if (shouldDispatchRegionLayoutUpdateEvent())
        dispatchRegionLayoutUpdateEvent();
    
    if (shouldDispatchRegionOversetChangeEvent())
        dispatchRegionOversetChangeEvent();
}

void RenderFlowThread::updateLogicalWidth()
{
    LayoutUnit logicalWidth = initialLogicalWidth();
    for (RenderRegionList::iterator iter = m_regionList.begin(); iter != m_regionList.end(); ++iter) {
        RenderRegion* region = *iter;
        ASSERT(!region->needsLayout() || region->isRenderRegionSet());
        logicalWidth = max(region->pageLogicalWidth(), logicalWidth);
    }
    setLogicalWidth(logicalWidth);

    // If the regions have non-uniform logical widths, then insert inset information for the RenderFlowThread.
    for (RenderRegionList::iterator iter = m_regionList.begin(); iter != m_regionList.end(); ++iter) {
        RenderRegion* region = *iter;
        LayoutUnit regionLogicalWidth = region->pageLogicalWidth();
        if (regionLogicalWidth != logicalWidth) {
            LayoutUnit logicalLeft = style()->direction() == LTR ? LayoutUnit() : logicalWidth - regionLogicalWidth;
            region->setRenderBoxRegionInfo(this, logicalLeft, regionLogicalWidth, false);
        }
    }
}

void RenderFlowThread::computeLogicalHeight(LayoutUnit, LayoutUnit logicalTop, LogicalExtentComputedValues& computedValues) const
{
    computedValues.m_position = logicalTop;
    computedValues.m_extent = 0;

    const LayoutUnit maxFlowSize = RenderFlowThread::maxLogicalHeight();
    for (RenderRegionList::const_iterator iter = m_regionList.begin(); iter != m_regionList.end(); ++iter) {
        RenderRegion* region = *iter;
        ASSERT(!region->needsLayout() || region->isRenderRegionSet());

        LayoutUnit distanceToMaxSize = maxFlowSize - computedValues.m_extent;
        computedValues.m_extent += std::min(distanceToMaxSize, region->logicalHeightOfAllFlowThreadContent());

        // If we reached the maximum size there's no point in going further.
        if (computedValues.m_extent == maxFlowSize)
            return;
    }
}

LayoutRect RenderFlowThread::computeRegionClippingRect(const LayoutPoint& offset, const LayoutRect& flowThreadPortionRect, const LayoutRect& flowThreadPortionOverflowRect) const
{
    LayoutRect regionClippingRect(offset + (flowThreadPortionOverflowRect.location() - flowThreadPortionRect.location()), flowThreadPortionOverflowRect.size());
    if (style()->isFlippedBlocksWritingMode())
        regionClippingRect.move(flowThreadPortionRect.size() - flowThreadPortionOverflowRect.size());
    return regionClippingRect;
}

void RenderFlowThread::paintFlowThreadPortionInRegion(PaintInfo& paintInfo, RenderRegion* region, const LayoutRect& flowThreadPortionRect, const LayoutRect& flowThreadPortionOverflowRect, const LayoutPoint& paintOffset) const
{
    GraphicsContext* context = paintInfo.context;
    if (!context)
        return;

    // RenderFlowThread should start painting its content in a position that is offset
    // from the region rect's current position. The amount of offset is equal to the location of
    // the flow thread portion in the flow thread's local coordinates.
    // Note that we have to pixel snap the location at which we're going to paint, since this is necessary
    // to minimize the amount of incorrect snapping that would otherwise occur.
    // If we tried to paint by applying a non-integral translation, then all the
    // layout code that attempted to pixel snap would be incorrect.
    IntPoint adjustedPaintOffset;
    LayoutPoint portionLocation;
    if (style()->isFlippedBlocksWritingMode()) {
        LayoutRect flippedFlowThreadPortionRect(flowThreadPortionRect);
        flipForWritingMode(flippedFlowThreadPortionRect);
        portionLocation = flippedFlowThreadPortionRect.location();
    } else
        portionLocation = flowThreadPortionRect.location();
    adjustedPaintOffset = roundedIntPoint(paintOffset - portionLocation);

    // The clipping rect for the region is set up by assuming the flowThreadPortionRect is going to paint offset from adjustedPaintOffset.
    // Remember that we pixel snapped and moved the paintOffset and stored the snapped result in adjustedPaintOffset. Now we add back in
    // the flowThreadPortionRect's location to get the spot where we expect the portion to actually paint. This can be non-integral and
    // that's ok. We then pixel snap the resulting clipping rect to account for snapping that will occur when the flow thread paints.
    IntRect regionClippingRect = pixelSnappedIntRect(computeRegionClippingRect(adjustedPaintOffset + portionLocation, flowThreadPortionRect, flowThreadPortionOverflowRect));

    PaintInfo info(paintInfo);
    info.rect.intersect(regionClippingRect);

    if (!info.rect.isEmpty()) {
        context->save();

        context->clip(regionClippingRect);

        context->translate(adjustedPaintOffset.x(), adjustedPaintOffset.y());
        info.rect.moveBy(-adjustedPaintOffset);
        
        PaintBehavior paintBehavior = 0;
        if (info.phase == PaintPhaseTextClip)
            paintBehavior |= PaintBehaviorForceBlackText;
        else if (info.phase == PaintPhaseSelection)
            paintBehavior |= PaintBehaviorSelectionOnly;

        layer()->paint(context, info.rect, paintBehavior, 0, region, RenderLayer::PaintLayerTemporaryClipRects);

        context->restore();
    }
}

bool RenderFlowThread::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction hitTestAction)
{
    if (hitTestAction == HitTestBlockBackground)
        return false;
    return RenderBlock::nodeAtPoint(request, result, locationInContainer, accumulatedOffset, hitTestAction);
}

bool RenderFlowThread::hitTestFlowThreadPortionInRegion(RenderRegion* region, const LayoutRect& flowThreadPortionRect, const LayoutRect& flowThreadPortionOverflowRect, const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset) const
{
    LayoutRect regionClippingRect = computeRegionClippingRect(accumulatedOffset, flowThreadPortionRect, flowThreadPortionOverflowRect);
    if (!regionClippingRect.contains(locationInContainer.point()))
        return false;

    LayoutSize renderFlowThreadOffset;
    if (style()->isFlippedBlocksWritingMode()) {
        LayoutRect flippedFlowThreadPortionRect(flowThreadPortionRect);
        flipForWritingMode(flippedFlowThreadPortionRect);
        renderFlowThreadOffset = accumulatedOffset - flippedFlowThreadPortionRect.location();
    } else
        renderFlowThreadOffset = accumulatedOffset - flowThreadPortionRect.location();

    // Always ignore clipping, since the RenderFlowThread has nothing to do with the bounds of the FrameView.
    HitTestRequest newRequest(request.type() | HitTestRequest::IgnoreClipping | HitTestRequest::DisallowShadowContent);

    // Make a new temporary HitTestLocation in the new region.
    HitTestLocation newHitTestLocation(locationInContainer, -renderFlowThreadOffset, region);

    bool isPointInsideFlowThread = layer()->hitTest(newRequest, newHitTestLocation, result);

    // FIXME: Should we set result.m_localPoint back to the RenderRegion's coordinate space or leave it in the RenderFlowThread's coordinate
    // space? Right now it's staying in the RenderFlowThread's coordinate space, which may end up being ok. We will know more when we get around to
    // patching positionForPoint.
    return isPointInsideFlowThread;
}

bool RenderFlowThread::shouldRepaint(const LayoutRect& r) const
{
    if (view()->printing() || r.isEmpty())
        return false;

    return true;
}

void RenderFlowThread::repaintRectangleInRegions(const LayoutRect& repaintRect, bool immediate) const
{
    if (!shouldRepaint(repaintRect) || !hasValidRegionInfo())
        return;

    LayoutStateDisabler layoutStateDisabler(view()); // We can't use layout state to repaint, since the regions are somewhere else.

    // We can't use currentFlowThread as it is possible to have interleaved flow threads and the wrong one could be used.
    // Let each region figure out the proper enclosing flow thread.
    CurrentRenderFlowThreadDisabler disabler(view());
    
    for (RenderRegionList::const_iterator iter = m_regionList.begin(); iter != m_regionList.end(); ++iter) {
        RenderRegion* region = *iter;

        region->repaintFlowThreadContent(repaintRect, immediate);
    }
}

RenderRegion* RenderFlowThread::regionAtBlockOffset(LayoutUnit offset, bool extendLastRegion, RegionAutoGenerationPolicy autoGenerationPolicy)
{
    ASSERT(!m_regionsInvalidated);

    if (autoGenerationPolicy == AllowRegionAutoGeneration)
        autoGenerateRegionsToBlockOffset(offset);

    if (offset <= 0)
        return m_regionList.isEmpty() ? 0 : m_regionList.first();

    RegionSearchAdapter adapter(offset);
    m_regionIntervalTree.allOverlapsWithAdapter<RegionSearchAdapter>(adapter);

    // If no region was found, the offset is in the flow thread overflow.
    // The last region will contain the offset if extendLastRegion is set or if the last region is a set.
    if (!adapter.result() && !m_regionList.isEmpty() && (extendLastRegion || m_regionList.last()->isRenderRegionSet()))
        return m_regionList.last();

    return adapter.result();
}
    
LayoutPoint RenderFlowThread::adjustedPositionRelativeToOffsetParent(const RenderBoxModelObject& boxModelObject, const LayoutPoint& startPoint)
{
    LayoutPoint referencePoint = startPoint;
    
    // FIXME: This needs to be adapted for different writing modes inside the flow thread.
    RenderRegion* startRegion = regionAtBlockOffset(referencePoint.y());
    if (startRegion) {
        // Take into account the offset coordinates of the region.
        RenderBoxModelObject* currObject = startRegion;
        RenderBoxModelObject* currOffsetParent;
        while ((currOffsetParent = currObject->offsetParent())) {
            referencePoint.move(currObject->offsetLeft(), currObject->offsetTop());
            
            // Since we're looking for the offset relative to the body, we must also
            // take into consideration the borders of the region's offsetParent.
            if (currOffsetParent->isBox() && !currOffsetParent->isBody())
                referencePoint.move(toRenderBox(currOffsetParent)->borderLeft(), toRenderBox(currOffsetParent)->borderTop());
            
            currObject = currOffsetParent;
        }
        
        // We need to check if any of this box's containing blocks start in a different region
        // and if so, drop the object's top position (which was computed relative to its containing block
        // and is no longer valid) and recompute it using the region in which it flows as reference.
        bool wasComputedRelativeToOtherRegion = false;
        const RenderBlock* objContainingBlock = boxModelObject.containingBlock();
        while (objContainingBlock && !objContainingBlock->isRenderNamedFlowThread()) {
            // Check if this object is in a different region.
            RenderRegion* parentStartRegion = 0;
            RenderRegion* parentEndRegion = 0;
            getRegionRangeForBox(objContainingBlock, parentStartRegion, parentEndRegion);
            if (parentStartRegion && parentStartRegion != startRegion) {
                wasComputedRelativeToOtherRegion = true;
                break;
            }
            objContainingBlock = objContainingBlock->containingBlock();
        }
        
        if (wasComputedRelativeToOtherRegion) {
            if (boxModelObject.isBox()) {
                // Use borderBoxRectInRegion to account for variations such as percentage margins.
                LayoutRect borderBoxRect = toRenderBox(&boxModelObject)->borderBoxRectInRegion(startRegion, RenderBox::DoNotCacheRenderBoxRegionInfo);
                referencePoint.move(borderBoxRect.location().x(), 0);
            }
            
            // Get the logical top coordinate of the current object.
            LayoutUnit top = 0;
            if (boxModelObject.isRenderBlock())
                top = toRenderBlock(&boxModelObject)->offsetFromLogicalTopOfFirstPage();
            else {
                if (boxModelObject.containingBlock())
                    top = boxModelObject.containingBlock()->offsetFromLogicalTopOfFirstPage();
                
                if (boxModelObject.isBox())
                    top += toRenderBox(&boxModelObject)->topLeftLocation().y();
                else if (boxModelObject.isRenderInline())
                    top -= toRenderInline(&boxModelObject)->borderTop();
            }
            
            // Get the logical top of the region this object starts in
            // and compute the object's top, relative to the region's top.
            LayoutUnit regionLogicalTop = startRegion->pageLogicalTopForOffset(top);
            LayoutUnit topRelativeToRegion = top - regionLogicalTop;
            referencePoint.setY(startRegion->offsetTop() + topRelativeToRegion);
            
            // Since the top has been overriden, check if the
            // relative/sticky positioning must be reconsidered.
            if (boxModelObject.isRelPositioned())
                referencePoint.move(0, boxModelObject.relativePositionOffset().height());
            else if (boxModelObject.isStickyPositioned())
                referencePoint.move(0, boxModelObject.stickyPositionOffset().height());
        }
        
        // Since we're looking for the offset relative to the body, we must also
        // take into consideration the borders of the region.
        referencePoint.move(startRegion->borderLeft(), startRegion->borderTop());
    }
    
    return referencePoint;
}

LayoutUnit RenderFlowThread::pageLogicalTopForOffset(LayoutUnit offset)
{
    RenderRegion* region = regionAtBlockOffset(offset);
    return region ? region->pageLogicalTopForOffset(offset) : LayoutUnit();
}

LayoutUnit RenderFlowThread::pageLogicalWidthForOffset(LayoutUnit offset)
{
    RenderRegion* region = regionAtBlockOffset(offset, true);
    return region ? region->pageLogicalWidth() : contentLogicalWidth();
}

LayoutUnit RenderFlowThread::pageLogicalHeightForOffset(LayoutUnit offset)
{
    RenderRegion* region = regionAtBlockOffset(offset);
    if (!region)
        return 0;

    return region->pageLogicalHeight();
}

LayoutUnit RenderFlowThread::pageRemainingLogicalHeightForOffset(LayoutUnit offset, PageBoundaryRule pageBoundaryRule)
{
    RenderRegion* region = regionAtBlockOffset(offset);
    if (!region)
        return 0;

    LayoutUnit pageLogicalTop = region->pageLogicalTopForOffset(offset);
    LayoutUnit pageLogicalHeight = region->pageLogicalHeight();
    LayoutUnit pageLogicalBottom = pageLogicalTop + pageLogicalHeight;
    LayoutUnit remainingHeight = pageLogicalBottom - offset;
    if (pageBoundaryRule == IncludePageBoundary) {
        // If IncludePageBoundary is set, the line exactly on the top edge of a
        // region will act as being part of the previous region.
        remainingHeight = intMod(remainingHeight, pageLogicalHeight);
    }
    return remainingHeight;
}

RenderRegion* RenderFlowThread::mapFromFlowToRegion(TransformState& transformState) const
{
    if (!hasValidRegionInfo())
        return 0;

    LayoutRect boxRect = transformState.mappedQuad().enclosingBoundingBox();
    flipForWritingMode(boxRect);

    // FIXME: We need to refactor RenderObject::absoluteQuads to be able to split the quads across regions,
    // for now we just take the center of the mapped enclosing box and map it to a region.
    // Note: Using the center in order to avoid rounding errors.

    LayoutPoint center = boxRect.center();
    RenderRegion* renderRegion = const_cast<RenderFlowThread*>(this)->regionAtBlockOffset(isHorizontalWritingMode() ? center.y() : center.x(), true, DisallowRegionAutoGeneration);
    if (!renderRegion)
        return 0;

    LayoutRect flippedRegionRect(renderRegion->flowThreadPortionRect());
    flipForWritingMode(flippedRegionRect);

    transformState.move(renderRegion->contentBoxRect().location() - flippedRegionRect.location());

    return renderRegion;
}

void RenderFlowThread::removeRenderBoxRegionInfo(RenderBox* box)
{
    if (!hasRegions())
        return;

    // If the region chain was invalidated the next layout will clear the box information from all the regions.
    if (m_regionsInvalidated) {
        ASSERT(selfNeedsLayout());
        return;
    }

    RenderRegion* startRegion;
    RenderRegion* endRegion;
    getRegionRangeForBox(box, startRegion, endRegion);

    for (RenderRegionList::iterator iter = m_regionList.find(startRegion); iter != m_regionList.end(); ++iter) {
        RenderRegion* region = *iter;
        region->removeRenderBoxRegionInfo(box);
        if (region == endRegion)
            break;
    }

#ifndef NDEBUG
    // We have to make sure we did not leave any RenderBoxRegionInfo attached.
    for (RenderRegionList::iterator iter = m_regionList.begin(); iter != m_regionList.end(); ++iter) {
        RenderRegion* region = *iter;
        ASSERT(!region->renderBoxRegionInfo(box));
    }
#endif

    m_regionRangeMap.remove(box);
}

bool RenderFlowThread::logicalWidthChangedInRegionsForBlock(const RenderBlock* block)
{
    if (!hasRegions())
        return false;

    RenderRegion* startRegion;
    RenderRegion* endRegion;
    getRegionRangeForBox(block, startRegion, endRegion);

    // When the region chain is invalidated the box information is discarded so we must assume the width has changed.
    if (m_pageLogicalSizeChanged && !startRegion)
        return true;

    // Not necessary for the flow thread, since we already computed the correct info for it.
    if (block == this)
        return false;

    for (RenderRegionList::iterator iter = m_regionList.find(startRegion); iter != m_regionList.end(); ++iter) {
        RenderRegion* region = *iter;
        ASSERT(!region->needsLayout() || region->isRenderRegionSet());

        OwnPtr<RenderBoxRegionInfo> oldInfo = region->takeRenderBoxRegionInfo(block);
        if (!oldInfo)
            continue;

        LayoutUnit oldLogicalWidth = oldInfo->logicalWidth();
        RenderBoxRegionInfo* newInfo = block->renderBoxRegionInfo(region);
        if (!newInfo || newInfo->logicalWidth() != oldLogicalWidth)
            return true;

        if (region == endRegion)
            break;
    }

    return false;
}

LayoutUnit RenderFlowThread::contentLogicalWidthOfFirstRegion() const
{
    RenderRegion* firstValidRegionInFlow = firstRegion();
    if (!firstValidRegionInFlow)
        return 0;
    return isHorizontalWritingMode() ? firstValidRegionInFlow->contentWidth() : firstValidRegionInFlow->contentHeight();
}

LayoutUnit RenderFlowThread::contentLogicalHeightOfFirstRegion() const
{
    RenderRegion* firstValidRegionInFlow = firstRegion();
    if (!firstValidRegionInFlow)
        return 0;
    return isHorizontalWritingMode() ? firstValidRegionInFlow->contentHeight() : firstValidRegionInFlow->contentWidth();
}

LayoutUnit RenderFlowThread::contentLogicalLeftOfFirstRegion() const
{
    RenderRegion* firstValidRegionInFlow = firstRegion();
    if (!firstValidRegionInFlow)
        return 0;
    return isHorizontalWritingMode() ? firstValidRegionInFlow->flowThreadPortionRect().x() : firstValidRegionInFlow->flowThreadPortionRect().y();
}

RenderRegion* RenderFlowThread::firstRegion() const
{
    if (!hasValidRegionInfo())
        return 0;
    return m_regionList.first();
}

RenderRegion* RenderFlowThread::lastRegion() const
{
    if (!hasValidRegionInfo())
        return 0;
    return m_regionList.last();
}

void RenderFlowThread::clearRenderObjectCustomStyle(const RenderObject* object,
    const RenderRegion* oldStartRegion, const RenderRegion* oldEndRegion,
    const RenderRegion* newStartRegion, const RenderRegion* newEndRegion)
{
    // Clear the styles for the object in the regions.
    // The styles are not cleared for the regions that are contained in both ranges.
    bool insideOldRegionRange = false;
    bool insideNewRegionRange = false;
    for (RenderRegionList::iterator iter = m_regionList.begin(); iter != m_regionList.end(); ++iter) {
        RenderRegion* region = *iter;

        if (oldStartRegion == region)
            insideOldRegionRange = true;
        if (newStartRegion == region)
            insideNewRegionRange = true;

        if (!(insideOldRegionRange && insideNewRegionRange))
            region->clearObjectStyleInRegion(object);

        if (oldEndRegion == region)
            insideOldRegionRange = false;
        if (newEndRegion == region)
            insideNewRegionRange = false;
    }
}

void RenderFlowThread::setRegionRangeForBox(const RenderBox* box, LayoutUnit offsetFromLogicalTopOfFirstPage)
{
    if (!hasRegions())
        return;

    ASSERT(box->logicalHeight() >= 0);

    // FIXME: Not right for differing writing-modes.
    RenderRegion* startRegion = regionAtBlockOffset(offsetFromLogicalTopOfFirstPage, true);
    RenderRegion* endRegion = regionAtBlockOffset(offsetFromLogicalTopOfFirstPage + box->logicalHeight(), true);
    RenderRegionRangeMap::iterator it = m_regionRangeMap.find(box);
    if (it == m_regionRangeMap.end()) {
        m_regionRangeMap.set(box, RenderRegionRange(startRegion, endRegion));
        clearRenderObjectCustomStyle(box);
        return;
    }

    // If nothing changed, just bail.
    RenderRegionRange& range = it->value;
    if (range.startRegion() == startRegion && range.endRegion() == endRegion)
        return;

    // Delete any info that we find before our new startRegion and after our new endRegion.
    for (RenderRegionList::iterator iter = m_regionList.begin(); iter != m_regionList.end(); ++iter) {
        RenderRegion* region = *iter;
        if (region == startRegion) {
            iter = m_regionList.find(endRegion);
            continue;
        }

        region->removeRenderBoxRegionInfo(box);

        if (region == range.endRegion())
            break;
    }

    clearRenderObjectCustomStyle(box, range.startRegion(), range.endRegion(), startRegion, endRegion);
    range.setRange(startRegion, endRegion);
}

void RenderFlowThread::getRegionRangeForBox(const RenderBox* box, RenderRegion*& startRegion, RenderRegion*& endRegion) const
{
    startRegion = 0;
    endRegion = 0;
    RenderRegionRangeMap::const_iterator it = m_regionRangeMap.find(box);
    if (it == m_regionRangeMap.end())
        return;

    const RenderRegionRange& range = it->value;
    startRegion = range.startRegion();
    endRegion = range.endRegion();
    ASSERT(m_regionList.contains(startRegion) && m_regionList.contains(endRegion));
}

void RenderFlowThread::applyBreakAfterContent(LayoutUnit clientHeight)
{
    // Simulate a region break at height. If it points inside an auto logical height region,
    // then it may determine the region computed autoheight.
    addForcedRegionBreak(clientHeight, this, false);
}

bool RenderFlowThread::regionInRange(const RenderRegion* targetRegion, const RenderRegion* startRegion, const RenderRegion* endRegion) const
{
    ASSERT(targetRegion);

    for (RenderRegionList::const_iterator it = m_regionList.find(const_cast<RenderRegion*>(startRegion)); it != m_regionList.end(); ++it) {
        const RenderRegion* currRegion = *it;
        if (targetRegion == currRegion)
            return true;
        if (currRegion == endRegion)
            break;
    }

    return false;
}

// Check if the content is flown into at least a region with region styling rules.
void RenderFlowThread::checkRegionsWithStyling()
{
    bool hasRegionsWithStyling = false;
    for (RenderRegionList::iterator iter = m_regionList.begin(); iter != m_regionList.end(); ++iter) {
        RenderRegion* region = *iter;
        if (region->hasCustomRegionStyle()) {
            hasRegionsWithStyling = true;
            break;
        }
    }
    m_hasRegionsWithStyling = hasRegionsWithStyling;
}

bool RenderFlowThread::objectInFlowRegion(const RenderObject* object, const RenderRegion* region) const
{
    ASSERT(object);
    ASSERT(region);

    RenderFlowThread* flowThread = object->flowThreadContainingBlock();
    if (flowThread != this)
        return false;
    if (!m_regionList.contains(const_cast<RenderRegion*>(region)))
        return false;

    RenderBox* enclosingBox = object->enclosingBox();
    RenderRegion* enclosingBoxStartRegion = 0;
    RenderRegion* enclosingBoxEndRegion = 0;
    getRegionRangeForBox(enclosingBox, enclosingBoxStartRegion, enclosingBoxEndRegion);
    if (!regionInRange(region, enclosingBoxStartRegion, enclosingBoxEndRegion))
        return false;

    if (object->isBox())
        return true;

    LayoutRect objectABBRect = object->absoluteBoundingBoxRect(true);
    if (!objectABBRect.width())
        objectABBRect.setWidth(1);
    if (!objectABBRect.height())
        objectABBRect.setHeight(1); 
    if (objectABBRect.intersects(region->absoluteBoundingBoxRect(true)))
        return true;

    if (region == lastRegion()) {
        // If the object does not intersect any of the enclosing box regions
        // then the object is in last region.
        for (RenderRegionList::const_iterator it = m_regionList.find(enclosingBoxStartRegion); it != m_regionList.end(); ++it) {
            const RenderRegion* currRegion = *it;
            if (currRegion == region)
                break;
            if (objectABBRect.intersects(currRegion->absoluteBoundingBoxRect(true)))
                return false;
        }
        return true;
    }

    return false;
}

#ifndef NDEBUG
bool RenderFlowThread::isAutoLogicalHeightRegionsCountConsistent() const
{
    unsigned autoLogicalHeightRegions = 0;
    for (RenderRegionList::const_iterator iter = m_regionList.begin(); iter != m_regionList.end(); ++iter) {
        const RenderRegion* region = *iter;
        if (region->hasAutoLogicalHeight())
            autoLogicalHeightRegions++;
    }

    return autoLogicalHeightRegions == m_autoLogicalHeightRegionsCount;
}
#endif

// During the normal layout phase of the named flow the regions are initialized with a height equal to their max-height.
// This way unforced breaks are automatically placed when a region is full and the content height/position correctly estimated.
// Also, the region where a forced break falls is exactly the region found at the forced break offset inside the flow content.
void RenderFlowThread::initializeRegionsComputedAutoHeight(RenderRegion* startRegion)
{
    ASSERT(!inConstrainedLayoutPhase());
    if (!hasAutoLogicalHeightRegions())
        return;

    RenderRegionList::iterator regionIter = startRegion ? m_regionList.find(startRegion) : m_regionList.begin();
    for (; regionIter != m_regionList.end(); ++regionIter) {
        RenderRegion* region = *regionIter;
        if (region->hasAutoLogicalHeight())
            region->setComputedAutoHeight(region->maxPageLogicalHeight());
    }
}

void RenderFlowThread::markAutoLogicalHeightRegionsForLayout()
{
    ASSERT(hasAutoLogicalHeightRegions());

    for (RenderRegionList::iterator iter = m_regionList.begin(); iter != m_regionList.end(); ++iter) {
        RenderRegion* region = *iter;
        if (!region->hasAutoLogicalHeight())
            continue;

        // FIXME: We need to find a way to avoid marking all the regions ancestors for layout
        // as we are already inside layout.
        region->setNeedsLayout(true);
    }
}

void RenderFlowThread::updateRegionsFlowThreadPortionRect(const RenderRegion* lastRegionWithContent)
{
    ASSERT(!lastRegionWithContent || (!inConstrainedLayoutPhase() && hasAutoLogicalHeightRegions()));
    LayoutUnit logicalHeight = 0;
    bool emptyRegionsSegment = false;
    // FIXME: Optimize not to clear the interval all the time. This implies manually managing the tree nodes lifecycle.
    m_regionIntervalTree.clear();
    m_regionIntervalTree.initIfNeeded();
    for (RenderRegionList::iterator iter = m_regionList.begin(); iter != m_regionList.end(); ++iter) {
        RenderRegion* region = *iter;

        // If we find an empty auto-height region, clear the computedAutoHeight value.
        if (emptyRegionsSegment && region->hasAutoLogicalHeight())
            region->clearComputedAutoHeight();

        LayoutUnit regionLogicalWidth = region->pageLogicalWidth();
        LayoutUnit regionLogicalHeight = std::min<LayoutUnit>(RenderFlowThread::maxLogicalHeight() - logicalHeight, region->logicalHeightOfAllFlowThreadContent());

        LayoutRect regionRect(style()->direction() == LTR ? LayoutUnit() : logicalWidth() - regionLogicalWidth, logicalHeight, regionLogicalWidth, regionLogicalHeight);

        region->setFlowThreadPortionRect(isHorizontalWritingMode() ? regionRect : regionRect.transposedRect());

        m_regionIntervalTree.add(RegionIntervalTree::createInterval(logicalHeight, logicalHeight + regionLogicalHeight, region));

        logicalHeight += regionLogicalHeight;

        // Once we find the last region with content the next regions are considered empty.
        if (lastRegionWithContent == region)
            emptyRegionsSegment = true;
    }

    ASSERT(!lastRegionWithContent || emptyRegionsSegment);
}

// Even if we require the break to occur at offsetBreakInFlowThread, because regions may have min/max-height values,
// it is possible that the break will occur at a different offset than the original one required.
// offsetBreakAdjustment measures the different between the requested break offset and the current break offset.
bool RenderFlowThread::addForcedRegionBreak(LayoutUnit offsetBreakInFlowThread, RenderObject* breakChild, bool isBefore, LayoutUnit* offsetBreakAdjustment)
{
    // We take breaks into account for height computation for auto logical height regions
    // only in the layout phase in which we lay out the flows threads unconstrained
    // and we use the content breaks to determine the computed auto height for
    // auto logical height regions.
    if (inConstrainedLayoutPhase())
        return false;

    // Breaks can come before or after some objects. We need to track these objects, so that if we get
    // multiple breaks for the same object (for example because of multiple layouts on the same object),
    // we need to invalidate every other region after the old one and start computing from fresh.
    RenderObjectToRegionMap& mapToUse = isBefore ? m_breakBeforeToRegionMap : m_breakAfterToRegionMap;
    RenderObjectToRegionMap::iterator iter = mapToUse.find(breakChild);
    if (iter != mapToUse.end()) {
        RenderRegionList::iterator regionIter = m_regionList.find(iter->value);
        ASSERT(regionIter != m_regionList.end());
        ASSERT((*regionIter)->hasAutoLogicalHeight());
        initializeRegionsComputedAutoHeight(*regionIter);

        // We need to update the regions flow thread portion rect because we are going to process
        // a break on these regions.
        updateRegionsFlowThreadPortionRect();
    }

    // Simulate a region break at offsetBreakInFlowThread. If it points inside an auto logical height region,
    // then it determines the region computed auto height.
    RenderRegion* region = regionAtBlockOffset(offsetBreakInFlowThread);
    if (!region)
        return false;

    bool lastBreakAfterContent = breakChild == this;
    bool hasComputedAutoHeight = false;

    LayoutUnit currentRegionOffsetInFlowThread = isHorizontalWritingMode() ? region->flowThreadPortionRect().y() : region->flowThreadPortionRect().x();
    LayoutUnit offsetBreakInCurrentRegion = offsetBreakInFlowThread - currentRegionOffsetInFlowThread;

    if (region->hasAutoLogicalHeight()) {
        // A forced break can appear only in an auto-height region that didn't have a forced break before.
        // This ASSERT is a good-enough heuristic to verify the above condition.
        ASSERT(region->maxPageLogicalHeight() == region->computedAutoHeight());

        mapToUse.set(breakChild, region);

        hasComputedAutoHeight = true;

        // Compute the region height pretending that the offsetBreakInCurrentRegion is the logicalHeight for the auto-height region.
        LayoutUnit regionComputedAutoHeight = region->constrainContentBoxLogicalHeightByMinMax(offsetBreakInCurrentRegion);

        // The new height of this region needs to be smaller than the initial value, the max height. A forced break is the only way to change the initial
        // height of an auto-height region besides content ending.
        ASSERT(regionComputedAutoHeight <= region->maxPageLogicalHeight());

        region->setComputedAutoHeight(regionComputedAutoHeight);

        currentRegionOffsetInFlowThread += regionComputedAutoHeight;
    } else
        currentRegionOffsetInFlowThread += isHorizontalWritingMode() ? region->flowThreadPortionRect().height() : region->flowThreadPortionRect().width();

    // If the break was found inside an auto-height region its size changed so we need to recompute the flow thread portion rectangles.
    // Also, if this is the last break after the content we need to clear the computedAutoHeight value on the last empty regions.
    if (hasAutoLogicalHeightRegions() && lastBreakAfterContent)
        updateRegionsFlowThreadPortionRect(region);
    else if (hasComputedAutoHeight)
        updateRegionsFlowThreadPortionRect();

    if (offsetBreakAdjustment)
        *offsetBreakAdjustment = max<LayoutUnit>(0, currentRegionOffsetInFlowThread - offsetBreakInFlowThread);

    return hasComputedAutoHeight;
}

void RenderFlowThread::incrementAutoLogicalHeightRegions()
{
    if (!m_autoLogicalHeightRegionsCount)
        view()->flowThreadController()->incrementFlowThreadsWithAutoLogicalHeightRegions();
    ++m_autoLogicalHeightRegionsCount;
}

void RenderFlowThread::decrementAutoLogicalHeightRegions()
{
    ASSERT(m_autoLogicalHeightRegionsCount > 0);
    --m_autoLogicalHeightRegionsCount;
    if (!m_autoLogicalHeightRegionsCount)
        view()->flowThreadController()->decrementFlowThreadsWithAutoLogicalHeightRegions();
}

void RenderFlowThread::collectLayerFragments(LayerFragments& layerFragments, const LayoutRect& layerBoundingBox, const LayoutRect& dirtyRect)
{
    ASSERT(!m_regionsInvalidated);
    
    for (RenderRegionList::const_iterator iter = m_regionList.begin(); iter != m_regionList.end(); ++iter) {
        RenderRegion* region = *iter;
        region->collectLayerFragments(layerFragments, layerBoundingBox, dirtyRect);
    }
}

LayoutRect RenderFlowThread::fragmentsBoundingBox(const LayoutRect& layerBoundingBox)
{
    ASSERT(!m_regionsInvalidated);
    
    LayoutRect result;
    for (RenderRegionList::const_iterator iter = m_regionList.begin(); iter != m_regionList.end(); ++iter) {
        RenderRegion* region = *iter;
        LayerFragments fragments;
        region->collectLayerFragments(fragments, layerBoundingBox, PaintInfo::infiniteRect());
        for (size_t i = 0; i < fragments.size(); ++i) {
            const LayerFragment& fragment = fragments.at(i);
            LayoutRect fragmentRect(layerBoundingBox);
            fragmentRect.intersect(fragment.paginationClip);
            fragmentRect.moveBy(fragment.paginationOffset);
            result.unite(fragmentRect);
        }
    }
    
    return result;
}

bool RenderFlowThread::hasCachedOffsetFromLogicalTopOfFirstRegion(const RenderBox* box) const
{
    return m_boxesToOffsetMap.contains(box);
}

LayoutUnit RenderFlowThread::cachedOffsetFromLogicalTopOfFirstRegion(const RenderBox* box) const
{
    return m_boxesToOffsetMap.get(box);
}

void RenderFlowThread::setOffsetFromLogicalTopOfFirstRegion(const RenderBox* box, LayoutUnit offset)
{
    m_boxesToOffsetMap.set(box, offset);
}

void RenderFlowThread::clearOffsetFromLogicalTopOfFirstRegion(const RenderBox* box)
{
    ASSERT(m_boxesToOffsetMap.contains(box));
    m_boxesToOffsetMap.remove(box);
}

const RenderBox* RenderFlowThread::currentActiveRenderBox() const
{
    if (m_activeObjectsStack.isEmpty())
        return 0;

    const RenderObject* currentObject = m_activeObjectsStack.last();
    return currentObject->isBox() ? toRenderBox(currentObject) : 0;
}

void RenderFlowThread::pushFlowThreadLayoutState(const RenderObject* object)
{
    if (const RenderBox* currentBoxDescendant = currentActiveRenderBox()) {
        LayoutState* layoutState = currentBoxDescendant->view()->layoutState();
        if (layoutState && layoutState->isPaginated()) {
            ASSERT(layoutState->m_renderer == currentBoxDescendant);
            LayoutSize offsetDelta = layoutState->m_layoutOffset - layoutState->m_pageOffset;
            setOffsetFromLogicalTopOfFirstRegion(currentBoxDescendant, currentBoxDescendant->isHorizontalWritingMode() ? offsetDelta.height() : offsetDelta.width());
        }
    }

    m_activeObjectsStack.add(object);
}

void RenderFlowThread::popFlowThreadLayoutState()
{
    m_activeObjectsStack.removeLast();

    if (const RenderBox* currentBoxDescendant = currentActiveRenderBox()) {
        LayoutState* layoutState = currentBoxDescendant->view()->layoutState();
        if (layoutState && layoutState->isPaginated())
            clearOffsetFromLogicalTopOfFirstRegion(currentBoxDescendant);
    }
}

LayoutUnit RenderFlowThread::offsetFromLogicalTopOfFirstRegion(const RenderBlock* currentBlock) const
{
    // First check if we cached the offset for the block if it's an ancestor containing block of the box
    // being currently laid out.
    if (hasCachedOffsetFromLogicalTopOfFirstRegion(currentBlock))
        return cachedOffsetFromLogicalTopOfFirstRegion(currentBlock);

    // If it's the current box being laid out, use the layout state.
    const RenderBox* currentBoxDescendant = currentActiveRenderBox();
    if (currentBlock == currentBoxDescendant) {
        LayoutState* layoutState = view()->layoutState();
        ASSERT(layoutState->m_renderer == currentBlock);
        ASSERT(layoutState && layoutState->isPaginated());
        LayoutSize offsetDelta = layoutState->m_layoutOffset - layoutState->m_pageOffset;
        return currentBoxDescendant->isHorizontalWritingMode() ? offsetDelta.height() : offsetDelta.width();
    }

    // As a last resort, take the slow path.
    LayoutRect blockRect(0, 0, currentBlock->width(), currentBlock->height());
    while (currentBlock && !currentBlock->isRenderFlowThread()) {
        RenderBlock* containerBlock = currentBlock->containingBlock();
        ASSERT(containerBlock);
        if (!containerBlock)
            return 0;
        LayoutPoint currentBlockLocation = currentBlock->location();

        if (containerBlock->style()->writingMode() != currentBlock->style()->writingMode()) {
            // We have to put the block rect in container coordinates
            // and we have to take into account both the container and current block flipping modes
            if (containerBlock->style()->isFlippedBlocksWritingMode()) {
                if (containerBlock->isHorizontalWritingMode())
                    blockRect.setY(currentBlock->height() - blockRect.maxY());
                else
                    blockRect.setX(currentBlock->width() - blockRect.maxX());
            }
            currentBlock->flipForWritingMode(blockRect);
        }
        blockRect.moveBy(currentBlockLocation);
        currentBlock = containerBlock;
    }

    return currentBlock->isHorizontalWritingMode() ? blockRect.y() : blockRect.x();
}

void RenderFlowThread::RegionSearchAdapter::collectIfNeeded(const RegionInterval& interval)
{
    if (m_result)
        return;
    if (interval.low() <= m_offset && interval.high() > m_offset)
        m_result = interval.data();
}

void RenderFlowThread::mapLocalToContainer(const RenderLayerModelObject* repaintContainer, TransformState& transformState, MapCoordinatesFlags mode, bool* wasFixed) const
{
    if (this == repaintContainer)
        return;

    if (RenderRegion* region = mapFromFlowToRegion(transformState))
        // FIXME: The cast below is probably not the best solution, we may need to find a better way.
        static_cast<const RenderObject*>(region)->mapLocalToContainer(region->containerForRepaint(), transformState, mode, wasFixed);
}

CurrentRenderFlowThreadMaintainer::CurrentRenderFlowThreadMaintainer(RenderFlowThread* renderFlowThread)
    : m_renderFlowThread(renderFlowThread)
    , m_previousRenderFlowThread(0)
{
    if (!m_renderFlowThread)
        return;
    RenderView* view = m_renderFlowThread->view();
    m_previousRenderFlowThread = view->flowThreadController()->currentRenderFlowThread();
    ASSERT(!m_previousRenderFlowThread || !renderFlowThread->isRenderNamedFlowThread());
    view->flowThreadController()->setCurrentRenderFlowThread(m_renderFlowThread);
}

CurrentRenderFlowThreadMaintainer::~CurrentRenderFlowThreadMaintainer()
{
    if (!m_renderFlowThread)
        return;
    RenderView* view = m_renderFlowThread->view();
    ASSERT(view->flowThreadController()->currentRenderFlowThread() == m_renderFlowThread);
    view->flowThreadController()->setCurrentRenderFlowThread(m_previousRenderFlowThread);
}


} // namespace WebCore
