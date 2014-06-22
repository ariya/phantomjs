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
#include "AuthenticationChallengeManager.h"

#include "Credential.h"
#include "KURL.h"
#include "PageClientBlackBerry.h"
#include "ProtectionSpace.h"

#include <BlackBerryPlatformAssert.h>
#include <BlackBerryPlatformLog.h>
#include <wtf/Assertions.h>
#include <wtf/HashMap.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>

namespace WebCore {

typedef HashMap<PageClientBlackBerry*, bool> PageVisibilityMap;

struct ChallengeInfo {
    ChallengeInfo(const KURL&, const ProtectionSpace&, const Credential&, AuthenticationChallengeClient*, PageClientBlackBerry*);

    KURL url;
    ProtectionSpace space;
    Credential credential;
    AuthenticationChallengeClient* authClient;
    PageClientBlackBerry* pageClient;
    bool blocked;
};

ChallengeInfo::ChallengeInfo(const KURL& aUrl, const ProtectionSpace& aSpace, const Credential& aCredential,
    AuthenticationChallengeClient* anAuthClient, PageClientBlackBerry* aPageClient)
    : url(aUrl)
    , space(aSpace)
    , credential(aCredential)
    , authClient(anAuthClient)
    , pageClient(aPageClient)
    , blocked(false)
{
}

class AuthenticationChallengeManagerPrivate {
public:
    AuthenticationChallengeManagerPrivate();

    bool resumeAuthenticationChallenge(PageClientBlackBerry*);
    void startAuthenticationChallenge(ChallengeInfo*);
    bool pageExists(PageClientBlackBerry*);

    ChallengeInfo* m_activeChallenge;
    PageVisibilityMap m_pageVisibilityMap;
    Vector<OwnPtr<ChallengeInfo> > m_challenges;
};

AuthenticationChallengeManagerPrivate::AuthenticationChallengeManagerPrivate()
    : m_activeChallenge(0)
{
}

bool AuthenticationChallengeManagerPrivate::resumeAuthenticationChallenge(PageClientBlackBerry* client)
{
    ASSERT(!m_activeChallenge);

    for (size_t i = 0; i < m_challenges.size(); ++i) {
        if (m_challenges[i]->pageClient == client && m_challenges[i]->blocked) {
            startAuthenticationChallenge(m_challenges[i].get());
            return true;
        }
    }

    return false;
}

void AuthenticationChallengeManagerPrivate::startAuthenticationChallenge(ChallengeInfo* info)
{
    m_activeChallenge = info;
    m_activeChallenge->blocked = false;
    m_activeChallenge->pageClient->authenticationChallenge(m_activeChallenge->url, m_activeChallenge->space, m_activeChallenge->credential);
}

bool AuthenticationChallengeManagerPrivate::pageExists(PageClientBlackBerry* client)
{
    return m_pageVisibilityMap.find(client) != m_pageVisibilityMap.end();
}

SINGLETON_INITIALIZER_THREADUNSAFE(AuthenticationChallengeManager)

AuthenticationChallengeManager::AuthenticationChallengeManager()
    : d(adoptPtr(new AuthenticationChallengeManagerPrivate))
{
}

void AuthenticationChallengeManager::pageCreated(PageClientBlackBerry* client)
{
    d->m_pageVisibilityMap.add(client, true);
}

void AuthenticationChallengeManager::pageDeleted(PageClientBlackBerry* client)
{
    d->m_pageVisibilityMap.remove(client);

    if (d->m_activeChallenge && d->m_activeChallenge->pageClient == client)
        d->m_activeChallenge = 0;

    Vector<OwnPtr<ChallengeInfo> > existing;
    d->m_challenges.swap(existing);

    for (size_t i = 0; i < existing.size(); ++i) {
        if (existing[i]->pageClient != client)
            d->m_challenges.append(existing[i].release());
    }
}

void AuthenticationChallengeManager::pageVisibilityChanged(PageClientBlackBerry* client, bool visible)
{
    PageVisibilityMap::iterator iter = d->m_pageVisibilityMap.find(client);

    ASSERT(iter != d->m_pageVisibilityMap.end());
    if (iter == d->m_pageVisibilityMap.end()) {
        d->m_pageVisibilityMap.add(client, visible);
        return;
    }

    if (iter->value == visible)
        return;

    iter->value = visible;
    if (!visible)
        return;

    if (d->m_activeChallenge)
        return;

    d->resumeAuthenticationChallenge(client);
}

void AuthenticationChallengeManager::authenticationChallenge(const KURL& url, const ProtectionSpace& space,
    const Credential& credential, AuthenticationChallengeClient* authClient, PageClientBlackBerry* pageClient)
{
    BLACKBERRY_ASSERT(authClient);
    BLACKBERRY_ASSERT(pageClient);

    ChallengeInfo* info = new ChallengeInfo(url, space, credential, authClient, pageClient);
    d->m_challenges.append(adoptPtr(info));

    if (d->m_activeChallenge || !pageClient->isVisible()) {
        info->blocked = true;
        return;
    }

    d->startAuthenticationChallenge(info);
}

void AuthenticationChallengeManager::cancelAuthenticationChallenge(AuthenticationChallengeClient* client)
{
    BLACKBERRY_ASSERT(client);

    if (d->m_activeChallenge && d->m_activeChallenge->authClient == client)
        d->m_activeChallenge = 0;

    Vector<OwnPtr<ChallengeInfo> > existing;
    d->m_challenges.swap(existing);

    ChallengeInfo* next = 0;
    PageClientBlackBerry* page = 0;

    for (size_t i = 0; i < existing.size(); ++i) {
        if (existing[i]->authClient != client) {
            if (page && !next && existing[i]->pageClient == page)
                next = existing[i].get();
            d->m_challenges.append(existing[i].release());
        } else if (d->m_activeChallenge == existing[i].get())
            page = existing[i]->pageClient;
    }

    if (next)
        d->startAuthenticationChallenge(next);
}

void AuthenticationChallengeManager::notifyChallengeResult(const KURL&, const ProtectionSpace& space,
    AuthenticationChallengeResult result, const Credential& credential)
{
    d->m_activeChallenge = 0;

    Vector<OwnPtr<ChallengeInfo> > existing;
    d->m_challenges.swap(existing);

    ChallengeInfo* next = 0;
    PageClientBlackBerry* page = 0;

    for (size_t i = 0; i < existing.size(); ++i) {
        if (existing[i]->space != space) {
            if (page && !next && existing[i]->pageClient == page)
                next = existing[i].get();
            d->m_challenges.append(existing[i].release());
        } else {
            page = existing[i]->pageClient;
            existing[i]->authClient->notifyChallengeResult(existing[i]->url, space, result, credential);

            // After calling notifyChallengeResult(), page could be destroyed or something.
            if (!d->pageExists(page) || !page->isVisible())
                page = 0;
        }
    }

    if (next)
        d->startAuthenticationChallenge(next);
}

} // namespace WebCore
