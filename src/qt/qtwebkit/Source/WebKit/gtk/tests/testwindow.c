/*
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
#include <gtk/gtk.h>
#include <webkit/webkit.h>

static void notify_load_status_cb(WebKitWebView* web_view, GParamSpec* pspec, gpointer data)
{
    if (webkit_web_view_get_load_status(web_view) == WEBKIT_LOAD_FINISHED) {
        GMainLoop* loop = (GMainLoop*)data;

        g_main_loop_quit(loop);
    }
}

static void test_webkit_window_scrollbar_policy(void)
{
    GMainLoop* loop;
    GtkWidget* scrolledWindow;
    GtkWidget* webView;
    WebKitWebFrame* mainFrame;
    GtkPolicyType horizontalPolicy;
    GtkPolicyType verticalPolicy;

    loop = g_main_loop_new(NULL, TRUE);

    scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    webView = webkit_web_view_new();
    g_object_ref_sink(webView);

    g_signal_connect(webView, "notify::load-status",
                     G_CALLBACK(notify_load_status_cb), loop);

    gtk_container_add(GTK_CONTAINER(scrolledWindow), webView);

    mainFrame = webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(webView));

    /* Test we correctly apply policy for not having scrollbars; This
     * case is special, because we turn the policy from NEVER to
     * AUTOMATIC, since we cannot easily represent the same thing
     * using GtkScrolledWindow */
    webkit_web_view_load_html_string(WEBKIT_WEB_VIEW(webView),
                                     "<html><body>WebKit!</body><script>document.getElementsByTagName('body')[0].style.overflow = 'hidden';</script></html>",
                                     "file://");

    g_main_loop_run(loop);

    gtk_scrolled_window_get_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
                                   &horizontalPolicy, &verticalPolicy);

    g_assert(horizontalPolicy == GTK_POLICY_AUTOMATIC);
    g_assert(verticalPolicy == GTK_POLICY_AUTOMATIC);

    g_assert(GTK_POLICY_NEVER == webkit_web_frame_get_horizontal_scrollbar_policy(mainFrame));
    g_assert(GTK_POLICY_NEVER == webkit_web_frame_get_vertical_scrollbar_policy(mainFrame));

    /* Test we correctly apply policy for always having scrollbars */
    webkit_web_view_load_html_string(WEBKIT_WEB_VIEW(webView),
                                     "<html><body>WebKit!</body><script>document.getElementsByTagName('body')[0].style.overflow = 'scroll';</script></html>",
                                     "file://");

    g_main_loop_run(loop);

    gtk_scrolled_window_get_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
                                   &horizontalPolicy, &verticalPolicy);

    g_assert(horizontalPolicy == GTK_POLICY_ALWAYS);
    g_assert(verticalPolicy == GTK_POLICY_ALWAYS);

    g_assert(horizontalPolicy == webkit_web_frame_get_horizontal_scrollbar_policy(mainFrame));
    g_assert(verticalPolicy == webkit_web_frame_get_vertical_scrollbar_policy(mainFrame));

    /* Test we correctly apply policy for having scrollbars when needed */
    webkit_web_view_load_html_string(WEBKIT_WEB_VIEW(webView),
                                     "<html><body>WebKit!</body><script>document.getElementsByTagName('body')[0].style.overflow = 'auto';</script></html>",
                                     "file://");

    g_main_loop_run(loop);

    gtk_scrolled_window_get_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
                                   &horizontalPolicy, &verticalPolicy);

    g_assert(horizontalPolicy == GTK_POLICY_AUTOMATIC);
    g_assert(verticalPolicy == GTK_POLICY_AUTOMATIC);

    g_assert(horizontalPolicy == webkit_web_frame_get_horizontal_scrollbar_policy(mainFrame));
    g_assert(verticalPolicy == webkit_web_frame_get_vertical_scrollbar_policy(mainFrame));

    g_object_unref(webView);
}

int main(int argc, char** argv)
{
    gtk_test_init(&argc, &argv, NULL);

    g_test_bug_base("https://bugs.webkit.org/");
    g_test_add_func("/webkit/window/scrollbar_policy", test_webkit_window_scrollbar_policy);
    return g_test_run ();
}
