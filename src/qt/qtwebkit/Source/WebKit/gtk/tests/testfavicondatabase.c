/*
 * Copyright (C) 2012 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "autotoolsconfig.h"
#include "test_utils.h"
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <string.h>
#include <webkit/webkit.h>

const int gIconSize = 16;

GMainLoop *loop;
char *baseURI;

static void
serverCallback(SoupServer *server, SoupMessage *message, const char *path, GHashTable *query, SoupClientContext *context, void *data)
{
    if (message->method != SOUP_METHOD_GET) {
        soup_message_set_status(message, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    soup_message_set_status(message, SOUP_STATUS_OK);

    char *contents;
    gsize length;
    if (g_str_equal(path, "/favicon.ico")) {
        GError *error = NULL;

        g_file_get_contents("blank.ico", &contents, &length, &error);
        g_assert(!error);
    } else {
        contents = g_strdup("<html><body>test</body></html>");
        length = strlen(contents);
    }

    soup_message_body_append(message->response_body, SOUP_MEMORY_TAKE, contents, length);
    soup_message_body_complete(message->response_body);
}

static void deleteDatabaseFileIfExists(const char *databasePath)
{
    if (!g_file_test(databasePath, G_FILE_TEST_IS_DIR))
        return;

    char *databaseFilename = g_build_filename(databasePath, "WebpageIcons.db", NULL);
    if (g_unlink(databaseFilename) == -1) {
        g_free(databaseFilename);
        return;
    }

    g_free(databaseFilename);
    g_rmdir(databasePath);
}

static void testWebKitFaviconDatabaseSetPath()
{
    char *databasePath = g_build_filename(g_get_tmp_dir(), "webkit-testfavicondatabase", NULL);
    deleteDatabaseFileIfExists(databasePath);

    WebKitFaviconDatabase *database = webkit_get_favicon_database();
    webkit_favicon_database_set_path(database, databasePath);

    g_assert_cmpstr(databasePath, ==, webkit_favicon_database_get_path(database));

    g_free(databasePath);
}

// See the comment in main() function that goes with this same guard.
#ifdef NDEBUG

static void faviconDatabaseGetValidFaviconCallback(GObject *sourceObject, GAsyncResult *result, void *userData)
{
    gboolean *beenHere = (gboolean*)userData;
    GError *error = NULL;
    GdkPixbuf *icon = webkit_favicon_database_get_favicon_pixbuf_finish(WEBKIT_FAVICON_DATABASE(sourceObject), result, &error);
    g_assert(icon);
    g_object_unref(icon);

    *beenHere = TRUE;

    g_main_loop_quit(loop);
}

static void faviconDatabaseGetInvalidFaviconCallback(GObject *sourceObject, GAsyncResult *result, void *userData)
{
    gboolean *beenHere = (gboolean*)userData;
    GError *error = NULL;
    GdkPixbuf *icon = webkit_favicon_database_get_favicon_pixbuf_finish(WEBKIT_FAVICON_DATABASE(sourceObject), result, &error);
    g_assert(!icon);

    *beenHere = TRUE;

    g_main_loop_quit(loop);
}

static void faviconDatabaseGetFaviconCancelledCallback(GObject *sourceObject, GAsyncResult *result, void *userData)
{
    gboolean *beenHere = (gboolean*)userData;
    GError *error = NULL;
    GdkPixbuf *icon = webkit_favicon_database_get_favicon_pixbuf_finish(WEBKIT_FAVICON_DATABASE(sourceObject), result, &error);
    g_assert(!icon);
    g_assert(error && g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED));

    *beenHere = TRUE;

    g_main_loop_quit(loop);
}

static inline void quitMainLoopIfLoadCompleted(gboolean *iconOrPageLoaded)
{
    if (*iconOrPageLoaded)
        g_main_loop_quit(loop);
    else
        *iconOrPageLoaded = TRUE;
}

static void idleQuitLoopCallback(WebKitWebView *webView, GParamSpec *paramSpec, gboolean *iconOrPageLoaded)
{
    WebKitLoadStatus status = webkit_web_view_get_load_status(webView);

    if (status == WEBKIT_LOAD_FINISHED || status == WEBKIT_LOAD_FAILED)
        quitMainLoopIfLoadCompleted(iconOrPageLoaded);
}

static void webkitWebViewIconLoaded(WebKitFaviconDatabase *database, const char *frameURI, gboolean *iconOrPageLoaded)
{
    quitMainLoopIfLoadCompleted(iconOrPageLoaded);
}

static void loadURI(const char *uri)
{
    WebKitWebView *view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    gboolean iconOrPageLoaded = FALSE;

    webkit_web_view_load_uri(view, uri);

    g_signal_connect(view, "notify::load-status", G_CALLBACK(idleQuitLoopCallback), &iconOrPageLoaded);
    g_signal_connect(view, "icon-loaded", G_CALLBACK(webkitWebViewIconLoaded), &iconOrPageLoaded);

    g_main_loop_run(loop);

    g_signal_handlers_disconnect_matched(view, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, &iconOrPageLoaded);
}

static gboolean faviconDatabaseGetValidFaviconIdle(void *userData)
{
    webkit_favicon_database_get_favicon_pixbuf(webkit_get_favicon_database(), baseURI,
                                               gIconSize, gIconSize, NULL,
                                               faviconDatabaseGetValidFaviconCallback, userData);
    return FALSE;
}

static gboolean faviconDatabaseGetInvalidFaviconIdle(void *userData)
{
    webkit_favicon_database_get_favicon_pixbuf(webkit_get_favicon_database(), "http://www.webkitgtk.org/",
                                               gIconSize, gIconSize, NULL,
                                               faviconDatabaseGetInvalidFaviconCallback, userData);
    return FALSE;
}

static gboolean faviconDatabaseGetFaviconCancelledIdle(void *userData)
{
    GCancellable *cancellable = g_cancellable_new();
    webkit_favicon_database_get_favicon_pixbuf(webkit_get_favicon_database(), baseURI,
                                               gIconSize, gIconSize, cancellable,
                                               faviconDatabaseGetFaviconCancelledCallback, userData);
    g_cancellable_cancel(cancellable);
    g_object_unref(cancellable);
    return FALSE;
}

static void testWebKitFaviconDatabaseGetFavicon()
{
    gboolean beenToIconCallback;

    loop = g_main_loop_new(NULL, TRUE);

    /* Load uri to make sure favicon is added to database. */
    loadURI(baseURI);

    beenToIconCallback = FALSE;
    g_idle_add((GSourceFunc)faviconDatabaseGetValidFaviconIdle, &beenToIconCallback);
    g_main_loop_run(loop);
    g_assert(beenToIconCallback);

    beenToIconCallback = FALSE;
    g_idle_add((GSourceFunc)faviconDatabaseGetInvalidFaviconIdle, &beenToIconCallback);
    g_main_loop_run(loop);
    g_assert(beenToIconCallback);

    beenToIconCallback = FALSE;
    g_idle_add((GSourceFunc)faviconDatabaseGetFaviconCancelledIdle, &beenToIconCallback);
    g_main_loop_run(loop);
    g_assert(beenToIconCallback);
}

static void testWebKitFaviconDatabaseGetFaviconURI()
{
    char *iconURI = webkit_favicon_database_get_favicon_uri(webkit_get_favicon_database(), baseURI);
    char *expectedURI = g_strdup_printf("%sfavicon.ico", baseURI);
    g_assert_cmpstr(iconURI, ==, expectedURI);
    g_free(expectedURI);
    g_free(iconURI);
}

#endif

static void testWebKitFaviconDatabaseRemoveAll(void)
{
    WebKitFaviconDatabase *database = webkit_get_favicon_database();
    webkit_favicon_database_clear(database);
    char *iconURI = webkit_favicon_database_get_favicon_uri(database, baseURI);
    g_assert(!iconURI);
    g_free(iconURI);
}

static void testWebKitFaviconDatabaseCloseDatabase(void)
{
    WebKitFaviconDatabase *database = webkit_get_favicon_database();
    char *databasePath = g_strdup(webkit_favicon_database_get_path(database));
    webkit_favicon_database_set_path(database, 0);
    deleteDatabaseFileIfExists(databasePath);
    g_free(databasePath);
}

int main(int argc, char **argv)
{
    gtk_test_init(&argc, &argv, NULL);

    /* This hopefully makes the test independent of the path it's called from. */
    testutils_relative_chdir("Source/WebKit/gtk/tests/resources/test.html", argv[0]);

    SoupServer *server = soup_server_new(SOUP_SERVER_PORT, 0, NULL);
    soup_server_run_async(server);

    soup_server_add_handler(server, NULL, serverCallback, NULL, NULL);

    SoupURI *soupURI = soup_uri_new("http://127.0.0.1/");
    soup_uri_set_port(soupURI, soup_server_get_port(server));

    baseURI = soup_uri_to_string(soupURI, FALSE);
    soup_uri_free(soupURI);

    g_test_bug_base("https://bugs.webkit.org/");
    g_test_add_func("/webkit/favicondatabase/set-path", testWebKitFaviconDatabaseSetPath);

    // These two tests will trigger an ASSERTION on debug builds due
    // to http://webkit.org/b/67582. Remove the guards once the bug is fixed.
#ifdef NDEBUG
    g_test_add_func("/webkit/favicondatabase/get-favicon", testWebKitFaviconDatabaseGetFavicon);
    g_test_add_func("/webkit/favicondatabase/get-favicon-uri", testWebKitFaviconDatabaseGetFaviconURI);
#endif

    g_test_add_func("/webkit/favicondatabase/remove-all", testWebKitFaviconDatabaseRemoveAll);
    g_test_add_func("/webkit/favicondatabase/close-db", testWebKitFaviconDatabaseCloseDatabase);

    return g_test_run();
}
