/*
 * Copyright (C) 2009 Jan Michael Alonzo
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
#include <glib.h>
#include <gtk/gtk.h>
#include <libsoup/soup.h>
#include <string.h>
#include <webkit/webkit.h>

#define INDEX_HTML "<html></html>"
#define MAIN_HTML "<html><head><script language=\"javascript\" src=\"/javascript.js\"></script></head><body><h1>hah</h1></html>"
#define JAVASCRIPT "function blah () { var a = 1; }"

GMainLoop* loop;
SoupSession *session;
char *base_uri;
WebKitWebResource* main_resource;
WebKitWebResource* sub_resource;

typedef struct {
    WebKitWebResource* webResource;
    WebKitWebView* webView;
} WebResourceFixture;

/* For real request testing */
static void
server_callback (SoupServer *server, SoupMessage *msg,
                 const char *path, GHashTable *query,
                 SoupClientContext *context, gpointer data)
{
    if (msg->method != SOUP_METHOD_GET) {
        soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    soup_message_set_status (msg, SOUP_STATUS_OK);

    /* Redirect */
    if (g_str_equal (path, "/")) {
        soup_message_set_status (msg, SOUP_STATUS_MOVED_PERMANENTLY);

        soup_message_headers_append (msg->response_headers,
                                     "Location", "/index.html");
    } else if (g_str_equal (path, "/index.html")) {
        soup_message_body_append (msg->response_body,
                                  SOUP_MEMORY_COPY,
                                  INDEX_HTML,
                                  strlen (INDEX_HTML));
    } else if (g_str_equal (path, "/main.html")) {
        soup_message_body_append (msg->response_body,
                                  SOUP_MEMORY_COPY,
                                  MAIN_HTML,
                                  strlen (MAIN_HTML));
    } else if (g_str_equal (path, "/javascript.js")) {
        soup_message_body_append (msg->response_body,
                                  SOUP_MEMORY_COPY,
                                  JAVASCRIPT,
                                  strlen (JAVASCRIPT));
    }


    soup_message_body_complete (msg->response_body);
}

static void web_resource_fixture_setup(WebResourceFixture* fixture, gconstpointer data)
{
    fixture->webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(fixture->webView);
    const gchar* webData = "<html></html>";
    fixture->webResource = webkit_web_resource_new(webData, strlen(webData), "http://example.com/", "text/html", "utf8", "Example.com");
    g_assert(fixture->webResource);
}

static void web_resource_fixture_teardown(WebResourceFixture* fixture, gconstpointer data)
{
    g_assert(fixture->webResource);
    g_object_unref(fixture->webResource);
    g_object_unref(fixture->webView);
}

static void test_webkit_web_resource_get_url(WebResourceFixture* fixture, gconstpointer data)
{
    gchar* url;
    g_object_get(G_OBJECT(fixture->webResource), "uri", &url, NULL);
    g_assert_cmpstr(url, ==, "http://example.com/");
    g_assert_cmpstr(webkit_web_resource_get_uri(fixture->webResource) ,==,"http://example.com/");
    g_free(url);
}

static void test_webkit_web_resource_get_data(WebResourceFixture* fixture, gconstpointer data)
{
    GString* charData = webkit_web_resource_get_data(fixture->webResource);
    g_assert_cmpstr(charData->str, ==, "<html></html>");
}

static void test_webkit_web_resource_get_mime_type(WebResourceFixture* fixture, gconstpointer data)
{
    gchar* mime_type;
    g_object_get(G_OBJECT(fixture->webResource), "mime-type", &mime_type, NULL);
    g_assert_cmpstr(mime_type, ==, "text/html");
    g_assert_cmpstr(webkit_web_resource_get_mime_type(fixture->webResource),==,"text/html");
    g_free(mime_type);
}

static void test_webkit_web_resource_get_encoding(WebResourceFixture* fixture, gconstpointer data)
{
    gchar* text_encoding;
    g_object_get(G_OBJECT(fixture->webResource), "encoding", &text_encoding, NULL);
    g_assert_cmpstr(text_encoding, ==, "utf8");
    g_assert_cmpstr(webkit_web_resource_get_encoding(fixture->webResource),==,"utf8");
    g_free(text_encoding);
}

static void test_webkit_web_resource_get_frame_name(WebResourceFixture* fixture, gconstpointer data)
{
    gchar* frame_name;
    g_object_get(G_OBJECT(fixture->webResource), "frame-name", &frame_name, NULL);
    g_assert_cmpstr(frame_name, ==, "Example.com");
    g_assert_cmpstr(webkit_web_resource_get_frame_name(fixture->webResource),==,"Example.com");
    g_free(frame_name);
}

static void resource_request_starting_cb(WebKitWebView* web_view, WebKitWebFrame* web_frame, WebKitWebResource* web_resource, WebKitNetworkRequest* request, WebKitNetworkResponse* response, gpointer data)
{
    gint* been_there = data;
    *been_there = *been_there + 1;

    if (*been_there == 1) {
        g_assert(!main_resource);
        main_resource = g_object_ref(web_resource);

        g_assert_cmpstr(webkit_web_resource_get_uri(web_resource), ==, base_uri);

        /* This should be a redirect, so the response must be NULL */
        g_assert(!response);
    } else if (*been_there == 2) {
        char* uri = g_strdup_printf("%sindex.html", base_uri);

        g_assert_cmpstr(webkit_web_resource_get_uri(web_resource), ==, uri);

        /* Cancel the request. */
        webkit_network_request_set_uri(request, "about:blank");

        g_free(uri);
    }
}

static void notify_load_status_cb(WebKitWebView* web_view, GParamSpec* pspec, gpointer data)
{
    if (webkit_web_view_get_load_status(web_view) == WEBKIT_LOAD_FINISHED) {
        gboolean* been_there = data;
        *been_there = TRUE;

        g_assert_cmpstr(webkit_web_view_get_uri(web_view), ==, "about:blank");

        g_main_loop_quit(loop);
    }
}

static void test_web_resource_loading()
{
    WebKitWebView* web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    gint been_to_resource_request_starting = 0;
    gboolean been_to_load_finished = FALSE;
    WebKitWebFrame* web_frame;
    WebKitWebDataSource* data_source;

    loop = g_main_loop_new(NULL, TRUE);

    g_object_ref_sink(web_view);

    g_signal_connect(web_view, "resource-request-starting",
                     G_CALLBACK(resource_request_starting_cb),
                     &been_to_resource_request_starting);

    g_signal_connect(web_view, "notify::load-status",
                     G_CALLBACK(notify_load_status_cb),
                     &been_to_load_finished);

    webkit_web_view_load_uri(web_view, base_uri);

    /* We won't get finished immediately, because of the redirect */
    g_main_loop_run(loop);
    
    web_frame = webkit_web_view_get_main_frame(web_view);
    data_source = webkit_web_frame_get_data_source(web_frame);

    g_assert(main_resource);
    g_assert(webkit_web_data_source_get_main_resource(data_source) == main_resource);
    g_object_unref(main_resource);
    
    g_assert_cmpint(been_to_resource_request_starting, ==, 2);
    g_assert_cmpint(been_to_load_finished, ==, TRUE);

    g_object_unref(web_view);
    g_main_loop_unref(loop);
}

static void resource_request_starting_sub_cb(WebKitWebView* web_view, WebKitWebFrame* web_frame, WebKitWebResource* web_resource, WebKitNetworkRequest* request, WebKitNetworkResponse* response, gpointer data)
{
    if (!main_resource)
        main_resource = g_object_ref(web_resource);
    else if (!sub_resource)
      sub_resource = g_object_ref(web_resource);
}

static void notify_load_status_sub_cb(WebKitWebView* web_view, GParamSpec* pspec, gpointer data)
{
    if (webkit_web_view_get_load_status(web_view) == WEBKIT_LOAD_FINISHED)
        g_main_loop_quit(loop);
}

static gboolean idle_quit_loop_cb(gpointer data)
{
    g_main_loop_quit(loop);
    return FALSE;
}

static void test_web_resource_sub_resource_loading()
{
    WebKitWebView* web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    WebKitWebFrame* web_frame;
    WebKitWebDataSource* data_source;
    GList* sub_resources;
    char* uri = g_strdup_printf("%smain.html", base_uri);

    main_resource = NULL;
    
    loop = g_main_loop_new(NULL, TRUE);

    g_object_ref_sink(web_view);

    g_signal_connect(web_view, "resource-request-starting",
                     G_CALLBACK(resource_request_starting_sub_cb),
                     NULL);

    g_signal_connect(web_view, "notify::load-status",
                     G_CALLBACK(notify_load_status_sub_cb),
                     NULL);

    webkit_web_view_load_uri(web_view, uri);

    g_main_loop_run(loop);

    /* The main resource should be loaded; now let's wait for the sub-resource to load */
    g_idle_add(idle_quit_loop_cb, NULL);
    g_main_loop_run(loop);
    
    g_assert(main_resource && sub_resource);
    g_assert(main_resource != sub_resource);

    web_frame = webkit_web_view_get_main_frame(web_view);
    data_source = webkit_web_frame_get_data_source(web_frame);

    g_assert(webkit_web_data_source_get_main_resource(data_source) == main_resource);
    g_object_unref(main_resource);

    sub_resources = webkit_web_data_source_get_subresources(data_source);
    // Expected resources: javascripts.js, favicon.ico
    g_assert(sub_resources);
    g_assert(sub_resources->next);
    g_assert(!sub_resources->next->next);

    // Test that the object we got from the data source is the same
    // that went through resource-request-starting. Note that the order is
    // not important (and not guaranteed since the resources are stored in a
    // hashtable).
    g_assert(WEBKIT_WEB_RESOURCE(sub_resources->data) == sub_resource
             || WEBKIT_WEB_RESOURCE(sub_resources->next->data) == sub_resource);

    g_object_unref(web_view);
    g_main_loop_unref(loop);
}

int main(int argc, char** argv)
{
    SoupServer* server;
    SoupURI* soup_uri;

    gtk_test_init(&argc, &argv, NULL);

    server = soup_server_new(SOUP_SERVER_PORT, 0, NULL);
    soup_server_run_async(server);

    soup_server_add_handler(server, NULL, server_callback, NULL, NULL);

    soup_uri = soup_uri_new("http://127.0.0.1/");
    soup_uri_set_port(soup_uri, soup_server_get_port(server));

    base_uri = soup_uri_to_string(soup_uri, FALSE);
    soup_uri_free(soup_uri);

    g_test_bug_base("https://bugs.webkit.org/");
    g_test_add("/webkit/webresource/get_url",
               WebResourceFixture, 0, web_resource_fixture_setup,
               test_webkit_web_resource_get_url, web_resource_fixture_teardown);
    g_test_add("/webkit/webresource/get_mime_type",
               WebResourceFixture, 0, web_resource_fixture_setup,
               test_webkit_web_resource_get_mime_type, web_resource_fixture_teardown);
    g_test_add("/webkit/webresource/get_text_encoding_name",
               WebResourceFixture, 0, web_resource_fixture_setup,
               test_webkit_web_resource_get_encoding, web_resource_fixture_teardown);
    g_test_add("/webkit/webresource/get_frame_name",
               WebResourceFixture, 0, web_resource_fixture_setup,
               test_webkit_web_resource_get_frame_name, web_resource_fixture_teardown);
    g_test_add("/webkit/webresource/get_data",
               WebResourceFixture, 0, web_resource_fixture_setup,
               test_webkit_web_resource_get_data, web_resource_fixture_teardown);

    g_test_add_func("/webkit/webresource/loading", test_web_resource_loading);
    g_test_add_func("/webkit/webresource/sub_resource_loading", test_web_resource_sub_resource_loading);

    return g_test_run ();
}
