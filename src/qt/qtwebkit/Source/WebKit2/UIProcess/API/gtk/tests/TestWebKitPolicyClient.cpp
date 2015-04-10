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

#include "LoadTrackingTest.h"
#include "WebKitTestServer.h"
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

static WebKitTestServer* kServer;

class PolicyClientTest: public LoadTrackingTest {
public:
    MAKE_GLIB_TEST_FIXTURE(PolicyClientTest);

    enum PolicyDecisionResponse {
        Use,
        Ignore,
        Download,
        None
    };

    PolicyClientTest()
        : LoadTrackingTest()
        , m_policyDecisionResponse(None)
        , m_policyDecisionTypeFilter(0)
        , m_respondToPolicyDecisionAsynchronously(false)
        , m_haltMainLoopAfterMakingDecision(false)
    {
        g_signal_connect(m_webView, "decide-policy", G_CALLBACK(decidePolicyCallback), this);
    }

    static gboolean quitMainLoopLater(GMainLoop* loop)
    {
        g_main_loop_quit(loop);
        return FALSE;
    }

    static void respondToPolicyDecision(PolicyClientTest* test, WebKitPolicyDecision* decision)
    {
        switch (test->m_policyDecisionResponse) {
        case Use:
            webkit_policy_decision_use(decision);
            break;
        case Ignore:
            webkit_policy_decision_ignore(decision);
            break;
        case Download:
            webkit_policy_decision_download(decision);
            break;
        case None:
            break;
        }

        if (test->m_haltMainLoopAfterMakingDecision)
            g_idle_add(reinterpret_cast<GSourceFunc>(quitMainLoopLater), test->m_mainLoop);
    }

    static gboolean respondToPolicyDecisionLater(PolicyClientTest* test)
    {
        respondToPolicyDecision(test, test->m_previousPolicyDecision.get());
        test->m_previousPolicyDecision = 0;
        return FALSE;
    }

    static gboolean decidePolicyCallback(WebKitWebView* webView, WebKitPolicyDecision* decision, WebKitPolicyDecisionType type, PolicyClientTest* test)
    {
        if (test->m_policyDecisionTypeFilter != type)
            return FALSE;

        test->m_previousPolicyDecision = decision;
        if (test->m_respondToPolicyDecisionAsynchronously) {
            g_idle_add(reinterpret_cast<GSourceFunc>(respondToPolicyDecisionLater), test);
            return TRUE;
        }

        respondToPolicyDecision(test, decision);

        // We return FALSE here to ensure that the default policy decision
        // handler doesn't override whatever we use here.
        return FALSE;
    }

    PolicyDecisionResponse m_policyDecisionResponse;
    int m_policyDecisionTypeFilter;
    bool m_respondToPolicyDecisionAsynchronously;
    bool m_haltMainLoopAfterMakingDecision;
    GRefPtr<WebKitPolicyDecision> m_previousPolicyDecision;
};

static void testNavigationPolicy(PolicyClientTest* test, gconstpointer)
{
    test->m_policyDecisionTypeFilter = WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION;

    test->m_policyDecisionResponse = PolicyClientTest::Use;
    test->loadHtml("<html/>", "http://webkitgtk.org/");
    test->waitUntilLoadFinished();
    g_assert_cmpint(test->m_loadEvents.size(), ==, 3);

    // Ideally we'd like to have a more intensive test here, but it's still pretty tricky
    // to trigger different types of navigations with the GTK+ WebKit2 API.
    WebKitNavigationPolicyDecision* decision = WEBKIT_NAVIGATION_POLICY_DECISION(test->m_previousPolicyDecision.get());
    g_assert_cmpint(webkit_navigation_policy_decision_get_navigation_type(decision), ==, WEBKIT_NAVIGATION_TYPE_OTHER);
    g_assert_cmpint(webkit_navigation_policy_decision_get_mouse_button(decision), ==, 0);
    g_assert_cmpint(webkit_navigation_policy_decision_get_modifiers(decision), ==, 0);
    g_assert_cmpstr(webkit_navigation_policy_decision_get_frame_name(decision), ==, 0);
    WebKitURIRequest* request = webkit_navigation_policy_decision_get_request(decision);
    g_assert_cmpstr(webkit_uri_request_get_uri(request), ==, "http://webkitgtk.org/");

    test->m_policyDecisionResponse = PolicyClientTest::Use;
    test->m_respondToPolicyDecisionAsynchronously = true;
    test->loadHtml("<html/>", "http://webkitgtk.org/");
    test->waitUntilLoadFinished();
    g_assert_cmpint(test->m_loadEvents.size(), ==, 3);

    // If we are waiting until load completion, it will never complete if we ignore the
    // navigation. So we tell the main loop to quit sometime later.
    test->m_policyDecisionResponse = PolicyClientTest::Ignore;
    test->m_respondToPolicyDecisionAsynchronously = false;
    test->m_haltMainLoopAfterMakingDecision = true;
    test->loadHtml("<html/>", "http://webkitgtk.org/");
    test->waitUntilLoadFinished();
    g_assert_cmpint(test->m_loadEvents.size(), ==, 0);

    test->m_policyDecisionResponse = PolicyClientTest::Ignore;
    test->loadHtml("<html/>", "http://webkitgtk.org/");
    test->waitUntilLoadFinished();
    g_assert_cmpint(test->m_loadEvents.size(), ==, 0);
}

static void testResponsePolicy(PolicyClientTest* test, gconstpointer)
{
    test->m_policyDecisionTypeFilter = WEBKIT_POLICY_DECISION_TYPE_RESPONSE;

    test->m_policyDecisionResponse = PolicyClientTest::Use;
    test->loadURI(kServer->getURIForPath("/").data());
    test->waitUntilLoadFinished();
    g_assert_cmpint(test->m_loadEvents.size(), ==, 3);
    g_assert_cmpint(test->m_loadEvents[0], ==, LoadTrackingTest::ProvisionalLoadStarted);
    g_assert_cmpint(test->m_loadEvents[1], ==, LoadTrackingTest::LoadCommitted);
    g_assert_cmpint(test->m_loadEvents[2], ==, LoadTrackingTest::LoadFinished);

    test->m_respondToPolicyDecisionAsynchronously = true;
    test->loadURI(kServer->getURIForPath("/").data());
    test->waitUntilLoadFinished();
    g_assert_cmpint(test->m_loadEvents.size(), ==, 3);
    g_assert_cmpint(test->m_loadEvents[0], ==, LoadTrackingTest::ProvisionalLoadStarted);
    g_assert_cmpint(test->m_loadEvents[1], ==, LoadTrackingTest::LoadCommitted);
    g_assert_cmpint(test->m_loadEvents[2], ==, LoadTrackingTest::LoadFinished);

    test->m_respondToPolicyDecisionAsynchronously = false;
    test->m_policyDecisionResponse = PolicyClientTest::Ignore;
    test->loadURI(kServer->getURIForPath("/").data());
    test->waitUntilLoadFinished();

    g_assert_cmpint(test->m_loadEvents.size(), ==, 3);
    g_assert_cmpint(test->m_loadEvents[0], ==, LoadTrackingTest::ProvisionalLoadStarted);
    g_assert_cmpint(test->m_loadEvents[1], ==, LoadTrackingTest::ProvisionalLoadFailed);
    g_assert_cmpint(test->m_loadEvents[2], ==, LoadTrackingTest::LoadFinished);
}

struct CreateCallbackData {
    bool triedToOpenWindow;
    GMainLoop* mainLoop;
};

static WebKitWebView* createCallback(WebKitWebView* webView, CreateCallbackData* data)
{
    data->triedToOpenWindow = true;
    g_main_loop_quit(data->mainLoop);
    return 0;
}

static void testNewWindowPolicy(PolicyClientTest* test, gconstpointer)
{
    static const char* windowOpeningHTML =
        "<html><body>"
        "    <a id=\"link\" href=\"http://www.google.com\" target=\"_blank\">Link</a>"
        "    <script>"
        "        var event = document.createEvent('MouseEvents');"
        "        event.initEvent('click', true, false);"
        "        document.getElementById('link').dispatchEvent(event);"
        "    </script>"
        "</body></html>";
    test->m_policyDecisionTypeFilter = WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION;
    webkit_settings_set_javascript_can_open_windows_automatically(webkit_web_view_get_settings(test->m_webView), TRUE);

    CreateCallbackData data;
    data.triedToOpenWindow = false;
    data.mainLoop = test->m_mainLoop;

    g_signal_connect(test->m_webView, "create", G_CALLBACK(createCallback), &data);
    test->m_policyDecisionResponse = PolicyClientTest::Use;
    test->loadHtml(windowOpeningHTML, "http://webkitgtk.org/");
    test->wait(1);
    g_assert(data.triedToOpenWindow);

    WebKitNavigationPolicyDecision* decision = WEBKIT_NAVIGATION_POLICY_DECISION(test->m_previousPolicyDecision.get());
    g_assert_cmpstr(webkit_navigation_policy_decision_get_frame_name(decision), ==, "_blank");

    // Using a short timeout is a bit ugly here, but it's hard to get around because if we block
    // the new window signal we cannot halt the main loop in the create callback. If we
    // halt the main loop in the policy decision, the create callback never executes.
    data.triedToOpenWindow = false;
    test->m_policyDecisionResponse = PolicyClientTest::Ignore;
    test->loadHtml(windowOpeningHTML, "http://webkitgtk.org/");
    test->wait(.2);
    g_assert(!data.triedToOpenWindow);
}

static void serverCallback(SoupServer* server, SoupMessage* message, const char* path, GHashTable*, SoupClientContext*, gpointer)
{
    if (message->method != SOUP_METHOD_GET) {
        soup_message_set_status(message, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    if (g_str_equal(path, "/")) {
        static const char* responseString = "<html><body>Testing!</body></html>";
        soup_message_set_status(message, SOUP_STATUS_OK);
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, responseString, strlen(responseString));
        soup_message_body_complete(message->response_body);
    } else
        soup_message_set_status(message, SOUP_STATUS_NOT_FOUND);
}

void beforeAll()
{
    kServer = new WebKitTestServer();
    kServer->run(serverCallback);

    PolicyClientTest::add("WebKitPolicyClient", "navigation-policy", testNavigationPolicy);
    PolicyClientTest::add("WebKitPolicyClient", "response-policy", testResponsePolicy);
    PolicyClientTest::add("WebKitPolicyClient", "new-window-policy", testNewWindowPolicy);
}

void afterAll()
{
    delete kServer;
}
