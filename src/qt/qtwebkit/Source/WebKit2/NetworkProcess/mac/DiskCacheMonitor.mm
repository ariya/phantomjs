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

#import "config.h"
#import "DiskCacheMonitor.h"

#import "NetworkConnectionToWebProcess.h"
#import "NetworkProcessConnectionMessages.h"
#import "NetworkResourceLoader.h"
#import "WebCoreArgumentCoders.h"

#ifdef __has_include
#if __has_include(<CFNetwork/CFURLCachePriv.h>)
#include <CFNetwork/CFURLCachePriv.h>
#endif
#endif

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090

typedef void (^CFCachedURLResponseCallBackBlock)(CFCachedURLResponseRef);
extern "C" void _CFCachedURLResponseSetBecameFileBackedCallBackBlock(CFCachedURLResponseRef, CFCachedURLResponseCallBackBlock, dispatch_queue_t);

using namespace WebCore;

namespace WebKit {

// The maximum number of seconds we'll try to wait for a resource to be disk cached before we forget the request.
static const double diskCacheMonitorTimeout = 20;

void DiskCacheMonitor::monitorFileBackingStoreCreation(CFCachedURLResponseRef cachedResponse, NetworkResourceLoader* loader)
{
    if (!cachedResponse)
        return;

    ASSERT(loader);

    new DiskCacheMonitor(cachedResponse, loader); // Balanced by adoptPtr in the blocks setup in the constructor, one of which is guaranteed to run.
}

DiskCacheMonitor::DiskCacheMonitor(CFCachedURLResponseRef cachedResponse, NetworkResourceLoader* loader)
    : m_connectionToWebProcess(loader->connectionToWebProcess())
    , m_resourceRequest(loader->request())
{
    ASSERT(isMainThread());

    // Set up a delayed callback to cancel this monitor if the resource hasn't been cached yet.
    __block DiskCacheMonitor* rawMonitor = this;

    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * diskCacheMonitorTimeout), dispatch_get_main_queue(), ^{
        adoptPtr(rawMonitor); // Balanced by `new DiskCacheMonitor` in monitorFileBackingStoreCreation.
        rawMonitor = 0;
    });

    // Set up the disk caching callback to create the ShareableResource and send it to the WebProcess.
    CFCachedURLResponseCallBackBlock block = ^(CFCachedURLResponseRef cachedResponse)
    {
        // If the monitor isn't there then it timed out before this resource was cached to disk.
        if (!rawMonitor)
            return;

        OwnPtr<DiskCacheMonitor> monitor = adoptPtr(rawMonitor); // Balanced by `new DiskCacheMonitor` in monitorFileBackingStoreCreation.
        rawMonitor = 0;
        
        ShareableResource::Handle handle;
        NetworkResourceLoader::tryGetShareableHandleFromCFURLCachedResponse(handle, cachedResponse);
        if (handle.isNull())
            return;

        monitor->send(Messages::NetworkProcessConnection::DidCacheResource(monitor->resourceRequest(), handle));
    };

    _CFCachedURLResponseSetBecameFileBackedCallBackBlock(cachedResponse, block, dispatch_get_main_queue());
}

CoreIPC::Connection* DiskCacheMonitor::messageSenderConnection()
{
    return m_connectionToWebProcess->connection();
}

uint64_t DiskCacheMonitor::messageSenderDestinationID()
{
    return 0;
}

} // namespace WebKit

#endif // #if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
