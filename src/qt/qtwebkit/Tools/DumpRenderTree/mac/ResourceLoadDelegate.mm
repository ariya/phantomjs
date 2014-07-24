/*
 * Copyright (C) 2007, 2011 Apple Inc.  All rights reserved.
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

#import "config.h"
#import "ResourceLoadDelegate.h"

#import "DumpRenderTree.h"
#import "TestRunner.h"
#import <WebKit/WebKit.h>
#import <WebKit/WebTypesInternal.h>
#import <WebKit/WebDataSourcePrivate.h>
#import <wtf/Assertions.h>

using namespace std;

@interface NSURL (DRTExtras)
- (NSString *)_drt_descriptionSuitableForTestResult;
@end

@interface NSError (DRTExtras)
- (NSString *)_drt_descriptionSuitableForTestResult;
@end

@interface NSURLResponse (DRTExtras)
- (NSString *)_drt_descriptionSuitableForTestResult;
@end

@interface NSURLRequest (DRTExtras)
- (NSString *)_drt_descriptionSuitableForTestResult;
@end

@implementation NSError (DRTExtras)
- (NSString *)_drt_descriptionSuitableForTestResult 
{
    NSString *str = [NSString stringWithFormat:@"<NSError domain %@, code %ld", [self domain], static_cast<long>([self code])];
    NSURL *failingURL;

    if ((failingURL = [[self userInfo] objectForKey:@"NSErrorFailingURLKey"]))
        str = [str stringByAppendingFormat:@", failing URL \"%@\"", [failingURL _drt_descriptionSuitableForTestResult]];

    str = [str stringByAppendingFormat:@">"];

    return str;
}

@end

@implementation NSURL (DRTExtras)

- (NSString *)_drt_descriptionSuitableForTestResult 
{
    if (![self isFileURL])
        return [self absoluteString];

    WebDataSource *dataSource = [mainFrame dataSource];
    if (!dataSource)
        dataSource = [mainFrame provisionalDataSource];

    NSString *basePath = [[[[dataSource request] URL] path] stringByDeletingLastPathComponent];
    basePath = [basePath stringByAppendingString:@"/"];

    if ([[self path] hasPrefix:basePath])
        return [[self path] substringFromIndex:[basePath length]];
    return [self absoluteString];
}

@end

@implementation NSURLResponse (DRTExtras)

- (NSString *)_drt_descriptionSuitableForTestResult
{
    int statusCode = 0;
    if ([self isKindOfClass:[NSHTTPURLResponse class]])
        statusCode = [(NSHTTPURLResponse *)self statusCode];
    return [NSString stringWithFormat:@"<NSURLResponse %@, http status code %i>", [[self URL] _drt_descriptionSuitableForTestResult], statusCode];
}

@end

@implementation NSURLRequest (DRTExtras)

- (NSString *)_drt_descriptionSuitableForTestResult
{
    NSString *httpMethod = [self HTTPMethod];
    if (!httpMethod)
        httpMethod = @"(none)";
    return [NSString stringWithFormat:@"<NSURLRequest URL %@, main document URL %@, http method %@>", [[self URL] _drt_descriptionSuitableForTestResult], [[self mainDocumentURL] _drt_descriptionSuitableForTestResult], httpMethod];
}

@end

@implementation ResourceLoadDelegate

- (id)webView: (WebView *)wv identifierForInitialRequest: (NSURLRequest *)request fromDataSource: (WebDataSource *)dataSource
{
    ASSERT([[dataSource webFrame] dataSource] || [[dataSource webFrame] provisionalDataSource]);

    if (!done)
        return [[request URL] _drt_descriptionSuitableForTestResult];

    return @"<unknown>";
}

BOOL isLocalhost(NSString *host)
{
    // FIXME: Support IPv6 loopbacks.
    return NSOrderedSame == [host compare:@"127.0.0.1"] || NSOrderedSame == [host caseInsensitiveCompare:@"localhost"];
}

BOOL hostIsUsedBySomeTestsToGenerateError(NSString *host)
{
    return NSOrderedSame == [host compare:@"255.255.255.255"];
}

-(NSURLRequest *)webView: (WebView *)wv resource:identifier willSendRequest: (NSURLRequest *)request redirectResponse:(NSURLResponse *)redirectResponse fromDataSource:(WebDataSource *)dataSource
{
    if (!done && gTestRunner->dumpResourceLoadCallbacks()) {
        NSString *string = [NSString stringWithFormat:@"%@ - willSendRequest %@ redirectResponse %@", identifier, [request _drt_descriptionSuitableForTestResult],
            [redirectResponse _drt_descriptionSuitableForTestResult]];
        printf("%s\n", [string UTF8String]);
    }

    if (!done && !gTestRunner->deferMainResourceDataLoad()) {
        [dataSource _setDeferMainResourceDataLoad:false];
    }

    if (!done && gTestRunner->willSendRequestReturnsNull())
        return nil;

    if (!done && gTestRunner->willSendRequestReturnsNullOnRedirect() && redirectResponse) {
        printf("Returning null for this redirect\n");
        return nil;
    }

    NSURL *url = [request URL];
    NSString *host = [url host];
    if (host && (NSOrderedSame == [[url scheme] caseInsensitiveCompare:@"http"] || NSOrderedSame == [[url scheme] caseInsensitiveCompare:@"https"])) {
        NSString *testPathOrURL = [NSString stringWithUTF8String:gTestRunner->testPathOrURL().c_str()];
        NSString *lowercaseTestPathOrURL = [testPathOrURL lowercaseString];
        NSString *testHost = 0;
        if ([lowercaseTestPathOrURL hasPrefix:@"http:"] || [lowercaseTestPathOrURL hasPrefix:@"https:"])
            testHost = [[NSURL URLWithString:testPathOrURL] host];
        if (!isLocalhost(host) && !hostIsUsedBySomeTestsToGenerateError(host) && (!testHost || isLocalhost(testHost))) {
            printf("Blocked access to external URL %s\n", [[url absoluteString] cStringUsingEncoding:NSUTF8StringEncoding]);
            return nil;
        }
    }

    if (disallowedURLs && CFSetContainsValue(disallowedURLs, url))
        return nil;

    NSMutableURLRequest *newRequest = [request mutableCopy];
    const set<string>& clearHeaders = gTestRunner->willSendRequestClearHeaders();
    for (set<string>::const_iterator header = clearHeaders.begin(); header != clearHeaders.end(); ++header) {
        NSString *nsHeader = [[NSString alloc] initWithUTF8String:header->c_str()];
        [newRequest setValue:nil forHTTPHeaderField:nsHeader];
        [nsHeader release];
    }
    const std::string& destination = gTestRunner->redirectionDestinationForURL([[url absoluteString] UTF8String]);
    if (destination.length())
        [newRequest setURL:[NSURL URLWithString:[NSString stringWithUTF8String:destination.data()]]];

    return [newRequest autorelease];
}

- (void)webView:(WebView *)wv resource:(id)identifier didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge fromDataSource:(WebDataSource *)dataSource
{
    if (!gTestRunner->handlesAuthenticationChallenges()) {
        NSString *string = [NSString stringWithFormat:@"%@ - didReceiveAuthenticationChallenge - Simulating cancelled authentication sheet", identifier];
        printf("%s\n", [string UTF8String]);

        [[challenge sender] continueWithoutCredentialForAuthenticationChallenge:challenge];
        return;
    }
    
    const char* user = gTestRunner->authenticationUsername().c_str();
    NSString *nsUser = [NSString stringWithFormat:@"%s", user ? user : ""];

    const char* password = gTestRunner->authenticationPassword().c_str();
    NSString *nsPassword = [NSString stringWithFormat:@"%s", password ? password : ""];

    NSString *string = [NSString stringWithFormat:@"%@ - didReceiveAuthenticationChallenge - Responding with %@:%@", identifier, nsUser, nsPassword];
    printf("%s\n", [string UTF8String]);
    
    [[challenge sender] useCredential:[NSURLCredential credentialWithUser:nsUser password:nsPassword persistence:NSURLCredentialPersistenceForSession]
                              forAuthenticationChallenge:challenge];
}

- (void)webView:(WebView *)wv resource:(id)identifier didCancelAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge fromDataSource:(WebDataSource *)dataSource
{
}

-(void)webView: (WebView *)wv resource:identifier didReceiveResponse: (NSURLResponse *)response fromDataSource:(WebDataSource *)dataSource
{
    if (!done && gTestRunner->dumpResourceLoadCallbacks()) {
        NSString *string = [NSString stringWithFormat:@"%@ - didReceiveResponse %@", identifier, [response _drt_descriptionSuitableForTestResult]];
        printf("%s\n", [string UTF8String]);
    }
    if (!done && gTestRunner->dumpResourceResponseMIMETypes())
        printf("%s has MIME type %s\n", [[[[response URL] relativePath] lastPathComponent] UTF8String], [[response MIMEType] UTF8String]);
}

-(void)webView: (WebView *)wv resource:identifier didReceiveContentLength: (NSInteger)length fromDataSource:(WebDataSource *)dataSource
{
}

-(void)webView: (WebView *)wv resource:identifier didFinishLoadingFromDataSource:(WebDataSource *)dataSource
{
    if (!done && gTestRunner->dumpResourceLoadCallbacks()) {
        NSString *string = [NSString stringWithFormat:@"%@ - didFinishLoading", identifier];
        printf("%s\n", [string UTF8String]);
    }
}

-(void)webView: (WebView *)wv resource:identifier didFailLoadingWithError:(NSError *)error fromDataSource:(WebDataSource *)dataSource
{
    if (!done && gTestRunner->dumpResourceLoadCallbacks()) {
        NSString *string = [NSString stringWithFormat:@"%@ - didFailLoadingWithError: %@", identifier, [error _drt_descriptionSuitableForTestResult]];
        printf("%s\n", [string UTF8String]);
    }
}

- (void)webView: (WebView *)wv plugInFailedWithError:(NSError *)error dataSource:(WebDataSource *)dataSource
{
    // The call to -display here simulates the "Plug-in not found" sheet that Safari shows.
    // It is used for platform/mac/plugins/update-widget-from-style-recalc.html
    [wv display];
}

-(NSCachedURLResponse *) webView: (WebView *)wv resource:(id)identifier willCacheResponse:(NSCachedURLResponse *)response fromDataSource:(WebDataSource *)dataSource
{
    if (!done && gTestRunner->dumpWillCacheResponse()) {
        NSString *string = [NSString stringWithFormat:@"%@ - willCacheResponse: called", identifier];
        printf("%s\n", [string UTF8String]);
    }
    return response;
}

-(BOOL)webView: (WebView*)webView shouldPaintBrokenImageForURL:(NSURL*)imageURL
{
    // Only log the message when shouldPaintBrokenImage() returns NO; this avoids changing results of layout tests with failed
    // images, e.g., security/block-test-no-port.html.
    if (!done && gTestRunner->dumpResourceLoadCallbacks() && !gTestRunner->shouldPaintBrokenImage()) {
        NSString *string = [NSString stringWithFormat:@"%@ - shouldPaintBrokenImage: NO", [imageURL _drt_descriptionSuitableForTestResult]];
        printf("%s\n", [string UTF8String]);
    }

    return gTestRunner->shouldPaintBrokenImage();
}
@end
