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
#include "ScrollbarThemeSafari.h"

#if USE(SAFARI_THEME)

#include "GraphicsContext.h"
#include "IntRect.h"
#include "Page.h"
#include "PlatformMouseEvent.h"
#include "ScrollableArea.h"
#include "Scrollbar.h"
#include "ScrollbarThemeWin.h"
#include "Settings.h"
#include "SoftLinking.h"

#include <CoreGraphics/CoreGraphics.h>

// If you have an empty placeholder SafariThemeConstants.h, then include SafariTheme.h
// This is a workaround until a version of WebKitSupportLibrary is released with an updated SafariThemeConstants.h 
#include <SafariTheme/SafariThemeConstants.h>
#ifndef SafariThemeConstants_h
#include <SafariTheme/SafariTheme.h>
#endif

// FIXME: There are repainting problems due to Aqua scroll bar buttons' visual overflow.

using namespace std;

namespace WebCore {

using namespace SafariTheme;

ScrollbarTheme* ScrollbarTheme::nativeTheme()
{
    static ScrollbarThemeSafari safariTheme;
    static ScrollbarThemeWin windowsTheme;
    if (Settings::shouldPaintNativeControls())
        return &windowsTheme;
    return &safariTheme;
}

// FIXME: Get these numbers from CoreUI.
static int cScrollbarThickness[] = { 15, 11 };
static int cRealButtonLength[] = { 28, 21 };
static int cButtonInset[] = { 14, 11 };
static int cButtonHitInset[] = { 3, 2 };
// cRealButtonLength - cButtonInset
static int cButtonLength[] = { 14, 10 };
static int cThumbMinLength[] = { 26, 20 };

#ifdef DEBUG_ALL
SOFT_LINK_DEBUG_LIBRARY(SafariTheme)
#else
SOFT_LINK_LIBRARY(SafariTheme)
#endif

SOFT_LINK(SafariTheme, paintThemePart, void, __stdcall, 
            (ThemePart part, CGContextRef context, const CGRect& rect, NSControlSize size, ThemeControlState state),
            (part, context, rect, size, state))

static ScrollbarControlState scrollbarControlStateFromThemeState(ThemeControlState state)
{
    ScrollbarControlState s = 0;
    if (state & ActiveState)
        s |= ActiveScrollbarState;
    if (state & EnabledState)
        s |= EnabledScrollbarState;
    if (state & PressedState)
        s |= PressedScrollbarState;
    return s;
}

ScrollbarThemeSafari::~ScrollbarThemeSafari()
{
}

int ScrollbarThemeSafari::scrollbarThickness(ScrollbarControlSize controlSize)
{
    return cScrollbarThickness[controlSize];
}

bool ScrollbarThemeSafari::hasButtons(ScrollbarThemeClient* scrollbar)
{
    return scrollbar->enabled() && (scrollbar->orientation() == HorizontalScrollbar ? 
             scrollbar->width() : 
             scrollbar->height()) >= 2 * (cRealButtonLength[scrollbar->controlSize()] - cButtonHitInset[scrollbar->controlSize()]);
}

bool ScrollbarThemeSafari::hasThumb(ScrollbarThemeClient* scrollbar)
{
    return scrollbar->enabled() && (scrollbar->orientation() == HorizontalScrollbar ? 
             scrollbar->width() : 
             scrollbar->height()) >= 2 * cButtonInset[scrollbar->controlSize()] + cThumbMinLength[scrollbar->controlSize()] + 1;
}

static IntRect buttonRepaintRect(const IntRect& buttonRect, ScrollbarOrientation orientation, ScrollbarControlSize controlSize, bool start)
{
    IntRect paintRect(buttonRect);
    if (orientation == HorizontalScrollbar) {
        paintRect.setWidth(cRealButtonLength[controlSize]);
        if (!start)
            paintRect.setX(buttonRect.x() - (cRealButtonLength[controlSize] - buttonRect.width()));
    } else {
        paintRect.setHeight(cRealButtonLength[controlSize]);
        if (!start)
            paintRect.setY(buttonRect.y() - (cRealButtonLength[controlSize] - buttonRect.height()));
    }

    return paintRect;
}

IntRect ScrollbarThemeSafari::backButtonRect(ScrollbarThemeClient* scrollbar, ScrollbarPart part, bool painting)
{
    IntRect result;

    // Windows just has single arrows.
    if (part == BackButtonEndPart)
        return result;

    int thickness = scrollbarThickness(scrollbar->controlSize());
    if (scrollbar->orientation() == HorizontalScrollbar)
        result = IntRect(scrollbar->x(), scrollbar->y(), cButtonLength[scrollbar->controlSize()], thickness);
    else
        result = IntRect(scrollbar->x(), scrollbar->y(), thickness, cButtonLength[scrollbar->controlSize()]);
    if (painting)
        return buttonRepaintRect(result, scrollbar->orientation(), scrollbar->controlSize(), true);
    return result;
}

IntRect ScrollbarThemeSafari::forwardButtonRect(ScrollbarThemeClient* scrollbar, ScrollbarPart part, bool painting)
{
    IntRect result;
    
    // Windows just has single arrows.
    if (part == ForwardButtonStartPart)
        return result;

    int thickness = scrollbarThickness(scrollbar->controlSize());
    if (scrollbar->orientation() == HorizontalScrollbar)
        result = IntRect(scrollbar->x() + scrollbar->width() - cButtonLength[scrollbar->controlSize()], scrollbar->y(), cButtonLength[scrollbar->controlSize()], thickness);
    else
        result = IntRect(scrollbar->x(), scrollbar->y() + scrollbar->height() - cButtonLength[scrollbar->controlSize()], thickness, cButtonLength[scrollbar->controlSize()]);
    if (painting)
        return buttonRepaintRect(result, scrollbar->orientation(), scrollbar->controlSize(), false);
    return result;
}

static IntRect trackRepaintRect(const IntRect& trackRect, ScrollbarOrientation orientation, ScrollbarControlSize controlSize)
{
    IntRect paintRect(trackRect);
    if (orientation == HorizontalScrollbar)
        paintRect.inflateX(cButtonLength[controlSize]);
    else
        paintRect.inflateY(cButtonLength[controlSize]);

    return paintRect;
}

IntRect ScrollbarThemeSafari::trackRect(ScrollbarThemeClient* scrollbar, bool painting)
{
    if (painting || !hasButtons(scrollbar))
        return scrollbar->frameRect();
    
    IntRect result;
    int thickness = scrollbarThickness(scrollbar->controlSize());
    if (scrollbar->orientation() == HorizontalScrollbar) 
        return IntRect(scrollbar->x() + cButtonLength[scrollbar->controlSize()], scrollbar->y(), scrollbar->width() - 2 * cButtonLength[scrollbar->controlSize()], thickness);
    return IntRect(scrollbar->x(), scrollbar->y() + cButtonLength[scrollbar->controlSize()], thickness, scrollbar->height() - 2 * cButtonLength[scrollbar->controlSize()]);
}

int ScrollbarThemeSafari::minimumThumbLength(ScrollbarThemeClient* scrollbar)
{
    return cThumbMinLength[scrollbar->controlSize()];
}

bool ScrollbarThemeSafari::shouldCenterOnThumb(ScrollbarThemeClient*, const PlatformMouseEvent& evt)
{
    return evt.shiftKey() && evt.button() == LeftButton;
}

void ScrollbarThemeSafari::paintTrackBackground(GraphicsContext* graphicsContext, ScrollbarThemeClient* scrollbar, const IntRect& trackRect)
{
    if (!SafariThemeLibrary())
        return;
    NSControlSize size = scrollbar->controlSize() == SmallScrollbar ? NSSmallControlSize : NSRegularControlSize;
    ThemeControlState state = 0;
    if (scrollbar->isScrollableAreaActive())
        state |= ActiveState;
    if (hasButtons(scrollbar))
        state |= EnabledState;
    paintThemePart(scrollbar->orientation() == VerticalScrollbar ? VScrollTrackPart : HScrollTrackPart, graphicsContext->platformContext(), trackRect, size, state); 
}

void ScrollbarThemeSafari::paintButton(GraphicsContext* graphicsContext, ScrollbarThemeClient* scrollbar, const IntRect& buttonRect, ScrollbarPart part)
{
    if (!SafariThemeLibrary())
        return;
    NSControlSize size = scrollbar->controlSize() == SmallScrollbar ? NSSmallControlSize : NSRegularControlSize;
    ThemeControlState state = 0;
    if (scrollbar->isScrollableAreaActive())
        state |= ActiveState;
    if (hasButtons(scrollbar))
        state |= EnabledState;
    if (scrollbar->pressedPart() == part)
        state |= PressedState;
    if (part == BackButtonStartPart)
        paintThemePart(scrollbar->orientation() == VerticalScrollbar ? ScrollUpArrowPart : ScrollLeftArrowPart, graphicsContext->platformContext(),
                       buttonRect, size, state);
    else if (part == ForwardButtonEndPart)
        paintThemePart(scrollbar->orientation() == VerticalScrollbar ? ScrollDownArrowPart : ScrollRightArrowPart, graphicsContext->platformContext(),
                       buttonRect, size, state);
}

void ScrollbarThemeSafari::paintThumb(GraphicsContext* graphicsContext, ScrollbarThemeClient* scrollbar, const IntRect& thumbRect)
{
    if (!SafariThemeLibrary())
        return;
    NSControlSize size = scrollbar->controlSize() == SmallScrollbar ? NSSmallControlSize : NSRegularControlSize;
    ThemeControlState state = 0;
    if (scrollbar->isScrollableAreaActive())
        state |= ActiveState;
    if (hasThumb(scrollbar))
        state |= EnabledState;
    if (scrollbar->pressedPart() == ThumbPart)
        state |= PressedState;
    paintThemePart(scrollbar->orientation() == VerticalScrollbar ? VScrollThumbPart : HScrollThumbPart, graphicsContext->platformContext(), 
                   thumbRect, size, state);
}

}

#endif
