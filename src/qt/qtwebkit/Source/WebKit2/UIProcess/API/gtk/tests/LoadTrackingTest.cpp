/*
 * Copyright (C) 2011 Igalia S.L.
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

#include <webkit2/webkit2.h>

static void loadChangedCallback(WebKitWebView* webView, WebKitLoadEvent loadEvent, LoadTrackingTest* test)
{
    switch (loadEvent) {
    case WEBKIT_LOAD_STARTED:
        g_assert(webkit_web_view_is_loading(webView));
        g_assert_cmpstr(test->m_activeURI.data(), ==, webkit_web_view_get_uri(webView));
        test->provisionalLoadStarted();
        break;
    case WEBKIT_LOAD_REDIRECTED:
        g_assert(webkit_web_view_is_loading(webView));
        test->m_activeURI = webkit_web_view_get_uri(webView);
        if (!test->m_redirectURI.isNull())
            g_assert_cmpstr(test->m_redirectURI.data(), ==, test->m_activeURI.data());
        test->provisionalLoadReceivedServerRedirect();
        break;
    case WEBKIT_LOAD_COMMITTED: {
        g_assert(webkit_web_view_is_loading(webView));
        g_assert_cmpstr(test->m_activeURI.data(), ==, webkit_web_view_get_uri(webView));

        // Check that on committed we always have a main resource with a response.
        WebKitWebResource* resource = webkit_web_view_get_main_resource(webView);
        g_assert(resource);
        g_assert(webkit_web_resource_get_response(resource));

        test->loadCommitted();
        break;
    }
    case WEBKIT_LOAD_FINISHED:
        g_assert(!webkit_web_view_is_loading(webView));
        if (!test->m_loadFailed)
            g_assert_cmpstr(test->m_activeURI.data(), ==, webkit_web_view_get_uri(webView));
        test->loadFinished();
        break;
    default:
        g_assert_not_reached();
    }
}

static void loadFailedCallback(WebKitWebView* webView, WebKitLoadEvent loadEvent, const char* failingURI, GError* error, LoadTrackingTest* test)
{
    test->m_loadFailed = true;
    test->m_error.set(g_error_copy(error));

    switch (loadEvent) {
    case WEBKIT_LOAD_STARTED:
        g_assert(!webkit_web_view_is_loading(webView));
        g_assert_cmpstr(test->m_activeURI.data(), ==, webkit_web_view_get_uri(webView));
        g_assert(error);
        test->provisionalLoadFailed(failingURI, error);
        break;
    case WEBKIT_LOAD_COMMITTED:
        g_assert(!webkit_web_view_is_loading(webView));
        g_assert_cmpstr(test->m_activeURI.data(), ==, webkit_web_view_get_uri(webView));
        g_assert(error);
        test->loadFailed(failingURI, error);
        break;
    default:
        g_assert_not_reached();
    }
}

static void estimatedProgressChangedCallback(GObject*, GParamSpec*, LoadTrackingTest* test)
{
    test->estimatedProgressChanged();
}

LoadTrackingTest::LoadTrackingTest()
    : m_runLoadUntilCompletion(false)
    , m_loadFailed(false)
{
    g_signal_connect(m_webView, "load-changed", G_CALLBACK(loadChangedCallback), this);
    g_signal_connect(m_webView, "load-failed", G_CALLBACK(loadFailedCallback), this);
    g_signal_connect(m_webView, "notify::estimated-load-progress", G_CALLBACK(estimatedProgressChangedCallback), this);

    g_assert(!webkit_web_view_get_uri(m_webView));
}

LoadTrackingTest::~LoadTrackingTest()
{
    g_signal_handlers_disconnect_matched(m_webView, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
}

void LoadTrackingTest::waitUntilLoadFinished()
{
    m_estimatedProgress = 0;
    m_runLoadUntilCompletion = true;
    g_main_loop_run(m_mainLoop);
}

void LoadTrackingTest::provisionalLoadStarted()
{
    m_loadEvents.append(ProvisionalLoadStarted);
}

void LoadTrackingTest::provisionalLoadReceivedServerRedirect()
{
    m_loadEvents.append(ProvisionalLoadReceivedServerRedirect);
}

void LoadTrackingTest::provisionalLoadFailed(const gchar* failingURI, GError* error)
{
    m_loadEvents.append(ProvisionalLoadFailed);
}

void LoadTrackingTest::loadCommitted()
{
    m_loadEvents.append(LoadCommitted);
}

void LoadTrackingTest::loadFinished()
{
    m_loadEvents.append(LoadFinished);
    if (m_runLoadUntilCompletion)
        g_main_loop_quit(m_mainLoop);
}

void LoadTrackingTest::loadFailed(const gchar* failingURI, GError* error)
{
    m_loadEvents.append(LoadFailed);
}

void LoadTrackingTest::estimatedProgressChanged()
{
    double progress = webkit_web_view_get_estimated_load_progress(m_webView);
    g_assert_cmpfloat(m_estimatedProgress, <, progress);
    m_estimatedProgress = progress;
}

void LoadTrackingTest::loadURI(const char* uri)
{
    m_loadEvents.clear();
    m_estimatedProgress = 0;
    m_error.clear();
    WebViewTest::loadURI(uri);
}

void LoadTrackingTest::loadHtml(const char* html, const char* baseURI)
{
    m_loadEvents.clear();
    m_estimatedProgress = 0;
    m_error.clear();
    WebViewTest::loadHtml(html, baseURI);
}

void LoadTrackingTest::loadPlainText(const char* plainText)
{
    m_loadEvents.clear();
    m_estimatedProgress = 0;
    m_error.clear();
    WebViewTest::loadPlainText(plainText);
}

void LoadTrackingTest::loadRequest(WebKitURIRequest* request)
{
    m_loadEvents.clear();
    m_estimatedProgress = 0;
    m_error.clear();
    WebViewTest::loadRequest(request);
}

void LoadTrackingTest::reload()
{
    m_loadEvents.clear();
    m_estimatedProgress = 0;
    m_error.clear();
    webkit_web_view_reload(m_webView);
}

void LoadTrackingTest::goBack()
{
    m_loadEvents.clear();
    m_estimatedProgress = 0;
    m_error.clear();
    WebViewTest::goBack();
}

void LoadTrackingTest::goForward()
{
    m_loadEvents.clear();
    m_estimatedProgress = 0;
    m_error.clear();
    WebViewTest::goForward();
}
