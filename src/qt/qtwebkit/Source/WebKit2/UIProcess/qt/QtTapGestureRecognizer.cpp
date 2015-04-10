/*
 * Copyright (C) 2011, 2012 Nokia Corporation and/or its subsidiary(-ies)
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
#include "QtTapGestureRecognizer.h"

#include "QtWebPageEventHandler.h"
#include <QLineF>

namespace WebKit {

// FIXME: These constants should possibly depend on DPI.
static const int panDistanceThreshold = 10;
static const int maxDoubleTapDistance = 120;

// FIXME: These constants should possibly be consistent across the platform.
static const int tapAndHoldTime = 1000;
static const int maxDoubleTapInterval = 500;
static const int highlightDelay = 100;


QtTapGestureRecognizer::QtTapGestureRecognizer(QtWebPageEventHandler* eventHandler)
    : QtGestureRecognizer(eventHandler)
    , m_candidate(SingleTapCandidate)
{
}

bool QtTapGestureRecognizer::withinDistance(const QTouchEvent::TouchPoint& touchPoint, int distance)
{
    ASSERT(m_lastTouchPoint.id() != -1);
    return QLineF(touchPoint.screenPos(), m_lastTouchPoint.screenPos()).length() < distance;
}

void QtTapGestureRecognizer::update(const QTouchEvent::TouchPoint& touchPoint)
{
    switch (m_state) {
    case NoGesture:
        m_doubleTapTimer.stop(); // Cancel other pending single tap event.
        m_state = GestureRecognitionStarted;

        ASSERT(!m_tapAndHoldTimer.isActive());
        m_tapAndHoldTimer.start(tapAndHoldTime, this);

        // Early return if this is the second touch point of a potential double tap gesture.
        if (m_candidate == DoubleTapCandidate && withinDistance(touchPoint, maxDoubleTapDistance))
            return;

        // The below in fact resets any previous single tap event.
        m_candidate = SingleTapCandidate;
        m_lastTouchPoint = touchPoint;
        m_highlightTimer.start(highlightDelay, this);
        m_doubleTapTimer.start(maxDoubleTapInterval, this);
        break;
    case GestureRecognitionStarted:
        // If the touch point moves further than the threshold, we cancel the tap gesture.
        if (!withinDistance(touchPoint, panDistanceThreshold))
            reset();
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

void QtTapGestureRecognizer::finish(const QTouchEvent::TouchPoint& touchPoint)
{
    ASSERT(m_eventHandler);
    m_tapAndHoldTimer.stop();

    // Gesture has been cancelled, ignore.
    if (m_state == NoGesture)
        return;

    m_state = NoGesture;

    if (m_candidate == SingleTapCandidate) {
        if (m_doubleTapTimer.isActive()) {
            m_candidate = DoubleTapCandidate;
            m_lastTouchPoint = touchPoint;
            // Early return since this is a potential double tap gesture.
            return;
        }
        // This happens when the finger is released (gesture finished) after the single
        // tap timeout elapsed (500ms) but before the tap-and-hold timeout (1000ms) fired.
        m_eventHandler->handleSingleTapEvent(touchPoint);
    } else // DoubleTapCandidate
        m_eventHandler->handleDoubleTapEvent(touchPoint);

    reset();
}

void QtTapGestureRecognizer::cancel()
{
    if (m_lastTouchPoint.id() == -1)
        return;

    reset();
}

void QtTapGestureRecognizer::highlightTimeout()
{
    m_highlightTimer.stop();

    // Gesture has been cancelled, ignore.
    if (m_lastTouchPoint.id() == -1)
        return;

    m_eventHandler->activateTapHighlight(m_lastTouchPoint);
}

void QtTapGestureRecognizer::singleTapTimeout()
{
    m_doubleTapTimer.stop();

    // Finger is still pressed or gesture has been cancelled, ignore.
    if (m_tapAndHoldTimer.isActive() || m_lastTouchPoint.id() == -1)
        return;

    m_eventHandler->handleSingleTapEvent(m_lastTouchPoint);
    reset();
}

void QtTapGestureRecognizer::tapAndHoldTimeout()
{
    m_tapAndHoldTimer.stop();

    // Gesture has been cancelled, ignore.
    if (m_lastTouchPoint.id() == -1)
        return;

#if 0 // No support for synthetic context menus in WK2 yet.
    m_eventHandler->handleTapAndHoldEvent(m_lastTouchPoint);
#endif
    reset();
}

void QtTapGestureRecognizer::reset()
{
    m_eventHandler->deactivateTapHighlight();

    m_candidate = SingleTapCandidate;
    m_lastTouchPoint.setId(-1);
    m_highlightTimer.stop();
    m_doubleTapTimer.stop();
    m_tapAndHoldTimer.stop();

    QtGestureRecognizer::reset();
}

void QtTapGestureRecognizer::timerEvent(QTimerEvent* ev)
{
    int timerId = ev->timerId();
    if (timerId == m_highlightTimer.timerId())
        highlightTimeout();
    else if (timerId == m_doubleTapTimer.timerId())
        singleTapTimeout();
    else if (timerId == m_tapAndHoldTimer.timerId())
        tapAndHoldTimeout();
    else
        QObject::timerEvent(ev);
}

} // namespace WebKit
