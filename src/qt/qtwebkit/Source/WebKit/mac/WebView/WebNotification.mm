/*
 * Copyright (C) 2012 Apple Computer, Inc.  All rights reserved.
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

#import "WebNotification.h"

#import "WebNotificationInternal.h"

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
#import "WebSecurityOriginInternal.h"
#import <WebCore/Notification.h>
#import <WebCore/ScriptExecutionContext.h>
#import <wtf/RefPtr.h>

using namespace WebCore;
#endif

OBJC_CLASS WebNotificationInternal;

@interface WebNotificationPrivate : NSObject
{
@public
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    RefPtr<Notification> _internal;
    uint64_t _notificationID;
#endif
}
@end

@implementation WebNotificationPrivate
@end

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
@implementation WebNotification (WebNotificationInternal)
Notification* core(WebNotification *notification)
{
    if (!notification->_private)
        return 0;
    return notification->_private->_internal.get();
}

- (id)initWithCoreNotification:(Notification*)coreNotification notificationID:(uint64_t)notificationID
{
    if (!(self = [super init]))
        return nil;
    _private = [[WebNotificationPrivate alloc] init];
    _private->_internal = coreNotification;
    _private->_notificationID = notificationID;
    return self;
}
@end
#endif

@implementation WebNotification
- (id)init
{
    return nil;
}

- (NSString *)title
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    return core(self)->title();
#else
    return nil;
#endif
}

- (NSString *)body
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    return core(self)->body();
#else
    return nil;
#endif
}

- (NSString *)tag
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    return core(self)->tag();
#else
    return nil;
#endif
}

- (NSString *)iconURL
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    return core(self)->iconURL();
#else
    return nil;
#endif
}

- (NSString *)lang
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    return core(self)->lang();
#else
    return nil;
#endif
}

- (NSString *)dir
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    return core(self)->dir();
#else
    return nil;
#endif
}

- (WebSecurityOrigin *)origin
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    return [[[WebSecurityOrigin alloc] _initWithWebCoreSecurityOrigin:core(self)->scriptExecutionContext()->securityOrigin()] autorelease];
#else
    return nil;
#endif
}

- (uint64_t)notificationID
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    return _private->_notificationID;
#else
    return 0;
#endif
}

- (void)dispatchShowEvent
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    core(self)->dispatchShowEvent();
#endif
}

- (void)dispatchCloseEvent
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    core(self)->dispatchCloseEvent();
#endif
}

- (void)dispatchClickEvent
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    core(self)->dispatchClickEvent();
#endif
}

- (void)dispatchErrorEvent
{
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    core(self)->dispatchErrorEvent();
#endif
}

@end

