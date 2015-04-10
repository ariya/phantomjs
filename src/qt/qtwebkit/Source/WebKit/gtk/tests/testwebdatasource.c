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
#include <webkit/webkit.h>

static const gshort defaultTimeout = 10;
guint waitTimer;
gboolean shouldWait;

typedef struct {
    WebKitWebView* webView;
    WebKitWebFrame* mainFrame;
} WebDataSourceFixture;

static void test_webkit_web_data_source_get_initial_request()
{
    WebKitWebView* view;
    WebKitWebFrame* frame;
    WebKitWebDataSource* dataSource;
    WebKitNetworkRequest* initialRequest;

    view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(view);
    frame = webkit_web_view_get_main_frame(view);

    WebKitNetworkRequest* request = webkit_network_request_new("http://www.google.com");
    webkit_web_frame_load_request(frame, request);
    g_object_unref(request);

    dataSource = webkit_web_frame_get_provisional_data_source(frame);
    g_assert(dataSource);
    initialRequest = webkit_web_data_source_get_initial_request(dataSource);
    g_assert_cmpstr(webkit_network_request_get_uri(initialRequest), ==, "http://www.google.com/");

    g_object_unref(view);
}

static void notify_load_status_unreachable_cb(WebKitWebView* view, GParamSpec* pspec, GMainLoop* loop)
{
    WebKitLoadStatus status = webkit_web_view_get_load_status (view);
    WebKitWebFrame* frame = webkit_web_view_get_main_frame(view);

    g_assert(status != WEBKIT_LOAD_FINISHED);

    if (status != WEBKIT_LOAD_FAILED)
        return;

    WebKitWebDataSource* datasource = webkit_web_frame_get_data_source(frame);

    g_assert_cmpstr("http://this.host.does.not.exist/doireallyexist.html", ==,
                    webkit_web_data_source_get_unreachable_uri(datasource));

    g_main_loop_quit(loop);
}

static void notify_load_status_cb(WebKitWebView* view, GParamSpec* pspec, GMainLoop* loop)
{
    WebKitLoadStatus status = webkit_web_view_get_load_status (view);
    WebKitWebFrame* frame = webkit_web_view_get_main_frame(view);
    WebKitWebDataSource* dataSource = webkit_web_frame_get_data_source(frame);

    if (status == WEBKIT_LOAD_COMMITTED) {
        g_assert(webkit_web_data_source_is_loading(dataSource));
        return;
    }
    else if (status != WEBKIT_LOAD_FINISHED)
        return;

    /* Test get_request */
    g_test_message("Testing webkit_web_data_source_get_request");
    WebKitNetworkRequest* request = webkit_web_data_source_get_request(dataSource);
    g_assert_cmpstr(webkit_network_request_get_uri(request), ==, "http://www.webkit.org/");

    /* Test get_main_resource */
    g_test_message("Testing webkit_web_data_source_get_main_resource");
    WebKitWebResource* resource = webkit_web_data_source_get_main_resource(dataSource);
    g_assert_cmpstr("text/html", ==, webkit_web_resource_get_mime_type(resource));
    g_assert_cmpstr("http://www.webkit.org/", ==, webkit_web_resource_get_uri(resource));

    /* Test get_data. We just test if data has certain size for the mean time */
    g_test_message("Testing webkit_web_data_source_get_data has certain size");
    GString* data = webkit_web_data_source_get_data(dataSource);
    g_assert(data->len > 100);

    /* FIXME: Add test for get_encoding */

    g_main_loop_quit(loop);
}

static gboolean wait_timer_fired(GMainLoop* loop)
{
    waitTimer = 0;
    g_main_loop_quit(loop);

    return FALSE;
}

static void test_webkit_web_data_source()
{
    WebKitWebView* view;
    GMainLoop* loop;

    view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(view);
    loop = g_main_loop_new(NULL, TRUE);
    g_signal_connect(view, "notify::load-status", G_CALLBACK(notify_load_status_cb), loop);
    webkit_web_view_load_uri(view, "http://www.webkit.org");

    waitTimer = g_timeout_add_seconds(defaultTimeout, (GSourceFunc)wait_timer_fired, loop);

    g_main_loop_run(loop);

    if (waitTimer)
        g_source_remove(waitTimer);

    waitTimer = 0;

    g_main_loop_unref(loop);
    g_object_unref(view);
}

static void notify_load_status_lifetime_cb(WebKitWebView* view, GParamSpec* pspec, GMainLoop* loop)
{
    WebKitLoadStatus status = webkit_web_view_get_load_status (view);
    WebKitWebFrame* frame = webkit_web_view_get_main_frame(view);
    WebKitWebDataSource* dataSource = webkit_web_frame_get_data_source(frame);

    if (status == WEBKIT_LOAD_COMMITTED) {
        g_assert(webkit_web_data_source_is_loading(dataSource));
        return;
    } else if (status != WEBKIT_LOAD_FINISHED)
        return;

    g_main_loop_quit(loop);
}

static void test_webkit_web_data_source_lifetime()
{
    WebKitWebView* view;
    GMainLoop* loop;

    view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(view);
    loop = g_main_loop_new(NULL, TRUE);
    g_signal_connect(view, "notify::load-status", G_CALLBACK(notify_load_status_lifetime_cb), loop);
    webkit_web_view_load_uri(view, "http://www.webkit.org");

    waitTimer = g_timeout_add_seconds(defaultTimeout, (GSourceFunc)wait_timer_fired, loop);

    g_main_loop_run(loop);

    WebKitWebDataSource* dataSource = webkit_web_frame_get_data_source(webkit_web_view_get_main_frame(view));
    GList* subResources = webkit_web_data_source_get_subresources(dataSource);
    gint numberOfResources = g_list_length(subResources);
    g_list_free(subResources);

    g_assert_cmpint(webkit_web_view_get_load_status(view), ==, WEBKIT_LOAD_FINISHED);

    webkit_web_view_load_uri(view, "http://gnome.org");

    g_assert_cmpint(webkit_web_view_get_load_status(view), ==, WEBKIT_LOAD_PROVISIONAL);

    webkit_web_view_stop_loading(view);

    g_assert_cmpint(webkit_web_view_get_load_status(view), ==, WEBKIT_LOAD_FAILED);

    subResources = webkit_web_data_source_get_subresources(dataSource);
    g_assert_cmpint(numberOfResources, ==, g_list_length(subResources));
    g_list_free(subResources);

    if (waitTimer)
        g_source_remove(waitTimer);

    waitTimer = 0;

    g_main_loop_unref(loop);
    g_object_unref(view);
}

static void test_webkit_web_data_source_unreachable_uri()
{
    /* FIXME: this test fails currently. */
    return;

    WebKitWebView* view;
    GMainLoop* loop;

    view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(view);
    loop = g_main_loop_new(NULL, TRUE);
    g_signal_connect(view, "notify::load-status", G_CALLBACK(notify_load_status_unreachable_cb), loop);
    webkit_web_view_load_uri(view, "http://this.host.does.not.exist/doireallyexist.html");

    waitTimer = g_timeout_add_seconds(defaultTimeout, (GSourceFunc)wait_timer_fired, loop);

    g_main_loop_run(loop);

    if (waitTimer)
        g_source_remove(waitTimer);

    waitTimer = 0;

    g_main_loop_unref(loop);
    g_object_unref(view);
}

int main(int argc, char** argv)
{
    gtk_test_init(&argc, &argv, NULL);

    g_test_bug_base("https://bugs.webkit.org/");
    g_test_bug("24758");
    g_test_add_func("/webkit/webdatasource/get_initial_request",
                    test_webkit_web_data_source_get_initial_request);
    g_test_add_func("/webkit/webdatasource/api",
                    test_webkit_web_data_source);
    g_test_add_func("/webkit/webdatasource/unreachable_uri",
                    test_webkit_web_data_source_unreachable_uri);
    g_test_add_func("/webkit/webdatasource/lifetime",
                    test_webkit_web_data_source_lifetime);

    return g_test_run ();
}
