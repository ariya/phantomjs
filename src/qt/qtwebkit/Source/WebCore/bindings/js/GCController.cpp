/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
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

#include "config.h"
#include "GCController.h"

#include "JSDOMWindow.h"
#include <runtime/VM.h>
#include <runtime/JSLock.h>
#include <heap/Heap.h>
#include <wtf/StdLibExtras.h>

using namespace JSC;

namespace WebCore {

static void collect(void*)
{
    JSLockHolder lock(JSDOMWindow::commonVM());
    JSDOMWindow::commonVM()->heap.collectAllGarbage();
}

GCController& gcController()
{
    DEFINE_STATIC_LOCAL(GCController, staticGCController, ());
    return staticGCController;
}

GCController::GCController()
#if !USE(CF) && !PLATFORM(BLACKBERRY) && !PLATFORM(QT)
    : m_GCTimer(this, &GCController::gcTimerFired)
#endif
{
}

void GCController::garbageCollectSoon()
{
    // We only use reportAbandonedObjectGraph on systems with CoreFoundation 
    // since it uses a runloop-based timer that is currently only available on 
    // systems with CoreFoundation. If and when the notion of a run loop is pushed 
    // down into WTF so that more platforms can take advantage of it, we will be 
    // able to use reportAbandonedObjectGraph on more platforms.
#if USE(CF) || PLATFORM(BLACKBERRY) || PLATFORM(QT)
    JSLockHolder lock(JSDOMWindow::commonVM());
    JSDOMWindow::commonVM()->heap.reportAbandonedObjectGraph();
#else
    if (!m_GCTimer.isActive())
        m_GCTimer.startOneShot(0);
#endif
}

#if !USE(CF) && !PLATFORM(BLACKBERRY) && !PLATFORM(QT)
void GCController::gcTimerFired(Timer<GCController>*)
{
    collect(0);
}
#endif

void GCController::garbageCollectNow()
{
    JSLockHolder lock(JSDOMWindow::commonVM());
    if (!JSDOMWindow::commonVM()->heap.isBusy())
        JSDOMWindow::commonVM()->heap.collectAllGarbage();
}

void GCController::garbageCollectOnAlternateThreadForDebugging(bool waitUntilDone)
{
    ThreadIdentifier threadID = createThread(collect, 0, "WebCore: GCController");

    if (waitUntilDone) {
        waitForThreadCompletion(threadID);
        return;
    }

    detachThread(threadID);
}

void GCController::setJavaScriptGarbageCollectorTimerEnabled(bool enable)
{
    JSDOMWindow::commonVM()->heap.setGarbageCollectionTimerEnabled(enable);
}

void GCController::discardAllCompiledCode()
{
    JSLockHolder lock(JSDOMWindow::commonVM());
    JSDOMWindow::commonVM()->discardAllCode();
}

} // namespace WebCore
