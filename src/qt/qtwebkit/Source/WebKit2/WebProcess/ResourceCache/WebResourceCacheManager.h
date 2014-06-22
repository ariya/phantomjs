/*
 * Copyright (C) 2011, 2013 Apple Inc. All rights reserved.
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

#ifndef WebResourceCacheManager_h
#define WebResourceCacheManager_h

#include "MessageReceiver.h"
#include "ResourceCachesToClear.h"
#include "WebProcessSupplement.h"
#include <WebCore/MemoryCache.h>
#include <wtf/Noncopyable.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

class WebProcess;
struct SecurityOriginData;

class WebResourceCacheManager : public WebProcessSupplement, public CoreIPC::MessageReceiver {
    WTF_MAKE_NONCOPYABLE(WebResourceCacheManager);
public:
    WebResourceCacheManager(WebProcess*);

    static const char* supplementName();

private:
    // CoreIPC::MessageReceiver
    // Implemented in generated WebResourceCacheManagerMessageReceiver.cpp
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;

    void getCacheOrigins(uint64_t callbackID) const;
    void returnCacheOrigins(uint64_t callbackID, const WebCore::MemoryCache::SecurityOriginSet&) const;
    void clearCacheForOrigin(const SecurityOriginData&, uint32_t cachesToClear) const;
    void clearCacheForAllOrigins(uint32_t cachesToClear) const;

#if USE(CFURLCACHE)
    static RetainPtr<CFArrayRef> cfURLCacheHostNames();
#if ENABLE(CACHE_PARTITIONING)
    typedef void (^CacheCallback)(RetainPtr<CFArrayRef>);
    static void cfURLCacheHostNamesWithCallback(CacheCallback);
#endif
    static void clearCFURLCacheForHostNames(CFArrayRef);
#endif

    WebProcess* m_process;
};

} // namespace WebKit

#endif // WebResourceCacheManager_h
