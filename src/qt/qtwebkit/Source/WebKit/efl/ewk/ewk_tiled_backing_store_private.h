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

#ifndef ewk_tiled_backing_store_private_h
#define ewk_tiled_backing_store_private_h

#include "EWebKit.h"
#include <Evas.h>

/* Enable accounting of render time in tile statistics */
// #define TILE_STATS_ACCOUNT_RENDER_TIME

/* If define ewk will do more accounting to check for memory leaks
 * try "kill -USR1 $PID" to get instantaneous debug
 * try "kill -USR2 $PID" to get instantaneous debug and force flush of cache
 */
#undef DEBUG_MEM_LEAKS

static const int defaultTileWidth = 256;
static const int defaultTileHeigth = 256;

static const float zoomStepMinimum = 0.01;

#define TILE_SIZE_AT_ZOOM(SIZE, ZOOM) ((int)roundf((SIZE) * (ZOOM)))
#define TILE_ZOOM_AT_SIZE(SIZE, ORIG_TILE) ((float)(SIZE) / (float)(ORIG_TILE))
#define ROUNDED_ZOOM(SIZE, ZOOM) ((float)(SIZE) / (float)(((int)roundf((SIZE) / (ZOOM)))))

typedef struct _Ewk_Tile                     Ewk_Tile;
typedef struct _Ewk_Tile_Stats               Ewk_Tile_Stats;
typedef struct _Ewk_Tile_Matrix              Ewk_Tile_Matrix;

struct _Ewk_Tile_Stats {
    double last_used;        /**< time of last use */
#ifdef TILE_STATS_ACCOUNT_RENDER_TIME
    double render_time;      /**< amount of time this tile took to render */
#endif
    unsigned int area;       /**< cache for (w * h) */
    unsigned int misses;     /**< number of times it became dirty but not
                              * repainted at all since it was not visible.
                              */
    bool full_update : 1; /**< tile requires full size update */
};

struct _Ewk_Tile {
    Eina_Tiler* updates;    /**< updated/dirty areas */
    Ewk_Tile_Stats stats;       /**< tile usage statistics */
    unsigned long column, row; /**< tile tile position */
    Evas_Coord x, y;        /**< tile coordinate position */

    /** Never ever change those after tile is created (respect const!) */
    const Evas_Coord width, height;        /**< tile size (see TILE_SIZE_AT_ZOOM()) */
    const Evas_Colorspace cspace; /**< colorspace */
    const float zoom;             /**< zoom level contents were rendered at */
    const size_t bytes;           /**< bytes used in pixels. keep
                                   * before pixels to guarantee
                                   * alignement!
                                   */
    int visible;                  /**< visibility counter of this tile */
    Evas_Object* image;           /**< Evas Image, the tile to be rendered */
};

/* view */
Evas_Object* ewk_tiled_backing_store_add(Evas* e);

void ewk_tiled_backing_store_render_cb_set(Evas_Object* o, bool (*cb)(void* data, Ewk_Tile* t, const Eina_Rectangle* area), const void* data);

bool ewk_tiled_backing_store_scroll_full_offset_set(Evas_Object* o, Evas_Coord x, Evas_Coord y);
bool ewk_tiled_backing_store_scroll_full_offset_add(Evas_Object* o, Evas_Coord dx, Evas_Coord dy);
bool ewk_tiled_backing_store_scroll_inner_offset_add(Evas_Object* o, Evas_Coord dx, Evas_Coord dy, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h);

bool ewk_tiled_backing_store_zoom_set(Evas_Object* o, float* zoom, Evas_Coord cx, Evas_Coord cy, Evas_Coord* offx, Evas_Coord* offy);
bool ewk_tiled_backing_store_zoom_weak_set(Evas_Object* o, float zoom, Evas_Coord cx, Evas_Coord cy);
void ewk_tiled_backing_store_fix_offsets(Evas_Object* o, Evas_Coord w, Evas_Coord h);
void ewk_tiled_backing_store_zoom_weak_smooth_scale_set(Evas_Object* o, bool smooth_scale);
void ewk_tiled_backing_store_alpha_set(Evas_Object* o, bool has_alpha);
bool ewk_tiled_backing_store_update(Evas_Object* o, const Eina_Rectangle* update);
void ewk_tiled_backing_store_updates_process_pre_set(Evas_Object* o, void* (*cb)(void* data, Evas_Object* o), const void* data);
void ewk_tiled_backing_store_updates_process_post_set(Evas_Object* o, void* (*cb)(void* data, void* pre_data, Evas_Object* o), const void* data);
void ewk_tiled_backing_store_updates_process(Evas_Object* o);
void ewk_tiled_backing_store_updates_clear(Evas_Object* o);
void ewk_tiled_backing_store_contents_resize(Evas_Object* o, Evas_Coord width, Evas_Coord height);
void ewk_tiled_backing_store_disabled_update_set(Evas_Object* o, bool value);
void ewk_tiled_backing_store_flush(Evas_Object* o);
void ewk_tiled_backing_store_enable_scale_set(Evas_Object* o, bool value);

Ewk_Tile_Unused_Cache* ewk_tiled_backing_store_tile_unused_cache_get(const Evas_Object* o);
void ewk_tiled_backing_store_tile_unused_cache_set(Evas_Object* o, Ewk_Tile_Unused_Cache* tuc);

bool ewk_tiled_backing_store_pre_render_region(Evas_Object* o, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h, float zoom);
bool ewk_tiled_backing_store_pre_render_relative_radius(Evas_Object* o, unsigned int n, float zoom);
bool ewk_tiled_backing_store_pre_render_spiral_queue(Evas_Object* o, Eina_Rectangle* view_rect, Eina_Rectangle* render_rect, int max_memory, float zoom);
void ewk_tiled_backing_store_pre_render_cancel(Evas_Object* o);

bool ewk_tiled_backing_store_disable_render(Evas_Object* o);
bool ewk_tiled_backing_store_enable_render(Evas_Object* o);

#endif // ewk_tiled_backing_store_h
