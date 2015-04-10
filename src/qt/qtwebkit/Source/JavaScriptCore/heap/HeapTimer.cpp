/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#include "config.h"
#include "HeapTimer.h"

#include "APIShims.h"
#include "JSObject.h"
#include "JSString.h"

#include <wtf/MainThread.h>
#include <wtf/Threading.h>

#if PLATFORM(QT)
#include <QCoreApplication>
#include <QMutexLocker>
#include <QThread>
#include <QTimerEvent>
#elif PLATFORM(EFL)
#include <Ecore.h>
#endif

namespace JSC {

#if USE(CF)
    
const CFTimeInterval HeapTimer::s_decade = 60 * 60 * 24 * 365 * 10;

static const void* retainAPILock(const void* info)
{
    static_cast<JSLock*>(const_cast<void*>(info))->ref();
    return info;
}

static void releaseAPILock(const void* info)
{
    static_cast<JSLock*>(const_cast<void*>(info))->deref();
}

HeapTimer::HeapTimer(VM* vm, CFRunLoopRef runLoop)
    : m_vm(vm)
    , m_runLoop(runLoop)
{
    memset(&m_context, 0, sizeof(CFRunLoopTimerContext));
    m_context.info = &vm->apiLock();
    m_context.retain = retainAPILock;
    m_context.release = releaseAPILock;
    m_timer = adoptCF(CFRunLoopTimerCreate(0, s_decade, s_decade, 0, 0, HeapTimer::timerDidFire, &m_context));
    CFRunLoopAddTimer(m_runLoop.get(), m_timer.get(), kCFRunLoopCommonModes);
}

HeapTimer::~HeapTimer()
{
    CFRunLoopRemoveTimer(m_runLoop.get(), m_timer.get(), kCFRunLoopCommonModes);
    CFRunLoopTimerInvalidate(m_timer.get());
}

void HeapTimer::timerDidFire(CFRunLoopTimerRef timer, void* context)
{
    JSLock* apiLock = static_cast<JSLock*>(context);
    apiLock->lock();

    VM* vm = apiLock->vm();
    // The VM has been destroyed, so we should just give up.
    if (!vm) {
        apiLock->unlock();
        return;
    }

    HeapTimer* heapTimer = 0;
    if (vm->heap.activityCallback()->m_timer.get() == timer)
        heapTimer = vm->heap.activityCallback();
    else if (vm->heap.sweeper()->m_timer.get() == timer)
        heapTimer = vm->heap.sweeper();
    else
        RELEASE_ASSERT_NOT_REACHED();

    {
        APIEntryShim shim(vm);
        heapTimer->doWork();
    }

    apiLock->unlock();
}

#elif PLATFORM(BLACKBERRY)

HeapTimer::HeapTimer(VM* vm)
    : m_vm(vm)
    , m_timer(this, &HeapTimer::timerDidFire)
{
    // FIXME: Implement HeapTimer for other threads.
    if (WTF::isMainThread() && !m_timer.tryCreateClient())
        CRASH();
}

HeapTimer::~HeapTimer()
{
}

void HeapTimer::timerDidFire()
{
    doWork();
}

void HeapTimer::invalidate()
{
}

#elif PLATFORM(QT)

HeapTimer::HeapTimer(VM* vm)
    : m_vm(vm)
    , m_newThread(0)
    , m_mutex(QMutex::NonRecursive)
{
    // The HeapTimer might be created before the runLoop is started,
    // but we need to ensure the thread has an eventDispatcher already.
    QEventLoop fakeLoop(this);
}

HeapTimer::~HeapTimer()
{
    QMutexLocker lock(&m_mutex);
    m_timer.stop();
}

void HeapTimer::timerEvent(QTimerEvent*)
{
    QMutexLocker lock(&m_mutex);
    if (m_newThread) {
        // We need to wait with processing until we are on the right thread.
        return;
    }

    APIEntryShim shim(m_vm);
    doWork();
}

void HeapTimer::customEvent(QEvent*)
{
    ASSERT(m_newThread);
    QMutexLocker lock(&m_mutex);
    moveToThread(m_newThread);
    m_newThread = 0;
}

#elif PLATFORM(EFL)

HeapTimer::HeapTimer(VM* vm)
    : m_vm(vm)
    , m_timer(0)
{
}

HeapTimer::~HeapTimer()
{
    stop();
}

Ecore_Timer* HeapTimer::add(double delay, void* agent)
{
    return ecore_timer_add(delay, reinterpret_cast<Ecore_Task_Cb>(timerEvent), agent);
}
    
void HeapTimer::stop()
{
    if (!m_timer)
        return;

    ecore_timer_del(m_timer);
    m_timer = 0;
}

bool HeapTimer::timerEvent(void* info)
{
    HeapTimer* agent = static_cast<HeapTimer*>(info);
    
    APIEntryShim shim(agent->m_vm);
    agent->doWork();
    agent->m_timer = 0;
    
    return ECORE_CALLBACK_CANCEL;
}

#else
HeapTimer::HeapTimer(VM* vm)
    : m_vm(vm)
{
}

HeapTimer::~HeapTimer()
{
}

void HeapTimer::invalidate()
{
}

#endif
    

} // namespace JSC
