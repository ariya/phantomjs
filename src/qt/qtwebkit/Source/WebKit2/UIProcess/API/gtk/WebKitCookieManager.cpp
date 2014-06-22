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
#include "WebKitCookieManager.h"

#include "SoupCookiePersistentStorageType.h"
#include "WebCookieManagerProxy.h"
#include "WebKitCookieManagerPrivate.h"
#include "WebKitEnumTypes.h"
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

using namespace WebKit;

/**
 * SECTION: WebKitCookieManager
 * @Short_description: Defines how to handle cookies in a #WebKitWebContext
 * @Title: WebKitCookieManager
 *
 * The #WebKitCookieManager defines how to handle cookies in a
 * #WebKitWebContext. Get it from the context with
 * webkit_web_context_get_cookie_manager(), and use it to set where to
 * store cookies, with webkit_cookie_manager_set_persistent_storage(),
 * to get the list of domains with cookies, with
 * webkit_cookie_manager_get_domains_with_cookies(), or to set the
 * acceptance policy, with webkit_cookie_manager_get_accept_policy()
 * (among other actions).
 *
 */

enum {
    CHANGED,

    LAST_SIGNAL
};

struct _WebKitCookieManagerPrivate {
    ~_WebKitCookieManagerPrivate()
    {
        webCookieManager->stopObservingCookieChanges();
    }

    RefPtr<WebCookieManagerProxy> webCookieManager;
};

static guint signals[LAST_SIGNAL] = { 0, };

WEBKIT_DEFINE_TYPE(WebKitCookieManager, webkit_cookie_manager, G_TYPE_OBJECT)

COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_COOKIE_PERSISTENT_STORAGE_TEXT, SoupCookiePersistentStorageText);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE, SoupCookiePersistentStorageSQLite);

COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS, HTTPCookieAcceptPolicyAlways);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_COOKIE_POLICY_ACCEPT_NEVER, HTTPCookieAcceptPolicyNever);
COMPILE_ASSERT_MATCHING_ENUM(WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY, HTTPCookieAcceptPolicyOnlyFromMainDocumentDomain);

static void webkit_cookie_manager_class_init(WebKitCookieManagerClass* findClass)
{
    GObjectClass* gObjectClass = G_OBJECT_CLASS(findClass);

    /**
     * WebKitCookieManager::changed:
     * @cookie_manager: the #WebKitCookieManager
     *
     * This signal is emitted when cookies are added, removed or modified.
     */
    signals[CHANGED] =
        g_signal_new("changed",
                     G_TYPE_FROM_CLASS(gObjectClass),
                     G_SIGNAL_RUN_LAST,
                     0, 0, 0,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);
}

static void cookiesDidChange(WKCookieManagerRef, const void* clientInfo)
{
    g_signal_emit(WEBKIT_COOKIE_MANAGER(clientInfo), signals[CHANGED], 0);
}

WebKitCookieManager* webkitCookieManagerCreate(WebCookieManagerProxy* webCookieManager)
{
    WebKitCookieManager* manager = WEBKIT_COOKIE_MANAGER(g_object_new(WEBKIT_TYPE_COOKIE_MANAGER, NULL));
    manager->priv->webCookieManager = webCookieManager;

    WKCookieManagerClient wkCookieManagerClient = {
        kWKCookieManagerClientCurrentVersion,
        manager, // clientInfo
        cookiesDidChange
    };
    WKCookieManagerSetClient(toAPI(webCookieManager), &wkCookieManagerClient);
    manager->priv->webCookieManager->startObservingCookieChanges();

    return manager;
}

/**
 * webkit_cookie_manager_set_persistent_storage:
 * @cookie_manager: a #WebKitCookieManager
 * @filename: the filename to read to/write from
 * @storage: a #WebKitCookiePersistentStorage
 *
 * Set the @filename where non-session cookies are stored persistently using
 * @storage as the format to read/write the cookies.
 * Cookies are initially read from @filename to create an initial set of cookies.
 * Then, non-session cookies will be written to @filename when the WebKitCookieManager::changed
 * signal is emitted.
 * By default, @cookie_manager doesn't store the cookies persistenly, so you need to call this
 * method to keep cookies saved across sessions.
 */
void webkit_cookie_manager_set_persistent_storage(WebKitCookieManager* manager, const char* filename, WebKitCookiePersistentStorage storage)
{
    g_return_if_fail(WEBKIT_IS_COOKIE_MANAGER(manager));
    g_return_if_fail(filename);

    manager->priv->webCookieManager->stopObservingCookieChanges();
    manager->priv->webCookieManager->setCookiePersistentStorage(String::fromUTF8(filename), storage);
    manager->priv->webCookieManager->startObservingCookieChanges();
}

/**
 * webkit_cookie_manager_set_accept_policy:
 * @cookie_manager: a #WebKitCookieManager
 * @policy: a #WebKitCookieAcceptPolicy
 *
 * Set the cookie acceptance policy of @cookie_manager as @policy.
 */
void webkit_cookie_manager_set_accept_policy(WebKitCookieManager* manager, WebKitCookieAcceptPolicy policy)
{
    g_return_if_fail(WEBKIT_IS_COOKIE_MANAGER(manager));

    manager->priv->webCookieManager->setHTTPCookieAcceptPolicy(policy);
}

static void webkitCookieManagerGetAcceptPolicyCallback(WKHTTPCookieAcceptPolicy policy, WKErrorRef, void* context)
{
    GRefPtr<GTask> task = adoptGRef(G_TASK(context));
    g_task_return_int(task.get(), policy);
}

/**
 * webkit_cookie_manager_get_accept_policy:
 * @cookie_manager: a #WebKitCookieManager
 * @cancellable: (allow-none): a #GCancellable or %NULL to ignore
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously get the cookie acceptance policy of @cookie_manager.
 *
 * When the operation is finished, @callback will be called. You can then call
 * webkit_cookie_manager_get_accept_policy_finish() to get the result of the operation.
 */
void webkit_cookie_manager_get_accept_policy(WebKitCookieManager* manager, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer userData)
{
    g_return_if_fail(WEBKIT_IS_COOKIE_MANAGER(manager));

    GTask* task = g_task_new(manager, cancellable, callback, userData);
    manager->priv->webCookieManager->getHTTPCookieAcceptPolicy(HTTPCookieAcceptPolicyCallback::create(task, webkitCookieManagerGetAcceptPolicyCallback));
}

/**
 * webkit_cookie_manager_get_accept_policy_finish:
 * @cookie_manager: a #WebKitCookieManager
 * @result: a #GAsyncResult
 * @error: return location for error or %NULL to ignore
 *
 * Finish an asynchronous operation started with webkit_cookie_manager_get_accept_policy().
 *
 * Returns: the cookie acceptance policy of @cookie_manager as a #WebKitCookieAcceptPolicy.
 */
WebKitCookieAcceptPolicy webkit_cookie_manager_get_accept_policy_finish(WebKitCookieManager* manager, GAsyncResult* result, GError** error)
{
    g_return_val_if_fail(WEBKIT_IS_COOKIE_MANAGER(manager), WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY);
    g_return_val_if_fail(g_task_is_valid(result, manager), WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY);

    gssize returnValue = g_task_propagate_int(G_TASK(result), error);
    return returnValue == -1 ? WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY : static_cast<WebKitCookieAcceptPolicy>(returnValue);
}

static void webkitCookieManagerGetDomainsWithCookiesCallback(WKArrayRef wkDomains, WKErrorRef, void* context)
{
    GRefPtr<GTask> task = adoptGRef(G_TASK(context));
    if (g_task_return_error_if_cancelled(task.get()))
        return;

    ImmutableArray* domains = toImpl(wkDomains);
    GPtrArray* returnValue = g_ptr_array_sized_new(domains->size());
    for (size_t i = 0; i < domains->size(); ++i) {
        WebString* domainString = static_cast<WebString*>(domains->at(i));
        String domain = domainString->string();
        if (domain.isEmpty())
            continue;
        g_ptr_array_add(returnValue, g_strdup(domain.utf8().data()));
    }
    g_ptr_array_add(returnValue, 0);
    g_task_return_pointer(task.get(), g_ptr_array_free(returnValue, FALSE), reinterpret_cast<GDestroyNotify>(g_strfreev));
}

/**
 * webkit_cookie_manager_get_domains_with_cookies:
 * @cookie_manager: a #WebKitCookieManager
 * @cancellable: (allow-none): a #GCancellable or %NULL to ignore
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously get the list of domains for which @cookie_manager contains cookies.
 *
 * When the operation is finished, @callback will be called. You can then call
 * webkit_cookie_manager_get_domains_with_cookies_finish() to get the result of the operation.
 */
void webkit_cookie_manager_get_domains_with_cookies(WebKitCookieManager* manager, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer userData)
{
    g_return_if_fail(WEBKIT_IS_COOKIE_MANAGER(manager));

    GTask* task = g_task_new(manager, cancellable, callback, userData);
    manager->priv->webCookieManager->getHostnamesWithCookies(ArrayCallback::create(task, webkitCookieManagerGetDomainsWithCookiesCallback));
}

/**
 * webkit_cookie_manager_get_domains_with_cookies_finish:
 * @cookie_manager: a #WebKitCookieManager
 * @result: a #GAsyncResult
 * @error: return location for error or %NULL to ignore
 *
 * Finish an asynchronous operation started with webkit_cookie_manager_get_domains_with_cookies().
 * The return value is a %NULL terminated list of strings which should
 * be released with g_strfreev().
 *
 * Returns: (transfer full) (array zero-terminated=1): A %NULL terminated array of domain names
 *    or %NULL in case of error.
 */
gchar** webkit_cookie_manager_get_domains_with_cookies_finish(WebKitCookieManager* manager, GAsyncResult* result, GError** error)
{
    g_return_val_if_fail(WEBKIT_IS_COOKIE_MANAGER(manager), 0);
    g_return_val_if_fail(g_task_is_valid(result, manager), 0);

    return reinterpret_cast<char**>(g_task_propagate_pointer(G_TASK(result), error));
}

/**
 * webkit_cookie_manager_delete_cookies_for_domain:
 * @cookie_manager: a #WebKitCookieManager
 * @domain: a domain name
 *
 * Remove all cookies of @cookie_manager for the given @domain.
 */
void webkit_cookie_manager_delete_cookies_for_domain(WebKitCookieManager* manager, const gchar* domain)
{
    g_return_if_fail(WEBKIT_IS_COOKIE_MANAGER(manager));
    g_return_if_fail(domain);

    manager->priv->webCookieManager->deleteCookiesForHostname(String::fromUTF8(domain));
}

/**
 * webkit_cookie_manager_delete_all_cookies:
 * @cookie_manager: a #WebKitCookieManager
 *
 * Delete all cookies of @cookie_manager
 */
void webkit_cookie_manager_delete_all_cookies(WebKitCookieManager* manager)
{
    g_return_if_fail(WEBKIT_IS_COOKIE_MANAGER(manager));

    manager->priv->webCookieManager->deleteAllCookies();
}
