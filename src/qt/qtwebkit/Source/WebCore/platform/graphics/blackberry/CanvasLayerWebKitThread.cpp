/*
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
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
#include "CanvasLayerWebKitThread.h"

#include "LayerCompositingThread.h"

#if USE(ACCELERATED_COMPOSITING) && ENABLE(ACCELERATED_2D_CANVAS)

#include <BlackBerryPlatformGLES2Program.h>

using BlackBerry::Platform::Graphics::GLES2Program;

namespace WebCore {

void CanvasLayerWebKitThread::deleteTextures()
{
}

class CanvasLayerCompositingThreadClient : public LayerCompositingThreadClient {
public:
    CanvasLayerCompositingThreadClient(BlackBerry::Platform::Graphics::Buffer*, const IntSize&);

    void layerCompositingThreadDestroyed(LayerCompositingThread*) { }
    void layerVisibilityChanged(LayerCompositingThread*, bool) { }
    void uploadTexturesIfNeeded(LayerCompositingThread*) { }

    void drawTextures(LayerCompositingThread*, const GLES2Program&, double scale, const FloatRect& clipRect);
    void deleteTextures(LayerCompositingThread*);

    void commitPendingTextureUploads(LayerCompositingThread*);

    void clearBuffer() { m_buffer = 0; }

private:
    BlackBerry::Platform::Graphics::Buffer* m_buffer;
    IntSize m_size;
};

CanvasLayerCompositingThreadClient::CanvasLayerCompositingThreadClient(BlackBerry::Platform::Graphics::Buffer* buffer, const IntSize& size)
    : m_buffer(buffer)
    , m_size(size)
{
}

void CanvasLayerCompositingThreadClient::drawTextures(LayerCompositingThread* layer, const GLES2Program&, double, const FloatRect& /*clipRect*/)
{
    if (!m_buffer)
        return;

    TransformationMatrix dt = layer->drawTransform();
    dt.translate(-layer->bounds().width() / 2.0, -layer->bounds().height() / 2.0);
    dt.scaleNonUniform(static_cast<double>(layer->bounds().width()) / m_size.width(), static_cast<double>(layer->bounds().height()) / m_size.height());
    blitToBuffer(0, m_buffer, reinterpret_cast<BlackBerry::Platform::TransformationMatrix&>(dt),
        BlackBerry::Platform::Graphics::SourceOver, static_cast<unsigned char>(layer->drawOpacity() * 255));
}

void CanvasLayerCompositingThreadClient::deleteTextures(LayerCompositingThread*)
{
    // Nothing to do here, the buffer is not owned by us.
}

void CanvasLayerCompositingThreadClient::commitPendingTextureUploads(LayerCompositingThread*)
{
    if (!m_buffer)
        return;

    // This method is called during a synchronization point between WebKit and compositing thread.
    // This is our only chance to transfer the back display list to the front display list without
    // race conditions.
    // 1. Flush back display list to front
    BlackBerry::Platform::Graphics::releaseBufferDrawable(m_buffer);
    // 2. Draw front display list to FBO
    BlackBerry::Platform::Graphics::updateBufferBackingSurface(m_buffer);
}


CanvasLayerWebKitThread::CanvasLayerWebKitThread(BlackBerry::Platform::Graphics::Buffer* buffer, const IntSize& size)
    : LayerWebKitThread(CanvasLayer, 0)
{
    m_compositingThreadClient = new CanvasLayerCompositingThreadClient(buffer, size);
    layerCompositingThread()->setClient(m_compositingThreadClient);
}

CanvasLayerWebKitThread::~CanvasLayerWebKitThread()
{
    layerCompositingThread()->setClient(0);
    delete m_compositingThreadClient;
}

void CanvasLayerWebKitThread::clearBuffer(CanvasLayerWebKitThread* layer)
{
    layer->m_compositingThreadClient->clearBuffer();
}

}

#endif // USE(ACCELERATED_COMPOSITING) && ENABLE(ACCELERATED_2D_CANVAS)
