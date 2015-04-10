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

#import "WebPluginContainerCheck.h"

#import "WebFrameInternal.h"
#import "WebPluginContainerPrivate.h"
#import "WebPluginController.h"
#import "WebPolicyDelegatePrivate.h"
#import "WebView.h"
#import "WebViewInternal.h"
#import <Foundation/NSDictionary.h>
#import <Foundation/NSURL.h>
#import <Foundation/NSURLRequest.h>
#import <WebCore/Document.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameLoader.h>
#import <WebCore/FrameLoaderTypes.h>
#import <WebCore/SecurityOrigin.h>
#import <wtf/Assertions.h>
#import <wtf/ObjcRuntimeExtras.h>

using namespace WebCore;

@implementation WebPluginContainerCheck

- (id)initWithRequest:(NSURLRequest *)request target:(NSString *)target resultObject:(id)obj selector:(SEL)selector controller:(id <WebPluginContainerCheckController>)controller contextInfo:(id)contextInfo /*optional*/
{
    if (!(self = [super init]))
        return nil;
    
    _request = [request copy];
    _target = [target copy];
    _resultObject = [obj retain];
    _resultSelector = selector;
    _contextInfo = [contextInfo retain];
    
    // controller owns us so don't retain, to avoid cycle
    _controller = controller;
    
    return self;
}

+ (id)checkWithRequest:(NSURLRequest *)request target:(NSString *)target resultObject:(id)obj selector:(SEL)selector controller:(id <WebPluginContainerCheckController>)controller contextInfo:(id)contextInfo /*optional*/
{
    return [[[self alloc] initWithRequest:request target:target resultObject:obj selector:selector controller:controller contextInfo:contextInfo] autorelease];
}

- (void)finalize
{
    // mandatory to complete or cancel before releasing this object
    ASSERT(_done);
    [super finalize];
}

- (void)dealloc
{
    // mandatory to complete or cancel before releasing this object
    ASSERT(_done);
    [super dealloc];
}

- (void)_continueWithPolicy:(PolicyAction)policy
{
    if (_contextInfo)
        wtfObjcMsgSend<void>(_resultObject, _resultSelector, (policy == PolicyUse), _contextInfo);
    else     
        wtfObjcMsgSend<void>(_resultObject, _resultSelector, (policy == PolicyUse));

    // this will call indirectly call cancel
    [_controller _webPluginContainerCancelCheckIfAllowedToLoadRequest:self];
}

- (BOOL)_isForbiddenFileLoad
{
   Frame* coreFrame = core([_controller webFrame]);
   ASSERT(coreFrame);
   if (!coreFrame->document()->securityOrigin()->canDisplay([_request URL])) {
       [self _continueWithPolicy:PolicyIgnore];
       return YES;
   }

   return NO;
}

- (NSDictionary *)_actionInformationWithURL:(NSURL *)URL
{
    return [NSDictionary dictionaryWithObjectsAndKeys:
               [NSNumber numberWithInt:WebNavigationTypePlugInRequest], WebActionNavigationTypeKey,
               [NSNumber numberWithInt:0], WebActionModifierFlagsKey,
               URL, WebActionOriginalURLKey,
               nil];
}

- (void)_askPolicyDelegate
{
    WebView *webView = [_controller webView];

    WebFrame *targetFrame;
    if ([_target length] > 0) {
        targetFrame = [[_controller webFrame] findFrameNamed:_target];
    } else {
        targetFrame = [_controller webFrame];
    }

    NSDictionary *action = [self _actionInformationWithURL:[_request URL]];

    _listener = [[WebPolicyDecisionListener alloc] _initWithTarget:self action:@selector(_continueWithPolicy:)];

    if (targetFrame == nil) {
        // would open new window
        [[webView _policyDelegateForwarder] webView:webView
                     decidePolicyForNewWindowAction:action
                                            request:_request
                                       newFrameName:_target
                                   decisionListener:_listener];
    } else {
        // would target existing frame
        [[webView _policyDelegateForwarder] webView:webView
                    decidePolicyForNavigationAction:action
                                            request:_request
                                              frame:targetFrame
                                   decisionListener:_listener];        
    }
}

- (void)start
{
    ASSERT(!_listener);
    ASSERT(!_done);

    if ([self _isForbiddenFileLoad])
        return;

    [self _askPolicyDelegate];
}

- (void)cancel
{
    if (_done)
        return;

    [_request release];
    _request = nil;
    
    [_target release];
    _target = nil;

    [_listener _invalidate];
    [_listener release];
    _listener = nil;

    [_resultObject autorelease];
    _resultObject = nil;

    _controller = nil;
    
    [_contextInfo release];
    _contextInfo = nil;

    _done = YES;
}

- (id)contextInfo
{
    return _contextInfo;   
}

@end
