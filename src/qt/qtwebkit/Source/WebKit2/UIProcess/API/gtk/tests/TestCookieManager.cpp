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
#include <glib/gstdio.h>

static WebKitTestServer* kServer;
static char* kTempDirectory;

static const char* kFirstPartyDomain = "127.0.0.1";
static const char* kThirdPartyDomain = "localhost";
static const char* kIndexHtmlFormat =
    "<html><body>"
    " <p>WebKitGTK+ Cookie Manager test</p>"
    " <img src='http://localhost:%u/image.png' width=5 height=5></img>"
    "</body></html>";

class CookieManagerTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(CookieManagerTest);

    static void cookiesChangedCallback(WebKitCookieManager*, CookieManagerTest* test)
    {
        test->m_cookiesChanged = true;
        if (test->m_finishLoopWhenCookiesChange)
            g_main_loop_quit(test->m_mainLoop);
    }

    CookieManagerTest()
        : WebViewTest()
        , m_cookieManager(webkit_web_context_get_cookie_manager(webkit_web_view_get_context(m_webView)))
        , m_acceptPolicy(WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY)
        , m_domains(0)
        , m_cookiesChanged(false)
        , m_finishLoopWhenCookiesChange(false)
    {
        g_signal_connect(m_cookieManager, "changed", G_CALLBACK(cookiesChangedCallback), this);
    }

    ~CookieManagerTest()
    {
        g_strfreev(m_domains);
        g_signal_handlers_disconnect_matched(m_cookieManager, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
        if (m_cookiesTextFile)
            g_unlink(m_cookiesTextFile.get());
        if (m_cookiesSQLiteFile)
            g_unlink(m_cookiesSQLiteFile.get());
    }

    void setPersistentStorage(WebKitCookiePersistentStorage storage)
    {
        const char* filename = 0;
        switch (storage) {
        case WEBKIT_COOKIE_PERSISTENT_STORAGE_TEXT:
            if (!m_cookiesTextFile)
                m_cookiesTextFile.set(g_build_filename(kTempDirectory, "cookies.txt", NULL));
            filename = m_cookiesTextFile.get();
            break;
        case WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE:
            if (!m_cookiesSQLiteFile)
                m_cookiesSQLiteFile.set(g_build_filename(kTempDirectory, "cookies.db", NULL));
            filename = m_cookiesSQLiteFile.get();
            break;
        default:
            g_assert_not_reached();
        }
        webkit_cookie_manager_set_persistent_storage(m_cookieManager, filename, storage);
    }

    static void getAcceptPolicyReadyCallback(GObject* object, GAsyncResult* result, gpointer userData)
    {
        GOwnPtr<GError> error;
        WebKitCookieAcceptPolicy policy = webkit_cookie_manager_get_accept_policy_finish(WEBKIT_COOKIE_MANAGER(object), result, &error.outPtr());
        g_assert(!error.get());

        CookieManagerTest* test = static_cast<CookieManagerTest*>(userData);
        test->m_acceptPolicy = policy;
        g_main_loop_quit(test->m_mainLoop);
    }

    WebKitCookieAcceptPolicy getAcceptPolicy()
    {
        m_acceptPolicy = WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY;
        webkit_cookie_manager_get_accept_policy(m_cookieManager, 0, getAcceptPolicyReadyCallback, this);
        g_main_loop_run(m_mainLoop);

        return m_acceptPolicy;
    }

    void setAcceptPolicy(WebKitCookieAcceptPolicy policy)
    {
        webkit_cookie_manager_set_accept_policy(m_cookieManager, policy);
    }

    static void getDomainsReadyCallback(GObject* object, GAsyncResult* result, gpointer userData)
    {
        GOwnPtr<GError> error;
        char** domains = webkit_cookie_manager_get_domains_with_cookies_finish(WEBKIT_COOKIE_MANAGER(object), result, &error.outPtr());
        g_assert(!error.get());

        CookieManagerTest* test = static_cast<CookieManagerTest*>(userData);
        test->m_domains = domains;
        g_main_loop_quit(test->m_mainLoop);
    }

    char** getDomains()
    {
        g_strfreev(m_domains);
        m_domains = 0;
        webkit_cookie_manager_get_domains_with_cookies(m_cookieManager, 0, getDomainsReadyCallback, this);
        g_main_loop_run(m_mainLoop);

        return m_domains;
    }

    bool hasDomain(const char* domain)
    {
        if (!m_domains)
            return false;

        for (size_t i = 0; m_domains[i]; ++i)
            if (g_str_equal(m_domains[i], domain))
                return true;
        return false;
    }

    void deleteCookiesForDomain(const char* domain)
    {
        webkit_cookie_manager_delete_cookies_for_domain(m_cookieManager, domain);
    }

    void deleteAllCookies()
    {
        webkit_cookie_manager_delete_all_cookies(m_cookieManager);
    }

    void waitUntilCookiesChanged()
    {
        m_cookiesChanged = false;
        m_finishLoopWhenCookiesChange = true;
        g_main_loop_run(m_mainLoop);
        m_finishLoopWhenCookiesChange = false;
    }

    WebKitCookieManager* m_cookieManager;
    WebKitCookieAcceptPolicy m_acceptPolicy;
    char** m_domains;
    bool m_cookiesChanged;
    bool m_finishLoopWhenCookiesChange;
    GOwnPtr<char> m_cookiesTextFile;
    GOwnPtr<char> m_cookiesSQLiteFile;
};

static void testCookieManagerAcceptPolicy(CookieManagerTest* test, gconstpointer)
{
    // Default policy is NO_THIRD_PARTY.
    g_assert_cmpint(test->getAcceptPolicy(), ==, WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY);
    test->loadURI(kServer->getURIForPath("/index.html").data());
    test->waitUntilLoadFinished();
    char** domains = test->getDomains();
    g_assert(domains);
    g_assert_cmpint(g_strv_length(domains), ==, 1);
    g_assert_cmpstr(domains[0], ==, kFirstPartyDomain);
    test->deleteAllCookies();

    test->setAcceptPolicy(WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS);
    g_assert_cmpint(test->getAcceptPolicy(), ==, WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS);
    test->loadURI(kServer->getURIForPath("/index.html").data());
    test->waitUntilLoadFinished();
    domains = test->getDomains();
    g_assert(domains);
    g_assert_cmpint(g_strv_length(domains), ==, 2);
    g_assert(test->hasDomain(kFirstPartyDomain));
    g_assert(test->hasDomain(kThirdPartyDomain));
    test->deleteAllCookies();

    test->setAcceptPolicy(WEBKIT_COOKIE_POLICY_ACCEPT_NEVER);
    g_assert_cmpint(test->getAcceptPolicy(), ==, WEBKIT_COOKIE_POLICY_ACCEPT_NEVER);
    test->loadURI(kServer->getURIForPath("/index.html").data());
    test->waitUntilLoadFinished();
    domains = test->getDomains();
    g_assert(domains);
    g_assert_cmpint(g_strv_length(domains), ==, 0);
}

static void testCookieManagerDeleteCookies(CookieManagerTest* test, gconstpointer)
{
    test->setAcceptPolicy(WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS);
    test->loadURI(kServer->getURIForPath("/index.html").data());
    test->waitUntilLoadFinished();
    g_assert_cmpint(g_strv_length(test->getDomains()), ==, 2);

    // Delete first party cookies.
    test->deleteCookiesForDomain(kFirstPartyDomain);
    g_assert_cmpint(g_strv_length(test->getDomains()), ==, 1);

    // Delete third party cookies.
    test->deleteCookiesForDomain(kThirdPartyDomain);
    g_assert_cmpint(g_strv_length(test->getDomains()), ==, 0);

    test->loadURI(kServer->getURIForPath("/index.html").data());
    test->waitUntilLoadFinished();
    g_assert_cmpint(g_strv_length(test->getDomains()), ==, 2);

    // Delete all cookies.
    test->deleteAllCookies();
    g_assert_cmpint(g_strv_length(test->getDomains()), ==, 0);
}

static void testCookieManagerCookiesChanged(CookieManagerTest* test, gconstpointer)
{
    g_assert(!test->m_cookiesChanged);
    test->setAcceptPolicy(WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS);
    test->loadURI(kServer->getURIForPath("/index.html").data());
    test->waitUntilLoadFinished();
    g_assert(test->m_cookiesChanged);

    test->deleteCookiesForDomain(kFirstPartyDomain);
    test->waitUntilCookiesChanged();
    g_assert(test->m_cookiesChanged);

    test->deleteAllCookies();
    test->waitUntilCookiesChanged();
    g_assert(test->m_cookiesChanged);
}

static void testCookieManagerPersistentStorage(CookieManagerTest* test, gconstpointer)
{
    test->setAcceptPolicy(WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS);

    // Text storage using a new file.
    test->setPersistentStorage(WEBKIT_COOKIE_PERSISTENT_STORAGE_TEXT);
    char** domains = test->getDomains();
    g_assert(domains);
    g_assert_cmpint(g_strv_length(domains), ==, 0);

    test->loadURI(kServer->getURIForPath("/index.html").data());
    test->waitUntilLoadFinished();
    g_assert(test->m_cookiesChanged);
    domains = test->getDomains();
    g_assert(domains);
    g_assert_cmpint(g_strv_length(domains), ==, 2);


    // SQLite storage using a new file.
    test->setPersistentStorage(WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);
    domains = test->getDomains();
    g_assert(domains);
    g_assert_cmpint(g_strv_length(domains), ==, 0);

    test->loadURI(kServer->getURIForPath("/index.html").data());
    test->waitUntilLoadFinished();
    g_assert(test->m_cookiesChanged);
    domains = test->getDomains();
    g_assert(domains);
    g_assert_cmpint(g_strv_length(domains), ==, 2);

    // Text storage using an existing file.
    test->setPersistentStorage(WEBKIT_COOKIE_PERSISTENT_STORAGE_TEXT);
    domains = test->getDomains();
    g_assert(domains);
    g_assert_cmpint(g_strv_length(domains), ==, 2);
    test->deleteAllCookies();
    g_assert_cmpint(g_strv_length(test->getDomains()), ==, 0);

    // SQLite storage with an existing file.
    test->setPersistentStorage(WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);
    domains = test->getDomains();
    g_assert(domains);
    g_assert_cmpint(g_strv_length(domains), ==, 2);
    test->deleteAllCookies();
    g_assert_cmpint(g_strv_length(test->getDomains()), ==, 0);
}

static void serverCallback(SoupServer* server, SoupMessage* message, const char* path, GHashTable*, SoupClientContext*, gpointer)
{
    if (message->method != SOUP_METHOD_GET) {
        soup_message_set_status(message, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    soup_message_set_status(message, SOUP_STATUS_OK);
    if (g_str_equal(path, "/index.html")) {
        char* indexHtml = g_strdup_printf(kIndexHtmlFormat, soup_server_get_port(server));
        soup_message_headers_replace(message->response_headers, "Set-Cookie", "foo=bar; Max-Age=60");
        soup_message_body_append(message->response_body, SOUP_MEMORY_TAKE, indexHtml, strlen(indexHtml));
    } else if (g_str_equal(path, "/image.png"))
        soup_message_headers_replace(message->response_headers, "Set-Cookie", "baz=qux; Max-Age=60");
    else
        soup_message_set_status(message, SOUP_STATUS_NOT_FOUND);
    soup_message_body_complete(message->response_body);
}

void beforeAll()
{
    kServer = new WebKitTestServer();
    kServer->run(serverCallback);

    kTempDirectory = g_dir_make_tmp("WebKit2Tests-XXXXXX", 0);
    g_assert(kTempDirectory);

    CookieManagerTest::add("WebKitCookieManager", "accept-policy", testCookieManagerAcceptPolicy);
    CookieManagerTest::add("WebKitCookieManager", "delete-cookies", testCookieManagerDeleteCookies);
    CookieManagerTest::add("WebKitCookieManager", "cookies-changed", testCookieManagerCookiesChanged);
    CookieManagerTest::add("WebKitCookieManager", "persistent-storage", testCookieManagerPersistentStorage);
}

void afterAll()
{
    delete kServer;
    g_rmdir(kTempDirectory);
}
