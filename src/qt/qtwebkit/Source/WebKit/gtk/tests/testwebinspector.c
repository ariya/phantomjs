/*
 * Copyright (C) 2012 Gustavo Noronha Silva <gns@gnome.org>
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

#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>

GMainLoop *loop;
GtkWidget *window;

static gboolean quitLoop(gpointer data)
{
    g_main_loop_quit(loop);
    return TRUE;
}

/* Ignore simple translation-related messages and upgrade other
 * messages to warnings.
 */
static gboolean consoleMessageCallback(WebKitWebView* webView, const char* message, unsigned int line, const char* sourceId)
{
    if (strstr(message, "Localized string") || strstr(message, "Protocol Error: the message is for non-existing domain 'Profiler'"))
        return TRUE;

    g_warning("Console: %s @%d: %s\n", sourceId, line, message);
    return TRUE;
}

static WebKitWebView* inspectElementCallback(WebKitWebInspector *inspector, WebKitWebView *inspectedWebView, int *timesElementInspected)
{
    *timesElementInspected = *timesElementInspected + 1;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    GtkWidget *newWebView = webkit_web_view_new();
    gtk_container_add(GTK_CONTAINER(window), newWebView);

    g_signal_connect(newWebView, "console-message",
                     G_CALLBACK(consoleMessageCallback), NULL);

    return WEBKIT_WEB_VIEW(newWebView);
}

static gboolean closeInspector(WebKitWebInspector *inspector, int *timesClosed)
{
    *timesClosed = *timesClosed + 1;

    gtk_widget_destroy(window);
    return TRUE;
}

static gboolean showInspector(WebKitWebInspector *inspector, gpointer data)
{
    g_idle_add(quitLoop, NULL);
    return TRUE;
}

static void loadFinished(WebKitWebView *webView, WebKitWebFrame *frame, gboolean *isLoadFinished)
{
    *isLoadFinished = TRUE;
    if (g_main_loop_is_running(loop))
        g_main_loop_quit(loop);
}

static void test_webkit_web_inspector_close_and_inspect()
{
    WebKitWebView *webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    loop = g_main_loop_new(NULL, TRUE);

    gboolean isLoadFinished = FALSE;
    g_signal_connect(webView, "load-finished", G_CALLBACK(loadFinished), &isLoadFinished);
    webkit_web_view_load_string(webView,
                                "<html><body><p>woohoo</p></body></html>",
                                "text/html", "UTF-8", "file://");
    if (!isLoadFinished)
        g_main_loop_run(loop);

    g_object_set(webkit_web_view_get_settings(webView), "enable-developer-extras", TRUE, NULL);
    WebKitWebInspector *inspector = webkit_web_view_get_inspector(webView);

    int timesElementInspected = 0;
    int timesClosed = 0;
    g_object_connect(inspector,
                     "signal::inspect-web-view", G_CALLBACK(inspectElementCallback), &timesElementInspected,
                     "signal::show-window", G_CALLBACK(showInspector), NULL,
                     "signal::close-window", G_CALLBACK(closeInspector), &timesClosed,
                     NULL);

    webkit_web_inspector_inspect_coordinates(inspector, 0.0, 0.0);
    g_assert_cmpint(timesElementInspected, ==, 1);

    g_main_loop_run(loop);

    webkit_web_inspector_close(inspector);
    g_assert_cmpint(timesClosed, ==, 1);

    webkit_web_inspector_inspect_coordinates(inspector, 0.0, 0.0);
    g_assert_cmpint(timesElementInspected, ==, 2);

    g_main_loop_run(loop);

    gtk_widget_destroy(GTK_WIDGET(webView));
    g_assert_cmpint(timesClosed, ==, 2);

    g_main_loop_unref(loop);
}

static void test_webkit_web_inspector_destroy_inspected_web_view()
{
    WebKitWebView *webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    loop = g_main_loop_new(NULL, TRUE);

    gboolean isLoadFinished = FALSE;
    g_signal_connect(webView, "load-finished", G_CALLBACK(loadFinished), &isLoadFinished);
    webkit_web_view_load_string(webView,
                                "<html><body><p>woohoo</p></body></html>",
                                "text/html", "UTF-8", "file://");
    if (!isLoadFinished)
        g_main_loop_run(loop);

    g_object_set(webkit_web_view_get_settings(webView), "enable-developer-extras", TRUE, NULL);
    WebKitWebInspector *inspector = webkit_web_view_get_inspector(webView);

    int timesElementInspected = 0;
    int timesClosed = 0;
    g_object_connect(inspector,
                     "signal::inspect-web-view", G_CALLBACK(inspectElementCallback), &timesElementInspected,
                     "signal::show-window", G_CALLBACK(showInspector), NULL,
                     "signal::close-window", G_CALLBACK(closeInspector), &timesClosed,
                     NULL);

    webkit_web_inspector_inspect_coordinates(inspector, 0.0, 0.0);
    g_assert_cmpint(timesElementInspected, ==, 1);

    g_main_loop_run(loop);

    gtk_widget_destroy(GTK_WIDGET(webView));
    g_assert_cmpint(timesClosed, ==, 1);

    g_main_loop_unref(loop);
}

int main(int argc, char** argv)
{
    gtk_test_init(&argc, &argv, NULL);

    g_test_bug_base("https://bugs.webkit.org/");
    g_test_add_func("/webkit/webinspector/destroy-inspected-web-view", test_webkit_web_inspector_destroy_inspected_web_view);
    g_test_add_func("/webkit/webinspector/close-and-inspect", test_webkit_web_inspector_close_and_inspect);

    return g_test_run();
}
