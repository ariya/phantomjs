/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

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
#include "GraphicsLayerTextureMapper.h"

#include "GraphicsContext.h"
#include "GraphicsLayerAnimation.h"
#include "GraphicsLayerFactory.h"
#include "ImageBuffer.h"
#include <wtf/CurrentTime.h>

#if USE(TEXTURE_MAPPER)

namespace WebCore {

TextureMapperLayer* toTextureMapperLayer(GraphicsLayer* layer)
{
    return layer ? toGraphicsLayerTextureMapper(layer)->layer() : 0;
}

PassOwnPtr<GraphicsLayer> GraphicsLayer::create(GraphicsLayerFactory* factory, GraphicsLayerClient* client)
{
    if (!factory)
        return adoptPtr(new GraphicsLayerTextureMapper(client));

    return factory->createGraphicsLayer(client);
}

PassOwnPtr<GraphicsLayer> GraphicsLayer::create(GraphicsLayerClient* client)
{
    return adoptPtr(new GraphicsLayerTextureMapper(client));
}

GraphicsLayerTextureMapper::GraphicsLayerTextureMapper(GraphicsLayerClient* client)
    : GraphicsLayer(client)
    , m_layer(adoptPtr(new TextureMapperLayer()))
    , m_compositedNativeImagePtr(0)
    , m_changeMask(NoChanges)
    , m_needsDisplay(false)
    , m_fixedToViewport(false)
    , m_debugBorderWidth(0)
    , m_contentsLayer(0)
    , m_animationStartTime(0)
    , m_isScrollable(false)
{
}

void GraphicsLayerTextureMapper::notifyChange(ChangeMask changeMask)
{
    m_changeMask |= changeMask;
    if (!client())
        return;
    client()->notifyFlushRequired(this);
}

void GraphicsLayerTextureMapper::setName(const String& name)
{
    GraphicsLayer::setName(name);
}

GraphicsLayerTextureMapper::~GraphicsLayerTextureMapper()
{
    if (m_contentsLayer)
        m_contentsLayer->setClient(0);

    willBeDestroyed();
}

void GraphicsLayerTextureMapper::willBeDestroyed()
{
    GraphicsLayer::willBeDestroyed();
}

/* \reimp (GraphicsLayer.h): The current size might change, thus we need to update the whole display.
*/
void GraphicsLayerTextureMapper::setNeedsDisplay()
{
    if (!drawsContent())
        return;

    m_needsDisplay = true;
    notifyChange(DisplayChange);
    addRepaintRect(FloatRect(FloatPoint(), m_size));
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setContentsNeedsDisplay()
{
    notifyChange(DisplayChange);
    addRepaintRect(contentsRect());
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setNeedsDisplayInRect(const FloatRect& rect)
{
    if (!drawsContent())
        return;

    if (m_needsDisplay)
        return;
    m_needsDisplayRect.unite(rect);
    notifyChange(DisplayChange);
    addRepaintRect(rect);
}

/* \reimp (GraphicsLayer.h)
*/
bool GraphicsLayerTextureMapper::setChildren(const Vector<GraphicsLayer*>& children)
{
    if (GraphicsLayer::setChildren(children)) {
        notifyChange(ChildrenChange);
        return true;
    }
    return false;
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::addChild(GraphicsLayer* layer)
{
    notifyChange(ChildrenChange);
    GraphicsLayer::addChild(layer);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::addChildAtIndex(GraphicsLayer* layer, int index)
{
    GraphicsLayer::addChildAtIndex(layer, index);
    notifyChange(ChildrenChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::addChildAbove(GraphicsLayer* layer, GraphicsLayer* sibling)
{
    GraphicsLayer::addChildAbove(layer, sibling);
    notifyChange(ChildrenChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::addChildBelow(GraphicsLayer* layer, GraphicsLayer* sibling)
{
    GraphicsLayer::addChildBelow(layer, sibling);
    notifyChange(ChildrenChange);
}

/* \reimp (GraphicsLayer.h)
*/
bool GraphicsLayerTextureMapper::replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild)
{
    if (GraphicsLayer::replaceChild(oldChild, newChild)) {
        notifyChange(ChildrenChange);
        return true;
    }
    return false;
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setMaskLayer(GraphicsLayer* value)
{
    if (value == maskLayer())
        return;
    GraphicsLayer::setMaskLayer(value);
    notifyChange(MaskLayerChange);

    if (!value)
        return;
    value->setSize(size());
    value->setContentsVisible(contentsAreVisible());
}


/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setReplicatedByLayer(GraphicsLayer* value)
{
    if (value == replicaLayer())
        return;
    GraphicsLayer::setReplicatedByLayer(value);
    notifyChange(ReplicaLayerChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setPosition(const FloatPoint& value)
{
    if (value == position())
        return;
    GraphicsLayer::setPosition(value);
    notifyChange(PositionChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setAnchorPoint(const FloatPoint3D& value)
{
    if (value == anchorPoint())
        return;
    GraphicsLayer::setAnchorPoint(value);
    notifyChange(AnchorPointChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setSize(const FloatSize& value)
{
    if (value == size())
        return;

    GraphicsLayer::setSize(value);
    if (maskLayer())
        maskLayer()->setSize(value);
    notifyChange(SizeChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setTransform(const TransformationMatrix& value)
{
    if (value == transform())
        return;

    GraphicsLayer::setTransform(value);
    notifyChange(TransformChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setChildrenTransform(const TransformationMatrix& value)
{
    if (value == childrenTransform())
        return;
    GraphicsLayer::setChildrenTransform(value);
    notifyChange(ChildrenTransformChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setPreserves3D(bool value)
{
    if (value == preserves3D())
        return;
    GraphicsLayer::setPreserves3D(value);
    notifyChange(Preserves3DChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setMasksToBounds(bool value)
{
    if (value == masksToBounds())
        return;
    GraphicsLayer::setMasksToBounds(value);
    notifyChange(MasksToBoundsChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setDrawsContent(bool value)
{
    if (value == drawsContent())
        return;
    GraphicsLayer::setDrawsContent(value);
    notifyChange(DrawsContentChange);

    if (value)
        setNeedsDisplay();
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setContentsVisible(bool value)
{
    if (value == contentsAreVisible())
        return;
    notifyChange(ContentsVisibleChange);
    GraphicsLayer::setContentsVisible(value);
    if (maskLayer())
        maskLayer()->setContentsVisible(value);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setContentsOpaque(bool value)
{
    if (value == contentsOpaque())
        return;
    notifyChange(ContentsOpaqueChange);
    GraphicsLayer::setContentsOpaque(value);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setBackfaceVisibility(bool value)
{
    if (value == backfaceVisibility())
        return;
    GraphicsLayer::setBackfaceVisibility(value);
    notifyChange(BackfaceVisibilityChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setOpacity(float value)
{
    if (value == opacity())
        return;
    GraphicsLayer::setOpacity(value);
    notifyChange(OpacityChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setContentsRect(const IntRect& value)
{
    if (value == contentsRect())
        return;
    GraphicsLayer::setContentsRect(value);
    notifyChange(ContentsRectChange);
}

void GraphicsLayerTextureMapper::setContentsToSolidColor(const Color& color)
{
    if (color == m_solidColor)
        return;

    m_solidColor = color;
    notifyChange(BackgroundColorChange);
}


/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setContentsToImage(Image* image)
{
    if (image) {
        // Make the decision about whether the image has changed.
        // This code makes the assumption that pointer equality on a NativeImagePtr is a valid way to tell if the image is changed.
        // This assumption is true in Qt, GTK and EFL.
        NativeImagePtr newNativeImagePtr = image->nativeImageForCurrentFrame();
        if (!newNativeImagePtr)
            return;

        if (newNativeImagePtr == m_compositedNativeImagePtr)
            return;

        m_compositedNativeImagePtr = newNativeImagePtr;
        if (!m_compositedImage)
            m_compositedImage = TextureMapperTiledBackingStore::create();
        m_compositedImage->setContentsToImage(image);
    } else {
        m_compositedNativeImagePtr = 0;
        m_compositedImage = 0;
    }

    setContentsToMedia(m_compositedImage.get());
    notifyChange(ContentChange);
    GraphicsLayer::setContentsToImage(image);
}

void GraphicsLayerTextureMapper::setContentsToMedia(TextureMapperPlatformLayer* media)
{
    if (media == m_contentsLayer)
        return;

    GraphicsLayer::setContentsToMedia(media);
    notifyChange(ContentChange);

    if (m_contentsLayer)
        m_contentsLayer->setClient(0);

    m_contentsLayer = media;

    if (m_contentsLayer)
        m_contentsLayer->setClient(this);
}

void GraphicsLayerTextureMapper::setShowDebugBorder(bool show)
{
    if (isShowingDebugBorder() == show)
        return;

    GraphicsLayer::setShowDebugBorder(show);
    notifyChange(DebugVisualsChange);
}

void GraphicsLayerTextureMapper::setShowRepaintCounter(bool show)
{
    if (isShowingRepaintCounter() == show)
        return;

    GraphicsLayer::setShowRepaintCounter(show);
    notifyChange(DebugVisualsChange);
}

void GraphicsLayerTextureMapper::didCommitScrollOffset(const IntSize& offset)
{
    if (offset.isZero())
        return;

    m_committedScrollOffset = offset;
    notifyChange(CommittedScrollOffsetChange);
}

void GraphicsLayerTextureMapper::setIsScrollable(bool isScrollable)
{
    if (m_isScrollable == isScrollable)
        return;

    m_isScrollable = isScrollable;
    notifyChange(IsScrollableChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::flushCompositingStateForThisLayerOnly()
{
    prepareBackingStoreIfNeeded();
    commitLayerChanges();
    m_layer->syncAnimations();
    updateBackingStoreIfNeeded();
}

void GraphicsLayerTextureMapper::prepareBackingStoreIfNeeded()
{
    if (!shouldHaveBackingStore()) {
        m_backingStore.clear();
        m_changeMask |= BackingStoreChange;
    } else {
        if (!m_backingStore) {
            m_backingStore = TextureMapperTiledBackingStore::create();
            m_changeMask |= BackingStoreChange;
        }
    }

    updateDebugBorderAndRepaintCount();
}

void GraphicsLayerTextureMapper::updateDebugBorderAndRepaintCount()
{
    if (isShowingDebugBorder())
        updateDebugIndicators();

    // When this has its own backing store (e.g. Qt WK1), update the repaint count before calling TextureMapperLayer::flushCompositingStateForThisLayerOnly().
    bool needsToRepaint = shouldHaveBackingStore() && (m_needsDisplay || !m_needsDisplayRect.isEmpty());
    if (isShowingRepaintCounter() && needsToRepaint) {
        incrementRepaintCount();
        m_changeMask |= RepaintCountChange;
    }
}

void GraphicsLayerTextureMapper::setDebugBorder(const Color& color, float width)
{
    m_debugBorderColor = color;
    m_debugBorderWidth = width;
    m_changeMask |= DebugVisualsChange;
}

static void toTextureMapperLayerVector(const Vector<GraphicsLayer*>& layers, Vector<TextureMapperLayer*>& texmapLayers)
{
    texmapLayers.reserveCapacity(layers.size());
    for (size_t i = 0; i < layers.size(); ++i)
        texmapLayers.append(toTextureMapperLayer(layers[i]));
}

void GraphicsLayerTextureMapper::commitLayerChanges()
{
    if (m_changeMask == NoChanges)
        return;

    if (m_changeMask & ChildrenChange) {
        Vector<TextureMapperLayer*> textureMapperLayerChildren;
        toTextureMapperLayerVector(children(), textureMapperLayerChildren);
        m_layer->setChildren(textureMapperLayerChildren);
    }

    if (m_changeMask & MaskLayerChange)
        m_layer->setMaskLayer(toTextureMapperLayer(maskLayer()));

    if (m_changeMask & ReplicaLayerChange)
        m_layer->setReplicaLayer(toTextureMapperLayer(replicaLayer()));

    if (m_changeMask & PositionChange)
        m_layer->setPosition(position());

    if (m_changeMask & AnchorPointChange)
        m_layer->setAnchorPoint(anchorPoint());

    if (m_changeMask & SizeChange)
        m_layer->setSize(size());

    if (m_changeMask & TransformChange)
        m_layer->setTransform(transform());

    if (m_changeMask & ChildrenTransformChange)
        m_layer->setChildrenTransform(childrenTransform());

    if (m_changeMask & Preserves3DChange)
        m_layer->setPreserves3D(preserves3D());

    if (m_changeMask & ContentsRectChange)
        m_layer->setContentsRect(contentsRect());

    if (m_changeMask & MasksToBoundsChange)
        m_layer->setMasksToBounds(masksToBounds());

    if (m_changeMask & DrawsContentChange)
        m_layer->setDrawsContent(drawsContent());

    if (m_changeMask & ContentsVisibleChange)
        m_layer->setContentsVisible(contentsAreVisible());

    if (m_changeMask & ContentsOpaqueChange)
        m_layer->setContentsOpaque(contentsOpaque());

    if (m_changeMask & BackfaceVisibilityChange)
        m_layer->setBackfaceVisibility(backfaceVisibility());

    if (m_changeMask & OpacityChange)
        m_layer->setOpacity(opacity());

    if (m_changeMask & BackgroundColorChange)
        m_layer->setSolidColor(solidColor());

#if ENABLE(CSS_FILTERS)
    if (m_changeMask & FilterChange)
        m_layer->setFilters(filters());
#endif

    if (m_changeMask & BackingStoreChange)
        m_layer->setBackingStore(m_backingStore);

    if (m_changeMask & DebugVisualsChange)
        m_layer->setDebugVisuals(isShowingDebugBorder(), debugBorderColor(), debugBorderWidth(), isShowingRepaintCounter());

    if (m_changeMask & RepaintCountChange)
        m_layer->setRepaintCount(repaintCount());

    if (m_changeMask & ContentChange)
        m_layer->setContentsLayer(platformLayer());

    if (m_changeMask & AnimationChange)
        m_layer->setAnimations(m_animations);

    if (m_changeMask & AnimationStarted)
        client()->notifyAnimationStarted(this, m_animationStartTime);

    if (m_changeMask & FixedToViewporChange)
        m_layer->setFixedToViewport(fixedToViewport());

    if (m_changeMask & IsScrollableChange)
        m_layer->setIsScrollable(isScrollable());

    if (m_changeMask & CommittedScrollOffsetChange)
        m_layer->didCommitScrollOffset(m_committedScrollOffset);

    m_changeMask = NoChanges;
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::flushCompositingState(const FloatRect& rect)
{
    if (!m_layer->textureMapper())
        return;

    flushCompositingStateForThisLayerOnly();

    if (maskLayer())
        maskLayer()->flushCompositingState(rect);
    if (replicaLayer())
        replicaLayer()->flushCompositingState(rect);
    for (size_t i = 0; i < children().size(); ++i)
        children()[i]->flushCompositingState(rect);
}

void GraphicsLayerTextureMapper::updateBackingStoreIfNeeded()
{
    TextureMapper* textureMapper = m_layer->textureMapper();
    if (!textureMapper)
        return;

    if (!shouldHaveBackingStore()) {
        ASSERT(!m_backingStore);
        return;
    }
    ASSERT(m_backingStore);

    IntRect dirtyRect = enclosingIntRect(FloatRect(FloatPoint::zero(), m_size));
    if (!m_needsDisplay)
        dirtyRect.intersect(enclosingIntRect(m_needsDisplayRect));
    if (dirtyRect.isEmpty())
        return;

#if PLATFORM(QT) && !defined(QT_NO_DYNAMIC_CAST)
    ASSERT(dynamic_cast<TextureMapperTiledBackingStore*>(m_backingStore.get()));
#endif
    TextureMapperTiledBackingStore* backingStore = static_cast<TextureMapperTiledBackingStore*>(m_backingStore.get());

    backingStore->updateContents(textureMapper, this, m_size, dirtyRect, BitmapTexture::UpdateCanModifyOriginalImageData);

    m_needsDisplay = false;
    m_needsDisplayRect = IntRect();
}

bool GraphicsLayerTextureMapper::shouldHaveBackingStore() const
{
    return drawsContent() && contentsAreVisible() && !m_size.isEmpty();
}

bool GraphicsLayerTextureMapper::addAnimation(const KeyframeValueList& valueList, const IntSize& boxSize, const Animation* anim, const String& keyframesName, double timeOffset)
{
    ASSERT(!keyframesName.isEmpty());

    if (!anim || anim->isEmptyOrZeroDuration() || valueList.size() < 2 || (valueList.property() != AnimatedPropertyWebkitTransform && valueList.property() != AnimatedPropertyOpacity))
        return false;

    bool listsMatch = false;
    bool hasBigRotation;

    if (valueList.property() == AnimatedPropertyWebkitTransform)
        listsMatch = validateTransformOperations(valueList, hasBigRotation) >= 0;

    const double currentTime = WTF::currentTime();
    m_animations.add(GraphicsLayerAnimation(keyframesName, valueList, boxSize, anim, currentTime - timeOffset, listsMatch));
    // m_animationStartTime is the time of the first real frame of animation, now or delayed by a negative offset.
    if (timeOffset > 0)
        m_animationStartTime = currentTime;
    else
        m_animationStartTime = currentTime - timeOffset;
    notifyChange(AnimationChange);
    notifyChange(AnimationStarted);
    return true;
}

void GraphicsLayerTextureMapper::setAnimations(const GraphicsLayerAnimations& animations)
{
    m_animations = animations;
    notifyChange(AnimationChange);
}


void GraphicsLayerTextureMapper::pauseAnimation(const String& animationName, double timeOffset)
{
    m_animations.pause(animationName, timeOffset);
}

void GraphicsLayerTextureMapper::removeAnimation(const String& animationName)
{
    m_animations.remove(animationName);
}

#if ENABLE(CSS_FILTERS)
bool GraphicsLayerTextureMapper::setFilters(const FilterOperations& filters)
{
    TextureMapper* textureMapper = m_layer->textureMapper();
    // TextureMapperImageBuffer does not support CSS filters.
    if (!textureMapper || textureMapper->accelerationMode() == TextureMapper::SoftwareMode)
        return false;
    notifyChange(FilterChange);
    return GraphicsLayer::setFilters(filters);
}
#endif

void GraphicsLayerTextureMapper::setFixedToViewport(bool fixed)
{
    if (m_fixedToViewport == fixed)
        return;

    m_fixedToViewport = fixed;
    notifyChange(FixedToViewporChange);
}

void GraphicsLayerTextureMapper::setRepaintCount(int repaintCount)
{
    m_repaintCount = repaintCount;
    notifyChange(RepaintCountChange);
}

}
#endif
