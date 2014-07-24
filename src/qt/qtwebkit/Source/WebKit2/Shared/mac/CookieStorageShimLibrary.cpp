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
#include "CookieStorageShimLibrary.h"

#if ENABLE(NETWORK_PROCESS)

#include <WebCore/DynamicLinkerInterposing.h>

extern "C" CFDictionaryRef _CFHTTPCookieStorageCopyRequestHeaderFieldsForURL(CFAllocatorRef inAllocator, CFHTTPCookieStorageRef inCookieStorage, CFURLRef inRequestURL);

namespace WebKit {

extern "C" void WebKitCookieStorageShimInitialize(const CookieStorageShimCallbacks&);

static CookieStorageShimCallbacks cookieStorageShimCallbacks;

class ShimProtector {
public:
    ShimProtector() { ++m_count; }
    ~ShimProtector() { --m_count; }
    static unsigned count() { return m_count; }
private:
    __thread static unsigned m_count;
};

__thread unsigned ShimProtector::m_count = 0;

static CFDictionaryRef shimCFHTTPCookieStorageCopyRequestHeaderFieldsForURL(CFAllocatorRef inAllocator, CFHTTPCookieStorageRef inCookieStorage, CFURLRef inRequestURL)
{
    ShimProtector protector;

    do {
        // Protect against uninitialized callbacks:
        if (!cookieStorageShimCallbacks.cookieStorageCopyRequestHeaderFieldsForURL)
            break;

        // Protect against accidental recursion:
        if (ShimProtector::count() > 1)
            break;

        CFDictionaryRef results = cookieStorageShimCallbacks.cookieStorageCopyRequestHeaderFieldsForURL(inCookieStorage, inRequestURL);
        if (!results)
            break;

        return results;
    } while (0);

    // If we've failed to retrieve the cookies manually, fall back to the original imposed function:
    return _CFHTTPCookieStorageCopyRequestHeaderFieldsForURL(inAllocator, inCookieStorage, inRequestURL);
}

DYLD_INTERPOSE(shimCFHTTPCookieStorageCopyRequestHeaderFieldsForURL, _CFHTTPCookieStorageCopyRequestHeaderFieldsForURL);

__attribute__((visibility("default")))
void WebKitCookieStorageShimInitialize(const CookieStorageShimCallbacks& callbacks)
{
    // Because the value of cookieStorageShimCallbacks will be read from mulitple threads,
    // only allow it to be initialized once.
    static int initialized = 0;
    if (!OSAtomicCompareAndSwapInt(0, 1, &initialized)) {
        return;
    }

    cookieStorageShimCallbacks = callbacks;
}
    
}

#endif // ENABLE(NETWORK_PROCESS)
