/*
 * 2010 Igalia S.L
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
#include "DragImage.h"

#include "Image.h"
#include "RefPtrCairo.h"
#include <cairo.h>
#include <gdk/gdk.h>

namespace WebCore {

IntSize dragImageSize(DragImageRef image)
{
    if (image)
        return IntSize(cairo_image_surface_get_width(image), cairo_image_surface_get_height(image));

    return IntSize(0, 0);
}

void deleteDragImage(DragImageRef image)
{
    if (image)
        cairo_surface_destroy(image);
}

DragImageRef scaleDragImage(DragImageRef image, FloatSize scale)
{
    if (!image)
        return 0;

    int newWidth = scale.width() * cairo_image_surface_get_width(image);
    int newHeight = scale.height() * cairo_image_surface_get_height(image);
    cairo_surface_t* scaledSurface = cairo_surface_create_similar(image, CAIRO_CONTENT_COLOR_ALPHA, newWidth, newHeight);

    RefPtr<cairo_t> context = adoptRef(cairo_create(scaledSurface));
    cairo_scale(context.get(), scale.width(), scale.height());
    cairo_pattern_set_extend(cairo_get_source(context.get()), CAIRO_EXTEND_PAD);
    cairo_pattern_set_filter(cairo_get_source(context.get()), CAIRO_FILTER_BEST);
    cairo_set_operator(context.get(), CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(context.get(), image, 0, 0);
    cairo_paint(context.get());

    deleteDragImage(image);
    return scaledSurface;
}

DragImageRef dissolveDragImageToFraction(DragImageRef image, float fraction)
{
    if (!image)
        return 0;

    if (!gdk_screen_is_composited(gdk_screen_get_default()))
        return image;

    RefPtr<cairo_t> context = adoptRef(cairo_create(image));
    cairo_set_operator(context.get(), CAIRO_OPERATOR_DEST_IN);
    cairo_set_source_rgba(context.get(), 0, 0, 0, fraction);
    cairo_paint(context.get());
    return image;
}

DragImageRef createDragImageFromImage(Image* image, RespectImageOrientationEnum)
{
    return image->nativeImageForCurrentFrame().leakRef();
}

DragImageRef createDragImageIconForCachedImageFilename(const String&)
{
    return 0;
}

}
