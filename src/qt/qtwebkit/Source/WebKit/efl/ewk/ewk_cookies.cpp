/*
    Copyright (C) 2010 ProFUSION embedded systems
    Copyright (C) 2010 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "ewk_cookies.h"

#include "CookieJarSoup.h"
#include "ResourceHandle.h"
#include <Eina.h>
#include <eina_safety_checks.h>
#include <glib.h>
#include <libsoup/soup.h>
#include <wtf/text/CString.h>

Eina_Bool ewk_cookies_file_set(const char* filename)
{
    SoupCookieJar* cookieJar = 0;
    if (filename)
        cookieJar = soup_cookie_jar_text_new(filename, FALSE);
    else
        cookieJar = soup_cookie_jar_new();

    if (!cookieJar)
        return false;

    soup_cookie_jar_set_accept_policy(cookieJar, SOUP_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY);

    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    SoupSessionFeature* oldjar = soup_session_get_feature(session, SOUP_TYPE_COOKIE_JAR);
    if (oldjar)
        soup_session_remove_feature(session, oldjar);

    WebCore::setSoupCookieJar(cookieJar);
    soup_session_add_feature(session, SOUP_SESSION_FEATURE(cookieJar));

    return true;
}

void ewk_cookies_clear(void)
{
    GSList* list;
    GSList* p;
    SoupCookieJar* cookieJar = WebCore::soupCookieJar();

    list = soup_cookie_jar_all_cookies(cookieJar);
    for (p = list; p; p = p->next)
        soup_cookie_jar_delete_cookie(cookieJar, (SoupCookie*)p->data);

    soup_cookies_free(list);
}

Eina_List* ewk_cookies_get_all(void)
{
    Eina_List* result = 0;
    GSList* list;
    GSList* p;
    SoupCookieJar* cookieJar = WebCore::soupCookieJar();

    list = soup_cookie_jar_all_cookies(cookieJar);
    for (p = list; p; p = p->next) {
        SoupCookie* cookie = static_cast<SoupCookie*>(p->data);
        Ewk_Cookie* ewkCookie = new Ewk_Cookie;
        ewkCookie->name = eina_stringshare_add(cookie->name);
        ewkCookie->value = eina_stringshare_add(cookie->value);
        ewkCookie->domain = eina_stringshare_add(cookie->domain);
        ewkCookie->path = eina_stringshare_add(cookie->path);
        ewkCookie->expires = soup_date_to_time_t(cookie->expires);
        ewkCookie->secure = static_cast<Eina_Bool>(cookie->secure);
        ewkCookie->http_only = static_cast<Eina_Bool>(cookie->http_only);
        result = eina_list_append(result, ewkCookie);
    }

    soup_cookies_free(list);

    return result;
}

void ewk_cookies_cookie_del(Ewk_Cookie* cookie)
{
    EINA_SAFETY_ON_NULL_RETURN(cookie);
    GSList* list;
    GSList* p;
    SoupCookieJar* cookieJar = WebCore::soupCookieJar();
    SoupCookie* cookie1 = soup_cookie_new(
        cookie->name, cookie->value, cookie->domain, cookie->path, -1);

    list = soup_cookie_jar_all_cookies(cookieJar);
    for (p = list; p; p = p->next) {
        SoupCookie* cookie2 = static_cast<SoupCookie*>(p->data);
        if (soup_cookie_equal(cookie1, cookie2)) {
            soup_cookie_jar_delete_cookie(cookieJar, cookie2);
            break;
        }
    }

    soup_cookie_free(cookie1);
    soup_cookies_free(list);
}

void ewk_cookies_cookie_free(Ewk_Cookie* cookie)
{
    EINA_SAFETY_ON_NULL_RETURN(cookie);
    eina_stringshare_del(cookie->name);
    eina_stringshare_del(cookie->value);
    eina_stringshare_del(cookie->domain);
    eina_stringshare_del(cookie->path);
    delete cookie;
}

void ewk_cookies_policy_set(Ewk_Cookie_Policy cookiePolicy)
{
    SoupCookieJar* cookieJar = WebCore::soupCookieJar();
    SoupCookieJarAcceptPolicy policy;

    policy = SOUP_COOKIE_JAR_ACCEPT_ALWAYS;
    switch (cookiePolicy) {
    case EWK_COOKIE_JAR_ACCEPT_NEVER:
        policy = SOUP_COOKIE_JAR_ACCEPT_NEVER;
        break;
    case EWK_COOKIE_JAR_ACCEPT_ALWAYS:
        policy = SOUP_COOKIE_JAR_ACCEPT_ALWAYS;
        break;
    case EWK_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY:
        policy = SOUP_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY;
        break;
    }

    soup_cookie_jar_set_accept_policy(cookieJar, policy);
}

Ewk_Cookie_Policy ewk_cookies_policy_get(void)
{
    Ewk_Cookie_Policy ewkPolicy = EWK_COOKIE_JAR_ACCEPT_ALWAYS;
    SoupCookieJar* cookieJar = WebCore::soupCookieJar();
    SoupCookieJarAcceptPolicy policy;

    policy = soup_cookie_jar_get_accept_policy(cookieJar);
    switch (policy) {
    case SOUP_COOKIE_JAR_ACCEPT_NEVER:
        ewkPolicy = EWK_COOKIE_JAR_ACCEPT_NEVER;
        break;
    case SOUP_COOKIE_JAR_ACCEPT_ALWAYS:
        ewkPolicy = EWK_COOKIE_JAR_ACCEPT_ALWAYS;
        break;
    case SOUP_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY:
        ewkPolicy = EWK_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY;
        break;
    }

    return ewkPolicy;
}
