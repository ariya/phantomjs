/*
 * Copyright (C) 2011 Igalia S.L.
 * Portions Copyright (c) 2011 Motorola Mobility, Inc.  All rights reserved.
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

#include "config.h"
#include "WebViewTest.h"

#include <JavaScriptCore/JSRetainPtr.h>
#include <WebCore/GOwnPtrGtk.h>

WebViewTest::WebViewTest()
    : m_webView(WEBKIT_WEB_VIEW(g_object_ref_sink(webkit_web_view_new())))
    , m_mainLoop(g_main_loop_new(0, TRUE))
    , m_parentWindow(0)
    , m_javascriptResult(0)
    , m_resourceDataSize(0)
    , m_surface(0)
{
    assertObjectIsDeletedWhenTestFinishes(G_OBJECT(m_webView));
}

WebViewTest::~WebViewTest()
{
    if (m_parentWindow)
        gtk_widget_destroy(m_parentWindow);
    if (m_javascriptResult)
        webkit_javascript_result_unref(m_javascriptResult);
    if (m_surface)
        cairo_surface_destroy(m_surface);
    g_object_unref(m_webView);
    g_main_loop_unref(m_mainLoop);
}

void WebViewTest::loadURI(const char* uri)
{
    m_activeURI = uri;
    webkit_web_view_load_uri(m_webView, uri);
}

void WebViewTest::loadHtml(const char* html, const char* baseURI)
{
    if (!baseURI)
        m_activeURI = "about:blank";
    else
        m_activeURI = baseURI;
    webkit_web_view_load_html(m_webView, html, baseURI);
}

void WebViewTest::loadPlainText(const char* plainText)
{
    m_activeURI = "about:blank";
    webkit_web_view_load_plain_text(m_webView, plainText);
}

void WebViewTest::loadRequest(WebKitURIRequest* request)
{
    m_activeURI = webkit_uri_request_get_uri(request);
    webkit_web_view_load_request(m_webView, request);
}

void WebViewTest::loadAlternateHTML(const char* html, const char* contentURI, const char* baseURI)
{
    m_activeURI = contentURI;
    webkit_web_view_load_alternate_html(m_webView, html, contentURI, baseURI);
}

void WebViewTest::goBack()
{
    if (webkit_web_view_can_go_back(m_webView)) {
        WebKitBackForwardList* list = webkit_web_view_get_back_forward_list(m_webView);
        WebKitBackForwardListItem* item = webkit_back_forward_list_get_nth_item(list, -1);
        m_activeURI = webkit_back_forward_list_item_get_original_uri(item);
    }

    // Call go_back even when can_go_back returns FALSE to check nothing happens.
    webkit_web_view_go_back(m_webView);
}

void WebViewTest::goForward()
{
    if (webkit_web_view_can_go_forward(m_webView)) {
        WebKitBackForwardList* list = webkit_web_view_get_back_forward_list(m_webView);
        WebKitBackForwardListItem* item = webkit_back_forward_list_get_nth_item(list, 1);
        m_activeURI = webkit_back_forward_list_item_get_original_uri(item);
    }

    // Call go_forward even when can_go_forward returns FALSE to check nothing happens.
    webkit_web_view_go_forward(m_webView);
}

void WebViewTest::goToBackForwardListItem(WebKitBackForwardListItem* item)
{
    m_activeURI = webkit_back_forward_list_item_get_original_uri(item);
    webkit_web_view_go_to_back_forward_list_item(m_webView, item);
}

void WebViewTest::quitMainLoop()
{
    g_main_loop_quit(m_mainLoop);
}

void WebViewTest::quitMainLoopAfterProcessingPendingEvents()
{
    while (gtk_events_pending())
        gtk_main_iteration();
    quitMainLoop();
}

static gboolean quitMainLoopIdleCallback(WebViewTest* test)
{
    test->quitMainLoop();
    return FALSE;
}

void WebViewTest::wait(double seconds)
{
    g_timeout_add_seconds(seconds, reinterpret_cast<GSourceFunc>(quitMainLoopIdleCallback), this);
    g_main_loop_run(m_mainLoop);
}

static void loadChanged(WebKitWebView* webView, WebKitLoadEvent loadEvent, WebViewTest* test)
{
    if (loadEvent != WEBKIT_LOAD_FINISHED)
        return;
    g_signal_handlers_disconnect_by_func(webView, reinterpret_cast<void*>(loadChanged), test);
    g_main_loop_quit(test->m_mainLoop);
}

void WebViewTest::waitUntilLoadFinished()
{
    g_signal_connect(m_webView, "load-changed", G_CALLBACK(loadChanged), this);
    g_main_loop_run(m_mainLoop);
}

static void titleChanged(WebKitWebView* webView, GParamSpec*, WebViewTest* test)
{
    if (!test->m_expectedTitle.isNull() && test->m_expectedTitle != webkit_web_view_get_title(webView))
        return;

    g_signal_handlers_disconnect_by_func(webView, reinterpret_cast<void*>(titleChanged), test);
    g_main_loop_quit(test->m_mainLoop);
}

void WebViewTest::waitUntilTitleChangedTo(const char* expectedTitle)
{
    m_expectedTitle = expectedTitle;
    g_signal_connect(m_webView, "notify::title", G_CALLBACK(titleChanged), this);
    g_main_loop_run(m_mainLoop);
    m_expectedTitle = CString();
}

void WebViewTest::waitUntilTitleChanged()
{
    waitUntilTitleChangedTo(0);
}

static gboolean parentWindowMapped(GtkWidget* widget, GdkEvent*, WebViewTest* test)
{
    g_signal_handlers_disconnect_by_func(widget, reinterpret_cast<void*>(parentWindowMapped), test);
    g_main_loop_quit(test->m_mainLoop);

    return FALSE;
}

void WebViewTest::showInWindow(GtkWindowType windowType)
{
    g_assert(!m_parentWindow);
    m_parentWindow = gtk_window_new(windowType);
    gtk_container_add(GTK_CONTAINER(m_parentWindow), GTK_WIDGET(m_webView));
    gtk_widget_show(GTK_WIDGET(m_webView));
    gtk_widget_show(m_parentWindow);
}

void WebViewTest::showInWindowAndWaitUntilMapped(GtkWindowType windowType)
{
    g_assert(!m_parentWindow);
    m_parentWindow = gtk_window_new(windowType);
    gtk_container_add(GTK_CONTAINER(m_parentWindow), GTK_WIDGET(m_webView));
    gtk_widget_show(GTK_WIDGET(m_webView));

    g_signal_connect(m_parentWindow, "map-event", G_CALLBACK(parentWindowMapped), this);
    gtk_widget_show(m_parentWindow);
    g_main_loop_run(m_mainLoop);
}

void WebViewTest::resizeView(int width, int height)
{
    GtkAllocation allocation;
    gtk_widget_get_allocation(GTK_WIDGET(m_webView), &allocation);
    if (width != -1)
        allocation.width = width;
    if (height != -1)
        allocation.height = height;
    gtk_widget_size_allocate(GTK_WIDGET(m_webView), &allocation);
}

void WebViewTest::selectAll()
{
    webkit_web_view_execute_editing_command(m_webView, "SelectAll");
}

static void resourceGetDataCallback(GObject* object, GAsyncResult* result, gpointer userData)
{
    size_t dataSize;
    GOwnPtr<GError> error;
    unsigned char* data = webkit_web_resource_get_data_finish(WEBKIT_WEB_RESOURCE(object), result, &dataSize, &error.outPtr());
    g_assert(data);

    WebViewTest* test = static_cast<WebViewTest*>(userData);
    test->m_resourceData.set(reinterpret_cast<char*>(data));
    test->m_resourceDataSize = dataSize;
    g_main_loop_quit(test->m_mainLoop);
}

const char* WebViewTest::mainResourceData(size_t& mainResourceDataSize)
{
    m_resourceDataSize = 0;
    m_resourceData.clear();
    WebKitWebResource* resource = webkit_web_view_get_main_resource(m_webView);
    g_assert(resource);

    webkit_web_resource_get_data(resource, 0, resourceGetDataCallback, this);
    g_main_loop_run(m_mainLoop);

    mainResourceDataSize = m_resourceDataSize;
    return m_resourceData.get();
}

void WebViewTest::mouseMoveTo(int x, int y, unsigned int mouseModifiers)
{
    g_assert(m_parentWindow);
    GtkWidget* viewWidget = GTK_WIDGET(m_webView);
    g_assert(gtk_widget_get_realized(viewWidget));

    GOwnPtr<GdkEvent> event(gdk_event_new(GDK_MOTION_NOTIFY));
    event->motion.x = x;
    event->motion.y = y;

    event->motion.time = GDK_CURRENT_TIME;
    event->motion.window = gtk_widget_get_window(viewWidget);
    g_object_ref(event->motion.window);
    event->motion.device = gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gtk_widget_get_display(viewWidget)));
    event->motion.state = mouseModifiers;
    event->motion.axes = 0;

    int xRoot, yRoot;
    gdk_window_get_root_coords(gtk_widget_get_window(viewWidget), x, y, &xRoot, &yRoot);
    event->motion.x_root = xRoot;
    event->motion.y_root = yRoot;
    gtk_main_do_event(event.get());
}

void WebViewTest::clickMouseButton(int x, int y, unsigned int button, unsigned int mouseModifiers)
{
    doMouseButtonEvent(GDK_BUTTON_PRESS, x, y, button, mouseModifiers);
    doMouseButtonEvent(GDK_BUTTON_RELEASE, x, y, button, mouseModifiers);
}

void WebViewTest::keyStroke(unsigned int keyVal, unsigned int keyModifiers)
{
    g_assert(m_parentWindow);
    GtkWidget* viewWidget = GTK_WIDGET(m_webView);
    g_assert(gtk_widget_get_realized(viewWidget));

    GOwnPtr<GdkEvent> event(gdk_event_new(GDK_KEY_PRESS));
    event->key.keyval = keyVal;

    event->key.time = GDK_CURRENT_TIME;
    event->key.window = gtk_widget_get_window(viewWidget);
    g_object_ref(event->key.window);
    gdk_event_set_device(event.get(), gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gtk_widget_get_display(viewWidget))));
    event->key.state = keyModifiers;

    // When synthesizing an event, an invalid hardware_keycode value can cause it to be badly processed by GTK+.
    GOwnPtr<GdkKeymapKey> keys;
    int keysCount;
    if (gdk_keymap_get_entries_for_keyval(gdk_keymap_get_default(), keyVal, &keys.outPtr(), &keysCount))
        event->key.hardware_keycode = keys.get()[0].keycode;

    gtk_main_do_event(event.get());
    event->key.type = GDK_KEY_RELEASE;
    gtk_main_do_event(event.get());
}

void WebViewTest::doMouseButtonEvent(GdkEventType eventType, int x, int y, unsigned int button, unsigned int mouseModifiers)
{
    g_assert(m_parentWindow);
    GtkWidget* viewWidget = GTK_WIDGET(m_webView);
    g_assert(gtk_widget_get_realized(viewWidget));

    GOwnPtr<GdkEvent> event(gdk_event_new(eventType));
    event->button.window = gtk_widget_get_window(viewWidget);
    g_object_ref(event->button.window);

    event->button.time = GDK_CURRENT_TIME;
    event->button.x = x;
    event->button.y = y;
    event->button.axes = 0;
    event->button.state = mouseModifiers;
    event->button.button = button;

    event->button.device = gdk_device_manager_get_client_pointer(gdk_display_get_device_manager(gtk_widget_get_display(viewWidget)));

    int xRoot, yRoot;
    gdk_window_get_root_coords(gtk_widget_get_window(viewWidget), x, y, &xRoot, &yRoot);
    event->button.x_root = xRoot;
    event->button.y_root = yRoot;
    gtk_main_do_event(event.get());
}

static void runJavaScriptReadyCallback(GObject*, GAsyncResult* result, WebViewTest* test)
{
    test->m_javascriptResult = webkit_web_view_run_javascript_finish(test->m_webView, result, test->m_javascriptError);
    g_main_loop_quit(test->m_mainLoop);
}

static void runJavaScriptFromGResourceReadyCallback(GObject*, GAsyncResult* result, WebViewTest* test)
{
    test->m_javascriptResult = webkit_web_view_run_javascript_from_gresource_finish(test->m_webView, result, test->m_javascriptError);
    g_main_loop_quit(test->m_mainLoop);
}

WebKitJavascriptResult* WebViewTest::runJavaScriptAndWaitUntilFinished(const char* javascript, GError** error)
{
    if (m_javascriptResult)
        webkit_javascript_result_unref(m_javascriptResult);
    m_javascriptResult = 0;
    m_javascriptError = error;
    webkit_web_view_run_javascript(m_webView, javascript, 0, reinterpret_cast<GAsyncReadyCallback>(runJavaScriptReadyCallback), this);
    g_main_loop_run(m_mainLoop);

    return m_javascriptResult;
}

WebKitJavascriptResult* WebViewTest::runJavaScriptFromGResourceAndWaitUntilFinished(const char* resource, GError** error)
{
    if (m_javascriptResult)
        webkit_javascript_result_unref(m_javascriptResult);
    m_javascriptResult = 0;
    m_javascriptError = error;
    webkit_web_view_run_javascript_from_gresource(m_webView, resource, 0, reinterpret_cast<GAsyncReadyCallback>(runJavaScriptFromGResourceReadyCallback), this);
    g_main_loop_run(m_mainLoop);

    return m_javascriptResult;
}

static char* jsValueToCString(JSGlobalContextRef context, JSValueRef value)
{
    g_assert(value);
    g_assert(JSValueIsString(context, value));

    JSRetainPtr<JSStringRef> stringValue(Adopt, JSValueToStringCopy(context, value, 0));
    g_assert(stringValue);

    size_t cStringLength = JSStringGetMaximumUTF8CStringSize(stringValue.get());
    char* cString = static_cast<char*>(g_malloc(cStringLength));
    JSStringGetUTF8CString(stringValue.get(), cString, cStringLength);
    return cString;
}

char* WebViewTest::javascriptResultToCString(WebKitJavascriptResult* javascriptResult)
{
    JSGlobalContextRef context = webkit_javascript_result_get_global_context(javascriptResult);
    g_assert(context);
    return jsValueToCString(context, webkit_javascript_result_get_value(javascriptResult));
}

double WebViewTest::javascriptResultToNumber(WebKitJavascriptResult* javascriptResult)
{
    JSGlobalContextRef context = webkit_javascript_result_get_global_context(javascriptResult);
    g_assert(context);
    JSValueRef value = webkit_javascript_result_get_value(javascriptResult);
    g_assert(value);
    g_assert(JSValueIsNumber(context, value));

    return JSValueToNumber(context, value, 0);
}

bool WebViewTest::javascriptResultToBoolean(WebKitJavascriptResult* javascriptResult)
{
    JSGlobalContextRef context = webkit_javascript_result_get_global_context(javascriptResult);
    g_assert(context);
    JSValueRef value = webkit_javascript_result_get_value(javascriptResult);
    g_assert(value);
    g_assert(JSValueIsBoolean(context, value));

    return JSValueToBoolean(context, value);
}

bool WebViewTest::javascriptResultIsNull(WebKitJavascriptResult* javascriptResult)
{
    JSGlobalContextRef context = webkit_javascript_result_get_global_context(javascriptResult);
    g_assert(context);
    JSValueRef value = webkit_javascript_result_get_value(javascriptResult);
    g_assert(value);

    return JSValueIsNull(context, value);
}

bool WebViewTest::javascriptResultIsUndefined(WebKitJavascriptResult* javascriptResult)
{
    JSGlobalContextRef context = webkit_javascript_result_get_global_context(javascriptResult);
    g_assert(context);
    JSValueRef value = webkit_javascript_result_get_value(javascriptResult);
    g_assert(value);

    return JSValueIsUndefined(context, value);
}

static void onSnapshotReady(WebKitWebView* web_view, GAsyncResult* res, WebViewTest* test)
{
    GOwnPtr<GError> error;
    test->m_surface = webkit_web_view_get_snapshot_finish(web_view, res, &error.outPtr());
    g_assert(!test->m_surface || !error.get());
    if (error)
        g_assert_error(error.get(), WEBKIT_SNAPSHOT_ERROR, WEBKIT_SNAPSHOT_ERROR_FAILED_TO_CREATE);
    test->quitMainLoop();
}

cairo_surface_t* WebViewTest::getSnapshotAndWaitUntilReady(WebKitSnapshotRegion region, WebKitSnapshotOptions options)
{
    if (m_surface)
        cairo_surface_destroy(m_surface);
    m_surface = 0;
    webkit_web_view_get_snapshot(m_webView, region, options, 0, reinterpret_cast<GAsyncReadyCallback>(onSnapshotReady), this);
    g_main_loop_run(m_mainLoop);
    return m_surface;
}
