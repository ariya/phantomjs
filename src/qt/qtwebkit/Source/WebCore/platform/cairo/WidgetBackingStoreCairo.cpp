/*
 * Copyright (C) 2011, Igalia S.L.
 * Copyright (C) 2011 Samsung Electronics
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "WidgetBackingStoreCairo.h"

#include "CairoUtilities.h"
#include "RefPtrCairo.h"
#include <cairo.h>

namespace WebCore {

static PassRefPtr<cairo_surface_t> createSurfaceForBackingStore(PlatformWidget widget, const IntSize& size)
{
#if PLATFORM(GTK)
    return adoptRef(gdk_window_create_similar_surface(gtk_widget_get_window(widget), CAIRO_CONTENT_COLOR_ALPHA, size.width(), size.height()));
#else
    UNUSED_PARAM(widget);
    return adoptRef(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.width(), size.height()));
#endif
}

PassOwnPtr<WidgetBackingStore> WidgetBackingStoreCairo::create(PlatformWidget widget, const IntSize& size)
{
    return adoptPtr(new WidgetBackingStoreCairo(widget, size));
}

// We keep two copies of the surface here, which will double the memory usage, but increase
// scrolling performance since we do not have to keep reallocating a memory region during
// quick scrolling requests.
WidgetBackingStoreCairo::WidgetBackingStoreCairo(PlatformWidget widget, const IntSize& size)
    : WidgetBackingStore(size)
    , m_surface(createSurfaceForBackingStore(widget, size))
    , m_scrollSurface(createSurfaceForBackingStore(widget, size))

{
}

WidgetBackingStoreCairo::~WidgetBackingStoreCairo()
{
}

cairo_surface_t* WidgetBackingStoreCairo::cairoSurface()
{
    return m_surface.get();
}

void WidgetBackingStoreCairo::scroll(const IntRect& scrollRect, const IntSize& scrollOffset)
{
    IntRect targetRect(scrollRect);
    targetRect.move(scrollOffset);
    targetRect.shiftMaxXEdgeTo(targetRect.maxX() - scrollOffset.width());
    targetRect.shiftMaxYEdgeTo(targetRect.maxY() - scrollOffset.height());
    if (targetRect.isEmpty())
        return;

    copyRectFromOneSurfaceToAnother(m_surface.get(), m_scrollSurface.get(),
                                    scrollOffset, targetRect);
    copyRectFromOneSurfaceToAnother(m_scrollSurface.get(), m_surface.get(),
                                    IntSize(), targetRect);
}

} // namespace WebCore
