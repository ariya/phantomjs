/*
 *  Copyright (C) 20010 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "SoupURIUtils.h"

#include <wtf/gobject/GOwnPtr.h>
#include <libsoup/soup.h>

namespace WebCore {

// Motivated by https://bugs.webkit.org/show_bug.cgi?id=38956. libsoup
// does not add the password to the URL when calling
// soup_uri_to_string, and thus the requests are not properly
// built. Fixing soup_uri_to_string is a no-no as the maintainer does
// not want to break compatibility with previous implementations
KURL soupURIToKURL(SoupURI* soupURI)
{
    GOwnPtr<gchar> urlString(soup_uri_to_string(soupURI, FALSE));
    KURL url(KURL(), String::fromUTF8(urlString.get()));

    if (!soupURI->password)
        return url;

    url.setPass(String::fromUTF8(soupURI->password));
    return url;
}

}
