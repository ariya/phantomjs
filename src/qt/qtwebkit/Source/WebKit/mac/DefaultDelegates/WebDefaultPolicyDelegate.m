/*
 * Copyright (C) 2005, 2008 Apple Inc. All rights reserved.
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

#import "WebDefaultPolicyDelegate.h"

#import "WebDataSource.h"
#import "WebFrame.h"
#import "WebPolicyDelegatePrivate.h"
#import "WebViewInternal.h"
#import <Foundation/NSURLConnection.h>
#import <Foundation/NSURLRequest.h>
#import <Foundation/NSURLResponse.h>
#import <wtf/Assertions.h>

@implementation WebDefaultPolicyDelegate

static WebDefaultPolicyDelegate *sharedDelegate = nil;

// Return a object with vanilla implementations of the protocol's methods
// Note this feature relies on our default delegate being stateless
+ (WebDefaultPolicyDelegate *)sharedPolicyDelegate
{
    if (!sharedDelegate) {
        sharedDelegate = [[WebDefaultPolicyDelegate alloc] init];
    }
    return sharedDelegate;
}

- (void)webView: (WebView *)wv unableToImplementPolicyWithError:(NSError *)error frame:(WebFrame *)frame
{
    LOG_ERROR("called unableToImplementPolicyWithError:%@ inFrame:%@", error, frame);
}


- (void)webView: (WebView *)wv decidePolicyForMIMEType:(NSString *)type
                                               request:(NSURLRequest *)request
                                                 frame:(WebFrame *)frame
                                      decisionListener:(id <WebPolicyDecisionListener>)listener
{
    if ([[request URL] isFileURL]) {
        BOOL isDirectory = NO;
        BOOL exists = [[NSFileManager defaultManager] fileExistsAtPath:[[request URL] path] isDirectory:&isDirectory];

        if (exists && !isDirectory && [wv _canShowMIMEType:type])
            [listener use];
        else
            [listener ignore];
    } else if ([wv _canShowMIMEType:type])
        [listener use];
    else
        [listener ignore];
}

- (void)webView: (WebView *)wv decidePolicyForNavigationAction:(NSDictionary *)actionInformation 
                                                       request:(NSURLRequest *)request
                                                         frame:(WebFrame *)frame
                                              decisionListener:(id <WebPolicyDecisionListener>)listener
{
    WebNavigationType navType = [[actionInformation objectForKey:WebActionNavigationTypeKey] intValue];

    if ([WebView _canHandleRequest:request forMainFrame:frame == [wv mainFrame]]) {
        [listener use];
    } else if (navType == WebNavigationTypePlugInRequest) {
        [listener use];
    } else {
        // A file URL shouldn't fall through to here, but if it did,
        // it would be a security risk to open it.
        if (![[request URL] isFileURL]) {
            [[NSWorkspace sharedWorkspace] openURL:[request URL]];
        }
        [listener ignore];
    }
}

- (void)webView: (WebView *)wv decidePolicyForNewWindowAction:(NSDictionary *)actionInformation 
                                                      request:(NSURLRequest *)request
                                                 newFrameName:(NSString *)frameName
                                             decisionListener:(id <WebPolicyDecisionListener>)listener
{
    [listener use];
}

// Temporary SPI needed for <rdar://problem/3951283> can view pages from the back/forward cache that should be disallowed by Parental Controls
- (BOOL)webView:(WebView *)webView shouldGoToHistoryItem:(WebHistoryItem *)item
{
    return YES;
}

@end

