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

#include "config.h"
#include "GtkVersioning.h"

#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_X11
#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#endif

GdkDevice *getDefaultGDKPointerDevice(GdkWindow* window)
{
#ifndef GTK_API_VERSION_2
    GdkDeviceManager *manager =  gdk_display_get_device_manager(gdk_window_get_display(window));
    return gdk_device_manager_get_client_pointer(manager);
#else
    return gdk_device_get_core_pointer();
#endif // GTK_API_VERSION_2
}

#ifdef GTK_API_VERSION_2
static cairo_format_t
gdk_cairo_format_for_content(cairo_content_t content)
{
    switch (content) {
    case CAIRO_CONTENT_COLOR:
        return CAIRO_FORMAT_RGB24;
    case CAIRO_CONTENT_ALPHA:
        return CAIRO_FORMAT_A8;
    case CAIRO_CONTENT_COLOR_ALPHA:
    default:
        return CAIRO_FORMAT_ARGB32;
    }
}

static cairo_surface_t*
gdk_cairo_surface_coerce_to_image(cairo_surface_t* surface,
                                  cairo_content_t content,
                                  int width,
                                  int height)
{
    cairo_surface_t * copy;
    cairo_t * cr;

    if (cairo_surface_get_type(surface) == CAIRO_SURFACE_TYPE_IMAGE
        && cairo_surface_get_content(surface) == content
        && cairo_image_surface_get_width(surface) >= width
        && cairo_image_surface_get_height(surface) >= height)
        return cairo_surface_reference(surface);

    copy = cairo_image_surface_create(gdk_cairo_format_for_content(content),
                                      width,
                                      height);

    cr = cairo_create(copy);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);

    return copy;
}

static void
convert_alpha(guchar * destData, int destStride,
              guchar * srcData, int srcStride,
              int srcX, int srcY, int width, int height)
{
    int x, y;

    srcData += srcStride * srcY + srcY * 4;

    for (y = 0; y < height; y++) {
        guint32 * src = (guint32 *) srcData;

        for (x = 0; x < width; x++) {
            guint alpha = src[x] >> 24;

            if (!alpha) {
                destData[x * 4 + 0] = 0;
                destData[x * 4 + 1] = 0;
                destData[x * 4 + 2] = 0;
            } else {
                destData[x * 4 + 0] = (((src[x] & 0xff0000) >> 16) * 255 + alpha / 2) / alpha;
                destData[x * 4 + 1] = (((src[x] & 0x00ff00) >>  8) * 255 + alpha / 2) / alpha;
                destData[x * 4 + 2] = (((src[x] & 0x0000ff) >>  0) * 255 + alpha / 2) / alpha;
            }
            destData[x * 4 + 3] = alpha;
        }

        srcData += srcStride;
        destData += destStride;
    }
}

static void
convert_no_alpha(guchar * destData, int destStride, guchar * srcData,
                 int srcStride, int srcX, int srcY,
                 int width, int height)
{
    int x, y;

    srcData += srcStride * srcY + srcX * 4;

    for (y = 0; y < height; y++) {
        guint32 * src = (guint32 *) srcData;

        for (x = 0; x < width; x++) {
            destData[x * 3 + 0] = src[x] >> 16;
            destData[x * 3 + 1] = src[x] >>  8;
            destData[x * 3 + 2] = src[x];
        }

        srcData += srcStride;
        destData += destStride;
    }
}

/**
 * gdk_pixbuf_get_from_surface:
 * @surface: surface to copy from
 * @src_x: Source X coordinate within @surface
 * @src_y: Source Y coordinate within @surface
 * @width: Width in pixels of region to get
 * @height: Height in pixels of region to get
 *
 * Transfers image data from a #cairo_surface_t and converts it to an RGB(A)
 * representation inside a #GdkPixbuf. This allows you to efficiently read
 * individual pixels from cairo surfaces. For #GdkWindows, use
 * gdk_pixbuf_get_from_window() instead.
 *
 * This function will create an RGB pixbuf with 8 bits per channel. The pixbuf
 * will contain an alpha channel if the @surface contains one.
 *
 * Return value: (transfer full): A newly-created pixbuf with a reference count
 * of 1, or %NULL on error
 **/
GdkPixbuf*
gdk_pixbuf_get_from_surface(cairo_surface_t * surface,
                            int srcX, int srcY,
                            int width, int height)
{
    cairo_content_t content;
    GdkPixbuf * dest;

    /* General sanity checks */
    g_return_val_if_fail(surface, NULL);
    g_return_val_if_fail(srcX >= 0 && srcY >= 0, NULL);
    g_return_val_if_fail(width > 0 && height > 0, NULL);

    content = cairo_surface_get_content(surface) | CAIRO_CONTENT_COLOR;
    dest = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
                          !!(content & CAIRO_CONTENT_ALPHA),
                          8,
                          width, height);

    surface = gdk_cairo_surface_coerce_to_image(surface, content, srcX + width, srcY + height);
    cairo_surface_flush(surface);
    if (cairo_surface_status(surface) || !dest) {
        cairo_surface_destroy(surface);
        return NULL;
    }

    if (gdk_pixbuf_get_has_alpha(dest))
        convert_alpha(gdk_pixbuf_get_pixels(dest),
                       gdk_pixbuf_get_rowstride(dest),
                       cairo_image_surface_get_data(surface),
                       cairo_image_surface_get_stride(surface),
                       srcX, srcY,
                       width, height);
    else
        convert_no_alpha(gdk_pixbuf_get_pixels(dest),
                          gdk_pixbuf_get_rowstride(dest),
                          cairo_image_surface_get_data(surface),
                          cairo_image_surface_get_stride(surface),
                          srcX, srcY,
                          width, height);

    cairo_surface_destroy(surface);
    return dest;
}

#ifdef GDK_WINDOWING_X11
static int getScreenCurrentDesktop(GdkScreen *screen)
{
    Display *display = GDK_DISPLAY_XDISPLAY(gdk_screen_get_display(screen));
    Window rootWindow = XRootWindow(display, GDK_SCREEN_XNUMBER(screen));
    Atom currentDesktop = XInternAtom(display, "_NET_CURRENT_DESKTOP", True);

    Atom type;
    int format;
    unsigned long itemsCount, bytesAfter;
    unsigned char *returnedData = NULL;
    XGetWindowProperty(display, rootWindow, currentDesktop, 0, G_MAXLONG, False, XA_CARDINAL,
                       &type, &format, &itemsCount, &bytesAfter, &returnedData);

    int workspace = 0;
    if (type == XA_CARDINAL && format == 32 && itemsCount > 0)
        workspace = (int)returnedData[0];

    if (returnedData)
        XFree(returnedData);

    return workspace;
}

static void getScreenWorkArea(GdkScreen *screen, GdkRectangle *area)
{
    Display *display = GDK_DISPLAY_XDISPLAY(gdk_screen_get_display(screen));
    Atom workArea = XInternAtom(display, "_NET_WORKAREA", True);

    /* Defaults in case of error. */
    area->x = 0;
    area->y = 0;
    area->width = gdk_screen_get_width(screen);
    area->height = gdk_screen_get_height(screen);

    if (workArea == None)
        return;

    Window rootWindow = XRootWindow(display, GDK_SCREEN_XNUMBER(screen));
    Atom type;
    int format;
    unsigned long itemsCount, bytesAfter;
    unsigned char *returnedData = 0;
    int result = XGetWindowProperty(display, rootWindow, workArea, 0, 4 * 32 /* Max length */, False, AnyPropertyType,
                                    &type, &format, &itemsCount, &bytesAfter, &returnedData);
    if (result != Success || type == None || !format || bytesAfter || itemsCount % 4)
        return;

    int desktop = getScreenCurrentDesktop(screen);
    long *workAreas = (long *)returnedData;
    area->x = workAreas[desktop * 4];
    area->y = workAreas[desktop * 4 + 1];
    area->width = workAreas[desktop * 4 + 2];
    area->height = workAreas[desktop * 4 + 3];

    XFree(returnedData);
}
#endif // GDK_WINDOWING_X11

void gdk_screen_get_monitor_workarea(GdkScreen *screen, int monitor, GdkRectangle *area)
{
    gdk_screen_get_monitor_geometry(screen, monitor, area);

#ifdef GDK_WINDOWING_X11
    GdkRectangle workArea;
    getScreenWorkArea(screen, &workArea);
    gdk_rectangle_intersect(&workArea, area, area);
#endif
}
#endif // GTK_API_VERSION_2
