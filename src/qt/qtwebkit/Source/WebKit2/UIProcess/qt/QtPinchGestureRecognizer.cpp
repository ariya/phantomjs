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
#include "QtPinchGestureRecognizer.h"


#include "PageViewportControllerClientQt.h"
#include "QtWebPageEventHandler.h"
#include <QtCore/QLineF>

namespace WebKit {

const qreal pinchInitialTriggerDistanceThreshold = 5.;

static inline QPointF computePinchCenter(const QTouchEvent::TouchPoint& point1, const QTouchEvent::TouchPoint& point2)
{
    return (point1.pos() + point2.pos()) / 2.0f;
}

QtPinchGestureRecognizer::QtPinchGestureRecognizer(QtWebPageEventHandler* eventHandler)
    : QtGestureRecognizer(eventHandler)
{
    reset();
}

bool QtPinchGestureRecognizer::update(const QTouchEvent::TouchPoint& point1, const QTouchEvent::TouchPoint& point2)
{
    const qreal currentFingerDistance = QLineF(point1.screenPos(), point2.screenPos()).length();
    switch (m_state) {
    case NoGesture:
        m_initialFingerDistance = currentFingerDistance;
        m_state = GestureRecognitionStarted;
        return false;
    case GestureRecognitionStarted: {
        const qreal pinchDistance = qAbs(currentFingerDistance - m_initialFingerDistance);
        if (pinchDistance < pinchInitialTriggerDistanceThreshold)
            return false;
        m_state = GestureRecognized;
        if (viewportController())
            viewportController()->pinchGestureStarted(computePinchCenter(point1, point2));

        // We reset the initial span distance to the current distance of the
        // touch points in order to avoid the jump caused by the events which
        // were skipped between the recognition start and the actual recognition.
        m_initialFingerDistance = currentFingerDistance;

        // fall through
    }
    case GestureRecognized:
        const qreal totalScaleFactor = currentFingerDistance / m_initialFingerDistance;
        const QPointF touchCenterInViewCoordinates = computePinchCenter(point1, point2);
        if (viewportController())
            viewportController()->pinchGestureRequestUpdate(touchCenterInViewCoordinates, totalScaleFactor);
        return true;
        break;
    }

    ASSERT_NOT_REACHED();
    return false;
}

void QtPinchGestureRecognizer::finish()
{
    if (m_state == NoGesture)
        return;

    if (viewportController())
        viewportController()->pinchGestureEnded();
    reset();
}

void QtPinchGestureRecognizer::cancel()
{
    if (m_state == NoGesture)
        return;

    if (viewportController())
        viewportController()->pinchGestureCancelled();
    reset();
}

} // namespace WebKit

