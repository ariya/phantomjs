/*
    Copyright (C) 2009-2012 Samsung Electronics
    Copyright (C) 2009-2010 ProFUSION embedded systems

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

/**
 * @file    ewk_paint_context_private.h
 * @brief   Describes the paint context API.
 */

#ifndef ewk_paint_context_private_h
#define ewk_paint_context_private_h

#include "FrameView.h"
#include "GraphicsContext.h"
#include "RefPtrCairo.h"
#include <Evas.h>
#include <cairo.h>

/// Creates a type name for @a _Ewk_Paint_Context.
typedef struct _Ewk_Paint_Context Ewk_Paint_Context;

/**
 * @brief Structure that keeps the paint context.
 *
 * @internal
 *
 * @note This is not for general use but just for subclasses that want
 *       to define their own backing store.
 */
struct _Ewk_Paint_Context {
    OwnPtr<WebCore::GraphicsContext> graphicContext;
    RefPtr<cairo_t> cairo;
    RefPtr<cairo_surface_t> surface; /**< surface used to create cairo object */
    Evas_Object* image; /**< image used to create cairo surface */
    unsigned char* pixels; /**< pixels form image */
};

/**
 * @internal
 * Creates a new paint context using a cairo as output.
 *
 * @param cairo context to use as paint destination, a new
 *        reference is taken, so it's safe to call @c cairo_destroy()
 *        after this function returns.
 *
 * @return a newly allocated instance of @c Ewk_Paint_Context on success,
 *         or @c 0 on failure
 *
 * @note This is not for general use but just for subclasses that want
 *       to define their own backing store.
 */
Ewk_Paint_Context* ewk_paint_context_new(cairo_t* cairo);

/**
 * @internal
 * Creates a new paint context using an image as output.
 *
 * @param image to use as paint destination
 *
 * @return a newly allocated instance of @c Ewk_Paint_Context on success,
 *         or @c 0 on failure
 *
 * @note This is not for general use but just for subclasses that want
 *       to define their own backing store.
 */
Ewk_Paint_Context* ewk_paint_context_from_image_new(Evas_Object* image);

/**
 * @internal
 * Creates a new paint context using an image as output.
 *
 * @param pixel pointer to pixel buffer
 * @param width size of pixel buffer
 * @param height size of pixel buffer
 * @param colorSpace Evas_Colorspace of pixel buffer
 *
 * @return a newly allocated instance of @c Ewk_Paint_Context on success,
 *         or @c 0 on failure
 *
 * @note This is not for general use but just for subclasses that want
 *       to define their own backing store.
 */
Ewk_Paint_Context* ewk_paint_context_from_image_data_new(uint8_t* pixels, int width, int height, int colorSpace);

/**
 * @internal
 * Destroys the previously created the paint context.
 *
 * @param context the paint context to destroy, must @b not be @c 0
 *
 * @note This is not for general use but just for subclasses that want
 *       to define their own backing store.
 */
void ewk_paint_context_free(Ewk_Paint_Context* context);

/**
 * @internal
 * Saves (push to stack) the paint context status.
 *
 * @param context the paint context to save, must @b not be @c 0
 *
 * @see ewk_paint_context_restore()
 *
 * @note This is not for general use but just for subclasses that want
 *       to define their own backing store.
 */
void ewk_paint_context_save(Ewk_Paint_Context* context);

/**
 * @internal
 * Restores (pop from stack) the paint context status.
 *
 * @param context the paint context to restore, must @b not be @c 0
 *
 * @see ewk_paint_context_save()
 *
 * @note This is not for general use but just for subclasses that want
 *       to define their own backing store.
 */
void ewk_paint_context_restore(Ewk_Paint_Context* context);

/**
 * @internal
 * Clips the paint context drawings to the given area.
 *
 * @param context the paint context to clip, must @b not be @c 0
 * @param area clip area to use, must @b not be @c 0
 *
 * @see ewk_paint_context_save()
 * @see ewk_paint_context_restore()
 *
 * @note This is not for general use but just for subclasses that want
 *       to define their own backing store.
 */
void ewk_paint_context_clip(Ewk_Paint_Context* context, const Eina_Rectangle* area);

/**
 * @internal
 * Scales the contents by the given factors.
 *
 * This function applies a scaling transformation using Cairo.
 *
 * @param context the paint context to scale, must @b not be @c 0
 * @param scale_x the scale factor for the X dimension
 * @param scale_y the scale factor for the Y dimension
 */
void ewk_paint_context_scale(Ewk_Paint_Context* context, float scale_x, float scale_y);

/**
 * @internal
 * Performs a translation of the origin coordinates.
 *
 * This function moves the origin coordinates by @a x and @a y pixels.
 *
 * @param context the paint context to translate, must @b not be @c 0
 * @param x amount of pixels to translate in the X dimension
 * @param y amount of pixels to translate in the Y dimension
 */
void ewk_paint_context_translate(Ewk_Paint_Context* context, float x, float y);

/**
 * @internal
 * Paints the context using given area.
 *
 * @param context the paint context to paint, must @b not be @c 0
 * @param view the view to paint
 * @param area the paint area to use, coordinates are relative to current viewport,
 *        thus "scrolled", must @b not be @c 0
 *
 * @note One may use cairo functions on the cairo context to
 *       translate, scale or any modification that may fit his desires.
 *
 * @see ewk_paint_context_clip()
 * @see ewk_paint_context_paint_contents()
 *
 * @note This is not for general use but just for subclasses that want
 *       to define their own backing store.
 */
void ewk_paint_context_paint(Ewk_Paint_Context* context, WebCore::FrameView* view, const Eina_Rectangle* area);

/**
 * @internal
 * Paints just contents using context using given area.
 *
 * Unlike ewk_paint_context_paint(), this function paint just
 * bare contents and ignores any scrolling, scrollbars and extras. It
 * will walk the rendering tree and paint contents inside the given
 * area to the cairo context specified in @a context.
 *
 * @param context the paint context to paint, must @b not be @c 0.
 * @param view the view to paint
 * @param area the paint area to use, coordinates are absolute to page, must @b not be @c 0
 *
 * @note One may use cairo functions on the cairo context to
 *       translate, scale or any modification that may fit his desires.
 *
 * @see ewk_paint_context_clip()
 * @see ewk_paint_context_paint()
 *
 * @note This is not for general use but just for subclasses that want
 *       to define their own backing store.
 */
void ewk_paint_context_paint_contents(Ewk_Paint_Context* context, WebCore::FrameView* view, const Eina_Rectangle* area);

#endif // ewk_paint_context_private_h
