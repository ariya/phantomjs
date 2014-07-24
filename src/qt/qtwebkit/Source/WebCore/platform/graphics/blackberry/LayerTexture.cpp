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
#include "LayerTexture.h"

#if USE(ACCELERATED_COMPOSITING)

#include "Color.h"
#include "IntRect.h"
#include "TextureCacheCompositingThread.h"

#include <BlackBerryPlatformGLES2ContextState.h>
#include <BlackBerryPlatformGraphicsContext.h>
#include <GLES2/gl2.h>
#include <wtf/CurrentTime.h>
#include <wtf/OwnArrayPtr.h>

namespace WebCore {

LayerTexture::LayerTexture(bool isColor)
    : m_protectionCount(0)
    , m_buffer(0)
    , m_isColor(isColor)
    , m_bufferSizeInBytes(0)
{
    textureCacheCompositingThread()->install(this, IntSize(), BlackBerry::Platform::Graphics::BackedWhenNecessary);
}

LayerTexture::~LayerTexture()
{
    textureCacheCompositingThread()->textureDestroyed(this);
}

void LayerTexture::updateContents(BlackBerry::Platform::Graphics::Buffer* buffer)
{
    if (m_buffer)
        BlackBerry::Platform::Graphics::destroyBuffer(m_buffer);
    m_buffer = buffer;
    size_t newBufferSizeInBytes = BlackBerry::Platform::Graphics::bufferSizeInBytes(m_buffer);
    textureCacheCompositingThread()->textureSizeInBytesChanged(this, newBufferSizeInBytes - m_bufferSizeInBytes);
    m_bufferSizeInBytes = newBufferSizeInBytes;
}

void LayerTexture::setContentsToColor(const Color& color)
{
    RGBA32 rgba = color.rgb();

    if (m_buffer)
        BlackBerry::Platform::Graphics::destroyBuffer(m_buffer);

    m_buffer = BlackBerry::Platform::Graphics::createBuffer(IntSize(1, 1), BlackBerry::Platform::Graphics::BackedWhenNecessary);
    if (BlackBerry::Platform::Graphics::PlatformGraphicsContext* gc = lockBufferDrawable(m_buffer)) {
        gc->setFillColor(rgba);
        gc->addFillRect(BlackBerry::Platform::FloatRect(0, 0, 1, 1));
    }
    releaseBufferDrawable(m_buffer);

    IntSize oldSize = m_size;
    m_size = IntSize(1, 1);
    if (m_size != oldSize)
        textureCacheCompositingThread()->textureResized(this, oldSize);
}

bool LayerTexture::protect(const IntSize& size, BlackBerry::Platform::Graphics::BufferType type)
{
    if (!m_buffer) {
        // We may have been evicted by the TextureCacheCompositingThread,
        // attempt to install us again.
        if (!textureCacheCompositingThread()->install(this, size, type))
            return false;
    }

    ++m_protectionCount;

    if (m_size == size)
        return true;

    textureCacheCompositingThread()->resizeTexture(this, size, type);

    return true;
}

Platform3DObject LayerTexture::platformTexture() const
{
    using namespace BlackBerry::Platform::Graphics;

    Platform3DObject platformTexture = reinterpret_cast<Platform3DObject>(platformBufferHandle(m_buffer));

    // Force creation if it's 0.
    if (!platformTexture) {
        // This call can cause display list to render to backing, which can mutate a lot of GL state.
        GLES2ContextState::VertexAttributeStateSaver vertexAttribStateSaver;
        GLES2ContextState::ProgramStateSaver programSaver;
        GLES2ContextState::TextureAndFBOStateSaver textureSaver;

        lockAndBindBufferGLTexture(m_buffer, GL_TEXTURE_2D);
        platformTexture = reinterpret_cast<Platform3DObject>(platformBufferHandle(m_buffer));
    }

    return platformTexture;
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
