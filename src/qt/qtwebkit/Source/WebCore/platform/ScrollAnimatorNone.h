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

#ifndef ScrollAnimatorNone_h
#define ScrollAnimatorNone_h

#if ENABLE(SMOOTH_SCROLLING)

#if !ENABLE(REQUEST_ANIMATION_FRAME)
#error "SMOOTH_SCROLLING requires REQUEST_ANIMATION_FRAME to be enabled."
#endif

#include "FloatPoint.h"
#include "ScrollAnimator.h"
#include "Timer.h"
#include <wtf/OwnPtr.h>

class ScrollAnimatorNoneTest;

namespace WebCore {

class IntPoint;
class ActivePlatformGestureAnimation;
struct ScrollAnimatorParameters;

class ScrollAnimatorNone : public ScrollAnimator {
public:
    explicit ScrollAnimatorNone(ScrollableArea*);
    virtual ~ScrollAnimatorNone();

    virtual bool scroll(ScrollbarOrientation, ScrollGranularity, float step, float multiplier);
    virtual void scrollToOffsetWithoutAnimation(const FloatPoint&);

#if !USE(REQUEST_ANIMATION_FRAME_TIMER)
    virtual void cancelAnimations();
    virtual void serviceScrollAnimations();
#endif

    virtual void willEndLiveResize();
    virtual void didAddVerticalScrollbar(Scrollbar*);
    virtual void didAddHorizontalScrollbar(Scrollbar*);

    enum Curve {
        Linear,
        Quadratic,
        Cubic,
        Quartic,
        Bounce
    };

    struct Parameters {
        Parameters();
        Parameters(bool isEnabled, double animationTime, double repeatMinimumSustainTime, Curve attackCurve, double attackTime, Curve releaseCurve, double releaseTime, Curve coastTimeCurve, double maximumCoastTime);

        // Note that the times can be overspecified such that releaseTime or releaseTime and attackTime are greater
        // than animationTime. animationTime takes priority over releaseTime, capping it. attackTime is capped at
        // whatever time remains, or zero if none.
        bool m_isEnabled;
        double m_animationTime;
        double m_repeatMinimumSustainTime;

        Curve m_attackCurve;
        double m_attackTime;

        Curve m_releaseCurve;
        double m_releaseTime;

        Curve m_coastTimeCurve;
        double m_maximumCoastTime;
    };

protected:
    virtual void animationWillStart() { }
    virtual void animationDidFinish() { }

    Parameters parametersForScrollGranularity(ScrollGranularity) const;

    friend class ::ScrollAnimatorNoneTest;

    struct PerAxisData {
        PerAxisData(ScrollAnimatorNone* parent, float* currentPos, int visibleLength);
        void reset();
        bool updateDataFromParameters(float step, float multiplier, float scrollableSize, double currentTime, Parameters*);
        bool animateScroll(double currentTime);
        void updateVisibleLength(int visibleLength);

        static double curveAt(Curve, double t);
        static double attackCurve(Curve, double deltaT, double curveT, double startPos, double attackPos);
        static double releaseCurve(Curve, double deltaT, double curveT, double releasePos, double desiredPos);
        static double coastCurve(Curve, double factor);

        static double curveIntegralAt(Curve, double t);
        static double attackArea(Curve, double startT, double endT);
        static double releaseArea(Curve, double startT, double endT);

        float* m_currentPosition;
        double m_currentVelocity;

        double m_desiredPosition;
        double m_desiredVelocity;

        double m_startPosition;
        double m_startTime;
        double m_startVelocity;

        double m_animationTime;
        double m_lastAnimationTime;

        double m_attackPosition;
        double m_attackTime;
        Curve m_attackCurve;

        double m_releasePosition;
        double m_releaseTime;
        Curve m_releaseCurve;

        int m_visibleLength;
    };

#if USE(REQUEST_ANIMATION_FRAME_TIMER)
    void animationTimerFired(Timer<ScrollAnimatorNone>*);
    void startNextTimer(double delay);
#else
    void startNextTimer();
#endif
    void animationTimerFired();

    void stopAnimationTimerIfNeeded();
    bool animationTimerActive();
    void updateVisibleLengths();

    PerAxisData m_horizontalData;
    PerAxisData m_verticalData;

    double m_startTime;
#if USE(REQUEST_ANIMATION_FRAME_TIMER)
    Timer<ScrollAnimatorNone> m_animationTimer;
#else
    bool m_animationActive;
#endif
};

} // namespace WebCore

#endif // ENABLE(SMOOTH_SCROLLING)

#endif // ScrollAnimatorNone_h
