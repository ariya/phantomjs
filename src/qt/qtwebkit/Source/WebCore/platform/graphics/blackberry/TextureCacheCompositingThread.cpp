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
#include "TextureCacheCompositingThread.h"

#if USE(ACCELERATED_COMPOSITING)

#include "IntRect.h"

#include <GLES2/gl2.h>
#include <wtf/Assertions.h>

using BlackBerry::Platform::Graphics::Buffer;
using BlackBerry::Platform::Graphics::BufferType;

#define DEBUG_TEXTURE_MEMORY_USAGE 0

namespace WebCore {

static const int defaultMemoryLimit = 64 * 1024 * 1024; // Measured in bytes.

// Used to protect a newly created texture from being immediately evicted
// before someone has a chance to protect it for legitimate reasons.
class TextureProtector {
public:
    TextureProtector(LayerTexture* texture)
        : m_texture(texture)
    {
        m_texture->protect();
    }

    ~TextureProtector()
    {
        m_texture->unprotect();
    }

private:
    LayerTexture* m_texture;
};

TextureCacheCompositingThread::TextureCacheCompositingThread()
    : m_memoryUsage(0)
    , m_memoryLimit(defaultMemoryLimit)
{
}

Buffer* TextureCacheCompositingThread::createBuffer(const IntSize& size, BufferType type)
{
    return BlackBerry::Platform::Graphics::createBuffer(size, type);
}

void TextureCacheCompositingThread::collectGarbage()
{
    for (Garbage::iterator it = m_garbage.begin(); it != m_garbage.end(); ++it)
        BlackBerry::Platform::Graphics::destroyBuffer((*it).buffer);

    m_garbage.clear();
}

void TextureCacheCompositingThread::textureResized(LayerTexture* texture, const IntSize& oldSize)
{
    int delta = (texture->width() * texture->height() - oldSize.width() * oldSize.height()) * LayerTexture::bytesPerPixel();
    incMemoryUsage(delta);
    if (delta > 0)
        prune();
}

void TextureCacheCompositingThread::textureSizeInBytesChanged(LayerTexture*, int delta)
{
    incMemoryUsage(delta);
    if (delta > 0)
        prune();
}

void TextureCacheCompositingThread::textureDestroyed(LayerTexture* texture)
{
    if (texture->isColor()) {
        m_garbage.append(ZombieTexture(texture));
        return;
    }

    TextureSet::iterator it = m_textures.find(texture);
    evict(it);
    m_textures.remove(it);
}

bool TextureCacheCompositingThread::install(LayerTexture* texture, const IntSize& size, BufferType type)
{
    if (!texture)
        return true;

    if (!texture->buffer() && !size.isEmpty()) {
        Buffer* buffer = createBuffer(size, type);
        ASSERT(buffer);

        texture->setBuffer(buffer);
        texture->setSize(size);
        textureResized(texture, IntSize());
    }

    if (!texture->isColor())
        m_textures.add(texture);

    return true;
}

bool TextureCacheCompositingThread::resizeTexture(LayerTexture* texture, const IntSize& size, BufferType type)
{
    IntSize oldSize = texture->size();

    // Reallocate the buffer
    Buffer* buffer = createBuffer(size, type);
    ASSERT(buffer);

    BlackBerry::Platform::Graphics::destroyBuffer(texture->buffer());
    texture->setBuffer(buffer);
    texture->setSize(size);
    textureResized(texture, oldSize);
    return true;
}

void TextureCacheCompositingThread::evict(const TextureSet::iterator& it)
{
    if (it == m_textures.end())
        return;

    LayerTexture* texture = *it;
    if (texture->buffer()) {
        int delta = 0;
        delta += texture->width() * texture->height() * LayerTexture::bytesPerPixel();
        delta += texture->sizeInBytes();
        if (delta)
            decMemoryUsage(delta);
        m_garbage.append(ZombieTexture(texture));
    }

    texture->setBuffer(0);
}

void TextureCacheCompositingThread::textureAccessed(LayerTexture* texture)
{
    if (texture->isColor())
        return;

    TextureSet::iterator it = m_textures.find(texture);
    if (it == m_textures.end())
        return;

    m_textures.remove(it);
    m_textures.add(texture);
}

TextureCacheCompositingThread* textureCacheCompositingThread()
{
    static TextureCacheCompositingThread* staticCache = new TextureCacheCompositingThread;
    return staticCache;
}

void TextureCacheCompositingThread::prune(size_t limit)
{
    while (m_memoryUsage > limit) {
        bool found = false;
        for (TextureSet::iterator it = m_textures.begin(); it != m_textures.end(); ++it) {
            LayerTexture* texture = *it;
            if (texture->isProtected() || (texture->size().isEmpty() && !texture->sizeInBytes()))
                continue;
            evict(it);
            found = true;
            break;
        }
        if (!found)
            break;
    }
}

void TextureCacheCompositingThread::clear()
{
    m_colors.clear();
    collectGarbage();
}

void TextureCacheCompositingThread::setMemoryUsage(size_t memoryUsage)
{
    m_memoryUsage = memoryUsage;
#if DEBUG_TEXTURE_MEMORY_USAGE
    fprintf(stderr, "Texture memory usage %u kB\n", m_memoryUsage / 1024);
#endif
}

PassRefPtr<LayerTexture> TextureCacheCompositingThread::textureForContents(Buffer* contents)
{
    RefPtr<LayerTexture> texture = createTexture();

    // Protect newly created texture from being evicted.
    TextureProtector protector(texture.get());

    texture->updateContents(contents);

    return texture.release();
}

PassRefPtr<LayerTexture> TextureCacheCompositingThread::textureForColor(const Color& color)
{
    // Just to make sure we don't get fooled by some malicious web page
    // into caching millions of color textures.
    if (m_colors.size() > 100)
        m_colors.clear();

    ColorTextureMap::iterator it = m_colors.find(color);
    RefPtr<LayerTexture> texture;
    if (it == m_colors.end()) {
        texture = LayerTexture::create(true /* isColor */);
#if DEBUG_TEXTURE_MEMORY_USAGE
        fprintf(stderr, "Creating texture 0x%x for color 0x%x\n", texture.get(), color.rgb());
#endif
        m_colors.set(color, texture);
    } else
        texture = (*it).value;

    // Color textures can't be evicted, so no need for TextureProtector.

    if (texture->size() != IntSize(1, 1))
        texture->setContentsToColor(color);

    return texture.release();
}

PassRefPtr<LayerTexture> TextureCacheCompositingThread::updateContents(const RefPtr<LayerTexture>& textureIn, Buffer* contents)
{
    RefPtr<LayerTexture> texture(textureIn);

    // If the texture was 0, or needs to transition from a solid color texture to a contents texture,
    // create a new texture.
    if (!texture || texture->isColor())
        texture = createTexture();

    // Protect newly created texture from being evicted.
    TextureProtector protector(texture.get());

    texture->updateContents(contents);

    return texture.release();
}

} // namespace WebCore

#endif
