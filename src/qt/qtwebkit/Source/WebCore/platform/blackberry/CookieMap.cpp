/*
 * Copyright (C) 2008, 2009 Julien Chaffraix <julien.chaffraix@gmail.com>
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

#define ENABLE_COOKIE_DEBUG 0

#include "config.h"
#include "CookieMap.h"

#include "CookieManager.h"
#include "Logging.h"
#include "ParsedCookie.h"
#include <wtf/text/CString.h>

#if ENABLE_COOKIE_DEBUG
#include <BlackBerryPlatformLog.h>
#define CookieLog(format, ...) BlackBerry::Platform::logAlways(BlackBerry::Platform::LogLevelInfo, format, ## __VA_ARGS__)
#else
#define CookieLog(format, ...)
#endif // ENABLE_COOKIE_DEBUG

namespace WebCore {

CookieMap::CookieMap(const String& name)
    : m_oldestCookie(0)
    , m_name(name)
{
}

CookieMap::~CookieMap()
{
    deleteAllCookiesAndDomains();
}

bool CookieMap::addOrReplaceCookie(PassRefPtr<ParsedCookie> prpCandidateCookie, RefPtr<ParsedCookie>& replacedCookie, CookieFilter filter)
{
    RefPtr<ParsedCookie> candidateCookie = prpCandidateCookie;
    CookieLog("CookieMap - Attempting to add cookie - %s", cookie->name().utf8().data());

    size_t cookieCount = m_cookieVector.size();
    for (size_t i = 0; i < cookieCount; i++) {
        if (m_cookieVector[i]->name() == candidateCookie->name() && m_cookieVector[i]->path() == candidateCookie->path()) {

            if (filter == NoHttpOnlyCookie && m_cookieVector[i]->isHttpOnly())
                return false;

            replacedCookie = m_cookieVector[i];
            m_cookieVector[i] = candidateCookie;
            if (replacedCookie == m_oldestCookie)
                updateOldestCookie();
            return true;
        }
    }

    m_cookieVector.append(candidateCookie);
    if (!candidateCookie->isSession())
        cookieManager().addedCookie();
    if (!m_oldestCookie || m_oldestCookie->lastAccessed() > candidateCookie->lastAccessed())
        m_oldestCookie = candidateCookie;
    return true;
}

PassRefPtr<ParsedCookie> CookieMap::removeCookieAtIndex(int position, const PassRefPtr<ParsedCookie> cookie)
{
    ASSERT(0 <= position && static_cast<unsigned>(position) < m_cookieVector.size());
    RefPtr<ParsedCookie> prevCookie = m_cookieVector[position];
    m_cookieVector.remove(position);

    if (prevCookie == m_oldestCookie)
        updateOldestCookie();
    else if (prevCookie != cookie) {
        // The cookie we used to search is force expired, we must do the same
        // to the cookie in memory too.
        if (cookie->isForceExpired())
            prevCookie->forceExpire();
    }

    if (!prevCookie->isSession())
        cookieManager().removedCookie();
    return prevCookie;
}

PassRefPtr<ParsedCookie> CookieMap::removeCookie(const PassRefPtr<ParsedCookie> cookie, CookieFilter filter)
{
    size_t cookieCount = m_cookieVector.size();
    for (size_t position = 0; position < cookieCount; ++position) {
        if (m_cookieVector[position]->name() == cookie->name() && m_cookieVector[position]->path() == cookie->path()) {
            if (filter == NoHttpOnlyCookie && m_cookieVector[position]->isHttpOnly())
                return 0;
            return removeCookieAtIndex(position, cookie);
        }
    }
    return 0;
}

CookieMap* CookieMap::getSubdomainMap(const String& subdomain)
{
#if ENABLE_COOKIE_DEBUG
    if (!m_subdomains.contains(subdomain))
        CookieLog("CookieMap - %s does not exist in this map", subdomain.utf8().data());
#endif
    return m_subdomains.get(subdomain);
}

void CookieMap::addSubdomainMap(const String& subdomain, CookieMap* newDomain)
{
    CookieLog("CookieMap - Attempting to add subdomain - %s", subdomain.utf8().data());
    m_subdomains.add(subdomain, newDomain);
}

void CookieMap::getAllCookies(Vector<RefPtr<ParsedCookie> >* stackOfCookies)
{
    CookieLog("CookieMap - Attempting to copy Map:%s cookies with %d cookies into vectors", m_name.utf8().data(), m_cookieVector.size());

    stackOfCookies->reserveCapacity(stackOfCookies->size() + m_cookieVector.size());

    size_t position = 0;
    while (position < m_cookieVector.size()) {
        RefPtr<ParsedCookie> newCookie = m_cookieVector[position];
        if (newCookie->hasExpired()) {
            // Notice that we don't delete from backingstore. These expired cookies will be
            // deleted when manager loads the backingstore again.
            removeCookieAtIndex(position, newCookie);
        } else {
            stackOfCookies->append(newCookie);
            position++;
        }
    }

    CookieLog("CookieMap - stack of cookies now have %d cookies in it", (*stackOfCookies).size());
}

PassRefPtr<ParsedCookie> CookieMap::removeOldestCookie()
{
    // FIXME: Make sure it finds the GLOBAL oldest cookie, not the first oldestcookie it finds.
    RefPtr<ParsedCookie> oldestCookie = m_oldestCookie;

    // If this map has an oldestCookie, remove it. If not, do a DFS to search for a child that does
    if (!oldestCookie) {

        CookieLog("CookieMap - no oldestCookie exist");

        // Base case is if the map has no child and no cookies, we return a null.
        if (!m_subdomains.size()) {
            CookieLog("CookieMap - no subdomains, base case reached, return 0");
            return 0;
        }

        CookieLog("CookieMap - looking into subdomains");

        for (HashMap<String, CookieMap*>::iterator it = m_subdomains.begin(); it != m_subdomains.end(); ++it) {
            oldestCookie = it->value->removeOldestCookie();
            if (oldestCookie)
                break;
        }
    } else {
        CookieLog("CookieMap - oldestCookie exist.");
        oldestCookie = removeCookie(m_oldestCookie);
    }

    return oldestCookie;
}

void CookieMap::updateOldestCookie()
{
    size_t size = m_cookieVector.size();
    if (!size) {
        m_oldestCookie = 0;
        return;
    }

    m_oldestCookie = m_cookieVector[0];
    for (size_t i = 1; i < size; ++i) {
        if (m_oldestCookie->lastAccessed() > m_cookieVector[i]->lastAccessed())
            m_oldestCookie = m_cookieVector[i];
    }
}

void CookieMap::deleteAllCookiesAndDomains()
{
    deleteAllValues(m_subdomains);
    m_subdomains.clear();
    m_cookieVector.clear();

    m_oldestCookie = 0;
}

void CookieMap::getAllChildCookies(Vector<RefPtr<ParsedCookie> >* stackOfCookies)
{
    CookieLog("CookieMap - getAllChildCookies in Map - %s", getName().utf8().data());
    getAllCookies(stackOfCookies);
    for (HashMap<String, CookieMap*>::iterator it = m_subdomains.begin(); it != m_subdomains.end(); ++it)
        it->value->getAllChildCookies(stackOfCookies);
}

} // namespace WebCore
