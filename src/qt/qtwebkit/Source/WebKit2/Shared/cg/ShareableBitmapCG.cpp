/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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
#include "ShareableBitmap.h"

#include <WebCore/BitmapImage.h>
#include <WebCore/GraphicsContext.h>
#include <wtf/RetainPtr.h>
#include "CGUtilities.h"

using namespace WebCore;

namespace WebKit {

static CGBitmapInfo bitmapInfo(ShareableBitmap::Flags flags)
{
    CGBitmapInfo info = kCGBitmapByteOrder32Host;
    if (flags & ShareableBitmap::SupportsAlpha)
        info |= kCGImageAlphaPremultipliedFirst;
    else
        info |= kCGImageAlphaNoneSkipFirst;

    return info;
}

PassOwnPtr<GraphicsContext> ShareableBitmap::createGraphicsContext()
{
    RetainPtr<CGColorSpaceRef> colorSpace = adoptCF(CGColorSpaceCreateDeviceRGB());

    ref(); // Balanced by deref in releaseBitmapContextData.
    RetainPtr<CGContextRef> bitmapContext = adoptCF(CGBitmapContextCreateWithData(data(),
        m_size.width(), m_size.height(), 8, m_size.width() * 4, colorSpace.get(),
        bitmapInfo(m_flags), releaseBitmapContextData, this));

    // We want the origin to be in the top left corner so we flip the backing store context.
    CGContextTranslateCTM(bitmapContext.get(), 0, m_size.height());
    CGContextScaleCTM(bitmapContext.get(), 1, -1);

    return adoptPtr(new GraphicsContext(bitmapContext.get()));
}

void ShareableBitmap::paint(WebCore::GraphicsContext& context, const IntPoint& destination, const IntRect& source)
{
    paintImage(context.platformContext(), makeCGImageCopy().get(), 1, destination, source);
}

void ShareableBitmap::paint(WebCore::GraphicsContext& context, float scaleFactor, const IntPoint& destination, const IntRect& source)
{
    paintImage(context.platformContext(), makeCGImageCopy().get(), scaleFactor, destination, source);
}

RetainPtr<CGImageRef> ShareableBitmap::makeCGImageCopy()
{
    OwnPtr<GraphicsContext> graphicsContext = createGraphicsContext();
    RetainPtr<CGImageRef> image = adoptCF(CGBitmapContextCreateImage(graphicsContext->platformContext()));
    return image;
}

RetainPtr<CGImageRef> ShareableBitmap::makeCGImage()
{
    ref(); // Balanced by deref in releaseDataProviderData.
    RetainPtr<CGDataProvider> dataProvider = adoptCF(CGDataProviderCreateWithData(this, data(), sizeInBytes(), releaseDataProviderData));
    return createCGImage(dataProvider.get());
}

RetainPtr<CGImageRef> ShareableBitmap::createCGImage(CGDataProviderRef dataProvider) const
{
    ASSERT_ARG(dataProvider, dataProvider);

    RetainPtr<CGColorSpaceRef> colorSpace = adoptCF(CGColorSpaceCreateDeviceRGB());
    RetainPtr<CGImageRef> image = adoptCF(CGImageCreate(m_size.width(), m_size.height(), 8, 32, m_size.width() * 4, colorSpace.get(), bitmapInfo(m_flags), dataProvider, 0, false, kCGRenderingIntentDefault));
    return image;
}

void ShareableBitmap::releaseBitmapContextData(void* typelessBitmap, void* typelessData)
{
    ShareableBitmap* bitmap = static_cast<ShareableBitmap*>(typelessBitmap);
    ASSERT_UNUSED(typelessData, bitmap->data() == typelessData);
    bitmap->deref(); // Balanced by ref in createGraphicsContext.
}

void ShareableBitmap::releaseDataProviderData(void* typelessBitmap, const void* typelessData, size_t)
{
    ShareableBitmap* bitmap = static_cast<ShareableBitmap*>(typelessBitmap);
    ASSERT_UNUSED(typelessData, bitmap->data() == typelessData);
    bitmap->deref(); // Balanced by ref in createCGImage.
}

PassRefPtr<Image> ShareableBitmap::createImage()
{
    RetainPtr<CGImageRef> platformImage = makeCGImage();
    if (!platformImage)
        return 0;

    // BitmapImage::create adopts the CGImageRef that's passed in, which is why we need to leakRef here.
    return BitmapImage::create(platformImage.leakRef());
}

} // namespace WebKit
