/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef ScrollAnimatorMac_h
#define ScrollAnimatorMac_h

#if ENABLE(SMOOTH_SCROLLING)

#include "IntRect.h"
#include "FloatPoint.h"
#include "FloatSize.h"
#include "ScrollAnimator.h"
#include "Timer.h"
#include <wtf/RetainPtr.h>

#ifdef __OBJC__
@class ScrollAnimationHelperDelegate;
@class ScrollbarPainterDelegate;
@class ScrollbarPainterControllerDelegate;
@class ScrollbarPainterDelegate;
#else
class ScrollAnimationHelperDelegate;
class ScrollbarPainterDelegate;
class ScrollbarPainterControllerDelegate;
class ScrollbarPainterDelegate;
#endif

#if USE(WK_SCROLLBAR_PAINTER)
typedef struct __WKScrollbarPainterController *WKScrollbarPainterControllerRef;
#endif

namespace WebCore {

class Scrollbar;

class ScrollAnimatorMac : public ScrollAnimator {
public:
    ScrollAnimatorMac(ScrollableArea*);
    virtual ~ScrollAnimatorMac();

    virtual bool scroll(ScrollbarOrientation, ScrollGranularity, float step, float multiplier);
    virtual void scrollToOffsetWithoutAnimation(const FloatPoint&);

#if ENABLE(RUBBER_BANDING)
    virtual void handleWheelEvent(PlatformWheelEvent&);
#if ENABLE(GESTURE_EVENTS)
    virtual void handleGestureEvent(const PlatformGestureEvent&);
#endif
#endif

    virtual void cancelAnimations();

    void immediateScrollToPoint(const FloatPoint& newPosition);
    void immediateScrollByDeltaX(float deltaX);
    void immediateScrollByDeltaY(float deltaY);

    void setIsDrawingIntoLayer(bool b) { m_drawingIntoLayer = b; }
    bool isDrawingIntoLayer() const { return m_drawingIntoLayer; }

    bool haveScrolledSincePageLoad() const { return m_haveScrolledSincePageLoad; }

#if USE(WK_SCROLLBAR_PAINTER)
    bool scrollbarPaintTimerIsActive() const;
    void startScrollbarPaintTimer();
    void stopScrollbarPaintTimer();
#endif

    void setVisibleScrollerThumbRect(const IntRect&);

private:
    RetainPtr<id> m_scrollAnimationHelper;
    RetainPtr<ScrollAnimationHelperDelegate> m_scrollAnimationHelperDelegate;

#if USE(WK_SCROLLBAR_PAINTER)
    RetainPtr<WKScrollbarPainterControllerRef> m_scrollbarPainterController;
    RetainPtr<ScrollbarPainterControllerDelegate> m_scrollbarPainterControllerDelegate;
    RetainPtr<id> m_scrollbarPainterDelegate;

    void initialScrollbarPaintTimerFired(Timer<ScrollAnimatorMac>*);
    Timer<ScrollAnimatorMac> m_initialScrollbarPaintTimer;
#endif
    
    virtual void notityPositionChanged();
    virtual void contentAreaWillPaint() const;
    virtual void mouseEnteredContentArea() const;
    virtual void mouseExitedContentArea() const;
    virtual void mouseMovedInContentArea() const;
    virtual void willStartLiveResize();
    virtual void contentsResized() const;
    virtual void willEndLiveResize();
    virtual void contentAreaDidShow() const;
    virtual void contentAreaDidHide() const;
    void didBeginScrollGesture() const;
    void didEndScrollGesture() const;

    virtual void didAddVerticalScrollbar(Scrollbar*);
    virtual void willRemoveVerticalScrollbar(Scrollbar*);
    virtual void didAddHorizontalScrollbar(Scrollbar*);
    virtual void willRemoveHorizontalScrollbar(Scrollbar*);

    float adjustScrollXPositionIfNecessary(float) const;
    float adjustScrollYPositionIfNecessary(float) const;
    FloatPoint adjustScrollPositionIfNecessary(const FloatPoint&) const;

#if ENABLE(RUBBER_BANDING)
    bool allowsVerticalStretching() const;
    bool allowsHorizontalStretching() const;
    bool pinnedInDirection(float deltaX, float deltaY);
    void snapRubberBand();
    void snapRubberBandTimerFired(Timer<ScrollAnimatorMac>*);
    void smoothScrollWithEvent(PlatformWheelEvent&);
    void beginScrollGesture();
    void endScrollGesture();

    bool m_inScrollGesture;
    bool m_momentumScrollInProgress;
    bool m_ignoreMomentumScrolls;
    
    CFTimeInterval m_lastMomemtumScrollTimestamp;
    FloatSize m_overflowScrollDelta;
    FloatSize m_stretchScrollForce;
    FloatSize m_momentumVelocity;

    // Rubber band state.
    CFTimeInterval m_startTime;
    FloatSize m_startStretch;
    FloatPoint m_origOrigin;
    FloatSize m_origVelocity;
    Timer<ScrollAnimatorMac> m_snapRubberBandTimer;
#endif
    bool m_drawingIntoLayer;
    bool m_haveScrolledSincePageLoad;
    IntRect m_visibleScrollerThumbRect;
};

} // namespace WebCore

#endif // ENABLE(SMOOTH_SCROLLING)

#endif // ScrollAnimatorMac_h
