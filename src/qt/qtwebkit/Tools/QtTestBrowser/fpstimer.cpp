/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 * All rights reserved.
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

#include "fpstimer.h"

#include <QDateTime>
#include <QTimerEvent>

// We save a maximum of 10000 frames, and purge 2000 at a time
#define MAX_FRAMES_SAVED 10000
#define FRAMES_TO_PURGE_WHEN_FULL 2000
// 60 FPS
#define FPS_MEASURE_INTERVAL 1000 / 60

FpsTimer::FpsTimer(QObject* parent)
    : QObject(parent)
    , m_timer(0)
{
}

int FpsTimer::numFrames(int spanMillis) const
{
    const QTime now = QTime::currentTime();

    int count = 0;
    for (int i = m_frames.length() - 1; i >= 0; --i, ++count) {
        int msecs = m_frames[i].msecsTo(now);
        if (msecs < 0)
            msecs += 24 * 60 * 60 * 1000;
        if (msecs > spanMillis)
            break;
    }
    return count;
}

void FpsTimer::start()
{
    m_timer = startTimer(FPS_MEASURE_INTERVAL);
}

void FpsTimer::stop()
{
    if (!m_timer)
        return;
    killTimer(m_timer);
    m_frames.clear();
}

void FpsTimer::timerEvent(QTimerEvent* event)
{
    if (event->timerId() != m_timer)
        return;
    m_frames.append(QTime::currentTime());
    if (m_frames.length() > MAX_FRAMES_SAVED)
        m_frames.erase(m_frames.begin(), m_frames.begin() + FRAMES_TO_PURGE_WHEN_FULL);
}
