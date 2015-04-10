/*
 * Copyright (C) 2010 Igalia S.L.
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
#include <gtk/gtk.h>
#include <webkit/webkit.h>

#define HTML_DOCUMENT "<html><head><title></title></head><style type='text/css'>#test { font-size: 16px; }</style><body><p id='test'>test</p></body></html>"

typedef struct {
    GtkWidget* webView;
    GtkWidget* window;
    WebKitDOMDOMWindow* domWindow;
    GMainLoop* loop;

    gboolean loaded;
    gboolean clicked;
    gconstpointer data;
} DomDomviewFixture;

static gboolean finish_loading(DomDomviewFixture* fixture)
{
    if (g_main_loop_is_running(fixture->loop))
        g_main_loop_quit(fixture->loop);

    return FALSE;
}

static void dom_domview_fixture_setup(DomDomviewFixture* fixture, gconstpointer data)
{
    fixture->loop = g_main_loop_new(NULL, TRUE);
    fixture->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    fixture->webView = webkit_web_view_new();
    fixture->data = data;

    gtk_container_add(GTK_CONTAINER(fixture->window), GTK_WIDGET(fixture->webView));
}

static void dom_domview_fixture_teardown(DomDomviewFixture* fixture, gconstpointer data)
{
    gtk_widget_destroy(fixture->window);
    g_main_loop_unref(fixture->loop);
}

static void dom_dom_window_fixture_setup(DomDomviewFixture* fixture, gconstpointer data)
{
    fixture->loop = g_main_loop_new(NULL, TRUE);
    fixture->webView = webkit_web_view_new();
    g_object_ref_sink(fixture->webView);

    if (data != NULL)
        webkit_web_view_load_string(WEBKIT_WEB_VIEW (fixture->webView), (const char*) data, NULL, NULL, NULL);

    g_idle_add((GSourceFunc)finish_loading, fixture);
    g_main_loop_run(fixture->loop);
}

static void dom_dom_window_fixture_teardown(DomDomviewFixture* fixture, gconstpointer data)
{
    if (fixture->webView)
        g_object_unref(fixture->webView);
    g_main_loop_unref(fixture->loop);
}

static gboolean loadedCallback(WebKitDOMDOMWindow* view, WebKitDOMEvent* event, DomDomviewFixture* fixture)
{
    g_assert(fixture->loaded == FALSE);
    fixture->loaded = TRUE;

    return FALSE;
}

static gboolean clickedCallback(WebKitDOMDOMWindow* view, WebKitDOMEvent* event, DomDomviewFixture* fixture)
{
    WebKitDOMEventTarget* target;
    gushort phase;

    g_assert(event);
    g_assert(WEBKIT_DOM_IS_EVENT(event));

    // We should catch this in the bubbling up phase, since we are connecting to the toplevel object
    phase = webkit_dom_event_get_event_phase(event);
    g_assert_cmpint(phase, ==, 3);

    target = webkit_dom_event_get_current_target(event);
    g_assert(target == WEBKIT_DOM_EVENT_TARGET(view));

    g_assert(fixture->clicked == FALSE);
    fixture->clicked = TRUE;

    finish_loading(fixture);

    return FALSE;
}

gboolean map_event_cb(GtkWidget *widget, GdkEvent* event, DomDomviewFixture* fixture)
{
    webkit_web_view_load_string(WEBKIT_WEB_VIEW (fixture->webView), (const char*)fixture->data, NULL, NULL, NULL);

    return FALSE;
}

static void load_event_callback(WebKitWebView* webView, GParamSpec* spec, DomDomviewFixture* fixture)
{
    WebKitLoadStatus status = webkit_web_view_get_load_status(webView);
    if (status == WEBKIT_LOAD_FINISHED) {
        webkit_dom_event_target_add_event_listener(WEBKIT_DOM_EVENT_TARGET(fixture->domWindow), "click", G_CALLBACK(clickedCallback), false, fixture);

        g_assert(fixture->clicked == FALSE);
        gtk_test_widget_click(GTK_WIDGET(fixture->webView), 1, 0);
    }

}

static void test_dom_domview_signals(DomDomviewFixture* fixture, gconstpointer data)
{
    g_assert(fixture);
    WebKitWebView* view = (WebKitWebView*)fixture->webView;
    g_assert(view);
    WebKitDOMDocument* document = webkit_web_view_get_dom_document(view);
    g_assert(document);
    WebKitDOMDOMWindow* domWindow = webkit_dom_document_get_default_view(document);
    g_assert(domWindow);

    fixture->domWindow = domWindow;

    webkit_dom_event_target_add_event_listener(WEBKIT_DOM_EVENT_TARGET(fixture->domWindow), "load", G_CALLBACK(loadedCallback), false, fixture);
    g_signal_connect(fixture->window, "map-event", G_CALLBACK(map_event_cb), fixture);
    g_signal_connect(fixture->webView, "notify::load-status", G_CALLBACK(load_event_callback), fixture);

    gtk_widget_show_all(fixture->window);
    gtk_window_present(GTK_WINDOW(fixture->window));

    g_main_loop_run(fixture->loop);

    g_assert(fixture->loaded);
    g_assert(fixture->clicked);
}

static gboolean
clicked_cb(WebKitDOMEventTarget* target, WebKitDOMEvent* event, DomDomviewFixture* fixture)
{
    g_assert(fixture->clicked == FALSE);
    fixture->clicked = TRUE;
    finish_loading(fixture);
    return FALSE;
}

static void load_status_callback(WebKitWebView* webView, GParamSpec* spec, DomDomviewFixture* fixture)
{
    WebKitLoadStatus status = webkit_web_view_get_load_status(webView);
    if (status == WEBKIT_LOAD_FINISHED) {
        WebKitDOMDocument* document;
        WebKitDOMDOMWindow* domWindow;
        WebKitDOMElement* element;
        WebKitDOMEvent* event;
        glong clientX, clientY;

        document = webkit_web_view_get_dom_document(WEBKIT_WEB_VIEW(fixture->webView));
        g_assert(document);
        domWindow = webkit_dom_document_get_default_view(document);
        g_assert(domWindow);
        fixture->domWindow = domWindow;

        element = webkit_dom_document_get_element_by_id(document, "test");
        g_assert(element);
        event = webkit_dom_document_create_event(document, "MouseEvent", NULL);
        g_assert(event);
        g_assert(WEBKIT_DOM_IS_EVENT(event));
        g_assert(WEBKIT_DOM_IS_MOUSE_EVENT(event));
        clientX = webkit_dom_element_get_client_left(element);
        clientY = webkit_dom_element_get_client_top(element);
        webkit_dom_mouse_event_init_mouse_event(WEBKIT_DOM_MOUSE_EVENT(event),
                                                "click", TRUE, TRUE,
                                                fixture->domWindow, 0, 0, 0, clientX, clientY,
                                                FALSE, FALSE, FALSE, FALSE,
                                                1, WEBKIT_DOM_EVENT_TARGET(element));
        webkit_dom_event_target_add_event_listener(WEBKIT_DOM_EVENT_TARGET(element), "click", G_CALLBACK(clicked_cb), false, fixture);
        g_assert(fixture->clicked == FALSE);
        webkit_dom_event_target_dispatch_event(WEBKIT_DOM_EVENT_TARGET(element), event, NULL);
    }

}

static void test_dom_domview_dispatch_event(DomDomviewFixture* fixture, gconstpointer data)
{
    g_signal_connect(fixture->window, "map-event", G_CALLBACK(map_event_cb), fixture);
    g_signal_connect(fixture->webView, "notify::load-status", G_CALLBACK(load_status_callback), fixture);

    gtk_widget_show_all(fixture->window);
    gtk_window_present(GTK_WINDOW(fixture->window));

    g_main_loop_run (fixture->loop);
    g_assert(fixture->clicked);
}

static void test_dom_dom_window_get_computed_style(DomDomviewFixture* fixture, gconstpointer data)
{
    WebKitDOMDocument* document = webkit_web_view_get_dom_document(WEBKIT_WEB_VIEW(fixture->webView));
    g_assert(document);
    WebKitDOMDOMWindow* domWindow = webkit_dom_document_get_default_view(document);
    g_assert(domWindow);

    WebKitDOMElement*  element = webkit_dom_document_get_element_by_id(document, "test");
    g_assert(element);
    g_assert(WEBKIT_DOM_IS_ELEMENT(element));
    WebKitDOMCSSStyleDeclaration* cssStyle = webkit_dom_dom_window_get_computed_style(domWindow, element, NULL);
    gchar* fontSize = webkit_dom_css_style_declaration_get_property_value(cssStyle, "font-size");
    g_assert_cmpstr(fontSize, ==, "16px");
}

int main(int argc, char** argv)
{
    gtk_test_init(&argc, &argv, NULL);

    g_test_bug_base("https://bugs.webkit.org/");

    g_test_add("/webkit/domdomview/signals",
               DomDomviewFixture, HTML_DOCUMENT,
               dom_domview_fixture_setup,
               test_dom_domview_signals,
               dom_domview_fixture_teardown);

    g_test_add("/webkit/domdomview/dispatch_event",
               DomDomviewFixture, HTML_DOCUMENT,
               dom_domview_fixture_setup,
               test_dom_domview_dispatch_event,
               dom_domview_fixture_teardown);

    g_test_add("/webkit/domdomwindow/get_computed_style",
               DomDomviewFixture, HTML_DOCUMENT,
               dom_dom_window_fixture_setup,
               test_dom_dom_window_get_computed_style,
               dom_dom_window_fixture_teardown);

    return g_test_run();
}

