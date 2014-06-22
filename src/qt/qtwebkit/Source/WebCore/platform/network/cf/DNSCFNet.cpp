/*
 * Copyright (C) 2008 Collin Jackson  <collinj@webkit.org>
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
 * Copyright (C) 2012 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
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
#include "DNS.h"
#include "DNSResolveQueue.h"

#include "KURL.h"
#include "Timer.h"
#include <wtf/HashSet.h>
#include <wtf/MainThread.h>
#include <wtf/RetainPtr.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringHash.h>

#if PLATFORM(WIN)
#include "LoaderRunLoopCF.h"
#include <CFNetwork/CFNetwork.h>
#endif

namespace WebCore {

bool DNSResolveQueue::platformProxyIsEnabledInSystemPreferences()
{
    // Don't do DNS prefetch if proxies are involved. For many proxy types, the user agent is never exposed
    // to the IP address during normal operation. Querying an internal DNS server may not help performance,
    // as it doesn't necessarily look up the actual external IP. Also, if DNS returns a fake internal address,
    // local caches may keep it even after re-connecting to another network.

    RetainPtr<CFDictionaryRef> proxySettings = adoptCF(CFNetworkCopySystemProxySettings());
    if (!proxySettings)
        return false;

    RetainPtr<CFURLRef> httpCFURL = KURL(ParsedURLString, "http://example.com/").createCFURL();
    RetainPtr<CFURLRef> httpsCFURL = KURL(ParsedURLString, "https://example.com/").createCFURL();

    RetainPtr<CFArrayRef> httpProxyArray = adoptCF(CFNetworkCopyProxiesForURL(httpCFURL.get(), proxySettings.get()));
    RetainPtr<CFArrayRef> httpsProxyArray = adoptCF(CFNetworkCopyProxiesForURL(httpsCFURL.get(), proxySettings.get()));

    CFIndex httpProxyCount = CFArrayGetCount(httpProxyArray.get());
    CFIndex httpsProxyCount = CFArrayGetCount(httpsProxyArray.get());
    if (httpProxyCount == 1 && CFEqual(CFDictionaryGetValue(static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(httpProxyArray.get(), 0)), kCFProxyTypeKey), kCFProxyTypeNone))
        httpProxyCount = 0;
    if (httpsProxyCount == 1 && CFEqual(CFDictionaryGetValue(static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(httpsProxyArray.get(), 0)), kCFProxyTypeKey), kCFProxyTypeNone))
        httpsProxyCount = 0;

    return httpProxyCount || httpsProxyCount;
}

static void clientCallback(CFHostRef theHost, CFHostInfoType, const CFStreamError*, void*)
{
    DNSResolveQueue::shared().decrementRequestCount(); // It's ok to call shared() from a secondary thread, the static variable has already been initialized by now.
    CFRelease(theHost);
}

void DNSResolveQueue::platformResolve(const String& hostname)
{
    ASSERT(isMainThread());

    RetainPtr<CFHostRef> host = adoptCF(CFHostCreateWithName(0, hostname.createCFString().get()));
    if (!host) {
        decrementRequestCount();
        return;
    }

    CFHostClientContext context = { 0, 0, 0, 0, 0 };
    CFHostRef leakedHost = host.leakRef(); // The host will be released from clientCallback().
    Boolean result = CFHostSetClient(leakedHost, clientCallback, &context);
    ASSERT_UNUSED(result, result);
#if !PLATFORM(WIN)
    CFHostScheduleWithRunLoop(leakedHost, CFRunLoopGetMain(), kCFRunLoopCommonModes);
#else
    // On Windows, we run a separate thread with CFRunLoop, which is where clientCallback will be called.
    CFHostScheduleWithRunLoop(leakedHost, loaderRunLoop(), kCFRunLoopDefaultMode);
#endif
    CFHostStartInfoResolution(leakedHost, kCFHostAddresses, 0);
}

void prefetchDNS(const String& hostname)
{
    ASSERT(isMainThread());
    if (hostname.isEmpty())
        return;
    DNSResolveQueue::shared().add(hostname);
}

}
