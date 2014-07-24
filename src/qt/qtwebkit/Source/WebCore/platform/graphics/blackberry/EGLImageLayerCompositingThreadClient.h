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

#ifndef EGLImageLayerCompositingThreadClient_h
#define EGLImageLayerCompositingThreadClient_h

#if USE(ACCELERATED_COMPOSITING)

#include "LayerCompositingThreadClient.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/ThreadSafeRefCounted.h>

namespace BlackBerry {
namespace Platform {
namespace Graphics {
class GLES2SharedTextureAccessor;
}
}
}

namespace WebCore {

class LayerCompositingThread;

class EGLImageLayerCompositingThreadClient : public ThreadSafeRefCounted<EGLImageLayerCompositingThreadClient>, public LayerCompositingThreadClient {
public:
    static PassRefPtr<EGLImageLayerCompositingThreadClient> create()
    {
        return adoptRef(new EGLImageLayerCompositingThreadClient());
    }

    virtual ~EGLImageLayerCompositingThreadClient();

    virtual void layerCompositingThreadDestroyed(LayerCompositingThread*)
    {
        deref(); // Matched by ref() in constructor
    }

    virtual void layerVisibilityChanged(LayerCompositingThread*, bool visible) { }

    virtual void uploadTexturesIfNeeded(LayerCompositingThread*);
    virtual void drawTextures(LayerCompositingThread*, const BlackBerry::Platform::Graphics::GLES2Program&, double scale, const FloatRect& clipRect);
    virtual void deleteTextures(LayerCompositingThread*);

    virtual void bindContentsTexture(LayerCompositingThread*);

    void setTextureAccessor(BlackBerry::Platform::Graphics::GLES2SharedTextureAccessor*);

private:
    EGLImageLayerCompositingThreadClient()
        : m_textureAccessor(0)
    {
        ref(); // Matched by deref() in layerCompositingThreadDestroyed()
    }

    BlackBerry::Platform::Graphics::GLES2SharedTextureAccessor* m_textureAccessor;
};

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)

#endif // EGLImageLayerCompositingThreadClient_h
