/*
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
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

#include "LayerRendererSurface.h"

#include "LayerCompositingThread.h"
#include "LayerRenderer.h"
#include "LayerUtilities.h"
#include "TextureCacheCompositingThread.h"

namespace WebCore {

LayerRendererSurface::LayerRendererSurface(LayerRenderer* renderer, LayerCompositingThread* owner)
    : m_ownerLayer(owner)
    , m_layerRenderer(renderer)
    , m_opacity(1.0)
{
}

LayerRendererSurface::~LayerRendererSurface()
{
}

void LayerRendererSurface::setContentRect(const IntRect& contentRect)
{
    m_contentRect = contentRect;
    m_size = contentRect.size();
}

FloatRect LayerRendererSurface::boundingBox() const
{
    FloatRect rect = WebCore::boundingBox(transformedBounds());

    if (m_ownerLayer->replicaLayer())
        rect.unite(m_replicaDrawTransform.mapQuad(FloatRect(-origin(), size())).boundingBox());

    return rect;
}

Vector<FloatPoint, 4> LayerRendererSurface::transformedBounds() const
{
    return toVector<FloatPoint, 4>(m_drawTransform.mapQuad(FloatRect(-origin(), size())));
}

bool LayerRendererSurface::ensureTexture()
{
    if (!m_texture)
        m_texture = textureCacheCompositingThread()->createTexture();

    return m_texture->protect(m_size, BlackBerry::Platform::Graphics::AlwaysBacked);
}

void LayerRendererSurface::releaseTexture()
{
    m_texture->unprotect();
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
