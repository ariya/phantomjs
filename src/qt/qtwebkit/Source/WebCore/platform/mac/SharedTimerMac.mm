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
#import <stdio.h>
#import <wtf/Assertions.h>
#import <wtf/Noncopyable.h>
#import <wtf/PassOwnPtr.h>

namespace WebCore {

static CFRunLoopTimerRef sharedTimer;
static void (*sharedTimerFiredFunction)();
static void timerFired(CFRunLoopTimerRef, void*);

#if !defined(IOKIT_WITHOUT_LIBDISPATCH) && !PLATFORM(IOS) && __MAC_OS_X_VERSION_MAX_ALLOWED == 1060
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
    dispatch_queue_t m_dispatchQueue;
};

PowerObserver::PowerObserver()
    : m_powerConnection(0)
    , m_notificationPort(0)
    , m_notifierReference(0)
    , m_dispatchQueue(dispatch_queue_create("com.apple.WebKit.PowerObserver", 0))
{
    m_powerConnection = IORegisterForSystemPower(this, &m_notificationPort, didReceiveSystemPowerNotification, &m_notifierReference);
    if (!m_powerConnection)
        return;

    IONotificationPortSetDispatchQueue(m_notificationPort, m_dispatchQueue);
}

PowerObserver::~PowerObserver()
{
    if (!m_powerConnection)
        return;

    dispatch_release(m_dispatchQueue);

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

    // We need to restart the timer on the main thread.
    CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopCommonModes, ^() {
        restartSharedTimer();
    });
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
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    sharedTimerFiredFunction();
    [pool drain];
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
