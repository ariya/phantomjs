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

#import <WebKit/WebPanelAuthenticationHandler.h>

#import <Foundation/NSURLAuthenticationChallenge.h>
#import <WebKit/WebAuthenticationPanel.h>
#import <wtf/Assertions.h>

static NSString *WebModalDialogPretendWindow = @"WebModalDialogPretendWindow";

@implementation WebPanelAuthenticationHandler

WebPanelAuthenticationHandler *sharedHandler;

+ (id)sharedHandler
{
    if (sharedHandler == nil)
        sharedHandler = [[self alloc] init];
    return sharedHandler;
}

-(id)init
{
    self = [super init];
    if (self != nil) {
        windowToPanel = [[NSMapTable alloc] initWithKeyOptions:NSPointerFunctionsStrongMemory valueOptions:NSPointerFunctionsStrongMemory capacity:0];
        challengeToWindow = [[NSMapTable alloc] initWithKeyOptions:NSPointerFunctionsStrongMemory valueOptions:NSPointerFunctionsStrongMemory capacity:0];
        windowToChallengeQueue = [[NSMapTable alloc] initWithKeyOptions:NSPointerFunctionsStrongMemory valueOptions:NSPointerFunctionsStrongMemory capacity:0];
    }

    return self;
}

-(void)dealloc
{
    [windowToPanel release];
    [challengeToWindow release];    
    [windowToChallengeQueue release];    
    [super dealloc];
}

-(void)enqueueChallenge:(NSURLAuthenticationChallenge *)challenge forWindow:(id)window
{
    NSMutableArray *queue = [windowToChallengeQueue objectForKey:window];
    if (!queue) {
        queue = [[NSMutableArray alloc] init];
        [windowToChallengeQueue setObject:queue forKey:window];
        [queue release];
    }
    [queue addObject:challenge];
}

-(void)tryNextChallengeForWindow:(id)window
{
    NSMutableArray *queue = [windowToChallengeQueue objectForKey:window];
    if (!queue)
        return;

    NSURLAuthenticationChallenge *challenge = [[queue objectAtIndex:0] retain];
    [queue removeObjectAtIndex:0];
    if (![queue count])
        [windowToChallengeQueue removeObjectForKey:window];

    NSURLCredential *latestCredential = [[NSURLCredentialStorage sharedCredentialStorage] defaultCredentialForProtectionSpace:[challenge protectionSpace]];

    if ([latestCredential hasPassword]) {
        [[challenge sender] useCredential:latestCredential forAuthenticationChallenge:challenge];
        [challenge release];
        return;
    }
                                                                    
    [self startAuthentication:challenge window:(window == WebModalDialogPretendWindow ? nil : window)];
    [challenge release];
}


-(void)startAuthentication:(NSURLAuthenticationChallenge *)challenge window:(NSWindow *)w
{
    id window = w ? (id)w : (id)WebModalDialogPretendWindow;

    if ([windowToPanel objectForKey:window] != nil) {
        [self enqueueChallenge:challenge forWindow:window];
        return;
    }

    // In this case, we have an attached sheet that's not one of our
    // authentication panels, so enqueing is not an option. Just
    // cancel loading instead, since this case is fairly
    // unlikely (how would you be loading a page if you had an error
    // sheet up?)
    if ([w attachedSheet] != nil) {
        [[challenge sender] cancelAuthenticationChallenge:challenge];
        return;
    }

    WebAuthenticationPanel *panel = [[WebAuthenticationPanel alloc] initWithCallback:self selector:@selector(_authenticationDoneWithChallenge:result:)];
    [challengeToWindow setObject:window forKey:challenge];
    [windowToPanel setObject:panel forKey:window];
    [panel release];
    
    if (window == WebModalDialogPretendWindow)
        [panel runAsModalDialogWithChallenge:challenge];
    else
        [panel runAsSheetOnWindow:window withChallenge:challenge];
}

-(void)cancelAuthentication:(NSURLAuthenticationChallenge *)challenge
{
    id window = [challengeToWindow objectForKey:challenge];
    if (!window)
        return;

    WebAuthenticationPanel *panel = [windowToPanel objectForKey:window];
    [panel cancel:self];
}

-(void)_authenticationDoneWithChallenge:(NSURLAuthenticationChallenge *)challenge result:(NSURLCredential *)credential
{
    id window = [challengeToWindow objectForKey:challenge];
    [window retain];
    if (window != nil) {
        [windowToPanel removeObjectForKey:window];
        [challengeToWindow removeObjectForKey:challenge];
    }

    if (credential == nil) {
        [[challenge sender] continueWithoutCredentialForAuthenticationChallenge:challenge];
    } else {
        [[challenge sender] useCredential:credential forAuthenticationChallenge:challenge];
    }

    [self tryNextChallengeForWindow:window];
    [window release];
}

@end
