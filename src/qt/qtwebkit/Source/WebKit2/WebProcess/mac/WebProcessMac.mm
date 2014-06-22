/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#import "WebProcess.h"

#import "CustomProtocolManager.h"
#import "SandboxExtension.h"
#import "SandboxInitializationParameters.h"
#import "WKFullKeyboardAccessWatcher.h"
#import "WebFrame.h"
#import "WebInspector.h"
#import "WebPage.h"
#import "WebProcessCreationParameters.h"
#import "WebProcessProxyMessages.h"
#import <WebCore/FileSystem.h>
#import <WebCore/Font.h>
#import <WebCore/LocalizedStrings.h>
#import <WebCore/MemoryCache.h>
#import <WebCore/PageCache.h>
#import <WebCore/WebCoreNSURLExtras.h>
#import <WebKitSystemInterface.h>
#import <algorithm>
#import <dispatch/dispatch.h>
#import <mach/host_info.h>
#import <mach/mach.h>
#import <mach/mach_error.h>
#import <objc/runtime.h>
#import <stdio.h>

#if USE(SECURITY_FRAMEWORK)
#import "SecItemShim.h"
#endif

using namespace WebCore;

const CFStringRef kLSActivePageUserVisibleOriginsKey = CFSTR("LSActivePageUserVisibleOriginsKey");

namespace WebKit {

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

static uint64_t volumeFreeSize(NSString *path)
{
    NSDictionary *fileSystemAttributesDictionary = [[NSFileManager defaultManager] attributesOfFileSystemForPath:path error:NULL];
    return [[fileSystemAttributesDictionary objectForKey:NSFileSystemFreeSize] unsignedLongLongValue];
}

void WebProcess::platformSetCacheModel(CacheModel cacheModel)
{
    RetainPtr<NSString> nsurlCacheDirectory = adoptNS((NSString *)WKCopyFoundationCacheDirectory());
    if (!nsurlCacheDirectory)
        nsurlCacheDirectory = NSHomeDirectory();

    // As a fudge factor, use 1000 instead of 1024, in case the reported byte 
    // count doesn't align exactly to a megabyte boundary.
    uint64_t memSize = memorySize() / 1024 / 1000;
    uint64_t diskFreeSize = volumeFreeSize(nsurlCacheDirectory.get()) / 1024 / 1000;

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


    memoryCache()->setCapacities(cacheMinDeadCapacity, cacheMaxDeadCapacity, cacheTotalCapacity);
    memoryCache()->setDeadDecodedDataDeletionInterval(deadDecodedDataDeletionInterval);
    pageCache()->setCapacity(pageCacheCapacity);

    NSURLCache *nsurlCache = [NSURLCache sharedURLCache];

#if ENABLE(NETWORK_PROCESS)
    // FIXME: Once there is no loading being done in the WebProcess, we should remove this,
    // as calling [NSURLCache sharedURLCache] initializes the cache, which we would rather not do.
    if (m_usesNetworkProcess) {
        [nsurlCache setMemoryCapacity:0];
        [nsurlCache setDiskCapacity:0];
        return;
    }
#endif

    [nsurlCache setMemoryCapacity:urlCacheMemoryCapacity];
    [nsurlCache setDiskCapacity:max<unsigned long>(urlCacheDiskCapacity, [nsurlCache diskCapacity])]; // Don't shrink a big disk cache, since that would cause churn.
}

void WebProcess::platformClearResourceCaches(ResourceCachesToClear cachesToClear)
{
    if (cachesToClear == InMemoryResourceCachesOnly)
        return;

    // If we're using the network process then it is the only one that needs to clear the disk cache.
    if (usesNetworkProcess())
        return;

    if (!m_clearResourceCachesDispatchGroup)
        m_clearResourceCachesDispatchGroup = dispatch_group_create();

    dispatch_group_async(m_clearResourceCachesDispatchGroup, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        [[NSURLCache sharedURLCache] removeAllCachedResponses];
    });
}

static id NSApplicationAccessibilityFocusedUIElement(NSApplication*, SEL)
{
    WebPage* page = WebProcess::shared().focusedWebPage();
    if (!page || !page->accessibilityRemoteObject())
        return 0;

    return [page->accessibilityRemoteObject() accessibilityFocusedUIElement];
}

void WebProcess::platformInitializeWebProcess(const WebProcessCreationParameters& parameters, CoreIPC::MessageDecoder&)
{
    SandboxExtension::consumePermanently(parameters.uiProcessBundleResourcePathExtensionHandle);
    SandboxExtension::consumePermanently(parameters.localStorageDirectoryExtensionHandle);
    SandboxExtension::consumePermanently(parameters.databaseDirectoryExtensionHandle);
    SandboxExtension::consumePermanently(parameters.applicationCacheDirectoryExtensionHandle);
    SandboxExtension::consumePermanently(parameters.diskCacheDirectoryExtensionHandle);

    // When the network process is enabled, each web process wants a stand-alone
    // NSURLCache, which it can disable to save memory.
#if ENABLE(NETWORK_PROCESS)
    if (!m_usesNetworkProcess) {
#endif
        if (!parameters.diskCacheDirectory.isNull()) {
            [NSURLCache setSharedURLCache:adoptNS([[NSURLCache alloc]
                initWithMemoryCapacity:parameters.nsURLCacheMemoryCapacity
                diskCapacity:parameters.nsURLCacheDiskCapacity
                diskPath:parameters.diskCacheDirectory]).get()];
        }
#if ENABLE(NETWORK_PROCESS)
    }
#endif

    m_shouldForceScreenFontSubstitution = parameters.shouldForceScreenFontSubstitution;
    Font::setDefaultTypesettingFeatures(parameters.shouldEnableKerningAndLigaturesByDefault ? Kerning | Ligatures : 0);

    m_compositingRenderServerPort = parameters.acceleratedCompositingPort.port();

    m_presenterApplicationPid = parameters.presenterApplicationPid;

    // rdar://9118639 accessibilityFocusedUIElement in NSApplication defaults to use the keyWindow. Since there's
    // no window in WK2, NSApplication needs to use the focused page's focused element.
    Method methodToPatch = class_getInstanceMethod([NSApplication class], @selector(accessibilityFocusedUIElement));
    method_setImplementation(methodToPatch, (IMP)NSApplicationAccessibilityFocusedUIElement);
}

void WebProcess::initializeProcessName(const ChildProcessInitializationParameters& parameters)
{
    NSString *applicationName = [NSString stringWithFormat:WEB_UI_STRING("%@ Web Content", "Visible name of the web process. The argument is the application name."), (NSString *)parameters.uiProcessName];
    WKSetVisibleApplicationName((CFStringRef)applicationName);
}

void WebProcess::platformInitializeProcess(const ChildProcessInitializationParameters&)
{
    WKAXRegisterRemoteApp();

#if USE(SECURITY_FRAMEWORK)
    SecItemShim::shared().initialize(this);
#endif
}

void WebProcess::platformTerminate()
{
    if (m_clearResourceCachesDispatchGroup) {
        dispatch_group_wait(m_clearResourceCachesDispatchGroup, DISPATCH_TIME_FOREVER);
        dispatch_release(m_clearResourceCachesDispatchGroup);
        m_clearResourceCachesDispatchGroup = 0;
    }
}

void WebProcess::initializeSandbox(const ChildProcessInitializationParameters& parameters, SandboxInitializationParameters& sandboxParameters)
{
    // Need to overide the default, because service has a different bundle ID.
    NSBundle *webkit2Bundle = [NSBundle bundleForClass:NSClassFromString(@"WKView")];
    sandboxParameters.setOverrideSandboxProfilePath([webkit2Bundle pathForResource:@"com.apple.WebProcess" ofType:@"sb"]);

    ChildProcess::initializeSandbox(parameters, sandboxParameters);
}

void WebProcess::updateActivePages()
{
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    RetainPtr<CFMutableArrayRef> activePageURLs = adoptCF(CFArrayCreateMutable(0, 0, &kCFTypeArrayCallBacks));
    for (const auto& iter: m_pageMap) {
        WebPage* page = iter.value.get();
        WebFrame* mainFrame = page->mainWebFrame();
        if (!mainFrame)
            continue;
        String mainFrameOriginString;
        RefPtr<SecurityOrigin> mainFrameOrigin = SecurityOrigin::createFromString(mainFrame->url());
        if (!mainFrameOrigin->isUnique())
            mainFrameOriginString = mainFrameOrigin->toRawString();
        else
            mainFrameOriginString = KURL(KURL(), mainFrame->url()).protocol() + ':'; // toRawString() is not supposed to work with unique origins, and would just return "://".

        NSURL *originAsNSURL = [NSURL URLWithString:mainFrameOriginString];
        // +[NSURL URLWithString:] returns nil when its argument is malformed. It's unclear how we can possibly have a malformed URL here,
        // but it happens in practice according to <rdar://problem/14173389>. Leaving an assertion in to catch a reproducible case.
        ASSERT(originAsNSURL);
        NSString *userVisibleOriginString = originAsNSURL ? userVisibleString(originAsNSURL) : @"(null)";

        CFArrayAppendValue(activePageURLs.get(), userVisibleOriginString);
    }
    WKSetApplicationInformationItem(kLSActivePageUserVisibleOriginsKey, activePageURLs.get());
#endif
}

} // namespace WebKit
