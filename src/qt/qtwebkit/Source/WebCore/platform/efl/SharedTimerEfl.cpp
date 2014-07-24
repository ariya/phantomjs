/*
 * Copyright (C) 2008 Kenneth Rohde Christiansen
 *           (C) 2008 Afonso Rabelo Costa Jr.
 *           (C) 2009-2010 ProFUSION embedded systems
 *           (C) 2009-2010 Samsung Electronics
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
#include "SharedTimer.h"

#include <Ecore.h>
#include <wtf/Assertions.h>
#include <wtf/CurrentTime.h>
#include <wtf/MainThread.h>

namespace WebCore {

static Ecore_Timer *_sharedTimer = 0;

static void (*_timerFunction)();

void setSharedTimerFiredFunction(void (*func)())
{
    _timerFunction = func;
}

static Eina_Bool timerEvent(void*)
{
    _sharedTimer = 0;
    if (_timerFunction)
        _timerFunction();

    return ECORE_CALLBACK_CANCEL;
}

void stopSharedTimer()
{
    if (_sharedTimer) {
        ecore_timer_del(_sharedTimer);
        _sharedTimer = 0;
    }
}

void addNewTimer(double interval)
{
    stopSharedTimer();

    _sharedTimer = ecore_timer_loop_add(interval, timerEvent, 0);
}

void setSharedTimerFireInterval(double interval)
{
    addNewTimer(interval);
}

}

