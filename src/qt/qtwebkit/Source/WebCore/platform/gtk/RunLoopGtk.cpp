/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RunLoop.h"

#include <gdk/gdk.h>
#include <glib.h>

#include <wtf/MainThread.h>

namespace WebCore {

RunLoop::RunLoop()
{
    // g_main_context_default() doesn't add an extra reference.
    m_runLoopContext = isMainThread() ? g_main_context_default() : adoptGRef(g_main_context_new());
    ASSERT(m_runLoopContext);
    GRefPtr<GMainLoop> innermostLoop = adoptGRef(g_main_loop_new(m_runLoopContext.get(), FALSE));
    ASSERT(innermostLoop);
    m_runLoopMainLoops.append(innermostLoop);
}

RunLoop::~RunLoop()
{
    for (int i = m_runLoopMainLoops.size() - 1; i >= 0; --i) {
        if (!g_main_loop_is_running(m_runLoopMainLoops[i].get()))
            continue;
        g_main_loop_quit(m_runLoopMainLoops[i].get());
    }
}

void RunLoop::run()
{
    RunLoop* mainRunLoop = RunLoop::current();
    GMainLoop* innermostLoop = mainRunLoop->innermostLoop();
    if (!g_main_loop_is_running(innermostLoop)) {
        g_main_loop_run(innermostLoop);
        return;
    }

    // Create and run a nested loop if the innermost one was already running.
    GMainLoop* nestedMainLoop = g_main_loop_new(0, FALSE);
    mainRunLoop->pushNestedMainLoop(nestedMainLoop);
    g_main_loop_run(nestedMainLoop);
    mainRunLoop->popNestedMainLoop();
}

GMainLoop* RunLoop::innermostLoop()
{
    // The innermost main loop should always be there.
    ASSERT(!m_runLoopMainLoops.isEmpty());
    return m_runLoopMainLoops[0].get();
}

void RunLoop::pushNestedMainLoop(GMainLoop* nestedLoop)
{
    // The innermost main loop should always be there.
    ASSERT(!m_runLoopMainLoops.isEmpty());
    m_runLoopMainLoops.append(adoptGRef(nestedLoop));
}

void RunLoop::popNestedMainLoop()
{
    // The innermost main loop should always be there.
    ASSERT(!m_runLoopMainLoops.isEmpty());
    m_runLoopMainLoops.removeLast();
}

void RunLoop::stop()
{
    // The innermost main loop should always be there.
    ASSERT(!m_runLoopMainLoops.isEmpty());
    GRefPtr<GMainLoop> lastMainLoop = m_runLoopMainLoops.last();
    if (g_main_loop_is_running(lastMainLoop.get()))
        g_main_loop_quit(lastMainLoop.get());
}

gboolean RunLoop::queueWork(RunLoop* runLoop)
{
    runLoop->performWork();
    return FALSE;
}

void RunLoop::wakeUp()
{
    GRefPtr<GSource> source = adoptGRef(g_idle_source_new());
    g_source_set_priority(source.get(), G_PRIORITY_DEFAULT);
    g_source_set_callback(source.get(), reinterpret_cast<GSourceFunc>(&RunLoop::queueWork), this, 0);
    g_source_attach(source.get(), m_runLoopContext.get());

    g_main_context_wakeup(m_runLoopContext.get());
}

RunLoop::TimerBase::TimerBase(RunLoop* runLoop)
    : m_runLoop(runLoop)
    , m_timerSource(0)
{
}

RunLoop::TimerBase::~TimerBase()
{
    stop();
}

void RunLoop::TimerBase::clearTimerSource()
{
    m_timerSource = 0;
}

gboolean RunLoop::TimerBase::timerFiredCallback(RunLoop::TimerBase* timer)
{
    GSource* currentTimerSource = timer->m_timerSource.get();
    bool isRepeating = timer->isRepeating();
    // This can change the timerSource by starting a new timer within the callback.
    if (!isRepeating && currentTimerSource == timer->m_timerSource.get())
        timer->clearTimerSource();

    timer->fired();
    return isRepeating;
}

void RunLoop::TimerBase::start(double fireInterval, bool repeat)
{
    if (m_timerSource)
        stop();

    m_timerSource = adoptGRef(g_timeout_source_new(static_cast<guint>(fireInterval * 1000)));
    m_isRepeating = repeat;
    g_source_set_callback(m_timerSource.get(), reinterpret_cast<GSourceFunc>(&RunLoop::TimerBase::timerFiredCallback), this, 0);
    g_source_attach(m_timerSource.get(), m_runLoop->m_runLoopContext.get());
}

void RunLoop::TimerBase::stop()
{
    if (!m_timerSource)
        return;

    g_source_destroy(m_timerSource.get());
    clearTimerSource();
}

bool RunLoop::TimerBase::isActive() const
{
    return m_timerSource;
}

} // namespace WebCore
