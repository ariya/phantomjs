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
#include "CookieStorageShim.h"

#if ENABLE(NETWORK_PROCESS)

#include "CookieStorageShimLibrary.h"
#include "NetworkConnectionToWebProcess.h"
#include "NetworkProcessConnection.h"
#include "WebCoreArgumentCoders.h"
#include "WebProcess.h"
#include <WebCore/KURL.h>
#include <dlfcn.h>
#include <wtf/MainThread.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;

namespace WebKit {

static CFDictionaryRef webKitCookieStorageCopyRequestHeaderFieldsForURL(CFHTTPCookieStorageRef inCookieStorage, CFURLRef inRequestURL)
{
    String cookies;
    KURL firstPartyForCookiesURL;
    if (!WebProcess::shared().networkConnection()->connection()->sendSync(Messages::NetworkConnectionToWebProcess::CookieRequestHeaderFieldValue(false, firstPartyForCookiesURL, inRequestURL), Messages::NetworkConnectionToWebProcess::CookiesForDOM::Reply(cookies), 0))
        return 0;

    RetainPtr<CFStringRef> cfCookies = cookies.createCFString();
    static const void* cookieKeys[] = { CFSTR("Cookie") };
    const void* cookieValues[] = { cfCookies.get() };
    return CFDictionaryCreate(0, cookieKeys, cookieValues, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
}

CookieStorageShim& CookieStorageShim::shared()
{
    DEFINE_STATIC_LOCAL(CookieStorageShim, storage, ());
    return storage;
}

void CookieStorageShim::initialize()
{
    CookieStorageShimCallbacks callbacks = { &webKitCookieStorageCopyRequestHeaderFieldsForURL };
    CookieStorageShimInitializeFunc func = reinterpret_cast<CookieStorageShimInitializeFunc>(dlsym(RTLD_DEFAULT, "WebKitCookieStorageShimInitialize"));
    if (func)
        func(callbacks);
}

}

#endif // ENABLE(NETWORK_PROCESS)
