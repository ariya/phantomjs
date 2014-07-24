/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
#import "NavigationController.h"

#import <WebKit/WebFrame.h>
#import <WebKit/WebScriptObject.h>


@implementation NavigationController
+ (BOOL)isSelectorExcludedFromWebScript:(SEL)selector
{
    if (selector == @selector(evaluateWebScript:afterBackForwardNavigation:))
        return NO;
    return YES;
}

+ (NSString *)webScriptNameForSelector:(SEL)selector
{
    if (selector == @selector(evaluateWebScript:afterBackForwardNavigation:))
        return @"evalAfterBackForwardNavigation";
    return nil;
}

- (void)setPendingScript:(NSString *)script
{
    if (script != pendingScript) {
        [pendingScript release];
        pendingScript = [script copy];
    }
}

- (void)setPendingRequest:(NSURLRequest *)request
{
    if (request != pendingRequest) {
        [pendingRequest release];
        pendingRequest = [request copy];
    }
}

- (void)evaluateWebScript:(NSString *)script afterBackForwardNavigation:(NSString *)navigation
{
    // Allow both arguments to be optional
    if (![script isKindOfClass:[NSString class]])
        script = @"";
    if (![navigation isKindOfClass:[NSString class]])
        navigation = @"about:blank";
    
    [self setPendingScript:script];
    [self setPendingRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:navigation]]];
    pendingAction = Load;
}

- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame
{
    if (frame == [[frame webView] mainFrame]) {
        switch (pendingAction) {
            case Load:
                pendingAction = GoBack;
                [frame loadRequest:pendingRequest];
                [self setPendingRequest:nil];
                break;
            case GoBack:
                pendingAction = ExecuteScript;
                [[frame webView] goBack];
                break;
            case ExecuteScript:
                pendingAction = None;
                [[[frame webView] windowScriptObject] evaluateWebScript:pendingScript];
                [self setPendingScript:nil];
                break;
            case None:
            default:
                break;
        }
    }
}

- (void)dealloc
{
    [self setPendingScript:nil];
    [self setPendingRequest:nil];
    [super dealloc];
}
@end

