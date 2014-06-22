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

#include "config.h"

#include "TiledBackingStore.h"
#include "ewk_paint_context_private.h"
#include "ewk_private.h"

#if ENABLE(INSPECTOR)
#include "InspectorController.h"
#include "Page.h"
#endif

Ewk_Paint_Context* ewk_paint_context_new(cairo_t* cairo)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(cairo, 0);

    Ewk_Paint_Context* context = new Ewk_Paint_Context;
    context->graphicContext = adoptPtr(new WebCore::GraphicsContext(cairo));
    context->cairo = adoptRef(cairo_reference(cairo));
    context->image = 0;
    context->pixels = 0;

    return context;
}

Ewk_Paint_Context* ewk_paint_context_from_image_new(Evas_Object* image)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(image, 0);

    Evas_Coord width, height;
    uint8_t* pixels = static_cast<uint8_t*>(evas_object_image_data_get(image, true));
    evas_object_image_size_get(image, &width, &height);

    Ewk_Paint_Context* context = ewk_paint_context_from_image_data_new(pixels, width, height, EVAS_COLORSPACE_ARGB8888);
    if (context) {
        context->pixels = pixels;
        context->image = image;
    }

    return context;
}

Ewk_Paint_Context* ewk_paint_context_from_image_data_new(uint8_t* pixels, int width, int height, int colorSpace)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(pixels, 0);

    OwnPtr<Ewk_Paint_Context> context = adoptPtr(new Ewk_Paint_Context);
    context->pixels = pixels;
    context->image = 0;

    cairo_format_t format;
    int stride;
    switch (colorSpace) {
    case EVAS_COLORSPACE_ARGB8888:
        stride = width * 4;
        format = CAIRO_FORMAT_ARGB32;
        break;
    case EVAS_COLORSPACE_RGB565_A5P:
        stride = width * 2;
        format = CAIRO_FORMAT_RGB16_565;
        break;
    default:
        ERR("unknown color space: %d", colorSpace);
        return 0;
    }

    context->surface = adoptRef(cairo_image_surface_create_for_data(pixels, format, width, height, stride));
    cairo_status_t status = cairo_surface_status(context->surface.get());
    if (status != CAIRO_STATUS_SUCCESS) {
        ERR("could not create surface from data %dx%d: %s", width, height, cairo_status_to_string(status));
        return 0;
    }

    context->cairo = adoptRef(cairo_create(context->surface.get()));
    status = cairo_status(context->cairo.get());
    if (status != CAIRO_STATUS_SUCCESS) {
        ERR("could not create cairo from surface %dx%d: %s", width, height, cairo_status_to_string(status));
        return 0;
    }

    context->graphicContext = adoptPtr(new WebCore::GraphicsContext(context->cairo.get()));

    return context.release().leakPtr();
}

void ewk_paint_context_free(Ewk_Paint_Context* context)
{
    EINA_SAFETY_ON_NULL_RETURN(context);

    if (context->image && context->pixels) {
        // Decrease refcount inside image object.
        evas_object_image_data_set(context->image, context->pixels);
    }
    delete context;
}

void ewk_paint_context_save(Ewk_Paint_Context* context)
{
    EINA_SAFETY_ON_NULL_RETURN(context);

    cairo_save(context->cairo.get());
    context->graphicContext->save();
}

void ewk_paint_context_restore(Ewk_Paint_Context* context)
{
    EINA_SAFETY_ON_NULL_RETURN(context);

    context->graphicContext->restore();
    cairo_restore(context->cairo.get());
}

void ewk_paint_context_clip(Ewk_Paint_Context* context, const Eina_Rectangle* area)
{
    EINA_SAFETY_ON_NULL_RETURN(context);
    EINA_SAFETY_ON_NULL_RETURN(area);

    context->graphicContext->clip(WebCore::IntRect(*area));
}

void ewk_paint_context_scale(Ewk_Paint_Context* context, float scaleX, float scaleY)
{
    EINA_SAFETY_ON_NULL_RETURN(context);

    context->graphicContext->scale(WebCore::FloatSize(scaleX, scaleY));
}

void ewk_paint_context_translate(Ewk_Paint_Context* context, float x, float y)
{
    EINA_SAFETY_ON_NULL_RETURN(context);

    context->graphicContext->translate(x, y);
}

void ewk_paint_context_paint(Ewk_Paint_Context* context, WebCore::FrameView* view, const Eina_Rectangle* area)
{
    EINA_SAFETY_ON_NULL_RETURN(context);
    EINA_SAFETY_ON_NULL_RETURN(view);
    EINA_SAFETY_ON_NULL_RETURN(area);

    WebCore::IntRect paintArea(*area);

#if USE(TILED_BACKING_STORE)
    if (view->frame()->tiledBackingStore()) {
        int scrollX = view->scrollX();
        int scrollY = view->scrollY();

        context->graphicContext->translate(-scrollX, -scrollY);

        paintArea.move(scrollX, scrollY);

        view->frame()->tiledBackingStore()->paint(context->graphicContext.get(), paintArea);
        return;
    }
#endif

    if (view->isTransparent())
        context->graphicContext->clearRect(paintArea);
    view->paint(context->graphicContext.get(), paintArea);
}

void ewk_paint_context_paint_contents(Ewk_Paint_Context* context, WebCore::FrameView* view, const Eina_Rectangle* area)
{
    EINA_SAFETY_ON_NULL_RETURN(context);
    EINA_SAFETY_ON_NULL_RETURN(view);
    EINA_SAFETY_ON_NULL_RETURN(area);

    WebCore::IntRect paintArea(*area);

    if (view->isTransparent())
        context->graphicContext->clearRect(paintArea);
    view->paintContents(context->graphicContext.get(), paintArea);

#if ENABLE(INSPECTOR)
    WebCore::Page* page = view->frame()->page();
    if (page) {
        WebCore::InspectorController* controller = page->inspectorController();
        if (controller->highlightedNode())
            controller->drawHighlight(*context->graphicContext);
    }
#endif
}
