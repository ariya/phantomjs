/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/**
 * @file    ewk_context.h
 * @brief   Describes the context API.
 *
 * @note ewk_context encapsulates all pages related to specific use of WebKit.
 *
 * Applications have the option of creating a context different than the default one
 * and use it for a group of pages. All pages in the same context share the same
 * preferences, visited link set, local storage, etc.
 *
 * A process model can be specified per context. The default one is the shared model
 * where the web-engine process is shared among the pages in the context. The second
 * model allows each page to use a separate web-engine process. This latter model is
 * currently not supported by WebKit2/EFL.
 *
 */

#ifndef ewk_context_h
#define ewk_context_h

#include "ewk_cookie_manager.h"
#include "ewk_database_manager.h"
#include "ewk_favicon_database.h"
#include "ewk_navigation_data.h"
#include "ewk_storage_manager.h"
#include "ewk_url_scheme_request.h"
#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Declare Ewk_Context as Ewk_Object.
 *
 * @see Ewk_Object
 */
typedef struct EwkObject Ewk_Context;

/**
 * \enum    Ewk_Cache_Model
 *
 * @brief   Contains option for cache model
 */
enum Ewk_Cache_Model {
    /// Use the smallest cache capacity.
    EWK_CACHE_MODEL_DOCUMENT_VIEWER,
    /// Use bigger cache capacity than EWK_CACHE_MODEL_DOCUMENT_VIEWER.
    EWK_CACHE_MODEL_DOCUMENT_BROWSER,
    /// Use the biggest cache capacity.
    EWK_CACHE_MODEL_PRIMARY_WEBBROWSER
};

/// Creates a type name for the Ewk_Cache_Model.
typedef enum Ewk_Cache_Model Ewk_Cache_Model;

/**
 * @typedef Ewk_Url_Scheme_Request_Cb Ewk_Url_Scheme_Request_Cb
 * @brief Callback type for use with ewk_context_url_scheme_register().
 */
typedef void (*Ewk_Url_Scheme_Request_Cb) (Ewk_Url_Scheme_Request *request, void *user_data);

/**
 * @typedef Ewk_History_Navigation_Cb Ewk_History_Navigation_Cb
 * @brief Type definition for a function that will be called back when @a view did navigation (loaded new URL).
 */
typedef void (*Ewk_History_Navigation_Cb)(const Evas_Object *view, Ewk_Navigation_Data *navigation_data, void *user_data);

/**
 * @typedef Ewk_History_Client_Redirection_Cb Ewk_History_Client_Redirection_Cb
 * @brief Type definition for a function that will be called back when @a view performed a client redirect.
 */
typedef void (*Ewk_History_Client_Redirection_Cb)(const Evas_Object *view, const char *source_url, const char *destination_url, void *user_data);

/**
 * @typedef Ewk_History_Server_Redirection_Cb Ewk_History_Server_Redirection_Cb
 * @brief Type definition for a function that will be called back when @a view performed a server redirect.
 */
typedef void (*Ewk_History_Server_Redirection_Cb)(const Evas_Object *view, const char *source_url, const char *destination_url, void *user_data);

/**
 * @typedef Ewk_History_Title_Update_Cb Ewk_History_Title_Update_Cb
 * @brief Type definition for a function that will be called back when history title is updated.
 */
typedef void (*Ewk_History_Title_Update_Cb)(const Evas_Object *view, const char *title, const char *url, void *user_data);

/**
 * @typedef Ewk_Context_History_Client_Visited_Links_Populate_Cb Ewk_Context_History_Client_Visited_Links_Populate_Cb
 * @brief Type definition for a function that will be called back when client is asked to provide visited links from a client-managed storage.
 *
 * @see ewk_context_visited_link_add
 */
typedef void (*Ewk_History_Populate_Visited_Links_Cb)(void *user_data);

/**
 * Gets default Ewk_Context instance.
 *
 * The returned Ewk_Context object @b should not be unref'ed if application
 * does not call ewk_context_ref() for that.
 *
 * @return Ewk_Context object.
 */
EAPI Ewk_Context *ewk_context_default_get(void);

/**
 * Creates a new Ewk_Context.
 *
 * The returned Ewk_Context object @b should be unref'ed after use.
 *
 * @return Ewk_Context object on success or @c NULL on failure
 *
 * @see ewk_object_unref
 * @see ewk_context_new_with_injected_bundle_path
 */
EAPI Ewk_Context *ewk_context_new(void);

/**
 * Creates a new Ewk_Context.
 *
 * The returned Ewk_Context object @b should be unref'ed after use.
 *
 * @param path path of injected bundle library
 *
 * @return Ewk_Context object on success or @c NULL on failure
 *
 * @see ewk_object_unref
 * @see ewk_context_new
 */
EAPI Ewk_Context *ewk_context_new_with_injected_bundle_path(const char *path);

/**
 * Gets the cookie manager instance for this @a context.
 *
 * @param context context object to query.
 *
 * @return Ewk_Cookie_Manager object instance or @c NULL in case of failure.
 */
EAPI Ewk_Cookie_Manager *ewk_context_cookie_manager_get(const Ewk_Context *context);

/**
 * Gets the database manager instance for this @a context.
 *
 * @param context context object to query
 *
 * @return Ewk_Database_Manager object instance or @c NULL in case of failure
 */
EAPI Ewk_Database_Manager *ewk_context_database_manager_get(const Ewk_Context *context);

/**
 * Sets the favicon database directory for this @a context.
 *
 * Sets the directory path to be used to store the favicons database
 * for @a context on disk. Passing @c NULL as @a directory_path will
 * result in using the default directory for the platform.
 *
 * Calling this method also means enabling the favicons database for
 * its use from the applications, it is therefore expected to be
 * called only once. Further calls for the same instance of
 * @a context will not have any effect.
 *
 * @param context context object to update
 * @param directory_path database directory path to set
 *
 * @return @c EINA_TRUE if successful, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_context_favicon_database_directory_set(Ewk_Context *context, const char *directory_path);

/**
 * Gets the favicon database instance for this @a context.
 *
 * @param context context object to query.
 *
 * @return Ewk_Favicon_Database object instance or @c NULL in case of failure.
 */
EAPI Ewk_Favicon_Database *ewk_context_favicon_database_get(const Ewk_Context *context);

/**
 * Gets the storage manager instance for this @a context.
 *
 * @param context context object to query.
 *
 * @return Ewk_Storage_Manager object instance or @c NULL in case of failure.
 */
EAPI Ewk_Storage_Manager *ewk_context_storage_manager_get(const Ewk_Context *context);

/**
 * Register @a scheme in @a context.
 *
 * When an URL request with @a scheme is made in the #Ewk_Context, the callback
 * function provided will be called with a #Ewk_Url_Scheme_Request.
 *
 * It is possible to handle URL scheme requests asynchronously, by calling ewk_url_scheme_ref() on the
 * #Ewk_Url_Scheme_Request and calling ewk_url_scheme_request_finish() later when the data of
 * the request is available.
 *
 * @param context a #Ewk_Context object.
 * @param scheme the network scheme to register
 * @param callback the function to be called when an URL request with @a scheme is made.
 * @param user_data data to pass to callback function
 *
 * @code
 * static void about_url_scheme_request_cb(Ewk_Url_Scheme_Request *request, void *user_data)
 * {
 *     const char *path;
 *     char *contents_data = NULL;
 *     unsigned int contents_length = 0;
 *
 *     path = ewk_url_scheme_request_path_get(request);
 *     if (!strcmp(path, "plugins")) {
 *         // Initialize contents_data with the contents of plugins about page, and set its length to contents_length
 *     } else if (!strcmp(path, "memory")) {
 *         // Initialize contents_data with the contents of memory about page, and set its length to contents_length
 *     } else if (!strcmp(path, "applications")) {
 *         // Initialize contents_data with the contents of application about page, and set its length to contents_length
 *     } else {
 *         Eina_Strbuf *buf = eina_strbuf_new();
 *         eina_strbuf_append_printf(buf, "&lt;html&gt;&lt;body&gt;&lt;p&gt;Invalid about:%s page&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;", path);
 *         contents_data = eina_strbuf_string_steal(buf);
 *         contents_length = strlen(contents);
 *         eina_strbuf_free(buf);
 *     }
 *     ewk_url_scheme_request_finish(request, contents_data, contents_length, "text/html");
 *     free(contents_data);
 * }
 * @endcode
 */
EAPI Eina_Bool ewk_context_url_scheme_register(Ewk_Context *context, const char *scheme, Ewk_Url_Scheme_Request_Cb callback, void *user_data);

/**
 * Sets history callbacks for the given @a context.
 *
 * To stop listening for history events, you may call this function with @c
 * NULL for the callbacks.
 *
 * @param context context object to set history callbacks
 * @param navigate_func The function to call when @c ewk_view did navigation (may be @c NULL).
 * @param client_redirect_func The function to call when @c ewk_view performed a client redirect (may be @c NULL).
 * @param server_redirect_func The function to call when @c ewk_view performed a server redirect (may be @c NULL).
 * @param title_update_func The function to call when history title is updated (may be @c NULL).
 * @param populate_visited_links_func The function is called when client is asked to provide visited links from a
 *        client-managed storage (may be @c NULL).
 * @param data User data (may be @c NULL).
 */
EAPI void ewk_context_history_callbacks_set(Ewk_Context *context,
                                            Ewk_History_Navigation_Cb navigate_func,
                                            Ewk_History_Client_Redirection_Cb client_redirect_func,
                                            Ewk_History_Server_Redirection_Cb server_redirect_func,
                                            Ewk_History_Title_Update_Cb title_update_func,
                                            Ewk_History_Populate_Visited_Links_Cb populate_visited_links_func,
                                            void *data);

/**
 * Registers the given @a visited_url as visited link in @a context visited link cache.
 *
 * This function shall be invoked as a response to @c populateVisitedLinks callback of the history cient.
 *
 * @param context context object to add visited link data
 * @param visited_url visited url
 *
 * @see Ewk_Context_History_Client
 */
EAPI void ewk_context_visited_link_add(Ewk_Context *context, const char *visited_url);

/**
 * Set @a cache_model as the cache model for @a context.
 *
 * By default, it is EWK_CACHE_MODEL_DOCUMENT_VIEWER.
 *
 * @param context context object to update.
 * @param cache_model a #Ewk_Cache_Model.
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_cache_model_set(Ewk_Context *context, Ewk_Cache_Model cache_model);

/**
 * Gets the cache model for @a context.
 *
 * @param context context object to query.
 *
 * @return the cache model for the @a context.
 */
EAPI Ewk_Cache_Model ewk_context_cache_model_get(const Ewk_Context *context);

/**
 * Sets additional plugin path for @a context.
 *
 * @param context context object to set additional plugin path
 * @param path the path to be used for plugins
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI Eina_Bool ewk_context_additional_plugin_path_set(Ewk_Context *context, const char *path);

/**
 * Clears HTTP caches in local storage and all resources cached in memory 
 * such as images, CSS, JavaScript, XSL, and fonts for @a context.
 *
 * @param context context object to clear all resource caches
 */
EAPI void ewk_context_resource_cache_clear(Ewk_Context *context);

#ifdef __cplusplus
}
#endif

#endif // ewk_context_h
