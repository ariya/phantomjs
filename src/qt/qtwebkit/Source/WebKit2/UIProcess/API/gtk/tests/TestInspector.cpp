/*
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

#include "config.h"

#include "WebViewTest.h"
#include <wtf/gobject/GRefPtr.h>

class InspectorTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(InspectorTest);

    enum InspectorEvents {
        OpenWindow,
        BringToFront,
        Closed,
        Attach,
        Detach
    };

    static gboolean openWindowCallback(WebKitWebInspector*, InspectorTest* test)
    {
        return test->openWindow();
    }

    static gboolean bringToFrontCallback(WebKitWebInspector*, InspectorTest* test)
    {
        return test->bringToFront();
    }

    static void closedCallback(WebKitWebInspector*, InspectorTest* test)
    {
        return test->closed();
    }

    static gboolean attachCallback(WebKitWebInspector*, InspectorTest* test)
    {
        return test->attach();
    }

    static gboolean detachCallback(WebKitWebInspector*, InspectorTest* test)
    {
        return test->detach();
    }

    static const unsigned gMinimumAttachedInspectorWidth = 750;
    static const unsigned gMinimumAttachedInspectorHeight = 250;

    InspectorTest()
        : WebViewTest()
        , m_inspector(webkit_web_view_get_inspector(m_webView))
    {
        webkit_settings_set_enable_developer_extras(webkit_web_view_get_settings(m_webView), TRUE);
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(m_inspector));
        g_signal_connect(m_inspector, "open-window", G_CALLBACK(openWindowCallback), this);
        g_signal_connect(m_inspector, "bring-to-front", G_CALLBACK(bringToFrontCallback), this);
        g_signal_connect(m_inspector, "closed", G_CALLBACK(closedCallback), this);
        g_signal_connect(m_inspector, "attach", G_CALLBACK(attachCallback), this);
        g_signal_connect(m_inspector, "detach", G_CALLBACK(detachCallback), this);
    }

    ~InspectorTest()
    {
        g_signal_handlers_disconnect_matched(m_inspector, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
    }

    virtual bool openWindow()
    {
        m_events.append(OpenWindow);
        g_main_loop_quit(m_mainLoop);
        return TRUE;
    }

    virtual bool bringToFront()
    {
        m_events.append(BringToFront);
        g_main_loop_quit(m_mainLoop);
        return FALSE;
    }

    virtual void closed()
    {
        m_events.append(Closed);
    }

    virtual bool attach()
    {
        m_events.append(Attach);
        return TRUE;
    }

    virtual bool detach()
    {
        m_events.append(Detach);
        return TRUE;
    }


    static gboolean showIdle(InspectorTest* test)
    {
        webkit_web_inspector_show(test->m_inspector);
        return FALSE;
    }

    void show()
    {
        g_idle_add(reinterpret_cast<GSourceFunc>(showIdle), this);
        g_main_loop_run(m_mainLoop);
    }

    void resizeViewAndAttach()
    {
        // Resize the view to make room for the inspector.
        resizeView(gMinimumAttachedInspectorWidth, (gMinimumAttachedInspectorHeight + 1) * 4 / 3);
        webkit_web_inspector_attach(m_inspector);
    }

    static gboolean detachIdle(InspectorTest* test)
    {
        webkit_web_inspector_detach(test->m_inspector);
        return FALSE;
    }

    void detachAndWaitUntilWindowOpened()
    {
        g_idle_add(reinterpret_cast<GSourceFunc>(detachIdle), this);
        g_main_loop_run(m_mainLoop);
    }

    void close()
    {
        webkit_web_inspector_close(m_inspector);
    }

    WebKitWebInspector* m_inspector;
    Vector<InspectorEvents> m_events;
};

static void testInspectorDefault(InspectorTest* test, gconstpointer)
{
    test->showInWindowAndWaitUntilMapped(GTK_WINDOW_TOPLEVEL);
    test->resizeView(200, 200);
    test->loadHtml("<html><body><p>WebKitGTK+ Inspector test</p></body></html>", 0);
    test->waitUntilLoadFinished();

    test->show();
    // We don't add the view to a container, so consume the weak ref with GRefPtr.
    GRefPtr<WebKitWebViewBase> inspectorView = webkit_web_inspector_get_web_view(test->m_inspector);
    g_assert(inspectorView.get());
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(inspectorView.get()));
    g_assert(!webkit_web_inspector_is_attached(test->m_inspector));
    g_assert_cmpuint(webkit_web_inspector_get_attached_height(test->m_inspector), ==, 0);
    Vector<InspectorTest::InspectorEvents>& events = test->m_events;
    g_assert_cmpint(events.size(), ==, 1);
    g_assert_cmpint(events[0], ==, InspectorTest::OpenWindow);
    test->m_events.clear();

    test->show();
    events = test->m_events;
    g_assert_cmpint(events.size(), ==, 1);
    g_assert_cmpint(events[0], ==, InspectorTest::BringToFront);
    test->m_events.clear();

    test->resizeViewAndAttach();
    g_assert(webkit_web_inspector_is_attached(test->m_inspector));
    g_assert_cmpuint(webkit_web_inspector_get_attached_height(test->m_inspector), >=, InspectorTest::gMinimumAttachedInspectorHeight);
    events = test->m_events;
    g_assert_cmpint(events.size(), ==, 1);
    g_assert_cmpint(events[0], ==, InspectorTest::Attach);
    test->m_events.clear();

    test->detachAndWaitUntilWindowOpened();
    g_assert(!webkit_web_inspector_is_attached(test->m_inspector));
    events = test->m_events;
    g_assert_cmpint(events.size(), ==, 2);
    g_assert_cmpint(events[0], ==, InspectorTest::Detach);
    g_assert_cmpint(events[1], ==, InspectorTest::OpenWindow);
    test->m_events.clear();

    test->close();
    events = test->m_events;
    g_assert_cmpint(events.size(), ==, 1);
    g_assert_cmpint(events[0], ==, InspectorTest::Closed);
    test->m_events.clear();
}

class CustomInspectorTest: public InspectorTest {
public:
    MAKE_GLIB_TEST_FIXTURE(CustomInspectorTest);

    CustomInspectorTest()
        : InspectorTest()
        , m_inspectorWindow(0)
    {
    }

    ~CustomInspectorTest()
    {
        if (m_inspectorWindow)
            gtk_widget_destroy(m_inspectorWindow);
    }

    bool openWindow()
    {
        g_assert(!m_inspectorWindow);
        m_inspectorWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        WebKitWebViewBase* inspectorView = webkit_web_inspector_get_web_view(m_inspector);
        g_assert(inspectorView);
        gtk_container_add(GTK_CONTAINER(m_inspectorWindow), GTK_WIDGET(inspectorView));
        gtk_widget_show_all(m_inspectorWindow);

        return InspectorTest::openWindow();
    }

    void closed()
    {
        if (m_inspectorWindow) {
            gtk_widget_destroy(m_inspectorWindow);
            m_inspectorWindow = 0;
        }

        return InspectorTest::closed();
    }

    bool attach()
    {
        GRefPtr<WebKitWebViewBase> inspectorView = webkit_web_inspector_get_web_view(m_inspector);
        if (m_inspectorWindow) {
            gtk_container_remove(GTK_CONTAINER(m_inspectorWindow), GTK_WIDGET(inspectorView.get()));
            gtk_widget_destroy(m_inspectorWindow);
            m_inspectorWindow = 0;
        }

        GtkWidget* pane;
        if (gtk_bin_get_child(GTK_BIN(m_parentWindow)) == GTK_WIDGET(m_webView)) {
            GRefPtr<WebKitWebView> inspectedView = m_webView;
            gtk_container_remove(GTK_CONTAINER(m_parentWindow), GTK_WIDGET(m_webView));
            pane = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
            gtk_paned_add1(GTK_PANED(pane), GTK_WIDGET(m_webView));
            gtk_container_add(GTK_CONTAINER(m_parentWindow), pane);
            gtk_widget_show_all(pane);
        } else
            pane = gtk_bin_get_child(GTK_BIN(m_parentWindow));
        gtk_paned_set_position(GTK_PANED(pane), webkit_web_inspector_get_attached_height(m_inspector));
        gtk_paned_add2(GTK_PANED(pane), GTK_WIDGET(inspectorView.get()));

        return InspectorTest::attach();
    }

    bool detach()
    {
        GRefPtr<WebKitWebViewBase> inspectorView = webkit_web_inspector_get_web_view(m_inspector);
        GtkWidget* pane = gtk_bin_get_child(GTK_BIN(m_parentWindow));
        g_assert(GTK_IS_PANED(pane));
        gtk_container_remove(GTK_CONTAINER(pane), GTK_WIDGET(inspectorView.get()));
        return InspectorTest::detach();
    }

    void destroyWindow()
    {
        g_assert(m_inspectorWindow);
        gtk_widget_destroy(m_inspectorWindow);
        m_inspectorWindow = 0;
    }

    GtkWidget* m_inspectorWindow;
};

static void testInspectorManualAttachDetach(CustomInspectorTest* test, gconstpointer)
{
    test->showInWindowAndWaitUntilMapped(GTK_WINDOW_TOPLEVEL);
    test->resizeView(200, 200);
    test->loadHtml("<html><body><p>WebKitGTK+ Inspector test</p></body></html>", 0);
    test->waitUntilLoadFinished();

    test->show();
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(webkit_web_inspector_get_web_view(test->m_inspector)));
    g_assert(!webkit_web_inspector_is_attached(test->m_inspector));
    Vector<InspectorTest::InspectorEvents>& events = test->m_events;
    g_assert_cmpint(events.size(), ==, 1);
    g_assert_cmpint(events[0], ==, InspectorTest::OpenWindow);
    test->m_events.clear();

    test->resizeViewAndAttach();
    g_assert(webkit_web_inspector_is_attached(test->m_inspector));
    g_assert_cmpuint(webkit_web_inspector_get_attached_height(test->m_inspector), >=, InspectorTest::gMinimumAttachedInspectorHeight);
    events = test->m_events;
    g_assert_cmpint(events.size(), ==, 1);
    g_assert_cmpint(events[0], ==, InspectorTest::Attach);
    test->m_events.clear();

    test->detachAndWaitUntilWindowOpened();
    g_assert(!webkit_web_inspector_is_attached(test->m_inspector));
    events = test->m_events;
    g_assert_cmpint(events.size(), ==, 2);
    g_assert_cmpint(events[0], ==, InspectorTest::Detach);
    g_assert_cmpint(events[1], ==, InspectorTest::OpenWindow);
    test->m_events.clear();

    test->resizeViewAndAttach();
    g_assert(webkit_web_inspector_is_attached(test->m_inspector));
    test->m_events.clear();
    test->close();
    events = test->m_events;
    g_assert_cmpint(events.size(), ==, 2);
    g_assert_cmpint(events[0], ==, InspectorTest::Detach);
    g_assert_cmpint(events[1], ==, InspectorTest::Closed);
    test->m_events.clear();
}

static void testInspectorCustomContainerDestroyed(CustomInspectorTest* test, gconstpointer)
{
    test->showInWindowAndWaitUntilMapped(GTK_WINDOW_TOPLEVEL);
    test->resizeView(200, 200);
    test->loadHtml("<html><body><p>WebKitGTK+ Inspector test</p></body></html>", 0);
    test->waitUntilLoadFinished();

    test->show();
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(webkit_web_inspector_get_web_view(test->m_inspector)));
    g_assert(!webkit_web_inspector_is_attached(test->m_inspector));

    test->m_events.clear();
    test->destroyWindow();
    Vector<InspectorTest::InspectorEvents>& events = test->m_events;
    g_assert_cmpint(events.size(), ==, 1);
    g_assert_cmpint(events[0], ==, InspectorTest::Closed);
    test->m_events.clear();
}

void beforeAll()
{
    g_setenv("WEBKIT_INSPECTOR_PATH", WEBKIT_INSPECTOR_PATH, FALSE);
    InspectorTest::add("WebKitWebInspector", "default", testInspectorDefault);
    CustomInspectorTest::add("WebKitWebInspector", "manual-attach-detach", testInspectorManualAttachDetach);
    CustomInspectorTest::add("WebKitWebInspector", "custom-container-destroyed", testInspectorCustomContainerDestroyed);
}

void afterAll()
{
}
