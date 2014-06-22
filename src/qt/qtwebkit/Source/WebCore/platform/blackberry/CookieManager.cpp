/*
 * Copyright (C) 2008, 2009 Julien Chaffraix <julien.chaffraix@gmail.com>
 * Copyright (C) 2010, 2011, 2012, 2013 Research In Motion Limited. All rights reserved.
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
#define ENABLE_COOKIE_SUPER_VERBOSE_DEBUG 0
#define ENABLE_COOKIE_LIMIT_DEBUG 0

#include "config.h"
#include "CookieManager.h"

#include "CookieDatabaseBackingStore.h"
#include "CookieParser.h"
#include "FileSystem.h"
#include "Logging.h"
#include "WebSettings.h"
#include <BlackBerryPlatformExecutableMessage.h>
#include <BlackBerryPlatformMessageClient.h>
#include <BlackBerryPlatformNavigatorHandler.h>
#include <BlackBerryPlatformSettings.h>
#include <network/DomainTools.h>
#include <stdlib.h>
#include <wtf/CurrentTime.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

#if ENABLE_COOKIE_DEBUG
#include <BlackBerryPlatformLog.h>
#endif

#if ENABLE_COOKIE_SUPER_VERBOSE_DEBUG
#define CookieLog(format, ...) BlackBerry::Platform::logAlways(BlackBerry::Platform::LogLevelInfo, format, ## __VA_ARGS__)
#else
#define CookieLog(format, ...)
#endif // ENABLE_COOKIE_SUPER_VERBOSE_DEBUG

#if ENABLE_COOKIE_LIMIT_DEBUG
#define CookieLimitLog(format, ...) BlackBerry::Platform::logAlways(BlackBerry::Platform::LogLevelInfo, format, ## __VA_ARGS__)
#else
#define CookieLimitLog(format, ...)
#endif // ENABLE_COOKIE_LIMIT_DEBUG

namespace WebCore {

// Max count constants.
static const unsigned s_globalMaxCookieCount = 6000;
static const unsigned s_maxCookieCountPerHost = 60;
static const unsigned s_cookiesToDeleteWhenLimitReached = 60;
static const unsigned s_delayToStartCookieCleanup = 10;

CookieManager& cookieManager()
{
    static CookieManager *cookieManager = 0;
    if (!cookieManager) {
        // Open the cookieJar now and get the backing store cookies to fill the manager.
        cookieManager = new CookieManager;
        cookieManager->m_cookieBackingStore->open(cookieManager->cookieJar());
    }
    return *cookieManager;
}

CookieManager::CookieManager()
    : m_count(0)
    , m_privateMode(false)
    , m_shouldDumpAllCookies(false)
    , m_syncedWithDatabase(false)
    , m_cookieJarFileName(pathByAppendingComponent(BlackBerry::Platform::Settings::instance()->applicationDataDirectory().c_str(), "/cookieCollection.db"))
    , m_policy(CookieStorageAcceptPolicyAlways)
    , m_cookieBackingStore(CookieDatabaseBackingStore::create())
    , m_limitTimer(this, &CookieManager::cookieLimitCleanUp)
{
}

CookieManager::~CookieManager()
{
    removeAllCookies(DoNotRemoveFromBackingStore);
    // FIXME: m_managerMap and the top layer protocolMaps are not properly deleted.
    // Do not delete any protocol maps to avoid double-deletion of the maps that are
    // being used for both secure and non-secure protocols; this leak is OK since
    // there's nothing important in the hashtable destructors, and the memory will be reclaimed on exit

    // FIXME: CookieDatabaseBackingStore is not deleted, only flushed
    // (currently the destructor is never called since this class is a
    // singleton; on exit, the db is flushed manually. This call is only here
    // as a fallback in case this class is made a non-singleton.
    m_cookieBackingStore->sendChangesToDatabaseSynchronously();
}

// Sorting logic is based on Cookie Spec RFC6265, section 5.4.2
static bool cookieSorter(PassRefPtr<ParsedCookie> a, PassRefPtr<ParsedCookie> b)
{
    if (a->path().length() == b->path().length())
        return a->creationTime() < b->creationTime();
    return a->path().length() > b->path().length();
}

// Return whether we should ignore the scheme
static bool shouldIgnoreScheme(const String& protocol)
{
    // We want to ignore file and local schemes
    return protocol == "file" || protocol == "local";
}

void CookieManager::setCookies(const KURL& url, const String& value, CookieFilter filter)
{
    // If the database hasn't been sync-ed at this point, force a sync load
    if (!m_syncedWithDatabase && !m_privateMode)
        m_cookieBackingStore->openAndLoadDatabaseSynchronously(cookieJar());

    CookieLog("CookieManager - Setting cookies");
    CookieParser parser(url);
    Vector<RefPtr<ParsedCookie> > cookies = parser.parse(value);

    for (size_t i = 0; i < cookies.size(); ++i) {
        BackingStoreRemovalPolicy treatment = m_privateMode ? DoNotRemoveFromBackingStore : RemoveFromBackingStore;
        checkAndTreatCookie(cookies[i], treatment, filter);
    }
}

void CookieManager::setCookies(const KURL& url, const Vector<String>& cookies, CookieFilter filter)
{
    // If the database hasn't been sync-ed at this point, force a sync load
    if (!m_syncedWithDatabase && !m_privateMode)
        m_cookieBackingStore->openAndLoadDatabaseSynchronously(cookieJar());

    CookieLog("CookieManager - Setting cookies");
    CookieParser parser(url);
    for (size_t i = 0; i < cookies.size(); ++i) {
        BackingStoreRemovalPolicy treatment = m_privateMode ? DoNotRemoveFromBackingStore : RemoveFromBackingStore;
        if (RefPtr<ParsedCookie> parsedCookie = parser.parseOneCookie(cookies[i]))
            checkAndTreatCookie(parsedCookie, treatment, filter);
    }
}

String CookieManager::getCookie(const KURL& url, CookieFilter filter) const
{
    // If the database hasn't been sync-ed at this point, force a sync load
    if (!m_syncedWithDatabase && !m_privateMode)
        m_cookieBackingStore->openAndLoadDatabaseSynchronously(cookieJar());

    Vector<RefPtr<ParsedCookie> > rawCookies;
    rawCookies.reserveInitialCapacity(s_maxCookieCountPerHost);

    // Retrieve cookies related to this url
    getRawCookies(rawCookies, url, filter);

    CookieLog("CookieManager - there are %d cookies in raw cookies\n", rawCookies.size());

    // Generate the cookie header string using the retrieved cookies
    StringBuilder cookieStringBuilder;
    cookieStringBuilder.reserveCapacity(512);
    size_t cookieSize = rawCookies.size();
    for (size_t i = 0; i < cookieSize; i++) {
        cookieStringBuilder.append(rawCookies[i]->toNameValuePair());
        if (i != cookieSize-1)
            cookieStringBuilder.append("; ");
    }

    CookieLog("CookieManager - cookieString is - %s\n", cookieStringBuilder.toString().utf8().data());

    return cookieStringBuilder.toString();
}

String CookieManager::generateHtmlFragmentForCookies()
{
    // If the database hasn't been sync-ed at this point, force a sync load
    if (!m_syncedWithDatabase && !m_privateMode)
        m_cookieBackingStore->openAndLoadDatabaseSynchronously(cookieJar());

    CookieLog("CookieManager - generateHtmlFragmentForCookies\n");

    Vector<RefPtr<ParsedCookie> > cookieCandidates;
    for (HashMap<String, CookieMap*>::iterator it = m_managerMap.begin(); it != m_managerMap.end(); ++it)
        it->value->getAllChildCookies(&cookieCandidates);

    String result;
    RefPtr<ParsedCookie> cookie = 0;
    result.append(String("<table style=\"word-wrap:break-word\" cellSpacing=\"0\" cellPadding=\"0\" border=\"1\"><tr><th>Domain</th><th>Path</th><th>Protocol</th><th>Name</th><th>Value</th><th>Secure</th><th>HttpOnly</th><th>Session</th></tr>"));
    for (size_t i = 0; i < cookieCandidates.size(); ++i) {
        cookie = cookieCandidates[i];
        result.append(String("<tr><td align=\"center\">"));
        result.append(cookie->domain());
        result.append(String("<td align=\"center\">"));
        result.append(cookie->path());
        result.append(String("<td align=\"center\">"));
        result.append(cookie->protocol());
        result.append(String("<td align=\"center\">"));
        result.append(cookie->name());
        result.append(String("<td align=\"center\" style= \"word-break:break-all\">"));
        result.append(cookie->value());
        result.append(String("<td align=\"center\">"));
        result.append(String(cookie->isSecure() ? "Yes" : "No"));
        result.append(String("<td align=\"center\">"));
        result.append(String(cookie->isHttpOnly() ? "Yes" : "No"));
        result.append(String("<td align=\"center\">"));
        result.append(String(cookie->isSession() ? "Yes" : "No"));
        result.append(String("</td></tr>"));
    }
    result.append(String("</table>"));
    return result;
}

void CookieManager::getRawCookies(Vector<RefPtr<ParsedCookie> > &stackOfCookies, const KURL& requestURL, CookieFilter filter) const
{
    // Force a sync load of the database
    if (!m_syncedWithDatabase && !m_privateMode)
        m_cookieBackingStore->openAndLoadDatabaseSynchronously(cookieJar());

    CookieLog("CookieManager - getRawCookies - processing url with domain - %s & protocol: %s & path: %s\n", requestURL.host().utf8().data(), requestURL.protocol().utf8().data(), requestURL.path().utf8().data());

    const bool invalidScheme = shouldIgnoreScheme(requestURL.protocol());
    const bool specialCaseForWebWorks = invalidScheme && m_shouldDumpAllCookies;
    const bool isConnectionSecure = requestURL.protocolIs("https") || requestURL.protocolIs("wss") || specialCaseForWebWorks;

    Vector<RefPtr<ParsedCookie> > cookieCandidates;
    Vector<CookieMap*> protocolsToSearch;

    // Special Case: If a server sets a "secure" cookie over a non-secure channel and tries to access the cookie
    // over a secure channel, it will not succeed because the secure protocol isn't mapped to the insecure protocol yet.
    // Set the map to the non-secure version, so it'll search the mapping for a secure cookie.
    CookieMap* targetMap = m_managerMap.get(requestURL.protocol());
    if (!targetMap && isConnectionSecure) {
        CookieLog("CookieManager - special case: secure protocol are not linked yet.");
        if (requestURL.protocolIs("https"))
            targetMap = m_managerMap.get("http");
        else if (requestURL.protocolIs("wss"))
            targetMap = m_managerMap.get("ws");
    }

    // Decide which scheme tree we should look at.
    // Return on invalid schemes. cookies are currently disabled on file and local.
    // We only want to enable them for WebWorks that enabled a special flag.
    if (specialCaseForWebWorks)
        copyValuesToVector(m_managerMap, protocolsToSearch);
    else if (invalidScheme)
        return;
    else {
        protocolsToSearch.append(targetMap);
        // FIXME: this is a hack for webworks apps; RFC 6265 says "Cookies do not provide isolation by scheme"
        // so we should not be checking protocols at all. See PR 135595
        if (m_shouldDumpAllCookies) {
            protocolsToSearch.append(m_managerMap.get("file"));
            protocolsToSearch.append(m_managerMap.get("local"));
        }
    }

    Vector<String> delimitedHost;

    // IP addresses are stored in a particular format (due to ipv6). Reduce the ip address so we can match
    // it with the one in memory.
    BlackBerry::Platform::String canonicalIP = BlackBerry::Platform::getCanonicalIPFormat(requestURL.host());
    if (!canonicalIP.empty())
        delimitedHost.append(String(canonicalIP.c_str()));
    else
        requestURL.host().lower().split(".", true, delimitedHost);

    // Go through all the protocol trees that we need to search for
    // and get all cookies that are valid for this domain
    for (size_t k = 0; k < protocolsToSearch.size(); k++) {
        CookieMap* currentMap = protocolsToSearch[k];

        // if no cookies exist for this protocol, break right away
        if (!currentMap)
            continue;

        CookieLog("CookieManager - looking at protocol map %s \n", currentMap->getName().utf8().data());

        // Special case for local and files - because WebApps expect to get ALL cookies from the backing-store on local protocol
        if (specialCaseForWebWorks) {
            CookieLog("CookieManager - special case find in protocol map - %s\n", currentMap->getName().utf8().data());
            currentMap->getAllChildCookies(&cookieCandidates);
        } else {
            // Get cookies from the null domain map
            currentMap->getAllCookies(&cookieCandidates);

            // Get cookies from Host-only cookies
            if (canonicalIP.empty()) {
                CookieLog("CookieManager - looking for host-only cookies for host - %s", requestURL.host().utf8().data());
                CookieMap* hostMap = currentMap->getSubdomainMap(requestURL.host());
                if (hostMap)
                    hostMap->getAllCookies(&cookieCandidates);
            }

            // Get cookies from the valid domain maps
            int i = delimitedHost.size() - 1;
            while (i >= 0) {
                CookieLog("CookieManager - finding %s in currentmap\n", delimitedHost[i].utf8().data());
                currentMap = currentMap->getSubdomainMap(delimitedHost[i]);
                // if this subdomain/domain does not exist in our mapping then we simply exit
                if (!currentMap) {
                    CookieLog("CookieManager - cannot find next map exiting the while loop.\n");
                    break;
                }
                CookieLog("CookieManager - found the map, grabbing cookies from this map\n");
                currentMap->getAllCookies(&cookieCandidates);
                i--;
            }
        }
    }

    CookieLog("CookieManager - there are %d cookies in candidate\n", cookieCandidates.size());

    for (size_t i = 0; i < cookieCandidates.size(); ++i) {
        RefPtr<ParsedCookie> cookie = cookieCandidates[i];

        // According to the path-matches rules in RFC6265, section 5.1.4,
        // we should add a '/' at the end of cookie-path for comparison if the cookie-path is not end with '/'.
        String path = cookie->path();
        CookieLog("CookieManager - comparing cookie path %s (len %d) to request path %s (len %d)", path.utf8().data(), path.length(), requestURL.path().utf8().data(), path.length());
        if (!equalIgnoringCase(path, requestURL.path()) && !path.endsWith("/", false))
            path = path + "/";

        // Only secure connections have access to secure cookies. Unless specialCaseForWebWorks is true.
        // Get the cookies filtering out HttpOnly cookies if requested.
        if (requestURL.path().startsWith(path, false) && (isConnectionSecure || !cookie->isSecure()) && (filter == WithHttpOnlyCookies || !cookie->isHttpOnly())) {
            CookieLog("CookieManager - cookie chosen - %s\n", cookie->toString().utf8().data());
            cookie->setLastAccessed(currentTime());
            stackOfCookies.append(cookie);
        }
    }

    std::stable_sort(stackOfCookies.begin(), stackOfCookies.end(), cookieSorter);
}

void CookieManager::removeAllCookies(BackingStoreRemovalPolicy backingStoreRemoval)
{
    HashMap<String, CookieMap*>::iterator first = m_managerMap.begin();
    HashMap<String, CookieMap*>::iterator end = m_managerMap.end();
    for (HashMap<String, CookieMap*>::iterator it = first; it != end; ++it)
        it->value->deleteAllCookiesAndDomains();

    if (backingStoreRemoval == RemoveFromBackingStore)
        m_cookieBackingStore->removeAll();
    m_count = 0;
}

void CookieManager::setCookieJar(const char* fileName)
{
    m_cookieJarFileName = String(fileName);
    m_cookieBackingStore->open(m_cookieJarFileName);
}

void CookieManager::checkAndTreatCookie(PassRefPtr<ParsedCookie> prpCandidateCookie, BackingStoreRemovalPolicy postToBackingStore, CookieFilter filter)
{
    RefPtr<ParsedCookie> candidateCookie = prpCandidateCookie;
    CookieLog("CookieManager - checkAndTreatCookie - processing url with domain - %s & protocol %s\n", candidateCookie->domain().utf8().data(), candidateCookie->protocol().utf8().data());

    // Delete invalid cookies:
    // 1) A cookie which is not from http shouldn't have a httpOnly property.
    // 2) Cookies coming from schemes that we do not support and the special flag isn't on
    if ((filter == NoHttpOnlyCookie && candidateCookie->isHttpOnly()) || (shouldIgnoreScheme(candidateCookie->protocol()) && !m_shouldDumpAllCookies))
        return;

    const bool ignoreDomain = (candidateCookie->protocol() == "file" || candidateCookie->protocol() == "local");

    // Determine which protocol tree to add the cookie to. Create one if necessary.
    CookieMap* curMap = 0;
    if (m_managerMap.contains(candidateCookie->protocol()))
        curMap = m_managerMap.get(candidateCookie->protocol());
    else {
        // Check if it is a secure version, if it is, link it to the non-secure version
        // Link curMap to the new protocol as well as the old one if it doesn't exist
        if (candidateCookie->protocol() == "https") {
            curMap = m_managerMap.get("http");
            if (!curMap) {
                curMap = new CookieMap("http");
                m_managerMap.add("http", curMap);
            }
        } else if (candidateCookie->protocol() == "wss") {
            curMap = m_managerMap.get("ws");
            if (!curMap) {
                curMap = new CookieMap("ws");
                m_managerMap.add("ws", curMap);
            }
        } else
            curMap = new CookieMap(candidateCookie->protocol());

        CookieLog("CookieManager - adding protocol cookiemap - %s\n", curMap->getName().utf8().data());

        m_managerMap.add(candidateCookie->protocol(), curMap);
    }

    // If protocol support domain, we have to traverse the domain tree to find the right
    // cookieMap to handle with
    if (!ignoreDomain)
        curMap = findOrCreateCookieMap(curMap, candidateCookie);

    // Now that we have the proper map for this cookie, we can modify it
    // If cookie does not exist and has expired, delete it
    // If cookie exists and it has expired, so we must remove it from the map, if not update it
    // If cookie expired and came from the BackingStore (therefore does not exist), we have to remove from database
    // If cookie does not exist & it's valid, add it to the current map

    if (candidateCookie->hasExpired() || candidateCookie->isForceExpired()) {
        // Special case for getBackingStoreCookies() to catch expired cookies
        if (postToBackingStore == BackingStoreCookieEntry)
            m_cookieBackingStore->remove(candidateCookie);
        else if (curMap) {
            // RemoveCookie will return 0 if the cookie doesn't exist.
            RefPtr<ParsedCookie> expired = curMap->removeCookie(candidateCookie, filter);
            // Cookie is useless, Remove the cookie from the backingstore if it exists.
            // Backup check for BackingStoreCookieEntry incase someone incorrectly uses this enum.
            if (expired && postToBackingStore != BackingStoreCookieEntry && !expired->isSession()) {
                CookieLog("CookieManager - expired cookie is nonsession, deleting from db");
                m_cookieBackingStore->remove(expired);
            }
        }
    } else {
        ASSERT(curMap);
        CookieLog("CookieManager - adding cookiemap - %s\n", curMap->getName().utf8().data());
        addCookieToMap(curMap, candidateCookie, postToBackingStore, filter);
    }
}

void CookieManager::addCookieToMap(CookieMap* targetMap, PassRefPtr<ParsedCookie> prpCandidateCookie, BackingStoreRemovalPolicy postToBackingStore, CookieFilter filter)
{
    RefPtr<ParsedCookie> replacedCookie = 0;
    RefPtr<ParsedCookie> candidateCookie = prpCandidateCookie;

    if (!targetMap->addOrReplaceCookie(candidateCookie, replacedCookie, filter)) {
        CookieLog("CookieManager - rejecting new cookie - %s.\n", candidateCookie->toString().utf8().data());
        return;
    }

    if (replacedCookie) {
        CookieLog("CookieManager - updating new cookie - %s.\n", candidateCookie->toString().utf8().data());

        // A cookie was replaced in targetMap.
        // If old cookie is non-session and new one is, we have to delete it from backingstore
        // If new cookie is non-session and old one is, we have to add it to backingstore
        // If both sessions are non-session, then we update it in the backingstore
        bool newIsSession = candidateCookie->isSession();
        bool oldIsSession = replacedCookie->isSession();

        if (postToBackingStore == RemoveFromBackingStore) {
            if (!newIsSession && !oldIsSession)
                m_cookieBackingStore->update(candidateCookie);
            else if (newIsSession && !oldIsSession) {
                // Must manually decrease the counter because it was not counted when
                // the cookie was removed in cookieVector.
                removedCookie();
                m_cookieBackingStore->remove(replacedCookie);
            } else if (!newIsSession && oldIsSession) {
                // Must manually increase the counter because it was not counted when
                // the cookie was added in cookieVector.
                addedCookie();
                m_cookieBackingStore->insert(candidateCookie);
            }
        }
        return;
    }

    CookieLog("CookieManager - adding new cookie - %s.\n", candidateCookie->toString().utf8().data());

    RefPtr<ParsedCookie> oldestCookie = 0;
    // Check if we have not reached the per cookie domain limit.
    // If that is not true, we check if the global limit has been reached if backingstore mode is on
    // Two points:
    // 1) We only do a global check if backingstore mode is on because the global cookie limit only
    //    counts session cookies that are saved in the database. If the user goes over the limit
    //    when they are in private mode, we know that the total cookie limit will be under the limit
    //    once the user goes back to normal mode (memory deleted and reloaded from the database)
    // 2) We use else if for this statement because if we remove a cookie in the 1st statement
    //    then it means the global count will never exceed the limit

    CookieLimitLog("CookieManager - local count: %d  global count: %d", targetMap->count(), m_count);
    if (targetMap->count() > s_maxCookieCountPerHost) {
        CookieLog("CookieManager - deleting oldest cookie from this map due to domain count.\n");
        oldestCookie = targetMap->removeOldestCookie();
    } else if (m_count > s_globalMaxCookieCount && (postToBackingStore != DoNotRemoveFromBackingStore)) {
        CookieLimitLog("CookieManager - Global limit reached, initiate cookie limit clean up.");
        initiateCookieLimitCleanUp();
    }

    // Only add non session cookie to the backing store.
    if (postToBackingStore == RemoveFromBackingStore) {
        if (oldestCookie && !oldestCookie->isSession()) {
            CookieLog("CookieManager - oldestCookie exists, deleting it from backingstore and destructing.\n");
            m_cookieBackingStore->remove(oldestCookie);
        }
        if (!candidateCookie->isSession())
            m_cookieBackingStore->insert(candidateCookie);
    }
}

void CookieManager::getBackingStoreCookies()
{
    // Make sure private mode is off when the database thread calls this method
    if (m_privateMode)
        return;

    // If there exists cookies in memory, flush them out and we'll load everything from the database again
    if (m_count)
        removeAllCookies(DoNotRemoveFromBackingStore);

    Vector<RefPtr<ParsedCookie> > cookies;
    m_cookieBackingStore->getCookiesFromDatabase(cookies);
    CookieLog("CookieManager - Backingstore has %d cookies, loading them in memory now", cookies.size());
    for (size_t i = 0; i < cookies.size(); ++i) {
        RefPtr<ParsedCookie> newCookie = cookies[i];

        // The IP flag is not persisted in the database.
        if (BlackBerry::Platform::isIPAddress(newCookie->domain()))
            newCookie->setDomain(newCookie->domain(), true);

        checkAndTreatCookie(newCookie, BackingStoreCookieEntry);
    }
    CookieLog("CookieManager - Backingstore loading complete.");

    m_syncedWithDatabase = true;
}

void CookieManager::setPrivateMode(bool privateMode)
{
    if (m_privateMode == privateMode)
        return;

    m_privateMode = privateMode;

    // If we switched to private mode when the database cookies haven't loaded into memory yet
    // we can return because there's nothing in memory anyway.
    if (m_privateMode && !m_syncedWithDatabase)
        return;

    removeAllCookies(DoNotRemoveFromBackingStore);

    // If we are switching back to public mode, reload the database to memory.
    if (!m_privateMode)
        getBackingStoreCookies();
}

CookieMap* CookieManager::findOrCreateCookieMap(CookieMap* protocolMap, const PassRefPtr<ParsedCookie> candidateCookie)
{
    // Explode the domain with the '.' delimiter
    Vector<String> delimitedHost;

    // If the domain is an IP address or is a host-only domain, don't split it.
    if (candidateCookie->domainIsIPAddress() || candidateCookie->isHostOnly())
        delimitedHost.append(candidateCookie->domain());
    else
        candidateCookie->domain().split(".", delimitedHost);

    CookieMap* curMap = protocolMap;
    size_t hostSize = delimitedHost.size();

    CookieLog("CookieManager - looking at protocol map %s \n", protocolMap->getName().utf8().data());

    // Find & create necessary CookieMaps by traversing down the domain tree
    // Each CookieMap represent a subsection of the domain, delimited by "."
    int i = hostSize - 1;
    while (i >= 0) {
        CookieLog("CookieManager - finding %s in currentmap\n", delimitedHost[i].utf8().data());
        CookieMap* nextMap = curMap->getSubdomainMap(delimitedHost[i]);
        if (!nextMap) {
            CookieLog("CookieManager - cannot find map\n");
            if (candidateCookie->hasExpired())
                return 0;
            CookieLog("CookieManager - creating %s in currentmap %s\n", delimitedHost[i].utf8().data(), curMap->getName().utf8().data());
            nextMap = new CookieMap(delimitedHost[i]);
            CookieLog("CookieManager - adding subdomain to map\n");
            curMap->addSubdomainMap(delimitedHost[i], nextMap);
        }
        curMap = nextMap;
        i--;
    }
    return curMap;
}

void CookieManager::removeCookieWithName(const KURL& url, const String& cookieName)
{
    // Dispatch the message because the database cookies are not loaded in memory yet.
    if (!m_syncedWithDatabase && !m_privateMode) {
        typedef void (WebCore::CookieManager::*FunctionType)(const KURL&, const String&);
        BlackBerry::Platform::webKitThreadMessageClient()->dispatchMessage(
            BlackBerry::Platform::createMethodCallMessage<FunctionType, CookieManager, const KURL, const String>(
                &CookieManager::removeCookieWithName, this, url, cookieName));
        return;
    }

    // We get all cookies from all domains that domain matches the request domain
    // and delete any cookies with the specified name that path matches the request path
    Vector<RefPtr<ParsedCookie> > results;
    getRawCookies(results, url, WithHttpOnlyCookies);
    // Delete the cookies that path matches the request path
    for (size_t i = 0; i < results.size(); i++) {
        RefPtr<ParsedCookie> cookie = results[i];
        if (!equalIgnoringCase(cookie->name(), cookieName))
            continue;
        if (url.path().startsWith(cookie->path(), false)) {
            cookie->forceExpire();
            checkAndTreatCookie(cookie, RemoveFromBackingStore);
        }
    }
}

void CookieManager::initiateCookieLimitCleanUp()
{
    if (!m_limitTimer.isActive()) {
        CookieLog("CookieManager - Starting a timer for cookie cleanup");
        m_limitTimer.startOneShot(s_delayToStartCookieCleanup);
    } else {
#ifndef NDEBUG
        CookieLog("CookieManager - Cookie cleanup timer already running");
#endif
    }
}

void CookieManager::cookieLimitCleanUp(Timer<CookieManager>* timer)
{
    ASSERT_UNUSED(timer, timer == &m_limitTimer);

    CookieLimitLog("CookieManager - Starting cookie clean up");

    size_t numberOfCookiesOverLimit = (m_count > s_globalMaxCookieCount) ? m_count - s_globalMaxCookieCount : 0;
    size_t amountToDelete = s_cookiesToDeleteWhenLimitReached + numberOfCookiesOverLimit;

    CookieLimitLog("CookieManager - Excess: %d  Amount to Delete: %d", numberOfCookiesOverLimit, amountToDelete);

    // Call the database to delete 'amountToDelete' of cookies
    Vector<RefPtr<ParsedCookie> > cookiesToDelete;
    cookiesToDelete.reserveInitialCapacity(amountToDelete);

    CookieLimitLog("CookieManager - Calling database to clean up");
    m_cookieBackingStore->getCookiesFromDatabase(cookiesToDelete, amountToDelete);

    // Cookies are ordered in ASC order by lastAccessed
    for (size_t i = 0; i < amountToDelete; ++i) {
        // Expire them and call checkandtreat to delete them from memory and database
        RefPtr<ParsedCookie> newCookie = cookiesToDelete[i];
        CookieLimitLog("CookieManager - Expire cookie: %s and delete", newCookie->toString().utf8().data());
        newCookie->forceExpire();
        checkAndTreatCookie(newCookie, RemoveFromBackingStore);
    }

    CookieLimitLog("CookieManager - Cookie clean up complete.");
}

} // namespace WebCore
