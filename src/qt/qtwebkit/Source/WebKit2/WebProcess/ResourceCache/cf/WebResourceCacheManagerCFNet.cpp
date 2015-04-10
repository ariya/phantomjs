/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "WebResourceCacheManager.h"

#if USE(CFURLCACHE)

#if ENABLE(CACHE_PARTITIONING)
#include <WebCore/PublicSuffix.h>
#include <wtf/text/CString.h>
#endif

#if PLATFORM(MAC)
#include "WebKitSystemInterface.h"
#endif


namespace WebKit {

#if ENABLE(CACHE_PARTITIONING)
static RetainPtr<CFStringRef> partitionName(CFStringRef domain)
{
#if ENABLE(PUBLIC_SUFFIX_LIST)
    String highLevel = WebCore::topPrivatelyControlledDomain(domain);
    if (highLevel.isNull())
        return 0;
    CString utf8String = highLevel.utf8();
    return adoptCF(CFStringCreateWithBytes(0, reinterpret_cast<const UInt8*>(utf8String.data()), utf8String.length(), kCFStringEncodingUTF8, false));
#else
    return domain;
#endif
}
#endif

RetainPtr<CFArrayRef> WebResourceCacheManager::cfURLCacheHostNames()
{
    return adoptCF(WKCFURLCacheCopyAllHostNamesInPersistentStore());
}

#if ENABLE(CACHE_PARTITIONING)
void WebResourceCacheManager::cfURLCacheHostNamesWithCallback(CacheCallback callback)
{
    WKCFURLCacheCopyAllPartitionNames(^(CFArrayRef partitionNames) {
        RetainPtr<CFArrayRef> hostNamesInPersistentStore = adoptCF(WKCFURLCacheCopyAllHostNamesInPersistentStoreForPartition(CFSTR("")));
        RetainPtr<CFMutableArrayRef> hostNames = adoptCF(CFArrayCreateMutableCopy(0, 0, hostNamesInPersistentStore.get()));
        CFArrayAppendArray(hostNames.get(), partitionNames, CFRangeMake(0, CFArrayGetCount(partitionNames)));
        CFRelease(partitionNames);
        callback(std::move(hostNames));
    });
}
#endif

void WebResourceCacheManager::clearCFURLCacheForHostNames(CFArrayRef hostNames)
{
    WKCFURLCacheDeleteHostNamesInPersistentStore(hostNames);

#if ENABLE(CACHE_PARTITIONING)
    CFIndex size = CFArrayGetCount(hostNames);
    for (CFIndex i = 0; i < size; ++i) {
        RetainPtr<CFStringRef> partition = partitionName(static_cast<CFStringRef>(CFArrayGetValueAtIndex(hostNames, i)));
        RetainPtr<CFArrayRef> partitionHostNames = adoptCF(WKCFURLCacheCopyAllHostNamesInPersistentStoreForPartition(partition.get()));
        WKCFURLCacheDeleteHostNamesInPersistentStoreForPartition(partitionHostNames.get(), partition.get());
    }
#endif
}

} // namespace WebKit

#endif // USE(CFURLCACHE)
