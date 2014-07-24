/*
 * Copyright (C) 2003, 2006, 2008, 2012 Apple Inc. All rights reserved.
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

#import "config.h"
#import "PlatformCookieJar.h"

#if !USE(CFNETWORK)

#import "BlockExceptions.h"
#import "Cookie.h"
#import "CookieStorage.h"
#import "KURL.h"
#import "NetworkStorageSession.h"
#import "WebCoreSystemInterface.h"

namespace WebCore {

static RetainPtr<NSArray> filterCookies(NSArray *unfilteredCookies)
{
    NSUInteger count = [unfilteredCookies count];
    RetainPtr<NSMutableArray> filteredCookies = adoptNS([[NSMutableArray alloc] initWithCapacity:count]);

    for (NSUInteger i = 0; i < count; ++i) {
        NSHTTPCookie *cookie = (NSHTTPCookie *)[unfilteredCookies objectAtIndex:i];

        // <rdar://problem/5632883> On 10.5, NSHTTPCookieStorage would store an empty cookie,
        // which would be sent as "Cookie: =". We have a workaround in setCookies() to prevent
        // that, but we also need to avoid sending cookies that were previously stored, and
        // there's no harm to doing this check because such a cookie is never valid.
        if (![[cookie name] length])
            continue;

        if ([cookie isHTTPOnly])
            continue;

        [filteredCookies.get() addObject:cookie];
    }

    return filteredCookies;
}

String cookiesForDOM(const NetworkStorageSession& session, const KURL&, const KURL& url)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    NSArray *cookies = wkHTTPCookiesForURL(session.cookieStorage().get(), url);
    return [[NSHTTPCookie requestHeaderFieldsWithCookies:filterCookies(cookies).get()] objectForKey:@"Cookie"];

    END_BLOCK_OBJC_EXCEPTIONS;
    return String();
}

String cookieRequestHeaderFieldValue(const NetworkStorageSession& session, const KURL& /*firstParty*/, const KURL& url)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    NSArray *cookies = wkHTTPCookiesForURL(session.cookieStorage().get(), url);
    return [[NSHTTPCookie requestHeaderFieldsWithCookies:cookies] objectForKey:@"Cookie"];

    END_BLOCK_OBJC_EXCEPTIONS;
    return String();
}

void setCookiesFromDOM(const NetworkStorageSession& session, const KURL& firstParty, const KURL& url, const String& cookieStr)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    // <rdar://problem/5632883> On 10.5, NSHTTPCookieStorage would store an empty cookie,
    // which would be sent as "Cookie: =".
    if (cookieStr.isEmpty())
        return;

    // <http://bugs.webkit.org/show_bug.cgi?id=6531>, <rdar://4409034>
    // cookiesWithResponseHeaderFields doesn't parse cookies without a value
    String cookieString = cookieStr.contains('=') ? cookieStr : cookieStr + "=";

    NSURL *cookieURL = url;
    RetainPtr<NSArray> filteredCookies = filterCookies([NSHTTPCookie cookiesWithResponseHeaderFields:[NSDictionary dictionaryWithObject:cookieString forKey:@"Set-Cookie"] forURL:cookieURL]);
    ASSERT([filteredCookies.get() count] <= 1);

    wkSetHTTPCookiesForURL(session.cookieStorage().get(), filteredCookies.get(), cookieURL, firstParty);

    END_BLOCK_OBJC_EXCEPTIONS;
}

bool cookiesEnabled(const NetworkStorageSession& session, const KURL& /*firstParty*/, const KURL& /*url*/)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    NSHTTPCookieAcceptPolicy cookieAcceptPolicy = static_cast<NSHTTPCookieAcceptPolicy>(wkGetHTTPCookieAcceptPolicy(session.cookieStorage().get()));
    return cookieAcceptPolicy == NSHTTPCookieAcceptPolicyAlways || cookieAcceptPolicy == NSHTTPCookieAcceptPolicyOnlyFromMainDocumentDomain;

    END_BLOCK_OBJC_EXCEPTIONS;
    return false;
}

bool getRawCookies(const NetworkStorageSession& session, const KURL& /*firstParty*/, const KURL& url, Vector<Cookie>& rawCookies)
{
    rawCookies.clear();
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    NSArray *cookies = wkHTTPCookiesForURL(session.cookieStorage().get(), url);
    NSUInteger count = [cookies count];
    rawCookies.reserveCapacity(count);
    for (NSUInteger i = 0; i < count; ++i) {
        NSHTTPCookie *cookie = (NSHTTPCookie *)[cookies objectAtIndex:i];
        NSTimeInterval expires = [[cookie expiresDate] timeIntervalSince1970] * 1000;
        rawCookies.uncheckedAppend(Cookie([cookie name], [cookie value], [cookie domain], [cookie path], expires,
            [cookie isHTTPOnly], [cookie isSecure], [cookie isSessionOnly]));
    }

    END_BLOCK_OBJC_EXCEPTIONS;
    return true;
}

void deleteCookie(const NetworkStorageSession& session, const KURL& url, const String& cookieName)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    NSURL *cookieURL = url;
    RetainPtr<CFHTTPCookieStorageRef> cookieStorage = session.cookieStorage();
    NSArray *cookies = wkHTTPCookiesForURL(cookieStorage.get(), cookieURL);

    NSString *cookieNameString = cookieName;

    NSUInteger count = [cookies count];
    for (NSUInteger i = 0; i < count; ++i) {
        NSHTTPCookie *cookie = (NSHTTPCookie *)[cookies objectAtIndex:i];
        if ([[cookie name] isEqualToString:cookieNameString])
            wkDeleteHTTPCookie(cookieStorage.get(), cookie);
    }

    END_BLOCK_OBJC_EXCEPTIONS;
}

void getHostnamesWithCookies(const NetworkStorageSession& session, HashSet<String>& hostnames)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    NSArray *cookies = wkHTTPCookies(session.cookieStorage().get());
    
    for (NSHTTPCookie* cookie in cookies)
        hostnames.add([cookie domain]);
    
    END_BLOCK_OBJC_EXCEPTIONS;
}

void deleteCookiesForHostname(const NetworkStorageSession& session, const String& hostname)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    RetainPtr<CFHTTPCookieStorageRef> cookieStorage = session.cookieStorage();
    NSArray *cookies = wkHTTPCookies(cookieStorage.get());
    if (!cookies)
        return;
    
    for (NSHTTPCookie* cookie in cookies) {
        if (hostname == String([cookie domain]))
            wkDeleteHTTPCookie(cookieStorage.get(), cookie);
    }
    
    END_BLOCK_OBJC_EXCEPTIONS;
}

void deleteAllCookies(const NetworkStorageSession& session)
{
    wkDeleteAllHTTPCookies(session.cookieStorage().get());
}

}

#endif // !USE(CFNETWORK)
