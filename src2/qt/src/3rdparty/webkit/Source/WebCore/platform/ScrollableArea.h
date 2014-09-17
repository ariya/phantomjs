/*
 * Copyright (C) 2008, 2011 Apple Inc. All Rights Reserved.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef ScrollableArea_h
#define ScrollableArea_h

#include "IntRect.h"
#include "Scrollbar.h"
#include <wtf/Vector.h>

namespace WebCore {

class FloatPoint;
class GraphicsContext;
class PlatformGestureEvent;
class PlatformWheelEvent;
class ScrollAnimator;
#if USE(ACCELERATED_COMPOSITING)
class GraphicsLayer;
#endif

class ScrollableArea {
public:
    ScrollableArea();
    virtual ~ScrollableArea();

    bool scroll(ScrollDirection, ScrollGranularity, float multiplier = 1);
    void scrollToOffsetWithoutAnimation(const FloatPoint&);
    void scrollToOffsetWithoutAnimation(ScrollbarOrientation, float offset);
    void scrollToXOffsetWithoutAnimation(float x);
    void scrollToYOffsetWithoutAnimation(float x);

    void handleWheelEvent(PlatformWheelEvent&);
#if ENABLE(GESTURE_EVENTS)
    void handleGestureEvent(const PlatformGestureEvent&);
#endif

    // Functions for controlling if you can scroll past the end of the document.
    bool constrainsScrollingToContentEdge() const { return m_constrainsScrollingToContentEdge; }
    void setConstrainsScrollingToContentEdge(bool constrainsScrollingToContentEdge) { m_constrainsScrollingToContentEdge = constrainsScrollingToContentEdge; }

    void setVerticalScrollElasticity(ScrollElasticity scrollElasticity) { m_verticalScrollElasticity = scrollElasticity; }
    ScrollElasticity verticalScrollElasticity() const { return m_verticalScrollElasticity; }

    void setHorizontalScrollElasticity(ScrollElasticity scrollElasticity) { m_horizontalScrollElasticity = scrollElasticity; }
    ScrollElasticity horizontalScrollElasticity() const { return m_horizontalScrollElasticity; }

    bool inLiveResize() const { return m_inLiveResize; }
    void willStartLiveResize();
    void willEndLiveResize();

    void didAddVerticalScrollbar(Scrollbar*);
    void willRemoveVerticalScrollbar(Scrollbar*);
    void didAddHorizontalScrollbar(Scrollbar*);
    void willRemoveHorizontalScrollbar(Scrollbar*);

    bool hasOverlayScrollbars() const;
    virtual ScrollbarOverlayStyle recommendedScrollbarOverlayStyle() const { return ScrollbarOverlayStyleDefault; }

    ScrollAnimator* scrollAnimator() const { return m_scrollAnimator.get(); }
    const IntPoint& scrollOrigin() const { return m_scrollOrigin; }

    virtual bool isActive() const = 0;
    virtual int scrollSize(ScrollbarOrientation) const = 0;
    virtual int scrollPosition(Scrollbar*) const = 0;
    void invalidateScrollbar(Scrollbar*, const IntRect&);
    virtual bool isScrollCornerVisible() const = 0;
    virtual IntRect scrollCornerRect() const = 0;
    void invalidateScrollCorner();
    virtual void getTickmarks(Vector<IntRect>&) const { }

    // This function should be overriden by subclasses to perform the actual
    // scroll of the content.
    virtual void setScrollOffset(const IntPoint&) = 0;

    // Convert points and rects between the scrollbar and its containing view.
    // The client needs to implement these in order to be aware of layout effects
    // like CSS transforms.
    virtual IntRect convertFromScrollbarToContainingView(const Scrollbar* scrollbar, const IntRect& scrollbarRect) const
    {
        return scrollbar->Widget::convertToContainingView(scrollbarRect);
    }
    virtual IntRect convertFromContainingViewToScrollbar(const Scrollbar* scrollbar, const IntRect& parentRect) const
    {
        return scrollbar->Widget::convertFromContainingView(parentRect);
    }
    virtual IntPoint convertFromScrollbarToContainingView(const Scrollbar* scrollbar, const IntPoint& scrollbarPoint) const
    {
        return scrollbar->Widget::convertToContainingView(scrollbarPoint);
    }
    virtual IntPoint convertFromContainingViewToScrollbar(const Scrollbar* scrollbar, const IntPoint& parentPoint) const
    {
        return scrollbar->Widget::convertFromContainingView(parentPoint);
    }

    virtual Scrollbar* horizontalScrollbar() const { return 0; }
    virtual Scrollbar* verticalScrollbar() const { return 0; }

    virtual IntPoint scrollPosition() const { ASSERT_NOT_REACHED(); return IntPoint(); }
    virtual IntPoint minimumScrollPosition() const { ASSERT_NOT_REACHED(); return IntPoint(); }
    virtual IntPoint maximumScrollPosition() const { ASSERT_NOT_REACHED(); return IntPoint(); }
    virtual IntRect visibleContentRect(bool = false) const { ASSERT_NOT_REACHED(); return IntRect(); }
    virtual int visibleHeight() const { ASSERT_NOT_REACHED(); return 0; }
    virtual int visibleWidth() const { ASSERT_NOT_REACHED(); return 0; }
    virtual IntSize contentsSize() const { ASSERT_NOT_REACHED(); return IntSize(); }
    virtual IntSize overhangAmount() const { ASSERT_NOT_REACHED(); return IntSize(); }
    virtual IntPoint currentMousePosition() const { return IntPoint(); }
    virtual void didCompleteRubberBand(const IntSize&) const { ASSERT_NOT_REACHED(); }
    virtual bool shouldSuspendScrollAnimations() const { return true; }
    virtual void scrollbarStyleChanged() { }
    virtual void setVisibleScrollerThumbRect(const IntRect&) { }

    virtual void disconnectFromPage() { }

private:
    // NOTE: Only called from the ScrollAnimator.
    friend class ScrollAnimator;
    void setScrollOffsetFromAnimation(const IntPoint&);

    OwnPtr<ScrollAnimator> m_scrollAnimator;
    bool m_constrainsScrollingToContentEdge;

    bool m_inLiveResize;

    ScrollElasticity m_verticalScrollElasticity;
    ScrollElasticity m_horizontalScrollElasticity;

protected:
    virtual void invalidateScrollbarRect(Scrollbar*, const IntRect&) = 0;
    virtual void invalidateScrollCornerRect(const IntRect&) = 0;

#if USE(ACCELERATED_COMPOSITING)
    virtual GraphicsLayer* layerForHorizontalScrollbar() const { return 0; }
    virtual GraphicsLayer* layerForVerticalScrollbar() const { return 0; }
    virtual GraphicsLayer* layerForScrollCorner() const { return 0; }
#endif
    bool hasLayerForHorizontalScrollbar() const;
    bool hasLayerForVerticalScrollbar() const;
    bool hasLayerForScrollCorner() const;

    // There are 8 possible combinations of writing mode and direction. Scroll origin will be non-zero in the x or y axis
    // if there is any reversed direction or writing-mode. The combinations are:
    // writing-mode / direction     scrollOrigin.x() set    scrollOrigin.y() set
    // horizontal-tb / ltr          NO                      NO
    // horizontal-tb / rtl          YES                     NO
    // horizontal-bt / ltr          NO                      YES
    // horizontal-bt / rtl          YES                     YES
    // vertical-lr / ltr            NO                      NO
    // vertical-lr / rtl            NO                      YES
    // vertical-rl / ltr            YES                     NO
    // vertical-rl / rtl            YES                     YES
    IntPoint m_scrollOrigin;
};

} // namespace WebCore

#endif // ScrollableArea_h
