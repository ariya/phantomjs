/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#include "ColumnInfo.h"
#include "CSSPropertyNames.h"
#include "CSSStyleDeclaration.h"
#include "CSSStyleSelector.h"
#include "Chrome.h"
#include "Document.h"
#include "EventHandler.h"
#include "EventQueue.h"
#include "FloatPoint3D.h"
#include "FloatRect.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "Gradient.h"
#include "GraphicsContext.h"
#include "HTMLFrameOwnerElement.h"
#include "HTMLNames.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "OverflowEvent.h"
#include "OverlapTestRequestClient.h"
#include "Page.h"
#include "PlatformMouseEvent.h"
#include "RenderArena.h"
#include "RenderInline.h"
#include "RenderMarquee.h"
#include "RenderReplica.h"
#include "RenderScrollbar.h"
#include "RenderScrollbarPart.h"
#include "RenderTheme.h"
#include "RenderTreeAsText.h"
#include "RenderView.h"
#include "ScaleTransformOperation.h"
#include "Scrollbar.h"
#include "ScrollbarTheme.h"
#include "SelectionController.h"
#include "TextStream.h"
#include "TransformState.h"
#include "TransformationMatrix.h"
#include "TranslateTransformOperation.h"
#include <wtf/StdLibExtras.h>
#include <wtf/UnusedParam.h>
#include <wtf/text/CString.h>

#if USE(ACCELERATED_COMPOSITING)
#include "RenderLayerBacking.h"
#include "RenderLayerCompositor.h"
#endif

#if ENABLE(SVG)
#include "SVGNames.h"
#endif

#define MIN_INTERSECT_FOR_REVEAL 32

using namespace std;

namespace WebCore {

using namespace HTMLNames;

const int MinimumWidthWhileResizing = 100;
const int MinimumHeightWhileResizing = 40;

void* ClipRects::operator new(size_t sz, RenderArena* renderArena) throw()
{
    return renderArena->allocate(sz);
}

void ClipRects::operator delete(void* ptr, size_t sz)
{
    // Stash size where destroy can find it.
    *(size_t *)ptr = sz;
}

void ClipRects::destroy(RenderArena* renderArena)
{
    delete this;
    
    // Recover the size left there for us by operator delete and free the memory.
    renderArena->free(*(size_t *)this, this);
}

RenderLayer::RenderLayer(RenderBoxModelObject* renderer)
    : m_renderer(renderer)
    , m_parent(0)
    , m_previous(0)
    , m_next(0)
    , m_first(0)
    , m_last(0)
    , m_relX(0)
    , m_relY(0)
    , m_x(0)
    , m_y(0)
    , m_width(0)
    , m_height(0)
    , m_scrollX(0)
    , m_scrollY(0)
    , m_scrollLeftOverflow(0)
    , m_scrollTopOverflow(0)
    , m_scrollWidth(0)
    , m_scrollHeight(0)
    , m_inResizeMode(false)
    , m_posZOrderList(0)
    , m_negZOrderList(0)
    , m_normalFlowList(0)
    , m_clipRects(0) 
#ifndef NDEBUG    
    , m_clipRectsRoot(0)
#endif
    , m_scrollDimensionsDirty(true)
    , m_zOrderListsDirty(true)
    , m_normalFlowListDirty(true)
    , m_isNormalFlowOnly(shouldBeNormalFlowOnly())
    , m_usedTransparency(false)
    , m_paintingInsideReflection(false)
    , m_inOverflowRelayout(false)
    , m_needsFullRepaint(false)
    , m_overflowStatusDirty(true)
    , m_visibleContentStatusDirty(true)
    , m_hasVisibleContent(false)
    , m_visibleDescendantStatusDirty(false)
    , m_hasVisibleDescendant(false)
    , m_isPaginated(false)
    , m_3DTransformedDescendantStatusDirty(true)
    , m_has3DTransformedDescendant(false)
#if USE(ACCELERATED_COMPOSITING)
    , m_hasCompositingDescendant(false)
    , m_mustOverlapCompositedLayers(false)
#endif
    , m_containsDirtyOverlayScrollbars(false)
    , m_marquee(0)
    , m_staticInlinePosition(0)
    , m_staticBlockPosition(0)
    , m_reflection(0)
    , m_scrollCorner(0)
    , m_resizer(0)
{
    ScrollableArea::setConstrainsScrollingToContentEdge(false);

    if (!renderer->firstChild() && renderer->style()) {
        m_visibleContentStatusDirty = false;
        m_hasVisibleContent = renderer->style()->visibility() == VISIBLE;
    }

    if (Frame* frame = renderer->frame()) {
        if (Page* page = frame->page()) {
            m_page = page;
            m_page->addScrollableArea(this);
        }
    }
}

RenderLayer::~RenderLayer()
{
    if (inResizeMode() && !renderer()->documentBeingDestroyed()) {
        if (Frame* frame = renderer()->frame())
            frame->eventHandler()->resizeLayerDestroyed();
    }

    if (m_page)
        m_page->removeScrollableArea(this);

    destroyScrollbar(HorizontalScrollbar);
    destroyScrollbar(VerticalScrollbar);

    if (m_reflection)
        removeReflection();

    // Child layers will be deleted by their corresponding render objects, so
    // we don't need to delete them ourselves.

    delete m_posZOrderList;
    delete m_negZOrderList;
    delete m_normalFlowList;
    delete m_marquee;

#if USE(ACCELERATED_COMPOSITING)
    clearBacking();
#endif
    
    // Make sure we have no lingering clip rects.
    ASSERT(!m_clipRects);
    
    if (m_scrollCorner)
        m_scrollCorner->destroy();
    if (m_resizer)
        m_resizer->destroy();
}

#if USE(ACCELERATED_COMPOSITING)
RenderLayerCompositor* RenderLayer::compositor() const
{
    ASSERT(renderer()->view());
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

bool RenderLayer::hasAcceleratedCompositing() const
{
#if USE(ACCELERATED_COMPOSITING)
    return compositor()->hasAcceleratedCompositing();
#else
    return false;
#endif
}

bool RenderLayer::canRender3DTransforms() const
{
#if USE(ACCELERATED_COMPOSITING)
    return compositor()->canRender3DTransforms();
#else
    return false;
#endif
}

void RenderLayer::updateLayerPositions(UpdateLayerPositionsFlags flags, IntPoint* cachedOffset)
{
    updateLayerPosition(); // For relpositioned layers or non-positioned layers,
                           // we need to keep in sync, since we may have shifted relative
                           // to our parent layer.
    IntPoint oldCachedOffset;
    if (cachedOffset) {
        // We can't cache our offset to the repaint container if the mapping is anything more complex than a simple translation
        bool disableOffsetCache = renderer()->hasColumns() || renderer()->hasTransform() || isComposited();
#if ENABLE(SVG)
        disableOffsetCache = disableOffsetCache || renderer()->isSVGRoot();
#endif
        if (disableOffsetCache)
            cachedOffset = 0; // If our cached offset is invalid make sure it's not passed to any of our children
        else {
            oldCachedOffset = *cachedOffset;
            // Frequently our parent layer's renderer will be the same as our renderer's containing block.  In that case,
            // we just update the cache using our offset to our parent (which is m_x / m_y).  Otherwise, regenerated cached
            // offsets to the root from the render tree.
            if (!m_parent || m_parent->renderer() == renderer()->containingBlock())
                cachedOffset->move(m_x, m_y); // Fast case
            else {
                int x = 0;
                int y = 0;
                convertToLayerCoords(root(), x, y);
                *cachedOffset = IntPoint(x, y);
            }
        }
    }

    int x = 0;
    int y = 0;
    if (cachedOffset) {
        x += cachedOffset->x();
        y += cachedOffset->y();
#ifndef NDEBUG
        int nonCachedX = 0;
        int nonCachedY = 0;
        convertToLayerCoords(root(), nonCachedX, nonCachedY);
        ASSERT(x == nonCachedX);
        ASSERT(y == nonCachedY);
#endif
    } else
        convertToLayerCoords(root(), x, y);
    positionOverflowControls(x, y);

    updateVisibilityStatus();

    if (flags & UpdatePagination)
        updatePagination();
    else
        m_isPaginated = false;

    if (m_hasVisibleContent) {
        RenderView* view = renderer()->view();
        ASSERT(view);
        // FIXME: Optimize using LayoutState and remove the disableLayoutState() call
        // from updateScrollInfoAfterLayout().
        ASSERT(!view->layoutStateEnabled());

        RenderBoxModelObject* repaintContainer = renderer()->containerForRepaint();
        IntRect newRect = renderer()->clippedOverflowRectForRepaint(repaintContainer);
        IntRect newOutlineBox = renderer()->outlineBoundsForRepaint(repaintContainer, cachedOffset);
        // FIXME: Should ASSERT that value calculated for newOutlineBox using the cached offset is the same
        // as the value not using the cached offset, but we can't due to https://bugs.webkit.org/show_bug.cgi?id=37048
        if (flags & CheckForRepaint) {
            if (view && !view->printing()) {
                if (m_needsFullRepaint) {
                    renderer()->repaintUsingContainer(repaintContainer, m_repaintRect);
                    if (newRect != m_repaintRect)
                        renderer()->repaintUsingContainer(repaintContainer, newRect);
                } else
                    renderer()->repaintAfterLayoutIfNeeded(repaintContainer, m_repaintRect, m_outlineBox, &newRect, &newOutlineBox);
            }
        }
        m_repaintRect = newRect;
        m_outlineBox = newOutlineBox;
    } else {
        m_repaintRect = IntRect();
        m_outlineBox = IntRect();
    }

    m_needsFullRepaint = false;

    // Go ahead and update the reflection's position and size.
    if (m_reflection)
        m_reflection->layout();

#if USE(ACCELERATED_COMPOSITING)
    // Clear the IsCompositingUpdateRoot flag once we've found the first compositing layer in this update.
    bool isUpdateRoot = (flags & IsCompositingUpdateRoot);
    if (isComposited())
        flags &= ~IsCompositingUpdateRoot;
#endif

    if (renderer()->hasColumns())
        flags |= UpdatePagination;

    for (RenderLayer* child = firstChild(); child; child = child->nextSibling())
        child->updateLayerPositions(flags, cachedOffset);

#if USE(ACCELERATED_COMPOSITING)
    if ((flags & UpdateCompositingLayers) && isComposited())
        backing()->updateAfterLayout(RenderLayerBacking::CompositingChildren, isUpdateRoot);
#endif
        
    // With all our children positioned, now update our marquee if we need to.
    if (m_marquee)
        m_marquee->updateMarqueePosition();

    if (cachedOffset)
        *cachedOffset = oldCachedOffset;
}

IntRect RenderLayer::repaintRectIncludingDescendants() const
{
    IntRect repaintRect = m_repaintRect;
    for (RenderLayer* child = firstChild(); child; child = child->nextSibling())
        repaintRect.unite(child->repaintRectIncludingDescendants());
    return repaintRect;
}

void RenderLayer::computeRepaintRects()
{
    RenderBoxModelObject* repaintContainer = renderer()->containerForRepaint();
    m_repaintRect = renderer()->clippedOverflowRectForRepaint(repaintContainer);
    m_outlineBox = renderer()->outlineBoundsForRepaint(repaintContainer);
}

void RenderLayer::updateRepaintRectsAfterScroll(bool fixed)
{
    if (fixed || renderer()->style()->position() == FixedPosition) {
        computeRepaintRects();
        fixed = true;
    } else if (renderer()->hasTransform() && !renderer()->isRenderView()) {
        // Transforms act as fixed position containers, so nothing inside a
        // transformed element can be fixed relative to the viewport if the
        // transformed element is not fixed itself or child of a fixed element.
        return;
    }

    for (RenderLayer* child = firstChild(); child; child = child->nextSibling())
        child->updateRepaintRectsAfterScroll(fixed);
}

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
    }
    
    if (hasTransform) {
        RenderBox* box = renderBox();
        ASSERT(box);
        m_transform->makeIdentity();
        box->style()->applyTransform(*m_transform, box->borderBoxRect().size(), RenderStyle::IncludeTransformOrigin);
        makeMatrixRenderable(*m_transform, canRender3DTransforms());
    }

    if (had3DTransform != has3DTransform())
        dirty3DTransformedDescendantStatus();
}

TransformationMatrix RenderLayer::currentTransform() const
{
    if (!m_transform)
        return TransformationMatrix();

#if USE(ACCELERATED_COMPOSITING)
    if (renderer()->style()->isRunningAcceleratedAnimation()) {
        TransformationMatrix currTransform;
        RefPtr<RenderStyle> style = renderer()->animation()->getAnimatedStyleForRenderer(renderer());
        style->applyTransform(currTransform, renderBox()->borderBoxRect().size(), RenderStyle::IncludeTransformOrigin);
        makeMatrixRenderable(currTransform, canRender3DTransforms());
        return currTransform;
    }
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

static bool checkContainingBlockChainForPagination(RenderBoxModelObject* renderer, RenderBox* ancestorColumnsRenderer)
{
    RenderView* view = renderer->view();
    RenderBoxModelObject* prevBlock = renderer;
    RenderBlock* containingBlock;
    for (containingBlock = renderer->containingBlock();
         containingBlock && containingBlock != view && containingBlock != ancestorColumnsRenderer;
         containingBlock = containingBlock->containingBlock())
        prevBlock = containingBlock;
    
    // If the columns block wasn't in our containing block chain, then we aren't paginated by it.
    if (containingBlock != ancestorColumnsRenderer)
        return false;
        
    // If the previous block is absolutely positioned, then we can't be paginated by the columns block.
    if (prevBlock->isPositioned())
        return false;
        
    // Otherwise we are paginated by the columns block.
    return true;
}

void RenderLayer::updatePagination()
{
    m_isPaginated = false;
    if (isComposited() || !parent())
        return; // FIXME: We will have to deal with paginated compositing layers someday.
                // FIXME: For now the RenderView can't be paginated.  Eventually printing will move to a model where it is though.
    
    if (isNormalFlowOnly()) {
        m_isPaginated = parent()->renderer()->hasColumns();
        return;
    }

    // If we're not normal flow, then we need to look for a multi-column object between us and our stacking context.
    RenderLayer* ancestorStackingContext = stackingContext();
    for (RenderLayer* curr = parent(); curr; curr = curr->parent()) {
        if (curr->renderer()->hasColumns()) {
            m_isPaginated = checkContainingBlockChainForPagination(renderer(), curr->renderBox());
            return;
        }
        if (curr == ancestorStackingContext)
            return;
    }
}

void RenderLayer::setHasVisibleContent(bool b)
{ 
    if (m_hasVisibleContent == b && !m_visibleContentStatusDirty)
        return;
    m_visibleContentStatusDirty = false; 
    m_hasVisibleContent = b;
    if (m_hasVisibleContent) {
        RenderBoxModelObject* repaintContainer = renderer()->containerForRepaint();
        m_repaintRect = renderer()->clippedOverflowRectForRepaint(repaintContainer);
        m_outlineBox = renderer()->outlineBoundsForRepaint(repaintContainer);
        if (!isNormalFlowOnly()) {
            for (RenderLayer* sc = stackingContext(); sc; sc = sc->stackingContext()) {
                sc->dirtyZOrderLists();
                if (sc->hasVisibleContent())
                    break;
            }
        }
    }
    if (parent())
        parent()->childVisibilityChanged(m_hasVisibleContent);
}

void RenderLayer::dirtyVisibleContentStatus() 
{ 
    m_visibleContentStatusDirty = true; 
    if (parent())
        parent()->dirtyVisibleDescendantStatus();
}

void RenderLayer::childVisibilityChanged(bool newVisibility) 
{ 
    if (m_hasVisibleDescendant == newVisibility || m_visibleDescendantStatusDirty)
        return;
    if (newVisibility) {
        RenderLayer* l = this;
        while (l && !l->m_visibleDescendantStatusDirty && !l->m_hasVisibleDescendant) {
            l->m_hasVisibleDescendant = true;
            l = l->parent();
        }
    } else 
        dirtyVisibleDescendantStatus();
}

void RenderLayer::dirtyVisibleDescendantStatus()
{
    RenderLayer* l = this;
    while (l && !l->m_visibleDescendantStatusDirty) {
        l->m_visibleDescendantStatusDirty = true;
        l = l->parent();
    }
}

void RenderLayer::updateVisibilityStatus()
{
    if (m_visibleDescendantStatusDirty) {
        m_hasVisibleDescendant = false;
        for (RenderLayer* child = firstChild(); child; child = child->nextSibling()) {
            child->updateVisibilityStatus();        
            if (child->m_hasVisibleContent || child->m_hasVisibleDescendant) {
                m_hasVisibleDescendant = true;
                break;
            }
        }
        m_visibleDescendantStatusDirty = false;
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
    RenderLayer* curr = stackingContext();
    if (curr)
        curr->m_3DTransformedDescendantStatusDirty = true;
        
    // This propagates up through preserve-3d hierarchies to the enclosing flattening layer.
    // Note that preserves3D() creates stacking context, so we can just run up the stacking contexts.
    while (curr && curr->preserves3D()) {
        curr->m_3DTransformedDescendantStatusDirty = true;
        curr = curr->stackingContext();
    }
}

// Return true if this layer or any preserve-3d descendants have 3d.
bool RenderLayer::update3DTransformedDescendantStatus()
{
    if (m_3DTransformedDescendantStatusDirty) {
        m_has3DTransformedDescendant = false;

        // Transformed or preserve-3d descendants can only be in the z-order lists, not
        // in the normal flow list, so we only need to check those.
        if (m_posZOrderList) {
            for (unsigned i = 0; i < m_posZOrderList->size(); ++i)
                m_has3DTransformedDescendant |= m_posZOrderList->at(i)->update3DTransformedDescendantStatus();
        }

        // Now check our negative z-index children.
        if (m_negZOrderList) {
            for (unsigned i = 0; i < m_negZOrderList->size(); ++i)
                m_has3DTransformedDescendant |= m_negZOrderList->at(i)->update3DTransformedDescendantStatus();
        }
        
        m_3DTransformedDescendantStatusDirty = false;
    }
    
    // If we live in a 3d hierarchy, then the layer at the root of that hierarchy needs
    // the m_has3DTransformedDescendant set.
    if (preserves3D())
        return has3DTransform() || m_has3DTransformedDescendant;

    return has3DTransform();
}

void RenderLayer::updateLayerPosition()
{
    IntPoint localPoint;
    IntSize inlineBoundingBoxOffset; // We don't put this into the RenderLayer x/y for inlines, so we need to subtract it out when done.
    if (renderer()->isRenderInline()) {
        RenderInline* inlineFlow = toRenderInline(renderer());
        IntRect lineBox = inlineFlow->linesBoundingBox();
        setWidth(lineBox.width());
        setHeight(lineBox.height());
        inlineBoundingBoxOffset = IntSize(lineBox.x(), lineBox.y());
        localPoint += inlineBoundingBoxOffset;
    } else if (RenderBox* box = renderBox()) {
        setWidth(box->width());
        setHeight(box->height());
        localPoint += box->locationOffsetIncludingFlipping();
    }

    // Clear our cached clip rect information.
    clearClipRects();
 
    if (!renderer()->isPositioned() && renderer()->parent()) {
        // We must adjust our position by walking up the render tree looking for the
        // nearest enclosing object with a layer.
        RenderObject* curr = renderer()->parent();
        while (curr && !curr->hasLayer()) {
            if (curr->isBox() && !curr->isTableRow()) {
                // Rows and cells share the same coordinate space (that of the section).
                // Omit them when computing our xpos/ypos.
                localPoint += toRenderBox(curr)->locationOffsetIncludingFlipping();
            }
            curr = curr->parent();
        }
        if (curr->isBox() && curr->isTableRow()) {
            // Put ourselves into the row coordinate space.
            localPoint -= toRenderBox(curr)->locationOffsetIncludingFlipping();
        }
    }
    
    // Subtract our parent's scroll offset.
    if (renderer()->isPositioned() && enclosingPositionedAncestor()) {
        RenderLayer* positionedParent = enclosingPositionedAncestor();

        // For positioned layers, we subtract out the enclosing positioned layer's scroll offset.
        IntSize offset = positionedParent->scrolledContentOffset();
        localPoint -= offset;
        
        if (renderer()->isPositioned() && positionedParent->renderer()->isRelPositioned() && positionedParent->renderer()->isRenderInline()) {
            IntSize offset = toRenderInline(positionedParent->renderer())->relativePositionedInlineOffset(toRenderBox(renderer()));
            localPoint += offset;
        }
    } else if (parent()) {
        if (isComposited()) {
            // FIXME: Composited layers ignore pagination, so about the best we can do is make sure they're offset into the appropriate column.
            // They won't split across columns properly.
            IntSize columnOffset;
            parent()->renderer()->adjustForColumns(columnOffset, localPoint);
            localPoint += columnOffset;
        }

        IntSize scrollOffset = parent()->scrolledContentOffset();
        localPoint -= scrollOffset;
    }
        
    m_relX = m_relY = 0;
    if (renderer()->isRelPositioned()) {
        m_relX = renderer()->relativePositionOffsetX();
        m_relY = renderer()->relativePositionOffsetY();
        localPoint.move(m_relX, m_relY);
    }

    // FIXME: We'd really like to just get rid of the concept of a layer rectangle and rely on the renderers.
    localPoint -= inlineBoundingBoxOffset;
    setLocation(localPoint.x(), localPoint.y());
}

TransformationMatrix RenderLayer::perspectiveTransform() const
{
    if (!renderer()->hasTransform())
        return TransformationMatrix();

    RenderStyle* style = renderer()->style();
    if (!style->hasPerspective())
        return TransformationMatrix();

    // Maybe fetch the perspective from the backing?
    const IntRect borderBox = toRenderBox(renderer())->borderBoxRect();
    const float boxWidth = borderBox.width();
    const float boxHeight = borderBox.height();

    float perspectiveOriginX = style->perspectiveOriginX().calcFloatValue(boxWidth);
    float perspectiveOriginY = style->perspectiveOriginY().calcFloatValue(boxHeight);

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

    const IntRect borderBox = toRenderBox(renderer())->borderBoxRect();
    RenderStyle* style = renderer()->style();

    return FloatPoint(style->perspectiveOriginX().calcFloatValue(borderBox.width()),
                      style->perspectiveOriginY().calcFloatValue(borderBox.height()));
}

RenderLayer* RenderLayer::stackingContext() const
{
    RenderLayer* layer = parent();
    while (layer && !layer->renderer()->isRenderView() && !layer->renderer()->isRoot() && layer->renderer()->style()->hasAutoZIndex())
        layer = layer->parent();
    return layer;
}

static inline bool isPositionedContainer(RenderLayer* layer)
{
    RenderObject* o = layer->renderer();
    return o->isRenderView() || o->isPositioned() || o->isRelPositioned() || layer->hasTransform();
}

static inline bool isFixedPositionedContainer(RenderLayer* layer)
{
    RenderObject* o = layer->renderer();
    return o->isRenderView() || layer->hasTransform();
}

RenderLayer* RenderLayer::enclosingPositionedAncestor() const
{
    RenderLayer* curr = parent();
    while (curr && !isPositionedContainer(curr))
        curr = curr->parent();

    return curr;
}

RenderLayer* RenderLayer::enclosingTransformedAncestor() const
{
    RenderLayer* curr = parent();
    while (curr && !curr->renderer()->isRenderView() && !curr->transform())
        curr = curr->parent();

    return curr;
}

static inline const RenderLayer* compositingContainer(const RenderLayer* layer)
{
    return layer->isNormalFlowOnly() ? layer->parent() : layer->stackingContext();
}

#if USE(ACCELERATED_COMPOSITING)
RenderLayer* RenderLayer::enclosingCompositingLayer(bool includeSelf) const
{
    if (includeSelf && isComposited())
        return const_cast<RenderLayer*>(this);

    for (const RenderLayer* curr = compositingContainer(this); curr; curr = compositingContainer(curr)) {
        if (curr->isComposited())
            return const_cast<RenderLayer*>(curr);
    }
         
    return 0;
}
#endif

RenderLayer* RenderLayer::clippingRoot() const
{
#if USE(ACCELERATED_COMPOSITING)
    if (isComposited())
        return const_cast<RenderLayer*>(this);
#endif

    const RenderLayer* current = this;
    while (current) {
        if (current->renderer()->isRenderView())
            return const_cast<RenderLayer*>(current);

        current = compositingContainer(current);
        ASSERT(current);
        if (current->transform()
#if USE(ACCELERATED_COMPOSITING)
            || current->isComposited()
#endif
        )
            return const_cast<RenderLayer*>(current);
    }

    ASSERT_NOT_REACHED();
    return 0;
}

IntPoint RenderLayer::absoluteToContents(const IntPoint& absolutePoint) const
{
    // We don't use convertToLayerCoords because it doesn't know about transforms
    return roundedIntPoint(renderer()->absoluteToLocal(absolutePoint, false, true));
}

bool RenderLayer::requiresSlowRepaints() const
{
    if (isTransparent() || hasReflection() || hasTransform())
        return true;
    if (!parent())
        return false;
    return parent()->requiresSlowRepaints();
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

static IntRect transparencyClipBox(const RenderLayer* l, const RenderLayer* rootLayer, PaintBehavior paintBehavior);

static void expandClipRectForDescendantsAndReflection(IntRect& clipRect, const RenderLayer* l, const RenderLayer* rootLayer, PaintBehavior paintBehavior)
{
    // If we have a mask, then the clip is limited to the border box area (and there is
    // no need to examine child layers).
    if (!l->renderer()->hasMask()) {
        // Note: we don't have to walk z-order lists since transparent elements always establish
        // a stacking context.  This means we can just walk the layer tree directly.
        for (RenderLayer* curr = l->firstChild(); curr; curr = curr->nextSibling()) {
            if (!l->reflection() || l->reflectionLayer() != curr)
                clipRect.unite(transparencyClipBox(curr, rootLayer, paintBehavior));
        }
    }

    // If we have a reflection, then we need to account for that when we push the clip.  Reflect our entire
    // current transparencyClipBox to catch all child layers.
    // FIXME: Accelerated compositing will eventually want to do something smart here to avoid incorporating this
    // size into the parent layer.
    if (l->renderer()->hasReflection()) {
        int deltaX = 0;
        int deltaY = 0;
        l->convertToLayerCoords(rootLayer, deltaX, deltaY);
        clipRect.move(-deltaX, -deltaY);
        clipRect.unite(l->renderBox()->reflectedRect(clipRect));
        clipRect.move(deltaX, deltaY);
    }
}

static IntRect transparencyClipBox(const RenderLayer* l, const RenderLayer* rootLayer, PaintBehavior paintBehavior)
{
    // FIXME: Although this function completely ignores CSS-imposed clipping, we did already intersect with the
    // paintDirtyRect, and that should cut down on the amount we have to paint.  Still it
    // would be better to respect clips.
    
    if (rootLayer != l && l->paintsWithTransform(paintBehavior)) {
        // The best we can do here is to use enclosed bounding boxes to establish a "fuzzy" enough clip to encompass
        // the transformed layer and all of its children.
        int x = 0;
        int y = 0;
        l->convertToLayerCoords(rootLayer, x, y);

        TransformationMatrix transform;
        transform.translate(x, y);
        transform = transform * *l->transform();

        IntRect clipRect = l->boundingBox(l);
        expandClipRectForDescendantsAndReflection(clipRect, l, l, paintBehavior);
        return transform.mapRect(clipRect);
    }
    
    IntRect clipRect = l->boundingBox(rootLayer);
    expandClipRectForDescendantsAndReflection(clipRect, l, rootLayer, paintBehavior);
    return clipRect;
}

void RenderLayer::beginTransparencyLayers(GraphicsContext* p, const RenderLayer* rootLayer, PaintBehavior paintBehavior)
{
    if (p->paintingDisabled() || (paintsWithTransparency(paintBehavior) && m_usedTransparency))
        return;
    
    RenderLayer* ancestor = transparentPaintingAncestor();
    if (ancestor)
        ancestor->beginTransparencyLayers(p, rootLayer, paintBehavior);
    
    if (paintsWithTransparency(paintBehavior)) {
        m_usedTransparency = true;
        p->save();
        IntRect clipRect = transparencyClipBox(this, rootLayer, paintBehavior);
        p->clip(clipRect);
        p->beginTransparencyLayer(renderer()->opacity());
#ifdef REVEAL_TRANSPARENCY_LAYERS
        p->setFillColor(Color(0.0f, 0.0f, 0.5f, 0.2f), ColorSpaceDeviceRGB);
        p->fillRect(clipRect);
#endif
    }
}

void* RenderLayer::operator new(size_t sz, RenderArena* renderArena) throw()
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
        // Dirty the z-order list in which we are contained.  The stackingContext() can be null in the
        // case where we're building up generated content layers.  This is ok, since the lists will start
        // off dirty in that case anyway.
        child->dirtyStackingContextZOrderLists();
    }

    child->updateVisibilityStatus();
    if (child->m_hasVisibleContent || child->m_hasVisibleDescendant)
        childVisibilityChanged(true);
    
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
        // from the main layer tree, so we need to null-check the |stackingContext| value.
        oldChild->dirtyStackingContextZOrderLists();
    }

    oldChild->setPreviousSibling(0);
    oldChild->setNextSibling(0);
    oldChild->setParent(0);
    
    oldChild->updateVisibilityStatus();
    if (oldChild->m_hasVisibleContent || oldChild->m_hasVisibleDescendant)
        childVisibilityChanged(false);
    
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

    // Remove us from the parent.
    RenderLayer* parent = m_parent;
    RenderLayer* nextSib = nextSibling();
    parent->removeChild(this);
    
    if (reflection())
        removeChild(reflectionLayer());

    // Now walk our kids and reattach them to our parent.
    RenderLayer* current = m_first;
    while (current) {
        RenderLayer* next = current->nextSibling();
        removeChild(current);
        parent->addChild(current, nextSib);
        current->setNeedsFullRepaint();
        current->updateLayerPositions(); // Depends on hasLayer() already being false for proper layout.
        current = next;
    }

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

void 
RenderLayer::convertToLayerCoords(const RenderLayer* ancestorLayer, int& xPos, int& yPos) const
{
    if (ancestorLayer == this)
        return;

    EPosition position = renderer()->style()->position();
    if (position == FixedPosition && (!ancestorLayer || ancestorLayer == renderer()->view()->layer())) {
        // If the fixed layer's container is the root, just add in the offset of the view. We can obtain this by calling
        // localToAbsolute() on the RenderView.
        FloatPoint absPos = renderer()->localToAbsolute(FloatPoint(), true);
        xPos += absPos.x();
        yPos += absPos.y();
        return;
    }
 
    if (position == FixedPosition) {
        // For a fixed layers, we need to walk up to the root to see if there's a fixed position container
        // (e.g. a transformed layer). It's an error to call convertToLayerCoords() across a layer with a transform,
        // so we should always find the ancestor at or before we find the fixed position container.
        RenderLayer* fixedPositionContainerLayer = 0;
        bool foundAncestor = false;
        for (RenderLayer* currLayer = parent(); currLayer; currLayer = currLayer->parent()) {
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
            int fixedContainerX = 0;
            int fixedContainerY = 0;
            convertToLayerCoords(fixedPositionContainerLayer, fixedContainerX, fixedContainerY);
            
            int ancestorX = 0;
            int ancestorY = 0;
            ancestorLayer->convertToLayerCoords(fixedPositionContainerLayer, ancestorX, ancestorY);
        
            xPos += (fixedContainerX - ancestorX);
            yPos += (fixedContainerY - ancestorY);
            return;
        }
    }
    
    RenderLayer* parentLayer;
    if (position == AbsolutePosition || position == FixedPosition) {
        // Do what enclosingPositionedAncestor() does, but check for ancestorLayer along the way.
        parentLayer = parent();
        bool foundAncestorFirst = false;
        while (parentLayer) {
            if (isPositionedContainer(parentLayer))
                break;

            if (parentLayer == ancestorLayer) {
                foundAncestorFirst = true;
                break;
            }

            parentLayer = parentLayer->parent();
        }

        if (foundAncestorFirst) {
            // Found ancestorLayer before the abs. positioned container, so compute offset of both relative
            // to enclosingPositionedAncestor and subtract.
            RenderLayer* positionedAncestor = parentLayer->enclosingPositionedAncestor();

            int thisX = 0;
            int thisY = 0;
            convertToLayerCoords(positionedAncestor, thisX, thisY);
            
            int ancestorX = 0;
            int ancestorY = 0;
            ancestorLayer->convertToLayerCoords(positionedAncestor, ancestorX, ancestorY);
        
            xPos += (thisX - ancestorX);
            yPos += (thisY - ancestorY);
            return;
        }
    } else
        parentLayer = parent();
    
    if (!parentLayer)
        return;
    
    parentLayer->convertToLayerCoords(ancestorLayer, xPos, yPos);
    
    xPos += x();
    yPos += y();
}

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

void RenderLayer::panScrollFromPoint(const IntPoint& sourcePoint) 
{
    Frame* frame = renderer()->frame();
    if (!frame)
        return;
    
    IntPoint currentMousePosition = frame->eventHandler()->currentMousePosition();
    
    // We need to check if the current mouse position is out of the window. When the mouse is out of the window, the position is incoherent
    static IntPoint previousMousePosition;
    if (currentMousePosition.x() < 0 || currentMousePosition.y() < 0)
        currentMousePosition = previousMousePosition;
    else
        previousMousePosition = currentMousePosition;

    int xDelta = currentMousePosition.x() - sourcePoint.x();
    int yDelta = currentMousePosition.y() - sourcePoint.y();

    if (abs(xDelta) <= ScrollView::noPanScrollRadius) // at the center we let the space for the icon
        xDelta = 0;
    if (abs(yDelta) <= ScrollView::noPanScrollRadius)
        yDelta = 0;

    scrollByRecursively(adjustedScrollDelta(xDelta), adjustedScrollDelta(yDelta));
}

void RenderLayer::scrollByRecursively(int xDelta, int yDelta)
{
    if (!xDelta && !yDelta)
        return;

    bool restrictedByLineClamp = false;
    if (renderer()->parent())
        restrictedByLineClamp = !renderer()->parent()->style()->lineClamp().isNone();

    if (renderer()->hasOverflowClip() && !restrictedByLineClamp) {
        int newOffsetX = scrollXOffset() + xDelta;
        int newOffsetY = scrollYOffset() + yDelta;
        scrollToOffset(newOffsetX, newOffsetY);

        // If this layer can't do the scroll we ask the next layer up that can scroll to try
        int leftToScrollX = newOffsetX - scrollXOffset();
        int leftToScrollY = newOffsetY - scrollYOffset();
        if ((leftToScrollX || leftToScrollY) && renderer()->parent()) {
            RenderObject* nextRenderer = renderer()->parent();
            while (nextRenderer) {
                if (nextRenderer->isBox() && toRenderBox(nextRenderer)->canBeScrolledAndHasScrollableArea()) {
                    nextRenderer->enclosingLayer()->scrollByRecursively(leftToScrollX, leftToScrollY);
                    break;
                }
                nextRenderer = nextRenderer->parent();
            }

            Frame* frame = renderer()->frame();
            if (frame)
                frame->eventHandler()->updateAutoscrollRenderer();
        }
    } else if (renderer()->view()->frameView()) {
        // If we are here, we were called on a renderer that can be programmatically scrolled, but doesn't
        // have an overflow clip. Which means that it is a document node that can be scrolled.
        renderer()->view()->frameView()->scrollBy(IntSize(xDelta, yDelta));
        // FIXME: If we didn't scroll the whole way, do we want to try looking at the frames ownerElement? 
        // https://bugs.webkit.org/show_bug.cgi?id=28237
    }
}

void RenderLayer::scrollToOffset(int x, int y)
{
    ScrollableArea::scrollToOffsetWithoutAnimation(IntPoint(x, y));
}

void RenderLayer::scrollTo(int x, int y)
{
    RenderBox* box = renderBox();
    if (!box)
        return;

    if (box->style()->overflowX() != OMARQUEE) {
        if (x < 0)
            x = 0;
        if (y < 0)
            y = 0;
    
        // Call the scrollWidth/Height functions so that the dimensions will be computed if they need
        // to be (for overflow:hidden blocks).
        int maxX = scrollWidth() - box->clientWidth();
        if (maxX < 0)
            maxX = 0;
        int maxY = scrollHeight() - box->clientHeight();
        if (maxY < 0)
            maxY = 0;

        if (x > maxX)
            x = maxX;
        if (y > maxY)
            y = maxY;
    }
    
    // FIXME: Eventually, we will want to perform a blit.  For now never
    // blit, since the check for blitting is going to be very
    // complicated (since it will involve testing whether our layer
    // is either occluded by another layer or clipped by an enclosing
    // layer or contains fixed backgrounds, etc.).
    int newScrollX = x - m_scrollOrigin.x();
    int newScrollY = y - m_scrollOrigin.y();
    if (m_scrollY == newScrollY && m_scrollX == newScrollX)
        return;
    m_scrollX = newScrollX;
    m_scrollY = newScrollY;

    // Update the positions of our child layers. Don't have updateLayerPositions() update
    // compositing layers, because we need to do a deep update from the compositing ancestor.
    for (RenderLayer* child = firstChild(); child; child = child->nextSibling())
        child->updateLayerPositions(0);

    RenderView* view = renderer()->view();
    
    // We should have a RenderView if we're trying to scroll.
    ASSERT(view);
    if (view) {
#if ENABLE(DASHBOARD_SUPPORT)
        // Update dashboard regions, scrolling may change the clip of a
        // particular region.
        view->frameView()->updateDashboardRegions();
#endif

        view->updateWidgetPositions();
    }

#if USE(ACCELERATED_COMPOSITING)
    if (compositor()->inCompositingMode()) {
        // Our stacking context is guaranteed to contain all of our descendants that may need
        // repositioning, so update compositing layers from there.
        if (RenderLayer* compositingAncestor = stackingContext()->enclosingCompositingLayer()) {
            if (compositor()->compositingConsultsOverlap())
                compositor()->updateCompositingLayers(CompositingUpdateOnScroll, compositingAncestor);
            else {
                bool isUpdateRoot = true;
                compositingAncestor->backing()->updateAfterLayout(RenderLayerBacking::AllDescendants, isUpdateRoot);
            }
        }
    }
#endif

    RenderBoxModelObject* repaintContainer = renderer()->containerForRepaint();
    IntRect rectForRepaint = renderer()->clippedOverflowRectForRepaint(repaintContainer);

    Frame* frame = renderer()->frame();
    if (frame) {
        // The caret rect needs to be invalidated after scrolling
        frame->selection()->setCaretRectNeedsUpdate();

        FloatQuad quadForFakeMouseMoveEvent = FloatQuad(rectForRepaint);
        if (repaintContainer)
            quadForFakeMouseMoveEvent = repaintContainer->localToAbsoluteQuad(quadForFakeMouseMoveEvent);
        frame->eventHandler()->dispatchFakeMouseMoveEventSoonInQuad(quadForFakeMouseMoveEvent);
    }

    // Just schedule a full repaint of our object.
    if (view)
        renderer()->repaintUsingContainer(repaintContainer, rectForRepaint);

    // Schedule the scroll DOM event.
    renderer()->node()->document()->eventQueue()->enqueueOrDispatchScrollEvent(renderer()->node(), EventQueue::ScrollEventElementTarget);
}

void RenderLayer::scrollRectToVisible(const IntRect& rect, bool scrollToAnchor, const ScrollAlignment& alignX, const ScrollAlignment& alignY)
{
    RenderLayer* parentLayer = 0;
    IntRect newRect = rect;
    int xOffset = 0, yOffset = 0;

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
        FloatPoint absPos = box->localToAbsolute();
        absPos.move(box->borderLeft(), box->borderTop());

        IntRect layerBounds = IntRect(absPos.x() + scrollXOffset(), absPos.y() + scrollYOffset(), box->clientWidth(), box->clientHeight());
        IntRect exposeRect = IntRect(rect.x() + scrollXOffset(), rect.y() + scrollYOffset(), rect.width(), rect.height());
        IntRect r = getRectToExpose(layerBounds, exposeRect, alignX, alignY);
        
        xOffset = r.x() - absPos.x();
        yOffset = r.y() - absPos.y();
        // Adjust offsets if they're outside of the allowable range.
        xOffset = max(0, min(scrollWidth() - layerBounds.width(), xOffset));
        yOffset = max(0, min(scrollHeight() - layerBounds.height(), yOffset));
        
        if (xOffset != scrollXOffset() || yOffset != scrollYOffset()) {
            int diffX = scrollXOffset();
            int diffY = scrollYOffset();
            scrollToOffset(xOffset, yOffset);
            diffX = scrollXOffset() - diffX;
            diffY = scrollYOffset() - diffY;
            newRect.setX(rect.x() - diffX);
            newRect.setY(rect.y() - diffY);
        }
    } else if (!parentLayer && renderer()->isBox() && renderBox()->canBeProgramaticallyScrolled(scrollToAnchor)) {
        if (frameView) {
            if (renderer()->document() && renderer()->document()->ownerElement() && renderer()->document()->ownerElement()->renderer()) {
                IntRect viewRect = frameView->visibleContentRect();
                IntRect r = getRectToExpose(viewRect, rect, alignX, alignY);
                
                xOffset = r.x();
                yOffset = r.y();
                // Adjust offsets if they're outside of the allowable range.
                xOffset = max(0, min(frameView->contentsWidth(), xOffset));
                yOffset = max(0, min(frameView->contentsHeight(), yOffset));

                frameView->setScrollPosition(IntPoint(xOffset, yOffset));
                parentLayer = renderer()->document()->ownerElement()->renderer()->enclosingLayer();
                newRect.setX(rect.x() - frameView->scrollX() + frameView->x());
                newRect.setY(rect.y() - frameView->scrollY() + frameView->y());
            } else {
                IntRect viewRect = frameView->visibleContentRect();
                IntRect r = getRectToExpose(viewRect, rect, alignX, alignY);
                
                frameView->setScrollPosition(r.location());

                // This is the outermost view of a web page, so after scrolling this view we
                // scroll its container by calling Page::scrollRectIntoView.
                // This only has an effect on the Mac platform in applications
                // that put web views into scrolling containers, such as Mac OS X Mail.
                // The canAutoscroll function in EventHandler also knows about this.
                if (Frame* frame = frameView->frame()) {
                    if (Page* page = frame->page())
                        page->chrome()->scrollRectIntoView(rect);
                }
            }
        }
    }
    
    if (parentLayer)
        parentLayer->scrollRectToVisible(newRect, scrollToAnchor, alignX, alignY);

    if (frameView)
        frameView->resumeScheduledEvents();
}

IntRect RenderLayer::getRectToExpose(const IntRect &visibleRect, const IntRect &exposeRect, const ScrollAlignment& alignX, const ScrollAlignment& alignY)
{
    // Determine the appropriate X behavior.
    ScrollBehavior scrollX;
    IntRect exposeRectX(exposeRect.x(), visibleRect.y(), exposeRect.width(), visibleRect.height());
    int intersectWidth = intersection(visibleRect, exposeRectX).width();
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
    int x;
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
    IntRect exposeRectY(visibleRect.x(), exposeRect.y(), visibleRect.width(), exposeRect.height());
    int intersectHeight = intersection(visibleRect, exposeRectY).height();
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
    int y;
    if (scrollY == noScroll) 
        y = visibleRect.y();
    else if (scrollY == alignBottom)
        y = exposeRect.maxY() - visibleRect.height();
    else if (scrollY == alignCenter)
        y = exposeRect.y() + (exposeRect.height() - visibleRect.height()) / 2;
    else
        y = exposeRect.y();

    return IntRect(IntPoint(x, y), visibleRect.size());
}

void RenderLayer::autoscroll()
{
    Frame* frame = renderer()->frame();
    if (!frame)
        return;

    FrameView* frameView = frame->view();
    if (!frameView)
        return;

#if ENABLE(DRAG_SUPPORT)
    frame->eventHandler()->updateSelectionForMouseDrag();
#endif

    IntPoint currentDocumentPosition = frameView->windowToContents(frame->eventHandler()->currentMousePosition());
    scrollRectToVisible(IntRect(currentDocumentPosition, IntSize(1, 1)), false, ScrollAlignment::alignToEdgeIfNeeded, ScrollAlignment::alignToEdgeIfNeeded);
}

void RenderLayer::resize(const PlatformMouseEvent& evt, const IntSize& oldOffset)
{
    // FIXME: This should be possible on generated content but is not right now.
    if (!inResizeMode() || !renderer()->hasOverflowClip() || !renderer()->node())
        return;

    // Set the width and height of the shadow ancestor node if there is one.
    // This is necessary for textarea elements since the resizable layer is in the shadow content.
    Element* element = static_cast<Element*>(renderer()->node()->shadowAncestorNode());
    RenderBox* renderer = toRenderBox(element->renderer());

    EResize resize = renderer->style()->resize();
    if (resize == RESIZE_NONE)
        return;

    Document* document = element->document();
    if (!document->frame()->eventHandler()->mousePressed())
        return;

    float zoomFactor = renderer->style()->effectiveZoom();

    IntSize newOffset = offsetFromResizeCorner(document->view()->windowToContents(evt.pos()));
    newOffset.setWidth(newOffset.width() / zoomFactor);
    newOffset.setHeight(newOffset.height() / zoomFactor);
    
    IntSize currentSize = IntSize(renderer->width() / zoomFactor, renderer->height() / zoomFactor);
    IntSize minimumSize = element->minimumSizeForResizing().shrunkTo(currentSize);
    element->setMinimumSizeForResizing(minimumSize);
    
    IntSize adjustedOldOffset = IntSize(oldOffset.width() / zoomFactor, oldOffset.height() / zoomFactor);
    
    IntSize difference = (currentSize + newOffset - adjustedOldOffset).expandedTo(minimumSize) - currentSize;

    CSSStyleDeclaration* style = element->style();
    bool isBoxSizingBorder = renderer->style()->boxSizing() == BORDER_BOX;

    ExceptionCode ec;

    if (resize != RESIZE_VERTICAL && difference.width()) {
        if (element->isFormControlElement()) {
            // Make implicit margins from the theme explicit (see <http://bugs.webkit.org/show_bug.cgi?id=9547>).
            style->setProperty(CSSPropertyMarginLeft, String::number(renderer->marginLeft() / zoomFactor) + "px", false, ec);
            style->setProperty(CSSPropertyMarginRight, String::number(renderer->marginRight() / zoomFactor) + "px", false, ec);
        }
        int baseWidth = renderer->width() - (isBoxSizingBorder ? 0 : renderer->borderAndPaddingWidth());
        baseWidth = baseWidth / zoomFactor;
        style->setProperty(CSSPropertyWidth, String::number(baseWidth + difference.width()) + "px", false, ec);
    }

    if (resize != RESIZE_HORIZONTAL && difference.height()) {
        if (element->isFormControlElement()) {
            // Make implicit margins from the theme explicit (see <http://bugs.webkit.org/show_bug.cgi?id=9547>).
            style->setProperty(CSSPropertyMarginTop, String::number(renderer->marginTop() / zoomFactor) + "px", false, ec);
            style->setProperty(CSSPropertyMarginBottom, String::number(renderer->marginBottom() / zoomFactor) + "px", false, ec);
        }
        int baseHeight = renderer->height() - (isBoxSizingBorder ? 0 : renderer->borderAndPaddingHeight());
        baseHeight = baseHeight / zoomFactor;
        style->setProperty(CSSPropertyHeight, String::number(baseHeight + difference.height()) + "px", false, ec);
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

bool RenderLayer::isActive() const
{
    Page* page = renderer()->frame()->page();
    return page && page->focusController()->isActive();
}

static IntRect cornerRect(const RenderLayer* layer, const IntRect& bounds)
{
    int horizontalThickness;
    int verticalThickness;
    if (!layer->verticalScrollbar() && !layer->horizontalScrollbar()) {
        // FIXME: This isn't right.  We need to know the thickness of custom scrollbars
        // even when they don't exist in order to set the resizer square size properly.
        horizontalThickness = ScrollbarTheme::nativeTheme()->scrollbarThickness();
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
    return IntRect(bounds.maxX() - horizontalThickness - layer->renderer()->style()->borderRightWidth(), 
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
        return cornerRect(this, renderBox()->borderBoxRect());
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
        scrollCornerAndResizer = resizerCornerRect(this, box->borderBoxRect());
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
    return IntSize(const_cast<RenderLayer*>(this)->scrollWidth(), const_cast<RenderLayer*>(this)->scrollHeight());
}

int RenderLayer::visibleHeight() const
{
    return m_height;
}

int RenderLayer::visibleWidth() const
{
    return m_width;
}

bool RenderLayer::shouldSuspendScrollAnimations() const
{
    RenderView* view = renderer()->view();
    if (!view)
        return true;
    return view->frameView()->shouldSuspendScrollAnimations();
}

IntPoint RenderLayer::currentMousePosition() const
{
    return renderer()->frame() ? renderer()->frame()->eventHandler()->currentMousePosition() : IntPoint();
}

IntSize RenderLayer::scrollbarOffset(const Scrollbar* scrollbar) const
{
    RenderBox* box = renderBox();

    if (scrollbar == m_vBar.get())
        return IntSize(box->width() - box->borderRight() - scrollbar->width(), box->borderTop());

    if (scrollbar == m_hBar.get())
        return IntSize(box->borderLeft(), box->height() - box->borderBottom() - scrollbar->height());
    
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
    if (scrollbar == m_vBar.get())
        scrollRect.move(box->width() - box->borderRight() - scrollbar->width(), box->borderTop());
    else
        scrollRect.move(box->borderLeft(), box->height() - box->borderBottom() - scrollbar->height());
    renderer()->repaintRectangle(scrollRect);
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

PassRefPtr<Scrollbar> RenderLayer::createScrollbar(ScrollbarOrientation orientation)
{
    RefPtr<Scrollbar> widget;
    RenderObject* actualRenderer = renderer()->node() ? renderer()->node()->shadowAncestorNode()->renderer() : renderer();
    bool hasCustomScrollbarStyle = actualRenderer->isBox() && actualRenderer->style()->hasPseudoStyle(SCROLLBAR);
    if (hasCustomScrollbarStyle)
        widget = RenderScrollbar::createCustomScrollbar(this, orientation, toRenderBox(actualRenderer));
    else {
        widget = Scrollbar::createNativeScrollbar(this, orientation, RegularScrollbar);
        if (orientation == HorizontalScrollbar)
            didAddHorizontalScrollbar(widget.get());
        else 
            didAddVerticalScrollbar(widget.get());
    }
    renderer()->document()->view()->addChild(widget.get());        
    return widget.release();
}

void RenderLayer::destroyScrollbar(ScrollbarOrientation orientation)
{
    RefPtr<Scrollbar>& scrollbar = orientation == HorizontalScrollbar ? m_hBar : m_vBar;
    if (scrollbar) {
        if (scrollbar->isCustomScrollbar())
            static_cast<RenderScrollbar*>(scrollbar.get())->clearOwningRenderer();
        else {
            if (orientation == HorizontalScrollbar)
                willRemoveHorizontalScrollbar(scrollbar.get());
            else
                willRemoveVerticalScrollbar(scrollbar.get());
        }

        scrollbar->removeFromParent();
        scrollbar->disconnectFromScrollableArea();
        scrollbar = 0;
    }
}

void RenderLayer::setHasHorizontalScrollbar(bool hasScrollbar)
{
    if (hasScrollbar == (m_hBar != 0))
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

#if ENABLE(DASHBOARD_SUPPORT)
    // Force an update since we know the scrollbars have changed things.
    if (renderer()->document()->hasDashboardRegions())
        renderer()->document()->setDashboardRegionsDirty(true);
#endif
}

void RenderLayer::setHasVerticalScrollbar(bool hasScrollbar)
{
    if (hasScrollbar == (m_vBar != 0))
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

#if ENABLE(DASHBOARD_SUPPORT)
    // Force an update since we know the scrollbars have changed things.
    if (renderer()->document()->hasDashboardRegions())
        renderer()->document()->setDashboardRegionsDirty(true);
#endif
}

int RenderLayer::verticalScrollbarWidth(OverlayScrollbarSizeRelevancy relevancy) const
{
    if (!m_vBar || (m_vBar->isOverlayScrollbar() && relevancy == IgnoreOverlayScrollbarSize))
        return 0;
    return m_vBar->width();
}

int RenderLayer::horizontalScrollbarHeight(OverlayScrollbarSizeRelevancy relevancy) const
{
    if (!m_hBar || (m_hBar->isOverlayScrollbar() && relevancy == IgnoreOverlayScrollbarSize))
        return 0;
    return m_hBar->height();
}

IntSize RenderLayer::offsetFromResizeCorner(const IntPoint& absolutePoint) const
{
    // Currently the resize corner is always the bottom right corner
    IntPoint bottomRight(width(), height());
    IntPoint localPoint = absoluteToContents(absolutePoint);
    return localPoint - bottomRight;
}

bool RenderLayer::hasOverflowControls() const
{
    return m_hBar || m_vBar || m_scrollCorner || renderer()->style()->resize() != RESIZE_NONE;
}

void RenderLayer::positionOverflowControls(int tx, int ty)
{
    if (!m_hBar && !m_vBar && (!renderer()->hasOverflowClip() || renderer()->style()->resize() == RESIZE_NONE))
        return;
    
    RenderBox* box = renderBox();
    if (!box)
        return;

    const IntRect& borderBox = box->borderBoxRect();
    const IntRect& scrollCorner = scrollCornerRect();
    IntRect absBounds(borderBox.x() + tx, borderBox.y() + ty, borderBox.width(), borderBox.height());
    if (m_vBar)
        m_vBar->setFrameRect(IntRect(absBounds.maxX() - box->borderRight() - m_vBar->width(),
                                     absBounds.y() + box->borderTop(),
                                     m_vBar->width(),
                                     absBounds.height() - (box->borderTop() + box->borderBottom()) - scrollCorner.height()));

    if (m_hBar)
        m_hBar->setFrameRect(IntRect(absBounds.x() + box->borderLeft(),
                                     absBounds.maxY() - box->borderBottom() - m_hBar->height(),
                                     absBounds.width() - (box->borderLeft() + box->borderRight()) - scrollCorner.width(),
                                     m_hBar->height()));

#if USE(ACCELERATED_COMPOSITING)
    if (GraphicsLayer* layer = layerForHorizontalScrollbar()) {
        if (m_hBar) {
            layer->setPosition(IntPoint(m_hBar->frameRect().x() - tx, m_hBar->frameRect().y() - ty));
            layer->setSize(m_hBar->frameRect().size());
        }
        layer->setDrawsContent(m_hBar);
    }
    if (GraphicsLayer* layer = layerForVerticalScrollbar()) {
        if (m_vBar) {
            layer->setPosition(IntPoint(m_vBar->frameRect().x() - tx, m_vBar->frameRect().y() - ty));
            layer->setSize(m_vBar->frameRect().size());
        }
        layer->setDrawsContent(m_vBar);
    }

    if (GraphicsLayer* layer = layerForScrollCorner()) {
        const IntRect& scrollCornerAndResizer = scrollCornerAndResizerRect();
        layer->setPosition(scrollCornerAndResizer.location());
        layer->setSize(scrollCornerAndResizer.size());
        layer->setDrawsContent(!scrollCornerAndResizer.isEmpty());
    }
#endif

    if (m_scrollCorner)
        m_scrollCorner->setFrameRect(scrollCorner);
    if (m_resizer)
        m_resizer->setFrameRect(resizerCornerRect(this, borderBox));
}

int RenderLayer::scrollWidth()
{
    if (m_scrollDimensionsDirty)
        computeScrollDimensions();
    return m_scrollWidth;
}

int RenderLayer::scrollHeight()
{
    if (m_scrollDimensionsDirty)
        computeScrollDimensions();
    return m_scrollHeight;
}

int RenderLayer::overflowTop() const
{
    RenderBox* box = renderBox();
    IntRect overflowRect(box->layoutOverflowRect());
    box->flipForWritingMode(overflowRect);
    return overflowRect.y();
}

int RenderLayer::overflowBottom() const
{
    RenderBox* box = renderBox();
    IntRect overflowRect(box->layoutOverflowRect());
    box->flipForWritingMode(overflowRect);
    return overflowRect.maxY();
}

int RenderLayer::overflowLeft() const
{
    RenderBox* box = renderBox();
    IntRect overflowRect(box->layoutOverflowRect());
    box->flipForWritingMode(overflowRect);
    return overflowRect.x();
}

int RenderLayer::overflowRight() const
{
    RenderBox* box = renderBox();
    IntRect overflowRect(box->layoutOverflowRect());
    box->flipForWritingMode(overflowRect);
    return overflowRect.maxX();
}

void RenderLayer::computeScrollDimensions(bool* needHBar, bool* needVBar)
{
    RenderBox* box = renderBox();
    ASSERT(box);
    
    m_scrollDimensionsDirty = false;

    m_scrollLeftOverflow = overflowLeft() - box->borderLeft();
    m_scrollTopOverflow = overflowTop() - box->borderTop();

    m_scrollWidth = overflowRight() - overflowLeft();
    m_scrollHeight = overflowBottom() - overflowTop();
    
    m_scrollOrigin = IntPoint(-m_scrollLeftOverflow, -m_scrollTopOverflow);

    if (needHBar)
        *needHBar = m_scrollWidth > box->clientWidth();
    if (needVBar)
        *needVBar = m_scrollHeight > box->clientHeight();
}

void RenderLayer::updateOverflowStatus(bool horizontalOverflow, bool verticalOverflow)
{
    if (m_overflowStatusDirty) {
        m_horizontalOverflow = horizontalOverflow;
        m_verticalOverflow = verticalOverflow;
        m_overflowStatusDirty = false;
        return;
    }
    
    bool horizontalOverflowChanged = (m_horizontalOverflow != horizontalOverflow);
    bool verticalOverflowChanged = (m_verticalOverflow != verticalOverflow);
    
    if (horizontalOverflowChanged || verticalOverflowChanged) {
        m_horizontalOverflow = horizontalOverflow;
        m_verticalOverflow = verticalOverflow;
        
        if (FrameView* frameView = renderer()->document()->view()) {
            frameView->scheduleEvent(OverflowEvent::create(horizontalOverflowChanged, horizontalOverflow, verticalOverflowChanged, verticalOverflow),
                renderer()->node());
        }
    }
}

void RenderLayer::updateScrollInfoAfterLayout()
{
    RenderBox* box = renderBox();
    if (!box)
        return;

    m_scrollDimensionsDirty = true;

    bool horizontalOverflow, verticalOverflow;
    computeScrollDimensions(&horizontalOverflow, &verticalOverflow);

    if (box->style()->overflowX() != OMARQUEE) {
        // Layout may cause us to be in an invalid scroll position.  In this case we need
        // to pull our scroll offsets back to the max (or push them up to the min).
        int newX = max(0, min(scrollXOffset(), scrollWidth() - box->clientWidth()));
        int newY = max(0, min(scrollYOffset(), scrollHeight() - box->clientHeight()));
        if (newX != scrollXOffset() || newY != scrollYOffset()) {
            RenderView* view = renderer()->view();
            ASSERT(view);
            // scrollToOffset() may call updateLayerPositions(), which doesn't work
            // with LayoutState.
            // FIXME: Remove the disableLayoutState/enableLayoutState if the above changes.
            if (view)
                view->disableLayoutState();
            scrollToOffset(newX, newY);
            if (view)
                view->enableLayoutState();
        }
    }

    bool haveHorizontalBar = m_hBar;
    bool haveVerticalBar = m_vBar;
    
    // overflow:scroll should just enable/disable.
    if (renderer()->style()->overflowX() == OSCROLL)
        m_hBar->setEnabled(horizontalOverflow);
    if (renderer()->style()->overflowY() == OSCROLL)
        m_vBar->setEnabled(verticalOverflow);

    // A dynamic change from a scrolling overflow to overflow:hidden means we need to get rid of any
    // scrollbars that may be present.
    if (renderer()->style()->overflowX() == OHIDDEN && haveHorizontalBar)
        setHasHorizontalScrollbar(false);
    if (renderer()->style()->overflowY() == OHIDDEN && haveVerticalBar)
        setHasVerticalScrollbar(false);
    
    // overflow:auto may need to lay out again if scrollbars got added/removed.
    bool scrollbarsChanged = (box->hasAutoHorizontalScrollbar() && haveHorizontalBar != horizontalOverflow) || 
                             (box->hasAutoVerticalScrollbar() && haveVerticalBar != verticalOverflow);    
    if (scrollbarsChanged) {
        if (box->hasAutoHorizontalScrollbar())
            setHasHorizontalScrollbar(horizontalOverflow);
        if (box->hasAutoVerticalScrollbar())
            setHasVerticalScrollbar(verticalOverflow);

#if ENABLE(DASHBOARD_SUPPORT)
        // Force an update since we know the scrollbars have changed things.
        if (renderer()->document()->hasDashboardRegions())
            renderer()->document()->setDashboardRegionsDirty(true);
#endif

        renderer()->repaint();

        if (renderer()->style()->overflowX() == OAUTO || renderer()->style()->overflowY() == OAUTO) {
            if (!m_inOverflowRelayout) {
                // Our proprietary overflow: overlay value doesn't trigger a layout.
                m_inOverflowRelayout = true;
                renderer()->setNeedsLayout(true, false);
                if (renderer()->isRenderBlock()) {
                    RenderBlock* block = toRenderBlock(renderer());
                    block->scrollbarsChanged(box->hasAutoHorizontalScrollbar() && haveHorizontalBar != horizontalOverflow,
                                             box->hasAutoVerticalScrollbar() && haveVerticalBar != verticalOverflow);
                    block->layoutBlock(true);
                } else
                    renderer()->layout();
                m_inOverflowRelayout = false;
            }
        }
    }
    
    // If overflow:scroll is turned into overflow:auto a bar might still be disabled (Bug 11985).
    if (m_hBar && box->hasAutoHorizontalScrollbar())
        m_hBar->setEnabled(true);
    if (m_vBar && box->hasAutoVerticalScrollbar())
        m_vBar->setEnabled(true);

    // Set up the range (and page step/line step).
    if (m_hBar) {
        int clientWidth = box->clientWidth();
        int pageStep = max(max<int>(clientWidth * Scrollbar::minFractionToStepWhenPaging(), clientWidth - Scrollbar::maxOverlapBetweenPages()), 1);
        m_hBar->setSteps(Scrollbar::pixelsPerLineStep(), pageStep);
        m_hBar->setProportion(clientWidth, m_scrollWidth);
    }
    if (m_vBar) {
        int clientHeight = box->clientHeight();
        int pageStep = max(max<int>(clientHeight * Scrollbar::minFractionToStepWhenPaging(), clientHeight - Scrollbar::maxOverlapBetweenPages()), 1);
        m_vBar->setSteps(Scrollbar::pixelsPerLineStep(), pageStep);
        m_vBar->setProportion(clientHeight, m_scrollHeight);
    }
 
    RenderView* view = renderer()->view();
    view->disableLayoutState();
    scrollToOffset(scrollXOffset(), scrollYOffset());
    view->enableLayoutState();
 
    if (renderer()->node() && renderer()->document()->hasListenerType(Document::OVERFLOWCHANGED_LISTENER))
        updateOverflowStatus(horizontalOverflow, verticalOverflow);
}

void RenderLayer::paintOverflowControls(GraphicsContext* context, int tx, int ty, const IntRect& damageRect, bool paintingOverlayControls)
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
        RenderView* renderView = renderer()->view();
        renderView->layer()->setContainsDirtyOverlayScrollbars(true);
        m_cachedOverlayScrollbarOffset = IntPoint(tx, ty);
        renderView->frameView()->setContainsScrollableAreaWithOverlayScrollbars(true);
        return;
    }

    int offsetX = tx;
    int offsetY = ty;
    if (paintingOverlayControls) {
        offsetX = m_cachedOverlayScrollbarOffset.x();
        offsetY = m_cachedOverlayScrollbarOffset.y();
    }

    // Move the scrollbar widgets if necessary.  We normally move and resize widgets during layout, but sometimes
    // widgets can move without layout occurring (most notably when you scroll a document that
    // contains fixed positioned elements).
    positionOverflowControls(offsetX, offsetY);

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
    paintScrollCorner(context, offsetX, offsetY, damageRect);
    
    // Paint our resizer last, since it sits on top of the scroll corner.
    paintResizer(context, offsetX, offsetY, damageRect);
}

void RenderLayer::paintScrollCorner(GraphicsContext* context, int tx, int ty, const IntRect& damageRect)
{
    RenderBox* box = renderBox();
    ASSERT(box);

    IntRect cornerRect = scrollCornerRect();
    IntRect absRect = IntRect(cornerRect.x() + tx, cornerRect.y() + ty, cornerRect.width(), cornerRect.height());
    if (!absRect.intersects(damageRect))
        return;

    if (context->updatingControlTints()) {
        updateScrollCornerStyle();
        return;
    }

    if (m_scrollCorner) {
        m_scrollCorner->paintIntoRect(context, tx, ty, absRect);
        return;
    }

    // We don't want to paint white if we have overlay scrollbars, since we need
    // to see what is behind it.
    if (!hasOverlayScrollbars())
        context->fillRect(absRect, Color::white, box->style()->colorSpace());
}

void RenderLayer::paintResizer(GraphicsContext* context, int tx, int ty, const IntRect& damageRect)
{
    if (renderer()->style()->resize() == RESIZE_NONE)
        return;

    RenderBox* box = renderBox();
    ASSERT(box);

    IntRect cornerRect = resizerCornerRect(this, box->borderBoxRect());
    IntRect absRect = IntRect(cornerRect.x() + tx, cornerRect.y() + ty, cornerRect.width(), cornerRect.height());
    if (!absRect.intersects(damageRect))
        return;

    if (context->updatingControlTints()) {
        updateResizerStyle();
        return;
    }
    
    if (m_resizer) {
        m_resizer->paintIntoRect(context, tx, ty, absRect);
        return;
    }

    // Paint the resizer control.
    DEFINE_STATIC_LOCAL(RefPtr<Image>, resizeCornerImage, (Image::loadPlatformResource("textAreaResizeCorner")));
    IntPoint imagePoint(absRect.maxX() - resizeCornerImage->width(), absRect.maxY() - resizeCornerImage->height());
    context->drawImage(resizeCornerImage.get(), box->style()->colorSpace(), imagePoint);

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
    if (!renderer()->hasOverflowClip() || renderer()->style()->resize() == RESIZE_NONE)
        return false;
    
    RenderBox* box = renderBox();
    ASSERT(box);

    IntPoint localPoint = absoluteToContents(absolutePoint);

    IntRect localBounds(0, 0, box->width(), box->height());
    return resizerCornerRect(this, localBounds).contains(localPoint);
}
    
bool RenderLayer::hitTestOverflowControls(HitTestResult& result, const IntPoint& localPoint)
{
    if (!m_hBar && !m_vBar && (!renderer()->hasOverflowClip() || renderer()->style()->resize() == RESIZE_NONE))
        return false;

    RenderBox* box = renderBox();
    ASSERT(box);
    
    IntRect resizeControlRect;
    if (renderer()->style()->resize() != RESIZE_NONE) {
        resizeControlRect = resizerCornerRect(this, box->borderBoxRect());
        if (resizeControlRect.contains(localPoint))
            return true;
    }

    int resizeControlSize = max(resizeControlRect.height(), 0);

    if (m_vBar) {
        IntRect vBarRect(box->width() - box->borderRight() - m_vBar->width(), 
                         box->borderTop(),
                         m_vBar->width(),
                         box->height() - (box->borderTop() + box->borderBottom()) - (m_hBar ? m_hBar->height() : resizeControlSize));
        if (vBarRect.contains(localPoint)) {
            result.setScrollbar(m_vBar.get());
            return true;
        }
    }

    resizeControlSize = max(resizeControlRect.width(), 0);
    if (m_hBar) {
        IntRect hBarRect(box->borderLeft(),
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

void RenderLayer::paint(GraphicsContext* p, const IntRect& damageRect, PaintBehavior paintBehavior, RenderObject *paintingRoot)
{
    OverlapTestRequestMap overlapTestRequests;
    paintLayer(this, p, damageRect, paintBehavior, paintingRoot, &overlapTestRequests);
    OverlapTestRequestMap::iterator end = overlapTestRequests.end();
    for (OverlapTestRequestMap::iterator it = overlapTestRequests.begin(); it != end; ++it)
        it->first->setOverlapTestResult(false);
}

void RenderLayer::paintOverlayScrollbars(GraphicsContext* p, const IntRect& damageRect, PaintBehavior paintBehavior, RenderObject *paintingRoot)
{
    if (!m_containsDirtyOverlayScrollbars)
        return;
    paintLayer(this, p, damageRect, paintBehavior, paintingRoot, 0, PaintLayerHaveTransparency | PaintLayerTemporaryClipRects 
               | PaintLayerPaintingOverlayScrollbars);
    m_containsDirtyOverlayScrollbars = false;
}

static void setClip(GraphicsContext* p, const IntRect& paintDirtyRect, const IntRect& clipRect)
{
    if (paintDirtyRect == clipRect)
        return;
    p->save();
    p->clip(clipRect);
}

static void restoreClip(GraphicsContext* p, const IntRect& paintDirtyRect, const IntRect& clipRect)
{
    if (paintDirtyRect == clipRect)
        return;
    p->restore();
}

static void performOverlapTests(OverlapTestRequestMap& overlapTestRequests, const RenderLayer* rootLayer, const RenderLayer* layer)
{
    Vector<OverlapTestRequestClient*> overlappedRequestClients;
    OverlapTestRequestMap::iterator end = overlapTestRequests.end();
    IntRect boundingBox = layer->boundingBox(rootLayer);
    for (OverlapTestRequestMap::iterator it = overlapTestRequests.begin(); it != end; ++it) {
        if (!boundingBox.intersects(it->second))
            continue;

        it->first->setOverlapTestResult(true);
        overlappedRequestClients.append(it->first);
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

void RenderLayer::paintLayer(RenderLayer* rootLayer, GraphicsContext* p,
                        const IntRect& paintDirtyRect, PaintBehavior paintBehavior,
                        RenderObject* paintingRoot, OverlapTestRequestMap* overlapTestRequests,
                        PaintLayerFlags paintFlags)
{
#if USE(ACCELERATED_COMPOSITING)
    if (isComposited()) {
        // The updatingControlTints() painting pass goes through compositing layers,
        // but we need to ensure that we don't cache clip rects computed with the wrong root in this case.
        if (p->updatingControlTints() || (paintBehavior & PaintBehaviorFlattenCompositingLayers))
            paintFlags |= PaintLayerTemporaryClipRects;
        else if (!backing()->paintingGoesToWindow() && !shouldDoSoftwarePaint(this, paintFlags & PaintLayerPaintingReflection)) {
            // If this RenderLayer should paint into its backing, that will be done via RenderLayerBacking::paintIntoLayer().
            return;
        }
    }
#endif

    // Avoid painting layers when stylesheets haven't loaded.  This eliminates FOUC.
    // It's ok not to draw, because later on, when all the stylesheets do load, updateStyleSelector on the Document
    // will do a full repaint().
    if (renderer()->document()->didLayoutWithPendingStylesheets() && !renderer()->isRenderView() && !renderer()->isRoot())
        return;
    
    // If this layer is totally invisible then there is nothing to paint.
    if (!renderer()->opacity())
        return;

    if (paintsWithTransparency(paintBehavior))
        paintFlags |= PaintLayerHaveTransparency;

    // Apply a transform if we have one.  A reflection is considered to be a transform, since it is a flip and a translate.
    if (paintsWithTransform(paintBehavior) && !(paintFlags & PaintLayerAppliedTransform)) {
        TransformationMatrix layerTransform = renderableTransform(paintBehavior);
        // If the transform can't be inverted, then don't paint anything.
        if (!layerTransform.isInvertible())
            return;

        // If we have a transparency layer enclosing us and we are the root of a transform, then we need to establish the transparency
        // layer from the parent now, assuming there is a parent
        if (paintFlags & PaintLayerHaveTransparency) {
            if (parent())
                parent()->beginTransparencyLayers(p, rootLayer, paintBehavior);
            else
                beginTransparencyLayers(p, rootLayer, paintBehavior);
        }

        // Make sure the parent's clip rects have been calculated.
        IntRect clipRect = paintDirtyRect;
        if (parent()) {
            clipRect = backgroundClipRect(rootLayer, paintFlags & PaintLayerTemporaryClipRects);
            clipRect.intersect(paintDirtyRect);
        }
        
        // Push the parent coordinate space's clip.
        setClip(p, paintDirtyRect, clipRect);

        // Adjust the transform such that the renderer's upper left corner will paint at (0,0) in user space.
        // This involves subtracting out the position of the layer in our current coordinate space.
        int x = 0;
        int y = 0;
        convertToLayerCoords(rootLayer, x, y);
        TransformationMatrix transform(layerTransform);
        transform.translateRight(x, y);
        
        // Apply the transform.
        {
            GraphicsContextStateSaver stateSaver(*p);
            p->concatCTM(transform.toAffineTransform());

            // Now do a paint with the root layer shifted to be us.
            paintLayer(this, p, transform.inverse().mapRect(paintDirtyRect), paintBehavior, paintingRoot, overlapTestRequests, paintFlags | PaintLayerAppliedTransform);
        }        

        // Restore the clip.
        restoreClip(p, paintDirtyRect, clipRect);
        
        return;
    }

    PaintLayerFlags localPaintFlags = paintFlags & ~PaintLayerAppliedTransform;
    bool haveTransparency = localPaintFlags & PaintLayerHaveTransparency;

    // Paint the reflection first if we have one.
    if (m_reflection && !m_paintingInsideReflection) {
        // Mark that we are now inside replica painting.
        m_paintingInsideReflection = true;
        reflectionLayer()->paintLayer(rootLayer, p, paintDirtyRect, paintBehavior, paintingRoot, overlapTestRequests, localPaintFlags | PaintLayerPaintingReflection);
        m_paintingInsideReflection = false;
    }

    // Calculate the clip rects we should use.
    IntRect layerBounds, damageRect, clipRectToApply, outlineRect;
    calculateRects(rootLayer, paintDirtyRect, layerBounds, damageRect, clipRectToApply, outlineRect, localPaintFlags & PaintLayerTemporaryClipRects);
    int x = layerBounds.x();
    int y = layerBounds.y();
    int tx = x - renderBoxX();
    int ty = y - renderBoxY();
                             
    // Ensure our lists are up-to-date.
    updateCompositingAndLayerListsIfNeeded();

    bool forceBlackText = paintBehavior & PaintBehaviorForceBlackText;
    bool selectionOnly  = paintBehavior & PaintBehaviorSelectionOnly;
    
    // If this layer's renderer is a child of the paintingRoot, we render unconditionally, which
    // is done by passing a nil paintingRoot down to our renderer (as if no paintingRoot was ever set).
    // Else, our renderer tree may or may not contain the painting root, so we pass that root along
    // so it will be tested against as we descend through the renderers.
    RenderObject* paintingRootForRenderer = 0;
    if (paintingRoot && !renderer()->isDescendantOf(paintingRoot))
        paintingRootForRenderer = paintingRoot;

    if (overlapTestRequests && isSelfPaintingLayer())
        performOverlapTests(*overlapTestRequests, rootLayer, this);

    bool paintingOverlayScrollbars = paintFlags & PaintLayerPaintingOverlayScrollbars;

    // We want to paint our layer, but only if we intersect the damage rect.
    bool shouldPaint = intersectsDamageRect(layerBounds, damageRect, rootLayer) && m_hasVisibleContent && isSelfPaintingLayer();
    if (shouldPaint && !selectionOnly && !damageRect.isEmpty() && !paintingOverlayScrollbars) {
        // Begin transparency layers lazily now that we know we have to paint something.
        if (haveTransparency)
            beginTransparencyLayers(p, rootLayer, paintBehavior);
        
        // Paint our background first, before painting any child layers.
        // Establish the clip used to paint our background.
        setClip(p, paintDirtyRect, damageRect);

        // Paint the background.
        PaintInfo paintInfo(p, damageRect, PaintPhaseBlockBackground, false, paintingRootForRenderer, 0);
        renderer()->paint(paintInfo, tx, ty);

        // Restore the clip.
        restoreClip(p, paintDirtyRect, damageRect);
    }

    // Now walk the sorted list of children with negative z-indices.
    paintList(m_negZOrderList, rootLayer, p, paintDirtyRect, paintBehavior, paintingRoot, overlapTestRequests, localPaintFlags);

    // Now establish the appropriate clip and paint our child RenderObjects.
    if (shouldPaint && !clipRectToApply.isEmpty() && !paintingOverlayScrollbars) {
        // Begin transparency layers lazily now that we know we have to paint something.
        if (haveTransparency)
            beginTransparencyLayers(p, rootLayer, paintBehavior);

        // Set up the clip used when painting our children.
        setClip(p, paintDirtyRect, clipRectToApply);
        PaintInfo paintInfo(p, clipRectToApply, 
                                          selectionOnly ? PaintPhaseSelection : PaintPhaseChildBlockBackgrounds,
                                          forceBlackText, paintingRootForRenderer, 0);
        renderer()->paint(paintInfo, tx, ty);
        if (!selectionOnly) {
            paintInfo.phase = PaintPhaseFloat;
            renderer()->paint(paintInfo, tx, ty);
            paintInfo.phase = PaintPhaseForeground;
            paintInfo.overlapTestRequests = overlapTestRequests;
            renderer()->paint(paintInfo, tx, ty);
            paintInfo.phase = PaintPhaseChildOutlines;
            renderer()->paint(paintInfo, tx, ty);
        }

        // Now restore our clip.
        restoreClip(p, paintDirtyRect, clipRectToApply);
    }
    
    if (!outlineRect.isEmpty() && isSelfPaintingLayer() && !paintingOverlayScrollbars) {
        // Paint our own outline
        PaintInfo paintInfo(p, outlineRect, PaintPhaseSelfOutline, false, paintingRootForRenderer, 0);
        setClip(p, paintDirtyRect, outlineRect);
        renderer()->paint(paintInfo, tx, ty);
        restoreClip(p, paintDirtyRect, outlineRect);
    }
    
    // Paint any child layers that have overflow.
    paintList(m_normalFlowList, rootLayer, p, paintDirtyRect, paintBehavior, paintingRoot, overlapTestRequests, localPaintFlags);
    
    // Now walk the sorted list of children with positive z-indices.
    paintList(m_posZOrderList, rootLayer, p, paintDirtyRect, paintBehavior, paintingRoot, overlapTestRequests, localPaintFlags);
        
    if (renderer()->hasMask() && shouldPaint && !selectionOnly && !damageRect.isEmpty() && !paintingOverlayScrollbars) {
        setClip(p, paintDirtyRect, damageRect);

        // Paint the mask.
        PaintInfo paintInfo(p, damageRect, PaintPhaseMask, false, paintingRootForRenderer, 0);
        renderer()->paint(paintInfo, tx, ty);
        
        // Restore the clip.
        restoreClip(p, paintDirtyRect, damageRect);
    }

    if (paintingOverlayScrollbars) {
        setClip(p, paintDirtyRect, damageRect);
        paintOverflowControls(p, tx, ty, damageRect, true);
        restoreClip(p, paintDirtyRect, damageRect);
    }

    // End our transparency layer
    if (haveTransparency && m_usedTransparency && !m_paintingInsideReflection) {
        p->endTransparencyLayer();
        p->restore();
        m_usedTransparency = false;
    }
}

void RenderLayer::paintList(Vector<RenderLayer*>* list, RenderLayer* rootLayer, GraphicsContext* p,
                            const IntRect& paintDirtyRect, PaintBehavior paintBehavior,
                            RenderObject* paintingRoot, OverlapTestRequestMap* overlapTestRequests,
                            PaintLayerFlags paintFlags)
{
    if (!list)
        return;
    
    for (size_t i = 0; i < list->size(); ++i) {
        RenderLayer* childLayer = list->at(i);
        if (!childLayer->isPaginated())
            childLayer->paintLayer(rootLayer, p, paintDirtyRect, paintBehavior, paintingRoot, overlapTestRequests, paintFlags);
        else
            paintPaginatedChildLayer(childLayer, rootLayer, p, paintDirtyRect, paintBehavior, paintingRoot, overlapTestRequests, paintFlags);
    }
}

void RenderLayer::paintPaginatedChildLayer(RenderLayer* childLayer, RenderLayer* rootLayer, GraphicsContext* context,
                                           const IntRect& paintDirtyRect, PaintBehavior paintBehavior,
                                           RenderObject* paintingRoot, OverlapTestRequestMap* overlapTestRequests,
                                           PaintLayerFlags paintFlags)
{
    // We need to do multiple passes, breaking up our child layer into strips.
    Vector<RenderLayer*> columnLayers;
    RenderLayer* ancestorLayer = isNormalFlowOnly() ? parent() : stackingContext();
    for (RenderLayer* curr = childLayer->parent(); curr; curr = curr->parent()) {
        if (curr->renderer()->hasColumns() && checkContainingBlockChainForPagination(childLayer->renderer(), curr->renderBox()))
            columnLayers.append(curr);
        if (curr == ancestorLayer)
            break;
    }

    ASSERT(columnLayers.size());
    
    paintChildLayerIntoColumns(childLayer, rootLayer, context, paintDirtyRect, paintBehavior, paintingRoot, overlapTestRequests, paintFlags, columnLayers, columnLayers.size() - 1);
}

void RenderLayer::paintChildLayerIntoColumns(RenderLayer* childLayer, RenderLayer* rootLayer, GraphicsContext* context,
                                             const IntRect& paintDirtyRect, PaintBehavior paintBehavior,
                                             RenderObject* paintingRoot, OverlapTestRequestMap* overlapTestRequests,
                                             PaintLayerFlags paintFlags, const Vector<RenderLayer*>& columnLayers, size_t colIndex)
{
    RenderBlock* columnBlock = toRenderBlock(columnLayers[colIndex]->renderer());

    ASSERT(columnBlock && columnBlock->hasColumns());
    if (!columnBlock || !columnBlock->hasColumns())
        return;
    
    int layerX = 0;
    int layerY = 0;
    columnBlock->layer()->convertToLayerCoords(rootLayer, layerX, layerY);
    
    bool isHorizontal = columnBlock->style()->isHorizontalWritingMode();

    ColumnInfo* colInfo = columnBlock->columnInfo();
    unsigned colCount = columnBlock->columnCount(colInfo);
    int currLogicalTopOffset = 0;
    for (unsigned i = 0; i < colCount; i++) {
        // For each rect, we clip to the rect, and then we adjust our coords.
        IntRect colRect = columnBlock->columnRectAt(colInfo, i);
        columnBlock->flipForWritingMode(colRect);
        int logicalLeftOffset = (isHorizontal ? colRect.x() : colRect.y()) - columnBlock->logicalLeftOffsetForContent();
        IntSize offset = isHorizontal ? IntSize(logicalLeftOffset, currLogicalTopOffset) : IntSize(currLogicalTopOffset, logicalLeftOffset);

        colRect.move(layerX, layerY);

        IntRect localDirtyRect(paintDirtyRect);
        localDirtyRect.intersect(colRect);
        
        if (!localDirtyRect.isEmpty()) {
            GraphicsContextStateSaver stateSaver(*context);
            
            // Each strip pushes a clip, since column boxes are specified as being
            // like overflow:hidden.
            context->clip(colRect);

            if (!colIndex) {
                // Apply a translation transform to change where the layer paints.
                TransformationMatrix oldTransform;
                bool oldHasTransform = childLayer->transform();
                if (oldHasTransform)
                    oldTransform = *childLayer->transform();
                TransformationMatrix newTransform(oldTransform);
                newTransform.translateRight(offset.width(), offset.height());
                
                childLayer->m_transform = adoptPtr(new TransformationMatrix(newTransform));
                childLayer->paintLayer(rootLayer, context, localDirtyRect, paintBehavior, paintingRoot, overlapTestRequests, paintFlags);
                if (oldHasTransform)
                    childLayer->m_transform = adoptPtr(new TransformationMatrix(oldTransform));
                else
                    childLayer->m_transform.clear();
            } else {
                // Adjust the transform such that the renderer's upper left corner will paint at (0,0) in user space.
                // This involves subtracting out the position of the layer in our current coordinate space.
                int childX = 0;
                int childY = 0;
                columnLayers[colIndex - 1]->convertToLayerCoords(rootLayer, childX, childY);
                TransformationMatrix transform;
                transform.translateRight(childX + offset.width(), childY + offset.height());
                
                // Apply the transform.
                context->concatCTM(transform.toAffineTransform());

                // Now do a paint with the root layer shifted to be the next multicol block.
                paintChildLayerIntoColumns(childLayer, columnLayers[colIndex - 1], context, transform.inverse().mapRect(localDirtyRect), paintBehavior, 
                                           paintingRoot, overlapTestRequests, paintFlags, 
                                           columnLayers, colIndex - 1);
            }
        }

        // Move to the next position.
        int blockDelta = isHorizontal ? colRect.height() : colRect.width();
        if (columnBlock->style()->isFlippedBlocksWritingMode())
            currLogicalTopOffset += blockDelta;
        else
            currLogicalTopOffset -= blockDelta;
    }
}

static inline IntRect frameVisibleRect(RenderObject* renderer)
{
    FrameView* frameView = renderer->document()->view();
    if (!frameView)
        return IntRect();

    return frameView->visibleContentRect();
}

bool RenderLayer::hitTest(const HitTestRequest& request, HitTestResult& result)
{
    renderer()->document()->updateLayout();
    
    IntRect hitTestArea = renderer()->view()->documentRect();
    if (!request.ignoreClipping())
        hitTestArea.intersect(frameVisibleRect(renderer()));

    RenderLayer* insideLayer = hitTestLayer(this, 0, request, result, hitTestArea, result.point(), false);
    if (!insideLayer) {
        // We didn't hit any layer. If we are the root layer and the mouse is -- or just was -- down, 
        // return ourselves. We do this so mouse events continue getting delivered after a drag has 
        // exited the WebView, and so hit testing over a scrollbar hits the content document.
        if ((request.active() || request.mouseUp()) && renderer()->isRenderView()) {
            renderer()->updateHitTestResult(result, result.point());
            insideLayer = this;
        }
    }

    // Now determine if the result is inside an anchor - if the urlElement isn't already set.
    Node* node = result.innerNode();
    if (node && !result.URLElement())
        result.setURLElement(static_cast<Element*>(node->enclosingLinkEventParentOrSelf()));

    // Next set up the correct :hover/:active state along the new chain.
    updateHoverActiveState(request, result);
    
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
                                        const IntRect& hitTestRect, const IntPoint& hitTestPoint,
                                        const HitTestingTransformState* containerTransformState) const
{
    RefPtr<HitTestingTransformState> transformState;
    int offsetX = 0;
    int offsetY = 0;
    if (containerTransformState) {
        // If we're already computing transform state, then it's relative to the container (which we know is non-null).
        transformState = HitTestingTransformState::create(*containerTransformState);
        convertToLayerCoords(containerLayer, offsetX, offsetY);
    } else {
        // If this is the first time we need to make transform state, then base it off of hitTestPoint,
        // which is relative to rootLayer.
        transformState = HitTestingTransformState::create(hitTestPoint, FloatQuad(hitTestRect));
        convertToLayerCoords(rootLayer, offsetX, offsetY);
    }
    
    RenderObject* containerRenderer = containerLayer ? containerLayer->renderer() : 0;
    if (renderer()->shouldUseTransformFromContainer(containerRenderer)) {
        TransformationMatrix containerTransform;
        renderer()->getTransformFromContainer(containerRenderer, IntSize(offsetX, offsetY), containerTransform);
        transformState->applyTransform(containerTransform, HitTestingTransformState::AccumulateTransform);
    } else {
        transformState->translate(offsetX, offsetY, HitTestingTransformState::AccumulateTransform);
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

// hitTestPoint and hitTestRect are relative to rootLayer.
// A 'flattening' layer is one preserves3D() == false.
// transformState.m_accumulatedTransform holds the transform from the containing flattening layer.
// transformState.m_lastPlanarPoint is the hitTestPoint in the plane of the containing flattening layer.
// transformState.m_lastPlanarQuad is the hitTestRect as a quad in the plane of the containing flattening layer.
// 
// If zOffset is non-null (which indicates that the caller wants z offset information), 
//  *zOffset on return is the z offset of the hit point relative to the containing flattening layer.
RenderLayer* RenderLayer::hitTestLayer(RenderLayer* rootLayer, RenderLayer* containerLayer, const HitTestRequest& request, HitTestResult& result,
                                       const IntRect& hitTestRect, const IntPoint& hitTestPoint, bool appliedTransform,
                                       const HitTestingTransformState* transformState, double* zOffset)
{
    // The natural thing would be to keep HitTestingTransformState on the stack, but it's big, so we heap-allocate.

    bool useTemporaryClipRects = false;
#if USE(ACCELERATED_COMPOSITING)
    useTemporaryClipRects = compositor()->inCompositingMode();
#endif
    useTemporaryClipRects |= renderer()->view()->frameView()->containsScrollableAreaWithOverlayScrollbars();

    IntRect hitTestArea = result.rectForPoint(hitTestPoint);

    // Apply a transform if we have one.
    if (transform() && !appliedTransform) {
        // Make sure the parent's clip rects have been calculated.
        if (parent()) {
            IntRect clipRect = backgroundClipRect(rootLayer, useTemporaryClipRects, IncludeOverlayScrollbarSize);
            // Go ahead and test the enclosing clip now.
            if (!clipRect.intersects(hitTestArea))
                return 0;
        }

        // Create a transform state to accumulate this transform.
        RefPtr<HitTestingTransformState> newTransformState = createLocalTransformState(rootLayer, containerLayer, hitTestRect, hitTestPoint, transformState);

        // If the transform can't be inverted, then don't hit test this layer at all.
        if (!newTransformState->m_accumulatedTransform.isInvertible())
            return 0;

        // Compute the point and the hit test rect in the coords of this layer by using the values
        // from the transformState, which store the point and quad in the coords of the last flattened
        // layer, and the accumulated transform which lets up map through preserve-3d layers.
        //
        // We can't just map hitTestPoint and hitTestRect because they may have been flattened (losing z)
        // by our container.
        IntPoint localPoint = roundedIntPoint(newTransformState->mappedPoint());
        IntRect localHitTestRect;
#if USE(ACCELERATED_COMPOSITING)
        if (isComposited()) {
            // It doesn't make sense to project hitTestRect into the plane of this layer, so use the same bounds we use for painting.
            localHitTestRect = backing()->compositedBounds();
        } else
#endif
            localHitTestRect = newTransformState->mappedQuad().enclosingBoundingBox();

        // Now do a hit test with the root layer shifted to be us.
        return hitTestLayer(this, containerLayer, request, result, localHitTestRect, localPoint, true, newTransformState.get(), zOffset);
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
        localTransformState = createLocalTransformState(rootLayer, containerLayer, hitTestRect, hitTestPoint, transformState);
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
    
    // Calculate the clip rects we should use.
    IntRect layerBounds;
    IntRect bgRect;
    IntRect fgRect;
    IntRect outlineRect;
    calculateRects(rootLayer, hitTestRect, layerBounds, bgRect, fgRect, outlineRect, useTemporaryClipRects, IncludeOverlayScrollbarSize);
    
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
    RenderLayer* hitLayer = hitTestList(m_posZOrderList, rootLayer, request, result, hitTestRect, hitTestPoint,
                                        localTransformState.get(), zOffsetForDescendantsPtr, zOffset, unflattenedTransformState.get(), depthSortDescendants);
    if (hitLayer) {
        if (!depthSortDescendants)
            return hitLayer;
        candidateLayer = hitLayer;
    }

    // Now check our overflow objects.
    hitLayer = hitTestList(m_normalFlowList, rootLayer, request, result, hitTestRect, hitTestPoint,
                           localTransformState.get(), zOffsetForDescendantsPtr, zOffset, unflattenedTransformState.get(), depthSortDescendants);
    if (hitLayer) {
        if (!depthSortDescendants)
            return hitLayer;
        candidateLayer = hitLayer;
    }

    // Next we want to see if the mouse pos is inside the child RenderObjects of the layer.
    if (fgRect.intersects(hitTestArea) && isSelfPaintingLayer()) {
        // Hit test with a temporary HitTestResult, because we only want to commit to 'result' if we know we're frontmost.
        HitTestResult tempResult(result.point(), result.topPadding(), result.rightPadding(), result.bottomPadding(), result.leftPadding());
        if (hitTestContents(request, tempResult, layerBounds, hitTestPoint, HitTestDescendants) &&
            isHitCandidate(this, false, zOffsetForContentsPtr, unflattenedTransformState.get())) {
            if (result.isRectBasedTest())
                result.append(tempResult);
            else
                result = tempResult;
            if (!depthSortDescendants)
                return this;
            // Foreground can depth-sort with descendant layers, so keep this as a candidate.
            candidateLayer = this;
        } else if (result.isRectBasedTest())
            result.append(tempResult);
    }

    // Now check our negative z-index children.
    hitLayer = hitTestList(m_negZOrderList, rootLayer, request, result, hitTestRect, hitTestPoint,
                                        localTransformState.get(), zOffsetForDescendantsPtr, zOffset, unflattenedTransformState.get(), depthSortDescendants);
    if (hitLayer) {
        if (!depthSortDescendants)
            return hitLayer;
        candidateLayer = hitLayer;
    }

    // If we found a layer, return. Child layers, and foreground always render in front of background.
    if (candidateLayer)
        return candidateLayer;

    if (bgRect.intersects(hitTestArea) && isSelfPaintingLayer()) {
        HitTestResult tempResult(result.point(), result.topPadding(), result.rightPadding(), result.bottomPadding(), result.leftPadding());
        if (hitTestContents(request, tempResult, layerBounds, hitTestPoint, HitTestSelf) &&
            isHitCandidate(this, false, zOffsetForContentsPtr, unflattenedTransformState.get())) {
            if (result.isRectBasedTest())
                result.append(tempResult);
            else
                result = tempResult;
            return this;
        } else if (result.isRectBasedTest())
            result.append(tempResult);
    }
    
    return 0;
}

bool RenderLayer::hitTestContents(const HitTestRequest& request, HitTestResult& result, const IntRect& layerBounds, const IntPoint& hitTestPoint, HitTestFilter hitTestFilter) const
{
    if (!renderer()->hitTest(request, result, hitTestPoint,
                            layerBounds.x() - renderBoxX(),
                            layerBounds.y() - renderBoxY(), 
                            hitTestFilter)) {
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
                                      const IntRect& hitTestRect, const IntPoint& hitTestPoint,
                                      const HitTestingTransformState* transformState, 
                                      double* zOffsetForDescendants, double* zOffset,
                                      const HitTestingTransformState* unflattenedTransformState,
                                      bool depthSortDescendants)
{
    if (!list)
        return 0;
    
    RenderLayer* resultLayer = 0;
    for (int i = list->size() - 1; i >= 0; --i) {
        RenderLayer* childLayer = list->at(i);
        RenderLayer* hitLayer = 0;
        HitTestResult tempResult(result.point(), result.topPadding(), result.rightPadding(), result.bottomPadding(), result.leftPadding());
        if (childLayer->isPaginated())
            hitLayer = hitTestPaginatedChildLayer(childLayer, rootLayer, request, tempResult, hitTestRect, hitTestPoint, transformState, zOffsetForDescendants);
        else
            hitLayer = childLayer->hitTestLayer(rootLayer, this, request, tempResult, hitTestRect, hitTestPoint, false, transformState, zOffsetForDescendants);

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
                                                     const IntRect& hitTestRect, const IntPoint& hitTestPoint, const HitTestingTransformState* transformState, double* zOffset)
{
    Vector<RenderLayer*> columnLayers;
    RenderLayer* ancestorLayer = isNormalFlowOnly() ? parent() : stackingContext();
    for (RenderLayer* curr = childLayer->parent(); curr; curr = curr->parent()) {
        if (curr->renderer()->hasColumns() && checkContainingBlockChainForPagination(childLayer->renderer(), curr->renderBox()))
            columnLayers.append(curr);
        if (curr == ancestorLayer)
            break;
    }

    ASSERT(columnLayers.size());
    return hitTestChildLayerColumns(childLayer, rootLayer, request, result, hitTestRect, hitTestPoint, transformState, zOffset,
                                    columnLayers, columnLayers.size() - 1);
}

RenderLayer* RenderLayer::hitTestChildLayerColumns(RenderLayer* childLayer, RenderLayer* rootLayer, const HitTestRequest& request, HitTestResult& result,
                                                   const IntRect& hitTestRect, const IntPoint& hitTestPoint, const HitTestingTransformState* transformState, double* zOffset,
                                                   const Vector<RenderLayer*>& columnLayers, size_t columnIndex)
{
    RenderBlock* columnBlock = toRenderBlock(columnLayers[columnIndex]->renderer());

    ASSERT(columnBlock && columnBlock->hasColumns());
    if (!columnBlock || !columnBlock->hasColumns())
        return 0;
    
    int layerX = 0;
    int layerY = 0;
    columnBlock->layer()->convertToLayerCoords(rootLayer, layerX, layerY);
    
    ColumnInfo* colInfo = columnBlock->columnInfo();
    int colCount = columnBlock->columnCount(colInfo);
    
    // We have to go backwards from the last column to the first.
    bool isHorizontal = columnBlock->style()->isHorizontalWritingMode();
    int logicalLeft = columnBlock->logicalLeftOffsetForContent();
    int currLogicalTopOffset = 0;
    int i;
    for (i = 0; i < colCount; i++) {
        IntRect colRect = columnBlock->columnRectAt(colInfo, i);
        int blockDelta =  (isHorizontal ? colRect.height() : colRect.width());
        if (columnBlock->style()->isFlippedBlocksWritingMode())
            currLogicalTopOffset += blockDelta;
        else
            currLogicalTopOffset -= blockDelta;
    }
    for (i = colCount - 1; i >= 0; i--) {
        // For each rect, we clip to the rect, and then we adjust our coords.
        IntRect colRect = columnBlock->columnRectAt(colInfo, i);
        columnBlock->flipForWritingMode(colRect);
        int currLogicalLeftOffset = (isHorizontal ? colRect.x() : colRect.y()) - logicalLeft;
        int blockDelta =  (isHorizontal ? colRect.height() : colRect.width());
        if (columnBlock->style()->isFlippedBlocksWritingMode())
            currLogicalTopOffset -= blockDelta;
        else
            currLogicalTopOffset += blockDelta;
        colRect.move(layerX, layerY);

        IntRect localClipRect(hitTestRect);
        localClipRect.intersect(colRect);
        
        IntSize offset = isHorizontal ? IntSize(currLogicalLeftOffset, currLogicalTopOffset) : IntSize(currLogicalTopOffset, currLogicalLeftOffset);

        if (!localClipRect.isEmpty() && localClipRect.intersects(result.rectForPoint(hitTestPoint))) {
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
                hitLayer = childLayer->hitTestLayer(rootLayer, columnLayers[0], request, result, localClipRect, hitTestPoint, false, transformState, zOffset);
                if (oldHasTransform)
                    childLayer->m_transform = adoptPtr(new TransformationMatrix(oldTransform));
                else
                    childLayer->m_transform.clear();
            } else {
                // Adjust the transform such that the renderer's upper left corner will be at (0,0) in user space.
                // This involves subtracting out the position of the layer in our current coordinate space.
                RenderLayer* nextLayer = columnLayers[columnIndex - 1];
                RefPtr<HitTestingTransformState> newTransformState = nextLayer->createLocalTransformState(rootLayer, nextLayer, localClipRect, hitTestPoint, transformState);
                newTransformState->translate(offset.width(), offset.height(), HitTestingTransformState::AccumulateTransform);
                IntPoint localPoint = roundedIntPoint(newTransformState->mappedPoint());
                IntRect localHitTestRect = newTransformState->mappedQuad().enclosingBoundingBox();
                newTransformState->flatten();

                hitLayer = hitTestChildLayerColumns(childLayer, columnLayers[columnIndex - 1], request, result, localHitTestRect, localPoint,
                                                    newTransformState.get(), zOffset, columnLayers, columnIndex - 1);
            }

            if (hitLayer)
                return hitLayer;
        }
    }

    return 0;
}

void RenderLayer::updateClipRects(const RenderLayer* rootLayer, OverlayScrollbarSizeRelevancy relevancy)
{
    if (m_clipRects) {
        ASSERT(rootLayer == m_clipRectsRoot);
        return; // We have the correct cached value.
    }
    
    // For transformed layers, the root layer was shifted to be us, so there is no need to
    // examine the parent.  We want to cache clip rects with us as the root.
    RenderLayer* parentLayer = rootLayer != this ? parent() : 0;
    if (parentLayer)
        parentLayer->updateClipRects(rootLayer, relevancy);

    ClipRects clipRects;
    calculateClipRects(rootLayer, clipRects, true, relevancy);

    if (parentLayer && parentLayer->clipRects() && clipRects == *parentLayer->clipRects())
        m_clipRects = parentLayer->clipRects();
    else
        m_clipRects = new (renderer()->renderArena()) ClipRects(clipRects);
    m_clipRects->ref();
#ifndef NDEBUG
    m_clipRectsRoot = rootLayer;
#endif
}

void RenderLayer::calculateClipRects(const RenderLayer* rootLayer, ClipRects& clipRects, bool useCached, OverlayScrollbarSizeRelevancy relevancy) const
{
    if (!parent()) {
        // The root layer's clip rect is always infinite.
        clipRects.reset(PaintInfo::infiniteRect());
        return;
    }

    // For transformed layers, the root layer was shifted to be us, so there is no need to
    // examine the parent.  We want to cache clip rects with us as the root.
    RenderLayer* parentLayer = rootLayer != this ? parent() : 0;
    
    // Ensure that our parent's clip has been calculated so that we can examine the values.
    if (parentLayer) {
        if (useCached && parentLayer->clipRects())
            clipRects = *parentLayer->clipRects();
        else
            parentLayer->calculateClipRects(rootLayer, clipRects);
    }
    else
        clipRects.reset(PaintInfo::infiniteRect());

    // A fixed object is essentially the root of its containing block hierarchy, so when
    // we encounter such an object, we reset our clip rects to the fixedClipRect.
    if (renderer()->style()->position() == FixedPosition) {
        clipRects.setPosClipRect(clipRects.fixedClipRect());
        clipRects.setOverflowClipRect(clipRects.fixedClipRect());
        clipRects.setFixed(true);
    }
    else if (renderer()->style()->position() == RelativePosition)
        clipRects.setPosClipRect(clipRects.overflowClipRect());
    else if (renderer()->style()->position() == AbsolutePosition)
        clipRects.setOverflowClipRect(clipRects.posClipRect());
    
    // Update the clip rects that will be passed to child layers.
    if (renderer()->hasOverflowClip() || renderer()->hasClip()) {
        // This layer establishes a clip of some kind.
        int x = 0;
        int y = 0;
        convertToLayerCoords(rootLayer, x, y);
        RenderView* view = renderer()->view();
        ASSERT(view);
        if (view && clipRects.fixed() && rootLayer->renderer() == view) {
            x -= view->frameView()->scrollXForFixedPosition();
            y -= view->frameView()->scrollYForFixedPosition();
        }
        
        if (renderer()->hasOverflowClip()) {
            IntRect newOverflowClip = toRenderBox(renderer())->overflowClipRect(x, y, relevancy);
            clipRects.setOverflowClipRect(intersection(newOverflowClip, clipRects.overflowClipRect()));
            if (renderer()->isPositioned() || renderer()->isRelPositioned())
                clipRects.setPosClipRect(intersection(newOverflowClip, clipRects.posClipRect()));
        }
        if (renderer()->hasClip()) {
            IntRect newPosClip = toRenderBox(renderer())->clipRect(x, y);
            clipRects.setPosClipRect(intersection(newPosClip, clipRects.posClipRect()));
            clipRects.setOverflowClipRect(intersection(newPosClip, clipRects.overflowClipRect()));
            clipRects.setFixedClipRect(intersection(newPosClip, clipRects.fixedClipRect()));
        }
    }
}

void RenderLayer::parentClipRects(const RenderLayer* rootLayer, ClipRects& clipRects, bool temporaryClipRects, OverlayScrollbarSizeRelevancy relevancy) const
{
    ASSERT(parent());
    if (temporaryClipRects) {
        parent()->calculateClipRects(rootLayer, clipRects, false, relevancy);
        return;
    }

    parent()->updateClipRects(rootLayer, relevancy);
    clipRects = *parent()->clipRects();
}

IntRect RenderLayer::backgroundClipRect(const RenderLayer* rootLayer, bool temporaryClipRects, OverlayScrollbarSizeRelevancy relevancy) const
{
    IntRect backgroundRect;
    if (parent()) {
        ClipRects parentRects;
        parentClipRects(rootLayer, parentRects, temporaryClipRects, relevancy);
        backgroundRect = renderer()->style()->position() == FixedPosition ? parentRects.fixedClipRect() :
                         (renderer()->isPositioned() ? parentRects.posClipRect() : 
                                                       parentRects.overflowClipRect());
        RenderView* view = renderer()->view();
        ASSERT(view);
        if (view && parentRects.fixed() && rootLayer->renderer() == view)
            backgroundRect.move(view->frameView()->scrollXForFixedPosition(), view->frameView()->scrollYForFixedPosition());
    }
    return backgroundRect;
}

void RenderLayer::calculateRects(const RenderLayer* rootLayer, const IntRect& paintDirtyRect, IntRect& layerBounds,
                                 IntRect& backgroundRect, IntRect& foregroundRect, IntRect& outlineRect, bool temporaryClipRects,
                                 OverlayScrollbarSizeRelevancy relevancy) const
{
    if (rootLayer != this && parent()) {
        backgroundRect = backgroundClipRect(rootLayer, temporaryClipRects, relevancy);
        backgroundRect.intersect(paintDirtyRect);
    } else
        backgroundRect = paintDirtyRect;

    foregroundRect = backgroundRect;
    outlineRect = backgroundRect;
    
    int x = 0;
    int y = 0;
    convertToLayerCoords(rootLayer, x, y);
    layerBounds = IntRect(x, y, width(), height());
    
    // Update the clip rects that will be passed to child layers.
    if (renderer()->hasOverflowClip() || renderer()->hasClip()) {
        // This layer establishes a clip of some kind.
        if (renderer()->hasOverflowClip())
            foregroundRect.intersect(toRenderBox(renderer())->overflowClipRect(x, y, relevancy));
        if (renderer()->hasClip()) {
            // Clip applies to *us* as well, so go ahead and update the damageRect.
            IntRect newPosClip = toRenderBox(renderer())->clipRect(x, y);
            backgroundRect.intersect(newPosClip);
            foregroundRect.intersect(newPosClip);
            outlineRect.intersect(newPosClip);
        }

        // If we establish a clip at all, then go ahead and make sure our background
        // rect is intersected with our layer's bounds.
        // FIXME: This could be changed to just use generic visual overflow.
        // See https://bugs.webkit.org/show_bug.cgi?id=37467 for more information.
        if (const ShadowData* boxShadow = renderer()->style()->boxShadow()) {
            IntRect overflow = layerBounds;
            do {
                if (boxShadow->style() == Normal) {
                    IntRect shadowRect = layerBounds;
                    shadowRect.move(boxShadow->x(), boxShadow->y());
                    shadowRect.inflate(boxShadow->blur() + boxShadow->spread());
                    overflow.unite(shadowRect);
                }

                boxShadow = boxShadow->next();
            } while (boxShadow);
            backgroundRect.intersect(overflow);
        } else
            backgroundRect.intersect(layerBounds);
    }
}

IntRect RenderLayer::childrenClipRect() const
{
    RenderView* renderView = renderer()->view();
    RenderLayer* clippingRootLayer = clippingRoot();
    IntRect layerBounds, backgroundRect, foregroundRect, outlineRect;
    calculateRects(clippingRootLayer, renderView->documentRect(), layerBounds, backgroundRect, foregroundRect, outlineRect);
    return clippingRootLayer->renderer()->localToAbsoluteQuad(FloatQuad(foregroundRect)).enclosingBoundingBox();
}

IntRect RenderLayer::selfClipRect() const
{
    RenderView* renderView = renderer()->view();
    RenderLayer* clippingRootLayer = clippingRoot();
    IntRect layerBounds, backgroundRect, foregroundRect, outlineRect;
    calculateRects(clippingRootLayer, renderView->documentRect(), layerBounds, backgroundRect, foregroundRect, outlineRect);
    return clippingRootLayer->renderer()->localToAbsoluteQuad(FloatQuad(backgroundRect)).enclosingBoundingBox();
}

void RenderLayer::addBlockSelectionGapsBounds(const IntRect& bounds)
{
    m_blockSelectionGapsBounds.unite(bounds);
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

    IntRect rect = m_blockSelectionGapsBounds;
    rect.move(-scrolledContentOffset());
    if (renderer()->hasOverflowClip())
        rect.intersect(toRenderBox(renderer())->overflowClipRect(0, 0));
    if (renderer()->hasClip())
        rect.intersect(toRenderBox(renderer())->clipRect(0, 0));
    if (!rect.isEmpty())
        renderer()->repaintRectangle(rect);
}

bool RenderLayer::intersectsDamageRect(const IntRect& layerBounds, const IntRect& damageRect, const RenderLayer* rootLayer) const
{
    // Always examine the canvas and the root.
    // FIXME: Could eliminate the isRoot() check if we fix background painting so that the RenderView
    // paints the root's background.
    if (renderer()->isRenderView() || renderer()->isRoot())
        return true;

    // If we aren't an inline flow, and our layer bounds do intersect the damage rect, then we 
    // can go ahead and return true.
    RenderView* view = renderer()->view();
    ASSERT(view);
    if (view && !renderer()->isRenderInline()) {
        IntRect b = layerBounds;
        b.inflate(view->maximalOutlineSize());
        if (b.intersects(damageRect))
            return true;
    }
        
    // Otherwise we need to compute the bounding box of this single layer and see if it intersects
    // the damage rect.
    return boundingBox(rootLayer).intersects(damageRect);
}

IntRect RenderLayer::localBoundingBox() const
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
    IntRect result;
    if (renderer()->isRenderInline())
        result = toRenderInline(renderer())->linesVisualOverflowBoundingBox();
    else if (renderer()->isTableRow()) {
        // Our bounding box is just the union of all of our cells' border/overflow rects.
        for (RenderObject* child = renderer()->firstChild(); child; child = child->nextSibling()) {
            if (child->isTableCell()) {
                IntRect bbox = toRenderBox(child)->borderBoxRect();
                result.unite(bbox);
                IntRect overflowRect = renderBox()->visualOverflowRect();
                if (bbox != overflowRect)
                    result.unite(overflowRect);
            }
        }
    } else {
        RenderBox* box = renderBox();
        ASSERT(box);
        if (box->hasMask())
            result = box->maskClipRect();
        else {
            IntRect bbox = box->borderBoxRect();
            result = bbox;
            IntRect overflowRect = box->visualOverflowRect();
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

IntRect RenderLayer::boundingBox(const RenderLayer* ancestorLayer) const
{    
    IntRect result = localBoundingBox();
    if (renderer()->isBox())
        renderBox()->flipForWritingMode(result);
    else
        renderer()->containingBlock()->flipForWritingMode(result);
    int deltaX = 0, deltaY = 0;
    convertToLayerCoords(ancestorLayer, deltaX, deltaY);
    result.move(deltaX, deltaY);
    return result;
}

IntRect RenderLayer::absoluteBoundingBox() const
{
    return boundingBox(root());
}

void RenderLayer::clearClipRectsIncludingDescendants()
{
    if (!m_clipRects)
        return;

    clearClipRects();
    
    for (RenderLayer* l = firstChild(); l; l = l->nextSibling())
        l->clearClipRectsIncludingDescendants();
}

void RenderLayer::clearClipRects()
{
    if (m_clipRects) {
        m_clipRects->deref(renderer()->renderArena());
        m_clipRects = 0;
#ifndef NDEBUG
        m_clipRectsRoot = 0;
#endif    
    }
}

#if USE(ACCELERATED_COMPOSITING)
RenderLayerBacking* RenderLayer::ensureBacking()
{
    if (!m_backing)
        m_backing = adoptPtr(new RenderLayerBacking(this));
    return m_backing.get();
}

void RenderLayer::clearBacking()
{
    m_backing.clear();
}

bool RenderLayer::hasCompositedMask() const
{
    return m_backing && m_backing->hasMaskLayer();
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
    bool paintsToWindow = !isComposited() || backing()->paintingGoesToWindow();
#else
    bool paintsToWindow = true;
#endif    
    return transform() && ((paintBehavior & PaintBehaviorFlattenCompositingLayers) || paintsToWindow);
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

static RenderObject* commonAncestor(RenderObject* obj1, RenderObject* obj2)
{
    if (!obj1 || !obj2)
        return 0;

    for (RenderObject* currObj1 = obj1; currObj1; currObj1 = currObj1->hoverAncestor())
        for (RenderObject* currObj2 = obj2; currObj2; currObj2 = currObj2->hoverAncestor())
            if (currObj1 == currObj2)
                return currObj1;

    return 0;
}

void RenderLayer::updateHoverActiveState(const HitTestRequest& request, HitTestResult& result)
{
    // We don't update :hover/:active state when the result is marked as readOnly.
    if (request.readOnly())
        return;

    Document* doc = renderer()->document();

    Node* activeNode = doc->activeNode();
    if (activeNode && !request.active()) {
        // We are clearing the :active chain because the mouse has been released.
        for (RenderObject* curr = activeNode->renderer(); curr; curr = curr->parent()) {
            if (curr->node() && !curr->isText())
                curr->node()->clearInActiveChain();
        }
        doc->setActiveNode(0);
    } else {
        Node* newActiveNode = result.innerNode();
        if (!activeNode && newActiveNode && request.active()) {
            // We are setting the :active chain and freezing it. If future moves happen, they
            // will need to reference this chain.
            for (RenderObject* curr = newActiveNode->renderer(); curr; curr = curr->parent()) {
                if (curr->node() && !curr->isText()) {
                    curr->node()->setInActiveChain();
                }
            }
            doc->setActiveNode(newActiveNode);
        }
    }

    // If the mouse is down and if this is a mouse move event, we want to restrict changes in 
    // :hover/:active to only apply to elements that are in the :active chain that we froze
    // at the time the mouse went down.
    bool mustBeInActiveChain = request.active() && request.mouseMove();

    // Check to see if the hovered node has changed.  If not, then we don't need to
    // do anything.  
    RefPtr<Node> oldHoverNode = doc->hoverNode();
    Node* newHoverNode = result.innerNode();

    // Update our current hover node.
    doc->setHoverNode(newHoverNode);

    // We have two different objects.  Fetch their renderers.
    RenderObject* oldHoverObj = oldHoverNode ? oldHoverNode->renderer() : 0;
    RenderObject* newHoverObj = newHoverNode ? newHoverNode->renderer() : 0;
    
    // Locate the common ancestor render object for the two renderers.
    RenderObject* ancestor = commonAncestor(oldHoverObj, newHoverObj);

    Vector<RefPtr<Node>, 32> nodesToRemoveFromChain;
    Vector<RefPtr<Node>, 32> nodesToAddToChain;

    if (oldHoverObj != newHoverObj) {
        // The old hover path only needs to be cleared up to (and not including) the common ancestor;
        for (RenderObject* curr = oldHoverObj; curr && curr != ancestor; curr = curr->hoverAncestor()) {
            if (curr->node() && !curr->isText() && (!mustBeInActiveChain || curr->node()->inActiveChain()))
                nodesToRemoveFromChain.append(curr->node());
        }
    }

    // Now set the hover state for our new object up to the root.
    for (RenderObject* curr = newHoverObj; curr; curr = curr->hoverAncestor()) {
        if (curr->node() && !curr->isText() && (!mustBeInActiveChain || curr->node()->inActiveChain()))
            nodesToAddToChain.append(curr->node());
    }

    size_t removeCount = nodesToRemoveFromChain.size();
    for (size_t i = 0; i < removeCount; ++i) {
        nodesToRemoveFromChain[i]->setActive(false);
        nodesToRemoveFromChain[i]->setHovered(false);
    }

    size_t addCount = nodesToAddToChain.size();
    for (size_t i = 0; i < addCount; ++i) {
        nodesToAddToChain[i]->setActive(request.active());
        nodesToAddToChain[i]->setHovered(true);
    }
}

// Helper for the sorting of layers by z-index.
static inline bool compareZIndex(RenderLayer* first, RenderLayer* second)
{
    return first->zIndex() < second->zIndex();
}

void RenderLayer::dirtyZOrderLists()
{
    if (m_posZOrderList)
        m_posZOrderList->clear();
    if (m_negZOrderList)
        m_negZOrderList->clear();
    m_zOrderListsDirty = true;

#if USE(ACCELERATED_COMPOSITING)
    if (!renderer()->documentBeingDestroyed())
        compositor()->setCompositingLayersNeedRebuild();
#endif
}

void RenderLayer::dirtyStackingContextZOrderLists()
{
    RenderLayer* sc = stackingContext();
    if (sc)
        sc->dirtyZOrderLists();
}

void RenderLayer::dirtyNormalFlowList()
{
    if (m_normalFlowList)
        m_normalFlowList->clear();
    m_normalFlowListDirty = true;

#if USE(ACCELERATED_COMPOSITING)
    if (!renderer()->documentBeingDestroyed())
        compositor()->setCompositingLayersNeedRebuild();
#endif
}

void RenderLayer::updateZOrderLists()
{
    if (!isStackingContext() || !m_zOrderListsDirty)
        return;

    for (RenderLayer* child = firstChild(); child; child = child->nextSibling())
        if (!m_reflection || reflectionLayer() != child)
            child->collectLayers(m_posZOrderList, m_negZOrderList);

    // Sort the two lists.
    if (m_posZOrderList)
        std::stable_sort(m_posZOrderList->begin(), m_posZOrderList->end(), compareZIndex);

    if (m_negZOrderList)
        std::stable_sort(m_negZOrderList->begin(), m_negZOrderList->end(), compareZIndex);

    m_zOrderListsDirty = false;
}

void RenderLayer::updateNormalFlowList()
{
    if (!m_normalFlowListDirty)
        return;
        
    for (RenderLayer* child = firstChild(); child; child = child->nextSibling()) {
        // Ignore non-overflow layers and reflections.
        if (child->isNormalFlowOnly() && (!m_reflection || reflectionLayer() != child)) {
            if (!m_normalFlowList)
                m_normalFlowList = new Vector<RenderLayer*>;
            m_normalFlowList->append(child);
        }
    }
    
    m_normalFlowListDirty = false;
}

void RenderLayer::collectLayers(Vector<RenderLayer*>*& posBuffer, Vector<RenderLayer*>*& negBuffer)
{
    updateVisibilityStatus();

    // Overflow layers are just painted by their enclosing layers, so they don't get put in zorder lists.
    if ((m_hasVisibleContent || (m_hasVisibleDescendant && isStackingContext())) && !isNormalFlowOnly()) {
        // Determine which buffer the child should be in.
        Vector<RenderLayer*>*& buffer = (zIndex() >= 0) ? posBuffer : negBuffer;

        // Create the buffer if it doesn't exist yet.
        if (!buffer)
            buffer = new Vector<RenderLayer*>;
        
        // Append ourselves at the end of the appropriate buffer.
        buffer->append(this);
    }

    // Recur into our children to collect more layers, but only if we don't establish
    // a stacking context.
    if (m_hasVisibleDescendant && !isStackingContext()) {
        for (RenderLayer* child = firstChild(); child; child = child->nextSibling()) {
            // Ignore reflections.
            if (!m_reflection || reflectionLayer() != child)
                child->collectLayers(posBuffer, negBuffer);
        }
    }
}

void RenderLayer::updateLayerListsIfNeeded()
{
    updateZOrderLists();
    updateNormalFlowList();
}

void RenderLayer::updateCompositingAndLayerListsIfNeeded()
{
#if USE(ACCELERATED_COMPOSITING)
    if (compositor()->inCompositingMode()) {
        if ((isStackingContext() && m_zOrderListsDirty) || m_normalFlowListDirty)
            compositor()->updateCompositingLayers(CompositingUpdateOnPaitingOrHitTest, this);
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
    if (backing()->paintingGoesToWindow()) {
        // If we're trying to repaint the placeholder document layer, propagate the
        // repaint to the native view system.
        RenderView* view = renderer()->view();
        if (view)
            view->repaintViewRectangle(absoluteBoundingBox());
    } else
        backing()->setContentsNeedDisplay();
}

void RenderLayer::setBackingNeedsRepaintInRect(const IntRect& r)
{
    // https://bugs.webkit.org/show_bug.cgi?id=61159 describes an unreproducible crash here,
    // so assert but check that the layer is composited.
    ASSERT(isComposited());
    if (!isComposited() || backing()->paintingGoesToWindow()) {
        // If we're trying to repaint the placeholder document layer, propagate the
        // repaint to the native view system.
        IntRect absRect(r);
        int x = 0;
        int y = 0;
        convertToLayerCoords(root(), x, y);
        absRect.move(x, y);

        RenderView* view = renderer()->view();
        if (view)
            view->repaintViewRectangle(absRect);
    } else
        backing()->setContentsNeedDisplayInRect(r);
}

// Since we're only painting non-composited layers, we know that they all share the same repaintContainer.
void RenderLayer::repaintIncludingNonCompositingDescendants(RenderBoxModelObject* repaintContainer)
{
    renderer()->repaintUsingContainer(repaintContainer, renderer()->clippedOverflowRectForRepaint(repaintContainer));

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
                || renderer()->isVideo()
                || renderer()->isEmbeddedObject()
                || renderer()->isApplet()
                || renderer()->isRenderIFrame()
                || renderer()->style()->specifiesColumns())
            && !renderer()->isPositioned()
            && !renderer()->isRelPositioned()
            && !renderer()->hasTransform()
            && !isTransparent();
}

bool RenderLayer::isSelfPaintingLayer() const
{
    return !isNormalFlowOnly()
        || renderer()->hasReflection()
        || renderer()->hasMask()
        || renderer()->isTableRow()
        || renderer()->isVideo()
        || renderer()->isEmbeddedObject()
        || renderer()->isApplet()
        || renderer()->isRenderIFrame();
}

void RenderLayer::styleChanged(StyleDifference diff, const RenderStyle* oldStyle)
{
    bool isNormalFlowOnly = shouldBeNormalFlowOnly();
    if (isNormalFlowOnly != m_isNormalFlowOnly) {
        m_isNormalFlowOnly = isNormalFlowOnly;
        RenderLayer* p = parent();
        if (p)
            p->dirtyNormalFlowList();
        dirtyStackingContextZOrderLists();
    }

    if (renderer()->style()->overflowX() == OMARQUEE && renderer()->style()->marqueeBehavior() != MNONE && renderer()->isBox()) {
        if (!m_marquee)
            m_marquee = new RenderMarquee(this);
        m_marquee->updateMarqueeStyle();
    }
    else if (m_marquee) {
        delete m_marquee;
        m_marquee = 0;
    }
    
    if (!hasReflection() && m_reflection)
        removeReflection();
    else if (hasReflection()) {
        if (!m_reflection)
            createReflection();
        updateReflectionStyle();
    }
    
    // FIXME: Need to detect a swap from custom to native scrollbars (and vice versa).
    if (m_hBar)
        m_hBar->styleChanged();
    if (m_vBar)
        m_vBar->styleChanged();
    
    updateScrollCornerStyle();
    updateResizerStyle();

#if USE(ACCELERATED_COMPOSITING)
    updateTransform();

    if (compositor()->updateLayerCompositingState(this))
        compositor()->setCompositingLayersNeedRebuild();
    else if (m_backing)
        m_backing->updateGraphicsLayerGeometry();
    else if (oldStyle && oldStyle->overflowX() != renderer()->style()->overflowX()) {
        if (stackingContext()->hasCompositingDescendant())
            compositor()->setCompositingLayersNeedRebuild();
    }
    
    if (m_backing && diff >= StyleDifferenceRepaint)
        m_backing->setContentsNeedDisplay();
#else
    UNUSED_PARAM(diff);
#endif
}

void RenderLayer::updateScrollCornerStyle()
{
    RenderObject* actualRenderer = renderer()->node() ? renderer()->node()->shadowAncestorNode()->renderer() : renderer();
    RefPtr<RenderStyle> corner = renderer()->hasOverflowClip() ? actualRenderer->getUncachedPseudoStyle(SCROLLBAR_CORNER, actualRenderer->style()) : PassRefPtr<RenderStyle>(0);
    if (corner) {
        if (!m_scrollCorner) {
            m_scrollCorner = new (renderer()->renderArena()) RenderScrollbarPart(renderer()->document());
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
    RenderObject* actualRenderer = renderer()->node() ? renderer()->node()->shadowAncestorNode()->renderer() : renderer();
    RefPtr<RenderStyle> resizer = renderer()->hasOverflowClip() ? actualRenderer->getUncachedPseudoStyle(RESIZER, actualRenderer->style()) : PassRefPtr<RenderStyle>(0);
    if (resizer) {
        if (!m_resizer) {
            m_resizer = new (renderer()->renderArena()) RenderScrollbarPart(renderer()->document());
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
    m_reflection = new (renderer()->renderArena()) RenderReplica(renderer()->document());
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

void RenderLayer::updateContentsScale(float scale)
{
#if USE(ACCELERATED_COMPOSITING)
    if (m_backing)
        m_backing->updateContentsScale(scale);
#endif
}

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
