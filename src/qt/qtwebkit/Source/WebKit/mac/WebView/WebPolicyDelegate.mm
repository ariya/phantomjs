/*
 * Copyright (C) 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#import "WebPolicyDelegatePrivate.h"

#import <WebCore/FrameLoaderTypes.h>
#import <wtf/ObjcRuntimeExtras.h>

using namespace WebCore;

NSString *WebActionButtonKey = @"WebActionButtonKey"; 
NSString *WebActionElementKey = @"WebActionElementKey";
NSString *WebActionFormKey = @"WebActionFormKey";
NSString *WebActionModifierFlagsKey = @"WebActionModifierFlagsKey";
NSString *WebActionNavigationTypeKey = @"WebActionNavigationTypeKey";
NSString *WebActionOriginalURLKey = @"WebActionOriginalURLKey";

@interface WebPolicyDecisionListenerPrivate : NSObject
{
@public
    id target;
    SEL action;
}

- (id)initWithTarget:(id)target action:(SEL)action;

@end

@implementation WebPolicyDecisionListenerPrivate

- (id)initWithTarget:(id)t action:(SEL)a
{
    self = [super init];
    if (!self)
        return nil;
    target = [t retain];
    action = a;
    return self;
}

- (void)dealloc
{
    [target release];
    [super dealloc];
}

@end

@implementation WebPolicyDecisionListener

- (id)_initWithTarget:(id)target action:(SEL)action
{
    self = [super init];
    if (!self)
        return nil;
    _private = [[WebPolicyDecisionListenerPrivate alloc] initWithTarget:target action:action];
    return self;
}

-(void)dealloc
{
    [_private release];
    [super dealloc];
}

- (void)_usePolicy:(PolicyAction)policy
{
    if (_private->target)
        wtfObjcMsgSend<void>(_private->target, _private->action, policy);
}

- (void)_invalidate
{
    id target = _private->target;
    _private->target = nil;
    [target release];
}

// WebPolicyDecisionListener implementation

- (void)use
{
    [self _usePolicy:PolicyUse];
}

- (void)ignore
{
    [self _usePolicy:PolicyIgnore];
}

- (void)download
{
    [self _usePolicy:PolicyDownload];
}

@end
