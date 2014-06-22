/*
 * Copyright (C) Julien Chaffraix <julien.chaffraix@gmail.com>
 * Copyright (C) 2011, 2012 Research In Motion Limited. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
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

#ifndef CookieMap_h
#define CookieMap_h

#include <wtf/HashMap.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

enum CookieFilter {
    NoHttpOnlyCookie,
    WithHttpOnlyCookies,
};

class ParsedCookie;

/* A cookie map is a node in the tree held by CookieManager that represents
 * cookies that matches a common domain.
 *
 * A CookieMap holds reference to the cookies that it contains and the child
 * domains that exist within the tree.
 *
 * The number of cookie per host is limited by CookieManager::s_maxCookieCountPerHost
 */

class CookieMap {

public:
    CookieMap(const String& name = "");
    ~CookieMap();

    unsigned count() const { return m_cookieVector.size(); }
    const String& getName() const { return m_name; }

    // Return false if the candidateCookie is rejected.
    bool addOrReplaceCookie(PassRefPtr<ParsedCookie> prpCandidateCookie, RefPtr<ParsedCookie>& replacedCookie, CookieFilter = WithHttpOnlyCookies);

    // Need to return the reference to the removed cookie so manager can deal with it (garbage collect).
    PassRefPtr<ParsedCookie> removeCookie(const PassRefPtr<ParsedCookie>, CookieFilter = WithHttpOnlyCookies);

    // Returns a map with that given subdomain.
    CookieMap* getSubdomainMap(const String&);
    void addSubdomainMap(const String&, CookieMap*);
    void deleteAllCookiesAndDomains();

    void getAllCookies(Vector<RefPtr<ParsedCookie> >*);
    void getAllChildCookies(Vector<RefPtr<ParsedCookie> >* stackOfCookies);
    PassRefPtr<ParsedCookie> removeOldestCookie();

private:
    void updateOldestCookie();
    PassRefPtr<ParsedCookie> removeCookieAtIndex(int position, const PassRefPtr<ParsedCookie>);

    Vector<RefPtr<ParsedCookie> > m_cookieVector;
    // The key is a subsection of the domain.
    // ex: if inserting accounts.google.com & this cookiemap is "com", this subdomain map will contain "google"
    // the "google" cookiemap will contain "accounts" in its subdomain map.
    HashMap<String, CookieMap*> m_subdomains;

    // Store the oldest cookie to speed up LRU checks.
    RefPtr<ParsedCookie> m_oldestCookie;
    const String m_name;

    // FIXME : should have a m_shouldUpdate flag to update the network layer only when the map has changed.
};

} // namespace WebCore

#endif // CookieMap_h
