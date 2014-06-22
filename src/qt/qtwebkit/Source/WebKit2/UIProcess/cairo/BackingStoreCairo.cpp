/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Igalia S.L.
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
#include "BackingStore.h"

#include "ShareableBitmap.h"
#include "UpdateInfo.h"
#include "WebPageProxy.h"
#include <WebCore/GraphicsContext.h>
#include <WebCore/WidgetBackingStoreCairo.h>
#include <cairo.h>

#if PLATFORM(GTK) && defined(GDK_WINDOWING_X11)
#include <WebCore/WidgetBackingStoreGtkX11.h>
#include <gdk/gdkx.h>
#endif

#if PLATFORM(EFL)
#include "EwkView.h"
#endif

using namespace WebCore;

namespace WebKit {

#if PLATFORM(GTK)
static OwnPtr<WidgetBackingStore> createBackingStoreForGTK(GtkWidget* widget, const IntSize& size)
{
#ifdef GDK_WINDOWING_X11
    GdkDisplay* display = gdk_display_manager_get_default_display(gdk_display_manager_get());
    if (GDK_IS_X11_DISPLAY(display))
        return WebCore::WidgetBackingStoreGtkX11::create(widget, size);
#endif
    return WebCore::WidgetBackingStoreCairo::create(widget, size);
}
#endif

void BackingStore::paint(cairo_t* context, const IntRect& rect)
{
    ASSERT(m_backingStore);

    cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(context, m_backingStore->cairoSurface(), 0, 0);
    cairo_rectangle(context, rect.x(), rect.y(), rect.width(), rect.height());
    cairo_fill(context);
}

void BackingStore::incorporateUpdate(ShareableBitmap* bitmap, const UpdateInfo& updateInfo)
{
    if (!m_backingStore)
#if PLATFORM(EFL)
        m_backingStore = WidgetBackingStoreCairo::create(EwkView::toEvasObject(toAPI(m_webPageProxy)), size());
#else
        m_backingStore = createBackingStoreForGTK(m_webPageProxy->viewWidget(), size());
#endif

    scroll(updateInfo.scrollRect, updateInfo.scrollOffset);

    // Paint all update rects.
    IntPoint updateRectLocation = updateInfo.updateRectBounds.location();
    RefPtr<cairo_t> context(adoptRef(cairo_create(m_backingStore->cairoSurface())));
    GraphicsContext graphicsContext(context.get());
    for (size_t i = 0; i < updateInfo.updateRects.size(); ++i) {
        IntRect updateRect = updateInfo.updateRects[i];
        IntRect srcRect = updateRect;
        srcRect.move(-updateRectLocation.x(), -updateRectLocation.y());
        bitmap->paint(graphicsContext, updateRect.location(), srcRect);
    }
}

void BackingStore::scroll(const IntRect& scrollRect, const IntSize& scrollOffset)
{
    if (scrollOffset.isZero())
        return;

    ASSERT(m_backingStore);
    m_backingStore->scroll(scrollRect, scrollOffset);
}

} // namespace WebKit
