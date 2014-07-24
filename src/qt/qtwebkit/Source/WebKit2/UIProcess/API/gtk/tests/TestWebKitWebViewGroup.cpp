/*
 * Copyright (C) 2013 Igalia S.L.
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

#include "WebKitTestServer.h"
#include "WebViewTest.h"
#include <cstdarg>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <wtf/gobject/GRefPtr.h>

static WebKitTestServer* kServer;

// These are all here so that they can be changed easily, if necessary.
static const char* kStyleSheetHTML = "<html><div id=\"styledElement\">Sweet stylez!</div></html>";
static const char* kInjectedStyleSheet = "#styledElement { font-weight: bold; }";
static const char* kStyleSheetTestScript = "getComputedStyle(document.getElementById('styledElement'))['font-weight']";
static const char* kStyleSheetTestScriptResult = "bold";

static void testWebViewGroupDefault(Test* test, gconstpointer)
{
    // Default group is shared by all WebViews by default.
    GRefPtr<WebKitWebView> webView1 = WEBKIT_WEB_VIEW(webkit_web_view_new());
    GRefPtr<WebKitWebView> webView2 = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_assert(webkit_web_view_get_group(webView1.get()) == webkit_web_view_get_group(webView2.get()));

    // Settings are shared by all web view in the same group.
    g_assert(webkit_web_view_get_settings(webView1.get()) == webkit_web_view_get_settings(webView2.get()));
    g_assert(webkit_web_view_get_settings(webView1.get()) == webkit_web_view_group_get_settings(webkit_web_view_get_group(webView2.get())));
}

static void testWebViewGroupNewGroup(Test* test, gconstpointer)
{
    // Passing 0 as group name generates the name automatically.
    GRefPtr<WebKitWebViewGroup> viewGroup1 = adoptGRef(webkit_web_view_group_new(0));
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(viewGroup1.get()));
    g_assert(webkit_web_view_group_get_name(viewGroup1.get()));

    // New group with a given name.
    GRefPtr<WebKitWebViewGroup> viewGroup2 = adoptGRef(webkit_web_view_group_new("TestGroup2"));
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(viewGroup2.get()));
    g_assert_cmpstr(webkit_web_view_group_get_name(viewGroup2.get()), ==, "TestGroup2");
    g_assert_cmpstr(webkit_web_view_group_get_name(viewGroup2.get()), !=, webkit_web_view_group_get_name(viewGroup1.get()));

    // Every group has its own settings.
    g_assert(webkit_web_view_group_get_settings(viewGroup1.get()) != webkit_web_view_group_get_settings(viewGroup2.get()));
}

static void testWebViewNewWithGroup(Test* test, gconstpointer)
{
    GRefPtr<WebKitWebViewGroup> viewGroup1 = adoptGRef(webkit_web_view_group_new("TestGroup1"));
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(viewGroup1.get()));
    GRefPtr<WebKitWebView> webView1 = WEBKIT_WEB_VIEW(webkit_web_view_new_with_group(viewGroup1.get()));
    g_assert(webkit_web_view_get_group(webView1.get()) == viewGroup1.get());

    GRefPtr<WebKitWebView> webView2 = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_assert(webkit_web_view_get_group(webView2.get()) != viewGroup1.get());

    // Settings should be different for views in different groups.
    g_assert(webkit_web_view_get_settings(webView1.get()) != webkit_web_view_get_settings(webView2.get()));
}

static void testWebViewGroupSettings(Test* test, gconstpointer)
{
    GRefPtr<WebKitWebViewGroup> viewGroup1 = adoptGRef(webkit_web_view_group_new("TestGroup1"));
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(viewGroup1.get()));
    GRefPtr<WebKitSettings> newSettings = adoptGRef(webkit_settings_new_with_settings("enable-javascript", FALSE, NULL));
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(newSettings.get()));
    webkit_web_view_group_set_settings(viewGroup1.get(), newSettings.get());
    g_assert(webkit_web_view_group_get_settings(viewGroup1.get()) == newSettings.get());

    GRefPtr<WebKitWebView> webView1 = WEBKIT_WEB_VIEW(webkit_web_view_new_with_group(viewGroup1.get()));
    GRefPtr<WebKitWebView> webView2 = WEBKIT_WEB_VIEW(webkit_web_view_new());
    WebKitSettings* webView1Settings = webkit_web_view_get_settings(webView1.get());
    WebKitSettings* webView2Settings = webkit_web_view_get_settings(webView2.get());
    g_assert(webView1Settings != webView2Settings);
    g_assert(webkit_settings_get_enable_javascript(webView1Settings) != webkit_settings_get_enable_javascript(webView2Settings));

    webkit_web_view_set_settings(webView1.get(), webView2Settings);
    g_assert(webkit_web_view_get_settings(webView1.get()) == webView2Settings);
    g_assert(webkit_web_view_group_get_settings(webkit_web_view_get_group(webView1.get())) == webView2Settings);
}

static bool isStyleSheetInjectedForURLAtPath(WebViewTest* test, const char* path)
{
    test->loadURI(kServer->getURIForPath(path).data());
    test->waitUntilLoadFinished();

    GOwnPtr<GError> error;
    WebKitJavascriptResult* javascriptResult = test->runJavaScriptAndWaitUntilFinished(kStyleSheetTestScript, &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());

    GOwnPtr<char> resultString(WebViewTest::javascriptResultToCString(javascriptResult));
    return !g_strcmp0(resultString.get(), kStyleSheetTestScriptResult);
}

static void fillURLListFromPaths(char** list, const char* path, ...)
{
    va_list argumentList;
    va_start(argumentList, path);

    int i = 0;
    while (path) {
        // FIXME: We must use a wildcard for the host here until http://wkbug.com/112476 is fixed.
        // Until that time patterns with port numbers in them will not properly match URLs with port numbers.
        list[i++] = g_strdup_printf("http://*/%s*", path);
        path = va_arg(argumentList, const char*);
    }
}

static void removeOldInjectedStyleSheetsAndResetLists(WebKitWebViewGroup* group, char** whitelist, char** blacklist)
{
    webkit_web_view_group_remove_all_user_style_sheets(group);

    while (*whitelist) {
        g_free(*whitelist);
        *whitelist = 0;
        whitelist++;
    }

    while (*blacklist) {
        g_free(*blacklist);
        *blacklist = 0;
        blacklist++;
    }
}

static void testWebViewGroupInjectedStyleSheet(WebViewTest* test, gconstpointer)
{
    WebKitWebViewGroup* group = webkit_web_view_get_group(test->m_webView);
    char* whitelist[3] = { 0, 0, 0 };
    char* blacklist[3] = { 0, 0, 0 };

    removeOldInjectedStyleSheetsAndResetLists(group, whitelist, blacklist);

    // Without a whitelist or a blacklist all URLs should have the injected style sheet.
    static const char* randomPath = "somerandompath";
    g_assert(!isStyleSheetInjectedForURLAtPath(test, randomPath));
    webkit_web_view_group_add_user_style_sheet(group, kInjectedStyleSheet, 0, 0, 0, WEBKIT_INJECTED_CONTENT_FRAMES_ALL);
    g_assert(isStyleSheetInjectedForURLAtPath(test, randomPath));

    removeOldInjectedStyleSheetsAndResetLists(group, whitelist, blacklist);

    fillURLListFromPaths(blacklist, randomPath, 0);
    webkit_web_view_group_add_user_style_sheet(group, kInjectedStyleSheet, 0, 0, blacklist, WEBKIT_INJECTED_CONTENT_FRAMES_ALL);
    g_assert(!isStyleSheetInjectedForURLAtPath(test, randomPath));
    g_assert(isStyleSheetInjectedForURLAtPath(test, "someotherrandompath"));

    removeOldInjectedStyleSheetsAndResetLists(group, whitelist, blacklist);

    static const char* inTheWhiteList = "inthewhitelist";
    static const char* notInWhitelist = "notinthewhitelist";
    static const char* inTheWhiteListAndBlackList = "inthewhitelistandblacklist";

    fillURLListFromPaths(whitelist, inTheWhiteList, inTheWhiteListAndBlackList, 0);
    fillURLListFromPaths(blacklist, inTheWhiteListAndBlackList, 0);
    webkit_web_view_group_add_user_style_sheet(group, kInjectedStyleSheet, 0, whitelist, blacklist, WEBKIT_INJECTED_CONTENT_FRAMES_ALL);
    g_assert(isStyleSheetInjectedForURLAtPath(test, inTheWhiteList));
    g_assert(!isStyleSheetInjectedForURLAtPath(test, inTheWhiteListAndBlackList));
    g_assert(!isStyleSheetInjectedForURLAtPath(test, notInWhitelist));

    // It's important to clean up the environment before other tests.
    removeOldInjectedStyleSheetsAndResetLists(group, whitelist, blacklist);
}

static void serverCallback(SoupServer* server, SoupMessage* message, const char* path, GHashTable*, SoupClientContext*, gpointer)
{
    soup_message_set_status(message, SOUP_STATUS_OK);
    soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, kStyleSheetHTML, strlen(kStyleSheetHTML));
    soup_message_body_complete(message->response_body);
}

void beforeAll()
{
    kServer = new WebKitTestServer();
    kServer->run(serverCallback);

    Test::add("WebKitWebViewGroup", "default-group", testWebViewGroupDefault);
    Test::add("WebKitWebViewGroup", "new-group", testWebViewGroupNewGroup);
    Test::add("WebKitWebView", "new-with-group", testWebViewNewWithGroup);
    Test::add("WebKitWebViewGroup", "settings", testWebViewGroupSettings);
    WebViewTest::add("WebKitWebViewGroup", "injected-style-sheet", testWebViewGroupInjectedStyleSheet);
}

void afterAll()
{
    delete kServer;
}
