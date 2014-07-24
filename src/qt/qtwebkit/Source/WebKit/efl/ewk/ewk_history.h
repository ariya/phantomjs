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

#ifndef ewk_history_h
#define ewk_history_h

#include <Eina.h>
#include <Evas.h>
#include <cairo.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file ewk_history.h
 * @brief The history (back-forward list) associated with a given ewk_view.
 *
 * Changing the history affects immediately the view, changing the
 * current uri, for example.
 *
 * When ewk_view is navigated or uris are set, history automatically
 * updates. That's why no direct access to history structure is
 * allowed.
 */
typedef struct _Ewk_History         Ewk_History;

/**
 * Represents one item from Ewk_History.
 */
typedef struct _Ewk_History_Item    Ewk_History_Item;



/**
 * Clear the current history, if there is any.
 *
 * @param history which history instance to modify.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool         ewk_history_clear(Ewk_History *history);

/**
 * Go forward in history one item, if possible.
 *
 * @param history which history instance to modify.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool         ewk_history_forward(Ewk_History *history);

/**
 * Go back in history one item, if possible.
 *
 * @param history which history instance to modify.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool         ewk_history_back(Ewk_History *history);

/**
 * Adds the given item to history.
 *
 * Memory handling: This will not modify or even take references to
 * given item (Ewk_History_Item), so you should still handle it with
 * ewk_history_item_free().
 *
 * @param history which history instance to modify.
 * @param item reference to add to history. Unmodified. Must @b not be @c NULL.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool         ewk_history_history_item_add(Ewk_History *history, const Ewk_History_Item *item);

/**
 * Sets the given item as current in history (go to item).
 *
 * Memory handling: This will not modify or even take references to
 * given item (Ewk_History_Item), so you should still handle it with
 * ewk_history_item_free().
 *
 * @param history which history instance to modify.
 * @param item reference to go to history. Unmodified. Must @b not be @c NULL.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool         ewk_history_history_item_set(Ewk_History *history,  const Ewk_History_Item *item);

/**
 * Get the first item from back list, if any.
 *
 * @param history which history instance to query.
 *
 * @return the @b newly allocated item instance. This memory must be
 *         released with ewk_history_item_free() after use.
 */
EAPI Ewk_History_Item *ewk_history_history_item_back_get(const Ewk_History *history);

/**
 * Get the current item in history, if any.
 *
 * @param history which history instance to query.
 *
 * @return the @b newly allocated item instance or @c NULL on error. This memory
 *         must be released with ewk_history_item_free() after use.
 */
EAPI Ewk_History_Item *ewk_history_history_item_current_get(const Ewk_History *history);

/**
 * Get the first item from forward list, if any.
 *
 * @param history which history instance to query.
 *
 * @return the @b newly allocated item instance. This memory must be
 *         released with ewk_history_item_free() after use.
 */
EAPI Ewk_History_Item *ewk_history_history_item_forward_get(const Ewk_History *history);

/**
 * Get item at given position, if any at that index.
 *
 * @param history which history instance to query.
 * @param index position of item to get.
 *
 * @return the @b newly allocated item instance. This memory must be
 *         released with ewk_history_item_free() after use.
 */
EAPI Ewk_History_Item *ewk_history_history_item_nth_get(const Ewk_History *history, int index);

/**
 * Queries if given item is in history.
 *
 * Memory handling: This will not modify or even take references to
 * given item (Ewk_History_Item), so you should still handle it with
 * ewk_history_item_free().
 *
 * @param history which history instance to modify.
 * @param item reference to check in history. Must @b not be @c NULL.
 *
 * @return @c EINA_TRUE if in history, @c EINA_FALSE if not or failure.
 */
EAPI Eina_Bool         ewk_history_history_item_contains(const Ewk_History *history, const Ewk_History_Item *item);

/**
 * Get the whole forward list.
 *
 * @param history which history instance to query.
 *
 * @return a newly allocated list of @b newly allocated item
 *         instance. This memory of each item must be released with
 *         ewk_history_item_free() after use. use
 *         ewk_history_item_list_free() for convenience.
 *
 * @see ewk_history_item_list_free()
 * @see ewk_history_forward_list_get_with_limit()
 */
EAPI Eina_List        *ewk_history_forward_list_get(const Ewk_History *history);

/**
 * Get the forward list within the given limit.
 *
 * @param history which history instance to query.
 * @param limit the maximum number of items to return.
 *
 * @return a newly allocated list of @b newly allocated item
 *         instance. This memory of each item must be released with
 *         ewk_history_item_free() after use. use
 *         ewk_history_item_list_free() for convenience.
 *
 * @see ewk_history_item_list_free()
 * @see ewk_history_forward_list_length()
 * @see ewk_history_forward_list_get()
 */
EAPI Eina_List        *ewk_history_forward_list_get_with_limit(const Ewk_History *history, int limit);

/**
 * Get the whole size of forward list.
 *
 * @param history which history instance to query.
 *
 * @return number of elements in whole list.
 *
 * @see ewk_history_forward_list_get_with_limit()
 */
EAPI int               ewk_history_forward_list_length(const Ewk_History *history);

/**
 * Get the whole back list.
 *
 * @param history which history instance to query.
 *
 * @return a newly allocated list of @b newly allocated item
 *         instance. This memory of each item must be released with
 *         ewk_history_item_free() after use. use
 *         ewk_history_item_list_free() for convenience.
 *
 * @see ewk_history_item_list_free()
 * @see ewk_history_back_list_get_with_limit()
 */
EAPI Eina_List        *ewk_history_back_list_get(const Ewk_History *history);

/**
 * Get the back list within the given limit.
 *
 * @param history which history instance to query.
 * @param limit the maximum number of items to return.
 *
 * @return a newly allocated list of @b newly allocated item
 *         instance. This memory of each item must be released with
 *         ewk_history_item_free() after use. use
 *         ewk_history_item_list_free() for convenience.
 *
 * @see ewk_history_item_list_free()
 * @see ewk_history_back_list_length()
 * @see ewk_history_back_list_get()
 */
EAPI Eina_List        *ewk_history_back_list_get_with_limit(const Ewk_History *history, int limit);

/**
 * Get the whole size of back list.
 *
 * @param history which history instance to query.
 *
 * @return number of elements in whole list.
 *
 * @see ewk_history_back_list_get_with_limit()
 */
EAPI int               ewk_history_back_list_length(const Ewk_History *history);

/**
 * Get maximum capacity of given history.
 *
 * @param history which history instance to query.
 *
 * @return maximum number of entries this history will hold.
 */
EAPI int               ewk_history_limit_get(Ewk_History *history);

/**
 * Set maximum capacity of given history.
 *
 * @param history which history instance to modify.
 * @param limit maximum size to allow.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool         ewk_history_limit_set(const Ewk_History *history, int limit);

/**
 * Create a new history item with given URI and title.
 *
 * @param uri where this resource is located.
 * @param title resource title.
 *
 * @return newly allocated history item or @c NULL on errors. You must
 *         free this item with ewk_history_item_free().
 */
EAPI Ewk_History_Item *ewk_history_item_new(const char *uri, const char *title);

/**
 * Free given history item instance.
 *
 * @param item what to free.
 */
EAPI void              ewk_history_item_free(Ewk_History_Item *item);

/**
 * Free given list and associated history items instances.
 *
 * @param history_items list of items to free (both list nodes and
 *        item instances).
 */
EAPI void              ewk_history_item_list_free(Eina_List *history_items);

/**
 * Query title for given history item.
 *
 * @param item history item to query.
 *
 * @return the title pointer, that may be @c NULL. This pointer is
 *         guaranteed to be eina_stringshare, so whenever possible
 *         save yourself some cpu cycles and use
 *         eina_stringshare_ref() instead of eina_stringshare_add() or
 *         strdup().
 */
EAPI const char       *ewk_history_item_title_get(const Ewk_History_Item *item);

/**
 * Query alternate title for given history item.
 *
 * @param item history item to query.
 *
 * @return the alternate title pointer, that may be @c NULL. This
 *         pointer is guaranteed to be eina_stringshare, so whenever
 *         possible save yourself some cpu cycles and use
 *         eina_stringshare_ref() instead of eina_stringshare_add() or
 *         strdup().
 */
EAPI const char       *ewk_history_item_title_alternate_get(const Ewk_History_Item *item);

/**
 * Set alternate title for given history item.
 *
 * @param item history item to query.
 * @param title new alternate title to use for given item. No
 *        references are kept after this function returns.
 */
EAPI void              ewk_history_item_title_alternate_set(Ewk_History_Item *item, const char *title);

/**
 * Query URI for given history item.
 *
 * @param item history item to query.
 *
 * @return the URI pointer, that may be @c NULL. This pointer is
 *         guaranteed to be eina_stringshare, so whenever possible
 *         save yourself some cpu cycles and use
 *         eina_stringshare_ref() instead of eina_stringshare_add() or
 *         strdup().
 */
EAPI const char       *ewk_history_item_uri_get(const Ewk_History_Item *item);

/**
 * Query original URI for given history item.
 *
 * @param item history item to query.
 *
 * @return the original URI pointer, that may be @c NULL. This pointer
 *         is guaranteed to be eina_stringshare, so whenever possible
 *         save yourself some cpu cycles and use
 *         eina_stringshare_ref() instead of eina_stringshare_add() or
 *         strdup().
 */
EAPI const char       *ewk_history_item_uri_original_get(const Ewk_History_Item *item);

/**
 * Query last visited time for given history item.
 *
 * @param item history item to query.
 *
 * @return the time in seconds this item was visited.
 */
EAPI double            ewk_history_item_time_last_visited_get(const Ewk_History_Item *item);

/**
 * Get the icon (aka favicon) associated with this history item.
 *
 * @note in order to have this working, one must open icon database
 *       with ewk_settings_icon_database_path_set().
 *
 * @param item history item to query.
 *
 * @return the surface reference or @c NULL on errors. Note that the
 *         reference may be to a standard fallback icon.
 */
EAPI cairo_surface_t  *ewk_history_item_icon_surface_get(const Ewk_History_Item *item);

/**
 * Add an Evas_Object of type 'image' to given canvas with history item icon.
 *
 * This is an utility function that creates an Evas_Object of type
 * image set to have fill always match object size
 * (evas_object_image_filled_add()), saving some code to use it from Evas.
 *
 * @note in order to have this working, one must open icon database
 *       with ewk_settings_icon_database_path_set().
 *
 * @param item history item to query.
 * @param canvas evas instance where to add resulting object.
 *
 * @return newly allocated Evas_Object instance or @c NULL on
 *         errors. Delete the object with evas_object_del().
 */
EAPI Evas_Object      *ewk_history_item_icon_object_add(const Ewk_History_Item *item, Evas *canvas);

/**
 * Query if given item is still in page cache.
 *
 * @param item history item to query.
 *
 * @return @c EINA_TRUE if in cache, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool         ewk_history_item_page_cache_exists(const Ewk_History_Item *item);

/**
 * Query number of times item was visited.
 *
 * @param item history item to query.
 *
 * @return number of visits.
 */
EAPI int               ewk_history_item_visit_count(const Ewk_History_Item *item);

/**
 * Query if last visit to item was failure or not.
 *
 * @param item history item to query.
 *
 * @return @c EINA_TRUE if last visit was failure, @c EINA_FALSE if it
 *         was fine.
 */
EAPI Eina_Bool         ewk_history_item_visit_last_failed(const Ewk_History_Item *item);

#ifdef __cplusplus
}
#endif
#endif // ewk_history_h
