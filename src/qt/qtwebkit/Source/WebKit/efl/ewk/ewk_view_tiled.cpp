/*
    Copyright (C) 2009-2010 Samsung Electronics
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
#include "ewk_view.h"

#include "ewk_private.h"
#include "ewk_tiled_backing_store_private.h"
#include "ewk_view_private.h"
#include <Evas.h>
#include <eina_safety_checks.h>

static Ewk_View_Smart_Class _parent_sc = EWK_VIEW_SMART_CLASS_INIT_NULL;

static bool _ewk_view_tiled_render_cb(void* data, Ewk_Tile* tile, const Eina_Rectangle* area)
{
    Ewk_View_Private_Data* priv = static_cast<Ewk_View_Private_Data*>(data);
    Eina_Rectangle rect = {area->x + tile->x, area->y + tile->y, area->w, area->h};

    uint8_t* pixels = static_cast<uint8_t*>(evas_object_image_data_get(tile->image, true));
    Ewk_Paint_Context* context = ewk_paint_context_from_image_data_new(pixels, tile->width, tile->height, tile->cspace);

    ewk_paint_context_translate(context, -tile->x, -tile->y);
    bool result = ewk_view_paint_contents(priv, context, &rect);
    ewk_paint_context_free(context);

    evas_object_image_data_set(tile->image, pixels);

    return result;
}

static void* _ewk_view_tiled_updates_process_pre(void* data, Evas_Object*)
{
    Ewk_View_Private_Data* priv = static_cast<Ewk_View_Private_Data*>(data);
    ewk_view_layout_if_needed_recursive(priv);
    return 0;
}

static Evas_Object* _ewk_view_tiled_smart_backing_store_add(Ewk_View_Smart_Data* smartData)
{
    Evas_Object* backingStore = ewk_tiled_backing_store_add(smartData->base.evas);
    ewk_tiled_backing_store_render_cb_set(backingStore, _ewk_view_tiled_render_cb, smartData->_priv);
    ewk_tiled_backing_store_updates_process_pre_set
        (backingStore, _ewk_view_tiled_updates_process_pre, smartData->_priv);
    return backingStore;
}

static void
_ewk_view_tiled_contents_size_changed_cb(void* data, Evas_Object*, void* eventInfo)
{
    Evas_Coord* size = static_cast<Evas_Coord*>(eventInfo);
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);

    ewk_tiled_backing_store_contents_resize
        (smartData->backing_store, size[0], size[1]);
}

static void _ewk_view_tiled_smart_add(Evas_Object* ewkView)
{
    Ewk_View_Smart_Data* smartData;

    _parent_sc.sc.add(ewkView);

    smartData = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(ewkView));
    if (!smartData)
        return;

    evas_object_smart_callback_add(
        smartData->main_frame, "contents,size,changed",
        _ewk_view_tiled_contents_size_changed_cb, smartData);
}

static Eina_Bool _ewk_view_tiled_smart_scrolls_process(Ewk_View_Smart_Data* smartData)
{
    const WTF::Vector<WebCore::IntSize>& scrollOffset = ewk_view_scroll_offsets_get(smartData->_priv);
    for (size_t i = 0; i < scrollOffset.size(); ++i)
        ewk_tiled_backing_store_scroll_full_offset_add(smartData->backing_store, scrollOffset[i].width(), scrollOffset[i].height());

    return true;
}

static Eina_Bool _ewk_view_tiled_smart_repaints_process(Ewk_View_Smart_Data* smartData)
{
    const Eina_Rectangle* paintRect, * endOfpaintRect;
    size_t count;
    int scrollX, scrollY;

    ewk_frame_scroll_pos_get(smartData->main_frame, &scrollX, &scrollY);

    paintRect = ewk_view_repaints_pop(smartData->_priv, &count);
    endOfpaintRect = paintRect + count;
    for (; paintRect < endOfpaintRect; paintRect++) {
        Eina_Rectangle rect;
        rect.x = paintRect->x + scrollX;
        rect.y = paintRect->y + scrollY;
        rect.w = paintRect->w;
        rect.h = paintRect->h;
        ewk_tiled_backing_store_update(smartData->backing_store, &rect);
    }
    ewk_tiled_backing_store_updates_process(smartData->backing_store);

    return true;
}

static Eina_Bool _ewk_view_tiled_smart_contents_resize(Ewk_View_Smart_Data* smartData, int width, int height)
{
    ewk_tiled_backing_store_contents_resize(smartData->backing_store, width, height);
    return true;
}

static Eina_Bool _ewk_view_tiled_smart_zoom_set(Ewk_View_Smart_Data* smartData, float zoom, Evas_Coord centerX, Evas_Coord centerY)
{
    Evas_Coord x, y, width, height;
    Eina_Bool result;
    result = ewk_tiled_backing_store_zoom_set(smartData->backing_store,
                                         &zoom, centerX, centerY, &x, &y);
    if (!result)
        return result;
    ewk_tiled_backing_store_disabled_update_set(smartData->backing_store, true);
    result = _parent_sc.zoom_set(smartData, zoom, centerX, centerY);
    ewk_frame_scroll_set(smartData->main_frame, -x, -y);
    ewk_frame_scroll_size_get(smartData->main_frame, &width, &height);
    ewk_tiled_backing_store_fix_offsets(smartData->backing_store, width, height);
    ewk_view_scrolls_process(smartData);
    evas_object_smart_calculate(smartData->backing_store);
    evas_object_smart_calculate(smartData->self);
    ewk_tiled_backing_store_disabled_update_set(smartData->backing_store, false);
    return result;
}

static Eina_Bool _ewk_view_tiled_smart_zoom_weak_set(Ewk_View_Smart_Data* smartData, float zoom, Evas_Coord centerX, Evas_Coord centerY)
{
    return ewk_tiled_backing_store_zoom_weak_set(smartData->backing_store, zoom, centerX, centerY);
}

static void _ewk_view_tiled_smart_zoom_weak_smooth_scale_set(Ewk_View_Smart_Data* smartData, Eina_Bool smoothScale)
{
    ewk_tiled_backing_store_zoom_weak_smooth_scale_set(smartData->backing_store, smoothScale);
}

static void _ewk_view_tiled_smart_bg_color_set(Ewk_View_Smart_Data* smartData, unsigned char /*red*/, unsigned char /*green*/, unsigned char /*blue*/, unsigned char alpha)
{
    ewk_tiled_backing_store_alpha_set(smartData->backing_store, alpha < 255);
}

static void _ewk_view_tiled_smart_flush(Ewk_View_Smart_Data* smartData)
{
    ewk_tiled_backing_store_flush(smartData->backing_store);
    _parent_sc.flush(smartData);
}

static Eina_Bool _ewk_view_tiled_smart_pre_render_region(Ewk_View_Smart_Data* smartData, Evas_Coord x, Evas_Coord y, Evas_Coord width, Evas_Coord height, float zoom)
{
    return ewk_tiled_backing_store_pre_render_region
               (smartData->backing_store, x, y, width, height, zoom);
}

static Eina_Bool _ewk_view_tiled_smart_pre_render_relative_radius(Ewk_View_Smart_Data* smartData, unsigned int n, float zoom)
{
    return ewk_tiled_backing_store_pre_render_relative_radius
               (smartData->backing_store, n, zoom);
}

static inline int _ewk_view_tiled_rect_collision_check(Eina_Rectangle destination, Eina_Rectangle source)
{
    int direction = 0;
    if (destination.x < source.x)
        direction = direction | (1 << 0); // 0 bit shift, left
    if (destination.y < source.y)
        direction = direction | (1 << 1); // 1 bit shift, top
    if (destination.x + destination.w > source.x + source.w)
        direction = direction | (1 << 2); // 2 bit shift, right
    if (destination.y + destination.h > source.y + source.h)
        direction = direction | (1 << 3); // 3 bit shift, bottom
    DBG("check collision %d\r\n", direction);
    return direction;
}

static inline void _ewk_view_tiled_rect_collision_resolve(int direction, Eina_Rectangle* destination, Eina_Rectangle source)
{
    if (direction & (1 << 0)) // 0 bit shift, left
        destination->x = source.x;
    if (direction & (1 << 1)) // 1 bit shift, top
        destination->y = source.y;
    if (direction & (1 << 2)) // 2 bit shift, right
        destination->x = destination->x - ((destination->x + destination->w) - (source.x + source.w));
    if (direction & (1 << 3)) // 3 bit shift, bottom
        destination->y = destination->y - ((destination->y + destination->h) - (source.y + source.h));
}

static Eina_Bool _ewk_view_tiled_smart_pre_render_start(Ewk_View_Smart_Data* smartData)
{
    int contentWidth, contentHeight;
    ewk_frame_contents_size_get(smartData->main_frame, &contentWidth, &contentHeight);

    int viewX, viewY, viewWidth, viewHeight;
    ewk_frame_visible_content_geometry_get(smartData->main_frame, false, &viewX, &viewY, &viewWidth, &viewHeight);

    if (viewWidth <= 0 || viewHeight <= 0 || contentWidth <= 0 || contentHeight <= 0)
        return false;

    if (viewWidth >= contentWidth && viewHeight >= contentHeight)
        return false;

    int previousViewX, previousViewY;
    previousViewX = smartData->previousView.x;
    previousViewY = smartData->previousView.y;

    if (previousViewX < 0 || previousViewX > contentWidth || previousViewY < 0 || previousViewY > contentHeight)
        previousViewX = previousViewY = 0;

    float currentViewZoom = ewk_view_zoom_get(smartData->self);

    // pre-render works when two conditions are met.
    // zoom has been changed.
    // and the view has been moved more than tile size.
    if (abs(previousViewX - viewX) < defaultTileWidth
        && abs(previousViewY - viewY) < defaultTileHeigth
        && smartData->previousView.zoom == currentViewZoom) {
        return false;
    }

    // store previous view position and zoom.
    smartData->previousView.x = viewX;
    smartData->previousView.y = viewY;
    smartData->previousView.zoom = currentViewZoom;

    // cancelling previous pre-rendering list if exists.
    ewk_view_pre_render_cancel(smartData->self);

    Ewk_Tile_Unused_Cache* tileUnusedCache = ewk_view_tiled_unused_cache_get(smartData->self);
    int maxMemory = ewk_tile_unused_cache_max_get(tileUnusedCache);
    if (maxMemory <= viewWidth * viewHeight * EWK_ARGB_BYTES_SIZE)
        return false;

    Eina_Rectangle viewRect = {viewX, viewY, viewWidth, viewHeight};
    Eina_Rectangle contentRect = {0, 0, contentWidth, contentHeight};

    // get a base render rect.
    const int contentMemory = contentWidth * contentHeight * EWK_ARGB_BYTES_SIZE;

    // get render rect's width and height.
    Eina_Rectangle renderRect;
    if (maxMemory > contentMemory)
        renderRect = contentRect;
    else {
        // Make a base rectangle as big as possible with using maxMemory.
        // and then reshape the base rectangle to fit to contents.
        const int baseSize = static_cast<int>(sqrt(maxMemory / 4.0f));
        const float widthRate = (viewRect.w + (defaultTileWidth * 2)) / static_cast<float>(baseSize);
        const float heightRate = baseSize / static_cast<float>(contentHeight);
        const float rectRate = std::max(widthRate, heightRate);

        renderRect.w = static_cast<int>(baseSize * rectRate);
        renderRect.h = static_cast<int>(baseSize / rectRate);
        renderRect.x = viewRect.x + (viewRect.w / 2) - (renderRect.w / 2);
        renderRect.y = viewRect.y + (viewRect.h / 2) - (renderRect.h / 2);

        // reposition of renderRect, if the renderRect overlapped the content rect, this code moves the renderRect inside the content rect.
        int collisionSide = _ewk_view_tiled_rect_collision_check(renderRect, contentRect);
        if (collisionSide > 0)
            _ewk_view_tiled_rect_collision_resolve(collisionSide, &renderRect, contentRect);

        // check abnormal render rect
        if (renderRect.x < 0)
            renderRect.x = 0;
        if (renderRect.y < 0)
            renderRect.y = 0;
        if (renderRect.w > contentWidth)
            renderRect.w = contentWidth;
        if (renderRect.h > contentHeight)
            renderRect.h = contentHeight;
    }

    // enqueue tiles into tiled backing store in spiral order.
    ewk_tiled_backing_store_pre_render_spiral_queue(smartData->backing_store, &viewRect, &renderRect, maxMemory, currentViewZoom);

    return true;
}

static void _ewk_view_tiled_smart_pre_render_cancel(Ewk_View_Smart_Data* smartData)
{
    ewk_tiled_backing_store_pre_render_cancel(smartData->backing_store);
}

static Eina_Bool _ewk_view_tiled_smart_disable_render(Ewk_View_Smart_Data* smartData)
{
    return ewk_tiled_backing_store_disable_render(smartData->backing_store);
}

static Eina_Bool _ewk_view_tiled_smart_enable_render(Ewk_View_Smart_Data* smartData)
{
    return ewk_tiled_backing_store_enable_render(smartData->backing_store);
}

Eina_Bool ewk_view_tiled_smart_set(Ewk_View_Smart_Class* api)
{
    if (!ewk_view_base_smart_set(api))
        return false;

    if (EINA_UNLIKELY(!_parent_sc.sc.add)) {
        _parent_sc.sc.name = ewkViewTiledName;
        ewk_view_base_smart_set(&_parent_sc);
        api->sc.parent = reinterpret_cast<Evas_Smart_Class*>(&_parent_sc);
    }

    api->sc.add = _ewk_view_tiled_smart_add;

    api->backing_store_add = _ewk_view_tiled_smart_backing_store_add;
    api->scrolls_process = _ewk_view_tiled_smart_scrolls_process;
    api->repaints_process = _ewk_view_tiled_smart_repaints_process;
    api->contents_resize = _ewk_view_tiled_smart_contents_resize;
    api->zoom_set = _ewk_view_tiled_smart_zoom_set;
    api->zoom_weak_set = _ewk_view_tiled_smart_zoom_weak_set;
    api->zoom_weak_smooth_scale_set = _ewk_view_tiled_smart_zoom_weak_smooth_scale_set;
    api->bg_color_set = _ewk_view_tiled_smart_bg_color_set;
    api->flush = _ewk_view_tiled_smart_flush;
    api->pre_render_region = _ewk_view_tiled_smart_pre_render_region;
    api->pre_render_relative_radius = _ewk_view_tiled_smart_pre_render_relative_radius;
    api->pre_render_start = _ewk_view_tiled_smart_pre_render_start;
    api->pre_render_cancel = _ewk_view_tiled_smart_pre_render_cancel;
    api->disable_render = _ewk_view_tiled_smart_disable_render;
    api->enable_render = _ewk_view_tiled_smart_enable_render;
    return true;
}

static inline Evas_Smart* _ewk_view_tiled_smart_class_new(void)
{
    static Ewk_View_Smart_Class api = EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION(ewkViewTiledName);
    static Evas_Smart* smart = 0;

    if (EINA_UNLIKELY(!smart)) {
        ewk_view_tiled_smart_set(&api);
        smart = evas_smart_class_new(&api.sc);
    }

    return smart;
}

Evas_Object* ewk_view_tiled_add(Evas* canvas)
{
    return evas_object_smart_add(canvas, _ewk_view_tiled_smart_class_new());
}

Ewk_Tile_Unused_Cache* ewk_view_tiled_unused_cache_get(const Evas_Object* ewkView)
{
    EWK_VIEW_TYPE_CHECK_OR_RETURN(ewkView, ewkViewTiledName, 0);
    Ewk_View_Smart_Data* smartData = ewk_view_smart_data_get(ewkView);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData, 0);
    return ewk_tiled_backing_store_tile_unused_cache_get(smartData->backing_store);
}

void ewk_view_tiled_unused_cache_set(Evas_Object* ewkView, Ewk_Tile_Unused_Cache* cache)
{
    EWK_VIEW_TYPE_CHECK_OR_RETURN(ewkView, ewkViewTiledName);
    Ewk_View_Smart_Data* smartData = ewk_view_smart_data_get(ewkView);
    EINA_SAFETY_ON_NULL_RETURN(smartData);
    ewk_tiled_backing_store_tile_unused_cache_set(smartData->backing_store, cache);
}
