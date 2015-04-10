/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "PlatformCookieJar.h"

#include "Cookie.h"
#include "KURL.h"
#include "NetworkingContext.h"
#include "ResourceHandle.h"
#include <windows.h>
#include <Wininet.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

void setCookiesFromDOM(const NetworkStorageSession&, const KURL&, const KURL& url, const String& value)
{
    // FIXME: Deal with firstParty argument.
    String str = url.string();
    String val = value;
    InternetSetCookie(str.charactersWithNullTermination().data(), 0, val.charactersWithNullTermination().data());
}

String cookiesForDOM(const NetworkStorageSession&, const KURL&, const KURL& url)
{
    // FIXME: Deal with firstParty argument.

    String str = url.string();

    DWORD count = 0;
    if (!InternetGetCookie(str.charactersWithNullTermination().data(), 0, 0, &count))
        return String();

    if (count <= 1) // Null terminator counts as 1.
        return String();

    Vector<UChar> buffer(count);
    if (!InternetGetCookie(str.charactersWithNullTermination().data(), 0, buffer.data(), &count))
        return String();

    buffer.shrink(count - 1); // Ignore the null terminator.
    return String::adopt(buffer);
}

String cookieRequestHeaderFieldValue(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url)
{
    // FIXME: include HttpOnly cookie
    return cookiesForDOM(session.context(), firstParty, url);
}

bool cookiesEnabled(const NetworkStorageSession& session, const KURL& /*firstParty*/, const KURL& /*url*/)
{
    return true;
}

bool getRawCookies(const NetworkStorageSession& session, const KURL& /*firstParty*/, const KURL& /*url*/, Vector<Cookie>& rawCookies)
{
    // FIXME: Not yet implemented
    rawCookies.clear();
    return false; // return true when implemented
}

void deleteCookie(const NetworkStorageSession&, const KURL&, const String&)
{
    // FIXME: Not yet implemented
}

void getHostnamesWithCookies(const NetworkStorageSession&, HashSet<String>& hostnames)
{
    // FIXME: Not yet implemented
}

void deleteCookiesForHostname(const NetworkStorageSession&, const String& hostname)
{
    // FIXME: Not yet implemented
}

void deleteAllCookies(const NetworkStorageSession&)
{
    // FIXME: Not yet implemented
}

}
