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

#import "WebKitLauncherURLProtocol.h"
#import "WebKitNightlyEnabler.h"

#if ENABLE_SPARKLE
#import <AppKit/AppKit.h>
#import <Sparkle/Sparkle.h>
#endif

@interface NSObject (WKBrowsingContextControllerMethods)
+ (void)registerSchemeForCustomProtocol:(NSString *)scheme;
@end

@interface WebKitLauncherURLProtocol (ImplementationDetails)
-(void)handleIsWebKitLauncherAvailableJS;
-(void)handleCheckForUpdates;
-(void)resourceNotFound;
@end

@implementation WebKitLauncherURLProtocol

+(void)load
{
    [NSClassFromString(@"WKBrowsingContextController") registerSchemeForCustomProtocol:@"x-webkit-launcher"];
    [NSURLProtocol registerClass:self];
}

+(BOOL)canInitWithRequest:(NSURLRequest *)request
{
    if (![[[request URL] scheme] isEqualToString:@"x-webkit-launcher"])
        return NO;

    NSURL *mainDocumentURL = [request mainDocumentURL];
    if (!mainDocumentURL)
        return NO;

    if ([[mainDocumentURL scheme] isEqualToString:@"file"])
        return YES;

    NSString *mainDocumentHost = [mainDocumentURL host];
    if (![mainDocumentHost isEqualToString:@"webkit.org"] && ![mainDocumentHost hasSuffix:@".webkit.org"])
        return NO;

    return YES;
}

+(NSURLRequest *)canonicalRequestForRequest:(NSURLRequest *)request
{
    return request;
}

-(void)startLoading
{
    NSURLRequest *request = [self request];
    NSString *resourceSpecifier = [[request URL] resourceSpecifier];
    if ([resourceSpecifier isEqualToString:@"is-x-webkit-launcher-available.js"]) {
        [self handleIsWebKitLauncherAvailableJS];
        return;
    }
    if ([resourceSpecifier isEqualToString:@"webkit-version-information.js"]) {
        [self handleWebKitVersionInformation];
        return;
    }
#if ENABLE_SPARKLE
    if ([resourceSpecifier isEqualToString:@"check-for-updates"]) {
        [self handleCheckForUpdates];
        return;
    }
#endif
    [self resourceNotFound];
}

-(void)stopLoading
{
}

-(void)handleIsWebKitLauncherAvailableJS
{
    id client = [self client];
    NSURLResponse *response = [[NSURLResponse alloc] initWithURL:[[self request] URL] MIMEType:@"text/javascript" expectedContentLength:0 textEncodingName:@"utf-8"];
    [client URLProtocol:self didReceiveResponse:response cacheStoragePolicy:NSURLCacheStorageAllowed];
    [response release];

    NSData *data = [@"var isWebKitLauncherAvailable = true;" dataUsingEncoding:NSUTF8StringEncoding];
    [client URLProtocol:self didLoadData:data];
    [client URLProtocolDidFinishLoading:self];
}

-(void)handleWebKitVersionInformation
{
    id client = [self client];
    NSURLResponse *response = [[NSURLResponse alloc] initWithURL:[[self request] URL] MIMEType:@"text/javascript" expectedContentLength:0 textEncodingName:@"utf-8"];
    [client URLProtocol:self didReceiveResponse:response cacheStoragePolicy:NSURLCacheStorageAllowed];
    [response release];

    NSBundle *bundle = webKitLauncherBundle();
    int revision = [[[bundle infoDictionary] valueForKey:(NSString *)kCFBundleVersionKey] intValue];
    NSString *branch = [NSString stringWithContentsOfURL:[bundle URLForResource:@"BRANCH" withExtension:nil] encoding:NSUTF8StringEncoding error:0];
    branch = [branch stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
    if (!branch)
        branch = @"trunk";

    NSString *script = [NSString stringWithFormat:@"var webKitRevision = %d; var webKitBranch = \"%@\";", revision, branch];
    NSData *data = [script dataUsingEncoding:NSUTF8StringEncoding];
    [client URLProtocol:self didLoadData:data];
    [client URLProtocolDidFinishLoading:self];
}

#if ENABLE_SPARKLE
-(void)handleCheckForUpdates
{
    id client = [self client];
    NSURLResponse *response = [[NSURLResponse alloc] initWithURL:[[self request] URL] MIMEType:@"text/plain" expectedContentLength:0 textEncodingName:@"utf-8"];
    [client URLProtocol:self didReceiveResponse:response cacheStoragePolicy:NSURLCacheStorageNotAllowed];
    [response release];

    SUUpdater *updater = [SUUpdater updaterForBundle:webKitLauncherBundle()];
    [updater performSelectorOnMainThread:@selector(checkForUpdates:) withObject:self waitUntilDone:NO];
    [client URLProtocolDidFinishLoading:self];
}
#endif

-(void)resourceNotFound
{
    id client = [self client];
    NSDictionary *infoDictionary = [NSDictionary dictionaryWithObject:NSURLErrorFailingURLStringErrorKey forKey:[[self request] URL]];
    NSError *error = [NSError errorWithDomain:NSURLErrorDomain code:NSURLErrorFileDoesNotExist userInfo:infoDictionary];
    [client URLProtocol:self didFailWithError:error];
}

@end
