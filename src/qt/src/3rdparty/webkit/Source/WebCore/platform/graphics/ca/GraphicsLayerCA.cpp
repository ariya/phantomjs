/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include "GraphicsLayerCA.h"

#include "Animation.h"
#include "FloatConversion.h"
#include "FloatRect.h"
#include "PlatformCALayer.h"
#include "PlatformString.h"
#include "RotateTransformOperation.h"
#include "ScaleTransformOperation.h"
#include "SystemTime.h"
#include "TranslateTransformOperation.h"
#include <QuartzCore/CATransform3D.h>
#include <limits.h>
#include <wtf/CurrentTime.h>
#include <wtf/text/StringConcatenate.h>

using namespace std;

#define HAVE_MODERN_QUARTZCORE (!defined(BUILDING_ON_LEOPARD))

namespace WebCore {

// The threshold width or height above which a tiled layer will be used. This should be
// large enough to avoid tiled layers for most GraphicsLayers, but less than the OpenGL
// texture size limit on all supported hardware.
static const int cMaxPixelDimension = 2000;

// If we send a duration of 0 to CA, then it will use the default duration
// of 250ms. So send a very small value instead.
static const float cAnimationAlmostZeroDuration = 1e-3f;

static bool isTransformTypeTransformationMatrix(TransformOperation::OperationType transformType)
{
    switch (transformType) {
    case TransformOperation::SKEW_X:
    case TransformOperation::SKEW_Y:
    case TransformOperation::SKEW:
    case TransformOperation::MATRIX:
    case TransformOperation::ROTATE_3D:
    case TransformOperation::MATRIX_3D:
    case TransformOperation::PERSPECTIVE:
    case TransformOperation::IDENTITY:
    case TransformOperation::NONE:
        return true;
    default:
        return false;
    }
}

static bool isTransformTypeFloatPoint3D(TransformOperation::OperationType transformType)
{
    switch (transformType) {
    case TransformOperation::SCALE:
    case TransformOperation::SCALE_3D:
    case TransformOperation::TRANSLATE:
    case TransformOperation::TRANSLATE_3D:
        return true;
    default:
        return false;
    }
}

static bool isTransformTypeNumber(TransformOperation::OperationType transformType)
{
    return !isTransformTypeTransformationMatrix(transformType) && !isTransformTypeFloatPoint3D(transformType);
}

static void getTransformFunctionValue(const TransformOperation* transformOp, TransformOperation::OperationType transformType, const IntSize& size, float& value)
{
    switch (transformType) {
    case TransformOperation::ROTATE:
    case TransformOperation::ROTATE_X:
    case TransformOperation::ROTATE_Y:
        value = transformOp ? narrowPrecisionToFloat(deg2rad(static_cast<const RotateTransformOperation*>(transformOp)->angle())) : 0;
        break;
    case TransformOperation::SCALE_X:
        value = transformOp ? narrowPrecisionToFloat(static_cast<const ScaleTransformOperation*>(transformOp)->x()) : 1;
        break;
    case TransformOperation::SCALE_Y:
        value = transformOp ? narrowPrecisionToFloat(static_cast<const ScaleTransformOperation*>(transformOp)->y()) : 1;
        break;
    case TransformOperation::SCALE_Z:
        value = transformOp ? narrowPrecisionToFloat(static_cast<const ScaleTransformOperation*>(transformOp)->z()) : 1;
        break;
    case TransformOperation::TRANSLATE_X:
        value = transformOp ? narrowPrecisionToFloat(static_cast<const TranslateTransformOperation*>(transformOp)->x(size)) : 0;
        break;
    case TransformOperation::TRANSLATE_Y:
        value = transformOp ? narrowPrecisionToFloat(static_cast<const TranslateTransformOperation*>(transformOp)->y(size)) : 0;
        break;
    case TransformOperation::TRANSLATE_Z:
        value = transformOp ? narrowPrecisionToFloat(static_cast<const TranslateTransformOperation*>(transformOp)->z(size)) : 0;
        break;
    default:
        break;
    }
}

static void getTransformFunctionValue(const TransformOperation* transformOp, TransformOperation::OperationType transformType, const IntSize& size, FloatPoint3D& value)
{
    switch (transformType) {
    case TransformOperation::SCALE:
    case TransformOperation::SCALE_3D:
        value.setX(transformOp ? narrowPrecisionToFloat(static_cast<const ScaleTransformOperation*>(transformOp)->x()) : 1);
        value.setY(transformOp ? narrowPrecisionToFloat(static_cast<const ScaleTransformOperation*>(transformOp)->y()) : 1);
        value.setZ(transformOp ? narrowPrecisionToFloat(static_cast<const ScaleTransformOperation*>(transformOp)->z()) : 1);
        break;
    case TransformOperation::TRANSLATE:
    case TransformOperation::TRANSLATE_3D:
        value.setX(transformOp ? narrowPrecisionToFloat(static_cast<const TranslateTransformOperation*>(transformOp)->x(size)) : 0);
        value.setY(transformOp ? narrowPrecisionToFloat(static_cast<const TranslateTransformOperation*>(transformOp)->y(size)) : 0);
        value.setZ(transformOp ? narrowPrecisionToFloat(static_cast<const TranslateTransformOperation*>(transformOp)->z(size)) : 0);
        break;
    default:
        break;
    }
}

static void getTransformFunctionValue(const TransformOperation* transformOp, TransformOperation::OperationType transformType, const IntSize& size, TransformationMatrix& value)
{
    switch (transformType) {
    case TransformOperation::SKEW_X:
    case TransformOperation::SKEW_Y:
    case TransformOperation::SKEW:
    case TransformOperation::MATRIX:
    case TransformOperation::ROTATE_3D:
    case TransformOperation::MATRIX_3D:
    case TransformOperation::PERSPECTIVE:
    case TransformOperation::IDENTITY:
    case TransformOperation::NONE:
        if (transformOp)
            transformOp->apply(value, size);
        else
            value.makeIdentity();
        break;
    default:
        break;
    }
}

#if HAVE_MODERN_QUARTZCORE
static PlatformCAAnimation::ValueFunctionType getValueFunctionNameForTransformOperation(TransformOperation::OperationType transformType)
{
    // Use literal strings to avoid link-time dependency on those symbols.
    switch (transformType) {
    case TransformOperation::ROTATE_X:
        return PlatformCAAnimation::RotateX;
    case TransformOperation::ROTATE_Y:
        return PlatformCAAnimation::RotateY;
    case TransformOperation::ROTATE:
        return PlatformCAAnimation::RotateZ;
    case TransformOperation::SCALE_X:
        return PlatformCAAnimation::ScaleX;
    case TransformOperation::SCALE_Y:
        return PlatformCAAnimation::ScaleY;
    case TransformOperation::SCALE_Z:
        return PlatformCAAnimation::ScaleZ;
    case TransformOperation::TRANSLATE_X:
        return PlatformCAAnimation::TranslateX;
    case TransformOperation::TRANSLATE_Y:
        return PlatformCAAnimation::TranslateY;
    case TransformOperation::TRANSLATE_Z:
        return PlatformCAAnimation::TranslateZ;
    case TransformOperation::SCALE:
    case TransformOperation::SCALE_3D:
        return PlatformCAAnimation::Scale;
    case TransformOperation::TRANSLATE:
    case TransformOperation::TRANSLATE_3D:
        return PlatformCAAnimation::Translate;
    default:
        return PlatformCAAnimation::NoValueFunction;
    }
}
#endif

static String propertyIdToString(AnimatedPropertyID property)
{
    switch (property) {
    case AnimatedPropertyWebkitTransform:
        return "transform";
    case AnimatedPropertyOpacity:
        return "opacity";
    case AnimatedPropertyBackgroundColor:
        return "backgroundColor";
    case AnimatedPropertyInvalid:
        ASSERT_NOT_REACHED();
    }
    ASSERT_NOT_REACHED();
    return "";
}

static String animationIdentifier(const String& animationName, AnimatedPropertyID property, int index)
{
    return makeString(animationName, '_', String::number(property), '_', String::number(index));
}

static bool animationHasStepsTimingFunction(const KeyframeValueList& valueList, const Animation* anim)
{
    if (anim->timingFunction()->isStepsTimingFunction())
        return true;
    
    for (unsigned i = 0; i < valueList.size(); ++i) {
        const TimingFunction* timingFunction = valueList.at(i)->timingFunction();
        if (timingFunction && timingFunction->isStepsTimingFunction())
            return true;
    }

    return false;
}

PassOwnPtr<GraphicsLayer> GraphicsLayer::create(GraphicsLayerClient* client)
{
    return adoptPtr(new GraphicsLayerCA(client));
}

GraphicsLayerCA::GraphicsLayerCA(GraphicsLayerClient* client)
    : GraphicsLayer(client)
    , m_contentsLayerPurpose(NoContentsLayer)
    , m_contentsLayerHasBackgroundColor(false)
    , m_uncommittedChanges(NoChange)
    , m_contentsScale(1)
    , m_allowTiledLayer(true)
{
    m_layer = PlatformCALayer::create(PlatformCALayer::LayerTypeWebLayer, this);

#if !HAVE_MODERN_QUARTZCORE
    setContentsOrientation(defaultContentsOrientation());
#endif

    updateDebugIndicators();
}

GraphicsLayerCA::~GraphicsLayerCA()
{
    // We release our references to the PlatformCALayers here, but do not actively unparent them,
    // since that will cause a commit and break our batched commit model. The layers will
    // get released when the rootmost modified GraphicsLayerCA rebuilds its child layers.
    
    // Clean up the layer.
    if (m_layer)
        m_layer->setOwner(0);
    
    if (m_contentsLayer)
        m_contentsLayer->setOwner(0);
        
    if (m_structuralLayer)
        m_structuralLayer->setOwner(0);
    
    removeCloneLayers();
}

void GraphicsLayerCA::setName(const String& name)
{
    String longName = String::format("CALayer(%p) GraphicsLayer(%p) ", m_layer.get(), this) + name;
    GraphicsLayer::setName(longName);
    noteLayerPropertyChanged(NameChanged);
}

PlatformLayer* GraphicsLayerCA::platformLayer() const
{
    return primaryLayer()->platformLayer();
}

bool GraphicsLayerCA::setChildren(const Vector<GraphicsLayer*>& children)
{
    bool childrenChanged = GraphicsLayer::setChildren(children);
    if (childrenChanged)
        noteSublayersChanged();
    
    return childrenChanged;
}

void GraphicsLayerCA::addChild(GraphicsLayer* childLayer)
{
    GraphicsLayer::addChild(childLayer);
    noteSublayersChanged();
}

void GraphicsLayerCA::addChildAtIndex(GraphicsLayer* childLayer, int index)
{
    GraphicsLayer::addChildAtIndex(childLayer, index);
    noteSublayersChanged();
}

void GraphicsLayerCA::addChildBelow(GraphicsLayer* childLayer, GraphicsLayer* sibling)
{
    GraphicsLayer::addChildBelow(childLayer, sibling);
    noteSublayersChanged();
}

void GraphicsLayerCA::addChildAbove(GraphicsLayer* childLayer, GraphicsLayer* sibling)
{
    GraphicsLayer::addChildAbove(childLayer, sibling);
    noteSublayersChanged();
}

bool GraphicsLayerCA::replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild)
{
    if (GraphicsLayer::replaceChild(oldChild, newChild)) {
        noteSublayersChanged();
        return true;
    }
    return false;
}

void GraphicsLayerCA::removeFromParent()
{
    if (m_parent)
        static_cast<GraphicsLayerCA*>(m_parent)->noteSublayersChanged();
    GraphicsLayer::removeFromParent();
}

void GraphicsLayerCA::setMaskLayer(GraphicsLayer* layer)
{
    if (layer == m_maskLayer)
        return;

    GraphicsLayer::setMaskLayer(layer);
    noteLayerPropertyChanged(MaskLayerChanged);

    propagateLayerChangeToReplicas();
    
    if (m_replicatedLayer)
        static_cast<GraphicsLayerCA*>(m_replicatedLayer)->propagateLayerChangeToReplicas();
}

void GraphicsLayerCA::setReplicatedLayer(GraphicsLayer* layer)
{
    if (layer == m_replicatedLayer)
        return;

    GraphicsLayer::setReplicatedLayer(layer);
    noteLayerPropertyChanged(ReplicatedLayerChanged);
}

void GraphicsLayerCA::setReplicatedByLayer(GraphicsLayer* layer)
{
    if (layer == m_replicaLayer)
        return;

    GraphicsLayer::setReplicatedByLayer(layer);
    noteSublayersChanged();
    noteLayerPropertyChanged(ReplicatedLayerChanged);
}

void GraphicsLayerCA::setPosition(const FloatPoint& point)
{
    if (point == m_position)
        return;

    GraphicsLayer::setPosition(point);
    noteLayerPropertyChanged(PositionChanged);
}

void GraphicsLayerCA::setAnchorPoint(const FloatPoint3D& point)
{
    if (point == m_anchorPoint)
        return;

    GraphicsLayer::setAnchorPoint(point);
    noteLayerPropertyChanged(AnchorPointChanged);
}

void GraphicsLayerCA::setSize(const FloatSize& size)
{
    if (size == m_size)
        return;

    GraphicsLayer::setSize(size);
    noteLayerPropertyChanged(SizeChanged);
}

void GraphicsLayerCA::setTransform(const TransformationMatrix& t)
{
    if (t == m_transform)
        return;

    GraphicsLayer::setTransform(t);
    noteLayerPropertyChanged(TransformChanged);
}

void GraphicsLayerCA::setChildrenTransform(const TransformationMatrix& t)
{
    if (t == m_childrenTransform)
        return;

    GraphicsLayer::setChildrenTransform(t);
    noteLayerPropertyChanged(ChildrenTransformChanged);
}

void GraphicsLayerCA::moveOrCopyLayerAnimation(MoveOrCopy operation, const String& animationIdentifier, PlatformCALayer *fromLayer, PlatformCALayer *toLayer)
{
    RefPtr<PlatformCAAnimation> anim = fromLayer->animationForKey(animationIdentifier);
    if (!anim)
        return;

    switch (operation) {
    case Move:
        fromLayer->removeAnimationForKey(animationIdentifier);
        toLayer->addAnimationForKey(animationIdentifier, anim.get());
        break;

    case Copy:
        toLayer->addAnimationForKey(animationIdentifier, anim.get());
        break;
    }
}

void GraphicsLayerCA::moveOrCopyAnimationsForProperty(MoveOrCopy operation, AnimatedPropertyID property, PlatformCALayer *fromLayer, PlatformCALayer *toLayer)
{
    // Look for running animations affecting this property.
    AnimationsMap::const_iterator end = m_runningAnimations.end();
    for (AnimationsMap::const_iterator it = m_runningAnimations.begin(); it != end; ++it) {
        const Vector<LayerPropertyAnimation>& propertyAnimations = it->second;
        size_t numAnimations = propertyAnimations.size();
        for (size_t i = 0; i < numAnimations; ++i) {
            const LayerPropertyAnimation& currAnimation = propertyAnimations[i];
            if (currAnimation.m_property == property)
                moveOrCopyLayerAnimation(operation, animationIdentifier(currAnimation.m_name, currAnimation.m_property, currAnimation.m_index), fromLayer, toLayer);
        }
    }
}

void GraphicsLayerCA::setPreserves3D(bool preserves3D)
{
    if (preserves3D == m_preserves3D)
        return;

    GraphicsLayer::setPreserves3D(preserves3D);
    noteLayerPropertyChanged(Preserves3DChanged);
}

void GraphicsLayerCA::setMasksToBounds(bool masksToBounds)
{
    if (masksToBounds == m_masksToBounds)
        return;

    GraphicsLayer::setMasksToBounds(masksToBounds);
    noteLayerPropertyChanged(MasksToBoundsChanged);
}

void GraphicsLayerCA::setDrawsContent(bool drawsContent)
{
    if (drawsContent == m_drawsContent)
        return;

    GraphicsLayer::setDrawsContent(drawsContent);
    noteLayerPropertyChanged(DrawsContentChanged);
}

void GraphicsLayerCA::setAcceleratesDrawing(bool acceleratesDrawing)
{
    if (acceleratesDrawing == m_acceleratesDrawing)
        return;

    GraphicsLayer::setAcceleratesDrawing(acceleratesDrawing);
    noteLayerPropertyChanged(AcceleratesDrawingChanged);
}

void GraphicsLayerCA::setAllowTiledLayer(bool allowTiledLayer)
{
    if (allowTiledLayer == m_allowTiledLayer)
        return;

    m_allowTiledLayer = allowTiledLayer;
    
    // Handling this as a SizeChanged will cause use to switch in or out of tiled layer as needed
    noteLayerPropertyChanged(SizeChanged);
}

void GraphicsLayerCA::setBackgroundColor(const Color& color)
{
    if (m_backgroundColorSet && m_backgroundColor == color)
        return;

    GraphicsLayer::setBackgroundColor(color);

    m_contentsLayerHasBackgroundColor = true;
    noteLayerPropertyChanged(BackgroundColorChanged);
}

void GraphicsLayerCA::clearBackgroundColor()
{
    if (!m_backgroundColorSet)
        return;

    GraphicsLayer::clearBackgroundColor();
    m_contentsLayerHasBackgroundColor = false;
    noteLayerPropertyChanged(BackgroundColorChanged);
}

void GraphicsLayerCA::setContentsOpaque(bool opaque)
{
    if (m_contentsOpaque == opaque)
        return;

    GraphicsLayer::setContentsOpaque(opaque);
    noteLayerPropertyChanged(ContentsOpaqueChanged);
}

void GraphicsLayerCA::setBackfaceVisibility(bool visible)
{
    if (m_backfaceVisibility == visible)
        return;
    
    GraphicsLayer::setBackfaceVisibility(visible);
    noteLayerPropertyChanged(BackfaceVisibilityChanged);
}

void GraphicsLayerCA::setOpacity(float opacity)
{
    float clampedOpacity = max(0.0f, min(opacity, 1.0f));

    if (clampedOpacity == m_opacity)
        return;

    GraphicsLayer::setOpacity(clampedOpacity);
    noteLayerPropertyChanged(OpacityChanged);
}

void GraphicsLayerCA::setNeedsDisplay()
{
    FloatRect hugeRect(-numeric_limits<float>::max() / 2, -numeric_limits<float>::max() / 2,
                       numeric_limits<float>::max(), numeric_limits<float>::max());

    setNeedsDisplayInRect(hugeRect);
}

void GraphicsLayerCA::setNeedsDisplayInRect(const FloatRect& r)
{
    if (!drawsContent())
        return;

    FloatRect rect(r);
    FloatRect layerBounds(FloatPoint(), m_size);
    rect.intersect(layerBounds);
    if (rect.isEmpty())
        return;
    
    const size_t maxDirtyRects = 32;
    
    for (size_t i = 0; i < m_dirtyRects.size(); ++i) {
        if (m_dirtyRects[i].contains(rect))
            return;
    }
    
    if (m_dirtyRects.size() < maxDirtyRects)
        m_dirtyRects.append(rect);
    else
        m_dirtyRects[0].unite(rect);

    noteLayerPropertyChanged(DirtyRectsChanged);
}

void GraphicsLayerCA::setContentsNeedsDisplay()
{
    noteLayerPropertyChanged(ContentsNeedsDisplay);
}

void GraphicsLayerCA::setContentsRect(const IntRect& rect)
{
    if (rect == m_contentsRect)
        return;

    GraphicsLayer::setContentsRect(rect);
    noteLayerPropertyChanged(ContentsRectChanged);
}

bool GraphicsLayerCA::addAnimation(const KeyframeValueList& valueList, const IntSize& boxSize, const Animation* anim, const String& animationName, double timeOffset)
{
    ASSERT(!animationName.isEmpty());

    if (!anim || anim->isEmptyOrZeroDuration() || valueList.size() < 2)
        return false;

#if !HAVE_MODERN_QUARTZCORE
    // Older versions of QuartzCore do not handle opacity in transform layers properly, so we will
    // always do software animation in that case.
    if (valueList.property() == AnimatedPropertyOpacity)
        return false;
#endif

    // CoreAnimation does not handle the steps() timing function. Fall back
    // to software animation in that case.
    if (animationHasStepsTimingFunction(valueList, anim))
        return false;

    bool createdAnimations = false;
    if (valueList.property() == AnimatedPropertyWebkitTransform)
        createdAnimations = createTransformAnimationsFromKeyframes(valueList, anim, animationName, timeOffset, boxSize);
    else
        createdAnimations = createAnimationFromKeyframes(valueList, anim, animationName, timeOffset);

    if (createdAnimations)
        noteLayerPropertyChanged(AnimationChanged);
        
    return createdAnimations;
}

void GraphicsLayerCA::pauseAnimation(const String& animationName, double timeOffset)
{
    if (!animationIsRunning(animationName))
        return;

    AnimationsToProcessMap::iterator it = m_animationsToProcess.find(animationName);
    if (it != m_animationsToProcess.end()) {
        AnimationProcessingAction& processingInfo = it->second;
        // If an animation is scheduled to be removed, don't change the remove to a pause.
        if (processingInfo.action != Remove)
            processingInfo.action = Pause;
    } else
        m_animationsToProcess.add(animationName, AnimationProcessingAction(Pause, timeOffset));

    noteLayerPropertyChanged(AnimationChanged);
}

void GraphicsLayerCA::removeAnimation(const String& animationName)
{
    if (!animationIsRunning(animationName))
        return;

    m_animationsToProcess.add(animationName, AnimationProcessingAction(Remove));
    noteLayerPropertyChanged(AnimationChanged);
}

void GraphicsLayerCA::platformCALayerAnimationStarted(CFTimeInterval startTime)
{
    if (m_client)
        m_client->notifyAnimationStarted(this, startTime);
}

void GraphicsLayerCA::setContentsToImage(Image* image)
{
    if (image) {
        CGImageRef newImage = image->nativeImageForCurrentFrame();
        if (!newImage)
            return;

        // Check to see if the image changed; we have to do this because the call to
        // CGImageCreateCopyWithColorSpace() below can create a new image every time.
        if (m_uncorrectedContentsImage && m_uncorrectedContentsImage.get() == newImage)
            return;
        
        m_uncorrectedContentsImage = newImage;
        m_pendingContentsImage = newImage;

#if !PLATFORM(WIN)
        CGColorSpaceRef colorSpace = CGImageGetColorSpace(m_pendingContentsImage.get());

        static CGColorSpaceRef deviceRGB = CGColorSpaceCreateDeviceRGB();
        if (colorSpace && CFEqual(colorSpace, deviceRGB)) {
            // CoreGraphics renders images tagged with DeviceRGB using the color space of the main display. When we hand such
            // images to CA we need to tag them similarly so CA rendering matches CG rendering.
            static CGColorSpaceRef genericRGB = CGDisplayCopyColorSpace(kCGDirectMainDisplay);
            m_pendingContentsImage.adoptCF(CGImageCreateCopyWithColorSpace(m_pendingContentsImage.get(), genericRGB));
        }
#endif
        m_contentsLayerPurpose = ContentsLayerForImage;
        if (!m_contentsLayer)
            noteSublayersChanged();
    } else {
        m_uncorrectedContentsImage = 0;
        m_pendingContentsImage = 0;
        m_contentsLayerPurpose = NoContentsLayer;
        if (m_contentsLayer)
            noteSublayersChanged();
    }

    noteLayerPropertyChanged(ContentsImageChanged);
}

void GraphicsLayerCA::setContentsToMedia(PlatformLayer* mediaLayer)
{
    if (m_contentsLayer && mediaLayer == m_contentsLayer->platformLayer())
        return;
        
    // FIXME: The passed in layer might be a raw layer or an externally created 
    // PlatformCALayer. To determine this we attempt to get the
    // PlatformCALayer pointer. If this returns a null pointer we assume it's
    // raw. This test might be invalid if the raw layer is, for instance, the
    // PlatformCALayer is using a user data pointer in the raw layer, and
    // the creator of the raw layer is using it for some other purpose.
    // For now we don't support such a case.
    PlatformCALayer* platformCALayer = PlatformCALayer::platformCALayer(mediaLayer);
    m_contentsLayer = mediaLayer ? (platformCALayer ? platformCALayer : PlatformCALayer::create(mediaLayer, this)) : 0;
    m_contentsLayerPurpose = mediaLayer ? ContentsLayerForMedia : NoContentsLayer;

    noteSublayersChanged();
    noteLayerPropertyChanged(ContentsMediaLayerChanged);
}

void GraphicsLayerCA::setContentsToCanvas(PlatformLayer* canvasLayer)
{
    if (m_contentsLayer && canvasLayer == m_contentsLayer->platformLayer())
        return;
    
    // Create the PlatformCALayer to wrap the incoming layer
    m_contentsLayer = canvasLayer ? PlatformCALayer::create(canvasLayer, this) : 0;
    
    m_contentsLayerPurpose = canvasLayer ? ContentsLayerForCanvas : NoContentsLayer;

    noteSublayersChanged();
    noteLayerPropertyChanged(ContentsCanvasLayerChanged);
}
    
void GraphicsLayerCA::layerDidDisplay(PlatformLayer* layer)
{
    PlatformCALayer* currentLayer = PlatformCALayer::platformCALayer(layer);
    PlatformCALayer* sourceLayer;
    LayerMap* layerCloneMap;

    if (currentLayer == m_layer) {
        sourceLayer = m_layer.get();
        layerCloneMap = m_layerClones.get();
    } else if (currentLayer == m_contentsLayer) {
        sourceLayer = m_contentsLayer.get();
        layerCloneMap = m_contentsLayerClones.get();
    } else
        return;

    if (layerCloneMap) {
        LayerMap::const_iterator end = layerCloneMap->end();
        for (LayerMap::const_iterator it = layerCloneMap->begin(); it != end; ++it) {
            PlatformCALayer* currClone = it->second.get();
            if (!currClone)
                continue;

            if (currClone->contents() != sourceLayer->contents())
                currClone->setContents(sourceLayer->contents());
            else
                currClone->setContentsChanged();
        }
    }
}

void GraphicsLayerCA::syncCompositingState()
{
    recursiveCommitChanges();
}

void GraphicsLayerCA::syncCompositingStateForThisLayerOnly()
{
    commitLayerChangesBeforeSublayers();
    commitLayerChangesAfterSublayers();
}

void GraphicsLayerCA::recursiveCommitChanges()
{
    commitLayerChangesBeforeSublayers();

    if (m_maskLayer)
        static_cast<GraphicsLayerCA*>(m_maskLayer)->commitLayerChangesBeforeSublayers();

    const Vector<GraphicsLayer*>& childLayers = children();
    size_t numChildren = childLayers.size();
    for (size_t i = 0; i < numChildren; ++i) {
        GraphicsLayerCA* curChild = static_cast<GraphicsLayerCA*>(childLayers[i]);
        curChild->recursiveCommitChanges();
    }

    if (m_replicaLayer)
        static_cast<GraphicsLayerCA*>(m_replicaLayer)->recursiveCommitChanges();

    if (m_maskLayer)
        static_cast<GraphicsLayerCA*>(m_maskLayer)->commitLayerChangesAfterSublayers();

    commitLayerChangesAfterSublayers();
}

void GraphicsLayerCA::commitLayerChangesBeforeSublayers()
{
    if (!m_uncommittedChanges)
        return;

    // Need to handle Preserves3DChanged first, because it affects which layers subsequent properties are applied to
    if (m_uncommittedChanges & (Preserves3DChanged | ReplicatedLayerChanged))
        updateStructuralLayer();

    if (m_uncommittedChanges & NameChanged)
        updateLayerNames();

    if (m_uncommittedChanges & ContentsImageChanged) // Needs to happen before ChildrenChanged
        updateContentsImage();
        
    if (m_uncommittedChanges & ContentsMediaLayerChanged) // Needs to happen before ChildrenChanged
        updateContentsMediaLayer();
    
    if (m_uncommittedChanges & ContentsCanvasLayerChanged) // Needs to happen before ChildrenChanged
        updateContentsCanvasLayer();
    
    if (m_uncommittedChanges & BackgroundColorChanged) // Needs to happen before ChildrenChanged, and after updating image or video
        updateLayerBackgroundColor();

    if (m_uncommittedChanges & ChildrenChanged)
        updateSublayerList();

    if (m_uncommittedChanges & PositionChanged)
        updateLayerPosition();
    
    if (m_uncommittedChanges & AnchorPointChanged)
        updateAnchorPoint();
    
    if (m_uncommittedChanges & SizeChanged)
        updateLayerSize();

    if (m_uncommittedChanges & TransformChanged)
        updateTransform();

    if (m_uncommittedChanges & ChildrenTransformChanged)
        updateChildrenTransform();
    
    if (m_uncommittedChanges & MasksToBoundsChanged)
        updateMasksToBounds();
    
    if (m_uncommittedChanges & DrawsContentChanged)
        updateLayerDrawsContent();

    if (m_uncommittedChanges & ContentsOpaqueChanged)
        updateContentsOpaque();

    if (m_uncommittedChanges & BackfaceVisibilityChanged)
        updateBackfaceVisibility();

    if (m_uncommittedChanges & OpacityChanged)
        updateOpacityOnLayer();
    
    if (m_uncommittedChanges & AnimationChanged)
        updateLayerAnimations();
    
    if (m_uncommittedChanges & DirtyRectsChanged)
        repaintLayerDirtyRects();
    
    if (m_uncommittedChanges & ContentsRectChanged)
        updateContentsRect();

    if (m_uncommittedChanges & MaskLayerChanged)
        updateMaskLayer();

    if (m_uncommittedChanges & ContentsNeedsDisplay)
        updateContentsNeedsDisplay();
    
    if (m_uncommittedChanges & AcceleratesDrawingChanged)
        updateAcceleratesDrawing();
    
    if (m_uncommittedChanges & ContentsScaleChanged)
        updateContentsScale();
}

void GraphicsLayerCA::commitLayerChangesAfterSublayers()
{
    if (!m_uncommittedChanges)
        return;

    if (m_uncommittedChanges & ReplicatedLayerChanged)
        updateReplicatedLayers();

    m_uncommittedChanges = NoChange;
}

void GraphicsLayerCA::updateLayerNames()
{
    switch (structuralLayerPurpose()) {
    case StructuralLayerForPreserves3D:
        m_structuralLayer->setName("Transform layer " + name());
        break;
    case StructuralLayerForReplicaFlattening:
        m_structuralLayer->setName("Replica flattening layer " + name());
        break;
    case NoStructuralLayer:
        break;
    }
    m_layer->setName(name());
}

void GraphicsLayerCA::updateSublayerList()
{
    PlatformCALayerList newSublayers;
    const Vector<GraphicsLayer*>& childLayers = children();

    if (m_structuralLayer || m_contentsLayer || childLayers.size() > 0) {
        if (m_structuralLayer) {
            // Add the replica layer first.
            if (m_replicaLayer)
                newSublayers.append(static_cast<GraphicsLayerCA*>(m_replicaLayer)->primaryLayer());
            // Add the primary layer. Even if we have negative z-order children, the primary layer always comes behind.
            newSublayers.append(m_layer);
        } else if (m_contentsLayer) {
            // FIXME: add the contents layer in the correct order with negative z-order children.
            // This does not cause visible rendering issues because currently contents layers are only used
            // for replaced elements that don't have children.
            newSublayers.append(m_contentsLayer);
        }
        
        size_t numChildren = childLayers.size();
        for (size_t i = 0; i < numChildren; ++i) {
            GraphicsLayerCA* curChild = static_cast<GraphicsLayerCA*>(childLayers[i]);
            PlatformCALayer* childLayer = curChild->layerForSuperlayer();
            newSublayers.append(childLayer);
        }

        for (size_t i = 0; i < newSublayers.size(); --i)
            newSublayers[i]->removeFromSuperlayer();
    }

    if (m_structuralLayer) {
        m_structuralLayer->setSublayers(newSublayers);

        if (m_contentsLayer) {
            // If we have a transform layer, then the contents layer is parented in the 
            // primary layer (which is itself a child of the transform layer).
            m_layer->removeAllSublayers();
            m_layer->appendSublayer(m_contentsLayer.get());
        }
    } else
        m_layer->setSublayers(newSublayers);
}

void GraphicsLayerCA::updateLayerPosition()
{
    FloatSize usedSize = m_usingTiledLayer ? constrainedSize() : m_size;

    // Position is offset on the layer by the layer anchor point.
    FloatPoint posPoint(m_position.x() + m_anchorPoint.x() * usedSize.width(),
                          m_position.y() + m_anchorPoint.y() * usedSize.height());
    
    primaryLayer()->setPosition(posPoint);

    if (LayerMap* layerCloneMap = primaryLayerClones()) {
        LayerMap::const_iterator end = layerCloneMap->end();
        for (LayerMap::const_iterator it = layerCloneMap->begin(); it != end; ++it) {
            FloatPoint clonePosition = posPoint;
            if (m_replicaLayer && isReplicatedRootClone(it->first)) {
                // Maintain the special-case position for the root of a clone subtree,
                // which we set up in replicatedLayerRoot().
                clonePosition = positionForCloneRootLayer();
            }
            it->second->setPosition(clonePosition);
        }
    }
}

void GraphicsLayerCA::updateLayerSize()
{
    FloatRect rect(0, 0, m_size.width(), m_size.height());
    if (m_structuralLayer) {
        m_structuralLayer->setBounds(rect);
        
        if (LayerMap* layerCloneMap = m_structuralLayerClones.get()) {
            LayerMap::const_iterator end = layerCloneMap->end();
            for (LayerMap::const_iterator it = layerCloneMap->begin(); it != end; ++it)
                it->second->setBounds(rect);
        }

        // The anchor of the contents layer is always at 0.5, 0.5, so the position is center-relative.
        CGPoint centerPoint = CGPointMake(m_size.width() / 2.0f, m_size.height() / 2.0f);
        m_layer->setPosition(centerPoint);

        if (LayerMap* layerCloneMap = m_layerClones.get()) {
            LayerMap::const_iterator end = layerCloneMap->end();
            for (LayerMap::const_iterator it = layerCloneMap->begin(); it != end; ++it)
                it->second->setPosition(centerPoint);
        }
    }
    
    bool needTiledLayer = requiresTiledLayer(m_size);
    if (needTiledLayer != m_usingTiledLayer)
        swapFromOrToTiledLayer(needTiledLayer);
    
    if (m_usingTiledLayer) {
        FloatSize sizeToUse = constrainedSize();
        rect = CGRectMake(0, 0, sizeToUse.width(), sizeToUse.height());
    }
    
    m_layer->setBounds(rect);
    if (LayerMap* layerCloneMap = m_layerClones.get()) {
        LayerMap::const_iterator end = layerCloneMap->end();
        for (LayerMap::const_iterator it = layerCloneMap->begin(); it != end; ++it)
            it->second->setBounds(rect);
    }
    
    // Contents transform may depend on height.
    updateContentsTransform();

    // Note that we don't resize m_contentsLayer. It's up the caller to do that.

    // if we've changed the bounds, we need to recalculate the position
    // of the layer, taking anchor point into account.
    updateLayerPosition();
}

void GraphicsLayerCA::updateAnchorPoint()
{
    primaryLayer()->setAnchorPoint(m_anchorPoint);

    if (LayerMap* layerCloneMap = primaryLayerClones()) {
        LayerMap::const_iterator end = layerCloneMap->end();
        for (LayerMap::const_iterator it = layerCloneMap->begin(); it != end; ++it) {   
            PlatformCALayer* currLayer = it->second.get();
            currLayer->setAnchorPoint(m_anchorPoint);
        }
    }

    updateLayerPosition();
}

void GraphicsLayerCA::updateTransform()
{
    primaryLayer()->setTransform(m_transform);

    if (LayerMap* layerCloneMap = primaryLayerClones()) {
        LayerMap::const_iterator end = layerCloneMap->end();
        for (LayerMap::const_iterator it = layerCloneMap->begin(); it != end; ++it) {
            PlatformCALayer* currLayer = it->second.get();
            if (m_replicaLayer && isReplicatedRootClone(it->first)) {
                // Maintain the special-case transform for the root of a clone subtree,
                // which we set up in replicatedLayerRoot().
                currLayer->setTransform(TransformationMatrix());
            } else
                currLayer->setTransform(m_transform);
        }
    }
}

void GraphicsLayerCA::updateChildrenTransform()
{
    primaryLayer()->setSublayerTransform(m_childrenTransform);

    if (LayerMap* layerCloneMap = primaryLayerClones()) {
        LayerMap::const_iterator end = layerCloneMap->end();
        for (LayerMap::const_iterator it = layerCloneMap->begin(); it != end; ++it)
            it->second->setSublayerTransform(m_childrenTransform);
    }
}

void GraphicsLayerCA::updateMasksToBounds()
{
    m_layer->setMasksToBounds(m_masksToBounds);

    if (LayerMap* layerCloneMap = m_layerClones.get()) {
        LayerMap::const_iterator end = layerCloneMap->end();
        for (LayerMap::const_iterator it = layerCloneMap->begin(); it != end; ++it)
            it->second->setMasksToBounds(m_masksToBounds);
    }

    updateDebugIndicators();
}

void GraphicsLayerCA::updateContentsOpaque()
{
    m_layer.get()->setOpaque(m_contentsOpaque);

    if (LayerMap* layerCloneMap = m_layerClones.get()) {
        LayerMap::const_iterator end = layerCloneMap->end();
        for (LayerMap::const_iterator it = layerCloneMap->begin(); it != end; ++it)
            it->second->setOpaque(m_contentsOpaque);
    }
}

void GraphicsLayerCA::updateBackfaceVisibility()
{
    if (m_structuralLayer && structuralLayerPurpose() == StructuralLayerForReplicaFlattening) {
        m_structuralLayer->setDoubleSided(m_backfaceVisibility);

        if (LayerMap* layerCloneMap = m_structuralLayerClones.get()) {
            LayerMap::const_iterator end = layerCloneMap->end();
            for (LayerMap::const_iterator it = layerCloneMap->begin(); it != end; ++it)
                it->second->setDoubleSided(m_backfaceVisibility);
        }
    }

    m_layer->setDoubleSided(m_backfaceVisibility);

    if (LayerMap* layerCloneMap = m_layerClones.get()) {
        LayerMap::const_iterator end = layerCloneMap->end();
        for (LayerMap::const_iterator it = layerCloneMap->begin(); it != end; ++it)
            it->second->setDoubleSided(m_backfaceVisibility);
    }
}

void GraphicsLayerCA::updateStructuralLayer()
{
    ensureStructuralLayer(structuralLayerPurpose());
}

void GraphicsLayerCA::ensureStructuralLayer(StructuralLayerPurpose purpose)
{
    if (purpose == NoStructuralLayer) {
        if (m_structuralLayer) {
            // Replace the transformLayer in the parent with this layer.
            m_layer->removeFromSuperlayer();
 
            // If m_layer doesn't have a parent, it means it's the root layer and
            // is likely hosted by something that is not expecting to be changed
            ASSERT(m_structuralLayer->superlayer());
            m_structuralLayer->superlayer()->replaceSublayer(m_structuralLayer.get(), m_layer.get());

            moveOrCopyAnimationsForProperty(Move, AnimatedPropertyWebkitTransform, m_structuralLayer.get(), m_layer.get());
            moveOrCopyAnimationsForProperty(Move, AnimatedPropertyOpacity, m_structuralLayer.get(), m_layer.get());

            // Release the structural layer.
            m_structuralLayer = 0;

            // Update the properties of m_layer now that we no longer have a structural layer.
            updateLayerPosition();
            updateLayerSize();
            updateAnchorPoint();
            updateTransform();
            updateChildrenTransform();

            updateSublayerList();
            updateOpacityOnLayer();
        }
        return;
    }
    
    bool structuralLayerChanged = false;
    
    if (purpose == StructuralLayerForPreserves3D) {
        if (m_structuralLayer && m_structuralLayer->layerType() != PlatformCALayer::LayerTypeTransformLayer)
            m_structuralLayer = 0;
        
        if (!m_structuralLayer) {
            m_structuralLayer = PlatformCALayer::create(PlatformCALayer::LayerTypeTransformLayer, this);
            structuralLayerChanged = true;
        }
    } else {
        if (m_structuralLayer && m_structuralLayer->layerType() != PlatformCALayer::LayerTypeLayer)
            m_structuralLayer = 0;

        if (!m_structuralLayer) {
            m_structuralLayer = PlatformCALayer::create(PlatformCALayer::LayerTypeLayer, this);
            structuralLayerChanged = true;
        }
    }
    
    if (!structuralLayerChanged)
        return;
    
    updateLayerNames();

    // Update the properties of the structural layer.
    updateLayerPosition();
    updateLayerSize();
    updateAnchorPoint();
    updateTransform();
    updateChildrenTransform();
    updateBackfaceVisibility();
    
    // Set properties of m_layer to their default values, since these are expressed on on the structural layer.
    FloatPoint point(m_size.width() / 2.0f, m_size.height() / 2.0f);
    FloatPoint3D anchorPoint(0.5f, 0.5f, 0);
    m_layer->setPosition(point);
    m_layer->setAnchorPoint(anchorPoint);
    m_layer->setTransform(TransformationMatrix());
    m_layer->setOpacity(1);
    if (m_layerClones) {
        LayerMap::const_iterator end = m_layerClones->end();
        for (LayerMap::const_iterator it = m_layerClones->begin(); it != end; ++it) {
            PlatformCALayer* currLayer = it->second.get();
            currLayer->setPosition(point);
            currLayer->setAnchorPoint(anchorPoint);
            currLayer->setTransform(TransformationMatrix());
            currLayer->setOpacity(1);
        }
    }

    // Move this layer to be a child of the transform layer.
    // If m_layer doesn't have a parent, it means it's the root layer and
    // is likely hosted by something that is not expecting to be changed
    ASSERT(m_layer->superlayer());
    m_layer->superlayer()->replaceSublayer(m_layer.get(), m_structuralLayer.get());
    m_structuralLayer->appendSublayer(m_layer.get());

    moveOrCopyAnimationsForProperty(Move, AnimatedPropertyWebkitTransform, m_layer.get(), m_structuralLayer.get());
    moveOrCopyAnimationsForProperty(Move, AnimatedPropertyOpacity, m_layer.get(), m_structuralLayer.get());
    
    updateSublayerList();
    updateOpacityOnLayer();
}

GraphicsLayerCA::StructuralLayerPurpose GraphicsLayerCA::structuralLayerPurpose() const
{
    if (preserves3D())
        return StructuralLayerForPreserves3D;
    
    if (isReplicated())
        return StructuralLayerForReplicaFlattening;
    
    return NoStructuralLayer;
}

void GraphicsLayerCA::updateLayerDrawsContent()
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

void GraphicsLayerCA::updateAcceleratesDrawing()
{
    m_layer->setAcceleratesDrawing(m_acceleratesDrawing);
}
    
void GraphicsLayerCA::updateLayerBackgroundColor()
{
    if (!m_contentsLayer)
        return;

    // We never create the contents layer just for background color yet.
    if (m_backgroundColorSet)
        m_contentsLayer->setBackgroundColor(m_backgroundColor);
    else
        m_contentsLayer->setBackgroundColor(Color::transparent);
}

void GraphicsLayerCA::updateContentsImage()
{
    if (m_pendingContentsImage) {
        if (!m_contentsLayer.get()) {
            m_contentsLayer = PlatformCALayer::create(PlatformCALayer::LayerTypeLayer, this);
#ifndef NDEBUG
            m_contentsLayer->setName("Image Layer");
#endif
            setupContentsLayer(m_contentsLayer.get());
            // m_contentsLayer will be parented by updateSublayerList
        }

        // FIXME: maybe only do trilinear if the image is being scaled down,
        // but then what if the layer size changes?
#ifndef BUILDING_ON_LEOPARD
        m_contentsLayer->setMinificationFilter(PlatformCALayer::Trilinear);
#endif
        m_contentsLayer->setContents(m_pendingContentsImage.get());
        m_pendingContentsImage = 0;

        if (m_contentsLayerClones) {
            LayerMap::const_iterator end = m_contentsLayerClones->end();
            for (LayerMap::const_iterator it = m_contentsLayerClones->begin(); it != end; ++it)
                it->second->setContents(m_contentsLayer->contents());
        }
        
        updateContentsRect();
    } else {
        // No image.
        // m_contentsLayer will be removed via updateSublayerList.
        m_contentsLayer = 0;
    }
}

void GraphicsLayerCA::updateContentsMediaLayer()
{
    // Video layer was set as m_contentsLayer, and will get parented in updateSublayerList().
    if (m_contentsLayer) {
        setupContentsLayer(m_contentsLayer.get());
        updateContentsRect();
    }
}

void GraphicsLayerCA::updateContentsCanvasLayer()
{
    // CanvasLayer was set as m_contentsLayer, and will get parented in updateSublayerList().
    if (m_contentsLayer) {
        setupContentsLayer(m_contentsLayer.get());
        m_contentsLayer->setNeedsDisplay();
        updateContentsRect();
    }
}

void GraphicsLayerCA::updateContentsRect()
{
    if (!m_contentsLayer)
        return;

    FloatPoint point(m_contentsRect.x(), m_contentsRect.y());
    FloatRect rect(0, 0, m_contentsRect.width(), m_contentsRect.height());

    m_contentsLayer->setPosition(point);
    m_contentsLayer->setBounds(rect);

    if (m_contentsLayerClones) {
        LayerMap::const_iterator end = m_contentsLayerClones->end();
        for (LayerMap::const_iterator it = m_contentsLayerClones->begin(); it != end; ++it) {
            it->second->setPosition(point);
            it->second->setBounds(rect);
        }
    }
}

void GraphicsLayerCA::updateMaskLayer()
{
    PlatformCALayer* maskCALayer = m_maskLayer ? static_cast<GraphicsLayerCA*>(m_maskLayer)->primaryLayer() : 0;
    m_layer->setMask(maskCALayer);

    LayerMap* maskLayerCloneMap = m_maskLayer ? static_cast<GraphicsLayerCA*>(m_maskLayer)->primaryLayerClones() : 0;
    
    if (LayerMap* layerCloneMap = m_layerClones.get()) {
        LayerMap::const_iterator end = layerCloneMap->end();
        for (LayerMap::const_iterator it = layerCloneMap->begin(); it != end; ++it) {            
            PlatformCALayer* maskClone = maskLayerCloneMap ? maskLayerCloneMap->get(it->first).get() : 0;
            it->second->setMask(maskClone);
        }
    }
}

void GraphicsLayerCA::updateReplicatedLayers()
{
    // Clone the descendants of the replicated layer, and parent under us.
    ReplicaState replicaState(ReplicaState::ReplicaBranch);

    RefPtr<PlatformCALayer>replicaRoot = replicatedLayerRoot(replicaState);
    if (!replicaRoot)
        return;

    if (m_structuralLayer)
        m_structuralLayer->insertSublayer(replicaRoot.get(), 0);
    else
        m_layer->insertSublayer(replicaRoot.get(), 0);
}

// For now, this assumes that layers only ever have one replica, so replicaIndices contains only 0 and 1.
GraphicsLayerCA::CloneID GraphicsLayerCA::ReplicaState::cloneID() const
{
    size_t depth = m_replicaBranches.size();

    const size_t bitsPerUChar = sizeof(UChar) * 8;
    size_t vectorSize = (depth + bitsPerUChar - 1) / bitsPerUChar;
    
    Vector<UChar> result(vectorSize);
    result.fill(0);

    // Create a string from the bit sequence which we can use to identify the clone.
    // Note that the string may contain embedded nulls, but that's OK.
    for (size_t i = 0; i < depth; ++i) {
        UChar& currChar = result[i / bitsPerUChar];
        currChar = (currChar << 1) | m_replicaBranches[i];
    }
    
    return String::adopt(result);
}

PassRefPtr<PlatformCALayer> GraphicsLayerCA::replicatedLayerRoot(ReplicaState& replicaState)
{
    // Limit replica nesting, to avoid 2^N explosion of replica layers.
    if (!m_replicatedLayer || replicaState.replicaDepth() == ReplicaState::maxReplicaDepth)
        return 0;

    GraphicsLayerCA* replicatedLayer = static_cast<GraphicsLayerCA*>(m_replicatedLayer);
    
    RefPtr<PlatformCALayer> clonedLayerRoot = replicatedLayer->fetchCloneLayers(this, replicaState, RootCloneLevel);
    FloatPoint cloneRootPosition = replicatedLayer->positionForCloneRootLayer();

    // Replica root has no offset or transform
    clonedLayerRoot->setPosition(cloneRootPosition);
    clonedLayerRoot->setTransform(TransformationMatrix());

    return clonedLayerRoot;
}

void GraphicsLayerCA::updateLayerAnimations()
{
    if (m_animationsToProcess.size()) {
        AnimationsToProcessMap::const_iterator end = m_animationsToProcess.end();
        for (AnimationsToProcessMap::const_iterator it = m_animationsToProcess.begin(); it != end; ++it) {
            const String& currAnimationName = it->first;
            AnimationsMap::iterator animationIt = m_runningAnimations.find(currAnimationName);
            if (animationIt == m_runningAnimations.end())
                continue;

            const AnimationProcessingAction& processingInfo = it->second;
            const Vector<LayerPropertyAnimation>& animations = animationIt->second;
            for (size_t i = 0; i < animations.size(); ++i) {
                const LayerPropertyAnimation& currAnimation = animations[i];
                switch (processingInfo.action) {
                case Remove:
                    removeCAAnimationFromLayer(currAnimation.m_property, currAnimationName, currAnimation.m_index);
                    break;
                case Pause:
                    pauseCAAnimationOnLayer(currAnimation.m_property, currAnimationName, currAnimation.m_index, processingInfo.timeOffset);
                    break;
                }
            }

            if (processingInfo.action == Remove)
                m_runningAnimations.remove(currAnimationName);
        }
    
        m_animationsToProcess.clear();
    }
    
    size_t numAnimations;
    if ((numAnimations = m_uncomittedAnimations.size())) {
        for (size_t i = 0; i < numAnimations; ++i) {
            const LayerPropertyAnimation& pendingAnimation = m_uncomittedAnimations[i];
            setAnimationOnLayer(pendingAnimation.m_animation.get(), pendingAnimation.m_property, pendingAnimation.m_name, pendingAnimation.m_index, pendingAnimation.m_timeOffset);
            
            AnimationsMap::iterator it = m_runningAnimations.find(pendingAnimation.m_name);
            if (it == m_runningAnimations.end()) {
                Vector<LayerPropertyAnimation> animations;
                animations.append(pendingAnimation);
                m_runningAnimations.add(pendingAnimation.m_name, animations);
            } else {
                Vector<LayerPropertyAnimation>& animations = it->second;
                animations.append(pendingAnimation);
            }
        }
        
        m_uncomittedAnimations.clear();
    }
}

void GraphicsLayerCA::setAnimationOnLayer(PlatformCAAnimation* caAnim, AnimatedPropertyID property, const String& animationName, int index, double timeOffset)
{
    PlatformCALayer* layer = animatedLayer(property);
    
    if (timeOffset)
        caAnim->setBeginTime(CACurrentMediaTime() - timeOffset);

    String animationID = animationIdentifier(animationName, property, index);

    layer->removeAnimationForKey(animationID);
    layer->addAnimationForKey(animationID, caAnim);

    if (LayerMap* layerCloneMap = animatedLayerClones(property)) {
        LayerMap::const_iterator end = layerCloneMap->end();
        for (LayerMap::const_iterator it = layerCloneMap->begin(); it != end; ++it) {
            // Skip immediate replicas, since they move with the original.
            if (m_replicaLayer && isReplicatedRootClone(it->first))
                continue;

            it->second->removeAnimationForKey(animationID);
            it->second->addAnimationForKey(animationID, caAnim);
        }
    }
}

// Workaround for <rdar://problem/7311367>
static void bug7311367Workaround(PlatformCALayer* transformLayer, const TransformationMatrix& transform)
{
    if (!transformLayer)
        return;

    TransformationMatrix caTransform = transform;
    caTransform.setM41(caTransform.m41() + 1);
    transformLayer->setTransform(caTransform);

    caTransform.setM41(caTransform.m41() - 1);
    transformLayer->setTransform(caTransform);
}

bool GraphicsLayerCA::removeCAAnimationFromLayer(AnimatedPropertyID property, const String& animationName, int index)
{
    PlatformCALayer* layer = animatedLayer(property);

    String animationID = animationIdentifier(animationName, property, index);

    if (!layer->animationForKey(animationID))
        return false;
    
    layer->removeAnimationForKey(animationID);
    bug7311367Workaround(m_structuralLayer.get(), m_transform);

    if (LayerMap* layerCloneMap = animatedLayerClones(property)) {
        LayerMap::const_iterator end = layerCloneMap->end();
        for (LayerMap::const_iterator it = layerCloneMap->begin(); it != end; ++it) {
            // Skip immediate replicas, since they move with the original.
            if (m_replicaLayer && isReplicatedRootClone(it->first))
                continue;

            it->second ->removeAnimationForKey(animationID);
        }
    }
    return true;
}

void GraphicsLayerCA::pauseCAAnimationOnLayer(AnimatedPropertyID property, const String& animationName, int index, double timeOffset)
{
    PlatformCALayer* layer = animatedLayer(property);

    String animationID = animationIdentifier(animationName, property, index);

    RefPtr<PlatformCAAnimation> curAnim = layer->animationForKey(animationID);
    if (!curAnim)
        return;

    // Animations on the layer are immutable, so we have to clone and modify.
    RefPtr<PlatformCAAnimation> newAnim = curAnim->copy();

    newAnim->setSpeed(0);
    newAnim->setTimeOffset(timeOffset);
    
    layer->addAnimationForKey(animationID, newAnim.get()); // This will replace the running animation.

    // Pause the animations on the clones too.
    if (LayerMap* layerCloneMap = animatedLayerClones(property)) {
        LayerMap::const_iterator end = layerCloneMap->end();
        for (LayerMap::const_iterator it = layerCloneMap->begin(); it != end; ++it) {
            // Skip immediate replicas, since they move with the original.
            if (m_replicaLayer && isReplicatedRootClone(it->first))
                continue;
            it->second->addAnimationForKey(animationID, newAnim.get());
        }
    }
}

void GraphicsLayerCA::repaintLayerDirtyRects()
{
    if (!m_dirtyRects.size())
        return;

    for (size_t i = 0; i < m_dirtyRects.size(); ++i)
        m_layer->setNeedsDisplay(&(m_dirtyRects[i]));
    
    m_dirtyRects.clear();
}

void GraphicsLayerCA::updateContentsNeedsDisplay()
{
    if (m_contentsLayer)
        m_contentsLayer->setNeedsDisplay();
}

bool GraphicsLayerCA::createAnimationFromKeyframes(const KeyframeValueList& valueList, const Animation* animation, const String& animationName, double timeOffset)
{
    ASSERT(valueList.property() != AnimatedPropertyWebkitTransform);
    
    bool isKeyframe = valueList.size() > 2;
    bool valuesOK;
    
    bool additive = false;
    int animationIndex = 0;
    
    RefPtr<PlatformCAAnimation> caAnimation;
    
    if (isKeyframe) {
        caAnimation = createKeyframeAnimation(animation, valueList.property(), additive);
        valuesOK = setAnimationKeyframes(valueList, animation, caAnimation.get());
    } else {
        caAnimation = createBasicAnimation(animation, valueList.property(), additive);
        valuesOK = setAnimationEndpoints(valueList, animation, caAnimation.get());
    }
    
    if (!valuesOK)
        return false;

    m_uncomittedAnimations.append(LayerPropertyAnimation(caAnimation, animationName, valueList.property(), animationIndex, timeOffset));

    return true;
}

bool GraphicsLayerCA::createTransformAnimationsFromKeyframes(const KeyframeValueList& valueList, const Animation* animation, const String& animationName, double timeOffset, const IntSize& boxSize)
{
    ASSERT(valueList.property() == AnimatedPropertyWebkitTransform);

    TransformOperationList functionList;
    bool listsMatch, hasBigRotation;
    fetchTransformOperationList(valueList, functionList, listsMatch, hasBigRotation);

    // We need to fall back to software animation if we don't have setValueFunction:, and
    // we would need to animate each incoming transform function separately. This is the
    // case if we have a rotation >= 180 or we have more than one transform function.
    if ((hasBigRotation || functionList.size() > 1) && !PlatformCAAnimation::supportsValueFunction())
        return false;

    bool validMatrices = true;

    // If functionLists don't match we do a matrix animation, otherwise we do a component hardware animation.
    // Also, we can't do component animation unless we have valueFunction, so we need to do matrix animation
    // if that's not true as well.
    bool isMatrixAnimation = !listsMatch || !PlatformCAAnimation::supportsValueFunction();
    
    size_t numAnimations = isMatrixAnimation ? 1 : functionList.size();
    bool isKeyframe = valueList.size() > 2;
    
    // Iterate through the transform functions, sending an animation for each one.
    for (size_t animationIndex = 0; animationIndex < numAnimations; ++animationIndex) {
        TransformOperation::OperationType transformOp = isMatrixAnimation ? TransformOperation::MATRIX_3D : functionList[animationIndex];
        RefPtr<PlatformCAAnimation> caAnimation;

#if defined(BUILDING_ON_LEOPARD) || defined(BUILDING_ON_SNOW_LEOPARD) || PLATFORM(WIN)
        // CA applies animations in reverse order (<rdar://problem/7095638>) so we need the last one we add (per property)
        // to be non-additive.
        // FIXME: This fix has not been added to QuartzCore on Windows yet (<rdar://problem/9112233>) so we expect the
        // reversed animation behavior
        bool additive = animationIndex < (numAnimations - 1);
#else
        bool additive = animationIndex > 0;
#endif
        if (isKeyframe) {
            caAnimation = createKeyframeAnimation(animation, valueList.property(), additive);
            validMatrices = setTransformAnimationKeyframes(valueList, animation, caAnimation.get(), animationIndex, transformOp, isMatrixAnimation, boxSize);
        } else {
            caAnimation = createBasicAnimation(animation, valueList.property(), additive);
            validMatrices = setTransformAnimationEndpoints(valueList, animation, caAnimation.get(), animationIndex, transformOp, isMatrixAnimation, boxSize);
        }
        
        if (!validMatrices)
            break;
    
        m_uncomittedAnimations.append(LayerPropertyAnimation(caAnimation, animationName, valueList.property(), animationIndex, timeOffset));
    }

    return validMatrices;
}

PassRefPtr<PlatformCAAnimation> GraphicsLayerCA::createBasicAnimation(const Animation* anim, AnimatedPropertyID property, bool additive)
{
    RefPtr<PlatformCAAnimation> basicAnim = PlatformCAAnimation::create(PlatformCAAnimation::Basic, propertyIdToString(property));
    setupAnimation(basicAnim.get(), anim, additive);
    return basicAnim;
}

PassRefPtr<PlatformCAAnimation>GraphicsLayerCA::createKeyframeAnimation(const Animation* anim, AnimatedPropertyID property, bool additive)
{
    RefPtr<PlatformCAAnimation> keyframeAnim = PlatformCAAnimation::create(PlatformCAAnimation::Keyframe, propertyIdToString(property));
    setupAnimation(keyframeAnim.get(), anim, additive);
    return keyframeAnim;
}

void GraphicsLayerCA::setupAnimation(PlatformCAAnimation* propertyAnim, const Animation* anim, bool additive)
{
    double duration = anim->duration();
    if (duration <= 0)
        duration = cAnimationAlmostZeroDuration;

    float repeatCount = anim->iterationCount();
    if (repeatCount == Animation::IterationCountInfinite)
        repeatCount = numeric_limits<float>::max();
    else if (anim->direction() == Animation::AnimationDirectionAlternate)
        repeatCount /= 2;

    PlatformCAAnimation::FillModeType fillMode = PlatformCAAnimation::NoFillMode;
    switch (anim->fillMode()) {
    case AnimationFillModeNone:
        fillMode = PlatformCAAnimation::Forwards; // Use "forwards" rather than "removed" because the style system will remove the animation when it is finished. This avoids a flash.
        break;
    case AnimationFillModeBackwards:
        fillMode = PlatformCAAnimation::Both; // Use "both" rather than "backwards" because the style system will remove the animation when it is finished. This avoids a flash.
        break;
    case AnimationFillModeForwards:
        fillMode = PlatformCAAnimation::Forwards;
        break;
    case AnimationFillModeBoth:
        fillMode = PlatformCAAnimation::Both;
        break;
    }

    propertyAnim->setDuration(duration);
    propertyAnim->setRepeatCount(repeatCount);
    propertyAnim->setAutoreverses(anim->direction());
    propertyAnim->setRemovedOnCompletion(false);
    propertyAnim->setAdditive(additive);
    propertyAnim->setFillMode(fillMode);
}

const TimingFunction* GraphicsLayerCA::timingFunctionForAnimationValue(const AnimationValue* animValue, const Animation* anim)
{
    if (animValue->timingFunction())
        return animValue->timingFunction();
    if (anim->isTimingFunctionSet())
        return anim->timingFunction().get();
    
    return CubicBezierTimingFunction::defaultTimingFunction();
}

bool GraphicsLayerCA::setAnimationEndpoints(const KeyframeValueList& valueList, const Animation* anim, PlatformCAAnimation* basicAnim)
{
    switch (valueList.property()) {
    case AnimatedPropertyOpacity: {
        basicAnim->setFromValue(static_cast<const FloatAnimationValue*>(valueList.at(0))->value());
        basicAnim->setToValue(static_cast<const FloatAnimationValue*>(valueList.at(1))->value());
        break;
    }
    default:
        ASSERT_NOT_REACHED(); // we don't animate color yet
        break;
    }

    // This codepath is used for 2-keyframe animations, so we still need to look in the start
    // for a timing function.
    const TimingFunction* timingFunction = timingFunctionForAnimationValue(valueList.at(0), anim);
    if (timingFunction)
        basicAnim->setTimingFunction(timingFunction);

    return true;
}

bool GraphicsLayerCA::setAnimationKeyframes(const KeyframeValueList& valueList, const Animation* anim, PlatformCAAnimation* keyframeAnim)
{
    Vector<float> keyTimes;
    Vector<float> values;
    Vector<const TimingFunction*> timingFunctions;
    
    for (unsigned i = 0; i < valueList.size(); ++i) {
        const AnimationValue* curValue = valueList.at(i);
        keyTimes.append(curValue->keyTime());

        switch (valueList.property()) {
        case AnimatedPropertyOpacity: {
            const FloatAnimationValue* floatValue = static_cast<const FloatAnimationValue*>(curValue);
            values.append(floatValue->value());
            break;
        }
        default:
            ASSERT_NOT_REACHED(); // we don't animate color yet
            break;
        }

        timingFunctions.append(timingFunctionForAnimationValue(curValue, anim));
    }
    
    // We toss the last tfArray value because it has to one shorter than the others.
    timingFunctions.removeLast();

    keyframeAnim->setKeyTimes(keyTimes);
    keyframeAnim->setValues(values);
    keyframeAnim->setTimingFunctions(timingFunctions);
    
    return true;
}

bool GraphicsLayerCA::setTransformAnimationEndpoints(const KeyframeValueList& valueList, const Animation* anim, PlatformCAAnimation* basicAnim, int functionIndex, TransformOperation::OperationType transformOpType, bool isMatrixAnimation, const IntSize& boxSize)
{
    ASSERT(valueList.size() == 2);
    const TransformAnimationValue* startValue = static_cast<const TransformAnimationValue*>(valueList.at(0));
    const TransformAnimationValue* endValue = static_cast<const TransformAnimationValue*>(valueList.at(1));
    
    if (isMatrixAnimation) {
        TransformationMatrix fromTransform, toTransform;
        startValue->value()->apply(boxSize, fromTransform);
        endValue->value()->apply(boxSize, toTransform);

        // If any matrix is singular, CA won't animate it correctly. So fall back to software animation
        if (!fromTransform.isInvertible() || !toTransform.isInvertible())
            return false;
            
        basicAnim->setFromValue(fromTransform);
        basicAnim->setToValue(toTransform);
    } else {
        if (isTransformTypeNumber(transformOpType)) {
            float fromValue;
            getTransformFunctionValue(startValue->value()->at(functionIndex), transformOpType, boxSize, fromValue);
            basicAnim->setFromValue(fromValue);
            
            float toValue;
            getTransformFunctionValue(endValue->value()->at(functionIndex), transformOpType, boxSize, toValue);
            basicAnim->setToValue(toValue);
        } else if (isTransformTypeFloatPoint3D(transformOpType)) {
            FloatPoint3D fromValue;
            getTransformFunctionValue(startValue->value()->at(functionIndex), transformOpType, boxSize, fromValue);
            basicAnim->setFromValue(fromValue);
            
            FloatPoint3D toValue;
            getTransformFunctionValue(endValue->value()->at(functionIndex), transformOpType, boxSize, toValue);
            basicAnim->setToValue(toValue);
        } else {
            TransformationMatrix fromValue;
            getTransformFunctionValue(startValue->value()->at(functionIndex), transformOpType, boxSize, fromValue);
            basicAnim->setFromValue(fromValue);

            TransformationMatrix toValue;
            getTransformFunctionValue(endValue->value()->at(functionIndex), transformOpType, boxSize, toValue);
            basicAnim->setToValue(toValue);
        }
    }

    // This codepath is used for 2-keyframe animations, so we still need to look in the start
    // for a timing function.
    const TimingFunction* timingFunction = timingFunctionForAnimationValue(valueList.at(0), anim);
    basicAnim->setTimingFunction(timingFunction);

#if HAVE_MODERN_QUARTZCORE
    PlatformCAAnimation::ValueFunctionType valueFunction = getValueFunctionNameForTransformOperation(transformOpType);
    if (valueFunction != PlatformCAAnimation::NoValueFunction)
        basicAnim->setValueFunction(valueFunction);
#endif

    return true;
}

bool GraphicsLayerCA::setTransformAnimationKeyframes(const KeyframeValueList& valueList, const Animation* animation, PlatformCAAnimation* keyframeAnim, int functionIndex, TransformOperation::OperationType transformOpType, bool isMatrixAnimation, const IntSize& boxSize)
{
    Vector<float> keyTimes;
    Vector<float> floatValues;
    Vector<FloatPoint3D> floatPoint3DValues;
    Vector<TransformationMatrix> transformationMatrixValues;
    Vector<const TimingFunction*> timingFunctions;

    for (unsigned i = 0; i < valueList.size(); ++i) {
        const TransformAnimationValue* curValue = static_cast<const TransformAnimationValue*>(valueList.at(i));
        keyTimes.append(curValue->keyTime());

        if (isMatrixAnimation) {
            TransformationMatrix transform;
            curValue->value()->apply(boxSize, transform);

            // If any matrix is singular, CA won't animate it correctly. So fall back to software animation
            if (!transform.isInvertible())
                return false;

            transformationMatrixValues.append(transform);
        } else {
            const TransformOperation* transformOp = curValue->value()->at(functionIndex);
            if (isTransformTypeNumber(transformOpType)) {
                float value;
                getTransformFunctionValue(transformOp, transformOpType, boxSize, value);
                floatValues.append(value);
            } else if (isTransformTypeFloatPoint3D(transformOpType)) {
                FloatPoint3D value;
                getTransformFunctionValue(transformOp, transformOpType, boxSize, value);
                floatPoint3DValues.append(value);
            } else {
                TransformationMatrix value;
                getTransformFunctionValue(transformOp, transformOpType, boxSize, value);
                transformationMatrixValues.append(value);
            }
        }

        const TimingFunction* timingFunction = timingFunctionForAnimationValue(curValue, animation);
        timingFunctions.append(timingFunction);
    }
    
    // We toss the last tfArray value because it has to one shorter than the others.
    timingFunctions.removeLast();

    keyframeAnim->setKeyTimes(keyTimes);
    
    if (isTransformTypeNumber(transformOpType))
        keyframeAnim->setValues(floatValues);
    else if (isTransformTypeFloatPoint3D(transformOpType))
        keyframeAnim->setValues(floatPoint3DValues);
    else
        keyframeAnim->setValues(transformationMatrixValues);
        
    keyframeAnim->setTimingFunctions(timingFunctions);

#if HAVE_MODERN_QUARTZCORE
    PlatformCAAnimation::ValueFunctionType valueFunction = getValueFunctionNameForTransformOperation(transformOpType);
    if (valueFunction != PlatformCAAnimation::NoValueFunction)
        keyframeAnim->setValueFunction(valueFunction);
#endif
    return true;
}

void GraphicsLayerCA::suspendAnimations(double time)
{
    double t = PlatformCALayer::currentTimeToMediaTime(time ? time : currentTime());
    primaryLayer()->setSpeed(0);
    primaryLayer()->setTimeOffset(t);

    // Suspend the animations on the clones too.
    if (LayerMap* layerCloneMap = primaryLayerClones()) {
        LayerMap::const_iterator end = layerCloneMap->end();
        for (LayerMap::const_iterator it = layerCloneMap->begin(); it != end; ++it) {
            it->second->setSpeed(0);
            it->second->setTimeOffset(t);
        }
    }
}

void GraphicsLayerCA::resumeAnimations()
{
    primaryLayer()->setSpeed(1);
    primaryLayer()->setTimeOffset(0);

    // Resume the animations on the clones too.
    if (LayerMap* layerCloneMap = primaryLayerClones()) {
        LayerMap::const_iterator end = layerCloneMap->end();
        for (LayerMap::const_iterator it = layerCloneMap->begin(); it != end; ++it) {
            it->second->setSpeed(1);
            it->second->setTimeOffset(0);
        }
    }
}

PlatformCALayer* GraphicsLayerCA::hostLayerForSublayers() const
{
    return m_structuralLayer.get() ? m_structuralLayer.get() : m_layer.get(); 
}

PlatformCALayer* GraphicsLayerCA::layerForSuperlayer() const
{
    return m_structuralLayer ? m_structuralLayer.get() : m_layer.get();
}

PlatformCALayer* GraphicsLayerCA::animatedLayer(AnimatedPropertyID property) const
{
    return (property == AnimatedPropertyBackgroundColor) ? m_contentsLayer.get() : primaryLayer();
}

GraphicsLayerCA::LayerMap* GraphicsLayerCA::animatedLayerClones(AnimatedPropertyID property) const
{
    return (property == AnimatedPropertyBackgroundColor) ? m_contentsLayerClones.get() : primaryLayerClones();
}

void GraphicsLayerCA::setContentsScale(float scale)
{
    float newScale = clampedContentsScaleForScale(scale);
    if (newScale == m_contentsScale)
        return;

    m_contentsScale = newScale;
    noteLayerPropertyChanged(ContentsScaleChanged);
}

float GraphicsLayerCA::clampedContentsScaleForScale(float scale) const
{
    // Define some limits as a sanity check for the incoming scale value
    // those too small to see.
    const float maxScale = 5.0f;
    const float minScale = 0.01f;
    
    // Avoid very slight scale changes that would be doing extra work for no benefit
    const float maxAllowableDelta = 0.05f;

    // Clamp
    float result = max(minScale, min(scale, maxScale));

    // If it hasn't changed much, don't do any work
    return ((fabs(result - m_contentsScale) / m_contentsScale) < maxAllowableDelta) ? m_contentsScale : result;
}

void GraphicsLayerCA::updateContentsScale()
{
    bool needTiledLayer = requiresTiledLayer(m_size);
    if (needTiledLayer != m_usingTiledLayer)
        swapFromOrToTiledLayer(needTiledLayer);

    m_layer->setContentsScale(m_contentsScale);
    if (drawsContent())
        m_layer->setNeedsDisplay();
}

void GraphicsLayerCA::setDebugBackgroundColor(const Color& color)
{    
    if (color.isValid())
        m_layer->setBackgroundColor(color);
    else
        m_layer->setBackgroundColor(Color::transparent);
}

void GraphicsLayerCA::setDebugBorder(const Color& color, float borderWidth)
{    
    if (color.isValid()) {
        m_layer->setBorderColor(color);
        m_layer->setBorderWidth(borderWidth);
    } else {
        m_layer->setBorderColor(Color::transparent);
        m_layer->setBorderWidth(0);
    }
}

FloatSize GraphicsLayerCA::constrainedSize() const
{
    FloatSize constrainedSize = m_size;
#if defined(BUILDING_ON_LEOPARD) || defined(BUILDING_ON_SNOW_LEOPARD)
    float tileColumns = ceilf(m_size.width() / kTiledLayerTileSize);
    float tileRows = ceilf(m_size.height() / kTiledLayerTileSize);
    double numTiles = tileColumns * tileRows;
    
    const unsigned cMaxTileCount = 512;
    while (numTiles > cMaxTileCount) {
        // Constrain the wider dimension.
        if (constrainedSize.width() >= constrainedSize.height()) {
            tileColumns = max(floorf(cMaxTileCount / tileRows), 1.0f);
            constrainedSize.setWidth(tileColumns * kTiledLayerTileSize);
        } else {
            tileRows = max(floorf(cMaxTileCount / tileColumns), 1.0f);
            constrainedSize.setHeight(tileRows * kTiledLayerTileSize);
        }
        numTiles = tileColumns * tileRows;
    }
#endif
    return constrainedSize;
}

bool GraphicsLayerCA::requiresTiledLayer(const FloatSize& size) const
{
    if (!m_drawsContent || !m_allowTiledLayer)
        return false;

    // FIXME: catch zero-size height or width here (or earlier)?
    return size.width() > cMaxPixelDimension || size.height() > cMaxPixelDimension;
}

void GraphicsLayerCA::swapFromOrToTiledLayer(bool useTiledLayer)
{
    ASSERT(useTiledLayer != m_usingTiledLayer);
    RefPtr<PlatformCALayer> oldLayer = m_layer;
    
    m_layer = PlatformCALayer::create(useTiledLayer ? PlatformCALayer::LayerTypeWebTiledLayer : PlatformCALayer::LayerTypeWebLayer, this);
    m_layer->setContentsScale(m_contentsScale);

    m_usingTiledLayer = useTiledLayer;
    
    if (useTiledLayer) {
#if !HAVE_MODERN_QUARTZCORE
        // Tiled layer has issues with flipped coordinates.
        setContentsOrientation(CompositingCoordinatesTopDown);
#endif
    } else {
#if !HAVE_MODERN_QUARTZCORE
        setContentsOrientation(GraphicsLayerCA::defaultContentsOrientation());
#endif
    }

    m_layer->adoptSublayers(oldLayer.get());
    
    // FIXME: Skip this step if we don't have a superlayer. This is problably a benign
    // case that happens while restructuring the layer tree.
    ASSERT(oldLayer->superlayer());
    if (oldLayer->superlayer())
        oldLayer->superlayer()->replaceSublayer(oldLayer.get(), m_layer.get());

    updateContentsTransform();

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
    String name = String::format("CALayer(%p) GraphicsLayer(%p) ", m_layer.get(), this) + m_name;
    m_layer->setName(name);
#endif

    // move over animations
    moveOrCopyAnimationsForProperty(Move, AnimatedPropertyWebkitTransform, oldLayer.get(), m_layer.get());
    moveOrCopyAnimationsForProperty(Move, AnimatedPropertyOpacity, oldLayer.get(), m_layer.get());
    moveOrCopyAnimationsForProperty(Move, AnimatedPropertyBackgroundColor, oldLayer.get(), m_layer.get());
    
    // need to tell new layer to draw itself
    setNeedsDisplay();
    
    updateDebugIndicators();
}

GraphicsLayer::CompositingCoordinatesOrientation GraphicsLayerCA::defaultContentsOrientation() const
{
#if !HAVE_MODERN_QUARTZCORE
    // Older QuartzCore does not support -geometryFlipped, so we manually flip the root
    // layer geometry, and then flip the contents of each layer back so that the CTM for CG
    // is unflipped, allowing it to do the correct font auto-hinting.
    return CompositingCoordinatesBottomUp;
#else
    return CompositingCoordinatesTopDown;
#endif
}

void GraphicsLayerCA::updateContentsTransform()
{
#if !HAVE_MODERN_QUARTZCORE
    if (contentsOrientation() == CompositingCoordinatesBottomUp) {
        CGAffineTransform contentsTransform = CGAffineTransformMakeScale(1, -1);
        contentsTransform = CGAffineTransformTranslate(contentsTransform, 0, -m_layer->bounds().size().height());
        m_layer->setContentsTransform(TransformationMatrix(contentsTransform));
    }
#endif
}

void GraphicsLayerCA::setupContentsLayer(PlatformCALayer* contentsLayer)
{
    // Turn off implicit animations on the inner layer.
    contentsLayer->setMasksToBounds(true);

    if (defaultContentsOrientation() == CompositingCoordinatesBottomUp) {
        TransformationMatrix flipper(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, -1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
        contentsLayer->setTransform(flipper);
        contentsLayer->setAnchorPoint(FloatPoint3D(0, 1, 0));
    } else
        contentsLayer->setAnchorPoint(FloatPoint3D());

    if (showDebugBorders()) {
        contentsLayer->setBorderColor(Color(0, 0, 128, 180));
        contentsLayer->setBorderWidth(1.0f);
    }
}

PassRefPtr<PlatformCALayer> GraphicsLayerCA::findOrMakeClone(CloneID cloneID, PlatformCALayer *sourceLayer, LayerMap* clones, CloneLevel cloneLevel)
{
    if (!sourceLayer)
        return 0;

    RefPtr<PlatformCALayer> resultLayer;

    // Add with a dummy value to get an iterator for the insertion position, and a boolean that tells
    // us whether there's an item there. This technique avoids two hash lookups.
    RefPtr<PlatformCALayer> dummy;
    pair<LayerMap::iterator, bool> addResult = clones->add(cloneID, dummy);
    if (!addResult.second) {
        // Value was not added, so it exists already.
        resultLayer = addResult.first->second.get();
    } else {
        resultLayer = cloneLayer(sourceLayer, cloneLevel);
#ifndef NDEBUG
        resultLayer->setName(String::format("Clone %d of layer %p", cloneID[0U], sourceLayer));
#endif
        addResult.first->second = resultLayer;
    }

    return resultLayer;
}   

void GraphicsLayerCA::ensureCloneLayers(CloneID cloneID, RefPtr<PlatformCALayer>& primaryLayer, RefPtr<PlatformCALayer>& structuralLayer, RefPtr<PlatformCALayer>& contentsLayer, CloneLevel cloneLevel)
{
    structuralLayer = 0;
    contentsLayer = 0;

    if (!m_layerClones)
        m_layerClones = adoptPtr(new LayerMap);

    if (!m_structuralLayerClones && m_structuralLayer)
        m_structuralLayerClones = adoptPtr(new LayerMap);

    if (!m_contentsLayerClones && m_contentsLayer)
        m_contentsLayerClones = adoptPtr(new LayerMap);

    primaryLayer = findOrMakeClone(cloneID, m_layer.get(), m_layerClones.get(), cloneLevel);
    structuralLayer = findOrMakeClone(cloneID, m_structuralLayer.get(), m_structuralLayerClones.get(), cloneLevel);
    contentsLayer = findOrMakeClone(cloneID, m_contentsLayer.get(), m_contentsLayerClones.get(), cloneLevel);
}

void GraphicsLayerCA::removeCloneLayers()
{
    m_layerClones = nullptr;
    m_structuralLayerClones = nullptr;
    m_contentsLayerClones = nullptr;
}

FloatPoint GraphicsLayerCA::positionForCloneRootLayer() const
{
    // This can get called during a sync when we've just removed the m_replicaLayer.
    if (!m_replicaLayer)
        return FloatPoint();

    FloatPoint replicaPosition = m_replicaLayer->replicatedLayerPosition();
    return FloatPoint(replicaPosition.x() + m_anchorPoint.x() * m_size.width(),
                      replicaPosition.y() + m_anchorPoint.y() * m_size.height());
}

void GraphicsLayerCA::propagateLayerChangeToReplicas()
{
    for (GraphicsLayer* currLayer = this; currLayer; currLayer = currLayer->parent()) {
        GraphicsLayerCA* currLayerCA = static_cast<GraphicsLayerCA*>(currLayer);
        if (!currLayerCA->hasCloneLayers())
            break;

        if (currLayerCA->replicaLayer())
            static_cast<GraphicsLayerCA*>(currLayerCA->replicaLayer())->noteLayerPropertyChanged(ReplicatedLayerChanged);
    }
}

PassRefPtr<PlatformCALayer> GraphicsLayerCA::fetchCloneLayers(GraphicsLayer* replicaRoot, ReplicaState& replicaState, CloneLevel cloneLevel)
{
    RefPtr<PlatformCALayer> primaryLayer;
    RefPtr<PlatformCALayer> structuralLayer;
    RefPtr<PlatformCALayer> contentsLayer;
    ensureCloneLayers(replicaState.cloneID(), primaryLayer, structuralLayer, contentsLayer, cloneLevel);

    if (m_maskLayer) {
        RefPtr<PlatformCALayer> maskClone = static_cast<GraphicsLayerCA*>(m_maskLayer)->fetchCloneLayers(replicaRoot, replicaState, IntermediateCloneLevel);
        primaryLayer->setMask(maskClone.get());
    }

    if (m_replicatedLayer) {
        // We are a replica being asked for clones of our layers.
        RefPtr<PlatformCALayer> replicaRoot = replicatedLayerRoot(replicaState);
        if (!replicaRoot)
            return 0;

        if (structuralLayer) {
            structuralLayer->insertSublayer(replicaRoot.get(), 0);
            return structuralLayer;
        }
        
        primaryLayer->insertSublayer(replicaRoot.get(), 0);
        return primaryLayer;
    }

    const Vector<GraphicsLayer*>& childLayers = children();
    Vector<RefPtr<PlatformCALayer> > clonalSublayers;

    RefPtr<PlatformCALayer> replicaLayer;
    
    if (m_replicaLayer && m_replicaLayer != replicaRoot) {
        // We have nested replicas. Ask the replica layer for a clone of its contents.
        replicaState.setBranchType(ReplicaState::ReplicaBranch);
        replicaLayer = static_cast<GraphicsLayerCA*>(m_replicaLayer)->fetchCloneLayers(replicaRoot, replicaState, RootCloneLevel);
        replicaState.setBranchType(ReplicaState::ChildBranch);
    }
    
    if (replicaLayer || structuralLayer || contentsLayer || childLayers.size() > 0) {
        if (structuralLayer) {
            // Replicas render behind the actual layer content.
            if (replicaLayer)
                clonalSublayers.append(replicaLayer);
                
            // Add the primary layer next. Even if we have negative z-order children, the primary layer always comes behind.
            clonalSublayers.append(primaryLayer);
        } else if (contentsLayer) {
            // FIXME: add the contents layer in the correct order with negative z-order children.
            // This does not cause visible rendering issues because currently contents layers are only used
            // for replaced elements that don't have children.
            clonalSublayers.append(contentsLayer);
        }
        
        replicaState.push(ReplicaState::ChildBranch);

        size_t numChildren = childLayers.size();
        for (size_t i = 0; i < numChildren; ++i) {
            GraphicsLayerCA* curChild = static_cast<GraphicsLayerCA*>(childLayers[i]);

            RefPtr<PlatformCALayer> childLayer = curChild->fetchCloneLayers(replicaRoot, replicaState, IntermediateCloneLevel);
            if (childLayer)
                clonalSublayers.append(childLayer);
        }

        replicaState.pop();

        for (size_t i = 0; i < clonalSublayers.size(); ++i)
            clonalSublayers[i]->removeFromSuperlayer();
    }
    
    RefPtr<PlatformCALayer> result;
    if (structuralLayer) {
        structuralLayer->setSublayers(clonalSublayers);

        if (contentsLayer) {
            // If we have a transform layer, then the contents layer is parented in the 
            // primary layer (which is itself a child of the transform layer).
            primaryLayer->removeAllSublayers();
            primaryLayer->appendSublayer(contentsLayer.get());
        }

        result = structuralLayer;
    } else {
        primaryLayer->setSublayers(clonalSublayers);
        result = primaryLayer;
    }

    return result;
}

PassRefPtr<PlatformCALayer> GraphicsLayerCA::cloneLayer(PlatformCALayer *layer, CloneLevel cloneLevel)
{
    PlatformCALayer::LayerType layerType = (layer->layerType() == PlatformCALayer::LayerTypeTransformLayer) ? 
                                                PlatformCALayer::LayerTypeTransformLayer : PlatformCALayer::LayerTypeLayer;
    RefPtr<PlatformCALayer> newLayer = PlatformCALayer::create(layerType, this);
    
    newLayer->setPosition(layer->position());
    newLayer->setBounds(layer->bounds());
    newLayer->setAnchorPoint(layer->anchorPoint());
    newLayer->setTransform(layer->transform());
    newLayer->setSublayerTransform(layer->sublayerTransform());
    newLayer->setContents(layer->contents());
    newLayer->setMasksToBounds(layer->masksToBounds());
    newLayer->setDoubleSided(layer->isDoubleSided());
    newLayer->setOpaque(layer->isOpaque());
    newLayer->setBackgroundColor(layer->backgroundColor());
    newLayer->setContentsScale(layer->contentsScale());

    if (cloneLevel == IntermediateCloneLevel) {
        newLayer->setOpacity(layer->opacity());
        moveOrCopyAnimationsForProperty(Copy, AnimatedPropertyWebkitTransform, layer, newLayer.get());
        moveOrCopyAnimationsForProperty(Copy, AnimatedPropertyOpacity, layer, newLayer.get());
    }
    
    if (showDebugBorders()) {
        newLayer->setBorderColor(Color(255, 122, 251));
        newLayer->setBorderWidth(2);
    }
    
    return newLayer;
}

void GraphicsLayerCA::setOpacityInternal(float accumulatedOpacity)
{
    LayerMap* layerCloneMap = 0;
    
    if (preserves3D()) {
        m_layer->setOpacity(accumulatedOpacity);
        layerCloneMap = m_layerClones.get();
    } else {
        primaryLayer()->setOpacity(accumulatedOpacity);
        layerCloneMap = primaryLayerClones();
    }

    if (layerCloneMap) {
        LayerMap::const_iterator end = layerCloneMap->end();
        for (LayerMap::const_iterator it = layerCloneMap->begin(); it != end; ++it) {
            if (m_replicaLayer && isReplicatedRootClone(it->first))
                continue;
            it->second->setOpacity(m_opacity);
        }
    }
}

void GraphicsLayerCA::updateOpacityOnLayer()
{
#if !HAVE_MODERN_QUARTZCORE
    // Distribute opacity either to our own layer or to our children. We pass in the 
    // contribution from our parent(s).
    distributeOpacity(parent() ? parent()->accumulatedOpacity() : 1);
#else
    primaryLayer()->setOpacity(m_opacity);

    if (LayerMap* layerCloneMap = primaryLayerClones()) {
        LayerMap::const_iterator end = layerCloneMap->end();
        for (LayerMap::const_iterator it = layerCloneMap->begin(); it != end; ++it) {
            if (m_replicaLayer && isReplicatedRootClone(it->first))
                continue;

            it->second->setOpacity(m_opacity);
        }
        
    }
#endif
}

void GraphicsLayerCA::noteSublayersChanged()
{
    noteLayerPropertyChanged(ChildrenChanged);
    propagateLayerChangeToReplicas();
}

void GraphicsLayerCA::noteLayerPropertyChanged(LayerChangeFlags flags)
{
    if (!m_uncommittedChanges && m_client)
        m_client->notifySyncRequired(this);

    m_uncommittedChanges |= flags;
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
