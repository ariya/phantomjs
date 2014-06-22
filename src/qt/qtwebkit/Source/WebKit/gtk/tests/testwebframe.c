/*
 * Copyright (C) 2008 Holger Hans Peter Freyther
 * Copyright (C) 2009 Collabora Ltd.
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
#include <errno.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>

static int numberOfFramesCreated = 0;

static void createFrameSignalTestFrameCreatedCallback(WebKitWebView* webView, WebKitWebFrame* frame, gpointer data)
{
    numberOfFramesCreated++;
}

static gboolean createFrameSignalTestTimeout(gpointer data)
{
    g_assert_cmpint(numberOfFramesCreated, ==, 2);
    g_main_loop_quit((GMainLoop*) data);
    return FALSE;
}

static void test_webkit_web_frame_created_signal(void)
{
    GtkWidget* webView;
    GtkWidget* window;
    GMainLoop* loop = g_main_loop_new(NULL, TRUE);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    webView = webkit_web_view_new();
    g_signal_connect(webView, "frame-created", G_CALLBACK(createFrameSignalTestFrameCreatedCallback), loop);

    // We want to ensure that exactly two create-frame signals are
    // fired and no more, so we set a timeout here. There does not appear
    // to be a simple way via the API to figure out when all frames have
    // loaded.
    g_timeout_add(500, createFrameSignalTestTimeout, loop);

    gtk_container_add(GTK_CONTAINER(window), webView);
    gtk_widget_show(window);
    gtk_widget_show(webView);

    webkit_web_view_load_string(WEBKIT_WEB_VIEW(webView),
        "<html><body>Frames!"
        "<iframe></iframe>"
        "<iframe></iframe>"
        "</body></html>",
        "text/html", "utf-8", "file://");
    g_main_loop_run(loop);
}

static void test_webkit_web_frame_create_destroy(void)
{
    GtkWidget *webView;
    GtkWidget *window;

    g_test_bug("21837");
    webView = webkit_web_view_new();
    g_object_ref_sink(webView);
    g_assert_cmpint(G_OBJECT(webView)->ref_count, ==, 1);
    // This crashed with the original version
    g_object_unref(webView);

    g_test_bug("25042");
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    webView = webkit_web_view_new();
    gtk_container_add(GTK_CONTAINER(window), webView);
    gtk_widget_show(window);
    gtk_widget_show(webView);
    gtk_widget_destroy(webView);
}

static void test_webkit_web_frame_lifetime(void)
{
    WebKitWebView* webView;
    WebKitWebFrame* webFrame;
    g_test_bug("21837");

    webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    g_assert_cmpint(G_OBJECT(webView)->ref_count, ==, 1);
    webFrame = webkit_web_view_get_main_frame(webView);
    g_assert_cmpint(G_OBJECT(webFrame)->ref_count, ==, 1);

    // Add dummy reference on the WebKitWebFrame to keep it alive
    g_object_ref(webFrame);
    g_assert_cmpint(G_OBJECT(webFrame)->ref_count, ==, 2);

    // This crashed with the original version
    g_object_unref(webView);

    // Make sure that the frame got deleted as well. We did this
    // by adding an extra ref on the WebKitWebFrame and we should
    // be the one holding the last reference.
    g_assert_cmpint(G_OBJECT(webFrame)->ref_count, ==, 1);
    g_object_unref(webFrame);
}

static gboolean print_requested_cb(WebKitWebView* webView, WebKitWebFrame* webFrame, GMainLoop* loop)
{
    g_object_set_data(G_OBJECT(webView), "signal-handled", GINT_TO_POINTER(TRUE));
    g_main_loop_quit(loop);
    return TRUE;
}

static void print_timeout(GMainLoop* loop)
{
    if (g_main_loop_is_running(loop))
        g_main_loop_quit(loop);
}

static void test_webkit_web_frame_printing(void)
{
    WebKitWebView* webView;

    webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(webView);
    g_assert_cmpint(G_OBJECT(webView)->ref_count, ==, 1);

    webkit_web_view_load_string(webView,
                                "<html><body><h1>WebKitGTK+!</h1></body></html>",
                                "text/html",
                                "utf-8",
                                "file://");

    GMainLoop* loop = g_main_loop_new(NULL, TRUE);

    // Does javascript print() work correctly?
    g_signal_connect(webView, "print-requested",
                     G_CALLBACK(print_requested_cb),
                     loop);

    g_object_set_data(G_OBJECT(webView), "signal-handled", GINT_TO_POINTER(FALSE));
    webkit_web_view_execute_script (webView, "print();");

    // Give javascriptcore some time to process the print request, but
    // prepare a timeout to avoid it running forever in case the signal is
    // never emitted.
    g_timeout_add(1000, (GSourceFunc)print_timeout, loop);
    g_main_loop_run(loop);

    g_assert_cmpint(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(webView), "signal-handled")), ==, TRUE);

    // Does printing directly to a file?
    GError *error = NULL;
    gchar* temporaryFilename = NULL;
    gint fd = g_file_open_tmp ("webkit-testwebframe-XXXXXX", &temporaryFilename, &error);
    close(fd);

    if (error) {
        g_critical("Failed to open a temporary file for writing: %s.", error->message);
        g_error_free(error);
        goto cleanup;
    }

    // We delete the file, so that we can easily figure out that the
    // file got printed;
    if (g_unlink(temporaryFilename) == -1) {
        g_warning("Failed to delete the temporary file: %s.\nThis may cause the test to be bogus.", g_strerror(errno));
    }

    WebKitWebFrame* webFrame = webkit_web_view_get_main_frame(webView);
    GtkPrintOperation* operation = gtk_print_operation_new();
    GtkPrintOperationAction action = GTK_PRINT_OPERATION_ACTION_EXPORT;
    GtkPrintOperationResult result;

    gtk_print_operation_set_export_filename(operation, temporaryFilename);
    result = webkit_web_frame_print_full (webFrame, operation, action, NULL);

    g_assert_cmpint(result, ==, GTK_PRINT_OPERATION_RESULT_APPLY);
    g_assert_cmpint(g_file_test(temporaryFilename, G_FILE_TEST_IS_REGULAR), ==, TRUE);

    g_unlink(temporaryFilename);
    g_object_unref(operation);
cleanup:
    g_object_unref(webView);
    g_free(temporaryFilename);
}

static void test_webkit_web_frame_response()
{
    WebKitWebFrame* frame = g_object_new(WEBKIT_TYPE_WEB_FRAME, NULL);
    WebKitNetworkResponse* response = webkit_web_frame_get_network_response(frame);
    g_assert(!response);
    g_object_unref(frame);
}

int main(int argc, char** argv)
{
    gtk_test_init(&argc, &argv, NULL);

    g_test_bug_base("https://bugs.webkit.org/");
    g_test_add_func("/webkit/webview/create_destroy", test_webkit_web_frame_create_destroy);
    g_test_add_func("/webkit/webview/frame-created_signal", test_webkit_web_frame_created_signal);
    g_test_add_func("/webkit/webframe/lifetime", test_webkit_web_frame_lifetime);
    g_test_add_func("/webkit/webview/printing", test_webkit_web_frame_printing);
    g_test_add_func("/webkit/webview/response", test_webkit_web_frame_response);
    return g_test_run ();
}
