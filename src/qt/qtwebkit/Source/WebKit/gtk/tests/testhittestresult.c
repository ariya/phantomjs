/*
 * Copyright (C) 2009 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2,1 of the License, or (at your option) any later version.
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
#include <glib/gstdio.h>
#include <webkit/webkit.h>

typedef struct {
  char* data;
  guint flag;
} TestInfo;

static GMainLoop* loop;

typedef struct {
    WebKitWebView* webView;
    TestInfo* info;
} HitTestResultFixture;

TestInfo*
test_info_new(const char* data, guint flag)
{
    TestInfo* info;

    info = g_slice_new(TestInfo);
    info->data = g_strdup(data);
    info->flag = flag;

    return info;
}

void
test_info_destroy(TestInfo* info)
{
    g_free(info->data);
    g_slice_free(TestInfo, info);
}

static void hit_test_result_fixture_setup(HitTestResultFixture* fixture, gconstpointer data)
{
    fixture->webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(fixture->webView);
    loop = g_main_loop_new(NULL, TRUE);
    fixture->info = (TestInfo*)data;
}

static void hit_test_result_fixture_teardown(HitTestResultFixture* fixture, gconstpointer data)
{
    g_object_unref(fixture->webView);
    g_main_loop_unref(loop);
    test_info_destroy(fixture->info);
}

static void
load_status_cb(WebKitWebView* webView,
               GParamSpec* spec,
               gpointer data)
{
    WebKitLoadStatus status = webkit_web_view_get_load_status(webView);
    TestInfo* info = (TestInfo*)data;

    if (status == WEBKIT_LOAD_FINISHED) {
        WebKitHitTestResult* result;
        guint context;
        GdkEvent* event = gdk_event_new(GDK_BUTTON_PRESS);
        WebKitDOMNode* node;
        gint x, y;

        /* Close enough to 0,0 */
        event->button.x = 5;
        event->button.y = 5;

        result = webkit_web_view_get_hit_test_result(webView, (GdkEventButton*) event);
        gdk_event_free(event);
        g_assert(result);

        g_object_get(result, "context", &context, NULL);
        g_assert(context & info->flag);

        g_object_get(result, "inner-node", &node, NULL);
        g_assert(node);
        g_assert(WEBKIT_DOM_IS_NODE(node));

        g_object_get(result, "x", &x, "y", &y, NULL);
        g_assert_cmpint(x, ==, 5);
        g_assert_cmpint(y, ==, 5);

        /* We can only test these node types at the moment. In the
         * input case there seems to be an extra layer with a DIV on
         * top of the input, which gets assigned to the inner-node.
         * tag */
        if (info->flag == WEBKIT_HIT_TEST_RESULT_CONTEXT_DOCUMENT)
            g_assert(WEBKIT_DOM_IS_HTML_HTML_ELEMENT(node));
        else if (info->flag == WEBKIT_HIT_TEST_RESULT_CONTEXT_IMAGE)
            g_assert(WEBKIT_DOM_IS_HTML_IMAGE_ELEMENT(node));
        else if (info->flag == WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK) {
            /* The hit test will give us the inner text node, we want
             * the A tag */
            WebKitDOMNode* parent = webkit_dom_node_get_parent_node(node);
            g_assert(WEBKIT_DOM_IS_HTML_ANCHOR_ELEMENT(parent));
        }

        g_object_unref(result);
        g_main_loop_quit(loop);
    }
}

static void
test_webkit_hit_test_result(HitTestResultFixture* fixture, gconstpointer data)
{
    TestInfo* info = (TestInfo*)data;
    GtkAllocation allocation = { 0, 0, 50, 50 };

    webkit_web_view_load_string(fixture->webView,
                                info->data,
                                "text/html",
                                "utf-8",
                                "file://");
    gtk_widget_size_allocate(GTK_WIDGET(fixture->webView), &allocation);
    g_signal_connect(fixture->webView, "notify::load-status", G_CALLBACK(load_status_cb), info);
    g_main_loop_run(loop);
}

int main(int argc, char** argv)
{
    gtk_test_init(&argc, &argv, NULL);

    g_test_bug_base("https://bugs.webkit.org/");

    g_test_add("/webkit/hittestresult/document", HitTestResultFixture, 
               test_info_new("<html><body><h1>WebKitGTK+!</h1></body></html>",
                             WEBKIT_HIT_TEST_RESULT_CONTEXT_DOCUMENT),
               hit_test_result_fixture_setup, test_webkit_hit_test_result, hit_test_result_fixture_teardown);
    /* We hardcode all elements to be at 0,0 so that we know where to
     * generate the button events */
    g_test_add("/webkit/hittestresult/image", HitTestResultFixture,
               test_info_new("<html><body><img style='position:absolute; left:0; top:0'src='0xdeadbeef' width=50 height=50></img></body></html>",
                             WEBKIT_HIT_TEST_RESULT_CONTEXT_IMAGE),
               hit_test_result_fixture_setup, test_webkit_hit_test_result, hit_test_result_fixture_teardown);
    g_test_add("/webkit/hittestresult/editable", HitTestResultFixture,
               test_info_new("<html><body><input style='position:absolute; left:0; top:0' size='35'></input>></body></html>",
                             WEBKIT_HIT_TEST_RESULT_CONTEXT_EDITABLE),
               hit_test_result_fixture_setup, test_webkit_hit_test_result, hit_test_result_fixture_teardown);
    g_test_add("/webkit/hittestresult/link", HitTestResultFixture,
               test_info_new("<html><body><a style='position:absolute; left:0; top:0' href='http://www.example.com'>HELLO WORLD</a></body></html>",
                             WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK),
               hit_test_result_fixture_setup, test_webkit_hit_test_result, hit_test_result_fixture_teardown);
               
    return g_test_run ();
}

