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

#ifndef LayerTile_h
#define LayerTile_h

#if USE(ACCELERATED_COMPOSITING)

#include "LayerTexture.h"

#include <wtf/RefPtr.h>

namespace WebCore {

class IntRect;
class TileIndex;

class LayerTile {
    WTF_MAKE_FAST_ALLOCATED;
public:
    LayerTile();
    ~LayerTile();

    LayerTexture* texture() const { return m_texture.get(); }

    bool isVisible() const { return m_visible; }
    void setVisible(bool);

    bool isDirty() const { return m_contentsDirty || !m_texture || m_texture->isDirty(); }

    bool hasTexture() const { return m_texture && m_texture->buffer(); }

    void setContents(BlackBerry::Platform::Graphics::Buffer*);
    void updateContents(BlackBerry::Platform::Graphics::Buffer*, double scale);
    void discardContents();

    // Returns 0 if contents are resolution independent or scale is simply unknown.
    double contentsScale() const { return m_scale; }

    // The texture contents are dirty due to appearance of page changing, or a change in contents scale.
    void setContentsDirty() { m_contentsDirty = true; }

private:
    void setTexture(PassRefPtr<LayerTexture>);

    // Never assign to m_texture directly, use setTexture() above.
    RefPtr<LayerTexture> m_texture;
    double m_scale;
    bool m_contentsDirty : 1;
    bool m_visible : 1;
};

}

#endif // USE(ACCELERATED_COMPOSITING)

#endif // Texture_h
