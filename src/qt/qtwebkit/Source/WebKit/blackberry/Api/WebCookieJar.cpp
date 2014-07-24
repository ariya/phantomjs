/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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

#include "WebCookieJar.h"

#include "CookieManager.h"
#include "KURL.h"
#include <BlackBerryPlatformString.h>

using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

WebCookieJar::WebCookieJar()
{
}

std::vector<BlackBerry::Platform::String> WebCookieJar::cookies(const BlackBerry::Platform::String& url)
{
    KURL kurl = KURL(KURL(), url);

    Vector<RefPtr<ParsedCookie> > rawCookies;
    cookieManager().getRawCookies(rawCookies, kurl, WithHttpOnlyCookies);

    std::vector<BlackBerry::Platform::String> result;
    for (size_t i = 0; i < rawCookies.size(); ++i)
        result.push_back(rawCookies[i]->toNameValuePair());

    return result;
}

void WebCookieJar::setCookies(const BlackBerry::Platform::String& url, const std::vector<BlackBerry::Platform::String>& cookies)
{
    KURL kurl = KURL(KURL(), url);
    Vector<String> coreCookies;
    for (size_t i = 0; i < cookies.size(); ++i)
        coreCookies.append(cookies[i]);
    cookieManager().setCookies(kurl, coreCookies, WithHttpOnlyCookies);
}

}
}
