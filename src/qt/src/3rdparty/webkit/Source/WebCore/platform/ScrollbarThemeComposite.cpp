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

#include "config.h"
#include "ScrollbarThemeComposite.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "Page.h"
#include "PlatformMouseEvent.h"
#include "Scrollbar.h"
#include "ScrollableArea.h"
#include "Settings.h"

using namespace std;

namespace WebCore {

#if PLATFORM(WIN)
static Page* pageForScrollView(ScrollView* view)
{
    if (!view)
        return 0;
    if (!view->isFrameView())
        return 0;
    FrameView* frameView = static_cast<FrameView*>(view);
    if (!frameView->frame())
        return 0;
    return frameView->frame()->page();
}
#endif

bool ScrollbarThemeComposite::paint(Scrollbar* scrollbar, GraphicsContext* graphicsContext, const IntRect& damageRect)
{
    // Create the ScrollbarControlPartMask based on the damageRect
    ScrollbarControlPartMask scrollMask = NoPart;

    IntRect backButtonStartPaintRect;
    IntRect backButtonEndPaintRect;
    IntRect forwardButtonStartPaintRect;
    IntRect forwardButtonEndPaintRect;
    if (hasButtons(scrollbar)) {
        backButtonStartPaintRect = backButtonRect(scrollbar, BackButtonStartPart, true);
        if (damageRect.intersects(backButtonStartPaintRect))
            scrollMask |= BackButtonStartPart;
        backButtonEndPaintRect = backButtonRect(scrollbar, BackButtonEndPart, true);
        if (damageRect.intersects(backButtonEndPaintRect))
            scrollMask |= BackButtonEndPart;
        forwardButtonStartPaintRect = forwardButtonRect(scrollbar, ForwardButtonStartPart, true);
        if (damageRect.intersects(forwardButtonStartPaintRect))
            scrollMask |= ForwardButtonStartPart;
        forwardButtonEndPaintRect = forwardButtonRect(scrollbar, ForwardButtonEndPart, true);
        if (damageRect.intersects(forwardButtonEndPaintRect))
            scrollMask |= ForwardButtonEndPart;
    }

    IntRect startTrackRect;
    IntRect thumbRect;
    IntRect endTrackRect;
    IntRect trackPaintRect = trackRect(scrollbar, true);
    if (damageRect.intersects(trackPaintRect))
        scrollMask |= TrackBGPart;
    bool thumbPresent = hasThumb(scrollbar);
    if (thumbPresent) {
        IntRect track = trackRect(scrollbar);
        splitTrack(scrollbar, track, startTrackRect, thumbRect, endTrackRect);
        if (damageRect.intersects(thumbRect))
            scrollMask |= ThumbPart;
        if (damageRect.intersects(startTrackRect))
            scrollMask |= BackTrackPart;
        if (damageRect.intersects(endTrackRect))
            scrollMask |= ForwardTrackPart;
    }

#if PLATFORM(WIN)
    // FIXME: This API makes the assumption that the custom scrollbar's metrics will match
    // the theme's metrics.  This is not a valid assumption.  The ability for a client to paint
    // custom scrollbars should be removed once scrollbars can be styled via CSS.
    if (Page* page = pageForScrollView(scrollbar->parent())) {
        if (page->settings()->shouldPaintCustomScrollbars()) {
            float proportion = static_cast<float>(scrollbar->visibleSize()) / scrollbar->totalSize();
            float value = scrollbar->currentPos() / static_cast<float>(scrollbar->maximum());
            ScrollbarControlState s = 0;
            if (scrollbar->scrollableArea()->isActive())
                s |= ActiveScrollbarState;
            if (scrollbar->enabled())
                s |= EnabledScrollbarState;
            if (scrollbar->pressedPart() != NoPart)
                s |= PressedScrollbarState;
            if (page->chrome()->client()->paintCustomScrollbar(graphicsContext,
                                                               scrollbar->frameRect(), 
                                                               scrollbar->controlSize(),
                                                               s, 
                                                               scrollbar->pressedPart(), 
                                                               scrollbar->orientation() == VerticalScrollbar,
                                                               value,
                                                               proportion,
                                                               scrollMask))
                return true;
        }
    }
#endif

    // Paint the scrollbar background (only used by custom CSS scrollbars).
    paintScrollbarBackground(graphicsContext, scrollbar);

    // Paint the back and forward buttons.
    if (scrollMask & BackButtonStartPart)
        paintButton(graphicsContext, scrollbar, backButtonStartPaintRect, BackButtonStartPart);
    if (scrollMask & BackButtonEndPart)
        paintButton(graphicsContext, scrollbar, backButtonEndPaintRect, BackButtonEndPart);
    if (scrollMask & ForwardButtonStartPart)
        paintButton(graphicsContext, scrollbar, forwardButtonStartPaintRect, ForwardButtonStartPart);
    if (scrollMask & ForwardButtonEndPart)
        paintButton(graphicsContext, scrollbar, forwardButtonEndPaintRect, ForwardButtonEndPart);
    
    if (scrollMask & TrackBGPart)
        paintTrackBackground(graphicsContext, scrollbar, trackPaintRect);
    
    if ((scrollMask & ForwardTrackPart) || (scrollMask & BackTrackPart)) {
        // Paint the track pieces above and below the thumb.
        if (scrollMask & BackTrackPart)
            paintTrackPiece(graphicsContext, scrollbar, startTrackRect, BackTrackPart);
        if (scrollMask & ForwardTrackPart)
            paintTrackPiece(graphicsContext, scrollbar, endTrackRect, ForwardTrackPart);

        paintTickmarks(graphicsContext, scrollbar, trackPaintRect);
    }

    // Paint the thumb.
    if (scrollMask & ThumbPart)
        paintThumb(graphicsContext, scrollbar, thumbRect);

    return true;
}

ScrollbarPart ScrollbarThemeComposite::hitTest(Scrollbar* scrollbar, const PlatformMouseEvent& evt)
{
    ScrollbarPart result = NoPart;
    if (!scrollbar->enabled())
        return result;

    IntPoint mousePosition = scrollbar->convertFromContainingWindow(evt.pos());
    mousePosition.move(scrollbar->x(), scrollbar->y());
    
    if (!scrollbar->frameRect().contains(mousePosition))
        return NoPart;

    result = ScrollbarBGPart;

    IntRect track = trackRect(scrollbar);
    if (track.contains(mousePosition)) {
        IntRect beforeThumbRect;
        IntRect thumbRect;
        IntRect afterThumbRect;
        splitTrack(scrollbar, track, beforeThumbRect, thumbRect, afterThumbRect);
        if (thumbRect.contains(mousePosition))
            result = ThumbPart;
        else if (beforeThumbRect.contains(mousePosition))
            result = BackTrackPart;
        else if (afterThumbRect.contains(mousePosition))
            result = ForwardTrackPart;
        else
            result = TrackBGPart;
    } else if (backButtonRect(scrollbar, BackButtonStartPart).contains(mousePosition))
        result = BackButtonStartPart;
    else if (backButtonRect(scrollbar, BackButtonEndPart).contains(mousePosition))
        result = BackButtonEndPart;
    else if (forwardButtonRect(scrollbar, ForwardButtonStartPart).contains(mousePosition))
        result = ForwardButtonStartPart;
    else if (forwardButtonRect(scrollbar, ForwardButtonEndPart).contains(mousePosition))
        result = ForwardButtonEndPart;
    return result;
}

void ScrollbarThemeComposite::invalidatePart(Scrollbar* scrollbar, ScrollbarPart part)
{
    if (part == NoPart)
        return;

    IntRect result;    
    switch (part) {
        case BackButtonStartPart:
            result = backButtonRect(scrollbar, BackButtonStartPart, true);
            break;
        case BackButtonEndPart:
            result = backButtonRect(scrollbar, BackButtonEndPart, true);
            break;
        case ForwardButtonStartPart:
            result = forwardButtonRect(scrollbar, ForwardButtonStartPart, true);
            break;
        case ForwardButtonEndPart:
            result = forwardButtonRect(scrollbar, ForwardButtonEndPart, true);
            break;
        case TrackBGPart:
            result = trackRect(scrollbar, true);
            break;
        case ScrollbarBGPart:
            result = scrollbar->frameRect();
            break;
        default: {
            IntRect beforeThumbRect, thumbRect, afterThumbRect;
            splitTrack(scrollbar, trackRect(scrollbar), beforeThumbRect, thumbRect, afterThumbRect);
            if (part == BackTrackPart)
                result = beforeThumbRect;
            else if (part == ForwardTrackPart)
                result = afterThumbRect;
            else
                result = thumbRect;
        }
    }
    result.move(-scrollbar->x(), -scrollbar->y());
    scrollbar->invalidateRect(result);
}

void ScrollbarThemeComposite::splitTrack(Scrollbar* scrollbar, const IntRect& unconstrainedTrackRect, IntRect& beforeThumbRect, IntRect& thumbRect, IntRect& afterThumbRect)
{
    // This function won't even get called unless we're big enough to have some combination of these three rects where at least
    // one of them is non-empty.
    IntRect trackRect = constrainTrackRectToTrackPieces(scrollbar, unconstrainedTrackRect);
    int thickness = scrollbar->orientation() == HorizontalScrollbar ? scrollbar->height() : scrollbar->width();
    int thumbPos = thumbPosition(scrollbar);
    if (scrollbar->orientation() == HorizontalScrollbar) {
        thumbRect = IntRect(trackRect.x() + thumbPos, trackRect.y() + (trackRect.height() - thickness) / 2, thumbLength(scrollbar), thickness); 
        beforeThumbRect = IntRect(trackRect.x(), trackRect.y(), thumbPos + thumbRect.width() / 2, trackRect.height()); 
        afterThumbRect = IntRect(trackRect.x() + beforeThumbRect.width(), trackRect.y(), trackRect.maxX() - beforeThumbRect.maxX(), trackRect.height());
    } else {
        thumbRect = IntRect(trackRect.x() + (trackRect.width() - thickness) / 2, trackRect.y() + thumbPos, thickness, thumbLength(scrollbar));
        beforeThumbRect = IntRect(trackRect.x(), trackRect.y(), trackRect.width(), thumbPos + thumbRect.height() / 2); 
        afterThumbRect = IntRect(trackRect.x(), trackRect.y() + beforeThumbRect.height(), trackRect.width(), trackRect.maxY() - beforeThumbRect.maxY());
    }
}

// Returns the size represented by track taking into account scrolling past
// the end of the document.
static float usedTotalSize(Scrollbar* scrollbar)
{
    float overhangAtStart = -scrollbar->currentPos();
    float overhangAtEnd = scrollbar->currentPos() + scrollbar->visibleSize() - scrollbar->totalSize();
    float overhang = max(0.0f, max(overhangAtStart, overhangAtEnd));
    return scrollbar->totalSize() + overhang;
}

int ScrollbarThemeComposite::thumbPosition(Scrollbar* scrollbar)
{
    if (scrollbar->enabled())
        return max(0.0f, scrollbar->currentPos()) * (trackLength(scrollbar) - thumbLength(scrollbar)) / (usedTotalSize(scrollbar) - scrollbar->visibleSize());
    return 0;
}

int ScrollbarThemeComposite::thumbLength(Scrollbar* scrollbar)
{
    if (!scrollbar->enabled())
        return 0;

    float proportion = scrollbar->visibleSize() / usedTotalSize(scrollbar);
    int trackLen = trackLength(scrollbar);
    int length = proportion * trackLen;
    length = max(length, minimumThumbLength(scrollbar));
    if (length > trackLen)
        length = 0; // Once the thumb is below the track length, it just goes away (to make more room for the track).
    return length;
}

int ScrollbarThemeComposite::minimumThumbLength(Scrollbar* scrollbar)
{
    return scrollbarThickness(scrollbar->controlSize());
}

int ScrollbarThemeComposite::trackPosition(Scrollbar* scrollbar)
{
    IntRect constrainedTrackRect = constrainTrackRectToTrackPieces(scrollbar, trackRect(scrollbar));
    return (scrollbar->orientation() == HorizontalScrollbar) ? constrainedTrackRect.x() - scrollbar->x() : constrainedTrackRect.y() - scrollbar->y();
}

int ScrollbarThemeComposite::trackLength(Scrollbar* scrollbar)
{
    IntRect constrainedTrackRect = constrainTrackRectToTrackPieces(scrollbar, trackRect(scrollbar));
    return (scrollbar->orientation() == HorizontalScrollbar) ? constrainedTrackRect.width() : constrainedTrackRect.height();
}

void ScrollbarThemeComposite::paintScrollCorner(ScrollView* view, GraphicsContext* context, const IntRect& cornerRect)
{
    FrameView* frameView = static_cast<FrameView*>(view);
    Page* page = frameView->frame() ? frameView->frame()->page() : 0;
    if (page && page->settings()->shouldPaintCustomScrollbars() && page->chrome()->client()->paintCustomScrollCorner(context, cornerRect))
        return;
    context->fillRect(cornerRect, Color::white, ColorSpaceDeviceRGB);
}

}
