/*
 * Copyright (C) 2009, 2010, 2011 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "PlatformKeyboardEvent.h"

#include "NotImplemented.h"
#include "WindowsKeyboardCodes.h"

#include <BlackBerryPlatformKeyboardEvent.h>
#include <BlackBerryPlatformLog.h>
#include <BlackBerryPlatformScreen.h>
#include <wtf/CurrentTime.h>
#include <wtf/text/CString.h>

namespace WebCore {

static String keyIdentifierForBlackBerryCharacter(unsigned character)
{
    switch (character) {
    case KEYCODE_RETURN:
    case KEYCODE_KP_ENTER:
        return "Enter";
    case KEYCODE_BACKSPACE:
        return "U+0008";
    case KEYCODE_DELETE:
        return "U+007F";
    case KEYCODE_ESCAPE:
        return "Escape";
    case KEYCODE_PAUSE:
        return "Pause";
    case KEYCODE_PRINT:
        return "PrintScreen";
    case KEYCODE_TAB:
    case KEYCODE_BACK_TAB:
        return "U+0009";
    case KEYCODE_LEFT:
    case KEYCODE_KP_LEFT:
        return "Left";
    case KEYCODE_RIGHT:
    case KEYCODE_KP_RIGHT:
        return "Right";
    case KEYCODE_UP:
    case KEYCODE_KP_UP:
        return "Up";
    case KEYCODE_DOWN:
    case KEYCODE_KP_DOWN:
        return "Down";
    case KEYCODE_KP_DELETE:
        return "U+007F";
    case KEYCODE_MENU:
    case KEYCODE_LEFT_ALT:
    case KEYCODE_RIGHT_ALT:
        return "Alt";
    case KEYCODE_HOME:
    case KEYCODE_KP_HOME:
        return "Home";
    case KEYCODE_INSERT:
    case KEYCODE_KP_INSERT:
        return "Insert";
    case KEYCODE_PG_UP:
    case KEYCODE_KP_PG_UP:
        return "PageUp";
    case KEYCODE_PG_DOWN:
    case KEYCODE_KP_PG_DOWN:
        return "PageDown";
    case KEYCODE_END:
    case KEYCODE_KP_END:
        return "End";
    case KEYCODE_F1:
        return "F1";
    case KEYCODE_F2:
        return "F2";
    case KEYCODE_F3:
        return "F3";
    case KEYCODE_F4:
        return "F4";
    case KEYCODE_F5:
        return "F5";
    case KEYCODE_F6:
        return "F6";
    case KEYCODE_F7:
        return "F7";
    case KEYCODE_F8:
        return "F8";
    case KEYCODE_F9:
        return "F9";
    case KEYCODE_F10:
        return "F10";
    case KEYCODE_F11:
        return "F11";
    case KEYCODE_F12:
        return "F12";
    default:
        return String::format("U+%04X", WTF::toASCIIUpper(character));
    }
}

static int windowsKeyCodeForBlackBerryKeycode(unsigned keycode)
{
    switch (keycode) {
    case KEYCODE_RETURN:
    case KEYCODE_KP_ENTER:
        return VK_RETURN; // (0D) Return key
    case KEYCODE_BACKSPACE:
        return VK_BACK; // (08) BACKSPACE key
    case KEYCODE_DELETE:
        return VK_DELETE; // (2E) DEL key
    case KEYCODE_ESCAPE:
        return VK_ESCAPE;
    case KEYCODE_SPACE:
        return VK_SPACE;
    case '0':
    case ')':
        return VK_0;
    case '1':
    case '!':
        return VK_1;
    case '2':
    case '@':
        return VK_2;
    case '3':
    case '#':
        return VK_3;
    case '4':
    case '$':
        return VK_4;
    case '5':
    case '%':
        return VK_5;
    case '6':
    case '^':
        return VK_6;
    case '7':
    case '&':
        return VK_7;
    case '8':
    case '*':
        return VK_8;
    case '9':
    case '(':
        return VK_9;
    case 'a':
    case 'A':
        return VK_A;
    case 'b':
    case 'B':
        return VK_B;
    case 'c':
    case 'C':
        return VK_C;
    case 'd':
    case 'D':
        return VK_D;
    case 'e':
    case 'E':
        return VK_E;
    case 'f':
    case 'F':
        return VK_F;
    case 'g':
    case 'G':
        return VK_G;
    case 'h':
    case 'H':
        return VK_H;
    case 'i':
    case 'I':
        return VK_I;
    case 'j':
    case 'J':
        return VK_J;
    case 'k':
    case 'K':
        return VK_K;
    case 'l':
    case 'L':
        return VK_L;
    case 'm':
    case 'M':
        return VK_M;
    case 'n':
    case 'N':
        return VK_N;
    case 'o':
    case 'O':
        return VK_O;
    case 'p':
    case 'P':
        return VK_P;
    case 'q':
    case 'Q':
        return VK_Q;
    case 'r':
    case 'R':
        return VK_R;
    case 's':
    case 'S':
        return VK_S;
    case 't':
    case 'T':
        return VK_T;
    case 'u':
    case 'U':
        return VK_U;
    case 'v':
    case 'V':
        return VK_V;
    case 'w':
    case 'W':
        return VK_W;
    case 'x':
    case 'X':
        return VK_X;
    case 'y':
    case 'Y':
        return VK_Y;
    case 'z':
    case 'Z':
        return VK_Z;
    case '+':
    case '=':
        return VK_OEM_PLUS;
    case '-':
    case '_':
        return VK_OEM_MINUS;
    case '<':
    case ',':
        return VK_OEM_COMMA;
    case '>':
    case '.':
        return VK_OEM_PERIOD;
    case ':':
    case ';':
        return VK_OEM_1;
    case '/':
    case '?':
        return VK_OEM_2;
    case '~':
    case '`':
        return VK_OEM_3;
    case '{':
    case '[':
        return VK_OEM_4;
    case '|':
    case '\\':
        return VK_OEM_5;
    case '}':
    case ']':
        return VK_OEM_6;
    case '"':
    case '\'':
        return VK_OEM_7;
    case KEYCODE_PAUSE:
        return VK_PAUSE;
    case KEYCODE_PRINT:
        return VK_PRINT;
    case KEYCODE_SCROLL_LOCK:
        return VK_SCROLL;
    case KEYCODE_TAB:
    case KEYCODE_BACK_TAB:
        return VK_TAB;
    case KEYCODE_LEFT:
    case KEYCODE_KP_LEFT:
        return VK_LEFT;
    case KEYCODE_RIGHT:
    case KEYCODE_KP_RIGHT:
        return VK_RIGHT;
    case KEYCODE_UP:
    case KEYCODE_KP_UP:
        return VK_UP;
    case KEYCODE_DOWN:
    case KEYCODE_KP_DOWN:
        return VK_DOWN;
    case KEYCODE_KP_DELETE:
        return VK_DELETE;
    case KEYCODE_MENU:
    case KEYCODE_LEFT_ALT:
    case KEYCODE_RIGHT_ALT:
        return VK_MENU;
    case KEYCODE_HOME:
    case KEYCODE_KP_HOME:
        return VK_HOME;
    case KEYCODE_INSERT:
    case KEYCODE_KP_INSERT:
        return VK_INSERT;
    case KEYCODE_PG_UP:
    case KEYCODE_KP_PG_UP:
        return VK_PRIOR;
    case KEYCODE_PG_DOWN:
    case KEYCODE_KP_PG_DOWN:
        return VK_NEXT;
    case KEYCODE_END:
    case KEYCODE_KP_END:
        return VK_END;
    case KEYCODE_CAPS_LOCK:
        return VK_CAPITAL;
    case KEYCODE_LEFT_SHIFT:
    case KEYCODE_RIGHT_SHIFT:
        return VK_SHIFT;
    case KEYCODE_LEFT_CTRL:
    case KEYCODE_RIGHT_CTRL:
        return VK_CONTROL;
    case KEYCODE_NUM_LOCK:
        return VK_NUMLOCK;
    case KEYCODE_KP_PLUS:
        return VK_ADD;
    case KEYCODE_KP_MINUS:
        return VK_SUBTRACT;
    case KEYCODE_KP_MULTIPLY:
        return VK_MULTIPLY;
    case KEYCODE_KP_DIVIDE:
        return VK_DIVIDE;
    case KEYCODE_KP_FIVE:
        return VK_NUMPAD5;
    case KEYCODE_F1:
        return VK_F1;
    case KEYCODE_F2:
        return VK_F2;
    case KEYCODE_F3:
        return VK_F3;
    case KEYCODE_F4:
        return VK_F4;
    case KEYCODE_F5:
        return VK_F5;
    case KEYCODE_F6:
        return VK_F6;
    case KEYCODE_F7:
        return VK_F7;
    case KEYCODE_F8:
        return VK_F8;
    case KEYCODE_F9:
        return VK_F9;
    case KEYCODE_F10:
        return VK_F10;
    case KEYCODE_F11:
        return VK_F11;
    case KEYCODE_F12:
        return VK_F12;
    default:
        return 0;
    }
}

unsigned adjustCharacterFromOS(unsigned character)
{
    // Use windows key character as ASCII value when possible to enhance readability.
    switch (character) {
    case KEYCODE_BACKSPACE:
        return VK_BACK;
    case KEYCODE_KP_DELETE:
    case KEYCODE_DELETE:
        return 0x7f;
    case KEYCODE_ESCAPE:
        return VK_ESCAPE;
    case KEYCODE_RETURN:
    case KEYCODE_KP_ENTER:
        return VK_RETURN;
    case KEYCODE_KP_PLUS:
        return VK_ADD;
    case KEYCODE_KP_MINUS:
        return VK_SUBTRACT;
    case KEYCODE_KP_MULTIPLY:
        return VK_MULTIPLY;
    case KEYCODE_KP_DIVIDE:
        return VK_DIVIDE;
    case KEYCODE_KP_FIVE:
    case KEYCODE_HOME:
    case KEYCODE_KP_HOME:
    case KEYCODE_END:
    case KEYCODE_KP_END:
    case KEYCODE_INSERT:
    case KEYCODE_KP_INSERT:
    case KEYCODE_PG_UP:
    case KEYCODE_KP_PG_UP:
    case KEYCODE_PG_DOWN:
    case KEYCODE_KP_PG_DOWN:
    case KEYCODE_MENU:
    case KEYCODE_LEFT_ALT:
    case KEYCODE_RIGHT_ALT:
    case KEYCODE_CAPS_LOCK:
    case KEYCODE_LEFT_SHIFT:
    case KEYCODE_RIGHT_SHIFT:
    case KEYCODE_LEFT_CTRL:
    case KEYCODE_RIGHT_CTRL:
    case KEYCODE_NUM_LOCK:
    case KEYCODE_PRINT:
    case KEYCODE_SCROLL_LOCK:
    case KEYCODE_PAUSE:
    case KEYCODE_F1:
    case KEYCODE_F2:
    case KEYCODE_F3:
    case KEYCODE_F4:
    case KEYCODE_F5:
    case KEYCODE_F6:
    case KEYCODE_F7:
    case KEYCODE_F8:
    case KEYCODE_F9:
    case KEYCODE_F10:
    case KEYCODE_F11:
    case KEYCODE_F12:
    case KEYCODE_LEFT:
    case KEYCODE_RIGHT:
    case KEYCODE_UP:
    case KEYCODE_DOWN:
        return 0;
    default:
        break;
    }
    return character;
}

static inline PlatformKeyboardEvent::Type toWebCorePlatformKeyboardEventType(const BlackBerry::Platform::KeyboardEvent::Type type)
{
    switch (type) {
    case BlackBerry::Platform::KeyboardEvent::KeyDown:
        return PlatformEvent::KeyDown;
    case BlackBerry::Platform::KeyboardEvent::KeyUp:
        return PlatformEvent::KeyUp;
    case BlackBerry::Platform::KeyboardEvent::KeyChar:
    default:
        return PlatformEvent::Char;
    }
}

PlatformKeyboardEvent::PlatformKeyboardEvent(const BlackBerry::Platform::KeyboardEvent& event)
    : PlatformEvent(toWebCorePlatformKeyboardEventType(event.type()), event.shiftActive() || (event.character() == KEYCODE_BACK_TAB), event.ctrlActive(), event.altActive(), event.metaActive(), currentTime())
    , m_keyIdentifier(keyIdentifierForBlackBerryCharacter(event.character()))
    , m_windowsVirtualKeyCode(windowsKeyCodeForBlackBerryKeycode(event.keycode() ? event.keycode() : event.character())) // if keycode isn't valid, use character as it's unconverted.
    , m_autoRepeat(false)
    , m_isKeypad(false)
    , m_unmodifiedCharacter(event.character())
{
    unsigned character = adjustCharacterFromOS(event.character());
    UChar utf16[3] = {0};
    int destLength = 0;
    UErrorCode ec = U_ZERO_ERROR;
    u_strFromUTF32(utf16, 3, &destLength, reinterpret_cast<UChar32*>(&character), 1, &ec);
    if (ec) {
        BBLOG(BlackBerry::Platform::LogLevelCritical, "PlatformKeyboardEvent::PlatformKeyboardEvent Error converting 0x%x to string ec (%d).", character, ec);
        return;
    }
    m_text = String(utf16, destLength);
    m_unmodifiedText = m_text;

    if (event.character() == KEYCODE_BACK_TAB)
        m_modifiers |= ShiftKey; // BackTab should be treated as Shift + Tab.

    BBLOG(BlackBerry::Platform::LogLevelInfo, "Keyboard event received text=%lc, keyIdentifier=%s, windowsVirtualKeyCode=%d", event.character(), m_keyIdentifier.latin1().data(), m_windowsVirtualKeyCode);
}

bool PlatformKeyboardEvent::currentCapsLockState()
{
    notImplemented();
    return false;
}

void PlatformKeyboardEvent::disambiguateKeyDownEvent(PlatformEvent::Type type, bool backwardCompatibilityMode)
{
    // Can only change type from KeyDown to RawKeyDown or Char, as we lack information for other conversions.
    ASSERT(m_type == PlatformEvent::KeyDown);
    m_type = type;

    if (backwardCompatibilityMode)
        return;

    if (type == PlatformEvent::RawKeyDown) {
        m_text = String();
        m_unmodifiedText = String();
    } else {
        m_keyIdentifier = String();
        m_windowsVirtualKeyCode = 0;
    }
}

void PlatformKeyboardEvent::getCurrentModifierState(bool& shiftKey, bool& ctrlKey, bool& altKey, bool& metaKey)
{
    int modifiers = BlackBerry::Platform::Graphics::Screen::primaryScreen()->getCurrentModifiersState();
    shiftKey = modifiers & KEYMOD_SHIFT;
    ctrlKey = modifiers & KEYMOD_CTRL;
    altKey = modifiers & KEYMOD_ALT;
    metaKey = false;
}

} // namespace WebCore
