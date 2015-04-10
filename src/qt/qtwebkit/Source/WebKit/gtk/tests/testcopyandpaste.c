/*
 * Copyright (C) 2010 Igalia S.L.
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
    char* expectedContent;
} TestInfo;

typedef struct {
    GtkWidget* window;
    WebKitWebView* webView;
    GMainLoop* loop;
    TestInfo* info;
} CopyAndPasteFixture;

TestInfo*
test_info_new(const char* page, const char* expectedContent)
{
    TestInfo* info;
    info = g_slice_new0(TestInfo);
    info->page = g_strdup(page);
    if (expectedContent)
        info->expectedContent = g_strdup(expectedContent);
    return info;
}

void
test_info_destroy(TestInfo* info)
{
    g_free(info->page);
    g_free(info->expectedContent);
    g_slice_free(TestInfo, info);
}

static void copy_and_paste_fixture_setup(CopyAndPasteFixture* fixture, gconstpointer data)
{
    fixture->loop = g_main_loop_new(NULL, TRUE);

    fixture->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    fixture->webView = WEBKIT_WEB_VIEW(webkit_web_view_new());

    gtk_container_add(GTK_CONTAINER(fixture->window), GTK_WIDGET(fixture->webView));
}

static void copy_and_paste_fixture_teardown(CopyAndPasteFixture* fixture, gconstpointer data)
{
    gtk_widget_destroy(fixture->window);
    g_main_loop_unref(fixture->loop);
    test_info_destroy(fixture->info);
}

static void load_status_cb(WebKitWebView* webView, GParamSpec* spec, gpointer data)
{
    CopyAndPasteFixture* fixture = (CopyAndPasteFixture*)data;
    WebKitLoadStatus status = webkit_web_view_get_load_status(webView);
    if (status != WEBKIT_LOAD_FINISHED)
        return;

    GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_clear(clipboard);

    webkit_web_view_copy_clipboard(webView);

    gchar* text = gtk_clipboard_wait_for_text(clipboard);
    g_assert(text || !fixture->info->expectedContent);
    g_assert(!text || !strcmp(text, fixture->info->expectedContent));
    g_free(text);

    // Verify that the markup starts with the proper content-type meta tag prefix.
    GtkSelectionData* selectionData = gtk_clipboard_wait_for_contents(clipboard, gdk_atom_intern("text/html", FALSE));
    if (selectionData) {
        static const char* markupPrefix = "<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\">";
        char* markup = g_strndup((const char*) gtk_selection_data_get_data(selectionData),
            gtk_selection_data_get_length(selectionData));
        g_assert(strlen(markupPrefix) <= strlen(markup));
        g_assert(!strncmp(markupPrefix, markup, strlen(markupPrefix)));
        g_free(markup);
    }

    g_assert(!gtk_clipboard_wait_is_uris_available(clipboard));
    g_assert(!gtk_clipboard_wait_is_image_available(clipboard));

    g_main_loop_quit(fixture->loop);
}

gboolean map_event_cb(GtkWidget *widget, GdkEvent* event, gpointer data)
{
    CopyAndPasteFixture* fixture = (CopyAndPasteFixture*)data;
    webkit_web_view_load_string(fixture->webView, fixture->info->page,
                                "text/html", "utf-8", "file://");
    return FALSE;
}

static void test_copy_and_paste(CopyAndPasteFixture* fixture, gconstpointer data)
{
    fixture->info = (TestInfo*)data;
    g_signal_connect(fixture->window, "map-event",
                     G_CALLBACK(map_event_cb), fixture);

    gtk_widget_show(fixture->window);
    gtk_widget_show(GTK_WIDGET(fixture->webView));
    gtk_window_present(GTK_WINDOW(fixture->window));
    gtk_widget_grab_focus(GTK_WIDGET(fixture->webView));

    g_signal_connect(fixture->webView, "notify::load-status",
                     G_CALLBACK(load_status_cb), fixture);

    g_main_loop_run(fixture->loop);
}

static CopyAndPasteFixture* currentFixture;
static JSValueRef runPasteTestCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    gtk_widget_grab_focus(GTK_WIDGET(currentFixture->webView));

    // Simulate a paste keyboard sequence.
    GdkEvent* event = gdk_event_new(GDK_KEY_PRESS);
    event->key.keyval = gdk_unicode_to_keyval('v');
    event->key.state = GDK_CONTROL_MASK;
    event->key.window = gtk_widget_get_window(GTK_WIDGET(currentFixture->webView));
    g_object_ref(event->key.window);
#ifndef GTK_API_VERSION_2
    GdkDeviceManager* manager =  gdk_display_get_device_manager(gdk_window_get_display(event->key.window));
    gdk_event_set_device(event, gdk_device_manager_get_client_pointer(manager));
#endif

    GdkKeymapKey* keys;
    gint n_keys;
    if (gdk_keymap_get_entries_for_keyval(gdk_keymap_get_default(), event->key.keyval, &keys, &n_keys)) {
        event->key.hardware_keycode = keys[0].keycode;
        g_free(keys);
    }

    gtk_main_do_event(event);
    event->key.type = GDK_KEY_RELEASE;
    gtk_main_do_event(event);
    gdk_event_free(event);

    JSStringRef scriptString = JSStringCreateWithUTF8CString("document.body.innerHTML;");
    JSValueRef value = JSEvaluateScript(context, scriptString, 0, 0, 0, 0);
    JSStringRelease(scriptString);

    g_assert(JSValueIsString(context, value));
    JSStringRef actual = JSValueToStringCopy(context, value, exception);
    g_assert(!exception || !*exception);
    g_assert(currentFixture->info->expectedContent);
    JSStringRef expected = JSStringCreateWithUTF8CString(currentFixture->info->expectedContent);
    g_assert(JSStringIsEqual(expected, actual));

    JSStringRelease(expected);
    JSStringRelease(actual);
    g_main_loop_quit(currentFixture->loop);
    return JSValueMakeUndefined(context);
}

static void window_object_cleared_callback(WebKitWebView* web_view, WebKitWebFrame* web_frame, JSGlobalContextRef context, JSObjectRef window_object, gpointer data)
{
    JSStringRef name = JSStringCreateWithUTF8CString("runTest");
    JSObjectRef testComplete = JSObjectMakeFunctionWithCallback(context, name, runPasteTestCallback);
    JSObjectSetProperty(context, window_object, name, testComplete, kJSPropertyAttributeNone, 0);
    JSStringRelease(name);
}

static void pasting_test_get_data_callback(GtkClipboard* clipboard, GtkSelectionData* selection_data, guint info, gpointer data)
{
    gtk_selection_data_set(selection_data, gdk_atom_intern("text/html", FALSE), 8, (const guchar*) data, strlen((char*)data) + 1);
}

static void pasting_test_clear_data_callback(GtkClipboard* clipboard, gpointer data)
{
    g_free(data);
}

static void test_pasting_markup(CopyAndPasteFixture* fixture, gconstpointer data)
{
    fixture->info = (TestInfo*)data;
    currentFixture = fixture;

    GtkTargetList* targetList = gtk_target_list_new(0, 0);
    gtk_target_list_add(targetList, gdk_atom_intern("text/html", FALSE), 0, 0);

    int numberOfTargets = 1;
    GtkTargetEntry* targetTable = gtk_target_table_new_from_list(targetList, &numberOfTargets);
    gtk_clipboard_set_with_data(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
                                targetTable, numberOfTargets,
                                pasting_test_get_data_callback,
                                pasting_test_clear_data_callback,
                                g_strdup(fixture->info->expectedContent));
    gtk_target_list_unref(targetList);
    gtk_target_table_free(targetTable, numberOfTargets);

    g_signal_connect(fixture->window, "map-event",
                     G_CALLBACK(map_event_cb), fixture);
    g_signal_connect(fixture->webView, "window-object-cleared",
                     G_CALLBACK(window_object_cleared_callback), fixture);

    gtk_widget_show(fixture->window);
    gtk_widget_show(GTK_WIDGET(fixture->webView));
    gtk_window_present(GTK_WINDOW(fixture->window));

    g_main_loop_run(fixture->loop);
}


int main(int argc, char** argv)
{
    gtk_test_init(&argc, &argv, NULL);

    g_test_bug_base("https://bugs.webkit.org/");
    const char* selected_span_html = "<html><body>"
        "<span id=\"mainspan\">All work and no play <span>make Jack a dull</span> boy.</span>"
        "<script>document.getSelection().collapse();\n"
        "document.getSelection().selectAllChildren(document.getElementById('mainspan'));\n"
        "</script></body></html>";
    const char* no_selection_html = "<html><body>"
        "<span id=\"mainspan\">All work and no play <span>make Jack a dull</span> boy</span>"
        "<script>document.getSelection().collapse();\n"
        "</script></body></html>";

    g_test_add("/webkit/copyandpaste/selection", CopyAndPasteFixture,
               test_info_new(selected_span_html, "All work and no play make Jack a dull boy."),
               copy_and_paste_fixture_setup,
               test_copy_and_paste,
               copy_and_paste_fixture_teardown);
    g_test_add("/webkit/copyandpaste/no-selection", CopyAndPasteFixture,
               test_info_new(no_selection_html, 0),
               copy_and_paste_fixture_setup,
               test_copy_and_paste,
               copy_and_paste_fixture_teardown);

    const char* paste_test_html = "<html>"
        "<body onLoad=\"document.body.focus(); runTest();\" contentEditable=\"true\">"
        "</body></html>";
    g_test_add("/webkit/copyandpaste/paste-markup", CopyAndPasteFixture,
               test_info_new(paste_test_html, "bobby"),
               copy_and_paste_fixture_setup,
               test_pasting_markup,
               copy_and_paste_fixture_teardown);

    return g_test_run();
}
