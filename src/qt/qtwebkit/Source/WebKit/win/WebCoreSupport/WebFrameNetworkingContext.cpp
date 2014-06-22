/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "WebFrameNetworkingContext.h"
#include "WebView.h"
#if USE(CFNETWORK)
#include <CFNetwork/CFHTTPCookiesPriv.h>
#endif
#include <WebCore/FrameLoader.h>
#include <WebCore/FrameLoaderClient.h>
#include <WebCore/NetworkStorageSession.h>
#include <WebCore/Page.h>
#include <WebCore/ResourceError.h>
#include <WebCore/Settings.h>
#if USE(CFNETWORK)
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#endif

using namespace WebCore;

static NetworkStorageSession* privateSession;
static String* identifierBase;

#if USE(CFNETWORK)
void WebFrameNetworkingContext::setCookieAcceptPolicyForAllContexts(WebKitCookieStorageAcceptPolicy policy)
{
    if (RetainPtr<CFHTTPCookieStorageRef> cookieStorage = NetworkStorageSession::defaultStorageSession().cookieStorage())
        CFHTTPCookieStorageSetCookieAcceptPolicy(cookieStorage.get(), policy);

    if (privateSession)
        CFHTTPCookieStorageSetCookieAcceptPolicy(privateSession->cookieStorage().get(), policy);
}
#endif

void WebFrameNetworkingContext::setPrivateBrowsingStorageSessionIdentifierBase(const String& base)
{
    ASSERT(isMainThread());

    delete identifierBase;

    identifierBase = new String(base);
}

void WebFrameNetworkingContext::ensurePrivateBrowsingSession()
{
#if USE(CFNETWORK)
    ASSERT(isMainThread());

    if (privateSession)
        return;

    String base;
    if (!identifierBase) {
        if (CFTypeRef bundleValue = CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(), kCFBundleIdentifierKey))
            if (CFGetTypeID(bundleValue) == CFStringGetTypeID())
                base = reinterpret_cast<CFStringRef>(bundleValue);
    }
    else
        base = *identifierBase;

    privateSession = NetworkStorageSession::createPrivateBrowsingSession(base).leakPtr();
#endif
}

void WebFrameNetworkingContext::destroyPrivateBrowsingSession()
{
    ASSERT(isMainThread());

    delete privateSession;
    privateSession = 0;
}

ResourceError WebFrameNetworkingContext::blockedError(const ResourceRequest& request) const
{
    return frame()->loader()->client()->blockedError(request);
}

String WebFrameNetworkingContext::referrer() const
{
    return frame()->loader()->referrer();
}

#if USE(CFNETWORK)
NetworkStorageSession& WebFrameNetworkingContext::storageSession() const
{
    ASSERT(isMainThread());

    if (frame() && frame()->settings() && frame()->settings()->privateBrowsingEnabled())
        return *privateSession;

    return NetworkStorageSession::defaultStorageSession();
}
#endif
