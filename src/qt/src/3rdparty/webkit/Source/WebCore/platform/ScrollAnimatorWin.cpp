/*
 * Copyright (c) 2010, Google Inc. All rights reserved.
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

#include "ScrollAnimatorWin.h"

#include "FloatPoint.h"
#include "ScrollableArea.h"
#include "ScrollbarTheme.h"
#include <algorithm>
#include <wtf/CurrentTime.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

PassOwnPtr<ScrollAnimator> ScrollAnimator::create(ScrollableArea* scrollableArea)
{
    return adoptPtr(new ScrollAnimatorWin(scrollableArea));
}

const double ScrollAnimatorWin::animationTimerDelay = 0.01;

ScrollAnimatorWin::PerAxisData::PerAxisData(ScrollAnimatorWin* parent, float* currentPos)
    : m_currentPos(currentPos)
    , m_desiredPos(0)
    , m_currentVelocity(0)
    , m_desiredVelocity(0)
    , m_lastAnimationTime(0)
    , m_animationTimer(parent, &ScrollAnimatorWin::animationTimerFired)
{
}


ScrollAnimatorWin::ScrollAnimatorWin(ScrollableArea* scrollableArea)
    : ScrollAnimator(scrollableArea)
    , m_horizontalData(this, &m_currentPosX)
    , m_verticalData(this, &m_currentPosY)
{
}

ScrollAnimatorWin::~ScrollAnimatorWin()
{
    stopAnimationTimerIfNeeded(&m_horizontalData);
    stopAnimationTimerIfNeeded(&m_verticalData);
}

bool ScrollAnimatorWin::scroll(ScrollbarOrientation orientation, ScrollGranularity granularity, float step, float multiplier)
{
    // Don't animate jumping to the beginning or end of the document.
    if (granularity == ScrollByDocument)
        return ScrollAnimator::scroll(orientation, granularity, step, multiplier);

    // This is an animatable scroll.  Calculate the scroll delta.
    PerAxisData* data = (orientation == VerticalScrollbar) ? &m_verticalData : &m_horizontalData;
    float newPos = std::max(std::min(data->m_desiredPos + (step * multiplier), static_cast<float>(m_scrollableArea->scrollSize(orientation))), 0.0f);
    if (newPos == data->m_desiredPos)
        return false;
    data->m_desiredPos = newPos;

    // Calculate the animation velocity.
    if (*data->m_currentPos == data->m_desiredPos)
        return false;
    bool alreadyAnimating = data->m_animationTimer.isActive();
    // There are a number of different sources of scroll requests.  We want to
    // make both keyboard and wheel-generated scroll requests (which can come at
    // unpredictable rates) and autoscrolling from holding down the mouse button
    // on a scrollbar part (where the request rate can be obtained from the
    // scrollbar theme) feel smooth, responsive, and similar.
    //
    // When autoscrolling, the scrollbar's autoscroll timer will call us to
    // increment the desired position by |step| (with |multiplier| == 1) every
    // ScrollbarTheme::nativeTheme()->autoscrollTimerDelay() seconds.  If we set
    // the desired velocity to exactly this rate, smooth scrolling will neither
    // race ahead (and then have to slow down) nor increasingly lag behind, but
    // will be smooth and synchronized.
    //
    // Note that because of the acceleration period, the current position in
    // this case would lag the desired one by a small, constant amount (see
    // comments on animateScroll()); the exact amount is given by
    //   lag = |step| - v(0.5tA + tD)
    // Where
    //   v = The steady-state velocity,
    //       |step| / ScrollbarTheme::nativeTheme()->autoscrollTimerDelay()
    //   tA = accelerationTime()
    //   tD = The time we pretend has already passed when starting to scroll,
    //        |animationTimerDelay|
    //
    // This lag provides some buffer against timer jitter so we're less likely
    // to hit the desired position and stop (and thus have to re-accelerate,
    // causing a visible hitch) while waiting for the next autoscroll increment.
    //
    // Thus, for autoscroll-timer-triggered requests, the ideal steady-state
    // distance to travel in each time interval is:
    //   float animationStep = step;
    // Note that when we're not already animating, this is exactly the same as
    // the distance to the target position.  We'll return to that in a moment.
    //
    // For keyboard and wheel scrolls, we don't know when the next increment
    // will be requested.  If we set the target velocity based on how far away
    // from the target position we are, then for keyboard/wheel events that come
    // faster than the autoscroll delay, we'll asymptotically approach the
    // velocity needed to stay smoothly in sync with the user's actions; for
    // events that come slower, we'll scroll one increment and then pause until
    // the next event fires.
    float animationStep = fabs(newPos - *data->m_currentPos);
    // If a key is held down (or the wheel continually spun), then once we have
    // reached a velocity close to the steady-state velocity, we're likely to
    // hit the desired position at around the same time we'd expect the next
    // increment to occur -- bad because it leads to hitching as described above
    // (if autoscroll-based requests didn't result in a small amount of constant
    // lag).  So if we're called again while already animating, we want to trim
    // the animationStep slightly to maintain lag like what's described above.
    // (I say "maintain" since we'll already be lagged due to the acceleration
    // during the first scroll period.)
    //
    // Remember that trimming won't cause us to fall steadily further behind
    // here, because the further behind we are, the larger the base step value
    // above.  Given the scrolling algorithm in animateScroll(), the practical
    // effect will actually be that, assuming a constant trim factor, we'll lag
    // by a constant amount depending on the rate at which increments occur
    // compared to the autoscroll timer delay.  The exact lag is given by
    //   lag = |step| * ((r / k) - 1)
    // Where
    //   r = The ratio of the autoscroll repeat delay,
    //       ScrollbarTheme::nativeTheme()->autoscrollTimerDelay(), to the
    //       key/wheel repeat delay (i.e. > 1 when keys repeat faster)
    //   k = The velocity trim constant given below
    //
    // We want to choose the trim factor such that for calls that come at the
    // autoscroll timer rate, we'll wind up with the same lag as in the
    // "perfect" case described above (or, to put it another way, we'll end up
    // with |animationStep| == |step| * |multiplier| despite the actual distance
    // calculated above being larger than that).  This will result in "perfect"
    // behavior for autoscrolling without having to special-case it.
    if (alreadyAnimating)
        animationStep /= (2.0 - ((1.0 / ScrollbarTheme::nativeTheme()->autoscrollTimerDelay()) * (0.5 * accelerationTime() + animationTimerDelay)));
    // The result of all this is that single keypresses or wheel flicks will
    // scroll in the same time period as single presses of scrollbar elements;
    // holding the mouse down on a scrollbar part will scroll as fast as
    // possible without hitching; and other repeated scroll events will also
    // scroll with the same time lag as holding down the mouse on a scrollbar
    // part.
    data->m_desiredVelocity = animationStep / ScrollbarTheme::nativeTheme()->autoscrollTimerDelay();

    // If we're not already scrolling, start.
    if (!alreadyAnimating)
        animateScroll(data);
    return true;
}

void ScrollAnimatorWin::scrollToOffsetWithoutAnimation(const FloatPoint& offset)
{
    stopAnimationTimerIfNeeded(&m_horizontalData);
    stopAnimationTimerIfNeeded(&m_verticalData);

    *m_horizontalData.m_currentPos = offset.x();
    m_horizontalData.m_desiredPos = offset.x();
    m_horizontalData.m_currentVelocity = 0;
    m_horizontalData.m_desiredVelocity = 0;

    *m_verticalData.m_currentPos = offset.y();
    m_verticalData.m_desiredPos = offset.y();
    m_verticalData.m_currentVelocity = 0;
    m_verticalData.m_desiredVelocity = 0;

    notityPositionChanged();
}

double ScrollAnimatorWin::accelerationTime()
{
    // We elect to use ScrollbarTheme::nativeTheme()->autoscrollTimerDelay() as
    // the length of time we'll take to accelerate from 0 to our target
    // velocity.  Choosing a larger value would produce a more pronounced
    // acceleration effect.
    return ScrollbarTheme::nativeTheme()->autoscrollTimerDelay();
}

void ScrollAnimatorWin::animationTimerFired(Timer<ScrollAnimatorWin>* timer)
{
    animateScroll((timer == &m_horizontalData.m_animationTimer) ? &m_horizontalData : &m_verticalData);
}

void ScrollAnimatorWin::stopAnimationTimerIfNeeded(PerAxisData* data)
{
    if (data->m_animationTimer.isActive())
        data->m_animationTimer.stop();
}

void ScrollAnimatorWin::animateScroll(PerAxisData* data)
{
    // Note on smooth scrolling perf versus non-smooth scrolling perf:
    // The total time to perform a complete scroll is given by
    //   t = t0 + 0.5tA - tD + tS
    // Where
    //   t0 = The time to perform the scroll without smooth scrolling
    //   tA = The acceleration time,
    //        ScrollbarTheme::nativeTheme()->autoscrollTimerDelay() (see below)
    //   tD = |animationTimerDelay|
    //   tS = A value less than or equal to the time required to perform a
    //        single scroll increment, i.e. the work done due to calling
    //        client()->valueChanged() (~0 for simple pages, larger for complex
    //        pages).
    //
    // Because tA and tD are fairly small, the total lag (as users perceive it)
    // is negligible for simple pages and roughly tS for complex pages.  Without
    // knowing in advance how large tS is it's hard to do better than this.
    // Perhaps we could try to remember previous values and forward-compensate.


    // We want to update the scroll position based on the time it's been since
    // our last update.  This may be longer than our ideal time, especially if
    // the page is complex or the system is slow.
    //
    // To avoid feeling laggy, if we've just started smooth scrolling we pretend
    // we've already accelerated for one ideal interval, so that we'll scroll at
    // least some distance immediately.
    double lastScrollInterval = data->m_currentVelocity ? (WTF::currentTime() - data->m_lastAnimationTime) : animationTimerDelay;

    // Figure out how far we've actually traveled and update our current
    // velocity.
    float distanceTraveled;
    if (data->m_currentVelocity < data->m_desiredVelocity) {
        // We accelerate at a constant rate until we reach the desired velocity.
        float accelerationRate = data->m_desiredVelocity / accelerationTime();

        // Figure out whether contant acceleration has caused us to reach our
        // target velocity.
        float potentialVelocityChange = accelerationRate * lastScrollInterval;
        float potentialNewVelocity = data->m_currentVelocity + potentialVelocityChange;
        if (potentialNewVelocity > data->m_desiredVelocity) {
            // We reached the target velocity at some point between our last
            // update and now.  The distance traveled can be calculated in two
            // pieces: the distance traveled while accelerating, and the
            // distance traveled after reaching the target velocity.
            float actualVelocityChange = data->m_desiredVelocity - data->m_currentVelocity;
            float accelerationInterval = actualVelocityChange / accelerationRate;
            // The distance traveled under constant acceleration is the area
            // under a line segment with a constant rising slope.  Break this
            // into a triangular portion atop a rectangular portion and sum.
            distanceTraveled = ((data->m_currentVelocity + (actualVelocityChange / 2)) * accelerationInterval);
            // The distance traveled at the target velocity is simply
            // (target velocity) * (remaining time after accelerating).
            distanceTraveled += (data->m_desiredVelocity * (lastScrollInterval - accelerationInterval));
            data->m_currentVelocity = data->m_desiredVelocity;
        } else {
            // Constant acceleration through the entire time interval.
            distanceTraveled = (data->m_currentVelocity + (potentialVelocityChange / 2)) * lastScrollInterval;
            data->m_currentVelocity = potentialNewVelocity;
        }
    } else {
        // We've already reached the target velocity, so the distance we've
        // traveled is simply (current velocity) * (elapsed time).
        distanceTraveled = data->m_currentVelocity * lastScrollInterval;
        // If our desired velocity has decreased, drop the current velocity too.
        data->m_currentVelocity = data->m_desiredVelocity;
    }

    // Now update the scroll position based on the distance traveled.
    if (distanceTraveled >= fabs(data->m_desiredPos - *data->m_currentPos)) {
        // We've traveled far enough to reach the desired position.  Stop smooth
        // scrolling.
        *data->m_currentPos = data->m_desiredPos;
        data->m_currentVelocity = 0;
        data->m_desiredVelocity = 0;
    } else {
        // Not yet at the target position.  Travel towards it and set up the
        // next update.
        if (*data->m_currentPos > data->m_desiredPos)
            distanceTraveled = -distanceTraveled;
        *data->m_currentPos += distanceTraveled;
        data->m_animationTimer.startOneShot(animationTimerDelay);
        data->m_lastAnimationTime = WTF::currentTime();
    }

    notityPositionChanged();
}

} // namespace WebCore

#endif // ENABLE(SMOOTH_SCROLLING)
