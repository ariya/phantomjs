/*
 *  Copyright (C) 2010 Igalia S.L.
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
#include "ImageBuffer.h"

#include "GdkCairoUtilities.h"
#include <wtf/gobject/GOwnPtr.h>
#include "GRefPtrGtk.h"
#include "MIMETypeRegistry.h"
#include <cairo.h>
#include <gtk/gtk.h>
#include <wtf/text/Base64.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

static bool encodeImage(cairo_surface_t* surface, const String& mimeType, const double* quality, GOwnPtr<gchar>& buffer, gsize& bufferSize)
{
    // List of supported image encoding types comes from the GdkPixbuf documentation.
    // http://developer.gnome.org/gdk-pixbuf/stable/gdk-pixbuf-File-saving.html#gdk-pixbuf-save-to-bufferv

    String type = mimeType.substring(sizeof "image");
    if (type != "jpeg" && type != "png" && type != "tiff" && type != "ico" && type != "bmp")
        return false;

    GRefPtr<GdkPixbuf> pixbuf;
    if (type == "jpeg") {
        // JPEG doesn't support alpha channel. The <canvas> spec states that toDataURL() must encode a Porter-Duff
        // composite source-over black for image types that do not support alpha.
        RefPtr<cairo_surface_t> newSurface = adoptRef(cairo_image_surface_create_for_data(cairo_image_surface_get_data(surface),
                                                                                          CAIRO_FORMAT_RGB24,
                                                                                          cairo_image_surface_get_width(surface),
                                                                                          cairo_image_surface_get_height(surface),
                                                                                          cairo_image_surface_get_stride(surface)));
        pixbuf = adoptGRef(cairoImageSurfaceToGdkPixbuf(newSurface.get()));
    } else
        pixbuf = adoptGRef(cairoImageSurfaceToGdkPixbuf(surface));
    if (!pixbuf)
        return false;

    GError* error = 0;
    if (type == "jpeg" && quality && *quality >= 0.0 && *quality <= 1.0) {
        String qualityString = String::format("%d", static_cast<int>(*quality * 100.0 + 0.5));
        gdk_pixbuf_save_to_buffer(pixbuf.get(), &buffer.outPtr(), &bufferSize, type.utf8().data(), &error, "quality", qualityString.utf8().data(), NULL);
    } else
        gdk_pixbuf_save_to_buffer(pixbuf.get(), &buffer.outPtr(), &bufferSize, type.utf8().data(), &error, NULL);

    return !error;
}

String ImageBuffer::toDataURL(const String& mimeType, const double* quality, CoordinateSystem) const
{
    ASSERT(MIMETypeRegistry::isSupportedImageMIMETypeForEncoding(mimeType));

    GOwnPtr<gchar> buffer(0);
    gsize bufferSize;
    if (!encodeImage(m_data.m_surface.get(), mimeType, quality, buffer, bufferSize))
        return "data:,";

    Vector<char> base64Data;
    base64Encode(buffer.get(), bufferSize, base64Data);

    return "data:" + mimeType + ";base64," + base64Data;
}

}
