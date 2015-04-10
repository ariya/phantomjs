/*
 * Copyright (c) 2011, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(SMOOTH_SCROLLING)

#include "ScrollAnimatorNone.h"

#include "FloatPoint.h"
#include "NotImplemented.h"
#include <wtf/OwnArrayPtr.h>
#include "ScrollableArea.h"
#include "ScrollbarTheme.h"
#include <algorithm>
#include <wtf/CurrentTime.h>
#include <wtf/PassOwnPtr.h>

#if ENABLE(GESTURE_EVENTS)
#include "PlatformGestureEvent.h"
#endif

using namespace std;

namespace WebCore {

const double kFrameRate = 60;
const double kTickTime = 1 / kFrameRate;
const double kMinimumTimerInterval = .001;
const double kZoomTicks = 11;

#if !(PLATFORM(BLACKBERRY))
PassOwnPtr<ScrollAnimator> ScrollAnimator::create(ScrollableArea* scrollableArea)
{
    if (scrollableArea && scrollableArea->scrollAnimatorEnabled())
        return adoptPtr(new ScrollAnimatorNone(scrollableArea));
    return adoptPtr(new ScrollAnimator(scrollableArea));
}
#endif

ScrollAnimatorNone::Parameters::Parameters()
    : m_isEnabled(false)
{
}

ScrollAnimatorNone::Parameters::Parameters(bool isEnabled, double animationTime, double repeatMinimumSustainTime, Curve attackCurve, double attackTime, Curve releaseCurve, double releaseTime, Curve coastTimeCurve, double maximumCoastTime)
    : m_isEnabled(isEnabled)
    , m_animationTime(animationTime)
    , m_repeatMinimumSustainTime(repeatMinimumSustainTime)
    , m_attackCurve(attackCurve)
    , m_attackTime(attackTime)
    , m_releaseCurve(releaseCurve)
    , m_releaseTime(releaseTime)
    , m_coastTimeCurve(coastTimeCurve)
    , m_maximumCoastTime(maximumCoastTime)
{
}

double ScrollAnimatorNone::PerAxisData::curveAt(Curve curve, double t)
{
    switch (curve) {
    case Linear:
        return t;
    case Quadratic:
        return t * t;
    case Cubic:
        return t * t * t;
    case Quartic:
        return t * t * t * t;
    case Bounce:
        // Time base is chosen to keep the bounce points simpler:
        // 1 (half bounce coming in) + 1 + .5 + .25
        const double kTimeBase = 2.75;
        const double kTimeBaseSquared = kTimeBase * kTimeBase;
        if (t < 1 / kTimeBase)
            return kTimeBaseSquared * t * t;
        if (t < 2 / kTimeBase) {
            // Invert a [-.5,.5] quadratic parabola, center it in [1,2].
            double t1 = t - 1.5 / kTimeBase;
            const double kParabolaAtEdge = 1 - .5 * .5;
            return kTimeBaseSquared * t1 * t1 + kParabolaAtEdge;
        }
        if (t < 2.5 / kTimeBase) {
            // Invert a [-.25,.25] quadratic parabola, center it in [2,2.5].
            double t2 = t - 2.25 / kTimeBase;
            const double kParabolaAtEdge = 1 - .25 * .25;
            return kTimeBaseSquared * t2 * t2 + kParabolaAtEdge;
        }
            // Invert a [-.125,.125] quadratic parabola, center it in [2.5,2.75].
        const double kParabolaAtEdge = 1 - .125 * .125;
        t -= 2.625 / kTimeBase;
        return kTimeBaseSquared * t * t + kParabolaAtEdge;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

double ScrollAnimatorNone::PerAxisData::attackCurve(Curve curve, double deltaTime, double curveT, double startPosition, double attackPosition)
{
    double t = deltaTime / curveT;
    double positionFactor = curveAt(curve, t);
    return startPosition + positionFactor * (attackPosition - startPosition);
}

double ScrollAnimatorNone::PerAxisData::releaseCurve(Curve curve, double deltaTime, double curveT, double releasePosition, double desiredPosition)
{
    double t = deltaTime / curveT;
    double positionFactor = 1 - curveAt(curve, 1 - t);
    return releasePosition + (positionFactor * (desiredPosition - releasePosition));
}

double ScrollAnimatorNone::PerAxisData::coastCurve(Curve curve, double factor)
{
    return 1 - curveAt(curve, 1 - factor);
}

double ScrollAnimatorNone::PerAxisData::curveIntegralAt(Curve curve, double t)
{
    switch (curve) {
    case Linear:
        return t * t / 2;
    case Quadratic:
        return t * t * t / 3;
    case Cubic:
        return t * t * t * t / 4;
    case Quartic:
        return t * t * t * t * t / 5;
    case Bounce:
        const double kTimeBase = 2.75;
        const double kTimeBaseSquared = kTimeBase * kTimeBase;
        const double kTimeBaseSquaredOverThree = kTimeBaseSquared / 3;
        double area;
        double t1 = min(t, 1 / kTimeBase);
        area = kTimeBaseSquaredOverThree * t1 * t1 * t1;
        if (t < 1 / kTimeBase)
            return area;

        t1 = min(t - 1 / kTimeBase, 1 / kTimeBase);
        // The integral of kTimeBaseSquared * (t1 - .5 / kTimeBase) * (t1 - .5 / kTimeBase) + kParabolaAtEdge
        const double kSecondInnerOffset = kTimeBaseSquared * .5 / kTimeBase;
        double bounceArea = t1 * (t1 * (kTimeBaseSquaredOverThree * t1 - kSecondInnerOffset) + 1);
        area += bounceArea;
        if (t < 2 / kTimeBase)
            return area;

        t1 = min(t - 2 / kTimeBase, 0.5 / kTimeBase);
        // The integral of kTimeBaseSquared * (t1 - .25 / kTimeBase) * (t1 - .25 / kTimeBase) + kParabolaAtEdge
        const double kThirdInnerOffset = kTimeBaseSquared * .25 / kTimeBase;
        bounceArea =  t1 * (t1 * (kTimeBaseSquaredOverThree * t1 - kThirdInnerOffset) + 1);
        area += bounceArea;
        if (t < 2.5 / kTimeBase)
            return area;

        t1 = t - 2.5 / kTimeBase;
        // The integral of kTimeBaseSquared * (t1 - .125 / kTimeBase) * (t1 - .125 / kTimeBase) + kParabolaAtEdge
        const double kFourthInnerOffset = kTimeBaseSquared * .125 / kTimeBase;
        bounceArea = t1 * (t1 * (kTimeBaseSquaredOverThree * t1 - kFourthInnerOffset) + 1);
        area += bounceArea;
        return area;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

double ScrollAnimatorNone::PerAxisData::attackArea(Curve curve, double startT, double endT)
{
    double startValue = curveIntegralAt(curve, startT);
    double endValue = curveIntegralAt(curve, endT);
    return endValue - startValue;
}

double ScrollAnimatorNone::PerAxisData::releaseArea(Curve curve, double startT, double endT)
{
    double startValue = curveIntegralAt(curve, 1 - endT);
    double endValue = curveIntegralAt(curve, 1 - startT);
    return endValue - startValue;
}

ScrollAnimatorNone::PerAxisData::PerAxisData(ScrollAnimatorNone* parent, float* currentPosition, int visibleLength)
    : m_currentPosition(currentPosition)
    , m_visibleLength(visibleLength)
{
    reset();
}

void ScrollAnimatorNone::PerAxisData::reset()
{
    m_currentVelocity = 0;

    m_desiredPosition = 0;
    m_desiredVelocity = 0;

    m_startPosition = 0;
    m_startTime = 0;
    m_startVelocity = 0;

    m_animationTime = 0;
    m_lastAnimationTime = 0;

    m_attackPosition = 0;
    m_attackTime = 0;
    m_attackCurve = Quadratic;

    m_releasePosition = 0;
    m_releaseTime = 0;
    m_releaseCurve = Quadratic;
}


bool ScrollAnimatorNone::PerAxisData::updateDataFromParameters(float step, float multiplier, float scrollableSize, double currentTime, Parameters* parameters)
{
    float delta = step * multiplier;
    if (!m_startTime || !delta || (delta < 0) != (m_desiredPosition - *m_currentPosition < 0)) {
        m_desiredPosition = *m_currentPosition;
        m_startTime = 0;
    }
    float newPosition = m_desiredPosition + delta;

    if (newPosition < 0 || newPosition > scrollableSize)
        newPosition = max(min(newPosition, scrollableSize), 0.0f);

    if (newPosition == m_desiredPosition)
        return false;

    m_desiredPosition = newPosition;

    if (!m_startTime) {
        m_attackTime = parameters->m_attackTime;
        m_attackCurve = parameters->m_attackCurve;
    }
    m_animationTime = parameters->m_animationTime;
    m_releaseTime = parameters->m_releaseTime;
    m_releaseCurve = parameters->m_releaseCurve;

    // Prioritize our way out of over constraint.
    if (m_attackTime + m_releaseTime > m_animationTime) {
        if (m_releaseTime > m_animationTime)
            m_releaseTime = m_animationTime;
        m_attackTime = m_animationTime - m_releaseTime;
    }

    if (!m_startTime) {
        // FIXME: This should be the time from the event that got us here.
        m_startTime = currentTime - kTickTime / 2;
        m_startPosition = *m_currentPosition;
        m_lastAnimationTime = m_startTime;
    }
    m_startVelocity = m_currentVelocity;

    double remainingDelta = m_desiredPosition - *m_currentPosition;

    double attackAreaLeft = 0;

    double deltaTime = m_lastAnimationTime - m_startTime;
    double attackTimeLeft = max(0., m_attackTime - deltaTime);
    double timeLeft = m_animationTime - deltaTime;
    double minTimeLeft = m_releaseTime + min(parameters->m_repeatMinimumSustainTime, m_animationTime - m_releaseTime - attackTimeLeft);
    if (timeLeft < minTimeLeft) {
        m_animationTime = deltaTime + minTimeLeft;
        timeLeft = minTimeLeft;
    }

    if (parameters->m_maximumCoastTime > (parameters->m_repeatMinimumSustainTime + parameters->m_releaseTime)) {
        double targetMaxCoastVelocity = m_visibleLength * .25 * kFrameRate;
        // This needs to be as minimal as possible while not being intrusive to page up/down.
        double minCoastDelta = m_visibleLength;

        if (fabs(remainingDelta) > minCoastDelta) {
            double maxCoastDelta = parameters->m_maximumCoastTime * targetMaxCoastVelocity;
            double coastFactor = min(1., (fabs(remainingDelta) - minCoastDelta) / (maxCoastDelta - minCoastDelta));

            // We could play with the curve here - linear seems a little soft. Initial testing makes me want to feed into the sustain time more aggressively.
            double coastMinTimeLeft = min(parameters->m_maximumCoastTime, minTimeLeft + coastCurve(parameters->m_coastTimeCurve, coastFactor) * (parameters->m_maximumCoastTime - minTimeLeft));

            double additionalTime = max(0., coastMinTimeLeft - minTimeLeft);
            if (additionalTime) {
                double additionalReleaseTime = min(additionalTime, parameters->m_releaseTime / (parameters->m_releaseTime + parameters->m_repeatMinimumSustainTime) * additionalTime);
                m_releaseTime = parameters->m_releaseTime + additionalReleaseTime;
                m_animationTime = deltaTime + coastMinTimeLeft;
                timeLeft = coastMinTimeLeft;
            }
        }
    }

    double releaseTimeLeft = min(timeLeft, m_releaseTime);
    double sustainTimeLeft = max(0., timeLeft - releaseTimeLeft - attackTimeLeft);

    if (attackTimeLeft) {
        double attackSpot = deltaTime / m_attackTime;
        attackAreaLeft = attackArea(m_attackCurve, attackSpot, 1) * m_attackTime;
    }

    double releaseSpot = (m_releaseTime - releaseTimeLeft) / m_releaseTime;
    double releaseAreaLeft  = releaseArea(m_releaseCurve, releaseSpot, 1) * m_releaseTime;

    m_desiredVelocity = remainingDelta / (attackAreaLeft + sustainTimeLeft + releaseAreaLeft);
    m_releasePosition = m_desiredPosition - m_desiredVelocity * releaseAreaLeft;
    if (attackAreaLeft)
        m_attackPosition = m_startPosition + m_desiredVelocity * attackAreaLeft;
    else
        m_attackPosition = m_releasePosition - (m_animationTime - m_releaseTime - m_attackTime) * m_desiredVelocity;

    if (sustainTimeLeft) {
        double roundOff = m_releasePosition - ((attackAreaLeft ? m_attackPosition : *m_currentPosition) + m_desiredVelocity * sustainTimeLeft);
        m_desiredVelocity += roundOff / sustainTimeLeft;
    }

    return true;
}

// FIXME: Add in jank detection trace events into this function.
bool ScrollAnimatorNone::PerAxisData::animateScroll(double currentTime)
{
    double lastScrollInterval = currentTime - m_lastAnimationTime;
    if (lastScrollInterval < kMinimumTimerInterval)
        return true;

    m_lastAnimationTime = currentTime;

    double deltaTime = currentTime - m_startTime;
    double newPosition = *m_currentPosition;

    if (deltaTime > m_animationTime) {
        *m_currentPosition = m_desiredPosition;
        reset();
        return false;
    }
    if (deltaTime < m_attackTime)
        newPosition = attackCurve(m_attackCurve, deltaTime, m_attackTime, m_startPosition, m_attackPosition);
    else if (deltaTime < (m_animationTime - m_releaseTime))
        newPosition = m_attackPosition + (deltaTime - m_attackTime) * m_desiredVelocity;
    else {
        // release is based on targeting the exact final position.
        double releaseDeltaT = deltaTime - (m_animationTime - m_releaseTime);
        newPosition = releaseCurve(m_releaseCurve, releaseDeltaT, m_releaseTime, m_releasePosition, m_desiredPosition);
    }

    // Normalize velocity to a per second amount. Could be used to check for jank.
    if (lastScrollInterval > 0)
        m_currentVelocity = (newPosition - *m_currentPosition) / lastScrollInterval;
    *m_currentPosition = newPosition;

    return true;
}

void ScrollAnimatorNone::PerAxisData::updateVisibleLength(int visibleLength)
{
    m_visibleLength = visibleLength;
}

ScrollAnimatorNone::ScrollAnimatorNone(ScrollableArea* scrollableArea)
    : ScrollAnimator(scrollableArea)
    , m_horizontalData(this, &m_currentPosX, scrollableArea->visibleWidth())
    , m_verticalData(this, &m_currentPosY, scrollableArea->visibleHeight())
    , m_startTime(0)
#if USE(REQUEST_ANIMATION_FRAME_TIMER)
    , m_animationTimer(this, &ScrollAnimatorNone::animationTimerFired)
#else
    , m_animationActive(false)
#endif
{
}

ScrollAnimatorNone::~ScrollAnimatorNone()
{
    stopAnimationTimerIfNeeded();
}

ScrollAnimatorNone::Parameters ScrollAnimatorNone::parametersForScrollGranularity(ScrollGranularity granularity) const
{
#if !PLATFORM(QT)
    switch (granularity) {
    case ScrollByDocument:
        return Parameters(true, 20 * kTickTime, 10 * kTickTime, Cubic, 10 * kTickTime, Cubic, 10 * kTickTime, Linear, 1);
    case ScrollByLine:
        return Parameters(true, 10 * kTickTime, 7 * kTickTime, Cubic, 3 * kTickTime, Cubic, 3 * kTickTime, Linear, 1);
    case ScrollByPage:
        return Parameters(true, 15 * kTickTime, 10 * kTickTime, Cubic, 5 * kTickTime, Cubic, 5 * kTickTime, Linear, 1);
    case ScrollByPixel:
        return Parameters(true, 11 * kTickTime, 2 * kTickTime, Cubic, 3 * kTickTime, Cubic, 3 * kTickTime, Quadratic, 1.25);
    default:
        ASSERT_NOT_REACHED();
    }
#else
    // This is a slightly different strategy for the animation with a steep attack curve and natural release curve.
    // The fast acceleration makes the animation look more responsive to user input.
    switch (granularity) {
    case ScrollByDocument:
        return Parameters(true, 20 * kTickTime, 10 * kTickTime, Cubic, 6 * kTickTime, Quadratic, 10 * kTickTime, Quadratic, 22 * kTickTime);
    case ScrollByLine:
        return Parameters(true, 6 * kTickTime, 5 * kTickTime, Cubic, 1 * kTickTime, Quadratic, 4 * kTickTime, Linear, 1);
    case ScrollByPage:
        return Parameters(true, 12 * kTickTime, 10 * kTickTime, Cubic, 3 * kTickTime, Quadratic, 6 * kTickTime, Linear, 1);
    case ScrollByPixel:
        return Parameters(true, 8 * kTickTime, 3 * kTickTime, Cubic, 2 * kTickTime, Quadratic, 5 * kTickTime, Quadratic, 1.25);
    default:
        ASSERT_NOT_REACHED();
    }
#endif
    return Parameters();
}

bool ScrollAnimatorNone::scroll(ScrollbarOrientation orientation, ScrollGranularity granularity, float step, float multiplier)
{
    if (!m_scrollableArea->scrollAnimatorEnabled())
        return ScrollAnimator::scroll(orientation, granularity, step, multiplier);

    // FIXME: get the type passed in. MouseWheel could also be by line, but should still have different
    // animation parameters than the keyboard.
    Parameters parameters;
    switch (granularity) {
    case ScrollByDocument:
    case ScrollByLine:
    case ScrollByPage:
    case ScrollByPixel:
        parameters = parametersForScrollGranularity(granularity);
        break;
    case ScrollByPrecisePixel:
        return ScrollAnimator::scroll(orientation, granularity, step, multiplier);
    }

    // If the individual input setting is disabled, bail.
    if (!parameters.m_isEnabled)
        return ScrollAnimator::scroll(orientation, granularity, step, multiplier);

    // This is an animatable scroll. Set the animation in motion using the appropriate parameters.
    float scrollableSize = static_cast<float>(m_scrollableArea->scrollSize(orientation));

    PerAxisData& data = (orientation == VerticalScrollbar) ? m_verticalData : m_horizontalData;
    bool needToScroll = data.updateDataFromParameters(step, multiplier, scrollableSize, WTF::monotonicallyIncreasingTime(), &parameters);
    if (needToScroll && !animationTimerActive()) {
        m_startTime = data.m_startTime;
        animationWillStart();
        animationTimerFired();
    }
    return needToScroll;
}

void ScrollAnimatorNone::scrollToOffsetWithoutAnimation(const FloatPoint& offset)
{
    stopAnimationTimerIfNeeded();

    FloatSize delta = FloatSize(offset.x() - *m_horizontalData.m_currentPosition, offset.y() - *m_verticalData.m_currentPosition);

    m_horizontalData.reset();
    *m_horizontalData.m_currentPosition = offset.x();
    m_horizontalData.m_desiredPosition = offset.x();

    m_verticalData.reset();
    *m_verticalData.m_currentPosition = offset.y();
    m_verticalData.m_desiredPosition = offset.y();

    notifyPositionChanged(delta);
}

#if !USE(REQUEST_ANIMATION_FRAME_TIMER)
void ScrollAnimatorNone::cancelAnimations()
{
    m_animationActive = false;
}

void ScrollAnimatorNone::serviceScrollAnimations()
{
    if (m_animationActive)
        animationTimerFired();
}
#endif

void ScrollAnimatorNone::willEndLiveResize()
{
    updateVisibleLengths();
}

void ScrollAnimatorNone::didAddVerticalScrollbar(Scrollbar*)
{
    updateVisibleLengths();
}

void ScrollAnimatorNone::didAddHorizontalScrollbar(Scrollbar*)
{
    updateVisibleLengths();
}

void ScrollAnimatorNone::updateVisibleLengths()
{
    m_horizontalData.updateVisibleLength(scrollableArea()->visibleWidth());
    m_verticalData.updateVisibleLength(scrollableArea()->visibleHeight());
}

#if USE(REQUEST_ANIMATION_FRAME_TIMER)
void ScrollAnimatorNone::animationTimerFired(Timer<ScrollAnimatorNone>* timer)
{
    animationTimerFired();
}
#endif

void ScrollAnimatorNone::animationTimerFired()
{
    double currentTime = WTF::monotonicallyIncreasingTime();
    double deltaToNextFrame = ceil((currentTime - m_startTime) * kFrameRate) / kFrameRate - (currentTime - m_startTime);
    currentTime += deltaToNextFrame;

    bool continueAnimation = false;
    if (m_horizontalData.m_startTime && m_horizontalData.animateScroll(currentTime))
        continueAnimation = true;
    if (m_verticalData.m_startTime && m_verticalData.animateScroll(currentTime))
        continueAnimation = true;

    if (continueAnimation)
#if USE(REQUEST_ANIMATION_FRAME_TIMER)
        startNextTimer(max(kMinimumTimerInterval, deltaToNextFrame));
#else
        startNextTimer();
    else
        m_animationActive = false;
#endif

    notifyPositionChanged(FloatSize());

    if (!continueAnimation)
        animationDidFinish();
}

#if USE(REQUEST_ANIMATION_FRAME_TIMER)
void ScrollAnimatorNone::startNextTimer(double delay)
{
    m_animationTimer.startOneShot(delay);
}
#else
void ScrollAnimatorNone::startNextTimer()
{
    if (scrollableArea()->scheduleAnimation())
        m_animationActive = true;
}
#endif

bool ScrollAnimatorNone::animationTimerActive()
{
#if USE(REQUEST_ANIMATION_FRAME_TIMER)
    return m_animationTimer.isActive();
#else
    return m_animationActive;
#endif
}

void ScrollAnimatorNone::stopAnimationTimerIfNeeded()
{
    if (animationTimerActive())
#if USE(REQUEST_ANIMATION_FRAME_TIMER)
        m_animationTimer.stop();
#else
        m_animationActive = false;
#endif
}

} // namespace WebCore

#endif // ENABLE(SMOOTH_SCROLLING)
