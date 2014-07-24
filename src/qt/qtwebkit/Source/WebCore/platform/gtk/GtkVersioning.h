/*
 * Copyright (C) 2010 Collabora Ltd.
 * Copyright (C) 2010 Igalia, S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef GtkVersioning_h
#define GtkVersioning_h

#include <gtk/gtk.h>

#ifndef GTK_API_VERSION_2
#include <gdk/gdkkeysyms-compat.h>
#endif

G_BEGIN_DECLS

// Macros to avoid deprecation checking churn
#ifndef GTK_API_VERSION_2
#define GDK_WINDOW_XWINDOW(window) (gdk_x11_window_get_xid(window))
#else
GdkPixbuf* gdk_pixbuf_get_from_surface(cairo_surface_t* surface, int srcX, int srcY,
                                       int width, int height);
void gdk_screen_get_monitor_workarea(GdkScreen *, int monitor, GdkRectangle *area);
// Define GDK_IS_X11_DISPLAY dummy for GTK+ 2.0 compatibility.
#ifdef GDK_WINDOWING_X11
#define GDK_IS_X11_DISPLAY(display) 1
#else
#define GDK_IS_X11_DISPLAY(display) 0
#endif
#endif

GdkDevice* getDefaultGDKPointerDevice(GdkWindow* window);

// gtk_widget_get_preferred_size() appeared only in GTK 3.0.
#if !GTK_CHECK_VERSION (2, 91, 0)  // gtk_widget_get_preferred_size appeared about then.
#define gtk_widget_get_preferred_size(widget, minimumSize, naturalSize) \
        (gtk_widget_size_request((widget), ((minimumSize))))
#endif

G_END_DECLS

#endif // GtkVersioning_h
