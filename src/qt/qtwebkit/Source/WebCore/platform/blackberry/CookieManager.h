/*
 * Copyright (C) 2008, 2009 Julien Chaffraix <julien.chaffraix@gmail.com>
 * Copyright (C) 2010, 2012 Research In Motion Limited. All rights reserved.
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

#ifndef CookieManager_h
#define CookieManager_h

#include "CookieMap.h"
#include "ParsedCookie.h"
#include "Timer.h"
#include <BlackBerryPlatformGuardedPointer.h>
#include <wtf/HashMap.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class CookieDatabaseBackingStore;
class KURL;

enum BackingStoreRemovalPolicy {
    RemoveFromBackingStore,
    BackingStoreCookieEntry,
    DoNotRemoveFromBackingStore
};

enum CookieStorageAcceptPolicy {
    CookieStorageAcceptPolicyAlways,
    CookieStorageAcceptPolicyNever,
    CookieStorageAcceptPolicyOnlyFromMainDocumentDomain
};

/*
  * The CookieManager class is a singleton class that handles and selectively persists
  * incoming cookies. This class contains a tree of domains for quicker
  * cookie domain lookup. The top of the tree represents a null value for a null domain.
  * The null domain contains references to top level domains and each node below
  * represents a sub-section of a domain, delimited by "."
  *
  * If a cookie has a domain "a.b.com", it will be stored in the node named "a" in this tree.
  * in the branch ""->"com"->"b"->"a"
  *
  * Cookie specs follow the RFC 6265 spec sheet.
  * http://tools.ietf.org/html/rfc6265
  */

class CookieManager: public BlackBerry::Platform::GuardedPointerBase {
public:
    bool canLocalAccessAllCookies() const { return m_shouldDumpAllCookies; }
    void setCanLocalAccessAllCookies(bool enabled) { m_shouldDumpAllCookies = enabled; }

    void setCookies(const KURL&, const String& value, CookieFilter = WithHttpOnlyCookies);
    void setCookies(const KURL&, const Vector<String>& cookies, CookieFilter);

    void removeAllCookies(BackingStoreRemovalPolicy);
    void removeCookieWithName(const KURL&, const String& cookieName);

    unsigned short cookiesCount() const { return m_count; }

    void setCookieJar(const char*);
    const String& cookieJar() const { return m_cookieJarFileName; }

    // Count update method
    void removedCookie()
    {
        ASSERT(m_count > 0);
        --m_count;
    }
    void addedCookie() { ++m_count; }

    static unsigned maxCookieLength() { return s_maxCookieLength; }

    void setCookiePolicy(CookieStorageAcceptPolicy policy) { m_policy = policy; }
    CookieStorageAcceptPolicy cookiePolicy() const { return m_policy; }
    void setPrivateMode(bool);

    String generateHtmlFragmentForCookies();
    String getCookie(const KURL& requestURL, CookieFilter) const;

    // Returns all cookies that are associated with the specified URL as raw cookies.
    void getRawCookies(Vector<RefPtr<ParsedCookie> >& stackOfCookies, const KURL& requestURL, CookieFilter = WithHttpOnlyCookies) const;

private:
    friend CookieManager& cookieManager();
    friend class CookieDatabaseBackingStore;

    CookieManager();
    virtual ~CookieManager();

    void checkAndTreatCookie(PassRefPtr<ParsedCookie> prpCandidateCookie, BackingStoreRemovalPolicy, CookieFilter = WithHttpOnlyCookies);

    void addCookieToMap(CookieMap* targetMap, PassRefPtr<ParsedCookie> prpCandidateCookie, BackingStoreRemovalPolicy postToBackingStore, CookieFilter = WithHttpOnlyCookies);

    CookieMap* findOrCreateCookieMap(CookieMap* protocolMap, const PassRefPtr<ParsedCookie> candidateCookie);

    void initiateCookieLimitCleanUp();
    void cookieLimitCleanUp(Timer<CookieManager>*);

    HashMap<String, CookieMap*> m_managerMap;

    unsigned short m_count;

    bool m_privateMode;
    bool m_shouldDumpAllCookies;
    bool m_syncedWithDatabase;

    String m_cookieJarFileName;

    // FIXME: This method should be removed.
    void getBackingStoreCookies();

    // Cookie size limit of 4kB as advised per RFC2109
    static const unsigned s_maxCookieLength = 4096;

    CookieStorageAcceptPolicy m_policy;

    CookieDatabaseBackingStore* m_cookieBackingStore;
    Timer<CookieManager> m_limitTimer;

    DISABLE_COPY(CookieManager)
};

// Get the global instance.
CookieManager& cookieManager();

} // namespace WebCore

#endif // CookieManager_h
