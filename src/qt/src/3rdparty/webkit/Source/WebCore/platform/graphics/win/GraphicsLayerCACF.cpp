/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if USE(ACCELERATED_COMPOSITING)

#include "GraphicsLayerCACF.h"

#include "FloatConversion.h"
#include "FloatRect.h"
#include "Font.h"
#include "FontSelector.h"
#include "Image.h"
#include "PlatformString.h"
#include "SystemTime.h"
#include "WebLayer.h"
#include "WebTiledLayer.h"
#include <wtf/CurrentTime.h>
#include <wtf/StringExtras.h>
#include <wtf/text/CString.h>

using namespace std;

namespace WebCore {

// The threshold width or height above which a tiled layer will be used. This should be
// large enough to avoid tiled layers for most GraphicsLayers, but less than the D3D
// texture size limit on all supported hardware.
static const int cMaxPixelDimension = 2000;

// The width and height of a single tile in a tiled layer. Should be large enough to
// avoid lots of small tiles (and therefore lots of drawing callbacks), but small enough
// to keep the overall tile cost low.
static const int cTiledLayerTileSize = 512;

static inline void copyTransform(CATransform3D& toT3D, const TransformationMatrix& t)
{
    toT3D.m11 = narrowPrecisionToFloat(t.m11());
    toT3D.m12 = narrowPrecisionToFloat(t.m12());
    toT3D.m13 = narrowPrecisionToFloat(t.m13());
    toT3D.m14 = narrowPrecisionToFloat(t.m14());
    toT3D.m21 = narrowPrecisionToFloat(t.m21());
    toT3D.m22 = narrowPrecisionToFloat(t.m22());
    toT3D.m23 = narrowPrecisionToFloat(t.m23());
    toT3D.m24 = narrowPrecisionToFloat(t.m24());
    toT3D.m31 = narrowPrecisionToFloat(t.m31());
    toT3D.m32 = narrowPrecisionToFloat(t.m32());
    toT3D.m33 = narrowPrecisionToFloat(t.m33());
    toT3D.m34 = narrowPrecisionToFloat(t.m34());
    toT3D.m41 = narrowPrecisionToFloat(t.m41());
    toT3D.m42 = narrowPrecisionToFloat(t.m42());
    toT3D.m43 = narrowPrecisionToFloat(t.m43());
    toT3D.m44 = narrowPrecisionToFloat(t.m44());
}

TransformationMatrix CAToTransform3D(const CATransform3D& fromT3D)
{
    return TransformationMatrix(
        fromT3D.m11,
        fromT3D.m12,
        fromT3D.m13,
        fromT3D.m14,
        fromT3D.m21,
        fromT3D.m22,
        fromT3D.m23,
        fromT3D.m24,
        fromT3D.m31,
        fromT3D.m32,
        fromT3D.m33,
        fromT3D.m34,
        fromT3D.m41,
        fromT3D.m42,
        fromT3D.m43,
        fromT3D.m44);
}

static void setLayerBorderColor(WKCACFLayer* layer, const Color& color)
{
    layer->setBorderColor(cachedCGColor(color, ColorSpaceDeviceRGB));
}

static void clearBorderColor(WKCACFLayer* layer)
{
    layer->setBorderColor(0);
}

static void setLayerBackgroundColor(WKCACFLayer* layer, const Color& color)
{
    layer->setBackgroundColor(cachedCGColor(color, ColorSpaceDeviceRGB));
}

static void clearLayerBackgroundColor(WKCACFLayer* layer)
{
    layer->setBackgroundColor(0);
}

PassOwnPtr<GraphicsLayer> GraphicsLayer::create(GraphicsLayerClient* client)
{
    return new GraphicsLayerCACF(client);
}

GraphicsLayerCACF::GraphicsLayerCACF(GraphicsLayerClient* client)
    : GraphicsLayer(client)
    , m_contentsLayerPurpose(NoContentsLayer)
    , m_contentsLayerHasBackgroundColor(false)
{
    m_layer = WebLayer::create(WKCACFLayer::Layer, this);
    
    updateDebugIndicators();
}

GraphicsLayerCACF::~GraphicsLayerCACF()
{
    // clean up the WK layer
    if (m_layer)
        m_layer->removeFromSuperlayer();
    
    if (m_contentsLayer)
        m_contentsLayer->removeFromSuperlayer();

    if (m_transformLayer)
        m_transformLayer->removeFromSuperlayer();
}

void GraphicsLayerCACF::setName(const String& name)
{
    String longName = String::format("CALayer(%p) GraphicsLayer(%p) ", m_layer.get(), this) + name;
    GraphicsLayer::setName(longName);
    
    m_layer->setName(longName);
}

bool GraphicsLayerCACF::setChildren(const Vector<GraphicsLayer*>& children)
{
    bool childrenChanged = GraphicsLayer::setChildren(children);
    // FIXME: GraphicsLayer::setChildren calls addChild() for each sublayer, which
    // will end up calling updateSublayerList() N times.
    if (childrenChanged)
        updateSublayerList();
    
    return childrenChanged;
}

void GraphicsLayerCACF::addChild(GraphicsLayer* childLayer)
{
    GraphicsLayer::addChild(childLayer);
    updateSublayerList();
}

void GraphicsLayerCACF::addChildAtIndex(GraphicsLayer* childLayer, int index)
{
    GraphicsLayer::addChildAtIndex(childLayer, index);
    updateSublayerList();
}

void GraphicsLayerCACF::addChildBelow(GraphicsLayer* childLayer, GraphicsLayer* sibling)
{
    GraphicsLayer::addChildBelow(childLayer, sibling);
    updateSublayerList();
}

void GraphicsLayerCACF::addChildAbove(GraphicsLayer* childLayer, GraphicsLayer *sibling)
{
    GraphicsLayer::addChildAbove(childLayer, sibling);
    updateSublayerList();
}

bool GraphicsLayerCACF::replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild)
{
    if (GraphicsLayer::replaceChild(oldChild, newChild)) {
        updateSublayerList();
        return true;
    }
    return false;
}

void GraphicsLayerCACF::removeFromParent()
{
    GraphicsLayer::removeFromParent();
    layerForSuperlayer()->removeFromSuperlayer();            
}

void GraphicsLayerCACF::setPosition(const FloatPoint& point)
{
    GraphicsLayer::setPosition(point);
    updateLayerPosition();
}

void GraphicsLayerCACF::setAnchorPoint(const FloatPoint3D& point)
{
    if (point == m_anchorPoint)
        return;

    GraphicsLayer::setAnchorPoint(point);
    updateAnchorPoint();
}

void GraphicsLayerCACF::setSize(const FloatSize& size)
{
    if (size == m_size)
        return;

    GraphicsLayer::setSize(size);
    updateLayerSize();
}

void GraphicsLayerCACF::setTransform(const TransformationMatrix& t)
{
    if (t == m_transform)
        return;

    GraphicsLayer::setTransform(t);
    updateTransform();
}

void GraphicsLayerCACF::setChildrenTransform(const TransformationMatrix& t)
{
    if (t == m_childrenTransform)
        return;

    GraphicsLayer::setChildrenTransform(t);
    updateChildrenTransform();
}

void GraphicsLayerCACF::setPreserves3D(bool preserves3D)
{
    if (preserves3D == m_preserves3D)
        return;

    GraphicsLayer::setPreserves3D(preserves3D);
    updateLayerPreserves3D();
}

void GraphicsLayerCACF::setMasksToBounds(bool masksToBounds)
{
    if (masksToBounds == m_masksToBounds)
        return;

    GraphicsLayer::setMasksToBounds(masksToBounds);
    updateMasksToBounds();
}

void GraphicsLayerCACF::setDrawsContent(bool drawsContent)
{
    if (drawsContent == m_drawsContent)
        return;

    GraphicsLayer::setDrawsContent(drawsContent);
    updateLayerDrawsContent();
}

void GraphicsLayerCACF::setBackgroundColor(const Color& color)
{
    if (m_backgroundColorSet && m_backgroundColor == color)
        return;

    GraphicsLayer::setBackgroundColor(color);

    m_contentsLayerHasBackgroundColor = true;
    updateLayerBackgroundColor();
}

void GraphicsLayerCACF::clearBackgroundColor()
{
    if (!m_backgroundColorSet)
        return;

    GraphicsLayer::clearBackgroundColor();
    clearLayerBackgroundColor(m_contentsLayer.get());
}

void GraphicsLayerCACF::setContentsOpaque(bool opaque)
{
    if (m_contentsOpaque == opaque)
        return;

    GraphicsLayer::setContentsOpaque(opaque);
    updateContentsOpaque();
}

void GraphicsLayerCACF::setBackfaceVisibility(bool visible)
{
    if (m_backfaceVisibility == visible)
        return;
    
    GraphicsLayer::setBackfaceVisibility(visible);
    updateBackfaceVisibility();
}
    
void GraphicsLayerCACF::setOpacity(float opacity)
{
    float clampedOpacity = max(min(opacity, 1.0f), 0.0f);
    
    if (m_opacity == clampedOpacity)
        return;
    
    GraphicsLayer::setOpacity(clampedOpacity);
    primaryLayer()->setOpacity(opacity);
}

void GraphicsLayerCACF::setNeedsDisplay()
{
    if (drawsContent())
        m_layer->setNeedsDisplay();
}

void GraphicsLayerCACF::setNeedsDisplayInRect(const FloatRect& rect)
{
    if (drawsContent()) {
        CGRect cgRect = rect;
        m_layer->setNeedsDisplay(&cgRect);
    }
}

void GraphicsLayerCACF::setContentsRect(const IntRect& rect)
{
    if (rect == m_contentsRect)
        return;

    GraphicsLayer::setContentsRect(rect);
    updateContentsRect();
}

void GraphicsLayerCACF::setContentsToImage(Image* image)
{
    bool childrenChanged = false;

    if (image) {
        m_pendingContentsImage = image->nativeImageForCurrentFrame();
        m_contentsLayerPurpose = ContentsLayerForImage;
        if (!m_contentsLayer)
            childrenChanged = true;
    } else {
        m_pendingContentsImage = 0;
        m_contentsLayerPurpose = NoContentsLayer;
        if (m_contentsLayer)
            childrenChanged = true;
    }

    updateContentsImage();

    // This has to happen after updateContentsImage
    if (childrenChanged)
        updateSublayerList();
}

void GraphicsLayerCACF::setContentsToMedia(PlatformLayer* mediaLayer)
{
    if (mediaLayer == m_contentsLayer)
        return;

    m_contentsLayer = mediaLayer;
    m_contentsLayerPurpose = mediaLayer ? ContentsLayerForMedia : NoContentsLayer;

    updateContentsMedia();

    // This has to happen after updateContentsMedia
    updateSublayerList();
}

PlatformLayer* GraphicsLayerCACF::hostLayerForSublayers() const
{
    return m_transformLayer ? m_transformLayer.get() : m_layer.get();
}

PlatformLayer* GraphicsLayerCACF::layerForSuperlayer() const
{
    return m_transformLayer ? m_transformLayer.get() : m_layer.get();
}

PlatformLayer* GraphicsLayerCACF::platformLayer() const
{
    return primaryLayer();
}

void GraphicsLayerCACF::setDebugBackgroundColor(const Color& color)
{
    if (color.isValid())
        setLayerBackgroundColor(m_layer.get(), color);
    else
        clearLayerBackgroundColor(m_layer.get());
}

void GraphicsLayerCACF::setDebugBorder(const Color& color, float borderWidth)
{
    if (color.isValid()) {
        setLayerBorderColor(m_layer.get(), color);
        m_layer->setBorderWidth(borderWidth);
    } else {
        clearBorderColor(m_layer.get());
        m_layer->setBorderWidth(0);
    }
}

bool GraphicsLayerCACF::requiresTiledLayer(const FloatSize& size) const
{
    if (!m_drawsContent)
        return false;

    // FIXME: catch zero-size height or width here (or earlier)?
    return size.width() > cMaxPixelDimension || size.height() > cMaxPixelDimension;
}

void GraphicsLayerCACF::swapFromOrToTiledLayer(bool useTiledLayer)
{
    if (useTiledLayer == m_usingTiledLayer)
        return;

    CGSize tileSize = CGSizeMake(cTiledLayerTileSize, cTiledLayerTileSize);

    RefPtr<WKCACFLayer> oldLayer = m_layer;
    if (useTiledLayer)
        m_layer = WebTiledLayer::create(tileSize, this);
    else
        m_layer = WebLayer::create(WKCACFLayer::Layer, this);
    
    m_usingTiledLayer = useTiledLayer;
    
    m_layer->adoptSublayers(oldLayer.get());
    if (oldLayer->superlayer())
        oldLayer->superlayer()->replaceSublayer(oldLayer.get(), m_layer.get());
    
    updateLayerPosition();
    updateLayerSize();
    updateAnchorPoint();
    updateTransform();
    updateChildrenTransform();
    updateMasksToBounds();
    updateContentsOpaque();
    updateBackfaceVisibility();
    updateLayerBackgroundColor();
    
    updateOpacityOnLayer();
    
#ifndef NDEBUG
    String name = String::format("CALayer(%p) GraphicsLayer(%p) %s", m_layer.get(), this, m_usingTiledLayer ? "[Tiled Layer] " : "") + m_name;
    m_layer->setName(name);
#endif

    // need to tell new layer to draw itself
    setNeedsDisplay();
    
    updateDebugIndicators();
}

GraphicsLayer::CompositingCoordinatesOrientation GraphicsLayerCACF::defaultContentsOrientation() const
{
    return CompositingCoordinatesTopDown;
}

void GraphicsLayerCACF::updateSublayerList()
{
    Vector<RefPtr<WKCACFLayer> > newSublayers;

    if (m_transformLayer) {
        // Add the primary layer first. Even if we have negative z-order children, the primary layer always comes behind.
        newSublayers.append(m_layer.get());
    } else if (m_contentsLayer) {
        // FIXME: add the contents layer in the correct order with negative z-order children.
        // This does not cause visible rendering issues because currently contents layers are only used
        // for replaced elements that don't have children.
        newSublayers.append(m_contentsLayer.get());
    }
    
    const Vector<GraphicsLayer*>& childLayers = children();
    size_t numChildren = childLayers.size();
    for (size_t i = 0; i < numChildren; ++i) {
        GraphicsLayerCACF* curChild = static_cast<GraphicsLayerCACF*>(childLayers[i]);
     
        WKCACFLayer* childLayer = curChild->layerForSuperlayer();
        newSublayers.append(childLayer);
    }

    for (int i = 0; i < newSublayers.size(); ++i)
        newSublayers[i]->removeFromSuperlayer();

    if (m_transformLayer) {
        m_transformLayer->setSublayers(newSublayers);

        if (m_contentsLayer) {
            // If we have a transform layer, then the contents layer is parented in the 
            // primary layer (which is itself a child of the transform layer).
            m_layer->removeAllSublayers();
            m_layer->addSublayer(m_contentsLayer);
        }
    } else
        m_layer->setSublayers(newSublayers);
}

void GraphicsLayerCACF::updateLayerPosition()
{
    // Position is offset on the layer by the layer anchor point.
    CGPoint posPoint = CGPointMake(m_position.x() + m_anchorPoint.x() * m_size.width(),
                                   m_position.y() + m_anchorPoint.y() * m_size.height());
    
    primaryLayer()->setPosition(posPoint);
}

void GraphicsLayerCACF::updateLayerSize()
{
    CGRect rect = CGRectMake(0, 0, m_size.width(), m_size.height());
    if (m_transformLayer) {
        m_transformLayer->setBounds(rect);
        // The anchor of the contents layer is always at 0.5, 0.5, so the position is center-relative.
        CGPoint centerPoint = CGPointMake(m_size.width() / 2.0f, m_size.height() / 2.0f);
        m_layer->setPosition(centerPoint);
    }
    
    bool needTiledLayer = requiresTiledLayer(m_size);
    if (needTiledLayer != m_usingTiledLayer)
        swapFromOrToTiledLayer(needTiledLayer);
    
    m_layer->setBounds(rect);
    
    // Note that we don't resize m_contentsLayer. It's up the caller to do that.

    // if we've changed the bounds, we need to recalculate the position
    // of the layer, taking anchor point into account.
    updateLayerPosition();
}

void GraphicsLayerCACF::updateAnchorPoint()
{
    primaryLayer()->setAnchorPoint(FloatPoint(m_anchorPoint.x(), m_anchorPoint.y()));
    primaryLayer()->setAnchorPointZ(m_anchorPoint.z());
    updateLayerPosition();
}

void GraphicsLayerCACF::updateTransform()
{
    CATransform3D transform;
    copyTransform(transform, m_transform);
    primaryLayer()->setTransform(transform);
}

void GraphicsLayerCACF::updateChildrenTransform()
{
    CATransform3D transform;
    copyTransform(transform, m_childrenTransform);
    primaryLayer()->setSublayerTransform(transform);
}

void GraphicsLayerCACF::updateMasksToBounds()
{
    m_layer->setMasksToBounds(m_masksToBounds);
    updateDebugIndicators();
}

void GraphicsLayerCACF::updateContentsOpaque()
{
    m_layer->setOpaque(m_contentsOpaque);
}

void GraphicsLayerCACF::updateBackfaceVisibility()
{
    m_layer->setDoubleSided(m_backfaceVisibility);
}

void GraphicsLayerCACF::updateLayerPreserves3D()
{
    if (m_preserves3D && !m_transformLayer) {
        // Create the transform layer.
        m_transformLayer = WebLayer::create(WKCACFLayer::TransformLayer, this);

#ifndef NDEBUG
        m_transformLayer->setName(String().format("Transform Layer CATransformLayer(%p) GraphicsLayer(%p)", m_transformLayer.get(), this));
#endif
        // Copy the position from this layer.
        updateLayerPosition();
        updateLayerSize();
        updateAnchorPoint();
        updateTransform();
        updateChildrenTransform();
        
        CGPoint point = CGPointMake(m_size.width() / 2.0f, m_size.height() / 2.0f);
        m_layer->setPosition(point);

        m_layer->setAnchorPoint(CGPointMake(0.5f, 0.5f));
        m_layer->setTransform(CATransform3DIdentity);
        
        // Set the old layer to opacity of 1. Further down we will set the opacity on the transform layer.
        m_layer->setOpacity(1);

        // Move this layer to be a child of the transform layer.
        if (m_layer->superlayer())
            m_layer->superlayer()->replaceSublayer(m_layer.get(), m_transformLayer.get());
        m_transformLayer->addSublayer(m_layer.get());

        updateSublayerList();
    } else if (!m_preserves3D && m_transformLayer) {
        // Relace the transformLayer in the parent with this layer.
        m_layer->removeFromSuperlayer();
        m_transformLayer->superlayer()->replaceSublayer(m_transformLayer.get(), m_layer.get());

        // Release the transform layer.
        m_transformLayer = 0;

        updateLayerPosition();
        updateLayerSize();
        updateAnchorPoint();
        updateTransform();
        updateChildrenTransform();

        updateSublayerList();
    }

    updateOpacityOnLayer();
}

void GraphicsLayerCACF::updateLayerDrawsContent()
{
    bool needTiledLayer = requiresTiledLayer(m_size);
    if (needTiledLayer != m_usingTiledLayer)
        swapFromOrToTiledLayer(needTiledLayer);

    if (m_drawsContent)
        m_layer->setNeedsDisplay();
    else
        m_layer->setContents(0);

    updateDebugIndicators();
}

void GraphicsLayerCACF::updateLayerBackgroundColor()
{
    if (!m_contentsLayer)
        return;

    // We never create the contents layer just for background color yet.
    if (m_backgroundColorSet)
        setLayerBackgroundColor(m_contentsLayer.get(), m_backgroundColor);
    else
        clearLayerBackgroundColor(m_contentsLayer.get());
}

void GraphicsLayerCACF::updateContentsImage()
{
    if (m_pendingContentsImage) {
        if (!m_contentsLayer.get()) {
            RefPtr<WKCACFLayer> imageLayer = WebLayer::create(WKCACFLayer::Layer, this);
#ifndef NDEBUG
            imageLayer->setName("Image Layer");
#endif
            setupContentsLayer(imageLayer.get());
            m_contentsLayer = imageLayer;
            // m_contentsLayer will be parented by updateSublayerList
        }

        // FIXME: maybe only do trilinear if the image is being scaled down,
        // but then what if the layer size changes?
        m_contentsLayer->setMinificationFilter(WKCACFLayer::Trilinear);
        m_contentsLayer->setContents(m_pendingContentsImage.get());
        m_pendingContentsImage = 0;
        
        updateContentsRect();
    } else {
        // No image.
        // m_contentsLayer will be removed via updateSublayerList.
        m_contentsLayer = 0;
    }
}

void GraphicsLayerCACF::updateContentsMedia()
{
    // Media layer was set as m_contentsLayer, and will get parented in updateSublayerList().
    if (m_contentsLayer) {
        setupContentsLayer(m_contentsLayer.get());
        updateContentsRect();
    }
}

void GraphicsLayerCACF::updateContentsRect()
{
    if (!m_contentsLayer)
        return;
        
    CGPoint point = CGPointMake(m_contentsRect.x(),
                                m_contentsRect.y());
    CGRect rect = CGRectMake(0.0f,
                             0.0f,
                             m_contentsRect.width(),
                             m_contentsRect.height());

    m_contentsLayer->setPosition(point);
    m_contentsLayer->setBounds(rect);
}

void GraphicsLayerCACF::setupContentsLayer(WKCACFLayer* contentsLayer)
{
    if (contentsLayer == m_contentsLayer)
        return;

    if (m_contentsLayer) {
        m_contentsLayer->removeFromSuperlayer();
        m_contentsLayer = 0;
    }
    
    if (contentsLayer) {
        m_contentsLayer = contentsLayer;

        if (defaultContentsOrientation() == CompositingCoordinatesBottomUp) {
            CATransform3D flipper = {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, -1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f};
            m_contentsLayer->setTransform(flipper);
            m_contentsLayer->setAnchorPoint(CGPointMake(0.0f, 1.0f));
        } else
            m_contentsLayer->setAnchorPoint(CGPointMake(0.0f, 0.0f));

        // Insert the content layer first. Video elements require this, because they have
        // shadow content that must display in front of the video.
        m_layer->insertSublayer(m_contentsLayer.get(), 0);

        updateContentsRect();

        if (showDebugBorders()) {
            setLayerBorderColor(m_contentsLayer.get(), Color(0, 0, 128, 180));
            m_contentsLayer->setBorderWidth(1.0f);
        }
    }
    updateDebugIndicators();
}

// This function simply mimics the operation of GraphicsLayerCA
void GraphicsLayerCACF::updateOpacityOnLayer()
{
    primaryLayer()->setOpacity(m_opacity);
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
