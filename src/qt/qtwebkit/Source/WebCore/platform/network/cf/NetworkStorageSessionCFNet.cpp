/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "NetworkStorageSession.h"

#include <wtf/MainThread.h>
#include <wtf/PassOwnPtr.h>

#if PLATFORM(MAC)
#include "WebCoreSystemInterface.h"
#else
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#endif

namespace WebCore {

NetworkStorageSession::NetworkStorageSession(RetainPtr<CFURLStorageSessionRef> platformSession)
    : m_platformSession(platformSession)
    , m_isPrivate(false)
{
}

static OwnPtr<NetworkStorageSession>& defaultNetworkStorageSession()
{
    ASSERT(isMainThread());
    DEFINE_STATIC_LOCAL(OwnPtr<NetworkStorageSession>, session, ());
    return session;
}

void NetworkStorageSession::switchToNewTestingSession()
{
    // Set a private session for testing to avoid interfering with global cookies. This should be different from private browsing session.
    // FIXME: It looks like creating a new session with the same identifier may be just creating a reference to the same storage. See <rdar://problem/11571450> and <rdar://problem/12384380>.
#if PLATFORM(MAC)
    defaultNetworkStorageSession() = adoptPtr(new NetworkStorageSession(adoptCF(wkCreatePrivateStorageSession(CFSTR("Private WebKit Session")))));
#else
    defaultNetworkStorageSession() = adoptPtr(new NetworkStorageSession(adoptCF(wkCreatePrivateStorageSession(CFSTR("Private WebKit Session"), defaultNetworkStorageSession()->platformSession()))));
#endif
}

#if PLATFORM(WIN)
static RetainPtr<CFHTTPCookieStorageRef>& cookieStorageOverride()
{
    DEFINE_STATIC_LOCAL(RetainPtr<CFHTTPCookieStorageRef>, cookieStorage, ());
    return cookieStorage;
}

void overrideCookieStorage(CFHTTPCookieStorageRef cookieStorage)
{
    ASSERT(isMainThread());
    // FIXME: Why don't we retain it? The only caller is an API method that takes cookie storage as a raw argument.
    cookieStorageOverride() = adoptCF(cookieStorage);
}

CFHTTPCookieStorageRef overridenCookieStorage()
{
    return cookieStorageOverride().get();
}
#endif

NetworkStorageSession& NetworkStorageSession::defaultStorageSession()
{
    if (!defaultNetworkStorageSession())
        defaultNetworkStorageSession() = adoptPtr(new NetworkStorageSession(0));
    return *defaultNetworkStorageSession();
}

PassOwnPtr<NetworkStorageSession> NetworkStorageSession::createPrivateBrowsingSession(const String& identifierBase)
{
    RetainPtr<CFStringRef> cfIdentifier = String(identifierBase + ".PrivateBrowsing").createCFString();

#if PLATFORM(MAC)
    OwnPtr<NetworkStorageSession> session = adoptPtr(new NetworkStorageSession(adoptCF(wkCreatePrivateStorageSession(cfIdentifier.get()))));
#else
    OwnPtr<NetworkStorageSession> session = adoptPtr(new NetworkStorageSession(adoptCF(wkCreatePrivateStorageSession(cfIdentifier.get(), defaultNetworkStorageSession()->platformSession()))));
#endif
    session->m_isPrivate = true;

    return session.release();
}

RetainPtr<CFHTTPCookieStorageRef> NetworkStorageSession::cookieStorage() const
{
#if PLATFORM(WIN)
    if (RetainPtr<CFHTTPCookieStorageRef>& override = cookieStorageOverride())
        return override;
#endif

    if (m_platformSession)
        return adoptCF(wkCopyHTTPCookieStorage(m_platformSession.get()));

#if USE(CFNETWORK)
    return wkGetDefaultHTTPCookieStorage();
#else
    // When using NSURLConnection, we also use its shared cookie storage.
    return 0;
#endif
}

}
