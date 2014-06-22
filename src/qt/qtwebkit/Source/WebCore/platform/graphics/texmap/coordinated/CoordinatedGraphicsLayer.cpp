/*
 Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 Copyright (C) 2010 Apple Inc. All rights reserved.
 Copyright (C) 2012 Company 100, Inc.
 Copyright (C) 2012 Intel Corporation. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 */

#include "config.h"

#if USE(COORDINATED_GRAPHICS)
#include "CoordinatedGraphicsLayer.h"

#include "CoordinatedTile.h"
#include "FloatQuad.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "GraphicsLayer.h"
#include "Page.h"
#include "ScrollableArea.h"
#include "TextureMapperPlatformLayer.h"
#include <wtf/CurrentTime.h>
#include <wtf/HashMap.h>
#ifndef NDEBUG
#include <wtf/TemporaryChange.h>
#endif
#include <wtf/text/CString.h>

namespace WebCore {

static CoordinatedLayerID toCoordinatedLayerID(GraphicsLayer* layer)
{
    return layer ? toCoordinatedGraphicsLayer(layer)->id() : 0;
}

void CoordinatedGraphicsLayer::didChangeLayerState()
{
    m_shouldSyncLayerState = true;
    if (client())
        client()->notifyFlushRequired(this);
}

void CoordinatedGraphicsLayer::didChangeAnimations()
{
    m_shouldSyncAnimations = true;
    if (client())
        client()->notifyFlushRequired(this);
}

void CoordinatedGraphicsLayer::didChangeChildren()
{
    m_shouldSyncChildren = true;
    if (client())
        client()->notifyFlushRequired(this);
}

#if ENABLE(CSS_FILTERS)
void CoordinatedGraphicsLayer::didChangeFilters()
{
    m_shouldSyncFilters = true;
    if (client())
        client()->notifyFlushRequired(this);
}
#endif

void CoordinatedGraphicsLayer::didChangeImageBacking()
{
    m_shouldSyncImageBacking = true;
    if (client())
        client()->notifyFlushRequired(this);
}

void CoordinatedGraphicsLayer::setShouldUpdateVisibleRect()
{
    m_shouldUpdateVisibleRect = true;
    for (size_t i = 0; i < children().size(); ++i)
        toCoordinatedGraphicsLayer(children()[i])->setShouldUpdateVisibleRect();
    if (replicaLayer())
        toCoordinatedGraphicsLayer(replicaLayer())->setShouldUpdateVisibleRect();
}

void CoordinatedGraphicsLayer::didChangeGeometry()
{
    didChangeLayerState();
    setShouldUpdateVisibleRect();
}

CoordinatedGraphicsLayer::CoordinatedGraphicsLayer(GraphicsLayerClient* client)
    : GraphicsLayer(client)
#ifndef NDEBUG
    , m_isPurging(false)
#endif
    , m_shouldUpdateVisibleRect(true)
    , m_shouldSyncLayerState(true)
    , m_shouldSyncChildren(true)
    , m_shouldSyncFilters(true)
    , m_shouldSyncImageBacking(true)
    , m_shouldSyncAnimations(true)
    , m_fixedToViewport(false)
    , m_movingVisibleRect(false)
    , m_pendingContentsScaleAdjustment(false)
    , m_pendingVisibleRectAdjustment(false)
#if USE(GRAPHICS_SURFACE)
    , m_isValidCanvas(false)
    , m_pendingCanvasOperation(None)
#endif
    , m_coordinator(0)
    , m_compositedNativeImagePtr(0)
    , m_canvasPlatformLayer(0)
    , m_animationStartedTimer(this, &CoordinatedGraphicsLayer::animationStartedTimerFired)
    , m_scrollableArea(0)
{
    static CoordinatedLayerID nextLayerID = 1;
    m_id = nextLayerID++;
}

CoordinatedGraphicsLayer::~CoordinatedGraphicsLayer()
{
    if (m_coordinator) {
        purgeBackingStores();
        m_coordinator->detachLayer(this);
    }
    ASSERT(!m_coordinatedImageBacking);
    ASSERT(!m_mainBackingStore);
    willBeDestroyed();
}

bool CoordinatedGraphicsLayer::setChildren(const Vector<GraphicsLayer*>& children)
{
    bool ok = GraphicsLayer::setChildren(children);
    if (!ok)
        return false;
    didChangeChildren();
    return true;
}

void CoordinatedGraphicsLayer::addChild(GraphicsLayer* layer)
{
    GraphicsLayer::addChild(layer);
    didChangeChildren();
}

void CoordinatedGraphicsLayer::addChildAtIndex(GraphicsLayer* layer, int index)
{
    GraphicsLayer::addChildAtIndex(layer, index);
    didChangeChildren();
}

void CoordinatedGraphicsLayer::addChildAbove(GraphicsLayer* layer, GraphicsLayer* sibling)
{
    GraphicsLayer::addChildAbove(layer, sibling);
    didChangeChildren();
}

void CoordinatedGraphicsLayer::addChildBelow(GraphicsLayer* layer, GraphicsLayer* sibling)
{
    GraphicsLayer::addChildBelow(layer, sibling);
    didChangeChildren();
}

bool CoordinatedGraphicsLayer::replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild)
{
    bool ok = GraphicsLayer::replaceChild(oldChild, newChild);
    if (!ok)
        return false;
    didChangeChildren();
    return true;
}

void CoordinatedGraphicsLayer::removeFromParent()
{
    if (CoordinatedGraphicsLayer* parentLayer = toCoordinatedGraphicsLayer(parent()))
        parentLayer->didChangeChildren();
    GraphicsLayer::removeFromParent();
}

void CoordinatedGraphicsLayer::setPosition(const FloatPoint& p)
{
    if (position() == p)
        return;

    GraphicsLayer::setPosition(p);
    m_layerState.positionChanged = true;
    didChangeGeometry();
}

void CoordinatedGraphicsLayer::setAnchorPoint(const FloatPoint3D& p)
{
    if (anchorPoint() == p)
        return;

    GraphicsLayer::setAnchorPoint(p);
    m_layerState.anchorPointChanged = true;
    didChangeGeometry();
}

void CoordinatedGraphicsLayer::setSize(const FloatSize& size)
{
    if (this->size() == size)
        return;

    GraphicsLayer::setSize(size);
    m_layerState.sizeChanged = true;

    if (maskLayer())
        maskLayer()->setSize(size);
    didChangeGeometry();
}

void CoordinatedGraphicsLayer::setTransform(const TransformationMatrix& t)
{
    if (transform() == t)
        return;

    GraphicsLayer::setTransform(t);
    m_layerState.transformChanged = true;

    didChangeGeometry();
}

void CoordinatedGraphicsLayer::setChildrenTransform(const TransformationMatrix& t)
{
    if (childrenTransform() == t)
        return;

    GraphicsLayer::setChildrenTransform(t);
    m_layerState.childrenTransformChanged = true;

    didChangeGeometry();
}

void CoordinatedGraphicsLayer::setPreserves3D(bool b)
{
    if (preserves3D() == b)
        return;

    GraphicsLayer::setPreserves3D(b);
    m_layerState.preserves3D = b;
    m_layerState.flagsChanged = true;

    didChangeGeometry();
}

void CoordinatedGraphicsLayer::setMasksToBounds(bool b)
{
    if (masksToBounds() == b)
        return;
    GraphicsLayer::setMasksToBounds(b);
    m_layerState.masksToBounds = b;
    m_layerState.flagsChanged = true;

    didChangeGeometry();
}

void CoordinatedGraphicsLayer::setDrawsContent(bool b)
{
    if (drawsContent() == b)
        return;
    GraphicsLayer::setDrawsContent(b);
    m_layerState.drawsContent = b;
    m_layerState.flagsChanged = true;

    didChangeLayerState();
}

void CoordinatedGraphicsLayer::setContentsVisible(bool b)
{
    if (contentsAreVisible() == b)
        return;
    GraphicsLayer::setContentsVisible(b);
    m_layerState.contentsVisible = b;
    m_layerState.flagsChanged = true;

    if (maskLayer())
        maskLayer()->setContentsVisible(b);

    didChangeLayerState();
}

void CoordinatedGraphicsLayer::setContentsOpaque(bool b)
{
    if (contentsOpaque() == b)
        return;
    if (m_mainBackingStore)
        m_mainBackingStore->setSupportsAlpha(!b);
    GraphicsLayer::setContentsOpaque(b);
    m_layerState.contentsOpaque = b;
    m_layerState.flagsChanged = true;

    didChangeLayerState();
}

void CoordinatedGraphicsLayer::setBackfaceVisibility(bool b)
{
    if (backfaceVisibility() == b)
        return;

    GraphicsLayer::setBackfaceVisibility(b);
    m_layerState.backfaceVisible = b;
    m_layerState.flagsChanged = true;

    didChangeLayerState();
}

void CoordinatedGraphicsLayer::setOpacity(float opacity)
{
    if (this->opacity() == opacity)
        return;

    GraphicsLayer::setOpacity(opacity);
    m_layerState.opacity = opacity;
    m_layerState.opacityChanged = true;

    didChangeLayerState();
}

void CoordinatedGraphicsLayer::setContentsRect(const IntRect& r)
{
    if (contentsRect() == r)
        return;

    GraphicsLayer::setContentsRect(r);
    m_layerState.contentsRect = r;
    m_layerState.contentsRectChanged = true;

    didChangeLayerState();
}

void CoordinatedGraphicsLayer::setContentsTileSize(const IntSize& s)
{
    if (contentsTileSize() == s)
        return;

    GraphicsLayer::setContentsTileSize(s);
    m_layerState.contentsTileSize = s;
    m_layerState.contentsTilingChanged = true;
    didChangeLayerState();
}

void CoordinatedGraphicsLayer::setContentsTilePhase(const IntPoint& p)
{
    if (contentsTilePhase() == p)
        return;

    GraphicsLayer::setContentsTilePhase(p);
    m_layerState.contentsTilePhase = p;
    m_layerState.contentsTilingChanged = true;
    didChangeLayerState();
}

static bool s_shouldSupportContentsTiling = false;

void CoordinatedGraphicsLayer::setShouldSupportContentsTiling(bool s)
{
    s_shouldSupportContentsTiling = s;
}

bool GraphicsLayer::supportsContentsTiling()
{
    return s_shouldSupportContentsTiling;
}

void CoordinatedGraphicsLayer::setContentsNeedsDisplay()
{
#if USE(GRAPHICS_SURFACE)
    if (m_canvasPlatformLayer)
        m_pendingCanvasOperation |= SyncCanvas;
#endif

    if (client())
        client()->notifyFlushRequired(this);

    addRepaintRect(contentsRect());
}

void CoordinatedGraphicsLayer::setContentsToCanvas(PlatformLayer* platformLayer)
{
#if USE(GRAPHICS_SURFACE)
    if (m_canvasPlatformLayer) {
        ASSERT(m_canvasToken.isValid());
        if (!platformLayer) {
            m_pendingCanvasOperation |= DestroyCanvas;
            m_pendingCanvasOperation &= ~CreateCanvas;
        }  else if ((m_canvasSize != platformLayer->platformLayerSize()) || (m_canvasToken != platformLayer->graphicsSurfaceToken())) {
            // m_canvasToken can be different to platformLayer->graphicsSurfaceToken(), even if m_canvasPlatformLayer equals platformLayer.
            m_pendingCanvasOperation |= RecreateCanvas;
        }
    } else {
        if (platformLayer)
            m_pendingCanvasOperation |= CreateAndSyncCanvas;
    }

    m_canvasPlatformLayer = platformLayer;
    // m_canvasToken is updated only here. In detail, when GraphicsContext3D is changed or reshaped, m_canvasToken is changed and setContentsToCanvas() is always called.
    m_canvasSize = m_canvasPlatformLayer ? m_canvasPlatformLayer->platformLayerSize() : IntSize();
    m_canvasToken = m_canvasPlatformLayer ? m_canvasPlatformLayer->graphicsSurfaceToken() : GraphicsSurfaceToken();
    ASSERT(!(!m_canvasToken.isValid() && m_canvasPlatformLayer));

    if (client())
        client()->notifyFlushRequired(this);
#else
    UNUSED_PARAM(platformLayer);
#endif
}

#if ENABLE(CSS_FILTERS)
bool CoordinatedGraphicsLayer::setFilters(const FilterOperations& newFilters)
{
    if (filters() == newFilters)
        return true;

    if (!GraphicsLayer::setFilters(newFilters))
        return false;

    didChangeFilters();
    return true;
}
#endif

void CoordinatedGraphicsLayer::setContentsToSolidColor(const Color& color)
{
    if (m_layerState.solidColor == color)
        return;

    m_layerState.solidColor = color;
    m_layerState.solidColorChanged = true;

    didChangeLayerState();
}

void CoordinatedGraphicsLayer::setShowDebugBorder(bool show)
{
    if (isShowingDebugBorder() == show)
        return;

    GraphicsLayer::setShowDebugBorder(show);
    m_layerState.showDebugBorders = true;
    m_layerState.flagsChanged = true;

    didChangeLayerState();
}

void CoordinatedGraphicsLayer::setShowRepaintCounter(bool show)
{
    if (isShowingRepaintCounter() == show)
        return;

    GraphicsLayer::setShowRepaintCounter(show);
    m_layerState.showRepaintCounter = true;
    m_layerState.flagsChanged = true;

    didChangeLayerState();
}

void CoordinatedGraphicsLayer::setContentsToImage(Image* image)
{
    NativeImagePtr newNativeImagePtr = image ? image->nativeImageForCurrentFrame() : 0;
    if (newNativeImagePtr) {
        // This code makes the assumption that pointer equality on a NativeImagePtr is a valid way to tell if the image is changed.
        // This assumption is true in Qt, GTK and EFL.
        if (newNativeImagePtr == m_compositedNativeImagePtr)
            return;

        m_compositedImage = image;
        m_compositedNativeImagePtr = newNativeImagePtr;
    } else {
        m_compositedImage = 0;
        m_compositedNativeImagePtr = 0;
    }

    GraphicsLayer::setContentsToImage(image);
    didChangeImageBacking();
}

void CoordinatedGraphicsLayer::setMaskLayer(GraphicsLayer* layer)
{
    if (layer == maskLayer())
        return;

    GraphicsLayer::setMaskLayer(layer);

    if (!layer)
        return;

    layer->setSize(size());
    layer->setContentsVisible(contentsAreVisible());
    CoordinatedGraphicsLayer* coordinatedLayer = toCoordinatedGraphicsLayer(layer);
    coordinatedLayer->didChangeLayerState();

    m_layerState.mask = coordinatedLayer->id();
    m_layerState.maskChanged = true;

    didChangeLayerState();
}

bool CoordinatedGraphicsLayer::shouldDirectlyCompositeImage(Image* image) const
{
    if (!image || !image->isBitmapImage())
        return false;

    enum { MaxDimenstionForDirectCompositing = 2000 };
    if (image->width() > MaxDimenstionForDirectCompositing || image->height() > MaxDimenstionForDirectCompositing)
        return false;

    return true;
}

void CoordinatedGraphicsLayer::setReplicatedByLayer(GraphicsLayer* layer)
{
    if (layer == replicaLayer())
        return;

    GraphicsLayer::setReplicatedByLayer(layer);
    m_layerState.replica = toCoordinatedLayerID(layer);
    m_layerState.replicaChanged = true;
    didChangeLayerState();
}

void CoordinatedGraphicsLayer::setNeedsDisplay()
{
    setNeedsDisplayInRect(FloatRect(FloatPoint(), size()));
}

void CoordinatedGraphicsLayer::setNeedsDisplayInRect(const FloatRect& rect)
{
    if (m_mainBackingStore)
        m_mainBackingStore->invalidate(IntRect(rect));

    didChangeLayerState();

    addRepaintRect(rect);
}

CoordinatedLayerID CoordinatedGraphicsLayer::id() const
{
    return m_id;
}

void CoordinatedGraphicsLayer::setScrollableArea(ScrollableArea* scrollableArea)
{
    bool oldScrollable = isScrollable();
    m_scrollableArea = scrollableArea;
    if (oldScrollable == isScrollable())
        return;

    m_layerState.isScrollable = isScrollable();
    m_layerState.flagsChanged = true;
    didChangeLayerState();
}

void CoordinatedGraphicsLayer::commitScrollOffset(const IntSize& offset)
{
    if (!isScrollable() || offset.isZero())
        return;

    m_scrollableArea->notifyScrollPositionChanged(m_scrollableArea->scrollPosition() + offset);
    m_layerState.committedScrollOffset += offset;
    m_layerState.committedScrollOffsetChanged = true;
    didChangeLayerState();
}

void CoordinatedGraphicsLayer::setFixedToViewport(bool isFixed)
{
    if (m_fixedToViewport == isFixed)
        return;

    m_fixedToViewport = isFixed;
    m_layerState.fixedToViewport = isFixed;
    m_layerState.flagsChanged = true;

    didChangeLayerState();
}

void CoordinatedGraphicsLayer::flushCompositingState(const FloatRect& rect)
{
    if (!m_coordinator->isFlushingLayerChanges()) {
        if (client())
            client()->notifyFlushRequired(this);
        return;
    }

    if (CoordinatedGraphicsLayer* mask = toCoordinatedGraphicsLayer(maskLayer()))
        mask->flushCompositingStateForThisLayerOnly();

    if (CoordinatedGraphicsLayer* replica = toCoordinatedGraphicsLayer(replicaLayer()))
        replica->flushCompositingStateForThisLayerOnly();

    flushCompositingStateForThisLayerOnly();

    for (size_t i = 0; i < children().size(); ++i)
        children()[i]->flushCompositingState(rect);
}

CoordinatedGraphicsLayer* toCoordinatedGraphicsLayer(GraphicsLayer* layer)
{
    return static_cast<CoordinatedGraphicsLayer*>(layer);
}

void CoordinatedGraphicsLayer::syncChildren()
{
    if (!m_shouldSyncChildren)
        return;
    m_shouldSyncChildren = false;
    m_layerState.childrenChanged = true;
    m_layerState.children.clear();
    for (size_t i = 0; i < children().size(); ++i)
        m_layerState.children.append(toCoordinatedLayerID(children()[i]));
}

#if ENABLE(CSS_FILTERS)
void CoordinatedGraphicsLayer::syncFilters()
{
    if (!m_shouldSyncFilters)
        return;
    m_shouldSyncFilters = false;

    m_layerState.filters = GraphicsLayer::filters();
    m_layerState.filtersChanged = true;
}
#endif

void CoordinatedGraphicsLayer::syncImageBacking()
{
    if (!m_shouldSyncImageBacking)
        return;
    m_shouldSyncImageBacking = false;

    if (m_compositedNativeImagePtr) {
        ASSERT(!shouldHaveBackingStore());
        ASSERT(m_compositedImage);

        bool imageInstanceReplaced = m_coordinatedImageBacking && (m_coordinatedImageBacking->id() != CoordinatedImageBacking::getCoordinatedImageBackingID(m_compositedImage.get()));
        if (imageInstanceReplaced)
            releaseImageBackingIfNeeded();

        if (!m_coordinatedImageBacking) {
            m_coordinatedImageBacking = m_coordinator->createImageBackingIfNeeded(m_compositedImage.get());
            m_coordinatedImageBacking->addHost(this);
            m_layerState.imageID = m_coordinatedImageBacking->id();
        }

        m_coordinatedImageBacking->markDirty();
        m_layerState.imageChanged = true;
    } else
        releaseImageBackingIfNeeded();

    // syncImageBacking() changed m_layerState.imageID.
    didChangeLayerState();
}

void CoordinatedGraphicsLayer::syncLayerState()
{
    if (!m_shouldSyncLayerState)
        return;
    m_shouldSyncLayerState = false;

    m_layerState.childrenTransform = childrenTransform();
    m_layerState.contentsRect = contentsRect();
    m_layerState.mask = toCoordinatedLayerID(maskLayer());
    m_layerState.opacity = opacity();
    m_layerState.replica = toCoordinatedLayerID(replicaLayer());
    m_layerState.transform = transform();

    m_layerState.anchorPoint = m_adjustedAnchorPoint;
    m_layerState.pos = m_adjustedPosition;
    m_layerState.size = m_adjustedSize;

    if (m_layerState.flagsChanged) {
        m_layerState.drawsContent = drawsContent();
        m_layerState.contentsVisible = contentsAreVisible();
        m_layerState.backfaceVisible = backfaceVisibility();
        m_layerState.masksToBounds = masksToBounds();
        m_layerState.preserves3D = preserves3D();
        m_layerState.fixedToViewport = fixedToViewport();
        m_layerState.showDebugBorders = isShowingDebugBorder();
        m_layerState.showRepaintCounter = isShowingRepaintCounter();
        m_layerState.isScrollable = isScrollable();
    }

    if (m_layerState.showDebugBorders)
        updateDebugIndicators();
}

void CoordinatedGraphicsLayer::setDebugBorder(const Color& color, float width)
{
    ASSERT(m_layerState.showDebugBorders);
    if (m_layerState.debugBorderColor != color) {
        m_layerState.debugBorderColor = color;
        m_layerState.debugBorderColorChanged = true;
    }

    if (m_layerState.debugBorderWidth != width) {
        m_layerState.debugBorderWidth = width;
        m_layerState.debugBorderWidthChanged = true;
    }
}

void CoordinatedGraphicsLayer::syncAnimations()
{
    if (!m_shouldSyncAnimations)
        return;

    m_shouldSyncAnimations = false;
    m_layerState.animations = m_animations.getActiveAnimations();
    m_layerState.animationsChanged = true;
}

#if USE(GRAPHICS_SURFACE)
void CoordinatedGraphicsLayer::syncCanvas()
{
    destroyCanvasIfNeeded();
    createCanvasIfNeeded();

    if (!(m_pendingCanvasOperation & SyncCanvas))
        return;

    m_pendingCanvasOperation &= ~SyncCanvas;

    if (!m_isValidCanvas)
        return;

    ASSERT(m_canvasPlatformLayer);
    m_layerState.canvasFrontBuffer = m_canvasPlatformLayer->copyToGraphicsSurface();
    m_layerState.canvasShouldSwapBuffers = true;
}

void CoordinatedGraphicsLayer::destroyCanvasIfNeeded()
{
    if (!(m_pendingCanvasOperation & DestroyCanvas))
        return;

    if (m_isValidCanvas) {
        m_isValidCanvas = false;
        m_layerState.canvasToken = GraphicsSurfaceToken();
        m_layerState.canvasChanged = true;
    }

    m_pendingCanvasOperation &= ~DestroyCanvas;
}

void CoordinatedGraphicsLayer::createCanvasIfNeeded()
{
    if (!(m_pendingCanvasOperation & CreateCanvas))
        return;

    ASSERT(m_canvasPlatformLayer);
    if (!m_isValidCanvas) {
        m_layerState.canvasSize = m_canvasPlatformLayer->platformLayerSize();
        m_layerState.canvasToken = m_canvasPlatformLayer->graphicsSurfaceToken();
        m_layerState.canvasSurfaceFlags = m_canvasPlatformLayer->graphicsSurfaceFlags();
        m_layerState.canvasChanged = true;
        m_isValidCanvas = true;
    }

    m_pendingCanvasOperation &= ~CreateCanvas;
}
#endif

void CoordinatedGraphicsLayer::flushCompositingStateForThisLayerOnly()
{
    ASSERT(m_coordinator->isFlushingLayerChanges());

    // When we have a transform animation, we need to update visible rect every frame to adjust the visible rect of a backing store.
    bool hasActiveTransformAnimation = selfOrAncestorHasActiveTransformAnimation();
    if (hasActiveTransformAnimation)
        m_movingVisibleRect = true;

    // Sets the values.
    computePixelAlignment(m_adjustedPosition, m_adjustedSize, m_adjustedAnchorPoint, m_pixelAlignmentOffset);

    syncImageBacking();
    syncLayerState();
    syncAnimations();
    computeTransformedVisibleRect();
    syncChildren();
#if ENABLE(CSS_FILTERS)
    syncFilters();
#endif
#if USE(GRAPHICS_SURFACE)
    syncCanvas();
#endif

    // Only unset m_movingVisibleRect after we have updated the visible rect after the animation stopped.
    if (!hasActiveTransformAnimation)
        m_movingVisibleRect = false;
}

void CoordinatedGraphicsLayer::syncPendingStateChangesIncludingSubLayers()
{
    if (m_layerState.hasPendingChanges()) {
        m_coordinator->syncLayerState(m_id, m_layerState);
        resetLayerState();
    }

    for (size_t i = 0; i < children().size(); ++i)
        toCoordinatedGraphicsLayer(children()[i])->syncPendingStateChangesIncludingSubLayers();
}

void CoordinatedGraphicsLayer::resetLayerState()
{
    m_layerState.changeMask = 0;
    m_layerState.tilesToCreate.clear();
    m_layerState.tilesToRemove.clear();
    m_layerState.tilesToUpdate.clear();
    m_layerState.committedScrollOffset = IntSize();
}

bool CoordinatedGraphicsLayer::imageBackingVisible()
{
    ASSERT(m_coordinatedImageBacking);
    return tiledBackingStoreVisibleRect().intersects(contentsRect());
}

void CoordinatedGraphicsLayer::releaseImageBackingIfNeeded()
{
    if (!m_coordinatedImageBacking)
        return;

    ASSERT(m_coordinator);
    m_coordinatedImageBacking->removeHost(this);
    m_coordinatedImageBacking.clear();
    m_layerState.imageID = InvalidCoordinatedImageBackingID;
    m_layerState.imageChanged = true;
}

void CoordinatedGraphicsLayer::tiledBackingStorePaintBegin()
{
}

CoordinatedGraphicsLayer* CoordinatedGraphicsLayer::findFirstDescendantWithContentsRecursively()
{
    if (shouldHaveBackingStore())
        return this;

    for (size_t i = 0; i < children().size(); ++i) {
        CoordinatedGraphicsLayer* layer = toCoordinatedGraphicsLayer(children()[i])->findFirstDescendantWithContentsRecursively();
        if (layer)
            return layer;
    }

    return 0;
}

void CoordinatedGraphicsLayer::setVisibleContentRectTrajectoryVector(const FloatPoint& trajectoryVector)
{
    if (!m_mainBackingStore)
        return;

    m_mainBackingStore->setTrajectoryVector(trajectoryVector);
    setNeedsVisibleRectAdjustment();
}

void CoordinatedGraphicsLayer::deviceOrPageScaleFactorChanged()
{
    if (shouldHaveBackingStore())
        m_pendingContentsScaleAdjustment = true;
}

float CoordinatedGraphicsLayer::effectiveContentsScale()
{
    return selfOrAncestorHaveNonAffineTransforms() ? 1 : deviceScaleFactor() * pageScaleFactor();
}

void CoordinatedGraphicsLayer::adjustContentsScale()
{
    ASSERT(shouldHaveBackingStore());
    if (!m_mainBackingStore || m_mainBackingStore->contentsScale() == effectiveContentsScale())
        return;

    // Between creating the new backing store and painting the content,
    // we do not want to drop the previous one as that might result in
    // briefly seeing flickering as the old tiles may be dropped before
    // something replaces them.
    m_previousBackingStore = m_mainBackingStore.release();

    // No reason to save the previous backing store for non-visible areas.
    m_previousBackingStore->removeAllNonVisibleTiles();
}

void CoordinatedGraphicsLayer::createBackingStore()
{
    m_mainBackingStore = adoptPtr(new TiledBackingStore(this, CoordinatedTileBackend::create(this)));
    m_mainBackingStore->setSupportsAlpha(!contentsOpaque());
    m_mainBackingStore->setContentsScale(effectiveContentsScale());
}

void CoordinatedGraphicsLayer::tiledBackingStorePaint(GraphicsContext* context, const IntRect& rect)
{
    if (rect.isEmpty())
        return;
    paintGraphicsLayerContents(*context, rect);
}

void CoordinatedGraphicsLayer::tiledBackingStorePaintEnd(const Vector<IntRect>& updatedRects)
{
    if (!isShowingRepaintCounter() || updatedRects.isEmpty())
        return;

    m_layerState.repaintCount = incrementRepaintCount();
    m_layerState.repaintCountChanged = true;
}

void CoordinatedGraphicsLayer::tiledBackingStoreHasPendingTileCreation()
{
    setNeedsVisibleRectAdjustment();
    if (client())
        client()->notifyFlushRequired(this);
}

IntRect CoordinatedGraphicsLayer::tiledBackingStoreContentsRect()
{
    return IntRect(0, 0, size().width(), size().height());
}

static void clampToContentsRectIfRectIsInfinite(FloatRect& rect, const IntRect& contentsRect)
{
    if (rect.width() >= LayoutUnit::nearlyMax() || rect.width() <= LayoutUnit::nearlyMin()) {
        rect.setX(contentsRect.x());
        rect.setWidth(contentsRect.width());
    }

    if (rect.height() >= LayoutUnit::nearlyMax() || rect.height() <= LayoutUnit::nearlyMin()) {
        rect.setY(contentsRect.y());
        rect.setHeight(contentsRect.height());
    }
}

IntRect CoordinatedGraphicsLayer::tiledBackingStoreVisibleRect()
{
    // Non-invertible layers are not visible.
    if (!m_layerTransform.combined().isInvertible())
        return IntRect();

    // Return a projection of the visible rect (surface coordinates) onto the layer's plane (layer coordinates).
    // The resulting quad might be squewed and the visible rect is the bounding box of this quad,
    // so it might spread further than the real visible area (and then even more amplified by the cover rect multiplier).
    ASSERT(m_cachedInverseTransform == m_layerTransform.combined().inverse());
    FloatRect rect = m_cachedInverseTransform.clampedBoundsOfProjectedQuad(FloatQuad(m_coordinator->visibleContentsRect()));
    clampToContentsRectIfRectIsInfinite(rect, tiledBackingStoreContentsRect());
    return enclosingIntRect(rect);
}

Color CoordinatedGraphicsLayer::tiledBackingStoreBackgroundColor() const
{
    return contentsOpaque() ? Color::white : Color::transparent;
}

bool CoordinatedGraphicsLayer::paintToSurface(const IntSize& size, uint32_t& atlas, IntPoint& offset, CoordinatedSurface::Client* client)
{
    ASSERT(m_coordinator);
    ASSERT(m_coordinator->isFlushingLayerChanges());
    return m_coordinator->paintToSurface(size, contentsOpaque() ? CoordinatedSurface::NoFlags : CoordinatedSurface::SupportsAlpha, atlas, offset, client);
}

void CoordinatedGraphicsLayer::createTile(uint32_t tileID, const SurfaceUpdateInfo& updateInfo, const IntRect& tileRect)
{
    ASSERT(m_coordinator);
    ASSERT(m_coordinator->isFlushingLayerChanges());

    TileCreationInfo creationInfo;
    creationInfo.tileID = tileID;
    creationInfo.scale = updateInfo.scaleFactor;

    m_layerState.tilesToCreate.append(creationInfo);
    updateTile(tileID, updateInfo, tileRect);
}

void CoordinatedGraphicsLayer::updateTile(uint32_t tileID, const SurfaceUpdateInfo& updateInfo, const IntRect& tileRect)
{
    ASSERT(m_coordinator);
    ASSERT(m_coordinator->isFlushingLayerChanges());

    TileUpdateInfo tileUpdateInfo;
    tileUpdateInfo.tileID = tileID;
    tileUpdateInfo.tileRect = tileRect;
    tileUpdateInfo.updateInfo = updateInfo;
    m_layerState.tilesToUpdate.append(tileUpdateInfo);
}

void CoordinatedGraphicsLayer::removeTile(uint32_t tileID)
{
    ASSERT(m_coordinator);
    ASSERT(m_coordinator->isFlushingLayerChanges() || m_isPurging);
    m_layerState.tilesToRemove.append(tileID);
}

void CoordinatedGraphicsLayer::updateContentBuffersIncludingSubLayers()
{
    if (CoordinatedGraphicsLayer* mask = toCoordinatedGraphicsLayer(maskLayer()))
        mask->updateContentBuffers();

    if (CoordinatedGraphicsLayer* replica = toCoordinatedGraphicsLayer(replicaLayer()))
        replica->updateContentBuffers();

    updateContentBuffers();

    for (size_t i = 0; i < children().size(); ++i)
        toCoordinatedGraphicsLayer(children()[i])->updateContentBuffersIncludingSubLayers();
}

void CoordinatedGraphicsLayer::updateContentBuffers()
{
    if (!shouldHaveBackingStore()) {
        m_mainBackingStore.clear();
        m_previousBackingStore.clear();
        return;
    }

    if (m_pendingContentsScaleAdjustment) {
        adjustContentsScale();
        m_pendingContentsScaleAdjustment = false;
    }

    // This is the only place we (re)create the main tiled backing store, once we
    // have a remote client and we are ready to send our data to the UI process.
    if (!m_mainBackingStore)
        createBackingStore();

    if (m_pendingVisibleRectAdjustment) {
        m_pendingVisibleRectAdjustment = false;
        m_mainBackingStore->coverWithTilesIfNeeded();
    }

    m_mainBackingStore->updateTileBuffers();

    // The previous backing store is kept around to avoid flickering between
    // removing the existing tiles and painting the new ones. The first time
    // the visibleRect is full painted we remove the previous backing store.
    if (m_mainBackingStore->visibleAreaIsCovered())
        m_previousBackingStore.clear();
}

void CoordinatedGraphicsLayer::purgeBackingStores()
{
#ifndef NDEBUG
    TemporaryChange<bool> updateModeProtector(m_isPurging, true);
#endif
    m_mainBackingStore.clear();
    m_previousBackingStore.clear();

    releaseImageBackingIfNeeded();

    didChangeLayerState();
}

void CoordinatedGraphicsLayer::setCoordinator(CoordinatedGraphicsLayerClient* coordinator)
{
    m_coordinator = coordinator;
}

void CoordinatedGraphicsLayer::setNeedsVisibleRectAdjustment()
{
    if (shouldHaveBackingStore())
        m_pendingVisibleRectAdjustment = true;
}

bool CoordinatedGraphicsLayer::hasPendingVisibleChanges()
{
    if (opacity() < 0.01 && !m_animations.hasActiveAnimationsOfType(AnimatedPropertyOpacity))
        return false;

    for (size_t i = 0; i < children().size(); ++i) {
        if (toCoordinatedGraphicsLayer(children()[i])->hasPendingVisibleChanges())
            return true;
    }

    bool shouldSyncCanvas = false;
#if USE(GRAPHICS_SURFACE)
    shouldSyncCanvas = m_pendingCanvasOperation & SyncCanvas;
#endif

    if (!m_shouldSyncLayerState && !m_shouldSyncChildren && !m_shouldSyncFilters && !m_shouldSyncImageBacking && !m_shouldSyncAnimations && !shouldSyncCanvas)
        return false;

    return tiledBackingStoreVisibleRect().intersects(tiledBackingStoreContentsRect());
}

static inline bool isIntegral(float value)
{
    return static_cast<int>(value) == value;
}

FloatPoint CoordinatedGraphicsLayer::computePositionRelativeToBase()
{
    FloatPoint offset;
    for (const GraphicsLayer* currLayer = this; currLayer; currLayer = currLayer->parent())
        offset += currLayer->position();

    return offset;
}

void CoordinatedGraphicsLayer::computePixelAlignment(FloatPoint& position, FloatSize& size, FloatPoint3D& anchorPoint, FloatSize& alignmentOffset)
{
    if (isIntegral(effectiveContentsScale())) {
        position = m_position;
        size = m_size;
        anchorPoint = m_anchorPoint;
        alignmentOffset = FloatSize();
        return;
    }

    FloatPoint positionRelativeToBase = computePositionRelativeToBase();

    FloatRect baseRelativeBounds(positionRelativeToBase, m_size);
    FloatRect scaledBounds = baseRelativeBounds;

    // Scale by the effective scale factor to compute the screen-relative bounds.
    scaledBounds.scale(effectiveContentsScale());

    // Round to integer boundaries.
    // NOTE: When using enclosingIntRect (as mac) it will have different sizes depending on position.
    FloatRect alignedBounds = roundedIntRect(scaledBounds);

    // Convert back to layer coordinates.
    alignedBounds.scale(1 / effectiveContentsScale());

    // Convert back to layer coordinates.
    alignmentOffset = baseRelativeBounds.location() - alignedBounds.location();

    position = m_position - alignmentOffset;
    size = alignedBounds.size();

    // Now we have to compute a new anchor point which compensates for rounding.
    float anchorPointX = m_anchorPoint.x();
    float anchorPointY = m_anchorPoint.y();

    if (alignedBounds.width())
        anchorPointX = (baseRelativeBounds.width() * anchorPointX + alignmentOffset.width()) / alignedBounds.width();

    if (alignedBounds.height())
        anchorPointY = (baseRelativeBounds.height() * anchorPointY + alignmentOffset.height()) / alignedBounds.height();

    anchorPoint = FloatPoint3D(anchorPointX, anchorPointY, m_anchorPoint.z() * effectiveContentsScale());
}

void CoordinatedGraphicsLayer::computeTransformedVisibleRect()
{
    if (!m_shouldUpdateVisibleRect && !m_movingVisibleRect)
        return;

    m_shouldUpdateVisibleRect = false;
    TransformationMatrix currentTransform = transform();
    if (m_movingVisibleRect)
        client()->getCurrentTransform(this, currentTransform);
    m_layerTransform.setLocalTransform(currentTransform);

    m_layerTransform.setAnchorPoint(m_adjustedAnchorPoint);
    m_layerTransform.setPosition(m_adjustedPosition);
    m_layerTransform.setSize(m_adjustedSize);

    m_layerTransform.setFlattening(!preserves3D());
    m_layerTransform.setChildrenTransform(childrenTransform());
    m_layerTransform.combineTransforms(parent() ? toCoordinatedGraphicsLayer(parent())->m_layerTransform.combinedForChildren() : TransformationMatrix());

    m_cachedInverseTransform = m_layerTransform.combined().inverse();

    // The combined transform will be used in tiledBackingStoreVisibleRect.
    setNeedsVisibleRectAdjustment();
}

bool CoordinatedGraphicsLayer::shouldHaveBackingStore() const
{
    return drawsContent() && contentsAreVisible() && !m_size.isEmpty();
}

bool CoordinatedGraphicsLayer::selfOrAncestorHasActiveTransformAnimation() const
{
    if (m_animations.hasActiveAnimationsOfType(AnimatedPropertyWebkitTransform))
        return true;

    if (!parent())
        return false;

    return toCoordinatedGraphicsLayer(parent())->selfOrAncestorHasActiveTransformAnimation();
}

bool CoordinatedGraphicsLayer::selfOrAncestorHaveNonAffineTransforms()
{
    if (m_animations.hasActiveAnimationsOfType(AnimatedPropertyWebkitTransform))
        return true;

    if (!m_layerTransform.combined().isAffine())
        return true;

    if (!parent())
        return false;

    return toCoordinatedGraphicsLayer(parent())->selfOrAncestorHaveNonAffineTransforms();
}

bool CoordinatedGraphicsLayer::addAnimation(const KeyframeValueList& valueList, const IntSize& boxSize, const Animation* anim, const String& keyframesName, double delayAsNegativeTimeOffset)
{
    ASSERT(!keyframesName.isEmpty());

    if (!anim || anim->isEmptyOrZeroDuration() || valueList.size() < 2 || (valueList.property() != AnimatedPropertyWebkitTransform && valueList.property() != AnimatedPropertyOpacity && valueList.property() != AnimatedPropertyWebkitFilter))
        return false;

    bool listsMatch = false;
    bool ignoredHasBigRotation;

    if (valueList.property() == AnimatedPropertyWebkitTransform)
        listsMatch = validateTransformOperations(valueList, ignoredHasBigRotation) >= 0;

    m_lastAnimationStartTime = WTF::currentTime() - delayAsNegativeTimeOffset;
    m_animations.add(GraphicsLayerAnimation(keyframesName, valueList, boxSize, anim, m_lastAnimationStartTime, listsMatch));
    m_animationStartedTimer.startOneShot(0);
    didChangeAnimations();
    return true;
}

void CoordinatedGraphicsLayer::pauseAnimation(const String& animationName, double time)
{
    m_animations.pause(animationName, time);
    didChangeAnimations();
}

void CoordinatedGraphicsLayer::removeAnimation(const String& animationName)
{
    m_animations.remove(animationName);
    didChangeAnimations();
}

void CoordinatedGraphicsLayer::suspendAnimations(double time)
{
    m_animations.suspend(time);
    didChangeAnimations();
}

void CoordinatedGraphicsLayer::resumeAnimations()
{
    m_animations.resume();
    didChangeAnimations();
}

void CoordinatedGraphicsLayer::animationStartedTimerFired(Timer<CoordinatedGraphicsLayer>*)
{
    client()->notifyAnimationStarted(this, m_lastAnimationStartTime);
}
} // namespace WebCore
#endif // USE(COORDINATED_GRAPHICS)
