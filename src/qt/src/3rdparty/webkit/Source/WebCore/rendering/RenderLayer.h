/*
 * Copyright (C) 2003, 2009 Apple Inc. All rights reserved.
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
#include "ScrollBehavior.h"
#include "ScrollableArea.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

class HitTestRequest;
class HitTestResult;
class HitTestingTransformState;
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

class ClipRects {
public:
    ClipRects()
        : m_refCnt(0)
        , m_fixed(false)
    {
    }

    ClipRects(const IntRect& r)
        : m_overflowClipRect(r)
        , m_fixedClipRect(r)
        , m_posClipRect(r)
        , m_refCnt(0)
        , m_fixed(false)
    {
    }

    ClipRects(const ClipRects& other)
        : m_overflowClipRect(other.overflowClipRect())
        , m_fixedClipRect(other.fixedClipRect())
        , m_posClipRect(other.posClipRect())
        , m_refCnt(0)
        , m_fixed(other.fixed())
    {
    }

    void reset(const IntRect& r)
    {
        m_overflowClipRect = r;
        m_fixedClipRect = r;
        m_posClipRect = r;
        m_fixed = false;
    }
    
    const IntRect& overflowClipRect() const { return m_overflowClipRect; }
    void setOverflowClipRect(const IntRect& r) { m_overflowClipRect = r; }

    const IntRect& fixedClipRect() const { return m_fixedClipRect; }
    void setFixedClipRect(const IntRect&r) { m_fixedClipRect = r; }

    const IntRect& posClipRect() const { return m_posClipRect; }
    void setPosClipRect(const IntRect& r) { m_posClipRect = r; }

    bool fixed() const { return m_fixed; }
    void setFixed(bool fixed) { m_fixed = fixed; }

    void ref() { m_refCnt++; }
    void deref(RenderArena* renderArena) { if (--m_refCnt == 0) destroy(renderArena); }

    void destroy(RenderArena*);

    // Overloaded new operator.
    void* operator new(size_t, RenderArena*) throw();

    // Overridden to prevent the normal delete from being called.
    void operator delete(void*, size_t);

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
    // The normal operator new is disallowed on all render objects.
    void* operator new(size_t) throw();

private:
    IntRect m_overflowClipRect;
    IntRect m_fixedClipRect;
    IntRect m_posClipRect;
    unsigned m_refCnt : 31;
    bool m_fixed : 1;
};

class RenderLayer : public ScrollableArea {
public:
    friend class RenderReplica;

    RenderLayer(RenderBoxModelObject*);
    ~RenderLayer();

    RenderBoxModelObject* renderer() const { return m_renderer; }
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
    void setBackingNeedsRepaintInRect(const IntRect& r); // r is in the coordinate space of the layer's render object
    void repaintIncludingNonCompositingDescendants(RenderBoxModelObject* repaintContainer);
#endif

    void styleChanged(StyleDifference, const RenderStyle* oldStyle);

    RenderMarquee* marquee() const { return m_marquee; }

    bool isNormalFlowOnly() const { return m_isNormalFlowOnly; }
    bool isSelfPaintingLayer() const;

    bool requiresSlowRepaints() const;

    bool isTransparent() const;
    RenderLayer* transparentPaintingAncestor();
    void beginTransparencyLayers(GraphicsContext*, const RenderLayer* rootLayer, PaintBehavior);

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
    
    int x() const { return m_x; }
    int y() const { return m_y; }
    void setLocation(int x, int y)
    {
        m_x = x;
        m_y = y;
    }

    int width() const { return m_width; }
    int height() const { return m_height; }
    IntSize size() const { return IntSize(m_width, m_height); }
    
    void setWidth(int w) { m_width = w; }
    void setHeight(int h) { m_height = h; }

    int scrollWidth();
    int scrollHeight();

    void panScrollFromPoint(const IntPoint&);

    // Scrolling methods for layers that can scroll their overflow.
    void scrollByRecursively(int xDelta, int yDelta);

    IntSize scrolledContentOffset() const { return IntSize(scrollXOffset() + m_scrollLeftOverflow, scrollYOffset() + m_scrollTopOverflow); }

    int scrollXOffset() const { return m_scrollX + m_scrollOrigin.x(); }
    int scrollYOffset() const { return m_scrollY + m_scrollOrigin.y(); }

    void scrollToOffset(int x, int y);
    void scrollToXOffset(int x) { scrollToOffset(x, m_scrollY + m_scrollOrigin.y()); }
    void scrollToYOffset(int y) { scrollToOffset(m_scrollX + m_scrollOrigin.x(), y); }
    void scrollRectToVisible(const IntRect&, bool scrollToAnchor = false, const ScrollAlignment& alignX = ScrollAlignment::alignCenterIfNeeded, const ScrollAlignment& alignY = ScrollAlignment::alignCenterIfNeeded);

    IntRect getRectToExpose(const IntRect& visibleRect, const IntRect& exposeRect, const ScrollAlignment& alignX, const ScrollAlignment& alignY);

    void setHasHorizontalScrollbar(bool);
    void setHasVerticalScrollbar(bool);

    PassRefPtr<Scrollbar> createScrollbar(ScrollbarOrientation);
    void destroyScrollbar(ScrollbarOrientation);

    // ScrollableArea overrides
    virtual Scrollbar* horizontalScrollbar() const { return m_hBar.get(); }
    virtual Scrollbar* verticalScrollbar() const { return m_vBar.get(); }

    int verticalScrollbarWidth(OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize) const;
    int horizontalScrollbarHeight(OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize) const;

    bool hasOverflowControls() const;
    bool isPointInResizeControl(const IntPoint& absolutePoint) const;
    bool hitTestOverflowControls(HitTestResult&, const IntPoint& localPoint);
    IntSize offsetFromResizeCorner(const IntPoint& absolutePoint) const;

    void paintOverflowControls(GraphicsContext*, int tx, int ty, const IntRect& damageRect, bool paintingOverlayControls = false);
    void paintScrollCorner(GraphicsContext*, int tx, int ty, const IntRect& damageRect);
    void paintResizer(GraphicsContext*, int tx, int ty, const IntRect& damageRect);

    void updateScrollInfoAfterLayout();

    bool scroll(ScrollDirection, ScrollGranularity, float multiplier = 1);
    void autoscroll();

    void resize(const PlatformMouseEvent&, const IntSize&);
    bool inResizeMode() const { return m_inResizeMode; }
    void setInResizeMode(bool b) { m_inResizeMode = b; }

    bool isRootLayer() const { return renderer()->isRenderView(); }
    
#if USE(ACCELERATED_COMPOSITING)
    RenderLayerCompositor* compositor() const;
    
    // Notification from the renderer that its content changed (e.g. current frame of image changed).
    // Allows updates of layer content without repainting.
    enum ContentChangeType { ImageChanged, MaskImageChanged, CanvasChanged, VideoChanged, FullScreenChanged };
    void contentChanged(ContentChangeType);
#endif

    // Returns true if the accelerated compositing is enabled
    bool hasAcceleratedCompositing() const;

    bool canRender3DTransforms() const;

    void updateLayerPosition();
    
    enum UpdateLayerPositionsFlag {
        CheckForRepaint = 1,
        IsCompositingUpdateRoot = 1 << 1,
        UpdateCompositingLayers = 1 << 2,
        UpdatePagination = 1 << 3
    };
    typedef unsigned UpdateLayerPositionsFlags;
    void updateLayerPositions(UpdateLayerPositionsFlags = CheckForRepaint | IsCompositingUpdateRoot | UpdateCompositingLayers, IntPoint* cachedOffset = 0);

    void updateTransform();

    void relativePositionOffset(int& relX, int& relY) const { relX += m_relX; relY += m_relY; }
    IntSize relativePositionOffset() const { return IntSize(m_relX, m_relY); }

    void clearClipRectsIncludingDescendants();
    void clearClipRects();

    void addBlockSelectionGapsBounds(const IntRect&);
    void clearBlockSelectionGapsBounds();
    void repaintBlockSelectionGaps();

    // Get the enclosing stacking context for this layer.  A stacking context is a layer
    // that has a non-auto z-index.
    RenderLayer* stackingContext() const;
    bool isStackingContext() const { return !hasAutoZIndex() || renderer()->isRenderView(); }

    void dirtyZOrderLists();
    void dirtyStackingContextZOrderLists();
    void updateZOrderLists();
    Vector<RenderLayer*>* posZOrderList() const { return m_posZOrderList; }
    Vector<RenderLayer*>* negZOrderList() const { return m_negZOrderList; }

    void dirtyNormalFlowList();
    void updateNormalFlowList();
    Vector<RenderLayer*>* normalFlowList() const { return m_normalFlowList; }

    bool hasVisibleContent() const { return m_hasVisibleContent; }
    bool hasVisibleDescendant() const { return m_hasVisibleDescendant; }
    void setHasVisibleContent(bool);
    void dirtyVisibleContentStatus();

    // Gets the nearest enclosing positioned ancestor layer (also includes
    // the <html> layer and the root layer).
    RenderLayer* enclosingPositionedAncestor() const;

    // The layer relative to which clipping rects for this layer are computed.
    RenderLayer* clippingRoot() const;

#if USE(ACCELERATED_COMPOSITING)
    // Enclosing compositing layer; if includeSelf is true, may return this.
    RenderLayer* enclosingCompositingLayer(bool includeSelf = true) const;
    // Ancestor compositing layer, excluding this.
    RenderLayer* ancestorCompositingLayer() const { return enclosingCompositingLayer(false); }
#endif

    void convertToLayerCoords(const RenderLayer* ancestorLayer, int& x, int& y) const;

    bool hasAutoZIndex() const { return renderer()->style()->hasAutoZIndex(); }
    int zIndex() const { return renderer()->style()->zIndex(); }

    // The two main functions that use the layer system.  The paint method
    // paints the layers that intersect the damage rect from back to
    // front.  The hitTest method looks for mouse events by walking
    // layers that intersect the point from front to back.
    void paint(GraphicsContext*, const IntRect& damageRect, PaintBehavior = PaintBehaviorNormal, RenderObject* paintingRoot = 0);
    bool hitTest(const HitTestRequest&, HitTestResult&);
    void paintOverlayScrollbars(GraphicsContext*, const IntRect& damageRect, PaintBehavior, RenderObject* paintingRoot);

    // This method figures out our layerBounds in coordinates relative to
    // |rootLayer}.  It also computes our background and foreground clip rects
    // for painting/event handling.
    void calculateRects(const RenderLayer* rootLayer, const IntRect& paintDirtyRect, IntRect& layerBounds,
                        IntRect& backgroundRect, IntRect& foregroundRect, IntRect& outlineRect, bool temporaryClipRects = false,
                        OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize) const;

    // Compute and cache clip rects computed with the given layer as the root
    void updateClipRects(const RenderLayer* rootLayer, OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize);
    // Compute and return the clip rects. If useCached is true, will used previously computed clip rects on ancestors
    // (rather than computing them all from scratch up the parent chain).
    void calculateClipRects(const RenderLayer* rootLayer, ClipRects&, bool useCached = false, OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize) const;
    ClipRects* clipRects() const { return m_clipRects; }

    IntRect childrenClipRect() const; // Returns the foreground clip rect of the layer in the document's coordinate space.
    IntRect selfClipRect() const; // Returns the background clip rect of the layer in the document's coordinate space.

    bool intersectsDamageRect(const IntRect& layerBounds, const IntRect& damageRect, const RenderLayer* rootLayer) const;

    // Bounding box relative to some ancestor layer.
    IntRect boundingBox(const RenderLayer* rootLayer) const;
    // Bounding box in the coordinates of this layer.
    IntRect localBoundingBox() const;
    // Bounding box relative to the root.
    IntRect absoluteBoundingBox() const;

    void updateHoverActiveState(const HitTestRequest&, HitTestResult&);

    // Return a cached repaint rect, computed relative to the layer renderer's containerForRepaint.
    IntRect repaintRect() const { return m_repaintRect; }
    IntRect repaintRectIncludingDescendants() const;
    void computeRepaintRects();
    void updateRepaintRectsAfterScroll(bool fixed = false);
    void setNeedsFullRepaint(bool f = true) { m_needsFullRepaint = f; }
    
    int staticInlinePosition() const { return m_staticInlinePosition; }
    int staticBlockPosition() const { return m_staticBlockPosition; }
    void setStaticInlinePosition(int position) { m_staticInlinePosition = position; }
    void setStaticBlockPosition(int position) { m_staticBlockPosition = position; }

    bool hasTransform() const { return renderer()->hasTransform(); }
    // Note that this transform has the transform-origin baked in.
    TransformationMatrix* transform() const { return m_transform.get(); }
    // currentTransform computes a transform which takes accelerated animations into account. The
    // resulting transform has transform-origin baked in. If the layer does not have a transform,
    // returns the identity matrix.
    TransformationMatrix currentTransform() const;
    TransformationMatrix renderableTransform(PaintBehavior) const;
    
    // Get the perspective transform, which is applied to transformed sublayers.
    // Returns true if the layer has a -webkit-perspective.
    // Note that this transform has the perspective-origin baked in.
    TransformationMatrix perspectiveTransform() const;
    FloatPoint perspectiveOrigin() const;
    bool preserves3D() const { return renderer()->style()->transformStyle3D() == TransformStyle3DPreserve3D; }
    bool has3DTransform() const { return m_transform && !m_transform->isAffine(); }

     // Overloaded new operator.  Derived classes must override operator new
    // in order to allocate out of the RenderArena.
    void* operator new(size_t, RenderArena*) throw();

    // Overridden to prevent the normal delete from being called.
    void operator delete(void*, size_t);

#if USE(ACCELERATED_COMPOSITING)
    bool isComposited() const { return m_backing != 0; }
    bool hasCompositedMask() const;
    RenderLayerBacking* backing() const { return m_backing.get(); }
    RenderLayerBacking* ensureBacking();
    void clearBacking();
    virtual GraphicsLayer* layerForHorizontalScrollbar() const;
    virtual GraphicsLayer* layerForVerticalScrollbar() const;
    virtual GraphicsLayer* layerForScrollCorner() const;
#else
    bool isComposited() const { return false; }
    bool hasCompositedMask() const { return false; }
#endif

    bool paintsWithTransparency(PaintBehavior paintBehavior) const
    {
        return isTransparent() && ((paintBehavior & PaintBehaviorFlattenCompositingLayers) || !isComposited());
    }

    bool paintsWithTransform(PaintBehavior) const;

    bool containsDirtyOverlayScrollbars() const { return m_containsDirtyOverlayScrollbars; }
    void setContainsDirtyOverlayScrollbars(bool dirtyScrollbars) { m_containsDirtyOverlayScrollbars = dirtyScrollbars; }

private:
    // The normal operator new is disallowed on all render objects.
    void* operator new(size_t) throw();

    void setNextSibling(RenderLayer* next) { m_next = next; }
    void setPreviousSibling(RenderLayer* prev) { m_previous = prev; }
    void setParent(RenderLayer* parent);
    void setFirstChild(RenderLayer* first) { m_first = first; }
    void setLastChild(RenderLayer* last) { m_last = last; }

    int renderBoxX() const { return renderer()->isBox() ? toRenderBox(renderer())->x() : 0; }
    int renderBoxY() const { return renderer()->isBox() ? toRenderBox(renderer())->y() : 0; }

    void collectLayers(Vector<RenderLayer*>*&, Vector<RenderLayer*>*&);

    void updateLayerListsIfNeeded();
    void updateCompositingAndLayerListsIfNeeded();
    
    enum PaintLayerFlag {
        PaintLayerHaveTransparency = 1,
        PaintLayerAppliedTransform = 1 << 1,
        PaintLayerTemporaryClipRects = 1 << 2,
        PaintLayerPaintingReflection = 1 << 3,
        PaintLayerPaintingOverlayScrollbars = 1 << 4
    };
    
    typedef unsigned PaintLayerFlags;

    void paintLayer(RenderLayer* rootLayer, GraphicsContext*, const IntRect& paintDirtyRect,
                    PaintBehavior, RenderObject* paintingRoot, OverlapTestRequestMap* = 0,
                    PaintLayerFlags = 0);
    void paintList(Vector<RenderLayer*>*, RenderLayer* rootLayer, GraphicsContext* p,
                   const IntRect& paintDirtyRect, PaintBehavior,
                   RenderObject* paintingRoot, OverlapTestRequestMap*,
                   PaintLayerFlags);
    void paintPaginatedChildLayer(RenderLayer* childLayer, RenderLayer* rootLayer, GraphicsContext*,
                                  const IntRect& paintDirtyRect, PaintBehavior,
                                  RenderObject* paintingRoot, OverlapTestRequestMap*,
                                  PaintLayerFlags);
    void paintChildLayerIntoColumns(RenderLayer* childLayer, RenderLayer* rootLayer, GraphicsContext*,
                                    const IntRect& paintDirtyRect, PaintBehavior,
                                    RenderObject* paintingRoot, OverlapTestRequestMap*,
                                    PaintLayerFlags, const Vector<RenderLayer*>& columnLayers, size_t columnIndex);

    RenderLayer* hitTestLayer(RenderLayer* rootLayer, RenderLayer* containerLayer, const HitTestRequest& request, HitTestResult& result,
                              const IntRect& hitTestRect, const IntPoint& hitTestPoint, bool appliedTransform,
                              const HitTestingTransformState* transformState = 0, double* zOffset = 0);
    RenderLayer* hitTestList(Vector<RenderLayer*>*, RenderLayer* rootLayer, const HitTestRequest& request, HitTestResult& result,
                             const IntRect& hitTestRect, const IntPoint& hitTestPoint,
                             const HitTestingTransformState* transformState, double* zOffsetForDescendants, double* zOffset,
                             const HitTestingTransformState* unflattenedTransformState, bool depthSortDescendants);
    RenderLayer* hitTestPaginatedChildLayer(RenderLayer* childLayer, RenderLayer* rootLayer, const HitTestRequest& request, HitTestResult& result,
                                            const IntRect& hitTestRect, const IntPoint& hitTestPoint,
                                            const HitTestingTransformState* transformState, double* zOffset);
    RenderLayer* hitTestChildLayerColumns(RenderLayer* childLayer, RenderLayer* rootLayer, const HitTestRequest& request, HitTestResult& result,
                                          const IntRect& hitTestRect, const IntPoint& hitTestPoint,
                                          const HitTestingTransformState* transformState, double* zOffset,
                                          const Vector<RenderLayer*>& columnLayers, size_t columnIndex);
                                    
    PassRefPtr<HitTestingTransformState> createLocalTransformState(RenderLayer* rootLayer, RenderLayer* containerLayer,
                            const IntRect& hitTestRect, const IntPoint& hitTestPoint,
                            const HitTestingTransformState* containerTransformState) const;
    
    bool hitTestContents(const HitTestRequest&, HitTestResult&, const IntRect& layerBounds, const IntPoint& hitTestPoint, HitTestFilter) const;
    
    void computeScrollDimensions(bool* needHBar = 0, bool* needVBar = 0);

    bool shouldBeNormalFlowOnly() const; 

    // ScrollableArea interface
    virtual int scrollSize(ScrollbarOrientation orientation) const;
    virtual void setScrollOffset(const IntPoint&);
    virtual int scrollPosition(Scrollbar*) const;
    virtual void invalidateScrollbarRect(Scrollbar*, const IntRect&);
    virtual void invalidateScrollCornerRect(const IntRect&);
    virtual bool isActive() const;
    virtual bool isScrollCornerVisible() const;
    virtual IntRect scrollCornerRect() const;
    virtual IntRect convertFromScrollbarToContainingView(const Scrollbar*, const IntRect&) const;
    virtual IntRect convertFromContainingViewToScrollbar(const Scrollbar*, const IntRect&) const;
    virtual IntPoint convertFromScrollbarToContainingView(const Scrollbar*, const IntPoint&) const;
    virtual IntPoint convertFromContainingViewToScrollbar(const Scrollbar*, const IntPoint&) const;
    virtual IntSize contentsSize() const;
    virtual int visibleHeight() const;
    virtual int visibleWidth() const;
    virtual IntPoint currentMousePosition() const;
    virtual bool shouldSuspendScrollAnimations() const;

    // Rectangle encompassing the scroll corner and resizer rect.
    IntRect scrollCornerAndResizerRect() const;

    virtual void disconnectFromPage() { m_page = 0; }

    // NOTE: This should only be called by the overriden setScrollOffset from ScrollableArea.
    void scrollTo(int x, int y);

    IntSize scrollbarOffset(const Scrollbar*) const;
    
    void updateOverflowStatus(bool horizontalOverflow, bool verticalOverflow);

    void childVisibilityChanged(bool newVisibility);
    void dirtyVisibleDescendantStatus();
    void updateVisibilityStatus();

    // This flag is computed by RenderLayerCompositor, which knows more about 3d hierarchies than we do.
    void setHas3DTransformedDescendant(bool b) { m_has3DTransformedDescendant = b; }
    bool has3DTransformedDescendant() const { return m_has3DTransformedDescendant; }
    
    void dirty3DTransformedDescendantStatus();
    // Both updates the status, and returns true if descendants of this have 3d.
    bool update3DTransformedDescendantStatus();

    Node* enclosingElement() const;

    void createReflection();
    void removeReflection();

    void updateReflectionStyle();
    bool paintingInsideReflection() const { return m_paintingInsideReflection; }
    void setPaintingInsideReflection(bool b) { m_paintingInsideReflection = b; }
    
    void parentClipRects(const RenderLayer* rootLayer, ClipRects&, bool temporaryClipRects = false, OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize) const;
    IntRect backgroundClipRect(const RenderLayer* rootLayer, bool temporaryClipRects, OverlayScrollbarSizeRelevancy = IgnoreOverlayScrollbarSize) const;

    RenderLayer* enclosingTransformedAncestor() const;

    // Convert a point in absolute coords into layer coords, taking transforms into account
    IntPoint absoluteToContents(const IntPoint&) const;

    void positionOverflowControls(int tx, int ty);
    void updateScrollCornerStyle();
    void updateResizerStyle();

    void updatePagination();
    bool isPaginated() const { return m_isPaginated; }

#if USE(ACCELERATED_COMPOSITING)    
    bool hasCompositingDescendant() const { return m_hasCompositingDescendant; }
    void setHasCompositingDescendant(bool b)  { m_hasCompositingDescendant = b; }
    
    bool mustOverlapCompositedLayers() const { return m_mustOverlapCompositedLayers; }
    void setMustOverlapCompositedLayers(bool b) { m_mustOverlapCompositedLayers = b; }
#endif

    void updateContentsScale(float);

    friend class RenderLayerBacking;
    friend class RenderLayerCompositor;
    friend class RenderBoxModelObject;

    // Only safe to call from RenderBoxModelObject::destroyLayer(RenderArena*)
    void destroy(RenderArena*);

    int overflowTop() const;
    int overflowBottom() const;
    int overflowLeft() const;
    int overflowRight() const;

protected:
    RenderBoxModelObject* m_renderer;

    RenderLayer* m_parent;
    RenderLayer* m_previous;
    RenderLayer* m_next;
    RenderLayer* m_first;
    RenderLayer* m_last;

    IntRect m_repaintRect; // Cached repaint rects. Used by layout.
    IntRect m_outlineBox;

    // Our current relative position offset.
    int m_relX;
    int m_relY;

    // Our (x,y) coordinates are in our parent layer's coordinate space.
    int m_x;
    int m_y;

    // The layer's width/height
    int m_width;
    int m_height;

    // Our scroll offsets if the view is scrolled.
    int m_scrollX;
    int m_scrollY;
    
    int m_scrollLeftOverflow;
    int m_scrollTopOverflow;
    
    // The width/height of our scrolled area.
    int m_scrollWidth;
    int m_scrollHeight;

    // For layers with overflow, we have a pair of scrollbars.
    RefPtr<Scrollbar> m_hBar;
    RefPtr<Scrollbar> m_vBar;

    // Keeps track of whether the layer is currently resizing, so events can cause resizing to start and stop.
    bool m_inResizeMode;

    // For layers that establish stacking contexts, m_posZOrderList holds a sorted list of all the
    // descendant layers within the stacking context that have z-indices of 0 or greater
    // (auto will count as 0).  m_negZOrderList holds descendants within our stacking context with negative
    // z-indices.
    Vector<RenderLayer*>* m_posZOrderList;
    Vector<RenderLayer*>* m_negZOrderList;

    // This list contains child layers that cannot create stacking contexts.  For now it is just
    // overflow layers, but that may change in the future.
    Vector<RenderLayer*>* m_normalFlowList;

    ClipRects* m_clipRects;      // Cached clip rects used when painting and hit testing.
#ifndef NDEBUG
    const RenderLayer* m_clipRectsRoot;   // Root layer used to compute clip rects.
#endif

    bool m_scrollDimensionsDirty : 1;
    bool m_zOrderListsDirty : 1;
    bool m_normalFlowListDirty: 1;
    bool m_isNormalFlowOnly : 1;

    bool m_usedTransparency : 1; // Tracks whether we need to close a transparent layer, i.e., whether
                                 // we ended up painting this layer or any descendants (and therefore need to
                                 // blend).
    bool m_paintingInsideReflection : 1;  // A state bit tracking if we are painting inside a replica.
    bool m_inOverflowRelayout : 1;
    bool m_needsFullRepaint : 1;

    bool m_overflowStatusDirty : 1;
    bool m_horizontalOverflow : 1;
    bool m_verticalOverflow : 1;
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
    bool m_mustOverlapCompositedLayers : 1;
#endif

    bool m_containsDirtyOverlayScrollbars : 1;

    IntPoint m_cachedOverlayScrollbarOffset;

    RenderMarquee* m_marquee; // Used by layers with overflow:marquee
    
    // Cached normal flow values for absolute positioned elements with static left/top values.
    int m_staticInlinePosition;
    int m_staticBlockPosition;
    
    OwnPtr<TransformationMatrix> m_transform;
    
    // May ultimately be extended to many replicas (with their own paint order).
    RenderReplica* m_reflection;
    
    // Renderers to hold our custom scroll corner and resizer.
    RenderScrollbarPart* m_scrollCorner;
    RenderScrollbarPart* m_resizer;

private:
    IntRect m_blockSelectionGapsBounds;

#if USE(ACCELERATED_COMPOSITING)
    OwnPtr<RenderLayerBacking> m_backing;
#endif

    Page* m_page;
};

} // namespace WebCore

#ifndef NDEBUG
// Outside the WebCore namespace for ease of invocation from gdb.
void showLayerTree(const WebCore::RenderLayer*);
void showLayerTree(const WebCore::RenderObject*);
#endif

#endif // RenderLayer_h
