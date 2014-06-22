/*
 * Copyright (C) 2005 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import <WebKit/WebDownload.h>

#import <Foundation/NSURLAuthenticationChallenge.h>
#import <Foundation/NSURLDownload.h>
#import <WebCore/AuthenticationCF.h>
#import <WebCore/AuthenticationMac.h>
#import <WebCore/Credential.h>
#import <WebCore/CredentialStorage.h>
#import <WebCore/ProtectionSpace.h>
#import <WebKit/WebPanelAuthenticationHandler.h>
#import <wtf/Assertions.h>

#import "WebTypesInternal.h"

#if USE(CFNETWORK)
#import <CFNetwork/CFNetwork.h>
#import <CFNetwork/CFURLConnection.h>
#endif

using namespace WebCore;

@class NSURLConnectionDelegateProxy;

// FIXME: The following are NSURLDownload SPI - it would be nice to not have to override them at 
// some point in the future
@interface NSURLDownload (WebDownloadCapability)
- (id)_initWithLoadingConnection:(NSURLConnection *)connection
                         request:(NSURLRequest *)request
                        response:(NSURLResponse *)response
                        delegate:(id)delegate
                           proxy:(NSURLConnectionDelegateProxy *)proxy;
- (id)_initWithRequest:(NSURLRequest *)request
              delegate:(id)delegate
             directory:(NSString *)directory;

#if USE(CFNETWORK)
- (id)_initWithLoadingCFURLConnection:(CFURLConnectionRef)connection
                              request:(CFURLRequestRef)request
                             response:(CFURLResponseRef)response
                             delegate:(id)delegate
                                proxy:(NSURLConnectionDelegateProxy *)proxy;
#endif

@end

@interface WebDownloadInternal : NSObject <NSURLDownloadDelegate>
{
@public
    id realDelegate;
}

- (void)setRealDelegate:(id)rd;

@end

@implementation WebDownloadInternal

- (void)dealloc
{
    [realDelegate release];
    [super dealloc];
}

- (void)setRealDelegate:(id)rd
{
    [rd retain];
    [realDelegate release];
    realDelegate = rd;
}

- (BOOL)respondsToSelector:(SEL)selector
{
    if (selector == @selector(downloadDidBegin:) ||
        selector == @selector(download:willSendRequest:redirectResponse:) ||
        selector == @selector(download:didReceiveResponse:) ||
        selector == @selector(download:didReceiveDataOfLength:) ||
        selector == @selector(download:shouldDecodeSourceDataOfMIMEType:) ||
        selector == @selector(download:decideDestinationWithSuggestedFilename:) ||
        selector == @selector(download:didCreateDestination:) ||
        selector == @selector(downloadDidFinish:) ||
        selector == @selector(download:didFailWithError:) ||
        selector == @selector(download:shouldBeginChildDownloadOfSource:delegate:) ||
        selector == @selector(download:didBeginChildDownload:)) {
        return [realDelegate respondsToSelector:selector];
    }

    return [super respondsToSelector:selector];
}

- (void)downloadDidBegin:(NSURLDownload *)download
{
    [realDelegate downloadDidBegin:download];
}

- (NSURLRequest *)download:(NSURLDownload *)download willSendRequest:(NSURLRequest *)request redirectResponse:(NSURLResponse *)redirectResponse
{
    return [realDelegate download:download willSendRequest:request redirectResponse:redirectResponse];
}

- (void)download:(NSURLDownload *)download didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
    // Try previously stored credential first.
    if (![challenge previousFailureCount]) {
        NSURLCredential *credential = mac(CredentialStorage::get(core([challenge protectionSpace])));
        if (credential) {
            [[challenge sender] useCredential:credential forAuthenticationChallenge:challenge];
            return;
        }
    }

    if ([realDelegate respondsToSelector:@selector(download:didReceiveAuthenticationChallenge:)]) {
        [realDelegate download:download didReceiveAuthenticationChallenge:challenge];
    } else {
        NSWindow *window = nil;
        if ([realDelegate respondsToSelector:@selector(downloadWindowForAuthenticationSheet:)]) {
            window = [realDelegate downloadWindowForAuthenticationSheet:(WebDownload *)download];
        }

        [[WebPanelAuthenticationHandler sharedHandler] startAuthentication:challenge window:window];
    }
}

- (void)download:(NSURLDownload *)download didCancelAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
    if ([realDelegate respondsToSelector:@selector(download:didCancelAuthenticationChallenge:)]) {
        [realDelegate download:download didCancelAuthenticationChallenge:challenge];
    } else {
        [[WebPanelAuthenticationHandler sharedHandler] cancelAuthentication:challenge];
    }
}

- (void)download:(NSURLDownload *)download didReceiveResponse:(NSURLResponse *)response
{
    [realDelegate download:download didReceiveResponse:response];
}

- (void)download:(NSURLDownload *)download didReceiveDataOfLength:(NSUInteger)length
{
    [realDelegate download:download didReceiveDataOfLength:length];
}

- (BOOL)download:(NSURLDownload *)download shouldDecodeSourceDataOfMIMEType:(NSString *)encodingType
{
    return [realDelegate download:download shouldDecodeSourceDataOfMIMEType:encodingType];
}

- (void)download:(NSURLDownload *)download decideDestinationWithSuggestedFilename:(NSString *)filename
{
    [realDelegate download:download decideDestinationWithSuggestedFilename:filename];
}

- (void)download:(NSURLDownload *)download didCreateDestination:(NSString *)path
{
    [realDelegate download:download didCreateDestination:path];
}

- (void)downloadDidFinish:(NSURLDownload *)download
{
    [realDelegate downloadDidFinish:download];
}

- (void)download:(NSURLDownload *)download didFailWithError:(NSError *)error
{
    [realDelegate download:download didFailWithError:error];
}

- (NSURLRequest *)download:(NSURLDownload *)download shouldBeginChildDownloadOfSource:(NSURLRequest *)child delegate:(id *)childDelegate
{
    return [realDelegate download:download shouldBeginChildDownloadOfSource:child delegate:childDelegate];
}

- (void)download:(NSURLDownload *)parent didBeginChildDownload:(NSURLDownload *)child
{
    [realDelegate download:parent didBeginChildDownload:child];
}

@end

@implementation WebDownload

- (void)_setRealDelegate:(id)delegate
{
    if (_webInternal == nil) {
        _webInternal = [[WebDownloadInternal alloc] init];
        [_webInternal setRealDelegate:delegate];
    } else {
        ASSERT(_webInternal == delegate);
    }
}

- (id)init
{
    self = [super init];
    if (self != nil) {
        // _webInternal can be set up before init by _setRealDelegate
        if (_webInternal == nil) {
            _webInternal = [[WebDownloadInternal alloc] init];
        }
    }
    return self;
}

- (void)dealloc
{
    [_webInternal release];
    [super dealloc];
}

- (id)initWithRequest:(NSURLRequest *)request delegate:(id<NSURLDownloadDelegate>)delegate
{
    [self _setRealDelegate:delegate];
    return [super initWithRequest:request delegate:_webInternal];
}

- (id)_initWithLoadingConnection:(NSURLConnection *)connection
                         request:(NSURLRequest *)request
                        response:(NSURLResponse *)response
                        delegate:(id)delegate
                           proxy:(NSURLConnectionDelegateProxy *)proxy
{
    [self _setRealDelegate:delegate];
    return [super _initWithLoadingConnection:connection request:request response:response delegate:_webInternal proxy:proxy];
}

#if USE(CFNETWORK)
- (id)_initWithLoadingCFURLConnection:(CFURLConnectionRef)connection
                              request:(CFURLRequestRef)request
                             response:(CFURLResponseRef)response
                             delegate:(id)delegate
                                proxy:(NSURLConnectionDelegateProxy *)proxy
{
    [self _setRealDelegate:delegate];
    return [super _initWithLoadingCFURLConnection:connection request:request response:response delegate:_webInternal proxy:proxy];
}
#endif

- (id)_initWithRequest:(NSURLRequest *)request
              delegate:(id)delegate
             directory:(NSString *)directory
{
    [self _setRealDelegate:delegate];
    return [super _initWithRequest:request delegate:_webInternal directory:directory];
}

- (void)connection:(NSURLConnection *)connection willStopBufferingData:(NSData *)data
{
    // NSURLConnection calls this method even if it is not implemented.
    // This happens because NSURLConnection caches the results of respondsToSelector.
    // Those results become invalid when the delegate of NSURLConnectionDelegateProxy is changed.
    // This is a workaround since this problem needs to be fixed in NSURLConnectionDelegateProxy.
    // <rdar://problem/3913270> NSURLConnection calls unimplemented delegate method in WebDownload
}

@end
