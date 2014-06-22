/*
 * Copyright (C) 2009 Jan Michael Alonzo
 * Copyright (C) 2009 Gustavo Noronha Silva
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

#include <glib.h>
#include <glib/gstdio.h>
#include <libsoup/soup.h>
#include <string.h>
#include <webkit/webkit.h>
#include <unistd.h>

GMainLoop* loop;
SoupSession *session;
char* base_uri;

/* For real request testing */
static void
server_callback(SoupServer *server, SoupMessage *msg,
                 const char *path, GHashTable *query,
                 SoupClientContext *context, gpointer data)
{
    if (msg->method != SOUP_METHOD_GET) {
        soup_message_set_status(msg, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    soup_message_set_status(msg, SOUP_STATUS_OK);

    /* PDF */
    if (g_str_equal(path, "/pdf")) {
        char* contents;
        gsize length;
        GError* error = NULL;

        g_file_get_contents("test.pdf", &contents, &length, &error);
        g_assert(!error);

        soup_message_body_append(msg->response_body, SOUP_MEMORY_TAKE, contents, length);
    } else if (g_str_equal(path, "/html")) {
        char* contents;
        gsize length;
        GError* error = NULL;

        g_file_get_contents("test.html", &contents, &length, &error);
        g_assert(!error);

        soup_message_body_append(msg->response_body, SOUP_MEMORY_TAKE, contents, length);
    } else if (g_str_equal(path, "/text")) {
        char* contents;
        gsize length;
        GError* error = NULL;

        soup_message_headers_append(msg->response_headers, "Content-Disposition", "attachment; filename=test.txt");

        g_file_get_contents("test.txt", &contents, &length, &error);
        g_assert(!error);

        soup_message_body_append(msg->response_body, SOUP_MEMORY_TAKE, contents, length);
    }

    soup_message_body_complete(msg->response_body);
}

static void idle_quit_loop_cb(WebKitWebView* web_view, GParamSpec* pspec, gpointer data)
{
    if (webkit_web_view_get_load_status(web_view) == WEBKIT_LOAD_FINISHED ||
        webkit_web_view_get_load_status(web_view) == WEBKIT_LOAD_FAILED)
        g_main_loop_quit(loop);
}

static gboolean mime_type_policy_decision_requested_cb(WebKitWebView* view, WebKitWebFrame* frame,
                                                       WebKitNetworkRequest* request, const char* mime_type,
                                                       WebKitWebPolicyDecision* decision, gpointer data)
{
    char* type = (char*)data;

    if (g_str_equal(type, "pdf")) {
        g_assert_cmpstr(mime_type, ==, "application/pdf");
        g_assert(!webkit_web_view_can_show_mime_type(view, mime_type));
    } else if (g_str_equal(type, "html")) {
        g_assert_cmpstr(mime_type, ==, "text/html");
        g_assert(webkit_web_view_can_show_mime_type(view, mime_type));
    } else if (g_str_equal(type, "text")) {
        WebKitNetworkResponse* response = webkit_web_frame_get_network_response(frame);
        SoupMessage* message = webkit_network_response_get_message(response);
        char* disposition;

        g_assert(message);
        soup_message_headers_get_content_disposition(message->response_headers,
                                                     &disposition, NULL);
        g_object_unref(response);

        g_assert_cmpstr(disposition, ==, "attachment");
        g_free(disposition);

        g_assert_cmpstr(mime_type, ==, "text/plain");
        g_assert(webkit_web_view_can_show_mime_type(view, mime_type));
    }

    g_free(type);

    return FALSE;
}

static void testRemoteMimeType(const void* data)
{
    const char* name = (const char*) data;
    WebKitWebView* view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(G_OBJECT(view));

    loop = g_main_loop_new(NULL, TRUE);

    g_object_connect(G_OBJECT(view),
                     "signal::notify::load-status", idle_quit_loop_cb, NULL,
                     "signal::mime-type-policy-decision-requested", mime_type_policy_decision_requested_cb, g_strdup(name),
                     NULL);

    char* effective_uri = g_strdup_printf("%s%s", base_uri, name);
    webkit_web_view_load_uri(view, effective_uri);
    g_free(effective_uri);

    g_main_loop_run(loop);

    g_object_unref(view);
}

static void testLocalMimeType(const void* data)
{
     const char* typeName = (const char*) data;
     WebKitWebView* view = WEBKIT_WEB_VIEW(webkit_web_view_new());
     g_object_ref_sink(G_OBJECT(view));

     loop = g_main_loop_new(NULL, TRUE);

     g_object_connect(G_OBJECT(view),
                      "signal::notify::load-status", idle_quit_loop_cb, NULL,
                      "signal::mime-type-policy-decision-requested", mime_type_policy_decision_requested_cb, g_strdup(typeName),
                      NULL);

     gchar* filename = g_strdup_printf("test.%s", typeName);
     GFile* file = g_file_new_for_path(filename);
     g_free(filename);

     gchar* fileURI = g_file_get_uri(file);
     g_object_unref(file);

     webkit_web_view_load_uri(view, fileURI);
     g_free(fileURI);
 
     g_main_loop_run(loop);
     g_object_unref(view);
}

int main(int argc, char** argv)
{
    SoupServer* server;
    SoupURI* soup_uri;

    gtk_test_init(&argc, &argv, NULL);

    /* Hopefully make test independent of the path it's called from. */
    testutils_relative_chdir("Source/WebKit/gtk/tests/resources/test.html", argv[0]);

    server = soup_server_new(SOUP_SERVER_PORT, 0, NULL);
    soup_server_run_async(server);

    soup_server_add_handler(server, NULL, server_callback, NULL, NULL);

    soup_uri = soup_uri_new("http://127.0.0.1/");
    soup_uri_set_port(soup_uri, soup_server_get_port(server));

    base_uri = soup_uri_to_string(soup_uri, FALSE);
    soup_uri_free(soup_uri);

    g_test_bug_base("https://bugs.webkit.org/");
    g_test_add_data_func("/webkit/mime/remote-PDF", "pdf", testRemoteMimeType);
    g_test_add_data_func("/webkit/mime/remote-HTML", "html", testRemoteMimeType);
    g_test_add_data_func("/webkit/mime/remote-TEXT", "text", testRemoteMimeType);
    g_test_add_data_func("/webkit/mime/local-PDF", "pdf", testLocalMimeType);
    g_test_add_data_func("/webkit/mime/local-HTML", "html", testLocalMimeType);
    g_test_add_data_func("/webkit/mime/local-TEXT", "text", testLocalMimeType);

    return g_test_run();
}
