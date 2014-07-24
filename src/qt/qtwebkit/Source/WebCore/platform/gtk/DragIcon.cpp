/*
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
#include "DragIcon.h"
#include "GdkCairoUtilities.h"

#include <gtk/gtk.h>

namespace WebCore {

#ifdef GTK_API_VERSION_2
static gboolean dragIconWindowDrawEventCallback(GtkWidget*, GdkEventExpose* event, DragIcon* icon)
{
    RefPtr<cairo_t> context = adoptRef(gdk_cairo_create(event->window));
    icon->draw(context.get());
    return TRUE;
}
#else
static gboolean dragIconWindowDrawEventCallback(GtkWidget*, cairo_t* context, DragIcon* icon)
{
    if (!gdk_cairo_get_clip_rectangle(context, 0))
        return FALSE;
    icon->draw(context);
    return TRUE;
}
#endif // GTK_API_VERSION_2

DragIcon::DragIcon()
    : isComposited(gdk_screen_is_composited(gdk_screen_get_default()))
    , m_window(0)
{
    if (!isComposited)
        return;

    m_window = gtk_window_new(GTK_WINDOW_POPUP);
#ifdef GTK_API_VERSION_2
    g_signal_connect(m_window, "expose-event", G_CALLBACK(dragIconWindowDrawEventCallback), this);
#else
    g_signal_connect(m_window, "draw", G_CALLBACK(dragIconWindowDrawEventCallback), this);
#endif

    // This strategy originally comes from Chromium: src/chrome/browser/gtk/tab_contents_drag_source.cc
    GdkScreen* screen = gtk_widget_get_screen(m_window);
#ifdef GTK_API_VERSION_2
    GdkColormap* rgba = gdk_screen_get_rgba_colormap(screen);
    if (rgba)
        gtk_widget_set_colormap(m_window, rgba);
#else
    GdkVisual* visual = gdk_screen_get_rgba_visual(screen);
    if (!visual)
        visual = gdk_screen_get_system_visual(screen);
    gtk_widget_set_visual(m_window, visual);
#endif // GTK_API_VERSION_2
}

DragIcon::~DragIcon()
{
    if (m_window)
        gtk_widget_destroy(m_window);
}

void DragIcon::draw(cairo_t* context)
{
    cairo_rectangle(context, 0, 0,
                    cairo_image_surface_get_width(m_image.get()),
                    cairo_image_surface_get_height(m_image.get()));
    cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(context, m_image.get(), 0, 0);
    cairo_fill(context);
}

void DragIcon::setImage(cairo_surface_t* image)
{
    ASSERT(image);
    m_image = image;
    m_imageSize = IntSize(cairo_image_surface_get_width(image), cairo_image_surface_get_height(image));
    if (isComposited) {
        gtk_window_resize(GTK_WINDOW(m_window), m_imageSize.width(), m_imageSize.height());
        return;
    }

#ifdef GTK_API_VERSION_2
    m_pixbuf = adoptGRef(cairoImageSurfaceToGdkPixbuf(image));
#endif
}

void DragIcon::useForDrag(GdkDragContext* context)
{
    IntPoint hotspot(m_imageSize);
    hotspot.scale(0.5, 0.5);
    useForDrag(context, hotspot);
}

void DragIcon::useForDrag(GdkDragContext* context, const IntPoint& hotspot)
{
    if (isComposited) {
        gtk_drag_set_icon_widget(context, m_window, hotspot.x(), hotspot.y());
        return;
    }
#ifdef GTK_API_VERSION_2
    gtk_drag_set_icon_pixbuf(context, m_pixbuf.get(), hotspot.x(), hotspot.y());
#else
    gtk_drag_set_icon_surface(context, m_image.get());
#endif
}

} // namespace WebCore
