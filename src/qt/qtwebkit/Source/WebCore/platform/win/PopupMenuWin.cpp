/*
 * Copyright (C) 2006, 2007, 2008, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile Inc.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "PopupMenuWin.h"

#include "BitmapInfo.h"
#include "Document.h"
#include "FloatRect.h"
#include "FontSelector.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HTMLNames.h"
#include "HWndDC.h"
#include "HostWindow.h"
#include "LengthFunctions.h"
#include "Page.h"
#include "PlatformMouseEvent.h"
#include "PlatformScreen.h"
#include "RenderMenuList.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "Scrollbar.h"
#include "ScrollbarTheme.h"
#include "SimpleFontData.h"
#include "TextRun.h"
#include "WebCoreInstanceHandle.h"
#include "WindowsExtras.h"

#include <windows.h>
#include <windowsx.h>
#if OS(WINCE)
#include <ResDefCE.h>
#define MAKEPOINTS(l) (*((POINTS FAR *)&(l)))
#endif

#define HIGH_BIT_MASK_SHORT 0x8000

using std::min;

namespace WebCore {

using namespace HTMLNames;

// Default Window animation duration in milliseconds
static const int defaultAnimationDuration = 200;
// Maximum height of a popup window
static const int maxPopupHeight = 320;

const int optionSpacingMiddle = 1;
const int popupWindowBorderWidth = 1;

static LPCWSTR kPopupWindowClassName = L"PopupWindowClass";

// This is used from within our custom message pump when we want to send a
// message to the web view and not have our message stolen and sent to
// the popup window.
static const UINT WM_HOST_WINDOW_FIRST = WM_USER;
static const UINT WM_HOST_WINDOW_CHAR = WM_USER + WM_CHAR; 
static const UINT WM_HOST_WINDOW_MOUSEMOVE = WM_USER + WM_MOUSEMOVE;

// FIXME: Remove this as soon as practical.
static inline bool isASCIIPrintable(unsigned c)
{
    return c >= 0x20 && c <= 0x7E;
}

static void translatePoint(LPARAM& lParam, HWND from, HWND to)
{
    POINT pt;
    pt.x = (short)GET_X_LPARAM(lParam);
    pt.y = (short)GET_Y_LPARAM(lParam);    
    ::MapWindowPoints(from, to, &pt, 1);
    lParam = MAKELPARAM(pt.x, pt.y);
}

static FloatRect monitorFromHwnd(HWND hwnd)
{
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFOEX monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(monitor, &monitorInfo);
    return monitorInfo.rcWork;
}

PopupMenuWin::PopupMenuWin(PopupMenuClient* client)
    : m_popupClient(client)
    , m_scrollbar(0)
    , m_popup(0)
    , m_DC(0)
    , m_bmp(0)
    , m_wasClicked(false)
    , m_itemHeight(0)
    , m_scrollOffset(0)
    , m_wheelDelta(0)
    , m_focusedIndex(0)
    , m_scrollbarCapturingMouse(false)
    , m_showPopup(false)
{
}

PopupMenuWin::~PopupMenuWin()
{
    if (m_bmp)
        ::DeleteObject(m_bmp);
    if (m_DC)
        ::DeleteDC(m_DC);
    if (m_popup)
        ::DestroyWindow(m_popup);
    if (m_scrollbar)
        m_scrollbar->setParent(0);
}

void PopupMenuWin::disconnectClient()
{
    m_popupClient = 0;
}

LPCWSTR PopupMenuWin::popupClassName()
{
    return kPopupWindowClassName;
}

void PopupMenuWin::show(const IntRect& r, FrameView* view, int index)
{
    calculatePositionAndSize(r, view);
    if (clientRect().isEmpty())
        return;

    HWND hostWindow = view->hostWindow()->platformPageClient();

    if (!m_scrollbar && visibleItems() < client()->listSize()) {
        // We need a scroll bar
        m_scrollbar = client()->createScrollbar(this, VerticalScrollbar, SmallScrollbar);
        m_scrollbar->styleChanged();
    }

    // We need to reposition the popup window to its final coordinates.
    // Before calling this, the popup hwnd is currently the size of and at the location of the menu list client so it needs to be updated.
    ::MoveWindow(m_popup, m_windowRect.x(), m_windowRect.y(), m_windowRect.width(), m_windowRect.height(), false);

    // Determine whether we should animate our popups
    // Note: Must use 'BOOL' and 'FALSE' instead of 'bool' and 'false' to avoid stack corruption with SystemParametersInfo
    BOOL shouldAnimate = FALSE;
#if !OS(WINCE)
    ::SystemParametersInfo(SPI_GETCOMBOBOXANIMATION, 0, &shouldAnimate, 0);

    if (shouldAnimate) {
        RECT viewRect = {0};
        ::GetWindowRect(hostWindow, &viewRect);
        if (!::IsRectEmpty(&viewRect))
            ::AnimateWindow(m_popup, defaultAnimationDuration, AW_BLEND);
    } else
#endif
        ::ShowWindow(m_popup, SW_SHOWNOACTIVATE);

    if (client()) {
        int index = client()->selectedIndex();
        if (index >= 0)
            setFocusedIndex(index);
    }

    m_showPopup = true;

    // Protect the popup menu in case its owner is destroyed while we're running the message pump.
    RefPtr<PopupMenu> protect(this);

    ::SetCapture(hostWindow);

    MSG msg;
    HWND activeWindow;

    while (::GetMessage(&msg, 0, 0, 0)) {
        switch (msg.message) {
            case WM_HOST_WINDOW_MOUSEMOVE:
            case WM_HOST_WINDOW_CHAR: 
                if (msg.hwnd == m_popup) {
                    // This message should be sent to the host window.
                    msg.hwnd = hostWindow;
                    msg.message -= WM_HOST_WINDOW_FIRST;
                }
                break;

            // Steal mouse messages.
#if !OS(WINCE)
            case WM_NCMOUSEMOVE:
            case WM_NCLBUTTONDOWN:
            case WM_NCLBUTTONUP:
            case WM_NCLBUTTONDBLCLK:
            case WM_NCRBUTTONDOWN:
            case WM_NCRBUTTONUP:
            case WM_NCRBUTTONDBLCLK:
            case WM_NCMBUTTONDOWN:
            case WM_NCMBUTTONUP:
            case WM_NCMBUTTONDBLCLK:
#endif
            case WM_MOUSEWHEEL:
                msg.hwnd = m_popup;
                break;

            // These mouse messages use client coordinates so we need to convert them.
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_LBUTTONDBLCLK:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_RBUTTONDBLCLK:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MBUTTONDBLCLK: {
                // Translate the coordinate.
                translatePoint(msg.lParam, msg.hwnd, m_popup);

                msg.hwnd = m_popup;
                break;
            }

            // Steal all keyboard messages.
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_CHAR:
            case WM_DEADCHAR:
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_SYSCHAR:
            case WM_SYSDEADCHAR:
                msg.hwnd = m_popup;
                break;
        }

        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);

        if (!m_popupClient)
            break;

        if (!m_showPopup)
            break;
        activeWindow = ::GetActiveWindow();
        if (activeWindow != hostWindow && !::IsChild(activeWindow, hostWindow))
            break;
        if (::GetCapture() != hostWindow)
            break;
    }

    if (::GetCapture() == hostWindow)
        ::ReleaseCapture();

    // We're done, hide the popup if necessary.
    hide();
}

void PopupMenuWin::hide()
{
    if (!m_showPopup)
        return;

    m_showPopup = false;

    ::ShowWindow(m_popup, SW_HIDE);

    if (client())
        client()->popupDidHide();

    // Post a WM_NULL message to wake up the message pump if necessary.
    ::PostMessage(m_popup, WM_NULL, 0, 0);
}

// The screen that the popup is placed on should be whichever one the popup menu button lies on.
// We fake an hwnd (here we use the popup's hwnd) on top of the button which we can then use to determine the screen.
// We can then proceed with our final position/size calculations.
void PopupMenuWin::calculatePositionAndSize(const IntRect& r, FrameView* v)
{
    // First get the screen coordinates of the popup menu client.
    HWND hostWindow = v->hostWindow()->platformPageClient();
    IntRect absoluteBounds = ((RenderMenuList*)m_popupClient)->absoluteBoundingBoxRect();
    IntRect absoluteScreenCoords(v->contentsToWindow(absoluteBounds.location()), absoluteBounds.size());
    POINT absoluteLocation(absoluteScreenCoords.location());
    if (!::ClientToScreen(v->hostWindow()->platformPageClient(), &absoluteLocation))
        return;
    absoluteScreenCoords.setLocation(absoluteLocation);

    // Now set the popup menu's location temporarily to these coordinates so we can determine which screen the popup should lie on.
    // We create or move m_popup as necessary.
    if (!m_popup) {
        registerClass();
        DWORD exStyle = WS_EX_LTRREADING;
        m_popup = ::CreateWindowExW(exStyle, kPopupWindowClassName, L"PopupMenu",
            WS_POPUP | WS_BORDER,
            absoluteScreenCoords.x(), absoluteScreenCoords.y(), absoluteScreenCoords.width(), absoluteScreenCoords.height(),
            hostWindow, 0, WebCore::instanceHandle(), this);

        if (!m_popup)
            return;
    } else
        ::MoveWindow(m_popup, absoluteScreenCoords.x(), absoluteScreenCoords.y(), absoluteScreenCoords.width(), absoluteScreenCoords.height(), false);

    FloatRect screen = monitorFromHwnd(m_popup);
    
    // Now we determine the actual location and measurements of the popup itself.
    // r is in absolute document coordinates, but we want to be in screen coordinates.

    // First, move to WebView coordinates
    IntRect rScreenCoords(v->contentsToWindow(r.location()), r.size());

    // Then, translate to screen coordinates
    POINT location(rScreenCoords.location());
    if (!::ClientToScreen(v->hostWindow()->platformPageClient(), &location))
        return;

    rScreenCoords.setLocation(location);

    // First, determine the popup's height
    int itemCount = client()->listSize();
    m_itemHeight = client()->menuStyle().font().fontMetrics().height() + optionSpacingMiddle;
    int naturalHeight = m_itemHeight * itemCount;
    int popupHeight = min(maxPopupHeight, naturalHeight);
    // The popup should show an integral number of items (i.e. no partial items should be visible)
    popupHeight -= popupHeight % m_itemHeight;
    
    // Next determine its width
    int popupWidth = 0;
    for (int i = 0; i < itemCount; ++i) {
        String text = client()->itemText(i);
        if (text.isEmpty())
            continue;

        Font itemFont = client()->menuStyle().font();
        if (client()->itemIsLabel(i)) {
            FontDescription d = itemFont.fontDescription();
            d.setWeight(d.bolderWeight());
            itemFont = Font(d, itemFont.letterSpacing(), itemFont.wordSpacing());
            itemFont.update(m_popupClient->fontSelector());
        }

        popupWidth = max(popupWidth, static_cast<int>(ceilf(itemFont.width(TextRun(text.characters(), text.length())))));
    }

    if (naturalHeight > maxPopupHeight)
        // We need room for a scrollbar
        popupWidth += ScrollbarTheme::theme()->scrollbarThickness(SmallScrollbar);

    // Add padding to align the popup text with the <select> text
    popupWidth += max<int>(0, client()->clientPaddingRight() - client()->clientInsetRight()) + max<int>(0, client()->clientPaddingLeft() - client()->clientInsetLeft());

    // Leave room for the border
    popupWidth += 2 * popupWindowBorderWidth;
    popupHeight += 2 * popupWindowBorderWidth;

    // The popup should be at least as wide as the control on the page
    popupWidth = max(rScreenCoords.width() - client()->clientInsetLeft() - client()->clientInsetRight(), popupWidth);

    // Always left-align items in the popup.  This matches popup menus on the mac.
    int popupX = rScreenCoords.x() + client()->clientInsetLeft();

    IntRect popupRect(popupX, rScreenCoords.maxY(), popupWidth, popupHeight);

    // Check that we don't go off the screen vertically
    if (popupRect.maxY() > screen.height()) {
        // The popup will go off the screen, so try placing it above the client
        if (rScreenCoords.y() - popupRect.height() < 0) {
            // The popup won't fit above, either, so place it whereever's bigger and resize it to fit
            if ((rScreenCoords.y() + rScreenCoords.height() / 2) < (screen.height() / 2)) {
                // Below is bigger
                popupRect.setHeight(screen.height() - popupRect.y());
            } else {
                // Above is bigger
                popupRect.setY(0);
                popupRect.setHeight(rScreenCoords.y());
            }
        } else {
            // The popup fits above, so reposition it
            popupRect.setY(rScreenCoords.y() - popupRect.height());
        }
    }

    // Check that we don't go off the screen horizontally
    if (popupRect.x() + popupRect.width() > screen.width() + screen.x())
        popupRect.setX(screen.x() + screen.width() - popupRect.width());
    if (popupRect.x() < screen.x())
        popupRect.setX(screen.x());

    m_windowRect = popupRect;
    return;
}

bool PopupMenuWin::setFocusedIndex(int i, bool hotTracking)
{
    if (i < 0 || i >= client()->listSize() || i == focusedIndex())
        return false;

    if (!client()->itemIsEnabled(i))
        return false;

    invalidateItem(focusedIndex());
    invalidateItem(i);

    m_focusedIndex = i;

    if (!hotTracking)
        client()->setTextFromItem(i);

    if (!scrollToRevealSelection())
        ::UpdateWindow(m_popup);

    return true;
}

int PopupMenuWin::visibleItems() const
{
    return clientRect().height() / m_itemHeight;
}

int PopupMenuWin::listIndexAtPoint(const IntPoint& point) const
{
    return m_scrollOffset + point.y() / m_itemHeight;
}

int PopupMenuWin::focusedIndex() const
{
    return m_focusedIndex;
}

void PopupMenuWin::focusFirst()
{
    if (!client())
        return;

    int size = client()->listSize();

    for (int i = 0; i < size; ++i)
        if (client()->itemIsEnabled(i)) {
            setFocusedIndex(i);
            break;
        }
}

void PopupMenuWin::focusLast()
{
    if (!client())
        return;

    int size = client()->listSize();

    for (int i = size - 1; i > 0; --i)
        if (client()->itemIsEnabled(i)) {
            setFocusedIndex(i);
            break;
        }
}

bool PopupMenuWin::down(unsigned lines)
{
    if (!client())
        return false;

    int size = client()->listSize();

    int lastSelectableIndex, selectedListIndex;
    lastSelectableIndex = selectedListIndex = focusedIndex();
    for (int i = selectedListIndex + 1; i >= 0 && i < size; ++i)
        if (client()->itemIsEnabled(i)) {
            lastSelectableIndex = i;
            if (i >= selectedListIndex + (int)lines)
                break;
        }

    return setFocusedIndex(lastSelectableIndex);
}

bool PopupMenuWin::up(unsigned lines)
{
    if (!client())
        return false;

    int size = client()->listSize();

    int lastSelectableIndex, selectedListIndex;
    lastSelectableIndex = selectedListIndex = focusedIndex();
    for (int i = selectedListIndex - 1; i >= 0 && i < size; --i)
        if (client()->itemIsEnabled(i)) {
            lastSelectableIndex = i;
            if (i <= selectedListIndex - (int)lines)
                break;
        }

    return setFocusedIndex(lastSelectableIndex);
}

void PopupMenuWin::invalidateItem(int index)
{
    if (!m_popup)
        return;

    IntRect damageRect(clientRect());
    damageRect.setY(m_itemHeight * (index - m_scrollOffset));
    damageRect.setHeight(m_itemHeight);
    if (m_scrollbar)
        damageRect.setWidth(damageRect.width() - m_scrollbar->frameRect().width());

    RECT r = damageRect;
    ::InvalidateRect(m_popup, &r, TRUE);
}

IntRect PopupMenuWin::clientRect() const
{
    IntRect clientRect = m_windowRect;
    clientRect.inflate(-popupWindowBorderWidth);
    clientRect.setLocation(IntPoint(0, 0));
    return clientRect;
}

void PopupMenuWin::incrementWheelDelta(int delta)
{
    m_wheelDelta += delta;
}

void PopupMenuWin::reduceWheelDelta(int delta)
{
    ASSERT(delta >= 0);
    ASSERT(delta <= abs(m_wheelDelta));

    if (m_wheelDelta > 0)
        m_wheelDelta -= delta;
    else if (m_wheelDelta < 0)
        m_wheelDelta += delta;
    else
        return;
}

bool PopupMenuWin::scrollToRevealSelection()
{
    if (!m_scrollbar)
        return false;

    int index = focusedIndex();

    if (index < m_scrollOffset) {
        ScrollableArea::scrollToOffsetWithoutAnimation(VerticalScrollbar, index);
        return true;
    }

    if (index >= m_scrollOffset + visibleItems()) {
        ScrollableArea::scrollToOffsetWithoutAnimation(VerticalScrollbar, index - visibleItems() + 1);
        return true;
    }

    return false;
}

void PopupMenuWin::updateFromElement()
{
    if (!m_popup)
        return;

    m_focusedIndex = client()->selectedIndex();

    ::InvalidateRect(m_popup, 0, TRUE);
    if (!scrollToRevealSelection())
        ::UpdateWindow(m_popup);
}

const int separatorPadding = 4;
const int separatorHeight = 1;
void PopupMenuWin::paint(const IntRect& damageRect, HDC hdc)
{
    if (!m_popup)
        return;

    if (!m_DC) {
        m_DC = ::CreateCompatibleDC(HWndDC(m_popup));
        if (!m_DC)
            return;
    }

    if (m_bmp) {
        bool keepBitmap = false;
        BITMAP bitmap;
        if (GetObject(m_bmp, sizeof(bitmap), &bitmap))
            keepBitmap = bitmap.bmWidth == clientRect().width()
                && bitmap.bmHeight == clientRect().height();
        if (!keepBitmap) {
            DeleteObject(m_bmp);
            m_bmp = 0;
        }
    }
    if (!m_bmp) {
#if OS(WINCE)
        BitmapInfo bitmapInfo = BitmapInfo::createBottomUp(clientRect().size(), BitmapInfo::BitCount16);
#else
        BitmapInfo bitmapInfo = BitmapInfo::createBottomUp(clientRect().size());
#endif
        void* pixels = 0;
        m_bmp = ::CreateDIBSection(m_DC, &bitmapInfo, DIB_RGB_COLORS, &pixels, 0, 0);
        if (!m_bmp)
            return;

        ::SelectObject(m_DC, m_bmp);
    }

    GraphicsContext context(m_DC);

    int itemCount = client()->listSize();

    // listRect is the damageRect translated into the coordinates of the entire menu list (which is itemCount * m_itemHeight pixels tall)
    IntRect listRect = damageRect;
    listRect.move(IntSize(0, m_scrollOffset * m_itemHeight));

    for (int y = listRect.y(); y < listRect.maxY(); y += m_itemHeight) {
        int index = y / m_itemHeight;

        Color optionBackgroundColor, optionTextColor;
        PopupMenuStyle itemStyle = client()->itemStyle(index);
        if (index == focusedIndex()) {
            optionBackgroundColor = RenderTheme::defaultTheme()->activeListBoxSelectionBackgroundColor();
            optionTextColor = RenderTheme::defaultTheme()->activeListBoxSelectionForegroundColor();
        } else {
            optionBackgroundColor = itemStyle.backgroundColor();
            optionTextColor = itemStyle.foregroundColor();
        }

        // itemRect is in client coordinates
        IntRect itemRect(0, (index - m_scrollOffset) * m_itemHeight, damageRect.width(), m_itemHeight);

        // Draw the background for this menu item
        if (itemStyle.isVisible())
            context.fillRect(itemRect, optionBackgroundColor, ColorSpaceDeviceRGB);

        if (client()->itemIsSeparator(index)) {
            IntRect separatorRect(itemRect.x() + separatorPadding, itemRect.y() + (itemRect.height() - separatorHeight) / 2, itemRect.width() - 2 * separatorPadding, separatorHeight);
            context.fillRect(separatorRect, optionTextColor, ColorSpaceDeviceRGB);
            continue;
        }

        String itemText = client()->itemText(index);
            
        TextDirection direction = (itemText.defaultWritingDirection() == WTF::Unicode::RightToLeft) ? RTL : LTR;
        TextRun textRun(itemText, 0, 0, TextRun::AllowTrailingExpansion, direction);

        context.setFillColor(optionTextColor, ColorSpaceDeviceRGB);
        
        Font itemFont = client()->menuStyle().font();
        if (client()->itemIsLabel(index)) {
            FontDescription d = itemFont.fontDescription();
            d.setWeight(d.bolderWeight());
            itemFont = Font(d, itemFont.letterSpacing(), itemFont.wordSpacing());
            itemFont.update(m_popupClient->fontSelector());
        }
        
        // Draw the item text
        if (itemStyle.isVisible()) {
            int textX = max<int>(0, client()->clientPaddingLeft() - client()->clientInsetLeft());
            if (RenderTheme::defaultTheme()->popupOptionSupportsTextIndent() && itemStyle.textDirection() == LTR)
                textX += minimumIntValueForLength(itemStyle.textIndent(), itemRect.width());
            int textY = itemRect.y() + itemFont.fontMetrics().ascent() + (itemRect.height() - itemFont.fontMetrics().height()) / 2;
            context.drawBidiText(itemFont, textRun, IntPoint(textX, textY));
        }
    }

    if (m_scrollbar)
        m_scrollbar->paint(&context, damageRect);

    HWndDC hWndDC;
    HDC localDC = hdc ? hdc : hWndDC.setHWnd(m_popup);

    ::BitBlt(localDC, damageRect.x(), damageRect.y(), damageRect.width(), damageRect.height(), m_DC, damageRect.x(), damageRect.y(), SRCCOPY);
}

int PopupMenuWin::scrollSize(ScrollbarOrientation orientation) const
{
    return ((orientation == VerticalScrollbar) && m_scrollbar) ? (m_scrollbar->totalSize() - m_scrollbar->visibleSize()) : 0;
}

int PopupMenuWin::scrollPosition(Scrollbar*) const
{
    return m_scrollOffset;
}

void PopupMenuWin::setScrollOffset(const IntPoint& offset)
{
    scrollTo(offset.y());
}

void PopupMenuWin::scrollTo(int offset)
{
    ASSERT(m_scrollbar);

    if (!m_popup)
        return;

    if (m_scrollOffset == offset)
        return;

    int scrolledLines = m_scrollOffset - offset;
    m_scrollOffset = offset;

    UINT flags = SW_INVALIDATE;

#ifdef CAN_SET_SMOOTH_SCROLLING_DURATION
    BOOL shouldSmoothScroll = FALSE;
    ::SystemParametersInfo(SPI_GETLISTBOXSMOOTHSCROLLING, 0, &shouldSmoothScroll, 0);
    if (shouldSmoothScroll)
        flags |= MAKEWORD(SW_SMOOTHSCROLL, smoothScrollAnimationDuration);
#endif

    IntRect listRect = clientRect();
    if (m_scrollbar)
        listRect.setWidth(listRect.width() - m_scrollbar->frameRect().width());
    RECT r = listRect;
    ::ScrollWindowEx(m_popup, 0, scrolledLines * m_itemHeight, &r, 0, 0, 0, flags);
    if (m_scrollbar) {
        r = m_scrollbar->frameRect();
        ::InvalidateRect(m_popup, &r, TRUE);
    }
    ::UpdateWindow(m_popup);
}

void PopupMenuWin::invalidateScrollbarRect(Scrollbar* scrollbar, const IntRect& rect)
{
    IntRect scrollRect = rect;
    scrollRect.move(scrollbar->x(), scrollbar->y());
    RECT r = scrollRect;
    ::InvalidateRect(m_popup, &r, false);
}

int PopupMenuWin::visibleHeight() const
{
    return m_scrollbar ? m_scrollbar->visibleSize() : m_windowRect.height();
}

int PopupMenuWin::visibleWidth() const
{
    return m_windowRect.width();
}

IntSize PopupMenuWin::contentsSize() const
{
    return m_windowRect.size();
}

bool PopupMenuWin::scrollbarsCanBeActive() const
{
    return m_showPopup;
}

IntRect PopupMenuWin::scrollableAreaBoundingBox() const
{
    return m_windowRect;
}

void PopupMenuWin::registerClass()
{
    static bool haveRegisteredWindowClass = false;

    if (haveRegisteredWindowClass)
        return;

#if OS(WINCE)
    WNDCLASS wcex;
#else
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.hIconSm        = 0;
    wcex.style          = CS_DROPSHADOW;
#endif

    wcex.lpfnWndProc    = PopupMenuWndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = sizeof(PopupMenu*); // For the PopupMenu pointer
    wcex.hInstance      = WebCore::instanceHandle();
    wcex.hIcon          = 0;
    wcex.hCursor        = LoadCursor(0, IDC_ARROW);
    wcex.hbrBackground  = 0;
    wcex.lpszMenuName   = 0;
    wcex.lpszClassName  = kPopupWindowClassName;

    haveRegisteredWindowClass = true;

#if OS(WINCE)
    RegisterClass(&wcex);
#else
    RegisterClassEx(&wcex);
#endif
}


LRESULT CALLBACK PopupMenuWin::PopupMenuWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (PopupMenuWin* popup = static_cast<PopupMenuWin*>(getWindowPointer(hWnd, 0)))
        return popup->wndProc(hWnd, message, wParam, lParam);

    if (message == WM_CREATE) {
        LPCREATESTRUCT createStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);

        // Associate the PopupMenu with the window.
        setWindowPointer(hWnd, 0, createStruct->lpCreateParams);
        return 0;
    }

    return ::DefWindowProc(hWnd, message, wParam, lParam);
}

const int smoothScrollAnimationDuration = 5000;

LRESULT PopupMenuWin::wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;

    switch (message) {
#if !OS(WINCE)
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
#endif
        case WM_SIZE: {
            if (!scrollbar())
                break;

            IntSize size(LOWORD(lParam), HIWORD(lParam));
            scrollbar()->setFrameRect(IntRect(size.width() - scrollbar()->width(), 0, scrollbar()->width(), size.height()));

            int visibleItems = this->visibleItems();
            scrollbar()->setEnabled(visibleItems < client()->listSize());
            scrollbar()->setSteps(1, max(1, visibleItems - 1));
            scrollbar()->setProportion(visibleItems, client()->listSize());

            break;
        }
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN: {
            if (!client())
                break;

            bool altKeyPressed = GetKeyState(VK_MENU) & HIGH_BIT_MASK_SHORT;
            bool ctrlKeyPressed = GetKeyState(VK_CONTROL) & HIGH_BIT_MASK_SHORT;

            lResult = 0;
            switch (LOWORD(wParam)) {
                case VK_F4: {
                    if (!altKeyPressed && !ctrlKeyPressed) {
                        int index = focusedIndex();
                        ASSERT(index >= 0);
                        client()->valueChanged(index);
                        hide();
                    }
                    break;
                }
                case VK_DOWN:
                    if (altKeyPressed) {
                        int index = focusedIndex();
                        ASSERT(index >= 0);
                        client()->valueChanged(index);
                        hide();
                    } else
                        down();
                    break;
                case VK_RIGHT:
                    down();
                    break;
                case VK_UP:
                    if (altKeyPressed) {
                        int index = focusedIndex();
                        ASSERT(index >= 0);
                        client()->valueChanged(index);
                        hide();
                    } else
                        up();
                    break;
                case VK_LEFT:
                    up();
                    break;
                case VK_HOME:
                    focusFirst();
                    break;
                case VK_END:
                    focusLast();
                    break;
                case VK_PRIOR:
                    if (focusedIndex() != scrollOffset()) {
                        // Set the selection to the first visible item
                        int firstVisibleItem = scrollOffset();
                        up(focusedIndex() - firstVisibleItem);
                    } else {
                        // The first visible item is selected, so move the selection back one page
                        up(visibleItems());
                    }
                    break;
                case VK_NEXT: {
                    int lastVisibleItem = scrollOffset() + visibleItems() - 1;
                    if (focusedIndex() != lastVisibleItem) {
                        // Set the selection to the last visible item
                        down(lastVisibleItem - focusedIndex());
                    } else {
                        // The last visible item is selected, so move the selection forward one page
                        down(visibleItems());
                    }
                    break;
                }
                case VK_TAB:
                    ::SendMessage(client()->hostWindow()->platformPageClient(), message, wParam, lParam);
                    hide();
                    break;
                case VK_ESCAPE:
                    hide();
                    break;
                default:
                    if (isASCIIPrintable(wParam))
                        // Send the keydown to the WebView so it can be used for type-to-select.
                        // Since we know that the virtual key is ASCII printable, it's OK to convert this to
                        // a WM_CHAR message. (We don't want to call TranslateMessage because that will post a
                        // WM_CHAR message that will be stolen and redirected to the popup HWND.
                        ::PostMessage(m_popup, WM_HOST_WINDOW_CHAR, wParam, lParam);
                    else
                        lResult = 1;
                    break;
            }
            break;
        }
        case WM_CHAR: {
            if (!client())
                break;

            lResult = 0;
            int index;
            switch (wParam) {
                case 0x0D:   // Enter/Return
                    hide();
                    index = focusedIndex();
                    ASSERT(index >= 0);
                    client()->valueChanged(index);
                    break;
                case 0x1B:   // Escape
                    hide();
                    break;
                case 0x09:   // TAB
                case 0x08:   // Backspace
                case 0x0A:   // Linefeed
                default:     // Character
                    lResult = 1;
                    break;
            }
            break;
        }
        case WM_MOUSEMOVE: {
            IntPoint mousePoint(MAKEPOINTS(lParam));
            if (scrollbar()) {
                IntRect scrollBarRect = scrollbar()->frameRect();
                if (scrollbarCapturingMouse() || scrollBarRect.contains(mousePoint)) {
                    // Put the point into coordinates relative to the scroll bar
                    mousePoint.move(-scrollBarRect.x(), -scrollBarRect.y());
                    PlatformMouseEvent event(hWnd, message, wParam, MAKELPARAM(mousePoint.x(), mousePoint.y()));
                    scrollbar()->mouseMoved(event);
                    break;
                }
            }

            BOOL shouldHotTrack = FALSE;
#if !OS(WINCE)
            ::SystemParametersInfo(SPI_GETHOTTRACKING, 0, &shouldHotTrack, 0);
#endif

            RECT bounds;
            GetClientRect(popupHandle(), &bounds);
            if (!::PtInRect(&bounds, mousePoint) && !(wParam & MK_LBUTTON) && client()) {
                // When the mouse is not inside the popup menu and the left button isn't down, just
                // repost the message to the web view.

                // Translate the coordinate.
                translatePoint(lParam, m_popup, client()->hostWindow()->platformPageClient());

                ::PostMessage(m_popup, WM_HOST_WINDOW_MOUSEMOVE, wParam, lParam);
                break;
            }

            if ((shouldHotTrack || wParam & MK_LBUTTON) && ::PtInRect(&bounds, mousePoint))
                setFocusedIndex(listIndexAtPoint(mousePoint), true);

            break;
        }
        case WM_LBUTTONDOWN: {
            IntPoint mousePoint(MAKEPOINTS(lParam));
            if (scrollbar()) {
                IntRect scrollBarRect = scrollbar()->frameRect();
                if (scrollBarRect.contains(mousePoint)) {
                    // Put the point into coordinates relative to the scroll bar
                    mousePoint.move(-scrollBarRect.x(), -scrollBarRect.y());
                    PlatformMouseEvent event(hWnd, message, wParam, MAKELPARAM(mousePoint.x(), mousePoint.y()));
                    scrollbar()->mouseDown(event);
                    setScrollbarCapturingMouse(true);
                    break;
                }
            }

            // If the mouse is inside the window, update the focused index. Otherwise, 
            // hide the popup.
            RECT bounds;
            GetClientRect(m_popup, &bounds);
            if (::PtInRect(&bounds, mousePoint))
                setFocusedIndex(listIndexAtPoint(mousePoint), true);
            else
                hide();
            break;
        }
        case WM_LBUTTONUP: {
            IntPoint mousePoint(MAKEPOINTS(lParam));
            if (scrollbar()) {
                IntRect scrollBarRect = scrollbar()->frameRect();
                if (scrollbarCapturingMouse() || scrollBarRect.contains(mousePoint)) {
                    setScrollbarCapturingMouse(false);
                    // Put the point into coordinates relative to the scroll bar
                    mousePoint.move(-scrollBarRect.x(), -scrollBarRect.y());
                    PlatformMouseEvent event(hWnd, message, wParam, MAKELPARAM(mousePoint.x(), mousePoint.y()));
                    scrollbar()->mouseUp(event);
                    // FIXME: This is a hack to work around Scrollbar not invalidating correctly when it doesn't have a parent widget
                    RECT r = scrollBarRect;
                    ::InvalidateRect(popupHandle(), &r, TRUE);
                    break;
                }
            }
            // Only hide the popup if the mouse is inside the popup window.
            RECT bounds;
            GetClientRect(popupHandle(), &bounds);
            if (client() && ::PtInRect(&bounds, mousePoint)) {
                hide();
                int index = focusedIndex();
                if (index >= 0)
                    client()->valueChanged(index);
            }
            break;
        }

        case WM_MOUSEWHEEL: {
            if (!scrollbar())
                break;

            int i = 0;
            for (incrementWheelDelta(GET_WHEEL_DELTA_WPARAM(wParam)); abs(wheelDelta()) >= WHEEL_DELTA; reduceWheelDelta(WHEEL_DELTA)) {
                if (wheelDelta() > 0)
                    ++i;
                else
                    --i;
            }

            ScrollableArea::scroll(i > 0 ? ScrollUp : ScrollDown, ScrollByLine, abs(i));
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT paintInfo;
            ::BeginPaint(popupHandle(), &paintInfo);
            paint(paintInfo.rcPaint, paintInfo.hdc);
            ::EndPaint(popupHandle(), &paintInfo);
            lResult = 0;
            break;
        }
#if !OS(WINCE)
        case WM_PRINTCLIENT:
            paint(clientRect(), (HDC)wParam);
            break;
#endif
        default:
            lResult = DefWindowProc(hWnd, message, wParam, lParam);
    }

    return lResult;
}

}
