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

#ifndef ewk_tiled_model_private_h
#define ewk_tiled_model_private_h

#include "ewk_tiled_backing_store_private.h"

#include <Evas.h>

/* model */
Ewk_Tile *ewk_tile_new(Evas *evas, Evas_Coord w, Evas_Coord h, float zoom, Evas_Colorspace cspace);
void ewk_tile_free(Ewk_Tile *t);
void ewk_tile_unused_cache_clear(Ewk_Tile_Unused_Cache *tuc);
void ewk_tile_show(Ewk_Tile *t);
void ewk_tile_hide(Ewk_Tile *t);
size_t ewk_tile_memory_size_get(const Ewk_Tile *t);
Eina_Bool ewk_tile_visible_get(Ewk_Tile *t);
void ewk_tile_update_full(Ewk_Tile *t);
void ewk_tile_update_area(Ewk_Tile *t, const Eina_Rectangle *r);
void ewk_tile_updates_process(Ewk_Tile *t, void (*cb)(void *data, Ewk_Tile *t, const Eina_Rectangle *update), const void *data);
void ewk_tile_updates_clear(Ewk_Tile *t);

/* cache of unused tiles */
Ewk_Tile_Unused_Cache *ewk_tile_unused_cache_new(size_t max);
void ewk_tile_unused_cache_lock_area(Ewk_Tile_Unused_Cache *tuc, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h, float zoom);
void ewk_tile_unused_cache_unlock_area(Ewk_Tile_Unused_Cache *tuc);
Ewk_Tile_Unused_Cache *ewk_tile_unused_cache_ref(Ewk_Tile_Unused_Cache *tuc);
void ewk_tile_unused_cache_unref(Ewk_Tile_Unused_Cache *tuc);

void ewk_tile_unused_cache_dirty(Ewk_Tile_Unused_Cache *tuc);

void ewk_tile_unused_cache_freeze(Ewk_Tile_Unused_Cache *tuc);
void ewk_tile_unused_cache_thaw(Ewk_Tile_Unused_Cache *tuc);

Eina_Bool ewk_tile_unused_cache_tile_get(Ewk_Tile_Unused_Cache *tuc, Ewk_Tile *t);
Eina_Bool ewk_tile_unused_cache_tile_put(Ewk_Tile_Unused_Cache *tuc, Ewk_Tile *t, void (* tile_free_cb)(void *data, Ewk_Tile *t), const void *data);

#endif // ewk_tiled_model_private_h
