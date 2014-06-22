/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights reserved.
 *
 * Portions are Copyright (C) 1998 Netscape Communications Corporation.
 *
 * Other contributors:
 *   Robert O'Callahan <roc+@cs.cmu.edu>
 *   David Baron <dbaron@fas.harvard.edu>
 *   Christian Biesinger <cbiesinger@web.de>
 *   Randall Jesup <rjesup@wgate.com>
 *   Roland Mainz <roland.mainz@informatik.med.uni-giessen.de>
 *   Josh Soref <timeless@mac.com>
 *   Boris Zbarsky <bzbarsky@mit.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Alternatively, the contents of this file may be used under the terms
 * of either the Mozilla Public License Version 1.1, found at
 * http://www.mozilla.org/MPL/ (the "MPL") or the GNU General Public
 * License Version 2.0, found at http://www.fsf.org/copyleft/gpl.html
 * (the "GPL"), in which case the provisions of the MPL or the GPL are
 * applicable instead of those above.  If you wish to allow use of your
 * version of this file only under the terms of one of those two
 * licenses (the MPL or the GPL) and not to allow others to use your
 * version of this file under the LGPL, indicate your decision by
 * deletingthe provisions above and replace them with the notice and
 * other provisions required by the MPL or the GPL, as the case may be.
 * If you do not delete the provisions above, a recipient may use your
 * version of this file under any of the LGPL, the MPL or the GPL.
 */

#include "config.h"
#include "RenderLayer.h"

#include "AnimationController.h"
#include "ColumnInfo.h"
#include "CSSPropertyNames.h"
#include "Chrome.h"
#include "Document.h"
#include "DocumentEventQueue.h"
#include "EventHandler.h"
#include "FeatureObserver.h"
#include "FloatConversion.h"
#include "FloatPoint3D.h"
#include "FloatRect.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameSelection.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "Gradient.h"
#include "GraphicsContext.h"
#include "HTMLFrameElement.h"
#include "HTMLFrameOwnerElement.h"
#include "HTMLNames.h"
#include "HistogramSupport.h"
#include "HitTestingTransformState.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "OverflowEvent.h"
#include "OverlapTestRequestClient.h"
#include "Page.h"
#include "PlatformMouseEvent.h"
#include "RenderArena.h"
#include "RenderFlowThread.h"
#include "RenderGeometryMap.h"
#include "RenderInline.h"
#include "RenderMarquee.h"
#include "RenderReplica.h"
#include "RenderSVGResourceClipper.h"
#include "RenderScrollbar.h"
#include "RenderScrollbarPart.h"
#include "RenderTheme.h"
#include "RenderTreeAsText.h"
#include "RenderView.h"
#include "ScaleTransformOperation.h"
#include "ScrollAnimator.h"
#include "Scrollbar.h"
#include "ScrollbarTheme.h"
#include "ScrollingCoordinator.h"
#include "Settings.h"
#include "ShadowRoot.h"
#include "SourceGraphic.h"
#include "StylePropertySet.h"
#include "StyleResolver.h"
#include "TextStream.h"
#include "TransformationMatrix.h"
#include "TranslateTransformOperation.h"
#include <wtf/StdLibExtras.h>
#include <wtf/text/CString.h>

#if ENABLE(CSS_FILTERS)
#include "FEColorMatrix.h"
#include "FEMerge.h"
#include "FilterEffectRenderer.h"
#include "RenderLayerFilterInfo.h"
#endif

#if USE(ACCELERATED_COMPOSITING)
#include "RenderLayerBacking.h"
#include "RenderLayerCompositor.h"
#endif

#if ENABLE(SVG)
#include "SVGNames.h"
#endif

#if ENABLE(CSS_SHADERS) && USE(3D_GRAPHICS)
#include "CustomFilterGlobalContext.h"
#include "CustomFilterOperation.h"
#include "CustomFilterValidatedProgram.h"
#include "ValidatedCustomFilterOperation.h"
#endif

#define MIN_INTERSECT_FOR_REVEAL 32

using namespace std;

namespace WebCore {

using namespace HTMLNames;

const int MinimumWidthWhileResizing = 100;
const int MinimumHeightWhileResizing = 40;

bool ClipRect::intersects(const HitTestLocation& hitTestLocation) const
{
    return hitTestLocation.intersects(m_rect);
}

void makeMatrixRenderable(TransformationMatrix& matrix, bool has3DRendering)
{
#if !ENABLE(3D_RENDERING)
    UNUSED_PARAM(has3DRendering);
    matrix.makeAffine();
#else
    if (!has3DRendering)
        matrix.makeAffine();
#endif
}

RenderLayer::RenderLayer(RenderLayerModelObject* renderer)
    : m_inResizeMode(false)
    , m_scrollDimensionsDirty(true)
    , m_normalFlowListDirty(true)
    , m_hasSelfPaintingLayerDescendant(false)
    , m_hasSelfPaintingLayerDescendantDirty(false)
    , m_hasOutOfFlowPositionedDescendant(false)
    , m_hasOutOfFlowPositionedDescendantDirty(true)
    , m_needsCompositedScrolling(false)
    , m_descendantsAreContiguousInStackingOrder(false)
    , m_isRootLayer(renderer->isRenderView())
    , m_usedTransparency(false)
    , m_paintingInsideReflection(false)
    , m_inOverflowRelayout(false)
    , m_repaintStatus(NeedsNormalRepaint)
    , m_visibleContentStatusDirty(true)
    , m_hasVisibleContent(false)
    , m_visibleDescendantStatusDirty(false)
    , m_hasVisibleDescendant(false)
    , m_isPaginated(false)
    , m_3DTransformedDescendantStatusDirty(true)
    , m_has3DTransformedDescendant(false)
#if USE(ACCELERATED_COMPOSITING)
    , m_hasCompositingDescendant(false)
    , m_indirectCompositingReason(NoIndirectCompositingReason)
    , m_viewportConstrainedNotCompositedReason(NoNotCompositedReason)
#endif
    , m_containsDirtyOverlayScrollbars(false)
    , m_updatingMarqueePosition(false)
#if !ASSERT_DISABLED
    , m_layerListMutationAllowed(true)
#endif
#if ENABLE(CSS_FILTERS)
    , m_hasFilterInfo(false)
#endif
#if ENABLE(CSS_COMPOSITING)
    , m_blendMode(BlendModeNormal)
#endif
    , m_renderer(renderer)
    , m_parent(0)
    , m_previous(0)
    , m_next(0)
    , m_first(0)
    , m_last(0)
    , m_staticInlinePosition(0)
    , m_staticBlockPosition(0)
    , m_reflection(0)
    , m_scrollCorner(0)
    , m_resizer(0)
    , m_enclosingPaginationLayer(0)
{
    m_isNormalFlowOnly = shouldBeNormalFlowOnly();
    m_isSelfPaintingLayer = shouldBeSelfPaintingLayer();

    // Non-stacking containers should have empty z-order lists. As this is already the case,
    // there is no need to dirty / recompute these lists.
    m_zOrderListsDirty = isStackingContainer();

    ScrollableArea::setConstrainsScrollingToContentEdge(false);

    if (!renderer->firstChild() && renderer->style()) {
        m_visibleContentStatusDirty = false;
        m_hasVisibleContent = renderer->style()->visibility() == VISIBLE;
    }

    Node* node = renderer->node();
    if (node && node->isElementNode()) {
        // We save and restore only the scrollOffset as the other scroll values are recalculated.
        Element* element = toElement(node);
        m_scrollOffset = element->savedLayerScrollOffset();
        if (!m_scrollOffset.isZero())
            scrollAnimator()->setCurrentPosition(FloatPoint(m_scrollOffset.width(), m_scrollOffset.height()));
        element->setSavedLayerScrollOffset(IntSize());
    }
}

RenderLayer::~RenderLayer()
{
    if (inResizeMode() && !renderer()->documentBeingDestroyed()) {
        if (Frame* frame = renderer()->frame())
            frame->eventHandler()->resizeLayerDestroyed();
    }

    if (Frame* frame = renderer()->frame()) {
        if (FrameView* frameView = frame->view())
            frameView->removeScrollableArea(this);
    }

    if (!m_renderer->documentBeingDestroyed()) {
        Node* node = m_renderer->node();
        if (node && node->isElementNode())
            toElement(node)->setSavedLayerScrollOffset(m_scrollOffset);
    }

    destroyScrollbar(HorizontalScrollbar);
    destroyScrollbar(VerticalScrollbar);

    if (renderer()->frame() && renderer()->frame()->page()) {
        if (ScrollingCoordinator* scrollingCoordinator = renderer()->frame()->page()->scrollingCoordinator())
            scrollingCoordinator->willDestroyScrollableArea(this);
    }

    if (m_reflection)
        removeReflection();
    
#if ENABLE(CSS_FILTERS)
    removeFilterInfoIfNeeded();
#endif

    // Child layers will be deleted by their corresponding render objects, so
    // we don't need to delete them ourselves.

#if USE(ACCELERATED_COMPOSITING)
    clearBacking(true);
#endif
    
    if (m_scrollCorner)
        m_scrollCorner->destroy();
    if (m_resizer)
        m_resizer->destroy();
}

String RenderLayer::name() const
{
    StringBuilder name;
    name.append(renderer()->renderName());

    if (Element* element = renderer()->node() && renderer()->node()->isElementNode() ? toElement(renderer()->node()) : 0) {
        name.append(' ');
        name.append(element->tagName());

        if (element->hasID()) {
            name.appendLiteral(" id=\'");
            name.append(element->getIdAttribute());
            name.append('\'');
        }

        if (element->hasClass()) {
            name.appendLiteral(" class=\'");
            for (size_t i = 0; i < element->classNames().size(); ++i) {
                if (i > 0)
                    name.append(' ');
                name.append(element->classNames()[i]);
            }
            name.append('\'');
        }
    }

    if (isReflection())
        name.appendLiteral(" (reflection)");

    return name.toString();
}

#if USE(ACCELERATED_COMPOSITING)
RenderLayerCompositor* RenderLayer::compositor() const
{
    if (!renderer()->view())
        return 0;
    return renderer()->view()->compositor();
}

void RenderLayer::contentChanged(ContentChangeType changeType)
{
    // This can get called when video becomes accelerated, so the layers may change.
    if ((changeType == CanvasChanged || changeType == VideoChanged || changeType == FullScreenChanged) && compositor()->updateLayerCompositingState(this))
        compositor()->setCompositingLayersNeedRebuild();

    if (m_backing)
        m_backing->contentChanged(changeType);
}
#endif // USE(ACCELERATED_COMPOSITING)

bool RenderLayer::canRender3DTransforms() const
{
#if USE(ACCELERATED_COMPOSITING)
    return compositor()->canRender3DTransforms();
#else
    return false;
#endif
}

#if ENABLE(CSS_FILTERS)
bool RenderLayer::paintsWithFilters() const
{
    // FIXME: Eventually there will be more factors than isComposited() to decide whether or not to render the filter
    if (!renderer()->hasFilter())
        return false;
        
#if USE(ACCELERATED_COMPOSITING)
    if (!isComposited())
        return true;

    if (!m_backing || !m_backing->canCompositeFilters())
        return true;
#endif

    return false;
}
    
bool RenderLayer::requiresFullLayerImageForFilters() const 
{
    if (!paintsWithFilters())
        return false;
    FilterEffectRenderer* filter = filterRenderer();
    return filter ? filter->hasFilterThatMovesPixels() : false;
}

FilterEffectRenderer* RenderLayer::filterRenderer() const
{
    RenderLayerFilterInfo* filterInfo = this->filterInfo();
    return filterInfo ? filterInfo->renderer() : 0;
}

RenderLayerFilterInfo* RenderLayer::filterInfo() const
{
    return hasFilterInfo() ? RenderLayerFilterInfo::filterInfoForRenderLayer(this) : 0;
}

RenderLayerFilterInfo* RenderLayer::ensureFilterInfo()
{
    return RenderLayerFilterInfo::createFilterInfoForRenderLayerIfNeeded(this);
}

void RenderLayer::removeFilterInfoIfNeeded()
{
    if (hasFilterInfo())
        RenderLayerFilterInfo::removeFilterInfoForRenderLayer(this); 
}
#endif

LayoutPoint RenderLayer::computeOffsetFromRoot(bool& hasLayerOffset) const
{
    hasLayerOffset = true;

    if (!parent())
        return LayoutPoint();

    // This is similar to root() but we check if an ancestor layer would
    // prevent the optimization from working.
    const RenderLayer* rootLayer = 0;
    for (const RenderLayer* parentLayer = parent(); parentLayer; rootLayer = parentLayer, parentLayer = parentLayer->parent()) {
        hasLayerOffset = parentLayer->canUseConvertToLayerCoords();
        if (!hasLayerOffset)
            return LayoutPoint();
    }
    ASSERT(rootLayer == root());

    LayoutPoint offset;
    parent()->convertToLayerCoords(rootLayer, offset);
    return offset;
}

void RenderLayer::updateLayerPositionsAfterLayout(const RenderLayer* rootLayer, UpdateLayerPositionsFlags flags)
{
    RenderGeometryMap geometryMap(UseTransforms);
    if (this != rootLayer)
        geometryMap.pushMappingsToAncestor(parent(), 0);
    updateLayerPositions(&geometryMap, flags);
}

void RenderLayer::updateLayerPositions(RenderGeometryMap* geometryMap, UpdateLayerPositionsFlags flags)
{
    updateLayerPosition(); // For relpositioned layers or non-positioned layers,
                           // we need to keep in sync, since we may have shifted relative
                           // to our parent layer.
    if (geometryMap)
        geometryMap->pushMappingsToAncestor(this, parent());

    // Clear our cached clip rect information.
    clearClipRects();
    
    if (hasOverflowControls()) {
        LayoutPoint offsetFromRoot;
        if (geometryMap)
            offsetFromRoot = LayoutPoint(geometryMap->absolutePoint(FloatPoint()));
        else {
            // FIXME: It looks suspicious to call convertToLayerCoords here
            // as canUseConvertToLayerCoords may be true for an ancestor layer.
            convertToLayerCoords(root(), offsetFromRoot);
        }
        positionOverflowControls(toIntSize(roundedIntPoint(offsetFromRoot)));
    }

    updateDescendantDependentFlags();

    if (flags & UpdatePagination)
        updatePagination();
    else {
        m_isPaginated = false;
        m_enclosingPaginationLayer = 0;
    }

    if (m_hasVisibleContent) {
        RenderView* view = renderer()->view();
        ASSERT(view);
        // FIXME: LayoutState does not work with RenderLayers as there is not a 1-to-1
        // mapping between them and the RenderObjects. It would be neat to enable
        // LayoutState outside the layout() phase and use it here.
        ASSERT(!view->layoutStateEnabled());

        RenderLayerModelObject* repaintContainer = renderer()->containerForRepaint();
        LayoutRect oldRepaintRect = m_repaintRect;
        LayoutRect oldOutlineBox = m_outlineBox;
        computeRepaintRects(repaintContainer, geometryMap);

        // FIXME: Should ASSERT that value calculated for m_outlineBox using the cached offset is the same
        // as the value not using the cached offset, but we can't due to https://bugs.webkit.org/show_bug.cgi?id=37048
        if (flags & CheckForRepaint) {
            if (view && !view->printing()) {
                if (m_repaintStatus & NeedsFullRepaint) {
                    renderer()->repaintUsingContainer(repaintContainer, pixelSnappedIntRect(oldRepaintRect));
                    if (m_repaintRect != oldRepaintRect)
                        renderer()->repaintUsingContainer(repaintContainer, pixelSnappedIntRect(m_repaintRect));
                } else if (shouldRepaintAfterLayout())
                    renderer()->repaintAfterLayoutIfNeeded(repaintContainer, oldRepaintRect, oldOutlineBox, &m_repaintRect, &m_outlineBox);
            }
        }
    } else
        clearRepaintRects();

    m_repaintStatus = NeedsNormalRepaint;

    // Go ahead and update the reflection's position and size.
    if (m_reflection)
        m_reflection->layout();

#if USE(ACCELERATED_COMPOSITING)
    // Clear the IsCompositingUpdateRoot flag once we've found the first compositing layer in this update.
    bool isUpdateRoot = (flags & IsCompositingUpdateRoot);
    if (isComposited())
        flags &= ~IsCompositingUpdateRoot;
#endif

    if (useRegionBasedColumns() && renderer()->isInFlowRenderFlowThread()) {
        updatePagination();
        flags |= UpdatePagination;
    }
    
    if (renderer()->hasColumns())
        flags |= UpdatePagination;

    for (RenderLayer* child = firstChild(); child; child = child->nextSibling())
        child->updateLayerPositions(geometryMap, flags);

#if USE(ACCELERATED_COMPOSITING)
    if ((flags & UpdateCompositingLayers) && isComposited()) {
        RenderLayerBacking::UpdateAfterLayoutFlags updateFlags = RenderLayerBacking::CompositingChildrenOnly;
        if (flags & NeedsFullRepaintInBacking)
            updateFlags |= RenderLayerBacking::NeedsFullRepaint;
        if (isUpdateRoot)
            updateFlags |= RenderLayerBacking::IsUpdateRoot;
        backing()->updateAfterLayout(updateFlags);
    }
#endif
        
    // With all our children positioned, now update our marquee if we need to.
    if (m_marquee) {
        // FIXME: would like to use TemporaryChange<> but it doesn't work with bitfields.
        bool oldUpdatingMarqueePosition = m_updatingMarqueePosition;
        m_updatingMarqueePosition = true;
        m_marquee->updateMarqueePosition();
        m_updatingMarqueePosition = oldUpdatingMarqueePosition;
    }

    if (geometryMap)
        geometryMap->popMappingsToAncestor(parent());
}

LayoutRect RenderLayer::repaintRectIncludingNonCompositingDescendants() const
{
    LayoutRect repaintRect = m_repaintRect;
    for (RenderLayer* child = firstChild(); child; child = child->nextSibling()) {
        // Don't include repaint rects for composited child layers; they will paint themselves and have a different origin.
        if (child->isComposited())
            continue;

        repaintRect.unite(child->repaintRectIncludingNonCompositingDescendants());
    }
    return repaintRect;
}

void RenderLayer::setAncestorChainHasSelfPaintingLayerDescendant()
{
    for (RenderLayer* layer = this; layer; layer = layer->parent()) {
        if (!layer->m_hasSelfPaintingLayerDescendantDirty && layer->hasSelfPaintingLayerDescendant())
            break;

        layer->m_hasSelfPaintingLayerDescendantDirty = false;
        layer->m_hasSelfPaintingLayerDescendant = true;
    }
}

void RenderLayer::dirtyAncestorChainHasSelfPaintingLayerDescendantStatus()
{
    for (RenderLayer* layer = this; layer; layer = layer->parent()) {
        layer->m_hasSelfPaintingLayerDescendantDirty = true;
        // If we have reached a self-painting layer, we know our parent should have a self-painting descendant
        // in this case, there is no need to dirty our ancestors further.
        if (layer->isSelfPaintingLayer()) {
            ASSERT(!parent() || parent()->m_hasSelfPaintingLayerDescendantDirty || parent()->hasSelfPaintingLayerDescendant());
            break;
        }
    }
}

bool RenderLayer::acceleratedCompositingForOverflowScrollEnabled() const
{
    return renderer()->frame()
        && renderer()->frame()->page()
        && renderer()->frame()->page()->settings()->acceleratedCompositingForOverflowScrollEnabled();
}

// If we are a stacking container, then this function will determine if our
// descendants for a contiguous block in stacking order. This is required in
// order for an element to be safely promoted to a stacking container. It is safe
// to become a stacking container if this change would not alter the stacking
// order of layers on the page. That can only happen if a non-descendant appear
// between us and our descendants in stacking order. Here's an example:
//
//                                 this
//                                /  |  \.
//                               A   B   C
//                              /\   |   /\.
//                             0 -8  D  2  7
//                                   |
//                                   5
//
// I've labeled our normal flow descendants A, B, C, and D, our stacking
// container descendants with their z indices, and us with 'this' (we're a
// stacking container and our zIndex doesn't matter here). These nodes appear in
// three lists: posZOrder, negZOrder, and normal flow (keep in mind that normal
// flow layers don't overlap). So if we arrange these lists in order we get our
// stacking order:
//
//                     [-8], [A-D], [0, 2, 5, 7]--> pos z-order.
//                       |     |
//        Neg z-order. <-+     +--> Normal flow descendants.
//
// We can then assign new, 'stacking' order indices to these elements as follows:
//
//                     [-8], [A-D], [0, 2, 5, 7]
// 'Stacking' indices:  -1     0     1  2  3  4
//
// Note that the normal flow descendants can share an index because they don't
// stack/overlap. Now our problem becomes very simple: a layer can safely become
// a stacking container if the stacking-order indices of it and its descendants
// appear in a contiguous block in the list of stacking indices. This problem
// can be solved very efficiently by calculating the min/max stacking indices in
// the subtree, and the number stacking container descendants. Once we have this
// information, we know that the subtree's indices form a contiguous block if:
//
//           maxStackIndex - minStackIndex == numSCDescendants
//
// So for node A in the example above we would have:
//   maxStackIndex = 1
//   minStackIndex = -1
//   numSCDecendants = 2
//
// and so,
//       maxStackIndex - minStackIndex == numSCDescendants
//  ===>                      1 - (-1) == 2
//  ===>                             2 == 2
//
//  Since this is true, A can safely become a stacking container.
//  Now, for node C we have:
//
//   maxStackIndex = 4
//   minStackIndex = 0 <-- because C has stacking index 0.
//   numSCDecendants = 2
//
// and so,
//       maxStackIndex - minStackIndex == numSCDescendants
//  ===>                         4 - 0 == 2
//  ===>                             4 == 2
//
// Since this is false, C cannot be safely promoted to a stacking container. This
// happened because of the elements with z-index 5 and 0. Now if 5 had been a
// child of C rather than D, and A had no child with Z index 0, we would have had:
//
//   maxStackIndex = 3
//   minStackIndex = 0 <-- because C has stacking index 0.
//   numSCDecendants = 3
//
// and so,
//       maxStackIndex - minStackIndex == numSCDescendants
//  ===>                         3 - 0 == 3
//  ===>                             3 == 3
//
//  And we would conclude that C could be promoted.
void RenderLayer::updateDescendantsAreContiguousInStackingOrder()
{
    if (!isStackingContext() || !acceleratedCompositingForOverflowScrollEnabled())
        return;

    ASSERT(!m_normalFlowListDirty);
    ASSERT(!m_zOrderListsDirty);

    OwnPtr<Vector<RenderLayer*> > posZOrderList;
    OwnPtr<Vector<RenderLayer*> > negZOrderList;
    rebuildZOrderLists(StopAtStackingContexts, posZOrderList, negZOrderList);

    // Create a reverse lookup.
    HashMap<const RenderLayer*, int> lookup;

    if (negZOrderList) {
        int stackingOrderIndex = -1;
        size_t listSize = negZOrderList->size();
        for (size_t i = 0; i < listSize; ++i) {
            RenderLayer* currentLayer = negZOrderList->at(listSize - i - 1);
            if (!currentLayer->isStackingContext())
                continue;
            lookup.set(currentLayer, stackingOrderIndex--);
        }
    }

    if (posZOrderList) {
        size_t listSize = posZOrderList->size();
        int stackingOrderIndex = 1;
        for (size_t i = 0; i < listSize; ++i) {
            RenderLayer* currentLayer = posZOrderList->at(i);
            if (!currentLayer->isStackingContext())
                continue;
            lookup.set(currentLayer, stackingOrderIndex++);
        }
    }

    int minIndex = 0;
    int maxIndex = 0;
    int count = 0;
    bool firstIteration = true;
    updateDescendantsAreContiguousInStackingOrderRecursive(lookup, minIndex, maxIndex, count, firstIteration);
}

void RenderLayer::updateDescendantsAreContiguousInStackingOrderRecursive(const HashMap<const RenderLayer*, int>& lookup, int& minIndex, int& maxIndex, int& count, bool firstIteration)
{
    if (isStackingContext() && !firstIteration) {
        if (lookup.contains(this)) {
            minIndex = std::min(minIndex, lookup.get(this));
            maxIndex = std::max(maxIndex, lookup.get(this));
            count++;
        }
        return;
    }

    for (RenderLayer* child = firstChild(); child; child = child->nextSibling()) {
        int childMinIndex = 0;
        int childMaxIndex = 0;
        int childCount = 0;
        child->updateDescendantsAreContiguousInStackingOrderRecursive(lookup, childMinIndex, childMaxIndex, childCount, false);
        if (childCount) {
            count += childCount;
            minIndex = std::min(minIndex, childMinIndex);
            maxIndex = std::max(maxIndex, childMaxIndex);
        }
    }

    if (!isStackingContext()) {
        bool newValue = maxIndex - minIndex == count;
#if USE(ACCELERATED_COMPOSITING)
        bool didUpdate = newValue != m_descendantsAreContiguousInStackingOrder;
#endif
        m_descendantsAreContiguousInStackingOrder = newValue;
#if USE(ACCELERATED_COMPOSITING)
        if (didUpdate)
            updateNeedsCompositedScrolling();
#endif
    }
}

void RenderLayer::computeRepaintRects(const RenderLayerModelObject* repaintContainer, const RenderGeometryMap* geometryMap)
{
    ASSERT(!m_visibleContentStatusDirty);

    m_repaintRect = renderer()->clippedOverflowRectForRepaint(repaintContainer);
    m_outlineBox = renderer()->outlineBoundsForRepaint(repaintContainer, geometryMap);
}


void RenderLayer::computeRepaintRectsIncludingDescendants()
{
    // FIXME: computeRepaintRects() has to walk up the parent chain for every layer to compute the rects.
    // We should make this more efficient.
    // FIXME: it's wrong to call this when layout is not up-to-date, which we do.
    computeRepaintRects(renderer()->containerForRepaint());

    for (RenderLayer* layer = firstChild(); layer; layer = layer->nextSibling())
        layer->computeRepaintRectsIncludingDescendants();
}

void RenderLayer::clearRepaintRects()
{
    ASSERT(!m_hasVisibleContent);
    ASSERT(!m_visibleContentStatusDirty);

    m_repaintRect = IntRect();
    m_outlineBox = IntRect();
}

void RenderLayer::updateLayerPositionsAfterDocumentScroll()
{
    ASSERT(this == renderer()->view()->layer());

    RenderGeometryMap geometryMap(UseTransforms);
    updateLayerPositionsAfterScroll(&geometryMap);
}

void RenderLayer::updateLayerPositionsAfterOverflowScroll()
{
    RenderGeometryMap geometryMap(UseTransforms);
    RenderView* view = renderer()->view();
    if (this != view->layer())
        geometryMap.pushMappingsToAncestor(parent(), 0);

    // FIXME: why is it OK to not check the ancestors of this layer in order to
    // initialize the HasSeenViewportConstrainedAncestor and HasSeenAncestorWithOverflowClip flags?
    updateLayerPositionsAfterScroll(&geometryMap, IsOverflowScroll);
}

void RenderLayer::updateLayerPositionsAfterScroll(RenderGeometryMap* geometryMap, UpdateLayerPositionsAfterScrollFlags flags)
{
    // FIXME: This shouldn't be needed, but there are some corner cases where
    // these flags are still dirty. Update so that the check below is valid.
    updateDescendantDependentFlags();

    // If we have no visible content and no visible descendants, there is no point recomputing
    // our rectangles as they will be empty. If our visibility changes, we are expected to
    // recompute all our positions anyway.
    if (!m_hasVisibleDescendant && !m_hasVisibleContent)
        return;

    bool positionChanged = updateLayerPosition();
    if (positionChanged)
        flags |= HasChangedAncestor;

    if (geometryMap)
        geometryMap->pushMappingsToAncestor(this, parent());

    if (flags & HasChangedAncestor || flags & HasSeenViewportConstrainedAncestor || flags & IsOverflowScroll)
        clearClipRects();

    if (renderer()->style()->hasViewportConstrainedPosition())
        flags |= HasSeenViewportConstrainedAncestor;

    if (renderer()->hasOverflowClip())
        flags |= HasSeenAncestorWithOverflowClip;

    if (flags & HasSeenViewportConstrainedAncestor
        || (flags & IsOverflowScroll && flags & HasSeenAncestorWithOverflowClip)) {
        // FIXME: We could track the repaint container as we walk down the tree.
        computeRepaintRects(renderer()->containerForRepaint(), geometryMap);
    } else {
        // Check that our cached rects are correct.
        ASSERT(m_repaintRect == renderer()->clippedOverflowRectForRepaint(renderer()->containerForRepaint()));
        ASSERT(m_outlineBox == renderer()->outlineBoundsForRepaint(renderer()->containerForRepaint(), geometryMap));
    }
    
    for (RenderLayer* child = firstChild(); child; child = child->nextSibling())
        child->updateLayerPositionsAfterScroll(geometryMap, flags);

    // We don't update our reflection as scrolling is a translation which does not change the size()
    // of an object, thus RenderReplica will still repaint itself properly as the layer position was
    // updated above.

    if (m_marquee) {
        bool oldUpdatingMarqueePosition = m_updatingMarqueePosition;
        m_updatingMarqueePosition = true;
        m_marquee->updateMarqueePosition();
        m_updatingMarqueePosition = oldUpdatingMarqueePosition;
    }

    if (geometryMap)
        geometryMap->popMappingsToAncestor(parent());
}

#if USE(ACCELERATED_COMPOSITING)
void RenderLayer::positionNewlyCreatedOverflowControls()
{
    if (!backing()->hasUnpositionedOverflowControlsLayers())
        return;

    RenderGeometryMap geometryMap(UseTransforms);
    RenderView* view = renderer()->view();
    if (this != view->layer() && parent())
        geometryMap.pushMappingsToAncestor(parent(), 0);

    LayoutPoint offsetFromRoot = LayoutPoint(geometryMap.absolutePoint(FloatPoint()));
    positionOverflowControls(toIntSize(roundedIntPoint(offsetFromRoot)));
}
#endif

#if ENABLE(CSS_COMPOSITING)
void RenderLayer::updateBlendMode()
{
    BlendMode newBlendMode = renderer()->style()->blendMode();
    if (newBlendMode != m_blendMode) {
        m_blendMode = newBlendMode;
        if (backing())
            backing()->setBlendMode(newBlendMode);
    }
}
#endif

void RenderLayer::updateTransform()
{
    // hasTransform() on the renderer is also true when there is transform-style: preserve-3d or perspective set,
    // so check style too.
    bool hasTransform = renderer()->hasTransform() && renderer()->style()->hasTransform();
    bool had3DTransform = has3DTransform();

    bool hadTransform = m_transform;
    if (hasTransform != hadTransform) {
        if (hasTransform)
            m_transform = adoptPtr(new TransformationMatrix);
        else
            m_transform.clear();
        
        // Layers with transforms act as clip rects roots, so clear the cached clip rects here.
        clearClipRectsIncludingDescendants();
    }
    
    if (hasTransform) {
        RenderBox* box = renderBox();
        ASSERT(box);
        m_transform->makeIdentity();
        box->style()->applyTransform(*m_transform, box->pixelSnappedBorderBoxRect().size(), RenderStyle::IncludeTransformOrigin);
        makeMatrixRenderable(*m_transform, canRender3DTransforms());
    }

    if (had3DTransform != has3DTransform())
        dirty3DTransformedDescendantStatus();
}

TransformationMatrix RenderLayer::currentTransform(RenderStyle::ApplyTransformOrigin applyOrigin) const
{
    if (!m_transform)
        return TransformationMatrix();

#if USE(ACCELERATED_COMPOSITING)
    if (renderer()->style()->isRunningAcceleratedAnimation()) {
        TransformationMatrix currTransform;
        RefPtr<RenderStyle> style = renderer()->animation()->getAnimatedStyleForRenderer(renderer());
        style->applyTransform(currTransform, renderBox()->pixelSnappedBorderBoxRect().size(), applyOrigin);
        makeMatrixRenderable(currTransform, canRender3DTransforms());
        return currTransform;
    }

    // m_transform includes transform-origin, so we need to recompute the transform here.
    if (applyOrigin == RenderStyle::ExcludeTransformOrigin) {
        RenderBox* box = renderBox();
        TransformationMatrix currTransform;
        box->style()->applyTransform(currTransform, box->pixelSnappedBorderBoxRect().size(), RenderStyle::ExcludeTransformOrigin);
        makeMatrixRenderable(currTransform, canRender3DTransforms());
        return currTransform;
    }
#else
    UNUSED_PARAM(applyOrigin);
#endif

    return *m_transform;
}

TransformationMatrix RenderLayer::renderableTransform(PaintBehavior paintBehavior) const
{
    if (!m_transform)
        return TransformationMatrix();
    
    if (paintBehavior & PaintBehaviorFlattenCompositingLayers) {
        TransformationMatrix matrix = *m_transform;
        makeMatrixRenderable(matrix, false /* flatten 3d */);
        return matrix;
    }

    return *m_transform;
}

RenderLayer* RenderLayer::enclosingOverflowClipLayer(IncludeSelfOrNot includeSelf) const
{
    const RenderLayer* layer = (includeSelf == IncludeSelf) ? this : parent();
    while (layer) {
        if (layer->renderer()->hasOverflowClip())
            return const_cast<RenderLayer*>(layer);

        layer = layer->parent();
    }
    return 0;
}

static bool checkContainingBlockChainForPagination(RenderLayerModelObject* renderer, RenderBox* ancestorColumnsRenderer)
{
    RenderView* view = renderer->view();
    RenderLayerModelObject* prevBlock = renderer;
    RenderBlock* containingBlock;
    for (containingBlock = renderer->containingBlock();
         containingBlock && containingBlock != view && containingBlock != ancestorColumnsRenderer;
         containingBlock = containingBlock->containingBlock())
        prevBlock = containingBlock;
    
    // If the columns block wasn't in our containing block chain, then we aren't paginated by it.
    if (containingBlock != ancestorColumnsRenderer)
        return false;
        
    // If the previous block is absolutely positioned, then we can't be paginated by the columns block.
    if (prevBlock->isOutOfFlowPositioned())
        return false;
        
    // Otherwise we are paginated by the columns block.
    return true;
}

bool RenderLayer::useRegionBasedColumns() const
{
    const Settings* settings = renderer()->document()->settings();
    return settings && settings->regionBasedColumnsEnabled();
}

void RenderLayer::updatePagination()
{
    m_isPaginated = false;
    m_enclosingPaginationLayer = 0;

    if (isComposited() || !parent())
        return; // FIXME: We will have to deal with paginated compositing layers someday.
                // FIXME: For now the RenderView can't be paginated.  Eventually printing will move to a model where it is though.

    // The main difference between the paginated booleans for the old column code and the new column code
    // is that each paginated layer has to paint on its own with the new code. There is no
    // recurring into child layers. This means that the m_isPaginated bits for the new column code can't just be set on
    // "roots" that get split and paint all their descendants. Instead each layer has to be checked individually and
    // genuinely know if it is going to have to split itself up when painting only its contents (and not any other descendant
    // layers). We track an enclosingPaginationLayer instead of using a simple bit, since we want to be able to get back
    // to that layer easily.
    bool regionBasedColumnsUsed = useRegionBasedColumns();
    if (regionBasedColumnsUsed && renderer()->isInFlowRenderFlowThread()) {
        m_enclosingPaginationLayer = this;
        return;
    }

    if (isNormalFlowOnly()) {
        if (regionBasedColumnsUsed) {
            // Content inside a transform is not considered to be paginated, since we simply
            // paint the transform multiple times in each column, so we don't have to use
            // fragments for the transformed content.
            m_enclosingPaginationLayer = parent()->enclosingPaginationLayer();
            if (m_enclosingPaginationLayer && m_enclosingPaginationLayer->hasTransform())
                m_enclosingPaginationLayer = 0;
        } else
            m_isPaginated = parent()->renderer()->hasColumns();
        return;
    }

    // For the new columns code, we want to walk up our containing block chain looking for an enclosing layer. Once
    // we find one, then we just check its pagination status.
    if (regionBasedColumnsUsed) {
        RenderView* view = renderer()->view();
        RenderBlock* containingBlock;
        for (containingBlock = renderer()->containingBlock();
             containingBlock && containingBlock != view;
             containingBlock = containingBlock->containingBlock()) {
            if (containingBlock->hasLayer()) {
                // Content inside a transform is not considered to be paginated, since we simply
                // paint the transform multiple times in each column, so we don't have to use
                // fragments for the transformed content.
                m_enclosingPaginationLayer = containingBlock->layer()->enclosingPaginationLayer();
                if (m_enclosingPaginationLayer && m_enclosingPaginationLayer->hasTransform())
                    m_enclosingPaginationLayer = 0;
                return;
            }
        }
        return;
    }

    // If we're not normal flow, then we need to look for a multi-column object between us and our stacking container.
    RenderLayer* ancestorStackingContainer = stackingContainer();
    for (RenderLayer* curr = parent(); curr; curr = curr->parent()) {
        if (curr->renderer()->hasColumns()) {
            m_isPaginated = checkContainingBlockChainForPagination(renderer(), curr->renderBox());
            return;
        }
        if (curr == ancestorStackingContainer)
            return;
    }
}

bool RenderLayer::canBeStackingContainer() const
{
    if (isStackingContext() || !stackingContainer())
        return true;

    return m_descendantsAreContiguousInStackingOrder;
}

void RenderLayer::setHasVisibleContent()
{ 
    if (m_hasVisibleContent && !m_visibleContentStatusDirty) {
        ASSERT(!parent() || parent()->hasVisibleDescendant());
        return;
    }

    m_visibleContentStatusDirty = false; 
    m_hasVisibleContent = true;
    computeRepaintRects(renderer()->containerForRepaint());
    if (!isNormalFlowOnly()) {
        // We don't collect invisible layers in z-order lists if we are not in compositing mode.
        // As we became visible, we need to dirty our stacking containers ancestors to be properly
        // collected. FIXME: When compositing, we could skip this dirtying phase.
        for (RenderLayer* sc = stackingContainer(); sc; sc = sc->stackingContainer()) {
            sc->dirtyZOrderLists();
            if (sc->hasVisibleContent())
                break;
        }
    }

    if (parent())
        parent()->setAncestorChainHasVisibleDescendant();
}

void RenderLayer::dirtyVisibleContentStatus() 
{ 
    m_visibleContentStatusDirty = true; 
    if (parent())
        parent()->dirtyAncestorChainVisibleDescendantStatus();
}

void RenderLayer::dirtyAncestorChainVisibleDescendantStatus()
{
    for (RenderLayer* layer = this; layer; layer = layer->parent()) {
        if (layer->m_visibleDescendantStatusDirty)
            break;

        layer->m_visibleDescendantStatusDirty = true;
    }
}

void RenderLayer::setAncestorChainHasVisibleDescendant()
{
    for (RenderLayer* layer = this; layer; layer = layer->parent()) {
        if (!layer->m_visibleDescendantStatusDirty && layer->hasVisibleDescendant())
            break;

        layer->m_hasVisibleDescendant = true;
        layer->m_visibleDescendantStatusDirty = false;
    }
}

void RenderLayer::updateDescendantDependentFlags(HashSet<const RenderObject*>* outOfFlowDescendantContainingBlocks)
{
    if (m_visibleDescendantStatusDirty || m_hasSelfPaintingLayerDescendantDirty || m_hasOutOfFlowPositionedDescendantDirty) {
        m_hasVisibleDescendant = false;
        m_hasSelfPaintingLayerDescendant = false;
        m_hasOutOfFlowPositionedDescendant = false;

        HashSet<const RenderObject*> childOutOfFlowDescendantContainingBlocks;
        for (RenderLayer* child = firstChild(); child; child = child->nextSibling()) {
            childOutOfFlowDescendantContainingBlocks.clear();
            child->updateDescendantDependentFlags(&childOutOfFlowDescendantContainingBlocks);

            bool childIsOutOfFlowPositioned = child->renderer() && child->renderer()->isOutOfFlowPositioned();
            if (childIsOutOfFlowPositioned)
                childOutOfFlowDescendantContainingBlocks.add(child->renderer()->containingBlock());

            if (outOfFlowDescendantContainingBlocks) {
                HashSet<const RenderObject*>::const_iterator it = childOutOfFlowDescendantContainingBlocks.begin();
                for (; it != childOutOfFlowDescendantContainingBlocks.end(); ++it)
                    outOfFlowDescendantContainingBlocks->add(*it);
            }

            bool hasVisibleDescendant = child->m_hasVisibleContent || child->m_hasVisibleDescendant;
            bool hasSelfPaintingLayerDescendant = child->isSelfPaintingLayer() || child->hasSelfPaintingLayerDescendant();
            bool hasOutOfFlowPositionedDescendant = !childOutOfFlowDescendantContainingBlocks.isEmpty();

            m_hasVisibleDescendant |= hasVisibleDescendant;
            m_hasSelfPaintingLayerDescendant |= hasSelfPaintingLayerDescendant;
            m_hasOutOfFlowPositionedDescendant |= hasOutOfFlowPositionedDescendant;

            if (m_hasVisibleDescendant && m_hasSelfPaintingLayerDescendant && m_hasOutOfFlowPositionedDescendant)
                break;
        }

        if (outOfFlowDescendantContainingBlocks && renderer())
            outOfFlowDescendantContainingBlocks->remove(renderer());

        m_visibleDescendantStatusDirty = false;
        m_hasSelfPaintingLayerDescendantDirty = false;

#if USE(ACCELERATED_COMPOSITING)
        if (m_hasOutOfFlowPositionedDescendantDirty)
            updateNeedsCompositedScrolling();
#endif
        m_hasOutOfFlowPositionedDescendantDirty = false;
    }

    if (m_visibleContentStatusDirty) {
        if (renderer()->style()->visibility() == VISIBLE)
            m_hasVisibleContent = true;
        else {
            // layer may be hidden but still have some visible content, check for this
            m_hasVisibleContent = false;
            RenderObject* r = renderer()->firstChild();
            while (r) {
                if (r->style()->visibility() == VISIBLE && !r->hasLayer()) {
                    m_hasVisibleContent = true;
                    break;
                }
                if (r->firstChild() && !r->hasLayer())
                    r = r->firstChild();
                else if (r->nextSibling())
                    r = r->nextSibling();
                else {
                    do {
                        r = r->parent();
                        if (r == renderer())
                            r = 0;
                    } while (r && !r->nextSibling());
                    if (r)
                        r = r->nextSibling();
                }
            }
        }    
        m_visibleContentStatusDirty = false; 
    }
}

void RenderLayer::dirty3DTransformedDescendantStatus()
{
    RenderLayer* curr = stackingContainer();
    if (curr)
        curr->m_3DTransformedDescendantStatusDirty = true;
        
    // This propagates up through preserve-3d hierarchies to the enclosing flattening layer.
    // Note that preserves3D() creates stacking context, so we can just run up the stacking containers.
    while (curr && curr->preserves3D()) {
        curr->m_3DTransformedDescendantStatusDirty = true;
        curr = curr->stackingContainer();
    }
}

// Return true if this layer or any preserve-3d descendants have 3d.
bool RenderLayer::update3DTransformedDescendantStatus()
{
    if (m_3DTransformedDescendantStatusDirty) {
        m_has3DTransformedDescendant = false;

        updateZOrderLists();

        // Transformed or preserve-3d descendants can only be in the z-order lists, not
        // in the normal flow list, so we only need to check those.
        if (Vector<RenderLayer*>* positiveZOrderList = posZOrderList()) {
            for (unsigned i = 0; i < positiveZOrderList->size(); ++i)
                m_has3DTransformedDescendant |= positiveZOrderList->at(i)->update3DTransformedDescendantStatus();
        }

        // Now check our negative z-index children.
        if (Vector<RenderLayer*>* negativeZOrderList = negZOrderList()) {
            for (unsigned i = 0; i < negativeZOrderList->size(); ++i)
                m_has3DTransformedDescendant |= negativeZOrderList->at(i)->update3DTransformedDescendantStatus();
        }
        
        m_3DTransformedDescendantStatusDirty = false;
    }
    
    // If we live in a 3d hierarchy, then the layer at the root of that hierarchy needs
    // the m_has3DTransformedDescendant set.
    if (preserves3D())
        return has3DTransform() || m_has3DTransformedDescendant;

    return has3DTransform();
}

bool RenderLayer::updateLayerPosition()
{
    LayoutPoint localPoint;
    LayoutSize inlineBoundingBoxOffset; // We don't put this into the RenderLayer x/y for inlines, so we need to subtract it out when done.
    if (renderer()->isInline() && renderer()->isRenderInline()) {
        RenderInline* inlineFlow = toRenderInline(renderer());
        IntRect lineBox = inlineFlow->linesBoundingBox();
        setSize(lineBox.size());
        inlineBoundingBoxOffset = toSize(lineBox.location());
        localPoint += inlineBoundingBoxOffset;
    } else if (RenderBox* box = renderBox()) {
        // FIXME: Is snapping the size really needed here for the RenderBox case?
        setSize(pixelSnappedIntSize(box->size(), box->location()));
        localPoint += box->topLeftLocationOffset();
    }

    if (!renderer()->isOutOfFlowPositioned() && renderer()->parent()) {
        // We must adjust our position by walking up the render tree looking for the
        // nearest enclosing object with a layer.
        RenderObject* curr = renderer()->parent();
        while (curr && !curr->hasLayer()) {
            if (curr->isBox() && !curr->isTableRow()) {
                // Rows and cells share the same coordinate space (that of the section).
                // Omit them when computing our xpos/ypos.
                localPoint += toRenderBox(curr)->topLeftLocationOffset();
            }
            curr = curr->parent();
        }
        if (curr->isBox() && curr->isTableRow()) {
            // Put ourselves into the row coordinate space.
            localPoint -= toRenderBox(curr)->topLeftLocationOffset();
        }
    }
    
    // Subtract our parent's scroll offset.
    if (renderer()->isOutOfFlowPositioned() && enclosingPositionedAncestor()) {
        RenderLayer* positionedParent = enclosingPositionedAncestor();

        // For positioned layers, we subtract out the enclosing positioned layer's scroll offset.
        if (positionedParent->renderer()->hasOverflowClip()) {
            LayoutSize offset = positionedParent->scrolledContentOffset();
            localPoint -= offset;
        }
        
        if (renderer()->isOutOfFlowPositioned() && positionedParent->renderer()->isInFlowPositioned() && positionedParent->renderer()->isRenderInline()) {
            LayoutSize offset = toRenderInline(positionedParent->renderer())->offsetForInFlowPositionedInline(toRenderBox(renderer()));
            localPoint += offset;
        }
    } else if (parent()) {
        if (parent()->renderer()->hasOverflowClip()) {
            IntSize scrollOffset = parent()->scrolledContentOffset();
            localPoint -= scrollOffset;
        }
    }
    
    bool positionOrOffsetChanged = false;
    if (renderer()->isInFlowPositioned()) {
        LayoutSize newOffset = toRenderBoxModelObject(renderer())->offsetForInFlowPosition();
        positionOrOffsetChanged = newOffset != m_offsetForInFlowPosition;
        m_offsetForInFlowPosition = newOffset;
        localPoint.move(m_offsetForInFlowPosition);
    } else {
        m_offsetForInFlowPosition = LayoutSize();
    }

    // FIXME: We'd really like to just get rid of the concept of a layer rectangle and rely on the renderers.
    localPoint -= inlineBoundingBoxOffset;
    
    positionOrOffsetChanged |= location() != localPoint;
    setLocation(localPoint);
    return positionOrOffsetChanged;
}

TransformationMatrix RenderLayer::perspectiveTransform() const
{
    if (!renderer()->hasTransform())
        return TransformationMatrix();

    RenderStyle* style = renderer()->style();
    if (!style->hasPerspective())
        return TransformationMatrix();

    // Maybe fetch the perspective from the backing?
    const IntRect borderBox = toRenderBox(renderer())->pixelSnappedBorderBoxRect();
    const float boxWidth = borderBox.width();
    const float boxHeight = borderBox.height();

    float perspectiveOriginX = floatValueForLength(style->perspectiveOriginX(), boxWidth);
    float perspectiveOriginY = floatValueForLength(style->perspectiveOriginY(), boxHeight);

    // A perspective origin of 0,0 makes the vanishing point in the center of the element.
    // We want it to be in the top-left, so subtract half the height and width.
    perspectiveOriginX -= boxWidth / 2.0f;
    perspectiveOriginY -= boxHeight / 2.0f;
    
    TransformationMatrix t;
    t.translate(perspectiveOriginX, perspectiveOriginY);
    t.applyPerspective(style->perspective());
    t.translate(-perspectiveOriginX, -perspectiveOriginY);
    
    return t;
}

FloatPoint RenderLayer::perspectiveOrigin() const
{
    if (!renderer()->hasTransform())
        return FloatPoint();

    const LayoutRect borderBox = toRenderBox(renderer())->borderBoxRect();
    RenderStyle* style = renderer()->style();

    return FloatPoint(floatValueForLength(style->perspectiveOriginX(), borderBox.width()),
                      floatValueForLength(style->perspectiveOriginY(), borderBox.height()));
}

RenderLayer* RenderLayer::stackingContainer() const
{
    RenderLayer* layer = parent();
    while (layer && !layer->isStackingContainer())
        layer = layer->parent();

    ASSERT(!layer || layer->isStackingContainer());
    return layer;
}

static inline bool isPositionedContainer(RenderLayer* layer)
{
    RenderLayerModelObject* layerRenderer = layer->renderer();
    return layer->isRootLayer() || layerRenderer->isPositioned() || layer->hasTransform();
}

static inline bool isFixedPositionedContainer(RenderLayer* layer)
{
    return layer->isRootLayer() || layer->hasTransform();
}

RenderLayer* RenderLayer::enclosingPositionedAncestor() const
{
    RenderLayer* curr = parent();
    while (curr && !isPositionedContainer(curr))
        curr = curr->parent();

    return curr;
}

RenderLayer* RenderLayer::enclosingScrollableLayer() const
{
    for (RenderObject* nextRenderer = renderer()->parent(); nextRenderer; nextRenderer = nextRenderer->parent()) {
        if (nextRenderer->isBox() && toRenderBox(nextRenderer)->canBeScrolledAndHasScrollableArea())
            return nextRenderer->enclosingLayer();
    }

    return 0;
}

IntRect RenderLayer::scrollableAreaBoundingBox() const
{
    return renderer()->absoluteBoundingBoxRect();
}

bool RenderLayer::scrollbarAnimationsAreSuppressed() const
{
    RenderView* view = renderer()->view();
    if (!view)
        return false;
    return view->frameView()->scrollbarAnimationsAreSuppressed();
}

RenderLayer* RenderLayer::enclosingTransformedAncestor() const
{
    RenderLayer* curr = parent();
    while (curr && !curr->isRootLayer() && !curr->transform())
        curr = curr->parent();

    return curr;
}

static inline const RenderLayer* compositingContainer(const RenderLayer* layer)
{
    return layer->isNormalFlowOnly() ? layer->parent() : layer->stackingContainer();
}

inline bool RenderLayer::shouldRepaintAfterLayout() const
{
#if USE(ACCELERATED_COMPOSITING)
    if (m_repaintStatus == NeedsNormalRepaint)
        return true;

    // Composited layers that were moved during a positioned movement only
    // layout, don't need to be repainted. They just need to be recomposited.
    ASSERT(m_repaintStatus == NeedsFullRepaintForPositionedMovementLayout);
    return !isComposited();
#else
    return true;
#endif
}

#if USE(ACCELERATED_COMPOSITING)
RenderLayer* RenderLayer::enclosingCompositingLayer(IncludeSelfOrNot includeSelf) const
{
    if (includeSelf == IncludeSelf && isComposited())
        return const_cast<RenderLayer*>(this);

    for (const RenderLayer* curr = compositingContainer(this); curr; curr = compositingContainer(curr)) {
        if (curr->isComposited())
            return const_cast<RenderLayer*>(curr);
    }
         
    return 0;
}

RenderLayer* RenderLayer::enclosingCompositingLayerForRepaint(IncludeSelfOrNot includeSelf) const
{
    if (includeSelf == IncludeSelf && isComposited() && !backing()->paintsIntoCompositedAncestor())
        return const_cast<RenderLayer*>(this);

    for (const RenderLayer* curr = compositingContainer(this); curr; curr = compositingContainer(curr)) {
        if (curr->isComposited() && !curr->backing()->paintsIntoCompositedAncestor())
            return const_cast<RenderLayer*>(curr);
    }
         
    return 0;
}
#endif

#if ENABLE(CSS_FILTERS)
RenderLayer* RenderLayer::enclosingFilterLayer(IncludeSelfOrNot includeSelf) const
{
    const RenderLayer* curr = (includeSelf == IncludeSelf) ? this : parent();
    for (; curr; curr = curr->parent()) {
        if (curr->requiresFullLayerImageForFilters())
            return const_cast<RenderLayer*>(curr);
    }
    
    return 0;
}

RenderLayer* RenderLayer::enclosingFilterRepaintLayer() const
{
    for (const RenderLayer* curr = this; curr; curr = curr->parent()) {
        if ((curr != this && curr->requiresFullLayerImageForFilters()) || curr->isComposited() || curr->isRootLayer())
            return const_cast<RenderLayer*>(curr);
    }
    return 0;
}

void RenderLayer::setFilterBackendNeedsRepaintingInRect(const LayoutRect& rect, bool immediate)
{
    if (rect.isEmpty())
        return;
    
    LayoutRect rectForRepaint = rect;
    renderer()->style()->filterOutsets().expandRect(rectForRepaint);

    RenderLayerFilterInfo* filterInfo = this->filterInfo();
    ASSERT(filterInfo);
    filterInfo->expandDirtySourceRect(rectForRepaint);
    
#if ENABLE(CSS_SHADERS)
    ASSERT(filterInfo->renderer());
    if (filterInfo->renderer()->hasCustomShaderFilter()) {
        // If we have at least one custom shader, we need to update the whole bounding box of the layer, because the
        // shader can address any ouput pixel.
        // Note: This is only for output rect, so there's no need to expand the dirty source rect.
        rectForRepaint.unite(calculateLayerBounds(this));
    }
#endif
    
    RenderLayer* parentLayer = enclosingFilterRepaintLayer();
    ASSERT(parentLayer);
    FloatQuad repaintQuad(rectForRepaint);
    LayoutRect parentLayerRect = renderer()->localToContainerQuad(repaintQuad, parentLayer->renderer()).enclosingBoundingBox();
    
#if USE(ACCELERATED_COMPOSITING)
    if (parentLayer->isComposited()) {
        if (!parentLayer->backing()->paintsIntoWindow()) {
            parentLayer->setBackingNeedsRepaintInRect(parentLayerRect);
            return;
        }
        // If the painting goes to window, redirect the painting to the parent RenderView.
        parentLayer = renderer()->view()->layer();
        parentLayerRect = renderer()->localToContainerQuad(repaintQuad, parentLayer->renderer()).enclosingBoundingBox();
    }
#endif

    if (parentLayer->paintsWithFilters()) {
        parentLayer->setFilterBackendNeedsRepaintingInRect(parentLayerRect, immediate);
        return;        
    }
    
    if (parentLayer->isRootLayer()) {
        RenderView* view = toRenderView(parentLayer->renderer());
        view->repaintViewRectangle(parentLayerRect, immediate);
        return;
    }
    
    ASSERT_NOT_REACHED();
}

bool RenderLayer::hasAncestorWithFilterOutsets() const
{
    for (const RenderLayer* curr = this; curr; curr = curr->parent()) {
        RenderLayerModelObject* renderer = curr->renderer();
        if (renderer->style()->hasFilterOutsets())
            return true;
    }
    return false;
}
#endif
    
RenderLayer* RenderLayer::clippingRootForPainting() const
{
#if USE(ACCELERATED_COMPOSITING)
    if (isComposited())
        return const_cast<RenderLayer*>(this);
#endif

    const RenderLayer* current = this;
    while (current) {
        if (current->isRootLayer())
            return const_cast<RenderLayer*>(current);

        current = compositingContainer(current);
        ASSERT(current);
        if (current->transform()
#if USE(ACCELERATED_COMPOSITING)
            || (current->isComposited() && !current->backing()->paintsIntoCompositedAncestor())
#endif
        )
            return const_cast<RenderLayer*>(current);
    }

    ASSERT_NOT_REACHED();
    return 0;
}

LayoutPoint RenderLayer::absoluteToContents(const LayoutPoint& absolutePoint) const
{
    // We don't use convertToLayerCoords because it doesn't know about transforms
    return roundedLayoutPoint(renderer()->absoluteToLocal(absolutePoint, UseTransforms));
}

bool RenderLayer::cannotBlitToWindow() const
{
    if (isTransparent() || hasReflection() || hasTransform())
        return true;
    if (!parent())
        return false;
    return parent()->cannotBlitToWindow();
}

bool RenderLayer::isTransparent() const
{
#if ENABLE(SVG)
    if (renderer()->node() && renderer()->node()->namespaceURI() == SVGNames::svgNamespaceURI)
        return false;
#endif
    return renderer()->isTransparent() || renderer()->hasMask();
}

RenderLayer* RenderLayer::transparentPaintingAncestor()
{
    if (isComposited())
        return 0;

    for (RenderLayer* curr = parent(); curr; curr = curr->parent()) {
        if (curr->isComposited())
            return 0;
        if (curr->isTransparent())
            return curr;
    }
    return 0;
}

enum TransparencyClipBoxBehavior {
    PaintingTransparencyClipBox,
    HitTestingTransparencyClipBox
};

enum TransparencyClipBoxMode {
    DescendantsOfTransparencyClipBox,
    RootOfTransparencyClipBox
};

static LayoutRect transparencyClipBox(const RenderLayer*, const RenderLayer* rootLayer, TransparencyClipBoxBehavior, TransparencyClipBoxMode, PaintBehavior = 0);

static void expandClipRectForDescendantsAndReflection(LayoutRect& clipRect, const RenderLayer* layer, const RenderLayer* rootLayer,
    TransparencyClipBoxBehavior transparencyBehavior, PaintBehavior paintBehavior)
{
    // If we have a mask, then the clip is limited to the border box area (and there is
    // no need to examine child layers).
    if (!layer->renderer()->hasMask()) {
        // Note: we don't have to walk z-order lists since transparent elements always establish
        // a stacking container. This means we can just walk the layer tree directly.
        for (RenderLayer* curr = layer->firstChild(); curr; curr = curr->nextSibling()) {
            if (!layer->reflection() || layer->reflectionLayer() != curr)
                clipRect.unite(transparencyClipBox(curr, rootLayer, transparencyBehavior, DescendantsOfTransparencyClipBox, paintBehavior));
        }
    }

    // If we have a reflection, then we need to account for that when we push the clip.  Reflect our entire
    // current transparencyClipBox to catch all child layers.
    // FIXME: Accelerated compositing will eventually want to do something smart here to avoid incorporating this
    // size into the parent layer.
    if (layer->renderer()->hasReflection()) {
        LayoutPoint delta;
        layer->convertToLayerCoords(rootLayer, delta);
        clipRect.move(-delta.x(), -delta.y());
        clipRect.unite(layer->renderBox()->reflectedRect(clipRect));
        clipRect.moveBy(delta);
    }
}

static LayoutRect transparencyClipBox(const RenderLayer* layer, const RenderLayer* rootLayer, TransparencyClipBoxBehavior transparencyBehavior,
    TransparencyClipBoxMode transparencyMode, PaintBehavior paintBehavior)
{
    // FIXME: Although this function completely ignores CSS-imposed clipping, we did already intersect with the
    // paintDirtyRect, and that should cut down on the amount we have to paint.  Still it
    // would be better to respect clips.
    
    if (rootLayer != layer && ((transparencyBehavior == PaintingTransparencyClipBox && layer->paintsWithTransform(paintBehavior))
        || (transparencyBehavior == HitTestingTransparencyClipBox && layer->hasTransform()))) {
        // The best we can do here is to use enclosed bounding boxes to establish a "fuzzy" enough clip to encompass
        // the transformed layer and all of its children.
        const RenderLayer* paginationLayer = transparencyMode == DescendantsOfTransparencyClipBox ? layer->enclosingPaginationLayer() : 0;
        const RenderLayer* rootLayerForTransform = paginationLayer ? paginationLayer : rootLayer;
        LayoutPoint delta;
        layer->convertToLayerCoords(rootLayerForTransform, delta);

        TransformationMatrix transform;
        transform.translate(delta.x(), delta.y());
        transform = transform * *layer->transform();

        // We don't use fragment boxes when collecting a transformed layer's bounding box, since it always
        // paints unfragmented.
        LayoutRect clipRect = layer->boundingBox(layer);
        expandClipRectForDescendantsAndReflection(clipRect, layer, layer, transparencyBehavior, paintBehavior);
#if ENABLE(CSS_FILTERS)
        layer->renderer()->style()->filterOutsets().expandRect(clipRect);
#endif
        LayoutRect result = transform.mapRect(clipRect);
        if (!paginationLayer)
            return result;
        
        // We have to break up the transformed extent across our columns.
        // Split our box up into the actual fragment boxes that render in the columns/pages and unite those together to
        // get our true bounding box.
        RenderFlowThread* enclosingFlowThread = toRenderFlowThread(paginationLayer->renderer());
        result = enclosingFlowThread->fragmentsBoundingBox(result);
        
        LayoutPoint rootLayerDelta;
        paginationLayer->convertToLayerCoords(rootLayer, rootLayerDelta);
        result.moveBy(rootLayerDelta);
        return result;
    }
    
    LayoutRect clipRect = layer->boundingBox(rootLayer, RenderLayer::UseFragmentBoxes);
    expandClipRectForDescendantsAndReflection(clipRect, layer, rootLayer, transparencyBehavior, paintBehavior);
#if ENABLE(CSS_FILTERS)
    layer->renderer()->style()->filterOutsets().expandRect(clipRect);
#endif
    return clipRect;
}

LayoutRect RenderLayer::paintingExtent(const RenderLayer* rootLayer, const LayoutRect& paintDirtyRect, PaintBehavior paintBehavior)
{
    return intersection(transparencyClipBox(this, rootLayer, PaintingTransparencyClipBox, RootOfTransparencyClipBox, paintBehavior), paintDirtyRect);
}

void RenderLayer::beginTransparencyLayers(GraphicsContext* context, const RenderLayer* rootLayer, const LayoutRect& paintDirtyRect, PaintBehavior paintBehavior)
{
    if (context->paintingDisabled() || (paintsWithTransparency(paintBehavior) && m_usedTransparency))
        return;
    
    RenderLayer* ancestor = transparentPaintingAncestor();
    if (ancestor)
        ancestor->beginTransparencyLayers(context, rootLayer, paintDirtyRect, paintBehavior);
    
    if (paintsWithTransparency(paintBehavior)) {
        m_usedTransparency = true;
        context->save();
        LayoutRect clipRect = paintingExtent(rootLayer, paintDirtyRect, paintBehavior);
        context->clip(clipRect);
        context->beginTransparencyLayer(renderer()->opacity());
#ifdef REVEAL_TRANSPARENCY_LAYERS
        context->setFillColor(Color(0.0f, 0.0f, 0.5f, 0.2f), ColorSpaceDeviceRGB);
        context->fillRect(clipRect);
#endif
    }
}

void* RenderLayer::operator new(size_t sz, RenderArena* renderArena)
{
    return renderArena->allocate(sz);
}

void RenderLayer::operator delete(void* ptr, size_t sz)
{
    // Stash size where destroy can find it.
    *(size_t *)ptr = sz;
}

void RenderLayer::destroy(RenderArena* renderArena)
{
    delete this;

    // Recover the size left there for us by operator delete and free the memory.
    renderArena->free(*(size_t *)this, this);
}

void RenderLayer::addChild(RenderLayer* child, RenderLayer* beforeChild)
{
    RenderLayer* prevSibling = beforeChild ? beforeChild->previousSibling() : lastChild();
    if (prevSibling) {
        child->setPreviousSibling(prevSibling);
        prevSibling->setNextSibling(child);
        ASSERT(prevSibling != child);
    } else
        setFirstChild(child);

    if (beforeChild) {
        beforeChild->setPreviousSibling(child);
        child->setNextSibling(beforeChild);
        ASSERT(beforeChild != child);
    } else
        setLastChild(child);

    child->setParent(this);

    if (child->isNormalFlowOnly())
        dirtyNormalFlowList();

    if (!child->isNormalFlowOnly() || child->firstChild()) {
        // Dirty the z-order list in which we are contained. The stackingContainer() can be null in the
        // case where we're building up generated content layers. This is ok, since the lists will start
        // off dirty in that case anyway.
        child->dirtyStackingContainerZOrderLists();
    }

    child->updateDescendantDependentFlags();
    if (child->m_hasVisibleContent || child->m_hasVisibleDescendant)
        setAncestorChainHasVisibleDescendant();

    if (child->isSelfPaintingLayer() || child->hasSelfPaintingLayerDescendant())
        setAncestorChainHasSelfPaintingLayerDescendant();

    if (child->renderer() && (child->renderer()->isOutOfFlowPositioned() || child->hasOutOfFlowPositionedDescendant()))
        setAncestorChainHasOutOfFlowPositionedDescendant(child->renderer()->containingBlock());

#if USE(ACCELERATED_COMPOSITING)
    compositor()->layerWasAdded(this, child);
#endif
}

RenderLayer* RenderLayer::removeChild(RenderLayer* oldChild)
{
#if USE(ACCELERATED_COMPOSITING)
    if (!renderer()->documentBeingDestroyed())
        compositor()->layerWillBeRemoved(this, oldChild);
#endif

    // remove the child
    if (oldChild->previousSibling())
        oldChild->previousSibling()->setNextSibling(oldChild->nextSibling());
    if (oldChild->nextSibling())
        oldChild->nextSibling()->setPreviousSibling(oldChild->previousSibling());

    if (m_first == oldChild)
        m_first = oldChild->nextSibling();
    if (m_last == oldChild)
        m_last = oldChild->previousSibling();

    if (oldChild->isNormalFlowOnly())
        dirtyNormalFlowList();
    if (!oldChild->isNormalFlowOnly() || oldChild->firstChild()) { 
        // Dirty the z-order list in which we are contained.  When called via the
        // reattachment process in removeOnlyThisLayer, the layer may already be disconnected
        // from the main layer tree, so we need to null-check the |stackingContainer| value.
        oldChild->dirtyStackingContainerZOrderLists();
    }

    if ((oldChild->renderer() && oldChild->renderer()->isOutOfFlowPositioned()) || oldChild->hasOutOfFlowPositionedDescendant())
        dirtyAncestorChainHasOutOfFlowPositionedDescendantStatus();

    oldChild->setPreviousSibling(0);
    oldChild->setNextSibling(0);
    oldChild->setParent(0);
    
    oldChild->updateDescendantDependentFlags();
    if (oldChild->m_hasVisibleContent || oldChild->m_hasVisibleDescendant)
        dirtyAncestorChainVisibleDescendantStatus();

    if (oldChild->isSelfPaintingLayer() || oldChild->hasSelfPaintingLayerDescendant())
        dirtyAncestorChainHasSelfPaintingLayerDescendantStatus();

    return oldChild;
}

void RenderLayer::removeOnlyThisLayer()
{
    if (!m_parent)
        return;

    // Mark that we are about to lose our layer. This makes render tree
    // walks ignore this layer while we're removing it.
    m_renderer->setHasLayer(false);

#if USE(ACCELERATED_COMPOSITING)
    compositor()->layerWillBeRemoved(m_parent, this);
#endif

    // Dirty the clip rects.
    clearClipRectsIncludingDescendants();

    RenderLayer* nextSib = nextSibling();

    // Remove the child reflection layer before moving other child layers.
    // The reflection layer should not be moved to the parent.
    if (reflection())
        removeChild(reflectionLayer());

    // Now walk our kids and reattach them to our parent.
    RenderLayer* current = m_first;
    while (current) {
        RenderLayer* next = current->nextSibling();
        removeChild(current);
        m_parent->addChild(current, nextSib);
        current->setRepaintStatus(NeedsFullRepaint);
        // updateLayerPositions depends on hasLayer() already being false for proper layout.
        ASSERT(!renderer()->hasLayer());
        current->updateLayerPositions(0); // FIXME: use geometry map.
        current = next;
    }

    // Remove us from the parent.
    m_parent->removeChild(this);
    m_renderer->destroyLayer();
}

void RenderLayer::insertOnlyThisLayer()
{
    if (!m_parent && renderer()->parent()) {
        // We need to connect ourselves when our renderer() has a parent.
        // Find our enclosingLayer and add ourselves.
        RenderLayer* parentLayer = renderer()->parent()->enclosingLayer();
        ASSERT(parentLayer);
        RenderLayer* beforeChild = parentLayer->reflectionLayer() != this ? renderer()->parent()->findNextLayer(parentLayer, renderer()) : 0;
        parentLayer->addChild(this, beforeChild);
    }

    // Remove all descendant layers from the hierarchy and add them to the new position.
    for (RenderObject* curr = renderer()->firstChild(); curr; curr = curr->nextSibling())
        curr->moveLayers(m_parent, this);

    // Clear out all the clip rects.
    clearClipRectsIncludingDescendants();
}

void RenderLayer::convertToPixelSnappedLayerCoords(const RenderLayer* ancestorLayer, IntPoint& roundedLocation, ColumnOffsetAdjustment adjustForColumns) const
{
    LayoutPoint location = roundedLocation;
    convertToLayerCoords(ancestorLayer, location, adjustForColumns);
    roundedLocation = roundedIntPoint(location);
}

void RenderLayer::convertToPixelSnappedLayerCoords(const RenderLayer* ancestorLayer, IntRect& roundedRect, ColumnOffsetAdjustment adjustForColumns) const
{
    LayoutRect rect = roundedRect;
    convertToLayerCoords(ancestorLayer, rect, adjustForColumns);
    roundedRect = pixelSnappedIntRect(rect);
}

// Returns the layer reached on the walk up towards the ancestor.
static inline const RenderLayer* accumulateOffsetTowardsAncestor(const RenderLayer* layer, const RenderLayer* ancestorLayer, LayoutPoint& location, RenderLayer::ColumnOffsetAdjustment adjustForColumns)
{
    ASSERT(ancestorLayer != layer);

    const RenderLayerModelObject* renderer = layer->renderer();
    EPosition position = renderer->style()->position();

    // FIXME: Special casing RenderFlowThread so much for fixed positioning here is not great.
    RenderFlowThread* fixedFlowThreadContainer = position == FixedPosition ? renderer->flowThreadContainingBlock() : 0;
    if (fixedFlowThreadContainer && !fixedFlowThreadContainer->isOutOfFlowPositioned())
        fixedFlowThreadContainer = 0;

    // FIXME: Positioning of out-of-flow(fixed, absolute) elements collected in a RenderFlowThread
    // may need to be revisited in a future patch.
    // If the fixed renderer is inside a RenderFlowThread, we should not compute location using localToAbsolute,
    // since localToAbsolute maps the coordinates from named flow to regions coordinates and regions can be
    // positioned in a completely different place in the viewport (RenderView).
    if (position == FixedPosition && !fixedFlowThreadContainer && (!ancestorLayer || ancestorLayer == renderer->view()->layer())) {
        // If the fixed layer's container is the root, just add in the offset of the view. We can obtain this by calling
        // localToAbsolute() on the RenderView.
        FloatPoint absPos = renderer->localToAbsolute(FloatPoint(), IsFixed);
        location += LayoutSize(absPos.x(), absPos.y());
        return ancestorLayer;
    }

    // For the fixed positioned elements inside a render flow thread, we should also skip the code path below
    // Otherwise, for the case of ancestorLayer == rootLayer and fixed positioned element child of a transformed
    // element in render flow thread, we will hit the fixed positioned container before hitting the ancestor layer.
    if (position == FixedPosition && !fixedFlowThreadContainer) {
        // For a fixed layers, we need to walk up to the root to see if there's a fixed position container
        // (e.g. a transformed layer). It's an error to call convertToLayerCoords() across a layer with a transform,
        // so we should always find the ancestor at or before we find the fixed position container.
        RenderLayer* fixedPositionContainerLayer = 0;
        bool foundAncestor = false;
        for (RenderLayer* currLayer = layer->parent(); currLayer; currLayer = currLayer->parent()) {
            if (currLayer == ancestorLayer)
                foundAncestor = true;

            if (isFixedPositionedContainer(currLayer)) {
                fixedPositionContainerLayer = currLayer;
                ASSERT_UNUSED(foundAncestor, foundAncestor);
                break;
            }
        }
        
        ASSERT(fixedPositionContainerLayer); // We should have hit the RenderView's layer at least.

        if (fixedPositionContainerLayer != ancestorLayer) {
            LayoutPoint fixedContainerCoords;
            layer->convertToLayerCoords(fixedPositionContainerLayer, fixedContainerCoords);

            LayoutPoint ancestorCoords;
            ancestorLayer->convertToLayerCoords(fixedPositionContainerLayer, ancestorCoords);

            location += (fixedContainerCoords - ancestorCoords);
            return ancestorLayer;
        }
    }
    
    RenderLayer* parentLayer;
    if (position == AbsolutePosition || position == FixedPosition) {
        // Do what enclosingPositionedAncestor() does, but check for ancestorLayer along the way.
        parentLayer = layer->parent();
        bool foundAncestorFirst = false;
        while (parentLayer) {
            // RenderFlowThread is a positioned container, child of RenderView, positioned at (0,0).
            // This implies that, for out-of-flow positioned elements inside a RenderFlowThread,
            // we are bailing out before reaching root layer.
            if (isPositionedContainer(parentLayer))
                break;

            if (parentLayer == ancestorLayer) {
                foundAncestorFirst = true;
                break;
            }

            parentLayer = parentLayer->parent();
        }

        // We should not reach RenderView layer past the RenderFlowThread layer for any
        // children of the RenderFlowThread.
        if (renderer->flowThreadContainingBlock() && !layer->isOutOfFlowRenderFlowThread())
            ASSERT(parentLayer != renderer->view()->layer());

        if (foundAncestorFirst) {
            // Found ancestorLayer before the abs. positioned container, so compute offset of both relative
            // to enclosingPositionedAncestor and subtract.
            RenderLayer* positionedAncestor = parentLayer->enclosingPositionedAncestor();

            LayoutPoint thisCoords;
            layer->convertToLayerCoords(positionedAncestor, thisCoords);
            
            LayoutPoint ancestorCoords;
            ancestorLayer->convertToLayerCoords(positionedAncestor, ancestorCoords);

            location += (thisCoords - ancestorCoords);
            return ancestorLayer;
        }
    } else
        parentLayer = layer->parent();
    
    if (!parentLayer)
        return 0;

    location += toSize(layer->location());

    if (adjustForColumns == RenderLayer::AdjustForColumns) {
        if (RenderLayer* parentLayer = layer->parent()) {
            LayoutSize layerColumnOffset;
            parentLayer->renderer()->adjustForColumns(layerColumnOffset, location);
            location += layerColumnOffset;
        }
    }

    return parentLayer;
}

void RenderLayer::convertToLayerCoords(const RenderLayer* ancestorLayer, LayoutPoint& location, ColumnOffsetAdjustment adjustForColumns) const
{
    if (ancestorLayer == this)
        return;

    const RenderLayer* currLayer = this;
    while (currLayer && currLayer != ancestorLayer)
        currLayer = accumulateOffsetTowardsAncestor(currLayer, ancestorLayer, location, adjustForColumns);
}

void RenderLayer::convertToLayerCoords(const RenderLayer* ancestorLayer, LayoutRect& rect, ColumnOffsetAdjustment adjustForColumns) const
{
    LayoutPoint delta;
    convertToLayerCoords(ancestorLayer, delta, adjustForColumns);
    rect.move(-delta.x(), -delta.y());
}

#if USE(ACCELERATED_COMPOSITING)
bool RenderLayer::usesCompositedScrolling() const
{
    return isComposited() && backing()->scrollingLayer();
}

bool RenderLayer::needsCompositedScrolling() const
{
    return m_needsCompositedScrolling;
}

void RenderLayer::updateNeedsCompositedScrolling()
{
    bool oldNeedsCompositedScrolling = m_needsCompositedScrolling;

    FrameView* frameView = renderer()->view()->frameView();
    if (!frameView || !frameView->containsScrollableArea(this))
        m_needsCompositedScrolling = false;
    else {
        bool forceUseCompositedScrolling = acceleratedCompositingForOverflowScrollEnabled()
            && canBeStackingContainer()
            && !hasOutOfFlowPositionedDescendant();

#if ENABLE(ACCELERATED_OVERFLOW_SCROLLING)
        m_needsCompositedScrolling = forceUseCompositedScrolling || renderer()->style()->useTouchOverflowScrolling();
#else
        m_needsCompositedScrolling = forceUseCompositedScrolling;
#endif
        // We gather a boolean value for use with Google UMA histograms to
        // quantify the actual effects of a set of patches attempting to
        // relax composited scrolling requirements, thereby increasing the
        // number of composited overflow divs.
        if (acceleratedCompositingForOverflowScrollEnabled())
            HistogramSupport::histogramEnumeration("Renderer.NeedsCompositedScrolling", m_needsCompositedScrolling, 2);
    }

    if (oldNeedsCompositedScrolling != m_needsCompositedScrolling) {
        updateSelfPaintingLayer();
        if (isStackingContainer())
            dirtyZOrderLists();
        else
            clearZOrderLists();

        dirtyStackingContainerZOrderLists();

        compositor()->setShouldReevaluateCompositingAfterLayout();
        compositor()->setCompositingLayersNeedRebuild();
    }
}
#endif

static inline int adjustedScrollDelta(int beginningDelta) {
    // This implemention matches Firefox's.
    // http://mxr.mozilla.org/firefox/source/toolkit/content/widgets/browser.xml#856.
    const int speedReducer = 12;

    int adjustedDelta = beginningDelta / speedReducer;
    if (adjustedDelta > 1)
        adjustedDelta = static_cast<int>(adjustedDelta * sqrt(static_cast<double>(adjustedDelta))) - 1;
    else if (adjustedDelta < -1)
        adjustedDelta = static_cast<int>(adjustedDelta * sqrt(static_cast<double>(-adjustedDelta))) + 1;

    return adjustedDelta;
}

static inline IntSize adjustedScrollDelta(const IntSize& delta)
{
    return IntSize(adjustedScrollDelta(delta.width()), adjustedScrollDelta(delta.height()));
}

void RenderLayer::panScrollFromPoint(const IntPoint& sourcePoint)
{
    Frame* frame = renderer()->frame();
    if (!frame)
        return;
    
    IntPoint lastKnownMousePosition = frame->eventHandler()->lastKnownMousePosition();
    
    // We need to check if the last known mouse position is out of the window. When the mouse is out of the window, the position is incoherent
    static IntPoint previousMousePosition;
    if (lastKnownMousePosition.x() < 0 || lastKnownMousePosition.y() < 0)
        lastKnownMousePosition = previousMousePosition;
    else
        previousMousePosition = lastKnownMousePosition;

    IntSize delta = lastKnownMousePosition - sourcePoint;

    if (abs(delta.width()) <= ScrollView::noPanScrollRadius) // at the center we let the space for the icon
        delta.setWidth(0);
    if (abs(delta.height()) <= ScrollView::noPanScrollRadius)
        delta.setHeight(0);

    scrollByRecursively(adjustedScrollDelta(delta), ScrollOffsetClamped);
}

void RenderLayer::scrollByRecursively(const IntSize& delta, ScrollOffsetClamping clamp)
{
    if (delta.isZero())
        return;

    bool restrictedByLineClamp = false;
    if (renderer()->parent())
        restrictedByLineClamp = !renderer()->parent()->style()->lineClamp().isNone();

    if (renderer()->hasOverflowClip() && !restrictedByLineClamp) {
        IntSize newScrollOffset = scrollOffset() + delta;
        scrollToOffset(newScrollOffset, clamp);

        // If this layer can't do the scroll we ask the next layer up that can scroll to try
        IntSize remainingScrollOffset = newScrollOffset - scrollOffset();
        if (!remainingScrollOffset.isZero() && renderer()->parent()) {
            if (RenderLayer* scrollableLayer = enclosingScrollableLayer())
                scrollableLayer->scrollByRecursively(remainingScrollOffset);

            Frame* frame = renderer()->frame();
            if (frame)
                frame->eventHandler()->updateAutoscrollRenderer();
        }
    } else if (renderer()->view()->frameView()) {
        // If we are here, we were called on a renderer that can be programmatically scrolled, but doesn't
        // have an overflow clip. Which means that it is a document node that can be scrolled.
        renderer()->view()->frameView()->scrollBy(delta);

        // FIXME: If we didn't scroll the whole way, do we want to try looking at the frames ownerElement? 
        // https://bugs.webkit.org/show_bug.cgi?id=28237
    }
}

IntSize RenderLayer::clampScrollOffset(const IntSize& scrollOffset) const
{
    RenderBox* box = renderBox();
    ASSERT(box);

    int maxX = scrollWidth() - box->pixelSnappedClientWidth();
    int maxY = scrollHeight() - box->pixelSnappedClientHeight();

    int x = max(min(scrollOffset.width(), maxX), 0);
    int y = max(min(scrollOffset.height(), maxY), 0);
    return IntSize(x, y);
}

void RenderLayer::scrollToOffset(const IntSize& scrollOffset, ScrollOffsetClamping clamp)
{
    IntSize newScrollOffset = clamp == ScrollOffsetClamped ? clampScrollOffset(scrollOffset) : scrollOffset;
    if (newScrollOffset != this->scrollOffset())
        scrollToOffsetWithoutAnimation(IntPoint(newScrollOffset));
}

void RenderLayer::scrollTo(int x, int y)
{
    RenderBox* box = renderBox();
    if (!box)
        return;

    if (box->style()->overflowX() != OMARQUEE) {
        // Ensure that the dimensions will be computed if they need to be (for overflow:hidden blocks).
        if (m_scrollDimensionsDirty)
            computeScrollDimensions();
    }
    
    // FIXME: Eventually, we will want to perform a blit.  For now never
    // blit, since the check for blitting is going to be very
    // complicated (since it will involve testing whether our layer
    // is either occluded by another layer or clipped by an enclosing
    // layer or contains fixed backgrounds, etc.).
    IntSize newScrollOffset = IntSize(x - scrollOrigin().x(), y - scrollOrigin().y());
    if (m_scrollOffset == newScrollOffset)
        return;
    m_scrollOffset = newScrollOffset;

    Frame* frame = renderer()->frame();
    InspectorInstrumentation::willScrollLayer(frame);

    RenderView* view = renderer()->view();
    
    // We should have a RenderView if we're trying to scroll.
    ASSERT(view);

    // Update the positions of our child layers (if needed as only fixed layers should be impacted by a scroll).
    // We don't update compositing layers, because we need to do a deep update from the compositing ancestor.
    bool inLayout = view ? view->frameView()->isInLayout() : false;
    if (!inLayout) {
        // If we're in the middle of layout, we'll just update layers once layout has finished.
        updateLayerPositionsAfterOverflowScroll();
        if (view) {
            // Update regions, scrolling may change the clip of a particular region.
#if ENABLE(DASHBOARD_SUPPORT) || ENABLE(DRAGGABLE_REGION)
            view->frameView()->updateAnnotatedRegions();
#endif
            view->updateWidgetPositions();
        }

        if (!m_updatingMarqueePosition) {
            // Avoid updating compositing layers if, higher on the stack, we're already updating layer
            // positions. Updating layer positions requires a full walk of up-to-date RenderLayers, and
            // in this case we're still updating their positions; we'll update compositing layers later
            // when that completes.
            updateCompositingLayersAfterScroll();
        }
    }

    RenderLayerModelObject* repaintContainer = renderer()->containerForRepaint();
    if (frame) {
        // The caret rect needs to be invalidated after scrolling
        frame->selection()->setCaretRectNeedsUpdate();

        FloatQuad quadForFakeMouseMoveEvent = FloatQuad(m_repaintRect);
        if (repaintContainer)
            quadForFakeMouseMoveEvent = repaintContainer->localToAbsoluteQuad(quadForFakeMouseMoveEvent);
        frame->eventHandler()->dispatchFakeMouseMoveEventSoonInQuad(quadForFakeMouseMoveEvent);
    }

    bool requiresRepaint = true;

#if USE(ACCELERATED_COMPOSITING)
    if (compositor()->inCompositingMode() && usesCompositedScrolling())
        requiresRepaint = false;
#endif

    // Just schedule a full repaint of our object.
    if (view && requiresRepaint)
        renderer()->repaintUsingContainer(repaintContainer, pixelSnappedIntRect(m_repaintRect));

    // Schedule the scroll DOM event.
    if (renderer()->node())
        renderer()->node()->document()->eventQueue()->enqueueOrDispatchScrollEvent(renderer()->node(), DocumentEventQueue::ScrollEventElementTarget);

    InspectorInstrumentation::didScrollLayer(frame);
    if (scrollsOverflow())
        frame->loader()->client()->didChangeScrollOffset(); 
}

static inline bool frameElementAndViewPermitScroll(HTMLFrameElementBase* frameElementBase, FrameView* frameView) 
{
    // If scrollbars aren't explicitly forbidden, permit scrolling.
    if (frameElementBase && frameElementBase->scrollingMode() != ScrollbarAlwaysOff)
        return true;

    // If scrollbars are forbidden, user initiated scrolls should obviously be ignored.
    if (frameView->wasScrolledByUser())
        return false;

    // Forbid autoscrolls when scrollbars are off, but permits other programmatic scrolls,
    // like navigation to an anchor.
    return !frameView->frame()->eventHandler()->autoscrollInProgress();
}

void RenderLayer::scrollRectToVisible(const LayoutRect& rect, const ScrollAlignment& alignX, const ScrollAlignment& alignY)
{
    RenderLayer* parentLayer = 0;
    LayoutRect newRect = rect;

    // We may end up propagating a scroll event. It is important that we suspend events until 
    // the end of the function since they could delete the layer or the layer's renderer().
    FrameView* frameView = renderer()->document()->view();
    if (frameView)
        frameView->pauseScheduledEvents();

    bool restrictedByLineClamp = false;
    if (renderer()->parent()) {
        parentLayer = renderer()->parent()->enclosingLayer();
        restrictedByLineClamp = !renderer()->parent()->style()->lineClamp().isNone();
    }

    if (renderer()->hasOverflowClip() && !restrictedByLineClamp) {
        // Don't scroll to reveal an overflow layer that is restricted by the -webkit-line-clamp property.
        // This will prevent us from revealing text hidden by the slider in Safari RSS.
        RenderBox* box = renderBox();
        ASSERT(box);
        LayoutRect localExposeRect(box->absoluteToLocalQuad(FloatQuad(FloatRect(rect)), UseTransforms).boundingBox());
        LayoutRect layerBounds(0, 0, box->clientWidth(), box->clientHeight());
        LayoutRect r = getRectToExpose(layerBounds, layerBounds, localExposeRect, alignX, alignY);

        IntSize clampedScrollOffset = clampScrollOffset(scrollOffset() + toIntSize(roundedIntRect(r).location()));
        if (clampedScrollOffset != scrollOffset()) {
            IntSize oldScrollOffset = scrollOffset();
            scrollToOffset(clampedScrollOffset);
            IntSize scrollOffsetDifference = scrollOffset() - oldScrollOffset;
            localExposeRect.move(-scrollOffsetDifference);
            newRect = LayoutRect(box->localToAbsoluteQuad(FloatQuad(FloatRect(localExposeRect)), UseTransforms).boundingBox());
        }
    } else if (!parentLayer && renderer()->isBox() && renderBox()->canBeProgramaticallyScrolled()) {
        if (frameView) {
            Element* ownerElement = 0;
            if (renderer()->document())
                ownerElement = renderer()->document()->ownerElement();

            if (ownerElement && ownerElement->renderer()) {
                HTMLFrameElementBase* frameElementBase = 0;

                if (ownerElement->hasTagName(frameTag) || ownerElement->hasTagName(iframeTag))
                    frameElementBase = toHTMLFrameElementBase(ownerElement);

                if (frameElementAndViewPermitScroll(frameElementBase, frameView)) {
                    LayoutRect viewRect = frameView->visibleContentRect();
                    LayoutRect exposeRect = getRectToExpose(viewRect, viewRect, rect, alignX, alignY);

                    int xOffset = roundToInt(exposeRect.x());
                    int yOffset = roundToInt(exposeRect.y());
                    // Adjust offsets if they're outside of the allowable range.
                    xOffset = max(0, min(frameView->contentsWidth(), xOffset));
                    yOffset = max(0, min(frameView->contentsHeight(), yOffset));

                    frameView->setScrollPosition(IntPoint(xOffset, yOffset));
                    if (frameView->safeToPropagateScrollToParent()) {
                        parentLayer = ownerElement->renderer()->enclosingLayer();
                        // FIXME: This doesn't correctly convert the rect to
                        // absolute coordinates in the parent.
                        newRect.setX(rect.x() - frameView->scrollX() + frameView->x());
                        newRect.setY(rect.y() - frameView->scrollY() + frameView->y());
                    } else
                        parentLayer = 0;
                }
            } else {
                LayoutRect viewRect = frameView->visibleContentRect();
                LayoutRect visibleRectRelativeToDocument = viewRect;
                IntSize scrollOffsetRelativeToDocument = frameView->scrollOffsetRelativeToDocument();
                visibleRectRelativeToDocument.setLocation(IntPoint(scrollOffsetRelativeToDocument.width(), scrollOffsetRelativeToDocument.height()));

                LayoutRect r = getRectToExpose(viewRect, visibleRectRelativeToDocument, rect, alignX, alignY);
                
                frameView->setScrollPosition(roundedIntPoint(r.location()));

                // This is the outermost view of a web page, so after scrolling this view we
                // scroll its container by calling Page::scrollRectIntoView.
                // This only has an effect on the Mac platform in applications
                // that put web views into scrolling containers, such as Mac OS X Mail.
                // The canAutoscroll function in EventHandler also knows about this.
                if (Frame* frame = frameView->frame()) {
                    if (Page* page = frame->page())
                        page->chrome().scrollRectIntoView(pixelSnappedIntRect(rect));
                }
            }
        }
    }
    
    if (parentLayer)
        parentLayer->scrollRectToVisible(newRect, alignX, alignY);

    if (frameView)
        frameView->resumeScheduledEvents();
}

void RenderLayer::updateCompositingLayersAfterScroll()
{
#if USE(ACCELERATED_COMPOSITING)
    if (compositor()->inCompositingMode()) {
        // Our stacking container is guaranteed to contain all of our descendants that may need
        // repositioning, so update compositing layers from there.
        if (RenderLayer* compositingAncestor = stackingContainer()->enclosingCompositingLayer()) {
            if (usesCompositedScrolling() && !hasOutOfFlowPositionedDescendant())
                compositor()->updateCompositingLayers(CompositingUpdateOnCompositedScroll, compositingAncestor);
            else
                compositor()->updateCompositingLayers(CompositingUpdateOnScroll, compositingAncestor);
        }
    }
#endif
}

LayoutRect RenderLayer::getRectToExpose(const LayoutRect &visibleRect, const LayoutRect &visibleRectRelativeToDocument, const LayoutRect &exposeRect, const ScrollAlignment& alignX, const ScrollAlignment& alignY)
{
    // Determine the appropriate X behavior.
    ScrollBehavior scrollX;
    LayoutRect exposeRectX(exposeRect.x(), visibleRect.y(), exposeRect.width(), visibleRect.height());
    LayoutUnit intersectWidth = intersection(visibleRect, exposeRectX).width();
    if (intersectWidth == exposeRect.width() || intersectWidth >= MIN_INTERSECT_FOR_REVEAL)
        // If the rectangle is fully visible, use the specified visible behavior.
        // If the rectangle is partially visible, but over a certain threshold,
        // then treat it as fully visible to avoid unnecessary horizontal scrolling
        scrollX = ScrollAlignment::getVisibleBehavior(alignX);
    else if (intersectWidth == visibleRect.width()) {
        // If the rect is bigger than the visible area, don't bother trying to center. Other alignments will work.
        scrollX = ScrollAlignment::getVisibleBehavior(alignX);
        if (scrollX == alignCenter)
            scrollX = noScroll;
    } else if (intersectWidth > 0)
        // If the rectangle is partially visible, but not above the minimum threshold, use the specified partial behavior
        scrollX = ScrollAlignment::getPartialBehavior(alignX);
    else
        scrollX = ScrollAlignment::getHiddenBehavior(alignX);
    // If we're trying to align to the closest edge, and the exposeRect is further right
    // than the visibleRect, and not bigger than the visible area, then align with the right.
    if (scrollX == alignToClosestEdge && exposeRect.maxX() > visibleRect.maxX() && exposeRect.width() < visibleRect.width())
        scrollX = alignRight;

    // Given the X behavior, compute the X coordinate.
    LayoutUnit x;
    if (scrollX == noScroll) 
        x = visibleRect.x();
    else if (scrollX == alignRight)
        x = exposeRect.maxX() - visibleRect.width();
    else if (scrollX == alignCenter)
        x = exposeRect.x() + (exposeRect.width() - visibleRect.width()) / 2;
    else
        x = exposeRect.x();

    // Determine the appropriate Y behavior.
    ScrollBehavior scrollY;
    LayoutRect exposeRectY(visibleRect.x(), exposeRect.y(), visibleRect.width(), exposeRect.height());
    LayoutUnit intersectHeight = intersection(visibleRectRelativeToDocument, exposeRectY).height();
    if (intersectHeight == exposeRect.height())
        // If the rectangle is fully visible, use the specified visible behavior.
        scrollY = ScrollAlignment::getVisibleBehavior(alignY);
    else if (intersectHeight == visibleRect.height()) {
        // If the rect is bigger than the visible area, don't bother trying to center. Other alignments will work.
        scrollY = ScrollAlignment::getVisibleBehavior(alignY);
        if (scrollY == alignCenter)
            scrollY = noScroll;
    } else if (intersectHeight > 0)
        // If the rectangle is partially visible, use the specified partial behavior
        scrollY = ScrollAlignment::getPartialBehavior(alignY);
    else
        scrollY = ScrollAlignment::getHiddenBehavior(alignY);
    // If we're trying to align to the closest edge, and the exposeRect is further down
    // than the visibleRect, and not bigger than the visible area, then align with the bottom.
    if (scrollY == alignToClosestEdge && exposeRect.maxY() > visibleRect.maxY() && exposeRect.height() < visibleRect.height())
        scrollY = alignBottom;

    // Given the Y behavior, compute the Y coordinate.
    LayoutUnit y;
    if (scrollY == noScroll) 
        y = visibleRect.y();
    else if (scrollY == alignBottom)
        y = exposeRect.maxY() - visibleRect.height();
    else if (scrollY == alignCenter)
        y = exposeRect.y() + (exposeRect.height() - visibleRect.height()) / 2;
    else
        y = exposeRect.y();

    return LayoutRect(LayoutPoint(x, y), visibleRect.size());
}

void RenderLayer::autoscroll(const IntPoint& position)
{
    Frame* frame = renderer()->frame();
    if (!frame)
        return;

    FrameView* frameView = frame->view();
    if (!frameView)
        return;

    IntPoint currentDocumentPosition = frameView->windowToContents(position);
    scrollRectToVisible(LayoutRect(currentDocumentPosition, LayoutSize(1, 1)), ScrollAlignment::alignToEdgeIfNeeded, ScrollAlignment::alignToEdgeIfNeeded);
}

bool RenderLayer::canResize() const
{
    if (!renderer())
        return false;
    // We need a special case for <iframe> because they never have
    // hasOverflowClip(). However, they do "implicitly" clip their contents, so
    // we want to allow resizing them also.
    return (renderer()->hasOverflowClip() || renderer()->isRenderIFrame()) && renderer()->style()->resize() != RESIZE_NONE;
}

void RenderLayer::resize(const PlatformMouseEvent& evt, const LayoutSize& oldOffset)
{
    // FIXME: This should be possible on generated content but is not right now.
    if (!inResizeMode() || !canResize() || !renderer()->node())
        return;

    ASSERT(renderer()->node()->isElementNode());
    Element* element = toElement(renderer()->node());
    RenderBox* renderer = toRenderBox(element->renderer());

    Document* document = element->document();
    if (!document->frame()->eventHandler()->mousePressed())
        return;

    float zoomFactor = renderer->style()->effectiveZoom();

    LayoutSize newOffset = offsetFromResizeCorner(document->view()->windowToContents(evt.position()));
    newOffset.setWidth(newOffset.width() / zoomFactor);
    newOffset.setHeight(newOffset.height() / zoomFactor);
    
    LayoutSize currentSize = LayoutSize(renderer->width() / zoomFactor, renderer->height() / zoomFactor);
    LayoutSize minimumSize = element->minimumSizeForResizing().shrunkTo(currentSize);
    element->setMinimumSizeForResizing(minimumSize);
    
    LayoutSize adjustedOldOffset = LayoutSize(oldOffset.width() / zoomFactor, oldOffset.height() / zoomFactor);
    if (renderer->style()->shouldPlaceBlockDirectionScrollbarOnLogicalLeft()) {
        newOffset.setWidth(-newOffset.width());
        adjustedOldOffset.setWidth(-adjustedOldOffset.width());
    }
    
    LayoutSize difference = (currentSize + newOffset - adjustedOldOffset).expandedTo(minimumSize) - currentSize;

    ASSERT_WITH_SECURITY_IMPLICATION(element->isStyledElement());
    StyledElement* styledElement = static_cast<StyledElement*>(element);
    bool isBoxSizingBorder = renderer->style()->boxSizing() == BORDER_BOX;

    EResize resize = renderer->style()->resize();
    if (resize != RESIZE_VERTICAL && difference.width()) {
        if (element->isFormControlElement()) {
            // Make implicit margins from the theme explicit (see <http://bugs.webkit.org/show_bug.cgi?id=9547>).
            styledElement->setInlineStyleProperty(CSSPropertyMarginLeft, String::number(renderer->marginLeft() / zoomFactor) + "px", false);
            styledElement->setInlineStyleProperty(CSSPropertyMarginRight, String::number(renderer->marginRight() / zoomFactor) + "px", false);
        }
        LayoutUnit baseWidth = renderer->width() - (isBoxSizingBorder ? LayoutUnit() : renderer->borderAndPaddingWidth());
        baseWidth = baseWidth / zoomFactor;
        styledElement->setInlineStyleProperty(CSSPropertyWidth, String::number(roundToInt(baseWidth + difference.width())) + "px", false);
    }

    if (resize != RESIZE_HORIZONTAL && difference.height()) {
        if (element->isFormControlElement()) {
            // Make implicit margins from the theme explicit (see <http://bugs.webkit.org/show_bug.cgi?id=9547>).
            styledElement->setInlineStyleProperty(CSSPropertyMarginTop, String::number(renderer->marginTop() / zoomFactor) + "px", false);
            styledElement->setInlineStyleProperty(CSSPropertyMarginBottom, String::number(renderer->marginBottom() / zoomFactor) + "px", false);
        }
        LayoutUnit baseHeight = renderer->height() - (isBoxSizingBorder ? LayoutUnit() : renderer->borderAndPaddingHeight());
        baseHeight = baseHeight / zoomFactor;
        styledElement->setInlineStyleProperty(CSSPropertyHeight, String::number(roundToInt(baseHeight + difference.height())) + "px", false);
    }

    document->updateLayout();

    // FIXME (Radar 4118564): We should also autoscroll the window as necessary to keep the point under the cursor in view.
}

int RenderLayer::scrollSize(ScrollbarOrientation orientation) const
{
    Scrollbar* scrollbar = ((orientation == HorizontalScrollbar) ? m_hBar : m_vBar).get();
    return scrollbar ? (scrollbar->totalSize() - scrollbar->visibleSize()) : 0;
}

void RenderLayer::setScrollOffset(const IntPoint& offset)
{
    scrollTo(offset.x(), offset.y());
}

int RenderLayer::scrollPosition(Scrollbar* scrollbar) const
{
    if (scrollbar->orientation() == HorizontalScrollbar)
        return scrollXOffset();
    if (scrollbar->orientation() == VerticalScrollbar)
        return scrollYOffset();
    return 0;
}

IntPoint RenderLayer::scrollPosition() const
{
    return IntPoint(m_scrollOffset);
}

IntPoint RenderLayer::minimumScrollPosition() const
{
    return -scrollOrigin();
}

IntPoint RenderLayer::maximumScrollPosition() const
{
    // FIXME: m_scrollSize may not be up-to-date if m_scrollDimensionsDirty is true.
    return -scrollOrigin() + roundedIntSize(m_scrollSize) - visibleContentRect(IncludeScrollbars).size();
}

IntRect RenderLayer::visibleContentRect(VisibleContentRectIncludesScrollbars scrollbarInclusion) const
{
    int verticalScrollbarWidth = 0;
    int horizontalScrollbarHeight = 0;
    if (scrollbarInclusion == IncludeScrollbars) {
        verticalScrollbarWidth = (verticalScrollbar() && !verticalScrollbar()->isOverlayScrollbar()) ? verticalScrollbar()->width() : 0;
        horizontalScrollbarHeight = (horizontalScrollbar() && !horizontalScrollbar()->isOverlayScrollbar()) ? horizontalScrollbar()->height() : 0;
    }
    
    return IntRect(IntPoint(scrollXOffset(), scrollYOffset()),
                   IntSize(max(0, m_layerSize.width() - verticalScrollbarWidth), 
                           max(0, m_layerSize.height() - horizontalScrollbarHeight)));
}

IntSize RenderLayer::overhangAmount() const
{
    return IntSize();
}

bool RenderLayer::isActive() const
{
    Page* page = renderer()->frame()->page();
    return page && page->focusController()->isActive();
}

static int cornerStart(const RenderLayer* layer, int minX, int maxX, int thickness)
{
    if (layer->renderer()->style()->shouldPlaceBlockDirectionScrollbarOnLogicalLeft())
        return minX + layer->renderer()->style()->borderLeftWidth();
    return maxX - thickness - layer->renderer()->style()->borderRightWidth();
}

static IntRect cornerRect(const RenderLayer* layer, const IntRect& bounds)
{
    int horizontalThickness;
    int verticalThickness;
    if (!layer->verticalScrollbar() && !layer->horizontalScrollbar()) {
        // FIXME: This isn't right.  We need to know the thickness of custom scrollbars
        // even when they don't exist in order to set the resizer square size properly.
        horizontalThickness = ScrollbarTheme::theme()->scrollbarThickness();
        verticalThickness = horizontalThickness;
    } else if (layer->verticalScrollbar() && !layer->horizontalScrollbar()) {
        horizontalThickness = layer->verticalScrollbar()->width();
        verticalThickness = horizontalThickness;
    } else if (layer->horizontalScrollbar() && !layer->verticalScrollbar()) {
        verticalThickness = layer->horizontalScrollbar()->height();
        horizontalThickness = verticalThickness;
    } else {
        horizontalThickness = layer->verticalScrollbar()->width();
        verticalThickness = layer->horizontalScrollbar()->height();
    }
    return IntRect(cornerStart(layer, bounds.x(), bounds.maxX(), horizontalThickness),
                   bounds.maxY() - verticalThickness - layer->renderer()->style()->borderBottomWidth(),
                   horizontalThickness, verticalThickness);
}

IntRect RenderLayer::scrollCornerRect() const
{
    // We have a scrollbar corner when a scrollbar is visible and not filling the entire length of the box.
    // This happens when:
    // (a) A resizer is present and at least one scrollbar is present
    // (b) Both scrollbars are present.
    bool hasHorizontalBar = horizontalScrollbar();
    bool hasVerticalBar = verticalScrollbar();
    bool hasResizer = renderer()->style()->resize() != RESIZE_NONE;
    if ((hasHorizontalBar && hasVerticalBar) || (hasResizer && (hasHorizontalBar || hasVerticalBar)))
        return cornerRect(this, renderBox()->pixelSnappedBorderBoxRect());
    return IntRect();
}

static IntRect resizerCornerRect(const RenderLayer* layer, const IntRect& bounds)
{
    ASSERT(layer->renderer()->isBox());
    if (layer->renderer()->style()->resize() == RESIZE_NONE)
        return IntRect();
    return cornerRect(layer, bounds);
}

IntRect RenderLayer::scrollCornerAndResizerRect() const
{
    RenderBox* box = renderBox();
    if (!box)
        return IntRect();
    IntRect scrollCornerAndResizer = scrollCornerRect();
    if (scrollCornerAndResizer.isEmpty())
        scrollCornerAndResizer = resizerCornerRect(this, box->pixelSnappedBorderBoxRect());
    return scrollCornerAndResizer;
}

bool RenderLayer::isScrollCornerVisible() const
{
    ASSERT(renderer()->isBox());
    return !scrollCornerRect().isEmpty();
}

IntRect RenderLayer::convertFromScrollbarToContainingView(const Scrollbar* scrollbar, const IntRect& scrollbarRect) const
{
    RenderView* view = renderer()->view();
    if (!view)
        return scrollbarRect;

    IntRect rect = scrollbarRect;
    rect.move(scrollbarOffset(scrollbar));

    return view->frameView()->convertFromRenderer(renderer(), rect);
}

IntRect RenderLayer::convertFromContainingViewToScrollbar(const Scrollbar* scrollbar, const IntRect& parentRect) const
{
    RenderView* view = renderer()->view();
    if (!view)
        return parentRect;

    IntRect rect = view->frameView()->convertToRenderer(renderer(), parentRect);
    rect.move(-scrollbarOffset(scrollbar));
    return rect;
}

IntPoint RenderLayer::convertFromScrollbarToContainingView(const Scrollbar* scrollbar, const IntPoint& scrollbarPoint) const
{
    RenderView* view = renderer()->view();
    if (!view)
        return scrollbarPoint;

    IntPoint point = scrollbarPoint;
    point.move(scrollbarOffset(scrollbar));
    return view->frameView()->convertFromRenderer(renderer(), point);
}

IntPoint RenderLayer::convertFromContainingViewToScrollbar(const Scrollbar* scrollbar, const IntPoint& parentPoint) const
{
    RenderView* view = renderer()->view();
    if (!view)
        return parentPoint;

    IntPoint point = view->frameView()->convertToRenderer(renderer(), parentPoint);

    point.move(-scrollbarOffset(scrollbar));
    return point;
}

IntSize RenderLayer::contentsSize() const
{
    return IntSize(scrollWidth(), scrollHeight());
}

int RenderLayer::visibleHeight() const
{
    return m_layerSize.height();
}

int RenderLayer::visibleWidth() const
{
    return m_layerSize.width();
}

bool RenderLayer::shouldSuspendScrollAnimations() const
{
    RenderView* view = renderer()->view();
    if (!view)
        return true;
    return view->frameView()->shouldSuspendScrollAnimations();
}

bool RenderLayer::scrollbarsCanBeActive() const
{
    RenderView* view = renderer()->view();
    if (!view)
        return false;
    return view->frameView()->scrollbarsCanBeActive();
}

IntPoint RenderLayer::lastKnownMousePosition() const
{
    return renderer()->frame() ? renderer()->frame()->eventHandler()->lastKnownMousePosition() : IntPoint();
}

bool RenderLayer::isHandlingWheelEvent() const
{
    return renderer()->frame() ? renderer()->frame()->eventHandler()->isHandlingWheelEvent() : false;
}

IntRect RenderLayer::rectForHorizontalScrollbar(const IntRect& borderBoxRect) const
{
    if (!m_hBar)
        return IntRect();

    const RenderBox* box = renderBox();
    const IntRect& scrollCorner = scrollCornerRect();

    return IntRect(horizontalScrollbarStart(borderBoxRect.x()),
        borderBoxRect.maxY() - box->borderBottom() - m_hBar->height(),
        borderBoxRect.width() - (box->borderLeft() + box->borderRight()) - scrollCorner.width(),
        m_hBar->height());
}

IntRect RenderLayer::rectForVerticalScrollbar(const IntRect& borderBoxRect) const
{
    if (!m_vBar)
        return IntRect();

    const RenderBox* box = renderBox();
    const IntRect& scrollCorner = scrollCornerRect();

    return IntRect(verticalScrollbarStart(borderBoxRect.x(), borderBoxRect.maxX()),
        borderBoxRect.y() + box->borderTop(),
        m_vBar->width(),
        borderBoxRect.height() - (box->borderTop() + box->borderBottom()) - scrollCorner.height());
}

LayoutUnit RenderLayer::verticalScrollbarStart(int minX, int maxX) const
{
    const RenderBox* box = renderBox();
    if (renderer()->style()->shouldPlaceBlockDirectionScrollbarOnLogicalLeft())
        return minX + box->borderLeft();
    return maxX - box->borderRight() - m_vBar->width();
}

LayoutUnit RenderLayer::horizontalScrollbarStart(int minX) const
{
    const RenderBox* box = renderBox();
    int x = minX + box->borderLeft();
    if (renderer()->style()->shouldPlaceBlockDirectionScrollbarOnLogicalLeft())
        x += m_vBar ? m_vBar->width() : resizerCornerRect(this, box->pixelSnappedBorderBoxRect()).width();
    return x;
}

IntSize RenderLayer::scrollbarOffset(const Scrollbar* scrollbar) const
{
    RenderBox* box = renderBox();

    if (scrollbar == m_vBar.get())
        return IntSize(verticalScrollbarStart(0, box->width()), box->borderTop());

    if (scrollbar == m_hBar.get())
        return IntSize(horizontalScrollbarStart(0), box->height() - box->borderBottom() - scrollbar->height());
    
    ASSERT_NOT_REACHED();
    return IntSize();
}

void RenderLayer::invalidateScrollbarRect(Scrollbar* scrollbar, const IntRect& rect)
{
#if USE(ACCELERATED_COMPOSITING)
    if (scrollbar == m_vBar.get()) {
        if (GraphicsLayer* layer = layerForVerticalScrollbar()) {
            layer->setNeedsDisplayInRect(rect);
            return;
        }
    } else {
        if (GraphicsLayer* layer = layerForHorizontalScrollbar()) {
            layer->setNeedsDisplayInRect(rect);
            return;
        }
    }
#endif
    IntRect scrollRect = rect;
    RenderBox* box = renderBox();
    ASSERT(box);
    // If we are not yet inserted into the tree, there is no need to repaint.
    if (!box->parent())
        return;

    if (scrollbar == m_vBar.get())
        scrollRect.move(verticalScrollbarStart(0, box->width()), box->borderTop());
    else
        scrollRect.move(horizontalScrollbarStart(0), box->height() - box->borderBottom() - scrollbar->height());
    LayoutRect repaintRect = scrollRect;
    renderBox()->flipForWritingMode(repaintRect);
    renderer()->repaintRectangle(repaintRect);
}

void RenderLayer::invalidateScrollCornerRect(const IntRect& rect)
{
#if USE(ACCELERATED_COMPOSITING)
    if (GraphicsLayer* layer = layerForScrollCorner()) {
        layer->setNeedsDisplayInRect(rect);
        return;
    }
#endif
    if (m_scrollCorner)
        m_scrollCorner->repaintRectangle(rect);
    if (m_resizer)
        m_resizer->repaintRectangle(rect);
}

static inline RenderObject* rendererForScrollbar(RenderObject* renderer)
{
    if (Node* node = renderer->node()) {
        if (ShadowRoot* shadowRoot = node->containingShadowRoot()) {
            if (shadowRoot->type() == ShadowRoot::UserAgentShadowRoot)
                return shadowRoot->host()->renderer();
        }
    }

    return renderer;
}

PassRefPtr<Scrollbar> RenderLayer::createScrollbar(ScrollbarOrientation orientation)
{
    RefPtr<Scrollbar> widget;
    RenderObject* actualRenderer = rendererForScrollbar(renderer());
    bool hasCustomScrollbarStyle = actualRenderer->isBox() && actualRenderer->style()->hasPseudoStyle(SCROLLBAR);
    if (hasCustomScrollbarStyle)
        widget = RenderScrollbar::createCustomScrollbar(this, orientation, actualRenderer->node());
    else {
        widget = Scrollbar::createNativeScrollbar(this, orientation, RegularScrollbar);
        didAddScrollbar(widget.get(), orientation);
    }
    renderer()->document()->view()->addChild(widget.get());        
    return widget.release();
}

void RenderLayer::destroyScrollbar(ScrollbarOrientation orientation)
{
    RefPtr<Scrollbar>& scrollbar = orientation == HorizontalScrollbar ? m_hBar : m_vBar;
    if (!scrollbar)
        return;

    if (!scrollbar->isCustomScrollbar())
        willRemoveScrollbar(scrollbar.get(), orientation);

    scrollbar->removeFromParent();
    scrollbar->disconnectFromScrollableArea();
    scrollbar = 0;
}

bool RenderLayer::scrollsOverflow() const
{
    if (!renderer()->isBox())
        return false;

    return toRenderBox(renderer())->scrollsOverflow();
}

void RenderLayer::setHasHorizontalScrollbar(bool hasScrollbar)
{
    if (hasScrollbar == hasHorizontalScrollbar())
        return;

    if (hasScrollbar)
        m_hBar = createScrollbar(HorizontalScrollbar);
    else
        destroyScrollbar(HorizontalScrollbar);

    // Destroying or creating one bar can cause our scrollbar corner to come and go.  We need to update the opposite scrollbar's style.
    if (m_hBar)
        m_hBar->styleChanged();
    if (m_vBar)
        m_vBar->styleChanged();

    // Force an update since we know the scrollbars have changed things.
#if ENABLE(DASHBOARD_SUPPORT) || ENABLE(DRAGGABLE_REGION)
    if (renderer()->document()->hasAnnotatedRegions())
        renderer()->document()->setAnnotatedRegionsDirty(true);
#endif
}

void RenderLayer::setHasVerticalScrollbar(bool hasScrollbar)
{
    if (hasScrollbar == hasVerticalScrollbar())
        return;

    if (hasScrollbar)
        m_vBar = createScrollbar(VerticalScrollbar);
    else
        destroyScrollbar(VerticalScrollbar);

     // Destroying or creating one bar can cause our scrollbar corner to come and go.  We need to update the opposite scrollbar's style.
    if (m_hBar)
        m_hBar->styleChanged();
    if (m_vBar)
        m_vBar->styleChanged();

    // Force an update since we know the scrollbars have changed things.
#if ENABLE(DASHBOARD_SUPPORT) || ENABLE(DRAGGABLE_REGION)
    if (renderer()->document()->hasAnnotatedRegions())
        renderer()->document()->setAnnotatedRegionsDirty(true);
#endif
}

ScrollableArea* RenderLayer::enclosingScrollableArea() const
{
    if (RenderLayer* scrollableLayer = enclosingScrollableLayer())
        return scrollableLayer;

    // FIXME: We should return the frame view here (or possibly an ancestor frame view,
    // if the frame view isn't scrollable.
    return 0;
}

int RenderLayer::verticalScrollbarWidth(OverlayScrollbarSizeRelevancy relevancy) const
{
    if (!m_vBar || (m_vBar->isOverlayScrollbar() && (relevancy == IgnoreOverlayScrollbarSize || !m_vBar->shouldParticipateInHitTesting())))
        return 0;
    return m_vBar->width();
}

int RenderLayer::horizontalScrollbarHeight(OverlayScrollbarSizeRelevancy relevancy) const
{
    if (!m_hBar || (m_hBar->isOverlayScrollbar() && (relevancy == IgnoreOverlayScrollbarSize || !m_hBar->shouldParticipateInHitTesting())))
        return 0;
    return m_hBar->height();
}

IntSize RenderLayer::offsetFromResizeCorner(const IntPoint& absolutePoint) const
{
    // Currently the resize corner is either the bottom right corner or the bottom left corner.
    // FIXME: This assumes the location is 0, 0. Is this guaranteed to always be the case?
    IntSize elementSize = size();
    if (renderer()->style()->shouldPlaceBlockDirectionScrollbarOnLogicalLeft())
        elementSize.setWidth(0);
    IntPoint resizerPoint = IntPoint(elementSize);
    IntPoint localPoint = roundedIntPoint(absoluteToContents(absolutePoint));
    return localPoint - resizerPoint;
}

bool RenderLayer::hasOverflowControls() const
{
    return m_hBar || m_vBar || m_scrollCorner || renderer()->style()->resize() != RESIZE_NONE;
}

void RenderLayer::positionOverflowControls(const IntSize& offsetFromRoot)
{
    if (!m_hBar && !m_vBar && !canResize())
        return;
    
    RenderBox* box = renderBox();
    if (!box)
        return;

    const IntRect borderBox = box->pixelSnappedBorderBoxRect();
    const IntRect& scrollCorner = scrollCornerRect();
    IntRect absBounds(borderBox.location() + offsetFromRoot, borderBox.size());
    if (m_vBar) {
        IntRect vBarRect = rectForVerticalScrollbar(borderBox);
        vBarRect.move(offsetFromRoot);
        m_vBar->setFrameRect(vBarRect);
    }
    
    if (m_hBar) {
        IntRect hBarRect = rectForHorizontalScrollbar(borderBox);
        hBarRect.move(offsetFromRoot);
        m_hBar->setFrameRect(hBarRect);
    }
    
    if (m_scrollCorner)
        m_scrollCorner->setFrameRect(scrollCorner);
    if (m_resizer)
        m_resizer->setFrameRect(resizerCornerRect(this, borderBox));

#if USE(ACCELERATED_COMPOSITING)    
    if (isComposited())
        backing()->positionOverflowControlsLayers(offsetFromRoot);
#endif
}

int RenderLayer::scrollWidth() const
{
    ASSERT(renderBox());
    if (m_scrollDimensionsDirty)
        const_cast<RenderLayer*>(this)->computeScrollDimensions();
    return snapSizeToPixel(m_scrollSize.width(), renderBox()->clientLeft() + renderBox()->x());
}

int RenderLayer::scrollHeight() const
{
    ASSERT(renderBox());
    if (m_scrollDimensionsDirty)
        const_cast<RenderLayer*>(this)->computeScrollDimensions();
    return snapSizeToPixel(m_scrollSize.height(), renderBox()->clientTop() + renderBox()->y());
}

LayoutUnit RenderLayer::overflowTop() const
{
    RenderBox* box = renderBox();
    LayoutRect overflowRect(box->layoutOverflowRect());
    box->flipForWritingMode(overflowRect);
    return overflowRect.y();
}

LayoutUnit RenderLayer::overflowBottom() const
{
    RenderBox* box = renderBox();
    LayoutRect overflowRect(box->layoutOverflowRect());
    box->flipForWritingMode(overflowRect);
    return overflowRect.maxY();
}

LayoutUnit RenderLayer::overflowLeft() const
{
    RenderBox* box = renderBox();
    LayoutRect overflowRect(box->layoutOverflowRect());
    box->flipForWritingMode(overflowRect);
    return overflowRect.x();
}

LayoutUnit RenderLayer::overflowRight() const
{
    RenderBox* box = renderBox();
    LayoutRect overflowRect(box->layoutOverflowRect());
    box->flipForWritingMode(overflowRect);
    return overflowRect.maxX();
}

void RenderLayer::computeScrollDimensions()
{
    RenderBox* box = renderBox();
    ASSERT(box);

    m_scrollDimensionsDirty = false;

    m_scrollSize.setWidth(overflowRight() - overflowLeft());
    m_scrollSize.setHeight(overflowBottom() - overflowTop());

    int scrollableLeftOverflow = overflowLeft() - box->borderLeft();
    int scrollableTopOverflow = overflowTop() - box->borderTop();
    setScrollOrigin(IntPoint(-scrollableLeftOverflow, -scrollableTopOverflow));
}

bool RenderLayer::hasScrollableHorizontalOverflow() const
{
    return hasHorizontalOverflow() && renderBox()->scrollsOverflowX();
}

bool RenderLayer::hasScrollableVerticalOverflow() const
{
    return hasVerticalOverflow() && renderBox()->scrollsOverflowY();
}

bool RenderLayer::hasHorizontalOverflow() const
{
    ASSERT(!m_scrollDimensionsDirty);

    return scrollWidth() > renderBox()->pixelSnappedClientWidth();
}

bool RenderLayer::hasVerticalOverflow() const
{
    ASSERT(!m_scrollDimensionsDirty);

    return scrollHeight() > renderBox()->pixelSnappedClientHeight();
}

void RenderLayer::updateScrollbarsAfterLayout()
{
    RenderBox* box = renderBox();
    ASSERT(box);

    // List box parts handle the scrollbars by themselves so we have nothing to do.
    if (box->style()->appearance() == ListboxPart)
        return;

    bool hasHorizontalOverflow = this->hasHorizontalOverflow();
    bool hasVerticalOverflow = this->hasVerticalOverflow();

    // overflow:scroll should just enable/disable.
    if (renderer()->style()->overflowX() == OSCROLL)
        m_hBar->setEnabled(hasHorizontalOverflow);
    if (renderer()->style()->overflowY() == OSCROLL)
        m_vBar->setEnabled(hasVerticalOverflow);

    // overflow:auto may need to lay out again if scrollbars got added/removed.
    bool autoHorizontalScrollBarChanged = box->hasAutoHorizontalScrollbar() && (hasHorizontalScrollbar() != hasHorizontalOverflow);
    bool autoVerticalScrollBarChanged = box->hasAutoVerticalScrollbar() && (hasVerticalScrollbar() != hasVerticalOverflow);

    if (autoHorizontalScrollBarChanged || autoVerticalScrollBarChanged) {
        if (box->hasAutoHorizontalScrollbar())
            setHasHorizontalScrollbar(hasHorizontalOverflow);
        if (box->hasAutoVerticalScrollbar())
            setHasVerticalScrollbar(hasVerticalOverflow);

        updateSelfPaintingLayer();

        // Force an update since we know the scrollbars have changed things.
#if ENABLE(DASHBOARD_SUPPORT) || ENABLE(DRAGGABLE_REGION)
        if (renderer()->document()->hasAnnotatedRegions())
            renderer()->document()->setAnnotatedRegionsDirty(true);
#endif

        renderer()->repaint();

        if (renderer()->style()->overflowX() == OAUTO || renderer()->style()->overflowY() == OAUTO) {
            if (!m_inOverflowRelayout) {
                // Our proprietary overflow: overlay value doesn't trigger a layout.
                m_inOverflowRelayout = true;
                renderer()->setNeedsLayout(true, MarkOnlyThis);
                if (renderer()->isRenderBlock()) {
                    RenderBlock* block = toRenderBlock(renderer());
                    block->scrollbarsChanged(autoHorizontalScrollBarChanged, autoVerticalScrollBarChanged);
                    block->layoutBlock(true);
                } else
                    renderer()->layout();
                m_inOverflowRelayout = false;
            }
        }
    }

    // Set up the range (and page step/line step).
    if (m_hBar) {
        int clientWidth = box->pixelSnappedClientWidth();
        int pageStep = max(max<int>(clientWidth * Scrollbar::minFractionToStepWhenPaging(), clientWidth - Scrollbar::maxOverlapBetweenPages()), 1);
        m_hBar->setSteps(Scrollbar::pixelsPerLineStep(), pageStep);
        m_hBar->setProportion(clientWidth, m_scrollSize.width());
    }
    if (m_vBar) {
        int clientHeight = box->pixelSnappedClientHeight();
        int pageStep = max(max<int>(clientHeight * Scrollbar::minFractionToStepWhenPaging(), clientHeight - Scrollbar::maxOverlapBetweenPages()), 1);
        m_vBar->setSteps(Scrollbar::pixelsPerLineStep(), pageStep);
        m_vBar->setProportion(clientHeight, m_scrollSize.height());
    }

    updateScrollableAreaSet(hasScrollableHorizontalOverflow() || hasScrollableVerticalOverflow());
}

void RenderLayer::updateScrollInfoAfterLayout()
{
    RenderBox* box = renderBox();
    if (!box)
        return;

    m_scrollDimensionsDirty = true;
    IntSize originalScrollOffset = scrollOffset();

    computeScrollDimensions();

    if (box->style()->overflowX() != OMARQUEE) {
        // Layout may cause us to be at an invalid scroll position. In this case we need
        // to pull our scroll offsets back to the max (or push them up to the min).
        IntSize clampedScrollOffset = clampScrollOffset(scrollOffset());
        if (clampedScrollOffset != scrollOffset())
            scrollToOffset(clampedScrollOffset);
    }

    updateScrollbarsAfterLayout();

    if (originalScrollOffset != scrollOffset())
        scrollToOffsetWithoutAnimation(IntPoint(scrollOffset()));

#if USE(ACCELERATED_COMPOSITING)
    // Composited scrolling may need to be enabled or disabled if the amount of overflow changed.
    if (renderer()->view() && compositor()->updateLayerCompositingState(this))
        compositor()->setCompositingLayersNeedRebuild();
#endif
}

bool RenderLayer::overflowControlsIntersectRect(const IntRect& localRect) const
{
    const IntRect borderBox = renderBox()->pixelSnappedBorderBoxRect();

    if (rectForHorizontalScrollbar(borderBox).intersects(localRect))
        return true;

    if (rectForVerticalScrollbar(borderBox).intersects(localRect))
        return true;

    if (scrollCornerRect().intersects(localRect))
        return true;
    
    if (resizerCornerRect(this, borderBox).intersects(localRect))
        return true;

    return false;
}

void RenderLayer::paintOverflowControls(GraphicsContext* context, const IntPoint& paintOffset, const IntRect& damageRect, bool paintingOverlayControls)
{
    // Don't do anything if we have no overflow.
    if (!renderer()->hasOverflowClip())
        return;

    // Overlay scrollbars paint in a second pass through the layer tree so that they will paint
    // on top of everything else. If this is the normal painting pass, paintingOverlayControls
    // will be false, and we should just tell the root layer that there are overlay scrollbars
    // that need to be painted. That will cause the second pass through the layer tree to run,
    // and we'll paint the scrollbars then. In the meantime, cache tx and ty so that the 
    // second pass doesn't need to re-enter the RenderTree to get it right.
    if (hasOverlayScrollbars() && !paintingOverlayControls) {
        m_cachedOverlayScrollbarOffset = paintOffset;
#if USE(ACCELERATED_COMPOSITING)
        // It's not necessary to do the second pass if the scrollbars paint into layers.
        if ((m_hBar && layerForHorizontalScrollbar()) || (m_vBar && layerForVerticalScrollbar()))
            return;
#endif
        IntRect localDamgeRect = damageRect;
        localDamgeRect.moveBy(-paintOffset);
        if (!overflowControlsIntersectRect(localDamgeRect))
            return;

        RenderView* renderView = renderer()->view();

        RenderLayer* paintingRoot = 0;
#if USE(ACCELERATED_COMPOSITING)
        paintingRoot = enclosingCompositingLayer();
#endif
        if (!paintingRoot)
            paintingRoot = renderView->layer();

        paintingRoot->setContainsDirtyOverlayScrollbars(true);
        return;
    }

    // This check is required to avoid painting custom CSS scrollbars twice.
    if (paintingOverlayControls && !hasOverlayScrollbars())
        return;

    IntPoint adjustedPaintOffset = paintOffset;
    if (paintingOverlayControls)
        adjustedPaintOffset = m_cachedOverlayScrollbarOffset;

    // Move the scrollbar widgets if necessary.  We normally move and resize widgets during layout, but sometimes
    // widgets can move without layout occurring (most notably when you scroll a document that
    // contains fixed positioned elements).
    positionOverflowControls(toIntSize(adjustedPaintOffset));

    // Now that we're sure the scrollbars are in the right place, paint them.
    if (m_hBar
#if USE(ACCELERATED_COMPOSITING)
        && !layerForHorizontalScrollbar()
#endif
              )
        m_hBar->paint(context, damageRect);
    if (m_vBar
#if USE(ACCELERATED_COMPOSITING)
        && !layerForVerticalScrollbar()
#endif
              )
        m_vBar->paint(context, damageRect);

#if USE(ACCELERATED_COMPOSITING)
    if (layerForScrollCorner())
        return;
#endif

    // We fill our scroll corner with white if we have a scrollbar that doesn't run all the way up to the
    // edge of the box.
    paintScrollCorner(context, adjustedPaintOffset, damageRect);
    
    // Paint our resizer last, since it sits on top of the scroll corner.
    paintResizer(context, adjustedPaintOffset, damageRect);
}

void RenderLayer::paintScrollCorner(GraphicsContext* context, const IntPoint& paintOffset, const IntRect& damageRect)
{
    RenderBox* box = renderBox();
    ASSERT(box);

    IntRect absRect = scrollCornerRect();
    absRect.moveBy(paintOffset);
    if (!absRect.intersects(damageRect))
        return;

    if (context->updatingControlTints()) {
        updateScrollCornerStyle();
        return;
    }

    if (m_scrollCorner) {
        m_scrollCorner->paintIntoRect(context, paintOffset, absRect);
        return;
    }

    // We don't want to paint white if we have overlay scrollbars, since we need
    // to see what is behind it.
    if (!hasOverlayScrollbars())
        context->fillRect(absRect, Color::white, box->style()->colorSpace());
}

void RenderLayer::drawPlatformResizerImage(GraphicsContext* context, IntRect resizerCornerRect)
{
    float deviceScaleFactor = WebCore::deviceScaleFactor(renderer()->frame());

    RefPtr<Image> resizeCornerImage;
    IntSize cornerResizerSize;
    if (deviceScaleFactor >= 2) {
        DEFINE_STATIC_LOCAL(Image*, resizeCornerImageHiRes, (Image::loadPlatformResource("textAreaResizeCorner@2x").leakRef()));
        resizeCornerImage = resizeCornerImageHiRes;
        cornerResizerSize = resizeCornerImage->size();
        cornerResizerSize.scale(0.5f);
    } else {
        DEFINE_STATIC_LOCAL(Image*, resizeCornerImageLoRes, (Image::loadPlatformResource("textAreaResizeCorner").leakRef()));
        resizeCornerImage = resizeCornerImageLoRes;
        cornerResizerSize = resizeCornerImage->size();
    }

    if (renderer()->style()->shouldPlaceBlockDirectionScrollbarOnLogicalLeft()) {
        context->save();
        context->translate(resizerCornerRect.x() + cornerResizerSize.width(), resizerCornerRect.y() + resizerCornerRect.height() - cornerResizerSize.height());
        context->scale(FloatSize(-1.0, 1.0));
        context->drawImage(resizeCornerImage.get(), renderer()->style()->colorSpace(), IntRect(IntPoint(), cornerResizerSize));
        context->restore();
        return;
    }
    IntRect imageRect(resizerCornerRect.maxXMaxYCorner() - cornerResizerSize, cornerResizerSize);
    context->drawImage(resizeCornerImage.get(), renderer()->style()->colorSpace(), imageRect);
}

void RenderLayer::paintResizer(GraphicsContext* context, const IntPoint& paintOffset, const IntRect& damageRect)
{
    if (renderer()->style()->resize() == RESIZE_NONE)
        return;

    RenderBox* box = renderBox();
    ASSERT(box);

    IntRect absRect = resizerCornerRect(this, box->pixelSnappedBorderBoxRect());
    absRect.moveBy(paintOffset);
    if (!absRect.intersects(damageRect))
        return;

    if (context->updatingControlTints()) {
        updateResizerStyle();
        return;
    }
    
    if (m_resizer) {
        m_resizer->paintIntoRect(context, paintOffset, absRect);
        return;
    }

    drawPlatformResizerImage(context, absRect);

    // Draw a frame around the resizer (1px grey line) if there are any scrollbars present.
    // Clipping will exclude the right and bottom edges of this frame.
    if (!hasOverlayScrollbars() && (m_vBar || m_hBar)) {
        GraphicsContextStateSaver stateSaver(*context);
        context->clip(absRect);
        IntRect largerCorner = absRect;
        largerCorner.setSize(IntSize(largerCorner.width() + 1, largerCorner.height() + 1));
        context->setStrokeColor(Color(makeRGB(217, 217, 217)), ColorSpaceDeviceRGB);
        context->setStrokeThickness(1.0f);
        context->setFillColor(Color::transparent, ColorSpaceDeviceRGB);
        context->drawRect(largerCorner);
    }
}

bool RenderLayer::isPointInResizeControl(const IntPoint& absolutePoint) const
{
    if (!canResize())
        return false;
    
    RenderBox* box = renderBox();
    ASSERT(box);

    IntPoint localPoint = roundedIntPoint(absoluteToContents(absolutePoint));

    IntRect localBounds(0, 0, box->pixelSnappedWidth(), box->pixelSnappedHeight());
    return resizerCornerRect(this, localBounds).contains(localPoint);
}

bool RenderLayer::hitTestOverflowControls(HitTestResult& result, const IntPoint& localPoint)
{
    if (!m_hBar && !m_vBar && !canResize())
        return false;

    RenderBox* box = renderBox();
    ASSERT(box);
    
    IntRect resizeControlRect;
    if (renderer()->style()->resize() != RESIZE_NONE) {
        resizeControlRect = resizerCornerRect(this, box->pixelSnappedBorderBoxRect());
        if (resizeControlRect.contains(localPoint))
            return true;
    }

    int resizeControlSize = max(resizeControlRect.height(), 0);

    // FIXME: We should hit test the m_scrollCorner and pass it back through the result.

    if (m_vBar && m_vBar->shouldParticipateInHitTesting()) {
        LayoutRect vBarRect(verticalScrollbarStart(0, box->width()),
                            box->borderTop(),
                            m_vBar->width(),
                            box->height() - (box->borderTop() + box->borderBottom()) - (m_hBar ? m_hBar->height() : resizeControlSize));
        if (vBarRect.contains(localPoint)) {
            result.setScrollbar(m_vBar.get());
            return true;
        }
    }

    resizeControlSize = max(resizeControlRect.width(), 0);
    if (m_hBar && m_hBar->shouldParticipateInHitTesting()) {
        LayoutRect hBarRect(horizontalScrollbarStart(0),
                            box->height() - box->borderBottom() - m_hBar->height(),
                            box->width() - (box->borderLeft() + box->borderRight()) - (m_vBar ? m_vBar->width() : resizeControlSize),
                            m_hBar->height());
        if (hBarRect.contains(localPoint)) {
            result.setScrollbar(m_hBar.get());
            return true;
        }
    }

    return false;
}

bool RenderLayer::scroll(ScrollDirection direction, ScrollGranularity granularity, float multiplier)
{
    return ScrollableArea::scroll(direction, granularity, multiplier);
}

void RenderLayer::paint(GraphicsContext* context, const LayoutRect& damageRect, PaintBehavior paintBehavior, RenderObject* subtreePaintRoot, RenderRegion* region, PaintLayerFlags paintFlags)
{
    OverlapTestRequestMap overlapTestRequests;

    LayerPaintingInfo paintingInfo(this, enclosingIntRect(damageRect), paintBehavior, LayoutSize(), subtreePaintRoot, region, &overlapTestRequests);
    paintLayer(context, paintingInfo, paintFlags);

    OverlapTestRequestMap::iterator end = overlapTestRequests.end();
    for (OverlapTestRequestMap::iterator it = overlapTestRequests.begin(); it != end; ++it)
        it->key->setOverlapTestResult(false);
}

void RenderLayer::paintOverlayScrollbars(GraphicsContext* context, const LayoutRect& damageRect, PaintBehavior paintBehavior, RenderObject* subtreePaintRoot)
{
    if (!m_containsDirtyOverlayScrollbars)
        return;

    LayerPaintingInfo paintingInfo(this, enclosingIntRect(damageRect), paintBehavior, LayoutSize(), subtreePaintRoot);
    paintLayer(context, paintingInfo, PaintLayerPaintingOverlayScrollbars);

    m_containsDirtyOverlayScrollbars = false;
}

static bool inContainingBlockChain(RenderLayer* startLayer, RenderLayer* endLayer)
{
    if (startLayer == endLayer)
        return true;
    
    RenderView* view = startLayer->renderer()->view();
    for (RenderBlock* currentBlock = startLayer->renderer()->containingBlock(); currentBlock && currentBlock != view; currentBlock = currentBlock->containingBlock()) {
        if (currentBlock->layer() == endLayer)
            return true;
    }
    
    return false;
}

void RenderLayer::clipToRect(RenderLayer* rootLayer, GraphicsContext* context, const LayoutRect& paintDirtyRect, const ClipRect& clipRect,
                             BorderRadiusClippingRule rule)
{
    if (clipRect.rect() != paintDirtyRect) {
        context->save();
        context->clip(pixelSnappedIntRect(clipRect.rect()));
    }

    if (!clipRect.hasRadius())
        return;

    // If the clip rect has been tainted by a border radius, then we have to walk up our layer chain applying the clips from
    // any layers with overflow. The condition for being able to apply these clips is that the overflow object be in our
    // containing block chain so we check that also.
    for (RenderLayer* layer = rule == IncludeSelfForBorderRadius ? this : parent(); layer; layer = layer->parent()) {
        if (layer->renderer()->hasOverflowClip() && layer->renderer()->style()->hasBorderRadius() && inContainingBlockChain(this, layer)) {
                LayoutPoint delta;
                layer->convertToLayerCoords(rootLayer, delta);
                context->clipRoundedRect(layer->renderer()->style()->getRoundedInnerBorderFor(LayoutRect(delta, layer->size())));
        }

        if (layer == rootLayer)
            break;
    }
}

void RenderLayer::restoreClip(GraphicsContext* context, const LayoutRect& paintDirtyRect, const ClipRect& clipRect)
{
    if (clipRect.rect() == paintDirtyRect)
        return;
    context->restore();
}

static void performOverlapTests(OverlapTestRequestMap& overlapTestRequests, const RenderLayer* rootLayer, const RenderLayer* layer)
{
    Vector<OverlapTestRequestClient*> overlappedRequestClients;
    OverlapTestRequestMap::iterator end = overlapTestRequests.end();
    LayoutRect boundingBox = layer->boundingBox(rootLayer);
    for (OverlapTestRequestMap::iterator it = overlapTestRequests.begin(); it != end; ++it) {
        if (!boundingBox.intersects(it->value))
            continue;

        it->key->setOverlapTestResult(true);
        overlappedRequestClients.append(it->key);
    }
    for (size_t i = 0; i < overlappedRequestClients.size(); ++i)
        overlapTestRequests.remove(overlappedRequestClients[i]);
}

#if USE(ACCELERATED_COMPOSITING)
static bool shouldDoSoftwarePaint(const RenderLayer* layer, bool paintingReflection)
{
    return paintingReflection && !layer->has3DTransform();
}
#endif
    
static inline bool shouldSuppressPaintingLayer(RenderLayer* layer)
{
    // Avoid painting descendants of the root layer when stylesheets haven't loaded. This eliminates FOUC.
    // It's ok not to draw, because later on, when all the stylesheets do load, updateStyleSelector on the Document
    // will do a full repaint().
    if (layer->renderer()->document()->didLayoutWithPendingStylesheets() && !layer->isRootLayer() && !layer->renderer()->isRoot())
        return true;

    // Avoid painting all layers if the document is in a state where visual updates aren't allowed.
    // A full repaint will occur in Document::implicitClose() if painting is suppressed here.
    if (!layer->renderer()->document()->visualUpdatesAllowed())
        return true;

    return false;
}

#if USE(ACCELERATED_COMPOSITING)
static bool paintForFixedRootBackground(const RenderLayer* layer, RenderLayer::PaintLayerFlags paintFlags)
{
    return layer->renderer()->isRoot() && (paintFlags & RenderLayer::PaintLayerPaintingRootBackgroundOnly);
}
#endif

void RenderLayer::paintLayer(GraphicsContext* context, const LayerPaintingInfo& paintingInfo, PaintLayerFlags paintFlags)
{
#if USE(ACCELERATED_COMPOSITING)
    if (isComposited()) {
        // The updatingControlTints() painting pass goes through compositing layers,
        // but we need to ensure that we don't cache clip rects computed with the wrong root in this case.
        if (context->updatingControlTints() || (paintingInfo.paintBehavior & PaintBehaviorFlattenCompositingLayers))
            paintFlags |= PaintLayerTemporaryClipRects;
        else if (!backing()->paintsIntoWindow()
            && !backing()->paintsIntoCompositedAncestor()
            && !shouldDoSoftwarePaint(this, paintFlags & PaintLayerPaintingReflection)
            && !paintForFixedRootBackground(this, paintFlags)) {
            // If this RenderLayer should paint into its backing, that will be done via RenderLayerBacking::paintIntoLayer().
            return;
        }
    } else if (viewportConstrainedNotCompositedReason() == NotCompositedForBoundsOutOfView) {
        // Don't paint out-of-view viewport constrained layers (when doing prepainting) because they will never be visible
        // unless their position or viewport size is changed.
        ASSERT(renderer()->style()->position() == FixedPosition);
        return;
    }
#endif

    // Non self-painting leaf layers don't need to be painted as their renderer() should properly paint itself.
    if (!isSelfPaintingLayer() && !hasSelfPaintingLayerDescendant())
        return;

    if (shouldSuppressPaintingLayer(this))
        return;
    
    // If this layer is totally invisible then there is nothing to paint.
    if (!renderer()->opacity())
        return;

    if (paintsWithTransparency(paintingInfo.paintBehavior))
        paintFlags |= PaintLayerHaveTransparency;

    // PaintLayerAppliedTransform is used in RenderReplica, to avoid applying the transform twice.
    if (paintsWithTransform(paintingInfo.paintBehavior) && !(paintFlags & PaintLayerAppliedTransform)) {
        TransformationMatrix layerTransform = renderableTransform(paintingInfo.paintBehavior);
        // If the transform can't be inverted, then don't paint anything.
        if (!layerTransform.isInvertible())
            return;

        // If we have a transparency layer enclosing us and we are the root of a transform, then we need to establish the transparency
        // layer from the parent now, assuming there is a parent
        if (paintFlags & PaintLayerHaveTransparency) {
            if (parent())
                parent()->beginTransparencyLayers(context, paintingInfo.rootLayer, paintingInfo.paintDirtyRect, paintingInfo.paintBehavior);
            else
                beginTransparencyLayers(context, paintingInfo.rootLayer, paintingInfo.paintDirtyRect, paintingInfo.paintBehavior);
        }

        if (enclosingPaginationLayer()) {
            paintTransformedLayerIntoFragments(context, paintingInfo, paintFlags);
            return;
        }

        // Make sure the parent's clip rects have been calculated.
        ClipRect clipRect = paintingInfo.paintDirtyRect;
        if (parent()) {
            ClipRectsContext clipRectsContext(paintingInfo.rootLayer, paintingInfo.region, (paintFlags & PaintLayerTemporaryClipRects) ? TemporaryClipRects : PaintingClipRects,
                IgnoreOverlayScrollbarSize, (paintFlags & PaintLayerPaintingOverflowContents) ? IgnoreOverflowClip : RespectOverflowClip);
            clipRect = backgroundClipRect(clipRectsContext);
            clipRect.intersect(paintingInfo.paintDirtyRect);
        
            // Push the parent coordinate space's clip.
            parent()->clipToRect(paintingInfo.rootLayer, context, paintingInfo.paintDirtyRect, clipRect);
        }

        paintLayerByApplyingTransform(context, paintingInfo, paintFlags);

        // Restore the clip.
        if (parent())
            parent()->restoreClip(context, paintingInfo.paintDirtyRect, clipRect);

        return;
    }
    
    paintLayerContentsAndReflection(context, paintingInfo, paintFlags);
}

void RenderLayer::paintLayerContentsAndReflection(GraphicsContext* context, const LayerPaintingInfo& paintingInfo, PaintLayerFlags paintFlags)
{
    ASSERT(isSelfPaintingLayer() || hasSelfPaintingLayerDescendant());

    PaintLayerFlags localPaintFlags = paintFlags & ~(PaintLayerAppliedTransform);

    // Paint the reflection first if we have one.
    if (m_reflection && !m_paintingInsideReflection) {
        // Mark that we are now inside replica painting.
        m_paintingInsideReflection = true;
        reflectionLayer()->paintLayer(context, paintingInfo, localPaintFlags | PaintLayerPaintingReflection);
        m_paintingInsideReflection = false;
    }

    localPaintFlags |= PaintLayerPaintingCompositingAllPhases;
    paintLayerContents(context, paintingInfo, localPaintFlags);
}

bool RenderLayer::setupFontSubpixelQuantization(GraphicsContext* context, bool& didQuantizeFonts)
{
    if (context->paintingDisabled())
        return false;

    bool scrollingOnMainThread = true;
    Frame* frame = renderer()->frame();
#if ENABLE(THREADED_SCROLLING)
    if (frame) {
        if (Page* page = frame->page()) {
            if (ScrollingCoordinator* scrollingCoordinator = page->scrollingCoordinator())
                scrollingOnMainThread = scrollingCoordinator->shouldUpdateScrollLayerPositionOnMainThread();
        }
    }
#endif

    // FIXME: We shouldn't have to disable subpixel quantization for overflow clips or subframes once we scroll those
    // things on the scrolling thread.
    bool contentsScrollByPainting = (renderer()->hasOverflowClip() && !usesCompositedScrolling()) || (frame && frame->ownerElement());
    if (scrollingOnMainThread || contentsScrollByPainting) {
        didQuantizeFonts = context->shouldSubpixelQuantizeFonts();
        context->setShouldSubpixelQuantizeFonts(false);
        return true;
    }
    return false;
}

bool RenderLayer::setupClipPath(GraphicsContext* context, const LayerPaintingInfo& paintingInfo, const LayoutPoint& offsetFromRoot, LayoutRect& rootRelativeBounds, bool& rootRelativeBoundsComputed)
{
    if (!renderer()->hasClipPath() || context->paintingDisabled())
        return false;

    RenderStyle* style = renderer()->style();

    ASSERT(style->clipPath());
    if (style->clipPath()->getOperationType() == ClipPathOperation::SHAPE) {
        ShapeClipPathOperation* clipPath = static_cast<ShapeClipPathOperation*>(style->clipPath());

        if (!rootRelativeBoundsComputed) {
            rootRelativeBounds = calculateLayerBounds(paintingInfo.rootLayer, &offsetFromRoot, 0);
            rootRelativeBoundsComputed = true;
        }

        context->save();
        context->clipPath(clipPath->path(rootRelativeBounds), clipPath->windRule());
        return true;
    }

#if ENABLE(SVG)
    if (style->clipPath()->getOperationType() == ClipPathOperation::REFERENCE) {
        ReferenceClipPathOperation* referenceClipPathOperation = static_cast<ReferenceClipPathOperation*>(style->clipPath());
        Document* document = renderer()->document();
        Element* element = document ? document->getElementById(referenceClipPathOperation->fragment()) : 0;
        if (element && element->hasTagName(SVGNames::clipPathTag) && element->renderer()) {
            if (!rootRelativeBoundsComputed) {
                rootRelativeBounds = calculateLayerBounds(paintingInfo.rootLayer, &offsetFromRoot, 0);
                rootRelativeBoundsComputed = true;
            }

            // FIXME: This should use a safer cast such as toRenderSVGResourceContainer().
            // FIXME: Should this do a context->save() and return true so we restore the context?
            static_cast<RenderSVGResourceClipper*>(element->renderer())->applyClippingToContext(renderer(), rootRelativeBounds, paintingInfo.paintDirtyRect, context);
        }
    }
#endif

    return false;
}

#if ENABLE(CSS_FILTERS)
PassOwnPtr<FilterEffectRendererHelper> RenderLayer::setupFilters(GraphicsContext* context, LayerPaintingInfo& paintingInfo, PaintLayerFlags paintFlags, const LayoutPoint& offsetFromRoot, LayoutRect& rootRelativeBounds, bool& rootRelativeBoundsComputed)
{
    if (context->paintingDisabled())
        return nullptr;

    if (paintFlags & PaintLayerPaintingOverlayScrollbars)
        return nullptr;

    bool hasPaintedFilter = filterRenderer() && paintsWithFilters();
    if (!hasPaintedFilter)
        return nullptr;

    OwnPtr<FilterEffectRendererHelper> filterPainter = adoptPtr(new FilterEffectRendererHelper(hasPaintedFilter));
    if (!filterPainter->haveFilterEffect())
        return nullptr;
    
    RenderLayerFilterInfo* filterInfo = this->filterInfo();
    ASSERT(filterInfo);
    LayoutRect filterRepaintRect = filterInfo->dirtySourceRect();
    filterRepaintRect.move(offsetFromRoot.x(), offsetFromRoot.y());

    if (!rootRelativeBoundsComputed) {
        rootRelativeBounds = calculateLayerBounds(paintingInfo.rootLayer, &offsetFromRoot, 0);
        rootRelativeBoundsComputed = true;
    }

    if (filterPainter->prepareFilterEffect(this, rootRelativeBounds, paintingInfo.paintDirtyRect, filterRepaintRect)) {
        // Now we know for sure, that the source image will be updated, so we can revert our tracking repaint rect back to zero.
        filterInfo->resetDirtySourceRect();

        if (!filterPainter->beginFilterEffect())
            return nullptr;

        // Check that we didn't fail to allocate the graphics context for the offscreen buffer.
        ASSERT(filterPainter->hasStartedFilterEffect());

        paintingInfo.paintDirtyRect = filterPainter->repaintRect();
        // If the filter needs the full source image, we need to avoid using the clip rectangles.
        // Otherwise, if for example this layer has overflow:hidden, a drop shadow will not compute correctly.
        // Note that we will still apply the clipping on the final rendering of the filter.
        paintingInfo.clipToDirtyRect = !filterRenderer()->hasFilterThatMovesPixels();
        return filterPainter.release();
    }
    return nullptr;
}

GraphicsContext* RenderLayer::applyFilters(FilterEffectRendererHelper* filterPainter, GraphicsContext* originalContext, LayerPaintingInfo& paintingInfo, LayerFragments& layerFragments)
{
    ASSERT(filterPainter->hasStartedFilterEffect());
    // Apply the correct clipping (ie. overflow: hidden).
    // FIXME: It is incorrect to just clip to the damageRect here once multiple fragments are involved.
    ClipRect backgroundRect = layerFragments.isEmpty() ? ClipRect() : layerFragments[0].backgroundRect;
    clipToRect(paintingInfo.rootLayer, originalContext, paintingInfo.paintDirtyRect, backgroundRect);
    filterPainter->applyFilterEffect(originalContext);
    restoreClip(originalContext, paintingInfo.paintDirtyRect, backgroundRect);
    return originalContext;
}
#endif

void RenderLayer::paintLayerContents(GraphicsContext* context, const LayerPaintingInfo& paintingInfo, PaintLayerFlags paintFlags)
{
    ASSERT(isSelfPaintingLayer() || hasSelfPaintingLayerDescendant());

    PaintLayerFlags localPaintFlags = paintFlags & ~(PaintLayerAppliedTransform);
    bool haveTransparency = localPaintFlags & PaintLayerHaveTransparency;
    bool isSelfPaintingLayer = this->isSelfPaintingLayer();
    bool isPaintingOverlayScrollbars = paintFlags & PaintLayerPaintingOverlayScrollbars;
    bool isPaintingScrollingContent = paintFlags & PaintLayerPaintingCompositingScrollingPhase;
    bool isPaintingCompositedForeground = paintFlags & PaintLayerPaintingCompositingForegroundPhase;
    bool isPaintingCompositedBackground = paintFlags & PaintLayerPaintingCompositingBackgroundPhase;
    bool isPaintingOverflowContents = paintFlags & PaintLayerPaintingOverflowContents;
    // Outline always needs to be painted even if we have no visible content. Also,
    // the outline is painted in the background phase during composited scrolling.
    // If it were painted in the foreground phase, it would move with the scrolled
    // content. When not composited scrolling, the outline is painted in the
    // foreground phase. Since scrolled contents are moved by repainting in this
    // case, the outline won't get 'dragged along'.
    bool shouldPaintOutline = isSelfPaintingLayer && !isPaintingOverlayScrollbars
        && ((isPaintingScrollingContent && isPaintingCompositedBackground)
        || (!isPaintingScrollingContent && isPaintingCompositedForeground));
    bool shouldPaintContent = m_hasVisibleContent && isSelfPaintingLayer && !isPaintingOverlayScrollbars;

    if (localPaintFlags & PaintLayerPaintingRootBackgroundOnly && !renderer()->isRenderView() && !renderer()->isRoot())
        return;

    // Ensure our lists are up-to-date.
    updateLayerListsIfNeeded();

    LayoutPoint offsetFromRoot;
    convertToLayerCoords(paintingInfo.rootLayer, offsetFromRoot);

    LayoutRect rootRelativeBounds;
    bool rootRelativeBoundsComputed = false;

    // FIXME: We shouldn't have to disable subpixel quantization for overflow clips or subframes once we scroll those
    // things on the scrolling thread.
    bool didQuantizeFonts = true;
    bool needToAdjustSubpixelQuantization = setupFontSubpixelQuantization(context, didQuantizeFonts);

    // Apply clip-path to context.
    bool hasClipPath = setupClipPath(context, paintingInfo, offsetFromRoot, rootRelativeBounds, rootRelativeBoundsComputed);

    LayerPaintingInfo localPaintingInfo(paintingInfo);

    GraphicsContext* transparencyLayerContext = context;
#if ENABLE(CSS_FILTERS)
    OwnPtr<FilterEffectRendererHelper> filterPainter = setupFilters(context, localPaintingInfo, paintFlags, offsetFromRoot, rootRelativeBounds, rootRelativeBoundsComputed);
    if (filterPainter) {
        context = filterPainter->filterContext();
        if (context != transparencyLayerContext && haveTransparency) {
            // If we have a filter and transparency, we have to eagerly start a transparency layer here, rather than risk a child layer lazily starts one with the wrong context.
            beginTransparencyLayers(transparencyLayerContext, localPaintingInfo.rootLayer, paintingInfo.paintDirtyRect, localPaintingInfo.paintBehavior);
        }
    }
#endif

    // If this layer's renderer is a child of the subtreePaintRoot, we render unconditionally, which
    // is done by passing a nil subtreePaintRoot down to our renderer (as if no subtreePaintRoot was ever set).
    // Otherwise, our renderer tree may or may not contain the subtreePaintRoot root, so we pass that root along
    // so it will be tested against as we descend through the renderers.
    RenderObject* subtreePaintRootForRenderer = 0;
    if (localPaintingInfo.subtreePaintRoot && !renderer()->isDescendantOf(localPaintingInfo.subtreePaintRoot))
        subtreePaintRootForRenderer = localPaintingInfo.subtreePaintRoot;

    if (localPaintingInfo.overlapTestRequests && isSelfPaintingLayer)
        performOverlapTests(*localPaintingInfo.overlapTestRequests, localPaintingInfo.rootLayer, this);

    bool forceBlackText = localPaintingInfo.paintBehavior & PaintBehaviorForceBlackText;
    bool selectionOnly  = localPaintingInfo.paintBehavior & PaintBehaviorSelectionOnly;
    
    PaintBehavior paintBehavior = PaintBehaviorNormal;
    if (localPaintFlags & PaintLayerPaintingSkipRootBackground)
        paintBehavior |= PaintBehaviorSkipRootBackground;
    else if (localPaintFlags & PaintLayerPaintingRootBackgroundOnly)
        paintBehavior |= PaintBehaviorRootBackgroundOnly;

    LayerFragments layerFragments;
    if (shouldPaintContent || shouldPaintOutline || isPaintingOverlayScrollbars) {
        // Collect the fragments. This will compute the clip rectangles and paint offsets for each layer fragment, as well as whether or not the content of each
        // fragment should paint. If the parent's filter dictates full repaint to ensure proper filter effect,
        // use the overflow clip as dirty rect, instead of no clipping. It maintains proper clipping for overflow::scroll.
        LayoutRect paintDirtyRect = localPaintingInfo.paintDirtyRect;
        if (!paintingInfo.clipToDirtyRect && renderer()->hasOverflowClip()) {
            // We can turn clipping back by requesting full repaint for the overflow area.
            localPaintingInfo.clipToDirtyRect = true;
            paintDirtyRect = selfClipRect();
        }
        collectFragments(layerFragments, localPaintingInfo.rootLayer, localPaintingInfo.region, paintDirtyRect,
            (localPaintFlags & PaintLayerTemporaryClipRects) ? TemporaryClipRects : PaintingClipRects, IgnoreOverlayScrollbarSize,
            (isPaintingOverflowContents) ? IgnoreOverflowClip : RespectOverflowClip, &offsetFromRoot);
        updatePaintingInfoForFragments(layerFragments, localPaintingInfo, localPaintFlags, shouldPaintContent, &offsetFromRoot);
    }
    
    if (isPaintingCompositedBackground) {
        // Paint only the backgrounds for all of the fragments of the layer.
        if (shouldPaintContent && !selectionOnly)
            paintBackgroundForFragments(layerFragments, context, transparencyLayerContext, paintingInfo.paintDirtyRect, haveTransparency,
                localPaintingInfo, paintBehavior, subtreePaintRootForRenderer);
    }

    // Now walk the sorted list of children with negative z-indices.
    if ((isPaintingScrollingContent && isPaintingOverflowContents) || (!isPaintingScrollingContent && isPaintingCompositedBackground))
        paintList(negZOrderList(), context, localPaintingInfo, localPaintFlags);
    
    if (isPaintingCompositedForeground) {
        if (shouldPaintContent)
            paintForegroundForFragments(layerFragments, context, transparencyLayerContext, paintingInfo.paintDirtyRect, haveTransparency,
                localPaintingInfo, paintBehavior, subtreePaintRootForRenderer, selectionOnly, forceBlackText);
    }

    if (shouldPaintOutline)
        paintOutlineForFragments(layerFragments, context, localPaintingInfo, paintBehavior, subtreePaintRootForRenderer);

    if (isPaintingCompositedForeground) {
        // Paint any child layers that have overflow.
        paintList(m_normalFlowList.get(), context, localPaintingInfo, localPaintFlags);
    
        // Now walk the sorted list of children with positive z-indices.
        paintList(posZOrderList(), context, localPaintingInfo, localPaintFlags);
    }

    if (isPaintingOverlayScrollbars)
        paintOverflowControlsForFragments(layerFragments, context, localPaintingInfo);

#if ENABLE(CSS_FILTERS)
    if (filterPainter) {
        context = applyFilters(filterPainter.get(), transparencyLayerContext, localPaintingInfo, layerFragments);
        filterPainter.clear();
    }
#endif
    
    // Make sure that we now use the original transparency context.
    ASSERT(transparencyLayerContext == context);

    if ((localPaintFlags & PaintLayerPaintingCompositingMaskPhase) && shouldPaintContent && renderer()->hasMask() && !selectionOnly) {
        // Paint the mask for the fragments.
        paintMaskForFragments(layerFragments, context, localPaintingInfo, subtreePaintRootForRenderer);
    }

    // End our transparency layer
    if (haveTransparency && m_usedTransparency && !m_paintingInsideReflection) {
        context->endTransparencyLayer();
        context->restore();
        m_usedTransparency = false;
    }

    // Re-set this to whatever it was before we painted the layer.
    if (needToAdjustSubpixelQuantization)
        context->setShouldSubpixelQuantizeFonts(didQuantizeFonts);

    if (hasClipPath)
        context->restore();
}

void RenderLayer::paintLayerByApplyingTransform(GraphicsContext* context, const LayerPaintingInfo& paintingInfo, PaintLayerFlags paintFlags, const LayoutPoint& translationOffset)
{
    // This involves subtracting out the position of the layer in our current coordinate space, but preserving
    // the accumulated error for sub-pixel layout.
    LayoutPoint delta;
    convertToLayerCoords(paintingInfo.rootLayer, delta);
    delta.moveBy(translationOffset);
    TransformationMatrix transform(renderableTransform(paintingInfo.paintBehavior));
    IntPoint roundedDelta = roundedIntPoint(delta);
    transform.translateRight(roundedDelta.x(), roundedDelta.y());
    LayoutSize adjustedSubPixelAccumulation = paintingInfo.subPixelAccumulation + (delta - roundedDelta);

    // Apply the transform.
    GraphicsContextStateSaver stateSaver(*context);
    context->concatCTM(transform.toAffineTransform());

    // Now do a paint with the root layer shifted to be us.
    LayerPaintingInfo transformedPaintingInfo(this, enclosingIntRect(transform.inverse().mapRect(paintingInfo.paintDirtyRect)), paintingInfo.paintBehavior,
        adjustedSubPixelAccumulation, paintingInfo.subtreePaintRoot, paintingInfo.region, paintingInfo.overlapTestRequests);
    paintLayerContentsAndReflection(context, transformedPaintingInfo, paintFlags);
}

void RenderLayer::paintList(Vector<RenderLayer*>* list, GraphicsContext* context, const LayerPaintingInfo& paintingInfo, PaintLayerFlags paintFlags)
{
    if (!list)
        return;

    if (!hasSelfPaintingLayerDescendant())
        return;

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(this);
#endif

    for (size_t i = 0; i < list->size(); ++i) {
        RenderLayer* childLayer = list->at(i);
        if (childLayer->isOutOfFlowRenderFlowThread())
            continue;
        if (!childLayer->isPaginated())
            childLayer->paintLayer(context, paintingInfo, paintFlags);
        else
            paintPaginatedChildLayer(childLayer, context, paintingInfo, paintFlags);
    }
}

void RenderLayer::collectFragments(LayerFragments& fragments, const RenderLayer* rootLayer, RenderRegion* region, const LayoutRect& dirtyRect,
    ClipRectsType clipRectsType, OverlayScrollbarSizeRelevancy inOverlayScrollbarSizeRelevancy, ShouldRespectOverflowClip respectOverflowClip, const LayoutPoint* offsetFromRoot,
    const LayoutRect* layerBoundingBox)
{
    if (!enclosingPaginationLayer() || hasTransform()) {
        // For unpaginated layers, there is only one fragment.
        LayerFragment fragment;
        ClipRectsContext clipRectsContext(rootLayer, region, clipRectsType, inOverlayScrollbarSizeRelevancy, respectOverflowClip);
        calculateRects(clipRectsContext, dirtyRect, fragment.layerBounds, fragment.backgroundRect, fragment.foregroundRect, fragment.outlineRect, offsetFromRoot);
        fragments.append(fragment);
        return;
    }
    
    // Compute our offset within the enclosing pagination layer.
    LayoutPoint offsetWithinPaginatedLayer;
    convertToLayerCoords(enclosingPaginationLayer(), offsetWithinPaginatedLayer);
    
    // Calculate clip rects relative to the enclosingPaginationLayer. The purpose of this call is to determine our bounds clipped to intermediate
    // layers between us and the pagination context. It's important to minimize the number of fragments we need to create and this helps with that.
    ClipRectsContext paginationClipRectsContext(enclosingPaginationLayer(), region, clipRectsType, inOverlayScrollbarSizeRelevancy, respectOverflowClip);
    LayoutRect layerBoundsInFlowThread;
    ClipRect backgroundRectInFlowThread;
    ClipRect foregroundRectInFlowThread;
    ClipRect outlineRectInFlowThread;
    calculateRects(paginationClipRectsContext, PaintInfo::infiniteRect(), layerBoundsInFlowThread, backgroundRectInFlowThread, foregroundRectInFlowThread,
        outlineRectInFlowThread, &offsetWithinPaginatedLayer);
    
    // Take our bounding box within the flow thread and clip it.
    LayoutRect layerBoundingBoxInFlowThread = layerBoundingBox ? *layerBoundingBox : boundingBox(enclosingPaginationLayer(), 0, &offsetWithinPaginatedLayer);
    layerBoundingBoxInFlowThread.intersect(backgroundRectInFlowThread.rect());

    // Shift the dirty rect into flow thread coordinates.
    LayoutPoint offsetOfPaginationLayerFromRoot;
    enclosingPaginationLayer()->convertToLayerCoords(rootLayer, offsetOfPaginationLayerFromRoot);
    LayoutRect dirtyRectInFlowThread(dirtyRect);
    dirtyRectInFlowThread.moveBy(-offsetOfPaginationLayerFromRoot);

    // Tell the flow thread to collect the fragments. We pass enough information to create a minimal number of fragments based off the pages/columns
    // that intersect the actual dirtyRect as well as the pages/columns that intersect our layer's bounding box.
    RenderFlowThread* enclosingFlowThread = toRenderFlowThread(enclosingPaginationLayer()->renderer());
    enclosingFlowThread->collectLayerFragments(fragments, layerBoundingBoxInFlowThread, dirtyRectInFlowThread);
    
    if (fragments.isEmpty())
        return;
    
    // Get the parent clip rects of the pagination layer, since we need to intersect with that when painting column contents.
    ClipRect ancestorClipRect = dirtyRect;
    if (enclosingPaginationLayer()->parent()) {
        ClipRectsContext clipRectsContext(rootLayer, region, clipRectsType, inOverlayScrollbarSizeRelevancy, respectOverflowClip);
        ancestorClipRect = enclosingPaginationLayer()->backgroundClipRect(clipRectsContext);
        ancestorClipRect.intersect(dirtyRect);
    }

    for (size_t i = 0; i < fragments.size(); ++i) {
        LayerFragment& fragment = fragments.at(i);
        
        // Set our four rects with all clipping applied that was internal to the flow thread.
        fragment.setRects(layerBoundsInFlowThread, backgroundRectInFlowThread, foregroundRectInFlowThread, outlineRectInFlowThread);
        
        // Shift to the root-relative physical position used when painting the flow thread in this fragment.
        fragment.moveBy(fragment.paginationOffset + offsetOfPaginationLayerFromRoot);

        // Intersect the fragment with our ancestor's background clip so that e.g., columns in an overflow:hidden block are
        // properly clipped by the overflow.
        fragment.intersect(ancestorClipRect.rect());
        
        // Now intersect with our pagination clip. This will typically mean we're just intersecting the dirty rect with the column
        // clip, so the column clip ends up being all we apply.
        fragment.intersect(fragment.paginationClip);
    }
}

void RenderLayer::updatePaintingInfoForFragments(LayerFragments& fragments, const LayerPaintingInfo& localPaintingInfo, PaintLayerFlags localPaintFlags,
    bool shouldPaintContent, const LayoutPoint* offsetFromRoot)
{
    ASSERT(offsetFromRoot);
    for (size_t i = 0; i < fragments.size(); ++i) {
        LayerFragment& fragment = fragments.at(i);
        fragment.shouldPaintContent = shouldPaintContent;
        if (this != localPaintingInfo.rootLayer || !(localPaintFlags & PaintLayerPaintingOverflowContents)) {
            LayoutPoint newOffsetFromRoot = *offsetFromRoot + fragment.paginationOffset;
            fragment.shouldPaintContent &= intersectsDamageRect(fragment.layerBounds, fragment.backgroundRect.rect(), localPaintingInfo.rootLayer, &newOffsetFromRoot);
        }
    }
}

void RenderLayer::paintTransformedLayerIntoFragments(GraphicsContext* context, const LayerPaintingInfo& paintingInfo, PaintLayerFlags paintFlags)
{
    LayerFragments enclosingPaginationFragments;
    LayoutPoint offsetOfPaginationLayerFromRoot;
    LayoutRect transformedExtent = transparencyClipBox(this, enclosingPaginationLayer(), PaintingTransparencyClipBox, RootOfTransparencyClipBox, paintingInfo.paintBehavior);
    enclosingPaginationLayer()->collectFragments(enclosingPaginationFragments, paintingInfo.rootLayer, paintingInfo.region, paintingInfo.paintDirtyRect,
        (paintFlags & PaintLayerTemporaryClipRects) ? TemporaryClipRects : PaintingClipRects, IgnoreOverlayScrollbarSize,
        (paintFlags & PaintLayerPaintingOverflowContents) ? IgnoreOverflowClip : RespectOverflowClip, &offsetOfPaginationLayerFromRoot, &transformedExtent);
    
    for (size_t i = 0; i < enclosingPaginationFragments.size(); ++i) {
        const LayerFragment& fragment = enclosingPaginationFragments.at(i);
        
        // Apply the page/column clip for this fragment, as well as any clips established by layers in between us and
        // the enclosing pagination layer.
        LayoutRect clipRect = fragment.backgroundRect.rect();
        
        // Now compute the clips within a given fragment
        if (parent() != enclosingPaginationLayer()) {
            enclosingPaginationLayer()->convertToLayerCoords(paintingInfo.rootLayer, offsetOfPaginationLayerFromRoot);
    
            ClipRectsContext clipRectsContext(enclosingPaginationLayer(), paintingInfo.region, (paintFlags & PaintLayerTemporaryClipRects) ? TemporaryClipRects : PaintingClipRects,
                IgnoreOverlayScrollbarSize, (paintFlags & PaintLayerPaintingOverflowContents) ? IgnoreOverflowClip : RespectOverflowClip);
            LayoutRect parentClipRect = backgroundClipRect(clipRectsContext).rect();
            parentClipRect.moveBy(fragment.paginationOffset + offsetOfPaginationLayerFromRoot);
            clipRect.intersect(parentClipRect);
        }

        parent()->clipToRect(paintingInfo.rootLayer, context, paintingInfo.paintDirtyRect, clipRect);
        paintLayerByApplyingTransform(context, paintingInfo, paintFlags, fragment.paginationOffset);
        parent()->restoreClip(context, paintingInfo.paintDirtyRect, clipRect);
    }
}

void RenderLayer::paintBackgroundForFragments(const LayerFragments& layerFragments, GraphicsContext* context, GraphicsContext* transparencyLayerContext,
    const LayoutRect& transparencyPaintDirtyRect, bool haveTransparency, const LayerPaintingInfo& localPaintingInfo, PaintBehavior paintBehavior,
    RenderObject* subtreePaintRootForRenderer)
{
    for (size_t i = 0; i < layerFragments.size(); ++i) {
        const LayerFragment& fragment = layerFragments.at(i);
        if (!fragment.shouldPaintContent)
            continue;

        // Begin transparency layers lazily now that we know we have to paint something.
        if (haveTransparency)
            beginTransparencyLayers(transparencyLayerContext, localPaintingInfo.rootLayer, transparencyPaintDirtyRect, localPaintingInfo.paintBehavior);
    
        if (localPaintingInfo.clipToDirtyRect) {
            // Paint our background first, before painting any child layers.
            // Establish the clip used to paint our background.
            clipToRect(localPaintingInfo.rootLayer, context, localPaintingInfo.paintDirtyRect, fragment.backgroundRect, DoNotIncludeSelfForBorderRadius); // Background painting will handle clipping to self.
        }
        
        // Paint the background.
        // FIXME: Eventually we will collect the region from the fragment itself instead of just from the paint info.
        PaintInfo paintInfo(context, pixelSnappedIntRect(fragment.backgroundRect.rect()), PaintPhaseBlockBackground, paintBehavior, subtreePaintRootForRenderer, localPaintingInfo.region, 0, 0, localPaintingInfo.rootLayer->renderer());
        renderer()->paint(paintInfo, toPoint(fragment.layerBounds.location() - renderBoxLocation() + localPaintingInfo.subPixelAccumulation));

        if (localPaintingInfo.clipToDirtyRect)
            restoreClip(context, localPaintingInfo.paintDirtyRect, fragment.backgroundRect);
    }
}

void RenderLayer::paintForegroundForFragments(const LayerFragments& layerFragments, GraphicsContext* context, GraphicsContext* transparencyLayerContext,
    const LayoutRect& transparencyPaintDirtyRect, bool haveTransparency, const LayerPaintingInfo& localPaintingInfo, PaintBehavior paintBehavior,
    RenderObject* subtreePaintRootForRenderer, bool selectionOnly, bool forceBlackText)
{
    // Begin transparency if we have something to paint.
    if (haveTransparency) {
        for (size_t i = 0; i < layerFragments.size(); ++i) {
            const LayerFragment& fragment = layerFragments.at(i);
            if (fragment.shouldPaintContent && !fragment.foregroundRect.isEmpty()) {
                beginTransparencyLayers(transparencyLayerContext, localPaintingInfo.rootLayer, transparencyPaintDirtyRect, localPaintingInfo.paintBehavior);
                break;
            }
        }
    }
    
    PaintBehavior localPaintBehavior = forceBlackText ? (PaintBehavior)PaintBehaviorForceBlackText : paintBehavior;

    // Optimize clipping for the single fragment case.
    bool shouldClip = localPaintingInfo.clipToDirtyRect && layerFragments.size() == 1 && layerFragments[0].shouldPaintContent && !layerFragments[0].foregroundRect.isEmpty();
    if (shouldClip)
        clipToRect(localPaintingInfo.rootLayer, context, localPaintingInfo.paintDirtyRect, layerFragments[0].foregroundRect);
    
    // We have to loop through every fragment multiple times, since we have to repaint in each specific phase in order for
    // interleaving of the fragments to work properly.
    paintForegroundForFragmentsWithPhase(selectionOnly ? PaintPhaseSelection : PaintPhaseChildBlockBackgrounds, layerFragments,
        context, localPaintingInfo, localPaintBehavior, subtreePaintRootForRenderer);
    
    if (!selectionOnly) {
        paintForegroundForFragmentsWithPhase(PaintPhaseFloat, layerFragments, context, localPaintingInfo, localPaintBehavior, subtreePaintRootForRenderer);
        paintForegroundForFragmentsWithPhase(PaintPhaseForeground, layerFragments, context, localPaintingInfo, localPaintBehavior, subtreePaintRootForRenderer);
        paintForegroundForFragmentsWithPhase(PaintPhaseChildOutlines, layerFragments, context, localPaintingInfo, localPaintBehavior, subtreePaintRootForRenderer);
    }
    
    if (shouldClip)
        restoreClip(context, localPaintingInfo.paintDirtyRect, layerFragments[0].foregroundRect);
}

void RenderLayer::paintForegroundForFragmentsWithPhase(PaintPhase phase, const LayerFragments& layerFragments, GraphicsContext* context,
    const LayerPaintingInfo& localPaintingInfo, PaintBehavior paintBehavior, RenderObject* subtreePaintRootForRenderer)
{
    bool shouldClip = localPaintingInfo.clipToDirtyRect && layerFragments.size() > 1;

    for (size_t i = 0; i < layerFragments.size(); ++i) {
        const LayerFragment& fragment = layerFragments.at(i);
        if (!fragment.shouldPaintContent || fragment.foregroundRect.isEmpty())
            continue;
        
        if (shouldClip)
            clipToRect(localPaintingInfo.rootLayer, context, localPaintingInfo.paintDirtyRect, fragment.foregroundRect);
    
        PaintInfo paintInfo(context, pixelSnappedIntRect(fragment.foregroundRect.rect()), phase, paintBehavior, subtreePaintRootForRenderer, localPaintingInfo.region, 0, 0, localPaintingInfo.rootLayer->renderer());
        if (phase == PaintPhaseForeground)
            paintInfo.overlapTestRequests = localPaintingInfo.overlapTestRequests;
        renderer()->paint(paintInfo, toPoint(fragment.layerBounds.location() - renderBoxLocation() + localPaintingInfo.subPixelAccumulation));
        
        if (shouldClip)
            restoreClip(context, localPaintingInfo.paintDirtyRect, fragment.foregroundRect);
    }
}

void RenderLayer::paintOutlineForFragments(const LayerFragments& layerFragments, GraphicsContext* context, const LayerPaintingInfo& localPaintingInfo,
    PaintBehavior paintBehavior, RenderObject* subtreePaintRootForRenderer)
{
    for (size_t i = 0; i < layerFragments.size(); ++i) {
        const LayerFragment& fragment = layerFragments.at(i);
        if (fragment.outlineRect.isEmpty())
            continue;
    
        // Paint our own outline
        PaintInfo paintInfo(context, pixelSnappedIntRect(fragment.outlineRect.rect()), PaintPhaseSelfOutline, paintBehavior, subtreePaintRootForRenderer, localPaintingInfo.region, 0, 0, localPaintingInfo.rootLayer->renderer());
        clipToRect(localPaintingInfo.rootLayer, context, localPaintingInfo.paintDirtyRect, fragment.outlineRect, DoNotIncludeSelfForBorderRadius);
        renderer()->paint(paintInfo, toPoint(fragment.layerBounds.location() - renderBoxLocation() + localPaintingInfo.subPixelAccumulation));
        restoreClip(context, localPaintingInfo.paintDirtyRect, fragment.outlineRect);
    }
}

void RenderLayer::paintMaskForFragments(const LayerFragments& layerFragments, GraphicsContext* context, const LayerPaintingInfo& localPaintingInfo,
    RenderObject* subtreePaintRootForRenderer)
{
    for (size_t i = 0; i < layerFragments.size(); ++i) {
        const LayerFragment& fragment = layerFragments.at(i);
        if (!fragment.shouldPaintContent)
            continue;

        if (localPaintingInfo.clipToDirtyRect)
            clipToRect(localPaintingInfo.rootLayer, context, localPaintingInfo.paintDirtyRect, fragment.backgroundRect, DoNotIncludeSelfForBorderRadius); // Mask painting will handle clipping to self.
        
        // Paint the mask.
        // FIXME: Eventually we will collect the region from the fragment itself instead of just from the paint info.
        PaintInfo paintInfo(context, pixelSnappedIntRect(fragment.backgroundRect.rect()), PaintPhaseMask, PaintBehaviorNormal, subtreePaintRootForRenderer, localPaintingInfo.region, 0, 0, localPaintingInfo.rootLayer->renderer());
        renderer()->paint(paintInfo, toPoint(fragment.layerBounds.location() - renderBoxLocation() + localPaintingInfo.subPixelAccumulation));
        
        if (localPaintingInfo.clipToDirtyRect)
            restoreClip(context, localPaintingInfo.paintDirtyRect, fragment.backgroundRect);
    }
}

void RenderLayer::paintOverflowControlsForFragments(const LayerFragments& layerFragments, GraphicsContext* context, const LayerPaintingInfo& localPaintingInfo)
{
    for (size_t i = 0; i < layerFragments.size(); ++i) {
        const LayerFragment& fragment = layerFragments.at(i);
        clipToRect(localPaintingInfo.rootLayer, context, localPaintingInfo.paintDirtyRect, fragment.backgroundRect);
        paintOverflowControls(context, roundedIntPoint(toPoint(fragment.layerBounds.location() - renderBoxLocation() + localPaintingInfo.subPixelAccumulation)),
            pixelSnappedIntRect(fragment.backgroundRect.rect()), true);
        restoreClip(context, localPaintingInfo.paintDirtyRect, fragment.backgroundRect);
    }
}

void RenderLayer::paintPaginatedChildLayer(RenderLayer* childLayer, GraphicsContext* context, const LayerPaintingInfo& paintingInfo, PaintLayerFlags paintFlags)
{
    // We need to do multiple passes, breaking up our child layer into strips.
    Vector<RenderLayer*> columnLayers;
    RenderLayer* ancestorLayer = isNormalFlowOnly() ? parent() : stackingContainer();
    for (RenderLayer* curr = childLayer->parent(); curr; curr = curr->parent()) {
        if (curr->renderer()->hasColumns() && checkContainingBlockChainForPagination(childLayer->renderer(), curr->renderBox()))
            columnLayers.append(curr);
        if (curr == ancestorLayer)
            break;
    }

    // It is possible for paintLayer() to be called after the child layer ceases to be paginated but before
    // updateLayerPositions() is called and resets the isPaginated() flag, see <rdar://problem/10098679>.
    // If this is the case, just bail out, since the upcoming call to updateLayerPositions() will repaint the layer.
    if (!columnLayers.size())
        return;

    paintChildLayerIntoColumns(childLayer, context, paintingInfo, paintFlags, columnLayers, columnLayers.size() - 1);
}

void RenderLayer::paintChildLayerIntoColumns(RenderLayer* childLayer, GraphicsContext* context, const LayerPaintingInfo& paintingInfo,
    PaintLayerFlags paintFlags, const Vector<RenderLayer*>& columnLayers, size_t colIndex)
{
    RenderBlock* columnBlock = toRenderBlock(columnLayers[colIndex]->renderer());

    ASSERT(columnBlock && columnBlock->hasColumns());
    if (!columnBlock || !columnBlock->hasColumns())
        return;
    
    LayoutPoint layerOffset;
    // FIXME: It looks suspicious to call convertToLayerCoords here
    // as canUseConvertToLayerCoords is true for this layer.
    columnBlock->layer()->convertToLayerCoords(paintingInfo.rootLayer, layerOffset);
    
    bool isHorizontal = columnBlock->style()->isHorizontalWritingMode();

    ColumnInfo* colInfo = columnBlock->columnInfo();
    unsigned colCount = columnBlock->columnCount(colInfo);
    LayoutUnit currLogicalTopOffset = columnBlock->initialBlockOffsetForPainting();
    LayoutUnit blockDelta = columnBlock->blockDeltaForPaintingNextColumn();
    for (unsigned i = 0; i < colCount; i++) {
        // For each rect, we clip to the rect, and then we adjust our coords.
        LayoutRect colRect = columnBlock->columnRectAt(colInfo, i);
        columnBlock->flipForWritingMode(colRect);

        LayoutUnit logicalLeftOffset = (isHorizontal ? colRect.x() : colRect.y()) - columnBlock->logicalLeftOffsetForContent();
        LayoutSize offset = isHorizontal ? LayoutSize(logicalLeftOffset, currLogicalTopOffset) : LayoutSize(currLogicalTopOffset, logicalLeftOffset);
        colRect.moveBy(layerOffset);
        
        LayoutRect localDirtyRect(paintingInfo.paintDirtyRect);
        localDirtyRect.intersect(colRect);
        if (!localDirtyRect.isEmpty()) {
            GraphicsContextStateSaver stateSaver(*context);
            
            // Each strip pushes a clip, since column boxes are specified as being
            // like overflow:hidden.
            context->clip(pixelSnappedIntRect(colRect));

            if (!colIndex) {
                // Apply a translation transform to change where the layer paints.
                TransformationMatrix oldTransform;
                bool oldHasTransform = childLayer->transform();
                if (oldHasTransform)
                    oldTransform = *childLayer->transform();
                TransformationMatrix newTransform(oldTransform);
                newTransform.translateRight(roundToInt(offset.width()), roundToInt(offset.height()));
                
                childLayer->m_transform = adoptPtr(new TransformationMatrix(newTransform));
                
                LayerPaintingInfo localPaintingInfo(paintingInfo);
                localPaintingInfo.paintDirtyRect = localDirtyRect;
                childLayer->paintLayer(context, localPaintingInfo, paintFlags);

                if (oldHasTransform)
                    childLayer->m_transform = adoptPtr(new TransformationMatrix(oldTransform));
                else
                    childLayer->m_transform.clear();
            } else {
                // Adjust the transform such that the renderer's upper left corner will paint at (0,0) in user space.
                // This involves subtracting out the position of the layer in our current coordinate space.
                LayoutPoint childOffset;
                columnLayers[colIndex - 1]->convertToLayerCoords(paintingInfo.rootLayer, childOffset);
                TransformationMatrix transform;
                transform.translateRight(roundToInt(childOffset.x() + offset.width()), roundToInt(childOffset.y() + offset.height()));
                
                // Apply the transform.
                context->concatCTM(transform.toAffineTransform());

                // Now do a paint with the root layer shifted to be the next multicol block.
                LayerPaintingInfo columnPaintingInfo(paintingInfo);
                columnPaintingInfo.rootLayer = columnLayers[colIndex - 1];
                columnPaintingInfo.paintDirtyRect = transform.inverse().mapRect(localDirtyRect);
                paintChildLayerIntoColumns(childLayer, context, columnPaintingInfo, paintFlags, columnLayers, colIndex - 1);
            }
        }

        // Move to the next position.
        currLogicalTopOffset += blockDelta;
    }
}

static inline LayoutRect frameVisibleRect(RenderObject* renderer)
{
    FrameView* frameView = renderer->document()->view();
    if (!frameView)
        return LayoutRect();

    return frameView->visibleContentRect();
}

bool RenderLayer::hitTest(const HitTestRequest& request, HitTestResult& result)
{
    return hitTest(request, result.hitTestLocation(), result);
}

bool RenderLayer::hitTest(const HitTestRequest& request, const HitTestLocation& hitTestLocation, HitTestResult& result)
{
    ASSERT(isSelfPaintingLayer() || hasSelfPaintingLayerDescendant());

    renderer()->document()->updateLayout();
    
    LayoutRect hitTestArea = isOutOfFlowRenderFlowThread() ? toRenderFlowThread(renderer())->borderBoxRect() : renderer()->view()->documentRect();
    if (!request.ignoreClipping())
        hitTestArea.intersect(frameVisibleRect(renderer()));

    RenderLayer* insideLayer = hitTestLayer(this, 0, request, result, hitTestArea, hitTestLocation, false);
    if (!insideLayer) {
        // We didn't hit any layer. If we are the root layer and the mouse is -- or just was -- down, 
        // return ourselves. We do this so mouse events continue getting delivered after a drag has 
        // exited the WebView, and so hit testing over a scrollbar hits the content document.
        if (!request.isChildFrameHitTest() && (request.active() || request.release()) && isRootLayer()) {
            renderer()->updateHitTestResult(result, toRenderView(renderer())->flipForWritingMode(hitTestLocation.point()));
            insideLayer = this;
        }
    }

    // Now determine if the result is inside an anchor - if the urlElement isn't already set.
    Node* node = result.innerNode();
    if (node && !result.URLElement())
        result.setURLElement(toElement(node->enclosingLinkEventParentOrSelf()));

    // Now return whether we were inside this layer (this will always be true for the root
    // layer).
    return insideLayer;
}

Node* RenderLayer::enclosingElement() const
{
    for (RenderObject* r = renderer(); r; r = r->parent()) {
        if (Node* e = r->node())
            return e;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

#if ENABLE(DIALOG_ELEMENT)
bool RenderLayer::isInTopLayer() const
{
    Node* node = renderer()->node();
    return node && node->isElementNode() && toElement(node)->isInTopLayer();
}

bool RenderLayer::isInTopLayerSubtree() const
{
    for (const RenderLayer* layer = this; layer; layer = layer->parent()) {
        if (layer->isInTopLayer())
            return true;
    }
    return false;
}
#endif

// Compute the z-offset of the point in the transformState.
// This is effectively projecting a ray normal to the plane of ancestor, finding where that
// ray intersects target, and computing the z delta between those two points.
static double computeZOffset(const HitTestingTransformState& transformState)
{
    // We got an affine transform, so no z-offset
    if (transformState.m_accumulatedTransform.isAffine())
        return 0;

    // Flatten the point into the target plane
    FloatPoint targetPoint = transformState.mappedPoint();
    
    // Now map the point back through the transform, which computes Z.
    FloatPoint3D backmappedPoint = transformState.m_accumulatedTransform.mapPoint(FloatPoint3D(targetPoint));
    return backmappedPoint.z();
}

PassRefPtr<HitTestingTransformState> RenderLayer::createLocalTransformState(RenderLayer* rootLayer, RenderLayer* containerLayer,
                                        const LayoutRect& hitTestRect, const HitTestLocation& hitTestLocation,
                                        const HitTestingTransformState* containerTransformState,
                                        const LayoutPoint& translationOffset) const
{
    RefPtr<HitTestingTransformState> transformState;
    LayoutPoint offset;
    if (containerTransformState) {
        // If we're already computing transform state, then it's relative to the container (which we know is non-null).
        transformState = HitTestingTransformState::create(*containerTransformState);
        convertToLayerCoords(containerLayer, offset);
    } else {
        // If this is the first time we need to make transform state, then base it off of hitTestLocation,
        // which is relative to rootLayer.
        transformState = HitTestingTransformState::create(hitTestLocation.transformedPoint(), hitTestLocation.transformedRect(), FloatQuad(hitTestRect));
        convertToLayerCoords(rootLayer, offset);
    }
    offset.moveBy(translationOffset);

    RenderObject* containerRenderer = containerLayer ? containerLayer->renderer() : 0;
    if (renderer()->shouldUseTransformFromContainer(containerRenderer)) {
        TransformationMatrix containerTransform;
        renderer()->getTransformFromContainer(containerRenderer, toLayoutSize(offset), containerTransform);
        transformState->applyTransform(containerTransform, HitTestingTransformState::AccumulateTransform);
    } else {
        transformState->translate(offset.x(), offset.y(), HitTestingTransformState::AccumulateTransform);
    }
    
    return transformState;
}


static bool isHitCandidate(const RenderLayer* hitLayer, bool canDepthSort, double* zOffset, const HitTestingTransformState* transformState)
{
    if (!hitLayer)
        return false;

    // The hit layer is depth-sorting with other layers, so just say that it was hit.
    if (canDepthSort)
        return true;
    
    // We need to look at z-depth to decide if this layer was hit.
    if (zOffset) {
        ASSERT(transformState);
        // This is actually computing our z, but that's OK because the hitLayer is coplanar with us.
        double childZOffset = computeZOffset(*transformState);
        if (childZOffset > *zOffset) {
            *zOffset = childZOffset;
            return true;
        }
        return false;
    }

    return true;
}

// hitTestLocation and hitTestRect are relative to rootLayer.
// A 'flattening' layer is one preserves3D() == false.
// transformState.m_accumulatedTransform holds the transform from the containing flattening layer.
// transformState.m_lastPlanarPoint is the hitTestLocation in the plane of the containing flattening layer.
// transformState.m_lastPlanarQuad is the hitTestRect as a quad in the plane of the containing flattening layer.
// 
// If zOffset is non-null (which indicates that the caller wants z offset information), 
//  *zOffset on return is the z offset of the hit point relative to the containing flattening layer.
RenderLayer* RenderLayer::hitTestLayer(RenderLayer* rootLayer, RenderLayer* containerLayer, const HitTestRequest& request, HitTestResult& result,
                                       const LayoutRect& hitTestRect, const HitTestLocation& hitTestLocation, bool appliedTransform,
                                       const HitTestingTransformState* transformState, double* zOffset)
{
    if (!isSelfPaintingLayer() && !hasSelfPaintingLayerDescendant())
        return 0;

    // The natural thing would be to keep HitTestingTransformState on the stack, but it's big, so we heap-allocate.

    // Apply a transform if we have one.
    if (transform() && !appliedTransform) {
        if (enclosingPaginationLayer())
            return hitTestTransformedLayerInFragments(rootLayer, containerLayer, request, result, hitTestRect, hitTestLocation, transformState, zOffset);

        // Make sure the parent's clip rects have been calculated.
        if (parent()) {
            ClipRectsContext clipRectsContext(rootLayer, hitTestLocation.region(), RootRelativeClipRects, IncludeOverlayScrollbarSize);
            ClipRect clipRect = backgroundClipRect(clipRectsContext);
            // Go ahead and test the enclosing clip now.
            if (!clipRect.intersects(hitTestLocation))
                return 0;
        }

        return hitTestLayerByApplyingTransform(rootLayer, containerLayer, request, result, hitTestRect, hitTestLocation, transformState, zOffset);
    }

    // Ensure our lists and 3d status are up-to-date.
    updateCompositingAndLayerListsIfNeeded();
    update3DTransformedDescendantStatus();

    RefPtr<HitTestingTransformState> localTransformState;
    if (appliedTransform) {
        // We computed the correct state in the caller (above code), so just reference it.
        ASSERT(transformState);
        localTransformState = const_cast<HitTestingTransformState*>(transformState);
    } else if (transformState || m_has3DTransformedDescendant || preserves3D()) {
        // We need transform state for the first time, or to offset the container state, so create it here.
        localTransformState = createLocalTransformState(rootLayer, containerLayer, hitTestRect, hitTestLocation, transformState);
    }

    // Check for hit test on backface if backface-visibility is 'hidden'
    if (localTransformState && renderer()->style()->backfaceVisibility() == BackfaceVisibilityHidden) {
        TransformationMatrix invertedMatrix = localTransformState->m_accumulatedTransform.inverse();
        // If the z-vector of the matrix is negative, the back is facing towards the viewer.
        if (invertedMatrix.m33() < 0)
            return 0;
    }

    RefPtr<HitTestingTransformState> unflattenedTransformState = localTransformState;
    if (localTransformState && !preserves3D()) {
        // Keep a copy of the pre-flattening state, for computing z-offsets for the container
        unflattenedTransformState = HitTestingTransformState::create(*localTransformState);
        // This layer is flattening, so flatten the state passed to descendants.
        localTransformState->flatten();
    }

    // The following are used for keeping track of the z-depth of the hit point of 3d-transformed
    // descendants.
    double localZOffset = -numeric_limits<double>::infinity();
    double* zOffsetForDescendantsPtr = 0;
    double* zOffsetForContentsPtr = 0;
    
    bool depthSortDescendants = false;
    if (preserves3D()) {
        depthSortDescendants = true;
        // Our layers can depth-test with our container, so share the z depth pointer with the container, if it passed one down.
        zOffsetForDescendantsPtr = zOffset ? zOffset : &localZOffset;
        zOffsetForContentsPtr = zOffset ? zOffset : &localZOffset;
    } else if (m_has3DTransformedDescendant) {
        // Flattening layer with 3d children; use a local zOffset pointer to depth-test children and foreground.
        depthSortDescendants = true;
        zOffsetForDescendantsPtr = zOffset ? zOffset : &localZOffset;
        zOffsetForContentsPtr = zOffset ? zOffset : &localZOffset;
    } else if (zOffset) {
        zOffsetForDescendantsPtr = 0;
        // Container needs us to give back a z offset for the hit layer.
        zOffsetForContentsPtr = zOffset;
    }

    // This variable tracks which layer the mouse ends up being inside.
    RenderLayer* candidateLayer = 0;

    // Begin by walking our list of positive layers from highest z-index down to the lowest z-index.
    RenderLayer* hitLayer = hitTestList(posZOrderList(), rootLayer, request, result, hitTestRect, hitTestLocation,
                                        localTransformState.get(), zOffsetForDescendantsPtr, zOffset, unflattenedTransformState.get(), depthSortDescendants);
    if (hitLayer) {
        if (!depthSortDescendants)
            return hitLayer;
        candidateLayer = hitLayer;
    }

    // Now check our overflow objects.
    hitLayer = hitTestList(m_normalFlowList.get(), rootLayer, request, result, hitTestRect, hitTestLocation,
                           localTransformState.get(), zOffsetForDescendantsPtr, zOffset, unflattenedTransformState.get(), depthSortDescendants);
    if (hitLayer) {
        if (!depthSortDescendants)
            return hitLayer;
        candidateLayer = hitLayer;
    }

    // Collect the fragments. This will compute the clip rectangles for each layer fragment.
    LayerFragments layerFragments;
    collectFragments(layerFragments, rootLayer, hitTestLocation.region(), hitTestRect, RootRelativeClipRects, IncludeOverlayScrollbarSize);

    if (canResize() && hitTestResizerInFragments(layerFragments, hitTestLocation)) {
        renderer()->updateHitTestResult(result, hitTestLocation.point());
        return this;
    }

    // Next we want to see if the mouse pos is inside the child RenderObjects of the layer. Check
    // every fragment in reverse order.
    if (isSelfPaintingLayer()) {
        // Hit test with a temporary HitTestResult, because we only want to commit to 'result' if we know we're frontmost.
        HitTestResult tempResult(result.hitTestLocation());
        bool insideFragmentForegroundRect = false;
        if (hitTestContentsForFragments(layerFragments, request, tempResult, hitTestLocation, HitTestDescendants, insideFragmentForegroundRect)
            && isHitCandidate(this, false, zOffsetForContentsPtr, unflattenedTransformState.get())) {
            if (result.isRectBasedTest())
                result.append(tempResult);
            else
                result = tempResult;
            if (!depthSortDescendants)
                return this;
            // Foreground can depth-sort with descendant layers, so keep this as a candidate.
            candidateLayer = this;
        } else if (insideFragmentForegroundRect && result.isRectBasedTest())
            result.append(tempResult);
    }

    // Now check our negative z-index children.
    hitLayer = hitTestList(negZOrderList(), rootLayer, request, result, hitTestRect, hitTestLocation,
        localTransformState.get(), zOffsetForDescendantsPtr, zOffset, unflattenedTransformState.get(), depthSortDescendants);
    if (hitLayer) {
        if (!depthSortDescendants)
            return hitLayer;
        candidateLayer = hitLayer;
    }

    // If we found a layer, return. Child layers, and foreground always render in front of background.
    if (candidateLayer)
        return candidateLayer;

    if (isSelfPaintingLayer()) {
        HitTestResult tempResult(result.hitTestLocation());
        bool insideFragmentBackgroundRect = false;
        if (hitTestContentsForFragments(layerFragments, request, tempResult, hitTestLocation, HitTestSelf, insideFragmentBackgroundRect)
            && isHitCandidate(this, false, zOffsetForContentsPtr, unflattenedTransformState.get())) {
            if (result.isRectBasedTest())
                result.append(tempResult);
            else
                result = tempResult;
            return this;
        }
        if (insideFragmentBackgroundRect && result.isRectBasedTest())
            result.append(tempResult);
    }

    return 0;
}

bool RenderLayer::hitTestContentsForFragments(const LayerFragments& layerFragments, const HitTestRequest& request, HitTestResult& result,
    const HitTestLocation& hitTestLocation, HitTestFilter hitTestFilter, bool& insideClipRect) const
{
    if (layerFragments.isEmpty())
        return false;

    for (int i = layerFragments.size() - 1; i >= 0; --i) {
        const LayerFragment& fragment = layerFragments.at(i);
        if ((hitTestFilter == HitTestSelf && !fragment.backgroundRect.intersects(hitTestLocation))
            || (hitTestFilter == HitTestDescendants && !fragment.foregroundRect.intersects(hitTestLocation)))
            continue;
        insideClipRect = true;
        if (hitTestContents(request, result, fragment.layerBounds, hitTestLocation, hitTestFilter))
            return true;
    }
    
    return false;
}

bool RenderLayer::hitTestResizerInFragments(const LayerFragments& layerFragments, const HitTestLocation& hitTestLocation) const
{
    if (layerFragments.isEmpty())
        return false;

    for (int i = layerFragments.size() - 1; i >= 0; --i) {
        const LayerFragment& fragment = layerFragments.at(i);
        if (fragment.backgroundRect.intersects(hitTestLocation) && resizerCornerRect(this, pixelSnappedIntRect(fragment.layerBounds)).contains(hitTestLocation.roundedPoint()))
            return true;
    }
    
    return false;
}

RenderLayer* RenderLayer::hitTestTransformedLayerInFragments(RenderLayer* rootLayer, RenderLayer* containerLayer, const HitTestRequest& request, HitTestResult& result,
    const LayoutRect& hitTestRect, const HitTestLocation& hitTestLocation, const HitTestingTransformState* transformState, double* zOffset)
{
    LayerFragments enclosingPaginationFragments;
    LayoutPoint offsetOfPaginationLayerFromRoot;
    LayoutRect transformedExtent = transparencyClipBox(this, enclosingPaginationLayer(), HitTestingTransparencyClipBox, RootOfTransparencyClipBox);
    enclosingPaginationLayer()->collectFragments(enclosingPaginationFragments, rootLayer, hitTestLocation.region(), hitTestRect,
        RootRelativeClipRects, IncludeOverlayScrollbarSize, RespectOverflowClip, &offsetOfPaginationLayerFromRoot, &transformedExtent);

    for (int i = enclosingPaginationFragments.size() - 1; i >= 0; --i) {
        const LayerFragment& fragment = enclosingPaginationFragments.at(i);
        
        // Apply the page/column clip for this fragment, as well as any clips established by layers in between us and
        // the enclosing pagination layer.
        LayoutRect clipRect = fragment.backgroundRect.rect();
        
        // Now compute the clips within a given fragment
        if (parent() != enclosingPaginationLayer()) {
            enclosingPaginationLayer()->convertToLayerCoords(rootLayer, offsetOfPaginationLayerFromRoot);
    
            ClipRectsContext clipRectsContext(enclosingPaginationLayer(), hitTestLocation.region(), RootRelativeClipRects, IncludeOverlayScrollbarSize);
            LayoutRect parentClipRect = backgroundClipRect(clipRectsContext).rect();
            parentClipRect.moveBy(fragment.paginationOffset + offsetOfPaginationLayerFromRoot);
            clipRect.intersect(parentClipRect);
        }
        
        if (!hitTestLocation.intersects(clipRect))
            continue;

        RenderLayer* hitLayer = hitTestLayerByApplyingTransform(rootLayer, containerLayer, request, result, hitTestRect, hitTestLocation,
            transformState, zOffset, fragment.paginationOffset);
        if (hitLayer)
            return hitLayer;
    }
    
    return 0;
}

RenderLayer* RenderLayer::hitTestLayerByApplyingTransform(RenderLayer* rootLayer, RenderLayer* containerLayer, const HitTestRequest& request, HitTestResult& result,
    const LayoutRect& hitTestRect, const HitTestLocation& hitTestLocation, const HitTestingTransformState* transformState, double* zOffset,
    const LayoutPoint& translationOffset)
{
    // Create a transform state to accumulate this transform.
    RefPtr<HitTestingTransformState> newTransformState = createLocalTransformState(rootLayer, containerLayer, hitTestRect, hitTestLocation, transformState, translationOffset);

    // If the transform can't be inverted, then don't hit test this layer at all.
    if (!newTransformState->m_accumulatedTransform.isInvertible())
        return 0;

    // Compute the point and the hit test rect in the coords of this layer by using the values
    // from the transformState, which store the point and quad in the coords of the last flattened
    // layer, and the accumulated transform which lets up map through preserve-3d layers.
    //
    // We can't just map hitTestLocation and hitTestRect because they may have been flattened (losing z)
    // by our container.
    FloatPoint localPoint = newTransformState->mappedPoint();
    FloatQuad localPointQuad = newTransformState->mappedQuad();
    LayoutRect localHitTestRect = newTransformState->boundsOfMappedArea();
    HitTestLocation newHitTestLocation;
    if (hitTestLocation.isRectBasedTest())
        newHitTestLocation = HitTestLocation(localPoint, localPointQuad);
    else
        newHitTestLocation = HitTestLocation(localPoint);

    // Now do a hit test with the root layer shifted to be us.
    return hitTestLayer(this, containerLayer, request, result, localHitTestRect, newHitTestLocation, true, newTransformState.get(), zOffset);
}

bool RenderLayer::hitTestContents(const HitTestRequest& request, HitTestResult& result, const LayoutRect& layerBounds, const HitTestLocation& hitTestLocation, HitTestFilter hitTestFilter) const
{
    ASSERT(isSelfPaintingLayer() || hasSelfPaintingLayerDescendant());

    if (!renderer()->hitTest(request, result, hitTestLocation, toLayoutPoint(layerBounds.location() - renderBoxLocation()), hitTestFilter)) {
        // It's wrong to set innerNode, but then claim that you didn't hit anything, unless it is
        // a rect-based test.
        ASSERT(!result.innerNode() || (result.isRectBasedTest() && result.rectBasedTestResult().size()));
        return false;
    }

    // For positioned generated content, we might still not have a
    // node by the time we get to the layer level, since none of
    // the content in the layer has an element. So just walk up
    // the tree.
    if (!result.innerNode() || !result.innerNonSharedNode()) {
        Node* e = enclosingElement();
        if (!result.innerNode())
            result.setInnerNode(e);
        if (!result.innerNonSharedNode())
            result.setInnerNonSharedNode(e);
    }
        
    return true;
}

RenderLayer* RenderLayer::hitTestList(Vector<RenderLayer*>* list, RenderLayer* rootLayer,
                                      const HitTestRequest& request, HitTestResult& result,
                                      const LayoutRect& hitTestRect, const HitTestLocation& hitTestLocation,
                                      const HitTestingTransformState* transformState, 
                                      double* zOffsetForDescendants, double* zOffset,
                                      const HitTestingTransformState* unflattenedTransformState,
                                      bool depthSortDescendants)
{
    if (!list)
        return 0;

    if (!hasSelfPaintingLayerDescendant())
        return 0;

    RenderLayer* resultLayer = 0;
    for (int i = list->size() - 1; i >= 0; --i) {
        RenderLayer* childLayer = list->at(i);
        if (childLayer->isOutOfFlowRenderFlowThread())
            continue;
        RenderLayer* hitLayer = 0;
        HitTestResult tempResult(result.hitTestLocation());
        if (childLayer->isPaginated())
            hitLayer = hitTestPaginatedChildLayer(childLayer, rootLayer, request, tempResult, hitTestRect, hitTestLocation, transformState, zOffsetForDescendants);
        else
            hitLayer = childLayer->hitTestLayer(rootLayer, this, request, tempResult, hitTestRect, hitTestLocation, false, transformState, zOffsetForDescendants);

        // If it a rect-based test, we can safely append the temporary result since it might had hit
        // nodes but not necesserily had hitLayer set.
        if (result.isRectBasedTest())
            result.append(tempResult);

        if (isHitCandidate(hitLayer, depthSortDescendants, zOffset, unflattenedTransformState)) {
            resultLayer = hitLayer;
            if (!result.isRectBasedTest())
                result = tempResult;
            if (!depthSortDescendants)
                break;
        }
    }

    return resultLayer;
}

RenderLayer* RenderLayer::hitTestPaginatedChildLayer(RenderLayer* childLayer, RenderLayer* rootLayer, const HitTestRequest& request, HitTestResult& result,
                                                     const LayoutRect& hitTestRect, const HitTestLocation& hitTestLocation, const HitTestingTransformState* transformState, double* zOffset)
{
    Vector<RenderLayer*> columnLayers;
    RenderLayer* ancestorLayer = isNormalFlowOnly() ? parent() : stackingContainer();
    for (RenderLayer* curr = childLayer->parent(); curr; curr = curr->parent()) {
        if (curr->renderer()->hasColumns() && checkContainingBlockChainForPagination(childLayer->renderer(), curr->renderBox()))
            columnLayers.append(curr);
        if (curr == ancestorLayer)
            break;
    }

    ASSERT(columnLayers.size());
    return hitTestChildLayerColumns(childLayer, rootLayer, request, result, hitTestRect, hitTestLocation, transformState, zOffset,
                                    columnLayers, columnLayers.size() - 1);
}

RenderLayer* RenderLayer::hitTestChildLayerColumns(RenderLayer* childLayer, RenderLayer* rootLayer, const HitTestRequest& request, HitTestResult& result,
                                                   const LayoutRect& hitTestRect, const HitTestLocation& hitTestLocation, const HitTestingTransformState* transformState, double* zOffset,
                                                   const Vector<RenderLayer*>& columnLayers, size_t columnIndex)
{
    RenderBlock* columnBlock = toRenderBlock(columnLayers[columnIndex]->renderer());

    ASSERT(columnBlock && columnBlock->hasColumns());
    if (!columnBlock || !columnBlock->hasColumns())
        return 0;

    LayoutPoint layerOffset;
    columnBlock->layer()->convertToLayerCoords(rootLayer, layerOffset);

    ColumnInfo* colInfo = columnBlock->columnInfo();
    int colCount = columnBlock->columnCount(colInfo);

    // We have to go backwards from the last column to the first.
    bool isHorizontal = columnBlock->style()->isHorizontalWritingMode();
    LayoutUnit logicalLeft = columnBlock->logicalLeftOffsetForContent();
    LayoutUnit currLogicalTopOffset = columnBlock->initialBlockOffsetForPainting();
    LayoutUnit blockDelta = columnBlock->blockDeltaForPaintingNextColumn();
    currLogicalTopOffset += colCount * blockDelta;
    for (int i = colCount - 1; i >= 0; i--) {
        // For each rect, we clip to the rect, and then we adjust our coords.
        LayoutRect colRect = columnBlock->columnRectAt(colInfo, i);
        columnBlock->flipForWritingMode(colRect);
        LayoutUnit currLogicalLeftOffset = (isHorizontal ? colRect.x() : colRect.y()) - logicalLeft;
        currLogicalTopOffset -= blockDelta;

        LayoutSize offset = isHorizontal ? LayoutSize(currLogicalLeftOffset, currLogicalTopOffset) : LayoutSize(currLogicalTopOffset, currLogicalLeftOffset);

        colRect.moveBy(layerOffset);

        LayoutRect localClipRect(hitTestRect);
        localClipRect.intersect(colRect);

        if (!localClipRect.isEmpty() && hitTestLocation.intersects(localClipRect)) {
            RenderLayer* hitLayer = 0;
            if (!columnIndex) {
                // Apply a translation transform to change where the layer paints.
                TransformationMatrix oldTransform;
                bool oldHasTransform = childLayer->transform();
                if (oldHasTransform)
                    oldTransform = *childLayer->transform();
                TransformationMatrix newTransform(oldTransform);
                newTransform.translateRight(offset.width(), offset.height());

                childLayer->m_transform = adoptPtr(new TransformationMatrix(newTransform));
                hitLayer = childLayer->hitTestLayer(rootLayer, columnLayers[0], request, result, localClipRect, hitTestLocation, false, transformState, zOffset);
                if (oldHasTransform)
                    childLayer->m_transform = adoptPtr(new TransformationMatrix(oldTransform));
                else
                    childLayer->m_transform.clear();
            } else {
                // Adjust the transform such that the renderer's upper left corner will be at (0,0) in user space.
                // This involves subtracting out the position of the layer in our current coordinate space.
                RenderLayer* nextLayer = columnLayers[columnIndex - 1];
                RefPtr<HitTestingTransformState> newTransformState = nextLayer->createLocalTransformState(rootLayer, nextLayer, localClipRect, hitTestLocation, transformState);
                newTransformState->translate(offset.width(), offset.height(), HitTestingTransformState::AccumulateTransform);
                FloatPoint localPoint = newTransformState->mappedPoint();
                FloatQuad localPointQuad = newTransformState->mappedQuad();
                LayoutRect localHitTestRect = newTransformState->mappedArea().enclosingBoundingBox();
                HitTestLocation newHitTestLocation;
                if (hitTestLocation.isRectBasedTest())
                    newHitTestLocation = HitTestLocation(localPoint, localPointQuad);
                else
                    newHitTestLocation = HitTestLocation(localPoint);
                newTransformState->flatten();

                hitLayer = hitTestChildLayerColumns(childLayer, columnLayers[columnIndex - 1], request, result, localHitTestRect, newHitTestLocation,
                                                    newTransformState.get(), zOffset, columnLayers, columnIndex - 1);
            }

            if (hitLayer)
                return hitLayer;
        }
    }

    return 0;
}

void RenderLayer::updateClipRects(const ClipRectsContext& clipRectsContext)
{
    ClipRectsType clipRectsType = clipRectsContext.clipRectsType;
    ASSERT(clipRectsType < NumCachedClipRectsTypes);
    if (m_clipRectsCache && m_clipRectsCache->getClipRects(clipRectsType, clipRectsContext.respectOverflowClip)) {
        ASSERT(clipRectsContext.rootLayer == m_clipRectsCache->m_clipRectsRoot[clipRectsType]);
        ASSERT(m_clipRectsCache->m_scrollbarRelevancy[clipRectsType] == clipRectsContext.overlayScrollbarSizeRelevancy);
        
#ifdef CHECK_CACHED_CLIP_RECTS
        // This code is useful to check cached clip rects, but is too expensive to leave enabled in debug builds by default.
        ClipRectsContext tempContext(clipRectsContext);
        tempContext.clipRectsType = TemporaryClipRects;
        ClipRects clipRects;
        calculateClipRects(tempContext, clipRects);
        ASSERT(clipRects == *m_clipRectsCache->getClipRects(clipRectsType, clipRectsContext.respectOverflowClip).get());
#endif
        return; // We have the correct cached value.
    }
    
    // For transformed layers, the root layer was shifted to be us, so there is no need to
    // examine the parent.  We want to cache clip rects with us as the root.
    RenderLayer* parentLayer = clipRectsContext.rootLayer != this ? parent() : 0;
    if (parentLayer)
        parentLayer->updateClipRects(clipRectsContext);

    ClipRects clipRects;
    calculateClipRects(clipRectsContext, clipRects);

    if (!m_clipRectsCache)
        m_clipRectsCache = adoptPtr(new ClipRectsCache);

    if (parentLayer && parentLayer->clipRects(clipRectsContext) && clipRects == *parentLayer->clipRects(clipRectsContext))
        m_clipRectsCache->setClipRects(clipRectsType, clipRectsContext.respectOverflowClip, parentLayer->clipRects(clipRectsContext));
    else
        m_clipRectsCache->setClipRects(clipRectsType, clipRectsContext.respectOverflowClip, ClipRects::create(clipRects));

#ifndef NDEBUG
    m_clipRectsCache->m_clipRectsRoot[clipRectsType] = clipRectsContext.rootLayer;
    m_clipRectsCache->m_scrollbarRelevancy[clipRectsType] = clipRectsContext.overlayScrollbarSizeRelevancy;
#endif
}

void RenderLayer::calculateClipRects(const ClipRectsContext& clipRectsContext, ClipRects& clipRects) const
{
    if (!parent()) {
        // The root layer's clip rect is always infinite.
        clipRects.reset(PaintInfo::infiniteRect());
        return;
    }
    
    ClipRectsType clipRectsType = clipRectsContext.clipRectsType;
    bool useCached = clipRectsType != TemporaryClipRects;

    // For transformed layers, the root layer was shifted to be us, so there is no need to
    // examine the parent.  We want to cache clip rects with us as the root.
    RenderLayer* parentLayer = clipRectsContext.rootLayer != this ? parent() : 0;
    
    // Ensure that our parent's clip has been calculated so that we can examine the values.
    if (parentLayer) {
        if (useCached && parentLayer->clipRects(clipRectsContext))
            clipRects = *parentLayer->clipRects(clipRectsContext);
        else {
            ClipRectsContext parentContext(clipRectsContext);
            parentContext.overlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize; // FIXME: why?
            parentLayer->calculateClipRects(parentContext, clipRects);
        }
    } else
        clipRects.reset(PaintInfo::infiniteRect());

    // A fixed object is essentially the root of its containing block hierarchy, so when
    // we encounter such an object, we reset our clip rects to the fixedClipRect.
    if (renderer()->style()->position() == FixedPosition) {
        clipRects.setPosClipRect(clipRects.fixedClipRect());
        clipRects.setOverflowClipRect(clipRects.fixedClipRect());
        clipRects.setFixed(true);
    } else if (renderer()->style()->hasInFlowPosition())
        clipRects.setPosClipRect(clipRects.overflowClipRect());
    else if (renderer()->style()->position() == AbsolutePosition)
        clipRects.setOverflowClipRect(clipRects.posClipRect());
    
    // Update the clip rects that will be passed to child layers.
    if ((renderer()->hasOverflowClip() && (clipRectsContext.respectOverflowClip == RespectOverflowClip || this != clipRectsContext.rootLayer)) || renderer()->hasClip()) {
        // This layer establishes a clip of some kind.

        // This offset cannot use convertToLayerCoords, because sometimes our rootLayer may be across
        // some transformed layer boundary, for example, in the RenderLayerCompositor overlapMap, where
        // clipRects are needed in view space.
        LayoutPoint offset;
        offset = roundedLayoutPoint(renderer()->localToContainerPoint(FloatPoint(), clipRectsContext.rootLayer->renderer()));
        RenderView* view = renderer()->view();
        ASSERT(view);
        if (view && clipRects.fixed() && clipRectsContext.rootLayer->renderer() == view) {
            offset -= view->frameView()->scrollOffsetForFixedPosition();
        }
        
        if (renderer()->hasOverflowClip()) {
            ClipRect newOverflowClip = toRenderBox(renderer())->overflowClipRectForChildLayers(offset, clipRectsContext.region, clipRectsContext.overlayScrollbarSizeRelevancy);
            if (renderer()->style()->hasBorderRadius())
                newOverflowClip.setHasRadius(true);
            clipRects.setOverflowClipRect(intersection(newOverflowClip, clipRects.overflowClipRect()));
            if (renderer()->isPositioned())
                clipRects.setPosClipRect(intersection(newOverflowClip, clipRects.posClipRect()));
        }
        if (renderer()->hasClip()) {
            LayoutRect newPosClip = toRenderBox(renderer())->clipRect(offset, clipRectsContext.region);
            clipRects.setPosClipRect(intersection(newPosClip, clipRects.posClipRect()));
            clipRects.setOverflowClipRect(intersection(newPosClip, clipRects.overflowClipRect()));
            clipRects.setFixedClipRect(intersection(newPosClip, clipRects.fixedClipRect()));
        }
    }
}

void RenderLayer::parentClipRects(const ClipRectsContext& clipRectsContext, ClipRects& clipRects) const
{
    ASSERT(parent());
    if (clipRectsContext.clipRectsType == TemporaryClipRects) {
        parent()->calculateClipRects(clipRectsContext, clipRects);
        return;
    }

    parent()->updateClipRects(clipRectsContext);
    clipRects = *parent()->clipRects(clipRectsContext);
}

static inline ClipRect backgroundClipRectForPosition(const ClipRects& parentRects, EPosition position)
{
    if (position == FixedPosition)
        return parentRects.fixedClipRect();

    if (position == AbsolutePosition)
        return parentRects.posClipRect();

    return parentRects.overflowClipRect();
}

ClipRect RenderLayer::backgroundClipRect(const ClipRectsContext& clipRectsContext) const
{
    ASSERT(parent());
    
    ClipRects parentRects;

    // If we cross into a different pagination context, then we can't rely on the cache.
    // Just switch over to using TemporaryClipRects.
    if (clipRectsContext.clipRectsType != TemporaryClipRects && parent()->enclosingPaginationLayer() != enclosingPaginationLayer()) {
        ClipRectsContext tempContext(clipRectsContext);
        tempContext.clipRectsType = TemporaryClipRects;
        parentClipRects(tempContext, parentRects);
    } else
        parentClipRects(clipRectsContext, parentRects);
    
    ClipRect backgroundClipRect = backgroundClipRectForPosition(parentRects, renderer()->style()->position());
    RenderView* view = renderer()->view();
    ASSERT(view);

    // Note: infinite clipRects should not be scrolled here, otherwise they will accidentally no longer be considered infinite.
    if (parentRects.fixed() && clipRectsContext.rootLayer->renderer() == view && backgroundClipRect != PaintInfo::infiniteRect())
        backgroundClipRect.move(view->frameView()->scrollOffsetForFixedPosition());

    return backgroundClipRect;
}

void RenderLayer::calculateRects(const ClipRectsContext& clipRectsContext, const LayoutRect& paintDirtyRect, LayoutRect& layerBounds,
    ClipRect& backgroundRect, ClipRect& foregroundRect, ClipRect& outlineRect, const LayoutPoint* offsetFromRoot) const
{
    if (clipRectsContext.rootLayer != this && parent()) {
        backgroundRect = backgroundClipRect(clipRectsContext);
        backgroundRect.intersect(paintDirtyRect);
    } else
        backgroundRect = paintDirtyRect;

    foregroundRect = backgroundRect;
    outlineRect = backgroundRect;
    
    LayoutPoint offset;
    if (offsetFromRoot)
        offset = *offsetFromRoot;
    else
        convertToLayerCoords(clipRectsContext.rootLayer, offset);
    layerBounds = LayoutRect(offset, size());

    // Update the clip rects that will be passed to child layers.
    if (renderer()->hasClipOrOverflowClip()) {
        // This layer establishes a clip of some kind.
        if (renderer()->hasOverflowClip() && (this != clipRectsContext.rootLayer || clipRectsContext.respectOverflowClip == RespectOverflowClip)) {
            foregroundRect.intersect(toRenderBox(renderer())->overflowClipRect(offset, clipRectsContext.region, clipRectsContext.overlayScrollbarSizeRelevancy));
            if (renderer()->style()->hasBorderRadius())
                foregroundRect.setHasRadius(true);
        }

        if (renderer()->hasClip()) {
            // Clip applies to *us* as well, so go ahead and update the damageRect.
            LayoutRect newPosClip = toRenderBox(renderer())->clipRect(offset, clipRectsContext.region);
            backgroundRect.intersect(newPosClip);
            foregroundRect.intersect(newPosClip);
            outlineRect.intersect(newPosClip);
        }

        // If we establish a clip at all, then go ahead and make sure our background
        // rect is intersected with our layer's bounds including our visual overflow,
        // since any visual overflow like box-shadow or border-outset is not clipped by overflow:auto/hidden.
        if (renderBox()->hasVisualOverflow()) {
            // FIXME: Does not do the right thing with CSS regions yet, since we don't yet factor in the
            // individual region boxes as overflow.
            LayoutRect layerBoundsWithVisualOverflow = renderBox()->visualOverflowRect();
            renderBox()->flipForWritingMode(layerBoundsWithVisualOverflow); // Layers are in physical coordinates, so the overflow has to be flipped.
            layerBoundsWithVisualOverflow.moveBy(offset);
            if (this != clipRectsContext.rootLayer || clipRectsContext.respectOverflowClip == RespectOverflowClip)
                backgroundRect.intersect(layerBoundsWithVisualOverflow);
        } else {
            // Shift the bounds to be for our region only.
            LayoutRect bounds = renderBox()->borderBoxRectInRegion(clipRectsContext.region);
            bounds.moveBy(offset);
            if (this != clipRectsContext.rootLayer || clipRectsContext.respectOverflowClip == RespectOverflowClip)
                backgroundRect.intersect(bounds);
        }
    }
}

LayoutRect RenderLayer::childrenClipRect() const
{
    // FIXME: border-radius not accounted for.
    // FIXME: Regions not accounted for.
    RenderView* renderView = renderer()->view();
    RenderLayer* clippingRootLayer = clippingRootForPainting();
    LayoutRect layerBounds;
    ClipRect backgroundRect, foregroundRect, outlineRect;
    ClipRectsContext clipRectsContext(clippingRootLayer, 0, TemporaryClipRects);
    // Need to use temporary clip rects, because the value of 'dontClipToOverflow' may be different from the painting path (<rdar://problem/11844909>).
    calculateRects(clipRectsContext, renderView->unscaledDocumentRect(), layerBounds, backgroundRect, foregroundRect, outlineRect);
    return clippingRootLayer->renderer()->localToAbsoluteQuad(FloatQuad(foregroundRect.rect())).enclosingBoundingBox();
}

LayoutRect RenderLayer::selfClipRect() const
{
    // FIXME: border-radius not accounted for.
    // FIXME: Regions not accounted for.
    RenderView* renderView = renderer()->view();
    RenderLayer* clippingRootLayer = clippingRootForPainting();
    LayoutRect layerBounds;
    ClipRect backgroundRect, foregroundRect, outlineRect;
    ClipRectsContext clipRectsContext(clippingRootLayer, 0, PaintingClipRects);
    calculateRects(clipRectsContext, renderView->documentRect(), layerBounds, backgroundRect, foregroundRect, outlineRect);
    return clippingRootLayer->renderer()->localToAbsoluteQuad(FloatQuad(backgroundRect.rect())).enclosingBoundingBox();
}

LayoutRect RenderLayer::localClipRect() const
{
    // FIXME: border-radius not accounted for.
    // FIXME: Regions not accounted for.
    RenderLayer* clippingRootLayer = clippingRootForPainting();
    LayoutRect layerBounds;
    ClipRect backgroundRect, foregroundRect, outlineRect;
    ClipRectsContext clipRectsContext(clippingRootLayer, 0, PaintingClipRects);
    calculateRects(clipRectsContext, PaintInfo::infiniteRect(), layerBounds, backgroundRect, foregroundRect, outlineRect);

    LayoutRect clipRect = backgroundRect.rect();
    if (clipRect == PaintInfo::infiniteRect())
        return clipRect;

    LayoutPoint clippingRootOffset;
    convertToLayerCoords(clippingRootLayer, clippingRootOffset);
    clipRect.moveBy(-clippingRootOffset);

    return clipRect;
}

void RenderLayer::addBlockSelectionGapsBounds(const LayoutRect& bounds)
{
    m_blockSelectionGapsBounds.unite(enclosingIntRect(bounds));
}

void RenderLayer::clearBlockSelectionGapsBounds()
{
    m_blockSelectionGapsBounds = IntRect();
    for (RenderLayer* child = firstChild(); child; child = child->nextSibling())
        child->clearBlockSelectionGapsBounds();
}

void RenderLayer::repaintBlockSelectionGaps()
{
    for (RenderLayer* child = firstChild(); child; child = child->nextSibling())
        child->repaintBlockSelectionGaps();

    if (m_blockSelectionGapsBounds.isEmpty())
        return;

    LayoutRect rect = m_blockSelectionGapsBounds;
    rect.move(-scrolledContentOffset());
    if (renderer()->hasOverflowClip() && !usesCompositedScrolling())
        rect.intersect(toRenderBox(renderer())->overflowClipRect(LayoutPoint(), 0)); // FIXME: Regions not accounted for.
    if (renderer()->hasClip())
        rect.intersect(toRenderBox(renderer())->clipRect(LayoutPoint(), 0)); // FIXME: Regions not accounted for.
    if (!rect.isEmpty())
        renderer()->repaintRectangle(rect);
}

bool RenderLayer::intersectsDamageRect(const LayoutRect& layerBounds, const LayoutRect& damageRect, const RenderLayer* rootLayer, const LayoutPoint* offsetFromRoot) const
{
    // Always examine the canvas and the root.
    // FIXME: Could eliminate the isRoot() check if we fix background painting so that the RenderView
    // paints the root's background.
    if (isRootLayer() || renderer()->isRoot())
        return true;

    // If we aren't an inline flow, and our layer bounds do intersect the damage rect, then we 
    // can go ahead and return true.
    RenderView* view = renderer()->view();
    ASSERT(view);
    if (view && !renderer()->isRenderInline()) {
        LayoutRect b = layerBounds;
        b.inflate(view->maximalOutlineSize());
        if (b.intersects(damageRect))
            return true;
    }
        
    // Otherwise we need to compute the bounding box of this single layer and see if it intersects
    // the damage rect.
    return boundingBox(rootLayer, 0, offsetFromRoot).intersects(damageRect);
}

LayoutRect RenderLayer::localBoundingBox(CalculateLayerBoundsFlags flags) const
{
    // There are three special cases we need to consider.
    // (1) Inline Flows.  For inline flows we will create a bounding box that fully encompasses all of the lines occupied by the
    // inline.  In other words, if some <span> wraps to three lines, we'll create a bounding box that fully encloses the
    // line boxes of all three lines (including overflow on those lines).
    // (2) Left/Top Overflow.  The width/height of layers already includes right/bottom overflow.  However, in the case of left/top
    // overflow, we have to create a bounding box that will extend to include this overflow.
    // (3) Floats.  When a layer has overhanging floats that it paints, we need to make sure to include these overhanging floats
    // as part of our bounding box.  We do this because we are the responsible layer for both hit testing and painting those
    // floats.
    LayoutRect result;
    if (renderer()->isInline() && renderer()->isRenderInline())
        result = toRenderInline(renderer())->linesVisualOverflowBoundingBox();
    else if (renderer()->isTableRow()) {
        // Our bounding box is just the union of all of our cells' border/overflow rects.
        for (RenderObject* child = renderer()->firstChild(); child; child = child->nextSibling()) {
            if (child->isTableCell()) {
                LayoutRect bbox = toRenderBox(child)->borderBoxRect();
                result.unite(bbox);
                LayoutRect overflowRect = renderBox()->visualOverflowRect();
                if (bbox != overflowRect)
                    result.unite(overflowRect);
            }
        }
    } else {
        RenderBox* box = renderBox();
        ASSERT(box);
        if (!(flags & DontConstrainForMask) && box->hasMask()) {
            result = box->maskClipRect();
            box->flipForWritingMode(result); // The mask clip rect is in physical coordinates, so we have to flip, since localBoundingBox is not.
        } else {
            LayoutRect bbox = box->borderBoxRect();
            result = bbox;
            LayoutRect overflowRect = box->visualOverflowRect();
            if (bbox != overflowRect)
                result.unite(overflowRect);
        }
    }

    RenderView* view = renderer()->view();
    ASSERT(view);
    if (view)
        result.inflate(view->maximalOutlineSize()); // Used to apply a fudge factor to dirty-rect checks on blocks/tables.

    return result;
}

LayoutRect RenderLayer::boundingBox(const RenderLayer* ancestorLayer, CalculateLayerBoundsFlags flags, const LayoutPoint* offsetFromRoot) const
{    
    LayoutRect result = localBoundingBox(flags);
    if (renderer()->isBox())
        renderBox()->flipForWritingMode(result);
    else
        renderer()->containingBlock()->flipForWritingMode(result);

    if (enclosingPaginationLayer() && (flags & UseFragmentBoxes)) {
        // Split our box up into the actual fragment boxes that render in the columns/pages and unite those together to
        // get our true bounding box.
        LayoutPoint offsetWithinPaginationLayer;
        convertToLayerCoords(enclosingPaginationLayer(), offsetWithinPaginationLayer);        
        result.moveBy(offsetWithinPaginationLayer);

        RenderFlowThread* enclosingFlowThread = toRenderFlowThread(enclosingPaginationLayer()->renderer());
        result = enclosingFlowThread->fragmentsBoundingBox(result);
        
        LayoutPoint delta;
        if (offsetFromRoot)
            delta = *offsetFromRoot;
        else
            enclosingPaginationLayer()->convertToLayerCoords(ancestorLayer, delta);
        result.moveBy(delta);
        return result;
    }

    LayoutPoint delta;
    if (offsetFromRoot)
        delta = *offsetFromRoot;
    else
        convertToLayerCoords(ancestorLayer, delta);
    
    result.moveBy(delta);
    return result;
}

IntRect RenderLayer::absoluteBoundingBox() const
{
    return pixelSnappedIntRect(boundingBox(root()));
}

LayoutRect RenderLayer::calculateLayerBounds(const RenderLayer* ancestorLayer, const LayoutPoint* offsetFromRoot, CalculateLayerBoundsFlags flags) const
{
    if (!isSelfPaintingLayer())
        return LayoutRect();

    // FIXME: This could be improved to do a check like hasVisibleNonCompositingDescendantLayers() (bug 92580).
    if ((flags & ExcludeHiddenDescendants) && this != ancestorLayer && !hasVisibleContent() && !hasVisibleDescendant())
        return LayoutRect();

    RenderLayerModelObject* renderer = this->renderer();

    if (isRootLayer()) {
        // The root layer is always just the size of the document.
        return renderer->view()->unscaledDocumentRect();
    }

    LayoutRect boundingBoxRect = localBoundingBox(flags);

    if (renderer->isBox())
        toRenderBox(renderer)->flipForWritingMode(boundingBoxRect);
    else
        renderer->containingBlock()->flipForWritingMode(boundingBoxRect);

    if (renderer->isRoot()) {
        // If the root layer becomes composited (e.g. because some descendant with negative z-index is composited),
        // then it has to be big enough to cover the viewport in order to display the background. This is akin
        // to the code in RenderBox::paintRootBoxFillLayers().
        if (FrameView* frameView = renderer->view()->frameView()) {
            LayoutUnit contentsWidth = frameView->contentsWidth();
            LayoutUnit contentsHeight = frameView->contentsHeight();
        
            boundingBoxRect.setWidth(max(boundingBoxRect.width(), contentsWidth - boundingBoxRect.x()));
            boundingBoxRect.setHeight(max(boundingBoxRect.height(), contentsHeight - boundingBoxRect.y()));
        }
    }

    LayoutRect unionBounds = boundingBoxRect;

    if (flags & UseLocalClipRectIfPossible) {
        LayoutRect localClipRect = this->localClipRect();
        if (localClipRect != PaintInfo::infiniteRect()) {
            if ((flags & IncludeSelfTransform) && paintsWithTransform(PaintBehaviorNormal))
                localClipRect = transform()->mapRect(localClipRect);

            LayoutPoint ancestorRelOffset;
            convertToLayerCoords(ancestorLayer, ancestorRelOffset);
            localClipRect.moveBy(ancestorRelOffset);
            return localClipRect;
        }
    }

    // FIXME: should probably just pass 'flags' down to descendants.
    CalculateLayerBoundsFlags descendantFlags = DefaultCalculateLayerBoundsFlags | (flags & ExcludeHiddenDescendants) | (flags & IncludeCompositedDescendants);

    const_cast<RenderLayer*>(this)->updateLayerListsIfNeeded();

    if (RenderLayer* reflection = reflectionLayer()) {
        if (!reflection->isComposited()) {
            LayoutRect childUnionBounds = reflection->calculateLayerBounds(this, 0, descendantFlags);
            unionBounds.unite(childUnionBounds);
        }
    }
    
    ASSERT(isStackingContainer() || (!posZOrderList() || !posZOrderList()->size()));

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(const_cast<RenderLayer*>(this));
#endif

    if (Vector<RenderLayer*>* negZOrderList = this->negZOrderList()) {
        size_t listSize = negZOrderList->size();
        for (size_t i = 0; i < listSize; ++i) {
            RenderLayer* curLayer = negZOrderList->at(i);
            if (flags & IncludeCompositedDescendants || !curLayer->isComposited()) {
                LayoutRect childUnionBounds = curLayer->calculateLayerBounds(this, 0, descendantFlags);
                unionBounds.unite(childUnionBounds);
            }
        }
    }

    if (Vector<RenderLayer*>* posZOrderList = this->posZOrderList()) {
        size_t listSize = posZOrderList->size();
        for (size_t i = 0; i < listSize; ++i) {
            RenderLayer* curLayer = posZOrderList->at(i);
            if (flags & IncludeCompositedDescendants || !curLayer->isComposited()) {
                LayoutRect childUnionBounds = curLayer->calculateLayerBounds(this, 0, descendantFlags);
                unionBounds.unite(childUnionBounds);
            }
        }
    }

    if (Vector<RenderLayer*>* normalFlowList = this->normalFlowList()) {
        size_t listSize = normalFlowList->size();
        for (size_t i = 0; i < listSize; ++i) {
            RenderLayer* curLayer = normalFlowList->at(i);
            // RenderView will always return the size of the document, before reaching this point,
            // so there's no way we could hit a RenderNamedFlowThread here.
            ASSERT(!curLayer->isOutOfFlowRenderFlowThread());
            if (flags & IncludeCompositedDescendants || !curLayer->isComposited()) {
                LayoutRect curAbsBounds = curLayer->calculateLayerBounds(this, 0, descendantFlags);
                unionBounds.unite(curAbsBounds);
            }
        }
    }
    
#if ENABLE(CSS_FILTERS)
    // FIXME: We can optimize the size of the composited layers, by not enlarging
    // filtered areas with the outsets if we know that the filter is going to render in hardware.
    // https://bugs.webkit.org/show_bug.cgi?id=81239
    if (flags & IncludeLayerFilterOutsets)
        renderer->style()->filterOutsets().expandRect(unionBounds);
#endif

    if ((flags & IncludeSelfTransform) && paintsWithTransform(PaintBehaviorNormal)) {
        TransformationMatrix* affineTrans = transform();
        boundingBoxRect = affineTrans->mapRect(boundingBoxRect);
        unionBounds = affineTrans->mapRect(unionBounds);
    }

    LayoutPoint ancestorRelOffset;
    if (offsetFromRoot)
        ancestorRelOffset = *offsetFromRoot;
    else
        convertToLayerCoords(ancestorLayer, ancestorRelOffset);
    unionBounds.moveBy(ancestorRelOffset);
    
    return unionBounds;
}

void RenderLayer::clearClipRectsIncludingDescendants(ClipRectsType typeToClear)
{
    // FIXME: it's not clear how this layer not having clip rects guarantees that no descendants have any.
    if (!m_clipRectsCache)
        return;

    clearClipRects(typeToClear);
    
    for (RenderLayer* l = firstChild(); l; l = l->nextSibling())
        l->clearClipRectsIncludingDescendants(typeToClear);
}

void RenderLayer::clearClipRects(ClipRectsType typeToClear)
{
    if (typeToClear == AllClipRectTypes)
        m_clipRectsCache = nullptr;
    else {
        ASSERT(typeToClear < NumCachedClipRectsTypes);
        RefPtr<ClipRects> dummy;
        m_clipRectsCache->setClipRects(typeToClear, RespectOverflowClip, dummy);
        m_clipRectsCache->setClipRects(typeToClear, IgnoreOverflowClip, dummy);
    }
}

#if USE(ACCELERATED_COMPOSITING)
RenderLayerBacking* RenderLayer::ensureBacking()
{
    if (!m_backing) {
        m_backing = adoptPtr(new RenderLayerBacking(this));
        compositor()->layerBecameComposited(this);

#if ENABLE(CSS_FILTERS)
        updateOrRemoveFilterEffectRenderer();
#endif
#if ENABLE(CSS_COMPOSITING)
        backing()->setBlendMode(m_blendMode);
#endif
    }
    return m_backing.get();
}

void RenderLayer::clearBacking(bool layerBeingDestroyed)
{
    if (m_backing && !renderer()->documentBeingDestroyed())
        compositor()->layerBecameNonComposited(this);
    m_backing.clear();

#if ENABLE(CSS_FILTERS)
    if (!layerBeingDestroyed)
        updateOrRemoveFilterEffectRenderer();
#else
    UNUSED_PARAM(layerBeingDestroyed);
#endif
}

bool RenderLayer::hasCompositedMask() const
{
    return m_backing && m_backing->hasMaskLayer();
}

GraphicsLayer* RenderLayer::layerForScrolling() const
{
    return m_backing ? m_backing->scrollingContentsLayer() : 0;
}

GraphicsLayer* RenderLayer::layerForHorizontalScrollbar() const
{
    return m_backing ? m_backing->layerForHorizontalScrollbar() : 0;
}

GraphicsLayer* RenderLayer::layerForVerticalScrollbar() const
{
    return m_backing ? m_backing->layerForVerticalScrollbar() : 0;
}

GraphicsLayer* RenderLayer::layerForScrollCorner() const
{
    return m_backing ? m_backing->layerForScrollCorner() : 0;
}
#endif

bool RenderLayer::paintsWithTransform(PaintBehavior paintBehavior) const
{
#if USE(ACCELERATED_COMPOSITING)
    bool paintsToWindow = !isComposited() || backing()->paintsIntoWindow();
#else
    bool paintsToWindow = true;
#endif    
    return transform() && ((paintBehavior & PaintBehaviorFlattenCompositingLayers) || paintsToWindow);
}

bool RenderLayer::backgroundIsKnownToBeOpaqueInRect(const LayoutRect& localRect) const
{
    if (!isSelfPaintingLayer() && !hasSelfPaintingLayerDescendant())
        return false;

    if (paintsWithTransparency(PaintBehaviorNormal))
        return false;

    // We can't use hasVisibleContent(), because that will be true if our renderer is hidden, but some child
    // is visible and that child doesn't cover the entire rect.
    if (renderer()->style()->visibility() != VISIBLE)
        return false;

#if ENABLE(CSS_FILTERS)
    if (paintsWithFilters() && renderer()->style()->filter().hasFilterThatAffectsOpacity())
        return false;
#endif

    // FIXME: Handle simple transforms.
    if (paintsWithTransform(PaintBehaviorNormal))
        return false;

    // FIXME: Remove this check.
    // This function should not be called when layer-lists are dirty.
    // It is somehow getting triggered during style update.
    if (m_zOrderListsDirty || m_normalFlowListDirty)
        return false;

    // FIXME: We currently only check the immediate renderer,
    // which will miss many cases.
    if (renderer()->backgroundIsKnownToBeOpaqueInRect(localRect))
        return true;
    
    // We can't consult child layers if we clip, since they might cover
    // parts of the rect that are clipped out.
    if (renderer()->hasOverflowClip())
        return false;
    
    return listBackgroundIsKnownToBeOpaqueInRect(posZOrderList(), localRect)
        || listBackgroundIsKnownToBeOpaqueInRect(negZOrderList(), localRect)
        || listBackgroundIsKnownToBeOpaqueInRect(normalFlowList(), localRect);
}

bool RenderLayer::listBackgroundIsKnownToBeOpaqueInRect(const Vector<RenderLayer*>* list, const LayoutRect& localRect) const
{
    if (!list || list->isEmpty())
        return false;

    for (Vector<RenderLayer*>::const_reverse_iterator iter = list->rbegin(); iter != list->rend(); ++iter) {
        const RenderLayer* childLayer = *iter;
        if (childLayer->isComposited())
            continue;

        if (!childLayer->canUseConvertToLayerCoords())
            continue;

        LayoutPoint childOffset;
        LayoutRect childLocalRect(localRect);
        childLayer->convertToLayerCoords(this, childOffset);
        childLocalRect.moveBy(-childOffset);

        if (childLayer->backgroundIsKnownToBeOpaqueInRect(childLocalRect))
            return true;
    }
    return false;
}

void RenderLayer::setParent(RenderLayer* parent)
{
    if (parent == m_parent)
        return;

#if USE(ACCELERATED_COMPOSITING)
    if (m_parent && !renderer()->documentBeingDestroyed())
        compositor()->layerWillBeRemoved(m_parent, this);
#endif
    
    m_parent = parent;
    
#if USE(ACCELERATED_COMPOSITING)
    if (m_parent && !renderer()->documentBeingDestroyed())
        compositor()->layerWasAdded(m_parent, this);
#endif
}

// Helper for the sorting of layers by z-index.
static inline bool compareZIndex(RenderLayer* first, RenderLayer* second)
{
    return first->zIndex() < second->zIndex();
}

void RenderLayer::dirtyZOrderLists()
{
    ASSERT(m_layerListMutationAllowed);
    ASSERT(isStackingContainer());

    if (m_posZOrderList)
        m_posZOrderList->clear();
    if (m_negZOrderList)
        m_negZOrderList->clear();
    m_zOrderListsDirty = true;

#if USE(ACCELERATED_COMPOSITING)
    if (!renderer()->documentBeingDestroyed()) {
        compositor()->setCompositingLayersNeedRebuild();
        if (acceleratedCompositingForOverflowScrollEnabled())
            compositor()->setShouldReevaluateCompositingAfterLayout();
    }
#endif
}

void RenderLayer::dirtyStackingContainerZOrderLists()
{
    RenderLayer* sc = stackingContainer();
    if (sc)
        sc->dirtyZOrderLists();
}

void RenderLayer::dirtyNormalFlowList()
{
    ASSERT(m_layerListMutationAllowed);

    if (m_normalFlowList)
        m_normalFlowList->clear();
    m_normalFlowListDirty = true;

#if USE(ACCELERATED_COMPOSITING)
    if (!renderer()->documentBeingDestroyed()) {
        compositor()->setCompositingLayersNeedRebuild();
        if (acceleratedCompositingForOverflowScrollEnabled())
            compositor()->setShouldReevaluateCompositingAfterLayout();
    }
#endif
}

void RenderLayer::rebuildZOrderLists()
{
    ASSERT(m_layerListMutationAllowed);
    ASSERT(isDirtyStackingContainer());
    rebuildZOrderLists(StopAtStackingContainers, m_posZOrderList, m_negZOrderList);
    m_zOrderListsDirty = false;
}

void RenderLayer::rebuildZOrderLists(CollectLayersBehavior behavior, OwnPtr<Vector<RenderLayer*> >& posZOrderList, OwnPtr<Vector<RenderLayer*> >& negZOrderList)
{
#if USE(ACCELERATED_COMPOSITING)
    bool includeHiddenLayers = compositor()->inCompositingMode();
#else
    bool includeHiddenLayers = false;
#endif
    for (RenderLayer* child = firstChild(); child; child = child->nextSibling())
        if (!m_reflection || reflectionLayer() != child)
            child->collectLayers(includeHiddenLayers, behavior, posZOrderList, negZOrderList);

    // Sort the two lists.
    if (posZOrderList)
        std::stable_sort(posZOrderList->begin(), posZOrderList->end(), compareZIndex);

    if (negZOrderList)
        std::stable_sort(negZOrderList->begin(), negZOrderList->end(), compareZIndex);

#if ENABLE(DIALOG_ELEMENT)
    // Append layers for top layer elements after normal layer collection, to ensure they are on top regardless of z-indexes.
    // The renderers of top layer elements are children of the view, sorted in top layer stacking order.
    if (isRootLayer()) {
        RenderObject* view = renderer()->view();
        for (RenderObject* child = view->firstChild(); child; child = child->nextSibling()) {
            Element* childElement = (child->node() && child->node()->isElementNode()) ? toElement(child->node()) : 0;
            if (childElement && childElement->isInTopLayer()) {
                RenderLayer* layer = toRenderLayerModelObject(child)->layer();
                posZOrderList->append(layer);
            }
        }
    }
#endif

}

void RenderLayer::updateNormalFlowList()
{
    if (!m_normalFlowListDirty)
        return;

    ASSERT(m_layerListMutationAllowed);

    for (RenderLayer* child = firstChild(); child; child = child->nextSibling()) {
        // Ignore non-overflow layers and reflections.
        if (child->isNormalFlowOnly() && (!m_reflection || reflectionLayer() != child)) {
            if (!m_normalFlowList)
                m_normalFlowList = adoptPtr(new Vector<RenderLayer*>);
            m_normalFlowList->append(child);
        }
    }
    
    m_normalFlowListDirty = false;
}

void RenderLayer::collectLayers(bool includeHiddenLayers, CollectLayersBehavior behavior, OwnPtr<Vector<RenderLayer*> >& posBuffer, OwnPtr<Vector<RenderLayer*> >& negBuffer)
{
#if ENABLE(DIALOG_ELEMENT)
    if (isInTopLayer())
        return;
#endif

    updateDescendantDependentFlags();

    bool isStacking = behavior == StopAtStackingContexts ? isStackingContext() : isStackingContainer();
    // Overflow layers are just painted by their enclosing layers, so they don't get put in zorder lists.
    bool includeHiddenLayer = includeHiddenLayers || (m_hasVisibleContent || (m_hasVisibleDescendant && isStacking));
    if (includeHiddenLayer && !isNormalFlowOnly()) {
        // Determine which buffer the child should be in.
        OwnPtr<Vector<RenderLayer*> >& buffer = (zIndex() >= 0) ? posBuffer : negBuffer;

        // Create the buffer if it doesn't exist yet.
        if (!buffer)
            buffer = adoptPtr(new Vector<RenderLayer*>);
        
        // Append ourselves at the end of the appropriate buffer.
        buffer->append(this);
    }

    // Recur into our children to collect more layers, but only if we don't establish
    // a stacking context/container.
    if ((includeHiddenLayers || m_hasVisibleDescendant) && !isStacking) {
        for (RenderLayer* child = firstChild(); child; child = child->nextSibling()) {
            // Ignore reflections.
            if (!m_reflection || reflectionLayer() != child)
                child->collectLayers(includeHiddenLayers, behavior, posBuffer, negBuffer);
        }
    }
}

void RenderLayer::updateLayerListsIfNeeded()
{
    bool shouldUpdateDescendantsAreContiguousInStackingOrder = isStackingContext() && (m_zOrderListsDirty || m_normalFlowListDirty);
    updateZOrderLists();
    updateNormalFlowList();

    if (RenderLayer* reflectionLayer = this->reflectionLayer()) {
        reflectionLayer->updateZOrderLists();
        reflectionLayer->updateNormalFlowList();
    }

    if (shouldUpdateDescendantsAreContiguousInStackingOrder) {
        updateDescendantsAreContiguousInStackingOrder();
        // The above function can cause us to update m_needsCompositedScrolling
        // and dirty our layer lists. Refresh them if necessary.
        updateZOrderLists();
        updateNormalFlowList();
    }
}

void RenderLayer::updateCompositingAndLayerListsIfNeeded()
{
#if USE(ACCELERATED_COMPOSITING)
    if (compositor()->inCompositingMode()) {
        if (isDirtyStackingContainer() || m_normalFlowListDirty)
            compositor()->updateCompositingLayers(CompositingUpdateOnHitTest, this);
        return;
    }
#endif
    updateLayerListsIfNeeded();
}

void RenderLayer::repaintIncludingDescendants()
{
    renderer()->repaint();
    for (RenderLayer* curr = firstChild(); curr; curr = curr->nextSibling())
        curr->repaintIncludingDescendants();
}

#if USE(ACCELERATED_COMPOSITING)
void RenderLayer::setBackingNeedsRepaint()
{
    ASSERT(isComposited());
    if (backing()->paintsIntoWindow()) {
        // If we're trying to repaint the placeholder document layer, propagate the
        // repaint to the native view system.
        RenderView* view = renderer()->view();
        if (view)
            view->repaintViewRectangle(absoluteBoundingBox());
    } else
        backing()->setContentsNeedDisplay();
}

void RenderLayer::setBackingNeedsRepaintInRect(const LayoutRect& r)
{
    // https://bugs.webkit.org/show_bug.cgi?id=61159 describes an unreproducible crash here,
    // so assert but check that the layer is composited.
    ASSERT(isComposited());
    if (!isComposited() || backing()->paintsIntoWindow()) {
        // If we're trying to repaint the placeholder document layer, propagate the
        // repaint to the native view system.
        LayoutRect absRect(r);
        LayoutPoint delta;
        convertToLayerCoords(root(), delta);
        absRect.moveBy(delta);

        RenderView* view = renderer()->view();
        if (view)
            view->repaintViewRectangle(absRect);
    } else
        backing()->setContentsNeedDisplayInRect(pixelSnappedIntRect(r));
}

// Since we're only painting non-composited layers, we know that they all share the same repaintContainer.
void RenderLayer::repaintIncludingNonCompositingDescendants(RenderLayerModelObject* repaintContainer)
{
    renderer()->repaintUsingContainer(repaintContainer, pixelSnappedIntRect(renderer()->clippedOverflowRectForRepaint(repaintContainer)));

    for (RenderLayer* curr = firstChild(); curr; curr = curr->nextSibling()) {
        if (!curr->isComposited())
            curr->repaintIncludingNonCompositingDescendants(repaintContainer);
    }
}
#endif

bool RenderLayer::shouldBeNormalFlowOnly() const
{
    return (renderer()->hasOverflowClip()
                || renderer()->hasReflection()
                || renderer()->hasMask()
                || renderer()->isCanvas()
                || renderer()->isVideo()
                || renderer()->isEmbeddedObject()
                || renderer()->isRenderIFrame()
                || (renderer()->style()->specifiesColumns() && !isRootLayer()))
            && !renderer()->isPositioned()
            && !renderer()->hasTransform()
            && !renderer()->hasClipPath()
#if ENABLE(CSS_FILTERS)
            && !renderer()->hasFilter()
#endif
#if ENABLE(CSS_COMPOSITING)
            && !renderer()->hasBlendMode()
#endif
            && !isTransparent()
            && !needsCompositedScrolling()
            ;
}

bool RenderLayer::shouldBeSelfPaintingLayer() const
{
    return !isNormalFlowOnly()
        || hasOverlayScrollbars()
        || needsCompositedScrolling()
        || renderer()->hasReflection()
        || renderer()->hasMask()
        || renderer()->isTableRow()
        || renderer()->isCanvas()
        || renderer()->isVideo()
        || renderer()->isEmbeddedObject()
        || renderer()->isRenderIFrame();
}

void RenderLayer::updateSelfPaintingLayer()
{
    bool isSelfPaintingLayer = shouldBeSelfPaintingLayer();
    if (m_isSelfPaintingLayer == isSelfPaintingLayer)
        return;

    m_isSelfPaintingLayer = isSelfPaintingLayer;
    if (!parent())
        return;
    if (isSelfPaintingLayer)
        parent()->setAncestorChainHasSelfPaintingLayerDescendant();
    else
        parent()->dirtyAncestorChainHasSelfPaintingLayerDescendantStatus();
}

bool RenderLayer::hasNonEmptyChildRenderers() const
{
    // Some HTML can cause whitespace text nodes to have renderers, like:
    // <div>
    // <img src=...>
    // </div>
    // so test for 0x0 RenderTexts here
    for (RenderObject* child = renderer()->firstChild(); child; child = child->nextSibling()) {
        if (!child->hasLayer()) {
            if (child->isRenderInline() || !child->isBox())
                return true;
        
            if (toRenderBox(child)->width() > 0 || toRenderBox(child)->height() > 0)
                return true;
        }
    }
    return false;
}

static bool hasBoxDecorations(const RenderStyle* style)
{
    return style->hasBorder() || style->hasBorderRadius() || style->hasOutline() || style->hasAppearance() || style->boxShadow() || style->hasFilter();
}

bool RenderLayer::hasBoxDecorationsOrBackground() const
{
    return hasBoxDecorations(renderer()->style()) || renderer()->hasBackground();
}

bool RenderLayer::hasVisibleBoxDecorations() const
{
    if (!hasVisibleContent())
        return false;

    return hasBoxDecorationsOrBackground() || hasOverflowControls();
}

bool RenderLayer::isVisuallyNonEmpty() const
{
    ASSERT(!m_visibleDescendantStatusDirty);

    if (hasVisibleContent() && hasNonEmptyChildRenderers())
        return true;

    if (renderer()->isReplaced() || renderer()->hasMask())
        return true;

    if (hasVisibleBoxDecorations())
        return true;

    return false;
}

void RenderLayer::updateStackingContextsAfterStyleChange(const RenderStyle* oldStyle)
{
    if (!oldStyle)
        return;

    bool wasStackingContext = isStackingContext(oldStyle);
    bool isStackingContext = this->isStackingContext();
    if (isStackingContext != wasStackingContext) {
        dirtyStackingContainerZOrderLists();
        if (isStackingContext)
            dirtyZOrderLists();
        else
            clearZOrderLists();
        return;
    }

    // FIXME: RenderLayer already handles visibility changes through our visiblity dirty bits. This logic could
    // likely be folded along with the rest.
    if (oldStyle->zIndex() != renderer()->style()->zIndex() || oldStyle->visibility() != renderer()->style()->visibility()) {
        dirtyStackingContainerZOrderLists();
        if (isStackingContext)
            dirtyZOrderLists();
    }
}

static bool overflowRequiresScrollbar(EOverflow overflow)
{
    return overflow == OSCROLL;
}

static bool overflowDefinesAutomaticScrollbar(EOverflow overflow)
{
    return overflow == OAUTO || overflow == OOVERLAY;
}

void RenderLayer::updateScrollbarsAfterStyleChange(const RenderStyle* oldStyle)
{
    // Overflow are a box concept.
    RenderBox* box = renderBox();
    if (!box)
        return;

    // List box parts handle the scrollbars by themselves so we have nothing to do.
    if (box->style()->appearance() == ListboxPart)
        return;

    EOverflow overflowX = box->style()->overflowX();
    EOverflow overflowY = box->style()->overflowY();

    // To avoid doing a relayout in updateScrollbarsAfterLayout, we try to keep any automatic scrollbar that was already present.
    bool needsHorizontalScrollbar = (hasHorizontalScrollbar() && overflowDefinesAutomaticScrollbar(overflowX)) || overflowRequiresScrollbar(overflowX);
    bool needsVerticalScrollbar = (hasVerticalScrollbar() && overflowDefinesAutomaticScrollbar(overflowY)) || overflowRequiresScrollbar(overflowY);
    setHasHorizontalScrollbar(needsHorizontalScrollbar);
    setHasVerticalScrollbar(needsVerticalScrollbar);

    // With overflow: scroll, scrollbars are always visible but may be disabled.
    // When switching to another value, we need to re-enable them (see bug 11985).
    if (needsHorizontalScrollbar && oldStyle && oldStyle->overflowX() == OSCROLL && overflowX != OSCROLL) {
        ASSERT(hasHorizontalScrollbar());
        m_hBar->setEnabled(true);
    }

    if (needsVerticalScrollbar && oldStyle && oldStyle->overflowY() == OSCROLL && overflowY != OSCROLL) {
        ASSERT(hasVerticalScrollbar());
        m_vBar->setEnabled(true);
    }

    if (!m_scrollDimensionsDirty)
        updateScrollableAreaSet(hasScrollableHorizontalOverflow() || hasScrollableVerticalOverflow());
}

void RenderLayer::setAncestorChainHasOutOfFlowPositionedDescendant(RenderObject* containingBlock)
{
    for (RenderLayer* layer = this; layer; layer = layer->parent()) {
        if (!layer->m_hasOutOfFlowPositionedDescendantDirty && layer->hasOutOfFlowPositionedDescendant())
            break;

        layer->m_hasOutOfFlowPositionedDescendantDirty = false;
        layer->m_hasOutOfFlowPositionedDescendant = true;
#if USE(ACCELERATED_COMPOSITING)
        layer->updateNeedsCompositedScrolling();
#endif

        if (layer->renderer() && layer->renderer() == containingBlock)
            break;
    }
}

void RenderLayer::dirtyAncestorChainHasOutOfFlowPositionedDescendantStatus()
{
    m_hasOutOfFlowPositionedDescendantDirty = true;
    if (parent())
        parent()->dirtyAncestorChainHasOutOfFlowPositionedDescendantStatus();
}

void RenderLayer::updateOutOfFlowPositioned(const RenderStyle* oldStyle)
{
    bool wasOutOfFlowPositioned = oldStyle && (oldStyle->position() == AbsolutePosition || oldStyle->position() == FixedPosition);
    if (parent() && ((renderer() && renderer()->isOutOfFlowPositioned()) != wasOutOfFlowPositioned)) {
        parent()->dirtyAncestorChainHasOutOfFlowPositionedDescendantStatus();
#if USE(ACCELERATED_COMPOSITING)
        if (!renderer()->documentBeingDestroyed() && acceleratedCompositingForOverflowScrollEnabled())
            compositor()->setShouldReevaluateCompositingAfterLayout();
#endif
    }
}

#if USE(ACCELERATED_COMPOSITING)
inline bool RenderLayer::needsCompositingLayersRebuiltForClip(const RenderStyle* oldStyle, const RenderStyle* newStyle) const
{
    ASSERT(newStyle);
    return oldStyle && (oldStyle->clip() != newStyle->clip() || oldStyle->hasClip() != newStyle->hasClip());
}

inline bool RenderLayer::needsCompositingLayersRebuiltForOverflow(const RenderStyle* oldStyle, const RenderStyle* newStyle) const
{
    ASSERT(newStyle);
    return !isComposited() && oldStyle && (oldStyle->overflowX() != newStyle->overflowX()) && stackingContainer()->hasCompositingDescendant();
}
#endif // USE(ACCELERATED_COMPOSITING)

void RenderLayer::styleChanged(StyleDifference, const RenderStyle* oldStyle)
{
    bool isNormalFlowOnly = shouldBeNormalFlowOnly();
    if (isNormalFlowOnly != m_isNormalFlowOnly) {
        m_isNormalFlowOnly = isNormalFlowOnly;
        RenderLayer* p = parent();
        if (p)
            p->dirtyNormalFlowList();
        dirtyStackingContainerZOrderLists();
    }

    if (renderer()->style()->overflowX() == OMARQUEE && renderer()->style()->marqueeBehavior() != MNONE && renderer()->isBox()) {
        if (!m_marquee)
            m_marquee = adoptPtr(new RenderMarquee(this));
        FeatureObserver::observe(renderer()->document(), renderer()->isHTMLMarquee() ? FeatureObserver::HTMLMarqueeElement : FeatureObserver::CSSOverflowMarquee);
        m_marquee->updateMarqueeStyle();
    }
    else if (m_marquee) {
        m_marquee.clear();
    }

    updateScrollbarsAfterStyleChange(oldStyle);
    updateStackingContextsAfterStyleChange(oldStyle);
    // Overlay scrollbars can make this layer self-painting so we need
    // to recompute the bit once scrollbars have been updated.
    updateSelfPaintingLayer();
    updateOutOfFlowPositioned(oldStyle);

    if (!hasReflection() && m_reflection)
        removeReflection();
    else if (hasReflection()) {
        if (!m_reflection)
            createReflection();
        FeatureObserver::observe(renderer()->document(), FeatureObserver::Reflection);
        updateReflectionStyle();
    }
    
    // FIXME: Need to detect a swap from custom to native scrollbars (and vice versa).
    if (m_hBar)
        m_hBar->styleChanged();
    if (m_vBar)
        m_vBar->styleChanged();
    
    updateScrollCornerStyle();
    updateResizerStyle();

    updateDescendantDependentFlags();
    updateTransform();
#if ENABLE(CSS_COMPOSITING)
    updateBlendMode();
#endif
#if ENABLE(CSS_FILTERS)
    updateOrRemoveFilterClients();
#endif

#if USE(ACCELERATED_COMPOSITING)
    updateNeedsCompositedScrolling();

    const RenderStyle* newStyle = renderer()->style();
    if (compositor()->updateLayerCompositingState(this)
        || needsCompositingLayersRebuiltForClip(oldStyle, newStyle)
        || needsCompositingLayersRebuiltForOverflow(oldStyle, newStyle))
        compositor()->setCompositingLayersNeedRebuild();
    else if (isComposited())
        backing()->updateGraphicsLayerGeometry();
#endif

#if ENABLE(CSS_FILTERS)
    updateOrRemoveFilterEffectRenderer();
#if USE(ACCELERATED_COMPOSITING)
    bool backingDidCompositeLayers = isComposited() && backing()->canCompositeFilters();
    if (isComposited() && backingDidCompositeLayers && !backing()->canCompositeFilters()) {
        // The filters used to be drawn by platform code, but now the platform cannot draw them anymore.
        // Fallback to drawing them in software.
        setBackingNeedsRepaint();
    }
#endif
#endif
}

void RenderLayer::updateScrollableAreaSet(bool hasOverflow)
{
    Frame* frame = renderer()->frame();
    if (!frame)
        return;

    FrameView* frameView = frame->view();
    if (!frameView)
        return;

    bool isVisibleToHitTest = renderer()->visibleToHitTesting();
    if (HTMLFrameOwnerElement* owner = frame->ownerElement())
        isVisibleToHitTest &= owner->renderer() && owner->renderer()->visibleToHitTesting();

    bool isScrollable = hasOverflow && isVisibleToHitTest;
    bool addedOrRemoved = false;
    if (isScrollable)
        addedOrRemoved = frameView->addScrollableArea(this);
    else
        addedOrRemoved = frameView->removeScrollableArea(this);
    
    if (addedOrRemoved) {
#if USE(ACCELERATED_COMPOSITING)
        updateNeedsCompositedScrolling();
#endif
    }
}

void RenderLayer::updateScrollCornerStyle()
{
    RenderObject* actualRenderer = rendererForScrollbar(renderer());
    RefPtr<RenderStyle> corner = renderer()->hasOverflowClip() ? actualRenderer->getUncachedPseudoStyle(PseudoStyleRequest(SCROLLBAR_CORNER), actualRenderer->style()) : PassRefPtr<RenderStyle>(0);
    if (corner) {
        if (!m_scrollCorner) {
            m_scrollCorner = RenderScrollbarPart::createAnonymous(renderer()->document());
            m_scrollCorner->setParent(renderer());
        }
        m_scrollCorner->setStyle(corner.release());
    } else if (m_scrollCorner) {
        m_scrollCorner->destroy();
        m_scrollCorner = 0;
    }
}

void RenderLayer::updateResizerStyle()
{
    RenderObject* actualRenderer = rendererForScrollbar(renderer());
    RefPtr<RenderStyle> resizer = renderer()->hasOverflowClip() ? actualRenderer->getUncachedPseudoStyle(PseudoStyleRequest(RESIZER), actualRenderer->style()) : PassRefPtr<RenderStyle>(0);
    if (resizer) {
        if (!m_resizer) {
            m_resizer = RenderScrollbarPart::createAnonymous(renderer()->document());
            m_resizer->setParent(renderer());
        }
        m_resizer->setStyle(resizer.release());
    } else if (m_resizer) {
        m_resizer->destroy();
        m_resizer = 0;
    }
}

RenderLayer* RenderLayer::reflectionLayer() const
{
    return m_reflection ? m_reflection->layer() : 0;
}

void RenderLayer::createReflection()
{
    ASSERT(!m_reflection);
    m_reflection = RenderReplica::createAnonymous(renderer()->document());
    m_reflection->setParent(renderer()); // We create a 1-way connection.
}

void RenderLayer::removeReflection()
{
    if (!m_reflection->documentBeingDestroyed())
        m_reflection->removeLayers(this);

    m_reflection->setParent(0);
    m_reflection->destroy();
    m_reflection = 0;
}

void RenderLayer::updateReflectionStyle()
{
    RefPtr<RenderStyle> newStyle = RenderStyle::create();
    newStyle->inheritFrom(renderer()->style());
    
    // Map in our transform.
    TransformOperations transform;
    switch (renderer()->style()->boxReflect()->direction()) {
        case ReflectionBelow:
            transform.operations().append(TranslateTransformOperation::create(Length(0, Fixed), Length(100., Percent), TransformOperation::TRANSLATE));
            transform.operations().append(TranslateTransformOperation::create(Length(0, Fixed), renderer()->style()->boxReflect()->offset(), TransformOperation::TRANSLATE));
            transform.operations().append(ScaleTransformOperation::create(1.0, -1.0, ScaleTransformOperation::SCALE));
            break;
        case ReflectionAbove:
            transform.operations().append(ScaleTransformOperation::create(1.0, -1.0, ScaleTransformOperation::SCALE));
            transform.operations().append(TranslateTransformOperation::create(Length(0, Fixed), Length(100., Percent), TransformOperation::TRANSLATE));
            transform.operations().append(TranslateTransformOperation::create(Length(0, Fixed), renderer()->style()->boxReflect()->offset(), TransformOperation::TRANSLATE));
            break;
        case ReflectionRight:
            transform.operations().append(TranslateTransformOperation::create(Length(100., Percent), Length(0, Fixed), TransformOperation::TRANSLATE));
            transform.operations().append(TranslateTransformOperation::create(renderer()->style()->boxReflect()->offset(), Length(0, Fixed), TransformOperation::TRANSLATE));
            transform.operations().append(ScaleTransformOperation::create(-1.0, 1.0, ScaleTransformOperation::SCALE));
            break;
        case ReflectionLeft:
            transform.operations().append(ScaleTransformOperation::create(-1.0, 1.0, ScaleTransformOperation::SCALE));
            transform.operations().append(TranslateTransformOperation::create(Length(100., Percent), Length(0, Fixed), TransformOperation::TRANSLATE));
            transform.operations().append(TranslateTransformOperation::create(renderer()->style()->boxReflect()->offset(), Length(0, Fixed), TransformOperation::TRANSLATE));
            break;
    }
    newStyle->setTransform(transform);

    // Map in our mask.
    newStyle->setMaskBoxImage(renderer()->style()->boxReflect()->mask());
    
    m_reflection->setStyle(newStyle.release());
}

#if ENABLE(CSS_SHADERS)
bool RenderLayer::isCSSCustomFilterEnabled() const
{
    // We only want to enable shaders if WebGL is also enabled on this platform.
    const Settings* settings = renderer()->document()->settings();
    return settings && settings->isCSSCustomFilterEnabled() && settings->webGLEnabled();
}
#endif

#if ENABLE(CSS_FILTERS)
FilterOperations RenderLayer::computeFilterOperations(const RenderStyle* style)
{
#if !ENABLE(CSS_SHADERS)
    return style->filter();
#else
    const FilterOperations& filters = style->filter();
    if (!filters.hasCustomFilter())
        return filters;

    if (!isCSSCustomFilterEnabled()) {
        // CSS Custom filters should not parse at all in this case, but there might be
        // remaining styles that were parsed when the flag was enabled. Reproduces in DumpRenderTree
        // because it resets the flag while the previous test is still loaded.
        return FilterOperations();
    }

    FilterOperations outputFilters;
    for (size_t i = 0; i < filters.size(); ++i) {
        RefPtr<FilterOperation> filterOperation = filters.operations().at(i);
        if (filterOperation->getOperationType() == FilterOperation::CUSTOM) {
            // We have to wait until the program of CSS Shaders is loaded before setting it on the layer.
            // Note that we will handle the loading of the shaders and repainting of the layer in updateOrRemoveFilterClients.
            const CustomFilterOperation* customOperation = static_cast<const CustomFilterOperation*>(filterOperation.get());
            RefPtr<CustomFilterProgram> program = customOperation->program();
            if (!program->isLoaded())
                continue;

            RefPtr<CustomFilterValidatedProgram> validatedProgram = program->validatedProgram();
            if (!validatedProgram) {
                // Lazily create a validated program and store it on the CustomFilterProgram.
                CustomFilterGlobalContext* globalContext = renderer()->view()->customFilterGlobalContext();
                validatedProgram = CustomFilterValidatedProgram::create(globalContext, program->programInfo());
                program->setValidatedProgram(validatedProgram);
            }

            if (!validatedProgram->isInitialized())
                continue;

            RefPtr<ValidatedCustomFilterOperation> validatedOperation = ValidatedCustomFilterOperation::create(validatedProgram.release(), 
                customOperation->parameters(), customOperation->meshRows(), customOperation->meshColumns(), customOperation->meshType());
            outputFilters.operations().append(validatedOperation.release());
            continue;
        }
        outputFilters.operations().append(filterOperation.release());
    }
    return outputFilters;
#endif
}

void RenderLayer::updateOrRemoveFilterClients()
{
    if (!hasFilter()) {
        removeFilterInfoIfNeeded();
        return;
    }

#if ENABLE(CSS_SHADERS)
    if (renderer()->style()->filter().hasCustomFilter())
        ensureFilterInfo()->updateCustomFilterClients(renderer()->style()->filter());
    else if (hasFilterInfo())
        filterInfo()->removeCustomFilterClients();
#endif

#if ENABLE(SVG)
    if (renderer()->style()->filter().hasReferenceFilter())
        ensureFilterInfo()->updateReferenceFilterClients(renderer()->style()->filter());
    else if (hasFilterInfo())
        filterInfo()->removeReferenceFilterClients();
#endif
}

void RenderLayer::updateOrRemoveFilterEffectRenderer()
{
    // FilterEffectRenderer is only used to render the filters in software mode,
    // so we always need to run updateOrRemoveFilterEffectRenderer after the composited
    // mode might have changed for this layer.
    if (!paintsWithFilters()) {
        // Don't delete the whole filter info here, because we might use it
        // for loading CSS shader files.
        if (RenderLayerFilterInfo* filterInfo = this->filterInfo())
            filterInfo->setRenderer(0);

        // Early-return only if we *don't* have reference filters.
        // For reference filters, we still want the FilterEffect graph built
        // for us, even if we're composited.
        if (!renderer()->style()->filter().hasReferenceFilter())
            return;
    }
    
    RenderLayerFilterInfo* filterInfo = ensureFilterInfo();
    if (!filterInfo->renderer()) {
        RefPtr<FilterEffectRenderer> filterRenderer = FilterEffectRenderer::create();
        RenderingMode renderingMode = renderer()->frame()->page()->settings()->acceleratedFiltersEnabled() ? Accelerated : Unaccelerated;
        filterRenderer->setRenderingMode(renderingMode);
        filterInfo->setRenderer(filterRenderer.release());
        
        // We can optimize away code paths in other places if we know that there are no software filters.
        renderer()->document()->view()->setHasSoftwareFilters(true);
    }

    // If the filter fails to build, remove it from the layer. It will still attempt to
    // go through regular processing (e.g. compositing), but never apply anything.
    if (!filterInfo->renderer()->build(renderer(), computeFilterOperations(renderer()->style())))
        filterInfo->setRenderer(0);
}

void RenderLayer::filterNeedsRepaint()
{
    renderer()->node()->setNeedsStyleRecalc(SyntheticStyleChange);
    if (renderer()->view())
        renderer()->repaint();
}
#endif

} // namespace WebCore

#ifndef NDEBUG
void showLayerTree(const WebCore::RenderLayer* layer)
{
    if (!layer)
        return;

    if (WebCore::Frame* frame = layer->renderer()->frame()) {
        WTF::String output = externalRepresentation(frame, WebCore::RenderAsTextShowAllLayers | WebCore::RenderAsTextShowLayerNesting | WebCore::RenderAsTextShowCompositedLayers | WebCore::RenderAsTextShowAddresses | WebCore::RenderAsTextShowIDAndClass | WebCore::RenderAsTextDontUpdateLayout | WebCore::RenderAsTextShowLayoutState);
        fprintf(stderr, "%s\n", output.utf8().data());
    }
}

void showLayerTree(const WebCore::RenderObject* renderer)
{
    if (!renderer)
        return;
    showLayerTree(renderer->enclosingLayer());
}
#endif
