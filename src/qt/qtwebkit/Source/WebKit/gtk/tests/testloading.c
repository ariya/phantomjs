/*
 * Copyright (C) 2009, 2010 Gustavo Noronha Silva
 * Copyright (C) 2009 Igalia S.L.
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
#include <gtk/gtk.h>
#include <libsoup/soup.h>
#include <string.h>
#include <webkit/webkit.h>

/* This string has to be rather big because of the cancelled test - it
 * looks like soup refuses to send or receive a too small chunk */
#define HTML_STRING "<html><body>Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!Testing!</body></html>"

SoupURI* base_uri;

/* For real request testing */
static void
server_callback(SoupServer* server, SoupMessage* msg,
                const char* path, GHashTable* query,
                SoupClientContext* context, gpointer data)
{
    if (msg->method != SOUP_METHOD_GET) {
        soup_message_set_status(msg, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    soup_message_set_status(msg, SOUP_STATUS_OK);

    if (g_str_equal(path, "/test_loading_status") || g_str_equal(path, "/test_loading_status2"))
        soup_message_body_append(msg->response_body, SOUP_MEMORY_STATIC, HTML_STRING, strlen(HTML_STRING));
    else if (g_str_equal(path, "/test_load_error")) {
        soup_message_set_status(msg, SOUP_STATUS_CANT_CONNECT);
    } else if (g_str_equal(path, "/test_loading_cancelled")) {
        soup_message_headers_set_encoding(msg->response_headers, SOUP_ENCODING_CHUNKED);
        soup_message_body_append(msg->response_body, SOUP_MEMORY_STATIC, HTML_STRING, strlen(HTML_STRING));
        soup_server_unpause_message(server, msg);
        return;
    }

    soup_message_body_complete(msg->response_body);
}

typedef struct {
    WebKitWebView* webView;
    GMainLoop *loop;
    gboolean has_been_provisional;
    gboolean has_been_committed;
    gboolean has_been_first_visually_non_empty_layout;
    gboolean has_been_finished;
    gboolean has_been_failed;
    gboolean has_been_load_error;
} WebLoadingFixture;

static void web_loading_fixture_setup(WebLoadingFixture* fixture, gconstpointer data)
{
    fixture->webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    fixture->loop = g_main_loop_new(NULL, TRUE);
    g_object_ref_sink(fixture->webView);
    fixture->has_been_provisional = FALSE;
    fixture->has_been_committed = FALSE;
    fixture->has_been_first_visually_non_empty_layout = FALSE;
    fixture->has_been_finished = FALSE;
    fixture->has_been_failed = FALSE;
    fixture->has_been_load_error = FALSE;
}

static void web_loading_fixture_teardown(WebLoadingFixture* fixture, gconstpointer data)
{
    g_object_unref(fixture->webView);
    g_main_loop_unref(fixture->loop);
}

static char* get_uri_for_path(const char* path)
{
    SoupURI* uri;
    char* uri_string;

    uri = soup_uri_new_with_base(base_uri, path);
    uri_string = soup_uri_to_string(uri, FALSE);
    soup_uri_free (uri);

    return uri_string;
}

static void load_finished_cb(WebKitWebView* web_view, WebKitWebFrame* web_frame, WebLoadingFixture* fixture)
{
    g_assert(fixture->has_been_provisional);
    g_assert(fixture->has_been_committed);
    g_assert(fixture->has_been_first_visually_non_empty_layout);

    g_main_loop_quit(fixture->loop);
}


static void status_changed_cb(GObject* object, GParamSpec* pspec, WebLoadingFixture* fixture)
{
    WebKitLoadStatus status = webkit_web_view_get_load_status(WEBKIT_WEB_VIEW(object));

    switch (status) {
    case WEBKIT_LOAD_PROVISIONAL:
        g_assert(!fixture->has_been_provisional);
        g_assert(!fixture->has_been_committed);
        g_assert(!fixture->has_been_first_visually_non_empty_layout);
        fixture->has_been_provisional = TRUE;
        break;
    case WEBKIT_LOAD_COMMITTED:
        g_assert(fixture->has_been_provisional);
        g_assert(!fixture->has_been_committed);
        g_assert(!fixture->has_been_first_visually_non_empty_layout);
        fixture->has_been_committed = TRUE;
        break;
    case WEBKIT_LOAD_FIRST_VISUALLY_NON_EMPTY_LAYOUT:
        g_assert(fixture->has_been_provisional);
        g_assert(fixture->has_been_committed);
        g_assert(!fixture->has_been_first_visually_non_empty_layout);
        fixture->has_been_first_visually_non_empty_layout = TRUE;
        break;
    case WEBKIT_LOAD_FINISHED:
        g_assert(fixture->has_been_provisional);
        g_assert(fixture->has_been_committed);
        g_assert(fixture->has_been_first_visually_non_empty_layout);
        break;
    default:
        g_assert_not_reached();
    }
}

static void test_loading_status(WebLoadingFixture* fixture, gconstpointer data)
{
    char* uri_string;

    g_assert_cmpint(webkit_web_view_get_load_status(fixture->webView), ==, WEBKIT_LOAD_PROVISIONAL);

    g_object_connect(G_OBJECT(fixture->webView),
                     "signal::notify::load-status", G_CALLBACK(status_changed_cb), fixture,
                     "signal::load-finished", G_CALLBACK(load_finished_cb), fixture,
                     NULL);

    uri_string = get_uri_for_path("/test_loading_status");

    /* load_uri will trigger the navigation-policy-decision-requested
     * signal emission;
     */
    webkit_web_view_load_uri(fixture->webView, uri_string);
    g_free(uri_string);

    g_main_loop_run(fixture->loop);
}

static void load_error_status_changed_cb(GObject* object, GParamSpec* pspec, WebLoadingFixture* fixture)
{
    WebKitLoadStatus status = webkit_web_view_get_load_status(WEBKIT_WEB_VIEW(object));

    switch(status) {
    case WEBKIT_LOAD_PROVISIONAL:
        g_assert(!fixture->has_been_provisional);
        fixture->has_been_provisional = TRUE;
        break;
    case WEBKIT_LOAD_COMMITTED:
        g_assert(!fixture->has_been_committed);
        fixture->has_been_committed = TRUE;
        break;
    case WEBKIT_LOAD_FINISHED:
        g_assert(fixture->has_been_provisional);
        g_assert(fixture->has_been_load_error);
        g_assert(fixture->has_been_failed);
        g_assert(!fixture->has_been_finished);
        fixture->has_been_finished = TRUE;
        break;
    case WEBKIT_LOAD_FAILED:
        g_assert(!fixture->has_been_failed);
        fixture->has_been_failed = TRUE;
        g_main_loop_quit(fixture->loop);
        break;
    default:
        break;
    }
}

static gboolean load_error_cb(WebKitWebView* webView, WebKitWebFrame* frame, const char* uri, GError *error, WebLoadingFixture* fixture)
{
    g_assert(fixture->has_been_provisional);
    g_assert(!fixture->has_been_load_error);
    fixture->has_been_load_error = TRUE;

    return FALSE;
}

static void test_loading_error(WebLoadingFixture* fixture, gconstpointer data)
{
    char* uri_string;

    g_test_bug("28842");

    g_signal_connect(fixture->webView, "load-error", G_CALLBACK(load_error_cb), fixture);
    g_signal_connect(fixture->webView, "notify::load-status", G_CALLBACK(load_error_status_changed_cb), fixture);

    uri_string = get_uri_for_path("/test_load_error");
    webkit_web_view_load_uri(fixture->webView, uri_string);
    g_free(uri_string);

    g_main_loop_run(fixture->loop);

    g_assert(fixture->has_been_provisional);
    g_assert(!fixture->has_been_committed);
    g_assert(fixture->has_been_load_error);
    g_assert(fixture->has_been_failed);
    g_assert(!fixture->has_been_finished);
}

/* Cancelled load */

static gboolean load_cancelled_cb(WebKitWebView* webView, WebKitWebFrame* frame, const char* uri, GError *error, WebLoadingFixture* fixture)
{
    g_assert(fixture->has_been_provisional);
    g_assert(fixture->has_been_failed);
    g_assert(!fixture->has_been_load_error);
    g_assert(error->code == WEBKIT_NETWORK_ERROR_CANCELLED);
    fixture->has_been_load_error = TRUE;

    return TRUE;
}

static gboolean stop_load (gpointer data)
{
    webkit_web_view_stop_loading(WEBKIT_WEB_VIEW(data));
    return FALSE;
}

static void load_cancelled_status_changed_cb(GObject* object, GParamSpec* pspec, WebLoadingFixture* fixture)
{
    WebKitLoadStatus status = webkit_web_view_get_load_status(WEBKIT_WEB_VIEW(object));

    switch(status) {
    case WEBKIT_LOAD_PROVISIONAL:
        g_assert(!fixture->has_been_provisional);
        g_assert(!fixture->has_been_failed);
        fixture->has_been_provisional = TRUE;
        break;
    case WEBKIT_LOAD_COMMITTED:
        g_idle_add (stop_load, object);
        break;
    case WEBKIT_LOAD_FAILED:
        g_assert(fixture->has_been_provisional);
        g_assert(!fixture->has_been_failed);
        g_assert(!fixture->has_been_load_error);
        fixture->has_been_failed = TRUE;
        g_main_loop_quit(fixture->loop);
        break;
    case WEBKIT_LOAD_FINISHED:
        g_assert_not_reached();
        break;
    default:
        break;
    }
}

static void test_loading_cancelled(WebLoadingFixture* fixture, gconstpointer data)
{
    char* uri_string;

    g_test_bug("29644");

    g_signal_connect(fixture->webView, "load-error", G_CALLBACK(load_cancelled_cb), fixture);
    g_signal_connect(fixture->webView, "notify::load-status", G_CALLBACK(load_cancelled_status_changed_cb), fixture);

    uri_string = get_uri_for_path("/test_loading_cancelled");
    webkit_web_view_load_uri(fixture->webView, uri_string);
    g_free(uri_string);

    g_main_loop_run(fixture->loop);
}

static void load_goback_status_changed_cb(GObject* object, GParamSpec* pspec, WebLoadingFixture* fixture)
{
    WebKitLoadStatus status = webkit_web_view_get_load_status(WEBKIT_WEB_VIEW(object));

    switch(status) {
    case WEBKIT_LOAD_PROVISIONAL:
        g_assert(!fixture->has_been_provisional);
        fixture->has_been_provisional = TRUE;
        break;
    case WEBKIT_LOAD_COMMITTED:
        g_assert(fixture->has_been_provisional);
        fixture->has_been_committed = TRUE;
        break;
    case WEBKIT_LOAD_FAILED:
        g_assert_not_reached();
        break;
    case WEBKIT_LOAD_FINISHED:
        g_assert(fixture->has_been_provisional);
        g_assert(fixture->has_been_committed);
        fixture->has_been_finished = TRUE;
        g_main_loop_quit(fixture->loop);
        break;
    default:
        break;
    }
}

static void load_wentback_status_changed_cb(GObject* object, GParamSpec* pspec, WebLoadingFixture* fixture)
{
    WebKitLoadStatus status = webkit_web_view_get_load_status(WEBKIT_WEB_VIEW(object));
    char* uri_string;
    char* uri_string2;

    uri_string = get_uri_for_path("/test_loading_status");
    uri_string2 = get_uri_for_path("/test_loading_status2");

    switch(status) {
    case WEBKIT_LOAD_PROVISIONAL:
        g_assert_cmpstr(webkit_web_view_get_uri(fixture->webView), ==, uri_string2);
        break;
    case WEBKIT_LOAD_COMMITTED:
        g_assert_cmpstr(webkit_web_view_get_uri(fixture->webView), ==, uri_string);
        break;
    case WEBKIT_LOAD_FAILED:
        g_assert_not_reached();
        break;
    case WEBKIT_LOAD_FINISHED:
        g_assert_cmpstr(webkit_web_view_get_uri(fixture->webView), ==, uri_string);
        g_main_loop_quit(fixture->loop);
        break;
    default:
        break;
    }

    g_free(uri_string);
    g_free(uri_string2);
}

static void load_error_test(WebKitWebView* webview, WebKitWebFrame* frame, const char* uri, GError* error)
{
    g_debug("Error: %s", error->message);
}

static void test_loading_goback(WebLoadingFixture* fixture, gconstpointer data)
{
    char* uri_string;

    g_signal_connect(fixture->webView, "notify::load-status", G_CALLBACK(load_goback_status_changed_cb), fixture);

    g_signal_connect(fixture->webView, "load-error", G_CALLBACK(load_error_test), fixture);

    uri_string = get_uri_for_path("/test_loading_status");
    webkit_web_view_load_uri(fixture->webView, uri_string);
    g_free(uri_string);

    g_main_loop_run(fixture->loop);

    fixture->has_been_provisional = FALSE;
    fixture->has_been_committed = FALSE;
    fixture->has_been_first_visually_non_empty_layout = FALSE;
    fixture->has_been_finished = FALSE;
    fixture->has_been_failed = FALSE;
    fixture->has_been_load_error = FALSE;

    uri_string = get_uri_for_path("/test_loading_status2");
    webkit_web_view_load_uri(fixture->webView, uri_string);
    g_free(uri_string);

    g_main_loop_run(fixture->loop);

    g_signal_handlers_disconnect_by_func(fixture->webView, load_goback_status_changed_cb, fixture);

    fixture->has_been_provisional = FALSE;
    fixture->has_been_committed = FALSE;
    fixture->has_been_first_visually_non_empty_layout = FALSE;
    fixture->has_been_finished = FALSE;
    fixture->has_been_failed = FALSE;
    fixture->has_been_load_error = FALSE;

    g_signal_connect(fixture->webView, "notify::load-status", G_CALLBACK(load_wentback_status_changed_cb), fixture);
    webkit_web_view_go_back(fixture->webView);

    g_main_loop_run(fixture->loop);

    g_signal_handlers_disconnect_by_func(fixture->webView, load_wentback_status_changed_cb, fixture);
}

int main(int argc, char** argv)
{
    SoupServer* server;

    gtk_test_init(&argc, &argv, NULL);

    server = soup_server_new(SOUP_SERVER_PORT, 0, NULL);
    soup_server_run_async(server);

    soup_server_add_handler(server, NULL, server_callback, NULL, NULL);

    base_uri = soup_uri_new("http://127.0.0.1/");
    soup_uri_set_port(base_uri, soup_server_get_port(server));

    g_test_bug_base("https://bugs.webkit.org/");
    g_test_add("/webkit/loading/status",
               WebLoadingFixture, NULL,
               web_loading_fixture_setup,
               test_loading_status,
               web_loading_fixture_teardown);
    g_test_add("/webkit/loading/error",
               WebLoadingFixture, NULL,
               web_loading_fixture_setup,
               test_loading_error,
               web_loading_fixture_teardown);
    g_test_add("/webkit/loading/cancelled",
               WebLoadingFixture, NULL,
               web_loading_fixture_setup,
               test_loading_cancelled,
               web_loading_fixture_teardown);
    g_test_add("/webkit/loading/goback",
               WebLoadingFixture, NULL,
               web_loading_fixture_setup,
               test_loading_goback,
               web_loading_fixture_teardown);
    return g_test_run();
}

