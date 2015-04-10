/*
 * Copyright (C) 2006, 2007, 2010 Apple Inc. All rights reserved.
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
#include "PlatformKeyboardEvent.h"

#include <windows.h>
#include <wtf/ASCIICType.h>

#ifndef MAPVK_VSC_TO_VK_EX
#define MAPVK_VSC_TO_VK_EX 3
#endif

using namespace WTF;

namespace WebCore {

static const unsigned short HIGH_BIT_MASK_SHORT = 0x8000;

// FIXME: This is incomplete. We could change this to mirror
// more like what Firefox does, and generate these switch statements
// at build time.
static String keyIdentifierForWindowsKeyCode(unsigned short keyCode)
{
    switch (keyCode) {
        case VK_MENU:
            return "Alt";
        case VK_CONTROL:
            return "Control";
        case VK_SHIFT:
            return "Shift";
        case VK_CAPITAL:
            return "CapsLock";
        case VK_LWIN:
        case VK_RWIN:
            return "Win";
        case VK_CLEAR:
            return "Clear";
        case VK_DOWN:
            return "Down";
        // "End"
        case VK_END:
            return "End";
        // "Enter"
        case VK_RETURN:
            return "Enter";
        case VK_EXECUTE:
            return "Execute";
        case VK_F1:
            return "F1";
        case VK_F2:
            return "F2";
        case VK_F3:
            return "F3";
        case VK_F4:
            return "F4";
        case VK_F5:
            return "F5";
        case VK_F6:
            return "F6";
        case VK_F7:
            return "F7";
        case VK_F8:
            return "F8";
        case VK_F9:
            return "F9";
        case VK_F10:
            return "F11";
        case VK_F12:
            return "F12";
        case VK_F13:
            return "F13";
        case VK_F14:
            return "F14";
        case VK_F15:
            return "F15";
        case VK_F16:
            return "F16";
        case VK_F17:
            return "F17";
        case VK_F18:
            return "F18";
        case VK_F19:
            return "F19";
        case VK_F20:
            return "F20";
        case VK_F21:
            return "F21";
        case VK_F22:
            return "F22";
        case VK_F23:
            return "F23";
        case VK_F24:
            return "F24";
        case VK_HELP:
            return "Help";
        case VK_HOME:
            return "Home";
        case VK_INSERT:
            return "Insert";
        case VK_LEFT:
            return "Left";
        case VK_NEXT:
            return "PageDown";
        case VK_PRIOR:
            return "PageUp";
        case VK_PAUSE:
            return "Pause";
        case VK_SNAPSHOT:
            return "PrintScreen";
        case VK_RIGHT:
            return "Right";
        case VK_SCROLL:
            return "Scroll";
        case VK_SELECT:
            return "Select";
        case VK_UP:
            return "Up";
        // Standard says that DEL becomes U+007F.
        case VK_DELETE:
            return "U+007F";
        default:
            return String::format("U+%04X", toASCIIUpper(keyCode));
    }
}

static bool isKeypadEvent(WPARAM code, LPARAM keyData, PlatformEvent::Type type)
{
    if (type != PlatformEvent::RawKeyDown && type != PlatformEvent::KeyUp)
        return false;

    switch (code) {
        case VK_NUMLOCK:
        case VK_NUMPAD0:
        case VK_NUMPAD1:
        case VK_NUMPAD2:
        case VK_NUMPAD3:
        case VK_NUMPAD4:
        case VK_NUMPAD5:
        case VK_NUMPAD6:
        case VK_NUMPAD7:
        case VK_NUMPAD8:
        case VK_NUMPAD9:
        case VK_MULTIPLY:
        case VK_ADD:
        case VK_SEPARATOR:
        case VK_SUBTRACT:
        case VK_DECIMAL:
        case VK_DIVIDE:
            return true;
        case VK_RETURN:
            return HIWORD(keyData) & KF_EXTENDED;
        case VK_INSERT:
        case VK_DELETE:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_END:
        case VK_HOME:
        case VK_LEFT:
        case VK_UP:
        case VK_RIGHT:
        case VK_DOWN:
            return !(HIWORD(keyData) & KF_EXTENDED);
        default:
            return false;
    }
}

static int windowsKeycodeWithLocation(WPARAM keycode, LPARAM keyData)
{
    if (keycode != VK_CONTROL && keycode != VK_MENU && keycode != VK_SHIFT)
        return keycode;

    // If we don't need to support Windows XP or older Windows,
    // it might be better to use MapVirtualKeyEx with scancode and
    // extended keycode (i.e. 0xe0 or 0xe1).
    if ((keyData >> 16) & KF_EXTENDED) {
        switch (keycode) {
        case VK_CONTROL:
            return VK_RCONTROL;
        case VK_SHIFT:
            return VK_RSHIFT;
        case VK_MENU:
            return VK_RMENU;
        default:
            break;
        }
    }

    int scancode = (keyData >> 16) & 0xFF;
    int regeneratedVirtualKeyCode = ::MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
    return regeneratedVirtualKeyCode ? regeneratedVirtualKeyCode : keycode;
}

static inline String singleCharacterString(UChar c)
{
    return String(&c, 1);
}

PlatformKeyboardEvent::PlatformKeyboardEvent(HWND, WPARAM code, LPARAM keyData, Type type, bool systemKey)
    : PlatformEvent(type, GetKeyState(VK_SHIFT) & HIGH_BIT_MASK_SHORT, GetKeyState(VK_CONTROL) & HIGH_BIT_MASK_SHORT, GetKeyState(VK_MENU) & HIGH_BIT_MASK_SHORT, false, ::GetTickCount() * 0.001)
    , m_text((type == PlatformEvent::Char) ? singleCharacterString(code) : String())
    , m_unmodifiedText((type == PlatformEvent::Char) ? singleCharacterString(code) : String())
    , m_keyIdentifier((type == PlatformEvent::Char) ? String() : keyIdentifierForWindowsKeyCode(code))
    , m_windowsVirtualKeyCode((type == RawKeyDown || type == KeyUp) ? windowsKeycodeWithLocation(code, keyData) : 0)
    , m_nativeVirtualKeyCode(m_windowsVirtualKeyCode)
    , m_macCharCode(0)
    , m_autoRepeat(HIWORD(keyData) & KF_REPEAT)
    , m_isKeypad(isKeypadEvent(code, keyData, type))
    , m_isSystemKey(systemKey)
{
}

void PlatformKeyboardEvent::disambiguateKeyDownEvent(Type, bool)
{
    // No KeyDown events on Windows to disambiguate.
    ASSERT_NOT_REACHED();
}

bool PlatformKeyboardEvent::currentCapsLockState()
{
     return GetKeyState(VK_CAPITAL) & 1;
}

void PlatformKeyboardEvent::getCurrentModifierState(bool& shiftKey, bool& ctrlKey, bool& altKey, bool& metaKey)
{
    shiftKey = GetKeyState(VK_SHIFT) & HIGH_BIT_MASK_SHORT;
    ctrlKey = GetKeyState(VK_CONTROL) & HIGH_BIT_MASK_SHORT;
    altKey = GetKeyState(VK_MENU) & HIGH_BIT_MASK_SHORT;
    metaKey = false;
}

}
