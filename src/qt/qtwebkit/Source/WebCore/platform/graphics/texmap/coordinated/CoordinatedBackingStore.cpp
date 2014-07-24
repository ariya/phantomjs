/*
 Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

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
#include "CoordinatedBackingStore.h"

#if USE(COORDINATED_GRAPHICS)
#include "CoordinatedSurface.h"
#include "GraphicsLayer.h"
#include "TextureMapper.h"
#include "TextureMapperGL.h"

namespace WebCore {

void CoordinatedBackingStoreTile::swapBuffers(TextureMapper* textureMapper)
{
    if (!m_surface)
        return;

    FloatRect tileRect(m_tileRect);
    tileRect.scale(1. / m_scale);
    bool shouldReset = false;
    if (tileRect != rect()) {
        setRect(tileRect);
        shouldReset = true;
    }
    RefPtr<BitmapTexture> texture = this->texture();
    if (!texture) {
        texture = textureMapper->createTexture();
        setTexture(texture.get());
        shouldReset = true;
    }

    ASSERT(textureMapper->maxTextureSize().width() >= m_tileRect.size().width());
    ASSERT(textureMapper->maxTextureSize().height() >= m_tileRect.size().height());
    if (shouldReset)
        texture->reset(m_tileRect.size(), m_surface->supportsAlpha());

    m_surface->copyToTexture(texture, m_sourceRect, m_surfaceOffset);
    m_surface.clear();
}

void CoordinatedBackingStoreTile::setBackBuffer(const IntRect& tileRect, const IntRect& sourceRect, PassRefPtr<CoordinatedSurface> buffer, const IntPoint& offset)
{
    m_sourceRect = sourceRect;
    m_tileRect = tileRect;
    m_surfaceOffset = offset;
    m_surface = buffer;
}

void CoordinatedBackingStore::createTile(uint32_t id, float scale)
{
    m_tiles.add(id, CoordinatedBackingStoreTile(scale));
    m_scale = scale;
}

void CoordinatedBackingStore::removeTile(uint32_t id)
{
    ASSERT(m_tiles.contains(id));
    m_tilesToRemove.add(id);
}

void CoordinatedBackingStore::removeAllTiles()
{
    CoordinatedBackingStoreTileMap::iterator end = m_tiles.end();
    for (CoordinatedBackingStoreTileMap::iterator it = m_tiles.begin(); it != end; ++it)
        m_tilesToRemove.add(it->key);
}

void CoordinatedBackingStore::updateTile(uint32_t id, const IntRect& sourceRect, const IntRect& tileRect, PassRefPtr<CoordinatedSurface> backBuffer, const IntPoint& offset)
{
    CoordinatedBackingStoreTileMap::iterator it = m_tiles.find(id);
    ASSERT(it != m_tiles.end());
    it->value.setBackBuffer(tileRect, sourceRect, backBuffer, offset);
}

PassRefPtr<BitmapTexture> CoordinatedBackingStore::texture() const
{
    CoordinatedBackingStoreTileMap::const_iterator end = m_tiles.end();
    for (CoordinatedBackingStoreTileMap::const_iterator it = m_tiles.begin(); it != end; ++it) {
        RefPtr<BitmapTexture> texture = it->value.texture();
        if (texture)
            return texture;
    }

    return PassRefPtr<BitmapTexture>();
}

void CoordinatedBackingStore::setSize(const FloatSize& size)
{
    m_pendingSize = size;
}

void CoordinatedBackingStore::paintTilesToTextureMapper(Vector<TextureMapperTile*>& tiles, TextureMapper* textureMapper, const TransformationMatrix& transform, float opacity, const FloatRect& rect)
{
    for (size_t i = 0; i < tiles.size(); ++i)
        tiles[i]->paint(textureMapper, transform, opacity, calculateExposedTileEdges(rect, tiles[i]->rect()));
}

TransformationMatrix CoordinatedBackingStore::adjustedTransformForRect(const FloatRect& targetRect)
{
    return TransformationMatrix::rectToRect(rect(), targetRect);
}

void CoordinatedBackingStore::paintToTextureMapper(TextureMapper* textureMapper, const FloatRect& targetRect, const TransformationMatrix& transform, float opacity)
{
    if (m_tiles.isEmpty())
        return;
    ASSERT(!m_size.isZero());

    Vector<TextureMapperTile*> tilesToPaint;
    Vector<TextureMapperTile*> previousTilesToPaint;

    // We have to do this every time we paint, in case the opacity has changed.
    CoordinatedBackingStoreTileMap::iterator end = m_tiles.end();
    FloatRect coveredRect;
    for (CoordinatedBackingStoreTileMap::iterator it = m_tiles.begin(); it != end; ++it) {
        CoordinatedBackingStoreTile& tile = it->value;
        if (!tile.texture())
            continue;

        if (tile.scale() == m_scale) {
            tilesToPaint.append(&tile);
            coveredRect.unite(tile.rect());
            continue;
        }

        // Only show the previous tile if the opacity is high, otherwise effect looks like a bug.
        // We show the previous-scale tile anyway if it doesn't intersect with any current-scale tile.
        if (opacity < 0.95 && coveredRect.intersects(tile.rect()))
            continue;

        previousTilesToPaint.append(&tile);
    }

    // targetRect is on the contents coordinate system, so we must compare two rects on the contents coordinate system.
    // See TiledBackingStore.
    TransformationMatrix adjustedTransform = transform * adjustedTransformForRect(targetRect);

    paintTilesToTextureMapper(previousTilesToPaint, textureMapper, adjustedTransform, opacity, rect());
    paintTilesToTextureMapper(tilesToPaint, textureMapper, adjustedTransform, opacity, rect());
}

void CoordinatedBackingStore::drawBorder(TextureMapper* textureMapper, const Color& borderColor, float borderWidth, const FloatRect& targetRect, const TransformationMatrix& transform)
{
    TransformationMatrix adjustedTransform = transform * adjustedTransformForRect(targetRect);
    CoordinatedBackingStoreTileMap::iterator end = m_tiles.end();
    for (CoordinatedBackingStoreTileMap::iterator it = m_tiles.begin(); it != end; ++it)
        textureMapper->drawBorder(borderColor, borderWidth, it->value.rect(), adjustedTransform);
}

void CoordinatedBackingStore::drawRepaintCounter(TextureMapper* textureMapper, int repaintCount, const Color& borderColor, const FloatRect& targetRect, const TransformationMatrix& transform)
{
    TransformationMatrix adjustedTransform = transform * adjustedTransformForRect(targetRect);
    CoordinatedBackingStoreTileMap::iterator end = m_tiles.end();
    for (CoordinatedBackingStoreTileMap::iterator it = m_tiles.begin(); it != end; ++it)
        textureMapper->drawNumber(repaintCount, borderColor, it->value.rect().location(), adjustedTransform);
}

void CoordinatedBackingStore::commitTileOperations(TextureMapper* textureMapper)
{
    if (!m_pendingSize.isZero()) {
        m_size = m_pendingSize;
        m_pendingSize = FloatSize();
    }

    HashSet<uint32_t>::iterator tilesToRemoveEnd = m_tilesToRemove.end();
    for (HashSet<uint32_t>::iterator it = m_tilesToRemove.begin(); it != tilesToRemoveEnd; ++it)
        m_tiles.remove(*it);
    m_tilesToRemove.clear();

    CoordinatedBackingStoreTileMap::iterator tilesEnd = m_tiles.end();
    for (CoordinatedBackingStoreTileMap::iterator it = m_tiles.begin(); it != tilesEnd; ++it)
        it->value.swapBuffers(textureMapper);
}

} // namespace WebCore
#endif // USE(COORDINATED_GRAPHICS)
