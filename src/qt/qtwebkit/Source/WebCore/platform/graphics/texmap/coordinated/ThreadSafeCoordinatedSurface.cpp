/*
 * Copyright (C) 2013 Company 100, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ThreadSafeCoordinatedSurface.h"

#if USE(COORDINATED_GRAPHICS)

#include "TextureMapper.h"

namespace WebCore {

PassRefPtr<ThreadSafeCoordinatedSurface> ThreadSafeCoordinatedSurface::create(const IntSize& size, CoordinatedSurface::Flags flags)
{
    return adoptRef(new ThreadSafeCoordinatedSurface(size, flags, ImageBuffer::create(size)));
}

ThreadSafeCoordinatedSurface::ThreadSafeCoordinatedSurface(const IntSize& size, CoordinatedSurface::Flags flags, PassOwnPtr<ImageBuffer> buffer)
    : CoordinatedSurface(size, flags)
    , m_imageBuffer(buffer)
{
}

ThreadSafeCoordinatedSurface::~ThreadSafeCoordinatedSurface()
{
}

void ThreadSafeCoordinatedSurface::paintToSurface(const IntRect& rect, CoordinatedSurface::Client* client)
{
    ASSERT(client);
    ASSERT(m_imageBuffer);

    GraphicsContext* context = m_imageBuffer->context();
    context->save();
    context->clip(rect);
    context->translate(rect.x(), rect.y());

    client->paintToSurfaceContext(context);

    context->restore();
}

void ThreadSafeCoordinatedSurface::copyToTexture(PassRefPtr<BitmapTexture> passTexture, const IntRect& target, const IntPoint& sourceOffset)
{
    RefPtr<BitmapTexture> texture(passTexture);

    ASSERT(m_imageBuffer);
    RefPtr<Image> image = m_imageBuffer->copyImage(DontCopyBackingStore);
    texture->updateContents(image.get(), target, sourceOffset, BitmapTexture::UpdateCanModifyOriginalImageData);
}

} // namespace WebCore

#endif // USE(COORDINATED_GRAPHICS)
