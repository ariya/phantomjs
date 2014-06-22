/*
 * Copyright (C) 2011 Benjamin Poulain <benjamin@webkit.org>
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
#include "QtPanGestureRecognizer.h"

#include "PageViewportControllerClientQt.h"
#include "QtWebPageEventHandler.h"

namespace WebKit {

QtPanGestureRecognizer::QtPanGestureRecognizer(QtWebPageEventHandler* eventHandler)
    : QtGestureRecognizer(eventHandler)
{
    reset();
}

bool QtPanGestureRecognizer::update(const QTouchEvent::TouchPoint& touchPoint, qint64 eventTimestampMillis)
{
    m_lastPosition = touchPoint.pos();
    m_lastEventTimestampMillis = eventTimestampMillis;

    switch (m_state) {
    case NoGesture:
        m_state = GestureRecognitionStarted;
        m_firstScreenPosition = touchPoint.screenPos();
        if (viewportController())
            viewportController()->cancelScrollAnimation();
        return false;
    case GestureRecognitionStarted: {
        // To start the gesture, the delta from start in screen coordinates
        // must be bigger than the trigger threshold.
        QPointF totalOffsetFromStart(touchPoint.screenPos() - m_firstScreenPosition);
        if (qAbs(totalOffsetFromStart.x()) < panningInitialTriggerDistanceThreshold && qAbs(totalOffsetFromStart.y()) < panningInitialTriggerDistanceThreshold)
            return false;

        m_state = GestureRecognized;
        if (viewportController())
            viewportController()->panGestureStarted(touchPoint.pos(), eventTimestampMillis);
        return true;
    }
    case GestureRecognized:
        if (viewportController())
            viewportController()->panGestureRequestUpdate(touchPoint.pos(), eventTimestampMillis);
        return true;
    default:
        ASSERT_NOT_REACHED();
    }
    return false;
}

void QtPanGestureRecognizer::finish(const QTouchEvent::TouchPoint& touchPoint, qint64 eventTimestampMillis)
{
    if (m_state == NoGesture)
        return;

    if (viewportController())
        viewportController()->panGestureEnded(touchPoint.pos(), eventTimestampMillis);
    reset();
}

void QtPanGestureRecognizer::cancel()
{
    if (m_state == NoGesture)
        return;

    if (viewportController()) {
        viewportController()->panGestureEnded(m_lastPosition, m_lastEventTimestampMillis);
        viewportController()->panGestureCancelled();
    }
    reset();
}

} // namespace WebKit
