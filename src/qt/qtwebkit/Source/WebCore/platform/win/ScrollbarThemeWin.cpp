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
#include "ScrollbarThemeWin.h"

#include "GraphicsContext.h"
#include "LocalWindowsContext.h"
#include "PlatformMouseEvent.h"
#include "Scrollbar.h"
#include "SoftLinking.h"
#include "SystemInfo.h"

// Generic state constants
#define TS_NORMAL    1
#define TS_HOVER     2
#define TS_ACTIVE    3
#define TS_DISABLED  4

#define SP_BUTTON          1
#define SP_THUMBHOR        2
#define SP_THUMBVERT       3
#define SP_TRACKSTARTHOR   4
#define SP_TRACKENDHOR     5
#define SP_TRACKSTARTVERT  6
#define SP_TRACKENDVERT    7
#define SP_GRIPPERHOR      8
#define SP_GRIPPERVERT     9

#define TS_UP_BUTTON       0
#define TS_DOWN_BUTTON     4
#define TS_LEFT_BUTTON     8
#define TS_RIGHT_BUTTON    12
#define TS_UP_BUTTON_HOVER   17
#define TS_DOWN_BUTTON_HOVER  18
#define TS_LEFT_BUTTON_HOVER  19
#define TS_RIGHT_BUTTON_HOVER   20

using namespace std;

namespace WebCore {

static HANDLE scrollbarTheme;
static bool runningVista;

// FIXME:  Refactor the soft-linking code so that it can be shared with RenderThemeWin
SOFT_LINK_LIBRARY(uxtheme)
SOFT_LINK(uxtheme, OpenThemeData, HANDLE, WINAPI, (HWND hwnd, LPCWSTR pszClassList), (hwnd, pszClassList))
SOFT_LINK(uxtheme, CloseThemeData, HRESULT, WINAPI, (HANDLE hTheme), (hTheme))
SOFT_LINK(uxtheme, DrawThemeBackground, HRESULT, WINAPI, (HANDLE hTheme, HDC hdc, int iPartId, int iStateId, const RECT* pRect, const RECT* pClipRect), (hTheme, hdc, iPartId, iStateId, pRect, pClipRect))
SOFT_LINK(uxtheme, IsThemeActive, BOOL, WINAPI, (), ())
SOFT_LINK(uxtheme, IsThemeBackgroundPartiallyTransparent, BOOL, WINAPI, (HANDLE hTheme, int iPartId, int iStateId), (hTheme, iPartId, iStateId))

// Constants used to figure the drag rect outside which we should snap the
// scrollbar thumb back to its origin.  These calculations are based on
// observing the behavior of the MSVC8 main window scrollbar + some
// guessing/extrapolation.
static const int kOffEndMultiplier = 3;
static const int kOffSideMultiplier = 8;

static void checkAndInitScrollbarTheme()
{
    if (uxthemeLibrary() && !scrollbarTheme && IsThemeActive())
        scrollbarTheme = OpenThemeData(0, L"Scrollbar");
}

#if !USE(SAFARI_THEME)
ScrollbarTheme* ScrollbarTheme::nativeTheme()
{
    static ScrollbarThemeWin winTheme;
    return &winTheme;
}
#endif

ScrollbarThemeWin::ScrollbarThemeWin()
{
    static bool initialized;
    if (!initialized) {
        initialized = true;
        checkAndInitScrollbarTheme();
        runningVista = (windowsVersion() >= WindowsVista);
    }
}

ScrollbarThemeWin::~ScrollbarThemeWin()
{
}

int ScrollbarThemeWin::scrollbarThickness(ScrollbarControlSize)
{
    static int thickness;
    if (!thickness)
        thickness = ::GetSystemMetrics(SM_CXVSCROLL);
    return thickness;
}

void ScrollbarThemeWin::themeChanged()
{
    if (!scrollbarTheme)
        return;

    CloseThemeData(scrollbarTheme);
    scrollbarTheme = 0;
}

bool ScrollbarThemeWin::invalidateOnMouseEnterExit()
{
    return runningVista;
}

bool ScrollbarThemeWin::hasThumb(ScrollbarThemeClient* scrollbar)
{
    return thumbLength(scrollbar) > 0;
}

IntRect ScrollbarThemeWin::backButtonRect(ScrollbarThemeClient* scrollbar, ScrollbarPart part, bool)
{
    // Windows just has single arrows.
    if (part == BackButtonEndPart)
        return IntRect();

    // Our desired rect is essentially 17x17.
    
    // Our actual rect will shrink to half the available space when
    // we have < 34 pixels left.  This allows the scrollbar
    // to scale down and function even at tiny sizes.
    int thickness = scrollbarThickness();
    if (scrollbar->orientation() == HorizontalScrollbar)
        return IntRect(scrollbar->x(), scrollbar->y(),
                       scrollbar->width() < 2 * thickness ? scrollbar->width() / 2 : thickness, thickness);
    return IntRect(scrollbar->x(), scrollbar->y(),
                   thickness, scrollbar->height() < 2 * thickness ? scrollbar->height() / 2 : thickness);
}

IntRect ScrollbarThemeWin::forwardButtonRect(ScrollbarThemeClient* scrollbar, ScrollbarPart part, bool)
{
    // Windows just has single arrows.
    if (part == ForwardButtonStartPart)
        return IntRect();
    
    // Our desired rect is essentially 17x17.
    
    // Our actual rect will shrink to half the available space when
    // we have < 34 pixels left.  This allows the scrollbar
    // to scale down and function even at tiny sizes.
    int thickness = scrollbarThickness();
    if (scrollbar->orientation() == HorizontalScrollbar) {
        int w = scrollbar->width() < 2 * thickness ? scrollbar->width() / 2 : thickness;
        return IntRect(scrollbar->x() + scrollbar->width() - w, scrollbar->y(), w, thickness);
    }
    
    int h = scrollbar->height() < 2 * thickness ? scrollbar->height() / 2 : thickness;
    return IntRect(scrollbar->x(), scrollbar->y() + scrollbar->height() - h, thickness, h);
}

IntRect ScrollbarThemeWin::trackRect(ScrollbarThemeClient* scrollbar, bool)
{
    int thickness = scrollbarThickness();
    if (scrollbar->orientation() == HorizontalScrollbar) {
        if (scrollbar->width() < 2 * thickness)
            return IntRect();
        return IntRect(scrollbar->x() + thickness, scrollbar->y(), scrollbar->width() - 2 * thickness, thickness);
    }
    if (scrollbar->height() < 2 * thickness)
        return IntRect();
    return IntRect(scrollbar->x(), scrollbar->y() + thickness, thickness, scrollbar->height() - 2 * thickness);
}

bool ScrollbarThemeWin::shouldCenterOnThumb(ScrollbarThemeClient*, const PlatformMouseEvent& evt)
{
    return evt.shiftKey() && evt.button() == LeftButton;
}

bool ScrollbarThemeWin::shouldSnapBackToDragOrigin(ScrollbarThemeClient* scrollbar, const PlatformMouseEvent& evt)
{
    // Find the rect within which we shouldn't snap, by expanding the track rect
    // in both dimensions.
    IntRect rect = trackRect(scrollbar);
    const bool horz = scrollbar->orientation() == HorizontalScrollbar;
    const int thickness = scrollbarThickness(scrollbar->controlSize());
    rect.inflateX((horz ? kOffEndMultiplier : kOffSideMultiplier) * thickness);
    rect.inflateY((horz ? kOffSideMultiplier : kOffEndMultiplier) * thickness);

    // Convert the event to local coordinates.
    IntPoint mousePosition = scrollbar->convertFromContainingWindow(evt.position());
    mousePosition.move(scrollbar->x(), scrollbar->y());

    // We should snap iff the event is outside our calculated rect.
    return !rect.contains(mousePosition);
}

void ScrollbarThemeWin::paintTrackBackground(GraphicsContext* context, ScrollbarThemeClient* scrollbar, const IntRect& rect)
{
    // Just assume a forward track part.  We only paint the track as a single piece when there is no thumb.
    if (!hasThumb(scrollbar))
        paintTrackPiece(context, scrollbar, rect, ForwardTrackPart);
}

void ScrollbarThemeWin::paintTrackPiece(GraphicsContext* context, ScrollbarThemeClient* scrollbar, const IntRect& rect, ScrollbarPart partType)
{
    checkAndInitScrollbarTheme();

    bool start = partType == BackTrackPart;
    int part;
    if (scrollbar->orientation() == HorizontalScrollbar)
        part = start ? SP_TRACKSTARTHOR : SP_TRACKENDHOR;
    else
        part = start ? SP_TRACKSTARTVERT : SP_TRACKENDVERT;

    int state;
    if (!scrollbar->enabled())
        state = TS_DISABLED;
    else if ((scrollbar->hoveredPart() == BackTrackPart && start) ||
             (scrollbar->hoveredPart() == ForwardTrackPart && !start))
        state = (scrollbar->pressedPart() == scrollbar->hoveredPart() ? TS_ACTIVE : TS_HOVER);
    else
        state = TS_NORMAL;

    bool alphaBlend = false;
    if (scrollbarTheme)
        alphaBlend = IsThemeBackgroundPartiallyTransparent(scrollbarTheme, part, state);

    LocalWindowsContext windowsContext(context, rect, alphaBlend);
    RECT themeRect(rect);

    if (scrollbarTheme)
        DrawThemeBackground(scrollbarTheme, windowsContext.hdc(), part, state, &themeRect, 0);
    else {
        DWORD color3DFace = ::GetSysColor(COLOR_3DFACE);
        DWORD colorScrollbar = ::GetSysColor(COLOR_SCROLLBAR);
        DWORD colorWindow = ::GetSysColor(COLOR_WINDOW);
        HDC hdc = windowsContext.hdc();
        if ((color3DFace != colorScrollbar) && (colorWindow != colorScrollbar))
            ::FillRect(hdc, &themeRect, HBRUSH(COLOR_SCROLLBAR+1));
        else {
            static WORD patternBits[8] = { 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55 };
            HBITMAP patternBitmap = ::CreateBitmap(8, 8, 1, 1, patternBits);
            HBRUSH brush = ::CreatePatternBrush(patternBitmap);
            SaveDC(hdc);
            ::SetTextColor(hdc, ::GetSysColor(COLOR_3DHILIGHT));
            ::SetBkColor(hdc, ::GetSysColor(COLOR_3DFACE));
            ::SetBrushOrgEx(hdc, rect.x(), rect.y(), NULL);
            ::SelectObject(hdc, brush);
            ::FillRect(hdc, &themeRect, brush);
            ::RestoreDC(hdc, -1);
            ::DeleteObject(brush);  
            ::DeleteObject(patternBitmap);
        }
    }

    if (!alphaBlend && !context->isInTransparencyLayer())
        DIBPixelData::setRGBABitmapAlpha(windowsContext.hdc(), rect, 255);
}

void ScrollbarThemeWin::paintButton(GraphicsContext* context, ScrollbarThemeClient* scrollbar, const IntRect& rect, ScrollbarPart part)
{
    checkAndInitScrollbarTheme();

    bool start = (part == BackButtonStartPart);
    int xpState = 0;
    int classicState = 0;
    if (scrollbar->orientation() == HorizontalScrollbar)
        xpState = start ? TS_LEFT_BUTTON : TS_RIGHT_BUTTON;
    else
        xpState = start ? TS_UP_BUTTON : TS_DOWN_BUTTON;
    classicState = xpState / 4;

    if (!scrollbar->enabled()) {
        xpState += TS_DISABLED;
        classicState |= DFCS_INACTIVE;
    } else if ((scrollbar->hoveredPart() == BackButtonStartPart && start) ||
               (scrollbar->hoveredPart() == ForwardButtonEndPart && !start)) {
        if (scrollbar->pressedPart() == scrollbar->hoveredPart()) {
            xpState += TS_ACTIVE;
            classicState |= DFCS_PUSHED;
#if !OS(WINCE)
            classicState |= DFCS_FLAT;
#endif
        } else
            xpState += TS_HOVER;
    } else {
        if (scrollbar->hoveredPart() == NoPart || !runningVista)
            xpState += TS_NORMAL;
        else {
            if (scrollbar->orientation() == HorizontalScrollbar)
                xpState = start ? TS_LEFT_BUTTON_HOVER : TS_RIGHT_BUTTON_HOVER;
            else
                xpState = start ? TS_UP_BUTTON_HOVER : TS_DOWN_BUTTON_HOVER;
        }
    }

    bool alphaBlend = false;
    if (scrollbarTheme)
        alphaBlend = IsThemeBackgroundPartiallyTransparent(scrollbarTheme, SP_BUTTON, xpState);

    LocalWindowsContext windowsContext(context, rect, alphaBlend);
    RECT themeRect(rect);
    if (scrollbarTheme)
        DrawThemeBackground(scrollbarTheme, windowsContext.hdc(), SP_BUTTON, xpState, &themeRect, 0);
    else
        ::DrawFrameControl(windowsContext.hdc(), &themeRect, DFC_SCROLL, classicState);

    if (!alphaBlend && !context->isInTransparencyLayer())
        DIBPixelData::setRGBABitmapAlpha(windowsContext.hdc(), rect, 255);
}

static IntRect gripperRect(int thickness, const IntRect& thumbRect)
{
    // Center in the thumb.
    int gripperThickness = thickness / 2;
    return IntRect(thumbRect.x() + (thumbRect.width() - gripperThickness) / 2,
                   thumbRect.y() + (thumbRect.height() - gripperThickness) / 2,
                   gripperThickness, gripperThickness);
}

static void paintGripper(ScrollbarThemeClient* scrollbar, HDC hdc, const IntRect& rect)
{
    if (!scrollbarTheme)
        return;  // Classic look has no gripper.
   
    int state;
    if (!scrollbar->enabled())
        state = TS_DISABLED;
    else if (scrollbar->pressedPart() == ThumbPart)
        state = TS_ACTIVE; // Thumb always stays active once pressed.
    else if (scrollbar->hoveredPart() == ThumbPart)
        state = TS_HOVER;
    else
        state = TS_NORMAL;

    RECT themeRect(rect);
    DrawThemeBackground(scrollbarTheme, hdc, scrollbar->orientation() == HorizontalScrollbar ? SP_GRIPPERHOR : SP_GRIPPERVERT, state, &themeRect, 0);
}

void ScrollbarThemeWin::paintThumb(GraphicsContext* context, ScrollbarThemeClient* scrollbar, const IntRect& rect)
{
    checkAndInitScrollbarTheme();

    int state;
    if (!scrollbar->enabled())
        state = TS_DISABLED;
    else if (scrollbar->pressedPart() == ThumbPart)
        state = TS_ACTIVE; // Thumb always stays active once pressed.
    else if (scrollbar->hoveredPart() == ThumbPart)
        state = TS_HOVER;
    else
        state = TS_NORMAL;

    bool alphaBlend = false;
    if (scrollbarTheme)
        alphaBlend = IsThemeBackgroundPartiallyTransparent(scrollbarTheme, scrollbar->orientation() == HorizontalScrollbar ? SP_THUMBHOR : SP_THUMBVERT, state);
    LocalWindowsContext windowsContext(context, rect, alphaBlend);
    RECT themeRect(rect);
    if (scrollbarTheme) {
        DrawThemeBackground(scrollbarTheme, windowsContext.hdc(), scrollbar->orientation() == HorizontalScrollbar ? SP_THUMBHOR : SP_THUMBVERT, state, &themeRect, 0);
        paintGripper(scrollbar, windowsContext.hdc(), gripperRect(scrollbarThickness(), rect));
    } else
        ::DrawEdge(windowsContext.hdc(), &themeRect, EDGE_RAISED, BF_RECT | BF_MIDDLE);

    if (!alphaBlend && !context->isInTransparencyLayer())
        DIBPixelData::setRGBABitmapAlpha(windowsContext.hdc(), rect, 255);
}

}

