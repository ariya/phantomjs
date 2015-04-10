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

#include "LayerTile.h"

#include "TextureCacheCompositingThread.h"

namespace WebCore {

LayerTile::LayerTile()
    : m_scale(0)
    , m_contentsDirty(false)
    , m_visible(false)
{
}

LayerTile::~LayerTile()
{
    // Make sure to unprotect the texture if needed.
    setVisible(false);
}

void LayerTile::setContents(BlackBerry::Platform::Graphics::Buffer* contents)
{
    m_scale = 0; // Resolution independent
    setTexture(textureCacheCompositingThread()->textureForContents(contents));
}

void LayerTile::updateContents(BlackBerry::Platform::Graphics::Buffer* contents, double scale)
{
    m_scale = scale;
    setTexture(textureCacheCompositingThread()->updateContents(m_texture, contents));
}

void LayerTile::discardContents()
{
    m_scale = 0; // Unknown scale
    setTexture(0);
}

void LayerTile::setVisible(bool visible)
{
    if (visible == m_visible)
        return;

    m_visible = visible;

    if (!m_texture)
        return;

    // Protect the texture from being evicted from cache
    // if we are visible.
    if (visible)
        m_texture->protect();
    else
        m_texture->unprotect();
}

void LayerTile::setTexture(PassRefPtr<LayerTexture> texture)
{
    // Clear this flag regardless of the value of the texture parameter.
    // If it's 0, isDirty() will return true anyway.
    // If it's the same texture, perhaps there was no more detailed texture
    // available than the one we already had, and keeping the dirty flag will
    // result in an endless loop of updating to the same texture.
    m_contentsDirty = false;

    if (texture == m_texture)
        return;

    // Move protection over to the new texture.
    if (m_visible) {
        if (m_texture)
            m_texture->unprotect();
        if (texture)
            texture->protect();
    }

    m_texture = texture;
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
