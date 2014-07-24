/*
 * Copyright (C) 2006, 2010, 2011, 2013 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "SharedTimer.h"

#import "WebCoreThread.h"
#import "WebCoreThreadRun.h"
#import <wtf/Assertions.h>

using namespace WebCore;

namespace WebCore {
static CFRunLoopTimerRef sharedTimer;
static void timerFired(CFRunLoopTimerRef, void*);

static void applicationDidBecomeActive(CFNotificationCenterRef, void*, CFStringRef, const void*, CFDictionaryRef)
{
    WebThreadRun(^{
        if (!sharedTimer)
            return;

        stopSharedTimer();
        timerFired(0, 0);
    });
}

typedef void (*SharedTimerFiredFunction)();
static SharedTimerFiredFunction sharedTimerFiredFunction;

void setSharedTimerFiredFunction(SharedTimerFiredFunction function)
{
    ASSERT(!sharedTimerFiredFunction || sharedTimerFiredFunction == function);

    sharedTimerFiredFunction = function;
}

static void timerFired(CFRunLoopTimerRef, void*)
{
    @autoreleasepool {
        sharedTimerFiredFunction();
    }
}

void setSharedTimerFireInterval(double interval)
{
    ASSERT(sharedTimerFiredFunction);

    if (sharedTimer) {
        CFRunLoopTimerInvalidate(sharedTimer);
        CFRelease(sharedTimer);
    }

    CFAbsoluteTime fireDate = CFAbsoluteTimeGetCurrent() + interval;
    sharedTimer = CFRunLoopTimerCreate(0, fireDate, 0, 0, 0, timerFired, 0);
    CFRunLoopAddTimer(WebThreadRunLoop(), sharedTimer, kCFRunLoopCommonModes);

    static bool registeredForApplicationNotification = false;
    if (!registeredForApplicationNotification) {
        registeredForApplicationNotification = true;
        CFNotificationCenterRef notificationCenter = CFNotificationCenterGetLocalCenter();
        CFNotificationCenterAddObserver(notificationCenter, 0, applicationDidBecomeActive, CFSTR("UIApplicationDidBecomeActiveNotification"), NULL, CFNotificationSuspensionBehaviorCoalesce);
    }
}

void stopSharedTimer()
{
    if (!sharedTimer)
        return;

    CFRunLoopTimerInvalidate(sharedTimer);
    CFRelease(sharedTimer);
    sharedTimer = 0;
}

} // namespace WebCore
