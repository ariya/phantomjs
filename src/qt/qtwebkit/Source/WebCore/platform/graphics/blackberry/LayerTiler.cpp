/*
 * Copyright (C) 2011, 2012, 2013 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#if USE(ACCELERATED_COMPOSITING)

#include "LayerTiler.h"

#include "AffineTransform.h"
#include "BitmapImage.h"
#include "LayerCompositingThread.h"
#include "LayerMessage.h"
#include "LayerRenderer.h"
#include "LayerUtilities.h"
#include "LayerWebKitThread.h"
#include "TextureCacheCompositingThread.h"

#include <BlackBerryPlatformGLES2Program.h>
#include <BlackBerryPlatformScreen.h>
#include <BlackBerryPlatformSettings.h>
#include <GLES2/gl2.h>
#include _NTO_CPU_HDR_(smpxchg.h)

#define DEBUG_LAYER_VISIBILITY 0 // Show visible region of layers as a thick border, and print visible rect.
#if DEBUG_LAYER_VISIBILITY
#include <stdlib.h>
#endif

using namespace std;
using BlackBerry::Platform::Graphics::GLES2Program;

namespace WebCore {

class LayerVisibility {
public:
    LayerVisibility(const FloatRect& visibleRect, const HashSet<TileIndex>& tilesNeedingRender)
        : m_visibleRect(visibleRect)
        , m_tilesNeedingRender(tilesNeedingRender)
    {
    }

    FloatRect visibleRect() const { return m_visibleRect; }
    void setVisibleRect(const FloatRect& visibleRect) { m_visibleRect = visibleRect; }

    bool needsRender() const { return !m_tilesNeedingRender.isEmpty(); }
    bool tileNeedsRender(const TileIndex& index) const { return m_tilesNeedingRender.contains(index); }
    void markTileAsRendered(const TileIndex& index) { m_tilesNeedingRender.remove(index); m_tilesRendered.add(index); }
    void swapTilesNeedingRender(HashSet<TileIndex>& tilesNeedingRender) { m_tilesNeedingRender.swap(tilesNeedingRender); }

    void merge(LayerVisibility* visibility)
    {
        if (!visibility)
            return;
        m_tilesRendered.swap(visibility->m_tilesRendered);
        for (HashSet<TileIndex>::iterator it = m_tilesRendered.begin(); it != m_tilesRendered.end(); ++it)
            m_tilesNeedingRender.remove(*it);
    }

private:
    FloatRect m_visibleRect;
    HashSet<TileIndex> m_tilesNeedingRender;
    HashSet<TileIndex> m_tilesRendered;
};

// This is used to make the viewport as used in texture visibility calculations
// slightly larger so textures are uploaded before becoming really visible.
// Inflation is greater in Y direction (one screenful in either direction) since
// scrolling vertically is more common, and easily reaches higher velocity, than
// scrolling horizontally.
// The viewportInflation is expressed as a fraction to multiply the viewport by, and
// it is centered.
// A height of 3 means to consider one screenful above and one screenful below the
// current viewport as visible.
const FloatSize viewportInflation(1.2f, 3.0f);

// This is a small optimization to cut down on calls to operator new.
// Since the compositing thread normally swaps in new visibility objects much more
// often than the WebKit thread swaps them out, it's likely that it will swap out
// the previous visibility object and we can reuse that.
static LayerVisibility* s_spareVisibility = 0;

static IntSize defaultTileSize()
{
    static IntSize size(BlackBerry::Platform::Settings::instance()->tileSize());
    return size;
}

LayerTiler::LayerTiler(LayerWebKitThread* layer)
    : m_layer(layer)
    , m_needsBacking(false)
    , m_contentsDirty(false)
    , m_tileSize(defaultTileSize())
    , m_clearTextureJobs(false)
    , m_frontVisibility(0)
    , m_backVisibility(0)
{
    ref(); // This ref() is matched by a deref in layerCompositingThreadDestroyed();
}

LayerTiler::~LayerTiler()
{
    // Someone should have called LayerTiler::deleteTextures()
    // before now. We can't call it here because we have no
    // OpenGL context.
    ASSERT(m_tiles.isEmpty());

    delete m_frontVisibility;
    delete m_backVisibility;
}

void LayerTiler::layerWebKitThreadDestroyed()
{
    m_layer = 0;
}

void LayerTiler::layerCompositingThreadDestroyed(LayerCompositingThread*)
{
    ASSERT(isCompositingThread());
    deref(); // Matched by ref() in constructor;
}

void LayerTiler::setNeedsDisplay(const FloatRect& dirtyRect)
{
    m_dirtyRect.unite(dirtyRect);
    m_contentsDirty = true;
}

void LayerTiler::setNeedsDisplay()
{
    m_dirtyRect.setLocation(FloatPoint::zero());
    m_dirtyRect.setSize(m_layer->bounds());
    m_contentsDirty = true;
}

void LayerTiler::updateTextureContentsIfNeeded(double scale)
{
    updateTileSize();

    LayerVisibility* frontVisibility = takeFrontVisibility();
    if (frontVisibility) {
        // If we're dirty, start fresh. Otherwise, keep track of tiles rendered so far, to avoid re-rendering the same content.
        if (!m_contentsDirty)
            frontVisibility->merge(m_backVisibility);
        delete m_backVisibility;
        m_backVisibility = frontVisibility;
    }
    bool needsRender = m_backVisibility && m_backVisibility->needsRender();

    // Check if update is needed
    if (!m_contentsDirty && !needsRender)
        return;

#if DEBUG_LAYER_VISIBILITY
    if (m_backVisibility && !m_backVisibility->visibleRect().isEmpty())
        printf("Layer 0x%p local visible rect %s\n", m_layer, BlackBerry::Platform::FloatRect(m_backVisibility->visibleRect()).toString().c_str());
#endif

    // There's no point in drawing contents at a higher resolution for scale
    // invariant layers.
    if (m_layer->sizeIsScaleInvariant())
        scale = 1;

    // Render only visible tiles. Mask layers are a special case, because they're never considered
    // visible. Workaround this by always rendering them (a very fast operation with the
    // BlackBerry::Platform::GraphicsContext since it will just result in ref'ing the mask image).
    FloatRect visibleRect;
    if (m_layer->isMask() || m_layer->filters().size())
        visibleRect = FloatRect(IntPoint::zero(), m_layer->bounds());
    else if (m_backVisibility)
        visibleRect = m_backVisibility->visibleRect();
    else
        visibleRect = FloatRect(BlackBerry::Platform::FloatRect(BlackBerry::Platform::Settings::instance()->layerTilerPrefillRect()));

    IntRect dirtyRect = enclosingIntRect(m_dirtyRect);
    IntSize requiredTextureSize;

    if (m_layer->drawsContent()) {
        // Layer contents must be drawn into a canvas.
        IntRect untransformedDirtyRect(dirtyRect);
        IntRect boundsRect(IntPoint::zero(), m_layer->bounds());
        IntRect untransformedBoundsRect(boundsRect);
        requiredTextureSize = boundsRect.size();

        if (scale != 1) {
            TransformationMatrix matrix;
            matrix.scale(scale);

            dirtyRect = matrix.mapRect(untransformedDirtyRect);
            requiredTextureSize = matrix.mapRect(IntRect(IntPoint::zero(), requiredTextureSize)).size();
            boundsRect = matrix.mapRect(untransformedBoundsRect);
            // The visible rect is using unscaled coordinates in most cases.
            if (m_backVisibility || m_layer->isMask() || m_layer->filters().size())
                visibleRect = matrix.mapRect(visibleRect);
        }

        if (requiredTextureSize != m_pendingTextureSize)
            dirtyRect = boundsRect;
        else {
            // Clip the dirtyRect to the size of the layer to avoid drawing
            // outside the bounds of the backing texture.
            dirtyRect.intersect(boundsRect);
        }
    } else if (m_layer->contents()) {
        // Layer is a container, and it contains an Image.
        requiredTextureSize = m_layer->contents()->size();
        dirtyRect = IntRect(IntPoint::zero(), requiredTextureSize);
    }

    IntRect previousTextureRect(IntPoint::zero(), m_pendingTextureSize);
    if (m_pendingTextureSize != requiredTextureSize) {
        m_pendingTextureSize = requiredTextureSize;
        addTextureJob(TextureJob::resizeContents(m_pendingTextureSize));
    }

    bool contentsDirty = m_contentsDirty;
    if (m_contentsDirty) {
        // If we're not going to re-render all the covered tiles, mark those not covered as dirty.
        if (!visibleRect.contains(dirtyRect))
            addTextureJob(TextureJob::dirtyContents(dirtyRect));
        m_contentsDirty = false;
        m_dirtyRect = FloatRect();
    }

    // If we need display because we no longer need to be displayed, due to texture size becoming 0 x 0,
    // or if we're re-rendering the whole thing anyway, clear old texture jobs.
    if (requiredTextureSize.isEmpty() || dirtyRect == IntRect(IntPoint::zero(), requiredTextureSize))
        clearTextureJobs();

    if (visibleRect.isEmpty())
        return;

    HashSet<TileIndex> renderJobs;
    TileIndex first;
    TileIndex last;
    if (m_layer->contentsResolutionIndependent()) {
        // Resolution independent layers have all the needed data in the first tile.
        first = last = TileIndex(0, 0);
    } else {
        first = indexOfTile(flooredIntPoint(visibleRect.minXMinYCorner()));
        last = indexOfTile(ceiledIntPoint(visibleRect.maxXMaxYCorner()));
    }
    for (unsigned i = first.i(); i <= last.i(); ++i) {
        for (unsigned j = first.j(); j <= last.j(); ++j) {
            TileIndex index(i, j);
            IntRect tileRect = rectForTile(index, requiredTextureSize);
            if (tileRect.isEmpty())
                continue;

            if (m_backVisibility && m_backVisibility->tileNeedsRender(index)) {
#if DEBUG_LAYER_VISIBILITY
                printf("Tile at (%d, %d) needs render\n", index.i(), index.j());
#endif
                renderJobs.add(index);
                m_backVisibility->markTileAsRendered(index);
            } else if (contentsDirty && dirtyRect.intersects(tileRect))
                renderJobs.add(index);
        }
    }

    if (renderJobs.isEmpty())
        return;

    if (Image* image = m_layer->contents()) {
        // If we need backing, we have no choice but to enforce the tile size, which could cause clipping.
        // Otherwise, don't clip - include the whole image.
        IntSize bufferSize = m_needsBacking ? tileSize() : image->size();
        if (BlackBerry::Platform::Graphics::Buffer* buffer = createBuffer(bufferSize)) {
            IntRect contentsRect(IntPoint::zero(), image->size());
            m_layer->paintContents(buffer, contentsRect, scale);
            addTextureJob(TextureJob::setContents(buffer, contentsRect));
        }
    } else if (m_layer->drawsContent()) {
        for (HashSet<TileIndex>::iterator it = renderJobs.begin(); it != renderJobs.end(); ++it) {
            if (BlackBerry::Platform::Graphics::Buffer* buffer = createBuffer(tileSize())) {
                IntRect tileRect = rectForTile(*it, requiredTextureSize);
                m_layer->paintContents(buffer, tileRect, scale);
                addTextureJob(TextureJob::updateContents(buffer, tileRect));
            }
        }
    }
}

BlackBerry::Platform::Graphics::Buffer* LayerTiler::createBuffer(const IntSize& size)
{
    BlackBerry::Platform::Graphics::BufferType bufferType = m_needsBacking ? BlackBerry::Platform::Graphics::AlwaysBacked : BlackBerry::Platform::Graphics::BackedWhenNecessary;
    BlackBerry::Platform::Graphics::Buffer* buffer = BlackBerry::Platform::Graphics::createBuffer(size, bufferType);
    return buffer;
}

void LayerTiler::addTextureJob(const TextureJob& job)
{
    m_pendingTextureJobs.append(job);
}

void LayerTiler::clearTextureJobs()
{
    // Clear any committed texture jobs on next invocation of LayerTiler::commitPendingTextureUploads().
    m_clearTextureJobs = true;

    removeUpdateContentsJobs(m_pendingTextureJobs);
}

static size_t backingSizeInBytes = 0;

void LayerTiler::willCommit()
{
    backingSizeInBytes = 0;
}

void LayerTiler::commitPendingTextureUploads(LayerCompositingThread*)
{
    if (m_clearTextureJobs) {
        removeUpdateContentsJobs(m_textureJobs);
        m_clearTextureJobs = false;
    }

    // There's no point in rendering more than the cache capacity during one frame.
    const size_t maxBackingPerFrame = textureCacheCompositingThread()->memoryLimit();

    for (Vector<TextureJob>::iterator it = m_pendingTextureJobs.begin(); it != m_pendingTextureJobs.end(); ++it) {
        TextureJob& textureJob = *it;

        // Update backing surface for backed tiles now, to avoid dropping frames during animation.
        if (textureJob.m_contents && backingSizeInBytes < maxBackingPerFrame) {
            BlackBerry::Platform::Graphics::updateBufferBackingSurface(textureJob.m_contents);
            backingSizeInBytes += BlackBerry::Platform::Graphics::bufferSizeInBytes(textureJob.m_contents);
        }

        m_textureJobs.append(textureJob);
    }
    m_pendingTextureJobs.clear();
}

LayerVisibility* LayerTiler::swapFrontVisibility(LayerVisibility* visibility)
{
    return reinterpret_cast<LayerVisibility*>(_smp_xchg(reinterpret_cast<unsigned*>(&m_frontVisibility), reinterpret_cast<unsigned>(visibility)));
}

void LayerTiler::setFrontVisibility(const FloatRect& visibleRect, HashSet<TileIndex>& tilesNeedingRender)
{
    LayerVisibility* visibility = s_spareVisibility;
    if (visibility) {
        visibility->setVisibleRect(visibleRect);
        visibility->swapTilesNeedingRender(tilesNeedingRender);
    } else
        visibility = new LayerVisibility(visibleRect, tilesNeedingRender);
    s_spareVisibility = swapFrontVisibility(visibility);
}

void LayerTiler::layerVisibilityChanged(LayerCompositingThread*, bool visible)
{
    // For visible layers, we handle the tile-level visibility
    // in the draw loop, see LayerTiler::drawTextures().
    if (visible)
        return;

    HashSet<TileIndex> emptyTileSet;
    setFrontVisibility(FloatRect(), emptyTileSet);
}

void LayerTiler::uploadTexturesIfNeeded(LayerCompositingThread* layer)
{
    TileJobsMap tileJobsMap;
    Deque<TextureJob>::const_iterator textureJobIterEnd = m_textureJobs.end();
    for (Deque<TextureJob>::const_iterator textureJobIter = m_textureJobs.begin(); textureJobIter != textureJobIterEnd; ++textureJobIter)
        processTextureJob(*textureJobIter, tileJobsMap);

    TileJobsMap::const_iterator tileJobsIterEnd = tileJobsMap.end();
    for (TileJobsMap::const_iterator tileJobsIter = tileJobsMap.begin(); tileJobsIter != tileJobsIterEnd; ++tileJobsIter) {
        IntPoint origin = originOfTile(tileJobsIter->key);

        LayerTile* tile = m_tiles.get(tileJobsIter->key);
        if (!tile) {
            if (origin.x() >= m_requiredTextureSize.width() || origin.y() >= m_requiredTextureSize.height())
                continue;

            tile = new LayerTile();
            m_tiles.add(tileJobsIter->key, tile);
        }

        performTileJob(layer, tile, *tileJobsIter->value);
    }

    m_textureJobs.clear();
}

void LayerTiler::processTextureJob(const TextureJob& job, TileJobsMap& tileJobsMap)
{
    if (job.m_type == TextureJob::ResizeContents) {
        IntSize pendingTextureSize = job.m_dirtyRect.size();
        if (pendingTextureSize.width() < m_requiredTextureSize.width() || pendingTextureSize.height() < m_requiredTextureSize.height())
            pruneTextures();

        m_requiredTextureSize = pendingTextureSize;
        return;
    } else if (job.m_type == TextureJob::DirtyContents) {
        TileIndex first = indexOfTile(job.m_dirtyRect.minXMinYCorner());
        TileIndex last = indexOfTile(job.m_dirtyRect.maxXMaxYCorner());
        for (TileMap::iterator it = m_tiles.begin(); it != m_tiles.end(); ++it) {
            TileIndex index = (*it).key;
            if (index.i() >= first.i() && index.j() >= first.j() && index.i() <= last.i() && index.j() <= last.j())
                (*it).value->setContentsDirty();
        }
        return;
    }

    addTileJob(indexOfTile(job.m_dirtyRect.minXMinYCorner()), job, tileJobsMap);
}

void LayerTiler::addTileJob(const TileIndex& index, const TextureJob& job, TileJobsMap& tileJobsMap)
{
    // HashMap::add always returns a valid iterator even the key already exists.
    TileJobsMap::AddResult result = tileJobsMap.add(index, &job);

    // Successfully added the new job.
    if (result.isNewEntry)
        return;

    // Override the previous job.
    if (result.iterator->value && result.iterator->value->m_contents)
        BlackBerry::Platform::Graphics::destroyBuffer(result.iterator->value->m_contents);

    result.iterator->value = &job;
}

void LayerTiler::performTileJob(LayerCompositingThread* layer, LayerTile* tile, const TextureJob& job)
{
    switch (job.m_type) {
    case TextureJob::SetContents:
        tile->setContents(job.m_contents);
        return;
    case TextureJob::UpdateContents:
        tile->updateContents(job.m_contents, layer->contentsScale());
        return;
    case TextureJob::Unknown:
    case TextureJob::ResizeContents:
    case TextureJob::DirtyContents:
        ASSERT_NOT_REACHED();
        return;
    }
    ASSERT_NOT_REACHED();
}

bool LayerTiler::drawTile(LayerCompositingThread* layer, LayerTile* tile, const TileIndex& index, double scale, const FloatRect& dst, const FloatRect& dstClip)
{
    unsigned char globalAlpha = static_cast<unsigned char>(layer->drawOpacity() * 255);
    bool shouldDrawTile = dst.intersects(dstClip) && globalAlpha;

    // Even if the tile has wrong scale, it can be used as a "preview" if there is no risk of overlap with other tiles.
    bool tileHasCorrectScale = tile->contentsScale() == 0.0 || tile->contentsScale() == layer->contentsScale();
    if (!tileHasCorrectScale)
        shouldDrawTile = shouldDrawTile && (m_tiles.size() == 1 && !index.i() && !index.j());

    if (tile->hasTexture()) {
        LayerTexture* texture = tile->texture();
        textureCacheCompositingThread()->textureAccessed(texture);

        if (shouldDrawTile) {
            TransformationMatrix drawTransform = layer->drawTransform();
            drawTransform.translate(dst.x(), dst.y());
            if (layer->contentsResolutionIndependent())
                drawTransform.scaleNonUniform(dst.width() / m_requiredTextureSize.width(), dst.height() / m_requiredTextureSize.height());
            else if (tile->contentsScale())
                drawTransform.scale(1 / tile->contentsScale());
            if (layer->sizeIsScaleInvariant())
                drawTransform.scale(1 / scale);
            blitToBuffer(0, texture->buffer(), reinterpret_cast<BlackBerry::Platform::TransformationMatrix&>(drawTransform),
                BlackBerry::Platform::Graphics::SourceOver, globalAlpha);
        }
    }

    // Return false if the tile needs to be (re-)rendered.
    return !tile->isDirty() && tileHasCorrectScale;
}

static FloatRect inflateViewport(const FloatRect& viewport)
{
    // Everything is expressed in normalized device coordinates.
    FloatSize viewportSize(viewport.size());
    viewportSize.scale(viewportInflation.width(), viewportInflation.height());

    FloatSize viewportOffset(viewport.size());
    viewportOffset.scale(viewportInflation.width() - 1, viewportInflation.height() - 1);
    viewportOffset.scale(0.5);

    return FloatRect(viewport.location() - viewportOffset, viewportSize);
}

void LayerTiler::drawTextures(LayerCompositingThread* layer, const GLES2Program&, double scale, const FloatRect& clipRect)
{
    FloatRect boundsRect(-layer->origin(), layer->bounds());
    if (layer->sizeIsScaleInvariant())
        boundsRect.scale(1 / scale);

    FloatRect inflatedViewport = inflateViewport(clipRect);

    // Unprojecting points outside the transformed bounds gives poor numerical stability if there's a perspective effect,
    // so unproject the clipped transformed bounds instead.
    Vector<FloatPoint, 4> clipPolygon = intersectPolygonWithRect(layer->transformedBounds(), clipRect);
    if (clipPolygon.isEmpty()) {
        // The LayerRenderer figures we're visible, or it wouldn't have called into this method, but we beg to differ.
        layer->setVisible(false);
        return;
    }

    FloatRect normalizedLayerClipRect = FloatRect(0, 0, 1, 1);
    FloatRect normalizedLayerVisibleRect = FloatRect(0, 0, 1, 1);

    if (clipPolygon != layer->transformedBounds()) {
        Vector<FloatPoint, 4> normalizedLayerClipPolygon = unproject(layer, clipPolygon);
        normalizedLayerClipRect.intersect(boundingBox(normalizedLayerClipPolygon));

        Vector<FloatPoint, 4> visiblePolygon = intersectPolygonWithRect(layer->transformedBounds(), inflatedViewport);
        Vector<FloatPoint, 4> normalizedLayerVisiblePolygon = unproject(layer, visiblePolygon);
        normalizedLayerVisibleRect.intersect(boundingBox(normalizedLayerVisiblePolygon));
    }

    FloatSize tileSize;
    FloatRect dstClip;
    TileIndex first;
    TileIndex last;
    if (layer->contentsResolutionIndependent()) {
        // Resolution independent layers have all the needed data in the first tile
        tileSize = boundsRect.size();
        dstClip = boundsRect;
        first = last = TileIndex(0, 0);
    } else {
        FloatRect destinationRect = normalizedLayerVisibleRect;
        destinationRect.scale(m_requiredTextureSize.width(), m_requiredTextureSize.height());

        tileSize = m_tileSize;
        tileSize.scale(boundsRect.width() / m_requiredTextureSize.width(), boundsRect.height() / m_requiredTextureSize.height());

        dstClip = normalizedLayerClipRect;
        dstClip.scale(boundsRect.width(), boundsRect.height());
        dstClip.moveBy(boundsRect.location());

        // The layer contains pixels with coordinates (0 ... m_requiredTextureSize.width() - 1, 0 ... m_requiredTextureSize.height() - 1),
        // so clamp the destinationRect-to-tileindex calculation appropriately.
        first = indexOfTile(flooredIntPoint(destinationRect.minXMinYCorner()));
        last = indexOfTile(ceiledIntPoint(destinationRect.maxXMaxYCorner()).shrunkTo(IntPoint(-1, -1) + m_requiredTextureSize));
    }

    HashSet<TileIndex> tilesNeedingRender;
    for (unsigned i = first.i(); i <= last.i(); ++i) {
        for (unsigned j = first.j(); j <= last.j(); ++j) {
            TileIndex index(i, j);
            LayerTile* tile = m_tiles.get(index);
            if (!tile) {
                tile = new LayerTile();
                m_tiles.add(index, tile);
            }
            tile->setVisible(true);
            FloatPoint tileOrigin(index.i() * tileSize.width(), index.j() * tileSize.height());
            FloatRect dst(tileOrigin - 0.5 * boundsRect.size(), tileSize.shrunkTo(boundsRect.size() - toFloatSize(tileOrigin)));
            if (!drawTile(layer, tile, index, scale, dst, dstClip))
                tilesNeedingRender.add(index);
        }
    }

    for (TileMap::iterator it = m_tiles.begin(); it != m_tiles.end(); ++it) {
        TileIndex index = (*it).key;
        if (index.i() < first.i() || index.j() < first.j() || index.i() > last.i() || index.j() > last.j())
            (*it).value->setVisible(false);
    }

    // The visible rect will be used on the WebKit thread and should use the bounds expressed in
    // document coordinates, not corrected for scale invariance. So use "layer->bounds()" instead
    // of the local variable "boundsRect.size()".
    FloatRect layerVisibleRect = normalizedLayerVisibleRect;
    layerVisibleRect.scale(layer->bounds().width(), layer->bounds().height());

#if DEBUG_LAYER_VISIBILITY
    Vector<FloatPoint> v = unproject(layer, clipPolygon);
    for (size_t i = 0; i < v.size(); ++i) {
        v[i].scale(layer->bounds().width(), layer->bounds().height());
        v[i].move(toFloatSize(boundsRect.location()));
        v[i] = layer->drawTransform().mapPoint(v[i]);
    }

    // Use a thick border to make the border visible even for the root layer (which typically covers the screen width and/or height).
    unsigned tmp = reinterpret_cast<unsigned>(this);
    int t = rand_r(&tmp);
    glDisable(GL_SCISSOR_TEST);
    // When debugging, draw first layer->transformedBounds(), then clipPolygon, then v
    // to find out where the problem is (clipping to image plane, clipping to viewport or unprojecting).
    layer->layerRenderer()->drawDebugBorder(/* layer->transformedBounds() */ /* clipPolygon */ v, makeRGBAFromHSLA(static_cast<double>(t) / RAND_MAX, 1, 0.5, 1), 5);
    glEnable(GL_SCISSOR_TEST);
#endif

    // If we schedule a commit, visibility will be updated, and display will
    // happen if there are any visible and dirty textures.
    if (!tilesNeedingRender.isEmpty())
        layer->setNeedsCommit();

    setFrontVisibility(layerVisibleRect, tilesNeedingRender);
}

void LayerTiler::deleteTextures(LayerCompositingThread*)
{
    // Since textures are deleted by a synchronous message
    // from WebKit thread to compositing thread, we don't need
    // any synchronization mechanism here, even though we are
    // touching some WebKit thread state.
    if (m_tiles.size()) {
        for (TileMap::iterator it = m_tiles.begin(); it != m_tiles.end(); ++it)
            (*it).value->discardContents();
        m_tiles.clear();

        m_contentsDirty = true;
    }

    // For various reasons, e.g. page cache, someone may try
    // to render us after the textures were deleted.
    m_pendingTextureSize = IntSize();
    m_requiredTextureSize = IntSize();
}

void LayerTiler::pruneTextures()
{
    // Prune tiles that are no longer needed.
    Vector<TileIndex> tilesToDelete;
    for (TileMap::iterator it = m_tiles.begin(); it != m_tiles.end(); ++it) {
        TileIndex index = (*it).key;

        IntPoint origin = originOfTile(index);
        if (origin.x() >= m_requiredTextureSize.width() || origin.y() >= m_requiredTextureSize.height())
            tilesToDelete.append(index);
    }

    for (Vector<TileIndex>::iterator it = tilesToDelete.begin(); it != tilesToDelete.end(); ++it) {
        OwnPtr<LayerTile> tile = adoptPtr(m_tiles.take(*it));
        tile->discardContents();
    }
}

void LayerTiler::updateTileSize()
{
    IntSize size = m_layer->isMask() ? m_layer->bounds() : defaultTileSize();
    const IntSize maxTextureSize(2048, 2048);
    size = size.shrunkTo(maxTextureSize);

    if (m_tileSize == size || size.isEmpty())
        return;

    // Invalidate the whole layer if tile size changes.
    setNeedsDisplay();
    m_tileSize = size;
}

void LayerTiler::setNeedsBacking(bool needsBacking)
{
    if (m_needsBacking == needsBacking)
        return;

    m_needsBacking = needsBacking;
    updateTileSize();
}

void LayerTiler::scheduleCommit()
{
    ASSERT(isWebKitThread());

    if (m_layer)
        m_layer->setNeedsCommit();
}

TileIndex LayerTiler::indexOfTile(const WebCore::IntPoint& origin)
{
    int offsetX = origin.x();
    int offsetY = origin.y();
    if (offsetX)
        offsetX = offsetX / tileSize().width();
    if (offsetY)
        offsetY = offsetY / tileSize().height();
    return TileIndex(offsetX, offsetY);
}

IntPoint LayerTiler::originOfTile(const TileIndex& index)
{
    return IntPoint(index.i() * tileSize().width(), index.j() * tileSize().height());
}

IntRect LayerTiler::rectForTile(const TileIndex& index, const IntSize& bounds)
{
    IntPoint origin = originOfTile(index);
    IntSize size = tileSize().shrunkTo(bounds - toIntSize(origin));
    return IntRect(origin, size);
}

LayerTexture* LayerTiler::contentsTexture(LayerCompositingThread*)
{
    ASSERT(m_tiles.size() == 1);
    if (m_tiles.size() != 1)
        return 0;

    const LayerTile* tile = m_tiles.begin()->value;

    ASSERT(tile->hasTexture());
    if (!tile->hasTexture())
        return 0;

    return tile->texture();
}

} // namespace WebCore

#endif
