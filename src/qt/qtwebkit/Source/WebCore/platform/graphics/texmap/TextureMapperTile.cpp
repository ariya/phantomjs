/*
 Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies)

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
#include "TextureMapperTile.h"

#include "Image.h"
#include "TextureMapper.h"
#include <wtf/RefPtr.h>

namespace WebCore {

class GraphicsLayer;

void TextureMapperTile::updateContents(TextureMapper* textureMapper, Image* image, const IntRect& dirtyRect, BitmapTexture::UpdateContentsFlag updateContentsFlag)
{
    IntRect targetRect = enclosingIntRect(m_rect);
    targetRect.intersect(dirtyRect);
    if (targetRect.isEmpty())
        return;
    IntPoint sourceOffset = targetRect.location();

    // Normalize sourceRect to the buffer's coordinates.
    sourceOffset.move(-dirtyRect.x(), -dirtyRect.y());

    // Normalize targetRect to the texture's coordinates.
    targetRect.move(-m_rect.x(), -m_rect.y());
    if (!m_texture) {
        m_texture = textureMapper->createTexture();
        m_texture->reset(targetRect.size(), image->currentFrameKnownToBeOpaque() ? 0 : BitmapTexture::SupportsAlpha);
    }

    m_texture->updateContents(image, targetRect, sourceOffset, updateContentsFlag);
}

void TextureMapperTile::updateContents(TextureMapper* textureMapper, GraphicsLayer* sourceLayer, const IntRect& dirtyRect, BitmapTexture::UpdateContentsFlag updateContentsFlag)
{
    IntRect targetRect = enclosingIntRect(m_rect);
    targetRect.intersect(dirtyRect);
    if (targetRect.isEmpty())
        return;
    IntPoint sourceOffset = targetRect.location();

    // Normalize targetRect to the texture's coordinates.
    targetRect.move(-m_rect.x(), -m_rect.y());

    if (!m_texture) {
        m_texture = textureMapper->createTexture();
        m_texture->reset(targetRect.size(), BitmapTexture::SupportsAlpha);
    }

    m_texture->updateContents(textureMapper, sourceLayer, targetRect, sourceOffset, updateContentsFlag);
}

void TextureMapperTile::paint(TextureMapper* textureMapper, const TransformationMatrix& transform, float opacity, const unsigned exposedEdges)
{
    if (texture().get())
        textureMapper->drawTexture(*texture().get(), rect(), transform, opacity, exposedEdges);
}

} // namespace WebCore
#endif 
