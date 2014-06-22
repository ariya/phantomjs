/*
 * Copyright (C) 2013 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "PlatformCookieJar.h"

#include "CookieManager.h"
#include "NotImplemented.h"

namespace WebCore {

void setCookiesFromDOM(const NetworkStorageSession&, const KURL& /*firstParty*/, const KURL& url, const String& value)
{
    cookieManager().setCookies(url, value, NoHttpOnlyCookie);
}

String cookiesForDOM(const NetworkStorageSession&, const KURL& /*firstParty*/, const KURL& url)
{
    // 'HttpOnly' cookies should no be accessible from scripts, so we filter them out here.
    return cookieManager().getCookie(url, NoHttpOnlyCookie);
}

String cookieRequestHeaderFieldValue(const NetworkStorageSession&, const KURL& /*firstParty*/, const KURL& url)
{
    return cookieManager().getCookie(url, WithHttpOnlyCookies);
}

bool cookiesEnabled(const NetworkStorageSession&, const KURL& /*firstParty*/, const KURL& /*url*/)
{
    return !cookieManager().cookieJar().isEmpty();
}

bool getRawCookies(const NetworkStorageSession&, const KURL& /*firstParty*/, const KURL& url, Vector<Cookie>& rawCookies)
{
    Vector<RefPtr<ParsedCookie> > result;
    cookieManager().getRawCookies(result, url, WithHttpOnlyCookies);
    for (size_t i = 0; i < result.size(); i++)
        result[i]->appendWebCoreCookie(rawCookies);
    return true;
}

void deleteCookie(const NetworkStorageSession&, const KURL& url, const String& name)
{
    cookieManager().removeCookieWithName(url, name);
}

void getHostnamesWithCookies(const NetworkStorageSession&, HashSet<String>& /*hostnames*/)
{
    notImplemented();
}

void deleteCookiesForHostname(const NetworkStorageSession&, const String& /*hostname*/)
{
    notImplemented();
}

void deleteAllCookies(const NetworkStorageSession&)
{
    notImplemented();
}

}
