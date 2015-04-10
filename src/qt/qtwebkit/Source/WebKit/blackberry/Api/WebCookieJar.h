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

#ifndef WebCookieJar_h
#define WebCookieJar_h

#include "BlackBerryGlobal.h"

#include <vector>

namespace BlackBerry {
namespace Platform {
class String;
}

namespace WebKit {

class WebPage;

/**
 * Represents the cookie database.
 *
 * You can obtain an instance of WebCookieJar by calling WebPage::cookieJar().
 */
class BLACKBERRY_EXPORT WebCookieJar {
public:
    /**
     * Returns a list of cookies for the URL specified.
     *
     * All cookies whose domain and path match the provided URL will be returned.
     */
    std::vector<BlackBerry::Platform::String> cookies(const BlackBerry::Platform::String& url);

    /**
     * This will add the cookies provided in the list to the cookie database.
     * If a cookie with the same name and domain+path as one of the cookies
     * provided already exists, it will be replaced. Other cookies that already
     * existed will remain.
     *
     * If no domain and/or path is provided in a cookie, the domain and/or path
     * will be inferred from the provided URL.
     */
    void setCookies(const BlackBerry::Platform::String& url, const std::vector<BlackBerry::Platform::String>& cookies);

private:
    friend class WebPage;

    WebCookieJar();

    // Disable copy constructor and operator=.
    WebCookieJar(const WebCookieJar&);
    WebCookieJar& operator=(const WebCookieJar&);
};

}
}

#endif // WebCookieJar_h
