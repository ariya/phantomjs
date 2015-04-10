/*
 * Copyright (C) 2009, 2010 Martin Robinson <mrobinson@webkit.org>
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
#include <string.h>
#include <glib/gstdio.h>
#include <webkit/webkit.h>
#include <JavaScriptCore/JSStringRef.h>
#include <JavaScriptCore/JSContextRef.h>

typedef struct {
    char* page;
    char* text;
    gboolean shouldBeHandled;
} TestInfo;

typedef struct {
    GtkWidget* window;
    WebKitWebView* webView;
    GMainLoop* loop;
    TestInfo* info;
} KeyEventFixture;

TestInfo*
test_info_new(const char* page, gboolean shouldBeHandled)
{
    TestInfo* info;

    info = g_slice_new(TestInfo);
    info->page = g_strdup(page);
    info->shouldBeHandled = shouldBeHandled;
    info->text = 0;

    return info;
}

void
test_info_destroy(TestInfo* info)
{
    g_free(info->page);
    g_free(info->text);
    g_slice_free(TestInfo, info);
}

static void key_event_fixture_setup(KeyEventFixture* fixture, gconstpointer data)
{
    fixture->loop = g_main_loop_new(NULL, TRUE);

    fixture->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    fixture->webView = WEBKIT_WEB_VIEW(webkit_web_view_new());

    gtk_container_add(GTK_CONTAINER(fixture->window), GTK_WIDGET(fixture->webView));
}

static void key_event_fixture_teardown(KeyEventFixture* fixture, gconstpointer data)
{
    gtk_widget_destroy(fixture->window);
    g_main_loop_unref(fixture->loop);
    test_info_destroy(fixture->info);
}

static gboolean key_press_event_cb(WebKitWebView* webView, GdkEvent* event, gpointer data)
{
    KeyEventFixture* fixture = (KeyEventFixture*)data;
    gboolean handled = GTK_WIDGET_GET_CLASS(fixture->webView)->key_press_event(GTK_WIDGET(fixture->webView), &event->key);
    g_assert_cmpint(handled, ==, fixture->info->shouldBeHandled);

    return FALSE;
}

static gboolean key_release_event_cb(WebKitWebView* webView, GdkEvent* event, gpointer data)
{
    // WebCore never seems to mark keyup events as handled.
    KeyEventFixture* fixture = (KeyEventFixture*)data;
    gboolean handled = GTK_WIDGET_GET_CLASS(fixture->webView)->key_press_event(GTK_WIDGET(fixture->webView), &event->key);
    g_assert(!handled);

    g_main_loop_quit(fixture->loop);

    return FALSE;
}

static void test_keypress_events_load_status_cb(WebKitWebView* webView, GParamSpec* spec, gpointer data)
{
    KeyEventFixture* fixture = (KeyEventFixture*)data;
    WebKitLoadStatus status = webkit_web_view_get_load_status(webView);
    if (status == WEBKIT_LOAD_FINISHED) {
        g_signal_connect(fixture->webView, "key-press-event",
                         G_CALLBACK(key_press_event_cb), fixture);
        g_signal_connect(fixture->webView, "key-release-event",
                         G_CALLBACK(key_release_event_cb), fixture);
        if (!gtk_test_widget_send_key(GTK_WIDGET(fixture->webView),
                                      gdk_unicode_to_keyval('a'), 0))
            g_assert_not_reached();
    }

}

gboolean map_event_cb(GtkWidget *widget, GdkEvent* event, gpointer data)
{
    KeyEventFixture* fixture = (KeyEventFixture*)data;
    webkit_web_view_load_string(fixture->webView, fixture->info->page,
                                "text/html", "utf-8", "file://");
    return FALSE;
}

static void setup_keyevent_test(KeyEventFixture* fixture, gconstpointer data, GCallback load_event_callback)
{
    fixture->info = (TestInfo*)data;
    g_signal_connect(fixture->window, "map-event",
                     G_CALLBACK(map_event_cb), fixture);

    gtk_widget_grab_focus(GTK_WIDGET(fixture->webView));
    gtk_widget_show(fixture->window);
    gtk_widget_show(GTK_WIDGET(fixture->webView));
    gtk_window_present(GTK_WINDOW(fixture->window));

    g_signal_connect(fixture->webView, "notify::load-status",
                     load_event_callback, fixture);

    g_main_loop_run(fixture->loop);
}

static void test_keypress_events(KeyEventFixture* fixture, gconstpointer data)
{
    setup_keyevent_test(fixture, data, G_CALLBACK(test_keypress_events_load_status_cb));
}

static gboolean element_text_equal_to(JSContextRef context, const gchar* text)
{
    JSStringRef scriptString = JSStringCreateWithUTF8CString(
      "window.document.getElementById(\"in\").value;");
    JSValueRef value = JSEvaluateScript(context, scriptString, 0, 0, 0, 0);
    JSStringRelease(scriptString);

    // If the value isn't a string, the element is probably a div
    // so grab the innerText instead.
    if (!JSValueIsString(context, value)) {
        JSStringRef scriptString = JSStringCreateWithUTF8CString(
          "window.document.getElementById(\"in\").innerText;");
        value = JSEvaluateScript(context, scriptString, 0, 0, 0, 0);
        JSStringRelease(scriptString);
    }

    g_assert(JSValueIsString(context, value));
    JSStringRef inputString = JSValueToStringCopy(context, value, 0);
    g_assert(inputString);

    gint size = JSStringGetMaximumUTF8CStringSize(inputString);
    gchar* cString = g_malloc(size);
    JSStringGetUTF8CString(inputString, cString, size);
    JSStringRelease(inputString);

    gboolean result = g_utf8_collate(cString, text) == 0;
    g_free(cString);
    return result;
}

static void test_ime_load_status_cb(WebKitWebView* webView, GParamSpec* spec, gpointer data)
{
    KeyEventFixture* fixture = (KeyEventFixture*)data;
    WebKitLoadStatus status = webkit_web_view_get_load_status(webView);
    if (status != WEBKIT_LOAD_FINISHED)
        return;

    JSGlobalContextRef context = webkit_web_frame_get_global_context(
        webkit_web_view_get_main_frame(webView));
    g_assert(context);

    GtkIMContext* imContext = 0;
    g_object_get(webView, "im-context", &imContext, NULL);
    g_assert(imContext);

    // Test that commits that happen outside of key events
    // change the text field immediately. This closely replicates
    // the behavior of SCIM.
    g_assert(element_text_equal_to(context, ""));
    g_signal_emit_by_name(imContext, "commit", "a");
    g_assert(element_text_equal_to(context, "a"));
    g_signal_emit_by_name(imContext, "commit", "b");
    g_assert(element_text_equal_to(context, "ab"));
    g_signal_emit_by_name(imContext, "commit", "c");
    g_assert(element_text_equal_to(context, "abc"));

    g_object_unref(imContext);
    g_main_loop_quit(fixture->loop);
}

static void test_ime(KeyEventFixture* fixture, gconstpointer data)
{
    setup_keyevent_test(fixture, data, G_CALLBACK(test_ime_load_status_cb));
}

static gboolean verify_contents(gpointer data)
{
    KeyEventFixture* fixture = (KeyEventFixture*)data;
    JSGlobalContextRef context = webkit_web_frame_get_global_context(
        webkit_web_view_get_main_frame(fixture->webView));
    g_assert(context);

    g_assert(element_text_equal_to(context, fixture->info->text));
    g_main_loop_quit(fixture->loop);
    return FALSE;
}

static void test_blocking_load_status_cb(WebKitWebView* webView, GParamSpec* spec, gpointer data)
{
    KeyEventFixture* fixture = (KeyEventFixture*)data;
    WebKitLoadStatus status = webkit_web_view_get_load_status(webView);
    if (status != WEBKIT_LOAD_FINISHED)
        return;

    // The first keypress event should not modify the field.
    fixture->info->text = g_strdup("bc");
    if (!gtk_test_widget_send_key(GTK_WIDGET(fixture->webView),
                                 gdk_unicode_to_keyval('a'), 0))
        g_assert_not_reached();
    if (!gtk_test_widget_send_key(GTK_WIDGET(fixture->webView),
                                  gdk_unicode_to_keyval('b'), 0))
        g_assert_not_reached();
    if (!gtk_test_widget_send_key(GTK_WIDGET(fixture->webView),
                                  gdk_unicode_to_keyval('c'), 0))
        g_assert_not_reached();

    g_idle_add(verify_contents, fixture);
}

static void test_blocking(KeyEventFixture* fixture, gconstpointer data)
{
    setup_keyevent_test(fixture, data, G_CALLBACK(test_blocking_load_status_cb));
}

#if defined(GDK_WINDOWING_X11)
static void test_xim_load_status_cb(WebKitWebView* webView, GParamSpec* spec, gpointer data)
{
    KeyEventFixture* fixture = (KeyEventFixture*)data;
    WebKitLoadStatus status = webkit_web_view_get_load_status(webView);
    if (status != WEBKIT_LOAD_FINISHED)
        return;

    GtkIMContext* imContext = 0;
    g_object_get(webView, "im-context", &imContext, NULL);
    g_assert(imContext);

    gchar* originalId = g_strdup(gtk_im_multicontext_get_context_id(GTK_IM_MULTICONTEXT(imContext)));
    gtk_im_multicontext_set_context_id(GTK_IM_MULTICONTEXT(imContext), "xim");

    // Test that commits that happen outside of key events
    // change the text field immediately. This closely replicates
    // the behavior of SCIM.
    fixture->info->text = g_strdup("debian");
    if (!gtk_test_widget_send_key(GTK_WIDGET(fixture->webView),
                                 gdk_unicode_to_keyval('d'), 0))
        g_assert_not_reached();
    if (!gtk_test_widget_send_key(GTK_WIDGET(fixture->webView),
                             gdk_unicode_to_keyval('e'), 0))
        g_assert_not_reached();
    if (!gtk_test_widget_send_key(GTK_WIDGET(fixture->webView),
                             gdk_unicode_to_keyval('b'), 0))
        g_assert_not_reached();
    if (!gtk_test_widget_send_key(GTK_WIDGET(fixture->webView),
                             gdk_unicode_to_keyval('i'), 0))
        g_assert_not_reached();
    if (!gtk_test_widget_send_key(GTK_WIDGET(fixture->webView),
                             gdk_unicode_to_keyval('a'), 0))
        g_assert_not_reached();
    if (!gtk_test_widget_send_key(GTK_WIDGET(fixture->webView),
                             gdk_unicode_to_keyval('n'), 0))
        g_assert_not_reached();

    gtk_im_multicontext_set_context_id(GTK_IM_MULTICONTEXT(imContext), originalId);
    g_free(originalId);
    g_object_unref(imContext);

    g_idle_add(verify_contents, fixture);
}

static void test_xim(KeyEventFixture* fixture, gconstpointer data)
{
    setup_keyevent_test(fixture, data, G_CALLBACK(test_xim_load_status_cb));
}
#endif

int main(int argc, char** argv)
{
    gtk_test_init(&argc, &argv, NULL);

    g_test_bug_base("https://bugs.webkit.org/");


    // We'll test input on a slew of different node types. Key events to
    // text inputs and editable divs should be marked as handled. Key events
    // to buttons and links should not.
    const char* textinput_html = "<html><body><input id=\"in\" type=\"text\">"
        "<script>document.getElementById('in').focus();</script></body></html>";
    const char* button_html = "<html><body><input id=\"in\" type=\"button\">"
        "<script>document.getElementById('in').focus();</script></body></html>";
    const char* link_html = "<html><body><a href=\"http://www.gnome.org\" id=\"in\">"
        "LINKY MCLINKERSON</a><script>document.getElementById('in').focus();</script>"
        "</body></html>";
    const char* div_html = "<html><body><div id=\"in\" contenteditable=\"true\">"
        "<script>document.getElementById('in').focus();</script></body></html>";

    // These are similar to the blocks above, but they should block the first
    // keypress modifying the editable node.
    const char* textinput_html_blocking = "<html><body>"
        "<input id=\"in\" type=\"text\" "
        "onkeypress=\"if (first) {event.preventDefault();first=false;}\">"
        "<script>first = true;\ndocument.getElementById('in').focus();</script>\n"
        "</script></body></html>";
    const char* div_html_blocking = "<html><body>"
        "<div id=\"in\" contenteditable=\"true\" "
        "onkeypress=\"if (first) {event.preventDefault();first=false;}\">"
        "<script>first = true; document.getElementById('in').focus();</script>\n"
        "</script></body></html>";

    g_test_add("/webkit/keyevents/event-textinput", KeyEventFixture,
               test_info_new(textinput_html, TRUE),
               key_event_fixture_setup,
               test_keypress_events,
               key_event_fixture_teardown);
    g_test_add("/webkit/keyevents/event-buttons", KeyEventFixture,
               test_info_new(button_html, FALSE),
               key_event_fixture_setup,
               test_keypress_events,
               key_event_fixture_teardown);
    g_test_add("/webkit/keyevents/event-link", KeyEventFixture,
               test_info_new(link_html, FALSE),
               key_event_fixture_setup,
               test_keypress_events,
               key_event_fixture_teardown);
    g_test_add("/webkit/keyevent/event-div", KeyEventFixture,
               test_info_new(div_html, TRUE),
               key_event_fixture_setup,
               test_keypress_events,
               key_event_fixture_teardown);
    g_test_add("/webkit/keyevent/ime-textinput", KeyEventFixture,
               test_info_new(textinput_html, TRUE),
               key_event_fixture_setup,
               test_ime,
               key_event_fixture_teardown);
    g_test_add("/webkit/keyevent/ime-div", KeyEventFixture,
               test_info_new(div_html, TRUE),
               key_event_fixture_setup,
               test_ime,
               key_event_fixture_teardown);
    g_test_add("/webkit/keyevent/block-textinput", KeyEventFixture,
               test_info_new(textinput_html_blocking, TRUE),
               key_event_fixture_setup,
               test_blocking,
               key_event_fixture_teardown);
    g_test_add("/webkit/keyevent/block-div", KeyEventFixture,
               test_info_new(div_html_blocking, TRUE),
               key_event_fixture_setup,
               test_blocking,
               key_event_fixture_teardown);
#if defined(GDK_WINDOWING_X11)
    g_test_add("/webkit/keyevent/xim-textinput", KeyEventFixture,
               test_info_new(textinput_html, TRUE),
               key_event_fixture_setup,
               test_xim,
               key_event_fixture_teardown);
    g_test_add("/webkit/keyevent/xim-div", KeyEventFixture,
               test_info_new(div_html, TRUE),
               key_event_fixture_setup,
               test_xim,
               key_event_fixture_teardown);
#endif

    return g_test_run();
}

