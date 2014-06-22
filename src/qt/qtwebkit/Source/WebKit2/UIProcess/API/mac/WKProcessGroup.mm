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

#import "config.h"
#import "WKProcessGroup.h"
#import "WKProcessGroupPrivate.h"

#import "ObjCObjectGraph.h"
#import "WKConnectionInternal.h"
#import "WKContext.h"
#import "WKRetainPtr.h"
#import "WKStringCF.h"
#import <wtf/RetainPtr.h>

@interface WKProcessGroupData : NSObject {
@public
    // Underlying context object.
    WKRetainPtr<WKContextRef> _contextRef;

    // Delegate for callbacks.
    id<WKProcessGroupDelegate> _delegate;
}
@end

@implementation WKProcessGroupData
@end

@implementation WKProcessGroup

static void didCreateConnection(WKContextRef, WKConnectionRef connectionRef, const void* clientInfo)
{
    WKProcessGroup *processGroup = (WKProcessGroup *)clientInfo;
    if ([processGroup.delegate respondsToSelector:@selector(processGroup:didCreateConnectionToWebProcessPlugIn:)]) {
        RetainPtr<WKConnection> connection = adoptNS([[WKConnection alloc] _initWithConnectionRef:connectionRef]);
        [processGroup.delegate processGroup:processGroup didCreateConnectionToWebProcessPlugIn:connection.get()];
    }
}

static void setUpConnectionClient(WKProcessGroup *processGroup, WKContextRef contextRef)
{
    WKContextConnectionClient connectionClient;
    memset(&connectionClient, 0, sizeof(connectionClient));

    connectionClient.version = kWKContextConnectionClientCurrentVersion;
    connectionClient.clientInfo = processGroup;
    connectionClient.didCreateConnection = didCreateConnection;

    WKContextSetConnectionClient(contextRef, &connectionClient);
}

static WKTypeRef getInjectedBundleInitializationUserData(WKContextRef, const void* clientInfo)
{
    WKProcessGroup *processGroup = (WKProcessGroup *)clientInfo;
    if ([processGroup.delegate respondsToSelector:@selector(processGroupWillCreateConnectionToWebProcessPlugIn:)]) {
        RetainPtr<id> initializationUserData = [processGroup.delegate processGroupWillCreateConnectionToWebProcessPlugIn:processGroup];
        RefPtr<WebKit::ObjCObjectGraph> wkMessageBody = WebKit::ObjCObjectGraph::create(initializationUserData.get());
        return (WKTypeRef)wkMessageBody.release().leakRef();
    }

    return 0;
}

static void setUpInectedBundleClient(WKProcessGroup *processGroup, WKContextRef contextRef)
{
    WKContextInjectedBundleClient injectedBundleClient;
    memset(&injectedBundleClient, 0, sizeof(injectedBundleClient));

    injectedBundleClient.version = kWKContextInjectedBundleClientCurrentVersion;
    injectedBundleClient.clientInfo = processGroup;
    injectedBundleClient.getInjectedBundleInitializationUserData = getInjectedBundleInitializationUserData;

    WKContextSetInjectedBundleClient(contextRef, &injectedBundleClient);
}

- (id)init
{
    return [self initWithInjectedBundleURL:nil];
}

- (id)initWithInjectedBundleURL:(NSURL *)bundleURL
{
    self = [super init];
    if (!self)
        return nil;

    _data = [[WKProcessGroupData alloc] init];
    
    if (bundleURL)
        _data->_contextRef = adoptWK(WKContextCreateWithInjectedBundlePath(adoptWK(WKStringCreateWithCFString((CFStringRef)[bundleURL path])).get()));
    else
        _data->_contextRef = adoptWK(WKContextCreate());

    setUpConnectionClient(self, _data->_contextRef.get());
    setUpInectedBundleClient(self, _data->_contextRef.get());

    return self;
}

- (void)dealloc
{
    WKContextSetConnectionClient(_data->_contextRef.get(), 0);
    WKContextSetInjectedBundleClient(_data->_contextRef.get(), 0);

    [_data release];
    [super dealloc];
}

- (id<WKProcessGroupDelegate>)delegate
{
    return _data->_delegate;
}

- (void)setDelegate:(id<WKProcessGroupDelegate>)delegate
{
    _data->_delegate = delegate;
}

@end

@implementation WKProcessGroup (Private)

- (WKContextRef)_contextRef
{
    return _data->_contextRef.get();
}

@end
