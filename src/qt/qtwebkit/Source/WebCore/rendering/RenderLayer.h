/*
 * Copyright (C) 2003, 2009, 2012 Apple Inc. All rights reserved.
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

#ifndef RenderLayer_h
#define RenderLayer_h

#include "PaintInfo.h"
#include "RenderBox.h"
#include "ScrollableArea.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

#if ENABLE(CSS_FILTERS)
class FilterEffectRenderer;
class FilterEffectRendererHelper;
class FilterOperations;
class RenderLayerFilterInfo;
#endif
class HitTestRequest;
class HitTestResult;
class HitTestingTransformState;
class RenderFlowThread;
class RenderGeometryMap;
class RenderMarquee;
class RenderReplica;
class RenderScrollbarPart;
class RenderStyle;
class RenderView;
class Scrollbar;
class TransformationMatrix;

#if USE(ACCELERATED_COMPOSITING)
class RenderLayerBacking;
class RenderLayerCompositor;
#endif

enum BorderRadiusClippingRule { IncludeSelfForBorderRadius, DoNotIncludeSelfForBorderRadius };
enum IncludeSelfOrNot { IncludeSelf, ExcludeSelf };

enum RepaintStatus {
    NeedsNormalRepaint,
    NeedsFullRepaint,
    NeedsFullRepaintForPositionedMovementLayout
};

class ClipRect {
public:
    ClipRect()
    : m_hasRadius(false)
    { }
    
    ClipRect(const LayoutRect& rect)
    : m_rect(rect)
    , m_hasRadius(false)
    { }
    
    const LayoutRect& rect() const { return m_rect; }
    void setRect(const LayoutRect& rect) { m_rect = rect; }

    bool hasRadius() const { return m_hasRadius; }
    void setHasRadius(bool hasRadius) { m_hasRadius = hasRadius; }

    bool operator==(const ClipRect& other) const { return rect() == other.rect() && hasRadius() == other.hasRadius(); }
    bool operator!=(const ClipRect& other) const { return rect() != other.rect() || hasRadius() != other.hasRadius(); }
    bool operator!=(const LayoutRect& otherRect) const { return rect() != otherRect; }

    void intersect(const LayoutRect& other) { m_rect.intersect(other); }
    void intersect(const ClipRect& other)
    {
        m_rect.intersect(other.rect());
        if (other.hasRadius())
            m_hasRadius = true;
    }
    void move(LayoutUnit x, LayoutUnit y) { m_rect.move(x, y); }
    void move(const LayoutSize& size) { m_rect.move(size); }
    void moveBy(const LayoutPoint& point) { m_rect.moveBy(point); }

    bool isEmpty() const { return m_rect.isEmpty(); }
    bool intersects(const LayoutRect& rect) const { return m_rect.intersects(rect); }
    bool intersects(const HitTestLocation&) const;

private:
    LayoutRect m_rect;
    bool m_hasRadius;
};

inline ClipRect intersection(const ClipRect& a, const ClipRect& b)
{
    ClipRect c = a;
    c.intersect(b);
    return c;
}

class ClipRects {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassRefPtr<ClipRects> create()
    {
        return adoptRef(new ClipRects);
    }

    static PassRefPtr<ClipRects> create(const ClipRects& other)
    {
        return adoptRef(new ClipRects(other));
    }

    ClipRects()
        : m_refCnt(1)
        , m_fixed(false)
    {
    }

    void reset(const LayoutRect& r)
    {
        m_overflowClipRect = r;
        m_fixedClipRect = r;
        m_posClipRect = r;
        m_fixed = false;
    }
    
    const ClipRect& overflowClipRect() const { return m_overflowClipRect; }
    void setOverflowClipRect(const ClipRect& r) { m_overflowClipRect = r; }

    const ClipRect& fixedClipRect() const { return m_fixedClipRect; }
    void setFixedClipRect(const ClipRect&r) { m_fixedClipRect = r; }

    const ClipRect& posClipRect() const { return m_posClipRect; }
    void setPosClipRect(const ClipRect& r) { m_posClipRect = r; }

    bool fixed() const { return m_fixed; }
    void setFixed(bool fixed) { m_fixed = fixed; }

    void ref() { m_refCnt++; }
    void deref()
    {
        if (!--m_refCnt)
            delete this;
    }

    bool operator==(const ClipRects& other) const
    {
        return m_overflowClipRect == other.overflowClipRect() &&
               m_fixedClipRect == other.fixedClipRect() &&
               m_posClipRect == other.posClipRect() &&
               m_fixed == other.fixed();
    }

    ClipRects& operator=(const ClipRects& other)
    {
        m_overflowClipRect = other.overflowClipRect();
        m_fixedClipRect = other.fixedClipRect();
        m_posClipRect = other.posClipRect();
        m_fixed = other.fixed();
        return *this;
    }

private:
    ClipRects(const LayoutRect& r)
        : m_overflowClipRect(r)
        , m_fixedClipRect(r)
        , m_posClipRect(r)
        , m_refCnt(1)
        , m_fixed(false)
    {
    }

    ClipRects(const ClipRects& other)
        : m_overflowClipRect(other.overflowClipRect())
        , m_fixedClipRect(other.fixedClipRect())
        , m_posClipRect(other.posClipRect())
        , m_refCnt(1)
        , m_fixed(other.fixed())
    {
    }

    ClipRect m_overflowClipRect;
    ClipRect m_fixedClipRect;
    ClipRect m_posClipRect;
    unsigned m_refCnt : 31;
    bool m_fixed : 1;
};

enum ClipRectsType {
    PaintingClipRects, // Relative to painting ancestor. Used for painting.
    RootRelativeClipRects, // Relative to the ancestor treated as the root (e.g. transformed layer). Used for hit testing.
    AbsoluteClipRects, // Relative to the RenderView's layer. Used for compositing overlap testing.
    NumCachedClipRectsTypes,
    AllClipRectTypes,
    TemporaryClipRects
};

enum ShouldRespectOverflowClip {
    IgnoreOverflowClip,
    RespectOverflowClip
};

struct ClipRectsCache {
    WTF_MAKE_FAST_ALLOCATED;
public:
    ClipRectsCache()
    {
#ifndef NDEBUG
        for (int i = 0; i < NumCachedClipRectsTypes; ++i) {
            m_clipRectsRoot[i] = 0;
            m_scrollbarRelevancy[i] = IgnoreOverlayScrollbarSize;
        }
#endif
    }

    PassRefPtr<ClipRects> getClipRects(ClipRectsType clipRectsType, ShouldRespectOverflowClip respectOverflow) { return m_clipRects[getIndex(clipRectsType, respectOverflow)]; }
    void setClipRects(ClipRectsType clipRectsType, ShouldRespectOverflowClip respectOverflow, PassRefPtr<ClipRects> clipRects) { m_clipRects[getIndex(clipRectsType, respectOverflow)] = clipRects; }

#ifndef NDEBUG
    const RenderLayer* m_clipRectsRoot[NumCachedClipRectsTypes];
    OverlayScrollbarSizeRelevancy m_scrollbarRelevancy[NumCachedClipRectsTypes];
#endif

private:
    int getIndex(ClipRectsType clipRectsType, ShouldRespectOverflowClip respectOverflow)
    {
        int index = static_cast<int>(clipRectsType);
        if (respectOverflow == RespectOverflowClip)
            index += static_cast<int>(NumCachedClipRectsTypes);
        return index;
    }

    RefPtr<ClipRects> m_clipRects[NumCachedClipRectsTypes * 2];
};

struct LayerFragment {
public:
    LayerFragment()
        : shouldPaintContent(false)
    { }

    void setRects(const LayoutRect& bounds, const ClipRect& background, const ClipRect& foreground, const ClipRect& outline)
    {
        layerBounds = bounds;
        backgroundRect = background;
        foregroundRect = foreground;
        outlineRect = outline;
    }
    
    void moveBy(const LayoutPoint& offset)
    {
        layerBounds.moveBy(offset);
        backgroundRect.moveBy(offset);
        foregroundRect.moveBy(offset);
        outlineRect.moveBy(offset);
        paginationClip.moveBy(offset);
    }
    
    void intersect(const LayoutRect& rect)
    {
        backgroundRect.intersect(rect);
        foregroundRect.intersect(rect);
        outlineRect.intersect(rect);
    }
    
    bool shouldPaintContent;
    LayoutRect layerBounds;
    ClipRect backgroundRect;
    ClipRect foregroundRect;
    ClipRect outlineRect;
    
    // Unique to paginated fragments. The physical translation to apply to shift the layer when painting/hit-testing.
    LayoutPoint paginationOffset;
    
    // Also unique to paginated fragments. An additional clip that applies to the layer. It is in layer-local
    // (physical) coordinates.
    LayoutRect paginationClip;
};

typedef Vector<LayerFragment, 1> LayerFragments;

class RenderLayer : public ScrollableArea {
public:
    friend class RenderReplica;

    RenderLayer(RenderLayerModelObject*);
    ~RenderLayer();

    String name() const;

    RenderLayerModelObject* renderer() const { return m_renderer; }
    RenderBox* renderBox() const { return m_renderer && m_renderer->isBox() ? toRenderBox(m_renderer) : 0; }
    RenderLayer* parent() const { return m_parent; }
    RenderLayer* previousSibling() const { return m_previous; }
    RenderLayer* nextSibling() const { return m_next; }
    RenderLayer* firstChild() const { return m_first; }
    RenderLayer* lastChild() const { return m_last; }

    void addChild(RenderLayer* newChild, RenderLayer* beforeChild = 0);
    RenderLayer* removeChild(RenderLayer*);

    void removeOnlyThisLayer();
    void insertOnlyThisLayer();

    void repaintIncludingDescendants();

#if USE(ACCELERATED_COMPOSITING)
    // Indicate that the layer contents need to be repainted. Only has an effect
    // if layer compositing is being used,
    void setBackingNeedsRepaint();
    void setBackingNeedsRepaintInRect(const LayoutRect&); // r is in the coordinate space of the layer's render object
    void repaintIncludingNonCompositingDescendants(RenderLayerModelObject* repaintContainer);
#endif

    void styleChanged(StyleDifference, const RenderStyle* oldStyle);

    RenderMarquee* marquee() const { return m_marquee.get(); }

    bool isNormalFlowOnly() const { return m_isNormalFlowOnly; }
    bool isSelfPaintingLayer() const { return m_isSelfPaintingLayer; }

    bool cannotBlitToWindow() const;

    bool isTransparent() const;
    RenderLayer* transparentPaintingAncestor();
    void beginTransparencyLayers(GraphicsContext*, const RenderLayer* rootLayer, const LayoutRect& paintDirtyRect, PaintBehavior);

    bool hasReflection() const { return renderer()->hasReflection(); }
    bool isReflection() const { return renderer()->isReplica(); }
    RenderReplica* reflection() const { return m_reflection; }
    RenderLayer* reflectionLayer() const;

    const RenderLayer* root() const
    {
        const RenderLayer* curr = this;
        while (curr->parent())
            curr = curr->parent();
        return curr;
    }
    
    const LayoutPoint& location() const { return m_topLeft; }
    void setLocation(const LayoutPoint& p) { m_topLeft = p; }

    const IntSize& size() const { return m_layerSize; }
    void setSize(const IntSize& size) { m_layerSize = size; }

    LayoutRect rect() const { return LayoutRect(location(), size()); }

    int scrollWidth() const;
    int scrollHeight() const;

    void panScrollFromPoint(const IntPoint&);

    enum ScrollOffsetClamping {
        ScrollOffsetUnclamped,
        ScrollOffsetClamped
    };

    // Scrolling methods for layers that can scroll their overflow.
    void scrollByRecursively(const IntSize&, ScrollOffsetClamping = ScrollOffsetUnclamped);
    void scrollToOffset(const IntSize&, ScrollOffsetClamping = ScrollOffsetUnclamped);
    void scrollToXOffset(int x, ScrollOffsetClamping clamp = ScrollOffsetUnclamped) { scrollToOffset(IntSize(x, scrollYOffset()), clamp); }
    void scrollToYOffset(int y, ScrollOffsetClamping clamp = ScrollOffsetUnclamped) { scrollToOffset(IntSize(scrollXOffset(), y), clamp); }

    int scrollXOffset() const { return m_scrollOffset.width() + scrollOrigin().x(); }
    int scrollYOffset() const { return m_scrollOffset.height() + scrollOrigin().y(); }
    IntSize scrollOffset() const { return IntSize(scrollXOffset(), scrollYOffset()); }

    void scrollRectToVisible(const LayoutRect&, const ScrollAlignment& alignX, const ScrollAlignment& alignY);

    LayoutRect getRectToExpose(const LayoutRect& visibleRect, const LayoutRect& visibleRectRelativeToDocument, const LayoutRect& exposeRect, const ScrollAlignment& alignX, const ScrollAlignment& alignY);

    bool scrollsOverflow() const;
    bool hasScrollbars() const { return m_hBar || m_vBar; }
    void setHasHorizontalScrollbar(bool);
    void setHasVerticalScrollbar(bool);

    PassRefPtr<Scrollbar> createScrollbar(ScrollbarOrientation);
    void destroyScrollbar(ScrollbarOrientation);

    bool hasHorizontalScrollbar() const { return horizontalScrollbar(); }
    bool hasVerticalScrollbar() const { return verticalScrollbar(); }

    // ScrollableArea overrides
    virtual Scrollbar* horizontalScrollbar() const { return m_hBar.get(); }
    virtual Scrollbar* verticalScrollbar() const { return m_vBar.get(); }
    virtual ScrollableArea* enclosingScrollableArea() const;

    int verticalScrollbarWidth(OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize) const;
    int horizontalScrollbarHeight(OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize) const;

    bool hasOverflowControls() const;
    bool isPointInResizeControl(const IntPoint& absolutePoint) const;
    bool hitTestOverflowControls(HitTestResult&, const IntPoint& localPoint);
    IntSize offsetFromResizeCorner(const IntPoint& absolutePoint) const;

    void paintOverflowControls(GraphicsContext*, const IntPoint&, const IntRect& damageRect, bool paintingOverlayControls = false);
    void paintScrollCorner(GraphicsContext*, const IntPoint&, const IntRect& damageRect);
    void paintResizer(GraphicsContext*, const IntPoint&, const IntRect& damageRect);

    void updateScrollInfoAfterLayout();

    bool scroll(ScrollDirection, ScrollGranularity, float multiplier = 1);
    void autoscroll(const IntPoint&);

    bool canResize() const;
    void resize(const PlatformMouseEvent&, const LayoutSize&);
    bool inResizeMode() const { return m_inResizeMode; }
    void setInResizeMode(bool b) { m_inResizeMode = b; }

    bool isRootLayer() const { return m_isRootLayer; }

#if USE(ACCELERATED_COMPOSITING)
    RenderLayerCompositor* compositor() const;
    
    // Notification from the renderer that its content changed (e.g. current frame of image changed).
    // Allows updates of layer content without repainting.
    void contentChanged(ContentChangeType);
#endif

    bool canRender3DTransforms() const;

    enum UpdateLayerPositionsFlag {
        CheckForRepaint = 1 << 0,
        NeedsFullRepaintInBacking = 1 << 1,
        IsCompositingUpdateRoot = 1 << 2,
        UpdateCompositingLayers = 1 << 3,
        UpdatePagination = 1 << 4
    };
    typedef unsigned UpdateLayerPositionsFlags;
    static const UpdateLayerPositionsFlags defaultFlags = CheckForRepaint | IsCompositingUpdateRoot | UpdateCompositingLayers;

    void updateLayerPositionsAfterLayout(const RenderLayer* rootLayer, UpdateLayerPositionsFlags);

    void updateLayerPositionsAfterOverflowScroll();
    void updateLayerPositionsAfterDocumentScroll();

#if USE(ACCELERATED_COMPOSITING)
    void positionNewlyCreatedOverflowControls();
#endif
    
    bool isPaginated() const { return m_isPaginated; }
    RenderLayer* enclosingPaginationLayer() const { return m_enclosingPaginationLayer; }

    void updateTransform();
    
#if ENABLE(CSS_COMPOSITING)
    void updateBlendMode();
#endif

    const LayoutSize& offsetForInFlowPosition() const { return m_offsetForInFlowPosition; }

    void clearClipRectsIncludingDescendants(ClipRectsType typeToClear = AllClipRectTypes);
    void clearClipRects(ClipRectsType typeToClear = AllClipRectTypes);

    void addBlockSelectionGapsBounds(const LayoutRect&);
    void clearBlockSelectionGapsBounds();
    void repaintBlockSelectionGaps();

    // A stacking context is a layer that has a non-auto z-index.
    bool isStackingContext() const { return isStackingContext(renderer()->style()); }

    // A stacking container can have z-order lists. All stacking contexts are
    // stacking containers, but the converse is not true. Layers that use
    // composited scrolling are stacking containers, but they may not
    // necessarily be stacking contexts.
    bool isStackingContainer() const { return isStackingContext() || needsCompositedScrolling(); }

    // Gets the enclosing stacking container for this layer, excluding this
    // layer itself.
    RenderLayer* stackingContainer() const;

    // Gets the enclosing stacking container for this layer, possibly the layer
    // itself, if it is a stacking container.
    RenderLayer* enclosingStackingContainer() { return isStackingContainer() ? this : stackingContainer(); }

    void dirtyZOrderLists();
    void dirtyStackingContainerZOrderLists();

    Vector<RenderLayer*>* posZOrderList() const
    {
        ASSERT(!m_zOrderListsDirty);
        ASSERT(isStackingContainer() || !m_posZOrderList);
        return m_posZOrderList.get();
    }

    bool hasNegativeZOrderList() const { return negZOrderList() && negZOrderList()->size(); }

    Vector<RenderLayer*>* negZOrderList() const
    {
        ASSERT(!m_zOrderListsDirty);
        ASSERT(isStackingContainer() || !m_negZOrderList);
        return m_negZOrderList.get();
    }

    void dirtyNormalFlowList();
    Vector<RenderLayer*>* normalFlowList() const { ASSERT(!m_normalFlowListDirty); return m_normalFlowList.get(); }

    // Update our normal and z-index lists.
    void updateLayerListsIfNeeded();

    // FIXME: We should ASSERT(!m_visibleContentStatusDirty) here, but see https://bugs.webkit.org/show_bug.cgi?id=71044
    // ditto for hasVisibleDescendant(), see https://bugs.webkit.org/show_bug.cgi?id=71277
    bool hasVisibleContent() const { return m_hasVisibleContent; }
    bool hasVisibleDescendant() const { return m_hasVisibleDescendant; }

    void setHasVisibleContent();
    void dirtyVisibleContentStatus();

    bool hasBoxDecorationsOrBackground() const;
    bool hasVisibleBoxDecorations() const;
    // Returns true if this layer has visible content (ignoring any child layers).
    bool isVisuallyNonEmpty() const;
    // True if this layer container renderers that paint.
    bool hasNonEmptyChildRenderers() const;

    // FIXME: We should ASSERT(!m_hasSelfPaintingLayerDescendantDirty); here but we hit the same bugs as visible content above.
    // Part of the issue is with subtree relayout: we don't check if our ancestors have some descendant flags dirty, missing some updates.
    bool hasSelfPaintingLayerDescendant() const { return m_hasSelfPaintingLayerDescendant; }

    // This returns true if we have an out of flow positioned descendant whose
    // containing block is not a descendant of ours. If this is true, we cannot
    // automatically opt into composited scrolling since this out of flow
    // positioned descendant would become clipped by us, possibly altering the 
    // rendering of the page.
    // FIXME: We should ASSERT(!m_hasOutOfFlowPositionedDescendantDirty); here but we may hit the same bugs as visible content above.
    bool hasOutOfFlowPositionedDescendant() const { return m_hasOutOfFlowPositionedDescendant; }

    // Gets the nearest enclosing positioned ancestor layer (also includes
    // the <html> layer and the root layer).
    RenderLayer* enclosingPositionedAncestor() const;

    // Returns the nearest enclosing layer that is scrollable.
    RenderLayer* enclosingScrollableLayer() const;

    // The layer relative to which clipping rects for this layer are computed.
    RenderLayer* clippingRootForPainting() const;

    RenderLayer* enclosingOverflowClipLayer(IncludeSelfOrNot) const;

#if USE(ACCELERATED_COMPOSITING)
    // Enclosing compositing layer; if includeSelf is true, may return this.
    RenderLayer* enclosingCompositingLayer(IncludeSelfOrNot = IncludeSelf) const;
    RenderLayer* enclosingCompositingLayerForRepaint(IncludeSelfOrNot = IncludeSelf) const;
    // Ancestor compositing layer, excluding this.
    RenderLayer* ancestorCompositingLayer() const { return enclosingCompositingLayer(ExcludeSelf); }
#endif

#if ENABLE(CSS_FILTERS)
    RenderLayer* enclosingFilterLayer(IncludeSelfOrNot = IncludeSelf) const;
    RenderLayer* enclosingFilterRepaintLayer() const;
    void setFilterBackendNeedsRepaintingInRect(const LayoutRect&, bool immediate);
    bool hasAncestorWithFilterOutsets() const;
#endif

    bool canUseConvertToLayerCoords() const
    {
        // These RenderObject have an impact on their layers' without them knowing about it.
        return !renderer()->hasColumns() && !renderer()->hasTransform()
#if ENABLE(SVG)
            && !renderer()->isSVGRoot()
#endif
            ;
    }

    // FIXME: adjustForColumns allows us to position compositing layers in columns correctly, but eventually they need to be split across columns too.
    enum ColumnOffsetAdjustment { DontAdjustForColumns, AdjustForColumns };
    void convertToPixelSnappedLayerCoords(const RenderLayer* ancestorLayer, IntPoint& location, ColumnOffsetAdjustment adjustForColumns = DontAdjustForColumns) const;
    void convertToPixelSnappedLayerCoords(const RenderLayer* ancestorLayer, IntRect&, ColumnOffsetAdjustment adjustForColumns = DontAdjustForColumns) const;
    void convertToLayerCoords(const RenderLayer* ancestorLayer, LayoutPoint&, ColumnOffsetAdjustment adjustForColumns = DontAdjustForColumns) const;
    void convertToLayerCoords(const RenderLayer* ancestorLayer, LayoutRect&, ColumnOffsetAdjustment adjustForColumns = DontAdjustForColumns) const;

    int zIndex() const { return renderer()->style()->zIndex(); }

    enum PaintLayerFlag {
        PaintLayerHaveTransparency = 1,
        PaintLayerAppliedTransform = 1 << 1,
        PaintLayerTemporaryClipRects = 1 << 2,
        PaintLayerPaintingReflection = 1 << 3,
        PaintLayerPaintingOverlayScrollbars = 1 << 4,
        PaintLayerPaintingCompositingBackgroundPhase = 1 << 5,
        PaintLayerPaintingCompositingForegroundPhase = 1 << 6,
        PaintLayerPaintingCompositingMaskPhase = 1 << 7,
        PaintLayerPaintingCompositingScrollingPhase = 1 << 8,
        PaintLayerPaintingOverflowContents = 1 << 9,
        PaintLayerPaintingRootBackgroundOnly = 1 << 10,
        PaintLayerPaintingSkipRootBackground = 1 << 11,
        PaintLayerPaintingCompositingAllPhases = (PaintLayerPaintingCompositingBackgroundPhase | PaintLayerPaintingCompositingForegroundPhase | PaintLayerPaintingCompositingMaskPhase)
    };
    
    typedef unsigned PaintLayerFlags;

    // The two main functions that use the layer system.  The paint method
    // paints the layers that intersect the damage rect from back to
    // front.  The hitTest method looks for mouse events by walking
    // layers that intersect the point from front to back.
    void paint(GraphicsContext*, const LayoutRect& damageRect, PaintBehavior = PaintBehaviorNormal, RenderObject* subtreePaintRoot = 0,
        RenderRegion* = 0, PaintLayerFlags = 0);
    bool hitTest(const HitTestRequest&, HitTestResult&);
    bool hitTest(const HitTestRequest&, const HitTestLocation&, HitTestResult&);
    void paintOverlayScrollbars(GraphicsContext*, const LayoutRect& damageRect, PaintBehavior, RenderObject* subtreePaintRoot = 0);

    struct ClipRectsContext {
        ClipRectsContext(const RenderLayer* inRootLayer, RenderRegion* inRegion, ClipRectsType inClipRectsType, OverlayScrollbarSizeRelevancy inOverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize, ShouldRespectOverflowClip inRespectOverflowClip = RespectOverflowClip)
            : rootLayer(inRootLayer)
            , region(inRegion)
            , clipRectsType(inClipRectsType)
            , overlayScrollbarSizeRelevancy(inOverlayScrollbarSizeRelevancy)
            , respectOverflowClip(inRespectOverflowClip)
        { }
        const RenderLayer* rootLayer;
        RenderRegion* region;
        ClipRectsType clipRectsType;
        OverlayScrollbarSizeRelevancy overlayScrollbarSizeRelevancy;
        ShouldRespectOverflowClip respectOverflowClip;
    };

    // This method figures out our layerBounds in coordinates relative to
    // |rootLayer}.  It also computes our background and foreground clip rects
    // for painting/event handling.
    // Pass offsetFromRoot if known.
    void calculateRects(const ClipRectsContext&, const LayoutRect& paintDirtyRect, LayoutRect& layerBounds,
        ClipRect& backgroundRect, ClipRect& foregroundRect, ClipRect& outlineRect, const LayoutPoint* offsetFromRoot = 0) const;

    // Compute and cache clip rects computed with the given layer as the root
    void updateClipRects(const ClipRectsContext&);
    // Compute and return the clip rects. If useCached is true, will used previously computed clip rects on ancestors
    // (rather than computing them all from scratch up the parent chain).
    void calculateClipRects(const ClipRectsContext&, ClipRects&) const;

    ClipRects* clipRects(const ClipRectsContext& context) const
    {
        ASSERT(context.clipRectsType < NumCachedClipRectsTypes);
        return m_clipRectsCache ? m_clipRectsCache->getClipRects(context.clipRectsType, context.respectOverflowClip).get() : 0;
    }

    LayoutRect childrenClipRect() const; // Returns the foreground clip rect of the layer in the document's coordinate space.
    LayoutRect selfClipRect() const; // Returns the background clip rect of the layer in the document's coordinate space.
    LayoutRect localClipRect() const; // Returns the background clip rect of the layer in the local coordinate space.

    // Pass offsetFromRoot if known.
    bool intersectsDamageRect(const LayoutRect& layerBounds, const LayoutRect& damageRect, const RenderLayer* rootLayer, const LayoutPoint* offsetFromRoot = 0) const;

    enum CalculateLayerBoundsFlag {
        IncludeSelfTransform = 1 << 0,
        UseLocalClipRectIfPossible = 1 << 1,
        IncludeLayerFilterOutsets = 1 << 2,
        ExcludeHiddenDescendants = 1 << 3,
        DontConstrainForMask = 1 << 4,
        IncludeCompositedDescendants = 1 << 5,
        UseFragmentBoxes = 1 << 6,
        DefaultCalculateLayerBoundsFlags =  IncludeSelfTransform | UseLocalClipRectIfPossible | IncludeLayerFilterOutsets | UseFragmentBoxes
    };
    typedef unsigned CalculateLayerBoundsFlags;

    // Bounding box relative to some ancestor layer. Pass offsetFromRoot if known.
    LayoutRect boundingBox(const RenderLayer* rootLayer, CalculateLayerBoundsFlags = 0, const LayoutPoint* offsetFromRoot = 0) const;
    // Bounding box in the coordinates of this layer.
    LayoutRect localBoundingBox(CalculateLayerBoundsFlags = 0) const;
    // Pixel snapped bounding box relative to the root.
    IntRect absoluteBoundingBox() const;

    // Bounds used for layer overlap testing in RenderLayerCompositor.
    LayoutRect overlapBounds() const { return overlapBoundsIncludeChildren() ? calculateLayerBounds(this) : localBoundingBox(); }

#if ENABLE(CSS_FILTERS)
    // If true, this layer's children are included in its bounds for overlap testing.
    // We can't rely on the children's positions if this layer has a filter that could have moved the children's pixels around.
    bool overlapBoundsIncludeChildren() const { return hasFilter() && renderer()->style()->filter().hasFilterThatMovesPixels(); }
#else
    bool overlapBoundsIncludeChildren() const { return false; }
#endif

    // Can pass offsetFromRoot if known.
    LayoutRect calculateLayerBounds(const RenderLayer* ancestorLayer, const LayoutPoint* offsetFromRoot = 0, CalculateLayerBoundsFlags = DefaultCalculateLayerBoundsFlags) const;
    
    // WARNING: This method returns the offset for the parent as this is what updateLayerPositions expects.
    LayoutPoint computeOffsetFromRoot(bool& hasLayerOffset) const;

    // Return a cached repaint rect, computed relative to the layer renderer's containerForRepaint.
    LayoutRect repaintRect() const { return m_repaintRect; }
    LayoutRect repaintRectIncludingNonCompositingDescendants() const;

    void setRepaintStatus(RepaintStatus status) { m_repaintStatus = status; }

    LayoutUnit staticInlinePosition() const { return m_staticInlinePosition; }
    LayoutUnit staticBlockPosition() const { return m_staticBlockPosition; }
   
    void setStaticInlinePosition(LayoutUnit position) { m_staticInlinePosition = position; }
    void setStaticBlockPosition(LayoutUnit position) { m_staticBlockPosition = position; }

    bool hasTransform() const { return renderer()->hasTransform(); }
    // Note that this transform has the transform-origin baked in.
    TransformationMatrix* transform() const { return m_transform.get(); }
    // currentTransform computes a transform which takes accelerated animations into account. The
    // resulting transform has transform-origin baked in. If the layer does not have a transform,
    // returns the identity matrix.
    TransformationMatrix currentTransform(RenderStyle::ApplyTransformOrigin = RenderStyle::IncludeTransformOrigin) const;
    TransformationMatrix renderableTransform(PaintBehavior) const;
    
    // Get the perspective transform, which is applied to transformed sublayers.
    // Returns true if the layer has a -webkit-perspective.
    // Note that this transform has the perspective-origin baked in.
    TransformationMatrix perspectiveTransform() const;
    FloatPoint perspectiveOrigin() const;
    bool preserves3D() const { return renderer()->style()->transformStyle3D() == TransformStyle3DPreserve3D; }
    bool has3DTransform() const { return m_transform && !m_transform->isAffine(); }

#if ENABLE(CSS_FILTERS)
    virtual void filterNeedsRepaint();
    bool hasFilter() const { return renderer()->hasFilter(); }
#else
    bool hasFilter() const { return false; }
#endif

#if ENABLE(CSS_COMPOSITING)
    bool hasBlendMode() const { return renderer()->hasBlendMode(); }
#else
    bool hasBlendMode() const { return false; }
#endif

    // Overloaded new operator. Derived classes must override operator new
    // in order to allocate out of the RenderArena.
    void* operator new(size_t, RenderArena*);

    // Overridden to prevent the normal delete from being called.
    void operator delete(void*, size_t);

#if USE(ACCELERATED_COMPOSITING)
    bool isComposited() const { return m_backing != 0; }
    bool hasCompositedMask() const;
    RenderLayerBacking* backing() const { return m_backing.get(); }
    RenderLayerBacking* ensureBacking();
    void clearBacking(bool layerBeingDestroyed = false);
    virtual GraphicsLayer* layerForScrolling() const;
    virtual GraphicsLayer* layerForHorizontalScrollbar() const;
    virtual GraphicsLayer* layerForVerticalScrollbar() const;
    virtual GraphicsLayer* layerForScrollCorner() const;
    virtual bool usesCompositedScrolling() const OVERRIDE;
    bool needsCompositedScrolling() const;
    bool needsCompositingLayersRebuiltForClip(const RenderStyle* oldStyle, const RenderStyle* newStyle) const;
    bool needsCompositingLayersRebuiltForOverflow(const RenderStyle* oldStyle, const RenderStyle* newStyle) const;
#else
    bool isComposited() const { return false; }
    bool hasCompositedMask() const { return false; }
    bool usesCompositedScrolling() const { return false; }
    bool needsCompositedScrolling() const { return false; }
#endif

    bool paintsWithTransparency(PaintBehavior paintBehavior) const
    {
        return isTransparent() && ((paintBehavior & PaintBehaviorFlattenCompositingLayers) || !isComposited());
    }

    bool paintsWithTransform(PaintBehavior) const;

    // Returns true if background phase is painted opaque in the given rect.
    // The query rect is given in local coordinates.
    bool backgroundIsKnownToBeOpaqueInRect(const LayoutRect&) const;

    bool containsDirtyOverlayScrollbars() const { return m_containsDirtyOverlayScrollbars; }
    void setContainsDirtyOverlayScrollbars(bool dirtyScrollbars) { m_containsDirtyOverlayScrollbars = dirtyScrollbars; }

#if ENABLE(CSS_SHADERS)
    bool isCSSCustomFilterEnabled() const;
#endif

#if ENABLE(CSS_FILTERS)
    FilterOperations computeFilterOperations(const RenderStyle*);
    bool paintsWithFilters() const;
    bool requiresFullLayerImageForFilters() const;
    FilterEffectRenderer* filterRenderer() const;

    RenderLayerFilterInfo* filterInfo() const;
    RenderLayerFilterInfo* ensureFilterInfo();
    void removeFilterInfoIfNeeded();
    
    bool hasFilterInfo() const { return m_hasFilterInfo; }
    void setHasFilterInfo(bool hasFilterInfo) { m_hasFilterInfo = hasFilterInfo; }
#endif

#if !ASSERT_DISABLED
    bool layerListMutationAllowed() const { return m_layerListMutationAllowed; }
    void setLayerListMutationAllowed(bool flag) { m_layerListMutationAllowed = flag; }
#endif

    Node* enclosingElement() const;

#if ENABLE(DIALOG_ELEMENT)
    bool isInTopLayer() const;
    bool isInTopLayerSubtree() const;
#endif

#if USE(ACCELERATED_COMPOSITING)
    enum ViewportConstrainedNotCompositedReason {
        NoNotCompositedReason,
        NotCompositedForBoundsOutOfView,
        NotCompositedForNonViewContainer,
        NotCompositedForNoVisibleContent,
    };

    void setViewportConstrainedNotCompositedReason(ViewportConstrainedNotCompositedReason reason) { m_viewportConstrainedNotCompositedReason = reason; }
    ViewportConstrainedNotCompositedReason viewportConstrainedNotCompositedReason() const { return static_cast<ViewportConstrainedNotCompositedReason>(m_viewportConstrainedNotCompositedReason); }
#endif
    
    bool isOutOfFlowRenderFlowThread() const { return renderer()->isOutOfFlowRenderFlowThread(); }

private:
    enum CollectLayersBehavior { StopAtStackingContexts, StopAtStackingContainers };

    void updateZOrderLists();
    void rebuildZOrderLists();
    void rebuildZOrderLists(CollectLayersBehavior, OwnPtr<Vector<RenderLayer*> >&, OwnPtr<Vector<RenderLayer*> >&);
    void clearZOrderLists();

    void updateNormalFlowList();

    // Non-auto z-index always implies stacking context here, because StyleResolver::adjustRenderStyle already adjusts z-index
    // based on positioning and other criteria.
    bool isStackingContext(const RenderStyle* style) const { return !style->hasAutoZIndex() || isRootLayer(); }

    bool isDirtyStackingContainer() const { return m_zOrderListsDirty && isStackingContainer(); }

    void setAncestorChainHasSelfPaintingLayerDescendant();
    void dirtyAncestorChainHasSelfPaintingLayerDescendantStatus();

    bool acceleratedCompositingForOverflowScrollEnabled() const;
    void updateDescendantsAreContiguousInStackingOrder();
    void updateDescendantsAreContiguousInStackingOrderRecursive(const HashMap<const RenderLayer*, int>&, int& minIndex, int& maxIndex, int& count, bool firstIteration);

    void computeRepaintRects(const RenderLayerModelObject* repaintContainer, const RenderGeometryMap* = 0);
    void computeRepaintRectsIncludingDescendants();
    void clearRepaintRects();

    void clipToRect(RenderLayer* rootLayer, GraphicsContext*, const LayoutRect& paintDirtyRect, const ClipRect&,
                    BorderRadiusClippingRule = IncludeSelfForBorderRadius);
    void restoreClip(GraphicsContext*, const LayoutRect& paintDirtyRect, const ClipRect&);

    bool shouldRepaintAfterLayout() const;

    void updateSelfPaintingLayer();
    void updateStackingContextsAfterStyleChange(const RenderStyle* oldStyle);

    void updateScrollbarsAfterStyleChange(const RenderStyle* oldStyle);
    void updateScrollbarsAfterLayout();

    void setAncestorChainHasOutOfFlowPositionedDescendant(RenderObject* containingBlock);
    void dirtyAncestorChainHasOutOfFlowPositionedDescendantStatus();
    void updateOutOfFlowPositioned(const RenderStyle* oldStyle);

    void updateNeedsCompositedScrolling();

    // Returns true if the position changed.
    bool updateLayerPosition();

    void updateLayerPositions(RenderGeometryMap* = 0, UpdateLayerPositionsFlags = defaultFlags);

    enum UpdateLayerPositionsAfterScrollFlag {
        NoFlag = 0,
        IsOverflowScroll = 1 << 0,
        HasSeenViewportConstrainedAncestor = 1 << 1,
        HasSeenAncestorWithOverflowClip = 1 << 2,
        HasChangedAncestor = 1 << 3
    };
    typedef unsigned UpdateLayerPositionsAfterScrollFlags;
    void updateLayerPositionsAfterScroll(RenderGeometryMap*, UpdateLayerPositionsAfterScrollFlags = NoFlag);

    friend IntSize RenderBox::scrolledContentOffset() const;
    IntSize scrolledContentOffset() const { return m_scrollOffset; }

    IntSize clampScrollOffset(const IntSize&) const;

    // The normal operator new is disallowed on all render objects.
    void* operator new(size_t) throw();

    void setNextSibling(RenderLayer* next) { m_next = next; }
    void setPreviousSibling(RenderLayer* prev) { m_previous = prev; }
    void setParent(RenderLayer* parent);
    void setFirstChild(RenderLayer* first) { m_first = first; }
    void setLastChild(RenderLayer* last) { m_last = last; }

    LayoutPoint renderBoxLocation() const { return renderer()->isBox() ? toRenderBox(renderer())->location() : LayoutPoint(); }

    void collectLayers(bool includeHiddenLayers, CollectLayersBehavior, OwnPtr<Vector<RenderLayer*> >&, OwnPtr<Vector<RenderLayer*> >&);

    void updateCompositingAndLayerListsIfNeeded();

    struct LayerPaintingInfo {
        LayerPaintingInfo(RenderLayer* inRootLayer, const LayoutRect& inDirtyRect, PaintBehavior inPaintBehavior, const LayoutSize& inSubPixelAccumulation, RenderObject* inSubtreePaintRoot = 0, RenderRegion*inRegion = 0, OverlapTestRequestMap* inOverlapTestRequests = 0)
            : rootLayer(inRootLayer)
            , subtreePaintRoot(inSubtreePaintRoot)
            , paintDirtyRect(inDirtyRect)
            , subPixelAccumulation(inSubPixelAccumulation)
            , region(inRegion)
            , overlapTestRequests(inOverlapTestRequests)
            , paintBehavior(inPaintBehavior)
            , clipToDirtyRect(true)
        { }
        RenderLayer* rootLayer;
        RenderObject* subtreePaintRoot; // only paint descendants of this object
        LayoutRect paintDirtyRect; // relative to rootLayer;
        LayoutSize subPixelAccumulation;
        RenderRegion* region; // May be null.
        OverlapTestRequestMap* overlapTestRequests; // May be null.
        PaintBehavior paintBehavior;
        bool clipToDirtyRect;
    };

    bool setupFontSubpixelQuantization(GraphicsContext*, bool& didQuantizeFonts);
    bool setupClipPath(GraphicsContext*, const LayerPaintingInfo&, const LayoutPoint& offsetFromRoot, LayoutRect& rootRelativeBounds, bool& rootRelativeBoundsComputed);
#if ENABLE(CSS_FILTERS)
    PassOwnPtr<FilterEffectRendererHelper> setupFilters(GraphicsContext*, LayerPaintingInfo&, PaintLayerFlags, const LayoutPoint& offsetFromRoot, LayoutRect& rootRelativeBounds, bool& rootRelativeBoundsComputed);
    GraphicsContext* applyFilters(FilterEffectRendererHelper*, GraphicsContext* originalContext, LayerPaintingInfo&, LayerFragments&);
#endif

    void paintLayer(GraphicsContext*, const LayerPaintingInfo&, PaintLayerFlags);
    void paintLayerContentsAndReflection(GraphicsContext*, const LayerPaintingInfo&, PaintLayerFlags);
    void paintLayerByApplyingTransform(GraphicsContext*, const LayerPaintingInfo&, PaintLayerFlags, const LayoutPoint& translationOffset = LayoutPoint());
    void paintLayerContents(GraphicsContext*, const LayerPaintingInfo&, PaintLayerFlags);
    void paintList(Vector<RenderLayer*>*, GraphicsContext*, const LayerPaintingInfo&, PaintLayerFlags);
    void paintPaginatedChildLayer(RenderLayer* childLayer, GraphicsContext*, const LayerPaintingInfo&, PaintLayerFlags);
    void paintChildLayerIntoColumns(RenderLayer* childLayer, GraphicsContext*, const LayerPaintingInfo&, PaintLayerFlags, const Vector<RenderLayer*>& columnLayers, size_t columnIndex);

    void collectFragments(LayerFragments&, const RenderLayer* rootLayer, RenderRegion*, const LayoutRect& dirtyRect,
        ClipRectsType, OverlayScrollbarSizeRelevancy inOverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize,
        ShouldRespectOverflowClip = RespectOverflowClip, const LayoutPoint* offsetFromRoot = 0, const LayoutRect* layerBoundingBox = 0);
    void updatePaintingInfoForFragments(LayerFragments&, const LayerPaintingInfo&, PaintLayerFlags, bool shouldPaintContent, const LayoutPoint* offsetFromRoot);
    void paintBackgroundForFragments(const LayerFragments&, GraphicsContext*, GraphicsContext* transparencyLayerContext,
        const LayoutRect& transparencyPaintDirtyRect, bool haveTransparency, const LayerPaintingInfo&, PaintBehavior, RenderObject* paintingRootForRenderer);
    void paintForegroundForFragments(const LayerFragments&, GraphicsContext*, GraphicsContext* transparencyLayerContext,
        const LayoutRect& transparencyPaintDirtyRect, bool haveTransparency, const LayerPaintingInfo&, PaintBehavior, RenderObject* paintingRootForRenderer,
        bool selectionOnly, bool forceBlackText);
    void paintForegroundForFragmentsWithPhase(PaintPhase, const LayerFragments&, GraphicsContext*, const LayerPaintingInfo&, PaintBehavior, RenderObject* paintingRootForRenderer);
    void paintOutlineForFragments(const LayerFragments&, GraphicsContext*, const LayerPaintingInfo&, PaintBehavior, RenderObject* paintingRootForRenderer);
    void paintOverflowControlsForFragments(const LayerFragments&, GraphicsContext*, const LayerPaintingInfo&);
    void paintMaskForFragments(const LayerFragments&, GraphicsContext*, const LayerPaintingInfo&, RenderObject* paintingRootForRenderer);
    void paintTransformedLayerIntoFragments(GraphicsContext*, const LayerPaintingInfo&, PaintLayerFlags);

    RenderLayer* hitTestLayer(RenderLayer* rootLayer, RenderLayer* containerLayer, const HitTestRequest& request, HitTestResult& result,
                              const LayoutRect& hitTestRect, const HitTestLocation&, bool appliedTransform,
                              const HitTestingTransformState* transformState = 0, double* zOffset = 0);
    RenderLayer* hitTestLayerByApplyingTransform(RenderLayer* rootLayer, RenderLayer* containerLayer, const HitTestRequest&, HitTestResult&,
        const LayoutRect& hitTestRect, const HitTestLocation&, const HitTestingTransformState* = 0, double* zOffset = 0,
        const LayoutPoint& translationOffset = LayoutPoint());
    RenderLayer* hitTestList(Vector<RenderLayer*>*, RenderLayer* rootLayer, const HitTestRequest& request, HitTestResult& result,
                             const LayoutRect& hitTestRect, const HitTestLocation&,
                             const HitTestingTransformState* transformState, double* zOffsetForDescendants, double* zOffset,
                             const HitTestingTransformState* unflattenedTransformState, bool depthSortDescendants);
    RenderLayer* hitTestPaginatedChildLayer(RenderLayer* childLayer, RenderLayer* rootLayer, const HitTestRequest& request, HitTestResult& result,
                                            const LayoutRect& hitTestRect, const HitTestLocation&,
                                            const HitTestingTransformState* transformState, double* zOffset);
    RenderLayer* hitTestChildLayerColumns(RenderLayer* childLayer, RenderLayer* rootLayer, const HitTestRequest& request, HitTestResult& result,
                                          const LayoutRect& hitTestRect, const HitTestLocation&,
                                          const HitTestingTransformState* transformState, double* zOffset,
                                          const Vector<RenderLayer*>& columnLayers, size_t columnIndex);

    PassRefPtr<HitTestingTransformState> createLocalTransformState(RenderLayer* rootLayer, RenderLayer* containerLayer,
                            const LayoutRect& hitTestRect, const HitTestLocation&,
                            const HitTestingTransformState* containerTransformState,
                            const LayoutPoint& translationOffset = LayoutPoint()) const;
    
    bool hitTestContents(const HitTestRequest&, HitTestResult&, const LayoutRect& layerBounds, const HitTestLocation&, HitTestFilter) const;
    bool hitTestContentsForFragments(const LayerFragments&, const HitTestRequest&, HitTestResult&, const HitTestLocation&, HitTestFilter, bool& insideClipRect) const;
    bool hitTestResizerInFragments(const LayerFragments&, const HitTestLocation&) const;
    RenderLayer* hitTestTransformedLayerInFragments(RenderLayer* rootLayer, RenderLayer* containerLayer, const HitTestRequest&, HitTestResult&,
        const LayoutRect& hitTestRect, const HitTestLocation&, const HitTestingTransformState* = 0, double* zOffset = 0);

    bool listBackgroundIsKnownToBeOpaqueInRect(const Vector<RenderLayer*>*, const LayoutRect&) const;

    void computeScrollDimensions();
    bool hasHorizontalOverflow() const;
    bool hasVerticalOverflow() const;
    bool hasScrollableHorizontalOverflow() const;
    bool hasScrollableVerticalOverflow() const;

    bool shouldBeNormalFlowOnly() const;

    bool shouldBeSelfPaintingLayer() const;

    int scrollPosition(Scrollbar*) const;
    
    // ScrollableArea interface
    virtual void invalidateScrollbarRect(Scrollbar*, const IntRect&);
    virtual void invalidateScrollCornerRect(const IntRect&);
    virtual bool isActive() const;
    virtual bool isScrollCornerVisible() const;
    virtual IntRect scrollCornerRect() const;
    virtual IntRect convertFromScrollbarToContainingView(const Scrollbar*, const IntRect&) const;
    virtual IntRect convertFromContainingViewToScrollbar(const Scrollbar*, const IntRect&) const;
    virtual IntPoint convertFromScrollbarToContainingView(const Scrollbar*, const IntPoint&) const;
    virtual IntPoint convertFromContainingViewToScrollbar(const Scrollbar*, const IntPoint&) const;
    virtual int scrollSize(ScrollbarOrientation) const;
    virtual void setScrollOffset(const IntPoint&);
    virtual IntPoint scrollPosition() const;
    virtual IntPoint minimumScrollPosition() const;
    virtual IntPoint maximumScrollPosition() const;
    virtual IntRect visibleContentRect(VisibleContentRectIncludesScrollbars) const;
    virtual int visibleHeight() const;
    virtual int visibleWidth() const;
    virtual IntSize contentsSize() const;
    virtual IntSize overhangAmount() const;
    virtual IntPoint lastKnownMousePosition() const;
    virtual bool isHandlingWheelEvent() const OVERRIDE;
    virtual bool shouldSuspendScrollAnimations() const;
    virtual bool scrollbarsCanBeActive() const;
    virtual IntRect scrollableAreaBoundingBox() const OVERRIDE;
    virtual bool scrollbarAnimationsAreSuppressed() const OVERRIDE;

    // Rectangle encompassing the scroll corner and resizer rect.
    IntRect scrollCornerAndResizerRect() const;

    // NOTE: This should only be called by the overriden setScrollOffset from ScrollableArea.
    void scrollTo(int, int);
    void updateCompositingLayersAfterScroll();

    IntSize scrollbarOffset(const Scrollbar*) const;
    
    void updateScrollableAreaSet(bool hasOverflow);

    void dirtyAncestorChainVisibleDescendantStatus();
    void setAncestorChainHasVisibleDescendant();

    void updateDescendantDependentFlags(HashSet<const RenderObject*>* outOfFlowDescendantContainingBlocks = 0);

    // This flag is computed by RenderLayerCompositor, which knows more about 3d hierarchies than we do.
    void setHas3DTransformedDescendant(bool b) { m_has3DTransformedDescendant = b; }
    bool has3DTransformedDescendant() const { return m_has3DTransformedDescendant; }
    
    void dirty3DTransformedDescendantStatus();
    // Both updates the status, and returns true if descendants of this have 3d.
    bool update3DTransformedDescendantStatus();

    void createReflection();
    void removeReflection();

    void updateReflectionStyle();
    bool paintingInsideReflection() const { return m_paintingInsideReflection; }
    void setPaintingInsideReflection(bool b) { m_paintingInsideReflection = b; }

#if ENABLE(CSS_FILTERS)
    void updateOrRemoveFilterClients();
    void updateOrRemoveFilterEffectRenderer();
#endif

    void parentClipRects(const ClipRectsContext&, ClipRects&) const;
    ClipRect backgroundClipRect(const ClipRectsContext&) const;

    LayoutRect paintingExtent(const RenderLayer* rootLayer, const LayoutRect& paintDirtyRect, PaintBehavior);

    RenderLayer* enclosingTransformedAncestor() const;

    // Convert a point in absolute coords into layer coords, taking transforms into account
    LayoutPoint absoluteToContents(const LayoutPoint&) const;

    void positionOverflowControls(const IntSize&);
    void updateScrollCornerStyle();
    void updateResizerStyle();

    void drawPlatformResizerImage(GraphicsContext*, IntRect resizerCornerRect);

    void updatePagination();
    
    // FIXME: Temporary. Remove when new columns come online.
    bool useRegionBasedColumns() const;
    
#if USE(ACCELERATED_COMPOSITING)    
    bool hasCompositingDescendant() const { return m_hasCompositingDescendant; }
    void setHasCompositingDescendant(bool b)  { m_hasCompositingDescendant = b; }
    
    enum IndirectCompositingReason {
        NoIndirectCompositingReason,
        IndirectCompositingForStacking,
        IndirectCompositingForOverlap,
        IndirectCompositingForBackgroundLayer,
        IndirectCompositingForGraphicalEffect, // opacity, mask, filter, transform etc.
        IndirectCompositingForPerspective,
        IndirectCompositingForPreserve3D
    };
    
    void setIndirectCompositingReason(IndirectCompositingReason reason) { m_indirectCompositingReason = reason; }
    IndirectCompositingReason indirectCompositingReason() const { return static_cast<IndirectCompositingReason>(m_indirectCompositingReason); }
    bool mustCompositeForIndirectReasons() const { return m_indirectCompositingReason; }
#endif

    // Returns true if z ordering would not change if this layer were a stacking container.
    bool canBeStackingContainer() const;

    friend class RenderLayerBacking;
    friend class RenderLayerCompositor;
    friend class RenderLayerModelObject;

    // Only safe to call from RenderBoxModelObject::destroyLayer(RenderArena*)
    void destroy(RenderArena*);

    LayoutUnit overflowTop() const;
    LayoutUnit overflowBottom() const;
    LayoutUnit overflowLeft() const;
    LayoutUnit overflowRight() const;

    IntRect rectForHorizontalScrollbar(const IntRect& borderBoxRect) const;
    IntRect rectForVerticalScrollbar(const IntRect& borderBoxRect) const;

    LayoutUnit verticalScrollbarStart(int minX, int maxX) const;
    LayoutUnit horizontalScrollbarStart(int minX) const;

    bool overflowControlsIntersectRect(const IntRect& localRect) const;

protected:
    // The bitfields are up here so they will fall into the padding from ScrollableArea on 64-bit.

    // Keeps track of whether the layer is currently resizing, so events can cause resizing to start and stop.
    bool m_inResizeMode : 1;

    bool m_scrollDimensionsDirty : 1;
    bool m_zOrderListsDirty : 1;
    bool m_normalFlowListDirty: 1;
    bool m_isNormalFlowOnly : 1;

    bool m_isSelfPaintingLayer : 1;

    // If have no self-painting descendants, we don't have to walk our children during painting. This can lead to
    // significant savings, especially if the tree has lots of non-self-painting layers grouped together (e.g. table cells).
    bool m_hasSelfPaintingLayerDescendant : 1;
    bool m_hasSelfPaintingLayerDescendantDirty : 1;

    // If we have no out of flow positioned descendants and no non-descendant
    // appears between our descendants in stacking order, then we may become a
    // stacking context.
    bool m_hasOutOfFlowPositionedDescendant : 1;
    bool m_hasOutOfFlowPositionedDescendantDirty : 1;

    bool m_needsCompositedScrolling : 1;

    // If this is true, then no non-descendant appears between any of our
    // descendants in stacking order. This is one of the requirements of being
    // able to safely become a stacking context.
    bool m_descendantsAreContiguousInStackingOrder : 1;

    const bool m_isRootLayer : 1;

    bool m_usedTransparency : 1; // Tracks whether we need to close a transparent layer, i.e., whether
                                 // we ended up painting this layer or any descendants (and therefore need to
                                 // blend).
    bool m_paintingInsideReflection : 1;  // A state bit tracking if we are painting inside a replica.
    bool m_inOverflowRelayout : 1;
    unsigned m_repaintStatus : 2; // RepaintStatus

    bool m_visibleContentStatusDirty : 1;
    bool m_hasVisibleContent : 1;
    bool m_visibleDescendantStatusDirty : 1;
    bool m_hasVisibleDescendant : 1;

    bool m_isPaginated : 1; // If we think this layer is split by a multi-column ancestor, then this bit will be set.

    bool m_3DTransformedDescendantStatusDirty : 1;
    bool m_has3DTransformedDescendant : 1;  // Set on a stacking context layer that has 3D descendants anywhere
                                            // in a preserves3D hierarchy. Hint to do 3D-aware hit testing.
#if USE(ACCELERATED_COMPOSITING)
    bool m_hasCompositingDescendant : 1; // In the z-order tree.
    unsigned m_indirectCompositingReason : 3;
    unsigned m_viewportConstrainedNotCompositedReason : 2;
#endif

    bool m_containsDirtyOverlayScrollbars : 1;
    bool m_updatingMarqueePosition : 1;

#if !ASSERT_DISABLED
    bool m_layerListMutationAllowed : 1;
#endif

#if ENABLE(CSS_FILTERS)
    bool m_hasFilterInfo : 1;
#endif

#if ENABLE(CSS_COMPOSITING)
    BlendMode m_blendMode;
#endif

    RenderLayerModelObject* m_renderer;

    RenderLayer* m_parent;
    RenderLayer* m_previous;
    RenderLayer* m_next;
    RenderLayer* m_first;
    RenderLayer* m_last;

    LayoutRect m_repaintRect; // Cached repaint rects. Used by layout.
    LayoutRect m_outlineBox;

    // Our current relative position offset.
    LayoutSize m_offsetForInFlowPosition;

    // Our (x,y) coordinates are in our parent layer's coordinate space.
    LayoutPoint m_topLeft;

    // The layer's width/height
    IntSize m_layerSize;

    // This is the (scroll) offset from scrollOrigin().
    IntSize m_scrollOffset;

    // The width/height of our scrolled area.
    LayoutSize m_scrollSize;

    // For layers with overflow, we have a pair of scrollbars.
    RefPtr<Scrollbar> m_hBar;
    RefPtr<Scrollbar> m_vBar;

    // For layers that establish stacking contexts, m_posZOrderList holds a sorted list of all the
    // descendant layers within the stacking context that have z-indices of 0 or greater
    // (auto will count as 0).  m_negZOrderList holds descendants within our stacking context with negative
    // z-indices.
    OwnPtr<Vector<RenderLayer*> > m_posZOrderList;
    OwnPtr<Vector<RenderLayer*> > m_negZOrderList;

    // This list contains child layers that cannot create stacking contexts.  For now it is just
    // overflow layers, but that may change in the future.
    OwnPtr<Vector<RenderLayer*> > m_normalFlowList;

    OwnPtr<ClipRectsCache> m_clipRectsCache;
    
    IntPoint m_cachedOverlayScrollbarOffset;

    OwnPtr<RenderMarquee> m_marquee; // Used by layers with overflow:marquee
    
    // Cached normal flow values for absolute positioned elements with static left/top values.
    LayoutUnit m_staticInlinePosition;
    LayoutUnit m_staticBlockPosition;

    OwnPtr<TransformationMatrix> m_transform;
    
    // May ultimately be extended to many replicas (with their own paint order).
    RenderReplica* m_reflection;
        
    // Renderers to hold our custom scroll corner and resizer.
    RenderScrollbarPart* m_scrollCorner;
    RenderScrollbarPart* m_resizer;

    // Pointer to the enclosing RenderLayer that caused us to be paginated. It is 0 if we are not paginated.
    RenderLayer* m_enclosingPaginationLayer;

private:
    IntRect m_blockSelectionGapsBounds;

#if USE(ACCELERATED_COMPOSITING)
    OwnPtr<RenderLayerBacking> m_backing;
#endif
};

inline void RenderLayer::clearZOrderLists()
{
    ASSERT(!isStackingContainer());

    m_posZOrderList.clear();
    m_negZOrderList.clear();
}

inline void RenderLayer::updateZOrderLists()
{
    if (!m_zOrderListsDirty)
        return;

    if (!isStackingContainer()) {
        clearZOrderLists();
        m_zOrderListsDirty = false;
        return;
    }

    rebuildZOrderLists();
}

#if !ASSERT_DISABLED
class LayerListMutationDetector {
public:
    LayerListMutationDetector(RenderLayer* layer)
        : m_layer(layer)
        , m_previousMutationAllowedState(layer->layerListMutationAllowed())
    {
        m_layer->setLayerListMutationAllowed(false);
    }
    
    ~LayerListMutationDetector()
    {
        m_layer->setLayerListMutationAllowed(m_previousMutationAllowedState);
    }

private:
    RenderLayer* m_layer;
    bool m_previousMutationAllowedState;
};
#endif

void makeMatrixRenderable(TransformationMatrix&, bool has3DRendering);

} // namespace WebCore

#ifndef NDEBUG
// Outside the WebCore namespace for ease of invocation from gdb.
void showLayerTree(const WebCore::RenderLayer*);
void showLayerTree(const WebCore::RenderObject*);
#endif

#endif // RenderLayer_h
