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

#if defined(__LP64__) && defined(__clang__)

#import "WKWebProcessPlugIn.h"
#import "WKWebProcessPlugInPrivate.h"
#import "WKWebProcessPlugInInternal.h"

#import "InjectedBundle.h"
#import "WKConnectionInternal.h"
#import "WKBundle.h"
#import "WKBundleAPICast.h"
#import "WKRetainPtr.h"
#import "WKWebProcessPlugInBrowserContextControllerInternal.h"
#import <wtf/RetainPtr.h>

typedef HashMap<WKBundlePageRef, RetainPtr<WKWebProcessPlugInBrowserContextController *>> BundlePageWrapperCache;

@interface WKWebProcessPlugInController () {
    RetainPtr<id <WKWebProcessPlugIn> > _principalClassInstance;
    WKRetainPtr<WKBundleRef> _bundleRef;
    BundlePageWrapperCache _bundlePageWrapperCache;
    RetainPtr<WKConnection *> _connectionWrapper;
}
@end

@implementation WKWebProcessPlugInController (Internal)

static void didCreatePage(WKBundleRef bundle, WKBundlePageRef page, const void* clientInfo)
{
    WKWebProcessPlugInController *plugInController = (WKWebProcessPlugInController *)clientInfo;
    id<WKWebProcessPlugIn> principalClassInstance = plugInController->_principalClassInstance.get();

    if ([principalClassInstance respondsToSelector:@selector(webProcessPlugIn:didCreateBrowserContextController:)]) {
        ASSERT(!plugInController->_bundlePageWrapperCache.contains(page));

        WKWebProcessPlugInBrowserContextController* browserContextController = [[WKWebProcessPlugInBrowserContextController alloc] _initWithBundlePageRef:page];
        plugInController->_bundlePageWrapperCache.set(page, browserContextController);

        [principalClassInstance webProcessPlugIn:plugInController didCreateBrowserContextController:browserContextController];
    }
}

static void willDestroyPage(WKBundleRef bundle, WKBundlePageRef page, const void* clientInfo)
{
    WKWebProcessPlugInController *plugInController = (WKWebProcessPlugInController *)clientInfo;
    id<WKWebProcessPlugIn> principalClassInstance = plugInController->_principalClassInstance.get();

    // If we never added the bundle page to the cache, which can happen if webProcessPlugIn:didCreateBrowserContextController: is not implemented,
    // there is no reason to call webProcessPlugIn:willDestroyBrowserContextController:, so don't.
    BundlePageWrapperCache::iterator it = plugInController->_bundlePageWrapperCache.find(page);
    if (it == plugInController->_bundlePageWrapperCache.end()) {
        ASSERT(![principalClassInstance respondsToSelector:@selector(webProcessPlugIn:didCreateBrowserContextController:)]);
        return;
    }

    if ([principalClassInstance respondsToSelector:@selector(webProcessPlugIn:willDestroyBrowserContextController:)])
        [principalClassInstance webProcessPlugIn:plugInController willDestroyBrowserContextController:it->value.get()];

    plugInController->_bundlePageWrapperCache.remove(it);
}

static void setUpBundleClient(WKWebProcessPlugInController *plugInController, WKBundleRef bundleRef)
{
    WKBundleClient bundleClient;
    memset(&bundleClient, 0, sizeof(bundleClient));

    bundleClient.version = kWKBundleClientCurrentVersion;
    bundleClient.clientInfo = plugInController;
    bundleClient.didCreatePage = didCreatePage;
    bundleClient.willDestroyPage = willDestroyPage;

    WKBundleSetClient(bundleRef, &bundleClient);
}

static WKWebProcessPlugInController *sharedInstance;

+ (WKWebProcessPlugInController *)_shared
{
    ASSERT_WITH_MESSAGE(sharedInstance, "+[WKWebProcessPlugIn _shared] called without first initializing it.");
    return sharedInstance;
}

- (id)_initWithPrincipalClassInstance:(id<WKWebProcessPlugIn>)principalClassInstance bundleRef:(WKBundleRef)bundleRef
{
    self = [super init];
    if (!self)
        return nil;

    _principalClassInstance = principalClassInstance;
    _bundleRef = bundleRef;
    _connectionWrapper = adoptNS([[WKConnection alloc] _initWithConnectionRef:WKBundleGetApplicationConnection(_bundleRef.get())]);

    ASSERT_WITH_MESSAGE(!sharedInstance, "WKWebProcessPlugInController initialized multiple times.");
    sharedInstance = self;

    setUpBundleClient(self, bundleRef);

    return self;
}

- (WKWebProcessPlugInBrowserContextController *)_browserContextControllerForBundlePageRef:(WKBundlePageRef)pageRef
{
    ASSERT(_bundlePageWrapperCache.contains(pageRef));
    return _bundlePageWrapperCache.get(pageRef).get();
}

@end

@implementation WKWebProcessPlugInController

- (WKConnection *)connection
{
    return _connectionWrapper.get();
}

@end

@implementation WKWebProcessPlugInController (Private)

- (WKBundleRef)_bundleRef
{
    return _bundleRef.get();
}

@end

#endif // defined(__LP64__) && defined(__clang__)
