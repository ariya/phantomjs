/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include "config.h"
#include "GCActivityCallback.h"

#include "APIShims.h"
#include "Heap.h"
#include "JSGlobalData.h"
#include "JSLock.h"
#include "JSObject.h"
#include "ScopeChain.h"
#include <wtf/RetainPtr.h>
#include <wtf/WTFThreadData.h>

#if !USE(CF)
#error "This file should only be used on CF platforms."
#endif

namespace JSC {

struct DefaultGCActivityCallbackPlatformData {
    static void trigger(CFRunLoopTimerRef, void *info);

    RetainPtr<CFRunLoopTimerRef> timer;
    RetainPtr<CFRunLoopRef> runLoop;
    CFRunLoopTimerContext context;
};

const CFTimeInterval decade = 60 * 60 * 24 * 365 * 10;
const CFTimeInterval triggerInterval = 2; // seconds

void DefaultGCActivityCallbackPlatformData::trigger(CFRunLoopTimerRef timer, void *info)
{
    Heap* heap = static_cast<Heap*>(info);
    APIEntryShim shim(heap->globalData());
    heap->collectAllGarbage();
    CFRunLoopTimerSetNextFireDate(timer, CFAbsoluteTimeGetCurrent() + decade);
}

DefaultGCActivityCallback::DefaultGCActivityCallback(Heap* heap)
{
    commonConstructor(heap, CFRunLoopGetCurrent());
}

DefaultGCActivityCallback::DefaultGCActivityCallback(Heap* heap, CFRunLoopRef runLoop)
{
    commonConstructor(heap, runLoop);
}

DefaultGCActivityCallback::~DefaultGCActivityCallback()
{
    CFRunLoopRemoveTimer(d->runLoop.get(), d->timer.get(), kCFRunLoopCommonModes);
    CFRunLoopTimerInvalidate(d->timer.get());
    d->context.info = 0;
    d->runLoop = 0;
    d->timer = 0;
}

void DefaultGCActivityCallback::commonConstructor(Heap* heap, CFRunLoopRef runLoop)
{
    d = adoptPtr(new DefaultGCActivityCallbackPlatformData);

    memset(&d->context, 0, sizeof(CFRunLoopTimerContext));
    d->context.info = heap;
    d->runLoop = runLoop;
    d->timer.adoptCF(CFRunLoopTimerCreate(0, decade, decade, 0, 0, DefaultGCActivityCallbackPlatformData::trigger, &d->context));
    CFRunLoopAddTimer(d->runLoop.get(), d->timer.get(), kCFRunLoopCommonModes);
}

void DefaultGCActivityCallback::operator()()
{
    CFRunLoopTimerSetNextFireDate(d->timer.get(), CFAbsoluteTimeGetCurrent() + triggerInterval);
}

void DefaultGCActivityCallback::synchronize()
{
    if (CFRunLoopGetCurrent() == d->runLoop.get())
        return;
    CFRunLoopRemoveTimer(d->runLoop.get(), d->timer.get(), kCFRunLoopCommonModes);
    d->runLoop = CFRunLoopGetCurrent();
    CFRunLoopAddTimer(d->runLoop.get(), d->timer.get(), kCFRunLoopCommonModes);
}

}
