/*
 Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

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
#include "TextureMapperNode.h"

#include "GraphicsLayerTextureMapper.h"

namespace WebCore {

class TextureMapperCache {
public:
    void mark(BitmapTexture* texture);

    class Entry {
    public:
        RefPtr<BitmapTexture> texture;
        Entry() : previousCost(0) { }
        inline int computeCost() const
        {
            if (!texture || !texture->isValid() || texture->isPacked())
                return 0;
            const IntSize textureSize = texture->size();
            // An image's cost in bytes is width * height * bytes per pixel (4).
            return textureSize.width() * textureSize.height() * 4;
        }
        Entry(BitmapTexture* newTexture)
            : texture(newTexture)
        {
        }
        bool operator==(const Entry& other) const { return texture == other.texture; }
        int previousCost;
    };

    TextureMapperCache()
        : m_totalCost(0)
    {
    }

    void purge();
    Vector<Entry> m_data;
    int m_totalCost;
#ifndef TEXMAP_TEXTURE_CACHE_KBS
#define TEXMAP_TEXTURE_CACHE_KBS 24 * 1024
#endif
    static const int MaxCost = TEXMAP_TEXTURE_CACHE_KBS * 1024;
    static const int PurgeAmount = MaxCost / 4;
};


void TextureMapperCache::purge()
{
    const int size = m_data.size();

    if (m_totalCost <= TextureMapperCache::MaxCost)
        return;

    // Ensure that we have the right count. It might be inaccurate if content changed size.
    // We only do this when we're actually ready to purge.
    m_totalCost = 0;
    for (int i = 0; i < size; ++i)
        m_totalCost += m_data[i].computeCost();

    for (int i = size-1; i >= 0 && m_totalCost > TextureMapperCache::MaxCost - TextureMapperCache::PurgeAmount; --i) {
        Entry& entry = m_data[i];
        if (entry.texture->isLocked() || !entry.texture->isValid() || entry.texture->isPacked())
            continue;
        m_totalCost -= entry.previousCost;
        entry.texture->pack();
        m_data.remove(i);
    }
}

void TextureMapperCache::mark(BitmapTexture* texture)
{
    if (!texture || !texture->isValid())
        return;

    Entry entry(texture);
    size_t index = m_data.find(entry);
    if (!index)
        return;

    int previousCost = 0;

    if (index < m_data.size()) {
        previousCost = m_data[index].previousCost;
        m_data.remove(index);
    }
    const int cost = entry.computeCost();
    m_totalCost -= previousCost;
    m_totalCost += (entry.previousCost = cost);
    m_data.prepend(entry);
}

class TextureMapperCacheLock {
public:
    TextureMapperCacheLock(BitmapTexture* texture) : m_texture(texture)
    {
        if (m_texture)
            m_texture->lock();
    }
    ~TextureMapperCacheLock()
    {
        if (m_texture)
            m_texture->unlock();
    }

private:
    RefPtr<BitmapTexture> m_texture;
};


TextureMapperCache* TextureMapperNode::cache()
{
    TextureMapperNode* root = rootLayer();
    if (!root)
        return 0;
    if (!root->m_cache)
        root->m_cache = new TextureMapperCache;
    return root->m_cache;
}

void TextureMapperNode::setNeedsDisplayInRect(IntRect rect)
{
    if (m_platformClient) {
        if (m_state.hasSurfaceDescendants) {
            m_platformClient->setNeedsDisplay();
            return;
        }
        rect.intersect(IntRect(0, 0, m_size.width(), m_size.height()));
        if (rect.isEmpty())
            return;
        m_platformClient->setNeedsDisplayInRect(rect);
        return;
    }

    if (!m_parent)
        return;

    m_parent->setNeedsDisplayInRect(rect);
}

void TextureMapperNode::setNeedsDisplay()
{
    if (m_effectTarget)
        m_effectTarget->setNeedsDisplay();
    if (m_transforms.targetBoundingRect.isEmpty())
        return;
    if (m_state.drawsContent || m_currentContent.contentType != HTMLContentType)
        setNeedsDisplayInRect(m_transforms.targetBoundingRect);
}

void TextureMapperNode::setPlatformLayerClient(TextureMapperLayerClient* client)
{
    m_platformClient = client;
}

int TextureMapperNode::compareGraphicsLayersZValue(const void* a, const void* b)
{
    typedef const TextureMapperNode* NodePtr;
    const NodePtr* nodeA = static_cast<const NodePtr*>(a);
    const NodePtr* nodeB = static_cast<const NodePtr*>(b);
    return int(((*nodeA)->m_transforms.centerZ - (*nodeB)->m_transforms.centerZ) * 1000);
}

void TextureMapperNode::sortByZOrder(Vector<TextureMapperNode* >& array, int first, int last)
{
    qsort(array.data(), array.size(), sizeof(TextureMapperNode*), TextureMapperNode::compareGraphicsLayersZValue);
}

bool TextureMapperNode::hasSurfaceDescendants() const
{
    if (m_layerType == ClipLayer || m_layerType == TransparencyLayer || m_state.replicaLayer)
        return true;

    const int size = m_children.size();
    for (int i = 0; i < size; ++i) {
        if (TextureMapperNode* child = m_children[i]) {
            if (child->hasSurfaceDescendants())
                return true;
        }
    }
    return false;
}

int TextureMapperNode::countDescendantsWithContent() const
{
    if (!m_state.visible || m_state.opacity < 0.001)
        return 0;

    int descendantsWithContent = (m_state.drawsContent || m_currentContent.contentType != HTMLContentType) ? 1 : 0;

    const int size = m_children.size();
    for (int i = 0; i < size; ++i) {
        if (TextureMapperNode* child = m_children[i])
            descendantsWithContent += child->countDescendantsWithContent();
    }

    return descendantsWithContent;
}

TextureMapperNode* TextureMapperNode::toTextureMapperNode(GraphicsLayer* layer)
{
    return layer ? static_cast<TextureMapperNode*>(layer->platformLayer()) : 0;
}

void TextureMapperNode::computeLayerType()
{
    const bool selfHasContent = m_state.drawsContent || (m_currentContent.contentType != HTMLContentType);
    const bool hasDescendantsWithContent = m_state.descendantsWithContent - (selfHasContent ? 1 : 0);
    const bool hasTransparency = m_state.opacity < 0.99 || m_state.maskLayer;
    const bool hasReplica = m_state.replicaLayer;

    //  DefaultLayer: draws itself and its children directly to the current framebuffer.
    //                any layer that doesn't conform to the other rules is a DefaultLayer.
    m_layerType = DefaultLayer;

    //  RootLayer: the top level. Draws to a framebuffer, and the target texture draws into the viewport.
    //            only one layer is the root layer.
    if (!m_parent && !m_effectTarget) {
        m_layerType = RootLayer;
        return;
    }

    // A layer with no contents is always a default layer.
    if (!m_state.descendantsWithContent)
        return;

    //  ClipLayer: creates a new framebuffer, the size of the layer, and then paints it to the enclosing BitmapTexture with the layer's transform/opacity.
    //              A clip layer is a layer that masks to bounds, doesn't preserve 3D, has children, and has a transparency/mask or a non-rectangular transform.
    if (hasDescendantsWithContent && m_state.maskLayer) {
        m_layerType = ClipLayer;
        return;
    }

    //  ScissorLayer: draws to the current framebuffer, and applies an extra scissor before drawing its children.
    //                A scissor layer is a layer with children that masks to bounds, is not a transparency layer, and has a rectangular clip.
    if (m_state.masksToBounds && hasDescendantsWithContent) {
        if (hasTransparency || !m_state.transform.isIdentityOrTranslation() || m_parent->m_state.preserves3D)
            m_layerType = ClipLayer;
        else
            m_layerType = ScissorLayer;
        return;
    }

    //  TransparencyLayer: creates a new framebuffer idetical in size to the current framebuffer. Then draws the fb's texture to the current framebuffer with identity transform.
    //                     Used for layers with children and transparency/mask that preserve 3D or don't mask to bounds.
    if ((hasReplica && hasDescendantsWithContent) || (hasReplica && hasTransparency) || (hasTransparency && m_state.descendantsWithContent > 1))
        m_layerType = TransparencyLayer;
}

void TextureMapperNode::initializeTextureMapper(TextureMapper* textureMapper)
{
    if (m_texture)
        return;
    m_surface = textureMapper->createTexture();
    m_replicaSurface = textureMapper->createTexture();
    m_texture = textureMapper->createTexture();
    cache()->mark(m_texture.get());
}

TextureMapperNode::TextureMapperNode()
    : m_layerType(DefaultLayer)
    , m_surface(0)
    , m_parent(0)
    , m_effectTarget(0)
    , m_platformClient(0)
    , m_cache(0)
{
}

TextureMapperNode* TextureMapperNode::rootLayer()
{
    if (m_effectTarget)
        return m_effectTarget->rootLayer();
    if (m_parent)
        return m_parent->rootLayer();
    return this;
}

void TextureMapperNode::invalidateTransform()
{
    m_transforms.dirty = true;
    if (m_layerType != ClipLayer)
        m_state.dirty = true;
    if (m_state.replicaLayer)
        m_state.replicaLayer->invalidateTransform();
    const int size = m_children.size();
    for (int i = 0; i < size; ++i) {
        if (TextureMapperNode* layer = m_children[i])
            layer->invalidateTransform();
    }
}

void TextureMapperNode::computeLocalTransform()
{
    if (!m_transforms.localDirty)
        return;
    const float originX = m_state.anchorPoint.x() * m_size.width();
    const float originY = m_state.anchorPoint.y() * m_size.height();
    m_transforms.local =
        TransformationMatrix()
        .translate3d(originX + m_state.pos.x(), originY + m_state.pos.y(), m_state.anchorPoint.z())
        .multiply(m_state.transform)
        .translate3d(-originX, -originY, -m_state.anchorPoint.z());
    m_transforms.localDirty = false;
}

void TextureMapperNode::flattenTo2DSpaceIfNecessary()
{
    if (m_state.preserves3D)
        return;

    m_transforms.forDescendants.setM13(0);
    m_transforms.forDescendants.setM23(0);
    m_transforms.forDescendants.setM31(0);
    m_transforms.forDescendants.setM32(0);
    m_transforms.forDescendants.setM33(1);
    m_transforms.forDescendants.setM34(0);
    m_transforms.forDescendants.setM43(0);
}

IntSize TextureMapperNode::nearestSurfaceSize() const
{
    if (m_layerType == ClipLayer || m_layerType == RootLayer)
        return m_surface && !m_surface->size().isEmpty() ? m_surface->size() : m_size;
    return m_parent->nearestSurfaceSize();
}

void TextureMapperNode::computeReplicaTransform()
{
    if (!m_state.replicaLayer)
        return;

    m_nearestSurfaceSize = nearestSurfaceSize();

    if (m_layerType != TransparencyLayer) {
        m_transforms.replica = TransformationMatrix(m_transforms.target).multiply(m_state.replicaLayer->m_transforms.local);
        return;
    }

    const float originX = m_transforms.target.m41();
    const float originY = m_transforms.target.m42();
    m_transforms.replica =
            TransformationMatrix()
                .translate(originX, originY)
                .multiply(m_state.replicaLayer->m_transforms.local)
                .translate(-originX, -originY);
}

void TextureMapperNode::computeTransformations()
{
    if (!m_transforms.dirty)
        return;

    m_transforms.dirty = false;
    if ((m_size.isEmpty() && m_state.masksToBounds))
        return;

    TextureMapperNode* parent = m_parent;
    computeLocalTransform();

    m_transforms.target = TransformationMatrix(parent ? parent->m_transforms.forDescendants : TransformationMatrix()).multiply(m_transforms.local);
    m_transforms.forDescendants = (m_layerType == ClipLayer ? TransformationMatrix() : m_transforms.target);

    if (m_effectTarget)
        return;

    m_transforms.targetBoundingRect = IntRect(m_transforms.target.mapRect(entireRect()));
    if (m_state.replicaLayer)
        m_state.replicaLayer->computeTransformations();

    flattenTo2DSpaceIfNecessary();

    if (!m_state.backfaceVisibility && m_transforms.target.inverse().m33() < 0) {
        m_state.visible = false;
        return;
    }
    m_state.visible = true;

    if (parent && parent->m_state.preserves3D)
        m_transforms.centerZ = m_transforms.target.mapPoint(FloatPoint3D(m_size.width() / 2, m_size.height() / 2, 0)).z();

    if (!m_children.size())
        return;

    if (m_state.childrenTransform.isIdentity())
        return;

    const FloatPoint centerPoint = FloatPoint(m_size.width() / 2, m_size.height() / 2);
    if (m_transforms.perspectiveDirty)
        m_transforms.perspective = TransformationMatrix()
            .translate(centerPoint.x(), centerPoint.y())
            .multiply(m_state.childrenTransform)
            .translate(-centerPoint.x(), -centerPoint.y());
    m_transforms.perspectiveDirty = false;
    m_transforms.forDescendants.multiply(m_transforms.perspective);
}

void TextureMapperNode::uploadTextureFromContent(TextureMapper* textureMapper, const IntRect& visibleRect, GraphicsLayer* layer)
{
    if (m_size.isEmpty() || !layer) {
        m_texture->destroy();
        return;
    }

    if (m_currentContent.contentType == DirectImageContentType) {
        if (m_currentContent.image)
            m_texture->setContentsToImage(m_currentContent.image.get());
        return;
    }

    if (m_currentContent.contentType == MediaContentType) {
        if (!m_currentContent.media)
            return;
        m_texture->reset(m_size, true);
        PlatformGraphicsContext* platformContext = m_texture->beginPaintMedia();
        GraphicsContext context(platformContext);
        m_currentContent.media->paint(&context);
        m_texture->endPaint();
        return;
    }

    const bool needsReset = (m_texture->contentSize() != m_size) || !m_texture->isValid();
    if ((m_currentContent.contentType != HTMLContentType)
        || (!m_currentContent.needsDisplay && m_currentContent.needsDisplayRect.isEmpty() && !needsReset))
        return;

    IntRect dirtyRect = IntRect(0, 0, m_size.width(), m_size.height());
    if (!needsReset && !m_currentContent.needsDisplay)
        dirtyRect.intersect(m_currentContent.needsDisplayRect);

    if (needsReset)
        m_texture->reset(m_size, m_state.contentsOpaque);

    {
        GraphicsContext context(m_texture->beginPaint(dirtyRect));
        if (textureMapper) {
            context.setImageInterpolationQuality(textureMapper->imageInterpolationQuality());
            context.setTextDrawingMode(textureMapper->textDrawingMode());
        }
        layer->paintGraphicsLayerContents(context, dirtyRect);
    }
    m_texture->endPaint();
    m_currentContent.needsDisplay = false;
}


void TextureMapperNode::paint(TextureMapper* textureMapper, const TextureMapperContentLayer::PaintOptions& options)
{
    ASSERT(m_layerType == RootLayer);
    if (m_size.isEmpty())
        return;

    TexmapPaintOptions opt;
    opt.opacity = 1;
    opt.rootLayer = this;
    opt.scissorRect = options.targetRect;
    opt.visibleRect = options.visibleRect;
    opt.textureMapper = textureMapper;
    opt.surface = 0;
    opt.cache = m_cache;
    paintRecursive(opt);

    if (textureMapper->allowSurfaceForRoot() || m_state.hasSurfaceDescendants) {
        textureMapper->bindSurface(0);
        textureMapper->paintToTarget(*m_surface.get(), options.viewportSize, options.transform, options.opacity * m_state.opacity, options.targetRect);
    }
    m_cache->purge();
}

void TextureMapperNode::paintSelf(const TexmapPaintOptions& options)
{
    if (m_size.isEmpty() || (!m_state.drawsContent && m_currentContent.contentType == HTMLContentType))
        return;

    RefPtr<BitmapTexture> replicaMaskTexture;
    m_texture->unpack();

    RefPtr<BitmapTexture> maskTexture = m_state.maskLayer ? m_state.maskLayer->m_texture : 0;
    if (m_state.replicaLayer && m_state.replicaLayer->m_state.maskLayer)
        replicaMaskTexture = m_state.replicaLayer->m_state.maskLayer->m_texture;

    if (maskTexture)
        maskTexture->unpack();

    if (replicaMaskTexture)
        replicaMaskTexture->unpack();

    const float opacity = options.isSurface ? 1 : options.opacity;

    if (m_state.replicaLayer && !options.isSurface)
        options.textureMapper->drawTexture(*m_texture.get(), replicaRect(), m_transforms.replica,
                         opacity * m_state.replicaLayer->m_state.opacity,
                         replicaMaskTexture ? replicaMaskTexture.get() : maskTexture.get());

    const IntRect rect = m_layerType == ClipLayer ? entireRect() : targetRect();
    const TransformationMatrix transform = m_layerType == ClipLayer ? TransformationMatrix() : m_transforms.target;
    options.textureMapper->drawTexture(*m_texture.get(), rect, transform, opacity, options.isSurface ? 0 : maskTexture.get());
    options.cache->mark(m_texture.get());
}

bool TextureMapperNode::paintReplica(const TexmapPaintOptions& options)
{
    BitmapTexture& texture = *m_surface.get();
    TextureMapperNode* replica = m_state.replicaLayer;
    RefPtr<BitmapTexture> maskTexture;
    if (TextureMapperNode* mask = m_state.maskLayer)
        maskTexture = mask->m_texture;
    RefPtr<BitmapTexture> replicaMaskTexture;
    if (!replica)
        return false;

    if (replica && replica->m_state.maskLayer)
        replicaMaskTexture = replica->m_state.maskLayer->m_texture;

    if (replicaMaskTexture)
        replicaMaskTexture->unpack();
    ASSERT(m_replicaSurface);
    m_replicaSurface->reset(options.surface->size());
    m_replicaSurface->setOffset(options.surface->offset());
    options.cache->mark(m_replicaSurface.get());
    options.textureMapper->bindSurface(m_replicaSurface.get());
    options.textureMapper->drawTexture(texture, replicaRect(), m_transforms.replica, replica->m_state.opacity, replicaMaskTexture ? replicaMaskTexture.get() : maskTexture.get());
    options.textureMapper->drawTexture(texture, IntRect(IntPoint(0, 0), options.surface->size()), TransformationMatrix(), 1.0f, maskTexture.get());
    options.textureMapper->bindSurface(options.surface);
    options.cache->mark(options.surface);
    options.textureMapper->drawTexture(*m_replicaSurface.get(), IntRect(IntPoint(0, 0), options.surface->size()), TransformationMatrix(), options.opacity, 0);
    return true;
}

void TextureMapperNode::paintSurface(const TexmapPaintOptions& options)
{
    if (m_layerType == RootLayer || m_layerType == DefaultLayer || m_layerType == ScissorLayer)
        return;

    RefPtr<BitmapTexture> maskTexture;
    if (TextureMapperNode* mask = m_state.maskLayer)
        maskTexture = mask->m_texture;

    ASSERT(m_surface);
    BitmapTexture& texture = *m_surface.get();
    if (maskTexture)
        maskTexture->unpack();
    texture.unpack();

    if (paintReplica(options))
        return;

    options.textureMapper->bindSurface(options.surface);
    options.textureMapper->drawTexture(texture,
                             m_layerType == TransparencyLayer ? IntRect(IntPoint(0, 0), options.surface->size()) :
                             targetRect(),
                             m_layerType == TransparencyLayer ? TransformationMatrix() : m_transforms.target,
                             options.opacity, maskTexture.get());
    options.cache->mark(&texture);
}

void TextureMapperNode::paintSelfAndChildren(const TexmapPaintOptions& options, TexmapPaintOptions& optionsForDescendants)
{
    bool didPaintSelf = false;
    if (!m_state.preserves3D || m_children.isEmpty()) {
        paintSelf(options);
        didPaintSelf = true;
    }

    if (m_children.isEmpty() && !options.isSurface)
        return;

    if (m_layerType == ScissorLayer)
        optionsForDescendants.scissorRect.intersect(m_transforms.target.mapRect(IntRect(0, 0, m_size.width(), m_size.height())));

    for (int i = 0; i < m_children.size(); ++i) {
        TextureMapperNode* layer = m_children[i];
        if (!layer)
            continue;

        if (!didPaintSelf && layer->m_transforms.centerZ >= 0) {
            paintSelf(options);
            didPaintSelf = true;
        }
        layer->paintRecursive(optionsForDescendants);
        if (options.isSurface) {
            ASSERT(m_surface);
            options.cache->mark(m_surface.get());
            options.textureMapper->bindSurface(m_surface.get());
        }
    }
    if (!didPaintSelf) {
        paintSelf(options);
        didPaintSelf = true;
    }
}

void TextureMapperNode::paintRecursive(TexmapPaintOptions options)
{
    bool isDirty = m_state.dirty;
    m_state.dirty = false;

    if ((m_size.isEmpty() && (m_state.masksToBounds
        || m_children.isEmpty())) || !m_state.visible || options.opacity < 0.01 || m_state.opacity < 0.01)
        return;

    computeReplicaTransform();

    if (m_state.maskLayer)
        m_state.maskLayer->m_state.dirty = false;

    if (m_state.replicaLayer) {
        m_state.replicaLayer->m_state.dirty = false;
        if (m_state.replicaLayer->m_state.maskLayer)
            m_state.replicaLayer->m_state.maskLayer->m_state.dirty = false;
    }

    const bool isSurface = (m_layerType == ClipLayer
                            || m_layerType == TransparencyLayer
                            || (m_layerType == RootLayer
                                && (options.textureMapper->allowSurfaceForRoot() || m_state.hasSurfaceDescendants)
                                ));

    const IntRect boundingRectfromNearestSurface = m_transforms.targetBoundingRect;

    options.opacity *= m_state.opacity;

    TexmapPaintOptions optionsForDescendants(options);
    optionsForDescendants.opacity = isSurface ? 1 : options.opacity;
    options.isSurface = isSurface;

    if (m_layerType == ClipLayer) {
        optionsForDescendants.visibleRect = TransformationMatrix().translate(-boundingRectfromNearestSurface.x(), -boundingRectfromNearestSurface.y()).mapRect(options.visibleRect);
        optionsForDescendants.scissorRect = IntRect(0, 0, m_size.width(), m_size.height());
    }

    if (m_layerType == ScissorLayer)
        optionsForDescendants.scissorRect.intersect(m_transforms.targetBoundingRect);
    options.textureMapper->setClip(optionsForDescendants.scissorRect);

    TextureMapperCacheLock(m_texture.get());
    TextureMapperCacheLock(m_surface.get());
    TextureMapperCacheLock(m_replicaSurface.get());

    options.cache->purge();

    if (isSurface) {
        ASSERT(m_surface);
        if (!m_surface->isValid())
            isDirty = true;
        if (m_state.tiled) {
            m_surface->reset(options.visibleRect.size());
            m_surface->setOffset(options.visibleRect.location());
        } else if (isDirty)
            m_surface->reset(m_layerType == TransparencyLayer ? options.surface->size() : m_size);
        options.cache->mark(m_surface.get());
        options.textureMapper->bindSurface(m_surface.get());
        optionsForDescendants.surface = m_surface.get();
    } else if (m_surface)
        m_surface->destroy();

    if (isDirty || !isSurface || m_state.tiled || !m_surface->isValid())
        paintSelfAndChildren(options, optionsForDescendants);

    paintSurface(options);
}

TextureMapperNode::~TextureMapperNode()
{
    setNeedsDisplay();
    {
        const int childrenSize = m_children.size();
        for (int i = childrenSize-1; i >= 0; --i) {
            ASSERT(m_children[i]->m_parent == this);
            m_children[i]->m_parent = 0;
        }
    }
    if (m_parent)
        m_parent->m_children.remove(m_parent->m_children.find(this));
    if (m_cache)
        delete m_cache;
}

void TextureMapperNode::performPostSyncOperations()
{
    const LayerType prevLayerType = m_layerType;
    computeLayerType();
    if (prevLayerType != m_layerType)
        m_state.dirty = true;
    if (m_transforms.dirty)
        setNeedsDisplay();

    computeTransformations();
    if (m_state.maskLayer && !m_state.dirty)
        m_state.dirty = m_state.maskLayer->m_state.dirty;
    if (m_state.replicaLayer && !m_state.dirty)
        m_state.dirty = m_state.replicaLayer->m_state.dirty;

    const int size = m_children.size();

    for (int i = size - 1; i >= 0; --i) {
        TextureMapperNode* layer = m_children[i];

        layer->performPostSyncOperations();
        if (!m_state.dirty)
            m_state.dirty = layer->m_state.dirty;
    }
    m_state.hasSurfaceDescendants = hasSurfaceDescendants();
    if (m_state.dirty)
        m_state.descendantsWithContent = countDescendantsWithContent();

    if (m_state.preserves3D)
        sortByZOrder(m_children, 0, size);
    if (m_state.dirty)
        setNeedsDisplay();
}

void TextureMapperNode::syncCompositingState(GraphicsLayerTextureMapper* graphicsLayer, bool recurse)
{
    TextureMapper* textureMapper = rootLayer()->m_platformClient->textureMapper();
    syncCompositingStateInternal(graphicsLayer, recurse, textureMapper);
    performPostSyncOperations();
}

void TextureMapperNode::syncCompositingStateSelf(GraphicsLayerTextureMapper* graphicsLayer, TextureMapper* textureMapper)
{
    const int changeMask = graphicsLayer->changeMask();
    initializeTextureMapper(textureMapper);
    const TextureMapperNode::ContentData& pendingContent = graphicsLayer->pendingContent();
    if (changeMask == NoChanges && pendingContent.needsDisplayRect.isEmpty() && !pendingContent.needsDisplay)
        return;

    setNeedsDisplay();
    if (m_parent)
        m_parent->m_state.dirty = true;

    if (m_currentContent.contentType == HTMLContentType && (changeMask & ParentChange)) {
        // The WebCore compositor manages item ownership. We have to make sure graphicsview doesn't
        // try to snatch that ownership.

        if (!graphicsLayer->parent())
            m_parent = 0;
        else
            m_parent = toTextureMapperNode(graphicsLayer->parent());

        if (!graphicsLayer->parent() && m_parent) {
            size_t index = m_parent->m_children.find(this);
            m_parent->m_children.remove(index);
        }
    }

    if (changeMask & ChildrenChange) {
        m_children.clear();
        for (size_t i = 0; i < graphicsLayer->children().size(); ++i) {
            if (TextureMapperNode* child = toTextureMapperNode(graphicsLayer->children()[i])) {
                if (!child)
                    continue;
                m_children.append(child);
                child->m_parent = this;
            }
        }
        m_state.dirty = true;
    }

    if (changeMask & (SizeChange | ContentsRectChange)) {
        IntSize wantedSize = IntSize(graphicsLayer->size().width(), graphicsLayer->size().height());
        if (wantedSize.isEmpty() && pendingContent.contentType == HTMLContentType)
            wantedSize = IntSize(graphicsLayer->contentsRect().width(), graphicsLayer->contentsRect().height());

        if (wantedSize != m_size) {
            m_size = IntSize(wantedSize.width(), wantedSize.height());
            if (m_platformClient)
                m_platformClient->setSizeChanged(m_size);
            const bool needsTiling = m_size.width() > 2000 || m_size.height() > 2000;
            if (m_state.tiled != needsTiling)
                m_state.tiled = needsTiling;
            m_state.dirty = true;
        }
    }

    if (changeMask & MaskLayerChange) {
       if (TextureMapperNode* layer = toTextureMapperNode(graphicsLayer->maskLayer()))
           layer->m_effectTarget = this;
    }

    if (changeMask & ReplicaLayerChange) {
       if (TextureMapperNode* layer = toTextureMapperNode(graphicsLayer->replicaLayer()))
           layer->m_effectTarget = this;
    }

    if (changeMask & (TransformChange | SizeChange | AnchorPointChange | PositionChange))
        m_transforms.localDirty = true;

    if (changeMask & (ChildrenTransformChange | SizeChange))
        m_transforms.perspectiveDirty = true;

    if (changeMask & (ChildrenTransformChange | Preserves3DChange | TransformChange | AnchorPointChange | SizeChange | ContentsRectChange | BackfaceVisibilityChange | PositionChange | MaskLayerChange | DrawsContentChange | ContentChange | ReplicaLayerChange))    {
        // Due to the differences between the way WebCore handles transforms and the way Qt handles transforms,
        // all these elements affect the transforms of all the descendants.
        invalidateTransform();
    }

    if (changeMask & DisplayChange)
        m_state.dirty = true;

    m_state.maskLayer = toTextureMapperNode(graphicsLayer->maskLayer());
    m_state.replicaLayer = toTextureMapperNode(graphicsLayer->replicaLayer());
    m_state.pos = graphicsLayer->position();
    m_state.anchorPoint = graphicsLayer->anchorPoint();
    m_state.size = graphicsLayer->size();
    m_state.transform = graphicsLayer->transform();
    m_state.contentsRect = graphicsLayer->contentsRect();
    m_state.opacity = graphicsLayer->opacity();
    m_state.contentsRect = graphicsLayer->contentsRect();
    m_state.preserves3D = graphicsLayer->preserves3D();
    m_state.masksToBounds = graphicsLayer->masksToBounds();
    m_state.drawsContent = graphicsLayer->drawsContent();
    m_state.contentsOpaque = graphicsLayer->contentsOpaque();
    m_state.backfaceVisibility = graphicsLayer->backfaceVisibility();
    m_state.childrenTransform = graphicsLayer->childrenTransform();
    m_currentContent.contentType = pendingContent.contentType;
    m_currentContent.image = pendingContent.image;
    m_currentContent.media = pendingContent.media;
    m_currentContent.backgroundColor = pendingContent.backgroundColor;
    m_currentContent.needsDisplay = m_currentContent.needsDisplay || pendingContent.needsDisplay;
    m_currentContent.needsDisplayRect.unite(pendingContent.needsDisplayRect);

}

void TextureMapperNode::syncCompositingStateInternal(GraphicsLayerTextureMapper* graphicsLayer, bool recurse, TextureMapper* textureMapper)
{
    syncCompositingStateSelf(graphicsLayer, textureMapper);

    graphicsLayer->didSynchronize();

    if (m_state.maskLayer) {
        m_state.maskLayer->syncCompositingStateInternal(toGraphicsLayerTextureMapper(graphicsLayer->maskLayer()), false, textureMapper);
        if (m_state.maskLayer->m_size.isEmpty())
            m_state.maskLayer->m_size = m_size;
    }

    if (m_state.replicaLayer)
        m_state.replicaLayer->syncCompositingStateInternal(toGraphicsLayerTextureMapper(graphicsLayer->replicaLayer()), false, textureMapper);

    if (m_state.dirty)
        uploadTextureFromContent(textureMapper, m_state.visibleRect, graphicsLayer);

    m_currentContent.needsDisplayRect = IntRect();
    m_currentContent.needsDisplay = false;

    if (!recurse)
        return;

    Vector<GraphicsLayer*> children = graphicsLayer->children();
    for (int i = children.size() - 1; i >= 0; --i) {
        TextureMapperNode* node = toTextureMapperNode(children[i]);
        if (!node)
            continue;
        node->syncCompositingStateInternal(toGraphicsLayerTextureMapper(children[i]), true, textureMapper);
    }
}

}
