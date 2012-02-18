/*
 * Copyright (C) 2004, 2006, 2007, 2010 Apple Inc. All rights reserved.
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

#import "config.h"
#import "PlatformKeyboardEvent.h"

#if PLATFORM(MAC)

#import "KeyEventCocoa.h"
#import "Logging.h"
#import "WindowsKeyboardCodes.h"
#import <Carbon/Carbon.h>

using namespace WTF;

namespace WebCore {

static bool isKeypadEvent(NSEvent* event)
{
    // Check that this is the type of event that has a keyCode.
    switch ([event type]) {
        case NSKeyDown:
        case NSKeyUp:
        case NSFlagsChanged:
            break;
        default:
            return false;
    }

    if ([event modifierFlags] & NSNumericPadKeyMask)
        return true;

    switch ([event keyCode]) {
        case 71: // Clear
        case 81: // =
        case 75: // /
        case 67: // *
        case 78: // -
        case 69: // +
        case 76: // Enter
        case 65: // .
        case 82: // 0
        case 83: // 1
        case 84: // 2
        case 85: // 3
        case 86: // 4
        case 87: // 5
        case 88: // 6
        case 89: // 7
        case 91: // 8
        case 92: // 9
            return true;
     }

     return false;
}

static inline bool isKeyUpEvent(NSEvent *event)
{
    if ([event type] != NSFlagsChanged)
        return [event type] == NSKeyUp;
    // FIXME: This logic fails if the user presses both Shift keys at once, for example:
    // we treat releasing one of them as keyDown.
    switch ([event keyCode]) {
        case 54: // Right Command
        case 55: // Left Command
            return ([event modifierFlags] & NSCommandKeyMask) == 0;

        case 57: // Capslock
            return ([event modifierFlags] & NSAlphaShiftKeyMask) == 0;

        case 56: // Left Shift
        case 60: // Right Shift
            return ([event modifierFlags] & NSShiftKeyMask) == 0;

        case 58: // Left Alt
        case 61: // Right Alt
            return ([event modifierFlags] & NSAlternateKeyMask) == 0;

        case 59: // Left Ctrl
        case 62: // Right Ctrl
            return ([event modifierFlags] & NSControlKeyMask) == 0;

        case 63: // Function
            return ([event modifierFlags] & NSFunctionKeyMask) == 0;
    }
    return false;
}

static inline String textFromEvent(NSEvent* event)
{
    if ([event type] == NSFlagsChanged)
        return "";
    return [event characters];
}


static inline String unmodifiedTextFromEvent(NSEvent* event)
{
    if ([event type] == NSFlagsChanged)
        return "";
    return [event charactersIgnoringModifiers];
}

static String keyIdentifierForKeyEvent(NSEvent* event)
{
    if ([event type] == NSFlagsChanged)
        switch ([event keyCode]) {
            case 54: // Right Command
            case 55: // Left Command
                return "Meta";

            case 57: // Capslock
                return "CapsLock";

            case 56: // Left Shift
            case 60: // Right Shift
                return "Shift";

            case 58: // Left Alt
            case 61: // Right Alt
                return "Alt";

            case 59: // Left Ctrl
            case 62: // Right Ctrl
                return "Control";

            default:
                ASSERT_NOT_REACHED();
                return "";
        }

    NSString *s = [event charactersIgnoringModifiers];
    if ([s length] != 1) {
        LOG(Events, "received an unexpected number of characters in key event: %u", [s length]);
        return "Unidentified";
    }
    return keyIdentifierForCharCode([s characterAtIndex:0]);
}

static int windowsKeyCodeForKeyEvent(NSEvent *event)
{
    int code = 0;
    // There are several kinds of characters for which we produce key code from char code:
    // 1. Roman letters. Windows keyboard layouts affect both virtual key codes and character codes for these,
    //    so e.g. 'A' gets the same keyCode on QWERTY, AZERTY or Dvorak layouts.
    // 2. Keys for which there is no known Mac virtual key codes, like PrintScreen.
    // 3. Certain punctuation keys. On Windows, these are also remapped depending on current keyboard layout,
    //    but see comment in windowsKeyCodeForCharCode().
    if ([event type] == NSKeyDown || [event type] == NSKeyUp) {
        // Cmd switches Roman letters for Dvorak-QWERTY layout, so try modified characters first.
        NSString* s = [event characters];
        code = [s length] > 0 ? windowsKeyCodeForCharCode([s characterAtIndex:0]) : 0;
        if (code)
            return code;

        // Ctrl+A on an AZERTY keyboard would get VK_Q keyCode if we relied on -[NSEvent keyCode] below.
        s = [event charactersIgnoringModifiers];
        code = [s length] > 0 ? windowsKeyCodeForCharCode([s characterAtIndex:0]) : 0;
        if (code)
            return code;
    }

    // Map Mac virtual key code directly to Windows one for any keys not handled above.
    // E.g. the key next to Caps Lock has the same Event.keyCode on U.S. keyboard ('A') and on Russian keyboard (CYRILLIC LETTER EF).
    return windowsKeyCodeForKeyCode([event keyCode]);
}

PlatformKeyboardEvent::PlatformKeyboardEvent(NSEvent *event)
    : m_type(isKeyUpEvent(event) ? PlatformKeyboardEvent::KeyUp : PlatformKeyboardEvent::KeyDown)
    , m_text(textFromEvent(event))
    , m_unmodifiedText(unmodifiedTextFromEvent(event))
    , m_keyIdentifier(keyIdentifierForKeyEvent(event))
    , m_autoRepeat(([event type] != NSFlagsChanged) && [event isARepeat])
    , m_windowsVirtualKeyCode(windowsKeyCodeForKeyEvent(event))
    , m_nativeVirtualKeyCode([event keyCode])
    , m_isKeypad(isKeypadEvent(event))
    , m_shiftKey([event modifierFlags] & NSShiftKeyMask)
    , m_ctrlKey([event modifierFlags] & NSControlKeyMask)
    , m_altKey([event modifierFlags] & NSAlternateKeyMask)
    , m_metaKey([event modifierFlags] & NSCommandKeyMask)
    , m_macEvent(event)
{
    // Always use 13 for Enter/Return -- we don't want to use AppKit's different character for Enter.
    if (m_windowsVirtualKeyCode == VK_RETURN) {
        m_text = "\r";
        m_unmodifiedText = "\r";
    }

    // AppKit sets text to "\x7F" for backspace, but the correct KeyboardEvent character code is 8.
    if (m_windowsVirtualKeyCode == VK_BACK) {
        m_text = "\x8";
        m_unmodifiedText = "\x8";
    }

    // Always use 9 for Tab -- we don't want to use AppKit's different character for shift-tab.
    if (m_windowsVirtualKeyCode == VK_TAB) {
        m_text = "\x9";
        m_unmodifiedText = "\x9";
    }
}

void PlatformKeyboardEvent::disambiguateKeyDownEvent(Type type, bool backwardCompatibilityMode)
{
    // Can only change type from KeyDown to RawKeyDown or Char, as we lack information for other conversions.
    ASSERT(m_type == KeyDown);
    ASSERT(type == RawKeyDown || type == Char);
    m_type = type;
    if (backwardCompatibilityMode)
        return;

    if (type == RawKeyDown) {
        m_text = String();
        m_unmodifiedText = String();
    } else {
        m_keyIdentifier = String();
        m_windowsVirtualKeyCode = 0;
        if (m_text.length() == 1 && (m_text[0U] >= 0xF700 && m_text[0U] <= 0xF7FF)) {
            // According to NSEvents.h, OpenStep reserves the range 0xF700-0xF8FF for function keys. However, some actual private use characters
            // happen to be in this range, e.g. the Apple logo (Option+Shift+K).
            // 0xF7FF is an arbitrary cut-off.
            m_text = String();
            m_unmodifiedText = String();
        }
    }
}

bool PlatformKeyboardEvent::currentCapsLockState()
{
    return GetCurrentKeyModifiers() & alphaLock;
}

void PlatformKeyboardEvent::getCurrentModifierState(bool& shiftKey, bool& ctrlKey, bool& altKey, bool& metaKey)
{
    UInt32 currentModifiers = GetCurrentKeyModifiers();
    shiftKey = currentModifiers & ::shiftKey;
    ctrlKey = currentModifiers & ::controlKey;
    altKey = currentModifiers & ::optionKey;
    metaKey = currentModifiers & ::cmdKey;
}

}

#endif // PLATFORM(MAC)
