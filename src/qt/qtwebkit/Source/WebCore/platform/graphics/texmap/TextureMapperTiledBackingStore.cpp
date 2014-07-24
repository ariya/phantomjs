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

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)
#include "TextureMapperTiledBackingStore.h"

#include "ImageBuffer.h"
#include "TextureMapper.h"

namespace WebCore {

class GraphicsLayer;

TextureMapperTiledBackingStore::TextureMapperTiledBackingStore()
{
}

void TextureMapperTiledBackingStore::updateContentsFromImageIfNeeded(TextureMapper* textureMapper)
{
    if (!m_image)
        return;

    updateContents(textureMapper, m_image.get(), m_image->size(), m_image->rect(), BitmapTexture::UpdateCannotModifyOriginalImageData);
    m_image.clear();
}

TransformationMatrix TextureMapperTiledBackingStore::adjustedTransformForRect(const FloatRect& targetRect)
{
    return TransformationMatrix::rectToRect(rect(), targetRect);
}

void TextureMapperTiledBackingStore::paintToTextureMapper(TextureMapper* textureMapper, const FloatRect& targetRect, const TransformationMatrix& transform, float opacity)
{
    updateContentsFromImageIfNeeded(textureMapper);
    TransformationMatrix adjustedTransform = transform * adjustedTransformForRect(targetRect);
    for (size_t i = 0; i < m_tiles.size(); ++i)
        m_tiles[i].paint(textureMapper, adjustedTransform, opacity, calculateExposedTileEdges(rect(), m_tiles[i].rect()));
}

void TextureMapperTiledBackingStore::drawBorder(TextureMapper* textureMapper, const Color& borderColor, float borderWidth, const FloatRect& targetRect, const TransformationMatrix& transform)
{
    TransformationMatrix adjustedTransform = transform * adjustedTransformForRect(targetRect);
    for (size_t i = 0; i < m_tiles.size(); ++i)
        textureMapper->drawBorder(borderColor, borderWidth, m_tiles[i].rect(), adjustedTransform);
}

void TextureMapperTiledBackingStore::drawRepaintCounter(TextureMapper* textureMapper, int repaintCount, const Color& borderColor, const FloatRect& targetRect, const TransformationMatrix& transform)
{
    TransformationMatrix adjustedTransform = transform * adjustedTransformForRect(targetRect);
    for (size_t i = 0; i < m_tiles.size(); ++i)
        textureMapper->drawNumber(repaintCount, borderColor, m_tiles[i].rect().location(), adjustedTransform);
}

void TextureMapperTiledBackingStore::createOrDestroyTilesIfNeeded(const FloatSize& size, const IntSize& tileSize, bool hasAlpha, IntRect& repaintRect)
{
    if (size == m_size)
        return;
    m_size = size;

    Vector<FloatRect> tileRectsToAdd;
    Vector<int> tileIndicesToRemove;
    static const size_t TileEraseThreshold = 6;

    // This method recycles tiles. We check which tiles we need to add, which to remove, and use as many
    // removable tiles as replacement for new tiles when possible.
    for (float y = 0; y < m_size.height(); y += tileSize.height()) {
        for (float x = 0; x < m_size.width(); x += tileSize.width()) {
            FloatRect tileRect(x, y, tileSize.width(), tileSize.height());
            tileRect.intersect(rect());
            tileRectsToAdd.append(tileRect);
        }
    }

    // Check which tiles need to be removed, and which already exist.
    for (int i = m_tiles.size() - 1; i >= 0; --i) {
        FloatRect oldTile = m_tiles[i].rect();
        bool existsAlready = false;

        for (int j = tileRectsToAdd.size() - 1; j >= 0; --j) {
            FloatRect newTile = tileRectsToAdd[j];
            if (oldTile != newTile)
                continue;

            // A tile that we want to add already exists, no need to add or remove it.
            existsAlready = true;
            tileRectsToAdd.remove(j);
            break;
        }

        // This tile is not needed.
        if (!existsAlready) {
            tileIndicesToRemove.append(i);
            // If the removed tile was painted ensure the area it covered is repainted.
            if (m_tiles[i].texture())
                repaintRect.unite(enclosingIntRect(oldTile));
        }
    }

    // Recycle removable tiles to be used for newly requested tiles.
    for (size_t i = 0; i < tileRectsToAdd.size(); ++i) {
        if (!tileIndicesToRemove.isEmpty()) {
            // We recycle an existing tile for usage with a new tile rect.
            TextureMapperTile& tile = m_tiles[tileIndicesToRemove.last()];
            tileIndicesToRemove.removeLast();
            tile.setRect(tileRectsToAdd[i]);

            if (tile.texture())
                tile.texture()->reset(enclosingIntRect(tile.rect()).size(), hasAlpha ? BitmapTexture::SupportsAlpha : 0);
            continue;
        }

        m_tiles.append(TextureMapperTile(tileRectsToAdd[i]));
    }

    // Remove unnecessary tiles, if they weren't recycled.
    // We use a threshold to make sure we don't create/destroy tiles too eagerly.
    for (size_t i = 0; i < tileIndicesToRemove.size() && m_tiles.size() > TileEraseThreshold; ++i)
        m_tiles.remove(tileIndicesToRemove[i]);
}

void TextureMapperTiledBackingStore::updateContents(TextureMapper* textureMapper, Image* image, const FloatSize& totalSize, const IntRect& dirtyRect, BitmapTexture::UpdateContentsFlag updateContentsFlag)
{
    IntRect repaintRect(dirtyRect);
    createOrDestroyTilesIfNeeded(totalSize, textureMapper->maxTextureSize(), !image->currentFrameKnownToBeOpaque(), repaintRect);
    for (size_t i = 0; i < m_tiles.size(); ++i)
        m_tiles[i].updateContents(textureMapper, image, repaintRect, updateContentsFlag);
}

void TextureMapperTiledBackingStore::updateContents(TextureMapper* textureMapper, GraphicsLayer* sourceLayer, const FloatSize& totalSize, const IntRect& dirtyRect, BitmapTexture::UpdateContentsFlag updateContentsFlag)
{
    IntRect repaintRect(dirtyRect);
    createOrDestroyTilesIfNeeded(totalSize, textureMapper->maxTextureSize(), true, repaintRect);
    for (size_t i = 0; i < m_tiles.size(); ++i)
        m_tiles[i].updateContents(textureMapper, sourceLayer, repaintRect, updateContentsFlag);
}

PassRefPtr<BitmapTexture> TextureMapperTiledBackingStore::texture() const
{
    for (size_t i = 0; i < m_tiles.size(); ++i) {
        RefPtr<BitmapTexture> texture = m_tiles[i].texture();
        if (texture)
            return texture;
    }

    return PassRefPtr<BitmapTexture>();
}

} // namespace WebCore
#endif
