/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#import "config.h"
#import "BackingStore.h"

#import "CGUtilities.h"
#import "ShareableBitmap.h"
#import "UpdateInfo.h"
#import "WebPageProxy.h"
#import <WebCore/GraphicsContext.h>
#import <WebCore/Region.h>

using namespace WebCore;

namespace WebKit {

void BackingStore::performWithScrolledRectTransform(const IntRect& rect, void (^block)(const IntRect&, const IntSize&))
{
    if (m_scrolledRect.isEmpty() || m_scrolledRectOffset.isZero() || !m_scrolledRect.intersects(rect)) {
        block(rect, IntSize());
        return;
    }

    // The part of rect that's outside the scrolled rect is not translated.
    Region untranslatedRegion = rect;
    untranslatedRegion.subtract(m_scrolledRect);
    Vector<IntRect> untranslatedRects = untranslatedRegion.rects();
    for (size_t i = 0; i < untranslatedRects.size(); ++i)
        block(untranslatedRects[i], IntSize());

    // The part of rect that intersects the scrolled rect comprises up to four parts, each subject
    // to a different translation (all translations are equivalent modulo the dimensions of the
    // scrolled rect to the scroll offset).
    IntRect intersection = rect;
    intersection.intersect(m_scrolledRect);

    IntRect scrolledRect = m_scrolledRect;
    IntSize offset = m_scrolledRectOffset;
    scrolledRect.move(-offset);

    IntRect part = intersection;
    part.intersect(scrolledRect);
    if (!part.isEmpty())
        block(part, offset);

    part = intersection;
    offset += IntSize(0, -m_scrolledRect.height());
    scrolledRect.move(IntSize(0, m_scrolledRect.height()));
    part.intersect(scrolledRect);
    if (!part.isEmpty())
        block(part, offset);

    part = intersection;
    offset += IntSize(-m_scrolledRect.width(), 0);
    scrolledRect.move(IntSize(m_scrolledRect.width(), 0));
    part.intersect(scrolledRect);
    if (!part.isEmpty())
        block(part, offset);

    part = intersection;
    offset += IntSize(0, m_scrolledRect.height());
    scrolledRect.move(IntSize(0, -m_scrolledRect.height()));
    part.intersect(scrolledRect);
    if (!part.isEmpty())
        block(part, offset);
}

void BackingStore::resetScrolledRect()
{
    ASSERT(!m_scrolledRect.isEmpty());

    if (m_scrolledRectOffset.isZero()) {
        m_scrolledRect = IntRect();
        return;
    }

    IntSize scaledSize = m_scrolledRect.size();
    scaledSize.scale(m_deviceScaleFactor);

    RetainPtr<CGColorSpaceRef> colorSpace = adoptCF(CGColorSpaceCreateDeviceRGB());
    RetainPtr<CGContextRef> context = adoptCF(CGBitmapContextCreate(0, scaledSize.width(), scaledSize.height(), 8, scaledSize.width() * 4, colorSpace.get(), kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host));

    CGContextScaleCTM(context.get(), m_deviceScaleFactor, m_deviceScaleFactor);

    CGContextScaleCTM(context.get(), 1, -1);
    CGContextTranslateCTM(context.get(), -m_scrolledRect.x(), -m_scrolledRect.maxY());
    paint(context.get(), m_scrolledRect);

    IntRect sourceRect(IntPoint(), m_scrolledRect.size());
    paintBitmapContext(backingStoreContext(), context.get(), m_deviceScaleFactor, m_scrolledRect.location(), sourceRect);

    m_scrolledRect = IntRect();
    m_scrolledRectOffset = IntSize();
}

void BackingStore::paint(PlatformGraphicsContext context, const IntRect& rect)
{
    // FIXME: This is defined outside the block to work around bugs in llvm-gcc 4.2.
    __block CGRect source;
    performWithScrolledRectTransform(rect, ^(const IntRect& part, const IntSize& offset) {
        if (m_cgLayer) {
            CGContextSaveGState(context);
            CGContextClipToRect(context, part);

            CGContextScaleCTM(context, 1, -1);
            CGContextDrawLayerAtPoint(context, CGPointMake(-offset.width(), offset.height() - m_size.height()), m_cgLayer.get());

            CGContextRestoreGState(context);
            return;
        }

        ASSERT(m_bitmapContext);
        source = part;
        source.origin.x += offset.width();
        source.origin.y += offset.height();
        paintBitmapContext(context, m_bitmapContext.get(), m_deviceScaleFactor, part.location(), source);
    });
}

CGContextRef BackingStore::backingStoreContext()
{
    if (m_cgLayer)
        return CGLayerGetContext(m_cgLayer.get());

    // Try to create a layer.
    if (CGContextRef containingWindowContext = m_webPageProxy->containingWindowGraphicsContext()) {
        m_cgLayer = adoptCF(CGLayerCreateWithContext(containingWindowContext, m_size, 0));
        CGContextRef layerContext = CGLayerGetContext(m_cgLayer.get());
        
        CGContextSetBlendMode(layerContext, kCGBlendModeCopy);

        // We want the origin to be in the top left corner so flip the backing store context.
        CGContextTranslateCTM(layerContext, 0, m_size.height());
        CGContextScaleCTM(layerContext, 1, -1);

        if (m_bitmapContext) {
            // Paint the contents of the bitmap into the layer context.
            paintBitmapContext(layerContext, m_bitmapContext.get(), m_deviceScaleFactor, CGPointZero, CGRectMake(0, 0, m_size.width(), m_size.height()));
            m_bitmapContext = nullptr;
        }

        return layerContext;
    }

    if (!m_bitmapContext) {
        RetainPtr<CGColorSpaceRef> colorSpace = adoptCF(CGColorSpaceCreateDeviceRGB());

        IntSize scaledSize(m_size);
        scaledSize.scale(m_deviceScaleFactor);
        m_bitmapContext = adoptCF(CGBitmapContextCreate(0, scaledSize.width(), scaledSize.height(), 8, scaledSize.width() * 4, colorSpace.get(), kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host));

        CGContextSetBlendMode(m_bitmapContext.get(), kCGBlendModeCopy);

        CGContextScaleCTM(m_bitmapContext.get(), m_deviceScaleFactor, m_deviceScaleFactor);

        // We want the origin to be in the top left corner so flip the backing store context.
        CGContextTranslateCTM(m_bitmapContext.get(), 0, m_size.height());
        CGContextScaleCTM(m_bitmapContext.get(), 1, -1);
    }

    return m_bitmapContext.get();
}

void BackingStore::incorporateUpdate(ShareableBitmap* bitmap, const UpdateInfo& updateInfo)
{
    CGContextRef context = backingStoreContext();

    scroll(updateInfo.scrollRect, updateInfo.scrollOffset);

    IntPoint updateRectLocation = updateInfo.updateRectBounds.location();

    GraphicsContext ctx(context);
    __block GraphicsContext* graphicsContext = &ctx;

    // Paint all update rects.
    for (size_t i = 0; i < updateInfo.updateRects.size(); ++i) {
        IntRect updateRect = updateInfo.updateRects[i];
        IntRect srcRect = updateRect;
        // FIXME: This is defined outside the block to work around bugs in llvm-gcc 4.2.
        __block IntRect srcPart;
        performWithScrolledRectTransform(srcRect, ^(const IntRect& part, const IntSize& offset) {
            srcPart = part;
            srcPart.move(-updateRectLocation.x(), -updateRectLocation.y());
            bitmap->paint(*graphicsContext, updateInfo.deviceScaleFactor, part.location() + offset, srcPart);
        });
    }
}

void BackingStore::scroll(const IntRect& scrollRect, const IntSize& scrollOffset)
{
    if (scrollOffset.isZero())
        return;

    ASSERT(!scrollRect.isEmpty());

    if (!m_scrolledRect.isEmpty() && m_scrolledRect != scrollRect)
        resetScrolledRect();

    m_scrolledRect = scrollRect;

    int width = (m_scrolledRectOffset.width() - scrollOffset.width()) % m_scrolledRect.width();
    if (width < 0)
        width += m_scrolledRect.width();
    m_scrolledRectOffset.setWidth(width);

    int height = (m_scrolledRectOffset.height() - scrollOffset.height()) % m_scrolledRect.height();
    if (height < 0)
        height += m_scrolledRect.height();
    m_scrolledRectOffset.setHeight(height);
}

} // namespace WebKit
