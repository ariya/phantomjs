/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "BitmapImage.h"
#include "FileSystem.h"
#include "GdkCairoUtilities.h"
#include "GOwnPtrGtk.h"
#include "SharedBuffer.h"
#include <wtf/text/CString.h>
#include <cairo.h>
#include <gtk/gtk.h>

namespace WebCore {

static char* getPathToImageResource(char* resource)
{
    if (g_getenv("WEBKIT_TOP_LEVEL"))
        return g_build_filename(g_getenv("WEBKIT_TOP_LEVEL"), "Source", "WebCore", "Resources", resource, NULL);

    return g_build_filename(sharedResourcesPath().data(), "images", resource, NULL);
}

static CString getThemeIconFileName(const char* name, int size)
{
    GtkIconInfo* iconInfo = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(),
                                                       name, size, GTK_ICON_LOOKUP_NO_SVG);
    // Try to fallback on MISSING_IMAGE.
    if (!iconInfo)
        iconInfo = gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(),
                                              GTK_STOCK_MISSING_IMAGE, size,
                                              GTK_ICON_LOOKUP_NO_SVG);
    if (iconInfo) {
        GOwnPtr<GtkIconInfo> info(iconInfo);
        return CString(gtk_icon_info_get_filename(info.get()));
    }

    // No icon was found, this can happen if not GTK theme is set. In
    // that case an empty Image will be created.
    return CString();
}

static PassRefPtr<SharedBuffer> loadResourceSharedBuffer(CString name)
{
    GOwnPtr<gchar> content;
    gsize length;
    if (!g_file_get_contents(name.data(), &content.outPtr(), &length, 0))
        return SharedBuffer::create();

    return SharedBuffer::create(content.get(), length);
}

void BitmapImage::invalidatePlatformData()
{
}

PassRefPtr<Image> loadImageFromFile(CString fileName)
{
    RefPtr<BitmapImage> img = BitmapImage::create();
    if (!fileName.isNull()) {
        RefPtr<SharedBuffer> buffer = loadResourceSharedBuffer(fileName);
        img->setData(buffer.release(), true);
    }
    return img.release();
}

PassRefPtr<Image> Image::loadPlatformResource(const char* name)
{
    CString fileName;
    if (!strcmp("missingImage", name))
        fileName = getThemeIconFileName(GTK_STOCK_MISSING_IMAGE, 16);
    if (fileName.isNull()) {
        GOwnPtr<gchar> imageName(g_strdup_printf("%s.png", name));
        GOwnPtr<gchar> glibFileName(getPathToImageResource(imageName.get()));
        fileName = glibFileName.get();
    }

    return loadImageFromFile(fileName);
}

PassRefPtr<Image> Image::loadPlatformThemeIcon(const char* name, int size)
{
    return loadImageFromFile(getThemeIconFileName(name, size));
}

GdkPixbuf* BitmapImage::getGdkPixbuf()
{
    RefPtr<cairo_surface_t> surface = nativeImageForCurrentFrame();
    return surface ? cairoImageSurfaceToGdkPixbuf(surface.get()) : 0;
}

}
