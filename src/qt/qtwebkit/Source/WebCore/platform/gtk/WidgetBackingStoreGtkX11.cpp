/*
 * Copyright (C) 2011, Igalia S.L.
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
#include "WidgetBackingStoreGtkX11.h"

#include "GtkVersioning.h"
#include "RefPtrCairo.h"
#include <cairo-xlib.h>
#include <cairo.h>
#include <gdk/gdkx.h>

namespace WebCore {

PassOwnPtr<WidgetBackingStore> WidgetBackingStoreGtkX11::create(GtkWidget* widget, const IntSize& size)
{
    return adoptPtr(new WidgetBackingStoreGtkX11(widget, size));
}

// We keep two copies of the surface here, which will double the memory usage, but increase
// scrolling performance since we do not have to keep reallocating a memory region during
// quick scrolling requests.
WidgetBackingStoreGtkX11::WidgetBackingStoreGtkX11(GtkWidget* widget, const IntSize& size)
    : WidgetBackingStore(size)
{
    GdkVisual* visual = gtk_widget_get_visual(widget);
    GdkScreen* screen = gdk_visual_get_screen(visual);
    m_display = GDK_SCREEN_XDISPLAY(screen);
    m_pixmap = XCreatePixmap(m_display, GDK_WINDOW_XID(gdk_screen_get_root_window(screen)),
        size.width(), size.height(), gdk_visual_get_depth(visual));
    m_gc = XCreateGC(m_display, m_pixmap, 0, 0);

    m_surface = adoptRef(cairo_xlib_surface_create(m_display, m_pixmap,
        GDK_VISUAL_XVISUAL(visual), size.width(), size.height()));
}

WidgetBackingStoreGtkX11::~WidgetBackingStoreGtkX11()
{
    XFreePixmap(m_display, m_pixmap);
    XFreeGC(m_display, m_gc);
}

cairo_surface_t* WidgetBackingStoreGtkX11::cairoSurface()
{
    return m_surface.get();
}

void WidgetBackingStoreGtkX11::scroll(const IntRect& scrollRect, const IntSize& scrollOffset)
{
    IntRect targetRect(scrollRect);
    targetRect.move(scrollOffset);
    targetRect.intersect(scrollRect);
    if (targetRect.isEmpty())
        return;

    cairo_surface_flush(m_surface.get());
    XCopyArea(m_display, m_pixmap, m_pixmap, m_gc, 
        targetRect.x() - scrollOffset.width(), targetRect.y() - scrollOffset.height(),
        targetRect.width(), targetRect.height(),
        targetRect.x(), targetRect.y());
    cairo_surface_mark_dirty_rectangle(m_surface.get(),
        targetRect.x(), targetRect.y(),
        targetRect.width(), targetRect.height());
}

} // namespace WebCore
