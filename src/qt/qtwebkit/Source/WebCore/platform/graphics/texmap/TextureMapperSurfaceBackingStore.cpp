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

#if USE(ACCELERATED_COMPOSITING) && USE(GRAPHICS_SURFACE)
#include "TextureMapperSurfaceBackingStore.h"

#include "GraphicsSurface.h"

namespace WebCore {

void TextureMapperSurfaceBackingStore::setGraphicsSurface(PassRefPtr<GraphicsSurface> surface)
{
    m_graphicsSurface = surface;
}

void TextureMapperSurfaceBackingStore::swapBuffersIfNeeded(uint32_t)
{
    if (m_graphicsSurface)
        m_graphicsSurface->swapBuffers();
}

PassRefPtr<BitmapTexture> TextureMapperSurfaceBackingStore::texture() const
{
    // FIXME: Instead of just returning an empty texture, we should wrap the texture contents into a BitmapTexture.
    RefPtr<BitmapTexture> emptyTexture;
    return emptyTexture;
}

void TextureMapperSurfaceBackingStore::paintToTextureMapper(TextureMapper* textureMapper, const FloatRect& targetRect, const TransformationMatrix& transform, float opacity)
{
    if (m_graphicsSurface)
        m_graphicsSurface->paintToTextureMapper(textureMapper, targetRect, transform, opacity);
}

} // namespace WebCore
#endif
