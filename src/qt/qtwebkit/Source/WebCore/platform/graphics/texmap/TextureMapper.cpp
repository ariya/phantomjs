/*
 Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

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
#include "TextureMapper.h"

#include "FilterOperations.h"
#include "GraphicsLayer.h"
#include "TextureMapperImageBuffer.h"
#include "Timer.h"
#include <wtf/CurrentTime.h>
#include <wtf/NonCopyingSort.h>

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)

namespace WebCore {

struct BitmapTexturePoolEntry {
    explicit BitmapTexturePoolEntry(PassRefPtr<BitmapTexture> texture)
        : m_texture(texture)
    { }
    inline void markUsed() { m_timeLastUsed = monotonicallyIncreasingTime(); }
    static bool compareTimeLastUsed(const BitmapTexturePoolEntry& a, const BitmapTexturePoolEntry& b)
    {
        return a.m_timeLastUsed - b.m_timeLastUsed > 0;
    }

    RefPtr<BitmapTexture> m_texture;
    double m_timeLastUsed;
};

class BitmapTexturePool {
    WTF_MAKE_NONCOPYABLE(BitmapTexturePool);
    WTF_MAKE_FAST_ALLOCATED;
public:
    BitmapTexturePool();

    PassRefPtr<BitmapTexture> acquireTexture(const IntSize&, TextureMapper*);

private:
    void scheduleReleaseUnusedTextures();
    void releaseUnusedTexturesTimerFired(Timer<BitmapTexturePool>*);

    Vector<BitmapTexturePoolEntry> m_textures;
    Timer<BitmapTexturePool> m_releaseUnusedTexturesTimer;

    static const double s_releaseUnusedSecondsTolerance;
    static const double s_releaseUnusedTexturesTimerInterval;
};

const double BitmapTexturePool::s_releaseUnusedSecondsTolerance = 3;
const double BitmapTexturePool::s_releaseUnusedTexturesTimerInterval = 0.5;

BitmapTexturePool::BitmapTexturePool()
    : m_releaseUnusedTexturesTimer(this, &BitmapTexturePool::releaseUnusedTexturesTimerFired)
{ }

void BitmapTexturePool::scheduleReleaseUnusedTextures()
{
    if (m_releaseUnusedTexturesTimer.isActive())
        m_releaseUnusedTexturesTimer.stop();

    m_releaseUnusedTexturesTimer.startOneShot(s_releaseUnusedTexturesTimerInterval);
}

void BitmapTexturePool::releaseUnusedTexturesTimerFired(Timer<BitmapTexturePool>*)
{
    if (m_textures.isEmpty())
        return;

    // Delete entries, which have been unused in s_releaseUnusedSecondsTolerance.
    nonCopyingSort(m_textures.begin(), m_textures.end(), BitmapTexturePoolEntry::compareTimeLastUsed);

    double minUsedTime = monotonicallyIncreasingTime() - s_releaseUnusedSecondsTolerance;
    for (size_t i = 0; i < m_textures.size(); ++i) {
        if (m_textures[i].m_timeLastUsed < minUsedTime) {
            m_textures.remove(i, m_textures.size() - i);
            break;
        }
    }
}

PassRefPtr<BitmapTexture> BitmapTexturePool::acquireTexture(const IntSize& size, TextureMapper* textureMapper)
{
    BitmapTexturePoolEntry* selectedEntry = 0;
    for (size_t i = 0; i < m_textures.size(); ++i) {
        BitmapTexturePoolEntry* entry = &m_textures[i];

        // If the surface has only one reference (the one in m_textures), we can safely reuse it.
        if (entry->m_texture->refCount() > 1)
            continue;

        if (entry->m_texture->canReuseWith(size)) {
            selectedEntry = entry;
            break;
        }
    }

    if (!selectedEntry) {
        m_textures.append(BitmapTexturePoolEntry(textureMapper->createTexture()));
        selectedEntry = &m_textures.last();
    }

    scheduleReleaseUnusedTextures();
    selectedEntry->markUsed();
    return selectedEntry->m_texture;
}

PassRefPtr<BitmapTexture> TextureMapper::acquireTextureFromPool(const IntSize& size)
{
    RefPtr<BitmapTexture> selectedTexture = m_texturePool->acquireTexture(size, this);
    selectedTexture->reset(size, BitmapTexture::SupportsAlpha);
    return selectedTexture;
}

PassOwnPtr<TextureMapper> TextureMapper::create(AccelerationMode mode)
{
    if (mode == SoftwareMode)
        return TextureMapperImageBuffer::create();
    return platformCreateAccelerated();
}

TextureMapper::TextureMapper(AccelerationMode accelerationMode)
    : m_context(0)
    , m_interpolationQuality(InterpolationDefault)
    , m_textDrawingMode(TextModeFill)
    , m_texturePool(adoptPtr(new BitmapTexturePool()))
    , m_accelerationMode(accelerationMode)
    , m_isMaskMode(false)
    , m_wrapMode(StretchWrap)
{ }

TextureMapper::~TextureMapper()
{ }

void BitmapTexture::updateContents(TextureMapper* textureMapper, GraphicsLayer* sourceLayer, const IntRect& targetRect, const IntPoint& offset, UpdateContentsFlag updateContentsFlag)
{
    OwnPtr<ImageBuffer> imageBuffer = ImageBuffer::create(targetRect.size());
    GraphicsContext* context = imageBuffer->context();
    context->setImageInterpolationQuality(textureMapper->imageInterpolationQuality());
    context->setTextDrawingMode(textureMapper->textDrawingMode());

    IntRect sourceRect(targetRect);
    sourceRect.setLocation(offset);
    context->translate(-offset.x(), -offset.y());
    sourceLayer->paintGraphicsLayerContents(*context, sourceRect);

    RefPtr<Image> image = imageBuffer->copyImage(DontCopyBackingStore);

    updateContents(image.get(), targetRect, IntPoint(), updateContentsFlag);
}

} // namespace

#endif
