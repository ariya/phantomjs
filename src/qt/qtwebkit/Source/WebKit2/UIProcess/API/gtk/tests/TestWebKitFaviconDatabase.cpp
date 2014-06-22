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

#include "WebKitTestServer.h"
#include "WebViewTest.h"
#include <glib/gstdio.h>
#include <libsoup/soup.h>
#include <wtf/gobject/GOwnPtr.h>

static WebKitTestServer* kServer;
static char* kTempDirectory;

class FaviconDatabaseTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(FaviconDatabaseTest);

    FaviconDatabaseTest()
        : m_webContext(webkit_web_context_get_default())
        , m_favicon(0)
        , m_error(0)
        , m_faviconNotificationReceived(false)
    {
        WebKitFaviconDatabase* database = webkit_web_context_get_favicon_database(m_webContext);
        g_signal_connect(database, "favicon-changed", G_CALLBACK(faviconChangedCallback), this);
    }

    ~FaviconDatabaseTest()
    {
        if (m_favicon)
            cairo_surface_destroy(m_favicon);

        WebKitFaviconDatabase* database = webkit_web_context_get_favicon_database(m_webContext);
        g_signal_handlers_disconnect_matched(database, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
    }

    static void faviconChangedCallback(WebKitFaviconDatabase* database, const char* pageURI, const char* faviconURI, FaviconDatabaseTest* test)
    {
        if (!g_strcmp0(webkit_web_view_get_uri(test->m_webView), pageURI))
            test->m_faviconURI = faviconURI;
    }

    static void viewFaviconChangedCallback(WebKitWebView* webView, GParamSpec* pspec, gpointer data)
    {
        FaviconDatabaseTest* test = static_cast<FaviconDatabaseTest*>(data);
        g_assert(test->m_webView == webView);
        test->m_faviconNotificationReceived = true;
        test->quitMainLoop();
    }

    static void getFaviconCallback(GObject* sourceObject, GAsyncResult* result, void* data)
    {
        FaviconDatabaseTest* test = static_cast<FaviconDatabaseTest*>(data);
        WebKitFaviconDatabase* database = webkit_web_context_get_favicon_database(test->m_webContext);
        test->m_favicon = webkit_favicon_database_get_favicon_finish(database, result, &test->m_error.outPtr());
        test->quitMainLoop();
    }

    void waitUntilFaviconChanged()
    {
        m_faviconNotificationReceived = false;
        unsigned long handlerID = g_signal_connect(m_webView, "notify::favicon", G_CALLBACK(viewFaviconChangedCallback), this);
        g_main_loop_run(m_mainLoop);
        g_signal_handler_disconnect(m_webView, handlerID);
    }

    void getFaviconForPageURIAndWaitUntilReady(const char* pageURI)
    {
        m_error.clear();
        if (m_favicon) {
            cairo_surface_destroy(m_favicon);
            m_favicon = 0;
        }

        WebKitFaviconDatabase* database = webkit_web_context_get_favicon_database(m_webContext);
        webkit_favicon_database_get_favicon(database, pageURI, 0, getFaviconCallback, this);
        g_main_loop_run(m_mainLoop);
    }

    WebKitWebContext* m_webContext;
    cairo_surface_t* m_favicon;
    CString m_faviconURI;
    GOwnPtr<GError> m_error;
    bool m_faviconNotificationReceived;
};

static void
serverCallback(SoupServer* server, SoupMessage* message, const char* path, GHashTable* query, SoupClientContext* context, void* data)
{
    if (message->method != SOUP_METHOD_GET) {
        soup_message_set_status(message, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    if (g_str_equal(path, "/favicon.ico")) {
        soup_message_set_status(message, SOUP_STATUS_NOT_FOUND);
        soup_message_body_complete(message->response_body);
        return;
    }

    char* contents;
    gsize length;
    if (g_str_equal(path, "/icon/favicon.ico")) {
        GOwnPtr<char> pathToFavicon(g_build_filename(Test::getWebKit1TestResoucesDir().data(), "blank.ico", NULL));
        g_file_get_contents(pathToFavicon.get(), &contents, &length, 0);
        soup_message_body_append(message->response_body, SOUP_MEMORY_TAKE, contents, length);
    } else if (g_str_equal(path, "/nofavicon")) {
        static const char* noFaviconHTML = "<html><head><body>test</body></html>";
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, noFaviconHTML, strlen(noFaviconHTML));
    } else {
        static const char* contentsHTML = "<html><head><link rel='icon' href='/icon/favicon.ico' type='image/x-ico; charset=binary'></head><body>test</body></html>";
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, contentsHTML, strlen(contentsHTML));
    }

    soup_message_set_status(message, SOUP_STATUS_OK);
    soup_message_body_complete(message->response_body);
}

static void testNotInitialized(FaviconDatabaseTest* test)
{
    // Try to retrieve a valid favicon from a not initialized database.
    test->getFaviconForPageURIAndWaitUntilReady(kServer->getURIForPath("/foo").data());
    g_assert(!test->m_favicon);
    g_assert(test->m_error);
    g_assert_cmpint(test->m_error->code, ==, WEBKIT_FAVICON_DATABASE_ERROR_NOT_INITIALIZED);
}

static void testSetDirectory(FaviconDatabaseTest* test)
{
    webkit_web_context_set_favicon_database_directory(test->m_webContext, kTempDirectory);
    g_assert_cmpstr(kTempDirectory, ==, webkit_web_context_get_favicon_database_directory(test->m_webContext));
}

static void testClearDatabase(FaviconDatabaseTest* test)
{
    WebKitFaviconDatabase* database = webkit_web_context_get_favicon_database(test->m_webContext);
    webkit_favicon_database_clear(database);

    GOwnPtr<char> iconURI(webkit_favicon_database_get_favicon_uri(database, kServer->getURIForPath("/foo").data()));
    g_assert(!iconURI);
}

static void testGetFavicon(FaviconDatabaseTest* test)
{
    // We need to load the page first to ensure the icon data will be
    // in the database in case there's an associated favicon.
    test->loadURI(kServer->getURIForPath("/foo").data());
    test->waitUntilFaviconChanged();
    CString faviconURI = kServer->getURIForPath("/icon/favicon.ico");

    // Check the API retrieving a valid favicon.
    test->getFaviconForPageURIAndWaitUntilReady(kServer->getURIForPath("/foo").data());
    g_assert(test->m_favicon);
    g_assert_cmpstr(test->m_faviconURI.data(), ==, faviconURI.data());
    g_assert(!test->m_error);

    // Check that width and height match those from blank.ico (16x16 favicon).
    g_assert_cmpint(cairo_image_surface_get_width(test->m_favicon), ==, 16);
    g_assert_cmpint(cairo_image_surface_get_height(test->m_favicon), ==, 16);

    // Check that another page with the same favicon return the same icon.
    cairo_surface_t* favicon = cairo_surface_reference(test->m_favicon);
    test->loadURI(kServer->getURIForPath("/bar").data());
    // It's a new page in the database, so favicon will change twice, first to reset it
    // and then when the icon is loaded.
    test->waitUntilFaviconChanged();
    test->waitUntilFaviconChanged();
    test->getFaviconForPageURIAndWaitUntilReady(kServer->getURIForPath("/bar").data());
    g_assert(test->m_favicon);
    g_assert_cmpstr(test->m_faviconURI.data(), ==, faviconURI.data());
    g_assert(test->m_favicon == favicon);
    g_assert(!test->m_error);
    cairo_surface_destroy(favicon);

    // Check the API retrieving an invalid favicon.
    test->loadURI(kServer->getURIForPath("/nofavicon").data());
    test->waitUntilFaviconChanged();

    test->getFaviconForPageURIAndWaitUntilReady(kServer->getURIForPath("/nofavicon").data());
    g_assert(!test->m_favicon);
    g_assert(test->m_error);
}

static void testGetFaviconURI(FaviconDatabaseTest* test)
{
    WebKitFaviconDatabase* database = webkit_web_context_get_favicon_database(test->m_webContext);

    CString baseURI = kServer->getURIForPath("/foo");
    GOwnPtr<char> iconURI(webkit_favicon_database_get_favicon_uri(database, baseURI.data()));
    ASSERT_CMP_CSTRING(iconURI.get(), ==, kServer->getURIForPath("/icon/favicon.ico"));
}

static void testWebViewFavicon(FaviconDatabaseTest* test)
{
    test->m_faviconURI = CString();

    cairo_surface_t* iconFromWebView = webkit_web_view_get_favicon(test->m_webView);
    g_assert(!iconFromWebView);

    test->loadURI(kServer->getURIForPath("/foo").data());
    test->waitUntilFaviconChanged();
    g_assert(test->m_faviconNotificationReceived);
    // The icon is known and hasn't changed in the database, so notify::favicon is emitted
    // but WebKitFaviconDatabase::icon-changed isn't.
    g_assert(test->m_faviconURI.isNull());

    iconFromWebView = webkit_web_view_get_favicon(test->m_webView);
    g_assert(iconFromWebView);
    g_assert_cmpuint(cairo_image_surface_get_width(iconFromWebView), ==, 16);
    g_assert_cmpuint(cairo_image_surface_get_height(iconFromWebView), ==, 16);
}

static void testFaviconDatabase(FaviconDatabaseTest* test, gconstpointer)
{
    // These tests depend on this order to run properly so we declare them in a single one.
    // See https://bugs.webkit.org/show_bug.cgi?id=111434.
    testNotInitialized(test);
    testSetDirectory(test);
    testGetFavicon(test);
    testGetFaviconURI(test);
    testWebViewFavicon(test);
    testClearDatabase(test);
}

void beforeAll()
{
    // Start a soup server for testing.
    kServer = new WebKitTestServer();
    kServer->run(serverCallback);

    kTempDirectory = g_dir_make_tmp("WebKit2Tests-XXXXXX", 0);
    g_assert(kTempDirectory);

    // Add tests to the suite.
    FaviconDatabaseTest::add("WebKitFaviconDatabase", "favicon-database-test", testFaviconDatabase);
}

static void webkitFaviconDatabaseFinalizedCallback(gpointer, GObject*)
{
    if (!g_file_test(kTempDirectory, G_FILE_TEST_IS_DIR))
        return;

    GOwnPtr<char> filename(g_build_filename(kTempDirectory, "WebpageIcons.db", NULL));
    g_unlink(filename.get());

    g_rmdir(kTempDirectory);
}

void afterAll()
{
    delete kServer;

    // Delete the temporary files after the IconDatabase has been
    // closed, that is, once WebKitFaviconDatabase is being destroyed.
    WebKitFaviconDatabase* database = webkit_web_context_get_favicon_database(webkit_web_context_get_default());
    g_object_weak_ref(G_OBJECT(database), webkitFaviconDatabaseFinalizedCallback, 0);
}
