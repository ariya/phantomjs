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
#include "WebGLLayerWebKitThread.h"

#if USE(ACCELERATED_COMPOSITING) && ENABLE(WEBGL)

#include "GraphicsContext3D.h"
#include <pthread.h>

namespace WebCore {

WebGLLayerWebKitThread::WebGLLayerWebKitThread()
    : EGLImageLayerWebKitThread(WebGLLayer)
    , m_webGLContext(0)
{
}

WebGLLayerWebKitThread::~WebGLLayerWebKitThread()
{
    deleteTextures();
}

void WebGLLayerWebKitThread::updateTextureContentsIfNeeded()
{
    if (!m_webGLContext || !m_webGLContext->makeContextCurrent())
        return;

    m_webGLContext->prepareTexture();

    updateFrontBuffer(m_webGLContext->getInternalFramebufferSize(), m_webGLContext->platformTexture());
}

void WebGLLayerWebKitThread::deleteTextures()
{
    if (m_webGLContext && m_webGLContext->makeContextCurrent())
        deleteFrontBuffer();
}

void WebGLLayerWebKitThread::webGLContextDestroyed()
{
    deleteTextures();
    m_webGLContext = 0;
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING) && ENABLE(WEBGL)
