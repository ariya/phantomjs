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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
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

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)

#include "DisplayRefreshMonitor.h"

#include <QuartzCore/QuartzCore.h>
#include <wtf/CurrentTime.h>
#include <wtf/MainThread.h>

namespace WebCore {

static CVReturn displayLinkCallback(CVDisplayLinkRef, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags, CVOptionFlags*, void* data)
{
    DisplayRefreshMonitor* monitor = static_cast<DisplayRefreshMonitor*>(data);

    double nowSeconds = static_cast<double>(now->videoTime) / static_cast<double>(now->videoTimeScale);
    double outputTimeSeconds = static_cast<double>(outputTime->videoTime) / static_cast<double>(outputTime->videoTimeScale);
    monitor->displayLinkFired(nowSeconds, outputTimeSeconds);

    return kCVReturnSuccess;
}

DisplayRefreshMonitor::~DisplayRefreshMonitor()
{
    if (m_displayLink) {
        CVDisplayLinkStop(m_displayLink);
        CVDisplayLinkRelease(m_displayLink);
        m_displayLink = 0;
    }

    cancelCallOnMainThread(DisplayRefreshMonitor::handleDisplayRefreshedNotificationOnMainThread, this);
}

bool DisplayRefreshMonitor::requestRefreshCallback()
{
    if (!m_active)
        return false;

    if (!m_displayLink) {
        m_active = false;
        CVReturn error = CVDisplayLinkCreateWithCGDisplay(m_displayID, &m_displayLink);
        if (error)
            return false;

        error = CVDisplayLinkSetOutputCallback(m_displayLink, displayLinkCallback, this);
        if (error)
            return false;

        error = CVDisplayLinkStart(m_displayLink);
        if (error)
            return false;

        m_active = true;
    }

    MutexLocker lock(m_mutex);
    m_scheduled = true;
    return true;
}

void DisplayRefreshMonitor::displayLinkFired(double nowSeconds, double outputTimeSeconds)
{
    MutexLocker lock(m_mutex);
    if (!m_previousFrameDone)
        return;

    m_previousFrameDone = false;

    double webKitMonotonicNow = monotonicallyIncreasingTime();
    double timeUntilOutput = outputTimeSeconds - nowSeconds;
    // FIXME: Should this be using webKitMonotonicNow?
    m_monotonicAnimationStartTime = webKitMonotonicNow + timeUntilOutput;

    callOnMainThread(handleDisplayRefreshedNotificationOnMainThread, this);
}

}

#endif // USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
