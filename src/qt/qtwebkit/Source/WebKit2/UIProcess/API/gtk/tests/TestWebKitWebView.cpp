/*
 * Copyright (C) 2011 Igalia S.L.
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
#include "WebViewTest.h"
#include <JavaScriptCore/JSStringRef.h>
#include <JavaScriptCore/JSValueRef.h>
#include <glib/gstdio.h>
#include <wtf/HashSet.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/StringHash.h>

static void testWebViewDefaultContext(WebViewTest* test, gconstpointer)
{
    g_assert(webkit_web_view_get_context(test->m_webView) == webkit_web_context_get_default());

    // Check that a web view created with g_object_new has the default context.
    GRefPtr<WebKitWebView> webView = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW, NULL));
    g_assert(webkit_web_view_get_context(webView.get()) == webkit_web_context_get_default());
}

static void testWebViewCustomCharset(WebViewTest* test, gconstpointer)
{
    g_assert(!webkit_web_view_get_custom_charset(test->m_webView));
    webkit_web_view_set_custom_charset(test->m_webView, "utf8");
    g_assert_cmpstr(webkit_web_view_get_custom_charset(test->m_webView), ==, "utf8");
    // Go back to the default charset.
    webkit_web_view_set_custom_charset(test->m_webView, 0);
    g_assert(!webkit_web_view_get_custom_charset(test->m_webView));
}

static void testWebViewSettings(WebViewTest* test, gconstpointer)
{
    WebKitSettings* defaultSettings = webkit_web_view_get_settings(test->m_webView);
    g_assert(defaultSettings);
    g_assert(webkit_settings_get_enable_javascript(defaultSettings));

    GRefPtr<WebKitSettings> newSettings = adoptGRef(webkit_settings_new());
    g_object_set(G_OBJECT(newSettings.get()), "enable-javascript", FALSE, NULL);
    webkit_web_view_set_settings(test->m_webView, newSettings.get());

    WebKitSettings* settings = webkit_web_view_get_settings(test->m_webView);
    g_assert(settings != defaultSettings);
    g_assert(!webkit_settings_get_enable_javascript(settings));

    GRefPtr<GtkWidget> webView2 = webkit_web_view_new();
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(webView2.get()));
    webkit_web_view_set_settings(WEBKIT_WEB_VIEW(webView2.get()), settings);
    g_assert(webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webView2.get())) == settings);

    GRefPtr<WebKitSettings> newSettings2 = adoptGRef(webkit_settings_new());
    webkit_web_view_set_settings(WEBKIT_WEB_VIEW(webView2.get()), newSettings2.get());
    settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webView2.get()));
    g_assert(settings == newSettings2.get());
    g_assert(webkit_settings_get_enable_javascript(settings));
}

static const char* kAlertDialogMessage = "WebKitGTK+ alert dialog message";
static const char* kConfirmDialogMessage = "WebKitGTK+ confirm dialog message";
static const char* kPromptDialogMessage = "WebKitGTK+ prompt dialog message";
static const char* kPromptDialogReturnedText = "WebKitGTK+ prompt dialog returned text";

class UIClientTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(UIClientTest);

    enum WebViewEvents {
        Create,
        ReadyToShow,
        RunAsModal,
        Close
    };

    class WindowProperties {
    public:
        WindowProperties()
            : m_isNull(true)
            , m_toolbarVisible(true)
            , m_statusbarVisible(true)
            , m_scrollbarsVisible(true)
            , m_menubarVisible(true)
            , m_locationbarVisible(true)
            , m_resizable(true)
            , m_fullscreen(false)
        {
            memset(&m_geometry, 0, sizeof(GdkRectangle));
        }

        WindowProperties(WebKitWindowProperties* windowProperties)
            : m_isNull(false)
            , m_toolbarVisible(webkit_window_properties_get_toolbar_visible(windowProperties))
            , m_statusbarVisible(webkit_window_properties_get_statusbar_visible(windowProperties))
            , m_scrollbarsVisible(webkit_window_properties_get_scrollbars_visible(windowProperties))
            , m_menubarVisible(webkit_window_properties_get_menubar_visible(windowProperties))
            , m_locationbarVisible(webkit_window_properties_get_locationbar_visible(windowProperties))
            , m_resizable(webkit_window_properties_get_resizable(windowProperties))
            , m_fullscreen(webkit_window_properties_get_fullscreen(windowProperties))
        {
            webkit_window_properties_get_geometry(windowProperties, &m_geometry);
        }

        WindowProperties(GdkRectangle* geometry, bool toolbarVisible, bool statusbarVisible, bool scrollbarsVisible, bool menubarVisible,
                         bool locationbarVisible, bool resizable, bool fullscreen)
            : m_isNull(false)
            , m_geometry(*geometry)
            , m_toolbarVisible(toolbarVisible)
            , m_statusbarVisible(statusbarVisible)
            , m_scrollbarsVisible(scrollbarsVisible)
            , m_menubarVisible(menubarVisible)
            , m_locationbarVisible(locationbarVisible)
            , m_resizable(resizable)
            , m_fullscreen(fullscreen)
        {
        }

        bool isNull() const { return m_isNull; }

        void assertEqual(const WindowProperties& other) const
        {
            g_assert_cmpint(m_geometry.x, ==, other.m_geometry.x);
            g_assert_cmpint(m_geometry.y, ==, other.m_geometry.y);
            g_assert_cmpint(m_geometry.width, ==, other.m_geometry.width);
            g_assert_cmpint(m_geometry.height, ==, other.m_geometry.height);
            g_assert_cmpint(static_cast<int>(m_toolbarVisible), ==, static_cast<int>(other.m_toolbarVisible));
            g_assert_cmpint(static_cast<int>(m_statusbarVisible), ==, static_cast<int>(other.m_statusbarVisible));
            g_assert_cmpint(static_cast<int>(m_scrollbarsVisible), ==, static_cast<int>(other.m_scrollbarsVisible));
            g_assert_cmpint(static_cast<int>(m_menubarVisible), ==, static_cast<int>(other.m_menubarVisible));
            g_assert_cmpint(static_cast<int>(m_locationbarVisible), ==, static_cast<int>(other.m_locationbarVisible));
            g_assert_cmpint(static_cast<int>(m_resizable), ==, static_cast<int>(other.m_resizable));
            g_assert_cmpint(static_cast<int>(m_fullscreen), ==, static_cast<int>(other.m_fullscreen));
        }

    private:
        bool m_isNull;

        GdkRectangle m_geometry;

        bool m_toolbarVisible;
        bool m_statusbarVisible;
        bool m_scrollbarsVisible;
        bool m_menubarVisible;
        bool m_locationbarVisible;

        bool m_resizable;
        bool m_fullscreen;
    };

    static void windowPropertiesNotifyCallback(GObject*, GParamSpec* paramSpec, UIClientTest* test)
    {
        test->m_windowPropertiesChanged.add(g_param_spec_get_name(paramSpec));
    }

    static GtkWidget* viewCreateCallback(WebKitWebView* webView, UIClientTest* test)
    {
        return test->viewCreate(webView);
    }

    static void viewReadyToShowCallback(WebKitWebView* webView, UIClientTest* test)
    {
        test->viewReadyToShow(webView);
    }

    static void viewCloseCallback(WebKitWebView* webView, UIClientTest* test)
    {
        test->viewClose(webView);
    }

    void scriptAlert(WebKitScriptDialog* dialog)
    {
        switch (m_scriptDialogType) {
        case WEBKIT_SCRIPT_DIALOG_ALERT:
            g_assert_cmpstr(webkit_script_dialog_get_message(dialog), ==, kAlertDialogMessage);
            break;
        case WEBKIT_SCRIPT_DIALOG_CONFIRM:
            g_assert(m_scriptDialogConfirmed);
            g_assert_cmpstr(webkit_script_dialog_get_message(dialog), ==, "confirmed");

            break;
        case WEBKIT_SCRIPT_DIALOG_PROMPT:
            g_assert_cmpstr(webkit_script_dialog_get_message(dialog), ==, kPromptDialogReturnedText);
            break;
        }

        g_main_loop_quit(m_mainLoop);
    }

    void scriptConfirm(WebKitScriptDialog* dialog)
    {
        g_assert_cmpstr(webkit_script_dialog_get_message(dialog), ==, kConfirmDialogMessage);
        m_scriptDialogConfirmed = !m_scriptDialogConfirmed;
        webkit_script_dialog_confirm_set_confirmed(dialog, m_scriptDialogConfirmed);
    }

    void scriptPrompt(WebKitScriptDialog* dialog)
    {
        g_assert_cmpstr(webkit_script_dialog_get_message(dialog), ==, kPromptDialogMessage);
        g_assert_cmpstr(webkit_script_dialog_prompt_get_default_text(dialog), ==, "default");
        webkit_script_dialog_prompt_set_text(dialog, kPromptDialogReturnedText);
    }

    static gboolean scriptDialog(WebKitWebView*, WebKitScriptDialog* dialog, UIClientTest* test)
    {
        switch (webkit_script_dialog_get_dialog_type(dialog)) {
        case WEBKIT_SCRIPT_DIALOG_ALERT:
            test->scriptAlert(dialog);
            break;
        case WEBKIT_SCRIPT_DIALOG_CONFIRM:
            test->scriptConfirm(dialog);
            break;
        case WEBKIT_SCRIPT_DIALOG_PROMPT:
            test->scriptPrompt(dialog);
            break;
        }

        return TRUE;
    }

    static void mouseTargetChanged(WebKitWebView*, WebKitHitTestResult* hitTestResult, guint modifiers, UIClientTest* test)
    {
        g_assert(WEBKIT_IS_HIT_TEST_RESULT(hitTestResult));
        test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(hitTestResult));

        test->m_mouseTargetHitTestResult = hitTestResult;
        test->m_mouseTargetModifiers = modifiers;
        g_main_loop_quit(test->m_mainLoop);
    }

    static gboolean permissionRequested(WebKitWebView*, WebKitPermissionRequest* request, UIClientTest* test)
    {
        g_assert(WEBKIT_IS_PERMISSION_REQUEST(request));
        test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(request));

        if (test->m_allowPermissionRequests)
            webkit_permission_request_allow(request);
        else
            webkit_permission_request_deny(request);

        return TRUE;
    }

    UIClientTest()
        : m_scriptDialogType(WEBKIT_SCRIPT_DIALOG_ALERT)
        , m_scriptDialogConfirmed(true)
        , m_allowPermissionRequests(false)
        , m_mouseTargetModifiers(0)
    {
        webkit_settings_set_javascript_can_open_windows_automatically(webkit_web_view_get_settings(m_webView), TRUE);
        g_signal_connect(m_webView, "create", G_CALLBACK(viewCreateCallback), this);
        g_signal_connect(m_webView, "script-dialog", G_CALLBACK(scriptDialog), this);
        g_signal_connect(m_webView, "mouse-target-changed", G_CALLBACK(mouseTargetChanged), this);
        g_signal_connect(m_webView, "permission-request", G_CALLBACK(permissionRequested), this);
    }

    ~UIClientTest()
    {
        g_signal_handlers_disconnect_matched(m_webView, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
    }

    void waitUntilMainLoopFinishes()
    {
        g_main_loop_run(m_mainLoop);
    }

    void setExpectedWindowProperties(const WindowProperties& windowProperties)
    {
        m_windowProperties = windowProperties;
    }

    WebKitHitTestResult* moveMouseAndWaitUntilMouseTargetChanged(int x, int y, unsigned int mouseModifiers = 0)
    {
        mouseMoveTo(x, y, mouseModifiers);
        g_main_loop_run(m_mainLoop);
        return m_mouseTargetHitTestResult.get();
    }

    virtual GtkWidget* viewCreate(WebKitWebView* webView)
    {
        g_assert(webView == m_webView);

        GtkWidget* newWebView = webkit_web_view_new_with_context(webkit_web_view_get_context(webView));
        g_object_ref_sink(newWebView);

        m_webViewEvents.append(Create);

        WebKitWindowProperties* windowProperties = webkit_web_view_get_window_properties(WEBKIT_WEB_VIEW(newWebView));
        g_assert(windowProperties);
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(windowProperties));
        m_windowPropertiesChanged.clear();

        g_signal_connect(windowProperties, "notify", G_CALLBACK(windowPropertiesNotifyCallback), this);
        g_signal_connect(newWebView, "ready-to-show", G_CALLBACK(viewReadyToShowCallback), this);
        g_signal_connect(newWebView, "close", G_CALLBACK(viewCloseCallback), this);

        return newWebView;
    }

    virtual void viewReadyToShow(WebKitWebView* webView)
    {
        g_assert(webView != m_webView);

        WebKitWindowProperties* windowProperties = webkit_web_view_get_window_properties(webView);
        g_assert(windowProperties);
        if (!m_windowProperties.isNull())
            WindowProperties(windowProperties).assertEqual(m_windowProperties);

        m_webViewEvents.append(ReadyToShow);
    }

    virtual void viewClose(WebKitWebView* webView)
    {
        g_assert(webView != m_webView);

        m_webViewEvents.append(Close);
        g_object_unref(webView);

        g_main_loop_quit(m_mainLoop);
    }

    Vector<WebViewEvents> m_webViewEvents;
    WebKitScriptDialogType m_scriptDialogType;
    bool m_scriptDialogConfirmed;
    bool m_allowPermissionRequests;
    WindowProperties m_windowProperties;
    HashSet<WTF::String> m_windowPropertiesChanged;
    GRefPtr<WebKitHitTestResult> m_mouseTargetHitTestResult;
    unsigned int m_mouseTargetModifiers;
};

static void testWebViewCreateReadyClose(UIClientTest* test, gconstpointer)
{
    test->loadHtml("<html><body onLoad=\"window.open().close();\"></html>", 0);
    test->waitUntilMainLoopFinishes();

    Vector<UIClientTest::WebViewEvents>& events = test->m_webViewEvents;
    g_assert_cmpint(events.size(), ==, 3);
    g_assert_cmpint(events[0], ==, UIClientTest::Create);
    g_assert_cmpint(events[1], ==, UIClientTest::ReadyToShow);
    g_assert_cmpint(events[2], ==, UIClientTest::Close);
}

static gboolean checkMimeTypeForFilter(GtkFileFilter* filter, const gchar* mimeType)
{
    GtkFileFilterInfo filterInfo;
    filterInfo.contains = GTK_FILE_FILTER_MIME_TYPE;
    filterInfo.mime_type = mimeType;
    return gtk_file_filter_filter(filter, &filterInfo);
}

class ModalDialogsTest: public UIClientTest {
public:
    MAKE_GLIB_TEST_FIXTURE(ModalDialogsTest);

    static void dialogRunAsModalCallback(WebKitWebView* webView, ModalDialogsTest* test)
    {
        g_assert(webView != test->m_webView);
        test->m_webViewEvents.append(RunAsModal);
    }

    GtkWidget* viewCreate(WebKitWebView* webView)
    {
        g_assert(webView == m_webView);

        GtkWidget* newWebView = UIClientTest::viewCreate(webView);
        g_signal_connect(newWebView, "run-as-modal", G_CALLBACK(dialogRunAsModalCallback), this);
        return newWebView;
    }

    void viewReadyToShow(WebKitWebView* webView)
    {
        g_assert(webView != m_webView);
        m_webViewEvents.append(ReadyToShow);
    }
};

static void testWebViewAllowModalDialogs(ModalDialogsTest* test, gconstpointer)
{
    WebKitSettings* settings = webkit_web_view_get_settings(test->m_webView);
    webkit_settings_set_allow_modal_dialogs(settings, TRUE);

    test->loadHtml("<html><body onload=\"window.showModalDialog('data:text/html,<html><body/><script>window.close();</script></html>')\"></body></html>", 0);
    test->waitUntilMainLoopFinishes();

    Vector<UIClientTest::WebViewEvents>& events = test->m_webViewEvents;
    g_assert_cmpint(events.size(), ==, 4);
    g_assert_cmpint(events[0], ==, UIClientTest::Create);
    g_assert_cmpint(events[1], ==, UIClientTest::ReadyToShow);
    g_assert_cmpint(events[2], ==, UIClientTest::RunAsModal);
    g_assert_cmpint(events[3], ==, UIClientTest::Close);
}

static void testWebViewDisallowModalDialogs(ModalDialogsTest* test, gconstpointer)
{
    WebKitSettings* settings = webkit_web_view_get_settings(test->m_webView);
    webkit_settings_set_allow_modal_dialogs(settings, FALSE);

    test->loadHtml("<html><body onload=\"window.showModalDialog('data:text/html,<html><body/><script>window.close();</script></html>')\"></body></html>", 0);
    // We need to use a timeout here because the viewClose() function
    // won't ever be called as the dialog won't be created.
    test->wait(1);

    Vector<UIClientTest::WebViewEvents>& events = test->m_webViewEvents;
    g_assert_cmpint(events.size(), ==, 0);
}

static void testWebViewJavaScriptDialogs(UIClientTest* test, gconstpointer)
{
    static const char* htmlOnLoadFormat = "<html><body onLoad=\"%s\"></body></html>";
    static const char* jsAlertFormat = "alert('%s')";
    static const char* jsConfirmFormat = "do { confirmed = confirm('%s'); } while (!confirmed); alert('confirmed');";
    static const char* jsPromptFormat = "alert(prompt('%s', 'default'));";

    test->m_scriptDialogType = WEBKIT_SCRIPT_DIALOG_ALERT;
    GOwnPtr<char> alertDialogMessage(g_strdup_printf(jsAlertFormat, kAlertDialogMessage));
    GOwnPtr<char> alertHTML(g_strdup_printf(htmlOnLoadFormat, alertDialogMessage.get()));
    test->loadHtml(alertHTML.get(), 0);
    test->waitUntilMainLoopFinishes();

    test->m_scriptDialogType = WEBKIT_SCRIPT_DIALOG_CONFIRM;
    GOwnPtr<char> confirmDialogMessage(g_strdup_printf(jsConfirmFormat, kConfirmDialogMessage));
    GOwnPtr<char> confirmHTML(g_strdup_printf(htmlOnLoadFormat, confirmDialogMessage.get()));
    test->loadHtml(confirmHTML.get(), 0);
    test->waitUntilMainLoopFinishes();

    test->m_scriptDialogType = WEBKIT_SCRIPT_DIALOG_PROMPT;
    GOwnPtr<char> promptDialogMessage(g_strdup_printf(jsPromptFormat, kPromptDialogMessage));
    GOwnPtr<char> promptHTML(g_strdup_printf(htmlOnLoadFormat, promptDialogMessage.get()));
    test->loadHtml(promptHTML.get(), 0);
    test->waitUntilMainLoopFinishes();
}

static void testWebViewWindowProperties(UIClientTest* test, gconstpointer)
{
    static const char* windowProrpertiesString = "left=100,top=150,width=400,height=400,location=no,menubar=no,status=no,toolbar=no,scrollbars=no";
    GdkRectangle geometry = { 100, 150, 400, 400 };
    test->setExpectedWindowProperties(UIClientTest::WindowProperties(&geometry, false, false, false, false, false, true, false));

    GOwnPtr<char> htmlString(g_strdup_printf("<html><body onLoad=\"window.open('', '', '%s').close();\"></body></html>", windowProrpertiesString));
    test->loadHtml(htmlString.get(), 0);
    test->waitUntilMainLoopFinishes();

    static const char* propertiesChanged[] = {
        "geometry", "locationbar-visible", "menubar-visible", "statusbar-visible", "toolbar-visible", "scrollbars-visible"
    };
    for (size_t i = 0; i < G_N_ELEMENTS(propertiesChanged); ++i)
        g_assert(test->m_windowPropertiesChanged.contains(propertiesChanged[i]));

    Vector<UIClientTest::WebViewEvents>& events = test->m_webViewEvents;
    g_assert_cmpint(events.size(), ==, 3);
    g_assert_cmpint(events[0], ==, UIClientTest::Create);
    g_assert_cmpint(events[1], ==, UIClientTest::ReadyToShow);
    g_assert_cmpint(events[2], ==, UIClientTest::Close);
}

static void testWebViewMouseTarget(UIClientTest* test, gconstpointer)
{
    test->showInWindowAndWaitUntilMapped(GTK_WINDOW_TOPLEVEL);

    const char* linksHoveredHTML =
        "<html><body>"
        " <a style='position:absolute; left:1; top:1' href='http://www.webkitgtk.org' title='WebKitGTK+ Title'>WebKitGTK+ Website</a>"
        " <img style='position:absolute; left:1; top:10' src='0xdeadbeef' width=5 height=5></img>"
        " <a style='position:absolute; left:1; top:20' href='http://www.webkitgtk.org/logo' title='WebKitGTK+ Logo'><img src='0xdeadbeef' width=5 height=5></img></a>"
        " <input style='position:absolute; left:1; top:30' size='10'></input>"
        " <div style='position:absolute; left:1; top:50; width:30; height:30; overflow:scroll'>&nbsp;</div>"
        " <video style='position:absolute; left:1; top:100' width='300' height='300' controls='controls' preload='none'><source src='movie.ogg' type='video/ogg' /></video>"
        "</body></html>";

    test->loadHtml(linksHoveredHTML, "file:///");
    test->waitUntilLoadFinished();

    // Move over link.
    WebKitHitTestResult* hitTestResult = test->moveMouseAndWaitUntilMouseTargetChanged(1, 1);
    g_assert(webkit_hit_test_result_context_is_link(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_image(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_media(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_editable(hitTestResult));
    g_assert_cmpstr(webkit_hit_test_result_get_link_uri(hitTestResult), ==, "http://www.webkitgtk.org/");
    g_assert_cmpstr(webkit_hit_test_result_get_link_title(hitTestResult), ==, "WebKitGTK+ Title");
    g_assert_cmpstr(webkit_hit_test_result_get_link_label(hitTestResult), ==, "WebKitGTK+ Website");
    g_assert(!test->m_mouseTargetModifiers);

    // Move out of the link.
    hitTestResult = test->moveMouseAndWaitUntilMouseTargetChanged(0, 0);
    g_assert(!webkit_hit_test_result_context_is_link(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_image(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_media(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_editable(hitTestResult));
    g_assert(!test->m_mouseTargetModifiers);

    // Move over image with GDK_CONTROL_MASK.
    hitTestResult = test->moveMouseAndWaitUntilMouseTargetChanged(1, 10, GDK_CONTROL_MASK);
    g_assert(!webkit_hit_test_result_context_is_link(hitTestResult));
    g_assert(webkit_hit_test_result_context_is_image(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_media(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_editable(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_scrollbar(hitTestResult));
    g_assert_cmpstr(webkit_hit_test_result_get_image_uri(hitTestResult), ==, "file:///0xdeadbeef");
    g_assert(test->m_mouseTargetModifiers & GDK_CONTROL_MASK);

    // Move over image link.
    hitTestResult = test->moveMouseAndWaitUntilMouseTargetChanged(1, 20);
    g_assert(webkit_hit_test_result_context_is_link(hitTestResult));
    g_assert(webkit_hit_test_result_context_is_image(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_media(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_editable(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_scrollbar(hitTestResult));
    g_assert_cmpstr(webkit_hit_test_result_get_link_uri(hitTestResult), ==, "http://www.webkitgtk.org/logo");
    g_assert_cmpstr(webkit_hit_test_result_get_image_uri(hitTestResult), ==, "file:///0xdeadbeef");
    g_assert_cmpstr(webkit_hit_test_result_get_link_title(hitTestResult), ==, "WebKitGTK+ Logo");
    g_assert(!webkit_hit_test_result_get_link_label(hitTestResult));
    g_assert(!test->m_mouseTargetModifiers);

    // Move over media.
    hitTestResult = test->moveMouseAndWaitUntilMouseTargetChanged(1, 100);
    g_assert(!webkit_hit_test_result_context_is_link(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_image(hitTestResult));
    g_assert(webkit_hit_test_result_context_is_media(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_editable(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_scrollbar(hitTestResult));
    g_assert_cmpstr(webkit_hit_test_result_get_media_uri(hitTestResult), ==, "file:///movie.ogg");
    g_assert(!test->m_mouseTargetModifiers);

    // Mover over input.
    hitTestResult = test->moveMouseAndWaitUntilMouseTargetChanged(5, 35);
    g_assert(!webkit_hit_test_result_context_is_link(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_image(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_media(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_scrollbar(hitTestResult));
    g_assert(webkit_hit_test_result_context_is_editable(hitTestResult));
    g_assert(!test->m_mouseTargetModifiers);

    // Move over scrollbar.
    hitTestResult = test->moveMouseAndWaitUntilMouseTargetChanged(5, 75);
    g_assert(!webkit_hit_test_result_context_is_link(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_image(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_media(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_editable(hitTestResult));
    g_assert(webkit_hit_test_result_context_is_scrollbar(hitTestResult));
    g_assert(!test->m_mouseTargetModifiers);
}

static void testWebViewPermissionRequests(UIClientTest* test, gconstpointer)
{
    // Some versions of geoclue give a runtime warning because it tries
    // to register the error quark twice. See https://bugs.webkit.org/show_bug.cgi?id=89858.
    // Make warnings non-fatal for this test to make it pass.
    test->removeLogFatalFlag(G_LOG_LEVEL_WARNING);
    test->showInWindowAndWaitUntilMapped();
    static const char* geolocationRequestHTML =
        "<html>"
        "  <script>"
        "  function runTest()"
        "  {"
        "    navigator.geolocation.getCurrentPosition(function(p) { document.title = \"OK\" },"
        "                                             function(e) { document.title = e.code });"
        "  }"
        "  </script>"
        "  <body onload='runTest();'></body>"
        "</html>";

    // Test denying a permission request.
    test->m_allowPermissionRequests = false;
    test->loadHtml(geolocationRequestHTML, 0);
    test->waitUntilTitleChanged();

    // According to the Geolocation API specification, '1' is the
    // error code returned for the PERMISSION_DENIED error.
    // http://dev.w3.org/geo/api/spec-source.html#position_error_interface
    const gchar* result = webkit_web_view_get_title(test->m_webView);
    g_assert_cmpstr(result, ==, "1");

    // Test allowing a permission request.
    test->m_allowPermissionRequests = true;
    test->loadHtml(geolocationRequestHTML, 0);
    test->waitUntilTitleChanged();

    // Check that we did not get the PERMISSION_DENIED error now.
    result = webkit_web_view_get_title(test->m_webView);
    g_assert_cmpstr(result, !=, "1");
    test->addLogFatalFlag(G_LOG_LEVEL_WARNING);
}

static void testWebViewZoomLevel(WebViewTest* test, gconstpointer)
{
    g_assert_cmpfloat(webkit_web_view_get_zoom_level(test->m_webView), ==, 1);
    webkit_web_view_set_zoom_level(test->m_webView, 2.5);
    g_assert_cmpfloat(webkit_web_view_get_zoom_level(test->m_webView), ==, 2.5);

    webkit_settings_set_zoom_text_only(webkit_web_view_get_settings(test->m_webView), TRUE);
    // The zoom level shouldn't change when zoom-text-only setting changes.
    g_assert_cmpfloat(webkit_web_view_get_zoom_level(test->m_webView), ==, 2.5);
}

static void testWebViewRunJavaScript(WebViewTest* test, gconstpointer)
{
    static const char* html = "<html><body><a id='WebKitLink' href='http://www.webkitgtk.org/' title='WebKitGTK+ Title'>WebKitGTK+ Website</a></body></html>";
    test->loadHtml(html, 0);
    test->waitUntilLoadFinished();

    GOwnPtr<GError> error;
    WebKitJavascriptResult* javascriptResult = test->runJavaScriptAndWaitUntilFinished("window.document.getElementById('WebKitLink').title;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    GOwnPtr<char> valueString(WebViewTest::javascriptResultToCString(javascriptResult));
    g_assert_cmpstr(valueString.get(), ==, "WebKitGTK+ Title");

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("window.document.getElementById('WebKitLink').href;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    valueString.set(WebViewTest::javascriptResultToCString(javascriptResult));
    g_assert_cmpstr(valueString.get(), ==, "http://www.webkitgtk.org/");

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("window.document.getElementById('WebKitLink').textContent", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    valueString.set(WebViewTest::javascriptResultToCString(javascriptResult));
    g_assert_cmpstr(valueString.get(), ==, "WebKitGTK+ Website");

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("a = 25;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    g_assert_cmpfloat(WebViewTest::javascriptResultToNumber(javascriptResult), ==, 25);

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("a = 2.5;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    g_assert_cmpfloat(WebViewTest::javascriptResultToNumber(javascriptResult), ==, 2.5);

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("a = true", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    g_assert(WebViewTest::javascriptResultToBoolean(javascriptResult));

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("a = false", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    g_assert(!WebViewTest::javascriptResultToBoolean(javascriptResult));

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("a = null", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    g_assert(WebViewTest::javascriptResultIsNull(javascriptResult));

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("function Foo() { a = 25; } Foo();", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    g_assert(WebViewTest::javascriptResultIsUndefined(javascriptResult));

    javascriptResult = test->runJavaScriptFromGResourceAndWaitUntilFinished("/org/webkit/webkit2gtk/tests/link-title.js", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    valueString.set(WebViewTest::javascriptResultToCString(javascriptResult));
    g_assert_cmpstr(valueString.get(), ==, "WebKitGTK+ Title");

    javascriptResult = test->runJavaScriptFromGResourceAndWaitUntilFinished("/wrong/path/to/resource.js", &error.outPtr());
    g_assert(!javascriptResult);
    g_assert_error(error.get(), G_RESOURCE_ERROR, G_RESOURCE_ERROR_NOT_FOUND);
    error.clear();

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("foo();", &error.outPtr());
    g_assert(!javascriptResult);
    g_assert_error(error.get(), WEBKIT_JAVASCRIPT_ERROR, WEBKIT_JAVASCRIPT_ERROR_SCRIPT_FAILED);
}

class FileChooserTest: public UIClientTest {
public:
    MAKE_GLIB_TEST_FIXTURE(FileChooserTest);

    FileChooserTest()
    {
        g_signal_connect(m_webView, "run-file-chooser", G_CALLBACK(runFileChooserCallback), this);
    }

    static gboolean runFileChooserCallback(WebKitWebView*, WebKitFileChooserRequest* request, FileChooserTest* test)
    {
        test->runFileChooser(request);
        return TRUE;
    }

    void runFileChooser(WebKitFileChooserRequest* request)
    {
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(request));
        m_fileChooserRequest = request;
        g_main_loop_quit(m_mainLoop);
    }

    WebKitFileChooserRequest* clickMouseButtonAndWaitForFileChooserRequest(int x, int y)
    {
        clickMouseButton(x, y);
        g_main_loop_run(m_mainLoop);
        return m_fileChooserRequest.get();
    }

private:
    GRefPtr<WebKitFileChooserRequest> m_fileChooserRequest;
};

static void testWebViewFileChooserRequest(FileChooserTest* test, gconstpointer)
{
    test->showInWindowAndWaitUntilMapped();
    static const char* fileChooserHTMLFormat = "<html><body><input style='position:absolute;left:0;top:0;margin:0;padding:0' type='file' %s/></body></html>";

    // Multiple selections not allowed, no MIME filtering.
    GOwnPtr<char> simpleFileUploadHTML(g_strdup_printf(fileChooserHTMLFormat, ""));
    test->loadHtml(simpleFileUploadHTML.get(), 0);
    test->waitUntilLoadFinished();
    WebKitFileChooserRequest* fileChooserRequest = test->clickMouseButtonAndWaitForFileChooserRequest(5, 5);
    g_assert(!webkit_file_chooser_request_get_select_multiple(fileChooserRequest));

    const gchar* const* mimeTypes = webkit_file_chooser_request_get_mime_types(fileChooserRequest);
    g_assert(!mimeTypes);
    GtkFileFilter* filter = webkit_file_chooser_request_get_mime_types_filter(fileChooserRequest);
    g_assert(!filter);
    const gchar* const* selectedFiles = webkit_file_chooser_request_get_selected_files(fileChooserRequest);
    g_assert(!selectedFiles);
    webkit_file_chooser_request_cancel(fileChooserRequest);

    // Multiple selections allowed, no MIME filtering, some pre-selected files.
    GOwnPtr<char> multipleSelectionFileUploadHTML(g_strdup_printf(fileChooserHTMLFormat, "multiple"));
    test->loadHtml(multipleSelectionFileUploadHTML.get(), 0);
    test->waitUntilLoadFinished();
    fileChooserRequest = test->clickMouseButtonAndWaitForFileChooserRequest(5, 5);
    g_assert(webkit_file_chooser_request_get_select_multiple(fileChooserRequest));

    mimeTypes = webkit_file_chooser_request_get_mime_types(fileChooserRequest);
    g_assert(!mimeTypes);
    filter = webkit_file_chooser_request_get_mime_types_filter(fileChooserRequest);
    g_assert(!filter);
    selectedFiles = webkit_file_chooser_request_get_selected_files(fileChooserRequest);
    g_assert(!selectedFiles);

    // Select some files.
    const gchar* filesToSelect[4] = { "/foo", "/foo/bar", "/foo/bar/baz", 0 };
    webkit_file_chooser_request_select_files(fileChooserRequest, filesToSelect);

    // Check the files that have been just selected.
    selectedFiles = webkit_file_chooser_request_get_selected_files(fileChooserRequest);
    g_assert(selectedFiles);
    g_assert_cmpstr(selectedFiles[0], ==, "/foo");
    g_assert_cmpstr(selectedFiles[1], ==, "/foo/bar");
    g_assert_cmpstr(selectedFiles[2], ==, "/foo/bar/baz");
    g_assert(!selectedFiles[3]);

    // Perform another request to check if the list of files selected
    // in the previous step appears now as part of the new request.
    fileChooserRequest = test->clickMouseButtonAndWaitForFileChooserRequest(5, 5);
    selectedFiles = webkit_file_chooser_request_get_selected_files(fileChooserRequest);
    g_assert(selectedFiles);
    g_assert_cmpstr(selectedFiles[0], ==, "/foo");
    g_assert_cmpstr(selectedFiles[1], ==, "/foo/bar");
    g_assert_cmpstr(selectedFiles[2], ==, "/foo/bar/baz");
    g_assert(!selectedFiles[3]);
    webkit_file_chooser_request_cancel(fileChooserRequest);

    // Multiple selections not allowed, only accept images, audio and video files..
    GOwnPtr<char> mimeFilteredFileUploadHTML(g_strdup_printf(fileChooserHTMLFormat, "accept='audio/*,video/*,image/*'"));
    test->loadHtml(mimeFilteredFileUploadHTML.get(), 0);
    test->waitUntilLoadFinished();
    fileChooserRequest = test->clickMouseButtonAndWaitForFileChooserRequest(5, 5);
    g_assert(!webkit_file_chooser_request_get_select_multiple(fileChooserRequest));

    mimeTypes = webkit_file_chooser_request_get_mime_types(fileChooserRequest);
    g_assert(mimeTypes);
    g_assert_cmpstr(mimeTypes[0], ==, "audio/*");
    g_assert_cmpstr(mimeTypes[1], ==, "video/*");
    g_assert_cmpstr(mimeTypes[2], ==, "image/*");
    g_assert(!mimeTypes[3]);

    filter = webkit_file_chooser_request_get_mime_types_filter(fileChooserRequest);
    g_assert(GTK_IS_FILE_FILTER(filter));
    g_assert(checkMimeTypeForFilter(filter, "audio/*"));
    g_assert(checkMimeTypeForFilter(filter, "video/*"));
    g_assert(checkMimeTypeForFilter(filter, "image/*"));

    selectedFiles = webkit_file_chooser_request_get_selected_files(fileChooserRequest);
    g_assert(!selectedFiles);
    webkit_file_chooser_request_cancel(fileChooserRequest);
}

class FullScreenClientTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(FullScreenClientTest);

    enum FullScreenEvent {
        None,
        Enter,
        Leave
    };

    static gboolean viewEnterFullScreenCallback(WebKitWebView*, FullScreenClientTest* test)
    {
        test->m_event = Enter;
        g_main_loop_quit(test->m_mainLoop);
        return FALSE;
    }

    static gboolean viewLeaveFullScreenCallback(WebKitWebView*, FullScreenClientTest* test)
    {
        test->m_event = Leave;
        g_main_loop_quit(test->m_mainLoop);
        return FALSE;
    }

    FullScreenClientTest()
        : m_event(None)
    {
        webkit_settings_set_enable_fullscreen(webkit_web_view_get_settings(m_webView), TRUE);
        g_signal_connect(m_webView, "enter-fullscreen", G_CALLBACK(viewEnterFullScreenCallback), this);
        g_signal_connect(m_webView, "leave-fullscreen", G_CALLBACK(viewLeaveFullScreenCallback), this);
    }

    ~FullScreenClientTest()
    {
        g_signal_handlers_disconnect_matched(m_webView, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
    }

    void requestFullScreenAndWaitUntilEnteredFullScreen()
    {
        m_event = None;
        webkit_web_view_run_javascript(m_webView, "document.documentElement.webkitRequestFullScreen();", 0, 0, 0);
        g_main_loop_run(m_mainLoop);
    }

    static gboolean leaveFullScreenIdle(FullScreenClientTest* test)
    {
        test->keyStroke(GDK_KEY_Escape);
        return FALSE;
    }

    void leaveFullScreenAndWaitUntilLeftFullScreen()
    {
        m_event = None;
        g_idle_add(reinterpret_cast<GSourceFunc>(leaveFullScreenIdle), this);
        g_main_loop_run(m_mainLoop);
    }

    FullScreenEvent m_event;
};

static void testWebViewFullScreen(FullScreenClientTest* test, gconstpointer)
{
    test->showInWindowAndWaitUntilMapped();
    test->loadHtml("<html><body>FullScreen test</body></html>", 0);
    test->waitUntilLoadFinished();
    test->requestFullScreenAndWaitUntilEnteredFullScreen();
    g_assert_cmpint(test->m_event, ==, FullScreenClientTest::Enter);
    test->leaveFullScreenAndWaitUntilLeftFullScreen();
    g_assert_cmpint(test->m_event, ==, FullScreenClientTest::Leave);
}

static void testWebViewCanShowMIMEType(WebViewTest* test, gconstpointer)
{
    // Supported MIME types.
    g_assert(webkit_web_view_can_show_mime_type(test->m_webView, "text/html"));
    g_assert(webkit_web_view_can_show_mime_type(test->m_webView, "text/plain"));
    g_assert(webkit_web_view_can_show_mime_type(test->m_webView, "image/jpeg"));

    // Unsupported MIME types.
    g_assert(!webkit_web_view_can_show_mime_type(test->m_webView, "text/vcard"));
    g_assert(!webkit_web_view_can_show_mime_type(test->m_webView, "application/pdf"));
    g_assert(!webkit_web_view_can_show_mime_type(test->m_webView, "application/zip"));
    g_assert(!webkit_web_view_can_show_mime_type(test->m_webView, "application/octet-stream"));

    // Plugins are only supported when enabled.
    webkit_web_context_set_additional_plugins_directory(webkit_web_view_get_context(test->m_webView), WEBKIT_TEST_PLUGIN_DIR);
    g_assert(webkit_web_view_can_show_mime_type(test->m_webView, "application/x-webkit-test-netscape"));
    webkit_settings_set_enable_plugins(webkit_web_view_get_settings(test->m_webView), FALSE);
    g_assert(!webkit_web_view_can_show_mime_type(test->m_webView, "application/x-webkit-test-netscape"));
}

class FormClientTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(FormClientTest);

    static void submitFormCallback(WebKitWebView*, WebKitFormSubmissionRequest* request, FormClientTest* test)
    {
        test->submitForm(request);
    }

    FormClientTest()
        : m_submitPositionX(0)
        , m_submitPositionY(0)
    {
        g_signal_connect(m_webView, "submit-form", G_CALLBACK(submitFormCallback), this);
    }

    ~FormClientTest()
    {
        g_signal_handlers_disconnect_matched(m_webView, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
    }

    void submitForm(WebKitFormSubmissionRequest* request)
    {
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(request));
        m_request = request;
        webkit_form_submission_request_submit(request);
        quitMainLoop();
    }

    GHashTable* waitUntilFormSubmittedAndGetTextFields()
    {
        g_main_loop_run(m_mainLoop);
        return webkit_form_submission_request_get_text_fields(m_request.get());
    }

    static gboolean doClickIdleCallback(FormClientTest* test)
    {
        test->clickMouseButton(test->m_submitPositionX, test->m_submitPositionY, 1);
        return FALSE;
    }

    void submitFormAtPosition(int x, int y)
    {
        m_submitPositionX = x;
        m_submitPositionY = y;
        g_idle_add(reinterpret_cast<GSourceFunc>(doClickIdleCallback), this);
    }

    int m_submitPositionX;
    int m_submitPositionY;
    GRefPtr<WebKitFormSubmissionRequest> m_request;
};

static void testWebViewSubmitForm(FormClientTest* test, gconstpointer)
{
    test->showInWindowAndWaitUntilMapped();

    const char* formHTML =
        "<html><body>"
        " <form action='#'>"
        "  <input type='text' name='text1' value='value1'>"
        "  <input type='text' name='text2' value='value2'>"
        "  <input type='password' name='password' value='secret'>"
        "  <textarea cols='5' rows='5' name='textarea'>Text</textarea>"
        "  <input type='hidden' name='hidden1' value='hidden1'>"
        "  <input type='submit' value='Submit' style='position:absolute; left:1; top:1' size='10'>"
        " </form>"
        "</body></html>";

    test->loadHtml(formHTML, "file:///");
    test->waitUntilLoadFinished();

    test->submitFormAtPosition(5, 5);
    GHashTable* values = test->waitUntilFormSubmittedAndGetTextFields();
    g_assert(values);
    g_assert_cmpuint(g_hash_table_size(values), ==, 3);
    g_assert_cmpstr(static_cast<char*>(g_hash_table_lookup(values, "text1")), ==, "value1");
    g_assert_cmpstr(static_cast<char*>(g_hash_table_lookup(values, "text2")), ==, "value2");
    g_assert_cmpstr(static_cast<char*>(g_hash_table_lookup(values, "password")), ==, "secret");
}

class SaveWebViewTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(SaveWebViewTest);

    SaveWebViewTest()
        : m_tempDirectory(g_dir_make_tmp("WebKit2SaveViewTest-XXXXXX", 0))
    {
    }

    ~SaveWebViewTest()
    {
        if (G_IS_FILE(m_file.get()))
            g_file_delete(m_file.get(), 0, 0);

        if (G_IS_INPUT_STREAM(m_inputStream.get()))
            g_input_stream_close(m_inputStream.get(), 0, 0);

        if (m_tempDirectory)
            g_rmdir(m_tempDirectory.get());
    }

    static void webViewSavedToStreamCallback(GObject* object, GAsyncResult* result, SaveWebViewTest* test)
    {
        GOwnPtr<GError> error;
        test->m_inputStream = adoptGRef(webkit_web_view_save_finish(test->m_webView, result, &error.outPtr()));
        g_assert(G_IS_INPUT_STREAM(test->m_inputStream.get()));
        g_assert(!error);

        test->quitMainLoop();
    }

    static void webViewSavedToFileCallback(GObject* object, GAsyncResult* result, SaveWebViewTest* test)
    {
        GOwnPtr<GError> error;
        g_assert(webkit_web_view_save_to_file_finish(test->m_webView, result, &error.outPtr()));
        g_assert(!error);

        test->quitMainLoop();
    }

    void saveAndWaitForStream()
    {
        webkit_web_view_save(m_webView, WEBKIT_SAVE_MODE_MHTML, 0, reinterpret_cast<GAsyncReadyCallback>(webViewSavedToStreamCallback), this);
        g_main_loop_run(m_mainLoop);
    }

    void saveAndWaitForFile()
    {
        m_saveDestinationFilePath.set(g_build_filename(m_tempDirectory.get(), "testWebViewSaveResult.mht", NULL));
        m_file = adoptGRef(g_file_new_for_path(m_saveDestinationFilePath.get()));
        webkit_web_view_save_to_file(m_webView, m_file.get(), WEBKIT_SAVE_MODE_MHTML, 0, reinterpret_cast<GAsyncReadyCallback>(webViewSavedToFileCallback), this);
        g_main_loop_run(m_mainLoop);
    }

    GOwnPtr<char> m_tempDirectory;
    GOwnPtr<char> m_saveDestinationFilePath;
    GRefPtr<GInputStream> m_inputStream;
    GRefPtr<GFile> m_file;
};

static void testWebViewSave(SaveWebViewTest* test, gconstpointer)
{
    test->loadHtml("<html>"
                   "<body>"
                   "  <p>A paragraph with plain text</p>"
                   "  <p>"
                   "    A red box: <img src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAIAAAD91JpzAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH3AYWDTMVwnSZnwAAAB1pVFh0Q29tbWVudAAAAAAAQ3JlYXRlZCB3aXRoIEdJTVBkLmUHAAAAFklEQVQI12P8z8DAwMDAxMDAwMDAAAANHQEDK+mmyAAAAABJRU5ErkJggg=='></br>"
                   "    A blue box: <img src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAIAAAD91JpzAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH3AYWDTMvBHhALQAAAB1pVFh0Q29tbWVudAAAAAAAQ3JlYXRlZCB3aXRoIEdJTVBkLmUHAAAAFklEQVQI12Nk4PnPwMDAxMDAwMDAAAALrwEPPIs1pgAAAABJRU5ErkJggg=='>"
                   "  </p>"
                   "</body>"
                   "</html>", 0);
    test->waitUntilLoadFinished();

    // Write to a file and to an input stream.
    test->saveAndWaitForFile();
    test->saveAndWaitForStream();

    // We should have exactly the same amount of bytes in the file
    // than those coming from the GInputStream. We don't compare the
    // strings read since the 'Date' field and the boundaries will be
    // different on each case. MHTML functionality will be tested by
    // Layout tests, so checking the amount of bytes is enough.
    GOwnPtr<GError> error;
    gchar buffer[512] = { 0 };
    gssize readBytes = 0;
    gssize totalBytesFromStream = 0;
    while (readBytes = g_input_stream_read(test->m_inputStream.get(), &buffer, 512, 0, &error.outPtr())) {
        g_assert(!error);
        totalBytesFromStream += readBytes;
    }

    // Check that the file exists and that it contains the same amount of bytes.
    GRefPtr<GFileInfo> fileInfo = adoptGRef(g_file_query_info(test->m_file.get(), G_FILE_ATTRIBUTE_STANDARD_SIZE, static_cast<GFileQueryInfoFlags>(0), 0, 0));
    g_assert_cmpint(g_file_info_get_size(fileInfo.get()), ==, totalBytesFromStream);
}

static void testWebViewMode(WebViewTest* test, gconstpointer)
{
    static const char* indexHTML = "<html><body><p>Test Web View Mode</p></body></html>";

    // Web mode.
    g_assert_cmpuint(webkit_web_view_get_view_mode(test->m_webView), ==, WEBKIT_VIEW_MODE_WEB);
    test->loadHtml(indexHTML, 0);
    test->waitUntilLoadFinished();
    WebKitJavascriptResult* javascriptResult = test->runJavaScriptAndWaitUntilFinished("window.document.body.textContent;", 0);
    GOwnPtr<char> valueString(WebViewTest::javascriptResultToCString(javascriptResult));
    g_assert_cmpstr(valueString.get(), ==, "Test Web View Mode");

    // Source mode.
    webkit_web_view_set_view_mode(test->m_webView, WEBKIT_VIEW_MODE_SOURCE);
    test->loadHtml(indexHTML, 0);
    test->waitUntilLoadFinished();
    javascriptResult = test->runJavaScriptAndWaitUntilFinished("window.document.body.textContent;", 0);
    valueString.set(WebViewTest::javascriptResultToCString(javascriptResult));
    g_assert_cmpstr(valueString.get(), ==, indexHTML);
}

// To test page visibility API. Currently only 'visible' and 'hidden' states are implemented fully in WebCore.
// See also http://www.w3.org/TR/2011/WD-page-visibility-20110602/ and https://developers.google.com/chrome/whitepapers/pagevisibility
static void testWebViewPageVisibility(WebViewTest* test, gconstpointer)
{
    test->loadHtml("<html><title></title>"
        "<body><p>Test Web Page Visibility</p>"
        "<script>"
        "document.addEventListener(\"webkitvisibilitychange\", onVisibilityChange, false);"
        "function onVisibilityChange() {"
        "    document.title = document.webkitVisibilityState;"
        "}"
        "</script>"
        "</body></html>",
        0);

    // Wait untill the page is loaded. Initial visibility should be 'hidden'.
    test->waitUntilLoadFinished();

    GOwnPtr<GError> error;
    WebKitJavascriptResult* javascriptResult = test->runJavaScriptAndWaitUntilFinished("document.webkitVisibilityState;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    GOwnPtr<char> valueString(WebViewTest::javascriptResultToCString(javascriptResult));
    g_assert_cmpstr(valueString.get(), ==, "hidden");

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("document.webkitHidden;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    g_assert(WebViewTest::javascriptResultToBoolean(javascriptResult));

    // Show the page. The visibility should be updated to 'visible'.
    test->showInWindow();
    test->waitUntilTitleChanged();

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("document.webkitVisibilityState;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    valueString.set(WebViewTest::javascriptResultToCString(javascriptResult));
    g_assert_cmpstr(valueString.get(), ==, "visible");

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("document.webkitHidden;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    g_assert(!WebViewTest::javascriptResultToBoolean(javascriptResult));

    // Hide the page. The visibility should be updated to 'hidden'.
    gtk_widget_hide(GTK_WIDGET(test->m_webView));
    test->waitUntilTitleChanged();

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("document.webkitVisibilityState;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    valueString.set(WebViewTest::javascriptResultToCString(javascriptResult));
    g_assert_cmpstr(valueString.get(), ==, "hidden");

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("document.webkitHidden;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    g_assert(WebViewTest::javascriptResultToBoolean(javascriptResult));
}

class SnapshotWebViewTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(SnapshotWebViewTest);

    static void onSnapshotCancelledReady(WebKitWebView* web_view, GAsyncResult* res, SnapshotWebViewTest* test)
    {
        GOwnPtr<GError> error;
        test->m_surface = webkit_web_view_get_snapshot_finish(web_view, res, &error.outPtr());
        g_assert(!test->m_surface);
        g_assert_error(error.get(), G_IO_ERROR, G_IO_ERROR_CANCELLED);
        test->quitMainLoop();
    }

    gboolean getSnapshotAndCancel()
    {
        if (m_surface)
            cairo_surface_destroy(m_surface);
        m_surface = 0;
        GRefPtr<GCancellable> cancellable = adoptGRef(g_cancellable_new());
        webkit_web_view_get_snapshot(m_webView, WEBKIT_SNAPSHOT_REGION_VISIBLE, WEBKIT_SNAPSHOT_OPTIONS_NONE, cancellable.get(), reinterpret_cast<GAsyncReadyCallback>(onSnapshotCancelledReady), this);
        g_cancellable_cancel(cancellable.get());
        g_main_loop_run(m_mainLoop);

        return true;
    }

};

static void testWebViewSnapshot(SnapshotWebViewTest* test, gconstpointer)
{
    test->loadHtml("<html><body><p>Whatever</p></body></html>", 0);
    test->waitUntilLoadFinished();

    // WebView not visible.
    cairo_surface_t* surface1 = test->getSnapshotAndWaitUntilReady(WEBKIT_SNAPSHOT_REGION_VISIBLE, WEBKIT_SNAPSHOT_OPTIONS_NONE);
    g_assert(!surface1);

    // Show surface, resize to 50x50, try again.
    test->showInWindowAndWaitUntilMapped();
    test->resizeView(50, 50);
    surface1 = test->getSnapshotAndWaitUntilReady(WEBKIT_SNAPSHOT_REGION_VISIBLE, WEBKIT_SNAPSHOT_OPTIONS_NONE);
    g_assert(surface1);

    // obtained surface should be at the most 50x50. Store the size
    // for comparison later.
    int width = cairo_image_surface_get_width(surface1);
    int height = cairo_image_surface_get_height(surface1);
    g_assert_cmpint(width, <=, 50);
    g_assert_cmpint(height, <=, 50);

    // Select all text in the WebView, request a snapshot ignoring selection.
    test->selectAll();
    surface1 = cairo_surface_reference(test->getSnapshotAndWaitUntilReady(WEBKIT_SNAPSHOT_REGION_VISIBLE, WEBKIT_SNAPSHOT_OPTIONS_NONE));
    g_assert(surface1);
    g_assert_cmpint(cairo_image_surface_get_width(surface1), ==, width);
    g_assert_cmpint(cairo_image_surface_get_height(surface1), ==, height);

    // Create identical surface.
    cairo_surface_t* surface2 = test->getSnapshotAndWaitUntilReady(WEBKIT_SNAPSHOT_REGION_VISIBLE, WEBKIT_SNAPSHOT_OPTIONS_NONE);
    g_assert(surface2);

    // Compare these two, they should be identical.
    g_assert(Test::cairoSurfacesEqual(surface1, surface2));

    // Request a new snapshot, including the selection this time. The
    // size should be the same but the result must be different to the
    // one previously obtained.
    surface2 = test->getSnapshotAndWaitUntilReady(WEBKIT_SNAPSHOT_REGION_VISIBLE, WEBKIT_SNAPSHOT_OPTIONS_INCLUDE_SELECTION_HIGHLIGHTING);
    g_assert(surface2);
    g_assert_cmpint(cairo_image_surface_get_width(surface2), ==, width);
    g_assert_cmpint(cairo_image_surface_get_height(surface2), ==, height);
    g_assert(!Test::cairoSurfacesEqual(surface1, surface2));

    // Request a snapshot of the whole document in the WebView. The
    // result should be different from the size obtained previously.
    surface2 = test->getSnapshotAndWaitUntilReady(WEBKIT_SNAPSHOT_REGION_FULL_DOCUMENT, WEBKIT_SNAPSHOT_OPTIONS_NONE);
    g_assert(surface2);
    g_assert_cmpint(cairo_image_surface_get_width(surface2),  >, width);
    g_assert_cmpint(cairo_image_surface_get_height(surface2), >, height);
    g_assert(!Test::cairoSurfacesEqual(surface1, surface2));

    cairo_surface_destroy(surface1);

    g_assert(test->getSnapshotAndCancel());
}

void beforeAll()
{
    WebViewTest::add("WebKitWebView", "default-context", testWebViewDefaultContext);
    WebViewTest::add("WebKitWebView", "custom-charset", testWebViewCustomCharset);
    WebViewTest::add("WebKitWebView", "settings", testWebViewSettings);
    UIClientTest::add("WebKitWebView", "create-ready-close", testWebViewCreateReadyClose);
    ModalDialogsTest::add("WebKitWebView", "allow-modal-dialogs", testWebViewAllowModalDialogs);
    ModalDialogsTest::add("WebKitWebView", "disallow-modal-dialogs", testWebViewDisallowModalDialogs);
    UIClientTest::add("WebKitWebView", "javascript-dialogs", testWebViewJavaScriptDialogs);
    UIClientTest::add("WebKitWebView", "window-properties", testWebViewWindowProperties);
    UIClientTest::add("WebKitWebView", "mouse-target", testWebViewMouseTarget);
    UIClientTest::add("WebKitWebView", "permission-requests", testWebViewPermissionRequests);
    WebViewTest::add("WebKitWebView", "zoom-level", testWebViewZoomLevel);
    WebViewTest::add("WebKitWebView", "run-javascript", testWebViewRunJavaScript);
    FileChooserTest::add("WebKitWebView", "file-chooser-request", testWebViewFileChooserRequest);
    FullScreenClientTest::add("WebKitWebView", "fullscreen", testWebViewFullScreen);
    WebViewTest::add("WebKitWebView", "can-show-mime-type", testWebViewCanShowMIMEType);
    FormClientTest::add("WebKitWebView", "submit-form", testWebViewSubmitForm);
    SaveWebViewTest::add("WebKitWebView", "save", testWebViewSave);
    WebViewTest::add("WebKitWebView", "view-mode", testWebViewMode);
    SnapshotWebViewTest::add("WebKitWebView", "snapshot", testWebViewSnapshot);
    WebViewTest::add("WebKitWebView", "page-visibility", testWebViewPageVisibility);
}

void afterAll()
{
}
