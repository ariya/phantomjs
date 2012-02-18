/*
 * Copyright (C) 2006, 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "config.h"
#import "SharedTimer.h"

#import <IOKit/IOMessage.h>
#import <IOKit/pwr_mgt/IOPMLib.h>
#import <wtf/Assertions.h>
#import <wtf/Noncopyable.h>
#import <wtf/PassOwnPtr.h>
#import <wtf/UnusedParam.h>

#include <stdio.h>

// On Snow Leopard and newer we'll ask IOKit to deliver notifications on a queue.
#ifdef BUILDING_ON_LEOPARD
#define IOKIT_WITHOUT_LIBDISPATCH 1
#endif

namespace WebCore {

static CFRunLoopTimerRef sharedTimer;
static void (*sharedTimerFiredFunction)();
static void timerFired(CFRunLoopTimerRef, void*);

#if !defined(IOKIT_WITHOUT_LIBDISPATCH) && defined(BUILDING_ON_SNOW_LEOPARD)
extern "C" void IONotificationPortSetDispatchQueue(IONotificationPortRef notify, dispatch_queue_t queue);
#endif

class PowerObserver {
    WTF_MAKE_NONCOPYABLE(PowerObserver);
    
public:
    static PassOwnPtr<PowerObserver> create()
    {
        return adoptPtr(new PowerObserver);
    }
    ~PowerObserver();

private:
    PowerObserver();

    static void didReceiveSystemPowerNotification(void* context, io_service_t, uint32_t messageType, void* messageArgument);
    void didReceiveSystemPowerNotification(io_service_t, uint32_t messageType, void* messageArgument);

    void restartSharedTimer();

    io_connect_t m_powerConnection;
    IONotificationPortRef m_notificationPort;
    io_object_t m_notifierReference;
#ifdef IOKIT_WITHOUT_LIBDISPATCH
    CFRunLoopSourceRef m_runLoopSource;
#else
    dispatch_queue_t m_dispatchQueue;
#endif
};

PowerObserver::PowerObserver()
    : m_powerConnection(0)
    , m_notificationPort(0)
    , m_notifierReference(0)
#ifdef IOKIT_WITHOUT_LIBDISPATCH
    , m_runLoopSource(0)    
#else
    , m_dispatchQueue(dispatch_queue_create("com.apple.WebKit.PowerObserver", 0))
#endif
{
    m_powerConnection = IORegisterForSystemPower(this, &m_notificationPort, didReceiveSystemPowerNotification, &m_notifierReference);
    if (!m_powerConnection)
        return;

#ifdef IOKIT_WITHOUT_LIBDISPATCH
    m_runLoopSource = IONotificationPortGetRunLoopSource(m_notificationPort);
    CFRunLoopAddSource(CFRunLoopGetMain(), m_runLoopSource, kCFRunLoopCommonModes);
#else
    IONotificationPortSetDispatchQueue(m_notificationPort, m_dispatchQueue);
#endif
}

PowerObserver::~PowerObserver()
{
    if (!m_powerConnection)
        return;

#ifdef IOKIT_WITHOUT_LIBDISPATCH
    CFRunLoopRemoveSource(CFRunLoopGetMain(), m_runLoopSource, kCFRunLoopCommonModes);
#else
    dispatch_release(m_dispatchQueue);
#endif

    IODeregisterForSystemPower(&m_notifierReference);
    IOServiceClose(m_powerConnection);
    IONotificationPortDestroy(m_notificationPort);
}

void PowerObserver::didReceiveSystemPowerNotification(void* context, io_service_t service, uint32_t messageType, void* messageArgument)
{
    static_cast<PowerObserver*>(context)->didReceiveSystemPowerNotification(service, messageType, messageArgument);
}

void PowerObserver::didReceiveSystemPowerNotification(io_service_t, uint32_t messageType, void* messageArgument)
{
    IOAllowPowerChange(m_powerConnection, reinterpret_cast<long>(messageArgument));

    // We only care about the "wake from sleep" message.
    if (messageType != kIOMessageSystemWillPowerOn)
        return;

#ifdef IOKIT_WITHOUT_LIBDISPATCH
    restartSharedTimer();
#else
    // We need to restart the timer on the main thread.
    CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopCommonModes, ^() {
        restartSharedTimer();
    });
#endif
}

void PowerObserver::restartSharedTimer()
{
    ASSERT(CFRunLoopGetCurrent() == CFRunLoopGetMain());

    if (!sharedTimer)
        return;

    stopSharedTimer();
    timerFired(0, 0);
}

static PowerObserver* PowerObserver;

void setSharedTimerFiredFunction(void (*f)())
{
    ASSERT(!sharedTimerFiredFunction || sharedTimerFiredFunction == f);

    sharedTimerFiredFunction = f;
}

static void timerFired(CFRunLoopTimerRef, void*)
{
    // FIXME: We can remove this global catch-all if we fix <rdar://problem/5299018>.
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    sharedTimerFiredFunction();
    [pool drain];
}

void setSharedTimerFireTime(double fireTime)
{
    ASSERT(sharedTimerFiredFunction);

    if (sharedTimer) {
        CFRunLoopTimerInvalidate(sharedTimer);
        CFRelease(sharedTimer);
    }

    CFAbsoluteTime fireDate = fireTime - kCFAbsoluteTimeIntervalSince1970;
    sharedTimer = CFRunLoopTimerCreate(0, fireDate, 0, 0, 0, timerFired, 0);
    CFRunLoopAddTimer(CFRunLoopGetCurrent(), sharedTimer, kCFRunLoopCommonModes);
    
    if (!PowerObserver)
        PowerObserver = PowerObserver::create().leakPtr();
}

void stopSharedTimer()
{
    if (sharedTimer) {
        CFRunLoopTimerInvalidate(sharedTimer);
        CFRelease(sharedTimer);
        sharedTimer = 0;
    }
}

} // namespace WebCore
