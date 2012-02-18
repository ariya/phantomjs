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

#ifndef ScrollAnimatorWin_h
#define ScrollAnimatorWin_h

#if ENABLE(SMOOTH_SCROLLING)

#include "ScrollAnimator.h"
#include "Timer.h"

namespace WebCore {

class ScrollAnimatorWin : public ScrollAnimator {
public:
    ScrollAnimatorWin(ScrollableArea*);
    virtual ~ScrollAnimatorWin();

    virtual bool scroll(ScrollbarOrientation, ScrollGranularity, float step, float multiplier);
    virtual void scrollToOffsetWithoutAnimation(const FloatPoint&);

private:
    struct PerAxisData {
        PerAxisData(ScrollAnimatorWin* parent, float* currentPos);

        float* m_currentPos;
        float m_desiredPos;
        float m_currentVelocity;
        float m_desiredVelocity;
        double m_lastAnimationTime;
        Timer<ScrollAnimatorWin> m_animationTimer;
    };

    static double accelerationTime();
    static const double animationTimerDelay;

    void animationTimerFired(Timer<ScrollAnimatorWin>*);
    void stopAnimationTimerIfNeeded(PerAxisData*);
    void animateScroll(PerAxisData*);

    PerAxisData m_horizontalData;
    PerAxisData m_verticalData;
};

} // namespace WebCore

#endif // ENABLE(SMOOTH_SCROLLING)

#endif // ScrollAnimatorWin_h
