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

#include "WebKitTestServer.h"
#include "WebViewTest.h"
#include <wtf/gobject/GRefPtr.h>

static WebKitTestServer* kServer;

static const char* kIndexHtml =
    "<html><head>"
    " <link rel='stylesheet' href='/style.css' type='text/css'>"
    " <script language='javascript' src='/javascript.js'></script>"
    "</head><body>WebKitGTK+ resources test</body></html>";

static const char* kStyleCSS =
    "body {"
    "    margin: 0px;"
    "    padding: 0px;"
    "    font-family: sans-serif;"
    "    background: url(/blank.ico) 0 0 no-repeat;"
    "    color: black;"
    "}";

static const char* kJavascript = "function foo () { var a = 1; }";

class ResourcesTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(ResourcesTest);

    static void resourceSentRequestCallback(WebKitWebResource* resource, WebKitURIRequest* request, WebKitURIResponse* redirectResponse, ResourcesTest* test)
    {
        test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(request));
        if (redirectResponse)
            test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(redirectResponse));
        test->resourceSentRequest(resource, request, redirectResponse);
    }

    static void resourceReceivedResponseCallback(WebKitWebResource* resource, GParamSpec*, ResourcesTest* test)
    {
        g_assert(webkit_web_resource_get_response(resource));
        test->resourceReceivedResponse(resource);
    }

    static void resourceReceivedDataCallback(WebKitWebResource* resource, guint64 bytesReceived, ResourcesTest* test)
    {
        test->resourceReceivedData(resource, bytesReceived);
    }

    static void resourceFinishedCallback(WebKitWebResource* resource, ResourcesTest* test)
    {
        test->resourceFinished(resource);
    }

    static void resourceFailedCallback(WebKitWebResource* resource, GError* error, ResourcesTest* test)
    {
        g_assert(error);
        test->resourceFailed(resource, error);
    }

    static void resourceLoadStartedCallback(WebKitWebView* webView, WebKitWebResource* resource, WebKitURIRequest* request, ResourcesTest* test)
    {
        test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(resource));
        test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(request));

        // Ignore favicons.
        if (g_str_has_suffix(webkit_uri_request_get_uri(request), "favicon.ico"))
            return;

        test->resourceLoadStarted(resource, request);
        g_signal_connect(resource, "sent-request", G_CALLBACK(resourceSentRequestCallback), test);
        g_signal_connect(resource, "notify::response", G_CALLBACK(resourceReceivedResponseCallback), test);
        g_signal_connect(resource, "received-data", G_CALLBACK(resourceReceivedDataCallback), test);
        g_signal_connect(resource, "finished", G_CALLBACK(resourceFinishedCallback), test);
        g_signal_connect(resource, "failed", G_CALLBACK(resourceFailedCallback), test);
    }

    void clearSubresources()
    {
        g_list_free_full(m_subresources, reinterpret_cast<GDestroyNotify>(g_object_unref));
        m_subresources = 0;
    }

    ResourcesTest()
        : WebViewTest()
        , m_resourcesLoaded(0)
        , m_resourcesToLoad(0)
        , m_resourceDataSize(0)
        , m_subresources(0)
    {
        g_signal_connect(m_webView, "resource-load-started", G_CALLBACK(resourceLoadStartedCallback), this);
    }

    ~ResourcesTest()
    {
        clearSubresources();
    }

    virtual void resourceLoadStarted(WebKitWebResource* resource, WebKitURIRequest* request)
    {
    }

    virtual void resourceSentRequest(WebKitWebResource* resource, WebKitURIRequest* request, WebKitURIResponse* redirectResponse)
    {
    }

    virtual void resourceReceivedResponse(WebKitWebResource* resource)
    {
    }

    virtual void resourceReceivedData(WebKitWebResource* resource, guint64 bytesReceived)
    {
    }

    virtual void resourceFinished(WebKitWebResource* resource)
    {
        g_signal_handlers_disconnect_matched(resource, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
        if (webkit_web_view_get_main_resource(m_webView) != resource)
            m_subresources = g_list_prepend(m_subresources, g_object_ref(resource));
        if (++m_resourcesLoaded == m_resourcesToLoad)
            g_main_loop_quit(m_mainLoop);
    }

    virtual void resourceFailed(WebKitWebResource* resource, GError* error)
    {
        g_assert_not_reached();
    }

    void waitUntilResourcesLoaded(size_t resourcesCount)
    {
        m_resourcesLoaded = 0;
        m_resourcesToLoad = resourcesCount;
        clearSubresources();
        g_main_loop_run(m_mainLoop);
    }

    GList* subresources()
    {
        return m_subresources;
    }

    static void resourceGetDataCallback(GObject* object, GAsyncResult* result, gpointer userData)
    {
        size_t dataSize;
        GOwnPtr<GError> error;
        unsigned char* data = webkit_web_resource_get_data_finish(WEBKIT_WEB_RESOURCE(object), result, &dataSize, &error.outPtr());
        g_assert(!error.get());
        g_assert(data);
        g_assert_cmpint(dataSize, >, 0);

        ResourcesTest* test = static_cast<ResourcesTest*>(userData);
        test->m_resourceData.set(reinterpret_cast<char*>(data));
        test->m_resourceDataSize = dataSize;
        g_main_loop_quit(test->m_mainLoop);
    }

    void checkResourceData(WebKitWebResource* resource)
    {
        m_resourceDataSize = 0;
        webkit_web_resource_get_data(resource, 0, resourceGetDataCallback, this);
        g_main_loop_run(m_mainLoop);

        const char* uri = webkit_web_resource_get_uri(resource);
        if (uri == kServer->getURIForPath("/")) {
            g_assert_cmpint(m_resourceDataSize, ==, strlen(kIndexHtml));
            g_assert(!strncmp(m_resourceData.get(), kIndexHtml, m_resourceDataSize));
        } else if (uri == kServer->getURIForPath("/style.css")) {
            g_assert_cmpint(m_resourceDataSize, ==, strlen(kStyleCSS));
            g_assert(!strncmp(m_resourceData.get(), kStyleCSS, m_resourceDataSize));
        } else if (uri == kServer->getURIForPath("/javascript.js")) {
            g_assert_cmpint(m_resourceDataSize, ==, strlen(kJavascript));
            g_assert(!strncmp(m_resourceData.get(), kJavascript, m_resourceDataSize));
        } else
            g_assert_not_reached();
        m_resourceData.clear();
    }

    size_t m_resourcesLoaded;
    size_t m_resourcesToLoad;
    GOwnPtr<char> m_resourceData;
    size_t m_resourceDataSize;
    GList* m_subresources;
};

static void testWebViewResources(ResourcesTest* test, gconstpointer)
{
    // Nothing loaded yet, there shoulnd't be resources.
    g_assert(!webkit_web_view_get_main_resource(test->m_webView));
    g_assert(!test->subresources());

    // Load simple page without subresources.
    test->loadHtml("<html><body>Testing WebKitGTK+</body></html>", 0);
    test->waitUntilLoadFinished();
    WebKitWebResource* resource = webkit_web_view_get_main_resource(test->m_webView);
    g_assert(resource);
    g_assert_cmpstr(webkit_web_view_get_uri(test->m_webView), ==, webkit_web_resource_get_uri(resource));
    g_assert(!test->subresources());

    // Load simple page with subresources.
    test->loadURI(kServer->getURIForPath("/").data());
    test->waitUntilResourcesLoaded(4);

    resource = webkit_web_view_get_main_resource(test->m_webView);
    g_assert(resource);
    g_assert_cmpstr(webkit_web_view_get_uri(test->m_webView), ==, webkit_web_resource_get_uri(resource));
    GList* subresources = test->subresources();
    g_assert(subresources);
    g_assert_cmpint(g_list_length(subresources), ==, 3);

#if 0
    // Load the same URI again.
    // FIXME: we need a workaround for bug https://bugs.webkit.org/show_bug.cgi?id=78510.
    test->loadURI(kServer->getURIForPath("/").data());
    test->waitUntilResourcesLoaded(4);
#endif

    // Reload.
    webkit_web_view_reload_bypass_cache(test->m_webView);
    test->waitUntilResourcesLoaded(4);
}

class SingleResourceLoadTest: public ResourcesTest {
public:
    MAKE_GLIB_TEST_FIXTURE(SingleResourceLoadTest);

    enum LoadEvents {
        Started,
        SentRequest,
        Redirected,
        ReceivedResponse,
        ReceivedData,
        Finished,
        Failed
    };

    SingleResourceLoadTest()
        : ResourcesTest()
        , m_resourceDataReceived(0)
    {
         m_resourcesToLoad = 2;
    }

    void resourceLoadStarted(WebKitWebResource* resource, WebKitURIRequest* request)
    {
        if (resource == webkit_web_view_get_main_resource(m_webView))
            return;

        m_resourceDataReceived = 0;
        m_resource = resource;
        m_loadEvents.append(Started);
    }

    void resourceSentRequest(WebKitWebResource* resource, WebKitURIRequest* request, WebKitURIResponse* redirectResponse)
    {
        if (resource != m_resource)
            return;

        if (redirectResponse)
            m_loadEvents.append(Redirected);
        else
            m_loadEvents.append(SentRequest);
    }

    void resourceReceivedResponse(WebKitWebResource* resource)
    {
        if (resource != m_resource)
            return;

        m_loadEvents.append(ReceivedResponse);
    }

    void resourceReceivedData(WebKitWebResource* resource, guint64 bytesReceived)
    {
        if (resource != m_resource)
            return;

        m_resourceDataReceived += bytesReceived;
        if (!m_loadEvents.contains(ReceivedData))
            m_loadEvents.append(ReceivedData);
    }

    void resourceFinished(WebKitWebResource* resource)
    {
        if (resource != m_resource) {
            ResourcesTest::resourceFinished(resource);
            return;
        }

        if (!m_loadEvents.contains(Failed)) {
            WebKitURIResponse* response = webkit_web_resource_get_response(m_resource.get());
            g_assert(response);
            g_assert_cmpint(webkit_uri_response_get_content_length(response), ==, m_resourceDataReceived);
        }
        m_loadEvents.append(Finished);
        ResourcesTest::resourceFinished(resource);
    }

    void resourceFailed(WebKitWebResource* resource, GError* error)
    {
        if (resource == m_resource)
            m_loadEvents.append(Failed);
    }

    void waitUntilResourceLoadFinished()
    {
        m_resource = 0;
        m_resourcesLoaded = 0;
        g_main_loop_run(m_mainLoop);
    }

    WebKitURIResponse* waitUntilResourceLoadFinishedAndReturnURIResponse()
    {
        waitUntilResourceLoadFinished();
        g_assert(m_resource);
        return webkit_web_resource_get_response(m_resource.get());
    }

    GRefPtr<WebKitWebResource> m_resource;
    Vector<LoadEvents> m_loadEvents;
    guint64 m_resourceDataReceived;
};

static void testWebResourceLoading(SingleResourceLoadTest* test, gconstpointer)
{
    test->loadURI(kServer->getURIForPath("/javascript.html").data());
    test->waitUntilResourceLoadFinished();
    g_assert(test->m_resource);
    Vector<SingleResourceLoadTest::LoadEvents>& events = test->m_loadEvents;
    g_assert_cmpint(events.size(), ==, 5);
    g_assert_cmpint(events[0], ==, SingleResourceLoadTest::Started);
    g_assert_cmpint(events[1], ==, SingleResourceLoadTest::SentRequest);
    g_assert_cmpint(events[2], ==, SingleResourceLoadTest::ReceivedResponse);
    g_assert_cmpint(events[3], ==, SingleResourceLoadTest::ReceivedData);
    g_assert_cmpint(events[4], ==, SingleResourceLoadTest::Finished);
    events.clear();

    test->loadURI(kServer->getURIForPath("/redirected-css.html").data());
    test->waitUntilResourceLoadFinished();
    g_assert(test->m_resource);
    g_assert_cmpint(events.size(), ==, 6);
    g_assert_cmpint(events[0], ==, SingleResourceLoadTest::Started);
    g_assert_cmpint(events[1], ==, SingleResourceLoadTest::SentRequest);
    g_assert_cmpint(events[2], ==, SingleResourceLoadTest::Redirected);
    g_assert_cmpint(events[3], ==, SingleResourceLoadTest::ReceivedResponse);
    g_assert_cmpint(events[4], ==, SingleResourceLoadTest::ReceivedData);
    g_assert_cmpint(events[5], ==, SingleResourceLoadTest::Finished);
    events.clear();

    test->loadURI(kServer->getURIForPath("/invalid-css.html").data());
    test->waitUntilResourceLoadFinished();
    g_assert(test->m_resource);
    g_assert_cmpint(events.size(), ==, 4);
    g_assert_cmpint(events[0], ==, SingleResourceLoadTest::Started);
    g_assert_cmpint(events[1], ==, SingleResourceLoadTest::SentRequest);
    g_assert_cmpint(events[2], ==, SingleResourceLoadTest::Failed);
    g_assert_cmpint(events[3], ==, SingleResourceLoadTest::Finished);
    events.clear();
}

static void testWebResourceResponse(SingleResourceLoadTest* test, gconstpointer)
{
    // No cached resource: First load.
    test->loadURI(kServer->getURIForPath("/javascript.html").data());
    WebKitURIResponse* response = test->waitUntilResourceLoadFinishedAndReturnURIResponse();
    g_assert_cmpint(webkit_uri_response_get_status_code(response), ==, SOUP_STATUS_OK);

    // No cached resource: Second load.
    test->loadURI(kServer->getURIForPath("/javascript.html").data());
    response = test->waitUntilResourceLoadFinishedAndReturnURIResponse();
    g_assert_cmpint(webkit_uri_response_get_status_code(response), ==, SOUP_STATUS_OK);

    // No cached resource: Reload.
    webkit_web_view_reload(test->m_webView);
    response = test->waitUntilResourceLoadFinishedAndReturnURIResponse();
    g_assert_cmpint(webkit_uri_response_get_status_code(response), ==, SOUP_STATUS_OK);

    // Cached resource: First load.
    test->loadURI(kServer->getURIForPath("/image.html").data());
    response = test->waitUntilResourceLoadFinishedAndReturnURIResponse();
    g_assert_cmpint(webkit_uri_response_get_status_code(response), ==, SOUP_STATUS_OK);

    // Cached resource: Second load.
    test->loadURI(kServer->getURIForPath("/image.html").data());
    response = test->waitUntilResourceLoadFinishedAndReturnURIResponse();
    g_assert_cmpint(webkit_uri_response_get_status_code(response), ==, SOUP_STATUS_OK);

    // Cached resource: Reload.
    webkit_web_view_reload(test->m_webView);
    response = test->waitUntilResourceLoadFinishedAndReturnURIResponse();
    g_assert_cmpint(webkit_uri_response_get_status_code(response), ==, SOUP_STATUS_NOT_MODIFIED);
}

static void testWebResourceMimeType(SingleResourceLoadTest* test, gconstpointer)
{
    test->loadURI(kServer->getURIForPath("/javascript.html").data());
    WebKitURIResponse* response = test->waitUntilResourceLoadFinishedAndReturnURIResponse();
    g_assert_cmpstr(webkit_uri_response_get_mime_type(response), ==, "text/javascript");

    test->loadURI(kServer->getURIForPath("/image.html").data());
    response = test->waitUntilResourceLoadFinishedAndReturnURIResponse();
    g_assert_cmpstr(webkit_uri_response_get_mime_type(response), ==, "image/vnd.microsoft.icon");

    test->loadURI(kServer->getURIForPath("/redirected-css.html").data());
    response = test->waitUntilResourceLoadFinishedAndReturnURIResponse();
    g_assert_cmpstr(webkit_uri_response_get_mime_type(response), ==, "text/css");
}

static void testWebResourceSuggestedFilename(SingleResourceLoadTest* test, gconstpointer)
{
    test->loadURI(kServer->getURIForPath("/javascript.html").data());
    WebKitURIResponse* response = test->waitUntilResourceLoadFinishedAndReturnURIResponse();
    g_assert_cmpstr(webkit_uri_response_get_suggested_filename(response), ==, "JavaScript.js");

    test->loadURI(kServer->getURIForPath("/image.html").data());
    response = test->waitUntilResourceLoadFinishedAndReturnURIResponse();
    g_assert(!webkit_uri_response_get_suggested_filename(response));
}

class ResourceURITrackingTest: public SingleResourceLoadTest {
public:
    MAKE_GLIB_TEST_FIXTURE(ResourceURITrackingTest);

    ResourceURITrackingTest()
        : SingleResourceLoadTest()
    {
    }

    static void uriChanged(WebKitWebResource* resource, GParamSpec*, ResourceURITrackingTest* test)
    {
        g_assert(resource == test->m_resource.get());
        g_assert_cmpstr(test->m_activeURI.data(), !=, webkit_web_resource_get_uri(test->m_resource.get()));
        test->m_activeURI = webkit_web_resource_get_uri(test->m_resource.get());
    }

    void resourceLoadStarted(WebKitWebResource* resource, WebKitURIRequest* request)
    {
        if (resource == webkit_web_view_get_main_resource(m_webView))
            return;

        m_resource = resource;
        m_activeURI = webkit_web_resource_get_uri(resource);
        checkActiveURI("/redirected.css");
        g_assert_cmpstr(m_activeURI.data(), ==, webkit_uri_request_get_uri(request));
        g_signal_connect(resource, "notify::uri", G_CALLBACK(uriChanged), this);
    }

    void resourceSentRequest(WebKitWebResource* resource, WebKitURIRequest* request, WebKitURIResponse* redirectResponse)
    {
        if (resource != m_resource)
            return;

        if (redirectResponse)
            checkActiveURI("/simple-style.css");
        else
            checkActiveURI("/redirected.css");
        g_assert_cmpstr(m_activeURI.data(), ==, webkit_uri_request_get_uri(request));
    }

    void resourceReceivedResponse(WebKitWebResource* resource)
    {
        if (resource != m_resource)
            return;

        checkActiveURI("/simple-style.css");
    }

    void resourceReceivedData(WebKitWebResource* resource, guint64 bytesReceived)
    {
    }

    void resourceFinished(WebKitWebResource* resource)
    {
        if (resource == m_resource)
            checkActiveURI("/simple-style.css");
        ResourcesTest::resourceFinished(resource);
    }

    void resourceFailed(WebKitWebResource*, GError*)
    {
        g_assert_not_reached();
    }

    CString m_activeURI;

private:
    void checkActiveURI(const char* uri)
    {
        ASSERT_CMP_CSTRING(m_activeURI, ==, kServer->getURIForPath(uri));
    }
};

static void testWebResourceActiveURI(ResourceURITrackingTest* test, gconstpointer)
{
    test->loadURI(kServer->getURIForPath("/redirected-css.html").data());
    test->waitUntilResourceLoadFinished();
}

static void testWebResourceGetData(ResourcesTest* test, gconstpointer)
{
    test->loadURI(kServer->getURIForPath("/").data());
    // FIXME: this should be 4 instead of 3, but we don't get the css image resource
    // due to bug https://bugs.webkit.org/show_bug.cgi?id=78510.
    test->waitUntilResourcesLoaded(3);

    WebKitWebResource* resource = webkit_web_view_get_main_resource(test->m_webView);
    g_assert(resource);
    test->checkResourceData(resource);

    GList* subresources = test->subresources();
    for (GList* item = subresources; item; item = g_list_next(item))
        test->checkResourceData(WEBKIT_WEB_RESOURCE(item->data));
}

static void testWebViewResourcesHistoryCache(SingleResourceLoadTest* test, gconstpointer)
{
    CString javascriptURI = kServer->getURIForPath("/javascript.html");
    test->loadURI(javascriptURI.data());
    test->waitUntilResourceLoadFinished();
    WebKitWebResource* resource = webkit_web_view_get_main_resource(test->m_webView);
    g_assert(resource);
    g_assert_cmpstr(webkit_web_resource_get_uri(resource), ==, javascriptURI.data());

    CString simpleStyleCSSURI = kServer->getURIForPath("/simple-style-css.html");
    test->loadURI(simpleStyleCSSURI.data());
    test->waitUntilResourceLoadFinished();
    resource = webkit_web_view_get_main_resource(test->m_webView);
    g_assert(resource);
    g_assert_cmpstr(webkit_web_resource_get_uri(resource), ==, simpleStyleCSSURI.data());

    test->goBack();
    test->waitUntilResourceLoadFinished();
    resource = webkit_web_view_get_main_resource(test->m_webView);
    g_assert(resource);
    g_assert_cmpstr(webkit_web_resource_get_uri(resource), ==, javascriptURI.data());

    test->goForward();
    test->waitUntilResourceLoadFinished();
    resource = webkit_web_view_get_main_resource(test->m_webView);
    g_assert(resource);
    g_assert_cmpstr(webkit_web_resource_get_uri(resource), ==, simpleStyleCSSURI.data());
}

class SendRequestTest: public SingleResourceLoadTest {
public:
    MAKE_GLIB_TEST_FIXTURE(SendRequestTest);

    void resourceSentRequest(WebKitWebResource* resource, WebKitURIRequest* request, WebKitURIResponse* redirectResponse)
    {
        if (resource != m_resource)
            return;

        g_assert_cmpstr(webkit_uri_request_get_uri(request), ==, m_expectedNewResourceURI.data());
        g_assert_cmpstr(webkit_uri_request_get_uri(request), ==, webkit_web_resource_get_uri(resource));

        SingleResourceLoadTest::resourceSentRequest(resource, request, redirectResponse);
    }

    void resourceFailed(WebKitWebResource* resource, GError* error)
    {
        if (resource != m_resource)
            return;

        g_assert_cmpstr(webkit_web_resource_get_uri(resource), ==, m_expectedCancelledResourceURI.data());
        g_assert_error(error, WEBKIT_NETWORK_ERROR, WEBKIT_NETWORK_ERROR_CANCELLED);

        SingleResourceLoadTest::resourceFailed(resource, error);
    }

    void setExpectedNewResourceURI(const CString& uri)
    {
        m_expectedNewResourceURI = uri;
    }

    void setExpectedCancelledResourceURI(const CString& uri)
    {
        m_expectedCancelledResourceURI = uri;
    }

    CString m_expectedNewResourceURI;
    CString m_expectedCancelledResourceURI;
};

static void testWebResourceSendRequest(SendRequestTest* test, gconstpointer)
{
    test->setExpectedNewResourceURI(kServer->getURIForPath("/javascript.js"));
    test->loadURI(kServer->getURIForPath("relative-javascript.html").data());
    test->waitUntilResourceLoadFinished();
    g_assert(test->m_resource);

    Vector<SingleResourceLoadTest::LoadEvents>& events = test->m_loadEvents;
    g_assert_cmpint(events.size(), ==, 5);
    g_assert_cmpint(events[0], ==, SingleResourceLoadTest::Started);
    g_assert_cmpint(events[1], ==, SingleResourceLoadTest::SentRequest);
    g_assert_cmpint(events[2], ==, SingleResourceLoadTest::ReceivedResponse);
    g_assert_cmpint(events[3], ==, SingleResourceLoadTest::ReceivedData);
    g_assert_cmpint(events[4], ==, SingleResourceLoadTest::Finished);
    events.clear();

    // Cancel request.
    test->setExpectedCancelledResourceURI(kServer->getURIForPath("/cancel-this.js"));
    test->loadURI(kServer->getURIForPath("/resource-to-cancel.html").data());
    test->waitUntilResourceLoadFinished();
    g_assert(test->m_resource);

    g_assert_cmpint(events.size(), ==, 3);
    g_assert_cmpint(events[0], ==, SingleResourceLoadTest::Started);
    g_assert_cmpint(events[1], ==, SingleResourceLoadTest::Failed);
    g_assert_cmpint(events[2], ==, SingleResourceLoadTest::Finished);
    events.clear();
}

static void addCacheHTTPHeadersToResponse(SoupMessage* message)
{
    // The actual date doesn't really matter.
    SoupDate* soupDate = soup_date_new_from_now(0);
    GOwnPtr<char> date(soup_date_to_string(soupDate, SOUP_DATE_HTTP));
    soup_message_headers_append(message->response_headers, "Last-Modified", date.get());
    soup_date_free(soupDate);
    soup_message_headers_append(message->response_headers, "Cache-control", "public, max-age=31536000");
    soupDate = soup_date_new_from_now(3600);
    date.set(soup_date_to_string(soupDate, SOUP_DATE_HTTP));
    soup_message_headers_append(message->response_headers, "Expires", date.get());
    soup_date_free(soupDate);
}

static void serverCallback(SoupServer* server, SoupMessage* message, const char* path, GHashTable*, SoupClientContext*, gpointer)
{
    if (message->method != SOUP_METHOD_GET) {
        soup_message_set_status(message, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    soup_message_set_status(message, SOUP_STATUS_OK);

    if (soup_message_headers_get_one(message->request_headers, "If-Modified-Since")) {
        soup_message_set_status(message, SOUP_STATUS_NOT_MODIFIED);
        soup_message_body_complete(message->response_body);
        return;
    }

    if (g_str_equal(path, "/")) {
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, kIndexHtml, strlen(kIndexHtml));
    } else if (g_str_equal(path, "/javascript.html")) {
        static const char* javascriptHtml = "<html><head><script language='javascript' src='/javascript.js'></script></head><body></body></html>";
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, javascriptHtml, strlen(javascriptHtml));
    } else if (g_str_equal(path, "/image.html")) {
        static const char* imageHTML = "<html><body><img src='/blank.ico'></img></body></html>";
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, imageHTML, strlen(imageHTML));
    } else if (g_str_equal(path, "/redirected-css.html")) {
        static const char* redirectedCSSHtml = "<html><head><link rel='stylesheet' href='/redirected.css' type='text/css'></head><body></html>";
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, redirectedCSSHtml, strlen(redirectedCSSHtml));
    } else if (g_str_equal(path, "/invalid-css.html")) {
        static const char* invalidCSSHtml = "<html><head><link rel='stylesheet' href='/invalid.css' type='text/css'></head><body></html>";
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, invalidCSSHtml, strlen(invalidCSSHtml));
    } else if (g_str_equal(path, "/simple-style-css.html")) {
        static const char* simpleStyleCSSHtml = "<html><head><link rel='stylesheet' href='/simple-style.css' type='text/css'></head><body></html>";
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, simpleStyleCSSHtml, strlen(simpleStyleCSSHtml));
    } else if (g_str_equal(path, "/style.css")) {
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, kStyleCSS, strlen(kStyleCSS));
        addCacheHTTPHeadersToResponse(message);
    } else if (g_str_equal(path, "/javascript.js")) {
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, kJavascript, strlen(kJavascript));
        soup_message_headers_append(message->response_headers, "Content-Type", "text/javascript");
        soup_message_headers_append(message->response_headers, "Content-Disposition", "filename=JavaScript.js");
    } else if (g_str_equal(path, "/relative-javascript.html")) {
        static const char* javascriptRelativeHTML = "<html><head><script language='javascript' src='remove-this/javascript.js'></script></head><body></body></html>";
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, javascriptRelativeHTML, strlen(javascriptRelativeHTML));
    } else if (g_str_equal(path, "/resource-to-cancel.html")) {
        static const char* resourceToCancelHTML = "<html><head><script language='javascript' src='cancel-this.js'></script></head><body></body></html>";
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, resourceToCancelHTML, strlen(resourceToCancelHTML));
    } else if (g_str_equal(path, "/blank.ico")) {
        GOwnPtr<char> filePath(g_build_filename(Test::getWebKit1TestResoucesDir().data(), path, NULL));
        char* contents;
        gsize contentsLength;
        g_file_get_contents(filePath.get(), &contents, &contentsLength, 0);
        soup_message_body_append(message->response_body, SOUP_MEMORY_TAKE, contents, contentsLength);
        addCacheHTTPHeadersToResponse(message);
    } else if (g_str_equal(path, "/simple-style.css")) {
        static const char* simpleCSS =
            "body {"
            "    margin: 0px;"
            "    padding: 0px;"
            "}";
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, simpleCSS, strlen(simpleCSS));
        soup_message_headers_append(message->response_headers, "Content-Type", "text/css");
    } else if (g_str_equal(path, "/redirected.css")) {
        soup_message_set_status(message, SOUP_STATUS_MOVED_PERMANENTLY);
        soup_message_headers_append(message->response_headers, "Location", "/simple-style.css");
    } else if (g_str_equal(path, "/invalid.css"))
        soup_message_set_status(message, SOUP_STATUS_CANT_CONNECT);
    else
        soup_message_set_status(message, SOUP_STATUS_NOT_FOUND);
    soup_message_body_complete(message->response_body);
}

void beforeAll()
{
    kServer = new WebKitTestServer();
    kServer->run(serverCallback);

    webkit_web_context_set_web_extensions_directory(webkit_web_context_get_default(), WEBKIT_TEST_WEB_EXTENSIONS_DIR);

    ResourcesTest::add("WebKitWebView", "resources", testWebViewResources);
    SingleResourceLoadTest::add("WebKitWebResource", "loading", testWebResourceLoading);
    SingleResourceLoadTest::add("WebKitWebResource", "response", testWebResourceResponse);
    SingleResourceLoadTest::add("WebKitWebResource", "mime-type", testWebResourceMimeType);
    SingleResourceLoadTest::add("WebKitWebResource", "suggested-filename", testWebResourceSuggestedFilename);
    ResourceURITrackingTest::add("WebKitWebResource", "active-uri", testWebResourceActiveURI);
    ResourcesTest::add("WebKitWebResource", "get-data", testWebResourceGetData);
    SingleResourceLoadTest::add("WebKitWebView", "history-cache", testWebViewResourcesHistoryCache);
    SendRequestTest::add("WebKitWebPage", "send-request", testWebResourceSendRequest);
}

void afterAll()
{
    delete kServer;
}
