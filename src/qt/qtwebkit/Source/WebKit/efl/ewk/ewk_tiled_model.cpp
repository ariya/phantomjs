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

#define __STDC_FORMAT_MACROS
#include "config.h"

#include "ewk_private.h"
#include "ewk_tiled_backing_store_private.h"
#include "ewk_tiled_model_private.h"
#include <Ecore_Evas.h>
#include <Eina.h>
#include <eina_safety_checks.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#ifdef TILE_STATS_ACCOUNT_RENDER_TIME
#include <sys/time.h>
#endif

#define IDX(col, row, rowspan) (col + (rowidth * rowspan))

#ifdef DEBUG_MEM_LEAKS
static uint64_t tiles_allocated = 0;
static uint64_t tiles_freed = 0;
static uint64_t bytes_allocated = 0;
static uint64_t bytes_freed = 0;

struct tile_account {
    Evas_Coord size;
    struct {
        uint64_t allocated;
        uint64_t freed;
    } tiles, bytes;
};

static size_t accounting_len = 0;
static struct tile_account* accounting = 0;

static inline struct tile_account* _ewk_tile_account_get(const Ewk_Tile* tile)
{
    struct tile_account* acc;

    for (size_t i = 0; i < accounting_len; i++) {
        if (accounting[i].size == tile->width)
            return accounting + i;
    }

    accounting = static_cast<struct tile_account*>(realloc(accounting, sizeof(struct tile_account) * (accounting_len + 1)));

    acc = accounting + accounting_len;
    acc->size = tile->width;
    acc->tiles.allocated = 0;
    acc->tiles.freed = 0;
    acc->bytes.allocated = 0;
    acc->bytes.freed = 0;

    accounting_len++;

    return acc;
}

static inline void _ewk_tile_account_allocated(const Ewk_Tile* tile)
{
    struct tile_account* acc = _ewk_tile_account_get(tile);
    if (!acc)
        return;
    acc->bytes.allocated += tile->bytes;
    acc->tiles.allocated++;

    bytes_allocated += tile->bytes;
    tiles_allocated++;
}

static inline void _ewk_tile_account_freed(const Ewk_Tile* tile)
{
    struct tile_account* acc = _ewk_tile_account_get(tile);
    if (!acc)
        return;

    acc->bytes.freed += tile->bytes;
    acc->tiles.freed++;

    bytes_freed += tile->bytes;
    tiles_freed++;
}

void ewk_tile_accounting_dbg()
{
    struct tile_account* acc;
    struct tile_account* acc_end;

    printf("TILE BALANCE: tiles[+%" PRIu64 ",-%" PRIu64 ":%" PRIu64 "] "
           "bytes[+%" PRIu64 ",-%" PRIu64 ":%" PRIu64 "]\n",
           tiles_allocated, tiles_freed, tiles_allocated - tiles_freed,
           bytes_allocated, bytes_freed, bytes_allocated - bytes_freed);

    if (!accounting_len)
        return;

    acc = accounting;
    acc_end = acc + accounting_len;
    printf("BEGIN: TILE BALANCE DETAILS (TO THIS MOMENT!):\n");
    for (; acc < acc_end; acc++) {
        uint64_t tiles, bytes;

        tiles = acc->tiles.allocated - acc->tiles.freed;
        bytes = acc->bytes.allocated - acc->bytes.freed;

        printf("   %4d: tiles[+%4" PRIu64 ",-%4" PRIu64 ":%4" PRIu64 "] "
               "bytes[+%8" PRIu64 ",-%8" PRIu64 ":%8" PRIu64 "]%s\n",
               acc->size,
               acc->tiles.allocated, acc->tiles.freed, tiles,
               acc->bytes.allocated, acc->bytes.freed, bytes,
               (bytes || tiles) ? " POSSIBLE LEAK" : "");
    }
    printf("END: TILE BALANCE DETAILS (TO THIS MOMENT!):\n");
}
#else

static inline void _ewk_tile_account_allocated(const Ewk_Tile*) { }
static inline void _ewk_tile_account_freed(const Ewk_Tile*) { }

void ewk_tile_accounting_dbg()
{
    printf("compile webkit with DEBUG_MEM_LEAKS defined!\n");
}
#endif

/**
 * Create a new tile of given size, zoom level and colorspace.
 *
 * After created these properties are immutable as they're the basic
 * characteristic of the tile and any change will lead to invalid
 * memory access.
 *
 * Other members are of free-access and no getters/setters are
 * provided in orderr to avoid expensive operations on those, however
 * some are better manipulated with provided functions, such as
 * ewk_tile_show() and ewk_tile_hide() to change
 * @c visible or ewk_tile_update_full(), ewk_tile_update_area(),
 * ewk_tile_updates_clear() to change @c stats.misses,
 * @c stats.full_update and @c updates.
 */
Ewk_Tile* ewk_tile_new(Evas* evas, Evas_Coord width, Evas_Coord height, float zoom, Evas_Colorspace colorSpace)
{
    Evas_Coord* evasCoord;
    Evas_Colorspace* evasColorSpace;
    float* tileZoom;
    size_t* tileSize;
    Ewk_Tile* tile;
    unsigned int area;
    size_t bytes;
    Ecore_Evas* ecoreEvas;
    const char* engine;

    area = width * height;

    if (colorSpace == EVAS_COLORSPACE_ARGB8888)
        bytes = area * 4;
    else if (colorSpace == EVAS_COLORSPACE_RGB565_A5P)
        bytes = area * 2;
    else {
        ERR("unknown color space: %d", colorSpace);
        return 0;
    }

    DBG("size: %dx%d (%d), zoom: %f, cspace=%d", width, height, area, (double)zoom, colorSpace);

    tile = static_cast<Ewk_Tile*>(malloc(sizeof(Ewk_Tile)));
    if (!tile)
        return 0;

    tile->image = evas_object_image_add(evas);

    ecoreEvas = ecore_evas_ecore_evas_get(evas);
    engine = ecore_evas_engine_name_get(ecoreEvas);
    if (engine && !strcmp(engine, "opengl_x11"))
        evas_object_image_content_hint_set(tile->image, EVAS_IMAGE_CONTENT_HINT_DYNAMIC);

    tile->visible = 0;
    tile->updates = 0;

    memset(&tile->stats, 0, sizeof(Ewk_Tile_Stats));
    tile->stats.area = area;

    /* ugly, but let's avoid at all costs having users to modify those */
    evasCoord = (Evas_Coord*)&tile->width;
    *evasCoord = width;

    evasCoord = (Evas_Coord*)&tile->height;
    *evasCoord = height;

    evasColorSpace = (Evas_Colorspace*)&tile->cspace;
    *evasColorSpace = colorSpace;

    tileZoom = (float*)&tile->zoom;
    *tileZoom = zoom;

    tileSize = (size_t*)&tile->bytes;
    *tileSize = bytes;

    evas_object_image_size_set(tile->image, tile->width, tile->height);
    evas_object_image_colorspace_set(tile->image, tile->cspace);
    _ewk_tile_account_allocated(tile);

    return tile;
}

/**
 * Free tile memory.
 */
void ewk_tile_free(Ewk_Tile* tile)
{
    _ewk_tile_account_freed(tile);

    if (tile->updates)
        eina_tiler_free(tile->updates);

    evas_object_del(tile->image);
    free(tile);
}

/**
 * @internal
 * Returns memory size used by given tile
 *
 * @param t tile to size check
 * @return Returns used memory or zero if object is NULL.
 */
size_t ewk_tile_memory_size_get(const Ewk_Tile* tile)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(tile, 0);
    return sizeof(Ewk_Tile) + tile->bytes;
}

/**
 * Make the tile visible, incrementing its counter.
 */
void ewk_tile_show(Ewk_Tile* tile)
{
    tile->visible++;
    evas_object_show(tile->image);
}

/**
 * Decrement the visibility counter, making it invisible if necessary.
 */
void ewk_tile_hide(Ewk_Tile* tile)
{
    tile->visible--;
    if (!tile->visible)
        evas_object_hide(tile->image);
}

/**
 * Returns true if the tile is visible, false otherwise.
 */
Eina_Bool ewk_tile_visible_get(Ewk_Tile* tile)
{
    return !!tile->visible;
}

/**
 * Mark whole tile as dirty and requiring update.
 */
void ewk_tile_update_full(Ewk_Tile* tile)
{
    /* TODO: list of tiles pending updates? */
    tile->stats.misses++;

    if (!tile->stats.full_update) {
        tile->stats.full_update = true;
        if (tile->updates) {
            eina_tiler_free(tile->updates);
            tile->updates = 0;
        }
    }
}

/**
 * Mark the specific subarea as dirty and requiring update.
 */
void ewk_tile_update_area(Ewk_Tile* tile, const Eina_Rectangle* rect)
{
    /* TODO: list of tiles pending updates? */
    tile->stats.misses++;

    if (tile->stats.full_update)
        return;

    if (!rect->x && !rect->y && rect->w == tile->width && rect->h == tile->height) {
        tile->stats.full_update = true;
        if (tile->updates) {
            eina_tiler_free(tile->updates);
            tile->updates = 0;
        }
        return;
    }

    if (!tile->updates) {
        tile->updates = eina_tiler_new(tile->width, tile->height);
        if (!tile->updates) {
            CRITICAL("could not create eina_tiler %dx%d.", tile->width, tile->height);
            return;
        }
    }

    eina_tiler_rect_add(tile->updates, rect);
}

/**
 * For each updated region, call the given function.
 *
 * This will not change the tile statistics or clear the processed
 * updates, use ewk_tile_updates_clear() for that.
 */
void ewk_tile_updates_process(Ewk_Tile* tile, void (*callback)(void* data, Ewk_Tile* tile, const Eina_Rectangle* update), const void* data)
{
    if (tile->stats.full_update) {
        Eina_Rectangle rect;
        rect.x = 0;
        rect.y = 0;
        rect.w = tile->width;
        rect.h = tile->height;
#ifdef TILE_STATS_ACCOUNT_RENDER_TIME
        struct timeval timev;
        double renderStartTime;
        gettimeofday(&timev, 0);
        renderStartTime = (double)timev.tv_sec +
                       (((double)timev.tv_usec) / 1000000);
#endif
        callback((void*)data, tile, &rect);
#ifdef TILE_STATS_ACCOUNT_RENDER_TIME
        gettimeofday(&timev, 0);
        tile->stats.render_time = (double)timev.tv_sec +
                               (((double)timev.tv_usec) / 1000000) - renderStartTime;
#endif
    } else if (tile->updates) {
        Eina_Iterator* itr = eina_tiler_iterator_new(tile->updates);
        Eina_Rectangle* rect;
        if (!itr) {
            CRITICAL("could not create tiler iterator!");
            return;
        }
        EINA_ITERATOR_FOREACH(itr, rect)
            callback((void*)data, tile, rect);
        eina_iterator_free(itr);
    }
}

/**
 * Clear all updates in region, if any.
 *
 * This will change the tile statistics, specially zero stat.misses
 * and unset stats.full_update. If tile->updates existed, then it will be
 * destroyed.
 *
 * This function is usually called after ewk_tile_updates_process() is
 * called.
 */
void ewk_tile_updates_clear(Ewk_Tile* tile)
{
    /* TODO: remove from list of pending updates? */
    tile->stats.misses = 0;

    if (tile->stats.full_update)
        tile->stats.full_update = 0;
    else if (tile->updates) {
        eina_tiler_free(tile->updates);
        tile->updates = 0;
    }
}

typedef struct _Ewk_Tile_Unused_Cache_Entry Ewk_Tile_Unused_Cache_Entry;
struct _Ewk_Tile_Unused_Cache_Entry {
    Ewk_Tile* tile;
    int weight;
    struct {
        void (*callback)(void* data, Ewk_Tile* tile);
        void* data;
    } tile_free;
};

struct _Ewk_Tile_Unused_Cache {
    struct {
        Eina_List* list;
        size_t count;
        size_t allocated;
    } entries;
    struct {
        size_t max;  /**< watermark (in bytes) to start freeing tiles */
        size_t used; /**< in bytes, maybe more than max. */
    } memory;
    struct {
        Evas_Coord x, y, width, height;
        float zoom;
        Eina_Bool locked;
    } locked;
    int references;
    unsigned int frozen;
};

static const size_t TILE_UNUSED_CACHE_ALLOCATE_INITIAL = 128;
static const size_t TILE_UNUSED_CACHE_ALLOCATE_STEP = 16;
static const size_t TILE_UNUSED_CACHE_MAX_FREE = 32;

/**
 * Cache of unused tiles (those that are not visible).
 *
 * The cache of unused tiles.
 *
 * @param max cache size in bytes.
 *
 * @return newly allocated cache of unused tiles, use
 *         ewk_tile_unused_cache_free() to release resources. If not
 *         possible to allocate memory, @c 0 is returned.
 */
Ewk_Tile_Unused_Cache* ewk_tile_unused_cache_new(size_t max)
{
    Ewk_Tile_Unused_Cache* tileUnusedCache;

    tileUnusedCache = new Ewk_Tile_Unused_Cache;
    memset(tileUnusedCache, 0, sizeof(Ewk_Tile_Unused_Cache));

    DBG("tileUnusedCache=%p", tileUnusedCache);
    tileUnusedCache->memory.max = max;
    tileUnusedCache->references = 1;
    return tileUnusedCache;
}

void ewk_tile_unused_cache_lock_area(Ewk_Tile_Unused_Cache* tileUnusedCache, Evas_Coord x, Evas_Coord y, Evas_Coord width, Evas_Coord height, float zoom)
{
    EINA_SAFETY_ON_NULL_RETURN(tileUnusedCache);

    tileUnusedCache->locked.locked = true;
    tileUnusedCache->locked.x = x;
    tileUnusedCache->locked.y = y;
    tileUnusedCache->locked.width = width;
    tileUnusedCache->locked.height = height;
    tileUnusedCache->locked.zoom = zoom;
}

void ewk_tile_unused_cache_unlock_area(Ewk_Tile_Unused_Cache* tileUnusedCache)
{
    EINA_SAFETY_ON_NULL_RETURN(tileUnusedCache);

    tileUnusedCache->locked.locked = false;
}

/**
 * Free cache of unused tiles.
 *
 * This function should be only called by ewk_tile_unused_cache_unref
 * function. Calling this function without considering reference counting
 * may lead to unknown results.
 *
 * Those tiles that are still visible will remain live. The unused
 * tiles will be freed.
 *
 * @see ewk_tile_unused_cache_unref()
 */
static void _ewk_tile_unused_cache_free(Ewk_Tile_Unused_Cache* tileUnusedCache)
{
    EINA_SAFETY_ON_NULL_RETURN(tileUnusedCache);

    DBG("tileUnusedCache=%p, "
        "entries=(count:%zd, allocated:%zd), "
        "memory=(max:%zd, used:%zd)",
        tileUnusedCache, tileUnusedCache->entries.count, tileUnusedCache->entries.allocated,
        tileUnusedCache->memory.max, tileUnusedCache->memory.used);

    ewk_tile_unused_cache_clear(tileUnusedCache);
    delete tileUnusedCache;
}

/**
 * Clear cache of unused tiles.
 *
 * Any tiles that are in the cache are freed. The only tiles that are
 * kept are those that aren't in the cache (i.e. that are visible).
 */
void ewk_tile_unused_cache_clear(Ewk_Tile_Unused_Cache* tileUnusedCache)
{
    EINA_SAFETY_ON_NULL_RETURN(tileUnusedCache);

    if (!tileUnusedCache->entries.count)
        return;

    void* item;
    EINA_LIST_FREE(tileUnusedCache->entries.list, item) {
        Ewk_Tile_Unused_Cache_Entry* itr = static_cast<Ewk_Tile_Unused_Cache_Entry*>(item);
        itr->tile_free.callback(itr->tile_free.data, itr->tile);
        delete itr;
    }

    tileUnusedCache->memory.used = 0;
    tileUnusedCache->entries.count = 0;
}

/**
 * Hold reference to cache.
 *
 * @return same pointer as taken.
 *
 * @see ewk_tile_unused_cache_unref()
 */
Ewk_Tile_Unused_Cache* ewk_tile_unused_cache_ref(Ewk_Tile_Unused_Cache* tileUnusedCache)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(tileUnusedCache, 0);
    tileUnusedCache->references++;
    return tileUnusedCache;
}

/**
 * Release cache reference, freeing it if it drops to zero.
 *
 * @see ewk_tile_unused_cache_ref()
 * @see ewk_tile_unused_cache_free()
 */
void ewk_tile_unused_cache_unref(Ewk_Tile_Unused_Cache* tileUnusedCache)
{
    EINA_SAFETY_ON_NULL_RETURN(tileUnusedCache);
    tileUnusedCache->references--;
    if (!tileUnusedCache->references)
        _ewk_tile_unused_cache_free(tileUnusedCache);
}

void ewk_tile_unused_cache_max_set(Ewk_Tile_Unused_Cache* tileUnusedCache, size_t max)
{
    EINA_SAFETY_ON_NULL_RETURN(tileUnusedCache);
    size_t oldMax = tileUnusedCache->memory.max;
    tileUnusedCache->memory.max = max;
    /* Cache flush when new max is lower then old one */
    if (oldMax > max)
        ewk_tile_unused_cache_auto_flush(tileUnusedCache);
}

size_t ewk_tile_unused_cache_max_get(const Ewk_Tile_Unused_Cache* tileUnusedCache)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(tileUnusedCache, 0);
    return tileUnusedCache->memory.max;
}

size_t ewk_tile_unused_cache_used_get(const Ewk_Tile_Unused_Cache* tileUnusedCache)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(tileUnusedCache, 0);
    return tileUnusedCache->memory.used;
}

size_t ewk_tile_unused_cache_flush(Ewk_Tile_Unused_Cache* tileUnusedCache, size_t bytes)
{
    Eina_List* list, * listNext;
    EINA_SAFETY_ON_NULL_RETURN_VAL(tileUnusedCache, 0);
    size_t done;
    unsigned int count;

    if (!tileUnusedCache->entries.count)
        return 0;
    if (bytes < 1)
        return 0;

    /*
     * NOTE: the cache is a FIFO queue currently.
     * Don't need to sort any more.
     */

    void* item;
    done = 0;
    count = 0;
    EINA_LIST_FOREACH_SAFE(tileUnusedCache->entries.list, list, listNext, item) {
        Ewk_Tile_Unused_Cache_Entry* itr = static_cast<Ewk_Tile_Unused_Cache_Entry*>(item);
        Ewk_Tile* tile = itr->tile;
        if (done > bytes)
            break;
        if (tileUnusedCache->locked.locked
            && tile->x + tile->width > tileUnusedCache->locked.x
            && tile->y + tile->height > tileUnusedCache->locked.y
            && tile->x < tileUnusedCache->locked.x + tileUnusedCache->locked.width
            && tile->y < tileUnusedCache->locked.y + tileUnusedCache->locked.height
            && tile->zoom == tileUnusedCache->locked.zoom) {
            continue;
        }
        done += ewk_tile_memory_size_get(itr->tile);
        itr->tile_free.callback(itr->tile_free.data, itr->tile);
        tileUnusedCache->entries.list = eina_list_remove_list(tileUnusedCache->entries.list, list);
        delete itr;
        count++;
    }

    tileUnusedCache->memory.used -= done;
    tileUnusedCache->entries.count -= count;

    return done;
}

void ewk_tile_unused_cache_auto_flush(Ewk_Tile_Unused_Cache* tileUnusedCache)
{
    EINA_SAFETY_ON_NULL_RETURN(tileUnusedCache);
    if (tileUnusedCache->memory.used <= tileUnusedCache->memory.max)
        return;
    ewk_tile_unused_cache_flush(tileUnusedCache, tileUnusedCache->memory.used - tileUnusedCache->memory.max);
    if (tileUnusedCache->memory.used > tileUnusedCache->memory.max)
        CRITICAL("Cache still using too much memory: %zd KB; max: %zd KB",
                 tileUnusedCache->memory.used, tileUnusedCache->memory.max);
}

/**
 * Freeze cache to not do maintenance tasks.
 *
 * Maintenance tasks optimize cache usage, but maybe we know we should
 * hold on them until we do the last operation, in this case we freeze
 * while operating and then thaw when we're done.
 *
 * @see ewk_tile_unused_cache_thaw()
 */
void ewk_tile_unused_cache_freeze(Ewk_Tile_Unused_Cache* tileUnusedCache)
{
    tileUnusedCache->frozen++;
}

/**
 * Unfreezes maintenance tasks.
 *
 * If this is the last counterpart of freeze, then maintenance tasks
 * will run immediately.
 */
void ewk_tile_unused_cache_thaw(Ewk_Tile_Unused_Cache* tileUnusedCache)
{
    if (!tileUnusedCache->frozen) {
        ERR("thawing more than freezing!");
        return;
    }

    tileUnusedCache->frozen--;
}

/**
 * Get tile from cache of unused tiles, removing it from the cache.
 *
 * If the tile is used, then it's not in cache of unused tiles, so it
 * is removed from the cache and may be given back with
 * ewk_tile_unused_cache_tile_put().
 *
 * @param tileUnusedCache cache of unused tiles
 * @param tile the tile to be removed from Ewk_Tile_Unused_Cache.
 *
 * @return #true on success, #false otherwise.
 */
Eina_Bool ewk_tile_unused_cache_tile_get(Ewk_Tile_Unused_Cache* tileUnusedCache, Ewk_Tile* tile)
{
    Eina_List* iterateEntry;
    void* item;

    EINA_LIST_FOREACH(tileUnusedCache->entries.list, iterateEntry, item) {
        Ewk_Tile_Unused_Cache_Entry* entry = static_cast<Ewk_Tile_Unused_Cache_Entry*>(item);
        if (entry->tile == tile) {
            tileUnusedCache->entries.count--;
            tileUnusedCache->memory.used -= ewk_tile_memory_size_get(tile);
            tileUnusedCache->entries.list = eina_list_remove_list(tileUnusedCache->entries.list, iterateEntry);
            delete entry;

            return true;
        }
    }

    ERR("tile %p not found in cache %p", tile, tileUnusedCache);
    return false;
}

/**
 * Put tile into cache of unused tiles, adding it to the cache.
 *
 * This should be called when @c tile->visible is @c 0 and no objects are
 * using the tile anymore, making it available to be expired and have
 * its memory replaced.
 *
 * Note that tiles are not automatically deleted if cache is full,
 * instead the cache will have more bytes used than maximum and one
 * can call ewk_tile_unused_cache_auto_flush() to free them. This is done
 * because usually we want a lazy operation for better performance.
 *
 * @param tileUnusedCache cache of unused tiles
 * @param tile tile to be added to cache.
 * @param tileFreeCallback function used to free tiles.
 * @param data context to give back to @a tile_free_cb as first argument.
 *
 * @return #true on success, #false otherwise. If @c tile->visible
 *         is not #false, then it will return #false.
 *
 * @see ewk_tile_unused_cache_auto_flush()
 */
Eina_Bool ewk_tile_unused_cache_tile_put(Ewk_Tile_Unused_Cache* tileUnusedCache, Ewk_Tile* tile, void (* tileFreeCallback)(void* data, Ewk_Tile* tile), const void* data)
{
    Ewk_Tile_Unused_Cache_Entry* unusedCacheEntry;

    if (tile->visible) {
        ERR("tile=%p is not unused (visible=%d)", tile, tile->visible);
        return false;
    }

    unusedCacheEntry = new Ewk_Tile_Unused_Cache_Entry;

    tileUnusedCache->entries.list = eina_list_append(tileUnusedCache->entries.list, unusedCacheEntry);
    if (eina_error_get()) {
        ERR("List allocation failed");
        return false;
    }

    unusedCacheEntry->tile = tile;
    unusedCacheEntry->weight = 0; /* calculated just before sort */
    unusedCacheEntry->tile_free.callback = tileFreeCallback;
    unusedCacheEntry->tile_free.data = (void*)data;

    tileUnusedCache->entries.count++;
    tileUnusedCache->memory.used += ewk_tile_memory_size_get(tile);

    return true;
}

void ewk_tile_unused_cache_dbg(const Ewk_Tile_Unused_Cache* tileUnusedCache)
{
    void* item;
    Eina_List* list;
    int count = 0;
    printf("Cache of unused tiles: entries: %zu/%zu, memory: %zu/%zu\n",
           tileUnusedCache->entries.count, tileUnusedCache->entries.allocated,
           tileUnusedCache->memory.used, tileUnusedCache->memory.max);

    EINA_LIST_FOREACH(tileUnusedCache->entries.list, list, item) {
        const Ewk_Tile* tile = static_cast<Ewk_Tile_Unused_Cache_Entry*>(item)->tile;
        printf(" [%3lu,%3lu + %dx%d @ %0.3f]%c",
               tile->column, tile->row, tile->width, tile->height, tile->zoom,
               tile->visible ? '*' : ' ');

        if (!(count % 4))
            printf("\n");
    }

    printf("\n");
}
