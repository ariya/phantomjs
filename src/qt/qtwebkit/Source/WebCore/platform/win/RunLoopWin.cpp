/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include "WindowsExtras.h"
#include <wtf/CurrentTime.h>

using namespace std;

namespace WebCore {

static const UINT PerformWorkMessage = WM_USER + 1;
static const LPWSTR kRunLoopMessageWindowClassName = L"RunLoopMessageWindow";

LRESULT CALLBACK RunLoop::RunLoopWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (RunLoop* runLoop = static_cast<RunLoop*>(getWindowPointer(hWnd, 0)))
        return runLoop->wndProc(hWnd, message, wParam, lParam);

    if (message == WM_CREATE) {
        LPCREATESTRUCT createStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);

        // Associate the RunLoop with the window.
        setWindowPointer(hWnd, 0, createStruct->lpCreateParams);
        return 0;
    }

    return ::DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT RunLoop::wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case PerformWorkMessage:
        performWork();
        return 0;
    case WM_TIMER:
        RunLoop::TimerBase::timerFired(this, wParam);
        return 0;
    }

    return ::DefWindowProc(hWnd, message, wParam, lParam);
}

void RunLoop::run()
{
    MSG message;
    while (BOOL result = ::GetMessage(&message, 0, 0, 0)) {
        if (result == -1)
            break;
        ::TranslateMessage(&message);
        ::DispatchMessage(&message);
    }
}

void RunLoop::stop()
{
    ::PostQuitMessage(0);
}

bool RunLoop::registerRunLoopMessageWindowClass()
{
    // FIXME: This really only needs to be called once.

    WNDCLASS windowClass = { 0 };
    windowClass.lpfnWndProc     = RunLoop::RunLoopWndProc;
    windowClass.cbWndExtra      = sizeof(RunLoop*);
    windowClass.lpszClassName   = kRunLoopMessageWindowClassName;

    return !!::RegisterClass(&windowClass);
}

RunLoop::RunLoop()
{
    registerRunLoopMessageWindowClass();

    m_runLoopMessageWindow = ::CreateWindow(kRunLoopMessageWindowClassName, 0, 0,
                                            CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
                                            HWND_MESSAGE, 0, 0, this);
    ASSERT(::IsWindow(m_runLoopMessageWindow));
}

RunLoop::~RunLoop()
{
    // FIXME: Tear down the work item queue here.
}

void RunLoop::wakeUp()
{
    // FIXME: No need to wake up the run loop if we've already called dispatch
    // before the run loop has had the time to respond.
    ::PostMessage(m_runLoopMessageWindow, PerformWorkMessage, reinterpret_cast<WPARAM>(this), 0);
}

// RunLoop::Timer

void RunLoop::TimerBase::timerFired(RunLoop* runLoop, uint64_t ID)
{
    TimerMap::iterator it = runLoop->m_activeTimers.find(ID);
    if (it == runLoop->m_activeTimers.end()) {
        // The timer must have been stopped after the WM_TIMER message was posted to the message queue.
        return;
    }

    TimerBase* timer = it->value;

    if (!timer->m_isRepeating) {
        runLoop->m_activeTimers.remove(it);
        ::KillTimer(runLoop->m_runLoopMessageWindow, ID);
    }

    timer->fired();
}

static uint64_t generateTimerID()
{
    static uint64_t uniqueTimerID = 1;
    return uniqueTimerID++;
}

RunLoop::TimerBase::TimerBase(RunLoop* runLoop)
    : m_runLoop(runLoop)
    , m_ID(generateTimerID())
    , m_isRepeating(false)
{
}

RunLoop::TimerBase::~TimerBase()
{
    stop();
}

void RunLoop::TimerBase::start(double nextFireInterval, bool repeat)
{
    m_isRepeating = repeat;
    m_runLoop->m_activeTimers.set(m_ID, this);
    ::SetTimer(m_runLoop->m_runLoopMessageWindow, m_ID, nextFireInterval * 1000, 0);
}

void RunLoop::TimerBase::stop()
{
    TimerMap::iterator it = m_runLoop->m_activeTimers.find(m_ID);
    if (it == m_runLoop->m_activeTimers.end())
        return;

    m_runLoop->m_activeTimers.remove(it);
    ::KillTimer(m_runLoop->m_runLoopMessageWindow, m_ID);
}

bool RunLoop::TimerBase::isActive() const
{
    return m_runLoop->m_activeTimers.contains(m_ID);
}

} // namespace WebCore
