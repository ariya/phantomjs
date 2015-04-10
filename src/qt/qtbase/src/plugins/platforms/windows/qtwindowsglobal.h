/****************************************************************************
**
** Copyright (C) 2013 Samuel Gaist <samuel.gaist@edeltech.ch>
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTWINDOWSGLOBAL_H
#define QTWINDOWSGLOBAL_H

#include "qtwindows_additional.h"
#include <QtCore/qnamespace.h>
#ifdef Q_OS_WINCE
#  include "qplatformfunctions_wince.h"
#endif

QT_BEGIN_NAMESPACE

namespace QtWindows
{

enum
{
    WindowEventFlag = 0x10000,
    MouseEventFlag = 0x20000,
    NonClientEventFlag = 0x40000,
    InputMethodEventFlag = 0x80000,
    KeyEventFlag = 0x100000,
    KeyDownEventFlag = 0x200000,
    TouchEventFlag = 0x400000,
    ClipboardEventFlag = 0x800000,
    ApplicationEventFlag = 0x1000000,
    ThemingEventFlag = 0x2000000
};

enum WindowsEventType // Simplify event types
{
    ExposeEvent = WindowEventFlag + 1,
    ActivateWindowEvent = WindowEventFlag + 2,
    DeactivateWindowEvent = WindowEventFlag + 3,
    MouseActivateWindowEvent = WindowEventFlag + 4,
    LeaveEvent = WindowEventFlag + 5,
    CloseEvent = WindowEventFlag + 6,
    ShowEvent = WindowEventFlag + 7,
    HideEvent = WindowEventFlag + 8,
    DestroyEvent = WindowEventFlag + 9,
    MoveEvent = WindowEventFlag + 10,
    ResizeEvent = WindowEventFlag + 12,
    QuerySizeHints = WindowEventFlag + 15,
    CalculateSize = WindowEventFlag + 16,
    FocusInEvent = WindowEventFlag + 17,
    FocusOutEvent = WindowEventFlag + 18,
    WhatsThisEvent = WindowEventFlag + 19,
    MouseEvent = MouseEventFlag + 1,
    MouseWheelEvent = MouseEventFlag + 2,
    CursorEvent = MouseEventFlag + 3,
    TouchEvent = TouchEventFlag + 1,
    NonClientMouseEvent = NonClientEventFlag + MouseEventFlag + 1,
    NonClientHitTest = NonClientEventFlag + 2,
    KeyEvent = KeyEventFlag + 1,
    KeyDownEvent = KeyEventFlag + KeyDownEventFlag + 1,
    KeyboardLayoutChangeEvent = KeyEventFlag + 2,
    InputMethodKeyEvent = InputMethodEventFlag + KeyEventFlag + 1,
    InputMethodKeyDownEvent = InputMethodEventFlag + KeyEventFlag + KeyDownEventFlag + 1,
    ClipboardEvent = ClipboardEventFlag + 1,
    ActivateApplicationEvent = ApplicationEventFlag + 1,
    DeactivateApplicationEvent = ApplicationEventFlag + 2,
    AccessibleObjectFromWindowRequest = ApplicationEventFlag + 3,
    QueryEndSessionApplicationEvent = ApplicationEventFlag + 4,
    EndSessionApplicationEvent = ApplicationEventFlag + 5,
    InputMethodStartCompositionEvent = InputMethodEventFlag + 1,
    InputMethodCompositionEvent = InputMethodEventFlag + 2,
    InputMethodEndCompositionEvent = InputMethodEventFlag + 3,
    InputMethodOpenCandidateWindowEvent = InputMethodEventFlag + 4,
    InputMethodCloseCandidateWindowEvent = InputMethodEventFlag + 5,
    InputMethodRequest = InputMethodEventFlag + 6,
    ThemeChanged = ThemingEventFlag + 1,
    CompositionSettingsChanged = ThemingEventFlag + 2,
    DisplayChangedEvent = 437,
    SettingChangedEvent = DisplayChangedEvent + 1,
    ContextMenu = 123,
    UnknownEvent = 542
};

} // namespace QtWindows

inline QtWindows::WindowsEventType windowsEventType(UINT message, WPARAM wParamIn)
{
    switch (message) {
    case WM_PAINT:
    case WM_ERASEBKGND:
        return QtWindows::ExposeEvent;
    case WM_CLOSE:
        return QtWindows::CloseEvent;
    case WM_DESTROY:
        return QtWindows::DestroyEvent;
    case WM_ACTIVATEAPP:
        return (int)wParamIn ?
        QtWindows::ActivateApplicationEvent : QtWindows::DeactivateApplicationEvent;
    case WM_MOUSEACTIVATE:
        return QtWindows::MouseActivateWindowEvent;
    case WM_ACTIVATE:
        return  LOWORD(wParamIn) == WA_INACTIVE ?
            QtWindows::DeactivateWindowEvent : QtWindows::ActivateWindowEvent;
    case WM_SETCURSOR:
        return QtWindows::CursorEvent;
    case WM_MOUSELEAVE:
        return QtWindows::MouseEvent;
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
        return QtWindows::MouseWheelEvent;
    case WM_MOVE:
        return QtWindows::MoveEvent;
    case WM_SHOWWINDOW:
        return wParamIn ? QtWindows::ShowEvent : QtWindows::HideEvent;
    case WM_SIZE:
        return QtWindows::ResizeEvent;
    case WM_NCCALCSIZE:
        return QtWindows::CalculateSize;
#ifndef Q_OS_WINCE
    case WM_NCHITTEST:
        return QtWindows::NonClientHitTest;
#endif // !Q_OS_WINCE
    case WM_GETMINMAXINFO:
        return QtWindows::QuerySizeHints;
    case WM_KEYDOWN:                        // keyboard event
    case WM_SYSKEYDOWN:
        return QtWindows::KeyDownEvent;
    case WM_KEYUP:
    case WM_SYSKEYUP:
    case WM_CHAR:
        return QtWindows::KeyEvent;
    case WM_IME_CHAR:
        return QtWindows::InputMethodKeyEvent;
    case WM_IME_KEYDOWN:
        return QtWindows::InputMethodKeyDownEvent;
#ifdef WM_INPUTLANGCHANGE
    case WM_INPUTLANGCHANGE:
        return QtWindows::KeyboardLayoutChangeEvent;
#endif // WM_INPUTLANGCHANGE
    case WM_TOUCH:
        return QtWindows::TouchEvent;
    case WM_CHANGECBCHAIN:
    case WM_DRAWCLIPBOARD:
    case WM_RENDERFORMAT:
    case WM_RENDERALLFORMATS:
    case WM_DESTROYCLIPBOARD:
        return QtWindows::ClipboardEvent;
    case WM_IME_STARTCOMPOSITION:
        return QtWindows::InputMethodStartCompositionEvent;
    case WM_IME_ENDCOMPOSITION:
        return QtWindows::InputMethodEndCompositionEvent;
    case WM_IME_COMPOSITION:
        return QtWindows::InputMethodCompositionEvent;
    case WM_IME_REQUEST:
        return QtWindows::InputMethodRequest;
    case WM_IME_NOTIFY:
         switch (int(wParamIn)) {
         case IMN_OPENCANDIDATE:
             return QtWindows::InputMethodOpenCandidateWindowEvent;
         case IMN_CLOSECANDIDATE:
             return QtWindows::InputMethodCloseCandidateWindowEvent;
         default:
             break;
         }
    case WM_GETOBJECT:
        return QtWindows::AccessibleObjectFromWindowRequest;
    case WM_SETFOCUS:
        return QtWindows::FocusInEvent;
    case WM_KILLFOCUS:
        return QtWindows::FocusOutEvent;
    // Among other things, WM_SETTINGCHANGE happens when the taskbar is moved
    // and therefore the "working area" changes.
    // http://msdn.microsoft.com/en-us/library/ms695534(v=vs.85).aspx
    case WM_SETTINGCHANGE:
        return QtWindows::SettingChangedEvent;
    case WM_DISPLAYCHANGE:
        return QtWindows::DisplayChangedEvent;
    case WM_THEMECHANGED:
#ifdef WM_SYSCOLORCHANGE // Windows 7: Handle color change as theme change (QTBUG-34170).
    case WM_SYSCOLORCHANGE:
#endif
        return QtWindows::ThemeChanged;
    case WM_DWMCOMPOSITIONCHANGED:
        return QtWindows::CompositionSettingsChanged;
#ifndef QT_NO_CONTEXTMENU
    case WM_CONTEXTMENU:
        return QtWindows::ContextMenu;
#endif
    case WM_SYSCOMMAND:
#ifndef Q_OS_WINCE
        if ((wParamIn & 0xfff0) == SC_CONTEXTHELP)
            return QtWindows::WhatsThisEvent;
#endif
        break;
#if !defined(Q_OS_WINCE) && !defined(QT_NO_SESSIONMANAGER)
    case WM_QUERYENDSESSION:
        return QtWindows::QueryEndSessionApplicationEvent;
    case WM_ENDSESSION:
        return QtWindows::EndSessionApplicationEvent;
#endif
    default:
        break;
    }
    if (message >= WM_NCMOUSEMOVE && message <= WM_NCMBUTTONDBLCLK)
        return QtWindows::NonClientMouseEvent; //
    if ((message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)
         || (message >= WM_XBUTTONDOWN && message <= WM_XBUTTONDBLCLK))
        return QtWindows::MouseEvent;
    return QtWindows::UnknownEvent;
}

QT_END_NAMESPACE

#endif // QTWINDOWSGLOBAL_H
