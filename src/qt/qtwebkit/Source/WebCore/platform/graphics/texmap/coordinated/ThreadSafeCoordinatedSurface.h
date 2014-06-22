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

#ifndef ThreadSafeCoordinatedSurface_h
#define ThreadSafeCoordinatedSurface_h

#if USE(COORDINATED_GRAPHICS)
#include "CoordinatedSurface.h"
#include "ImageBuffer.h"

namespace WebCore {

class ThreadSafeCoordinatedSurface : public CoordinatedSurface {
public:
    virtual ~ThreadSafeCoordinatedSurface();

    static PassRefPtr<ThreadSafeCoordinatedSurface> create(const IntSize&, Flags);

    virtual void paintToSurface(const IntRect&, CoordinatedSurface::Client*) OVERRIDE;
    virtual void copyToTexture(PassRefPtr<BitmapTexture>, const IntRect& target, const IntPoint& sourceOffset) OVERRIDE;

private:
    ThreadSafeCoordinatedSurface(const IntSize&, Flags, PassOwnPtr<ImageBuffer>);

    OwnPtr<ImageBuffer> m_imageBuffer;
};

} // namespace WebCore

#endif // USE(COORDINATED_GRAPHICS)

#endif // ThreadSafeCoordinatedSurface_h
