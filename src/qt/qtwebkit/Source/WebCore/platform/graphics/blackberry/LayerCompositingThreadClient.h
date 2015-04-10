/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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

#ifndef LayerCompositingThreadClient_h
#define LayerCompositingThreadClient_h

#if USE(ACCELERATED_COMPOSITING)

namespace BlackBerry {
namespace Platform {
namespace Graphics {
class GLES2Program;
}
}
}
namespace WebCore {

class FloatRect;
class LayerCompositingThread;
class LayerTexture;

class LayerCompositingThreadClient {
public:
    virtual ~LayerCompositingThreadClient() { }

    virtual void layerCompositingThreadDestroyed(LayerCompositingThread*) = 0;

    virtual void layerVisibilityChanged(LayerCompositingThread*, bool visible) = 0;

    virtual void uploadTexturesIfNeeded(LayerCompositingThread*) = 0;
    virtual void drawTextures(LayerCompositingThread*, const BlackBerry::Platform::Graphics::GLES2Program&, double scale, const FloatRect& clipRect) = 0;
    virtual void deleteTextures(LayerCompositingThread*) = 0;

    // Optional. Allows layers to serve as a mask for other layers
    virtual LayerTexture* contentsTexture(LayerCompositingThread*) { return 0; }

    // Optional. Allow the layer to commit thread sensitive content during a sync point
    virtual void commitPendingTextureUploads(LayerCompositingThread*) { }

    // Unlike the other methods here, this one will be called on the WebKit thread
    virtual void scheduleCommit() { }
};

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)

#endif
