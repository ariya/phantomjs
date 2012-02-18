/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
#import "ProfilerServer.h"

#import "JSProfilerPrivate.h"
#import "JSRetainPtr.h"
#import <Foundation/Foundation.h>

#if PLATFORM(IOS_SIMULATOR)
#import <Foundation/NSDistributedNotificationCenter.h>
#endif

@interface ProfilerServer : NSObject {
@private
    NSString *_serverName;
    unsigned _listenerCount;
}
+ (ProfilerServer *)sharedProfileServer;
- (void)startProfiling;
- (void)stopProfiling;
@end

@implementation ProfilerServer

+ (ProfilerServer *)sharedProfileServer
{
    static ProfilerServer *sharedServer;
    if (!sharedServer)
        sharedServer = [[ProfilerServer alloc] init];
    return sharedServer;
}

- (id)init
{
    if (!(self = [super init]))
        return nil;

    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    if ([defaults boolForKey:@"EnableJSProfiling"])
        [self startProfiling];

#if !PLATFORM(IOS) || PLATFORM(IOS_SIMULATOR)
    // FIXME: <rdar://problem/6546135>
    // The catch-all notifications
    [[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(startProfiling) name:@"ProfilerServerStartNotification" object:nil];
    [[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(stopProfiling) name:@"ProfilerServerStopNotification" object:nil];
#endif

    // The specific notifications
    NSProcessInfo *processInfo = [NSProcessInfo processInfo];
    _serverName = [[NSString alloc] initWithFormat:@"ProfilerServer-%d", [processInfo processIdentifier]];

#if !PLATFORM(IOS) || PLATFORM(IOS_SIMULATOR)
    // FIXME: <rdar://problem/6546135>
    [[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(startProfiling) name:[_serverName stringByAppendingString:@"-Start"] object:nil];
    [[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(stopProfiling) name:[_serverName stringByAppendingString:@"-Stop"] object:nil];
#endif

    [pool drain];

    return self;
}

- (void)startProfiling
{
    if (++_listenerCount > 1)
        return;
    JSRetainPtr<JSStringRef> profileName(Adopt, JSStringCreateWithUTF8CString([_serverName UTF8String]));
    JSStartProfiling(0, profileName.get());
}

- (void)stopProfiling
{
    if (!_listenerCount || --_listenerCount > 0)
        return;
    JSRetainPtr<JSStringRef> profileName(Adopt, JSStringCreateWithUTF8CString([_serverName UTF8String]));
    JSEndProfiling(0, profileName.get());
}

@end

namespace JSC {

void startProfilerServerIfNeeded()
{
    [ProfilerServer sharedProfileServer];
}

} // namespace JSC
