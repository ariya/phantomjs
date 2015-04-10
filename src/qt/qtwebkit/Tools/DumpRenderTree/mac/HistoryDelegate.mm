/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */
#import "config.h"
#import "HistoryDelegate.h"

#import "DumpRenderTree.h"
#import "TestRunner.h"

#import <WebKit/WebNavigationData.h>
#import <WebKit/WebView.h>

@interface NSURL (DRTExtras)
- (NSString *)_drt_descriptionSuitableForTestResult;
@end

@implementation HistoryDelegate

- (void)webView:(WebView *)webView didNavigateWithNavigationData:(WebNavigationData *)navigationData inFrame:(WebFrame *)webFrame
{
    NSURL *url = [navigationData url] ? [NSURL URLWithString:[navigationData url]] : nil;
    bool hasClientRedirect = [[navigationData clientRedirectSource] length];
    NSHTTPURLResponse *httpResponse = [[navigationData response] isKindOfClass:[NSHTTPURLResponse class]] ? (NSHTTPURLResponse *)[navigationData response] : nil;
    bool wasFailure = [navigationData hasSubstituteData] || (httpResponse && [httpResponse statusCode] >= 400);
            
    printf("WebView navigated to url \"%s\" with title \"%s\" with HTTP equivalent method \"%s\".  The navigation was %s and was %s%s.\n", 
        url ? [[url _drt_descriptionSuitableForTestResult] UTF8String] : "<none>", 
        [navigationData title] ? [[navigationData title] UTF8String] : "",
        [navigationData originalRequest] ? [[[navigationData originalRequest] HTTPMethod] UTF8String] : "",
        wasFailure ? "a failure" : "successful", 
        hasClientRedirect ? "a client redirect from " : "not a client redirect",
        hasClientRedirect ? [[navigationData clientRedirectSource] UTF8String] : "");
}
                              
- (void)webView:(WebView *)webView didPerformClientRedirectFromURL:(NSString *)sourceURL toURL:(NSString *)destinationURL inFrame:(WebFrame *)webFrame
{
    NSURL *source = [NSURL URLWithString:sourceURL];
    NSURL *dest = [NSURL URLWithString:destinationURL];
    printf("WebView performed a client redirect from \"%s\" to \"%s\".\n", [[source _drt_descriptionSuitableForTestResult] UTF8String], [[dest _drt_descriptionSuitableForTestResult] UTF8String]);
}

- (void)webView:(WebView *)webView didPerformServerRedirectFromURL:(NSString *)sourceURL toURL:(NSString *)destinationURL inFrame:(WebFrame *)webFrame
{
    NSURL *source = [NSURL URLWithString:sourceURL];
    NSURL *dest = [NSURL URLWithString:destinationURL];
    printf("WebView performed a server redirect from \"%s\" to \"%s\".\n", [[source _drt_descriptionSuitableForTestResult] UTF8String], [[dest _drt_descriptionSuitableForTestResult] UTF8String]);
}

- (void)webView:(WebView *)webView updateHistoryTitle:(NSString *)title forURL:(NSString *)url
{
    printf("WebView updated the title for history URL \"%s\" to \"%s\".\n", [[[NSURL URLWithString:url]_drt_descriptionSuitableForTestResult] UTF8String], [title UTF8String]);
}

- (void)populateVisitedLinksForWebView:(WebView *)webView
{
    if (gTestRunner->dumpVisitedLinksCallback())
        printf("Asked to populate visited links for WebView \"%s\"\n", [[[NSURL URLWithString:[webView mainFrameURL]] _drt_descriptionSuitableForTestResult] UTF8String]);
}

@end
