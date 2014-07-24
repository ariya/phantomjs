/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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

#import "WebNSObjectExtras.h"

#import <wtf/Assertions.h>

@interface WebMainThreadInvoker : NSProxy
{
    id target;
    id exception;
}
@end

static bool returnTypeIsObject(NSInvocation *invocation)
{
    // Could use either _C_ID or NSObjCObjectType, but it seems that neither is
    // both available and non-deprecated on all versions of Mac OS X we support.
    return strchr([[invocation methodSignature] methodReturnType], '@');
}

@implementation WebMainThreadInvoker

- (id)initWithTarget:(id)passedTarget
{
    target = passedTarget;
    return self;
}

- (void)forwardInvocation:(NSInvocation *)invocation
{
    [invocation setTarget:target];
    [invocation performSelectorOnMainThread:@selector(_webkit_invokeAndHandleException:) withObject:self waitUntilDone:YES];
    if (exception) {
        id exceptionToThrow = [exception autorelease];
        exception = nil;
        @throw exceptionToThrow;
    } else if (returnTypeIsObject(invocation)) {
        // _webkit_invokeAndHandleException retained the return value on the main thread.
        // Now autorelease it on the calling thread.
        id returnValue;
        [invocation getReturnValue:&returnValue];
        [returnValue autorelease];
    }
}

- (NSMethodSignature *)methodSignatureForSelector:(SEL)selector
{
    return [target methodSignatureForSelector:selector];
}

- (void)handleException:(id)passedException
{
    ASSERT(!exception);
    exception = [passedException retain];
}

@end

@implementation NSInvocation (WebMainThreadInvoker)

- (void)_webkit_invokeAndHandleException:(WebMainThreadInvoker *)exceptionHandler
{
    @try {
        [self invoke];
    } @catch (id exception) {
        [exceptionHandler handleException:exception];
        return;
    }
    if (returnTypeIsObject(self)) {
        // Retain the return value on the main thread.
        // -[WebMainThreadInvoker forwardInvocation:] will autorelease it on the calling thread.
        id value;
        [self getReturnValue:&value];
        [value retain];
    }
}

@end

@implementation NSObject (WebNSObjectExtras)

+ (id)_webkit_invokeOnMainThread
{
    return [[[WebMainThreadInvoker alloc] initWithTarget:self] autorelease];
}

- (id)_webkit_invokeOnMainThread
{
    return [[[WebMainThreadInvoker alloc] initWithTarget:self] autorelease];
}

@end
