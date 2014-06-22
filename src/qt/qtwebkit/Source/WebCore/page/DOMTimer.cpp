/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 *
 */

#include "config.h"
#include "DOMTimer.h"

#include "InspectorInstrumentation.h"
#include "ScheduledAction.h"
#include "ScriptExecutionContext.h"
#include "UserGestureIndicator.h"
#include <wtf/CurrentTime.h>
#include <wtf/HashSet.h>
#include <wtf/StdLibExtras.h>

using namespace std;

namespace WebCore {

static const int maxIntervalForUserGestureForwarding = 1000; // One second matches Gecko.
static const int maxTimerNestingLevel = 5;
static const double oneMillisecond = 0.001;

static int timerNestingLevel = 0;
    
static inline bool shouldForwardUserGesture(int interval, int nestingLevel)
{
    return UserGestureIndicator::processingUserGesture()
        && interval <= maxIntervalForUserGestureForwarding
        && nestingLevel == 1; // Gestures should not be forwarded to nested timers.
}

DOMTimer::DOMTimer(ScriptExecutionContext* context, PassOwnPtr<ScheduledAction> action, int interval, bool singleShot)
    : SuspendableTimer(context)
    , m_nestingLevel(timerNestingLevel + 1)
    , m_action(action)
    , m_originalInterval(interval)
    , m_shouldForwardUserGesture(shouldForwardUserGesture(interval, m_nestingLevel))
{
    // Keep asking for the next id until we're given one that we don't already have.
    do {
        m_timeoutId = context->circularSequentialID();
    } while (!context->addTimeout(m_timeoutId, this));

    double intervalMilliseconds = intervalClampedToMinimum(interval, context->minimumTimerInterval());
    if (singleShot)
        startOneShot(intervalMilliseconds);
    else
        startRepeating(intervalMilliseconds);
}

DOMTimer::~DOMTimer()
{
    if (scriptExecutionContext())
        scriptExecutionContext()->removeTimeout(m_timeoutId);
}

int DOMTimer::install(ScriptExecutionContext* context, PassOwnPtr<ScheduledAction> action, int timeout, bool singleShot)
{
    // DOMTimer constructor links the new timer into a list of ActiveDOMObjects held by the 'context'.
    // The timer is deleted when context is deleted (DOMTimer::contextDestroyed) or explicitly via DOMTimer::removeById(),
    // or if it is a one-time timer and it has fired (DOMTimer::fired).
    DOMTimer* timer = new DOMTimer(context, action, timeout, singleShot);

    timer->suspendIfNeeded();
    InspectorInstrumentation::didInstallTimer(context, timer->m_timeoutId, timeout, singleShot);

    return timer->m_timeoutId;
}

void DOMTimer::removeById(ScriptExecutionContext* context, int timeoutId)
{
    // timeout IDs have to be positive, and 0 and -1 are unsafe to
    // even look up since they are the empty and deleted value
    // respectively
    if (timeoutId <= 0)
        return;

    InspectorInstrumentation::didRemoveTimer(context, timeoutId);

    delete context->findTimeout(timeoutId);
}

void DOMTimer::fired()
{
    ScriptExecutionContext* context = scriptExecutionContext();
    timerNestingLevel = m_nestingLevel;
    ASSERT(!context->activeDOMObjectsAreSuspended());
    UserGestureIndicator gestureIndicator(m_shouldForwardUserGesture ? DefinitelyProcessingUserGesture : PossiblyProcessingUserGesture);
    // Only the first execution of a multi-shot timer should get an affirmative user gesture indicator.
    m_shouldForwardUserGesture = false;

    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willFireTimer(context, m_timeoutId);

    // Simple case for non-one-shot timers.
    if (isActive()) {
        double minimumInterval = context->minimumTimerInterval();
        if (repeatInterval() && repeatInterval() < minimumInterval) {
            m_nestingLevel++;
            if (m_nestingLevel >= maxTimerNestingLevel)
                augmentRepeatInterval(minimumInterval - repeatInterval());
        }

        // No access to member variables after this point, it can delete the timer.
        m_action->execute(context);

        InspectorInstrumentation::didFireTimer(cookie);

        return;
    }

    // Delete timer before executing the action for one-shot timers.
    OwnPtr<ScheduledAction> action = m_action.release();

    // No access to member variables after this point.
    delete this;

    action->execute(context);

    InspectorInstrumentation::didFireTimer(cookie);

    timerNestingLevel = 0;
}

void DOMTimer::contextDestroyed()
{
    SuspendableTimer::contextDestroyed();
    delete this;
}

void DOMTimer::stop()
{
    SuspendableTimer::stop();
    // Need to release JS objects potentially protected by ScheduledAction
    // because they can form circular references back to the ScriptExecutionContext
    // which will cause a memory leak.
    m_action.clear();
}

void DOMTimer::adjustMinimumTimerInterval(double oldMinimumTimerInterval)
{
    if (m_nestingLevel < maxTimerNestingLevel)
        return;

    double newMinimumInterval = scriptExecutionContext()->minimumTimerInterval();
    double newClampedInterval = intervalClampedToMinimum(m_originalInterval, newMinimumInterval);

    if (repeatInterval()) {
        augmentRepeatInterval(newClampedInterval - repeatInterval());
        return;
    }

    double previousClampedInterval = intervalClampedToMinimum(m_originalInterval, oldMinimumTimerInterval);
    augmentFireInterval(newClampedInterval - previousClampedInterval);
}

double DOMTimer::intervalClampedToMinimum(int timeout, double minimumTimerInterval) const
{
    double intervalMilliseconds = max(oneMillisecond, timeout * oneMillisecond);

    if (intervalMilliseconds < minimumTimerInterval && m_nestingLevel >= maxTimerNestingLevel)
        intervalMilliseconds = minimumTimerInterval;
    return intervalMilliseconds;
}

double DOMTimer::alignedFireTime(double fireTime) const
{
    double alignmentInterval = scriptExecutionContext()->timerAlignmentInterval();
    if (alignmentInterval) {
        double currentTime = monotonicallyIncreasingTime();
        if (fireTime <= currentTime)
            return fireTime;

        double alignedTime = ceil(fireTime / alignmentInterval) * alignmentInterval;
        return alignedTime;
    }

    return fireTime;
}

} // namespace WebCore
