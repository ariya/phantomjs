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

#include "PlatformCALayer.h"

#include "AbstractCACFLayerTreeHost.h"
#include "Font.h"
#include "GraphicsContext.h"
#include "PlatformCALayerWinInternal.h"
#include <QuartzCore/CoreAnimationCF.h>
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#include <wtf/CurrentTime.h>
#include <wtf/text/CString.h>

using namespace WebCore;

bool PlatformCALayer::isValueFunctionSupported()
{
    return true;
}

void PlatformCALayer::setOwner(PlatformCALayerClient* owner)
{
    m_owner = owner;
}

static CFStringRef toCACFLayerType(PlatformCALayer::LayerType type)
{
    return (type == PlatformCALayer::LayerTypeTransformLayer) ? kCACFTransformLayer : kCACFLayer;
}

static CFStringRef toCACFFilterType(PlatformCALayer::FilterType type)
{
    switch (type) {
    case PlatformCALayer::Linear: return kCACFFilterLinear;
    case PlatformCALayer::Nearest: return kCACFFilterNearest;
    case PlatformCALayer::Trilinear: return kCACFFilterTrilinear;
    default: return 0;
    }
}

static AbstractCACFLayerTreeHost* layerTreeHostForLayer(const PlatformCALayer* layer)
{
    // We need the AbstractCACFLayerTreeHost associated with this layer, which is stored in the UserData of the CACFContext
    void* userData = wkCACFLayerGetContextUserData(layer->platformLayer());
    if (!userData)
        return 0;

    return static_cast<AbstractCACFLayerTreeHost*>(userData);
}

static PlatformCALayerWinInternal* intern(const PlatformCALayer* layer)
{
    return static_cast<PlatformCALayerWinInternal*>(CACFLayerGetUserData(layer->platformLayer()));
}

static PlatformCALayerWinInternal* intern(void* layer)
{
    return static_cast<PlatformCALayerWinInternal*>(CACFLayerGetUserData(static_cast<CACFLayerRef>(layer)));
}

PassRefPtr<PlatformCALayer> PlatformCALayer::create(LayerType layerType, PlatformCALayerClient* owner)
{
    return adoptRef(new PlatformCALayer(layerType, 0, owner));
}

PassRefPtr<PlatformCALayer> PlatformCALayer::create(void* platformLayer, PlatformCALayerClient* owner)
{
    return adoptRef(new PlatformCALayer(LayerTypeCustom, static_cast<PlatformLayer*>(platformLayer), owner));
}

static void displayCallback(CACFLayerRef caLayer, CGContextRef context)
{
    ASSERT_ARG(caLayer, CACFLayerGetUserData(caLayer));
    intern(caLayer)->displayCallback(caLayer, context);
}

static void layoutSublayersProc(CACFLayerRef caLayer) 
{
    PlatformCALayer* layer = PlatformCALayer::platformCALayer(caLayer);
    if (layer && layer->owner())
        layer->owner()->platformCALayerLayoutSublayersOfLayer(layer);
}

PlatformCALayer::PlatformCALayer(LayerType layerType, PlatformLayer* layer, PlatformCALayerClient* owner)
    : m_owner(owner)
{
    if (layer) {
        m_layerType = LayerTypeCustom;
        m_layer = layer;
    } else {
        m_layerType = layerType;
        m_layer = adoptCF(CACFLayerCreate(toCACFLayerType(layerType)));

        // Create the PlatformCALayerWinInternal object and point to it in the userdata.
        PlatformCALayerWinInternal* intern = new PlatformCALayerWinInternal(this);
        CACFLayerSetUserData(m_layer.get(), intern);

        // Set the display callback
        CACFLayerSetDisplayCallback(m_layer.get(), displayCallback);
        CACFLayerSetLayoutCallback(m_layer.get(), layoutSublayersProc);    
    }
}

PlatformCALayer::~PlatformCALayer()
{
    // Toss all the kids
    removeAllSublayers();

    // Get rid of the user data
    PlatformCALayerWinInternal* layerIntern = intern(this);
    CACFLayerSetUserData(m_layer.get(), 0);

    delete layerIntern;
}

PlatformCALayer* PlatformCALayer::platformCALayer(void* platformLayer)
{
    if (!platformLayer)
        return 0;
    
    PlatformCALayerWinInternal* layerIntern = intern(platformLayer);
    return layerIntern ? layerIntern->owner() : 0;
}

PassRefPtr<PlatformCALayer> PlatformCALayer::clone(PlatformCALayerClient* owner) const
{
    PlatformCALayer::LayerType type = (layerType() == PlatformCALayer::LayerTypeTransformLayer) ?
        PlatformCALayer::LayerTypeTransformLayer : PlatformCALayer::LayerTypeLayer;
    RefPtr<PlatformCALayer> newLayer = PlatformCALayer::create(type, owner);

    newLayer->setPosition(position());
    newLayer->setBounds(bounds());
    newLayer->setAnchorPoint(anchorPoint());
    newLayer->setTransform(transform());
    newLayer->setSublayerTransform(sublayerTransform());
    newLayer->setContents(contents());
    newLayer->setMasksToBounds(masksToBounds());
    newLayer->setDoubleSided(isDoubleSided());
    newLayer->setOpaque(isOpaque());
    newLayer->setBackgroundColor(backgroundColor());
    newLayer->setContentsScale(contentsScale());
#if ENABLE(CSS_FILTERS)
    newLayer->copyFiltersFrom(this);
#endif

    return newLayer;
}

PlatformLayer* PlatformCALayer::platformLayer() const
{
    return m_layer.get();
}

PlatformCALayer* PlatformCALayer::rootLayer() const
{
    AbstractCACFLayerTreeHost* host = layerTreeHostForLayer(this);
    return host ? host->rootLayer() : 0;
}

void PlatformCALayer::animationStarted(CFTimeInterval beginTime)
{
    // Update start time for any animation not yet started
    CFTimeInterval cacfBeginTime = currentTimeToMediaTime(beginTime);

    HashMap<String, RefPtr<PlatformCAAnimation> >::const_iterator end = m_animations.end();
    for (HashMap<String, RefPtr<PlatformCAAnimation> >::const_iterator it = m_animations.begin(); it != end; ++it)
        it->value->setActualStartTimeIfNeeded(cacfBeginTime);

    if (m_owner)
        m_owner->platformCALayerAnimationStarted(beginTime);
}

static void resubmitAllAnimations(PlatformCALayer* layer)
{
    HashMap<String, RefPtr<PlatformCAAnimation> >::const_iterator end = layer->animations().end();
    for (HashMap<String, RefPtr<PlatformCAAnimation> >::const_iterator it = layer->animations().begin(); it != end; ++it)
        CACFLayerAddAnimation(layer->platformLayer(), it->key.createCFString().get(), it->value->platformAnimation());
}

void PlatformCALayer::ensureAnimationsSubmitted()
{
    resubmitAllAnimations(this);

    PlatformCALayerList children;
    intern(this)->getSublayers(children);
    for (size_t i = 0; i < children.size(); ++i)
        children[i]->ensureAnimationsSubmitted();
}

void PlatformCALayer::setNeedsDisplay(const FloatRect* dirtyRect)
{
    intern(this)->setNeedsDisplay(dirtyRect);
}
    
void PlatformCALayer::setNeedsCommit()
{
    AbstractCACFLayerTreeHost* host = layerTreeHostForLayer(this);
    if (host)
        host->layerTreeDidChange();
}

void PlatformCALayer::setContentsChanged()
{
    // FIXME: There is no equivalent of setContentsChanged in CACF. For now I will
    // set contents to 0 and then back to its original value to see if that
    // kicks CACF into redisplaying.
    RetainPtr<CFTypeRef> contents = CACFLayerGetContents(m_layer.get());
    CACFLayerSetContents(m_layer.get(), 0);
    CACFLayerSetContents(m_layer.get(), contents.get());
    setNeedsCommit();
}

void PlatformCALayer::setNeedsLayout()
{
    if (!m_owner || !m_owner->platformCALayerRespondsToLayoutChanges())
        return;

    CACFLayerSetNeedsLayout(m_layer.get());
    setNeedsCommit();
}

PlatformCALayer* PlatformCALayer::superlayer() const
{
    return platformCALayer(CACFLayerGetSuperlayer(m_layer.get()));
}

void PlatformCALayer::removeFromSuperlayer()
{
    CACFLayerRemoveFromSuperlayer(m_layer.get());
    setNeedsCommit();
}

void PlatformCALayer::setSublayers(const PlatformCALayerList& list)
{
    intern(this)->setSublayers(list);
}

void PlatformCALayer::removeAllSublayers()
{
    intern(this)->removeAllSublayers();
}

void PlatformCALayer::appendSublayer(PlatformCALayer* layer)
{
    // This must be in terms of insertSublayer instead of a direct call so PlatformCALayerInternal can override.
    insertSublayer(layer, sublayerCount());
}

void PlatformCALayer::insertSublayer(PlatformCALayer* layer, size_t index)
{
    intern(this)->insertSublayer(layer, index);
}

void PlatformCALayer::replaceSublayer(PlatformCALayer* reference, PlatformCALayer* newLayer)
{
    // This must not use direct calls to allow PlatformCALayerInternal to override.
    ASSERT_ARG(reference, reference);
    ASSERT_ARG(reference, reference->superlayer() == this);

    if (reference == newLayer)
        return;

    int referenceIndex = intern(this)->indexOfSublayer(reference);
    ASSERT(referenceIndex != -1);
    if (referenceIndex == -1)
        return;

    reference->removeFromSuperlayer();

    if (newLayer) {
        newLayer->removeFromSuperlayer();
        insertSublayer(newLayer, referenceIndex);
    }
}

size_t PlatformCALayer::sublayerCount() const
{
    return intern(this)->sublayerCount();
}

void PlatformCALayer::adoptSublayers(PlatformCALayer* source)
{
    PlatformCALayerList sublayers;
    intern(source)->getSublayers(sublayers);

    // Use setSublayers() because it properly nulls out the superlayer pointers.
    setSublayers(sublayers);
}

void PlatformCALayer::addAnimationForKey(const String& key, PlatformCAAnimation* animation)
{
    // Add it to the animation list
    m_animations.add(key, animation);

    CACFLayerAddAnimation(m_layer.get(), key.createCFString().get(), animation->platformAnimation());
    setNeedsCommit();

    // Tell the host about it so we can fire the start animation event
    AbstractCACFLayerTreeHost* host = layerTreeHostForLayer(this);
    if (host)
        host->addPendingAnimatedLayer(this);
}

void PlatformCALayer::removeAnimationForKey(const String& key)
{
    // Remove it from the animation list
    m_animations.remove(key);

    CACFLayerRemoveAnimation(m_layer.get(), key.createCFString().get());

    // We don't "remove" a layer from AbstractCACFLayerTreeHost when it loses an animation.
    // There may be other active animations on the layer and if an animation
    // callback is fired on a layer without any animations no harm is done.

    setNeedsCommit();
}

PassRefPtr<PlatformCAAnimation> PlatformCALayer::animationForKey(const String& key)
{
    HashMap<String, RefPtr<PlatformCAAnimation> >::iterator it = m_animations.find(key);
    if (it == m_animations.end())
        return 0;

    return it->value;
}

PlatformCALayer* PlatformCALayer::mask() const
{
    return platformCALayer(CACFLayerGetMask(m_layer.get()));
}

void PlatformCALayer::setMask(PlatformCALayer* layer)
{
    CACFLayerSetMask(m_layer.get(), layer ? layer->platformLayer() : 0);
    setNeedsCommit();
}

bool PlatformCALayer::isOpaque() const
{
    return CACFLayerIsOpaque(m_layer.get());
}

void PlatformCALayer::setOpaque(bool value)
{
    CACFLayerSetOpaque(m_layer.get(), value);
    setNeedsCommit();
}

FloatRect PlatformCALayer::bounds() const
{
    return CACFLayerGetBounds(m_layer.get());
}

void PlatformCALayer::setBounds(const FloatRect& value)
{
    intern(this)->setBounds(value);
    setNeedsLayout();
}

FloatPoint3D PlatformCALayer::position() const
{
    CGPoint point = CACFLayerGetPosition(m_layer.get());
    return FloatPoint3D(point.x, point.y, CACFLayerGetZPosition(m_layer.get()));
}

void PlatformCALayer::setPosition(const FloatPoint3D& value)
{
    CACFLayerSetPosition(m_layer.get(), CGPointMake(value.x(), value.y()));
    CACFLayerSetZPosition(m_layer.get(), value.z());
    setNeedsCommit();
}

FloatPoint3D PlatformCALayer::anchorPoint() const
{
    CGPoint point = CACFLayerGetAnchorPoint(m_layer.get());
    float z = CACFLayerGetAnchorPointZ(m_layer.get());
    return FloatPoint3D(point.x, point.y, z);
}

void PlatformCALayer::setAnchorPoint(const FloatPoint3D& value)
{
    CACFLayerSetAnchorPoint(m_layer.get(), CGPointMake(value.x(), value.y()));
    CACFLayerSetAnchorPointZ(m_layer.get(), value.z());
    setNeedsCommit();
}

TransformationMatrix PlatformCALayer::transform() const
{
    return CACFLayerGetTransform(m_layer.get());
}

void PlatformCALayer::setTransform(const TransformationMatrix& value)
{
    CACFLayerSetTransform(m_layer.get(), value);
    setNeedsCommit();
}

TransformationMatrix PlatformCALayer::sublayerTransform() const
{
    return CACFLayerGetSublayerTransform(m_layer.get());
}

void PlatformCALayer::setSublayerTransform(const TransformationMatrix& value)
{
    CACFLayerSetSublayerTransform(m_layer.get(), value);
    setNeedsCommit();
}

TransformationMatrix PlatformCALayer::contentsTransform() const
{
    // ContentsTransform is not used
    return TransformationMatrix();
}

void PlatformCALayer::setContentsTransform(const TransformationMatrix&)
{
    // ContentsTransform is not used
}

bool PlatformCALayer::isHidden() const
{
    return CACFLayerIsHidden(m_layer.get());
}

void PlatformCALayer::setHidden(bool value)
{
    CACFLayerSetHidden(m_layer.get(), value);
    setNeedsCommit();
}

bool PlatformCALayer::isGeometryFlipped() const
{
    return CACFLayerIsGeometryFlipped(m_layer.get());
}

void PlatformCALayer::setGeometryFlipped(bool value)
{
    CACFLayerSetGeometryFlipped(m_layer.get(), value);
    setNeedsCommit();
}

bool PlatformCALayer::isDoubleSided() const
{
    return CACFLayerIsDoubleSided(m_layer.get());
}

void PlatformCALayer::setDoubleSided(bool value)
{
    CACFLayerSetDoubleSided(m_layer.get(), value);
    setNeedsCommit();
}

bool PlatformCALayer::masksToBounds() const
{
    return CACFLayerGetMasksToBounds(m_layer.get());
}

void PlatformCALayer::setMasksToBounds(bool value)
{
    CACFLayerSetMasksToBounds(m_layer.get(), value);
    setNeedsCommit();
}

bool PlatformCALayer::acceleratesDrawing() const
{
    return false;
}

void PlatformCALayer::setAcceleratesDrawing(bool)
{
}

CFTypeRef PlatformCALayer::contents() const
{
    return CACFLayerGetContents(m_layer.get());
}

void PlatformCALayer::setContents(CFTypeRef value)
{
    CACFLayerSetContents(m_layer.get(), value);
    setNeedsCommit();
}

FloatRect PlatformCALayer::contentsRect() const
{
    return CACFLayerGetContentsRect(m_layer.get());
}

void PlatformCALayer::setContentsRect(const FloatRect& value)
{
    CACFLayerSetContentsRect(m_layer.get(), value);
    setNeedsCommit();
}

void PlatformCALayer::setMinificationFilter(FilterType value)
{
    CACFLayerSetMinificationFilter(m_layer.get(), toCACFFilterType(value));
}

void PlatformCALayer::setMagnificationFilter(FilterType value)
{
    CACFLayerSetMagnificationFilter(m_layer.get(), toCACFFilterType(value));
    setNeedsCommit();
}

Color PlatformCALayer::backgroundColor() const
{
    return CACFLayerGetBackgroundColor(m_layer.get());
}

void PlatformCALayer::setBackgroundColor(const Color& value)
{
    CGFloat components[4];
    value.getRGBA(components[0], components[1], components[2], components[3]);

    RetainPtr<CGColorSpaceRef> colorSpace = adoptCF(CGColorSpaceCreateDeviceRGB());
    RetainPtr<CGColorRef> color = adoptCF(CGColorCreate(colorSpace.get(), components));

    CACFLayerSetBackgroundColor(m_layer.get(), color.get());
    setNeedsCommit();
}

float PlatformCALayer::borderWidth() const
{
    return CACFLayerGetBorderWidth(m_layer.get());
}

void PlatformCALayer::setBorderWidth(float value)
{
    CACFLayerSetBorderWidth(m_layer.get(), value);
    setNeedsCommit();
}

Color PlatformCALayer::borderColor() const
{
    return CACFLayerGetBorderColor(m_layer.get());
}

void PlatformCALayer::setBorderColor(const Color& value)
{
    CGFloat components[4];
    value.getRGBA(components[0], components[1], components[2], components[3]);

    RetainPtr<CGColorSpaceRef> colorSpace = adoptCF(CGColorSpaceCreateDeviceRGB());
    RetainPtr<CGColorRef> color = adoptCF(CGColorCreate(colorSpace.get(), components));

    CACFLayerSetBorderColor(m_layer.get(), color.get());
    setNeedsCommit();
}

float PlatformCALayer::opacity() const
{
    return CACFLayerGetOpacity(m_layer.get());
}

void PlatformCALayer::setOpacity(float value)
{
    CACFLayerSetOpacity(m_layer.get(), value);
    setNeedsCommit();
}

#if ENABLE(CSS_FILTERS)

void PlatformCALayer::setFilters(const FilterOperations&)
{
}

void PlatformCALayer::copyFiltersFrom(const PlatformCALayer*)
{
}

bool PlatformCALayer::filtersCanBeComposited(const FilterOperations&)
{
    return false;
}

#endif // ENABLE(CSS_FILTERS)

String PlatformCALayer::name() const
{
    return CACFLayerGetName(m_layer.get());
}

void PlatformCALayer::setName(const String& value)
{
    CACFLayerSetName(m_layer.get(), value.createCFString().get());
    setNeedsCommit();
}

FloatRect PlatformCALayer::frame() const
{
    return CACFLayerGetFrame(m_layer.get());
}

void PlatformCALayer::setFrame(const FloatRect& value)
{
    intern(this)->setFrame(value);
    setNeedsLayout();
}

float PlatformCALayer::speed() const
{
    return CACFLayerGetSpeed(m_layer.get());
}

void PlatformCALayer::setSpeed(float value)
{
    CACFLayerSetSpeed(m_layer.get(), value);
    setNeedsCommit();
}

CFTimeInterval PlatformCALayer::timeOffset() const
{
    return CACFLayerGetTimeOffset(m_layer.get());
}

void PlatformCALayer::setTimeOffset(CFTimeInterval value)
{
    CACFLayerSetTimeOffset(m_layer.get(), value);
    setNeedsCommit();
}

float PlatformCALayer::contentsScale() const
{
    return 1;
}

void PlatformCALayer::setContentsScale(float)
{
}

TiledBacking* PlatformCALayer::tiledBacking()
{
    return 0;
}

#ifndef NDEBUG
static void printIndent(int indent)
{
    for ( ; indent > 0; --indent)
        fprintf(stderr, "  ");
}

static void printTransform(const CATransform3D& transform)
{
    fprintf(stderr, "[%g %g %g %g; %g %g %g %g; %g %g %g %g; %g %g %g %g]",
                    transform.m11, transform.m12, transform.m13, transform.m14, 
                    transform.m21, transform.m22, transform.m23, transform.m24, 
                    transform.m31, transform.m32, transform.m33, transform.m34, 
                    transform.m41, transform.m42, transform.m43, transform.m44);
}

static void printLayer(const PlatformCALayer* layer, int indent)
{
    FloatPoint3D layerPosition = layer->position();
    FloatPoint3D layerAnchorPoint = layer->anchorPoint();
    FloatRect layerBounds = layer->bounds();
    printIndent(indent);

    char* layerTypeName = 0;
    switch (layer->layerType()) {
    case PlatformCALayer::LayerTypeLayer: layerTypeName = "layer"; break;
    case PlatformCALayer::LayerTypeWebLayer: layerTypeName = "web-layer"; break;
    case PlatformCALayer::LayerTypeTransformLayer: layerTypeName = "transform-layer"; break;
    case PlatformCALayer::LayerTypeWebTiledLayer: layerTypeName = "web-tiled-layer"; break;
    case PlatformCALayer::LayerTypeRootLayer: layerTypeName = "root-layer"; break;
    case PlatformCALayer::LayerTypeCustom: layerTypeName = "custom-layer"; break;
    }

    fprintf(stderr, "(%s [%g %g %g] [%g %g %g %g] [%g %g %g] superlayer=%p\n",
        layerTypeName,
        layerPosition.x(), layerPosition.y(), layerPosition.z(), 
        layerBounds.x(), layerBounds.y(), layerBounds.width(), layerBounds.height(),
        layerAnchorPoint.x(), layerAnchorPoint.y(), layerAnchorPoint.z(), layer->superlayer());

    // Print name if needed
    String layerName = layer->name();
    if (!layerName.isEmpty()) {
        printIndent(indent + 1);
        fprintf(stderr, "(name %s)\n", layerName.utf8().data());
    }

    // Print masksToBounds if needed
    bool layerMasksToBounds = layer->masksToBounds();
    if (layerMasksToBounds) {
        printIndent(indent + 1);
        fprintf(stderr, "(masksToBounds true)\n");
    }

    // Print opacity if needed
    float layerOpacity = layer->opacity();
    if (layerOpacity != 1) {
        printIndent(indent + 1);
        fprintf(stderr, "(opacity %hf)\n", layerOpacity);
    }

    // Print sublayerTransform if needed
    TransformationMatrix layerTransform = layer->sublayerTransform();
    if (!layerTransform.isIdentity()) {
        printIndent(indent + 1);
        fprintf(stderr, "(sublayerTransform ");
        printTransform(layerTransform);
        fprintf(stderr, ")\n");
    }

    // Print transform if needed
    layerTransform = layer->transform();
    if (!layerTransform.isIdentity()) {
        printIndent(indent + 1);
        fprintf(stderr, "(transform ");
        printTransform(layerTransform);
        fprintf(stderr, ")\n");
    }

    // Print contents if needed
    CFTypeRef layerContents = layer->contents();
    if (layerContents) {
        if (CFGetTypeID(layerContents) == CGImageGetTypeID()) {
            CGImageRef imageContents = static_cast<CGImageRef>(const_cast<void*>(layerContents));
            printIndent(indent + 1);
            fprintf(stderr, "(contents (image [%d %d]))\n",
                CGImageGetWidth(imageContents), CGImageGetHeight(imageContents));
        }
    }

    // Print sublayers if needed
    int n = layer->sublayerCount();
    if (n > 0) {
        printIndent(indent + 1);
        fprintf(stderr, "(sublayers\n");

        PlatformCALayerList sublayers;
        intern(layer)->getSublayers(sublayers);
        ASSERT(n == sublayers.size());
        for (int i = 0; i < n; ++i)
            printLayer(sublayers[i].get(), indent + 2);

        printIndent(indent + 1);
        fprintf(stderr, ")\n");
    }

    printIndent(indent);
    fprintf(stderr, ")\n");
}

void PlatformCALayer::printTree() const
{
    // Print heading info
    CGRect rootBounds = bounds();
    fprintf(stderr, "\n\n** Render tree at time %g (bounds %g, %g %gx%g) **\n\n", 
        currentTime(), rootBounds.origin.x, rootBounds.origin.y, rootBounds.size.width, rootBounds.size.height);

    // Print layer tree from the root
    printLayer(this, 0);
}
#endif // #ifndef NDEBUG

#endif // USE(ACCELERATED_COMPOSITING)
