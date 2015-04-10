/*
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/** FIXME
 * This file borrows code heavily from platform/graphics/win/GraphicsLayerCACF.cpp
 * (and hence it includes both copyrights)
 * Ideally the common code (mostly the code that keeps track of the layer hierarchy)
 * should be kept separate and shared between platforms. It would be a well worthwhile
 * effort once the Windows implementation (binaries and headers) of CoreAnimation is
 * checked in to the WebKit repository. Until then only Apple can make this happen.
 */

#include "config.h"

#if USE(ACCELERATED_COMPOSITING)

#include "GraphicsLayerBlackBerry.h"

#include "FloatConversion.h"
#include "FloatRect.h"
#include "GraphicsLayerFactory.h"
#include "Image.h"
#include "LayerAnimation.h"
#include "LayerWebKitThread.h"
#include "NotImplemented.h"

#if DEBUG_LAYER_ANIMATION
#include <wtf/text/CString.h>
#endif

namespace WebCore {

static void setLayerBorderColor(LayerWebKitThread& layer, const Color& color)
{
    layer.setBorderColor(color);
}

static void clearBorderColor(LayerWebKitThread& layer)
{
    layer.setBorderColor(Color::transparent);
}

static void setLayerBackgroundColor(LayerWebKitThread& layer, const Color& color)
{
    layer.setBackgroundColor(color);
}

static void clearLayerBackgroundColor(LayerWebKitThread& layer)
{
    layer.setBackgroundColor(Color::transparent);
}

PassOwnPtr<GraphicsLayer> GraphicsLayer::create(GraphicsLayerFactory* factory, GraphicsLayerClient* client)
{
    if (!factory)
        return adoptPtr(new GraphicsLayerBlackBerry(client));

    return factory->createGraphicsLayer(client);
}

PassOwnPtr<GraphicsLayer> GraphicsLayer::create(GraphicsLayerClient* client)
{
    return adoptPtr(new GraphicsLayerBlackBerry(client));
}

GraphicsLayerBlackBerry::GraphicsLayerBlackBerry(GraphicsLayerClient* client)
    : GraphicsLayer(client)
    , m_suspendTime(0)
    , m_contentsLayerPurpose(NoContentsLayer)
{
    m_layer = LayerWebKitThread::create(LayerData::Layer, this);

    updateDebugIndicators();
}

GraphicsLayerBlackBerry::~GraphicsLayerBlackBerry()
{
    // Do cleanup while we can still safely call methods on the derived class.
    willBeDestroyed();
}

void GraphicsLayerBlackBerry::willBeDestroyed()
{
    if (m_layer)
        m_layer->setOwner(0);
    if (m_contentsLayer)
        m_contentsLayer->setOwner(0);
    if (m_transformLayer)
        m_transformLayer->setOwner(0);

    GraphicsLayer::willBeDestroyed();
}

void GraphicsLayerBlackBerry::setName(const String& inName)
{
    String name = String::format("GraphicsLayerBlackBerry(%p) GraphicsLayer(%p) ", m_layer.get(), this) + inName;
    GraphicsLayer::setName(name);
}

bool GraphicsLayerBlackBerry::setChildren(const Vector<GraphicsLayer*>& children)
{
    bool childrenChanged = GraphicsLayer::setChildren(children);
    // FIXME: GraphicsLayer::setChildren calls addChild() for each sublayer, which
    // will end up calling updateSublayerList() N times.
    if (childrenChanged)
        updateSublayerList();

    return childrenChanged;
}

void GraphicsLayerBlackBerry::addChild(GraphicsLayer* childLayer)
{
    GraphicsLayer::addChild(childLayer);
    updateSublayerList();
}

void GraphicsLayerBlackBerry::addChildAtIndex(GraphicsLayer* childLayer, int index)
{
    GraphicsLayer::addChildAtIndex(childLayer, index);
    updateSublayerList();
}

void GraphicsLayerBlackBerry::addChildBelow(GraphicsLayer* childLayer, GraphicsLayer* sibling)
{
    GraphicsLayer::addChildBelow(childLayer, sibling);
    updateSublayerList();
}

void GraphicsLayerBlackBerry::addChildAbove(GraphicsLayer* childLayer, GraphicsLayer *sibling)
{
    GraphicsLayer::addChildAbove(childLayer, sibling);
    updateSublayerList();
}

bool GraphicsLayerBlackBerry::replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild)
{
    if (!GraphicsLayer::replaceChild(oldChild, newChild))
        return false;

    updateSublayerList();
    return true;
}

void GraphicsLayerBlackBerry::removeFromParent()
{
    GraphicsLayer::removeFromParent();
    layerForSuperlayer()->removeFromSuperlayer();
}

void GraphicsLayerBlackBerry::setPosition(const FloatPoint& point)
{
    GraphicsLayer::setPosition(point);
    updateLayerPosition();
}

void GraphicsLayerBlackBerry::setAnchorPoint(const FloatPoint3D& point)
{
    if (point == m_anchorPoint)
        return;

    GraphicsLayer::setAnchorPoint(point);
    updateAnchorPoint();
}

void GraphicsLayerBlackBerry::setSize(const FloatSize& size)
{
    if (size == m_size)
        return;

    GraphicsLayer::setSize(size);
    updateLayerSize();
}

void GraphicsLayerBlackBerry::setTransform(const TransformationMatrix& transform)
{
    if (transform == m_transform)
        return;

    GraphicsLayer::setTransform(transform);
    updateTransform();
}

void GraphicsLayerBlackBerry::setChildrenTransform(const TransformationMatrix& transform)
{
    if (transform == m_childrenTransform)
        return;

    GraphicsLayer::setChildrenTransform(transform);
    updateChildrenTransform();
}

void GraphicsLayerBlackBerry::setPreserves3D(bool preserves3D)
{
    if (preserves3D == m_preserves3D)
        return;

    GraphicsLayer::setPreserves3D(preserves3D);
    updateLayerPreserves3D();
}

void GraphicsLayerBlackBerry::setMasksToBounds(bool masksToBounds)
{
    if (masksToBounds == m_masksToBounds)
        return;

    GraphicsLayer::setMasksToBounds(masksToBounds);
    updateMasksToBounds();
}

void GraphicsLayerBlackBerry::setDrawsContent(bool drawsContent)
{
    // Note carefully this early-exit is only correct because we also properly initialize
    // LayerWebKitThread::isDrawable() whenever m_contentsLayer is set to a new layer in setupContentsLayer().
    if (drawsContent == m_drawsContent)
        return;

    GraphicsLayer::setDrawsContent(drawsContent);
    updateLayerIsDrawable();
}

void GraphicsLayerBlackBerry::setContentsVisible(bool contentsVisible)
{
    // Note carefully this early-exit is only correct because we also properly initialize
    // LayerWebKitThread::isDrawable() whenever m_contentsLayer is set to a new layer in setupContentsLayer().
    if (contentsVisible == m_contentsVisible)
        return;

    GraphicsLayer::setContentsVisible(contentsVisible);
    updateLayerIsDrawable();
}

void GraphicsLayerBlackBerry::setMaskLayer(GraphicsLayer* maskLayer)
{
    if (maskLayer == m_maskLayer)
        return;

    GraphicsLayer::setMaskLayer(maskLayer);

    LayerWebKitThread* maskLayerWebKit = m_maskLayer ? m_maskLayer->platformLayer() : 0;
    if (maskLayerWebKit)
        maskLayerWebKit->setIsMask(true);
    m_layer->setMaskLayer(maskLayerWebKit);
}

void GraphicsLayerBlackBerry::setReplicatedByLayer(GraphicsLayer* layer)
{
    GraphicsLayerBlackBerry* layerWebKit = static_cast<GraphicsLayerBlackBerry*>(layer);
    GraphicsLayer::setReplicatedByLayer(layer);
    LayerWebKitThread* replicaLayer = layerWebKit ? layerWebKit->primaryLayer() : 0;
    primaryLayer()->setReplicaLayer(replicaLayer);
}

void GraphicsLayerBlackBerry::setHasFixedContainer(bool hasFixedContainer)
{
    if (hasFixedContainer == m_hasFixedContainer)
        return;

    GraphicsLayer::setHasFixedContainer(hasFixedContainer);
    updateHasFixedContainer();
}

void GraphicsLayerBlackBerry::setHasFixedAncestorInDOMTree(bool hasFixedAncestorInDOMTree)
{
    if (hasFixedAncestorInDOMTree == m_hasFixedAncestorInDOMTree)
        return;

    GraphicsLayer::setHasFixedAncestorInDOMTree(hasFixedAncestorInDOMTree);
    updateHasFixedAncestorInDOMTree();
}

#if ENABLE(CSS_FILTERS)
bool GraphicsLayerBlackBerry::setFilters(const FilterOperations& filters)
{
    if (m_filters == filters)
        return true;

    bool canCompositeFilters = LayerWebKitThread::filtersCanBeComposited(filters);
    if (canCompositeFilters) {
        m_filters = filters;
        GraphicsLayer::setFilters(filters);
        updateFilters();
    } else {
        m_filters.clear();
        notImplemented();
    }

    return canCompositeFilters;
}
#endif

void GraphicsLayerBlackBerry::setBackgroundColor(const Color& color)
{
    if (color == m_backgroundColor)
        return;

    GraphicsLayer::setBackgroundColor(color);

    updateLayerBackgroundColor();
}

void GraphicsLayerBlackBerry::setContentsOpaque(bool opaque)
{
    if (m_contentsOpaque == opaque)
        return;

    GraphicsLayer::setContentsOpaque(opaque);
    updateContentsOpaque();
}

void GraphicsLayerBlackBerry::setBackfaceVisibility(bool visible)
{
    if (m_backfaceVisibility == visible)
        return;

    GraphicsLayer::setBackfaceVisibility(visible);
    updateBackfaceVisibility();
}

void GraphicsLayerBlackBerry::setOpacity(float opacity)
{
    float clampedOpacity = clampTo(opacity, 0.0f, 1.0f);

    if (m_opacity == clampedOpacity)
        return;

    GraphicsLayer::setOpacity(clampedOpacity);
    primaryLayer()->setOpacity(opacity);
}

void GraphicsLayerBlackBerry::setContentsNeedsDisplay()
{
    if (m_contentsLayer)
        m_contentsLayer->setNeedsDisplay();
}

void GraphicsLayerBlackBerry::setNeedsDisplay()
{
    if (drawsContent())
        m_layer->setNeedsDisplay();
}

void GraphicsLayerBlackBerry::setNeedsDisplayInRect(const FloatRect& rect)
{
    if (drawsContent())
        m_layer->setNeedsDisplayInRect(rect);
}

void GraphicsLayerBlackBerry::setContentsRect(const IntRect& rect)
{
    if (rect == m_contentsRect)
        return;

    GraphicsLayer::setContentsRect(rect);
    updateContentsRect();
}

static PassRefPtr<LayerAnimation> removeAnimationByIdAndProperty(int id, AnimatedPropertyID property, Vector<RefPtr<LayerAnimation> >& list)
{
    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i]->id() == id && list[i]->property() == property) {
            RefPtr<LayerAnimation> layerAnimation = list[i];
            list.remove(i);
            return layerAnimation;
        }
    }

    return PassRefPtr<LayerAnimation>();
}

static PassRefPtr<LayerAnimation> removeAnimationByName(const String& animationName, Vector<RefPtr<LayerAnimation> >& list)
{
    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i]->name() == animationName) {
            RefPtr<LayerAnimation> layerAnimation = list[i];
            list.remove(i);
            return layerAnimation;
        }
    }

    return PassRefPtr<LayerAnimation>();
}

bool GraphicsLayerBlackBerry::addAnimation(const KeyframeValueList& values, const IntSize& boxSize, const Animation* animation, const String& animationName, double timeOffset)
{
    // This is what GraphicsLayerCA checks for.
    if (!animation || animation->isEmptyOrZeroDuration() || values.size() < 2)
        return false;

    // We only support these two kinds of properties at the moment.
    if (values.property() != AnimatedPropertyWebkitTransform && values.property() != AnimatedPropertyOpacity)
        return false;

    // Remove any running animation for the same property.
    // FIXME: Maybe this is superstition, I got the idea from GraphicsLayerQt
    // WebCore might be adding an animation with the same name, but for a different property
    removeAnimationByIdAndProperty(LayerAnimation::idFromAnimation(animation), values.property(), m_runningAnimations);
    removeAnimationByIdAndProperty(LayerAnimation::idFromAnimation(animation), values.property(), m_suspendedAnimations);

    RefPtr<LayerAnimation> layerAnimation = LayerAnimation::create(values, boxSize, animation, animationName, timeOffset);

#if DEBUG_LAYER_ANIMATION
    fprintf(stderr, "LayerAnimation 0x%08x: Adding animation %s for property %d\n", (int)layerAnimation.get(), animationName.latin1().data(), values.property());
#endif

    m_runningAnimations.append(layerAnimation);

    updateAnimations();

    return true;
}

void GraphicsLayerBlackBerry::pauseAnimation(const String& animationName, double timeOffset)
{
    // WebCore might have added several animations with the same name, but for different properties

    while (RefPtr<LayerAnimation> animation = removeAnimationByName(animationName, m_runningAnimations)) {
#if DEBUG_LAYER_ANIMATION
        fprintf(stderr, "LayerAnimation 0x%08x: Pausing animation %s\n", (int)animation.get(), animation->name().latin1().data());
#endif

        // LayerAnimation is readonly. Create a new animation with the same data except for timeOffset.
        // WebCore will adjust the timeOffset for paused animations so it can be used to calculate the
        // progress for the paused animation without knowing the exact timestamp when the animation was
        // paused.
        // If an animation was started with a timeOffset dt_orig and paused dt_pause seconds later, the
        // cloned animation will have a timeOffset of dt_pause + dt_orig.
        animation = animation->clone(timeOffset);
        m_suspendedAnimations.append(animation);

#if DEBUG_LAYER_ANIMATION
        fprintf(stderr, "LayerAnimation 0x%08x: Paused animation %s\n", (int)animation.get(), animation->name().latin1().data());
#endif
    };

    updateAnimations();
}

void GraphicsLayerBlackBerry::removeAnimation(const String& animationName)
{
    // WebCore might have added several animations with the same name, but for different properties

#if DEBUG_LAYER_ANIMATION
    fprintf(stderr, "LayerAnimation: Removing animation %s\n", animationName.latin1().data());
#endif

    while (removeAnimationByName(animationName, m_runningAnimations)) { }
    while (removeAnimationByName(animationName, m_suspendedAnimations)) { }

    updateAnimations();
}

void GraphicsLayerBlackBerry::suspendAnimations(double time)
{
#if DEBUG_LAYER_ANIMATION
    fprintf(stderr, "LayerAnimation: Suspending animations\n");
#endif
    m_suspendTime = time;
}

void GraphicsLayerBlackBerry::resumeAnimations()
{
#if DEBUG_LAYER_ANIMATION
    fprintf(stderr, "LayerAnimation: Resuming animations\n");
#endif
    m_suspendTime = 0;
}

void GraphicsLayerBlackBerry::setContentsToImage(Image* image)
{
    bool childrenChanged = false;
    if (image) {
        m_contentsLayerPurpose = ContentsLayerForImage;
        if (!m_contentsLayer)
            childrenChanged = true;
    } else {
        m_contentsLayerPurpose = NoContentsLayer;
        if (m_contentsLayer)
            childrenChanged = true;
    }

    updateContentsImage(image);

    if (childrenChanged)
        updateSublayerList();
}

void GraphicsLayerBlackBerry::updateContentsImage(Image* image)
{
    if (image) {
        if (!m_contentsLayer.get()) {
            RefPtr<LayerWebKitThread> imageLayer = LayerWebKitThread::create(LayerData::Layer, this);

            setupContentsLayer(imageLayer.get());
            m_contentsLayer = imageLayer;
            // m_contentsLayer will be parented by updateSublayerList.
        }
        m_contentsLayer->setContents(image);

        updateContentsRect();
    } else {
        // No image. m_contentsLayer will be removed via updateSublayerList.
        m_contentsLayer = 0;
    }
}

void GraphicsLayerBlackBerry::setContentsToCanvas(PlatformLayer* platformLayer)
{
    bool childrenChanged = false;
    if (platformLayer) {
        platformLayer->setOwner(this);
        if (m_contentsLayer.get() != platformLayer) {
            setupContentsLayer(platformLayer);
            m_contentsLayer = platformLayer;
            m_contentsLayerPurpose = ContentsLayerForCanvas;
            childrenChanged = true;
        }
        m_contentsLayer->setNeedsDisplay();
        updateContentsRect();
    } else {
        if (m_contentsLayer) {
            childrenChanged = true;

            // The old contents layer will be removed via updateSublayerList.
            m_contentsLayer = 0;
        }
    }

    if (childrenChanged)
        updateSublayerList();
}

void GraphicsLayerBlackBerry::setContentsToMedia(PlatformLayer* layer)
{
    bool childrenChanged = false;
    if (layer) {
        if (!m_contentsLayer.get() || m_contentsLayerPurpose != ContentsLayerForVideo) {
            setupContentsLayer(layer);
            m_contentsLayer = layer;
            m_contentsLayerPurpose = ContentsLayerForVideo;
            childrenChanged = true;
        }

        if (layer->owner() != this) {
            layer->setOwner(this);
            layer->setNeedsDisplay();
        }
        updateContentsRect();
    } else {
        if (m_contentsLayer) {
            childrenChanged = true;

            // The old contents layer will be removed via updateSublayerList.
            m_contentsLayer = 0;
        }
    }

    if (childrenChanged)
        updateSublayerList();
}

PlatformLayer* GraphicsLayerBlackBerry::hostLayerForSublayers() const
{
    return m_transformLayer ? m_transformLayer.get() : m_layer.get();
}

PlatformLayer* GraphicsLayerBlackBerry::layerForSuperlayer() const
{
    return m_transformLayer ? m_transformLayer.get() : m_layer.get();
}

PlatformLayer* GraphicsLayerBlackBerry::platformLayer() const
{
    return primaryLayer();
}

void GraphicsLayerBlackBerry::setDebugBackgroundColor(const Color& color)
{
    if (color.isValid())
        setLayerBackgroundColor(*m_layer, color);
    else
        clearLayerBackgroundColor(*m_layer);
}

void GraphicsLayerBlackBerry::setDebugBorder(const Color& color, float borderWidth)
{
    if (color.isValid()) {
        setLayerBorderColor(*m_layer, color);
        m_layer->setBorderWidth(borderWidth);
    } else {
        clearBorderColor(*m_layer);
        m_layer->setBorderWidth(0);
    }
}

void GraphicsLayerBlackBerry::updateSublayerList()
{
    Vector<RefPtr<LayerWebKitThread> > newSublayers;

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
        GraphicsLayerBlackBerry* curChild = static_cast<GraphicsLayerBlackBerry*>(childLayers[i]);

        LayerWebKitThread* childLayer = curChild->layerForSuperlayer();
        newSublayers.append(childLayer);
    }

    for (size_t i = 0; i < newSublayers.size(); ++i)
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

void GraphicsLayerBlackBerry::updateLayerPosition()
{
    // Position is offset on the layer by the layer anchor point.
    FloatPoint layerPosition(
        m_position.x() + m_anchorPoint.x() * m_size.width(),
        m_position.y() + m_anchorPoint.y() * m_size.height());

    primaryLayer()->setPosition(layerPosition);
}

void GraphicsLayerBlackBerry::updateLayerSize()
{
    IntSize layerSize(m_size.width(), m_size.height());
    if (m_transformLayer) {
        m_transformLayer->setBounds(layerSize);
        // The anchor of the contents layer is always at 0.5, 0.5, so the position is center-relative.
        FloatPoint centerPoint(m_size.width() / 2, m_size.height() / 2);
        m_layer->setPosition(centerPoint);
    }

    m_layer->setBounds(layerSize);

    // Note that we don't resize m_contentsLayer. It's up the caller to do that.

    // If we've changed the bounds, we need to recalculate the position
    // of the layer, taking anchor point into account.
    updateLayerPosition();
}

void GraphicsLayerBlackBerry::updateAnchorPoint()
{
    primaryLayer()->setAnchorPoint(FloatPoint(m_anchorPoint.x(), m_anchorPoint.y()));
    primaryLayer()->setAnchorPointZ(m_anchorPoint.z());
    updateLayerPosition();
}

void GraphicsLayerBlackBerry::updateTransform()
{
    primaryLayer()->setTransform(m_transform);
}

void GraphicsLayerBlackBerry::updateChildrenTransform()
{
    primaryLayer()->setSublayerTransform(m_childrenTransform);
}

void GraphicsLayerBlackBerry::updateMasksToBounds()
{
    m_layer->setMasksToBounds(m_masksToBounds);
    updateDebugIndicators();
}

void GraphicsLayerBlackBerry::updateContentsOpaque()
{
    m_layer->setOpaque(m_contentsOpaque);
}

void GraphicsLayerBlackBerry::updateBackfaceVisibility()
{
    m_layer->setDoubleSided(m_backfaceVisibility);
}

void GraphicsLayerBlackBerry::updateLayerPreserves3D()
{
    if (m_preserves3D && !m_transformLayer) {
        // Create the transform layer.
        m_transformLayer = LayerWebKitThread::create(LayerData::TransformLayer, this);

        // Copy the position from this layer.
        updateLayerPosition();
        updateLayerSize();
        updateAnchorPoint();
        updateTransform();
        updateChildrenTransform();
        updateAnimations();

        m_layer->setPosition(FloatPoint(m_size.width() / 2.0f, m_size.height() / 2.0f));

        m_layer->setAnchorPoint(FloatPoint(0.5f, 0.5f));
        TransformationMatrix identity;
        m_layer->setTransform(identity);

        // Set the old layer to opacity of 1. Further down we will set the opacity on the transform layer.
        m_layer->setOpacity(1);

        // Move this layer to be a child of the transform layer.
        if (m_layer->superlayer())
            m_layer->superlayer()->replaceSublayer(m_layer.get(), m_transformLayer.get());
        m_transformLayer->addSublayer(m_layer.get());

        m_transformLayer->setPreserves3D(true);
        m_layer->setPreserves3D(true);

        updateSublayerList();
    } else if (!m_preserves3D && m_transformLayer) {
        // Relace the transformLayer in the parent with this layer.
        m_layer->removeFromSuperlayer();
        if (m_transformLayer->superlayer())
            m_transformLayer->superlayer()->replaceSublayer(m_transformLayer.get(), m_layer.get());

        // Release the transform layer.
        m_transformLayer = 0;

        updateLayerPosition();
        updateLayerSize();
        updateAnchorPoint();
        updateTransform();
        updateChildrenTransform();
        updateAnimations();

        m_layer->setPreserves3D(false);

        updateSublayerList();
    }

    updateOpacityOnLayer();
}

void GraphicsLayerBlackBerry::updateLayerIsDrawable()
{
    // For the rest of the accelerated compositor code, there is no reason to make a
    // distinction between drawsContent and contentsVisible. So, for m_layer, these two
    // flags are combined here. m_contentsLayer shouldn't receive the drawsContent flag
    // so it is only given contentsVisible.
    m_layer->setDrawable(m_drawsContent && m_contentsVisible);

    if (m_contentsLayer)
        m_contentsLayer->setDrawable(m_contentsVisible);

    if (m_drawsContent)
        m_layer->setNeedsDisplay();

    updateDebugIndicators();
}

void GraphicsLayerBlackBerry::updateHasFixedContainer()
{
    m_layer->setHasFixedContainer(m_hasFixedContainer);
}

void GraphicsLayerBlackBerry::updateHasFixedAncestorInDOMTree()
{
    m_layer->setHasFixedAncestorInDOMTree(m_hasFixedAncestorInDOMTree);
}

void GraphicsLayerBlackBerry::updateLayerBackgroundColor()
{
    if (m_backgroundColor.isValid())
        setLayerBackgroundColor(*m_layer, m_backgroundColor);
    else
        clearLayerBackgroundColor(*m_layer);
}

#if ENABLE(CSS_FILTERS)
void GraphicsLayerBlackBerry::updateFilters()
{
    if (!m_filters.size())
        return;

    primaryLayer()->setFilters(m_filters);
}
#endif

void GraphicsLayerBlackBerry::updateAnimations()
{
    // When there is a transform layer, the transform must be set on that layer
    // instead of the content layer. Opacity can be set on the transform layer or the
    // layer with equal outcome, but currently it is also set on the transform
    // layer. Since we only accelerate animations of these two properties, it
    // is safe to move all accelerated animations to the transform layer, and
    // remove them from the layer proper.
    // So the following code, while it looks strange, is correct:
    // we transfer all animations to the transform layer if it exists.
    // FIXME: If other properties become animated, it may not be equivalent to
    // move them to the transform layer. Then this code needs to be revisited
    // to only move the transform animations to the transform layer.
    primaryLayer()->setRunningAnimations(m_runningAnimations);
    primaryLayer()->setSuspendedAnimations(m_suspendedAnimations);

    // We need to move the animations to the transform layer if there is one.
    if (m_transformLayer) {
        m_layer->setRunningAnimations(Vector<RefPtr<LayerAnimation> >());
        m_layer->setSuspendedAnimations(Vector<RefPtr<LayerAnimation> >());
    }
}

void GraphicsLayerBlackBerry::updateContentsVideo()
{
    // FIXME: Implement
}

void GraphicsLayerBlackBerry::updateContentsRect()
{
    if (!m_contentsLayer)
        return;

    if (m_contentsLayer->position() != m_contentsRect.location())
        m_contentsLayer->setPosition(m_contentsRect.location());

    if (m_contentsLayer->bounds() != m_contentsRect.size())
        m_contentsLayer->setBounds(m_contentsRect.size());
}

void GraphicsLayerBlackBerry::setupContentsLayer(LayerWebKitThread* contentsLayer)
{
    if (contentsLayer == m_contentsLayer)
        return;

    if (m_contentsLayer) {
        m_contentsLayer->removeFromSuperlayer();
        m_contentsLayer = 0;
    }

    if (contentsLayer) {
        m_contentsLayer = contentsLayer;

        m_contentsLayer->setAnchorPoint(FloatPoint::zero());

        // It is necessary to update setDrawable as soon as we receive the new contentsLayer, for
        // the correctness of early exit conditions in setDrawsContent() and setContentsVisible().
        m_contentsLayer->setDrawable(m_contentsVisible);

        // Insert the content layer first. Video elements require this, because they have
        // shadow content that must display in front of the video.
        m_layer->insertSublayer(m_contentsLayer.get(), 0);

        updateContentsRect();

        if (isShowingDebugBorder()) {
            setLayerBorderColor(*m_contentsLayer, Color(0, 0, 128, 180));
            m_contentsLayer->setBorderWidth(1);
        }
    }
    updateDebugIndicators();
}

// This function simply mimics the operation of GraphicsLayerCA
void GraphicsLayerBlackBerry::updateOpacityOnLayer()
{
    primaryLayer()->setOpacity(m_opacity);
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
