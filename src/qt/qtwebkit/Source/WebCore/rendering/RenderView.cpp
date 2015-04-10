/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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
#include "RenderView.h"

#include "ColumnInfo.h"
#include "Document.h"
#include "Element.h"
#include "FloatQuad.h"
#include "FlowThreadController.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HTMLFrameOwnerElement.h"
#include "HTMLIFrameElement.h"
#include "HitTestResult.h"
#include "Page.h"
#include "RenderGeometryMap.h"
#include "RenderLayer.h"
#include "RenderLayerBacking.h"
#include "RenderNamedFlowThread.h"
#include "RenderSelectionInfo.h"
#include "RenderWidget.h"
#include "RenderWidgetProtector.h"
#include "StyleInheritedData.h"
#include "TransformState.h"
#include <wtf/StackStats.h>

#if USE(ACCELERATED_COMPOSITING)
#include "RenderLayerCompositor.h"
#endif

#if ENABLE(CSS_SHADERS) && USE(3D_GRAPHICS)
#include "CustomFilterGlobalContext.h"
#endif

namespace WebCore {

RenderView::RenderView(Document* document)
    : RenderBlock(document)
    , m_frameView(document->view())
    , m_selectionStart(0)
    , m_selectionEnd(0)
    , m_selectionStartPos(-1)
    , m_selectionEndPos(-1)
    , m_maximalOutlineSize(0)
    , m_pageLogicalHeight(0)
    , m_pageLogicalHeightChanged(false)
    , m_layoutState(0)
    , m_layoutStateDisableCount(0)
    , m_renderQuoteHead(0)
    , m_renderCounterCount(0)
    , m_selectionWasCaret(false)
{
    // init RenderObject attributes
    setInline(false);
    
    m_minPreferredLogicalWidth = 0;
    m_maxPreferredLogicalWidth = 0;

    setPreferredLogicalWidthsDirty(true, MarkOnlyThis);
    
    setPositionState(AbsolutePosition); // to 0,0 :)
}

RenderView::~RenderView()
{
}

bool RenderView::hitTest(const HitTestRequest& request, HitTestResult& result)
{
    return hitTest(request, result.hitTestLocation(), result);
}

bool RenderView::hitTest(const HitTestRequest& request, const HitTestLocation& location, HitTestResult& result)
{
    if (layer()->hitTest(request, location, result))
        return true;

    // FIXME: Consider if this test should be done unconditionally.
    if (request.allowsFrameScrollbars() && m_frameView) {
        // ScrollView scrollbars are not the same as RenderLayer scrollbars tested by RenderLayer::hitTestOverflowControls,
        // so we need to test ScrollView scrollbars separately here.
        Scrollbar* frameScrollbar = m_frameView->scrollbarAtPoint(location.roundedPoint());
        if (frameScrollbar) {
            result.setScrollbar(frameScrollbar);
            return true;
        }
    }

    return false;
}

void RenderView::computeLogicalHeight(LayoutUnit logicalHeight, LayoutUnit, LogicalExtentComputedValues& computedValues) const
{
    computedValues.m_extent = (!shouldUsePrintingLayout() && m_frameView) ? LayoutUnit(viewLogicalHeight()) : logicalHeight;
}

void RenderView::updateLogicalWidth()
{
    if (!shouldUsePrintingLayout() && m_frameView)
        setLogicalWidth(viewLogicalWidth());
}

LayoutUnit RenderView::availableLogicalHeight(AvailableLogicalHeightType heightType) const
{
    // If we have columns, then the available logical height is reduced to the column height.
    if (hasColumns())
        return columnInfo()->columnHeight();
    return RenderBlock::availableLogicalHeight(heightType);
}

bool RenderView::isChildAllowed(RenderObject* child, RenderStyle*) const
{
    return child->isBox();
}

void RenderView::layoutContent(const LayoutState& state)
{
    UNUSED_PARAM(state);
    ASSERT(needsLayout());

    RenderBlock::layout();
    if (hasRenderNamedFlowThreads())
        flowThreadController()->layoutRenderNamedFlowThreads();
#ifndef NDEBUG
    checkLayoutState(state);
#endif
}

#ifndef NDEBUG
void RenderView::checkLayoutState(const LayoutState& state)
{
    ASSERT(layoutDeltaMatches(LayoutSize()));
    ASSERT(!m_layoutStateDisableCount);
    ASSERT(m_layoutState == &state);
}
#endif

static RenderBox* enclosingSeamlessRenderer(Document* doc)
{
    if (!doc)
        return 0;
    Element* ownerElement = doc->seamlessParentIFrame();
    if (!ownerElement)
        return 0;
    return ownerElement->renderBox();
}

void RenderView::addChild(RenderObject* newChild, RenderObject* beforeChild)
{
    // Seamless iframes are considered part of an enclosing render flow thread from the parent document. This is necessary for them to look
    // up regions in the parent document during layout.
    if (newChild && !newChild->isRenderFlowThread()) {
        RenderBox* seamlessBox = enclosingSeamlessRenderer(document());
        if (seamlessBox && seamlessBox->flowThreadContainingBlock())
            newChild->setFlowThreadState(seamlessBox->flowThreadState());
    }
    RenderBlock::addChild(newChild, beforeChild);
}

bool RenderView::initializeLayoutState(LayoutState& state)
{
    bool isSeamlessAncestorInFlowThread = false;

    // FIXME: May be better to push a clip and avoid issuing offscreen repaints.
    state.m_clipped = false;
    
    // Check the writing mode of the seamless ancestor. It has to match our document's writing mode, or we won't inherit any
    // pagination information.
    RenderBox* seamlessAncestor = enclosingSeamlessRenderer(document());
    LayoutState* seamlessLayoutState = seamlessAncestor ? seamlessAncestor->view()->layoutState() : 0;
    bool shouldInheritPagination = seamlessLayoutState && !m_pageLogicalHeight && seamlessAncestor->style()->writingMode() == style()->writingMode();
    
    state.m_pageLogicalHeight = shouldInheritPagination ? seamlessLayoutState->m_pageLogicalHeight : m_pageLogicalHeight;
    state.m_pageLogicalHeightChanged = shouldInheritPagination ? seamlessLayoutState->m_pageLogicalHeightChanged : m_pageLogicalHeightChanged;
    state.m_isPaginated = state.m_pageLogicalHeight;
    if (state.m_isPaginated && shouldInheritPagination) {
        // Set up the correct pagination offset. We can use a negative offset in order to push the top of the RenderView into its correct place
        // on a page. We can take the iframe's offset from the logical top of the first page and make the negative into the pagination offset within the child
        // view.
        bool isFlipped = seamlessAncestor->style()->isFlippedBlocksWritingMode();
        LayoutSize layoutOffset = seamlessLayoutState->layoutOffset();
        LayoutSize iFrameOffset(layoutOffset.width() + seamlessAncestor->x() + (!isFlipped ? seamlessAncestor->borderLeft() + seamlessAncestor->paddingLeft() :
            seamlessAncestor->borderRight() + seamlessAncestor->paddingRight()),
            layoutOffset.height() + seamlessAncestor->y() + (!isFlipped ? seamlessAncestor->borderTop() + seamlessAncestor->paddingTop() :
            seamlessAncestor->borderBottom() + seamlessAncestor->paddingBottom()));
        
        LayoutSize offsetDelta = seamlessLayoutState->m_pageOffset - iFrameOffset;
        state.m_pageOffset = offsetDelta;
        
        // Set the current render flow thread to point to our ancestor. This will allow the seamless document to locate the correct
        // regions when doing a layout.
        if (seamlessAncestor->flowThreadContainingBlock()) {
            flowThreadController()->setCurrentRenderFlowThread(seamlessAncestor->view()->flowThreadController()->currentRenderFlowThread());
            isSeamlessAncestorInFlowThread = true;
        }
    }

    // FIXME: We need to make line grids and exclusions work with seamless iframes as well here. Basically all layout state information needs
    // to propagate here and not just pagination information.
    return isSeamlessAncestorInFlowThread;
}

// The algorithm below assumes this is a full layout. In case there are previously computed values for regions, supplemental steps are taken
// to ensure the results are the same as those obtained from a full layout (i.e. the auto-height regions from all the flows are marked as needing
// layout).
// 1. The flows are laid out from the outer flow to the inner flow. This successfully computes the outer non-auto-height regions size so the 
// inner flows have the necessary information to correctly fragment the content.
// 2. The flows are laid out from the inner flow to the outer flow. After an inner flow is laid out it goes into the constrained layout phase
// and marks the auto-height regions they need layout. This means the outer flows will relayout if they depend on regions with auto-height regions
// belonging to inner flows. This step will correctly set the computedAutoHeight for the auto-height regions. It's possible for non-auto-height
// regions to relayout if they depend on auto-height regions. This will invalidate the inner flow threads and mark them as needing layout.
// 3. The last step is to do one last layout if there are pathological dependencies between non-auto-height regions and auto-height regions
// as detected in the previous step.
void RenderView::layoutContentInAutoLogicalHeightRegions(const LayoutState& state)
{
    // We need to invalidate all the flows with auto-height regions if one such flow needs layout.
    // If none is found we do a layout a check back again afterwards.
    if (!flowThreadController()->updateFlowThreadsNeedingLayout()) {
        // Do a first layout of the content. In some cases more layouts are not needed (e.g. only flows with non-auto-height regions have changed).
        layoutContent(state);

        // If we find no named flow needing a two step layout after the first layout, exit early.
        // Otherwise, initiate the two step layout algorithm and recompute all the flows.
        if (!flowThreadController()->updateFlowThreadsNeedingTwoStepLayout())
            return;
    }

    // Layout to recompute all the named flows with auto-height regions.
    layoutContent(state);

    // Propagate the computed auto-height values upwards.
    // Non-auto-height regions may invalidate the flow thread because they depended on auto-height regions, but that's ok.
    flowThreadController()->updateFlowThreadsIntoConstrainedPhase();

    // Do one last layout that should update the auto-height regions found in the main flow
    // and solve pathological dependencies between regions (e.g. a non-auto-height region depending
    // on an auto-height one).
    if (needsLayout())
        layoutContent(state);
}

void RenderView::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;
    if (!document()->paginated())
        setPageLogicalHeight(0);

    if (shouldUsePrintingLayout())
        m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth = logicalWidth();

    // Use calcWidth/Height to get the new width/height, since this will take the full page zoom factor into account.
    bool relayoutChildren = !shouldUsePrintingLayout() && (!m_frameView || width() != viewWidth() || height() != viewHeight());
    if (relayoutChildren) {
        setChildNeedsLayout(true, MarkOnlyThis);
        for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
            if ((child->isBox() && (toRenderBox(child)->hasRelativeLogicalHeight() || toRenderBox(child)->hasViewportPercentageLogicalHeight()))
                    || child->style()->logicalHeight().isPercent()
                    || child->style()->logicalMinHeight().isPercent()
                    || child->style()->logicalMaxHeight().isPercent()
                    || child->style()->logicalHeight().isViewportPercentage()
                    || child->style()->logicalMinHeight().isViewportPercentage()
                    || child->style()->logicalMaxHeight().isViewportPercentage()
#if ENABLE(SVG)
                    || child->isSVGRoot()
#endif
                )
                child->setChildNeedsLayout(true, MarkOnlyThis);
        }
    }

    ASSERT(!m_layoutState);
    if (!needsLayout())
        return;

    LayoutState state;
    bool isSeamlessAncestorInFlowThread = initializeLayoutState(state);

    m_pageLogicalHeightChanged = false;
    m_layoutState = &state;

    if (checkTwoPassLayoutForAutoHeightRegions())
        layoutContentInAutoLogicalHeightRegions(state);
    else
        layoutContent(state);

#ifndef NDEBUG
    checkLayoutState(state);
#endif
    m_layoutState = 0;
    setNeedsLayout(false);
    
    if (isSeamlessAncestorInFlowThread)
        flowThreadController()->setCurrentRenderFlowThread(0);
}

LayoutUnit RenderView::pageOrViewLogicalHeight() const
{
    if (document()->printing())
        return pageLogicalHeight();
    
    if (hasColumns() && !style()->hasInlineColumnAxis()) {
        if (int pageLength = frameView()->pagination().pageLength)
            return pageLength;
    }

    return viewLogicalHeight();
}

void RenderView::mapLocalToContainer(const RenderLayerModelObject* repaintContainer, TransformState& transformState, MapCoordinatesFlags mode, bool* wasFixed) const
{
    // If a container was specified, and was not 0 or the RenderView,
    // then we should have found it by now.
    ASSERT_ARG(repaintContainer, !repaintContainer || repaintContainer == this);
    ASSERT_UNUSED(wasFixed, !wasFixed || *wasFixed == (mode & IsFixed));

    if (!repaintContainer && mode & UseTransforms && shouldUseTransformFromContainer(0)) {
        TransformationMatrix t;
        getTransformFromContainer(0, LayoutSize(), t);
        transformState.applyTransform(t);
    }
    
    if (mode & IsFixed && m_frameView)
        transformState.move(m_frameView->scrollOffsetForFixedPosition());
}

const RenderObject* RenderView::pushMappingToContainer(const RenderLayerModelObject* ancestorToStopAt, RenderGeometryMap& geometryMap) const
{
    // If a container was specified, and was not 0 or the RenderView,
    // then we should have found it by now.
    ASSERT_ARG(ancestorToStopAt, !ancestorToStopAt || ancestorToStopAt == this);

    LayoutSize scrollOffset;

    if (m_frameView)
        scrollOffset = m_frameView->scrollOffsetForFixedPosition();

    if (!ancestorToStopAt && shouldUseTransformFromContainer(0)) {
        TransformationMatrix t;
        getTransformFromContainer(0, LayoutSize(), t);
        geometryMap.pushView(this, scrollOffset, &t);
    } else
        geometryMap.pushView(this, scrollOffset);

    return 0;
}

void RenderView::mapAbsoluteToLocalPoint(MapCoordinatesFlags mode, TransformState& transformState) const
{
    if (mode & IsFixed && m_frameView)
        transformState.move(m_frameView->scrollOffsetForFixedPosition());

    if (mode & UseTransforms && shouldUseTransformFromContainer(0)) {
        TransformationMatrix t;
        getTransformFromContainer(0, LayoutSize(), t);
        transformState.applyTransform(t);
    }
}

bool RenderView::requiresColumns(int desiredColumnCount) const
{
    if (m_frameView)
        return m_frameView->pagination().mode != Pagination::Unpaginated;

    return RenderBlock::requiresColumns(desiredColumnCount);
}

void RenderView::calcColumnWidth()
{
    int columnWidth = contentLogicalWidth();
    if (m_frameView && style()->hasInlineColumnAxis()) {
        if (int pageLength = m_frameView->pagination().pageLength)
            columnWidth = pageLength;
    }
    setDesiredColumnCountAndWidth(1, columnWidth);
}

ColumnInfo::PaginationUnit RenderView::paginationUnit() const
{
    if (m_frameView)
        return m_frameView->pagination().behavesLikeColumns ? ColumnInfo::Column : ColumnInfo::Page;

    return ColumnInfo::Page;
}

void RenderView::paint(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    // If we ever require layout but receive a paint anyway, something has gone horribly wrong.
    ASSERT(!needsLayout());
    // RenderViews should never be called to paint with an offset not on device pixels.
    ASSERT(LayoutPoint(IntPoint(paintOffset.x(), paintOffset.y())) == paintOffset);

    // This avoids painting garbage between columns if there is a column gap.
    if (m_frameView && m_frameView->pagination().mode != Pagination::Unpaginated && paintInfo.shouldPaintWithinRoot(this))
        paintInfo.context->fillRect(paintInfo.rect, m_frameView->baseBackgroundColor(), ColorSpaceDeviceRGB);

    paintObject(paintInfo, paintOffset);
}

static inline bool isComposited(RenderObject* object)
{
    return object->hasLayer() && toRenderLayerModelObject(object)->layer()->isComposited();
}

static inline bool rendererObscuresBackground(RenderObject* rootObject)
{
    if (!rootObject)
        return false;
    
    RenderStyle* style = rootObject->style();
    if (style->visibility() != VISIBLE
        || style->opacity() != 1
        || style->hasTransform())
        return false;
    
    if (isComposited(rootObject))
        return false;

    const RenderObject* rootRenderer = rootObject->rendererForRootBackground();
    if (rootRenderer->style()->backgroundClip() == TextFillBox)
        return false;

    return true;
}

void RenderView::paintBoxDecorations(PaintInfo& paintInfo, const LayoutPoint&)
{
    if (!paintInfo.shouldPaintWithinRoot(this))
        return;

    // Check to see if we are enclosed by a layer that requires complex painting rules.  If so, we cannot blit
    // when scrolling, and we need to use slow repaints.  Examples of layers that require this are transparent layers,
    // layers with reflections, or transformed layers.
    // FIXME: This needs to be dynamic.  We should be able to go back to blitting if we ever stop being inside
    // a transform, transparency layer, etc.
    Element* elt;
    for (elt = document()->ownerElement(); view() && elt && elt->renderer(); elt = elt->document()->ownerElement()) {
        RenderLayer* layer = elt->renderer()->enclosingLayer();
        if (layer->cannotBlitToWindow()) {
            frameView()->setCannotBlitToWindow();
            break;
        }

#if USE(ACCELERATED_COMPOSITING)
        if (RenderLayer* compositingLayer = layer->enclosingCompositingLayerForRepaint()) {
            if (!compositingLayer->backing()->paintsIntoWindow()) {
                frameView()->setCannotBlitToWindow();
                break;
            }
        }
#endif
    }

    if (document()->ownerElement() || !view())
        return;

    if (paintInfo.skipRootBackground())
        return;

    bool rootFillsViewport = false;
    bool rootObscuresBackground = false;
    Node* documentElement = document()->documentElement();
    if (RenderObject* rootRenderer = documentElement ? documentElement->renderer() : 0) {
        // The document element's renderer is currently forced to be a block, but may not always be.
        RenderBox* rootBox = rootRenderer->isBox() ? toRenderBox(rootRenderer) : 0;
        rootFillsViewport = rootBox && !rootBox->x() && !rootBox->y() && rootBox->width() >= width() && rootBox->height() >= height();
        rootObscuresBackground = rendererObscuresBackground(rootRenderer);
    }
    
    Page* page = document()->page();
    float pageScaleFactor = page ? page->pageScaleFactor() : 1;

    // If painting will entirely fill the view, no need to fill the background.
    if (rootFillsViewport && rootObscuresBackground && pageScaleFactor >= 1)
        return;

    // This code typically only executes if the root element's visibility has been set to hidden,
    // if there is a transform on the <html>, or if there is a page scale factor less than 1.
    // Only fill with the base background color (typically white) if we're the root document, 
    // since iframes/frames with no background in the child document should show the parent's background.
    if (frameView()->isTransparent()) // FIXME: This needs to be dynamic.  We should be able to go back to blitting if we ever stop being transparent.
        frameView()->setCannotBlitToWindow(); // The parent must show behind the child.
    else {
        Color baseColor = frameView()->baseBackgroundColor();
        if (baseColor.alpha()) {
            CompositeOperator previousOperator = paintInfo.context->compositeOperation();
            paintInfo.context->setCompositeOperation(CompositeCopy);
            paintInfo.context->fillRect(paintInfo.rect, baseColor, style()->colorSpace());
            paintInfo.context->setCompositeOperation(previousOperator);
        } else
            paintInfo.context->clearRect(paintInfo.rect);
    }
}

bool RenderView::shouldRepaint(const LayoutRect& r) const
{
    if (printing() || r.width() == 0 || r.height() == 0)
        return false;

    if (!m_frameView)
        return false;

    if (m_frameView->repaintsDisabled())
        return false;

    return true;
}

void RenderView::repaintViewRectangle(const LayoutRect& ur, bool immediate) const
{
    if (!shouldRepaint(ur))
        return;

    // We always just invalidate the root view, since we could be an iframe that is clipped out
    // or even invisible.
    Element* elt = document()->ownerElement();
    if (!elt)
        m_frameView->repaintContentRectangle(pixelSnappedIntRect(ur), immediate);
    else if (RenderBox* obj = elt->renderBox()) {
        LayoutRect vr = viewRect();
        LayoutRect r = intersection(ur, vr);
        
        // Subtract out the contentsX and contentsY offsets to get our coords within the viewing
        // rectangle.
        r.moveBy(-vr.location());

        // FIXME: Hardcoded offsets here are not good.
        r.moveBy(obj->contentBoxRect().location());
        obj->repaintRectangle(r, immediate);
    }
}

void RenderView::repaintRectangleInViewAndCompositedLayers(const LayoutRect& ur, bool immediate)
{
    if (!shouldRepaint(ur))
        return;

    repaintViewRectangle(ur, immediate);
    
#if USE(ACCELERATED_COMPOSITING)
    if (compositor()->inCompositingMode()) {
        IntRect repaintRect = pixelSnappedIntRect(ur);
        compositor()->repaintCompositedLayers(&repaintRect);
    }
#endif
}

void RenderView::repaintViewAndCompositedLayers()
{
    repaint();
    
#if USE(ACCELERATED_COMPOSITING)
    if (compositor()->inCompositingMode())
        compositor()->repaintCompositedLayers();
#endif
}

LayoutRect RenderView::visualOverflowRect() const
{
    if (m_frameView->paintsEntireContents())
        return layoutOverflowRect();

    return RenderBlock::visualOverflowRect();
}

void RenderView::computeRectForRepaint(const RenderLayerModelObject* repaintContainer, LayoutRect& rect, bool fixed) const
{
    // If a container was specified, and was not 0 or the RenderView,
    // then we should have found it by now.
    ASSERT_ARG(repaintContainer, !repaintContainer || repaintContainer == this);

    if (printing())
        return;

    if (style()->isFlippedBlocksWritingMode()) {
        // We have to flip by hand since the view's logical height has not been determined.  We
        // can use the viewport width and height.
        if (style()->isHorizontalWritingMode())
            rect.setY(viewHeight() - rect.maxY());
        else
            rect.setX(viewWidth() - rect.maxX());
    }

    if (fixed && m_frameView)
        rect.move(m_frameView->scrollOffsetForFixedPosition());
        
    // Apply our transform if we have one (because of full page zooming).
    if (!repaintContainer && layer() && layer()->transform())
        rect = layer()->transform()->mapRect(rect);
}

void RenderView::absoluteRects(Vector<IntRect>& rects, const LayoutPoint& accumulatedOffset) const
{
    rects.append(pixelSnappedIntRect(accumulatedOffset, layer()->size()));
}

void RenderView::absoluteQuads(Vector<FloatQuad>& quads, bool* wasFixed) const
{
    if (wasFixed)
        *wasFixed = false;
    quads.append(FloatRect(FloatPoint(), layer()->size()));
}

static RenderObject* rendererAfterPosition(RenderObject* object, unsigned offset)
{
    if (!object)
        return 0;

    RenderObject* child = object->childAt(offset);
    return child ? child : object->nextInPreOrderAfterChildren();
}

IntRect RenderView::selectionBounds(bool clipToVisibleContent) const
{
    document()->updateStyleIfNeeded();

    typedef HashMap<RenderObject*, OwnPtr<RenderSelectionInfo> > SelectionMap;
    SelectionMap selectedObjects;

    RenderObject* os = m_selectionStart;
    RenderObject* stop = rendererAfterPosition(m_selectionEnd, m_selectionEndPos);
    while (os && os != stop) {
        if ((os->canBeSelectionLeaf() || os == m_selectionStart || os == m_selectionEnd) && os->selectionState() != SelectionNone) {
            // Blocks are responsible for painting line gaps and margin gaps. They must be examined as well.
            selectedObjects.set(os, adoptPtr(new RenderSelectionInfo(os, clipToVisibleContent)));
            RenderBlock* cb = os->containingBlock();
            while (cb && !cb->isRenderView()) {
                OwnPtr<RenderSelectionInfo>& blockInfo = selectedObjects.add(cb, nullptr).iterator->value;
                if (blockInfo)
                    break;
                blockInfo = adoptPtr(new RenderSelectionInfo(cb, clipToVisibleContent));
                cb = cb->containingBlock();
            }
        }

        os = os->nextInPreOrder();
    }

    // Now create a single bounding box rect that encloses the whole selection.
    LayoutRect selRect;
    SelectionMap::iterator end = selectedObjects.end();
    for (SelectionMap::iterator i = selectedObjects.begin(); i != end; ++i) {
        RenderSelectionInfo* info = i->value.get();
        // RenderSelectionInfo::rect() is in the coordinates of the repaintContainer, so map to page coordinates.
        LayoutRect currRect = info->rect();
        if (RenderLayerModelObject* repaintContainer = info->repaintContainer()) {
            FloatQuad absQuad = repaintContainer->localToAbsoluteQuad(FloatRect(currRect));
            currRect = absQuad.enclosingBoundingBox(); 
        }
        selRect.unite(currRect);
    }
    return pixelSnappedIntRect(selRect);
}

void RenderView::repaintSelection() const
{
    document()->updateStyleIfNeeded();

    HashSet<RenderBlock*> processedBlocks;

    RenderObject* end = rendererAfterPosition(m_selectionEnd, m_selectionEndPos);
    for (RenderObject* o = m_selectionStart; o && o != end; o = o->nextInPreOrder()) {
        if (!o->canBeSelectionLeaf() && o != m_selectionStart && o != m_selectionEnd)
            continue;
        if (o->selectionState() == SelectionNone)
            continue;

        RenderSelectionInfo(o, true).repaint();

        // Blocks are responsible for painting line gaps and margin gaps. They must be examined as well.
        for (RenderBlock* block = o->containingBlock(); block && !block->isRenderView(); block = block->containingBlock()) {
            if (!processedBlocks.add(block).isNewEntry)
                break;
            RenderSelectionInfo(block, true).repaint();
        }
    }
}

#if USE(ACCELERATED_COMPOSITING)
// Compositing layer dimensions take outline size into account, so we have to recompute layer
// bounds when it changes.
// FIXME: This is ugly; it would be nice to have a better way to do this.
void RenderView::setMaximalOutlineSize(int o)
{
    if (o != m_maximalOutlineSize) {
        m_maximalOutlineSize = o;

        // maximalOutlineSize affects compositing layer dimensions.
        compositor()->setCompositingLayersNeedRebuild();    // FIXME: this really just needs to be a geometry update.
    }
}
#endif

void RenderView::setSelection(RenderObject* start, int startPos, RenderObject* end, int endPos, SelectionRepaintMode blockRepaintMode)
{
    // Make sure both our start and end objects are defined.
    // Check www.msnbc.com and try clicking around to find the case where this happened.
    if ((start && !end) || (end && !start))
        return;

    bool caretChanged = m_selectionWasCaret != view()->frame()->selection()->isCaret();
    m_selectionWasCaret = view()->frame()->selection()->isCaret();
    // Just return if the selection hasn't changed.
    if (m_selectionStart == start && m_selectionStartPos == startPos &&
        m_selectionEnd == end && m_selectionEndPos == endPos && !caretChanged)
        return;

    if ((start && end) && (start->flowThreadContainingBlock() != end->flowThreadContainingBlock()))
        return;

    // Record the old selected objects.  These will be used later
    // when we compare against the new selected objects.
    int oldStartPos = m_selectionStartPos;
    int oldEndPos = m_selectionEndPos;

    // Objects each have a single selection rect to examine.
    typedef HashMap<RenderObject*, OwnPtr<RenderSelectionInfo> > SelectedObjectMap;
    SelectedObjectMap oldSelectedObjects;
    SelectedObjectMap newSelectedObjects;

    // Blocks contain selected objects and fill gaps between them, either on the left, right, or in between lines and blocks.
    // In order to get the repaint rect right, we have to examine left, middle, and right rects individually, since otherwise
    // the union of those rects might remain the same even when changes have occurred.
    typedef HashMap<RenderBlock*, OwnPtr<RenderBlockSelectionInfo> > SelectedBlockMap;
    SelectedBlockMap oldSelectedBlocks;
    SelectedBlockMap newSelectedBlocks;

    RenderObject* os = m_selectionStart;
    RenderObject* stop = rendererAfterPosition(m_selectionEnd, m_selectionEndPos);
    while (os && os != stop) {
        if ((os->canBeSelectionLeaf() || os == m_selectionStart || os == m_selectionEnd) && os->selectionState() != SelectionNone) {
            // Blocks are responsible for painting line gaps and margin gaps.  They must be examined as well.
            oldSelectedObjects.set(os, adoptPtr(new RenderSelectionInfo(os, true)));
            if (blockRepaintMode == RepaintNewXOROld) {
                RenderBlock* cb = os->containingBlock();
                while (cb && !cb->isRenderView()) {
                    OwnPtr<RenderBlockSelectionInfo>& blockInfo = oldSelectedBlocks.add(cb, nullptr).iterator->value;
                    if (blockInfo)
                        break;
                    blockInfo = adoptPtr(new RenderBlockSelectionInfo(cb));
                    cb = cb->containingBlock();
                }
            }
        }

        os = os->nextInPreOrder();
    }

    // Now clear the selection.
    SelectedObjectMap::iterator oldObjectsEnd = oldSelectedObjects.end();
    for (SelectedObjectMap::iterator i = oldSelectedObjects.begin(); i != oldObjectsEnd; ++i)
        i->key->setSelectionStateIfNeeded(SelectionNone);

    // set selection start and end
    m_selectionStart = start;
    m_selectionStartPos = startPos;
    m_selectionEnd = end;
    m_selectionEndPos = endPos;

    // Update the selection status of all objects between m_selectionStart and m_selectionEnd
    if (start && start == end)
        start->setSelectionStateIfNeeded(SelectionBoth);
    else {
        if (start)
            start->setSelectionStateIfNeeded(SelectionStart);
        if (end)
            end->setSelectionStateIfNeeded(SelectionEnd);
    }

    RenderObject* o = start;
    stop = rendererAfterPosition(end, endPos);

    while (o && o != stop) {
        if (o != start && o != end && o->canBeSelectionLeaf())
            o->setSelectionStateIfNeeded(SelectionInside);
        o = o->nextInPreOrder();
    }

    if (blockRepaintMode != RepaintNothing)
        layer()->clearBlockSelectionGapsBounds();

    // Now that the selection state has been updated for the new objects, walk them again and
    // put them in the new objects list.
    o = start;
    while (o && o != stop) {
        if ((o->canBeSelectionLeaf() || o == start || o == end) && o->selectionState() != SelectionNone) {
            newSelectedObjects.set(o, adoptPtr(new RenderSelectionInfo(o, true)));
            RenderBlock* cb = o->containingBlock();
            while (cb && !cb->isRenderView()) {
                OwnPtr<RenderBlockSelectionInfo>& blockInfo = newSelectedBlocks.add(cb, nullptr).iterator->value;
                if (blockInfo)
                    break;
                blockInfo = adoptPtr(new RenderBlockSelectionInfo(cb));
                cb = cb->containingBlock();
            }
        }

        o = o->nextInPreOrder();
    }

    if (!m_frameView || blockRepaintMode == RepaintNothing)
        return;

    m_frameView->beginDeferredRepaints();

    // Have any of the old selected objects changed compared to the new selection?
    for (SelectedObjectMap::iterator i = oldSelectedObjects.begin(); i != oldObjectsEnd; ++i) {
        RenderObject* obj = i->key;
        RenderSelectionInfo* newInfo = newSelectedObjects.get(obj);
        RenderSelectionInfo* oldInfo = i->value.get();
        if (!newInfo || oldInfo->rect() != newInfo->rect() || oldInfo->state() != newInfo->state() ||
            (m_selectionStart == obj && oldStartPos != m_selectionStartPos) ||
            (m_selectionEnd == obj && oldEndPos != m_selectionEndPos)) {
            oldInfo->repaint();
            if (newInfo) {
                newInfo->repaint();
                newSelectedObjects.remove(obj);
            }
        }
    }

    // Any new objects that remain were not found in the old objects dict, and so they need to be updated.
    SelectedObjectMap::iterator newObjectsEnd = newSelectedObjects.end();
    for (SelectedObjectMap::iterator i = newSelectedObjects.begin(); i != newObjectsEnd; ++i)
        i->value->repaint();

    // Have any of the old blocks changed?
    SelectedBlockMap::iterator oldBlocksEnd = oldSelectedBlocks.end();
    for (SelectedBlockMap::iterator i = oldSelectedBlocks.begin(); i != oldBlocksEnd; ++i) {
        RenderBlock* block = i->key;
        RenderBlockSelectionInfo* newInfo = newSelectedBlocks.get(block);
        RenderBlockSelectionInfo* oldInfo = i->value.get();
        if (!newInfo || oldInfo->rects() != newInfo->rects() || oldInfo->state() != newInfo->state()) {
            oldInfo->repaint();
            if (newInfo) {
                newInfo->repaint();
                newSelectedBlocks.remove(block);
            }
        }
    }

    // Any new blocks that remain were not found in the old blocks dict, and so they need to be updated.
    SelectedBlockMap::iterator newBlocksEnd = newSelectedBlocks.end();
    for (SelectedBlockMap::iterator i = newSelectedBlocks.begin(); i != newBlocksEnd; ++i)
        i->value->repaint();

    m_frameView->endDeferredRepaints();
}

void RenderView::getSelection(RenderObject*& startRenderer, int& startOffset, RenderObject*& endRenderer, int& endOffset) const
{
    startRenderer = m_selectionStart;
    startOffset = m_selectionStartPos;
    endRenderer = m_selectionEnd;
    endOffset = m_selectionEndPos;
}

void RenderView::clearSelection()
{
    layer()->repaintBlockSelectionGaps();
    setSelection(0, -1, 0, -1, RepaintNewMinusOld);
}

void RenderView::selectionStartEnd(int& startPos, int& endPos) const
{
    startPos = m_selectionStartPos;
    endPos = m_selectionEndPos;
}

bool RenderView::printing() const
{
    return document()->printing();
}

bool RenderView::shouldUsePrintingLayout() const
{
    if (!printing() || !m_frameView)
        return false;
    Frame* frame = m_frameView->frame();
    return frame && frame->shouldUsePrintingLayout();
}

size_t RenderView::getRetainedWidgets(Vector<RenderWidget*>& renderWidgets)
{
    size_t size = m_widgets.size();

    renderWidgets.reserveCapacity(size);

    RenderWidgetSet::const_iterator end = m_widgets.end();
    for (RenderWidgetSet::const_iterator it = m_widgets.begin(); it != end; ++it) {
        renderWidgets.uncheckedAppend(*it);
        (*it)->ref();
    }
    
    return size;
}

void RenderView::releaseWidgets(Vector<RenderWidget*>& renderWidgets)
{
    size_t size = renderWidgets.size();

    for (size_t i = 0; i < size; ++i)
        renderWidgets[i]->deref(renderArena());
}

void RenderView::updateWidgetPositions()
{
    // updateWidgetPosition() can possibly cause layout to be re-entered (via plug-ins running
    // scripts in response to NPP_SetWindow, for example), so we need to keep the Widgets
    // alive during enumeration.    

    Vector<RenderWidget*> renderWidgets;
    size_t size = getRetainedWidgets(renderWidgets);
    
    for (size_t i = 0; i < size; ++i)
        renderWidgets[i]->updateWidgetPosition();

    for (size_t i = 0; i < size; ++i)
        renderWidgets[i]->widgetPositionsUpdated();

    releaseWidgets(renderWidgets);
}

void RenderView::addWidget(RenderWidget* o)
{
    m_widgets.add(o);
}

void RenderView::removeWidget(RenderWidget* o)
{
    m_widgets.remove(o);
}

void RenderView::notifyWidgets(WidgetNotification notification)
{
    Vector<RenderWidget*> renderWidgets;
    size_t size = getRetainedWidgets(renderWidgets);

    for (size_t i = 0; i < size; ++i)
        renderWidgets[i]->notifyWidget(notification);

    releaseWidgets(renderWidgets);
}

LayoutRect RenderView::viewRect() const
{
    if (shouldUsePrintingLayout())
        return LayoutRect(LayoutPoint(), size());
    if (m_frameView)
        return m_frameView->visibleContentRect();
    return LayoutRect();
}


IntRect RenderView::unscaledDocumentRect() const
{
    LayoutRect overflowRect(layoutOverflowRect());
    flipForWritingMode(overflowRect);
    return pixelSnappedIntRect(overflowRect);
}

bool RenderView::rootBackgroundIsEntirelyFixed() const
{
    RenderObject* rootObject = document()->documentElement() ? document()->documentElement()->renderer() : 0;
    if (!rootObject)
        return false;

    RenderObject* rootRenderer = rootObject->rendererForRootBackground();
    return rootRenderer->hasEntirelyFixedBackground();
}

LayoutRect RenderView::backgroundRect(RenderBox* backgroundRenderer) const
{
    if (!hasColumns())
        return unscaledDocumentRect();

    ColumnInfo* columnInfo = this->columnInfo();
    LayoutRect backgroundRect(0, 0, columnInfo->desiredColumnWidth(), columnInfo->columnHeight() * columnInfo->columnCount());
    if (!isHorizontalWritingMode())
        backgroundRect = backgroundRect.transposedRect();
    backgroundRenderer->flipForWritingMode(backgroundRect);

    return backgroundRect;
}

IntRect RenderView::documentRect() const
{
    FloatRect overflowRect(unscaledDocumentRect());
    if (hasTransform())
        overflowRect = layer()->currentTransform().mapRect(overflowRect);
    return IntRect(overflowRect);
}

int RenderView::viewHeight() const
{
    int height = 0;
    if (!shouldUsePrintingLayout() && m_frameView) {
        height = m_frameView->layoutHeight();
        height = m_frameView->useFixedLayout() ? ceilf(style()->effectiveZoom() * float(height)) : height;
    }
    return height;
}

int RenderView::viewWidth() const
{
    int width = 0;
    if (!shouldUsePrintingLayout() && m_frameView) {
        width = m_frameView->layoutWidth();
        width = m_frameView->useFixedLayout() ? ceilf(style()->effectiveZoom() * float(width)) : width;
    }
    return width;
}

int RenderView::viewLogicalHeight() const
{
    int height = style()->isHorizontalWritingMode() ? viewHeight() : viewWidth();
    return height;
}

float RenderView::zoomFactor() const
{
    Frame* frame = m_frameView->frame();
    return frame ? frame->pageZoomFactor() : 1;
}

void RenderView::pushLayoutState(RenderObject* root)
{
    ASSERT(m_layoutStateDisableCount == 0);
    ASSERT(m_layoutState == 0);

    pushLayoutStateForCurrentFlowThread(root);
    m_layoutState = new (renderArena()) LayoutState(root);
}

bool RenderView::shouldDisableLayoutStateForSubtree(RenderObject* renderer) const
{
    RenderObject* o = renderer;
    while (o) {
        if (o->hasColumns() || o->hasTransform() || o->hasReflection())
            return true;
        o = o->container();
    }
    return false;
}

void RenderView::updateHitTestResult(HitTestResult& result, const LayoutPoint& point)
{
    if (result.innerNode())
        return;

    Node* node = document()->documentElement();
    if (node) {
        result.setInnerNode(node);
        if (!result.innerNonSharedNode())
            result.setInnerNonSharedNode(node);

        LayoutPoint adjustedPoint = point;
        offsetForContents(adjustedPoint);

        result.setLocalPoint(adjustedPoint);
    }
}

// FIXME: This function is obsolete and only used by embedded WebViews inside AppKit NSViews.
// Do not add callers of this function!
// The idea here is to take into account what object is moving the pagination point, and
// thus choose the best place to chop it.
void RenderView::setBestTruncatedAt(int y, RenderBoxModelObject* forRenderer, bool forcedBreak)
{
    // Nobody else can set a page break once we have a forced break.
    if (m_legacyPrinting.m_forcedPageBreak)
        return;

    // Forced breaks always win over unforced breaks.
    if (forcedBreak) {
        m_legacyPrinting.m_forcedPageBreak = true;
        m_legacyPrinting.m_bestTruncatedAt = y;
        return;
    }

    // Prefer the widest object that tries to move the pagination point
    IntRect boundingBox = forRenderer->borderBoundingBox();
    if (boundingBox.width() > m_legacyPrinting.m_truncatorWidth) {
        m_legacyPrinting.m_truncatorWidth = boundingBox.width();
        m_legacyPrinting.m_bestTruncatedAt = y;
    }
}

#if USE(ACCELERATED_COMPOSITING)
bool RenderView::usesCompositing() const
{
    return m_compositor && m_compositor->inCompositingMode();
}

RenderLayerCompositor* RenderView::compositor()
{
    if (!m_compositor)
        m_compositor = adoptPtr(new RenderLayerCompositor(this));

    return m_compositor.get();
}
#endif

void RenderView::setIsInWindow(bool isInWindow)
{
#if USE(ACCELERATED_COMPOSITING)
    if (m_compositor)
        m_compositor->setIsInWindow(isInWindow);
#endif
}

#if ENABLE(CSS_SHADERS) && USE(3D_GRAPHICS)
CustomFilterGlobalContext* RenderView::customFilterGlobalContext()
{
    if (!m_customFilterGlobalContext)
        m_customFilterGlobalContext = adoptPtr(new CustomFilterGlobalContext());
    return m_customFilterGlobalContext.get();
}
#endif

void RenderView::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBlock::styleDidChange(diff, oldStyle);
    if (hasRenderNamedFlowThreads())
        flowThreadController()->styleDidChange();
}

bool RenderView::hasRenderNamedFlowThreads() const
{
    return m_flowThreadController && m_flowThreadController->hasRenderNamedFlowThreads();
}

bool RenderView::checkTwoPassLayoutForAutoHeightRegions() const
{
    return hasRenderNamedFlowThreads() && m_flowThreadController->hasFlowThreadsWithAutoLogicalHeightRegions();
}

FlowThreadController* RenderView::flowThreadController()
{
    if (!m_flowThreadController)
        m_flowThreadController = FlowThreadController::create(this);

    return m_flowThreadController.get();
}

void RenderView::pushLayoutStateForCurrentFlowThread(const RenderObject* object)
{
    if (!m_flowThreadController)
        return;

    RenderFlowThread* currentFlowThread = m_flowThreadController->currentRenderFlowThread();
    if (!currentFlowThread)
        return;

    currentFlowThread->pushFlowThreadLayoutState(object);
}

void RenderView::popLayoutStateForCurrentFlowThread()
{
    if (!m_flowThreadController)
        return;

    RenderFlowThread* currentFlowThread = m_flowThreadController->currentRenderFlowThread();
    if (!currentFlowThread)
        return;

    currentFlowThread->popFlowThreadLayoutState();
}

RenderBlock::IntervalArena* RenderView::intervalArena()
{
    if (!m_intervalArena)
        m_intervalArena = IntervalArena::create();
    return m_intervalArena.get();
}

FragmentationDisabler::FragmentationDisabler(RenderObject* root)
{
    RenderView* renderView = root->view();
    ASSERT(renderView);

    LayoutState* layoutState = renderView->layoutState();

    m_root = root;
    m_fragmenting = layoutState && layoutState->isPaginated();
    m_flowThreadState = m_root->flowThreadState();
#ifndef NDEBUG
    m_layoutState = layoutState;
#endif

    if (layoutState)
        layoutState->m_isPaginated = false;
        
    if (m_flowThreadState != RenderObject::NotInsideFlowThread)
        m_root->setFlowThreadStateIncludingDescendants(RenderObject::NotInsideFlowThread);
}

FragmentationDisabler::~FragmentationDisabler()
{
    RenderView* renderView = m_root->view();
    ASSERT(renderView);

    LayoutState* layoutState = renderView->layoutState();
#ifndef NDEBUG
    ASSERT(m_layoutState == layoutState);
#endif

    if (layoutState)
        layoutState->m_isPaginated = m_fragmenting;
        
    if (m_flowThreadState != RenderObject::NotInsideFlowThread)
        m_root->setFlowThreadStateIncludingDescendants(m_flowThreadState);
}

} // namespace WebCore
