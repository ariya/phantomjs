/*
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2010 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "CairoUtilitiesEfl.h"

#include "RefPtrCairo.h"

namespace WebCore {

PassRefPtr<Evas_Object> evasObjectFromCairoImageSurface(Evas* canvas, cairo_surface_t* surface)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(canvas, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(surface, 0);

    cairo_status_t status = cairo_surface_status(surface);
    if (status != CAIRO_STATUS_SUCCESS) {
        EINA_LOG_ERR("cairo surface is invalid: %s", cairo_status_to_string(status));
        return 0;
    }

    cairo_surface_type_t type = cairo_surface_get_type(surface);
    if (type != CAIRO_SURFACE_TYPE_IMAGE) {
        EINA_LOG_ERR("unknown surface type %d, required %d (CAIRO_SURFACE_TYPE_IMAGE).",
            type, CAIRO_SURFACE_TYPE_IMAGE);
        return 0;
    }

    cairo_format_t format = cairo_image_surface_get_format(surface);
    if (format != CAIRO_FORMAT_ARGB32 && format != CAIRO_FORMAT_RGB24) {
        EINA_LOG_ERR("unknown surface format %d, expected %d or %d.",
            format, CAIRO_FORMAT_ARGB32, CAIRO_FORMAT_RGB24);
        return 0;
    }

    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    int stride = cairo_image_surface_get_stride(surface);
    if (width <= 0 || height <= 0 || stride <= 0) {
        EINA_LOG_ERR("invalid image size %dx%d, stride=%d", width, height, stride);
        return 0;
    }

    void* data = cairo_image_surface_get_data(surface);
    if (!data) {
        EINA_LOG_ERR("could not get source data.");
        return 0;
    }

    RefPtr<Evas_Object> image = adoptRef(evas_object_image_filled_add(canvas));
    if (!image) {
        EINA_LOG_ERR("could not add image to canvas.");
        return 0;
    }

    evas_object_image_colorspace_set(image.get(), EVAS_COLORSPACE_ARGB8888);
    evas_object_image_size_set(image.get(), width, height);
    evas_object_image_alpha_set(image.get(), format == CAIRO_FORMAT_ARGB32);

    if (evas_object_image_stride_get(image.get()) != stride) {
        EINA_LOG_ERR("evas' stride %d diverges from cairo's %d.",
            evas_object_image_stride_get(image.get()), stride);
        return 0;
    }

    evas_object_image_data_copy_set(image.get(), data);

    return image.release();
}

PassRefPtr<cairo_surface_t> createSurfaceForBackingStore(Ecore_Evas* ee)
{
    ASSERT(ee);

    int width;
    int height;
    ecore_evas_geometry_get(ee, 0, 0, &width, &height);
    ASSERT(width > 0 && height > 0);

    unsigned char* buffer = static_cast<unsigned char*>(const_cast<void*>(ecore_evas_buffer_pixels_get(ee)));
    RefPtr<cairo_surface_t> surface = adoptRef(cairo_image_surface_create_for_data(buffer, CAIRO_FORMAT_ARGB32, width, height, width * 4));

    cairo_status_t status = cairo_surface_status(surface.get());
    if (status != CAIRO_STATUS_SUCCESS) {
        EINA_LOG_ERR("Could not create cairo surface: %s", cairo_status_to_string(status));
        return 0;
    }

    return surface.release();
}

PassRefPtr<cairo_surface_t> createSurfaceForImage(Evas_Object* image)
{
    ASSERT(image);

    Evas_Coord width;
    Evas_Coord height;
    evas_object_image_size_get(image, &width, &height);
    ASSERT(width > 0 && height > 0);

    unsigned char* buffer = static_cast<unsigned char*>(const_cast<void*>(evas_object_image_data_get(image, true)));
    RefPtr<cairo_surface_t> surface = adoptRef(cairo_image_surface_create_for_data(buffer, CAIRO_FORMAT_ARGB32, width, height, width * 4));

    cairo_status_t status = cairo_surface_status(surface.get());
    if (status != CAIRO_STATUS_SUCCESS) {
        EINA_LOG_ERR("Could not create cairo surface: %s", cairo_status_to_string(status));
        return 0;
    }

    return surface.release();
}

}
