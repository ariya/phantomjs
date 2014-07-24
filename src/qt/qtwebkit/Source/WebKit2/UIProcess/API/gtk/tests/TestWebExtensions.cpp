/*
 * Copyright (C) 2012 Igalia S.L.
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

#include "config.h"

#include "WebKitTestBus.h"
#include "WebViewTest.h"
#include <wtf/gobject/GRefPtr.h>

static WebKitTestBus* bus;

static void testWebExtensionGetTitle(WebViewTest* test, gconstpointer)
{
    test->loadHtml("<html><head><title>WebKitGTK+ Web Extensions Test</title></head><body></body></html>", 0);
    test->waitUntilLoadFinished();

    GRefPtr<GDBusProxy> proxy = adoptGRef(bus->createProxy("org.webkit.gtk.WebExtensionTest",
        "/org/webkit/gtk/WebExtensionTest" , "org.webkit.gtk.WebExtensionTest", test->m_mainLoop));
    GRefPtr<GVariant> result = adoptGRef(g_dbus_proxy_call_sync(
        proxy.get(),
        "GetTitle",
        g_variant_new("(t)", webkit_web_view_get_page_id(test->m_webView)),
        G_DBUS_CALL_FLAGS_NONE,
        -1, 0, 0));
    g_assert(result);

    const char* title;
    g_variant_get(result.get(), "(&s)", &title);
    g_assert_cmpstr(title, ==, "WebKitGTK+ Web Extensions Test");
}

static void documentLoadedCallback(GDBusConnection*, const char*, const char*, const char*, const char*, GVariant*, WebViewTest* test)
{
    g_main_loop_quit(test->m_mainLoop);
}

static void testDocumentLoadedSignal(WebViewTest* test, gconstpointer)
{
    GRefPtr<GDBusProxy> proxy = adoptGRef(bus->createProxy("org.webkit.gtk.WebExtensionTest",
        "/org/webkit/gtk/WebExtensionTest", "org.webkit.gtk.WebExtensionTest", test->m_mainLoop));
    GDBusConnection* connection = g_dbus_proxy_get_connection(proxy.get());
    guint id = g_dbus_connection_signal_subscribe(connection,
        0,
        "org.webkit.gtk.WebExtensionTest",
        "DocumentLoaded",
        "/org/webkit/gtk/WebExtensionTest",
        0,
        G_DBUS_SIGNAL_FLAGS_NONE,
        reinterpret_cast<GDBusSignalCallback>(documentLoadedCallback),
        test,
        0);
    g_assert(id);

    test->loadHtml("<html><head><title>WebKitGTK+ Web Extensions Test</title></head><body></body></html>", 0);
    g_main_loop_run(test->m_mainLoop);
    g_dbus_connection_signal_unsubscribe(connection, id);
}

static gboolean webProcessCrashedCallback(WebKitWebView*, WebViewTest* test)
{
    test->quitMainLoop();

    return FALSE;
}

static void testWebKitWebViewProcessCrashed(WebViewTest* test, gconstpointer)
{
    test->loadHtml("<html></html>", 0);
    test->waitUntilLoadFinished();

    g_signal_connect(test->m_webView, "web-process-crashed",
        G_CALLBACK(webProcessCrashedCallback), test);

    GRefPtr<GDBusProxy> proxy = adoptGRef(bus->createProxy("org.webkit.gtk.WebExtensionTest",
        "/org/webkit/gtk/WebExtensionTest", "org.webkit.gtk.WebExtensionTest", test->m_mainLoop));

    GRefPtr<GVariant> result = adoptGRef(g_dbus_proxy_call_sync(
        proxy.get(),
        "AbortProcess",
        0,
        G_DBUS_CALL_FLAGS_NONE,
        -1, 0, 0));
    g_assert(!result);
    g_main_loop_run(test->m_mainLoop);
}

void beforeAll()
{
    webkit_web_context_set_web_extensions_directory(webkit_web_context_get_default(), WEBKIT_TEST_WEB_EXTENSIONS_DIR);
    bus = new WebKitTestBus();
    if (!bus->run())
        return;

    WebViewTest::add("WebKitWebExtension", "dom-document-title", testWebExtensionGetTitle);
    WebViewTest::add("WebKitWebExtension", "document-loaded-signal", testDocumentLoadedSignal);
    WebViewTest::add("WebKitWebView", "web-process-crashed", testWebKitWebViewProcessCrashed);
}

void afterAll()
{
    delete bus;
}
