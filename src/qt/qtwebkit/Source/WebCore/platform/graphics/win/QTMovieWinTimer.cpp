/*
 * Copyright (C) 2006, 2007, 2009 Apple Computer, Inc.  All rights reserved.
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

#include <windows.h>

#include "QTMovieWinTimer.h"

#include <wtf/Assertions.h>

// This file is used by the QuickTime access DLL. It copies some WebCore code 
// which can't be used directly due to dependency issues.

// FIXME: Find a way to do timers that can manage <10ms resolution properly too.

static UINT_PTR timerID;
static void (*sharedTimerFiredFunction)();

static HINSTANCE instanceHandle = 0;

static HWND timerWindowHandle = 0;
static UINT timerFiredMessage = 0;
static const LPCWSTR kTimerWindowClassName = L"TimerWindowClass";
static bool processingCustomTimerMessage = false;

static LRESULT CALLBACK TimerWindowWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == timerFiredMessage) {
        processingCustomTimerMessage = true;
        sharedTimerFiredFunction();
        processingCustomTimerMessage = false;
    } else if (message == WM_TIMER && wParam == timerID) {
        stopSharedTimer();
        sharedTimerFiredFunction();
    } else
        return DefWindowProc(hWnd, message, wParam, lParam);
    return 0;
}

static void initializeOffScreenTimerWindow()
{
    if (timerWindowHandle)
        return;
    
    WNDCLASSEX wcex;
    memset(&wcex, 0, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc    = TimerWindowWndProc;
    wcex.hInstance      = instanceHandle;
    wcex.lpszClassName  = kTimerWindowClassName;
    RegisterClassEx(&wcex);

    timerWindowHandle = CreateWindow(kTimerWindowClassName, 0, 0,
       CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, HWND_MESSAGE, 0, instanceHandle, 0);
    timerFiredMessage = RegisterWindowMessage(L"com.apple.WebKit.TimerFired");
}

void setSharedTimerFiredFunction(void (*f)())
{
    sharedTimerFiredFunction = f;
}

void setSharedTimerFireDelay(double interval)
{
    ASSERT(sharedTimerFiredFunction);

    unsigned intervalInMS;
    if (interval < 0)
        intervalInMS = 0;
    else {
        interval *= 1000;
        if (interval > USER_TIMER_MAXIMUM)
            intervalInMS = USER_TIMER_MAXIMUM;
        else
            intervalInMS = (unsigned)interval;
    }

    stopSharedTimer();
    initializeOffScreenTimerWindow();

    // We don't allow nested PostMessages, since the custom messages will effectively starve
    // painting and user input. (Win32 has a tri-level queue with application messages > 
    // user input > WM_PAINT/WM_TIMER.)
    // In addition, if the queue contains input events that have been there since the last call to
    // GetQueueStatus, PeekMessage or GetMessage we favor timers.
    if (intervalInMS < USER_TIMER_MINIMUM 
        && !processingCustomTimerMessage 
        && !LOWORD(::GetQueueStatus(QS_ALLINPUT))) {
        // Windows SetTimer does not allow timeouts smaller than 10ms (USER_TIMER_MINIMUM)
        PostMessage(timerWindowHandle, timerFiredMessage, 0, 0);
    } else
        timerID = SetTimer(timerWindowHandle, timerFiredMessage, intervalInMS, 0);
}

void stopSharedTimer()
{
    if (timerID) {
        KillTimer(timerWindowHandle, timerID);
        timerID = 0;
    }
}

void setSharedTimerInstanceHandle(HINSTANCE handle)
{
    instanceHandle = handle;
}

double systemTime()
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER t;
    memcpy(&t, &ft, sizeof(t));
    return t.QuadPart * 0.0000001 - 11644473600.0;
}
