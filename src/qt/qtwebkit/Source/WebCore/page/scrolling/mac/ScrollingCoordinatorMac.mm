/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(THREADED_SCROLLING)

#import "ScrollingCoordinatorMac.h"

#include "GraphicsLayer.h"
#include "Frame.h"
#include "FrameView.h"
#include "IntRect.h"
#include "Page.h"
#include "PlatformWheelEvent.h"
#include "PluginViewBase.h"
#include "Region.h"
#include "RenderLayerCompositor.h"
#include "RenderView.h"
#include "ScrollAnimator.h"
#include "ScrollingConstraints.h"
#include "ScrollingStateFixedNode.h"
#include "ScrollingStateScrollingNode.h"
#include "ScrollingStateStickyNode.h"
#include "ScrollingStateTree.h"
#include "ScrollingThread.h"
#include "ScrollingTree.h"
#include "TiledBacking.h"

#include <wtf/Functional.h>
#include <wtf/MainThread.h>
#include <wtf/PassRefPtr.h>


namespace WebCore {

class ScrollingCoordinatorPrivate {
};

ScrollingCoordinatorMac::ScrollingCoordinatorMac(Page* page)
    : ScrollingCoordinator(page)
    , m_scrollingStateTree(ScrollingStateTree::create())
    , m_scrollingTree(ScrollingTree::create(this))
    , m_scrollingStateTreeCommitterTimer(this, &ScrollingCoordinatorMac::scrollingStateTreeCommitterTimerFired)
{
}

ScrollingCoordinatorMac::~ScrollingCoordinatorMac()
{
    ASSERT(!m_scrollingTree);
}

void ScrollingCoordinatorMac::pageDestroyed()
{
    ScrollingCoordinator::pageDestroyed();

    m_scrollingStateTreeCommitterTimer.stop();

    // Invalidating the scrolling tree will break the reference cycle between the ScrollingCoordinator and ScrollingTree objects.
    ScrollingThread::dispatch(bind(&ScrollingTree::invalidate, m_scrollingTree.release()));
}

ScrollingTree* ScrollingCoordinatorMac::scrollingTree() const
{
    ASSERT(m_scrollingTree);
    return m_scrollingTree.get();
}

bool ScrollingCoordinatorMac::isRubberBandInProgress() const
{
    return scrollingTree()->isRubberBandInProgress();
}

bool ScrollingCoordinatorMac::rubberBandsAtBottom() const
{
    return scrollingTree()->rubberBandsAtBottom();
}

void ScrollingCoordinatorMac::setRubberBandsAtBottom(bool rubberBandsAtBottom)
{
    scrollingTree()->setRubberBandsAtBottom(rubberBandsAtBottom);
}

bool ScrollingCoordinatorMac::rubberBandsAtTop() const
{
    return scrollingTree()->rubberBandsAtTop();
}

void ScrollingCoordinatorMac::setRubberBandsAtTop(bool rubberBandsAtTop)
{
    scrollingTree()->setRubberBandsAtTop(rubberBandsAtTop);
}
    
void ScrollingCoordinatorMac::setScrollPinningBehavior(ScrollPinningBehavior pinning)
{
    scrollingTree()->setScrollPinningBehavior(pinning);
}

void ScrollingCoordinatorMac::commitTreeStateIfNeeded()
{
    if (!m_scrollingStateTree->hasChangedProperties())
        return;

    commitTreeState();
    m_scrollingStateTreeCommitterTimer.stop();
}

void ScrollingCoordinatorMac::frameViewLayoutUpdated(FrameView* frameView)
{
    ASSERT(isMainThread());
    ASSERT(m_page);

    // If there isn't a root node yet, don't do anything. We'll be called again after creating one.
    if (!m_scrollingStateTree->rootStateNode())
        return;

    // Compute the region of the page that we can't do fast scrolling for. This currently includes
    // all scrollable areas, such as subframes, overflow divs and list boxes. We need to do this even if the
    // frame view whose layout was updated is not the main frame.
    Region nonFastScrollableRegion = computeNonFastScrollableRegion(m_page->mainFrame(), IntPoint());

    // In the future, we may want to have the ability to set non-fast scrolling regions for more than
    // just the root node. But right now, this concept only applies to the root.
    setNonFastScrollableRegionForNode(nonFastScrollableRegion, m_scrollingStateTree->rootStateNode());

    if (!coordinatesScrollingForFrameView(frameView))
        return;

    ScrollingStateScrollingNode* node = toScrollingStateScrollingNode(m_scrollingStateTree->stateNodeForID(frameView->scrollLayerID()));
    if (!node)
        return;

    ScrollParameters scrollParameters;
    scrollParameters.horizontalScrollElasticity = frameView->horizontalScrollElasticity();
    scrollParameters.verticalScrollElasticity = frameView->verticalScrollElasticity();
    scrollParameters.hasEnabledHorizontalScrollbar = frameView->horizontalScrollbar() && frameView->horizontalScrollbar()->enabled();
    scrollParameters.hasEnabledVerticalScrollbar = frameView->verticalScrollbar() && frameView->verticalScrollbar()->enabled();
    scrollParameters.horizontalScrollbarMode = frameView->horizontalScrollbarMode();
    scrollParameters.verticalScrollbarMode = frameView->verticalScrollbarMode();

    scrollParameters.scrollOrigin = frameView->scrollOrigin();
    scrollParameters.viewportRect = IntRect(IntPoint(), frameView->visibleContentRect().size());
    scrollParameters.totalContentsSize = frameView->totalContentsSize();
    scrollParameters.frameScaleFactor = frameView->frame()->frameScaleFactor();
    scrollParameters.headerHeight = frameView->headerHeight();
    scrollParameters.footerHeight = frameView->footerHeight();

    setScrollParametersForNode(scrollParameters, node);
}

void ScrollingCoordinatorMac::recomputeWheelEventHandlerCountForFrameView(FrameView* frameView)
{
    ScrollingStateScrollingNode* node = toScrollingStateScrollingNode(m_scrollingStateTree->stateNodeForID(frameView->scrollLayerID()));
    if (!node)
        return;
    setWheelEventHandlerCountForNode(computeCurrentWheelEventHandlerCount(), node);
}

void ScrollingCoordinatorMac::frameViewRootLayerDidChange(FrameView* frameView)
{
    ASSERT(isMainThread());
    ASSERT(m_page);

    if (!coordinatesScrollingForFrameView(frameView))
        return;

    // If the root layer does not have a ScrollingStateNode, then we should create one.
    ensureRootStateNodeForFrameView(frameView);
    ASSERT(m_scrollingStateTree->rootStateNode());

    ScrollingCoordinator::frameViewRootLayerDidChange(frameView);

    ScrollingStateScrollingNode* node = toScrollingStateScrollingNode(m_scrollingStateTree->stateNodeForID(frameView->scrollLayerID()));
    setScrollLayerForNode(scrollLayerForFrameView(frameView), node);
    setCounterScrollingLayerForNode(counterScrollingLayerForFrameView(frameView), node);
    setHeaderLayerForNode(headerLayerForFrameView(frameView), node);
    setFooterLayerForNode(footerLayerForFrameView(frameView), node);
}

void ScrollingCoordinatorMac::scrollableAreaScrollbarLayerDidChange(ScrollableArea* scrollableArea, ScrollbarOrientation)
{
    ASSERT(isMainThread());
    ASSERT(m_page);

    if (scrollableArea != static_cast<ScrollableArea*>(m_page->mainFrame()->view()))
        return;

    // FIXME: Implement.
}

bool ScrollingCoordinatorMac::requestScrollPositionUpdate(FrameView* frameView, const IntPoint& scrollPosition)
{
    ASSERT(isMainThread());
    ASSERT(m_page);

    if (!coordinatesScrollingForFrameView(frameView))
        return false;

    if (frameView->inProgrammaticScroll() || frameView->frame()->document()->inPageCache())
        updateMainFrameScrollPosition(scrollPosition, frameView->inProgrammaticScroll(), SetScrollingLayerPosition);

    // If this frame view's document is being put into the page cache, we don't want to update our
    // main frame scroll position. Just let the FrameView think that we did.
    if (frameView->frame()->document()->inPageCache())
        return true;

    ScrollingStateScrollingNode* stateNode = toScrollingStateScrollingNode(m_scrollingStateTree->stateNodeForID(frameView->scrollLayerID()));
    if (!stateNode)
        return false;

    stateNode->setRequestedScrollPosition(scrollPosition, frameView->inProgrammaticScroll());
    scheduleTreeStateCommit();
    return true;
}

bool ScrollingCoordinatorMac::handleWheelEvent(FrameView*, const PlatformWheelEvent& wheelEvent)
{
    ASSERT(isMainThread());
    ASSERT(m_page);

    if (m_scrollingTree->willWheelEventStartSwipeGesture(wheelEvent))
        return false;

    ScrollingThread::dispatch(bind(&ScrollingTree::handleWheelEvent, m_scrollingTree.get(), wheelEvent));

    return true;
}

ScrollingNodeID ScrollingCoordinatorMac::attachToStateTree(ScrollingNodeType nodeType, ScrollingNodeID newNodeID, ScrollingNodeID parentID)
{
    return m_scrollingStateTree->attachNode(nodeType, newNodeID, parentID);
}

void ScrollingCoordinatorMac::detachFromStateTree(ScrollingNodeID nodeID)
{
    m_scrollingStateTree->detachNode(nodeID);
}

void ScrollingCoordinatorMac::clearStateTree()
{
    m_scrollingStateTree->clear();
}

void ScrollingCoordinatorMac::ensureRootStateNodeForFrameView(FrameView* frameView)
{
    ASSERT(frameView->scrollLayerID());
    attachToStateTree(ScrollingNode, frameView->scrollLayerID(), 0);
}

void ScrollingCoordinatorMac::setScrollLayerForNode(GraphicsLayer* scrollLayer, ScrollingStateNode* node)
{
    node->setScrollLayer(scrollLayer);
    scheduleTreeStateCommit();
}

void ScrollingCoordinatorMac::setCounterScrollingLayerForNode(GraphicsLayer* layer, ScrollingStateScrollingNode* node)
{
    node->setCounterScrollingLayer(layer);
    scheduleTreeStateCommit();
}

void ScrollingCoordinatorMac::setHeaderLayerForNode(GraphicsLayer* headerLayer, ScrollingStateScrollingNode* node)
{
    // Headers and footers are only supported on the root node.
    ASSERT(node == m_scrollingStateTree->rootStateNode());

    node->setHeaderLayer(headerLayer);
    scheduleTreeStateCommit();
}

void ScrollingCoordinatorMac::setFooterLayerForNode(GraphicsLayer* footerLayer, ScrollingStateScrollingNode* node)
{
    // Headers and footers are only supported on the root node.
    ASSERT(node == m_scrollingStateTree->rootStateNode());

    node->setFooterLayer(footerLayer);
    scheduleTreeStateCommit();
}

void ScrollingCoordinatorMac::setNonFastScrollableRegionForNode(const Region& region, ScrollingStateScrollingNode* node)
{
    node->setNonFastScrollableRegion(region);
    scheduleTreeStateCommit();
}

void ScrollingCoordinatorMac::setScrollParametersForNode(const ScrollParameters& scrollParameters, ScrollingStateScrollingNode* node)
{
    node->setHorizontalScrollElasticity(scrollParameters.horizontalScrollElasticity);
    node->setVerticalScrollElasticity(scrollParameters.verticalScrollElasticity);
    node->setHasEnabledHorizontalScrollbar(scrollParameters.hasEnabledHorizontalScrollbar);
    node->setHasEnabledVerticalScrollbar(scrollParameters.hasEnabledVerticalScrollbar);
    node->setHorizontalScrollbarMode(scrollParameters.horizontalScrollbarMode);
    node->setVerticalScrollbarMode(scrollParameters.verticalScrollbarMode);

    node->setScrollOrigin(scrollParameters.scrollOrigin);
    node->setViewportRect(scrollParameters.viewportRect);
    node->setTotalContentsSize(scrollParameters.totalContentsSize);
    node->setFrameScaleFactor(scrollParameters.frameScaleFactor);
    node->setHeaderHeight(scrollParameters.headerHeight);
    node->setFooterHeight(scrollParameters.footerHeight);

    scheduleTreeStateCommit();
}

void ScrollingCoordinatorMac::setWheelEventHandlerCountForNode(unsigned wheelEventHandlerCount, ScrollingStateScrollingNode* node)
{
    node->setWheelEventHandlerCount(wheelEventHandlerCount);
    scheduleTreeStateCommit();
}

void ScrollingCoordinatorMac::setShouldUpdateScrollLayerPositionOnMainThread(MainThreadScrollingReasons reasons)
{
    if (!m_scrollingStateTree->rootStateNode())
        return;

    // The FrameView's GraphicsLayer is likely to be out-of-synch with the PlatformLayer
    // at this point. So we'll update it before we switch back to main thread scrolling
    // in order to avoid layer positioning bugs.
    if (reasons)
        updateMainFrameScrollLayerPosition();
    m_scrollingStateTree->rootStateNode()->setShouldUpdateScrollLayerPositionOnMainThread(reasons);
    scheduleTreeStateCommit();
}

void ScrollingCoordinatorMac::updateMainFrameScrollLayerPosition()
{
    ASSERT(isMainThread());

    if (!m_page)
        return;

    FrameView* frameView = m_page->mainFrame()->view();
    if (!frameView)
        return;

    if (GraphicsLayer* scrollLayer = scrollLayerForFrameView(frameView))
        scrollLayer->setPosition(-frameView->scrollPosition());
}

void ScrollingCoordinatorMac::syncChildPositions(const LayoutRect& viewportRect)
{
    if (!m_scrollingStateTree->rootStateNode())
        return;

    Vector<OwnPtr<ScrollingStateNode> >* children = m_scrollingStateTree->rootStateNode()->children();
    if (!children)
        return;

    // FIXME: We'll have to traverse deeper into the tree at some point.
    size_t size = children->size();
    for (size_t i = 0; i < size; ++i) {
        ScrollingStateNode* child = children->at(i).get();
        child->syncLayerPositionForViewportRect(viewportRect);
    }
}

void ScrollingCoordinatorMac::updateScrollingNode(ScrollingNodeID nodeID, GraphicsLayer* scrollLayer, GraphicsLayer* counterScrollingLayer)
{
    ScrollingStateScrollingNode* node = toScrollingStateScrollingNode(m_scrollingStateTree->stateNodeForID(nodeID));
    ASSERT(node);
    if (!node)
        return;

    node->setScrollLayer(scrollLayer);
    node->setCounterScrollingLayer(counterScrollingLayer);
    scheduleTreeStateCommit();
}

void ScrollingCoordinatorMac::updateViewportConstrainedNode(ScrollingNodeID nodeID, const ViewportConstraints& constraints, GraphicsLayer* graphicsLayer)
{
    ASSERT(supportsFixedPositionLayers());

    ScrollingStateNode* node = m_scrollingStateTree->stateNodeForID(nodeID);
    if (!node)
        return;

    switch (constraints.constraintType()) {
    case ViewportConstraints::FixedPositionConstaint: {
        ScrollingStateFixedNode* fixedNode = toScrollingStateFixedNode(node);
        setScrollLayerForNode(graphicsLayer, fixedNode);
        fixedNode->updateConstraints((const FixedPositionViewportConstraints&)constraints);
        break;
    }
    case ViewportConstraints::StickyPositionConstraint: {
        ScrollingStateStickyNode* stickyNode = toScrollingStateStickyNode(node);
        setScrollLayerForNode(graphicsLayer, stickyNode);
        stickyNode->updateConstraints((const StickyPositionViewportConstraints&)constraints);
        break;
    }
    }
    scheduleTreeStateCommit();
}

void ScrollingCoordinatorMac::scheduleTreeStateCommit()
{
    if (m_scrollingStateTreeCommitterTimer.isActive())
        return;

    if (!m_scrollingStateTree->hasChangedProperties())
        return;

    m_scrollingStateTreeCommitterTimer.startOneShot(0);
}

void ScrollingCoordinatorMac::scrollingStateTreeCommitterTimerFired(Timer<ScrollingCoordinatorMac>*)
{
    commitTreeState();
}

void ScrollingCoordinatorMac::commitTreeState()
{
    ASSERT(m_scrollingStateTree->hasChangedProperties());

    OwnPtr<ScrollingStateTree> treeState = m_scrollingStateTree->commit();
    ScrollingThread::dispatch(bind(&ScrollingTree::commitNewTreeState, m_scrollingTree.get(), treeState.release()));

    FrameView* frameView = m_page->mainFrame()->view();
    if (!frameView)
        return;
    
    TiledBacking* tiledBacking = frameView->tiledBacking();
    if (!tiledBacking)
        return;

    ScrollingModeIndication indicatorMode;
    if (shouldUpdateScrollLayerPositionOnMainThread())
        indicatorMode = MainThreadScrollingBecauseOfStyleIndication;
    else if (m_scrollingStateTree->rootStateNode() && m_scrollingStateTree->rootStateNode()->wheelEventHandlerCount())
        indicatorMode =  MainThreadScrollingBecauseOfEventHandlersIndication;
    else
        indicatorMode = ThreadedScrollingIndication;
    
    tiledBacking->setScrollingModeIndication(indicatorMode);
}

String ScrollingCoordinatorMac::scrollingStateTreeAsText() const
{
    if (m_scrollingStateTree->rootStateNode())
        return m_scrollingStateTree->rootStateNode()->scrollingStateTreeAsText();

    return String();
}

} // namespace WebCore

#endif // ENABLE(THREADED_SCROLLING)
