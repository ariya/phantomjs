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
#include "ewk_view.h"

#include "TiledBackingStore.h"
#include "ewk_frame_private.h"
#include "ewk_private.h"
#include "ewk_view_private.h"

#include <Ecore_Evas.h>
#include <Evas.h>
#include <eina_safety_checks.h>
#include <string.h>

#if ENABLE(INSPECTOR)
#include "InspectorController.h"
#include "Page.h"
#endif

static Ewk_View_Smart_Class _parent_sc = EWK_VIEW_SMART_CLASS_INIT_NULL;

static void _ewk_view_single_on_del(void* data, Evas*, Evas_Object*, void*)
{
    Evas_Object* clip = (Evas_Object*)data;
    evas_object_del(clip);
}

static void _ewk_view_single_smart_add(Evas_Object* ewkView)
{
    Ewk_View_Smart_Data* smartData;

    _parent_sc.sc.add(ewkView);

    smartData = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(ewkView));
    if (!smartData)
        return;

    Evas_Object* clip = evas_object_rectangle_add(smartData->base.evas);
    evas_object_show(clip);

    evas_object_event_callback_add
        (smartData->backing_store, EVAS_CALLBACK_DEL, _ewk_view_single_on_del, clip);
}

static Evas_Object* _ewk_view_single_smart_backing_store_add(Ewk_View_Smart_Data* smartData)
{
    Evas_Object* bs = evas_object_image_add(smartData->base.evas);
    EINA_SAFETY_ON_NULL_RETURN_VAL(bs, 0);
    const Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(smartData->base.evas);
    const char* engine = ecore_evas_engine_name_get(ecoreEvas);
    if (!strncmp(engine, "opengl_x11", strlen("opengl_x11")))
        evas_object_image_content_hint_set(bs, EVAS_IMAGE_CONTENT_HINT_DYNAMIC);

    evas_object_image_alpha_set(bs, false);
    evas_object_image_smooth_scale_set(bs, smartData->zoom_weak_smooth_scale);

    return bs;
}

static void _ewk_view_single_smart_resize(Evas_Object* ewkView, Evas_Coord width, Evas_Coord height)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(ewkView));
    _parent_sc.sc.resize(ewkView, width, height);

    if (!smartData)
        return;

    // these should be queued and processed in calculate as well!
    evas_object_image_size_set(smartData->backing_store, width, height);
    if (smartData->animated_zoom.zoom.current < 0.00001) {
        Evas_Object* clip = evas_object_clip_get(smartData->backing_store);
        Evas_Coord x, y, cw, ch;
        evas_object_image_fill_set(smartData->backing_store, 0, 0, width, height);
        evas_object_geometry_get(smartData->backing_store, &x, &y, 0, 0);
        evas_object_move(clip, x, y);
        ewk_frame_contents_size_get(smartData->main_frame, &cw, &ch);
        if (width > cw)
            width = cw;
        if (height > ch)
            height = ch;
        evas_object_resize(clip, width, height);
    }
}

static inline void _ewk_view_screen_move(uint32_t* image, size_t destinationX, size_t destinationY, size_t sourceX, size_t sourceY, size_t copyWidth, size_t copyHeight, size_t imageWidth)
{
    uint32_t* sourceBegin = image + (imageWidth * sourceY) + sourceX;
    uint32_t* destinationBegin = image + (imageWidth * destinationY) + destinationX;

    size_t copyLength = copyWidth * 4;
    const int moveLineUpDown = sourceY >= destinationY ? 1 : -1;
    int startHeight = sourceY >= destinationY ? 0 : copyHeight - 1;

    uint32_t* source, * destination;
    if ((destinationX > sourceX && destinationX < sourceX + copyWidth)
        || (destinationX < sourceX && destinationX + copyWidth > sourceX)) {
        for (size_t i = 0; i < copyHeight; i++) {
            source = sourceBegin + (imageWidth * startHeight);
            destination = destinationBegin + (imageWidth * startHeight);
            startHeight = startHeight + moveLineUpDown;
            memmove(destination, source, copyLength);
        }
    } else {
        for (size_t i = 0; i < copyHeight; i++) {
            source = sourceBegin + (imageWidth * startHeight);
            destination = destinationBegin + (imageWidth * startHeight);
            startHeight = startHeight + moveLineUpDown;
            memcpy(destination, source, copyLength);
        }
    }
}

static inline void _ewk_view_single_scroll_process_single(Ewk_View_Smart_Data* smartData, void* pixels, Evas_Coord width, Evas_Coord height, const WebCore::IntSize& scrollOffset, const WebCore::IntRect& rectToScroll)
{
    int scrollX = rectToScroll.x();
    int scrollY = rectToScroll.y();
    int scrollWidth = rectToScroll.width();
    int scrollHeight = rectToScroll.height();

    DBG("%d,%d + %d,%d %+03d,%+03d, store: %p %dx%d",
        scrollX, scrollY, scrollWidth, scrollHeight, scrollOffset.width(), scrollOffset.height(), pixels, width, height);

    if (abs(scrollOffset.width()) >= scrollWidth || abs(scrollOffset.height()) >= scrollHeight) {
        ewk_view_repaint_add(smartData->_priv, scrollX, scrollY, scrollWidth, scrollHeight);
        return;
    }

    if (scrollX < 0) {
        scrollWidth += scrollX;
        scrollX = 0;
    }
    if (scrollY < 0) {
        scrollHeight += scrollY;
        scrollY = 0;
    }

    if (scrollX + scrollWidth > width)
        scrollWidth = width - scrollX;
    if (scrollY + scrollHeight > height)
        scrollHeight = height - scrollY;

    if (scrollWidth <= 0 || scrollHeight <= 0)
        return;

    int sourceX = scrollOffset.width() < 0 ? abs(scrollOffset.width()) : 0;
    int sourceY = scrollOffset.height() < 0 ? abs(scrollOffset.height()) : 0;
    int destinationX = scrollOffset.width() < 0 ? 0 : scrollOffset.width();
    int destinationY = scrollOffset.height() < 0 ? 0 : scrollOffset.height();
    int copyWidth = scrollWidth - abs(scrollOffset.width());
    int copyHeight = scrollHeight - abs(scrollOffset.height());
    if (scrollOffset.width() || scrollOffset.height()) {
        _ewk_view_screen_move(static_cast<uint32_t*>(pixels), destinationX, destinationY, sourceX, sourceY, copyWidth, copyHeight, width);
        evas_object_image_data_update_add(smartData->backing_store, destinationX, destinationY, copyWidth, copyHeight);
    }

    Eina_Rectangle verticalUpdate;
    verticalUpdate.x = destinationX ? 0 : copyWidth - 1;
    verticalUpdate.y = 0;
    verticalUpdate.w = abs(scrollOffset.width());
    verticalUpdate.h = scrollHeight;
    if (verticalUpdate.w && verticalUpdate.h)
        ewk_view_repaint_add(smartData->_priv, verticalUpdate.x, verticalUpdate.y, verticalUpdate.w, verticalUpdate.h);

    Eina_Rectangle horizontalUpdate;
    horizontalUpdate.x = destinationX;
    horizontalUpdate.y = destinationY ? 0 : copyHeight - 1;
    horizontalUpdate.w = copyWidth;
    horizontalUpdate.h = abs(scrollOffset.height());
    if (horizontalUpdate.w && horizontalUpdate.h)
        ewk_view_repaint_add(smartData->_priv, horizontalUpdate.x, horizontalUpdate.y, horizontalUpdate.w, horizontalUpdate.h);
}

static Eina_Bool _ewk_view_single_smart_scrolls_process(Ewk_View_Smart_Data* smartData)
{
    Evas_Coord imageWidth, imageHeight;
    void* pixels = evas_object_image_data_get(smartData->backing_store, 1);
    evas_object_image_size_get(smartData->backing_store, &imageWidth, &imageHeight);

    const WTF::Vector<WebCore::IntSize>& scrollOffset = ewk_view_scroll_offsets_get(smartData->_priv);
    const WTF::Vector<WebCore::IntRect>& rectsToScroll = ewk_view_scroll_rects_get(smartData->_priv);
    for (size_t i = 0; i < scrollOffset.size(); ++i)
        _ewk_view_single_scroll_process_single(smartData, pixels, imageWidth, imageHeight, scrollOffset[i], rectsToScroll[i]);

    evas_object_image_data_set(smartData->backing_store, pixels);

    return true;
}

static Eina_Bool _ewk_view_single_smart_repaints_process(Ewk_View_Smart_Data* smartData)
{
    if (smartData->animated_zoom.zoom.current < 0.00001) {
        Evas_Object* clip = evas_object_clip_get(smartData->backing_store);

        // reset effects of zoom_weak_set()
        evas_object_image_fill_set(smartData->backing_store, 0, 0, smartData->view.w, smartData->view.h);
        evas_object_move(clip, smartData->view.x, smartData->view.y);

        Evas_Coord width = smartData->view.w;
        Evas_Coord height = smartData->view.h;

        Evas_Coord centerWidth, centerHeight;
        ewk_frame_contents_size_get(smartData->main_frame, &centerWidth, &centerHeight);
        if (width > centerWidth)
            width = centerWidth;

        if (height > centerHeight)
            height = centerHeight;

        evas_object_resize(clip, width, height);
    }

    Evas_Coord imageWidth, imageHeight;
    evas_object_image_size_get(smartData->backing_store, &imageWidth, &imageHeight);

    Eina_Tiler* tiler = eina_tiler_new(imageWidth, imageHeight);
    if (!tiler) {
        ERR("could not create tiler %dx%d", imageWidth, imageHeight);
        return false;
    }

    ewk_view_layout_if_needed_recursive(smartData->_priv);

    size_t count;
    const Eina_Rectangle* paintRequest = ewk_view_repaints_pop(smartData->_priv, &count);
    const Eina_Rectangle* paintRequestEnd = paintRequest + count;
    for (; paintRequest < paintRequestEnd; paintRequest++)
        eina_tiler_rect_add(tiler, paintRequest);

    Eina_Iterator* iterator = eina_tiler_iterator_new(tiler);
    if (!iterator) {
        ERR("could not get iterator for tiler");
        eina_tiler_free(tiler);
        return false;
    }

#if USE(TILED_BACKING_STORE)
    WebCore::Frame* mainFrame = EWKPrivate::coreFrame(smartData->main_frame);
    if (mainFrame && mainFrame->tiledBackingStore())
        mainFrame->tiledBackingStore()->coverWithTilesIfNeeded();
#endif

    Ewk_Paint_Context* context = ewk_paint_context_from_image_new(smartData->backing_store);
    ewk_paint_context_save(context);

    Eina_Rectangle* rect;
    EINA_ITERATOR_FOREACH(iterator, rect) {
        ewk_view_paint(smartData->_priv, context, rect);
        evas_object_image_data_update_add(smartData->backing_store, rect->x, rect->y, rect->w, rect->h);
    }

#if ENABLE(INSPECTOR)
    WebCore::Page* page = EWKPrivate::corePage(smartData->self);
    if (page) {
        WebCore::InspectorController* controller = page->inspectorController();
        if (controller->highlightedNode())
            controller->drawHighlight(*context->graphicContext);
    }
#endif

    ewk_paint_context_restore(context);
    ewk_paint_context_free(context);

    eina_tiler_free(tiler);
    eina_iterator_free(iterator);

    return true;
}

static Eina_Bool _ewk_view_single_smart_zoom_weak_set(Ewk_View_Smart_Data* smartData, float zoom, Evas_Coord centerX, Evas_Coord centerY)
{
    // TODO: review
    float scale = zoom / smartData->animated_zoom.zoom.start;
    Evas_Coord w = smartData->view.w * scale;
    Evas_Coord h = smartData->view.h * scale;
    Evas_Coord dx, dy, cw, ch;
    Evas_Object* clip = evas_object_clip_get(smartData->backing_store);

    ewk_frame_contents_size_get(smartData->main_frame, &cw, &ch);
    if (smartData->view.w > 0 && smartData->view.h > 0) {
        dx = (w * (smartData->view.w - centerX)) / smartData->view.w;
        dy = (h * (smartData->view.h - centerY)) / smartData->view.h;
    } else {
        dx = 0;
        dy = 0;
    }

    evas_object_image_fill_set(smartData->backing_store, centerX + dx, centerY + dy, w, h);

    if (smartData->view.w > 0 && smartData->view.h > 0) {
        dx = ((smartData->view.w - w) * centerX) / smartData->view.w;
        dy = ((smartData->view.h - h) * centerY) / smartData->view.h;
    } else {
        dx = 0;
        dy = 0;
    }
    evas_object_move(clip, smartData->view.x + dx, smartData->view.y + dy);

    if (cw < smartData->view.w)
        w = cw * scale;
    if (ch < smartData->view.h)
        h = ch * scale;
    evas_object_resize(clip, w, h);
    return true;
}

static void _ewk_view_single_smart_zoom_weak_smooth_scale_set(Ewk_View_Smart_Data* smartData, Eina_Bool smooth_scale)
{
    evas_object_image_smooth_scale_set(smartData->backing_store, smooth_scale);
}

static void _ewk_view_single_smart_bg_color_set(Ewk_View_Smart_Data* smartData, unsigned char /*red*/, unsigned char /*green*/, unsigned char /*blue*/, unsigned char alpha)
{
    evas_object_image_alpha_set(smartData->backing_store, alpha < 255);
}

Eina_Bool ewk_view_single_smart_set(Ewk_View_Smart_Class* api)
{
    if (!ewk_view_base_smart_set(api))
        return false;

    if (EINA_UNLIKELY(!_parent_sc.sc.add))
        ewk_view_base_smart_set(&_parent_sc);

    api->sc.add = _ewk_view_single_smart_add;
    api->sc.resize = _ewk_view_single_smart_resize;

    api->backing_store_add = _ewk_view_single_smart_backing_store_add;
    api->scrolls_process = _ewk_view_single_smart_scrolls_process;
    api->repaints_process = _ewk_view_single_smart_repaints_process;
    api->zoom_weak_set = _ewk_view_single_smart_zoom_weak_set;
    api->zoom_weak_smooth_scale_set = _ewk_view_single_smart_zoom_weak_smooth_scale_set;
    api->bg_color_set = _ewk_view_single_smart_bg_color_set;

    return true;
}

static inline Evas_Smart* _ewk_view_single_smart_class_new(void)
{
    static Ewk_View_Smart_Class api = EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION(ewkViewSingleName);
    static Evas_Smart* smart = 0;

    if (EINA_UNLIKELY(!smart)) {
        ewk_view_single_smart_set(&api);
        smart = evas_smart_class_new(&api.sc);
    }

    return smart;
}

Evas_Object* ewk_view_single_add(Evas* canvas)
{
    return evas_object_smart_add(canvas, _ewk_view_single_smart_class_new());
}
