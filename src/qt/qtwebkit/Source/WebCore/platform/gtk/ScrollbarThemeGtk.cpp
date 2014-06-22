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
#include "ScrollbarThemeGtk.h"

#include "PlatformMouseEvent.h"
#include "RenderThemeGtk.h"
#include "ScrollView.h"
#include "Scrollbar.h"

namespace WebCore {

static HashSet<ScrollbarThemeClient*>* gScrollbars;

ScrollbarTheme* ScrollbarTheme::nativeTheme()
{
    static ScrollbarThemeGtk theme;
    return &theme;
}

ScrollbarThemeGtk::~ScrollbarThemeGtk()
{
}

void ScrollbarThemeGtk::registerScrollbar(ScrollbarThemeClient* scrollbar)
{
    if (!gScrollbars)
        gScrollbars = new HashSet<ScrollbarThemeClient*>;
    gScrollbars->add(scrollbar);
}

void ScrollbarThemeGtk::unregisterScrollbar(ScrollbarThemeClient* scrollbar)
{
    gScrollbars->remove(scrollbar);
    if (gScrollbars->isEmpty()) {
        delete gScrollbars;
        gScrollbars = 0;
    }
}

void ScrollbarThemeGtk::updateScrollbarsFrameThickness()
{
    if (!gScrollbars)
        return;

    // Update the thickness of every interior frame scrollbar widget. The
    // platform-independent scrollbar them code isn't yet smart enough to get
    // this information when it paints.
    HashSet<ScrollbarThemeClient*>::iterator end = gScrollbars->end();
    for (HashSet<ScrollbarThemeClient*>::iterator it = gScrollbars->begin(); it != end; ++it) {
        ScrollbarThemeClient* scrollbar = (*it);

        // Top-level scrollbar i.e. scrollbars who have a parent ScrollView
        // with no parent are native, and thus do not need to be resized.
        if (!scrollbar->parent() || !scrollbar->parent()->parent())
            return;

        int thickness = scrollbarThickness(scrollbar->controlSize());
        if (scrollbar->orientation() == HorizontalScrollbar)
            scrollbar->setFrameRect(IntRect(0, scrollbar->parent()->height() - thickness, scrollbar->width(), thickness));
        else
            scrollbar->setFrameRect(IntRect(scrollbar->parent()->width() - thickness, 0, thickness, scrollbar->height()));
    }
}

bool ScrollbarThemeGtk::hasThumb(ScrollbarThemeClient* scrollbar)
{
    // This method is just called as a paint-time optimization to see if
    // painting the thumb can be skipped.  We don't have to be exact here.
    return thumbLength(scrollbar) > 0;
}

IntRect ScrollbarThemeGtk::backButtonRect(ScrollbarThemeClient* scrollbar, ScrollbarPart part, bool)
{
    if (part == BackButtonEndPart && !m_hasBackButtonEndPart)
        return IntRect();
    if (part == BackButtonStartPart && !m_hasBackButtonStartPart)
        return IntRect();

    int x = scrollbar->x() + m_troughBorderWidth;
    int y = scrollbar->y() + m_troughBorderWidth;
    IntSize size = buttonSize(scrollbar);
    if (part == BackButtonStartPart)
        return IntRect(x, y, size.width(), size.height());

    // BackButtonEndPart (alternate button)
    if (scrollbar->orientation() == HorizontalScrollbar)
        return IntRect(scrollbar->x() + scrollbar->width() - m_troughBorderWidth - (2 * size.width()), y, size.width(), size.height());

    // VerticalScrollbar alternate button
    return IntRect(x, scrollbar->y() + scrollbar->height() - m_troughBorderWidth - (2 * size.height()), size.width(), size.height());
}

IntRect ScrollbarThemeGtk::forwardButtonRect(ScrollbarThemeClient* scrollbar, ScrollbarPart part, bool)
{
    if (part == ForwardButtonStartPart && !m_hasForwardButtonStartPart)
        return IntRect();
    if (part == ForwardButtonEndPart && !m_hasForwardButtonEndPart)
        return IntRect();

    IntSize size = buttonSize(scrollbar);
    if (scrollbar->orientation() == HorizontalScrollbar) {
        int y = scrollbar->y() + m_troughBorderWidth;
        if (part == ForwardButtonEndPart)
            return IntRect(scrollbar->x() + scrollbar->width() - size.width() - m_troughBorderWidth, y, size.width(), size.height());

        // ForwardButtonStartPart (alternate button)
        return IntRect(scrollbar->x() + m_troughBorderWidth + size.width(), y, size.width(), size.height());
    }

    // VerticalScrollbar
    int x = scrollbar->x() + m_troughBorderWidth;
    if (part == ForwardButtonEndPart)
        return IntRect(x, scrollbar->y() + scrollbar->height() - size.height() - m_troughBorderWidth, size.width(), size.height());

    // ForwardButtonStartPart (alternate button)
    return IntRect(x, scrollbar->y() + m_troughBorderWidth + size.height(), size.width(), size.height());
}

IntRect ScrollbarThemeGtk::trackRect(ScrollbarThemeClient* scrollbar, bool)
{
    // The padding along the thumb movement axis includes the trough border
    // plus the size of stepper spacing (the space between the stepper and
    // the place where the thumb stops). There is often no stepper spacing.
    int movementAxisPadding = m_troughBorderWidth + m_stepperSpacing;

    // The fatness of the scrollbar on the non-movement axis.
    int thickness = scrollbarThickness(scrollbar->controlSize());

    int startButtonsOffset = 0;
    int buttonsWidth = 0;
    if (m_hasForwardButtonStartPart) {
        startButtonsOffset += m_stepperSize;
        buttonsWidth += m_stepperSize;
    }
    if (m_hasBackButtonStartPart) {
        startButtonsOffset += m_stepperSize;
        buttonsWidth += m_stepperSize;
    }
    if (m_hasBackButtonEndPart)
        buttonsWidth += m_stepperSize;
    if (m_hasForwardButtonEndPart)
        buttonsWidth += m_stepperSize;

    if (scrollbar->orientation() == HorizontalScrollbar) {
        // Once the scrollbar becomes smaller than the natural size of the
        // two buttons, the track disappears.
        if (scrollbar->width() < 2 * thickness)
            return IntRect();
        return IntRect(scrollbar->x() + movementAxisPadding + startButtonsOffset, scrollbar->y(),
                       scrollbar->width() - (2 * movementAxisPadding) - buttonsWidth, thickness);
    }

    if (scrollbar->height() < 2 * thickness)
        return IntRect();
    return IntRect(scrollbar->x(), scrollbar->y() + movementAxisPadding + startButtonsOffset,
                   thickness, scrollbar->height() - (2 * movementAxisPadding) - buttonsWidth);
}

IntRect ScrollbarThemeGtk::thumbRect(ScrollbarThemeClient* scrollbar, const IntRect& unconstrainedTrackRect)
{
    IntRect trackRect = constrainTrackRectToTrackPieces(scrollbar, unconstrainedTrackRect);
    int thumbPos = thumbPosition(scrollbar);
    if (scrollbar->orientation() == HorizontalScrollbar)
        return IntRect(trackRect.x() + thumbPos, trackRect.y() + (trackRect.height() - m_thumbFatness) / 2, thumbLength(scrollbar), m_thumbFatness); 

    // VerticalScrollbar
    return IntRect(trackRect.x() + (trackRect.width() - m_thumbFatness) / 2, trackRect.y() + thumbPos, m_thumbFatness, thumbLength(scrollbar));
}

bool ScrollbarThemeGtk::paint(ScrollbarThemeClient* scrollbar, GraphicsContext* graphicsContext, const IntRect& damageRect)
{
    if (graphicsContext->paintingDisabled())
        return false;

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

    IntRect trackPaintRect = trackRect(scrollbar, true);
    if (damageRect.intersects(trackPaintRect))
        scrollMask |= TrackBGPart;

    if (m_troughUnderSteppers && (scrollMask & BackButtonStartPart
            || scrollMask & BackButtonEndPart
            || scrollMask & ForwardButtonStartPart
            || scrollMask & ForwardButtonEndPart))
        scrollMask |= TrackBGPart;

    bool thumbPresent = hasThumb(scrollbar);
    IntRect currentThumbRect;
    if (thumbPresent) {
        IntRect track = trackRect(scrollbar, false);
        currentThumbRect = thumbRect(scrollbar, track);
        if (damageRect.intersects(currentThumbRect))
            scrollMask |= ThumbPart;
    }

    ScrollbarControlPartMask allButtons = BackButtonStartPart | BackButtonEndPart
                                         | ForwardButtonStartPart | ForwardButtonEndPart;
    if (scrollMask & TrackBGPart || scrollMask & ThumbPart || scrollMask & allButtons)
        paintScrollbarBackground(graphicsContext, scrollbar);
        paintTrackBackground(graphicsContext, scrollbar, trackPaintRect);

    // Paint the back and forward buttons.
    if (scrollMask & BackButtonStartPart)
        paintButton(graphicsContext, scrollbar, backButtonStartPaintRect, BackButtonStartPart);
    if (scrollMask & BackButtonEndPart)
        paintButton(graphicsContext, scrollbar, backButtonEndPaintRect, BackButtonEndPart);
    if (scrollMask & ForwardButtonStartPart)
        paintButton(graphicsContext, scrollbar, forwardButtonStartPaintRect, ForwardButtonStartPart);
    if (scrollMask & ForwardButtonEndPart)
        paintButton(graphicsContext, scrollbar, forwardButtonEndPaintRect, ForwardButtonEndPart);

    // Paint the thumb.
    if (scrollMask & ThumbPart)
        paintThumb(graphicsContext, scrollbar, currentThumbRect);

    return true;
}

bool ScrollbarThemeGtk::shouldCenterOnThumb(ScrollbarThemeClient*, const PlatformMouseEvent& event)
{
    return (event.shiftKey() && event.button() == LeftButton) || (event.button() == MiddleButton);
}

int ScrollbarThemeGtk::scrollbarThickness(ScrollbarControlSize)
{
    return m_thumbFatness + (m_troughBorderWidth * 2);
}

IntSize ScrollbarThemeGtk::buttonSize(ScrollbarThemeClient* scrollbar)
{
    if (scrollbar->orientation() == VerticalScrollbar)
        return IntSize(m_thumbFatness, m_stepperSize);

    // HorizontalScrollbar
    return IntSize(m_stepperSize, m_thumbFatness);
}

int ScrollbarThemeGtk::minimumThumbLength(ScrollbarThemeClient* scrollbar)
{
    return m_minThumbLength;
}

}

