/*
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2010 Samsung Electronics
    Copyright (C) 2012 Intel Corporation

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

#ifndef ewk_settings_h
#define ewk_settings_h

#include <Eina.h>
#include <Evas.h>
#include <cairo.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file ewk_settings.h
 *
 * @brief General purpose settings, not tied to any view object.
 */

/**
 * Returns the default quota for Web Database databases. By default
 * this value is 1MB.
 *
 * @return the current default database quota in bytes
 */
EAPI uint64_t         ewk_settings_web_database_default_quota_get(void);

/**
 * Sets the default maximum size (in bytes) an HTML5 Web Database database can have.
 *
 * By default, this value is 1MB.
 *
 * @param maximum_size the new maximum size a database is allowed
 */
EAPI void             ewk_settings_web_database_default_quota_set(uint64_t maximum_size);

/**
 * Sets the current path to the directory WebKit will write Web
 * Database databases.
 *
 * By default, the value is @c ~/.cache/WebKitEfl/Databases
 *
 * @param path the new database directory path
 */
EAPI void             ewk_settings_web_database_path_set(const char *path);

/**
 * Sets the current path to the directory where WebKit will write the
 * HTML5 local storage indexing database (the one keeping track of
 * individual views' local storage databases).
 *
 * By default, the value is @c ~/.local/share/WebKitEfl/LocalStorage
 *
 * @param path the new local storage database directory path
 *
 * @note You may want to call
 * ewk_view_setting_local_storage_database_path_set() on the same @p
 * path, here, for your views.
 */
EAPI void ewk_settings_local_storage_path_set(const char* path);

/**
 * Returns directory's path where the HTML5 local storage indexing
 * database is stored.
 *
 * This is guaranteed to be eina-stringshared, so whenever possible
 * save yourself some cpu cycles and use eina_stringshare_ref()
 * instead of eina_stringshare_add() or strdup().
 *
 * By default, the value is @c ~/.local/share/WebKitEfl/LocalStorage
 *
 * @return database path or @c NULL, on errors.
 *
 * @see ewk_settings_local_storage_path_set()
 */
EAPI const char* ewk_settings_local_storage_path_get(void);

/**
 * Removes @b all HTML 5 local storage databases.
 */
EAPI void ewk_settings_local_storage_database_clear();

/**
 * Clears the HTML 5 local storage database for the given URL
 * (origin).
 *
 * @param url which URL to clear local storage to.
 *
 * After this call, the file holding the local storage database for
 * that origin will be deleted, along with its entry on the local
 * storage files database (a file stored under the path returned by
 * ewk_settings_local_storage_path_get()).
 */
EAPI void ewk_settings_local_storage_database_origin_clear(const char *url);

/**
 * Returns directory path where web database is stored.
 *
 * By default, the value is @c ~/.cache/WebKitEfl/Databases
 *
 * This is guaranteed to be eina_stringshare, so whenever possible
 * save yourself some cpu cycles and use eina_stringshare_ref()
 * instead of eina_stringshare_add() or strdup().
 *
 * @return database path or @c NULL if none or web database is not supported
 */
EAPI const char      *ewk_settings_web_database_path_get(void);

/**
 * Sets directory where to store icon database, opening or closing database.
 *
 * Icon database must be opened only once. If you try to set a path when the icon
 * database is already open, this function returns @c EINA_FALSE.
 *
 * @param directory where to store icon database, must be
 *        write-able, if @c NULL is given, then database is closed
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on errors
 */
EAPI Eina_Bool        ewk_settings_icon_database_path_set(const char *path);

/**
 * Returns directory path where icon database is stored.
 *
 * This is guaranteed to be eina_stringshare, so whenever possible
 * save yourself some cpu cycles and use eina_stringshare_ref()
 * instead of eina_stringshare_add() or strdup().
 *
 * @return database path or @c NULL if none is set
 */
EAPI const char      *ewk_settings_icon_database_path_get(void);

/**
 * Removes all known icons from database.
 *
 * Database must be opened with ewk_settings_icon_database_path_set()
 * in order to work.
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise, like
 *         closed database.
 */
EAPI Eina_Bool        ewk_settings_icon_database_clear(void);

/**
 * Queries icon for given URL, returning associated cairo surface.
 *
 * @note In order to have this working, one must open icon database
 *       with ewk_settings_icon_database_path_set().
 *
 * @param url which url to query icon
 *
 * @return cairo surface if any, or @c NULL on failure
 */
EAPI cairo_surface_t *ewk_settings_icon_database_icon_surface_get(const char *url);

/**
 * Gets image representing the given URL.
 *
 * This is an utility function that creates an Evas_Object of type
 * image set to have fill always match object size
 * (evas_object_image_filled_add()), saving some code to use it from Evas.
 *
 * @note In order to have this working, one must open icon database
 *       with ewk_settings_icon_database_path_set().
 *
 * @note The "load,finished" signal doesn't guarantee that icons are completely loaded and
 *        saved to database. Icon can be taken after the "icon,received" signal.
 *
 * @param url which url to query icon
 * @param canvas evas instance where to add resulting object
 *
 * @return newly allocated Evas_Object instance or @c NULL on
 *         errors. Delete the object with evas_object_del().
 */
EAPI Evas_Object     *ewk_settings_icon_database_icon_object_get(const char *url, Evas *canvas);

/**
 * Sets the path where the application cache will be stored.
 *
 * The Offline Application Caching APIs are part of HTML5 and allow applications to store data locally that is accessed
 * when the network cannot be reached.
 *
 * By default, the path is @c ~/.cache/WebKitEfl/Applications
 * Once the path is set, the feature is enabled and the path cannot be changed.
 *
 * @param path where to store cache, must be write-able.
 *
 * @sa ewk_view_setting_application_cache_set
 */
EAPI void             ewk_settings_application_cache_path_set(const char *path);

/**
 * Returns the path where the HTML5 application cache is stored.
 *
 * The Offline Application Caching APIs are part of HTML5 and allow applications to store data locally that is accessed
 * when the network cannot be reached.
 *
 * By default, the path is @c ~/.cache/WebKitEfl/Applications
 *
 * @return eina_stringshare'd path value.
 *
 * @sa ewk_view_setting_application_cache_set
 */
EAPI const char      *ewk_settings_application_cache_path_get(void);

/**
 * Returns the maximum size, in bytes, of the application cache for HTML5 Offline Web Applications.
 *
 * By default, applications are allowed unlimited storage space.
 *
 * @sa ewk_view_setting_offine_app_cache_set
 */
EAPI int64_t          ewk_settings_application_cache_max_quota_get(void);

/**
 * Sets the maximum size, in bytes, of the application cache for HTML5 Offline Web Applications.
 *
 * By default, applications are allowed unlimited storage space.
 *
 * Note that calling this function will delete all the entries currently in the app cache.
 *
 * @param maximum_size the new maximum size, in bytes.
 *
 * @sa ewk_view_setting_application_cache_enabled_set
 */
EAPI void             ewk_settings_application_cache_max_quota_set(int64_t maximum_size);

/**
 * Removes all entries from the HTML5 application cache.
 *
 * @sa ewk_view_setting_application_cache_enabled_set, ewk_settings_application_cache_path_set
 */
EAPI void             ewk_settings_application_cache_clear(void);

/**
 * Returns whether the in-memory object cache is enabled.
 *
 * The object cache is responsible for holding resources such as scripts, stylesheets
 * and images in memory.
 *
 * By default, the cache is enabled.
 *
 * @return @c EINA_TRUE if the cache is enabled or @c EINA_FALSE if not
 *
 * @sa ewk_settings_object_cache_capacity_set
 */
EAPI Eina_Bool        ewk_settings_object_cache_enable_get(void);

/**
 * Enables/disables the in-memory object cache of WebCore, possibly clearing it.
 *
 * The object cache is responsible for holding resources such as scripts, stylesheets
 * and images in memory.
 *
 * By default, the cache is enabled.
 *
 * Disabling the cache will remove all resources from the cache.
 * They may still live on if they are referenced by some Web page though.
 *
 * @param set @c EINA_TRUE to enable memory cache, @c EINA_FALSE to disable
 */
EAPI void             ewk_settings_object_cache_enable_set(Eina_Bool set);

/**
 * Returns whether Shadow DOM is enabled.
 *
 * Shadow DOM is a method of establishing and maintaining functional boundaries between
 * DOM subtrees and how these subtrees interact with each other within a document tree,
 * thus enabling better functional encapsulation within DOM.
 *
 * By default, Shadow DOM is disabled.
 *
 * @return @c EINA_TRUE if Shadow DOM is enabled or @c EINA_FALSE if not
 *
 * @sa ewk_settings_shadow_dom_enable_set
 */
EAPI Eina_Bool    ewk_settings_shadow_dom_enable_get(void);

/**
 * Enables/disables Shadow DOM functionality.
 *
 * Shadow DOM is a method of establishing and maintaining functional boundaries between
 * DOM subtrees and how these subtrees interact with each other within a document tree,
 * thus enabling better functional encapsulation within DOM.
 *
 * By default, Shadow DOM is disabled.
 *
 * @param set @c EINA_TRUE to enable Shadow DOM, @c EINA_FALSE to disable
 */
EAPI Eina_Bool    ewk_settings_shadow_dom_enable_set(Eina_Bool enable);

/**
 * Defines the capacities for the in-memory object cache.
 *
 * The object cache is responsible for holding resources such as scripts, stylesheets
 * and images in memory.
 *
 * By default, @p min_dead_bytes is 0 and both @p max_dead_bytes and @p total_bytes are 8MB.
 *
 * @param min_dead_bytes The maximum number of bytes that dead resources should consume when
 *                       the cache is under pressure.
 * @param max_dead_bytes The maximum number of bytes that dead resources should consume when
 *                       the cache is not under pressure.
 * @param total_bytes    The maximum number of bytes that the cache should consume overall.
 *
 * @param capacity the maximum number of bytes that the cache should consume overall
 */
EAPI void             ewk_settings_object_cache_capacity_set(unsigned min_dead_bytes, unsigned max_dead_bytes, unsigned total_bytes);

/**
 * Returns the maximum number of pages in the memory page cache.
 *
 * By default, maximum number of pages is 3.
 *
 * @return  The maximum number of pages in the memory page cache.
 *
 * @sa ewk_settings_page_cache_capacity_set
 */
EAPI unsigned         ewk_settings_page_cache_capacity_get(void);

/**
 * Defines the capacity for the memory page cache.
 *
 * The page cache is responsible for holding visited web pages in memory. So it improves user experience when navigating forth or back
 * to pages in the forward/back history as the cached pages do not require to be loaded from server.
 *
 * By default, @p pages is 3.
 *
 * @param pages The maximum number of pages to keep in the memory page cache.
 */
EAPI void             ewk_settings_page_cache_capacity_set(unsigned pages);

/**
 * Clears all memory caches.
 *
 * This function clears all memory caches, which include the object cache (for resources such as
 * images, scripts and stylesheets), the page cache, the font cache and the Cross-Origin Preflight
 * cache.
 */
EAPI void             ewk_settings_memory_cache_clear(void);

/**
 * Sets values for repaint throttling.
 *
 * It allows to slow down page loading and
 * should ensure displaying a content with many css/gif animations.
 *
 * These values can be used as a example for repaints throttling.
 * 0,     0,   0,    0    - default WebCore's values, these do not delay any repaints
 * 0.025, 0,   2.5,  0.5  - recommended values for dynamic content
 * 0.01,  0,   1,    0.2  - minimal level
 * 0.025, 1,   5,    0.5  - medium level
 * 0.1,   2,   10,   1    - heavy level
 *
 * @param deferred_repaint_delay a normal delay
 * @param initial_deferred_repaint_delay_during_loading negative value would mean that first few repaints happen without a delay
 * @param max_deferred_repaint_delay_during_loading the delay grows on each repaint to this maximum value
 * @param deferred_repaint_delay_increment_during_loading on each repaint the delay increses by this amount
 */
EAPI void             ewk_settings_repaint_throttling_set(double deferred_repaint_delay, double initial_deferred_repaint_delay_during_loading, double max_deferred_repaint_delay_during_loading, double deferred_repaint_delay_increment_during_loading);

/**
 * Gets the default interval for DOMTimers on all pages.
 *
 * DOMTimer processes javascript function registered by setInterval() based on interval value.
 *
 * @return default minimum interval for DOMTimers
 */
EAPI double           ewk_settings_default_timer_interval_get(void);

/**
 * Sets the CSS media type.
 *
 * Setting this will override the normal value of the CSS media property.
 *
 * Setting the value to @c NULL will restore the internal default value.
 *
 * @param type css media type to be set, must be write-able
 *
 * @sa ewk_settings_css_media_type_get
 */
EAPI void             ewk_settings_css_media_type_set(const char *type);

/**
 * Returns the current CSS media type.
 *
 * It will only return the value set through ewk_settings_css_media_type_set and not the one used internally.
 *
 * This is guaranteed to be eina_stringshare, so whenever possible
 * save yourself some cpu cycles and use eina_stringshare_ref()
 * instead of eina_stringshare_add() or strdup().
 * 
 * @return css media type set by user or @c NULL if none is set
 *
 * @sa ewk_settings_css_media_type_set
 */
EAPI const char      *ewk_settings_css_media_type_get(void);

#ifdef __cplusplus
}
#endif
#endif // ewk_settings_h
