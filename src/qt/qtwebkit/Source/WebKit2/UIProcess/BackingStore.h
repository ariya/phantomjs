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

#ifndef BackingStore_h
#define BackingStore_h

#include <WebCore/IntRect.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>

#if PLATFORM(MAC)
#include <wtf/RetainPtr.h>
#endif

#if PLATFORM(QT)
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#endif

#if USE(CAIRO)
#include <RefPtrCairo.h>
#include <WebCore/WidgetBackingStore.h>
#endif

namespace WebKit {

class ShareableBitmap;
class UpdateInfo;
class WebPageProxy;

class BackingStore {
    WTF_MAKE_NONCOPYABLE(BackingStore);

public:
    static PassOwnPtr<BackingStore> create(const WebCore::IntSize&, float deviceScaleFactor, WebPageProxy*);
    ~BackingStore();

    const WebCore::IntSize& size() const { return m_size; }
    float deviceScaleFactor() const { return m_deviceScaleFactor; }

#if PLATFORM(MAC)
    typedef CGContextRef PlatformGraphicsContext;
#elif PLATFORM(QT)
    typedef QPainter* PlatformGraphicsContext;
#elif USE(CAIRO)
    typedef cairo_t* PlatformGraphicsContext;
#endif

    void paint(PlatformGraphicsContext, const WebCore::IntRect&);
    void incorporateUpdate(const UpdateInfo&);

private:
    BackingStore(const WebCore::IntSize&, float deviceScaleFactor, WebPageProxy*);

    void incorporateUpdate(ShareableBitmap*, const UpdateInfo&);
    void scroll(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollOffset);

    WebCore::IntSize m_size;
    float m_deviceScaleFactor;
    WebPageProxy* m_webPageProxy;

#if PLATFORM(MAC)
    CGContextRef backingStoreContext();

    void performWithScrolledRectTransform(const WebCore::IntRect&, void (^)(const WebCore::IntRect&, const WebCore::IntSize&));
    void resetScrolledRect();

    RetainPtr<CGLayerRef> m_cgLayer;
    RetainPtr<CGContextRef> m_bitmapContext;

    // The rectange that was scrolled most recently.
    WebCore::IntRect m_scrolledRect;

    // Contents of m_scrolledRect are offset by this amount (and wrapped around) with respect to
    // their original location.
    WebCore::IntSize m_scrolledRectOffset;
#elif PLATFORM(QT)
    QPixmap m_pixmap;
#elif USE(CAIRO)
    OwnPtr<WebCore::WidgetBackingStore> m_backingStore;
#endif
};

} // namespace WebKit

#endif // BackingStore_h
