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

#import "config.h"
#import "NetworkProcess.h"

#if ENABLE(NETWORK_PROCESS)

#import "NetworkProcessCreationParameters.h"
#import "NetworkResourceLoader.h"
#import "PlatformCertificateInfo.h"
#import "ResourceCachesToClear.h"
#import "SandboxExtension.h"
#import "SandboxInitializationParameters.h"
#import "StringUtilities.h"
#import <WebCore/FileSystem.h>
#import <WebCore/LocalizedStrings.h>
#import <WebKitSystemInterface.h>
#import <mach/host_info.h>
#import <mach/mach.h>
#import <mach/mach_error.h>
#import <sysexits.h>
#import <wtf/text/WTFString.h>

#if USE(SECURITY_FRAMEWORK)
#import "SecItemShim.h"
#endif

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
typedef struct _CFURLCache* CFURLCacheRef;
extern "C" CFURLCacheRef CFURLCacheCopySharedURLCache();
extern "C" void _CFURLCacheSetMinSizeForVMCachedResource(CFURLCacheRef, CFIndex);
#endif

using namespace WebCore;

@interface NSURLRequest (Details) 
+ (void)setAllowsSpecificHTTPSCertificate:(NSArray *)allow forHost:(NSString *)host;
@end

namespace WebKit {

void NetworkProcess::initializeProcess(const ChildProcessInitializationParameters&)
{
    // Having a window server connection in this process would result in spin logs (<rdar://problem/13239119>).
    setApplicationIsDaemon();
}

void NetworkProcess::initializeProcessName(const ChildProcessInitializationParameters& parameters)
{
    NSString *applicationName = [NSString stringWithFormat:WEB_UI_STRING("%@ Networking", "visible name of the network process. The argument is the application name."), (NSString *)parameters.uiProcessName];
    WKSetVisibleApplicationName((CFStringRef)applicationName);
}

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
static void overrideSystemProxies(const String& httpProxy, const String& httpsProxy)
{
    NSMutableDictionary *proxySettings = [NSMutableDictionary dictionary];

    if (!httpProxy.isNull()) {
        KURL httpProxyURL(KURL(), httpProxy);
        if (httpProxyURL.isValid()) {
            [proxySettings setObject:nsStringFromWebCoreString(httpProxyURL.host()) forKey:(NSString *)kCFNetworkProxiesHTTPProxy];
            if (httpProxyURL.hasPort()) {
                NSNumber *port = [NSNumber numberWithInt:httpProxyURL.port()];
                [proxySettings setObject:port forKey:(NSString *)kCFNetworkProxiesHTTPPort];
            }
        }
        else
            NSLog(@"Malformed HTTP Proxy URL '%s'.  Expected 'http://<hostname>[:<port>]'\n", httpProxy.utf8().data());
    }

    if (!httpsProxy.isNull()) {
        KURL httpsProxyURL(KURL(), httpsProxy);
        if (httpsProxyURL.isValid()) {
            [proxySettings setObject:nsStringFromWebCoreString(httpsProxyURL.host()) forKey:(NSString *)kCFNetworkProxiesHTTPSProxy];
            if (httpsProxyURL.hasPort()) {
                NSNumber *port = [NSNumber numberWithInt:httpsProxyURL.port()];
                [proxySettings setObject:port forKey:(NSString *)kCFNetworkProxiesHTTPSPort];
            }
        } else
            NSLog(@"Malformed HTTPS Proxy URL '%s'.  Expected 'https://<hostname>[:<port>]'\n", httpsProxy.utf8().data());
    }

    if ([proxySettings count] > 0)
        WKCFNetworkSetOverrideSystemProxySettings((CFDictionaryRef)proxySettings);
}
#endif // __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070

void NetworkProcess::platformInitializeNetworkProcess(const NetworkProcessCreationParameters& parameters)
{
    m_diskCacheDirectory = parameters.diskCacheDirectory;

    if (!m_diskCacheDirectory.isNull()) {
        SandboxExtension::consumePermanently(parameters.diskCacheDirectoryExtensionHandle);
        [NSURLCache setSharedURLCache:adoptNS([[NSURLCache alloc]
            initWithMemoryCapacity:parameters.nsURLCacheMemoryCapacity
            diskCapacity:parameters.nsURLCacheDiskCapacity
            diskPath:parameters.diskCacheDirectory]).get()];
    }

#if USE(SECURITY_FRAMEWORK)
    SecItemShim::shared().initialize(this);
#endif

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
    if (!parameters.httpProxy.isNull() || !parameters.httpsProxy.isNull())
        overrideSystemProxies(parameters.httpProxy, parameters.httpsProxy);
#endif

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    RetainPtr<CFURLCacheRef> cache = adoptCF(CFURLCacheCopySharedURLCache());
    if (!cache)
        return;

    _CFURLCacheSetMinSizeForVMCachedResource(cache.get(), NetworkResourceLoader::fileBackedResourceMinimumSize());
#endif
}

static uint64_t memorySize()
{
    static host_basic_info_data_t hostInfo;

    static dispatch_once_t once;
    dispatch_once(&once, ^() {
        mach_port_t host = mach_host_self();
        mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;
        kern_return_t r = host_info(host, HOST_BASIC_INFO, (host_info_t)&hostInfo, &count);
        mach_port_deallocate(mach_task_self(), host);

        if (r != KERN_SUCCESS)
            LOG_ERROR("%s : host_info(%d) : %s.\n", __FUNCTION__, r, mach_error_string(r));
    });

    return hostInfo.max_mem;
}

static uint64_t volumeFreeSize(const String& path)
{
    NSDictionary *fileSystemAttributesDictionary = [[NSFileManager defaultManager] attributesOfFileSystemForPath:(NSString *)path error:NULL];
    return [[fileSystemAttributesDictionary objectForKey:NSFileSystemFreeSize] unsignedLongLongValue];
}

void NetworkProcess::platformSetCacheModel(CacheModel cacheModel)
{

    // As a fudge factor, use 1000 instead of 1024, in case the reported byte 
    // count doesn't align exactly to a megabyte boundary.
    uint64_t memSize = memorySize() / 1024 / 1000;
    uint64_t diskFreeSize = volumeFreeSize(m_diskCacheDirectory) / 1024 / 1000;

    unsigned cacheTotalCapacity = 0;
    unsigned cacheMinDeadCapacity = 0;
    unsigned cacheMaxDeadCapacity = 0;
    double deadDecodedDataDeletionInterval = 0;
    unsigned pageCacheCapacity = 0;
    unsigned long urlCacheMemoryCapacity = 0;
    unsigned long urlCacheDiskCapacity = 0;

    calculateCacheSizes(cacheModel, memSize, diskFreeSize,
        cacheTotalCapacity, cacheMinDeadCapacity, cacheMaxDeadCapacity, deadDecodedDataDeletionInterval,
        pageCacheCapacity, urlCacheMemoryCapacity, urlCacheDiskCapacity);


    NSURLCache *nsurlCache = [NSURLCache sharedURLCache];
    [nsurlCache setMemoryCapacity:urlCacheMemoryCapacity];
    [nsurlCache setDiskCapacity:std::max<unsigned long>(urlCacheDiskCapacity, [nsurlCache diskCapacity])]; // Don't shrink a big disk cache, since that would cause churn.
}

void NetworkProcess::allowSpecificHTTPSCertificateForHost(const PlatformCertificateInfo& certificateInfo, const String& host)
{
    [NSURLRequest setAllowsSpecificHTTPSCertificate:(NSArray *)certificateInfo.certificateChain() forHost:(NSString *)host];
}

void NetworkProcess::initializeSandbox(const ChildProcessInitializationParameters& parameters, SandboxInitializationParameters& sandboxParameters)
{
    // Need to overide the default, because service has a different bundle ID.
    NSBundle *webkit2Bundle = [NSBundle bundleForClass:NSClassFromString(@"WKView")];
    sandboxParameters.setOverrideSandboxProfilePath([webkit2Bundle pathForResource:@"com.apple.WebKit.NetworkProcess" ofType:@"sb"]);

    ChildProcess::initializeSandbox(parameters, sandboxParameters);
}

void NetworkProcess::clearCacheForAllOrigins(uint32_t cachesToClear)
{
    ResourceCachesToClear resourceCachesToClear = static_cast<ResourceCachesToClear>(cachesToClear);
    if (resourceCachesToClear == InMemoryResourceCachesOnly)
        return;

    if (!m_clearCacheDispatchGroup)
        m_clearCacheDispatchGroup = dispatch_group_create();

    dispatch_group_async(m_clearCacheDispatchGroup, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        [[NSURLCache sharedURLCache] removeAllCachedResponses];
    });
}

void NetworkProcess::platformTerminate()
{
    if (m_clearCacheDispatchGroup) {
        dispatch_group_wait(m_clearCacheDispatchGroup, DISPATCH_TIME_FOREVER);
        dispatch_release(m_clearCacheDispatchGroup);
        m_clearCacheDispatchGroup = 0;
    }
}

} // namespace WebKit

#endif // ENABLE(NETWORK_PROCESS)
