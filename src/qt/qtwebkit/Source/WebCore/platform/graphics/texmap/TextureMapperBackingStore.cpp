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
#include "TextureMapperBackingStore.h"

#include "GraphicsLayer.h"
#include "ImageBuffer.h"
#include "TextureMapper.h"

#if USE(GRAPHICS_SURFACE)
#include "GraphicsSurface.h"
#include "TextureMapperGL.h"
#endif

namespace WebCore {

unsigned TextureMapperBackingStore::calculateExposedTileEdges(const FloatRect& totalRect, const FloatRect& tileRect)
{
    unsigned exposedEdges = TextureMapper::NoEdges;
    if (!tileRect.x())
        exposedEdges |= TextureMapper::LeftEdge;
    if (!tileRect.y())
        exposedEdges |= TextureMapper::TopEdge;
    if (tileRect.width() + tileRect.x() >= totalRect.width())
        exposedEdges |= TextureMapper::RightEdge;
    if (tileRect.height() + tileRect.y() >= totalRect.height())
        exposedEdges |= TextureMapper::BottomEdge;
    return exposedEdges;
}

}
#endif
