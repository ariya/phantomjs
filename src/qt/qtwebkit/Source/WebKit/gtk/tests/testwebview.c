/*
 * Copyright (C) 2008 Holger Hans Peter Freyther
 * Copyright (C) 2009, 2010 Collabora Ltd.
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

#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>

GMainLoop* loop;
SoupSession *session;
char* base_uri;

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

    if (g_str_equal(path, "/favicon.ico")) {
        char* contents;
        gsize length;
        GError* error = NULL;

        g_file_get_contents("blank.ico", &contents, &length, &error);
        g_assert(!error);

        soup_message_body_append(msg->response_body, SOUP_MEMORY_TAKE, contents, length);
    } else if (g_str_equal(path, "/bigdiv.html")) {
        char* contents = g_strdup("<html><body><a id=\"link\" href=\"http://abc.def\">test</a><div style=\"background-color: green; height: 1200px;\"></div></body></html>");
        soup_message_body_append(msg->response_body, SOUP_MEMORY_TAKE, contents, strlen(contents));
    } else if (g_str_equal(path, "/iframe.html")) {
        char* contents = g_strdup("<html><body id=\"some-content\"><div style=\"background-color: green; height: 50px;\"></div><iframe src=\"bigdiv.html\"></iframe></body></html>");
        soup_message_body_append(msg->response_body, SOUP_MEMORY_TAKE, contents, strlen(contents));
    } else {
        char* contents = g_strdup("<html><body>test</body></html>");
        soup_message_body_append(msg->response_body, SOUP_MEMORY_TAKE, contents, strlen(contents));
    }

    soup_message_body_complete(msg->response_body);
}

static void idle_quit_loop_cb(WebKitWebView* web_view, GParamSpec* pspec, gpointer data)
{
    if (webkit_web_view_get_load_status(web_view) == WEBKIT_LOAD_FINISHED ||
        webkit_web_view_get_load_status(web_view) == WEBKIT_LOAD_FAILED)
        g_main_loop_quit(loop);
}

static gboolean timeout_cb(gpointer data)
{
    g_error("Didn't get icon-uri before timing out.");
    return FALSE;
}

static void icon_uri_changed_cb(WebKitWebView* web_view, GParamSpec* pspec, gpointer data)
{
    char* expected_uri;

    g_assert_cmpstr(g_param_spec_get_name(pspec), ==, "icon-uri");

    expected_uri = g_strdup_printf("%sfavicon.ico", base_uri);
    g_assert_cmpstr(webkit_web_view_get_icon_uri(web_view), ==, expected_uri);
    g_free(expected_uri);

    g_main_loop_quit(loop);
}

static void icon_loaded_cb(WebKitWebView* web_view, char* icon_uri, gpointer data)
{
    gboolean* been_here = (gboolean*)data;
    char* expected_uri = g_strdup_printf("%sfavicon.ico", base_uri);
    g_assert_cmpstr(icon_uri, ==, expected_uri);
    g_free(expected_uri);

    g_assert_cmpstr(icon_uri, ==, webkit_web_view_get_icon_uri(web_view));

    *been_here = TRUE;
}

static void test_webkit_web_view_icon_uri()
{
    gboolean been_to_icon_loaded = FALSE;
    WebKitWebView* view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_object_ref_sink(G_OBJECT(view));

    loop = g_main_loop_new(NULL, TRUE);

    g_object_connect(G_OBJECT(view),
                     "signal::notify::icon-uri", icon_uri_changed_cb, NULL,
                     "signal::icon-loaded", icon_loaded_cb, &been_to_icon_loaded,
                     NULL);

    webkit_web_view_load_uri(view, base_uri);

    guint timeout_id = g_timeout_add(500, timeout_cb, 0);

    g_main_loop_run(loop);

    g_source_remove(timeout_id);

    g_assert(been_to_icon_loaded);

    g_object_unref(view);
}

static gboolean map_event_cb(GtkWidget *widget, GdkEvent* event, gpointer data)
{
    GMainLoop* loop = (GMainLoop*)data;
    g_main_loop_quit(loop);

    return FALSE;
}

static gboolean quit_after_short_delay_cb(gpointer data)
{
    g_main_loop_quit((GMainLoop*)data);
    return FALSE;
}

static void test_webkit_web_view_grab_focus()
{
    char* uri = g_strconcat(base_uri, "iframe.html", NULL);
    GtkWidget* window = gtk_window_new(GTK_WINDOW_POPUP);
    GtkWidget* scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    WebKitWebView* view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    GtkAdjustment* adjustment;

    gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);

    gtk_container_add(GTK_CONTAINER(window), scrolled_window);
    gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(view));

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    loop = g_main_loop_new(NULL, TRUE);

    g_signal_connect(view, "notify::load-status", G_CALLBACK(idle_quit_loop_cb), NULL);

    /* Wait for window to show up */
    gtk_widget_show_all(window);
    g_signal_connect(window, "map-event",
                     G_CALLBACK(map_event_cb), loop);
    g_main_loop_run(loop);

    /* Load a page with a big div that will cause scrollbars to appear */
    webkit_web_view_load_uri(view, uri);
    g_main_loop_run(loop);

    adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_window));
    g_assert_cmpfloat(gtk_adjustment_get_value(adjustment), ==, 0.0);

    /* Since webkit_web_view_execute_script does not return a value,
       it is impossible to know if an inner document has focus after
       a node of it was focused via .focus() method.
       The code below is an workaround: if the node has focus, a scroll
       action is performed and afterward it is checked if the adjustment
       has to be different from 0.
    */
    char script[] = "var innerDoc = document.defaultView.frames[0].document; \
                     innerDoc.getElementById(\"link\").focus();              \
                     if (innerDoc.hasFocus())                                \
                        window.scrollBy(0, 100);";

    /* Focus an element using JavaScript */
    webkit_web_view_execute_script(view, script);

    /* Adjustments update asynchronously, so we must wait a bit. */
    g_timeout_add(100, quit_after_short_delay_cb, loop);
    g_main_loop_run(loop);

    /* Make sure the ScrolledWindow noticed the scroll */
    g_assert_cmpfloat(gtk_adjustment_get_value(adjustment), !=, 0.0);

    g_free(uri);
    gtk_widget_destroy(window);
}

static void do_test_webkit_web_view_adjustments(gboolean with_page_cache)
{
    char* effective_uri = g_strconcat(base_uri, "bigdiv.html", NULL);
    char* second_uri = g_strconcat(base_uri, "iframe.html", NULL);
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget* scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    WebKitWebView* view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    GtkAdjustment* adjustment;
    double lower;
    double upper;

    if (with_page_cache) {
        WebKitWebSettings* settings = webkit_web_view_get_settings(view);
        g_object_set(settings, "enable-page-cache", TRUE, NULL);
    }

    gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);

    gtk_container_add(GTK_CONTAINER(window), scrolled_window);
    gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(view));

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    loop = g_main_loop_new(NULL, TRUE);

    g_object_connect(G_OBJECT(view),
                     "signal::notify::load-status", idle_quit_loop_cb, NULL,
                     NULL);

    /* Wait for window to show up */
    gtk_widget_show_all(window);
    g_signal_connect(window, "map-event",
                     G_CALLBACK(map_event_cb), loop);
    g_main_loop_run(loop);

    /* Load a page with a big div that will cause scrollbars to appear */
    webkit_web_view_load_uri(view, effective_uri);
    g_main_loop_run(loop);

    /* Adjustments update asynchronously, so we must wait a bit. */
    g_timeout_add(100, quit_after_short_delay_cb, loop);
    g_main_loop_run(loop);

    adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_window));
    g_assert_cmpfloat(gtk_adjustment_get_value(adjustment), ==, 0.0);

    lower = gtk_adjustment_get_lower(adjustment);
    upper = gtk_adjustment_get_upper(adjustment);

    /* Scroll the view using JavaScript */
    webkit_web_view_execute_script(view, "window.scrollBy(0, 100)");

    /* Adjustments update asynchronously, so we must wait a bit. */
    g_timeout_add(100, quit_after_short_delay_cb, loop);
    g_main_loop_run(loop);

    /* Make sure the ScrolledWindow noticed the scroll */
    g_assert_cmpfloat(gtk_adjustment_get_value(adjustment), ==, 100.0);

    /* Load a second URI */
    webkit_web_view_load_uri(view, second_uri);
    g_main_loop_run(loop);

    /* The page loaded but the adjustments may not be updated yet. Wait a bit. */
    g_timeout_add(100, quit_after_short_delay_cb, loop);
    g_main_loop_run(loop);

    /* Make sure the scrollbar has been reset */
    g_assert_cmpfloat(gtk_adjustment_get_value(adjustment), ==, 0.0);

    /* Go back */
    webkit_web_view_go_back(view);

    /* When using page cache, go_back will return syncronously */
    if (!with_page_cache)
        g_main_loop_run(loop);

    /* Make sure GTK+ has time to process the changes in size, for the adjusments */
    while (gtk_events_pending())
        gtk_main_iteration();

    /* Make sure upper and lower bounds have been restored correctly */
    g_assert_cmpfloat(lower, ==, gtk_adjustment_get_lower(adjustment));
    g_assert_cmpfloat(upper, ==, gtk_adjustment_get_upper(adjustment));
    g_assert_cmpfloat(gtk_adjustment_get_value(adjustment), ==, 100.0);

    g_free(effective_uri);
    g_free(second_uri);

    gtk_widget_destroy(window);
}

static void test_webkit_web_view_adjustments()
{
    /* Test this with page cache disabled, and enabled. */
    do_test_webkit_web_view_adjustments(FALSE);
    do_test_webkit_web_view_adjustments(TRUE);
}

gboolean delayed_destroy(gpointer data)
{
    gtk_widget_destroy(GTK_WIDGET(data));
    g_main_loop_quit(loop);
    return FALSE;
}

static void test_webkit_web_view_destroy()
{
    GtkWidget* window;
    GtkWidget* web_view;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    web_view = webkit_web_view_new();

    gtk_container_add(GTK_CONTAINER(window), web_view);

    gtk_widget_show_all(window);

    loop = g_main_loop_new(NULL, TRUE);

    g_signal_connect(window, "map-event",
                     G_CALLBACK(map_event_cb), loop);
    g_main_loop_run(loop);

    g_idle_add(delayed_destroy, web_view);
    g_main_loop_run(loop);

    gtk_widget_destroy(window);
}

static void test_webkit_web_view_window_features()
{
    GtkWidget* window;
    GtkWidget* web_view;
    
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    web_view = webkit_web_view_new();
    
    gtk_container_add(GTK_CONTAINER(window), web_view);
    
    gtk_widget_show_all(window);
    
    loop = g_main_loop_new(NULL, TRUE);

    g_signal_connect(window, "map-event",
                     G_CALLBACK(map_event_cb), loop);
    g_main_loop_run(loop);
    
    /* Bug #36144 */
    g_object_set(G_OBJECT(web_view), "window-features", NULL, NULL);
    
    gtk_widget_destroy(window);
}    

static void test_webkit_web_view_in_offscreen_window_does_not_crash()
{
    loop = g_main_loop_new(NULL, TRUE);

    GtkWidget *window = gtk_offscreen_window_new();
    GtkWidget *web_view = webkit_web_view_new();

    gtk_container_add(GTK_CONTAINER(window), web_view);
    gtk_widget_show_all(window);
    g_signal_connect(web_view, "notify::load-status", G_CALLBACK(idle_quit_loop_cb), NULL);
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(web_view), base_uri);

    g_main_loop_run(loop);

    gtk_widget_destroy(window);
    g_main_loop_unref(loop);
}

static void test_webkit_web_view_does_not_steal_focus()
{
    loop = g_main_loop_new(NULL, TRUE);

    GtkWidget *window = gtk_offscreen_window_new();
    GtkWidget *webView = webkit_web_view_new();
    GtkWidget *entry = gtk_entry_new();

#ifdef GTK_API_VERSION_2
    GtkWidget *box = gtk_hbox_new(FALSE, 0);
#else
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#endif

    gtk_container_add(GTK_CONTAINER(box), webView);
    gtk_container_add(GTK_CONTAINER(box), entry);
    gtk_container_add(GTK_CONTAINER(window), box);
    gtk_widget_show_all(window);

    gtk_widget_grab_focus(entry);
    g_assert(gtk_widget_is_focus(entry));

    g_signal_connect(webView, "notify::load-status", G_CALLBACK(idle_quit_loop_cb), NULL);
    webkit_web_view_load_html_string(WEBKIT_WEB_VIEW(webView),
        "<html><body>"
        "    <input id=\"entry\" type=\"text\"/>"
        "    <script>"
        "        document.getElementById(\"entry\").focus();"
        "    </script>"
        "</body></html>", "file://");

    g_main_loop_run(loop);

    g_assert(gtk_widget_is_focus(entry));

    gtk_widget_destroy(window);
    g_main_loop_unref(loop);
}

static gboolean emitKeyStroke(WebKitWebView* webView)
{
    GdkEvent* pressEvent = gdk_event_new(GDK_KEY_PRESS);
    pressEvent->key.keyval = GDK_KEY_f;
    GdkWindow* window = gtk_widget_get_window(GTK_WIDGET(webView));
    pressEvent->key.window = window;
    g_object_ref(pressEvent->key.window);

#ifndef GTK_API_VERSION_2
    GdkDeviceManager* manager = gdk_display_get_device_manager(gdk_window_get_display(window));
    gdk_event_set_device(pressEvent, gdk_device_manager_get_client_pointer(manager));
#endif

    // When synthesizing an event, an invalid hardware_keycode value
    // can cause it to be badly processed by Gtk+.
    GdkKeymapKey* keys;
    gint n_keys;
    if (gdk_keymap_get_entries_for_keyval(gdk_keymap_get_default(), GDK_KEY_f, &keys, &n_keys)) {
        pressEvent->key.hardware_keycode = keys[0].keycode;
        g_free(keys);
    }

    GdkEvent* releaseEvent = gdk_event_copy(pressEvent);
    gtk_main_do_event(pressEvent);
    gdk_event_free(pressEvent);
    releaseEvent->key.type = GDK_KEY_RELEASE;
    gtk_main_do_event(releaseEvent);
    gdk_event_free(releaseEvent);

    return FALSE;
}

static gboolean entering_fullscreen_cb(WebKitWebView* webView, GObject* element, gboolean blocked)
{
    if (blocked)
        g_main_loop_quit(loop);
    else
        g_timeout_add(200, (GSourceFunc) emitKeyStroke, webView);
    return blocked;
}

static gboolean leaving_fullscreen_cb(WebKitWebView* webView, GObject* element, gpointer data)
{
    g_main_loop_quit(loop);
    return FALSE;
}

static void test_webkit_web_view_fullscreen(gconstpointer blocked)
{
    GtkWidget* window;
    GtkWidget* web_view;
    WebKitWebSettings *settings;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    web_view = webkit_web_view_new();

    settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(web_view));
    g_object_set(settings, "enable-fullscreen", TRUE, NULL);
    webkit_web_view_set_settings(WEBKIT_WEB_VIEW(web_view), settings);

    gtk_container_add(GTK_CONTAINER(window), web_view);

    gtk_widget_show_all(window);

    loop = g_main_loop_new(NULL, TRUE);

    g_signal_connect(web_view, "entering-fullscreen", G_CALLBACK(entering_fullscreen_cb), (gpointer) blocked);
    g_signal_connect(web_view, "leaving-fullscreen", G_CALLBACK(leaving_fullscreen_cb), NULL);

    webkit_web_view_load_string(WEBKIT_WEB_VIEW(web_view), "<html><body>"
                   "<script>"
                   "var eventName = 'keypress';"
                   "document.addEventListener(eventName, function () {"
                   "    document.documentElement.webkitRequestFullScreen();"
                   "}, false);"
                   "</script></body></html>", NULL, NULL, NULL);

    g_timeout_add(100, (GSourceFunc) emitKeyStroke, WEBKIT_WEB_VIEW(web_view));
    g_main_loop_run(loop);

    gtk_widget_destroy(window);
}

static gboolean checkMimeTypeForFilter(GtkFileFilter* filter, const gchar* mimeType)
{
    GtkFileFilterInfo filter_info;
    filter_info.contains = GTK_FILE_FILTER_MIME_TYPE;
    filter_info.mime_type = mimeType;
    return gtk_file_filter_filter(filter, &filter_info);
}

static gboolean runFileChooserCbNoMultiselNoMime(WebKitWebView* webview, WebKitFileChooserRequest* request, gpointer data)
{
    g_assert(!webkit_file_chooser_request_get_select_multiple(request));

    const gchar* const* mimeTypes = webkit_file_chooser_request_get_mime_types(request);
    g_assert(!mimeTypes);
    GtkFileFilter* filter = webkit_file_chooser_request_get_mime_types_filter(request);
    g_assert(!filter);

    const gchar* const* selectedFiles = webkit_file_chooser_request_get_selected_files(request);
    g_assert(!selectedFiles);

    g_main_loop_quit(loop);
    return TRUE;
}

static gboolean runFileChooserCbMultiselNoMime(WebKitWebView* webview, WebKitFileChooserRequest* request, gpointer data)
{
    g_assert(webkit_file_chooser_request_get_select_multiple(request));

    const gchar* const* mimeTypes = webkit_file_chooser_request_get_mime_types(request);
    g_assert(!mimeTypes);
    GtkFileFilter* filter = webkit_file_chooser_request_get_mime_types_filter(request);
    g_assert(!filter);
    const gchar* const* selectedFiles = webkit_file_chooser_request_get_selected_files(request);
    g_assert(!selectedFiles);

    // Select some files.
    const gchar* filesToSelect[4] = { "/foo", "/foo/bar", "/foo/bar/baz", 0 };
    webkit_file_chooser_request_select_files(request, filesToSelect);

    // Check the files that have been just selected.
    selectedFiles = webkit_file_chooser_request_get_selected_files(request);
    g_assert(selectedFiles);
    g_assert_cmpstr(selectedFiles[0], ==, "/foo");
    g_assert_cmpstr(selectedFiles[1], ==, "/foo/bar");
    g_assert_cmpstr(selectedFiles[2], ==, "/foo/bar/baz");
    g_assert(!selectedFiles[3]);

    g_main_loop_quit(loop);
    return TRUE;
}

static gboolean runFileChooserCbSelectionRetained(WebKitWebView* webview, WebKitFileChooserRequest* request, gpointer data)
{
    const gchar* const* selectedFiles = webkit_file_chooser_request_get_selected_files(request);
    g_assert(selectedFiles);
    g_assert_cmpstr(selectedFiles[0], ==, "/foo");
    g_assert_cmpstr(selectedFiles[1], ==, "/foo/bar");
    g_assert_cmpstr(selectedFiles[2], ==, "/foo/bar/baz");
    g_assert(!selectedFiles[3]);

    g_main_loop_quit(loop);
    return TRUE;
}

static gboolean runFileChooserCbNoMultiselAcceptTypes(WebKitWebView* webview, WebKitFileChooserRequest* request, gpointer data)
{
    g_assert(!webkit_file_chooser_request_get_select_multiple(request));

    const gchar* const* mimeTypes = webkit_file_chooser_request_get_mime_types(request);
    g_assert(mimeTypes);
    g_assert_cmpstr(mimeTypes[0], ==, "audio/*");
    g_assert_cmpstr(mimeTypes[1], ==, "video/*");
    g_assert_cmpstr(mimeTypes[2], ==, "image/*");
    g_assert(!mimeTypes[3]);

    GtkFileFilter* filter = webkit_file_chooser_request_get_mime_types_filter(request);
    g_assert(GTK_IS_FILE_FILTER(filter));
    g_assert(checkMimeTypeForFilter(filter, "audio/*"));
    g_assert(checkMimeTypeForFilter(filter, "video/*"));
    g_assert(checkMimeTypeForFilter(filter, "image/*"));

    const gchar* const* selectedFiles = webkit_file_chooser_request_get_selected_files(request);
    g_assert(!selectedFiles);

    g_main_loop_quit(loop);
    return TRUE;
}

void doMouseButtonEvent(GtkWidget* widget, GdkEventType eventType, int x, int y, unsigned int button, unsigned int modifiers)
{
    g_assert(gtk_widget_get_realized(widget));

    GdkEvent* event = gdk_event_new(eventType);
    event->button.window = gtk_widget_get_window(widget);
    g_object_ref(event->button.window);

    event->button.time = GDK_CURRENT_TIME;
    event->button.x = x;
    event->button.y = y;
    event->button.axes = 0;
    event->button.state = modifiers;
    event->button.button = button;

#ifndef GTK_API_VERSION_2
    event->button.device = gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gtk_widget_get_display(widget)));
#endif

    int xRoot, yRoot;
    gdk_window_get_root_coords(gtk_widget_get_window(widget), x, y, &xRoot, &yRoot);
    event->button.x_root = xRoot;
    event->button.y_root = yRoot;
    gtk_main_do_event(event);
}

static void clickMouseButton(GtkWidget* widget, int x, int y, unsigned int button, unsigned int modifiers)
{
    doMouseButtonEvent(widget, GDK_BUTTON_PRESS, x, y, button, modifiers);
    doMouseButtonEvent(widget, GDK_BUTTON_RELEASE, x, y, button, modifiers);
}

static gboolean clickMouseButtonAndWaitForFileChooserRequest(WebKitWebView* webView)
{
    clickMouseButton(GTK_WIDGET(webView), 5, 5, 1, 0);
    return TRUE;
}

static void test_webkit_web_view_file_chooser()
{
    const gchar* htmlFormatBase = "<html><body>"
            "<input style='position:absolute;left:0;top:0;margin:0;padding:0' type='file' %s/>"
            "</body></html>";

    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget* webView = webkit_web_view_new();
    gtk_container_add(GTK_CONTAINER(window), webView);
    gtk_widget_show_all(window);

    loop = g_main_loop_new(NULL, TRUE);

    // Multiple selections not allowed, no MIME filtering.
    gulong handler = g_signal_connect(webView, "run-file-chooser", G_CALLBACK(runFileChooserCbNoMultiselNoMime), NULL);
    gchar* htmlFormat = g_strdup_printf(htmlFormatBase, "");
    webkit_web_view_load_string(WEBKIT_WEB_VIEW(webView), htmlFormat, NULL, NULL, NULL);
    g_free(htmlFormat);

    g_timeout_add(100, (GSourceFunc) clickMouseButtonAndWaitForFileChooserRequest, WEBKIT_WEB_VIEW(webView));
    g_main_loop_run(loop);

    g_signal_handler_disconnect(webView, handler);

    // Multiple selections allowed, no MIME filtering, some pre-selected files.
    handler = g_signal_connect(webView, "run-file-chooser", G_CALLBACK(runFileChooserCbMultiselNoMime), NULL);
    htmlFormat = g_strdup_printf(htmlFormatBase, "multiple");
    webkit_web_view_load_string(WEBKIT_WEB_VIEW(webView), htmlFormat, NULL, NULL, NULL);
    g_free(htmlFormat);

    g_timeout_add(100, (GSourceFunc) clickMouseButtonAndWaitForFileChooserRequest, WEBKIT_WEB_VIEW(webView));
    g_main_loop_run(loop);

    g_signal_handler_disconnect(webView, handler);

    // Perform another request to check if the list of files selected
    // in the previous step appears now as part of the new request.
    handler = g_signal_connect(webView, "run-file-chooser", G_CALLBACK(runFileChooserCbSelectionRetained), NULL);
    g_timeout_add(100, (GSourceFunc) clickMouseButtonAndWaitForFileChooserRequest, WEBKIT_WEB_VIEW(webView));
    g_main_loop_run(loop);

    g_signal_handler_disconnect(webView, handler);

    // Multiple selections not allowed, only accept images, audio and video files.
    handler = g_signal_connect(webView, "run-file-chooser", G_CALLBACK(runFileChooserCbNoMultiselAcceptTypes), NULL);
    htmlFormat = g_strdup_printf(htmlFormatBase, "accept='audio/*,video/*,image/*'");
    webkit_web_view_load_string(WEBKIT_WEB_VIEW(webView), htmlFormat, NULL, NULL, NULL);
    g_free(htmlFormat);

    g_timeout_add(100, (GSourceFunc) clickMouseButtonAndWaitForFileChooserRequest, WEBKIT_WEB_VIEW(webView));
    g_main_loop_run(loop);

    g_signal_handler_disconnect(webView, handler);
    gtk_widget_destroy(window);
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
    g_test_add_func("/webkit/webview/icon-uri", test_webkit_web_view_icon_uri);
    g_test_add_func("/webkit/webview/adjustments", test_webkit_web_view_adjustments);
    g_test_add_func("/webkit/webview/destroy", test_webkit_web_view_destroy);
    g_test_add_func("/webkit/webview/grab_focus", test_webkit_web_view_grab_focus);
    g_test_add_func("/webkit/webview/window-features", test_webkit_web_view_window_features);
    g_test_add_func("/webkit/webview/webview-in-offscreen-window-does-not-crash", test_webkit_web_view_in_offscreen_window_does_not_crash);
    g_test_add_func("/webkit/webview/webview-does-not-steal-focus", test_webkit_web_view_does_not_steal_focus);
    g_test_add_data_func("/webkit/webview/fullscreen", GINT_TO_POINTER(FALSE), test_webkit_web_view_fullscreen);
    g_test_add_data_func("/webkit/webview/fullscreen-blocked", GINT_TO_POINTER(TRUE), test_webkit_web_view_fullscreen);
    g_test_add_func("/webkit/webview/file-chooser", test_webkit_web_view_file_chooser);

    return g_test_run ();
}
