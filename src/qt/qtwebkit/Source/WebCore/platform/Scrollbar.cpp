/*
 * Copyright (C) 2004, 2006, 2008 Apple Inc. All rights reserved.
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
 */

#include "config.h"
#include "Scrollbar.h"

#include "GraphicsContext.h"
#include "PlatformMouseEvent.h"
#include "ScrollAnimator.h"
#include "ScrollView.h"
#include "ScrollableArea.h"
#include "ScrollbarTheme.h"
#include <algorithm>

#if ENABLE(GESTURE_EVENTS)
#include "PlatformGestureEvent.h"
#endif

using namespace std;

#if PLATFORM(GTK)
// The position of the scrollbar thumb affects the appearance of the steppers, so
// when the thumb moves, we have to invalidate them for painting.
#define THUMB_POSITION_AFFECTS_BUTTONS
#endif

namespace WebCore {

#if !PLATFORM(EFL)
PassRefPtr<Scrollbar> Scrollbar::createNativeScrollbar(ScrollableArea* scrollableArea, ScrollbarOrientation orientation, ScrollbarControlSize size)
{
    return adoptRef(new Scrollbar(scrollableArea, orientation, size));
}
#endif

int Scrollbar::maxOverlapBetweenPages()
{
    static int maxOverlapBetweenPages = ScrollbarTheme::theme()->maxOverlapBetweenPages();
    return maxOverlapBetweenPages;
}

Scrollbar::Scrollbar(ScrollableArea* scrollableArea, ScrollbarOrientation orientation, ScrollbarControlSize controlSize,
                     ScrollbarTheme* theme)
    : m_scrollableArea(scrollableArea)
    , m_orientation(orientation)
    , m_controlSize(controlSize)
    , m_theme(theme)
    , m_visibleSize(0)
    , m_totalSize(0)
    , m_currentPos(0)
    , m_dragOrigin(0)
    , m_lineStep(0)
    , m_pageStep(0)
    , m_pixelStep(1)
    , m_hoveredPart(NoPart)
    , m_pressedPart(NoPart)
    , m_pressedPos(0)
    , m_scrollPos(0)
    , m_draggingDocument(false)
    , m_documentDragPos(0)
    , m_enabled(true)
    , m_scrollTimer(this, &Scrollbar::autoscrollTimerFired)
    , m_overlapsResizer(false)
    , m_suppressInvalidation(false)
    , m_isAlphaLocked(false)
{
    if (!m_theme)
        m_theme = ScrollbarTheme::theme();

    m_theme->registerScrollbar(this);

    // FIXME: This is ugly and would not be necessary if we fix cross-platform code to actually query for
    // scrollbar thickness and use it when sizing scrollbars (rather than leaving one dimension of the scrollbar
    // alone when sizing).
    int thickness = m_theme->scrollbarThickness(controlSize);
    Widget::setFrameRect(IntRect(0, 0, thickness, thickness));

    if (m_scrollableArea)
        m_currentPos = static_cast<float>(m_scrollableArea->scrollPosition(this));
}

Scrollbar::~Scrollbar()
{
    stopTimerIfNeeded();
    
    m_theme->unregisterScrollbar(this);
}

ScrollbarOverlayStyle Scrollbar::scrollbarOverlayStyle() const
{
    return m_scrollableArea ? m_scrollableArea->scrollbarOverlayStyle() : ScrollbarOverlayStyleDefault;
}

void Scrollbar::getTickmarks(Vector<IntRect>& tickmarks) const
{
    if (m_scrollableArea)
        m_scrollableArea->getTickmarks(tickmarks);
}

bool Scrollbar::isScrollableAreaActive() const
{
    return m_scrollableArea && m_scrollableArea->isActive();
}

bool Scrollbar::isScrollViewScrollbar() const
{
    return parent() && parent()->isScrollViewScrollbar(this);
}

void Scrollbar::offsetDidChange()
{
    ASSERT(m_scrollableArea);

    float position = static_cast<float>(m_scrollableArea->scrollPosition(this));
    if (position == m_currentPos)
        return;

    int oldThumbPosition = theme()->thumbPosition(this);
    m_currentPos = position;
    updateThumbPosition();
    if (m_pressedPart == ThumbPart)
        setPressedPos(m_pressedPos + theme()->thumbPosition(this) - oldThumbPosition);    
}

void Scrollbar::setProportion(int visibleSize, int totalSize)
{
    if (visibleSize == m_visibleSize && totalSize == m_totalSize)
        return;

    m_visibleSize = visibleSize;
    m_totalSize = totalSize;

    updateThumbProportion();
}

void Scrollbar::setSteps(int lineStep, int pageStep, int pixelsPerStep)
{
    m_lineStep = lineStep;
    m_pageStep = pageStep;
    m_pixelStep = 1.0f / pixelsPerStep;
}

void Scrollbar::updateThumb()
{
#ifdef THUMB_POSITION_AFFECTS_BUTTONS
    invalidate();
#else
    theme()->invalidateParts(this, ForwardTrackPart | BackTrackPart | ThumbPart);
#endif
}

void Scrollbar::updateThumbPosition()
{
    updateThumb();
}

void Scrollbar::updateThumbProportion()
{
    updateThumb();
}

void Scrollbar::paint(GraphicsContext* context, const IntRect& damageRect)
{
    if (context->updatingControlTints() && theme()->supportsControlTints()) {
        invalidate();
        return;
    }

    if (context->paintingDisabled() || !frameRect().intersects(damageRect))
        return;

    if (!theme()->paint(this, context, damageRect))
        Widget::paint(context, damageRect);
}

void Scrollbar::autoscrollTimerFired(Timer<Scrollbar>*)
{
    autoscrollPressedPart(theme()->autoscrollTimerDelay());
}

static bool thumbUnderMouse(Scrollbar* scrollbar)
{
    int thumbPos = scrollbar->theme()->trackPosition(scrollbar) + scrollbar->theme()->thumbPosition(scrollbar);
    int thumbLength = scrollbar->theme()->thumbLength(scrollbar);
    return scrollbar->pressedPos() >= thumbPos && scrollbar->pressedPos() < thumbPos + thumbLength;
}

void Scrollbar::autoscrollPressedPart(double delay)
{
    // Don't do anything for the thumb or if nothing was pressed.
    if (m_pressedPart == ThumbPart || m_pressedPart == NoPart)
        return;

    // Handle the track.
    if ((m_pressedPart == BackTrackPart || m_pressedPart == ForwardTrackPart) && thumbUnderMouse(this)) {
        theme()->invalidatePart(this, m_pressedPart);
        setHoveredPart(ThumbPart);
        return;
    }

    // Handle the arrows and track.
    if (m_scrollableArea && m_scrollableArea->scroll(pressedPartScrollDirection(), pressedPartScrollGranularity()))
        startTimerIfNeeded(delay);
}

void Scrollbar::startTimerIfNeeded(double delay)
{
    // Don't do anything for the thumb.
    if (m_pressedPart == ThumbPart)
        return;

    // Handle the track.  We halt track scrolling once the thumb is level
    // with us.
    if ((m_pressedPart == BackTrackPart || m_pressedPart == ForwardTrackPart) && thumbUnderMouse(this)) {
        theme()->invalidatePart(this, m_pressedPart);
        setHoveredPart(ThumbPart);
        return;
    }

    // We can't scroll if we've hit the beginning or end.
    ScrollDirection dir = pressedPartScrollDirection();
    if (dir == ScrollUp || dir == ScrollLeft) {
        if (m_currentPos == 0)
            return;
    } else {
        if (m_currentPos == maximum())
            return;
    }

    m_scrollTimer.startOneShot(delay);
}

void Scrollbar::stopTimerIfNeeded()
{
    if (m_scrollTimer.isActive())
        m_scrollTimer.stop();
}

ScrollDirection Scrollbar::pressedPartScrollDirection()
{
    if (m_orientation == HorizontalScrollbar) {
        if (m_pressedPart == BackButtonStartPart || m_pressedPart == BackButtonEndPart || m_pressedPart == BackTrackPart)
            return ScrollLeft;
        return ScrollRight;
    } else {
        if (m_pressedPart == BackButtonStartPart || m_pressedPart == BackButtonEndPart || m_pressedPart == BackTrackPart)
            return ScrollUp;
        return ScrollDown;
    }
}

ScrollGranularity Scrollbar::pressedPartScrollGranularity()
{
    if (m_pressedPart == BackButtonStartPart || m_pressedPart == BackButtonEndPart ||  m_pressedPart == ForwardButtonStartPart || m_pressedPart == ForwardButtonEndPart)
        return ScrollByLine;
    return ScrollByPage;
}

void Scrollbar::moveThumb(int pos, bool draggingDocument)
{
    if (!m_scrollableArea)
        return;

    int delta = pos - m_pressedPos;

    if (draggingDocument) {
        if (m_draggingDocument)
            delta = pos - m_documentDragPos;
        m_draggingDocument = true;
        FloatPoint currentPosition = m_scrollableArea->scrollAnimator()->currentPosition();
        int destinationPosition = (m_orientation == HorizontalScrollbar ? currentPosition.x() : currentPosition.y()) + delta;
        if (delta > 0)
            destinationPosition = min(destinationPosition + delta, maximum());
        else if (delta < 0)
            destinationPosition = max(destinationPosition + delta, 0);
        m_scrollableArea->scrollToOffsetWithoutAnimation(m_orientation, destinationPosition);
        m_documentDragPos = pos;
        return;
    }

    if (m_draggingDocument) {
        delta += m_pressedPos - m_documentDragPos;
        m_draggingDocument = false;
    }

    // Drag the thumb.
    int thumbPos = theme()->thumbPosition(this);
    int thumbLen = theme()->thumbLength(this);
    int trackLen = theme()->trackLength(this);
    int maxPos = trackLen - thumbLen;
    if (delta > 0)
        delta = min(maxPos - thumbPos, delta);
    else if (delta < 0)
        delta = max(-thumbPos, delta);
    
    if (delta) {
        float newPosition = static_cast<float>(thumbPos + delta) * maximum() / (trackLen - thumbLen);
        m_scrollableArea->scrollToOffsetWithoutAnimation(m_orientation, newPosition);
    }
}

void Scrollbar::setHoveredPart(ScrollbarPart part)
{
    if (part == m_hoveredPart)
        return;

    if ((m_hoveredPart == NoPart || part == NoPart) && theme()->invalidateOnMouseEnterExit())
        invalidate();  // Just invalidate the whole scrollbar, since the buttons at either end change anyway.
    else if (m_pressedPart == NoPart) {  // When there's a pressed part, we don't draw a hovered state, so there's no reason to invalidate.
        theme()->invalidatePart(this, part);
        theme()->invalidatePart(this, m_hoveredPart);
    }
    m_hoveredPart = part;
}

void Scrollbar::setPressedPart(ScrollbarPart part)
{
    if (m_pressedPart != NoPart)
        theme()->invalidatePart(this, m_pressedPart);
    m_pressedPart = part;
    if (m_pressedPart != NoPart)
        theme()->invalidatePart(this, m_pressedPart);
    else if (m_hoveredPart != NoPart)  // When we no longer have a pressed part, we can start drawing a hovered state on the hovered part.
        theme()->invalidatePart(this, m_hoveredPart);
}

#if ENABLE(GESTURE_EVENTS)
bool Scrollbar::gestureEvent(const PlatformGestureEvent& evt)
{
    bool handled = false;
    switch (evt.type()) {
    case PlatformEvent::GestureTapDown:
        setPressedPart(theme()->hitTest(this, evt.position()));
        m_pressedPos = (orientation() == HorizontalScrollbar ? convertFromContainingWindow(evt.position()).x() : convertFromContainingWindow(evt.position()).y());
        return true;
    case PlatformEvent::GestureTapDownCancel:
    case PlatformEvent::GestureScrollBegin:
        if (m_pressedPart == ThumbPart) {
            m_scrollPos = m_pressedPos;
            return true;
        }
        break;
    case PlatformEvent::GestureScrollUpdate:
    case PlatformEvent::GestureScrollUpdateWithoutPropagation:
        if (m_pressedPart == ThumbPart) {
            m_scrollPos += HorizontalScrollbar ? evt.deltaX() : evt.deltaY();
            moveThumb(m_scrollPos, false);
            return true;
        }
        break;
    case PlatformEvent::GestureScrollEnd:
        m_scrollPos = 0;
        break;
    case PlatformEvent::GestureTap:
        if (m_pressedPart != ThumbPart && m_pressedPart != NoPart)
            handled = m_scrollableArea && m_scrollableArea->scroll(pressedPartScrollDirection(), pressedPartScrollGranularity());
        break;
    default:
        break;
    }
    setPressedPart(NoPart);
    m_pressedPos = 0;
    return handled;
}
#endif

bool Scrollbar::mouseMoved(const PlatformMouseEvent& evt)
{
    if (m_pressedPart == ThumbPart) {
        if (theme()->shouldSnapBackToDragOrigin(this, evt)) {
            if (m_scrollableArea)
                m_scrollableArea->scrollToOffsetWithoutAnimation(m_orientation, m_dragOrigin);
        } else {
            moveThumb(m_orientation == HorizontalScrollbar ? 
                      convertFromContainingWindow(evt.position()).x() :
                      convertFromContainingWindow(evt.position()).y(), theme()->shouldDragDocumentInsteadOfThumb(this, evt));
        }
        return true;
    }

    if (m_pressedPart != NoPart)
        m_pressedPos = (orientation() == HorizontalScrollbar ? convertFromContainingWindow(evt.position()).x() : convertFromContainingWindow(evt.position()).y());

    ScrollbarPart part = theme()->hitTest(this, evt.position());
    if (part != m_hoveredPart) {
        if (m_pressedPart != NoPart) {
            if (part == m_pressedPart) {
                // The mouse is moving back over the pressed part.  We
                // need to start up the timer action again.
                startTimerIfNeeded(theme()->autoscrollTimerDelay());
                theme()->invalidatePart(this, m_pressedPart);
            } else if (m_hoveredPart == m_pressedPart) {
                // The mouse is leaving the pressed part.  Kill our timer
                // if needed.
                stopTimerIfNeeded();
                theme()->invalidatePart(this, m_pressedPart);
            }
        } 
        
        setHoveredPart(part);
    } 

    return true;
}

void Scrollbar::mouseEntered()
{
    if (m_scrollableArea)
        m_scrollableArea->mouseEnteredScrollbar(this);
}

bool Scrollbar::mouseExited()
{
    if (m_scrollableArea)
        m_scrollableArea->mouseExitedScrollbar(this);
    setHoveredPart(NoPart);
    return true;
}

bool Scrollbar::mouseUp(const PlatformMouseEvent& mouseEvent)
{
    setPressedPart(NoPart);
    m_pressedPos = 0;
    m_draggingDocument = false;
    stopTimerIfNeeded();

    if (m_scrollableArea) {
        // m_hoveredPart won't be updated until the next mouseMoved or mouseDown, so we have to hit test
        // to really know if the mouse has exited the scrollbar on a mouseUp.
        ScrollbarPart part = theme()->hitTest(this, mouseEvent.position());
        if (part == NoPart)
            m_scrollableArea->mouseExitedScrollbar(this);
    }

    return true;
}

bool Scrollbar::mouseDown(const PlatformMouseEvent& evt)
{
    // Early exit for right click
    if (evt.button() == RightButton)
        return true; // FIXME: Handled as context menu by Qt right now.  Should just avoid even calling this method on a right click though.

    setPressedPart(theme()->hitTest(this, evt.position()));
    int pressedPos = (orientation() == HorizontalScrollbar ? convertFromContainingWindow(evt.position()).x() : convertFromContainingWindow(evt.position()).y());
    
    if ((m_pressedPart == BackTrackPart || m_pressedPart == ForwardTrackPart) && theme()->shouldCenterOnThumb(this, evt)) {
        setHoveredPart(ThumbPart);
        setPressedPart(ThumbPart);
        m_dragOrigin = m_currentPos;
        int thumbLen = theme()->thumbLength(this);
        int desiredPos = pressedPos;
        // Set the pressed position to the middle of the thumb so that when we do the move, the delta
        // will be from the current pixel position of the thumb to the new desired position for the thumb.
        m_pressedPos = theme()->trackPosition(this) + theme()->thumbPosition(this) + thumbLen / 2;
        moveThumb(desiredPos);
        return true;
    } else if (m_pressedPart == ThumbPart)
        m_dragOrigin = m_currentPos;
    
    m_pressedPos = pressedPos;

    autoscrollPressedPart(theme()->initialAutoscrollTimerDelay());
    return true;
}

void Scrollbar::setFrameRect(const IntRect& rect)
{
    // Get our window resizer rect and see if we overlap. Adjust to avoid the overlap
    // if necessary.
    IntRect adjustedRect(rect);
    bool overlapsResizer = false;
    ScrollView* view = parent();
    if (view && !rect.isEmpty() && !view->windowResizerRect().isEmpty()) {
        IntRect resizerRect = view->convertFromContainingWindow(view->windowResizerRect());
        if (rect.intersects(resizerRect)) {
            if (orientation() == HorizontalScrollbar) {
                int overlap = rect.maxX() - resizerRect.x();
                if (overlap > 0 && resizerRect.maxX() >= rect.maxX()) {
                    adjustedRect.setWidth(rect.width() - overlap);
                    overlapsResizer = true;
                }
            } else {
                int overlap = rect.maxY() - resizerRect.y();
                if (overlap > 0 && resizerRect.maxY() >= rect.maxY()) {
                    adjustedRect.setHeight(rect.height() - overlap);
                    overlapsResizer = true;
                }
            }
        }
    }
    if (overlapsResizer != m_overlapsResizer) {
        m_overlapsResizer = overlapsResizer;
        if (view)
            view->adjustScrollbarsAvoidingResizerCount(m_overlapsResizer ? 1 : -1);
    }

    Widget::setFrameRect(adjustedRect);
}

void Scrollbar::setParent(ScrollView* parentView)
{
    if (!parentView && m_overlapsResizer && parent())
        parent()->adjustScrollbarsAvoidingResizerCount(-1);
    Widget::setParent(parentView);
}

void Scrollbar::setEnabled(bool e)
{ 
    if (m_enabled == e)
        return;
    m_enabled = e;
    theme()->updateEnabledState(this);
    invalidate();
}

bool Scrollbar::isOverlayScrollbar() const
{
    return m_theme->usesOverlayScrollbars();
}

bool Scrollbar::shouldParticipateInHitTesting()
{
    // Non-overlay scrollbars should always participate in hit testing.
    if (!isOverlayScrollbar())
        return true;
    return m_scrollableArea->scrollAnimator()->shouldScrollbarParticipateInHitTesting(this);
}

bool Scrollbar::isWindowActive() const
{
    return m_scrollableArea && m_scrollableArea->isActive();
}

void Scrollbar::invalidateRect(const IntRect& rect)
{
    if (suppressInvalidation())
        return;

    if (m_scrollableArea)
        m_scrollableArea->invalidateScrollbar(this, rect);
}

IntRect Scrollbar::convertToContainingView(const IntRect& localRect) const
{
    if (m_scrollableArea)
        return m_scrollableArea->convertFromScrollbarToContainingView(this, localRect);

    return Widget::convertToContainingView(localRect);
}

IntRect Scrollbar::convertFromContainingView(const IntRect& parentRect) const
{
    if (m_scrollableArea)
        return m_scrollableArea->convertFromContainingViewToScrollbar(this, parentRect);

    return Widget::convertFromContainingView(parentRect);
}

IntPoint Scrollbar::convertToContainingView(const IntPoint& localPoint) const
{
    if (m_scrollableArea)
        return m_scrollableArea->convertFromScrollbarToContainingView(this, localPoint);

    return Widget::convertToContainingView(localPoint);
}

IntPoint Scrollbar::convertFromContainingView(const IntPoint& parentPoint) const
{
    if (m_scrollableArea)
        return m_scrollableArea->convertFromContainingViewToScrollbar(this, parentPoint);

    return Widget::convertFromContainingView(parentPoint);
}

} // namespace WebCore
