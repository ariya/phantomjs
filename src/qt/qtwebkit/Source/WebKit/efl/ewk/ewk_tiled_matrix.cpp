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
#include "ewk_tiled_matrix_private.h"
#include "ewk_tiled_model_private.h"
#include <Eina.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

struct Ewk_Tile_Matrix_Entry {
    EINA_INLIST;
    float zoom;
    unsigned long count;
    Eina_Matrixsparse* matrix;
};

struct _Ewk_Tile_Matrix {
    Eina_Matrixsparse* matrix;
    Eina_Inlist* matrices;
    Ewk_Tile_Unused_Cache* tileUnusedCache;
    Evas_Colorspace cspace;
    struct {
        void (*callback)(void* data, Ewk_Tile* tile, const Eina_Rectangle* update);
        void* data;
    } render;
    unsigned int frozen;
    Eina_List* updates;
    struct {
        Evas_Coord width, height;
    } tile;
#ifdef DEBUG_MEM_LEAKS
    struct {
        struct {
            uint64_t allocated, freed;
        } tiles, bytes;
    } stats;
#endif
};

// Default 40 MB size of newly created cache
static const size_t DEFAULT_CACHE_SIZE = 40 * 1024 * 1024;

#ifdef DEBUG_MEM_LEAKS
static uint64_t tiles_leaked = 0;
static uint64_t bytes_leaked = 0;
#endif

static Ewk_Tile_Matrix_Entry* ewk_tile_matrix_entry_get(Ewk_Tile_Matrix* tileMatrix, float zoom)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(tileMatrix, 0);

    Ewk_Tile_Matrix_Entry* it;
    EINA_INLIST_FOREACH(tileMatrix->matrices, it) {
        if (it->zoom == zoom)
            return it;
    }

    return 0;
}

/* called when matrixsparse is resized or freed */
static void _ewk_tile_matrix_cell_free(void* userData, void* cellData)
{
    Ewk_Tile_Matrix* tileMatrix = static_cast<Ewk_Tile_Matrix*>(userData);
    Ewk_Tile* tile = static_cast<Ewk_Tile*>(cellData);

    if (!tile)
        return;

    ewk_tile_unused_cache_freeze(tileMatrix->tileUnusedCache);

    if (tile->updates || tile->stats.full_update)
        tileMatrix->updates = eina_list_remove(tileMatrix->updates, tile);

    if (tile->visible)
        ERR("freeing cell that is visible, leaking tile %p", tile);
    else {
        if (ewk_tile_unused_cache_tile_get(tileMatrix->tileUnusedCache, tile)) {
            DBG("tile cell does not exist anymore, free it %p", tile);
#ifdef DEBUG_MEM_LEAKS
            tileMatrix->stats.bytes.freed += tile->bytes;
            tileMatrix->stats.tiles.freed++;
#endif
            Ewk_Tile_Matrix_Entry* entry = ewk_tile_matrix_entry_get(tileMatrix, tile->zoom);
            if (!entry)
                ERR("can't find matrix for zoom %0.3f", tile->zoom);
            else
                --entry->count;

            ewk_tile_free(tile);
        } else
            ERR("tile %p was not in cache %p? leaking...", tile, tileMatrix->tileUnusedCache);
    }

    ewk_tile_unused_cache_thaw(tileMatrix->tileUnusedCache);
}

/* called when cache of unused tile is flushed */
static void _ewk_tile_matrix_tile_free(void* data, Ewk_Tile* tile)
{
    Ewk_Tile_Matrix* tileMatrix = static_cast<Ewk_Tile_Matrix*>(data);

    Ewk_Tile_Matrix_Entry* entry = ewk_tile_matrix_entry_get(tileMatrix, tile->zoom);
    if (!entry) {
        ERR("removing tile %p that was not in any matrix? Leaking...", tile);
        return;
    }

    Eina_Matrixsparse_Cell* cell;
    if (!eina_matrixsparse_cell_idx_get(entry->matrix, tile->row, tile->column, &cell)) {
        ERR("removing tile %p that was not in the matrix? Leaking...", tile);
        return;
    }

    if (!cell) {
        ERR("removing tile %p that was not in the matrix? Leaking...", tile);
        return;
    }

    if (tile->updates || tile->stats.full_update)
        tileMatrix->updates = eina_list_remove(tileMatrix->updates, tile);

    /* set to null to avoid double free */
    eina_matrixsparse_cell_data_replace(cell, 0, 0);
    eina_matrixsparse_cell_clear(cell);

    if (EINA_UNLIKELY(!!tile->visible)) {
        ERR("cache of unused tiles requesting deletion of used tile %p? "
            "Leaking...", tile);
        return;
    }

#ifdef DEBUG_MEM_LEAKS
    tileMatrix->stats.bytes.freed += tile->bytes;
    tileMatrix->stats.tiles.freed++;
#endif

    --entry->count;
    if (!entry->count && entry->matrix != tileMatrix->matrix) {
        eina_matrixsparse_free(entry->matrix);
        tileMatrix->matrices = eina_inlist_remove(tileMatrix->matrices, EINA_INLIST_GET(entry));
        delete entry;
    }

    ewk_tile_free(tile);
}

/**
 * Creates a new matrix of tiles.
 *
 * The tile matrix is responsible for keeping tiles around and
 * providing fast access to them. One can use it to retrieve new or
 * existing tiles and give them back, allowing them to be
 * freed/replaced by the cache.
 *
 * @param tileUnusedCache cache of unused tiles or @c 0 to create one
 *        automatically.
 * @param columns number of columns in the matrix.
 * @param rows number of rows in the matrix.
 * @param zoomLevel zoom level for the matrix.
 * @param cspace the color space used to create tiles in this matrix.
 * @param render_cb function used to render given tile update.
 * @param render_data context to give back to @a render_cb.
 *
 * @return newly allocated instance on success, @c 0 on failure.
 */
Ewk_Tile_Matrix* ewk_tile_matrix_new(Ewk_Tile_Unused_Cache* tileUnusedCache, unsigned long columns, unsigned long rows, float zoomLevel, Evas_Colorspace colorSpace, void (*renderCallback)(void* data, Ewk_Tile* tile, const Eina_Rectangle* update), const void* renderData)
{
    OwnPtr<Ewk_Tile_Matrix> tileMatrix = adoptPtr(new Ewk_Tile_Matrix);

    tileMatrix->matrices = 0;
    if (!ewk_tile_matrix_zoom_level_set(tileMatrix.get(), zoomLevel))
        ewk_tile_matrix_entry_new(tileMatrix.get(), zoomLevel);
    ewk_tile_matrix_resize(tileMatrix.get(), columns, rows);

    if (tileUnusedCache)
        tileMatrix->tileUnusedCache = ewk_tile_unused_cache_ref(tileUnusedCache);
    else {
        tileMatrix->tileUnusedCache = ewk_tile_unused_cache_new(DEFAULT_CACHE_SIZE);
        if (!tileMatrix->tileUnusedCache) {
            ERR("no cache of unused tile!");
            eina_matrixsparse_free(tileMatrix->matrix);
            return 0;
        }
    }

    tileMatrix->cspace = colorSpace;
    tileMatrix->render.callback = renderCallback;
    tileMatrix->render.data = (void*)renderData;
    tileMatrix->tile.width = defaultTileWidth;
    tileMatrix->tile.height = defaultTileHeigth;
    tileMatrix->frozen = 0;
    tileMatrix->updates = 0;

#ifdef DEBUG_MEM_LEAKS
    tileMatrix->stats.tiles.allocated = 0;
    tileMatrix->stats.tiles.freed = 0;
    tileMatrix->stats.bytes.allocated = 0;
    tileMatrix->stats.bytes.freed = 0;
#endif
    return tileMatrix.leakPtr();
}

/**
 * Find the matrix with the same zoom and set it as current matrix.
 *
 * @param tileMatrix tile matrix to search the matrix in.
 * @param zoom zoom factor to find the same matrix with it in matrices.
 *
 * @return @c true if found, @c false otherwise.
 */
bool ewk_tile_matrix_zoom_level_set(Ewk_Tile_Matrix* tileMatrix, float zoom)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(tileMatrix, false);

    Ewk_Tile_Matrix_Entry* it;
    EINA_INLIST_FOREACH(tileMatrix->matrices, it) {
        if (it->zoom == zoom) {
            tileMatrix->matrices = eina_inlist_promote(tileMatrix->matrices, EINA_INLIST_GET(it));
            tileMatrix->matrix = it->matrix;

            return true;
        }
    }
    return false;
}

void ewk_tile_matrix_entry_new(Ewk_Tile_Matrix* tileMatrix, float zoom)
{
    EINA_SAFETY_ON_NULL_RETURN(tileMatrix);

    OwnPtr<Ewk_Tile_Matrix_Entry> entry = adoptPtr(new Ewk_Tile_Matrix_Entry);
    entry->zoom = zoom;
    entry->count = 0;
    entry->matrix = eina_matrixsparse_new(1, 1, _ewk_tile_matrix_cell_free, tileMatrix);
    if (!entry->matrix) {
        ERR("could not create sparse matrix.");
        return;
    }
    tileMatrix->matrix = entry->matrix;
    tileMatrix->matrices = eina_inlist_prepend(tileMatrix->matrices, EINA_INLIST_GET(entry.leakPtr()));
}

void ewk_tile_matrix_invalidate(Ewk_Tile_Matrix* tileMatrix)
{
    EINA_SAFETY_ON_NULL_RETURN(tileMatrix);

    Eina_Inlist* matrixList = tileMatrix->matrices;
    while (matrixList) {
        Ewk_Tile_Matrix_Entry* it = EINA_INLIST_CONTAINER_GET(matrixList, Ewk_Tile_Matrix_Entry);
        Eina_Inlist* next = matrixList->next;

        if (it->matrix != tileMatrix->matrix) {
            eina_matrixsparse_free(it->matrix);
            tileMatrix->matrices = eina_inlist_remove(tileMatrix->matrices, matrixList);
            delete it;
        }

        matrixList = next;
    }
}

/**
 * Destroys tiles matrix, releasing its resources.
 *
 * The cache instance is unreferenced, possibly freeing it.
 */
void ewk_tile_matrix_free(Ewk_Tile_Matrix* tileMatrix)
{
    EINA_SAFETY_ON_NULL_RETURN(tileMatrix);

    ewk_tile_unused_cache_freeze(tileMatrix->tileUnusedCache);
    ewk_tile_matrix_invalidate(tileMatrix);
    Ewk_Tile_Matrix_Entry* entry = EINA_INLIST_CONTAINER_GET(tileMatrix->matrices, Ewk_Tile_Matrix_Entry);
    eina_matrixsparse_free(entry->matrix);
    tileMatrix->matrices = eina_inlist_remove(tileMatrix->matrices, reinterpret_cast<Eina_Inlist*>(entry));
    tileMatrix->matrices = 0;
    delete entry;

    ewk_tile_unused_cache_thaw(tileMatrix->tileUnusedCache);
    ewk_tile_unused_cache_unref(tileMatrix->tileUnusedCache);

#ifdef DEBUG_MEM_LEAKS
    uint64_t tiles = tileMatrix->stats.tiles.allocated - tileMatrix->stats.tiles.freed;
    uint64_t bytes = tileMatrix->stats.bytes.allocated - tileMatrix->stats.bytes.freed;

    tiles_leaked += tiles;
    bytes_leaked += bytes;

    if (tiles || bytes)
        ERR("tiled matrix leaked: tiles[+%" PRIu64 ",-%" PRIu64 ":%" PRIu64 "] "
            "bytes[+%" PRIu64 ",-%" PRIu64 ":%" PRIu64 "]",
            tileMatrix->stats.tiles.allocated, tileMatrix->stats.tiles.freed, tiles,
            tileMatrix->stats.bytes.allocated, tileMatrix->stats.bytes.freed, bytes);
    else if (tiles_leaked || bytes_leaked)
        WARN("tiled matrix had no leaks: tiles[+%" PRIu64 ",-%" PRIu64 "] "
            "bytes[+%" PRIu64 ",-%" PRIu64 "], but some other leaked "
            "%" PRIu64 " tiles (%" PRIu64 " bytes)",
            tileMatrix->stats.tiles.allocated, tileMatrix->stats.tiles.freed,
            tileMatrix->stats.bytes.allocated, tileMatrix->stats.bytes.freed,
            tiles_leaked, bytes_leaked);
    else
        INFO("tiled matrix had no leaks: tiles[+%" PRIu64 ",-%" PRIu64 "] "
            "bytes[+%" PRIu64 ",-%" PRIu64 "]",
            tileMatrix->stats.tiles.allocated, tileMatrix->stats.tiles.freed,
            tileMatrix->stats.bytes.allocated, tileMatrix->stats.bytes.freed);
#endif

    delete tileMatrix;
}

/**
 * Resize matrix to given number of rows and columns.
 */
void ewk_tile_matrix_resize(Ewk_Tile_Matrix* tileMatrix, unsigned long cols, unsigned long rows)
{
    EINA_SAFETY_ON_NULL_RETURN(tileMatrix);

    eina_matrixsparse_size_set(tileMatrix->matrix, rows, cols);
}

void ewk_tile_matrix_size_get(Ewk_Tile_Matrix* tileMatrix, unsigned long* columns, unsigned long* rows)
{
    EINA_SAFETY_ON_NULL_RETURN(tileMatrix);

    eina_matrixsparse_size_get(tileMatrix->matrix, rows, columns);
}

/**
 * Get the cache of unused tiles in use by given matrix.
 *
 * No reference is taken to the cache.
 */
Ewk_Tile_Unused_Cache* ewk_tile_matrix_unused_cache_get(const Ewk_Tile_Matrix* tileMatrix)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(tileMatrix, 0);

    return tileMatrix->tileUnusedCache;
}

/**
 * Get the exact tile for the given position and zoom.
 *
 * If the tile.widthas unused then it's removed from the cache.
 *
 * After usage, please give it back using
 * ewk_tile_matrix_tile_put(). If you just want to check if it exists,
 * then use ewk_tile_matrix_tile_exact_exists().
 *
 * @param tileMatrix the tile matrix to get tile from.
 * @param column the column number.
 * @param row the row number.
 * @param zoom the exact zoom to use.
 *
 * @return The tile instance or @c 0 if none is found. If the tile
 *         was in the unused cache it will be @b removed (thus
 *         considered used) and one should give it back with
 *         ewk_tile_matrix_tile_put() afterwards.
 *
 * @see ewk_tile_matrix_tile_exact_get()
 */
Ewk_Tile* ewk_tile_matrix_tile_exact_get(Ewk_Tile_Matrix* tileMatrix, unsigned long column, unsigned long row, float zoom)
{
    Ewk_Tile* tile = static_cast<Ewk_Tile*>(eina_matrixsparse_data_idx_get(tileMatrix->matrix, row, column));
    if (!tile)
        return 0;

    if (tile->zoom != zoom)
        return 0;

#ifndef NDEBUG
    if (!tile->visible) {
        if (!ewk_tile_unused_cache_tile_get(tileMatrix->tileUnusedCache, tile))
            WARN("Ewk_Tile was unused but not in cache? bug!");
    }
#endif

    return tile;
}

/**
 * Checks if tile of given zoom exists in matrix.
 *
 * @param tileMatrix the tile matrix to check tile existence.
 * @param column the column number.
 * @param row the row number.
 * @param zoom the exact zoom to query.
 *
 * @return @c true if found, @c false otherwise.
 *
 * @see ewk_tile_matrix_tile_exact_get()
 */
bool ewk_tile_matrix_tile_exact_exists(Ewk_Tile_Matrix* tileMatrix, unsigned long column, unsigned long row, float zoom)
{
    return ewk_tile_matrix_tile_exact_get(tileMatrix, column, row, zoom);
}

/**
 * Create a new tile at given position and zoom level in the matrix.
 *
 * The newly created tile is considered in use and not put into cache
 * of unused tiles. After it is used one should call
 * ewk_tile_matrix_tile_put() to give it back to matrix.
 *
 * @param tileMatrix the tile matrix to create tile on.
 * @param column the column number.
 * @param row the row number.
 * @param zoom the level to create tile, used to determine tile size.
 */
Ewk_Tile* ewk_tile_matrix_tile_new(Ewk_Tile_Matrix* tileMatrix, Evas* canvas, unsigned long column, unsigned long row, float zoom)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(tileMatrix, 0);
    EINA_SAFETY_ON_FALSE_RETURN_VAL(zoom > 0.0, 0);

    Ewk_Tile_Matrix_Entry* entry = ewk_tile_matrix_entry_get(tileMatrix, zoom);
    if (!entry) {
        ERR("could not get matrix at zoom %f for tile", zoom);
        return 0;
    }
    ++entry->count;

    Evas_Coord tileWidth = tileMatrix->tile.width;
    Evas_Coord tileHeight = tileMatrix->tile.height;

    Ewk_Tile* tile = ewk_tile_new(canvas, tileWidth, tileHeight, zoom, tileMatrix->cspace);
    if (!tile) {
        ERR("could not create tile %dx%d at %f, cspace=%d", tileWidth, tileHeight, (double)zoom, tileMatrix->cspace);
        return 0;
    }

    if (!eina_matrixsparse_data_idx_set(tileMatrix->matrix, row, column, tile)) {
        ERR("could not set matrix cell, row/col outside matrix dimensions!");
        ewk_tile_free(tile);
        return 0;
    }

    tile->column = column;
    tile->row = row;
    tile->x = column * tileWidth;
    tile->y = row * tileHeight;
    tile->stats.full_update = true;
    tileMatrix->updates = eina_list_append(tileMatrix->updates, tile);

#ifdef DEBUG_MEM_LEAKS
    tileMatrix->stats.bytes.allocated += tile->bytes;
    tileMatrix->stats.tiles.allocated++;
#endif

    return tile;
}

/**
 * Gives back the tile to the tile matrix.
 *
 * This will report the tile is no longer in use by the one that got
 * it with ewk_tile_matrix_tile_exact_get().
 *
 * Any previous reference to tile should be released
 * (ewk_tile.heightide()) before calling this function, so it will
 * be known if it is not visibile anymore and thus can be put into the
 * unused cache.
 *
 * @param tileMatrix the tile matrix to return tile to.
 * @param tile the tile instance to return, must @b not be @c 0.
 * @param last_used time in which tile.widthas last used.
 *
 * @return #true on success or #false on failure.
 */
bool ewk_tile_matrix_tile_put(Ewk_Tile_Matrix* tileMatrix, Ewk_Tile* tile, double lastUsed)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(tileMatrix, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(tile, false);

    if (tile->visible)
        return true;

    tile->stats.last_used = lastUsed;
    return ewk_tile_unused_cache_tile_put(tileMatrix->tileUnusedCache, tile, _ewk_tile_matrix_tile_free, tileMatrix);
}

void ewk_tile_matrix_tile_updates_clear(Ewk_Tile_Matrix* tileMatrix, Ewk_Tile* tile)
{
    EINA_SAFETY_ON_NULL_RETURN(tileMatrix);

    if (!tile->updates && !tile->stats.full_update)
        return;

    ewk_tile_updates_clear(tile);
    tileMatrix->updates = eina_list_remove(tileMatrix->updates, tile);
}

static bool _ewk_tile_matrix_slicer_setup(Ewk_Tile_Matrix* tileMatrix, const Eina_Rectangle* area, float zoom, Eina_Tile_Grid_Slicer* slicer)
{
    UNUSED_PARAM(zoom);
    if (area->w <= 0 || area->h <= 0) {
        WARN("invalid area region: %d,%d+%dx%d.", area->x, area->y, area->w, area->h);
        return false;
    }

    Evas_Coord x = area->x;
    Evas_Coord y = area->y;
    Evas_Coord width = area->w;
    Evas_Coord height = area->h;

    Evas_Coord tileWidth = tileMatrix->tile.width;
    Evas_Coord tileHeight = tileMatrix->tile.height;

    // cropping area region to fit matrix
    unsigned long rows, cols;
    eina_matrixsparse_size_get(tileMatrix->matrix, &rows, &cols);
    if (x < 0) {
        width += x;
        x = 0;
    }
    if (y < 0) {
        height += y;
        y = 0;
    }

    if (y + height - 1 > static_cast<long>(rows * tileHeight))
        height = rows * tileHeight - y;
    if (x + width - 1 > static_cast<long>(cols * tileWidth))
        width = cols * tileWidth - x;

    return eina_tile_grid_slicer_setup(slicer, x, y, width, height, tileWidth, tileHeight);
}


bool ewk_tile_matrix_update(Ewk_Tile_Matrix* tileMatrix, const Eina_Rectangle* update, float zoom)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(tileMatrix, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(update, false);

    if (update->w < 1 || update->h < 1) {
        DBG("Why we get updates with empty areas? %d,%d+%dx%d at zoom %f", update->x, update->y, update->w, update->h, zoom);
        return true;
    }

    Eina_Tile_Grid_Slicer slicer;
    if (!_ewk_tile_matrix_slicer_setup(tileMatrix, update, zoom, &slicer)) {
        ERR("Could not setup slicer for update %d,%d+%dx%d at zoom %f", update->x, update->y, update->w, update->h, zoom);
        return false;
    }

    const Eina_Tile_Grid_Info* info;
    while (eina_tile_grid_slicer_next(&slicer, &info)) {
        Ewk_Tile* tile = static_cast<Ewk_Tile*>(eina_matrixsparse_data_idx_get(tileMatrix->matrix, info->row, info->col));
        if (!tile)
            continue;

        if (!tile->updates && !tile->stats.full_update)
            tileMatrix->updates = eina_list_append(tileMatrix->updates, tile);
        if (info->full)
            ewk_tile_update_full(tile);
        else
            ewk_tile_update_area(tile, &info->rect);
    }


    return true;
}

void ewk_tile_matrix_updates_process(Ewk_Tile_Matrix* tileMatrix)
{
    EINA_SAFETY_ON_NULL_RETURN(tileMatrix);

    // process updates, unflag tiles
    Eina_List* list, *listNext;
    void* item;
    EINA_LIST_FOREACH_SAFE(tileMatrix->updates, list, listNext, item) {
        Ewk_Tile* tile = static_cast<Ewk_Tile*>(item);
        ewk_tile_updates_process(tile, tileMatrix->render.callback, tileMatrix->render.data);
        if (tile->visible) {
            ewk_tile_updates_clear(tile);
            tileMatrix->updates = eina_list_remove_list(tileMatrix->updates, list);
        }
    }
}

void ewk_tile_matrix_updates_clear(Ewk_Tile_Matrix* tileMatrix)
{
    EINA_SAFETY_ON_NULL_RETURN(tileMatrix);

    void* item;
    EINA_LIST_FREE(tileMatrix->updates, item)
        ewk_tile_updates_clear(static_cast<Ewk_Tile*>(item));
    tileMatrix->updates = 0;
}

#ifdef DEBUG_MEM_LEAKS
// remove me later!
void ewk_tile_matrix_dbg(const Ewk_Tile_Matrix* tileMatrix)
{
    Eina_Iterator* it = eina_matrixsparse_iterator_complete_new(tileMatrix->matrix);
    Eina_Matrixsparse_Cell* cell;
    bool wasPreviousEmpty = false;

    printf("Ewk_Tile Matrix: tiles[+%" PRIu64 ",-%" PRIu64 ":%" PRIu64 "] "
           "bytes[+%" PRIu64 ",-%" PRIu64 ":%" PRIu64 "]\n",
           tileMatrix->stats.tiles.allocated, tileMatrix->stats.tiles.freed,
           tileMatrix->stats.tiles.allocated - tileMatrix->stats.tiles.freed,
           tileMatrix->stats.bytes.allocated, tileMatrix->stats.bytes.freed,
           tileMatrix->stats.bytes.allocated - tileMatrix->stats.bytes.freed);

    EINA_ITERATOR_FOREACH(it, cell) {
        unsigned long row, column;
        eina_matrixsparse_cell_position_get(cell, &row, &column);

        Ewk_Tile* tile = static_cast<Ewk_Tile*>(eina_matrixsparse_cell_data_get(cell));
        if (!tile) {
            if (!wasPreviousEmpty) {
                wasPreviousEmpty = true;
                printf("Empty:");
            }
            printf(" [%lu,%lu]", column, row);
        } else {
            if (wasPreviousEmpty) {
                wasPreviousEmpty = false;
                printf("\n");
            }
            printf("%3lu,%3lu %10p:", column, row, tile);
            printf(" [%3lu,%3lu + %dx%d @ %0.3f]%c", tile->column, tile->row, tile->width, tile->height, tile->zoom, tile->visible ? '*' : ' ');
            printf("\n");
        }
    }
    if (wasPreviousEmpty)
        printf("\n");
    eina_iterator_free(it);

    ewk_tile_unused_cache_dbg(tileMatrix->tileUnusedCache);
}
#endif

/**
 * Freeze matrix to not do maintenance tasks.
 *
 * Maintenance tasks optimize usage, but maybe we know we should hold
 * on them until we do the last operation, in this case we freeze
 * while operating and then thaw when we're done.
 *
 * @see ewk_tile_matrix_thaw()
 */
void ewk_tile_matrix_freeze(Ewk_Tile_Matrix* tileMatrix)
{
    EINA_SAFETY_ON_NULL_RETURN(tileMatrix);

    if (!tileMatrix->frozen)
        ewk_tile_unused_cache_freeze(tileMatrix->tileUnusedCache);
    ++tileMatrix->frozen;
}

/**
 * Unfreezes maintenance tasks.
 *
 * If this is the last counterpart of freeze, then maintenance tasks
 * will run immediately.
 */
void ewk_tile_matrix_thaw(Ewk_Tile_Matrix* tileMatrix)
{
    EINA_SAFETY_ON_NULL_RETURN(tileMatrix);

    if (!tileMatrix->frozen) {
        ERR("thawing more than freezing!");
        return;
    }

    --tileMatrix->frozen;
    if (!tileMatrix->frozen)
        ewk_tile_unused_cache_thaw(tileMatrix->tileUnusedCache);
}
